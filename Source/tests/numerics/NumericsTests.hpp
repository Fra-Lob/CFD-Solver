#include "TestLinearSystem.cpp"

#include <exception>
#include <iostream>

int main()
{
    test::SuiteReporter reporter("NUMERICS TESTS");
    try {
        reporter.subgroup("Linear system");
        reporter.run("coefficient and RHS assembly, finalization and reset",
                     test::testLinearSystem);
        reporter.passed();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "NUMERICS TESTS stopped: " << error.what() << '\n';
        return 1;
    }
}
