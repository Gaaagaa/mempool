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
#include "xmem_pool.h"

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

#endif // __XMEM_COMM_H__
