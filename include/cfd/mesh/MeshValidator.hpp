#pragma once
#include <cfd/mesh/Mesh.hpp>
namespace cfd {
class MeshValidator {
public:
    // Validate completed topology, certified hex validity, stored metrics and patches.
    // Does not change the mesh or recompute its stored geometry.
    static void validate(const Mesh& mesh);
};
}
