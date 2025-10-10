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

#ifndef TEST_STATIC_CRT

typedef int (__cdecl *PFN_vsnprintf)(char *buf, size_t cnt, const char *fmt, va_list args);
static PFN_vsnprintf p_vsnprintf;

static BOOL Init(void)
{
    HMODULE hdll = LoadLibraryA(TEST_DLL_NAME);
    p_vsnprintf = (PFN_vsnprintf)GetProcAddress(hdll, "_vsnprintf");
    ok(p_vsnprintf != NULL, "Failed to load _vsnprintf from %s\n", TEST_DLL_NAME);
    return (p_vsnprintf != NULL);
}
#define _vsnprintf p_vsnprintf

#endif // !TEST_STATIC_CRT

static void call_varargs(char* buf, size_t buf_size, int expected_ret, LPCSTR formatString, ...)
{
    va_list args;
    int ret;
    /* Test the basic functionality */
    va_start(args, formatString);
    ret = _vsnprintf(buf, buf_size, formatString, args);
    va_end(args);
    ok(expected_ret == ret, "Test failed for `%s`: expected %i, got %i.\n", formatString, expected_ret, ret);
}

START_TEST(_vsnprintf)
{
    char buffer[255];

#ifndef TEST_STATIC_CRT
    if (!Init())
    {
        skip("Skipping tests, because _vsnprintf is not available\n");
        return;
    }
#endif

    /* Here you can mix wide and ANSI strings */
    call_varargs(buffer, 255, 12, "%S world!", L"hello");
    call_varargs(buffer, 255, 12, "%s world!", "hello");
    call_varargs(buffer, 255, 11, "%u cookies", 100);

    StartSeh()
#if defined(TEST_CRTDLL)||defined(TEST_USER32)
        call_varargs(NULL, INT_MAX, -1, "%s it really work?", "does");
#else
        if (GetNTVersion() >= _WIN32_WINNT_VISTA)
            call_varargs(NULL, INT_MAX, -1, "%s it really work?", "does");
        else
            call_varargs(NULL, INT_MAX, 20, "%s it really work?", "does");
#endif

#if defined(TEST_CRTDLL)||defined(TEST_USER32)
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
#if defined(TEST_CRTDLL)||defined(TEST_USER32)
        call_varargs(NULL, 0, -1, "%s it really work?", "does");
#else
        call_varargs(NULL, 0, 20, "%s it really work?", "does");
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

    /* One more NULL checks */
    StartSeh()
        call_varargs(buffer, 255, -1, NULL);
#if defined(TEST_CRTDLL)
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh((GetNTVersion() >= _WIN32_WINNT_VISTA) ? 0 : STATUS_ACCESS_VIOLATION);
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
}
