#include <cfd/mesh/GeometryProcessor.hpp>
#include "HexGeometry.hpp"

namespace cfd {
void MeshGeometryProcessor::computeFaces(Mesh& mesh) {
    mesh.requireHex3D();
    for(std::size_t f=0;f<mesh.faces.getNumber();++f) {
        const auto nodes=detail::canonicalFace(mesh.faces.getNodes(f));
        const auto a=mesh.nodes.getCoordinates(nodes[0]);
        double area=0;
        math::Vector3 areaVector{},moment{};
        for(unsigned t=1;t<3;++t) {
            const auto b=detail::subtract(mesh.nodes.getCoordinates(nodes[t]),a);
            const auto c=detail::subtract(mesh.nodes.getCoordinates(nodes[t+1]),a);
            const auto vector=math::productByCoefficient(math::vectorProduct(b,c),0.5);
            const double triangleArea=math::norm(vector);
            if(!std::isfinite(triangleArea) || triangleArea<=0)
                throw std::runtime_error("Degenerate triangle at face "+std::to_string(f));
            area+=triangleArea;
            areaVector=math::vectorSum(areaVector,vector);
            moment=math::vectorSum(moment,math::productByCoefficient(math::vectorSum(b,c),triangleArea/3));
        }
        mesh.faces.setArea(f,area);
        mesh.faces.setAreaVector(f,areaVector);
        mesh.faces.setCenter(f,math::vectorSum(a,math::divisionByCoefficient(moment,area)));
    }
}
void MeshGeometryProcessor::computeCells(Mesh& mesh) {
    mesh.requireHex3D();
    for(std::size_t cell=0;cell<mesh.cells.getNumber();++cell) {
        detail::checkHex(mesh,cell);
        const auto ids=mesh.cells.getNodes(cell);
        const auto origin=mesh.nodes.getCoordinates(ids[0]);
        math::Vector3 reference{};
        for(auto id:ids) reference=math::vectorSum(reference,detail::subtract(mesh.nodes.getCoordinates(id),origin));
        reference=math::vectorSum(origin,math::divisionByCoefficient(reference,8));
        double volume=0;
        math::Vector3 moment{};
        for(auto face:mesh.cells.getFaces(cell)) {
            const auto nodes=detail::canonicalFace(mesh.faces.getNodes(face));
            const double sign=mesh.faces.getOwner(face)==cell ? 1. : -1.;
            const auto a=detail::subtract(mesh.nodes.getCoordinates(nodes[0]),reference);
            for(unsigned t=1;t<3;++t) {
                const auto b=detail::subtract(mesh.nodes.getCoordinates(nodes[t]),reference);
                const auto c=detail::subtract(mesh.nodes.getCoordinates(nodes[t+1]),reference);
                const double v=sign*math::scalarProduct(a,math::vectorProduct(b,c))/6;
                if(!std::isfinite(v) || v<=0)
                    throw std::runtime_error("Non-star-shaped or inverted triangulated cell "+std::to_string(cell));
                volume+=v;
                moment=math::vectorSum(moment,math::productByCoefficient(math::vectorSum(a,b,c),v/4));
            }
        }
        mesh.cells.setVolume(cell,volume);
        mesh.cells.setCenter(cell,math::vectorSum(reference,math::divisionByCoefficient(moment,volume)));
    }
}
}
