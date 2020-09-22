/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for RtlMultipleAllocateHeap and RtlMultipleFreeHeap
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "precomp.h"

typedef ULONG (NTAPI *FN_RtlMultipleAllocateHeap)(IN PVOID, IN ULONG, IN SIZE_T, IN ULONG, OUT PVOID *);
typedef ULONG (NTAPI *FN_RtlMultipleFreeHeap)(IN PVOID, IN ULONG, IN ULONG, OUT PVOID *);

static FN_RtlMultipleAllocateHeap g_alloc = NULL;
static FN_RtlMultipleFreeHeap g_free = NULL;

#define TEST_ALLOC(ret_expected,err_expected,threw_excepted,HeapHandle,Flags,Size,Count,Array) \
    threw = 0; \
    SetLastError(-1); \
    _SEH2_TRY { \
        ret = g_alloc((HeapHandle), (Flags), (Size), (Count), (Array)); \
        err = GetLastError(); \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { \
        threw = _SEH2_GetExceptionCode(); \
    } \
    _SEH2_END \
    ok((ret) == (ret_expected), "ret excepted %d, but %d\n", (ret_expected), (ret)); \
    ok((err) == (err_expected), "err excepted %d, but %d\n", (err_expected), (err)); \
    ok((threw) == (threw_excepted), "threw excepted %d, but %d\n", (threw_excepted), (threw));

#define TEST_ALLOC_NO_RET(err_expected,threw_excepted,HeapHandle,Flags,Size,Count,Array) \
    threw = 0; \
    SetLastError(-1); \
    _SEH2_TRY { \
        ret = g_alloc((HeapHandle), (Flags), (Size), (Count), (Array)); \
        err = GetLastError(); \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { \
        threw = _SEH2_GetExceptionCode(); \
    } \
    _SEH2_END \
    ok((err) == (err_expected), "err excepted %d, but %d", (err_expected), (err)); \
    ok((threw) == (threw_excepted), "threw excepted %d, but %d\n", (threw_excepted), (threw));

#define TEST_FREE(ret_expected,err_expected,threw_excepted,HeapHandle,Flags,Count,Array) \
    threw = 0; \
    SetLastError(-1); \
    _SEH2_TRY { \
        ret = g_free((HeapHandle), (Flags), (Count), (Array)); \
        err = GetLastError(); \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { \
        threw = _SEH2_GetExceptionCode(); \
    } \
    _SEH2_END \
    ok((ret) == (ret_expected), "ret excepted %d, but %d\n", (ret_expected), (ret)); \
    ok((err) == (err_expected), "err excepted %d, but %d\n", (err_expected), (err)); \
    ok((threw) == (threw_excepted), "threw excepted %d, but %d\n", (threw_excepted), (threw));

#define ASSUME_ARRAY_ITEMS_ARE_NULL() \
    ok(Array[0] == NULL, "Array[0] is expected as NULL\n"); \
    ok(Array[1] == NULL, "Array[1] is expected as NULL\n"); \
    ok(Array[2] == NULL, "Array[2] is expected as NULL\n");

#define INT_EXPECTED(var,value) \
    ok((var) == (value), #var " expected %d, but %d\n", (value), (var))

static void
set_array(PVOID *array, PVOID p0, PVOID p1, PVOID p2)
{
    array[0] = p0;
    array[1] = p1;
    array[2] = p2;
}

static void
MultiHeapAllocTest()
{
    INT ret, threw, err;
    HANDLE HeapHandle = GetProcessHeap();
    PVOID Array[3] = {NULL, NULL, NULL};

    // HeapHandle is non-NULL and array is NULL
    TEST_ALLOC(0, -1, 0, HeapHandle, 0, 0, 0, NULL);
    TEST_ALLOC(0, -1, 0xC0000005, HeapHandle, 0, 0, 1, NULL);
    TEST_ALLOC(0, -1, 0, HeapHandle, 0, 1, 0, NULL);
    TEST_ALLOC(0, -1, 0xC0000005, HeapHandle, 0, 1, 1, NULL);

    // Array is non-NULL and contents are NULL
    set_array(Array, NULL, NULL, NULL);
    TEST_ALLOC(0, -1, 0, HeapHandle, 0, 0, 0, Array);
    ASSUME_ARRAY_ITEMS_ARE_NULL();

    set_array(Array, NULL, NULL, NULL);
    TEST_ALLOC(1, -1, 0, HeapHandle, 0, 0, 1, Array);
    ok(Array[0] != NULL, "Array[0] is expected as non-NULL\n");
    ok(Array[1] == NULL, "Array[1] is expected as NULL\n");
    ok(Array[2] == NULL, "Array[2] is expected as NULL\n");

    set_array(Array, NULL, NULL, NULL);
    TEST_ALLOC(0, -1, 0, HeapHandle, 0, 1, 0, Array);
    ASSUME_ARRAY_ITEMS_ARE_NULL();

    set_array(Array, NULL, NULL, NULL);
    TEST_ALLOC(1, -1, 0, HeapHandle, 0, 1, 1, Array);
    ok(Array[0] != NULL, "Array[0] is expected as non-NULL\n");
    ok(Array[1] == NULL, "Array[1] is expected as NULL\n");
    ok(Array[2] == NULL, "Array[2] is expected as NULL\n");

    set_array(Array, NULL, NULL, NULL);
    TEST_ALLOC(2, -1, 0, HeapHandle, 0, 1, 2, Array);
    ok(Array[0] != NULL, "Array[0] is expected as non-NULL\n");
    ok(Array[1] != NULL, "Array[1] is expected as non-NULL\n");
    ok(Array[2] == NULL, "Array[2] is expected as NULL\n");

    set_array(Array, NULL, NULL, NULL);
    TEST_ALLOC(3, -1, 0, HeapHandle, 0, 1, 3, Array);
    ok(Array[0] != NULL, "Array[0] is expected as non-NULL\n");
    ok(Array[1] != NULL, "Array[1] is expected as non-NULL\n");
    ok(Array[2] != NULL, "Array[2] is expected as non-NULL\n");

    // Array is non-NULL and contents are invalid pointers
    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_ALLOC(0, -1, 0, HeapHandle, 0, 0, 0, Array);
    ok(Array[0] == (PVOID)1, "Array[0] is expected as 1\n");
    ok(Array[1] == (PVOID)2, "Array[1] is expected as 2\n");
    ok(Array[2] == (PVOID)3, "Array[2] is expected as 3\n");

    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_ALLOC(1, -1, 0, HeapHandle, 0, 0, 1, Array);
    ok(Array[0] != NULL, "Array[0] is expected as non-NULL\n");
    ok(Array[1] == (PVOID)2, "Array[1] is expected as 2\n");
    ok(Array[2] == (PVOID)3, "Array[2] is expected as 3\n");

    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_ALLOC(0, -1, 0, HeapHandle, 0, 1, 0, Array);
    ok(Array[0] == (PVOID)1, "Array[0] is expected as 1\n");
    ok(Array[1] == (PVOID)2, "Array[1] is expected as 2\n");
    ok(Array[2] == (PVOID)3, "Array[2] is expected as 3\n");

    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_ALLOC(1, -1, 0, HeapHandle, 0, 1, 1, Array);
    ok(Array[0] != NULL, "Array[0] is expected as non-NULL\n");
    ok(Array[1] == (PVOID)2, "Array[1] is expected as non-NULL\n");
    ok(Array[2] == (PVOID)3, "Array[2] is expected as NULL\n");

    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_ALLOC(2, -1, 0, HeapHandle, 0, 1, 2, Array);
    ok(Array[0] != NULL, "Array[0] is expected as non-NULL\n");
    ok(Array[1] != NULL, "Array[1] is expected as non-NULL\n");
    ok(Array[2] == (PVOID)3, "Array[2] is expected as 3\n");

    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_ALLOC(3, -1, 0, HeapHandle, 0, 1, 3, Array);
    ok(Array[0] != NULL, "Array[0] is expected as non-NULL\n");
    ok(Array[1] != NULL, "Array[1] is expected as non-NULL\n");
    ok(Array[2] != NULL, "Array[2] is expected as non-NULL\n");

    // Array is non-NULL and too large to allocate
    set_array(Array, NULL, NULL, NULL);
    TEST_ALLOC_NO_RET(ERROR_NOT_ENOUGH_MEMORY, 0, HeapHandle, 0, 0x5FFFFFFF, 3, Array);
    ok(ret != 3, "excepted not allocated");
    set_array(Array, NULL, NULL, NULL);
    TEST_ALLOC_NO_RET(ERROR_NOT_ENOUGH_MEMORY, 0xC0000017, HeapHandle, HEAP_GENERATE_EXCEPTIONS, 0x5FFFFFFF, 3, Array);
    ok(ret != 3, "excepted not allocated");
}

static void
MultiHeapFreeTest()
{
    INT ret, threw, err;
    HANDLE HeapHandle = GetProcessHeap();
    PVOID Array[3] = {NULL, NULL, NULL};

    // HeapHandle is non-NULL and array is NULL
    TEST_FREE(0, -1, 0, HeapHandle, 0, 0, NULL);
    TEST_FREE(0, -1, 0, HeapHandle, 0, 0, NULL);
    TEST_FREE(0, -1, 0xC0000005, HeapHandle, 0, 1, NULL);
    TEST_FREE(0, -1, 0xC0000005, HeapHandle, 0, 2, NULL);
    TEST_FREE(0, -1, 0xC0000005, HeapHandle, 0, 3, NULL);

    // Array is non-NULL and contents are NULL
    set_array(Array, NULL, NULL, NULL);
    TEST_FREE(0, -1, 0, HeapHandle, 0, 0, Array);
    set_array(Array, NULL, NULL, NULL);
    TEST_FREE(1, -1, 0, HeapHandle, 0, 1, Array);
    set_array(Array, NULL, NULL, NULL);
    TEST_FREE(2, -1, 0, HeapHandle, 0, 2, Array);
    set_array(Array, NULL, NULL, NULL);
    TEST_FREE(3, -1, 0, HeapHandle, 0, 3, Array);

    // Array is non-NULL and contents are invalid pointers
    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_FREE(0, -1, 0, HeapHandle, 0, 0, Array);
    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_FREE(0, ERROR_INVALID_PARAMETER, 0, HeapHandle, 0, 1, Array);
    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_FREE(0, ERROR_INVALID_PARAMETER, 0, HeapHandle, 0, 2, Array);
    set_array(Array, (PVOID)1, (PVOID)2, (PVOID)3);
    TEST_FREE(0, ERROR_INVALID_PARAMETER, 0, HeapHandle, 0, 3, Array);

    // Array is non-NULL and contents are 1 valid pointer and 2 NULLs
    set_array(Array, NULL, NULL, NULL);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(0, -1, 0, HeapHandle, 0, 0, Array);

    set_array(Array, NULL, NULL, NULL);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(1, -1, 0, HeapHandle, 0, 1, Array);

    set_array(Array, NULL, NULL, NULL);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(2, -1, 0, HeapHandle, 0, 2, Array);

    set_array(Array, NULL, NULL, NULL);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(3, -1, 0, HeapHandle, 0, 3, Array);

    // Array is non-NULL and contents are 1 valid pointer and 2 invalids
    set_array(Array, NULL, (PVOID)2, (PVOID)3);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(0, -1, 0, HeapHandle, 0, 0, Array);

    set_array(Array, NULL, (PVOID)2, (PVOID)3);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(1, -1, 0, HeapHandle, 0, 1, Array);

    set_array(Array, NULL, (PVOID)2, (PVOID)3);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(1, ERROR_INVALID_PARAMETER, 0, HeapHandle, 0, 2, Array);

    set_array(Array, NULL, (PVOID)2, (PVOID)3);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(1, ERROR_INVALID_PARAMETER, 0, HeapHandle, 0, 3, Array);

    // Array is non-NULL and contents are 1 valid pointer and 2 invalids (generate exceptions)
    set_array(Array, NULL, (PVOID)2, (PVOID)3);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(0, -1, 0, HeapHandle, HEAP_GENERATE_EXCEPTIONS, 0, Array);

    set_array(Array, NULL, (PVOID)2, (PVOID)3);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(1, -1, 0, HeapHandle, HEAP_GENERATE_EXCEPTIONS, 1, Array);

    set_array(Array, NULL, (PVOID)2, (PVOID)3);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(1, ERROR_INVALID_PARAMETER, 0, HeapHandle, HEAP_GENERATE_EXCEPTIONS, 2, Array);

    set_array(Array, NULL, (PVOID)2, (PVOID)3);
    ret = g_alloc(HeapHandle, 0, 1, 1, Array);
    INT_EXPECTED(ret, 1);
    TEST_FREE(1, ERROR_INVALID_PARAMETER, 0, HeapHandle, HEAP_GENERATE_EXCEPTIONS, 3, Array);

    // Array is non-NULL and contents are 3 valid pointers
    set_array(Array, NULL, NULL, NULL);
    ret = g_alloc(HeapHandle, 0, 3, 3, Array);
    INT_EXPECTED(ret, 3);
    TEST_FREE(3, -1, 0, HeapHandle, 0, 3, Array);
}

START_TEST(RtlMultipleAllocateHeap)
{
    HINSTANCE ntdll = LoadLibraryA("ntdll");

    g_alloc = (FN_RtlMultipleAllocateHeap)GetProcAddress(ntdll, "RtlMultipleAllocateHeap");
    g_free = (FN_RtlMultipleFreeHeap)GetProcAddress(ntdll, "RtlMultipleFreeHeap");

    if (!g_alloc || !g_free)
    {
        skip("RtlMultipleAllocateHeap or RtlMultipleFreeHeap not found\n");
    }
    else
    {
        MultiHeapAllocTest();
        MultiHeapFreeTest();
    }

    FreeLibrary(ntdll);
}
