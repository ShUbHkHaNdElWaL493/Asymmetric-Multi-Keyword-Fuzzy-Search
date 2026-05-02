/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include "Bloom.hpp"
#include "IndexGen.hpp"
#include "KeyGen.hpp"
#include "QueryGen.hpp"

template <size_t l, size_t m, double w, size_t k>
class Scheme
{

    private:

        Bloom<l, m, w> bloom_filter;
        KeyGen<m> key_gen;
        IndexGen<l, m, w> index_gen;
        QueryGen<l, m, w> query_gen;
        std::vector<std::pair<std::string, std::pair<std::vector<double>, std::vector<double>>>> entries;
    
    public:

        Scheme();
        void addEntry(std::string document_id, std::vector<std::string> document_keywords);
        std::vector<std::pair<std::string, double>> match(std::vector<std::string> query_keywords);

};

template <size_t l, size_t m, double w, size_t k>
Scheme<l, m, w, k>::Scheme() :
bloom_filter(),
key_gen(),
index_gen(bloom_filter, key_gen.getKey()),
query_gen(bloom_filter, key_gen.getKey())
{}

template <size_t l, size_t m, double w, size_t k>
void Scheme<l, m, w, k>::addEntry(std::string document_id, std::vector<std::string> document_keywords)
{
    std::pair<std::vector<double>, std::vector<double>> encoded_document_keywords = index_gen.encode(document_keywords);
    std::pair<std::string, std::pair<std::vector<double>, std::vector<double>>> entry;
    entry.first = document_id;
    entry.second = encoded_document_keywords;
    this->entries.push_back(entry);
}

template <size_t l, size_t m, double w, size_t k>
double Scheme<l, m, w, k>::match(std::vector<std::string> query_keywords)
{
    std::vector<std::pair<std::string, double>> matches;
    for (const auto& entry : this->entries)
    {
        std::pair<std::vector<double>, std::vector<double>> encoded_q = query_gen.encode(query_keywords);
        double out = 0;
        for (size_t i = 0; i < m; i++)
        {
            out += entry.second.first[i] * encoded_q.first[i] + entry.second.second[i] * encoded_q.second[i];
        }
        if (matches.size() < k)
        {
            std::pair<std::string, double> new_match;
            new_match.first = entry.first;
            new_match.second = out;
            int index = 0;
            while (index < matches.size())
            {
                if (matches[i].second < out)
                {
                    break;
                }
                index++;
            }
            matches.insert(matches.begin() + index, new_match);
        } else
        {
            for (size_t i = 0; i < matches.size(); i++)
            {
                if (matches[i].second < out)
                {
                    std::pair<std::string, double> new_match;
                    new_match.first = entry.first;
                    new_match.second = out;
                    matches.insert(matches.begin() + i, new_match);
                    matches.pop_back();
                    break;
                }
            }
        }
    }
    return matches;
}