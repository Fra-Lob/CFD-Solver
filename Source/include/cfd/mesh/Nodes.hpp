#pragma once // Avoid multiple inclusions of this header file
#include <vector>
#include <array>
#include <stdexcept>
   
class Nodes {

private:
    // Dimensions
    std::size_t nx_ = 0, ny_ = 0, nz_ = 0; // Number of nodes in each direction

    // Geometry
    std::vector<double> coordinatesX_; // X coordinate of each node
    std::vector<double> coordinatesY_; // Y coordinate of each node
    std::vector<double> coordinatesZ_; // Z coordinate of each node

public:
    // ===================================================================================== //
    //                                 Adding nodes                                          //
    // ===================================================================================== //
    void addCoordinates(double x, double y, double z) {
        coordinatesX_.push_back(x);
        coordinatesY_.push_back(y);
        coordinatesZ_.push_back(z);
    }

    // ===================================================================================== //
    //                                     Setters                                           //
    // ===================================================================================== //
    // Coordinate changes require recomputing face and cell geometry.
    void setDimensions(std::size_t nx, std::size_t ny, std::size_t nz) {
        nx_ = nx;
        ny_ = ny;
        nz_ = nz;
    }

    void setCoordinates(std::size_t nodeId, const std::array<double, 3>& coordinates) {
        if (nodeId >= coordinatesX_.size()) {
            throw std::out_of_range("Node ID out of range");
        }
        coordinatesX_[nodeId] = coordinates[0];
        coordinatesY_[nodeId] = coordinates[1];
        coordinatesZ_[nodeId] = coordinates[2];
    }
        
    // ===================================================================================== //
    //                                       Getters                                         //
    // ===================================================================================== //
    std::size_t getNumber() const {
        return coordinatesX_.size();
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

    std::array<double, 3> getCoordinates(std::size_t nodeId) const {
        if (nodeId >= getNumber()) {
            throw std::out_of_range("Node ID out of range");
        }
        return {coordinatesX_[nodeId], coordinatesY_[nodeId], coordinatesZ_[nodeId]};
    }

    // ===================================================================================== //
    //                            Id <-> Indices conversion                                  //
    // ===================================================================================== //
    // Id -> Indices
    std::array<std::size_t, 3> getIndicesFromId(std::size_t nodeId) const {
        if (nodeId >= nx_ * ny_ * nz_) {
            throw std::out_of_range("Node ID out of range");
        }
        std::size_t i = nodeId % nx_;
        std::size_t j = (nodeId / nx_) % ny_;
        std::size_t k = nodeId / (nx_ * ny_);
        return {i, j, k};
    }

    // Indices -> Id
    std::size_t getIdFromIndices(std::size_t i, std::size_t j, std::size_t k) const {
        if (i >= nx_ || j >= ny_ || k >= nz_) {
            throw std::out_of_range("Node indices out of range");
        }
        return i + nx_ * (j + ny_ * k);
    }
};