/*
    CS22B1090
    Shubh Khandelwal
*/

#define K 10
#include "crow.h"
#include <iostream>
#include <sstream>
#include <string>
#include "Scheme.hpp"

// ---------------------------------------------------------------------------
// CORS helper — lets the browser hit the API even when the frontend is
// opened directly from disk (file://) or a different dev port.
// ---------------------------------------------------------------------------
static void add_cors(crow::response& res)
{
    res.add_header("Access-Control-Allow-Origin",  "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
}

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

        // ── OPTIONS preflight handlers ───────────────────────────────────────
        CROW_ROUTE(app, "/api/config").methods(crow::HTTPMethod::OPTIONS)
        ([](const crow::request& /*req*/)
        {
            crow::response res(204);
            add_cors(res);
            return res;
        });

        CROW_ROUTE(app, "/api/index").methods(crow::HTTPMethod::OPTIONS)
        ([](const crow::request& /*req*/)
        {
            crow::response res(204);
            add_cors(res);
            return res;
        });

        CROW_ROUTE(app, "/api/search").methods(crow::HTTPMethod::OPTIONS)
        ([](const crow::request& /*req*/)
        {
            crow::response res(204);
            add_cors(res);
            return res;
        });

        // ── POST /api/config ─────────────────────────────────────────────────
        CROW_ROUTE(app, "/api/config").methods(crow::HTTPMethod::POST)
        ([&S, &configured](const crow::request& req)
        {

            auto body = crow::json::load(req.body);
            if (!body)
            {
                crow::response res(400, "Invalid JSON");
                add_cors(res);
                return res;
            }

            const size_t L = static_cast<size_t>(body["L"].i());
            const size_t M = static_cast<size_t>(body["M"].i());
            const double W = body["W"].d();

            std::cout << "[config] L=" << (int)L << " M=" << (int)M << " W=" << W << "\n";

            S = std::make_unique<Scheme>(L, M, W, K); // Configure Scheme state
            configured = true;

            crow::json::wvalue resp;
            resp["status"] = "ok";
            crow::response res(resp);
            add_cors(res);
            return res;

        });

        // ── POST /api/index ──────────────────────────────────────────────────
        CROW_ROUTE(app, "/api/index").methods(crow::HTTPMethod::POST)
        ([&S, &configured](const crow::request& req)
        {
            if (!configured)
            {
                crow::response res(409, "Call /api/config before /api/index");
                add_cors(res);
                return res;
            }

            auto body = crow::json::load(req.body);
            if (!body || body.t() != crow::json::type::List)
            {
                crow::response res(400, "Expected a JSON array of document objects");
                add_cors(res);
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

            std::cout << "[index] Indexed " << body.size() << " documents.\n";

            crow::json::wvalue resp;
            resp["status"] = "indexed";
            crow::response res(resp);
            add_cors(res);
            return res;
        });

        // ── POST /api/search ─────────────────────────────────────────────────
        CROW_ROUTE(app, "/api/search").methods(crow::HTTPMethod::POST)
        ([&S, &configured](const crow::request& req)
        {
            if (!configured)
            {
                crow::response res(409, "Call /api/config before /api/search");
                add_cors(res);
                return res;
            }

            auto body = crow::json::load(req.body);
            if (!body)
            {
                crow::response res(400, "Invalid JSON");
                add_cors(res);
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
                add_cors(res);
                return res;
            }

            std::cout << "[search] keywords= ";
            for (auto& t : query_keywords)
            {
                std::cout << t << " ";
            }
            std::cout << "\n";

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
            add_cors(res);
            return res;
        });

        std::cout << "App listening on http://localhost:3000\n";
        app.port(3000).multithreaded().run();

    }
    catch (const std::exception& e)
    {
        std::cerr << "[fatal] " << e.what() << '\n';
        return 1;
    }

    return 0;
}