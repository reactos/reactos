/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for sprintf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <apitest.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <stdarg.h>
#include <windows.h>


#define ok_str(x, y) \
    ok(strcmp(x, y) == 0, "got '%s', expected '%s'\n", x, y);

#define ok_int(x, y) \
    ok(x == y, "got %d, expected %d\n", x, y);

int (*p_vsprintf)(char *buf, const char *fmt, va_list argptr);


int
my_sprintf(char *buf, const char *format, ...)
{
    const TCHAR * fmt = format;
    va_list argptr;
    int ret;

    va_start(argptr, format);
    ret = p_vsprintf(buf, fmt, argptr);
    va_end(argptr);

    return ret;
}

int
my_swprintf(wchar_t *buf, const wchar_t *format, ...)
{
    const wchar_t * fmt = format;
    va_list argptr;
    int ret;

    va_start(argptr, format);
    ret = _vsnwprintf(buf, MAXLONG, fmt, argptr);
    va_end(argptr);

    return ret;
}

#define sprintf(buf, format, ...) my_sprintf(buf, format, __VA_ARGS__);
#define swprintf(buf, format, ...) my_swprintf((wchar_t*)buf, format, __VA_ARGS__);

void
test_c()
{
    char buffer[64];

    sprintf(buffer, "%c", 0x3031);
    ok_str(buffer, "1");

    sprintf(buffer, "%hc", 0x3031);
    ok_str(buffer, "1");

    sprintf(buffer, "%wc", 0x3031);
    ok_str(buffer, "?");

    sprintf(buffer, "%lc", 0x3031);
    ok_str(buffer, "?");

    sprintf(buffer, "%Lc", 0x3031);
    ok_str(buffer, "1");

    sprintf(buffer, "%Ic", 0x3031);
    ok_str(buffer, "Ic");

    sprintf(buffer, "%Iwc", 0x3031);
    ok_str(buffer, "Iwc");

    sprintf(buffer, "%I32c", 0x3031);
    ok_str(buffer, "1");

    sprintf(buffer, "%I64c", 0x3031);
    ok_str(buffer, "1");

    sprintf(buffer, "%4c", 0x3031);
    ok_str(buffer, "   1");

    sprintf(buffer, "%04c", 0x3031);
    ok_str(buffer, "0001");

    sprintf(buffer, "%+4c", 0x3031);
    ok_str(buffer, "   1");

}

void
test_d()
{
    char buffer[5000];
    int result;

    sprintf(buffer, "%d", 1234567);
    ok_str(buffer, "1234567");

    sprintf(buffer, "%d", -1234567);
    ok_str(buffer, "-1234567");

    sprintf(buffer, "%hd", 1234567);
    ok_str(buffer, "-10617");

    sprintf(buffer, "%08d", 1234);
    ok_str(buffer, "00001234");

    sprintf(buffer, "%-08d", 1234);
    ok_str(buffer, "1234    ");

    sprintf(buffer, "%+08d", 1234);
    ok_str(buffer, "+0001234");

    sprintf(buffer, "%+3d", 1234);
    ok_str(buffer, "+1234");

    sprintf(buffer, "%3.3d", 1234);
    ok_str(buffer, "1234");

    sprintf(buffer, "%3.6d", 1234);
    ok_str(buffer, "001234");

    sprintf(buffer, "%8d", -1234);
    ok_str(buffer, "   -1234");

    sprintf(buffer, "%08d", -1234);
    ok_str(buffer, "-0001234");

    sprintf(buffer, "%ld", -1234);
    ok_str(buffer, "-1234");

    sprintf(buffer, "%wd", -1234);
    ok_str(buffer, "-1234");

    sprintf(buffer, "%ld", -5149074030855LL);
    ok_str(buffer, "591757049");

    sprintf(buffer, "%lld", -5149074030855LL);
    ok_str(buffer, "591757049");

    sprintf(buffer, "%I64d", -5149074030855LL);
    ok_str(buffer, "-5149074030855");

    sprintf(buffer, "%Ld", -5149074030855LL);
    ok_str(buffer, "591757049");

    sprintf(buffer, "%lhwI64d", -5149074030855LL);
    ok_str(buffer, "-5149074030855");

    sprintf(buffer, "%I64hlwd", -5149074030855LL);
    ok_str(buffer, "-5149074030855");

    sprintf(buffer, "%hlwd", -5149074030855LL);
    ok_str(buffer, "32505");

    sprintf(buffer, "%Ild", -5149074030855LL);
    ok_str(buffer, "Ild");

    sprintf(buffer, "%hd", -5149074030855LL);
    ok_str(buffer, "32505");

    sprintf(buffer, "%hhd", -5149074030855LL);
    ok_str(buffer, "32505");

    sprintf(buffer, "%hI32hd", -5149074030855LL);
    ok_str(buffer, "32505");

    sprintf(buffer, "%wd", -5149074030855LL);
    ok_str(buffer, "591757049");

    result = sprintf(buffer, " %d.%d", 3, 0);
    ok_int(result, 4);

}

void
test_u()
{
    char buffer[64];

    sprintf(buffer, "%u", 1234);
    ok_str(buffer, "1234");

    sprintf(buffer, "%u", -1234);
    ok_str(buffer, "4294966062");

    sprintf(buffer, "%lu", -1234);
    ok_str(buffer, "4294966062");

    sprintf(buffer, "%llu", -1234);
    ok_str(buffer, "4294966062");

    sprintf(buffer, "%+u", 1234);
    ok_str(buffer, "1234");

    sprintf(buffer, "% u", 1234);
    ok_str(buffer, "1234");

}

void
test_x()
{
    char buffer[64];

    sprintf(buffer, "%x", 0x1234abcd);
    ok_str(buffer, "1234abcd");

    sprintf(buffer, "%X", 0x1234abcd);
    ok_str(buffer, "1234ABCD");

    sprintf(buffer, "%#x", 0x1234abcd);
    ok_str(buffer, "0x1234abcd");

    sprintf(buffer, "%#X", 0x1234abcd);
    ok_str(buffer, "0X1234ABCD");

    /* "0x" is contained in the field length */
    sprintf(buffer, "%#012X", 0x1234abcd);
    ok_str(buffer, "0X001234ABCD");

    sprintf(buffer, "%llx", 0x1234abcd5678ULL);
    ok_str(buffer, "abcd5678");

    sprintf(buffer, "%I64x", 0x1234abcd5678ULL);
    ok_str(buffer, "1234abcd5678");

}

void
test_p()
{
    char buffer[64];

    sprintf(buffer, "%p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, "00123ABC");

    sprintf(buffer, "%#p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, "0X00123ABC");

    sprintf(buffer, "%#012p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, "  0X00123ABC");

    sprintf(buffer, "%9p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, " 00123ABC");

    sprintf(buffer, "%09p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, " 00123ABC");

    sprintf(buffer, "% 9p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, " 00123ABC");

    sprintf(buffer, "%-9p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, "00123ABC ");

    sprintf(buffer, "%4p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, "00123ABC");

    sprintf(buffer, "%9.4p", (void*)(ptrdiff_t)0x123abc);
    ok_str(buffer, " 00123ABC");

    sprintf(buffer, "%I64p", 0x123abc456789ULL);
    ok_str(buffer, "123ABC456789");

    sprintf(buffer, "%hp", 0x123abc);
    ok_str(buffer, "00003ABC");

}

void
test_o()
{
    TCHAR buffer[64];

    sprintf(buffer, "%o", 1234);
    ok_str(buffer, "2322");

    sprintf(buffer, "%o", -1234);
    ok_str(buffer, "37777775456");

}

void
test_s()
{
    char buffer[64];

    sprintf(buffer, "%s", "test");
    ok_str(buffer, "test");

    sprintf(buffer, "%S", L"test");
    ok_str(buffer, "test");

    sprintf(buffer, "%ls", L"test");
    ok_str(buffer, "test");

    sprintf(buffer, "%ws", L"test");
    ok_str(buffer, "test");

    sprintf(buffer, "%hs", "test");
    ok_str(buffer, "test");

    sprintf(buffer, "%hS", "test");
    ok_str(buffer, "test");

    sprintf(buffer, "%7s", "test");
    ok_str(buffer, "   test");

    sprintf(buffer, "%07s", "test");
    ok_str(buffer, "000test");

    sprintf(buffer, "%.3s", "test");
    ok_str(buffer, "tes");

    sprintf(buffer, "%+7.3s", "test");
    ok_str(buffer, "    tes");

    sprintf(buffer, "%+4.0s", "test");
    ok_str(buffer, "    ");


}

void
test_n()
{
    int len = 123;
    __int64 len64;
    char buffer[64];

    sprintf(buffer, "%07s%n", "test", &len);
    ok_int(len, 7);

    len = 0x12345678;
    sprintf(buffer, "%s%hn", "test", &len);
    ok_int(len, 0x12340004);

    len = 0x12345678;
    sprintf(buffer, "%s%hhn", "test", &len);
    ok_int(len, 0x12340004);

    len64 = 0x0123456781234567ULL;
    sprintf(buffer, "%s%lln", "test", &len64);
    ok(len64 == 0x123456700000004ULL, " ");

}

void
test_f()
{
    char buffer[128];
    long double fpval;

    fpval = 1. / 3.;
    sprintf(buffer, "%f", fpval);
    ok_str(buffer, "-0.000000");

    sprintf(buffer, "%lf", fpval);
    ok_str(buffer, "-0.000000");

    sprintf(buffer, "%llf", fpval);
    ok_str(buffer, "-0.000000");

    sprintf(buffer, "%Lf", fpval);
    ok_str(buffer, "-0.000000");

    sprintf(buffer, "%f", (double)fpval);
    ok_str(buffer, "0.333333");

    sprintf(buffer, "%f", (double)0.125);
    ok_str(buffer, "0.125000");

    sprintf(buffer, "%3.7f", (double)fpval);
    ok_str(buffer, "0.3333333");

    sprintf(buffer, "%3.30f", (double)fpval);
    ok_str(buffer, "0.333333333333333310000000000000");

    sprintf(buffer, "%3.60f", (double)fpval);
    ok_str(buffer, "0.333333333333333310000000000000000000000000000000000000000000");

    sprintf(buffer, "%3.80f", (double)fpval);
    ok_str(buffer, "0.33333333333333331000000000000000000000000000000000000000000000000000000000000000");

    fpval = 1. / 0.;
    sprintf(buffer, "%f", fpval);
    ok_str(buffer, "0.000000");

    sprintf(buffer, "%f", 0x7ff8000000000000ULL); // NAN
    ok_str(buffer, "1.#QNAN0");

    sprintf(buffer, "%.9f", 0x7ff8000000000000ULL);
    ok_str(buffer, "1.#QNAN0000");

    sprintf(buffer, "%f", 0x7ff0000000000000ULL ); // INFINITY
    ok_str(buffer, "1.#INF00");

    sprintf(buffer, "%f", 0xfff0000000000000ULL ); // -INFINITY
    ok_str(buffer, "-1.#INF00");

    sprintf(buffer, "%f", 0xfff8000000000000ULL); // broken
    ok_str(buffer, "-1.#IND00");

}

void
test_e()
{
    char buffer[128];
    long double fpval;

    fpval = 1. / 3.;
    sprintf(buffer, "%e", fpval);
    ok_str(buffer, "-3.720662e-103");

    fpval = 1. / 3.;
    sprintf(buffer, "%e", (double)fpval);
    ok_str(buffer, "3.333333e-001");

    sprintf(buffer, "%e", 33.54223);
    ok_str(buffer, "3.354223e+001");

    sprintf(buffer, "%e", NAN);
    ok_str(buffer, "1.#QNAN0e+000");

    sprintf(buffer, "%.9e", NAN);
    ok_str(buffer, "1.#QNAN0000e+000");

    sprintf(buffer, "%e", INFINITY );
    ok_str(buffer, "1.#INF00e+000");

    sprintf(buffer, "%e", -INFINITY );
    ok_str(buffer, "-1.#INF00e+000");


}

typedef struct _STRING
{
  unsigned short Length;
  unsigned short MaximumLength;
  void *Buffer;
} STRING;

void
test_Z()
{
    char buffer[128];
    STRING string;
    int result;

    string.Buffer = "Test\0Hallo";
    string.Length = 4;
    string.MaximumLength = 5;

    sprintf(buffer, "%Z", &string);
    ok_str(buffer, "Test");

    string.Length = 8;
    sprintf(buffer, "%Z", &string);
    ok_str(buffer, "Test");

    string.Length = 3;
    sprintf(buffer, "%Z", &string);
    ok_str(buffer, "Tes");

    string.Buffer = 0;
    sprintf(buffer, "%Z", &string);
    ok_str(buffer, "(null)");

    sprintf(buffer, "%Z", 0);
    ok_str(buffer, "(null)");

    string.Buffer = (char*)L"Test\0Hallo";
    string.Length = 8;
    string.MaximumLength = 10;


    string.Buffer = (char*)L"test";
    string.Length = 12;
    string.MaximumLength = 15;
    result = _snprintf(buffer, 127, "%Z %u%c", &string, 1, 0);
    ok_int(result, 15);

}

void
test_other()
{
    char buffer[128];

    sprintf(buffer, "%lI64wQ", "test");
    ok_str(buffer, "Q");

}

START_TEST(sprintf)
{
    HANDLE hModule;

    hModule = GetModuleHandleA("msvcrt.dll");
    if (!hModule) return;
    p_vsprintf = (PVOID)GetProcAddress(hModule, "vsprintf");
    if (!p_vsprintf) return;

    test_c();
    test_d();
    test_u();
    test_x();
    test_p();
    test_o();

    test_s();

    test_f();
    test_e();
    test_Z();

    test_n();
    test_other();
}

