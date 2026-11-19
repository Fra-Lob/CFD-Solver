# CFD solver and mesh preprocessor

A C++20 finite-volume project with a standalone mesh preparation workflow.
The existing structure-of-arrays `Mesh`, `Nodes`, `Faces` and `Cells` are shared
by the internal generators, optional Gmsh importer and native solver loader.

## Project layout and code style

```text
source/
    apps/       Application entry points
    include/    Public cfd headers
    src/        Implementations and private helpers
tests/          Regression suites and fixtures
examples/       Input cases
docs/           Workflow and native format documentation
```

All C++ files follow the root `.clang-format` configuration: four-space
indentation, explicit control-flow blocks, one operation per line and separated
function definitions. Section headers follow the existing entity containers.
The more involved parsing, geometry and serialization code also uses descriptive
names and comments identifying its main phases.

## Build and test

Requirements: a C++20 compiler, CMake >= 3.20 and Ninja. Gmsh is optional.

From the project root, the shortcuts are:

```sh
make
make debug
clean
```

`make` configures a **Release** Ninja build in `build-native` and compiles only
the preprocessor and native mesh loader. Test targets are disabled.
`make debug` configures a **Debug** build in `build-debug`, including all tests;
test executables are placed in `build-debug/tests`, outside the repository root.
Run the suite with `ctest --test-dir build-debug --output-on-failure`.
`clean` removes this project's recognized `build`/`build-*` CMake build directories
and known executables in the repository root, including legacy Windows/WSL names.
Source files, case configurations and case mesh outputs are preserved.
`make clean` is also supported.

In VS Code, **open a new terminal** after loading the project: workspace terminal
settings add the repository to PATH, making bare `make`/`clean` available on Windows.
On Linux/macOS, GNU Make uses the provided Makefile; the first configuration makes
the `clean` helper executable. Outside VS Code, use `./clean` on Unix, or
`.\make.cmd` and `.\clean.cmd` in PowerShell, or add the repository to your PATH.
Existing terminals can use the explicit paths immediately; no global shell
profile is modified.

There is one build configuration: `CMakeLists.txt`. The small Makefile contains
only command aliases to CMake, not a second source list or set of compiler flags.
The extensionless `clean` file is a Unix shell script, not a compiled executable;
`clean.cmd` serves Windows, where that shell script cannot run directly.
Both delegate to the same CMake cleanup action. These platform entry points are
kept so the existing short commands continue to work without changing shell profiles.

The equivalent CMake commands remain available:

```sh
cmake -S . -B build-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-native

cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
```

Application executables (`mesh_preprocessor` and `Main`) are written to the
repository root. On Windows CMake adds the `.exe` suffix automatically.

## Prepare a case

```sh
./mesh_preprocessor examples/cartesian
./mesh_preprocessor examples/cylindrical
./mesh_preprocessor examples/spherical
```

Each case contains `grid.info`. Processing generates `<name>.vtu` for ParaView
and `<name>.mesh` with completed topology, geometry and named boundary patches.
Invalid input does not replace valid existing outputs.

```cpp
#include <cfd/meshio/MeshReader.hpp>
auto mesh = cfd::MeshReader::read("examples/cartesian/box.mesh");
```

The native loader restores all stored data without running connectivity or
geometry builders. It is independent of the mesh origin. The separate main
application demonstrates this loading step; the CFD solver is still future work.

## Documentation

- [Configuration, architecture, geometry conventions and Gmsh setup](docs/preprocessor.md).
- [Versioned binary native mesh format](docs/mesh-format.md).
- [Cartesian example](examples/cartesian/grid.info).
- [Cylindrical annulus](examples/cylindrical/grid.info).
- [Spherical shell sector](examples/spherical/grid.info).
- [Custom Gmsh case](examples/custom/grid.info), requiring the optional SDK.

All solver cells have eight corner nodes and six faces. Generated cylindrical
and spherical meshes use analytic curved metrics and rational Bezier VTU cells
for curved visualization. Their domain is always six ordered endpoints, with a
separate three-value resolution and no dimension field. Full angular turns share
seam connectivity. Axes, origins, spherical poles and non-hexahedral Gmsh volumes
remain unsupported. See [configuration and geometry](docs/preprocessor.md).

Previously generated native outputs use an older format and must be regenerated.
