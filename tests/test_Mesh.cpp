#include <cfd/mesh/Mesh.hpp>
#include <cassert>
#include <iostream>

// =================================================================================== //
// Test for Nodes
// =================================================================================== //

void testNodesConstructor() {
    cfd::Mesh mesh;
    assert(mesh.nodes.getNumber() == 0);
}

void testNodeAddition() {
    cfd::Mesh mesh;
    mesh.nodes.addCoordinates(0.0, 0.0, 0.0);
    mesh.nodes.addCoordinates(1.0, 1.0, 1.0);
    mesh.nodes.addCoordinates(1.0, 0.0, 1.0);
    assert(mesh.nodes.getNumber() == 3);
    assert((mesh.nodes.getCoordinates(0) == std::array<double, 3>{0.0, 0.0, 0.0}));
    assert((mesh.nodes.getCoordinates(1) == std::array<double, 3>{1.0, 1.0, 1.0}));
    assert((mesh.nodes.getCoordinates(2) == std::array<double, 3>{1.0, 0.0, 1.0}));
}

void testInvalidNodeIndex() {
    cfd::Mesh mesh;
    mesh.nodes.addCoordinates(0.0, 0.0, 0.0);
    try {
        mesh.nodes.getCoordinates(1); // Invalid index
        assert(false); // Should not reach here
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception for invalid node index: " << e.what() << '\n';
    }
}

// =================================================================================== //
// Test for Faces
// =================================================================================== //

void testFacesConstructor() {
    cfd::Mesh mesh;
    assert(mesh.faces.getNumber() == 0);
}

void testFaceAddition() {
    cfd::Mesh mesh;
    mesh.faces.addByNodes({0, 1, 2, 3}, 0, 1);
    mesh.faces.addByNodes({4, 5, 6, 7}, 1);
    assert(mesh.faces.getNumber() == 2);
    assert((mesh.faces.getNodes(0) == std::array<std::size_t, 4>{0, 1, 2, 3}));
    assert(mesh.faces.getOwner(0) == 0);
    assert(mesh.faces.getNeighbor(0) == 1);
    assert((mesh.faces.getNodes(1) == std::array<std::size_t, 4>{4, 5, 6, 7}));
    assert(mesh.faces.getOwner(1) == 1);
    assert(mesh.faces.getNeighbor(1) == cfd::constants::INVALID_ID); // Boundary face
}

void testInvalidFaceIndex() {
    cfd::Mesh mesh;
    mesh.faces.addByNodes({0, 1, 2, 3}, 0, 1);
    try {
        mesh.faces.getNodes(1); // Invalid index
        assert(false); // Should not reach here
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception for invalid face index: " << e.what() << '\n';
    }
}

void testBoundaryCheck() {
    cfd::Mesh mesh;
    mesh.faces.addByNodes({0, 1, 2, 3}, 0, 1);
    mesh.faces.addByNodes({4, 5, 6, 7}, 1); // Boundary face
    assert(!mesh.faces.isBoundary(0));
    assert(mesh.faces.isBoundary(1));
}


// =================================================================================== //
// Main Run
// =================================================================================== //

int main()
{
    testNodesConstructor();
    testNodeAddition();
    testInvalidNodeIndex();
    testFacesConstructor();
    testFaceAddition();
    testInvalidFaceIndex();
    testBoundaryCheck();

    std::cout << "All tests passed!\n";
    return 0;
}