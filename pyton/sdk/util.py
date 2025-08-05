import os
import shutil
from typing import TypeVar, Callable, Iterable

T = TypeVar("T")
K = TypeVar("K")
V = TypeVar("V")

def unwrap(val: T | None) -> T:
    "Raises an exception if `val` is `None` - otherwise, returns `val`."

    if val is None:
        raise Exception()
    
    return val

def error(msg: str):
    print(f"error: {msg}")

def copy_many(root: str, file_list: list[tuple[str, str]]):
    for src, dst in file_list:
        dst = os.path.join(root, dst)

        if os.path.isdir(src):
            shutil.copytree(src, dst)
        else:
            os.makedirs(os.path.dirname(dst), exist_ok = True)
            shutil.copy(src, dst)

def flatten(xss: list[list[T]]):
    return [x for xs in xss for x in xs]

def find(collection: Iterable[T], predicate: Callable[[T], bool]):
    return next((x for x in collection if predicate(x)), None)

def omit(x: dict[K, V], target: K):
    "Returns a dictionary with every item except the one that has a key equal to `target`."
    return { k: v for k, v in x.items() if k != target }

def list_executables_on_path():
    """
    Finds all executable binaries on the PATH environment variables (i.e. ones that can be
    invoked by just their name, without needing their full absolute paths).
    """

    paths = os.environ["PATH"].split(os.path.pathsep)
    executables: list[str] = []
    for path in filter(os.path.isdir, paths):
        for file in os.listdir(path):
            if os.access(os.path.join(path, file), os.X_OK):
                executables.append(file)

    return executables