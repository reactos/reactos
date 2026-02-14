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

typedef int (__cdecl *PFN_vscwprintf)(const char *format, va_list argptr);
static PFN_vscwprintf p_vscwprintf;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_vscwprintf = (PFN_vscwprintf)GetProcAddress(hdll, "_vscwprintf");
    ok(p_vscwprintf != NULL, "Failed to load _vscwprintf from %s\n", TEST_DLL_NAME);
    return (p_vscwprintf != NULL);
}
#define _vscprintf p_vscwprintf

#endif // !TEST_STATIC_CRT

static void call_varargs(int expected_ret, LPCWSTR formatString, ...)
{
    va_list args;
    int ret;
    /* Test the basic functionality */
    va_start(args, formatString);
    ret = _vscwprintf(formatString, args);
    va_end(args);
    ok(expected_ret == ret, "expected %i, got %i.\n", expected_ret, ret);
}

START_TEST(_vscwprintf)
{
#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _vscwprintf is not available\n");
        return;
    }
#endif

    /* Lesson of the day: don't mix wide and ansi char */
    /* Lesson of the week: don't ignore the lesson of the day */
    call_varargs(12, L"%hs world!", "hello");
    call_varargs(12, L"%s world!", L"hello");
    call_varargs(17, L"Jack ate %u pies", 100);
    /* Do not test NULL argument. That is verified to SEGV on a */
    /* release-build with VC10 and MS' msvcrt. */
}
