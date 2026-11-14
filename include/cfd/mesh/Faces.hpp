#pragma once // Avoid multiple inclusions of this header file
#include <cfd/utils/Constants.hpp>
#include <vector>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <cmath>

namespace cfd {
    
    class Faces {

    private:
        // Connectivity
        std::vector<std::array<std::size_t, 4>> Nodes_; // Nodes in each face
        std::vector<std::size_t> Owners_; // Owner cell for each face
        std::vector<std::size_t> Neighbors_; // Neighbor cell for each face

        // Geometry
        std::vector<double> Areas_; // Area of each face
        std::vector<double> AreasX_; // X component of face area
        std::vector<double> AreasY_; // Y component of face area
        std::vector<double> AreasZ_; // Z component of face area
        std::vector<double> CentersX_; // X coordinate of face center
        std::vector<double> CentersY_; // Y coordinate of face center
        std::vector<double> CentersZ_; // Z coordinate of face center

        // Boundary
        std::vector<std::size_t> BoundaryPatchIds_; // Boundary patch ID for each face

    public:
        // ===================================================================================== //
        //                                 Adding faces                                          //
        // ===================================================================================== //
        void addByNodes(const std::array<std::size_t, 4>& nodes, 
                        std::size_t owner, 
                        std::size_t neighbor = constants::INVALID_ID,
                        std::size_t patchId = constants::INVALID_ID) {
            if (owner == constants::INVALID_ID || owner == neighbor) {
                throw std::invalid_argument("Face must have a valid owner distinct from neighbor");
            }
            if (neighbor != constants::INVALID_ID && patchId != constants::INVALID_ID) {
                throw std::invalid_argument("Internal face cannot have a boundary patch");
            }
            Nodes_.push_back(nodes);
            Owners_.push_back(owner);
            Neighbors_.push_back(neighbor);
            BoundaryPatchIds_.push_back(patchId);
            CentersX_.push_back(0);
            CentersY_.push_back(0);
            CentersZ_.push_back(0);
            Areas_.push_back(0);
            AreasX_.push_back(0);
            AreasY_.push_back(0);
            AreasZ_.push_back(0);
        }

        // ===================================================================================== //
        //                                      Setters                                          //
        // ===================================================================================== //
        // Connectivity changes require updating cell adjacency and recomputing geometry.
        void setNodes(std::size_t faceId, const std::array<std::size_t, 4>& nodes) {
            Nodes_.at(faceId) = nodes;
        }

        void setOwner(std::size_t faceId, std::size_t owner) {
            if (faceId >= Owners_.size()) {
                throw std::out_of_range("Face ID out of range");
            }
            if (owner == constants::INVALID_ID || owner == Neighbors_[faceId]) {
                throw std::invalid_argument("Face must have a valid owner distinct from neighbor");
            }
            Owners_[faceId] = owner;
        }

        void setNeighbor(std::size_t faceId, std::size_t neighbor) {
            if (faceId >= Neighbors_.size()) {
                throw std::out_of_range("Face ID out of range");
            }
            if (Owners_[faceId] == neighbor) {
                throw std::invalid_argument("Neighbor cannot be the same as owner");
            }
            if (Neighbors_[faceId] != constants::INVALID_ID) {
                throw std::logic_error("Neighbor already set for this face");
            }
            if (neighbor == constants::INVALID_ID) {
                throw std::invalid_argument("Neighbor must be a valid cell ID");
            }
            Neighbors_[faceId] = neighbor;
            BoundaryPatchIds_[faceId] = constants::INVALID_ID;
        }

        // Geometry
        void setArea(std::size_t faceId, double area) {
            if (faceId >= Areas_.size()) {
                throw std::out_of_range("Face ID out of range");
            }
            if (!std::isfinite(area) || area <= 0.0) {
                throw std::runtime_error("Face area must be finite and positive");
            }
            Areas_[faceId] = area;
        }

        void setAreaVector(std::size_t faceId, std::array<double, 3> areaVector) {
            if (faceId >= AreasX_.size()) {
                throw std::out_of_range("Face ID out of range");
            }
            AreasX_[faceId] = areaVector[0];
            AreasY_[faceId] = areaVector[1];
            AreasZ_[faceId] = areaVector[2];
        }

        void setCenter(std::size_t faceId, std::array<double, 3> center) {
            if (faceId >= CentersX_.size()) {
                throw std::out_of_range("Face ID out of range");
            }
            CentersX_[faceId] = center[0];
            CentersY_[faceId] = center[1];
            CentersZ_[faceId] = center[2];
        }

        // Boundary
        void setBoundaryPatchId(std::size_t faceId, std::size_t patchId) {
            if (!isBoundary(faceId) && patchId != constants::INVALID_ID) {
                throw std::invalid_argument("Internal face cannot have a boundary patch");
            }
            BoundaryPatchIds_[faceId] = patchId;
        }

        // ===================================================================================== //
        //                                      Getters                                          //
        // ===================================================================================== //
        // Connectivity
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

        // Geometry
        double getArea(std::size_t faceId) const {
            return Areas_.at(faceId);
        }

        std::array<double, 3> getAreaVector(std::size_t faceId) const {
            return {AreasX_.at(faceId), AreasY_.at(faceId), AreasZ_.at(faceId)};
        }

        std::array<double, 3> getCenter(std::size_t faceId) const {
            return {CentersX_.at(faceId), CentersY_.at(faceId), CentersZ_.at(faceId)};
        }

        // Boundary
        std::size_t getBoundaryPatchId(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }
            return BoundaryPatchIds_[index];
        }

        bool isBoundary(std::size_t index) const {  // Boundary check
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Face index out of range");
            }

            return Neighbors_[index] == constants::INVALID_ID;
        }
    };
}