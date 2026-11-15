#include "TestUtils.hpp"
#include <cfd/meshio/GridConfigReader.hpp>
#include <cfd/meshio/GmshImporter.hpp>
#include <cfd/meshio/Preprocessor.hpp>
#include <cfd/meshio/MeshReader.hpp>
#include <cfd/meshio/MeshWriter.hpp>
#include <cfd/meshio/VTKReader.hpp>
#include <cfd/meshio/VTKWriter.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/mesh/BoundaryPatchBuilder.hpp>
#include <cfd/mesh/MeshValidator.hpp>
#include <fstream>
#include <iostream>
#include <iterator>

namespace fs=std::filesystem;
using namespace cfd;
namespace {
template<class F> void rejects(F action,const std::string& label) {
    try { action(); } catch(const std::exception&) { return; }
    throw std::runtime_error("Expected rejection: "+label);
}
void save(const fs::path& path,const std::string& text) {
    std::ofstream out(path,std::ios::binary); out<<text; out.close();
    test::require(bool(out),"Cannot save test fixture");
}
std::string contents(const fs::path& path) {
    std::ifstream in(path,std::ios::binary);
    test::require(bool(in),"Cannot read test fixture");
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
Mesh complete(const MeshConfig& config) {
    auto mesh=MeshGenerator::generate(config);
    MeshConnectivityBuilder::build(mesh); MeshGeometryProcessor::compute(mesh);
    MeshBoundaryPatchBuilder::build(mesh); MeshValidator::validate(mesh);
    return mesh;
}
double volume(const Mesh& mesh) {
    double sum=0; for(std::size_t c=0;c<mesh.cells.getNumber();++c) sum+=mesh.cells.getVolume(c); return sum;
}
void counts(const Mesh& mesh,std::size_t nx,std::size_t ny,std::size_t nz) {
    test::require(mesh.nodes.getNumber()==(nx+1)*(ny+1)*(nz+1),"Node count");
    test::require(mesh.cells.getNumber()==nx*ny*nz,"Cell count");
    test::require(mesh.faces.getNumber()==(nx+1)*ny*nz+nx*(ny+1)*nz+nx*ny*(nz+1),"Face count");
    std::array<std::size_t,6> patches{};
    for(std::size_t f=0;f<mesh.faces.getNumber();++f) if(mesh.faces.isBoundary(f)) {
        test::require(mesh.faces.getBoundaryPatchId(f)<6,"Patch ID"); ++patches[mesh.faces.getBoundaryPatchId(f)];
    }
    test::require(patches==std::array<std::size_t,6>{ny*nz,ny*nz,nx*nz,nx*nz,nx*ny,nx*ny},"Patch counts");
}
void equal(const Mesh& a,const Mesh& b) {
    test::require(a.getName()==b.getName() && a.getType()==b.getType() && a.getDimension()==b.getDimension() &&
                  a.getDomain()==b.getDomain() && a.getPatchNames()==b.getPatchNames(),"All domain/patch metadata");
    test::require(a.nodes.getNumber()==b.nodes.getNumber() && a.cells.getNumber()==b.cells.getNumber() && a.faces.getNumber()==b.faces.getNumber(),"All entity counts");
    test::require(a.nodes.getNx()==b.nodes.getNx() && a.nodes.getNy()==b.nodes.getNy() && a.nodes.getNz()==b.nodes.getNz() &&
                  a.cells.getNx()==b.cells.getNx() && a.cells.getNy()==b.cells.getNy() && a.cells.getNz()==b.cells.getNz(),"Logical dimensions");
    for(std::size_t n=0;n<a.nodes.getNumber();++n) test::require(a.nodes.getCoordinates(n)==b.nodes.getCoordinates(n),"Node coordinates");
    for(std::size_t c=0;c<a.cells.getNumber();++c) test::require(
        a.cells.getNodes(c)==b.cells.getNodes(c) && a.cells.getFaces(c)==b.cells.getFaces(c) &&
        a.cells.getVolume(c)==b.cells.getVolume(c) && a.cells.getCenter(c)==b.cells.getCenter(c),"All cell attributes");
    for(std::size_t f=0;f<a.faces.getNumber();++f) test::require(
        a.faces.getNodes(f)==b.faces.getNodes(f) && a.faces.getOwner(f)==b.faces.getOwner(f) &&
        a.faces.getNeighbor(f)==b.faces.getNeighbor(f) && a.faces.getCenter(f)==b.faces.getCenter(f) &&
        a.faces.getArea(f)==b.faces.getArea(f) && a.faces.getAreaVector(f)==b.faces.getAreaVector(f) &&
        a.faces.getBoundaryPatchId(f)==b.faces.getBoundaryPatchId(f),"All face attributes");
}

void configTests(const fs::path& temp) {
    const std::string valid="\n# comment\nname = box\ntype=cartesian\nLx=2 # metres\nLy=3\nLz=4\nNx=2\nNy=3\nNz=4\n";
    save(temp/"grid.info",valid);
    auto c=GridConfigReader::read(temp);
    test::require(c.type==MeshType::Cartesian && c.domain==std::vector<double>{2,3,4} && c.Ny==3 && c.name=="box","Cartesian parsing");
    for(const auto& suffix:{"Nx=3\n","junk=1\n","bad line\n"}) {
        save(temp/"grid.info",valid+suffix); rejects([&]{GridConfigReader::read(temp);},"invalid syntax/duplicate/unknown");
    }
    for(const auto& [before,after]:std::initializer_list<std::pair<std::string,std::string>>{
        {"Nx=2","Nx=-1"},{"Nx=2","Nx=1.5"},{"Nx=2","Nx=0"},{"Nx=2","Nx=18446744073709551616"},
        {"Lx=2","Lx=nan"},{"Lx=2","Lx=inf"},{"Lx=2","Lx=-2"},{"Lx=2","Lx=2x"},
        {"name = box","name = ../escape"},{"type=cartesian","type=other"},{"Ny=3\n",""}}) {
        auto text=valid; text.replace(text.find(before),before.size(),after); save(temp/"grid.info",text);
        rejects([&]{GridConfigReader::read(temp);},before);
    }
    for(const auto& name:{"cylindrical","spherical","custom"}) {
        save(temp/"grid.info",contents(fs::path(CFD_SOURCE_DIR)/"examples"/name/"grid.info"));
        c=GridConfigReader::read(temp);
        test::require(c.dimension==3,"Physical dimension remains 3D");
        if(c.type==MeshType::Cylindrical) test::require(test::approx(c.domain[3],-std::numbers::pi/6),"Degrees to radians");
    }
    fs::remove(temp/"grid.info"); rejects([&]{GridConfigReader::read(temp);},"missing grid.info");
    std::cout<<"PASS configuration parsing and validation\n";
}

void generatedTests() {
    auto box=complete({MeshType::Cartesian,3,{2,3,4},4,3,2}); counts(box,4,3,2);
    test::require(test::approx(volume(box),24),"Cartesian analytical volume");
    const double pi=std::numbers::pi;
    for(std::size_t nr:{1u,3u}) for(std::size_t nt:{1u,12u}) {
        auto cylinder=complete({MeshType::Cylindrical,3,{1,2,3,-pi/6,pi/3},nr,nt,2}); counts(cylinder,nr,nt,2);
        const double expected=0.5*3*3*nt*std::sin((pi/2)/nt);
        test::require(test::approx(volume(cylinder),expected),"Exact chordal cylindrical volume");
    }
    const auto coarse=complete({MeshType::Spherical,3,{1,2,pi/6,2*pi/3,0,pi/2},2,4,4});
    const auto fine=complete({MeshType::Spherical,3,{1,2,pi/6,2*pi/3,0,pi/2},2,16,16});
    counts(fine,2,16,16);
    const double analytic=7./3*(std::cos(pi/6)-std::cos(2*pi/3))*(pi/2);
    test::require(std::abs(volume(fine)-analytic)<std::abs(volume(coarse)-analytic),"Spherical curved-volume convergence");
    test::require(std::abs(volume(fine)/analytic-1)<0.01,"Spherical refined curved-volume comparison");
    complete({MeshType::Spherical,3,{1,2,pi/4,pi/2,0,pi/3},1,1,1});
    for(const auto& config:std::initializer_list<MeshConfig>{
        {MeshType::Cylindrical,3,{0,2,3,0,pi/2},1,1,1},
        {MeshType::Cylindrical,3,{1,2,3,0,2*pi},1,4,1},
        {MeshType::Cylindrical,3,{1,2,3,0,1.5*pi},1,1,1},
        {MeshType::Spherical,3,{0,2,pi/4,pi/2,0,pi/2},1,1,1},
        {MeshType::Spherical,3,{1,2,0,pi/2,0,pi/2},1,1,1},
        {MeshType::Spherical,3,{1,2,pi/4,pi,0,pi/2},1,1,1},
        {MeshType::Spherical,3,{1,2,pi/4,pi/2,0,2*pi},1,1,4}})
        rejects([&]{MeshGenerator::generate(config);},"singular/periodic/inverted configuration");
    auto corrupt=box;
    corrupt.faces.setAreaVector(0,math::productByCoefficient(corrupt.faces.getAreaVector(0),-1));
    rejects([&]{MeshValidator::validate(corrupt);},"inward normal");
    corrupt=box; corrupt.faces.setBoundaryPatchId(0,constants::INVALID_ID);
    rejects([&]{MeshValidator::validate(corrupt);},"missing patch");
    corrupt=box; corrupt.faces.setAreaVector(0,math::productByCoefficient(corrupt.faces.getAreaVector(0),0.9));
    rejects([&]{MeshValidator::validate(corrupt);},"closure");
    corrupt=box; auto nodes=corrupt.cells.getNodes(0); std::swap(nodes[0],nodes[1]); corrupt.cells.setNodes(0,nodes);
    rejects([&]{MeshValidator::validate(corrupt);},"inverted cell");
    corrupt=box; corrupt.nodes.setCoordinates(0,corrupt.nodes.getCoordinates(1));
    rejects([&]{MeshValidator::validate(corrupt);},"collapsed cell");
    corrupt=box; corrupt.faces.addByNodes(corrupt.faces.getNodes(0),0);
    rejects([&]{MeshValidator::validate(corrupt);},"duplicate face");
    std::cout<<"PASS internal generators, analytical comparisons, topology, patches, orientations, closure and invalid cells\n";
}

void nativeTests(const fs::path& temp) {
    const double pi=std::numbers::pi;
    for(const auto& c:std::initializer_list<MeshConfig>{
        {MeshType::Cartesian,3,{2,3,4},3,2,2,"box"},
        {MeshType::Cylindrical,3,{1,2,3,-pi/6,pi/3},2,5,3,"annulus"},
        {MeshType::Spherical,3,{1,2,pi/6,pi/2,0,pi/2},2,4,5,"shell"}}) {
        const auto mesh=complete(c);
        const auto native=temp/(c.name+".mesh"),vtu=temp/(c.name+".vtu");
        MeshWriter::write(mesh,native); equal(mesh,MeshReader::read(native));
        MeshWriter::write(MeshReader::read(native),temp/"second.mesh");
        test::require(contents(native)==contents(temp/"second.mesh"),"Bit-exact native round trip");
        VTKWriter::write(mesh,vtu.string());
        const auto visual=VTKReader::read(vtu.string()); MeshValidator::validate(visual);
        test::require(test::approx(volume(visual),volume(mesh)),"VTU mesh metrics");
        auto bytes=contents(native);
        for(unsigned mutation=0;mutation<5;++mutation) {
            auto broken=bytes;
            if(mutation==0) broken[0]='X';
            if(mutation==1) broken[8]=99;
            if(mutation==2) broken.resize(broken.size()/2);
            if(mutation==3) broken.push_back('X');
            if(mutation==4) for(std::size_t i=broken.size()-8;i<broken.size();++i) broken[i]=char(255);
            save(temp/"bad.mesh",broken); rejects([&]{MeshReader::read(temp/"bad.mesh");},"corrupt native mesh");
        }
    }
    rejects([&]{MeshReader::read(temp/"missing.mesh");},"missing native mesh");
    // Exercise the common custom/warped-hex path even in a build without Gmsh.
    const auto grid=MeshGenerator::generate({MeshType::Cartesian,3,{1,1,1},1,1,1});
    Mesh custom(MeshType::Custom,3,{}); custom.setName("warped_custom");
    custom.nodes=grid.nodes; custom.cells=grid.cells;
    custom.nodes.setDimensions(0,0,0); custom.cells.setDimensions(0,0,0);
    const auto corner=custom.cells.getNodes(0)[6];
    auto coordinate=custom.nodes.getCoordinates(corner); coordinate[2]+=0.2;
    custom.nodes.setCoordinates(corner,coordinate);
    MeshConnectivityBuilder::build(custom); MeshGeometryProcessor::compute(custom);
    custom.setPatchNames({{42,"warped walls"}});
    for(std::size_t f=0;f<custom.faces.getNumber();++f) custom.faces.setBoundaryPatchId(f,42);
    MeshValidator::validate(custom);
    bool warped=false;
    for(std::size_t f=0;f<custom.faces.getNumber();++f)
        warped |= custom.faces.getArea(f)>math::norm(custom.faces.getAreaVector(f))*(1+1e-8);
    test::require(warped,"Warped scalar/vector area distinction");
    MeshWriter::write(custom,temp/"custom.mesh"); equal(custom,MeshReader::read(temp/"custom.mesh"));
    VTKWriter::write(custom,(temp/"custom.vtu").string());
    test::require(VTKReader::read((temp/"custom.vtu").string()).cells.getNumber()==1,"Custom VTU visualization");
    std::cout<<"PASS full native serialization, all attributes, malformed files and VTU round trip\n";
}

void pipelineTests(const fs::path& temp) {
    for(const auto& name:{"cartesian","cylindrical","spherical"}) {
        const auto directory=temp/name; fs::create_directories(directory);
        save(directory/"grid.info",contents(fs::path(CFD_SOURCE_DIR)/"examples"/name/"grid.info"));
        const auto mesh=Preprocessor::run(directory);
        equal(mesh,MeshReader::read(directory/(mesh.getName()+".mesh")));
        const auto native=directory/(mesh.getName()+".mesh"),vtu=directory/(mesh.getName()+".vtu");
        const auto oldNative=contents(native),oldVTU=contents(vtu);
        equal(mesh,Preprocessor::run(directory)); // Replace complete existing outputs.
        save(directory/"grid.info","name="+mesh.getName()+"\ntype=cartesian\nLx=-1\n");
        rejects([&]{Preprocessor::run(directory);},"invalid pipeline input");
        test::require(contents(native)==oldNative && contents(vtu)==oldVTU,"Invalid input overwrote existing outputs");
        for(const auto& entry:fs::directory_iterator(directory))
            test::require(!entry.path().filename().string().starts_with(".preprocessor-"),"Staging cleanup");
    }
    std::cout<<"PASS complete case workflows and existing-output protection\n";
}

void gmshTests(const fs::path& temp) {
    const auto source=fs::path(CFD_SOURCE_DIR);
    rejects([&]{GmshImporter::read(temp/"missing.msh");},"missing Gmsh input");
    if(GmshImporter::available()) {
        const auto mesh=GmshImporter::read(source/"examples/custom/hex.msh");
        MeshValidator::validate(mesh);
        test::require(mesh.cells.getNumber()==1 && mesh.faces.getNumber()==6 && test::approx(volume(mesh),1),"Gmsh fixture");
        test::require(mesh.getPatchNames().at(7)=="walls","Gmsh physical name/tag");
        for(std::size_t f=0;f<6;++f) test::require(mesh.faces.getBoundaryPatchId(f)==7,"Gmsh physical patch");
        MeshWriter::write(mesh,temp/"gmsh.mesh"); equal(mesh,MeshReader::read(temp/"gmsh.mesh"));
        rejects([&]{GmshImporter::read(source/"tests/mesh/unsupported.msh");},"tetrahedron");
        save(temp/"hex.msh",contents(source/"examples/custom/hex.msh"));
        save(temp/"grid.info",contents(source/"examples/custom/grid.info"));
        equal(mesh,Preprocessor::run(temp));
        std::cout<<"PASS Gmsh API hex import, physical groups, unsupported elements and custom pipeline\n";
    } else {
        bool unavailable=false;
        try { GmshImporter::read(source/"examples/custom/hex.msh"); }
        catch(const std::runtime_error& e) { unavailable=std::string(e.what()).find("Gmsh support is unavailable")!=std::string::npos; }
        test::require(unavailable,"Clear no-SDK diagnostic");
        std::cout<<"PASS no-Gmsh fallback (Gmsh API integration tests not run: SDK unavailable)\n";
    }
}
}

int main(int argc,char** argv) {
    try {
        if(argc!=2) throw std::runtime_error("Expected test group");
        const std::string group=argv[1]; const auto temp=fs::current_path()/("preprocessor-test-"+group); fs::create_directories(temp);
        if(group=="config") configTests(temp);
        else if(group=="generated") generatedTests();
        else if(group=="native") nativeTests(temp);
        else if(group=="pipeline") pipelineTests(temp);
        else if(group=="gmsh") gmshTests(temp);
        else throw std::runtime_error("Unknown test group");
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
