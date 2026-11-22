#include "TestUtils.hpp"
#include <meshio/MeshGenerator.hpp>

#include "TestGenerator.cpp"
#include "TestMeshConverter.cpp"

#include <exception>
#include <iostream>

int main() {

    test::SuiteReporter reporter("MESHIO TESTS");
    try {
        reporter.subgroup("Mesh generator");
        reporter.run("structured node coordinates, numbering and cell connectivity", [] {
            for (const MeshConfig config : {
                     MeshConfig{1.0, 1.0, 1.0, 1, 1, 1},
                     MeshConfig{2.0, 3.0, 4.0, 4, 3, 2}}) {
                const auto mesh = MeshGenerator::generate(config);
                test::testGenerator(mesh, config);
            }
        });
        reporter.subgroup("Mesh converter");
        reporter.run("mesh file write/read round trip including empty meshes",
                     test::testMeshConverter);
        reporter.passed();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MESHIO TESTS stopped: " << error.what() << '\n';
        return 1;
    }
}
