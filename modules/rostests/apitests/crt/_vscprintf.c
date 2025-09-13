/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for _vscprintf
 */

#include <apitest.h>

#include <stdio.h>
#include <tchar.h>
#include <errno.h>

#ifndef TEST_STATIC_CRT

typedef int (__cdecl *PFN_vscprintf)(const char *format, va_list argptr);
static PFN_vscprintf p_vscprintf;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_vscprintf = (PFN_vscprintf)GetProcAddress(hdll, "_vscprintf");
    ok(p_vscprintf != NULL, "Failed to load _vscprintf from %s\n", TEST_DLL_NAME);
    return (p_vscprintf != NULL);
}
#define _vscprintf p_vscprintf

#endif // !TEST_STATIC_CRT

static void call_varargs(int expected_ret, LPCSTR formatString, ...)
{
    va_list args;
    int ret;
    /* Test the basic functionality */
    va_start(args, formatString);
    ret = _vscprintf(formatString, args);
    va_end(args);
    ok(expected_ret == ret, "Test failed: expected %i, got %i.\n", expected_ret, ret);
}

START_TEST(_vscprintf)
{
#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _vscprintf is not available\n");
        return;
    }
#endif

    /* Here you can mix wide and ANSI strings */
    call_varargs(12, "%S world!", L"hello");
    call_varargs(12, "%s world!", "hello");
    call_varargs(11, "%u cookies", 100);
    /* Do not test NULL argument. That is verified to SEGV on a */
    /* release-build with VC10 and MS' msvcrt. */
}
