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
        std::vector<std::size_t> Owners_; // Owner cell for each face
        std::vector<std::size_t> Neighbors_; // Neighbor cell for each face

        // Geometry
        std::vector<double> CentersX_; // X coordinate of face center
        std::vector<double> CentersY_; // Y coordinate of face center
        std::vector<double> CentersZ_; // Z coordinate of face center
        std::vector<double> Areas_; // Area of each face
        std::vector<double> AreasX_; // X component of face area
        std::vector<double> AreasY_; // Y component of face area
        std::vector<double> AreasZ_; // Z component of face area

        // Boundary
        std::vector<std::size_t> BoundaryPatchIds_; // Boundary type for each face

    public:
        // Adding faces
        void addByNodes(const std::array<std::size_t, 4>& nodes, 
                        std::size_t owner, 
                        std::size_t neighbor = INVALID_ID,
                        std::size_t patchId = INVALID_ID) {
            Nodes_.push_back(nodes);
            Owners_.push_back(owner);
            Neighbors_.push_back(neighbor);
            BoundaryPatchIds_.push_back(patchId);
        }

        void setNeighbor(std::size_t faceId, std::size_t neighbor) {
            if (faceId >= Neighbors_.size()) {
                throw std::out_of_range("Face ID out of range");
            }
            if (Owners_[faceId] == neighbor) {
                throw std::invalid_argument("Neighbor cannot be the same as owner");
            }
            if (Neighbors_[faceId] != INVALID_ID) {
                throw std::logic_error("Neighbor already set for this face");
            }
            Neighbors_[faceId] = neighbor;
        }

        // Getters
        std::size_t getNumber() const {
            return Nodes_.size();
        }

        std::array<std::size_t, 4> getNodes(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return Nodes_[index];
        }

        std::size_t getOwner(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return Owners_[index];
        }

        std::size_t getNeighbor(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return Neighbors_[index];
        }

        std::size_t getBoundaryPatchId(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return BoundaryPatchIds_[index];
        }

        // Boundary check
        bool isBoundary(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }

            return Neighbors_[index] == INVALID_ID;
        }
    };
}