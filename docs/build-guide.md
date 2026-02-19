# 创建编译指南
cat > build-guide.md << 'EOF'
# 项目编译指南

## 环境要求
- Ubuntu 22.04+
- GCC 11+
- Make
- pthread 库

## 编译 Tinyhttpd

```bash
cd ~/httpd-project/source/Tinyhttpd
make clean
make
./httpd
```
## 编译 SDS
SDS 是库，不单独编译。使用时：
```bash
gcc -c sds.c -o sds.o
gcc your_program.c sds.o -o your_program
```
## 编译 C-Thread-Pool
```bash
cd ~/httpd-project/source/C-Thread-Pool
gcc -o example example.c thpool.c -pthread
```

