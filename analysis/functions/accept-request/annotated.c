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
    int cgi = 0;      /* becomes true if server decides this is a CGI
                       * program */
    char *query_string = NULL;

    numchars = get_line(client, buf, sizeof(buf));
    i = 0; j = 0;
    while (!ISspace(buf[i]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[i];
        i++;
    }
    j=i;
    method[i] = '\0';

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return;
    }

    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    i = 0;
    while (ISspace(buf[j]) && (j < numchars))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < numchars))
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0)
    {
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }

    sprintf(path, "htdocs%s", url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    if (stat(path, &st) == -1) {
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
        not_found(client);
    }
    else
    {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");
        if ((st.st_mode & S_IXUSR) ||
                (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH)    )
            cgi = 1;
        if (!cgi)
            serve_file(client, path);
        else
            execute_cgi(client, path, method, query_string);
    }

    close(client);
}
