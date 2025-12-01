# chess-king

C++ Chess AI

## Build

This project uses CMake. To build:

```bash
mkdir build
cd build
cmake ..
make
```

The executable will be located at `build/chess-king`.

## Usage

```bash
./build/chess-king -H <path to input history file> -m <path to output move file>
```

- `-H`: Path to the input history file containing moves in UCI long algebraic notation.
- `-m`: Path where the AI will write its next move.

## Constraints

- Pure C++ (STL only)
- Single-threaded
- No third-party dependencies

