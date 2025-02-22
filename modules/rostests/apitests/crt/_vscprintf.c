/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for _vscprintf
 */

#include <apitest.h>

#include <stdio.h>
#include <tchar.h>
#include <errno.h>

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
    /* Here you can mix wide and ANSI strings */
    call_varargs(12, "%S world!", L"hello");
    call_varargs(12, "%s world!", "hello");
    call_varargs(11, "%u cookies", 100);
    /* Do not test NULL argument. That is verified to SEGV on a */
    /* release-build with VC10 and MS' msvcrt. */
}
