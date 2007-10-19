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

#include "ntdll_test.h"


/* Function ptrs for ntdll calls */
static HMODULE hntdll = 0;
static NTSTATUS (WINAPI *pRtlUnicodeStringToAnsiString)(STRING *, const UNICODE_STRING *, BOOLEAN);
static VOID     (WINAPI *pRtlFreeAnsiString)(PSTRING);
static BOOLEAN  (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING,LPCSTR);
static VOID     (WINAPI *pRtlFreeUnicodeString)(PUNICODE_STRING);

static int      (WINAPIV *patoi)(const char *);
static long     (WINAPIV *patol)(const char *);
static LONGLONG (WINAPIV *p_atoi64)(const char *);
static LPSTR    (WINAPIV *p_itoa)(int, LPSTR, INT);
static LPSTR    (WINAPIV *p_ltoa)(long, LPSTR, INT);
static LPSTR    (WINAPIV *p_ultoa)(unsigned long, LPSTR, INT);
static LPSTR    (WINAPIV *p_i64toa)(LONGLONG, LPSTR, INT);
static LPSTR    (WINAPIV *p_ui64toa)(ULONGLONG, LPSTR, INT);

static int      (WINAPIV *p_wtoi)(LPWSTR);
static long     (WINAPIV *p_wtol)(LPWSTR);
static LONGLONG (WINAPIV *p_wtoi64)(LPWSTR);
static LPWSTR   (WINAPIV *p_itow)(int, LPWSTR, int);
static LPWSTR   (WINAPIV *p_ltow)(long, LPWSTR, INT);
static LPWSTR   (WINAPIV *p_ultow)(unsigned long, LPWSTR, INT);
static LPWSTR   (WINAPIV *p_i64tow)(LONGLONG, LPWSTR, INT);
static LPWSTR   (WINAPIV *p_ui64tow)(ULONGLONG, LPWSTR, INT);

static long     (WINAPIV *pwcstol)(LPCWSTR, LPWSTR *, INT);
static ULONG    (WINAPIV *pwcstoul)(LPCWSTR, LPWSTR *, INT);

static LPWSTR   (WINAPIV *p_wcschr)(LPCWSTR, WCHAR);
static LPWSTR   (WINAPIV *p_wcsrchr)(LPCWSTR, WCHAR);

static void InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    ok(hntdll != 0, "LoadLibrary failed\n");
    if (hntdll) {
	pRtlUnicodeStringToAnsiString = (void *)GetProcAddress(hntdll, "RtlUnicodeStringToAnsiString");
	pRtlFreeAnsiString = (void *)GetProcAddress(hntdll, "RtlFreeAnsiString");
	pRtlCreateUnicodeStringFromAsciiz = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeStringFromAsciiz");
	pRtlFreeUnicodeString = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");

	patoi = (void *)GetProcAddress(hntdll, "atoi");
	patol = (void *)GetProcAddress(hntdll, "atol");
	p_atoi64 = (void *)GetProcAddress(hntdll, "_atoi64");
	p_itoa = (void *)GetProcAddress(hntdll, "_itoa");
	p_ltoa = (void *)GetProcAddress(hntdll, "_ltoa");
	p_ultoa = (void *)GetProcAddress(hntdll, "_ultoa");
	p_i64toa = (void *)GetProcAddress(hntdll, "_i64toa");
	p_ui64toa = (void *)GetProcAddress(hntdll, "_ui64toa");

	p_wtoi = (void *)GetProcAddress(hntdll, "_wtoi");
	p_wtol = (void *)GetProcAddress(hntdll, "_wtol");
	p_wtoi64 = (void *)GetProcAddress(hntdll, "_wtoi64");
	p_itow = (void *)GetProcAddress(hntdll, "_itow");
	p_ltow = (void *)GetProcAddress(hntdll, "_ltow");
	p_ultow = (void *)GetProcAddress(hntdll, "_ultow");
	p_i64tow = (void *)GetProcAddress(hntdll, "_i64tow");
	p_ui64tow = (void *)GetProcAddress(hntdll, "_ui64tow");

	pwcstol = (void *)GetProcAddress(hntdll, "wcstol");
	pwcstoul = (void *)GetProcAddress(hntdll, "wcstoul");

	p_wcschr= (void *)GetProcAddress(hntdll, "wcschr");
	p_wcsrchr= (void *)GetProcAddress(hntdll, "wcsrchr");
    } /* if */
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
#define NB_ULONG2STR (sizeof(ulong2str)/sizeof(*ulong2str))


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
    long value;
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
    unsigned long value;
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

    for (test_num = 0; test_num < NB_ULONG2STR; test_num++) {
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
    long value;
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
    unsigned long value;
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
    int pos;
    WCHAR expected_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    LPWSTR result;

    for (test_num = 0; test_num < NB_ULONG2STR; test_num++) {
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

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulong2str[0].Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    result = p_itow(ulong2str[0].value, NULL, 10);
    ok(result == NULL,
       "(test a): _itow(%ld, NULL, 10) has result %p, expected: NULL\n",
       ulong2str[0].value, result);

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulong2str[0].Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    result = p_ltow(ulong2str[0].value, NULL, 10);
    ok(result == NULL,
       "(test b): _ltow(%ld, NULL, 10) has result %p, expected: NULL\n",
       ulong2str[0].value, result);

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulong2str[0].Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    result = p_ultow(ulong2str[0].value, NULL, 10);
    ok(result == NULL,
       "(test c): _ultow(%ld, NULL, 10) has result %p, expected: NULL\n",
       ulong2str[0].value, result);
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
#define NB_ULONGLONG2STR (sizeof(ulonglong2str)/sizeof(*ulonglong2str))


static void one_i64toa_test(int test_num, const ulonglong2str_t *ulonglong2str)
{
    LPSTR result;
    char dest_str[LARGE_STRI_BUFFER_LENGTH + 1];

    memset(dest_str, '-', LARGE_STRI_BUFFER_LENGTH);
    dest_str[LARGE_STRI_BUFFER_LENGTH] = '\0';
    result = p_i64toa(ulonglong2str->value, dest_str, ulonglong2str->base);
    ok(result == dest_str,
       "(test %d): _i64toa(%Lu, [out], %d) has result %p, expected: %p\n",
       test_num, ulonglong2str->value, ulonglong2str->base, result, dest_str);
    if (ulonglong2str->mask & 0x04) {
	if (memcmp(dest_str, ulonglong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) != 0) {
	    if (memcmp(dest_str, ulonglong2str[1].Buffer, LARGE_STRI_BUFFER_LENGTH) != 0) {
		ok(memcmp(dest_str, ulonglong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
		   "(test %d): _i64toa(%Lu, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
		   test_num, ulonglong2str->value, ulonglong2str->base, dest_str, ulonglong2str->Buffer);
	    } /* if */
	} /* if */
    } else {
	ok(memcmp(dest_str, ulonglong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
	   "(test %d): _i64toa(%Lu, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
	   test_num, ulonglong2str->value, ulonglong2str->base, dest_str, ulonglong2str->Buffer);
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
       "(test %d): _ui64toa(%Lu, [out], %d) has result %p, expected: %p\n",
       test_num, ulonglong2str->value, ulonglong2str->base, result, dest_str);
    ok(memcmp(dest_str, ulonglong2str->Buffer, LARGE_STRI_BUFFER_LENGTH) == 0,
       "(test %d): _ui64toa(%Lu, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, ulonglong2str->value, ulonglong2str->base, dest_str, ulonglong2str->Buffer);
}


static void test_ulonglongtoa(void)
{
    int test_num;

    for (test_num = 0; test_num < NB_ULONGLONG2STR; test_num++) {
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
       "(test %d): _i64tow(%llu, [out], %d) has result %p, expected: %p\n",
       test_num, ulonglong2str->value, ulonglong2str->base, result, dest_wstr);
    if (ulonglong2str->mask & 0x04) {
	if (memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) != 0) {
	    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
		expected_wstr[pos] = ulonglong2str[1].Buffer[pos];
	    } /* for */
	    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
	    if (memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) != 0) {
		ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
		   "(test %d): _i64tow(%llu, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
		   test_num, ulonglong2str->value, ulonglong2str->base, ansi_str.Buffer, ulonglong2str->Buffer);
	    } /* if */
	} /* if */
    } else {
	ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
	   "(test %d): _i64tow(%llu, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
	   test_num, ulonglong2str->value, ulonglong2str->base, ansi_str.Buffer, ulonglong2str->Buffer);
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
       "(test %d): _ui64tow(%llu, [out], %d) has result %p, expected: %p\n",
       test_num, ulonglong2str->value, ulonglong2str->base, result, dest_wstr);
    ok(memcmp(dest_wstr, expected_wstr, LARGE_STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
       "(test %d): _ui64tow(%llu, [out], %d) assigns string \"%s\", expected: \"%s\"\n",
       test_num, ulonglong2str->value, ulonglong2str->base, ansi_str.Buffer, ulonglong2str->Buffer);
    pRtlFreeAnsiString(&ansi_str);
}


static void test_ulonglongtow(void)
{
    int test_num;
    int pos;
    WCHAR expected_wstr[LARGE_STRI_BUFFER_LENGTH + 1];
    LPWSTR result;

    for (test_num = 0; test_num < NB_ULONGLONG2STR; test_num++) {
	if (ulonglong2str[test_num].mask & 0x10) {
	    one_i64tow_test(test_num, &ulonglong2str[test_num]);
	} /* if */
	if (p_ui64tow) {
	    if (ulonglong2str[test_num].mask & 0x20) {
		one_ui64tow_test(test_num, &ulonglong2str[test_num]);
	    } /* if */
	} /* if */
    } /* for */

    for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	expected_wstr[pos] = ulong2str[0].Buffer[pos];
    } /* for */
    expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
    result = p_i64tow(ulong2str[0].value, NULL, 10);
    ok(result == NULL,
       "(test d): _i64tow(%llu, NULL, 10) has result %p, expected: NULL\n",
       ulonglong2str[0].value, result);

    if (p_ui64tow) {
        for (pos = 0; pos < LARGE_STRI_BUFFER_LENGTH; pos++) {
	    expected_wstr[pos] = ulong2str[0].Buffer[pos];
	} /* for */
	expected_wstr[LARGE_STRI_BUFFER_LENGTH] = '\0';
	result = p_ui64tow(ulong2str[0].value, NULL, 10);
	ok(result == NULL,
	   "(test e): _ui64tow(%llu, NULL, 10) has result %p, expected: NULL\n",
	   ulonglong2str[0].value, result);
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
    { "0d1011101100",          0   }, /* explizit decimal with 0d */
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
#define NB_STR2LONG (sizeof(str2long)/sizeof(*str2long))


static void test_wtoi(void)
{
    int test_num;
    UNICODE_STRING uni;
    int result;

    for (test_num = 0; test_num < NB_STR2LONG; test_num++) {
	pRtlCreateUnicodeStringFromAsciiz(&uni, str2long[test_num].str);
	result = p_wtoi(uni.Buffer);
	ok(result == str2long[test_num].value,
	   "(test %d): call failed: _wtoi(\"%s\") has result %d, expected: %ld\n",
	   test_num, str2long[test_num].str, result, str2long[test_num].value);
	pRtlFreeUnicodeString(&uni);
    } /* for */
}


static void test_wtol(void)
{
    int test_num;
    UNICODE_STRING uni;
    LONG result;

    for (test_num = 0; test_num < NB_STR2LONG; test_num++) {
	pRtlCreateUnicodeStringFromAsciiz(&uni, str2long[test_num].str);
	result = p_wtol(uni.Buffer);
	ok(result == str2long[test_num].value,
	   "(test %d): call failed: _wtol(\"%s\") has result %ld, expected: %ld\n",
	   test_num, str2long[test_num].str, result, str2long[test_num].value);
	pRtlFreeUnicodeString(&uni);
    } /* for */
}


typedef struct {
    const char *str;
    LONGLONG value;
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
    { "-99999999999999999999", -ULL(0x6bc75e2d,0x630fffff) }, /* Big negative integer */
    { "-9223372036854775809",   ULL(0x7fffffff,0xffffffff) }, /* Too small to fit in 64 bits */
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
    { "9223372036854775808",    ULL(0x80000000,0x00000000) }, /* Pos int equal to smallest neg 64 bit int */
    { "9223372036854775809",    ULL(0x80000000,0x00000001) },
    { "18446744073709551614",   ULL(0xffffffff,0xfffffffe) },
    { "18446744073709551615",   ULL(0xffffffff,0xffffffff) }, /* Largest unsigned 64 bit integer */
    { "18446744073709551616",                            0 }, /* Too big to fit in 64 bits */
    { "99999999999999999999",   ULL(0x6bc75e2d,0x630fffff) }, /* Big positive integer */
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
    { "0d1011101100",          0   }, /* explizit decimal with 0d */
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
#define NB_STR2LONGLONG (sizeof(str2longlong)/sizeof(*str2longlong))


static void test_atoi64(void)
{
    int test_num;
    LONGLONG result;

    for (test_num = 0; test_num < NB_STR2LONGLONG; test_num++) {
	result = p_atoi64(str2longlong[test_num].str);
	ok(result == str2longlong[test_num].value,
	   "(test %d): call failed: _atoi64(\"%s\") has result %lld, expected: %lld\n",
	   test_num, str2longlong[test_num].str, result, str2longlong[test_num].value);
    } /* for */
}


static void test_wtoi64(void)
{
    int test_num;
    UNICODE_STRING uni;
    LONGLONG result;

    for (test_num = 0; test_num < NB_STR2LONGLONG; test_num++) {
	pRtlCreateUnicodeStringFromAsciiz(&uni, str2longlong[test_num].str);
	result = p_wtoi64(uni.Buffer);
	ok(result == str2longlong[test_num].value,
	   "(test %d): call failed: _wtoi64(\"%s\") has result %lld, expected: %lld\n",
	   test_num, str2longlong[test_num].str, result, str2longlong[test_num].value);
	pRtlFreeUnicodeString(&uni);
    } /* for */
}

static void test_wcsfuncs(void)
{
    static const WCHAR testing[] = {'T','e','s','t','i','n','g',0};
    ok (p_wcschr(testing,0)!=NULL, "wcschr Not finding terminating character\n");
    ok (p_wcsrchr(testing,0)!=NULL, "wcsrchr Not finding terminating character\n");
}

START_TEST(string)
{
    InitFunctionPtrs();

    if (p_ultoa)
        test_ulongtoa();
    if (p_ui64toa)
        test_ulonglongtoa();
    if (p_atoi64)
        test_atoi64();
    if (p_ultow)
        test_ulongtow();
    if (p_ui64tow)
        test_ulonglongtow();
    if (p_wtoi)
        test_wtoi();
    if (p_wtol)
        test_wtol();
    if (p_wtoi64)
        test_wtoi64();
    if (p_wcschr && p_wcsrchr)
        test_wcsfuncs();
}
