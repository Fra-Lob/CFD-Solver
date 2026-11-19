#pragma once
#include <cfd/mesh/Mesh.hpp>

#include <filesystem>

namespace cfd {
    class MeshReader {
        // ===================================================================================== //
        //                             Native mesh loading interface                             //
        // ===================================================================================== //
    public:
        // Restores stored data directly; no topology or geometry builders are called.
        static Mesh read(const std::filesystem::path& filename);
    };
} // namespace cfd
