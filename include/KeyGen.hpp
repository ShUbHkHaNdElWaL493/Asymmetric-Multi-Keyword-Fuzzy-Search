/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include <random>
#include "SecretKey.hpp"

class KeyGen
{

    private:

        SecretKey SK;
        std::vector<std::vector<double>> generateInvertibleMatrix(std::mt19937& gen) const;

    public:

        KeyGen(size_t m);
        const SecretKey& getKey() const;

};

std::vector<std::vector<double>> KeyGen::generateInvertibleMatrix(std::mt19937& gen) const
{

    const size_t m = this->SK.getSecurityParameter();
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

KeyGen::KeyGen(size_t m) : SK(m)
{

    std::random_device rd;
    std::mt19937 gen(rd());

    this->SK.setM1(this->generateInvertibleMatrix(gen));
    this->SK.setM2(this->generateInvertibleMatrix(gen));

    std::uniform_int_distribution<> bool_distribution(0, 1);
    std::vector<bool> S(this->SK.getSecurityParameter(), false);
    for (size_t i = 0; i < S.size(); i++)
    {
        if (bool_distribution(gen))
        {
            S[i] = true;
        }
    }
    this->SK.setS(S);

}

const SecretKey& KeyGen::getKey() const
{
    return this->SK;
}