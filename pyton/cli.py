import sys
import os
import signal
import argparse
import subprocess
from textwrap import dedent

from .sdk.transpiler import TranslationUnit
from .sdk.compose import compile_and_link, create_iso

is_wsl = os.path.isfile("/usr/bin/wslpath")

def get_package_root():
    return os.path.dirname(os.path.abspath(__file__))

def build_command(args: argparse.Namespace):
    assert type(args.input) is str
    assert type(args.artifacts) is str
    assert type(args.optimize) is bool

    entrypoint_source = open(args.input).read()
    compiled = compile(entrypoint_source, args.input, "exec")

    transpiler = TranslationUnit()
    entrypoint_fn = transpiler.translate(compiled, args.input, "__main__")
    transpiled = transpiler.transpile(entrypoint_fn)

    os.makedirs(args.artifacts, exist_ok=True)

    kernel_name = os.path.splitext(os.path.basename(args.input))[0]
    kernel_source_file = os.path.join(args.artifacts, f"{kernel_name}.c")
    open(kernel_source_file, "w").write(transpiled)

    package_root = get_package_root()
    kernel_binary = compile_and_link(
        kernel_source_file,
        os.path.join(package_root, "runtime"),
        os.path.join(package_root, "lib/freestnd-c-hdrs/include"),
        os.path.join(package_root, "lib"),
        args.artifacts,
        args.optimize
    )

    tmp_iso_root = os.path.join(args.artifacts, "obj", "iso")

    create_iso(
        kernel_binary,
        os.path.join(package_root, "resource/iso"),
        tmp_iso_root,
        os.path.join(package_root, "lib/limine"),
        os.path.join(args.artifacts, f"{kernel_name}.iso")
    )

def run_command(args: argparse.Namespace):
    assert type(args.target) is str
    assert type(args.debug) is bool

    if args.target.endswith(".iso"):
        print("(warning) target kernel ends with '.iso' - 'run' expects an extensionless name")
        print("(warning) for 'example.iso' or 'example.py', the kernel name is 'example'")

    iso = os.path.join("artifacts", f"{args.target}.iso")

    # if args.debug:
    #     gdbinit_path = os.path.join(os.getcwd(), ".gdbinit")
    #     kernel_name = os.path.splitext(os.path.basename(iso))[0]
    #     with open(gdbinit_path, "w") as f:
    #         f.write(dedent(
    #             f"""
    #             set osabi none
    #             target remote localhost:1234
    #             file ./artifacts/obj/{kernel_name}.elf
    #             """
    #         ).strip())

    qemu_args = [
        "qemu-system-x86_64" if not is_wsl else "qemu-system-x86_64.exe",
        "-M", "q35",
        "-cdrom", iso,
        "-boot", "d",
        "-d", "cpu_reset",
        "-m", "2G",
        "--no-reboot",
        "--no-shutdown"
    ]
    
    if args.debug:
        qemu_args.extend(["-S", "-s"])

    subprocess.run(qemu_args)

def debug_command(args: argparse.Namespace):
    assert type(args.target) is str

    p = subprocess.Popen([
        "gdb",
        "-ex", "set osabi none",
        "-ex", f"file ./artifacts/obj/{args.target}.elf",
        "-ex", "target remote localhost:1234"
    ])

    while p.poll() is None:
        try:
            p.wait()
        except KeyboardInterrupt:
            p.send_signal(signal.SIGINT)

def main():
    if sys.version_info < (3, 13, 0):
        print("Python >=3.13 is required to run Pyton.")
        sys.exit(1)

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build", help="Compile Python code to a bootable image")
    build_parser.add_argument("-i", "--input", type=str, required=True, help="the main entry-point file")
    build_parser.add_argument("-a", "--artifacts", default="artifacts", help="the directory to write all artifacts to")
    build_parser.add_argument("-O", "--optimize", action="store_true", help="enables GCC optimizations")

    run_parser = subparsers.add_parser("run", help="Run a compiled Pyton kernel in QEMU")
    run_parser.add_argument("-t", "--target", required=True, help="the name of the kernel under ./artifacts, w/o extension")
    run_parser.add_argument("-d", "--debug", action="store_true", help="wait for GDB to attach")

    debug_parser = subparsers.add_parser("debug", help="Launches GDB targetting the given kernel")
    debug_parser.add_argument("-t", "--target", required=True, help="the name of the kernel under ./artifacts, w/o extension")

    args = parser.parse_args()

    if args.command == "build":
        build_command(args)
    elif args.command == "run":
        run_command(args)
    elif args.command == "debug":
        debug_command(args)

if __name__ == '__main__':
    main()
