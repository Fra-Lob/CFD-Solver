#pragma once // Avoid multiple inclusions of this header file
#include <vector>
#include <array>
#include <cstddef>
#include <stdexcept>

namespace cfd {
    
    class Cells {

    private:
        std::vector<std::array<std::size_t, 8>> Nodes_; // Nodes in each cell
        std::vector<std::array<std::size_t, 6>> Faces_; // Faces in each cell
        std::vector<double> Volumes_; // Volume of each cell
        std::vector<double> CenterX_; // X coordinate of cell center
        std::vector<double> CenterY_; // Y coordinate of cell center
        std::vector<double> CenterZ_; // Z coordinate of cell center

    public:
        // Adding cells
        void addByNodes(const std::array<std::size_t, 8>& nodes) {
            Nodes_.push_back(nodes);
        }

        // Getters
        std::size_t getCellsNumber() const {
            return Nodes_.size();
        }

        std::array<std::size_t, 8> getCellNodes(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Cell index out of range");
            }
            return Nodes_[index];
        }
    };
}