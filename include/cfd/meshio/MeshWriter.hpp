#pragma once
#include <cfd/mesh/Mesh.hpp>
#include <filesystem>
namespace cfd {
class MeshWriter {
public:
    static void write(const Mesh& mesh, const std::filesystem::path& filename);
};
}
