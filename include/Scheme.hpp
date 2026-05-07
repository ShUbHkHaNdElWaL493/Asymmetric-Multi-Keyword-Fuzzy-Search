/*
    CS22B1090
    Shubh Khandelwal
*/

#pragma once
#include "Bloom.hpp"
#include "IndexGen.hpp"
#include "KeyGen.hpp"
#include "QueryGen.hpp"

class Scheme
{

    private:

        size_t k;
        IndexGen index_gen;
        QueryGen query_gen;
        std::vector<std::pair<std::pair<std::string, std::string>, std::pair<std::vector<double>, std::vector<double>>>> entries;
        Scheme(const Bloom& bloom_filter, const KeyGen& key_gen, size_t m, size_t k);
    
    public:

        Scheme(size_t l, size_t m, double w, size_t num_extra_probes, size_t k);
        void resetIndex();
        void addEntry(std::string document_id, std::string document_name, std::vector<std::string> document_keywords);
        std::vector<std::pair<std::pair<std::string, std::string>, double>> match(std::vector<std::string> query_keywords);

};

Scheme::Scheme(const Bloom& bloom_filter, const KeyGen& key_gen, size_t m, size_t k) : k(k), index_gen(bloom_filter, key_gen.getKey()), query_gen(bloom_filter, key_gen.getKey())
{}

Scheme::Scheme(size_t l, size_t m, double w, size_t num_extra_probes, size_t k) : Scheme(Bloom(l, m, w, num_extra_probes), KeyGen(m), m, k)
{}

void Scheme::resetIndex()
{
    this->entries.clear();
}

void Scheme::addEntry(std::string document_id, std::string document_name, std::vector<std::string> document_keywords)
{
    std::pair<std::vector<double>, std::vector<double>> encoded_document_keywords = this->index_gen.encode(document_keywords);
    std::pair<std::pair<std::string, std::string>, std::pair<std::vector<double>, std::vector<double>>> entry;
    entry.first.first = document_id;
    entry.first.second = document_name;
    entry.second = encoded_document_keywords;
    this->entries.push_back(entry);
}

std::vector<std::pair<std::pair<std::string, std::string>, double>> Scheme::match(std::vector<std::string> query_keywords)
{

    std::pair<std::vector<double>, std::vector<double>> encoded_q = this->query_gen.encode(query_keywords);
    std::vector<std::pair<std::pair<std::string, std::string>, double>> matches;

    std::pair<std::vector<double>, std::vector<double>> encoded_q_1 = this->index_gen.encode(query_keywords);
    double max_match = 0;
    for (size_t i = 0; i < encoded_q.first.size(); i++)
    {
        max_match += encoded_q.first[i] * encoded_q_1.first[i] + encoded_q.second[i] * encoded_q_1.second[i];
    }

    for (const auto& entry : this->entries)
    {
        double out = 0;
        for (size_t i = 0; i < encoded_q.second.size(); i++)
        {
            out += entry.second.first[i] * encoded_q.first[i] + entry.second.second[i] * encoded_q.second[i];
        }
        out /= max_match;
        if (matches.size() < this->k)
        {
            std::pair<std::pair<std::string, std::string>, double> new_match;
            new_match.first.first = entry.first.first;
            new_match.first.second = entry.first.second;
            new_match.second = out;
            size_t index = 0;
            while (index < matches.size())
            {
                if (matches[index].second < out)
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
                    std::pair<std::pair<std::string, std::string>, double> new_match;
                    new_match.first.first = entry.first.first;
                    new_match.first.second = entry.first.second;
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