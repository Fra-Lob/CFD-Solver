#include <cfd/mesh/MeshValidator.hpp>
#include "HexGeometry.hpp"
#include <set>

namespace cfd {
namespace {
void require(bool condition,const std::string& message,std::size_t id) {
    if(!condition) throw std::runtime_error("Mesh validation: "+message+" (entity "+std::to_string(id)+")");
}
bool finite(const math::Vector3& v) {
    return std::all_of(v.begin(),v.end(),[](double x){ return std::isfinite(x); });
}
void dimensions(std::array<std::size_t,3> d,std::size_t count) {
    if(d==std::array<std::size_t,3>{0,0,0}) return;
    std::size_t result=1;
    for(auto n:d) {
        if(!n || result>std::numeric_limits<std::size_t>::max()/n) throw std::runtime_error("Invalid logical dimensions");
        result*=n;
    }
    if(result!=count) throw std::runtime_error("Logical dimensions disagree with entity count");
}
}
void MeshValidator::validate(const Mesh& m) {
    m.requireHex3D();
    const auto np=m.nodes.getNumber(),nc=m.cells.getNumber(),nf=m.faces.getNumber();
    if(!np || !nc || !nf) throw std::runtime_error("Mesh validation: empty mesh");
    dimensions({m.nodes.getNx(),m.nodes.getNy(),m.nodes.getNz()},np);
    dimensions({m.cells.getNx(),m.cells.getNy(),m.cells.getNz()},nc);
    for(std::size_t n=0;n<np;++n) require(finite(m.nodes.getCoordinates(n)),"nonfinite node",n);
    std::set<std::array<std::size_t,4>> uniqueFaces;
    std::vector<unsigned> incidences(nf,0);
    for(std::size_t f=0;f<nf;++f) {
        auto nodes=m.faces.getNodes(f); std::sort(nodes.begin(),nodes.end());
        require(nodes.back()<np && std::adjacent_find(nodes.begin(),nodes.end())==nodes.end(),"invalid face nodes",f);
        require(uniqueFaces.insert(nodes).second,"duplicate face",f);
        const auto owner=m.faces.getOwner(f),neighbor=m.faces.getNeighbor(f),patch=m.faces.getBoundaryPatchId(f);
        require(owner<nc,"invalid owner",f);
        require(neighbor==constants::INVALID_ID || (neighbor<nc && neighbor!=owner),"invalid neighbor",f);
        require(std::isfinite(m.faces.getArea(f)) && m.faces.getArea(f)>0,"invalid area",f);
        const auto vector=m.faces.getAreaVector(f),center=m.faces.getCenter(f);
        require(finite(vector) && finite(center) && math::norm(vector)>0,"invalid face geometry",f);
        require(math::norm(vector)<=m.faces.getArea(f)*(1+1e-10),"area vector exceeds scalar area",f);
        if(neighbor==constants::INVALID_ID)
            require(patch!=constants::INVALID_ID && m.getPatchNames().contains(patch),"unclassified boundary patch",f);
        else require(patch==constants::INVALID_ID,"internal face has boundary patch",f);
    }
    std::set<std::array<std::size_t,8>> uniqueCells;
    for(std::size_t c=0;c<nc;++c) {
        const auto nodes=m.cells.getNodes(c);
        auto sorted=nodes; std::sort(sorted.begin(),sorted.end());
        require(sorted.back()<np && std::adjacent_find(sorted.begin(),sorted.end())==sorted.end(),"invalid cell nodes",c);
        require(uniqueCells.insert(sorted).second,"duplicate cell",c);
        detail::checkHex(m,c);
        require(std::isfinite(m.cells.getVolume(c)) && m.cells.getVolume(c)>0,"invalid cell volume",c);
        require(finite(m.cells.getCenter(c)),"invalid cell center",c);
        const auto faces=m.cells.getFaces(c);
        auto sortedFaces=faces; std::sort(sortedFaces.begin(),sortedFaces.end());
        require(sortedFaces.back()<nf && std::adjacent_find(sortedFaces.begin(),sortedFaces.end())==sortedFaces.end(),"invalid cell faces",c);
        math::Vector3 closure{}; double area=0;
        for(unsigned local=0;local<6;++local) {
            const auto f=faces[local]; ++incidences[f];
            std::array<std::size_t,4> expected;
            for(unsigned n=0;n<4;++n) expected[n]=nodes[detail::localFaces[local][n]];
            std::sort(expected.begin(),expected.end());
            auto actual=m.faces.getNodes(f); std::sort(actual.begin(),actual.end());
            require(actual==expected,"cell-to-face node mismatch",c);
            const auto owner=m.faces.getOwner(f),neighbor=m.faces.getNeighbor(f);
            require(owner==c || neighbor==c,"cell-to-face owner mismatch",c);
            const double sign=owner==c ? 1. : -1.;
            const auto outward=math::productByCoefficient(m.faces.getAreaVector(f),sign);
            const auto direction=detail::subtract(m.faces.getCenter(f),m.cells.getCenter(c));
            require(math::scalarProduct(outward,direction)>0,"inward face normal",f);
            // Each oriented triangle must also point away from the cell center.
            const auto fn=detail::canonicalFace(m.faces.getNodes(f));
            const auto a=detail::subtract(m.nodes.getCoordinates(fn[0]),m.cells.getCenter(c));
            for(unsigned t=1;t<3;++t) {
                const auto b=detail::subtract(m.nodes.getCoordinates(fn[t]),m.cells.getCenter(c));
                const auto z=detail::subtract(m.nodes.getCoordinates(fn[t+1]),m.cells.getCenter(c));
                require(sign*math::scalarProduct(a,math::vectorProduct(b,z))>0,"non-star-shaped face triangle",f);
            }
            closure=math::vectorSum(closure,outward); area+=m.faces.getArea(f);
        }
        require(math::norm(closure)<=1e-10*area,"area-vector closure failure",c);
    }
    for(std::size_t f=0;f<nf;++f)
        require(incidences[f]==(m.faces.isBoundary(f) ? 1u : 2u),"wrong face incidence count",f);
}
}
