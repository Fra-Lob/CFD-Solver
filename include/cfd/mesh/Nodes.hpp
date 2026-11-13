#pragma once // Avoid multiple inclusions of this header file
#include <vector>
#include <array>
#include <cstddef>
#include <stdexcept>

namespace cfd {
    
    class Nodes {

    private:
        std::size_t Nx_, Ny_, Nz_; // Number of nodes in each direction
        std::vector<double> coordinatesX_; // X coordinate of each node
        std::vector<double> coordinatesY_; // Y coordinate of each node
        std::vector<double> coordinatesZ_; // Z coordinate of each node

    public:
        //Constructor
        void setDimensions(std::size_t nx, std::size_t ny, std::size_t nz) {
            Nx_ = nx;
            Ny_ = ny;
            Nz_ = nz;
        }

        void addCoordinates(double x, double y, double z) {
            coordinatesX_.push_back(x);
            coordinatesY_.push_back(y);
            coordinatesZ_.push_back(z);
        }

        // Ids <-> Indices conversion
        std::size_t getIdFromIndices(std::size_t i, std::size_t j, std::size_t k) const {
            return i + (Nx_) * (j + (Ny_) * k);
        };

        std::array<std::size_t, 3> getIndicesFromId(std::size_t id) const {
            std::size_t i = id % Nx_;
            std::size_t j = (id / Nx_) % Ny_;
            std::size_t k = id / (Nx_ * Ny_);
            return {i, j, k};
        }
            
        // Getters
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
    };
}