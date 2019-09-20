/**
 * @file    xmem_pool.h
 * <pre>
 * Copyright (c) 2019, Gaaagaa All rights reserved.
 * 
 * 文件名称：xmem_pool.h
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

#ifndef __XMEM_POOL_H__
#define __XMEM_POOL_H__

#include "xtypes.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

/**
 * @enum  xmem_err_code
 * @brief 内存池相关的操作错误码。
 */
typedef enum xmem_err_code
{
    XMEM_ERR_OK              = 0x00000000, ///< 表示成功
    XMEM_ERR_UNKNOW          = 0xFFFFFFFF, ///< 未知错误

    XMEM_ERR_SLICE_NOT_FOUND = 0x00011010, ///< 内存分片在池中找不到
    XMEM_ERR_SLICE_UNALIGNED = 0x00011020, ///< 内存分片在分块中的地址未对齐
    XMEM_ERR_SLICE_RECYCLED  = 0x00011030, ///< 内存分片已经被回收
} xmem_err_code;

/**
 * @brief 执行堆内存块申请的函数类型。
 * 
 * @param [in ] xst_size    : 请求的堆内存块大小。
 * @param [in ] xht_context : 回调的上下文标识句柄。
 * 
 * @return x_void_t *
 *         - 成功，返回 堆内存块 地址；
 *         - 失败，返回 X_NULL。
 */
typedef x_void_t * (* xfunc_alloc_t)(x_size_t xst_size,
                                     x_handle_t xht_context);

/**
 * @brief 执行堆内存块释放的函数类型。
 * 
 * @param [in ] xmt_heap    : 释放的堆内存块。
 * @param [in ] xst_size    : 释放的堆内存块大小。
 * @param [in ] xht_context : 回调的上下文标识句柄。
 * 
 */
typedef x_void_t (* xfunc_free_t)(x_void_t * xmt_heap,
                                  x_size_t xst_size,
                                  x_handle_t xht_context);

/** 内存分片类型 */
typedef x_byte_t *  xmem_slice_t;

/** 内存池对象的结构体声明 */
struct xmem_pool_t;

/** 内存池对象操作句柄的类型声明 */
typedef struct xmem_pool_t * xmpool_handle_t;

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 按所需的（内存块）长度值，计算内存对齐后的数值。
 */
x_uint32_t xmem_align_size(x_uint32_t xut_size);

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
xmpool_handle_t xmpool_create(
    xfunc_alloc_t xfunc_alloc, xfunc_free_t xfunc_free, x_handle_t xht_context);

/**********************************************************/
/**
 * @brief 销毁内存池对象。
 */
x_void_t xmpool_destroy(xmpool_handle_t xmpool_ptr);

/**********************************************************/
/**
 * @brief 内存池对象 总共缓存的内存大小。
 */
x_uint64_t xmpool_cached_size(xmpool_handle_t xmpool_ptr);

/**********************************************************/
/**
 * @brief 内存池对象 可使用到的缓存大小。
 */
x_uint64_t xmpool_valid_size(xmpool_handle_t xmpool_ptr);

/**********************************************************/
/**
 * @brief 内存池对象 正在使用的缓存大小。
 */
x_uint64_t xmpool_using_size(xmpool_handle_t xmpool_ptr);

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
xmem_slice_t xmpool_alloc(xmpool_handle_t xmpool_ptr, x_uint32_t xut_size);

/**********************************************************/
/**
 * @brief 回收内存分片。
 * 
 * @param [in ] xmpool_ptr : 内存池对象的操作句柄。
 * @param [in ] xmem_slice : 待回收的内存分片。
 * 
 * @return x_int32_t
 *         - 成功，返回 XMEM_ERR_OK；
 *         - 失败，返回 错误码（参看 @see xmem_err_code 枚举值）。
 */
x_int32_t xmpool_recyc(xmpool_handle_t xmpool_ptr, xmem_slice_t xmem_slice);

/**********************************************************/
/**
 * @brief 释放内存池中未使用的缓存块。
 */
x_void_t xmpool_release_unused(xmpool_handle_t xmpool_ptr);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __XMEM_POOL_H__
