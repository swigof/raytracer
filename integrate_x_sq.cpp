#include "rtweekend.h"

#include <iostream>
#include <iomanip>

double icd(double d) {
    return std::sqrt(4.0 * d);
}

double pdf(double x) {
    return x / 2.0;
}

int main() {
    int N = 1000000;
    auto sum = 0.0;

    for (int i = 0; i < N; i++) {
        auto z = random_double();
        if (z == 0.0)  // Ignore zero to avoid NaNs
            continue;

        auto x = icd(z);
        sum += x*x / pdf(x); // counter-weight the nonuniformity
    }

    std::cout << std::fixed << std::setprecision(12);
    std::cout << "I = " << (sum / N) << '\n';
}