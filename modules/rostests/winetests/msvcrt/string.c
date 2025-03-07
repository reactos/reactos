/*
 * Unit test suite for string functions.
 *
 * Copyright 2004 Uwe Bonnes
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

#include "wine/test.h"
#include <string.h>
#include <mbstring.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <mbctype.h>
#include <locale.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <math.h>

/* make it use a definition from string.h */
#undef strncpy
#undef wcsncpy
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include <versionhelpers.h>

static char *buf_to_string(const unsigned char *bin, int len, int nr)
{
    static char buf[2][1024];
    char *w = buf[nr];
    int i;

    for (i = 0; i < len; i++)
    {
        sprintf(w, "%02x ", (unsigned char)bin[i]);
        w += strlen(w);
    }
    return buf[nr];
}

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }
#define expect_bin(buf, value, len) { ok(memcmp((buf), value, len) == 0, "Binary buffer mismatch - expected %s, got %s\n", buf_to_string((unsigned char *)value, len, 1), buf_to_string((buf), len, 0)); }

static void* (__cdecl *pmemcpy)(void *, const void *, size_t n);
static int (__cdecl *p_memcpy_s)(void *, size_t, const void *, size_t);
static int (__cdecl *p_memmove_s)(void *, size_t, const void *, size_t);
static int* (__cdecl *pmemcmp)(void *, const void *, size_t n);
static int (__cdecl *p_strcmp)(const char *, const char *);
static int (__cdecl *p_strncmp)(const char *, const char *, size_t);
static int (__cdecl *p_strcpy)(char *dst, const char *src);
static int (__cdecl *pstrcpy_s)(char *dst, size_t len, const char *src);
static int (__cdecl *pstrcat_s)(char *dst, size_t len, const char *src);
static int (__cdecl *p_strncpy_s)(char *dst, size_t size, const char *src, size_t count);
static int (__cdecl *p_strncat_s)(char *dst, size_t elem, const char *src, size_t count);
static int (__cdecl *p_mbscat_s)(unsigned char *dst, size_t size, const unsigned char *src);
static int (__cdecl *p_mbsnbcat_s)(unsigned char *dst, size_t size, const unsigned char *src, size_t count);
static int (__cdecl *p_mbsnbcpy_s)(unsigned char * dst, size_t size, const unsigned char * src, size_t count);
static int (__cdecl *p__mbscpy_s)(unsigned char*, size_t, const unsigned char*);
static int (__cdecl *p_wcscpy_s)(wchar_t *wcDest, size_t size, const wchar_t *wcSrc);
static int (__cdecl *p_wcsncpy_s)(wchar_t *wcDest, size_t size, const wchar_t *wcSrc, size_t count);
static int (__cdecl *p_wcsncat_s)(wchar_t *dst, size_t elem, const wchar_t *src, size_t count);
static int (__cdecl *p_wcsupr_s)(wchar_t *str, size_t size);
static size_t (__cdecl *p_strnlen)(const char *, size_t);
static __int64 (__cdecl *p_strtoi64)(const char *, char **, int);
static unsigned __int64 (__cdecl *p_strtoui64)(const char *, char **, int);
static __int64 (__cdecl *p_wcstoi64)(const wchar_t *, wchar_t **, int);
static unsigned __int64 (__cdecl *p_wcstoui64)(const wchar_t *, wchar_t **, int);
static int (__cdecl *pwcstombs_s)(size_t*,char*,size_t,const wchar_t*,size_t);
static int (__cdecl *p_wcstombs_s_l)(size_t*,char*,size_t,const wchar_t*,size_t,_locale_t);
static int (__cdecl *pmbstowcs_s)(size_t*,wchar_t*,size_t,const char*,size_t);
static size_t (__cdecl *p_mbsrtowcs)(wchar_t*, const char**, size_t, mbstate_t*);
static int (__cdecl *p_mbsrtowcs_s)(size_t*,wchar_t*,size_t,const char**,size_t,mbstate_t*);
static size_t (__cdecl *pwcsrtombs)(char*, const wchar_t**, size_t, int*);
static errno_t (__cdecl *p_gcvt_s)(char*,size_t,double,int);
static errno_t (__cdecl *p_itoa_s)(int,char*,size_t,int);
static errno_t (__cdecl *p_strlwr_s)(char*,size_t);
static errno_t (__cdecl *p_ultoa_s)(__msvcrt_ulong,char*,size_t,int);
static int *p__mb_cur_max;
static unsigned char *p_mbctype;
static int (__cdecl *p_wcslwr_s)(wchar_t*,size_t);
static errno_t (__cdecl *p_mbsupr_s)(unsigned char *str, size_t numberOfElements);
static errno_t (__cdecl *p_mbslwr_s)(unsigned char *str, size_t numberOfElements);
static int (__cdecl *p_wctob)(wint_t);
static wint_t (__cdecl *p_btowc)(int);
static size_t (__cdecl *p_wcrtomb)(char*, wchar_t, mbstate_t*);
static int (__cdecl *p_wcrtomb_s)(size_t*, char*, size_t, wchar_t, mbstate_t*);
static int (__cdecl *p_tolower)(int);
static int (__cdecl *p_towlower)(wint_t);
static int (__cdecl *p__towlower_l)(wint_t, _locale_t);
static int (__cdecl *p_towupper)(wint_t);
static int (__cdecl *p__towupper_l)(wint_t, _locale_t);
static _locale_t(__cdecl *p__create_locale)(int, const char*);
static void(__cdecl *p__free_locale)(_locale_t);
static size_t (__cdecl *p_mbrlen)(const char*, size_t, mbstate_t*);
static size_t (__cdecl *p_mbrtowc)(wchar_t*, const char*, size_t, mbstate_t*);
static int (__cdecl *p__atodbl_l)(_CRT_DOUBLE*,char*,_locale_t);
static double (__cdecl *p__atof_l)(const char*,_locale_t);
static double (__cdecl *p__strtod_l)(const char *,char**,_locale_t);
static int (__cdecl *p__strnset_s)(char*,size_t,int,size_t);
static int (__cdecl *p__wcsnset_s)(wchar_t*,size_t,wchar_t,size_t);
static int (__cdecl *p__wcsset_s)(wchar_t*,size_t,wchar_t);
static size_t (__cdecl *p__mbsnlen)(const unsigned char*, size_t);
static int (__cdecl *p__mbccpy_s)(unsigned char*, size_t, int*, const unsigned char*);
static int (__cdecl *p__memicmp)(const char*, const char*, size_t);
static int (__cdecl *p__memicmp_l)(const char*, const char*, size_t, _locale_t);
static size_t (__cdecl *p___strncnt)(const char*, size_t);
static unsigned int (__cdecl *p_mbsnextc_l)(const unsigned char*, _locale_t);
static int (__cdecl *p_mbscmp_l)(const unsigned char*, const unsigned char*, _locale_t);
static int (__cdecl *p__strnicmp_l)(const char*, const char*, size_t, _locale_t);
static int (__cdecl *p_toupper)(int);

int CDECL __STRINGTOLD(_LDOUBLE*, char**, const char*, int);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hMsvcrt,y)
#define SET(x,y) SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y)

static HMODULE hMsvcrt;

static void test_swab( void ) {
    char original[]  = "BADCFEHGJILKNMPORQTSVUXWZY@#";
    char expected1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ@#";
    char expected2[] = "ABCDEFGHIJKLMNOPQRSTUVWX$";
    char expected3[] = "$";

    char from[30];
    char to[30];

    int testsize;

    /* Test 1 - normal even case */
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 26;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected1,testsize) == 0, "Testing even size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 2 - uneven case  */
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 25;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected2,testsize) == 0, "Testing odd size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 3 - from = to */
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 26;
    memcpy(to, original, testsize);
    _swab( to, to, testsize );
    ok(memcmp(to,expected1,testsize) == 0, "Testing overlapped size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 4 - 1 bytes */
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 1;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected3,testsize) == 0, "Testing small size %d returned '%*.*s'\n", testsize, testsize, testsize, to);
}

static void test_strcspn(void)
{
    static const struct {
        const char *str;
        const char *rej;
        int ret;
    } tests[] = {
        { "test", "a", 4 },
        { "test", "e", 1 },
        { "test", "", 4 },
        { "", "a", 0 },
        { "a\xf1", "\xf1", 1 }
    };
    int r, i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        r = strcspn(tests[i].str, tests[i].rej);
        ok(r == tests[i].ret, "strcspn(\"%s\", \"%s\") = %d, expected %d\n",
                tests[i].str, tests[i].rej, r, tests[i].ret);
    }
}

#if 0      /* use this to generate more tests */

static void test_codepage(int cp)
{
    int i;
    int prev;
    int count = 1;

    ok(_setmbcp(cp) == 0, "Couldn't set mbcp\n");

    prev = p_mbctype[0];
    printf("static int result_cp_%d_mbctype[] = { ", cp);
    for (i = 1; i < 257; i++)
    {
        if (p_mbctype[i] != prev)
        {
            printf("0x%x,%d, ", prev, count);
            prev = p_mbctype[i];
            count = 1;
        }
        else
            count++;
    }
    printf("0x%x,%d };\n", prev, count);
}

#else

/* RLE-encoded mbctype tables for given codepages */
static int result_cp_932_mbctype[] = { 0x0,65, 0x8,1, 0x18,26, 0x8,6, 0x28,26, 0x8,4,
  0x0,1, 0x8,1, 0xc,31, 0x8,1, 0xa,5, 0x9,58, 0xc,29, 0,3 };
static int result_cp_936_mbctype[] = { 0x0,65, 0x8,1, 0x18,26, 0x8,6, 0x28,26, 0x8,6,
  0xc,126, 0,1 };
static int result_cp_949_mbctype[] = { 0x0,66, 0x18,26, 0x8,6, 0x28,26, 0x8,6, 0xc,126,
  0,1 };
static int result_cp_950_mbctype[] = { 0x0,65, 0x8,1, 0x18,26, 0x8,6, 0x28,26, 0x8,4,
  0x0,2, 0x4,32, 0xc,94, 0,1 };

static void test_cp_table(int cp, int *result)
{
    int i;
    int count = 0;
    int curr = 0;
    _setmbcp(cp);
    for (i = 0; i < 256; i++)
    {
        if (count == 0)
        {
            curr = result[0];
            count = result[1];
            result += 2;
        }
        ok(p_mbctype[i] == curr, "CP%d: Mismatch in ctype for character %d - %d instead of %d\n", cp, i-1, p_mbctype[i], curr);
        count--;
    }
}

#define test_codepage(num) test_cp_table(num, result_cp_##num##_mbctype);

#endif

static void test_mbcp(void)
{
    int mb_orig_max = *p__mb_cur_max;
    int curr_mbcp = _getmbcp();
    unsigned char *mbstring = (unsigned char *)"\xb0\xb1\xb2 \xb3\xb4 \xb5"; /* incorrect string */
    unsigned char *mbstring2 = (unsigned char *)"\xb0\xb1\xb2\xb3Q\xb4\xb5"; /* correct string */
    unsigned char *mbsonlylead = (unsigned char *)"\xb0\0\xb1\xb2 \xb3";
    unsigned char buf[16];
    unsigned char *ret;
    int step;
    CPINFO cp_info;

    /* _mbtype tests */

    /* An SBCS codepage test. The ctype of characters on e.g. CP1252 or CP1250 differs slightly
     * between versions of Windows. Also Windows 9x seems to ignore the codepage and always uses
     * CP1252 (or the ACP?) so we test only a few ASCII characters */
    _setmbcp(1252);
    expect_eq(p_mbctype[10], 0, char, "%x");
    expect_eq(p_mbctype[50], 0, char, "%x");
    expect_eq(p_mbctype[66], _SBUP, char, "%x");
    expect_eq(p_mbctype[100], _SBLOW, char, "%x");
    expect_eq(p_mbctype[128], 0, char, "%x");
    _setmbcp(1250);
    expect_eq(p_mbctype[10], 0, char, "%x");
    expect_eq(p_mbctype[50], 0, char, "%x");
    expect_eq(p_mbctype[66], _SBUP, char, "%x");
    expect_eq(p_mbctype[100], _SBLOW, char, "%x");
    expect_eq(p_mbctype[128], 0, char, "%x");

    /* double byte code pages */
    test_codepage(932);
    test_codepage(936);
    test_codepage(949);
    test_codepage(950);

    _setmbcp(936);
    ok(*p__mb_cur_max == mb_orig_max, "__mb_cur_max shouldn't be updated (is %d != %d)\n", *p__mb_cur_max, mb_orig_max);
    ok(_ismbblead('\354'), "\354 should be a lead byte\n");
    ok(_ismbblead(' ') == FALSE, "' ' should not be a lead byte\n");
    ok(_ismbblead(0x1234b0), "0x1234b0 should not be a lead byte\n");
    ok(_ismbblead(0x123420) == FALSE, "0x123420 should not be a lead byte\n");
    ok(_ismbbtrail('\xb0'), "\xa0 should be a trail byte\n");
    ok(_ismbbtrail(' ') == FALSE, "' ' should not be a trail byte\n");

    /* _ismbslead */
    expect_eq(_ismbslead(mbstring, &mbstring[0]), -1, int, "%d");
    expect_eq(_ismbslead(mbstring, &mbstring[1]), FALSE, int, "%d");
    expect_eq(_ismbslead(mbstring, &mbstring[2]), -1, int, "%d");
    expect_eq(_ismbslead(mbstring, &mbstring[3]), FALSE, int, "%d");
    expect_eq(_ismbslead(mbstring, &mbstring[4]), -1, int, "%d");
    expect_eq(_ismbslead(mbstring, &mbstring[5]), FALSE, int, "%d");
    expect_eq(_ismbslead(mbstring, &mbstring[6]), FALSE, int, "%d");
    expect_eq(_ismbslead(mbstring, &mbstring[7]), -1, int, "%d");
    expect_eq(_ismbslead(mbstring, &mbstring[8]), FALSE, int, "%d");

    expect_eq(_ismbslead(mbsonlylead, &mbsonlylead[0]), -1, int, "%d");
    expect_eq(_ismbslead(mbsonlylead, &mbsonlylead[1]), FALSE, int, "%d");
    expect_eq(_ismbslead(mbsonlylead, &mbsonlylead[2]), FALSE, int, "%d");
    expect_eq(_ismbslead(mbsonlylead, &mbsonlylead[5]), FALSE, int, "%d");

    /* _ismbstrail */
    expect_eq(_ismbstrail(mbstring, &mbstring[0]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbstring, &mbstring[1]), -1, int, "%d");
    expect_eq(_ismbstrail(mbstring, &mbstring[2]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbstring, &mbstring[3]), -1, int, "%d");
    expect_eq(_ismbstrail(mbstring, &mbstring[4]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbstring, &mbstring[5]), -1, int, "%d");
    expect_eq(_ismbstrail(mbstring, &mbstring[6]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbstring, &mbstring[7]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbstring, &mbstring[8]), -1, int, "%d");

    expect_eq(_ismbstrail(mbsonlylead, &mbsonlylead[0]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbsonlylead, &mbsonlylead[1]), -1, int, "%d");
    expect_eq(_ismbstrail(mbsonlylead, &mbsonlylead[2]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbsonlylead, &mbsonlylead[3]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbsonlylead, &mbsonlylead[4]), FALSE, int, "%d");
    expect_eq(_ismbstrail(mbsonlylead, &mbsonlylead[5]), FALSE, int, "%d");

    /* _mbsbtype */
    skip_2k3_crash expect_eq(_mbsbtype(NULL, 0), _MBC_ILLEGAL, int, "%d");
    expect_eq(_mbsbtype(mbstring, 0), _MBC_LEAD, int, "%d");
    expect_eq(_mbsbtype(mbstring, 1), _MBC_TRAIL, int, "%d");
    expect_eq(_mbsbtype(mbstring, 2), _MBC_LEAD, int, "%d");
    expect_eq(_mbsbtype(mbstring, 3), _MBC_ILLEGAL, int, "%d");
    expect_eq(_mbsbtype(mbstring, 4), _MBC_LEAD, int, "%d");
    expect_eq(_mbsbtype(mbstring, 5), _MBC_TRAIL, int, "%d");
    expect_eq(_mbsbtype(mbstring, 6), _MBC_SINGLE, int, "%d");
    expect_eq(_mbsbtype(mbstring, 7), _MBC_LEAD, int, "%d");
    expect_eq(_mbsbtype(mbstring, 8), _MBC_ILLEGAL, int, "%d");

    expect_eq(_mbsbtype(mbsonlylead, 0), _MBC_LEAD, int, "%d");
    expect_eq(_mbsbtype(mbsonlylead, 1), _MBC_ILLEGAL, int, "%d");
    expect_eq(_mbsbtype(mbsonlylead, 2), _MBC_ILLEGAL, int, "%d");
    expect_eq(_mbsbtype(mbsonlylead, 3), _MBC_ILLEGAL, int, "%d");
    expect_eq(_mbsbtype(mbsonlylead, 4), _MBC_ILLEGAL, int, "%d");
    expect_eq(_mbsbtype(mbsonlylead, 5), _MBC_ILLEGAL, int, "%d");

    /* _mbsnextc */
    expect_eq(_mbsnextc(mbstring), 0xb0b1, int, "%x");
    expect_eq(_mbsnextc(&mbstring[2]), 0xb220, int, "%x");  /* lead + invalid tail */
    expect_eq(_mbsnextc(&mbstring[3]), 0x20, int, "%x");    /* single char */

    if (!p_mbsnextc_l)
        win_skip("_mbsnextc_l tests\n");
    else
        expect_eq(p_mbsnextc_l(mbstring, NULL), 0xb0b1, int, "%x");

    /* _mbclen/_mbslen */
    expect_eq(_mbclen(mbstring), 2, int, "%d");
    expect_eq(_mbclen(&mbstring[2]), 2, int, "%d");
    expect_eq(_mbclen(&mbstring[3]), 1, int, "%d");
    expect_eq(_mbclen(mbsonlylead), 1, int, "%d");
    expect_eq(_mbslen(mbstring2), 4, int, "%d");
    expect_eq(_mbslen(mbsonlylead), 0, int, "%d");          /* lead + NUL not counted as character */
    expect_eq(_mbslen(mbstring), 4, int, "%d");             /* lead + invalid trail counted */

    if(!p__mbsnlen) {
        win_skip("_mbsnlen tests\n");
    }else {
        expect_eq(p__mbsnlen(mbstring, 8), 8, int, "%d");
        expect_eq(p__mbsnlen(mbstring, 9), 4, int, "%d");
        expect_eq(p__mbsnlen(mbstring, 10), 4, int, "%d");
        expect_eq(p__mbsnlen(mbsonlylead, 0), 0, int, "%d");
        expect_eq(p__mbsnlen(mbsonlylead, 1), 1, int, "%d");
        expect_eq(p__mbsnlen(mbsonlylead, 2), 0, int, "%d");
        expect_eq(p__mbsnlen(mbstring2, 7), 7, int, "%d");
        expect_eq(p__mbsnlen(mbstring2, 8), 4, int, "%d");
        expect_eq(p__mbsnlen(mbstring2, 9), 4, int, "%d");
    }

    /* mbrlen */
    if(!setlocale(LC_ALL, ".936") || !p_mbrlen) {
        win_skip("mbrlen tests\n");
    }else {
        mbstate_t state = 0;
        expect_eq(p_mbrlen((const char*)mbstring, 2, NULL), 2, int, "%d");
        expect_eq(p_mbrlen((const char*)&mbstring[2], 2, NULL), 2, int, "%d");
        expect_eq(p_mbrlen((const char*)&mbstring[3], 2, NULL), 1, int, "%d");
        expect_eq(p_mbrlen((const char*)mbstring, 1, NULL), -2, int, "%d");
        expect_eq(p_mbrlen((const char*)mbstring, 1, &state), -2, int, "%d");
        ok(state == mbstring[0], "incorrect state value (%x)\n", state);
        expect_eq(p_mbrlen((const char*)&mbstring[1], 1, &state), 2, int, "%d");
    }

    /* mbrtowc */
    if(!setlocale(LC_ALL, ".936") || !p_mbrtowc) {
        win_skip("mbrtowc tests\n");
    }else {
        mbstate_t state = 0;
        wchar_t dst;
        expect_eq(p_mbrtowc(&dst, (const char*)mbstring, 2, NULL), 2, int, "%d");
        ok(dst == 0x6c28, "dst = %x, expected 0x6c28\n", dst);
        expect_eq(p_mbrtowc(&dst, (const char*)mbstring+2, 2, NULL), 2, int, "%d");
        ok(dst == 0x3f, "dst = %x, expected 0x3f\n", dst);
        expect_eq(p_mbrtowc(&dst, (const char*)mbstring+3, 2, NULL), 1, int, "%d");
        ok(dst == 0x20, "dst = %x, expected 0x20\n", dst);
        expect_eq(p_mbrtowc(&dst, (const char*)mbstring, 1, NULL), -2, int, "%d");
        ok(dst == 0, "dst = %x, expected 0\n", dst);
        expect_eq(p_mbrtowc(&dst, (const char*)mbstring, 1, &state), -2, int, "%d");
        ok(dst == 0, "dst = %x, expected 0\n", dst);
        ok(state == mbstring[0], "incorrect state value (%x)\n", state);
        expect_eq(p_mbrtowc(&dst, (const char*)mbstring+1, 1, &state), 2, int, "%d");
        ok(dst == 0x6c28, "dst = %x, expected 0x6c28\n", dst);
        ok(state == 0, "incorrect state value (%x)\n", state);
    }
    setlocale(LC_ALL, "C");

    /* _mbccpy/_mbsncpy */
    memset(buf, 0xff, sizeof(buf));
    _mbccpy(buf, mbstring);
    expect_bin(buf, "\xb0\xb1\xff", 3);

    if(!p__mbccpy_s) {
        win_skip("_mbccpy_s tests\n");
    }else {
        int err, copied;

        memset(buf, 0xff, sizeof(buf));
        copied = -1;
        err = p__mbccpy_s(buf, 0, &copied, mbstring);
        ok(err == EINVAL, "_mbccpy_s returned %d\n", err);
        ok(!copied, "copied = %d\n", copied);
        ok(buf[0] == 0xff, "buf[0] = %x\n", buf[0]);

        memset(buf, 0xff, sizeof(buf));
        copied = -1;
        err = p__mbccpy_s(buf, 1, &copied, mbstring);
        ok(err == ERANGE, "_mbccpy_s returned %d\n", err);
        ok(!copied, "copied = %d\n", copied);
        ok(!buf[0], "buf[0] = %x\n", buf[0]);

        memset(buf, 0xff, sizeof(buf));
        copied = -1;
        err = p__mbccpy_s(buf, 2, &copied, mbstring);
        ok(!err, "_mbccpy_s returned %d\n", err);
        ok(copied == 2, "copied = %d\n", copied);
        expect_bin(buf, "\xb0\xb1\xff", 3);

        memset(buf, 0xff, sizeof(buf));
        copied = -1;
        err = p__mbccpy_s(buf, 2, &copied, (unsigned char *)"\xb0");
        ok(err == EILSEQ, "_mbccpy_s returned %d\n", err);
        ok(copied == 1, "copied = %d\n", copied);
        expect_bin(buf, "\x00\xff", 2);
    }

    skip_2k3_crash {
    errno = 0xdeadbeef;
    ret = _mbsncpy(NULL, mbstring, 1);
    ok(ret == NULL, "_mbsncpy returned %p, expected NULL\n", ret);
    ok(errno == EINVAL, "_mbsncpy returned %d\n", errno);

    memset(buf, 0xff, sizeof(buf));
    errno = 0xdeadbeef;
    ret = _mbsncpy(buf, NULL, 1);
    ok(ret == NULL, "_mbsncpy returned %p, expected NULL\n", ret);
    ok(errno == EINVAL, "_mbsncpy returned %d\n", errno);
    expect_bin(buf, "\xff\xff\xff", 3);
    }

    errno = 0xdeadbeef;
    ret = _mbsncpy(NULL, mbstring, 0);
    ok(ret == NULL, "_mbsncpy returned %p, expected NULL\n", ret);
    ok(errno == 0xdeadbeef, "_mbsncpy should not change errno\n");

    memset(buf, 0xff, sizeof(buf));
    errno = 0xdeadbeef;
    ret = _mbsncpy(buf, NULL, 0);
    ok(ret == buf, "_mbsncpy returned %p, expected %sp\n", ret, buf);
    ok(errno == 0xdeadbeef, "_mbsncpy should not change errno\n");

    skip_2k3_crash {
    memset(buf, 0xff, sizeof(buf));
    errno = 0xdeadbeef;
    ret = _mbsncpy(NULL, mbstring, 1);
    ok(ret == NULL, "_mbsncpy returned %p, expected NULL\n", ret);
    ok(errno == EINVAL, "_mbsncpy returned %d\n", errno);

    memset(buf, 0xff, sizeof(buf));
    errno = 0xdeadbeef;
    ret = _mbsncpy(buf, NULL, 1);
    ok(ret == NULL, "_mbsncpy returned %p, expected NULL\n", ret);
    ok(errno == EINVAL, "_mbsncpy returned %d\n", errno);
    }

    memset(buf, 0xff, sizeof(buf));
    ret = _mbsncpy(NULL, mbstring, 0);
    ok(ret == NULL, "_mbsncpy returned %p, expected %p\n", ret, buf);

    memset(buf, 0xff, sizeof(buf));
    ret = _mbsncpy(buf, NULL, 0);
    ok(ret == buf, "_mbsncpy returned %p, expected %sp\n", ret, buf);

    memset(buf, 0xff, sizeof(buf));
    ret = _mbsncpy(buf, mbstring, 0);
    ok(ret == buf, "_mbsncpy returned %p, expected %p\n", ret, buf);

    memset(buf, 0xff, sizeof(buf));
    _mbsncpy(buf, mbstring, 1);
    expect_bin(buf, "\xb0\xb1\xff", 3);
    memset(buf, 0xff, sizeof(buf));
    _mbsncpy(buf, mbstring, 2);
    expect_bin(buf, "\xb0\xb1\xb2 \xff", 5);
    memset(buf, 0xff, sizeof(buf));
    _mbsncpy(buf, mbstring, 3);
    expect_bin(buf, "\xb0\xb1\xb2 \xb3\xb4\xff", 7);
    memset(buf, 0xff, sizeof(buf));
    _mbsncpy(buf, mbstring, 4);
    expect_bin(buf, "\xb0\xb1\xb2 \xb3\xb4 \xff", 8);
    memset(buf, 0xff, sizeof(buf));
    _mbsncpy(buf, mbstring, 5);
    expect_bin(buf, "\xb0\xb1\xb2 \xb3\xb4 \0\0\xff", 10);
    memset(buf, 0xff, sizeof(buf));
    _mbsncpy(buf, mbsonlylead, 6);
    expect_bin(buf, "\0\0\0\0\0\0\0\xff", 8);

    memset(buf, 0xff, sizeof(buf));
    _mbsnbcpy(buf, mbstring2, 2);
    expect_bin(buf, "\xb0\xb1\xff", 3);
    _mbsnbcpy(buf, mbstring2, 3);
    expect_bin(buf, "\xb0\xb1\0\xff", 4);
    _mbsnbcpy(buf, mbstring2, 4);
    expect_bin(buf, "\xb0\xb1\xb2\xb3\xff", 5);
    memset(buf, 0xff, sizeof(buf));
    _mbsnbcpy(buf, mbsonlylead, 5);
    expect_bin(buf, "\0\0\0\0\0\xff", 6);

    /* _mbsinc/mbsdec */
    step = _mbsinc(mbstring) - mbstring;
    ok(step == 2, "_mbsinc adds %d (exp. 2)\n", step);
    step = _mbsinc(&mbstring[2]) - &mbstring[2];  /* lead + invalid tail */
    ok(step == 2, "_mbsinc adds %d (exp. 2)\n", step);

    step = _mbsninc(mbsonlylead, 1) - mbsonlylead;
    ok(step == 0, "_mbsninc adds %d (exp. 0)\n", step);
    step = _mbsninc(mbsonlylead, 2) - mbsonlylead;  /* lead + NUL byte + lead + char */
    ok(step == 0, "_mbsninc adds %d (exp. 0)\n", step);
    step = _mbsninc(mbstring2, 0) - mbstring2;
    ok(step == 0, "_mbsninc adds %d (exp. 2)\n", step);
    step = _mbsninc(mbstring2, 1) - mbstring2;
    ok(step == 2, "_mbsninc adds %d (exp. 2)\n", step);
    step = _mbsninc(mbstring2, 2) - mbstring2;
    ok(step == 4, "_mbsninc adds %d (exp. 4)\n", step);
    step = _mbsninc(mbstring2, 3) - mbstring2;
    ok(step == 5, "_mbsninc adds %d (exp. 5)\n", step);
    step = _mbsninc(mbstring2, 4) - mbstring2;
    ok(step == 7, "_mbsninc adds %d (exp. 7)\n", step);
    step = _mbsninc(mbstring2, 5) - mbstring2;
    ok(step == 7, "_mbsninc adds %d (exp. 7)\n", step);
    step = _mbsninc(mbstring2, 17) - mbstring2;
    ok(step == 7, "_mbsninc adds %d (exp. 7)\n", step);

    /* functions that depend on locale codepage, not mbcp.
     * we hope the current locale to be SBCS because setlocale(LC_ALL, ".1252") seems not to work yet
     * (as of Wine 0.9.43)
     */
    GetCPInfo(GetACP(), &cp_info);
    if (cp_info.MaxCharSize == 1)
    {
        expect_eq(mblen((char *)mbstring, 3), 1, int, "%x");
        expect_eq(_mbstrlen((char *)mbstring2), 7, int, "%d");
    }
    else
        skip("Current locale has double-byte charset - could lead to false positives\n");

    _setmbcp(1361);
    expect_eq(_ismbblead(0x80), 0, int, "%d");
    todo_wine {
      expect_eq(_ismbblead(0x81), 1, int, "%d");
      expect_eq(_ismbblead(0x83), 1, int, "%d");
    }
    expect_eq(_ismbblead(0x84), 1, int, "%d");
    expect_eq(_ismbblead(0xd3), 1, int, "%d");
    expect_eq(_ismbblead(0xd7), 0, int, "%d");
    expect_eq(_ismbblead(0xd8), 1, int, "%d");
    expect_eq(_ismbblead(0xd9), 1, int, "%d");

    expect_eq(_ismbbtrail(0x30), 0, int, "%d");
    expect_eq(_ismbbtrail(0x31), 1, int, "%d");
    expect_eq(_ismbbtrail(0x7e), 1, int, "%d");
    expect_eq(_ismbbtrail(0x7f), 0, int, "%d");
    expect_eq(_ismbbtrail(0x80), 0, int, "%d");
    expect_eq(_ismbbtrail(0x81), 1, int, "%d");
    expect_eq(_ismbbtrail(0xfe), 1, int, "%d");
    expect_eq(_ismbbtrail(0xff), 0, int, "%d");

    _setmbcp(curr_mbcp);
}

static void test_mbsspn( void)
{
    unsigned char str1[]="cabernet";
    unsigned char str2[]="shiraz";
    unsigned char set[]="abc";
    unsigned char empty[]="";
    unsigned char mbstr[]=" 2019\x94\x4e" "6\x8c\x8e" "29\x93\xfa";
    unsigned char mbset1[]="0123456789 \x94\x4e";
    unsigned char mbset2[]=" \x94\x4e\x8c\x8e";
    unsigned char mbset3[]="\x8e";
    int ret, cp = _getmbcp();

    ret=_mbsspn( str1, set);
    ok( ret==3, "_mbsspn returns %d should be 3\n", ret);
    ret=_mbsspn( str2, set);
    ok( ret==0, "_mbsspn returns %d should be 0\n", ret);
    ret=_mbsspn( str1, empty);
    ok( ret==0, "_mbsspn returns %d should be 0\n", ret);

    ret=_mbscspn( str1, set);
    ok( ret==0, "_mbscspn returns %d should be 0\n", ret);
    ret=_mbscspn( str2, set);
    ok( ret==4, "_mbscspn returns %d should be 4\n", ret);
    ret=_mbscspn( str1, empty);
    ok( ret==8, "_mbscspn returns %d should be 8\n", ret);

    _setmbcp( 932);
    ret=_mbsspn( mbstr, mbset1);
    ok( ret==8, "_mbsspn returns %d should be 8\n", ret);
    ret=_mbsspn( mbstr, mbset2);
    ok( ret==1, "_mbsspn returns %d should be 1\n", ret);
    ret=_mbsspn( mbstr+8, mbset1);
    ok( ret==0, "_mbsspn returns %d should be 0\n", ret);
    ret=_mbsspn( mbstr+8, mbset2);
    ok( ret==2, "_mbsspn returns %d should be 2\n", ret);
    ret=_mbsspn( mbstr, mbset3);
    ok( ret==14, "_mbsspn returns %d should be 14\n", ret);

    ret=_mbscspn( mbstr, mbset1);
    ok( ret==0, "_mbscspn returns %d should be 0\n", ret);
    ret=_mbscspn( mbstr, mbset2);
    ok( ret==0, "_mbscspn returns %d should be 0\n", ret);
    ret=_mbscspn( mbstr+8, mbset1);
    ok( ret==2, "_mbscspn returns %d should be 2\n", ret);
    ret=_mbscspn( mbstr+8, mbset2);
    ok( ret==0, "_mbscspn returns %d should be 0\n", ret);
    ret=_mbscspn( mbstr, mbset3);
    ok( ret==0, "_mbscspn returns %d should be 0\n", ret);

    _setmbcp( cp);
}

static void test_mbsspnp( void)
{
    unsigned char str1[]="cabernet";
    unsigned char str2[]="shiraz";
    unsigned char set[]="abc";
    unsigned char empty[]="";
    unsigned char full[]="abcenrt";
    unsigned char mbstr[]=" 2019\x94\x4e" "6\x8c\x8e" "29\x93\xfa";
    unsigned char mbset1[]="0123456789 \x94\x4e";
    unsigned char mbset2[]=" \x94\x4e\x8c\x8e";
    unsigned char* ret;
    int cp = _getmbcp();

    ret=_mbsspnp( str1, set);
    ok( ret[0]=='e', "_mbsspnp returns %c should be e\n", ret[0]);
    ret=_mbsspnp( str2, set);
    ok( ret[0]=='s', "_mbsspnp returns %c should be s\n", ret[0]);
    ret=_mbsspnp( str1, empty);
    ok( ret[0]=='c', "_mbsspnp returns %c should be c\n", ret[0]);
    ret=_mbsspnp( str1, full);
    ok( ret==NULL, "_mbsspnp returns %p should be NULL\n", ret);

    _setmbcp( 932);
    ret=_mbsspnp( mbstr, mbset1);
    ok( ret==mbstr+8, "_mbsspnp returns %p should be %p\n", ret, mbstr+8);
    ret=_mbsspnp( mbstr, mbset2);
    ok( ret==mbstr+1, "_mbsspnp returns %p should be %p\n", ret, mbstr+1);
    ret=_mbsspnp( mbstr+8, mbset1);
    ok( ret==mbstr+8, "_mbsspnp returns %p should be %p\n", ret, mbstr+8);
    ret=_mbsspnp( mbstr+8, mbset2);
    ok( ret==mbstr+10, "_mbsspnp returns %p should be %p\n", ret, mbstr+10);

    _setmbcp( cp);
}

static void test_strdup(void)
{
    char *str;
    errno = 0xdeadbeef;
    str = strdup(0);
    ok(str == 0, "strdup returned %s, expected NULL\n", wine_dbgstr_a(str));
    ok(errno == 0xdeadbeef, "errno is %d, expected 0xdeadbeef\n", errno);
}

static void test_wcsdup(void)
{
    WCHAR *str;
    errno = 0xdeadbeef;
    str = wcsdup(0);
    ok(str == 0, "wcsdup returned %s, expected NULL\n", wine_dbgstr_w(str));
    ok(errno == 0xdeadbeef, "errno is %d, expected 0xdeadbeef\n", errno);
}

static void test_strcmp(void)
{
    int ret = p_strcmp( "abc", "abcd" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "", "abc" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "abc", "ab\xa0" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strcmp( "ab\xb0", "ab\xa0" );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = p_strcmp( "ab\xc2", "ab\xc2" );
    ok( ret == 0, "wrong ret %d\n", ret );

    ret = p_strncmp( "abc", "abcd", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    skip_2k3_fail {
#ifdef _WIN64
    ret = p_strncmp( "", "abc", 3 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "ab\xa0", 4 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 1, "wrong ret %d\n", ret );
#else
    ret = p_strncmp( "", "abc", 3 );
    ok( ret == 0 - 'a', "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "ab\xa0", 4 );
    ok( ret == 'c' - 0xa0, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 0xb0 - 0xa0, "wrong ret %d\n", ret );
#endif
    }
    ret = p_strncmp( "ab\xb0", "ab\xa0", 2 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "ab\xc2", "ab\xc2", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abd", 0 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = p_strncmp( "abc", "abc", 12 );
    ok( ret == 0, "wrong ret %d\n", ret );
}

static void test_strcpy_s(void)
{
    char dest[8];
    const char small[] = "small";
    const char big[] = "atoolongstringforthislittledestination";
    int ret;

    if(!pstrcpy_s)
    {
        win_skip("strcpy_s not found\n");
        return;
    }

    memset(dest, 'X', sizeof(dest));
    ret = pstrcpy_s(dest, sizeof(dest), small);
    ok(ret == 0, "Copying a string into a big enough destination returned %d, expected 0\n", ret);
    ok(dest[0] == 's' && dest[1] == 'm' && dest[2] == 'a' && dest[3] == 'l' &&
       dest[4] == 'l' && dest[5] == '\0'&& dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    memset(dest, 'X', sizeof(dest));
    ret = pstrcpy_s(dest, 0, big);
    ok(ret == EINVAL, "Copying into a destination of size 0 returned %d, expected EINVAL\n", ret);
    ok(dest[0] == 'X' && dest[1] == 'X' && dest[2] == 'X' && dest[3] == 'X' &&
       dest[4] == 'X' && dest[5] == 'X' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
    ret = pstrcpy_s(dest, 0, NULL);
    ok(ret == EINVAL, "Copying into a destination of size 0 returned %d, expected EINVAL\n", ret);
    ok(dest[0] == 'X' && dest[1] == 'X' && dest[2] == 'X' && dest[3] == 'X' &&
       dest[4] == 'X' && dest[5] == 'X' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    memset(dest, 'X', sizeof(dest));
    ret = pstrcpy_s(dest, sizeof(dest), big);
    ok(ret == ERANGE, "Copying a big string in a small location returned %d, expected ERANGE\n", ret);
    ok(dest[0] == '\0'&& dest[1] == 't' && dest[2] == 'o' && dest[3] == 'o' &&
       dest[4] == 'l' && dest[5] == 'o' && dest[6] == 'n' && dest[7] == 'g',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    memset(dest, 'X', sizeof(dest));
    ret = pstrcpy_s(dest, sizeof(dest), NULL);
    ok(ret == EINVAL, "Copying from a NULL source string returned %d, expected EINVAL\n", ret);
    ok(dest[0] == '\0'&& dest[1] == 'X' && dest[2] == 'X' && dest[3] == 'X' &&
       dest[4] == 'X' && dest[5] == 'X' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    ret = pstrcpy_s(NULL, sizeof(dest), small);
    ok(ret == EINVAL, "Copying a big string a NULL dest returned %d, expected EINVAL\n", ret);

    /* strcpy overlapping buffers test */
    memset(dest, 'X', sizeof(dest));
    memcpy(dest+1, small, sizeof(small));
    p_strcpy(dest, dest+1);
    ok(dest[0] == 's' && dest[1] == 'm' && dest[2] == 'a' && dest[3] == 'l' &&
            dest[4] == 'l' && dest[5] == '\0' && dest[6] == '\0' && dest[7] == 'X',
            "Unexpected return data from strcpy: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
            dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    if(!p_strncpy_s)
    {
        win_skip("strncpy_s not found\n");
        return;
    }

    ret = p_strncpy_s(NULL, 18, big, ARRAY_SIZE(big));
    ok(ret == EINVAL, "p_strncpy_s expect EINVAL got %d\n", ret);

    dest[0] = 'A';
    ret = p_strncpy_s(dest, 8, NULL, 1);
    ok(ret == EINVAL, "expected EINVAL got %d\n", ret);
    ok(dest[0] == 0, "dest[0] not 0\n");

    dest[0] = 'A';
    ret = p_strncpy_s(dest, 8, NULL, 0);
    ok(ret == 0, "expected ERROR_SUCCESS got %d\n", ret);
    ok(dest[0] == 0, "dest[0] not 0\n");

    dest[0] = 'A';
    ret = p_strncpy_s(dest, 0, big, ARRAY_SIZE(big));
    ok(ret == ERANGE || ret == EINVAL, "expected ERANGE/EINVAL got %d\n", ret);
    ok(dest[0] == 0 || ret == EINVAL, "dest[0] not 0\n");

    ret = p_strncpy_s(dest, 8, small, ARRAY_SIZE(small));
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(!strcmp(dest, small), "dest != small\n");

    dest[0] = 'A';
    ret = p_strncpy_s(dest, 8, big, ARRAY_SIZE(big));
    ok(ret == ERANGE || ret == EINVAL, "expected ERANGE/EINVAL got %d\n", ret);
    ok(dest[0] == 0, "dest[0] not 0\n");

    dest[0] = 'A';
    ret = p_strncpy_s(dest, 5, big, -1);
    ok(ret == STRUNCATE, "expected STRUNCATE got %d\n", ret);
    ok(dest[4] == 0, "dest[4] not 0\n");
    ok(!memcmp(dest, big, 4), "dest = %s\n", wine_dbgstr_a(dest));

    ret = p_strncpy_s(NULL, 0, (void*)0xdeadbeef, 0);
    ok(ret == 0, "ret = %d\n", ret);

    dest[0] = '1';
    dest[1] = 0;
    ret = p_strncpy_s(dest+1, 4, dest, -1);
    ok(ret == STRUNCATE, "expected ERROR_SUCCESS got %d\n", ret);
    ok(dest[0]=='1' && dest[1]=='1' && dest[2]=='1' && dest[3]=='1',
            "dest = %s\n", wine_dbgstr_a(dest));
}

#define okchars(dst, b0, b1, b2, b3, b4, b5, b6, b7) \
    ok(dst[0] == b0 && dst[1] == b1 && dst[2] == b2 && dst[3] == b3 && \
       dst[4] == b4 && dst[5] == b5 && dst[6] == b6 && dst[7] == b7, \
       "Bad result: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",\
       dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7])

static void test_memcpy_s(void)
{
    static char dest[8], buf[32];
    static const char tiny[] = {'T',0,'I','N','Y',0};
    static const char big[] = {'a','t','o','o','l','o','n','g','s','t','r','i','n','g',0};
    int ret;
    if (!p_memcpy_s) {
        win_skip("memcpy_s not found\n");
        return;
    }

    /* Normal */
    memset(dest, 'X', sizeof(dest));
    ret = p_memcpy_s(dest, ARRAY_SIZE(dest), tiny, ARRAY_SIZE(tiny));
    ok(ret == 0, "Copying a buffer into a big enough destination returned %d, expected 0\n", ret);
    okchars(dest, tiny[0], tiny[1], tiny[2], tiny[3], tiny[4], tiny[5], 'X', 'X');

    /* Vary source size */
    errno = 0xdeadbeef;
    memset(dest, 'X', sizeof(dest));
    ret = p_memcpy_s(dest, ARRAY_SIZE(dest), big, ARRAY_SIZE(big));
    ok(ret == ERANGE, "Copying a big buffer to a small destination returned %d, expected ERANGE\n", ret);
    ok(errno == ERANGE, "errno is %d, expected ERANGE\n", errno);
    okchars(dest, 0, 0, 0, 0, 0, 0, 0, 0);

    /* Replace source with NULL */
    errno = 0xdeadbeef;
    memset(dest, 'X', sizeof(dest));
    ret = p_memcpy_s(dest, ARRAY_SIZE(dest), NULL, ARRAY_SIZE(tiny));
    ok(ret == EINVAL, "Copying a NULL source buffer returned %d, expected EINVAL\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);
    okchars(dest, 0, 0, 0, 0, 0, 0, 0, 0);

    /* Vary dest size */
    errno = 0xdeadbeef;
    memset(dest, 'X', sizeof(dest));
    ret = p_memcpy_s(dest, 0, tiny, ARRAY_SIZE(tiny));
    ok(ret == ERANGE, "Copying into a destination of size 0 returned %d, expected ERANGE\n", ret);
    ok(errno == ERANGE, "errno is %d, expected ERANGE\n", errno);
    okchars(dest, 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X');

    /* Replace dest with NULL */
    errno = 0xdeadbeef;
    ret = p_memcpy_s(NULL, ARRAY_SIZE(dest), tiny, ARRAY_SIZE(tiny));
    ok(ret == EINVAL, "Copying a tiny buffer to a big NULL destination returned %d, expected EINVAL\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);

    /* Combinations */
    errno = 0xdeadbeef;
    memset(dest, 'X', sizeof(dest));
    ret = p_memcpy_s(dest, 0, NULL, ARRAY_SIZE(tiny));
    ok(ret == EINVAL, "Copying a NULL buffer into a destination of size 0 returned %d, expected EINVAL\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);
    okchars(dest, 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X');

    ret = p_memcpy_s(buf, ARRAY_SIZE(buf), big, ARRAY_SIZE(big));
    ok(!ret, "memcpy_s returned %d\n", ret);
    ok(!memcmp(buf, big, sizeof(big)), "unexpected buf\n");

    ret = p_memcpy_s(buf + 1, ARRAY_SIZE(buf) - 1, buf, ARRAY_SIZE(big));
    ok(!ret, "memcpy_s returned %d\n", ret);
    ok(!memcmp(buf + 1, big, sizeof(big)), "unexpected buf\n");

    ret = p_memcpy_s(buf, ARRAY_SIZE(buf), buf + 1, ARRAY_SIZE(big));
    ok(!ret, "memcpy_s returned %d\n", ret);
    ok(!memcmp(buf, big, sizeof(big)), "unexpected buf\n");
}

static void test_memmove_s(void)
{
    static char dest[8];
    static const char tiny[] = {'T',0,'I','N','Y',0};
    static const char big[] = {'a','t','o','o','l','o','n','g','s','t','r','i','n','g',0};
    int ret;
    if (!p_memmove_s) {
        win_skip("memmove_s not found\n");
        return;
    }

    /* Normal */
    memset(dest, 'X', sizeof(dest));
    ret = p_memmove_s(dest, ARRAY_SIZE(dest), tiny, ARRAY_SIZE(tiny));
    ok(ret == 0, "Moving a buffer into a big enough destination returned %d, expected 0\n", ret);
    okchars(dest, tiny[0], tiny[1], tiny[2], tiny[3], tiny[4], tiny[5], 'X', 'X');

    /* Overlapping */
    memcpy(dest, big, sizeof(dest));
    ret = p_memmove_s(dest+1, ARRAY_SIZE(dest)-1, dest, ARRAY_SIZE(dest)-1);
    ok(ret == 0, "Moving a buffer up one char returned %d, expected 0\n", ret);
    okchars(dest, big[0], big[0], big[1], big[2], big[3], big[4], big[5], big[6]);

    /* Vary source size */
    errno = 0xdeadbeef;
    memset(dest, 'X', sizeof(dest));
    ret = p_memmove_s(dest, ARRAY_SIZE(dest), big, ARRAY_SIZE(big));
    ok(ret == ERANGE, "Moving a big buffer to a small destination returned %d, expected ERANGE\n", ret);
    ok(errno == ERANGE, "errno is %d, expected ERANGE\n", errno);
    okchars(dest, 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X');

    /* Replace source with NULL */
    errno = 0xdeadbeef;
    memset(dest, 'X', sizeof(dest));
    ret = p_memmove_s(dest, ARRAY_SIZE(dest), NULL, ARRAY_SIZE(tiny));
    ok(ret == EINVAL, "Moving a NULL source buffer returned %d, expected EINVAL\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);
    okchars(dest, 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X');

    /* Vary dest size */
    errno = 0xdeadbeef;
    memset(dest, 'X', sizeof(dest));
    ret = p_memmove_s(dest, 0, tiny, ARRAY_SIZE(tiny));
    ok(ret == ERANGE, "Moving into a destination of size 0 returned %d, expected ERANGE\n", ret);
    ok(errno == ERANGE, "errno is %d, expected ERANGE\n", errno);
    okchars(dest, 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X');

    /* Replace dest with NULL */
    errno = 0xdeadbeef;
    ret = p_memmove_s(NULL, ARRAY_SIZE(dest), tiny, ARRAY_SIZE(tiny));
    ok(ret == EINVAL, "Moving a tiny buffer to a big NULL destination returned %d, expected EINVAL\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);

    /* Combinations */
    errno = 0xdeadbeef;
    memset(dest, 'X', sizeof(dest));
    ret = p_memmove_s(dest, 0, NULL, ARRAY_SIZE(tiny));
    ok(ret == EINVAL, "Moving a NULL buffer into a destination of size 0 returned %d, expected EINVAL\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);
    okchars(dest, 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X');
}

static void test_strcat_s(void)
{
    char dest[8];
    const char *small = "sma";
    int ret;

    if(!pstrcat_s)
    {
        win_skip("strcat_s not found\n");
        return;
    }

    memset(dest, 'X', sizeof(dest));
    dest[0] = '\0';
    ret = pstrcat_s(dest, sizeof(dest), small);
    ok(ret == 0, "strcat_s: Copying a string into a big enough destination returned %d, expected 0\n", ret);
    ok(dest[0] == 's' && dest[1] == 'm' && dest[2] == 'a' && dest[3] == '\0'&&
       dest[4] == 'X' && dest[5] == 'X' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
    ret = pstrcat_s(dest, sizeof(dest), small);
    ok(ret == 0, "strcat_s: Attaching a string to a big enough destination returned %d, expected 0\n", ret);
    ok(dest[0] == 's' && dest[1] == 'm' && dest[2] == 'a' && dest[3] == 's' &&
       dest[4] == 'm' && dest[5] == 'a' && dest[6] == '\0'&& dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    ret = pstrcat_s(dest, sizeof(dest), small);
    ok(ret == ERANGE, "strcat_s: Attaching a string to a filled up destination returned %d, expected ERANGE\n", ret);
    ok(dest[0] == '\0'&& dest[1] == 'm' && dest[2] == 'a' && dest[3] == 's' &&
       dest[4] == 'm' && dest[5] == 'a' && dest[6] == 's' && dest[7] == 'm',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    memset(dest, 'X', sizeof(dest));
    dest[0] = 'a';
    dest[1] = '\0';

    ret = pstrcat_s(dest, 0, small);
    ok(ret == EINVAL, "strcat_s: Source len = 0 returned %d, expected EINVAL\n", ret);
    ok(dest[0] == 'a' && dest[1] == '\0'&& dest[2] == 'X' && dest[3] == 'X' &&
       dest[4] == 'X' && dest[5] == 'X' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    ret = pstrcat_s(dest, 0, NULL);
    ok(ret == EINVAL, "strcat_s: len = 0 and src = NULL returned %d, expected EINVAL\n", ret);
    ok(dest[0] == 'a' && dest[1] == '\0'&& dest[2] == 'X' && dest[3] == 'X' &&
       dest[4] == 'X' && dest[5] == 'X' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    ret = pstrcat_s(dest, sizeof(dest), NULL);
    ok(ret == EINVAL, "strcat_s:  Sourcing from NULL returned %d, expected EINVAL\n", ret);
    ok(dest[0] == '\0'&& dest[1] == '\0'&& dest[2] == 'X' && dest[3] == 'X' &&
       dest[4] == 'X' && dest[5] == 'X' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from strcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    ret = pstrcat_s(NULL, sizeof(dest), small);
    ok(ret == EINVAL, "strcat_s: Writing to a NULL string returned %d, expected EINVAL\n", ret);
}

static void test__mbscat_s(void)
{
    unsigned char dst[8], src[4];
    int err;
    int prev_cp = _getmbcp();

    if(!p_mbscat_s)
    {
        win_skip("_mbscat_s not found\n");
        return;
    }


    src[0] = dst[0] = 0;
    err = p_mbscat_s(NULL, sizeof(dst), src);
    ok(err == EINVAL, "_mbscat_s returned %d\n", err);

    err = p_mbscat_s(dst, sizeof(dst), NULL);
    ok(err == EINVAL, "_mbscat_s returned %d\n", err);

    dst[0] = 'a';
    err = p_mbscat_s(dst, 1, src);
    ok(err == EINVAL, "_mbscat_s returned %d\n", err);

    memset(dst, 'a', sizeof(dst));
    dst[6] = 0;
    src[0] = 'b';
    src[1] = 0;

    err = p_mbscat_s(dst, sizeof(dst), src);
    ok(err == 0, "_mbscat_s returned %d\n", err);
    ok(!memcmp(dst, "aaaaaab", 8), "dst = %s\n", dst);

    err = p_mbscat_s(dst, sizeof(dst), src);
    ok(err == ERANGE, "_mbscat_s returned %d\n", err);
    ok(!dst[0], "dst[0] = %c\n", dst[0]);
    ok(dst[1] == 'a', "dst[1] = %c\n", dst[1]);

    _setmbcp(932);
    /* test invalid str in dst */
    dst[0] = 0x81;
    dst[1] = 0x81;
    dst[2] = 0x52;
    dst[3] = 0;
    src[0] = 'a';
    src[1] = 0;
    err = p_mbscat_s(dst, sizeof(dst), src);
    ok(err == 0, "_mbscat_s returned %d\n", err);

    /* test invalid str in src */
    dst[0] = 0;
    src[0] = 0x81;
    src[1] = 0x81;
    src[2] = 0x52;
    src[3] = 0;
    err = p_mbscat_s(dst, sizeof(dst), src);
    ok(err == 0, "_mbscat_s returned %d\n", err);

    /* test dst with leading byte on the end of buffer */
    dst[0] = 'a';
    dst[1] = 0x81;
    dst[2] = 0;
    src[0] = 'R';
    src[1] = 0;
    err = p_mbscat_s(dst, sizeof(dst), src);
    ok(err == EILSEQ, "_mbscat_s returned %d\n", err);
    ok(!memcmp(dst, "aR", 3), "dst = %s\n", dst);

    /* test src with leading byte on the end of buffer */
    dst[0] = 'a';
    dst[1] = 0;
    src[0] = 'b';
    src[1] = 0x81;
    src[2] = 0;
    err = p_mbscat_s(dst, sizeof(dst), src);
    ok(err == EILSEQ, "_mbscat_s returned %d\n", err);
    ok(!memcmp(dst, "ab", 3), "dst = %s\n", dst);
    _setmbcp(prev_cp);
}

static void test__mbsnbcpy_s(void)
{
    unsigned char dest[8];
    const unsigned char big[] = "atoolongstringforthislittledestination";
    const unsigned char small[] = "small";
    int ret;

    if(!p_mbsnbcpy_s)
    {
        win_skip("_mbsnbcpy_s not found\n");
        return;
    }

    memset(dest, 'X', sizeof(dest));
    ret = p_mbsnbcpy_s(dest, sizeof(dest), small, sizeof(small));
    ok(ret == 0, "_mbsnbcpy_s: Copying a string into a big enough destination returned %d, expected 0\n", ret);
    ok(dest[0] == 's' && dest[1] == 'm' && dest[2] == 'a' && dest[3] == 'l' &&
       dest[4] == 'l' && dest[5] == '\0'&& dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from _mbsnbcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    /* WTF? */
    memset(dest, 'X', sizeof(dest));
    ret = p_mbsnbcpy_s(dest, sizeof(dest) - 2, big, sizeof(small));
    ok(ret == ERANGE, "_mbsnbcpy_s: Copying a too long string returned %d, expected ERANGE\n", ret);
    ok(dest[0] == '\0'&& dest[1] == 't' && dest[2] == 'o' && dest[3] == 'o' &&
       dest[4] == 'l' && dest[5] == 'o' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from _mbsnbcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    memset(dest, 'X', sizeof(dest));
    ret = p_mbsnbcpy_s(dest, sizeof(dest) - 2, big, 4);
    ok(ret == 0, "_mbsnbcpy_s: Copying a too long string with a count cap returned %d, expected 0\n", ret);
    ok(dest[0] == 'a' && dest[1] == 't' && dest[2] == 'o' && dest[3] == 'o' &&
       dest[4] == '\0'&& dest[5] == 'X' && dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from _mbsnbcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);

    memset(dest, 'X', sizeof(dest));
    ret = p_mbsnbcpy_s(dest, sizeof(dest) - 2, small, sizeof(small) + 10);
    ok(ret == 0, "_mbsnbcpy_s: Copying more data than the source string len returned %d, expected 0\n", ret);
    ok(dest[0] == 's' && dest[1] == 'm' && dest[2] == 'a' && dest[3] == 'l' &&
       dest[4] == 'l' && dest[5] == '\0'&& dest[6] == 'X' && dest[7] == 'X',
       "Unexpected return data from _mbsnbcpy_s: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
       dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

static void test__mbscpy_s(void)
{
    const unsigned char src[] = "source string";
    unsigned char dest[16];
    int ret;

    if(!p__mbscpy_s)
    {
        win_skip("_mbscpy_s not found\n");
        return;
    }

    ret = p__mbscpy_s(NULL, 0, src);
    ok(ret == EINVAL, "got %d\n", ret);
    ret = p__mbscpy_s(NULL, sizeof(dest), src);
    ok(ret == EINVAL, "got %d\n", ret);
    ret = p__mbscpy_s(dest, 0, src);
    ok(ret == EINVAL, "got %d\n", ret);
    dest[0] = 'x';
    ret = p__mbscpy_s(dest, sizeof(dest), NULL);
    ok(ret == EINVAL, "got %d\n", ret);
    ok(!dest[0], "dest buffer was not modified on invalid argument\n");

    memset(dest, 'X', sizeof(dest));
    ret = p__mbscpy_s(dest, sizeof(dest), src);
    ok(!ret, "got %d\n", ret);
    ok(!memcmp(dest, src, sizeof(src)), "dest = %s\n", dest);
    ok(dest[sizeof(src)] == 'X', "unused part of buffer was modified\n");

    memset(dest, 'X', sizeof(dest));
    ret = p__mbscpy_s(dest, 4, src);
    ok(ret == ERANGE, "got %d\n", ret);
    ok(!dest[0], "incorrect dest buffer (%d)\n", dest[0]);
    ok(dest[1] == src[1], "incorrect dest buffer (%d)\n", dest[1]);
}

static void test_wcscpy_s(void)
{
    static const WCHAR szLongText[] = L"ThisALongstring";
    static WCHAR szDest[18];
    static WCHAR szDestShort[8];
    int ret;

    if(!p_wcscpy_s)
    {
        win_skip("wcscpy_s not found\n");
        return;
    }

    /* Test NULL Dest */
    errno = EBADF;
    ret = p_wcscpy_s(NULL, 18, szLongText);
    ok(ret == EINVAL, "p_wcscpy_s expect EINVAL got %d\n", ret);
    ok(errno == EINVAL, "expected errno EINVAL got %d\n", errno);

    /* Test NULL Source */
    errno = EBADF;
    szDest[0] = 'A';
    ret = p_wcscpy_s(szDest, 18, NULL);
    ok(ret == EINVAL, "expected EINVAL got %d\n", ret);
    ok(errno == EINVAL, "expected errno EINVAL got %d\n", errno);
    ok(szDest[0] == 0, "szDest[0] not 0, got %c\n", szDest[0]);

    /* Test invalid size */
    errno = EBADF;
    szDest[0] = 'A';
    ret = p_wcscpy_s(szDest, 0, szLongText);
    /* Later versions changed the return value for this case to EINVAL,
     * and don't modify the result if the dest size is 0.
     */
    ok(ret == ERANGE || ret == EINVAL, "expected ERANGE/EINVAL got %d\n", ret);
    ok(errno == ERANGE || errno == EINVAL, "expected errno ERANGE/EINVAL got %d\n", errno);
    ok(szDest[0] == 0 || ret == EINVAL, "szDest[0] not 0\n");

    /* Copy same buffer size */
    ret = p_wcscpy_s(szDest, 18, szLongText);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(lstrcmpW(szDest, szLongText) == 0, "szDest != szLongText\n");

    /* dest == source */
    ret = p_wcscpy_s(szDest, 18, szDest);
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(lstrcmpW(szDest, szLongText) == 0, "szDest != szLongText\n");

    /* Copy smaller buffer size */
    errno = EBADF;
    szDest[0] = 'A';
    ret = p_wcscpy_s(szDestShort, 8, szLongText);
    ok(ret == ERANGE || ret == EINVAL, "expected ERANGE/EINVAL got %d\n", ret);
    ok(errno == ERANGE || errno == EINVAL, "expected errno ERANGE/EINVAL got %d\n", errno);
    ok(szDestShort[0] == 0, "szDestShort[0] not 0\n");

    if(!p_wcsncpy_s)
    {
        win_skip("wcsncpy_s not found\n");
        return;
    }

    ret = p_wcsncpy_s(NULL, 18, szLongText, ARRAY_SIZE(szLongText));
    ok(ret == EINVAL, "p_wcsncpy_s expect EINVAL got %d\n", ret);

    szDest[0] = 'A';
    ret = p_wcsncpy_s(szDest, 18, NULL, 1);
    ok(ret == EINVAL, "expected EINVAL got %d\n", ret);
    ok(szDest[0] == 0, "szDest[0] not 0\n");

    szDest[0] = 'A';
    ret = p_wcsncpy_s(szDest, 18, NULL, 0);
    ok(ret == 0, "expected ERROR_SUCCESS got %d\n", ret);
    ok(szDest[0] == 0, "szDest[0] not 0\n");

    szDest[0] = 'A';
    ret = p_wcsncpy_s(szDest, 0, szLongText, ARRAY_SIZE(szLongText));
    ok(ret == ERANGE || ret == EINVAL, "expected ERANGE/EINVAL got %d\n", ret);
    ok(szDest[0] == 0 || ret == EINVAL, "szDest[0] not 0\n");

    ret = p_wcsncpy_s(szDest, 18, szLongText, ARRAY_SIZE(szLongText));
    ok(ret == 0, "expected 0 got %d\n", ret);
    ok(lstrcmpW(szDest, szLongText) == 0, "szDest != szLongText\n");

    szDest[0] = 'A';
    ret = p_wcsncpy_s(szDestShort, 8, szLongText, ARRAY_SIZE(szLongText));
    ok(ret == ERANGE || ret == EINVAL, "expected ERANGE/EINVAL got %d\n", ret);
    ok(szDestShort[0] == 0, "szDestShort[0] not 0\n");

    szDest[0] = 'A';
    ret = p_wcsncpy_s(szDest, 5, szLongText, -1);
    ok(ret == STRUNCATE, "expected STRUNCATE got %d\n", ret);
    ok(szDest[4] == 0, "szDest[4] not 0\n");
    ok(!memcmp(szDest, szLongText, 4*sizeof(WCHAR)), "szDest = %s\n", wine_dbgstr_w(szDest));

    ret = p_wcsncpy_s(NULL, 0, (void*)0xdeadbeef, 0);
    ok(ret == 0, "ret = %d\n", ret);

    szDestShort[0] = '1';
    szDestShort[1] = 0;
    ret = p_wcsncpy_s(szDestShort+1, 4, szDestShort, -1);
    ok(ret == STRUNCATE, "expected ERROR_SUCCESS got %d\n", ret);
    ok(szDestShort[0]=='1' && szDestShort[1]=='1' && szDestShort[2]=='1' && szDestShort[3]=='1',
            "szDestShort = %s\n", wine_dbgstr_w(szDestShort));
}

static void test__wcsupr_s(void)
{
    static const WCHAR mixedString[] = L"MiXeDlowerUPPER";
    static const WCHAR expectedString[] = L"MIXEDLOWERUPPER";
    WCHAR testBuffer[2*ARRAY_SIZE(mixedString)];
    int ret;

    if (!p_wcsupr_s)
    {
        win_skip("_wcsupr_s not found\n");
        return;
    }

    /* Test NULL input string and invalid size. */
    errno = EBADF;
    ret = p_wcsupr_s(NULL, 0);
    ok(ret == EINVAL, "Expected _wcsupr_s to fail with EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    /* Test NULL input string and valid size. */
    errno = EBADF;
    ret = p_wcsupr_s(NULL, ARRAY_SIZE(testBuffer));
    ok(ret == EINVAL, "Expected _wcsupr_s to fail with EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    /* Test empty string with zero size. */
    errno = EBADF;
    testBuffer[0] = '\0';
    ret = p_wcsupr_s(testBuffer, 0);
    ok(ret == EINVAL, "Expected _wcsupr_s to fail with EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(testBuffer[0] == '\0', "Expected the buffer to be unchanged\n");

    /* Test empty string with size of one. */
    testBuffer[0] = '\0';
    ret = p_wcsupr_s(testBuffer, 1);
    ok(ret == 0, "Expected _wcsupr_s to succeed, got %d\n", ret);
    ok(testBuffer[0] == '\0', "Expected the buffer to be unchanged\n");

    /* Test one-byte buffer with zero size. */
    errno = EBADF;
    testBuffer[0] = 'x';
    ret = p_wcsupr_s(testBuffer, 0);
    ok(ret == EINVAL, "Expected _wcsupr_s to fail with EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(testBuffer[0] == '\0', "Expected the first buffer character to be null\n");

    /* Test one-byte buffer with size of one. */
    errno = EBADF;
    testBuffer[0] = 'x';
    ret = p_wcsupr_s(testBuffer, 1);
    ok(ret == EINVAL, "Expected _wcsupr_s to fail with EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(testBuffer[0] == '\0', "Expected the first buffer character to be null\n");

    /* Test invalid size. */
    wcscpy(testBuffer, mixedString);
    errno = EBADF;
    ret = p_wcsupr_s(testBuffer, 0);
    ok(ret == EINVAL, "Expected _wcsupr_s to fail with EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(testBuffer[0] == '\0', "Expected the first buffer character to be null\n");

    /* Test normal string uppercasing. */
    wcscpy(testBuffer, mixedString);
    ret = p_wcsupr_s(testBuffer, ARRAY_SIZE(mixedString));
    ok(ret == 0, "Expected _wcsupr_s to succeed, got %d\n", ret);
    ok(!wcscmp(testBuffer, expectedString), "Expected the string to be fully upper-case\n");

    /* Test uppercasing with a shorter buffer size count. */
    wcscpy(testBuffer, mixedString);
    errno = EBADF;
    ret = p_wcsupr_s(testBuffer, ARRAY_SIZE(mixedString) - 1);
    ok(ret == EINVAL, "Expected _wcsupr_s to fail with EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(testBuffer[0] == '\0', "Expected the first buffer character to be null\n");

    /* Test uppercasing with a longer buffer size count. */
    wcscpy(testBuffer, mixedString);
    ret = p_wcsupr_s(testBuffer, ARRAY_SIZE(testBuffer));
    ok(ret == 0, "Expected _wcsupr_s to succeed, got %d\n", ret);
    ok(!wcscmp(testBuffer, expectedString), "Expected the string to be fully upper-case\n");
}

static void test__wcslwr_s(void)
{
    static const WCHAR mixedString[] = L"MiXeDlowerUPPER";
    static const WCHAR expectedString[] = L"mixedlowerupper";
    WCHAR buffer[2*ARRAY_SIZE(mixedString)];
    int ret;

    if (!p_wcslwr_s)
    {
        win_skip("_wcslwr_s not found\n");
        return;
    }

    /* Test NULL input string and invalid size. */
    errno = EBADF;
    ret = p_wcslwr_s(NULL, 0);
    ok(ret == EINVAL, "expected EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "expected errno EINVAL, got %d\n", errno);

    /* Test NULL input string and valid size. */
    errno = EBADF;
    ret = p_wcslwr_s(NULL, ARRAY_SIZE(buffer));
    ok(ret == EINVAL, "expected EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "expected errno EINVAL, got %d\n", errno);

    /* Test empty string with zero size. */
    errno = EBADF;
    buffer[0] = 'a';
    ret = p_wcslwr_s(buffer, 0);
    ok(ret == EINVAL, "expected EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "expected errno EINVAL, got %d\n", errno);
    ok(buffer[0] == 0, "expected empty string\n");

    /* Test empty string with size of one. */
    buffer[0] = 0;
    ret = p_wcslwr_s(buffer, 1);
    ok(ret == 0, "got %d\n", ret);
    ok(buffer[0] == 0, "expected buffer to be unchanged\n");

    /* Test one-byte buffer with zero size. */
    errno = EBADF;
    buffer[0] = 'x';
    ret = p_wcslwr_s(buffer, 0);
    ok(ret == EINVAL, "expected EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == '\0', "expected empty string\n");

    /* Test one-byte buffer with size of one. */
    errno = EBADF;
    buffer[0] = 'x';
    ret = p_wcslwr_s(buffer, 1);
    ok(ret == EINVAL, "expected EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == '\0', "expected empty string\n");

    /* Test invalid size. */
    wcscpy(buffer, mixedString);
    errno = EBADF;
    ret = p_wcslwr_s(buffer, 0);
    ok(ret == EINVAL, "Expected EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == '\0', "expected empty string\n");

    /* Test normal string uppercasing. */
    wcscpy(buffer, mixedString);
    ret = p_wcslwr_s(buffer, ARRAY_SIZE(mixedString));
    ok(ret == 0, "expected 0, got %d\n", ret);
    ok(!wcscmp(buffer, expectedString), "expected lowercase\n");

    /* Test uppercasing with a shorter buffer size count. */
    wcscpy(buffer, mixedString);
    errno = EBADF;
    ret = p_wcslwr_s(buffer, ARRAY_SIZE(mixedString) - 1);
    ok(ret == EINVAL, "expected EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == '\0', "expected empty string\n");

    /* Test uppercasing with a longer buffer size count. */
    wcscpy(buffer, mixedString);
    ret = p_wcslwr_s(buffer, ARRAY_SIZE(buffer));
    ok(ret == 0, "expected 0, got %d\n", ret);
    ok(!wcscmp(buffer, expectedString), "expected lowercase\n");
}

static void test_mbcjisjms(void)
{
    /* List of value-pairs to test. The test assumes the last pair to be {0, ..} */
    unsigned int jisjms[][2] = { {0x2020, 0}, {0x2021, 0}, {0x2120, 0}, {0x2121, 0x8140},
                                 {0x7f7f, 0}, {0x7f7e, 0}, {0x7e7f, 0}, {0x7e7e, 0xeffc},
                                 {0x255f, 0x837e}, {0x2560, 0x8380}, {0x2561, 0x8381},
                                 {0x2121FFFF, 0}, {0x2223, 0x81a1}, {0x237e, 0x829e}, {0, 0}};
    int cp[] = { 932, 936, 939, 950, 1361, _MB_CP_SBCS };
    unsigned int i, j;
    int prev_cp = _getmbcp();

    for (i = 0; i < ARRAY_SIZE(cp); i++)
    {
        _setmbcp(cp[i]);
        for (j = 0; jisjms[j][0] != 0; j++)
        {
            unsigned int ret, exp;
            ret = _mbcjistojms(jisjms[j][0]);
            exp = (cp[i] == 932) ? jisjms[j][1] : jisjms[j][0];
            ok(ret == exp, "Expected 0x%x, got 0x%x (0x%x, codepage=%d)\n",
               exp, ret, jisjms[j][0], cp[i]);
        }
    }
    _setmbcp(prev_cp);
}

static void test_mbcjmsjis(void)
{
    /* List of value-pairs to test. The test assumes the last pair to be {0, ..} */
    unsigned int jmsjis[][2] = { {0x80fc, 0}, {0x813f, 0}, {0x8140, 0x2121},
                                 {0x817e, 0x215f}, {0x817f, 0}, {0x8180, 0x2160},
                                 {0x819e, 0x217e}, {0x819f, 0x2221}, {0x81fc, 0x227e},
                                 {0x81fd, 0}, {0x9ffc, 0x5e7e}, {0x9ffd, 0},
                                 {0xa040, 0}, {0xdffc, 0}, {0xe040, 0x5f21},
                                 {0xeffc, 0x7e7e}, {0xf040, 0}, {0x21, 0}, {0, 0}};
    int cp[] = { 932, 936, 939, 950, 1361, _MB_CP_SBCS };
    unsigned int i, j;
    int prev_cp = _getmbcp();

    for (i = 0; i < ARRAY_SIZE(cp); i++)
    {
        _setmbcp(cp[i]);
        for (j = 0; jmsjis[j][0] != 0; j++)
        {
            unsigned int ret, exp;
            ret = _mbcjmstojis(jmsjis[j][0]);
            exp = (cp[i] == 932) ? jmsjis[j][1] : jmsjis[j][0];
            ok(ret == exp, "Expected 0x%x, got 0x%x (0x%x, codepage=%d)\n",
               exp, ret, jmsjis[j][0], cp[i]);
        }
    }
    _setmbcp(prev_cp);
}

static void test_mbctohira(void)
{
    static const unsigned int mbchira_932[][2] = {
        {0x8152, 0x8152}, {0x8153, 0x8153}, {0x8154, 0x8154}, {0x8155, 0x8155},
        {0x82a0, 0x82a0}, {0x833f, 0x833f}, {0x8340, 0x829f}, {0x837e, 0x82dd},
        {0x837f, 0x837f}, {0x8380, 0x82de}, {0x8393, 0x82f1}, {0x8394, 0x8394},
        {0x8396, 0x8396}, {0x8397, 0x8397},
        {0xa5, 0xa5}, {0xb0, 0xb0}, {0xdd, 0xdd} };
    unsigned int i;
    unsigned int prev_cp = _getmbcp();

    _setmbcp(_MB_CP_SBCS);
    for (i = 0; i < ARRAY_SIZE(mbchira_932); i++)
    {
        int ret, exp = mbchira_932[i][0];
        ret = _mbctohira(mbchira_932[i][0]);
        ok(ret == exp, "Expected 0x%x, got 0x%x\n", exp, ret);
    }

    _setmbcp(932);
    for (i = 0; i < ARRAY_SIZE(mbchira_932); i++)
    {
        unsigned int ret, exp;
        ret = _mbctohira(mbchira_932[i][0]);
        exp = mbchira_932[i][1];
        ok(ret == exp, "Expected 0x%x, got 0x%x\n", exp, ret);
    }
    _setmbcp(prev_cp);
}

static void test_mbctokata(void)
{
    static const unsigned int mbckata_932[][2] = {
        {0x8152, 0x8152}, {0x8153, 0x8153}, {0x8154, 0x8154}, {0x8155, 0x8155},
        {0x833f, 0x833f}, {0x829f, 0x8340}, {0x82dd, 0x837e}, {0x837f, 0x837f},
        {0x82de, 0x8380}, {0x8394, 0x8394}, {0x8397, 0x8397},
        {0xa5, 0xa5}, {0xb0, 0xb0}, {0xdd, 0xdd} };
    unsigned int i;
    unsigned int prev_cp = _getmbcp();

    _setmbcp(_MB_CP_SBCS);
    for (i = 0; i < ARRAY_SIZE(mbckata_932); i++)
    {
        int ret, exp = mbckata_932[i][0];
        ret = _mbctokata(mbckata_932[i][0]);
        ok(ret == exp, "Expected 0x%x, got 0x%x\n", exp, ret);
    }

    _setmbcp(932);
    for (i = 0; i < ARRAY_SIZE(mbckata_932); i++)
    {
        unsigned int ret, exp;
        ret = _mbctokata(mbckata_932[i][0]);
        exp = mbckata_932[i][1];
        ok(ret == exp, "Expected 0x%x, got 0x%x\n", exp, ret);
    }
    _setmbcp(prev_cp);
}

static void test_mbbtombc(void)
{
    static const unsigned int mbbmbc[][2] = {
        {0x1f, 0x1f}, {0x20, 0x8140}, {0x39, 0x8258}, {0x40, 0x8197},
        {0x41, 0x8260}, {0x5e, 0x814f}, {0x7e, 0x8150}, {0x7f, 0x7f},
        {0x80, 0x80}, {0x81, 0x81}, {0xa0, 0xa0}, {0xa7, 0x8340},
        {0xb0, 0x815b}, {0xd1, 0x8380}, {0xff, 0xff}, {0,0}};
    int cp[] = { 932, 936, 939, 950, 1361, _MB_CP_SBCS };
    int i, j;
    int prev_cp = _getmbcp();

    for (i = 0; i < ARRAY_SIZE(cp); i++)
    {
        _setmbcp(cp[i]);
        for (j = 0; mbbmbc[j][0] != 0; j++)
        {
            unsigned int exp, ret;
            ret = _mbbtombc(mbbmbc[j][0]);
            exp = (cp[i] == 932) ? mbbmbc[j][1] : mbbmbc[j][0];
            ok(ret == exp, "Expected 0x%x, got 0x%x (0x%x, codepage %d)\n",
               exp, ret, mbbmbc[j][0], cp[i]);
        }
    }
    _setmbcp(prev_cp);
}

static void test_mbctombb(void)
{
    static const unsigned int mbcmbb_932[][2] = {
        {0x829e, 0x829e}, {0x829f, 0xa7}, {0x82f1, 0xdd}, {0x82f2, 0x82f2},
        {0x833f, 0x833f}, {0x8340, 0xa7}, {0x837e, 0xd0}, {0x837f, 0x837f},
        {0x8380, 0xd1}, {0x8396, 0xb9}, {0x8397, 0x8397}, {0x813f, 0x813f},
        {0x8140, 0x20}, {0x814c, 0x814c}, {0x814f, 0x5e}, {0x8197, 0x40},
        {0x8198, 0x8198}, {0x8258, 0x39}, {0x8259, 0x8259}, {0x825f, 0x825f},
        {0x8260, 0x41}, {0x82f1, 0xdd}, {0x82f2, 0x82f2}, {0,0}};
    unsigned int exp, ret, i;
    unsigned int prev_cp = _getmbcp();

    _setmbcp(932);
    for (i = 0; mbcmbb_932[i][0] != 0; i++)
    {
        ret = _mbctombb(mbcmbb_932[i][0]);
        exp = mbcmbb_932[i][1];
        ok(ret == exp, "Expected 0x%x, got 0x%x\n", exp, ret);
    }
    _setmbcp(prev_cp);
}

static void test_ismbckata(void) {
    struct katakana_pair {
        UINT c;
        BOOL exp;
    };
    static const struct katakana_pair tests[] = {
        {0x8152, FALSE}, {0x8153, FALSE}, {0x8154, FALSE}, {0x8155, FALSE},
        {0x82a0, FALSE}, {0x833f, FALSE}, {0x8340, TRUE }, {0x837e, TRUE },
        {0x837f, FALSE}, {0x8380, TRUE }, {0x8396, TRUE }, {0x8397, FALSE},
        {0xa5, FALSE}, {0xb0, FALSE}, {0xdd, FALSE}
    };
    unsigned int prev_cp = _getmbcp();
    int ret;
    unsigned int i;

    _setmbcp(_MB_CP_SBCS);
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        ret = _ismbckata(tests[i].c);
        ok(!ret, "expected 0, got %d for %04x\n", ret, tests[i].c);
    }

    _setmbcp(932);
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        ret = _ismbckata(tests[i].c);
        ok(!!ret == tests[i].exp, "expected %d, got %d for %04x\n",
           tests[i].exp, !!ret, tests[i].c);
    }

    _setmbcp(prev_cp);
}

static void test_ismbclegal(void) {
    unsigned int prev_cp = _getmbcp();
    int ret, exp, err;
    unsigned int i;

    _setmbcp(932); /* Japanese */
    err = 0;
    for(i = 0; i < 0x10000; i++) {
        ret = _ismbclegal(i);
        exp = ((HIBYTE(i) >= 0x81 && HIBYTE(i) <= 0x9F) ||
               (HIBYTE(i) >= 0xE0 && HIBYTE(i) <= 0xFC)) &&
              ((LOBYTE(i) >= 0x40 && LOBYTE(i) <= 0x7E) ||
               (LOBYTE(i) >= 0x80 && LOBYTE(i) <= 0xFC));
        if(ret != exp) {
            err = 1;
            break;
        }
    }
    ok(!err, "_ismbclegal (932) : Expected 0x%x, got 0x%x (0x%x)\n", exp, ret, i);
    _setmbcp(936); /* Chinese (GBK) */
    err = 0;
    for(i = 0; i < 0x10000; i++) {
        ret = _ismbclegal(i);
        exp = HIBYTE(i) >= 0x81 && HIBYTE(i) <= 0xFE &&
              LOBYTE(i) >= 0x40 && LOBYTE(i) <= 0xFE;
        if(ret != exp) {
            err = 1;
            break;
        }
    }
    ok(!err, "_ismbclegal (936) : Expected 0x%x, got 0x%x (0x%x)\n", exp, ret, i);
    _setmbcp(949); /* Korean */
    err = 0;
    for(i = 0; i < 0x10000; i++) {
        ret = _ismbclegal(i);
        exp = HIBYTE(i) >= 0x81 && HIBYTE(i) <= 0xFE &&
              LOBYTE(i) >= 0x41 && LOBYTE(i) <= 0xFE;
        if(ret != exp) {
            err = 1;
            break;
        }
    }
    ok(!err, "_ismbclegal (949) : Expected 0x%x, got 0x%x (0x%x)\n", exp, ret, i);
    _setmbcp(950); /* Chinese (Big5) */
    err = 0;
    for(i = 0; i < 0x10000; i++) {
        ret = _ismbclegal(i);
        exp = HIBYTE(i) >= 0x81 && HIBYTE(i) <= 0xFE &&
            ((LOBYTE(i) >= 0x40 && LOBYTE(i) <= 0x7E) ||
             (LOBYTE(i) >= 0xA1 && LOBYTE(i) <= 0xFE));
        if(ret != exp) {
            err = 1;
            break;
        }
    }
    ok(!err, "_ismbclegal (950) : Expected 0x%x, got 0x%x (0x%x)\n", exp, ret, i);
    _setmbcp(1361); /* Korean (Johab) */
    err = 0;
    for(i = 0; i < 0x10000; i++) {
        ret = _ismbclegal(i);
        exp = ((HIBYTE(i) >= 0x81 && HIBYTE(i) <= 0xD3) ||
               (HIBYTE(i) >= 0xD8 && HIBYTE(i) <= 0xF9)) &&
              ((LOBYTE(i) >= 0x31 && LOBYTE(i) <= 0x7E) ||
               (LOBYTE(i) >= 0x81 && LOBYTE(i) <= 0xFE)) &&
                HIBYTE(i) != 0xDF;
        if(ret != exp) {
            err = 1;
            break;
        }
    }
    todo_wine ok(!err, "_ismbclegal (1361) : Expected 0x%x, got 0x%x (0x%x)\n", exp, ret, i);

    _setmbcp(prev_cp);
}

static const struct {
    const char* string;
    const char* delimiter;
    int exp_offsetret1; /* returned offset from string after first call to strtok()
                           -1 means NULL  */
    int exp_offsetret2; /* returned offset from string after second call to strtok()
                           -1 means NULL  */
    int exp_offsetret3; /* returned offset from string after third call to strtok()
                           -1 means NULL  */
} testcases_strtok[] = {
    { "red cabernet", " ", 0, 4, -1 },
    { "sparkling white riesling", " ", 0, 10, 16 },
    { " pale cream sherry", "e ", 1, 6, 9 },
    /* end mark */
    { 0}
};

static void test_strtok(void)
{
    int i;
    char *strret;
    char teststr[100];
    for( i = 0; testcases_strtok[i].string; i++){
        strcpy( teststr, testcases_strtok[i].string);
        strret = strtok( teststr, testcases_strtok[i].delimiter);
        ok( (int)(strret - teststr) ==  testcases_strtok[i].exp_offsetret1 ||
                (!strret && testcases_strtok[i].exp_offsetret1 == -1),
                "string (%p) \'%s\' return %p\n",
                teststr, testcases_strtok[i].string, strret);
        if( !strret) continue;
        strret = strtok( NULL, testcases_strtok[i].delimiter);
        ok( (int)(strret - teststr) ==  testcases_strtok[i].exp_offsetret2 ||
                (!strret && testcases_strtok[i].exp_offsetret2 == -1),
                "second call string (%p) \'%s\' return %p\n",
                teststr, testcases_strtok[i].string, strret);
        if( !strret) continue;
        strret = strtok( NULL, testcases_strtok[i].delimiter);
        ok( (int)(strret - teststr) ==  testcases_strtok[i].exp_offsetret3 ||
                (!strret && testcases_strtok[i].exp_offsetret3 == -1),
                "third call string (%p) \'%s\' return %p\n",
                teststr, testcases_strtok[i].string, strret);
    }

    strcpy( teststr, "test a=b" );
    strret = strtok( teststr, " " );
    ok( strret == teststr, "strret = %p, expected %p\n", strret, teststr );
    strret = strtok( NULL, "ab=" );
    ok( !strret, "strret = %p, expected NULL\n", strret );
    strret = strtok( NULL, "=" );
    ok( !strret, "strret = %p, expected NULL\n", strret );
}

static void test_strtol(void)
{
    static char neg[] = "-0x";

    char* e;
    LONG l;
    ULONG ul;

    /* errno is only set in case of error, so reset errno to EBADF to check for errno modification */
    /* errno is modified on W2K8+ */
    errno = EBADF;
    l = strtol("-1234", &e, 0);
    ok(l==-1234, "wrong value %ld\n", l);
    ok(errno == EBADF || broken(errno == 0), "wrong errno %d\n", errno);
    errno = EBADF;
    ul = strtoul("1234", &e, 0);
    ok(ul==1234, "wrong value %lu\n", ul);
    ok(errno == EBADF || broken(errno == 0), "wrong errno %d\n", errno);

    errno = EBADF;
    l = strtol("2147483647L", &e, 0);
    ok(l==2147483647, "wrong value %ld\n", l);
    ok(errno == EBADF || broken(errno == 0), "wrong errno %d\n", errno);
    errno = EBADF;
    l = strtol("-2147483648L", &e, 0);
    ok(l==-2147483647L - 1, "wrong value %ld\n", l);
    ok(errno == EBADF || broken(errno == 0), "wrong errno %d\n", errno);
    errno = EBADF;
    ul = strtoul("4294967295UL", &e, 0);
    ok(ul==4294967295ul, "wrong value %lu\n", ul);
    ok(errno == EBADF || broken(errno == 0), "wrong errno %d\n", errno);

    errno = 0;
    l = strtol("9223372036854775807L", &e, 0);
    ok(l==2147483647, "wrong value %ld\n", l);
    ok(errno == ERANGE, "wrong errno %d\n", errno);
    errno = 0;
    ul = strtoul("9223372036854775807L", &e, 0);
    ok(ul==4294967295ul, "wrong value %lu\n", ul);
    ok(errno == ERANGE, "wrong errno %d\n", errno);

    errno = 0;
    ul = strtoul("-2", NULL, 0);
    ok(ul == -2, "wrong value %lu\n", ul);
    ok(errno == 0, "wrong errno %d\n", errno);

    errno = 0;
    ul = strtoul("-4294967294", NULL, 0);
    ok(ul == 2, "wrong value %lu\n", ul);
    ok(errno == 0, "wrong errno %d\n", errno);

    errno = 0;
    ul = strtoul("-4294967295", NULL, 0);
    ok(ul==1, "wrong value %lu\n", ul);
    ok(errno == 0, "wrong errno %d\n", errno);

    errno = 0;
    ul = strtoul("-4294967296", NULL, 0);
    ok(ul == 1, "wrong value %lu\n", ul);
    ok(errno == ERANGE, "wrong errno %d\n", errno);

    errno = 0;
    l = strtol(neg, &e, 0);
    ok(l == 0, "wrong value %ld\n", l);
    ok(errno == 0, "wrong errno %d\n", errno);
    ok(e == neg, "e = %p, neg = %p\n", e, neg);
}

static void test_strnlen(void)
{
    static const char str[] = "string";
    size_t res;

    if(!p_strnlen) {
        win_skip("strnlen not found\n");
        return;
    }

    res = p_strnlen(str, 20);
    ok(res == 6, "Returned length = %d\n", (int)res);

    res = p_strnlen(str, 3);
    ok(res == 3, "Returned length = %d\n", (int)res);

    res = p_strnlen(NULL, 0);
    ok(res == 0, "Returned length = %d\n", (int)res);
}

static void test__strtoi64(void)
{
    static const char no1[] = "31923";
    static const char no2[] = "-213312";
    static const char no3[] = "12aa";
    static const char no4[] = "abc12";
    static const char overflow[] = "99999999999999999999";
    static const char neg_overflow[] = "-99999999999999999999";
    static const char hex[] = "0x123";
    static const char oct[] = "000123";
    static const char blanks[] = "        12 212.31";

    __int64 res;
    unsigned __int64 ures;
    char *endpos;

    if(!p_strtoi64 || !p_strtoui64) {
        win_skip("_strtoi64 or _strtoui64 not found\n");
        return;
    }

    errno = 0xdeadbeef;
    res = p_strtoi64(no1, NULL, 10);
    ok(res == 31923, "res != 31923\n");
    res = p_strtoi64(no2, NULL, 10);
    ok(res == -213312, "res != -213312\n");
    res = p_strtoi64(no3, NULL, 10);
    ok(res == 12, "res != 12\n");
    res = p_strtoi64(no4, &endpos, 10);
    ok(res == 0, "res != 0\n");
    ok(endpos == no4, "Scanning was not stopped on first character\n");
    res = p_strtoi64(hex, &endpos, 10);
    ok(res == 0, "res != 0\n");
    ok(endpos == hex+1, "Incorrect endpos (%p-%p)\n", hex, endpos);
    res = p_strtoi64(oct, &endpos, 10);
    ok(res == 123, "res != 123\n");
    ok(endpos == oct+strlen(oct), "Incorrect endpos (%p-%p)\n", oct, endpos);
    res = p_strtoi64(blanks, &endpos, 10);
    ok(res == 12, "res != 12\n");
    ok(endpos == blanks+10, "Incorrect endpos (%p-%p)\n", blanks, endpos);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    res = p_strtoi64(overflow, &endpos, 10);
    ok(res == _I64_MAX, "res != _I64_MAX\n");
    ok(endpos == overflow+strlen(overflow), "Incorrect endpos (%p-%p)\n", overflow, endpos);
    ok(errno == ERANGE, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    res = p_strtoi64(neg_overflow, &endpos, 10);
    ok(res == _I64_MIN, "res != _I64_MIN\n");
    ok(endpos == neg_overflow+strlen(neg_overflow), "Incorrect endpos (%p-%p)\n", neg_overflow, endpos);
    ok(errno == ERANGE, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    res = p_strtoi64(no1, &endpos, 16);
    ok(res == 203043, "res != 203043\n");
    ok(endpos == no1+strlen(no1), "Incorrect endpos (%p-%p)\n", no1, endpos);
    res = p_strtoi64(no2, &endpos, 16);
    ok(res == -2175762, "res != -2175762\n");
    ok(endpos == no2+strlen(no2), "Incorrect endpos (%p-%p)\n", no2, endpos);
    res = p_strtoi64(no3, &endpos, 16);
    ok(res == 4778, "res != 4778\n");
    ok(endpos == no3+strlen(no3), "Incorrect endpos (%p-%p)\n", no3, endpos);
    res = p_strtoi64(no4, &endpos, 16);
    ok(res == 703506, "res != 703506\n");
    ok(endpos == no4+strlen(no4), "Incorrect endpos (%p-%p)\n", no4, endpos);
    res = p_strtoi64(hex, &endpos, 16);
    ok(res == 291, "res != 291\n");
    ok(endpos == hex+strlen(hex), "Incorrect endpos (%p-%p)\n", hex, endpos);
    res = p_strtoi64(oct, &endpos, 16);
    ok(res == 291, "res != 291\n");
    ok(endpos == oct+strlen(oct), "Incorrect endpos (%p-%p)\n", oct, endpos);
    res = p_strtoi64(blanks, &endpos, 16);
    ok(res == 18, "res != 18\n");
    ok(endpos == blanks+10, "Incorrect endpos (%p-%p)\n", blanks, endpos);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    res = p_strtoi64(hex, &endpos, 36);
    ok(res == 1541019, "res != 1541019\n");
    ok(endpos == hex+strlen(hex), "Incorrect endpos (%p-%p)\n", hex, endpos);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    res = p_strtoi64(no1, &endpos, 0);
    ok(res == 31923, "res != 31923\n");
    ok(endpos == no1+strlen(no1), "Incorrect endpos (%p-%p)\n", no1, endpos);
    res = p_strtoi64(no2, &endpos, 0);
    ok(res == -213312, "res != -213312\n");
    ok(endpos == no2+strlen(no2), "Incorrect endpos (%p-%p)\n", no2, endpos);
    res = p_strtoi64(no3, &endpos, 10);
    ok(res == 12, "res != 12\n");
    ok(endpos == no3+2, "Incorrect endpos (%p-%p)\n", no3, endpos);
    res = p_strtoi64(no4, &endpos, 10);
    ok(res == 0, "res != 0\n");
    ok(endpos == no4, "Incorrect endpos (%p-%p)\n", no4, endpos);
    res = p_strtoi64(hex, &endpos, 10);
    ok(res == 0, "res != 0\n");
    ok(endpos == hex+1, "Incorrect endpos (%p-%p)\n", hex, endpos);
    res = p_strtoi64(oct, &endpos, 10);
    ok(res == 123, "res != 123\n");
    ok(endpos == oct+strlen(oct), "Incorrect endpos (%p-%p)\n", oct, endpos);
    res = p_strtoi64(blanks, &endpos, 10);
    ok(res == 12, "res != 12\n");
    ok(endpos == blanks+10, "Incorrect endpos (%p-%p)\n", blanks, endpos);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    ures = p_strtoui64(no1, &endpos, 0);
    ok(ures == 31923, "ures != 31923\n");
    ok(endpos == no1+strlen(no1), "Incorrect endpos (%p-%p)\n", no1, endpos);
    ures = p_strtoui64(no2, &endpos, 0);
    ok(ures == -213312, "ures != -213312\n");
    ok(endpos == no2+strlen(no2), "Incorrect endpos (%p-%p)\n", no2, endpos);
    ures = p_strtoui64(no3, &endpos, 10);
    ok(ures == 12, "ures != 12\n");
    ok(endpos == no3+2, "Incorrect endpos (%p-%p)\n", no3, endpos);
    ures = p_strtoui64(no4, &endpos, 10);
    ok(ures == 0, "ures != 0\n");
    ok(endpos == no4, "Incorrect endpos (%p-%p)\n", no4, endpos);
    ures = p_strtoui64(hex, &endpos, 10);
    ok(ures == 0, "ures != 0\n");
    ok(endpos == hex+1, "Incorrect endpos (%p-%p)\n", hex, endpos);
    ures = p_strtoui64(oct, &endpos, 10);
    ok(ures == 123, "ures != 123\n");
    ok(endpos == oct+strlen(oct), "Incorrect endpos (%p-%p)\n", oct, endpos);
    ures = p_strtoui64(blanks, &endpos, 10);
    ok(ures == 12, "ures != 12\n");
    ok(endpos == blanks+10, "Incorrect endpos (%p-%p)\n", blanks, endpos);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    ures = p_strtoui64(overflow, &endpos, 10);
    ok(ures == _UI64_MAX, "ures != _UI64_MAX\n");
    ok(endpos == overflow+strlen(overflow), "Incorrect endpos (%p-%p)\n", overflow, endpos);
    ok(errno == ERANGE, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    ures = p_strtoui64(neg_overflow, &endpos, 10);
    ok(ures == 1, "ures != 1\n");
    ok(endpos == neg_overflow+strlen(neg_overflow), "Incorrect endpos (%p-%p)\n", neg_overflow, endpos);
    ok(errno == ERANGE, "errno = %x\n", errno);
}

static inline BOOL compare_double(double f, double g, unsigned int ulps)
{
    ULONGLONG x = *(ULONGLONG *)&f;
    ULONGLONG y = *(ULONGLONG *)&g;

    if (f < 0)
        x = ~x + 1;
    else
        x |= ((ULONGLONG)1)<<63;
    if (g < 0)
        y = ~y + 1;
    else
        y |= ((ULONGLONG)1)<<63;

    return (x>y ? x-y : y-x) <= ulps;
}

static void test__strtod(void)
{
    static const struct {
        const char *str;
        int len;
        double ret;
        int err;
    } tests[] = {
        { "12.1", 4, 12.1 },
        { "-13.721", 7, -13.721 },
        { "INF", 0, 0 },
        { ".21e12", 6, 210000000000.0 },
        { "214353e-3", 9, 214.353 },
        { "NAN", 0, 0 },
        { "12.1d2", 6, 12.1e2 },
        { "  d10", 0, 0 },
        { "0.1", 3, 0.1 },
        { "-0.1", 4, -0.1 },
        { "0.1281832188491894198128921", 27, 0.1281832188491894198128921 },
        { "0.82181281288121", 16, 0.82181281288121 },
        { "21921922352523587651128218821", 29, 21921922352523587651128218821.0 },
        { "0.1d238", 7, 0.1e238 },
        { "0.1D-4736", 9, 0, ERANGE },
        { "3.4028234663852887e38", 21, FLT_MAX },
        { "1.1754943508222875e-38", 22, FLT_MIN },
        { "1.7976931348623158e+308", 23, DBL_MAX },
        { "1.7976931348623159e+308", 23, INFINITY, ERANGE },
        { "2.2250738585072014e-308", 23, DBL_MIN },
        { "-1.7976931348623158e+308", 24, -DBL_MAX },
        { "-1.7976931348623159e+308", 24, -INFINITY, ERANGE },
        { "00", 2, 0 },
        { "00.", 3, 0 },
        { ".00", 3, 0 },
        { "-0.", 3, 0 },
        { "0e13", 4, 0 },
    };
    const char overflow[] = "1d9999999999999999999";

    char *end;
    double d;
    int i;

    for (i=0; i<ARRAY_SIZE(tests); i++)
    {
#ifdef __REACTOS__
        if ((i == 19) && IsReactOS())
        {
            skip("Skipping i == 19, because it crashes on ReactOS\n");
            continue;
        }
#endif
        errno = 0xdeadbeef;
        d = strtod(tests[i].str, &end);
        ok(d == tests[i].ret, "%d) d = %.16e\n", i, d);
        ok(end == tests[i].str + tests[i].len, "%d) len = %d\n",
                i, (int)(end - tests[i].str));
        ok(errno == tests[i].err || (!tests[i].err && errno == 0xdeadbeef) /* <= 2003 */,
                "%d) errno = %d\n", i, errno);
    }

    if (!p__strtod_l)
        win_skip("_strtod_l not found\n");
    else
    {
        errno = EBADF;
        d = strtod(NULL, NULL);
        ok(d == 0.0, "d = %.16e\n", d);
        ok(errno == EINVAL, "errno = %x\n", errno);

        errno = EBADF;
        end = (char *)0xdeadbeef;
        d = strtod(NULL, &end);
        ok(d == 0.0, "d = %.16e\n", d);
        ok(errno == EINVAL, "errno = %x\n", errno);
        ok(!end, "incorrect end ptr %p\n", end);

        errno = EBADF;
        d = p__strtod_l(NULL, NULL, NULL);
        ok(d == 0.0, "d = %.16e\n", d);
        ok(errno == EINVAL, "errno = %x\n", errno);
    }

    /* Set locale with non '.' decimal point (',') */
    if(!setlocale(LC_ALL, "Polish")) {
        win_skip("system with limited locales\n");
        return;
    }

    d = strtod("12.1", NULL);
    ok(d == 12.0, "d = %.16e\n", d);

    d = strtod("12,1", NULL);
    ok(d == 12.1, "d = %.16e\n", d);

    setlocale(LC_ALL, "C");

    errno = 0xdeadbeef;
    strtod(overflow, &end);
    ok(errno == ERANGE, "errno = %x\n", errno);
    ok(end == overflow+21, "incorrect end (%d)\n", (int)(end-overflow));

    errno = 0xdeadbeef;
    strtod("-1d309", NULL);
    ok(errno == ERANGE, "errno = %x\n", errno);
}

static void test_mbstowcs(void)
{
    static const wchar_t wSimple[] = L"text";
    static const wchar_t wHiragana[] = L"\x3042\x3043";
    static const char mSimple[] = "text";
    static const char mHiragana[] = { 0x82,0xa0,0x82,0xa1,0 };

    const wchar_t *pwstr;
    wchar_t wOut[6];
    char mOut[6];
    size_t ret;
    int err;
    const char *pmbstr;
    mbstate_t state;

    wOut[4] = '!'; wOut[5] = '\0';
    mOut[4] = '!'; mOut[5] = '\0';

    if(pmbstowcs_s) {
        /* crashes on some systems */
        errno = 0xdeadbeef;
        ret = mbstowcs(wOut, NULL, 4);
        ok(ret == -1, "mbstowcs did not return -1\n");
        ok(errno == EINVAL, "errno = %d\n", errno);
    }

    ret = mbstowcs(NULL, mSimple, 0);
    ok(ret == 4, "mbstowcs did not return 4\n");

    ret = mbstowcs(wOut, mSimple, 4);
    ok(ret == 4, "mbstowcs did not return 4\n");
    ok(!memcmp(wOut, wSimple, 4*sizeof(wchar_t)), "wOut = %s\n", wine_dbgstr_w(wOut));
    ok(wOut[4] == '!', "wOut[4] != \'!\'\n");

    ret = mbstowcs(NULL, "", 1);
    ok(ret == 0, "mbstowcs did not return 0, got %d\n", (int)ret);

    ret = mbstowcs(wOut, "", 1);
    ok(ret == 0, "mbstowcs did not return 0, got %d\n", (int)ret);
    ok(!wOut[0], "wOut = %s\n", wine_dbgstr_w(wOut));

    ret = wcstombs(NULL, wSimple, 0);
    ok(ret == 4, "wcstombs did not return 4\n");

    ret = wcstombs(mOut, wSimple, 6);
    ok(ret == 4, "wcstombs did not return 4\n");
    ok(!memcmp(mOut, mSimple, 5*sizeof(char)), "mOut = %s\n", mOut);

    ret = wcstombs(mOut, wSimple, 2);
    ok(ret == 2, "wcstombs did not return 2\n");
    ok(!memcmp(mOut, mSimple, 5*sizeof(char)), "mOut = %s\n", mOut);

    ret = wcstombs(NULL, L"", 1);
    ok(ret == 0, "wcstombs did not return 0, got %d\n", (int)ret);

    ret = wcstombs(mOut, L"", 1);
    ok(ret == 0, "wcstombs did not return 0, got %d\n", (int)ret);
    ok(!mOut[0], "mOut = %s\n", mOut);

    if(pwcsrtombs) {
        pwstr = wSimple;
        err = -3;
        ret = pwcsrtombs(mOut, &pwstr, 4, &err);
        ok(ret == 4, "wcsrtombs did not return 4\n");
        ok(err == 0, "err = %d\n", err);
        ok(pwstr == wSimple+4, "pwstr = %p (wszSimple = %p)\n", pwstr, wSimple);
        ok(!memcmp(mOut, mSimple, ret), "mOut = %s\n", mOut);

        pwstr = wSimple;
        ret = pwcsrtombs(mOut, &pwstr, 5, NULL);
        ok(ret == 4, "wcsrtombs did not return 4\n");
        ok(pwstr == NULL, "pwstr != NULL\n");
        ok(!memcmp(mOut, mSimple, sizeof(mSimple)), "mOut = %s\n", mOut);
    }

    if(!setlocale(LC_ALL, "Japanese_Japan.932")) {
        win_skip("Japanese_Japan.932 locale not available\n");
        return;
    }

    ret = mbstowcs(wOut, mHiragana, 6);
    ok(ret == 2, "mbstowcs did not return 2\n");
    ok(!memcmp(wOut, wHiragana, sizeof(wHiragana)), "wOut = %s\n", wine_dbgstr_w(wOut));

    ret = mbstowcs(wOut, "", 6);
    ok(ret == 0, "mbstowcs did not return 0, got %d\n", (int)ret);
    ok(!wOut[0], "wOut = %s\n", wine_dbgstr_w(wOut));

    errno = 0xdeadbeef;
    ret = mbstowcs(wOut, mHiragana+1, 5);
    ok(ret == -1, "mbstowcs did not return -1\n");
    ok(errno == EILSEQ, "errno = %d\n", errno);

    ret = wcstombs(mOut, wHiragana, 6);
    ok(ret == 4, "wcstombs did not return 4\n");
    ok(!memcmp(mOut, mHiragana, sizeof(mHiragana)), "mOut = %s\n", mOut);

    ret = wcstombs(mOut, L"", 6);
    ok(ret == 0, "wcstombs did not return 0, got %d\n", (int)ret);
    ok(!mOut[0], "mOut = %s\n", mOut);

    if(!pmbstowcs_s || !pwcstombs_s) {
        setlocale(LC_ALL, "C");
        win_skip("mbstowcs_s or wcstombs_s not available\n");
        return;
    }

    err = pmbstowcs_s(&ret, wOut, 1, mSimple, _TRUNCATE);
    ok(err == STRUNCATE, "err = %d\n", err);
    ok(ret == 1, "mbstowcs_s did not return 0\n");
    ok(!wOut[0], "wOut[0] = %d\n", wOut[0]);

    err = pmbstowcs_s(&ret, wOut, 6, mSimple, _TRUNCATE);
    ok(err == 0, "err = %d\n", err);
    ok(ret == 5, "mbstowcs_s did not return 5\n");
    ok(!memcmp(wOut, wSimple, sizeof(wSimple)), "wOut = %s\n", wine_dbgstr_w(wOut));

    err = pmbstowcs_s(&ret, wOut, 6, mHiragana, _TRUNCATE);
    ok(err == 0, "err = %d\n", err);
    ok(ret == 3, "mbstowcs_s did not return 3\n");
    ok(!memcmp(wOut, wHiragana, sizeof(wHiragana)), "wOut = %s\n", wine_dbgstr_w(wOut));

    err = pmbstowcs_s(&ret, wOut, 6, "", _TRUNCATE);
    ok(err == 0, "err = %d\n", err);
    ok(ret == 1, "mbstowcs_s did not return 1, got %d\n", (int)ret);
    ok(!wOut[0], "wOut = %s\n", wine_dbgstr_w(wOut));

    err = pmbstowcs_s(&ret, NULL, 0, mHiragana, 1);
    ok(err == 0, "err = %d\n", err);
    ok(ret == 3, "mbstowcs_s did not return 3\n");

    err = pwcstombs_s(&ret, mOut, 6, wSimple, _TRUNCATE);
    ok(err == 0, "err = %d\n", err);
    ok(ret == 5, "wcstombs_s did not return 5\n");
    ok(!memcmp(mOut, mSimple, sizeof(mSimple)), "mOut = %s\n", mOut);

    err = pwcstombs_s(&ret, mOut, 6, wHiragana, _TRUNCATE);
    ok(err == 0, "err = %d\n", err);
    ok(ret == 5, "wcstombs_s did not return 5\n");
    ok(!memcmp(mOut, mHiragana, sizeof(mHiragana)), "mOut = %s\n", mOut);

    err = pwcstombs_s(&ret, mOut, 6, L"", _TRUNCATE);
    ok(err == 0, "err = %d\n", err);
    ok(ret == 1, "wcstombs_s did not return 1, got %d\n", (int)ret);
    ok(!mOut[0], "mOut = %s\n", mOut);

    err = pwcstombs_s(&ret, NULL, 0, wHiragana, 1);
    ok(err == 0, "err = %d\n", err);
    ok(ret == 5, "wcstombs_s did not return 5\n");

    if(!pwcsrtombs) {
        setlocale(LC_ALL, "C");
        win_skip("wcsrtombs not available\n");
        return;
    }

    pwstr = wSimple;
    err = -3;
    ret = pwcsrtombs(mOut, &pwstr, 4, &err);
    ok(ret == 4, "wcsrtombs did not return 4\n");
    ok(err == 0, "err = %d\n", err);
    ok(pwstr == wSimple+4, "pwstr = %p (wszSimple = %p)\n", pwstr, wSimple);
    ok(!memcmp(mOut, mSimple, ret), "mOut = %s\n", mOut);

    pwstr = wSimple;
    ret = pwcsrtombs(mOut, &pwstr, 5, NULL);
    ok(ret == 4, "wcsrtombs did not return 4\n");
    ok(pwstr == NULL, "pwstr != NULL\n");
    ok(!memcmp(mOut, mSimple, sizeof(mSimple)), "mOut = %s\n", mOut);

    if(!p_mbsrtowcs) {
        setlocale(LC_ALL, "C");
        win_skip("mbsrtowcs not available\n");
        return;
    }

    pmbstr = mHiragana;
    ret = p_mbsrtowcs(NULL, &pmbstr, 6, NULL);
    ok(ret == 2, "mbsrtowcs did not return 2\n");
    ok(pmbstr == mHiragana, "pmbstr = %p, expected %p\n", pmbstr, mHiragana);

    pmbstr = mHiragana;
    ret = p_mbsrtowcs(wOut, &pmbstr, 6, NULL);
    ok(ret == 2, "mbsrtowcs did not return 2\n");
    ok(!memcmp(wOut, wHiragana, sizeof(wHiragana)), "wOut = %s\n", wine_dbgstr_w(wOut));
    ok(!pmbstr, "pmbstr != NULL\n");

    state = mHiragana[0];
    pmbstr = mHiragana+1;
    ret = p_mbsrtowcs(wOut, &pmbstr, 6, &state);
    ok(ret == 2, "mbsrtowcs did not return 2\n");
    ok(wOut[0] == 0x3042, "wOut[0] = %x\n", wOut[0]);
    ok(wOut[1] == 0xff61, "wOut[1] = %x\n", wOut[1]);
    ok(wOut[2] == 0, "wOut[2] = %x\n", wOut[2]);
    ok(!pmbstr, "pmbstr != NULL\n");

    errno = EBADF;
    ret = p_mbsrtowcs(wOut, NULL, 6, &state);
    ok(ret == -1, "mbsrtowcs did not return -1\n");
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    if(!p_mbsrtowcs_s) {
        setlocale(LC_ALL, "C");
        win_skip("mbsrtowcs_s not available\n");
        return;
    }

    pmbstr = mHiragana;
    err = p_mbsrtowcs_s(&ret, NULL, 0, NULL, 6, NULL);
    ok(ret == -1, "mbsrtowcs_s did not return -1\n");
    ok(err == EINVAL, "err = %d\n", err);
    err = p_mbsrtowcs_s(&ret, NULL, 1, &pmbstr, 6, NULL);
    ok(ret == -1, "mbsrtowcs_s did not return -1\n");
    ok(err == EINVAL, "err = %d\n", err);
    err = p_mbsrtowcs_s(&ret, wOut, 0, &pmbstr, 6, NULL);
    ok(ret == -1, "mbsrtowcs_s did not return -1\n");
    ok(err == EINVAL, "err = %d\n", err);

    pmbstr = mHiragana;
    errno = 0;
    err = p_mbsrtowcs_s(&ret, NULL, 0, &pmbstr, 6, NULL);
    ok(ret == 3, "mbsrtowcs_s did not return 3\n");
    ok(err == 0, "err = %d\n", err);
    ok(pmbstr == mHiragana, "pmbstr = %p, expected %p\n", pmbstr, mHiragana);
    ok(errno == 0, "errno = %d\n", errno);

    pmbstr = mHiragana;
    err = p_mbsrtowcs_s(&ret, wOut, 1, &pmbstr, 6, NULL);
    ok(ret == 2, "mbsrtowcs_s did not return 2\n");
    ok(err == 0, "err = %d\n", err);
    ok(!wOut[0], "wOut[0] = '%c'\n", wOut[0]);
    ok(pmbstr == mHiragana+2, "pmbstr = %p, expected %p\n", pmbstr, mHiragana+2);
    ok(errno == 0, "errno = %d\n", errno);

    pmbstr = mHiragana;
    err = p_mbsrtowcs_s(&ret, wOut, 2, &pmbstr, 6, NULL);
    ok(ret == 3, "mbsrtowcs_s did not return 3\n");
    ok(err == 0, "err = %d\n", err);
    ok(!wOut[0], "wOut[0] = '%c'\n", wOut[0]);
    ok(pmbstr == mHiragana+4, "pmbstr = %p, expected %p\n", pmbstr, mHiragana+4);
    ok(errno == 0, "errno = %d\n", errno);

    pmbstr = mHiragana;
    err = p_mbsrtowcs_s(&ret, wOut, 3, &pmbstr, 6, NULL);
    ok(ret == 3, "mbsrtowcs_s did not return 3\n");
    ok(err == 0, "err = %d\n", err);
    ok(!pmbstr, "pmbstr != NULL\n");
    ok(errno == 0, "errno = %d\n", errno);

    setlocale(LC_ALL, "C");
}

static void test__wcstombs_s_l(void)
{
    struct test {
        const wchar_t *wstr;
        size_t wlen;
        const char *str;
        size_t len;
        size_t ret;
        int err;
        const char *locale;
    } tests[] = {
        /* wstr                 str        ret err        locale */
        { L"",       0,         NULL,   0, 1,  0,         NULL },
        { L"\xfffd", 1,         NULL,   0, 2,  0,         NULL },
        { L"\xfffd", 1,         "",     1, 0,  EILSEQ,    NULL },
        { L"\xfffd", 1,         "",     6, 0,  EILSEQ,    NULL },
        { L"text",   _TRUNCATE, "text", 5, 5,  0,         NULL },
        { L"text",   _TRUNCATE, "",     1, 1,  STRUNCATE, NULL },
        { L"text",   5,         "",     3, 0,  ERANGE,    NULL },

        { L"",       0,         NULL,   0, 1,  0,         "English_United States.1252" },
        { L"\xfffd", 1,         NULL,   0, 0,  EILSEQ,    "English_United States.1252" },
        { L"\xfffd", 1,         "",     1, 0,  EILSEQ,    "English_United States.1252" },
        { L"\xfffd", 1,         "",     6, 0,  EILSEQ,    "English_United States.1252" },
        { L"text",   _TRUNCATE, "text", 5, 5,  0,         "English_United States.1252" },
        { L"text",   _TRUNCATE, "",     1, 1,  STRUNCATE, "English_United States.1252" },
        { L"text",   5,         "",     3, 0,  ERANGE,    "English_United States.1252" },
    };
    _locale_t locale;
    char out[6];
    size_t ret;
    int err;
    int i;

    if(!p__create_locale) {
        win_skip("_create_locale not available\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        if(tests[i].locale)
        {
            locale = p__create_locale(LC_ALL, tests[i].locale);
            ok(locale != NULL, "_create_locale failed for %s\n", tests[i].locale);
        }
        else
            locale = NULL;

        ret = ~0;
        memset(out, 0xcc, sizeof(out));
        err = p_wcstombs_s_l(&ret, tests[i].str ? out : NULL, tests[i].len,
                             tests[i].wstr, tests[i].wlen, locale);
        ok(ret == tests[i].ret, "%d: expected ret %Id, got %Id for '%s' in locale %s\n", i, tests[i].ret, ret,
            wine_dbgstr_w(tests[i].wstr), tests[i].locale);
        ok(err == tests[i].err, "%d: expected err %d, got %d for '%s' in locale %s\n", i, tests[i].err, err,
            wine_dbgstr_w(tests[i].wstr), tests[i].locale);
        if(tests[i].str)
            ok(!memcmp(out, tests[i].str, strlen(tests[i].str)+1),
                "%d: expected out %s, got %s for '%s' in locale %s\n", i, tests[i].str, out,
                wine_dbgstr_w(tests[i].wstr), tests[i].locale);

        p__free_locale(locale);
    }
}

static void test_gcvt(void)
{
    char buf[1024], *res;
    errno_t err;

    if(!p_gcvt_s) {
        win_skip("Skipping _gcvt tests\n");
        return;
    }

    errno = 0;
    res = _gcvt(1.2, -1, buf);
    ok(res == NULL, "res != NULL\n");
    ok(errno == ERANGE, "errno = %d\n", errno);

    errno = 0;
    res = _gcvt(1.2, 5, NULL);
    ok(res == NULL, "res != NULL\n");
    ok(errno == EINVAL, "errno = %d\n", errno);

    res = gcvt(1.2, 5, buf);
    ok(res == buf, "res != buf\n");
    ok(!strcmp(buf, "1.2"), "buf = %s\n", buf);

    buf[0] = 'x';
    err = p_gcvt_s(buf, 5, 1.2, 10);
    ok(err == ERANGE, "err = %d\n", err);
    ok(buf[0] == '\0', "buf[0] = %c\n", buf[0]);

    buf[0] = 'x';
    err = p_gcvt_s(buf, 4, 123456, 2);
    ok(err == ERANGE, "err = %d\n", err);
    ok(buf[0] == '\0', "buf[0] = %c\n", buf[0]);
}

static void test__itoa_s(void)
{
    errno_t ret;
    char buffer[33];

    if (!p_itoa_s)
    {
        win_skip("Skipping _itoa_s tests\n");
        return;
    }

    errno = EBADF;
    ret = p_itoa_s(0, NULL, 0, 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_itoa_s(0, buffer, 0, 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == 'X', "Expected the output buffer to be untouched\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_itoa_s(0, buffer, sizeof(buffer), 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_itoa_s(0, buffer, sizeof(buffer), 64);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_itoa_s(12345678, buffer, 4, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(errno == ERANGE, "Expected errno to be ERANGE, got %d\n", errno);
    ok(!memcmp(buffer, "\000765", 4),
       "Expected the output buffer to be null terminated with truncated output\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_itoa_s(12345678, buffer, 8, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(errno == ERANGE, "Expected errno to be ERANGE, got %d\n", errno);
    ok(!memcmp(buffer, "\0007654321", 8),
       "Expected the output buffer to be null terminated with truncated output\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_itoa_s(-12345678, buffer, 9, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(errno == ERANGE, "Expected errno to be ERANGE, got %d\n", errno);
    ok(!memcmp(buffer, "\00087654321", 9),
       "Expected the output buffer to be null terminated with truncated output\n");

    ret = p_itoa_s(12345678, buffer, 9, 10);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "12345678"),
       "Expected output buffer string to be \"12345678\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(43690, buffer, sizeof(buffer), 2);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "1010101010101010"),
       "Expected output buffer string to be \"1010101010101010\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(1092009, buffer, sizeof(buffer), 36);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "nell"),
       "Expected output buffer string to be \"nell\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(5704, buffer, sizeof(buffer), 18);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "hag"),
       "Expected output buffer string to be \"hag\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(-12345678, buffer, sizeof(buffer), 10);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "-12345678"),
       "Expected output buffer string to be \"-12345678\", got \"%s\"\n",
       buffer);

    itoa(100, buffer, 100);
    ok(!strcmp(buffer, "10"),
            "Expected output buffer string to be \"10\", got \"%s\"\n", buffer);
}

static void test__strlwr_s(void)
{
    errno_t ret;
    char buffer[20];

    if (!p_strlwr_s)
    {
        win_skip("Skipping _strlwr_s tests\n");
        return;
    }

    errno = EBADF;
    ret = p_strlwr_s(NULL, 0);
    ok(ret == EINVAL, "Expected _strlwr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_strlwr_s(NULL, sizeof(buffer));
    ok(ret == EINVAL, "Expected _strlwr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_strlwr_s(buffer, 0);
    ok(ret == EINVAL, "Expected _strlwr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    strcpy(buffer, "GoRrIsTeR");
    errno = EBADF;
    ret = p_strlwr_s(buffer, 5);
    ok(ret == EINVAL, "Expected _strlwr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(!memcmp(buffer, "\0oRrIsTeR", sizeof("\0oRrIsTeR")),
       "Expected the output buffer to be \"\\0oRrIsTeR\"\n");

    strcpy(buffer, "GoRrIsTeR");
    errno = EBADF;
    ret = p_strlwr_s(buffer, sizeof("GoRrIsTeR") - 1);
    ok(ret == EINVAL, "Expected _strlwr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(!memcmp(buffer, "\0oRrIsTeR", sizeof("\0oRrIsTeR")),
       "Expected the output buffer to be \"\\0oRrIsTeR\"\n");

    strcpy(buffer, "GoRrIsTeR");
    ret = p_strlwr_s(buffer, sizeof("GoRrIsTeR"));
    ok(ret == 0, "Expected _strlwr_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "gorrister"),
       "Expected the output buffer to be \"gorrister\", got \"%s\"\n",
       buffer);

    memcpy(buffer, "GoRrIsTeR\0ELLEN", sizeof("GoRrIsTeR\0ELLEN"));
    ret = p_strlwr_s(buffer, sizeof(buffer));
    ok(ret == 0, "Expected _strlwr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "gorrister\0ELLEN", sizeof("gorrister\0ELLEN")),
       "Expected the output buffer to be \"gorrister\\0ELLEN\", got \"%s\"\n",
       buffer);

    ret = p_strlwr_s((char *)"already_lowercase", sizeof("already_lowercase"));
    ok(ret == 0, "Expected _strlwr_s to return 0, got %d\n", ret);
}

static void test_strncat_s(void)
{
    int ret;
    char dst[4];
    char src[4];

    if (!p_wcsncat_s)
    {
        win_skip("skipping wcsncat_s tests\n");
        return;
    }

    strcpy(src, "abc");
    strcpy(dst, "a");
    ret = p_strncat_s(NULL, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    ret = p_strncat_s(dst, 0, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    ok(dst[0] == 'a', "dst %x\n", dst[0]);
    ret = p_strncat_s(dst, 0, src, _TRUNCATE);
    ok(ret == EINVAL, "err = %d\n", ret);
    ret = p_strncat_s(dst, 4, NULL, 1);
    ok(ret == EINVAL, "err = %d\n", ret);
    ok(!dst[0], "dst %x\n", dst[0]);
    ret = p_strncat_s(NULL, 4, src, 0);
    ok(ret == EINVAL, "err = %d\n", ret);
    strcpy(dst, "a");
    ret = p_strncat_s(dst, 4, NULL, 0);
    ok(ret == 0, "err = %d\n", ret);
    ok(dst[0] == 'a', "dst %x\n", dst[0]);

    dst[0] = 0;
    ret = p_strncat_s(dst, 2, src, 4);
    ok(ret == ERANGE, "err = %d\n", ret);

    dst[0] = 0;
    ret = p_strncat_s(dst, 2, src, _TRUNCATE);
    ok(ret == STRUNCATE, "err = %d\n", ret);
    ok(dst[0] == 'a' && dst[1] == 0, "dst is %s\n", wine_dbgstr_a(dst));

    strcpy(dst, "abc");
    dst[3] = 'd';
    ret = p_strncat_s(dst, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    ok(!dst[0], "dst %x\n", dst[0]);
}

static void test_wcsncat_s(void)
{
    int ret;
    wchar_t dst[4];
    wchar_t src[4];

    if (!p_wcsncat_s)
    {
        win_skip("skipping wcsncat_s tests\n");
        return;
    }

    wcscpy(src, L"abc");
    wcscpy(dst, L"a");
    ret = p_wcsncat_s(NULL, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    ret = p_wcsncat_s(dst, 0, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    ok(dst[0] == 'a', "dst %x\n", dst[0]);
    ret = p_wcsncat_s(dst, 0, src, _TRUNCATE);
    ok(ret == EINVAL, "err = %d\n", ret);
    ok(dst[0] == 'a', "dst %x\n", dst[0]);
    ret = p_wcsncat_s(dst, 4, NULL, 1);
    ok(ret == EINVAL, "err = %d\n", ret);
    ok(!dst[0], "dst %x\n", dst[0]);
    ret = p_wcsncat_s(NULL, 4, src, 0);
    ok(ret == EINVAL, "err = %d\n", ret);
    wcscpy(dst, L"a");
    ret = p_wcsncat_s(dst, 4, NULL, 0);
    ok(ret == 0, "err = %d\n", ret);
    ok(dst[0] == 'a', "dst %x\n", dst[0]);

    dst[0] = 0;
    ret = p_wcsncat_s(dst, 2, src, 4);
    ok(ret == ERANGE, "err = %d\n", ret);

    dst[0] = 0;
    ret = p_wcsncat_s(dst, 2, src, _TRUNCATE);
    ok(ret == STRUNCATE, "err = %d\n", ret);
    ok(dst[0] == 'a' && dst[1] == 0, "dst is %s\n", wine_dbgstr_w(dst));

    wcscpy(dst, L"abc");
    dst[3] = 'd';
    ret = p_wcsncat_s(dst, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    ok(!dst[0], "dst %x\n", dst[0]);
}

static void test__mbsnbcat_s(void)
{
    unsigned char dest[16];
    const unsigned char first[] = "dinosaur";
    const unsigned char second[] = "duck";
    int ret;

    if (!p_mbsnbcat_s)
    {
        win_skip("Skipping _mbsnbcat_s tests\n");
        return;
    }

    /* Test invalid arguments. */
    ret = p_mbsnbcat_s(NULL, 0, NULL, 0);
    ok(ret == 0, "Expected _mbsnbcat_s to return 0, got %d\n", ret);

    errno = EBADF;
    ret = p_mbsnbcat_s(NULL, 10, NULL, 0);
    ok(ret == EINVAL, "Expected _mbsnbcat_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_mbsnbcat_s(NULL, 0, NULL, 10);
    ok(ret == EINVAL, "Expected _mbsnbcat_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memset(dest, 'X', sizeof(dest));
    errno = EBADF;
    ret = p_mbsnbcat_s(dest, 0, NULL, 0);
    ok(ret == EINVAL, "Expected _mbsnbcat_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(dest[0] == 'X', "Expected the output buffer to be untouched\n");

    memset(dest, 'X', sizeof(dest));
    errno = EBADF;
    ret = p_mbsnbcat_s(dest, 0, second, 0);
    ok(ret == EINVAL, "Expected _mbsnbcat_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(dest[0] == 'X', "Expected the output buffer to be untouched\n");

    memset(dest, 'X', sizeof(dest));
    errno = EBADF;
    ret = p_mbsnbcat_s(dest, sizeof(dest), NULL, 0);
    ok(ret == EINVAL, "Expected _mbsnbcat_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(dest[0] == '\0', "Expected the output buffer to be null terminated\n");

    memset(dest, 'X', sizeof(dest));
    errno = EBADF;
    ret = p_mbsnbcat_s(dest, sizeof(dest), NULL, 10);
    ok(ret == EINVAL, "Expected _mbsnbcat_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(dest[0] == '\0', "Expected the output buffer to be null terminated\n");

    memset(dest, 'X', sizeof(dest));
    dest[0] = '\0';
    ret = p_mbsnbcat_s(dest, sizeof(dest), second, sizeof(second));
    ok(ret == 0, "Expected _mbsnbcat_s to return 0, got %d\n", ret);
    ok(!memcmp(dest, second, sizeof(second)),
       "Expected the output buffer string to be \"duck\"\n");

    /* Test source truncation behavior. */
    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    ret = p_mbsnbcat_s(dest, sizeof(dest), second, 0);
    ok(ret == 0, "Expected _mbsnbcat_s to return 0, got %d\n", ret);
    ok(!memcmp(dest, first, sizeof(first)),
       "Expected the output buffer string to be \"dinosaur\"\n");

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    ret = p_mbsnbcat_s(dest, sizeof(dest), second, sizeof(second));
    ok(ret == 0, "Expected _mbsnbcat_s to return 0, got %d\n", ret);
    ok(!memcmp(dest, "dinosaurduck", sizeof("dinosaurduck")),
       "Expected the output buffer string to be \"dinosaurduck\"\n");

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    ret = p_mbsnbcat_s(dest, sizeof(dest), second, sizeof(second) + 1);
    ok(ret == 0, "Expected _mbsnbcat_s to return 0, got %d\n", ret);
    ok(!memcmp(dest, "dinosaurduck", sizeof("dinosaurduck")),
       "Expected the output buffer string to be \"dinosaurduck\"\n");

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    ret = p_mbsnbcat_s(dest, sizeof(dest), second, sizeof(second) - 1);
    ok(ret == 0, "Expected _mbsnbcat_s to return 0, got %d\n", ret);
    ok(!memcmp(dest, "dinosaurduck", sizeof("dinosaurduck")),
       "Expected the output buffer string to be \"dinosaurduck\"\n");

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    ret = p_mbsnbcat_s(dest, sizeof(dest), second, sizeof(second) - 2);
    ok(ret == 0, "Expected _mbsnbcat_s to return 0, got %d\n", ret);
    ok(!memcmp(dest, "dinosaurduc", sizeof("dinosaurduc")),
       "Expected the output buffer string to be \"dinosaurduc\"\n");

    /* Test destination truncation behavior. */
    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    errno = EBADF;
    ret = p_mbsnbcat_s(dest, sizeof(first) - 1, second, sizeof(second));
    ok(ret == EINVAL, "Expected _mbsnbcat_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(!memcmp(dest, "\0inosaur", sizeof("\0inosaur") - 1),
       "Expected the output buffer string to be \"\\0inosaur\" without ending null terminator\n");

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    errno = EBADF;
    ret = p_mbsnbcat_s(dest, sizeof(first), second, sizeof(second));
    ok(ret == ERANGE, "Expected _mbsnbcat_s to return ERANGE, got %d\n", ret);
    ok(errno == ERANGE, "Expected errno to be ERANGE, got %d\n", errno);
    ok(!memcmp(dest, "\0inosaurd", sizeof("\0inosaurd") - 1),
       "Expected the output buffer string to be \"\\0inosaurd\" without ending null terminator\n");

    memset(dest, 'X', sizeof(dest));
    memcpy(dest, first, sizeof(first));
    errno = EBADF;
    ret = p_mbsnbcat_s(dest, sizeof(first) + 1, second, sizeof(second));
    ok(ret == ERANGE, "Expected _mbsnbcat_s to return ERANGE, got %d\n", ret);
    ok(errno == ERANGE, "Expected errno to be ERANGE, got %d\n", errno);
    ok(!memcmp(dest, "\0inosaurdu", sizeof("\0inosaurdu") - 1),
       "Expected the output buffer string to be \"\\0inosaurdu\" without ending null terminator\n");
}

static void test__mbsupr_s(void)
{
    errno_t ret;
    unsigned char buffer[20];
    int cp = _getmbcp();

    if (!p_mbsupr_s)
    {
        win_skip("Skipping _mbsupr_s tests\n");
        return;
    }

    errno = EBADF;
    ret = p_mbsupr_s(NULL, 0);
    ok(ret == 0, "Expected _mbsupr_s to return 0, got %d\n", ret);

    errno = EBADF;
    ret = p_mbsupr_s(NULL, sizeof(buffer));
    ok(ret == EINVAL, "Expected _mbsupr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_mbsupr_s(buffer, 0);
    ok(ret == EINVAL, "Expected _mbsupr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memcpy(buffer, "abcdefgh", sizeof("abcdefgh"));
    errno = EBADF;
    ret = p_mbsupr_s(buffer, sizeof("abcdefgh"));
    ok(ret == 0, "Expected _mbsupr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "ABCDEFGH", sizeof("ABCDEFGH")),
       "Expected the output buffer to be \"ABCDEFGH\", got \"%s\"\n",
       buffer);

    memcpy(buffer, "abcdefgh", sizeof("abcdefgh"));
    errno = EBADF;
    ret = p_mbsupr_s(buffer, sizeof(buffer));
    ok(ret == 0, "Expected _mbsupr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "ABCDEFGH", sizeof("ABCDEFGH")),
       "Expected the output buffer to be \"ABCDEFGH\", got \"%s\"\n",
       buffer);

    memcpy(buffer, "abcdefgh", sizeof("abcdefgh"));
    errno = EBADF;
    ret = p_mbsupr_s(buffer, sizeof("abcdefgh") - 1);
    ok(ret == EINVAL, "Expected _mbsupr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memcpy(buffer, "abcdefgh\0ijklmnop", sizeof("abcdefgh\0ijklmnop"));
    errno = EBADF;
    ret = p_mbsupr_s(buffer, sizeof(buffer));
    ok(ret == 0, "Expected _mbsupr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "ABCDEFGH\0ijklmnop", sizeof("ABCDEFGH\0ijklmnop")),
       "Expected the output buffer to be \"ABCDEFGH\\0ijklmnop\", got \"%s\"\n",
       buffer);

    _setmbcp(936);
    memcpy(buffer, "\xa2\xa1\xa2\xa2q", sizeof("\xa2\xa1\xa2\xa2q"));
    ret = p_mbsupr_s(buffer, sizeof(buffer));
    ok(ret == 0, "Expected _mbsupr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "\xa2\xf1\xa2\xf2Q", sizeof("\xa2\xf1\xa2\xf2Q")),
            "got %s\n", debugstr_a((char*)buffer));
    _setmbcp(cp);
}

static void test__mbslwr_s(void)
{
    errno_t ret;
    unsigned char buffer[20];
    int cp = _getmbcp();

    if (!p_mbslwr_s)
    {
        win_skip("Skipping _mbslwr_s tests\n");
        return;
    }

    errno = EBADF;
    ret = p_mbslwr_s(NULL, 0);
    ok(ret == 0, "Expected _mbslwr_s to return 0, got %d\n", ret);

    errno = EBADF;
    ret = p_mbslwr_s(NULL, sizeof(buffer));
    ok(ret == EINVAL, "Expected _mbslwr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_mbslwr_s(buffer, 0);
    ok(ret == EINVAL, "Expected _mbslwr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memcpy(buffer, "ABCDEFGH", sizeof("ABCDEFGH"));
    errno = EBADF;
    ret = p_mbslwr_s(buffer, sizeof("ABCDEFGH"));
    ok(ret == 0, "Expected _mbslwr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "abcdefgh", sizeof("abcdefgh")),
       "Expected the output buffer to be \"abcdefgh\", got \"%s\"\n",
       buffer);

    memcpy(buffer, "ABCDEFGH", sizeof("ABCDEFGH"));
    errno = EBADF;
    ret = p_mbslwr_s(buffer, sizeof(buffer));
    ok(ret == 0, "Expected _mbslwr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "abcdefgh", sizeof("abcdefgh")),
       "Expected the output buffer to be \"abcdefgh\", got \"%s\"\n",
       buffer);

    memcpy(buffer, "ABCDEFGH", sizeof("ABCDEFGH"));
    errno = EBADF;
    ret = p_mbslwr_s(buffer, sizeof("ABCDEFGH") - 1);
    ok(ret == EINVAL, "Expected _mbslwr_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memcpy(buffer, "ABCDEFGH\0IJKLMNOP", sizeof("ABCDEFGH\0IJKLMNOP"));
    errno = EBADF;
    ret = p_mbslwr_s(buffer, sizeof(buffer));
    ok(ret == 0, "Expected _mbslwr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "abcdefgh\0IJKLMNOP", sizeof("abcdefgh\0IJKLMNOP")),
       "Expected the output buffer to be \"abcdefgh\\0IJKLMNOP\", got \"%s\"\n",
       buffer);

    _setmbcp(936);
    memcpy(buffer, "\xa2\xf1\xa2\xf2Q", sizeof("\xa2\xf1\xa2\xf2Q"));
    ret = p_mbslwr_s(buffer, sizeof(buffer));
    ok(ret == 0, "Expected _mbsupr_s to return 0, got %d\n", ret);
    ok(!memcmp(buffer, "\xa2\xa1\xa2\xa2q", sizeof("\xa2\xa1\xa2\xa2q")),
            "got %s\n", debugstr_a((char*)buffer));
    _setmbcp(cp);
}

static void test__mbstok(void)
{
    const unsigned char delim[] = "t";

    char str[] = "!.!test";
    unsigned char *ret;

    strtok(str, "!");

    ret = _mbstok(NULL, delim);
    /* most versions of msvcrt use the same buffer for strtok and _mbstok */
    ok(!ret || broken((char*)ret==str+4),
            "_mbstok(NULL, \"t\") = %p, expected NULL (%p)\n", ret, str);

    ret = _mbstok(NULL, delim);
    ok(!ret, "_mbstok(NULL, \"t\") = %p, expected NULL\n", ret);
}

static void test__ultoa_s(void)
{
    errno_t ret;
    char buffer[33];

    if (!p_ultoa_s)
    {
        win_skip("Skipping _ultoa_s tests\n");
        return;
    }

    errno = EBADF;
    ret = p_ultoa_s(0, NULL, 0, 0);
    ok(ret == EINVAL, "Expected _ultoa_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_ultoa_s(0, buffer, 0, 0);
    ok(ret == EINVAL, "Expected _ultoa_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == 'X', "Expected the output buffer to be untouched\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_ultoa_s(0, buffer, sizeof(buffer), 0);
    ok(ret == EINVAL, "Expected _ultoa_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_ultoa_s(0, buffer, sizeof(buffer), 64);
    ok(ret == EINVAL, "Expected _ultoa_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_ultoa_s(12345678, buffer, 4, 10);
    ok(ret == ERANGE, "Expected _ultoa_s to return ERANGE, got %d\n", ret);
    ok(errno == ERANGE, "Expected errno to be ERANGE, got %d\n", errno);
    ok(!memcmp(buffer, "\000765", 4),
       "Expected the output buffer to be null terminated with truncated output\n");

    memset(buffer, 'X', sizeof(buffer));
    errno = EBADF;
    ret = p_ultoa_s(12345678, buffer, 8, 10);
    ok(ret == ERANGE, "Expected _ultoa_s to return ERANGE, got %d\n", ret);
    ok(errno == ERANGE, "Expected errno to be ERANGE, got %d\n", errno);
    ok(!memcmp(buffer, "\0007654321", 8),
       "Expected the output buffer to be null terminated with truncated output\n");

    ret = p_ultoa_s(12345678, buffer, 9, 10);
    ok(ret == 0, "Expected _ultoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "12345678"),
       "Expected output buffer string to be \"12345678\", got \"%s\"\n",
       buffer);

    ret = p_ultoa_s(43690, buffer, sizeof(buffer), 2);
    ok(ret == 0, "Expected _ultoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "1010101010101010"),
       "Expected output buffer string to be \"1010101010101010\", got \"%s\"\n",
       buffer);

    ret = p_ultoa_s(1092009, buffer, sizeof(buffer), 36);
    ok(ret == 0, "Expected _ultoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "nell"),
       "Expected output buffer string to be \"nell\", got \"%s\"\n",
       buffer);

    ret = p_ultoa_s(5704, buffer, sizeof(buffer), 18);
    ok(ret == 0, "Expected _ultoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "hag"),
       "Expected output buffer string to be \"hag\", got \"%s\"\n",
       buffer);
}

static void test_wctob(void)
{
    int ret, cp = _getmbcp();

    if(!p_wctob || !setlocale(LC_ALL, "chinese-traditional")) {
        win_skip("Skipping wctob tests\n");
        return;
    }

    ret = p_wctob(0x8141);
    ok(ret == EOF, "ret = %x\n", ret);

    ret = p_wctob(0x81);
    ok(ret == EOF, "ret = %x\n", ret);

    ret = p_wctob(0xe0);
    ok(ret == 0x61, "ret = %x\n", ret);

    _setmbcp(1250);
    ret = p_wctob(0x81);
    ok(ret == EOF, "ret = %x\n", ret);

    setlocale(LC_ALL, "C");
    ret = p_wctob(0x8141);
    ok(ret == EOF, "ret = %x\n", ret);

    ret = p_wctob(0x81);
    ok(ret == (signed char)0x81, "ret = %x\n", ret);

    ret = p_wctob(0x9f);
    ok(ret == (signed char)0x9f, "ret = %x\n", ret);

    ret = p_wctob(0xe0);
    ok(ret == (signed char)0xe0, "ret = %x\n", ret);

    _setmbcp(cp);
}

static void test_btowc(void)
{
    wint_t ret;
    int cp = _getmbcp();

    if(!p_btowc || !setlocale(LC_ALL, "chinese-traditional")) {
        win_skip("Skipping btowc tests\n");
        return;
    }

    ret = p_btowc(EOF);
    ok(ret == WEOF, "ret = %x\n", ret);

    ret = p_btowc(0x61);
    ok(ret == 0x61, "ret = %x\n", ret);

    ret = p_btowc(0x81);
    ok(ret == WEOF, "ret = %x\n", ret);

    ret = p_btowc(0xe0);
    ok(ret == WEOF, "ret = %x\n", ret);

    _setmbcp(1250);
    ret = p_btowc(0x61);
    ok(ret == 0x61, "ret = %x\n", ret);

    ret = p_btowc(0x81);
    ok(ret == WEOF, "ret = %x\n", ret);

    ret = p_btowc(0xe0);
    ok(ret == WEOF, "ret = %x\n", ret);

    if(!setlocale(LC_CTYPE, ".1250")) {
        win_skip("No codepage 1250 support\n");
        setlocale(LC_ALL, "C");
        _setmbcp(cp);
        return;
    }

    ret = p_btowc(0x61);
    ok(ret == 0x61, "ret = %x\n", ret);

    ret = p_btowc(0x81);
    ok(ret == 0x81, "ret = %x\n", ret);

    ret = p_btowc(0xe0);
    ok(ret == 0x155, "ret = %x\n", ret);

    ret = p_btowc(0x100);
    ok(ret == 0x00, "ret = %x\n", ret);

    ret = p_btowc(0x1e0);
    ok(ret == 0x155, "ret = %x\n", ret);

    setlocale(LC_ALL, "C");
    ret = p_btowc(0x61);
    ok(ret == 0x61, "ret = %x\n", ret);

    ret = p_btowc(0x81);
    ok(ret == 0x81, "ret = %x\n", ret);

    ret = p_btowc(0x9f);
    ok(ret == 0x9f, "ret = %x\n", ret);

    ret = p_btowc(0xe0);
    ok(ret == 0xe0, "ret = %x\n", ret);

    ret = p_btowc(0x100);
    ok(ret == 0x00, "ret = %x\n", ret);

    ret = p_btowc(0x1e0);
    ok(ret == 0xe0, "ret = %x\n", ret);

    _setmbcp(cp);
}

static void test_wctomb(void)
{
    mbstate_t state;
    unsigned char dst[10];
    size_t ret;
    int err;

    if(!p_wcrtomb || !setlocale(LC_ALL, "Japanese_Japan.932")) {
        win_skip("wcrtomb tests\n");
        return;
    }

    ret = p_wcrtomb(NULL, 0x3042, NULL);
    ok(ret == 2, "wcrtomb did not return 2\n");

    state = 1;
    dst[2] = 'a';
    ret = p_wcrtomb((char*)dst, 0x3042, &state);
    ok(ret == 2, "wcrtomb did not return 2\n");
    ok(state == 0, "state != 0\n");
    ok(dst[0] == 0x82, "dst[0] = %x, expected 0x82\n", dst[0]);
    ok(dst[1] == 0xa0, "dst[1] = %x, expected 0xa0\n", dst[1]);
    ok(dst[2] == 'a', "dst[2] != 'a'\n");

    ret = p_wcrtomb((char*)dst, 0x3043, NULL);
    ok(ret == 2, "wcrtomb did not return 2\n");
    ok(dst[0] == 0x82, "dst[0] = %x, expected 0x82\n", dst[0]);
    ok(dst[1] == 0xa1, "dst[1] = %x, expected 0xa1\n", dst[1]);

    ret = p_wcrtomb((char*)dst, 0x20, NULL);
    ok(ret == 1, "wcrtomb did not return 1\n");
    ok(dst[0] == 0x20, "dst[0] = %x, expected 0x20\n", dst[0]);

    ret = p_wcrtomb((char*)dst, 0xffff, NULL);
    ok(ret == -1, "wcrtomb did not return -1\n");
    ok(dst[0] == 0x3f, "dst[0] = %x, expected 0x3f\n", dst[0]);

    if(!p_wcrtomb_s) {
        win_skip("wcrtomb_s tests\n");
        setlocale(LC_ALL, "C");
        return;
    }

    state = 1;
    dst[2] = 'a';
    err = p_wcrtomb_s(&ret, (char*)dst, sizeof(dst), 0x3042, &state);
    ok(!err, "err = %d\n", err);
    ok(ret == 2, "ret != 2\n");
    ok(!state, "state != 0\n");
    ok(dst[0] == 0x82, "dst[0] = %x, expected 0x82\n", dst[0]);
    ok(dst[1] == 0xa0, "dst[1] = %x, expected 0xa0\n", dst[1]);
    ok(dst[2] == 'a', "dst[2] != 'a'\n");

    err = p_wcrtomb_s(&ret, (char*)dst, sizeof(dst), 0x3042, NULL);
    ok(!err, "err = %d\n", err);
    ok(ret == 2, "ret != 2\n");
    ok(!state, "state != 0\n");
    ok(dst[0] == 0x82, "dst[0] = %x, expected 0x82\n", dst[0]);
    ok(dst[1] == 0xa0, "dst[1] = %x, expected 0xa0\n", dst[1]);

    err = p_wcrtomb_s(&ret, (char*)dst, sizeof(dst), 0x20, NULL);
    ok(!err, "err = %d\n", err);
    ok(ret == 1, "ret != 1\n");
    ok(dst[0] == 0x20, "dst[0] = %x, expected 0x20\n", dst[0]);

    err = p_wcrtomb_s(&ret, NULL, 0, 0x20, NULL);
    ok(!err, "err = %d\n", err);
    ok(ret == 1, "ret != 1\n");

    err = p_wcrtomb_s(&ret, (char*)dst, sizeof(dst), 0xffff, NULL);
    ok(err == EILSEQ, "err = %d\n", err);
    ok(ret == -1, "wcrtomb did not return -1\n");
    ok(dst[0] == 0x3f, "dst[0] = %x, expected 0x3f\n", dst[0]);

    setlocale(LC_ALL, "C");
}

static void test_tolower(void)
{
    WCHAR chw, lower;
    char ch, lch;
    int ret, len;

    /* test C locale when locale was never changed */
    ret = p_tolower(0x41);
    ok(ret == 0x61, "ret = %x\n", ret);

    ret = p_tolower(0xF4);
    ok(ret == 0xF4, "ret = %x\n", ret);

    errno = 0xdeadbeef;
    ret = p_tolower((char)0xF4);
    ok(ret == (char)0xF4, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    ret = p_tolower((char)0xD0);
    ok(ret == (char)0xD0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    setlocale(LC_ALL, "C");
    errno = 0xdeadbeef;
    ret = p_tolower((char)0xF4);
    ok(ret == (char)0xF4, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    /* test C locale after setting locale */
    if(!setlocale(LC_ALL, "us")) {
        win_skip("skipping tolower tests that depends on locale\n");
        return;
    }
    setlocale(LC_ALL, "C");

    ch = 0xF4;
    errno = 0xdeadbeef;
    ret = p_tolower((signed char)ch);
    if(!MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, &ch, 1, &chw, 1) ||
            LCMapStringW(CP_ACP, LCMAP_LOWERCASE, &chw, 1, &lower, 1) != 1 ||
            (len = WideCharToMultiByte(CP_ACP, 0, &lower, 1, &lch, 1, NULL, NULL)) != 1)
        len = 0;
    if(len)
        ok(ret==(unsigned char)lch || broken(ret==ch)/*WinXP-*/, "ret = %x\n", ret);
    else
        ok(ret == ch, "ret = %x\n", ret);
    if(!len || ret==(unsigned char)lch)
        ok(errno == EILSEQ, "errno = %d\n", errno);

    ch = 0xD0;
    errno = 0xdeadbeef;
    ret = p_tolower((signed char)ch);
    if(!MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, &ch, 1, &chw, 1) ||
            LCMapStringW(CP_ACP, LCMAP_LOWERCASE, &chw, 1, &lower, 1) != 1 ||
            (len = WideCharToMultiByte(CP_ACP, 0, &lower, 1, &lch, 1, NULL, NULL)) != 1)
        len = 0;
    if(len)
        ok(ret==(unsigned char)lch || broken(ret==ch)/*WinXP-*/, "ret = %x\n", ret);
    else
        ok(ret == ch, "ret = %x\n", ret);
    if(!len || ret==(unsigned char)lch)
        ok(errno == EILSEQ, "errno = %d\n", errno);

    ret = p_tolower((unsigned char)0xD0);
    ok(ret == 0xD0, "ret = %x\n", ret);

    ok(setlocale(LC_ALL, "us") != NULL, "setlocale failed\n");

    ret = p_tolower((signed char)0xD0);
    ok(ret == 0xF0, "ret = %x\n", ret);

    ret = p_tolower((unsigned char)0xD0);
    ok(ret == 0xF0, "ret = %x\n", ret);

    skip_2k3_fail {
    ok(setlocale(LC_ALL, "Japanese_Japan.932") != NULL, "setlocale failed.\n");
    errno = 0xdeadbeef;
    ret = p_tolower((signed char)0xd0);
    ok(ret == 0xd0, "Got %#x.\n", ret);
    ok(errno == EILSEQ, "Got errno %d.\n", errno);
    errno = 0xdeadbeef;
    ret = p_tolower(0xd0);
    ok(ret == 0xd0, "Got %#x.\n", ret);
    ok(errno == 0xdeadbeef, "Got errno %d.\n", errno);

    ok(setlocale(LC_ALL, "Chinese_China.936") != NULL, "setlocale failed.\n");
    errno = 0xdeadbeef;
    ret = p_tolower((signed char)0xd0);
    ok(ret == (signed char)0xd0, "Got %#x.\n", ret);
    ok(errno == EILSEQ, "Got errno %d.\n", errno);
    errno = 0xdeadbeef;
    ret = p_tolower(0xd0);
    ok(ret == 0xd0, "Got %#x.\n", ret);
    ok(errno == 0xdeadbeef, "Got errno %d.\n", errno);

    ok(setlocale(LC_ALL, "Korean_Korea.949") != NULL, "setlocale failed.\n");
    errno = 0xdeadbeef;
    ret = p_tolower((signed char)0xd0);
    ok(ret == (signed char)0xd0, "Got %#x.\n", ret);
    ok(errno == EILSEQ, "Got errno %d.\n", errno);
    errno = 0xdeadbeef;
    ret = p_tolower(0xd0);
    ok(ret == 0xd0, "Got %#x.\n", ret);
    ok(errno == 0xdeadbeef, "Got errno %d.\n", errno);

    ok(setlocale(LC_ALL, "Chinese_Taiwan.950") != NULL, "setlocale failed.\n");
    errno = 0xdeadbeef;
    ret = p_tolower((signed char)0xd0);
    ok(ret == (signed char)0xd0, "Got %#x.\n", ret);
    ok(errno == EILSEQ, "Got errno %d.\n", errno);
    errno = 0xdeadbeef;
    ret = p_tolower(0xd0);
    ok(ret == 0xd0, "Got %#x.\n", ret);
    ok(errno == 0xdeadbeef, "Got errno %d.\n", errno);
    }

    setlocale(LC_ALL, "C");
}

static double mul_pow10(double x, double exp)
{
    int fpexcept = _EM_DENORMAL|_EM_INVALID|_EM_ZERODIVIDE|_EM_OVERFLOW|_EM_UNDERFLOW|_EM_INEXACT;
    BOOL negexp = (exp < 0);
    int fpcontrol;
    double ret;

    if(negexp)
        exp = -exp;
    fpcontrol = _control87(0, 0);
    _control87(fpexcept, 0xffffffff);
    ret = pow(10.0, exp);
    ret = (negexp ? x/ret : x*ret);
    _control87(fpcontrol, 0xffffffff);
    return ret;
}

static void test__atodbl(void)
{
    _CRT_DOUBLE d;
    char num[32];
    int i, j, ret;

    if(!p__atodbl_l) {
        /* Old versions of msvcrt use different values for _OVERFLOW and _UNDERFLOW
         * Because of this lets skip _atodbl tests when _atodbl_l is not available */
        win_skip("_atodbl_l is not available\n");
        return;
    }

    num[0] = 0;
    ret = p__atodbl_l(&d, num, NULL);
    ok(ret == 0, "_atodbl_l(&d, \"\", NULL) returned %d, expected 0\n", ret);
    ok(d.x == 0, "d.x = %lf, expected 0\n", d.x);
    ret = _atodbl(&d, num);
    ok(ret == 0, "_atodbl(&d, \"\") returned %d, expected 0\n", ret);
    ok(d.x == 0, "d.x = %lf, expected 0\n", d.x);

    strcpy(num, "t");
    ret = p__atodbl_l(&d, num, NULL);
    ok(ret == 0, "_atodbl_l(&d, \"t\", NULL) returned %d, expected 0\n", ret);
    ok(d.x == 0, "d.x = %lf, expected 0\n", d.x);
    ret = _atodbl(&d, num);
    ok(ret == 0, "_atodbl(&d, \"t\") returned %d, expected 0\n", ret);
    ok(d.x == 0, "d.x = %lf, expected 0\n", d.x);

    strcpy(num, "0");
    ret = p__atodbl_l(&d, num, NULL);
    ok(ret == 0, "_atodbl_l(&d, \"0\", NULL) returned %d, expected 0\n", ret);
    ok(d.x == 0, "d.x = %lf, expected 0\n", d.x);
    ret = _atodbl(&d, num);
    ok(ret == 0, "_atodbl(&d, \"0\") returned %d, expected 0\n", ret);
    ok(d.x == 0, "d.x = %lf, expected 0\n", d.x);

    strcpy(num, "123");
    ret = p__atodbl_l(&d, num, NULL);
    ok(ret == 0, "_atodbl_l(&d, \"123\", NULL) returned %d, expected 0\n", ret);
    ok(d.x == 123, "d.x = %lf, expected 123\n", d.x);
    ret = _atodbl(&d, num);
    ok(ret == 0, "_atodbl(&d, \"123\") returned %d, expected 0\n", ret);
    ok(d.x == 123, "d.x = %lf, expected 123\n", d.x);

    /* check over the whole range of (simple) normal doubles */
    for (j = DBL_MIN_10_EXP; j <= DBL_MAX_10_EXP; j++) {
        for (i = 1; i <= 9; i++) {
            double expected = mul_pow10(i, j);
            if (expected < DBL_MIN || expected > DBL_MAX) continue;
            snprintf(num, sizeof(num), "%de%d", i, j);
            ret = _atodbl(&d, num);
            ok(compare_double(d.x, expected, 2), "d.x = %.16e, expected %.16e\n", d.x, expected);
        }
    }

    /* check with denormal doubles */
    strcpy(num, "1e-309");
    ret = p__atodbl_l(&d, num, NULL);
    ok(ret == _UNDERFLOW, "_atodbl_l(&d, \"1e-309\", NULL) returned %d, expected _UNDERFLOW\n", ret);
    ok(compare_double(d.x, 1e-309, 1), "d.x = %.16e, expected 0\n", d.x);
    ret = _atodbl(&d, num);
    ok(ret == _UNDERFLOW, "_atodbl(&d, \"1e-309\") returned %d, expected _UNDERFLOW\n", ret);
    ok(compare_double(d.x, 1e-309, 1), "d.x = %.16e, expected 0\n", d.x);

    strcpy(num, "1e309");
    ret = p__atodbl_l(&d, num, NULL);
    ok(ret == _OVERFLOW, "_atodbl_l(&d, \"1e309\", NULL) returned %d, expected _OVERFLOW\n", ret);
    ret = _atodbl(&d, num);
    ok(ret == _OVERFLOW, "_atodbl(&d, \"1e309\") returned %d, expected _OVERFLOW\n", ret);
}

static void test__stricmp(void)
{
    int ret;

    ret = _stricmp("test", "test");
    ok(ret == 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("a", "z");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("z", "a");
    ok(ret > 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\xa5", "\xb9");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\xa5\xa1", "\xb9\xa1"); /* valid gbk characters */
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("abc\xa5\xa1", "abc");
    ok(ret > 0, "_stricmp returned %d\n", ret);

    if(!setlocale(LC_ALL, "polish")) {
        win_skip("stricmp tests\n");
        return;
    }

    ret = _stricmp("test", "test");
    ok(ret == 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("a", "z");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("z", "a");
    ok(ret > 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\xa5", "\xb9");
    ok(ret == 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("a", "\xb9");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\xa5\xa1", "\xb9\xa1"); /* valid gbk characters */
    ok(ret == 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("abc\xa5\xa1", "abc");
    ok(ret > 0, "_stricmp returned %d\n", ret);

    ok(setlocale(LC_ALL, ".936") != NULL, "setlocale failed.\n");
    ret = _stricmp("test", "test");
    ok(ret == 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("a", "z");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("z", "a");
    ok(ret > 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\xa5", "\xb9");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("a", "\xb9");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\xa5\xa1", "\xb9\xa1"); /* valid gbk characters */
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\x82\xa0", "\x83\x41"); /* valid shift-jis characters */
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\x81\x00", "\x81\x01"); /* invalid for gbk and shift-jis */
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("abc\xa5\xa1", "abc");
    ok(ret > 0, "_stricmp returned %d\n", ret);

    skip_2k3_fail ok(setlocale(LC_ALL, "Japanese_Japan.932") != NULL, "setlocale failed.\n");
    ret = _stricmp("test", "test");
    ok(ret == 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("a", "z");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("z", "a");
    ok(ret > 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\xa5", "\xb9");
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\xa5\xa1", "\xb9\xa1"); /* valid gbk characters */
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\x82\xa0", "\x83\x41"); /* valid shift-jis characters */
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("\x81\x00", "\x81\x01"); /* invalid for gbk and shift-jis */
    ok(ret < 0, "_stricmp returned %d\n", ret);
    ret = _stricmp("abc\x82\xa0", "abc");
    ok(ret > 0, "_stricmp returned %d\n", ret);

    setlocale(LC_ALL, "C");
}

static void test__wcstoi64(void)
{
    static const struct { WCHAR str[24]; __int64 res; unsigned __int64 ures; int base; } tests[] =
    {
        { L"9", 9, 9, 10 },
        { L" ", 0, 0 },
        { L"-1234", -1234, -1234 },
        { L"\x09\x0a\x0b\x0c\x0d -123", -123, -123 },
        { L"\xa0\x2002\x2003\x2028\x3000 +44", 44, 44 },
        { { 0x3231 }, 0, 0 }, /* PARENTHESIZED IDEOGRAPH STOCK */
        { { 0x4e00 }, 0, 0 }, /* CJK Ideograph, First */
        { { 0x0bef }, 0, 0 }, /* TAMIL DIGIT NINE */
        { { 0x0e59 }, 9, 9 }, /* THAI DIGIT NINE */
        { { 0xff19 }, 9, 9 }, /* FULLWIDTH DIGIT NINE */
        { { 0x00b9 }, 0, 0 }, /* SUPERSCRIPT ONE */
        { { '-',0x0e50,'x',0xff19,'1' }, -0x91, -0x91 },
        { { '+',0x0e50,0xff17,'1' }, 071, 071 },
        { { 0xff19,'f',0x0e59,0xff46 }, 0x9f9, 0x9f9, 16 },
        { L"4294967295", 4294967295, 4294967295 },
        { L"4294967296", 4294967296, 4294967296 },
        { L"9223372036854775807", 9223372036854775807, 9223372036854775807 },
        { L"9223372036854775808", _I64_MAX, 9223372036854775808u },
        { L"18446744073709551615", _I64_MAX, _UI64_MAX },
        { L"18446744073709551616", _I64_MAX, _UI64_MAX },
        { L"-4294967295", -4294967295, -4294967295 },
        { L"-4294967296", -4294967296, -4294967296 },
        { L"-9223372036854775807", -9223372036854775807, -9223372036854775807 },
        { L"-9223372036854775808", _I64_MIN, 9223372036854775808u },
        { L"-18446744073709551615", _I64_MIN, 1 },
        { L"-18446744073709551616", _I64_MIN, 1 },
    };
    static const WCHAR zeros[] = {
        0x660, 0x6f0, 0x966, 0x9e6, 0xa66, 0xae6, 0xb66, 0xc66, 0xce6,
        0xd66, 0xe50, 0xed0, 0xf20, 0x1040, 0x17e0, 0x1810, 0xff10
    };
    int i;

    __int64 res;
    unsigned __int64 ures;
    WCHAR *endpos;

    if (!p_wcstoi64 || !p_wcstoui64) {
        win_skip("_wcstoi64 or _wcstoui64 not found\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        if ((i == 20) && (_winver < 0x600))
        {
            skip("Skipping test with i = 20, because it fails on Windows 2003\n");
            continue;
        }
        res = p_wcstoi64( tests[i].str, &endpos, tests[i].base );
        ok( res == tests[i].res, "%u: %s res %s\n",
            i, wine_dbgstr_w(tests[i].str), wine_dbgstr_longlong(res) );
        if (!res) ok( endpos == tests[i].str, "%u: wrong endpos %p/%p\n", i, endpos, tests[i].str );
        ures = p_wcstoui64( tests[i].str, &endpos, tests[i].base );
        ok( ures == tests[i].ures, "%u: %s res %s\n",
            i, wine_dbgstr_w(tests[i].str), wine_dbgstr_longlong(ures) );
    }

    /* Test various unicode digits */
    for (i = 0; i < ARRAY_SIZE(zeros); ++i) {
        WCHAR tmp[] = {zeros[i] + 4, zeros[i], zeros[i] + 5, 0};
        res = p_wcstoi64(tmp, NULL, 0);
        ok(res == 405, "with zero = U+%04X: got %d, expected 405\n", zeros[i], (int)res);
        tmp[1] = zeros[i] + 10;
        res = p_wcstoi64(tmp, NULL, 16);
        ok(res == 4, "with zero = U+%04X: got %d, expected 4\n", zeros[i], (int)res);
    }
}

static void test__wcstol(void)
{
    static const struct { WCHAR str[24]; long res; unsigned long ures; int base; } tests[] =
    {
        { L"9", 9, 9, 10 },
        { L" ", 0, 0 },
        { L"-1234", -1234, -1234 },
        { L"\x09\x0a\x0b\x0c\x0d -123", -123, -123 },
        { L"\xa0\x2002\x2003\x2028\x3000 +44", 44, 44 },
        { { 0x3231 }, 0, 0 }, /* PARENTHESIZED IDEOGRAPH STOCK */
        { { 0x4e00 }, 0, 0 }, /* CJK Ideograph, First */
        { { 0x0bef }, 0, 0 }, /* TAMIL DIGIT NINE */
        { { 0x0e59 }, 9, 9 }, /* THAI DIGIT NINE */
        { { 0xff19 }, 9, 9 }, /* FULLWIDTH DIGIT NINE */
        { { 0x00b9 }, 0, 0 }, /* SUPERSCRIPT ONE */
        { { '-',0x0e50,'x',0xff19,'1' }, -0x91, -0x91 },
        { { '+',0x0e50,0xff17,'1' }, 071, 071 },
        { { 0xff19,'f',0x0e59,0xff46 }, 0x9f9, 0x9f9, 16 },
        { L"2147483647", 2147483647, 2147483647 },
        { L"2147483648", LONG_MAX, 2147483648 },
        { L"4294967295", LONG_MAX, 4294967295 },
        { L"4294967296", LONG_MAX, ULONG_MAX },
        { L"9223372036854775807", LONG_MAX, ULONG_MAX },
        { L"-2147483647", -2147483647, -2147483647 },
        { L"-2147483648", LONG_MIN, LONG_MIN },
        { L"-4294967295", LONG_MIN, 1 },
        { L"-4294967296", LONG_MIN, 1 },
        { L"-9223372036854775807", LONG_MIN, 1 },
    };
    static const WCHAR zeros[] = {
        0x660, 0x6f0, 0x966, 0x9e6, 0xa66, 0xae6, 0xb66, 0xc66, 0xce6,
        0xd66, 0xe50, 0xed0, 0xf20, 0x1040, 0x17e0, 0x1810, 0xff10
    };
    int i;

    long res;
    unsigned long ures;
    WCHAR *endpos;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        res = wcstol( tests[i].str, &endpos, tests[i].base );
        ok( res == tests[i].res, "%u: %s res %08lx\n",
            i, wine_dbgstr_w(tests[i].str), res );
        if (!res) ok( endpos == tests[i].str, "%u: wrong endpos %p/%p\n", i, endpos, tests[i].str );
        ures = wcstoul( tests[i].str, &endpos, tests[i].base );
        ok( ures == tests[i].ures, "%u: %s res %08lx\n",
            i, wine_dbgstr_w(tests[i].str), ures );
    }

    /* Test various unicode digits */
    for (i = 0; i < ARRAY_SIZE(zeros); ++i) {
        WCHAR tmp[] = {zeros[i] + 4, zeros[i], zeros[i] + 5, 0};
        res = wcstol(tmp, NULL, 0);
        ok(res == 405, "with zero = U+%04X: got %d, expected 405\n", zeros[i], (int)res);
        tmp[1] = zeros[i] + 10;
        res = wcstol(tmp, NULL, 16);
        ok(res == 4, "with zero = U+%04X: got %d, expected 4\n", zeros[i], (int)res);
    }
}

static void test_atoi(void)
{
    int r;

    r = atoi("0");
    ok(r == 0, "atoi(0) = %d\n", r);

    r = atoi("-1");
    ok(r == -1, "atoi(-1) = %d\n", r);

    r = atoi("1");
    ok(r == 1, "atoi(1) = %d\n", r);

    r = atoi("4294967296");
    ok(r == 0, "atoi(4294967296) = %d\n", r);
}

static void test_atol(void)
{
    int r;

    r = atol("0");
    ok(r == 0, "atol(0) = %d\n", r);

    r = atol("-1");
    ok(r == -1, "atol(-1) = %d\n", r);

    r = atol("1");
    ok(r == 1, "atol(1) = %d\n", r);

    r = atol("4294967296");
    ok(r == 0, "atol(4294967296) = %d\n", r);
}

static void test_atof(void)
{
    double d;

    d = atof("0.0");
    ok(d == 0.0, "d = %lf\n", d);

    d = atof("1.0");
    ok(d == 1.0, "d = %lf\n", d);

    d = atof("-1.0");
    ok(d == -1.0, "d = %lf\n", d);

    if (!p__atof_l)
    {
        win_skip("_atof_l not found\n");
        return;
    }

    errno = EBADF;
    d = atof(NULL);
    ok(d == 0.0, "d = %lf\n", d);
    ok(errno == EINVAL, "errno = %x\n", errno);

    errno = EBADF;
    d = p__atof_l(NULL, NULL);
    ok(d == 0.0, "d = %lf\n", d);
    ok(errno == EINVAL, "errno = %x\n", errno);
}

static void test_strncpy(void)
{
#define TEST_STRNCPY_LEN 10
    /* use function pointer to bypass gcc builtin */
    char *(__cdecl *p_strncpy)(char*,const char*,size_t);
    char *ret;
    char dst[TEST_STRNCPY_LEN + 1];
    char not_null_terminated[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};

    p_strncpy = (void *)GetProcAddress( GetModuleHandleA("msvcrt.dll"), "strncpy");

    /* strlen(src) > TEST_STRNCPY_LEN */
    ret = p_strncpy(dst, "01234567890123456789", TEST_STRNCPY_LEN);
    ok(ret == dst, "ret != dst\n");
    ok(!strncmp(dst, "0123456789", TEST_STRNCPY_LEN), "dst != 0123456789\n");

    /* without null-terminated */
    ret = p_strncpy(dst, not_null_terminated, TEST_STRNCPY_LEN);
    ok(ret == dst, "ret != dst\n");
    ok(!strncmp(dst, "0123456789", TEST_STRNCPY_LEN), "dst != 0123456789\n");

    /* strlen(src) < TEST_STRNCPY_LEN */
    strcpy(dst, "0123456789");
    ret = p_strncpy(dst, "012345", TEST_STRNCPY_LEN);
    ok(ret == dst, "ret != dst\n");
    ok(!strcmp(dst, "012345"), "dst != 012345\n");
    ok(dst[TEST_STRNCPY_LEN - 1] == '\0', "dst[TEST_STRNCPY_LEN - 1] != 0\n");

    /* strlen(src) == TEST_STRNCPY_LEN */
    ret = p_strncpy(dst, "0123456789", TEST_STRNCPY_LEN);
    ok(ret == dst, "ret != dst\n");
    ok(!strncmp(dst, "0123456789", TEST_STRNCPY_LEN), "dst != 0123456789\n");
}

static void test_strxfrm(void)
{
    char dest[256];
    size_t ret;

    /* crashes on old version of msvcrt */
    if(p__atodbl_l) {
        errno = 0xdeadbeef;
        ret = strxfrm(NULL, "src", 1);
        ok(ret == INT_MAX, "ret = %d\n", (int)ret);
        ok(errno == EINVAL, "errno = %d\n", errno);

        errno = 0xdeadbeef;
        ret = strxfrm(dest, NULL, 100);
        ok(ret == INT_MAX, "ret = %d\n", (int)ret);
        ok(errno == EINVAL, "errno = %d\n", errno);
    }

    ret = strxfrm(NULL, "src", 0);
    ok(ret == 3, "ret = %d\n", (int)ret);
    dest[0] = 'a';
    ret = strxfrm(dest, "src", 0);
    ok(ret == 3, "ret = %d\n", (int)ret);
    ok(dest[0] == 'a', "dest[0] = %d\n", dest[0]);

    dest[3] = 'a';
    ret = strxfrm(dest, "src", 5);
    ok(ret == 3, "ret = %d\n", (int)ret);
    ok(!strcmp(dest, "src"), "dest = %s\n", dest);

    errno = 0xdeadbeef;
    dest[1] = 'a';
    ret = strxfrm(dest, "src", 1);
    ok(ret == 3, "ret = %d\n", (int)ret);
    ok(dest[0] == 's', "dest[0] = %d\n", dest[0]);
    ok(dest[1] == 'a', "dest[1] = %d\n", dest[1]);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    ret = strxfrm(dest, "", 5);
    ok(ret == 0, "ret = %d\n", (int)ret);
    ok(!dest[0], "dest[0] = %d\n", dest[0]);

    if(!setlocale(LC_ALL, "polish")) {
        win_skip("stxfrm tests\n");
        return;
    }

    ret = strxfrm(NULL, "src", 0);
    ok(ret < sizeof(dest)-1, "ret = %d\n", (int)ret);
    dest[0] = 'a';
    ret = strxfrm(dest, "src", 0);
    ok(ret < sizeof(dest)-1, "ret = %d\n", (int)ret);
    ok(dest[0] == 'a', "dest[0] = %d\n", dest[0]);

    ret = strxfrm(dest, "src", ret+1);
    ok(ret < sizeof(dest)-1, "ret = %d\n", (int)ret);
    ok(dest[0], "dest[0] = 0\n");

    errno = 0xdeadbeef;
    dest[0] = 'a';
    ret = strxfrm(dest, "src", 5);
    ok(ret>5 && ret<sizeof(dest)-1, "ret = %d\n", (int)ret);
    ok(!dest[0] || broken(!p__atodbl_l && dest[0]=='a'), "dest[0] = %d\n", dest[0]);

    setlocale(LC_ALL, "C");
}

static void test__strnset_s(void)
{
    char buf[5] = {0};
    int r;

    if(!p__strnset_s) {
        win_skip("_strnset_s not available\n");
        return;
    }

    r = p__strnset_s(NULL, 0, 'a', 0);
    ok(r == 0, "r = %d\n", r);

    buf[0] = buf[1] = buf[2] = 'b';
    r = p__strnset_s(buf, sizeof(buf), 'a', 2);
    ok(r == 0, "r = %d\n", r);
    ok(!strcmp(buf, "aab"), "buf = %s\n", buf);

    r = p__strnset_s(buf, 0, 'a', 0);
    ok(r == EINVAL, "r = %d\n", r);

    r = p__strnset_s(NULL, 0, 'a', 1);
    ok(r == EINVAL, "r = %d\n", r);

    buf[3] = 'b';
    r = p__strnset_s(buf, sizeof(buf)-1, 'c', 2);
    ok(r == EINVAL, "r = %d\n", r);
    ok(!buf[0] && buf[1]=='c' && buf[2]=='b', "buf = %s\n", buf);
}

static void test__wcsnset_s(void)
{
    wchar_t text[] = L"text";
    int r;

    if(!p__wcsnset_s) {
        win_skip("_wcsnset_s not available\n");
        return;
    }

    r = p__wcsnset_s(NULL, 0, 'a', 0);
    ok(r == 0, "r = %d\n", r);

    r = p__wcsnset_s(text, 0, 'a', 1);
    ok(r == EINVAL, "r = %d\n", r);
    ok(text[0] == 't', "text[0] = %d\n", text[0]);

    r = p__wcsnset_s(NULL, 2, 'a', 1);
    ok(r == EINVAL, "r = %d\n", r);

    r = p__wcsnset_s(text, 2, 'a', 3);
    ok(r == EINVAL, "r = %d\n", r);
    ok(text[0] == 0, "text[0] = %d\n", text[0]);
    ok(text[1] == 'e', "text[1] = %d\n", text[1]);

    text[0] = 't';
    r = p__wcsnset_s(text, 5, 'a', 1);
    ok(r == 0, "r = %d\n", r);
    ok(text[0] == 'a', "text[0] = %d\n", text[0]);
    ok(text[1] == 'e', "text[1] = %d\n", text[1]);

    text[1] = 0;
    r = p__wcsnset_s(text, 5, 'b', 3);
    ok(r == 0, "r = %d\n", r);
    ok(text[0] == 'b', "text[0] = %d\n", text[0]);
    ok(text[1] == 0, "text[1] = %d\n", text[1]);
    ok(text[2] == 'x', "text[2] = %d\n", text[2]);
}

static void test__wcsset_s(void)
{
    wchar_t str[10];
    int r;

    if(!p__wcsset_s) {
        win_skip("_wcsset_s not available\n");
        return;
    }

    r = p__wcsset_s(NULL, 0, 'a');
    ok(r == EINVAL, "r = %d\n", r);

    str[0] = 'a';
    r = p__wcsset_s(str, 0, 'a');
    ok(r == EINVAL, "r = %d\n", r);
    ok(str[0] == 'a', "str[0] = %d\n", str[0]);

    str[0] = 'a';
    str[1] = 'b';
    r = p__wcsset_s(str, 2, 'c');
    ok(r == EINVAL, "r = %d\n", r);
    ok(!str[0], "str[0] = %d\n", str[0]);
    ok(str[1] == 'b', "str[1] = %d\n", str[1]);

    str[0] = 'a';
    str[1] = 0;
    str[2] = 'b';
    r = p__wcsset_s(str, 3, 'c');
    ok(r == 0, "r = %d\n", r);
    ok(str[0] == 'c', "str[0] = %d\n", str[0]);
    ok(str[1] == 0, "str[1] = %d\n", str[1]);
    ok(str[2] == 'b', "str[2] = %d\n", str[2]);
}

static void test__mbscmp(void)
{
    static const unsigned char a[] = {'a',0}, b[] = {'b',0};
    int ret;

    if (!p_mbrlen)
    {
        win_skip("_mbscmp tests\n");
        return;
    }

    ret = _mbscmp(NULL, NULL);
    ok(ret == INT_MAX, "got %d\n", ret);

    ret = _mbscmp(a, NULL);
    ok(ret == INT_MAX, "got %d\n", ret);

    ret = _mbscmp(NULL, a);
    ok(ret == INT_MAX, "got %d\n", ret);

    ret = _mbscmp(a, a);
    ok(!ret, "got %d\n", ret);

    ret = _mbscmp(a, b);
    ok(ret == -1, "got %d\n", ret);

    ret = _mbscmp(b, a);
    ok(ret == 1, "got %d\n", ret);

    if (!p_mbscmp_l)
    {
        win_skip("_mbscmp_l tests\n");
        return;
    }

    ret = p_mbscmp_l(a, b, NULL);
    ok(ret == -1, "got %d\n", ret);
}

static void test__ismbclx(void)
{
    int ret, cp = _getmbcp();

    ret = _ismbcl0(0);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl1(0);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl2(0);
    ok(!ret, "got %d\n", ret);

    _setmbcp(1252);

    ret = _ismbcl0(0x8140);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl1(0x889f);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl2(0x989f);
    ok(!ret, "got %d\n", ret);

    _setmbcp(932);

    ret = _ismbcl0(0);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl0(0x8140);
    ok(ret, "got %d\n", ret);

    ret = _ismbcl0(0x817f);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl1(0);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl1(0x889f);
    ok(ret, "got %d\n", ret);

    ret = _ismbcl1(0x88fd);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl2(0);
    ok(!ret, "got %d\n", ret);

    ret = _ismbcl2(0x989f);
    ok(ret, "got %d\n", ret);

    ret = _ismbcl2(0x993f);
    ok(!ret, "got %d\n", ret);

    _setmbcp(cp);
}

static void test__memicmp(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ret = p__memicmp(NULL, NULL, 0);
    ok(!ret, "got %d\n", ret);

    ret = p__memicmp(s1, s2, 2);
    ok(!ret, "got %d\n", ret);

    ret = p__memicmp(s1, s2, 3);
    ok(ret == -1, "got %d\n", ret);

    if (!p__memicmp_l)
        return;

    /* Following calls crash on WinXP/W2k3. */
    errno = 0xdeadbeef;
    ret = p__memicmp(NULL, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);

    errno = 0xdeadbeef;
    ret = p__memicmp(s1, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);

    errno = 0xdeadbeef;
    ret = p__memicmp(NULL, s2, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);
}

static void test__memicmp_l(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    if (!p__memicmp_l)
    {
        win_skip("_memicmp_l not found.\n");
        return;
    }

    errno = 0xdeadbeef;
    ret = p__memicmp_l(NULL, NULL, 0, NULL);
    ok(!ret, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "errno is %d, expected 0xdeadbeef\n", errno);

    errno = 0xdeadbeef;
    ret = p__memicmp_l(NULL, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);

    errno = 0xdeadbeef;
    ret = p__memicmp_l(s1, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);

    errno = 0xdeadbeef;
    ret = p__memicmp_l(NULL, s2, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == EINVAL, "errno is %d, expected EINVAL\n", errno);

    errno = 0xdeadbeef;
    ret = p__memicmp_l(s1, s2, 2, NULL);
    ok(!ret, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "errno is %d, expected 0xdeadbeef\n", errno);

    errno = 0xdeadbeef;
    ret = p__memicmp_l(s1, s2, 3, NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "errno is %d, expected 0xdeadbeef\n", errno);
}

static void test__strupr(void)
{
    const char str[] = "123";
    const char *const_p;
    char str2[4];
    char *mem, *p;
    DWORD prot;

    mem = VirtualAlloc(NULL, sizeof(str), MEM_COMMIT, PAGE_READWRITE);
    ok(mem != NULL, "VirtualAlloc failed\n");
    memcpy(mem, str, sizeof(str));
    ok(VirtualProtect(mem, sizeof(str), PAGE_READONLY, &prot), "VirtualProtect failed\n");

    strcpy(str2, "aBc");
    p = _strupr(str2);
    ok(p == str2, "_strupr returned %p\n", p);
    ok(!strcmp(str2, "ABC"), "str2 = %s\n", str2);

    p = _strupr(mem);
    ok(p == mem, "_strupr returned %p\n", p);
    ok(!strcmp(mem, "123"), "mem = %s\n", mem);

    const_p = "ALREADY_UPPERCASE";
    p = _strupr((char *)const_p);
    ok(p == const_p, "_strupr returned %p\n", p);

    if(!setlocale(LC_ALL, "english")) {
        VirtualFree(mem, sizeof(str), MEM_RELEASE);
        win_skip("English locale _strupr tests\n");
        return;
    }

    strcpy(str2, "aBc");
    p = _strupr(str2);
    ok(p == str2, "_strupr returned %p\n", p);
    ok(!strcmp(str2, "ABC"), "str2 = %s\n", str2);

    if (0) /* crashes on Windows */
    {
        p = _strupr(mem);
        ok(p == mem, "_strupr returned %p\n", p);
        ok(!strcmp(mem, "123"), "mem = %s\n", mem);
    }

    setlocale(LC_ALL, "C");
    VirtualFree(mem, sizeof(str), MEM_RELEASE);
}

static void test__tcsncoll(void)
{
    struct test {
        const char *locale;
        const char *str1;
        const char *str2;
        size_t count;
        int exp;
    };
    static const struct test tests[] = {
        { "English", "ABCD", "ABCD",  4,  0 },
        { "English", "ABCD", "ABCD", 10,  0 },

        { "English", "ABC",  "ABCD",  3,  0 },
        { "English", "ABC",  "ABCD",  4, -1 },
        { "English", "ABC",  "ABCD", 10, -1 },

        { "English", "ABCD",  "ABC",  3,  0 },
        { "English", "ABCD",  "ABC",  4,  1 },
        { "English", "ABCD",  "ABC", 10,  1 },

        { "English", "ABCe", "ABCf",  3,  0 },
        { "English", "abcd", "ABCD", 10, -1 },

        { "English", "AB D", "AB-D",  4,  1 },
        { "English", "AB D", "AB'D",  4,  1 },

        { "C",       "ABCD", "ABCD",  4,  0 },
        { "C",       "ABCD", "ABCD", 10,  0 },

        { "C",       "ABC",  "ABCD",  3,  0 },
        { "C",       "ABC",  "ABCD", 10, -1 },

        { "C",       "ABCD",  "ABC",  3,  0 },
        { "C",       "ABCD",  "ABC", 10,  1 },

        { "C",       "ABCe", "ABCf",  3,  0 },
        { "C",       "abcd", "ABCD", 10,  1 },

        { "C",       "AB D", "AB-D",  4,  -1 },
        { "C",       "AB D", "AB'D",  4,  -1 },
    };
    WCHAR str1W[16];
    WCHAR str2W[16];
    char str1[16];
    char str2[16];
    size_t len;
    int i, ret;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        if (!setlocale(LC_ALL, tests[i].locale))
        {
            win_skip("%s locale _tcsncoll tests\n", tests[i].locale);
            for (; i+1 < ARRAY_SIZE(tests); i++)
                if (strcmp(tests[i].locale, tests[i+1].locale)) break;
            continue;
        }

        memset(str1, 0xee, sizeof(str1));
        strcpy(str1, tests[i].str1);

        memset(str2, 0xff, sizeof(str2));
        strcpy(str2, tests[i].str2);

        ret = _strncoll(str1, str2, tests[i].count);
        if (!tests[i].exp)
            ok(!ret, "expected 0, got %d for %s, %s, %d for locale %s\n",
               ret, str1, str2, (int)tests[i].count, tests[i].locale);
        else if (tests[i].exp < 0)
            ok(ret < 0, "expected < 0, got %d for %s, %s, %d for locale %s\n",
               ret, str1, str2, (int)tests[i].count, tests[i].locale);
        else
            ok(ret > 0, "expected > 0, got %d for %s, %s, %d for locale %s\n",
               ret, str1, str2, (int)tests[i].count, tests[i].locale);

        memset(str1W, 0xee, sizeof(str1W));
        len = mbstowcs(str1W, str1, ARRAY_SIZE(str1W));
        str1W[len] = 0;

        memset(str2W, 0xff, sizeof(str2W));
        len = mbstowcs(str2W, str2, ARRAY_SIZE(str2W));
        str2W[len] = 0;

        ret = _wcsncoll(str1W, str2W, tests[i].count);
        if (!tests[i].exp)
            ok(!ret, "expected 0, got %d for %s, %s, %d for locale %s\n",
               ret, str1, str2, (int)tests[i].count, tests[i].locale);
        else if (tests[i].exp < 0)
            ok(ret < 0, "expected < 0, got %d for %s, %s, %d for locale %s\n",
               ret, str1, str2, (int)tests[i].count, tests[i].locale);
        else
            ok(ret > 0, "expected > 0, got %d for %s, %s, %d for locale %s\n",
               ret, str1, str2, (int)tests[i].count, tests[i].locale);
    }
}

static void test__tcscoll(void)
{
    struct test {
        const char *locale;
        const char *str1;
        const char *str2;
        int exp;
    };
    static const struct test tests[] = {
        { "English", "ABCD", "ABCD",  0 },
        { "English", "ABC",  "ABCD", -1 },
        { "English", "ABCD",  "ABC",  1 },
        { "English", "ABCe", "ABCf", -1 },
        { "English", "abcd", "ABCD", -1 },
        { "English", "AB D", "AB-D",  1 },
        { "English", "AB D", "AB'D",  1 },

        { "C",       "ABCD", "ABCD",  0 },
        { "C",       "ABC",  "ABCD", -1 },
        { "C",       "ABCD",  "ABC",  1 },
        { "C",       "ABCe", "ABCf", -1 },
        { "C",       "abcd", "ABCD",  1 },
        { "C",       "AB D", "AB-D", -1 },
        { "C",       "AB D", "AB'D", -1 },
    };
    WCHAR str1W[16];
    WCHAR str2W[16];
    char str1[16];
    char str2[16];
    size_t len;
    int i, ret;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        if (!setlocale(LC_ALL, tests[i].locale))
        {
            win_skip("%s locale _tcsncoll tests\n", tests[i].locale);
            for (; i+1 < ARRAY_SIZE(tests); i++)
                if (strcmp(tests[i].locale, tests[i+1].locale)) break;
            continue;
        }

        memset(str1, 0xee, sizeof(str1));
        strcpy(str1, tests[i].str1);

        memset(str2, 0xff, sizeof(str2));
        strcpy(str2, tests[i].str2);

        ret = strcoll(str1, str2);
        if (!tests[i].exp)
            ok(!ret, "expected 0, got %d for %s, %s for locale %s\n",
               ret, str1, str2, tests[i].locale);
        else if (tests[i].exp < 0)
            ok(ret < 0, "expected < 0, got %d for %s, %s for locale %s\n",
               ret, str1, str2, tests[i].locale);
        else
            ok(ret > 0, "expected > 0, got %d for %s, %s for locale %s\n",
               ret, str1, str2, tests[i].locale);

        memset(str1W, 0xee, sizeof(str1W));
        len = mbstowcs(str1W, str1, ARRAY_SIZE(str1W));
        str1W[len] = 0;

        memset(str2W, 0xff, sizeof(str2W));
        len = mbstowcs(str2W, str2, ARRAY_SIZE(str2W));
        str2W[len] = 0;

        ret = wcscoll(str1W, str2W);
        if (!tests[i].exp)
            ok(!ret, "expected 0, got %d for %s, %s for locale %s\n",
               ret, str1, str2, tests[i].locale);
        else if (tests[i].exp < 0)
            ok(ret < 0, "expected < 0, got %d for %s, %s for locale %s\n",
               ret, str1, str2, tests[i].locale);
        else
            ok(ret > 0, "expected > 0, got %d for %s, %s for locale %s\n",
               ret, str1, str2, tests[i].locale);
    }
}

static void test__tcsnicoll(void)
{
    struct test {
        const char *locale;
        const char *str1;
        const char *str2;
        size_t count;
        int exp;
    };
    static const struct test tests[] = {
        { "English", "abcd", "ABCD",  4,  0 },
        { "English", "abcd", "ABCD", 10,  0 },

        { "English", "abc",  "ABCD",  3,  0 },
        { "English", "abc",  "ABCD",  4, -1 },
        { "English", "abc",  "ABCD", 10, -1 },

        { "English", "abcd",  "ABC",  3,  0 },
        { "English", "abcd",  "ABC",  4,  1 },
        { "English", "abcd",  "ABC", 10,  1 },

        { "English", "abcE", "ABCF",  3,  0 },

        { "C",       "abcd", "ABCD",  4,  0 },
        { "C",       "abcd", "ABCD", 10,  0 },

        { "C",       "abc",  "ABCD",  3,  0 },
        { "C",       "abc",  "ABCD", 10, -1 },

        { "C",       "abcd",  "ABC",  3,  0 },
        { "C",       "abcd",  "ABC", 10,  1 },

        { "C",       "abce", "ABCf",  3,  0 },
    };
    WCHAR str1W[16];
    WCHAR str2W[16];
    char str1[16];
    char str2[16];
    size_t len;
    int i, ret;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        if (!setlocale(LC_ALL, tests[i].locale))
        {
            win_skip("%s locale _tcsnicoll tests\n", tests[i].locale);
            for (; i+1 < ARRAY_SIZE(tests); i++)
                if (strcmp(tests[i].locale, tests[i+1].locale)) break;
            continue;
        }

        memset(str1, 0xee, sizeof(str1));
        strcpy(str1, tests[i].str1);

        memset(str2, 0xff, sizeof(str2));
        strcpy(str2, tests[i].str2);

        ret = _strnicoll(str1, str2, tests[i].count);
        if (!tests[i].exp)
            ok(!ret, "expected 0, got %d for %s, %s, %d\n", ret, str1, str2, (int)tests[i].count);
        else if (tests[i].exp < 0)
            ok(ret < 0, "expected < 0, got %d for %s, %s, %d\n", ret, str1, str2, (int)tests[i].count);
        else
            ok(ret > 0, "expected > 0, got %d for %s, %s, %d\n", ret, str1, str2, (int)tests[i].count);

        memset(str1W, 0xee, sizeof(str1W));
        len = mbstowcs(str1W, str1, ARRAY_SIZE(str1W));
        str1W[len] = 0;

        memset(str2W, 0xff, sizeof(str2W));
        len = mbstowcs(str2W, str2, ARRAY_SIZE(str2W));
        str2W[len] = 0;

        ret = _wcsnicoll(str1W, str2W, tests[i].count);
        if (!tests[i].exp)
            ok(!ret, "expected 0, got %d for %s, %s, %d\n", ret, str1, str2, (int)tests[i].count);
        else if (tests[i].exp < 0)
            ok(ret < 0, "expected < 0, got %d for %s, %s, %d\n", ret, str1, str2, (int)tests[i].count);
        else
            ok(ret > 0, "expected > 0, got %d for %s, %s, %d\n", ret, str1, str2, (int)tests[i].count);
    }
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
        { NULL, 0, 0 },
        { "a", 0, 0 },
        { "a", 1, 1 },
        { "a", 10, 1 },
        { "abc", 1, 1 },
    };
    unsigned int i;
    size_t ret;

    if (!p___strncnt)
    {
        win_skip("__strncnt() is not available.\n");
        return;
    }

    if (0) /* crashes */
        ret = p___strncnt(NULL, 1);

    for (i = 0; i < ARRAY_SIZE(strncnt_tests); ++i)
    {
        ret = p___strncnt(strncnt_tests[i].str, strncnt_tests[i].size);
        ok(ret == strncnt_tests[i].ret, "%u: unexpected return value %u.\n", i, (int)ret);
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
        ret = p_towlower(i);
        if (i >= 'A' && i <= 'Z')
        {
            exp = i + 'a' - 'A';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);

        ret = p_towupper(i);
        if (i >= 'a' && i <= 'z')
        {
            exp = i + 'A' - 'a';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);
    }

    if (!p__towlower_l || !p__towupper_l || !p__create_locale)
    {
        win_skip("_towlower_l/_towupper_l/_create_locale not available\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(locales); i++) {
        locale = locales[i] ? p__create_locale(LC_ALL, locales[i]) : NULL;

        for (j = 0; j <= 0xffff; j++) {
            ret = p__towlower_l(j, locale);
            if (j >= 'A' && j <= 'Z')
            {
                exp = j + 'a' - 'A';
                ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
            }
            else
                ok(ret == j, "expected self %x, got %x for C locale\n", j, ret);

            ret = p__towupper_l(j, locale);
            if (j >= 'a' && j <= 'z')
            {
                exp = j + 'A' - 'a';
                ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
            }
            else
                ok(ret == j, "expected self %x, got %x for C locale\n", j, ret);
        }

        p__free_locale(locale);
    }
}

static void test_strstr(void)
{
    static char long_str[1024];
    const struct {
        const char *haystack;
        const char *needle;
        int off;
    } tests[] = {
        { "", "", 0 },
        { "", "a", -1 },
        { "a", "", 0 },
        { "aabc", "abc", 1 },
        { "aaaa", "aaaa", 0 },
        { "simple", "simple", 0 },
        { "aaaaxaaaaxaaaa", "aaaaa", -1 },
        { "aaaaxaaaaxaaaaa", "aaaaa", 10 },
        { "abcabcdababcdabcdabde", "abcdabd", 13 },
        { "abababababcabababcababbba", "abababcaba", 4 },
        { long_str, long_str+1, 0 }
    };
    const char *r, *exp;
    int i;

    memset(long_str, 'a', sizeof(long_str)-1);

    for (i=0; i<ARRAY_SIZE(tests); i++)
    {
        r = strstr(tests[i].haystack, tests[i].needle);
        exp = tests[i].off == -1 ? NULL : tests[i].haystack + tests[i].off;
        ok(r == exp, "%d) strstr returned %p, expected %p\n", i, r, exp);
    }
}

static void test_iswdigit(void)
{
    static const struct {
        WCHAR c;
        int r;
    } tests[] = {
        { '0', C1_DIGIT },
        { '9', C1_DIGIT },
        { 'a', 0 },
        { 0xff16, C1_DIGIT },
        { 0x0660, C1_DIGIT },
        { 0x0ce6, C1_DIGIT }
    };
    int i, r;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        r = iswdigit(tests[i].c);
        ok(r == tests[i].r, "iswdigit returned %x for %x\n", r, tests[i].c);
    }
}

static void test_wcscmp(void)
{
    int r;

    r = wcscmp(L"a", L"z");
    ok(r == -1, "wcscmp returned %d\n", r);

    r = wcscmp(L"z", L"a");
    ok(r == 1, "wcscmp returned %d\n", r);

    r = wcscmp(L"f", L"f");
    ok(!r, "wcscmp returned %d\n", r);
}

static const char* debugstr_ldouble(_LDOUBLE *v)
{
    static char buf[2 * ARRAY_SIZE(v->ld) + 1];
    int i;

    for(i=0; i<ARRAY_SIZE(v->ld); i++)
    {
        buf[2*i] = v->ld[i] / 16 + '0';
        if(buf[2*i] > '9') buf[2*i] -= 10 + '0' - 'a';
        buf[2*i+1] = v->ld[i] % 16 + '0';
        if(buf[2*i+1] > '9') buf[2*i+1] -= 10 + '0' - 'a';
    }
    buf[2 * ARRAY_SIZE(v->ld)] = 0;
    return buf;
}

static void test___STRINGTOLD(void)
{
    static const struct {
        const char *str;
        int endptr;
        int r;
        _LDOUBLE v;
        BOOL todo;
    } tests[] = {
        { "0", 1 },
        { "nan", 0, 4 },
        { "inf", 0, 4 },
        { "-0.0", 4, 0, {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 }} },
        { "1e0", 3, 0, {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x3f }} },
        { "1.7976931348623158e+308", 23, 0, {{ 0xaf, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x43 }} },
        { "1.7976931348623159e+308", 23, 0, {{ 0xb1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x43 }} },
        { "3.65e-4951", 10, 0, {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }} },
        { "1.82e-4951", 10, 0, {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}, TRUE },
        { "1e-99999", 8, 1, {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }} },
        { "1.18e+4932", 10, 0, {{ 0x25, 0x75, 0x06, 0x68, 0x8a, 0xf1, 0xe7, 0xfd, 0xfe, 0x7f }} },
        { "1.19e+4932", 10, 2, {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x7f }} },
        { "1e+99999", 8, 2, {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x7f }} },
    };

    char *endptr;
    _LDOUBLE v;
    int i, r;

    for(i=0; i<ARRAY_SIZE(tests); i++)
    {
        errno = 0xdeadbeef;
        r = __STRINGTOLD(&v, &endptr, tests[i].str, 0);
        todo_wine_if(tests[i].todo)
            ok(r == tests[i].r, "%d) r = %d\n", i, r);
        ok(endptr == tests[i].str + tests[i].endptr, "%d) endptr = %p, expected %p\n",
                i, endptr, tests[i].str+tests[i].endptr);
        ok(!memcmp(&v, &tests[i].v, sizeof(v)), "%d) v = %s\n", i, debugstr_ldouble(&v));
        ok(errno == 0xdeadbeef, "%d) errno = %x\n", i, errno);
    }
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
        {"English", 'I', 'i'},  /* LATIN CAPITAL LETTER I */
        {"English", 0x0130},    /* LATIN CAPITAL LETTER I WITH DOT ABOVE */

        {"Turkish", 'I', 'i'},  /* LATIN CAPITAL LETTER I */
        {"Turkish", 0x0130},    /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    };
    struct test lcases[] = {
        {"English", 'i', 'I'},  /* LATIN SMALL LETTER I */
        {"English", 0x0131},    /* LATIN SMALL LETTER DOTLESS I */

        {"Turkish", 'i', 'I'},  /* LATIN SMALL LETTER I */
        {"Turkish", 0x0131},    /* LATIN SMALL LETTER DOTLESS I */
    };

    for (i = 0; i < ARRAY_SIZE(ucases); i++) {
        if (!setlocale(LC_ALL, ucases[i].lang)) {
            win_skip("skipping special case tests for %s\n", ucases[i].lang);
            continue;
        }

        ret = p_towlower(ucases[i].ch);
        exp = ucases[i].exp ? ucases[i].exp : ucases[i].ch;
        ok(ret == exp, "expected lowercase %x, got %x for locale %s\n", exp, ret, ucases[i].lang);
    }

    for (i = 0; i < ARRAY_SIZE(lcases); i++) {
        if (!setlocale(LC_ALL, lcases[i].lang)) {
            win_skip("skipping special case tests for %s\n", lcases[i].lang);
            continue;
        }

        ret = p_towupper(lcases[i].ch);
        exp = lcases[i].exp ? lcases[i].exp : lcases[i].ch;
        ok(ret == exp, "expected uppercase %x, got %x for locale %s\n", exp, ret, lcases[i].lang);
    }

    setlocale(LC_ALL, "C");

    if (!p__towlower_l || !p__towupper_l || !p__create_locale)
    {
        win_skip("_towlower_l/_towupper_l/_create_locale not available\n");
        return;
    }

    /* test _towlower_l creating locale */
    for (i = 0; i < ARRAY_SIZE(ucases); i++) {
        if (!(locale = p__create_locale(LC_ALL, ucases[i].lang))) {
            win_skip("locale %s not available.  skipping\n", ucases[i].lang);
            continue;
        }

        ret = p__towlower_l(ucases[i].ch, locale);
        exp = ucases[i].exp ? ucases[i].exp : ucases[i].ch;
        ok(ret == exp, "expected lowercase %x, got %x for locale %s\n", exp, ret, ucases[i].lang);

        p__free_locale(locale);
    }

    /* test _towupper_l creating locale */
    for (i = 0; i < ARRAY_SIZE(lcases); i++) {
        if (!(locale = p__create_locale(LC_ALL, lcases[i].lang))) {
            win_skip("locale %s not available.  skipping\n", lcases[i].lang);
            continue;
        }

        ret = p__towupper_l(lcases[i].ch, locale);
        exp = lcases[i].exp ? lcases[i].exp : lcases[i].ch;
        ok(ret == exp, "expected uppercase %x, got %x for locale %s\n", exp, ret, lcases[i].lang);

        p__free_locale(locale);
    }
}


static void test__mbbtype(void)
{
    static const char *test_locales[] =
    {
        "Arabic_Algeria",
        "Chinese_China",
        "English_Australia",
        "French_Belgium",
        "German_Austria",
        "Greek",
        "Hindi",
        "Japanese",
        "Korean",
        "Polish",
        "Portuguese_Brazil",
        "Russian",
        "Spanish_Argentina",
        "Swedish_Finland",
        "Ukrainian",
        "Vietnamese",
    };

    int expected, ret;
    unsigned int c, i;

    for (i = 0; i < ARRAY_SIZE(test_locales); ++i)
    {
        setlocale(LC_ALL, test_locales[i]);
        _setmbcp(_MB_CP_LOCALE);
        for (c = 0; c < 256; ++c)
        {
            if (_ismbblead(c))
                expected = _MBC_LEAD;
            else if (isprint(c))
                expected = _MBC_SINGLE;
            else
                expected = _MBC_ILLEGAL;

            ret = _mbbtype(c, 0);
            ok(ret == expected, "test %u, c %#x, got ret %#x, expected %#x.\n", i, c, ret, expected);

            if (_ismbbtrail(c))
                expected = _MBC_TRAIL;
            else
                expected = _MBC_ILLEGAL;

            ret = _mbbtype(c, 1);
            ok(ret == expected, "test %u, c %#x, got ret %#x, expected %#x.\n", i, c, ret, expected);
        }
    }
}

static void test_wcsncpy(void)
{
    wchar_t dst[6], *p;

    memset(dst, 0xff, sizeof(dst));
    p = wcsncpy(dst, L"1234567", 6);
    ok(p == dst, "Unexpected return value.\n");
    ok(!memcmp(dst, L"123456", sizeof(dst)), "unexpected buffer %s\n",
            wine_dbgstr_wn(dst, ARRAY_SIZE(dst)));
}

static void test_mbsrev(void)
{
    unsigned char buf[64], *ret;
    int cp = _getmbcp();

    _setmbcp(932);

    strcpy((char *)buf, "\x36\x8c\x8e");
    ret = _mbsrev(buf);
    ok(ret == buf, "ret = %p, expected %p\n", ret, buf);
    ok(!memcmp(buf, "\x8c\x8e\x36", 4), "buf = %s\n", wine_dbgstr_a((char *)buf));

    /* test string that ends with leading byte */
    strcpy((char *)buf, "\x36\x8c");
    ret = _mbsrev(buf);
    ok(ret == buf, "ret = %p, expected %p\n", ret, buf);
    skip_2k3_fail ok(!memcmp(buf, "\x36", 2), "buf = %s\n", wine_dbgstr_a((char *)buf));

    _setmbcp(cp);
}

#ifndef __REACTOS__
static void test__tolower_l(void)
{
    int ret;

    ok(setlocale(LC_ALL, "english") != NULL, "setlocale failed.\n");
    ret = _tolower_l('\xa5', 0);
    ok(ret == 165, "Got %d.\n", ret);
    ret = _tolower_l('\xb9', 0);
    ok(ret == 185, "Got %d.\n", ret);
    ret = _tolower_l('a', 0);
    ok(ret == 97, "Got %d.\n", ret);

    ok(setlocale(LC_ALL, ".936") != NULL, "setlocale failed.\n");
    ret = _tolower_l('\xa5', 0);
    ok(ret == -91, "Got %d.\n", ret);
    ret = _tolower_l('\xb9', 0);
    ok(ret == -71, "Got %d.\n", ret);
    ret = _tolower_l('a', 0);
    ok(ret == 97, "Got %d.\n", ret);

    ok(setlocale(LC_ALL, "chinese-simplified") != NULL, "setlocale failed.\n");
    ret = _tolower_l('\xa5', 0);
    ok(ret == -91, "Got %d.\n", ret);
    ret = _tolower_l('\xb9', 0);
    ok(ret == -71, "Got %d.\n", ret);
    ret = _tolower_l('a', 0);
    ok(ret == 97, "Got %d.\n", ret);

    setlocale(LC_ALL, "C");
}
#endif

static void test__strnicmp_l(void)
{
    static const struct {
        const char *str1;
        const char *str2;
    } tests[] = {
        { "wine",             "win" },
        /* GBK string tests */
        { "\xce\xac",         "ceac" },
        { "\xce\xac\xc4\xe1", "\xce\xac" },
        { "\xce\xac""abc",    "\xce\xac" },
        { "abc\xce\xac",      "abc" },
        { "\xce\xac\xc4\xe1", "\xc4\xe1" },
        { "\xce\xac",         "\xc4\xe1" },
        { "\xb8\xdf",         "\xb8\xdb" },
        { "\xb9",             "\xa5" },
    };
    _locale_t locale;
    int ret, i;

    if (!p__strnicmp_l)
    {
        win_skip("_strnicmp_l isn't available.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        ret = p__strnicmp_l(tests[i].str1, tests[i].str2, INT_MAX, 0);
        ok(ret > 0, "tests[%d]: Got %d.\n", i, ret);

        ret = p__strnicmp_l(tests[i].str2, tests[i].str1, INT_MAX, 0);
        ok(ret < 0, "tests[%d]: Got %d.\n", i, ret);
    }

    if (!p__create_locale)
        win_skip("_create_locale isn't available.\n");
    else
    {
        locale = p__create_locale(LC_ALL, ".936");
        ok(locale != NULL, "Failed to create locale.\n");

        for (i = 0; i < ARRAY_SIZE(tests); i++)
        {
            ret = p__strnicmp_l(tests[i].str1, tests[i].str2, INT_MAX, locale);
            ok(ret > 0, "tests[%d]: Got %d.\n", i, ret);

            ret = p__strnicmp_l(tests[i].str2, tests[i].str1, INT_MAX, locale);
            ok(ret < 0, "tests[%d]: Got %d.\n", i, ret);
        }

        p__free_locale(locale);
    }

    if (!setlocale(LC_ALL, ".936"))
    {
        win_skip("Skip testing _strnicmp_l with 936 code page.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        ret = p__strnicmp_l(tests[i].str1, tests[i].str2, INT_MAX, 0);
        ok(ret > 0, "tests[%d]: Got %d.\n", i, ret);

        ret = p__strnicmp_l(tests[i].str2, tests[i].str1, INT_MAX, 0);
        ok(ret < 0, "tests[%d]: Got %d.\n", i, ret);
    }

    setlocale(LC_ALL, "C");
}

static void test_toupper(void)
{
    int ret;

    ok(setlocale(LC_ALL, "English_United States") != NULL, "setlocale failed.\n");
    errno = 0xdeadbeef;
    ret = p_toupper((signed char)0xf0);
    ok(ret == 0xd0, "Got %#x.\n", ret);
    skip_2k3_fail ok(errno == EILSEQ, "Got errno %d.\n", errno);
    errno = 0xdeadbeef;
    ret = p_toupper(0xf0);
    ok(ret == 0xd0, "Got %#x.\n", ret);
    ok(errno == 0xdeadbeef, "Got errno %d.\n", errno);

    ok(setlocale(LC_ALL, "Polish") != NULL, "setlocale failed.\n");
    errno = 0xdeadbeef;
    ret = p_toupper((signed char)0xa5);
    ok(ret == 0xa5, "Got %#x.\n", ret);
    skip_2k3_fail ok(errno == EILSEQ, "Got errno %d.\n", errno);
    errno = 0xdeadbeef;
    ret = p_toupper((signed char)0xb9);
    ok(ret == 0xa5, "Got %#x.\n", ret);
    skip_2k3_fail ok(errno == EILSEQ, "Got errno %d.\n", errno);

    skip_2k3_fail {
    ok(setlocale(LC_ALL, "Japanese_Japan.932") != NULL, "setlocale failed.\n");
    errno = 0xdeadbeef;
    ret = p_toupper((signed char)0xf0);
    ok(ret == (signed char)0xf0, "Got %#x.\n", ret);
    ok(errno == EILSEQ, "Got errno %d.\n", errno);
    errno = 0xdeadbeef;
    ret = p_toupper(0xf0);
    ok(ret == 0xf0, "Got %#x.\n", ret);
    ok(errno == 0xdeadbeef, "Got errno %d.\n", errno);

    ok(setlocale(LC_ALL, "Chinese_China.936") != NULL, "setlocale failed.\n");
    errno = 0xdeadbeef;
    ret = p_toupper((signed char)0xf0);
    ok(ret == (signed char)0xf0, "Got %#x.\n", ret);
    ok(errno == EILSEQ, "Got errno %d.\n", errno);
    errno = 0xdeadbeef;
    ret = p_toupper(0xf0);
    ok(ret == 0xf0, "Got %#x.\n", ret);
    ok(errno == 0xdeadbeef, "Got errno %d.\n", errno);
    }

    setlocale(LC_ALL, "C");
}

START_TEST(string)
{
    char mem[100];
    static const char xilstring[]="c:/xilinx";
    int nLen;

    hMsvcrt = GetModuleHandleA("msvcrt.dll");
    if (!hMsvcrt)
        hMsvcrt = GetModuleHandleA("msvcrtd.dll");
    ok(hMsvcrt != 0, "GetModuleHandleA failed\n");
    SET(pmemcpy,"memcpy");
    p_memcpy_s = (void*)GetProcAddress( hMsvcrt, "memcpy_s" );
    p_memmove_s = (void*)GetProcAddress( hMsvcrt, "memmove_s" );
    SET(pmemcmp,"memcmp");
    SET(p_mbctype,"_mbctype");
    SET(p__mb_cur_max,"__mb_cur_max");
    SET(p_strcpy, "strcpy");
    SET(p_strcmp, "strcmp");
    SET(p_strncmp, "strncmp");
    pstrcpy_s = (void *)GetProcAddress( hMsvcrt,"strcpy_s" );
    pstrcat_s = (void *)GetProcAddress( hMsvcrt,"strcat_s" );
    p_strncpy_s = (void *)GetProcAddress( hMsvcrt, "strncpy_s" );
    p_strncat_s = (void *)GetProcAddress( hMsvcrt, "strncat_s" );
    p_mbscat_s = (void*)GetProcAddress( hMsvcrt, "_mbscat_s" );
    p_mbsnbcat_s = (void *)GetProcAddress( hMsvcrt,"_mbsnbcat_s" );
    p_mbsnbcpy_s = (void *)GetProcAddress( hMsvcrt,"_mbsnbcpy_s" );
    p__mbscpy_s = (void *)GetProcAddress( hMsvcrt,"_mbscpy_s" );
    p_wcscpy_s = (void *)GetProcAddress( hMsvcrt,"wcscpy_s" );
    p_wcsncpy_s = (void *)GetProcAddress( hMsvcrt,"wcsncpy_s" );
    p_wcsncat_s = (void *)GetProcAddress( hMsvcrt,"wcsncat_s" );
    p_wcsupr_s = (void *)GetProcAddress( hMsvcrt,"_wcsupr_s" );
    p_strnlen = (void *)GetProcAddress( hMsvcrt,"strnlen" );
    p_strtoi64 = (void *)GetProcAddress(hMsvcrt, "_strtoi64");
    p_strtoui64 = (void *)GetProcAddress(hMsvcrt, "_strtoui64");
    p_wcstoi64 = (void *)GetProcAddress(hMsvcrt, "_wcstoi64");
    p_wcstoui64 = (void *)GetProcAddress(hMsvcrt, "_wcstoui64");
    pmbstowcs_s = (void *)GetProcAddress(hMsvcrt, "mbstowcs_s");
    pwcstombs_s = (void *)GetProcAddress(hMsvcrt, "wcstombs_s");
    p_wcstombs_s_l = (void *)GetProcAddress(hMsvcrt, "_wcstombs_s_l");
    pwcsrtombs = (void *)GetProcAddress(hMsvcrt, "wcsrtombs");
    p_gcvt_s = (void *)GetProcAddress(hMsvcrt, "_gcvt_s");
    p_itoa_s = (void *)GetProcAddress(hMsvcrt, "_itoa_s");
    p_strlwr_s = (void *)GetProcAddress(hMsvcrt, "_strlwr_s");
    p_ultoa_s = (void *)GetProcAddress(hMsvcrt, "_ultoa_s");
    p_wcslwr_s = (void*)GetProcAddress(hMsvcrt, "_wcslwr_s");
    p_mbsupr_s = (void*)GetProcAddress(hMsvcrt, "_mbsupr_s");
    p_mbslwr_s = (void*)GetProcAddress(hMsvcrt, "_mbslwr_s");
    p_btowc = (void*)GetProcAddress(hMsvcrt, "btowc");
    p_wctob = (void*)GetProcAddress(hMsvcrt, "wctob");
    p_wcrtomb = (void*)GetProcAddress(hMsvcrt, "wcrtomb");
    p_wcrtomb_s = (void*)GetProcAddress(hMsvcrt, "wcrtomb_s");
    p_tolower = (void*)GetProcAddress(hMsvcrt, "tolower");
    p_towlower = (void*)GetProcAddress(hMsvcrt, "towlower");
    p__towlower_l = (void*)GetProcAddress(hMsvcrt, "_towlower_l");
    p_towupper = (void*)GetProcAddress(hMsvcrt, "towupper");
    p__towupper_l = (void*)GetProcAddress(hMsvcrt, "_towupper_l");
    p__create_locale = (void*)GetProcAddress(hMsvcrt, "_create_locale");
    p__free_locale = (void*)GetProcAddress(hMsvcrt, "_free_locale");
    p_mbrlen = (void*)GetProcAddress(hMsvcrt, "mbrlen");
    p_mbrtowc = (void*)GetProcAddress(hMsvcrt, "mbrtowc");
    p_mbsrtowcs = (void*)GetProcAddress(hMsvcrt, "mbsrtowcs");
    p_mbsrtowcs_s = (void*)GetProcAddress(hMsvcrt, "mbsrtowcs_s");
    p__atodbl_l = (void*)GetProcAddress(hMsvcrt, "_atodbl_l");
    p__atof_l = (void*)GetProcAddress(hMsvcrt, "_atof_l");
    p__strtod_l = (void*)GetProcAddress(hMsvcrt, "_strtod_l");
    p__strnset_s = (void*)GetProcAddress(hMsvcrt, "_strnset_s");
    p__wcsnset_s = (void*)GetProcAddress(hMsvcrt, "_wcsnset_s");
    p__wcsset_s = (void*)GetProcAddress(hMsvcrt, "_wcsset_s");
    p__mbsnlen = (void*)GetProcAddress(hMsvcrt, "_mbsnlen");
    p__mbccpy_s = (void*)GetProcAddress(hMsvcrt, "_mbccpy_s");
    p__memicmp = (void*)GetProcAddress(hMsvcrt, "_memicmp");
    p__memicmp_l = (void*)GetProcAddress(hMsvcrt, "_memicmp_l");
    p___strncnt = (void*)GetProcAddress(hMsvcrt, "__strncnt");
    p_mbsnextc_l = (void*)GetProcAddress(hMsvcrt, "_mbsnextc_l");
    p_mbscmp_l = (void*)GetProcAddress(hMsvcrt, "_mbscmp_l");
    p__strnicmp_l = (void*)GetProcAddress(hMsvcrt, "_strnicmp_l");
    p_toupper = (void*)GetProcAddress(hMsvcrt, "toupper");

    /* MSVCRT memcpy behaves like memmove for overlapping moves,
       MFC42 CString::Insert seems to rely on that behaviour */
    strcpy(mem,xilstring);
    nLen=strlen(xilstring);
    pmemcpy(mem+5, mem,nLen+1);
    ok(pmemcmp(mem+5,xilstring, nLen) == 0,
       "Got result %s\n",mem+5);

    /* run tolower tests first */
    test_tolower();
    test_swab();
    test_strcspn();
    test_mbcp();
    test_mbsspn();
    test_mbsspnp();
    test_strdup();
    test_wcsdup();
    test_strcmp();
    test_strcpy_s();
    test_memcpy_s();
    test_memmove_s();
    test_strcat_s();
    test_strncat_s();
    test__mbscat_s();
    test__mbsnbcpy_s();
    test__mbscpy_s();
    test_mbcjisjms();
    test_mbcjmsjis();
    test_mbctohira();
    test_mbctokata();
    test_mbbtombc();
    test_mbctombb();
    test_ismbckata();
    test_ismbclegal();
    test_strtok();
    test__mbstok();
    test_wcscpy_s();
    test__wcsupr_s();
    test_strtol();
    test_strnlen();
    test__strtoi64();
    test__strtod();
    test_mbstowcs();
    test__wcstombs_s_l();
    test_gcvt();
    test__itoa_s();
    test__strlwr_s();
    test_wcsncat_s();
    test__mbsnbcat_s();
    test__ultoa_s();
    test__wcslwr_s();
    test__mbsupr_s();
    test__mbslwr_s();
    test_wctob();
    test_btowc();
    test_wctomb();
    test__atodbl();
    test__stricmp();
    test__wcstoi64();
    test__wcstol();
    test_atoi();
    test_atol();
    test_atof();
    test_strncpy();
    test_strxfrm();
    test__strnset_s();
    test__wcsnset_s();
    test__wcsset_s();
    test__mbscmp();
    test__ismbclx();
    test__memicmp();
    test__memicmp_l();
    test__strupr();
    test__tcsncoll();
    test__tcscoll();
    test__tcsnicoll();
    test___strncnt();
    test_C_locale();
    test_strstr();
    test_iswdigit();
    test_wcscmp();
    test___STRINGTOLD();
    test_SpecialCasing();
    test__mbbtype();
    test_wcsncpy();
    test_mbsrev();
#ifndef __REACTOS__
    test__tolower_l();
#endif
    test__strnicmp_l();
    test_toupper();
}
