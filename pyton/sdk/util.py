import os
import shutil
from typing import TypeVar, Callable, Iterable

T = TypeVar("T")

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
