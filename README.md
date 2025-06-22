# LGE - Functional Programming Language Compiler

LGE (**L**ambda **g**(**e**)) is a toy functional programming language compiler that I wrote to try out llvm. 🙂

## Language Syntax

### Function Definitions
```lge
let name: type = (param: type, ...) -> expression
```

### Types
- `int`: 32-bit integer
- `float`: 32-bit floating point
- `char`: 8-bit character
- `str`: C-style string
- `func`: Function pointer

### Built-in Functions
- `str_print(str) -> int`: Print string to stdout
- `str_read(int) -> str`: Read string from stdin (up to n characters)
- `str_len(str) -> int`: Get length of string
- `str_at(str, int) -> char`: Get character at index
- `str_sub(str, int, int) -> str`: Get substring from start to end index
- `str_find(str, str) -> int`: Find substring (-1 if not found)
- `int_to_str(int) -> str`: Convert integer to string
- `str_to_int(str) -> int`: Convert string to integer
- `float_to_str(float) -> str`: Convert float to string
- `str_to_float(str) -> float`: Convert string to float
- `str_cmp(str) -> int`: Returns 1 if true

### Comments and Line Continuation
- Comments start with `#`
- Line continuation with `\`

## Building Prerequisites

- Linux
- CMake 3.20+
- C++20 compatible compiler
- LLVM 18 development libraries
- Git
- python3 (for running tests)

## Usage

```sh
$> lgec --help
LGE
Usage: lgec [OPTIONS] input_file

Positionals:
  input_file TEXT:FILE REQUIRED
                              Input LGE source file

Options:
  -h,--help                   Print this help message and exit
  --dump-tokens               Dump lexer tokens to stdout
  --dump-ast                  Dump AST to stdout
```

### Basic Compilation
```bash
$> ./lgec tests/examples/hello_world.lge | lli -load=liblge_runtime.so
Hello world!
```

## License

This project is licensed under the MIT License - see the [MIT License](LICENSE) file for details.
