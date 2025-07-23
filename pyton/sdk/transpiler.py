import os
import re
import dis
import inspect
import textwrap
from types import CodeType
from typing import Protocol, Any
from dataclasses import dataclass

from .bytecode import *
from .importing import FullImport, SelectiveImport, get_all_imports, resolve_import
from .util import error, find, flatten
from .simplification import simplify_bytecode

def c_bool(x: bool):
    return "true" if x else "false"

def sanitize_identifier(x: str):
    return re.sub(r"[^_A-Za-z0-9]", "__", x)

def wellknown_global_macro(name: str):
    return f"PY_GLOBAL_{sanitize_identifier(name)}_WELLKNOWN"

class ExceptionTableEntry(Protocol):
    start: int
    end: int
    target: int
    depth: int
    lasti: bool

@dataclass
class TranspiledFunction:
    body: str
    origin: CodeType

class Module:
    "Represents data exclusive to a single module."

    def __init__(self, name: str):
        self.name = name
        "The name of the module, as defined by the import name (e.g. for `import re`, this will be `'re'`)."

        self.known_names: set[str] = set()
        "Stores all known names that may be either globals or locals."

        self.transpiled: dict[str, TranspiledFunction] = {}
        "Stores mappings between mangled function names and their C function bodies."

class TranslationUnit:
    """
    Represents a single translation unit, which contains C function bodies that
    represent Python code fragments (e.g. module-level code, functions, methods, etc.)
    """

    def __init__(self):
        self.known_consts: dict[Any, str] = {}
        "Maps constants to their global C symbol names."

        self.next_const_id = 1
        "The ID to assign to the next known constant."

        self.const_definitions: list[str] = []
        "C definitions of known constants."

        self.modules: dict[str, Module] = {}
        "All defined modules."

    def all_transpiled(self):
        "Returns a dictionary of all transpiled functions across all modules."
        return { k: v for d in (x.transpiled for x in self.modules.values()) for k, v in d.items() }

    def mangle(self, fn: CodeType, module: str):
        return f"pyfn__{module}_{sanitize_identifier(fn.co_qualname)}" 
    
    def mangle_global(self, name: str, module: str):
        if module == "__main__":
            # Globals under the __main__ module are considered "canonical".
            # This doesn't really match Python's behavior 1:1 (e.g. if we overwrite `print`
            # in the main module, imports will also get the overwrite), but it should be
            # fine for now...
            return "pyglobal__" + sanitize_identifier(name)
        
        return f"pyglobal__{module}_{sanitize_identifier(name)}"

    def get_or_create_const(
        self,
        const,
        source_bytecode: dis.Bytecode,
        source_fn: CodeType,
        source_path: str,
        source_module: str
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
                items.append(self.get_or_create_const(item, source_bytecode, source_fn, source_path, source_module))
            
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

            target_fn = self.translate(code, source_path, source_module, code_is_class_body)
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
        module: str,
        is_class_body = False
    ):
        """
        Transpiles the given code fragment into a C function, returning its mangled name.
        The function itself can be retrieved via `self.transpiled`. If the function was already
        transpiled before, this function does nothing.

        `is_class_body` should be set to `True` if the fragment corresponds to a class body,
        and will be passed into `builtins.__build_class__`.
        """
        mangled_name = self.mangle(fn, module)
        if mangled_name in self.all_transpiled():
            return mangled_name
    
        is_module = fn.co_name == "<module>"
        if is_module:
            assert not is_class_body
            self.modules[module] = Module(module)

        defined_preprocessor_syms = ["PY__EXCEPTION_HANDLER_LABEL"]

        body = [
            f"// Function {fn.co_qualname} of module {module}, declared on line {fn.co_firstlineno}, class body: {'yes' if is_class_body else 'no'}",
            f"void* stack[{fn.co_stacksize + 1}] = {{}};",
            f"int stack_current = -1;",
            f"pyobj_t* caught_exception = NULL;",
            f"#define PY__EXCEPTION_HANDLER_LABEL L_uncaught_exception"
        ]

        if is_module:
            body.append("")
            body.append(f"MODULE_PROLOGUE({module});")

        body.append("")
        body.append("// (constants start)")

        bytecode = dis.Bytecode(fn)
        exc_table: list[ExceptionTableEntry] = bytecode.exception_entries

        imports = get_all_imports(bytecode, fn)
        for imprt in imports:
            path = resolve_import(source_path, imprt.name)

            # This function is the <module> function of the imported module.
            # It will execute all code defined in the module, and ultimately, set up
            # its globals, which we can import from.
            module_body = self.translate(
                fn = compile(open(path, "r").read(), os.path.basename(path), "exec"),
                source_path = path,
                module = imprt.name
            )

            if type(imprt) is FullImport:
                # Full imports use LOAD_ATTR, which means that the imported module becomes
                # an object. For example:
                #       LOAD_NAME                0 (abc)
                #       LOAD_ATTR                2 (aaa)
                # In order to minimize the runtime overhead, we special-case this. In a module,
                # when we do:
                #       IMPORT_NAME              0 (abc)
                #       STORE_NAME               0 (abc)
                # We associate 'abc' with a special "module token" in the module that imported
                # 'abc'. Then, if we were to load 'xyz' from 'abc', we'd do the following:
                #       (
                #           pyglobal__abc.type == &py_type_moduletoken &&
                #           pyglobal__abc.as_moduletoken.id == <ID>
                #       ) ? pyglobal__abc_xyz : py_get_attribute(pyglobal__abc, "xyz")
                # This avoid an attribute lookup.
                error("full imports (e.g. import module) are not yet supported!")
                error("try a selective import (e.g. from module import abc) instead.")
                raise Exception("Full imports are not yet supported.")
            elif type(imprt) is SelectiveImport:
                # Selective imports are simpler. The selected globals of the importee are
                # assigned to the globals of the importer.
                body.append(f"// from {imprt.name} import {', '.join(f'({x} as {y})' for x, y in imprt.targets)}")
                body.append(f"{module_body}(NULL, 0, NULL, 0, NULL);")
                
                for from_target, to_target in imprt.targets:
                    body.append(f"{self.mangle_global(to_target, module)} = {self.mangle_global(from_target, imprt.name)};")

        ignore_ranges = [(x.start, x.end) for x in imports] + simplify_bytecode(bytecode)

        for i, const in enumerate(fn.co_consts):
            const_sym = self.get_or_create_const(const, bytecode, fn, source_path, module)
            body.append(f"#define const_{i} ({const_sym})")
            defined_preprocessor_syms.append(f"const_{i}")
            
        body.append("// (constants end)")
        body.append("")

        # Locals behave differently in both the module and class body scope.
        # In modules, locals are equivalent to globals.
        # In class bodies, locals are equivalent to `self`.
        if not is_module and not is_class_body:
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
            self.modules[module].known_names.add(name)

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

        for instr_idx, instr in enumerate(bytecode):
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

            exc_info = find(
                exc_table,
                lambda x: x.start <= instr.offset and x.end >= instr.offset
            )

            if exc_info is None and prev_handler_region is not None:
                body.append(f"// No exception handler for this region")
                body.append(f"#undef PY__EXCEPTION_HANDLER_LABEL")
                body.append(f"#define PY__EXCEPTION_HANDLER_LABEL L_uncaught_exception")
                prev_handler_region = None

            exc_depth = exc_info.depth if exc_info is not None else 0
            exc_lasti = instr.offset if exc_info is not None else -1

            if any(instr_idx >= r[0] and instr_idx <= r[1] for r in ignore_ranges):
                continue

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

                    if is_module:
                        # Locals are equivalent to globals in the entrypoint.
                        body.append(f'{STACK_PUSH} = NOT_NULL({self.mangle_global(name, module)});')
                    elif is_class_body:
                        # Locals are equivalent to `self` attributes in class bodies.
                        body.append(f"PY_OPCODE_LOAD_NAME_CLASS({name});")
                    else:
                        # If loc_{name} is NULL, that means that the local of that name isn't defined, so we
                        # search in the global symbol table and the builtins.
                        body.append(f'{STACK_PUSH} = loc_{name} != null ? loc_{name} : NOT_NULL({self.mangle_global(name, module)});')
                case "LOAD_CONST":
                    assert instr.arg is not None
                    const = fn.co_consts[instr.arg]
                    body.append(f"{STACK_PUSH} = &const_{instr.arg};")
                case "LOAD_GLOBAL":
                    assert instr.arg is not None
                    name = fn.co_names[instr.arg >> 1]

                    body.append(f'{STACK_PUSH} = {self.mangle_global(name, module)};')
                
                    # Changed in version 3.11: If the low bit of namei is set, then a
                    # NULL is pushed to the stack before the global variable.
                    #
                    # ^ the official docs say 'before', but it does seem like we need
                    # to push the NULL *after* the global.
                    if (instr.arg & 1) == 1:
                        body.append(f"stack[++stack_current] = NULL;")

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

                    if is_module:
                        body.append(f'{self.mangle_global(name, module)} = {STACK_POP};')
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
                    body.append(f"{STACK_PUSH} = {self.mangle_global("__build_class__", "__main__")};")
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

        self.modules[module].transpiled[mangled_name] = TranspiledFunction("\n".join(body), fn)
        return mangled_name

    def transpile(self, entrypoint: str | None = None):
        """
        Merges all compiled functions into one C file. `entrypoint` defines the mangled
        symbol name of the function to invoke as an entry-point - this is usually the
        `<module>` function of the `__main__` module.
        """
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
        for fn_name in self.all_transpiled().keys():
            lines.append(f"PY_DEFINE({fn_name});")

        lines.append("")
        lines.append("// Module-specific definitions/declarations")
        for module in self.modules.values():
            lines.append(f"// State for module {module.name}")
            lines.append(f"bool MODULE_INIT_STATE({module.name}) = false;")
            lines.append("")

            lines.append(f"// Known global names for {module.name}")
            for name in module.known_names:
                if module.name == "__main__":
                    lines.append(f"#ifndef {wellknown_global_macro(name)}")

                lines.append(f"pyobj_t* {self.mangle_global(name, module.name)} = NULL; // global '{name}'")
                
                if module.name == "__main__":
                    lines.append("#endif")
                    lines.append("")

            lines.append("")

        lines.append("// Known constants")
        lines.extend(self.const_definitions)
        lines.append("")

        if entrypoint is not None:
            lines.append(f"DEFINE_ENTRYPOINT({entrypoint});")
            lines.append("")

        lines.append("// Transpiled function implementations")
        for module in self.modules.values():
            for fn_name, fn_transpiled in module.transpiled.items():
                fn_body = fn_transpiled.body

                lines.append("PY_DEFINE(" + fn_name + ") {")

                if module.name != "__main__" and fn_transpiled.origin.co_name == "<module>":
                    # If this is not the main module, then we need to copy all well-known
                    # globals from the main module scope to our own scope - so that when
                    # we resolve "print" to "pyglobal__mymodule_print", we get "pyglobal__print"
                    # if it hasn't been overwritten yet.
                    #
                    # We only do this from the module entry-point functions.
                    for name in module.known_names:
                        lines.append(f"#ifdef {wellknown_global_macro(name)}")
                        lines.append(f"    {self.mangle_global(name, module.name)} = {self.mangle_global(name, "__main__")};")
                        lines.append( "#endif")

                lines.append(textwrap.indent(fn_body, "    "))
                lines.append("}")
                lines.append("")

        return "\n".join(lines)
    