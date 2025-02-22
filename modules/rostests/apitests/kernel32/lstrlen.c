/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for lstrlenA/W
 * PROGRAMMER:      Hermes Belusca-Maito
 */

#include "precomp.h"

LONG WINAPI VEHandler_1(PEXCEPTION_POINTERS ExceptionInfo)
{
    /*
     * Vectored Exception Handler possibly called for lstrlen(NULL).
     * Expected not to be called!
     */
    ok(FALSE, "VEHandler_1 called!\n");
    return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI VEHandler_2(PEXCEPTION_POINTERS ExceptionInfo)
{
    /* Vectored Exception Handler that should be called for lstrlen(<invalid_ptr>) */
    ok(TRUE, "VEHandler_2 not called?\n");
    return EXCEPTION_CONTINUE_SEARCH;
}

START_TEST(lstrlen)
{
    PVOID pVEH;

    /* Test basic functionality */
    ok(lstrlenA( "Hello World!") == 12, "lstrlenA failed!\n");
    ok(lstrlenW(L"Hello World!") == 12, "lstrlenW failed!\n");

    /*
     * NULL buffer is special and is considered separately;
     * no internal exception is generated.
     * Use Vectored Exception Handling to monitor for first-chance exceptions.
     */
pVEH = AddVectoredExceptionHandler(1, VEHandler_1);
    ok(lstrlenA(NULL) == 0, "lstrlenA should have returned 0.\n");
    ok(lstrlenW(NULL) == 0, "lstrlenW should have returned 0.\n");
RemoveVectoredExceptionHandler(pVEH);

    /*
     * Test some invalid buffers. Internal exceptions should be generated.
     * Use Vectored Exception Handling to monitor for first-chance exceptions.
     */
pVEH = AddVectoredExceptionHandler(1, VEHandler_2);
    ok(lstrlenA( (LPSTR)(LONG_PTR)0xbaadf00d) == 0, "lstrlenA should have returned 0.\n");
    ok(lstrlenW((LPWSTR)(LONG_PTR)0xbaadf00d) == 0, "lstrlenW should have returned 0.\n");
RemoveVectoredExceptionHandler(pVEH);
}
