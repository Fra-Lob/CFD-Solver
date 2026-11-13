#include <cfd/meshio/MeshGenerator.hpp>
#include <stdexcept>
#include <cmath>

namespace cfd {

    Mesh MeshGenerator::generate(const MeshConfig& config) {

        Mesh mesh;

        // Validate input parameters
        if (config.Nx == 0 || config.Ny == 0 || config.Nz == 0) {
            throw std::invalid_argument("Number of cells in each direction must be greater than zero.");
        }
        if (config.Lx <= 0.0 || config.Ly <= 0.0 || config.Lz <= 0.0) {
            throw std::invalid_argument("Domain lengths must be positive.");
        }
        if (!std::isfinite(config.Nx) || !std::isfinite(config.Ny) || !std::isfinite(config.Nz)) {
            throw std::invalid_argument("Number of cells must be finite.");
        }

        // Set the number of cells and nodes in the mesh
        mesh.cells.setDimensions(config.Nx, config.Ny, config.Nz);
        mesh.nodes.setDimensions(config.Nx + 1, config.Ny + 1, config.Nz + 1);

        // Create dx, dy, dz
        double dx = config.Lx / config.Nx;
        double dy = config.Ly / config.Ny;
        double dz = config.Lz / config.Nz;

        // Generate nodes
        for (std::size_t k = 0; k <= config.Nz; ++k) {
            for (std::size_t j = 0; j <= config.Ny; ++j) {
                for (std::size_t i = 0; i <= config.Nx; ++i) {
                    std::size_t nodeId = i + (config.Nx + 1) * (j + (config.Ny + 1) * k);
                    mesh.nodes.addCoordinates(i * dx, j * dy, k * dz);
                }
            }
        }

        // Generate cells 
        for (std::size_t k = 0; k < config.Nz; ++k) {
            for (std::size_t j = 0; j < config.Ny; ++j) {
                for (std::size_t i = 0; i < config.Nx; ++i) {
                    std::array<std::size_t, 8> cellNodes = {
                        mesh.nodes.getIdFromIndices(i, j, k),
                        mesh.nodes.getIdFromIndices(i + 1, j, k),
                        mesh.nodes.getIdFromIndices(i + 1, j + 1, k),
                        mesh.nodes.getIdFromIndices(i, j + 1, k),
                        mesh.nodes.getIdFromIndices(i, j, k + 1),
                        mesh.nodes.getIdFromIndices(i + 1, j, k + 1),
                        mesh.nodes.getIdFromIndices(i + 1, j + 1, k + 1),
                        mesh.nodes.getIdFromIndices(i, j + 1, k + 1)
                    };
                    std::size_t cellId = mesh.cells.getIdFromIndices(i, j, k);
                    mesh.cells.addByNodes(cellNodes, cellId);
                }
            }
        }

        return mesh;
    }

}