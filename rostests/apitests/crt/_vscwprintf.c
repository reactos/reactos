/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for _vscprintf
 */

#include <stdio.h>
#include <wine/test.h>
#include <tchar.h>
#include <errno.h>

static void call_varargs(int expected_ret, LPCWSTR formatString, ...)
{
    va_list args;
    int ret;
    /* Test the basic functionality */
    va_start(args, formatString);
    ret = _vscwprintf(formatString, args);
    ok(expected_ret == ret, "Test failed: expected %i, got %i.\n", expected_ret, ret);
}

START_TEST(_vscwprintf)
{
    /* Lesson of the day: don't mix wide and ansi char */
    call_varargs(19, L"%s world!", "hello");
    call_varargs(12, L"%s world!", L"hello");
    call_varargs(17, L"Jack ate %u pies", 100);
    /* Do not test NULL argument. That is verified to SEGV on a */
    /* release-build with VC10 and MS' msvcrt. */
}
