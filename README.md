﻿# Reframework HTTP 客户端插件

一个为 Reframework Lua 环境提供的异步 HTTP 客户端插件，支持 GET/POST 请求、自定义请求头。

---

## 安装

1. 将 `HttpClient.dll` 文件放入 Reframework 的 `plugins` 目录

---

## 基本用法

### 初始化客户端

```lua
local client = HttpClient.new()
client:add_header("User-Agent", "MyLuaClient/1.0")
```

### 发起 GET 请求

```lua
client:get("https://api.example.com/data")
    :then_(function(response)
        print("Status:", response.status)
        print("Body:", response.data)
    end)
```

### 发起 POST 请求

```lua
client:post("https://api.example.com/submit", "{'key':'value'}")
    :then_(function(response)
        -- 处理响应
    end)
```

---

## API 文档

### HttpClient

| 方法 | 参数 | 描述 |
|------|------|------|
| `new()` | - | 创建新客户端实例 |
| `get(url)` | `url`: 请求地址 | 发起 GET 请求 |
| `post(url, body)` | `url`: 请求地址<br>`body`: 请求体 | 发起 POST 请求 |
| `add_header(key, value)` | `key`: 头名称<br>`value`: 头值 | 添加请求头 |
| `remove_header(key)` | `key`: 要删除的头名称 | 移除请求头 |

### HttpFuture

| 方法 | 参数 | 描述 |
|------|------|------|
| `then_(callback)` | `callback(response)` | 设置响应回调函数 |

---

## 响应对象格式

```lua
{
    url = "原始请求URL",
    data = "响应内容",
    status = HTTP状态码,
    headers = { -- 响应头表
        ["Header-Name"] = "HeaderValue",
        ...
    }
}
```