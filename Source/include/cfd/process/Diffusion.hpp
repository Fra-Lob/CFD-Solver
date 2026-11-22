#pragma once

#include <cfd/mesh/Mesh.hpp>
#include <cfd/numerics/LinearSystem.hpp>
#include <cfd/physics/BoundaryConditions.hpp> 

class Diffusion {
public:
    static void assemble(
        const Mesh& mesh,
        double gamma,
        const BoundaryConditions& bc,
        LinearSystem& system
    );
};