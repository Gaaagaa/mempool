
#include "xtypes.h"
#include "xmem_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <chrono>

#ifdef _MSC_VER
#include <windows.h>
#else // !_MSC_VER
#include <unistd.h>
#include <sys/mman.h>
#endif // _MSC_VER

////////////////////////////////////////////////////////////////////////////////

#define XVERIFY(xptr) do { if (!(xptr)) assert(0); } while (0)

////////////////////////////////////////////////////////////////////////////////

using xtime_clock = std::chrono::system_clock;
using xtime_point = std::chrono::system_clock::time_point;
using xtime_value = std::chrono::microseconds;

#define xtime_dcast std::chrono::duration_cast< xtime_value >

////////////////////////////////////////////////////////////////////////////////

x_void_t * heap_alloc(x_size_t xst_size, x_handle_t xht_owner, x_handle_t xht_context)
{
#ifdef _MSC_VER
    return HeapAlloc(GetProcessHeap(), 0, xst_size);
#else // !_MSC_VER
    return mmap(X_NULL, xst_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif // _MSC_VER
}

x_void_t heap_free(x_void_t * xmt_heap, x_size_t xst_size, x_handle_t xht_owner, x_handle_t xht_context)
{
#ifdef _MSC_VER
    HeapFree(GetProcessHeap(), 0, xmt_heap);
#else // !_MSC_VER
    munmap(xmt_heap, xst_size);
#endif // _MSC_VER
}

////////////////////////////////////////////////////////////////////////////////

void test_xmpool1(x_int32_t xit_test_count, x_int32_t xit_test_size)
{
    x_int32_t xit_iter = 0;
    x_int32_t xit_jter = 0;

    xtime_point xtm_begin;
    xtime_value xtm_value;

    xmem_slice_t    xmem_slice = X_NULL;
    xmpool_handle_t xmpool_ptr = xmpool_create(&heap_alloc, &heap_free, X_NULL);

    //======================================

    xtm_begin = xtime_clock::now();
    for (xit_iter = 0; xit_iter < xit_test_count; ++xit_iter)
    {
        for (xit_jter = 1; xit_jter <= xit_test_size; ++xit_jter)
        {
            xmem_slice = xmpool_alloc(xmpool_ptr, xit_jter);
            assert(X_NULL != xmem_slice);
            xmem_slice[0] = (x_byte_t)xit_jter;
            XVERIFY(XMEM_ERR_OK == xmpool_recyc(xmpool_ptr, xmem_slice));
        }
    }
    xtm_value = xtime_dcast(xtime_clock::now() - xtm_begin);
    printf("[POOL, 1] time cost: %10d us\n", (int)xtm_value.count());

    //======================================

    xmpool_destroy(xmpool_ptr);
}

void test_malloc1(x_int32_t xit_test_count, x_int32_t xit_test_size)
{
    x_int32_t xit_iter = 0;
    x_int32_t xit_jter = 0;

    xtime_point xtm_begin;
    xtime_value xtm_value;

    xmem_slice_t xmem_slice = X_NULL;

    //======================================

    xtm_begin = xtime_clock::now();
    for (xit_iter = 0; xit_iter < xit_test_count; ++xit_iter)
    {
        for (xit_jter = 1; xit_jter <= xit_test_size; ++xit_jter)
        {
            xmem_slice = (xmem_slice_t)malloc(xit_jter);
            assert(X_NULL != xmem_slice);
            xmem_slice[0] = (x_byte_t)xit_jter;
            free(xmem_slice);
        }
    }
    xtm_value = xtime_dcast(xtime_clock::now() - xtm_begin);
    printf("[LIBC, 1] time cost: %10d us\n", (int)xtm_value.count());

    //======================================
}

//====================================================================

void test_xmpool2(x_int32_t xit_test_count, x_int32_t xit_alloc_count, x_int32_t xit_test_size)
{
    x_int32_t xit_iter = 0;
    x_int32_t xit_jter = 0;
    x_int32_t xit_kter = 0;

    xtime_point xtm_begin;
    xtime_value xtm_value;

    xmem_slice_t  * xmem_slice = (xmem_slice_t *)calloc(xit_alloc_count, sizeof(xmem_slice_t));
    xmpool_handle_t xmpool_ptr = xmpool_create(&heap_alloc, &heap_free, X_NULL);

    //======================================

    xtm_begin = xtime_clock::now();
    for (xit_iter = 0; xit_iter < xit_test_count; ++xit_iter)
    {
        for (xit_jter = 1; xit_jter <= xit_test_size; ++xit_jter)
        {
            for (xit_kter = 0; xit_kter < xit_alloc_count; ++xit_kter)
            {
                xmem_slice[xit_kter] = xmpool_alloc(xmpool_ptr, xit_jter);
                assert(X_NULL != xmem_slice[xit_kter]);
            }

            for (xit_kter = 0; xit_kter < xit_alloc_count; ++xit_kter)
            {
                xmem_slice[xit_kter][0] = (x_byte_t)xit_jter;
                XVERIFY(XMEM_ERR_OK == xmpool_recyc(xmpool_ptr, xmem_slice[xit_kter]));
                xmem_slice[xit_kter] = X_NULL;
            }
        }
    }
    xtm_value = xtime_dcast(xtime_clock::now() - xtm_begin);
    printf("[POOL, 2] time cost: %10d us\n", (int)xtm_value.count());

    //======================================

    xmpool_destroy(xmpool_ptr);
    free(*xmem_slice);
}

void test_malloc2(x_int32_t xit_test_count, x_int32_t xit_alloc_count, x_int32_t xit_test_size)
{
    x_int32_t xit_iter = 0;
    x_int32_t xit_jter = 0;
    x_int32_t xit_kter = 0;

    xtime_point xtm_begin;
    xtime_value xtm_value;

    xmem_slice_t * xmem_slice = (xmem_slice_t *)malloc(xit_alloc_count * sizeof(xmem_slice_t));

    //======================================

    xtm_begin = xtime_clock::now();
    for (xit_iter = 0; xit_iter < xit_test_count; ++xit_iter)
    {
        for (xit_jter = 1; xit_jter <= xit_test_size; ++xit_jter)
        {
            for (xit_kter = 0; xit_kter < xit_alloc_count; ++xit_kter)
            {
                xmem_slice[xit_kter] = (xmem_slice_t)malloc(xit_jter);
                assert(X_NULL != xmem_slice[xit_kter]);
            }

            for (xit_kter = 0; xit_kter < xit_alloc_count; ++xit_kter)
            {
                xmem_slice[xit_kter][0] = (x_byte_t)xit_jter;
                free(xmem_slice[xit_kter]);
                xmem_slice[xit_kter] = X_NULL;
            }
        }
    }
    xtm_value = xtime_dcast(xtime_clock::now() - xtm_begin);
    printf("[LIBC, 2] time cost: %10d us\n", (int)xtm_value.count());

    //======================================

    free(*xmem_slice);
}

//====================================================================

int main(int argc, char * argv[])
{
    x_int32_t xit_test_count  = 100;
    x_int32_t xit_alloc_count = 0;
    x_int32_t xit_test_size   = (32 * 1024);
    x_int32_t xit_first_test  = 0;

    printf("Usage: \n%s < test count > < alloc count [0] > < test size[32768] > < first_test [0] : 0 or 1 >\n\n", argv[0]);

    if (argc >= 2) xit_test_count  = atoi(argv[1]);
    if (argc >= 3) xit_alloc_count = atoi(argv[2]);
    if (argc >= 4) xit_test_size   = atoi(argv[3]);
    if (argc >= 5) xit_first_test  = atoi(argv[4]);

    printf("test  count: %10d\n", xit_test_count );
    printf("alloc count: %10d\n", xit_alloc_count);
    printf("test  size : %10d\n", xit_test_size  );

    if (xit_alloc_count > 0)
        printf("Tatol count: %10d\n", xit_test_count * xit_alloc_count * xit_test_size);
    else
        printf("Tatol count: %10d\n", xit_test_count * xit_test_size);

    printf("//======================================\n");

    if (0 != xit_first_test)
    {
        if (xit_alloc_count > 0)
        {
            test_xmpool2(xit_test_count, xit_alloc_count, xit_test_size);
            test_malloc2(xit_test_count, xit_alloc_count, xit_test_size);
        }
        else
        {
            test_xmpool1(xit_test_count, xit_test_size);
            test_malloc1(xit_test_count, xit_test_size);
        }
    }
    else
    {
        if (xit_alloc_count > 0)
        {
            test_malloc2(xit_test_count, xit_alloc_count, xit_test_size);
            test_xmpool2(xit_test_count, xit_alloc_count, xit_test_size);
        }
        else
        {
            test_malloc1(xit_test_count, xit_test_size);
            test_xmpool1(xit_test_count, xit_test_size);
        }
    }

    printf("//======================================\n");

	return 0;
}
