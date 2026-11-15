#pragma once
#include <cfd/meshio/MeshGenerator.hpp>
#include <filesystem>
namespace cfd {
class GridConfigReader {
public:
    static MeshConfig read(const std::filesystem::path& caseDirectory);
};
}
