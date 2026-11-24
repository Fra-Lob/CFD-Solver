
#include <io/CaseReader.hpp>
#include <process/Runner.hpp>
#include <userDefined/InitialCondition.hpp>

#include <iostream>

int main() {
    
    auto loaded = CaseReader::load();

    std::cout << "Loaded case '" << loaded.output.fileName << "' with "
              << loaded.config.scalarFields.size() << " scalar field(s)\n";

    // Initial conditions
    InitialConditions::setInitialConditions(loaded.mesh, loaded.config);

    // Main Run
    SimulationRunner::run(loaded.mesh, loaded.config, loaded.output);
    
    return 0;
}