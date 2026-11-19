#pragma once

#include <cfd/mesh/Mesh.hpp>

#include <string>

namespace cfd {

    class VTKWriter {
        // ===================================================================================== //
        //                                 VTU export interface                                  //
        // ===================================================================================== //
    public:
        // Write the mesh as a VTK XML UnstructuredGrid in ASCII format.
        // Generated curved domains use rational Bezier cells; other meshes use linear hexes.
        // filename is used as supplied (include the .vtu extension).
        // Existing files are overwritten. Faces and geometry need not be computed.
        // Throws std::invalid_argument for invalid node references or coordinates,
        // and std::runtime_error if opening or writing the file fails.
        static void write(const Mesh& mesh, const std::string& filename);
    };

} // namespace cfd
