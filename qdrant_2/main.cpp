// ============================================================
// Qdrant ベクトル登録・検索 サンプル (Visual Studio C++ Console)
// 依存ライブラリ:
//   - libcurl  (HTTP REST 通信)
//   - nlohmann/json (JSON シリアライズ・デシリアライズ)
//
// Qdrant 起動方法 (Docker):
//   docker run -p 6333:6333 qdrant/qdrant
// ============================================================

#include <windows.h>
#include <shellapi.h>    // CommandLineToArgvW
#include <fcntl.h>       // _O_U16TEXT
#include <io.h>          // _setmode
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <iomanip>

#include "qdrant_client.hpp"

#pragma comment(lib, "shell32.lib")
// ── ユーティリティ: ランダムベクトル生成 ───────────────────────
std::vector<float> makeRandomVector(size_t dim, unsigned seed = 42)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> vec(dim);
    for (auto& v : vec) v = dist(rng);
    return vec;
}
// ランダムなベクトルを生成するヘルパー
std::vector<float> randomVector(size_t dim, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> v(dim);
    for (auto& x : v) x = dist(rng);
    return v;
}

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
const std::string COLLECTION = "sample_collection";
const size_t      VECTOR_DIM = 128;  // ベクトル次元数

// ============================================================
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
    QdrantClient client("localhost", 6333);
    std::wcout << L"=== Qdrant C++ Client ===\n\n";

    std::wstring target_act = argv[1];
    if (target_act == L"init") {
        std::wcout << L"#init-start====\n";
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
        // =========================================================
        // 2. コレクション管理
        // =========================================================
        std::wcout << L"--- コレクション一覧 ---\n";
        auto collections = client.listCollections();
        if (collections.empty()) {
            std::wcout << L"  (コレクションなし)\n";
        } else {
            for (std::string& c : collections) {
                std::wcout << L"  - " << StringToWString(c) << L"\n";
            };
        }

        // =========================================================
        // 3. ベクトル登録 (Upsert)
        // =========================================================
        std::wcout << L"--- ベクトル登録 ---\n";

        std::vector<QdrantClient::Point> points;

        // ドキュメントサンプル
        struct Doc {
            uint64_t id;
            std::string title;
            std::string category;
            float price;
            unsigned seed;
        };

        std::vector<Doc> docs = {
            {1,  "りんごの育て方",     "農業",   980.0f,  1},
            {2,  "みかんの収穫時期",   "農業",   750.0f,  2},
            {3,  "C++プログラミング",  "IT",    2800.0f,  3},
            {4,  "Rustで始めるOS開発", "IT",    3200.0f,  4},
            {5,  "機械学習入門",       "AI",    1980.0f,  5},
            {6,  "深層学習の数学",     "AI",    2400.0f,  6},
            {7,  "料理の基本",         "料理",   580.0f,  7},
            {8,  "フランス料理入門",   "料理",  1200.0f,  8},
            {9,  "ベクトルDB徹底解説", "AI",    3000.0f,  9},
            {10, "分散システム設計",   "IT",    2600.0f, 10},
        };

        for (const auto& d : docs) {
            QdrantClient::Point p;
            p.id     = d.id;
            p.vector = randomVector(VECTOR_DIM, d.seed);
            p.payload = {
                {"title",    d.title},
                {"category", d.category},
                {"price",    d.price}
            };
            points.push_back(p);
        }

        client.upsertPoints(COLLECTION, points);
        std::wcout << L"\n";
    }
    if (target_act == L"search") {
        // =========================================================
        // 4. ベクトル検索 (類似検索)
        // =========================================================
        std::wcout << L"--- ベクトル類似検索 (Top N) ---\n";
        // seed=5 (機械学習入門) に近いベクトルで検索
        auto query_vec = randomVector(VECTOR_DIM, 5);
        std::wcout << L"クエリベクトル: ";
        std::wcout << L"\n\n";

        auto results = client.search(COLLECTION, query_vec, 5);
        for (const auto& r : results) {
            /*
            std::cout << "  ID: " << r.id
                    << "  スコア: " << std::fixed << std::setprecision(4) << r.score
                    << "  タイトル: " << r.payload["title"].get<std::string>()
                    << "  カテゴリ: " << r.payload["category"].get<std::string>()
                    << "\n";
            */
            std::wcout << L"  ID: " << r.id
                    << L"  スコア: " << r.score
                    << L"  タイトル: " << StringToWString(r.payload["title"].get<std::string>())
                    << L"  カテゴリ: " << StringToWString(r.payload["category"].get<std::string>())
                    << L"\n";

        }
        std::wcout << L"\n";
    }

    return 0;
}
