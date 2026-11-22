
#include <io/CaseReader.hpp>
#include <process/Runner.hpp>

#include <iostream>

int main() {
    
    auto loaded = CaseReader::load();

    std::cout << "Loaded case '" << loaded.config.name << "' with "
              << loaded.config.scalarFields.size() << " scalar field(s)\n";

    SimulationRunner::run(loaded.mesh, loaded.config);
    
    return 0;
}