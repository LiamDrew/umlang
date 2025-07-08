# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is `umlang`, a high-performance implementation of the Universal Machine (UM) - a 32-bit virtual machine with a RISC-style instruction set. The project includes multiple runtime implementations optimized for different use cases:

- **Interpreted Runtime** (`runtimes/interp/`) - Basic emulator-based UM runtime
- **JIT Runtime** (`runtimes/jit/`) - Just-in-time compiler for Arm64 and x86-64 platforms (up to 4.5x faster than emulator)
- **LLVM IR Runtime** (`runtimes/ir/`) - LLVM-optimized runtime leveraging IR optimizations
- **Assembler/Disassembler** (`umasm/`) - Tools for UM assembly language programs

## Architecture

### Universal Machine Specification
- 32-bit virtual machine with 8 general-purpose registers
- 14-instruction RISC-style instruction set
- Segmented memory model with 32-bit segment identifiers
- Special segment 0 contains executable instructions

### Key Performance Innovation
The JIT runtime uses a custom 32-bit memory allocator (Virt32) that maps 4GB of contiguous virtual memory and treats 32-bit addresses as offsets from a base 64-bit address. This eliminates the need for segment lookup tables and enables 32-to-64-bit address translation with a single addition instruction instead of memory access.

## Build and Development Commands

### JIT Runtime (Primary)
```bash
# Build JIT runtime (darwin-arm64)
cd runtimes/jit/darwin-arm64/jit
make

# Run benchmark
./jit ../umasm/sandmark.umz

# Clean
make clean
```

### LLVM IR Runtime
```bash
# Build IR runtime
cd runtimes/ir
mkdir -p build && cd build
cmake ..
make

# Run
./compiler [program.um]
```

### Assembler Tools
```bash
# Build assembler
cd umasm/tools
make

# Test runtimes
cd umasm
./runtest.sh [executable_name]           # Run all tests
./runtest.sh [executable_name] [test]    # Run specific test
```

## Directory Structure

- `runtimes/` - Runtime implementations
  - `interp/` - Interpreted runtime
  - `jit/` - JIT compiler (platform-specific subdirectories)
  - `ir/` - LLVM IR-based runtime
  - `tools/virt32/` - Virt32 memory allocator
- `umasm/` - UM assembler/disassembler and test binaries
- `docs/` - Project documentation and specifications

## Platform Support

### JIT Runtime Platforms
- `darwin-arm64/` - Native macOS on Apple Silicon
- `linux-arm64-container/` - ARM64 Linux via Docker
- `linux-x86-64-container/` - x86-64 Linux via Docker

### Docker Usage (Linux platforms)
```bash
# Build containers from project root
docker buildx build --platform=linux/amd64 -f Dockerfile.x86_64 -t umlang-x86_64 .
docker buildx build --platform=linux/arm64 -f Dockerfile.arm64 -t umlang-arm64 .

# Run containers with full project access
docker run --platform=linux/amd64 -it -v "$PWD:/home/developer/umlang" umlang-x86_64
docker run --platform=linux/arm64 -it -v "$PWD:/home/developer/umlang" umlang-arm64

# Inside container, build and test
cd runtimes/jit/linux-x86-64-container/jit && make
cd runtimes/jit/linux-arm64-container/jit && make
cd umasm && ./runtest.sh ../runtimes/jit/linux-x86-64-container/jit/jit
```

## Testing

Test programs are located in `umasm/umbinary/` and include:
- `sandmark.umz` - Primary performance benchmark (2M instructions)
- `midmark.um` - Alternative benchmark
- Various functional tests (`hello.um`, `add.um`, etc.)

## Performance Notes

The JIT runtime achieves significant performance improvements:
- 4.58x faster than emulator on Apple M3 (Arm64)
- 3.5x faster than emulator on Intel Xeon (x86-64)
- Key optimizations: register allocation, ahead-of-time compilation, custom memory allocator

## Important Files

- `runtimes/jit/darwin-arm64/jit/jit.c` - Main JIT compiler implementation
- `runtimes/jit/darwin-arm64/jit/utility.S` - Hand-written assembly for complex instructions
- `runtimes/tools/virt32/virt.c` - Virt32 memory allocator implementation
- `umasm/runtest.sh` - Test runner script