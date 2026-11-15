#include <cfd/mesh/ConnectivityBuilder.hpp>
#include "HexGeometry.hpp"

namespace cfd {

    void MeshConnectivityBuilder::build(Mesh& mesh) {
        mesh.requireHex3D();
        if (mesh.faces.getNumber()!=0) throw std::logic_error("Connectivity is already built");
        std::map<std::array<std::size_t, 4>, std::size_t> faceMap; // Map to store unique facesnding cell IDs
        for (std::size_t cellId = 0; cellId < mesh.cells.getNumber(); ++cellId) {
            std::array<std::size_t, 8> cellNodes = mesh.cells.getNodes(cellId);

            // Define the faces of the hexahedral cell
            std::array<std::array<std::size_t,4>,6> cellFaces;
            for(unsigned f=0;f<6;++f) for(unsigned n=0;n<4;++n)
                cellFaces[f][n]=cellNodes[detail::localFaces[f][n]];
            
            int localFaceId = 0;
            for (const auto& faceNodes : cellFaces) {
                std::array<std::size_t, 4> sortedFaceNodes = faceNodes;
                std::sort(sortedFaceNodes.begin(), sortedFaceNodes.end());

                std::size_t faceId;
                auto it = faceMap.find(sortedFaceNodes);
                if (it == faceMap.end()) {
                    // Face not found, add it to the map with the current cell as owner
                    faceId = mesh.faces.getNumber();
                    mesh.faces.addByNodes(faceNodes, cellId);
                    faceMap[sortedFaceNodes] = faceId;
                } else {
                    // Face found, set the neighbor for the existing face
                    faceId = it->second;
                    mesh.faces.setNeighbor(faceId, cellId);
                }
                mesh.cells.setFace(cellId, faceId, localFaceId);
                localFaceId++;
            }
        }
    }
}
