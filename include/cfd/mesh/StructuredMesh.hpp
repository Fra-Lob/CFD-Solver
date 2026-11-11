#pragma once // Avoid multiple inclusions of this header file
#include <array>

namespace cfd
{

    class StructuredMesh
    {
    private:
    // Number of cells in each direction
        int nx_;
        int ny_;
        int nz_; 

    public:
        // Constructor
        StructuredMesh(int nx, int ny, int nz);

        // Getters
        int getNx() const { return nx_; };
        int getNy() const { return ny_; };
        int getNz() const { return nz_; };
        int getTotalCells() const { return nx_ * ny_ * nz_; };

        // Convert index to coordinates and vice versa
        int getCellIndex(int i, int j, int k) const;
        std::array<int, 3> getCellIndices(int index) const;

        // Get cell neighbors

    };
}