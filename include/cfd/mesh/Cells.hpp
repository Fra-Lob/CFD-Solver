#pragma once // Avoid multiple inclusions of this header file
#include <vector>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <limits>

namespace cfd {
    
    class Cells {

    private:
        std::size_t Nx_, Ny_, Nz_; // Number of cells in each direction
        std::vector<std::array<std::size_t, 8>> Nodes_; // Nodes in each cell
        std::vector<std::array<std::size_t, 6>> Faces_; // Faces in each cell
        std::vector<double> Volumes_; // Volume of each cell
        std::vector<double> CentersX_; // X coordinate of cell center
        std::vector<double> CentersY_; // Y coordinate of cell center
        std::vector<double> CentersZ_; // Z coordinate of cell center

    public:
        // Adding cells
        void addByNodes(const std::array<std::size_t, 8>& nodes, std::size_t id) {
            Nodes_.push_back(nodes);

            Faces_.push_back({
                INVALID_ID, INVALID_ID, INVALID_ID,
                INVALID_ID, INVALID_ID, INVALID_ID
            });
        }

        // Adding face to cells
        void setFace(std::size_t cellId, std::size_t faceId, std::size_t localFaceId) {
            if (cellId >= Nodes_.size()) {
                throw std::out_of_range("Cell ID out of range");
            }
            if (localFaceId >= 6) {
                throw std::out_of_range("Local face ID out of range");
            }
            if (Faces_[cellId][localFaceId] != INVALID_ID) {
                throw std::logic_error("Cell face already assigned");
            }
            Faces_[cellId][localFaceId] = faceId;
        }

        // Set the number of cells in each direction
        void setDimensions(std::size_t nx, std::size_t ny, std::size_t nz) {
            Nx_ = nx;
            Ny_ = ny;
            Nz_ = nz;
        }

        // Ids <-> Indices conversion
        std::array<std::size_t, 3> getIndicesFromId(std::size_t id) const {
            if (id >= Nx_ * Ny_ * Nz_) {
                throw std::out_of_range("Cell ID out of range");
            }
            std::size_t i = id % Nx_;
            std::size_t j = (id / Nx_) % Ny_;
            std::size_t k = id / (Nx_ * Ny_);
            return {i, j, k};
        }

        std::size_t getIdFromIndices(std::size_t i, std::size_t j, std::size_t k) const {
            if (i >= Nx_ || j >= Ny_ || k >= Nz_) {
                throw std::out_of_range("Cell indices out of range");
            }
            return i + Nx_ * (j + Ny_ * k);
        }

        // Getters
        std::size_t getNumber() const {
            return Nodes_.size();
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

        std::array<std::size_t, 8> getNodes(std::size_t index) const {
            if (index >= Nodes_.size()) {
                throw std::out_of_range("Cell index out of range");
            }
            return Nodes_[index];
        }

        std::array<std::size_t, 6> getFaces(std::size_t index) const {
            if (index >= Faces_.size()) {
                throw std::out_of_range("Cell index out of range");
            }
            return Faces_[index];
        }
    };


}