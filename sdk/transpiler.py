import re
import dis
import textwrap
from types import CodeType

from .bytecode import *
from .util import error

def c_bool(x: bool):
    return "true" if x else "false"

def sanitize_identifier(x: str):
    return re.sub(r"[^_A-Za-z0-9]", "__", x)

class TranslationUnit:
    """
    Represents a single translation unit, which contains C function bodies that
    represent Python code fragments (e.g. module-level code, functions, methods, etc.)
    """

    def __init__(self):
        self.transpiled: dict[str, str] = {}
        "Stores mappings between mangled function names and their C function bodies."

        self.entrypoint: str | None = None
        "The mangled name of the function that is the entrypoint (main function) of the kernel."

        self.known_names: set[str] = set()
        "Stores all known names that may be either globals or locals."

    def mangle(self, fn: CodeType):
        return "pyfn__" + sanitize_identifier(fn.co_qualname)
    
    def mangle_global(self, name: str):
        return "pyglobal__" + sanitize_identifier(name)

    def translate(self, fn: CodeType, is_entrypoint = False):
        """
        Transpiles the given code fragment into a C function, returning its mangled name.
        The function itself can be retrieved via `self.transpiled`. If the function was already
        transpiled before, this function does nothing.

        `is_entrypoint` should be set to `True` if the fragment is supposed to be ran after
        system startup.
        """
        mangled_name = self.mangle(fn)
        if mangled_name in self.transpiled:
            return mangled_name

        defined_preprocessor_syms = ["PY__EXCEPTION_HANDLER_LABEL"]

        body = [
            f"// Function {fn.co_qualname}, declared on line {fn.co_firstlineno}",
            f"void* stack[{fn.co_stacksize + 1}] = {{}};",
            f"int stack_current = -1;",
            f"pyobj_t* caught_exception = NULL;",
            f"#define PY__EXCEPTION_HANDLER_LABEL L_uncaught_exception"
        ]

        body.append("")
        body.append("// (constants start)")
        for i, const in enumerate(fn.co_consts):
            if type(const) is str:
                body.append(f'static const pyobj_t const_{i} = {{ .type = &py_type_str, .as_str = "{const.replace("\n", "\\n").replace("\r", "")}" }};')
            elif type(const) is int:
                body.append(f"static const pyobj_t const_{i} = {{ .type = &py_type_int, .as_int = {const} }};")
            elif type(const) is float:
                body.append(f"static const pyobj_t const_{i} = {{ .type = &py_type_float, .as_float = {const} }};")
            elif type(const) is bool:
                body.append(f"#define const_{i} (py_true)" if const else f"#define const_{i} (py_false)")
                defined_preprocessor_syms.append(f"const_{i}")
            elif type(const).__name__ == "code":
                # If we have a 'code' constant, this means that this is a callable.
                code: CodeType = const
                target_fn = self.translate(code)
                body.append(f"static const pyobj_t const_{i} = {{ .type = &py_type_callable, .as_callable = &{target_fn} }};")
            elif const is None:
                body.append(f"#define const_{i} (py_none)")
                defined_preprocessor_syms.append(f"const_{i}")
            else:
                error(f"unknown constant type '{type(const).__name__}'!")
                error(f"the value of the constant is {const}")
                raise Exception(f"Unknown constant type: {type(const).__name__}")
            
        body.append("// (constants end)")
        body.append("")

        # We try to use local variables for actual locals if this isn't the entrypoint function.
        # If it *is* the entrypoint function, then locals are equivalent to globals.
        if not is_entrypoint:
            for name in fn.co_names:
                body.append(f"pyobj_t* loc_{name} = NULL;")

        # All names might be global. For example:
        #   def ex():
        #       print(a)
        #       a = 2
        #       print(a)
        # First, "a" would be found in the global symbol table and printed out. Then, it
        # would get defined in the local symbol table, and we'd print the local instead.
        for name in fn.co_names:
            self.known_names.add(name)

        body.append("// (function body start)")
        
        # This maps label indices (instr.label) to offsets.
        labels = sorted(dis.findlabels(fn.co_code))

        def label_by_offset(offset: int):
            "Gets the label at the given bytecode offset."
            return next(f"L{i + 1}" for i, x in enumerate(labels) if x == offset)

        bytecode = dis.Bytecode(fn)
        for instr in bytecode:
            body.append(f"// {str(instr).strip()}")

            if instr.label != None:
                body.append(f"L{instr.label}:")

            # Important to mention: stack_current points to the stack slot that will be
            # popped next. When pushing, it needs to be incremented BEFORE writing to the
            # slot.

            match instr.opname:
                case "RESUME":
                    pass # no-op
                case "PUSH_NULL":
                    body.append("stack[++stack_current] = NULL;")
                case "LOAD_NAME":
                    name = fn.co_names[instr.arg]

                    if is_entrypoint:
                        # Locals are equivalent to globals in the entrypoint.
                        body.append(f'stack[++stack_current] = {self.mangle_global(name)};')
                    else:
                        # If loc_{name} is NULL, that means that the local of that name isn't defined, so we
                        # search in the global symbol table and the builtins.
                        body.append(f'stack[++stack_current] = loc_{name} != null ? loc_{name} : {self.mangle_global(name)}')
                case "LOAD_CONST":
                    const = fn.co_consts[instr.arg]
                    body.append(f"stack[++stack_current] = &const_{instr.arg};");
                case "CALL":
                    body.append(f"PY_OPCODE_CALL({instr.arg});")
                case "POP_TOP":
                    body.append(f"stack_current--;")
                case "RETURN_CONST":
                    body.append(f"return WITH_RESULT(&const_{instr.arg});")
                case "STORE_NAME":
                    name = fn.co_names[instr.arg]

                    if is_entrypoint:
                        body.append(f'{self.mangle_global(name)} = stack[stack_current--];')
                    else:
                        body.append(f'loc_{fn.co_names[instr.arg]} = stack[stack_current--];')
                case "COMPARE_OP":
                    # left operand is first popped value, right operand is second popped value
                    operation = dis.cmp_op[instr.arg >> 5]
                    coerce_bool = (instr.arg & 16) != 0 # fifth lowest bit

                    op = {
                        "<": "lt",
                        "<=": "lte",
                        "==": "equ",
                        "!=": "neq",
                        ">": "gt",
                        ">=": "gte"
                    }[operation]

                    body.append(f"PY_OPCODE_COMPARISON({op}, {c_bool(coerce_bool)});")
                case "POP_JUMP_IF_FALSE":
                    target_label = label_by_offset(instr.jump_target)
                    body.append(f"PY_OPCODE_POP_JUMP_IF_FALSE({target_label});")
                case "BINARY_OP":
                    op = {
                        NB_ADD: "add",
                        NB_AND: "and",
                        NB_FLOOR_DIVIDE: "floordiv",
                        NB_LSHIFT: "lsh",
                        NB_MATRIX_MULTIPLY: "matmul",
                        NB_MULTIPLY: "mul",
                        NB_REMAINDER: "rem",
                        NB_OR: "or",
                        NB_POWER: "pow",
                        NB_RSHIFT: "rsh",
                        NB_SUBTRACT: "sub",
                        NB_TRUE_DIVIDE: "floordiv", # TODO!
                        NB_XOR: "xor",
                        NB_INPLACE_ADD: "iadd",
                        NB_INPLACE_AND: "iand",
                        NB_INPLACE_FLOOR_DIVIDE: "ifloordiv",
                        NB_INPLACE_LSHIFT: "ilsh",
                        NB_INPLACE_MATRIX_MULTIPLY: "imatmul",
                        NB_INPLACE_MULTIPLY: "imul",
                        NB_INPLACE_REMAINDER: "irem",
                        NB_INPLACE_OR: "ior",
                        NB_INPLACE_POWER: "ipow",
                        NB_INPLACE_RSHIFT: "irsh",
                        NB_INPLACE_SUBTRACT: "isub",
                        NB_INPLACE_TRUE_DIVIDE: "ifloordiv", # TODO!!
                        NB_INPLACE_XOR: "ixor",
                        NB_SUBSCR: "subscr"
                    }[instr.arg]

                    body.append(f"PY_OPCODE_OPERATION({op});")
                case "JUMP_BACKWARD" | "JUMP_BACKWARD_NO_INTERRUPT":
                    body.append(f"goto {label_by_offset(instr.jump_target)};")
                case _:
                    error(f"unknown opcode '{instr.opname}'!")
                    error(f"the full disassembly of the target function is displayed below")
                    print(dis.dis(fn))
                    raise Exception(f"Unknown opcode: {instr.opname}")
                
            body.append("")

        body.append("// (function end)")
        body.append("")

        # This is the default handler for exceptions if the exception table didn't define
        # one already. We simply pass the exception to the caller.
        body.append("L_uncaught_exception:")
        body.append("return WITH_EXCEPTION(caught_exception);")
        body.append("")

        for sym in defined_preprocessor_syms:
            body.append(f"#undef {sym}")

        self.transpiled[mangled_name] = "\n".join(body)

        if is_entrypoint:
            self.entrypoint = mangled_name

        return mangled_name

    def transpile(self):
        "Merges all compiled functions into one C file."
        lines: list[str] = []

        lines.append("// <auto-generated>")
        lines.append("// This code was transpiled from Python code by Pyton.")
        lines.append("// </auto-generated>")
        lines.append("")
        lines.append('#include <pyton_runtime.h>')
        lines.append("")
        lines.append('#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"')
        lines.append("")

        lines.append("// Transpiled function declarations")
        for fn_name in self.transpiled.keys():
            lines.append(f"PY_DEFINE({fn_name});")

        lines.append("")
        lines.append("// Known global names")
        for name in self.known_names:
            lines.append(f"#ifndef PY_GLOBAL_{sanitize_identifier(name)}_WELLKNOWN")
            lines.append(f"    pyobj_t* {self.mangle_global(name)} = NULL; // global '{name}'")
            lines.append(f"#endif")
            lines.append("")

        if self.entrypoint is not None:
            lines.append(f"DEFINE_ENTRYPOINT({self.entrypoint});")
            lines.append("")

        lines.append("// Transpiled function implementations")
        for fn_name, fn_body in self.transpiled.items():
            lines.append("PY_DEFINE(" + fn_name + ") {")
            lines.append(textwrap.indent(fn_body, "    "))
            lines.append("}")
            lines.append("")

        return "\n".join(lines)
    