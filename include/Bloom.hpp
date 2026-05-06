/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include <algorithm>
#include <array>
#include <random>
#include <vector>

class Bloom
{

    private:

    class Keyword
    {

        private:
        std::vector<size_t> bigram_representation;

        public:
        Keyword(const std::string& keyword);
        const std::vector<size_t>& fetch() const;

    };

    class HashFunction
    {

        private:
        double b, w;
        size_t m;
        size_t id;
        std::array<double, 676> a;

        public:
        HashFunction(size_t m, double w, size_t id);
        std::vector<size_t> hash(const Bloom::Keyword& keyword, size_t num_extra_probes) const;

    };

    double w;
    size_t l, m, num_extra_probes;
    std::vector<HashFunction> hash_functions;
    
    public:

    Bloom(size_t l, size_t m, double w, size_t num_extra_probes);
    std::vector<double> fit(const std::vector<std::string>& keywords) const;

};

Bloom::Keyword::Keyword(const std::string& keyword)
{
    for (size_t i = 0; i < keyword.length() - 1; i++)
    {
        size_t c = static_cast<size_t>(std::toupper(static_cast<unsigned char>(keyword.at(i)))) - 65;
        size_t c1 = static_cast<size_t>(std::toupper(static_cast<unsigned char>(keyword.at(i + 1)))) - 65;
        size_t index = c * 26 + c1;
        if (std::find(this->bigram_representation.begin(), this->bigram_representation.end(), index) == this->bigram_representation.end())
        {
            this->bigram_representation.push_back(index);
        }
    }
}

const std::vector<size_t>& Bloom::Keyword::fetch() const
{
    return this->bigram_representation;
}


Bloom::HashFunction::HashFunction(size_t m, double w, size_t id) : w(w), m(m), id(id)
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

std::vector<size_t> Bloom::HashFunction::hash(const Bloom::Keyword& keyword, size_t num_extra_probes) const
{

    double dot_product = 0;
    const std::vector<size_t>& representation = keyword.fetch();
    for (size_t index : representation)
    {
        dot_product += this->a[index];
    }

    double val = (dot_product + this->b) / this->w;
    int primary_hash = static_cast<int>(std::floor(val));

    std::vector<int> int_hashes;
    if (num_extra_probes % 2 == 0)
    {
        primary_hash -= num_extra_probes / 2;
    } else
    {
        if (val - primary_hash < 0.5)
        {
            primary_hash -= (num_extra_probes + 1) / 2;
        } else
        {
            primary_hash -= (num_extra_probes - 1) / 2;
        }
    }
    int last_hash = primary_hash + num_extra_probes;
    while (primary_hash <= last_hash)
    {
        int_hashes.push_back(primary_hash++);
    }

    std::vector<size_t> hashes;
    for (int hash : int_hashes)
    {
        size_t h1 = std::hash<int>{}(hash);
        size_t h2 = std::hash<size_t>{}(this->id);
        size_t combined_hash = h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        hashes.push_back(combined_hash % this->m);
    }

    return hashes;
    
}

Bloom::Bloom(size_t l, size_t m, double w, size_t num_extra_probes) : w(w), l(l), m(m), num_extra_probes(num_extra_probes)
{
    this->hash_functions.reserve(l);
    for (size_t i = 0; i < l; i++)
    {
        this->hash_functions.emplace_back(m, w, i);
    }
}

std::vector<double> Bloom::fit(const std::vector<std::string>& keywords) const
{
    std::vector<double> filter(this->m, 0.0);
    for (const std::string& keyword : keywords)
    {
        Keyword x(keyword);
        for (const HashFunction& hash_function : this->hash_functions)
        {
            for (size_t hash : hash_function.hash(x, this->num_extra_probes))
            {
                filter[hash] = 1;
            }
        }
    }
    return filter;
}