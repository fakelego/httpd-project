# accept_request() 详细流程图

## 主流程图

```mermaid
graph TB
    Start([开始]) --> ReadLine[读取 HTTP 请求行<br/>get_line]
    ReadLine --> ParseMethod[解析 HTTP 方法<br/>提取第一个单词]
    ParseMethod --> CheckMethod{方法是<br/>GET 或 POST?}
    
    CheckMethod -->|否| Return501[unimplemented<br/>返回 501]
    Return501 --> End
    
    CheckMethod -->|是| IsPOST{是 POST?}
    IsPOST -->|是| SetCGI1[设置 cgi = 1]
    IsPOST -->|否| ParseURL
    SetCGI1 --> ParseURL[解析 URL<br/>提取第二个单词]
    
    ParseURL --> IsGET{是 GET?}
    IsGET -->|否| BuildPath
    IsGET -->|是| FindQuery[在 URL 中<br/>查找 '?']
    
    FindQuery --> HasQuery{找到 '?'?}
    HasQuery -->|是| SplitURL[分离 URL 和<br/>查询字符串<br/>设置 cgi = 1]
    HasQuery -->|否| BuildPath
    SplitURL --> BuildPath[构建文件路径<br/>path = htdocs + url]
    
    BuildPath --> EndsSlash{路径以 / 结尾?}
    EndsSlash -->|是| AppendIndex[追加 index.html]
    EndsSlash -->|否| CheckFile
    AppendIndex --> CheckFile{文件存在?<br/>stat}
    
    CheckFile -->|否| DiscardHeaders[丢弃剩余请求头]
    DiscardHeaders --> Return404[not_found<br/>返回 404]
    Return404 --> CloseSocket
    
    CheckFile -->|是| IsDir{是目录?}
    IsDir -->|是| AppendIndex2[追加 /index.html]
    IsDir -->|否| CheckExec
    AppendIndex2 --> CheckExec{有执行权限?}
    
    CheckExec -->|是| SetCGI2[设置 cgi = 1]
    CheckExec -->|否| Route
    SetCGI2 --> Route{cgi == 1?}
    
    Route -->|是| CallCGI[execute_cgi<br/>执行 CGI 脚本]
    Route -->|否| CallServe[serve_file<br/>发送静态文件]
    
    CallCGI --> CloseSocket[关闭 socket<br/>close]
    CallServe --> CloseSocket
    CloseSocket --> End([结束])
```

## cgi 标志设置追踪

```mermaid
stateDiagram-v2
    [*] --> cgi_0: 初始化 cgi = 0
    
    cgi_0 --> cgi_1_post: POST 方法
    cgi_0 --> cgi_1_query: URL 有 '?'
    cgi_0 --> cgi_1_exec: 文件可执行
    cgi_0 --> cgi_0_static: 以上都不满足
    
    cgi_1_post --> [*]: 路由到 execute_cgi
    cgi_1_query --> [*]: 路由到 execute_cgi
    cgi_1_exec --> [*]: 路由到 execute_cgi
    cgi_0_static --> [*]: 路由到 serve_file
```

## 数据流示例

### 示例 1：静态文件

```
输入：
  客户端请求："GET /index.html HTTP/1.1"

处理流程：
  ┌─────────────────────────────────┐
  │ 读取请求行                      │
  │ buf = "GET /index.html HTTP/1.1"│
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 解析方法                        │
  │ method = "GET"                  │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 解析 URL                        │
  │ url = "/index.html"             │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ GET 方法，查找 '?'              │
  │ 没有找到                        │
  │ cgi 保持为 0                    │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 构建路径                        │
  │ path = "htdocs/index.html"      │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ stat() 检查文件                 │
  │ 文件存在，不是目录              │
  │ 没有执行权限                    │
  │ cgi 保持为 0                    │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 路由                            │
  │ cgi == 0                        │
  │ → serve_file(client, path)      │
  └─────────────────────────────────┘

输出：
  发送 htdocs/index.html 的内容
```

### 示例 2：CGI（带查询字符串）

```
输入：
  客户端请求："GET /color.cgi?color=red HTTP/1.1"

处理流程：
  ┌─────────────────────────────────────────┐
  │ 读取请求行                              │
  │ buf = "GET /color.cgi?color=red HTTP..." │
  └──────────┬──────────────────────────────┘
             ↓
  ┌─────────────────────────────────────────┐
  │ 解析方法                                │
  │ method = "GET"                          │
  └──────────┬──────────────────────────────┘
             ↓
  ┌─────────────────────────────────────────┐
  │ 解析 URL                                │
  │ url = "/color.cgi?color=red"            │
  └──────────┬──────────────────────────────┘
             ↓
  ┌─────────────────────────────────────────┐
  │ GET 方法，查找 '?'                      │
  │ 找到了！                                │
  │ 设置 cgi = 1                            │
  │ 分离：url = "/color.cgi"                │
  │      query_string = "color=red"         │
  └──────────┬──────────────────────────────┘
             ↓
  ┌─────────────────────────────────────────┐
  │ 构建路径                                │
  │ path = "htdocs/color.cgi"               │
  └──────────┬──────────────────────────────┘
             ↓
  ┌─────────────────────────────────────────┐
  │ stat() 检查文件                         │
  │ 文件存在，不是目录                      │
  │ 有执行权限（但 cgi 已经是 1 了）       │
  └──────────┬──────────────────────────────┘
             ↓
  ┌─────────────────────────────────────────┐
  │ 路由                                    │
  │ cgi == 1                                │
  │ → execute_cgi(client, path, method, qs) │
  └─────────────────────────────────────────┘

输出：
  执行 htdocs/color.cgi，传递查询字符串
  发送 CGI 的输出
```

### 示例 3：CGI（POST）

```
输入：
  客户端请求："POST /form.cgi HTTP/1.1"
  请求体："name=Alice&age=25"

处理流程：
  ┌─────────────────────────────────┐
  │ 读取请求行                      │
  │ buf = "POST /form.cgi HTTP/1.1" │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 解析方法                        │
  │ method = "POST"                 │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 检查方法                        │
  │ 是 POST → 设置 cgi = 1          │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 解析 URL                        │
  │ url = "/form.cgi"               │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 不是 GET，跳过查询字符串处理    │
  │ query_string = NULL             │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 构建路径                        │
  │ path = "htdocs/form.cgi"        │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ stat() 检查文件                 │
  │ 文件存在，有执行权限            │
  │ （cgi 已经是 1）                │
  └──────────┬──────────────────────┘
             ↓
  ┌─────────────────────────────────┐
  │ 路由                            │
  │ cgi == 1                        │
  │ → execute_cgi(client, path, ...)│
  └─────────────────────────────────┘

输出：
  execute_cgi 会读取请求体
  传递给 CGI 脚本
  发送 CGI 的输出
```

## 错误处理流程

```mermaid
graph LR
    A[accept_request] --> B{错误类型}
    B -->|方法不支持| C[unimplemented<br/>501]
    B -->|文件不存在| D[not_found<br/>404]
    B -->|其他| E[继续处理]
```

## 关键代码片段映射

| 流程步骤 | 代码位置 | 关键变量 |
|---------|---------|---------|
| 读取请求行 | Line 93 | `buf`, `numchars` |
| 解析方法 | Line 96-103 | `method`, `i`, `j` |
| 检查方法 | Line 105-109 | `method` |
| POST 处理 | Line 111-112 | `cgi` |
| 解析 URL | Line 114-123 | `url`, `i`, `j` |
| 查询字符串 | Line 125-137 | `query_string`, `cgi` |
| 构建路径 | Line 139-141 | `path` |
| 文件检查 | Line 143-155 | `st`, `cgi` |
