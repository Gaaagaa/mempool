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
#define XASSERT_CHECK(xptr)
#endif // ENABLE_XASSERT

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNU_C__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif // __GNU_C__

////////////////////////////////////////////////////////////////////////////////

struct xmem_chunk_t;
struct xmem_class_t;

typedef struct xmem_chunk_t * xchunk_handle_t;
typedef struct xmem_class_t * xclass_handle_t;

#define XMEM_RBNODE_SIZE    (5 * sizeof(x_handle_t))
#define XMEM_PAGE_SIZE      (1024 * 4)

#define XSLICE_SIZE____32      32 // the  increment is 32
#define XSLICE_SIZE____64      64
#define XSLICE_SIZE____96      96
#define XSLICE_SIZE___128     128 // next increment is 64
#define XSLICE_SIZE___192     192
#define XSLICE_SIZE___256     256
#define XSLICE_SIZE___320     320
#define XSLICE_SIZE___384     384
#define XSLICE_SIZE___448     448
#define XSLICE_SIZE___512     512 // next increment is 128
#define XSLICE_SIZE___640     640
#define XSLICE_SIZE___768     768
#define XSLICE_SIZE___896     896
#define XSLICE_SIZE__1024    1024 // next increment is 256
#define XSLICE_SIZE__1280    1280
#define XSLICE_SIZE__1536    1536
#define XSLICE_SIZE__1792    1792
#define XSLICE_SIZE__2048    2048
#define XSLICE_SIZE__2304    2304
#define XSLICE_SIZE__2560    2560
#define XSLICE_SIZE__2816    2816
#define XSLICE_SIZE__3072    3072
#define XSLICE_SIZE__3328    3328
#define XSLICE_SIZE__3584    3584
#define XSLICE_SIZE__3840    3840
#define XSLICE_SIZE__4096    4096 // next increment is 512
#define XSLICE_SIZE__4608    4608
#define XSLICE_SIZE__5120    5120
#define XSLICE_SIZE__5632    5632
#define XSLICE_SIZE__6144    6144
#define XSLICE_SIZE__6656    6656
#define XSLICE_SIZE__7168    7168
#define XSLICE_SIZE__7680    7680
#define XSLICE_SIZE__8192    8192 // next increment is 1024
#define XSLICE_SIZE__9216    9216
#define XSLICE_SIZE_10240   10240
#define XSLICE_SIZE_11264   11264
#define XSLICE_SIZE_12288   12288
#define XSLICE_SIZE_13312   13312
#define XSLICE_SIZE_14336   14336
#define XSLICE_SIZE_15360   15360
#define XSLICE_SIZE_16384   16384 // next increment is 2048
#define XSLICE_SIZE_18432   18432
#define XSLICE_SIZE_20480   20480
#define XSLICE_SIZE_22528   22528
#define XSLICE_SIZE_24576   24576
#define XSLICE_SIZE_26624   26624
#define XSLICE_SIZE_28672   28672
#define XSLICE_SIZE_30720   30720
#define XSLICE_SIZE_32768   32768 // next increment is 4096
#define XSLICE_SIZE_36864   36864
#define XSLICE_SIZE_40960   40960
#define XSLICE_SIZE_45056   45056
#define XSLICE_SIZE_49152   49152
#define XSLICE_SIZE_53248   53248
#define XSLICE_SIZE_57344   57344
#define XSLICE_SIZE_61440   61440
#define XSLICE_SIZE_65536   65536

#define XSLICE_TYPE_AMOUNT  58

#define XCHUNK_MIN_SIZE     (1024 * 256 )
#define XCHUNK_MAX_SIZE     (1024 * 1028)
#define XCHUNK_INC_SIZE     XMEM_PAGE_SIZE

/** 所有内存分片大小的数组表 */
static x_uint32_t X_slice_size_table[XSLICE_TYPE_AMOUNT] =
{
    XSLICE_SIZE____32,
    XSLICE_SIZE____64,
    XSLICE_SIZE____96,
    XSLICE_SIZE___128,
    XSLICE_SIZE___192,
    XSLICE_SIZE___256,
    XSLICE_SIZE___320,
    XSLICE_SIZE___384,
    XSLICE_SIZE___448,
    XSLICE_SIZE___512,
    XSLICE_SIZE___640,
    XSLICE_SIZE___768,
    XSLICE_SIZE___896,
    XSLICE_SIZE__1024,
    XSLICE_SIZE__1280,
    XSLICE_SIZE__1536,
    XSLICE_SIZE__1792,
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
    x_byte_t xbt_ptr[XMEM_RBNODE_SIZE]; ///< 此字段仅起到内存占位的作用
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
     * @brief 可用的（未被分配出去的）内存分片索引号队列。
     * @note  此为一个环形队列。
     */
    struct
    {
    x_uint16_t      xut_offset;    ///< 内存分片组的首地址偏移量
    x_uint16_t      xut_capacity;  ///< 队列的节点数组容量
    x_uint16_t      xut_bpos;      ///< 队列的节点起始位置
    x_uint16_t      xut_epos;      ///< 队列的节点结束位置
    x_uint16_t      xut_index[0];  ///< 队列的节点缓存数组
    } xslice_queue;
} xmem_chunk_t;

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
    (XCHUNK_SLICE_COUNT((xchunk_ptr)) == XCHUNK_SLICE_QSIZE((xchunk_ptr)))

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

    x_rbtree_ptr    xrbtree_ptr;   ///< 存储管理所有 chunk 内存块对象的红黑树
    xmem_class_t    xclass_ptr[XSLICE_TYPE_AMOUNT]; ///< 各个内存分类
} xmem_pool_t;

////////////////////////////////////////////////////////////////////////////////

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
 * @param [in ] xht_context : 回调的上下文标识句柄。
 * 
 * @return x_void_t *
 *         - 成功，返回 堆内存块 地址；
 *         - 失败，返回 X_NULL。
 */
static x_void_t * xmem_heap_alloc(x_size_t xst_size,
                                  x_handle_t xht_context)
{
    return malloc(xst_size);
}

/**********************************************************/
/**
 * @brief 默认的 堆内存块 释放接口。
 * 
 * @param [in ] xmt_heap    : 释放的堆内存块。
 * @param [in ] xst_size    : 释放的堆内存块大小。
 * @param [in ] xht_context : 回调的上下文标识句柄。
 * 
 */
static x_void_t xmem_heap_free(x_void_t * xmt_heap,
                               x_size_t xst_size,
                               x_handle_t xht_context)
{
    if (X_NULL != xmt_heap)
        free(xmt_heap);
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
    static x_uint32_t xsize_align_32[32] =
    {
          32,   64,   96,  128,  192,  192,  256,  256,
         320,  320,  384,  384,  448,  448,  512,  512,
         640,  640,  640,  640,  768,  768,  768,  768,
         896,  896,  896,  896, 1024, 1024, 1024, 1024
    };

    static x_uint32_t xsize_align_256[28] =
    {
        1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072,
        3328, 3584, 3840, 4096, 4608, 4608, 5120, 5120,
        5632, 5632, 6144, 6144, 6656, 6656, 7168, 7168,
        7680, 7680, 8192, 8192
    };

    static x_uint32_t xsize_align_1024[24] =
    {
         9216, 10240, 11264, 12288, 13312, 14336, 15360, 16384,
        18432, 18432, 20480, 20480, 22528, 22528, 24576, 24576,
        26624, 26624, 28672, 28672, 30720, 30720, 32768, 32768
    };

    if (xut_size <= 0)
    {
        return 0;
    }

#define X_TABLE_INDEX(size, align) (((size) + (align) - 1) / (align) - 1)
    if (xut_size <= XSLICE_SIZE__8192)
    {
        if (xut_size <= XSLICE_SIZE__1024)
            return xsize_align_32[X_TABLE_INDEX(xut_size, XSLICE_SIZE____32)];
        return xsize_align_256[
            X_TABLE_INDEX(xut_size - XSLICE_SIZE__1024, XSLICE_SIZE___256)];
    }
    else if (xut_size <= XSLICE_SIZE_32768)
    {
        return xsize_align_1024[
            X_TABLE_INDEX(xut_size - XSLICE_SIZE__8192, XSLICE_SIZE__1024)];
    }
#undef X_TABLE_INDEX

    if (xut_size > XSLICE_SIZE_65536)
        xut_size += sizeof(xmem_chunk_t);

#define X_ALIGN(size, align) (((size) + ((align) - 1)) & (~((align) - 1)))
    return X_ALIGN(xut_size, XMEM_PAGE_SIZE);
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
        if (xut_qindex ==
                xchunk_ptr->xslice_queue.xut_index[
                    xut_iter % xchunk_ptr->xslice_queue.xut_capacity])
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
    x_uint16_t xut_qindex = 0;

    if (XCHUNK_SLICE_EMPTY(xchunk_ptr))
    {
        return X_NULL;
    }

    xut_qindex = xchunk_ptr->xslice_queue.xut_index[
                        xchunk_ptr->xslice_queue.xut_bpos %
                        xchunk_ptr->xslice_queue.xut_capacity];
    xchunk_ptr->xslice_queue.xut_bpos += 1;
    XASSERT(0 != xchunk_ptr->xslice_queue.xut_bpos);

    xchunk_ptr->xowner.xclass_ptr->xslice_count -= 1;

    return (((xmem_slice_t)xchunk_ptr) +
            xchunk_ptr->xslice_queue.xut_offset +
            (xut_qindex * xchunk_ptr->xslice_size));
}

/**********************************************************/
/**
 * @brief 回收内存分片至 chunk 对象中。
 * 
 * @param [in ] xchunk_ptr : chunk 对象。
 * @param [in ] xmem_slice : 待回收的内存分片。
 * 
 * @return x_bool_t
 *         - 成功，返回 X_TRUE；
 *         - 失败，返回 X_FALSE，分片在分块中的地址未对齐。
 */
static x_bool_t xchunk_recyc_slice(
                    xchunk_handle_t xchunk_ptr,
                    xmem_slice_t xmem_slice)
{
    XASSERT((xmem_slice > (xmem_slice_t)xchunk_ptr) &&
            (xmem_slice < ((xmem_slice_t)xchunk_ptr + xchunk_ptr->xchunk_size)));
    XASSERT(XCHUNK_SLICE_QSIZE(xchunk_ptr) > 0);
    XASSERT(!XCHUNK_SLICE_QFULL(xchunk_ptr));

    x_bool_t   xbt_recyc  = X_FALSE;
    x_uint32_t xut_offset = 0;
    x_uint32_t xut_qindex = 0;

    do
    {
        //======================================

        xut_offset = (x_uint32_t)(xmem_slice - (xmem_slice_t)xchunk_ptr);
        if ((xut_offset < sizeof(xmem_chunk_t)) ||
            (xut_offset < xchunk_ptr->xslice_queue.xut_offset))
        {
            break;
        }

        xut_offset -= xchunk_ptr->xslice_queue.xut_offset;
        if (0 != (xut_offset % xchunk_ptr->xslice_size))
        {
            break;
        }

        //======================================

        xut_qindex = xut_offset / xchunk_ptr->xslice_size;
        XASSERT(xut_qindex < XCHUNK_SLICE_QSIZE(xchunk_ptr));
        XASSERT(!xchunk_find_qindex(xchunk_ptr, (x_uint16_t)xut_qindex));

        xchunk_ptr->xslice_queue.xut_index[
            xchunk_ptr->xslice_queue.xut_epos %
            xchunk_ptr->xslice_queue.xut_capacity] = (x_uint16_t)xut_qindex;
        xchunk_ptr->xslice_queue.xut_epos += 1;

        if (0 == xchunk_ptr->xslice_queue.xut_epos)
        {
            xchunk_ptr->xslice_queue.xut_epos  = XCHUNK_SLICE_COUNT(xchunk_ptr);
            xchunk_ptr->xslice_queue.xut_bpos %= xchunk_ptr->xslice_queue.xut_capacity;
            xchunk_ptr->xslice_queue.xut_epos += xchunk_ptr->xslice_queue.xut_bpos;
        }

        xchunk_ptr->xowner.xclass_ptr->xslice_count += 1;

        //======================================

        xbt_recyc = X_TRUE;
    } while (0);

    return xbt_recyc;
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
        XASSERT(xmem_slice > (xmem_slice_t)xchunk_ptr);

        if (xmem_slice >= ((xmem_slice_t)xchunk_ptr + xchunk_ptr->xchunk_size))
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
    XASSERT(xst_nsize <= XMEM_RBNODE_SIZE);
    return (xrbt_void_t *)((*(xchunk_handle_t *)xrbt_vkey)->xtree_node.xbt_ptr);
}

/**********************************************************/
/**
 * @brief 红黑树申请节点对象缓存的回调函数。
 *
 * @param [in ] xiter_node : 待释放的节点对象缓存。
 * @param [in ] xnode_size : 节点对象缓存的大小。
 */
static xrbt_void_t xrbtree_node_memfree(
                            x_rbnode_iter xiter_node,
                            xrbt_size_t xnode_size,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(xnode_size <= XMEM_RBNODE_SIZE);

    xchunk_handle_t xchunk_ptr = xrbtree_iter_chunk(xiter_node);
    xmpool_handle_t xmpool_ptr = (xmpool_handle_t)xrbt_ctxt;

    xmpool_ptr->xsize_valid -=
        (xchunk_ptr->xchunk_size - xchunk_ptr->xslice_queue.xut_offset);

    xmpool_ptr->xsize_cached -= xchunk_ptr->xchunk_size;
    xmpool_ptr->xfunc_free(xchunk_ptr,
                           xchunk_ptr->xchunk_size,
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
 * @brief 默认的 比较节点索引键值的回调函数类型。
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

#define XCHUNK_MSIZE(xrbt_vkey) ((*(xchunk_handle_t *)(xrbt_vkey))->xchunk_size)
#define XCHUNK_LADDR(xrbt_vkey) ((x_byte_t *)(*(xchunk_handle_t *)(xrbt_vkey)))
#define XCHUNK_RADDR(xrbt_vkey) (XCHUNK_LADDR(xrbt_vkey) + XCHUNK_MSIZE(xrbt_vkey))

    XASSERT(XCHUNK_MSIZE(xrbt_lkey) > 0);

    return (XCHUNK_RADDR(xrbt_lkey) <= XCHUNK_LADDR(xrbt_rkey));

#undef XCHUNK_MSIZE
#undef XCHUNK_LADDR
#undef XCHUNK_RADDR
}

//====================================================================

// 
// xmem_pool_t : 内部调用的相关接口
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

    for (xit_iter = 0; xit_iter < XSLICE_TYPE_AMOUNT; ++xit_iter)
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

    for (xit_iter = 0; xit_iter < XSLICE_TYPE_AMOUNT; ++xit_iter)
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
    case XSLICE_SIZE____32: xclass_ptr = &xmpool_ptr->xclass_ptr[ 0]; break;
    case XSLICE_SIZE____64: xclass_ptr = &xmpool_ptr->xclass_ptr[ 1]; break;
    case XSLICE_SIZE____96: xclass_ptr = &xmpool_ptr->xclass_ptr[ 2]; break;
    case XSLICE_SIZE___128: xclass_ptr = &xmpool_ptr->xclass_ptr[ 3]; break;
    case XSLICE_SIZE___192: xclass_ptr = &xmpool_ptr->xclass_ptr[ 4]; break;
    case XSLICE_SIZE___256: xclass_ptr = &xmpool_ptr->xclass_ptr[ 5]; break;
    case XSLICE_SIZE___320: xclass_ptr = &xmpool_ptr->xclass_ptr[ 6]; break;
    case XSLICE_SIZE___384: xclass_ptr = &xmpool_ptr->xclass_ptr[ 7]; break;
    case XSLICE_SIZE___448: xclass_ptr = &xmpool_ptr->xclass_ptr[ 8]; break;
    case XSLICE_SIZE___512: xclass_ptr = &xmpool_ptr->xclass_ptr[ 9]; break;
    case XSLICE_SIZE___640: xclass_ptr = &xmpool_ptr->xclass_ptr[10]; break;
    case XSLICE_SIZE___768: xclass_ptr = &xmpool_ptr->xclass_ptr[11]; break;
    case XSLICE_SIZE___896: xclass_ptr = &xmpool_ptr->xclass_ptr[12]; break;
    case XSLICE_SIZE__1024: xclass_ptr = &xmpool_ptr->xclass_ptr[13]; break;
    case XSLICE_SIZE__1280: xclass_ptr = &xmpool_ptr->xclass_ptr[14]; break;
    case XSLICE_SIZE__1536: xclass_ptr = &xmpool_ptr->xclass_ptr[15]; break;
    case XSLICE_SIZE__1792: xclass_ptr = &xmpool_ptr->xclass_ptr[16]; break;
    case XSLICE_SIZE__2048: xclass_ptr = &xmpool_ptr->xclass_ptr[17]; break;
    case XSLICE_SIZE__2304: xclass_ptr = &xmpool_ptr->xclass_ptr[18]; break;
    case XSLICE_SIZE__2560: xclass_ptr = &xmpool_ptr->xclass_ptr[19]; break;
    case XSLICE_SIZE__2816: xclass_ptr = &xmpool_ptr->xclass_ptr[20]; break;
    case XSLICE_SIZE__3072: xclass_ptr = &xmpool_ptr->xclass_ptr[21]; break;
    case XSLICE_SIZE__3328: xclass_ptr = &xmpool_ptr->xclass_ptr[22]; break;
    case XSLICE_SIZE__3584: xclass_ptr = &xmpool_ptr->xclass_ptr[23]; break;
    case XSLICE_SIZE__3840: xclass_ptr = &xmpool_ptr->xclass_ptr[24]; break;
    case XSLICE_SIZE__4096: xclass_ptr = &xmpool_ptr->xclass_ptr[25]; break;
    case XSLICE_SIZE__4608: xclass_ptr = &xmpool_ptr->xclass_ptr[26]; break;
    case XSLICE_SIZE__5120: xclass_ptr = &xmpool_ptr->xclass_ptr[27]; break;
    case XSLICE_SIZE__5632: xclass_ptr = &xmpool_ptr->xclass_ptr[28]; break;
    case XSLICE_SIZE__6144: xclass_ptr = &xmpool_ptr->xclass_ptr[29]; break;
    case XSLICE_SIZE__6656: xclass_ptr = &xmpool_ptr->xclass_ptr[30]; break;
    case XSLICE_SIZE__7168: xclass_ptr = &xmpool_ptr->xclass_ptr[31]; break;
    case XSLICE_SIZE__7680: xclass_ptr = &xmpool_ptr->xclass_ptr[32]; break;
    case XSLICE_SIZE__8192: xclass_ptr = &xmpool_ptr->xclass_ptr[33]; break;
    case XSLICE_SIZE__9216: xclass_ptr = &xmpool_ptr->xclass_ptr[34]; break;
    case XSLICE_SIZE_10240: xclass_ptr = &xmpool_ptr->xclass_ptr[35]; break;
    case XSLICE_SIZE_11264: xclass_ptr = &xmpool_ptr->xclass_ptr[36]; break;
    case XSLICE_SIZE_12288: xclass_ptr = &xmpool_ptr->xclass_ptr[37]; break;
    case XSLICE_SIZE_13312: xclass_ptr = &xmpool_ptr->xclass_ptr[38]; break;
    case XSLICE_SIZE_14336: xclass_ptr = &xmpool_ptr->xclass_ptr[39]; break;
    case XSLICE_SIZE_15360: xclass_ptr = &xmpool_ptr->xclass_ptr[40]; break;
    case XSLICE_SIZE_16384: xclass_ptr = &xmpool_ptr->xclass_ptr[41]; break;
    case XSLICE_SIZE_18432: xclass_ptr = &xmpool_ptr->xclass_ptr[42]; break;
    case XSLICE_SIZE_20480: xclass_ptr = &xmpool_ptr->xclass_ptr[43]; break;
    case XSLICE_SIZE_22528: xclass_ptr = &xmpool_ptr->xclass_ptr[44]; break;
    case XSLICE_SIZE_24576: xclass_ptr = &xmpool_ptr->xclass_ptr[45]; break;
    case XSLICE_SIZE_26624: xclass_ptr = &xmpool_ptr->xclass_ptr[46]; break;
    case XSLICE_SIZE_28672: xclass_ptr = &xmpool_ptr->xclass_ptr[47]; break;
    case XSLICE_SIZE_30720: xclass_ptr = &xmpool_ptr->xclass_ptr[48]; break;
    case XSLICE_SIZE_32768: xclass_ptr = &xmpool_ptr->xclass_ptr[49]; break;
    case XSLICE_SIZE_36864: xclass_ptr = &xmpool_ptr->xclass_ptr[50]; break;
    case XSLICE_SIZE_40960: xclass_ptr = &xmpool_ptr->xclass_ptr[51]; break;
    case XSLICE_SIZE_45056: xclass_ptr = &xmpool_ptr->xclass_ptr[52]; break;
    case XSLICE_SIZE_49152: xclass_ptr = &xmpool_ptr->xclass_ptr[53]; break;
    case XSLICE_SIZE_53248: xclass_ptr = &xmpool_ptr->xclass_ptr[54]; break;
    case XSLICE_SIZE_57344: xclass_ptr = &xmpool_ptr->xclass_ptr[55]; break;
    case XSLICE_SIZE_61440: xclass_ptr = &xmpool_ptr->xclass_ptr[56]; break;
    case XSLICE_SIZE_65536: xclass_ptr = &xmpool_ptr->xclass_ptr[57]; break;

    default:
        break;
    }

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
                            xchunk_size, xmpool_ptr->xht_context);
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

        xchunk_ptr->xslice_queue.xut_offset =
                (xchunk_size - 
                 xslice_size * xchunk_ptr->xslice_queue.xut_capacity);

        for (xut_iter = 0;
             xut_iter < xchunk_ptr->xslice_queue.xut_capacity;
             ++xut_iter)
        {
            xchunk_ptr->xslice_queue.xut_index[xut_iter] = xut_iter;
        }
        xchunk_ptr->xslice_queue.xut_bpos = 0;
        xchunk_ptr->xslice_queue.xut_epos =
                            xchunk_ptr->xslice_queue.xut_capacity;
    }

    xmpool_ptr->xsize_valid +=
        (xchunk_ptr->xchunk_size - xchunk_ptr->xslice_queue.xut_offset);

    if (!xrbtree_insert_chunk(xmpool_ptr->xrbtree_ptr, xchunk_ptr))
    {
        XASSERT(X_FALSE);

        xmpool_ptr->xsize_valid -=
            (xchunk_ptr->xchunk_size - xchunk_ptr->xslice_queue.xut_offset);

        xmpool_ptr->xsize_cached -= xchunk_ptr->xchunk_size;
        xmpool_ptr->xfunc_free(xchunk_ptr,
                               xchunk_ptr->xchunk_size,
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
    return xrbtree_erase_chunk(xmpool_ptr->xrbtree_ptr, xchunk_ptr);
}

//====================================================================

// 
// xmem_pool_t : 对外提供的相关操作接口
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
        (xmpool_handle_t)xmem_heap_alloc(sizeof(xmem_pool_t), X_NULL);

    XASSERT(X_NULL != xmpool_ptr);

    xmpool_ptr->xfunc_alloc = (X_NULL != xfunc_alloc) ? xfunc_alloc : &xmem_heap_alloc;
    xmpool_ptr->xfunc_free  = (X_NULL != xfunc_free ) ? xfunc_free  : &xmem_heap_free ;
    xmpool_ptr->xht_context = xht_context;

    xmpool_ptr->xsize_cached = 0;
    xmpool_ptr->xsize_valid  = 0;
    xmpool_ptr->xsize_using  = 0;

    xcallback.xctxt_t_callback = xmpool_ptr;
    xmpool_ptr->xrbtree_ptr = xrbtree_create(sizeof(xchunk_handle_t), &xcallback);
    XASSERT(XRBT_NULL != xmpool_ptr->xrbtree_ptr);

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

    if (XRBT_NULL != xmpool_ptr->xrbtree_ptr)
    {
        xrbtree_destroy(xmpool_ptr->xrbtree_ptr);
        xmpool_ptr->xrbtree_ptr = XRBT_NULL;
    }

    xmpool_class_release(xmpool_ptr);

    xmpool_ptr->xfunc_alloc  = X_NULL;
    xmpool_ptr->xfunc_free   = X_NULL;
    xmpool_ptr->xht_context  = X_NULL;
    xmpool_ptr->xsize_cached = 0;
    xmpool_ptr->xsize_valid  = 0;
    xmpool_ptr->xsize_using  = 0;

    xmem_heap_free(xmpool_ptr, sizeof(xmem_pool_t), X_NULL);
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
            xmem_slice =
                ((xmem_slice_t)xchunk_ptr) +
                    xchunk_ptr->xslice_queue.xut_offset;
        }
    }
    else
    {
        xclass_ptr = xmpool_get_class(xmpool_ptr, xut_size);
        XASSERT(X_NULL != xclass_ptr);

        xchunk_ptr = xclass_get_non_empty_chunk(xclass_ptr);
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

    //======================================

    xchunk_handle_t xchunk_ptr =
        xrbtree_hit_chunk(xmpool_ptr->xrbtree_ptr, xmem_slice);

    if (X_NULL == xchunk_ptr)
    {
        return XMEM_ERR_SLICE_NOT_FOUND;
    }

    //======================================
    // 回收 slice

    // 若 chunk 对象没有多个分片，则不属于分类管理的 chunk 对象，可直接删除
    if (XCHUNK_SLICE_QSIZE(xchunk_ptr) == 0)
    {
        if (xmem_slice != ((xmem_slice_t)xchunk_ptr +
                           xchunk_ptr->xslice_queue.xut_offset))
        {
            return XMEM_ERR_SLICE_UNALIGNED;
        }

        xmpool_ptr->xsize_using -= xchunk_ptr->xslice_size;

        if (!xmpool_dealloc_chunk(xmpool_ptr, xchunk_ptr))
        {
            XASSERT(X_FALSE);
        }

        return XMEM_ERR_OK;
    }

    // chunk 对象属于分类管理的 chunk 类型，需要进行 chunk 分片回收操作
    if (!xchunk_recyc_slice(xchunk_ptr, xmem_slice))
    {
        return XMEM_ERR_SLICE_UNALIGNED;
    }

    xmpool_ptr->xsize_using -= xchunk_ptr->xslice_size;

    xclass_list_update(xchunk_ptr->xowner.xclass_ptr, xchunk_ptr);

    //======================================

    return XMEM_ERR_OK;
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

    for (xit_iter = 0; xit_iter < XSLICE_TYPE_AMOUNT; ++xit_iter)
    {
        xclass_ptr = &xmpool_ptr->xclass_ptr[xit_iter];

        for (xchunk_ptr = XCLASS_LIST_FRONT(xclass_ptr);
             xchunk_ptr != XCLASS_LIST_TAIL(xclass_ptr);)
        {
            if (XCHUNK_SLICE_QFULL(xchunk_ptr))
            {
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

#ifdef __GNU_C__
#pragma GCC diagnostic pop
#endif // __GNU_C__

////////////////////////////////////////////////////////////////////////////////
