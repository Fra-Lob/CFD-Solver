#include <cfd/mesh/StructuredMesh.hpp>
#include <iostream>
#include <cassert>
#include <stdexcept>

void testDimensions() {
    cfd::StructuredMesh mesh(4, 3, 2);

    // Test mesh dimensions
    assert(mesh.getNx() == 4);
    assert(mesh.getNy() == 3);
    assert(mesh.getNz() == 2);
    assert(mesh.getTotalCells() == 24);
}

void testIndexConversion() {
    cfd::StructuredMesh mesh(4, 3, 2);

    // Test 3D <-> 1D indexing
    int pair_check = -1;
    for (int k = 0; k < mesh.getNz(); ++k) {
        for (int j = 0; j < mesh.getNy(); ++j) {
            for (int i = 0; i < mesh.getNx(); ++i) {
                int index = mesh.getCellIndex(i, j, k);
                auto indices = mesh.getCellIndices(index);
                assert(indices[0] == i);
                assert(indices[1] == j);
                assert(indices[2] == k);

                assert(index != pair_check);
                pair_check = index;
            }
        }
    }
}

void testInvalidDimensions() {
    try {
        cfd::StructuredMesh mesh(-1, 3, 2);
        assert(false); // Should not reach here
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception for invalid dimensions: " << e.what() << '\n';
    }
}

void testInvalidIndices() {
    cfd::StructuredMesh mesh(4, 3, 2);

    try {
        mesh.getCellIndex(4, 0, 0); // Invalid i index
        assert(false); // Should not reach here
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception for invalid cell index: " << e.what() << '\n';
    }

    try {
        mesh.getCellIndices(24); // Invalid 1D index
        assert(false); // Should not reach here
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception for invalid cell indices: " << e.what() << '\n';
    }
}

int main()
{
    testDimensions();
    testIndexConversion();
    testInvalidDimensions();
    testInvalidIndices();

    std::cout << "All StructuredMesh tests passed!\n";
    return 0;
}