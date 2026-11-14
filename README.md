
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

## Saving and loading meshes

```cpp
#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/meshio/VTKWriter.hpp>
#include <cfd/meshio/VTKReader.hpp>

// Mesh preparation (first run).
auto generated = cfd::MeshGenerator::generate({2.0, 3.0, 4.0, 10, 8, 6});
cfd::VTKWriter::write(generated, "mesh.vtu");

// Simulation setup (a later run).
auto mesh = cfd::VTKReader::read("mesh.vtu");
```

The reader supports single-piece ASCII XML UnstructuredGrid files with
eight-node hexahedra (VTK type 12), including files produced by the writer.
It rebuilds faces, owners/neighbors, boundary detection, face areas and area
vectors, face/cell centers and cell volumes using the existing mesh processors.
Malformed input, binary/appended arrays, multiple pieces and other cell types
produce exceptions. No external XML or VTK library is required.

Structured node/cell dimensions are recovered when the complete connectivity
matches the generator's indexing convention, including sheared meshes.
Otherwise these dimensions remain zero; access the mesh by node/cell IDs.
PointData, CellData and FieldData are ignored: solution fields and boundary
patch labels are not imported. Boundary patch IDs remain INVALID_ID and must
be assigned before applying boundary conditions.

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
