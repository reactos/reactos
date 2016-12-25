/*
 * Unit tests for fiber functions
 *
 * Copyright (c) 2010 André Hentschel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/test.h"

static LPVOID (WINAPI *pCreateFiber)(SIZE_T,LPFIBER_START_ROUTINE,LPVOID);
static LPVOID (WINAPI *pConvertThreadToFiber)(LPVOID);
static BOOL (WINAPI *pConvertFiberToThread)(void);
static void (WINAPI *pSwitchToFiber)(LPVOID);
static void (WINAPI *pDeleteFiber)(LPVOID);
static LPVOID (WINAPI *pConvertThreadToFiberEx)(LPVOID,DWORD);
static LPVOID (WINAPI *pCreateFiberEx)(SIZE_T,SIZE_T,DWORD,LPFIBER_START_ROUTINE,LPVOID);
static BOOL (WINAPI *pIsThreadAFiber)(void);
static DWORD (WINAPI *pFlsAlloc)(PFLS_CALLBACK_FUNCTION);
static BOOL (WINAPI *pFlsFree)(DWORD);
static PVOID (WINAPI *pFlsGetValue)(DWORD);
static BOOL (WINAPI *pFlsSetValue)(DWORD,PVOID);

static void *fibers[3];
static BYTE testparam = 185;
static DWORD fls_index_to_set = FLS_OUT_OF_INDEXES;
static void* fls_value_to_set;

static int fiberCount = 0;
static int cbCount = 0;

static VOID init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

#define X(f) p##f = (void*)GetProcAddress(hKernel32, #f);
    X(CreateFiber);
    X(ConvertThreadToFiber);
    X(ConvertFiberToThread);
    X(SwitchToFiber);
    X(DeleteFiber);
    X(ConvertThreadToFiberEx);
    X(CreateFiberEx);
    X(IsThreadAFiber);
    X(FlsAlloc);
    X(FlsFree);
    X(FlsGetValue);
    X(FlsSetValue);
#undef X
}

static VOID WINAPI FiberLocalStorageProc(PVOID lpFlsData)
{
    ok(lpFlsData == fls_value_to_set,
       "FlsData expected not to be changed, value is %p, expected %p\n",
       lpFlsData, fls_value_to_set);
    cbCount++;
}

static VOID WINAPI FiberMainProc(LPVOID lpFiberParameter)
{
    BYTE *tparam = (BYTE *)lpFiberParameter;
    fiberCount++;
    ok(*tparam == 185, "Parameterdata expected not to be changed\n");
    if (fls_index_to_set != FLS_OUT_OF_INDEXES)
    {
        void* ret;
        BOOL bret;

        ret = pFlsGetValue(fls_index_to_set);
        ok(ret == NULL, "FlsGetValue returned %p, expected NULL\n", ret);

        /* Set the FLS value */
        bret = pFlsSetValue(fls_index_to_set, fls_value_to_set);
        ok(bret, "FlsSetValue failed with error %u\n", GetLastError());

        /* Verify that FlsGetValue retrieves the value set by FlsSetValue */
        SetLastError( 0xdeadbeef );
        ret = pFlsGetValue(fls_index_to_set);
        ok(ret == fls_value_to_set, "FlsGetValue returned %p, expected %p\n", ret, fls_value_to_set);
        ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue error %u\n", GetLastError());
    }
    pSwitchToFiber(fibers[0]);
}

static void test_ConvertThreadToFiber(void)
{
    if (pConvertThreadToFiber)
    {
        fibers[0] = pConvertThreadToFiber(&testparam);
        ok(fibers[0] != NULL, "ConvertThreadToFiber failed with error %u\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertThreadToFiber not present\n" );
    }
}

static void test_ConvertThreadToFiberEx(void)
{
    if (pConvertThreadToFiberEx)
    {
        fibers[0] = pConvertThreadToFiberEx(&testparam, 0);
        ok(fibers[0] != NULL, "ConvertThreadToFiberEx failed with error %u\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertThreadToFiberEx not present\n" );
    }
}

static void test_ConvertFiberToThread(void)
{
    if (pConvertFiberToThread)
    {
        BOOL ret = pConvertFiberToThread();
        ok(ret, "ConvertFiberToThread failed with error %u\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertFiberToThread not present\n" );
    }
}

static void test_FiberHandling(void)
{
    fiberCount = 0;
    fibers[0] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[0] != NULL, "CreateFiber failed with error %u\n", GetLastError());
    pDeleteFiber(fibers[0]);

    test_ConvertThreadToFiber();
    test_ConvertFiberToThread();
    if (pConvertThreadToFiberEx)
        test_ConvertThreadToFiberEx();
    else
        test_ConvertThreadToFiber();

    fibers[1] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[1] != NULL, "CreateFiber failed with error %u\n", GetLastError());

    pSwitchToFiber(fibers[1]);
    ok(fiberCount == 1, "Wrong fiber count: %d\n", fiberCount);
    pDeleteFiber(fibers[1]);

    if (pCreateFiberEx)
    {
        fibers[1] = pCreateFiberEx(0,0,0,FiberMainProc,&testparam);
        ok(fibers[1] != NULL, "CreateFiberEx failed with error %u\n", GetLastError());

        pSwitchToFiber(fibers[1]);
        ok(fiberCount == 2, "Wrong fiber count: %d\n", fiberCount);
        pDeleteFiber(fibers[1]);
    }
    else win_skip( "CreateFiberEx not present\n" );

    if (pIsThreadAFiber) ok(pIsThreadAFiber(), "IsThreadAFiber reported FALSE\n");
    test_ConvertFiberToThread();
    if (pIsThreadAFiber) ok(!pIsThreadAFiber(), "IsThreadAFiber reported TRUE\n");
}

static void test_FiberLocalStorage(void)
{
    DWORD fls, fls_2;
    BOOL ret;
    void* val;

    if (!pFlsAlloc || !pFlsSetValue || !pFlsGetValue || !pFlsFree)
    {
        win_skip( "Fiber Local Storage not supported\n" );
        return;
    }

    /* Test an unallocated index
     * FlsFree should fail
     * FlsGetValue and FlsSetValue should succeed
     */
    SetLastError( 0xdeadbeef );
    ret = pFlsFree( 127 );
    ok( !ret, "freeing fls index 127 (unallocated) succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "freeing fls index 127 (unallocated) wrong error %u\n", GetLastError() );

    val = pFlsGetValue( 127 );
    ok( val == NULL,
        "getting fls index 127 (unallocated) failed with error %u\n", GetLastError() );

    ret = pFlsSetValue( 127, (void*) 0x217 );
    ok( ret, "setting fls index 127 (unallocated) failed with error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( 127 );
    ok( val == (void*) 0x217, "fls index 127 (unallocated) wrong value %p\n", val );
    ok( GetLastError() == ERROR_SUCCESS,
        "getting fls index 127 (unallocated) failed with error %u\n", GetLastError() );

    /* FlsFree, FlsGetValue, and FlsSetValue out of bounds should return
     * ERROR_INVALID_PARAMETER
     */
    SetLastError( 0xdeadbeef );
    ret = pFlsFree( 128 );
    ok( !ret, "freeing fls index 128 (out of bounds) succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "freeing fls index 128 (out of bounds) wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pFlsSetValue( 128, (void*) 0x217 );
    ok( !ret, "setting fls index 128 (out of bounds) succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "setting fls index 128 (out of bounds) wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( 128 );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "getting fls index 128 (out of bounds) wrong error %u\n", GetLastError() );

    /* Test index 0 */
    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( 0 );
    ok( !val, "fls index 0 set to %p\n", val );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "setting fls index wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pFlsSetValue( 0, (void *)0xdeadbeef );
    ok( !ret, "setting fls index 0 succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "setting fls index wrong error %u\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( 0 );
    ok( !val, "fls index 0 wrong value %p\n", val );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "setting fls index wrong error %u\n", GetLastError() );

    /* Test creating an FLS index */
    fls = pFlsAlloc( NULL );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed\n" );
    ok( fls != 0, "fls index 0 allocated\n" );
    val = pFlsGetValue( fls );
    ok( !val, "fls index %u wrong value %p\n", fls, val );
    ret = pFlsSetValue( fls, (void *)0xdeadbeef );
    ok( ret, "setting fls index %u failed\n", fls );
    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( fls );
    ok( val == (void *)0xdeadbeef, "fls index %u wrong value %p\n", fls, val );
    ok( GetLastError() == ERROR_SUCCESS,
        "getting fls index %u failed with error %u\n", fls, GetLastError() );
    pFlsFree( fls );

    /* Undefined behavior: verify the value is NULL after it the slot is freed */
    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( fls );
    ok( val == NULL, "fls index %u wrong value %p\n", fls, val );
    ok( GetLastError() == ERROR_SUCCESS,
        "getting fls index %u failed with error %u\n", fls, GetLastError() );

    /* Undefined behavior: verify the value is settable after the slot is freed */
    ret = pFlsSetValue( fls, (void *)0xdeadbabe );
    ok( ret, "setting fls index %u failed\n", fls );
    val = pFlsGetValue( fls );
    ok( val == (void *)0xdeadbabe, "fls index %u wrong value %p\n", fls, val );

    /* Try to create the same FLS index again, and verify that is initialized to NULL */
    fls_2 = pFlsAlloc( NULL );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %u\n", GetLastError() );
    /* If this fails it is not an API error, but the test will be inconclusive */
    ok( fls_2 == fls, "different FLS index allocated, was %u, now %u\n", fls, fls_2 );

    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( fls_2 );
    ok( val == NULL, "fls index %u wrong value %p\n", fls, val );
    ok( GetLastError() == ERROR_SUCCESS,
        "getting fls index %u failed with error %u\n", fls_2, GetLastError() );
    pFlsFree( fls_2 );
}

static void test_FiberLocalStorageCallback(PFLS_CALLBACK_FUNCTION cbfunc)
{
    DWORD fls;
    BOOL ret;
    void* val, *val2;

    if (!pFlsAlloc || !pFlsSetValue || !pFlsGetValue || !pFlsFree)
    {
        win_skip( "Fiber Local Storage not supported\n" );
        return;
    }

    /* Test that the callback is executed */
    cbCount = 0;
    fls = pFlsAlloc( cbfunc );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %u\n", GetLastError() );

    val = (void*) 0x1587;
    fls_value_to_set = val;
    ret = pFlsSetValue( fls, val );
    ok(ret, "FlsSetValue failed with error %u\n", GetLastError() );

    val2 = pFlsGetValue( fls );
    ok(val == val2, "FlsGetValue returned %p, expected %p\n", val2, val);

    ret = pFlsFree( fls );
    ok(ret, "FlsFree failed with error %u\n", GetLastError() );
    todo_wine ok( cbCount == 1, "Wrong callback count: %d\n", cbCount );

    /* Test that callback is not executed if value is NULL */
    cbCount = 0;
    fls = pFlsAlloc( cbfunc );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %u\n", GetLastError() );

    ret = pFlsSetValue( fls, NULL );
    ok( ret, "FlsSetValue failed with error %u\n", GetLastError() );

    pFlsFree( fls );
    ok( ret, "FlsFree failed with error %u\n", GetLastError() );
    ok( cbCount == 0, "Wrong callback count: %d\n", cbCount );
}

static void test_FiberLocalStorageWithFibers(PFLS_CALLBACK_FUNCTION cbfunc)
{
    void* val1 = (void*) 0x314;
    void* val2 = (void*) 0x152;
    BOOL ret;

    if (!pFlsAlloc || !pFlsFree || !pFlsSetValue || !pFlsGetValue)
    {
        win_skip( "Fiber Local Storage not supported\n" );
        return;
    }

    fls_index_to_set = pFlsAlloc(cbfunc);
    ok(fls_index_to_set != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %u\n", GetLastError());

    test_ConvertThreadToFiber();

    fiberCount = 0;
    cbCount = 0;
    fibers[1] = pCreateFiber(0,FiberMainProc,&testparam);
    fibers[2] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[1] != NULL, "CreateFiber failed with error %u\n", GetLastError());
    ok(fibers[2] != NULL, "CreateFiber failed with error %u\n", GetLastError());
    ok(fiberCount == 0, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 0, "Wrong callback count: %d\n", cbCount);

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val1;
    pSwitchToFiber(fibers[1]);
    ok(fiberCount == 1, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 0, "Wrong callback count: %d\n", cbCount);

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val2;
    pSwitchToFiber(fibers[2]);
    ok(fiberCount == 1, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 0, "Wrong callback count: %d\n", cbCount);

    fls_value_to_set = val2;
    ret = pFlsSetValue(fls_index_to_set, fls_value_to_set);
    ok(ret, "FlsSetValue failed\n");
    ok(val2 == pFlsGetValue(fls_index_to_set), "FlsGetValue failed\n");

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val1;
    pDeleteFiber(fibers[1]);
    ok(fiberCount == 0, "Wrong fiber count: %d\n", fiberCount);
    todo_wine ok(cbCount == 1, "Wrong callback count: %d\n", cbCount);

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val2;
    pFlsFree(fls_index_to_set);
    ok(fiberCount == 0, "Wrong fiber count: %d\n", fiberCount);
    todo_wine ok(cbCount == 2, "Wrong callback count: %d\n", cbCount);

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val1;
    pDeleteFiber(fibers[2]);
    ok(fiberCount == 0, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 0, "Wrong callback count: %d\n", cbCount);

    test_ConvertFiberToThread();
}

START_TEST(fiber)
{
    init_funcs();

    if (!pCreateFiber)
    {
        win_skip( "Fibers not supported by win95\n" );
        return;
    }

    test_FiberHandling();
    test_FiberLocalStorage();
    test_FiberLocalStorageCallback(FiberLocalStorageProc);
    test_FiberLocalStorageWithFibers(FiberLocalStorageProc);
}
