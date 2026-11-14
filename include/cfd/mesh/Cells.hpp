#pragma once // Avoid multiple inclusions of this header file
#include <cfd/utils/Constants.hpp>
#include <vector>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <cmath>

namespace cfd {
    
    class Cells {

    private:
        // Dimensions
        std::size_t Nx_ = 0, Ny_ = 0, Nz_ = 0; // Number of cells in each direction

        // Connectivity
        std::vector<std::array<std::size_t, 8>> Nodes_; // Nodes in each cell
        std::vector<std::array<std::size_t, 6>> Faces_; // Faces in each cell

        // Geometry
        std::vector<double> Volumes_; // Volume of each cell
        std::vector<double> CentersX_; // X coordinate of cell center
        std::vector<double> CentersY_; // Y coordinate of cell center
        std::vector<double> CentersZ_; // Z coordinate of cell center

    public:
        // ===================================================================================== //
        //                                 Adding cells                                          //
        // ===================================================================================== //
        void addByNodes(const std::array<std::size_t, 8>& nodes, std::size_t id) {
            if (id != Nodes_.size()) {
                throw std::invalid_argument("Cell ID must match the next sequential index");
            }
            Nodes_.push_back(nodes);

            Faces_.push_back({
                constants::INVALID_ID, constants::INVALID_ID, constants::INVALID_ID,
                constants::INVALID_ID, constants::INVALID_ID, constants::INVALID_ID
            });

            Volumes_.push_back(0);
            CentersX_.push_back(0);
            CentersY_.push_back(0);
            CentersZ_.push_back(0);
        }

        // ===================================================================================== //
        //                                     Setters                                           //
        // ===================================================================================== //
        void setFace(std::size_t cellId, std::size_t faceId, std::size_t localFaceId) {
            if (cellId >= Nodes_.size()) {
                throw std::out_of_range("Cell ID out of range");
            }
            if (localFaceId >= 6) {
                throw std::out_of_range("Local face ID out of range");
            }
            if (Faces_[cellId][localFaceId] != constants::INVALID_ID) {
                throw std::logic_error("Cell face already assigned");
            }
            Faces_[cellId][localFaceId] = faceId;
        }

        // Setters
        // Connectivity changes require rebuilding faces and recomputing geometry.
        void setNodes(std::size_t cellId, const std::array<std::size_t, 8>& nodes) {
            Nodes_.at(cellId) = nodes;
        }

        void setFaces(std::size_t cellId, const std::array<std::size_t, 6>& faces) {
            Faces_.at(cellId) = faces;
        }

        void setDimensions(std::size_t nx, std::size_t ny, std::size_t nz) {
            Nx_ = nx;
            Ny_ = ny;
            Nz_ = nz;
        }

        void setVolume(std::size_t cellId, double volume) {
            if (cellId >= Nodes_.size()) {
                throw std::out_of_range("Cell ID out of range");
            }
            if (!std::isfinite(volume) || volume <= 0.0) {
                throw std::runtime_error("Cell volume must be finite and positive");
            }
            Volumes_[cellId] = volume;
        }

        void setCenter(std::size_t cellId, std::array<double, 3> center) {
            if (cellId >= CentersX_.size()) {
                throw std::out_of_range("Cell ID out of range");
            }
            CentersX_[cellId] = center[0];
            CentersY_[cellId] = center[1];
            CentersZ_[cellId] = center[2];
        }

        // ===================================================================================== //
        //                                     Getters                                           //
        // ===================================================================================== //
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

        std::array<std::size_t, 8> getNodes(std::size_t cellId) const {
            if (cellId >= Nodes_.size()) {
                throw std::out_of_range("cellId out of range");
            }
            return Nodes_[cellId];
        }

        std::array<std::size_t, 6> getFaces(std::size_t cellId) const {
            if (cellId >= Faces_.size()) {
                throw std::out_of_range("cellId out of range");
            }
            return Faces_[cellId];
        }

        double getVolume(std::size_t cellId) const {
            return Volumes_.at(cellId);
        }

        std::array<double, 3> getCenter(std::size_t cellId) const {
            return {CentersX_.at(cellId), CentersY_.at(cellId), CentersZ_.at(cellId)};
        }

        // ===================================================================================== //
        //                                Id <-> Indices conversion                              //
        // ===================================================================================== //
        // Id -> Indices
        std::array<std::size_t, 3> getIndicesFromId(std::size_t id) const {
            if (id >= Nx_ * Ny_ * Nz_) {
                throw std::out_of_range("Cell ID out of range");
            }
            std::size_t i = id % Nx_;
            std::size_t j = (id / Nx_) % Ny_;
            std::size_t k = id / (Nx_ * Ny_);
            return {i, j, k};
        }
        // Indices -> Id
        std::size_t getIdFromIndices(std::size_t i, std::size_t j, std::size_t k) const {
            if (i >= Nx_ || j >= Ny_ || k >= Nz_) {
                throw std::out_of_range("Cell indices out of range");
            }
            return i + Nx_ * (j + Ny_ * k);
        }
    };
}