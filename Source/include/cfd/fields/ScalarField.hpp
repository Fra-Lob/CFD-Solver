#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace cfd {

class ScalarField {
private:
    std::string name_;
    std::vector<double> values_;
public:

// ================================================================================//
//                                  Constructor                                    //
// ================================================================================//

    ScalarField(std::string name, std::size_t nValues, double initialValue): 
        name_(std::move(name)),
        values_(nValues, initialValue) {}

// ================================================================================//
//                                      Getters                                    //
// ================================================================================//

    const std::string getName() {
        return name_;
    }

    const std::size_t getNumber() {
        return values_.size();
    }

    const double getValue(std::size_t cellId) {
        return values_.at(cellId);
    }

// ================================================================================//
//                                      Setters                                    //
// ================================================================================//

    void setValue(std::size_t cellId, double value) {
        values_.at(cellId) = value;
    }

    void fill(double value) {
    std::fill(values_.begin(), values_.end(), value);
    }
};
}