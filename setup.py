import os
from setuptools import setup, find_packages

def package_files(directory: str):
    paths: list[str] = []
    for path, _, filenames in os.walk(directory):
        for filename in filenames:
            paths.append(os.path.join("..", path, filename))

    return paths

extra_files = package_files("runtime") + package_files("lib") + package_files("resource")

setup(
    name = "pyton",
    version = "0.1.0",
    packages = find_packages(),
    package_data = {
        "pyton": extra_files,
    },
    include_package_data = True,
    entry_points = {
        "console_scripts": [
            "pyton=pyton.cli:main",
        ],
    },
    python_requires = ">=3.13",
    description = "A bare-metal standalone Python runtime",
    author = "ascpixi",
    classifiers = [
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.13",
        "Operating System :: OS Independent",
    ],
)
