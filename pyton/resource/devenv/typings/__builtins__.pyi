def read8(address: int) -> int: ...
def read16(address: int) -> int: ...
def read32(address: int) -> int: ...
def read64(address: int) -> int: ...

def write8(address: int, value: int) -> None: ...
def write16(address: int, value: int) -> None: ...
def write32(address: int, value: int) -> None: ...
def write64(address: int, value: int) -> None: ...

def extern(fn):
    """
    Specifies that a Python function declaration should be a marshalling stub to
    an imported native method.
    ```
        @extern
        def terminal_println(text: str) -> None: ...
    ```

    Python objects will be marshalled to their C forms as follows:
    - `str` -> `const char*`,
    - `int` -> `int64_t`,
    - `float` -> `double`,
    - `bool` -> `int` (`True` = `1`, `False` = `0`),
    - `NoneType` -> `uint64_t` of value `NULL`,
    - everything else -> `pyobj_t*`.

    Functions with this decorator must have their return type and parameters type-hinted.
    Parameters may be any type, while the return type is constrained to these type hints:
    - `str`: equivalent to `(const char*)ret_val`,
    - `bool`: `True` when `ret_val != 0`, `False` otherwise,
    - `int`: equivalent to `(int64_t)ret_val`,
    - `float`: equivalent to `(double)ret_val`,
    - `None`: `ret_val` is discarded, always `py_none`.

    This means that any C functions that return structures are not supported. They
    must return a value that fits inside a 64-bit register (pointers, integers, or
    floating-point values up to `double`).
    """
    return fn

def c_function(fn):
    '''
    Specifies that a Python function returns a string literal that is intended to be
    embedded directly into the transpiled source code of the resulting binary.
    For the parameters, the same marshalling rules apply as with `@extern`.
    ```
        @c_function
        def outb(port: int, value: int):
            return R"""
                uint16_t pport = port;
                uint8_t pvalue = value;

                __asm__ volatile (
                    "outb %b0, %w1"
                    : : "a"(pvalue), "Nd"(pport)
                    : "memory"
                );
            """
    ```
    '''
    return fn