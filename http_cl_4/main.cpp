
#include <windows.h>
#include <chrono>
#include <fcntl.h>       // _O_U16TEXT
#include <iostream>
#include <io.h>          // _setmode
#include <string>
#include <stdexcept>
#include <curl/curl.h>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <shellapi.h>    // CommandLineToArgvW
#include <vector>
#include <thread>

#include "http_client.hpp"

#pragma comment(lib, "shell32.lib")

// JSON用エイリアス
using json = nlohmann::json;

struct QueryReq {
    std::string title;
};
// これ一行で、QueryReq <=> json の変換が魔法のように可能になります
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QueryReq, title)

struct RedisDataReq {
    std::string key;
    std::string value;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RedisDataReq, key, value)

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
int send_post_request(const std::string& url, const std::string& json_body) {
    HttpClient client(30 /*timeout*/, true /*verify_ssl*/);
    auto resp = client.post_json(url, json_body);
    if (!resp.error.empty()) {
        std::wcerr << L"[ERROR] \n";
        return 1;
    }
    std::cout << "Status : " << resp.status_code << "\n";
    return 0;
}


//スレッドから呼ばれる関数
void child_func(std::vector<RedisDataReq> items) {
    try{    
        std::string url = "http://localhost:8000/redis/add";

        HttpClient client(30 /*timeout*/, true /*verify_ssl*/);
        int i;
        for (i = 0; i < items.size(); i++) {
            RedisDataReq item = items[i];
            //std::string str_key = "SET " + item.key + " :" + item.value;
            RedisDataReq req;
            req.key = item.key;
            req.value = item.value;
            json j1 = req;
            std::string json_str = j1.dump();            
            //std::cout << "str_key=" << str_key << std::endl;
            auto resp = client.post_json(url, json_str);
            if (!resp.error.empty()) {
                std::wcerr << L"[ERROR] \n";
                return;
            }           
        }          
        std::cout << "#end-set" << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}
void print_items(const std::vector<RedisDataReq>& items){
    int i;
    for(i= 0; i< items.size(); i++ ){
        RedisDataReq item = items[i];
        std::cout << "key=" << item.key << ", value=" << item.value << std::endl;
    }
}

int main()
{
    std::vector<RedisDataReq> add_items;
    int i;
    for (i = 0; i < 100; i++) {
        RedisDataReq item;
        item.key = "k:" + std::to_string(i);
        item.value = "value" + std::to_string(i);
        add_items.push_back(item);
    }
    std::cout << "items-size=" << add_items.size() << std::endl;
    std::cout << "items-size=" << add_items.size() << std::endl;
    const int thread_num = 3;
    std::vector<RedisDataReq> div0_items;
    std::vector<RedisDataReq> div1_items;
    std::vector<RedisDataReq> div2_items;
    for (i = 0; i < add_items.size(); i++) {
        int remainder = i % thread_num;
        //std::cout << "remainder=" << remainder << std::endl;
        if(remainder == 0) {
            div0_items.push_back(add_items[i]);
        }
        if(remainder == 1) {
            div1_items.push_back(add_items[i]);
        }
        if(remainder == 2) {
            div2_items.push_back(add_items[i]);
        }
    }
    std::cout << "div0_items-size=" << div0_items.size() << std::endl;
    std::cout << "div1_items-size=" << div1_items.size() << std::endl;
    std::cout << "div2_items-size=" << div2_items.size() << std::endl; 
    //print_items(div0_items);

    try{
        // 開始時刻
        auto start = std::chrono::high_resolution_clock::now();

        std::thread t1(child_func, div0_items); //各スレッドの定義
        std::thread t2(child_func, div1_items);
        std::thread t3(child_func, div2_items);

        t1.join();
        t2.join();
        t3.join();

        // 終了時刻
        auto end = std::chrono::high_resolution_clock::now();
        // 差分（ミリ秒）
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "time: " << duration.count() << " ms" << std::endl;        

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << std::endl;
        return 0;
    }   
}
