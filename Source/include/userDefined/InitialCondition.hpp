#pragma once

#include <cstddef>
#include <fields/ScalarField.hpp>
#include <mesh/Mesh.hpp>
#include <io/SimulationConfig.hpp>

class InitialConditions {
public:
    void static setInitialConditions(const Mesh& mesh,
                                      SimulationConfig& confing) {
                
        std::size_t nCells = mesh.cells.getNumber();
        for (auto& field : confing.scalarFields) {
            if (field.getName() == "C1") {
                for (std::size_t cellId = 0; cellId < nCells; cellId++) {
                    auto x = mesh.cells.getCenter(cellId)[0];
                    auto L = mesh.getDomain()[0];
                    field.setValue(cellId, std::sin(constants::PI * x / L));
                }
            }
        }
    }

};