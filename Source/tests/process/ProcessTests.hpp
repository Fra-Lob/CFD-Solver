#include "TestUtils.hpp"
#include "TestDiffusion.cpp"
#include "TestTimeDerivative.cpp"

#include <exception>
#include <iostream>

int main() {
    
    test::SuiteReporter reporter("PROCESS TESTS");
    try {
        reporter.subgroup("Steady-state diffusion");
        reporter.run("linear Dirichlet profile with adiabatic y/z boundaries",
                     test::testDiffusion);
        reporter.subgroup("Transient diffusion");
        reporter.run("implicit time-derivative diagonal and previous-state RHS",
                     test::testTimeDerivative);
        reporter.run("transient solve, interval output and final-step output",
                     test::testTransientRunnerAndOutputSchedule);
        reporter.passed();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PROCESS TESTS stopped: " << error.what() << '\n';
        return 1;
    }
}
