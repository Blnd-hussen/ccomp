# C++ Compiler Automation Tool (ccomp)

CCOMP is a command-line utility designed to automate the compilation and execution of C++ source files on Unix-like systems. it leverages regular expressions to extract include paths directly from the provided C++ code.

## Features

- Automatic include path extraction from the source file
- Optional specification of the compiler and version (e.g., `gnu-20`, `clang-20`).
- Optional execution of the compiled binary with valgrind
- Optional execution of the compiled binary
- Default output directory for compiled binaries (`./out`), with the option to specify a different path.

## Usage

The program expects at least one argument, which should be the path to a C++ file. Optionally, you can specify additional arguments to control the behaviour:

```bash
ccomp [options] <source_file>

Options:
  -c,  --compiler      Specifies the preferred compiler to use (e.g., gnu-20 or clang-20). If no valid compiler is provided, the default is system.
  -rv, --valgrind     Run the compiled program using Valgrind memory debugger after successful compilation (off by default).
  -r,  --run          Executes the compiled binary after successful compilation (default: off)
  -o,  --output       Specifies the output directory for the compiled binary (default: ./out)
```

## Example

Compile `file.cpp` and run the resulting binary:

```bash
ccomp -r file.cpp
```

Compile `file.cpp` and run the resulting binary under valgrind:

```bash
ccomp -rv file.cpp
```

Compile file.cpp with a custom output directory:

```bash
ccomp -o build file.cpp
```

Compile file.cpp with a custom output directory and run the resulting binary:

```bash
ccomp -r file.cpp -o build
```

The bellow command will evaluate to `clang++ -std=c++17 file.cpp -o build/file && valgrind build/file`

```bash
ccomp -rv file.cpp -o build -c clang-17
```

## Error Codes

- 1: Invalid usage (invalid source file)
- 2: C++ source file not provided
- 3: Compilation error or source file processing error

## Requirements

- Unix-like operating system
- GNU Compiler Collection (g++) or Clang compiler installed
- [Optional] valgrind installed

## How it Works

1. The program parses the command-line arguments, looking for the source file path and optional flags (-r, -rv, -o, -c) .

2. It validates the provided arguments and ensures a C++ source file is specified.

3. Include paths are extracted from the source file using regular expressions.

4. A system command is constructed using the preffered compiler path, the source file path, extracted include paths (if any), and the output directory.

5. The compilation is performed, and the exit code is checked for success.

6. If the -r flag is provided and compilation is successful, the program executes the compiled binary.

7. If the -rv flag is provided and compilation is successful, the program executes the compiled binary under valgrind.

8. If a compiler/compiler-version is specifies using the -c flag, the program will match the argument against the following regex:

```regex
^(gnu|clang)-[0-9]{2}$
```
