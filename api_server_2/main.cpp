#include "httplib.h"
#include <iostream>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <vector>
#include <string>
#include <mutex>
#include <sstream>

#include "RedisData.hpp"

using json = nlohmann::json;
// ─────────────────────────────────────────
// データ構造
// ─────────────────────────────────────────
struct Todo {
    int         id;
    std::string title;
    bool        done;
};

// インメモリストレージ
static std::vector<Todo> g_todos;
static int               g_next_id = 1;
static std::mutex        g_mutex;

// ─────────────────────────────────────────
// ヘルパー：Todo → JSON 文字列
// ─────────────────────────────────────────
std::string todo_to_json(const Todo& t) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << t.id << ","
        << "\"title\":\"" << t.title << "\","
        << "\"done\":" << (t.done ? "true" : "false")
        << "}";
    return oss.str();
}

std::string todos_to_json(const std::vector<Todo>& todos) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < todos.size(); ++i) {
        if (i > 0) oss << ",";
        oss << todo_to_json(todos[i]);
    }
    oss << "]";
    return oss.str();
}

// ─────────────────────────────────────────
// ヘルパー：JSON から値を取り出す（簡易版）
// ─────────────────────────────────────────
std::string extract_string(const std::string& json, const std::string& key) {
    // "key":"value" を探す
    std::string pattern = "\"" + key + "\":\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return "";
    pos += pattern.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

bool extract_bool(const std::string& json, const std::string& key, bool def = false) {
    std::string pattern = "\"" + key + "\":";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return def;
    pos += pattern.size();
    return json.substr(pos, 4) == "true";
}

// ─────────────────────────────────────────
// CORS ヘッダー（ブラウザからのアクセス用）
// ─────────────────────────────────────────
void set_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ─────────────────────────────────────────
// main
// ─────────────────────────────────────────
int main() {
    httplib::Server svr;

    // ── OPTIONS（プリフライト） ──────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
        res.status = 204;
        });

    // ── POST
    svr.Post("/redis/add", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        try {
            RedisData app(""); 
            app.add_handler(req, res);            
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }
    });

    svr.Post("/redis/get", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        try {
            RedisData app(""); 
            app.get_handler(req, res);            
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }
    });

    // ── GET /todos ── 一覧取得 ───────────────
    svr.Get("/todos", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        set_cors(res);
        res.set_content(todos_to_json(g_todos), "application/json");
    });

    // ── 起動 ────────────────────────────────
    int port_no = 8000;
    std::cout << "TODO Server running on http://localhost:8000\n";
    std::cout << "Endpoints:\n"
//        << "  GET    /todos\n"
        << "  POST /redis/add /redis/get   \n"

    svr.listen("0.0.0.0", port_no);
    return 0;
}
