import os
import glob
from subprocess import run

def path_to_libname(path: str):
    "Converts a path like `sys/mm.c` to `sys_mm`."
    return os.path.splitext(
        path.replace("/", "_").replace("\\", "_")
    )[0]

def compile_runtime(output_path: str, root: str, libc_headers_root: str, lib_headers: str):
    """
    Compiles the Pyton runtime as a static library. `root` determines the root directory of
    the runtime, while `output_path` determines the path where the library will be written to.
    """
    
    source_files = glob.glob("**/*.c", root_dir = root, recursive = True)
    print(f"(0/{len(source_files)}) compiling runtime...")

    object_files: list[str] = []
    obj_dir = os.path.dirname(output_path)
    for i, file in enumerate(source_files):
        out = os.path.join(obj_dir, path_to_libname(file)) + ".o"
        object_files.append(out)

        print(f"({i + 1}/{len(source_files)}) > {file} -> {out}")

        run([
            "gcc",
            "-Wall", "-Wno-unknown-pragmas", "-Wextra", "-Wno-unused-parameter",
            "-masm=intel",
            "-O2",
            "-m64",
            "-ffreestanding", "-nostdlib",
            "-mno-red-zone",
            "-I", root,
            "-isystem", libc_headers_root,
            "-isystem", lib_headers,
            "-c",
            os.path.join(root, file),
            "-o", out
        ], check = True)

    run(["ar", "rcs", output_path, *object_files]).check_returncode()
    print(f"(ok) runtime built and written to {output_path}")

def compile_and_link(
    filename: str,
    runtime_root: str,
    libc_headers_root: str,
    lib_path: str,
    artifact_dir: str
):
    """
    Compiles and links a C source file that was transpiled from Python by Pyton to a ready
    kernel binary. Returns the absolute path to the binary.
    """
    obj_dir = os.path.join(artifact_dir, "obj")
    os.makedirs(obj_dir, exist_ok = True)

    kernel_name = os.path.splitext(os.path.basename(filename))[0]
    kernel_obj_file = os.path.join(obj_dir, kernel_name + ".o")
    runtime_obj_file = os.path.join(obj_dir, "runtime.a")

    compile_runtime(runtime_obj_file, runtime_root, libc_headers_root, lib_path)

    print(f"(...) compiling kernel... {filename} -> {kernel_obj_file}")

    run([
        "gcc",
        "-Wall",
        "-Wextra",
        "-std=gnu17",
        "-nostdinc",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-stack-check",
        "-fno-PIC",
        "-ffunction-sections",
        "-fdata-sections",
        "-m64",
        "-march=x86-64",
        "-mno-80387",
        "-mno-mmx",
        "-mno-sse",
        "-mno-sse2",
        "-mno-red-zone",
        "-mcmodel=kernel",
        "-isystem", runtime_root,
        "-isystem", libc_headers_root,
        "-isystem", lib_path,
        "-DLIMINE_API_REVISION=3", # NOTE: Limine-dependent
        "-MMD",
        "-MP",
        "-c",
        filename,
        "-o", kernel_obj_file
    ]).check_returncode()

    dependencies = [
        "flanterm/flanterm.c",
        "flanterm/backends/fb.c"
    ]

    built_dependencies: list[str] = []

    print(f"(0/{len(dependencies)}) compiling dependencies...")
    for i, dep in enumerate(dependencies):
        out_path = os.path.join(obj_dir, path_to_libname(dep) + ".o")
        
        print(f"({i}/{len(dependencies)}) {dep} -> {out_path}")
        run([
            "gcc",
            "-Wall", "-Wno-unknown-pragmas", "-Wextra", "-Wno-unused-parameter",
            "-masm=intel",
            "-O2",
            "-m64",
            "-ffreestanding", "-nostdlib",
            "-mno-red-zone",
            "-isystem", libc_headers_root,
            "-c",
            os.path.join(lib_path, dep),
            "-o", out_path
        ], check = True)

        built_dependencies.append(out_path)

    kernel_binary = os.path.join(obj_dir, kernel_name + ".elf")
    print(f"(...) linking kernel... -> {kernel_binary}")

    run([
        "gcc",
        "-Wl,-m,elf_x86_64",
        "-Wl,--build-id=none",
        "-nostdlib",
        "-static",
        "-z", "max-page-size=0x1000",
        "-Wl,--gc-sections",
        "-T", os.path.join(runtime_root, "linker.ld"),
        *built_dependencies,
        kernel_obj_file,
        runtime_obj_file,
        "-o", kernel_binary
    ]).check_returncode()

    print(f"(ok) kernel linked! binary was placed in {kernel_binary}")

    return os.path.abspath(kernel_binary)