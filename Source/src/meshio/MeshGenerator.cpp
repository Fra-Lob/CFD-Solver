#include <meshio/MeshGenerator.hpp>

#include <array>
#include <cmath>
#include <stdexcept>

Mesh MeshGenerator::generate(const MeshConfig& config) {

    Mesh mesh;

    // Validate input parameters
    if (config.nx == 0 || config.ny == 0 || config.nz == 0) {
        throw std::invalid_argument("Number of cells in each direction must be greater than zero.");
    }
    if (!std::isfinite(config.lx) || !std::isfinite(config.ly) || !std::isfinite(config.lz) ||
        config.lx <= 0.0 || config.ly <= 0.0 || config.lz <= 0.0) {
        throw std::invalid_argument("Domain lengths must be finite and positive.");
    }

    // Set the number of cells and nodes in the mesh
    mesh.setDomain(config.lx, config.ly, config.lz);
    mesh.cells.setDimensions(config.nx, config.ny, config.nz);
    mesh.nodes.setDimensions(config.nx + 1, config.ny + 1, config.nz + 1);

    // Create dx, dy, dz
    double dx = config.lx / config.nx;
    double dy = config.ly / config.ny;
    double dz = config.lz / config.nz;

    // Generate nodes
    for (std::size_t k = 0; k <= config.nz; ++k) {
        for (std::size_t j = 0; j <= config.ny; ++j) {
            for (std::size_t i = 0; i <= config.nx; ++i) {
                mesh.nodes.addCoordinates(i * dx, j * dy, k * dz);
            }
        }
    }

    // Generate cells 
    for (std::size_t k = 0; k < config.nz; ++k) {
        for (std::size_t j = 0; j < config.ny; ++j) {
            for (std::size_t i = 0; i < config.nx; ++i) {
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