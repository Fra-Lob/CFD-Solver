#include <cfd/meshio/MeshGenerator.hpp>
#include <cassert>
#include <iostream>

void testMeshGenerator() {
    // Test case implementation
    cfd::MeshConfig config{1.0, 1.0, 1.0, 2, 2, 2}; // A simple 2x2x2 mesh
    cfd::Mesh mesh = cfd::MeshGenerator::generate(config);
    assert(mesh.nodes.getNumber() == 27); // 3x3x3 nodes

    // Check node coordinates
    auto coords = mesh.nodes.getCoordinates(1);
    assert(coords[0] == 0.5 && coords[1] == 0.0 && coords[2] == 0.0);

    // Check cell count
    assert(mesh.cells.getNumber() == 8); // 2x2x2

    // Check cell IDs and their corresponding indices
    for (std::size_t id = 0; id < mesh.cells.getNumber(); ++id) {
        auto indices = mesh.cells.getIndicesFromId(id);
        std::size_t computedId = mesh.cells.getIdFromIndices(indices[0], indices[1], indices[2]);
        assert(computedId == id);   
    }

    // Check node IDs and their corresponding indices
    for (std::size_t id = 0; id < mesh.nodes.getNumber(); ++id) {
        auto indices = mesh.nodes.getIndicesFromId(id);
        std::size_t computedId = mesh.nodes.getIdFromIndices(indices[0], indices[1], indices[2]);
        assert(computedId == id);
    }

    // Check that the cells have the right nodes
    std::array<std::size_t, 8> cellNodes = mesh.cells.getNodes(0);
    std::array<std::size_t, 8> expectedNodes = {0, 1, 4, 3, 9, 10, 13, 12};
    assert(cellNodes == expectedNodes);
}

int main() {
    testMeshGenerator();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}