#include "TestUtils.hpp"

#include <mesh/MeshBuilder.hpp>
#include <meshio/MeshConverter.hpp>
#include <meshio/MeshGenerator.hpp>

#include <filesystem>
#include <stdexcept>

namespace {

    void compare(const Mesh& expected, const Mesh& actual) {
        test::require(expected.nodes.getNumber() == actual.nodes.getNumber(), "Mesh node count");
        test::require(expected.cells.getNumber() == actual.cells.getNumber(), "Mesh cell count");
        test::require(expected.faces.getNumber() == actual.faces.getNumber(), "Mesh face count");
        test::require(expected.getDomain() == actual.getDomain(), "Mesh domain");
        test::require(expected.nodes.getNx() == actual.nodes.getNx() &&
                    expected.nodes.getNy() == actual.nodes.getNy() &&
                    expected.nodes.getNz() == actual.nodes.getNz(), "Node dimensions");
        test::require(expected.cells.getNx() == actual.cells.getNx() &&
                    expected.cells.getNy() == actual.cells.getNy() &&
                    expected.cells.getNz() == actual.cells.getNz(), "Cell dimensions");

        for (std::size_t node = 0; node < expected.nodes.getNumber(); ++node) {
            test::require(expected.nodes.getCoordinates(node) == actual.nodes.getCoordinates(node),
                        "Node coordinates");
        }
        for (std::size_t cell = 0; cell < expected.cells.getNumber(); ++cell) {
            test::require(expected.cells.getNodes(cell) == actual.cells.getNodes(cell), "Cell nodes");
            test::require(expected.cells.getFaces(cell) == actual.cells.getFaces(cell), "Cell faces");
            test::require(test::approx(expected.cells.getVolume(cell), actual.cells.getVolume(cell)),
                        "Cell volume");
            for (std::size_t direction = 0; direction < 3; ++direction) {
                test::require(test::approx(expected.cells.getCenter(cell)[direction],
                                        actual.cells.getCenter(cell)[direction]), "Cell center");
            }
        }
        for (std::size_t face = 0; face < expected.faces.getNumber(); ++face) {
            test::require(expected.faces.getNodes(face) == actual.faces.getNodes(face), "Face nodes");
            test::require(expected.faces.getOwner(face) == actual.faces.getOwner(face), "Face owner");
            test::require(expected.faces.getNeighbor(face) == actual.faces.getNeighbor(face), "Face neighbor");
            test::require(expected.faces.getBoundaryPatchId(face) == actual.faces.getBoundaryPatchId(face),
                        "Boundary patch");
            test::require(test::approx(expected.faces.getArea(face), actual.faces.getArea(face)), "Face area");
            for (std::size_t direction = 0; direction < 3; ++direction) {
                test::require(test::approx(expected.faces.getCenter(face)[direction],
                                        actual.faces.getCenter(face)[direction]), "Face center");
                test::require(test::approx(expected.faces.getAreaVector(face)[direction],
                                        actual.faces.getAreaVector(face)[direction]), "Face area vector");
            }
        }
    }
} // namespace

namespace test {

    void testMeshConverter()
    {
        const std::string path = "mesh-converter-test.mesh";
        try {
            auto mesh = MeshGenerator::generate({2.0, 3.0, 4.0, 3, 2, 2});
            MeshBuilder::computeConnectivity(mesh);
            MeshBuilder::computeFullGeometry(mesh);
            MeshBuilder::computeBoundaryPatch(mesh);

            MeshConverter::write(mesh, path);
            const auto loaded = MeshConverter::read(path);
            compare(mesh, loaded);

            const Mesh empty;
            MeshConverter::write(empty, path);
            compare(empty, MeshConverter::read(path));

            std::filesystem::remove(path);
        } catch (const std::exception& error) {
            std::filesystem::remove(path);
            throw;
        }
    }
}
