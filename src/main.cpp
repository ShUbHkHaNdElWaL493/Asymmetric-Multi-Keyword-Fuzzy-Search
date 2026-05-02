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
        // Scheme<L,M,W,K> is compile-time templated, so we lock in the same
        // defaults the frontend ships with (L=16, M=256, W=0.5, K=10).
        // When POST /api/config arrives we call S.init(L, M, W) to let the
        // scheme reconfigure its internal state without a recompile.
        constexpr size_t  DEFAULT_L = 16;
        constexpr size_t  DEFAULT_M = 256;
        constexpr double   DEFAULT_W = 0.5;

        Scheme<DEFAULT_L, DEFAULT_M, DEFAULT_W, K> S;
        bool configured = false; // becomes true after /api/config succeeds

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
        // Frontend payload:  { "L": 16, "M": 256, "W": 0.5 }
        // Calls S.init(L, M, W) so the scheme can reconfigure itself.
        // Returns 200 {"status":"ok"} on success.
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

            const uint8_t L = static_cast<uint8_t>(body["L"].i());
            const uint8_t M = static_cast<uint8_t>(body["M"].i());
            const double  W = body["W"].d();

            std::cout << "[config] L=" << (int)L
                      << "  M="        << (int)M
                      << "  W="        << W << "\n";

            S.init(L, M, W); // reconfigure internal LSH state
            configured = true;

            crow::json::wvalue resp;
            resp["status"] = "ok";
            crow::response res(resp);
            add_cors(res);
            return res;
        });

        // ── POST /api/index ──────────────────────────────────────────────────
        // Frontend payload:
        //   [ { "docId": "...", "docName": "...", "keywords": ["a","b"] }, … ]
        // Iterates the array and calls S.index(docId, docName, keywords)
        // for each document — same indexing method your Scheme exposes.
        // Returns 200 {"status":"indexed"} on success.
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

            S.reset_index(); // clear any previously built index

            for (auto& doc : body)
            {
                const std::string doc_id   = doc["docId"].s();
                const std::string doc_name = doc["docName"].s();

                std::vector<std::string> keywords;
                for (auto& kw : doc["keywords"])
                    keywords.push_back(kw.s());

                S.index(doc_id, doc_name, keywords);
            }

            std::cout << "[index] Indexed " << body.size() << " documents.\n";

            crow::json::wvalue resp;
            resp["status"] = "indexed";
            crow::response res(resp);
            add_cors(res);
            return res;
        });

        // ── POST /api/search ─────────────────────────────────────────────────
        // Frontend payload:
        //   { "query": "raw string", "tokens": ["tok1", "tok2", …] }
        //
        // Uses S.match(keywords) — the same function your original /match
        // route called — which returns std::pair<std::string, double>.
        //
        // Frontend expects a JSON array:
        //   [ { "docId": "...", "score": 0.95 }, … ]
        // so the single pair is wrapped in a one-element array.  If you
        // later change S.match() to return a vector of pairs, swap in the
        // vector variant below (marked with NOTE).
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

            // Prefer the pre-tokenised array the frontend sends; fall back
            // to splitting the raw query string on whitespace.
            std::vector<std::string> query_tokens;

            if (body.has("tokens") && body["tokens"].t() == crow::json::type::List)
            {
                for (auto& tok : body["tokens"])
                    query_tokens.push_back(tok.s());
            }

            if (query_tokens.empty() && body.has("query"))
            {
                std::istringstream ss(std::string(body["query"].s()));
                std::string tok;
                while (ss >> tok) query_tokens.push_back(tok);
            }

            if (query_tokens.empty())
            {
                crow::response res(400, "No query tokens provided");
                add_cors(res);
                return res;
            }

            std::cout << "[search] tokens=";
            for (auto& t : query_tokens) std::cout << t << " ";
            std::cout << "\n";

            // --- Call your existing S.match() ---
            // Current signature:  std::pair<std::string, double> match(keywords)
            // Wrap the result in a JSON array so the frontend can iterate it.
            //
            // NOTE: if you upgrade S.match() to return
            //   std::vector<std::pair<std::string,double>>
            // replace the block below with:
            //   auto matches = S.match(query_tokens);
            //   for (auto& [doc_id, score] : matches) { ... push entry ... }
            std::pair<std::string, double> match = S.match(query_tokens);

            crow::json::wvalue::list result_list;
            crow::json::wvalue entry;
            entry["docId"] = match.first;
            entry["score"] = match.second;
            result_list.push_back(std::move(entry));

            crow::json::wvalue response_json(result_list);
            crow::response res(response_json);
            add_cors(res);
            return res;
        });

        std::cout << "LSH Search Engine listening on http://localhost:3000\n";
        app.port(3000).multithreaded().run();

    }
    catch (const std::exception& e)
    {
        std::cerr << "[fatal] " << e.what() << '\n';
        return 1;
    }

    return 0;
}