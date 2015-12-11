/*
 * Unit test suite for MLANG APIs.
 *
 * Copyright 2004 Dmitry Timoshkov
 * Copyright 2009 Detlef Riekenberg
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "initguid.h"
#include "mlang.h"

#include "wine/test.h"

#ifndef CP_UNICODE
#define CP_UNICODE 1200
#endif

/* #define DUMP_CP_INFO */
/* #define DUMP_SCRIPT_INFO */

static BOOL (WINAPI *pGetCPInfoExA)(UINT, DWORD, LPCPINFOEXA);
static HRESULT (WINAPI *pConvertINetMultiByteToUnicode)(LPDWORD, DWORD, LPCSTR,
                                                        LPINT, LPWSTR, LPINT);
static HRESULT (WINAPI *pConvertINetUnicodeToMultiByte)(LPDWORD, DWORD, LPCWSTR,
                                                        LPINT, LPSTR, LPINT);
static HRESULT (WINAPI *pRfc1766ToLcidA)(LCID *, LPCSTR);
static HRESULT (WINAPI *pLcidToRfc1766A)(LCID, LPSTR, INT);

typedef struct lcid_tag_table {
    LPCSTR rfc1766;
    LCID lcid;
    HRESULT hr;
    LCID broken_lcid;
    LPCSTR broken_rfc;
} lcid_table_entry;

/* en, ar and zh use SUBLANG_NEUTRAL for the rfc1766 name without the country
   all others suppress the country with SUBLANG_DEFAULT.
   For 3 letter language codes, the rfc1766 is too small for the country */

static const lcid_table_entry  lcid_table[] = {
    {"e",     -1,       E_FAIL},
    {"",      -1,       E_FAIL},
    {"-",     -1,       E_FAIL},
    {"e-",    -1,       E_FAIL},

    {"ar",    1,        S_OK},
    {"zh",    4,        S_OK},

    {"de",    0x0407,   S_OK},
    {"de-ch", 0x0807,   S_OK},
    {"de-at", 0x0c07,   S_OK},
    {"de-lu", 0x1007,   S_OK},
    {"de-li", 0x1407,   S_OK},

    {"en",    9,        S_OK},
    {"en-gb", 0x809,    S_OK},
    {"en-GB", 0x809,    S_OK},
    {"EN-GB", 0x809,    S_OK},
    {"en-US", 0x409,    S_OK},
    {"en-us", 0x409,    S_OK},

    {"fr",    0x040c,   S_OK},
    {"fr-be", 0x080c,   S_OK},
    {"fr-ca", 0x0c0c,   S_OK},
    {"fr-ch", 0x100c,   S_OK},
    {"fr-lu", 0x140c,   S_OK},
    {"fr-mc", 0x180c,   S_OK, 0x040c, "fr"},

    {"it",    0x0410,   S_OK},
    {"it-ch", 0x0810,   S_OK},

    {"nl",    0x0413,   S_OK},
    {"nl-be", 0x0813,   S_OK},
    {"pl",    0x0415,   S_OK},
    {"ru",    0x0419,   S_OK},

    {"kok",   0x0457,   S_OK, 0x0412, "x-kok"}

};

#define TODO_NAME 1

typedef struct info_table_tag {
    LCID lcid;
    LANGID lang;
    DWORD todo;
    LPCSTR rfc1766;
    LPCWSTR localename;
    LPCWSTR broken_name;
} info_table_entry;

static const WCHAR de_en[] =   {'E','n','g','l','i','s','c','h',0};
static const WCHAR de_enca[] = {'E','n','g','l','i','s','c','h',' ',
                                '(','K','a','n','a','d','a',')',0};
static const WCHAR de_engb[] = {'E','n','g','l','i','s','c','h',' ',
                                '(','G','r','o',0xDF,'b','r','i','t','a','n','n','i','e','n',')',0};
static const WCHAR de_engb2[] ={'E','n','g','l','i','s','c','h',' ',
                                '(','V','e','r','e','i','n','i','g','t','e','s',' ',
                                'K',0xF6,'n','i','g','r','e','i','c',0};
static const WCHAR de_enus[] = {'E','n','g','l','i','s','c','h',' ',
                                '(','U','S','A',')',0};
static const WCHAR de_enus2[] ={'E','n','g','l','i','s','c','h',' ',
                                '(','V','e','r','e','i','n','i','g','t','e',' ',
                                'S','t','a','a','t','e','n',')',0};
static const WCHAR de_de[] =   {'D','e','u','t','s','c','h',' ',
                                '(','D','e','u','t','s','c','h','l','a','n','d',')',0};
static const WCHAR de_deat[] = {'D','e','u','t','s','c','h',' ',
                                '(',0xD6,'s','t','e','r','r','e','i','c','h',')',0};
static const WCHAR de_dech[] = {'D','e','u','t','s','c','h',' ',
                                '(','S','c','h','w','e','i','z',')',0};

static const WCHAR en_en[] =   {'E','n','g','l','i','s','h',0};
static const WCHAR en_enca[] = {'E','n','g','l','i','s','h',' ',
                                '(','C','a','n','a','d','a',')',0};
static const WCHAR en_engb[] = {'E','n','g','l','i','s','h',' ',
                                '(','U','n','i','t','e','d',' ','K','i','n','g','d','o','m',')',0};
static const WCHAR en_enus[] = {'E','n','g','l','i','s','h',' ',
                                '(','U','n','i','t','e','d',' ','S','t','a','t','e','s',')',0};
static const WCHAR en_de[] =   {'G','e','r','m','a','n',' ',
                                '(','G','e','r','m','a','n','y',')',0};
static const WCHAR en_deat[] = {'G','e','r','m','a','n',' ',
                                '(','A','u','s','t','r','i','a',')',0};
static const WCHAR en_dech[] = {'G','e','r','m','a','n',' ',
                                '(','S','w','i','t','z','e','r','l','a','n','d',')',0};

static const WCHAR fr_en[] =   {'A','n','g','l','a','i','s',0};
static const WCHAR fr_enca[] = {'A','n','g','l','a','i','s',' ',
                                '(','C','a','n','a','d','a',')',0};
static const WCHAR fr_engb[] = {'A','n','g','l','a','i','s',' ',
                                '(','R','o','y','a','u','m','e','-','U','n','i',')',0};
static const WCHAR fr_enus[] = {'A','n','g','l','a','i','s',' ',
                                '(',0xC9, 't','a','t','s','-','U','n','i','s',')',0};
static const WCHAR fr_enus2[] ={'A','n','g','l','a','i','s',' ',
                                '(','U','.','S','.',')',0};
static const WCHAR fr_de[] =   {'A','l','l','e','m','a','n','d',' ',
                                '(','A','l','l','e','m','a','g','n','e',')',0};
static const WCHAR fr_de2[] =  {'A','l','l','e','m','a','n','d',' ',
                                '(','S','t','a','n','d','a','r','d',')',0};
static const WCHAR fr_deat[] = {'A','l','l','e','m','a','n','d',' ',
                                '(','A','u','t','r','i','c','h','e',')',0};
static const WCHAR fr_dech[] = {'A','l','l','e','m','a','n','d',' ',
                                '(','S','u','i','s','s','e',')',0};

static const info_table_entry  info_table[] = {
    {MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),        MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "en", en_en},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),        MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "en-us", en_enus},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK),     MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "en-gb", en_engb},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),     MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "en-us", en_enus},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_CAN),    MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "en-ca", en_enca},

    {MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),         MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "de", en_de},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN),          MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "de", en_de},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_SWISS),    MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "de-ch", en_dech},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_AUSTRIAN), MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),
         0, "de-at", en_deat},

    {MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),        MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "en", de_en},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),        MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "en-us", de_enus, de_enus2},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK),     MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "en-gb", de_engb, de_engb2},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),     MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "en-us", de_enus, de_enus2},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_CAN),    MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "en-ca", de_enca},

    {MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),         MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "de", de_de},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN),          MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "de",de_de},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_SWISS),    MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "de-ch", de_dech},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_AUSTRIAN), MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),
         TODO_NAME, "de-at", de_deat},

    {MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL),        MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "en", fr_en},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),        MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "en-us", fr_enus, fr_enus2},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK),     MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "en-gb", fr_engb},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),     MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "en-us", fr_enus, fr_enus2},
    {MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_CAN),    MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "en-ca", fr_enca},

    {MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT),         MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "de", fr_de, fr_de2},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN),          MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "de", fr_de, fr_de2},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_SWISS),    MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "de-ch", fr_dech},
    {MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_AUSTRIAN), MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT),
         TODO_NAME, "de-at", fr_deat}

};

static BOOL init_function_ptrs(void)
{
    HMODULE hMlang;

    hMlang = GetModuleHandleA("mlang.dll");
    pConvertINetMultiByteToUnicode = (void *)GetProcAddress(hMlang, "ConvertINetMultiByteToUnicode");
    pConvertINetUnicodeToMultiByte = (void *)GetProcAddress(hMlang, "ConvertINetUnicodeToMultiByte");
    pRfc1766ToLcidA = (void *)GetProcAddress(hMlang, "Rfc1766ToLcidA");
    pLcidToRfc1766A = (void *)GetProcAddress(hMlang, "LcidToRfc1766A");

    pGetCPInfoExA = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCPInfoExA");

    return TRUE;
}

#define ok_w2(format, szString1, szString2) \
\
    if (lstrcmpW((szString1), (szString2)) != 0) \
    { \
        CHAR string1[256], string2[256]; \
        WideCharToMultiByte(CP_ACP, 0, (szString1), -1, string1, 256, NULL, NULL); \
        WideCharToMultiByte(CP_ACP, 0, (szString2), -1, string2, 256, NULL, NULL); \
        ok(0, (format), string1, string2); \
    }

static void test_multibyte_to_unicode_translations(IMultiLanguage2 *iML2)
{
    /* these APIs are broken regarding constness of the input buffer */
    char stringA[] = "Just a test string\0"; /* double 0 for CP_UNICODE tests */
    WCHAR stringW[] = {'J','u','s','t',' ','a',' ','t','e','s','t',' ','s','t','r','i','n','g',0};
    char bufA[256];
    WCHAR bufW[256];
    UINT lenA, lenW, expected_len;
    HRESULT ret;

    /* IMultiLanguage2_ConvertStringToUnicode tests */

    memset(bufW, 'x', sizeof(bufW));
    lenA = 0;
    lenW = sizeof(bufW)/sizeof(bufW[0]);
    ret = IMultiLanguage2_ConvertStringToUnicode(iML2, NULL, 1252, stringA, &lenA, bufW, &lenW);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringToUnicode failed: %08x\n", ret);
    ok(lenA == 0, "expected lenA 0, got %u\n", lenA);
    ok(lenW == 0, "expected lenW 0, got %u\n", lenW);

    memset(bufW, 'x', sizeof(bufW));
    lenA = -1;
    lenW = sizeof(bufW)/sizeof(bufW[0]);
    ret = IMultiLanguage2_ConvertStringToUnicode(iML2, NULL, 1252, stringA, &lenA, bufW, &lenW);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringToUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA), "expected lenA %u, got %u\n", lstrlenA(stringA), lenA);
    ok(lenW == lstrlenW(stringW), "expected lenW %u, got %u\n", lstrlenW(stringW), lenW);
    if (lenW < sizeof(bufW)/sizeof(bufW[0])) {
       /* can only happen if the convert call fails */
       ok(bufW[lenW] != 0, "buf should not be 0 terminated\n");
       bufW[lenW] = 0; /* -1 doesn't include 0 terminator */
    }
    ok(!lstrcmpW(bufW, stringW), "bufW/stringW mismatch\n");

    memset(bufW, 'x', sizeof(bufW));
    lenA = -1;
    lenW = 5;
    ret = IMultiLanguage2_ConvertStringToUnicode(iML2, NULL, 1252, stringA, &lenA, bufW, &lenW);
    ok(ret == E_FAIL, "IMultiLanguage2_ConvertStringToUnicode should fail: %08x\n", ret);
    ok(lenW == 0, "expected lenW 0, got %u\n", lenW);
    /* still has to do partial conversion */
    ok(!memcmp(bufW, stringW, 5 * sizeof(WCHAR)), "bufW/stringW mismatch\n");

    memset(bufW, 'x', sizeof(bufW));
    lenA = -1;
    lenW = sizeof(bufW)/sizeof(bufW[0]);
    ret = IMultiLanguage2_ConvertStringToUnicode(iML2, NULL, CP_UNICODE, stringA, &lenA, bufW, &lenW);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringToUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA), "expected lenA %u, got %u\n", lstrlenA(stringA), lenA);
    ok(lenW == lstrlenW(stringW)/(int)sizeof(WCHAR), "wrong lenW %u\n", lenW);
    ok(bufW[lenW] != 0, "buf should not be 0 terminated\n");
    bufW[lenW] = 0; /* -1 doesn't include 0 terminator */
    ok(!lstrcmpA((LPCSTR)bufW, stringA), "bufW/stringA mismatch\n");

    memset(bufW, 'x', sizeof(bufW));
    lenA = lstrlenA(stringA);
    lenW = 0;
    ret = IMultiLanguage2_ConvertStringToUnicode(iML2, NULL, 1252, stringA, &lenA, NULL, &lenW);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringToUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA), "expected lenA %u, got %u\n", lstrlenA(stringA), lenA);
    expected_len = MultiByteToWideChar(1252, 0, stringA, lenA, NULL, 0);
    ok(lenW == expected_len, "expected lenW %u, got %u\n", expected_len, lenW);

    memset(bufW, 'x', sizeof(bufW));
    lenA = lstrlenA(stringA);
    lenW = sizeof(bufW)/sizeof(bufW[0]);
    ret = pConvertINetMultiByteToUnicode(NULL, 1252, stringA, (INT *)&lenA, NULL, (INT *)&lenW);
    ok(ret == S_OK, "ConvertINetMultiByteToUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA), "expected lenA %u, got %u\n", lstrlenA(stringA), lenA);
    expected_len = MultiByteToWideChar(1252, 0, stringA, lenA, NULL, 0);
    ok(lenW == expected_len, "expected lenW %u, got %u\n", expected_len, lenW);

    memset(bufW, 'x', sizeof(bufW));
    lenA = lstrlenA(stringA);
    lenW = 0;
    ret = pConvertINetMultiByteToUnicode(NULL, 1252, stringA, (INT *)&lenA, NULL, (INT *)&lenW);
    ok(ret == S_OK, "ConvertINetMultiByteToUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA), "expected lenA %u, got %u\n", lstrlenA(stringA), lenA);
    expected_len = MultiByteToWideChar(1252, 0, stringA, lenA, NULL, 0);
    ok(lenW == expected_len, "expected lenW %u, got %u\n", expected_len, lenW);

    /* IMultiLanguage2_ConvertStringFromUnicode tests */

    memset(bufA, 'x', sizeof(bufA));
    lenW = 0;
    lenA = sizeof(bufA);
    ret = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, stringW, &lenW, bufA, &lenA);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringFromUnicode failed: %08x\n", ret);
    ok(lenA == 0, "expected lenA 0, got %u\n", lenA);
    ok(lenW == 0, "expected lenW 0, got %u\n", lenW);

    memset(bufA, 'x', sizeof(bufA));
    lenW = -1;
    lenA = sizeof(bufA);
    ret = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, stringW, &lenW, bufA, &lenA);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringFromUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA), "expected lenA %u, got %u\n", lstrlenA(stringA), lenA);
    ok(lenW == lstrlenW(stringW), "expected lenW %u, got %u\n", lstrlenW(stringW), lenW);
    ok(bufA[lenA] != 0, "buf should not be 0 terminated\n");
    bufA[lenA] = 0; /* -1 doesn't include 0 terminator */
    ok(!lstrcmpA(bufA, stringA), "bufA/stringA mismatch\n");

    memset(bufA, 'x', sizeof(bufA));
    lenW = -1;
    lenA = 5;
    ret = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, stringW, &lenW, bufA, &lenA);
    ok(ret == E_FAIL, "IMultiLanguage2_ConvertStringFromUnicode should fail: %08x\n", ret);
    ok(lenA == 0, "expected lenA 0, got %u\n", lenA);
    /* still has to do partial conversion */
    ok(!memcmp(bufA, stringA, 5), "bufW/stringW mismatch\n");

    memset(bufA, 'x', sizeof(bufA));
    lenW = -1;
    lenA = sizeof(bufA);
    ret = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, CP_UNICODE, stringW, &lenW, bufA, &lenA);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringFromUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA) * (int)sizeof(WCHAR), "wrong lenA %u\n", lenA);
    ok(lenW == lstrlenW(stringW), "expected lenW %u, got %u\n", lstrlenW(stringW), lenW);
    ok(bufA[lenA] != 0 && bufA[lenA+1] != 0, "buf should not be 0 terminated\n");
    bufA[lenA] = 0; /* -1 doesn't include 0 terminator */
    bufA[lenA+1] = 0; /* sizeof(WCHAR) */
    ok(!lstrcmpW((LPCWSTR)bufA, stringW), "bufA/stringW mismatch\n");

    memset(bufA, 'x', sizeof(bufA));
    lenW = lstrlenW(stringW);
    lenA = 0;
    ret = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, stringW, &lenW, NULL, &lenA);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringFromUnicode failed: %08x\n", ret);
    ok(lenW == lstrlenW(stringW), "expected lenW %u, got %u\n", lstrlenW(stringW), lenW);
    expected_len = WideCharToMultiByte(1252, 0, stringW, lenW, NULL, 0, NULL, NULL);
    ok(lenA == expected_len, "expected lenA %u, got %u\n", expected_len, lenA);
}

static void cpinfo_cmp(MIMECPINFO *cpinfo1, MIMECPINFO *cpinfo2)
{
    ok(cpinfo1->dwFlags == cpinfo2->dwFlags, "dwFlags mismatch: %08x != %08x\n", cpinfo1->dwFlags, cpinfo2->dwFlags);
    ok(cpinfo1->uiCodePage == cpinfo2->uiCodePage, "uiCodePage mismatch: %u != %u\n", cpinfo1->uiCodePage, cpinfo2->uiCodePage);
    ok(cpinfo1->uiFamilyCodePage == cpinfo2->uiFamilyCodePage, "uiFamilyCodePage mismatch: %u != %u\n", cpinfo1->uiFamilyCodePage, cpinfo2->uiFamilyCodePage);
    ok(!lstrcmpW(cpinfo1->wszDescription, cpinfo2->wszDescription), "wszDescription mismatch\n");
    ok(!lstrcmpW(cpinfo1->wszWebCharset, cpinfo2->wszWebCharset), "wszWebCharset mismatch\n");
    ok(!lstrcmpW(cpinfo1->wszHeaderCharset, cpinfo2->wszHeaderCharset), "wszHeaderCharset mismatch\n");
    ok(!lstrcmpW(cpinfo1->wszBodyCharset, cpinfo2->wszBodyCharset), "wszBodyCharset mismatch\n");
    ok(!lstrcmpW(cpinfo1->wszFixedWidthFont, cpinfo2->wszFixedWidthFont), "wszFixedWidthFont mismatch\n");
    ok(!lstrcmpW(cpinfo1->wszProportionalFont, cpinfo2->wszProportionalFont), "wszProportionalFont mismatch\n");
    ok(cpinfo1->bGDICharset == cpinfo2->bGDICharset, "bGDICharset mismatch: %d != %d\n", cpinfo1->bGDICharset, cpinfo2->bGDICharset);
}

#ifdef DUMP_CP_INFO
static const char *dump_mime_flags(DWORD flags)
{
    static char buf[1024];

    buf[0] = 0;

    if (flags & MIMECONTF_MAILNEWS) strcat(buf, " MIMECONTF_MAILNEWS");
    if (flags & MIMECONTF_BROWSER) strcat(buf, " MIMECONTF_BROWSER");
    if (flags & MIMECONTF_MINIMAL) strcat(buf, " MIMECONTF_MINIMAL");
    if (flags & MIMECONTF_IMPORT) strcat(buf, " MIMECONTF_IMPORT");
    if (flags & MIMECONTF_SAVABLE_MAILNEWS) strcat(buf, " MIMECONTF_SAVABLE_MAILNEWS");
    if (flags & MIMECONTF_SAVABLE_BROWSER) strcat(buf, " MIMECONTF_SAVABLE_BROWSER");
    if (flags & MIMECONTF_EXPORT) strcat(buf, " MIMECONTF_EXPORT");
    if (flags & MIMECONTF_PRIVCONVERTER) strcat(buf, " MIMECONTF_PRIVCONVERTER");
    if (flags & MIMECONTF_VALID) strcat(buf, " MIMECONTF_VALID");
    if (flags & MIMECONTF_VALID_NLS) strcat(buf, " MIMECONTF_VALID_NLS");
    if (flags & MIMECONTF_MIME_IE4) strcat(buf, " MIMECONTF_MIME_IE4");
    if (flags & MIMECONTF_MIME_LATEST) strcat(buf, " MIMECONTF_MIME_LATEST");
    if (flags & MIMECONTF_MIME_REGISTRY) strcat(buf, " MIMECONTF_MIME_REGISTRY");

    return buf;
}
#endif

static HRESULT check_convertible(IMultiLanguage2 *iML2, UINT from, UINT to)
{
    CHAR convert[MAX_PATH];
    BYTE dest[MAX_PATH];
    HRESULT hr;
    UINT srcsz, destsz;

    static WCHAR strW[] = {'a','b','c',0};

    /* Check to see if the target codepage has these characters or not */
    if (from != CP_UTF8)
    {
        BOOL fDefaultChar;
        char ch[10];
        int cb;
        cb = WideCharToMultiByte( from, WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                                  strW, 3, ch, sizeof(ch), NULL, &fDefaultChar);

        if(cb == 0 || fDefaultChar)
        {
            trace("target codepage %i does not contain 'abc'\n",from);
            return E_FAIL;
        }
    }

    srcsz = lstrlenW(strW) + 1;
    destsz = MAX_PATH;
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, from, strW,
                                                  &srcsz, convert, &destsz);
    if (hr != S_OK)
        return S_FALSE;

    srcsz = -1;
    destsz = MAX_PATH;
    hr = IMultiLanguage2_ConvertString(iML2, NULL, from, to, (BYTE *)convert,
                                       &srcsz, dest, &destsz);
    if (hr != S_OK)
        return S_FALSE;

    return S_OK;
}

static void test_EnumCodePages(IMultiLanguage2 *iML2, DWORD flags)
{
    IEnumCodePage *iEnumCP = NULL;
    MIMECPINFO *cpinfo;
    MIMECPINFO cpinfo2;
    HRESULT ret;
    ULONG i, n;
    UINT total;

    total = 0;
    ret = IMultiLanguage2_GetNumberOfCodePageInfo(iML2, &total);
    ok(ret == S_OK && total != 0, "IMultiLanguage2_GetNumberOfCodePageInfo: expected S_OK/!0, got %08x/%u\n", ret, total);

    trace("total mlang supported codepages %u\n", total);

    ret = IMultiLanguage2_EnumCodePages(iML2, flags, LANG_NEUTRAL, &iEnumCP);
    ok(ret == S_OK && iEnumCP, "IMultiLanguage2_EnumCodePages: expected S_OK/!NULL, got %08x/%p\n", ret, iEnumCP);

    ret = IEnumCodePage_Reset(iEnumCP);
    ok(ret == S_OK, "IEnumCodePage_Reset: expected S_OK, got %08x\n", ret);
    n = 65536;
    ret = IEnumCodePage_Next(iEnumCP, 0, NULL, &n);
    ok(ret == S_FALSE || ret == E_FAIL,
            "IEnumCodePage_Next: expected S_FALSE or E_FAIL, got %08x\n", ret);
    if (ret == S_FALSE)
        ok(n == 0, "IEnumCodePage_Next: expected 0/S_FALSE, got %u/%08x\n", n, ret);
    else if (ret == E_FAIL)
        ok(n == 65536, "IEnumCodePage_Next: expected 65536/E_FAIL, got %u/%08x\n", n, ret);
    ret = IEnumCodePage_Next(iEnumCP, 0, NULL, NULL);
    ok(ret == S_FALSE || ret == E_FAIL,
            "IEnumCodePage_Next: expected S_FALSE or E_FAIL, got %08x\n", ret);

    cpinfo = HeapAlloc(GetProcessHeap(), 0, sizeof(*cpinfo) * total * 2);

    n = total * 2;
    ret = IEnumCodePage_Next(iEnumCP, 0, cpinfo, &n);
    ok(ret == S_FALSE && n == 0, "IEnumCodePage_Next: expected S_FALSE/0, got %08x/%u\n", ret, n);

    n = total * 2;
    ret = IEnumCodePage_Next(iEnumCP, n, cpinfo, &n);
    ok(ret == S_OK && n != 0, "IEnumCodePage_Next: expected S_OK/!0, got %08x/%u\n", ret, n);

    trace("flags %08x, enumerated codepages %u\n", flags, n);

    if (!flags)
    {
	ok(n == total, "IEnumCodePage_Next: expected %u, got %u\n", total, n);

	flags = MIMECONTF_MIME_LATEST;
    }

    total = n;

    for (i = 0; i < n; i++)
    {
	CHARSETINFO csi;
	MIMECSETINFO mcsi;
        HRESULT convertible = S_OK;
	static const WCHAR autoW[] = {'_','a','u','t','o',0};
	static const WCHAR feffW[] = {'u','n','i','c','o','d','e','F','E','F','F',0};

#ifdef DUMP_CP_INFO
	trace("MIMECPINFO #%u:\n"
	      "dwFlags %08x %s\n"
	      "uiCodePage %u\n"
	      "uiFamilyCodePage %u\n"
	      "wszDescription %s\n"
	      "wszWebCharset %s\n"
	      "wszHeaderCharset %s\n"
	      "wszBodyCharset %s\n"
	      "wszFixedWidthFont %s\n"
	      "wszProportionalFont %s\n"
	      "bGDICharset %d\n\n",
	      i,
	      cpinfo[i].dwFlags, dump_mime_flags(cpinfo[i].dwFlags),
	      cpinfo[i].uiCodePage,
	      cpinfo[i].uiFamilyCodePage,
	      wine_dbgstr_w(cpinfo[i].wszDescription),
	      wine_dbgstr_w(cpinfo[i].wszWebCharset),
	      wine_dbgstr_w(cpinfo[i].wszHeaderCharset),
	      wine_dbgstr_w(cpinfo[i].wszBodyCharset),
	      wine_dbgstr_w(cpinfo[i].wszFixedWidthFont),
	      wine_dbgstr_w(cpinfo[i].wszProportionalFont),
	      cpinfo[i].bGDICharset);
#endif
	ok(cpinfo[i].dwFlags & flags, "enumerated flags %08x do not include requested %08x\n", cpinfo[i].dwFlags, flags);

	if (TranslateCharsetInfo((DWORD *)(INT_PTR)cpinfo[i].uiFamilyCodePage, &csi, TCI_SRCCODEPAGE))
	    ok(cpinfo[i].bGDICharset == csi.ciCharset, "%d != %d\n", cpinfo[i].bGDICharset, csi.ciCharset);
	else
            if (winetest_debug > 1)
                trace("TranslateCharsetInfo failed for cp %u\n", cpinfo[i].uiFamilyCodePage);

#ifdef DUMP_CP_INFO
        trace("%u: codepage %u family %u\n", i, cpinfo[i].uiCodePage, cpinfo[i].uiFamilyCodePage);
#endif

	/* support files for some codepages might be not installed, or
	 * the codepage is just an alias.
	 */
	if (IsValidCodePage(cpinfo[i].uiCodePage))
	{
	    ret = IMultiLanguage2_IsConvertible(iML2, cpinfo[i].uiCodePage, CP_UNICODE);
	    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(%u -> CP_UNICODE) = %08x\n", cpinfo[i].uiCodePage, ret);
	    ret = IMultiLanguage2_IsConvertible(iML2, CP_UNICODE, cpinfo[i].uiCodePage);
	    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(CP_UNICODE -> %u) = %08x\n", cpinfo[i].uiCodePage, ret);

            convertible = check_convertible(iML2, cpinfo[i].uiCodePage, CP_UTF8);
            if (convertible != E_FAIL)
            {
                ret = IMultiLanguage2_IsConvertible(iML2, cpinfo[i].uiCodePage, CP_UTF8);
                ok(ret == convertible, "IMultiLanguage2_IsConvertible(%u -> CP_UTF8) = %08x\n", cpinfo[i].uiCodePage, ret);
                ret = IMultiLanguage2_IsConvertible(iML2, CP_UTF8, cpinfo[i].uiCodePage);
                ok(ret == convertible, "IMultiLanguage2_IsConvertible(CP_UTF8 -> %u) = %08x\n", cpinfo[i].uiCodePage, ret);
            }
	}
	else
            if (winetest_debug > 1)
                trace("IsValidCodePage failed for cp %u\n", cpinfo[i].uiCodePage);

    if (memcmp(cpinfo[i].wszWebCharset,feffW,sizeof(WCHAR)*11)==0)
        skip("Legacy windows bug returning invalid charset of unicodeFEFF\n");
    else
    {
        ret = IMultiLanguage2_GetCharsetInfo(iML2, cpinfo[i].wszWebCharset, &mcsi);
        /* _autoxxx charsets are a fake and GetCharsetInfo fails for them */
        if (memcmp(cpinfo[i].wszWebCharset, autoW, 5 * sizeof(WCHAR)))
        {
            ok (ret == S_OK, "IMultiLanguage2_GetCharsetInfo failed: %08x\n", ret);
#ifdef DUMP_CP_INFO
            trace("%s: %u %u %s\n", wine_dbgstr_w(cpinfo[i].wszWebCharset), mcsi.uiCodePage, mcsi.uiInternetEncoding, wine_dbgstr_w(mcsi.wszCharset));
#endif
            ok(!lstrcmpiW(cpinfo[i].wszWebCharset, mcsi.wszCharset), "%s != %s\n",
               wine_dbgstr_w(cpinfo[i].wszWebCharset), wine_dbgstr_w(mcsi.wszCharset));

        if (0)
        {
            /* native mlang returns completely messed up encodings in some cases */
            ok(mcsi.uiInternetEncoding == cpinfo[i].uiCodePage || mcsi.uiInternetEncoding == cpinfo[i].uiFamilyCodePage,
            "%u != %u || %u\n", mcsi.uiInternetEncoding, cpinfo[i].uiCodePage, cpinfo[i].uiFamilyCodePage);
            ok(mcsi.uiCodePage == cpinfo[i].uiCodePage || mcsi.uiCodePage == cpinfo[i].uiFamilyCodePage,
            "%u != %u || %u\n", mcsi.uiCodePage, cpinfo[i].uiCodePage, cpinfo[i].uiFamilyCodePage);
            }
        }
    }

    if (memcmp(cpinfo[i].wszHeaderCharset,feffW,sizeof(WCHAR)*11)==0)
        skip("Legacy windows bug returning invalid charset of unicodeFEFF\n");
    else
    {
        ret = IMultiLanguage2_GetCharsetInfo(iML2, cpinfo[i].wszHeaderCharset, &mcsi);
        /* _autoxxx charsets are a fake and GetCharsetInfo fails for them */
        if (memcmp(cpinfo[i].wszHeaderCharset, autoW, 5 * sizeof(WCHAR)))
        {
            ok (ret == S_OK, "IMultiLanguage2_GetCharsetInfo failed: %08x\n", ret);
#ifdef DUMP_CP_INFO
            trace("%s: %u %u %s\n", wine_dbgstr_w(cpinfo[i].wszHeaderCharset), mcsi.uiCodePage, mcsi.uiInternetEncoding, wine_dbgstr_w(mcsi.wszCharset));
#endif
            ok(!lstrcmpiW(cpinfo[i].wszHeaderCharset, mcsi.wszCharset), "%s != %s\n",
               wine_dbgstr_w(cpinfo[i].wszHeaderCharset), wine_dbgstr_w(mcsi.wszCharset));

        if (0)
        {
            /* native mlang returns completely messed up encodings in some cases */
            ok(mcsi.uiInternetEncoding == cpinfo[i].uiCodePage || mcsi.uiInternetEncoding == cpinfo[i].uiFamilyCodePage,
            "%u != %u || %u\n", mcsi.uiInternetEncoding, cpinfo[i].uiCodePage, cpinfo[i].uiFamilyCodePage);
            ok(mcsi.uiCodePage == cpinfo[i].uiCodePage || mcsi.uiCodePage == cpinfo[i].uiFamilyCodePage,
            "%u != %u || %u\n", mcsi.uiCodePage, cpinfo[i].uiCodePage, cpinfo[i].uiFamilyCodePage);
        }
        }
    }

    if (memcmp(cpinfo[i].wszBodyCharset,feffW,sizeof(WCHAR)*11)==0)
        skip("Legacy windows bug returning invalid charset of unicodeFEFF\n");
    else
    {
        ret = IMultiLanguage2_GetCharsetInfo(iML2, cpinfo[i].wszBodyCharset, &mcsi);
        /* _autoxxx charsets are a fake and GetCharsetInfo fails for them */
        if (memcmp(cpinfo[i].wszBodyCharset, autoW, 5 * sizeof(WCHAR)))
        {
            ok (ret == S_OK, "IMultiLanguage2_GetCharsetInfo failed: %08x\n", ret);
#ifdef DUMP_CP_INFO
            trace("%s: %u %u %s\n", wine_dbgstr_w(cpinfo[i].wszBodyCharset), mcsi.uiCodePage, mcsi.uiInternetEncoding, wine_dbgstr_w(mcsi.wszCharset));
#endif
            ok(!lstrcmpiW(cpinfo[i].wszBodyCharset, mcsi.wszCharset), "%s != %s\n",
               wine_dbgstr_w(cpinfo[i].wszBodyCharset), wine_dbgstr_w(mcsi.wszCharset));

        if (0)
        {
            /* native mlang returns completely messed up encodings in some cases */
            ok(mcsi.uiInternetEncoding == cpinfo[i].uiCodePage || mcsi.uiInternetEncoding == cpinfo[i].uiFamilyCodePage,
            "%u != %u || %u\n", mcsi.uiInternetEncoding, cpinfo[i].uiCodePage, cpinfo[i].uiFamilyCodePage);
            ok(mcsi.uiCodePage == cpinfo[i].uiCodePage || mcsi.uiCodePage == cpinfo[i].uiFamilyCodePage,
            "%u != %u || %u\n", mcsi.uiCodePage, cpinfo[i].uiCodePage, cpinfo[i].uiFamilyCodePage);
        }
        }
    }
    }

    /* now IEnumCodePage_Next should fail, since pointer is at the end */
    n = 1;
    ret = IEnumCodePage_Next(iEnumCP, 1, &cpinfo2, &n);
    ok(ret == S_FALSE && n == 0, "IEnumCodePage_Next: expected S_FALSE/0, got %08x/%u\n", ret, n);

    ret = IEnumCodePage_Reset(iEnumCP);
    ok(ret == S_OK, "IEnumCodePage_Reset: expected S_OK, got %08x\n", ret);
    n = 0;
    ret = IEnumCodePage_Next(iEnumCP, 1, &cpinfo2, &n);
    ok(n == 1 && ret == S_OK, "IEnumCodePage_Next: expected 1/S_OK, got %u/%08x\n", n, ret);
    cpinfo_cmp(cpinfo, &cpinfo2);

    if (0)
    {
    /* Due to a bug in MS' implementation of IEnumCodePage_Skip
     * it's not used here.
     */
    ret = IEnumCodePage_Skip(iEnumCP, 1);
    ok(ret == S_OK, "IEnumCodePage_Skip: expected S_OK, got %08x\n", ret);
    }
    for (i = 0; i < total - 1; i++)
    {
        n = 0;
        ret = IEnumCodePage_Next(iEnumCP, 1, &cpinfo2, &n);
        ok(n == 1 && ret == S_OK, "IEnumCodePage_Next: expected 1/S_OK, got %u/%08x\n", n, ret);
        cpinfo_cmp(&cpinfo[i + 1], &cpinfo2);
    }

    HeapFree(GetProcessHeap(), 0, cpinfo);
    IEnumCodePage_Release(iEnumCP);
}

static void test_GetCharsetInfo_alias(IMultiLanguage *ml)
{
    WCHAR asciiW[] = {'a','s','c','i','i',0};
    MIMECSETINFO info;
    HRESULT hr;

    hr = IMultiLanguage_GetCharsetInfo(ml, asciiW, &info);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(!lstrcmpW(info.wszCharset, asciiW), "got %s\n", wine_dbgstr_w(info.wszCharset));
}

static void scriptinfo_cmp(SCRIPTINFO *sinfo1, SCRIPTINFO *sinfo2)
{
    ok(sinfo1->ScriptId == sinfo2->ScriptId, "ScriptId mismatch: %d != %d\n", sinfo1->ScriptId, sinfo2->ScriptId);
    ok(sinfo1->uiCodePage == sinfo2->uiCodePage, "uiCodePage mismatch: %u != %u\n", sinfo1->uiCodePage, sinfo2->uiCodePage);
    ok(!lstrcmpW(sinfo1->wszDescription, sinfo2->wszDescription), "wszDescription mismatch\n");
    ok(!lstrcmpW(sinfo1->wszFixedWidthFont, sinfo2->wszFixedWidthFont), "wszFixedWidthFont mismatch\n");
    ok(!lstrcmpW(sinfo1->wszProportionalFont, sinfo2->wszProportionalFont), "wszProportionalFont mismatch\n");
}

static void test_EnumScripts(IMultiLanguage2 *iML2, DWORD flags)
{
    IEnumScript *iEnumScript = NULL;
    SCRIPTINFO *sinfo;
    SCRIPTINFO sinfo2;
    HRESULT ret;
    ULONG i, n;
    UINT total;

    total = 0;
    ret = IMultiLanguage2_GetNumberOfScripts(iML2, &total);
    ok(ret == S_OK && total != 0, "IMultiLanguage2_GetNumberOfScripts: expected S_OK/!0, got %08x/%u\n", ret, total);

    trace("total mlang supported scripts %u\n", total);

    ret = IMultiLanguage2_EnumScripts(iML2, flags, LANG_NEUTRAL, &iEnumScript);
    ok(ret == S_OK && iEnumScript, "IMultiLanguage2_EnumScripts: expected S_OK/!NULL, got %08x/%p\n", ret, iEnumScript);

    ret = IEnumScript_Reset(iEnumScript);
    ok(ret == S_OK, "IEnumScript_Reset: expected S_OK, got %08x\n", ret);
    n = 65536;
    ret = IEnumScript_Next(iEnumScript, 0, NULL, &n);
    ok(n == 65536 && ret == E_FAIL, "IEnumScript_Next: expected 65536/E_FAIL, got %u/%08x\n", n, ret);
    ret = IEnumScript_Next(iEnumScript, 0, NULL, NULL);
    ok(ret == E_FAIL, "IEnumScript_Next: expected E_FAIL, got %08x\n", ret);

    sinfo = HeapAlloc(GetProcessHeap(), 0, sizeof(*sinfo) * total * 2);

    n = total * 2;
    ret = IEnumScript_Next(iEnumScript, 0, sinfo, &n);
    ok(ret == S_FALSE && n == 0, "IEnumScript_Next: expected S_FALSE/0, got %08x/%u\n", ret, n);

    n = total * 2;
    ret = IEnumScript_Next(iEnumScript, n, sinfo, &n);
    ok(ret == S_OK && n != 0, "IEnumScript_Next: expected S_OK, got %08x/%u\n", ret, n);

    trace("flags %08x, enumerated scripts %u\n", flags, n);

    if (!flags)
    {
	ok(n == total, "IEnumScript_Next: expected %u, got %u\n", total, n);
    }

    total = n;

    for (i = 0; pGetCPInfoExA && i < n; i++)
    {
#ifdef DUMP_SCRIPT_INFO
	trace("SCRIPTINFO #%u:\n"
	      "ScriptId %08x\n"
	      "uiCodePage %u\n"
	      "wszDescription %s\n"
	      "wszFixedWidthFont %s\n"
	      "wszProportionalFont %s\n\n",
	      i,
	      sinfo[i].ScriptId,
	      sinfo[i].uiCodePage,
	      wine_dbgstr_w(sinfo[i].wszDescription),
	      wine_dbgstr_w(sinfo[i].wszFixedWidthFont),
	      wine_dbgstr_w(sinfo[i].wszProportionalFont));
        trace("%u codepage %u\n", i, sinfo[i].uiCodePage);
#endif
    }

    /* now IEnumScript_Next should fail, since pointer is at the end */
    n = 1;
    ret = IEnumScript_Next(iEnumScript, 1, &sinfo2, &n);
    ok(ret == S_FALSE && n == 0, "IEnumScript_Next: expected S_FALSE/0, got %08x/%u\n", ret, n);

    ret = IEnumScript_Reset(iEnumScript);
    ok(ret == S_OK, "IEnumScript_Reset: expected S_OK, got %08x\n", ret);
    n = 0;
    ret = IEnumScript_Next(iEnumScript, 1, &sinfo2, &n);
    ok(n == 1 && ret == S_OK, "IEnumScript_Next: expected 1/S_OK, got %u/%08x\n", n, ret);
    scriptinfo_cmp(sinfo, &sinfo2);

    if (0)
    {
    /* Due to a bug in MS' implementation of IEnumScript_Skip
     * it's not used here.
     */
    ret = IEnumScript_Skip(iEnumScript, 1);
    ok(ret == S_OK, "IEnumScript_Skip: expected S_OK, got %08x\n", ret);
    }
    for (i = 0; i < total - 1; i++)
    {
        n = 0;
        ret = IEnumScript_Next(iEnumScript, 1, &sinfo2, &n);
        ok(n == 1 && ret == S_OK, "IEnumScript_Next: expected 1/S_OK, got %u/%08x\n", n, ret);
        scriptinfo_cmp(&sinfo[i + 1], &sinfo2);
    }

    HeapFree(GetProcessHeap(), 0, sinfo);
    IEnumScript_Release(iEnumScript);
}

static void IMLangFontLink_Test(IMLangFontLink* iMLFL)
{
    DWORD dwCodePages, dwManyCodePages;
    DWORD dwCmpCodePages;
    UINT CodePage;
    static const WCHAR str[] = { 'd', 0x0436, 0xff90 };
    LONG processed;
    HRESULT ret;

    dwCodePages = ~0u;
    ret = IMLangFontLink_CodePageToCodePages(iMLFL, -1, &dwCodePages);
    ok(ret == E_FAIL, "IMLangFontLink_CodePageToCodePages should fail: %x\n", ret);
    ok(dwCodePages == 0, "expected 0, got %u\n", dwCodePages);

    dwCodePages = 0;
    ret = IMLangFontLink_CodePageToCodePages(iMLFL, 932, &dwCodePages);
    ok(ret == S_OK, "IMLangFontLink_CodePageToCodePages error %x\n", ret);
    ok(dwCodePages == FS_JISJAPAN, "expected FS_JISJAPAN, got %08x\n", dwCodePages);
    CodePage = 0;
    ret = IMLangFontLink_CodePagesToCodePage(iMLFL, dwCodePages, 1035, &CodePage);
    ok(ret == S_OK, "IMLangFontLink_CodePagesToCodePage error %x\n", ret);
    ok(CodePage == 932, "Incorrect CodePage Returned (%i)\n",CodePage);

    dwManyCodePages = 0;
    ret = IMLangFontLink_CodePageToCodePages(iMLFL, 1252, &dwManyCodePages);
    ok(ret == S_OK, "IMLangFontLink_CodePageToCodePages error %x\n", ret);
    ok(dwManyCodePages == FS_LATIN1, "expected FS_LATIN1, got %08x\n", dwManyCodePages);
    dwCodePages = 0;
    ret = IMLangFontLink_CodePageToCodePages(iMLFL, 1256, &dwCodePages);
    ok(ret == S_OK, "IMLangFontLink_CodePageToCodePages error %x\n", ret);
    ok(dwCodePages == FS_ARABIC, "expected FS_ARABIC, got %08x\n", dwCodePages);
    dwManyCodePages |= dwCodePages;
    ret = IMLangFontLink_CodePageToCodePages(iMLFL, 874, &dwCodePages);
    ok(ret == S_OK, "IMLangFontLink_CodePageToCodePages error %x\n", ret);
    ok(dwCodePages == FS_THAI, "expected FS_THAI, got %08x\n", dwCodePages);
    dwManyCodePages |= dwCodePages;

    ret = IMLangFontLink_CodePagesToCodePage(iMLFL, dwManyCodePages, 1256, &CodePage);
    ok(ret == S_OK, "IMLangFontLink_CodePagesToCodePage error %x\n", ret);
    ok(CodePage == 1256, "Incorrect CodePage Returned (%i)\n",CodePage);

    ret = IMLangFontLink_CodePagesToCodePage(iMLFL, dwManyCodePages, 936, &CodePage);
    ok(ret == S_OK, "IMLangFontLink_CodePagesToCodePage error %x\n", ret);
    ok(CodePage == 1252, "Incorrect CodePage Returned (%i)\n",CodePage);

    /* Tests for GetCharCodePages */

    /* Latin 1 */
    dwCmpCodePages = FS_LATIN1 | FS_LATIN2 | FS_CYRILLIC | FS_GREEK | FS_TURKISH
        | FS_HEBREW | FS_ARABIC | FS_BALTIC | FS_VIETNAMESE | FS_THAI
        | FS_JISJAPAN | FS_CHINESESIMP | FS_WANSUNG | FS_CHINESETRAD;
    ret = IMLangFontLink_GetCharCodePages(iMLFL, 'd', &dwCodePages);
    ok(ret == S_OK, "IMLangFontLink_GetCharCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);

    dwCodePages = 0;
    processed = 0;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, str, 1, 0, &dwCodePages, &processed);
    ok(ret == S_OK, "IMLangFontLink_GetStrCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 1, "expected 1, got %d\n", processed);

    /* Cyrillic */
    dwCmpCodePages = FS_CYRILLIC | FS_JISJAPAN | FS_CHINESESIMP | FS_WANSUNG;
    ret = IMLangFontLink_GetCharCodePages(iMLFL, 0x0436, &dwCodePages);
    ok(ret == S_OK, "IMLangFontLink_GetCharCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);

    dwCodePages = 0;
    processed = 0;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, &str[1], 1, 0, &dwCodePages, &processed);
    ok(ret == S_OK, "IMLangFontLink_GetStrCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 1, "expected 1, got %d\n", processed);

    /* Japanese */
    dwCmpCodePages =  FS_JISJAPAN;
    ret = IMLangFontLink_GetCharCodePages(iMLFL, 0xff90, &dwCodePages);
    ok(ret == S_OK, "IMLangFontLink_GetCharCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);

    dwCodePages = 0;
    processed = 0;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, &str[2], 1, 0, &dwCodePages, &processed);
    ok(ret == S_OK, "IMLangFontLink_GetStrCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 1, "expected 1, got %d\n", processed);

    dwCmpCodePages = FS_CYRILLIC | FS_JISJAPAN | FS_CHINESESIMP | FS_WANSUNG;
    dwCodePages = 0;
    processed = 0;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, str, 2, 0, &dwCodePages, &processed);
    ok(ret == S_OK, "IMLangFontLink_GetStrCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 2, "expected 2, got %d\n", processed);

    dwCmpCodePages = FS_JISJAPAN;
    dwCodePages = 0;
    processed = 0;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, str, 3, 0, &dwCodePages, &processed);
    ok(ret == S_OK, "IMLangFontLink_GetStrCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 3, "expected 3, got %d\n", processed);

    dwCodePages = 0xffff;
    processed = -1;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, &str[2], 1, 0, &dwCodePages, &processed);
    ok(ret == S_OK, "IMLangFontLink_GetStrCodePages error %x\n", ret);
    ok(dwCodePages == dwCmpCodePages, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 1, "expected 0, got %d\n", processed);

    ret = IMLangFontLink_GetStrCodePages(iMLFL, &str[2], 1, 0, NULL, NULL);
    ok(ret == S_OK, "IMLangFontLink_GetStrCodePages error %x\n", ret);

    dwCodePages = 0xffff;
    processed = -1;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, str, -1, 0, &dwCodePages, &processed);
    ok(ret == E_INVALIDARG, "IMLangFontLink_GetStrCodePages should fail: %x\n", ret);
    ok(dwCodePages == 0, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 0, "expected 0, got %d\n", processed);

    dwCodePages = 0xffff;
    processed = -1;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, NULL, 1, 0, &dwCodePages, &processed);
    ok(ret == E_INVALIDARG, "IMLangFontLink_GetStrCodePages should fail: %x\n", ret);
    ok(dwCodePages == 0, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 0, "expected 0, got %d\n", processed);

    dwCodePages = 0xffff;
    processed = -1;
    ret = IMLangFontLink_GetStrCodePages(iMLFL, str, 0, 0, &dwCodePages, &processed);
    ok(ret == E_INVALIDARG, "IMLangFontLink_GetStrCodePages should fail: %x\n", ret);
    ok(dwCodePages == 0, "expected %x, got %x\n", dwCmpCodePages, dwCodePages);
    ok(processed == 0, "expected 0, got %d\n", processed);
}

/* copied from libs/wine/string.c */
static WCHAR *strstrW(const WCHAR *str, const WCHAR *sub)
{
    while (*str)
    {
        const WCHAR *p1 = str, *p2 = sub;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) return (WCHAR *)str;
        str++;
    }
    return NULL;
}

static void test_rfc1766(IMultiLanguage2 *iML2)
{
    IEnumRfc1766 *pEnumRfc1766;
    RFC1766INFO info;
    ULONG n;
    HRESULT ret;
    BSTR rfcstr;

    ret = IMultiLanguage2_EnumRfc1766(iML2, LANG_NEUTRAL, &pEnumRfc1766);
    ok(ret == S_OK, "IMultiLanguage2_EnumRfc1766 error %08x\n", ret);

    while (1)
    {
        ret = IEnumRfc1766_Next(pEnumRfc1766, 1, &info, &n);
        if (ret != S_OK) break;

#ifdef DUMP_CP_INFO
        trace("lcid %04x rfc_name %s locale_name %s\n",
              info.lcid, wine_dbgstr_w(info.wszRfc1766), wine_dbgstr_w(info.wszLocaleName));
#endif

        ok(n == 1, "couldn't fetch 1 RFC1766INFO structure\n");

        /* verify the Rfc1766 value */
        ret = IMultiLanguage2_GetRfc1766FromLcid(iML2, info.lcid, &rfcstr);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);

        /* not an exact 1:1 correspondence between lcid and rfc1766 in the
         * mlang database, e.g., nb-no -> 1044 -> no */
        ok(strstrW(info.wszRfc1766, rfcstr) != NULL,
           "Expected matching locale names\n");

        SysFreeString(rfcstr);
    }
    IEnumRfc1766_Release(pEnumRfc1766);
}

static void test_GetLcidFromRfc1766(IMultiLanguage2 *iML2)
{
    WCHAR rfc1766W[MAX_RFC1766_NAME + 1];
    LCID lcid;
    HRESULT ret;
    DWORD i;

    static WCHAR en[] = { 'e','n',0 };
    static WCHAR en_them[] = { 'e','n','-','t','h','e','m',0 };
    static WCHAR english[] = { 'e','n','g','l','i','s','h',0 };


    for(i = 0; i < sizeof(lcid_table) / sizeof(lcid_table[0]); i++) {
        lcid = -1;
        MultiByteToWideChar(CP_ACP, 0, lcid_table[i].rfc1766, -1, rfc1766W, MAX_RFC1766_NAME);
        ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, rfc1766W);

        /* IE <6.0 guess 0x412 (ko) from "kok" */
        ok((ret == lcid_table[i].hr) ||
            broken(lcid_table[i].broken_lcid && (ret == S_FALSE)),
            "#%02d: HRESULT 0x%x (expected 0x%x)\n", i, ret, lcid_table[i].hr);

        ok((lcid == lcid_table[i].lcid) ||
            broken(lcid == lcid_table[i].broken_lcid),   /* IE <6.0 */
            "#%02d: got LCID 0x%x (expected 0x%x)\n", i, lcid, lcid_table[i].lcid);
    }


    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, NULL, en);
    ok(ret == E_INVALIDARG, "GetLcidFromRfc1766 returned: %08x\n", ret);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, NULL);
    ok(ret == E_INVALIDARG, "GetLcidFromRfc1766 returned: %08x\n", ret);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, en_them);
    ok((ret == E_FAIL || ret == S_FALSE), "GetLcidFromRfc1766 returned: %08x\n", ret);
    if (ret == S_FALSE)
    {
        BSTR rfcstr;
        static WCHAR en[] = {'e','n',0};

        ret = IMultiLanguage2_GetRfc1766FromLcid(iML2, lcid, &rfcstr);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        ok_w2("Expected \"%s\",  got \"%s\"n", en, rfcstr);
    }

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, english);
    ok((ret == E_FAIL || ret == S_FALSE), "GetLcidFromRfc1766 returned: %08x\n", ret);
    if (ret == S_FALSE)
    {
        BSTR rfcstr;
        static WCHAR en[] = {'e','n',0};

        ret = IMultiLanguage2_GetRfc1766FromLcid(iML2, lcid, &rfcstr);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        ok_w2("Expected \"%s\",  got \"%s\"n", en, rfcstr);
    }

}

static void test_Rfc1766ToLcid(void)
{
    LCID lcid;
    HRESULT ret;
    DWORD i;

    for(i = 0; i < sizeof(lcid_table) / sizeof(lcid_table[0]); i++) {
        lcid = -1;
        ret = pRfc1766ToLcidA(&lcid, lcid_table[i].rfc1766);

        /* IE <6.0 guess 0x412 (ko) from "kok" */
        ok( (ret == lcid_table[i].hr) ||
            broken(lcid_table[i].broken_lcid && (ret == S_FALSE)),
            "#%02d: HRESULT 0x%x (expected 0x%x)\n", i, ret, lcid_table[i].hr);

        ok( (lcid == lcid_table[i].lcid) ||
            broken(lcid == lcid_table[i].broken_lcid),  /* IE <6.0 */
            "#%02d: got LCID 0x%x (expected 0x%x)\n", i, lcid, lcid_table[i].lcid);
    }

    ret = pRfc1766ToLcidA(&lcid, NULL);
    ok(ret == E_INVALIDARG, "got 0x%08x (expected E_INVALIDARG)\n", ret);

    ret = pRfc1766ToLcidA(NULL, "en");
    ok(ret == E_INVALIDARG, "got 0x%08x (expected E_INVALIDARG)\n", ret);
}

static void test_GetNumberOfCodePageInfo(IMultiLanguage2 *iML2)
{
    HRESULT hr;
    UINT value;

    value = 0xdeadbeef;
    hr = IMultiLanguage2_GetNumberOfCodePageInfo(iML2, &value);
    ok( (hr == S_OK) && value,
        "got 0x%x with %d (expected S_OK with '!= 0')\n", hr, value);

    hr = IMultiLanguage2_GetNumberOfCodePageInfo(iML2, NULL);
    ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

}

static void test_GetRfc1766FromLcid(IMultiLanguage2 *iML2)
{
    CHAR expected[MAX_RFC1766_NAME];
    CHAR buffer[MAX_RFC1766_NAME + 1];
    DWORD i;
    HRESULT hr;
    BSTR rfcstr;

    for(i = 0; i < sizeof(lcid_table) / sizeof(lcid_table[0]); i++) {
        buffer[0] = '\0';

        rfcstr = NULL;
        hr = IMultiLanguage2_GetRfc1766FromLcid(iML2, lcid_table[i].lcid, &rfcstr);
        ok(hr == lcid_table[i].hr,
            "#%02d: HRESULT 0x%x (expected 0x%x)\n", i, hr, lcid_table[i].hr);

        if (hr != S_OK)
            continue;   /* no result-string created */

        WideCharToMultiByte(CP_ACP, 0, rfcstr, -1, buffer, sizeof(buffer), NULL, NULL);
        LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, lcid_table[i].rfc1766,
                    lstrlenA(lcid_table[i].rfc1766) + 1, expected, MAX_RFC1766_NAME);

        /* IE <6.0 return "x-kok" for LCID 0x457 ("kok") */
        ok( (!lstrcmpA(buffer, expected)) ||
            broken(!lstrcmpA(buffer, lcid_table[i].broken_rfc)),
            "#%02d: got '%s' (expected '%s')\n", i, buffer, expected);

        SysFreeString(rfcstr);
    }

    hr = IMultiLanguage2_GetRfc1766FromLcid(iML2, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), NULL);
    ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);
}

static void test_LcidToRfc1766(void)
{
    CHAR expected[MAX_RFC1766_NAME];
    CHAR buffer[MAX_RFC1766_NAME * 2];
    HRESULT hr;
    DWORD i;

    for(i = 0; i < sizeof(lcid_table) / sizeof(lcid_table[0]); i++) {

        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';

        hr = pLcidToRfc1766A(lcid_table[i].lcid, buffer, MAX_RFC1766_NAME);

        /* IE <5.0 does not recognize 0x180c (fr-mc) and 0x457 (kok) */
        ok( (hr == lcid_table[i].hr) ||
            broken(lcid_table[i].broken_lcid && (hr == E_FAIL)),
            "#%02d: HRESULT 0x%x (expected 0x%x)\n", i, hr, lcid_table[i].hr);

        if (hr != S_OK)
            continue;

        LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, lcid_table[i].rfc1766,
                    lstrlenA(lcid_table[i].rfc1766) + 1, expected, MAX_RFC1766_NAME);

        /* IE <6.0 return "x-kok" for LCID 0x457 ("kok") */
        /* IE <5.0 return "fr" for LCID 0x180c ("fr-mc") */
        ok( (!lstrcmpA(buffer, expected)) ||
            broken(!lstrcmpA(buffer, lcid_table[i].broken_rfc)),
            "#%02d: got '%s' (expected '%s')\n", i, buffer, expected);

    }

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pLcidToRfc1766A(-1, buffer, MAX_RFC1766_NAME);
    ok(hr == E_FAIL, "got 0x%08x and '%s' (expected E_FAIL)\n", hr, buffer);

    hr = pLcidToRfc1766A(LANG_ENGLISH, NULL, MAX_RFC1766_NAME);
    ok(hr == E_INVALIDARG, "got 0x%08x (expected E_INVALIDARG)\n", hr);

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pLcidToRfc1766A(LANG_ENGLISH, buffer, -1);
    ok(hr == E_INVALIDARG, "got 0x%08x and '%s' (expected E_INVALIDARG)\n", hr, buffer);

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pLcidToRfc1766A(LANG_ENGLISH, buffer, 0);
    ok(hr == E_INVALIDARG, "got 0x%08x and '%s' (expected E_INVALIDARG)\n", hr, buffer);
}

static void test_GetRfc1766Info(IMultiLanguage2 *iML2)
{
    WCHAR short_broken_name[MAX_LOCALE_NAME];
    CHAR rfc1766A[MAX_RFC1766_NAME + 1];
    BYTE buffer[sizeof(RFC1766INFO) + 4];
    PRFC1766INFO prfc = (RFC1766INFO *) buffer;
    HRESULT ret;
    DWORD i;

    for(i = 0; i < sizeof(info_table) / sizeof(info_table[0]); i++) {
        memset(buffer, 'x', sizeof(RFC1766INFO) + 2);
        buffer[sizeof(buffer) -1] = 0;
        buffer[sizeof(buffer) -2] = 0;

        ret = IMultiLanguage2_GetRfc1766Info(iML2, info_table[i].lcid, info_table[i].lang, prfc);
        WideCharToMultiByte(CP_ACP, 0, prfc->wszRfc1766, -1, rfc1766A, MAX_RFC1766_NAME, NULL, NULL);
        ok(ret == S_OK, "#%02d: got 0x%x (expected S_OK)\n", i, ret);
        ok(prfc->lcid == info_table[i].lcid,
            "#%02d: got 0x%04x (expected 0x%04x)\n", i, prfc->lcid, info_table[i].lcid);

        ok(!lstrcmpA(rfc1766A, info_table[i].rfc1766),
            "#%02d: got '%s' (expected '%s')\n", i, rfc1766A, info_table[i].rfc1766);

        /* Some IE versions truncate an oversized name one character too short */
        if (info_table[i].broken_name) {
            lstrcpyW(short_broken_name, info_table[i].broken_name);
            short_broken_name[MAX_LOCALE_NAME - 2] = 0;
        }

        if (info_table[i].todo & TODO_NAME) {
            todo_wine
            ok( (!lstrcmpW(prfc->wszLocaleName, info_table[i].localename)) ||
               (info_table[i].broken_name && (
                broken(!lstrcmpW(prfc->wszLocaleName, info_table[i].broken_name)) || /* IE < 6.0 */
                broken(!lstrcmpW(prfc->wszLocaleName, short_broken_name)))),
                "#%02d: got %s (expected %s)\n", i,
                wine_dbgstr_w(prfc->wszLocaleName), wine_dbgstr_w(info_table[i].localename));
        }
        else
            ok( (!lstrcmpW(prfc->wszLocaleName, info_table[i].localename)) ||
               (info_table[i].broken_name && (
                broken(!lstrcmpW(prfc->wszLocaleName, info_table[i].broken_name)) || /* IE < 6.0 */
                broken(!lstrcmpW(prfc->wszLocaleName, short_broken_name)))),
                "#%02d: got %s (expected %s)\n", i,
                wine_dbgstr_w(prfc->wszLocaleName), wine_dbgstr_w(info_table[i].localename));

    }

    /* SUBLANG_NEUTRAL only allowed for English, Arabic, Chinese */
    ret = IMultiLanguage2_GetRfc1766Info(iML2, MAKELANGID(LANG_GERMAN, SUBLANG_NEUTRAL), LANG_ENGLISH, prfc);
    ok(ret == E_FAIL, "got 0x%x (expected E_FAIL)\n", ret);

    ret = IMultiLanguage2_GetRfc1766Info(iML2, MAKELANGID(LANG_ITALIAN, SUBLANG_NEUTRAL), LANG_ENGLISH, prfc);
    ok(ret == E_FAIL, "got 0x%x (expected E_FAIL)\n", ret);

    /* NULL not allowed */
    ret = IMultiLanguage2_GetRfc1766Info(iML2, 0, LANG_ENGLISH, prfc);
    ok(ret == E_FAIL, "got 0x%x (expected E_FAIL)\n", ret);

    ret = IMultiLanguage2_GetRfc1766Info(iML2, LANG_ENGLISH, LANG_ENGLISH, NULL);
    ok(ret == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", ret);
}

static void test_IMultiLanguage2_ConvertStringFromUnicode(IMultiLanguage2 *iML2)
{
    CHAR dest[MAX_PATH];
    CHAR invariate[MAX_PATH];
    UINT srcsz, destsz;
    HRESULT hr;

    static WCHAR src[] = {'a','b','c',0};

    memset(invariate, 'x', sizeof(invariate));

    /* pSrcStr NULL */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = sizeof(dest);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, NULL,
                                                  &srcsz, dest, &destsz);
    ok(hr == S_OK || hr == E_FAIL,"expected S_OK or E_FAIL, got %08x\n",hr);
    if (hr == S_OK)
    {
        ok(srcsz == lstrlenW(src) + 1,
           "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    }
    else if (hr == E_FAIL)
    {
        ok(srcsz == 0,
           "Expected %u, got %u\n", 0, srcsz);
    }

    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);
    ok(destsz == 0, "Expected 0, got %u\n", destsz);

    /* pcSrcSize NULL */
    memset(dest, 'x', sizeof(dest));
    destsz = sizeof(dest);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  NULL, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!strncmp(dest, "abc", 3),
       "Expected first three chars to be \"abc\"\n");
    ok(!memcmp(&dest[3], invariate, sizeof(dest) - 3),
       "Expected rest of dest to be unchanged, got %s\n", dest);
    ok(destsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), destsz);

    /* both pSrcStr and pcSrcSize NULL */
    memset(dest, 'x', sizeof(dest));
    destsz = sizeof(dest);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, NULL,
                                                  NULL, dest, &destsz);
    ok(hr == S_OK || hr == E_FAIL, "Expected S_OK or E_FAIL, got %08x\n", hr);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);
    ok(destsz == 0, "Expected 0, got %u\n", destsz);

    /* pDstStr NULL */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = sizeof(dest);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, NULL, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    ok(destsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);

    /* pcDstSize NULL */
    memset(dest, 'x', sizeof(dest));
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, dest, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);

    /* pcSrcSize is 0 */
    memset(dest, 'x', sizeof(dest));
    srcsz = 0;
    destsz = sizeof(dest);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == 0, "Expected 0, got %u\n", srcsz);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);
    ok(destsz == 0, "Expected 0, got %u\n", destsz);

    /* pcSrcSize does not include NULL terminator */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src);
    destsz = sizeof(dest);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), srcsz);
    ok(!strncmp(dest, "abc", 3), "Expected first three chars to be \"abc\"\n");
    ok(!memcmp(&dest[3], invariate, sizeof(dest) - 3),
       "Expected rest of dest to be unchanged, got %s\n", dest);
    ok(destsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), destsz);

    /* pcSrcSize includes NULL terminator */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = sizeof(dest);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1, "Expected 3, got %u\n", srcsz);
    ok(!lstrcmpA(dest, "abc"), "Expected \"abc\", got \"%s\"\n", dest);
    ok(destsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, destsz);

    /* pcSrcSize is -1 */
    memset(dest, 'x', sizeof(dest));
    srcsz = -1;
    destsz = sizeof(dest);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), srcsz);
    ok(!strncmp(dest, "abc", 3), "Expected first three chars to be \"abc\"\n");
    ok(!memcmp(&dest[3], invariate, sizeof(dest) - 3),
       "Expected rest of dest to be unchanged, got %s\n", dest);
    ok(destsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), destsz);

    /* pcDstSize is 0 */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = 0;
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);
    ok(destsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, destsz);

    /* pcDstSize is not large enough */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = lstrlenW(src);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, dest, &destsz);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);
    ok(srcsz == 0, "Expected 0, got %u\n", srcsz);
    ok(!strncmp(dest, "abc", 3), "Expected first three chars to be \"abc\"\n");
    ok(!memcmp(&dest[3], invariate, sizeof(dest) - 3),
       "Expected rest of dest to be unchanged, got %s\n", dest);
    ok(destsz == 0, "Expected 0, got %u\n", srcsz);

    /* pcDstSize (bytes) does not leave room for NULL terminator */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = lstrlenW(src) * sizeof(WCHAR);
    hr = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, src,
                                                  &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    ok(!lstrcmpA(dest, "abc"), "Expected \"abc\", got \"%s\"\n", dest);
    ok(destsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, destsz);
}

static void test_ConvertINetUnicodeToMultiByte(void)
{
    CHAR dest[MAX_PATH];
    CHAR invariate[MAX_PATH];
    INT srcsz, destsz;
    HRESULT hr;

    static WCHAR src[] = {'a','b','c',0};

    memset(invariate, 'x', sizeof(invariate));

    /* lpSrcStr NULL */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = sizeof(dest);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, NULL, &srcsz, dest, &destsz);
    ok(hr == S_OK || hr == E_FAIL, "Expected S_OK or E_FAIL, got %08x\n", hr);
    if (hr == S_OK)
        ok(srcsz == lstrlenW(src) + 1,
           "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    else if (hr == E_FAIL)
        ok(srcsz == 0,
           "Expected %u, got %u\n", 0, srcsz);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);
    ok(destsz == 0, "Expected 0, got %u\n", destsz);

    /* lpnWideCharCount NULL */
    memset(dest, 'x', sizeof(dest));
    destsz = sizeof(dest);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, NULL, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!strncmp(dest, "abc", 3),
       "Expected first three chars to be \"abc\"\n");
    ok(!memcmp(&dest[3], invariate, sizeof(dest) - 3),
       "Expected rest of dest to be unchanged, got %s\n", dest);
    ok(destsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), destsz);

    /* both lpSrcStr and lpnWideCharCount NULL */
    memset(dest, 'x', sizeof(dest));
    destsz = sizeof(dest);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, NULL, NULL, dest, &destsz);
    ok(hr == S_OK || hr == E_FAIL, "Expected S_OK or E_FAIL, got %08x\n", hr);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);
    ok(destsz == 0, "Expected 0, got %u\n", destsz);

    /* lpDstStr NULL */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = sizeof(dest);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, NULL, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    ok(destsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);

    /* lpnMultiCharCount NULL */
    memset(dest, 'x', sizeof(dest));
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, dest, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);

    /* lpnWideCharCount is 0 */
    memset(dest, 'x', sizeof(dest));
    srcsz = 0;
    destsz = sizeof(dest);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == 0, "Expected 0, got %u\n", srcsz);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);
    ok(destsz == 0, "Expected 0, got %u\n", destsz);

    /* lpnWideCharCount does not include NULL terminator */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src);
    destsz = sizeof(dest);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), srcsz);
    ok(!strncmp(dest, "abc", 3), "Expected first three chars to be \"abc\"\n");
    ok(!memcmp(&dest[3], invariate, sizeof(dest) - 3),
       "Expected rest of dest to be unchanged, got %s\n", dest);
    ok(destsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), destsz);

    /* lpnWideCharCount includes NULL terminator */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = sizeof(dest);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1, "Expected 3, got %u\n", srcsz);
    ok(!lstrcmpA(dest, "abc"), "Expected \"abc\", got \"%s\"\n", dest);
    ok(destsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, destsz);

    /* lpnWideCharCount is -1 */
    memset(dest, 'x', sizeof(dest));
    srcsz = -1;
    destsz = sizeof(dest);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), srcsz);
    ok(!strncmp(dest, "abc", 3), "Expected first three chars to be \"abc\"\n");
    ok(!memcmp(&dest[3], invariate, sizeof(dest) - 3),
       "Expected rest of dest to be unchanged, got %s\n", dest);
    ok(destsz == lstrlenW(src),
       "Expected %u, got %u\n", lstrlenW(src), destsz);

    /* lpnMultiCharCount is 0 */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = 0;
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    ok(!memcmp(dest, invariate, sizeof(dest)),
       "Expected dest to be unchanged, got %s\n", dest);
    ok(destsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, destsz);

    /* lpnMultiCharCount is not large enough */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = lstrlenW(src);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, dest, &destsz);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);
    ok(srcsz == 0, "Expected 0, got %u\n", srcsz);
    ok(!strncmp(dest, "abc", 3), "Expected first three chars to be \"abc\"\n");
    ok(!memcmp(&dest[3], invariate, sizeof(dest) - 3),
       "Expected rest of dest to be unchanged, got %s\n", dest);
    ok(destsz == 0, "Expected 0, got %u\n", srcsz);

    /* lpnMultiCharCount (bytes) does not leave room for NULL terminator */
    memset(dest, 'x', sizeof(dest));
    srcsz = lstrlenW(src) + 1;
    destsz = lstrlenW(src) * sizeof(WCHAR);
    hr = pConvertINetUnicodeToMultiByte(NULL, 1252, src, &srcsz, dest, &destsz);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(srcsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, srcsz);
    ok(!lstrcmpA(dest, "abc"), "Expected \"abc\", got \"%s\"\n", dest);
    ok(destsz == lstrlenW(src) + 1,
       "Expected %u, got %u\n", lstrlenW(src) + 1, destsz);
}

static void test_JapaneseConversion(void)
{
    /* Data */
    static WCHAR unc_jp[9][12] = {
  {9,0x31,0x20,0x3042,0x3044,0x3046,0x3048,0x304a,0x000d,0x000a},
  {9,0x31,0x20,0x30a2,0x30a4,0x30a6,0x30a8,0x30aa,0x000d,0x000a},
  {9,0x31,0x20,0xff71,0xff72,0xff73,0xff74,0xff75,0x000d,0x000a},
  {9,0x31,0x20,0x3041,0x3043,0x3045,0x3047,0x3049,0x000d,0x000a},
  {9,0x31,0x20,0x30a1,0x30a3,0x30a5,0x30a7,0x30a9,0x000d,0x000a},
  {9,0x31,0x20,0xff67,0xff68,0xff69,0xff6a,0xff6b,0x000d,0x000a},
  {9,0x31,0x20,0x300c,0x65e5,0x672c,0x8a9e,0x300d,0x000d,0x000a},
  {7,0x31,0x20,0x25c7,0x25c7,0x3012,0x000d,0x000a},
  {11,0x31,0x20,0x203b,0x3010,0x0074,0x0065,0x0073,0x0074,0x3011,0x000d,0x000a}
    };
    static CHAR jis_jp[9][27] = {
  {20,0x31,0x20,0x1b,0x24,0x42,0x24,0x22,0x24,0x24,0x24,0x26,0x24,0x28,
      0x24,0x2a,0x1b,0x28,0x42,0x0d,0x0a},
  {20,0x31,0x20,0x1b,0x24,0x42,0x25,0x22,0x25,0x24,0x25,0x26,0x25,0x28,
      0x25,0x2a,0x1b,0x28,0x42,0x0d,0x0a},
  {20,0x31,0x20,0x1b,0x24,0x42,0x25,0x22,0x25,0x24,0x25,0x26,0x25,0x28,
      0x25,0x2a,0x1b,0x28,0x42,0x0d,0x0a},
  {20,0x31,0x20,0x1b,0x24,0x42,0x24,0x21,0x24,0x23,0x24,0x25,0x24,0x27,
      0x24,0x29,0x1b,0x28,0x42,0x0d,0x0a},
  {20,0x31,0x20,0x1b,0x24,0x42,0x25,0x21,0x25,0x23,0x25,0x25,0x25,0x27,
      0x25,0x29,0x1b,0x28,0x42,0x0d,0x0a},
  {20,0x31,0x20,0x1b,0x24,0x42,0x25,0x21,0x25,0x23,0x25,0x25,0x25,0x27,
      0x25,0x29,0x1b,0x28,0x42,0x0d,0x0a},
  {20,0x31,0x20,0x1b,0x24,0x42,0x21,0x56,0x46,0x7c,0x4b,0x5c,0x38,0x6c,
      0x21,0x57,0x1b,0x28,0x42,0x0d,0x0a},
  {16,0x31,0x20,0x1b,0x24,0x42,0x21,0x7e,0x21,0x7e,0x22,0x29,0x1b,0x28,
      0x42,0x0d,0x0a},
  {26,0x31,0x20,0x1b,0x24,0x42,0x22,0x28,0x21,0x5a,0x1b,0x28,0x42,0x74,
      0x65,0x73,0x74,0x1b,0x24,0x42,0x21,0x5b,0x1b,0x28,0x42,0x0d,0x0a}
    };
    static CHAR sjis_jp[9][15] = {
  {14,0x31,0x20,0x82,0xa0,0x82,0xa2,0x82,0xa4,0x82,0xa6,0x82,0xa8,0x0d,0x0a},
  {14,0x31,0x20,0x83,0x41,0x83,0x43,0x83,0x45,0x83,0x47,0x83,0x49,0x0d,0x0a},
  {9,0x31,0x20,0xb1,0xb2,0xb3,0xb4,0xb5,0x0d,0x0a},
  {14,0x31,0x20,0x82,0x9f,0x82,0xa1,0x82,0xa3,0x82,0xa5,0x82,0xa7,0x0d,0x0a},
  {14,0x31,0x20,0x83,0x40,0x83,0x42,0x83,0x44,0x83,0x46,0x83,0x48,0x0d,0x0a},
  {9,0x31,0x20,0xa7,0xa8,0xa9,0xaa,0xab,0x0d,0x0a},
  {14,0x31,0x20,0x81,0x75,0x93,0xfa,0x96,0x7b,0x8c,0xea,0x81,0x76,0x0d,0x0a},
  {10,0x31,0x20,0x81,0x9e,0x81,0x9e,0x81,0xa7,0x0d,0x0a},
  {14,0x31,0x20,0x81,0xa6,0x81,0x79,0x74,0x65,0x73,0x74,0x81,0x7a,0x0d,0x0a}
    };
    static CHAR euc_jp[9][15] = {
  {14,0x31,0x20,0xa4,0xa2,0xa4,0xa4,0xa4,0xa6,0xa4,0xa8,0xa4,0xaa,0x0d,0x0a},
  {14,0x31,0x20,0xa5,0xa2,0xa5,0xa4,0xa5,0xa6,0xa5,0xa8,0xa5,0xaa,0x0d,0x0a},
  {14,0x31,0x20,0x8e,0xb1,0x8e,0xb2,0x8e,0xb3,0x8e,0xb4,0x8e,0xb5,0x0d,0x0a},
  {14,0x31,0x20,0xa4,0xa1,0xa4,0xa3,0xa4,0xa5,0xa4,0xa7,0xa4,0xa9,0x0d,0x0a},
  {14,0x31,0x20,0xa5,0xa1,0xa5,0xa3,0xa5,0xa5,0xa5,0xa7,0xa5,0xa9,0x0d,0x0a},
  {14,0x31,0x20,0x8e,0xa7,0x8e,0xa8,0x8e,0xa9,0x8e,0xaa,0x8e,0xab,0x0d,0x0a},
  {14,0x31,0x20,0xa1,0xd6,0xc6,0xfc,0xcb,0xdc,0xb8,0xec,0xa1,0xd7,0x0d,0x0a},
  {10,0x31,0x20,0xa1,0xfe,0xa1,0xfe,0xa2,0xa9,0x0d,0x0a},
  {14,0x31,0x20,0xa2,0xa8,0xa1,0xda,0x74,0x65,0x73,0x74,0xa1,0xdb,0x0d,0x0a}
    };

    INT srcsz, destsz;
    INT i;
    HRESULT hr;
    CHAR output[30];
    WCHAR outputW[30];
    int outlen;

    /* test unc->jis */
    for (i = 0; i < 9; i++)
    {
        int j;
        destsz = 30;
        outlen = jis_jp[i][0];
        srcsz = unc_jp[i][0];
        hr = pConvertINetUnicodeToMultiByte(NULL, 50220, &unc_jp[i][1], &srcsz, output, &destsz);
        if (hr == S_FALSE)
        {
            skip("Code page identifier 50220 is not supported\n");
            break;
        }
        ok(hr == S_OK,"(%i) Expected S_OK, got %08x\n", i, hr);
        ok(destsz == outlen, "(%i) Expected %i, got %i\n",i,outlen,destsz);
        ok(srcsz == unc_jp[i][0],"(%i) Expected %i, got %i\n",i,unc_jp[i][0],srcsz);
        ok(memcmp(output,&jis_jp[i][1],destsz)==0,"(%i) Strings do not match\n",i);

        /* and back */
        srcsz = outlen;
        destsz = 30;
        hr = pConvertINetMultiByteToUnicode(NULL, 50220, output, &srcsz, outputW,&destsz);

        /*
         * JIS does not have hankata so it get automatically converted to
         * zenkata. this means that strings 1 and 2 are identical as well as
         * strings 4 and 5.
         */
        j = i;
        if (i == 2) j = 1;
        if (i == 5) j = 4;

        ok(hr == S_OK,"(%i) Expected S_OK, got %08x\n",i, hr);
        ok(destsz == unc_jp[j][0],"(%i) Expected %i, got %i\n",i,unc_jp[j][0],destsz);
        ok(srcsz == outlen,"(%i) Expected %i, got %i\n",i,outlen,srcsz);
        ok(memcmp(outputW,&unc_jp[j][1],destsz)==0,"(%i) Strings do not match\n",i);
    }

    /* test unc->sjis */
    for (i = 0; i < 9; i++)
    {
        destsz = 30;
        outlen = sjis_jp[i][0];
        srcsz = unc_jp[i][0];

        hr = pConvertINetUnicodeToMultiByte(NULL, 932, &unc_jp[i][1], &srcsz, output, &destsz);
        if (hr == S_FALSE)
        {
            skip("Code page identifier 932 is not supported\n");
            break;
        }
        ok(hr == S_OK,"(%i) Expected S_OK, got %08x\n",i,hr);
        ok(destsz == outlen,"(%i) Expected %i, got %i\n",i,outlen,destsz);
        ok(srcsz == unc_jp[i][0],"(%i) Expected %i, got %i\n",i,unc_jp[i][0],srcsz);
        ok(memcmp(output,&sjis_jp[i][1],outlen)==0,"(%i) Strings do not match\n",i);

        srcsz = outlen;
        destsz = 30;
        hr = pConvertINetMultiByteToUnicode(NULL, 932, output, &srcsz, outputW,&destsz);

        ok(hr == S_OK,"(%i) Expected S_OK, got %08x\n", i, hr);
        ok(destsz == unc_jp[i][0],"(%i) Expected %i, got %i\n",i,unc_jp[i][0],destsz);
        ok(srcsz == outlen,"(%i) Expected %i, got %i\n",i,outlen,srcsz);
        ok(memcmp(outputW,&unc_jp[i][1],destsz)==0,"(%i) Strings do not match\n",i);
    }

    /* test unc->euc */
    for (i = 0; i < 9; i++)
    {
        destsz = 30;
        outlen = euc_jp[i][0];
        srcsz = unc_jp[i][0];

        hr = pConvertINetUnicodeToMultiByte(NULL, 51932, &unc_jp[i][1], &srcsz, output, &destsz);
        if (hr == S_FALSE)
        {
            skip("Code page identifier 51932 is not supported\n");
            break;
        }
        ok(hr == S_OK, "(%i) Expected S_OK, got %08x\n",i,hr);
        ok(destsz == outlen, "(%i) Expected %i, got %i\n",i,outlen,destsz);
        ok(srcsz == unc_jp[i][0],"(%i) Expected %i, got %i\n",i,unc_jp[i][0],destsz);
        ok(memcmp(output,&euc_jp[i][1],outlen)==0,"(%i) Strings do not match\n",i);

        srcsz = outlen;
        destsz = 30;
        hr = pConvertINetMultiByteToUnicode(NULL, 51932, output, &srcsz, outputW,&destsz);

        ok(hr == S_OK,"(%i) Expected S_OK, got %08x\n",i,hr);
        ok(destsz == unc_jp[i][0],"(%i) Expected %i, got %i\n",i,unc_jp[i][0],destsz);
        ok(srcsz == outlen,"(%i) Expected %i, got %i\n",i,outlen,srcsz);
        ok(memcmp(outputW,&unc_jp[i][1],destsz)==0,"(%i) Strings do not match\n",i);
    }

    /* Japanese autodetect */
    i = 0;
    destsz = 30;
    srcsz = jis_jp[i][0];
    hr = pConvertINetMultiByteToUnicode(NULL, 50932, &jis_jp[i][1], &srcsz, outputW, &destsz);
    if (hr == S_FALSE)
    {
        skip("Code page identifier 50932 is not supported\n");
        return;
    }
    ok(hr == S_OK,"(%i) Expected S_OK, got %08x\n",i,hr);
    ok(destsz == unc_jp[i][0],"(%i) Expected %i, got %i\n",i,unc_jp[i][0],destsz);
    ok(srcsz == jis_jp[i][0],"(%i) Expected %i, got %i\n",i,jis_jp[i][0],srcsz);
    ok(memcmp(outputW,&unc_jp[i][1],destsz)==0,"(%i) Strings do not match\n",i);

    i = 1;
    destsz = 30;
    srcsz = sjis_jp[i][0];
    hr = pConvertINetMultiByteToUnicode(NULL, 50932, &sjis_jp[i][1], &srcsz, outputW, &destsz);
    ok(hr == S_OK,"(%i) Expected S_OK, got %08x\n",i,hr);
    ok(destsz == unc_jp[i][0],"(%i) Expected %i, got %i\n",i,unc_jp[i][0],destsz);
    ok(srcsz == sjis_jp[i][0],"(%i) Expected %i, got %i\n",i,sjis_jp[i][0],srcsz);
    ok(memcmp(outputW,&unc_jp[i][1],destsz)==0,"(%i) Strings do not match\n",i);
}

static void test_GetScriptFontInfo(IMLangFontLink2 *font_link)
{
    HRESULT hr;
    UINT nfonts;
    SCRIPTFONTINFO sfi[1];

    nfonts = 0;
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidAsciiLatin, 0, &nfonts, NULL);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts, "unexpected result\n");

    nfonts = 0;
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidAsciiLatin, SCRIPTCONTF_FIXED_FONT, &nfonts, NULL);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts, "unexpected result\n");

    nfonts = 0;
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidAsciiLatin, SCRIPTCONTF_PROPORTIONAL_FONT, &nfonts, NULL);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts, "unexpected result\n");

    nfonts = 1;
    memset(sfi, 0, sizeof(sfi));
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidAsciiLatin, SCRIPTCONTF_FIXED_FONT, &nfonts, sfi);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts == 1, "got %u, expected 1\n", nfonts);
    ok(sfi[0].scripts != 0, "unexpected result\n");
    ok(sfi[0].wszFont[0], "unexpected result\n");

    nfonts = 1;
    memset(sfi, 0, sizeof(sfi));
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidAsciiLatin, SCRIPTCONTF_PROPORTIONAL_FONT, &nfonts, sfi);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts == 1, "got %u, expected 1\n", nfonts);
    ok(sfi[0].scripts != 0, "unexpected result\n");
    ok(sfi[0].wszFont[0], "unexpected result\n");
}

static void test_CodePageToScriptID(IMLangFontLink2 *font_link)
{
    HRESULT hr;
    UINT i;
    SCRIPT_ID sid;
    static const struct
    {
        UINT cp;
        SCRIPT_ID sid;
        HRESULT hr;
    }
    cp_sid[] =
    {
        {874,  sidThai},
        {932,  sidKana},
        {936,  sidHan},
        {949,  sidHangul},
        {950,  sidBopomofo},
        {1250, sidAsciiLatin},
        {1251, sidCyrillic},
        {1252, sidAsciiLatin},
        {1253, sidGreek},
        {1254, sidAsciiLatin},
        {1255, sidHebrew},
        {1256, sidArabic},
        {1257, sidAsciiLatin},
        {1258, sidAsciiLatin},
        {CP_UNICODE, 0, E_FAIL}
    };

    for (i = 0; i < sizeof(cp_sid)/sizeof(cp_sid[0]); i++)
    {
        hr = IMLangFontLink2_CodePageToScriptID(font_link, cp_sid[i].cp, &sid);
        ok(hr == cp_sid[i].hr, "%u CodePageToScriptID failed 0x%08x %u\n", i, hr, GetLastError());
        if (SUCCEEDED(hr))
        {
            ok(sid == cp_sid[i].sid,
               "%u got sid %u for codepage %u, expected %u\n", i, sid, cp_sid[i].cp, cp_sid[i].sid);
        }
    }
}

static void test_GetFontUnicodeRanges(IMLangFontLink2 *font_link)
{
    HRESULT hr;
    UINT count;
    HFONT hfont, old_hfont;
    LOGFONTA lf;
    HDC hdc;
    UNICODERANGE *ur;

    hdc = CreateCompatibleDC(0);
    memset(&lf, 0, sizeof(lf));
    lstrcpyA(lf.lfFaceName, "Arial");
    hfont = CreateFontIndirectA(&lf);
    old_hfont = SelectObject(hdc, hfont);

    count = 0;
    hr = IMLangFontLink2_GetFontUnicodeRanges(font_link, NULL, &count, NULL);
    ok(hr == E_FAIL, "expected E_FAIL, got 0x%08x\n", hr);

    hr = IMLangFontLink2_GetFontUnicodeRanges(font_link, hdc, NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_FAIL, got 0x%08x\n", hr);

    count = 0;
    hr = IMLangFontLink2_GetFontUnicodeRanges(font_link, hdc, &count, NULL);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(count, "expected count > 0\n");

    ur = HeapAlloc(GetProcessHeap(), 0, sizeof(*ur) * count);

    hr = IMLangFontLink2_GetFontUnicodeRanges(font_link, hdc, &count, ur);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);

    count--;
    hr = IMLangFontLink2_GetFontUnicodeRanges(font_link, hdc, &count, ur);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);

    HeapFree(GetProcessHeap(), 0, ur);

    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);
    DeleteDC(hdc);
}

static void test_IsCodePageInstallable(IMultiLanguage2 *ml2)
{
    UINT i;
    HRESULT hr;

    for (i = 0; i < 0xffff; i++)
    {
        hr = IMultiLanguage2_IsCodePageInstallable(ml2, i);

        /* it would be better to use IMultiLanguage2_ValidateCodePageEx here but that brings
         * up an installation dialog on some platforms, even when specifying CPIOD_PEEK.
         */
        if (IsValidCodePage(i))
            ok(hr == S_OK ||
               broken(hr == S_FALSE) ||  /* win2k */
               broken(hr == E_INVALIDARG),  /* win2k */
               "code page %u is valid but not installable 0x%08x\n", i, hr);
    }
}

static void test_GetGlobalFontLinkObject(void)
{
    HRESULT ret;
    void *unknown;

    ret = GetGlobalFontLinkObject(NULL);
    ok(ret == E_INVALIDARG, "expected E_INVALIDARG got %#x\n", ret);

    unknown = (void *)0xdeadbeef;
    ret = GetGlobalFontLinkObject(&unknown);
todo_wine {
    ok(ret == S_OK, "expected S_OK got %#x\n", ret);
    ok(unknown != NULL && unknown != (void *)0xdeadbeef,
       "GetGlobalFontLinkObject() returned %p\n", unknown);
    }
}

static void test_IMLangConvertCharset(IMultiLanguage *ml)
{
    IMLangConvertCharset *convert;
    WCHAR strW[] = {'a','b','c','d',0};
    UINT cp, src_size, dst_size;
    char strA[] = "abcd";
    WCHAR buffW[20];
    HRESULT hr;

    hr = IMultiLanguage_CreateConvertCharset(ml, CP_ACP, CP_UTF8, 0, &convert);
todo_wine
    ok(hr == S_FALSE, "expected S_FALSE got 0x%08x\n", hr);

    hr = IMLangConvertCharset_GetSourceCodePage(convert, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    cp = CP_UTF8;
    hr = IMLangConvertCharset_GetSourceCodePage(convert, &cp);
    ok(hr == S_OK, "expected S_OK got 0x%08x\n", hr);
    ok(cp == CP_ACP, "got %d\n", cp);

    hr = IMLangConvertCharset_GetDestinationCodePage(convert, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    cp = CP_ACP;
    hr = IMLangConvertCharset_GetDestinationCodePage(convert, &cp);
    ok(hr == S_OK, "expected S_OK got 0x%08x\n", hr);
    ok(cp == CP_UTF8, "got %d\n", cp);

    /* DoConversionToUnicode */
    hr = IMLangConvertCharset_Initialize(convert, CP_UTF8, CP_UNICODE, 0);
    ok(hr == S_OK, "expected S_OK got 0x%08x\n", hr);

    hr = IMLangConvertCharset_DoConversionToUnicode(convert, NULL, NULL, NULL, NULL);
    ok(hr == E_FAIL || broken(hr == S_OK) /* win2k */, "got 0x%08x\n", hr);

    src_size = -1;
    dst_size = 20;
    buffW[0] = 0;
    buffW[4] = 4;
    hr = IMLangConvertCharset_DoConversionToUnicode(convert, strA, &src_size, buffW, &dst_size);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!memcmp(buffW, strW, 4*sizeof(WCHAR)), "got converted string %s\n", wine_dbgstr_wn(buffW, dst_size));
    ok(dst_size == 4, "got %d\n", dst_size);
    ok(buffW[4] == 4, "got %d\n", buffW[4]);
    ok(src_size == 4, "got %d\n", src_size);

    src_size = -1;
    dst_size = 0;
    buffW[0] = 1;
    hr = IMLangConvertCharset_DoConversionToUnicode(convert, strA, &src_size, buffW, &dst_size);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(buffW[0] == 1, "got %d\n", buffW[0]);
    ok(dst_size == 4, "got %d\n", dst_size);
    ok(src_size == 4, "got %d\n", src_size);

    hr = IMLangConvertCharset_Initialize(convert, CP_UNICODE, CP_UNICODE, 0);
    ok(hr == S_OK, "expected S_OK got 0x%08x\n", hr);

    IMLangConvertCharset_Release(convert);
}

static const char stream_data[] = "VCARD2.1test;test";
static ULONG stream_pos;

static HRESULT WINAPI stream_QueryInterface(IStream *iface, REFIID riid, void **obj)
{
    ok(FALSE, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI stream_AddRef(IStream *iface)
{
    ok(FALSE, "unexpected call\n");
    return 2;
}

static ULONG WINAPI stream_Release(IStream *iface)
{
    ok(FALSE, "unexpected call\n");
    return 1;
}

static HRESULT WINAPI stream_Read(IStream *iface, void *buf, ULONG len, ULONG *read)
{
    ULONG size;

    if (stream_pos == sizeof(stream_data) - 1)
    {
        *read = 0;
        return S_FALSE;
    }
    size = min(sizeof(stream_data) - 1 - stream_pos, len);
    memcpy(buf, stream_data + stream_pos, size);
    stream_pos += size;
    *read = size;
    return S_OK;
}

static HRESULT WINAPI stream_Write(IStream *iface, const void *buf, ULONG len, ULONG *written)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin,
    ULARGE_INTEGER *newpos)
{
    if (origin == STREAM_SEEK_SET)
        stream_pos = move.QuadPart;
    else if (origin == STREAM_SEEK_CUR)
        stream_pos += move.QuadPart;
    else if (origin == STREAM_SEEK_END)
        stream_pos = sizeof(stream_data) - 1 - move.QuadPart;

    if (newpos) newpos->QuadPart = stream_pos;
    return S_OK;
}

static HRESULT WINAPI stream_SetSize(IStream *iface, ULARGE_INTEGER newsize)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_CopyTo(IStream *iface, IStream *stream, ULARGE_INTEGER len,
    ULARGE_INTEGER *read, ULARGE_INTEGER *written)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Commit(IStream *iface, DWORD flags)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Revert(IStream *iface)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_LockRegion(IStream *iface, ULARGE_INTEGER offset,
    ULARGE_INTEGER len, DWORD locktype)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset,
    ULARGE_INTEGER len, DWORD locktype)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Stat(IStream *iface, STATSTG *stg, DWORD flag)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Clone(IStream *iface, IStream **stream)
{
    ok(FALSE, "unexpected call\n");
    return E_NOTIMPL;
}

static /* const */ IStreamVtbl stream_vtbl =
{
    stream_QueryInterface,
    stream_AddRef,
    stream_Release,
    stream_Read,
    stream_Write,
    stream_Seek,
    stream_SetSize,
    stream_CopyTo,
    stream_Commit,
    stream_Revert,
    stream_LockRegion,
    stream_UnlockRegion,
    stream_Stat,
    stream_Clone
};

static IStream test_stream = { &stream_vtbl };

static void test_DetectOutboundCodePageInIStream(IMultiLanguage3 *ml)
{
    HRESULT hr;
    UINT nb_detected, detected[4];
    UINT preferred[] = {1250,1251,1252,65001};
    UINT preferred2[] = {1250,1251,1252};

    nb_detected = 0;
    memset(detected, 0, sizeof(detected));
    hr = IMultiLanguage3_DetectOutboundCodePageInIStream(ml,
        MLDETECTF_PRESERVE_ORDER, &test_stream, preferred,
        sizeof(preferred)/sizeof(preferred[0]), detected, &nb_detected, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    nb_detected = 1;
    memset(detected, 0, sizeof(detected));
    hr = IMultiLanguage3_DetectOutboundCodePageInIStream(ml,
        MLDETECTF_PRESERVE_ORDER, &test_stream, preferred,
        sizeof(preferred)/sizeof(preferred[0]), NULL, &nb_detected, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    nb_detected = 1;
    memset(detected, 0, sizeof(detected));
    hr = IMultiLanguage3_DetectOutboundCodePageInIStream(ml,
        MLDETECTF_PRESERVE_ORDER, &test_stream, preferred,
        sizeof(preferred)/sizeof(preferred[0]), detected, &nb_detected, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(nb_detected == 1, "got %u\n", nb_detected);
    ok(detected[0] == 65001, "got %u\n", detected[0]);

    nb_detected = 2;
    memset(detected, 0, sizeof(detected));
    hr = IMultiLanguage3_DetectOutboundCodePageInIStream(ml,
        MLDETECTF_PRESERVE_ORDER, &test_stream, preferred,
        sizeof(preferred)/sizeof(preferred[0]), detected, &nb_detected, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    todo_wine ok(nb_detected == 2, "got %u\n", nb_detected);
    ok(detected[0] == 65001, "got %u\n", detected[0]);
    todo_wine ok(detected[1] == 65000, "got %u\n", detected[1]);

    nb_detected = 3;
    memset(detected, 0, sizeof(detected));
    hr = IMultiLanguage3_DetectOutboundCodePageInIStream(ml,
        MLDETECTF_PRESERVE_ORDER, &test_stream, preferred,
        sizeof(preferred)/sizeof(preferred[0]), detected, &nb_detected, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    todo_wine ok(nb_detected == 3, "got %u\n", nb_detected);
    ok(detected[0] == 65001, "got %u\n", detected[0]);
    todo_wine ok(detected[1] == 65000, "got %u\n", detected[1]);
    todo_wine ok(detected[2] == 1200, "got %u\n", detected[2]);

    nb_detected = 4;
    memset(detected, 0, sizeof(detected));
    hr = IMultiLanguage3_DetectOutboundCodePageInIStream(ml,
        MLDETECTF_PRESERVE_ORDER, &test_stream, preferred,
        sizeof(preferred)/sizeof(preferred[0]), detected, &nb_detected, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    todo_wine ok(nb_detected == 3, "got %u\n", nb_detected);
    ok(detected[0] == 65001, "got %u\n", detected[0]);
    todo_wine ok(detected[1] == 65000, "got %u\n", detected[1]);
    todo_wine ok(detected[2] == 1200, "got %u\n", detected[2]);
    ok(detected[3] == 0, "got %u\n", detected[3]);

    nb_detected = 3;
    memset(detected, 0, sizeof(detected));
    hr = IMultiLanguage3_DetectOutboundCodePageInIStream(ml,
        MLDETECTF_PRESERVE_ORDER, &test_stream, preferred2,
        sizeof(preferred2)/sizeof(preferred2[0]), detected, &nb_detected, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    todo_wine ok(nb_detected == 3, "got %u\n", nb_detected);
    ok(detected[0] == 65001, "got %u\n", detected[0]);
    todo_wine ok(detected[1] == 65000, "got %u\n", detected[1]);
    todo_wine ok(detected[2] == 1200, "got %u\n", detected[2]);
}

START_TEST(mlang)
{
    IMultiLanguage  *iML = NULL;
    IMultiLanguage2 *iML2 = NULL;
    IMultiLanguage3 *iML3 = NULL;
    IMLangFontLink  *iMLFL = NULL;
    IMLangFontLink2 *iMLFL2 = NULL;
    HRESULT ret;

    if (!init_function_ptrs())
        return;

    CoInitialize(NULL);
    test_Rfc1766ToLcid();
    test_LcidToRfc1766();

    test_ConvertINetUnicodeToMultiByte();
    test_JapaneseConversion();

    test_GetGlobalFontLinkObject();

    trace("IMultiLanguage\n");
    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMultiLanguage, (void **)&iML);
    if (ret != S_OK || !iML) return;

    test_GetNumberOfCodePageInfo((IMultiLanguage2 *)iML);
    test_IMLangConvertCharset(iML);
    test_GetCharsetInfo_alias(iML);
    IMultiLanguage_Release(iML);


    /* IMultiLanguage2 (IE5.0 and above) */
    trace("IMultiLanguage2\n");
    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMultiLanguage2, (void **)&iML2);
    if (ret != S_OK || !iML2) return;

    test_rfc1766(iML2);
    test_GetLcidFromRfc1766(iML2);
    test_GetRfc1766FromLcid(iML2);
    test_GetRfc1766Info(iML2);
    test_GetNumberOfCodePageInfo(iML2);

    test_EnumCodePages(iML2, 0);
    test_EnumCodePages(iML2, MIMECONTF_MIME_LATEST);
    test_EnumCodePages(iML2, MIMECONTF_BROWSER);
    test_EnumCodePages(iML2, MIMECONTF_MINIMAL);
    test_EnumCodePages(iML2, MIMECONTF_VALID);
    /* FIXME: why MIMECONTF_MIME_REGISTRY returns 0 of supported codepages? */
    /*test_EnumCodePages(iML2, MIMECONTF_MIME_REGISTRY);*/

    test_EnumScripts(iML2, 0);
    test_EnumScripts(iML2, SCRIPTCONTF_SCRIPT_USER);
    test_EnumScripts(iML2, SCRIPTCONTF_SCRIPT_USER | SCRIPTCONTF_SCRIPT_HIDE | SCRIPTCONTF_SCRIPT_SYSTEM);

    ret = IMultiLanguage2_IsConvertible(iML2, CP_UTF8, CP_UNICODE);
    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(CP_UTF8 -> CP_UNICODE) = %08x\n", ret);
    ret = IMultiLanguage2_IsConvertible(iML2, CP_UNICODE, CP_UTF8);
    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(CP_UNICODE -> CP_UTF8) = %08x\n", ret);

    test_multibyte_to_unicode_translations(iML2);
    test_IMultiLanguage2_ConvertStringFromUnicode(iML2);

    test_IsCodePageInstallable(iML2);

    IMultiLanguage2_Release(iML2);


    /* IMLangFontLink */
    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMLangFontLink, (void **)&iMLFL);
    if (ret != S_OK || !iMLFL) return;

    IMLangFontLink_Test(iMLFL);
    IMLangFontLink_Release(iMLFL);

    /* IMLangFontLink2 */
    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMLangFontLink2, (void **)&iMLFL2);
    if (ret != S_OK || !iMLFL2) return;

    test_GetScriptFontInfo(iMLFL2);
    test_GetFontUnicodeRanges(iMLFL2);
    test_CodePageToScriptID(iMLFL2);
    IMLangFontLink2_Release(iMLFL2);

    trace("IMultiLanguage3\n");
    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMultiLanguage3, (void **)&iML3);
    if (ret == S_OK)
    {
        test_DetectOutboundCodePageInIStream(iML3);
        IMultiLanguage3_Release(iML3);
    }

    CoUninitialize();
}
