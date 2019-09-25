/**
 * @file    memheap_test.cpp
 * <pre>
 * Copyright (c) 2019, Gaaagaa All rights reserved.
 * 
 * �ļ����ƣ�memheap_test.cpp
 * �������ڣ�2019��09��24��
 * �ļ���ʶ��
 * �ļ�ժҪ�����ڴ������Գ���
 * 
 * ��ǰ�汾��1.0.0.0
 * ��    �ߣ�
 * ������ڣ�2019��09��24��
 * �汾ժҪ��
 * 
 * ȡ���汾��
 * ԭ����  ��
 * ������ڣ�
 * �汾ժҪ��
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

