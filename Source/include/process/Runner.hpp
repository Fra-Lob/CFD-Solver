#pragma once    

#include <io/SimulationConfig.hpp>
#include <io/OutputConfig.hpp>
#include <mesh/Mesh.hpp>

class SimulationRunner {
public:
    static void run(
        const Mesh& mesh,
        SimulationConfig& config,
        const OutputConfig& output);
};