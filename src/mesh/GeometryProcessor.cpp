#include <cfd/mesh/GeometryProcessor.hpp>

namespace cfd {

    void MeshGeometryProcessor::computeFaces(Mesh& mesh) {
        size_t nFaces = mesh.faces.getNumber();
        for (std::size_t faceId = 0; faceId < nFaces; ++faceId) {
            auto nodes = mesh.faces.getNodes(faceId);
            auto p0 = mesh.nodes.getCoordinates(nodes[0]);
            auto p1 = mesh.nodes.getCoordinates(nodes[1]);
            auto p2 = mesh.nodes.getCoordinates(nodes[2]);
            auto p3 = mesh.nodes.getCoordinates(nodes[3]);

            // Area calculation
            std::array<double, 3> side1 = math::vectorSum(p1, math::opposite(p0));
            std::array<double, 3> side2 = math::vectorSum(p3, math::opposite(p0));
            std::array<double, 3> diagonal = math::vectorSum(p2, math::opposite(p0));

            std::array<double, 3> areaVector1 = math::divisionByCoefficient(math::vectorProduct(side1, diagonal), 2.0);
            std::array<double, 3> areaVector2 = math::divisionByCoefficient(math::vectorProduct(diagonal, side2), 2.0);
            mesh.faces.setAreaVector(faceId, math::vectorSum(areaVector1, areaVector2));
            
            double area1 = math::norm(areaVector1);
            double area2 = math::norm(areaVector2);
            mesh.faces.setArea(faceId, area1 + area2);

            // Center calculation
            std::array<double, 3> center1 = math::divisionByCoefficient(math::vectorSum(p0, p1, p2), 3.0);
            std::array<double, 3> center2 = math::divisionByCoefficient(math::vectorSum(p0, p2, p3), 3.0);
            std::array<double, 3> center = {
                (center1[0] * area1 + center2[0] * area2) / (area1 + area2),
                (center1[1] * area1 + center2[1] * area2) / (area1 + area2),
                (center1[2] * area1 + center2[2] * area2) / (area1 + area2)
            };
            mesh.faces.setCenter(faceId, center);
        }
    }

    void MeshGeometryProcessor::computeCells(Mesh& mesh) {
        size_t nCells = mesh.cells.getNumber();
        for (std::size_t cellId = 0; cellId < nCells; ++cellId) {
            auto nodes = mesh.cells.getNodes(cellId);
            auto p0 = mesh.nodes.getCoordinates(nodes[0]);
            auto p1 = mesh.nodes.getCoordinates(nodes[1]);
            auto p2 = mesh.nodes.getCoordinates(nodes[2]);
            auto p3 = mesh.nodes.getCoordinates(nodes[3]);
            auto p4 = mesh.nodes.getCoordinates(nodes[4]);
            auto p5 = mesh.nodes.getCoordinates(nodes[5]);
            auto p6 = mesh.nodes.getCoordinates(nodes[6]);
            auto p7 = mesh.nodes.getCoordinates(nodes[7]);

            // Volumes calculation (5 tetrahedrs)
            double tetra1 = std::abs(  // 0-1-3-4
                math::scalarProduct(math::vectorSum(p1, math::opposite(p0)),  
                math::vectorProduct(math::vectorSum(p3, math::opposite(p0)), 
                                    math::vectorSum(p4, math::opposite(p0)))) / 6.0
            );
            double tetra2 = std::abs(  // 1-2-3-6
                math::scalarProduct(math::vectorSum(p2, math::opposite(p1)),  
                math::vectorProduct(math::vectorSum(p3, math::opposite(p1)), 
                                    math::vectorSum(p6, math::opposite(p1)))) / 6.0
            );
            double tetra3 = std::abs(  // 1-4-5-6
                math::scalarProduct(math::vectorSum(p4, math::opposite(p1)),  
                math::vectorProduct(math::vectorSum(p5, math::opposite(p1)), 
                                    math::vectorSum(p6, math::opposite(p1)))) / 6.0
            );
            double tetra4 = std::abs(  // 3-4-6-7
                math::scalarProduct(math::vectorSum(p4, math::opposite(p3)),  
                math::vectorProduct(math::vectorSum(p6, math::opposite(p3)), 
                                    math::vectorSum(p7, math::opposite(p3)))) / 6.0
            );
            double tetra5 = std::abs(  // 1-3-4-6
                math::scalarProduct(math::vectorSum(p3, math::opposite(p1)),  
                math::vectorProduct(math::vectorSum(p4, math::opposite(p1)), 
                                    math::vectorSum(p6, math::opposite(p1)))) / 6.0
            );

            double volume = tetra1 + tetra2 + tetra3 + tetra4 + tetra5;
            mesh.cells.setVolume(cellId, volume);

            // Center calculation
            std::array<double, 3> center1 = 
                math::divisionByCoefficient(math::vectorSum(p0, p1, p3, p4), 4);
            std::array<double, 3> center2 =
                math::divisionByCoefficient(math::vectorSum(p1, p2, p3, p6), 4);
            std::array<double, 3> center3 =
                math::divisionByCoefficient(math::vectorSum(p1, p4, p5, p6), 4);
            std::array<double, 3> center4 =
                math::divisionByCoefficient(math::vectorSum(p3, p4, p6, p7), 4);
            std::array<double, 3> center5 =
                math::divisionByCoefficient(math::vectorSum(p1, p3, p4, p6), 4);

            std::array<double, 3> center = {
                (center1[0] * tetra1 + center2[0] * tetra2 + center3[0] * tetra3 + 
                 center4[0] * tetra4 + center5[0] * tetra5  ) / volume,
                (center1[1] * tetra1 + center2[1] * tetra2 + center3[1] * tetra3 + 
                 center4[1] * tetra4 + center5[1] * tetra5  ) / volume,
                (center1[2] * tetra1 + center2[2] * tetra2 + center3[2] * tetra3 + 
                 center4[2] * tetra4 + center5[2] * tetra5  ) / volume,
            };
            mesh.cells.setCenter(cellId, center);
        }
    }   
}