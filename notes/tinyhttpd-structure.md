# Tinyhttpd 源码结构分析

## 目录结构
Tinyhttpd/
    ├── LICENSE
    ├── Makefile # 构建文件
    ├── README.md # 说明文档
    ├── htdocs/ # 网站根目录 
    │   ├── README
    │   ├── check.cgi
    │   ├── color.cgi # CGI
    │   └── index.html # 首页
    ├── httpd.c # 主程序（约600行）
    └── simpleclient.c

## 主要文件
- httpd.c: 所有代码都在一个文件中
- 行数: 约 600 行
- 函数数量: 13+ 个

## 核心函数列表
- 37:void accept_request(void *);
- 38:void bad_request(int);
- 39:void cat(int, FILE *);
- 40:void cannot_execute(int);
- 41:void error_die(const char *);
- 42:void execute_cgi(int, const char *, const char *, const char *);
- 43:int get_line(int, char *, int);
- 44:void headers(int, const char *);
- 45:void not_found(int);
- 46:void serve_file(int, const char *);
- 47:int startup(u_short *);
- 48:void unimplemented(int);
- 55:void accept_request(void *arg)
- 140:void bad_request(int client)
- 163:void cat(int client, FILE *resource)
- 179:void cannot_execute(int client)
- 198:void error_die(const char *sc)
- 210:void execute_cgi(int client, const char *path,
- 314:int get_line(int sock, char *buf, int size)
- 351:void headers(int client, const char *filename)
- 369:void not_found(int client)
- 400:void serve_file(int client, const char *filename)
- 429:int startup(u_short *port)
- 465:void unimplemented(int client)
- 489:int main(void) 

## 依赖的系统库
- sys/socket.h: Socket 编程
- pthread.h: 多线程
- sys/stat.h: 文件状态
- unistd.h: Unix标准

## 编译要求
- GCC
- pthread 库
- 无其他外部依赖EOF

