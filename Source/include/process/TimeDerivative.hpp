#pragma once

#include <mesh/Mesh.hpp>
#include <numerics/LinearSystem.hpp>
#include <fields/ScalarField.hpp> 
#include <utils/Math.hpp>

class TimeDerivative {
public:
    static void assemble(
        const Mesh& mesh,
        double rho,
        double dt,
        const ScalarField& oldField,
        LinearSystem& system);
};