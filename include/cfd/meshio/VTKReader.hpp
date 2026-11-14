#pragma once

#include <cfd/mesh/Mesh.hpp>
#include <string>

namespace cfd {

class VTKReader {
public:
    // Read a single-piece ASCII UnstructuredGrid containing VTK_HEXAHEDRON cells.
    // Rebuild faces, owners/neighbors, areas, centers and volumes.
    // Structured dimensions are recovered only for MeshGenerator's node/cell order;
    // otherwise they remain zero. PointData, CellData and FieldData are ignored;
    // boundary patch IDs remain INVALID_ID and solution fields are not restored.
    // Throws std::runtime_error for I/O errors, invalid or unsupported input.
    static Mesh read(const std::string& filename);
};

}
