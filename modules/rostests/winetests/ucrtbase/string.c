/*
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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <locale.h>
#include <mbctype.h>
#include <mbstring.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

#include <math.h>

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

_ACRTIMP int __cdecl _o_tolower(int);
_ACRTIMP int __cdecl _o_toupper(int);

static BOOL local_isnan(double d)
{
    return d != d;
}

#define test_strtod_str_errno(string, value, length, err) _test_strtod_str(__LINE__, string, value, length, err)
#define test_strtod_str(string, value, length) _test_strtod_str(__LINE__, string, value, length, 0)
static void _test_strtod_str(int line, const char* string, double value, int length, int err)
{
    char *end;
    double d;
    errno = 0xdeadbeef;
    d = strtod(string, &end);
    if(!err)
        ok_(__FILE__, line)(errno == 0xdeadbeef, "errno = %d\n", errno);
    else
        ok_(__FILE__, line)(errno == err, "errno = %d\n", errno);
    if (local_isnan(value))
        ok_(__FILE__, line)(local_isnan(d), "d = %.16le (\"%s\")\n", d, string);
    else
        ok_(__FILE__, line)(d == value, "d = %.16le (\"%s\")\n", d, string);
    ok_(__FILE__, line)(end == string + length, "incorrect end (%d, \"%s\")\n", (int)(end - string), string);
}

static void test_strtod(void)
{
    test_strtod_str("infinity", INFINITY, 8);
    test_strtod_str("INFINITY", INFINITY, 8);
    test_strtod_str("InFiNiTy", INFINITY, 8);
    test_strtod_str("INF", INFINITY, 3);
    test_strtod_str("-inf", -INFINITY, 4);
    test_strtod_str("inf42", INFINITY, 3);
    test_strtod_str("inffoo", INFINITY, 3);
    test_strtod_str("infini", INFINITY, 3);
    test_strtod_str("input", 0, 0);
    test_strtod_str("-input", 0, 0);
    test_strtod_str_errno("1.7976931348623159e+308", INFINITY, 23, ERANGE);
    test_strtod_str_errno("-1.7976931348623159e+308", -INFINITY, 24, ERANGE);

    test_strtod_str("NAN", NAN, 3);
    test_strtod_str("nan", NAN, 3);
    test_strtod_str("NaN", NAN, 3);

    test_strtod_str("0x42", 66, 4);
    test_strtod_str("0X42", 66, 4);
    test_strtod_str("-0x42", -66, 5);
    test_strtod_str("0x1p1", 2, 5);
    test_strtod_str("0x1P1", 2, 5);
    test_strtod_str("0x1p+1", 2, 6);
    test_strtod_str("0x2p-1", 1, 6);
    test_strtod_str("0xA", 10, 3);
    test_strtod_str("0xa", 10, 3);
    test_strtod_str("0xABCDEF", 11259375, 8);
    test_strtod_str("0Xabcdef", 11259375, 8);

    test_strtod_str("0x1.1", 1.0625, 5);
    test_strtod_str("0x1.1p1", 2.125, 7);
    test_strtod_str("0x1.A", 1.625, 5);
    test_strtod_str("0x1p1a", 2, 5);
    test_strtod_str("0xp3", 0, 1);
    test_strtod_str("0x.", 0, 1);
    test_strtod_str("0x.8", 0.5, 4);
    test_strtod_str("0x.8p", 0.5, 4);
    test_strtod_str("0x0p10000000000000000000000000", 0, 30);
    test_strtod_str("0x1p-1026", 1.3906711615670009e-309, 9);

    test_strtod_str("0x1ffffffffffffe.80000000000000000000", 9007199254740990.0, 37);
    test_strtod_str("0x1ffffffffffffe.80000000000000000001", 9007199254740991.0, 37);
    test_strtod_str("0x1fffffffffffff.80000000000000000000", 9007199254740992.0, 37);
    test_strtod_str("0x1fffffffffffff.80000000000000000001", 9007199254740992.0, 37);

    test_strtod_str("4.0621786324484881721115322e-53", 4.0621786324484881721115322e-53, 31);
    test_strtod_str("1.8905590910042396899370942", 1.8905590910042396899370942, 27);
    test_strtod_str("1.7976931348623158e+308", 1.7976931348623158e+308, 23);
    test_strtod_str("2.2250738585072014e-308", 2.2250738585072014e-308, 23);
    test_strtod_str("4.9406564584124654e-324", 4.9406564584124654e-324, 23);
    test_strtod_str("2.48e-324", 4.9406564584124654e-324, 9);
    test_strtod_str_errno("2.47e-324", 0, 9, ERANGE);
}

static void test_strtof(void)
{
    static const struct {
        const char *str;
        int len;
        float ret;
        int err;
    } tests[] = {
        { "12.1", 4, 12.1f },
        { "-13.721", 7, -13.721f },
        { "1.e40", 5, INFINITY, ERANGE },
        { "-1.e40", 6, -INFINITY, ERANGE },
        { "0.0", 3, 0.0f },
        { "-0.0", 4, 0.0f },
        { "1.4e-45", 7, 1.4e-45f },
        { "-1.4e-45", 8, -1.4e-45f },
        { "1.e-60", 6, 0, ERANGE },
        { "-1.e-60", 7, 0, ERANGE },
    };

    char *end;
    float f;
    int i;

    for (i=0; i<ARRAY_SIZE(tests); i++)
    {
        errno = 0xdeadbeef;
        f = strtof(tests[i].str, &end);
        ok(f == tests[i].ret, "%d) f = %.16e\n", i, f);
        ok(end == tests[i].str + tests[i].len, "%d) len = %d\n",
                i, (int)(end - tests[i].str));
        ok(errno == tests[i].err || (!tests[i].err && errno == 0xdeadbeef),
                "%d) errno = %d\n", i, errno);
    }
}

static void test__memicmp(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ret = _memicmp(NULL, NULL, 0);
    ok(!ret, "got %d\n", ret);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _memicmp(NULL, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _memicmp(s1, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _memicmp(NULL, s2, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    ret = _memicmp(s1, s2, 2);
    ok(!ret, "got %d\n", ret);

    ret = _memicmp(s1, s2, 3);
    ok(ret == -1, "got %d\n", ret);
}

static void test__memicmp_l(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ret = _memicmp_l(NULL, NULL, 0, NULL);
    ok(!ret, "got %d\n", ret);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _memicmp_l(NULL, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _memicmp_l(s1, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _memicmp_l(NULL, s2, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    ret = _memicmp_l(s1, s2, 2, NULL);
    ok(!ret, "got %d\n", ret);

    ret = _memicmp_l(s1, s2, 3, NULL);
    ok(ret == -1, "got %d\n", ret);
}


static void test___strncnt(void)
{
    static const struct
    {
        const char *str;
        size_t size;
        size_t ret;
    }
    strncnt_tests[] =
    {
        { "a", 0, 0 },
        { "a", 1, 1 },
        { "a", 10, 1 },
        { "abc", 1, 1 },
    };
    unsigned int i;
    size_t ret;

    for (i = 0; i < ARRAY_SIZE(strncnt_tests); ++i)
    {
        ret = __strncnt(strncnt_tests[i].str, strncnt_tests[i].size);
        ok(ret == strncnt_tests[i].ret, "%u: unexpected return value %u.\n", i, (int)ret);
    }

    if (0) /* crashes */
    {
        ret = __strncnt(NULL, 0);
        ret = __strncnt(NULL, 1);
    }
}

static void test_C_locale(void)
{
    int i, j;
    wint_t ret, exp;
    _locale_t locale;
    static const char *locales[] = { NULL, "C" };

    /* C locale only converts case for [a-zA-Z] */
    setlocale(LC_ALL, "C");
    for (i = 0; i <= 0xffff; i++)
    {
        ret = tolower(i);
        if (i >= 'A' && i <= 'Z')
        {
            exp = i + 'a' - 'A';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);

        ret = _tolower(i);
        exp = i + 'a' - 'A';
        ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);

        ret = _o_tolower(i);
        if (i >= 'A' && i <= 'Z')
        {
            exp = i + 'a' - 'A';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);

        ret = towlower(i);
        if (i >= 'A' && i <= 'Z')
        {
            exp = i + 'a' - 'A';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);

        ret = toupper(i);
        if (i >= 'a' && i <= 'z')
        {
            exp = i + 'A' - 'a';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);

        ret = _toupper(i);
        exp = i + 'A' - 'a';
        ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);

        ret = _o_toupper(i);
        if (i >= 'a' && i <= 'z')
        {
            exp = i + 'A' - 'a';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);

        ret = towupper(i);
        if (i >= 'a' && i <= 'z')
        {
            exp = i + 'A' - 'a';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);
    }

    for (i = 0; i < ARRAY_SIZE(locales); i++) {
        locale = locales[i] ? _create_locale(LC_ALL, locales[i]) : NULL;

        for (j = 0; j <= 0xffff; j++) {
            ret = _towlower_l(j, locale);
            if (j >= 'A' && j <= 'Z')
            {
                exp = j + 'a' - 'A';
                ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
            }
            else
                ok(ret == j, "expected self %x, got %x for C locale\n", j, ret);

            ret = _towupper_l(j, locale);
            if (j >= 'a' && j <= 'z')
            {
                exp = j + 'A' - 'a';
                ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
            }
            else
                ok(ret == j, "expected self %x, got %x for C locale\n", j, ret);
        }

        _free_locale(locale);
    }
}

static void test_mbsspn( void)
{
    unsigned char str1[] = "cabernet";
    unsigned char str2[] = "shiraz";
    unsigned char set[] = "abc";
    unsigned char empty[] = "";
    unsigned char mbstr[] = " 2019\x94\x4e" "6\x8c\x8e" "29\x93\xfa";
    unsigned char mbset1[] = "0123456789 \x94\x4e";
    unsigned char mbset2[] = " \x94\x4e\x8c\x8e";
    unsigned char mbset3[] = "\x8e";
    int ret, cp = _getmbcp();

    ret = _mbsspn(str1, set);
    ok(ret == 3, "_mbsspn returns %d should be 3\n", ret);
    ret = _mbsspn(str2, set);
    ok(ret == 0, "_mbsspn returns %d should be 0\n", ret);
    ret = _mbsspn(str1, empty);
    ok(ret == 0, "_mbsspn returns %d should be 0\n", ret);

    _setmbcp(932);
    ret = _mbsspn(mbstr, mbset1);
    ok(ret == 8, "_mbsspn returns %d should be 8\n", ret);
    ret = _mbsspn(mbstr, mbset2);
    ok(ret == 1, "_mbsspn returns %d should be 1\n", ret);
    ret = _mbsspn(mbstr+8, mbset1);
    ok(ret == 0, "_mbsspn returns %d should be 0\n", ret);
    ret = _mbsspn(mbstr+8, mbset2);
    ok(ret == 2, "_mbsspn returns %d should be 2\n", ret);
    ret = _mbsspn(mbstr, mbset3);
    ok(ret == 14, "_mbsspn returns %d should be 14\n", ret);

    _setmbcp(cp);
}

static void test_wcstok(void)
{
    static const wchar_t *input = L"two words";
    wchar_t buffer[16];
    wchar_t *token;
    wchar_t *next;

    next = NULL;
    wcscpy(buffer, input);
    token = wcstok(buffer, L" ", &next);
    ok(!wcscmp(L"two", token), "expected \"two\", got \"%ls\"\n", token);
    ok(next == token + 4, "expected %p, got %p\n", token + 4, next);
    token = wcstok(NULL, L" ", &next);
    ok(!wcscmp(L"words", token), "expected \"words\", got \"%ls\"\n", token);
    ok(next == token + 5, "expected %p, got %p\n", token + 5, next);
    token = wcstok(NULL, L" ", &next);
    ok(!token, "expected NULL, got %p\n", token);

    wcscpy(buffer, input);
    token = wcstok(buffer, L" ", NULL);
    ok(!wcscmp(L"two", token), "expected \"two\", got \"%ls\"\n", token);
    token = wcstok(NULL, L" ", NULL);
    ok(!wcscmp(L"words", token), "expected \"words\", got \"%ls\"\n", token);
    token = wcstok(NULL, L" ", NULL);
    ok(!token, "expected NULL, got %p\n", token);

    next = NULL;
    wcscpy(buffer, input);
    token = wcstok(buffer, L"=", &next);
    ok(!wcscmp(token, input), "expected \"%ls\", got \"%ls\"\n", input, token);
    ok(next == buffer + wcslen(input), "expected %p, got %p\n", buffer + wcslen(input), next);
    token = wcstok(NULL, L"=", &next);
    ok(!token, "expected NULL, got \"%ls\"\n", token);
    ok(next == buffer + wcslen(input), "expected %p, got %p\n", buffer + wcslen(input), next);

    next = NULL;
    wcscpy(buffer, L"");
    token = wcstok(buffer, L"=", &next);
    ok(token == NULL, "expected NULL, got \"%ls\"\n", token);
    ok(next == buffer, "expected %p, got %p\n", buffer, next);
    token = wcstok(NULL, L"=", &next);
    ok(!token, "expected NULL, got \"%ls\"\n", token);
    ok(next == buffer, "expected %p, got %p\n", buffer, next);
}

static void test__strnicmp(void)
{
    static const char str1[] = "TEST";
    static const char str2[] = "test";
    int ret;

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _strnicmp(str1, NULL, 2);
    CHECK_CALLED(invalid_parameter_handler);
    ok(ret == _NLSCMPERROR, "got %d.\n", ret);
    ok(errno == EINVAL, "Unexpected errno %d.\n", errno);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _strnicmp(str1, str2, -1);
    CHECK_CALLED(invalid_parameter_handler);
    ok(ret == _NLSCMPERROR, "got %d.\n", ret);
    ok(errno == EINVAL, "Unexpected errno %d.\n", errno);

    ret = _strnicmp(str1, str2, 0);
    ok(!ret, "got %d.\n", ret);

    ret = _strnicmp(str1, str2, 0x7fffffff);
    ok(!ret, "got %d.\n", ret);

    /* If numbers of characters to compare is too big return error */
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = _strnicmp(str1, str2, 0x80000000);
    CHECK_CALLED(invalid_parameter_handler);
    ok(ret == _NLSCMPERROR, "got %d.\n", ret);
    ok(errno == EINVAL, "Unexpected errno %d.\n", errno);
}

static void test_wcsnicmp(void)
{
    static const wchar_t str1[] = L"TEST";
    static const wchar_t str2[] = L"test";
    int ret;

    errno = 0xdeadbeef;
    ret = wcsnicmp(str1, str2, -1);
    ok(!ret, "got %d.\n", ret);

    ret = wcsnicmp(str1, str2, 0x7fffffff);
    ok(!ret, "got %d.\n", ret);
}

static void test_SpecialCasing(void)
{
    int i;
    wint_t ret, exp;
    _locale_t locale;
    struct test {
        const char *lang;
        wint_t ch;
        wint_t exp;
    };

    struct test ucases[] = {
        {"English", 'I', 'i'}, /* LATIN CAPITAL LETTER I */
        {"English", 0x0130},   /* LATIN CAPITAL LETTER I WITH DOT ABOVE */

        {"Turkish", 'I', 'i'}, /* LATIN CAPITAL LETTER I */
        {"Turkish", 0x0130},   /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    };
    struct test lcases[] = {
        {"English", 'i', 'I'}, /* LATIN SMALL LETTER I */
        {"English", 0x0131},   /* LATIN SMALL LETTER DOTLESS I */

        {"Turkish", 'i', 'I'}, /* LATIN SMALL LETTER I */
        {"Turkish", 0x0131},   /* LATIN SMALL LETTER DOTLESS I */
    };

    for (i = 0; i < ARRAY_SIZE(ucases); i++) {
        if (!setlocale(LC_ALL, ucases[i].lang)) {
            win_skip("skipping special case tests for %s\n", ucases[i].lang);
            continue;
        }

        ret = towlower(ucases[i].ch);
        exp = ucases[i].exp ? ucases[i].exp : ucases[i].ch;
        ok(ret == exp, "expected lowercase %x, got %x for locale %s\n", exp, ret, ucases[i].lang);
    }

    for (i = 0; i < ARRAY_SIZE(lcases); i++) {
        if (!setlocale(LC_ALL, lcases[i].lang)) {
            win_skip("skipping special case tests for %s\n", lcases[i].lang);
            continue;
        }

        ret = towupper(lcases[i].ch);
        exp = lcases[i].exp ? lcases[i].exp : lcases[i].ch;
        ok(ret == exp, "expected uppercase %x, got %x for locale %s\n", exp, ret, lcases[i].lang);
    }

    setlocale(LC_ALL, "C");

    /* test _towlower_l creating locale */
    for (i = 0; i < ARRAY_SIZE(ucases); i++) {
        if (!(locale = _create_locale(LC_ALL, ucases[i].lang))) {
            win_skip("locale %s not available.  skipping\n", ucases[i].lang);
            continue;
        }

        ret = _towlower_l(ucases[i].ch, locale);
        exp = ucases[i].exp ? ucases[i].exp : ucases[i].ch;
        ok(ret == exp, "expected lowercase %x, got %x for locale %s\n", exp, ret, ucases[i].lang);

        _free_locale(locale);
    }

    /* test _towupper_l creating locale */
    for (i = 0; i < ARRAY_SIZE(lcases); i++) {
        if (!(locale = _create_locale(LC_ALL, lcases[i].lang))) {
            win_skip("locale %s not available.  skipping\n", lcases[i].lang);
            continue;
        }

        ret = _towupper_l(lcases[i].ch, locale);
        exp = lcases[i].exp ? lcases[i].exp : lcases[i].ch;
        ok(ret == exp, "expected uppercase %x, got %x for locale %s\n", exp, ret, lcases[i].lang);

        _free_locale(locale);
    }
}

static void test__mbbtype_l(void)
{
    int expected, ret;
    unsigned int c;

    _setmbcp(_MB_CP_LOCALE);
    for (c = 0; c < 256; ++c)
    {
        expected = _mbbtype(c, 0);
        ret = _mbbtype_l(c, 0, NULL);
        ok(ret == expected, "c %#x, got ret %#x, expected %#x.\n", c, ret, expected);

        expected = _mbbtype(c, 1);
        ret = _mbbtype_l(c, 1, NULL);
        ok(ret == expected, "c %#x, got ret %#x, expected %#x.\n", c, ret, expected);
    }
}

static void test_strcmp(void)
{
    int ret = strcmp( "abc", "abcd" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "", "abc" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "abc", "ab\xa0" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "ab\xb0", "ab\xa0" );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = strcmp( "ab\xc2", "ab\xc2" );
    ok( ret == 0, "wrong ret %d\n", ret );

    ret = strncmp( "abc", "abcd", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "", "abc", 3 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "ab\xa0", 4 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xb0", "ab\xa0", 2 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xc2", "ab\xc2", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "abd", 0 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "abc", 12 );
    ok( ret == 0, "wrong ret %d\n", ret );
}

#define expect_bin(buf, value, len) { ok(memcmp((buf), value, len) == 0, \
                                         "Binary buffer mismatch - expected %s, got %s\n", \
                                         debugstr_an(value, len), debugstr_an((char *)(buf), len)); }

static void test__mbsncpy_s(void)
{
    unsigned char *mbstring = (unsigned char *)"\xb0\xb1\xb2\xb3Q\xb4\xb5\x0";
    unsigned char *mbstring2 = (unsigned char *)"\xb0\x0";
    unsigned char buf[16];
    errno_t err;
    int oldcp;

    oldcp = _getmbcp();
    if (_setmbcp(936))
    {
        skip("Code page 936 is not available, skipping test.\n");
        return;
    }

    errno = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    err = _mbsncpy_s(NULL, 0, mbstring, 0);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);

    errno = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    err = _mbsncpy_s(buf, 6, mbstring, 1);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\xb0\xb1\0\xcc", 4);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 6, mbstring, 2);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\xb0\xb1\xb2\xb3\0\xcc", 6);

    errno = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    err = _mbsncpy_s(buf, 2, mbstring, _TRUNCATE);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(err == STRUNCATE, "got %d.\n", err);
    expect_bin(buf, "\x00\xb1\xcc", 3);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 2, mbstring, 1);
    ok(errno == err, "got %d.\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == ERANGE, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc\xcc", 3);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 2, mbstring, 3);
    ok(errno == err, "got %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == ERANGE, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc\xcc", 3);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 1, mbstring, 3);
    ok(errno == err, "got %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == ERANGE, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 0, mbstring, 3);
    ok(errno == err, "got %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == EINVAL, "got %d.\n", err);
    expect_bin(buf, "\xcc", 1);

    memset(buf, 0xcc, sizeof(buf));
    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 0, mbstring, 0);
    ok(errno == err, "got %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);
    ok(err == EINVAL, "got %d.\n", err);
    expect_bin(buf, "\xcc", 1);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, -1, mbstring, 0);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, -1, mbstring, 256);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\xb0\xb1\xb2\xb3Q\xb4\xb5\x0\xcc", 9);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 1, mbstring2, 4);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 2, mbstring2, 4);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 1, mbstring2, _TRUNCATE);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(err == STRUNCATE, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 2, mbstring2, _TRUNCATE);
    ok(errno == 0xdeadbeef, "got %d\n", errno);
    ok(!err, "got %d.\n", err);
    expect_bin(buf, "\xb0\x0\xcc", 3);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 1, mbstring2, 1);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 2, mbstring2, 1);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 3, mbstring2, 1);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    memset(buf, 0xcc, sizeof(buf));
    errno = 0xdeadbeef;
    err = _mbsncpy_s(buf, 3, mbstring2, 2);
    ok(errno == err, "got %d\n", errno);
    ok(err == EILSEQ, "got %d.\n", err);
    expect_bin(buf, "\x0\xcc", 2);

    _setmbcp(oldcp);
}

static void test_mbstowcs(void)
{
    static const char mbs[] = { 0xc3, 0xa9, 0 };
    WCHAR wcs[2];
    size_t ret;

    if (!setlocale(LC_ALL, "en_US.UTF-8"))
    {
        win_skip("skipping UTF8 mbstowcs tests\n");
        return;
    }

    ret = mbstowcs(NULL, mbs, 0);
    ok(ret == 1, "mbstowcs returned %Id\n", ret);
    memset(wcs, 0xfe, sizeof(wcs));
    ret = mbstowcs(wcs, mbs, 1);
    ok(ret == 1, "mbstowcs returned %Id\n", ret);
    ok(wcs[0] == 0xe9, "wcsstring[0] = %x\n", wcs[0]);
    ok(wcs[1] == 0xfefe, "wcsstring[1] = %x\n", wcs[1]);
    setlocale(LC_ALL, "C");
}

START_TEST(string)
{
    ok(_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    test_strtod();
    test_strtof();
    test__memicmp();
    test__memicmp_l();
    test___strncnt();
    test_C_locale();
    test_mbsspn();
    test_wcstok();
    test__strnicmp();
    test_wcsnicmp();
    test_SpecialCasing();
    test__mbbtype_l();
    test_strcmp();
    test__mbsncpy_s();
    test_mbstowcs();
}
