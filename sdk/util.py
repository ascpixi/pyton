import os
import shutil

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