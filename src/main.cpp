/*
    CS22B1090
    Shubh Khandelwal
*/

#define K 10
#include "crow.h"
#include <sstream>
#include <string>
#include "Scheme.hpp"

int main()
{
    try
    {

        bool configured = false; // becomes true after /api/config succeeds
        std::unique_ptr<Scheme> S;

        crow::SimpleApp app;

        // ── Serve the SPA ────────────────────────────────────────────────────
        CROW_ROUTE(app, "/")
        ([](const crow::request& /*req*/, crow::response& res)
        {
            res.set_static_file_info("templates/index.html");
            res.end();
        });

        // ── POST /api/config ─────────────────────────────────────────────────
        CROW_ROUTE(app, "/api/config").methods(crow::HTTPMethod::POST)
        ([&S, &configured](const crow::request& req)
        {

            auto body = crow::json::load(req.body);
            if (!body)
            {
                crow::response res(400, "Invalid JSON");
                return res;
            }

            const size_t L = static_cast<size_t>(body["L"].i());
            const size_t M = static_cast<size_t>(body["M"].i());
            const double W = body["W"].d();

            S.reset();
            S = std::make_unique<Scheme>(L, M, W, K); // Configure Scheme state
            configured = true;

            crow::json::wvalue resp;
            resp["status"] = "ok";
            crow::response res(resp);
            return res;

        });

        // ── POST /api/build ──────────────────────────────────────────────────
        CROW_ROUTE(app, "/api/build").methods(crow::HTTPMethod::POST)
        ([&S, &configured](const crow::request& req)
        {
            if (!configured)
            {
                crow::response res(409, "Call /api/config before /api/build");
                return res;
            }

            auto body = crow::json::load(req.body);
            if (!body || body.t() != crow::json::type::List)
            {
                crow::response res(400, "Expected a JSON array of document objects");
                return res;
            }

            S->resetIndex();

            for (auto& document : body)
            {
                const std::string document_id   = document["docId"].s();
                const std::string document_name = document["docName"].s();
                std::vector<std::string> document_keywords;
                for (auto& kw : document["keywords"])
                {
                    document_keywords.push_back(kw.s());
                }
                S->addEntry(document_id, document_name, document_keywords);
            }

            crow::json::wvalue resp;
            resp["status"] = "indexed";
            crow::response res(resp);
            return res;
        });

        // ── POST /api/reset ──────────────────────────────────────────────────
        CROW_ROUTE(app, "/api/reset").methods(crow::HTTPMethod::POST)
        ([&S, &configured](const crow::request& req)
        {
            if (!configured)
            {
                crow::response res(409, "Call /api/config before /api/reset");
                return res;
            }

            auto body = crow::json::load(req.body);
            if (!body || body.t() != crow::json::type::Number)
            {
                crow::response res(400, "Expected a JSON array of index size");
                return res;
            }

            S->resetIndex();

            crow::json::wvalue resp;
            resp["status"] = "ok";
            crow::response res(resp);
            return res;
        });

        // ── POST /api/search ─────────────────────────────────────────────────
        CROW_ROUTE(app, "/api/search").methods(crow::HTTPMethod::POST)
        ([&S, &configured](const crow::request& req)
        {
            if (!configured)
            {
                crow::response res(409, "Call /api/config before /api/search");
                return res;
            }

            auto body = crow::json::load(req.body);
            if (!body)
            {
                crow::response res(400, "Invalid JSON");
                return res;
            }

            std::vector<std::string> query_keywords;

            if (body.has("tokens") && body["tokens"].t() == crow::json::type::List)
            {
                for (auto& tok : body["tokens"])
                {
                    query_keywords.push_back(tok.s());
                }
            }

            if (query_keywords.empty() && body.has("query"))
            {
                std::istringstream ss(std::string(body["query"].s()));
                std::string tok;
                while (ss >> tok) query_keywords.push_back(tok);
            }

            if (query_keywords.empty())
            {
                crow::response res(400, "No query keywords provided");
                return res;
            }

            std::vector<std::pair<std::pair<std::string, std::string>, double>> matches = S->match(query_keywords);
            crow::json::wvalue::list result_list;
            for (const std::pair<std::pair<std::string, std::string>, double> &match : matches)
            {
                crow::json::wvalue entry;
                entry["docId"] = match.first.first;
                entry["docName"] = match.first.second;
                entry["score"] = match.second;
                result_list.push_back(std::move(entry));
            }

            crow::json::wvalue response_json(result_list);
            crow::response res(response_json);
            return res;
        });

        app.port(3000).multithreaded().run();

    }
    catch (const std::exception& e)
    {
        return 1;
    }

    return 0;
}