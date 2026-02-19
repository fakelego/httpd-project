# 编译记录

## 日期
2026-02-14

## 编译命令
```bash
make
```

## 遇到的问题
1.[编译出现警告]
 - 错误信息:```
httpd.c: In function ‘execute_cgi’:
httpd.c:280:9: warning: argument 2 null where non-null expected [-Wnonnull]
  280 |         execl(path, NULL);
      |         ^~~~~
In file included from httpd.c:20:
/usr/include/unistd.h:594:12: note: in a call to function ‘execl’ declared ‘nonnull’
  594 | extern int execl (const char *__path, const char *__arg, ...)
      |            ^~~~~
httpd.c:280:9: warning: not enough variable arguments to fit a sentinel [-Wformat=]
  280 |         execl(path, NULL);
      |         ^~~~~
```
 - 解决方法:修改execl函数内的参数

## 成功输出
```
gcc -W -Wall -g -o httpd httpd.c -lpthread
```

## 生成的文件
- httpd(可执行文件)
- 大小: [记录]

