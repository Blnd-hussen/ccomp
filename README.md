# C++ Compiler Automation Tool (ccomp)

CCOMP is a command-line utility designed to automate the compilation and execution of C++ source files on Unix-like systems. It utilizes the GNU Compiler Collection (g++) for compilation and leverages regular expressions to extract include paths directly from the provided C++ code.

## Features

- Automatic include path extraction from the source file
- Optional execution of the compiled binary
  Usage:

## Usage

The program expects at least one argument, which should be the path to a C++ file. Optionally, you can specify additional arguments to control the behavior:

```bash
ccomp [options] <source_file>

Options:
  -r,  --run          Executes the compiled binary after successful compilation (default: off)
  -o,  --output       Specifies the output directory for the compiled binary (default: ./out)
```

## Example

Compile `file.cpp` and run the resulting binary:

```bash
ccomp -r file.cpp
```

Compile file.cpp with a custom output directory:

```bash
ccomp -o /build file.cpp
```

Compile file.cpp with a custom output directory and run the resulting binary:

```bash
ccomp -r file.cpp -o /build
```

## Error Codes

- 1: Invalid usage (invalid source file)
- 2: C++ source file not provided
- 3: Compilation error or source file processing error

## Requirements

- Unix-like operating system
- GNU Compiler Collection (g++) installed

## How it Works

1. The program parses the command-line arguments, looking for the source file path and optional flags (-r for run and -o for output directory).

2. It validates the provided arguments and ensures a C++ source file is specified.

3. Include paths are extracted from the source file using regular expressions.

4. A system command is constructed using g++, the source file path, extracted include paths (if any), and the output directory.

5. The compilation is performed, and the exit code is checked for success.

6. If the -r flag is provided and compilation is successful, the program executes the compiled binary.
