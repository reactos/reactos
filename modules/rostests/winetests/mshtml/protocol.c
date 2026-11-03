/*
 * Copyright 2005 Jacek Caban
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

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"
#include "shlwapi.h"
#include "wininet.h"

#include "initguid.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        expect_ ## func = FALSE; \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func  "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_JSProtocol, 0x3050F3B2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);

DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(ReportProgress);
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportResult);
DEFINE_EXPECT(outer_QI_test);

static HRESULT expect_hrResult;
static BOOL expect_hr_win32err = FALSE;
static DWORD bindf;

static const WCHAR about_blank_url[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
static const WCHAR about_test_url[] = {'a','b','o','u','t',':','t','e','s','t',0};
static const WCHAR about_res_url[] = {'r','e','s',':','b','l','a','n','k',0};
static const WCHAR javascript_test_url[] = {'j','a','v','a','s','c','r','i','p','t',':','t','e','s','t','(',')',0};

static WCHAR res_url_base[INTERNET_MAX_URL_LENGTH] = {'r','e','s',':','/','/'};
static unsigned res_url_base_len;

static HRESULT WINAPI ProtocolSink_QueryInterface(IInternetProtocolSink *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    return 2;
}

static ULONG WINAPI ProtocolSink_Release(IInternetProtocolSink *iface)
{
    return 1;
}

static HRESULT WINAPI ProtocolSink_Switch(IInternetProtocolSink *iface, PROTOCOLDATA *pProtocolData)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolSink_ReportProgress(IInternetProtocolSink *iface, ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    static const WCHAR text_html[] = {'t','e','x','t','/','h','t','m','l',0};

    CHECK_EXPECT(ReportProgress);

    ok(ulStatusCode == BINDSTATUS_MIMETYPEAVAILABLE
            || ulStatusCode == BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE,
            "ulStatusCode=%ld\n", ulStatusCode);
    ok(!lstrcmpW(szStatusText, text_html), "szStatusText != text/html\n");

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF, ULONG ulProgress,
        ULONG ulProgressMax)
{
    CHECK_EXPECT(ReportData);

    ok(ulProgress == ulProgressMax, "ulProgress != ulProgressMax\n");
    ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE),
            "grcf = %08lx\n", grfBSCF);

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult, DWORD dwError,
        LPCWSTR szResult)
{
    CHECK_EXPECT(ReportResult);

    if(expect_hr_win32err) {
        ok((hrResult&0xffff0000) == ((FACILITY_WIN32 << 16)|0x80000000) || hrResult == expect_hrResult,
                "expected win32 err or %08lx got: %08lx\n", expect_hrResult, hrResult);
    }else {
        ok(hrResult == expect_hrResult || (expect_hrResult == E_INVALIDARG && hrResult == MK_E_SYNTAX)
           || (expect_hrResult == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND) &&
               (hrResult == MK_E_SYNTAX || hrResult == HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND))),
           "expected: %08lx got: %08lx\n", expect_hrResult, hrResult);
        expect_hrResult = hrResult;
    }
    ok(dwError == 0, "dwError = %ld\n", dwError);
    ok(!szResult, "szResult != NULL\n");

    return S_OK;
}

static IInternetProtocolSinkVtbl protocol_sink_vtbl = {
    ProtocolSink_QueryInterface,
    ProtocolSink_AddRef,
    ProtocolSink_Release,
    ProtocolSink_Switch,
    ProtocolSink_ReportProgress,
    ProtocolSink_ReportData,
    ProtocolSink_ReportResult
};

static IInternetProtocolSink protocol_sink = {
    &protocol_sink_vtbl
};

static HRESULT WINAPI BindInfo_QueryInterface(IInternetBindInfo *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BindInfo_AddRef(IInternetBindInfo *iface)
{
    return 2;
}

static ULONG WINAPI BindInfo_Release(IInternetBindInfo *iface)
{
    return 1;
}

static HRESULT WINAPI BindInfo_GetBindInfo(IInternetBindInfo *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    CHECK_EXPECT(GetBindInfo);

    ok(grfBINDF != NULL, "grfBINDF == NULL\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %ld\n", pbindinfo->cbSize);

    *grfBINDF = bindf;
    return S_OK;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface, ULONG ulStringType, LPOLESTR *ppwzStr,
        ULONG cEl, ULONG *pcElFetched)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IInternetBindInfoVtbl bind_info_vtbl = {
    BindInfo_QueryInterface,
    BindInfo_AddRef,
    BindInfo_Release,
    BindInfo_GetBindInfo,
    BindInfo_GetBindString
};

static IInternetBindInfo bind_info = {
    &bind_info_vtbl
};

static void test_protocol_fail(IInternetProtocol *protocol, LPCWSTR url, HRESULT expected_hres,
        BOOL expect_win32err)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportResult);

    expect_hrResult = expected_hres;
    expect_hr_win32err = expect_win32err;
    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    if(expect_win32err)
        ok((hres&0xffff0000) == ((FACILITY_WIN32 << 16)|0x80000000) || hres == expect_hrResult,
                "expected win32 err or %08lx got: %08lx\n", expected_hres, hres);
    else
        ok(hres == expect_hrResult, "expected: %08lx got: %08lx\n", expect_hrResult, hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportResult);
}

static void protocol_start(IInternetProtocol *protocol, const WCHAR *url)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportResult);
    SET_EXPECT(ReportProgress);
    SET_EXPECT(ReportData);
    expect_hrResult = S_OK;
    expect_hr_win32err = FALSE;

    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08lx\n", hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportProgress);
    CHECK_CALLED(ReportData);
    CHECK_CALLED(ReportResult);
}

static void test_res_url_fail(const WCHAR *url_suffix)
{
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    IInternetProtocol *protocol;
    HRESULT hres;

    wcscpy(url, res_url_base);
    wcscat(url, url_suffix);

    hres = CoCreateInstance(&CLSID_ResProtocol, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not create ResProtocol instance: %08lx\n", hres);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportResult);
    expect_hr_win32err = TRUE;
    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(HRESULT_FACILITY(hres) == FACILITY_WIN32,
       "%s: expected win32 error, got: %08lx\n", debugstr_w(url_suffix), hres);
    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportResult);

    IInternetProtocol_Release(protocol);
}

static void test_res_url(const char *url_suffix)
{
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    IInternetProtocol *protocol;
    ULONG size, ref;
    BYTE buf[100];
    HRESULT hres;

    memcpy(url, res_url_base, res_url_base_len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, url_suffix, -1, url+res_url_base_len, ARRAY_SIZE(url)-res_url_base_len);

    hres = CoCreateInstance(&CLSID_ResProtocol, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not create ResProtocol instance: %08lx\n", hres);

    protocol_start(protocol, url);

    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &size);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);

    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08lx\n", hres);


    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "ref=%lu\n", ref);
}

static void res_sec_url_cmp(LPCWSTR url, DWORD size, LPCWSTR file)
{
    WCHAR buf[MAX_PATH];
    DWORD len;

    static const WCHAR fileW[] = {'f','i','l','e',':','/','/'};

    if(size < ARRAY_SIZE(fileW) || memcmp(url, fileW, sizeof(fileW))) {
        ok(0, "wrong URL protocol\n");
        return;
    }

    SetLastError(0xdeadbeef);
    len = SearchPathW(NULL, file, NULL, ARRAY_SIZE(buf), buf, NULL);
    if(!len) {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
            win_skip("SearchPathW is not implemented\n");
        else
            ok(0, "SearchPath failed: %lu\n", GetLastError());
        return;
    }

    len += ARRAY_SIZE(fileW)+1;
    ok(len == size, "wrong size %lu, expected %lu\n", size, len);
    ok(!lstrcmpW(url + ARRAY_SIZE(fileW), buf), "wrong file part %s\n", wine_dbgstr_w(url));
}

static void test_res_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IUnknown *unk;
    IClassFactory *factory;
    HRESULT hres;

    static const WCHAR blank_url[] =
        {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l','/','b','l','a','n','k','.','h','t','m',0};
    static const WCHAR test_part_url[] = {'r','e','s',':','/','/','C','S','S','/','t','e','s','t',0};
    static const WCHAR wrong_url1[] =
        {'m','s','h','t','m','l','.','d','l','l','/','b','l','a','n','k','.','m','t','h',0};
    static const WCHAR wrong_url2[] =
        {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l',0};
    static const WCHAR wrong_url3[] =
        {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l','/','x','x','.','h','t','m',0};
    static const WCHAR wrong_url4[] =
        {'r','e','s',':','/','/','x','x','.','d','l','l','/','b','l','a','n','k','.','h','t','m',0};
    static const WCHAR wrong_url5[] =
        {'r','e','s',':','/','/','s','h','t','m','l','.','d','l','l','/','b','l','a','n','k','.','h','t','m',0};
    static const WCHAR wrong_url6[] =
        {'r','e','s',':','/','/','c',':','\\','d','i','r','\\','f','i','l','e','.','d','l','l','/','b','l','a','n','k','.','h','t','m',0};
    static const WCHAR mshtml_dllW[] = {'m','s','h','t','m','l','.','d','l','l',0};

    hres = CoGetClassObject(&CLSID_ResProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == S_OK, "Could not get IInternetProtocolInfo interface: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        WCHAR buf[128];
        DWORD size, expected_size;
        int i;

        for(i = PARSE_CANONICALIZE; i <= PARSE_UNESCAPE; i++) {
            if(i != PARSE_SECURITY_URL && i != PARSE_DOMAIN) {
                hres = IInternetProtocolInfo_ParseUrl(protocol_info, blank_url, i, 0, buf,
                        ARRAY_SIZE(buf), &size, 0);
                ok(hres == INET_E_DEFAULT_ACTION,
                        "[%d] failed: %08lx, expected INET_E_DEFAULT_ACTION\n", i, hres);
            }
        }

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, blank_url, PARSE_SECURITY_URL, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK, "ParseUrl failed: %08lx\n", hres);
        res_sec_url_cmp(buf, size, mshtml_dllW);
        ok(size == lstrlenW(buf)+1, "size = %ld\n", size);
        expected_size = size;

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, blank_url, PARSE_SECURITY_URL, 0, buf,
                expected_size, &size, 0);
        ok(hres == S_OK, "ParseUrl failed: %08lx\n", hres);
        res_sec_url_cmp(buf, size, mshtml_dllW);
        ok(size == expected_size, "size = %ld\n", size);

        size = 0;
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, blank_url, PARSE_SECURITY_URL, 0, buf,
                3, &size, 0);
        ok(hres == S_FALSE, "ParseUrl failed: %08lx, expected S_FALSE\n", hres);
        ok(size == expected_size, "size = %ld\n", size);

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, wrong_url1, PARSE_SECURITY_URL, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == MK_E_SYNTAX || hres == E_INVALIDARG,
           "ParseUrl failed: %08lx, expected MK_E_SYNTAX\n", hres);

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, wrong_url5, PARSE_SECURITY_URL, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == MK_E_SYNTAX, "ParseUrl failed: %08lx, expected MK_E_SYNTAX\n", hres);

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, wrong_url6, PARSE_SECURITY_URL, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == MK_E_SYNTAX, "ParseUrl failed: %08lx, expected MK_E_SYNTAX\n", hres);

        size = 0xdeadbeef;
        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, blank_url, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK || hres == E_FAIL, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?', "buf changed\n");
        ok(size == ARRAY_SIZE(blank_url) || size == ARRAY_SIZE(buf), /* IE8 */ "size=%ld\n", size);

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, wrong_url1, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK || hres == E_FAIL, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?', "buf changed\n");
        ok(size == ARRAY_SIZE(wrong_url1) || size == ARRAY_SIZE(buf), /* IE8 */ "size=%ld\n", size);

        if (0)
        {
        /* Crashes on windows */
        size = 0xdeadbeef;
        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, NULL, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == E_FAIL, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?', "buf changed\n");
        ok(size == 1, "size=%lu, expected 1\n", size);

        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, blank_url, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), NULL, 0);
        ok(hres == E_POINTER, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?', "buf changed\n");

        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, NULL, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), NULL, 0);
        ok(hres == E_POINTER, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?', "buf changed\n");
        }

        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, blank_url, PARSE_UNESCAPE+1, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_DEFAULT_ACTION,
                "ParseUrl failed: %08lx, expected INET_E_DEFAULT_ACTION\n", hres);
        ok(buf[0] == '?', "buf changed\n");

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, blank_url, test_part_url,
                0, buf, ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER, "CombineUrl failed: %08lx\n", hres);
        ok(size == 0xdeadbeef, "size=%ld\n", size);

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, blank_url, test_part_url,
                URL_FILE_USE_PATHURL, buf, ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER, "CombineUrl failed: %08lx\n", hres);
        ok(size == 0xdeadbeef, "size=%ld\n", size);

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, NULL, NULL,
                URL_FILE_USE_PATHURL, NULL, 0xdeadbeef, NULL, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER, "CombineUrl failed: %08lx\n", hres);
        ok(size == 0xdeadbeef, "size=%ld\n", size);

        hres = IInternetProtocolInfo_CompareUrl(protocol_info, blank_url, blank_url, 0);
        ok(hres == E_NOTIMPL, "CompareUrl failed: %08lx\n", hres);

        hres = IInternetProtocolInfo_CompareUrl(protocol_info, NULL, NULL, 0xdeadbeef);
        ok(hres == E_NOTIMPL, "CompareUrl failed: %08lx\n", hres);

        for(i=0; i<30; i++) {
            if(i == QUERY_USES_NETWORK || i == QUERY_IS_SECURE || i == QUERY_IS_SAFE)
                continue;

            hres = IInternetProtocolInfo_QueryInfo(protocol_info, blank_url, i, 0,
                                                   buf, sizeof(buf), &size, 0);
            ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
               "QueryInfo(%d) returned: %08lx, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", i, hres);
        }

        size = 0xdeadbeef;
        memset(buf, '?', sizeof(buf));
        hres = IInternetProtocolInfo_QueryInfo(protocol_info, blank_url, QUERY_USES_NETWORK, 0,
                                               buf, sizeof(buf), &size, 0);
        ok(hres == S_OK, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx\n", hres);
        ok(size == sizeof(DWORD), "size=%ld\n", size);
        ok(!*(DWORD*)buf, "buf=%ld\n", *(DWORD*)buf);

        memset(buf, '?', sizeof(buf));
        hres = IInternetProtocolInfo_QueryInfo(protocol_info, blank_url, QUERY_USES_NETWORK, 0,
                                               buf, sizeof(buf), NULL, 0);
        ok(hres == S_OK, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx\n", hres);
        ok(!*(DWORD*)buf, "buf=%ld\n", *(DWORD*)buf);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, blank_url, QUERY_USES_NETWORK, 0,
                                               buf, 3, &size, 0);
        ok(hres == E_FAIL, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx, expected E_FAIL\n", hres);

        size = 0xdeadbeef;
        memset(buf, '?', sizeof(buf));
        hres = IInternetProtocolInfo_QueryInfo(protocol_info, NULL, QUERY_USES_NETWORK, 0,
                                               buf, sizeof(buf), &size, 0);
        ok(hres == S_OK, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx\n", hres);
        ok(size == sizeof(DWORD), "size=%ld\n", size);
        ok(!*(DWORD*)buf, "buf=%ld\n", *(DWORD*)buf);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, blank_url, QUERY_USES_NETWORK, 0,
                                               NULL, sizeof(buf), &size, 0);
        ok(hres == E_FAIL, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx, expected E_FAIL\n", hres);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, blank_url, 60, 0,
                                               NULL, sizeof(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
           "QueryInfo failed: %08lx, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", hres);

        IInternetProtocolInfo_Release(protocol_info);
    }

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        IInternetProtocol *protocol;
        BYTE buf[512];
        ULONG cb;
        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

        if(SUCCEEDED(hres)) {
            IInternetPriority *priority;

            hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority, (void**)&priority);
            ok(hres == E_NOINTERFACE,
               "QueryInterface(IInternetPriority) returned %08lx, expected E_NOINTEFACE\n", hres);

            test_protocol_fail(protocol, wrong_url1, E_INVALIDARG, FALSE);
            test_protocol_fail(protocol, wrong_url2,
                               HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), FALSE);
            test_protocol_fail(protocol, wrong_url3, E_FAIL, TRUE);
            test_protocol_fail(protocol, wrong_url4, E_FAIL, TRUE);

            cb = 0xdeadbeef;
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == E_FAIL, "Read returned %08lx expected E_FAIL\n", hres);
            ok(cb == 0xdeadbeef, "cb=%lu expected 0xdeadbeef\n", cb);
    
            protocol_start(protocol, blank_url);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == 2, "cb=%lu expected 2\n", cb);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_FALSE, "Read failed: %08lx expected S_FALSE\n", hres);
            ok(cb == 0, "cb=%lu expected 0\n", cb);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

            protocol_start(protocol, blank_url);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);

            protocol_start(protocol, blank_url);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n\n", hres);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == 2, "cb=%lu expected 2\n", cb);

            protocol_start(protocol, blank_url);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            protocol_start(protocol, blank_url);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);

            IInternetProtocol_Release(protocol);
        }

        IClassFactory_Release(factory);
    }

    IUnknown_Release(unk);

    test_res_url("/blank.html");
    test_res_url("/123");
    test_res_url("/#23/blank.html");
    test_res_url("/#23/123");
    test_res_url("/23/blank.html");
    test_res_url("/23/123");

    test_res_url("/jstest.html");
    test_res_url("/jstest-rtfile.html");
    test_res_url("/#2110/jstest-rtfile.html");
    test_res_url("/2110/jstest-rtfile.html");

    test_res_url_fail(L"/doesntexist/jstest-rtfile.html");
    test_res_url_fail(L"/2/jstest-rtfile.html");
    test_res_url_fail(L"/23/jstest-rtfile.html"); /* no fallback from explicit RT_HTML */

    test_res_url("/Test/res.html");
    test_res_url("/test/dir/dir2/res.html");

    if(GetProcAddress(GetModuleHandleA("urlmon.dll"), "CreateUri")) {
        test_res_url("/test/dir/dir2/res.html?query_part");
        test_res_url("/test/dir/dir2/res.html#hash_part");
        test_res_url("/#123");
        test_res_url("/#23/#123");
        test_res_url("/#123#456");
    }else {
        win_skip("IUri not supported\n");
    }
}

static void do_test_about_protocol(IClassFactory *factory, DWORD bf)
{
    IInternetProtocol *protocol;
    IInternetPriority *priority;
    BYTE buf[512];
    ULONG cb;
    HRESULT hres;

    static const WCHAR blank_html[] = {0xfeff,'<','H','T','M','L','>','<','/','H','T','M','L','>',0};
    static const WCHAR test_html[] =
        {0xfeff,'<','H','T','M','L','>','t','e','s','t','<','/','H','T','M','L','>',0};

    bindf = bf;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority, (void**)&priority);
    ok(hres == E_NOINTERFACE,
       "QueryInterface(IInternetPriority) returned %08lx, expected E_NOINTEFACE\n", hres);

    protocol_start(protocol, about_blank_url);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == sizeof(blank_html), "cb=%ld\n", cb);
    ok(!memcmp(buf, blank_html, cb), "Read wrong data\n");
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

    protocol_start(protocol, about_test_url);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == sizeof(test_html), "cb=%ld\n", cb);
    ok(!memcmp(buf, test_html, cb), "Read wrong data\n");
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

    protocol_start(protocol, about_res_url);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == sizeof(blank_html), "cb=%ld\n", cb);
    ok(!memcmp(buf, blank_html, cb), "Read wrong data\n");
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

    IInternetProtocol_Release(protocol);
}

static void test_about_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IUnknown *unk;
    IClassFactory *factory;
    HRESULT hres;

    hres = CoGetClassObject(&CLSID_AboutProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == S_OK, "Could not get IInternetProtocolInfo interface: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        WCHAR buf[128];
        DWORD size;
        int i;

        for(i = PARSE_CANONICALIZE; i <= PARSE_UNESCAPE; i++) {
            if(i != PARSE_SECURITY_URL && i != PARSE_DOMAIN) {
                hres = IInternetProtocolInfo_ParseUrl(protocol_info, about_blank_url, i, 0, buf,
                        ARRAY_SIZE(buf), &size, 0);
                ok(hres == INET_E_DEFAULT_ACTION,
                        "[%d] failed: %08lx, expected INET_E_DEFAULT_ACTION\n", i, hres);
            }
        }

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, about_blank_url, PARSE_SECURITY_URL, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK, "ParseUrl failed: %08lx\n", hres);
        ok(!lstrcmpW(about_blank_url, buf), "buf != blank_url\n");

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, about_blank_url, PARSE_SECURITY_URL, 0, buf,
                3, &size, 0);
        ok(hres == S_FALSE, "ParseUrl failed: %08lx, expected S_FALSE\n", hres);
        ok(size == 12, "size = %ld\n", size);

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, about_test_url, PARSE_SECURITY_URL, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK, "ParseUrl failed: %08lx\n", hres);
        ok(!lstrcmpW(about_test_url, buf), "buf != test_url\n");
        ok(size == 11, "size = %ld\n", size);

        size = 0xdeadbeef;
        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, about_blank_url, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == S_OK || hres == E_FAIL, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?' || buf[0] == '\0' /* Win10 */,
           "Expected buf to be unchanged or empty, got %s\n",
           wine_dbgstr_w(buf));
        ok(size == ARRAY_SIZE(about_blank_url) || size == ARRAY_SIZE(buf), /* IE8 */
           "size=%ld\n", size);

        if (0)
        {
        /* Crashes on windows */
        size = 0xdeadbeef;
        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, NULL, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == E_FAIL, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?', "buf changed\n");
        ok(size == 1, "size=%lu, expected 1\n", size);

        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, about_blank_url, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), NULL, 0);
        ok(hres == E_POINTER, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?', "buf changed\n");

        buf[0] = '?';
        hres = IInternetProtocolInfo_ParseUrl(protocol_info, NULL, PARSE_DOMAIN, 0, buf,
                ARRAY_SIZE(buf), NULL, 0);
        ok(hres == E_POINTER, "ParseUrl failed: %08lx\n", hres);
        ok(buf[0] == '?', "buf changed\n");
        }

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, about_blank_url, PARSE_UNESCAPE+1, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_DEFAULT_ACTION,
                "ParseUrl failed: %08lx, expected INET_E_DEFAULT_ACTION\n", hres);

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, about_blank_url, about_test_url,
                0, buf, ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER, "CombineUrl failed: %08lx\n", hres);
        ok(size == 0xdeadbeef, "size=%ld\n", size);

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, about_blank_url, about_test_url,
                URL_FILE_USE_PATHURL, buf, ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER, "CombineUrl failed: %08lx\n", hres);
        ok(size == 0xdeadbeef, "size=%ld\n", size);

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, NULL, NULL,
                URL_FILE_USE_PATHURL, buf, ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER, "CombineUrl failed: %08lx\n", hres);
        ok(size == 0xdeadbeef, "size=%ld\n", size);

        hres = IInternetProtocolInfo_CompareUrl(protocol_info, about_blank_url, about_blank_url, 0);
        ok(hres == E_NOTIMPL, "CompareUrl failed: %08lx\n", hres);

        hres = IInternetProtocolInfo_CompareUrl(protocol_info, NULL, NULL, 0xdeadbeef);
        ok(hres == E_NOTIMPL, "CompareUrl failed: %08lx\n", hres);

        for(i=0; i<30; i++) {
            switch(i) {
            case QUERY_CAN_NAVIGATE:
            case QUERY_USES_NETWORK:
            case QUERY_IS_CACHED:
            case QUERY_IS_INSTALLEDENTRY:
            case QUERY_IS_CACHED_OR_MAPPED:
            case QUERY_IS_SECURE:
            case QUERY_IS_SAFE:
            case QUERY_USES_HISTORYFOLDER:
            case QUERY_IS_CACHED_AND_USABLE_OFFLINE:
                break;
            default:
                hres = IInternetProtocolInfo_QueryInfo(protocol_info, about_blank_url, i, 0,
                                                       buf, sizeof(buf), &size, 0);
                ok(hres == E_FAIL, "QueryInfo(%d) returned: %08lx, expected E_FAIL\n", i, hres);
            }
        }

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, about_blank_url, QUERY_CAN_NAVIGATE, 0,
                                               buf, sizeof(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER ||
           hres == E_FAIL, /* win2k */
           "QueryInfo returned: %08lx, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER or E_FAIL\n", hres);

        size = 0xdeadbeef;
        memset(buf, '?', sizeof(buf));
        hres = IInternetProtocolInfo_QueryInfo(protocol_info, about_blank_url, QUERY_USES_NETWORK, 0,
                                               buf, sizeof(buf), &size, 0);
        ok(hres == S_OK, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx\n", hres);
        ok(size == sizeof(DWORD), "size=%ld\n", size);
        ok(!*(DWORD*)buf, "buf=%ld\n", *(DWORD*)buf);

        memset(buf, '?', sizeof(buf));
        hres = IInternetProtocolInfo_QueryInfo(protocol_info, about_blank_url, QUERY_USES_NETWORK, 0,
                                               buf, sizeof(buf), NULL, 0);
        ok(hres == S_OK, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx\n", hres);
        ok(!*(DWORD*)buf, "buf=%ld\n", *(DWORD*)buf);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, about_blank_url, QUERY_USES_NETWORK, 0,
                                               buf, 3, &size, 0);
        ok(hres == E_FAIL, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx, expected E_FAIL\n", hres);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, about_blank_url, QUERY_USES_NETWORK, 0,
                                               NULL, sizeof(buf), &size, 0);
        ok(hres == E_FAIL, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx, expected E_FAIL\n", hres);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, about_blank_url, 60, 0,
                                               NULL, sizeof(buf), &size, 0);
        ok(hres == E_FAIL, "QueryInfo failed: %08lx, expected E_FAIL\n", hres);

        IInternetProtocolInfo_Release(protocol_info);
    }

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        do_test_about_protocol(factory, 0);
        do_test_about_protocol(factory,
                BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NEEDFILE);

        IClassFactory_Release(factory);
    }

    IUnknown_Release(unk);
}

static void test_javascript_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IUnknown *unk;
    IClassFactory *factory;
    HRESULT hres;

    hres = CoGetClassObject(&CLSID_JSProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == S_OK, "Could not get IInternetProtocolInfo interface: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        WCHAR buf[128];
        DWORD size;
        int i;

        for(i = PARSE_CANONICALIZE; i <= PARSE_UNESCAPE; i++) {
            if(i != PARSE_SECURITY_URL && i != PARSE_DOMAIN) {
                hres = IInternetProtocolInfo_ParseUrl(protocol_info, javascript_test_url, i, 0, buf,
                        ARRAY_SIZE(buf), &size, 0);
                ok(hres == INET_E_DEFAULT_ACTION,
                        "[%d] failed: %08lx, expected INET_E_DEFAULT_ACTION\n", i, hres);
            }
        }

        hres = IInternetProtocolInfo_ParseUrl(protocol_info, javascript_test_url, PARSE_UNESCAPE+1, 0, buf,
                ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_DEFAULT_ACTION,
                "ParseUrl failed: %08lx, expected INET_E_DEFAULT_ACTION\n", hres);

        size = 0xdeadbeef;
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, javascript_test_url, javascript_test_url,
                0, buf, ARRAY_SIZE(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER, "CombineUrl failed: %08lx\n", hres);
        ok(size == 0xdeadbeef, "size=%ld\n", size);

        hres = IInternetProtocolInfo_CompareUrl(protocol_info, javascript_test_url, javascript_test_url, 0);
        ok(hres == E_NOTIMPL, "CompareUrl failed: %08lx\n", hres);

        for(i=0; i<30; i++) {
            switch(i) {
            case QUERY_USES_NETWORK:
            case QUERY_IS_SECURE:
                break;
            default:
                hres = IInternetProtocolInfo_QueryInfo(protocol_info, javascript_test_url, i, 0,
                                                       buf, sizeof(buf), &size, 0);
                ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
                   "QueryInfo(%d) returned: %08lx, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", i, hres);
            }
        }


        memset(buf, '?', sizeof(buf));
        hres = IInternetProtocolInfo_QueryInfo(protocol_info, javascript_test_url, QUERY_USES_NETWORK, 0,
                                               buf, sizeof(buf), &size, 0);
        ok(hres == S_OK, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx\n", hres);
        ok(size == sizeof(DWORD), "size=%ld\n", size);
        ok(!*(DWORD*)buf, "buf=%ld\n", *(DWORD*)buf);

        memset(buf, '?', sizeof(buf));
        hres = IInternetProtocolInfo_QueryInfo(protocol_info, javascript_test_url, QUERY_USES_NETWORK, 0,
                                               buf, sizeof(buf), NULL, 0);
        ok(hres == S_OK, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx\n", hres);
        ok(!*(DWORD*)buf, "buf=%ld\n", *(DWORD*)buf);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, javascript_test_url, QUERY_USES_NETWORK, 0,
                                               buf, 3, &size, 0);
        ok(hres == E_FAIL, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx, expected E_FAIL\n", hres);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, javascript_test_url, QUERY_USES_NETWORK, 0,
                                               NULL, sizeof(buf), &size, 0);
        ok(hres == E_FAIL, "QueryInfo(QUERY_USES_NETWORK) failed: %08lx, expected E_FAIL\n", hres);

        hres = IInternetProtocolInfo_QueryInfo(protocol_info, javascript_test_url, 60, 0,
                                               NULL, sizeof(buf), &size, 0);
        ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
           "QueryInfo failed: %08lx, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", hres);

        /* FIXME: test QUERY_IS_SECURE */

        IInternetProtocolInfo_Release(protocol_info);
    }

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres))
        IClassFactory_Release(factory);

    IUnknown_Release(unk);
}

static const IID outer_test_iid = {0xabcabc00,0,0,{0,0,0,0,0,0,0,0x66}};

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &outer_test_iid)) {
        CHECK_EXPECT(outer_QI_test);
        *ppv = (IUnknown*)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl outer_vtbl = {
    outer_QueryInterface,
    outer_AddRef,
    outer_Release
};

static void test_com_aggregation(const CLSID *clsid)
{
    IUnknown outer = { &outer_vtbl };
    IClassFactory *class_factory;
    IUnknown *unk, *unk2, *unk3;
    HRESULT hres;

    hres = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&class_factory);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);

    hres = IClassFactory_CreateInstance(class_factory, &outer, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CreateInstance returned: %08lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocol, (void**)&unk2);
    ok(hres == S_OK, "Could not get IDispatch iface: %08lx\n", hres);

    SET_EXPECT(outer_QI_test);
    hres = IUnknown_QueryInterface(unk2, &outer_test_iid, (void**)&unk3);
    CHECK_CALLED(outer_QI_test);
    ok(hres == S_OK, "Could not get IInternetProtocol iface: %08lx\n", hres);
    ok(unk3 == (IUnknown*)0xdeadbeef, "unexpected unk2\n");

    IUnknown_Release(unk2);
    IUnknown_Release(unk);

    unk = (void*)0xdeadbeef;
    hres = IClassFactory_CreateInstance(class_factory, &outer, &IID_IInternetProtocol, (void**)&unk);
    ok(hres == E_INVALIDARG, "CreateInstance returned: %08lx\n", hres);
    ok(!unk, "unk = %p\n", unk);

    IClassFactory_Release(class_factory);
}

START_TEST(protocol)
{
    res_url_base_len = 6 + GetModuleFileNameW(NULL, res_url_base + 6 /* strlen("res://") */,
                                              ARRAY_SIZE(res_url_base)-6);

    OleInitialize(NULL);

    test_res_protocol();
    test_about_protocol();
    test_javascript_protocol();

    test_com_aggregation(&CLSID_AboutProtocol);
    test_com_aggregation(&CLSID_ResProtocol);

    OleUninitialize();
}
