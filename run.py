import os
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--iso", required=True, help="the ISO to run")
parser.add_argument("-d", "--debug", action="store_true", help="wait for GDB to attach")
args = parser.parse_args()

if args.debug:
    open(".gdbinit", "w").writelines(
        f"""
        set osabi none
        target remote localhost:1234
        file ./artifacts/obj/{os.path.splitext(os.path.basename(args.iso))[0]}.elf
        """
    )

subprocess.run([
    "qemu-system-x86_64",
    "-M", "q35",
    "-cdrom", args.iso,
    "-boot", "d",
    "-m", "2G",
    "--no-reboot",
    "--no-shutdown",
    *(["-S", "-s"] if args.debug else [])
])