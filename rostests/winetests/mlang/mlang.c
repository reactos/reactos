/*
 * Unit test suite for MLANG APIs.
 *
 * Copyright 2004 Dmitry Timoshkov
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "initguid.h"
#include "mlang.h"

#include "wine/test.h"

#ifndef CP_UNICODE
#define CP_UNICODE 1200
#endif

#if 0
#define DUMP_CP_INFO
#define DUMP_SCRIPT_INFO

#if defined DUMP_CP_INFO || defined DUMP_SCRIPT_INFO
#include "wine/debug.h"
#endif
#endif /* 0 */

static BOOL (WINAPI *pGetCPInfoExA)(UINT, DWORD, LPCPINFOEXA);
static HRESULT (WINAPI *pConvertINetMultiByteToUnicode)(LPDWORD, DWORD, LPCSTR,
                                                        LPINT, LPWSTR, LPINT);
static HRESULT (WINAPI *pConvertINetUnicodeToMultiByte)(LPDWORD, DWORD, LPCWSTR,
                                                        LPINT, LPSTR, LPINT);

static BOOL init_function_ptrs(void)
{
    HMODULE hMlang;

    hMlang = LoadLibraryA("mlang.dll");
    if (!hMlang)
    {
        skip("mlang not available\n");
        return FALSE;
    }

    pConvertINetMultiByteToUnicode = (void *)GetProcAddress(hMlang, "ConvertINetMultiByteToUnicode");
    pConvertINetUnicodeToMultiByte = (void *)GetProcAddress(hMlang, "ConvertINetUnicodeToMultiByte");

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
    if (ret == S_FALSE)
        ok(n == 0 && ret == S_FALSE, "IEnumCodePage_Next: expected 0/S_FALSE, got %u/%08x\n", n, ret);
    else if (ret == E_FAIL)
        ok(n == 65536 && ret == E_FAIL, "IEnumCodePage_Next: expected 65536/E_FAIL, got %u/%08x\n", n, ret);
    ret = IEnumCodePage_Next(iEnumCP, 0, NULL, NULL);
    if (ret == S_FALSE)
        ok(ret == S_FALSE, "IEnumCodePage_Next: expected S_FALSE, got %08x\n", ret);
    else if (ret == E_FAIL)
        ok(n == 65536 && ret == E_FAIL, "IEnumCodePage_Next: expected 65536/E_FAIL, got %u/%08x\n", n, ret);

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
	    trace("TranslateCharsetInfo failed for cp %u\n", cpinfo[i].uiFamilyCodePage);

#ifdef DUMP_CP_INFO
        trace("%u: codepage %u family %u\n", i, cpinfo[i].uiCodePage, cpinfo[i].uiFamilyCodePage);
#endif
        /* Win95 does not support UTF-7 */
        if (cpinfo[i].uiCodePage == CP_UTF7) continue;

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
            ok(!lstrcmpiW(cpinfo[i].wszWebCharset, mcsi.wszCharset),
#ifdef DUMP_CP_INFO
                    "%s != %s\n",
            wine_dbgstr_w(cpinfo[i].wszWebCharset), wine_dbgstr_w(mcsi.wszCharset));
#else
                    "wszWebCharset mismatch\n");
#endif

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
            ok(!lstrcmpiW(cpinfo[i].wszHeaderCharset, mcsi.wszCharset),
#ifdef DUMP_CP_INFO
                    "%s != %s\n",
            wine_dbgstr_w(cpinfo[i].wszHeaderCharset), wine_dbgstr_w(mcsi.wszCharset));
#else
                    "wszHeaderCharset mismatch\n");
#endif

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
            ok(!lstrcmpiW(cpinfo[i].wszBodyCharset, mcsi.wszCharset),
#ifdef DUMP_CP_INFO
                    "%s != %s\n",
            wine_dbgstr_w(cpinfo[i].wszBodyCharset), wine_dbgstr_w(mcsi.wszCharset));
#else
                    "wszBodyCharset mismatch\n");
#endif

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
    cpinfo_cmp(&cpinfo[0], &cpinfo2);

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
	flags = SCRIPTCONTF_SCRIPT_USER | SCRIPTCONTF_SCRIPT_HIDE | SCRIPTCONTF_SCRIPT_SYSTEM;
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
    scriptinfo_cmp(&sinfo[0], &sinfo2);

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
    static const WCHAR str[3] = { 'd', 0x0436, 0xff90 };
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
    ret = IMLangFontLink_GetStrCodePages(iMLFL, &str[0], 1, 0, &dwCodePages, &processed);
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
    LCID lcid;
    HRESULT ret;

    static WCHAR e[] = { 'e',0 };
    static WCHAR en[] = { 'e','n',0 };
    static WCHAR empty[] = { 0 };
    static WCHAR dash[] = { '-',0 };
    static WCHAR e_dash[] = { 'e','-',0 };
    static WCHAR en_gb[] = { 'e','n','-','g','b',0 };
    static WCHAR en_us[] = { 'e','n','-','u','s',0 };
    static WCHAR en_them[] = { 'e','n','-','t','h','e','m',0 };
    static WCHAR english[] = { 'e','n','g','l','i','s','h',0 };

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, NULL, en);
    ok(ret == E_INVALIDARG, "GetLcidFromRfc1766 returned: %08x\n", ret);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, NULL);
    ok(ret == E_INVALIDARG, "GetLcidFromRfc1766 returned: %08x\n", ret);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, e);
    ok(ret == E_FAIL, "GetLcidFromRfc1766 returned: %08x\n", ret);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, empty);
    ok(ret == E_FAIL, "GetLcidFromRfc1766 returned: %08x\n", ret);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, dash);
    ok(ret == E_FAIL, "GetLcidFromRfc1766 returned: %08x\n", ret);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, e_dash);
    ok(ret == E_FAIL, "GetLcidFromRfc1766 returned: %08x\n", ret);

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

    lcid = 0;

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, en);
    ok(ret == S_OK, "GetLcidFromRfc1766 returned: %08x\n", ret);
    ok(lcid == 9, "got wrong lcid: %04x\n", lcid);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, en_gb);
    ok(ret == S_OK, "GetLcidFromRfc1766 returned: %08x\n", ret);
    ok(lcid == 0x809, "got wrong lcid: %04x\n", lcid);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, en_us);
    ok(ret == S_OK, "GetLcidFromRfc1766 returned: %08x\n", ret);
    ok(lcid == 0x409, "got wrong lcid: %04x\n", lcid);
}

static void test_GetRfc1766FromLcid(IMultiLanguage2 *iML2)
{
    HRESULT hr;
    BSTR rfcstr;
    LCID lcid;

    static WCHAR kok[] = {'k','o','k',0};

    hr = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, kok);
    /*
     * S_FALSE happens when 'kok' instead matches to a different Rfc1766 name
     * for example 'ko' so it is not a failure but does not give us what 
     * we are looking for
     */
    if (hr != S_FALSE)
    {
        ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

        hr = IMultiLanguage2_GetRfc1766FromLcid(iML2, lcid, &rfcstr);
        ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
        ok_w2("Expected \"%s\",  got \"%s\"n", kok, rfcstr);
        SysFreeString(rfcstr);
    }
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
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidLatin, 0, &nfonts, NULL);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts, "unexpected result\n");

    nfonts = 0;
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidLatin, SCRIPTCONTF_FIXED_FONT, &nfonts, NULL);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts, "unexpected result\n");

    nfonts = 0;
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidLatin, SCRIPTCONTF_PROPORTIONAL_FONT, &nfonts, NULL);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts, "unexpected result\n");

    nfonts = 1;
    memset(sfi, 0, sizeof(sfi));
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidLatin, SCRIPTCONTF_FIXED_FONT, &nfonts, sfi);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts == 1, "got %u, expected 1\n", nfonts);
    ok(sfi[0].scripts != 0, "unexpected result\n");
    ok(sfi[0].wszFont[0], "unexpected result\n");

    nfonts = 1;
    memset(sfi, 0, sizeof(sfi));
    hr = IMLangFontLink2_GetScriptFontInfo(font_link, sidLatin, SCRIPTCONTF_PROPORTIONAL_FONT, &nfonts, sfi);
    ok(hr == S_OK, "GetScriptFontInfo failed %u\n", GetLastError());
    ok(nfonts == 1, "got %u, expected 1\n", nfonts);
    ok(sfi[0].scripts != 0, "unexpected result\n");
    ok(sfi[0].wszFont[0], "unexpected result\n");
}

START_TEST(mlang)
{
    IMultiLanguage2 *iML2 = NULL;
    IMLangFontLink  *iMLFL = NULL;
    IMLangFontLink2 *iMLFL2 = NULL;
    HRESULT ret;

    if (!init_function_ptrs())
        return;

    CoInitialize(NULL);
    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMultiLanguage2, (void **)&iML2);
    if (ret != S_OK || !iML2) return;

    test_rfc1766(iML2);
    test_GetLcidFromRfc1766(iML2);
    test_GetRfc1766FromLcid(iML2);

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

    IMultiLanguage2_Release(iML2);

    test_ConvertINetUnicodeToMultiByte();

    test_JapaneseConversion();

    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMLangFontLink, (void **)&iMLFL);
    if (ret != S_OK || !iMLFL) return;

    IMLangFontLink_Test(iMLFL);
    IMLangFontLink_Release(iMLFL);

    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMLangFontLink2, (void **)&iMLFL2);
    if (ret != S_OK || !iMLFL2) return;

    test_GetScriptFontInfo(iMLFL2);
    IMLangFontLink2_Release(iMLFL2);

    CoUninitialize();
}
