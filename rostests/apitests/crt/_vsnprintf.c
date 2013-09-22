/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for _vsnprintf
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

static void call_varargs(char* buf, size_t buf_size, int expected_ret, LPCSTR formatString, ...)
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
    NTSTATUS ExceptionStatus;
    /* Here you can mix wide and ANSI strings */
    call_varargs(buffer, 255, 12, "%S world!", L"hello");
    call_varargs(buffer, 255, 12, "%s world!", "hello");
    call_varargs(buffer, 255, 11, "%u cookies", 100);
    /* This is how WINE implements _vcsprintf, and they are obviously wrong */
    StartSeh()
        call_varargs(NULL, INT_MAX, -1, "%s it really work?", "does");
#if defined(TEST_CRTDLL) || defined(TEST_USER32)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif
#if defined(TEST_MSVCRT) /* NTDLL doesn't use/set errno */
    ok(errno == EINVAL, "Expected EINVAL, got %u\n", errno);
#endif
    /* This one is no better */
    StartSeh()
        call_varargs(NULL, 0, -1, "%s it really work?", "does");
#if defined(TEST_CRTDLL) || defined(TEST_USER32)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif
#if defined(TEST_MSVCRT) /* NTDLL doesn't use/set errno */
    ok(errno == EINVAL, "Expected EINVAL, got %u\n", errno);
#endif
    /* One more NULL checks */
    StartSeh()
        call_varargs(buffer, 255, -1, NULL);
#if defined(TEST_CRTDLL)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif
#if defined(TEST_MSVCRT) /* NTDLL doesn't use/set errno */
    ok(errno == EINVAL, "Expected EINVAL, got %u\n", errno);
#endif
}
