#include <iostream>
#include <cmath> // For std::erf and std::sqrt

// Function to calculate the standard normal cumulative distribution function (CDF)
double standardNormalCDF(double z) {
    // Using the formula for standard normal CDF: Φ(z) = 0.5 * (1 + erf(z / sqrt(2)))
    return 0.5 * (1 + std::erf(z / std::sqrt(2)));
}

int main() {
    double z;
    
    // Prompt the user for input
    std::cout << "Enter the Z value: ";
    std::cin >> z;
    
    // Calculate the CDF value for the given Z
    double cdfValue = standardNormalCDF(z);
    
    // Output the result
    std::cout << "The cumulative probability up to Z = " << z << " is: " << cdfValue << std::endl;
    
    return 0;
}