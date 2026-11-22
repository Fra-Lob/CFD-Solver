#pragma once

#include <cfd/mesh/Mesh.hpp>

#include <string>

namespace cfd {

class MeshConverter {
public:
    static void write(const Mesh& mesh, const std::string& filename);
    static Mesh read(const std::string& filename);
};

}
