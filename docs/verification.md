# Implementation and verification report

## Architecture changes

* Added `GridConfigReader` for strict case-directory configuration, and a common
  dispatch to Cartesian, cylindrical and spherical generation functions.
* Extended mesh metadata with a name, Custom origin and named patch IDs, while
  preserving the public SoA containers and existing entity accessors.
* Reused connectivity building for all origins. Added logical sector boundary
  classification; Gmsh boundaries use physical surface groups instead.
* Replaced absolute tetrahedron volume accumulation with consistent signed
  integration over shared canonical face triangles; added Jacobian certification
  and common completed-mesh validation.
* Added versioned, little-endian native serialization with full attributes and
  an FNV-1a checksum. Loading never invokes connectivity or geometry builders.
* Added optional Gmsh API integration with a clear no-SDK fallback.
* Added a standalone preprocessor with staged output replacement and rollback;
  the separate main application demonstrates native solver loading.

Principal additions are under `include/cfd/meshio` and `src/meshio`:
`GridConfigReader`, `GmshImporter`, `MeshReader`, `MeshWriter`, `NativeMeshIO` and
`Preprocessor`; plus `MeshValidator`, the internal `HexGeometry` helpers,
`apps/preprocessor.cpp`, the case fixtures, native/geometry documentation and
`tests/mesh/testPreprocessor.cpp`. Existing Mesh, generator, geometry,
connectivity, boundary builder, VTU I/O, CMake and README were adapted.
Legacy Cartesian tests were retained.

The checked-out branch was `main` (despite the attachment describing it as a
feature branch). No checkout, commit or branch modification was performed;
`initial_version` was not modified.

## Commands actually executed

The commands below record the initial architecture verification. Current build
shortcuts are `make` (Release, no tests) and `make debug` (Debug with tests), with
CTest run as `ctest --test-dir build-debug --output-on-failure`. Explicit CMake
test configurations now require `-DBUILD_TESTING=ON`.

```sh
cmake -S . -B build-preprocessor-check -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-preprocessor-check
ctest --test-dir build-preprocessor-check --output-on-failure
```

Clean configuration and build succeeded with GNU C++ 13.2.0 on Windows.
All **8 CTest groups passed**:

| Group | Result |
| --- | --- |
| Mesh | PASS: existing Cartesian generator/connectivity/geometry regression |
| VTKIO | PASS: existing ASCII VTU round trips and malformed inputs |
| MeshProperties | PASS: metadata and Cartesian boundary regression |
| Preprocessor_config | PASS: parser, required/unknown/duplicate fields, conversions, missing files |
| Preprocessor_generated | PASS: Cartesian/cylindrical/spherical counts, geometry, patches, analytic comparisons and invalid cells |
| Preprocessor_native | PASS: exact comparison of all attributes, byte-exact reserialization, checksum/truncation errors, VTU round trip and warped Custom hex |
| Preprocessor_pipeline | PASS: all three internal case workflows, replacement of valid output pairs, protection against invalid input |
| Preprocessor_gmsh | PASS: explicit unavailable-SDK diagnostic; enabled API tests were not run |

The CLI was run separately on the supplied internal cases:

| Case | Nodes | Cells | Faces | Validation and native reload |
| --- | ---: | ---: | ---: | --- |
| Cartesian box | 396 | 250 | 875 | PASS |
| Cylindrical annulus | 340 | 192 | 700 | PASS |
| Spherical shell sector | 676 | 432 | 1512 | PASS |

All three generated VTU files were also parsed with the independent .NET XML
parser in PowerShell; point/connectivity/type lengths and every cell offset
were checked successfully. Native files were loaded using the separate main
application and the native round-trip tests compare all stored fields exactly.
The custom CLI was run and correctly returned failure with the no-Gmsh message.

## Limits of this verification

No Gmsh SDK was found by CMake. The enabled C++ API integration and conditional
hex/tetrahedron import tests therefore remain **unverified in this environment**.
A representative tagged hex MSH fixture and unsupported tetrahedron fixture are
included for a build with the SDK. The no-SDK behavior was tested explicitly.

No ParaView, pvpython or usable VTK Python installation was found. Direct
ParaView/VTK-tooling compatibility was not tested; XML validation and the
project's VTU reader succeeded. No exact curved FVM metrics, singular or periodic
topology, reduced-dimensional solver, or nonhex volume elements are claimed.
Output safety was tested for repeated valid processing and invalid input;
power-loss recovery and injected filesystem failures were not tested.

See [the workflow and geometry conventions](preprocessor.md) and
[the native format specification](mesh-format.md) for the supported scope.
