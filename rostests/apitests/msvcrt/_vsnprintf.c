/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for _vsnprintf
 */

#include <wine/test.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdarg.h>

void call_varargs(char* buf, size_t buf_size, int expected_ret, LPCSTR formatString, ...)
{
    va_list args;
    int ret;
    /* Test the basic functionality */
    va_start(args, formatString);
    ret = _vsnprintf(buf, 255, formatString, args);
    ok(expected_ret == ret, "Test failed: expected %i, got %i.\n", expected_ret, ret);
}

START_TEST(_vsnprintf)
{
    char buffer[255];
    /* Test basic functionality */
    call_varargs(buffer, 255, 12, "%s world!", "hello");
    /* This is how WINE implements _vcsprintf, and they are obviously wrong */
    call_varargs(NULL, INT_MAX, -1, "%s it really work?", "does");
    /* This one is no better */
    call_varargs(NULL, 0, -1, "%s it really work?", "does");
}
