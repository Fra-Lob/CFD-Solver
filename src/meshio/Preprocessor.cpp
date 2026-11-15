#include <cfd/meshio/Preprocessor.hpp>
#include <cfd/meshio/GridConfigReader.hpp>
#include <cfd/meshio/GmshImporter.hpp>
#include <cfd/meshio/MeshWriter.hpp>
#include <cfd/meshio/VTKWriter.hpp>
#include <cfd/mesh/ConnectivityBuilder.hpp>
#include <cfd/mesh/GeometryProcessor.hpp>
#include <cfd/mesh/BoundaryPatchBuilder.hpp>
#include <cfd/mesh/MeshValidator.hpp>
#include <chrono>
#include <ostream>

namespace cfd {
Mesh Preprocessor::run(const std::filesystem::path& caseDirectory, std::ostream* progress) {
    namespace fs=std::filesystem;
    const auto log=[&](const std::string& message) {
        if(progress) *progress << message << std::endl;
    };
    const auto directory=fs::absolute(caseDirectory);
    log("[1/8] Reading configuration: "+(directory/"grid.info").string());
    const auto c=GridConfigReader::read(directory);
    log("      Configuration loaded for domain '"+c.name+"'.");
    Mesh mesh;
    if(c.type==MeshType::Custom) {
        log("[2-5/8] Importing Gmsh nodes, cells, connectivity, geometry and physical patches: "+(directory/(c.name+".msh")).string());
        mesh=GmshImporter::read(directory/(c.name+".msh"));
        log("        Gmsh import completed.");
    }
    else {
        log("[2/8] Generating mesh nodes and hexahedral cells...");
        mesh=MeshGenerator::generate(c);
        log("      Generated "+std::to_string(mesh.nodes.getNumber())+" nodes and "+std::to_string(mesh.cells.getNumber())+" cells.");
        log("[3/8] Building faces and owner/neighbour connectivity...");
        MeshConnectivityBuilder::build(mesh);
        log("      Built "+std::to_string(mesh.faces.getNumber())+" unique faces.");
        log("[4/8] Computing face areas, area vectors, cell volumes and centroids...");
        MeshGeometryProcessor::compute(mesh);
        log("      Geometry calculations completed.");
        log("[5/8] Assigning boundary patches...");
        MeshBoundaryPatchBuilder::build(mesh);
        log("      Assigned "+std::to_string(mesh.getPatchNames().size())+" named patches.");
    }
    mesh.setName(c.name);
    log("[6/8] Validating topology, geometry, orientations, closure and patches...");
    MeshValidator::validate(mesh);
    log("      Mesh validation passed.");
    const std::array<fs::path,2> targets={directory/(c.name+".vtu"),directory/(c.name+".mesh")};
    for(const auto& target:targets)
        if(fs::exists(target) && (!fs::is_regular_file(target) || fs::is_symlink(target)))
            throw std::runtime_error("Output is not a regular file: "+target.string());
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path stage;
    for(unsigned attempt=0;attempt<100;++attempt) {
        stage=directory/(".preprocessor-"+std::to_string(stamp)+"-"+std::to_string(attempt));
        if(fs::create_directory(stage)) break;
        if(attempt==99) throw std::runtime_error("Cannot create output staging directory");
    }
    const std::array<fs::path,2> staged={stage/"new.vtu",stage/"new.mesh"};
    const std::array<fs::path,2> backups={stage/"old.vtu",stage/"old.mesh"};
    std::array<bool,2> saved{},installed{};
    const auto cleanup=[&] {
        std::error_code ignored;
        for(const auto& p:staged) fs::remove(p,ignored);
        for(const auto& p:backups) fs::remove(p,ignored);
        fs::remove(stage,ignored); // Nonrecursive; only our known staging files.
    };
    try {
        log("[7/8] Preparing VTU and native mesh output files...");
        log("      Writing ASCII VTU: "+targets[0].string());
        VTKWriter::write(mesh,staged[0].string());
        log("      VTU prepared. Writing native mesh: "+targets[1].string());
        MeshWriter::write(mesh,staged[1]);
        log("      Native mesh prepared.");
        log("[8/8] Installing the completed output files...");
        for(unsigned i=0;i<2;++i) if(fs::exists(targets[i])) {
            fs::rename(targets[i],backups[i]); saved[i]=true;
        }
        for(unsigned i=0;i<2;++i) { fs::rename(staged[i],targets[i]); installed[i]=true; }
    } catch(...) {
        // On ordinary I/O failure restore both old outputs. If restoration fails,
        // retain backups and report their location instead of deleting them.
        try {
            for(unsigned i=0;i<2;++i) if(installed[i]) fs::remove(targets[i]);
            for(unsigned i=0;i<2;++i) if(saved[i]) fs::rename(backups[i],targets[i]);
        } catch(...) { throw std::runtime_error("Output rollback failed; recover backups from "+stage.string()); }
        cleanup(); throw;
    }
    cleanup();
    log("      Both output files saved. Preprocessing completed.");
    return mesh;
}
}
