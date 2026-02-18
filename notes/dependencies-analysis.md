# 依赖库分析

## SDS 分析

### 文件结构
- sds.h: 头文件（API 定义）
- sds.c: 实现文件
- 行数: sds.c: 1328    sds.h: 274    total: 1602

### 核心 API
sds sdsnew(const char *init);
void sdsfree(sds s);
sds sdscar(sds s, const char *t);
size_t sdslen(const sds s);

### 数据结构
struct sdshdr8 {
    uint8_t len;
    uint8_t alloc;
    unsigned char flags;
    char buf[];
};

## C-Thread-Pool分析

### 文件结构
- thpool.h: 头文件
- thpool.c: 实现文件
- 行数: thpool.c: 571    thpool.h: 187    total: 758

### 核心API
threadpool thpool_init(int num_threads);
int thpool_add_work(threadpool, void (*function)(void*), void* arg);
void thpool_wait(threadpool);
void thpool_destroy(threadpool);

## 使用示例

