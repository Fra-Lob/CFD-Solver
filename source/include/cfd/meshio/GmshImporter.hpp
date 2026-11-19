#pragma once
#include <cfd/mesh/Mesh.hpp>

#include <filesystem>

namespace cfd {
    class GmshImporter {
        // ===================================================================================== //
        //                                 Gmsh import interface                                 //
        // ===================================================================================== //
    public:
        static bool available();
        // Uses the official Gmsh API, builds full topology, metrics and physical patches.
        static Mesh read(const std::filesystem::path& filename);
    };
} // namespace cfd
