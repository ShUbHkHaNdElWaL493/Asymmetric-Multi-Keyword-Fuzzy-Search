/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include <cmath>
#include <vector>

class SecretKey
{

    private:
        size_t m;
        std::vector<std::vector<double>> M1, M2, M1I, M2I, M1T, M2T;
        std::vector<bool> S;
    
    public:
        SecretKey(size_t m);
        std::vector<std::vector<double>> getM1() const;
        std::vector<std::vector<double>> getM2() const;
        std::vector<std::vector<double>> getM1I() const;
        std::vector<std::vector<double>> getM2I() const;
        std::vector<std::vector<double>> getM1T() const;
        std::vector<std::vector<double>> getM2T() const;
        std::vector<bool> getS() const;
        const size_t& getSecurityParameter() const;
        void setM1(const std::vector<std::vector<double>>& M1);
        void setM2(const std::vector<std::vector<double>>& M2);
        void setS(const std::vector<bool>& S);
    
};

SecretKey::SecretKey(size_t m) : m(m), S(m, false)
{}

std::vector<std::vector<double>> SecretKey::getM1() const { return this->M1; }
std::vector<std::vector<double>> SecretKey::getM2() const { return this->M2; }
std::vector<std::vector<double>> SecretKey::getM1I() const { return this->M1I; }
std::vector<std::vector<double>> SecretKey::getM2I() const { return this->M2I; }
std::vector<std::vector<double>> SecretKey::getM1T() const { return this->M1T; }
std::vector<std::vector<double>> SecretKey::getM2T() const { return this->M2T; }
std::vector<bool> SecretKey::getS() const { return this->S; }
const size_t& SecretKey::getSecurityParameter() const { return this->m; }

void SecretKey::setM1(const std::vector<std::vector<double>>& M1)
{

    this->M1 = M1;
    this->M1I.assign(this->m, std::vector<double>(this->m, 0.0));
    this->M1T.assign(this->m, std::vector<double>(this->m, 0.0));

    std::vector<std::vector<double>> A(this->m, std::vector<double> (2 * this->m, 0.0));
    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->m; j++)
        {
            A[i][j] = this->M1[i][j];
        }
    }
    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->m; j++)
        {
            if (i == j)
            {
                A[i][this->m + j] = 1;
            } else
            {
                A[i][this->m + j] = 0;
            }
        }
    }
    for (int i = 0; i < this->m; i++)
    {
        
        int pivotRow = i;
        for (int j = i + 1; j < this->m; j++)
        {
            if (std::abs(A[j][i]) > std::abs(A[pivotRow][i]))
            {
                pivotRow = j;
            }
        }
        std::swap(A[i], A[pivotRow]);
        double pivotElement = A[i][i];
        for (int j = i; j < 2 * this->m; j++)
        {
            A[i][j] /= pivotElement;
        }
        for (int j = 0; j < this->m; j++)
        {
            if (j != i)
            {
                double factor = A[j][i];
                for (int k = i; k < 2 * this->m; k++)
                {
                    A[j][k] -= factor * A[i][k];
                }
            }
        }

    }
    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->m; j++)
        {
            this->M1I[i][j] = A[i][this->m + j];
        }
    }

    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->m; j++)
        {
            this->M1T[i][j] = this->M1[j][i];
        }
    }

}

void SecretKey::setM2(const std::vector<std::vector<double>>& M2)
{

    this->M2 = M2;
    this->M2I.assign(this->m, std::vector<double>(this->m, 0.0));
    this->M2T.assign(this->m, std::vector<double>(this->m, 0.0));

    std::vector<std::vector<double>> A(this->m, std::vector<double> (2 * this->m, 0.0));
    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->m; j++)
        {
            A[i][j] = this->M2[i][j];
        }
    }
    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->m; j++)
        {
            if (i == j)
            {
                A[i][this->m + j] = 1;
            } else
            {
                A[i][this->m + j] = 0;
            }
        }
    }
    for (int i = 0; i < this->m; i++)
    {
        int pivotRow = i;
        for (int j = i + 1; j < this->m; j++)
        {
            if (std::abs(A[j][i]) > std::abs(A[pivotRow][i]))
            {
                pivotRow = j;
            }
        }
        std::swap(A[i], A[pivotRow]);
        double pivotElement = A[i][i];
        for (int j = i; j < 2 * this->m; j++)
        {
            A[i][j] /= pivotElement;
        }
        for (int j = 0; j < this->m; j++)
        {
            if (j != i)
            {
                double factor = A[j][i];
                for (int k = i; k < 2 * this->m; k++)
                {
                    A[j][k] -= factor * A[i][k];
                }
            }
        }

    }
    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->m; j++)
        {
            this->M2I[i][j] = A[i][this->m + j];
        }
    }

    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->m; j++)
        {
            M2T[i][j] = this->M2[j][i];
        }
    }

}

void SecretKey::setS(const std::vector<bool>& S) { this->S = S; }