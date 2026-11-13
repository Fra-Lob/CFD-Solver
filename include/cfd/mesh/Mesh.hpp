#pragma once

#include <cfd/mesh/Nodes.hpp>
#include <cfd/mesh/Faces.hpp>
#include <cfd/mesh/Cells.hpp>

namespace cfd {

class Mesh
{
public:
    Nodes nodes;
    Faces faces;
    Cells cells;

};

}