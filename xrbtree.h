/**
 * @file    xrbtree.h
 * <pre>
 * Copyright (c) 2019, Gaaagaa All rights reserved.
 * 
 * 文件名称：xrbtree.h
 * 创建日期：2019年09月09日
 * 文件标识：
 * 文件摘要：红黑树的相关数据定义以及操作接口。
 * 
 * 当前版本：1.0.0.0
 * 作    者：
 * 完成日期：2019年09月09日
 * 版本摘要：
 * 
 * 历史版本：
 * 原作者  ：
 * 完成日期：
 * 版本摘要：
 * </pre>
 */

#ifndef __XRBTREE_H__
#define __XRBTREE_H__

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// 红黑树的相关数据定义以及操作接口

//====================================================================

// 
// 红黑树的相关数据定义
// 

#define XRBT_FALSE  0
#define XRBT_TRUE   1
#define XRBT_NULL   0

typedef void           xrbt_void_t;
typedef unsigned char  xrbt_byte_t;
typedef int            xrbt_int32_t;
typedef unsigned int   xrbt_uint32_t;
typedef unsigned int   xrbt_size_t;
typedef unsigned int   xrbt_bool_t;
typedef void *         xrbt_vkey_t;
typedef void *         xrbt_ctxt_t;

/** 声明红黑树所使用的节点结构体 */
struct x_rbtree_node_t;

/** 声明红黑树所使用的节点迭代器 */
typedef struct x_rbtree_node_t * x_rbnode_iter;

/** 声明红黑树结构体 */
struct x_rbtree_t;

/** 声明红黑树对象指针 */
typedef struct x_rbtree_t * x_rbtree_ptr;

/**
 * @brief 申请节点对象缓存的回调函数类型。
 *
 * @param [in ] xrbt_vkey : 请求申请缓存的节点索引键（插入操作时回调回来的索引键）。
 * @param [in ] xst_nsize : 节点对象所需缓存的大小（即 请求申请缓存的大小）。
 * @param [in ] xrbt_ctxt : 回调的上下文标识。
 * 
 * @return xrbt_void_t *
 *         - 节点对象缓存。
 */
typedef xrbt_void_t * (* xfunc_node_memalloc_t)(
                            xrbt_vkey_t xrbt_vkey,
                            xrbt_size_t xst_nsize,
                            xrbt_ctxt_t xrbt_ctxt);

/**
 * @brief 释放节点对象缓存的回调函数类型。
 *
 * @param [in ] xiter_node : 待释放的节点对象缓存。
 * @param [in ] xst_nsize  : 节点对象缓存的大小。
 * @param [in ] xrbt_ctxt  : 回调的上下文标识。
 */
typedef xrbt_void_t (* xfunc_node_memfree_t)(
                            x_rbnode_iter xiter_node,
                            xrbt_size_t xst_nsize,
                            xrbt_ctxt_t xrbt_ctxt);

/**
 * @brief 拷贝节点对象的索引键值的回调函数类型。
 *
 * @param [out] xrbt_dkey : 目标的索引键缓存。
 * @param [in ] xrbt_skey : 源数据的索引键缓存。
 * @param [in ] xrbt_size : 索引键缓存大小。
 * @param [in ] xbt_move  : 是否采用右值 move 操作进行数据拷贝（用于兼容 C++11 后的对象右值引用操作）。
 * @param [in ] xrbt_ctxt : 回调的上下文标识。
 */
typedef xrbt_void_t (* xfunc_vkey_copyfrom_t)(
                            xrbt_vkey_t xrbt_dkey,
                            xrbt_vkey_t xrbt_skey,
                            xrbt_size_t xrbt_size,
                            xrbt_bool_t xbt_move ,
                            xrbt_ctxt_t xrbt_ctxt);

/**
 * @brief 析构节点对象的索引键值的回调函数类型。
 *
 * @param [out] xrbt_vkey : 索引键缓存。
 * @param [in ] xrbt_size : 索引键缓存大小。
 * @param [in ] xrbt_ctxt : 回调的上下文标识。
 */
typedef xrbt_void_t (* xfunc_vkey_destruct_t)(
                            xrbt_vkey_t xrbt_vkey,
                            xrbt_size_t xrbt_size,
                            xrbt_ctxt_t xrbt_ctxt);

/**
 * @brief 比较节点索引键值的回调函数类型。
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
typedef xrbt_bool_t (* xfunc_vkey_compare_t)(
                            xrbt_vkey_t xrbt_lkey,
                            xrbt_vkey_t xrbt_rkey,
                            xrbt_size_t xrbt_size,
                            xrbt_ctxt_t xrbt_ctxt);

/**
 * @struct x_rbtree_node_callback_t
 * @brief  红黑树节点的 相关回调函数接口 的结构体描述信息。
 */
typedef struct x_rbtree_node_callback_t
{
    xfunc_node_memalloc_t xfunc_n_memalloc; ///< 申请节点对象缓存的回调接口
    xfunc_node_memfree_t  xfunc_n_memfree ; ///< 释放节点对象缓存的回调接口
    xfunc_vkey_copyfrom_t xfunc_k_copyfrom; ///< 拷贝节点对象的索引键值的回调操作接口
    xfunc_vkey_destruct_t xfunc_k_destruct; ///< 析构节点对象的索引键值的回调操作接口
    xfunc_vkey_compare_t  xfunc_k_compare ; ///< 比较节点索引键值的回调接口
    xrbt_ctxt_t           xctxt_t_callback; ///< 回调的上下文标识
} xrbt_callback_t;

//====================================================================

// 
// 红黑树的相关操作接口
// 

/**********************************************************/
/**
 * @brief 创建 x_rbtree_t 对象。
 * @note
 * xcallback 会在红黑树对象内部另有缓存保存所设置的参数，外部无须持续保留；
 * 若某个回调函数为 XRBT_NULL，则取内部默认值。
 * 
 * @param [in ] xst_ksize : 索引键数据类型所需的缓存大小（如 sizeof 值）。
 * @param [in ] xcallback : 节点操作的相关回调函数。
 * 
 * @return x_rbtree_ptr
 *         - 成功，返回 x_rbtree_t 对象；
 *         - 失败，返回 XRBT_NULL；
 */
x_rbtree_ptr xrbtree_create(xrbt_size_t xst_ksize, xrbt_callback_t * xcallback);

/**********************************************************/
/**
 * @brief 销毁 x_rbtree_t 对象。
 */
xrbt_void_t xrbtree_destroy(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 清除 x_rbtree_t 对象中的所有节点。
 */
xrbt_void_t xrbtree_clear(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 返回 x_rbtree_t 对象中的节点数量。
 */
xrbt_size_t xrbtree_size(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 判断 x_rbtree_t 对象中节点数量是否为空。
 */
xrbt_bool_t xrbtree_empty(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 返回 x_rbtree_t 对象中的左手臂长度。
 */
xrbt_size_t xrbtree_left_length(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 返回 x_rbtree_t 对象中的右手臂长度。
 */
xrbt_size_t xrbtree_right_length(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 向 x_rbtree_t 对象插入新节点。
 * @note  回调设置索引键值时，不使用 move 操作方式。
 * 
 * @param [in ] xthis_ptr : 红黑树对象。
 * @param [in ] xrbt_vkey : 新节点的索引键值。
 * @param [out] xbt_ok    : 返回操作成功的标识。
 * 
 * @return x_rbnode_iter
 *         - 返回对应节点。
 */
x_rbnode_iter xrbtree_insert(
    x_rbtree_ptr xthis_ptr, xrbt_vkey_t xrbt_vkey, xrbt_bool_t * xbt_ok);

/**********************************************************/
/**
 * @brief 向 x_rbtree_t 对象插入新节点。
 * @note  回调设置索引键值时，使用 move 操作方式。
 * 
 * @param [in ] xthis_ptr : 红黑树对象。
 * @param [in ] xrbt_mkey : 新节点的索引键值。
 * @param [out] xbt_ok    : 返回操作成功的标识。
 * 
 * @return x_rbnode_iter
 *         - 返回对应节点。
 */
x_rbnode_iter xrbtree_insert_mkey(
    x_rbtree_ptr xthis_ptr, xrbt_vkey_t xrbt_mkey, xrbt_bool_t * xbt_ok);

/**********************************************************/
/**
 * @brief 从 x_rbtree_t 对象中删除指定节点。
 */
xrbt_void_t xrbtree_erase(x_rbtree_ptr xthis_ptr, x_rbnode_iter xiter_node);

/**********************************************************/
/**
 * @brief 从 x_rbtree_t 对象中删除指定节点。
 */
xrbt_bool_t xrbtree_erase_vkey(x_rbtree_ptr xthis_ptr, xrbt_vkey_t xrbt_vkey);

/**********************************************************/
/**
 * @brief 将节点对象停靠（插入）到红黑树中。
 * @note
 * xiter_node 必须处于分离状态（参看 @see xrbtree_iter_is_undocked()），
 * 而且其索引键必须 与 xthis_ptr 存储的节点索引键类型相同。
 * 
 * @param [in ] xthis_ptr  : 红黑树对象。
 * @param [in ] xiter_node : 停靠操作的节点对象。
 * 
 * @return x_rbnode_iter
 *         - 返回的 节点对象 == xiter_node，停靠成功。
 *         - 返回的 节点对象 != xiter_node，停靠失败（索引键值冲突）。
 */
x_rbnode_iter xrbtree_dock(x_rbtree_ptr xthis_ptr, x_rbnode_iter xiter_node);

/**********************************************************/
/**
 * @brief 将节点对象从红黑树中分离（移除）出来。
 * @note xiter_node 必须 已经隶属于 xthis_ptr 。
 * 
 * @param [in ] xthis_ptr  : 红黑树对象。
 * @param [in ] xiter_node : 分离操作的节点对象。
 * 
 * @return x_rbnode_iter
 *         - 返回的 节点对象 == xiter_node。
 */
x_rbnode_iter xrbtree_undock(x_rbtree_ptr xthis_ptr, x_rbnode_iter xiter_node);

/**********************************************************/
/**
 * @brief 在 x_rbtree_t 对象中查找指定节点。
 * @note  若返回 NIL 则表示 x_rbtree_t 对象不包含该节点键值。
 */
x_rbnode_iter xrbtree_find(x_rbtree_ptr xthis_ptr, xrbt_vkey_t xrbt_vkey);

/**********************************************************/
/**
 * @brief 返回的是首个不小于 指定索引键值 的 节点位置。
 */
x_rbnode_iter xrbtree_lower_bound(x_rbtree_ptr xthis_ptr, xrbt_vkey_t xrbt_vkey);

/**********************************************************/
/**
 * @brief 返回的是首个大于 指定索引键值 的 节点位置。
 */
x_rbnode_iter xrbtree_upper_bound(x_rbtree_ptr xthis_ptr, xrbt_vkey_t xrbt_vkey);

/**********************************************************/
/**
 * @brief 返回 x_rbtree_t 对象的根节点。
 */
x_rbnode_iter xrbtree_root(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 返回 x_rbtree_t 对象的正向遍历操作的起始节点。
 */
x_rbnode_iter xrbtree_begin(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 返回 x_rbtree_t 对象的正向遍历操作的终止节点。
 */
x_rbnode_iter xrbtree_end(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 返回（正向）下一个节点对象。
 */
x_rbnode_iter xrbtree_next(x_rbnode_iter xiter_node);

/**********************************************************/
/**
 * @brief 返回 x_rbtree_t 对象的反向遍历操作的起始节点。
 */
x_rbnode_iter xrbtree_rbegin(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 返回 x_rbtree_t 对象的反向遍历操作的终止节点。
 */
x_rbnode_iter xrbtree_rend(x_rbtree_ptr xthis_ptr);

/**********************************************************/
/**
 * @brief 返回（反向）下一个节点对象。
 */
x_rbnode_iter xrbtree_rnext(x_rbnode_iter xiter_node);

/**********************************************************/
/**
 * @brief 返回 节点对象 指向的索引键值。
 */
xrbt_vkey_t xrbtree_iter_vkey(x_rbnode_iter xiter_node);

/**********************************************************/
/**
 * @brief 判断 节点对象 是否为 NIL 。
 */
xrbt_bool_t xrbtree_iter_is_nil(x_rbnode_iter xiter_node);

/**********************************************************/
/**
 * @brief 判断 节点对象 是否为 非停靠状态（即其已被分离出红黑树）。
 */
xrbt_bool_t xrbtree_iter_is_undocked(x_rbnode_iter xiter_node);

/**********************************************************/
/**
 * @brief 返回 节点对象 所隶属的红黑树。
 * @note  内部会进行节点遍历操作，使用时应注重效率问题。
 */
x_rbtree_ptr xrbtree_iter_tree(x_rbnode_iter xiter_node);

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// declare C API for the rbtree node type

#define XRBTREE_CTYPE_API(_Kty, _Static, _Inline, _Suffix)                     \
                                                                               \
_Static _Inline xrbt_bool_t xrbtree_insert_##_Suffix(                          \
    x_rbtree_ptr xthis_ptr, _Kty xkey)                                         \
{                                                                              \
    xrbt_bool_t xbt_ok = XRBT_FALSE;                                           \
    xrbtree_insert(xthis_ptr, (xrbt_vkey_t)(&xkey), &xbt_ok);                  \
    return xbt_ok;                                                             \
}                                                                              \
                                                                               \
_Static _Inline xrbt_bool_t xrbtree_erase_##_Suffix(                           \
    x_rbtree_ptr xthis_ptr, _Kty xkey)                                         \
{                                                                              \
    return xrbtree_erase_vkey(xthis_ptr, (xrbt_vkey_t)(&xkey));                \
}                                                                              \
                                                                               \
_Static _Inline x_rbnode_iter xrbtree_find_##_Suffix(                          \
    x_rbtree_ptr xthis_ptr, _Kty xkey)                                         \
{                                                                              \
    return xrbtree_find(xthis_ptr, (xrbt_vkey_t)(&xkey));                      \
}                                                                              \
                                                                               \
_Static _Inline x_rbnode_iter xrbtree_lower_bound_##_Suffix(                   \
    x_rbtree_ptr xthis_ptr, _Kty xkey)                                         \
{                                                                              \
    return xrbtree_lower_bound(xthis_ptr, (xrbt_vkey_t)(&xkey));               \
}                                                                              \
                                                                               \
_Static _Inline x_rbnode_iter xrbtree_upper_bound_##_Kty(                      \
    x_rbtree_ptr xthis_ptr, _Kty xkey)                                         \
{                                                                              \
    return xrbtree_upper_bound(xthis_ptr, (xrbt_vkey_t)(&xkey));               \
}                                                                              \
                                                                               \
_Static _Inline _Kty xrbtree_iter_##_Suffix(                                   \
    x_rbnode_iter xiter_node)                                                  \
{                                                                              \
    return *(_Kty *)xrbtree_iter_vkey(xiter_node);                             \
}                                                                              \

#define XRBTREE_XFUNC_COPYFROM(_Kty, _Static, _Suffix)                         \
_Static xrbt_void_t xrbtree_xfunc_##_Suffix##_copyfrom(                        \
    xrbt_vkey_t xrbt_dkey,                                                     \
    xrbt_vkey_t xrbt_skey,                                                     \
    xrbt_size_t xrbt_size,                                                     \
    xrbt_bool_t xbt_move ,                                                     \
    xrbt_ctxt_t xrbt_ctxt)                                                     \
{                                                                              \
    *(_Kty *)xrbt_dkey = *(_Kty *)xrbt_skey;                                   \
}                                                                              \

#define XRBTREE_XFUNC_LESS_COMPARE(_Kty, _Static, _Suffix)                     \
_Static xrbt_bool_t xrbtree_xfunc_##_Suffix##_compare(                         \
    xrbt_vkey_t xrbt_lkey,                                                     \
    xrbt_vkey_t xrbt_rkey,                                                     \
    xrbt_size_t xrbt_size,                                                     \
    xrbt_ctxt_t xrbt_ctxt)                                                     \
{                                                                              \
    return (*(_Kty *)xrbt_lkey < *(_Kty *)xrbt_rkey);                          \
}                                                                              \

#define XRBTREE_XFUNC_GREATER_COMPARE(_Kty, _Static, _Suffix)                  \
_Static xrbt_bool_t xrbtree_xfunc_##_Suffix##_compare(                         \
    xrbt_vkey_t xrbt_lkey,                                                     \
    xrbt_vkey_t xrbt_rkey,                                                     \
    xrbt_size_t xrbt_size,                                                     \
    xrbt_ctxt_t xrbt_ctxt)                                                     \
{                                                                              \
    return (*(_Kty *)xrbt_lkey > *(_Kty *)xrbt_rkey);                          \
}                                                                              \

////////////////////////////////////////////////////////////////////////////////
// declare C++ API for the rbtree node type

#ifdef __cplusplus

#if __cplusplus >= 201103L
#include <utility> // for std::move()
#endif // __cplusplus >= 201103L

template< class _Kty >
inline xrbt_void_t xrbtree_vkey_copyfrom(xrbt_vkey_t xrbt_dkey,
                                         xrbt_vkey_t xrbt_skey,
                                         xrbt_size_t xrbt_size,
                                         xrbt_bool_t xbt_move ,
                                         xrbt_ctxt_t xrbt_ctxt)
{
#if __cplusplus >= 201103L
    if (xbt_move)
        *(_Kty *)xrbt_dkey = std::move(*(_Kty *)xrbt_skey);
    else
#endif // __cplusplus >= 201103L
        *(_Kty *)xrbt_dkey = *(_Kty *)xrbt_skey;
}

template< class _Kty >
inline xrbt_void_t xrbtree_vkey_destruct(xrbt_vkey_t xrbt_vkey,
                                         xrbt_size_t xrbt_size,
                                         xrbt_ctxt_t xrbt_ctxt)
{
    (*(_Kty *)xrbt_vkey).~_Kty();
}

template< class _Kty >
inline xrbt_bool_t xrbtree_vkey_compare(xrbt_vkey_t xrbt_lkey,
                                        xrbt_vkey_t xrbt_rkey,
                                        xrbt_size_t xrbt_size,
                                        xrbt_ctxt_t xrbt_ctxt)
{
    return (*(_Kty *)xrbt_lkey < *(_Kty *)xrbt_rkey);
}

template< class _Kty >
inline xrbt_callback_t xrbtree_default_callback(xrbt_ctxt_t xrbt_ctxt)
{
    xrbt_callback_t xcallback =
    {
        /* .xfunc_n_memalloc = */ XRBT_NULL,
        /* .xfunc_n_memfree  = */ XRBT_NULL,
        /* .xfunc_k_copyfrom = */ &xrbtree_vkey_copyfrom< _Kty >,
        /* .xfunc_k_destruct = */ &xrbtree_vkey_destruct< _Kty >,
        /* .xfunc_k_compare  = */ &xrbtree_vkey_compare< _Kty >,
        /* .xctxt_t_callback = */ xrbt_ctxt
    };

    return xcallback;
}

template< class _Kty >
inline x_rbtree_ptr xrbtree_create_k(xrbt_callback_t * xcallback = XRBT_NULL)
{
    if (XRBT_NULL == xcallback)
    {
        static xrbt_callback_t _S_callback =
            xrbtree_default_callback< _Kty >();
        xcallback = &_S_callback;
    }

    return xrbtree_create(sizeof(_Kty), xcallback);
}

template< class _Kty >
inline x_rbnode_iter xrbtree_insert_k(x_rbtree_ptr xthis_ptr,
                                      const _Kty & xkey,
                                      xrbt_bool_t * xbt_ok = XRBT_NULL)
{
    return xrbtree_insert(xthis_ptr, const_cast< _Kty * >(&xkey), xbt_ok);
}

#if __cplusplus >= 201103L
template< class _Kty >
inline x_rbnode_iter xrbtree_insert_k(x_rbtree_ptr xthis_ptr,
                                      _Kty && xkey,
                                      xrbt_bool_t * xbt_ok = XRBT_NULL)
{
    return xrbtree_insert_mkey(xthis_ptr, const_cast< _Kty * >(&xkey), xbt_ok);
}
#endif // __cplusplus >= 201103L

template< class _Kty >
inline xrbt_bool_t xrbtree_erase_k(x_rbtree_ptr xthis_ptr, const _Kty & xkey)
{
    return xrbtree_erase_vkey(xthis_ptr, const_cast< _Kty * >(&xkey));
}

template< class _Kty >
inline x_rbnode_iter xrbtree_find_k(x_rbtree_ptr xthis_ptr, const _Kty & xkey)
{
    return xrbtree_find(xthis_ptr, const_cast< _Kty * >(&xkey));
}

template< class _Kty >
inline x_rbnode_iter xrbtree_lower_bound_k(x_rbtree_ptr xthis_ptr,
                                           const _Kty & xkey)
{
    return xrbtree_lower_bound(xthis_ptr, const_cast< _Kty * >(&xkey));
}

template< class _Kty >
inline x_rbnode_iter xrbtree_upper_bound_k(x_rbtree_ptr xthis_ptr,
                                           const _Kty & xkey)
{
    return xrbtree_upper_bound(xthis_ptr, const_cast< _Kty * >(&xkey));
}

template< class _Kty >
inline _Kty & xrbtree_ikey_k(x_rbnode_iter xiter_node)
{
    return *(static_cast< _Kty * >(xrbtree_iter_vkey(xiter_node)));
}

#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////

#endif // __XRBTREE_H__
