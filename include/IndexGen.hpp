/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include "Bloom.hpp"
#include "SecretKey.hpp"

class IndexGen
{
    
    private:
    Bloom B;
    SecretKey SK;
    std::random_device rd;
    std::mt19937 gen;
    std::normal_distribution<double> random_distribution;

    public:
    IndexGen(Bloom B, const SecretKey& SK);
    std::pair<std::vector<double>, std::vector<double>> encode(std::vector<std::string> keywords);

};

IndexGen::IndexGen(Bloom B, const SecretKey& SK) : B(B), SK(SK), rd(), gen(rd()), random_distribution(0.0, 1.0)
{}

std::pair<std::vector<double>, std::vector<double>> IndexGen::encode(std::vector<std::string> keywords)
{

    const size_t m = this->SK.getSecurityParameter();
    std::vector<double> I = this->B.fit(keywords);
    std::vector<double> I1(m, 0.0);
    std::vector<double> I2(m, 0.0);
    for (size_t i = 0; i < m; i++)
    {
        if (this->SK.getS()[i])
        {
            I1[i] = I[i];
            I2[i] = I[i];
        } else
        {
            double r = random_distribution(gen);
            I1[i] = 0.5 * I[i] + r;
            I2[i] = 0.5 * I[i] - r;
        }
    }

    std::pair<std::vector<double>, std::vector<double>> EncodedI;
    EncodedI.first.assign(m, 0.0);
    EncodedI.second.assign(m, 0.0);
    std::vector<std::vector<double>> M1T = this->SK.getM1T();
    std::vector<std::vector<double>> M2T = this->SK.getM2T();
    for (size_t i = 0; i < m; i++)
    {
        EncodedI.first[i] = 0;
        EncodedI.second[i] = 0;
        for (size_t j = 0; j < m; j++)
        {
            EncodedI.first[i] += M1T[i][j] * I1[j];
            EncodedI.second[i] += M2T[i][j] * I2[j];
        }
    }

    return EncodedI;

}