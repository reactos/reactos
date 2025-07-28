/* Unit test suite for string functions and some wcstring functions
 *
 * Copyright 2003 Thomas Mertes
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
 *
 * NOTES
 * We use function pointers here as there is no import library for NTDLL on
 * windows.
 */

#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/test.h"


/* Function ptrs for ntdll calls */
static HMODULE hntdll = 0;
static NTSTATUS (WINAPI *pRtlUnicodeStringToAnsiString)(STRING *, const UNICODE_STRING *, BOOLEAN);
static VOID     (WINAPI *pRtlFreeAnsiString)(PSTRING);
static BOOLEAN  (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING,LPCSTR);
static VOID     (WINAPI *pRtlFreeUnicodeString)(PUNICODE_STRING);
static WCHAR    (WINAPI *pRtlUpcaseUnicodeChar)(WCHAR);

static int      (__cdecl *patoi)(const char *);
static LONG     (__cdecl *patol)(const char *);
static LONGLONG (__cdecl *p_atoi64)(const char *);
static LPSTR    (__cdecl *p_itoa)(int, LPSTR, INT);
static LPSTR    (__cdecl *p_ltoa)(LONG, LPSTR, INT);
static LPSTR    (__cdecl *p_ultoa)(ULONG, LPSTR, INT);
static LPSTR    (__cdecl *p_i64toa)(LONGLONG, LPSTR, INT);
static LPSTR    (__cdecl *p_ui64toa)(ULONGLONG, LPSTR, INT);

static int      (__cdecl *p_wtoi)(LPCWSTR);
static LONG     (__cdecl *p_wtol)(LPCWSTR);
static LONGLONG (__cdecl *p_wtoi64)(LPCWSTR);
static LONG     (__cdecl *pwcstol)(LPCWSTR,LPWSTR*,INT);
static ULONG    (__cdecl *pwcstoul)(LPCWSTR,LPWSTR*,INT);
static LPWSTR   (__cdecl *p_itow)(int, LPWSTR, int);
static LPWSTR   (__cdecl *p_ltow)(LONG, LPWSTR, INT);
static LPWSTR   (__cdecl *p_ultow)(ULONG, LPWSTR, INT);
static LPWSTR   (__cdecl *p_i64tow)(LONGLONG, LPWSTR, INT);
static LPWSTR   (__cdecl *p_ui64tow)(ULONGLONG, LPWSTR, INT);

static LPWSTR   (__cdecl *p_wcslwr)(LPWSTR);
static LPWSTR   (__cdecl *p_wcsupr)(LPWSTR);
static WCHAR    (__cdecl *ptowlower)(WCHAR);
static WCHAR    (__cdecl *ptowupper)(WCHAR);
static int      (__cdecl *p_wcsicmp)(LPCWSTR,LPCWSTR);
static int      (__cdecl *p_wcsnicmp)(LPCWSTR,LPCWSTR,int);

static LPWSTR   (__cdecl *pwcschr)(LPCWSTR, WCHAR);
static LPWSTR   (__cdecl *pwcsrchr)(LPCWSTR, WCHAR);
static void*    (__cdecl *pmemchr)(const void*, int, size_t);

static void     (__cdecl *pqsort)(void *,size_t,size_t, int(__cdecl *compar)(const void *, const void *) );
static void*    (__cdecl *pbsearch)(void *,void*,size_t,size_t, int(__cdecl *compar)(const void *, const void *) );
static int      (WINAPIV *p_snprintf)(char *, size_t, const char *, ...);
static int      (WINAPIV *p_snprintf_s)(char *, size_t, size_t, const char *, ...);
static int      (WINAPIV *p_snwprintf)(WCHAR *, size_t, const WCHAR *, ...);
static int      (WINAPIV *p_snwprintf_s)(WCHAR *, size_t, size_t, const WCHAR *, ...);

static int      (__cdecl *ptolower)(int);
static int      (__cdecl *ptoupper)(int);
static int      (__cdecl *p_strnicmp)(LPCSTR,LPCSTR,size_t);

static int      (WINAPIV *psscanf)(const char *, const char *, ...);

static int      (__cdecl *piswctype)(WCHAR,unsigned short);
static int      (__cdecl *piswalpha)(WCHAR);
static int      (__cdecl *piswdigit)(WCHAR);
static int      (__cdecl *piswlower)(WCHAR);
static int      (__cdecl *piswspace)(WCHAR);
static int      (__cdecl *piswxdigit)(WCHAR);

static int      (__cdecl *pisalnum)(int);
static int      (__cdecl *pisalpha)(int);
static int      (__cdecl *piscntrl)(int);
static int      (__cdecl *pisdigit)(int);
static int      (__cdecl *pisgraph)(int);
static int      (__cdecl *pislower)(int);
static int      (__cdecl *pisprint)(int);
static int      (__cdecl *pispunct)(int);
static int      (__cdecl *pisspace)(int);
static int      (__cdecl *pisupper)(int);
static int      (__cdecl *pisxdigit)(int);

static void InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    ok(hntdll != 0, "LoadLibrary failed\n");
#define X(name) p##name = (void *)GetProcAddress( hntdll, #name );
    X(RtlUnicodeStringToAnsiString);
    X(RtlFreeAnsiString);
    X(RtlCreateUnicodeStringFromAsciiz);
    X(RtlFreeUnicodeString);
    X(RtlUpcaseUnicodeChar);
    X(atoi);
    X(atol);
    X(_atoi64);
    X(_itoa);
    X(_ltoa);
    X(_ultoa);
    X(_i64toa);
    X(_ui64toa);
    X(_wtoi);
    X(_wtol);
    X(_wtoi64);
    X(wcstol);
    X(wcstoul);
    X(_itow);
    X(_ltow);
    X(_ultow);
    X(_i64tow);
    X(_ui64tow);
    X(_wcslwr);
    X(_wcsupr);
    X(towlower);
    X(towupper);
    X(_wcsicmp);
    X(_wcsnicmp);
    X(wcschr);
    X(wcsrchr);
    X(memchr);
    X(qsort);
    X(bsearch);
    X(_snprintf);
    X(_snprintf_s);
    X(_snwprintf);
    X(_snwprintf_s);
    X(tolower);
    X(toupper);
    X(_strnicmp);
    X(sscanf);
    X(iswctype);
    X(iswalpha);
    X(iswdigit);
    X(iswlower);
    X(iswspace);
    X(iswxdigit);
    X(isalnum);
    X(isalpha);
    X(iscntrl);
    X(isdigit);
    X(isgraph);
    X(islower);
    X(isprint);
    X(ispunct);
    X(isspace);
    X(isupper);
    X(isxdigit);
#undef X
}


#define LARGE_STRI_BUFFER_LENGTH 67

typedef struct {
    int base;
    ULONG value;
    const char *Buffer;
    int mask; /* ntdll/msvcrt: 0x01=itoa, 0x02=ltoa, 0x04=ultoa */
              /*               0x10=itow, 0x20=ltow, 0x40=ultow */
} ulong2str_t;

static const ulong2str_t ulong2str[] = {
    {10,         123, "123\0---------------------------------------------------------------", 0x77},

    { 2, 0x80000000U, "10000000000000000000000000000000\0----------------------------------", 0x67},
    { 2, -2147483647, "10000000000000000000000000000001\0----------------------------------", 0x67},
    { 2,      -65537, "11111111111111101111111111111111\0----------------------------------", 0x67},
    { 2,      -65536, "11111111111111110000000000000000\0----------------------------------", 0x67},
    { 2,      -65535, "11111111111111110000000000000001\0----------------------------------", 0x67},
    { 2,      -32768, "11111111111111111000000000000000\0----------------------------------", 0x67},
    { 2,      -32767, "11111111111111111000000000000001\0----------------------------------", 0x67},
    { 2,          -2, "11111111111111111111111111111110\0----------------------------------", 0x67},
    { 2,          -1, "11111111111111111111111111111111\0----------------------------------", 0x67},
    { 2,           0, "0\0-----------------------------------------------------------------", 0x77},
    { 2,           1, "1\0-----------------------------------------------------------------", 0x77},
    { 2,          10, "1010\0--------------------------------------------------------------", 0x77},
    { 2,         100, "1100100\0-----------------------------------------------------------", 0x77},
    { 2,        1000, "1111101000\0--------------------------------------------------------", 0x77},
    { 2,       10000, "10011100010000\0----------------------------------------------------", 0x77},
    { 2,       32767, "111111111111111\0---------------------------------------------------", 0x77},
    { 2,       32768, "1000000000000000\0--------------------------------------------------", 0x77},
    { 2,       65535, "1111111111111111\0--------------------------------------------------", 0x77},
    { 2,      100000, "11000011010100000\0-------------------------------------------------", 0x77},
    { 2,      234567, "111001010001000111\0------------------------------------------------", 0x77},
    { 2,      300000, "1001001001111100000\0-----------------------------------------------", 0x77},
    { 2,      524287, "1111111111111111111\0-----------------------------------------------", 0x77},
    { 2,      524288, "10000000000000000000\0----------------------------------------------", 0x67},
    { 2,     1000000, "11110100001001000000\0----------------------------------------------", 0x67},
    { 2,    10000000, "100110001001011010000000\0------------------------------------------", 0x67},
    { 2,   100000000, "101111101011110000100000000\0---------------------------------------", 0x67},
    { 2,  1000000000, "111011100110101100101000000000\0------------------------------------", 0x67},
    { 2,  1073741823, "111111111111111111111111111111\0------------------------------------", 0x67},
    { 2,  2147483646, "1111111111111111111111111111110\0-----------------------------------", 0x67},
    { 2,  2147483647, "1111111111111111111111111111111\0-----------------------------------", 0x67},
    { 2, 2147483648U, "10000000000000000000000000000000\0----------------------------------", 0x67},
    { 2, 2147483649U, "10000000000000000000000000000001\0----------------------------------", 0x67},
    { 2, 4294967294U, "11111111111111111111111111111110\0----------------------------------", 0x67},
    { 2,  0xFFFFFFFF, "11111111111111111111111111111111\0----------------------------------", 0x67},

    { 8, 0x80000000U, "20000000000\0-------------------------------------------------------", 0x77},
    { 8, -2147483647, "20000000001\0-------------------------------------------------------", 0x77},
    { 8,          -2, "37777777776\0-------------------------------------------------------", 0x77},
    { 8,          -1, "37777777777\0-------------------------------------------------------", 0x77},
    { 8,           0, "0\0-----------------------------------------------------------------", 0x77},
    { 8,           1, "1\0-----------------------------------------------------------------", 0x77},
    { 8,  2147483646, "17777777776\0-------------------------------------------------------", 0x77},
    { 8,  2147483647, "17777777777\0-------------------------------------------------------", 0x77},
    { 8, 2147483648U, "20000000000\0-------------------------------------------------------", 0x77},
    { 8, 2147483649U, "20000000001\0-------------------------------------------------------", 0x77},
    { 8, 4294967294U, "37777777776\0-------------------------------------------------------", 0x77},
    { 8, 4294967295U, "37777777777\0-------------------------------------------------------", 0x77},

    {10, 0x80000000U, "-2147483648\0-------------------------------------------------------", 0x33},
    {10, 0x80000000U, "2147483648\0--------------------------------------------------------", 0x44},
    {10, -2147483647, "-2147483647\0-------------------------------------------------------", 0x33},
    {10, -2147483647, "2147483649\0--------------------------------------------------------", 0x44},
    {10,          -2, "-2\0----------------------------------------------------------------", 0x33},
    {10,          -2, "4294967294\0--------------------------------------------------------", 0x44},
    {10,          -1, "-1\0----------------------------------------------------------------", 0x33},
    {10,          -1, "4294967295\0--------------------------------------------------------", 0x44},
    {10,           0, "0\0-----------------------------------------------------------------", 0x77},
    {10,           1, "1\0-----------------------------------------------------------------", 0x77},
    {10,          12, "12\0----------------------------------------------------------------", 0x77},
    {10,         123, "123\0---------------------------------------------------------------", 0x77},
    {10,        1234, "1234\0--------------------------------------------------------------", 0x77},
    {10,       12345, "12345\0-------------------------------------------------------------", 0x77},
    {10,      123456, "123456\0------------------------------------------------------------", 0x77},
    {10,     1234567, "1234567\0-----------------------------------------------------------", 0x77},
    {10,    12345678, "12345678\0----------------------------------------------------------", 0x77},
    {10,   123456789, "123456789\0---------------------------------------------------------", 0x77},
    {10,  2147483646, "2147483646\0--------------------------------------------------------", 0x77},
    {10,  2147483647, "2147483647\0--------------------------------------------------------", 0x77},
    {10, 2147483648U, "-2147483648\0-------------------------------------------------------", 0x33},
    {10, 2147483648U, "2147483648\0--------------------------------------------------------", 0x44},
    {10, 2147483649U, "-2147483647\0-------------------------------------------------------", 0x33},
    {10, 2147483649U, "2147483649\0--------------------------------------------------------", 0x44},
    {10, 4294967294U, "-2\0----------------------------------------------------------------", 0x33},
    {10, 4294967294U, "4294967294\0--------------------------------------------------------", 0x44},
    {10, 4294967295U, "-1\0----------------------------------------------------------------", 0x33},
    {10, 4294967295U, "4294967295\0--------------------------------------------------------", 0x44},

    {16,           0, "0\0-----------------------------------------------------------------", 0x77},
    {16,           1, "1\0-----------------------------------------------------------------", 0x77},
    {16,  2147483646, "7ffffffe\0----------------------------------------------------------", 0x77},
    {16,  2147483647, "7fffffff\0----------------------------------------------------------", 0x77},
    {16,  0x80000000, "80000000\0----------------------------------------------------------", 0x77},
    {16,  0x80000001, "80000001\0----------------------------------------------------------", 0x77},
    {16,  0xFFFFFFFE, "fffffffe\0----------------------------------------------------------", 0x77},
    {16,  0xFFFFFFFF, "ffffffff\0----------------------------------------------------------", 0x77},

    { 2,       32768, "1000000000000000\0--------------------------------------------------", 0x77},
    { 2,       65536, "10000000000000000\0-------------------------------------------------", 0x77},
    { 2,      131072, "100000000000000000\0------------------------------------------------", 0x77},
    {16,  0xffffffff, "ffffffff\0----------------------------------------------------------", 0x77},
    {16,         0xa, "a\0-----------------------------------------------------------------", 0x77},
    {16,           0, "0\0-----------------------------------------------------------------", 0x77},
    {20,     3368421, "111111\0------------------------------------------------------------", 0x77},
    {36,    62193781, "111111\0------------------------------------------------------------", 0x77},
    {37,    71270178, "111111\0------------------------------------------------------------", 0x77},
};


static void one_itoa_test(int test_num, const ulong2str_t *ulong2str)
{
    char dest_str[LARGE_STRI_BUFFER_LENGTH + 1];
    int value;
    LPSTR result;

    memset(dest_str, '-', LARGE_STRI_BUFFER_LENGTH);
    dest_str[LARGE_STRI_BUFFER_LENGTH] = '\0';
    value = ulong2str->value;
    result = p_itoa(value, dest_str, ulong2str->base);
    ok(result == dest_str,
       "(test %d): _itoa(%d, [out], %d) has result %p, expected: %p\n",
       test_num, value, ulong2str->base, result, dest_str);
    ok(memcmp(dest_str, ulong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
       "(test %d): _itoa(%d, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, value, ulong2str->base, dest_str, ulong2str->Buffer);
}


static void one_ltoa_test(int test_num, const ulong2str_t *ulong2str)
{
    char dest_str[LARGE_STRI_BUFFER_LENGTH + 1];
    LONG value;
    LPSTR result;

    memset(dest_str, '-', LARGE_STRI_BUFFER_LENGTH);
    dest_str[LARGE_STRI_BUFFER_LENGTH] = '\0';
    value = ulong2str->value;
    result = p_ltoa(ulong2str->value, dest_str, ulong2str->base);
    ok(result == dest_str,
       "(test %d): _ltoa(%ld, [out], %d) has result %p, expected: %p\n",
       test_num, value, ulong2str->base, result, dest_str);
    ok(memcmp(dest_str, ulong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
       "(test %d): _ltoa(%ld, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, value, ulong2str->base, dest_str, ulong2str->Buffer);
}


static void one_ultoa_test(int test_num, const ulong2str_t *ulong2str)
{
    char dest_str[LARGE_STRI_BUFFER_LENGTH + 1];
    ULONG value;
    LPSTR result;

    memset(dest_str, '-', LARGE_STRI_BUFFER_LENGTH);
    dest_str[LARGE_STRI_BUFFER_LENGTH] = '\0';
    value = ulong2str->value;
    result = p_ultoa(ulong2str->value, dest_str, ulong2str->base);
    ok(result == dest_str,
       "(test %d): _ultoa(%lu, [out], %d) has result %p, expected: %p\n",
       test_num, value, ulong2str->base, result, dest_str);
    ok(memcmp(dest_str, ulong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
       "(test %d): _ultoa(%lu, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, value, ulong2str->base, dest_str, ulong2str->Buffer);
}


static void test_ulongtoa(void)
{
    int test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(ulong2str); test_num++) {
	if (ulong2str[test_num].mask & 0x01) {
	    one_itoa_test(test_num, &ulong2str[test_num]);
	} /* if */
	if (ulong2str[test_num].mask & 0x02) {
	    one_ltoa_test(test_num, &ulong2str[test_num]);
	} /* if */
	if (ulong2str[test_num].mask & 0x04) {
	    one_ultoa_test(test_num, &ulong2str[test_num]);
	} /* if */
    } /* for */
}


static void one_itow_test(int test_num, const ulong2str_t *ulong2str)
{
    int pos;
    WCHAR expected_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    WCHAR dest_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    UNICODE_STRING unicode_string;
    STRING ansi_str;
    int value;
    LPWSTR result;

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulong2str->Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	dest_wstr[pos] = '-';
    } /* for */
    dest_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    unicode_string.Length = LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length + sizeof(WCHAR);
    unicode_string.Buffer = dest_wstr;
    value = ulong2str->value;
    result = p_itow(value, dest_wstr, ulong2str->base);
    pRtlUnicodeStringToAnsiString(&ansi_str, &unicode_string, 1);
    ok(result == dest_wstr,
       "(test %d): _itow(%d, [out], %d) has result %p, expected: %p\n",
       test_num, value, ulong2str->base, result, dest_wstr);
    ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
       "(test %d): _itow(%d, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, value, ulong2str->base, ansi_str.Buffer, ulong2str->Buffer);
    pRtlFreeAnsiString(&ansi_str);
}


static void one_ltow_test(int test_num, const ulong2str_t *ulong2str)
{
    int pos;
    WCHAR expected_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    WCHAR dest_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    UNICODE_STRING unicode_string;
    STRING ansi_str;
    LONG value;
    LPWSTR result;

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulong2str->Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	dest_wstr[pos] = '-';
    } /* for */
    dest_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    unicode_string.Length = LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length + sizeof(WCHAR);
    unicode_string.Buffer = dest_wstr;

    value = ulong2str->value;
    result = p_ltow(value, dest_wstr, ulong2str->base);
    pRtlUnicodeStringToAnsiString(&ansi_str, &unicode_string, 1);
    ok(result == dest_wstr,
       "(test %d): _ltow(%ld, [out], %d) has result %p, expected: %p\n",
       test_num, value, ulong2str->base, result, dest_wstr);
    ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
       "(test %d): _ltow(%ld, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, value, ulong2str->base, ansi_str.Buffer, ulong2str->Buffer);
    pRtlFreeAnsiString(&ansi_str);
}


static void one_ultow_test(int test_num, const ulong2str_t *ulong2str)
{
    int pos;
    WCHAR expected_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    WCHAR dest_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    UNICODE_STRING unicode_string;
    STRING ansi_str;
    ULONG value;
    LPWSTR result;

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulong2str->Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	dest_wstr[pos] = '-';
    } /* for */
    dest_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    unicode_string.Length = LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length + sizeof(WCHAR);
    unicode_string.Buffer = dest_wstr;

    value = ulong2str->value;
    result = p_ultow(value, dest_wstr, ulong2str->base);
    pRtlUnicodeStringToAnsiString(&ansi_str, &unicode_string, 1);
    ok(result == dest_wstr,
       "(test %d): _ultow(%lu, [out], %d) has result %p, expected: %p\n",
       test_num, value, ulong2str->base, result, dest_wstr);
    ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
       "(test %d): _ultow(%lu, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, value, ulong2str->base, ansi_str.Buffer, ulong2str->Buffer);
    pRtlFreeAnsiString(&ansi_str);
}


static void test_ulongtow(void)
{
    int test_num;
    LPWSTR result;

    for (test_num = 0; test_num < ARRAY_SIZE(ulong2str); test_num++) {
	if (ulong2str[test_num].mask & 0x10) {
	    one_itow_test(test_num, &ulong2str[test_num]);
	} /* if */
	if (ulong2str[test_num].mask & 0x20) {
	    one_ltow_test(test_num, &ulong2str[test_num]);
	} /* if */
	if (ulong2str[test_num].mask & 0x40) {
	    one_ultow_test(test_num, &ulong2str[test_num]);
	} /* if */
    } /* for */

    if (0) {
        /* Crashes on XP and W2K3 */
        result = p_itow(ulong2str[0].value, NULL, 10);
        ok(result == NULL,
           "(test a): _itow(%ld, NULL, 10) has result %p, expected: NULL\n",
           ulong2str[0].value, result);
    }

    if (0) {
        /* Crashes on XP and W2K3 */
        result = p_ltow(ulong2str[0].value, NULL, 10);
        ok(result == NULL,
           "(test b): _ltow(%ld, NULL, 10) has result %p, expected: NULL\n",
           ulong2str[0].value, result);
    }

    if (0) {
        /* Crashes on XP and W2K3 */
        result = p_ultow(ulong2str[0].value, NULL, 10);
        ok(result == NULL,
           "(test c): _ultow(%ld, NULL, 10) has result %p, expected: NULL\n",
           ulong2str[0].value, result);
    }
}

#define ULL(a,b) (((ULONGLONG)(a) << 32) | (b))

typedef struct {
    int base;
    ULONGLONG value;
    const char *Buffer;
    int mask; /* ntdll/msvcrt: 0x01=i64toa, 0x02=ui64toa, 0x04=wrong _i64toa try next example */
              /*               0x10=i64tow, 0x20=ui64tow, 0x40=wrong _i64tow try next example */
} ulonglong2str_t;

static const ulonglong2str_t ulonglong2str[] = {
    {10,          123, "123\0---------------------------------------------------------------", 0x33},

    { 2,  0x80000000U, "10000000000000000000000000000000\0----------------------------------", 0x33},
    { 2,  -2147483647, "1111111111111111111111111111111110000000000000000000000000000001\0--", 0x33},
    { 2,       -65537, "1111111111111111111111111111111111111111111111101111111111111111\0--", 0x33},
    { 2,       -65536, "1111111111111111111111111111111111111111111111110000000000000000\0--", 0x33},
    { 2,       -65535, "1111111111111111111111111111111111111111111111110000000000000001\0--", 0x33},
    { 2,       -32768, "1111111111111111111111111111111111111111111111111000000000000000\0--", 0x33},
    { 2,       -32767, "1111111111111111111111111111111111111111111111111000000000000001\0--", 0x33},
    { 2,           -2, "1111111111111111111111111111111111111111111111111111111111111110\0--", 0x33},
    { 2,           -1, "1111111111111111111111111111111111111111111111111111111111111111\0--", 0x33},
    { 2,            0, "0\0-----------------------------------------------------------------", 0x33},
    { 2,            1, "1\0-----------------------------------------------------------------", 0x33},
    { 2,           10, "1010\0--------------------------------------------------------------", 0x33},
    { 2,          100, "1100100\0-----------------------------------------------------------", 0x33},
    { 2,         1000, "1111101000\0--------------------------------------------------------", 0x33},
    { 2,        10000, "10011100010000\0----------------------------------------------------", 0x33},
    { 2,        32767, "111111111111111\0---------------------------------------------------", 0x33},
    { 2,        32768, "1000000000000000\0--------------------------------------------------", 0x33},
    { 2,        65535, "1111111111111111\0--------------------------------------------------", 0x33},
    { 2,       100000, "11000011010100000\0-------------------------------------------------", 0x33},
    { 2,       234567, "111001010001000111\0------------------------------------------------", 0x33},
    { 2,       300000, "1001001001111100000\0-----------------------------------------------", 0x33},
    { 2,       524287, "1111111111111111111\0-----------------------------------------------", 0x33},
    { 2,       524288, "10000000000000000000\0----------------------------------------------", 0x33},
    { 2,      1000000, "11110100001001000000\0----------------------------------------------", 0x33},
    { 2,     10000000, "100110001001011010000000\0------------------------------------------", 0x33},
    { 2,    100000000, "101111101011110000100000000\0---------------------------------------", 0x33},
    { 2,   1000000000, "111011100110101100101000000000\0------------------------------------", 0x33},
    { 2,   1073741823, "111111111111111111111111111111\0------------------------------------", 0x33},
    { 2,   2147483646, "1111111111111111111111111111110\0-----------------------------------", 0x33},
    { 2,   2147483647, "1111111111111111111111111111111\0-----------------------------------", 0x33},
    { 2,  2147483648U, "10000000000000000000000000000000\0----------------------------------", 0x33},
    { 2,  2147483649U, "10000000000000000000000000000001\0----------------------------------", 0x33},
    { 2,  4294967294U, "11111111111111111111111111111110\0----------------------------------", 0x33},
    { 2,   0xFFFFFFFF, "11111111111111111111111111111111\0----------------------------------", 0x33},
    { 2, ULL(0x1,0xffffffff), "111111111111111111111111111111111\0---------------------------------", 0x33},
    { 2, ((ULONGLONG)100000)*100000, "1001010100000010111110010000000000\0--------------------------------", 0x33},
    { 2, ULL(0x3,0xffffffff), "1111111111111111111111111111111111\0--------------------------------", 0x33},
    { 2, ULL(0x7,0xffffffff), "11111111111111111111111111111111111\0-------------------------------", 0x33},
    { 2, ULL(0xf,0xffffffff), "111111111111111111111111111111111111\0------------------------------", 0x33},
    { 2, ((ULONGLONG)100000)*1000000, "1011101001000011101101110100000000000\0-----------------------------", 0x33},
    { 2, ULL(0x1f,0xffffffff), "1111111111111111111111111111111111111\0-----------------------------", 0x33},
    { 2, ULL(0x3f,0xffffffff), "11111111111111111111111111111111111111\0----------------------------", 0x33},
    { 2, ULL(0x7f,0xffffffff), "111111111111111111111111111111111111111\0---------------------------", 0x33},
    { 2, ULL(0xff,0xffffffff), "1111111111111111111111111111111111111111\0--------------------------", 0x33},

    { 8,  0x80000000U, "20000000000\0-------------------------------------------------------", 0x33},
    { 8,  -2147483647, "1777777777760000000001\0--------------------------------------------", 0x33},
    { 8,           -2, "1777777777777777777776\0--------------------------------------------", 0x33},
    { 8,           -1, "1777777777777777777777\0--------------------------------------------", 0x33},
    { 8,            0, "0\0-----------------------------------------------------------------", 0x33},
    { 8,            1, "1\0-----------------------------------------------------------------", 0x33},
    { 8,   2147483646, "17777777776\0-------------------------------------------------------", 0x33},
    { 8,   2147483647, "17777777777\0-------------------------------------------------------", 0x33},
    { 8,  2147483648U, "20000000000\0-------------------------------------------------------", 0x33},
    { 8,  2147483649U, "20000000001\0-------------------------------------------------------", 0x33},
    { 8,  4294967294U, "37777777776\0-------------------------------------------------------", 0x33},
    { 8,  4294967295U, "37777777777\0-------------------------------------------------------", 0x33},

    {10,  0x80000000U, "2147483648\0--------------------------------------------------------", 0x33},
    {10,  -2147483647, "-2147483647\0-------------------------------------------------------", 0x55},
    {10,  -2147483647, "-18446744071562067969\0---------------------------------------------", 0x00},
    {10,  -2147483647, "18446744071562067969\0----------------------------------------------", 0x22},
    {10,           -2, "-2\0----------------------------------------------------------------", 0x55},
    {10,           -2, "-18446744073709551614\0---------------------------------------------", 0x00},
    {10,           -2, "18446744073709551614\0----------------------------------------------", 0x22},
    {10,           -1, "-1\0----------------------------------------------------------------", 0x55},
    {10,           -1, "-18446744073709551615\0---------------------------------------------", 0x00},
    {10,           -1, "18446744073709551615\0----------------------------------------------", 0x22},
    {10,            0, "0\0-----------------------------------------------------------------", 0x33},
    {10,            1, "1\0-----------------------------------------------------------------", 0x33},
    {10,           12, "12\0----------------------------------------------------------------", 0x33},
    {10,          123, "123\0---------------------------------------------------------------", 0x33},
    {10,         1234, "1234\0--------------------------------------------------------------", 0x33},
    {10,        12345, "12345\0-------------------------------------------------------------", 0x33},
    {10,       123456, "123456\0------------------------------------------------------------", 0x33},
    {10,      1234567, "1234567\0-----------------------------------------------------------", 0x33},
    {10,     12345678, "12345678\0----------------------------------------------------------", 0x33},
    {10,    123456789, "123456789\0---------------------------------------------------------", 0x33},
    {10,   2147483646, "2147483646\0--------------------------------------------------------", 0x33},
    {10,   2147483647, "2147483647\0--------------------------------------------------------", 0x33},
    {10,  2147483648U, "2147483648\0--------------------------------------------------------", 0x33},
    {10,  2147483649U, "2147483649\0--------------------------------------------------------", 0x33},
    {10,  4294967294U, "4294967294\0--------------------------------------------------------", 0x33},
    {10,  4294967295U, "4294967295\0--------------------------------------------------------", 0x33},
    {10, ULL(0x2,0xdfdc1c35), "12345678901\0-------------------------------------------------------", 0x33},
    {10, ULL(0xe5,0xf4c8f374), "987654321012\0------------------------------------------------------", 0x33},
    {10, ULL(0x1c0,0xfc161e3e), "1928374656574\0-----------------------------------------------------", 0x33},
    {10, ULL(0xbad,0xcafeface), "12841062955726\0----------------------------------------------------", 0x33},
    {10, ULL(0x5bad,0xcafeface), "100801993177806\0---------------------------------------------------", 0x33},
    {10, ULL(0xaface,0xbeefcafe), "3090515640699646\0--------------------------------------------------", 0x33},
    {10, ULL(0xa5beef,0xabcdcafe), "46653307746110206\0-------------------------------------------------", 0x33},
    {10, ULL(0x1f8cf9b,0xf2df3af1), "142091656963767025\0------------------------------------------------", 0x33},
    {10, ULL(0x0fffffff,0xffffffff), "1152921504606846975\0-----------------------------------------------", 0x33},
    {10, ULL(0x7fffffff,0xffffffff), "9223372036854775807\0-----------------------------------------------", 0x33},
    {10, ULL(0x80000000,0x00000000), "-9223372036854775808\0----------------------------------------------", 0x11},
    {10, ULL(0x80000000,0x00000000), "9223372036854775808\0-----------------------------------------------", 0x22},
    {10, ULL(0x80000000,0x00000001), "-9223372036854775807\0----------------------------------------------", 0x55},
    {10, ULL(0x80000000,0x00000001), "-9223372036854775809\0----------------------------------------------", 0x00},
    {10, ULL(0x80000000,0x00000001), "9223372036854775809\0-----------------------------------------------", 0x22},
    {10, ULL(0x80000000,0x00000002), "-9223372036854775806\0----------------------------------------------", 0x55},
    {10, ULL(0x80000000,0x00000002), "-9223372036854775810\0----------------------------------------------", 0x00},
    {10, ULL(0x80000000,0x00000002), "9223372036854775810\0-----------------------------------------------", 0x22},
    {10, ULL(0xffffffff,0xfffffffe), "-2\0----------------------------------------------------------------", 0x55},
    {10, ULL(0xffffffff,0xfffffffe), "-18446744073709551614\0---------------------------------------------", 0x00},
    {10, ULL(0xffffffff,0xfffffffe), "18446744073709551614\0----------------------------------------------", 0x22},
    {10, ULL(0xffffffff,0xffffffff), "-1\0----------------------------------------------------------------", 0x55},
    {10, ULL(0xffffffff,0xffffffff), "-18446744073709551615\0---------------------------------------------", 0x00},
    {10, ULL(0xffffffff,0xffffffff), "18446744073709551615\0----------------------------------------------", 0x22},

    {16,                  0, "0\0-----------------------------------------------------------------", 0x33},
    {16,                  1, "1\0-----------------------------------------------------------------", 0x33},
    {16,         2147483646, "7ffffffe\0----------------------------------------------------------", 0x33},
    {16,         2147483647, "7fffffff\0----------------------------------------------------------", 0x33},
    {16,         0x80000000, "80000000\0----------------------------------------------------------", 0x33},
    {16,         0x80000001, "80000001\0----------------------------------------------------------", 0x33},
    {16,         0xFFFFFFFE, "fffffffe\0----------------------------------------------------------", 0x33},
    {16,         0xFFFFFFFF, "ffffffff\0----------------------------------------------------------", 0x33},
    {16, ULL(0x1,0x00000000), "100000000\0---------------------------------------------------------", 0x33},
    {16, ULL(0xbad,0xdeadbeef), "baddeadbeef\0-------------------------------------------------------", 0x33},
    {16, ULL(0x80000000,0x00000000), "8000000000000000\0--------------------------------------------------", 0x33},
    {16, ULL(0xfedcba98,0x76543210), "fedcba9876543210\0--------------------------------------------------", 0x33},
    {16, ULL(0xffffffff,0x80000001), "ffffffff80000001\0--------------------------------------------------", 0x33},
    {16, ULL(0xffffffff,0xfffffffe), "fffffffffffffffe\0--------------------------------------------------", 0x33},
    {16, ULL(0xffffffff,0xffffffff), "ffffffffffffffff\0--------------------------------------------------", 0x33},

    { 2,        32768, "1000000000000000\0--------------------------------------------------", 0x33},
    { 2,        65536, "10000000000000000\0-------------------------------------------------", 0x33},
    { 2,       131072, "100000000000000000\0------------------------------------------------", 0x33},
    {16,   0xffffffff, "ffffffff\0----------------------------------------------------------", 0x33},
    {16,          0xa, "a\0-----------------------------------------------------------------", 0x33},
    {16,            0, "0\0-----------------------------------------------------------------", 0x33},
    {20,      3368421, "111111\0------------------------------------------------------------", 0x33},
    {36,     62193781, "111111\0------------------------------------------------------------", 0x33},
    {37,     71270178, "111111\0------------------------------------------------------------", 0x33},
    {99, ULL(0x2,0x3c9e468c), "111111\0------------------------------------------------------------", 0x33},
};


static void one_i64toa_test(int test_num, const ulonglong2str_t *ulonglong2str)
{
    LPSTR result;
    char dest_str[LARGE_STRI_BUFFER_LENGTH + 1];

    memset(dest_str, '-', LARGE_STRI_BUFFER_LENGTH);
    dest_str[LARGE_STRI_BUFFER_LENGTH] = '\0';
    result = p_i64toa(ulonglong2str->value, dest_str, ulonglong2str->base);
    ok(result == dest_str,
       "(test %d): _i64toa(%s, [out], %d) has result %p, expected: %p\n",
       test_num, wine_dbgstr_longlong(ulonglong2str->value), ulonglong2str->base, result, dest_str);
    if (ulonglong2str->mask & 0x04) {
	if (memcmp(dest_str, ulonglong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) != 0) {
	    if (memcmp(dest_str, ulonglong2str[1].Buffer, LARGE_STRI_BUFFER_LENGTH) != 0) {
		ok(memcmp(dest_str, ulonglong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
		   "(test %d): _i64toa(%s, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
		   test_num, wine_dbgstr_longlong(ulonglong2str->value),
                   ulonglong2str->base, dest_str, ulonglong2str->Buffer);
	    } /* if */
	} /* if */
    } else {
	ok(memcmp(dest_str, ulonglong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
	   "(test %d): _i64toa(%s, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
	   test_num, wine_dbgstr_longlong(ulonglong2str->value),
           ulonglong2str->base, dest_str, ulonglong2str->Buffer);
    } /* if */
}


static void one_ui64toa_test(int test_num, const ulonglong2str_t *ulonglong2str)
{
    LPSTR result;
    char dest_str[LARGE_STRI_BUFFER_LENGTH + 1];

    memset(dest_str, '-', LARGE_STRI_BUFFER_LENGTH);
    dest_str[LARGE_STRI_BUFFER_LENGTH] = '\0';
    result = p_ui64toa(ulonglong2str->value, dest_str, ulonglong2str->base);
    ok(result == dest_str,
       "(test %d): _ui64toa(%s, [out], %d) has result %p, expected: %p\n",
       test_num, wine_dbgstr_longlong(ulonglong2str->value), ulonglong2str->base, result, dest_str);
    ok(memcmp(dest_str, ulonglong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
       "(test %d): _ui64toa(%s, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, wine_dbgstr_longlong(ulonglong2str->value),
       ulonglong2str->base, dest_str, ulonglong2str->Buffer);
}


static void test_ulonglongtoa(void)
{
    int test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(ulonglong2str); test_num++) {
	if (ulonglong2str[test_num].mask & 0x01) {
	    one_i64toa_test(test_num, &ulonglong2str[test_num]);
	} /* if */
        if (p_ui64toa != NULL) {
	    if (ulonglong2str[test_num].mask & 0x02) {
		one_ui64toa_test(test_num, &ulonglong2str[test_num]);
	    } /* if */
	} /* if */
    } /* for */
}


static void one_i64tow_test(int test_num, const ulonglong2str_t *ulonglong2str)
{
    int pos;
    WCHAR expected_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    WCHAR dest_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    UNICODE_STRING unicode_string;
    STRING ansi_str;
    LPWSTR result;

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulonglong2str->Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	dest_wstr[pos] = '-';
    } /* for */
    dest_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    unicode_string.Length = LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length + sizeof(WCHAR);
    unicode_string.Buffer = dest_wstr;

    result = p_i64tow(ulonglong2str->value, dest_wstr, ulonglong2str->base);
    pRtlUnicodeStringToAnsiString(&ansi_str, &unicode_string, 1);
    ok(result == dest_wstr,
       "(test %d): _i64tow(0x%s, [out], %d) has result %p, expected: %p\n",
       test_num, wine_dbgstr_longlong(ulonglong2str->value), ulonglong2str->base, result, dest_wstr);
    if (ulonglong2str->mask & 0x04) {
	if (memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) != 0) {
	    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
		expected_wstr[pos] = ulonglong2str[1].Buffer[pos];
	    } /* for */
	    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
	    if (memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) != 0) {
		ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
                   "(test %d): _i64tow(0x%s, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
		   test_num, wine_dbgstr_longlong(ulonglong2str->value),
		   ulonglong2str->base, ansi_str.Buffer, ulonglong2str->Buffer);
	    } /* if */
	} /* if */
    } else {
	ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
           "(test %d): _i64tow(0x%s, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
	   test_num, wine_dbgstr_longlong(ulonglong2str->value),
	   ulonglong2str->base, ansi_str.Buffer, ulonglong2str->Buffer);
    } /* if */
    pRtlFreeAnsiString(&ansi_str);
}


static void one_ui64tow_test(int test_num, const ulonglong2str_t *ulonglong2str)
{
    int pos;
    WCHAR expected_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    WCHAR dest_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    UNICODE_STRING unicode_string;
    STRING ansi_str;
    LPWSTR result;

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulonglong2str->Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	dest_wstr[pos] = '-';
    } /* for */
    dest_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    unicode_string.Length = LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length + sizeof(WCHAR);
    unicode_string.Buffer = dest_wstr;

    result = p_ui64tow(ulonglong2str->value, dest_wstr, ulonglong2str->base);
    pRtlUnicodeStringToAnsiString(&ansi_str, &unicode_string, 1);
    ok(result == dest_wstr,
       "(test %d): _ui64tow(0x%s, [out], %d) has result %p, expected: %p\n",
       test_num, wine_dbgstr_longlong(ulonglong2str->value),
       ulonglong2str->base, result, dest_wstr);
    ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
       "(test %d): _ui64tow(0x%s, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, wine_dbgstr_longlong(ulonglong2str->value),
       ulonglong2str->base, ansi_str.Buffer, ulonglong2str->Buffer);
    pRtlFreeAnsiString(&ansi_str);
}


static void test_ulonglongtow(void)
{
    int test_num;
    LPWSTR result;

    for (test_num = 0; test_num < ARRAY_SIZE(ulonglong2str); test_num++) {
	if (ulonglong2str[test_num].mask & 0x10) {
	    one_i64tow_test(test_num, &ulonglong2str[test_num]);
	} /* if */
	if (p_ui64tow) {
	    if (ulonglong2str[test_num].mask & 0x20) {
		one_ui64tow_test(test_num, &ulonglong2str[test_num]);
	    } /* if */
	} /* if */
    } /* for */

    if (0) {
        /* Crashes on XP and W2K3 */
        result = p_i64tow(ulonglong2str[0].value, NULL, 10);
        ok(result == NULL,
           "(test d): _i64tow(0x%s, NULL, 10) has result %p, expected: NULL\n",
           wine_dbgstr_longlong(ulonglong2str[0].value), result);
    }

    if (p_ui64tow) {
        if (0) {
            /* Crashes on XP and W2K3 */
	    result = p_ui64tow(ulonglong2str[0].value, NULL, 10);
	    ok(result == NULL,
               "(test e): _ui64tow(0x%s, NULL, 10) has result %p, expected: NULL\n",
	       wine_dbgstr_longlong(ulonglong2str[0].value), result);
        }
    } /* if */
}


typedef struct {
    const char *str;
    LONG value;
} str2long_t;

static const str2long_t str2long[] = {
    { "1011101100",   1011101100   },
    { "1234567",         1234567   },
    { "-214",               -214   },
    { "+214",                214   }, /* The + sign is allowed also */
    { "--214",                 0   }, /* Do not accept more than one sign */
    { "-+214",                 0   },
    { "++214",                 0   },
    { "+-214",                 0   },
    { "\00141",                0   }, /* not whitespace char  1 */
    { "\00242",                0   }, /* not whitespace char  2 */
    { "\00343",                0   }, /* not whitespace char  3 */
    { "\00444",                0   }, /* not whitespace char  4 */
    { "\00545",                0   }, /* not whitespace char  5 */
    { "\00646",                0   }, /* not whitespace char  6 */
    { "\00747",                0   }, /* not whitespace char  7 */
    { "\01050",                0   }, /* not whitespace char  8 */
    { "\01151",               51   }, /*  is whitespace char  9 (tab) */
    { "\01252",               52   }, /*  is whitespace char 10 (lf) */
    { "\01353",               53   }, /*  is whitespace char 11 (vt) */
    { "\01454",               54   }, /*  is whitespace char 12 (ff) */
    { "\01555",               55   }, /*  is whitespace char 13 (cr) */
    { "\01656",                0   }, /* not whitespace char 14 */
    { "\01757",                0   }, /* not whitespace char 15 */
    { "\02060",                0   }, /* not whitespace char 16 */
    { "\02161",                0   }, /* not whitespace char 17 */
    { "\02262",                0   }, /* not whitespace char 18 */
    { "\02363",                0   }, /* not whitespace char 19 */
    { "\02464",                0   }, /* not whitespace char 20 */
    { "\02565",                0   }, /* not whitespace char 21 */
    { "\02666",                0   }, /* not whitespace char 22 */
    { "\02767",                0   }, /* not whitespace char 23 */
    { "\03070",                0   }, /* not whitespace char 24 */
    { "\03171",                0   }, /* not whitespace char 25 */
    { "\03272",                0   }, /* not whitespace char 26 */
    { "\03373",                0   }, /* not whitespace char 27 */
    { "\03474",                0   }, /* not whitespace char 28 */
    { "\03575",                0   }, /* not whitespace char 29 */
    { "\03676",                0   }, /* not whitespace char 30 */
    { "\03777",                0   }, /* not whitespace char 31 */
    { "\04080",               80   }, /*  is whitespace char 32 (space) */
    { " \n \r \t214",        214   },
    { " \n \r \t+214",       214   }, /* Signs can be used after whitespace */
    { " \n \r \t-214",      -214   },
    { "+214 0",              214   }, /* Space terminates the number */
    { " 214.01",             214   }, /* Decimal point not accepted */
    { " 214,01",             214   }, /* Decimal comma not accepted */
    { "f81",                   0   },
    { "0x12345",               0   }, /* Hex not accepted */
    { "00x12345",              0   },
    { "0xx12345",              0   },
    { "1x34",                  1   },
    { "-9999999999", -1410065407   }, /* Big negative integer */
    { "-2147483649",  2147483647   }, /* Too small to fit in 32 Bits */
    { "-2147483648",  0x80000000   }, /* Smallest negative integer */
    { "-2147483647", -2147483647   },
    { "-1",                   -1   },
    { "0",                     0   },
    { "1",                     1   },
    { "2147483646",   2147483646   },
    { "2147483647",   2147483647   }, /* Largest signed positive integer */
    { "2147483648",   2147483648UL }, /* Positive int equal to smallest negative int */
    { "2147483649",   2147483649UL },
    { "4294967294",   4294967294UL },
    { "4294967295",   4294967295UL }, /* Largest unsigned integer */
    { "4294967296",            0   }, /* Too big to fit in 32 Bits */
    { "9999999999",   1410065407   }, /* Big positive integer */
    { "056789",            56789   }, /* Leading zero and still decimal */
    { "b1011101100",           0   }, /* Binary (b-notation) */
    { "-b1011101100",          0   }, /* Negative Binary (b-notation) */
    { "b10123456789",          0   }, /* Binary with nonbinary digits (2-9) */
    { "0b1011101100",          0   }, /* Binary (0b-notation) */
    { "-0b1011101100",         0   }, /* Negative binary (0b-notation) */
    { "0b10123456789",         0   }, /* Binary with nonbinary digits (2-9) */
    { "-0b10123456789",        0   }, /* Negative binary with nonbinary digits (2-9) */
    { "0b1",                   0   }, /* one digit binary */
    { "0b2",                   0   }, /* empty binary */
    { "0b",                    0   }, /* empty binary */
    { "o1234567",              0   }, /* Octal (o-notation) */
    { "-o1234567",             0   }, /* Negative Octal (o-notation) */
    { "o56789",                0   }, /* Octal with nonoctal digits (8 and 9) */
    { "0o1234567",             0   }, /* Octal (0o-notation) */
    { "-0o1234567",            0   }, /* Negative octal (0o-notation) */
    { "0o56789",               0   }, /* Octal with nonoctal digits (8 and 9) */
    { "-0o56789",              0   }, /* Negative octal with nonoctal digits (8 and 9) */
    { "0o7",                   0   }, /* one digit octal */
    { "0o8",                   0   }, /* empty octal */
    { "0o",                    0   }, /* empty octal */
    { "0d1011101100",          0   }, /* explicit decimal with 0d */
    { "x89abcdef",             0   }, /* Hex with lower case digits a-f (x-notation) */
    { "xFEDCBA00",             0   }, /* Hex with upper case digits A-F (x-notation) */
    { "-xFEDCBA00",            0   }, /* Negative Hexadecimal (x-notation) */
    { "0x89abcdef",            0   }, /* Hex with lower case digits a-f (0x-notation) */
    { "0xFEDCBA00",            0   }, /* Hex with upper case digits A-F (0x-notation) */
    { "-0xFEDCBA00",           0   }, /* Negative Hexadecimal (0x-notation) */
    { "0xabcdefgh",            0   }, /* Hex with illegal lower case digits (g-z) */
    { "0xABCDEFGH",            0   }, /* Hex with illegal upper case digits (G-Z) */
    { "0xF",                   0   }, /* one digit hexadecimal */
    { "0xG",                   0   }, /* empty hexadecimal */
    { "0x",                    0   }, /* empty hexadecimal */
    { "",                      0   }, /* empty string */
/*  { NULL,                    0   }, */ /* NULL as string */
};


static void test_wtoi(void)
{
    int test_num;
    UNICODE_STRING uni;
    int result;

    for (test_num = 0; test_num < ARRAY_SIZE(str2long); test_num++) {
	pRtlCreateUnicodeStringFromAsciiz(&uni, str2long[test_num].str);
	result = p_wtoi(uni.Buffer);
	ok(result == str2long[test_num].value,
           "(test %d): call failed: _wtoi(\"%s\") has result %d, expected: %ld\n",
	   test_num, str2long[test_num].str, result, str2long[test_num].value);
	pRtlFreeUnicodeString(&uni);
    } /* for */
}

static void test_atoi(void)
{
    int test_num;
    int result;

    for (test_num = 0; test_num < ARRAY_SIZE(str2long); test_num++) {
        result = patoi(str2long[test_num].str);
        ok(result == str2long[test_num].value,
           "(test %d): call failed: _atoi(\"%s\") has result %d, expected: %ld\n",
           test_num, str2long[test_num].str, result, str2long[test_num].value);
    }
}

static void test_atol(void)
{
    int test_num;
    int result;

    for (test_num = 0; test_num < ARRAY_SIZE(str2long); test_num++) {
        result = patol(str2long[test_num].str);
        ok(result == str2long[test_num].value,
           "(test %d): call failed: _atol(\"%s\") has result %d, expected: %ld\n",
           test_num, str2long[test_num].str, result, str2long[test_num].value);
    }
}

static void test_wtol(void)
{
    int test_num;
    UNICODE_STRING uni;
    LONG result;

    for (test_num = 0; test_num < ARRAY_SIZE(str2long); test_num++) {
	pRtlCreateUnicodeStringFromAsciiz(&uni, str2long[test_num].str);
	result = p_wtol(uni.Buffer);
	ok(result == str2long[test_num].value,
           "(test %d): call failed: _wtol(\"%s\") has result %ld, expected: %ld\n",
	   test_num, str2long[test_num].str, result, str2long[test_num].value);
	pRtlFreeUnicodeString(&uni);
    }
    result = p_wtol( L"\t\xa0\n 12" );
    ok( result == 12, "got %ld\n", result );
    result = p_wtol( L"\x3000 12" );
    ok( result == 0, "got %ld\n", result );
}


typedef struct {
    const char *str;
    LONGLONG value;
    int overflow;
} str2longlong_t;

static const str2longlong_t str2longlong[] = {
    { "1011101100",   1011101100   },
    { "1234567",         1234567   },
    { "-214",               -214   },
    { "+214",                214   }, /* The + sign is allowed also */
    { "--214",                 0   }, /* Do not accept more than one sign */
    { "-+214",                 0   },
    { "++214",                 0   },
    { "+-214",                 0   },
    { "\00141",                0   }, /* not whitespace char  1 */
    { "\00242",                0   }, /* not whitespace char  2 */
    { "\00343",                0   }, /* not whitespace char  3 */
    { "\00444",                0   }, /* not whitespace char  4 */
    { "\00545",                0   }, /* not whitespace char  5 */
    { "\00646",                0   }, /* not whitespace char  6 */
    { "\00747",                0   }, /* not whitespace char  7 */
    { "\01050",                0   }, /* not whitespace char  8 */
    { "\01151",               51   }, /*  is whitespace char  9 (tab) */
    { "\01252",               52   }, /*  is whitespace char 10 (lf) */
    { "\01353",               53   }, /*  is whitespace char 11 (vt) */
    { "\01454",               54   }, /*  is whitespace char 12 (ff) */
    { "\01555",               55   }, /*  is whitespace char 13 (cr) */
    { "\01656",                0   }, /* not whitespace char 14 */
    { "\01757",                0   }, /* not whitespace char 15 */
    { "\02060",                0   }, /* not whitespace char 16 */
    { "\02161",                0   }, /* not whitespace char 17 */
    { "\02262",                0   }, /* not whitespace char 18 */
    { "\02363",                0   }, /* not whitespace char 19 */
    { "\02464",                0   }, /* not whitespace char 20 */
    { "\02565",                0   }, /* not whitespace char 21 */
    { "\02666",                0   }, /* not whitespace char 22 */
    { "\02767",                0   }, /* not whitespace char 23 */
    { "\03070",                0   }, /* not whitespace char 24 */
    { "\03171",                0   }, /* not whitespace char 25 */
    { "\03272",                0   }, /* not whitespace char 26 */
    { "\03373",                0   }, /* not whitespace char 27 */
    { "\03474",                0   }, /* not whitespace char 28 */
    { "\03575",                0   }, /* not whitespace char 29 */
    { "\03676",                0   }, /* not whitespace char 30 */
    { "\03777",                0   }, /* not whitespace char 31 */
    { "\04080",               80   }, /*  is whitespace char 32 (space) */
    { " \n \r \t214",        214   },
    { " \n \r \t+214",       214   }, /* Signs can be used after whitespace */
    { " \n \r \t-214",      -214   },
    { "+214 0",              214   }, /* Space terminates the number */
    { " 214.01",             214   }, /* Decimal point not accepted */
    { " 214,01",             214   }, /* Decimal comma not accepted */
    { "f81",                   0   },
    { "0x12345",               0   }, /* Hex not accepted */
    { "00x12345",              0   },
    { "0xx12345",              0   },
    { "1x34",                  1   },
    { "-99999999999999999999", -ULL(0x6bc75e2d,0x630fffff), -1 }, /* Big negative integer */
    { "-9223372036854775809",   ULL(0x7fffffff,0xffffffff), -1 }, /* Too small to fit in 64 bits */
    { "-9223372036854775808",   ULL(0x80000000,0x00000000) }, /* Smallest negative 64 bit integer */
    { "-9223372036854775807",  -ULL(0x7fffffff,0xffffffff) },
    { "-9999999999",           -ULL(0x00000002,0x540be3ff) },
    { "-2147483649",           -ULL(0x00000000,0x80000001) }, /* Too small to fit in 32 bits */
    { "-2147483648",           -ULL(0x00000000,0x80000000) }, /* Smallest 32 bits negative integer */
    { "-2147483647",                           -2147483647 },
    { "-1",                                             -1 },
    { "0",                                               0 },
    { "1",                                               1 },
    { "2147483646",                             2147483646 },
    { "2147483647",                             2147483647 }, /* Largest signed positive 32 bit integer */
    { "2147483648",             ULL(0x00000000,0x80000000) }, /* Pos int equal to smallest neg 32 bit int */
    { "2147483649",             ULL(0x00000000,0x80000001) },
    { "4294967294",             ULL(0x00000000,0xfffffffe) },
    { "4294967295",             ULL(0x00000000,0xffffffff) }, /* Largest unsigned 32 bit integer */
    { "4294967296",             ULL(0x00000001,0x00000000) }, /* Too big to fit in 32 Bits */
    { "9999999999",             ULL(0x00000002,0x540be3ff) },
    { "9223372036854775806",    ULL(0x7fffffff,0xfffffffe) },
    { "9223372036854775807",    ULL(0x7fffffff,0xffffffff) }, /* Largest signed positive 64 bit integer */
    { "9223372036854775808",    ULL(0x80000000,0x00000000), 1 }, /* Pos int equal to smallest neg 64 bit int */
    { "9223372036854775809",    ULL(0x80000000,0x00000001), 1 },
    { "18446744073709551614",   ULL(0xffffffff,0xfffffffe), 1 },
    { "18446744073709551615",   ULL(0xffffffff,0xffffffff), 1 }, /* Largest unsigned 64 bit integer */
    { "18446744073709551616",                            0, 1 }, /* Too big to fit in 64 bits */
    { "99999999999999999999",   ULL(0x6bc75e2d,0x630fffff), 1 }, /* Big positive integer */
    { "056789",            56789   }, /* Leading zero and still decimal */
    { "b1011101100",           0   }, /* Binary (b-notation) */
    { "-b1011101100",          0   }, /* Negative Binary (b-notation) */
    { "b10123456789",          0   }, /* Binary with nonbinary digits (2-9) */
    { "0b1011101100",          0   }, /* Binary (0b-notation) */
    { "-0b1011101100",         0   }, /* Negative binary (0b-notation) */
    { "0b10123456789",         0   }, /* Binary with nonbinary digits (2-9) */
    { "-0b10123456789",        0   }, /* Negative binary with nonbinary digits (2-9) */
    { "0b1",                   0   }, /* one digit binary */
    { "0b2",                   0   }, /* empty binary */
    { "0b",                    0   }, /* empty binary */
    { "o1234567",              0   }, /* Octal (o-notation) */
    { "-o1234567",             0   }, /* Negative Octal (o-notation) */
    { "o56789",                0   }, /* Octal with nonoctal digits (8 and 9) */
    { "0o1234567",             0   }, /* Octal (0o-notation) */
    { "-0o1234567",            0   }, /* Negative octal (0o-notation) */
    { "0o56789",               0   }, /* Octal with nonoctal digits (8 and 9) */
    { "-0o56789",              0   }, /* Negative octal with nonoctal digits (8 and 9) */
    { "0o7",                   0   }, /* one digit octal */
    { "0o8",                   0   }, /* empty octal */
    { "0o",                    0   }, /* empty octal */
    { "0d1011101100",          0   }, /* explicit decimal with 0d */
    { "x89abcdef",             0   }, /* Hex with lower case digits a-f (x-notation) */
    { "xFEDCBA00",             0   }, /* Hex with upper case digits A-F (x-notation) */
    { "-xFEDCBA00",            0   }, /* Negative Hexadecimal (x-notation) */
    { "0x89abcdef",            0   }, /* Hex with lower case digits a-f (0x-notation) */
    { "0xFEDCBA00",            0   }, /* Hex with upper case digits A-F (0x-notation) */
    { "-0xFEDCBA00",           0   }, /* Negative Hexadecimal (0x-notation) */
    { "0xabcdefgh",            0   }, /* Hex with illegal lower case digits (g-z) */
    { "0xABCDEFGH",            0   }, /* Hex with illegal upper case digits (G-Z) */
    { "0xF",                   0   }, /* one digit hexadecimal */
    { "0xG",                   0   }, /* empty hexadecimal */
    { "0x",                    0   }, /* empty hexadecimal */
    { "",                      0   }, /* empty string */
/*  { NULL,                    0   }, */ /* NULL as string */
};


static void test_atoi64(void)
{
    int test_num;
    LONGLONG result;

    for (test_num = 0; test_num < ARRAY_SIZE(str2longlong); test_num++) {
	result = p_atoi64(str2longlong[test_num].str);
        if (str2longlong[test_num].overflow)
            ok(result == str2longlong[test_num].value ||
               (result == ((str2longlong[test_num].overflow == -1) ?
                ULL(0x80000000,0x00000000) : ULL(0x7fffffff,0xffffffff))),
               "(test %d): call failed: _atoi64(\"%s\") has result 0x%s, expected: 0x%s\n",
               test_num, str2longlong[test_num].str, wine_dbgstr_longlong(result),
               wine_dbgstr_longlong(str2longlong[test_num].value));
        else
            ok(result == str2longlong[test_num].value,
               "(test %d): call failed: _atoi64(\"%s\") has result 0x%s, expected: 0x%s\n",
               test_num, str2longlong[test_num].str, wine_dbgstr_longlong(result),
               wine_dbgstr_longlong(str2longlong[test_num].value));
    }
}


static void test_wtoi64(void)
{
    int test_num;
    UNICODE_STRING uni;
    LONGLONG result;

    for (test_num = 0; test_num < ARRAY_SIZE(str2longlong); test_num++) {
	pRtlCreateUnicodeStringFromAsciiz(&uni, str2longlong[test_num].str);
	result = p_wtoi64(uni.Buffer);
        if (str2longlong[test_num].overflow)
            ok(result == str2longlong[test_num].value ||
               (result == ((str2longlong[test_num].overflow == -1) ?
                ULL(0x80000000,0x00000000) : ULL(0x7fffffff,0xffffffff))),
               "(test %d): call failed: _atoi64(\"%s\") has result 0x%s, expected: 0x%s\n",
               test_num, str2longlong[test_num].str, wine_dbgstr_longlong(result),
               wine_dbgstr_longlong(str2longlong[test_num].value));
        else
            ok(result == str2longlong[test_num].value,
               "(test %d): call failed: _atoi64(\"%s\") has result 0x%s, expected: 0x%s\n",
               test_num, str2longlong[test_num].str, wine_dbgstr_longlong(result),
               wine_dbgstr_longlong(str2longlong[test_num].value));
	pRtlFreeUnicodeString(&uni);
    }
    result = p_wtoi64( L"\t\xa0\n 12" );
    ok( result == 12, "got %s\n", wine_dbgstr_longlong(result) );
    result = p_wtoi64( L"\x2002\x2003 12" );
    ok( result == 0, "got %s\n", wine_dbgstr_longlong(result) );
}

static void test_wcstol(void)
{
    static const struct { WCHAR str[24]; LONG res; ULONG ures; int base; } tests[] =
    {
        { L"9", 9, 9, 10 },
        { L" ", 0, 0 },
        { L"-1234", -1234, -1234 },
        { L"\x09\x0a\x0b\x0c\x0d -123", -123, -123 },
        { L"\xa0 +44", 44, 44 },
        { L"\x2002\x2003 +55", 0, 0 },
        { L"\x3000 +66", 0, 0 },
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
    static const WCHAR zeros[] =
    {
        0x0660, 0x06f0, 0x0966, 0x09e6, 0x0a66, 0x0ae6, 0x0b66, 0x0c66, 0x0ce6,
        0x0d66, 0x0e50, 0x0ed0, 0x0f20, 0x1040, 0x17e0, 0x1810, 0xff10
    };
    unsigned int i;
    LONG res;
    ULONG ures;
    WCHAR *endpos;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        res = pwcstol( tests[i].str, &endpos, tests[i].base );
        ok( res == tests[i].res, "%u: %s res %08lx\n", i, wine_dbgstr_w(tests[i].str), res );
        if (!res) ok( endpos == tests[i].str, "%u: wrong endpos %p/%p\n", i, endpos, tests[i].str );
        ures = pwcstoul( tests[i].str, &endpos, tests[i].base );
        ok( ures == tests[i].ures, "%u: %s res %08lx\n", i, wine_dbgstr_w(tests[i].str), ures );
    }

    /* Test various unicode digits */
    for (i = 0; i < ARRAY_SIZE(zeros); ++i)
    {
        WCHAR tmp[] = { zeros[i] + 4, zeros[i], zeros[i] + 5, 0 };
        res = pwcstol(tmp, NULL, 0);
        ok(res == 405, "with zero = U+%04X: got %ld, expected 405\n", zeros[i], res);
        ures = pwcstoul(tmp, NULL, 0);
        ok(ures == 405, "with zero = U+%04X: got %lu, expected 405\n", zeros[i], ures);
        tmp[1] = zeros[i] + 10;
        res = pwcstol(tmp, NULL, 16);
        ok(res == 4, "with zero = U+%04X: got %ld, expected 4\n", zeros[i], res);
        ures = pwcstoul(tmp, NULL, 16);
        ok(ures == 4, "with zero = U+%04X: got %lu, expected 4\n", zeros[i], ures);
    }
}

static void test_wcschr(void)
{
    static const WCHAR teststringW[] = {'a','b','r','a','c','a','d','a','b','r','a',0};

    ok(pwcschr(teststringW, 'a') == teststringW + 0,
       "wcschr should have returned a pointer to the first 'a' character\n");
    ok(pwcschr(teststringW, 0) == teststringW + 11,
       "wcschr should have returned a pointer to the null terminator\n");
    ok(pwcschr(teststringW, 'x') == NULL,
       "wcschr should have returned NULL\n");
}

static void test_wcsrchr(void)
{
    static const WCHAR teststringW[] = {'a','b','r','a','c','a','d','a','b','r','a',0};

    ok(pwcsrchr(teststringW, 'a') == teststringW + 10,
       "wcsrchr should have returned a pointer to the last 'a' character\n");
    ok(pwcsrchr(teststringW, 0) == teststringW + 11,
       "wcsrchr should have returned a pointer to the null terminator\n");
    ok(pwcsrchr(teststringW, 'x') == NULL,
       "wcsrchr should have returned NULL\n");
}

static void test_wcslwrupr(void)
{
    static WCHAR teststringW[] = {'a','b','r','a','c','a','d','a','b','r','a',0};
    static WCHAR emptyW[] = {0};
    static const WCHAR constemptyW[] = {0};
    WCHAR buffer[65536];
    int i;

    if (0) /* crashes on native */
    {
        static const WCHAR conststringW[] = {'a','b','r','a','c','a','d','a','b','r','a',0};
        ok(p_wcslwr((LPWSTR)conststringW) == conststringW, "p_wcslwr returned different string\n");
        ok(p_wcsupr((LPWSTR)conststringW) == conststringW, "p_wcsupr returned different string\n");
        ok(p_wcslwr(NULL) == NULL, "p_wcslwr didn't returned NULL\n");
        ok(p_wcsupr(NULL) == NULL, "p_wcsupr didn't returned NULL\n");
    }
    ok(p_wcslwr(teststringW) == teststringW, "p_wcslwr returned different string\n");
    ok(p_wcsupr(teststringW) == teststringW, "p_wcsupr returned different string\n");
    ok(p_wcslwr(emptyW) == emptyW, "p_wcslwr returned different string\n");
    ok(p_wcsupr(emptyW) == emptyW, "p_wcsupr returned different string\n");
    ok(p_wcslwr((LPWSTR)constemptyW) == constemptyW, "p_wcslwr returned different string\n");
    ok(p_wcsupr((LPWSTR)constemptyW) == constemptyW, "p_wcsupr returned different string\n");

    for (i = 0; i < 65536; i++)
    {
        WCHAR lwr = ((i >= 'A' && i <= 'Z') || (i >= 0xc0 && i <= 0xd6) || (i >= 0xd8 && i <= 0xde)) ? i + 32 : i;
        WCHAR upr = pRtlUpcaseUnicodeChar( i );
        ok( ptowlower( i ) == lwr, "%04x: towlower got %04x expected %04x\n", i, ptowlower( i ), lwr );
        ok( ptowupper( i ) == upr, "%04x: towupper got %04x expected %04x\n", i, ptowupper( i ), upr );
    }

    for (i = 1; i < 65536; i++) buffer[i - 1] = i;
    buffer[65535] = 0;
    p_wcslwr( buffer );
    for (i = 1; i < 65536; i++)
        ok( buffer[i - 1] == (i >= 'A' && i <= 'Z' ? i + 32 : i), "%04x: got %04x\n", i, buffer[i-1] );

    for (i = 1; i < 65536; i++) buffer[i - 1] = i;
    buffer[65535] = 0;
    p_wcsupr( buffer );
    for (i = 1; i < 65536; i++)
        ok( buffer[i - 1] == (i >= 'a' && i <= 'z' ? i - 32 : i), "%04x: got %04x\n", i, buffer[i-1] );
}

static void test_wcsicmp(void)
{
    WCHAR buf_a[2], buf_b[2];
    int i, j, ret;

    buf_a[1] = buf_b[1] = 0;
    for (i = 0; i < 0x300; i++)
    {
        int lwr_a = (i >= 'A' && i <= 'Z') ? i + 32 : i;
        buf_a[0] = i;
        for (j = 0; j < 0x300; j++)
        {
            int lwr_b = (j >= 'A' && j <= 'Z') ? j + 32 : j;
            buf_b[0] = j;
            ret = p_wcsicmp( buf_a, buf_b );
            ok( ret == lwr_a - lwr_b, "%04x:%04x: strings differ %d\n", i, j, ret );
            ret = p_wcsnicmp( buf_a, buf_b, 2 );
            ok( ret == lwr_a - lwr_b, "%04x:%04x: strings differ %d\n", i, j, ret );
        }
    }
}

static int __cdecl intcomparefunc(const void *a, const void *b)
{
    const int *p = a, *q = b;

    ok (a != b, "must never get the same pointer\n");

    return *p - *q;
}

static int __cdecl charcomparefunc(const void *a, const void *b)
{
    const char *p = a, *q = b;

    ok (a != b, "must never get the same pointer\n");

    return *p - *q;
}

static int __cdecl strcomparefunc(const void *a, const void *b)
{
    const char * const *p = a;
    const char * const *q = b;

    ok (a != b, "must never get the same pointer\n");

    return lstrcmpA(*p, *q);
}

static int __cdecl istrcomparefunc(const void *a, const void *b)
{
    const char * const *p = a;
    const char * const *q = b;

    ok (a != b, "must never get the same pointer\n");

    return lstrcmpiA(*p, *q);
}

static void test_qsort(void)
{
    int arr[5] = { 23, 42, 8, 4, 16 };
    char carr[5] = { 42, 23, 4, 8, 16 };
    const char *strarr[7] = {
	"Hello",
	"Wine",
	"World",
	"!",
	"Hopefully",
	"Sorted",
	"."
    };
    const char *strarr2[7] = {
        "Hello",
        "Wine",
        "World",
        "!",
        "wine",
        "Sorted",
        "WINE"
    };

    pqsort ((void*)arr, 0, sizeof(int), intcomparefunc);
    ok(arr[0] == 23, "badly sorted, nmemb=0, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, nmemb=0, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, nmemb=0, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, nmemb=0, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, nmemb=0, arr[4] is %d\n", arr[4]);

    pqsort ((void*)arr, 1, sizeof(int), intcomparefunc);
    ok(arr[0] == 23, "badly sorted, nmemb=1, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, nmemb=1, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, nmemb=1, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, nmemb=1, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, nmemb=1, arr[4] is %d\n", arr[4]);

    pqsort ((void*)arr, 5, 0, intcomparefunc);
    ok(arr[0] == 23, "badly sorted, size=0, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, size=0, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, size=0, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, size=0, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, size=0, arr[4] is %d\n", arr[4]);

    pqsort ((void*)arr, 5, sizeof(int), intcomparefunc);
    ok(arr[0] == 4,  "badly sorted, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 8,  "badly sorted, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 16, "badly sorted, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 23, "badly sorted, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 42, "badly sorted, arr[4] is %d\n", arr[4]);

    pqsort ((void*)carr, 5, sizeof(char), charcomparefunc);
    ok(carr[0] == 4,  "badly sorted, carr[0] is %d\n", carr[0]);
    ok(carr[1] == 8,  "badly sorted, carr[1] is %d\n", carr[1]);
    ok(carr[2] == 16, "badly sorted, carr[2] is %d\n", carr[2]);
    ok(carr[3] == 23, "badly sorted, carr[3] is %d\n", carr[3]);
    ok(carr[4] == 42, "badly sorted, carr[4] is %d\n", carr[4]);

    pqsort ((void*)strarr, 7, sizeof(char*), strcomparefunc);
    ok(!strcmp(strarr[0],"!"),  "badly sorted, strarr[0] is %s\n", strarr[0]);
    ok(!strcmp(strarr[1],"."),  "badly sorted, strarr[1] is %s\n", strarr[1]);
    ok(!strcmp(strarr[2],"Hello"),  "badly sorted, strarr[2] is %s\n", strarr[2]);
    ok(!strcmp(strarr[3],"Hopefully"),  "badly sorted, strarr[3] is %s\n", strarr[3]);
    ok(!strcmp(strarr[4],"Sorted"),  "badly sorted, strarr[4] is %s\n", strarr[4]);
    ok(!strcmp(strarr[5],"Wine"),  "badly sorted, strarr[5] is %s\n", strarr[5]);
    ok(!strcmp(strarr[6],"World"),  "badly sorted, strarr[6] is %s\n", strarr[6]);

    pqsort ((void*)strarr2, 7, sizeof(char*), istrcomparefunc);
    ok(!strcmp(strarr2[0], "!"),  "badly sorted, strar2r[0] is %s\n", strarr2[0]);
    ok(!strcmp(strarr2[1], "Hello"),  "badly sorted, strarr2[1] is %s\n", strarr2[1]);
    ok(!strcmp(strarr2[2], "Sorted"),  "badly sorted, strarr2[2] is %s\n", strarr2[2]);
    ok(!strcmp(strarr2[3], "wine"),  "badly sorted, strarr2[3] is %s\n", strarr2[3]);
    ok(!strcmp(strarr2[4], "WINE"),  "badly sorted, strarr2[4] is %s\n", strarr2[4]);
    ok(!strcmp(strarr2[5], "Wine"),  "badly sorted, strarr2[5] is %s\n", strarr2[5]);
    ok(!strcmp(strarr2[6], "World"),  "badly sorted, strarr2[6] is %s\n", strarr2[6]);
}

static void test_bsearch(void)
{
    int arr[7] = { 1, 3, 4, 8, 16, 23, 42 };
    int *x, l, i, j;

    /* just try all array sizes */
    for (j=1;j<ARRAY_SIZE(arr);j++) {
        for (i=0;i<j;i++) {
            l = arr[i];
            x = pbsearch (&l, arr, j, sizeof(arr[0]), intcomparefunc);
            ok (x == &arr[i], "bsearch did not find %d entry in loopsize %d.\n", i, j);
        }
        l = 4242;
        x = pbsearch (&l, arr, j, sizeof(arr[0]), intcomparefunc);
        ok (x == NULL, "bsearch did find 4242 entry in loopsize %d.\n", j);
    }
}

static void test__snprintf(void)
{
    const char *origstring = "XXXXXXXXXXXX";
    const char *teststring = "hello world";
    char buffer[32];
    int res;

    res = p_snprintf(NULL, 0, teststring);
    ok(res == lstrlenA(teststring), "_snprintf returned %d, expected %d.\n", res, lstrlenA(teststring));

    if (res != -1)
    {
        res = p_snprintf(NULL, 1, teststring);
        ok(res == lstrlenA(teststring) /* WinXP */ || res < 0 /* Vista and greater */,
           "_snprintf returned %d, expected %d or < 0.\n", res, lstrlenA(teststring));
    }
    res = p_snprintf(buffer, strlen(teststring) - 1, teststring);
    ok(res < 0, "_snprintf returned %d, expected < 0.\n", res);

    strcpy(buffer,  origstring);
    res = p_snprintf(buffer, strlen(teststring), teststring);
    ok(res == lstrlenA(teststring), "_snprintf returned %d, expected %d.\n", res, lstrlenA(teststring));
    ok(!strcmp(buffer, "hello worldX"), "_snprintf returned buffer '%s', expected 'hello worldX'.\n", buffer);

    strcpy(buffer, origstring);
    res = p_snprintf(buffer, strlen(teststring) + 1, teststring);
    ok(res == lstrlenA(teststring), "_snprintf returned %d, expected %d.\n", res, lstrlenA(teststring));
    ok(!strcmp(buffer, teststring), "_snprintf returned buffer '%s', expected '%s'.\n", buffer, teststring);

    memset(buffer, 0x7c, sizeof(buffer));
    res = p_snprintf(buffer, 4, "test");
    ok(res == 4, "res = %d\n", res);
    ok(!memcmp(buffer, "test", 4), "buf = %s\n", buffer);
    ok(buffer[4] == 0x7c, "buffer[4] = %x\n", buffer[4]);

    memset(buffer, 0x7c, sizeof(buffer));
    res = p_snprintf(buffer, 3, "test");
    ok(res == -1, "res = %d\n", res);
    ok(!memcmp(buffer, "tes", 3), "buf = %s\n", buffer);
    ok(buffer[3] == 0x7c, "buffer[3] = %x\n", buffer[3]);

    memset(buffer, 0x7c, sizeof(buffer));
    res = p_snprintf(buffer, 4, "%s", "test");
    ok(res == 4, "res = %d\n", res);
    ok(!memcmp(buffer, "test", 4), "buf = %s\n", buffer);
    ok(buffer[4] == 0x7c, "buffer[4] = %x\n", buffer[4]);

    memset(buffer, 0x7c, sizeof(buffer));
    res = p_snprintf(buffer, 3, "%s", "test");
    ok(res == -1, "res = %d\n", res);
    ok(!memcmp(buffer, "tes", 3), "buf = %s\n", buffer);
    ok(buffer[3] == 0x7c, "buffer[3] = %x\n", buffer[3]);

    res = p_snprintf(buffer, sizeof(buffer), "%ls", L"test");
    ok(res == strlen(buffer), "wrong size %d\n", res);
    ok(!strcmp(buffer, "test"), "got %s\n", debugstr_a(buffer));

    res = p_snprintf(buffer, sizeof(buffer), "%Ls", "test");
    ok(res == strlen(buffer), "wrong size %d\n", res);
    ok(!strcmp(buffer, "test"), "got %s\n", debugstr_a(buffer));

    res = p_snprintf(buffer, sizeof(buffer), "%I64x %d", (ULONGLONG)0x1234567890, 1);
    ok(res == strlen(buffer), "wrong size %d\n", res);
    ok(!strcmp(buffer, "1234567890 1"), "got %s\n", debugstr_a(buffer));

    res = p_snprintf(buffer, sizeof(buffer), "%I32x %d", 0x123456, 1);
    ok(res == strlen(buffer), "wrong size %d\n", res);
    ok(!strcmp(buffer, "123456 1"), "got %s\n", debugstr_a(buffer));

    res = p_snprintf(buffer, sizeof(buffer), "%#x %#x", 0, 1);
    ok(res == lstrlenA(buffer), "wrong size %d\n", res);
    ok(!strcmp(buffer, "0 0x1"), "got %s\n", debugstr_a(buffer));

    res = p_snprintf(buffer, sizeof(buffer), "%hx %hd", 0x123456, 987654);
    ok(res == strlen(buffer), "wrong size %d\n", res);
    ok(!strcmp(buffer, "3456 4614"), "got %s\n", debugstr_a(buffer));

    if (sizeof(void *) == 8)
    {
        res = p_snprintf(buffer, sizeof(buffer), "%Ix %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "1234567890 1"), "got %s\n", debugstr_a(buffer));

        res = p_snprintf(buffer, sizeof(buffer), "%zx %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "1234567890 1") || broken(!strcmp(buffer, "zx 878082192")),
           "got %s\n", debugstr_a(buffer));

        res = p_snprintf(buffer, sizeof(buffer), "%tx %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "1234567890 1") || broken(!strcmp(buffer, "tx 878082192")),
           "got %s\n", debugstr_a(buffer));

        res = p_snprintf(buffer, sizeof(buffer), "%jx %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "1234567890 1") || broken(!strcmp(buffer, "jx 878082192")),
           "got %s\n", debugstr_a(buffer));

        res = p_snprintf(buffer, sizeof(buffer), "%llx %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "1234567890 1"), "got %s\n", debugstr_a(buffer));
    }
    else
    {
        res = p_snprintf(buffer, sizeof(buffer), "%Ix %d", (ULONG_PTR)0x123456, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "123456 1"), "got %s\n", debugstr_a(buffer));

        res = p_snprintf(buffer, sizeof(buffer), "%zx %d", (ULONG_PTR)0x123456, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "123456 1") || broken(!strcmp(buffer, "zx 1193046")),
           "got %s\n", debugstr_a(buffer));

        res = p_snprintf(buffer, sizeof(buffer), "%tx %d", (ULONG_PTR)0x123456, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "123456 1") || broken(!strcmp(buffer, "tx 1193046")),
           "got %s\n", debugstr_a(buffer));

        res = p_snprintf(buffer, sizeof(buffer), "%jx %d", 0x1234567890ull, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "1234567890 1") || broken(!strcmp(buffer, "jx 878082192")),
           "got %s\n", debugstr_a(buffer));

        res = p_snprintf(buffer, sizeof(buffer), "%llx %d", 0x1234567890ull, 1);
        ok(res == strlen(buffer), "wrong size %d\n", res);
        ok(!strcmp(buffer, "1234567890 1") || broken(!strcmp(buffer, "34567890 18")), /* winxp */
           "got %s\n", debugstr_a(buffer));
    }
}

static void test__snprintf_s(void)
{
    char buf[32];
    int res;

    if (!p_snprintf_s)
    {
        win_skip("_snprintf_s not available\n");
        return;
    }

    memset(buf, 0xcc, sizeof(buf));
    res = p_snprintf_s(buf, sizeof(buf), sizeof(buf), "test");
    ok(res == 4, "res = %d\n", res);
    ok(!strcmp(buf, "test"), "buf = %s\n", buf);

    memset(buf, 0xcc, sizeof(buf));
    res = p_snprintf_s(buf, 4, 4, "test");
    ok(res == -1, "res = %d\n", res);
    ok(!buf[0], "buf = %s\n", buf);

    memset(buf, 0xcc, sizeof(buf));
    res = p_snprintf_s(buf, 5, 4, "test");
    ok(res == 4, "res = %d\n", res);
    ok(!strcmp(buf, "test"), "buf = %s\n", buf);

    memset(buf, 0xcc, sizeof(buf));
    res = p_snprintf_s(buf, 5, 3, "test");
    ok(res == -1, "res = %d\n", res);
    ok(!strcmp(buf, "tes"), "buf = %s\n", buf);

    memset(buf, 0xcc, sizeof(buf));
    res = p_snprintf_s(buf, 4, 10, "test");
    ok(res == -1, "res = %d\n", res);
    ok(!buf[0], "buf = %s\n", buf);

    memset(buf, 0xcc, sizeof(buf));
    res = p_snprintf_s(buf, 6, 5, "test%c", 0);
    ok(res == 5, "res = %d\n", res);
    ok(!memcmp(buf, "test\0", 6), "buf = %s\n", buf);

}

static void test__snwprintf(void)
{
    const WCHAR *origstring = L"XXXXXXXXXXXX";
    const WCHAR *teststring = L"hello world";
    WCHAR buffer[32];
    int res;

    res = p_snwprintf(NULL, 0, teststring);
    ok(res == lstrlenW(teststring), "_snprintf returned %d, expected %d.\n", res, lstrlenW(teststring));

    res = p_snwprintf(buffer, lstrlenW(teststring) - 1, teststring);
    ok(res < 0, "_snprintf returned %d, expected < 0.\n", res);

    wcscpy(buffer,  origstring);
    res = p_snwprintf(buffer, lstrlenW(teststring), teststring);
    ok(res == lstrlenW(teststring), "_snprintf returned %d, expected %d.\n", res, lstrlenW(teststring));
    ok(!wcscmp(buffer, L"hello worldX"), "_snprintf returned buffer %s, expected 'hello worldX'.\n",
       debugstr_w(buffer));

    wcscpy(buffer, origstring);
    res = p_snwprintf(buffer, lstrlenW(teststring) + 1, teststring);
    ok(res == lstrlenW(teststring), "_snprintf returned %d, expected %d.\n", res, lstrlenW(teststring));
    ok(!wcscmp(buffer, teststring), "_snprintf returned buffer %s, expected %s.\n",
       debugstr_w(buffer), debugstr_w(teststring));

    memset(buffer, 0xcc, sizeof(buffer));
    res = p_snwprintf(buffer, 4, L"test");
    ok(res == 4, "res = %d\n", res);
    ok(!memcmp(buffer, L"test", 4 * sizeof(WCHAR)), "buf = %s\n", debugstr_w(buffer));
    ok(buffer[4] == 0xcccc, "buffer[4] = %x\n", buffer[4]);

    memset(buffer, 0xcc, sizeof(buffer));
    res = p_snwprintf(buffer, 3, L"test");
    ok(res == -1, "res = %d\n", res);
    ok(!memcmp(buffer, L"tes", 3 * sizeof(WCHAR)), "buf = %s\n", debugstr_w(buffer));
    ok(buffer[3] == 0xcccc, "buffer[3] = %x\n", buffer[3]);

    memset(buffer, 0xcc, sizeof(buffer));
    res = p_snwprintf(buffer, 4, L"%s", L"test");
    ok(res == 4, "res = %d\n", res);
    ok(!memcmp(buffer, L"test", 4 * sizeof(WCHAR)), "buf = %s\n", debugstr_w(buffer));
    ok(buffer[4] == 0xcccc, "buffer[4] = %x\n", buffer[4]);

    memset(buffer, 0xcc, sizeof(buffer));
    res = p_snwprintf(buffer, 3, L"%s", L"test");
    ok(res == -1, "res = %d\n", res);
    ok(!memcmp(buffer, L"tes", 3), "buf = %s\n", debugstr_w(buffer));
    ok(buffer[3] == 0xcccc, "buffer[3] = %x\n", buffer[3]);

    res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%I64x %d", (ULONGLONG)0x1234567890, 1);
    ok(res == lstrlenW(buffer), "wrong size %d\n", res);
    ok(!wcscmp(buffer, L"1234567890 1"), "got %s\n", debugstr_w(buffer));

    res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%I32x %d", 0x123456, 1);
    ok(res == lstrlenW(buffer), "wrong size %d\n", res);
    ok(!wcscmp(buffer, L"123456 1"), "got %s\n", debugstr_w(buffer));

    res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%#x %#x", 0, 1);
    ok(res == lstrlenW(buffer), "wrong size %d\n", res);
    ok(!wcscmp(buffer, L"0 0x1"), "got %s\n", debugstr_w(buffer));

    res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%hx %hd", 0x123456, 987654);
    ok(res == lstrlenW(buffer), "wrong size %d\n", res);
    ok(!wcscmp(buffer, L"3456 4614"), "got %s\n", debugstr_w(buffer));

    if (sizeof(void *) == 8)
    {
        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%Ix %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"1234567890 1"), "got %s\n", debugstr_w(buffer));

        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%zx %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"1234567890 1") || broken(!wcscmp(buffer, L"zx 878082192")),
           "got %s\n", debugstr_w(buffer));

        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%tx %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"1234567890 1") || broken(!wcscmp(buffer, L"tx 878082192")),
           "got %s\n", debugstr_w(buffer));

        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%jx %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"1234567890 1") || broken(!wcscmp(buffer, L"jx 878082192")),
           "got %s\n", debugstr_w(buffer));

        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%llx %d", (ULONG_PTR)0x1234567890, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"1234567890 1"), "got %s\n", debugstr_w(buffer));
    }
    else
    {
        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%Ix %d", (ULONG_PTR)0x123456, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"123456 1"), "got %s\n", debugstr_w(buffer));

        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%zx %d", (ULONG_PTR)0x123456, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"123456 1") || broken(!wcscmp(buffer, L"zx 1193046")),
           "got %s\n", debugstr_w(buffer));

        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%tx %d", (ULONG_PTR)0x123456, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"123456 1") || broken(!wcscmp(buffer, L"tx 1193046")),
           "got %s\n", debugstr_w(buffer));

        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%jx %d", 0x1234567890ull, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"1234567890 1") || broken(!wcscmp(buffer, L"jx 878082192")),
           "got %s\n", debugstr_w(buffer));

        res = p_snwprintf(buffer, ARRAY_SIZE(buffer), L"%llx %d", 0x1234567890ull, 1);
        ok(res == lstrlenW(buffer), "wrong size %d\n", res);
        ok(!wcscmp(buffer, L"1234567890 1") || broken(!wcscmp(buffer, L"34567890 18")), /* winxp */
           "got %s\n", debugstr_w(buffer));
    }
}

static void test__snwprintf_s(void)
{
    WCHAR buf[32];
    int res;

    if (!p_snwprintf_s)
    {
        win_skip("_snwprintf_s not available\n");
        return;
    }

    memset(buf, 0xcc, sizeof(buf));
    res = p_snwprintf_s(buf, sizeof(buf), sizeof(buf), L"test");
    ok(res == 4, "res = %d\n", res);
    ok(!wcscmp(buf, L"test"), "buf = %s\n", debugstr_w(buf));

    memset(buf, 0xcc, sizeof(buf));
    res = p_snwprintf_s(buf, 4, 4, L"test");
    ok(res == -1, "res = %d\n", res);
    ok(!buf[0], "buf = %s\n", debugstr_w(buf));

    memset(buf, 0xcc, sizeof(buf));
    res = p_snwprintf_s(buf, 5, 4, L"test");
    ok(res == 4, "res = %d\n", res);
    ok(!wcscmp(buf, L"test"), "buf = %s\n", debugstr_w(buf));

    memset(buf, 0xcc, sizeof(buf));
    res = p_snwprintf_s(buf, 5, 3, L"test");
    ok(res == -1, "res = %d\n", res);
    ok(!wcscmp(buf, L"tes"), "buf = %s\n", debugstr_w(buf));

    memset(buf, 0xcc, sizeof(buf));
    res = p_snwprintf_s(buf, 4, 10, L"test");
    ok(res == -1, "res = %d\n", res);
    ok(!buf[0], "buf = %s\n", debugstr_w(buf));

    memset(buf, 0xcc, sizeof(buf));
    res = p_snwprintf_s(buf, 6, 5, L"test%c", 0);
    ok(res == 5, "res = %d\n", res);
    ok(!memcmp(buf, L"test\0", 6 * sizeof(WCHAR)), "buf = %s\n", debugstr_w(buf));

}

static void test_printf_format(void)
{
    const struct
    {
        const char *spec;
        unsigned int arg_size;
        const char *expected;
        const WCHAR *expectedw;
        ULONG64 arg;
        const void *argw;
    }
    tests[] =
    {
        { "%qu", 0, "qu", NULL, 10 },
        { "%ll", 0, "", NULL, 10 },
        { "%lu", sizeof(ULONG), "65537", NULL, 65537 },
        { "%llu", sizeof(ULONG64), "10", NULL, 10 },
        { "%lllllllu", sizeof(ULONG64), "10", NULL, 10 },
        { "%#lx", sizeof(ULONG), "0xa", NULL, 10 },
        { "%#llx", sizeof(ULONG64), "0x1000000000", NULL, 0x1000000000 },
        { "%#lllx", sizeof(ULONG64), "0x1000000000", NULL, 0x1000000000 },
        { "%hu", sizeof(ULONG), "1", NULL, 65537 },
        { "%hlu", sizeof(ULONG), "1", NULL, 65537 },
        { "%hllx", sizeof(ULONG64), "100000010", NULL, 0x100000010 },
        { "%hlllx", sizeof(ULONG64), "100000010", NULL, 0x100000010 },
        { "%llhx", sizeof(ULONG64), "100000010", NULL, 0x100000010 },
        { "%lllhx", sizeof(ULONG64), "100000010", NULL, 0x100000010 },
        { "%lhu", sizeof(ULONG), "1", NULL, 65537 },
        { "%hhu", sizeof(ULONG), "1", NULL, 65537 },
        { "%hwu", sizeof(ULONG), "1", NULL, 65537 },
        { "%whu", sizeof(ULONG), "1", NULL, 65537 },
        { "%##lhllwlx", sizeof(ULONG64), "0x1000000010", NULL, 0x1000000010 },
        { "%##lhlwlx", sizeof(ULONG), "0x10", NULL, 0x1000000010 },
        { "%04lhlwllx", sizeof(ULONG64), "1000000010", NULL, 0x1000000010 },
        { "%s", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)"str", L"str" },
        { "%S", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%ls", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%lS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%lls", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)"str", L"str" },
        { "%llS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%llls", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%lllS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%lllls", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)"str", L"str" },
        { "%llllS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%hs", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)"str" },
        { "%hS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)"str" },
        { "%ws", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%wS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%hhs", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)"str" },
        { "%hhS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)"str" },
        { "%wws", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%wwS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%wwws", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%wwwS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str" },
        { "%hws", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%hwS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%whs", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%whS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%hwls", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%hwlls", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%hwlS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%hwllS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%lhws", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%llhws", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%lhwS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%llhwS", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)L"str", "str" },
        { "%c", sizeof(SHORT), "\xc8", L"\x95c8", 0x95c8 },
        { "%lc", sizeof(SHORT), "\x3f", L"\x95c8", 0x95c8 },
        { "%llc", sizeof(SHORT), "\xc8", L"\x95c8", 0x95c8 },
        { "%lllc", sizeof(SHORT), "\x3f", L"\x95c8", 0x95c8 },
        { "%llllc", sizeof(SHORT), "\xc8", L"\x95c8", 0x95c8 },
        { "%lllllc", sizeof(SHORT), "\x3f", L"\x95c8", 0x95c8 },
        { "%C", sizeof(SHORT), "\x3f", L"\xc8", 0x95c8 },
        { "%lC", sizeof(SHORT), "\x3f", L"\x95c8", 0x95c8 },
        { "%llC", sizeof(SHORT), "\x3f", L"\xc8", 0x95c8 },
        { "%lllC", sizeof(SHORT), "\x3f", L"\x95c8", 0x95c8 },
        { "%llllC", sizeof(SHORT), "\x3f", L"\xc8", 0x95c8 },
        { "%lllllC", sizeof(SHORT), "\x3f", L"\x95c8", 0x95c8 },
        { "%hc", sizeof(BYTE), "\xc8", L"\xc8", 0x95c8 },
        { "%hhc", sizeof(BYTE), "\xc8", L"\xc8", 0x95c8 },
        { "%hhhc", sizeof(BYTE), "\xc8", L"\xc8", 0x95c8 },
        { "%wc", sizeof(BYTE), "\x3f", L"\x95c8", 0x95c8 },
        { "%wC", sizeof(BYTE), "\x3f", L"\x95c8", 0x95c8 },
        { "%hwc", sizeof(BYTE), "\x3f", L"\xc8", 0x95c8 },
        { "%whc", sizeof(BYTE), "\x3f", L"\xc8", 0x95c8 },
        { "%hwC", sizeof(BYTE), "\x3f", L"\xc8", 0x95c8 },
        { "%whC", sizeof(BYTE), "\x3f", L"\xc8", 0x95c8 },
        { "%I64u", sizeof(ULONG64), "10", NULL, 10 },
        { "%llI64u", sizeof(ULONG64), "10", NULL, 10 },
        { "%I64llu", sizeof(ULONG64), "10", NULL, 10 },
        { "%I64s", sizeof(ULONG_PTR), "str", NULL, (ULONG_PTR)"str", L"str" },
        { "%q%u", sizeof(ULONG), "q10", NULL, 10 },
        { "%lhw%u", 0, "%u", NULL, 10 },
        { "%u% ", sizeof(ULONG), "10", NULL, 10 },
        { "%u% %u", sizeof(ULONG), "10%u", NULL, 10 },
        { "%  ll u", 0, " u", NULL, 10 },
        { "% llu", sizeof(ULONG64), "10", NULL, 10 },
        { "%# llx", sizeof(ULONG64), "0xa", NULL, 10 },
        { "%  #llx", sizeof(ULONG64), "0xa", NULL, 10 },
    };
    WCHAR ws[256], expectedw[256], specw[256];
    unsigned int i, j;
    char expected[256], spec[256], s[256];
    int len;

    p_snprintf(s, ARRAY_SIZE(s), "%C", 0x95c8);
    p_snwprintf(ws, ARRAY_SIZE(ws), L"%c", 0x95c8);
    p_snwprintf(expectedw, ARRAY_SIZE(expectedw), L"%C", 0x95c8);
    if (s[0] != 0x3f || ws[0] != 0x95c8 || expectedw[0] != 0xc8)
    {
        /* The test is not designed for testing locale but some of the test expected results depend on locale. */
        skip("An English non-UTF8 locale is required for the printf tests.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        strcpy(spec, tests[i].spec);
        winetest_push_context("%s", spec);
        strcat(spec,"|%s");
        *s = 0;
        *ws = 0;
        j = 0;
        do
            specw[j] = spec[j];
        while (specw[j++]);
        if (tests[i].argw)
            len = p_snwprintf(ws, ~(size_t)0, specw, tests[i].argw, L"end");
        switch (tests[i].arg_size)
        {
            case 0:
                p_snprintf(s, ~(size_t)0, spec, "end");
                len = p_snwprintf(ws, ~(size_t)0, specw, L"end");
                break;
            case 1:
            case 2:
            case 4:
                p_snprintf(s, ARRAY_SIZE(s), spec, (ULONG)tests[i].arg, "end");
                if (!tests[i].argw)
                    len = p_snwprintf(ws, ~(size_t)0, specw, (ULONG)tests[i].arg, L"end");
                break;
            case 8:
                p_snprintf(s, ~(size_t)0, spec, (ULONG64)tests[i].arg, "end");
                if (!tests[i].argw)
                    len = p_snwprintf(ws, ~(size_t)0, specw, (ULONG64)tests[i].arg, L"end");
                break;
            default:
                len = 0;
                ok(0, "unknown length %u.\n", tests[i].arg_size);
                break;
        }
        strcpy(expected, tests[i].expected);
        strcat(expected, "|end");
        ok(len == strlen(expected), "got len %d, expected %Id.\n", len, strlen(expected));
        ok(!strcmp(s, expected), "got %s, expected %s.\n", debugstr_a(s), debugstr_a(expected));
        if (tests[i].expectedw)
        {
            wcscpy(expectedw, tests[i].expectedw);
            wcscat(expectedw, L"|end");
        }
        else
        {
            for (j = 0; j < len; ++j)
                expectedw[j] = expected[j];
        }
        expectedw[len] = 0;
        ok(!wcscmp(ws, expectedw), "got %s, expected %s.\n", debugstr_w(ws), debugstr_w(expectedw));
        winetest_pop_context();
    }
}

static void test_tolower(void)
{
    int i, ret, exp_ret;

    if (!GetProcAddress(GetModuleHandleA("ntdll"), "NtRemoveIoCompletionEx"))
    {
        win_skip("tolower tests\n");
        return;
    }

    ok(ptolower != NULL, "tolower is not available\n");

    for (i = -512; i < 512; i++)
    {
        exp_ret = (char)i >= 'A' && (char)i <= 'Z' ? i - 'A' + 'a' : i;
        ret = ptolower(i);
        ok(ret == exp_ret, "tolower(%d) = %d\n", i, ret);
    }
}

static void test_toupper(void)
{

    int i, ret, exp_ret;
    char str[3], *p;
    WCHAR wc;

    ok(ptoupper != NULL, "toupper is not available\n");

    for (i = -512; i < 0xffff; i++)
    {
        str[0] = i;
        str[1] = i >> 8;
        str[2] = 0;
        p = str;
        wc = RtlAnsiCharToUnicodeChar( &p );
        wc = RtlUpcaseUnicodeChar( wc );
        ret = WideCharToMultiByte( CP_ACP, 0, &wc, 1, str, 2, NULL, NULL );
        ok(!ret || ret == 1 || ret == 2, "WideCharToMultiByte returned %d\n", ret);
        if (ret == 2)
            exp_ret = (unsigned char)str[1] + ((unsigned char)str[0] << 8);
        else if (ret == 1)
            exp_ret = (unsigned char)str[0];
        else
            exp_ret = (WCHAR)i;

        ret = (WCHAR)ptoupper(i);
        ok(ret == exp_ret, "toupper(%x) = %x, expected %x\n", i, ret, exp_ret);
    }
}

static void test__strnicmp(void)
{
    BOOL is_win64 = (sizeof(void *) > sizeof(int));
    int ret;

    ok(p_strnicmp != NULL, "_strnicmp is not available\n");

    ret = p_strnicmp("a", "C", 1);
    ok(ret == (is_win64 ? -2 : -1), "_strnicmp returned %d\n", ret);
    ret = p_strnicmp("a", "c", 1);
    ok(ret == (is_win64 ? -2 : -1), "_strnicmp returned %d\n", ret);
    ret = p_strnicmp("C", "a", 1);
    ok(ret == (is_win64 ? 2 : 1), "_strnicmp returned %d\n", ret);
    ret = p_strnicmp("c", "a", 1);
    ok(ret == (is_win64 ? 2 : 1), "_strnicmp returned %d\n", ret);
    ret = p_strnicmp("ijk0", "IJK1", 3);
    ok(!ret, "_strnicmp returned %d\n", ret);
    ret = p_strnicmp("ijk0", "IJK1", 4);
    ok(ret == -1, "_strnicmp returned %d\n", ret);
    ret = p_strnicmp("ijk\0X", "IJK\0Y", 5);
    ok(!ret, "_strnicmp returned %d\n", ret);
}

static void test_sscanf(void)
{
    double d = 0.0;
    float f = 0.0f;
    int i = 0;
    int ret;

    ret = psscanf("10", "%d", &i);
    ok(ret == 1, "ret = %d\n", ret);
    ok(i == 10, "i = %d\n", i);

    ret = psscanf("10", "%f", &f);
    ok(ret == 0 || broken(ret == 1) /* xp/2003 */, "ret = %d\n", ret);
    ok(f == 0.0f, "f = %f\n", f);

    ret = psscanf("10", "%g", &f);
    ok(ret == 0 || broken(ret == 1) /* xp/2003 */, "ret = %d\n", ret);
    ok(f == 0.0f, "f = %f\n", f);

    ret = psscanf("10", "%e", &f);
    ok(ret == 0 || broken(ret == 1) /* xp/2003 */, "ret = %d\n", ret);
    ok(f == 0.0f, "f = %f\n", f);

    ret = psscanf("10", "%lf", &d);
    ok(ret == 0 || broken(ret == 1) /* xp/2003 */, "ret = %d\n", ret);
    ok(d == 0.0, "d = %lf\n", f);

    ret = psscanf("10", "%lg", &d);
    ok(ret == 0 || broken(ret == 1) /* xp/2003 */, "ret = %d\n", ret);
    ok(d == 0.0, "d = %lf\n", f);

    ret = psscanf("10", "%le", &d);
    ok(ret == 0 || broken(ret == 1) /* xp/2003 */, "ret = %d\n", ret);
    ok(d == 0.0, "d = %lf\n", f);
}

static const unsigned short wctypes[256] =
{
    /* 00 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0068, 0x0028, 0x0028, 0x0028, 0x0028, 0x0020, 0x0020,
    /* 10 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 20 */
    0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 30 */
    0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084,
    0x0084, 0x0084, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 40 */
    0x0010, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* 50 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 60 */
    0x0010, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* 70 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0010, 0x0010, 0x0010, 0x0010, 0x0020,
    /* 80 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 90 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* a0 */
    0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* b0 */
    0x0010, 0x0010, 0x0014, 0x0014, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0014, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* c0 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* d0 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0010,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0102,
    /* e0 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* f0 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0010,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102
};

static void test_wctype(void)
{
    int i;

    for (i = 0; i < 65536; i++)
    {
        unsigned short type = (i < 256 ? wctypes[i] : 0);
        ok( piswctype( i, 0xffff ) == type, "%u: wrong type %x\n", i, piswctype( i, 0xffff ));
        ok( piswalpha( i ) == (type & (C1_ALPHA|C1_LOWER|C1_UPPER)), "%u: wrong iswalpha\n", i );
        ok( piswdigit( i ) == (type & C1_DIGIT), "%u: wrong iswdigit\n", i );
        ok( piswlower( i ) == (type & C1_LOWER), "%u: wrong iswlower\n", i );
        ok( piswspace( i ) == (type & C1_SPACE), "%u: wrong iswspace\n", i );
        ok( piswxdigit( i ) == (type & C1_XDIGIT), "%u: wrong iswxdigit\n", i );
    }
}

/* we could reuse wctypes except for TAB, which doesn't have C1_BLANK for some reason... */
static const unsigned short ctypes[256] =
{
    /* 00 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0028, 0x0028, 0x0028, 0x0028, 0x0028, 0x0020, 0x0020,
    /* 10 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 20 */
    0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 30 */
    0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084,
    0x0084, 0x0084, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 40 */
    0x0010, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* 50 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 60 */
    0x0010, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* 70 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0010, 0x0010, 0x0010, 0x0010, 0x0020
};

static void test_ctype(void)
{
    int i;

    for (i = -1; i < 256; i++)
    {
        unsigned short type = (i >= 0 ? ctypes[i] : 0);
        ok( pisalnum( i ) == (type & (C1_DIGIT|C1_LOWER|C1_UPPER)), "%u: wrong isalnum %x / %x\n", i, pisalnum(i), type );
        ok( pisalpha( i ) == (type & (C1_LOWER|C1_UPPER)), "%u: wrong isalpha %x / %x\n", i, pisalpha(i), type );
        ok( piscntrl( i ) == (type & C1_CNTRL), "%u: wrong iscntrl %x / %x\n", i, piscntrl( i ), type );
        ok( pisdigit( i ) == (type & C1_DIGIT), "%u: wrong isdigit %x / %x\n", i, pisdigit( i ), type );
        ok( pisgraph( i ) == (type & (C1_DIGIT|C1_PUNCT|C1_LOWER|C1_UPPER)), "%u: wrong isgraph %x / %x\n", i, pisgraph( i ), type );
        ok( pislower( i ) == (type & C1_LOWER), "%u: wrong islower %x / %x\n", i, pislower( i ), type );
        ok( pisprint( i ) == (type & (C1_DIGIT|C1_BLANK|C1_PUNCT|C1_LOWER|C1_UPPER)), "%u: wrong isprint %x / %x\n", i, pisprint( i ), type );
        ok( pispunct( i ) == (type & C1_PUNCT), "%u: wrong ispunct %x / %x\n", i, pispunct( i ), type );
        ok( pisspace( i ) == (type & C1_SPACE), "%u: wrong isspace %x / %x\n", i, pisspace( i ), type );
        ok( pisupper( i ) == (type & C1_UPPER), "%u: wrong isupper %x / %x\n", i, pisupper( i ), type );
        ok( pisxdigit( i ) == (type & C1_XDIGIT), "%u: wrong isxdigit %x / %x\n", i, pisxdigit( i ), type );
    }
}

static void test_memchr(void)
{
    const char s[] = "ab";
    char *r;

    r = pmemchr(s, 'z', 2);
    ok(!r, "memchr returned %p, expected NULL\n", r);

    r = pmemchr(s, 'a', 2);
    ok(r == s, "memchr returned %p, expected %p\n", r, s);

    r = pmemchr(s, 0x100 + 'a', 2);
    ok(r == s, "memchr returned %p, expected %p\n", r, s);

    r = pmemchr(s, -0x100 + 'a', 2);
    ok(r == s, "memchr returned %p, expected %p\n", r, s);
}

START_TEST(string)
{
    InitFunctionPtrs();

    test_ulongtoa();
    test_ulonglongtoa();
    test_atoi64();
    test_ulongtow();
    test_ulonglongtow();
    test_wtoi();
    test_wtol();
    test_wtoi64();
    test_wcstol();
    test_wcschr();
    test_wcsrchr();
    test_wcslwrupr();
    test_atoi();
    test_atol();
    test_qsort();
    test_bsearch();
    test__snprintf();
    test__snprintf_s();
    test__snwprintf();
    test__snwprintf_s();
    test_printf_format();
    test_tolower();
    test_toupper();
    test__strnicmp();
    test_wcsicmp();
    test_sscanf();
    test_wctype();
    test_ctype();
    test_memchr();
}
