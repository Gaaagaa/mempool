
#include "xtypes.h"
#include "xmem_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <chrono>

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

////////////////////////////////////////////////////////////////////////////////

using xtime_clock = std::chrono::system_clock;
using xtime_point = std::chrono::system_clock::time_point;
using xtime_value = std::chrono::microseconds;

#define xtime_dcast std::chrono::duration_cast< xtime_value >

////////////////////////////////////////////////////////////////////////////////

void test_xmpool1(x_int32_t xit_test_count, x_int32_t xit_test_size)
{
    x_int32_t xit_iter = 0;
    x_int32_t xit_jter = 0;

    xtime_point xtm_begin;
    xtime_value xtm_value;

    xmem_slice_t    xmem_slice = X_NULL;
    xmpool_handle_t xmpool_ptr = xmpool_create(X_NULL, X_NULL, X_NULL);

    //======================================

    xtm_begin = xtime_clock::now();
    for (xit_iter = 0; xit_iter < xit_test_count; ++xit_iter)
    {
        for (xit_jter = 1; xit_jter <= xit_test_size; ++xit_jter)
        {
            xmem_slice = xmpool_alloc(xmpool_ptr, xit_jter);
            XASSERT(X_NULL != xmem_slice);
            xmem_slice[0] = (x_byte_t)xit_jter;
            xmpool_recyc(xmpool_ptr, xmem_slice);
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
            XASSERT(X_NULL != xmem_slice);
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
    xmpool_handle_t xmpool_ptr = xmpool_create(X_NULL, X_NULL, X_NULL);

    //======================================

    xtm_begin = xtime_clock::now();
    for (xit_iter = 0; xit_iter < xit_test_count; ++xit_iter)
    {
        for (xit_jter = 1; xit_jter <= xit_test_size; ++xit_jter)
        {
            for (xit_kter = 0; xit_kter < xit_alloc_count; ++xit_kter)
            {
                xmem_slice[xit_kter] = xmpool_alloc(xmpool_ptr, xit_jter);
            }

            for (xit_kter = 0; xit_kter < xit_alloc_count; ++xit_kter)
            {
                if (X_NULL != xmem_slice[xit_kter])
                {
                    xmem_slice[xit_kter][0] = (x_byte_t)xit_jter;
                    xmpool_recyc(xmpool_ptr, xmem_slice[xit_kter]);
                    xmem_slice[xit_kter] = X_NULL;
                }
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
            }

            for (xit_kter = 0; xit_kter < xit_alloc_count; ++xit_kter)
            {
                if (X_NULL != xmem_slice[xit_kter])
                {
                    xmem_slice[xit_kter][0] = (x_byte_t)xit_jter;
                    free(xmem_slice[xit_kter]);
                    xmem_slice[xit_kter] = X_NULL;
                }
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
