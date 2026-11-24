#pragma once    

#include <io/SimulationConfig.hpp>
#include <io/OutputConfig.hpp>
#include <mesh/Mesh.hpp>

#include <io/CSVExporter.hpp>
#include <io/VTKConverter.hpp>
#include <numerics/LinearSystem.hpp>
#include <process/Diffusion.hpp>
#include <process/TimeDerivative.hpp>

#include <eigen3/Eigen/IterativeLinearSolvers>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

class SimulationRunner {
public:
    static std::string timeStampedFilename(const std::string& filename, 
                                                 std::size_t  timeStep);

    static void writeOutputs(const Mesh&             mesh,
                             const SimulationConfig& config,
                             const OutputConfig&     output,
                                   std::size_t       timeStep,
                                   bool              writeVtk,
                                   bool              writeCsv);

    static void run(const Mesh&             mesh,
                          SimulationConfig& config,
                    const OutputConfig&     output);
};