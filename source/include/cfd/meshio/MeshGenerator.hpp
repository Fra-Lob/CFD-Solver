
#pragma once

#include <cfd/mesh/Mesh.hpp>

namespace cfd {

    // ===================================================================================== //
    //                               Mesh generation configuration                           //
    // ===================================================================================== //
    struct MeshConfig {
        MeshType type = MeshType::Unknown;
        MeshDomain domain{};
        MeshResolution resolution{};
        std::string name = "mesh";
    };

    class MeshGenerator {
    public:
        // ===================================================================================== //
        //                                  Common generation interface                          //
        // ===================================================================================== //
        // Sector domains use radii, length/angle bounds; all internal angles are radians.
        static void validate(const MeshConfig& config);
        static Mesh generate(const MeshConfig& config);
    };

} // namespace cfd
