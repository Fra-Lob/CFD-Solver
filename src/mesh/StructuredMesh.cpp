#include <cfd/mesh/StructuredMesh.hpp>
#include <stdexcept>

namespace cfd
{
    StructuredMesh::StructuredMesh(int nx, int ny, int nz): nx_(nx), ny_(ny), nz_(nz) {
        if (nx <= 0 || ny <= 0 || nz <= 0) {
            throw std::invalid_argument("Number of cells in each direction must be positive.");
        }
    }

    int StructuredMesh::getCellIndex(int i, int j, int k) const {
        if (i < 0 || i >= nx_ || 
            j < 0 || j >= ny_ || 
            k < 0 || k >= nz_) {
            throw std::out_of_range("Cell indices are out of bounds.");
        }
        return i + nx_ * (j + ny_ * k);
    }

    std::array<int, 3> StructuredMesh::getCellIndices(int index) const {
        if (index < 0 || index >= getTotalCells()) {
            throw std::out_of_range("Cell index is out of bounds.");
        }
        int k = index / (nx_ * ny_);
        int j = (index % (nx_ * ny_)) / nx_;
        int i = index % nx_;
        return {i, j, k};
    }
}