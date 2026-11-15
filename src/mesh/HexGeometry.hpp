#pragma once
#include <cfd/mesh/Mesh.hpp>
#include <algorithm>
#include <limits>

namespace cfd::detail {
inline constexpr std::array<std::array<unsigned,4>,6> localFaces = {{
    {0,3,2,1},{4,5,6,7},{0,1,5,4},{3,7,6,2},{0,4,7,3},{1,2,6,5}
}};

inline std::array<std::size_t,4> canonicalFace(std::array<std::size_t,4> nodes) {
    std::rotate(nodes.begin(),std::min_element(nodes.begin(),nodes.end()),nodes.end());
    return nodes; // Same diagonal for both incidences of a shared face.
}
inline math::Vector3 subtract(const math::Vector3& a,const math::Vector3& b) {
    return math::vectorSum(a,math::opposite(b));
}

using Bernstein = std::array<double,27>;
inline bool positiveBernstein(const Bernstein& b,unsigned depth,double tolerance) {
    const auto [lo,hi]=std::minmax_element(b.begin(),b.end());
    if(*lo>tolerance) return true;
    if(*hi<=tolerance || depth==18) return false;
    const unsigned stride=depth%3==0 ? 1 : (depth%3==1 ? 3 : 9);
    Bernstein left=b,right=b;
    for(unsigned i=0;i<27;++i) if((i/stride)%3==0) {
        const double a=b[i],m=b[i+stride],z=b[i+2*stride];
        const double center=(a+2*m+z)/4;
        left[i]=a; left[i+stride]=(a+m)/2; left[i+2*stride]=center;
        right[i]=center; right[i+stride]=(m+z)/2; right[i+2*stride]=z;
    }
    return positiveBernstein(left,depth+1,tolerance) && positiveBernstein(right,depth+1,tolerance);
}

// Certify the trilinear hex Jacobian over the entire reference cube using
// tensor-quadratic Bernstein bounds. Ambiguous/nearly singular cells are rejected.
inline void checkHex(const Mesh& mesh,std::size_t cell) {
    const auto nodes=mesh.cells.getNodes(cell);
    std::array<math::Vector3,8> points;
    const auto origin=mesh.nodes.getCoordinates(nodes[0]);
    double scale=0;
    for(unsigned n=0;n<8;++n) {
        points[n]=subtract(mesh.nodes.getCoordinates(nodes[n]),origin);
        for(double x:points[n]) scale=std::max(scale,std::abs(x));
    }
    const auto invalid=[&] { throw std::runtime_error("Invalid, inverted or uncertifiable hexahedron at cell "+std::to_string(cell)); };
    if(!std::isfinite(scale) || scale<=0) invalid();
    for(auto& p:points) for(double& x:p) x/=scale;
    constexpr unsigned bits[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
    Bernstein b{};
    for(unsigned k=0;k<3;++k) for(unsigned j=0;j<3;++j) for(unsigned i=0;i<3;++i) {
        const double t[3]={i/2.,j/2.,k/2.};
        std::array<math::Vector3,3> jac{};
        for(unsigned n=0;n<8;++n) for(unsigned d=0;d<3;++d) {
            double weight=bits[n][d] ? 1. : -1.;
            for(unsigned e=0;e<3;++e) if(e!=d) weight*=bits[n][e] ? t[e] : 1-t[e];
            jac[d]=math::vectorSum(jac[d],math::productByCoefficient(points[n],weight));
        }
        b[i+3*j+9*k]=math::scalarProduct(jac[0],math::vectorProduct(jac[1],jac[2]));
    }
    for(unsigned stride:{1u,3u,9u})
        for(unsigned i=0;i<27;++i) if((i/stride)%3==0)
            b[i+stride]=2*b[i+stride]-(b[i]+b[i+2*stride])/2;
    double magnitude=0;
    for(double v:b) { if(!std::isfinite(v)) invalid(); magnitude=std::max(magnitude,std::abs(v)); }
    if(!positiveBernstein(b,0,256*std::numeric_limits<double>::epsilon()*magnitude)) invalid();
}
}
