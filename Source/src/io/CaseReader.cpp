#include <io/CaseReader.hpp>

#include <mesh/MeshBuilder.hpp>
#include <meshio/MeshConverter.hpp>

#include <charconv>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {

    std::string trim(std::string value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return {};
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    std::vector<std::string> split(const std::string& value) {
        std::vector<std::string> result;
        std::istringstream stream(value);
        std::string item;
        while (std::getline(stream, item, ',')) {
            item = trim(item);
            if (!item.empty()) result.push_back(item);
        }
        return result;
    }

    double parseDouble(const std::string& text) {
        std::istringstream stream(text);
        stream.imbue(std::locale::classic());
        double value = 0.0;
        if (!(stream >> value) || stream.peek() != std::char_traits<char>::eof()
            || !std::isfinite(value)) {
            throw std::runtime_error("Invalid floating-point value '" + text + "'");
        }
        return value;
    }

    std::size_t parseSize(const std::string& text) {
        std::size_t value = 0;
        const auto [end, error] = std::from_chars(
            text.data(),
            text.data() + text.size(),
            value
        );
        if (error != std::errc{} || end != text.data() + text.size()) {
            throw std::runtime_error("Invalid nonnegative integer '" + text + "'");
        }
        return value;
    }

    bool parseBool(const std::string& text) {
        const auto value = trim(text);
        if (value == "true" || value == "True" || value == "TRUE" || value == "1") return true;
        if (value == "false" || value == "False" || value == "FALSE" || value == "0") return false;
        throw std::runtime_error("Invalid boolean value '" + text + "'");
    }

    std::size_t parsePatch(const std::string& name) {
        if (name == "xmin") return static_cast<std::size_t>(CartesianPatch::XMin);
        if (name == "xmax") return static_cast<std::size_t>(CartesianPatch::XMax);
        if (name == "ymin") return static_cast<std::size_t>(CartesianPatch::YMin);
        if (name == "ymax") return static_cast<std::size_t>(CartesianPatch::YMax);
        if (name == "zmin") return static_cast<std::size_t>(CartesianPatch::ZMin);
        if (name == "zmax") return static_cast<std::size_t>(CartesianPatch::ZMax);
        throw std::runtime_error("Unknown Cartesian boundary patch '" + name + "'");
    }

    BoundaryType parseBoundaryType(const std::string& text) {
        if (text == "Dirichlet") return BoundaryType::Dirichlet;
        if (text == "Neumann") return BoundaryType::Neumann;
        throw std::runtime_error("Unknown boundary type '" + text + "'");
    }

    std::filesystem::path findInfoFile() {
        const auto current = std::filesystem::current_path();
        const auto preferred = current / "simulation.info";
        if (std::filesystem::is_regular_file(preferred)) {
            return preferred;
        }

        std::filesystem::path discovered;
        for (const auto& entry : std::filesystem::directory_iterator(current)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".info") continue;
            if (!discovered.empty()) {
                throw std::runtime_error(
                    "Multiple .info files found; expected one case file"
                );
            }
            discovered = entry.path();
        }
        if (discovered.empty()) {
            throw std::runtime_error(
                "No .info file found in " + current.string()
            );
        }
        return discovered;
    }

} // namespace

CaseReader::LoadedCase CaseReader::load() {
    return load(findInfoFile().string());
}

CaseReader::LoadedCase CaseReader::load(const std::string& filename) {
    const auto base = read(filename);

    std::filesystem::path meshFile(base.meshFile);
    if (meshFile.extension().empty()) meshFile += ".mesh";
    if (meshFile.is_relative()) {
        meshFile = std::filesystem::absolute(filename).parent_path() / meshFile;
    }

    if (!std::filesystem::is_regular_file(meshFile)) {
        throw std::runtime_error(
            "Cannot open mesh file '" + meshFile.string() +
            "' referenced by case '" + filename + "'"
        );
    }

    Mesh mesh = MeshConverter::read(meshFile.string());
    SimulationConfig config = read(filename, mesh.cells.getNumber());
    return {config, std::move(mesh)};
}

SimulationConfig CaseReader::read(const std::string& filename)
{
    return read(filename, 0);
}

SimulationConfig CaseReader::read(const std::string& filename, std::size_t cellCount)
{
    try {
        return readImpl(filename, cellCount);
    } catch (const std::exception& error) {
        throw std::runtime_error(
            "Cannot read case file '" + filename + "': " + error.what()
        );
    }
}

SimulationConfig CaseReader::readImpl(const std::string& filename, std::size_t cellCount)
{
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Cannot open case file '" + filename + "'");

    SimulationConfig config;
    std::unordered_map<std::string, std::string> values;
    std::unordered_map<std::size_t,
        std::pair<std::vector<std::string>, std::vector<double>>> boundaryValues;
    std::string section;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#') continue;

        if (line.find('=') == std::string::npos) {
            section = line;
            continue;
        }

        const auto separator = line.find('=');
        const auto key = trim(line.substr(0, separator));
        const auto value = trim(line.substr(separator + 1));

        if (key == "directory" || key == "vtk") {
            values[key] = value;
        } else if (section == "xmin" || section == "xmax" || section == "ymin"
            || section == "ymax" || section == "zmin" || section == "zmax") {
            const auto patch = parsePatch(section);
            if (key == "type") {
                boundaryValues[patch].first = split(value);
            } else if (key == "value") {
                boundaryValues[patch].second.clear();
                for (const auto& item : split(value)) {
                    boundaryValues[patch].second.push_back(parseDouble(item));
                }
            }
        } else {
            values[key] = value;
        }
    }
    file.close();
    if (file.bad()) throw std::runtime_error("Failed reading case file '" + filename + "'");

    try {
        config.name = values.at("name");
        config.type = values.at("type");
        config.meshFile = values.at("mesh");
        config.mode = values.at("mode");
        config.linearSolver = values.at("method");
        config.tolerance = parseDouble(values.at("tolerance"));
        config.maxIterations = parseSize(values.at("maxIterations"));
        config.transient = config.mode != "steady";
        if (values.count("timeStep")) {
            config.timeStep = parseDouble(values.at("timeStep"));
        }
        if (values.count("endTime")) {
            config.endTime = parseDouble(values.at("endTime"));
        }
        config.outputDirectory = values.count("directory") ? values.at("directory") : "results";
        config.writeVtk = values.count("vtk") ? parseBool(values.at("vtk")) : false;
    } catch (const std::out_of_range& error) {
        throw std::runtime_error(
            "Missing required key '" + std::string(error.what()) +
            "' in case file '" + filename + "'"
        );
    }

    if (!values.count("scalar fields") || !values.count("diffusivity")
        || !values.count("initialValues")) {
        throw std::runtime_error("Missing scalar-field configuration in '" + filename + "'");
    }

    const auto fieldNames = split(values.at("scalar fields"));
    const auto diffusivities = split(values.at("diffusivity"));
    const auto initialValues = split(values.at("initialValues"));
    if (fieldNames.empty()) throw std::runtime_error("No scalar fields were configured");
    if (diffusivities.size() != fieldNames.size()) {
        throw std::runtime_error("diffusivity count does not match scalar-field count");
    }
    if (initialValues.size() != fieldNames.size()) {
        throw std::runtime_error("initialValues count does not match scalar-field count");
    }

    config.scalarFields.reserve(fieldNames.size());
    for (std::size_t field = 0; field < fieldNames.size(); ++field) {
        const double initialValue = parseDouble(initialValues[field]);
        const double diffusivity = parseDouble(diffusivities[field]);

        ScalarField scalarField(fieldNames[field], cellCount, initialValue);
        scalarField.fill(initialValue);
        scalarField.setDiffusivity(diffusivity);

        BoundaryConditions boundaryConditions;

        for (const auto& [patch, boundary] : boundaryValues) {
            if (boundary.first.size() != fieldNames.size()
                || boundary.second.size() != fieldNames.size()) {
                throw std::runtime_error(
                    "Boundary type/value count does not match scalar-field count"
                );
            }
            const auto type = parseBoundaryType(trim(boundary.first[field]));
            boundaryConditions.set(
                patch,
                type,
                boundary.second[field]
            );
        }
        scalarField.setBoundaryConditions(boundaryConditions);

        config.scalarFields.push_back(std::move(scalarField));
    }

    return config;
}
