
# CFD-HPC

A modular Computational Fluid Dynamics framework developed
in modern C++.

## Project goals

- Finite Volume Method on structured meshes
- Scalar transport and incompressible Navier-Stokes
- Shared- and distributed-memory parallelization
- Integration with machine learning workflows

## Current status

Initial development environment setup.

## Requirements

- C++20 compiler
- CMake >= 3.20
- Ninja

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/cfd-hpc
```
