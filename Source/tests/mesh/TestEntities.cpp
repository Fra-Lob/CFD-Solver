#include <cfd/mesh/Mesh.hpp>

namespace test {
namespace {

void testNodesConstructor()
{
    Mesh mesh;
    require(mesh.nodes.getNumber() == 0, "Empty mesh starts without nodes");
}

void testNodeAddition()
{
    Mesh mesh;
    mesh.nodes.addCoordinates(0.0, 0.0, 0.0);
    mesh.nodes.addCoordinates(1.0, 1.0, 1.0);
    mesh.nodes.addCoordinates(1.0, 0.0, 1.0);
    require(mesh.nodes.getNumber() == 3, "Node count after addition");
    require(mesh.nodes.getCoordinates(0) == std::array<double, 3>{0.0, 0.0, 0.0},
            "First node coordinates");
    require(mesh.nodes.getCoordinates(1) == std::array<double, 3>{1.0, 1.0, 1.0},
            "Second node coordinates");
    require(mesh.nodes.getCoordinates(2) == std::array<double, 3>{1.0, 0.0, 1.0},
            "Third node coordinates");
}

void testInvalidNodeIndex()
{
    Mesh mesh;
    mesh.nodes.addCoordinates(0.0, 0.0, 0.0);
    bool threw = false;
    try {
        (void)mesh.nodes.getCoordinates(1);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "Out-of-range node access throws");
}

void testFacesConstructor()
{
    Mesh mesh;
    require(mesh.faces.getNumber() == 0, "Empty mesh starts without faces");
}

void testFaceAddition()
{
    Mesh mesh;
    mesh.faces.addByNodes({0, 1, 2, 3}, 0, 1);
    mesh.faces.addByNodes({4, 5, 6, 7}, 1);
    require(mesh.faces.getNumber() == 2, "Face count after addition");
    require(mesh.faces.getNodes(0) == std::array<std::size_t, 4>{0, 1, 2, 3},
            "First face nodes");
    require(mesh.faces.getOwner(0) == 0, "First face owner");
    require(mesh.faces.getNeighbor(0) == 1, "First face neighbor");
    require(mesh.faces.getNodes(1) == std::array<std::size_t, 4>{4, 5, 6, 7},
            "Second face nodes");
    require(mesh.faces.getOwner(1) == 1, "Second face owner");
    require(mesh.faces.getNeighbor(1) == constants::invalidId, "Boundary face neighbor");
}

void testInvalidFaceIndex()
{
    Mesh mesh;
    mesh.faces.addByNodes({0, 1, 2, 3}, 0, 1);
    bool threw = false;
    try {
        (void)mesh.faces.getNodes(1);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "Out-of-range face access throws");
}

void testBoundaryCheck()
{
    Mesh mesh;
    mesh.faces.addByNodes({0, 1, 2, 3}, 0, 1);
    mesh.faces.addByNodes({4, 5, 6, 7}, 1);
    require(!mesh.faces.isBoundary(0), "Face with a neighbor is internal");
    require(mesh.faces.isBoundary(1), "Face without a neighbor is boundary");
}

} // namespace

void testEntities(SuiteReporter& reporter)
{
    reporter.subgroup("Mesh entities");
    reporter.run("default node collection is empty", testNodesConstructor);
    reporter.run("node addition stores coordinates", testNodeAddition);
    reporter.run("invalid node access is rejected", testInvalidNodeIndex);
    reporter.run("default face collection is empty", testFacesConstructor);
    reporter.run("face addition stores connectivity", testFaceAddition);
    reporter.run("invalid face access is rejected", testInvalidFaceIndex);
    reporter.run("boundary detection uses neighbor presence", testBoundaryCheck);
}

}
