// ============================================================
//  PostgreSQL CRUD Sample - Visual Studio C++ (libpq)
//  Table: users (id SERIAL, name VARCHAR, email VARCHAR)
// ============================================================
//
//  [事前準備]
//  1. PostgreSQL をインストール
//     例: C:\Program Files\PostgreSQL\16\
//
//  2. Visual Studio プロジェクト設定
//     [C/C++] > 追加のインクルードディレクトリ:
//       C:\Program Files\PostgreSQL\16\include
//     [リンカー] > 追加のライブラリディレクトリ:
//       C:\Program Files\PostgreSQL\16\lib
//     [リンカー] > 追加の依存ファイル:
//       libpq.lib
//
//  3. 実行時に libpq.dll が PATH に含まれていること
//     (PostgreSQL の bin フォルダを PATH に追加)
//
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <libpq-fe.h>

// ------------------------------------------------------------
//  接続情報 (環境に合わせて変更してください)
// ------------------------------------------------------------
static const char* CONN_INFO =
"host=localhost "
"port=5432 "
"dbname=postgres "
"user=postgres "
"password=your_password";

// ------------------------------------------------------------
//  User 構造体
// ------------------------------------------------------------
struct User {
    int         id;
    std::string name;
    std::string email;
};

// ------------------------------------------------------------
//  RAII ラッパー: 接続を自動クローズ
// ------------------------------------------------------------
class PGConnection {
public:
    explicit PGConnection(const char* connInfo) {
        conn_ = PQconnectdb(connInfo);
        if (PQstatus(conn_) != CONNECTION_OK) {
            std::string err = PQerrorMessage(conn_);
            PQfinish(conn_);
            conn_ = nullptr;
            throw std::runtime_error("接続失敗: " + err);
        }
        std::cout << "[DB] PostgreSQL に接続しました\n";
    }

    ~PGConnection() {
        if (conn_) {
            PQfinish(conn_);
            std::cout << "[DB] 接続を閉じました\n";
        }
    }

    PGconn* get() const { return conn_; }

    // コピー禁止
    PGConnection(const PGConnection&) = delete;
    PGConnection& operator=(const PGConnection&) = delete;

private:
    PGconn* conn_;
};

// ------------------------------------------------------------
//  RAII ラッパー: クエリ結果を自動解放
// ------------------------------------------------------------
class PGResult {
public:
    explicit PGResult(PGresult* res) : res_(res) {}

    ~PGResult() {
        if (res_) PQclear(res_);
    }

    PGresult* get() const { return res_; }

    bool ok(ExecStatusType expected = PGRES_COMMAND_OK) const {
        return PQresultStatus(res_) == expected;
    }

    std::string errorMessage() const {
        return PQresultErrorMessage(res_);
    }

    PGResult(const PGResult&) = delete;
    PGResult& operator=(const PGResult&) = delete;

private:
    PGresult* res_;
};

// ============================================================
//  テーブル作成 (初回のみ)
// ============================================================
void createTable(PGConnection& db) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id    SERIAL PRIMARY KEY,"
        "  name  VARCHAR(100) NOT NULL,"
        "  email VARCHAR(200) NOT NULL UNIQUE"
        ");";

    PGResult res(PQexec(db.get(), sql));
    if (!res.ok()) {
        throw std::runtime_error("テーブル作成失敗: " + res.errorMessage());
    }
    std::cout << "[CREATE TABLE] users テーブルを確認/作成しました\n";
}

// ============================================================
//  CREATE - レコード追加
// ============================================================
int insertUser(PGConnection& db, const std::string& name, const std::string& email) {
    const char* sql =
        "INSERT INTO users (name, email) VALUES ($1, $2) RETURNING id;";

    const char* params[2] = { name.c_str(), email.c_str() };

    PGResult res(PQexecParams(
        db.get(), sql,
        2,       // パラメータ数
        nullptr, // パラメータ型 (サーバーに推論させる)
        params,
        nullptr, nullptr,
        0        // テキスト形式
    ));

    if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
        throw std::runtime_error("INSERT 失敗: " + res.errorMessage());
    }

    int newId = std::stoi(PQgetvalue(res.get(), 0, 0));
    std::cout << "[INSERT] id=" << newId
        << " name=" << name
        << " email=" << email << "\n";
    return newId;
}

// ============================================================
//  READ - 全件取得
// ============================================================
std::vector<User> getAllUsers(PGConnection& db) {
    const char* sql = "SELECT id, name, email FROM users ORDER BY id;";

    PGResult res(PQexec(db.get(), sql));
    if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
        throw std::runtime_error("SELECT 失敗: " + res.errorMessage());
    }

    std::vector<User> users;
    int rows = PQntuples(res.get());
    for (int i = 0; i < rows; ++i) {
        User u;
        u.id = std::stoi(PQgetvalue(res.get(), i, 0));
        u.name = PQgetvalue(res.get(), i, 1);
        u.email = PQgetvalue(res.get(), i, 2);
        users.push_back(u);
    }
    return users;
}

// ============================================================
//  READ - ID で 1 件取得
// ============================================================
bool getUserById(PGConnection& db, int id, User& out) {
    const char* sql =
        "SELECT id, name, email FROM users WHERE id = $1;";

    std::string idStr = std::to_string(id);
    const char* params[1] = { idStr.c_str() };

    PGResult res(PQexecParams(
        db.get(), sql, 1, nullptr, params, nullptr, nullptr, 0
    ));

    if (PQresultStatus(res.get()) != PGRES_TUPLES_OK) {
        throw std::runtime_error("SELECT (by id) 失敗: " + res.errorMessage());
    }

    if (PQntuples(res.get()) == 0) return false;

    out.id = std::stoi(PQgetvalue(res.get(), 0, 0));
    out.name = PQgetvalue(res.get(), 0, 1);
    out.email = PQgetvalue(res.get(), 0, 2);
    return true;
}

// ============================================================
//  UPDATE - name / email を更新
// ============================================================
bool updateUser(PGConnection& db, int id,
    const std::string& newName, const std::string& newEmail) {
    const char* sql =
        "UPDATE users SET name=$1, email=$2 WHERE id=$3;";

    std::string idStr = std::to_string(id);
    const char* params[3] = { newName.c_str(), newEmail.c_str(), idStr.c_str() };

    PGResult res(PQexecParams(
        db.get(), sql, 3, nullptr, params, nullptr, nullptr, 0
    ));

    if (!res.ok()) {
        throw std::runtime_error("UPDATE 失敗: " + res.errorMessage());
    }

    int affected = std::stoi(PQcmdTuples(res.get()));
    std::cout << "[UPDATE] id=" << id
        << " -> name=" << newName
        << " email=" << newEmail
        << " (" << affected << " 行更新)\n";
    return affected > 0;
}

// ============================================================
//  DELETE - ID でレコード削除
// ============================================================
bool deleteUser(PGConnection& db, int id) {
    const char* sql = "DELETE FROM users WHERE id = $1;";

    std::string idStr = std::to_string(id);
    const char* params[1] = { idStr.c_str() };

    PGResult res(PQexecParams(
        db.get(), sql, 1, nullptr, params, nullptr, nullptr, 0
    ));

    if (!res.ok()) {
        throw std::runtime_error("DELETE 失敗: " + res.errorMessage());
    }

    int affected = std::stoi(PQcmdTuples(res.get()));
    std::cout << "[DELETE] id=" << id
        << " (" << affected << " 行削除)\n";
    return affected > 0;
}

// ============================================================
//  表示ヘルパー
// ============================================================
void printUsers(const std::vector<User>& users) {
    std::cout << "\n--- users テーブル (" << users.size() << " 件) ---\n";
    std::cout << "ID  | NAME                | EMAIL\n";
    std::cout << "----|---------------------|---------------------------\n";
    for (const auto& u : users) {
        printf("%-4d| %-21s| %s\n",
            u.id, u.name.c_str(), u.email.c_str());
    }
    std::cout << "\n";
}

// ============================================================
//  main
// ============================================================
int main() {
    try {
        // 接続
        PGConnection db(CONN_INFO);

        // テーブル作成
        createTable(db);

        // ---- CREATE ----
        std::string title_1 = "tanaka-tarou";
        std::string title_2 = "satou-hanako";
        std::string title_3 = "suzuki-ichirou";

        std::cout << "\n=== CREATE ===\n";
        int id1 = insertUser(db, title_1, "tanaka@example.com");
        int id2 = insertUser(db, title_2, "sato@example.com");
        int id3 = insertUser(db, title_3, "suzuki@example.com");

        // ---- READ (全件) ----
        std::cout << "\n=== READ (全件) ===\n";
        printUsers(getAllUsers(db));

        // ---- READ (1件) ----
        std::cout << "=== READ (id=" << id1 << ") ===\n";
        User found;
        if (getUserById(db, id1, found)) {
            printf("  -> id=%d name=%s email=%s\n",
                found.id, found.name.c_str(), found.email.c_str());
        }

        // ---- READ (全件) ----
        std::cout << "\n=== READ (更新後) ===\n";
        printUsers(getAllUsers(db));

        // ---- DELETE ----
        std::cout << "=== DELETE ===\n";
        deleteUser(db, id3);

        // ---- READ (全件) ----
        std::cout << "\n=== READ (削除後) ===\n";
        printUsers(getAllUsers(db));

    }
    catch (const std::exception& ex) {
        std::cerr << "[ERROR] " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
