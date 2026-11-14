
#pragma once

#include <cfd/mesh/Mesh.hpp>

namespace cfd
{

struct MeshConfig
{
    MeshType type = MeshType::Unknown;
    unsigned dimension = 0;
    std::vector<double> domain;
    std::size_t Nx = 0, Ny = 0, Nz = 0;
};

class MeshGenerator
{
public:
    // Only Cartesian 3D generation is implemented; other descriptors are rejected.
    static Mesh generate(const MeshConfig& config);
};

} // namespace cfd
