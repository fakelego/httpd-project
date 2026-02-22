# Tinyhttpd 函数调用关系树

## 完整调用树

```
main()                                    [httpd.c:548]
│
├─ startup(&port)                         [httpd.c:45]
│  │
│  ├─ socket(PF_INET, SOCK_STREAM, 0)   [系统调用]
│  │  └─ 返回 server_sock
│  │
│  ├─ setsockopt(SO_REUSEADDR)           [系统调用]
│  │
│  ├─ bind(server_sock, &addr)           [系统调用]
│  │  ├─ 成功：绑定到指定端口
│  │  └─ 失败（端口0）：自动分配端口
│  │
│  ├─ getsockname()                      [系统调用]
│  │  └─ 获取实际端口号（如果是0）
│  │
│  └─ listen(server_sock, 5)             [系统调用]
│     └─ 返回 server_sock
│
└─ while(1) 主循环                        [httpd.c:560]
   │
   ├─ accept(server_sock, ...)           [系统调用]
   │  └─ 返回 client_sock
   │
   └─ pthread_create(&thread, NULL,       [系统调用]
                     accept_request,
                     (void*)client_sock)
      │
      └─ accept_request(client_sock)      [httpd.c:89] ← 新线程
         │
         ├─ get_line(client, buf, size)   [httpd.c:398]
         │  └─ recv(sock, &c, 1, 0)       [系统调用] ← 逐字节读取
         │
         ├─ 解析和判断（纯逻辑，无函数调用）
         │
         ├─ [分支 1] unimplemented(client) [httpd.c:523]
         │  ├─ sprintf(buf, "HTTP...")
         │  └─ send(client, buf, ...)     [系统调用]
         │
         ├─ [分支 2] not_found(client)    [httpd.c:447]
         │  ├─ sprintf(buf, "HTTP...")
         │  └─ send(client, buf, ...)     [系统调用]
         │
         ├─ [分支 3] serve_file(client, path) [httpd.c:468]
         │  │
         │  ├─ fopen(filename, "r")       [标准库]
         │  │
         │  ├─ headers(client, filename)  [httpd.c:426]
         │  │  ├─ sprintf(buf, "HTTP/1.0 200 OK\r\n")
         │  │  ├─ send(client, buf, ...)  [系统调用]
         │  │  ├─ sprintf(buf, SERVER_STRING)
         │  │  ├─ send(client, buf, ...)
         │  │  ├─ sprintf(buf, "Content-Type: text/html\r\n")
         │  │  ├─ send(client, buf, ...)
         │  │  └─ send(client, "\r\n", 2)
         │  │
         │  ├─ cat(client, resource)      [httpd.c:178]
         │  │  ├─ fgets(buf, size, resource) [标准库] ← 循环读取
         │  │  └─ send(client, buf, ...)   [系统调用]
         │  │
         │  └─ fclose(resource)           [标准库]
         │
         └─ [分支 4] execute_cgi(client, path, method, qs) [httpd.c:235]
            │
            ├─ get_line(client, buf, size) [多次调用，读取请求头]
            │
            ├─ pipe(cgi_output)            [系统调用]
            ├─ pipe(cgi_input)             [系统调用]
            │
            ├─ fork()                      [系统调用] ← 进程分叉点
            │
            ├─ [子进程] pid == 0
            │  ├─ dup2(cgi_output[1], STDOUT) [系统调用]
            │  ├─ dup2(cgi_input[0], STDIN)   [系统调用]
            │  ├─ close(...)                  [系统调用]
            │  ├─ putenv("REQUEST_METHOD=...") [标准库]
            │  ├─ putenv("QUERY_STRING=...")   [标准库]
            │  ├─ execl(path, path, NULL)      [系统调用] ← 替换进程
            │  └─ exit(0)                      [系统调用]
            │
            └─ [父进程] pid > 0
               ├─ close(cgi_output[1])      [系统调用]
               ├─ close(cgi_input[0])       [系统调用]
               │
               ├─ [POST] for (i=0; i<content_length; i++)
               │  ├─ recv(client, &c, 1)    [系统调用]
               │  └─ write(cgi_input[1], &c, 1) [系统调用]
               │
               ├─ while (read(cgi_output[0], &c, 1) > 0)
               │  └─ send(client, &c, 1)    [系统调用]
               │
               ├─ close(cgi_output[0])      [系统调用]
               ├─ close(cgi_input[1])       [系统调用]
               └─ waitpid(pid, &status, 0)  [系统调用]
```

## 调用深度统计

| 路径 | 调用深度 | 说明 |
|------|---------|------|
| main → startup | 2 | 初始化 |
| main → accept_request → serve_file → cat | 4 | 静态文件 |
| main → accept_request → execute_cgi | 3 | CGI（父进程） |
| main → accept_request → execute_cgi → execl | 4 | CGI（子进程） |

## 系统调用统计

| 系统调用 | 调用位置 | 调用次数 | 说明 |
|---------|---------|---------|------|
| `socket` | startup | 1 | 创建 socket |
| `bind` | startup | 1 | 绑定端口 |
| `listen` | startup | 1 | 开始监听 |
| `accept` | main | N | 每个连接1次 |
| `pthread_create` | main | N | 每个连接1次 |
| `recv` | get_line, execute_cgi | 多次 | 读取数据 |
| `send` | 各处 | 多次 | 发送数据 |
| `fork` | execute_cgi | 每个CGI 1次 | 创建子进程 |
| `execl` | execute_cgi (子进程) | 每个CGI 1次 | 执行CGI |
| `pipe` | execute_cgi | 每个CGI 2次 | 创建管道 |
| `close` | 各处 | 多次 | 关闭文件描述符 |

## 函数类别

### 网络相关
- startup() - 初始化服务器
- accept_request() - 主处理函数

### 请求解析
- get_line() - 逐行读取

### 响应生成
- headers() - 发送响应头
- cat() - 发送文件内容

### 错误处理
- bad_request() - 400
- not_found() - 404
- cannot_execute() - 500
- unimplemented() - 501
- error_die() - 致命错误

### 内容处理
- serve_file() - 静态文件
- execute_cgi() - CGI 脚本

## 关键路径分析

### 路径 1：静态文件请求

```
请求：GET /index.html HTTP/1.1

main
 └─ accept (client_sock = 4)
     └─ pthread_create
         └─ accept_request(4)
             ├─ get_line → "GET /index.html HTTP/1.1"
             ├─ 解析：method=GET, url=/index.html
             ├─ cgi = 0 （无查询，无执行权限）
             └─ serve_file(4, "htdocs/index.html")
                 ├─ headers(4, "htdocs/index.html")
                 │   └─ send → "HTTP/1.0 200 OK\r\n..."
                 ├─ cat(4, file)
                 │   └─ send → <html>...</html>
                 └─ close(4)

总系统调用：
- accept: 1
- pthread_create: 1
- recv: ~10 (读取请求)
- send: ~50 (发送响应)
- fopen: 1
- fread: N (文件大小)
- close: 1
```

### 路径 2：CGI 请求（GET）

```
请求：GET /color.cgi?color=red HTTP/1.1

main
 └─ accept (client_sock = 4)
     └─ pthread_create
         └─ accept_request(4)
             ├─ get_line → "GET /color.cgi?color=red HTTP/1.1"
             ├─ 解析：method=GET, url=/color.cgi, qs=color=red
             ├─ cgi = 1 （有查询字符串）
             └─ execute_cgi(4, "htdocs/color.cgi", "GET", "color=red")
                 ├─ pipe(cgi_output) → [3,5]
                 ├─ pipe(cgi_input) → [6,7]
                 ├─ fork() → pid=1234
                 │
                 ├─ [子进程 pid=0]
                 │   ├─ dup2(5, STDOUT) → CGI输出到管道
                 │   ├─ dup2(6, STDIN)  → 管道输入到CGI
                 │   ├─ putenv("REQUEST_METHOD=GET")
                 │   ├─ putenv("QUERY_STRING=color=red")
                 │   ├─ execl("htdocs/color.cgi", "htdocs/color.cgi", NULL)
                 │   └─ [进程被替换为 Perl 解释器]
                 │
                 └─ [父进程 pid=1234]
                     ├─ close(5, 6)
                     ├─ read(cgi_output) → 读取CGI输出
                     │   └─ "HTTP/1.0 200 OK\r\n<html>..."
                     ├─ send(4, ...) → 转发给客户端
                     ├─ waitpid(1234) → 等待子进程结束
                     └─ close(3, 7, 4)

总系统调用：
- accept: 1
- pthread_create: 1
- pipe: 2
- fork: 1
- dup2: 2 (子进程)
- execl: 1 (子进程)
- recv: ~10
- read: N (CGI输出)
- write: 0 (GET无请求体)
- send: N
- close: ~6
- waitpid: 1
```

## 性能瓶颈分析

### 瓶颈 1：get_line() 逐字节读取

```c
// 当前实现
while (...) {
    n = recv(sock, &c, 1, 0);  // 每次读1字节！
}

// 系统调用开销：
// - 每次 recv() 都是一次系统调用
// - 用户态 ↔ 内核态切换
// - 如果请求头有10行，每行20字节
//   → 200 次系统调用！

// 改进方案：
// - 一次读取更多数据到缓冲区
// - 在缓冲区中查找换行符
// - 减少系统调用次数
```

### 瓶颈 2：cat() 逐字节发送

```c
// 当前实现
while (fgets(buf, sizeof(buf), resource) != NULL)
    send(client, buf, strlen(buf), 0);

// 问题：
// - fgets 按行读取
// - send 多次调用
// - 对大文件效率低

// 改进方案：
// - 使用 sendfile() 系统调用（零拷贝）
// - 或增大缓冲区，减少 send 次数
```

### 瓶颈 3：每请求一线程

```c
// 当前实现
while (1) {
    client = accept(...);
    pthread_create(..., accept_request, client);
}

// 问题：
// - 线程创建/销毁开销大
// - 大量并发时，线程数爆炸
// - 内存占用高（每线程 ~8MB 栈）

// 改进方案：
// - 使用线程池
// - 预创建固定数量线程
// - 通过任务队列分配工作
```

## 内存使用分析

### 每个请求的内存占用

```
线程栈：8 MB（默认）
accept_request 局部变量：~2 KB
  - buf[1024]: 1 KB
  - method[255]: 255 bytes
  - url[255]: 255 bytes
  - path[512]: 512 bytes
  - 其他: ~200 bytes

如果 100 个并发请求：
- 100 线程 × 8 MB = 800 MB（栈）
- 100 × 2 KB = 200 KB（局部变量）
- 总计：~800 MB

使用线程池（8个线程）：
- 8 × 8 MB = 64 MB（栈）
- 8 × 2 KB = 16 KB（局部变量）
- 总计：~64 MB
节省：~740 MB！
```
