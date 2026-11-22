
#pragma once

#include <cfd/mesh/Mesh.hpp>

namespace cfd
{

struct MeshConfig
{
    double lx, ly, lz;
    std::size_t nx, ny, nz;
};

class MeshGenerator
{
public:
    static Mesh generate(const MeshConfig& config);
};

} // namespace cfd
