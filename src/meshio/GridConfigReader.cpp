#include <cfd/meshio/GridConfigReader.hpp>
#include <charconv>
#include <fstream>
#include <locale>
#include <set>
#include <sstream>

namespace cfd {
namespace {
std::string trim(const std::string& s) {
    const auto first=s.find_first_not_of(" \t\r\n");
    return first==std::string::npos ? "" : s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);
}
}
MeshConfig GridConfigReader::read(const std::filesystem::path& directory) {
    const auto path=directory/"grid.info";
    std::ifstream file(path);
    if(!file) throw std::runtime_error("Cannot open "+path.string());
    std::map<std::string,std::string> fields;
    std::string line;
    std::size_t lineNumber=0;
    while(std::getline(file,line)) {
        ++lineNumber;
        line=line.substr(0,line.find_first_of("#;"));
        line=trim(line);
        if(line.empty()) continue;
        const auto equal=line.find('=');
        if(equal==std::string::npos || line.find('=',equal+1)!=std::string::npos)
            throw std::runtime_error("grid.info line "+std::to_string(lineNumber)+": expected key = value");
        const auto key=trim(line.substr(0,equal)), value=trim(line.substr(equal+1));
        if(key.empty() || value.empty() || !fields.emplace(key,value).second)
            throw std::runtime_error("grid.info: empty or duplicate field at line "+std::to_string(lineNumber));
    }
    if(file.bad()) throw std::runtime_error("Error reading "+path.string());
    std::set<std::string> used;
    const auto get=[&](const std::string& key) {
        used.insert(key);
        if(!fields.contains(key)) throw std::runtime_error("grid.info: missing "+key);
        return fields.at(key);
    };
    const auto real=[&](const std::string& key) {
        std::istringstream stream(get(key)); stream.imbue(std::locale::classic());
        double value;
        if(!(stream>>value) || stream.peek()!=std::char_traits<char>::eof() || !std::isfinite(value))
            throw std::runtime_error("grid.info: invalid real number for "+key);
        return value;
    };
    const auto count=[&](const std::string& key) {
        const auto s=get(key); std::size_t value=0;
        const auto [end,error]=std::from_chars(s.data(),s.data()+s.size(),value);
        if(error!=std::errc{} || end!=s.data()+s.size() || value==0)
            throw std::runtime_error("grid.info: invalid positive integer for "+key);
        return value;
    };
    MeshConfig c;
    c.name=get("name");
    if(c.name=="." || c.name==".." || c.name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.")!=std::string::npos)
        throw std::runtime_error("grid.info: name must be a plain file stem");
    const auto type=get("type");
    c.dimension=3;
    const double rad=std::numbers::pi/180.0;
    if(type=="cartesian") {
        c.type=MeshType::Cartesian; c.domain={real("Lx"),real("Ly"),real("Lz")};
        c.Nx=count("Nx"); c.Ny=count("Ny"); c.Nz=count("Nz");
    } else if(type=="cylindrical") {
        c.type=MeshType::Cylindrical;
        c.domain={real("rInner"),real("rOuter"),real("length"),rad*real("thetaStart"),rad*real("thetaEnd")};
        c.Nx=count("Nr"); c.Ny=count("Ntheta"); c.Nz=count("Nz");
    } else if(type=="spherical") {
        c.type=MeshType::Spherical;
        c.domain={real("rInner"),real("rOuter"),rad*real("polarStart"),rad*real("polarEnd"),rad*real("azimuthStart"),rad*real("azimuthEnd")};
        c.Nx=count("Nr"); c.Ny=count("Ntheta"); c.Nz=count("Nphi");
    } else if(type=="custom") c.type=MeshType::Custom;
    else throw std::runtime_error("grid.info: unsupported type "+type);
    for(const auto& [key,value]:fields) if(!used.contains(key)) throw std::runtime_error("grid.info: unknown field "+key);
    if(c.type!=MeshType::Custom) MeshGenerator::validate(c);
    return c;
}
}
