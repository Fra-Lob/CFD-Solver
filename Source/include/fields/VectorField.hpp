#pragma once

#include <array>
#include <physics/BoundaryConditions.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VectorField {
private:
    std::string name_;

    std::vector<double> valuesX_;
    std::vector<double> valuesY_;
    std::vector<double> valuesZ_;

    std::array<double, 3> initialValue_;

public:
// ================================================================================//
//                                  Constructor                                    //
// ================================================================================//

    VectorField(): 
            name_("Undefined"),
            valuesX_(1, 0),
            valuesY_(1, 0),
            valuesZ_(1, 0),
            initialValue_({0, 0, 0}) {}

    VectorField(std::string name, std::size_t nValues, std::array<double, 3> initialValue): 
        name_(std::move(name)),
        valuesX_(nValues, initialValue[0]),
        valuesY_(nValues, initialValue[1]),
        valuesZ_(nValues, initialValue[2]),
        initialValue_(initialValue) {}

// ================================================================================//
//                                      Getters                                    //
// ================================================================================//

    const std::string& getName() const {
        return name_;
    }

    std::size_t getNumber() const {
        return valuesX_.size();
    }

    double getValueX(std::size_t cellId) const {
        return valuesX_.at(cellId);
    }

    double getValueY(std::size_t cellId) const {
        return valuesY_.at(cellId);
    }

    double getValueZ(std::size_t cellId) const {
        return valuesZ_.at(cellId);
    }

    std::array<double, 3> getInitialValue() const {
        return initialValue_;
    }

// ================================================================================//
//                                      Setters                                    //
// ================================================================================//

    void setValueX(std::size_t cellId, double value) {
        valuesX_.at(cellId) = value;
    }

    void setValueY(std::size_t cellId, double value) {
        valuesY_.at(cellId) = value;
    }

    void setValueZ(std::size_t cellId, double value) {
        valuesZ_.at(cellId) = value;
    }

    void fillX(double value) {
        std::fill(valuesX_.begin(), valuesX_.end(), value);
    }

    void fillY(double value) {
        std::fill(valuesY_.begin(), valuesY_.end(), value);
    }

    void fillZ(double value) {
        std::fill(valuesZ_.begin(), valuesZ_.end(), value);
    }
};