#pragma once // Avoid multiple inclusions of this header file
#include <vector>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <limits>

namespace cfd {
    static constexpr std::size_t INVALID_ID =
        std::numeric_limits<std::size_t>::max();
    
    class Faces {

    private:
        // Connectivity
        std::vector<std::array<std::size_t, 4>> Nodes_; // Nodes in each face
        std::vector<std::size_t> Owner_; // Owner cell for each face
        std::vector<std::size_t> Neighbor_; // Neighbor cell for each face

        // Geometry
        std::vector<double> CenterX_; // X coordinate of face center
        std::vector<double> CenterY_; // Y coordinate of face center
        std::vector<double> CenterZ_; // Z coordinate of face center
        std::vector<double> Area_; // Area of each face
        std::vector<double> AreaX_; // X component of face area
        std::vector<double> AreaY_; // Y component of face area
        std::vector<double> AreaZ_; // Z component of face area

        // Boundary
        std::vector<std::size_t> BoundaryPatchId_; // Boundary type for each face

    public:
        // Adding faces
        void addFace(const std::array<std::size_t, 4>& nodes, 
                        std::size_t owner, 
                        std::size_t neighbor = INVALID_ID,
                        std::size_t patchId = INVALID_ID) {
            Nodes_.push_back(nodes);
            Owner_.push_back(owner);
            Neighbor_.push_back(neighbor);
            BoundaryPatchId_.push_back(patchId);
        }

        // Getters
        std::size_t getFacesNumber() const {
            return Nodes_.size();
        }

        std::array<std::size_t, 4> getFaceNodes(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return Nodes_[index];
        }

        std::size_t getOwner(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return Owner_[index];
        }

        std::size_t getNeighbor(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return Neighbor_[index];
        }

        std::size_t getBoundaryPatchId(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return BoundaryPatchId_[index];
        }

        // Boundary check
        bool isBoundaryFace(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }

            return Neighbor_[index] == INVALID_ID;
        }
    };
}