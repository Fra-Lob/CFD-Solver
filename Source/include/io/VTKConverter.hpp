#pragma once

#include <mesh/Mesh.hpp>
#include <io/SimulationConfig.hpp>

#include <string>
#include <vector>

struct CellScalarData {
    std::string name;
    std::vector<double> values;
};

class VTKConverter {
public:
    static void write(
        const Mesh& mesh,
        const std::string& filename
    );
    static void write(
        const Mesh& mesh,
        const std::string& filename,
        const std::vector<CellScalarData>& cellScalars
    );
    static void write(
        const Mesh& mesh,
        const std::string& filename,
        const SimulationConfig& config
    );

    static Mesh read(const std::string& filename);
};