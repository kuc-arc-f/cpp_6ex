
#include "httplib.h"
#include <cmath>
#include <fcntl.h>       // _O_U16TEXT
#include <fstream>
#include <filesystem>
#include <iostream>
#include <io.h>          // _setmode
#include <iomanip>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <vector>
#include <random>
#include <shellapi.h>    // CommandLineToArgvW
#include <string>
#include <sstream>
#include <stdexcept>
#include <windows.h>

#pragma comment(lib, "shell32.lib")

using json = nlohmann::json;
#include "Database.hpp"
#include "http_client.hpp"

const size_t      VECTOR_DIM = 1024;  // ベクトル次元数
const std::string DB_PATH = "example.db";

//static std::vector<Todo> g_todos;
static int               g_next_id = 1;
static std::mutex        g_mutex;


struct QueryReq {
    std::string input;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QueryReq, input)

struct SearchReq {
    std::wstring input;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SearchReq, input)

//
std::wstring StringToWString(const std::string& str)
{
    if (str.empty()) return L"";

    int size_needed = MultiByteToWideChar(
        CP_UTF8, 0,
        str.c_str(), (int)str.size(),
        NULL, 0
    );

    std::wstring wstr(size_needed, 0);

    MultiByteToWideChar(
        CP_UTF8, 0,
        str.c_str(), (int)str.size(),
        &wstr[0], size_needed
    );

    return wstr;
}
// wstring ↔ string 変換ヘルパー (Windows ANSI 限定)
static std::string WStrToStr(const std::wstring& ws)
{
    if (ws.empty()) return {};
    int n = ::WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n - 1, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, s.data(), n, nullptr, nullptr);
    return s;
}

// std::wstring を UTF-8 の std::string に変換するヘルパー
std::string to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

struct SearchResponse {
    std::string ret;
    std::string text;
};   
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SearchResponse, ret, text)


struct ChatQuery {
    std::string role;
    std::string content;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChatQuery, role, content)

struct ChatRequest {
    std::string model;
    std::vector<ChatQuery> messages;
    double temperature;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChatRequest, model, messages, temperature)

const std::string API_URL_CHAT = "http://localhost:8090/v1/chat/completions";

std::string extractContent(const std::string& jsonStr)
{
    try {
        auto j = nlohmann::json::parse(jsonStr);
        return j["choices"][0]["message"]["content"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] JSON parse: " << e.what() << "\n";
        return "";
    }
}
// ファイルを文字列として読み込むユーティリティ
std::string readFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}
// ─────────────────────────────────────────
// CORS ヘッダー（ブラウザからのアクセス用）
// ─────────────────────────────────────────
void set_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}


/**
*
* @param
*
* @return
*/
std::string rag_search(std::string query) {
    std::string ret = "";
    try {
        QueryReq req_data;
        req_data.input = query;

        // 2. JSON オブジェクトの作成
        json j1 = req_data;
        std::string json_str = j1.dump();
        //std::wstring w_str = StringToWString(json_str);
        //std::wcout << L"w_str : " << w_str << L"\n";
        HttpClient client;

        auto resp = client.post(
            "http://localhost:8080/embedding",
            json_str
        ); 

        if(!resp.empty()) {
            //std::wcout << L"resp.statusCode=200 \n";
            json j = json::parse(resp);
            auto embedding = j[0]["embedding"];
            auto vec = embedding[0];
            int vlength = sizeof(vec) / sizeof(vec[0]);
            //std::wcout << L"vlen=" << vec.size() << L"\n";
            if (vec.size() != VECTOR_DIM) {
                std::wcerr << L"error , Embedding size mismatch" << std::endl; 
                return ret;
            }
            Database app(DB_PATH);
            int count =  app.get_count();
            //std::wcout << L"count=" << count << L"\n";
            if(count == 0){
                std::cout << "error, documet none \n";
                return ret;
            }   
            std::string resp_str = app.rag_search(vec);
            //std::wcout << L"resp_str=" << StringToWString(resp_str)  << std::endl;

            std::string out_str = "日本語で、回答して欲しい。 \n要約して欲しい。\n\n";
            //std::string resp_str = out;
            if(resp_str.empty()){
                out_str.append("user query: ");
                out_str.append(query);
                out_str.append(" \n");
            }else{
                out_str.append("context:");
                out_str.append(resp_str);
                out_str.append("\n user query: ");
                out_str.append(query);
                out_str.append(" \n");
            }
            //std::wcout << StringToWString(out_str)  << std::endl;
            ChatQuery req2;
            req2.role = "user";
            req2.content = out_str;
            json j2 = req2;
            std::string json_str2 = j2.dump();
            //std::cout << "json_str2:" << json_str2 << std::endl;
            std::vector<ChatQuery> chat_messages;
            chat_messages.push_back(req2);

            std::string target_msg = "[";
            target_msg.append(json_str2);
            target_msg.append("]");
            ChatRequest req3;
            req3.model = "local-model";
            req3.messages = chat_messages;
            req3.temperature = 0.7;
            json j3 = req3; // 構造体を代入するだけ！
            std::string json_str3 = j3.dump();
            //std::cout << "json_str3:" << json_str3 << std::endl;
            std::string requestBody = json_str3;
            //std::cout << "requestBody:" << requestBody  << std::endl;
            auto resp = client.post(
                API_URL_CHAT,
                requestBody
                // Content-Type は省略時 "application/json" が使われる
            );      
            if(!resp.empty()) {
                std::string reply = extractContent(resp);
                //std::wcout << "Assistant: " << StringToWString(reply)  << std::endl;
                ret = reply;
            }       

        }        
        return ret;
    }
    catch (const std::exception& e) {
        std::cout << "\n[ERROR] " << e.what() << std::endl;
        //std::wcout << L"Error , rag_search" << std::endl;
    }
    return ret;
}

int main()
{
    httplib::Server svr;

    // ── OPTIONS（プリフライト） ──────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
        res.status = 204;
    });

    // ── GET /todos ── 一覧取得 ───────────────
    svr.Get("/todos", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        res.set_content("", "application/json");
    });


    svr.Post("/api/rag_search", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
       // 1. Content-Typeの確認
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }
        try {
            json j = json::parse(req.body);
            std::string input = j.at("input").get<std::string>();
            //std::cout << "input=" << input << "\n";
            std::string reply = rag_search(input);

            SearchResponse resp3;
            resp3.ret = "OK";
            resp3.text = reply;
            json j4 = resp3;
            std::string json_str4 = j4.dump();
            res.status = 200;
            res.set_content(json_str4, "application/json");
            return;
        }
        catch (const std::exception& e) {
            // キーが存在しない場合など
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
            return;
        }
    }); 

    // ─── SSR HTMLページ (CSSを<link>タグで参照) ───
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::string html = R"(<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <title>cpp-httplib SSR</title>
    <!-- CSSはサーバーから /style.css として配信 -->
    <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4"></script>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
<!-- 
    <h1>Hello from , cpp-httplib SSR!</h1>
    <p>このページはサーバーサイドでレンダリングされています。</p>
-->
    <div id="app"></div>
    <script type="module" src="/client.js"></script>
</body>
</html>)";
        res.set_content(html, "text/html; charset=utf-8");
    });
    // ─── CSSファイルの配信 ───
    svr.Get("/style.css", [](const httplib::Request&, httplib::Response& res) {
        std::string css = readFile("static/style.css");
        if (css.empty()) {
            res.status = 404;
            res.set_content("CSS not found", "text/plain");
            return;
        }
        res.set_content(css, "text/css; charset=utf-8");
    });
    svr.Get("/client.js", [](const httplib::Request&, httplib::Response& res) {
        std::string css = readFile("static/client.js");
        if (css.empty()) {
            res.status = 404;
            res.set_content("CSS not found", "text/plain");
            return;
        }
        res.set_content(css, "application/javascript");
    });

    // ── 起動 ────────────────────────────────
    int port_no = 8000;
    std::cout << "TODO Server running on http://localhost:8000\n";

    svr.listen("0.0.0.0", port_no);
    return 0;    
}
