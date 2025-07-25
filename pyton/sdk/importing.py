import os
import dis
from types import CodeType
from dataclasses import dataclass

from .util import unwrap, error

@dataclass
class Import:
    name: str
    "The name to import. For example, `something` should correspond to `something.py`."

    start: int
    "The beginning of the bytecode range associated with the import, expressed in instruction indices."

    end: int
    "The end of the bytecode range associated with the import, expressed in instruction indices."

@dataclass
class FullImport(Import):
    alias: str
    "The alias to import the module under. For example, for `import something as abc`, this would be `'abc'`."

@dataclass
class SelectiveImport(Import):
    targets: list[tuple[str, str]]
    "A list of globals to import from the module, in the form of `(origin, name)`."

def get_all_imports(bytecode: dis.Bytecode):
    """
    Scans the bytecode for all recognized import sequences. If `IMPORT_NAME` is used,
    but with an unrecognized bytecode sequence, an exception is thrown.
    """

    fn = bytecode.codeobj

    # Imports can ONLY appear at the very top of the function, without any conditionals
    # allowed. This limitation is exclusive to Pyton, mainly because we're AOT-compiling
    # Python.
    body = [x for x in bytecode]
    import_indices = [
        i for i, instr in enumerate(body)
        if instr.opname == "IMPORT_NAME"
    ]

    def as_full_import(
        i: int,
        fromlist: tuple[str] | None,
        import_name: str
    ) -> FullImport | None:
        # import something
        # -2    LOAD_CONST 0                    (level)
        # -1    LOAD_CONST None                 (fromlist)
        #  *    IMPORT_NAME something
        # +1    STORE_NAME something
    
        # import something as customname
        # -2    LOAD_CONST 0                    (level)
        # -1    LOAD_CONST None                 (fromlist)
        #  *    IMPORT_NAME something
        #  +1    STORE_NAME customname

        if fromlist is not None or body[i + 1].opname != "STORE_NAME":
            return None

        return FullImport(
            name = import_name,
            start = i - 2,
            end = i + 1,
            alias = fn.co_names[unwrap(body[i + 1].arg)]
        )
    
    def as_selective_import(
        i: int,
        fromlist: tuple[str] | None,
        import_name: str
    ) -> SelectiveImport | None:
        # from something import abc
        # -2    LOAD_CONST 0                    (level)
        # -1    LOAD_CONST ('abc',)             (fromlist)
        #  *    IMPORT_NAME something
        # +1    IMPORT_FROM abc
        # +2    STORE_NAME abc
        # +3    POP_TOP
        #
        # from something import abc, cba as aaa
        # -2    LOAD_CONST 0                    (level)
        # -1    LOAD_CONST ('abc', 'cba')       (fromlist)
        #  *    IMPORT_NAME something
        # +1    IMPORT_FROM abc
        # +2    STORE_NAME abc
        # +3    IMPORT_FROM cba
        # +4    STORE_NAME aaa
        # +5    POP_TOP

        if fromlist is None:
            return None
        
        start = i - 2
        targets: list[tuple[str, str]] = []

        while True:
            if body[i + 1].opname == "POP_TOP":
                break

            if body[i + 1].opname != "IMPORT_FROM" or body[i + 2].opname != "STORE_NAME":
                return None
            
            origin = fn.co_names[unwrap(body[i + 1].arg)] # IMPORT_FROM
            alias = fn.co_names[unwrap(body[i + 2].arg)] # STORE_NAME

            targets.append((origin, alias))
            i += 2

        return SelectiveImport(
            name = import_name,
            start = start,
            end = i + 1, # points to POP_TOP
            targets = targets
        )
    
    imports: list[SelectiveImport | FullImport] = []

    for i in import_indices:
        assert body[i - 2].opname == "LOAD_CONST"
        assert body[i - 1].opname == "LOAD_CONST"
        
        level = fn.co_consts[unwrap(body[i - 2].arg)]
        assert type(level) is int

        if level != 0:
            raise Exception("Relative imports are not yet supported.")
        
        fromlist = fn.co_consts[unwrap(body[i - 1].arg)]
        assert type(fromlist) is tuple or fromlist is None
        
        import_name = fn.co_names[unwrap(body[i].arg)]

        full = as_full_import(i, fromlist, import_name)
        if full is not None:
            imports.append(full)
            continue

        selective = as_selective_import(i, fromlist, import_name)
        if selective is not None:
            imports.append(selective)
            continue

        error("encountered an unrecognized bytecode sequence!")
        error("the disassembly of the offending function is shown below")
        print(dis.dis(fn))
        raise Exception(f"Unknown import bytecode sequence near {i}.")

    return imports

def resolve_import(importer_path: str, name: str):
    "Resolves the name of an import (e.g. `re` for `import re`) to a file path."

    name_path = name.replace(".", "/")

    path = os.path.join(
        os.path.dirname(importer_path),
        os.path.dirname(name_path),
        f"{os.path.basename(name_path)}.py"
    )

    if not os.path.exists(path):
        error("import failed!")
        error(f"  from: {importer_path}")
        error(f"  target: {name}")
        error(f"  final path: {path}")
        raise Exception(f"Import '{name}' not found from '{importer_path}")

    return path
