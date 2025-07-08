import sys
if sys.version_info < (3, 13, 0):
    print("Python >=3.13 is required to run Pyton.")
    exit(1)

import os
import argparse
from sdk.transpiler import TranslationUnit
from sdk.compose import compile_and_link, create_iso

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input", type=str, help="the main entry-point file")
parser.add_argument("-a", "--artifacts", default="artifacts", help="the directory to write all artifacts to")
parser.add_argument("-O", "--optimize", action="store_true", help="enables GCC optimizations")
args = parser.parse_args()

entrypoint_source = open(args.input).read()
compiled = compile(entrypoint_source, args.input, "exec")

transpiler = TranslationUnit()
transpiler.translate(compiled, is_entrypoint = True)

transpiled = transpiler.transpile()

os.makedirs(args.artifacts, exist_ok = True)

kernel_name = os.path.splitext(os.path.basename(args.input))[0]
kernel_source_file = os.path.join(args.artifacts, f"{kernel_name}.c")
open(kernel_source_file, "w").write(transpiled)

kernel_binary = compile_and_link(
    kernel_source_file,
    "./runtime",
    "./lib/freestnd-c-hdrs",
    "./lib",
    args.artifacts,
    args.optimize
)

tmp_iso_root = os.path.join(args.artifacts, "obj", "iso")

create_iso(
    kernel_binary,
    "./resource/iso",
    tmp_iso_root,
    "./lib/limine",
    os.path.join(args.artifacts, f"{kernel_name}.iso")
)