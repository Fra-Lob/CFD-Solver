#pragma once

#include <cfd/mesh/Mesh.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/mesh/BoundaryPatchBuilder.hpp>

#include <string>
#include <charconv>
#include <fstream>
#include <iterator>
#include <locale>
#include <sstream>
#include <string_view>
#include <type_traits>

namespace cfd {

class VTKReader {
public:
    // Read a single-piece ASCII UnstructuredGrid containing VTK_HEXAHEDRON cells.
    // Rebuild faces, owners/neighbors, areas, centers and volumes.
    // Structured dimensions are recovered only for MeshGenerator's node/cell order;
    // otherwise they remain zero. cfd_mesh_* FieldData restores mesh properties
    // and enables automatic Cartesian boundary classification. Without metadata,
    // properties remain unspecified and boundary patch IDs remain INVALID_ID.
    // PointData, CellData and unrelated FieldData are ignored.
    // Throws std::runtime_error for I/O errors, invalid or unsupported input.
    static Mesh read(const std::string& filename);

    // Supply properties explicitly for older/external files without cfd metadata.
    // If the file already has properties they must agree with these arguments.
    static Mesh read(const std::string& filename, MeshType type,
                     unsigned dimension, const std::vector<double>& domain);
};

}
