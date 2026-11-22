#include "TestUtils.hpp"
#include "TestDiffusion.cpp"

#include <exception>
#include <iostream>

int main()
{
    test::SuiteReporter reporter("PROCESS TESTS");
    try {
        reporter.subgroup("Steady-state diffusion");
        reporter.run("linear Dirichlet profile with adiabatic y/z boundaries",
                     test::testDiffusion);
        reporter.passed();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PROCESS TESTS stopped: " << error.what() << '\n';
        return 1;
    }
}
