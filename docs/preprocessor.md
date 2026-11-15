# Mesh preprocessing

The public workflow is `cfd::Preprocessor::run(caseDirectory)`, also exposed by
the standalone `mesh_preprocessor` executable. The future solver uses
`cfd::MeshReader::read(path)` and receives the same `Mesh` for every origin.
Public `nodes`, `faces` and `cells` remain structure-of-arrays containers.

## Build and run

From the repository root:

Use `make` for Release without test targets, `make debug` for Debug with tests,
and `clean` to remove recognized build directories
and executables, or `make clean`. On Windows these are local command wrappers;
open a new VS Code terminal to apply the workspace PATH setting, or call
`.\make.cmd` / `.\clean.cmd` directly. On Unix, `make` uses the root Makefile and
the clean helper can also be called as `./clean`. Case inputs and mesh outputs
outside build directories are preserved. The explicit commands below still work:

```sh
cmake -S . -B build-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-native
make debug
ctest --test-dir build-debug --output-on-failure
./mesh_preprocessor examples/cartesian
./mesh_preprocessor examples/cylindrical
./mesh_preprocessor examples/spherical
```

On Windows use `./mesh_preprocessor.exe`. Application executables are emitted in
the source root; test executables are isolated in `build-debug/tests`.
The separate `Main` target demonstrates loading a completed native mesh.
CMake adds the platform's executable suffix, avoiding duplicate `.exe` suffixes.

The preprocessor returns nonzero on failure and prints entity counts, patch names
and counts, validation status and output paths on success. Both outputs are
written into the case directory, with the configured name as their file stem.
It also prints progress before and after configuration, generation/import,
connectivity, geometry, patch classification, validation, output preparation and
installation. Messages are flushed immediately so the last active phase remains
visible on failure. Library callers stay quiet by default; pass an optional
`std::ostream*` to `Preprocessor::run` to receive the same progress messages.

## grid.info

See the complete sample cases under [examples](../examples/). Blank lines and
`#` or `;` comments, including inline comments, are supported. Fields are
case-sensitive and can appear in any order. Duplicate, unknown and missing
fields, trailing numeric text, nonfinite values and unsafe file names fail.
Lengths use metres, user-facing angles use **degrees**, and resolutions count
three-dimensional cells even when a resolution equals one.

Every file requires `name` and `type`. A name is a plain file stem using letters,
digits, dots, underscores or hyphens, excluding `.` and `..`.

| type | Geometry fields | Resolution fields |
| --- | --- | --- |
| cartesian | Lx, Ly, Lz | Nx, Ny, Nz |
| cylindrical | rInner, rOuter, length, thetaStart, thetaEnd | Nr, Ntheta, Nz |
| spherical | rInner, rOuter, polarStart, polarEnd, azimuthStart, azimuthEnd | Nr, Ntheta, Nphi |
| custom | none; reads `<name>.msh` in the case directory | none |

Cartesian coordinates span `[0,Lx] x [0,Ly] x [0,Lz]`.
Cylindrical sectors map `x=r*cos(theta)`, `y=r*sin(theta)`, `z=axial`.
Spherical sectors use polar angle theta from +Z and azimuth phi from +X towards
+Y: `x=r*sin(theta)*cos(phi)`, `y=r*sin(theta)*sin(phi)`, `z=r*cos(theta)`.
All internal angles and metadata bounds are radians.

For sectors, `0 < rInner < rOuter` and ordered angular bounds are mandatory.
Cylindrical length is positive. Total cylindrical angle and spherical azimuth
extent are strictly below 360 degrees; each angular chord cell must span less
than 180 degrees. Spherical polar bounds are strictly between 0 and 180 degrees.
The axis, origin, poles and periodic full-turn topology are explicitly unsupported.
Invalid, inverted, degenerate and numerically uncertifiable cells fail validation.

## Internal interfaces and metadata

`MeshGenerator::generate(MeshConfig)` dispatches to dedicated Cartesian,
cylindrical and spherical coordinate functions. A common logical grid helper
preserves the eight-node VTK hex order. Topology is built once using the existing
connectivity builder. Geometry is completed once, boundary patches assigned, then
the common validator runs. Gmsh imports arrive completed and are not rebuilt by
the preprocessor. The native writer validates before serializing, and the native
reader validates restored data without running builders or modifying metrics.

`Mesh` retains `getType()`, `getDimension()`, `getDomain()`, constructor and
`setProperties()`. It also has `getName()/setName()` and
`getPatchNames()/setPatchNames()`. Patch metadata maps an ID to a name; per-face
IDs remain stored only in `Faces`. Origin is Cartesian, Cylindrical, Spherical
or Custom, independently of physical boundary conditions.

Generated sector metadata uses the following arrays:

* Cartesian: `{Lx,Ly,Lz}`.
* Cylindrical: `{rInner,rOuter,length,thetaStart,thetaEnd}`.
* Spherical: `{rInner,rOuter,polarStart,polarEnd,azimuthStart,azimuthEnd}`.
* Custom: an empty array, dimension 3; no Cartesian bounds are inferred.

The earlier short 1D/2D/3D descriptor constructors remain available, but generation
requires full 3D sector descriptors. Physical dimensional reduction is not implemented.
Unspecified legacy VTU metadata is still allowed; it does not imply a Cartesian box.

Logical counts in `Nodes` and `Cells` are independent of physical dimension.
Their axes are `(x,y,z)`, `(r,theta,z)` or `(r,polar,azimuth)` respectively.
Patch IDs 0..5 follow the min/max of these logical axes. Cartesian names are
xmin/xmax/ymin/ymax/zmin/zmax; sector names are listed by the CLI. Custom meshes
preserve physical surface tags and names from Gmsh instead.

## Discrete geometry convention

The metrics describe the **piecewise planar mesh**, not an exact curved solid.
Every global quadrilateral has a fixed diagonal: cyclically rotate its nodes so
the lowest global node ID is first, then use triangles `(0,1,2)` and `(0,2,3)`.
Both incident cells therefore use exactly the same surface. The scalar face area
is the sum of triangle areas; its vector is the sum of oriented triangle area
vectors, and its center is the scalar-area-weighted triangle centroid.
On a warped face, scalar area can exceed the magnitude of the area vector.

Cell volume and volumetric centroid use signed tetrahedral integration over that
closed triangulated boundary, with a local reference to reduce cancellation.
No absolute-volume correction hides an inverted cell. This gives analytical
triangle/tetrahedron metrics for the discrete solid and exact Cartesian box
metrics. For a cylindrical annulus the total chordal volume is
`0.5*(ro^2-ri^2)*length*Ntheta*sin(deltaTheta)`, approaching the curved volume
under angular refinement. Spherical metrics similarly converge to the analytical
shell-sector volume `(ro^3-ri^3)/3*(cos(theta0)-cos(theta1))*(phi1-phi0)`.
No exact curved areas are mixed into these chordal volumes or normals.

The validator checks topology, distinct nodes and faces, reciprocal incidences,
positive finite metrics, outward face/triangle orientation, patch completeness
and per-cell area-vector closure. Trilinear hex Jacobian positivity is certified
over the reference cube using tensor-quadratic Bernstein bounds and adaptive
subdivision (at most six bisections per axis). Near singular or unresolved cases
are rejected conservatively. The selected triangulated cells must also be
star-shaped with respect to the integration reference and stored volume centroid.
Highly warped valid finite elements outside this supported representation can
therefore be rejected; arbitrary polyhedra are not supported.

## Gmsh dependency

The official [Gmsh API](https://gmsh.info/doc/texinfo/gmsh.html#Gmsh-application-programming-interface)
is optional. CMake searches for `gmsh.h` and the `gmsh` library when
`CFD_ENABLE_GMSH=ON` (default). Supply an SDK location, using an SDK compatible
with the compiler and C++ ABI:

```sh
cmake -S . -B build-native -G Ninja -DGMSH_ROOT=/path/to/gmsh-sdk
```

Alternatively set `GMSH_INCLUDE_DIR` and `GMSH_LIBRARY` explicitly. On Windows
the corresponding DLL must be discoverable at runtime (e.g. on PATH). To disable
the integration use `-DCFD_ENABLE_GMSH=OFF`. Without an SDK, all internal cases
still build and custom processing fails with an explicit unavailable message.

The importer supports API-readable MSH files with linear hexahedral volume
elements (Gmsh type 5) and linear quadrangular physical surfaces (type 3).
It maps arbitrary node tags to contiguous IDs and derives corner permutation
from Gmsh reference-element coordinates. All volume element blocks are examined:
tetrahedra and other unsupported types are rejected, never skipped.
Physical surface names and tags are preserved. Missing or overlapping surface
classification, nonexternal physical faces and unsupported surface elements fail.
Triangle metrics and signed cell integration are computed by the common processor;
the importer does not assume the API provides complete FVM face metrics.
The importer owns a serial Gmsh session and refuses to run inside an existing one.

## Outputs and safety

`<name>.vtu` uses standard ASCII VTK XML UnstructuredGrid and type-12 hexahedra.
Its geometry is suitable for ParaView. Optional `cfd_mesh_*` FieldData records
generated-domain properties for the existing VTU reader; these fields are not
the solver's authoritative storage. Custom VTU output does not encode physical
patch data; use the native file for solver setup.

`<name>.mesh` preserves all completed data; see [the native format](mesh-format.md).

Validation precedes export. Both files are staged in a uniquely created directory
inside the case directory. Existing files are backed up before the completed pair
is installed, and ordinary I/O failures roll back both outputs. Restoration
failures leave backups and report their location. This is not a crash-atomic
two-file transaction and does not coordinate concurrent runs on the same case.
Output directories or symlinks are refused instead of being replaced.

## Tests

CTest keeps the Cartesian, geometry, property and VTU regression tests, and adds
configuration, curved generation/validation, full native round-trip, complete case
pipeline and optional Gmsh groups. Native round trips compare every stored entity
attribute and metadata, then verify byte-for-byte reserialization. Corruption,
truncation, invalid inputs and protection of existing outputs are covered.
The Gmsh group tests the API fixture and unsupported tetrahedra only when linked;
otherwise it explicitly reports that only the unavailable-dependency path ran.
