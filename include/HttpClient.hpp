#pragma once
#include <string>
#include <future>
#include <memory>
#include <map>
#include <sol/sol.hpp>

struct HttpResponse {
    std::string url;
    std::string body;
    std::map<std::string, std::string> headers;
    int status;
};

class HttpFuture {
public:
    HttpFuture(std::shared_ptr<std::future<HttpResponse>> future);
    HttpFuture& then_(sol::function onSuccess);
    HttpFuture& catch_(sol::function onError);

private:
    std::shared_ptr<std::future<HttpResponse>> future_;
};

class HttpClient {
public:
    HttpClient();
    HttpFuture get(const std::string& url);
    HttpFuture post(const std::string& url, const std::string& body);
    void add_header(const std::string& key, const std::string& value);
    void remove_header(const std::string& key);
private:
    std::map<std::string, std::string> headers_;
    std::mutex headers_mutex_;
};