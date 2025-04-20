#include "HttpClient.hpp"
#include <httplib.h>
#include <thread>
#include <stdexcept>
#include <regex>
#include <locale>
#include <codecvt>

// 辅助函数：转换字符串编码
static std::string convert_to_utf8(const std::string &input, const std::string &from_charset)
{
    if (from_charset == "utf-8" || from_charset == "UTF-8")
    {
        return input;
    }

    // 先将本地多字节编码转换为 UTF-16
    int wide_len = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, nullptr, 0);
    std::wstring wide(wide_len, 0);
    MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, &wide[0], wide_len);

    // 再将 UTF-16 转换为 UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(utf8_len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], utf8_len, nullptr, nullptr);

    return utf8;
}
// URL解析辅助函数
static void parse_url(const std::string &url,
                      std::string &protocol,
                      std::string &host,
                      std::string &path,
                      int &port)
{
    protocol = "http";
    host = "";
    path = "/";
    port = 80;

    std::regex url_regex(R"((https?)://([^/ :]+)(:(\d+))?(/[^ ]*)?)");
    std::smatch match;

    if (std::regex_match(url, match, url_regex))
    {
        protocol = match[1];
        host = match[2];

        if (match[4].matched)
        {
            port = std::stoi(match[4]);
        }
        else
        {
            port = (protocol == "https") ? 443 : 80;
        }

        if (match[5].matched)
        {
            path = match[5];
        }
    }
}
static std::string build_query_params(const std::string& url, const sol::table& data) {
    std::ostringstream oss;
    bool first = url.find('?') == std::string::npos;

    for (const auto& [k, v] : data) {
        std::string key = k.as<std::string>();
        std::string val = v.as<std::string>();
        oss << (first ? '?' : '&') << httplib::detail::encode_url(key) << '=' << httplib::detail::encode_url(val);
        first = false;
    }

    return url + oss.str();
}

template <typename ClientType, typename RequestFunc>
void perform_request(ClientType& cli,
                     const std::string& url,
                     const std::string& path,
                     std::shared_ptr<std::promise<HttpResponse>> promise,
                     RequestFunc do_request) {
    cli.set_follow_location(true);

    if (auto res = do_request(cli, path)) {
        HttpResponse response;
        response.url = url;
        response.body = res->body;
        response.status = res->status;

        std::string content_type = res->get_header_value("Content-Type");
        std::string charset = "utf-8";
        size_t charset_pos = content_type.find("charset=");
        if (charset_pos != std::string::npos) {
            charset = content_type.substr(charset_pos + 8);
            if (charset.find(';') != std::string::npos) {
                charset = charset.substr(0, charset.find(';'));
            }
        }

        response.body = convert_to_utf8(res->body, charset);

        std::map<std::string, std::string> headers;
        for (const auto& h : res->headers) {
            if (auto it = headers.find(h.first); it != headers.end()) {
                it->second += ", " + h.second;
            } else {
                headers[h.first] = h.second;
            }
        }
        response.headers = headers;

        promise->set_value(response);
    } else {
        throw std::runtime_error("请求失败: " + std::string(httplib::to_string(res.error())));
    }
}


HttpFuture::HttpFuture(std::shared_ptr<std::future<HttpResponse>> future)
    : future_(std::move(future)) {}

HttpFuture& HttpFuture::then_(sol::function onSuccess)
{
    std::thread([future = this->future_, onSuccess]() {

        try {
            auto result = future->get();
            sol::state_view lua = onSuccess.lua_state();
            sol::table response = lua.create_table();
            
            response["url"] = result.url;
            response["data"] = result.body;
            response["status"] = result.status;
            
            sol::table headers = lua.create_table();
            for (const auto& hdr : result.headers) {
                headers[hdr.first] = hdr.second;
            }
            response["headers"] = headers;
            
            onSuccess(response);
        } catch (...) {
            // 忽略成功回调中的异常
        } })
        .detach();

        return *this;
}

HttpFuture& HttpFuture::catch_(sol::function onError) {
    std::thread([future = this->future_, onError]() {

        try {
            auto result = future->get();
            sol::state_view lua = onError.lua_state();
            sol::table response = lua.create_table();
            
            response["url"] = result.url;
            response["data"] = result.body;
            response["status"] = result.status;
            
            sol::table headers = lua.create_table();
            for (const auto& hdr : result.headers) {
                headers[hdr.first] = hdr.second;
            }
            response["headers"] = headers;
            
            onError(response);
        } catch (const std::exception& ex) {
            // 捕获标准异常
            onError(ex.what());
        } catch (...) {
            // 捕获其他未知异常
            onError("Unknown exception occurred");
        }
    }).detach();

    return *this;
}

HttpClient::HttpClient() = default;

void HttpClient::add_header(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(headers_mutex_);
    headers_[key] = value;
}

void HttpClient::remove_header(const std::string& key) {
    std::lock_guard<std::mutex> lock(headers_mutex_);
    headers_.erase(key);
}

HttpFuture HttpClient::get(const std::string &url)
{

    auto promise = std::make_shared<std::promise<HttpResponse>>();
    auto future = std::make_shared<std::future<HttpResponse>>(promise->get_future());

    std::thread([=]() {
        try {
            std::string protocol, host, path;
            int port;
            parse_url(url, protocol, host, path, port);

            httplib::Headers headers;
            {
                std::lock_guard<std::mutex> lock(headers_mutex_);
                for (const auto& [k, v] : headers_) {
                    headers.emplace(k, v);
                }
            }

            if (protocol == "https") {
                httplib::SSLClient cli(host, port);
                perform_request(cli, url, path, promise,
                    [headers](auto& cli, const std::string& path) {
                        return cli.Get(path.c_str(), headers);
                    });
            } else {
                httplib::Client cli(host, port);
                perform_request(cli, url, path, promise,
                    [headers](auto& cli, const std::string& path) {
                        return cli.Get(path.c_str(), headers);
                    });
            }
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }).detach();

    return HttpFuture(future);
}

HttpFuture HttpClient::post(const std::string &url, const std::string &body)
{
    auto promise = std::make_shared<std::promise<HttpResponse>>();
    auto future = std::make_shared<std::future<HttpResponse>>(promise->get_future());

    std::thread([=]() {
        try {
            std::string protocol, host, path;
            int port;
            parse_url(url, protocol, host, path, port);

            httplib::Headers headers;
            {
                std::lock_guard<std::mutex> lock(headers_mutex_);
                for (const auto& [k, v] : headers_) {
                    headers.emplace(k, v);
                }
            }

            if (protocol == "https") {
                httplib::SSLClient cli(host, port);
                perform_request(cli, url, path, promise,
                    [&](auto& cli, const std::string& path) {
                        return cli.Post(path.c_str(), headers, body, "application/json");
                    });
            } else {
                httplib::Client cli(host, port);
                perform_request(cli, url, path, promise,
                    [&](auto& cli, const std::string& path) {
                        return cli.Post(path.c_str(), headers, body, "application/json");
                    });
            }
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }).detach();

    return HttpFuture(future);
}