#pragma once // Avoid multiple inclusions of this header file
#include <vector>
#include <array>
#include <cstddef>
#include <stdexcept>

namespace cfd {
    
    class Nodes {

    private:
        // Dimensions
        std::size_t Nx_ = 0, Ny_ = 0, Nz_ = 0; // Number of nodes in each direction

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
            Nx_ = nx;
            Ny_ = ny;
            Nz_ = nz;
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
            return Nx_;
        }

        std::size_t getNy() const {
            return Ny_;
        }

        std::size_t getNz() const {
            return Nz_;
        }

        std::array<double, 3> getCoordinates(std::size_t index) const {
            if (index >= coordinatesX_.size()) {
                throw std::out_of_range("Node index out of range");
            }
            return {coordinatesX_[index], coordinatesY_[index], coordinatesZ_[index]};
        }

        // ===================================================================================== //
        //                            Id <-> Indices conversion                                  //
        // ===================================================================================== //
        // Id -> Indices
        std::array<std::size_t, 3> getIndicesFromId(std::size_t id) const {
            if (id >= Nx_ * Ny_ * Nz_) {
                throw std::out_of_range("Node ID out of range");
            }
            std::size_t i = id % Nx_;
            std::size_t j = (id / Nx_) % Ny_;
            std::size_t k = id / (Nx_ * Ny_);
            return {i, j, k};
        }

        // Indices -> Id
        std::size_t getIdFromIndices(std::size_t i, std::size_t j, std::size_t k) const {
            if (i >= Nx_ || j >= Ny_ || k >= Nz_) {
                throw std::out_of_range("Node indices out of range");
            }
            return i + Nx_ * (j + Ny_ * k);
        }
    };
}