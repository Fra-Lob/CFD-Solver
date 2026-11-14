#include <cfd/mesh/BoundaryPatchBuilder.hpp>
#include <algorithm>
#include <limits>

namespace cfd {

    void MeshBoundaryPatchBuilder::buildCartesian(Mesh& mesh) {
        mesh.requireCartesian3D();
        const auto& domain = mesh.getDomain();
        std::vector<std::size_t> patches(mesh.faces.getNumber(), constants::INVALID_ID);
        
        size_t nFaces = mesh.faces.getNumber();
        for (std::size_t faceId = 0; faceId < nFaces; ++faceId) {
            if (mesh.faces.isBoundary(faceId)) {
                for (unsigned axis = 0; axis < 3; ++axis) {
                    const auto tolerance = 64 * std::numeric_limits<double>::epsilon() * domain[axis];
                    for (unsigned side = 0; side < 2; ++side) {
                        const double coordinate = side == 0 ? 0.0 : domain[axis];
                        const auto nodes = mesh.faces.getNodes(faceId);
                        const bool onPlane = std::all_of(nodes.begin(), nodes.end(), [&](std::size_t node) {
                            return std::abs(mesh.nodes.getCoordinates(node)[axis] - coordinate) <= tolerance;
                        });
                        if (onPlane) {
                            if (patches[faceId] != constants::INVALID_ID)
                                throw std::runtime_error("Boundary face matches multiple domain planes");
                            patches[faceId] = 2 * axis + side; // xmin/xmax/ymin/ymax/zmin/zmax
                        }
                    }
                }
                if (patches[faceId] == constants::INVALID_ID)
                    throw std::runtime_error("Boundary face does not lie on the declared Cartesian domain");
            }
        }
        // Commit only after every boundary face has been classified successfully.
        for (std::size_t faceId = 0; faceId < nFaces; ++faceId)
            mesh.faces.setBoundaryPatchId(faceId, patches[faceId]);
    }
}
