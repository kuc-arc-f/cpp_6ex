// todo_3.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//
#include <windows.h>
#include <shellapi.h>    // CommandLineToArgvW
#include <fcntl.h>       // _O_U16TEXT
#include <io.h>          // _setmode
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <sqlite3.h>

#pragma comment(lib, "shell32.lib")

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

// std::wstring を UTF-8 の std::string に変換するヘルパー
std::string to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
// ─────────────────────────────────────────
//  Data model
// ─────────────────────────────────────────
struct Todo {
    int         id;
    std::string title;
    bool        done;
    std::string created_at;
};

// ─────────────────────────────────────────
//  Database helper
// ─────────────────────────────────────────
class DB {
public:
    explicit DB(const std::string& path) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK)
            die("open");
        exec("PRAGMA journal_mode=WAL;");
        exec(R"(
            CREATE TABLE IF NOT EXISTS todos (
                id         INTEGER PRIMARY KEY AUTOINCREMENT,
                title      TEXT    NOT NULL,
                done       INTEGER NOT NULL DEFAULT 0,
                created_at TEXT    NOT NULL
            );
        )");
    }
    ~DB() { sqlite3_close(db_); }

    // ── Write ──────────────────────────────
    void add(const std::string& title) {
        std::string now = "";
        sqlite3_stmt* s;
        prepare("INSERT INTO todos (title, done, created_at) VALUES (?, 0, ?);", &s);
        sqlite3_bind_text(s, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 2, now.c_str(),   -1, SQLITE_TRANSIENT);
        step_and_finalize(s);
        //std::cout << "✓ 追加しました: [" << sqlite3_last_insert_rowid(db_) << "] " << title << "\n";
        std::wcout << L"✓ 追加しました: [" << sqlite3_last_insert_rowid(db_) << L"] " << std::endl;
    }

    void done(int id) {
        sqlite3_stmt* s;
        prepare("UPDATE todos SET done = 1 WHERE id = ?;", &s);
        sqlite3_bind_int(s, 1, id);
        step_and_finalize(s);
        if (sqlite3_changes(db_) == 0)
            std::cout << "ID " << id << " が見つかりません。\n";
        else
            std::cout << "✓ 完了しました: ID " << id << "\n";
    }

    void undone(int id) {
        sqlite3_stmt* s;
        prepare("UPDATE todos SET done = 0 WHERE id = ?;", &s);
        sqlite3_bind_int(s, 1, id);
        step_and_finalize(s);
        if (sqlite3_changes(db_) == 0)
            std::cout << "ID " << id << " が見つかりません。\n";
        else
            std::cout << "✓ 未完了に戻しました: ID " << id << "\n";
    }

    void remove(int id) {
        sqlite3_stmt* s;
        prepare("DELETE FROM todos WHERE id = ?;", &s);
        sqlite3_bind_int(s, 1, id);
        step_and_finalize(s);
        if (sqlite3_changes(db_) == 0)
            //std::cout << "ID " << id << " が見つかりません。\n";
            std::wcout << L"ID " << id << " が見つかりません。\n";
        else
            //std::cout << "✓ 削除しました: ID " << id << "\n";
            std::wcout << L"✓ 削除しました: ID " << id << std::endl;
    }

    void clear_done() {
        exec("DELETE FROM todos WHERE done = 1;");
        std::cout << "✓ 完了済みタスクをすべて削除しました。\n";
    }

    // ── Read ───────────────────────────────
    std::vector<Todo> list(const std::string& filter = "all") {
        std::string sql = "SELECT id, title, done, created_at FROM todos";
        if (filter == "pending")  sql += " WHERE done = 0";
        if (filter == "done")     sql += " WHERE done = 1";
        sql += " ORDER BY id;";

        sqlite3_stmt* s;
        prepare(sql, &s);
        std::vector<Todo> rows;
        while (sqlite3_step(s) == SQLITE_ROW) {
            rows.push_back({
                sqlite3_column_int (s, 0),
                reinterpret_cast<const char*>(sqlite3_column_text(s, 1)),
                sqlite3_column_int (s, 2) != 0,
                reinterpret_cast<const char*>(sqlite3_column_text(s, 3))
            });
        }
        sqlite3_finalize(s);
        return rows;
    }

private:
    sqlite3* db_ = nullptr;

    void exec(const std::string& sql) {
        char* err = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err ? err : "unknown";
            sqlite3_free(err);
            die(msg);
        }
    }

    void prepare(const std::string& sql, sqlite3_stmt** s) {
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, s, nullptr) != SQLITE_OK)
            die(sqlite3_errmsg(db_));
    }

    void step_and_finalize(sqlite3_stmt* s) {
        sqlite3_step(s);
        sqlite3_finalize(s);
    }

    [[noreturn]] static void die(const std::string& msg) {
        std::cerr << "DB error: " << msg << "\n";
        std::exit(1);
    }
};

// ─────────────────────────────────────────
//  Display
// ─────────────────────────────────────────
void print_table(const std::vector<Todo>& todos) {
    if (todos.empty()) {
        std::wcout << L"  (タスクはありません)\n";
        return;
    }
    std::wcout << L"\nTODO-LIST" << std::endl;
    std::wcout << L"----------------" << std::endl;
    std::wcout << L"ID , Title ," << std::endl;
    std::wcout << L"----------------" << std::endl;

    for (const auto& t : todos) {
        std::string status = t.done ? "✔" : "○";
        std::string title  = t.title;
        if (title.size() > 37) title = title.substr(0, 34) + "...";
        std::wcout << L""
                  //<< std::setw(5)  << t.id
                  << L"ID="  << t.id
                  //<< std::setw(6)  << status
                  << L" " << StringToWString(title)
                  << std::endl;
    }
    std::wcout << L"\n";
}

void help() {
    std::cout << R"(
使い方: todo <コマンド> [引数]

コマンド一覧:
  add <タイトル>   新しいタスクを追加
  list             全タスクを一覧表示
  list pending     未完了のタスクのみ表示
  list done        完了済みのタスクのみ表示
  done <ID>        タスクを完了にする
  undone <ID>      タスクを未完了に戻す
  rm <ID>          タスクを削除
  clear            完了済みをすべて削除
  help             このヘルプを表示

データベース: ~/.todo.db
)";
}

std::string DB_PATH = "todo.db";

// ─────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────
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

    // ④ 引数を表示
    //std::wcout << L"引数の数: " << argc << L"\n\n";

    for (int i = 0; i < argc; ++i)
    {
        //std::wcout << L"argv[" << i << L"] = " << argv[i] << L"\n";
    }

    //const char* home = std::getenv("HOME");
    //std::string db_path = home ? (std::string(home) + "/.todo.db") : "todo.db";
    std::string db_path = DB_PATH;
    DB db(db_path);

    //if (argc < 2) { help(); return 0; }
    if (argc < 2) {
        std::wcerr << L"error , command none \n"; 
        return 0; 
    }

    std::wstring cmd = argv[1];

    std::wstring cmd_0 = argv[0];
    //std::cout << "c0=" << cmd_0 << " \n";
    //std::cout << "c1=" << cmd << " \n";

    if (cmd == L"add") {
        if (argc < 3) {
          std::wcerr << L"タイトルを指定してください。\n"; 
          return 1; 
        }
        // Join remaining args as the title
        std::wstring input = argv[2];
        std::string title = to_utf8(input);
        db.add(title);

    } else if (cmd == L"list") {
        auto todos = db.list("all");
        print_table(todos);

    } else if (cmd == L"done") {
        if (argc < 3) { std::cerr << "IDを指定してください。\n"; return 1; }
        db.done(std::stoi(argv[2]));

    } else if (cmd == L"undone") {
        if (argc < 3) { std::cerr << "IDを指定してください。\n"; return 1; }
        db.undone(std::stoi(argv[2]));

    } else if (cmd == L"rm") {
        if (argc < 3) { 
            std::wcerr << L"IDを指定してください。\n"; 
            return 1; 
        }
        db.remove(std::stoi(argv[2]));

    } else if (cmd == L"clear") {
        db.clear_done();

//    } else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
//        help();

    } else {
        //std::wcerr << L"不明なコマンド: " << cmd << std::endl;
        std::wcerr << L"error , invalid command" << cmd << std::endl;
        return 1;
    }

    return 0;
}
