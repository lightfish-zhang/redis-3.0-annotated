# 跳跃表

## 源码分析

### 数据结构

```c
/* ZSETs use a specialized version of Skiplists */
/*
 * 跳跃表节点
 */
typedef struct zskiplistNode {

    // 成员对象
    robj *obj;

    // 分值
    double score;

    // 后退指针
    struct zskiplistNode *backward;

    // 层
    struct zskiplistLevel {

        // 前进指针
        struct zskiplistNode *forward;

        // 跨度
        unsigned int span;

    } level[];

} zskiplistNode;

/*
 * 跳跃表
 */
typedef struct zskiplist {

    // 表头节点和表尾节点
    struct zskiplistNode *header, *tail;

    // 表中节点的数量
    unsigned long length;

    // 表中层数最大的节点的层数
    int level;

} zskiplist;

```

- 跳跃表在单向链表的基础上
    + 节点的`* next`变成了`level[]`，可以说`level[]`是`* next`的数组，存储了后面第1个节点的指针、第2个节点的指针。。。第n个节点的指针
    + 跳跃表`skiplist`的属性`level`保存了表中层数最大的节点的层数。

- 比单向列表的优越点
    + 单向链表的查找，一个一个节点遍历的，时间复杂度为O(n), n为节点数
    + 而跳跃表，如果节点是按节点某属性的值大小排序的，也就是有序链表，当节点加上跳跃点后，那么遍历的效率就会大大增加
    + 举个例子，链表是排序的，并且节点中还存储了指向前面第二个节点的指针的话，那么在查找一个节点时，仅仅需要遍历N/2个节点即可。


### 应用

- 在redis中，跳跃表作为有序集合键的底层实现之一，应用场景:
    + 一个有序集合包含的元素数量比较多
    + 有序集合中元素的成员（member）是比较长的字符串

- 在redis中，跳跃表节点的层高都是1~32之间的随机数
- 在同一个跳跃表中，多个节点可以包含相同的分值，但每个节点的成员对象必须是唯一的
- 跳跃表中的节点按照分值大小进行排序，当分值相同时，节点按照成员对象的大小排序