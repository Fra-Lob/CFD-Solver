#pragma once
#include <cfd/mesh/Mesh.hpp>
#include <filesystem>
namespace cfd {
class GmshImporter {
public:
    static bool available();
    // Uses the official Gmsh API, builds full topology, metrics and physical patches.
    static Mesh read(const std::filesystem::path& filename);
};
}
