#pragma once
#include <cfd/meshio/MeshGenerator.hpp>

#include <filesystem>

namespace cfd {
    class GridConfigReader {
        // ===================================================================================== //
        //                             Case configuration interface                              //
        // ===================================================================================== //
    public:
        static MeshConfig read(const std::filesystem::path& caseDirectory);
    };
} // namespace cfd
