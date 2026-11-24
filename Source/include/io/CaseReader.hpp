#pragma once

#include <io/SimulationConfig.hpp>
#include <io/OutputConfig.hpp>
#include <mesh/Mesh.hpp>

#include <cstddef>
#include <string>

class CaseReader {
public:
    struct LoadedCase {
        SimulationConfig config;
        OutputConfig     output;
        Mesh             mesh;
    };

    static LoadedCase load();
    static LoadedCase load(const std::string& filename);
    static SimulationConfig read(const std::string& filename);
    static SimulationConfig read(const std::string& filename, std::size_t cellCount);
    static SimulationConfig read(
        const std::string&  filename,
              std::size_t   cellCount,
              OutputConfig& output);

private:
    static SimulationConfig readImpl(
        const std::string&  filename,
              std::size_t   cellCount,
              OutputConfig& output);
};