/**
 * @file    xmem_pool.c
 * <pre>
 * Copyright (c) 2019, Gaaagaa All rights reserved.
 * 
 * 文件名称：xmem_pool.cpp
 * 创建日期：2019年08月12日
 * 文件标识：
 * 文件摘要：实现内存池的相关操作接口。
 * 
 * 当前版本：1.0.0.0
 * 作    者：
 * 完成日期：2019年08月12日
 * 版本摘要：
 * 
 * 历史版本：
 * 原作者  ：
 * 完成日期：
 * 版本摘要：
 * </pre>
 */

#include "xmem_pool.h"
#include "xrbtree.h"

#include <stdlib.h>
#include <memory.h>

#ifdef _MSC_VER
#include <windows.h>
#elif defined(__GNUC__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#else
#error "Unknow platform"
#endif

////////////////////////////////////////////////////////////////////////////////

#ifndef ENABLE_XASSERT
#if ((defined _DEBUG) || (defined DEBUG))
#define ENABLE_XASSERT 1
#else // !((defined _DEBUG) || (defined DEBUG))
#define ENABLE_XASSERT 0
#endif // ((defined _DEBUG) || (defined DEBUG))
#endif // ENABLE_XASSERT

#ifndef XASSERT
#if ENABLE_XASSERT
#include <assert.h>
#define XASSERT(xptr)    assert(xptr)
#else // !ENABLE_XASSERT
#define XASSERT(xptr)
#endif // ENABLE_XASSERT
#endif // XASSERT

#if ENABLE_XASSERT
#define XASSERT_CHECK(xcheck, xptr)  do { if ((xcheck)) XASSERT(xptr); } while (0)
#else // !ENABLE_XASSERT
#define XASSERT_CHECK(xcheck, xptr)
#endif // ENABLE_XASSERT

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif // __GNUC__

////////////////////////////////////////////////////////////////////////////////

struct xmem_chunk_t;
struct xmem_class_t;
struct xslice_array_t;
struct xslice_rqueue_t;

typedef struct xmem_chunk_t    * xchunk_handle_t;
typedef struct xmem_class_t    * xclass_handle_t;
typedef struct xslice_array_t  * xslice_arrptr_t;
typedef struct xslice_rqueue_t * xsrque_handle_t;

#define XMEM_PAGE_SIZE      (1024 * 4)

/**
 * slice size table: 
 * 
 * | ----- | ---- | ---- | ---- | ---- | ---- | ---- | ----- | ----- | ----- | ----- | ----- |
 * | step  |    8 |   16 |   32 |   64 |  128 |  256 |   512 |  1024 |  2048 |  4096 |   X   |
 * | :---: | ---: | ---: | ---: | ---: | ---: | ---: | ----: | ----: | ----: | ----: | :---: |
 * | size  |    8 |  144 |  288 |  576 | 1152 | 2304 |  4608 |  9216 | 18432 | 36864 |   X   |
 * | size  |   16 |  160 |  320 |  640 | 1280 | 2560 |  5120 | 10240 | 20480 | 40960 |   X   |
 * | size  |   24 |  176 |  352 |  704 | 1408 | 2816 |  5632 | 11264 | 22528 | 45056 |   X   |
 * | size  |   32 |  192 |  384 |  768 | 1536 | 3072 |  6144 | 12288 | 24576 | 49152 |   X   |
 * | size  |   40 |  208 |  416 |  832 | 1664 | 3328 |  6656 | 13312 | 26624 | 53248 |   X   |
 * | size  |   48 |  224 |  448 |  896 | 1792 | 3584 |  7168 | 14336 | 28672 | 57344 |   X   |
 * | size  |   56 |  240 |  480 |  960 | 1920 | 3840 |  7680 | 15360 | 30720 | 61440 |   X   |
 * | size  |   64 |  256 |  512 | 1024 | 2048 | 4096 |  8192 | 16384 | 32768 | 65536 |   X   |
 * | size  |   72 |      |      |      |      |      |       |       |       |       |   X   |
 * | size  |   80 |      |      |      |      |      |       |       |       |       |   X   |
 * | size  |   88 |      |      |      |      |      |       |       |       |       |   X   |
 * | size  |   96 |      |      |      |      |      |       |       |       |       |   X   |
 * | size  |  104 |      |      |      |      |      |       |       |       |       |   X   |
 * | size  |  112 |      |      |      |      |      |       |       |       |       |   X   |
 * | size  |  120 |      |      |      |      |      |       |       |       |       |   X   |
 * | size  |  128 |      |      |      |      |      |       |       |       |       |   X   |
 * | ----- | ---- | ---- | ---- | ---- | ---- | ---- | ----- | ----- | ----- | ----- | ----- |
 * | count |   16 |    8 |    8 |    8 |    8 |    8 |     8 |     8 |     8 |     8 |  88   |
 * | ----- | ---- | ---- | ---- | ---- | ---- | ---- | ----- | ----- | ----- | ----- | ----- |
 */
#define XSLICE_SIZE_____8       8
#define XSLICE_SIZE____16      16
#define XSLICE_SIZE____24      24
#define XSLICE_SIZE____32      32
#define XSLICE_SIZE____40      40
#define XSLICE_SIZE____48      48
#define XSLICE_SIZE____56      56
#define XSLICE_SIZE____64      64
#define XSLICE_SIZE____72      72
#define XSLICE_SIZE____80      80
#define XSLICE_SIZE____88      88
#define XSLICE_SIZE____96      96
#define XSLICE_SIZE___104     104
#define XSLICE_SIZE___112     112
#define XSLICE_SIZE___120     120
#define XSLICE_SIZE___128     128
#define XSLICE_SIZE___144     144
#define XSLICE_SIZE___160     160
#define XSLICE_SIZE___176     176
#define XSLICE_SIZE___192     192
#define XSLICE_SIZE___208     208
#define XSLICE_SIZE___224     224
#define XSLICE_SIZE___240     240
#define XSLICE_SIZE___256     256
#define XSLICE_SIZE___288     288
#define XSLICE_SIZE___320     320
#define XSLICE_SIZE___352     352
#define XSLICE_SIZE___384     384
#define XSLICE_SIZE___416     416
#define XSLICE_SIZE___448     448
#define XSLICE_SIZE___480     480
#define XSLICE_SIZE___512     512
#define XSLICE_SIZE___576     576
#define XSLICE_SIZE___640     640
#define XSLICE_SIZE___704     704
#define XSLICE_SIZE___768     768
#define XSLICE_SIZE___832     832
#define XSLICE_SIZE___896     896
#define XSLICE_SIZE___960     960
#define XSLICE_SIZE__1024    1024
#define XSLICE_SIZE__1152    1152
#define XSLICE_SIZE__1280    1280
#define XSLICE_SIZE__1408    1408
#define XSLICE_SIZE__1536    1536
#define XSLICE_SIZE__1664    1664
#define XSLICE_SIZE__1792    1792
#define XSLICE_SIZE__1920    1920
#define XSLICE_SIZE__2048    2048
#define XSLICE_SIZE__2304    2304
#define XSLICE_SIZE__2560    2560
#define XSLICE_SIZE__2816    2816
#define XSLICE_SIZE__3072    3072
#define XSLICE_SIZE__3328    3328
#define XSLICE_SIZE__3584    3584
#define XSLICE_SIZE__3840    3840
#define XSLICE_SIZE__4096    4096
#define XSLICE_SIZE__4608    4608
#define XSLICE_SIZE__5120    5120
#define XSLICE_SIZE__5632    5632
#define XSLICE_SIZE__6144    6144
#define XSLICE_SIZE__6656    6656
#define XSLICE_SIZE__7168    7168
#define XSLICE_SIZE__7680    7680
#define XSLICE_SIZE__8192    8192
#define XSLICE_SIZE__9216    9216
#define XSLICE_SIZE_10240   10240
#define XSLICE_SIZE_11264   11264
#define XSLICE_SIZE_12288   12288
#define XSLICE_SIZE_13312   13312
#define XSLICE_SIZE_14336   14336
#define XSLICE_SIZE_15360   15360
#define XSLICE_SIZE_16384   16384
#define XSLICE_SIZE_18432   18432
#define XSLICE_SIZE_20480   20480
#define XSLICE_SIZE_22528   22528
#define XSLICE_SIZE_24576   24576
#define XSLICE_SIZE_26624   26624
#define XSLICE_SIZE_28672   28672
#define XSLICE_SIZE_30720   30720
#define XSLICE_SIZE_32768   32768
#define XSLICE_SIZE_36864   36864
#define XSLICE_SIZE_40960   40960
#define XSLICE_SIZE_45056   45056
#define XSLICE_SIZE_49152   49152
#define XSLICE_SIZE_53248   53248
#define XSLICE_SIZE_57344   57344
#define XSLICE_SIZE_61440   61440
#define XSLICE_SIZE_65536   65536

#define XSLICE_TYPE_COUNT  88

#define XCHUNK_MIN_SIZE     (1024 * 256 )
#define XCHUNK_MAX_SIZE     (1024 * 1028)
#define XCHUNK_INC_SIZE     XMEM_PAGE_SIZE

/** 所有内存分片大小的数组表 */
static x_uint32_t X_slice_size_table[XSLICE_TYPE_COUNT] =
{
    XSLICE_SIZE_____8,
    XSLICE_SIZE____16,
    XSLICE_SIZE____24,
    XSLICE_SIZE____32,
    XSLICE_SIZE____40,
    XSLICE_SIZE____48,
    XSLICE_SIZE____56,
    XSLICE_SIZE____64,
    XSLICE_SIZE____72,
    XSLICE_SIZE____80,
    XSLICE_SIZE____88,
    XSLICE_SIZE____96,
    XSLICE_SIZE___104,
    XSLICE_SIZE___112,
    XSLICE_SIZE___120,
    XSLICE_SIZE___128,
    XSLICE_SIZE___144,
    XSLICE_SIZE___160,
    XSLICE_SIZE___176,
    XSLICE_SIZE___192,
    XSLICE_SIZE___208,
    XSLICE_SIZE___224,
    XSLICE_SIZE___240,
    XSLICE_SIZE___256,
    XSLICE_SIZE___288,
    XSLICE_SIZE___320,
    XSLICE_SIZE___352,
    XSLICE_SIZE___384,
    XSLICE_SIZE___416,
    XSLICE_SIZE___448,
    XSLICE_SIZE___480,
    XSLICE_SIZE___512,
    XSLICE_SIZE___576,
    XSLICE_SIZE___640,
    XSLICE_SIZE___704,
    XSLICE_SIZE___768,
    XSLICE_SIZE___832,
    XSLICE_SIZE___896,
    XSLICE_SIZE___960,
    XSLICE_SIZE__1024,
    XSLICE_SIZE__1152,
    XSLICE_SIZE__1280,
    XSLICE_SIZE__1408,
    XSLICE_SIZE__1536,
    XSLICE_SIZE__1664,
    XSLICE_SIZE__1792,
    XSLICE_SIZE__1920,
    XSLICE_SIZE__2048,
    XSLICE_SIZE__2304,
    XSLICE_SIZE__2560,
    XSLICE_SIZE__2816,
    XSLICE_SIZE__3072,
    XSLICE_SIZE__3328,
    XSLICE_SIZE__3584,
    XSLICE_SIZE__3840,
    XSLICE_SIZE__4096,
    XSLICE_SIZE__4608,
    XSLICE_SIZE__5120,
    XSLICE_SIZE__5632,
    XSLICE_SIZE__6144,
    XSLICE_SIZE__6656,
    XSLICE_SIZE__7168,
    XSLICE_SIZE__7680,
    XSLICE_SIZE__8192,
    XSLICE_SIZE__9216,
    XSLICE_SIZE_10240,
    XSLICE_SIZE_11264,
    XSLICE_SIZE_12288,
    XSLICE_SIZE_13312,
    XSLICE_SIZE_14336,
    XSLICE_SIZE_15360,
    XSLICE_SIZE_16384,
    XSLICE_SIZE_18432,
    XSLICE_SIZE_20480,
    XSLICE_SIZE_22528,
    XSLICE_SIZE_24576,
    XSLICE_SIZE_26624,
    XSLICE_SIZE_28672,
    XSLICE_SIZE_30720,
    XSLICE_SIZE_32768,
    XSLICE_SIZE_36864,
    XSLICE_SIZE_40960,
    XSLICE_SIZE_45056,
    XSLICE_SIZE_49152,
    XSLICE_SIZE_53248,
    XSLICE_SIZE_57344,
    XSLICE_SIZE_61440,
    XSLICE_SIZE_65536
};

/**
 * @struct xchunk_alias_t
 * @brief  定义伪 chunk 的链表节点结构体，用于 双向链表 的 头部/尾部。
 * @note xchunk_alias_t 与 xmem_chunk_t 首部字段必须一致。
 */
typedef struct xchunk_alias_t
{
    x_uint32_t      xchunk_size;   ///< 内存块大小
    x_uint32_t      xslice_size;   ///< 内存分片大小

    /**
     * @brief 当前 chunk 对象所在 class 的双向链表节点信息。
     */
    struct
    {
    xchunk_handle_t xchunk_prev;   ///< 前驱节点
    xchunk_handle_t xchunk_next;   ///< 后继节点
    } xlist_node;
} xchunk_alias_t;

#define XCHUNK_RBNODE_SIZE    (5 * sizeof(x_handle_t))

/**
 * @struct xmem_chunk_t
 * @brief  内存块的结构体描述信息。
 */
typedef struct xmem_chunk_t
{
    x_uint32_t      xchunk_size;   ///< 内存块大小
    x_uint32_t      xslice_size;   ///< 内存分片大小

    /**
     * @brief 当前 chunk 对象所在 class 的双向链表节点信息。
     */
    struct
    {
    xchunk_handle_t xchunk_prev;   ///< 前驱节点
    xchunk_handle_t xchunk_next;   ///< 后继节点
    } xlist_node;

    /**
     * @brief 用于红黑树的节点占位结构体，记录当前 chunk 对象在存储管理中的位置信息。
     * @note  红黑树节点的 5 个基本字段：颜色值、父节点、左子树、右子树、索引键。
     */
    struct
    {
    x_byte_t xbt_ptr[XCHUNK_RBNODE_SIZE]; ///< 此字段仅起到内存占位的作用
    } xtree_node;

    /**
     * @brief 当前 chunk 对象的持有者。
     * @note
     * 内存分片队列中的 xslice_queue.xut_capacity，
     * 等于 0 时，持有对象为 xmem_pool_t 类型，否则为 xmem_class_t 类型。
     */
    union
    {
    xmpool_handle_t xmpool_ptr;    ///< 当前 chunk 所在的 pool 对象
    xclass_handle_t xclass_ptr;    ///< 当前 chunk 所在的 class 对象
    } xowner;

    /**
     * @brief 内存分片索引号队列。
     * @note
     * 此为一个环形队列，xut_index[] 数组中的值，
     * 第 0~14 位 表示分片索引号，
     * 第  15  位 表示分片是否已经分配出去。
     */
    struct
    {
    x_uint16_t      xut_offset;    ///< 内存分片组的首地址偏移量
    x_uint16_t      xut_capacity;  ///< 队列的分片索引数组容量
    x_uint16_t      xut_bpos;      ///< 队列的分片索引数组起始位置
    x_uint16_t      xut_epos;      ///< 队列的分片索引数组结束位置
    x_uint16_t      xut_index[0];  ///< 队列的分片索引数组
    } xslice_queue;
} xmem_chunk_t;

/** chunk 对象的左（起始）地址（xmem_slice_t 类型指针） */
#define XCHUNK_LADDR(xchunk_ptr) ((xmem_slice_t)(xchunk_ptr))

/** chunk 对象的右（结束）地址（xmem_slice_t 类型指针） */
#define XCHUNK_RADDR(xchunk_ptr) \
    (XCHUNK_LADDR(xchunk_ptr) + (xchunk_ptr)->xchunk_size)

/** chunk 对象分片队列大小（容量） */
#define XCHUNK_SLICE_QSIZE(xchunk_ptr) \
    ((xchunk_ptr)->xslice_queue.xut_capacity)

/** chunk 对象中未被分配出去的分片数量 */
#define XCHUNK_SLICE_COUNT(xchunk_ptr)                                    \
    ((x_uint16_t)(((x_uint16_t)(xchunk_ptr)->xslice_queue.xut_epos) +     \
                 ~((x_uint16_t)(xchunk_ptr)->xslice_queue.xut_bpos) + 1)) \

/** chunk 对象的分片队列是否已空，即所有分片都已经被分配出去 */
#define XCHUNK_SLICE_EMPTY(xchunk_ptr) \
    ((xchunk_ptr)->xslice_queue.xut_epos == (xchunk_ptr)->xslice_queue.xut_bpos)

/** chunk 对象的分片队列是否已满，即没有任何一个分片被分配出去 */
#define XCHUNK_SLICE_QFULL(xchunk_ptr) \
    (XCHUNK_SLICE_COUNT(xchunk_ptr) == XCHUNK_SLICE_QSIZE(xchunk_ptr))

/** chunk 对象中的分片索引号位置 */
#define XCHUNK_SLICE_INDEX(xchunk_ptr, xut_pos) \
    ((xchunk_ptr)->xslice_queue.xut_index[      \
        (xut_pos) % (xchunk_ptr)->xslice_queue.xut_capacity])

/** 获取 chunk 对象中的分片索引号值 */
#define XCHUNK_SLICE_INDEX_GET(xchunk_ptr, xut_pos) \
    (XCHUNK_SLICE_INDEX(xchunk_ptr, xut_pos) & 0x7FFF)

/** 设置 chunk 对象中的分片索引号值 */
#define XCHUNK_SLICE_INDEX_SET(xchunk_ptr, xut_pos, xut_index)  \
    (XCHUNK_SLICE_INDEX(xchunk_ptr, xut_pos) =                  \
        ((XCHUNK_SLICE_INDEX(xchunk_ptr, xut_pos) & 0x8000) |   \
         (((x_uint16_t)(xut_index)) & 0x7FFF)))

/** 判断 chunk 对象中的分片是否已被分配出去 */
#define XCHUNK_SLICE_ALLOCATED(xchunk_ptr, xut_index) \
    (0x8000 == (XCHUNK_SLICE_INDEX(xchunk_ptr, xut_index) & 0x8000))

/** 标识 chunk 对象中的分片已经被分配出去 */
#define XCHUNK_SLICE_ALLOCATED_SET(xchunk_ptr, xut_index)   \
    (XCHUNK_SLICE_INDEX(xchunk_ptr, xut_index) |= 0x8000)

/** 标识 chunk 对象中的分片未被分配出去 */
#define XCHUNK_SLICE_ALLOCATED_RESET(xchunk_ptr, xut_index) \
    (XCHUNK_SLICE_INDEX(xchunk_ptr, xut_index) &= 0x7FFF)

/** chunk 对象的分片起始地址 */
#define XCHUNK_SLICE_BEGIN(xchunk_ptr) \
    ((xmem_slice_t)(xchunk_ptr) + (xchunk_ptr)->xslice_queue.xut_offset)

/** 获取 chunk 对象的分片地址 */
#define XCHUNK_SLICE_GET(xchunk_ptr, xut_index)  \
    (XCHUNK_SLICE_BEGIN(xchunk_ptr) +            \
     ((x_uint16_t)(xut_index)) * (xchunk_ptr)->xslice_size)

/** chunk 对象的分片结束地址 */
#define XCHUNK_SLICE_END(xchunk_ptr) XCHUNK_RADDR(xchunk_ptr)

/**
 * @struct xmem_class_t
 * @brief  内存分类的结构体描述信息。
 */
typedef struct xmem_class_t
{
    x_uint32_t      xchunk_size;   ///< 内存块大小
    x_uint32_t      xslice_size;   ///< 内存分片大小

    x_uint32_t      xchunk_count;  ///< 内存块数量
    x_uint32_t      xslice_count;  ///< 可用的（未被分配出去的）内存分片数量

    xmpool_handle_t xmpool_ptr;    ///< 持有当前 内存分类 对象的 内存池

    /**
     * @brief 管理该内存分类下所有 chunk 所使用的双向链表结构体。
     */
    xchunk_alias_t  xlist_head;    ///< 双向链表的头部伪 chunk 节点
    xchunk_alias_t  xlist_tail;    ///< 双向链表的尾部伪 chunk 节点
} xmem_class_t;

#define XCLASS_LIST_HEAD(xclass_ptr)  ((xchunk_handle_t)&(xclass_ptr)->xlist_head)
#define XCLASS_LIST_FRONT(xclass_ptr) ((xclass_ptr)->xlist_head.xlist_node.xchunk_next)
#define XCLASS_LIST_BACK(xclass_ptr)  ((xclass_ptr)->xlist_tail.xlist_node.xchunk_prev)
#define XCLASS_LIST_TAIL(xclass_ptr)  ((xchunk_handle_t)&(xclass_ptr)->xlist_tail)

#define XSLICE_ARRAY_SIZE  \
    ((XMEM_PAGE_SIZE - (2 * sizeof(xslice_arrptr_t))) / sizeof(xmem_slice_t))

/**
 * @struct xslice_array_t
 * @brief  分片数组块。其作为 xslice_rqueue_t 的链表节点块。
 */
typedef struct xslice_array_t
{
    xslice_arrptr_t  xarray_prev;  ///< 前驱节点
    xslice_arrptr_t  xarray_next;  ///< 后继节点
    xmem_slice_t     xslice_aptr[XSLICE_ARRAY_SIZE]; ///< 分片数组
} xslice_array_t;

typedef volatile x_uint32_t      xatomic_size_t;
typedef volatile xslice_arrptr_t xatomic_arrptr_t;
typedef volatile x_uint32_t      xatomic_lock_t;

/**
 * @struct xslice_rqueue_t
 * @brief  用于存储 待回收的内存分片 的队列。
 */
typedef struct xslice_rqueue_t
{
    xatomic_size_t   xqueue_size;   ///< 队列中的分片数量
    xatomic_arrptr_t xarray_sptr;   ///< 用于保存临时分片数组块（备用缓存块）

    x_uint32_t       xarray_bpos;   ///< 队列中的起始位置
    xslice_arrptr_t  xarray_bptr;   ///< 分片数组块链表的起始块

    x_uint32_t       xarray_epos;   ///< 队列中的终点位置
    xslice_arrptr_t  xarray_eptr;   ///< 分片数组块链表的终点块
} xslice_rqueue_t;

#define XMPOOL_RBTREE_SIZE    (16 * sizeof(x_handle_t))

/**
 * @struct xmem_pool_t
 * @brief  内存池的结构体描述信息。
 */
typedef struct xmem_pool_t
{
    xfunc_alloc_t   xfunc_alloc;   ///< 申请堆内存块的接口
    xfunc_free_t    xfunc_free;    ///< 释放堆内存块的接口
    x_handle_t      xht_context;   ///< 调用 xfunc_alloc/xfunc_free 时回调的上下文句柄

    x_uint64_t      xsize_cached;  ///< 总共缓存的内存大小
    x_uint64_t      xsize_valid;   ///< 可使用到的缓存大小
    x_uint64_t      xsize_using;   ///< 正在使用的缓存大小

    x_uint32_t      xut_worktid;   ///< 隶属的工作线程 ID
    xatomic_lock_t  xspinlock_que; ///< 队列操作的同步旋转锁
    xslice_rqueue_t xslice_rqueue; ///< 待回收的内存分片 的队列

    xchunk_handle_t xchunk_cptr;   ///< 记录当前操作的 chunk 对象

    /**
     * @brief 存储管理所有 chunk 内存块对象的红黑树（使用 emplace 方式进行创建）。
     */
    struct
    {
    x_byte_t xbt_ptr[XMPOOL_RBTREE_SIZE]; ///< 此字段仅起到内存占位的作用
    } xrbtree;

    xmem_class_t    xclass_ptr[XSLICE_TYPE_COUNT]; ///< 各个内存分类
} xmem_pool_t;

#define XMPOOL_RBTREE(xmpool_ptr) ((x_rbtree_ptr)(xmpool_ptr)->xrbtree.xbt_ptr)

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 原子操作：比较成功后赋值。
 * @note  返回目标变量的旧值。
 */
static inline x_void_t * xatomic_xchg_ptr(
    x_void_t * volatile * xdst_ptr, x_void_t * xchg_ptr)
{
#ifdef _MSC_VER
    return _InterlockedExchangePointer(xdst_ptr, xchg_ptr);
#elif defined(__GNUC__)
    x_void_t * xold_ptr;
    do 
    {
        xold_ptr = *xdst_ptr;
    } while (!__sync_bool_compare_and_swap(xdst_ptr, xold_ptr, xchg_ptr));
    return xold_ptr;
#else 
    XASSERT(X_FALSE);
    x_void_t * xold_ptr = *xdst_ptr;
    *xdst_ptr = xchg_ptr;
    return xold_ptr;
#endif
}

/**********************************************************/
/**
 * @brief 原子操作：比较成功后赋值。
 * @note  返回目标变量的旧值。
 */
static inline x_uint32_t xatomic_cmpxchg_32(
    volatile x_uint32_t * xut_dest, x_uint32_t xut_exchange, x_uint32_t xut_compare)
{
#ifdef _MSC_VER
    return _InterlockedCompareExchange(xut_dest, xut_exchange, xut_compare);
#elif defined(__GNUC__)
    return __sync_val_compare_and_swap(xut_dest, xut_compare, xut_exchange);
#else
    XASSERT(X_FALSE);
    x_uint32_t xut_old = *xut_dest;
    if (*xut_dest == xut_exchange) *xut_dest = xut_exchange;
    return xut_old;
#endif
}

/**********************************************************/
/**
 * @brief 原子操作：加法。
 * @note  返回目标变量的旧值。
 */
static inline x_uint32_t xatomic_add_32(
    volatile x_uint32_t * xut_dest, x_uint32_t xut_value)
{
#ifdef _MSC_VER
    return _InterlockedExchangeAdd(xut_dest, xut_value);
#elif defined __GNUC__
    return __sync_fetch_and_add(xut_dest, xut_value);
#else
    XASSERT(X_FALSE);
    x_uint32_t xut_old = *xut_dest;
    *xut_dest += xut_value;
    return xut_old;
#endif
}

/**********************************************************/
/**
 * @brief 原子操作：减法。
 * @note  返回目标变量的旧值。
 */
static inline x_uint32_t xatomic_sub_32(
    volatile x_uint32_t * xut_dest, x_uint32_t xut_value)
{
#ifdef _MSC_VER
    return _InterlockedExchangeAdd(xut_dest, ~xut_value + 1);
#elif defined(__GNUC__)
    return __sync_fetch_and_sub(xut_dest, xut_value);
#else
    XASSERT(X_FALSE);
    x_uint32_t xut_old = *xut_dest;
    *xut_dest += xut_value;
    return xut_old;
#endif
}

/**********************************************************/
/**
 * @brief 获取当前线程 ID 值。
 */
static inline x_uint32_t xsys_tid(void)
{
#ifdef _MSC_VER
    return (x_uint32_t)GetCurrentThreadId();
#elif defined(__GNUC__)
    return (x_uint32_t)syscall(__NR_gettid);
#else
    XASSERT(X_FALSE);
    return 0;
#endif
}

/**********************************************************/
/**
 * @brief 内存清零。
 */
static x_void_t * xmem_clear(x_void_t * xmem_ptr, x_uint32_t xut_size)
{
    return memset(xmem_ptr, 0, xut_size);
}

/**********************************************************/
/**
 * @brief 默认的 堆内存块 申请接口。
 * 
 * @param [in ] xst_size    : 请求的堆内存块大小。
 * @param [in ] xht_owner   : 持有该（返回的）堆内存块的标识句柄。
 * @param [in ] xht_context : 回调的上下文标识句柄。
 * 
 * @return x_void_t *
 *         - 成功，返回 堆内存块 地址；
 *         - 失败，返回 X_NULL。
 */
static x_void_t * xmem_heap_alloc(x_size_t xst_size,
                                  x_handle_t xht_owner,
                                  x_handle_t xht_context)
{
    return malloc(xst_size);
}

/**********************************************************/
/**
 * @brief 默认的 堆内存块 释放接口。
 * 
 * @param [in ] xchunk_ptr  : 待释放的堆内存块。
 * @param [in ] xst_size    : 待释放的堆内存块大小。
 * @param [in ] xht_owner   : 持有该堆内存块的标识句柄。
 * @param [in ] xht_context : 回调的上下文标识句柄。
 */
static x_void_t xmem_heap_free(x_void_t * xchunk_ptr,
                               x_size_t xst_size,
                               x_handle_t xht_owner,
                               x_handle_t xht_context)
{
    if (X_NULL != xchunk_ptr)
        free(xchunk_ptr);
}

/**********************************************************/
/**
 * @brief 计算 xchunk_size 按 xslice_size 进行分片时，未使用到的字节数。
 */
static inline x_uint32_t xmem_chunk_unused_size(
                                    x_uint32_t xchunk_size,
                                    x_uint32_t xslice_size)
{
    XASSERT(xchunk_size >=
            (sizeof(xmem_chunk_t) + sizeof(x_uint16_t) + xslice_size));

    xchunk_size -= sizeof(xmem_chunk_t);
    xslice_size += sizeof(x_uint16_t  );

	return (xchunk_size - (xchunk_size / xslice_size * xslice_size));
}

/**********************************************************/
/**
 * @brief 计算 xchunk_size 按 xslice_size 进行分片时，可分片的数量。
 */
static inline x_uint32_t xmem_chunk_capacity(
                                    x_uint32_t xchunk_size,
                                    x_uint32_t xslice_size)
{
    XASSERT(xchunk_size >=
            (sizeof(xmem_chunk_t) + sizeof(x_uint16_t) + xslice_size));

	return ((xchunk_size - sizeof(xmem_chunk_t)) /
            (xslice_size + sizeof(x_uint16_t  )));
}

/**********************************************************/
/**
 * @brief 按所需的（内存块）长度值，计算内存对齐后的数值。
 */
x_uint32_t xmem_align_size(x_uint32_t xut_size)
{
    static x_uint32_t xsize_align_8[32] =
    {
          8,  16,  24,  32,  40,  48,  56,  64,
         72,  80,  88,  96, 104, 112, 120, 128,
        144, 144, 160, 160, 176, 176, 192, 192,
        208, 208, 224, 224, 240, 240, 256, 256
    };

    static x_uint32_t xsize_align_32[24] =
    {
        288, 320, 352, 384, 416, 448,  480,  512, 
        576, 576, 640, 640, 704, 704,  768,  768, 
        832, 832, 896, 896, 960, 960, 1024, 1024
    };

    static x_uint32_t xsize_align_128[24] =
    {
        1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048,
        2304, 2304, 2560, 2560, 2816, 2816, 3072, 3072,
        3328, 3328, 3584, 3584, 3840, 3840, 4096, 4096
    };

    static x_uint32_t xsize_align_512[24] =
    {
         4608,  5120,  5632,  6144,  6656,  7168,  7680,  8192, 
         9216,  9216, 10240, 10240, 11264, 11264, 12288, 12288, 
        13312, 13312, 14336, 14336, 15360, 15360, 16384, 16384
    };

    static x_uint32_t xsize_align_2048[24] =
    {
        18432, 20480, 22528, 24576, 26624, 28672, 30720, 32768,
        36864, 36864, 40960, 40960, 45056, 45056, 49152, 49152,
        53248, 53248, 57344, 57344, 61440, 61440, 65536, 65536
    };

#define X_INDEX(size, align) (((size) + (align) - 1) / (align) - 1)
#define X_ALIGN(size, align) (((size) + ((align) - 1)) & (~((align) - 1)))

    if (xut_size <= 1024)
    {
        if (xut_size <= 256)
        {
            if (xut_size <= 0)
                return 0;
            return xsize_align_8[X_INDEX(xut_size, 8)];
        }
        return xsize_align_32[X_INDEX(xut_size - 256, 32)];
    }
    else if (xut_size <= 16384)
    {
        if (xut_size <= 4096)
            return xsize_align_128[X_INDEX(xut_size - 1024, 128)];
        return xsize_align_512[X_INDEX(xut_size - 4096, 512)];
    }
    else if (xut_size <= 65536)
    {
        return xsize_align_2048[X_INDEX(xut_size - 16384, 2048)];
    }

    xut_size += sizeof(xmem_chunk_t);
    return X_ALIGN(xut_size, XMEM_PAGE_SIZE);

#undef X_INDEX
#undef X_ALIGN
}

////////////////////////////////////////////////////////////////////////////////

static inline x_bool_t xmpool_dealloc_chunk(xmpool_handle_t , xchunk_handle_t);

//====================================================================

// 
// xmem_chunk_t 相关的操作接口
// 

/**********************************************************/
/**
 * @brief 查找分片索引号是否还在 chunk 的分片队列中。
 */
static x_bool_t xchunk_find_qindex(
                    xchunk_handle_t xchunk_ptr,
                    x_uint16_t xut_qindex)
{
    x_uint16_t xut_iter = xchunk_ptr->xslice_queue.xut_bpos;
    while (xut_iter != xchunk_ptr->xslice_queue.xut_epos)
    {
        if (xut_qindex == XCHUNK_SLICE_INDEX_GET(xchunk_ptr, xut_iter))
        {
            return X_TRUE;
        }

        xut_iter += 1;
    }

    return X_FALSE;
}

/**********************************************************/
/**
 * @brief 从 chunk 对象中申请内存分片。
 * 
 * @param [in ] xchunk_ptr : chunk 对象。
 * 
 * @return xmem_slice_t
 *         - 成功，返回 内存分片；
 *         - 失败，返回 X_NULL，chunk 对象的分片队列已经为空。
 */
static xmem_slice_t xchunk_alloc_slice(xchunk_handle_t xchunk_ptr)
{
    x_uint16_t xut_index = 0;

    if (XCHUNK_SLICE_EMPTY(xchunk_ptr))
    {
        return X_NULL;
    }

    xut_index = XCHUNK_SLICE_INDEX_GET(
                    xchunk_ptr, xchunk_ptr->xslice_queue.xut_bpos);

    XASSERT(xut_index < XCHUNK_SLICE_QSIZE(xchunk_ptr));
    XASSERT(!XCHUNK_SLICE_ALLOCATED(xchunk_ptr, xut_index));

    // 设置分片“已被分配出去”的标识位
    XCHUNK_SLICE_ALLOCATED_SET(xchunk_ptr, xut_index);

    xchunk_ptr->xslice_queue.xut_bpos += 1;
    XASSERT(0 != xchunk_ptr->xslice_queue.xut_bpos);

    xchunk_ptr->xowner.xclass_ptr->xslice_count -= 1;

    return XCHUNK_SLICE_GET(xchunk_ptr, xut_index);
}

/**********************************************************/
/**
 * @brief 回收内存分片至 chunk 对象中。
 * 
 * @param [in ] xchunk_ptr : chunk 对象。
 * @param [in ] xmem_slice : 待回收的内存分片。
 * 
 * @return x_int32_t
 *         - 成功，返回 XMEM_ERR_OK；
 *         - 失败，返回 错误码。
 */
static x_int32_t xchunk_recyc_slice(
                    xchunk_handle_t xchunk_ptr,
                    xmem_slice_t xmem_slice)
{
    XASSERT((xmem_slice > XCHUNK_LADDR(xchunk_ptr)) &&
            (xmem_slice < XCHUNK_RADDR(xchunk_ptr)));
    XASSERT(XCHUNK_SLICE_QSIZE(xchunk_ptr) > 0);
    XASSERT(!XCHUNK_SLICE_QFULL(xchunk_ptr));

    x_int32_t  xit_error  = XMEM_ERR_UNKNOW;
    x_uint32_t xut_offset = 0;
    x_uint32_t xut_index  = 0;

    do
    {
        //======================================

        xut_offset = (x_uint32_t)(xmem_slice - XCHUNK_LADDR(xchunk_ptr));
        if (xut_offset < xchunk_ptr->xslice_queue.xut_offset)
        {
            xit_error = XMEM_ERR_SLICE_UNALIGNED;
            break;
        }

        xut_offset -= xchunk_ptr->xslice_queue.xut_offset;
        if (0 != (xut_offset % xchunk_ptr->xslice_size))
        {
            xit_error = XMEM_ERR_SLICE_UNALIGNED;
            break;
        }

        //======================================

        xut_index = xut_offset / xchunk_ptr->xslice_size;
        XASSERT(xut_index < XCHUNK_SLICE_QSIZE(xchunk_ptr));

        // 判断分片是否已经被回收
        if (!XCHUNK_SLICE_ALLOCATED(xchunk_ptr, xut_index))
        {
            xit_error = XMEM_ERR_SLICE_RECYCLED;
            break;
        }

        XASSERT(!xchunk_find_qindex(xchunk_ptr, (x_uint16_t)xut_index));
        XCHUNK_SLICE_INDEX_SET(
            xchunk_ptr, xchunk_ptr->xslice_queue.xut_epos, xut_index);

        // 标识分片“未被分配出去”
        XCHUNK_SLICE_ALLOCATED_RESET(xchunk_ptr, xut_index);

        xchunk_ptr->xslice_queue.xut_epos += 1;

        if (0 == xchunk_ptr->xslice_queue.xut_epos)
        {
            xchunk_ptr->xslice_queue.xut_epos  = XCHUNK_SLICE_COUNT(xchunk_ptr);
            xchunk_ptr->xslice_queue.xut_bpos %= xchunk_ptr->xslice_queue.xut_capacity;
            xchunk_ptr->xslice_queue.xut_epos += xchunk_ptr->xslice_queue.xut_bpos;
        }

        xchunk_ptr->xowner.xclass_ptr->xslice_count += 1;

        //======================================

        xit_error = XMEM_ERR_OK;
    } while (0);

    return xit_error;
}

//====================================================================

// 
// xmem_class_t 相关的操作接口
// 

/**********************************************************/
/**
 * @brief 将 chunk 对象从分类管理的链表中移除。
 */
static x_void_t xclass_list_erase_chunk(
                            xclass_handle_t xclass_ptr,
                            xchunk_handle_t xchunk_ptr)
{
    XASSERT(xclass_ptr == xchunk_ptr->xowner.xclass_ptr);

    xchunk_ptr->xlist_node.xchunk_prev->xlist_node.xchunk_next =
        xchunk_ptr->xlist_node.xchunk_next;
    xchunk_ptr->xlist_node.xchunk_next->xlist_node.xchunk_prev =
        xchunk_ptr->xlist_node.xchunk_prev;

    xclass_ptr->xchunk_count -= 1;
    xclass_ptr->xslice_count -= XCHUNK_SLICE_COUNT(xchunk_ptr);
}

/**********************************************************/
/**
 * @brief 将 chunk 对象插入到分类管理的链表头部。
 */
static x_void_t xclass_list_push_head(
                            xclass_handle_t xclass_ptr,
                            xchunk_handle_t xchunk_ptr)
{
    XASSERT(xclass_ptr == xchunk_ptr->xowner.xclass_ptr);

    if (XCLASS_LIST_FRONT(xclass_ptr) == xchunk_ptr)
    {
        return;
    }

    xchunk_ptr->xlist_node.xchunk_prev = XCLASS_LIST_HEAD(xclass_ptr);
    xchunk_ptr->xlist_node.xchunk_next = XCLASS_LIST_FRONT(xclass_ptr);

    XCLASS_LIST_FRONT(xclass_ptr)->xlist_node.xchunk_prev = xchunk_ptr;
    XCLASS_LIST_HEAD(xclass_ptr)->xlist_node.xchunk_next  = xchunk_ptr;

    xclass_ptr->xchunk_count += 1;
    xclass_ptr->xslice_count += XCHUNK_SLICE_COUNT(xchunk_ptr);
}

/**********************************************************/
/**
 * @brief 将 chunk 对象插入到分类管理的链表尾部。
 */
static x_void_t xclass_list_push_tail(
                            xclass_handle_t xclass_ptr,
                            xchunk_handle_t xchunk_ptr)
{
    XASSERT(xclass_ptr == xchunk_ptr->xowner.xclass_ptr);

    if (xclass_ptr->xlist_tail.xlist_node.xchunk_prev == xchunk_ptr)
    {
        return;
    }

    xchunk_ptr->xlist_node.xchunk_next = XCLASS_LIST_TAIL(xclass_ptr);
    xchunk_ptr->xlist_node.xchunk_prev = XCLASS_LIST_BACK(xclass_ptr);

    XCLASS_LIST_BACK(xclass_ptr)->xlist_node.xchunk_next = xchunk_ptr;
    XCLASS_LIST_TAIL(xclass_ptr)->xlist_node.xchunk_prev = xchunk_ptr;

    xclass_ptr->xchunk_count += 1;
    xclass_ptr->xslice_count += XCHUNK_SLICE_COUNT(xchunk_ptr);
}

/**********************************************************/
/**
 * @brief 当 chunk 对象有 分片 申请（或 回收）操作时，
 *        更新 chunk 对象在分类管理的链表中的位置，优化后续的操作。
 */
static x_void_t xclass_list_update(
                            xclass_handle_t xclass_ptr,
                            xchunk_handle_t xchunk_ptr)
{
    if (xclass_ptr->xchunk_count <= 1)
    {
        return;
    }

#if 0
    if (X_CHUNK_SLICE_QFULL(xchunk_ptr))
    {
        // 若当前分类下的分片数量，大于 chunk 容量的 4 倍，
        // 则直接释放掉当前 chunk 对象，否则，后续有可能将置于链表的起始位置
        if (xclass_ptr->xslice_count >
            (x_uint32_t)(4 * X_CHUNK_SLICE_QSIZE(xchunk_ptr)))
        {
            xmpool_dealloc_chunk(xclass_ptr->xmpool_ptr, xchunk_ptr);
            return;
        }
    }
#endif

    if (XCHUNK_SLICE_COUNT(xchunk_ptr) >
        XCHUNK_SLICE_COUNT(XCLASS_LIST_FRONT(xclass_ptr)))
    {
        xclass_list_erase_chunk(xclass_ptr, xchunk_ptr);
        xclass_list_push_head(xclass_ptr, xchunk_ptr);
    }
    else if (XCHUNK_SLICE_EMPTY(xchunk_ptr))
    {
        if (xchunk_ptr != xclass_ptr->xlist_tail.xlist_node.xchunk_prev)
        {
            xclass_list_erase_chunk(xclass_ptr, xchunk_ptr);
            xclass_list_push_tail(xclass_ptr, xchunk_ptr);
        }
    }
}

/**********************************************************/
/**
 * @brief 从分类管理的 class 对象中，获取首个非空（仍可分配到分片）chunk 对象。
 */
static xchunk_handle_t xclass_get_non_empty_chunk(xclass_handle_t xclass_ptr)
{
    xchunk_handle_t xchunk_ptr = X_NULL;

    if ((xclass_ptr->xchunk_count <= 0) || (xclass_ptr->xslice_count <= 0))
    {
        return X_NULL;
    }

    for (xchunk_ptr  = XCLASS_LIST_FRONT(xclass_ptr);
         xchunk_ptr != XCLASS_LIST_TAIL(xclass_ptr);
         xchunk_ptr  = xchunk_ptr->xlist_node.xchunk_next)
    {
        if (!XCHUNK_SLICE_EMPTY(xchunk_ptr))
        {
            if (xchunk_ptr != XCLASS_LIST_FRONT(xclass_ptr))
            {
                xclass_list_erase_chunk(xclass_ptr, xchunk_ptr);
                xclass_list_push_head(xclass_ptr, xchunk_ptr);
            }

            return xchunk_ptr;
        }
    }

    return X_NULL;
}

//====================================================================

// 
// xslice_rqueue_t : 分片回收队列的相关操作接口
// 

/**********************************************************/
/**
 * @brief 初始化 xslice_rqueue_t 对象。
 */
static x_void_t xsrque_init(xsrque_handle_t xsrque_ptr)
{
    xsrque_ptr->xqueue_size = 0;
    xsrque_ptr->xarray_sptr = X_NULL;
    xsrque_ptr->xarray_bpos = 0;
    xsrque_ptr->xarray_bptr = X_NULL;
    xsrque_ptr->xarray_epos = 0;
    xsrque_ptr->xarray_eptr = X_NULL;

    xsrque_ptr->xarray_bptr =
        (xslice_arrptr_t)xmem_heap_alloc(sizeof(xslice_array_t), X_NULL, X_NULL);
    xsrque_ptr->xarray_eptr = xsrque_ptr->xarray_bptr;

    XASSERT(X_NULL != xsrque_ptr->xarray_bptr);
}

/**********************************************************/
/**
 * @brief 释放 xslice_rqueue_t 对象的系统资源。
 */
static x_void_t xsrque_release(xsrque_handle_t xsrque_ptr)
{
    xslice_arrptr_t xarray_ptr = X_NULL;

    XASSERT(0 == xatomic_add_32(&xsrque_ptr->xqueue_size, 0));

    //======================================

    while (X_TRUE)
    {
        if (xsrque_ptr->xarray_bptr == xsrque_ptr->xarray_eptr)
        {
            xmem_heap_free(
                xsrque_ptr->xarray_bptr, sizeof(xslice_array_t), X_NULL, X_NULL);
            break;
        }

        xarray_ptr = xsrque_ptr->xarray_bptr;
        xsrque_ptr->xarray_bptr = xsrque_ptr->xarray_bptr->xarray_next;
        if (X_NULL != xarray_ptr)
        {
            xmem_heap_free(xarray_ptr, sizeof(xslice_array_t), X_NULL, X_NULL);
        }
    }

    //======================================

    xarray_ptr = (xslice_arrptr_t)xatomic_xchg_ptr(
        (x_void_t * volatile *)&xsrque_ptr->xarray_sptr, (x_void_t *)X_NULL);

    if (X_NULL != xarray_ptr)
    {
        xmem_heap_free(xarray_ptr, sizeof(xslice_array_t), X_NULL, X_NULL);
    }

    //======================================

    xsrque_ptr->xqueue_size = 0;
    xsrque_ptr->xarray_sptr = X_NULL;
    xsrque_ptr->xarray_bpos = 0;
    xsrque_ptr->xarray_bptr = X_NULL;
    xsrque_ptr->xarray_epos = 0;
    xsrque_ptr->xarray_eptr = X_NULL;

    //======================================
}

/**********************************************************/
/**
 * @brief 向 xslice_rqueue_t 队列尾端压入一个分片。
 */
static x_void_t xsrque_push(xsrque_handle_t xsrque_ptr, xmem_slice_t xemt_value)
{
    xslice_arrptr_t xarray_ptr = X_NULL;

    xsrque_ptr->xarray_eptr->xslice_aptr[xsrque_ptr->xarray_epos] = xemt_value;

    xatomic_add_32(&xsrque_ptr->xqueue_size, 1);

    if (++xsrque_ptr->xarray_epos == XSLICE_ARRAY_SIZE)
    {
        xarray_ptr = (xslice_arrptr_t)xatomic_xchg_ptr(
            (x_void_t * volatile *)&xsrque_ptr->xarray_sptr, (x_void_t *)X_NULL);

        if (X_NULL != xarray_ptr)
        {
            xsrque_ptr->xarray_eptr->xarray_next = xarray_ptr;
            xarray_ptr->xarray_prev = xsrque_ptr->xarray_eptr;
        }
        else
        {
            xsrque_ptr->xarray_eptr->xarray_next =
                (xslice_arrptr_t)xmem_heap_alloc(sizeof(xslice_array_t), X_NULL, X_NULL);
            xsrque_ptr->xarray_eptr->xarray_prev = xsrque_ptr->xarray_eptr;
        }

        xsrque_ptr->xarray_eptr = xsrque_ptr->xarray_eptr->xarray_next;
        xsrque_ptr->xarray_epos = 0;
    }
}

/**********************************************************/
/**
 * @brief 从 xslice_rqueue_t 队列前端弹出一个分片。
 */
static xmem_slice_t xsrque_pop(xsrque_handle_t xsrque_ptr)
{
    xmem_slice_t    xmem_slice = X_NULL;
    xslice_arrptr_t xarray_ptr = X_NULL;

    if (0 == xatomic_add_32(&xsrque_ptr->xqueue_size, 0))
    {
        return X_NULL;
    }

    xatomic_sub_32(&xsrque_ptr->xqueue_size, 1);

    xmem_slice = xsrque_ptr->xarray_bptr->xslice_aptr[xsrque_ptr->xarray_bpos];

    if (++xsrque_ptr->xarray_bpos == XSLICE_ARRAY_SIZE)
    {
        xarray_ptr = xsrque_ptr->xarray_bptr;
        xsrque_ptr->xarray_bptr = xsrque_ptr->xarray_bptr->xarray_next;
        XASSERT(X_NULL != xsrque_ptr->xarray_bptr);
        xsrque_ptr->xarray_bptr->xarray_prev = X_NULL;
        xsrque_ptr->xarray_bpos = 0;

        xarray_ptr = (xslice_arrptr_t)xatomic_xchg_ptr(
            (x_void_t * volatile *)&xsrque_ptr->xarray_sptr, xarray_ptr);

        if (X_NULL != xarray_ptr)
        {
            xmem_heap_free(xarray_ptr, sizeof(xslice_array_t), X_NULL, X_NULL);
        }
    }

    return xmem_slice;
}

//====================================================================

// 
// xmem_pool_t ：存储管理使用的红黑树的相关操作接口
// 

XRBTREE_CTYPE_API(xchunk_handle_t, static, inline, chunk)

/**********************************************************/
/**
 * @brief 查找内存分片可能隶属的 chunk 对象。
 * @note 该接口只检测内存分片的地址是否在 chunk 对象内存区域的范围内，
 *       对分片 “是否对齐 chunk 对象内的有效分片位置” 并不关心。
 */
static xchunk_handle_t xrbtree_hit_chunk(
                            x_rbtree_ptr xthis_ptr,
                            xmem_slice_t xmem_slice)
{
    x_rbnode_iter   xiter_node = XRBT_NULL;
    xchunk_handle_t xchunk_ptr = X_NULL;

#if ENABLE_XASSERT
    // 为了避免 xrbtree_chunk_compare() 中的 XASSERT_CHECK() 的
    // 错误断言，调试版本下使用这一部分代码实现

    // 将 xmem_slice 伪造成 chunk 对象，并将 xchunk_size 置 1，
    // 以便在 红黑树节点索引键 的回调比对时，好判断 slice 是否属于
    // 某个 chunk 对象，详细过程可参看 xrbtree_chunk_compare()
    x_uint32_t xsize_backup = ((xchunk_handle_t)xmem_slice)->xchunk_size;
    ((xchunk_handle_t)xmem_slice)->xchunk_size = 1;
#endif // ENABLE_XASSERT

    xiter_node = xrbtree_lower_bound_chunk(
                        xthis_ptr, (xchunk_handle_t)xmem_slice);

    if (!xrbtree_iter_is_nil(xiter_node))
    {
        xchunk_ptr = xrbtree_iter_chunk(xiter_node);
        XASSERT(X_NULL != xchunk_ptr);
        XASSERT(xmem_slice > XCHUNK_LADDR(xchunk_ptr));

        if (xmem_slice >= XCHUNK_RADDR(xchunk_ptr))
        {
            xchunk_ptr = X_NULL;
        }
    }

#if ENABLE_XASSERT
    // 还原 xchunk_size 的值
    ((xchunk_handle_t)xmem_slice)->xchunk_size = xsize_backup;
#endif // ENABLE_XASSERT

    return xchunk_ptr;
}

/**********************************************************/
/**
 * @brief 红黑树申请节点对象缓存的回调函数。
 *
 * @param [in ] xrbt_vkey : 请求申请缓存的节点索引键（插入操作时回调回来的索引键）。
 * @param [in ] xst_nsize : 节点对象所需缓存的大小（即 请求申请缓存的大小）。
 * @param [in ] xrbt_ctxt : 回调的上下文标识。
 * 
 * @return xrbt_void_t *
 *         - 节点对象缓存。
 */
static xrbt_void_t * xrbtree_node_memalloc(
                            xrbt_vkey_t xrbt_vkey,
                            xrbt_size_t xst_nsize,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(xst_nsize <= XCHUNK_RBNODE_SIZE);
    return (xrbt_void_t *)((*(xchunk_handle_t *)xrbt_vkey)->xtree_node.xbt_ptr);
}

/**********************************************************/
/**
 * @brief 红黑树释放节点对象缓存的回调函数。
 *
 * @param [in ] xiter_node : 待释放的节点对象缓存。
 * @param [in ] xnode_size : 节点对象缓存的大小。
 * @param [in ] xrbt_ctxt  : 回调的上下文标识。
 */
static xrbt_void_t xrbtree_node_memfree(
                            x_rbnode_iter xiter_node,
                            xrbt_size_t xnode_size,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(xnode_size <= XCHUNK_RBNODE_SIZE);

    xchunk_handle_t xchunk_ptr = xrbtree_iter_chunk(xiter_node);
    xmpool_handle_t xmpool_ptr = (xmpool_handle_t)xrbt_ctxt;

    xmpool_ptr->xsize_valid -=
        (xchunk_ptr->xchunk_size - xchunk_ptr->xslice_queue.xut_offset);

    xmpool_ptr->xsize_cached -= xchunk_ptr->xchunk_size;
    xmpool_ptr->xfunc_free(xchunk_ptr,
                           xchunk_ptr->xchunk_size,
                           (x_handle_t)xmpool_ptr,
                           xmpool_ptr->xht_context);
}

/**********************************************************/
/**
 * @brief 红黑树拷贝节点对象的索引键值的回调函数。
 *
 * @param [out] xrbt_dkey : 目标的索引键缓存。
 * @param [in ] xrbt_skey : 源数据的索引键缓存。
 * @param [in ] xrbt_size : 索引键缓存大小。
 * @param [in ] xbt_move  : 是否采用右值 move 操作进行数据拷贝。
 * @param [in ] xrbt_ctxt : 回调的上下文标识。
 */
static xrbt_void_t xrbtree_chunk_copyfrom(
                            xrbt_vkey_t xrbt_dkey,
                            xrbt_vkey_t xrbt_skey,
                            xrbt_size_t xrbt_size,
                            xrbt_bool_t xbt_move ,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(sizeof(xchunk_handle_t) == xrbt_size);
    *(xchunk_handle_t *)xrbt_dkey = *(xchunk_handle_t *)xrbt_skey;
}

/**********************************************************/
/**
 * @brief 红黑树析构节点对象的索引键值的回调函数。
 *
 * @param [out] xrbt_vkey : 索引键缓存。
 * @param [in ] xrbt_size : 索引键缓存大小。
 * @param [in ] xrbt_ctxt : 回调的上下文标识。
 */
static xrbt_void_t xrbtree_chunk_destruct(
                            xrbt_vkey_t xrbt_vkey,
                            xrbt_size_t xrbt_size,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(sizeof(xchunk_handle_t) == xrbt_size);

    xchunk_handle_t xchunk_ptr = *(xchunk_handle_t *)xrbt_vkey;

    XASSERT(XCHUNK_SLICE_QFULL(xchunk_ptr));

    // 分片容量大于 0 的情况下，chunk 才会进行分类管理
    if (XCHUNK_SLICE_QSIZE(xchunk_ptr) > 0)
    {
        XASSERT(X_NULL != xchunk_ptr->xowner.xclass_ptr);
        xclass_list_erase_chunk(xchunk_ptr->xowner.xclass_ptr, xchunk_ptr);
        xchunk_ptr->xowner.xclass_ptr = X_NULL;
    }
}

/**********************************************************/
/**
 * @brief 比较 chunk 节点索引键值的回调函数。
 * 
 * @param [in ] xrbt_lkey : 比较操作的左值。
 * @param [in ] xrbt_rkey : 比较操作的右值。
 * @param [in ] xrbt_size : xrbt_lkey（or xrbt_rkey） 缓存大小。
 * @param [in ] xrbt_ctxt : 回调的上下文标识。
 * 
 * @return xrbt_bool_t
 *         - 若 xrbt_lkey < xrbt_rkey ，返回 XRBT_TRUE；
 *         - 否则 返回 XRBT_FALSE。
 */
static xrbt_bool_t xrbtree_chunk_compare(
                            xrbt_vkey_t xrbt_lkey,
                            xrbt_vkey_t xrbt_rkey,
                            xrbt_size_t xrbt_size,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(sizeof(xchunk_handle_t) == xrbt_size);

#define XCHUNK_CAST(xrbt_vkey) (*(xchunk_handle_t *)(xrbt_vkey))
    XASSERT(XCHUNK_CAST(xrbt_lkey)->xchunk_size > 0);
    return (XCHUNK_RADDR(XCHUNK_CAST(xrbt_lkey)) <=
            XCHUNK_LADDR(XCHUNK_CAST(xrbt_rkey)));
#undef XCHUNK_CAST
}

//====================================================================

// 
// xmem_pool_t : internal calls
// 

/**********************************************************/
/**
 * @brief 初始化 内存分类对象 表。
 */
static x_void_t xmpool_class_initialize(xmpool_handle_t xmpool_ptr)
{
    xclass_handle_t xclass_ptr = X_NULL;

    x_int32_t  xit_iter   = 0;
    x_uint32_t xut_unused = 0;
    x_uint32_t xut_minusd = XCHUNK_MAX_SIZE;
    x_uint32_t xut_expect = 0;

    for (xit_iter = 0; xit_iter < XSLICE_TYPE_COUNT; ++xit_iter)
    {
        //======================================

        xclass_ptr = &xmpool_ptr->xclass_ptr[xit_iter];

        xclass_ptr->xchunk_size  = XCHUNK_MIN_SIZE;
        xclass_ptr->xslice_size  = X_slice_size_table[xit_iter];
        xclass_ptr->xchunk_count = 0;
        xclass_ptr->xslice_count = 0;
        xclass_ptr->xmpool_ptr   = xmpool_ptr;
        xclass_ptr->xlist_head.xchunk_size = sizeof(xchunk_alias_t);
        xclass_ptr->xlist_head.xslice_size = 0;
        xclass_ptr->xlist_head.xlist_node.xchunk_prev = X_NULL;
        xclass_ptr->xlist_head.xlist_node.xchunk_next = XCLASS_LIST_TAIL(xclass_ptr);
        xclass_ptr->xlist_tail.xchunk_size = sizeof(xchunk_alias_t);
        xclass_ptr->xlist_tail.xslice_size = 0;
        xclass_ptr->xlist_tail.xlist_node.xchunk_prev = XCLASS_LIST_HEAD(xclass_ptr);
        xclass_ptr->xlist_tail.xlist_node.xchunk_next = X_NULL;

        //======================================
        // 计算最优的内存分片方案（尽可能的利用 chunk 对象的缓存）

        xut_minusd = xmem_chunk_unused_size(
                            XCHUNK_MIN_SIZE, xclass_ptr->xslice_size);

        for (xut_expect  = XCHUNK_MIN_SIZE;
             xut_expect <= XCHUNK_MAX_SIZE;
             xut_expect += XCHUNK_INC_SIZE)
        {
            // 因分片的 索引号（只有 16 位）的最高位用于
            // 标识“分片是否已被分配出去”，
            // 所以 chunk 对象的容量上限也就为 0x00007FFF
            if (xmem_chunk_capacity(
                    xut_expect, xclass_ptr->xslice_size) > 0x00007FFF)
            {
                break;
            }

            xut_unused = xmem_chunk_unused_size(
                                xut_expect, xclass_ptr->xslice_size);
            if (xut_unused < xut_minusd)
            {
                xut_minusd = xut_unused;
                xclass_ptr->xchunk_size = xut_expect;
            }
        }

        //======================================
    }
}

/**********************************************************/
/**
 * @brief 释放 内存分类对象 表。
 */
static x_void_t xmpool_class_release(xmpool_handle_t xmpool_ptr)
{
    x_int32_t       xit_iter   = 0;
    xclass_handle_t xclass_ptr = X_NULL;

    for (xit_iter = 0; xit_iter < XSLICE_TYPE_COUNT; ++xit_iter)
    {
        xclass_ptr = &xmpool_ptr->xclass_ptr[xit_iter];

        XASSERT((0 == xclass_ptr->xchunk_count) &&
                (0 == xclass_ptr->xslice_count));

        xmem_clear(xclass_ptr, sizeof(xmem_class_t));
    }
}

/**********************************************************/
/**
 * @brief 按照给定的 分片大小，获取对应的 class 分类对象。
 */
static xclass_handle_t xmpool_get_class(
                            xmpool_handle_t xmpool_ptr,
                            x_uint32_t xslice_size)
{
    XASSERT(X_NULL != xmpool_ptr);
    XASSERT((xslice_size > 0) && (xmem_align_size(xslice_size) == xslice_size));

    xclass_handle_t xclass_ptr = X_NULL;

    switch (xslice_size)
    {
    case XSLICE_SIZE_____8 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 0]; break;
    case XSLICE_SIZE____16 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 1]; break;
    case XSLICE_SIZE____24 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 2]; break;
    case XSLICE_SIZE____32 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 3]; break;
    case XSLICE_SIZE____40 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 4]; break;
    case XSLICE_SIZE____48 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 5]; break;
    case XSLICE_SIZE____56 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 6]; break;
    case XSLICE_SIZE____64 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 7]; break;
    case XSLICE_SIZE____72 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 8]; break;
    case XSLICE_SIZE____80 : xclass_ptr = &xmpool_ptr->xclass_ptr[ 9]; break;
    case XSLICE_SIZE____88 : xclass_ptr = &xmpool_ptr->xclass_ptr[10]; break;
    case XSLICE_SIZE____96 : xclass_ptr = &xmpool_ptr->xclass_ptr[11]; break;
    case XSLICE_SIZE___104 : xclass_ptr = &xmpool_ptr->xclass_ptr[12]; break;
    case XSLICE_SIZE___112 : xclass_ptr = &xmpool_ptr->xclass_ptr[13]; break;
    case XSLICE_SIZE___120 : xclass_ptr = &xmpool_ptr->xclass_ptr[14]; break;
    case XSLICE_SIZE___128 : xclass_ptr = &xmpool_ptr->xclass_ptr[15]; break;
    case XSLICE_SIZE___144 : xclass_ptr = &xmpool_ptr->xclass_ptr[16]; break;
    case XSLICE_SIZE___160 : xclass_ptr = &xmpool_ptr->xclass_ptr[17]; break;
    case XSLICE_SIZE___176 : xclass_ptr = &xmpool_ptr->xclass_ptr[18]; break;
    case XSLICE_SIZE___192 : xclass_ptr = &xmpool_ptr->xclass_ptr[19]; break;
    case XSLICE_SIZE___208 : xclass_ptr = &xmpool_ptr->xclass_ptr[20]; break;
    case XSLICE_SIZE___224 : xclass_ptr = &xmpool_ptr->xclass_ptr[21]; break;
    case XSLICE_SIZE___240 : xclass_ptr = &xmpool_ptr->xclass_ptr[22]; break;
    case XSLICE_SIZE___256 : xclass_ptr = &xmpool_ptr->xclass_ptr[23]; break;
    case XSLICE_SIZE___288 : xclass_ptr = &xmpool_ptr->xclass_ptr[24]; break;
    case XSLICE_SIZE___320 : xclass_ptr = &xmpool_ptr->xclass_ptr[25]; break;
    case XSLICE_SIZE___352 : xclass_ptr = &xmpool_ptr->xclass_ptr[26]; break;
    case XSLICE_SIZE___384 : xclass_ptr = &xmpool_ptr->xclass_ptr[27]; break;
    case XSLICE_SIZE___416 : xclass_ptr = &xmpool_ptr->xclass_ptr[28]; break;
    case XSLICE_SIZE___448 : xclass_ptr = &xmpool_ptr->xclass_ptr[29]; break;
    case XSLICE_SIZE___480 : xclass_ptr = &xmpool_ptr->xclass_ptr[30]; break;
    case XSLICE_SIZE___512 : xclass_ptr = &xmpool_ptr->xclass_ptr[31]; break;
    case XSLICE_SIZE___576 : xclass_ptr = &xmpool_ptr->xclass_ptr[32]; break;
    case XSLICE_SIZE___640 : xclass_ptr = &xmpool_ptr->xclass_ptr[33]; break;
    case XSLICE_SIZE___704 : xclass_ptr = &xmpool_ptr->xclass_ptr[34]; break;
    case XSLICE_SIZE___768 : xclass_ptr = &xmpool_ptr->xclass_ptr[35]; break;
    case XSLICE_SIZE___832 : xclass_ptr = &xmpool_ptr->xclass_ptr[36]; break;
    case XSLICE_SIZE___896 : xclass_ptr = &xmpool_ptr->xclass_ptr[37]; break;
    case XSLICE_SIZE___960 : xclass_ptr = &xmpool_ptr->xclass_ptr[38]; break;
    case XSLICE_SIZE__1024 : xclass_ptr = &xmpool_ptr->xclass_ptr[39]; break;
    case XSLICE_SIZE__1152 : xclass_ptr = &xmpool_ptr->xclass_ptr[40]; break;
    case XSLICE_SIZE__1280 : xclass_ptr = &xmpool_ptr->xclass_ptr[41]; break;
    case XSLICE_SIZE__1408 : xclass_ptr = &xmpool_ptr->xclass_ptr[42]; break;
    case XSLICE_SIZE__1536 : xclass_ptr = &xmpool_ptr->xclass_ptr[43]; break;
    case XSLICE_SIZE__1664 : xclass_ptr = &xmpool_ptr->xclass_ptr[44]; break;
    case XSLICE_SIZE__1792 : xclass_ptr = &xmpool_ptr->xclass_ptr[45]; break;
    case XSLICE_SIZE__1920 : xclass_ptr = &xmpool_ptr->xclass_ptr[46]; break;
    case XSLICE_SIZE__2048 : xclass_ptr = &xmpool_ptr->xclass_ptr[47]; break;
    case XSLICE_SIZE__2304 : xclass_ptr = &xmpool_ptr->xclass_ptr[48]; break;
    case XSLICE_SIZE__2560 : xclass_ptr = &xmpool_ptr->xclass_ptr[49]; break;
    case XSLICE_SIZE__2816 : xclass_ptr = &xmpool_ptr->xclass_ptr[50]; break;
    case XSLICE_SIZE__3072 : xclass_ptr = &xmpool_ptr->xclass_ptr[51]; break;
    case XSLICE_SIZE__3328 : xclass_ptr = &xmpool_ptr->xclass_ptr[52]; break;
    case XSLICE_SIZE__3584 : xclass_ptr = &xmpool_ptr->xclass_ptr[53]; break;
    case XSLICE_SIZE__3840 : xclass_ptr = &xmpool_ptr->xclass_ptr[54]; break;
    case XSLICE_SIZE__4096 : xclass_ptr = &xmpool_ptr->xclass_ptr[55]; break;
    case XSLICE_SIZE__4608 : xclass_ptr = &xmpool_ptr->xclass_ptr[56]; break;
    case XSLICE_SIZE__5120 : xclass_ptr = &xmpool_ptr->xclass_ptr[57]; break;
    case XSLICE_SIZE__5632 : xclass_ptr = &xmpool_ptr->xclass_ptr[58]; break;
    case XSLICE_SIZE__6144 : xclass_ptr = &xmpool_ptr->xclass_ptr[59]; break;
    case XSLICE_SIZE__6656 : xclass_ptr = &xmpool_ptr->xclass_ptr[60]; break;
    case XSLICE_SIZE__7168 : xclass_ptr = &xmpool_ptr->xclass_ptr[61]; break;
    case XSLICE_SIZE__7680 : xclass_ptr = &xmpool_ptr->xclass_ptr[62]; break;
    case XSLICE_SIZE__8192 : xclass_ptr = &xmpool_ptr->xclass_ptr[63]; break;
    case XSLICE_SIZE__9216 : xclass_ptr = &xmpool_ptr->xclass_ptr[64]; break;
    case XSLICE_SIZE_10240 : xclass_ptr = &xmpool_ptr->xclass_ptr[65]; break;
    case XSLICE_SIZE_11264 : xclass_ptr = &xmpool_ptr->xclass_ptr[66]; break;
    case XSLICE_SIZE_12288 : xclass_ptr = &xmpool_ptr->xclass_ptr[67]; break;
    case XSLICE_SIZE_13312 : xclass_ptr = &xmpool_ptr->xclass_ptr[68]; break;
    case XSLICE_SIZE_14336 : xclass_ptr = &xmpool_ptr->xclass_ptr[69]; break;
    case XSLICE_SIZE_15360 : xclass_ptr = &xmpool_ptr->xclass_ptr[70]; break;
    case XSLICE_SIZE_16384 : xclass_ptr = &xmpool_ptr->xclass_ptr[71]; break;
    case XSLICE_SIZE_18432 : xclass_ptr = &xmpool_ptr->xclass_ptr[72]; break;
    case XSLICE_SIZE_20480 : xclass_ptr = &xmpool_ptr->xclass_ptr[73]; break;
    case XSLICE_SIZE_22528 : xclass_ptr = &xmpool_ptr->xclass_ptr[74]; break;
    case XSLICE_SIZE_24576 : xclass_ptr = &xmpool_ptr->xclass_ptr[75]; break;
    case XSLICE_SIZE_26624 : xclass_ptr = &xmpool_ptr->xclass_ptr[76]; break;
    case XSLICE_SIZE_28672 : xclass_ptr = &xmpool_ptr->xclass_ptr[77]; break;
    case XSLICE_SIZE_30720 : xclass_ptr = &xmpool_ptr->xclass_ptr[78]; break;
    case XSLICE_SIZE_32768 : xclass_ptr = &xmpool_ptr->xclass_ptr[79]; break;
    case XSLICE_SIZE_36864 : xclass_ptr = &xmpool_ptr->xclass_ptr[80]; break;
    case XSLICE_SIZE_40960 : xclass_ptr = &xmpool_ptr->xclass_ptr[81]; break;
    case XSLICE_SIZE_45056 : xclass_ptr = &xmpool_ptr->xclass_ptr[82]; break;
    case XSLICE_SIZE_49152 : xclass_ptr = &xmpool_ptr->xclass_ptr[83]; break;
    case XSLICE_SIZE_53248 : xclass_ptr = &xmpool_ptr->xclass_ptr[84]; break;
    case XSLICE_SIZE_57344 : xclass_ptr = &xmpool_ptr->xclass_ptr[85]; break;
    case XSLICE_SIZE_61440 : xclass_ptr = &xmpool_ptr->xclass_ptr[86]; break;
    case XSLICE_SIZE_65536 : xclass_ptr = &xmpool_ptr->xclass_ptr[87]; break;

    default: break;
    }

    XASSERT_CHECK(X_NULL != xclass_ptr, xslice_size == xclass_ptr->xslice_size);

    return xclass_ptr;
}

/**********************************************************/
/**
 * @brief 申请新的 chunk 对象。
 * 
 * @param [in ] xmpool_ptr  : 内存池对象。
 * @param [in ] xchunk_size : chunk 对象大小。
 * @param [in ] xslice_size : 分片大小。
 * 
 * @return xchunk_handle_t
 *         - 成功，返回 chunk 对象；
 *         - 失败，返回 X_NULL。
 */
static xchunk_handle_t xmpool_alloc_chunk(
                            xmpool_handle_t xmpool_ptr,
                            x_uint32_t xchunk_size,
                            x_uint32_t xslice_size)
{
    XASSERT(X_NULL != xmpool_ptr);
    XASSERT((xslice_size > 0) &&
            (xchunk_size >= (xslice_size + sizeof(xmem_chunk_t))));

    x_uint16_t xut_iter = 0;

    xchunk_handle_t xchunk_ptr =
        (xchunk_handle_t)xmpool_ptr->xfunc_alloc(
                            xchunk_size,
                            (x_handle_t)xmpool_ptr,
                            xmpool_ptr->xht_context);
    XASSERT(X_NULL != xchunk_ptr);
    xmpool_ptr->xsize_cached += xchunk_size;

    xmem_clear(xchunk_ptr, sizeof(xmem_chunk_t));

    if (xslice_size > XSLICE_SIZE_65536)
    {
        xchunk_ptr->xchunk_size = xchunk_size;
        xchunk_ptr->xslice_size = xchunk_size - sizeof(xmem_chunk_t);
        xchunk_ptr->xowner.xmpool_ptr = xmpool_ptr;
        xchunk_ptr->xslice_queue.xut_offset   = sizeof(xmem_chunk_t);
        xchunk_ptr->xslice_queue.xut_capacity = 0;
    }
    else
    {
        xchunk_ptr->xchunk_size = xchunk_size;
        xchunk_ptr->xslice_size = xslice_size;
        xchunk_ptr->xowner.xmpool_ptr = X_NULL;

        xchunk_ptr->xslice_queue.xut_capacity =
                xmem_chunk_capacity(xchunk_size, xslice_size);
        XASSERT(xchunk_ptr->xslice_queue.xut_capacity <= 0x7FFF);

        xchunk_ptr->xslice_queue.xut_offset =
                (xchunk_size - 
                 xslice_size * xchunk_ptr->xslice_queue.xut_capacity);

        for (xut_iter = 0;
             xut_iter < xchunk_ptr->xslice_queue.xut_capacity;
             ++xut_iter)
        {
            // 最高位为 0 值，表示分片未被分配出去
            XCHUNK_SLICE_INDEX(xchunk_ptr, xut_iter) = xut_iter & 0x7FFF;
        }
        xchunk_ptr->xslice_queue.xut_bpos = 0;
        xchunk_ptr->xslice_queue.xut_epos =
                            xchunk_ptr->xslice_queue.xut_capacity;
    }

    xmpool_ptr->xsize_valid +=
        (xchunk_ptr->xchunk_size - xchunk_ptr->xslice_queue.xut_offset);

    if (!xrbtree_insert_chunk(XMPOOL_RBTREE(xmpool_ptr), xchunk_ptr))
    {
        XASSERT(X_FALSE);

        xmpool_ptr->xsize_valid -=
            (xchunk_ptr->xchunk_size - xchunk_ptr->xslice_queue.xut_offset);

        xmpool_ptr->xsize_cached -= xchunk_ptr->xchunk_size;
        xmpool_ptr->xfunc_free(xchunk_ptr,
                               xchunk_ptr->xchunk_size,
                               (x_handle_t)xmpool_ptr,
                               xmpool_ptr->xht_context);
        xchunk_ptr = X_NULL;
    }

    return xchunk_ptr;
}

/**********************************************************/
/**
 * @brief 释放 chunk 对象。
 */
static inline x_bool_t xmpool_dealloc_chunk(
                                    xmpool_handle_t xmpool_ptr,
                                    xchunk_handle_t xchunk_ptr)
{
    return xrbtree_erase_chunk(XMPOOL_RBTREE(xmpool_ptr), xchunk_ptr);
}

//====================================================================

// 
// xmem_pool_t : public interfaces
// 

/**********************************************************/
/**
 * @brief 创建 内存池对象。
 * 
 * @param [in ] xfunc_alloc : 申请堆内存块的接口。
 * @param [in ] xfunc_free  : 释放堆内存块的接口。
 * @param [in ] xht_context : 调用 xfunc_alloc/xfunc_free 时回调的上下文句柄。
 * 
 * @return xmpool_handle_t
 *         - 成功，返回 内存池对象 的操作句柄。
 *         - 失败，返回 X_NULL。
 */
xmpool_handle_t xmpool_create(xfunc_alloc_t xfunc_alloc,
                              xfunc_free_t xfunc_free,
                              x_handle_t xht_context)
{
    xrbt_callback_t xcallback =
    {
        /* .xfunc_n_memalloc = */ &xrbtree_node_memalloc ,
        /* .xfunc_n_memfree  = */ &xrbtree_node_memfree  ,
        /* .xfunc_k_copyfrom = */ &xrbtree_chunk_copyfrom,
        /* .xfunc_k_destruct = */ &xrbtree_chunk_destruct,
        /* .xfunc_k_lesscomp = */ &xrbtree_chunk_compare ,
        /* .xctxt_t_callback = */ XRBT_NULL
    };

    xmpool_handle_t xmpool_ptr =
        (xmpool_handle_t)xmem_heap_alloc(sizeof(xmem_pool_t), X_NULL, X_NULL);

    XASSERT(X_NULL != xmpool_ptr);

    xmpool_ptr->xfunc_alloc = (X_NULL != xfunc_alloc) ? xfunc_alloc : &xmem_heap_alloc;
    xmpool_ptr->xfunc_free  = (X_NULL != xfunc_free ) ? xfunc_free  : &xmem_heap_free ;
    xmpool_ptr->xht_context = xht_context;

    xmpool_ptr->xsize_cached = 0;
    xmpool_ptr->xsize_valid  = 0;
    xmpool_ptr->xsize_using  = 0;

    xmpool_ptr->xut_worktid   = xsys_tid();
    xmpool_ptr->xspinlock_que = 0;
    xsrque_init(&xmpool_ptr->xslice_rqueue);

    xcallback.xctxt_t_callback = xmpool_ptr;
    XASSERT(XMPOOL_RBTREE_SIZE >= xrbtree_sizeof());
    xrbtree_emplace_create(XMPOOL_RBTREE(xmpool_ptr),
                           sizeof(xchunk_handle_t),
                           &xcallback);

    xmpool_ptr->xchunk_cptr = X_NULL;

    xmpool_class_initialize(xmpool_ptr);

    return xmpool_ptr;
}

/**********************************************************/
/**
 * @brief 销毁内存池对象。
 */
x_void_t xmpool_destroy(xmpool_handle_t xmpool_ptr)
{
    XASSERT(X_NULL != xmpool_ptr);
    XASSERT(0 == xmpool_ptr->xsize_using);

    xmpool_ptr->xut_worktid   = 0;
    xmpool_ptr->xspinlock_que = 0;
    xsrque_release(&xmpool_ptr->xslice_rqueue);

    xrbtree_emplace_destroy(XMPOOL_RBTREE(xmpool_ptr));
    xmpool_class_release(xmpool_ptr);

    xmpool_ptr->xfunc_alloc  = X_NULL;
    xmpool_ptr->xfunc_free   = X_NULL;
    xmpool_ptr->xht_context  = X_NULL;
    xmpool_ptr->xsize_cached = 0;
    xmpool_ptr->xsize_valid  = 0;
    xmpool_ptr->xsize_using  = 0;
    xmpool_ptr->xchunk_cptr  = X_NULL;

    xmem_heap_free(xmpool_ptr, sizeof(xmem_pool_t), X_NULL, X_NULL);
}

/**********************************************************/
/**
 * @brief 内存池对象 所隶属的工作线程 ID。
 */
x_uint32_t xmpool_worktid(xmpool_handle_t xmpool_ptr)
{
    XASSERT(X_NULL != xmpool_ptr);
    return xmpool_ptr->xut_worktid;
}

/**********************************************************/
/**
 * @brief 设置 内存池对象 所隶属的工作线程 ID。
 */
x_void_t xmpool_set_worktid(xmpool_handle_t xmpool_ptr, x_uint32_t xut_worktid)
{
    XASSERT(X_NULL != xmpool_ptr);
    xmpool_ptr->xut_worktid = xut_worktid;
}

/**********************************************************/
/**
 * @brief 内存池对象 总共缓存的内存大小。
 */
x_uint64_t xmpool_cached_size(xmpool_handle_t xmpool_ptr)
{
    XASSERT(X_NULL != xmpool_ptr);
    return xmpool_ptr->xsize_cached;
}

/**********************************************************/
/**
 * @brief 内存池对象 可使用到的缓存大小。
 */
x_uint64_t xmpool_valid_size(xmpool_handle_t xmpool_ptr)
{
    XASSERT(X_NULL != xmpool_ptr);
    return xmpool_ptr->xsize_valid;
}

/**********************************************************/
/**
 * @brief 内存池对象 正在使用的缓存大小。
 */
x_uint64_t xmpool_using_size(xmpool_handle_t xmpool_ptr)
{
    XASSERT(X_NULL != xmpool_ptr);
    return xmpool_ptr->xsize_using;
}

/**********************************************************/
/**
 * @brief 申请内存分片。
 * 
 * @param [in ] xmpool_ptr : 内存池对象的操作句柄。
 * @param [in ] xut_size   : 申请的内存分片大小。
 * 
 * @return xmem_slice_t
 *         - 成功，返回 内存分片；
 *         - 失败，返回 X_NULL 。
 */
xmem_slice_t xmpool_alloc(xmpool_handle_t xmpool_ptr, x_uint32_t xut_size)
{
    XASSERT(X_NULL != xmpool_ptr);

    xmem_slice_t    xmem_slice = X_NULL;
    xchunk_handle_t xchunk_ptr = X_NULL;
    xclass_handle_t xclass_ptr = X_NULL;

    //======================================

    if (xut_size <= 0)
        return X_NULL;
    xut_size = xmem_align_size(xut_size);

    //======================================

    if (xut_size > XSLICE_SIZE_65536)
    {
        xchunk_ptr = xmpool_alloc_chunk(
                            xmpool_ptr,
                            xut_size,
                            xut_size - sizeof(xmem_chunk_t));
        if (X_NULL != xchunk_ptr)
        {
            xmem_slice = XCHUNK_SLICE_BEGIN(xchunk_ptr);
        }
    }
    else
    {
        if ((X_NULL != xmpool_ptr->xchunk_cptr) &&
            (xut_size == xmpool_ptr->xchunk_cptr->xslice_size) &&
            !XCHUNK_SLICE_EMPTY(xmpool_ptr->xchunk_cptr))
        {
            xchunk_ptr = xmpool_ptr->xchunk_cptr;
        }
        else
        {
            xclass_ptr = xmpool_get_class(xmpool_ptr, xut_size);
            XASSERT(X_NULL != xclass_ptr);

            xchunk_ptr = xclass_get_non_empty_chunk(xclass_ptr);
        }

        if (X_NULL == xchunk_ptr)
        {
            xchunk_ptr = xmpool_alloc_chunk(
                                xmpool_ptr,
                                xclass_ptr->xchunk_size,
                                xclass_ptr->xslice_size);
            if (X_NULL != xchunk_ptr)
            {
                xchunk_ptr->xowner.xclass_ptr = xclass_ptr;
                xclass_list_push_head(xclass_ptr, xchunk_ptr);
                xmem_slice = xchunk_alloc_slice(xchunk_ptr);
            }
        }
        else
        {
            xmem_slice = xchunk_alloc_slice(xchunk_ptr);
            xclass_list_update(xchunk_ptr->xowner.xclass_ptr, xchunk_ptr);
        }
    }

    //======================================

    if ((X_NULL != xmem_slice) && (X_NULL != xchunk_ptr))
    {
        xmpool_ptr->xsize_using += xchunk_ptr->xslice_size;
    }

    xmpool_ptr->xchunk_cptr = xchunk_ptr;

    //======================================

    return xmem_slice;
}

/**********************************************************/
/**
 * @brief 回收内存分片。
 * 
 * @param [in ] xmpool_ptr : 内存池对象的操作句柄。
 * @param [in ] xmem_slice : 待回收的内存分片。
 * 
 * @return x_int32_t
 *         - 成功，返回 XMEM_ERR_OK；
 *         - 失败，返回 错误码（擦看 @see xmem_err_code 枚举值）。
 */
x_int32_t xmpool_recyc(xmpool_handle_t xmpool_ptr, xmem_slice_t xmem_slice)
{
    XASSERT(X_NULL != xmpool_ptr);
    XASSERT(X_NULL != xmem_slice);

    x_int32_t xit_error = XMEM_ERR_UNKNOW;

    //======================================

    xchunk_handle_t xchunk_ptr = X_NULL;

    if ((X_NULL != xmpool_ptr->xchunk_cptr) &&
        (xmem_slice > XCHUNK_LADDR(xmpool_ptr->xchunk_cptr)) &&
        (xmem_slice < XCHUNK_RADDR(xmpool_ptr->xchunk_cptr)))
    {
        xchunk_ptr = xmpool_ptr->xchunk_cptr;
    }
    else
    {
        xchunk_ptr = xrbtree_hit_chunk(XMPOOL_RBTREE(xmpool_ptr), xmem_slice);
    }

    if (X_NULL == xchunk_ptr)
    {
        return XMEM_ERR_SLICE_NOT_FOUND;
    }

    //======================================
    // 回收 slice

    // 若 chunk 对象没有多个分片，则不属于分类管理的 chunk 对象，可直接删除
    if (XCHUNK_SLICE_QSIZE(xchunk_ptr) == 0)
    {
        if (xmem_slice != XCHUNK_SLICE_BEGIN(xchunk_ptr))
        {
            return XMEM_ERR_SLICE_UNALIGNED;
        }

        if (xchunk_ptr == xmpool_ptr->xchunk_cptr)
        {
            xmpool_ptr->xchunk_cptr = X_NULL;
        }

        xmpool_ptr->xsize_using -= xchunk_ptr->xslice_size;

        if (!xmpool_dealloc_chunk(xmpool_ptr, xchunk_ptr))
        {
            XASSERT(X_FALSE);
        }

        return XMEM_ERR_OK;
    }

    // chunk 对象属于分类管理的 chunk 类型，需要进行 chunk 分片回收操作
    xit_error = xchunk_recyc_slice(xchunk_ptr, xmem_slice);
    if (XMEM_ERR_OK == xit_error)
    {
        xmpool_ptr->xsize_using -= xchunk_ptr->xslice_size;
        xclass_list_update(xchunk_ptr->xowner.xclass_ptr, xchunk_ptr);
    }

    xmpool_ptr->xchunk_cptr = xchunk_ptr;

    //======================================

    return xit_error;
}

/**********************************************************/
/**
 * @brief 释放内存池中未使用的缓存块。
 */
x_void_t xmpool_release_unused(xmpool_handle_t xmpool_ptr)
{
    XASSERT(X_NULL != xmpool_ptr);

    xclass_handle_t xclass_ptr = X_NULL;
    xchunk_handle_t xchunk_ptr = X_NULL;
    xchunk_handle_t xchunk_tmp = X_NULL;

    x_int32_t xit_iter = 0;

    for (xit_iter = 0; xit_iter < XSLICE_TYPE_COUNT; ++xit_iter)
    {
        xclass_ptr = &xmpool_ptr->xclass_ptr[xit_iter];

        for (xchunk_ptr = XCLASS_LIST_FRONT(xclass_ptr);
             xchunk_ptr != XCLASS_LIST_TAIL(xclass_ptr);)
        {
            if (XCHUNK_SLICE_QFULL(xchunk_ptr))
            {
                if (xmpool_ptr->xchunk_cptr == xchunk_ptr)
                {
                    xmpool_ptr->xchunk_cptr = X_NULL;
                }

                xchunk_tmp = xchunk_ptr->xlist_node.xchunk_next;
                xmpool_dealloc_chunk(xmpool_ptr, xchunk_ptr);
                xchunk_ptr = xchunk_tmp;
            }
            else
            {
                xchunk_ptr = xchunk_ptr->xlist_node.xchunk_next;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

////////////////////////////////////////////////////////////////////////////////
