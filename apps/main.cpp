
#include <cfd/mesh/StructuredMesh.hpp>
#include <iostream>
#include <cassert>

int main()
{
    cfd::StructuredMesh mesh(4, 3, 2);

    // Test mesh dimensions
    assert(mesh.getNx() == 4);
    assert(mesh.getNy() == 3);
    assert(mesh.getNz() == 2);
    assert(mesh.getTotalCells() == 24);

    // Test 3D -> 1D indexing
    assert(mesh.getCellIndex(2, 1, 1) == 18);

    // Test 1D -> 3D indexing
    auto indices = mesh.getCellIndices(18);

    assert(indices[0] == 2);
    assert(indices[1] == 1);
    assert(indices[2] == 1);

    std::cout << "All tests passed!" << '\n';

    return 0;
}