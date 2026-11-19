#pragma once

#include <cfd/mesh/Mesh.hpp>
#include <cfd/meshio/MeshGenerator.hpp>

namespace test {

    // ===================================================================================== //
    //                                   Cartesian regression interfaces                     //
    // ===================================================================================== //
    void testGenerator(const cfd::Mesh& mesh, const cfd::MeshConfig& config);

    void testConnectivity(const cfd::Mesh& mesh, const cfd::MeshConfig& config);

    void testGeometry(const cfd::Mesh& mesh, const cfd::MeshConfig& config);

} // namespace test
