#pragma once // Avoid multiple inclusions of this header file
#include <vector>
#include <array>
#include <cstddef>
#include <stdexcept>

namespace cfd {
    
    class Nodes {

    private:
        std::vector<double> X_;
        std::vector<double> Y_;
        std::vector<double> Z_;

    public:
        //Constructor

        // Adding nodes
        void addNode(double x, double y, double z) {
            X_.push_back(x);
            Y_.push_back(y);
            Z_.push_back(z);
        }

        // Getters
        std::size_t getNodesNumber() const {
            return X_.size();
        }

        std::array<double, 3> getNodeCoordinates(std::size_t index) const {
            if (index >= X_.size()) {
                throw std::out_of_range("Node index out of range");
            }
            return {X_[index], Y_[index], Z_[index]};
        }
    };
}