# Native mesh format, version 1

The native file restores a completed `cfd::Mesh`. It does not contain C++ object
memory, padding, vector layouts or implementation-specific serialized objects.
All integers are unsigned, fixed width, **little endian**. All real values are
IEEE-754 binary64, also little endian. IDs are zero-based except patch IDs,
which may retain external physical tags. `UINT64_MAX` encodes `INVALID_ID`.
Readers reject integers that cannot be represented by the host's `size_t`.

Notation: `u32` = 4 bytes, `u64` = 8 bytes, `f64` = 8 bytes; `vec3` = three f64s
in XYZ order. A string is a u64 byte length followed by exactly that many bytes,
without a terminator. Strings are opaque UTF-8-compatible byte sequences; lengths
are limited to 1 MiB by this implementation. Entity IDs are implicit record indices.

## Exact field ordering

| Field | Encoding |
| --- | --- |
| Magic | 8 bytes: `43 46 44 4d 45 53 48 00` (`CFDMESH\0`) |
| Version | u32, value 1 |
| Metric convention | u32, value 1 (canonical triangulated faces) |
| Origin/type | u32: 0 unknown, 1 Cartesian, 2 cylindrical, 3 spherical, 4 custom |
| Physical dimension | u32 (3 for the preprocessor; 0 only for unspecified legacy metadata) |
| Mesh name | string |
| Domain parameter count | u64 |
| Domain parameters | that many f64 values; schema in [preprocessor documentation](preprocessor.md) |
| Patch count | u64 |
| Patch metadata | for each patch, ascending ID: u64 patch ID, string name |
| Logical node dimensions | three u64: Nx, Ny, Nz (all zero when unspecified) |
| Logical cell dimensions | three u64: Nx, Ny, Nz (all zero when unspecified) |
| Node count | u64 |
| Cell count | u64 |
| Face count | u64 |
| Nodes | records below, in node ID order |
| Cells | records below, in cell ID order |
| Faces | records below, in face ID order |
| Checksum | u64 FNV-1a of every preceding byte |

A node record is **24 bytes**: `vec3 coordinates`.

A cell record is **144 bytes**:

1. Eight u64 node IDs in VTK_HEXAHEDRON order.
2. Six u64 face IDs, local order bottom/top/front/back/left/right, with local
   corner sets `{0,3,2,1}`, `{4,5,6,7}`, `{0,1,5,4}`, `{3,7,6,2}`, `{0,4,7,3}`, `{1,2,6,5}`.
3. f64 positive volume.
4. vec3 volumetric centroid.

A face record is **112 bytes**:

1. Four u64 node IDs in outward winding with respect to the owner.
2. u64 owner cell ID.
3. u64 neighbour cell ID, or UINT64_MAX for a boundary face.
4. vec3 area-weighted centroid.
5. f64 positive scalar area.
6. vec3 oriented area vector, outward from the owner.
7. u64 boundary patch ID, or UINT64_MAX for an internal face.

FNV-1a starts at decimal `14695981039346656037`. For every byte, XOR that unsigned
byte into the accumulator, then multiply by `1099511628211` modulo 2^64.
This is a corruption check, not authentication. The final checksum field is
excluded from its own digest.

## Reading and validation

The reader rejects unknown versions or metric conventions, truncated/trailing
data, checksum failures, impossible record counts, unreasonable string lengths,
nonfinite values, invalid metadata and invalid references. Counts are checked
against remaining file bytes before entity allocation. Mesh validation also
checks the restored topology, geometry constraints and patch classification.
Builders are never called on load and stored centroids, volumes, area vectors
and scalar areas are not recalculated or replaced.

```cpp
#include <cfd/meshio/MeshReader.hpp>
auto mesh = cfd::MeshReader::read("case/name.mesh");
```

The current version stores chordal/triangulated geometry only. Exact curved FVM
metrics, other topologies or additional solver fields would require an explicit
future convention/version rather than silently changing the meaning of these fields.
