#pragma once

#include <fields/ScalarField.hpp>
#include <fields/VectorField.hpp>

#include <cstddef>
#include <string>
#include <vector>

struct SimulationConfig {
    std::string type;
    std::string meshFile;
    std::vector<ScalarField> scalarFields;
    VectorField velocity;

    double timeStep = 0.0;
    std::size_t timeSteps = 0;
    double density = 1.0;

    double tolerance = 0.0;
    std::size_t maxIterations = 0;
};