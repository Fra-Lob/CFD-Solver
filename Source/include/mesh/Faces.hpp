#pragma once 

#include <utils/Constants.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

class Faces {

private:
    // Connectivity
    std::vector<std::array<std::size_t, 4>> nodes_; // Nodes in each face
    std::vector<std::size_t> owners_; // Owner cell for each face
    std::vector<std::size_t> neighbors_; // Neighbor cell for each face

    // Geometry
    std::vector<double> areas_; // Area of each face
    std::vector<double> areasX_; // X component of face area
    std::vector<double> areasY_; // Y component of face area
    std::vector<double> areasZ_; // Z component of face area
    std::vector<double> centersX_; // X coordinate of face center
    std::vector<double> centersY_; // Y coordinate of face center
    std::vector<double> centersZ_; // Z coordinate of face center

    // Boundary
    std::vector<std::size_t> boundaryPatchIds_; // Boundary patch ID for each face

public:
    // ===================================================================================== //
    //                                 Adding faces                                          //
    // ===================================================================================== //
    void addByNodes(const std::array<std::size_t, 4>& nodes, 
                    std::size_t owner, 
                    std::size_t neighbor = constants::invalidId,
                    std::size_t patchId = constants::invalidId) {
        if (owner == constants::invalidId || owner == neighbor) {
            throw std::invalid_argument("Face must have a valid owner distinct from neighbor");
        }
        if (neighbor != constants::invalidId && patchId != constants::invalidId) {
            throw std::invalid_argument("Internal face cannot have a boundary patch");
        }
        nodes_.push_back(nodes);
        owners_.push_back(owner);
        neighbors_.push_back(neighbor);
        boundaryPatchIds_.push_back(patchId);
        centersX_.push_back(0);
        centersY_.push_back(0);
        centersZ_.push_back(0);
        areas_.push_back(0);
        areasX_.push_back(0);
        areasY_.push_back(0);
        areasZ_.push_back(0);
    }

    // ===================================================================================== //
    //                                      Setters                                          //
    // ===================================================================================== //
    // Connectivity changes require updating cell adjacency and recomputing geometry.
    void setNodes(std::size_t faceId, const std::array<std::size_t, 4>& nodes) {
        nodes_.at(faceId) = nodes;
    }

    void setOwner(std::size_t faceId, std::size_t owner) {
        if (faceId >= owners_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        if (owner == constants::invalidId || owner == neighbors_[faceId]) {
            throw std::invalid_argument("Face must have a valid owner distinct from neighbor");
        }
        owners_[faceId] = owner;
    }

    void setNeighbor(std::size_t faceId, std::size_t neighbor) {
        if (faceId >= neighbors_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        if (owners_[faceId] == neighbor) {
            throw std::invalid_argument("Neighbor cannot be the same as owner");
        }
        if (neighbors_[faceId] != constants::invalidId) {
            throw std::logic_error("Neighbor already set for this face");
        }
        if (neighbor == constants::invalidId) {
            throw std::invalid_argument("Neighbor must be a valid cell ID");
        }
        neighbors_[faceId] = neighbor;
        boundaryPatchIds_[faceId] = constants::invalidId;
    }

    // Geometry
    void setArea(std::size_t faceId, double area) {
        if (faceId >= areas_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        if (!std::isfinite(area) || area <= 0.0) {
            throw std::runtime_error("Face area must be finite and positive");
        }
        areas_[faceId] = area;
    }

    void setAreaVector(std::size_t faceId, std::array<double, 3> areaVector) {
        if (faceId >= areasX_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        areasX_[faceId] = areaVector[0];
        areasY_[faceId] = areaVector[1];
        areasZ_[faceId] = areaVector[2];
    }

    void setCenter(std::size_t faceId, std::array<double, 3> center) {
        if (faceId >= centersX_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        centersX_[faceId] = center[0];
        centersY_[faceId] = center[1];
        centersZ_[faceId] = center[2];
    }

    // Boundary
    void setBoundaryPatchId(std::size_t faceId, std::size_t patchId) {
        if (!isBoundary(faceId) && patchId != constants::invalidId) {
            throw std::invalid_argument("Internal face cannot have a boundary patch");
        }
        boundaryPatchIds_[faceId] = patchId;
    }

    // ===================================================================================== //
    //                                      Getters                                          //
    // ===================================================================================== //
    // Connectivity
    std::size_t getNumber() const {
        return nodes_.size();
    }

    std::array<std::size_t, 4> getNodes(std::size_t faceId) const {
        if (faceId >= nodes_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        return nodes_[faceId];
    }

    std::size_t getOwner(std::size_t faceId) const {
        if (faceId >= nodes_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        return owners_[faceId];
    }

    std::size_t getNeighbor(std::size_t faceId) const {
        if (faceId >= nodes_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        return neighbors_[faceId];
    }

    // Geometry
    double getArea(std::size_t faceId) const {
        return areas_.at(faceId);
    }

    std::array<double, 3> getAreaVector(std::size_t faceId) const {
        return {areasX_.at(faceId), areasY_.at(faceId), areasZ_.at(faceId)};
    }

    std::array<double, 3> getCenter(std::size_t faceId) const {
        return {centersX_.at(faceId), centersY_.at(faceId), centersZ_.at(faceId)};
    }

    // Boundary
    std::size_t getBoundaryPatchId(std::size_t faceId) const {
        if (faceId >= nodes_.size()) {
            throw std::out_of_range("Face ID out of range");
        }
        return boundaryPatchIds_[faceId];
    }

    bool isBoundary(std::size_t faceId) const {  // Boundary check
        if (faceId >= nodes_.size()) {
            throw std::out_of_range("Face ID out of range");
        }

        return neighbors_[faceId] == constants::invalidId;
    }
};