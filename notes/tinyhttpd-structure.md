# Tinyhttpd 源码结构分析

## 目录结构
```
Tinyhttpd/ ├── httpd.c # 主程序（约 600 行） ├── Makefile # 构建文件 ├── htdocs/ # 网站根目录 │ ├── index.html # 首页 │ ├── color.cgi # CGI 示例 │ └── ... └── README.md # 说明文档
```

## 主要文件
- httpd.c: 所有代码都在一个文件中
- 行数: 约 600 行
- 函数数量: 13+ 个

## 核心函数列表
[使用 grep 命令获取]
```bash
grep -n "^void\|^int\|^unsigned" httpd.c | grep "("
```

## 依赖的系统库
- sys/socket.h: Socket 编程
- pthread.h: 多线程
- sys/stat.h: 文件状态
- unistd.h: Unix标准

## 编译要求
- GCC
- pthread 库
- 无其他外部依赖EOF

