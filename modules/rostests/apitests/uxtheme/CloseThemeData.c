/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CloseThemeData
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <apitest.h>
#include <stdio.h>
#include <windows.h>
#include <uxtheme.h>

static LONG WINAPI VEHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
    ok(FALSE, "VEHandler called!\n");
    return EXCEPTION_CONTINUE_SEARCH;
}

START_TEST(CloseThemeData)
{
    PVOID pVEH;
    HRESULT hr;

    pVEH = AddVectoredExceptionHandler(1, VEHandler);

    hr = CloseThemeData((HTHEME)(ULONG_PTR)0xdeaddeaddeaddeadULL);
    ok( hr == E_HANDLE, "Expected E_HANDLE, got 0x%lx\n", hr);

    RemoveVectoredExceptionHandler(pVEH);
}
