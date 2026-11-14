#pragma once

#include <cfd/mesh/Mesh.hpp>

namespace cfd {

class MeshGeometryProcessor
{
public:
    static void computeFaces(Mesh& mesh);
    static void computeCells(Mesh& mesh);

    static void compute(Mesh& mesh) {
        computeFaces(mesh);
        computeCells(mesh);
    }
};

}