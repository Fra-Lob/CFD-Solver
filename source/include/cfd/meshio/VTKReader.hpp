#pragma once

#include <cfd/mesh/Mesh.hpp>

#include <string>
#include <vector>

namespace cfd {

    class VTKReader {
        // ===================================================================================== //
        //                                 VTU loading interface                                 //
        // ===================================================================================== //
    public:
        // Read a single-piece ASCII UnstructuredGrid containing linear hexahedra or generated rational Bezier hexahedra.
        // Rebuild faces, owners/neighbors, areas, centers and volumes.
        // Structured dimensions are recovered only for MeshGenerator's node/cell order;
        // otherwise they remain zero. cfd_mesh_* FieldData restores mesh properties
        // and enables generated-domain boundary classification. Without metadata,
        // properties remain unspecified and boundary patch IDs remain INVALID_ID.
        // PointData, CellData and unrelated FieldData are ignored.
        // Throws std::runtime_error for I/O errors, invalid or unsupported input.
        static Mesh read(const std::string& filename);

        // Supply generated-domain properties for older files without cfd metadata.
        // If the file already has properties they must agree with these arguments.
        static Mesh read(const std::string& filename,
                         MeshType type,
                         const MeshDomain& domain,
                         const MeshResolution& resolution);
    };

} // namespace cfd
