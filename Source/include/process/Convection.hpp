#pragma once

#include "fields/VectorField.hpp"
#include <mesh/Mesh.hpp>
#include <numerics/LinearSystem.hpp>
#include <physics/BoundaryConditions.hpp> 
#include <utils/Math.hpp>

#include <algorithm> 

class Convection {
public:
    static void assemble(
        const Mesh&               mesh,
        const VectorField&        velocity,
        const double              rho,
        const BoundaryConditions& bc,
              LinearSystem&       system);
};