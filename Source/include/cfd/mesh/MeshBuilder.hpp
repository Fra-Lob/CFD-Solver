#pragma once

#include <cfd/mesh/Mesh.hpp>

#include <algorithm>
#include <cstddef>
#include <map>

namespace cfd {

enum class CartesianPatch : std::size_t {
    XMin = 0,
    XMax = 1,
    YMin = 2,
    YMax = 3,
    ZMin = 4,
    ZMax = 5
};

class MeshBuilder
{
public:
    static void computeConnectivity(Mesh& mesh);

    static void computeFacesGeometry(Mesh& mesh);
    static void computeCellsGeometry(Mesh& mesh);

    static void computeFullGeometry(Mesh& mesh) {
        computeFacesGeometry(mesh);
        computeCellsGeometry(mesh);
    }

    static void computeBoundaryPatch(Mesh& mesh);
};

}