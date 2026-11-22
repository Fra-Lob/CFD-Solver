#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace test {

    inline void require(bool condition, const std::string& message)
    {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    inline bool approx(double a, double b, double tol = 1e-10)
    {
        return std::abs(a - b) <=
            tol * std::max({1.0, std::abs(a), std::abs(b)});
    }

    class SuiteReporter {
    public:
        explicit SuiteReporter(std::string title)
            : title_(std::move(title))
        {
            const auto width = std::max<std::size_t>(title_.size() + 4, 20);
            std::cout << std::string(width, '=') << '\n';
            const auto padding = (width - title_.size()) / 2;
            std::cout << std::string(padding, ' ') << title_ << '\n';
            std::cout << std::string(width, '=') << "\n\n";
        }

        void subgroup(const std::string& title)
        {
            const auto width = std::max<std::size_t>(title.size(), 19);
            std::cout << std::string(width, '-') << '\n'
                    << title << '\n'
                    << std::string(width, '-') << '\n';
            testNumber_ = 0;
        }

        template<class Function>
        void run(const std::string& description, Function&& function)
        {
            std::cout << ++testNumber_ << ". Testing " << description << " ... ";
            try {
                std::forward<Function>(function)();
                std::cout << "PASSED\n\n";
            } catch (const std::exception& error) {
                std::cout << "FAILED: " << error.what() << '\n';
                throw;
            }
        }

        void passed() const
        {
            std::cout << title_ << ": PASSED\n";
        }

    private:
        std::string title_;
        std::size_t testNumber_ = 0;
    };

}