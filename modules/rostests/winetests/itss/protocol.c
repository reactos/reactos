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
#ifdef __REACTOS__
#define CONST_VTABLE
#endif

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
DEFINE_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
DEFINE_EXPECT(ReportProgress_DIRECTBIND);
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportResult);
DEFINE_EXPECT(outer_QI_test);

static HRESULT expect_hrResult;
static IInternetProtocol *read_protocol = NULL;
static DWORD bindf;

static const WCHAR blank_url1[] = L"its:test.chm::/blank.html";
static const WCHAR blank_url2[] = L"mS-iTs:test.chm::/blank.html";
static const WCHAR blank_url3[] = L"mk:@MSITStore:test.chm::/blank.html";
static const WCHAR blank_url4[] = L"its:test.chm::blank.html";
static const WCHAR blank_url5[] = L"its:test.chm::\\blank.html";
static const WCHAR blank_url6[] = L"its:test.chm::/%62lank.html";
static const WCHAR blank_url7[] = L"mk:@MSITStore:test.chm::\\blank.html";
static const WCHAR blank_url8[] = L"mk:@MSITStore:test.chm::/blank.html/";
static const WCHAR blank_url9[] = L"its:test.chm::/dir/../blank.html";

static enum {
    ITS_PROTOCOL,
    MK_PROTOCOL
} test_protocol;

static const WCHAR cache_file1[] = L"test.chm::/blank.html";
static const WCHAR cache_file2[] = L"test.chm::\\blank.html";
static const WCHAR cache_file3[] = L"test.chm::/blank.html/";
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
    switch(ulStatusCode) {
    case BINDSTATUS_BEGINDOWNLOADDATA:
        CHECK_EXPECT(ReportProgress_BEGINDOWNLOADDATA);
        ok(!szStatusText, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_SENDINGREQUEST:
        CHECK_EXPECT(ReportProgress_SENDINGREQUEST);
        if(test_protocol == ITS_PROTOCOL)
            ok(!lstrcmpW(szStatusText, L"blank.html"), "unexpected szStatusText\n");
        else
            ok(szStatusText == NULL, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        ok(!lstrcmpW(szStatusText, L"text/html"), "unexpected szStatusText\n");
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        CHECK_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        ok(!lstrcmpW(szStatusText, cache_file), "unexpected szStatusText\n");
        break;
    case BINDSTATUS_DIRECTBIND:
        CHECK_EXPECT(ReportProgress_DIRECTBIND);
        ok(!szStatusText, "szStatusText != NULL\n");
        break;
    default:
        ok(0, "unexpected ulStatusCode %ld\n", ulStatusCode);
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
        ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE), "grcf = %08lx\n", grfBSCF);
    else
        ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION), "grcf = %08lx\n", grfBSCF);

    if(read_protocol) {
        BYTE buf[100];
        DWORD cb = 0xdeadbeef;
        HRESULT hres;

        hres = IInternetProtocol_Read(read_protocol, buf, sizeof(buf), &cb);
        ok(hres == S_OK, "Read failed: %08lx\n", hres);
        ok(cb == 13, "cb=%lu expected 13\n", cb);
        ok(!memcmp(buf, "<html></html>", 13), "unexpected data\n");
    }

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult,
        DWORD dwError, LPCWSTR szResult)
{
    CHECK_EXPECT(ReportResult);

    ok(hrResult == expect_hrResult, "expected: %08lx got: %08lx\n", expect_hrResult, hrResult);
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

static void test_protocol_fail(IInternetProtocol *protocol, LPCWSTR url, HRESULT expected_hres)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportResult);

    expect_hrResult = expected_hres;
    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == expected_hres, "expected: %08lx got: %08lx\n", expected_hres, hres);

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
        SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
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
            SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
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
            SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
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
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = protocol_start(protocol, url, expect_mime);
    if(FAILED(hres)) {
        IInternetProtocol_Release(protocol);
        return;
    }

    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == 13, "cb=%lu expected 13\n", cb);
    ok(!memcmp(buf, "<html></html>", 13), "unexpected data\n");
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%ld\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(FAILED(hres))
        return;

    cb = 0xdeadbeef;
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == (test_protocol == ITS_PROTOCOL ? INET_E_DATA_NOT_AVAILABLE : E_FAIL),
       "Read returned %08lx\n", hres);
    ok(cb == 0xdeadbeef, "cb=%lu expected 0xdeadbeef\n", cb);

    protocol_start(protocol, url, expect_mime);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == 2, "cb=%lu expected 2\n", cb);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == 11, "cb=%lu, expected 11\n", cb);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_FALSE, "Read failed: %08lx expected S_FALSE\n", hres);
    ok(cb == 0, "cb=%lu expected 0\n", cb);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%ld\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(protocol, url, expect_mime);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == 11, "cb=%lu, expected 11\n", cb);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%ld\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(protocol, url, expect_mime);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == 2, "cb=%lu, expected 2\n", cb);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == 2, "cb=%lu, expected 2\n", cb);
    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(cb == 2, "cb=%lu expected 2\n", cb);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%ld\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&read_protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(read_protocol, url, expect_mime);
    ref = IInternetProtocol_Release(read_protocol);
    ok(!ref, "protocol ref=%ld\n", ref);
    read_protocol = NULL;
}

static const struct {
    const WCHAR *base_url;
    const WCHAR *rel_url;
    DWORD flags;
    HRESULT hres;
    const WCHAR *combined_url;
} combine_tests[] = {
    {L"its:test.chm::/blank.html", L"its:test.chm::/blank.html", 0, STG_E_INVALIDNAME, NULL},
    {L"mS-iTs:test.chm::/blank.html", L"mS-iTs:test.chm::/blank.html", 0, STG_E_INVALIDNAME, NULL},
    {L"its:test.chm::/blank.html", L"test.html", 0, S_OK, L"its:test.chm::/test.html"},
    {L"its:test.chm::/blank.html", L"test.chm::/test.html", 0, STG_E_INVALIDNAME, NULL},
    {L"its:test.chm::/blank.html", L"/test.html", 0, S_OK, L"its:test.chm::/test.html"},
    {L"its:test.chm::/blank.html", L"te:t.html", 0, STG_E_INVALIDNAME, NULL},
    {L"its:test.chm::/blank.html", L"/test.html", URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO, S_OK, L"its:test.chm::/test.html"},
    {L"its:test.chm::/blank.html", L"dir/test.html", 0, S_OK, L"its:test.chm::/dir/test.html"},
    {L"test.html", L"test.chm::/test.html", 0, 0x80041001, NULL},
    {L"its:test:.chm::/blank.html", L"test.html", 0, S_OK, L"its:test:.chm::/test.html"},
    {L"its:test.chm::/dir/blank.html", L"test.html", 0, S_OK, L"its:test.chm::/dir/test.html"},
    {L"its:test.chm::blank.html", L"test.html", 0, S_OK, L"its:test.chm::blank.htmltest.html"},
    {L"ms-its:test.chm::/dir/blank.html", L"test.html", 0, S_OK, L"ms-its:test.chm::/dir/test.html"},
    {L"mk:@MSITStore:test.chm::/dir/blank.html", L"test.html", 0, S_OK, L"mk:@MSITStore:test.chm::/dir/test.html"},
    {L"xxx:test.chm::/dir/blank.html", L"test.html", 0, INET_E_USE_DEFAULT_PROTOCOLHANDLER, NULL},
    {L"its:test.chm::/dir/blank.html", L"/test.html", 0, S_OK, L"its:test.chm::/test.html"},
    {L"its:test.chm::/blank.html", L"#frag", 0, S_OK, L"its:test.chm::/blank.html#frag"},
    {L"its:test.chm::/blank.html#hash", L"#frag", 0, S_OK, L"its:test.chm::/blank.html#hash#frag"},
    {L"its:test.chm::/blank.html", L"test.html#frag", 0, S_OK, L"its:test.chm::/test.html#frag"},
    {L"its:test.chm::/blank.html", L"/test.html#frag", 0, S_OK, L"its:test.chm::/test.html#frag"},
    {L"its:test.chm::/blank.html", L"?query", 0, S_OK, L"its:test.chm::/?query"},
    {L"its:test.chm::/blank.html#frag/blank", L"test.html", 0, S_OK, L"its:test.chm::/blank.html#frag/test.html"},
};

static void test_its_protocol_info(IInternetProtocol *protocol)
{
    IInternetProtocolInfo *info;
    WCHAR buf[1024];
    DWORD size, i;
    HRESULT hres;

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetProtocolInfo, (void**)&info);
    ok(hres == S_OK, "Could not get IInternetProtocolInfo interface: %08lx\n", hres);
    if(FAILED(hres))
        return;

    for(i = PARSE_CANONICALIZE; i <= PARSE_UNESCAPE; i++) {
        if(i != PARSE_CANONICALIZE && i != PARSE_SECURITY_URL) {
            hres = IInternetProtocolInfo_ParseUrl(info, blank_url1, i, 0, buf,
                    ARRAY_SIZE(buf), &size, 0);
            ok(hres == INET_E_DEFAULT_ACTION,
               "[%ld] failed: %08lx, expected INET_E_DEFAULT_ACTION\n", i, hres);
        }
    }

    for(i=0; i < ARRAY_SIZE(combine_tests); i++) {
        size = 0xdeadbeef;
        memset(buf, 0xfe, sizeof(buf));
        hres = IInternetProtocolInfo_CombineUrl(info, combine_tests[i].base_url,
                combine_tests[i].rel_url, combine_tests[i].flags, buf, ARRAY_SIZE(buf), &size, 0);
        ok(hres == combine_tests[i].hres, "[%ld] CombineUrl returned %08lx, expected %08lx\n",
           i, hres, combine_tests[i].hres);
        ok(size == (combine_tests[i].combined_url ? lstrlenW(combine_tests[i].combined_url)+1
           : 0xdeadbeef), "[%ld] unexpected size=%ld\n", i, size);
        if(combine_tests[i].combined_url)
            ok(!lstrcmpW(buf, combine_tests[i].combined_url), "[%ld] unexpected result: %s\n", i, wine_dbgstr_w(buf));
        else
            ok(buf[0] == 0xfefe, "buf changed\n");
    }

    size = 0xdeadbeef;
    memset(buf, 0xfe, sizeof(buf));
    hres = IInternetProtocolInfo_CombineUrl(info, L"its:test.chm::/blank.html", L"test.html", 0, buf,
            1, &size, 0);
    ok(hres == E_OUTOFMEMORY, "CombineUrl failed: %08lx\n", hres);
    ok(size == 25, "size=%ld\n", size);
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

    static const WCHAR wrong_url1[] = L"its:test.chm::/blan.html";
    static const WCHAR wrong_url2[] = L"its:tes.chm::b/lank.html";
    static const WCHAR wrong_url3[] = L"its:test.chm/blank.html";
    static const WCHAR wrong_url4[] = L"mk:@MSITStor:test.chm::/blank.html";
    static const WCHAR wrong_url5[] = L"file:tes.chm::/blank.html";

    test_protocol = ITS_PROTOCOL;

    hres = CoGetClassObject(&CLSID_ITSProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK ||
       broken(hres == REGDB_E_CLASSNOTREG), /* Some W95 and NT4 */
       "CoGetClassObject failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&info);
    ok(hres == E_NOINTERFACE, "Could not get IInternetProtocolInfo: %08lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        IInternetProtocol *protocol;

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
        if(SUCCEEDED(hres)) {
            test_its_protocol_info(protocol);

            test_protocol_fail(protocol, wrong_url1, STG_E_FILENOTFOUND);
            test_protocol_fail(protocol, wrong_url2, STG_E_FILENOTFOUND);
            test_protocol_fail(protocol, wrong_url3, STG_E_FILENOTFOUND);

            hres = IInternetProtocol_Start(protocol, wrong_url4, &protocol_sink, &bind_info, 0, 0);
            ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
               "Start failed: %08lx, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", hres);

            hres = IInternetProtocol_Start(protocol, wrong_url5, &protocol_sink, &bind_info, 0, 0);
            ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
               "Start failed: %08lx, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", hres);

            ref = IInternetProtocol_Release(protocol);
            ok(!ref, "protocol ref=%ld\n", ref);

            test_protocol_url(factory, blank_url1, TRUE);
            test_protocol_url(factory, blank_url2, TRUE);
            test_protocol_url(factory, blank_url3, TRUE);
            test_protocol_url(factory, blank_url4, TRUE);
            test_protocol_url(factory, blank_url5, TRUE);
            test_protocol_url(factory, blank_url6, TRUE);
            test_protocol_url(factory, blank_url8, TRUE);
            test_protocol_url(factory, blank_url9, TRUE);
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
       "CoGetClassObject failed: %08lx\n", hres);
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
    ok(ret, "DeleteFileA failed: %ld\n", GetLastError());
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
    ok(hres == S_OK, "Could not get IInternetProtocol iface: %08lx\n", hres);

    SET_EXPECT(outer_QI_test);
    hres = IUnknown_QueryInterface(unk2, &outer_test_iid, (void**)&unk3);
    CHECK_CALLED(outer_QI_test);
    ok(hres == S_OK, "Could not get IInternetProtocol iface: %08lx\n", hres);
    ok(unk3 == (IUnknown*)0xdeadbeef, "unexpected unk2\n");

    IUnknown_Release(unk2);
    IUnknown_Release(unk);

    unk = (void*)0xdeadbeef;
    hres = IClassFactory_CreateInstance(class_factory, &outer, &IID_IInternetProtocol, (void**)&unk);
    ok(hres == CLASS_E_NOAGGREGATION, "CreateInstance returned: %08lx\n", hres);
    ok(!unk, "unk = %p\n", unk);

    IClassFactory_Release(class_factory);
}

START_TEST(protocol)
{
    OleInitialize(NULL);

    if(!create_chm())
        return;

    test_its_protocol();
    test_mk_protocol();
    test_com_aggregation(&CLSID_ITSProtocol);

    delete_chm();
    OleUninitialize();
}
