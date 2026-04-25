#include <cmath>
#include <fcntl.h>       // _O_U16TEXT
#include <fstream>
#include <filesystem>
#include <iostream>
#include <io.h>          // _setmode
#include <iomanip>
#include <string>
#include <windows.h>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>
#include <objbase.h>   // CoCreateGuid#include <random>
#include <shellapi.h>    // CommandLineToArgvW
#include <nlohmann/json.hpp>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#include "HttpClient.h"
#include "qdrant_client.hpp"

// JSON用エイリアス
using json = nlohmann::json;

const std::string COLLECTION = "sample_collection";
const size_t      VECTOR_DIM = 1024;  // ベクトル次元数
const std::wstring API_URL_CHAT = L"http://localhost:8090/v1/chat/completions";

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


std::string GuidToString(const GUID& guid)
{
    char buf[64];
    snprintf(buf, sizeof(buf),
        "%08lX-%04X-%04X-%04X-%012llX",
        guid.Data1,
        guid.Data2,
        guid.Data3,
        (guid.Data4[0] << 8) | guid.Data4[1],
        *((unsigned long long*)&guid.Data4[2])
    );
    return std::string(buf);
}
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

// レスポンスを標準出力に表示
static void PrintResponse(const HttpResponse& resp)
{
    std::cout << "=== Response ===" << std::endl;
    std::cout << "Status : " << resp.statusCode
        << " " << WStrToStr(resp.statusText) << std::endl;

    std::cout << "--- Headers ---" << std::endl;
    for (auto& [k, v] : resp.headers)
        std::cout << WStrToStr(k) << ": " << WStrToStr(v) << std::endl;

    std::cout << "--- Body (" << resp.body.size() << " bytes) ---" << std::endl;
    // 長すぎる場合は先頭 512 文字だけ表示
    if (resp.body.size() <= 512)
        std::cout << resp.body << std::endl;
    else
        std::cout << resp.body.substr(0, 512) << "\n...(truncated)" << std::endl;

    std::cout << std::endl;
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
void vector_add(std::vector<float> embedding, std::string content) {
    try {
        QdrantClient client("localhost", 6333);
        std::wcout << L"=== Qdrant C++ Client ===\n\n";
        std::wcout << L"--- ベクトル登録 ---\n";

        std::vector<QdrantClient::Point> points;
        GUID guid;
        if (CoCreateGuid(&guid) == S_OK)
        {
            std::string uuid = GuidToString(guid);
            std::wcout << "UUID: " << StringToWString(uuid) << std::endl;
            std::vector<QdrantClient::Point> points;
            QdrantClient::Point p;
            p.id = std::hash<std::string>{}(uuid);
            p.vector = embedding;
            p.payload = {
                {"content",    content},
                {"doc_id", uuid}, 
            };
            points.push_back(p);
            client.upsertPoints(COLLECTION, points);
        }
        else
        {
            std::wcout << "Failed to create GUID" << std::endl;
            return;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
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
        auto resp = client.Post(
            L"http://localhost:8080/embedding",
            json_str,
            L"application/json");

        if (resp.statusCode == 200) {
            std::wcout << L"resp.statusCode=200 \n";
            std::string str = resp.body;
            json j = json::parse(str);
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
        //std::wcout << L"resp=" << resp << L"\n";
    }
    std::wcout << L"========================================\n";
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

    auto resp2 = client.Post(
        API_URL_CHAT,
        json_str3,
        L"application/json");

    if (resp2.statusCode == 200) {
        std::wcout << L"resp.statusCode=200 \n\n";
        std::string reply = extractContent(resp2.body);
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

        auto resp = client.Post(
            L"http://localhost:8080/embedding",
            json_str,
            L"application/json");

        if (resp.statusCode == 200) {
            std::wcout << L"resp.statusCode=200 \n";
            std::string str = resp.body;
            json j = json::parse(str);
            auto embedding = j[0]["embedding"];
            auto vec = embedding[0];
            int vlength = sizeof(vec) / sizeof(vec[0]);
            std::wcout << L"vlen=" << vec.size() << L"\n";

            QdrantClient qdrant_client("localhost", 6333);
            std::wcout << L"=== Qdrant C++ Client ===\n\n";

            auto results = qdrant_client.search(COLLECTION, vec, 1);
            std::string matches = "";
            for (const auto& r : results) {
                std::wcout   << L"  スコア: " << r.score
                << L"\n";
                if (r.score > 0.5) {
                    matches = r.payload["content"].get<std::string>();
                }
//                std::wcout << L"  ID: " << r.id
//                    << L"  content: " << StringToWString(r.payload["content"].get<std::string>())
            }
            std::wcout << L"\n"; 
            //std::wcout << L"matches=" << StringToWString(matches) << L"\n";
            std::string out_str = "日本語で、回答して欲しい。 \n要約して欲しい。\n\n";
            std::string resp_str = matches;
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
//
int main()
{
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

    // ④ 引数を表示
    std::wcout << L"引数の数: " << argc << L"\n\n";

    if (argc <= 1) {
        std::wcout << L"[ERROR] argment none \n";
        return 0;
    }

    std::wstring target_act = argv[1];
    if (target_act == L"init") {
        std::wcout << L"#init-start====\n";
        QdrantClient client("localhost", 6333);
        std::wcout << L"=== Qdrant C++ Client ===\n\n";
        // =========================================================
        // 2. コレクション管理
        // =========================================================
        std::wcout << L"--- Collections List ---\n";
        auto collections = client.listCollections();
        if (collections.empty()) {
            std::wcout << L"  (コレクションなし)\n";
        } else {
            for (std::string& c : collections) {
                std::wcout << L"  - " << StringToWString(c) << L"\n";
            };
        }

        // 既存コレクションを削除して再作成
        client.deleteCollection(COLLECTION);
        client.createCollection(COLLECTION, VECTOR_DIM, "Cosine");
        std::wcout << "\n";

    }    
    if (target_act == L"embed") {
        std::wcout << L"#embed-start====\n";
       //std::wstring dirPath = (argc >= 2) ? argv[2] : ".";
        std::wstring dirPath = L"./data";
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
    if (target_act == L"search") {
        std::wcout << L"search-start====\n";
        if (argc <= 2) {
            std::wcout << L"[ERROR] argment none \n";
            return 0;
        }
        std::wstring input = argv[2];
        std::string query =  to_utf8(input);
        rag_search(query);

        return 0;
    }

}

// プログラムの実行: Ctrl + F5 または [デバッグ] > [デバッグなしで開始] メニュー
// プログラムのデバッグ: F5 または [デバッグ] > [デバッグの開始] メニュー

// 作業を開始するためのヒント: 
//    1. ソリューション エクスプローラー ウィンドウを使用してファイルを追加/管理します 
//   2. チーム エクスプローラー ウィンドウを使用してソース管理に接続します
//   3. 出力ウィンドウを使用して、ビルド出力とその他のメッセージを表示します
//   4. エラー一覧ウィンドウを使用してエラーを表示します
//   5. [プロジェクト] > [新しい項目の追加] と移動して新しいコード ファイルを作成するか、[プロジェクト] > [既存の項目の追加] と移動して既存のコード ファイルをプロジェクトに追加します
//   6. 後ほどこのプロジェクトを再び開く場合、[ファイル] > [開く] > [プロジェクト] と移動して .sln ファイルを選択します
