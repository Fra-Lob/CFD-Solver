
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
cfd::MeshConfig config{cfd::MeshType::Cartesian, 3, {2.0, 3.0, 4.0}, 10, 8, 6};
auto generated = cfd::MeshGenerator::generate(config);
cfd::VTKWriter::write(generated, "mesh.vtu");

// Simulation setup (a later run).
auto mesh = cfd::VTKReader::read("mesh.vtu");
auto type = mesh.getType();
auto dimension = mesh.getDimension(); // Physical dimension, not node/cell counts.
const auto& domain = mesh.getDomain(); // {Lx, Ly, Lz} for Cartesian 3D.
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
Mesh properties are stored as three ASCII FieldData arrays on UnstructuredGrid:
`cfd_mesh_type` (1 = Cartesian, 2 = Cylindrical, 3 = Spherical),
`cfd_mesh_dimension`, and `cfd_mesh_domain` (one tuple with dimension components).
The reader restores them and rebuilds Cartesian boundary patches automatically:
0 = xmin, 1 = xmax, 2 = ymin, 3 = ymax, 4 = zmin, 5 = zmax.
The boundary builder uses `mesh.getDomain()` and checks all vertices of each
boundary face against the six domain planes; internal faces retain INVALID_ID.
It can also be called after generation as
`cfd::MeshBoundaryPatchBuilder::buildCartesian(mesh)` once connectivity is built.
PointData, CellData and unrelated FieldData are ignored; solution fields and
custom boundary patch labels are not imported.

Older/external files without these arrays still load with unspecified mesh
properties (Unknown, dimension 0, empty domain) and INVALID_ID boundary patches.
Supply the properties explicitly to reconstruct the patches:

```cpp
auto mesh = cfd::VTKReader::read("old-mesh.vtu", cfd::MeshType::Cartesian,
                                 3, {2.0, 3.0, 4.0});
```

## Mesh properties and supported domains

`Mesh(type, dimension, domain)` and `mesh.setProperties(type, dimension, domain)`
validate and set the complete descriptor. Domain parameters must be finite,
positive and match dimension (1, 2 or 3). Angles are in radians.

| Type | 1D domain | 2D domain | 3D domain |
| --- | --- | --- | --- |
| Cartesian | `{Lx}` | `{Lx, Ly}` | `{Lx, Ly, Lz}` |
| Cylindrical | `{R}` | `{L, R}` | `{L, R, theta}` |
| Spherical | `{R}` | `{R, theta}` | `{R, theta, phi}` |

Cylindrical theta and spherical phi are at most 2*pi; spherical theta is at most pi.
All coordinate intervals start at zero. Descriptors for cylindrical, spherical,
1D and 2D meshes can be constructed now, but generation, geometry processing and
boundary reconstruction currently support only Cartesian 3D. Unsupported
descriptors produce explicit errors in those operations and in the VTU reader.
Changing metadata does not move nodes or recompute geometry/patches; rebuild
them when necessary. Cartesian boundary classification requires a box aligned
with the coordinate axes, spanning `[0,Lx] x [0,Ly] x [0,Lz]`.

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
