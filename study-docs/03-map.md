# 字典

## 概念

- 字典，是一种保存键值对(key-value pair)的抽象数据结构，又称为符号表`symbol table`,关联数组`associative array`,映射`map`
- 字典的每一个键是唯一的，与一个值进行关联
- 在redis中，对数据库的增删改查是构建在对字典的操作之上的

## 源码

### 哈希表数据结构

- 代码定义`src/dict.h`

```c
/*
 * 哈希表
 *
 * 每个字典都使用两个哈希表，从而实现渐进式 rehash 。
 */
typedef struct dictht {
    
    // 哈希表数组
    dictEntry **table;

    // 哈希表大小
    unsigned long size;
    
    // 哈希表大小掩码，用于计算索引值
    // 总是等于 size - 1
    unsigned long sizemask;

    // 该哈希表已有节点的数量
    unsigned long used;

} dictht;
/*
 * 哈希表节点
 */
typedef struct dictEntry {
    
    // 键
    void *key;

    // 值
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
    } v;

    // 指向下个哈希表节点，形成链表
    struct dictEntry *next;

} dictEntry;
```

- redis使用`MurmurHash2`算法计算哈希值，参考 http://code.google.com/p/smhasher/
- 键冲突，哈希冲突的键值，使用单向链表存储，新节点总是添加链表的表头，这样添加的复杂度为O(1)
