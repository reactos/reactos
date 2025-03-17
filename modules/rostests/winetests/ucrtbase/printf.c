/*
 * Conformance tests for *printf functions.
 *
 * Copyright 2002 Uwe Bonnes
 * Copyright 2004 Aneurin Price
 * Copyright 2005 Mike McCormack
 * Copyright 2015 Martin Storsjo
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <inttypes.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(invalid_parameter_handler);

static inline float __port_ind(void)
{
    static const unsigned __ind_bytes = 0xffc00000;
    return *(const float *)&__ind_bytes;
}
#define IND __port_ind()

static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(invalid_parameter_handler);
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %Ix\n", arg);
}

static int WINAPIV vsprintf_wrapper(unsigned __int64 options, char *str,
                                    size_t len, const char *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = __stdio_common_vsprintf(options, str, len, format, NULL, valist);
    va_end(valist);
    return ret;
}

static void test_snprintf (void)
{
    const char *tests[] = {"short", "justfit", "justfits", "muchlonger", "", "1"};
    char buffer[8];
    int bufsizes[] = { 0, 1, sizeof(buffer) };
    unsigned int i, j;

    for (j = 0; j < ARRAY_SIZE(bufsizes); j++) {
        const int bufsiz = bufsizes[j];
        /* Legacy _snprintf style termination */
        for (i = 0; i < ARRAY_SIZE(tests); i++) {
            const char *fmt  = tests[i];
            const int expect = strlen(fmt) > bufsiz ? -1 : strlen(fmt);
            const int n      = vsprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, buffer, bufsiz, fmt);
            const int valid  = n < 0 ? bufsiz : (n == bufsiz ? n : n+1);

            ok (n == expect, "\"%s\": expected %d, returned %d\n",
                fmt, expect, n);
            ok (!memcmp (fmt, buffer, valid),
                "\"%s\": rendered \"%.*s\"\n", fmt, valid, buffer);
        }

        /* C99 snprintf style termination */
        for (i = 0; i < ARRAY_SIZE(tests); i++) {
            const char *fmt  = tests[i];
            const int expect = strlen(fmt);
            const int n      = vsprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, buffer, bufsiz, fmt);
            const int valid  = n >= bufsiz ? (bufsiz > 0 ? bufsiz - 1 : 0) : n < 0 ? 0 : n;

            ok (n == expect, "\"%s\": expected %d, returned %d\n",
                fmt, expect, n);
            ok (!memcmp (fmt, buffer, valid),
                "\"%s\": rendered \"%.*s\" bufsiz %d\n", fmt, valid, buffer, bufsiz);
            ok (bufsiz == 0 || buffer[valid] == '\0',
                "\"%s\": Missing null termination (ret %d) - is %d (bufsiz %d)\n", fmt, n, buffer[valid], bufsiz);
        }

        /* swprintf style termination */
        for (i = 0; i < ARRAY_SIZE(tests); i++) {
            const char *fmt  = tests[i];
            const int expect = strlen(fmt) >= bufsiz ? bufsiz > 0 ? -2 : -1 : strlen(fmt);
            const int n      = vsprintf_wrapper (0, buffer, bufsiz, fmt);
            const int valid  = n < 0 ? bufsiz > 0 ? bufsiz - 1 : 0 : n;

            ok (n == expect, "\"%s\": expected %d, returned %d\n",
                fmt, expect, n);
            ok (!memcmp (fmt, buffer, valid),
                "\"%s\": rendered \"%.*s\" bufsiz %d\n", fmt, valid, buffer, bufsiz);
            ok (bufsiz == 0 || buffer[valid] == '\0',
                "\"%s\": Missing null termination (ret %d) - is %d\n", fmt, n, buffer[valid]);
        }
    }

    ok (vsprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, NULL, 0, "abcd") == 4,
        "Failure to snprintf to NULL\n");
    ok (vsprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, NULL, 0, "abcd") == 4,
        "Failure to snprintf to NULL\n");
    ok (vsprintf_wrapper (0, NULL, 0, "abcd") == 4,
        "Failure to snprintf to NULL\n");
    ok (vsprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, buffer, 0, "abcd") == 4,
        "Failure to snprintf to zero length buffer\n");
    ok (vsprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, buffer, 0, "abcd") == -1,
        "Failure to snprintf to zero length buffer\n");
    ok (vsprintf_wrapper (0, buffer, 0, "abcd") == -1,
        "Failure to snprintf to zero length buffer\n");
    ok (vsprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, buffer, 0, "") == 0,
        "Failure to snprintf a zero length string to a zero length buffer\n");
    ok (vsprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, buffer, 0, "") == 0,
        "Failure to snprintf a zero length string to a zero length buffer\n");
    ok (vsprintf_wrapper (0, buffer, 0, "") == -1,
        "Failure to snprintf a zero length string to a zero length buffer\n");
}

static int WINAPIV vswprintf_wrapper(unsigned __int64 options, wchar_t *str,
                                     size_t len, const wchar_t *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = __stdio_common_vswprintf(options, str, len, format, NULL, valist);
    va_end(valist);
    return ret;
}

static void test_swprintf (void)
{
    const wchar_t str_short[]      = {'s','h','o','r','t',0};
    const wchar_t str_justfit[]    = {'j','u','s','t','f','i','t',0};
    const wchar_t str_justfits[]   = {'j','u','s','t','f','i','t','s',0};
    const wchar_t str_muchlonger[] = {'m','u','c','h','l','o','n','g','e','r',0};
    const wchar_t str_empty[]      = {0};
    const wchar_t *tests[] = {str_short, str_justfit, str_justfits, str_muchlonger};

    wchar_t buffer[8];
    char narrow[8], narrow_fmt[16];
    const int bufsiz = ARRAY_SIZE(buffer);
    unsigned int i;

    /* Legacy _snprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const wchar_t *fmt = tests[i];
        const int expect   = wcslen(fmt) > bufsiz ? -1 : wcslen(fmt);
        const int n        = vswprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, buffer, bufsiz, fmt);
        const int valid    = n < 0 ? bufsiz : (n == bufsiz ? n : n+1);

        WideCharToMultiByte (CP_ACP, 0, buffer, -1, narrow, sizeof(narrow), NULL, NULL);
        WideCharToMultiByte (CP_ACP, 0, fmt, -1, narrow_fmt, sizeof(narrow_fmt), NULL, NULL);
        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            narrow_fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid * sizeof(wchar_t)),
            "\"%s\": rendered \"%.*s\"\n", narrow_fmt, valid, narrow);
    }

    /* C99 snprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const wchar_t *fmt = tests[i];
        const int expect   = wcslen(fmt);
        const int n        = vswprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, buffer, bufsiz, fmt);
        const int valid    = n >= bufsiz ? bufsiz - 1 : n < 0 ? 0 : n;

        WideCharToMultiByte (CP_ACP, 0, buffer, -1, narrow, sizeof(narrow), NULL, NULL);
        WideCharToMultiByte (CP_ACP, 0, fmt, -1, narrow_fmt, sizeof(narrow_fmt), NULL, NULL);
        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            narrow_fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid * sizeof(wchar_t)),
            "\"%s\": rendered \"%.*s\"\n", narrow_fmt, valid, narrow);
        ok (buffer[valid] == '\0',
            "\"%s\": Missing null termination (ret %d) - is %d\n", narrow_fmt, n, buffer[valid]);
    }

    /* swprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const wchar_t *fmt = tests[i];
        const int expect   = wcslen(fmt) >= bufsiz ? -2 : wcslen(fmt);
        const int n        = vswprintf_wrapper (0, buffer, bufsiz, fmt);
        const int valid    = n < 0 ? bufsiz - 1 : n;

        WideCharToMultiByte (CP_ACP, 0, buffer, -1, narrow, sizeof(narrow), NULL, NULL);
        WideCharToMultiByte (CP_ACP, 0, fmt, -1, narrow_fmt, sizeof(narrow_fmt), NULL, NULL);
        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            narrow_fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid * sizeof(wchar_t)),
            "\"%s\": rendered \"%.*s\"\n", narrow_fmt, valid, narrow);
        ok (buffer[valid] == '\0',
            "\"%s\": Missing null termination (ret %d) - is %d\n", narrow_fmt, n, buffer[valid]);
    }

    ok (vswprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, NULL, 0, str_short) == 5,
        "Failure to swprintf to NULL\n");
    ok (vswprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, NULL, 0, str_short) == 5,
        "Failure to swprintf to NULL\n");
    ok (vswprintf_wrapper (0, NULL, 0, str_short) == 5,
        "Failure to swprintf to NULL\n");
    ok (vswprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, buffer, 0, str_short) == 5,
        "Failure to swprintf to a zero length buffer\n");
    ok (vswprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, buffer, 0, str_short) == -1,
        "Failure to swprintf to a zero length buffer\n");
    ok (vswprintf_wrapper (0, buffer, 0, str_short) == -1,
        "Failure to swprintf to a zero length buffer\n");
    ok (vswprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, buffer, 0, str_empty) == 0,
        "Failure to swprintf a zero length string to a zero length buffer\n");
    ok (vswprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, buffer, 0, str_empty) == 0,
        "Failure to swprintf a zero length string to a zero length buffer\n");
    ok (vswprintf_wrapper (0, buffer, 0, str_empty) == -1,
        "Failure to swprintf a zero length string to a zero length buffer\n");
}

static int WINAPIV vfprintf_wrapper(FILE *file,
                                    const char *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = __stdio_common_vfprintf(0, file, format, NULL, valist);
    va_end(valist);
    return ret;
}

static void test_fprintf(void)
{
    static const char file_name[] = "fprintf.tst";

    FILE *fp = fopen(file_name, "wb");
    char buf[1024];
    int ret;

    ret = vfprintf_wrapper(fp, "simple test\n");
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 12, "ftell returned %d\n", ret);

    ret = vfprintf_wrapper(fp, "contains%cnull\n", '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 26, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 12, "ftell returned %d\n", ret);
    ok(!strcmp(buf, "simple test\n"), "buf = %s\n", buf);

    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 26, "ret = %d\n", ret);
    ok(!memcmp(buf, "contains\0null\n", 14), "buf = %s\n", buf);

    fclose(fp);

    fp = fopen(file_name, "wt");

    ret = vfprintf_wrapper(fp, "simple test\n");
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);

    ret = vfprintf_wrapper(fp, "contains%cnull\n", '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 28, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);
    ok(!strcmp(buf, "simple test\r\n"), "buf = %s\n", buf);

    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 28, "ret = %d\n", ret);
    ok(!memcmp(buf, "contains\0null\r\n", 15), "buf = %s\n", buf);

    fclose(fp);
    unlink(file_name);
}

static int WINAPIV vfwprintf_wrapper(FILE *file,
                                     const wchar_t *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = __stdio_common_vfwprintf(0, file, format, NULL, valist);
    va_end(valist);
    return ret;
}

static void test_fwprintf(void)
{
    static const char file_name[] = "fprintf.tst";
    static const WCHAR simple[] = {'s','i','m','p','l','e',' ','t','e','s','t','\n',0};
    static const WCHAR cont_fmt[] = {'c','o','n','t','a','i','n','s','%','c','n','u','l','l','\n',0};
    static const WCHAR cont[] = {'c','o','n','t','a','i','n','s','\0','n','u','l','l','\n',0};

    FILE *fp = fopen(file_name, "wb");
    wchar_t bufw[1024];
    char bufa[1024];
    int ret;

    ret = vfwprintf_wrapper(fp, simple);
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 24, "ftell returned %d\n", ret);

    ret = vfwprintf_wrapper(fp, cont_fmt, '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 52, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    fgetws(bufw, ARRAY_SIZE(bufw), fp);
    ret = ftell(fp);
    ok(ret == 24, "ftell returned %d\n", ret);
    ok(!wcscmp(bufw, simple), "buf = %s\n", wine_dbgstr_w(bufw));

    fgetws(bufw, ARRAY_SIZE(bufw), fp);
    ret = ftell(fp);
    ok(ret == 52, "ret = %d\n", ret);
    ok(!memcmp(bufw, cont, 28), "buf = %s\n", wine_dbgstr_w(bufw));

    fclose(fp);

    fp = fopen(file_name, "wt");

    ret = vfwprintf_wrapper(fp, simple);
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);

    ret = vfwprintf_wrapper(fp, cont_fmt, '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 28, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    fgets(bufa, sizeof(bufa), fp);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);
    ok(!strcmp(bufa, "simple test\r\n"), "buf = %s\n", bufa);

    fgets(bufa, sizeof(bufa), fp);
    ret = ftell(fp);
    ok(ret == 28, "ret = %d\n", ret);
    ok(!memcmp(bufa, "contains\0null\r\n", 15), "buf = %s\n", bufa);

    fclose(fp);
    unlink(file_name);

    /* NULL format */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = vfwprintf_wrapper(fp, NULL);
    ok(errno == EINVAL, "expected errno EINVAL, got %d\n", errno);
    ok(ret == -1, "expected ret -1, got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    /* NULL file */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = vfwprintf_wrapper(NULL, simple);
    ok(errno == EINVAL, "expected errno EINVAL, got %d\n", errno);
    ok(ret == -1, "expected ret -1, got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    /* format using % with NULL arglist*/
    /* crashes on Windows */
    /* ret = __stdio_common_vfwprintf(0, fp, cont_fmt, NULL, NULL); */
}

static int WINAPIV _vsnprintf_s_wrapper(char *str, size_t sizeOfBuffer,
                                        size_t count, const char *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = __stdio_common_vsnprintf_s(0, str, sizeOfBuffer, count, format, NULL, valist);
    va_end(valist);
    return ret;
}

static void test_vsnprintf_s(void)
{
    const char format[] = "AB%uC";
    const char out7[] = "AB123C";
    const char out6[] = "AB123";
    const char out2[] = "A";
    const char out1[] = "";
    char buffer[14] = { 0 };
    int exp, got;

    /* Enough room. */
    exp = strlen(out7);

    got = _vsnprintf_s_wrapper(buffer, 14, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out7, buffer), "buffer wrong, got=%s\n", buffer);

    got = _vsnprintf_s_wrapper(buffer, 12, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out7, buffer), "buffer wrong, got=%s\n", buffer);

    got = _vsnprintf_s_wrapper(buffer, 7, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out7, buffer), "buffer wrong, got=%s\n", buffer);

    /* Not enough room. */
    exp = -1;

    got = _vsnprintf_s_wrapper(buffer, 6, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out6, buffer), "buffer wrong, got=%s\n", buffer);

    got = _vsnprintf_s_wrapper(buffer, 2, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out2, buffer), "buffer wrong, got=%s\n", buffer);

    got = _vsnprintf_s_wrapper(buffer, 1, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out1, buffer), "buffer wrong, got=%s\n", buffer);
}

static int WINAPIV _vsnwprintf_s_wrapper(WCHAR *str, size_t sizeOfBuffer,
                                        size_t count, const WCHAR *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = __stdio_common_vsnwprintf_s(0, str, sizeOfBuffer, count, format, NULL, valist);
    va_end(valist);
    return ret;
}

static void test_vsnwprintf_s(void)
{
    const WCHAR format[] = {'A','B','%','u','C',0};
    const WCHAR out7[] = {'A','B','1','2','3','C',0};
    const WCHAR out6[] = {'A','B','1','2','3',0};
    const WCHAR out2[] = {'A',0};
    const WCHAR out1[] = {0};
    WCHAR buffer[14] = { 0 };
    int exp, got;

    /* Enough room. */
    exp = lstrlenW(out7);

    got = _vsnwprintf_s_wrapper(buffer, 14, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 12, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 7, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    /* Not enough room. */
    exp = -1;

    got = _vsnwprintf_s_wrapper(buffer, 6, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out6, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 2, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out2, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 1, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out1, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));
}

static void test_printf_legacy_wide(void)
{
    const wchar_t wide[] = {'A','B','C','D',0};
    const char narrow[] = "abcd";
    const char out[] = "abcd ABCD";
    /* The legacy wide flag doesn't affect narrow printfs, so the same
     * format should behave the same both with and without the flag. */
    const char narrow_fmt[] = "%s %ls";
    /* The standard behaviour is to use the same format as for the narrow
     * case, while the legacy case has got a different meaning for %s. */
    const wchar_t std_wide_fmt[] = {'%','s',' ','%','l','s',0};
    const wchar_t legacy_wide_fmt[] = {'%','h','s',' ','%','s',0};
    char buffer[20];
    wchar_t wbuffer[20];

    vsprintf_wrapper(0, buffer, sizeof(buffer), narrow_fmt, narrow, wide);
    ok(!strcmp(buffer, out), "buffer wrong, got=%s\n", buffer);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS, buffer, sizeof(buffer), narrow_fmt, narrow, wide);
    ok(!strcmp(buffer, out), "buffer wrong, got=%s\n", buffer);

    vswprintf_wrapper(0, wbuffer, sizeof(wbuffer), std_wide_fmt, narrow, wide);
    WideCharToMultiByte(CP_ACP, 0, wbuffer, -1, buffer, sizeof(buffer), NULL, NULL);
    ok(!strcmp(buffer, out), "buffer wrong, got=%s\n", buffer);
    vswprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS, wbuffer, sizeof(wbuffer), legacy_wide_fmt, narrow, wide);
    WideCharToMultiByte(CP_ACP, 0, wbuffer, -1, buffer, sizeof(buffer), NULL, NULL);
    ok(!strcmp(buffer, out), "buffer wrong, got=%s\n", buffer);
}

static void test_printf_legacy_msvcrt(void)
{
    char buf[50];

    /* In standard mode, %F is a float format conversion, while it is a
     * length modifier in legacy msvcrt mode. In legacy mode, N is also
     * a length modifier. */
    vsprintf_wrapper(0, buf, sizeof(buf), "%F", 1.23);
    ok(!strcmp(buf, "1.230000"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY, buf, sizeof(buf), "%Fd %Nd", 123, 456);
    ok(!strcmp(buf, "123 456"), "buf = %s\n", buf);

    vsprintf_wrapper(0, buf, sizeof(buf), "%f %F %f %e %E %g %G", INFINITY, INFINITY, -INFINITY, INFINITY, INFINITY, INFINITY, INFINITY);
    ok(!strcmp(buf, "inf INF -inf inf INF inf INF"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY, buf, sizeof(buf), "%f", INFINITY);
    ok(!strcmp(buf, "1.#INF00"), "buf = %s\n", buf);
    vsprintf_wrapper(0, buf, sizeof(buf), "%f %F", NAN, NAN);
    ok(!strcmp(buf, "nan NAN"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY, buf, sizeof(buf), "%f", NAN);
    ok(!strcmp(buf, "1.#QNAN0"), "buf = %s\n", buf);
    vsprintf_wrapper(0, buf, sizeof(buf), "%f %F", IND, IND);
    ok(!strcmp(buf, "-nan(ind) -NAN(IND)"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY, buf, sizeof(buf), "%f", IND);
    ok(!strcmp(buf, "-1.#IND00"), "buf = %s\n", buf);
}

static void test_printf_legacy_three_digit_exp(void)
{
    char buf[20];

    vsprintf_wrapper(0, buf, sizeof(buf), "%E", 1.23);
    ok(!strcmp(buf, "1.230000E+00"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS, buf, sizeof(buf), "%E", 1.23);
    ok(!strcmp(buf, "1.230000E+000"), "buf = %s\n", buf);
    vsprintf_wrapper(0, buf, sizeof(buf), "%E", 1.23e+123);
    ok(!strcmp(buf, "1.230000E+123"), "buf = %s\n", buf);
}

static void test_printf_c99(void)
{
    char buf[30];
    int i;

    /* The msvcrt compatibility flag doesn't affect whether 'z' is interpreted
     * as size_t size for integers. */
     for (i = 0; i < 2; i++) {
        unsigned __int64 options = (i == 0) ? 0 :
            _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY;

        /* z modifier accepts size_t argument */
        vsprintf_wrapper(options, buf, sizeof(buf), "%zx %d", SIZE_MAX, 1);
        if (sizeof(size_t) == 8)
            ok(!strcmp(buf, "ffffffffffffffff 1"), "buf = %s\n", buf);
        else
            ok(!strcmp(buf, "ffffffff 1"), "buf = %s\n", buf);

        /* j modifier with signed format accepts intmax_t argument */
        vsprintf_wrapper(options, buf, sizeof(buf), "%jd %d", INTMAX_MIN, 1);
        ok(!strcmp(buf, "-9223372036854775808 1"), "buf = %s\n", buf);

        /* j modifier with unsigned format accepts uintmax_t argument */
        vsprintf_wrapper(options, buf, sizeof(buf), "%ju %d", UINTMAX_MAX, 1);
        ok(!strcmp(buf, "18446744073709551615 1"), "buf = %s\n", buf);

        /* t modifier accepts ptrdiff_t argument */
        vsprintf_wrapper(options, buf, sizeof(buf), "%td %d", PTRDIFF_MIN, 1);
        if (sizeof(ptrdiff_t) == 8)
            ok(!strcmp(buf, "-9223372036854775808 1"), "buf = %s\n", buf);
        else
            ok(!strcmp(buf, "-2147483648 1"), "buf = %s\n", buf);
    }
}

static void test_printf_natural_string(void)
{
    const wchar_t wide[] = {'A','B','C','D',0};
    const char narrow[] = "abcd";
    const char narrow_fmt[] = "%s %Ts";
    const char narrow_out[] = "abcd abcd";
    const wchar_t wide_fmt[] = {'%','s',' ','%','T','s',0};
    const wchar_t wide_out[] = {'a','b','c','d',' ','A','B','C','D',0};
    char buffer[20];
    wchar_t wbuffer[20];

    vsprintf_wrapper(0, buffer, sizeof(buffer), narrow_fmt, narrow, narrow);
    ok(!strcmp(buffer, narrow_out), "buffer wrong, got=%s\n", buffer);

    vswprintf_wrapper(0, wbuffer, sizeof(wbuffer), wide_fmt, narrow, wide);
    ok(!lstrcmpW(wbuffer, wide_out), "buffer wrong, got=%s\n", wine_dbgstr_w(wbuffer));
}

static void test_printf_fp(void)
{
    static const int flags[] = {
        0,
        _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY,
        _CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS,
        _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY
            | _CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS,
        _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY
            | _CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS
            | _CRT_INTERNAL_PRINTF_STANDARD_ROUNDING
    };
    const struct {
        const char *fmt;
        double d;
        const char *res[ARRAY_SIZE(flags)];
        const char *broken[ARRAY_SIZE(flags)];
    } tests[] = {
        { "%a", NAN, { "nan", "0x1.#QNAN00000000p+0", "nan", "0x1.#QNAN00000000p+0" }},
        { "%A", NAN, { "NAN", "0X1.#QNAN00000000P+0", "NAN", "0X1.#QNAN00000000P+0" }},
        { "%e", NAN, { "nan", "1.#QNAN0e+00", "nan", "1.#QNAN0e+000" }},
        { "%E", NAN, { "NAN", "1.#QNAN0E+00", "NAN", "1.#QNAN0E+000" }},
        { "%g", NAN, { "nan", "1.#QNAN", "nan", "1.#QNAN" }},
        { "%G", NAN, { "NAN", "1.#QNAN", "NAN", "1.#QNAN" }},
        { "%21a", NAN, { "                  nan", " 0x1.#QNAN00000000p+0", "                  nan", " 0x1.#QNAN00000000p+0" }},
        { "%20e", NAN, { "                 nan", "        1.#QNAN0e+00", "                 nan", "       1.#QNAN0e+000" }},
        { "%20g", NAN, { "                 nan", "             1.#QNAN", "                 nan", "             1.#QNAN" }},
        { "%.21a", NAN, { "nan", "0x1.#QNAN0000000000000000p+0", "nan", "0x1.#QNAN0000000000000000p+0" }},
        { "%.20e", NAN, { "nan", "1.#QNAN000000000000000e+00", "nan", "1.#QNAN000000000000000e+000" }},
        { "%.20g", NAN, { "nan", "1.#QNAN", "nan", "1.#QNAN" }},
        { "%.021a", NAN, { "nan", "0x1.#QNAN0000000000000000p+0", "nan", "0x1.#QNAN0000000000000000p+0" }},
        { "%.020e", NAN, { "nan", "1.#QNAN000000000000000e+00", "nan", "1.#QNAN000000000000000e+000" }},
        { "%.020g", NAN, { "nan", "1.#QNAN", "nan", "1.#QNAN" }},
        { "%#.21a", NAN, { "nan", "0x1.#QNAN0000000000000000p+0", "nan", "0x1.#QNAN0000000000000000p+0" }},
        { "%#.20e", NAN, { "nan", "1.#QNAN000000000000000e+00", "nan", "1.#QNAN000000000000000e+000" }},
        { "%#.20g", NAN, { "nan", "1.#QNAN00000000000000", "nan", "1.#QNAN00000000000000" }},
        { "%.1g", NAN, { "nan", "1", "nan", "1" }},
        { "%.2g", NAN, { "nan", "1.$", "nan", "1.$" }},
        { "%.3g", NAN, { "nan", "1.#R", "nan", "1.#R" }},

        { "%a", IND, { "-nan(ind)", "-0x1.#IND000000000p+0", "-nan(ind)", "-0x1.#IND000000000p+0" }},
        { "%e", IND, { "-nan(ind)", "-1.#IND00e+00", "-nan(ind)", "-1.#IND00e+000" }},
        { "%g", IND, { "-nan(ind)", "-1.#IND", "-nan(ind)", "-1.#IND" }},
        { "%21a", IND, { "            -nan(ind)", "-0x1.#IND000000000p+0", "            -nan(ind)", "-0x1.#IND000000000p+0" }},
        { "%20e", IND, { "           -nan(ind)", "       -1.#IND00e+00", "           -nan(ind)", "      -1.#IND00e+000" }},
        { "%20g", IND, { "           -nan(ind)", "             -1.#IND", "           -nan(ind)", "             -1.#IND" }},
        { "%.21a", IND, { "-nan(ind)", "-0x1.#IND00000000000000000p+0", "-nan(ind)", "-0x1.#IND00000000000000000p+0" }},
        { "%.20e", IND, { "-nan(ind)", "-1.#IND0000000000000000e+00", "-nan(ind)", "-1.#IND0000000000000000e+000" }},
        { "%.20g", IND, { "-nan(ind)", "-1.#IND", "-nan(ind)", "-1.#IND" }},
        { "%.021a", IND, { "-nan(ind)", "-0x1.#IND00000000000000000p+0", "-nan(ind)", "-0x1.#IND00000000000000000p+0" }},
        { "%.020e", IND, { "-nan(ind)", "-1.#IND0000000000000000e+00", "-nan(ind)", "-1.#IND0000000000000000e+000" }},
        { "%.020g", IND, { "-nan(ind)", "-1.#IND", "-nan(ind)", "-1.#IND" }},
        { "%#.21a", IND, { "-nan(ind)", "-0x1.#IND00000000000000000p+0", "-nan(ind)", "-0x1.#IND00000000000000000p+0" }},
        { "%#.20e", IND, { "-nan(ind)", "-1.#IND0000000000000000e+00", "-nan(ind)", "-1.#IND0000000000000000e+000" }},
        { "%#.20g", IND, { "-nan(ind)", "-1.#IND000000000000000", "-nan(ind)", "-1.#IND000000000000000" }},

        { "%a", INFINITY, { "inf", "0x1.#INF000000000p+0", "inf", "0x1.#INF000000000p+0" }},
        { "%e", INFINITY, { "inf", "1.#INF00e+00", "inf", "1.#INF00e+000" }},
        { "%g", INFINITY, { "inf", "1.#INF", "inf", "1.#INF" }},
        { "%21a", INFINITY, { "                  inf", " 0x1.#INF000000000p+0", "                  inf", " 0x1.#INF000000000p+0" }},
        { "%20e", INFINITY, { "                 inf", "        1.#INF00e+00", "                 inf", "       1.#INF00e+000" }},
        { "%20g", INFINITY, { "                 inf", "              1.#INF", "                 inf", "              1.#INF" }},
        { "%.21a", INFINITY, { "inf", "0x1.#INF00000000000000000p+0", "inf", "0x1.#INF00000000000000000p+0" }},
        { "%.20e", INFINITY, { "inf", "1.#INF0000000000000000e+00", "inf", "1.#INF0000000000000000e+000" }},
        { "%.20g", INFINITY, { "inf", "1.#INF", "inf", "1.#INF" }},
        { "%.021a", INFINITY, { "inf", "0x1.#INF00000000000000000p+0", "inf", "0x1.#INF00000000000000000p+0" }},
        { "%.020e", INFINITY, { "inf", "1.#INF0000000000000000e+00", "inf", "1.#INF0000000000000000e+000" }},
        { "%.020g", INFINITY, { "inf", "1.#INF", "inf", "1.#INF" }},
        { "%#.21a", INFINITY, { "inf", "0x1.#INF00000000000000000p+0", "inf", "0x1.#INF00000000000000000p+0" }},
        { "%#.20e", INFINITY, { "inf", "1.#INF0000000000000000e+00", "inf", "1.#INF0000000000000000e+000" }},
        { "%#.20g", INFINITY, { "inf", "1.#INF000000000000000", "inf", "1.#INF000000000000000" }},

        { "%a", -INFINITY, { "-inf", "-0x1.#INF000000000p+0", "-inf", "-0x1.#INF000000000p+0" }},
        { "%e", -INFINITY, { "-inf", "-1.#INF00e+00", "-inf", "-1.#INF00e+000" }},
        { "%g", -INFINITY, { "-inf", "-1.#INF", "-inf", "-1.#INF" }},
        { "%21a", -INFINITY, { "                 -inf", "-0x1.#INF000000000p+0", "                 -inf", "-0x1.#INF000000000p+0" }},
        { "%20e", -INFINITY, { "                -inf", "       -1.#INF00e+00", "                -inf", "      -1.#INF00e+000" }},
        { "%20g", -INFINITY, { "                -inf", "             -1.#INF", "                -inf", "             -1.#INF" }},
        { "%.21a", -INFINITY, { "-inf", "-0x1.#INF00000000000000000p+0", "-inf", "-0x1.#INF00000000000000000p+0" }},
        { "%.20e", -INFINITY, { "-inf", "-1.#INF0000000000000000e+00", "-inf", "-1.#INF0000000000000000e+000" }},
        { "%.20g", -INFINITY, { "-inf", "-1.#INF", "-inf", "-1.#INF" }},
        { "%.021a", -INFINITY, { "-inf", "-0x1.#INF00000000000000000p+0", "-inf", "-0x1.#INF00000000000000000p+0" }},
        { "%.020e", -INFINITY, { "-inf", "-1.#INF0000000000000000e+00", "-inf", "-1.#INF0000000000000000e+000" }},
        { "%.020g", -INFINITY, { "-inf", "-1.#INF", "-inf", "-1.#INF" }},
        { "%#.21a", -INFINITY, { "-inf", "-0x1.#INF00000000000000000p+0", "-inf", "-0x1.#INF00000000000000000p+0" }},
        { "%#.20e", -INFINITY, { "-inf", "-1.#INF0000000000000000e+00", "-inf", "-1.#INF0000000000000000e+000" }},
        { "%#.20g", -INFINITY, { "-inf", "-1.#INF000000000000000", "-inf", "-1.#INF000000000000000" }},

        { "%a", 0, { "0x0.0000000000000p+0" }},
        { "%A", 0, { "0X0.0000000000000P+0" }},
        { "%a", 0.5, { "0x1.0000000000000p-1" }},
        { "%a", 1, { "0x1.0000000000000p+0" }},
        { "%a", 20, { "0x1.4000000000000p+4" }},
        { "%a", -1, { "-0x1.0000000000000p+0" }},
        { "%a", 0.1, { "0x1.999999999999ap-4" }},
        { "%24a", 0.1, { "    0x1.999999999999ap-4" }},
        { "%024a", 0.1, { "0x00001.999999999999ap-4" }},
        { "%.2a", 0.1, { "0x1.9ap-4" }},
        { "%.20a", 0.1, { "0x1.999999999999a0000000p-4" }},
        { "%.a", 0.1e-20, { "0x1p-70" }},
        { "%a", 0.1e-20, { "0x1.2e3b40a0e9b4fp-70" }},
        { "%a", 4.9406564584124654e-324, { "0x0.0000000000001p-1022" }},
        { "%.0a", -1.5, { "-0x2p+0" }, { "-0x1p+0" }},
        { "%.0a", -0.5, { "-0x1p-1" }},
        { "%.0a", 0.5, { "0x1p-1" }},
        { "%.0a", 1.5, { "0x2p+0" }, { "0x1p+0" }},
        { "%.0a", 1.99, { "0x2p+0" }},
        { "%.0a", 2, { "0x1p+1" }},
        { "%.0a", 9.5, { "0x1p+3" }},
        { "%.0a", 10.5, { "0x1p+3" }},
        { "%#.0a", -1.5, { "-0x2.p+0" }, { "-0x1.p+0" }},
        { "%#.0a", -0.5, { "-0x1.p-1" }},
        { "%#.0a", 0.5, { "0x1.p-1" }},
        { "%#.0a", 1.5, { "0x2.p+0" }, { "0x1.p+0" }},
        { "%#.1a", 1.03125, { "0x1.1p+0", NULL, NULL, NULL, "0x1.0p+0" }, { "0x1.0p+0" }},
        { "%#.1a", 1.09375, { "0x1.2p+0" }, { "0x1.1p+0" }},
        { "%#.1a", 1.15625, { "0x1.3p+0", NULL, NULL, NULL, "0x1.2p+0" }, { "0x1.2p+0" }},

        { "%f", 0, { "0.000000" }},
        { "%e", 0, { "0.000000e+00", NULL, "0.000000e+000" }},
        { "%g", 0, { "0" }},
        { "%21f", 0, { "             0.000000" }},
        { "%20e", 0, { "        0.000000e+00", NULL, "       0.000000e+000" }},
        { "%20g", 0, { "                   0" }},
        { "%.21f", 0, { "0.000000000000000000000" }},
        { "%.20e", 0, { "0.00000000000000000000e+00", NULL, "0.00000000000000000000e+000" }},
        { "%.20g", 0, { "0" }},
        { "%.021f", 0, { "0.000000000000000000000" }},
        { "%.020e", 0, { "0.00000000000000000000e+00", NULL, "0.00000000000000000000e+000" }},
        { "%.020g", 0, { "0" }},
        { "%#.21f", 0, { "0.000000000000000000000" }},
        { "%#.20e", 0, { "0.00000000000000000000e+00", NULL, "0.00000000000000000000e+000" }},
        { "%#.20g", 0, { "0.0000000000000000000" }, { "0.00000000000000000000" }},

        { "%f", 123, { "123.000000" }},
        { "%e", 123, { "1.230000e+02", NULL, "1.230000e+002" }},
        { "%g", 123, { "123" }},
        { "%21f", 123, { "           123.000000" }},
        { "%20e", 123, { "        1.230000e+02", NULL, "       1.230000e+002" }},
        { "%20g", 123, { "                 123" }},
        { "%.21f", 123, { "123.000000000000000000000" }},
        { "%.20e", 123, { "1.23000000000000000000e+02", NULL, "1.23000000000000000000e+002" }},
        { "%.20g", 123, { "123" }},
        { "%.021f", 123, { "123.000000000000000000000" }},
        { "%.020e", 123, { "1.23000000000000000000e+02", NULL, "1.23000000000000000000e+002" }},
        { "%.020g", 123, { "123" }},
        { "%#.21f", 123, { "123.000000000000000000000" }},
        { "%#.20e", 123, { "1.23000000000000000000e+02", NULL, "1.23000000000000000000e+002" }},
        { "%#.20g", 123, { "123.00000000000000000" }},

        { "%f", -765, { "-765.000000" }},
        { "%e", -765, { "-7.650000e+02", NULL, "-7.650000e+002" }},
        { "%g", -765, { "-765" }},
        { "%21f", -765, { "          -765.000000" }},
        { "%20e", -765, { "       -7.650000e+02", NULL, "      -7.650000e+002" }},
        { "%20g", -765, { "                -765" }},
        { "%.21f", -765, { "-765.000000000000000000000" }},
        { "%.20e", -765, { "-7.65000000000000000000e+02", NULL, "-7.65000000000000000000e+002" }},
        { "%.20g", -765, { "-765" }},
        { "%.021f", -765, { "-765.000000000000000000000" }},
        { "%.020e", -765, { "-7.65000000000000000000e+02", NULL, "-7.65000000000000000000e+002" }},
        { "%.020g", -765, { "-765" }},
        { "%#.21f", -765, { "-765.000000000000000000000" }},
        { "%#.20e", -765, { "-7.65000000000000000000e+02", NULL, "-7.65000000000000000000e+002" }},
        { "%#.20g", -765, { "-765.00000000000000000" }},
        { "%.30f", 1.0/3.0, { "0.333333333333333314829616256247" }},
        { "%.30lf", sqrt(2), { "1.414213562373095145474621858739" }},
        { "%f", 3.141592653590000, { "3.141593" }},
        { "%.10f", 3.141592653590000, { "3.1415926536" }},
        { "%.11f", 3.141592653590000, { "3.14159265359" }},
        { "%.15f", 3.141592653590000, { "3.141592653590000" }},
        { "%.15f", M_PI, { "3.141592653589793" }},
        { "%.13f", 37.866261574537077, { "37.8662615745371" }},
        { "%.14f", 37.866261574537077, { "37.86626157453708" }},
        { "%.15f", 37.866261574537077, { "37.866261574537077" }},
        { "%.0g", 9.8949714229143402e-05, { "0.0001" }},
        { "%.0f", 0.5, { "1", NULL, NULL, NULL, "0" }, {NULL, NULL, NULL, NULL, "1" }},
        { "%.0f", 1.5, { "2" }},
        { "%.0f", 2.5, { "3", NULL, NULL, NULL, "2" }, {NULL, NULL, NULL, NULL, "3" }},
        { "%g", 9.999999999999999e-5, { "0.0001" }},

        { "%g", 0.0005, { "0.0005" }},
        { "%g", 0.00005, { "5e-05", NULL, "5e-005" }},
        { "%g", 0.000005, { "5e-06", NULL, "5e-006" }},
        { "%g", 999999999999999.0, { "1e+15", NULL, "1e+015" }},
        { "%g", 1000000000000000.0, { "1e+15", NULL, "1e+015" }},
        { "%.15g", 0.0005, { "0.0005" }},
        { "%.15g", 0.00005, { "5e-05", NULL, "5e-005" }},
        { "%.15g", 0.000005, { "5e-06", NULL, "5e-006" }},
        { "%.15g", 999999999999999.0, { "999999999999999" }},
        { "%.15g", 1000000000000000.0, { "1e+15", NULL, "1e+015" }},
    };

    const char *res = NULL;
    const char *broken_res;
    char buf[100];
    int i, j, r;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        broken_res = NULL;

        for (j = 0; j < ARRAY_SIZE(flags); j++)
        {
            if (tests[i].res[j]) res = tests[i].res[j];
            if (tests[i].broken[j]) broken_res = tests[i].broken[j];

            r = vsprintf_wrapper(flags[j], buf, sizeof(buf), tests[i].fmt, tests[i].d);
            ok(r == strlen(res) || broken(broken_res && r == strlen(broken_res)),
                    "%d,%d) r = %d, expected %Id\n", i, j, r, strlen(res));
            ok(!strcmp(buf, res) || broken(broken_res && !strcmp(buf, broken_res)),
                    "%d,%d) buf = %s, expected %s\n", i, j, buf, res);
        }
    }
}

static void test_printf_width_specification(void)
{
    int r;
    char buffer[20];

    r = vsprintf_wrapper(0, buffer, sizeof(buffer), "%0*2d", 1, 3);
    ok(r == 2, "r = %d\n", r);
    ok(!strcmp(buffer, "03"), "buffer wrong, got=%s\n", buffer);

    r = vsprintf_wrapper(0, buffer, sizeof(buffer), "%*0d", 1, 2);
    ok(r == 1, "r = %d\n", r);
    ok(!strcmp(buffer, "2"), "buffer wrong, got=%s\n", buffer);

    r = vsprintf_wrapper(0, buffer, sizeof(buffer), "% *2d", 0, 7);
    ok(r == 2, "r = %d\n", r);
    ok(!strcmp(buffer, " 7"), "buffer wrong, got=%s\n", buffer);
}

START_TEST(printf)
{
    ok(_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    test_snprintf();
    test_swprintf();
    test_fprintf();
    test_fwprintf();
    test_vsnprintf_s();
    test_vsnwprintf_s();
    test_printf_legacy_wide();
    test_printf_legacy_msvcrt();
    test_printf_legacy_three_digit_exp();
    test_printf_c99();
    test_printf_natural_string();
    test_printf_fp();
    test_printf_width_specification();
}
