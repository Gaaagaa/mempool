/**
 * @file    memheap_test.cpp
 * <pre>
 * Copyright (c) 2019, Gaaagaa All rights reserved.
 * 
 * 文件名称：memheap_test.cpp
 * 创建日期：2019年09月24日
 * 文件标识：
 * 文件摘要：堆内存管理测试程序。
 * 
 * 当前版本：1.0.0.0
 * 作    者：
 * 完成日期：2019年09月24日
 * 版本摘要：
 * 
 * 取代版本：
 * 原作者  ：
 * 完成日期：
 * 版本摘要：
 * </pre>
 */

#include "xmem_comm.h"

#include <stdio.h>
#include <chrono>

////////////////////////////////////////////////////////////////////////////////

void test_xmheap(void)
{
    xchunk_memptr_t xchunk_ptr = X_NULL;
    xmheap_handle_t xmheap_ptr = xmheap_create( 32 * 1024 * 1024,
                                               512 * 1024 * 1024);

    //======================================

    for (int i = 1; i <= 10000; ++i)
    {
        xchunk_ptr = xmheap_alloc(xmheap_ptr,
                                  i * XMEM_PAGE_SIZE,
                                  (xowner_handle_t)xmheap_ptr);

        XASSERT(XMEM_ERR_OK == xmheap_recyc(xmheap_ptr, xchunk_ptr));
    }

    //======================================

    xmheap_destroy(xmheap_ptr);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    test_xmheap();

    return 0;
}

