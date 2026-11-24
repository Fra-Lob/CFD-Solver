#include <process/Runner.hpp>

// ========================================================================================= //
//                              Output files' name managing                                  //
// ========================================================================================= //
std::string SimulationRunner::timeStampedFilename(const std::string& filename, 
                                                        std::size_t  timeStep) {
    if (timeStep == 0) return filename; // stationary

    const std::filesystem::path path(filename);
    return (path.parent_path() /
           (path.stem().string() + "t" + std::to_string(timeStep) +
            path.extension().string())).string();  // <filename>t<timeStep>
}

// ========================================================================================= //
//                                 Writing .vtu and .csv                                     //
// ========================================================================================= //
void SimulationRunner::writeOutputs(const Mesh&             mesh,
                                    const SimulationConfig& config,
                                    const OutputConfig&     output,
                                          std::size_t       timeStep,
                                          bool              writeVtk,
                                          bool              writeCsv) {
    if (writeVtk) {
        VTKConverter::write(mesh, SimulationRunner::timeStampedFilename(output.fileName, timeStep), config);
    }
    if (writeCsv) {
        CSVExporter::write(mesh, config, output, timeStep);
    }
}

// ========================================================================================= //
//                                         MAIN RUN                                          //
// ========================================================================================= //
void SimulationRunner::run(const Mesh&             mesh,
                                 SimulationConfig& config,
                           const OutputConfig&     output) {

    // transient or steady
    const bool transient = config.timeSteps > 0;
    const std::size_t steps = transient ? config.timeSteps : 1;
    const std::size_t cellCount = mesh.cells.getNumber();
    if (transient && output.writeVtk && output.vtkInterval == 0) {
        throw std::invalid_argument("VTK output interval must be positive");
    }
    if (transient && output.writeCsv && output.csvInterval == 0) {
        throw std::invalid_argument("CSV output interval must be positive");
    }

    // solver's settings
    Eigen::BiCGSTAB<Eigen::SparseMatrix<double>> solver;
    solver.setTolerance(config.tolerance);
    solver.setMaxIterations(static_cast<Eigen::Index>(config.maxIterations));

    // some printings
    std::cout << (transient ? "Starting transient simulation '" : "Starting steady simulation '")
              << output.fileName << "' with " << config.scalarFields.size()
              << " scalar field(s)\n";

    // ----------------------------------------------------------------------------------------- //
    //                                         Main Loop                                         //
    // ----------------------------------------------------------------------------------------- //
    for (std::size_t step = 1; step <= steps; ++step) {
        if (transient) {
            std::cout << "Time step " << step << '/' << steps
                      << " (t = " << step * config.timeStep << ")\n";
        }

        // field loop
        // -------------------------------------------------------------------------------------- //
        for (auto& field : config.scalarFields) {
            std::cout << "  Solving field '" << field.getName() << "' ... ";
            LinearSystem system(cellCount);
            Diffusion::assemble(
                mesh,
                field.getDiffusivity(),
                field.getBoundaryConditions(),
                system);
                
            Convection::assemble(
                mesh,
                config.velocity,
                config.density,
                field.getBoundaryConditions(),
                system);

            if (transient) {
                TimeDerivative::assemble(
                    mesh, 
                    config.density,
                    config.timeStep, 
                    field, 
                    system
                );
            }

            // Solving
            system.finalize();
            solver.compute(system.getMatrix());
            if (solver.info() != Eigen::Success) {
                throw std::runtime_error("Linear solver initialization failed");
            }

            Eigen::VectorXd initialGuess(static_cast<Eigen::Index>(cellCount));
            for (std::size_t cell = 0; cell < cellCount; ++cell) {
                initialGuess[static_cast<Eigen::Index>(cell)] = field.getValue(cell);
            }
            const Eigen::VectorXd solution =
                solver.solveWithGuess(system.getRhs(), initialGuess);
            if (solver.info() != Eigen::Success) {
                throw std::runtime_error(
                    "Linear solver did not converge for field '" + field.getName() + "'"
                );
            }

            for (std::size_t cell = 0; cell < cellCount; ++cell) {
                field.setValue(cell, solution[static_cast<Eigen::Index>(cell)]);
            }
            std::cout << "done (" << solver.iterations() << " iterations)\n";
        }
        // -------------------------------------------------------------------------------------- //

        // Writing outputs
        const bool writeVtk = output.writeVtk &&
            (!transient || step % output.vtkInterval == 0 || step == steps);
        const bool writeCsv = output.writeCsv &&
            (!transient || step % output.csvInterval == 0 || step == steps);
        if (writeVtk || writeCsv) {
            SimulationRunner::writeOutputs(
                mesh, config, output, transient ? step : 0, writeVtk, writeCsv);
            std::cout << "  Output written for "
                      << (transient ? "time step " + std::to_string(step) : "steady state") << '\n';
        }
    }

    std::cout << "Simulation completed\n";
}
