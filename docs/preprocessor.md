# Mesh preprocessing

The public workflow is `cfd::Preprocessor::run(caseDirectory)`, also exposed by
the standalone `mesh_preprocessor` executable. The future solver uses
`cfd::MeshReader::read(path)` and receives the same `Mesh` for every origin.
Public `nodes`, `faces` and `cells` remain structure-of-arrays containers.

The application code lives under `source/apps`, public headers under
`source/include/cfd` and implementations under `source/src`. Regression suites
remain under the top-level `tests` directory. The root `.clang-format` records
the shared C++ formatting conventions. CMake sections and C++ section headers
separate configuration, metadata, topology, geometry, I/O and validation phases.

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

## grid.info and domain description

A generated mesh is described by exactly four properties: name, type, domain,
and resolution. There is no independent dimension field or alternate domain schema.
Only volume hexahedra are implemented. Under the current resolution convention,
each generated axis must have more than one cell; other resolutions are rejected.

The domain is always `std::array<double, 6>`:
`{Dim1In, Dim1Out, Dim2In, Dim2Out, Dim3In, Dim3Out}`.
Resolution is `std::array<std::size_t, 3>`, in the corresponding coordinate order.
The type determines the physical meaning of the intervals:

| Type | Six domain values, in storage order | Resolution |
| --- | --- | --- |
| Cartesian | xMin, xMax, yMin, yMax, zMin, zMax | Nx, Ny, Nz |
| Cylindrical | rInner, rOuter, thetaStart, thetaEnd, zMin, zMax | Nr, Ntheta, Nz |
| Spherical | rInner, rOuter, polarStart, polarEnd, azimuthStart, azimuthEnd | Nr, Ntheta, Nphi |
| Custom | Six reserved zero values; explicit imported nodes define the geometry | Three zeros: unstructured indexing |

The input uses those type-specific keys, plus `name` and `type`. Custom input
only needs name/type and reads `<name>.msh`. See [the examples](../examples/).
Lengths are metres; input angles are degrees, converted to radians internally.
Cartesian domains can be translated and can contain negative coordinates.
Every interval must increase. The old Lx/Ly/Lz and cylindrical length keys are
replaced by explicit endpoints, without a second compatibility schema.

Blank lines and `#`/`;` comments are supported, including inline comments. Keys
are case sensitive. Duplicate, missing, unknown or invalid fields fail. Names
are plain file stems containing letters, digits, dots, underscores or hyphens.

```cpp
cfd::MeshConfig config{
    cfd::MeshType::Cylindrical,
    {1.0, 2.0, 0.0, std::numbers::pi / 2, -1.0, 3.0},
    {4, 12, 6},
    "annulus"
};
auto mesh = cfd::MeshGenerator::generate(config);
```

[Mesh](../source/include/cfd/mesh/Mesh.hpp) provides a constructor and atomic
`setProperties(type, domain, resolution)`, plus getters for all four properties
and `setName`. Changing metadata does not regenerate existing entities.
A default mesh has unspecified properties for external files without metadata.
Patch names map IDs to labels; individual face IDs remain in `Faces`.

## Coordinates, topology and boundaries

A single structured grid loop uses the type to map coordinates:

* Cartesian: `(x,y,z)`.
* Cylindrical: `(r*cos(theta), r*sin(theta), z)`.
* Spherical: `(r*sin(polar)*cos(azimuth), r*sin(polar)*sin(azimuth), r*cos(polar))`.

Cells retain eight corner nodes and six faces. The same connectivity builder
creates owners/neighbours for all types. Partial angular intervals use ordinary
boundary faces. A complete 360-degree cylindrical angle or spherical azimuth
wraps connectivity and shares the seam nodes and faces; the seam is internal.
There are no overlapping artificial boundary walls. Full turns require at least
three angular cells; each angular cell must span less than 180 degrees.

Patch IDs follow the lower/upper side of each coordinate, 0 through 5.
Cartesian names are xmin/xmax/ymin/ymax/zmin/zmax; cylindrical names are
inner/outer/angular_start/angular_end/axial_start/axial_end; spherical names are
inner/outer/polar_start/polar_end/azimuth_start/azimuth_end. A full turn omits
the two periodic direction patches. Imported custom patches use Gmsh physical tags.

Radial coordinates currently require `0 < rInner < rOuter`. Spherical polar
bounds must lie strictly between 0 and 180 degrees. Axes, origins and poles need
different cell topologies and are not implemented. Angular extents above a full
turn are rejected.

## Geometry used by the solver

Generated cylindrical and spherical meshes use exact integrals of their curved
coordinate surfaces, not chordal polyhedra:

* Cylindrical cell volume: `(ro^2-ri^2)/2 * (theta1-theta0) * (z1-z0)`.
* Spherical cell volume: `(ro^3-ri^3)/3 * (cos(polar0)-cos(polar1)) * (azimuth1-azimuth0)`.

Cell centers are volume centroids; face centers are surface-area centroids.
Scalar face area integrates `dA`. The oriented area vector integrates `n dA`
and points outward from the owner. Its magnitude can be smaller than the scalar
area on curved faces. Shared faces use the same metrics for both incident cells,
and oriented area vectors close for each cell. Annular cells need not be convex:
centroid-to-face and straight triangle tests do not define their curved normals.

The validator checks generated curved node coordinates/connectivity against the
domain/resolution, verifies analytic metrics and checks topology, patch coverage
and vector-area closure. It does not modify a loaded mesh.

Cartesian, external unspecified and Custom meshes use the existing generic
polyhedral calculation. Each face is rotated to start at its lowest node ID and
triangulated along a shared canonical diagonal. Areas and centers are integrated
over those triangles; signed tetrahedra give volumes and volume centroids.
Trilinear Jacobians and triangle orientation are checked conservatively.
Unsupported, inverted, degenerate or numerically uncertifiable cells fail.

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

## Outputs

`<name>.vtu` is ASCII VTK XML. Cartesian/custom output uses linear hexahedra
(type 12). Generated cylindrical/spherical output uses quadratic rational Bezier
hexahedra (type 79), with 27 control points per cell and `RationalWeights` in
PointData. Adjacent cells share controls. Rational angular weights represent
circular arcs exactly, including spherical surfaces. Control points are export
geometry, not extra solver nodes; the number of cells is unchanged.

In ParaView, increase **Nonlinear Subdivision Level** on the representation
(e.g. 3 or 4) to render the curved surfaces smoothly. Rendering still tessellates
surfaces into pixels/triangles; the file contains the rational curved geometry.
See [VTK's rational Bezier implementation report](https://www.kitware.com/main/wp-content/uploads/2020/03/Implementation-of-rational-Be%CC%81zier-cells-into-VTK-Report.pdf).

`cfd_mesh_type`, `cfd_mesh_name`, `cfd_mesh_domain` (six components), and
`cfd_mesh_resolution` (three components) FieldData restore generated metadata.
The name is stored as numeric UTF-8 bytes to avoid XML escaping ambiguities.
The reader accepts external linear hexes and this generated Bezier layout,
validates its controls/weights/topology, and restores the original solver mesh.
It does not support arbitrary external Bezier/NURBS meshes. Custom VTU output
does not carry physical patch metadata; native storage preserves it.

`<name>.mesh` stores completed mesh entities and all metadata, using
[native format version 2](mesh-format.md). Existing version-1 outputs must be
regenerated with the preprocessor. The native loader restores the stored metrics
and connectivity directly. Analytic validation checks their consistency without
rebuilding or replacing them.

Both outputs are staged before replacing existing files, with backups and rollback
for ordinary I/O failures. This is not a crash-atomic two-file transaction. Case
source files and directories/symlinks are not overwritten as output files.

## Tests

Run `make debug` followed by `ctest --test-dir build-debug --output-on-failure`.
The suites cover configuration errors, fixed metadata, translated Cartesian
bounds, analytical curved metrics, periodic seam connectivity, VTU round trips,
native byte-exact round trips and corrupt input. The Gmsh API cases run only
when its optional SDK is linked; otherwise the missing-dependency path is tested.
