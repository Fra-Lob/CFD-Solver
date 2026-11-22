#pragma once // Avoid multiple inclusions of this header file
#include <cfd/utils/Constants.hpp>
#include <vector>
#include <array>
#include <stdexcept>
#include <cmath>
  
class Cells {

private:
    // Dimensions
    std::size_t nx_ = 0, ny_ = 0, nz_ = 0; // Number of cells in each direction

    // Connectivity
    std::vector<std::array<std::size_t, 8>> nodes_; // Nodes in each cell
    std::vector<std::array<std::size_t, 6>> faces_; // Faces in each cell

    // Geometry
    std::vector<double> volumes_; // Volume of each cell
    std::vector<double> centersX_; // X coordinate of cell center
    std::vector<double> centersY_; // Y coordinate of cell center
    std::vector<double> centersZ_; // Z coordinate of cell center

public:
    // ===================================================================================== //
    //                                 Adding cells                                          //
    // ===================================================================================== //
    void addByNodes(const std::array<std::size_t, 8>& nodes, std::size_t id) {
        if (id != nodes_.size()) {
            throw std::invalid_argument("Cell ID must match the next sequential index");
        }
        nodes_.push_back(nodes);

        faces_.push_back({
            constants::invalidId, constants::invalidId, constants::invalidId,
            constants::invalidId, constants::invalidId, constants::invalidId
        });

        volumes_.push_back(0);
        centersX_.push_back(0);
        centersY_.push_back(0);
        centersZ_.push_back(0);
    }

    // ===================================================================================== //
    //                                     Setters                                           //
    // ===================================================================================== //
    void setFace(std::size_t cellId, std::size_t faceId, std::size_t localFaceId) {
        if (cellId >= nodes_.size()) {
            throw std::out_of_range("Cell ID out of range");
        }
        if (localFaceId >= 6) {
            throw std::out_of_range("Local face ID out of range");
        }
        if (faces_[cellId][localFaceId] != constants::invalidId) {
            throw std::logic_error("Cell face already assigned");
        }
        faces_[cellId][localFaceId] = faceId;
    }

    // Connectivity changes require rebuilding faces and recomputing geometry.
    void setNodes(std::size_t cellId, const std::array<std::size_t, 8>& nodes) {
        nodes_.at(cellId) = nodes;
    }

    void setFaces(std::size_t cellId, const std::array<std::size_t, 6>& faces) {
        faces_.at(cellId) = faces;
    }

    void setDimensions(std::size_t nx, std::size_t ny, std::size_t nz) {
        nx_ = nx;
        ny_ = ny;
        nz_ = nz;
    }

    void setVolume(std::size_t cellId, double volume) {
        if (cellId >= nodes_.size()) {
            throw std::out_of_range("Cell ID out of range");
        }
        if (!std::isfinite(volume) || volume <= 0.0) {
            throw std::runtime_error("Cell volume must be finite and positive");
        }
        volumes_[cellId] = volume;
    }

    void setCenter(std::size_t cellId, std::array<double, 3> center) {
        if (cellId >= centersX_.size()) {
            throw std::out_of_range("Cell ID out of range");
        }
        centersX_[cellId] = center[0];
        centersY_[cellId] = center[1];
        centersZ_[cellId] = center[2];
    }

    // ===================================================================================== //
    //                                     Getters                                           //
    // ===================================================================================== //
    std::size_t getNumber() const {
        return nodes_.size();
    }

    std::size_t getNx() const {
        return nx_;
    }

    std::size_t getNy() const {
        return ny_;
    }

    std::size_t getNz() const {
        return nz_;
    }

    std::array<std::size_t, 8> getNodes(std::size_t cellId) const {
        if (cellId >= nodes_.size()) {
            throw std::out_of_range("cellId out of range");
        }
        return nodes_[cellId];
    }

    std::array<std::size_t, 6> getFaces(std::size_t cellId) const {
        if (cellId >= faces_.size()) {
            throw std::out_of_range("cellId out of range");
        }
        return faces_[cellId];
    }

    double getVolume(std::size_t cellId) const {
        return volumes_.at(cellId);
    }

    std::array<double, 3> getCenter(std::size_t cellId) const {
        return {centersX_.at(cellId), centersY_.at(cellId), centersZ_.at(cellId)};
    }

    // ===================================================================================== //
    //                                Id <-> Indices conversion                              //
    // ===================================================================================== //
    // Id -> Indices
    std::array<std::size_t, 3> getIndicesFromId(std::size_t id) const {
        if (id >= nx_ * ny_ * nz_) {
            throw std::out_of_range("Cell ID out of range");
        }
        std::size_t i = id % nx_;
        std::size_t j = (id / nx_) % ny_;
        std::size_t k = id / (nx_ * ny_);
        return {i, j, k};
    }
    // Indices -> Id
    std::size_t getIdFromIndices(std::size_t i, std::size_t j, std::size_t k) const {
        if (i >= nx_ || j >= ny_ || k >= nz_) {
            throw std::out_of_range("Cell indices out of range");
        }
        return i + nx_ * (j + ny_ * k);
    }
};