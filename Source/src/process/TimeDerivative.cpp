#include <process/TimeDerivative.hpp>

void TimeDerivative::assemble(const Mesh& mesh,
                              double density,
                              double dt,
                              const ScalarField& oldField,
                              LinearSystem& system) {
    
    std::size_t nCells = mesh.cells.getNumber();
    for (std::size_t cellId = 0; cellId < nCells; cellId++) {
        double coefficient = density * mesh.cells.getVolume(cellId) / dt;
        double oldValue = oldField.getValue(cellId);

        system.addCoefficient(cellId, cellId, coefficient);// A_PP
        system.addRHS(cellId, coefficient * oldValue);
    }
}