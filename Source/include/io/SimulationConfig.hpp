#pragma once

#include <fields/ScalarField.hpp>

#include <cstddef>
#include <string>
#include <vector>

struct SimulationConfig {
    std::string name;
    std::string type;
    std::string meshFile;
    std::vector<ScalarField> scalarFields;

    bool transient = false;
    double timeStep = 0.0;
    double endTime = 0.0;
    std::string mode;

    std::string linearSolver;
    double tolerance = 0.0;
    std::size_t maxIterations = 0;

    std::string outputDirectory = "results";
    bool writeVtk = false;
};