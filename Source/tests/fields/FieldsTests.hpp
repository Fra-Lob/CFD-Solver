#include "TestScalarField.cpp"

#include <exception>
#include <iostream>

int main()
{
    test::SuiteReporter reporter("FIELDS TESTS");
    try {
        reporter.subgroup("Scalar field");
        reporter.run("constructor initializes name, size and cell values",
                     test::testScalarFieldInitialState);
        reporter.run("setValue updates the requested cell", test::testScalarFieldSetValue);
        reporter.run("fill assigns the value to every cell", test::testScalarFieldFill);
        reporter.run("out-of-range cell access is rejected", test::testScalarFieldBounds);
        reporter.passed();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FIELDS TESTS stopped: " << error.what() << '\n';
        return 1;
    }
}
