#pragma once

#include <cfd/mesh/Mesh.hpp>
#include <string>
#include <fstream>
#include <iomanip>
#include <locale>

namespace cfd {

class VTKWriter {
public:
    // Write the mesh as a VTK XML UnstructuredGrid in ASCII format.
    // filename is used as supplied (include the .vtu extension).
    // Existing files are overwritten. Faces and geometry need not be computed.
    // Throws std::invalid_argument for invalid node references or coordinates,
    // and std::runtime_error if opening or writing the file fails.
    static void write(
        const Mesh& mesh,
        const std::string& filename
    );
};

}
