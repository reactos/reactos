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

#define TRACE_2 OutputDebugStringA

static BOOL (WINAPI *pGetCPInfoExA)(UINT,DWORD,LPCPINFOEXA);

static void test_multibyte_to_unicode_translations(IMultiLanguage2 *iML2)
{
    /* these APIs are broken regarding constness of the input buffer */
    char stringA[] = "Just a test string\0"; /* double 0 for CP_UNICODE tests */
    WCHAR stringW[] = {'J','u','s','t',' ','a',' ','t','e','s','t',' ','s','t','r','i','n','g',0};
    char bufA[256];
    WCHAR bufW[256];
    UINT lenA, lenW, expected_len;
    HRESULT ret;
    HMODULE hMlang;
    FARPROC pConvertINetMultiByteToUnicode;
    FARPROC pConvertINetUnicodeToMultiByte;

    hMlang = LoadLibraryA("mlang.dll");
    ok(hMlang != 0, "couldn't load mlang.dll\n");

    pConvertINetMultiByteToUnicode = GetProcAddress(hMlang, "ConvertINetMultiByteToUnicode");
    ok(pConvertINetMultiByteToUnicode != NULL, "couldn't resolve ConvertINetMultiByteToUnicode\n");
    pConvertINetUnicodeToMultiByte = GetProcAddress(hMlang, "ConvertINetUnicodeToMultiByte");
    ok(pConvertINetUnicodeToMultiByte != NULL, "couldn't resolve ConvertINetUnicodeToMultiByte\n");

    /* IMultiLanguage2_ConvertStringToUnicode tests */

    memset(bufW, 'x', sizeof(bufW));
    lenA = 0;
    lenW = sizeof(bufW)/sizeof(bufW[0]);
    TRACE_2("Call IMultiLanguage2_ConvertStringToUnicode\n");
    ret = IMultiLanguage2_ConvertStringToUnicode(iML2, NULL, 1252, stringA, &lenA, bufW, &lenW);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringToUnicode failed: %08x\n", ret);
    ok(lenA == 0, "expected lenA 0, got %u\n", lenA);
    ok(lenW == 0, "expected lenW 0, got %u\n", lenW);

    memset(bufW, 'x', sizeof(bufW));
    lenA = -1;
    lenW = sizeof(bufW)/sizeof(bufW[0]);
    TRACE_2("Call IMultiLanguage2_ConvertStringToUnicode\n");
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
    TRACE_2("Call IMultiLanguage2_ConvertStringToUnicode\n");
    ret = IMultiLanguage2_ConvertStringToUnicode(iML2, NULL, 1252, stringA, &lenA, bufW, &lenW);
    ok(ret == E_FAIL, "IMultiLanguage2_ConvertStringToUnicode should fail: %08x\n", ret);
    ok(lenW == 0, "expected lenW 0, got %u\n", lenW);
    /* still has to do partial conversion */
    ok(!memcmp(bufW, stringW, 5 * sizeof(WCHAR)), "bufW/stringW mismatch\n");

    memset(bufW, 'x', sizeof(bufW));
    lenA = -1;
    lenW = sizeof(bufW)/sizeof(bufW[0]);
    TRACE_2("Call IMultiLanguage2_ConvertStringToUnicode\n");
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
    ret = pConvertINetMultiByteToUnicode(NULL, 1252, stringA, &lenA, NULL, &lenW);
    ok(ret == S_OK, "ConvertINetMultiByteToUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA), "expected lenA %u, got %u\n", lstrlenA(stringA), lenA);
    expected_len = MultiByteToWideChar(1252, 0, stringA, lenA, NULL, 0);
    ok(lenW == expected_len, "expected lenW %u, got %u\n", expected_len, lenW);

    memset(bufW, 'x', sizeof(bufW));
    lenA = lstrlenA(stringA);
    lenW = 0;
    ret = pConvertINetMultiByteToUnicode(NULL, 1252, stringA, &lenA, NULL, &lenW);
    ok(ret == S_OK, "ConvertINetMultiByteToUnicode failed: %08x\n", ret);
    ok(lenA == lstrlenA(stringA), "expected lenA %u, got %u\n", lstrlenA(stringA), lenA);
    expected_len = MultiByteToWideChar(1252, 0, stringA, lenA, NULL, 0);
    ok(lenW == expected_len, "expected lenW %u, got %u\n", expected_len, lenW);

    /* IMultiLanguage2_ConvertStringFromUnicode tests */

    memset(bufA, 'x', sizeof(bufA));
    lenW = 0;
    lenA = sizeof(bufA);
    TRACE_2("Call IMultiLanguage2_ConvertStringFromUnicode\n");
    ret = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, stringW, &lenW, bufA, &lenA);
    ok(ret == S_OK, "IMultiLanguage2_ConvertStringFromUnicode failed: %08x\n", ret);
    ok(lenA == 0, "expected lenA 0, got %u\n", lenA);
    ok(lenW == 0, "expected lenW 0, got %u\n", lenW);

    memset(bufA, 'x', sizeof(bufA));
    lenW = -1;
    lenA = sizeof(bufA);
    TRACE_2("Call IMultiLanguage2_ConvertStringFromUnicode\n");
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
    TRACE_2("Call IMultiLanguage2_ConvertStringFromUnicode\n");
    ret = IMultiLanguage2_ConvertStringFromUnicode(iML2, NULL, 1252, stringW, &lenW, bufA, &lenA);
    ok(ret == E_FAIL, "IMultiLanguage2_ConvertStringFromUnicode should fail: %08x\n", ret);
    ok(lenA == 0, "expected lenA 0, got %u\n", lenA);
    /* still has to do partial conversion */
    ok(!memcmp(bufA, stringA, 5), "bufW/stringW mismatch\n");

    memset(bufA, 'x', sizeof(bufA));
    lenW = -1;
    lenA = sizeof(bufA);
    TRACE_2("Call IMultiLanguage2_ConvertStringFromUnicode\n");
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

    memset(bufA, 'x', sizeof(bufA));
    lenW = lstrlenW(stringW);
    lenA = sizeof(bufA);
    ret = pConvertINetUnicodeToMultiByte(NULL, 1252, stringW, &lenW, NULL, &lenA);
    ok(ret == S_OK, "ConvertINetUnicodeToMultiByte failed: %08x\n", ret);
    ok(lenW == lstrlenW(stringW), "expected lenW %u, got %u\n", lstrlenW(stringW), lenW);
    expected_len = WideCharToMultiByte(1252, 0, stringW, lenW, NULL, 0, NULL, NULL);
    ok(lenA == expected_len, "expected lenA %u, got %u\n", expected_len, lenA);

    memset(bufA, 'x', sizeof(bufA));
    lenW = lstrlenW(stringW);
    lenA = 0;
    ret = pConvertINetUnicodeToMultiByte(NULL, 1252, stringW, &lenW, NULL, &lenA);
    ok(ret == S_OK, "ConvertINetUnicodeToMultiByte failed: %08x\n", ret);
    ok(lenW == lstrlenW(stringW), "expected lenW %u, got %u\n", lstrlenW(stringW), lenW);
    expected_len = WideCharToMultiByte(1252, 0, stringW, lenW, NULL, 0, NULL, NULL);
    ok(lenA == expected_len, "expected lenA %u, got %u\n", expected_len, lenA);
}

static inline void cpinfo_cmp(MIMECPINFO *cpinfo1, MIMECPINFO *cpinfo2)
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

static void test_EnumCodePages(IMultiLanguage2 *iML2, DWORD flags)
{
    IEnumCodePage *iEnumCP = NULL;
    MIMECPINFO *cpinfo;
    MIMECPINFO cpinfo2;
    HRESULT ret;
    ULONG i, n;
    UINT total;

    total = 0;
    TRACE_2("Call IMultiLanguage2_GetNumberOfCodePageInfo\n");
    ret = IMultiLanguage2_GetNumberOfCodePageInfo(iML2, &total);
    ok(ret == S_OK && total != 0, "IMultiLanguage2_GetNumberOfCodePageInfo: expected S_OK/!0, got %08x/%u\n", ret, total);

    trace("total mlang supported codepages %u\n", total);

    TRACE_2("Call IMultiLanguage2_EnumCodePages\n");
    ret = IMultiLanguage2_EnumCodePages(iML2, flags, LANG_NEUTRAL, &iEnumCP);
    trace("IMultiLanguage2_EnumCodePages = %08x, iEnumCP = %p\n", ret, iEnumCP);
    ok(ret == S_OK && iEnumCP, "IMultiLanguage2_EnumCodePages: expected S_OK/!NULL, got %08x/%p\n", ret, iEnumCP);

    TRACE_2("Call IEnumCodePage_Reset\n");
    ret = IEnumCodePage_Reset(iEnumCP);
    ok(ret == S_OK, "IEnumCodePage_Reset: expected S_OK, got %08x\n", ret);
    n = 65536;
    TRACE_2("Call IEnumCodePage_Next\n");
    ret = IEnumCodePage_Next(iEnumCP, 0, NULL, &n);
    ok(n == 0 && ret == S_FALSE, "IEnumCodePage_Next: expected 0/S_FALSE, got %u/%08x\n", n, ret);
    TRACE_2("Call IEnumCodePage_Next\n");
    ret = IEnumCodePage_Next(iEnumCP, 0, NULL, NULL);
    ok(ret == S_FALSE, "IEnumCodePage_Next: expected S_FALSE, got %08x\n", ret);

    cpinfo = HeapAlloc(GetProcessHeap(), 0, sizeof(*cpinfo) * total * 2);

    n = total * 2;
    TRACE_2("Call IEnumCodePage_Next\n");
    ret = IEnumCodePage_Next(iEnumCP, 0, cpinfo, &n);
    trace("IEnumCodePage_Next = %08x, n = %u\n", ret, n);
    ok(ret == S_FALSE && n == 0, "IEnumCodePage_Next: expected S_FALSE/0, got %08x/%u\n", ret, n);

    n = total * 2;
    TRACE_2("Call IEnumCodePage_Next\n");
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
	CPINFOEXA cpinfoex;
	CHARSETINFO csi;
	MIMECSETINFO mcsi;
	static const WCHAR autoW[] = {'_','a','u','t','o',0};

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

	if (TranslateCharsetInfo((DWORD *)cpinfo[i].uiFamilyCodePage, &csi, TCI_SRCCODEPAGE))
	    ok(cpinfo[i].bGDICharset == csi.ciCharset, "%d != %d\n", cpinfo[i].bGDICharset, csi.ciCharset);
	else
	    trace("TranslateCharsetInfo failed for cp %u\n", cpinfo[i].uiFamilyCodePage);

        if (pGetCPInfoExA)
        {
            if (pGetCPInfoExA(cpinfo[i].uiCodePage, 0, &cpinfoex))
                trace("CodePage %u name: %s\n", cpinfo[i].uiCodePage, cpinfoex.CodePageName);
            else
                trace("GetCPInfoExA failed for cp %u\n", cpinfo[i].uiCodePage);
            if (pGetCPInfoExA(cpinfo[i].uiFamilyCodePage, 0, &cpinfoex))
                trace("CodePage %u name: %s\n", cpinfo[i].uiFamilyCodePage, cpinfoex.CodePageName);
            else
                trace("GetCPInfoExA failed for cp %u\n", cpinfo[i].uiFamilyCodePage);
        }

        /* Win95 does not support UTF-7 */
        if (cpinfo[i].uiCodePage == CP_UTF7) continue;

	/* support files for some codepages might be not installed, or
	 * the codepage is just an alias.
	 */
	if (IsValidCodePage(cpinfo[i].uiCodePage))
	{
	    TRACE_2("Call IMultiLanguage2_IsConvertible\n");
	    ret = IMultiLanguage2_IsConvertible(iML2, cpinfo[i].uiCodePage, CP_UNICODE);
	    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(%u -> CP_UNICODE) = %08x\n", cpinfo[i].uiCodePage, ret);
	    TRACE_2("Call IMultiLanguage2_IsConvertible\n");
	    ret = IMultiLanguage2_IsConvertible(iML2, CP_UNICODE, cpinfo[i].uiCodePage);
	    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(CP_UNICODE -> %u) = %08x\n", cpinfo[i].uiCodePage, ret);

	    TRACE_2("Call IMultiLanguage2_IsConvertible\n");
	    ret = IMultiLanguage2_IsConvertible(iML2, cpinfo[i].uiCodePage, CP_UTF8);
	    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(%u -> CP_UTF8) = %08x\n", cpinfo[i].uiCodePage, ret);
	    TRACE_2("Call IMultiLanguage2_IsConvertible\n");
	    ret = IMultiLanguage2_IsConvertible(iML2, CP_UTF8, cpinfo[i].uiCodePage);
	    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(CP_UTF8 -> %u) = %08x\n", cpinfo[i].uiCodePage, ret);
	}
	else
	    trace("IsValidCodePage failed for cp %u\n", cpinfo[i].uiCodePage);

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
                "wszWebCharset mismatch");
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
                "wszHeaderCharset mismatch");
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
                "wszBodyCharset mismatch");
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

	trace("---\n");
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

static inline void scriptinfo_cmp(SCRIPTINFO *sinfo1, SCRIPTINFO *sinfo2)
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
    TRACE_2("Call IMultiLanguage2_GetNumberOfScripts\n");
    ret = IMultiLanguage2_GetNumberOfScripts(iML2, &total);
    ok(ret == S_OK && total != 0, "IMultiLanguage2_GetNumberOfScripts: expected S_OK/!0, got %08x/%u\n", ret, total);

    trace("total mlang supported scripts %u\n", total);

    TRACE_2("Call IMultiLanguage2_EnumScripts\n");
    ret = IMultiLanguage2_EnumScripts(iML2, flags, LANG_NEUTRAL, &iEnumScript);
    trace("IMultiLanguage2_EnumScripts = %08x, iEnumScript = %p\n", ret, iEnumScript);
    ok(ret == S_OK && iEnumScript, "IMultiLanguage2_EnumScripts: expected S_OK/!NULL, got %08x/%p\n", ret, iEnumScript);

    TRACE_2("Call IEnumScript_Reset\n");
    ret = IEnumScript_Reset(iEnumScript);
    ok(ret == S_OK, "IEnumScript_Reset: expected S_OK, got %08x\n", ret);
    n = 65536;
    TRACE_2("Call IEnumScript_Next\n");
    ret = IEnumScript_Next(iEnumScript, 0, NULL, &n);
    ok(n == 65536 && ret == E_FAIL, "IEnumScript_Next: expected 65536/E_FAIL, got %u/%08x\n", n, ret);
    TRACE_2("Call IEnumScript_Next\n");
    ret = IEnumScript_Next(iEnumScript, 0, NULL, NULL);
    ok(ret == E_FAIL, "IEnumScript_Next: expected E_FAIL, got %08x\n", ret);

    sinfo = HeapAlloc(GetProcessHeap(), 0, sizeof(*sinfo) * total * 2);

    n = total * 2;
    TRACE_2("Call IEnumScript_Next\n");
    ret = IEnumScript_Next(iEnumScript, 0, sinfo, &n);
    ok(ret == S_FALSE && n == 0, "IEnumScript_Next: expected S_FALSE/0, got %08x/%u\n", ret, n);

    n = total * 2;
    TRACE_2("Call IEnumScript_Next\n");
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
	CPINFOEXA cpinfoex;
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
#endif
	if (pGetCPInfoExA(sinfo[i].uiCodePage, 0, &cpinfoex))
	    trace("CodePage %u name: %s\n", sinfo[i].uiCodePage, cpinfoex.CodePageName);
	else
	    trace("GetCPInfoExA failed for cp %u\n", sinfo[i].uiCodePage);

	trace("---\n");
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
    DWORD   dwCodePages = 0;
    DWORD   dwManyCodePages = 0;
    UINT    CodePage = 0;

    ok(IMLangFontLink_CodePageToCodePages(iMLFL, 932, &dwCodePages)==S_OK,
            "IMLangFontLink_CodePageToCodePages failed\n");
    ok (dwCodePages != 0, "No CodePages returned\n");
    ok(IMLangFontLink_CodePagesToCodePage(iMLFL, dwCodePages, 1035,
                &CodePage)==S_OK, 
            "IMLangFontLink_CodePagesToCodePage failed\n");
    ok(CodePage == 932, "Incorrect CodePage Returned (%i)\n",CodePage);

    ok(IMLangFontLink_CodePageToCodePages(iMLFL, 1252, &dwCodePages)==S_OK,
            "IMLangFontLink_CodePageToCodePages failed\n");
    dwManyCodePages = dwManyCodePages | dwCodePages;
    ok(IMLangFontLink_CodePageToCodePages(iMLFL, 1256, &dwCodePages)==S_OK,
            "IMLangFontLink_CodePageToCodePages failed\n");
    dwManyCodePages = dwManyCodePages | dwCodePages;
    ok(IMLangFontLink_CodePageToCodePages(iMLFL, 874, &dwCodePages)==S_OK,
            "IMLangFontLink_CodePageToCodePages failed\n");
    dwManyCodePages = dwManyCodePages | dwCodePages;

    ok(IMLangFontLink_CodePagesToCodePage(iMLFL, dwManyCodePages, 1256,
                &CodePage)==S_OK, 
            "IMLangFontLink_CodePagesToCodePage failed\n");
    ok(CodePage == 1256, "Incorrect CodePage Returned (%i)\n",CodePage);

    ok(IMLangFontLink_CodePagesToCodePage(iMLFL, dwManyCodePages, 936,
                &CodePage)==S_OK, 
            "IMLangFontLink_CodePagesToCodePage failed\n");
    ok(CodePage == 1252, "Incorrect CodePage Returned (%i)\n",CodePage);
}

static void test_rfc1766(IMultiLanguage2 *iML2)
{
    IEnumRfc1766 *pEnumRfc1766;
    RFC1766INFO info;
    ULONG n;
    HRESULT ret;

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
        ok(IsValidLocale(info.lcid, LCID_SUPPORTED), "invalid lcid %04x\n", info.lcid);
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
    ok(ret == E_FAIL, "GetLcidFromRfc1766 returned: %08x\n", ret);

    ret = IMultiLanguage2_GetLcidFromRfc1766(iML2, &lcid, english);
    ok(ret == E_FAIL, "GetLcidFromRfc1766 returned: %08x\n", ret);

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

START_TEST(mlang)
{
    IMultiLanguage2 *iML2 = NULL;
    IMLangFontLink  *iMLFL = NULL;
    HRESULT ret;

    pGetCPInfoExA = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCPInfoExA");

    CoInitialize(NULL);
    TRACE_2("Call CoCreateInstance\n");
    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMultiLanguage2, (void **)&iML2);

    trace("ret = %08x, MultiLanguage2 iML2 = %p\n", ret, iML2);
    if (ret != S_OK || !iML2) return;

    test_rfc1766(iML2);
    test_GetLcidFromRfc1766(iML2);

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

    TRACE_2("Call IMultiLanguage2_IsConvertible\n");
    ret = IMultiLanguage2_IsConvertible(iML2, CP_UTF8, CP_UNICODE);
    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(CP_UTF8 -> CP_UNICODE) = %08x\n", ret);
    TRACE_2("Call IMultiLanguage2_IsConvertible\n");
    ret = IMultiLanguage2_IsConvertible(iML2, CP_UNICODE, CP_UTF8);
    ok(ret == S_OK, "IMultiLanguage2_IsConvertible(CP_UNICODE -> CP_UTF8) = %08x\n", ret);

    test_multibyte_to_unicode_translations(iML2);

    IMultiLanguage2_Release(iML2);

    ret = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IMLangFontLink, (void **)&iMLFL);

    trace("ret = %08x, IMLangFontLink iMLFL = %p\n", ret, iMLFL);
    if (ret != S_OK || !iML2) return;

    IMLangFontLink_Test(iMLFL);
    IMLangFontLink_Release(iMLFL);
    
    CoUninitialize();
}
