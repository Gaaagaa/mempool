/**
 * @file    xmem_heap.c
 * <pre>
 * Copyright (c) 2019, Gaaagaa All rights reserved.
 * 
 * 文件名称：xmem_heap.c
 * 创建日期：2019年09月21日
 * 文件标识：
 * 文件摘要：堆内存管理的相关数据定义以及操作接口。
 * 
 * 当前版本：1.0.0.0
 * 作    者：
 * 完成日期：2019年09月21日
 * 版本摘要：
 * 
 * 历史版本：
 * 原作者  ：
 * 完成日期：
 * 版本摘要：
 * </pre>
 */

#include "xmem_comm.h"
#include "xrbtree.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif // __GNUC__

////////////////////////////////////////////////////////////////////////////////

//====================================================================

struct xmem_block_t;
struct xchunk_context_t;
struct xarray_cctxt_t;

typedef struct xmem_block_t     * xblock_handle_t;
typedef struct xchunk_context_t * xchunk_ctxptr_t;
typedef struct xarray_cctxt_t   * xarray_ctxptr_t;

#define XMHEAP_PAGE_SIZE    (1 * XMEM_PAGE_SIZE)
#define XARRAY_BLOCK_SIZE   (512 * XMHEAP_PAGE_SIZE)
#define XARRAY_CCTXT_SIZE   (sizeof(xchunk_context_t))
#define XMHEAP_RBTREE_SIZE  (16 * sizeof(x_handle_t))
#define XMHEAP_RBNODE_SIZE  (5 * sizeof(x_handle_t))

/** 比特位的 0 位判断 */
#define XMEM_BITS_IS_0(xmem_bits, xut_bpos) \
    (0 == ((xmem_bits)[(xut_bpos) >> 3] & ((x_byte_t)(1 << ((xut_bpos) & 7)))))

/** 比特位的 1 位判断 */
#define XMEM_BITS_IS_1(xmem_bits, xut_bpos) \
    (0 != ((xmem_bits)[(xut_bpos) >> 3] & ((x_byte_t)(1 << ((xut_bpos) & 7)))))

//====================================================================

/**
 * @struct xmem_block_t
 * @brief  堆内存区块描述信息的结构体。
 */
typedef struct xmem_block_t
{
    /**
     * @brief 当前 block 对象所在堆管理的双向链表节点信息。
     */
    struct
    {
    xblock_handle_t xblock_prev;   ///< 前驱节点
    xblock_handle_t xblock_next;   ///< 后继节点
    } xlist_node;

    x_uint32_t      xblock_size;   ///< 堆内存区块的大小
    x_uint32_t      xmpage_size;   ///< 分页大小
    x_uint32_t      xmpage_nums;   ///< 分页数量
    x_uint32_t      xmpage_rems;   ///< 分页剩余数量

    x_uint32_t      xmpage_offset; ///< 分页起始地址的偏移量
    x_uint32_t      xmpage_cursor; ///< 分页 申请/释放 操作时的游标位置
    x_byte_t        xmpage_bit[0]; ///< 分页是否被（分配出去）占用的位标识数组
} xmem_block_t;

/** 获取 xmem_block_t 的起始分页地址 */
#define XBLOCK_PAGE_BEGIN(xblock_ptr) \
    ((xmem_slice_t)(xblock_ptr) + (xblock_ptr)->xmpage_offset)

/** 获取 xmem_block_t 的指定分页地址 */
#define XBLOCK_PAGE_ADDR(xblock_ptr, xut_index) \
    (XBLOCK_PAGE_BEGIN(xblock_ptr) + (xut_index) * (xblock_ptr)->xmpage_size)

/** 获取 xmem_block_t 的结束分页地址 */
#define XBLOCK_PAGE_END(xblock_ptr) \
    ((xmem_slice_t)(xblock_ptr) + (xblock_ptr)->xblock_size)

/**
 * @struct xblock_alias_t
 * @brief 定义伪 block 的链表节点结构体，用于 双向链表 的 头部/尾部。
 * @note xblock_alias_t 与 xmem_block_t 首部字段必须一致。
 */
typedef struct xblock_alias_t
{
    /**
     * @brief 当前 block 对象所在堆管理的双向链表节点信息。
     */
    struct
    {
    xblock_handle_t xblock_prev;   ///< 前驱节点
    xblock_handle_t xblock_next;   ///< 后继节点
    } xlist_node;
} xblock_alias_t;

//====================================================================

/**
 * @struct xchunk_context_t
 * @brief  内存块上下文描述信息的结构体。
 */
typedef struct xchunk_context_t
{
    x_uint32_t      xchunk_size;   ///< 对应的 chunk 大小
    xchunk_memptr_t xchunk_ptr;    ///< 指向对应的 chunk 地址
    xowner_handle_t xowner_ptr;    ///< 持有该 chunk 的标识句柄
    xblock_handle_t xblock_ptr;    ///< chunk 缓存所在的 block
    xarray_ctxptr_t xarray_ptr;    ///< 所隶属的 xarray_cctxt_t 缓存数组

    /**
     * @brief 用于红黑树的节点占位结构体，
     * 记录当前 xchunk_context_t 对象在存储管理中的位置信息。
     */
    struct
    {
    x_byte_t xbt_ptr[XMHEAP_RBNODE_SIZE]; ///< 此字段仅起到内存占位的作用
    } xtree_node;
} xchunk_context_t;

/** chunk 内存块的起始地址 */
#define XCCTXT_LADDR(xcctxt_ptr) \
    ((xmem_slice_t)((xcctxt_ptr)->xchunk_ptr))

/** chunk 内存块的结束地址 */
#define XCCTXT_RADDR(xcctxt_ptr) \
    (XCCTXT_LADDR(xcctxt_ptr) + (xcctxt_ptr)->xchunk_size)

/**
 * @struct xarray_cctxt_t
 * @brief xchunk_context_t 对象缓存数组块的结构体描述信息。
 */
typedef struct xarray_cctxt_t
{
    /**
     * @brief 双向链表节点信息。
     */
    struct
    {
    xarray_ctxptr_t xarray_prev;   ///< 前驱节点
    xarray_ctxptr_t xarray_next;   ///< 后继节点
    } xlist_node;

    x_uint32_t      xarray_size;   ///< 对象的缓存大小
    x_uint32_t      xslice_size;   ///< 分片大小（sizeof(xchunk_context_t)）

    /**
     * @brief 节点分片索引号队列（xchunk_context_t 缓存队列）。
     */
    XSLICE_QUEUE_DEFINED(x_uint32_t);
} xarray_cctxt_t;

/** xarray_cctxt_t 对象的左（起始）地址（xmem_slice_t 类型指针） */
#define XARRAY_LADDR(xarray_ptr) ((xmem_slice_t)(xarray_ptr))

/** xarray_cctxt_t 对象的右（结束）地址（xmem_slice_t 类型指针） */
#define XARRAY_RADDR(xarray_ptr) \
    (XARRAY_LADDR(xarray_ptr) + (xarray_ptr)->xarray_size)

/**
 * @struct xarray_alias_t
 * @brief xarray_cctxt_t 的伪结构体，用于双向链表的 头部/尾部 节点。
 */
typedef struct xarray_alias_t
{
    /**
     * @brief 双向链表节点信息。
     */
    struct
    {
    xarray_ctxptr_t xarray_prev;   ///< 前驱节点
    xarray_ctxptr_t xarray_next;   ///< 后继节点
    } xlist_node;
} xarray_alias_t;

//====================================================================

/**
 * @struct xmem_heap_t
 * @brief  堆内存管理的描述信息结构体。
 */
typedef struct xmem_heap_t
{
    xatomic_lock_t  xmheap_lock;   ///< 访问操作的原子旋转锁
    x_uint32_t      xsize_block;   ///< 申请单个堆内存区块的建议大小
    x_uint64_t      xsize_ulimit;  ///< 可申请堆内存大小的总和上限
    x_uint64_t      xsize_cached;  ///< 总共申请的堆内存大小
    x_uint64_t      xsize_valid;   ///< 可使用到的堆内存大小
    x_uint64_t      xsize_using;   ///< 正在使用的堆内存大小

    /**
     * @brief 保存所有向系统申请的 xmem_block_t 对象的双向链表。
     */
    struct
    {
    x_uint32_t      xlist_size;    ///< 双向链表的节点数量
    xblock_alias_t  xlist_head;    ///< 双向链表的头部伪 block 节点
    xblock_alias_t  xlist_tail;    ///< 双向链表的尾部伪 block 节点
    } xlist_block;

    /**
     * @brief 保存所有向系统申请的 xarray_cctxt_t 对象的双向链表。
     */
    struct
    {
    x_uint32_t      xlist_size;    ///< 双向链表的节点数量
    xarray_alias_t  xlist_head;    ///< 双向链表的头部伪 block 节点
    xarray_alias_t  xlist_tail;    ///< 双向链表的尾部伪 block 节点
    } xlist_array;

    xarray_ctxptr_t xarray_cptr;   ///< 当前使用的 堆数组区块 对象

    /**
     * @brief 记录所有分配出去的 chunk 上下文信息（xchunk_context_t）的红黑树。
     */
    struct
    {
    x_byte_t xbt_ptr[XMHEAP_RBTREE_SIZE]; ///< 此字段仅起到内存占位的作用
    } xrbtree;
} xmem_heap_t;

/** xmem_block_t 链表节点数量 */
#define XMHEAP_BLOCK_LIST_SIZE(xmheap_ptr) \
            ((xmheap_ptr)->xlist_block.xlist_size)

/** xmem_block_t 链表头部节点 */
#define XMHEAP_BLOCK_LIST_HEAD(xmheap_ptr)  \
            ((xblock_handle_t)&(xmheap_ptr)->xlist_block.xlist_head)

/** xmem_block_t 链表前端节点 */
#define XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr) \
            ((xmheap_ptr)->xlist_block.xlist_head.xlist_node.xblock_next)

/** xmem_block_t 链表后端节点 */
#define XMHEAP_BLOCK_LIST_BACK(xmheap_ptr)  \
            ((xmheap_ptr)->xlist_block.xlist_tail.xlist_node.xblock_prev)

/** xmem_block_t 链表尾部节点 */
#define XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr)  \
            ((xblock_handle_t)&(xmheap_ptr)->xlist_block.xlist_tail)

/** xarray_cctxt_t 链表节点数量 */
#define XMHEAP_ARRAY_LIST_SIZE(xmheap_ptr) \
            ((xmheap_ptr)->xlist_array.xlist_size)

/** xarray_cctxt_t 链表头部节点 */
#define XMHEAP_ARRAY_LIST_HEAD(xmheap_ptr)  \
            ((xarray_ctxptr_t)&(xmheap_ptr)->xlist_array.xlist_head)

/** xarray_cctxt_t 链表前端节点 */
#define XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr) \
            ((xmheap_ptr)->xlist_array.xlist_head.xlist_node.xarray_next)

/** xarray_cctxt_t 链表后端节点 */
#define XMHEAP_ARRAY_LIST_BACK(xmheap_ptr)  \
            ((xmheap_ptr)->xlist_array.xlist_tail.xlist_node.xarray_prev)

/** xarray_cctxt_t 链表尾部节点 */
#define XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr)  \
            ((xarray_ctxptr_t)&(xmheap_ptr)->xlist_array.xlist_tail)

/** 存储 xchunk_context_t 的红黑树 */
#define XMHEAP_RBTREE(xmheap_ptr) ((x_rbtree_ptr)(xmheap_ptr)->xrbtree.xbt_ptr)

////////////////////////////////////////////////////////////////////////////////
// 函数前置声明

static x_void_t  xmheap_free_block(xmheap_handle_t, xblock_handle_t);
static x_void_t  xmheap_free_array(xmheap_handle_t, xarray_ctxptr_t);

////////////////////////////////////////////////////////////////////////////////

//====================================================================

// 
// auxiliary functions
// 

/**********************************************************/
/**
 * @brief 按给定的 堆内存区块大小，计算出其可分页的数量。
 */
static inline x_uint32_t xmem_block_page_nums(x_uint32_t xblock_size)
{
    XASSERT(xblock_size > sizeof(xmem_block_t));
    return ((8 * (xblock_size - sizeof(xmem_block_t))) /
            (1 + 8 * XMHEAP_PAGE_SIZE));
}

/**********************************************************/
/**
 * @brief 计算 xarray_size 按 xslice_size 进行分片时，可分片的数量。
 */
static inline x_uint32_t xmem_array_capacity(
    x_uint32_t xarray_size, x_uint32_t xslice_size)
{
    XASSERT(xarray_size >=
            (sizeof(xarray_cctxt_t) + sizeof(x_uint32_t) + xslice_size));

	return ((xarray_size - sizeof(xarray_cctxt_t)) /
            (xslice_size + sizeof(x_uint32_t)));
}

/**********************************************************/
/**
 * @brief 对内存位区置位（0 或 1）。
 * @note 函数内部不对 xmem_bits 内存越界检测，使用时需要注意。
 * 
 * @param [in ] xmem_bits : 目标操作的内存位区。
 * @param [in ] xut_bpos  : 起始位置（按 位 计数）。
 * @param [in ] xut_nums  : 置位数量（按 位 计数）。
 * @param [in ] xut_vbit  : 置位值（0 或 1）。
 */
static x_void_t xmem_bits_set(xmem_slice_t xmem_bits,
                              x_uint32_t xut_bpos,
                              x_uint32_t xut_nums,
                              x_uint32_t xut_vbit)
{
    XASSERT(X_NULL != xmem_bits);

    x_uint32_t xut_head = 0;
    x_uint32_t xut_tail = 0;
    x_uint32_t xut_midl = 0;

    //======================================
    // 未跨字节的情况。
    // 满字节的情况（如 xut_bpos: 0, xut_nums: 8），
    // 可按跨字节的情况处理

    if ((xut_bpos >> 3) == ((xut_bpos + xut_nums) >> 3))
    {
        if (0 == xut_vbit)
        {
            xmem_bits[xut_bpos >> 3] &=
                ~((x_byte_t)(((1 << xut_nums) - 1) << (xut_bpos & 7)));
        }
        else
        {
            xmem_bits[xut_bpos >> 3] |=
                ((x_byte_t)(((1 << xut_nums) - 1) << (xut_bpos & 7)));
        }

        return;
    }

    //======================================
    // 跨字节的情况

    xut_head = (8 - (xut_bpos & 7)) & 7;
    xut_tail = (xut_bpos + xut_nums) & 7;
    xut_midl = xut_nums - xut_head - xut_tail;

    XASSERT(0 == (xut_midl & 7));

    if (0 == xut_vbit)
    {
        if (xut_head > 0)
            xmem_bits[xut_bpos >> 3] &= (x_byte_t)(0xFF >> xut_head);

        if (xut_midl > 0)
            memset(xmem_bits + ((xut_bpos + 7) >> 3), 0x00, xut_midl >> 3);

        if (xut_tail > 0)
            xmem_bits[(xut_bpos + xut_nums) >> 3] &=
                                (x_byte_t)(0xFF << xut_tail);
    }
    else
    {
        if (xut_head > 0)
            xmem_bits[xut_bpos >> 3] |= (x_byte_t)(0xFF << (8 - xut_head));

        if (xut_midl > 0)
            memset(xmem_bits + ((xut_bpos + 7) >> 3), 0xFF, xut_midl >> 3);

        if (xut_tail > 0)
            xmem_bits[(xut_bpos + xut_nums) >> 3] |=
                                (x_byte_t)(0xFF >> (8 - xut_tail));
    }

    //======================================
}

/**********************************************************/
/**
 * @brief 对内存位区进行 “0” 位检测。
 * @note 函数内部不对 xmem_bits 内存越界检测，使用时需要注意。
 * 
 * @param [in ] xmem_bits : 目标操作的内存位区。
 * @param [in ] xut_bpos  : 起始位置（按 位 计数）。
 * @param [in ] xut_nums  : 检测位数量（按 位 计数）。
 * 
 * @return x_uint32_t
 *         - 返回值表示最后进行 “0” 位检测的停止位置。
 */
static x_uint32_t xmem_bits_check_0(xmem_slice_t xmem_bits,
                                    x_uint32_t xut_bpos,
                                    x_uint32_t xut_nums)
{
    XASSERT(X_NULL != xmem_bits);
    XASSERT(xut_nums > 0);

    x_uint32_t xut_head = 0;
    x_uint32_t xut_tail = 0;
    x_uint32_t xut_midl = 0;

    x_uint32_t xut_iter = xut_bpos;

    //======================================
    // 未跨字节的情况。
    // 满字节的情况（如 xut_bpos: 0, xut_nums: 8），
    // 可按跨字节的情况处理

    if ((xut_bpos >> 3) == ((xut_bpos + xut_nums) >> 3))
    {
        while (xut_nums-- > 0)
        {
            if (XMEM_BITS_IS_1(xmem_bits, xut_iter))
                break;
            xut_iter += 1;
        }

        return xut_iter;
    }

    //======================================
    // 跨字节的情况

    xut_head = (8 - (xut_bpos & 7)) & 7;
    xut_tail = (xut_bpos + xut_nums) & 7;
    xut_midl = xut_nums - xut_head - xut_tail;

    XASSERT(0 == (xut_midl & 7));
    xut_midl >>= 3;

    while (xut_head-- > 0)
    {
        if (XMEM_BITS_IS_1(xmem_bits, xut_iter))
            return xut_iter;
        xut_iter += 1;
    }

    XASSERT(0 == (xut_iter & 7));

    while (xut_midl-- > 0)
    {
        if (0x00 == xmem_bits[xut_iter >> 3])
        {
            xut_iter += 8;
        }
        else
        {
            xut_midl = 8;
            while (xut_midl-- > 0)
            {
                if (XMEM_BITS_IS_1(xmem_bits, xut_iter))
                    break;
                xut_iter += 1;
            }

            return xut_iter;
        }
    }

    XASSERT(0 == (xut_iter & 7));

    while (xut_tail-- > 0)
    {
        if (XMEM_BITS_IS_1(xmem_bits, xut_iter))
            return xut_iter;
        xut_iter += 1;
    }

    //======================================

    return xut_iter;
}

/**********************************************************/
/**
 * @brief 对内存位区进行 “1” 位检测。
 * @note 函数内部不对 xmem_bits 内存越界检测，使用时需要注意。
 * 
 * @param [in ] xmem_bits : 目标操作的内存位区。
 * @param [in ] xut_bpos  : 起始位置（按 位 计数）。
 * @param [in ] xut_nums  : 检测位数量（按 位 计数）。
 * 
 * @return x_uint32_t
 *         - 返回值表示最后进行 “1” 位检测的停止位置。
 */
static x_uint32_t xmem_bits_check_1(xmem_slice_t xmem_bits,
                                    x_uint32_t xut_bpos,
                                    x_uint32_t xut_nums)
{
    XASSERT(X_NULL != xmem_bits);
    XASSERT(xut_nums > 0);

    x_uint32_t xut_head = 0;
    x_uint32_t xut_tail = 0;
    x_uint32_t xut_midl = 0;

    x_uint32_t xut_iter = xut_bpos;

    //======================================
    // 未跨字节的情况。
    // 满字节的情况（如 xut_bpos: 0, xut_nums: 8），
    // 可按跨字节的情况处理

    if ((xut_bpos >> 3) == ((xut_bpos + xut_nums) >> 3))
    {
        while (xut_nums-- > 0)
        {
            if (XMEM_BITS_IS_0(xmem_bits, xut_iter))
                break;
            xut_iter += 1;
        }

        return xut_iter;
    }

    //======================================
    // 跨字节的情况

    xut_head = (8 - (xut_bpos & 7)) & 7;
    xut_tail = (xut_bpos + xut_nums) & 7;
    xut_midl = xut_nums - xut_head - xut_tail;

    XASSERT(0 == (xut_midl & 7));
    xut_midl >>= 3;

    while (xut_head-- > 0)
    {
        if (XMEM_BITS_IS_0(xmem_bits, xut_iter))
            return xut_iter;
        xut_iter += 1;
    }

    XASSERT(0 == (xut_iter & 7));

    while (xut_midl-- > 0)
    {
        if (0xFF == xmem_bits[xut_iter >> 3])
        {
            xut_iter += 8;
        }
        else
        {
            xut_midl = 8;
            while (xut_midl-- > 0)
            {
                if (XMEM_BITS_IS_0(xmem_bits, xut_iter))
                    break;
                xut_iter += 1;
            }

            return xut_iter;
        }
    }

    XASSERT(0 == (xut_iter & 7));

    while (xut_tail-- > 0)
    {
        if (XMEM_BITS_IS_0(xmem_bits, xut_iter))
            return xut_iter;
        xut_iter += 1;
    }

    //======================================

    return xut_iter;
}

////////////////////////////////////////////////////////////////////////////////

//====================================================================

// 
// xmem_block_t : internal calls
// 

/**********************************************************/
/**
 * @brief 从堆内存区块中申请内存块。
 */
static xchunk_memptr_t xblock_alloc_chunk(
                            xblock_handle_t xblock_ptr,
                            x_uint32_t xchunk_size)
{
    XASSERT(xchunk_size == X_ALIGN(xchunk_size, xblock_ptr->xmpage_size));

    x_uint32_t xmpage_bpos = 0;
    x_uint32_t xmpage_epos = 0;
    x_uint32_t xmpage_cpos = 0;
    x_uint32_t xmpage_nums = xchunk_size / xblock_ptr->xmpage_size;

    if (xmpage_nums > xblock_ptr->xmpage_rems)
    {
        return X_NULL;
    }

    //======================================
    // 游标后进行遍历查找可用的内存块

    xmpage_bpos = xblock_ptr->xmpage_cursor;
    while ((xmpage_bpos + xmpage_nums) < xblock_ptr->xmpage_nums)
    {
        xmpage_epos = xmem_bits_check_0(xblock_ptr->xmpage_bit,
                                        xmpage_bpos,
                                        xmpage_nums);
        if (xmpage_epos >= (xmpage_bpos + xmpage_nums))
        {
            // 将内存块对应的区位置 1 ，标识已被分配
            xmem_bits_set(xblock_ptr->xmpage_bit, xmpage_bpos, xmpage_nums, 1);

            // 更新剩余分页数量
            xblock_ptr->xmpage_rems -= xmpage_nums;

            // 更新 游标 到 内存块末端 对应的索引位置
            xblock_ptr->xmpage_cursor = xmpage_bpos + xmpage_nums;

            // 返回内存块地址
            return XBLOCK_PAGE_ADDR(xblock_ptr, xmpage_bpos);
        }

        if (xmpage_bpos == xblock_ptr->xmpage_cursor)
        {
            // 记录游标后首个 “1” 的位置，
            // 作为“游标前遍历查找可用内存块”的末端位置
            xmpage_cpos = xmpage_epos;
        }

        xmpage_bpos = xmem_bits_check_1(xblock_ptr->xmpage_bit,
                                        xmpage_epos,
                                        xblock_ptr->xmpage_nums - xmpage_epos);
    }

    //======================================
    // 游标前遍历查找可用的内存块

    xmpage_bpos = 0;
    while ((xmpage_bpos + xmpage_nums) < xmpage_cpos)
    {
        xmpage_epos = xmem_bits_check_0(xblock_ptr->xmpage_bit,
                                        xmpage_bpos,
                                        xmpage_nums);
        if (xmpage_epos >= (xmpage_bpos + xmpage_nums))
        {
            // 将内存块对应的区位置 1 ，标识已被分配
            xmem_bits_set(xblock_ptr->xmpage_bit, xmpage_bpos, xmpage_nums, 1);

            // 更新剩余分页数量
            xblock_ptr->xmpage_rems -= xmpage_nums;

            // 更新 游标 到 内存块末端 对应的索引位置
            xblock_ptr->xmpage_cursor = xmpage_bpos + xmpage_nums;

            // 返回内存块地址
            return XBLOCK_PAGE_ADDR(xblock_ptr, xmpage_bpos);
        }

        xmpage_bpos = xmem_bits_check_1(xblock_ptr->xmpage_bit,
                                        xmpage_epos,
                                        xmpage_cpos - xmpage_epos);
    }

    //======================================

    return X_NULL;
}

/**********************************************************/
/**
 * @brief 从新申请的堆内存区块中申请内存块。
 */
static xchunk_memptr_t xblock_alloc_chunk_N(
                            xblock_handle_t xblock_ptr,
                            x_uint32_t xchunk_size)
{
    XASSERT(xblock_ptr->xmpage_nums == xblock_ptr->xmpage_rems);
    XASSERT(xchunk_size == X_ALIGN(xchunk_size, xblock_ptr->xmpage_size));
    XASSERT(xchunk_size <= (xblock_ptr->xmpage_nums * XMHEAP_PAGE_SIZE));

    x_uint32_t xmpage_nums = xchunk_size / xblock_ptr->xmpage_size;

    XASSERT(xmpage_nums <=
        xmem_bits_check_0(xblock_ptr->xmpage_bit, 0, xmpage_nums));

    // 将内存块对应的区位置 1 ，标识已被分配
    xmem_bits_set(xblock_ptr->xmpage_bit, 0, xmpage_nums, 1);

    // 更新剩余分页数量
    xblock_ptr->xmpage_rems -= xmpage_nums;

    // 更新 游标 到 内存块末端 对应的索引位置
    xblock_ptr->xmpage_cursor = xmpage_nums;

    // 返回内存块地址
    return XBLOCK_PAGE_ADDR(xblock_ptr, 0);
}

/**********************************************************/
/**
 * @brief 将 内存块 回收到 堆内存区块 中。
 */
static x_int32_t xblock_recyc_chunk(
                            xblock_handle_t xblock_ptr,
                            xchunk_memptr_t xchunk_ptr,
                            x_uint32_t xchunk_size)
{
    XASSERT((((xmem_slice_t)xchunk_ptr) >= XBLOCK_PAGE_BEGIN(xblock_ptr)) &&
            (((xmem_slice_t)xchunk_ptr) <  XBLOCK_PAGE_END(xblock_ptr)));
    XASSERT(xchunk_size == X_ALIGN(xchunk_size, xblock_ptr->xmpage_size));

    //======================================
    // 回收内存块

    x_uint32_t xmpage_nums = xchunk_size / xblock_ptr->xmpage_size;
    x_uint32_t xmpage_bpos = (x_uint32_t)((xmem_slice_t)xchunk_ptr -
                                          XBLOCK_PAGE_BEGIN(xblock_ptr));

    if (0 != (xmpage_bpos % xblock_ptr->xmpage_size))
    {
        return XMEM_ERR_UNALIGNED;
    }

    xmpage_bpos /= xblock_ptr->xmpage_size;

    XASSERT(xmem_bits_check_1(
                xblock_ptr->xmpage_bit, xmpage_bpos, xmpage_nums) >=
            (xmpage_bpos + xmpage_nums));
    xmem_bits_set(xblock_ptr->xmpage_bit, xmpage_bpos, xmpage_nums, 0);

    //======================================
    // 更新 block 信息

    xblock_ptr->xmpage_rems  += xmpage_nums;
    xblock_ptr->xmpage_cursor = xmpage_bpos;

    //======================================

    return XMEM_ERR_OK;
}

//====================================================================

// 
// xarray_cctxt_t : internal calls
// 

/**********************************************************/
/**
 * @brief 查找分片索引号是否还在 xarray_cctxt_t 的分片队列中。
 * @note  该接口只用于断言测试。
 */
static x_bool_t xarray_find_index(
    xarray_ctxptr_t xarray_ptr, x_uint32_t xut_index)
{
    x_uint32_t xut_iter = XSLICE_QUEUE(xarray_ptr).xut_bpos;
    while (xut_iter != XSLICE_QUEUE(xarray_ptr).xut_epos)
    {
        if (xut_index ==
            XSLICE_QUEUE_INDEX_GET(xarray_ptr, xut_iter, x_uint32_t))
        {
            return X_TRUE;
        }

        xut_iter += 1;
    }

    return X_FALSE;
}

/**********************************************************/
/**
 * @brief 从 xarray_cctxt_t 对象中申请 xchunk_context_t 缓存。
 * 
 * @param [in ] xarray_ptr : xarray_cctxt_t 对象。
 * 
 * @return xchunk_ctxptr_t
 *         - 成功，返回 xchunk_ctxptr_t 节点缓存；
 *         - 失败，返回 X_NULL，xarray_cctxt_t 对象的分片队列已经为空。
 */
static xchunk_ctxptr_t xarray_alloc_cctxt(xarray_ctxptr_t xarray_ptr)
{
    x_uint32_t xut_index = 0;

    if (XSLICE_QUEUE_IS_EMPTY(xarray_ptr))
    {
        return X_NULL;
    }

    xut_index = XSLICE_QUEUE_INDEX_GET(
                    xarray_ptr, XSLICE_QUEUE(xarray_ptr).xut_bpos, x_uint32_t);

    XASSERT(xut_index < XSLICE_QUEUE_CAPACITY(xarray_ptr));
    XASSERT(!XSLICE_QUEUE_IS_ALLOCATED(xarray_ptr, xut_index, x_uint32_t));

    // 设置分片“已被分配出去”的标识位
    XSLICE_QUEUE_ALLOCATED_SET(xarray_ptr, xut_index, x_uint32_t);

    XSLICE_QUEUE(xarray_ptr).xut_bpos += 1;
    XASSERT(0 != XSLICE_QUEUE(xarray_ptr).xut_bpos);

    return (xchunk_ctxptr_t)(XSLICE_QUEUE_GET(xarray_ptr, xut_index));
}

/**********************************************************/
/**
 * @brief 回收 xchunk_context_t 缓存分片至 xarray_cctxt_t 对象中。
 * 
 * @param [in ] xarray_ptr : chunk 对象。
 * @param [in ] xcctxt_ptr : 待回收的内存分片。
 * 
 * @return x_int32_t
 *         - 成功，返回 XMEM_ERR_OK；
 *         - 失败，返回 错误码。
 */
static x_int32_t xarray_recyc_cctxt(
                    xarray_ctxptr_t xarray_ptr,
                    xchunk_ctxptr_t xcctxt_ptr)
{
    XASSERT(((xmem_slice_t)xcctxt_ptr > XARRAY_LADDR(xarray_ptr)) &&
            ((xmem_slice_t)xcctxt_ptr < XARRAY_RADDR(xarray_ptr)));
    XASSERT(XSLICE_QUEUE_CAPACITY(xarray_ptr) > 0);
    XASSERT(!XSLICE_QUEUE_IS_FULL(xarray_ptr, x_uint32_t));

    x_int32_t  xit_error  = XMEM_ERR_UNKNOW;
    x_uint32_t xut_offset = 0;
    x_uint32_t xut_index  = 0;

    do
    {
        //======================================

        xut_offset = (x_uint32_t)((xmem_slice_t)xcctxt_ptr -
                                  XARRAY_LADDR(xarray_ptr));
        if (xut_offset < XSLICE_QUEUE(xarray_ptr).xut_offset)
        {
            xit_error = XMEM_ERR_UNALIGNED;
            break;
        }

        xut_offset -= XSLICE_QUEUE(xarray_ptr).xut_offset;
        if (0 != (xut_offset % xarray_ptr->xslice_size))
        {
            xit_error = XMEM_ERR_UNALIGNED;
            break;
        }

        //======================================

        xut_index = xut_offset / xarray_ptr->xslice_size;
        XASSERT(xut_index < XSLICE_QUEUE_CAPACITY(xarray_ptr));

        // 判断分片是否已经被回收
        if (!XSLICE_QUEUE_IS_ALLOCATED(xarray_ptr, xut_index, x_uint32_t))
        {
            xit_error = XMEM_ERR_RECYCLED;
            break;
        }

        XASSERT(!xarray_find_index(xarray_ptr, xut_index));
        XSLICE_QUEUE_INDEX_SET(xarray_ptr,
                               XSLICE_QUEUE(xarray_ptr).xut_epos,
                               xut_index,
                               x_uint32_t);

        // 标识分片“未被分配出去”
        XSLICE_QUEUE_ALLOCATED_RESET(xarray_ptr, xut_index, x_uint32_t);

        XSLICE_QUEUE(xarray_ptr).xut_epos += 1;

        if (0 == XSLICE_QUEUE(xarray_ptr).xut_epos)
        {
            XSLICE_QUEUE(xarray_ptr).xut_epos =
                XSLICE_QUEUE_COUNT(xarray_ptr, x_uint32_t);

            XSLICE_QUEUE(xarray_ptr).xut_bpos %=
                XSLICE_QUEUE_CAPACITY(xarray_ptr);

            XSLICE_QUEUE(xarray_ptr).xut_epos +=
                XSLICE_QUEUE(xarray_ptr).xut_bpos;
        }

        //======================================

        xit_error = XMEM_ERR_OK;
    } while (0);

    return xit_error;
}

//====================================================================

// 
// xmem_heap_t : internal calls
// 

/**********************************************************/
/**
 * @brief 将堆内存区块对象从链表中移出。
 */
static x_void_t xmheap_block_list_erase(
                            xmheap_handle_t xmheap_ptr,
                            xblock_handle_t xblock_ptr)
{
    XASSERT(X_NULL != xblock_ptr->xlist_node.xblock_prev);
    XASSERT(X_NULL != xblock_ptr->xlist_node.xblock_next);

    xblock_ptr->xlist_node.xblock_prev->xlist_node.xblock_next =
        xblock_ptr->xlist_node.xblock_next;
    xblock_ptr->xlist_node.xblock_next->xlist_node.xblock_prev =
        xblock_ptr->xlist_node.xblock_prev;

    xblock_ptr->xlist_node.xblock_prev = X_NULL;
    xblock_ptr->xlist_node.xblock_next = X_NULL;

    XMHEAP_BLOCK_LIST_SIZE(xmheap_ptr) -= 1;
}

/**********************************************************/
/**
 * @brief 将堆内存区块对象压入链表头部。
 */
static x_void_t xmheap_block_list_push_head(
                            xmheap_handle_t xmheap_ptr,
                            xblock_handle_t xblock_ptr)
{
    if (XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr) == xblock_ptr)
    {
        return;
    }

    xblock_ptr->xlist_node.xblock_prev = XMHEAP_BLOCK_LIST_HEAD(xmheap_ptr);
    xblock_ptr->xlist_node.xblock_next = XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr);

    XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr)->xlist_node.xblock_prev = xblock_ptr;
    XMHEAP_BLOCK_LIST_HEAD(xmheap_ptr)->xlist_node.xblock_next  = xblock_ptr;

    XMHEAP_BLOCK_LIST_SIZE(xmheap_ptr) += 1;
}

/**********************************************************/
/**
 * @brief 将堆内存区块对象压入链表尾部。
 */
static x_void_t xmheap_block_list_push_tail(
                            xmheap_handle_t xmheap_ptr,
                            xblock_handle_t xblock_ptr)
{
    if (XMHEAP_BLOCK_LIST_BACK(xmheap_ptr) == xblock_ptr)
    {
        return;
    }

    xblock_ptr->xlist_node.xblock_next = XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr);
    xblock_ptr->xlist_node.xblock_prev = XMHEAP_BLOCK_LIST_BACK(xmheap_ptr);

    XMHEAP_BLOCK_LIST_BACK(xmheap_ptr)->xlist_node.xblock_next = xblock_ptr;
    XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr)->xlist_node.xblock_prev = xblock_ptr;

    XMHEAP_BLOCK_LIST_SIZE(xmheap_ptr) += 1;
}

/**********************************************************/
/**
 * @brief 初始化 堆内存区块 管理链表。
 */
static x_void_t xmheap_block_list_init(xmheap_handle_t xmheap_ptr)
{
    XMHEAP_BLOCK_LIST_SIZE(xmheap_ptr) = 0;

    XMHEAP_BLOCK_LIST_HEAD(xmheap_ptr)->
            xlist_node.xblock_prev = X_NULL;
    XMHEAP_BLOCK_LIST_HEAD(xmheap_ptr)->
            xlist_node.xblock_next = XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr);
    XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr)->
            xlist_node.xblock_prev = XMHEAP_BLOCK_LIST_HEAD(xmheap_ptr);
    XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr)->
            xlist_node.xblock_next = X_NULL;
}

/**********************************************************/
/**
 * @brief 释放管理链表中的所有 堆内存区块。
 */
static x_void_t xmheap_block_list_release(xmheap_handle_t xmheap_ptr)
{
    xblock_handle_t xblock_ptr = XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr);
    while (xblock_ptr != XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr))
    {
        xmheap_free_block(xmheap_ptr, xblock_ptr);
        xblock_ptr = XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr);
    }
}

/**********************************************************/
/**
 * @brief 将 xarray_cctxt_t 对象从链表中移出。
 */
static x_void_t xmheap_array_list_erase(
                            xmheap_handle_t xmheap_ptr,
                            xarray_ctxptr_t xarray_ptr)
{
    XASSERT(X_NULL != xarray_ptr->xlist_node.xarray_prev);
    XASSERT(X_NULL != xarray_ptr->xlist_node.xarray_next);

    xarray_ptr->xlist_node.xarray_prev->xlist_node.xarray_next =
        xarray_ptr->xlist_node.xarray_next;
    xarray_ptr->xlist_node.xarray_next->xlist_node.xarray_prev =
        xarray_ptr->xlist_node.xarray_prev;

    xarray_ptr->xlist_node.xarray_prev = X_NULL;
    xarray_ptr->xlist_node.xarray_next = X_NULL;

    XMHEAP_ARRAY_LIST_SIZE(xmheap_ptr) -= 1;
}

/**********************************************************/
/**
 * @brief 将 xarray_cctxt_t 对象压入链表头部。
 */
static x_void_t xmheap_array_list_push_head(
                            xmheap_handle_t xmheap_ptr,
                            xarray_ctxptr_t xarray_ptr)
{
    if (XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr) == xarray_ptr)
    {
        return;
    }

    xarray_ptr->xlist_node.xarray_prev = XMHEAP_ARRAY_LIST_HEAD(xmheap_ptr);
    xarray_ptr->xlist_node.xarray_next = XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr);

    XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr)->xlist_node.xarray_prev = xarray_ptr;
    XMHEAP_ARRAY_LIST_HEAD(xmheap_ptr)->xlist_node.xarray_next  = xarray_ptr;

    XMHEAP_ARRAY_LIST_SIZE(xmheap_ptr) += 1;
}

/**********************************************************/
/**
 * @brief 将 xarray_cctxt_t 对象压入链表尾部。
 */
static x_void_t xmheap_array_list_push_tail(
                            xmheap_handle_t xmheap_ptr,
                            xarray_ctxptr_t xarray_ptr)
{
    if (XMHEAP_ARRAY_LIST_BACK(xmheap_ptr) == xarray_ptr)
    {
        return;
    }

    xarray_ptr->xlist_node.xarray_next = XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr);
    xarray_ptr->xlist_node.xarray_prev = XMHEAP_ARRAY_LIST_BACK(xmheap_ptr);

    XMHEAP_ARRAY_LIST_BACK(xmheap_ptr)->xlist_node.xarray_next = xarray_ptr;
    XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr)->xlist_node.xarray_prev = xarray_ptr;

    XMHEAP_ARRAY_LIST_SIZE(xmheap_ptr) += 1;
}

/**********************************************************/
/**
 * @brief 初始化 xchunk_context_t 使用的 缓存数组 链表。
 */
static x_void_t xmheap_array_list_init(xmheap_handle_t xmheap_ptr)
{
    XMHEAP_ARRAY_LIST_SIZE(xmheap_ptr) = 0;

    XMHEAP_ARRAY_LIST_HEAD(xmheap_ptr)->
            xlist_node.xarray_prev = X_NULL;
    XMHEAP_ARRAY_LIST_HEAD(xmheap_ptr)->
            xlist_node.xarray_next = XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr);
    XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr)->
            xlist_node.xarray_prev = XMHEAP_ARRAY_LIST_HEAD(xmheap_ptr);
    XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr)->
            xlist_node.xarray_next = X_NULL;
}

/**********************************************************/
/**
 * @brief 释放 xchunk_context_t 使用的 缓存数组 链表。
 */
static x_void_t xmheap_array_list_release(xmheap_handle_t xmheap_ptr)
{
    xarray_ctxptr_t xarray_ptr = XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr);
    while (xarray_ptr != XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr))
    {
        xmheap_free_array(xmheap_ptr, xarray_ptr);
        xarray_ptr = XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr);
    }
}

/**********************************************************/
/**
 * @brief 申请堆内存区块对象。
 * 
 * @param [in ] xmheap_ptr  : 堆管理对象。
 * @param [in ] xblock_size : 堆内存区块大小。
 * 
 * @return xblock_handle_t
 *         - 成功，返回 堆内存区块对象句柄；
 *         - 失败，返回 X_NULL。
 */
static xblock_handle_t xmheap_alloc_block(
                            xmheap_handle_t xmheap_ptr,
                            x_uint32_t xblock_size)
{
    XASSERT(xblock_size >= (sizeof(xmem_block_t) + XMHEAP_PAGE_SIZE));
    XASSERT(xblock_size == X_ALIGN(xblock_size, XMHEAP_PAGE_SIZE));

    x_uint32_t xmpage_nums = xmem_block_page_nums(xblock_size);

    xblock_handle_t xblock_ptr =
        (xblock_handle_t)xsys_heap_alloc(xblock_size);
    if (X_NULL == xblock_ptr)
    {
        return X_NULL;
    }

    xmheap_ptr->xsize_valid  += (xmpage_nums * XMHEAP_PAGE_SIZE);
    xmheap_ptr->xsize_cached += xblock_size;

    xblock_ptr->xlist_node.xblock_prev = X_NULL;
    xblock_ptr->xlist_node.xblock_next = X_NULL;

    xblock_ptr->xblock_size   = xblock_size;
    xblock_ptr->xmpage_size   = XMHEAP_PAGE_SIZE;
    xblock_ptr->xmpage_nums   = xmpage_nums;
    xblock_ptr->xmpage_rems   = xmpage_nums;
    xblock_ptr->xmpage_offset = xblock_size - (xmpage_nums * XMHEAP_PAGE_SIZE);
    xblock_ptr->xmpage_cursor = 0;

    xmem_clear(xblock_ptr->xmpage_bit, ((xmpage_nums + 7) / 8));

    return xblock_ptr;
}

/**********************************************************/
/**
 * @brief 释放堆内存区块对象。
 */
static x_void_t xmheap_free_block(
                            xmheap_handle_t xmheap_ptr,
                            xblock_handle_t xblock_ptr)
{
    XASSERT(xblock_ptr->xmpage_nums == xblock_ptr->xmpage_rems);

    xmheap_ptr->xsize_valid  -= (xblock_ptr->xmpage_nums * XMHEAP_PAGE_SIZE);
    xmheap_ptr->xsize_cached -= xblock_ptr->xblock_size;

    xmheap_block_list_erase(xmheap_ptr, xblock_ptr);
    xsys_heap_free(xblock_ptr, xblock_ptr->xblock_size);
}

/**********************************************************/
/**
 * @brief 释放 堆内存区块 链表中的空闲的 堆内存区块。
 */
static x_void_t xmheap_free_unused_block(xmheap_handle_t xmheap_ptr)
{
    xblock_handle_t xblock_tmp = X_NULL;
    xblock_handle_t xblock_ptr = XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr);

    while (xblock_ptr != XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr))
    {
        if (xblock_ptr->xmpage_nums == xblock_ptr->xmpage_rems)
        {
            xblock_tmp = xblock_ptr->xlist_node.xblock_next;
            xmheap_free_block(xmheap_ptr, xblock_ptr);
            xblock_ptr = xblock_tmp;
        }
        else
        {
            xblock_ptr = xblock_ptr->xlist_node.xblock_next;
        }
    }
}

/**********************************************************/
/**
 * @brief 申请堆数组区块对象（用于缓存 xchunk_context_t 对象）。
 * 
 * @param [in ] xmheap_ptr  : 堆管理对象。
 * @param [in ] xarray_size : 堆数组区块大小。
 * 
 * @return xblock_handle_t
 *         - 成功，返回 堆数组区块对象；
 *         - 失败，返回 X_NULL。
 */
static xarray_ctxptr_t xmheap_alloc_array(
                            xmheap_handle_t xmheap_ptr,
                            x_uint32_t xarray_size)
{
    XASSERT(X_NULL != xmheap_ptr);
    XASSERT(xarray_size > (sizeof(xchunk_context_t) + sizeof(xarray_cctxt_t)));

    x_uint32_t xut_iter    = 0;
    x_uint32_t xslice_size = sizeof(xchunk_context_t);

    xarray_ctxptr_t xarray_ptr = (xarray_ctxptr_t)xsys_heap_alloc(xarray_size);
    XASSERT(X_NULL != xarray_ptr);

    xmheap_ptr->xsize_cached += xarray_size;

    xmem_clear(xarray_ptr, sizeof(xarray_cctxt_t));

    //======================================

    xarray_ptr->xarray_size = xarray_size;
    xarray_ptr->xslice_size = xslice_size;

    XSLICE_QUEUE_CAPACITY(xarray_ptr) =
            xmem_array_capacity(xarray_size, xslice_size);
    XASSERT(XSLICE_QUEUE_CAPACITY(xarray_ptr) <= XSLICE_IMASK(x_uint32_t));

    XSLICE_QUEUE(xarray_ptr).xut_offset =
            (xarray_size - 
             xslice_size * XSLICE_QUEUE_CAPACITY(xarray_ptr));

    for (xut_iter = 0; xut_iter < XSLICE_QUEUE_CAPACITY(xarray_ptr); ++xut_iter)
    {
        // 最高位为 0 值，表示分片未被分配出去
        XSLICE_QUEUE(xarray_ptr).xut_index[xut_iter] =
                        (xut_iter & XSLICE_IMASK(x_uint32_t));
    }

    XSLICE_QUEUE(xarray_ptr).xut_bpos = 0;
    XSLICE_QUEUE(xarray_ptr).xut_epos = XSLICE_QUEUE_CAPACITY(xarray_ptr);

    //======================================

    return xarray_ptr;
}

/**********************************************************/
/**
 * @brief 释放堆数组区块对象。
 */
static x_void_t xmheap_free_array(
                            xmheap_handle_t xmheap_ptr,
                            xarray_ctxptr_t xarray_ptr)
{
    XASSERT(XSLICE_QUEUE_IS_FULL(xarray_ptr, x_uint32_t));

    if (xmheap_ptr->xarray_cptr == xarray_ptr)
    {
        xmheap_ptr->xarray_cptr = X_NULL;
    }

    xmheap_ptr->xsize_cached -= xarray_ptr->xarray_size;

    xmheap_array_list_erase(xmheap_ptr, xarray_ptr);
    xsys_heap_free(xarray_ptr, xarray_ptr->xarray_size);
}

/**********************************************************/
/**
 * @brief 释放 堆数组区块 链表中空闲的 堆数组区块对象。
 */
static x_void_t xmheap_free_unused_array(xmheap_handle_t xmheap_ptr)
{
    xarray_ctxptr_t xarray_tmp = X_NULL;
    xarray_ctxptr_t xarray_ptr = XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr);
    while (xarray_ptr != XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr))
    {
        if (XSLICE_QUEUE_IS_FULL(xarray_ptr, x_uint32_t))
        {
            xarray_tmp = xarray_ptr->xlist_node.xarray_next;
            xmheap_free_array(xmheap_ptr, xarray_ptr);
            xarray_ptr = xarray_tmp;
        }
        else
        {
            xarray_ptr = xarray_ptr->xlist_node.xarray_next;
        }
    }
}

/**********************************************************/
/**
 * @brief 从管理的 堆内存区块 链表中申请 内存块。
 */
static xchunk_memptr_t xmheap_alloc_chunk(
                            xmheap_handle_t xmheap_ptr,
                            x_uint32_t xchunk_size,
                            xblock_handle_t * xblock_pptr)
{
    XASSERT(X_NULL != xblock_pptr);
    XASSERT(X_ALIGN(xchunk_size, XMHEAP_PAGE_SIZE) == xchunk_size);

    xchunk_memptr_t xchunk_ptr  = X_NULL;
    xblock_handle_t xblock_iptr = XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr);
    xblock_handle_t xblock_eptr = X_NULL;

    x_uint32_t xblock_iter = 0;
    x_uint32_t xblock_size = xmheap_ptr->xsize_block;

    //======================================
    // 先从已有的（链表中的）堆内存区块 分配 内存块

    while ((xblock_iter++ < XMHEAP_BLOCK_LIST_SIZE(xmheap_ptr)) &&
           (xblock_iptr != XMHEAP_BLOCK_LIST_TAIL(xmheap_ptr)))
    {
        xchunk_ptr = xblock_alloc_chunk(xblock_iptr, xchunk_size);
        if (X_NULL != xchunk_ptr)
        {
            *xblock_pptr = xblock_iptr;
            return xchunk_ptr;
        }

        if ((xblock_iptr->xmpage_rems <= 0) &&
            (xblock_iptr != XMHEAP_BLOCK_LIST_BACK(xmheap_ptr)))
        {
            xblock_eptr = xblock_iptr->xlist_node.xblock_next;
            xmheap_block_list_erase(xmheap_ptr, xblock_iptr);
            xmheap_block_list_push_tail(xmheap_ptr, xblock_iptr);
            xblock_iptr = xblock_eptr;
        }
        else
        {
            xblock_iptr = xblock_iptr->xlist_node.xblock_next;
        }
    }

    //======================================
    // 申请新的 堆内存区块 来分配 内存块

    if ((xchunk_size >= (xmem_block_page_nums(xblock_size) * XMHEAP_PAGE_SIZE))
        ||
        ((xblock_size + xmheap_ptr->xsize_cached) > xmheap_ptr->xsize_ulimit))
    {
        xblock_size = sizeof(xmem_block_t) +
                      (((xchunk_size / XMHEAP_PAGE_SIZE) + 7) >> 3) +
                      xchunk_size;
        xblock_size = X_ALIGN(xblock_size, XMHEAP_PAGE_SIZE);
    }

    // 若超过上限值，则直接取消申请操作
    if ((xblock_size + xmheap_ptr->xsize_cached) > xmheap_ptr->xsize_ulimit)
    {
        return X_NULL;
    }

    *xblock_pptr = xmheap_alloc_block(xmheap_ptr, xblock_size);
    if (X_NULL == *xblock_pptr)
    {
        return X_NULL;
    }

    xchunk_ptr = xblock_alloc_chunk_N(*xblock_pptr, xchunk_size);
    XASSERT(X_NULL != xchunk_ptr);

    //======================================
    // 将新申请的 堆内存区块 加入到管理的链表中

    if ((XMHEAP_BLOCK_LIST_SIZE(xmheap_ptr) > 0) &&
        ((*xblock_pptr)->xmpage_rems <
         XMHEAP_BLOCK_LIST_FRONT(xmheap_ptr)->xmpage_rems))
    {
        xmheap_block_list_push_tail(xmheap_ptr, *xblock_pptr);
    }
    else
    {
        xmheap_block_list_push_head(xmheap_ptr, *xblock_pptr);
    }

    //======================================

    return xchunk_ptr;
}

/**********************************************************/
/**
 * @brief 申请 xchunk_context_t 对象。
 * 
 * @param [in ] xmheap_ptr  : 堆内存管理对象。
 * @param [in ] xchunk_size : 对应的内存块大小。
 * @param [in ] xchunk_ptr  : 指向对应的内存块地址。
 * @param [in ] xowner_ptr  : 持有该内存块的标识句柄。
 * @param [in ] xblock_ptr  : 内存块所属的 堆内存区块 对象。
 * 
 * @return xchunk_ctxptr_t
 *         - 成功，返回 xchunk_context_t 对象；
 *         - 失败，返回 X_NULL。
 */
static xchunk_ctxptr_t xmheap_alloc_cctxt(
                            xmheap_handle_t xmheap_ptr,
                            x_uint32_t xchunk_size,
                            xchunk_memptr_t xchunk_ptr,
                            xowner_handle_t xowner_ptr,
                            xblock_handle_t xblock_ptr)
{
    xchunk_ctxptr_t xcctxt_ptr = X_NULL;
    xarray_ctxptr_t xarray_ptr = X_NULL;

    //======================================

    if ((X_NULL != xmheap_ptr->xarray_cptr) &&
        XSLICE_QUEUE_NOT_EMPTY(xmheap_ptr->xarray_cptr))
    {
        xarray_ptr = xmheap_ptr->xarray_cptr;
    }
    else
    {
        for (xarray_ptr = XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr);
             xarray_ptr != XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr);
             xarray_ptr = xarray_ptr->xlist_node.xarray_next)
        {
            if (XSLICE_QUEUE_NOT_EMPTY(xarray_ptr))
            {
                if (xarray_ptr != XMHEAP_ARRAY_LIST_FRONT(xmheap_ptr))
                {
                    xmheap_array_list_erase(xmheap_ptr, xarray_ptr);
                    xmheap_array_list_push_head(xmheap_ptr, xarray_ptr);
                }

                break;
            }
        }

        if (XMHEAP_ARRAY_LIST_TAIL(xmheap_ptr) == xarray_ptr)
        {
            xarray_ptr = xmheap_alloc_array(xmheap_ptr, XARRAY_BLOCK_SIZE);
            XASSERT(X_NULL != xarray_ptr);
            xmheap_array_list_push_head(xmheap_ptr, xarray_ptr);
        }

        xmheap_ptr->xarray_cptr = xarray_ptr;
    }

    //======================================

    xcctxt_ptr = xarray_alloc_cctxt(xarray_ptr);
    XASSERT(X_NULL != xcctxt_ptr);
    xmem_clear(xcctxt_ptr, sizeof(xchunk_context_t));

    xcctxt_ptr->xchunk_size = xchunk_size;
    xcctxt_ptr->xchunk_ptr  = xchunk_ptr;
    xcctxt_ptr->xowner_ptr  = xowner_ptr;
    xcctxt_ptr->xblock_ptr  = xblock_ptr;
    xcctxt_ptr->xarray_ptr  = xarray_ptr;

    //======================================

    return xcctxt_ptr;
}

//====================================================================

// 
// xrbtree : internal calls or callbacks
// 

XRBTREE_CTYPE_API(xchunk_ctxptr_t, static, inline, cctxt)

#define XCCTXPTR_TCAST(xrbt_vkey) (*(xchunk_ctxptr_t *)(xrbt_vkey))
#define XCCTXPTR_LADDR(xrbt_vkey) (XCCTXT_LADDR(XCCTXPTR_TCAST(xrbt_vkey)))
#define XCCTXPTR_RADDR(xrbt_vkey) (XCCTXT_RADDR(XCCTXPTR_TCAST(xrbt_vkey)))

/**********************************************************/
/**
 * @brief 查找内存分片可能隶属的 xchunk_context_t 对象描述的内存块地址范围内。
 */
static xchunk_ctxptr_t xrbtree_hit_cctxt(
                            x_rbtree_ptr xthis_ptr,
                            xmem_slice_t xmem_ptr)
{
    xchunk_ctxptr_t  xcctxt_ptr = X_NULL;
    x_rbnode_iter    xiter_node = X_NULL;

    //======================================

    struct 
    {
        x_uint32_t      xchunk_size;
        xchunk_memptr_t xchunk_ptr;
    } xmhit_ctxt = { 1, (xchunk_memptr_t)xmem_ptr };

    xiter_node = xrbtree_find_cctxt(xthis_ptr, (xchunk_ctxptr_t)&xmhit_ctxt);
    if (xrbtree_iter_is_nil(xiter_node))
    {
        return X_NULL;
    }

    //======================================

    xcctxt_ptr = xrbtree_iter_cctxt(xiter_node);

    XASSERT((X_NULL != xcctxt_ptr) && (X_NULL != xcctxt_ptr->xchunk_ptr));
    XASSERT((xmem_ptr >= XCCTXT_LADDR(xcctxt_ptr)) &&
            (xmem_ptr <  XCCTXT_RADDR(xcctxt_ptr)));

    //======================================

    return xcctxt_ptr;
}

/**********************************************************/
/**
 * @brief 红黑树 xmem_heap_t.xrbtree 申请节点对象缓存的回调函数。
 */
static xrbt_void_t * xrbtree_cctxt_node_alloc(
                            xrbt_vkey_t xrbt_vkey,
                            xrbt_size_t xst_nsize,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(xst_nsize <= XMHEAP_RBNODE_SIZE);
    return (xrbt_void_t *)(XCCTXPTR_TCAST(xrbt_vkey)->xtree_node.xbt_ptr);
}

/**********************************************************/
/**
 * @brief 红黑树释放节点对象缓存的回调函数。
 */
static xrbt_void_t xrbtree_cctxt_node_free(
                            x_rbnode_iter xiter_node,
                            xrbt_size_t xnode_size,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(xnode_size <= XMHEAP_RBNODE_SIZE);

    xchunk_ctxptr_t xcctxt_ptr = xrbtree_iter_cctxt(xiter_node);
    xmheap_handle_t xmheap_ptr = (xmheap_handle_t)xrbt_ctxt;

    xarray_recyc_cctxt(xcctxt_ptr->xarray_ptr, xcctxt_ptr);
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
static xrbt_void_t xrbtree_cctxt_copyfrom(
                            xrbt_vkey_t xrbt_dkey,
                            xrbt_vkey_t xrbt_skey,
                            xrbt_size_t xrbt_size,
                            xrbt_bool_t xbt_move ,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(sizeof(xchunk_ctxptr_t) == xrbt_size);
    XCCTXPTR_TCAST(xrbt_dkey) = XCCTXPTR_TCAST(xrbt_skey);
}

/**********************************************************/
/**
 * @brief 红黑树析构节点对象的索引键值的回调函数。
 *
 * @param [out] xrbt_vkey : 索引键缓存。
 * @param [in ] xrbt_size : 索引键缓存大小。
 * @param [in ] xrbt_ctxt : 回调的上下文标识。
 */
static xrbt_void_t xrbtree_cctxt_destruct(
                            xrbt_vkey_t xrbt_vkey,
                            xrbt_size_t xrbt_size,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(sizeof(xchunk_ctxptr_t) == xrbt_size);

    XASSERT_CHECK(
        XMEM_ERR_OK !=
            xblock_recyc_chunk(XCCTXPTR_TCAST(xrbt_vkey)->xblock_ptr,
                               XCCTXPTR_TCAST(xrbt_vkey)->xchunk_ptr,
                               XCCTXPTR_TCAST(xrbt_vkey)->xchunk_size),
        X_FALSE);
}

/**********************************************************/
/**
 * @brief 服务于 xmem_heap_t.xrbtree_alloc 节点索引键值比较的回调函数。
 */
static xrbt_bool_t xrbtree_cctxt_compare(
                            xrbt_vkey_t xrbt_lkey,
                            xrbt_vkey_t xrbt_rkey,
                            xrbt_size_t xrbt_size,
                            xrbt_ctxt_t xrbt_ctxt)
{
    XASSERT(sizeof(xchunk_ctxptr_t) == xrbt_size);
    return (XCCTXPTR_RADDR(xrbt_lkey) <= XCCTXPTR_LADDR(xrbt_rkey));
}

//====================================================================

// 
// xmem_heap_t : public interfaces
// 

/**********************************************************/
/**
 * @brief 创建堆内存管理对象。
 * @note
 * 注意：
 *  - 所有 size 参数都按内存分页大小对齐。
 *  - xsize_block 最少值为 2M 。
 *  - xsize_ulimit 至少是 xsize_block 的 2 倍。
 *
 * @param [in ] xsize_block  : 申请单个堆内存区块的建议大小。
 * @param [in ] xsize_ulimit : 可申请堆内存大小的总和上限。
 *
 * @return xmheap_handle_t
 *         - 堆内存管理对象。
 */
xmheap_handle_t xmheap_create(x_uint32_t xsize_block, x_uint64_t xsize_ulimit)
{
    XASSERT(xsize_block  >= (512 * XMHEAP_PAGE_SIZE));
    XASSERT(xsize_ulimit >= (x_uint64_t)(2 * xsize_block));

    //======================================

    xrbt_callback_t xcallback =
    {
        /* .xfunc_n_memalloc = */ &xrbtree_cctxt_node_alloc,
        /* .xfunc_n_memfree  = */ &xrbtree_cctxt_node_free ,
        /* .xfunc_k_copyfrom = */ &xrbtree_cctxt_copyfrom  ,
        /* .xfunc_k_destruct = */ &xrbtree_cctxt_destruct  ,
        /* .xfunc_k_lesscomp = */ &xrbtree_cctxt_compare   ,
        /* .xctxt_t_callback = */ XRBT_NULL
    };

    //======================================

    xmheap_handle_t xmheap_ptr = xmem_alloc(sizeof(xmem_heap_t));
    XASSERT(X_NULL != xmheap_ptr);

    xmem_clear(xmheap_ptr, sizeof(xmem_heap_t));

    //======================================

    xmheap_block_list_init(xmheap_ptr);
    xmheap_array_list_init(xmheap_ptr);

    xmheap_ptr->xarray_cptr = X_NULL;

    xrbtree_emplace_create(XMHEAP_RBTREE(xmheap_ptr),
                           sizeof(xchunk_ctxptr_t),
                           &xcallback);

    //======================================

    if (xsize_block < (512 * XMHEAP_PAGE_SIZE))
    {
        xsize_block = (512 * XMHEAP_PAGE_SIZE);
    }

    xmheap_ptr->xsize_block  = X_ALIGN(xsize_block , XMHEAP_PAGE_SIZE);
    xmheap_ptr->xsize_ulimit = X_ALIGN(xsize_ulimit, XMHEAP_PAGE_SIZE);

    if (xmheap_ptr->xsize_ulimit < (x_uint64_t)(2 * xmheap_ptr->xsize_block))
    {
        xmheap_ptr->xsize_ulimit = (x_uint64_t)(2 * xmheap_ptr->xsize_block);
    }

    //======================================

    return xmheap_ptr;
}

/**********************************************************/
/**
 * @brief 销毁堆内存管理对象。
 */
x_void_t xmheap_destroy(xmheap_handle_t xmheap_ptr)
{
    XASSERT(X_NULL != xmheap_ptr);
    xatomic_spin_lock(&xmheap_ptr->xmheap_lock);

    //======================================

    xrbtree_emplace_destroy(XMHEAP_RBTREE(xmheap_ptr));
    xmheap_array_list_release(xmheap_ptr);
    xmheap_block_list_release(xmheap_ptr);

    //======================================

    xatomic_spin_unlock(&xmheap_ptr->xmheap_lock);
    xmem_free(xmheap_ptr);
}

/**********************************************************/
/**
 * @brief 堆内存管理对象 总共缓存的内存大小。
 */
x_uint64_t xmheap_cached_size(xmheap_handle_t xmheap_ptr)
{
    XASSERT(X_NULL != xmheap_ptr);
    return xmheap_ptr->xsize_cached;
}

/**********************************************************/
/**
 * @brief 堆内存管理对象 可使用到的缓存大小。
 */
x_uint64_t xmheap_valid_size(xmheap_handle_t xmheap_ptr)
{
    XASSERT(X_NULL != xmheap_ptr);
    return xmheap_ptr->xsize_valid;
}

/**********************************************************/
/**
 * @brief 堆内存管理对象 正在使用的缓存大小。
 */
x_uint64_t xmheap_using_size(xmheap_handle_t xmheap_ptr)
{
    XASSERT(X_NULL != xmheap_ptr);
    return xmheap_ptr->xsize_using;
}

/**********************************************************/
/**
 * @brief 申请内存块。
 * 
 * @param [in ] xmheap_ptr  : 堆内存管理 对象的操作句柄。
 * @param [in ] xchunk_size : 请求的内存块大小。
 * @param [in ] xowner_ptr  : 持有该（返回的）内存块的标识句柄。
 * 
 * @return xchunk_memptr_t
 *         - 成功，返回 内存块；
 *         - 失败，返回 X_NULL。
 */
xchunk_memptr_t xmheap_alloc(xmheap_handle_t xmheap_ptr,
                             x_uint32_t xchunk_size,
                             xowner_handle_t xowner_ptr)
{
    XASSERT(X_NULL != xmheap_ptr);

    xchunk_memptr_t xchunk_ptr = X_NULL;
    xblock_handle_t xblock_ptr = X_NULL;
    xchunk_ctxptr_t xcctxt_ptr = X_NULL;

    if (0 == xchunk_size)
        return X_NULL;
    xchunk_size = X_ALIGN(xchunk_size, XMHEAP_PAGE_SIZE);

    xatomic_spin_lock(&xmheap_ptr->xmheap_lock);

    //======================================

    xchunk_ptr = xmheap_alloc_chunk(xmheap_ptr, xchunk_size, &xblock_ptr);
    if (X_NULL != xchunk_ptr)
    {
        xcctxt_ptr = xmheap_alloc_cctxt(xmheap_ptr,
                                        xchunk_size,
                                        xchunk_ptr,
                                        xowner_ptr,
                                        xblock_ptr);
        XASSERT(X_NULL != xcctxt_ptr);

        xmheap_ptr->xsize_using += xchunk_size;

        if (!xrbtree_insert_cctxt(XMHEAP_RBTREE(xmheap_ptr), xcctxt_ptr))
        {
            XASSERT(X_FALSE);

            xmheap_ptr->xsize_using -= xchunk_size;

            xblock_recyc_chunk(xblock_ptr, xchunk_ptr, xchunk_size);
            xchunk_ptr = X_NULL;
        }
    }

    //======================================

    xatomic_spin_unlock(&xmheap_ptr->xmheap_lock);

    return xchunk_ptr;
}

/**********************************************************/
/**
 * @brief 回收内存块。
 * 
 * @param [in ] xmheap_ptr  : 堆内存管理 对象的操作句柄。
 * @param [in ] xchunk_ptr  : 待释放的内存块。
 * 
 * @return x_int32_t
 *         - 成功，返回 XMEM_ERR_OK；
 *         - 失败，返回 错误码。
 */
x_int32_t xmheap_recyc(xmheap_handle_t xmheap_ptr,
                       xchunk_memptr_t xchunk_ptr)
{
    XASSERT(X_NULL != xmheap_ptr);
    XASSERT(X_NULL != xchunk_ptr);

    x_int32_t       xit_error  = XMEM_ERR_UNKNOW;
    xchunk_ctxptr_t xcctxt_ptr = X_NULL;
    x_rbnode_iter   xiter_node = X_NULL;

    xatomic_spin_lock(&xmheap_ptr->xmheap_lock);

    //======================================

    do
    {
        xcctxt_ptr = xrbtree_hit_cctxt(
                        XMHEAP_RBTREE(xmheap_ptr), (xmem_slice_t)xchunk_ptr);
        if (X_NULL == xcctxt_ptr)
        {
            xit_error = XMEM_ERR_NOT_FOUND;
            break;
        }

        if (xcctxt_ptr->xchunk_ptr != xchunk_ptr)
        {
            xit_error = XMEM_ERR_UNALIGNED;
            break;
        }

        xmheap_ptr->xsize_using -= xcctxt_ptr->xchunk_size;

        // 从红黑树中删除对应的节点
        xiter_node = (x_rbnode_iter)xcctxt_ptr->xtree_node.xbt_ptr;
        XASSERT(xrbtree_iter_cctxt(xiter_node) == xcctxt_ptr);
        xrbtree_erase(XMHEAP_RBTREE(xmheap_ptr), xiter_node);

        // 若当前缓存的堆内存总和大于 上限值 的一半，
        // 则释放 空闲的 堆内存区块 和 堆数组区块
        if (xmheap_ptr->xsize_cached >= (xmheap_ptr->xsize_ulimit / 2))
        {
            xmheap_free_unused_block(xmheap_ptr);
            xmheap_free_unused_array(xmheap_ptr);
        }

        xit_error = XMEM_ERR_OK;
    } while (0);

    //======================================

    xatomic_spin_unlock(&xmheap_ptr->xmheap_lock);

    return xit_error;
}

/**********************************************************/
/**
 * @brief 释放未使用的堆缓存块。
 */
x_void_t xmheap_release_unused(xmheap_handle_t xmheap_ptr)
{
    XASSERT(X_NULL != xmheap_ptr);

    xatomic_spin_lock(&xmheap_ptr->xmheap_lock);

    //======================================

    xmheap_free_unused_block(xmheap_ptr);
    xmheap_free_unused_array(xmheap_ptr);

    //======================================

    xatomic_spin_unlock(&xmheap_ptr->xmheap_lock);
}

/**********************************************************/
/**
 * @brief 使用内存分片 HIT 测试操作，查询其所在的 chunk 快照信息。
 *
 * @param [in ] xmheap_ptr : 堆内存管理对象。
 * @param [in ] xslice_ptr : HIT 测试的内存分片。
 * @param [out] xshoot_ptr : 操作成功返回的 chunk 快照信息。
 *
 * @return x_int32_t
 *         - 成功，返回 XMEM_ERR_OK；
 *         - 失败，返回 错误码。
 */
x_int32_t xmheap_hit_chunk(xmheap_handle_t xmheap_ptr,
                           xmem_slice_t xslice_ptr,
                           xchunk_snapshoot_t * xshoot_ptr)
{
    XASSERT(X_NULL != xmheap_ptr);
    XASSERT(X_NULL != xslice_ptr);

    x_int32_t       xit_error = XMEM_ERR_UNKNOW;
    xchunk_ctxptr_t xcctxt_ptr = X_NULL;
    x_rbnode_iter   xiter_node = X_NULL;

    xatomic_spin_lock(&xmheap_ptr->xmheap_lock);

    //======================================

    do
    {
        xcctxt_ptr = xrbtree_hit_cctxt(XMHEAP_RBTREE(xmheap_ptr), xslice_ptr);
        if (X_NULL == xcctxt_ptr)
        {
            xit_error = XMEM_ERR_NOT_FOUND;
            break;
        }

        if (X_NULL != xshoot_ptr)
        {
            xshoot_ptr->xchunk_size = xcctxt_ptr->xchunk_size;
            xshoot_ptr->xchunk_ptr  = xcctxt_ptr->xchunk_ptr;
            xshoot_ptr->xowner_ptr  = xcctxt_ptr->xowner_ptr;
        }

        xit_error = XMEM_ERR_OK;
    } while (0);

    //======================================

    xatomic_spin_unlock(&xmheap_ptr->xmheap_lock);

    return xit_error;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC__

////////////////////////////////////////////////////////////////////////////////
