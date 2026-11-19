#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/meshio/MeshGenerator.hpp>

#include <exception>
#include <iostream>

#include "MeshTests.hpp"

// ===================================================================================== //
//                               Mesh regression pipeline                                //
// ===================================================================================== //
void testMesh(const cfd::MeshConfig& config) {
    auto mesh = cfd::MeshGenerator::generate(config);

    test::testGenerator(mesh, config);

    cfd::MeshConnectivityBuilder::build(mesh);

    test::testConnectivity(mesh, config);

    cfd::MeshGeometryProcessor::computeFaces(mesh);
    cfd::MeshGeometryProcessor::computeCells(mesh);

    test::testGeometry(mesh, config);
}

// ===================================================================================== //
//                               Cartesian regression cases                              //
// ===================================================================================== //
int main() {
    try {
        testMesh({cfd::MeshType::Cartesian, {0, 1.0, 0, 1.0, 0, 1.0}, {2, 2, 2}});
        testMesh({cfd::MeshType::Cartesian, {0, 2.0, 0, 3.0, 0, 4.0}, {2, 2, 2}});
        testMesh({cfd::MeshType::Cartesian, {0, 2.0, 0, 3.0, 0, 4.0}, {2, 2, 2}});
        testMesh({cfd::MeshType::Cartesian, {0, 2.0, 0, 3.0, 0, 4.0}, {4, 3, 2}});
        testMesh({cfd::MeshType::Cartesian, {0, 2.0, 0, 3.0, 0, 4.0}, {10, 8, 6}});

        std::cout << "\nAll mesh tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] " << e.what() << '\n';
        return 1;
    }
}
