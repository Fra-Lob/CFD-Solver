#pragma once

#include <cfd/mesh/Mesh.hpp>

#include <map>
#include <algorithm>

namespace cfd {

class MeshConnectivityBuilder
{
public:
    static void build(Mesh& mesh);
};

}