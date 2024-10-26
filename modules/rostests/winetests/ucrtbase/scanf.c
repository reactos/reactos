/*
 * Conformance tests for *scanf functions.
 *
 * Copyright 2002 Uwe Bonnes
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
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/test.h"

static int WINAPIV vsscanf_wrapper(unsigned __int64 options, const char *str, size_t len, const char *format, ...)
{
    int ret;
    va_list valist;
    va_start(valist, format);
    ret = __stdio_common_vsscanf(options, str, len, format, NULL, valist);
    va_end(valist);
    return ret;
}

static void test_sscanf(void)
{
    static const float float1 = -82.6267f, float2 = 27.76f;
    char buffer[100], buffer1[100];
    int result, ret, hour, min, count;
    LONGLONG result64;
    DWORD_PTR result_ptr;
    char c;
    void *ptr;
    float ret_float1, ret_float2;
    double double_res;
    unsigned int i;
    size_t ret_size;

    static const unsigned int tests[] =
    {
        0,
        _CRT_INTERNAL_SCANF_LEGACY_WIDE_SPECIFIERS,
        _CRT_INTERNAL_SCANF_LEGACY_MSVCRT_COMPATIBILITY,
    };

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        ret = vsscanf_wrapper(tests[i], "", -1, "%d", &result);
        ok(ret == EOF, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret = vsscanf_wrapper(tests[i], "000000000046F170", -1, "%p", &ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(ptr == (void *)0x46f170, "sscanf reads %p for flags %#x\n", ptr, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0046F171", -1, "%p", &ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(ptr == (void *)0x46f171, "sscanf reads %p for flags %#x\n", ptr, tests[i]);

        ret = vsscanf_wrapper(tests[i], "46F172", -1, "%p", &ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(ptr == (void *)0x46f172, "sscanf reads %p for flags %#x\n", ptr, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0x46F173", -1, "%p", &ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        todo_wine ok(ptr == (void *)0x46f173, "sscanf reads %p for flags %#x\n", ptr, tests[i]);

        ret = vsscanf_wrapper(tests[i], "-46F174", -1, "%p", &ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(ptr == (void *)(ULONG_PTR)-0x46f174, "sscanf reads %p for flags %#x\n", ptr, tests[i]);

        ret = vsscanf_wrapper(tests[i], "+46F175", -1, "%p", &ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(ptr == (void *)0x46f175, "sscanf reads %p for flags %#x\n", ptr, tests[i]);

        ret = vsscanf_wrapper(tests[i], "1233", -1, "%p", &ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(ptr == (void *)0x1233, "sscanf reads %p for flags %#x\n", ptr, tests[i]);

        ret = vsscanf_wrapper(tests[i], "1234", -1, "%P", &ptr);
        todo_wine ok(ret == 0, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0x519", -1, "%x", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 0x519, "sscanf reads %#x for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0x51a", -1, "%x", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 0x51a, "sscanf reads %#x for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0x51g", -1, "%x", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 0x51, "sscanf reads %#x for flags %#x\n", result, tests[i]);

        result = 0;
        ret = vsscanf_wrapper(tests[i], "-1", -1, "%x", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == -1, "sscanf reads %#x for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "\"%12@", -1, "%\"%%%d%@", &result);
        todo_wine ok(ret == 0, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        sprintf(buffer, "%f %f", float1, float2);
        ret = vsscanf_wrapper(tests[i], buffer, -1, "%f%f", &ret_float1, &ret_float2);
        ok(ret == 2, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(ret_float1 == float1, "got wrong float %.8e for flags %#x\n", ret_float1, tests[i]);
        ok(ret_float2 == float2, "got wrong float %.8e for flags %#x\n", ret_float2, tests[i]);

        sprintf(buffer, "%lf", 32.715);
        ret = vsscanf_wrapper(tests[i], buffer, -1, "%lf", &double_res);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(double_res == 32.715, "got wrong double %.16e for flags %#x\n", double_res, tests[i]);
        ret = vsscanf_wrapper(tests[i], buffer, -1, "%Lf", &double_res);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(double_res == 32.715, "got wrong double %.16e for flags %#x\n", double_res, tests[i]);

        ret = vsscanf_wrapper(tests[i], "1.1e-30", -1, "%lf", &double_res);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(double_res == 1.1e-30, "got wrong double %.16e for flags %#x\n", double_res, tests[i]);

        ret = vsscanf_wrapper(tests[i], "  Waverly", -1, "%*c%[^\n]", buffer);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(!strcmp(buffer, " Waverly"), "got string '%s' for flags %#x\n", buffer, tests[i]);

        ret = vsscanf_wrapper(tests[i], "abcefgdh", -1, "%*[a-cg-e]%c", &buffer[0]);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(buffer[0] == 'd', "got char '%c' for flags %#x\n", buffer[0], tests[i]);

        ret = vsscanf_wrapper(tests[i], "abcefgdh", -1, "%*[a-cd-dg-e]%c", &buffer[0]);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(buffer[0] == 'h', "got char '%c' for flags %#x\n", buffer[0], tests[i]);

        strcpy(buffer, "foo");
        strcpy(buffer1, "bar");
        ret = vsscanf_wrapper(tests[i], "a", -1, "%s%s", buffer, buffer1);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(!strcmp(buffer, "a"), "got string '%s' for flags %#x\n", buffer, tests[i]);
        ok(!strcmp(buffer1, "bar"), "got string '%s' for flags %#x\n", buffer1, tests[i]);

        ret = vsscanf_wrapper(tests[i], "21:59:20", -1, "%d%n", &result, &count);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 21, "got wrong number %d for flags %#x\n", result, tests[i]);
        ok(count == 2, "got wrong count %d for flags %#x\n", count, tests[i]);

        ret = vsscanf_wrapper(tests[i], ":59:20", -1, "%*c%n", &count);
        ok(ret == 0, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(count == 1, "got wrong count %d for flags %#x\n", count, tests[i]);

        result = 0xdeadbeef;
        ret = vsscanf_wrapper(tests[i], "12345678", -1, "%hd", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 0xdead614e, "got wrong number %#x for flags %#x\n", result, tests[i]);

        result = 0xdeadbeef;
        ret = vsscanf_wrapper(tests[i], "12345678", -1, "%hhd", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 0xdeadbe4e, "got wrong number %#x for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "12345678901234", -1, "%lld", &result64);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result64 == 12345678901234, "got wrong number 0x%s for flags %#x\n",
                wine_dbgstr_longlong(result64), tests[i]);

        ret = vsscanf_wrapper(tests[i], "123", -1, "%i", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 123, "got wrong number %d for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "-1", -1, "%i", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == -1, "got wrong number %d for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "123", -1, "%d", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 123, "got wrong number %d for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "-1", -1, "%d", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == -1, "got wrong number %d for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "017", -1, "%i", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 15, "got wrong number %d for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0x17", -1, "%i", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == 23, "got wrong number %d for flags %#x\n", result, tests[i]);

        ret = vsscanf_wrapper(tests[i], "-1", -1, "%o", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == -1, "got wrong number %d for flags %#x\n", result, tests[i]);

        ret = 0xdeadbeef;
        ret = vsscanf_wrapper(tests[i], "-1", -1, "%u", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == -1, "got wrong number %d for flags %#x\n", result, tests[i]);

        c = 0x55;
        ret = vsscanf_wrapper(tests[i], "a", -1, "%c", &c);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(c == 'a', "got wrong char '%c' for flags %#x\n", c, tests[i]);

        c = 0x55;
        ret = vsscanf_wrapper(tests[i], " a", -1, "%c", &c);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(c == ' ', "got wrong char '%c' for flags %#x\n", c, tests[i]);

        c = 0x55;
        ret = vsscanf_wrapper(tests[i], "18:59", -1, "%d:%d%c", &hour, &min, &c);
        ok(ret == 2, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(hour == 18, "got wrong char '%c' for flags %#x\n", hour, tests[i]);
        ok(min == 59, "got wrong char '%c' for flags %#x\n", min, tests[i]);
        ok(c == 0x55, "got wrong char '%c' for flags %#x\n", c, tests[i]);

        strcpy(buffer, "foo");
        strcpy(buffer1, "bar");
        ret = vsscanf_wrapper(tests[i], "abc   def", -1, "%s %n%s", buffer, &count, buffer1);
        ok(ret == 2, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(!strcmp(buffer, "abc"), "got wrong string '%s' for flags %#x\n", buffer, tests[i]);
        ok(count == 6, "got wrong count %d for flags %#x\n", count, tests[i]);
        ok(!strcmp(buffer1, "def"), "got wrong string '%s' for flags %#x\n", buffer1, tests[i]);

        ret = vsscanf_wrapper(tests[i], "3:45", -1, "%d:%d%n", &hour, &min, &count);
        ok(ret == 2, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(hour == 3, "got wrong char '%c' for flags %#x\n", hour, tests[i]);
        ok(min == 45, "got wrong char '%c' for flags %#x\n", min, tests[i]);
        ok(count == 4, "got wrong count %d for flags %#x\n", count, tests[i]);

        strcpy(buffer, "foo");
        strcpy(buffer1, "bar");
        ret = vsscanf_wrapper(tests[i], "test=value\xda", -1, "%[^=] = %[^;]", buffer, buffer1);
        ok(ret == 2, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(!strcmp(buffer, "test"), "got wrong string '%s' for flags %#x\n", buffer, tests[i]);
        ok(!strcmp(buffer1, "value\xda"), "got wrong string '%s' for flags %#x\n", buffer1, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0.1", 3, "%lf%n", &double_res, &count);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(double_res == 0.1, "got wrong double %.16e for flags %#x\n", double_res, tests[i]);
        ok(count == 3, "got wrong count %d for flags %#x\n", count, tests[i]);

        ret = vsscanf_wrapper(tests[i], "a", -1, "%lf%n", &double_res, &count);
        ok(ret == 0, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret = vsscanf_wrapper(tests[i], "aa", -1, "%c%lf%n", &c, &double_res, &count);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret = vsscanf_wrapper(tests[i], "a0e", -1, "%c%lf%n", &c, &double_res, &count);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0.", -1, "%lf%n", &double_res, &count);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(double_res == 0, "got wrong double %.16e for flags %#x\n", double_res, tests[i]);
        ok(count == 2, "got wrong count %d for flags %#x\n", count, tests[i]);

        ret = vsscanf_wrapper(tests[i], "0.", 2, "%lf%n", &double_res, &count);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(double_res == 0, "got wrong double %.16e for flags %#x\n", double_res, tests[i]);
        ok(count == 2, "got wrong count %d for flags %#x\n", count, tests[i]);

        ret = vsscanf_wrapper(tests[i], "1e", -1, "%lf%n", &double_res, &count);
        ok(ret == -1, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret = vsscanf_wrapper(tests[i], "1e ", 2, "%lf%n", &double_res, &count);
        ok(ret == -1, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret = vsscanf_wrapper(tests[i], "1e+", -1, "%lf%n", &double_res, &count);
        ok(ret == -1, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret = vsscanf_wrapper(tests[i], "inf", -1, "%lf%n", &double_res, &count);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(double_res == INFINITY, "got wrong double %.16e for flags %#x\n", double_res, tests[i]);
        ok(count == 3, "got wrong count %d for flags %#x\n", count, tests[i]);

        ret = vsscanf_wrapper(tests[i], "infa", -1, "%lf%n", &double_res, &count);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(double_res == INFINITY, "got wrong double %.16e for flags %#x\n", double_res, tests[i]);
        ok(count == 3, "got wrong count %d for flags %#x\n", count, tests[i]);

        ret = vsscanf_wrapper(tests[i], "infi", -1, "%lf%n", &double_res, &count);
        ok(ret == -1, "sscanf returned %d for flags %#x\n", ret, tests[i]);

        ret_size = ~0;
        ret = vsscanf_wrapper(tests[i], "1", -1, "%zd", &ret_size);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(ret_size == 1, "got wrong size_t %s for flags %#x\n",
                wine_dbgstr_longlong((LONGLONG)ret_size), tests[i]);

        result64 = 0;
        ret = vsscanf_wrapper(tests[i], "12345678901234", -1, "%I64d", &result64);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result64 == 12345678901234ll, "got wrong number 0x%s for flags %#x\n",
                wine_dbgstr_longlong(result64), tests[i]);

        result = 0;
        ret = vsscanf_wrapper(tests[i], "12345678901234", -1, "%I32d", &result);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result == (int)12345678901234ll, /* this is always truncated to 32bit */
           "got wrong number 0x%d for flags %#x\n", result, tests[i]);

        result_ptr = 0;
        ret = vsscanf_wrapper(tests[i], "0x87654321", -1, "%Ix", &result_ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result_ptr == 0x87654321,
           "got wrong number %Ix for flags %#x\n", result_ptr, tests[i]);

        result_ptr = 0;
        ret = vsscanf_wrapper(tests[i], "0x123456789", -1, "%Ix", &result_ptr);
        ok(ret == 1, "sscanf returned %d for flags %#x\n", ret, tests[i]);
        ok(result_ptr == (DWORD_PTR)0x123456789ull, /* this is truncated on 32bit systems */
           "got wrong number %Ix for flags %#x\n", result_ptr, tests[i]);
    }
}

START_TEST(scanf)
{
    test_sscanf();
}
