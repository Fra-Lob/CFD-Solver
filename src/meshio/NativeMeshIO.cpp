#include <cfd/meshio/MeshWriter.hpp>
#include <cfd/meshio/MeshReader.hpp>
#include <cfd/mesh/MeshValidator.hpp>
#include <bit>
#include <cstdint>
#include <fstream>
#include <limits>

namespace cfd {
namespace {
constexpr char magic[8]={'C','F','D','M','E','S','H','\0'};
static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559);

class Output {
    std::ofstream file;
    std::uint64_t checksum_=14695981039346656037ull;
public:
    explicit Output(const std::filesystem::path& path):file(path,std::ios::binary|std::ios::trunc) {
        if(!file) throw std::runtime_error("Cannot write "+path.string());
    }
    void bytes(const char* data,std::size_t size) {
        file.write(data,static_cast<std::streamsize>(size));
        for(std::size_t i=0;i<size;++i) { checksum_^=static_cast<unsigned char>(data[i]); checksum_*=1099511628211ull; }
    }
    void integer(std::uint64_t value,unsigned size=8) {
        for(unsigned i=0;i<size;++i) { const char b=static_cast<char>((value>>(8*i))&255); bytes(&b,1); }
    }
    void real(double value) { integer(std::bit_cast<std::uint64_t>(value)); }
    void id(std::size_t value) { integer(value==constants::INVALID_ID ? UINT64_MAX : value); }
    void vector(const math::Vector3& v) { for(double value:v) real(value); }
    void string(const std::string& s) {
        if(s.size()>1048576) throw std::runtime_error("Native mesh string exceeds 1 MiB");
        integer(s.size()); bytes(s.data(),s.size());
    }
    void close() { const auto digest=checksum_; integer(digest); file.close(); if(!file) throw std::runtime_error("Native mesh write failed"); }
};

class Input {
    std::ifstream file;
    std::uint64_t remaining_;
    std::uint64_t checksum_=14695981039346656037ull;
public:
    explicit Input(const std::filesystem::path& path):file(path,std::ios::binary),remaining_(0) {
        if(!file) throw std::runtime_error("Cannot read "+path.string());
        remaining_=std::filesystem::file_size(path);
    }
    std::uint64_t remaining() const { return remaining_; }
    std::uint64_t checksum() const { return checksum_; }
    void bytes(char* data,std::size_t n) {
        if(n>remaining_) throw std::runtime_error("Truncated native mesh");
        file.read(data,static_cast<std::streamsize>(n));
        if(!file) throw std::runtime_error("Native mesh read failed");
        remaining_-=n;
        for(std::size_t i=0;i<n;++i) { checksum_^=static_cast<unsigned char>(data[i]); checksum_*=1099511628211ull; }
    }
    std::uint64_t integer(unsigned size=8) {
        unsigned char b[8]{}; bytes(reinterpret_cast<char*>(b),size);
        std::uint64_t v=0;
        for(unsigned i=0;i<size;++i) v|=std::uint64_t(b[i])<<(8*i);
        return v;
    }
    std::size_t count() {
        const auto v=integer();
        if(v>=std::numeric_limits<std::size_t>::max()) throw std::runtime_error("Native mesh count overflow");
        return static_cast<std::size_t>(v);
    }
    std::size_t id() {
        const auto v=integer();
        if(v==UINT64_MAX) return constants::INVALID_ID;
        if(v>=std::numeric_limits<std::size_t>::max()) throw std::runtime_error("Native mesh ID overflow");
        return static_cast<std::size_t>(v);
    }
    double real() {
        const auto v=std::bit_cast<double>(integer());
        if(!std::isfinite(v)) throw std::runtime_error("Nonfinite native mesh value");
        return v;
    }
    math::Vector3 vector() { math::Vector3 v; for(auto& x:v) x=real(); return v; }
    std::string string() {
        const auto n=count();
        if(n>1048576 || n>remaining_) throw std::runtime_error("Invalid native mesh string length");
        std::string s(n,'\0'); bytes(s.data(),n); return s;
    }
};
}

void MeshWriter::write(const Mesh& m,const std::filesystem::path& filename) {
    MeshValidator::validate(m);
    Output out(filename);
    out.bytes(magic,8); out.integer(1,4); out.integer(1,4); // Version, triangulated metric convention.
    out.integer(static_cast<unsigned>(m.getType()),4); out.integer(m.getDimension(),4);
    out.string(m.getName()); out.integer(m.getDomain().size());
    for(auto d:m.getDomain()) out.real(d);
    out.integer(m.getPatchNames().size());
    for(const auto& [tag,name]:m.getPatchNames()) { out.id(tag); out.string(name); }
    for(auto n:{m.nodes.getNx(),m.nodes.getNy(),m.nodes.getNz(),m.cells.getNx(),m.cells.getNy(),m.cells.getNz()}) out.integer(n);
    out.integer(m.nodes.getNumber()); out.integer(m.cells.getNumber()); out.integer(m.faces.getNumber());
    for(std::size_t n=0;n<m.nodes.getNumber();++n) out.vector(m.nodes.getCoordinates(n));
    for(std::size_t c=0;c<m.cells.getNumber();++c) {
        for(auto n:m.cells.getNodes(c)) out.id(n);
        for(auto f:m.cells.getFaces(c)) out.id(f);
        out.real(m.cells.getVolume(c)); out.vector(m.cells.getCenter(c));
    }
    for(std::size_t f=0;f<m.faces.getNumber();++f) {
        for(auto n:m.faces.getNodes(f)) out.id(n);
        out.id(m.faces.getOwner(f)); out.id(m.faces.getNeighbor(f));
        out.vector(m.faces.getCenter(f)); out.real(m.faces.getArea(f));
        out.vector(m.faces.getAreaVector(f)); out.id(m.faces.getBoundaryPatchId(f));
    }
    out.close();
}

Mesh MeshReader::read(const std::filesystem::path& filename) {
    try {
        Input in(filename);
        char header[8]; in.bytes(header,8);
        if(!std::equal(std::begin(header),std::end(header),std::begin(magic))) throw std::runtime_error("Invalid native mesh identifier");
        if(in.integer(4)!=1 || in.integer(4)!=1) throw std::runtime_error("Unsupported native mesh version or metric convention");
        const auto type=in.integer(4),dimension=in.integer(4);
        if(type>4 || dimension>3) throw std::runtime_error("Invalid mesh properties");
        Mesh mesh;
        mesh.setName(in.string());
        const auto nd=in.count(); if(nd>6) throw std::runtime_error("Invalid domain parameter count");
        std::vector<double> domain(nd); for(auto& d:domain) d=in.real();
        if(type==0) {
            if(dimension!=0 || nd!=0) throw std::runtime_error("Inconsistent unspecified mesh properties");
        } else mesh.setProperties(static_cast<MeshType>(type),static_cast<unsigned>(dimension),domain);
        const auto nPatches=in.count();
        if(nPatches>in.remaining()/16) throw std::runtime_error("Invalid patch count");
        std::map<std::size_t,std::string> patches;
        for(std::size_t p=0;p<nPatches;++p) {
            const auto id=in.id(); const auto name=in.string();
            if(!patches.emplace(id,name).second) throw std::runtime_error("Duplicate patch ID");
        }
        mesh.setPatchNames(patches);
        std::array<std::size_t,6> dims; for(auto& d:dims) d=in.count();
        mesh.nodes.setDimensions(dims[0],dims[1],dims[2]); mesh.cells.setDimensions(dims[3],dims[4],dims[5]);
        const auto np=in.count(),nc=in.count(),nf=in.count();
        // Validate record sizes against the actual file before allocating entities.
        if(in.remaining()<8) throw std::runtime_error("Missing native mesh checksum");
        auto available=in.remaining()-8;
        for(auto [count,size]:{std::pair{np,24u},std::pair{nc,144u},std::pair{nf,112u}}) {
            if(count>available/size) throw std::runtime_error("Native mesh counts exceed file size");
            available-=count*size;
        }
        if(available!=0) throw std::runtime_error("Native mesh has trailing data");
        for(std::size_t n=0;n<np;++n) { const auto p=in.vector(); mesh.nodes.addCoordinates(p[0],p[1],p[2]); }
        for(std::size_t c=0;c<nc;++c) {
            std::array<std::size_t,8> nodes; for(auto& n:nodes) { n=in.id(); if(n>=np) throw std::runtime_error("Invalid cell node ID"); }
            std::array<std::size_t,6> faces; for(auto& f:faces) { f=in.id(); if(f>=nf) throw std::runtime_error("Invalid cell face ID"); }
            mesh.cells.addByNodes(nodes,c); mesh.cells.setFaces(c,faces);
            mesh.cells.setVolume(c,in.real()); mesh.cells.setCenter(c,in.vector());
        }
        for(std::size_t f=0;f<nf;++f) {
            std::array<std::size_t,4> nodes; for(auto& n:nodes) { n=in.id(); if(n>=np) throw std::runtime_error("Invalid face node ID"); }
            const auto owner=in.id(),neighbor=in.id();
            if(owner>=nc || (neighbor!=constants::INVALID_ID && neighbor>=nc)) throw std::runtime_error("Invalid face adjacency");
            mesh.faces.addByNodes(nodes,owner,neighbor);
            mesh.faces.setCenter(f,in.vector()); mesh.faces.setArea(f,in.real());
            mesh.faces.setAreaVector(f,in.vector()); mesh.faces.setBoundaryPatchId(f,in.id());
        }
        const auto digest=in.checksum();
        if(in.integer()!=digest) throw std::runtime_error("Native mesh checksum mismatch");
        MeshValidator::validate(mesh);
        return mesh;
    } catch(const std::exception& e) {
        throw std::runtime_error("Cannot read native mesh '"+filename.string()+"': "+e.what());
    }
}
}
