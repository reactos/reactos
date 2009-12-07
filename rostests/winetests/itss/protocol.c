/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "ole2.h"
#include "urlmon.h"
#include "shlwapi.h"

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

#define SET_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        SET_CALLED(func); \
    }while(0)

DEFINE_GUID(CLSID_ITSProtocol,0x9d148291,0xb9c8,0x11d0,0xa4,0xcc,0x00,0x00,0xf8,0x01,0x49,0xf6);

DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(ReportProgress_BEGINDOWNLOADDATA);
DEFINE_EXPECT(ReportProgress_SENDINGREQUEST);
DEFINE_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
DEFINE_EXPECT(ReportProgress_DIRECTBIND);
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportResult);

static HRESULT expect_hrResult;
static IInternetProtocol *read_protocol = NULL;
static DWORD bindf;

static const WCHAR blank_url1[] = {'i','t','s',':',
    't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR blank_url2[] = {'m','S','-','i','T','s',':',
    't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR blank_url3[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
    't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR blank_url4[] = {'i','t','s',':',
    't','e','s','t','.','c','h','m',':',':','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR blank_url5[] = {'i','t','s',':',
    't','e','s','t','.','c','h','m',':',':','\\','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR blank_url6[] = {'i','t','s',':',
    't','e','s','t','.','c','h','m',':',':','/','%','6','2','l','a','n','k','.','h','t','m','l',0};
static const WCHAR blank_url7[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
    't','e','s','t','.','c','h','m',':',':','\\','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR blank_url8[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
    't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l','/',0};

static enum {
    ITS_PROTOCOL,
    MK_PROTOCOL
} test_protocol;

static const WCHAR cache_file1[] =
    {'t','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR cache_file2[] =
    {'t','e','s','t','.','c','h','m',':',':','\\','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR cache_file3[] =
    {'t','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l','/',0};
static const WCHAR *cache_file = cache_file1;

static HRESULT WINAPI ProtocolSink_QueryInterface(IInternetProtocolSink *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        *ppv = iface;
        return S_OK;
    }
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
    static const WCHAR blank_html[] = {'b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR text_html[] = {'t','e','x','t','/','h','t','m','l',0};

    switch(ulStatusCode) {
    case BINDSTATUS_BEGINDOWNLOADDATA:
        CHECK_EXPECT(ReportProgress_BEGINDOWNLOADDATA);
        ok(!szStatusText, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_SENDINGREQUEST:
        CHECK_EXPECT(ReportProgress_SENDINGREQUEST);
        if(test_protocol == ITS_PROTOCOL)
            ok(!lstrcmpW(szStatusText, blank_html), "unexpected szStatusText\n");
        else
            ok(szStatusText == NULL, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        ok(!lstrcmpW(szStatusText, text_html), "unexpected szStatusText\n");
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        CHECK_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
        ok(!lstrcmpW(szStatusText, cache_file), "unexpected szStatusText\n");
        break;
    case BINDSTATUS_DIRECTBIND:
        CHECK_EXPECT(ReportProgress_DIRECTBIND);
        ok(!szStatusText, "szStatusText != NULL\n");
        break;
    default:
        ok(0, "unexpected ulStatusCode %d\n", ulStatusCode);
        break;
    }

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF, ULONG ulProgress,
        ULONG ulProgressMax)
{
    CHECK_EXPECT(ReportData);

    ok(ulProgress == ulProgressMax, "ulProgress != ulProgressMax\n");
    if(test_protocol == ITS_PROTOCOL)
        ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE), "grcf = %08x\n", grfBSCF);
    else
        ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION), "grcf = %08x\n", grfBSCF);

    if(read_protocol) {
        BYTE buf[100];
        DWORD cb = 0xdeadbeef;
        HRESULT hres;

        hres = IInternetProtocol_Read(read_protocol, buf, sizeof(buf), &cb);
        ok(hres == S_OK, "Read failed: %08x\n", hres);
        ok(cb == 13, "cb=%u expected 13\n", cb);
        ok(!memcmp(buf, "<html></html>", 13), "unexpected data\n");
    }

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult,
        DWORD dwError, LPCWSTR szResult)
{
    CHECK_EXPECT(ReportResult);

    ok(hrResult == expect_hrResult, "expected: %08x got: %08x\n", expect_hrResult, hrResult);
    ok(dwError == 0, "dwError = %d\n", dwError);
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
    if(grfBINDF)
        ok(!*grfBINDF, "*grfBINDF != 0\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %d\n", pbindinfo->cbSize);

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

static void test_protocol_fail(IInternetProtocol *protocol, LPCWSTR url, HRESULT expected_hres)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportResult);

    expect_hrResult = expected_hres;
    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == expected_hres, "expected: %08x got: %08x\n", expected_hres, hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportResult);
}

#define protocol_start(p,u,e) _protocol_start(__LINE__,p,u,e)
static HRESULT _protocol_start(unsigned line, IInternetProtocol *protocol, LPCWSTR url, BOOL expect_mime)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    if(test_protocol == MK_PROTOCOL)
        SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    if(expect_mime)
        SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    if(test_protocol == MK_PROTOCOL)
        SET_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
    SET_EXPECT(ReportData);
    if(test_protocol == ITS_PROTOCOL)
        SET_EXPECT(ReportProgress_BEGINDOWNLOADDATA);
    SET_EXPECT(ReportResult);
    expect_hrResult = S_OK;

    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);

    if(FAILED(hres)) {
        SET_CALLED(GetBindInfo);
        if(test_protocol == MK_PROTOCOL)
            SET_CALLED(ReportProgress_DIRECTBIND);
        SET_CALLED(ReportProgress_SENDINGREQUEST);
        if(expect_mime)
            SET_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        if(test_protocol == MK_PROTOCOL)
            SET_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
        SET_CALLED(ReportData);
        if(test_protocol == ITS_PROTOCOL)
            SET_CALLED(ReportProgress_BEGINDOWNLOADDATA);
        SET_CALLED(ReportResult);
    }else {
        CHECK_CALLED(GetBindInfo);
        if(test_protocol == MK_PROTOCOL)
            SET_CALLED(ReportProgress_DIRECTBIND);
        CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        if(expect_mime)
            CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        if(test_protocol == MK_PROTOCOL)
            SET_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
        CHECK_CALLED(ReportData);
        if(test_protocol == ITS_PROTOCOL)
            CHECK_CALLED(ReportProgress_BEGINDOWNLOADDATA);
        CHECK_CALLED(ReportResult);
    }

    return hres;
}

static void test_protocol_url(IClassFactory *factory, LPCWSTR url, BOOL expect_mime)
{
    IInternetProtocol *protocol;
    BYTE buf[512];
    ULONG cb, ref;
    HRESULT hres;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = protocol_start(protocol, url, expect_mime);
    if(FAILED(hres)) {
        IInternetProtocol_Release(protocol);
        return;
    }

    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 13, "cb=%u expected 13\n", cb);
    ok(!memcmp(buf, "<html></html>", 13), "unexpected data\n");
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%d\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    cb = 0xdeadbeef;
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == (test_protocol == ITS_PROTOCOL ? INET_E_DATA_NOT_AVAILABLE : E_FAIL),
       "Read returned %08x\n", hres);
    ok(cb == 0xdeadbeef, "cb=%u expected 0xdeadbeef\n", cb);

    protocol_start(protocol, url, expect_mime);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 2, "cb=%u expected 2\n", cb);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 11, "cb=%u, expected 11\n", cb);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_FALSE, "Read failed: %08x expected S_FALSE\n", hres);
    ok(cb == 0, "cb=%u expected 0\n", cb);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%d\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(protocol, url, expect_mime);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08x\n", hres);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 11, "cb=%u, expected 11\n", cb);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%d\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(protocol, url, expect_mime);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08x\n", hres);
    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08x\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 2, "cb=%u, expected 2\n", cb);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 2, "cb=%u, expected 2\n", cb);
    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08x\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 2, "cb=%u expected 2\n", cb);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%d\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&read_protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(read_protocol, url, expect_mime);
    ref = IInternetProtocol_Release(read_protocol);
    ok(!ref, "protocol ref=%d\n", ref);
    read_protocol = NULL;
}

static const WCHAR rel_url1[] =
    {'t','e','s','t','.','h','t','m','l',0};
static const WCHAR rel_url2[] =
    {'t','e','s','t','.','c','h','m',':',':','/','t','e','s','t','.','h','t','m','l',0};
static const WCHAR rel_url3[] =
    {'/','t','e','s','t','.','h','t','m','l',0};
static const WCHAR rel_url4[] =
    {'t','e',':','t','.','h','t','m','l',0};
static const WCHAR rel_url5[] =
    {'d','i','r','/','t','e','s','t','.','h','t','m','l',0};

static const WCHAR base_url1[] = {'i','t','s',':',
    't','e','s','t',':','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR base_url2[] = {'i','t','s',':','t','e','s','t','.','c','h','m',
    ':',':','/','d','i','r','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR base_url3[] = {'m','s','-','i','t','s',':','t','e','s','t','.','c','h','m',
    ':',':','/','d','i','r','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR base_url4[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
    't','e','s','t','.','c','h','m',':',':','/','d','i','r','/',
    'b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR base_url5[] = {'x','x','x',':','t','e','s','t','.','c','h','m',
    ':',':','/','d','i','r','/','b','l','a','n','k','.','h','t','m','l',0};

static const WCHAR combined_url1[] = {'i','t','s',':',
    't','e','s','t','.','c','h','m',':',':','/','t','e','s','t','.','h','t','m','l',0};
static const WCHAR combined_url2[] = {'i','t','s',':',
    't','e','s','t','.','c','h','m',':',':','/','d','i','r','/','t','e','s','t','.','h','t','m','l',0};
static const WCHAR combined_url3[] = {'i','t','s',':',
    't','e','s','t',':','.','c','h','m',':',':','/','t','e','s','t','.','h','t','m','l',0};
static const WCHAR combined_url4[] = {'i','t','s',':','t','e','s','t','.','c','h','m',
    ':',':','b','l','a','n','k','.','h','t','m','l','t','e','s','t','.','h','t','m','l',0};
static const WCHAR combined_url5[] = {'m','s','-','i','t','s',':',
    't','e','s','t','.','c','h','m',':',':','/','d','i','r','/','t','e','s','t','.','h','t','m','l',0};
static const WCHAR combined_url6[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
    't','e','s','t','.','c','h','m',':',':','/','d','i','r','/','t','e','s','t','.','h','t','m','l',0};

static const struct {
    LPCWSTR base_url;
    LPCWSTR rel_url;
    DWORD flags;
    HRESULT hres;
    LPCWSTR combined_url;
} combine_tests[] = {
    {blank_url1, blank_url1, 0, STG_E_INVALIDNAME, NULL},
    {blank_url2, blank_url2, 0, STG_E_INVALIDNAME, NULL},
    {blank_url1, rel_url1, 0, S_OK, combined_url1},
    {blank_url1, rel_url2, 0, STG_E_INVALIDNAME, NULL},
    {blank_url1, rel_url3, 0, S_OK, combined_url1},
    {blank_url1, rel_url4, 0, STG_E_INVALIDNAME, NULL},
    {blank_url1, rel_url3, URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO, S_OK, combined_url1},
    {blank_url1, rel_url5, 0, S_OK, combined_url2},
    {rel_url1, rel_url2, 0, 0x80041001, NULL},
    {base_url1, rel_url1, 0, S_OK, combined_url3},
    {base_url2, rel_url1, 0, S_OK, combined_url2},
    {blank_url4, rel_url1, 0, S_OK, combined_url4},
    {base_url3, rel_url1, 0, S_OK, combined_url5},
    {base_url4, rel_url1, 0, S_OK, combined_url6},
    {base_url5, rel_url1, 0, INET_E_USE_DEFAULT_PROTOCOLHANDLER, NULL},
    {base_url2, rel_url3, 0, S_OK, combined_url1},
};

static void test_its_protocol_info(IInternetProtocol *protocol)
{
    IInternetProtocolInfo *info;
    WCHAR buf[1024];
    DWORD size, i;
    HRESULT hres;

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetProtocolInfo, (void**)&info);
    ok(hres == S_OK, "Could not get IInternetProtocolInfo interface: %08x\n", hres);
    if(FAILED(hres))
        return;

    for(i = PARSE_CANONICALIZE; i <= PARSE_UNESCAPE; i++) {
        if(i != PARSE_CANONICALIZE && i != PARSE_SECURITY_URL) {
            hres = IInternetProtocolInfo_ParseUrl(info, blank_url1, i, 0, buf,
                    sizeof(buf)/sizeof(buf[0]), &size, 0);
            ok(hres == INET_E_DEFAULT_ACTION,
               "[%d] failed: %08x, expected INET_E_DEFAULT_ACTION\n", i, hres);
        }
    }

    for(i=0; i < sizeof(combine_tests)/sizeof(combine_tests[0]); i++) {
        size = 0xdeadbeef;
        memset(buf, 0xfe, sizeof(buf));
        hres = IInternetProtocolInfo_CombineUrl(info, combine_tests[i].base_url,
                combine_tests[i].rel_url, combine_tests[i].flags, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == combine_tests[i].hres, "[%d] CombineUrl returned %08x, expected %08x\n",
           i, hres, combine_tests[i].hres);
        ok(size == (combine_tests[i].combined_url ? lstrlenW(combine_tests[i].combined_url)+1
           : 0xdeadbeef), "[%d] unexpected size=%d\n", i, size);
        if(combine_tests[i].combined_url)
            ok(!lstrcmpW(combine_tests[i].combined_url, buf), "[%d] unexpected result\n", i);
        else
            ok(buf[0] == 0xfefe, "buf changed\n");
    }

    size = 0xdeadbeef;
    memset(buf, 0xfe, sizeof(buf));
    hres = IInternetProtocolInfo_CombineUrl(info, blank_url1, rel_url1, 0, buf,
            1, &size, 0);
    ok(hres == E_OUTOFMEMORY, "CombineUrl failed: %08x\n", hres);
    ok(size == sizeof(combined_url1)/sizeof(WCHAR), "size=%d\n", size);
    ok(buf[0] == 0xfefe, "buf changed\n");

    IInternetProtocolInfo_Release(info);
}

static void test_its_protocol(void)
{
    IInternetProtocolInfo *info;
    IClassFactory *factory;
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    static const WCHAR wrong_url1[] =
        {'i','t','s',':','t','e','s','t','.','c','h','m',':',':','/','b','l','a','n','.','h','t','m','l',0};
    static const WCHAR wrong_url2[] =
        {'i','t','s',':','t','e','s','.','c','h','m',':',':','b','/','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR wrong_url3[] =
        {'i','t','s',':','t','e','s','t','.','c','h','m','/','b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR wrong_url4[] = {'m','k',':','@','M','S','I','T','S','t','o','r',':',
         't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR wrong_url5[] = {'f','i','l','e',':',
        't','e','s','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};

    test_protocol = ITS_PROTOCOL;

    hres = CoGetClassObject(&CLSID_ITSProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK ||
       broken(hres == REGDB_E_CLASSNOTREG), /* Some W95 and NT4 */
       "CoGetClassObject failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&info);
    ok(hres == E_NOINTERFACE, "Could not get IInternetProtocolInfo: %08x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        IInternetProtocol *protocol;

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
        if(SUCCEEDED(hres)) {
            test_its_protocol_info(protocol);

            test_protocol_fail(protocol, wrong_url1, STG_E_FILENOTFOUND);
            test_protocol_fail(protocol, wrong_url2, STG_E_FILENOTFOUND);
            test_protocol_fail(protocol, wrong_url3, STG_E_FILENOTFOUND);

            hres = IInternetProtocol_Start(protocol, wrong_url4, &protocol_sink, &bind_info, 0, 0);
            ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
               "Start failed: %08x, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", hres);

            hres = IInternetProtocol_Start(protocol, wrong_url5, &protocol_sink, &bind_info, 0, 0);
            ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
               "Start failed: %08x, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", hres);

            ref = IInternetProtocol_Release(protocol);
            ok(!ref, "protocol ref=%d\n", ref);

            test_protocol_url(factory, blank_url1, TRUE);
            test_protocol_url(factory, blank_url2, TRUE);
            test_protocol_url(factory, blank_url3, TRUE);
            test_protocol_url(factory, blank_url4, TRUE);
            test_protocol_url(factory, blank_url5, TRUE);
            test_protocol_url(factory, blank_url6, TRUE);
            test_protocol_url(factory, blank_url8, TRUE);
            bindf = BINDF_FROMURLMON | BINDF_NEEDFILE;
            test_protocol_url(factory, blank_url1, TRUE);
        }

        IClassFactory_Release(factory);
    }

    IUnknown_Release(unk);
}

static void test_mk_protocol(void)
{
    IClassFactory *cf;
    HRESULT hres;

    test_protocol = MK_PROTOCOL;

    hres = CoGetClassObject(&CLSID_MkProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory,
                            (void**)&cf);
    ok(hres == S_OK ||
       broken(hres == REGDB_E_CLASSNOTREG), /* Some W95 and NT4 */
       "CoGetClassObject failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    cache_file = cache_file1;
    test_protocol_url(cf, blank_url3, TRUE);
    cache_file = cache_file2;
    test_protocol_url(cf, blank_url7, TRUE);
    cache_file = cache_file3;
    test_protocol_url(cf, blank_url8, FALSE);

    IClassFactory_Release(cf);
}

static BOOL create_chm(void)
{
    HANDLE file;
    HRSRC src;
    DWORD size;

    file = CreateFileA("test.chm", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Could not create test.chm file\n");
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    src = FindResourceA(NULL, MAKEINTRESOURCEA(60), MAKEINTRESOURCEA(60));

    WriteFile(file, LoadResource(NULL, src), SizeofResource(NULL, src), &size, NULL);
    CloseHandle(file);

    return TRUE;
}

static void delete_chm(void)
{
    BOOL ret;

    ret = DeleteFileA("test.chm");
    ok(ret, "DeleteFileA failed: %d\n", GetLastError());
}

START_TEST(protocol)
{
    OleInitialize(NULL);

    if(!create_chm())
        return;

    test_its_protocol();
    test_mk_protocol();

    delete_chm();
    OleUninitialize();
}
