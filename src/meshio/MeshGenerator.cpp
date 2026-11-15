#include <cfd/meshio/MeshGenerator.hpp>
#include <limits>
#include <numbers>

namespace cfd {
namespace {
std::size_t checkedProduct(std::size_t a, std::size_t b) {
    if (b && a > std::numeric_limits<std::size_t>::max()/b)
        throw std::invalid_argument("Mesh resolution overflows entity counts");
    return a*b;
}
template<class Coordinates>
Mesh generateGrid(const MeshConfig& c, Coordinates coordinates) {
    Mesh mesh(c.type, c.dimension, c.domain);
    mesh.setName(c.name);
    mesh.cells.setDimensions(c.Nx,c.Ny,c.Nz);
    mesh.nodes.setDimensions(c.Nx+1,c.Ny+1,c.Nz+1);
    for (std::size_t k=0;k<=c.Nz;++k)
        for (std::size_t j=0;j<=c.Ny;++j)
            for (std::size_t i=0;i<=c.Nx;++i) {
                const auto p=coordinates(double(i)/c.Nx,double(j)/c.Ny,double(k)/c.Nz);
                mesh.nodes.addCoordinates(p[0],p[1],p[2]);
            }
    for (std::size_t k=0;k<c.Nz;++k)
        for (std::size_t j=0;j<c.Ny;++j)
            for (std::size_t i=0;i<c.Nx;++i) {
                const auto n=[&](auto x,auto y,auto z){ return mesh.nodes.getIdFromIndices(x,y,z); };
                mesh.cells.addByNodes({n(i,j,k),n(i+1,j,k),n(i+1,j+1,k),n(i,j+1,k),
                    n(i,j,k+1),n(i+1,j,k+1),n(i+1,j+1,k+1),n(i,j+1,k+1)},mesh.cells.getNumber());
            }
    return mesh;
}
}
void MeshGenerator::validate(const MeshConfig& c) {
    Mesh descriptor(c.type,c.dimension,c.domain);
    descriptor.requireHex3D();
    if (c.type==MeshType::Custom || c.type==MeshType::Unknown)
        throw std::invalid_argument("MeshGenerator requires an internal geometry type");
    if (c.type==MeshType::Cylindrical && c.domain.size()!=5)
        throw std::invalid_argument("Cylindrical generation needs {rInner,rOuter,length,thetaStart,thetaEnd}");
    if (c.type==MeshType::Spherical && c.domain.size()!=6)
        throw std::invalid_argument("Spherical generation needs {rInner,rOuter,polarStart,polarEnd,azimuthStart,azimuthEnd}");
    const auto max=std::numeric_limits<std::size_t>::max();
    if (!c.Nx || !c.Ny || !c.Nz || c.Nx==max || c.Ny==max || c.Nz==max)
        throw std::invalid_argument("Mesh resolutions must be positive and representable");
    checkedProduct(checkedProduct(c.Nx+1,c.Ny+1),c.Nz+1);
    checkedProduct(checkedProduct(c.Nx,c.Ny),c.Nz);
    if (c.type==MeshType::Cylindrical && (c.domain[4]-c.domain[3])/c.Ny >= std::numbers::pi)
        throw std::invalid_argument("Each cylindrical angular cell must span less than 180 degrees");
    if (c.type==MeshType::Spherical && (c.domain[5]-c.domain[4])/c.Nz >= std::numbers::pi)
        throw std::invalid_argument("Each spherical azimuthal cell must span less than 180 degrees");
}
Mesh MeshGenerator::generate(const MeshConfig& c) {
    validate(c);
    switch(c.type) {
        case MeshType::Cartesian: return generateCartesian(c);
        case MeshType::Cylindrical: return generateCylindrical(c);
        case MeshType::Spherical: return generateSpherical(c);
        default: throw std::invalid_argument("Unsupported generator");
    }
}
Mesh MeshGenerator::generateCartesian(const MeshConfig& c) {
    return generateGrid(c,[&](double u,double v,double w){
        return std::array<double,3>{u*c.domain[0],v*c.domain[1],w*c.domain[2]};
    });
}
Mesh MeshGenerator::generateCylindrical(const MeshConfig& c) {
    return generateGrid(c,[&](double u,double v,double w){
        const auto& d=c.domain;
        const double r=d[0]+u*(d[1]-d[0]), theta=d[3]+v*(d[4]-d[3]);
        return std::array<double,3>{r*std::cos(theta),r*std::sin(theta),w*d[2]};
    });
}
Mesh MeshGenerator::generateSpherical(const MeshConfig& c) {
    return generateGrid(c,[&](double u,double v,double w){
        const auto& d=c.domain;
        const double r=d[0]+u*(d[1]-d[0]), theta=d[2]+v*(d[3]-d[2]), phi=d[4]+w*(d[5]-d[4]);
        return std::array<double,3>{r*std::sin(theta)*std::cos(phi),r*std::sin(theta)*std::sin(phi),r*std::cos(theta)};
    });
}
}
