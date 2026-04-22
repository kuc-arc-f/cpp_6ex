#include <iostream>
#include <fstream>
#include <string>
#include "nlohmann/json.hpp"

// JSON用エイリアス
using json = nlohmann::json;

struct QueryReq {
    std::string content;
    std::string username;
};
// これ一行で、QueryReq <=> json の変換が魔法のように可能になります
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QueryReq, content, username)

int main(int argc, char* argv[])
{
    try{
        QueryReq req;
        req.content = "test";
        req.username = "Webhook-4";
        json j = req; // 構造体を代入するだけ！
        std::string json_str = j.dump();
        std::cout << json_str << std::endl;

        //decode
        std::string target = R"({"title": "hello world"})";
        json j1 = json::parse(target);
        std::string t1 = j1.at("title").get<std::string>();
        std::cout << "title=" << t1 << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error , main" << std::endl;
        return 0;
    }    

    return 0;
}