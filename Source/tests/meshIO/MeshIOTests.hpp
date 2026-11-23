#include "TestUtils.hpp"
#include <meshio/MeshGenerator.hpp>

#include "TestGenerator.cpp"
#include "TestMeshConverter.cpp"
#include "TestCaseReader.cpp"
#include "TestCSVExporter.cpp"

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
        reporter.subgroup("Case reader");
        reporter.run("key-value parsing, scalar fields and boundary conditions",
                     test::testCaseReader);
        reporter.subgroup("CSV exporter");
        reporter.run("uniform line sampling, scalar interpolation and Dirichlet/Neumann faces",
                     test::testCSVExporter);
        reporter.passed();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MESHIO TESTS stopped: " << error.what() << '\n';
        return 1;
    }
}
