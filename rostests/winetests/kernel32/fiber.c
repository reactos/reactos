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

static LPVOID fibers[2];
static BYTE testparam = 185;
static WORD cbCount;

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
    cbCount++;
    ok(lpFlsData == (PVOID) 1587, "FlsData expected not to be changed\n");
}

static VOID WINAPI FiberMainProc(LPVOID lpFiberParameter)
{
    BYTE *tparam = (BYTE *)lpFiberParameter;
    cbCount++;
    ok(*tparam == 185, "Parameterdata expected not to be changed\n");
    pSwitchToFiber(fibers[0]);
}

static void test_ConvertThreadToFiber(void)
{
    if (pConvertThreadToFiber)
    {
        fibers[0] = pConvertThreadToFiber(&testparam);
        ok(fibers[0] != 0, "ConvertThreadToFiber failed with error %d\n", GetLastError());
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
        ok(fibers[0] != 0, "ConvertThreadToFiberEx failed with error %d\n", GetLastError());
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
        ok(ret, "ConvertFiberToThread failed with error %d\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertFiberToThread not present\n" );
    }
}

static void test_FiberHandling(void)
{
    cbCount = 0;
    fibers[0] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[0] != 0, "CreateFiber failed with error %d\n", GetLastError());
    pDeleteFiber(fibers[0]);

    test_ConvertThreadToFiber();
    test_ConvertFiberToThread();
    if (pConvertThreadToFiberEx)
        test_ConvertThreadToFiberEx();
    else
        test_ConvertThreadToFiber();


    fibers[1] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[1] != 0, "CreateFiber failed with error %d\n", GetLastError());

    pSwitchToFiber(fibers[1]);
    ok(cbCount == 1, "Wrong callback count: %d\n", cbCount);
    pDeleteFiber(fibers[1]);

    if (!pCreateFiberEx)
    {
        win_skip( "CreateFiberEx not present\n" );
        return;
    }

    SetLastError(0xdeadbeef);
    fibers[1] = pCreateFiberEx(0,0,0,FiberMainProc,&testparam);
    ok(fibers[1] != 0, "CreateFiberEx failed with error %d\n", GetLastError());

    pSwitchToFiber(fibers[1]);
    ok(cbCount == 2, "Wrong callback count: %d\n", cbCount);
    pDeleteFiber(fibers[1]);

    if (!pIsThreadAFiber)
    {
        win_skip( "IsThreadAFiber not present\n" );
        return;
    }

    ok(pIsThreadAFiber(), "IsThreadAFiber reported FALSE\n");
    test_ConvertFiberToThread();
    ok(!pIsThreadAFiber(), "IsThreadAFiber reported TRUE\n");
}

static void test_FiberLocalStorage(PFLS_CALLBACK_FUNCTION cbfunc)
{
    DWORD fls;
    BOOL ret;
    PVOID val = (PVOID) 1587;

    if (!pFlsAlloc)
    {
        win_skip( "Fiber Local Storage not supported\n" );
        return;
    }
    cbCount = 0;

    fls = pFlsAlloc(cbfunc);
    ok(fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %d\n", GetLastError());

    ret = pFlsSetValue(fls, val);
    ok(ret, "FlsSetValue failed\n");
    ok(val == pFlsGetValue(fls), "FlsGetValue failed\n");

    ret = pFlsFree(fls);
    ok(ret, "FlsFree failed\n");
    if (cbfunc)
        todo_wine ok(cbCount == 1, "Wrong callback count: %d\n", cbCount);

    /* test index 0 */
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

    fls = pFlsAlloc( NULL );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed\n" );
    ok( fls != 0, "fls index 0 allocated\n" );
    val = pFlsGetValue( fls );
    ok( !val, "fls index %u wrong value %p\n", fls, val );
    ret = pFlsSetValue( fls, (void *)0xdeadbeef );
    ok( ret, "setting fls index %u failed\n", fls );
    val = pFlsGetValue( fls );
    ok( val == (void *)0xdeadbeef, "fls index %u wrong value %p\n", fls, val );
    pFlsFree( fls );
    ret = pFlsSetValue( fls, (void *)0xdeadbabe );
    ok( ret, "setting fls index %u failed\n", fls );
    val = pFlsGetValue( fls );
    ok( val == (void *)0xdeadbabe, "fls index %u wrong value %p\n", fls, val );
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
    test_FiberLocalStorage(NULL);
    test_FiberLocalStorage(FiberLocalStorageProc);
}
