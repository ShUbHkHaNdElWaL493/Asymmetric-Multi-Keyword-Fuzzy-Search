/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include <array>
#include <random>
#include <vector>

class Bloom
{

    private:

    class Keyword
    {

        private:
        std::vector<uint16_t> bigram_representation;

        public:
        Keyword(const std::string& keyword);
        const std::vector<uint16_t>& fetch() const;

    };

    class LocalitySensitiveHashFunction
    {

        private:
        double b, w;
        size_t m;
        std::array<double, 676> a;

        public:
        LocalitySensitiveHashFunction(size_t m, double w);
        int hash(const Bloom::Keyword& keyword) const;

    };

    double w;
    size_t l, m;
    std::vector<LocalitySensitiveHashFunction> hash_functions;
    
    public:

    Bloom(size_t l, size_t m, double w);
    std::vector<double> fit(const std::vector<std::string>& keywords) const;

};

Bloom::Keyword::Keyword(const std::string& keyword)
{
    for (size_t i = 0; i < keyword.length() - 1; i++)
    {
        uint16_t c = static_cast<uint16_t>(std::toupper(static_cast<unsigned char>(keyword.at(i)))) - 65;
        uint16_t c1 = static_cast<uint16_t>(std::toupper(static_cast<unsigned char>(keyword.at(i + 1)))) - 65;
        uint16_t index = c * 26 + c1;
        if (std::find(this->bigram_representation.begin(), this->bigram_representation.end(), index) == this->bigram_representation.end())
        {
            this->bigram_representation.push_back(index);
        }
    }
}

const std::vector<uint16_t>& Bloom::Keyword::fetch() const
{
    return this->bigram_representation;
}


Bloom::LocalitySensitiveHashFunction::LocalitySensitiveHashFunction(size_t m, double w) : w(w), m(m)
{

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::normal_distribution<double> normal_dist(0.0, 1.0);
    std::uniform_real_distribution<double> uniform_float_dist(0.0, w);

    for (size_t i = 0; i < 676; i++)
    {
        this->a[i] = normal_dist(gen);
    }
    this->b = uniform_float_dist(gen);

}

int Bloom::LocalitySensitiveHashFunction::hash(const Bloom::Keyword& keyword) const
{

    double dot_product = 0;
    const std::vector<uint16_t>& representation = keyword.fetch();

    for (uint16_t index : representation)
    {
        dot_product += this->a[index];
    }

    double val = (dot_product + this->b) / this->w;
    int hash_val = static_cast<int>(std::floor(val)) % (int)(this->m);

    while (hash_val < 0) hash_val += this->m;
    return hash_val;
    
}

Bloom::Bloom(size_t l, size_t m, double w) : w(w), l(l), m(m)
{
    this->hash_functions.reserve(l);
    for (size_t i = 0; i < l; i++)
    {
        this->hash_functions.emplace_back(m, w);
    }
}

std::vector<double> Bloom::fit(const std::vector<std::string>& keywords) const
{
    std::vector<double> filter(this->m, 0.0);
    for (const std::string& keyword : keywords)
    {
        Keyword x(keyword);
        for (const LocalitySensitiveHashFunction& hash_function : this->hash_functions)
        {
            filter[hash_function.hash(x)] = 1;
        }
    }
    return filter;
}