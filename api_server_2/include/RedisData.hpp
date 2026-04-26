#pragma once
#include <iostream>
#include <chrono>
#include <string>
#include <thread>
#include <nlohmann/json.hpp> // JSONライブラリ

#include <hiredis/hiredis.h>

using json = nlohmann::json;

struct RedisDataReq {
    std::string key;
    std::string value;
};

class RedisData {
private:
    redisContext* m_conn = nullptr;

public:
    explicit RedisData(std::string str){
        m_conn = redisConnect("127.0.0.1", 6379);
    }

    int set(const std::string& key, const std::string& value) {
        redisReply* reply = (redisReply*)redisCommand(m_conn, "SET %s %s", key.c_str(), value.c_str());
        if (reply == nullptr) {
            std::cerr << "Failed to execute SET command: " << m_conn->errstr << std::endl;
            return -1;
        }
        freeReplyObject(reply);
        return 0;        
    }

    std::string get(const std::string& key) {
        redisReply* reply = (redisReply*)redisCommand(m_conn, "GET %s", key.c_str());
        if (reply == nullptr) {
            std::cerr << "Failed to execute GET command: " << m_conn->errstr << std::endl;
            return "";
        }
        std::string value = reply->str ? reply->str : "";
        freeReplyObject(reply);
        return value;
    }


    ~RedisData() {
        redisFree(m_conn); 
    }

    /**
    *
    * @param
    *
    * @return
    */
    void add_handler(const httplib::Request& req, httplib::Response& res) {
        std::string ret = "";
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            json j = json::parse(req.body);

            std::string key = j.at("key").get<std::string>();
            std::cout << "key=" << key << "\n";

            std::string value = j.at("value").get<std::string>();
            std::cout << "value=" << value << "\n";
            int ret = set(key, value);
            if (ret != 0) {
                res.status = 500;
                res.set_content("Failed to set value in Redis", "text/plain");
                return;
            }
            res.status = 201;
            res.set_content("OK", "application/json");
            return;
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
            return;
        }        
    }    

    void get_handler(const httplib::Request& req, httplib::Response& res) {
        std::string ret = "";
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            json j = json::parse(req.body);

            std::string key = j.at("key").get<std::string>();
            std::cout << "key=" << key << "\n";

            std::string ret = get(key);
            std::cout << "ret=" << ret << "\n";
            if (ret.empty()) {
                res.status = 500;
                res.set_content("Failed to set value in Redis", "text/plain");
                return;
            }
            res.status = 201;
            res.set_content(ret, "application/json");
            return;
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
            return;
        }        
    }    
};
