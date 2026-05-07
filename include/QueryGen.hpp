/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include "Bloom.hpp"
#include "SecretKey.hpp"

class QueryGen
{

    private:
    Bloom B;
    std::random_device rd;
    std::mt19937 gen;
    std::normal_distribution<double> random_distribution;
    std::vector<bool> S;
    std::vector<std::vector<double>> M1I, M2I;

    public:
    QueryGen(Bloom B, const SecretKey& SK);
    std::pair<std::vector<double>, std::vector<double>> encode(std::vector<std::string> keywords);

};

QueryGen::QueryGen(Bloom B, const SecretKey& SK) : B(B), rd(), gen(rd()), random_distribution(0.0, 1.0), M1I(SK.getM1I()), M2I(SK.getM2I()), S(SK.getS())
{}

std::pair<std::vector<double>, std::vector<double>> QueryGen::encode(std::vector<std::string> keywords)
{

    std::vector<double> Q = this->B.fit(keywords);
    const size_t m = Q.size();

    std::vector<double> Q1(m, 0.0);
    std::vector<double> Q2(m, 0.0);
    for (size_t i = 0; i < m; i++)
    {
        if (this->S[i])
        {
            double r = random_distribution(gen);
            Q1[i] = 0.5 * Q[i] + r;
            Q2[i] = 0.5 * Q[i] - r;
        } else
        {
            Q1[i] = Q[i];
            Q2[i] = Q[i];
        }
    }

    std::pair<std::vector<double>, std::vector<double>> EncodedQ;
    EncodedQ.first.assign(m, 0.0);
    EncodedQ.second.assign(m, 0.0);
    for (size_t i = 0; i < m; i++)
    {
        EncodedQ.first[i] = 0;
        EncodedQ.second[i] = 0;
        for (size_t j = 0; j < m; j++)
        {
            EncodedQ.first[i] += this->M1I[i][j] * Q1[j];
            EncodedQ.second[i] += this->M2I[i][j] * Q2[j];
        }
    }

    return EncodedQ;

}