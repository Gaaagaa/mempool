/**
 * @file    xmem_comm.h
 * <pre>
 * Copyright (c) 2019, Gaaagaa All rights reserved.
 * 
 * 文件名称：xmem_comm.h
 * 创建日期：2019年09月22日
 * 文件标识：
 * 文件摘要：公共数据声明与定义。
 * 
 * 当前版本：1.0.0.0
 * 作    者：
 * 完成日期：2019年09月22日
 * 版本摘要：
 * 
 * 历史版本：
 * 原作者  ：
 * 完成日期：
 * 版本摘要：
 * </pre>
 */

#ifndef __XMEM_COMM_H__
#define __XMEM_COMM_H__

#include "xtypes.h"

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

#define XMEM_PAGE_SIZE  (4 * 1024)

/** 按 (align = 2^n) 的倍数对齐 size */
#define X_ALIGN(size, align) (((size) + ((align) - 1)) & (~((align) - 1)))

////////////////////////////////////////////////////////////////////////////////

/**
 * @enum  xmem_err_code
 * @brief 内存相关的操作错误码。
 */
typedef enum xmem_err_code
{
    XMEM_ERR_OK              = 0x00000000, ///< 表示成功
    XMEM_ERR_UNKNOW          = 0xFFFFFFFF, ///< 未知错误

    XMEM_ERR_SLICE_NOT_FOUND = 0x00011010, ///< 内存分片在找不到
    XMEM_ERR_SLICE_UNALIGNED = 0x00011020, ///< 内存分片在分块中的地址未对齐
    XMEM_ERR_SLICE_RECYCLED  = 0x00011030, ///< 内存分片已经被回收
} xmem_err_code;

////////////////////////////////////////////////////////////////////////////////

/** 内存分片类型 */
typedef x_byte_t *  xmem_slice_t;

/** 定义分片队列的内嵌结构体变量 */
#define XSLICE_QUEUE_DEFINED(__size_type) \
    struct                                \
    {                                     \
        __size_type xut_offset;           \
        __size_type xut_capacity;         \
        __size_type xut_bpos;             \
        __size_type xut_epos;             \
        __size_type xut_index[0];         \
    } xslice_queue                        \

/** 分片的索引号的位掩码值 */
#define XSLICE_IMASK(__size_type) ((__size_type)(((__size_type)(-1)) >> 1))

/** 分片的分配标识的位掩码值 */
#define XSLICE_AMASK(__size_type) ((__size_type)(XSLICE_IMASK(__size_type) + 1))

/** 分片大小 */
#define XSLICE_MSIZE(xmsque_ptr)  ((xmsque_ptr)->xslice_size)

/** 分片队列 */
#define XSLICE_QUEUE(xmsque_ptr)  ((xmsque_ptr)->xslice_queue)

/** 分片队列容量 */
#define XSLICE_QUEUE_CAPACITY(xmsque_ptr) (XSLICE_QUEUE(xmsque_ptr).xut_capacity)

/** 未被分配出去的分片数量 */
#define XSLICE_QUEUE_COUNT(xmsque_ptr, __size_type)    \
    ((__size_type)(XSLICE_QUEUE(xmsque_ptr).xut_epos - \
                   XSLICE_QUEUE(xmsque_ptr).xut_bpos)) \

/** 分片队列是否已空，即所有分片都已经被分配出去 */
#define XSLICE_QUEUE_IS_EMPTY(xmsque_ptr) \
    (XSLICE_QUEUE(xmsque_ptr).xut_epos == XSLICE_QUEUE(xmsque_ptr).xut_bpos)

/** 分片队列是否已满，即没有任何一个分片被分配出去 */
#define XSLICE_QUEUE_IS_FULL(xmsque_ptr, __size_type) \
    (XSLICE_QUEUE_COUNT(xmsque_ptr, __size_type) == \
        XSLICE_QUEUE_CAPACITY(xmsque_ptr))

/** chunk 对象中的分片索引号位置 */
#define XSLICE_QUEUE_INDEX(xmsque_ptr, xut_pos)         \
    (XSLICE_QUEUE(xmsque_ptr).xut_index[                \
        (xut_pos) % XSLICE_QUEUE_CAPACITY(xmsque_ptr)]) \

/** 获取分片索引号值 */
#define XSLICE_QUEUE_INDEX_GET(xmsque_ptr, xut_pos, __size_type) \
    (XSLICE_QUEUE_INDEX(xmsque_ptr, xut_pos) & XSLICE_IMASK(__size_type))

/** 设置分片索引号值 */
#define XSLICE_QUEUE_INDEX_SET(xmsque_ptr, xut_pos, xut_index, __size_type) \
    (XSLICE_QUEUE_INDEX(xmsque_ptr, xut_pos) =                              \
        ((XSLICE_QUEUE_INDEX(xmsque_ptr, xut_pos) &                         \
            XSLICE_AMASK(__size_type)) |                                    \
                (((__size_type)(xut_index)) & XSLICE_IMASK(__size_type))))  \

/** 判断分片是否已被分配出去 */
#define XSLICE_QUEUE_IS_ALLOCATED(xmsque_ptr, xut_index, __size_type) \
    ((XSLICE_QUEUE_INDEX(xmsque_ptr, xut_index) &                     \
        XSLICE_AMASK(__size_type)) == XSLICE_AMASK(__size_type))      \

/** 标识分片已经被分配出去 */
#define XSLICE_QUEUE_ALLOCATED_SET(xmsque_ptr, xut_index, __size_type) \
    (XSLICE_QUEUE_INDEX(xmsque_ptr, xut_index) |= XSLICE_AMASK(__size_type))

/** 标识分片未被分配出去 */
#define XSLICE_QUEUE_ALLOCATED_RESET(xmsque_ptr, xut_index, __size_type) \
    (XSLICE_QUEUE_INDEX(xmsque_ptr, xut_index) &= XSLICE_IMASK(__size_type))

/** 分片起始地址 */
#define XSLICE_QUEUE_BEGIN(xmsque_ptr) \
    ((xmem_slice_t)(xmsque_ptr) + XSLICE_QUEUE(xmsque_ptr).xut_offset)

/** 获取分片地址 */
#define XSLICE_QUEUE_GET(xmsque_ptr, xut_index)  \
    (XSLICE_QUEUE_BEGIN(xmsque_ptr) + ((xut_index)) * XSLICE_MSIZE(xmsque_ptr))

/** 分片结束地址 */
#define XSLICE_QUEUE_END(xmsque_ptr) \
    (XSLICE_QUEUE_GET(xmsque_ptr, XSLICE_QUEUE_CAPACITY(xmsque_ptr)))

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

////////////////////////////////////////////////////////////////////////////////

#include "xmem_pool.h"

////////////////////////////////////////////////////////////////////////////////

#endif // __XMEM_COMM_H__
