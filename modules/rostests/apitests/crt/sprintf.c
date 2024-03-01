/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for sprintf
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <apitest_guard.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <tchar.h>
#include <float.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

#ifdef TEST_STATIC_CRT
#undef todo_ros
#define todo_ros todo_if(1)
#endif

#ifdef _MSC_VER
#pragma warning(disable:4778) // unterminated format string '%'
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#pragma GCC diagnostic ignored "-Wnonnull"
#if __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wformat-overflow"
#endif
#endif

#undef assert
#if DBG
#define assert(x) if (!(x)) __int2c()
#else
#define assert(x)
#endif

#define ok_sprintf_1_(line, format, arg, expected) \
{ \
    char buffer[500]; \
    int len = sprintf(buffer, format, arg); \
    assert(len < _countof(buffer)); \
    int expected_len = (int)strlen(expected); \
    ok_(__FILE__, line)(len == expected_len && !strcmp(buffer, expected), \
    "format '%s', expected '%s' (%d), got '%s' (%d)\n", \
    format, expected, expected_len, buffer, len); \
}

#define ok_sprintf_1(format, arg, expected) \
    ok_sprintf_1_(__LINE__, format, arg, expected)

static void test_basic(void)
{
    int Length;
    CHAR Buffer[128];

    /* basic parameter tests */
    StartSeh()
        Length = sprintf(NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        Length = sprintf(NULL, "");
        ok_int(Length, 0);
#if TEST_CRTDLL || TEST_USER32
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    StartSeh()
        Length = sprintf(NULL, "Hello");
        ok_int(Length, 5);
#if TEST_CRTDLL || TEST_USER32
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    /* some basic formats */
    Length = sprintf(Buffer, "abcde");
    ok_str(Buffer, "abcde");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%%");
    ok_str(Buffer, "%");
    ok_int(Length, 1);

    Length = sprintf(Buffer, "%");
    ok_str(Buffer, "");
    ok_int(Length, 0);

    Length = sprintf(Buffer, "%%%");
    ok_str(Buffer, "%");
    ok_int(Length, 1);
}

static void test_int_d(void)
{
    ok_sprintf_1("%d", 8, "8");

}

static void test_string(void)
{
    int Length;
    CHAR Buffer[128];
    PCHAR String;

    Length = sprintf(Buffer, "%s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    /* field width for %s */
    Length = sprintf(Buffer, "%8s", "hello");
    ok_str(Buffer, "   hello");
    ok_int(Length, 8);

    Length = sprintf(Buffer, "%4s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%-8s", "hello");
    ok_str(Buffer, "hello   ");
    ok_int(Length, 8);

    Length = sprintf(Buffer, "%-5s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%0s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%-0s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%*s", -8, "hello");
#ifdef TEST_USER32
    ok_str(Buffer, "*s");
    ok_int(Length, 2);
#else
    ok_str(Buffer, "hello   ");
    ok_int(Length, 8);
#endif

    /* precision for %s */
    Length = sprintf(Buffer, "%.s", "hello");
    ok_str(Buffer, "");
    ok_int(Length, 0);

    Length = sprintf(Buffer, "%.0s", "hello");
    ok_str(Buffer, "");
    ok_int(Length, 0);

    Length = sprintf(Buffer, "%.10s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%.5s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%.4s", "hello");
    ok_str(Buffer, "hell");
    ok_int(Length, 4);

    StartSeh()
        Length = sprintf(Buffer, "%.*s", -1, "hello");
#ifdef TEST_USER32
        ok_str(Buffer, "*s");
        ok_int(Length, 2);
#else
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
#endif
    EndSeh(STATUS_SUCCESS);

    String = AllocateGuarded(6);
    if (!String)
    {
        skip("Guarded allocation failure\n");
        return;
    }

    strcpy(String, "hello");
    StartSeh()
        Length = sprintf(Buffer, "%.8s", String);
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%.6s", String);
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%.5s", String);
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%.4s", String);
        ok_str(Buffer, "hell");
        ok_int(Length, 4);
    EndSeh(STATUS_SUCCESS);

    String[5] = '!';
    StartSeh()
        Length = sprintf(Buffer, "%.5s", String);
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
#ifdef TEST_USER32
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    StartSeh()
        Length = sprintf(Buffer, "%.6s", String);
        ok_str(Buffer, "hello!");
        ok_int(Length, 6);
#ifdef TEST_USER32
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    StartSeh()
        Length = sprintf(Buffer, "%.*s", 5, String);
#ifdef TEST_USER32
        ok_str(Buffer, "*s");
        ok_int(Length, 2);
#else
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
#endif
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%.*s", 6, String);
#ifdef TEST_USER32
        ok_str(Buffer, "*s");
        ok_int(Length, 2);
#else
        ok_str(Buffer, "hello!");
        ok_int(Length, 6);
#endif
    EndSeh(STATUS_SUCCESS);

    /* both field width and precision */
    StartSeh()
        Length = sprintf(Buffer, "%8.5s", String);
        ok_str(Buffer, "   hello");
        ok_int(Length, 8);
#ifdef TEST_USER32
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    StartSeh()
        Length = sprintf(Buffer, "%-*.6s", -8, String);
#ifdef TEST_USER32
        ok_str(Buffer, "*.6s");
        ok_int(Length, 4);
#else
        ok_str(Buffer, "hello!  ");
        ok_int(Length, 8);
#endif
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%*.*s", -8, 6, String);
#ifdef TEST_USER32
        ok_str(Buffer, "*.*s");
        ok_int(Length, 4);
#else
        ok_str(Buffer, "hello!  ");
        ok_int(Length, 8);
#endif
    EndSeh(STATUS_SUCCESS);

    FreeGuarded(String);
}


//
// Floating point values.
// The format pecifier: "%[flags][width][.precision][length]specifier"
// flags: '-', '+', ' ', '#', '0'
// specifier: 'e', 'E', 'f', 'F', 'g', 'G', 'a', 'A'
//
// The results is printed as:
// [left_ws_padding][sign][left_0_padding][integer_significant][integer_0_pad][.][fraction_significant][fraction_0_pad]
//

static const UINT64 g_Inf = 0x7FF0000000000000ULL;
static const UINT64 g_NegInf = 0xFFF0000000000000ULL;
static const UINT64 g_NaN0 = 0x7FF8000000000000ULL;
static const UINT64 g_Ind = 0xFFF8000000000000ULL;
static const UINT64 g_NaN1 = 0x7FF0000000000001ULL;
static const UINT64 g_NaN2 = 0x7FF8000000000001ULL;
static const UINT64 g_NaN3 = 0x7FFFFFFFFFFFFFFFULL;
static const UINT64 g_NaN4 = 0x7FF80000000000F1ULL;
static const UINT64 g_NegNaN1 = 0xFFF0000000000001ULL;

void test_float_f(void)
{
    ok_sprintf_1("%f", 0.0, "0.000000");
    ok_sprintf_1("%f", 1.0, "1.000000");
    ok_sprintf_1("%f", -1.0, "-1.000000");
    ok_sprintf_1("%f", 1.23456789, "1.234568");
    ok_sprintf_1("%f", 0.00123456789, "0.001235");
    todo_ros ok_sprintf_1("%f", FLT_MAX, "340282346638528860000000000000000000000.000000");
    todo_ros ok_sprintf_1("%f", DBL_MAX, "179769313486231570000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.000000");
    ok_sprintf_1("% f", 1.0, " 1.000000");
    ok_sprintf_1("% f", -1.0, "-1.000000");
    ok_sprintf_1("%4f", 1.0, "1.000000");
    ok_sprintf_1("%8f", 1.0, "1.000000");
    ok_sprintf_1("%9f", 1.0, " 1.000000");
    ok_sprintf_1("% 9f", 1.0, " 1.000000");
    ok_sprintf_1("%9f", -1.0, "-1.000000");
    ok_sprintf_1("%10f", -1.0, " -1.000000");
    ok_sprintf_1("% 10f", -1.0, " -1.000000");
    ok_sprintf_1("%0f", 1.0, "1.000000");
    ok_sprintf_1("%010f", -1.0, "-01.000000");
    ok_sprintf_1("%.0f", 0.6, "1");
    ok_sprintf_1("%.0f", 1.23456789, "1");
    ok_sprintf_1("%.3f", 1.23456789, "1.235");
    ok_sprintf_1("%.11f", 1.23456789, "1.23456789000");
    ok_sprintf_1("%.1f", 9.999, "10.0");
    ok_sprintf_1("%.2f", 0.099, "0.10");
    ok_sprintf_1("%f", -123.45678, "-123.456780");
    ok_sprintf_1("%f", -9.2559631349317830737e+061, "-92559631349317831000000000000000000000000000000000000000000000.000000");

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4477) // format string
#endif
    // MSVC uses the FPU, which can lead to wrong output, so we put the values
    // directly on the stack to get the correct results.
    ok_sprintf_1("%f", g_Inf, "1.#INF00");
    ok_sprintf_1("%f", g_NegInf, "-1.#INF00");
    ok_sprintf_1("%f", g_Ind, "-1.#IND00");
    ok_sprintf_1("%f", g_NaN0, "1.#QNAN0");
    ok_sprintf_1("%f", g_NaN1, "1.#SNAN0");
    ok_sprintf_1("%f", g_NegNaN1, "-1.#SNAN0");
    ok_sprintf_1("%f", g_NaN2, "1.#QNAN0");
    ok_sprintf_1("%f", g_NaN3, "1.#QNAN0");
    ok_sprintf_1("%f", g_NaN4, "1.#QNAN0");
    ok_sprintf_1("%10f", g_Inf, "  1.#INF00");
    ok_sprintf_1("%.10f", g_Inf, "1.#INF000000");
    ok_sprintf_1("%.0f", g_Inf, "1");
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    // %lf (same as %f)
    ok_sprintf_1("%lf", 0.0, "0.000000");
    ok_sprintf_1("%lf", 1.0, "1.000000");
    ok_sprintf_1("%lf", -1.0, "-1.000000");
    ok_sprintf_1("%lf", -123.45678, "-123.456780");
}

void test_float_e(void)
{
    ok_sprintf_1("%e", 1.0, "1.000000e+000");
    ok_sprintf_1("% 13e", 1.0, " 1.000000e+000");
    ok_sprintf_1("% 14e", 1.0, " 1.000000e+000");
    ok_sprintf_1("% 15e", 1.0, "  1.000000e+000");
    ok_sprintf_1("%013e", 1.0, "1.000000e+000");
    ok_sprintf_1("%014e", 1.0, "01.000000e+000");
    ok_sprintf_1("%015e", 1.0, "001.000000e+000");
    ok_sprintf_1("%.0e", 1.23456789, "1e+000");
    ok_sprintf_1("%.3e", 1.23456789, "1.235e+000");
    ok_sprintf_1("%.11e", 1.23456789, "1.23456789000e+000");
    ok_sprintf_1("%.16e", 1.23456789e20, "1.2345678900000000e+020");
    ok_sprintf_1("%20e", 1e30, "       1.000000e+030");
    ok_sprintf_1("%.20e", 1e30, "1.00000000000000000000e+030");
    ok_sprintf_1("%.20le", 1.0000000000000000199e+030, "1.00000000000000000000e+030");
    ok_sprintf_1("%.23le", 1.0000000000000000199e+030, "1.00000000000000000000000e+030");

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4477) // format string
#endif
    // MSVC uses the FPU, which can lead to wrong output, so we put the values
    // directly on the stack to get the correct results.
    ok_sprintf_1("%e", g_Inf, "1.#INF00e+000");
    ok_sprintf_1("%e", g_NegInf, "-1.#INF00e+000");
    ok_sprintf_1("%e", g_NaN1, "1.#SNAN0e+000");
    ok_sprintf_1("%e", g_NegNaN1, "-1.#SNAN0e+000");
    ok_sprintf_1("%e", g_NaN2, "1.#QNAN0e+000");
    ok_sprintf_1("%e", g_NaN3, "1.#QNAN0e+000");
    ok_sprintf_1("%e", g_NaN4, "1.#QNAN0e+000");
    ok_sprintf_1("%14e", g_Inf, " 1.#INF00e+000");
    ok_sprintf_1("%.10e", g_Inf, "1.#INF000000e+000");
    ok_sprintf_1("%.0e", g_Inf, "1e+000");
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    char buffer[100];
    int len = sprintf(buffer, "%-+.*E", 3, 999999999999.9);
    ok_str(buffer, "+1.000E+012");
    ok_int(len, 11);
}

void test_float_g(void)
{
    ok_sprintf_1("%g", 1.23456789, "1.23457");
    ok_sprintf_1("%.4g", 1.23456789, "1.235");
    ok_sprintf_1("%.4g", 10.23456789, "10.23");
    ok_sprintf_1("%g", 0.0100, "0.01");
    ok_sprintf_1("%g", 12.0100, "12.01");
    ok_sprintf_1("%g", 100000., "100000");
    ok_sprintf_1("%g", 999999.9, "1e+006");
    ok_sprintf_1("%g", 1000000., "1e+006");
    ok_sprintf_1("%g", 1200000., "1.2e+006");
    ok_sprintf_1("%f", 1.123e+007, "11230000.000000");
    ok_sprintf_1("%g", 1.123e+007, "1.123e+007");
    ok_sprintf_1("%.0g", 1.123e+007, "1e+007");
    ok_sprintf_1("%.1g", 1.123e+007, "1e+007");
    ok_sprintf_1("%.2g", 1.123e+007, "1.1e+007");
    ok_sprintf_1("%.7g", 1.123e+007, "1.123e+007");
    ok_sprintf_1("%.7e", 1.123e+007, "1.1230000e+007");
    ok_sprintf_1("%.7g", 1.123456789e+007, "1.123457e+007");
    ok_sprintf_1("%.7G", 9.9999999747524270788e-007, "1E-006");
}

void test_float_a(void)
{
    /* Test a few intersting values */
    ok_sprintf_1("%a", 0.0, "0x0.000000p+0");
    ok_sprintf_1("%a", -0.0, "-0x0.000000p+0");
    ok_sprintf_1("%a", 1.5, "0x1.800000p+0");
    ok_sprintf_1("%a", 2.0, "0x1.000000p+1");
    ok_sprintf_1("%a", 9.9999999747524270788e-007, "0x1.0c6f7ap-20");
    ok_sprintf_1("%.3a", 9.9999999747524270788e-007, "0x1.0c7p-20");
    ok_sprintf_1("%a", DBL_MIN, "0x1.000000p-1022");
    ok_sprintf_1("%a", DBL_MAX, "0x2.000000p+1023");
    ok_sprintf_1("%a", *(double*)&g_Inf, "0x1.#INF00p+0");
    ok_sprintf_1("%a", *(double*)&g_NegInf, "-0x1.#INF00p+0");
    ok_sprintf_1("%a", *(double*)&g_NaN1, "0x1.#SNAN0p+0");
    ok_sprintf_1("%a", *(double*)&g_NegNaN1, "-0x1.#SNAN0p+0");
    ok_sprintf_1("%a", *(double*)&g_NaN2, "0x1.#QNAN0p+0");
    ok_sprintf_1("%a", *(double*)&g_NaN3, "0x1.#QNAN0p+0");
    ok_sprintf_1("%a", *(double*)&g_NaN4, "0x1.#QNAN0p+0");
}

/* NOTE: This test is not only used for all the CRT apitests, but also for
 *       user32's wsprintf. Make sure to test them all */
START_TEST(sprintf)
{
    test_basic();
    test_int_d();
    test_string();
#if !defined(TEST_USER32) && !defined(TEST_NTDLL)
    test_float_f();
    test_float_e();
    test_float_g();
    if (NtCurrentPeb()->OSMajorVersion >= 6)
        todo_ros test_float_a();
    else
        skip("Skipping test_float_a() on Windows versions < Vista\n");
#endif
}
