
#include <windows.h>
#include <fcntl.h>       // _O_U16TEXT
#include <iostream>
#include <io.h>          // _setmode
#include <string>
#include <map>
#include <stdexcept>
#include <curl/curl.h>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <shellapi.h>    // CommandLineToArgvW

#include "http_client.hpp"

#pragma comment(lib, "shell32.lib")

// JSON用エイリアス
using json = nlohmann::json;

struct QueryReq {
    std::string title;
};
// これ一行で、QueryReq <=> json の変換が魔法のように可能になります
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QueryReq, title)

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
//
//int main(int argc, char* argv[])
int main()
{
    HttpClient client(30 /*timeout*/, true /*verify_ssl*/);
    try{
        //POST
        QueryReq req;
        req.title = "test-1";

        json j = req; // 構造体を代入するだけ！
        std::string json_str = j.dump();
        //std::wcout << L"#Start discord post" << std::endl;
        //std::wcout << StringToWString(json_str) << std::endl;
        auto resp = client.post_json("http://localhost:8000/todos", json_str);
        if (!resp.error.empty()) {
            std::wcerr << L"[ERROR] \n";
            return 1;
        }
        std::cout << "Status : " << resp.status_code << "\n";

        //GET
        auto resp2 = client.get("http://localhost:8000/todos");
        if (!resp2.error.empty()) {
            std::wcerr << L"[ERROR] \n";
            return 1;
        }
        std::cout << "Status : " << resp2.status_code << "\n";
        std::cout << "Body   :\n" << resp2.body << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << std::endl;
        //std::wcout << L"Error , main" << std::endl;
        return 0;
    }   
}
