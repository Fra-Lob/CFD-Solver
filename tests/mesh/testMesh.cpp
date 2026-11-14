#include "MeshTests.hpp"

#include <cfd/meshio/MeshGenerator.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>

#include <exception>
#include <iostream>

void testMesh(const cfd::MeshConfig& config)
{
    auto mesh = cfd::MeshGenerator::generate(config);

    test::testGenerator(mesh, config);

    cfd::MeshConnectivityBuilder::build(mesh);

    test::testConnectivity(mesh, config);

    cfd::MeshGeometryProcessor::computeFaces(mesh);
    cfd::MeshGeometryProcessor::computeCells(mesh);

    test::testGeometry(mesh, config);
}

int main()
{
    try {
        testMesh({1.0, 1.0, 1.0, 1, 1, 1});
        testMesh({2.0, 3.0, 4.0, 2, 1, 1});
        testMesh({2.0, 3.0, 4.0, 2, 2, 2});
        testMesh({2.0, 3.0, 4.0, 4, 3, 2});
        testMesh({2.0, 3.0, 4.0, 10, 8, 6});

        std::cout << "\nAll mesh tests passed!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] " << e.what() << '\n';
        return 1;
    }
}