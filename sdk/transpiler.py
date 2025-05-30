import re
import dis
import textwrap
from types import CodeType

from .util import error

def c_bool(x: bool):
    return "true" if x else "false"

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

    def mangle(self, fn: CodeType):
        return "pyfn__" + re.sub(r"[^_A-Za-z0-9]", "__", fn.co_qualname)

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

        defined_preprocessor_syms = []

        body = [
            f"// Function {fn.co_qualname}, declared on line {fn.co_firstlineno}",
            f"void* stack[{fn.co_stacksize + 1}];",
            f"int stack_current = -1;",
            ""
        ]

        body.append("")
        body.append("// (constants start)")
        for i, const in enumerate(fn.co_consts):
            if type(const) is str:
                body.append(f'static const pyobj_t const_{i} = {{ .type = &py_type_str, .as_str = "{const}" }};')
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

        body.append("// (function body start)")

        bytecode = dis.Bytecode(fn)
        for instr in bytecode:
            body.append(f"// {str(instr).strip()}")

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
                        body.append(f'stack[++stack_current] = py_resolve_symbol("{name}");')
                    else:
                        # If loc_{name} is NULL, that means that the local of that name isn't defined, so we
                        # search in the global symbol table and the builtins.
                        body.append(f'stack[++stack_current] = loc_{name} != null ? loc_{name} : py_resolve_symbol("{name}");')
                case "LOAD_CONST":
                    const = fn.co_consts[instr.arg]
                    body.append(f"stack[++stack_current] = &const_{instr.arg};");
                case "CALL":
                    body.append(f"PY_OPCODE_CALL({instr.arg});")
                case "POP_TOP":
                    body.append(f"stack_current--;")
                case "RETURN_CONST":
                    body.append(f"return &const_{instr.arg};")
                case "STORE_NAME":
                    if is_entrypoint:
                        body.append(f'py_assign_global("{fn.co_names[instr.arg]}", stack[stack_current--]);')
                    else:
                        body.append(f'loc_{fn.co_names[instr.arg]} = stack[stack_current--];')
                case "COMPARE_OP":
                    # left operand is first popped value, right operand is second popped value
                    operation = dis.cmp_op[instr.arg >> 5]
                    coerce_bool = (instr.arg & 16) != 0 # fifth lowest bit

                    opcode_fn_name = {
                        "<": "py_opcode_compare_lt",
                        "<=": "py_opcode_compare_lte",
                        "==": "py_opcode_compare_equ",
                        "!=": "py_opcode_compare_neq",
                        ">": "py_opcode_compare_gt",
                        ">=": "py_opcode_compare_gte"
                    }[operation]

                    body.append(f"{opcode_fn_name}(&stack, &stack_current, {c_bool(coerce_bool)});")
                case _:
                    error(f"unknown opcode '{instr.opname}'!")
                    error(f"the full disassembly of the target function is displayed below")
                    print(dis.dis(fn))
                    raise Exception(f"Unknown opcode: {instr.opname}")
                
            body.append("")

        body.append("// (function end)")
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

        lines.append("// Transpiled function declarations")
        for fn_name in self.transpiled.keys():
            lines.append(f"PY_DEFINE({fn_name});")

        if self.entrypoint is not None:
            lines.append("")
            lines.append(f"DEFINE_ENTRYPOINT({self.entrypoint});")
            lines.append("")

        lines.append("// Transpiled function implementations")
        for fn_name, fn_body in self.transpiled.items():
            lines.append("PY_DEFINE(" + fn_name + ") {")
            lines.append(textwrap.indent(fn_body, "    "))
            lines.append("}")
            lines.append("")

        return "\n".join(lines)
    