#include <cfd/meshio/Preprocessor.hpp>
#include <iostream>

int main(int argc,char** argv) {
    if(argc!=2) { std::cerr<<"Usage: mesh_preprocessor <case_directory>\n"; return 1; }
    try {
        const auto mesh=cfd::Preprocessor::run(argv[1], &std::cout);
        const char* types[]={"unknown","cartesian","cylindrical","spherical","custom"};
        std::cout<<"Domain: "<<mesh.getName()<<" ("<<types[static_cast<unsigned>(mesh.getType())]<<")\n"
                 <<"Nodes: "<<mesh.nodes.getNumber()<<", cells: "<<mesh.cells.getNumber()<<", faces: "<<mesh.faces.getNumber()<<'\n';
        std::map<std::size_t,std::size_t> counts;
        for(std::size_t f=0;f<mesh.faces.getNumber();++f) if(mesh.faces.isBoundary(f)) ++counts[mesh.faces.getBoundaryPatchId(f)];
        for(const auto& [id,name]:mesh.getPatchNames()) std::cout<<"Patch "<<id<<" ("<<name<<"): "<<counts[id]<<" faces\n";
        const auto base=std::filesystem::absolute(argv[1])/mesh.getName();
        std::cout<<"Validation: PASS\nVTU: "<<base.string()<<".vtu\nNative mesh: "<<base.string()<<".mesh\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"Preprocessor error: "<<e.what()<<'\n'; return 1; }
}
