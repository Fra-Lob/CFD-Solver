#include "TestUtils.hpp"

#include <fields/ScalarField.hpp>

namespace test {
    
    void testScalarFieldInitialState()
    {
        ScalarField field("pressure", 3, 101325.0);
        require(field.getName() == "pressure", "Scalar field name");
        require(field.getNumber() == 3, "Scalar field size");
        for (std::size_t cell = 0; cell < field.getNumber(); ++cell) {
            require(approx(field.getValue(cell), 101325.0), "Scalar field initial value");
        }
    }

    void testScalarFieldSetValue()
    {
        ScalarField field("temperature", 2, 0.0);
        field.setValue(1, 298.15);
        require(approx(field.getValue(0), 0.0), "Unmodified scalar field value");
        require(approx(field.getValue(1), 298.15), "Updated scalar field value");
    }

    void testScalarFieldFill()
    {
        ScalarField field("velocityMagnitude", 4, 1.0);
        field.fill(2.5);
        for (std::size_t cell = 0; cell < field.getNumber(); ++cell) {
            require(approx(field.getValue(cell), 2.5), "Filled scalar field value");
        }
    }

    void testScalarFieldBounds()
    {
        ScalarField field("density", 1, 1.2);
        bool getRejected = false;
        bool setRejected = false;
        try {
            (void)field.getValue(1);
        } catch (const std::out_of_range&) {
            getRejected = true;
        }
        try {
            field.setValue(1, 1.0);
        } catch (const std::out_of_range&) {
            setRejected = true;
        }
        require(getRejected, "Out-of-range scalar field read");
        require(setRejected, "Out-of-range scalar field write");
    }
}