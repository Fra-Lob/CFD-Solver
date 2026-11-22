#include <cfd/meshio/MeshConverter.hpp>

#include <cfd/mesh/MeshBuilder.hpp>

#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& filename, const std::string& message)
{
    throw std::runtime_error("MeshConverter: " + filename + ": " + message);
}

void expect(std::istream& input, const std::string& expected, const std::string& filename)
{
    std::string token;
    if (!(input >> token) || token != expected) {
        fail(filename, "expected '" + expected + "'");
    }
}

std::string readToken(std::istream& input, const std::string& filename)
{
    std::string token;
    if (!(input >> token)) fail(filename, "unexpected end of file");
    return token;
}

std::size_t readSize(std::istream& input, const std::string& filename)
{
    const auto token = readToken(input, filename);
    std::size_t value = 0;
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc{} || end != token.data() + token.size()) {
        fail(filename, "invalid nonnegative integer '" + token + "'");
    }
    return value;
}

double readDouble(std::istream& input, const std::string& filename)
{
    const auto token = readToken(input, filename);
    std::istringstream parser(token);
    parser.imbue(std::locale::classic());
    double value = 0.0;
    if (!(parser >> value) || parser.peek() != std::char_traits<char>::eof() || !std::isfinite(value)) {
        fail(filename, "invalid or nonfinite floating-point value '" + token + "'");
    }
    return value;
}

void validate(const Mesh& mesh)
{
    const auto domain = mesh.getDomain();
    for (const auto length : domain) {
        if (!std::isfinite(length)) {
            throw std::invalid_argument("MeshConverter: domain lengths must be finite");
        }
    }
    for (std::size_t node = 0; node < mesh.nodes.getNumber(); ++node) {
        for (const auto coordinate : mesh.nodes.getCoordinates(node)) {
            if (!std::isfinite(coordinate)) {
                throw std::invalid_argument("MeshConverter: node coordinates must be finite");
            }
        }
    }
    for (std::size_t cell = 0; cell < mesh.cells.getNumber(); ++cell) {
        for (const auto node : mesh.cells.getNodes(cell)) {
            if (node >= mesh.nodes.getNumber()) {
                throw std::invalid_argument("MeshConverter: cell references a nonexistent node");
            }
        }
        for (const auto face : mesh.cells.getFaces(cell)) {
            if (face == constants::invalidId || face >= mesh.faces.getNumber()) {
                throw std::invalid_argument("MeshConverter: connectivity must be built before writing");
            }
        }
    }
}

} // namespace

void MeshConverter::write(const Mesh& mesh, const std::string& filename)
{
    validate(mesh);
    std::ofstream file(filename);
    if (!file) throw std::runtime_error("MeshConverter: cannot open file: " + filename);
    file.imbue(std::locale::classic());
    file << std::setprecision(std::numeric_limits<double>::max_digits10);

    const auto domain = mesh.getDomain();
    file << "CFD_MESH 1\n"
         << "DOMAIN " << domain[0] << ' ' << domain[1] << ' ' << domain[2] << '\n'
         << "DIMENSIONS "
         << mesh.cells.getNx() << ' ' << mesh.cells.getNy() << ' ' << mesh.cells.getNz() << ' '
         << mesh.nodes.getNx() << ' ' << mesh.nodes.getNy() << ' ' << mesh.nodes.getNz() << '\n'
         << "NODES " << mesh.nodes.getNumber() << '\n';
    for (std::size_t node = 0; node < mesh.nodes.getNumber(); ++node) {
        const auto xyz = mesh.nodes.getCoordinates(node);
        file << xyz[0] << ' ' << xyz[1] << ' ' << xyz[2] << '\n';
    }

    file << "CELLS " << mesh.cells.getNumber() << '\n';
    for (std::size_t cell = 0; cell < mesh.cells.getNumber(); ++cell) {
        for (const auto node : mesh.cells.getNodes(cell)) file << node << ' ';
        file << '\n';
    }

    file << "BOUNDARY_PATCHES " << mesh.faces.getNumber() << '\n';
    for (std::size_t face = 0; face < mesh.faces.getNumber(); ++face) {
        file << mesh.faces.getBoundaryPatchId(face) << '\n';
    }
    file << "END\n";
    file.close();
    if (!file) throw std::runtime_error("MeshConverter: failed writing file: " + filename);
}

Mesh MeshConverter::read(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("MeshConverter: cannot open file: " + filename);

    expect(file, "CFD_MESH", filename);
    if (readSize(file, filename) != 1) fail(filename, "unsupported mesh file version");
    expect(file, "DOMAIN", filename);
    const std::array<double, 3> domain = {
        readDouble(file, filename), readDouble(file, filename), readDouble(file, filename)
    };

    expect(file, "DIMENSIONS", filename);
    const auto cellNx = readSize(file, filename);
    const auto cellNy = readSize(file, filename);
    const auto cellNz = readSize(file, filename);
    const auto nodeNx = readSize(file, filename);
    const auto nodeNy = readSize(file, filename);
    const auto nodeNz = readSize(file, filename);

    expect(file, "NODES", filename);
    const auto nodeCount = readSize(file, filename);
    Mesh mesh;
    mesh.setDomain(domain[0], domain[1], domain[2]);
    mesh.nodes.setDimensions(nodeNx, nodeNy, nodeNz);
    mesh.cells.setDimensions(cellNx, cellNy, cellNz);
    for (std::size_t node = 0; node < nodeCount; ++node) {
        const auto x = readDouble(file, filename);
        const auto y = readDouble(file, filename);
        const auto z = readDouble(file, filename);
        mesh.nodes.addCoordinates(x, y, z);
    }

    expect(file, "CELLS", filename);
    const auto cellCount = readSize(file, filename);
    if (cellCount > std::numeric_limits<std::size_t>::max() / 8) {
        fail(filename, "cell connectivity size overflows");
    }
    for (std::size_t cell = 0; cell < cellCount; ++cell) {
        std::array<std::size_t, 8> nodes{};
        for (auto& node : nodes) node = readSize(file, filename);
        mesh.cells.addByNodes(nodes, cell);
    }

    expect(file, "BOUNDARY_PATCHES", filename);
    const auto patchCount = readSize(file, filename);
    std::vector<std::size_t> patches;
    patches.reserve(patchCount);
    for (std::size_t face = 0; face < patchCount; ++face) {
        patches.push_back(readSize(file, filename));
    }
    expect(file, "END", filename);
    if (file >> std::ws && !file.eof()) fail(filename, "unexpected content after END");

    MeshBuilder::computeConnectivity(mesh);
    if (mesh.faces.getNumber() != patchCount) {
        fail(filename, "boundary patch count does not match reconstructed topology");
    }
    MeshBuilder::computeFullGeometry(mesh);
    for (std::size_t face = 0; face < patchCount; ++face) {
        if (patches[face] != constants::invalidId && !mesh.faces.isBoundary(face)) {
            fail(filename, "internal face has a boundary patch");
        }
        if (mesh.faces.isBoundary(face)) mesh.faces.setBoundaryPatchId(face, patches[face]);
    }
    return mesh;
}