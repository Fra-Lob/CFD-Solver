#include <cfd/meshio/MeshGenerator.hpp>

namespace cfd {

    Mesh MeshGenerator::generate(const MeshConfig& config) {

        Mesh mesh(config.type, config.dimension, config.domain);
        mesh.requireCartesian3D();

        // Validate input parameters
        if (config.Nx == 0 || config.Ny == 0 || config.Nz == 0) {
            throw std::invalid_argument("Number of cells in each direction must be greater than zero.");
        }

        // Set the number of cells and nodes in the mesh
        mesh.cells.setDimensions(config.Nx, config.Ny, config.Nz);
        mesh.nodes.setDimensions(config.Nx + 1, config.Ny + 1, config.Nz + 1);

        // Create dx, dy, dz
        double dx = mesh.getDomain()[0] / config.Nx;
        double dy = mesh.getDomain()[1] / config.Ny;
        double dz = mesh.getDomain()[2] / config.Nz;

        // Generate nodes
        for (std::size_t k = 0; k <= config.Nz; ++k) {
            for (std::size_t j = 0; j <= config.Ny; ++j) {
                for (std::size_t i = 0; i <= config.Nx; ++i) {
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
