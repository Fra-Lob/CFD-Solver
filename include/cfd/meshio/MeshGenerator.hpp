
#pragma once

#include <cfd/mesh/Mesh.hpp>
#include <cstddef>

namespace cfd
{

struct MeshConfig
{
    double Lx, Ly, Lz;
    std::size_t Nx, Ny, Nz;
};

class MeshGenerator
{
public:
    static Mesh generate(const MeshConfig& config);
};

} // namespace cfd
