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

////////////////////////////////////////////////////////////////////////////////

/** 堆内存块的类型定义 */
typedef x_void_t * xmheap_chunk_t;

/** 堆内存管理的结构体声明 */
struct xmem_heap_t;

/** 堆内存管理的操作句柄类型定义 */
typedef struct xmem_heap_t * xmheap_handle_t;

/////////////////////////////////////////////////////////////////////////////////

/**********************************************************/
/**
 * @brief 创建堆内存管理对象。
 */
xmheap_handle_t xmheap_create(void);

/**********************************************************/
/**
 * @brief 销毁堆内存管理对象。
 */
x_void_t xmheap_destroy(xmheap_handle_t xmheap_ptr);

/**********************************************************/
/**
 * @brief 申请堆内存块。
 * 
 * @param [in ] xmheap_ptr   : 堆内存管理 对象的操作句柄。
 * @param [in ] xchunk_size  : 请求的堆内存块大小。
 * @param [in ] xchunk_owner : 持有该（返回的）堆内存块的标识句柄。
 * 
 * @return xmheap_chunk_t
 *         - 成功，返回 堆内存块；
 *         - 失败，返回 X_NULL。
 */
xmheap_chunk_t xmheap_alloc(xmheap_handle_t xmheap_ptr,
                            x_uint32_t xchunk_size,
                            x_handle_t xchunk_owner);

/**********************************************************/
/**
 * @brief 回收堆内存块。
 * 
 * @param [in ] xmheap_ptr   : 堆内存管理 对象的操作句柄。
 * @param [in ] xchunk_ptr   : 待释放的堆内存块。
 * @param [in ] xchunk_size  : 待释放的堆内存块大小。
 * @param [in ] xchunk_owner : 持有该堆内存块的标识句柄。
 */
x_void_t xmheap_recyc(xmheap_handle_t xmheap_ptr,
                      xmheap_chunk_t xchunk_ptr,
                      x_uint32_t xchunk_size,
                      x_handle_t xchunk_owner);

////////////////////////////////////////////////////////////////////////////////

#endif // __XMEM_HEAP_H__
