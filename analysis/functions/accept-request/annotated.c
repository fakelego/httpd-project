/**********************************************************************/
/* 
 * 函数名：accept_request
 * 
 * 功能概述：
 *   处理客户端的 HTTP 请求，是 Tinyhttpd 的核心处理函数
 * 
 * 参数：
 *   void *arg - 客户端 socket 文件描述符
 *               通过 pthread_create 传入，需要类型转换
 * 
 * 返回值：
 *   void* - 线程函数要求的返回类型，实际不返回值
 * 
 * 调用者：
 *   main() → pthread_create() → accept_request()
 * 
 * 被调用者：
 *   get_line()       - 读取 HTTP 请求行
 *   unimplemented()  - 返回 501 错误
 *   bad_request()    - 返回 400 错误
 *   not_found()      - 返回 404 错误
 *   serve_file()     - 处理静态文件
 *   execute_cgi()    - 执行 CGI 脚本
 * 
 * 工作流程：
 *   1. 读取并解析 HTTP 请求行
 *   2. 判断请求方法（GET/POST）
 *   3. 解析 URL 和查询字符串
 *   4. 判断请求类型（静态 vs CGI）
 *   5. 路由到对应的处理函数
 *   6. 关闭连接
 * 
 * 注意事项：
 *   - 本函数在独立线程中运行
 *   - 所有缓冲区都是固定大小（SDS 改进目标）
 *   - 不支持除 GET/POST 外的方法
 */
/**********************************************************************/
void accept_request(void *arg)
{
    // ======================= 变量声明区 ==========================

    // === 网络相关 ===
    int client = (intptr_t)arg;
    /* 客户端 socket 文件描述符
     * 从 void* 转换为 int
     * 用于后续的 recv/send 操作
     */
    char buf[1024];
    /* 临时缓冲区，用于：
     * 1. 读取 HTTP 请求行
     * 2. 读取请求头
     * 3. 构建响应
     * 问题：如果请求行超过 1024 字节会溢出
     * 改进：使用 SDS 动态扩展
     */
    size_t numchars;
    /* 读取的字符数
     * 用于判断是否读取完毕
     */
    char method[255];
    /* HTTP 方法（GET、POST 等）
     * 问题：255 字节对方法名来说太大（浪费）
     * 改进：使用 SDS，只分配实际需要的空间
     */
    char url[255];
    /* 请求的 URL 路径
     * 例如："/index.html" 或 "/color.cgi?color=red"
     * 问题：URL 可能超过 255 字节（尤其带长查询字符串）
     * 改进：使用 SDS 动态扩展
     */
    char path[512];
    /* 文件系统中的实际路径
     * 格式："htdocs" + url
     * 例如："htdocs/index.html"
     * 问题：路径可能超过 512 字节
     * 改进：使用 SDS
     */
    size_t i, j;
    /* 字符串处理的索引变量
     * i: 通常用于 method/url/path 的索引
     * j: 通常用于 buf 的索引
     */

    // === 文件状态 ===
    struct stat st;
    /* 文件状态结构体（系统调用 stat() 使用）
     * 包含文件信息：
     * - st_mode: 文件类型和权限
     * - st_size: 文件大小
     * - st_mtime: 修改时间
     * 等等
     */

    // === CGI 标志 ===
    int cgi = 0;
    /* CGI 标志，判断是否需要执行 CGI
     * 0 = 静态文件（HTML、CSS、图片等）
     * 1 = CGI 脚本（需要动态执行）
     * 
     * 设置为 1 的三种情况：
     * 1. POST 方法（一定是 CGI）
     * 2. URL 包含查询字符串（? 后面有参数）
     * 3. 文件有执行权限
     */

    // === 查询字符串 ===
    char *query_string = NULL;
    /* 指向查询字符串的指针
     * 例如 URL 是 "/color.cgi?color=red"
     * 则 query_string 指向 "color=red"
     * 
     * 为什么用指针而不是数组？
     * 因为它只是指向 url 数组中的某个位置
     * 不需要额外分配内存
     */

    // ==================== 步骤 1：读取 HTTP 请求行 ====================
    numchars = get_line(client, buf, sizeof(buf));
    /* 调用 get_line() 读取一行
     * 
     * 输入：
     * - client: socket 文件描述符
     * - buf: 存储读取内容的缓冲区
     * - sizeof(buf): 缓冲区大小（1024）
     * 
     * 输出：
     * - 返回值：读取的字符数
     * - buf: 存储了请求行，例如：
     *        "GET /index.html HTTP/1.1\r\n"
     * 
     * get_line() 的特点：
     * 1. 一次读一个字节（效率较低，但简单）
     * 2. 处理 \r\n 和 \n 两种换行符
     * 3. 返回读取的字符数（包括 \r\n）
     */

    // ==================== 步骤 2：解析 HTTP 方法 ====================
    i = 0; j = 0;
    /* 初始化索引
     * i: method 数组的索引
     * j: buf 数组的索引
     */

    // 从 buf 中提取第一个单词（HTTP 方法）
    while (!ISspace(buf[i]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[i];  // 逐字符复制
        i++;
    }
    /* 循环说明：
     * 1. ISspace(buf[i]): 检查是否是空白字符（空格、\t、\n 等）
     * 2. i < sizeof(method) - 1: 防止溢出（保留一个位置给 \0）
     * 3. 循环结束条件：遇到空格 或 method 数组满了
     *
     * 示例：
     * buf = "GET /index.html HTTP/1.1"
     *        ↑↑↑ 提取这部分
     * 循环后：method = "GET"
     */

    j=i;    //j = i (指向方法(实例中GET)后的空格)


    method[i] = '\0';
    /* 添加字符串结束符
     * C 语言字符串必须以 \0 结尾
     * 现在 method = "GET\0"
     */

    // ==================== 步骤 3：检查方法是否支持 ====================

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        /* strcasecmp(): 不区分大小写的字符串比较
         * 返回 0 表示相等，非 0 表示不相等
         * 
         * 逻辑：
         * if (method != "GET" && method != "POST")
         * 
         * 即：如果既不是 GET 也不是 POST
         */
        unimplemented(client);
        /* 发送 501 Not Implemented 响应
         * HTTP 501: 服务器不支持请求的功能
         */
        return;    //结束函数(线程也会结束)
    }

    // ==================== 步骤 4：POST 方法特殊处理 ====================
    if (strcasecmp(method, "POST") == 0)
        cgi = 1;
    /* POST 请求一定需要 CGI 处理
     * 
     * 1. POST 请求包含请求体（表单数据）
     * 2. 需要 CGI 脚本来处理这些数据
     * 3. 静态文件无法处理 POST 数据
     * 
     * 例如：
     * POST /color.cgi HTTP/1.1
     * Content-Length: 10
     * 
     * color=blue
     * ↑ 这部分数据需要 CGI 处理
     */

// ==================== 步骤 5：解析 URL ====================

    i = 0;   // 重置 i，用于 url 数组索引

    // 跳过方法后面的空格
    while (ISspace(buf[j]) && (j < numchars))
        j++;
    /* 现在 j 指向 URL 的第一个字符
     * 例如：buf = "GET /index.html HTTP/1.1"
     *                   ↑
     *                   j 在这里
     */

    // 提取 URL（第二个单词）
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < numchars))
    {
        url[i] = buf[j];
        i++; j++;
    }
    /* 循环结束后：
     * url = "/index.html"
     * j 指向 HTTP 版本前的空格
     */
    
    url[i] = '\0';
    
    /* 添加字符串结束符
     * 现在 url = "/index.html\0"
     */

    // =========== 步骤 6：GET 请求的查询字符串处理 ============
    if (strcasecmp(method, "GET") == 0)
    {
        /* 只有 GET 请求才处理查询字符串
         * POST 的数据在请求体中，不在 URL 中
         */

        query_string = url;
        /* query_string 指向 url 的开头
         * 现在开始在 url 中查找 '?'
         */

       // 查找查询字符串的开始（'?' 字符）
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        /* 循环说明：
         * 1. *query_string: 解引用指针，获取当前字符
         * 2. 如果不是 '?' 也不是结束符，继续前进
         * 
         * 例子 1：url = "/index.html"
         *   循环到末尾，query_string 指向 '\0'
         * 
         * 例子 2：url = "/color.cgi?color=red"
         *   循环停止在 '?'，query_string 指向 '?'
         */
        if (*query_string == '?')
        {
 	    /* 找到了查询字符串
             * 现在：url = "/color.cgi?color=red"
             *                         ↑
             *                   query_string 指向这里
             */
            cgi = 1;
            /* 有查询字符串，标记为 CGI
             * 因为需要把参数传递给 CGI 脚本
             */
            *query_string = '\0';
            /* 截断 url，把 '?' 改成 '\0'
             * 现在：url = "/color.cgi\0color=red"
             *                         ↑
             *              这里变成了字符串结束符
             * 
             * 结果：url = "/color.cgi"（字符串自动在 \0 处结束）
             */
            query_string++;
            /* 指针前进一个位置
             * 现在：query_string 指向 "color=red"
             * 
             * 把 URL 分成了两部分：
             * - url = "/color.cgi"（路径）
             * - query_string = "color=red"（参数）
             */
        }
        /* 如果没有找到 '?'，query_string 保持为 NULL
         * 这种情况下是普通的 GET 请求（没有参数）
         */
    }
    // ==================== 步骤 7：构建文件系统路径 ====================

    sprintf(path, "htdocs%s", url);
    /* 拼接路径
     * 
     * 格式："htdocs" + url
     * 
     * 例子：
     * - url = "/index.html"
     *   → path = "htdocs/index.html"
     * 
     * - url = "/color.cgi"
     *   → path = "htdocs/color.cgi"
     * 
     * - url = "/"
     *   → path = "htdocs/"
     * 
     * 为什么加 "htdocs"？
     * 1. 安全性：限制访问范围在 htdocs 目录内
     * 2. 组织性：所有网站文件集中管理
     * 3. 防止访问系统文件（例如 /etc/passwd）
     */
    
    // 如果路径以 '/' 结尾（访问目录），默认加上 index.html
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    /* 处理目录访问
     * 
     * 例子：
     * - 输入：path = "htdocs/"
     * - 输出：path = "htdocs/index.html"
     * 
     * 这是 Web 服务器的标准行为：
     * 访问目录时自动返回 index.html
     * 
     * 例如：
     * - http://localhost/ → htdocs/index.html
     * - http://localhost/images/ → htdocs/images/index.html
     */

    // ==================== 步骤 8：检查文件是否存在 ====================
    if (stat(path, &st) == -1) {
        /* stat() 系统调用
         * 
         * 功能：获取文件信息
         * 
         * 参数：
         * - path: 文件路径
         * - &st: 存储文件信息的结构体指针
         * 
         * 返回值：
         * - 0: 成功，st 包含文件信息
         * - -1: 失败，文件不存在或无权限
         * 
         * errno 会被设置：
         * - ENOENT: 文件不存在
         * - EACCES: 权限不足
         * 等等
         */
        
        // 文件不存在，读取并丢弃剩余的请求头
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
        /* 为什么要读取并丢弃？
         * 
         * HTTP 请求格式：
         * GET /index.html HTTP/1.1    ← 请求行（已经读取）
         * Host: localhost              ← 请求头
         * User-Agent: curl             ← 请求头
         * Accept: '/'                  ← 请求头
         *                              ← 空行（请求结束）
         * 
         * 如果不读取完，这些数据会留在 socket 缓冲区
         * 影响下一个请求的处理
         * 
         * 循环停止条件：
         * 1. 读取到空行（只有 \n）
         * 2. 读取失败（numchars <= 0）
         */

        not_found(client);
        /* 发送 404 Not Found 响应
         * 
         * HTTP 404 响应格式：
         * HTTP/1.0 404 NOT FOUND
         * Content-Type: text/html
         * 
         * <HTML><BODY>
         * <P>The server could not fulfill your request...
         * </BODY></HTML>
         */
    }
    else
    {
        // 文件存在，继续处理
        
        // ==================== 步骤 9：判断是否是目录 ====================
        
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");
        /* 文件类型判断
         * 
         * st.st_mode: 文件模式（类型 + 权限）
         * S_IFMT: 文件类型掩码（0170000）
         * S_IFDIR: 目录类型（0040000）
         * 
         * 按位与操作：
         * st.st_mode & S_IFMT 提取文件类型部分
         * 
         * 如果是目录，追加 "/index.html"
         * 例如：
         * - path = "htdocs"
         * → path = "htdocs/index.html"
         */
        
        // ==================== 步骤 10：检查是否有执行权限 ====================
        if ((st.st_mode & S_IXUSR) ||
                (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH)    )
            cgi = 1;
        /* 执行权限检查
         * 
         * st.st_mode 的权限位：
         * - S_IXUSR (0100): 所有者执行权限
         * - S_IXGRP (0010): 组执行权限
         * - S_IXOTH (0001): 其他人执行权限
         * 
         * 按位或：只要有任何一个执行权限，就是 true
         * 
         * 为什么有执行权限就是 CGI？
         * 1. 可执行文件通常是脚本（Perl、Python、Shell 等）
         * 2. 需要执行后获取动态内容
         * 3. 不能直接发送文件内容
         * 
         * 例如：
         * - index.html: -rw-r--r-- (644) → 不可执行 → 静态文件
         * - color.cgi:  -rwxr-xr-x (755) → 可执行 → CGI
         */
        
        // ==================== 步骤 11：路由到对应的处理函数 ====================
        if (!cgi)
            serve_file(client, path);
        else
            execute_cgi(client, path, method, query_string);
        /* 根据 cgi 标志决定如何处理
         * 
         * cgi = 0（静态文件）：
         * - 调用 serve_file()
         * - 直接读取文件内容并发送
         * - 例如：HTML、CSS、JavaScript、图片
         * 
         * cgi = 1（动态内容）：
         * - 调用 execute_cgi()
         * - fork 子进程执行脚本
         * - 通过管道获取输出
         * - 发送给客户端
         * 
         * cgi 标志设置为 1 的三种情况回顾：
         * 1. POST 方法
         * 2. URL 包含查询字符串（?）
         * 3. 文件有执行权限
         */
    }

    // ==================== 步骤 12：关闭连接 ====================

    close(client);
    /* 关闭客户端 socket
     * 
     * 完整的 TCP 四次挥手：
     * 1. 服务器发送 FIN
     * 2. 客户端回复 ACK
     * 3. 客户端发送 FIN
     * 4. 服务器回复 ACK
     * 
     * close() 会触发这个过程
     * 
     * HTTP/1.0 默认：每个请求后关闭连接
     * HTTP/1.1 默认：保持连接（Keep-Alive）
     * 
     * Tinyhttpd 使用 HTTP/1.0，所以每次都关闭
     */
    
    /* 函数结束，线程也随之结束
     * pthread 会自动清理线程资源
     */
}
