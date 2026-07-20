#pragma once
#include <windows.h>
#include <winhttp.h>
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <nlohmann/json.hpp> // nlohmann/jsonが必要です

#include "openrouter_client.hpp"

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

const std::string COLLECTION = "sample_collection";
const size_t      VECTOR_DIM = 3072;  // ベクトル次元数
const std::wstring API_URL_CHAT = L"http://localhost:8090/v1/chat/completions";

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


// UTF-8文字列をワイド文字列(LPCWSTR)に変換するヘルパー
std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], len);
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    return wide;
}

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

class MyRag {
private:
    std::string m_name;

public:
    explicit MyRag(std::string str){}

    ~MyRag() {}


    // Gemini Embedding API から埋め込みベクトルを取得する関数
    std::vector<float> getGeminiEmbedding(const std::string& apiKey, const std::string& text) {
        std::vector<float> ret;
        const std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-embedding-001:embedContent";
        
        // リクエストボディの構築
        json requestBody = {
            {"model", "models/gemini-embedding-001"},
            {"content", {
                {"parts", {{{"text", text}}}}
            }}
        };

        cpr::Response r = cpr::Post(
            cpr::Url{url},
            cpr::Header{{"Content-Type", "application/json"}, 
                        {"x-goog-api-key", apiKey}},
            cpr::Body{requestBody.dump()}
        );

        // レスポンスのパースと embedding 配列の抽出
        try {
            if (r.status_code != 200) {
                std::wcout << "error , status_code:" << r.status_code << std::endl;
                throw std::runtime_error("API request failed with status ");
            }

            json response = json::parse(r.text);
            std::vector<float> embedding = response["embedding"]["values"]
                                                .get<std::vector<float>>();
            return embedding;
        } catch (const json::exception& e) {
            //throw std::runtime_error(std::string("JSON parse error: ") + e.what() + 
            //                         "\nRaw response: " + r.text);
            std::wcout << e.what() << std::endl;
            return ret;
        }
    }

    std::string rag_search(
        std::string query, 
        std::string api_key 
    ) 
    {
        std::string ret = "";
        try {
            auto embedding = getGeminiEmbedding(api_key, query);
            //std::wcout << L"Embedding dimensions: " << embedding.size() << std::endl;
            return ret;
        }
        catch (const std::exception& e) {
            std::wcerr << L"\n[ERROR] " << e.what() << L"\n";
            std::wcerr << L"Error , rag_search" << std::endl;
        }
        return ret;
    }    


};
