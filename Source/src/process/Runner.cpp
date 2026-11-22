#include <eigen3/Eigen/IterativeLinearSolvers>
#include <process/Runner.hpp>
#include <process/Diffusion.hpp>
#include <io/VTKConverter.hpp>
#include <numerics/LinearSystem.hpp>
#include <stdexcept>

void SimulationRunner::run(
        const Mesh& mesh,
        SimulationConfig& config) {

    //if (config.linearSolver == "conjugateGradient") {
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>> solver;
    //}

    solver.setTolerance(config.tolerance);
    solver.setMaxIterations(config.maxIterations);

    for(ScalarField& field : config.scalarFields) {
        // Initialization
        std::size_t nCells = mesh.cells.getNumber();
        LinearSystem system(nCells);

        // Diffusion
        Diffusion::assemble(mesh, 
                            field.getDiffusivity(), 
                            field.getBoundaryConditions(), 
                            system);

        // Solving
        system.finalize();
        solver.compute(system.getMatrix());

        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Linear solver initialization failed");
        }

        Eigen::VectorXd solution = solver.solve(system.getRhs());

        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Linear solver did not converge");
        }

        for (std::size_t cellId = 0; cellId < nCells; cellId++) {
            field.setValue(cellId, solution[static_cast<Eigen::Index>(cellId)]);
        }
    }

    if (config.writeVtk) {
        VTKConverter::write(mesh, config.name, config);
    }
}