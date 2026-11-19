# Verification of the domain and curved-geometry update

## Commands executed

```sh
cmake -S . -B build-check -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-check --parallel 6
ctest --test-dir build-check --output-on-failure
```

GNU C++ 13.2.0 on Windows: configuration and compilation succeeded.
All eight CTest groups passed:

| Group | Checks |
| --- | --- |
| Mesh | Cartesian generation, node/cell numbering, connectivity, geometry, closure |
| VTKIO | Translated Cartesian and rational Bezier round trips, name/resolution metadata, full-turn cylinder/sphere, malformed XML and arrays |
| MeshProperties | Fixed six endpoints, resolution validation, atomic setters, translated boundary planes |
| Preprocessor_config | Type-specific endpoint keys, strict parsing, degrees-to-radians conversion, invalid fields/counts |
| Preprocessor_generated | Exact cylindrical/spherical volumes, cylindrical surface areas and volume centroid, periodic seam topology, spherical outer area, invalid geometry |
| Preprocessor_native | Exact metadata/entities, byte-identical reserialization, checksum/truncation errors, VTU reload, warped Custom mesh |
| Preprocessor_pipeline | All internal examples, repeated output replacement, invalid-input protection |
| Preprocessor_gmsh | Clear unavailable-SDK diagnostic; enabled Gmsh API branch not tested |

The VTU test independently evaluates exported rational Bernstein sums at
parameters between control points. It checks the physical radius on inner/outer
surfaces and inside cells for cylindrical and spherical geometry, including
full turns. A straight chord representation would fail this test.

## Example workflows

The preprocessor was run on all three internal example directories and regenerated
both ASCII VTU and native version-2 files:

| Case | Solver nodes | Cells | Faces | Physical patches |
| --- | ---: | ---: | ---: | ---: |
| Cartesian box | 396 | 250 | 875 | 6 |
| Full cylindrical annulus | 320 | 192 | 688 | 4 |
| Spherical shell interval | 676 | 432 | 1512 | 6 |

VTU control points are additional export data, so curved VTU point counts exceed
solver node counts. Cell counts are preserved. The cylindrical example retains
a 360-degree extent and now uses 16 angular cells in place of the invalid count 1.

## Verification limits

ParaView and VTK Python are not installed. Actual rendering in ParaView has not
been tested here; the implementation follows VTK's rational Bezier cell format,
and XML structure, round trips and rational radius evaluation were verified.
Increase Nonlinear Subdivision Level in ParaView to see smooth surface rendering.

No Gmsh SDK was found. The enabled API import branch remains unverified in this
environment; its explicit unavailable-dependency path passed.

Axes/origins, spherical poles and non-hexahedral volume cells remain unsupported.
See [the workflow](preprocessor.md) and [native format version 2](mesh-format.md).
