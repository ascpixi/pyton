import re
import dis
import inspect
import textwrap
from types import CodeType
from typing import Protocol, Any

from .bytecode import *
from .util import error, flatten

def c_bool(x: bool):
    return "true" if x else "false"

def sanitize_identifier(x: str):
    return re.sub(r"[^_A-Za-z0-9]", "__", x)

class ExceptionTableEntry(Protocol):
    start: int
    end: int
    target: int
    depth: int
    lasti: bool

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

        self.known_consts: dict[Any, str] = {}
        "Maps constants to their global C symbol names."

        self.next_const_id = 1
        "The ID to assign to the next known constant."

        self.const_definitions: list[str] = []
        "C definitions of known constants."

    def mangle(self, fn: CodeType):
        return "pyfn__" + sanitize_identifier(fn.co_qualname)
    
    def mangle_global(self, name: str):
        return "pyglobal__" + sanitize_identifier(name)

    def get_or_create_const(
        self,
        const,
        source_bytecode: dis.Bytecode,
        source_fn: CodeType,
        source_path: str
    ):
        """
        Gets or creates the known constant object for `const`, and returns its C symbol
        name. Recognized types for `const` are the following:
        - `bool`,
        - `NoneType`,
        - `str`,
        - `int`,
        - `float`,
        - `tuple`,
        - `code`.
        """
        if const in self.known_consts:
            return self.known_consts[const]
        
        if type(const) is bool:
            return "py_true" if const else "py_false";
        elif const is None:
            return "py_none"

        name = f"py_const_{self.next_const_id}"
        self.next_const_id += 1
        self.known_consts[const] = name

        if type(const) is str:
            self.const_definitions.append(f'static pyobj_t {name} = {{ .type = &py_type_str, .as_str = STR("{const.replace("\n", "\\n").replace("\r", "")}") }};')
        elif type(const) is int:
            self.const_definitions.append(f"static pyobj_t {name} = {{ .type = &py_type_int, .as_int = {const} }};")
        elif type(const) is float:
            self.const_definitions.append(f"static pyobj_t {name} = {{ .type = &py_type_float, .as_float = {const} }};")
        elif type(const) is tuple:
            items: list[str] = []
            for item in const:
                items.append(self.get_or_create_const(item, source_bytecode, source_fn, source_path))
            
            self.const_definitions.append(
                f"static pyobj_t* {name}_elements[] = " + "{ " +
                ", ".join(f"&{item_name}" for item_name in items) +
                " };"
            )

            self.const_definitions.append("static pyobj_t " + name + " = {")
            self.const_definitions.append("    .type = &py_type_tuple,")
            self.const_definitions.append("    .as_list = {")
            self.const_definitions.append(f"        .elements = {name}_elements,")
            self.const_definitions.append(f"        .length = {len(const)},")
            self.const_definitions.append(f"        .capacity = {len(const)}")
            self.const_definitions.append("    }")
            self.const_definitions.append("};")
        elif type(const).__name__ == "code":
            # If we have a 'code' constant, this means that this is a callable.
            code: CodeType = const

            # We first need to check if this constant is loaded as the first code
            # constant before a __build_class__ invocation. If it is, then we
            # classify it as a class body.
            searching_for_build_class = True
            code_is_class_body = False

            for instr in source_bytecode:
                if searching_for_build_class:
                    if instr.opname == "LOAD_BUILD_CLASS":
                        searching_for_build_class = False

                    continue

                # We've encountered a LOAD_BUILD_CLASS instruction, now we
                # check if any LOAD_CONST's appear that reference this code constant
                if instr.opname != "LOAD_CONST":
                    continue

                if instr.arg is not None and source_fn.co_consts[instr.arg] == const:
                    code_is_class_body = True
                    break

            target_fn = self.translate(code, source_path, False, code_is_class_body)
            self.const_definitions.append(f"static pyobj_t {name} = {{ .type = &py_type_function, .as_function = &{target_fn} }};")
        else:
            error(f"unknown constant type '{type(const).__name__}'!")
            error(f"the value of the constant is {const}")
            error(f"the full disassembly of the target function is displayed below")
            print(dis.dis(source_fn))
            raise Exception(f"Unknown constant type: {type(const).__name__}")

        return name

    def translate(
        self,
        fn: CodeType,
        source_path: str,
        is_entrypoint = False,
        is_class_body = False
    ):
        """
        Transpiles the given code fragment into a C function, returning its mangled name.
        The function itself can be retrieved via `self.transpiled`. If the function was already
        transpiled before, this function does nothing.

        `is_entrypoint` should be set to `True` if the fragment is supposed to be ran after
        system startup.
        
        `is_class_body` should be set to `True` if the fragment corresponds to a class body,
        and will be passed into `builtins.__build_class__`.
        """
        mangled_name = self.mangle(fn)
        if mangled_name in self.transpiled:
            return mangled_name

        defined_preprocessor_syms = ["PY__EXCEPTION_HANDLER_LABEL"]

        body = [
            f"// Function {fn.co_qualname}, declared on line {fn.co_firstlineno}, class body: {'yes' if is_class_body else 'no'}",
            f"void* stack[{fn.co_stacksize + 1}] = {{}};",
            f"int stack_current = -1;",
            f"pyobj_t* caught_exception = NULL;",
            f"#define PY__EXCEPTION_HANDLER_LABEL L_uncaught_exception"
        ]

        body.append("")
        body.append("// (constants start)")

        bytecode = dis.Bytecode(fn)
        exc_table: list[ExceptionTableEntry] = bytecode.exception_entries

        # imports = get_all_imports(bytecode, fn)
        # for imprt in imports:
        #     path = resolve_import(source_path, imprt.name)
        #     module_fn = compile(open(path, "r").read(), os.path.basename(path), "exec")

        #     if type(imprt) == FullImport:
        #         self.translate()

        for i, const in enumerate(fn.co_consts):
            const_sym = self.get_or_create_const(const, bytecode, fn, source_path)
            body.append(f"#define const_{i} ({const_sym})")
            defined_preprocessor_syms.append(f"const_{i}")
            
        body.append("// (constants end)")
        body.append("")

        # Locals behave differently in both the entry-point and class body scope.
        # In the entry-point, locals are equivalent to globals.
        # In class bodies, locals are equivalent to `self`.
        if not is_entrypoint and not is_class_body:
            body.append("int argc_all = argc + (( self != NULL ? 1 : 0 ));")

            for name in fn.co_varnames:
                body.append(f"pyobj_t* loc_{name} = NULL;")

            # Arguments also boil down to variables - their names are in the following order
            # in the co_varnames list:
            #   - positional-or-keyword arguments,
            #   - positional arguments,
            #   - keyword arguments (after `*` or `*<name>`),
            #   - varargs tuple name e.g. `*args` (if inspect.CO_VARARGS is set),
            #   - varkeywords tuple name e.g. `**kwargs` (if inspect.CO_VARKEYWORDS is set).
            
            if (fn.co_flags & inspect.CO_VARARGS) == 0:
                # We don't have a varargs tuple, so we may encounter a situation where
                # we have too many positional arguments.
                body.append(f"PY_POS_ARG_MAX({fn.co_argcount});")
            
            if fn.co_argcount != 0:
                # TODO: This will need to change when we add support for default arguments, which
                #       Python implements in an extremely silly way, where they are not even part
                #       of the code object itself.
                body.append(f"PY_POS_ARG_MIN({fn.co_argcount});")

            # Copy positional-or-keyword + positional arguments
            body.append(
                "pyobj_t** pos_args[] = { " + ", ".join(
                    f"&loc_{fn.co_varnames[i]}" for i in range(fn.co_argcount)
                ) + " };"
            )

            # This will also account for 'self'.
            body.append(f"PY_POS_ARGS_TO_VARS({fn.co_argcount});")

        if is_class_body:
            # For class bodies, self must ALWAYS be provided. This is special-cased
            # in the runtime.
            body.append(f'ENSURE_NOT_NULL(self);')

        # All names might be global. For example:
        #   def ex():
        #       print(a)
        #       a = 2
        #       print(a)
        # First, "a" would be found in the global symbol table and printed out. Then, it
        # would get defined in the local symbol table, and we'd print the local instead.
        for name in fn.co_names:
            self.known_names.add(name)

        body.append("")
        body.append("// (function body start)")
        
        # This maps label indices (instr.label) to offsets.
        labels = sorted([
            *dis.findlabels(fn.co_code), # type: ignore
            *flatten([[x.start, x.end, x.target] for x in exc_table])
        ])

        def label_by_offset(offset: int):
            "Gets the label at the given bytecode offset."
            return next((f"L{i + 1}" for i, x in enumerate(labels) if x == offset), None)

        prev_handler_region: str | None = None

        for instr in bytecode:
            body.append(f"// {instr.offset}: {str(instr).strip()}")

            label = label_by_offset(instr.offset)
            if label is not None:
                body.append(f"{label}:")
                
            for entry in exc_table:
                if not (entry.start <= instr.offset and entry.end >= instr.offset):
                    continue
                
                handler_label = label_by_offset(entry.target)

                if prev_handler_region != handler_label:
                    body.append(f"// Exception region: {entry.start} to {entry.end}, target {entry.target}, depth {entry.depth}, lasti: {'yes' if entry.lasti else 'no'}")
                    body.append(f"#undef PY__EXCEPTION_HANDLER_LABEL")
                    body.append(f"#define PY__EXCEPTION_HANDLER_LABEL {handler_label}")
                    prev_handler_region = handler_label
                    
                break

            exc_info = next(
                (x for x in exc_table if (x.start <= instr.offset and x.end >= instr.offset)),
                None
            )

            if exc_info is None and prev_handler_region is not None:
                body.append(f"// No exception handler for this region")
                body.append(f"#undef PY__EXCEPTION_HANDLER_LABEL")
                body.append(f"#define PY__EXCEPTION_HANDLER_LABEL L_uncaught_exception")
                prev_handler_region = None

            exc_depth = exc_info.depth if exc_info is not None else 0
            exc_lasti = instr.offset if exc_info is not None else -1

            # Important to mention: stack_current points to the stack slot that will be
            # popped next. When pushing, it needs to be incremented BEFORE writing to the
            # slot.
            STACK_PUSH = "stack[++stack_current]"
            STACK_POP = "stack[stack_current--]"

            match instr.opname:
                case "RESUME" | "NOP":
                    pass # no-op
                case "PUSH_NULL":
                    body.append(f"{STACK_PUSH} = NULL;")
                case "LOAD_NAME":
                    assert instr.arg is not None
                    name = fn.co_names[instr.arg]

                    if is_entrypoint:
                        # Locals are equivalent to globals in the entrypoint.
                        body.append(f'{STACK_PUSH} = {self.mangle_global(name)};')
                    elif is_class_body:
                        # Locals are equivalent to `self` attributes in class bodies.
                        body.append(f"PY_OPCODE_LOAD_NAME_CLASS({name});")
                    else:
                        # If loc_{name} is NULL, that means that the local of that name isn't defined, so we
                        # search in the global symbol table and the builtins.
                        body.append(f'{STACK_PUSH} = loc_{name} != null ? loc_{name} : {self.mangle_global(name)}')
                case "LOAD_CONST":
                    assert instr.arg is not None
                    const = fn.co_consts[instr.arg]
                    body.append(f"{STACK_PUSH} = &const_{instr.arg};")
                case "LOAD_GLOBAL":
                    assert instr.arg is not None
                    name = fn.co_names[instr.arg >> 1]

                    # Changed in version 3.11: If the low bit of namei is set, then a
                    # NULL is pushed to the stack before the global variable.
                    if (instr.arg & 1) == 1:
                        body.append(f"stack[++stack_current] = NULL;")

                    body.append(f'{STACK_PUSH} = {self.mangle_global(name)};')
                case "LOAD_FAST":
                    assert instr.arg is not None
                    
                    if not is_class_body:
                        body.append(f'{STACK_PUSH} = loc_{fn.co_varnames[instr.arg]};')
                    else:
                        body.append(f'{STACK_PUSH} = NOT_NULL(py_get_attribute(self, "{fn.co_varnames[instr.arg]}"));')
                case "LOAD_FAST_LOAD_FAST":
                    assert instr.arg is not None

                    if not is_class_body:
                        body.append(f'{STACK_PUSH} = loc_{fn.co_varnames[instr.arg >> 4]};')
                        body.append(f'{STACK_PUSH} = loc_{fn.co_varnames[instr.arg & 15]};')
                    else:
                        body.append(f'{STACK_PUSH} = NOT_NULL(py_get_attribute(self, "{fn.co_varnames[instr.arg >> 4]}"));')
                        body.append(f'{STACK_PUSH} = NOT_NULL(py_get_attribute(self, "{fn.co_varnames[instr.arg & 15]}"));')
                case "CALL":
                    body.append(f"PY_OPCODE_CALL({instr.arg}, {exc_depth}, {exc_lasti});")
                case "RETURN_VALUE":
                    # We don't do STACK_POP here, since it would be redundant to decrement
                    # the stack_current counter.
                    body.append(f"return WITH_RESULT(stack[stack_current]);")
                case "POP_TOP":
                    body.append(f"stack_current--;")
                case "RETURN_CONST":
                    body.append(f"return WITH_RESULT(&const_{instr.arg});")
                case "STORE_NAME":
                    assert instr.arg is not None
                    name = fn.co_names[instr.arg]

                    if is_entrypoint:
                        body.append(f'{self.mangle_global(name)} = {STACK_POP};')
                    elif is_class_body:
                        body.append(f'py_set_attribute(self, STR("{name}"), {STACK_POP});')
                    else:
                        body.append(f'loc_{fn.co_names[instr.arg]} = {STACK_POP};')
                case "STORE_FAST":
                    assert instr.arg is not None
                    name = fn.co_varnames[instr.arg]

                    if not is_class_body:
                        body.append(f"loc_{name} = (pyobj_t*)({STACK_POP});")
                    else:
                        body.append(f'py_set_attribute(self, STR("{name}"), {STACK_POP});')
                case "STORE_ATTR":
                    assert instr.arg is not None
                    name = fn.co_names[instr.arg]
                    body.append(f'PY_OPCODE_STORE_ATTR("{name}");')
                case "LOAD_ATTR":
                    assert instr.arg is not None
                    name = fn.co_names[instr.arg >> 1]
                    if (instr.arg & 1) == 0:
                        # The low bit of namei is not set
                        body.append(f'PY_OPCODE_LOAD_ATTR("{name}");')
                    else:
                        body.append(f'PY_OPCODE_LOAD_ATTR_CALLABLE("{name}");')
                case "COMPARE_OP":
                    assert instr.arg is not None

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

                    body.append(f"PY_OPCODE_COMPARISON({op}, {c_bool(coerce_bool)}, {exc_depth}, {exc_lasti});")
                case "POP_JUMP_IF_FALSE":
                    target_label = label_by_offset(instr.jump_target)
                    body.append(f"PY_OPCODE_POP_JUMP_IF_FALSE({target_label});")
                case "POP_JUMP_IF_TRUE":
                    target_label = label_by_offset(instr.jump_target)
                    body.append(f"PY_OPCODE_POP_JUMP_IF_TRUE({target_label});")
                case "BINARY_OP":
                    assert instr.arg is not None
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

                    body.append(f"PY_OPCODE_OPERATION({op}, {exc_depth}, {exc_lasti});")
                case "JUMP_BACKWARD" | "JUMP_BACKWARD_NO_INTERRUPT":
                    body.append(f"goto {label_by_offset(instr.jump_target)};")
                case "RAISE_VARARGS":
                    if instr.arg == 0:
                        # 0: `raise` (re-raise previous exception)
                        body.append(f"RAISE_CATCHABLE(caught_exception, {exc_depth}, {exc_lasti});")
                    elif instr.arg == 1:
                        # 1: `raise STACK[-1]` (raise exception instance or type at STACK[-1])
                        body.append(f"RAISE_CATCHABLE({STACK_POP}, {exc_depth}, {exc_lasti});")
                    else:
                        # TODO: arg == 2
                        raise Exception(f"RAISE_VARARGS argc = {instr.arg} not implemented")
                case "PUSH_EXC_INFO":
                    body.append("PY_OPCODE_PUSH_EXC_INFO();")
                case "MAKE_FUNCTION":
                    body.append("// (already a function)")
                case "SET_FUNCTION_ATTRIBUTE":
                    assert instr.arg is not None
                    if instr.arg == 0x01:
                        # a tuple of default values for positional-only and
                        # positional-or-keyword parameters in positional order
                        raise Exception("Default values for arguments are not yet supported.")
                    elif instr.arg == 0x02:
                        # a dictionary of keyword-only parameters’ default values
                        raise Exception("Default values for arguments are not yet supported.")
                    elif instr.arg == 0x04:
                        # a tuple of strings containing parameters’ annotations
                        body.append("PY_OPCODE_SET_FUNC_ATTR_ANNOTATIONS();")
                    elif instr.arg == 0x08:
                        # a tuple containing cells for free variables, making a closure
                        raise Exception("Closures are not yet supported.")
                    else:
                        raise Exception(f"Unknown SET_FUNCTION_ATTRIBUTE flag: 0x{instr.arg:X}")
                case "LOAD_BUILD_CLASS":
                    body.append(f"{STACK_PUSH} = {self.mangle_global("__build_class__")};")
                case "POP_EXCEPT":
                    # TODO: not sure what the difference between this and POP_TOP is?
                    body.append(f"stack_current--;")
                case "COPY":
                    body.append(f"PY_OPCODE_COPY({instr.arg});")
                case "SWAP":
                    body.append(f"PY_OPCODE_SWAP({instr.arg});")
                case "RERAISE":
                    body.append(f"RAISE_CATCHABLE({STACK_POP}, {exc_depth}, {exc_lasti});")
                
                    # if instr.oparg != 0:
                    #     # If oparg is non-zero, pops an additional value from the stack
                    #     # which is used to set f_lasti of the current frame.
                    #     body.append(f"stack_current--;")
                case "CHECK_EXC_MATCH":
                    body.append("PY_OPCODE_CHECK_EXC_MATCH();")
                case "GET_ITER":
                    body.append(f"PY_OPCODE_GET_ITER({exc_depth}, {exc_lasti});")
                case "FOR_ITER":
                    target_label = label_by_offset(instr.jump_target)
                    body.append(f"PY_OPCODE_FOR_ITER({target_label}, {exc_depth}, {exc_lasti});")
                case "END_FOR":
                    # Removes the top-of-stack item. Equivalent to POP_TOP.
                    body.append(f"stack_current--;")
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
        lines.append('#pragma GCC diagnostic ignored "-Wunused-label"')
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

        lines.append("// Known constants")
        lines.extend(self.const_definitions)
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
    