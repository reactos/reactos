/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for mbstowcs_s
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <specstrings.h>

#define ok_errno(x) ok_hex(errno, (x))
#define ok_wchar(x,y) ok_int(x,y)

errno_t
mbstowcs_s(
    size_t *cchConverted,
    wchar_t *widechar,
    size_t charoutct,
    const char *multibyte,
    size_t count);

#define MSVCRT_INVALID_PMT(x) _invalid_parameterA(#x, __FUNCTION__, __FILE__, __LINE__, 0)
#define MSVCRT_CHECK_PMT(x)   ((x) || (MSVCRT_INVALID_PMT(x),0))

wchar_t g_expression[64];
wchar_t g_function[64];
wchar_t g_file[128];
unsigned int g_line;
uintptr_t g_pReserved;

void
_test_invalid_parameter(
   const wchar_t * expression,
   const wchar_t * function,
   const wchar_t * file,
   unsigned int line,
   uintptr_t pReserved)
{
    wcsncpy(g_expression, expression, sizeof(g_expression) / sizeof(wchar_t));
    wcsncpy(g_function, function, sizeof(g_function) / sizeof(wchar_t));
    wcsncpy(g_file, file, sizeof(g_file) / sizeof(wchar_t));
    g_line = line;
    g_pReserved = pReserved;
}

START_TEST(mbstowcs_s)
{
//    _set_invalid_parameter_handler(_test_invalid_parameter);

    errno_t ret;
    size_t cchConverted;
    wchar_t widechar[10];

    *_errno() = 0;
    cchConverted = 0xf00bac;
    widechar[5] = 0xFF;
    ret = mbstowcs_s(&cchConverted, widechar, 6, "hallo", 5);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_wchar(widechar[5], 0);
    ok_wstr(widechar, L"hallo");
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    widechar[0] = 0xFF;
    ret = mbstowcs_s(&cchConverted, widechar, 1, "", 0);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 1);
    ok_wchar(widechar[0], 0);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    widechar[0] = 0xFF;
    widechar[1] = 0xFF;
    widechar[2] = 0xFF;
    widechar[3] = 0xFF;
    widechar[4] = 0xFF;
    widechar[5] = 0xFF;
    ret = mbstowcs_s(&cchConverted, widechar, 5, "hallo", 5);
    ok_long(ret, ERANGE);
    ok_size_t(cchConverted, 0);
    ok_wchar(widechar[5], 0xFF);
    ok_wchar(widechar[4], L'o');
    ok_wchar(widechar[3], L'l');
    ok_wchar(widechar[2], L'l');
    ok_wchar(widechar[1], L'a');
    ok_wchar(widechar[0], 0);
    ok_errno(ERANGE);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    widechar[0] = 0xFF;
    widechar[1] = 0xFF;
    widechar[2] = 0xFF;
    widechar[3] = 0xFF;
    widechar[4] = 0xFF;
    widechar[5] = 0xFF;
    ret = mbstowcs_s(&cchConverted, widechar, 3, "hallo", 5);
    ok_long(ret, ERANGE);
    ok_size_t(cchConverted, 0);
    ok_wchar(widechar[5], 0xFF);
    ok_wchar(widechar[4], 0xFF);
    ok_wchar(widechar[3], 0xFF);
    ok_wchar(widechar[2], L'l');
    ok_wchar(widechar[1], L'a');
    ok_wchar(widechar[0], 0);
    ok_errno(ERANGE);

    *_errno() = 0;
    ret = mbstowcs_s(0, 0, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_errno(EINVAL);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, 0, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_size_t(cchConverted, 0);
    ok_errno(EINVAL);

    *_errno() = 0;
    widechar[0] = L'x';
    ret = mbstowcs_s(0, widechar, 0, 0, 0);
    ok_long(ret, EINVAL);
    ok_wchar(widechar[0], L'x');
    ok_errno(EINVAL);

    *_errno() = 0;
    ret = mbstowcs_s(0, widechar, 10, "hallo", 5);
    ok_long(ret, 0);
    ok_errno(0);

    *_errno() = 0;
    ret = mbstowcs_s(0, widechar, 0, "hallo", 5);
    ok_long(ret, EINVAL);
    ok_errno(EINVAL);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, 0, 10, "hallo", 5);
    ok_long(ret, EINVAL);
    ok_size_t(cchConverted, 0xf00bac);
    ok_errno(EINVAL);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, 0, 0, "hallo", 5);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, widechar, 10, 0, 5);
    ok_long(ret, EINVAL);
    ok_size_t(cchConverted, 0);
    ok_errno(EINVAL);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, widechar, 10, "hallo", 0);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 1);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    widechar[0] = 0xABCD;
    widechar[1] = 0xABCD;
    widechar[2] = 0xABCD;
    widechar[3] = 0xABCD;
    widechar[4] = 0xABCD;
    widechar[5] = 0xABCD;
    ret = mbstowcs_s(&cchConverted, widechar, 10, "hallo", 2);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 3);
    ok_wchar(widechar[5], 0xABCD);
    ok_wchar(widechar[4], 0xABCD);
    ok_wchar(widechar[3], 0xABCD);
    ok_wchar(widechar[2], 0);
    ok_wchar(widechar[1], L'a');
    ok_wchar(widechar[0], L'h');
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, widechar, 10, 0, 0);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 1);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, widechar, 10, "hallo", 7);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, 0, 0, "hallo", 7);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    widechar[0] = 0xABCD;
    widechar[1] = 0xABCD;
    widechar[2] = 0xABCD;
    widechar[3] = 0xABCD;
    widechar[4] = 0xABCD;
    widechar[5] = 0xABCD;
    ret = mbstowcs_s(&cchConverted, widechar, 5, "hallo", _TRUNCATE);
    ok_long(ret, STRUNCATE);
    ok_size_t(cchConverted, 5);
    ok_wchar(widechar[5], 0xABCD);
    ok_wchar(widechar[4], 0);
    ok_wchar(widechar[3], L'l');
    ok_wchar(widechar[2], L'l');
    ok_wchar(widechar[1], L'a');
    ok_wchar(widechar[0], L'h');
    ok_errno(0);

    *_errno() = 0;
    cchConverted = 0xf00bac;
    ret = mbstowcs_s(&cchConverted, widechar, 10, "hallo", -1);
    ok_long(ret, 0);
    ok_size_t(cchConverted, 6);
    ok_errno(0);
}
