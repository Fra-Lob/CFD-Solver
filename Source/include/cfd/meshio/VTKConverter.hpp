#pragma once

#include <cfd/mesh/Mesh.hpp>

#include <string>
#include <vector>

namespace cfd {

struct CellScalarData {
    std::string name;
    std::vector<double> values;
};

class VTKConverter {
public:
    static void write(
        const Mesh& mesh,
        const std::string& filename,
        const std::vector<CellScalarData>& cellScalars = {}
    );

    static Mesh read(const std::string& filename);
};

}
