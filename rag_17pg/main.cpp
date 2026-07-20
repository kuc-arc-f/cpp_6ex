
#include <cmath>
#include <cpr/cpr.h>
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

#include "include/dotenv.h"
#include "pgvector_client.h"
#include "http_client.hpp"
#include "include/my_rag.hpp"
#include "include/openrouter_client.hpp"

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
int ebmed(std::string query, std::string api_key) {
    int ret = 0;
    try {
        MyRag rLib("");
        auto embedding = rLib.getGeminiEmbedding(api_key, query);
        std::wcout << L"Embedding dimensions: " << embedding.size() << std::endl;
        vector_add(embedding, query);

    }
    catch (const std::exception& e) {
        std::wcout << L"Error , main \n";
        return 1;
    }
    return 0;
}


// 読み込んだデータを表示する
void printTextFiles(const std::vector<TextFile>& files, std::string api_key) {
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
        int resp = ebmed(target, api_key);
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
        if (r.distance < 0.4) {
            matches = r.label;
        }        
    }
    ret = matches;
    return ret;
}


/**
*
* @param
*
* @return
*/
void rag_search(
    std::string query, std::string api_key , std::string open_api_key , std::string model_name
) {
    try {
        MyRag rLib("");
        auto embedding = rLib.getGeminiEmbedding(api_key, query);
        std::wcout << L"Embedding dimensions: " << embedding.size() << std::endl;

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
        auto resultsCos   = client.searchCosine(embedding, 1);
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

        OpenRouterClient open_client(open_api_key);
        auto response = open_client.sendChatCompletion(
            model_name,
            out_str,
            1.0,
            2000
        );
        if (response.has_value()) {
            auto outStr = response.value();
            std::wcout << L"AI=" << StringToWString(outStr) << L"\n";
            //std::cout << "Response: " << response.value() << std::endl;
            return;
        } else {
            std::string s_error = "Failed to get , OpenRouterClient response";
            std::cerr << "Failed to get , OpenRouterClient response" << std::endl;
            return;
        }        
        //send_chat(out_str);
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
    dotenv env(".env");
    static std::string resp;
    std::string api_key = env.get("GEMINI_API_KEY", "");
    if(api_key == ""){
        resp="error , GEMINI_API_KEY none.";
        return -1;
    }   
    //std::cout << "api_key=" << api_key << std::endl;
    std::string open_api_key = env.get("OPENROUTER_API_KEY");
    if (open_api_key == "") {
        resp="error , OPENROUTER_API_KEY none.";
        return -1;
    }        
    std::string open_model = env.get("OPENROUTER_MODEL");
    if (open_model == "") {
        resp="error , OPENROUTER_MODEL none.";
        return -1;
    }

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
            printTextFiles(allFiles , api_key);
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
        rag_search(query, api_key, open_api_key, open_model);

        return 0;
    }
    std::wcout << L"\n============================\n";
    std::wcout << L"  完了\n";
    std::wcout << L"============================\n";
    return 0;
}


