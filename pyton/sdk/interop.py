import dis
from types import CodeType
from typing import Literal
from textwrap import dedent
from dataclasses import dataclass

from .util import omit, unwrap

InteropType = Literal[
    "INT",      # int <-> int64_t
    "STRING",   # str <-> string_t
    "FLOAT",    # float <-> double
    "BOOL",     # bool <-> int (True = 1, False = 0)
    "NONE",     # NoneType <-> void* (None = NULL)
    "OBJ"       # Any <-> pyobj_t
]

@dataclass
class ExternSpec:
    """
    Represents the specification for an `@extern` function, holding the details
    about the target symbol and their parameter types.
    """

    symbol: str
    "The name of the symbol to import."

    param_types: dict[str, InteropType]
    "A map of parameter names to their types."

    return_type: InteropType
    "The return type of the function."

    def c_name(self):
        "Returns the `PY_DEFINE` name of the function."
        return f"_extern_{self.symbol}"

@dataclass
class ExternFunction:
    "Represents a specific occurence of an `@extern` function."

    start: int
    "The start of the bytecode range of the region that assigns the function `symbol`."

    end: int
    "The end of the bytecode range of the region that assigns the function `symbol`."

    spec: ExternSpec
    "The information about the target symbol and how data should be marshalled."

def _get_interop_type(value: str | None) -> InteropType:
    if value is None:
        return "NONE"

    match value:
        case "str": return "STRING"
        case "int": return "INT"
        case "float": return "FLOAT"
        case "bool": return "BOOL"

    return "OBJ"

def _interop_to_c_type(target: InteropType):
    match target:
        case "BOOL": return "bool"
        case "FLOAT": return "double"
        case "INT": return "int64_t"
        case "NONE": return "void"
        case "OBJ": return "pyobj_t*"
        case "STRING": return "string_t"

def get_all_externs(bytecode: dis.Bytecode) -> list[ExternFunction]:
    "Finds all functions decorated with `@extern` in the bytecode of a function."

    fn = bytecode.codeobj
    body = [*bytecode]

    externs: list[ExternFunction] = []

    for i in range(len(body)):
        result = _scan_single(body, fn, i)
        if result is None:
            continue

        externs.append(result)

    return externs
        
def _scan_single(body: list[dis.Instruction], fn: CodeType, i: int) -> ExternFunction | None:
    start_idx = i

    if body[i].opname != "LOAD_NAME" or fn.co_names[unwrap(body[i].arg)] != "extern":
        return None

    # For a definition like this:
    #       @extern
    #       def func(p1: int, p2: str) -> None: ...
    # ...we expect the following bytecode:
    #       LOAD_NAME                0 (extern)
    #       LOAD_CONST               0 ('p1')
    #       LOAD_NAME                1 (int)
    #       LOAD_CONST               1 ('p2')
    #       LOAD_NAME                2 (str)
    #       LOAD_CONST               2 ('return')
    #       LOAD_CONST               3 (None)
    #       BUILD_TUPLE              6
    #       LOAD_CONST               4 (<code object func>)
    #       MAKE_FUNCTION
    #       SET_FUNCTION_ATTRIBUTE   4 (annotations)
    #       CALL                     0
    #       STORE_NAME               3 (func)

    annotations: dict[str, InteropType] = {}

    i += 1
    while body[i].opname == "LOAD_CONST" and body[i + 1].opname in ("LOAD_NAME", "LOAD_CONST"):
        pname_instr = body[i]
        ptype_instr = body[i + 1]

        assert pname_instr.arg is not None
        assert ptype_instr.arg is not None

        pname = fn.co_consts[pname_instr.arg]
        ptype = _get_interop_type(
            (fn.co_consts if ptype_instr.opname == "LOAD_CONST" else fn.co_names)[ptype_instr.arg]
        )

        annotations[pname] = ptype

        i += 1 # skip the corresponding type load instruction

    if (
        body[i + 0].opname != "BUILD_TUPLE" or
        body[i + 1].opname != "LOAD_CONST" or
        body[i + 2].opname != "MAKE_FUNCTION" or
        body[i + 3].opname != "SET_FUNCTION_ATTRIBUTE" or
        body[i + 4].opname != "CALL" or
        body[i + 5].opname != "STORE_NAME"
    ):
        return None
    
    end_idx = i + 5 # points to STORE_NAME
    
    sym_name = fn.co_names[unwrap(body[i + 5].arg)]

    return ExternFunction(
        start_idx,
        end_idx,
        ExternSpec(
            sym_name,
            omit(annotations, "return"),
            annotations.get("return", "NONE")
        )
    )

def create_marshalling_stub(extern: ExternSpec):
    "Returns the definition of a marshalling stub representing the given extern function."

    ret = _interop_to_c_type(extern.return_type)
    decl_params = ", ".join(f"{_interop_to_c_type(x[1])} {x[0]}" for x in extern.param_types.items())
    call_params = ", ".join(f"arg_{x}" for x in extern.param_types.keys())

    body: list[str] = [
        f"ASSERT(argc == {len(extern.param_types)});",
        "ENSURE_NOT_NULL(argv);"
    ]

    for i, (pname, ptype) in enumerate(extern.param_types.items()):
        body.append(f"{_interop_to_c_type(ptype)} arg_{pname} = argv[{i}];")

    body.append(f"{_interop_to_c_type(extern.return_type)} ret = {extern.symbol}({call_params});")

    match extern.return_type:
        case "INT":     body.append("return MARSHALLED_INT(ret);")
        case "STRING":  body.append("return MARSHALLED_STR(ret);")
        case "BOOL":    body.append("return MARSHALLED_BOOL(ret);")
        case "FLOAT":   body.append("return MARSHALLED_FLOAT(ret);")
        case "NONE":    body.append("return WITH_RESULT(&py_none);")
        case "OBJ":     raise Exception("Cannot marshal a return type to pyobj_t*.")
        
    return [
        f"extern {ret} {extern.symbol}({decl_params});",
        "",
        f"PY_DEFINE({extern.c_name()}) {{",
        *(f"    {x}" for x in body),
        "}"
    ]
