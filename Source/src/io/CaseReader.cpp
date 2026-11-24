#include "fields/VectorField.hpp"
#include <io/CaseReader.hpp>

#include <mesh/MeshBuilder.hpp>
#include <meshio/MeshConverter.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

// ====================================================================================== //
namespace {

    // Removing all blancks in a string
    std::string trim(std::string value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return {};
        const auto last = value.find_last_not_of(" \t\r\n");
        // substring extraction
        return value.substr(first, last - first + 1);
    }

    // String normalization (single space separation)
    std::string normalizeKey(const std::string& value) {
        std::istringstream input(trim(value));
        std::string word;
        std::string result;
        while (input >> word) {
            if (!result.empty()) result += ' ';
            result += word;
        }
        return result;
    }

    // Dividing comma-separated strings into lists (1, 2 -> {1, 2})
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

    // String to double, with validation
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

    // String to size_t (non-negative integer)
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

    // String to bool
    bool parseBool(const std::string& text) {
        const auto value = trim(text);
        if (value == "true" || value == "True" || value == "TRUE" || value == "1") return true;
        if (value == "false" || value == "False" || value == "FALSE" || value == "0") return false;
        throw std::runtime_error("Invalid boolean value '" + text + "'");
    }

    // Output configuration extraction (format: true/false, <saving_interval>)
    void parseOutputSetting(const std::string& text,
                            const std::string& name,
                                  bool&        enabled,
                                  std::size_t& interval) {
        std::string normalized = text;
        std::replace(normalized.begin(), normalized.end(), ',', ' ');
        std::istringstream input(normalized);
        std::vector<std::string> tokens;
        std::string token;
        while (input >> token) tokens.push_back(token);
        if (tokens.empty() || tokens.size() > 2) {
            throw std::runtime_error(
                "Expected '" + name + " = <bool>, <interval>'"
            );
        }
        enabled = parseBool(tokens[0]); // output activation
        if (!enabled) {
            interval = 0;
            return;
        }
        if (tokens.size() != 2) {
            throw std::runtime_error(
                "Enabled " + name + " output requires a save interval"
            );
        }
        interval = parseSize(tokens[1]); // output save interval
        if (interval == 0) {
            throw std::runtime_error(name + " output interval must be positive");
        }
    }

    // Reader for points coordinates, string -> array3 (format: (x, y, z))
    std::vector<std::array<double, 3>> parsePoints(const std::string& text) {
        std::vector<std::array<double, 3>> points;
        std::size_t position = 0;
        while (position < text.size()) {
            while (position < text.size()
                   && (text[position] == ' ' || text[position] == '\t'
                       || text[position] == ',')) {
                ++position;
            }
            if (position == text.size()) break;
            if (text[position] != '(') {
                throw std::runtime_error("Expected '(' in CSV point list");
            }
            const auto end = text.find(')', position + 1);
            if (end == std::string::npos) {
                throw std::runtime_error("Missing ')' in CSV point list");
            }
            const auto coordinates = split(text.substr(position + 1, end - position - 1));
            if (coordinates.size() != 3) {
                throw std::runtime_error("Each CSV point must have three coordinates");
            }
            points.push_back({
                parseDouble(coordinates[0]),
                parseDouble(coordinates[1]),
                parseDouble(coordinates[2])
            });
            position = end + 1;
        }
        return points;
    }

    // Reader for Boundary Patches
    std::size_t parsePatch(const std::string& name) {
        if (name == "xmin") return static_cast<std::size_t>(CartesianPatch::XMin);
        if (name == "xmax") return static_cast<std::size_t>(CartesianPatch::XMax);
        if (name == "ymin") return static_cast<std::size_t>(CartesianPatch::YMin);
        if (name == "ymax") return static_cast<std::size_t>(CartesianPatch::YMax);
        if (name == "zmin") return static_cast<std::size_t>(CartesianPatch::ZMin);
        if (name == "zmax") return static_cast<std::size_t>(CartesianPatch::ZMax);
        throw std::runtime_error("Unknown Cartesian boundary patch '" + name + "'");
    }

    // Reader for Boundary Types
    BoundaryType parseBoundaryType(const std::string& text) {
        if (text == "Dirichlet") return BoundaryType::Dirichlet;
        if (text == "Neumann") return BoundaryType::Neumann;
        throw std::runtime_error("Unknown boundary type '" + text + "'");
    }

    // File finder
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

} 
// ====================================================================================== //

// Loader (default)
CaseReader::LoadedCase CaseReader::load() {
    return load(findInfoFile().string());
}

// Loader (with filename)
CaseReader::LoadedCase CaseReader::load(const std::string& filename) {
    OutputConfig output;
    const auto base = read(filename, 0, output);

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
    SimulationConfig config = read(filename, mesh.cells.getNumber(), output);
    return {std::move(config), std::move(output), std::move(mesh)};
}

// Reader (default)
SimulationConfig CaseReader::read(const std::string& filename) {
    return read(filename, 0);
}

// Reader (output config generation)
SimulationConfig CaseReader::read(const std::string& filename, std::size_t cellCount) {
    OutputConfig output;
    return read(filename, cellCount, output);
}

// Reader (output config update )
SimulationConfig CaseReader::read(const std::string&  filename,
                                        std::size_t   cellCount,
                                        OutputConfig& output) {
    try {
        return readImpl(filename, cellCount, output);
    } catch (const std::exception& error) {
        throw std::runtime_error(
            "Cannot read case file '" + filename + "': " + error.what());
    }
}

// MAIN READER
SimulationConfig CaseReader::readImpl(const std::string&  filename,
                                            std::size_t   cellCount,
                                            OutputConfig& output) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Cannot open case file '" + filename + "'");

    output = OutputConfig{};
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
        const auto key = normalizeKey(line.substr(0, separator));
        const auto value = trim(line.substr(separator + 1));

        if (key == "directory" || key == "vtk" || key == "csv"
            || key.rfind("csv ", 0) == 0) {
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
        // ------------------------------------------------------------- //
        //                  Simulation Configuration                     //
        // ------------------------------------------------------------- //
        output.fileName = values.at("name");
        config.type = values.at("type");
        config.meshFile = values.at("mesh");
        config.timeStep = parseDouble(values.at("dt"));
        config.timeSteps = parseSize(values.at("timesteps"));
        config.tolerance = parseDouble(values.at("tolerance"));
        config.maxIterations = parseSize(values.at("maxIterations"));
        config.density = values.count("density")
            ? parseDouble(values.at("density"))
            : 1.0;
        // ------------------------------------------------------------- //
        //                    Output Configuration                       //
        // ------------------------------------------------------------- //
        output.outputDirectory = values.count("directory")
            ? values.at("directory")
            : "results";
        if (values.count("vtk")) {
            parseOutputSetting(
                values.at("vtk"), "vtu", output.writeVtk, output.vtkInterval
            );
        }
        if (values.count("csv")) {
            parseOutputSetting(
                values.at("csv"), "csv", output.writeCsv, output.csvInterval
            );
        }
    } catch (const std::out_of_range& error) {
        throw std::runtime_error(
            "Missing required key '" + std::string(error.what()) +
            "' in case file '" + filename + "'"
        );
    }
    // ------------------------------------------------------------- //
    //                   Timesteps Configuration                     //
    // ------------------------------------------------------------- //
    if (config.timeSteps > 0) {
        if (config.timeStep <= 0.0) {
            throw std::runtime_error("dt must be positive when timesteps is greater than zero");
        }
        if (config.density <= 0.0) {
            throw std::runtime_error("density must be positive for transient simulations");
        }
    }

    // ------------------------------------------------------------- //
    //                  Scalar Field Configuration                   //
    // ------------------------------------------------------------- //
    if (!values.count("scalar fields") || !values.count("diffusivity")
        || !values.count("initial values")) {
        throw std::runtime_error("Missing scalar-field configuration in '" + filename + "'");
    }

    const auto fieldNames = split(values.at("scalar fields"));
    const auto diffusivities = split(values.at("diffusivity"));
    const auto initialValues = split(values.at("initial values"));
    if (fieldNames.empty()) throw std::runtime_error("No scalar fields were configured");
    if (diffusivities.size() != fieldNames.size()) {
        throw std::runtime_error("diffusivity count does not match scalar-field count");
    }
    if (initialValues.size() != fieldNames.size()) {
        throw std::runtime_error("initial values count does not match scalar-field count");
    }
    config.scalarFields.reserve(fieldNames.size());

    // Generation of each Scalar Field
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

    // ------------------------------------------------------------- //
    //                 Velocity Field Configuration                  //
    // ------------------------------------------------------------- //
    const auto initialVelocity = split(values.at("initial velocity"));
    if (initialVelocity.size() != 3) {
        throw std::runtime_error("missing initial velocity component");
    }

    double initialValueX = parseDouble(initialVelocity[0]);
    double initialValueY = parseDouble(initialVelocity[1]);
    double initialValueZ = parseDouble(initialVelocity[2]);

    VectorField vectorField("Velocity", cellCount, {initialValueX,
                                                                                initialValueY, 
                                                                                initialValueZ});
    config.velocity = vectorField;

    // ------------------------------------------------------------- //
    //                  CSV Writer Configuration                     //
    // ------------------------------------------------------------- //
    if (output.writeCsv) {
        if (!values.count("csv start points") || !values.count("csv end points")
            || !values.count("csv resolution")) {
            throw std::runtime_error(
                "csv=true requires csv start points, csv end points and csv resolution"
            );
        }
        const auto startPoints = parsePoints(values.at("csv start points"));
        const auto endPoints = parsePoints(values.at("csv end points"));
        const auto resolutions = split(values.at("csv resolution"));
        if (startPoints.empty() || startPoints.size() != endPoints.size()
            || startPoints.size() != resolutions.size()) {
            throw std::runtime_error(
                "CSV start points, end points and resolutions must have matching counts"
            );
        }
        output.csvLines.reserve(startPoints.size());
        for (std::size_t line = 0; line < startPoints.size(); ++line) {
            const auto resolution = parseSize(resolutions[line]);
            if (resolution == 0) {
                throw std::runtime_error("CSV line resolution must be positive");
            }
            output.csvLines.push_back({
                startPoints[line],
                endPoints[line],
                resolution
            });
        }
    }

    return config;
}
