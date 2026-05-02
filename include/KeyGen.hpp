/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include <random>
#include "SecretKey.hpp"

template<size_t m>
class KeyGen
{

    private:

        SecretKey<m> SK;
        std::vector<std::vector<double>> generateInvertibleMatrix(std::mt19937& gen);

    public:

        KeyGen();
        const SecretKey<m>& getKey();

};

template<size_t m>
std::vector<std::vector<double>> KeyGen<m>::generateInvertibleMatrix(std::mt19937& gen)
{

    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::vector<std::vector<double>> A(m, std::vector<double> (m, 0.0));

    for (int i = 0; i < m; ++i)
    {
        double row_sum = 0;
        for (int j = 0; j < m; ++j)
        {
            A[i][j] = dist(gen);
            row_sum += std::abs(A[i][j]);
        }
        A[i][i] = (A[i][i] >= 0 ? 1.0 : -1.0) * (row_sum + 1.0);
    }
    return A;

}

template<size_t m>
KeyGen<m>::KeyGen()
{

    std::random_device rd;
    std::mt19937 gen(rd());

    this->SK.setM1(this->generateInvertibleMatrix(gen));
    this->SK.setM2(this->generateInvertibleMatrix(gen));

    std::uniform_int_distribution<> bool_distribution(0, 1);
    std::bitset<m> S;
    for (size_t i = 0; i < m; i++)
    {
        if (bool_distribution(gen))
        {
            S.set(i);
        }
    }
    this->SK.setS(S);

}

template<size_t m>
const SecretKey<m>& KeyGen<m>::getKey()
{
    return this->SK;
}