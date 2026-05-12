/*
 * MimeInternational tests
 *
 * Copyright 2008 Huw Davies
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
#include "windows.h"
#include "ole2.h"
#include "ocidl.h"

#include "mimeole.h"

#include "initguid.h"
#include "mlang.h"

#include <stdio.h>

#include "wine/test.h"

static void test_create(void)
{
    IMimeInternational *internat, *internat2;
    HRESULT hr;
    ULONG ref;

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = MimeOleGetInternat(&internat2);
    ok(hr == S_OK, "ret %08lx\n", hr);

    /* Under w2k8 it's no longer a singleton */
    if(internat == internat2)
    {
        /* test to show that the object is a singleton with
           a reference held by the dll. */
        ref = IMimeInternational_Release(internat2);
        ok(ref == 2 ||
           ref == 1, /* win95 - object is a static singleton */
           "got %ld\n", ref);

        ref = IMimeInternational_Release(internat);
        ok(ref == 1, "got %ld\n", ref);
    }
    else
    {
        ref = IMimeInternational_Release(internat2);
        ok(ref == 0, "got %ld\n", ref);

        ref = IMimeInternational_Release(internat);
        ok(ref == 0, "got %ld\n", ref);
    }

}

static inline HRESULT get_mlang(IMultiLanguage **ml)
{
    return CoCreateInstance(&CLSID_CMultiLanguage, NULL,  CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                            &IID_IMultiLanguage, (void **)ml);
}

static HRESULT mlang_getcsetinfo(const char *charset, MIMECSETINFO *mlang_info)
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, charset, -1, NULL, 0);
    BSTR bstr = SysAllocStringLen(NULL, len - 1);
    HRESULT hr;
    IMultiLanguage *ml;

    MultiByteToWideChar(CP_ACP, 0, charset, -1, bstr, len);

    hr = get_mlang(&ml);

    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_GetCharsetInfo(ml, bstr, mlang_info);
        IMultiLanguage_Release(ml);
    }
    SysFreeString(bstr);
    if(FAILED(hr)) hr = MIME_E_NOT_FOUND;
    return hr;
}

static HRESULT mlang_getcodepageinfo(UINT cp, MIMECPINFO *mlang_cp_info)
{
    HRESULT hr;
    IMultiLanguage *ml;

    hr = get_mlang(&ml);

    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_GetCodePageInfo(ml, cp, mlang_cp_info);
        IMultiLanguage_Release(ml);
    }
    return hr;
}

static HRESULT mlang_getcsetinfo_from_cp(UINT cp, CHARSETTYPE charset_type, MIMECSETINFO *mlang_info)
{
    MIMECPINFO mlang_cp_info;
    WCHAR *charset_name;
    HRESULT hr;
    IMultiLanguage *ml;

    hr = mlang_getcodepageinfo(cp, &mlang_cp_info);
    if(FAILED(hr)) return hr;

    switch(charset_type)
    {
    case CHARSET_BODY:
        charset_name = mlang_cp_info.wszBodyCharset;
        break;
    case CHARSET_HEADER:
        charset_name = mlang_cp_info.wszHeaderCharset;
        break;
    case CHARSET_WEB:
        charset_name = mlang_cp_info.wszWebCharset;
        break;
    }

    hr = get_mlang(&ml);

    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_GetCharsetInfo(ml, charset_name, mlang_info);
        IMultiLanguage_Release(ml);
    }
    return hr;
}

static void test_charset(void)
{
    IMimeInternational *internat;
    HRESULT hr;
    HCHARSET hcs, hcs_windows_1252, hcs_windows_1251;
    INETCSETINFO cs_info;
    MIMECSETINFO mlang_cs_info;

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeInternational_FindCharset(internat, "nonexistent", &hcs);
    ok(hr == MIME_E_NOT_FOUND, "got %08lx\n", hr);

    hr = IMimeInternational_FindCharset(internat, "windows-1252", &hcs_windows_1252);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = IMimeInternational_FindCharset(internat, "windows-1252", &hcs);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(hcs_windows_1252 == hcs, "got different hcharsets for the same name\n");
    hr = IMimeInternational_FindCharset(internat, "WiNdoWs-1252", &hcs);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(hcs_windows_1252 == hcs, "got different hcharsets for the same name\n");

    hr = IMimeInternational_FindCharset(internat, "windows-1251", &hcs_windows_1251);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(hcs_windows_1252 != hcs_windows_1251, "got the same hcharset for the different names\n");

    hr = IMimeInternational_GetCharsetInfo(internat, hcs_windows_1252, &cs_info);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = mlang_getcsetinfo("windows-1252", &mlang_cs_info);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(cs_info.cpiWindows == mlang_cs_info.uiCodePage, "cpiWindows %ld while mlang uiCodePage %d\n",
       cs_info.cpiWindows, mlang_cs_info.uiCodePage);
    ok(cs_info.cpiInternet == mlang_cs_info.uiInternetEncoding, "cpiInternet %ld while mlang uiInternetEncoding %d\n",
       cs_info.cpiInternet, mlang_cs_info.uiInternetEncoding);
    ok(cs_info.hCharset == hcs_windows_1252, "hCharset doesn't match requested\n");
    ok(!strcmp(cs_info.szName, "windows-1252"), "szName doesn't match requested\n");

    hr = IMimeInternational_GetCodePageCharset(internat, 1252, CHARSET_BODY, &hcs);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = IMimeInternational_GetCharsetInfo(internat, hcs, &cs_info);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = mlang_getcsetinfo_from_cp(1252, CHARSET_BODY, &mlang_cs_info);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(cs_info.cpiWindows == mlang_cs_info.uiCodePage, "cpiWindows %ld while mlang uiCodePage %d\n",
       cs_info.cpiWindows, mlang_cs_info.uiCodePage);
    ok(cs_info.cpiInternet == mlang_cs_info.uiInternetEncoding, "cpiInternet %ld while mlang uiInternetEncoding %d\n",
       cs_info.cpiInternet, mlang_cs_info.uiInternetEncoding);

    IMimeInternational_Release(internat);
}

static void test_defaultcharset(void)
{
    IMimeInternational *internat;
    HRESULT hr;
    HCHARSET hcs_default, hcs, hcs_windows_1251;

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeInternational_GetDefaultCharset(internat, &hcs_default);
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeInternational_GetCodePageCharset(internat, GetACP(), CHARSET_BODY, &hcs);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(hcs_default == hcs, "Unexpected default charset\n");

    hr = IMimeInternational_FindCharset(internat, "windows-1251", &hcs_windows_1251);
    ok(hr == S_OK, "got %08lx\n", hr);
    hr = IMimeInternational_SetDefaultCharset(internat, hcs_windows_1251);
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeInternational_GetDefaultCharset(internat, &hcs);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(hcs == hcs_windows_1251, "didn't retrieve recently set default\n");
    /* Set the old default back again */
    hr = IMimeInternational_SetDefaultCharset(internat, hcs_default);
    ok(hr == S_OK, "ret %08lx\n", hr);

    IMimeInternational_Release(internat);
}

static void test_convert(void)
{
    IMimeInternational *internat;
    HRESULT hr;
    BLOB src, dst;
    ULONG read;
    PROPVARIANT prop_in, prop_out;
    static char test_string[] = "test string";
    static WCHAR test_stringW[] = L"test string";

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08lx\n", hr);

    src.pBlobData = (BYTE*)test_string;
    src.cbSize = sizeof(test_string);
    hr = IMimeInternational_ConvertBuffer(internat, 1252, 28591, &src, &dst, &read);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(read == sizeof(test_string), "got %ld\n", read);
    ok(dst.cbSize == sizeof(test_string), "got %ld\n", dst.cbSize);
    CoTaskMemFree(dst.pBlobData);

    src.cbSize = 2;
    hr = IMimeInternational_ConvertBuffer(internat, 1252, 28591, &src, &dst, &read);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(read == 2, "got %ld\n", read);
    ok(dst.cbSize == 2, "got %ld\n", dst.cbSize);
    CoTaskMemFree(dst.pBlobData);

    prop_in.vt = VT_LPWSTR;
    prop_in.pwszVal = test_stringW;
    hr = IMimeInternational_ConvertString(internat, CP_UNICODE, 1252, &prop_in, &prop_out);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(prop_out.vt == VT_LPSTR, "got %d\n", prop_out.vt);
    ok(!strcmp(prop_out.pszVal, test_string), "got %s\n", prop_out.pszVal);
    PropVariantClear(&prop_out);

    /* If in.vt is VT_LPWSTR, ignore cpiSrc */
    prop_in.vt = VT_LPWSTR;
    prop_in.pwszVal = test_stringW;
    hr = IMimeInternational_ConvertString(internat, 28591, 1252, &prop_in, &prop_out);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(prop_out.vt == VT_LPSTR, "got %d\n", prop_out.vt);
    ok(!strcmp(prop_out.pszVal, test_string), "got %s\n", prop_out.pszVal);
    PropVariantClear(&prop_out);

    prop_in.vt = VT_LPSTR;
    prop_in.pszVal = test_string;
    hr = IMimeInternational_ConvertString(internat, 28591, CP_UNICODE, &prop_in, &prop_out);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(prop_out.vt == VT_LPWSTR, "got %d\n", prop_out.vt);
    ok(!lstrcmpW(prop_out.pwszVal, test_stringW), "mismatched strings\n");
    PropVariantClear(&prop_out);

    /* If in.vt is VT_LPSTR and cpiSrc is CP_UNICODE, use another multibyte codepage (probably GetACP()) */
    prop_in.vt = VT_LPSTR;
    prop_in.pszVal = test_string;
    hr = IMimeInternational_ConvertString(internat, CP_UNICODE, CP_UNICODE, &prop_in, &prop_out);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(prop_out.vt == VT_LPWSTR, "got %d\n", prop_out.vt);
    ok(!lstrcmpW(prop_out.pwszVal, test_stringW), "mismatched strings\n");
    PropVariantClear(&prop_out);

    IMimeInternational_Release(internat);
}

START_TEST(mimeintl)
{
    OleInitialize(NULL);
    test_create();
    test_charset();
    test_defaultcharset();
    test_convert();
    OleUninitialize();
}
