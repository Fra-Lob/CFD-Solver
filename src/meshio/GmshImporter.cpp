#include <cfd/meshio/GmshImporter.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/mesh/MeshValidator.hpp>
#include <algorithm>
#include <set>
#if CFD_HAS_GMSH
#include <gmsh.h>
#endif

namespace cfd {
bool GmshImporter::available() { return CFD_HAS_GMSH!=0; }

Mesh GmshImporter::read(const std::filesystem::path& filename) {
    if(!std::filesystem::is_regular_file(filename)) throw std::runtime_error("Missing Gmsh file: "+filename.string());
#if !CFD_HAS_GMSH
    throw std::runtime_error("Gmsh support is unavailable: configure with CFD_ENABLE_GMSH=ON and the Gmsh SDK");
#else
    struct Session {
        Session() {
            if(gmsh::isInitialized()) throw std::runtime_error("GmshImporter requires its own Gmsh session");
            gmsh::initialize();
        }
        ~Session() { try { gmsh::finalize(); } catch(...) {} }
    };
    try {
        Session session;
        gmsh::option::setNumber("General.Terminal",0);
        gmsh::open(filename.string());
        Mesh mesh(MeshType::Custom,3,{}); mesh.setName(filename.stem().string());
        std::vector<std::size_t> tags;
        std::vector<double> xyz,parametric;
        gmsh::model::mesh::getNodes(tags,xyz,parametric);
        if(xyz.size()!=3*tags.size()) throw std::runtime_error("Invalid Gmsh node coordinates");
        std::map<std::size_t,std::size_t> ids;
        for(std::size_t n=0;n<tags.size();++n) {
            if(!ids.emplace(tags[n],n).second) throw std::runtime_error("Duplicate Gmsh node tag");
            mesh.nodes.addCoordinates(xyz[3*n],xyz[3*n+1],xyz[3*n+2]);
        }
        std::vector<int> types;
        std::vector<std::vector<std::size_t>> elementTags,elementNodes;
        gmsh::model::mesh::getElements(types,elementTags,elementNodes,3);
        for(std::size_t block=0;block<types.size();++block) {
            if(types[block]!=5) throw std::runtime_error("Unsupported Gmsh volume element type "+std::to_string(types[block])+"; expected linear hexahedron type 5");
            // Recover Gmsh -> VTK/internal corner ordering from reference coordinates.
            std::string name; int dim,order,nNodes,nPrimary; std::vector<double> local;
            gmsh::model::mesh::getElementProperties(5,name,dim,order,nNodes,local,nPrimary);
            if(nNodes!=8 || local.size()!=24) throw std::runtime_error("Unexpected Gmsh hex reference element");
            constexpr int bits[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
            std::array<unsigned,8> permutation{};
            for(unsigned i=0;i<8;++i) {
                bool found=false;
                for(unsigned j=0;j<8;++j) {
                    bool matches=true;
                    for(unsigned d=0;d<3;++d) matches &= (local[3*j+d]>0)==bool(bits[i][d]);
                    if(matches) { permutation[i]=j; found=true; break; }
                }
                if(!found) throw std::runtime_error("Cannot map Gmsh hexahedron node ordering");
            }
            if(elementNodes[block].size()!=8*elementTags[block].size()) throw std::runtime_error("Invalid Gmsh connectivity");
            for(std::size_t e=0;e<elementTags[block].size();++e) {
                std::array<std::size_t,8> nodes;
                for(unsigned n=0;n<8;++n) nodes[n]=ids.at(elementNodes[block][8*e+permutation[n]]);
                mesh.cells.addByNodes(nodes,mesh.cells.getNumber());
            }
        }
        MeshConnectivityBuilder::build(mesh);
        MeshGeometryProcessor::compute(mesh);
        std::map<std::array<std::size_t,4>,std::size_t> boundary;
        for(std::size_t f=0;f<mesh.faces.getNumber();++f) if(mesh.faces.isBoundary(f)) {
            auto nodes=mesh.faces.getNodes(f); std::sort(nodes.begin(),nodes.end()); boundary.emplace(nodes,f);
        }
        gmsh::vectorpair groups;
        gmsh::model::getPhysicalGroups(groups,2);
        std::map<std::size_t,std::string> patches;
        for(const auto& [dim,tag]:groups) {
            if(tag<=0) throw std::runtime_error("Invalid Gmsh physical tag");
            std::string name; gmsh::model::getPhysicalName(2,tag,name);
            if(name.empty()) name="physical_"+std::to_string(tag);
            patches.emplace(static_cast<std::size_t>(tag),name);
            std::vector<int> surfaces; gmsh::model::getEntitiesForPhysicalGroup(2,tag,surfaces);
            for(int surface:surfaces) {
                gmsh::model::mesh::getElements(types,elementTags,elementNodes,2,surface);
                for(std::size_t b=0;b<types.size();++b) {
                    if(types[b]!=3) throw std::runtime_error("Physical surfaces must contain linear quadrangles (type 3)");
                    if(elementNodes[b].size()!=4*elementTags[b].size()) throw std::runtime_error("Invalid Gmsh surface connectivity");
                    for(std::size_t e=0;e<elementTags[b].size();++e) {
                        std::array<std::size_t,4> nodes;
                        for(unsigned n=0;n<4;++n) nodes[n]=ids.at(elementNodes[b][4*e+n]);
                        std::sort(nodes.begin(),nodes.end());
                        const auto it=boundary.find(nodes);
                        if(it==boundary.end()) throw std::runtime_error("Physical surface does not match an external hex face");
                        if(mesh.faces.getBoundaryPatchId(it->second)!=constants::INVALID_ID)
                            throw std::runtime_error("Boundary face belongs to overlapping physical groups");
                        mesh.faces.setBoundaryPatchId(it->second,static_cast<std::size_t>(tag));
                    }
                }
            }
        }
        mesh.setPatchNames(patches);
        MeshValidator::validate(mesh);
        return mesh;
    } catch(const std::exception& e) { throw std::runtime_error("Gmsh import '"+filename.string()+"': "+e.what()); }
      catch(...) { throw std::runtime_error("Gmsh API failed importing "+filename.string()); }
#endif
}
}
