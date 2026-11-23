#pragma once

#include <io/SimulationConfig.hpp>
#include <io/OutputConfig.hpp>
#include <mesh/Mesh.hpp>

class CSVExporter {
public:
    static void write(
        const Mesh& mesh,
        const SimulationConfig& config,
        const OutputConfig& output,
        std::size_t timeStep = 0
    );
};