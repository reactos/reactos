/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for _vsnwprintf
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

static void call_varargs(wchar_t* buf, size_t buf_size, int expected_ret, LPCWSTR formatString, ...)
{
    va_list args;
    int ret;
    /* Test the basic functionality */
    va_start(args, formatString);
    ret = _vsnwprintf(buf, buf_size, formatString, args);
    ok(expected_ret == ret, "Test failed: expected %i, got %i.\n", expected_ret, ret);
}

START_TEST(_vsnwprintf)
{
    wchar_t buffer[255];

    /* Test basic functionality */
    //call_varargs(buffer, 255, 10, L"%s world!", "hello"); // this test is broken
    call_varargs(buffer, 255, 12, L"%s world!", L"hello");
    call_varargs(buffer, 255, 11, L"%u cookies", 100);
    /* This is how WINE implements _vcsprintf, and they are obviously wrong */
    StartSeh()
#if defined(TEST_CRTDLL)
        call_varargs(NULL, INT_MAX, -1, L"%s it really work?", L"does");
#else
        call_varargs(NULL, INT_MAX, 20, L"%s it really work?", L"does");
#endif

#if defined(TEST_USER32)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

#if defined(TEST_USER32)/* NTDLL doesn't use/set errno */
    ok(errno == EINVAL, "Expected EINVAL, got %u\n", errno);
#else
    ok(errno == 0, "Expected 0, got %u\n", errno);
#endif

    /* This one is no better */
    StartSeh()
#if defined(TEST_CRTDLL)
        call_varargs(NULL, 0, -1, L"%s it really work?", L"does");
#else
        call_varargs(NULL, 0, 20, L"%s it really work?", L"does");
#endif
    EndSeh(STATUS_SUCCESS);
    ok(errno == 0, "Expected 0, got %u\n", errno);


    /* One more NULL checks */
    StartSeh()
        call_varargs(buffer, 255, -1, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);
    ok(errno == 0, "Expected 0, got %u\n", errno);
}
