#pragma once

#include <mesh/Mesh.hpp>
#include <numerics/LinearSystem.hpp>
#include <physics/BoundaryConditions.hpp> 
#include <utils/Math.hpp>

class Diffusion {
public:
    static void assemble(
        const Mesh& mesh,
        double gamma,
        const BoundaryConditions& bc,
        LinearSystem& system
    );
};