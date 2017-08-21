# 整数集合

## 源码分析

### 数据结构

```c
typedef struct intset {
    
    // 编码方式
    uint32_t encoding;

    // 集合包含的元素数量
    uint32_t length;

    // 保存元素的数组
    int8_t contents[];

} intset;
```

- `encoding`属性，
    + `INTSET_ENC_INT16` 
    + `INTSET_ENC_INT32` 
    + `INTSET_ENC_INT64` 

- `contents[]`，整数集合，每个元素从小到大排序，数组中不出现重复项
- API与复杂度
    + `insetAdd`/`insetRemove`，在c语言的数组中插入或新的元素，时间复杂度O(N)
    + `insetNew`, 创建一个新的整数集合，O(1)
    + `insetFind` 检查给定值是否存在与集合，因为是有序数组，可以使用二分法，O(logN)
    + `intsetRandom` 随机返回一个元素 O(1)
    + `intsetGet` 取指定索引的元素 O(1)
    + `intsetLen` 返回元素个数 O(1)
    + `intsetBlobLen` 返回集合所占内存字节数
