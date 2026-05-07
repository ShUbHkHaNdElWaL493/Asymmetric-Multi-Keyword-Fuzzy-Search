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
    std::random_device rd;
    std::mt19937 gen;
    std::normal_distribution<double> random_distribution;
    std::vector<bool> S;
    std::vector<std::vector<double>> M1T, M2T;

    public:
    IndexGen(Bloom B, const SecretKey& SK);
    std::pair<std::vector<double>, std::vector<double>> encode(std::vector<std::string> keywords);

};

IndexGen::IndexGen(Bloom B, const SecretKey& SK) : B(B), rd(), gen(rd()), random_distribution(0.0, 1.0), M1T(SK.getM1T()), M2T(SK.getM2T()), S(SK.getS())
{}

std::pair<std::vector<double>, std::vector<double>> IndexGen::encode(std::vector<std::string> keywords)
{

    std::vector<double> I = this->B.fit(keywords);
    const size_t m = I.size();
    
    std::vector<double> I1(m, 0.0);
    std::vector<double> I2(m, 0.0);
    for (size_t i = 0; i < m; i++)
    {
        if (this->S[i])
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
    for (size_t i = 0; i < m; i++)
    {
        EncodedI.first[i] = 0;
        EncodedI.second[i] = 0;
        for (size_t j = 0; j < m; j++)
        {
            EncodedI.first[i] += this->M1T[i][j] * I1[j];
            EncodedI.second[i] += this->M2T[i][j] * I2[j];
        }
    }

    return EncodedI;

}