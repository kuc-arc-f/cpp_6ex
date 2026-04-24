
#include <cmath>
#include <fcntl.h>       // _O_U16TEXT
#include <fstream>
#include <filesystem>
#include <iostream>
#include <io.h>          // _setmode
#include <string>
#include <windows.h>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>
#include <random>
#include <iomanip>
#include <shellapi.h>    // CommandLineToArgvW
#include <nlohmann/json.hpp>

#pragma comment(lib, "shell32.lib")

#include "pgvector_client.h"
#include "http_client.hpp"

// JSON用エイリアス
using json = nlohmann::json;

const std::string DB_HOST = "localhost";
const std::string DB_NAME = "mydb";
const std::string DB_USER = "root";
const std::string DB_PASSWORD = "admin";

// 1ファイル分のデータを保持する構造体
struct TextFile {
    std::string filename;
    std::vector<std::string> lines;
};

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

// ============================================================
//  ユーティリティ: L2 正規化
// ============================================================
FloatVec normalize(FloatVec v) {
    float norm = 0.0f;
    for (float x : v) norm += x * x;
    norm = std::sqrt(norm);
    if (norm > 1e-9f)
        for (auto& x : v) x /= norm;
    return v;
}

/**
*
* @param
*
* @return
*/
void vector_add(std::vector<float> embedding, std::string content) {
    try {
        PGConnConfig cfg;
        cfg.host = DB_HOST;
        cfg.port = 5432;
        cfg.dbname = DB_NAME;
        cfg.user = DB_USER;
        cfg.password = DB_PASSWORD;

        constexpr int DIM = 128;   // ベクトル次元数
        constexpr int TOPK = 3;     // 近傍検索件数

        PGVectorClient client(cfg);
        // =====================================================
        //  1. 接続
        // =====================================================
        client.connect();

        // =====================================================
        //  3. サンプルベクトルを登録
        // =====================================================
        std::wcout << L"\n--- ベクトル登録 ---\n";
        client.insertEmbedding(content, embedding);

    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}

// .txt ファイルを読み込んで行を返す
TextFile loadTextFile(const std::filesystem::path& filepath) {
    TextFile tf;
    tf.filename = filepath.filename().string();

    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        std::cerr << "[警告] ファイルを開けません: " << filepath << "\n";
        return tf;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        tf.lines.push_back(line);
    }
    return tf;
}

/**
*
* @param
*
* @return
*/
int ebmed(std::string query) {
    int ret = 0;

    HttpClient client;
    std::string api_url = "";
    struct QueryReq req_data;
    req_data.input = query;

    json j_req = req_data;
    //std::cout << j_req.dump() << std::endl;

    std::string json_str = j_req.dump();
    //std::cout << "json_str : " << json_str << "\n";
    try {
        auto resp = client.post(
            "http://localhost:8080/embedding",
            json_str
            // Content-Type は省略時 "application/json" が使われる
        );

        if(!resp.empty()) {
            std::wcout << L"resp.statusCode=200 \n";
            //std::string str = resp.body;
            json j = json::parse(resp);
            auto embedding = j[0]["embedding"];
            auto vec = embedding[0];
            int vlength = sizeof(vec) / sizeof(vec[0]);
            std::wcout << L"vlen=" << vec.size() << L"\n";
            vector_add(vec, query);
        }
    }
    catch (const std::exception& e) {
        std::wcout << L"Error , main \n";
        return 1;
    }
    return 0;
}


// 読み込んだデータを表示する
void printTextFiles(const std::vector<TextFile>& files) {
    for (const auto& tf : files) {
        std::wcout << L"========================================\n";
        //std::wstring w_filename = StringToWString(tf.filename);
        //std::wcout << L"ファイル名: " << w_filename << L"\n";
        std::wcout << L"行数      : " << tf.lines.size() << L"\n";
        std::wcout << L"----------------------------------------\n";
        std::string target = "";
        for (size_t i = 0; i < tf.lines.size(); ++i) {
            //std::cout << "[" << i + 1 << "] " << tf.lines[i] << "\n";
            std::string tmp = tf.lines[i] + "\n";
            target.append(tmp);
        }
        std::wstring w_str = StringToWString(target);
        std::wcout <<  w_str << "\n";
        int resp = ebmed(target);
        std::wcout << L"resp=" << resp << L"\n";
    }
    std::wcout << L"========================================\n";
}

std::string getStringResult(const std::vector<SearchResult>& results,
                  const std::string& title)
{
    std::string ret = "";
    int rank = 1;
    std::string matches = "";
    for (const auto& r : results) {
        std::wcout << L"r.id=" << r.id << L"\n";
        std::wcout << L"r.distance=" << r.distance << L"\n";
        if (r.distance < 0.5) {
            matches = r.label;
        }        
    }
    ret = matches;
    return ret;
}

// ============================================================
//  ユーティリティ: 検索結果表示
// ============================================================
void printResults(const std::vector<SearchResult>& results,
                  const std::string& title)
{
    //std::wcout << L"\n=== " << title << " L===\n";
    /*
    std::cout << std::left
              << std::setw(6)  << "Rank"
              << std::setw(6)  << "ID"
              << std::setw(20) << "Label"
              << std::setw(12) << "Distance"
              << "\n";
    std::cout << std::string(44, '-') << "\n";
    */

    int rank = 1;
    for (const auto& r : results) {
        /*
        std::cout << std::setw(6)  << rank++
                  << std::setw(6)  << r.id
                  << std::setw(20) << r.label
                  << std::fixed << std::setprecision(6)
                  << std::setw(12) << r.distance
                  << "\n";
        */
        std::wcout << L"r.id=" << r.id << L"\n";
        std::wcout << L"r.distance=" << r.distance << L"\n";
        std::wstring w_label = StringToWString(r.label);
        std::wcout << L"r.label=" << std::setw(20) << w_label << L"\n";
    }
}
struct ChatQuery {
    std::string role;
    std::string content;
};
// これ一行で、QueryReq <=> json の変換が魔法のように可能になります
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

void send_chat(std::string query) {
    ChatQuery req2;
    req2.role = "user";
    req2.content = query;
    json j2 = req2;
    std::string json_str2 = j2.dump();
    std::wstring w_str2 = StringToWString(json_str2);
    //std::wcout << L"json_str2:" << w_str2 << std::endl;
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
    std::wstring w_str3 = StringToWString(json_str3);
    //std::wcout << L"json_str3:" << w_str3 << std::endl;
    HttpClient client;
    auto resp2 = client.post(
        API_URL_CHAT,
        json_str3
        // Content-Type は省略時 "application/json" が使われる
    );

    if(!resp2.empty()) {
        std::wcout << L"resp.statusCode=200 \n\n";
        std::string reply = extractContent(resp2);
        std::wstring w_str4 = StringToWString(reply);
        std::wcout << L"Assistant: " << w_str4 << std::endl; 
        std::wcout << L"\n" << std::endl;
    }
}

/**
*
* @param
*
* @return
*/
void rag_search(std::string query) {
    try {
        QueryReq req_data;
        req_data.input = query;

        // 2. JSON オブジェクトの作成
        json j1 = req_data;
        //j1["input"] = to_utf8(req_data.input); // UTF-8に変換して格納
        std::string json_str = j1.dump();
        std::wstring w_str = StringToWString(json_str);
        std::wcout << L"w_str : " << w_str << L"\n";
        HttpClient client;
        auto resp = client.post(
            "http://localhost:8080/embedding",
            json_str
            // Content-Type は省略時 "application/json" が使われる
        );

        if(!resp.empty()) {
            std::wcout << L"resp.statusCode=200 \n";
            //std::string str = resp.body;
            json j = json::parse(resp);
            auto embedding = j[0]["embedding"];
            auto vec = embedding[0];
            int vlength = sizeof(vec) / sizeof(vec[0]);
            std::wcout << L"vlen=" << vec.size() << L"\n";

            //
            PGConnConfig cfg;
            cfg.host = DB_HOST;
            cfg.port = 5432;
            cfg.dbname = DB_NAME;
            cfg.user = DB_USER;
            cfg.password = DB_PASSWORD;

            PGVectorClient client(cfg);
            // =====================================================
            //  1. 接続
            // =====================================================
            client.connect();
            auto resultsCos   = client.searchCosine(vec, 1);
            std::string out =  getStringResult(resultsCos, "Cosine-similar");
            //std::wcout << L"out=" << StringToWString(out) << L"\n";

            std::string out_str = "日本語で、回答して欲しい。 \n要約して欲しい。\n\n";
            std::string resp_str = out;
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
            std::wcout << StringToWString(out_str)  << std::endl;
            send_chat(out_str);
        }        
    }
    catch (const std::exception& e) {
        std::wcerr << L"\n[ERROR] " << e.what() << L"\n";
        std::wcerr << L"Error , rag_search" << std::endl;
    }
}

// ============================================================
//  メイン
// ============================================================
int main() {
    // ① stdout をワイド文字モードに切り替え
    _setmode(_fileno(stdout), _O_U16TEXT);

    // ② コンソールの出力コードページを UTF-8 に設定
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // ③ Windows API でコマンドライン全体を UTF-16 で取得
    //    GetCommandLineW() はプロセス起動時のコマンドライン文字列を返す
    int    argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (argv == nullptr)
    {
        std::wcerr << L"CommandLineToArgvW が失敗しました。\n";
        return 1;
    }

    std::wcout << L"引数の数: " << argc << L"\n\n";
    if (argc <= 2) {
        std::wcout << L"[ERROR] argment none" << L"\n";
        return 0;
    }
    // 引数でフォルダを指定、省略時はカレントディレクトリ
    std::wcout << L"arg[1]=" << argv[1] << L"\n";
    std::wcout << L"arg[2]=" << argv[2] << L"\n";

    // ---------------------------------------------------------
    //  接続設定 (環境に合わせて変更)
    // ---------------------------------------------------------

    std::wstring target_act = argv[1];
    if (target_act == L"embed") {
        std::wcout << L"#embed-start====\n";
       //std::wstring dirPath = (argc >= 2) ? argv[2] : ".";
        std::wstring dirPath = argv[2];
        try {
            if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
                std::wcerr << L"[ERROR] folder none: " << dirPath << L"\n";
                return 1;
            }
            std::wcout << L"対象フォルダ: " << std::filesystem::absolute(dirPath) << L"\n\n";

            std::vector<TextFile> allFiles;

            // フォルダ内の .txt ファイルをすべて列挙
            for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                if (entry.is_regular_file() &&
                    (entry.path().extension() == ".txt" || entry.path().extension() == ".md")) {
                    TextFile tf = loadTextFile(entry.path());
                    allFiles.push_back(std::move(tf));
                }
            }

            if (allFiles.empty()) {
                std::wcout << L".txt ファイルが見つかりませんでした。\n";
                return 0;
            }
            std::wcout << L"読み込んだファイル数: " << allFiles.size() << L"\n\n";
            printTextFiles(allFiles);
            return 0;
        }
        catch (const std::exception& ex) {
            std::wcerr << L"\n[ERROR] " << ex.what() << L"\n";
            return 1;
        }
    }
    //search
    if (target_act == L"search") {
        std::wcout << L"search-start====\n";
        std::wstring input = argv[2];
        std::string query =  to_utf8(input);
        rag_search(query);

        return 0;
    }
    std::wcout << L"\n============================\n";
    std::wcout << L"  完了\n";
    std::wcout << L"============================\n";
    return 0;
}


