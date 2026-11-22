#pragma once

#include <cfd/utils/Math.hpp>
#include <cfd/mesh/Nodes.hpp>
#include <cfd/mesh/Faces.hpp>
#include <cfd/mesh/Cells.hpp>

namespace cfd {

class Mesh {
    std::array<double, 3> domain{};

public:
// ====================================================================//
//                            Mesh Entities                            //
// ====================================================================//
    Nodes nodes;
    Faces faces;
    Cells cells;

// ====================================================================//
//                          Getters & Setters                          //
// ====================================================================//
    void setDomain(double dim1, double dim2, double dim3) {
        domain = {dim1, dim2, dim3};
    }

    std::array<double, 3> getDomain() const {
        return domain;
    }

};

}