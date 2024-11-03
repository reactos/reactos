/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Testing
 * COPYRIGHT:   Copyright 2019-2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifndef ATLTEST_H_
#define ATLTEST_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

int g_atltest_executed = 0;
int g_atltest_failed = 0;
int g_atltest_skipped = 0;

const char *g_atltest_file = NULL;
int g_atltest_line = 0;

void atltest_set_location(const char *file, int line)
{
    g_atltest_file = file;
    g_atltest_line = line;
}

void atltest_ok(int value, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    if (!value)
    {
        printf("%s (%d): ", g_atltest_file, g_atltest_line);
        vprintf(fmt, va);
        g_atltest_failed++;
    }
    g_atltest_executed++;
    va_end(va);
}

void atltest_skip(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    printf("%s (%d): test skipped: ", g_atltest_file, g_atltest_line);
    vprintf(fmt, va);
    g_atltest_skipped++;
    va_end(va);
}

#undef ok
#define ok(value, ...) do { \
    atltest_set_location(__FILE__, __LINE__); \
    atltest_ok(value, __VA_ARGS__); \
} while (0)
#define ok_(x1,x2) atltest_set_location(x1,x2); atltest_ok

#undef skip
#define skip(...) do { \
    atltest_set_location(__FILE__, __LINE__); \
    atltest_skip(__VA_ARGS__); \
} while (0)

#undef trace
#define trace printf

static void atltest_start_test(void);
extern const char *g_atltest_name;

#define START_TEST(x) \
    const char *g_atltest_name = #x; \
    static void atltest_start_test(void)

int main(void)
{
    atltest_start_test();
    printf("%s: %d tests executed (0 marked as todo, %d failures), %d skipped.\n",
           g_atltest_name, g_atltest_executed, g_atltest_failed, g_atltest_skipped);
    return g_atltest_failed;
}

char *wine_dbgstr_w(const wchar_t *wstr)
{
    static char buf[512];
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, buf, _countof(buf), NULL, NULL);
    return buf;
}

#define ok_hex(expression, result) \
    do { \
        int _value = (expression); \
        ok(_value == (result), "Wrong value for '%s', expected: " #result " (0x%x), got: 0x%x\n", \
           #expression, (int)(result), _value); \
    } while (0)

#define ok_dec(expression, result) \
    do { \
        int _value = (expression); \
        ok(_value == (result), "Wrong value for '%s', expected: " #result " (%d), got: %d\n", \
           #expression, (int)(result), _value); \
    } while (0)

#define ok_ptr(expression, result) \
    do { \
        const void *_value = (expression); \
        ok(_value == (result), "Wrong value for '%s', expected: " #result " (%p), got: %p\n", \
           #expression, (void*)(result), _value); \
    } while (0)

#define ok_size_t(expression, result) \
    do { \
        size_t _value = (expression); \
        ok(_value == (result), "Wrong value for '%s', expected: " #result " (%Ix), got: %Ix\n", \
           #expression, (size_t)(result), _value); \
    } while (0)

#define ok_char(expression, result) ok_hex(expression, result)

#define ok_err(error) \
    ok(GetLastError() == (error), "Wrong last error. Expected " #error ", got 0x%lx\n", GetLastError())

#define ok_str(x, y) \
    ok(strcmp(x, y) == 0, "Wrong string. Expected '%s', got '%s'\n", y, x)

#define ok_wstr(x, y) \
    ok(wcscmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

#define ok_long(expression, result) ok_hex(expression, result)
#define ok_int(expression, result) ok_dec(expression, result)
#define ok_ntstatus(status, expected) ok_hex(status, expected)
#define ok_hdl ok_ptr

static inline const char *wine_dbgstr_point(const POINT *ppt)
{
    static char s_asz[4][40]; /* Ring buffer */
    static int s_i = 0;
    char *buf;

    if (!ppt)
        return "(null)";
    if (IS_INTRESOURCE(ppt))
        return "(invalid ptr)";

    buf = s_asz[s_i];
    s_i = (s_i + 1) % _countof(s_asz);
    sprintf_s(buf, _countof(s_asz[0]), "(%ld, %ld)", ppt->x, ppt->y);
    return buf;
}

static inline const char *wine_dbgstr_size(const SIZE *psize)
{
    return wine_dbgstr_point((const POINT *)psize);
}

static inline const char *wine_dbgstr_rect(const RECT *prc)
{
    static char s_asz[4][80]; /* Ring buffer */
    static int s_i = 0;
    char *buf;

    if (!prc)
        return "(null)";
    if (IS_INTRESOURCE(prc))
        return "(invalid ptr)";

    buf = s_asz[s_i];
    s_i = (s_i + 1) % _countof(s_asz);
    sprintf_s(buf, _countof(s_asz[0]), "(%ld, %ld) - (%ld, %ld)",
              prc->left, prc->top, prc->right, prc->bottom);
    return buf;
}

#define broken(x) x

#endif  /* ndef ATLTEST_H_ */
