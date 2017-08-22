# redis的动态字符串

## 动态字符串的数据结构

- 代码 /src/sds.h

```c
/*
 * 保存字符串对象的结构
 */
struct sdshdr {
    
    // buf 中已占用空间的长度
    int len;

    // buf 中剩余可用空间的长度
    int free;

    // 数据空间
    char buf[];
};
```

- `buf`遵循c语言的字符串以`\0`结尾的惯例，以直接使用c语言标准库的函数。
- `len`不包含`\0`的1byte
- `buf`的长度 = `len` + `free` + 1
- redis有`SDS`的API操作，如`STRLEN`命令

### 改善c语言字符串的问题

- c字符串不保存长度，所以获取长度的时间复杂度为O(n)，`SDS`结构体保存长度，获取长度的时间复杂度为O(1)
- c标准函数的`char *strcat(char *dest, const char *src)`用作拼接字符串，但是需要`dest`留有足够的空间，需要程序员控制。而`SDS`的API需要修改字符串时，会先检查空间，按需修改空间大小，防止出现空间溢出的问题
- 分配内存空间是一个耗时的操作，对于io密集的数据库redis, 它的优化策略是空间预分配和惰性空间释放
    + 空间预分配，长度少于1MB时，`len`=`free`；长度大于等于1MB时，`free`=1MB
    + 惰性空间释放，缩短字符串时不会回收空间，在有需要时才真正释放空间

- c字符串的字符必须符合某种编码如`ASCII`,并且除了末尾外其他元素不得含有`\0`，这些限制使c字符串只能保存文本数据，不能保存如图片、压缩文件、音频等二进制数据
- `SDS`的`buf`保存的是字节数组，它的API都是二进制安全的`binary-safe`，使用`len`来判断字符串是否结束，而不使用`\0`来判断末尾
- `buf`使用`\0`结尾，兼容部分c字符串函数，重用`string.h`，如`strcasecmp()`