#pragma once

#include <cfd/mesh/Mesh.hpp>

namespace cfd {

class MeshConnectivityBuilder
{
public:
    static void build(Mesh& mesh);
};

}