# 字典

## 概念

- 字典，是一种保存键值对(key-value pair)的抽象数据结构，又称为符号表`symbol table`,关联数组`associative array`,映射`map`
- 字典的每一个键是唯一的，与一个值进行关联
- 在redis中，对数据库的增删改查是构建在对字典的操作之上的

## 源码分析

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


### 哈希表的扩展与收缩 —— rehash

- 各个名词
    + `ht`哈希表，`ht[0]`是rehash前的哈希表，`ht[1]`是rehash后的哈希表
    + `ht.used` 当前包含的键值对数量
    + `load factor`=`ht.used`/`ht.size`，负载因子为哈希表已保存字节数与哈希表大小的比

- 哈希表的rehash
    + 扩展，分配更大的空间，`ht[1].size`=`ht[0].size * 2`
    + `rehash`，重新计算键的哈希值和索引值，然后将键值对放置到`ht[1]`哈希表的相应的位置上
    + 收缩，当哈希表的负载因子小于0.1时，程序自动开始对哈希表执行收缩操作

- 子进程存在时，用来判断是否扩展操作的负载因子的临界值会提高
    + 如`BGSAVE`,`BGREWRITEAOF`命令，程序会开子进程去处理
    + 提高扩展操作的临界值，避免不必要的内存写入操作（父子进程同时扩展？）

- rehash的过程是渐进式的
    + 当`ht[0]`的键值对是百万计时，显然，短时间不能一次性全部refash到`ht[1]`，数据库不能花费长时间在这上面
    + 分多次、渐进式地执行refash，是取于平衡的操作
    + 字典中维持一个索引计数器，`rehashidx`，初始值0
    + rehash进行期间，每次对字典的增删改查，都会顺便将`ht[0]`的一个键值对rehash到`ht[1]`，且`rehashidx += 1`
    + rehash结束，`rehashidx`设为-1
    + 由此可见，rehash将计算工作均摊到字典的每个操作上，分而治之
    + rehash进行期间，对一个键值的查找，先查`ht[0]`，若不存在就查`ht[1]`
    + rehash进行期间，新增操作则只对`ht[1]`，最终使`ht[0]`变成空表    