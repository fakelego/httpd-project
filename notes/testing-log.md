# 测试记录

## 测试环境
- 日期：2026-02-14
- 端口：4000

## 测试用例

### Test 1: 首页访问
- 命令：`curl http://localhost:PORT/`
- 结果：成功
- 输出：```html
<HTML>
<TITLE>Index</TITLE>
<BODY>
<P>Welcome to J. David's webserver.
<H1>CGI demo
<FORM ACTION="color.cgi" METHOD="POST">
Enter a color: <INPUT TYPE="text" NAME="color">
<INPUT TYPE="submit">
</FORM>
</BODY>
</HTML>
```

### Test 2: 静态文件
- 命令：`curl http://localhost:PORT/index.html`
- 结果：成功
- 输出：```html
<HTML>
<TITLE>Index</TITLE>
<BODY>
<P>Welcome to J. David's webserver.
<H1>CGI demo
<FORM ACTION="color.cgi" METHOD="POST">
Enter a color: <INPUT TYPE="text" NAME="color">
<INPUT TYPE="submit">
</FORM>
</BODY>
</HTML>
```

### Test 3: CGI GET
- 命令：`curl http://localhost:PORT/color.cgi?color=red`
- 结果：成功
- 输出：```html
<!DOCTYPE html
        PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
         "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US">
<head>
<title>RED</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
</head>
<body bgcolor="red">
<h1>This is red</h1>
</body>
```

### Test 4: CGI POST
- 命令：`curl -X POST -d "color=blue" http://localhost:PORT/color.cgi`
- 结果：成功
- 输出：```html
<!DOCTYPE html
        PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
         "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US">
<head>
<title>BLUE</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
</head>
<body bgcolor="blue">
<h1>This is blue</h1>
</body>
</html>
```

## 遇到的问题
[curl http://localhost:4000/color.cgi?color=red测试代码运行报错curl: (8) Header without colon]

## 修复过程
[将重复发送的HTTP 状态行语句移动到父进程分支中]
