#include <Windows.h>
#include <sol/sol.hpp>
#include "HttpClient.hpp"
#include "reframework/API.hpp"

using API = reframework::API;

void on_lua_state_created(lua_State* l) {
    API::LuaLock _{};

    sol::state_view lua{l};

    lua.new_usertype<HttpClient>("HttpClient",
        sol::constructors<HttpClient()>(),
        "get", &HttpClient::get,
        "post", &HttpClient::post,
        "add_header", &HttpClient::add_header,
        "remove_header", &HttpClient::remove_header
    );

    lua.new_usertype<HttpFuture>("HttpFuture",
        "then_", &HttpFuture::then_,
        "error_", &HttpFuture::catch_
    );
}

extern "C" __declspec(dllexport) bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam* param) {
    API::initialize(param);
    const auto functions = param->functions;
    functions->on_lua_state_created(on_lua_state_created);

    return true;
}