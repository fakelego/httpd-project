# accept_request() 变量状态追踪表

## 示例请求：GET /color.cgi?color=red HTTP/1.1

| 步骤           | 代码行  | client | buf                                 | method | url                     | query_string | path               | cgi | 说明                   |
|----------------|---------|--------|-------------------------------------|--------|-------------------------|--------------|--------------------|-----|------------------------|
| **初始化**     | 89      | 4      | ""                                  | ""     | ""                      | NULL         | ""                 | 0   | 函数开始               |
| **读取请求行** | 93      | 4      | "GET /color.cgi?color=red HTTP/1.1" | ""     | ""                      | NULL         | ""                 | 0   | get_line 返回          |
| **解析方法**   | 96-103  | 4      | "GET..."                            | "GET"  | ""                      | NULL         | ""                 | 0   | 提取第一个单词         |
| **检查方法**   | 105-109 | 4      | "GET..."                            | "GET"  | ""                      | NULL         | ""                 | 0   | 方法支持，继续         |
| **POST检查**   | 111-112 | 4      | "GET..."                            | "GET"  | ""                      | NULL         | ""                 | 0   | 不是POST，跳过         |
| **解析URL**    | 114-123 | 4      | "GET..."                            | "GET"  | "/color.cgi?color=red"  | NULL         | ""                 | 0   | 提取第二个单词         |
| **查找'?'**    | 128-129 | 4      | "GET..."                            | "GET"  | "/color.cgi?color=red"  | 指向'?'      | ""                 | 0   | 找到查询字符串         |
| **设置CGI**    | 131     | 4      | "GET..."                            | "GET"  | "/color.cgi?color=red"  | 指向'?'      | ""                 | 1   | 有查询参数             |
| **截断URL**    | 132     | 4      | "GET..."                            | "GET"  | "/color.cgi\0color=red" | 指向'?'      | ""                 | 1   | URL被截断              |
| **指向参数**   | 133     | 4      | "GET..."                            | "GET"  | "/color.cgi"            | "color=red"  | ""                 | 1   | 指针前进               |
| **构建路径**   | 139     | 4      | "GET..."                            | "GET"  | "/color.cgi"            | "color=red"  | "htdocs/color.cgi" | 1   | 拼接路径               |
| **检查'/'**    | 140-141 | 4      | "GET..."                            | "GET"  | "/color.cgi"            | "color=red"  | "htdocs/color.cgi" | 1   | 不是目录访问           |
| **stat()**     | 143     | 4      | "GET..."                            | "GET"  | "/color.cgi"            | "color=red"  | "htdocs/color.cgi" | 1   | 文件存在               |
| **检查目录**   | 147-148 | 4      | "GET..."                            | "GET"  | "/color.cgi"            | "color=red"  | "htdocs/color.cgi" | 1   | 不是目录               |
| **检查权限**   | 149-152 | 4      | "GET..."                            | "GET"  | "/color.cgi"            | "color=red"  | "htdocs/color.cgi" | 1   | 有执行权限             |
| **路由**       | 153-156 | 4      | "GET..."                            | "GET"  | "/color.cgi"            | "color=red"  | "htdocs/color.cgi" | 1   | cgi==1,调用execute_cgi |

## 关键观察

### URL 的分离技巧
```c
// 初始状态
url = "/color.cgi?color=red"
query_string = url  // 指向开头

// 查找 '?'
while (*query_string != '?') query_string++;
// 现在 query_string 指向 '?'

// 分离
*query_string = '\0';  // URL 变成 "/color.cgi\0color=red"
query_string++;        // 指向 "color=red"

// 结果
url = "/color.cgi"           // 字符串到 \0 结束
query_string = "color=red"   // 指向参数部分
```

### cgi 标志的三个设置点
1. **Line 112**: `if (POST) cgi = 1`
2. **Line 131**: `if (found '?') cgi = 1`
3. **Line 152**: `if (executable) cgi = 1`

### 内存布局（重要！）

```
栈内存布局：
┌──────────────────────────┐ ← 高地址
│ ...                      │
├──────────────────────────┤
│ client (int, 4 bytes)    │
├──────────────────────────┤
│ buf[1024]                │ ← 1024 字节
├──────────────────────────┤
│ method[255]              │ ← 255 字节
├──────────────────────────┤
│ url[255]                 │ ← 255 字节（被修改）
├──────────────────────────┤
│ path[512]                │ ← 512 字节
├──────────────────────────┤
│ i, j (size_t, 8字节各)   │
├──────────────────────────┤
│ st (struct stat)         │ ← ~144 字节
├──────────────────────────┤
│ cgi (int, 4 bytes)       │
├──────────────────────────┤
│ query_string (指针,8字节)│ ← 指向 url 数组内部
└──────────────────────────┘ ← 低地址

总栈空间约：1024 + 255 + 255 + 512 + 160 ≈ 2.2 KB
```

## SDS 替换影响分析

| 变量     | 当前类型     | 固定大小 | SDS 类型 | 节省/改进                                |
|----------|--------------|----------|----------|------------------------------------------|
| `buf`    | `char[1024]` | 1024     | `sds`    | 动态分配，按需扩展                       |
| `method` | `char[255]`  | 255      | `sds`    | 典型大小 3-4 字节（GET/POST），节省 250+ |
| `url`    | `char[255]`  | 255      | `sds`    | 动态分配，支持长 URL                     |
| `path`   | `char[512]`  | 512      | `sds`    | 动态分配，支持深路径                     |

**总节省栈空间**：~2 KB → ~100 bytes（指针）  
**额外开销**：堆分配时间 + SDS 头部（~3-5 bytes per string）  
**收益**：安全性↑，灵活性↑，代码可读性↑
