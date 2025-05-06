#include <iostream>
#include <cmath>

double standardNormalCDF(double z) {
    // Using the formula for standard normal CDF: Φ(z) = 0.5 * (1 + erf(z / sqrt(2)))
    return 0.5 * (1 + std::erf(z / std::sqrt(2)));
}

int main() {
    // 输入分布的已知参数
    double mean, p90, z, sigma;
    std::cout << "请输入分布的数学期望（均值）：";
    std::cin >> mean;
    std::cout << "请输入分布的 P90：";
    std::cin >> p90;
    // 输入待估算的正整数
    double input;
    std::cout << "请输入所使用的抽数：";
    std::cin >> input;

    sigma = (p90 - mean)/1.28155;
    z = (input - mean)/sigma;
    double cdfValue = standardNormalCDF(z);
    // 输出结果
    std::cout << "输入值 " << input << " 对应累积概率约为 " 
              << cdfValue * 100.0 << "% 。" << std::endl;
    std::cout <<"debug: sigma1 = " << sigma << "z1 = "<< z;
    //泊松分布近似正态分布得到σ^2 = λ
    sigma = std::sqrt(5000);
    z = (input - mean)/sigma;
    cdfValue = standardNormalCDF(z);
    // 输出结果
    std::cout << "输入值 " << input << " 对应累积概率约为 " 
              << cdfValue * 100.0 << "% 。" << std::endl;
    std::cout <<"debug: sigma2 = " << sigma << "z2 = "<< z;
    return 0;
}