#pragma once

#include <mesh/Mesh.hpp>

#include <string>

class MeshConverter {
public:
    static void write(const Mesh& mesh, const std::string& filename);
    static Mesh read(const std::string& filename);
};
