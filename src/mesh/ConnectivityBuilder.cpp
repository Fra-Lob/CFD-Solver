#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <map>
#include <algorithm>

namespace cfd {

    void MeshConnectivityBuilder::build(Mesh& mesh) {
        std::map<std::array<std::size_t, 4>, std::size_t> faceMap; // Map to store unique facesnding cell IDs
        for (std::size_t cellId = 0; cellId < mesh.cells.getNumber(); ++cellId) {
            std::array<std::size_t, 8> cellNodes = mesh.cells.getNodes(cellId);

            // Define the faces of the hexahedral cell
            std::array<std::array<std::size_t, 4>, 6> cellFaces = {{
                {cellNodes[0], cellNodes[3], cellNodes[2], cellNodes[1]}, // Bottom face
                {cellNodes[4], cellNodes[5], cellNodes[6], cellNodes[7]}, // Top face
                {cellNodes[0], cellNodes[1], cellNodes[5], cellNodes[4]}, // Front face
                {cellNodes[3], cellNodes[7], cellNodes[6], cellNodes[2]}, // Back face
                {cellNodes[0], cellNodes[4], cellNodes[7], cellNodes[3]}, // Left face
                {cellNodes[1], cellNodes[2], cellNodes[6], cellNodes[5]}  // Right face
            }};
            
            int localFaceId = 0;
            for (const auto& faceNodes : cellFaces) {
                std::array<std::size_t, 4> sortedFaceNodes = faceNodes;
                std::sort(sortedFaceNodes.begin(), sortedFaceNodes.end());

                auto it = faceMap.find(sortedFaceNodes);
                if (it == faceMap.end()) {
                    // Face not found, add it to the map with the current cell as owner
                    std::size_t faceId = mesh.faces.getNumber();
                    mesh.faces.addByNodes(faceNodes, cellId);
                    faceMap[sortedFaceNodes] = faceId;
                    mesh.cells.setFace(cellId, faceId, localFaceId);
                } else {
                    // Face found, set the neighbor for the existing face
                    std::size_t ownerCellId = it->second;
                    mesh.faces.setNeighbor(it->second, cellId);
                    mesh.cells.setFace(cellId, it->second, localFaceId);
                }
                localFaceId++;
            }
        }
    }
}