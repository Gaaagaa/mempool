/**
 * @file    xmem_heap.h
 * <pre>
 * Copyright (c) 2019, Gaaagaa All rights reserved.
 * 
 * 文件名称：xmem_heap.h
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

#ifndef __XMEM_HEAP_H__
#define __XMEM_HEAP_H__

#ifndef __XMEM_COMM_H__
#error "Please include xmem_comm.h"
#endif // __XMEM_COMM_H__

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

/** 堆内存块的类型定义 */
typedef xmem_handle_t xchunk_memptr_t;

/** 持有者对象句柄 */
typedef xmem_handle_t xowner_handle_t;

/** 堆内存管理的结构体声明 */
struct xmem_heap_t;

/** 堆内存管理的操作句柄类型定义 */
typedef struct xmem_heap_t * xmheap_handle_t;

/**
 * @struct xchunk_snapshoot_t
 * @brief 从堆内存管理中分配出去的内存块快照信息。
 * @note 此结构体用于查询操作。
 */
typedef struct xchunk_snapshoot_t
{
    x_uint32_t       xchunk_size;  ///< 对应的 chunk 大小
    xchunk_memptr_t  xchunk_ptr;   ///< 指向对应的 chunk 地址
    xowner_handle_t  xowner_ptr;   ///< 持有该 chunk 的标识句柄
} xchunk_snapshoot_t;

////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 创建堆内存管理对象。
 * @note 注意，xsize_ulimit 至少是 xsize_block 的 2 倍，且都是按内存分页大小对齐。
 * 
 * @param [in ] xsize_block  : 申请单个堆内存区块的建议大小。
 * @param [in ] xsize_ulimit : 可申请堆内存大小的总和上限。
 * 
 * @return xmheap_handle_t
 *         - 堆内存管理对象。
 */
xmheap_handle_t xmheap_create(x_uint32_t xsize_block, x_uint64_t xsize_ulimit);

/**********************************************************/
/**
 * @brief 销毁堆内存管理对象。
 */
x_void_t xmheap_destroy(xmheap_handle_t xmheap_ptr);

/**********************************************************/
/**
 * @brief 堆内存管理对象 总共缓存的内存大小。
 */
x_uint64_t xmheap_cached_size(xmheap_handle_t xmheap_ptr);

/**********************************************************/
/**
 * @brief 堆内存管理对象 可使用到的缓存大小。
 */
x_uint64_t xmheap_valid_size(xmheap_handle_t xmheap_ptr);

/**********************************************************/
/**
 * @brief 堆内存管理对象 正在使用的缓存大小。
 */
x_uint64_t xmheap_using_size(xmheap_handle_t xmheap_ptr);

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
                             xowner_handle_t xowner_ptr);

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
                       xchunk_memptr_t xchunk_ptr);

/**********************************************************/
/**
 * @brief 释放未使用的堆缓存块。
 */
x_void_t xmheap_release_unused(xmheap_handle_t xmheap_ptr);

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
                           xchunk_snapshoot_t * xshoot_ptr);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __XMEM_HEAP_H__
