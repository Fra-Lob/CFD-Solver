#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

struct CSVLine {
    std::array<double, 3> startPoint;
    std::array<double, 3> endPoint;
    std::size_t resolution;
};

struct OutputConfig {
    std::string fileName;
    std::string outputDirectory = "results";

    bool writeVtk = false;
    std::size_t vtkInterval = 0;

    bool writeCsv = false;
    std::size_t csvInterval = 0;
    std::vector<CSVLine> csvLines;
};
