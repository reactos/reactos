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

#ifndef TEST_STATIC_CRT

typedef int (__cdecl *PFN_vsnwprintf)(wchar_t *buf, size_t cnt, const wchar_t *fmt, va_list args);
static PFN_vsnwprintf p_vsnwprintf;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_vsnwprintf = (PFN_vsnwprintf)GetProcAddress(hdll, "_vsnwprintf");
    ok(p_vsnwprintf != NULL, "Failed to load _vsnwprintf from %s\n", TEST_DLL_NAME);
    return (p_vsnwprintf != NULL);
}
#define _vsnwprintf p_vsnwprintf

#endif // !TEST_STATIC_CRT

static void call_varargs(wchar_t* buf, size_t buf_size, int expected_ret, LPCWSTR formatString, ...)
{
    va_list args;
    int ret;
    /* Test the basic functionality */
    va_start(args, formatString);
    ret = _vsnwprintf(buf, buf_size, formatString, args);
    va_end(args);
    ok(expected_ret == ret, "Test failed for `%ls`: expected %i, got %i.\n", formatString, expected_ret, ret);
}

START_TEST(_vsnwprintf)
{
    wchar_t buffer[255];

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _vsnwprintf is not available\n");
        return;
    }
#endif

    /* Test basic functionality */
    //call_varargs(buffer, 255, 10, L"%s world!", "hello"); // this test is broken
    call_varargs(buffer, 255, 12, L"%s world!", L"hello");
    call_varargs(buffer, 255, 11, L"%u cookies", 100);
    /* This is how WINE implements _vcsprintf, and they are obviously wrong */
    StartSeh()
#if defined(TEST_CRTDLL)
        call_varargs(NULL, INT_MAX, -1, L"%s it really work?", L"does");
#else
        if (GetNTVersion() >= _WIN32_WINNT_VISTA)
            call_varargs(NULL, INT_MAX, -1, L"%s it really work?", L"does");
        else
            call_varargs(NULL, INT_MAX, 20, L"%s it really work?", L"does");
#endif

#if defined(TEST_USER32)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

#if defined(TEST_USER32)
    ok_eq_uint(errno, EINVAL);
#elif defined(TEST_NTDLL) || defined(TEST_CRTDLL)
    ok_eq_uint(errno, 0);
#else
    if (GetNTVersion() >= _WIN32_WINNT_VISTA)
        ok_eq_uint(errno, ERROR_BAD_COMMAND);
    else
        ok_eq_uint(errno, 0);
#endif

    /* This one is no better */
    StartSeh()
#if defined(TEST_CRTDLL)
        call_varargs(NULL, 0, -1, L"%s it really work?", L"does");
#else
        call_varargs(NULL, 0, 20, L"%s it really work?", L"does");
#endif
    EndSeh(STATUS_SUCCESS);
#if defined(TEST_NTDLL) || defined(TEST_CRTDLL)
    ok_eq_uint(errno, 0);
#else
    if (GetNTVersion() >= _WIN32_WINNT_VISTA)
        ok_eq_uint(errno, ERROR_BAD_COMMAND);
    else
        ok_eq_uint(errno, 0);
#endif

    /* One more NULL checks */
    StartSeh()
        call_varargs(buffer, 255, -1, NULL);
#if defined(TEST_CRTDLL)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((GetNTVersion() >= _WIN32_WINNT_VISTA) ? 0 : STATUS_ACCESS_VIOLATION);
#endif
#if defined(TEST_NTDLL) || defined(TEST_CRTDLL)
    ok_eq_uint(errno, 0);
#else
    if (GetNTVersion() >= _WIN32_WINNT_VISTA)
        ok_eq_uint(errno, ERROR_BAD_COMMAND);
    else
        ok_eq_uint(errno, 0);
#endif
}
