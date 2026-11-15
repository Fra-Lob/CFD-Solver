
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
    std::string name = "mesh";
};

class MeshGenerator
{
public:
    // Sector domains use radii, length/angle bounds; all internal angles are radians.
    static void validate(const MeshConfig& config);
    static Mesh generate(const MeshConfig& config);
private:
    static Mesh generateCartesian(const MeshConfig& config);
    static Mesh generateCylindrical(const MeshConfig& config);
    static Mesh generateSpherical(const MeshConfig& config);
};

} // namespace cfd
