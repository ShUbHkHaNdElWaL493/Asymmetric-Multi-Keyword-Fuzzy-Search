/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include "Bloom.hpp"
#include "SecretKey.hpp"

template <size_t l, size_t m, double w>
class QueryGen
{

    Bloom<l, m, w> B;
    SecretKey<m> SK;
    std::random_device rd;
    std::mt19937 gen;
    std::normal_distribution<double> random_distribution;

    public:
    QueryGen(Bloom<l, m, w> B, const SecretKey<m>& SK);
    std::pair<std::vector<double>, std::vector<double>> encode(std::vector<std::string> keywords);

};

template <size_t l, size_t m, double w>
QueryGen<l, m, w>::QueryGen(Bloom<l, m, w> B, const SecretKey<m>& SK) : B(B), SK(SK), rd(), gen(rd()), random_distribution(0.0, 1.0)
{}

template <size_t l, size_t m, double w>
std::pair<std::vector<double>, std::vector<double>> QueryGen<l, m, w>::encode(std::vector<std::string> keywords)
{

    std::vector<double> Q = this->B.fit(keywords);
    std::vector<double> Q1(m, 0.0);
    std::vector<double> Q2(m, 0.0);
    for (size_t i = 0; i < m; i++)
    {
        if (this->SK.getS()[i])
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
    std::vector<std::vector<double>> M1I = this->SK.getM1I();
    std::vector<std::vector<double>> M2I = this->SK.getM2I();
    for (size_t i = 0; i < m; i++)
    {
        EncodedQ.first[i] = 0;
        EncodedQ.second[i] = 0;
        for (size_t j = 0; j < m; j++)
        {
            EncodedQ.first[i] += M1I[i][j] * Q1[j];
            EncodedQ.second[i] += M2I[i][j] * Q2[j];
        }
    }

    return EncodedQ;

}