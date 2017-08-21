# 压缩列表

## 概念

- 是为了尽可能地节约内存而设计的特殊编码双端链表。没有定义c语言的结构体，而是特殊的二进制格式。

### 源码中的注释和翻译

```
The ziplist is a specially encoded dually linked list that is designed
to be very memory efficient. 

Ziplist 是为了尽可能地节约内存而设计的特殊编码双端链表。

It stores both strings and integer values,
where integers are encoded as actual integers instead of a series of
characters. 

Ziplist 可以储存字符串值和整数值，
其中，整数值被保存为实际的整数，而不是字符数组。

It allows push and pop operations on either side of the list
in O(1) time. However, because every operation requires a reallocation of
the memory used by the ziplist, the actual complexity is related to the
amount of memory used by the ziplist.

Ziplist 允许在列表的两端进行 O(1) 复杂度的 push 和 pop 操作。
但是，因为这些操作都需要对整个 ziplist 进行内存重分配，
所以实际的复杂度和 ziplist 占用的内存大小有关。

----------------------------------------------------------------------------

ZIPLIST OVERALL LAYOUT:
Ziplist 的整体布局：

The general layout of the ziplist is as follows:
以下是 ziplist 的一般布局：

<zlbytes><zltail><zllen><entry><entry><zlend>

<zlbytes> is an unsigned integer to hold the number of bytes that the
ziplist occupies. This value needs to be stored to be able to resize the
entire structure without the need to traverse it first.

<zlbytes> 是一个无符号整数，保存着 ziplist 使用的内存数量。

通过这个值，程序可以直接对 ziplist 的内存大小进行调整，
而无须为了计算 ziplist 的内存大小而遍历整个列表。

<zltail> is the offset to the last entry in the list. This allows a pop
operation on the far side of the list without the need for full traversal.

<zltail> 保存着到达列表中最后一个节点的偏移量。

这个偏移量使得对表尾的 pop 操作可以在无须遍历整个列表的情况下进行。

<zllen> is the number of entries.When this value is larger than 2**16-2,
we need to traverse the entire list to know how many items it holds.

<zllen> 保存着列表中的节点数量。

当 zllen 保存的值大于 2**16-2 时，
程序需要遍历整个列表才能知道列表实际包含了多少个节点。

<zlend> is a single byte special value, equal to 255, which indicates the
end of the list.

<zlend> 的长度为 1 字节，值为 255 ，标识列表的末尾。

ZIPLIST ENTRIES:
ZIPLIST 节点：

Every entry in the ziplist is prefixed by a header that contains two pieces
of information. First, the length of the previous entry is stored to be
able to traverse the list from back to front. Second, the encoding with an
optional string length of the entry itself is stored.

每个 ziplist 节点的前面都带有一个 header ，这个 header 包含两部分信息：

1)前置节点的长度，在程序从后向前遍历时使用。

2)当前节点所保存的值的类型和长度。

The length of the previous entry is encoded in the following way:
If this length is smaller than 254 bytes, it will only consume a single
byte that takes the length as value. When the length is greater than or
equal to 254, it will consume 5 bytes. The first byte is set to 254 to
indicate a larger value is following. The remaining 4 bytes take the
length of the previous entry as value.

编码前置节点的长度的方法如下：

1) 如果前置节点的长度小于 254 字节，那么程序将使用 1 个字节来保存这个长度值。

2) 如果前置节点的长度大于等于 254 字节，那么程序将使用 5 个字节来保存这个长度值：
   a) 第 1 个字节的值将被设为 254 ，用于标识这是一个 5 字节长的长度值。
   b) 之后的 4 个字节则用于保存前置节点的实际长度。

The other header field of the entry itself depends on the contents of the
entry. When the entry is a string, the first 2 bits of this header will hold
the type of encoding used to store the length of the string, followed by the
actual length of the string. When the entry is an integer the first 2 bits
are both set to 1. The following 2 bits are used to specify what kind of
integer will be stored after this header. An overview of the different
types and encodings is as follows:

header 另一部分的内容和节点所保存的值有关。

1) 如果节点保存的是字符串值，
   那么这部分 header 的头 2 个位将保存编码字符串长度所使用的类型，
   而之后跟着的内容则是字符串的实际长度。

|00pppppp| - 1 byte
     String value with length less than or equal to 63 bytes (6 bits).
     字符串的长度小于或等于 63 字节。
|01pppppp|qqqqqqqq| - 2 bytes
     String value with length less than or equal to 16383 bytes (14 bits).
     字符串的长度小于或等于 16383 字节。
|10______|qqqqqqqq|rrrrrrrr|ssssssss|tttttttt| - 5 bytes
     String value with length greater than or equal to 16384 bytes.
     字符串的长度大于或等于 16384 字节。

2) 如果节点保存的是整数值，
   那么这部分 header 的头 2 位都将被设置为 1 ，
   而之后跟着的 2 位则用于标识节点所保存的整数的类型。

|11000000| - 1 byte
     Integer encoded as int16_t (2 bytes).
     节点的值为 int16_t 类型的整数，长度为 2 字节。
|11010000| - 1 byte
     Integer encoded as int32_t (4 bytes).
     节点的值为 int32_t 类型的整数，长度为 4 字节。
|11100000| - 1 byte
     Integer encoded as int64_t (8 bytes).
     节点的值为 int64_t 类型的整数，长度为 8 字节。
|11110000| - 1 byte
     Integer encoded as 24 bit signed (3 bytes).
     节点的值为 24 位（3 字节）长的整数。
|11111110| - 1 byte
     Integer encoded as 8 bit signed (1 byte).
     节点的值为 8 位（1 字节）长的整数。
|1111xxxx| - (with xxxx between 0000 and 1101) immediate 4 bit integer.
     Unsigned integer from 0 to 12. The encoded value is actually from
     1 to 13 because 0000 and 1111 can not be used, so 1 should be
     subtracted from the encoded 4 bit value to obtain the right value.
     节点的值为介于 0 至 12 之间的无符号整数。
     因为 0000 和 1111 都不能使用，所以位的实际值将是 1 至 13 。
     程序在取得这 4 个位的值之后，还需要减去 1 ，才能计算出正确的值。
     比如说，如果位的值为 0001 = 1 ，那么程序返回的值将是 1 - 1 = 0 。
|11111111| - End of ziplist.
     ziplist 的结尾标识

All the integers are represented in little endian byte order.

所有整数都表示为小端字节序。
```

```
/* 
空白 ziplist 示例图

area        |<---- ziplist header ---->|<-- end -->|

size          4 bytes   4 bytes 2 bytes  1 byte
            +---------+--------+-------+-----------+
component   | zlbytes | zltail | zllen | zlend     |
            |         |        |       |           |
value       |  1011   |  1010  |   0   | 1111 1111 |
            +---------+--------+-------+-----------+
                                       ^
                                       |
                               ZIPLIST_ENTRY_HEAD
                                       &
address                        ZIPLIST_ENTRY_TAIL
                                       &
                               ZIPLIST_ENTRY_END

非空 ziplist 示例图

area        |<---- ziplist header ---->|<----------- entries ------------->|<-end->|

size          4 bytes  4 bytes  2 bytes    ?        ?        ?        ?     1 byte
            +---------+--------+-------+--------+--------+--------+--------+-------+
component   | zlbytes | zltail | zllen | entry1 | entry2 |  ...   | entryN | zlend |
            +---------+--------+-------+--------+--------+--------+--------+-------+
                                       ^                          ^        ^
address                                |                          |        |
                                ZIPLIST_ENTRY_HEAD                |   ZIPLIST_ENTRY_END
                                                                  |
                                                        ZIPLIST_ENTRY_TAIL
*/
```