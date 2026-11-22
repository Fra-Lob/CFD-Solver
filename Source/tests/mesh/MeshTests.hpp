#include "TestUtils.hpp"
#include <cfd/mesh/MeshBuilder.hpp>
#include <cfd/meshio/MeshGenerator.hpp>

#include "TestConnectivity.cpp"
#include "TestEntities.cpp"
#include "TestGeometry.cpp"

#include <exception>
#include <array>
#include <iostream>

int main()
{
    test::SuiteReporter reporter("MESH TESTS");
    try {
        test::testEntities(reporter);

        const std::array configs = {
            MeshConfig{1.0, 1.0, 1.0, 1, 1, 1},
            MeshConfig{2.0, 3.0, 4.0, 2, 1, 1},
            MeshConfig{2.0, 3.0, 4.0, 2, 2, 2},
            MeshConfig{2.0, 3.0, 4.0, 4, 3, 2},
            MeshConfig{2.0, 3.0, 4.0, 10, 8, 6}
        };
        reporter.subgroup("Connectivity");
        reporter.run("face and cell adjacency on representative grids", [&] {
            for (const auto& config : configs) {
                auto mesh = MeshGenerator::generate(config);
                MeshBuilder::computeConnectivity(mesh);
                test::testConnectivity(mesh, config);
            }
        });
        reporter.subgroup("Geometry");
        reporter.run("face and cell geometry and boundary patches on representative grids", [&] {
            for (const auto& config : configs) {
                auto mesh = MeshGenerator::generate(config);
                MeshBuilder::computeConnectivity(mesh);
                MeshBuilder::computeFullGeometry(mesh);
                MeshBuilder::computeBoundaryPatch(mesh);
                test::testGeometry(mesh, config);
            }
        });
        reporter.passed();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MESH TESTS stopped: " << error.what() << '\n';
        return 1;
    }
}
