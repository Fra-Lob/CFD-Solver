#include <cfd/meshio/VTKReader.hpp>

namespace cfd {
namespace {

[[noreturn]] void fail(const std::string& message)
{
    throw std::runtime_error("VTKReader: " + message);
}

// A small parser for the XML subset used by ASCII VTU. No DTDs, entities,
// CDATA or binary payloads; unsupported constructs are rejected explicitly.
struct Element {
    std::string name;
    std::map<std::string, std::string> attributes;
    std::vector<Element> children;
    std::string text;

    std::string attribute(const std::string& key) const {
        const auto it = attributes.find(key);
        return it == attributes.end() ? "" : it->second;
    }

    const Element& child(const std::string& key) const {
        const Element* result = nullptr;
        for (const auto& item : children) {
            if (item.name == key) {
                if (result) fail("multiple " + key + " elements are unsupported");
                result = &item;
            }
        }
        if (!result) fail("missing " + key + " element");
        return *result;
    }
};

class XMLParser {
    std::string_view input;
    std::size_t pos = 0;

    bool starts(std::string_view token) const { return input.substr(pos, token.size()) == token; }
    void whitespace() {
        while (pos < input.size() && std::string_view(" \t\r\n").find(input[pos]) != std::string_view::npos) ++pos;
    }
    void skipUntil(std::string_view end) {
        const auto next = input.find(end, pos);
        if (next == std::string_view::npos) fail("unterminated XML comment or declaration");
        pos = next + end.size();
    }
    std::string name() {
        const auto first = pos;
        while (pos < input.size()) {
            const char c = input[pos];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == ':' || c == '-' || c == '.')) break;
            ++pos;
        }
        if (pos == first) fail("expected XML name");
        return std::string(input.substr(first, pos - first));
    }
    Element element(unsigned depth) {
        if (depth > 32) fail("XML nesting is too deep");
        if (!starts("<")) fail("expected opening XML tag");
        ++pos;
        Element result;
        result.name = name();
        for (;;) {
            const auto before = pos;
            whitespace();
            if (starts("/>")) { pos += 2; return result; }
            if (starts(">")) { ++pos; break; }
            if (pos == before) fail("expected whitespace before attribute");
            const auto key = name();
            whitespace();
            if (!starts("=")) fail("expected attribute assignment");
            ++pos;
            whitespace();
            if (!starts("\"") && !starts("'")) fail("expected quoted attribute");
            const char quote = input[pos++];
            const auto end = input.find(quote, pos);
            if (end == std::string_view::npos) fail("unterminated attribute");
            const auto value = input.substr(pos, end - pos);
            if (value.find_first_of("<&") != std::string_view::npos) fail("XML entities are unsupported");
            if (!result.attributes.emplace(key, std::string(value)).second) fail("duplicate attribute " + key);
            pos = end + 1;
        }
        for (;;) {
            if (pos == input.size()) fail("unclosed " + result.name + " element");
            if (starts("<!--")) { skipUntil("-->"); continue; }
            if (starts("</")) {
                pos += 2;
                if (name() != result.name) fail("mismatched closing tag");
                whitespace();
                if (!starts(">")) fail("invalid closing tag");
                ++pos;
                return result;
            }
            if (starts("<!") || starts("<?")) fail("unsupported XML construct");
            if (starts("<")) { result.children.push_back(element(depth + 1)); continue; }
            const auto end = input.find('<', pos);
            if (end == std::string_view::npos) fail("truncated XML document");
            const auto value = input.substr(pos, end - pos);
            if (value.find('&') != std::string_view::npos) fail("XML entities are unsupported");
            result.text.append(value);
            pos = end;
        }
    }
public:
    explicit XMLParser(std::string_view source) : input(source) {}
    Element parse() {
        if (starts("\xEF\xBB\xBF")) pos += 3; // UTF-8 BOM
        whitespace();
        if (starts("<?xml")) skipUntil("?>");
        whitespace();
        while (starts("<!--")) { skipUntil("-->"); whitespace(); }
        auto root = element(0);
        whitespace();
        while (starts("<!--")) { skipUntil("-->"); whitespace(); }
        if (pos != input.size()) fail("unexpected content after root element");
        return root;
    }
};

std::size_t integer(const std::string& token)
{
    std::size_t result = 0;
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), result);
    if (error != std::errc{} || end != token.data() + token.size() || token.empty())
        fail("invalid nonnegative integer: " + token);
    return result;
}

std::size_t product(std::size_t count, std::size_t components)
{
    if (count > std::numeric_limits<std::size_t>::max() / components) fail("array size overflow");
    return count * components;
}

void checkArray(const Element& array, std::size_t components, bool floating)
{
    if (array.name != "DataArray" || !array.children.empty()) fail("expected numeric DataArray");
    if (array.attribute("format") != "ascii") fail("only ASCII DataArrays are supported");
    const auto n = array.attribute("NumberOfComponents");
    if ((n.empty() ? 1 : integer(n)) != components) fail("incorrect NumberOfComponents");
    const auto type = array.attribute("type");
    const std::string types = floating ? " Float32 Float64 " : " Int8 UInt8 Int16 UInt16 Int32 UInt32 Int64 UInt64 ";
    if (type.empty() || types.find(" " + type + " ") == std::string::npos) fail("unsupported DataArray type: " + type);
}

template<class T>
std::vector<T> values(const Element& array, std::size_t count)
{
    std::istringstream stream(array.text);
    stream.imbue(std::locale::classic());
    std::vector<T> result;
    std::string token;
    while (stream >> token) {
        if (result.size() == count) fail("too many values in DataArray " + array.attribute("Name"));
        if constexpr (std::is_same_v<T, std::size_t>) {
            result.push_back(integer(token));
        } else {
            std::istringstream number(token);
            number.imbue(std::locale::classic());
            double value;
            if (!(number >> value) || number.peek() != std::char_traits<char>::eof() || !std::isfinite(value))
                fail("invalid or nonfinite coordinate");
            result.push_back(value);
        }
    }
    if (result.size() != count) fail("incorrect value count in DataArray " + array.attribute("Name"));
    return result;
}

const Element* findArray(const Element& cells, const std::string& name)
{
    const Element* result = nullptr;
    for (const auto& array : cells.children) {
        if (array.name == "DataArray" && array.attribute("Name") == name) {
            if (result) fail("duplicate DataArray " + name);
            result = &array;
        }
    }
    return result;
}

const Element& namedArray(const Element& cells, const std::string& name)
{
    const auto result = findArray(cells, name);
    if (!result) fail("missing DataArray " + name);
    checkArray(*result, 1, false);
    return *result;
}

void readProperties(const Element& grid, Mesh& mesh)
{
    const Element* field = nullptr;
    for (const auto& child : grid.children) {
        if (child.name == "FieldData") {
            if (field) fail("duplicate FieldData");
            field = &child;
        }
    }
    if (!field) return; // Older VTU files have no domain metadata.
    const auto type = findArray(*field, "cfd_mesh_type");
    const auto dimension = findArray(*field, "cfd_mesh_dimension");
    const auto domain = findArray(*field, "cfd_mesh_domain");
    if (!type && !dimension && !domain) return;
    if (!type || !dimension || !domain) fail("incomplete mesh properties");
    checkArray(*type, 1, false);
    checkArray(*dimension, 1, false);
    const auto t = values<std::size_t>(*type, 1)[0];
    const auto d = values<std::size_t>(*dimension, 1)[0];
    if (t < 1 || t > 3 || d < 1 || d > 3) fail("invalid mesh type or dimension");
    const auto components=integer(domain->attribute("NumberOfComponents"));
    if(components<1 || components>6) fail("invalid domain parameter count");
    checkArray(*domain, components, true);
    mesh.setProperties(static_cast<MeshType>(t), static_cast<unsigned>(d), values<double>(*domain, components));
    mesh.requireHex3D();
}

// VTU has no structured dimensions. Recover them only after checking the
// complete connectivity against the generator's i-fastest indexing convention.
void recoverDimensions(Mesh& mesh)
{
    if (mesh.cells.getNumber() == 0) return;
    const auto first = mesh.cells.getNodes(0);
    const auto nx = first[3];
    if (nx < 2 || first[4] % nx != 0) return;
    const auto ny = first[4] / nx;
    if (ny < 2 || first[4] > mesh.nodes.getNumber() || mesh.nodes.getNumber() % first[4] != 0) return;
    const auto nz = mesh.nodes.getNumber() / first[4];
    if (nz < 2 || product(product(nx - 1, ny - 1), nz - 1) != mesh.cells.getNumber()) return;
    for (std::size_t c = 0; c < mesh.cells.getNumber(); ++c) {
        const auto i = c % (nx - 1);
        const auto j = (c / (nx - 1)) % (ny - 1);
        const auto k = c / ((nx - 1) * (ny - 1));
        const auto p = i + nx * (j + ny * k);
        const std::array<std::size_t, 8> expected = {
            p, p + 1, p + nx + 1, p + nx,
            p + nx * ny, p + nx * ny + 1, p + nx * ny + nx + 1, p + nx * ny + nx
        };
        if (mesh.cells.getNodes(c) != expected) return;
    }
    mesh.nodes.setDimensions(nx, ny, nz);
    mesh.cells.setDimensions(nx - 1, ny - 1, nz - 1);
}

} // namespace

Mesh VTKReader::read(const std::string& filename)
{
    try {
        std::ifstream file(filename);
        if (!file) fail("cannot open file");
        const std::string xml{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        if (file.bad()) fail("failed reading file");
        const auto root = XMLParser(xml).parse();
        if (root.name != "VTKFile" || root.attribute("type") != "UnstructuredGrid")
            fail("expected VTK UnstructuredGrid");
        const auto& grid = root.child("UnstructuredGrid");
        const auto& piece = grid.child("Piece");
        const auto numberOfPoints = integer(piece.attribute("NumberOfPoints"));
        const auto numberOfCells = integer(piece.attribute("NumberOfCells"));
        const auto& points = piece.child("Points").child("DataArray");
        checkArray(points, 3, true);
        const auto coordinates = values<double>(points, product(numberOfPoints, 3));
        const auto& cells = piece.child("Cells");
        const auto connectivity = values<std::size_t>(namedArray(cells, "connectivity"), product(numberOfCells, 8));
        const auto offsets = values<std::size_t>(namedArray(cells, "offsets"), numberOfCells);
        const auto types = values<std::size_t>(namedArray(cells, "types"), numberOfCells);

        Mesh mesh;
        readProperties(grid, mesh);
        for (std::size_t p = 0; p < numberOfPoints; ++p)
            mesh.nodes.addCoordinates(coordinates[3 * p], coordinates[3 * p + 1], coordinates[3 * p + 2]);
        for (std::size_t c = 0; c < numberOfCells; ++c) {
            if (types[c] != 12) fail("only VTK_HEXAHEDRON (type 12) cells are supported");
            if (offsets[c] != (c + 1) * 8) fail("invalid hexahedron connectivity offset");
            std::array<std::size_t, 8> nodes;
            std::copy_n(connectivity.begin() + c * 8, 8, nodes.begin());
            auto sorted = nodes;
            std::sort(sorted.begin(), sorted.end());
            if (sorted.back() >= numberOfPoints) fail("cell references a nonexistent node");
            if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) fail("cell has repeated nodes");
            mesh.cells.addByNodes(nodes, c);
        }
        recoverDimensions(mesh);
        MeshConnectivityBuilder::build(mesh);
        MeshGeometryProcessor::compute(mesh);
        if (mesh.hasProperties()) MeshBoundaryPatchBuilder::build(mesh);
        return mesh;
    } catch (const std::exception& error) {
        throw std::runtime_error("Cannot read VTU file '" + filename + "': " + error.what());
    }
}

Mesh VTKReader::read(const std::string& filename, MeshType type,
                     unsigned dimension, const std::vector<double>& domain)
{
    Mesh properties(type, dimension, domain);
    properties.requireHex3D();
    auto mesh = read(filename);
    if (mesh.hasProperties()) {
        if (mesh.getType() != type || mesh.getDimension() != dimension || mesh.getDomain() != domain)
            throw std::runtime_error("VTKReader: supplied mesh properties disagree with " + filename);
    } else {
        mesh.setProperties(type, dimension, domain);
        MeshBoundaryPatchBuilder::build(mesh);
    }
    return mesh;
}

}
