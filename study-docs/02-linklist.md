# 链表

## redis实现的链表的数据结构

```c
/* Node, List, and Iterator are the only data structures used currently. */

/*
 * 双端链表节点
 */
typedef struct listNode {

    // 前置节点
    struct listNode *prev;

    // 后置节点
    struct listNode *next;

    // 节点的值
    void *value;

} listNode;

/*
 * 双端链表迭代器
 */
typedef struct listIter {

    // 当前迭代到的节点
    listNode *next;

    // 迭代的方向
    int direction;

} listIter;

/*
 * 双端链表结构
 */
typedef struct list {

    // 表头节点
    listNode *head;

    // 表尾节点
    listNode *tail;

    // 节点值复制函数
    void *(*dup)(void *ptr);

    // 节点值释放函数
    void (*free)(void *ptr);

    // 节点值对比函数
    int (*match)(void *ptr, void *key);

    // 链表所包含的节点数量
    unsigned long len;

} list;
```

### 特点

- 双端，节点有`prev`/`next`，获取前后节点的复杂度为O(1)
- 无环，表头的`prev`=NULL，表尾的`next`=NULL，对链表的遍历以NULL结束
- 带表头指针和表尾指针，`list`有`head`/`tail`
- 带链表长度计数器，`list`有`len`，获取链表长度时间复杂度为O(1)
- 多态，node的`value`为`void *`，该链表结构可以用于不同的类型的值
