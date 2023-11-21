# ARM Assembler Project

## Overview
This project is an assembler for ARM architecture, designed to convert ARM assembly language code into machine code, with extended functionalities for handling ELF (Executable and Linkable Format) files.

## Features
- **Token Scanning**: Efficient scanning of ARM assembly language to identify lexical elements.
- **Parsing**: Constructs parse trees from assembly language inputs, handling a variety of assembly constructs.
- **Machine Code Generation**: Translates parsed assembly instructions into executable machine code.
- **ELF File Support**: Extensive support for generating and manipulating ELF format object files.
  - **Initialization**: Set up and initialize ELF contexts (`elf_init.c`).
  - **Adding Instructions and Symbols**: Add instructions and symbols to ELF sections and headers (`elf_add.c`).
  - **ELF Writing**: Finalize and write ELF structures to disk (`elf_write.c`).
- **Configurable Output**: Options for hex and ELF object file outputs.
- **Debugging Support**: Debugging features for development and troubleshooting.

## Requirements
- Standard C development environment
- GNU Make
- ELF library (included as a submodule)

## Installation

### Clone the Repository
```bash
git clone [repository URL]
cd [repository directory]
```

### Build the Project
```bash
make
```

## Usage
Use the assembler to convert ARM assembly code into machine code or an ELF object file.

### Basic Command
```bash
./project04 [options] <sourcefile.asm>
```

### Options
- `-o <outputfile>`: Specify the output file name.
- `--hex`: Generate output in hex format.
- `--obj`: Generate an ELF object file.
- `--debug`: Enable debugging mode.

## ELF Handling
- **Initialization (`elf_init.c`)**: Prepares ELF contexts for use.
- **Adding Instructions/Symbols (`elf_add.c`)**: Handles addition of instructions and symbols to ELF sections.
- **Writing ELF Files (`elf_write.c`)**: Finalizes and writes ELF structures to disk.

## Contributing
Contributions are welcome. Please send pull requests or report issues via the repository's issue tracker.
