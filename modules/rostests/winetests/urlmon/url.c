/*
 * UrlMon URL tests
 *
 * Copyright 2004 Kevin Koltzau
 * Copyright 2004-2007 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "urlmon.h"
#include "wininet.h"
#include "mshtml.h"
#include "shlwapi.h"

#include "wine/test.h"

static HRESULT (WINAPI *pCreateAsyncBindCtxEx)(IBindCtx *, DWORD,
                IBindStatusCallback *, IEnumFORMATETC *, IBindCtx **, DWORD);
static HRESULT (WINAPI *pCreateUri)(LPCWSTR, DWORD, DWORD_PTR, IUri**);

DEFINE_OLEGUID(CLSID_CompositeMoniker, 0x309, 0, 0 );
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(CLSID_IdentityUnmarshal,0x0000001b,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IBindStatusCallbackHolder,0x79eac9cc,0xbaf9,0x11ce,0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b);
extern CLSID CLSID_AboutProtocol;

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED_BROKEN(func) \
    do { \
        ok(called_ ## func || broken(!called_ ## func), "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(QueryInterface_IServiceProvider);
DEFINE_EXPECT(QueryInterface_IHttpNegotiate);
DEFINE_EXPECT(QueryInterface_IBindStatusCallback);
DEFINE_EXPECT(QueryInterface_IBindStatusCallbackEx);
DEFINE_EXPECT(QueryInterface_IBindStatusCallbackHolder);
DEFINE_EXPECT(QueryInterface_IAuthenticate);
DEFINE_EXPECT(QueryInterface_IInternetProtocol);
DEFINE_EXPECT(QueryInterface_IWindowForBindingUI);
DEFINE_EXPECT(QueryInterface_IHttpSecurity);
DEFINE_EXPECT(QueryService_IAuthenticate);
DEFINE_EXPECT(QueryService_IInternetProtocol);
DEFINE_EXPECT(QueryService_IInternetBindInfo);
DEFINE_EXPECT(QueryService_IWindowForBindingUI);
DEFINE_EXPECT(QueryService_IHttpSecurity);
DEFINE_EXPECT(BeginningTransaction);
DEFINE_EXPECT(OnResponse);
DEFINE_EXPECT(QueryInterface_IHttpNegotiate2);
DEFINE_EXPECT(GetRootSecurityId);
DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(GetBindInfoEx);
DEFINE_EXPECT(OnStartBinding);
DEFINE_EXPECT(OnProgress_FINDINGRESOURCE);
DEFINE_EXPECT(OnProgress_CONNECTING);
DEFINE_EXPECT(OnProgress_REDIRECTING);
DEFINE_EXPECT(OnProgress_SENDINGREQUEST);
DEFINE_EXPECT(OnProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(OnProgress_BEGINDOWNLOADDATA);
DEFINE_EXPECT(OnProgress_DOWNLOADINGDATA);
DEFINE_EXPECT(OnProgress_ENDDOWNLOADDATA);
DEFINE_EXPECT(OnProgress_CACHEFILENAMEAVAILABLE);
DEFINE_EXPECT(OnStopBinding);
DEFINE_EXPECT(OnDataAvailable);
DEFINE_EXPECT(OnObjectAvailable);
DEFINE_EXPECT(Obj_OnStartBinding);
DEFINE_EXPECT(Obj_OnStopBinding);
DEFINE_EXPECT(Obj_GetBindInfo);
DEFINE_EXPECT(Obj_OnProgress_BEGINDOWNLOADDATA);
DEFINE_EXPECT(Obj_OnProgress_ENDDOWNLOADDATA);
DEFINE_EXPECT(Obj_OnProgress_SENDINGREQUEST);
DEFINE_EXPECT(Obj_OnProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(Obj_OnProgress_CLASSIDAVAILABLE);
DEFINE_EXPECT(Obj_OnProgress_BEGINSYNCOPERATION);
DEFINE_EXPECT(Obj_OnProgress_ENDSYNCOPERATION);
DEFINE_EXPECT(Obj_OnProgress_FINDINGRESOURCE);
DEFINE_EXPECT(Obj_OnProgress_CONNECTING);
DEFINE_EXPECT(Obj_OnProgress_REDIRECTING);
DEFINE_EXPECT(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
DEFINE_EXPECT(Start);
DEFINE_EXPECT(Read);
DEFINE_EXPECT(LockRequest);
DEFINE_EXPECT(Terminate);
DEFINE_EXPECT(UnlockRequest);
DEFINE_EXPECT(Continue);
DEFINE_EXPECT(Abort);
DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(Load);
DEFINE_EXPECT(PutProperty_MIMETYPEPROP);
DEFINE_EXPECT(PutProperty_CLASSIDPROP);
DEFINE_EXPECT(SetPriority);
DEFINE_EXPECT(GetWindow_IHttpSecurity);
DEFINE_EXPECT(GetWindow_IWindowForBindingUI);
DEFINE_EXPECT(GetWindow_ICodeInstall);
DEFINE_EXPECT(OnSecurityProblem);

static const WCHAR winetest_data_urlW[] =
    {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/',
     't','e','s','t','s','/','d','a','t','a','.','p','h','p',0};
static const WCHAR about_blankW[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};

static WCHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

#define WINEHQ_IP "4.4.81.126"
static const WCHAR wszWineHQSite[] = L"gitlab.winehq.org";
static const WCHAR wszWineHQIP[] = L"" WINEHQ_IP;
static const CHAR wszIndexHtmlA[] = "index.html";
static const WCHAR cache_fileW[] = {'c',':','\\','c','a','c','h','e','.','h','t','m',0};
static const CHAR dwl_htmlA[] = "dwl.html";
static const WCHAR dwl_htmlW[] = {'d','w','l','.','h','t','m','l',0};
static const CHAR test_txtA[] = "test.txt";
static const WCHAR emptyW[] = {0};

static BOOL stopped_binding = FALSE, stopped_obj_binding = FALSE, emulate_protocol = FALSE,
    data_available = FALSE, http_is_first = TRUE, bind_to_object = FALSE, filedwl_api, post_test;
static DWORD nread = 0, bindf = 0, prot_state = 0, thread_id, tymed, security_problem;
static const WCHAR *reported_url;
static CHAR mime_type[512];
static IInternetProtocolSink *protocol_sink = NULL;
static IBinding *current_binding;
static HANDLE complete_event, complete_event2;
static HRESULT binding_hres;
static HRESULT onsecurityproblem_hres;
static HRESULT abort_hres;
static BOOL have_IHttpNegotiate2, use_bscex, is_async_prot;
static BOOL test_redirect, use_cache_file, callback_read, no_callback, test_abort;
static WCHAR cache_file_name[MAX_PATH];
static WCHAR http_cache_file[MAX_PATH];
static BOOL only_check_prot_args = FALSE;
static BOOL invalid_cn_accepted = FALSE;
static BOOL abort_start = FALSE;
static BOOL abort_progress = FALSE;
static BOOL async_switch = FALSE;
static BOOL strict_bsc_qi;
static DWORD bindtest_flags;
static const char *test_file;

static WCHAR file_url[INTERNET_MAX_URL_LENGTH], current_url[INTERNET_MAX_URL_LENGTH];

static enum {
    HTTP_TEST,
    ABOUT_TEST,
    FILE_TEST,
    ITS_TEST,
    MK_TEST,
    HTTPS_TEST,
    FTP_TEST,
    WINETEST_TEST,
    WINETEST_SYNC_TEST
} test_protocol;

static enum {
    BEFORE_DOWNLOAD,
    DOWNLOADING,
    END_DOWNLOAD
} download_state;

static BOOL proxy_active(void)
{
    HKEY internet_settings;
    DWORD proxy_enable;
    DWORD size;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
                      0, KEY_QUERY_VALUE, &internet_settings) != ERROR_SUCCESS)
        return FALSE;

    size = sizeof(DWORD);
    if (RegQueryValueExA(internet_settings, "ProxyEnable", NULL, NULL, (LPBYTE) &proxy_enable, &size) != ERROR_SUCCESS)
        proxy_enable = 0;

    RegCloseKey(internet_settings);

    return proxy_enable != 0;
}

static BOOL is_urlmon_protocol(int prot)
{
    return prot == FILE_TEST || prot == HTTP_TEST || prot == HTTPS_TEST || prot == FTP_TEST || prot == MK_TEST;
}

static void test_CreateURLMoniker(LPCWSTR url1, LPCWSTR url2)
{
    HRESULT hr;
    IMoniker *mon1 = NULL;
    IMoniker *mon2 = NULL;

    hr = CreateURLMoniker(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG,
       "Expected CreateURLMoniker to return E_INVALIDARG, got 0x%08lx\n", hr);

    mon1 = (IMoniker *)0xdeadbeef;
    hr = CreateURLMoniker(NULL, NULL, &mon1);
    ok(hr == E_INVALIDARG,
       "Expected CreateURLMoniker to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(mon1 == NULL, "Expected the output pointer to be NULL, got %p\n", mon1);

    hr = CreateURLMoniker(NULL, emptyW, NULL);
    ok(hr == E_INVALIDARG,
       "Expected CreateURLMoniker to return E_INVALIDARG, got 0x%08lx\n", hr);

    hr = CreateURLMoniker(NULL, emptyW, &mon1);
    ok(hr == S_OK ||
       broken(hr == MK_E_SYNTAX), /* IE5/IE5.01/IE6 SP2 */
       "Expected CreateURLMoniker to return S_OK, got 0x%08lx\n", hr);
    if(mon1) IMoniker_Release(mon1);

    hr = CreateURLMoniker(NULL, url1, &mon1);
    ok(hr == S_OK, "failed to create moniker: 0x%08lx\n", hr);
    if(hr == S_OK) {
        hr = CreateURLMoniker(mon1, url2, &mon2);
        ok(hr == S_OK, "failed to create moniker: 0x%08lx\n", hr);
    }
    if(mon1) IMoniker_Release(mon1);
    if(mon2) IMoniker_Release(mon2);
}

static void test_create(void)
{
    static const WCHAR relativeW[] = {'a','/','b','.','t','x','t',0};
    IStream *stream;
    IMoniker *mon;
    IBindCtx *bctx;
    HRESULT hr;

    static const WCHAR TEST_PART_URL_1[] = {'/','t','e','s','t','s','/','d','a','t','a','.','p','h','p',0};

    test_CreateURLMoniker(winetest_data_urlW, TEST_PART_URL_1);

    mon = (void*)0xdeadbeef;
    hr = CreateURLMoniker(NULL, relativeW, &mon);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = CreateBindCtx(0, &bctx);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    stream = (void*)0xdeadbeef;
    hr = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&stream);
    todo_wine ok(hr == INET_E_UNKNOWN_PROTOCOL, "got 0x%08lx\n", hr);
    ok(stream == NULL, "got %p\n", stream);

    hr = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    stream = (void*)0xdeadbeef;
    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IStream, (void**)&stream);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(stream == NULL || broken(stream == (void*)0xdeadbeef) /* starting XP SP3 it's set to null */,
        "got %p\n", stream);

    IMoniker_Release(mon);

    mon = (void*)0xdaedbeef;
    hr = CreateURLMoniker(NULL, winetest_data_urlW, &mon);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    stream = (void*)0xdeadbeef;
    hr = IMoniker_BindToStorage(mon, NULL, NULL, &IID_IStream, (void**)&stream);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(stream == NULL || broken(stream == (void*)0xdeadbeef) /* starting XP SP3 it's set to null */,
        "got %p\n", stream);

    hr = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    IMoniker_Release(mon);
    IBindCtx_Release(bctx);
}

static HRESULT WINAPI Priority_QueryInterface(IInternetPriority *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Priority_AddRef(IInternetPriority *iface)
{
    return 2;
}

static ULONG WINAPI Priority_Release(IInternetPriority *iface)
{
    return 1;
}

static HRESULT WINAPI Priority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    CHECK_EXPECT(SetPriority);
    ok(!nPriority, "nPriority = %ld\n", nPriority);
    return S_OK;
}

static HRESULT WINAPI Priority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IInternetPriorityVtbl InternetPriorityVtbl = {
    Priority_QueryInterface,
    Priority_AddRef,
    Priority_Release,
    Priority_SetPriority,
    Priority_GetPriority
};

static IInternetPriority InternetPriority = { &InternetPriorityVtbl };

static HRESULT WINAPI Protocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    if (winetest_debug > 1) trace("IInternetProtocol::QueryInterface(%s)\n", debugstr_guid(riid));

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocol, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        if(!is_urlmon_protocol(test_protocol))
            return E_NOINTERFACE;

        *ppv = &InternetPriority;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI Protocol_AddRef(IInternetProtocol *iface)
{
    return 2;
}

static ULONG WINAPI Protocol_Release(IInternetProtocol *iface)
{
    return 1;
}

static void test_switch_fail(void)
{
    IInternetProtocolSink *binding_sink;
    PROTOCOLDATA protocoldata = {0};
    HRESULT hres;

    static BOOL tested_switch_fail;

    if(tested_switch_fail)
        return;

    tested_switch_fail = TRUE;

    hres = IBinding_QueryInterface(current_binding, &IID_IInternetProtocolSink, (void**)&binding_sink);
    ok(hres == S_OK, "Could not get IInternetProtocolSink iface: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
        ok(hres == E_FAIL, "Switch failed: %08lx, expected E_FAIL\n", hres);
        IInternetProtocolSink_Release(binding_sink);
    }
}

static DWORD WINAPI thread_proc(PVOID arg)
{
    PROTOCOLDATA protocoldata = {0};
    HRESULT hres;

    if(!no_callback) {
        if(bind_to_object)
            SET_EXPECT(Obj_OnProgress_FINDINGRESOURCE);
        else
            SET_EXPECT(OnProgress_FINDINGRESOURCE);
    }
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_FINDINGRESOURCE, wszWineHQSite);
    ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);
    if(!no_callback) {
        ok( WaitForSingleObject(complete_event, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
        if(bind_to_object)
            CHECK_CALLED(Obj_OnProgress_FINDINGRESOURCE);
        else
            CHECK_CALLED(OnProgress_FINDINGRESOURCE);
    }

    if(!no_callback) {
        if(bind_to_object)
            SET_EXPECT(Obj_OnProgress_CONNECTING);
        else
            SET_EXPECT(OnProgress_CONNECTING);
    }
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_CONNECTING, wszWineHQIP);
    ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);
    if(!no_callback) {
        ok( WaitForSingleObject(complete_event, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
        if(bind_to_object)
            CHECK_CALLED(Obj_OnProgress_CONNECTING);
        else
            CHECK_CALLED(OnProgress_CONNECTING);
    }

    if(!no_callback) {
        if(bind_to_object)
            SET_EXPECT(Obj_OnProgress_SENDINGREQUEST);
        else
            SET_EXPECT(OnProgress_SENDINGREQUEST);
    }
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_SENDINGREQUEST, NULL);
    ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);
    if(!no_callback) {
        ok( WaitForSingleObject(complete_event, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
        if(bind_to_object)
            CHECK_CALLED(Obj_OnProgress_SENDINGREQUEST);
        else
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
    }

    if(test_redirect) {
        if(bind_to_object)
            SET_EXPECT(Obj_OnProgress_REDIRECTING);
        else
            SET_EXPECT(OnProgress_REDIRECTING);
        hres = IInternetProtocolSink_ReportProgress(protocol_sink, BINDSTATUS_REDIRECTING, winetest_data_urlW);
        ok(hres == S_OK, "ReportProgress(BINDSTATUS_REFIRECTING) failed: %08lx\n", hres);
        ok( WaitForSingleObject(complete_event, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
        if(bind_to_object)
            CHECK_CALLED(Obj_OnProgress_REDIRECTING);
        else
            CHECK_CALLED(OnProgress_REDIRECTING);
    }

    test_switch_fail();

    SET_EXPECT(Continue);
    prot_state = 1;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08lx\n", hres);
    ok( WaitForSingleObject(complete_event, 90000) == WAIT_OBJECT_0, "wait timed out\n" );

    CHECK_CALLED(Continue);
    CHECK_CALLED(Read);
    if(bind_to_object) {
        CHECK_CALLED(Obj_OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_BEGINDOWNLOADDATA);
        CHECK_CALLED(Obj_OnProgress_CLASSIDAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_BEGINSYNCOPERATION);
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(PutProperty_MIMETYPEPROP);
        CHECK_CALLED_BROKEN(PutProperty_CLASSIDPROP);
        CHECK_CALLED(Load);
        CHECK_CALLED(Obj_OnProgress_ENDSYNCOPERATION);
        CHECK_CALLED(OnObjectAvailable);
        CHECK_CALLED(Obj_OnStopBinding);
    }else if(!no_callback) {
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        CHECK_CALLED(OnDataAvailable);
    }else {
        CHECK_CALLED(LockRequest);
    }

    SET_EXPECT(Continue);
    prot_state = 2;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08lx\n", hres);
    ok( WaitForSingleObject(complete_event, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
    CHECK_CALLED(Continue);
    if(test_abort) {
        CHECK_CALLED(OnProgress_DOWNLOADINGDATA);
        CHECK_CALLED(OnStopBinding);
        SetEvent(complete_event2);
        return 0;
    }else {
        CHECK_CALLED(Read);
        if(!no_callback) {
            CHECK_CALLED(OnProgress_DOWNLOADINGDATA);
            CHECK_CALLED(OnDataAvailable);
        }
    }

    SET_EXPECT(Continue);
    prot_state = 2;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08lx\n", hres);
    ok( WaitForSingleObject(complete_event, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
    CHECK_CALLED(Continue);
    CHECK_CALLED(Read);
    if(!no_callback) {
        CHECK_CALLED(OnProgress_DOWNLOADINGDATA);
        CHECK_CALLED(OnDataAvailable);
    }

    SET_EXPECT(Continue);
    prot_state = 3;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08lx\n", hres);
    ok( WaitForSingleObject(complete_event, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
    CHECK_CALLED(Continue);
    CHECK_CALLED(Read);
    if(!no_callback) {
        CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
        CHECK_CALLED(OnDataAvailable);
        CHECK_CALLED(OnStopBinding);
    }

    SET_EXPECT(Read);

    SetEvent(complete_event2);
    return 0;
}

static HRESULT WINAPI Protocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    BINDINFO bindinfo;
    DWORD bind_info, bscf = BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION;
    HRESULT hres;

    static const STGMEDIUM stgmed_zero = {0};
    static const SECURITY_ATTRIBUTES sa_zero = {0};

    CHECK_EXPECT(Start);

    nread = 0;

    reported_url = szUrl;
    if(!filedwl_api) /* FIXME */
        ok(szUrl && !lstrcmpW(szUrl, current_url), "wrong url %s\n", wine_dbgstr_w(szUrl));
    ok(pOIProtSink != NULL, "pOIProtSink == NULL\n");
    ok(pOIBindInfo != NULL, "pOIBindInfo == NULL\n");
    ok(grfPI == 0, "grfPI=%ld, expected 0\n", grfPI);
    ok(dwReserved == 0, "dwReserved=%Ix, expected 0\n", dwReserved);

    if(!filedwl_api && binding_hres != S_OK) {
        SET_EXPECT(OnStopBinding);
        SET_EXPECT(Terminate);
        hres = IInternetProtocolSink_ReportResult(pOIProtSink, binding_hres, 0, NULL);
        ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);
        CHECK_CALLED(OnStopBinding);
        CHECK_CALLED(Terminate);

        return S_OK;
    }

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = 0;
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bind_info, &bindinfo);
    ok(hres == E_INVALIDARG, "GetBindInfo returned: %08lx, expected E_INVALIDARG\n", hres);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bind_info, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);

    ok(bind_info & BINDF_FROMURLMON, "BINDF_FROMURLMON is not set\n");

    if(filedwl_api || !is_urlmon_protocol(test_protocol) || tymed != TYMED_ISTREAM ||
       !(bindf&BINDF_ASYNCSTORAGE) || !(bindf&BINDF_PULLDATA))
        ok(bind_info & BINDF_NEEDFILE, "BINDF_NEEDFILE is not set\n");
    else if(test_protocol != MK_TEST) /* IE10 sets BINDF_NEEDFILE for mk: protocol */
        ok(!(bind_info & BINDF_NEEDFILE), "BINDF_NEEDFILE is set\n");

    bind_info &= ~(BINDF_NEEDFILE|BINDF_FROMURLMON);
    if(filedwl_api || no_callback)
        ok(bind_info == BINDF_PULLDATA, "bind_info = %lx, expected BINDF_PULLDATA\n", bind_info);
    else
        ok(bind_info == (bindf & ~(BINDF_NEEDFILE|BINDF_FROMURLMON)), "bind_info = %lx, expected %lx\n",
           bind_info, (bindf & ~(BINDF_NEEDFILE|BINDF_FROMURLMON)));

    ok(bindinfo.cbSize == sizeof(bindinfo), "bindinfo.cbSize = %ld\n", bindinfo.cbSize);
    ok(!bindinfo.szExtraInfo, "bindinfo.szExtraInfo = %p\n", bindinfo.szExtraInfo);
    ok(!memcmp(&bindinfo.stgmedData, &stgmed_zero, sizeof(STGMEDIUM)), "wrong stgmedData\n");
    ok(!bindinfo.grfBindInfoF, "bindinfo.grfBindInfoF = %ld\n", bindinfo.grfBindInfoF);
    ok(!bindinfo.dwBindVerb, "bindinfo.dwBindVerb = %ld\n", bindinfo.dwBindVerb);
    ok(!bindinfo.szCustomVerb, "bindinfo.szCustomVerb = %p\n", bindinfo.szCustomVerb);
    ok(!bindinfo.cbstgmedData, "bindinfo.cbstgmedData = %ld\n", bindinfo.cbstgmedData);
    ok(bindinfo.dwOptions == (bind_to_object ? 0x100000 : 0), "bindinfo.dwOptions = %lx\n", bindinfo.dwOptions);
    ok(!bindinfo.dwOptionsFlags, "bindinfo.dwOptionsFlags = %ld\n", bindinfo.dwOptionsFlags);
    ok(!bindinfo.dwCodePage, "bindinfo.dwCodePage = %ld\n", bindinfo.dwCodePage);
    ok(!memcmp(&bindinfo.securityAttributes, &sa_zero, sizeof(sa_zero)), "wrong bindinfo.securityAttributes\n");
    ok(IsEqualGUID(&bindinfo.iid, &IID_NULL), "wrong bindinfo.iid\n");
    ok(!bindinfo.pUnk, "bindinfo.pUnk = %p\n", bindinfo.pUnk);
    ok(!bindinfo.dwReserved, "bindinfo.dwReserved = %ld\n", bindinfo.dwReserved);

    if(only_check_prot_args)
        return E_FAIL;

    switch(test_protocol) {
    case MK_TEST:
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_DIRECTBIND, NULL);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08lx\n", hres);

    case FILE_TEST:
    case ITS_TEST:
        if(bind_to_object)
            SET_EXPECT(Obj_OnProgress_SENDINGREQUEST);
        else
            SET_EXPECT(OnProgress_SENDINGREQUEST);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_SENDINGREQUEST, emptyW);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08lx\n", hres);
        if(bind_to_object)
            CHECK_CALLED(Obj_OnProgress_SENDINGREQUEST);
        else
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
    case WINETEST_SYNC_TEST:
        IInternetProtocolSink_AddRef(pOIProtSink);
        protocol_sink = pOIProtSink;
    default:
        break;
    }

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST) {
        IServiceProvider *service_provider;
        IHttpNegotiate *http_negotiate;
        IHttpNegotiate2 *http_negotiate2;
        IHttpSecurity *http_security;
        LPWSTR ua = (LPWSTR)0xdeadbeef, accept_mimes[256];
        LPWSTR additional_headers = (LPWSTR)0xdeadbeef;
        BYTE sec_id[100];
        DWORD fetched = 256, size = 100;
        DWORD tid;

        static const WCHAR wszMimes[] = {'*','/','*',0};

        SET_EXPECT(QueryService_IInternetBindInfo);
        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_USER_AGENT,
                                               &ua, 1, &fetched);
        CLEAR_CALLED(QueryService_IInternetBindInfo); /* IE <8 */

        ok(hres == E_NOINTERFACE,
           "GetBindString(BINDSTRING_USER_AGETNT) failed: %08lx\n", hres);
        ok(fetched == 256, "fetched = %ld, expected 254\n", fetched);
        ok(ua == (LPWSTR)0xdeadbeef, "ua =  %p\n", ua);

        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                               accept_mimes, 256, &fetched);
        ok(hres == S_OK,
           "GetBindString(BINDSTRING_ACCEPT_MIMES) failed: %08lx\n", hres);
        ok(fetched == 1, "fetched = %ld, expected 1\n", fetched);
        ok(!lstrcmpW(wszMimes, accept_mimes[0]), "unexpected mimes\n");
        CoTaskMemFree(accept_mimes[0]);

        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                               NULL, 256, &fetched);
        ok(hres == E_INVALIDARG,
           "GetBindString(BINDSTRING_ACCEPT_MIMES) failed: %08lx\n", hres);

        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                               accept_mimes, 256, NULL);
        ok(hres == E_INVALIDARG,
           "GetBindString(BINDSTRING_ACCEPT_MIMES) failed: %08lx\n", hres);

        hres = IInternetBindInfo_QueryInterface(pOIBindInfo, &IID_IServiceProvider,
                                                (void**)&service_provider);
        ok(hres == S_OK, "QueryInterface failed: %08lx\n", hres);

        SET_EXPECT(QueryInterface_IHttpNegotiate);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
                &IID_IHttpNegotiate, (void**)&http_negotiate);
        CLEAR_CALLED(QueryInterface_IHttpNegotiate); /* IE <8 */
        ok(hres == S_OK, "QueryService failed: %08lx\n", hres);

        if(!no_callback) {
            SET_EXPECT(BeginningTransaction);
            SET_EXPECT(QueryInterface_IHttpNegotiate);
        }
        hres = IHttpNegotiate_BeginningTransaction(http_negotiate, current_url,
                                                   NULL, 0, &additional_headers);
        if(!no_callback) {
            CHECK_CALLED_BROKEN(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(BeginningTransaction);
        }
        IHttpNegotiate_Release(http_negotiate);
        ok(hres == S_OK, "BeginningTransction failed: %08lx\n", hres);
        ok(additional_headers == NULL, "additional_headers=%p\n", additional_headers);

        SET_EXPECT(QueryInterface_IHttpNegotiate2);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate2,
                &IID_IHttpNegotiate2, (void**)&http_negotiate2);
        CLEAR_CALLED(QueryInterface_IHttpNegotiate2); /* IE <8 */
        ok(hres == S_OK, "QueryService failed: %08lx\n", hres);

        size = 512;
        if(!no_callback) {
            SET_EXPECT(QueryInterface_IHttpNegotiate2);
            SET_EXPECT(GetRootSecurityId);
        }
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, sec_id, &size, 0);
        if(!no_callback) {
            CHECK_CALLED_BROKEN(QueryInterface_IHttpNegotiate2);
            CHECK_CALLED(GetRootSecurityId);
        }
        IHttpNegotiate2_Release(http_negotiate2);
        ok(hres == E_FAIL, "GetRootSecurityId failed: %08lx, expected E_FAIL\n", hres);
        ok(size == (no_callback ? 512 : 13), "size=%ld\n", size);

        if(!no_callback) {
            SET_EXPECT(QueryService_IHttpSecurity);
            SET_EXPECT(QueryInterface_IHttpSecurity);
        }
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpSecurity,
                &IID_IHttpSecurity, (void**)&http_security);
        ok(hres == (no_callback ? E_NOINTERFACE : S_OK), "QueryService failed: 0x%08lx\n", hres);
        if(!no_callback) {
            CHECK_CALLED(QueryService_IHttpSecurity);
            CHECK_CALLED(QueryInterface_IHttpSecurity);
        }

        IServiceProvider_Release(service_provider);

        IInternetProtocolSink_AddRef(pOIProtSink);
        protocol_sink = pOIProtSink;

        if(async_switch) {
            PROTOCOLDATA data;

            memset(&data, 0, sizeof(data));
            data.grfFlags = PI_FORCE_ASYNC;
            prot_state = 0;
            hres = IInternetProtocolSink_Switch(pOIProtSink, &data);
            ok(hres == S_OK, "Switch failed: %08lx\n", hres);
            SET_EXPECT(Continue);
            SetEvent(complete_event2);
            return E_PENDING;
        } else {
            CreateThread(NULL, 0, thread_proc, NULL, 0, &tid);
            return S_OK;
        }
    }

    if(test_protocol == FILE_TEST) {
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_CACHEFILENAMEAVAILABLE, file_url+7);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08lx\n", hres);

        if(bind_to_object)
            SET_EXPECT(Obj_OnProgress_MIMETYPEAVAILABLE);
        else
            SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE) failed: %08lx\n", hres);
        if(bind_to_object)
            CHECK_CALLED(Obj_OnProgress_MIMETYPEAVAILABLE);
        else
            CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    }else if(test_protocol == WINETEST_SYNC_TEST) {
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08lx\n", hres);
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    }else {
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_MIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08lx\n", hres);
    }

    if(test_protocol == ABOUT_TEST)
        bscf |= BSCF_DATAFULLYAVAILABLE;
    if(test_protocol == ITS_TEST)
        bscf = BSCF_FIRSTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE;

    SET_EXPECT(Read);
    if(bind_to_object) {
        if(test_protocol != FILE_TEST && test_protocol != MK_TEST && test_protocol != WINETEST_SYNC_TEST)
            SET_EXPECT(Obj_OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(Obj_OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            SET_EXPECT(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
        SET_EXPECT(Obj_OnProgress_ENDDOWNLOADDATA);
        SET_EXPECT(Obj_OnProgress_CLASSIDAVAILABLE);
        SET_EXPECT(Obj_OnProgress_BEGINSYNCOPERATION);
        SET_EXPECT(CreateInstance);
        SET_EXPECT(PutProperty_MIMETYPEPROP);
        SET_EXPECT(PutProperty_CLASSIDPROP);
        SET_EXPECT(Load);
        SET_EXPECT(Obj_OnProgress_ENDSYNCOPERATION);
        SET_EXPECT(OnObjectAvailable);
        SET_EXPECT(Obj_OnStopBinding);
    }else {
        if(test_protocol != FILE_TEST && test_protocol != MK_TEST)
            SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_CACHEFILENAMEAVAILABLE);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
        SET_EXPECT(LockRequest);
        if(!filedwl_api)
            SET_EXPECT(OnDataAvailable);
        if(test_protocol != WINETEST_SYNC_TEST) /* Set in Read after ReportResult call */
            SET_EXPECT(OnStopBinding);
    }

    hres = IInternetProtocolSink_ReportData(pOIProtSink, bscf, 13, 13);
    ok(hres == S_OK, "ReportData failed: %08lx\n", hres);

    CHECK_CALLED(Read);
    if(bind_to_object) {
        if(test_protocol != FILE_TEST && test_protocol != MK_TEST)
            CHECK_CALLED(Obj_OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            CHECK_CALLED(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_ENDDOWNLOADDATA);
        CHECK_CALLED(Obj_OnProgress_CLASSIDAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_BEGINSYNCOPERATION);
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(PutProperty_MIMETYPEPROP);
        CHECK_CALLED_BROKEN(PutProperty_CLASSIDPROP);
        CHECK_CALLED(Load);
        CHECK_CALLED(Obj_OnProgress_ENDSYNCOPERATION);
        CHECK_CALLED(OnObjectAvailable);
        CHECK_CALLED(Obj_OnStopBinding);
    }else {
        if(test_protocol != FILE_TEST && test_protocol != MK_TEST && test_protocol != WINETEST_SYNC_TEST)
            CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_CACHEFILENAMEAVAILABLE);
        CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
        CHECK_CALLED(LockRequest);
        if(!filedwl_api)
            CHECK_CALLED(OnDataAvailable);
        CHECK_CALLED(OnStopBinding);
    }

    if(test_protocol == ITS_TEST) {
        SET_EXPECT(Read);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_BEGINDOWNLOADDATA, NULL);
        ok(hres == S_OK, "ReportProgress(BINDSTATUS_BEGINDOWNLOADDATA) failed: %08lx\n", hres);
        CHECK_CALLED(Read);
    }else if(!bind_to_object && test_protocol == FILE_TEST) {
        SET_EXPECT(Read);
        hres = IInternetProtocolSink_ReportData(pOIProtSink, bscf, 13, 13);
        ok(hres == S_OK, "ReportData failed: %08lx\n", hres);
        CHECK_CALLED(Read);
    }

    if(test_protocol != WINETEST_SYNC_TEST) {
        SET_EXPECT(Terminate);
        hres = IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);
        ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);
        CHECK_CALLED(Terminate);
    }

    return S_OK;
}

static HRESULT WINAPI Protocol_Continue(IInternetProtocol *iface,
        PROTOCOLDATA *pProtocolData)
{
    DWORD bscf = 0;
    HRESULT hres;

    CHECK_EXPECT(Continue);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    if(!bind_to_object)
        ok(reported_url && !lstrcmpW(reported_url, current_url), "wrong url %s\n", wine_dbgstr_w(reported_url));

    ok(pProtocolData != NULL, "pProtocolData == NULL\n");
    if(!pProtocolData)
        return S_OK;

    switch(prot_state) {
    case 0:
        hres = IInternetProtocolSink_ReportProgress(protocol_sink,
                    BINDSTATUS_SENDINGREQUEST, NULL);
        ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);

        hres = IInternetProtocolSink_ReportProgress(protocol_sink,
                BINDSTATUS_MIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
                "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08lx\n", hres);

        bscf |= BSCF_FIRSTDATANOTIFICATION|BSCF_INTERMEDIATEDATANOTIFICATION;
        break;
    case 1: {
        IServiceProvider *service_provider;
        IHttpNegotiate *http_negotiate;
        static const WCHAR header[] = {'?',0};

        hres = IInternetProtocolSink_QueryInterface(protocol_sink, &IID_IServiceProvider,
                                                    (void**)&service_provider);
        ok(hres == S_OK, "Could not get IServiceProvicder\n");

        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
                                             &IID_IHttpNegotiate, (void**)&http_negotiate);
        ok(hres == S_OK, "Could not get IHttpNegotiate\n");

        if(!no_callback) {
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(OnResponse);
        }
        hres = IHttpNegotiate_OnResponse(http_negotiate, 200, header, NULL, NULL);
        if(!no_callback) {
            CHECK_CALLED_BROKEN(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(OnResponse);
        }
        IHttpNegotiate_Release(http_negotiate);
        ok(hres == S_OK, "OnResponse failed: %08lx\n", hres);

        if(test_protocol == HTTPS_TEST || test_redirect) {
            hres = IInternetProtocolSink_ReportProgress(protocol_sink, BINDSTATUS_ACCEPTRANGES, NULL);
            ok(hres == S_OK, "ReportProgress(BINDSTATUS_ACCEPTRANGES) failed: %08lx\n", hres);
        }

        hres = IInternetProtocolSink_ReportProgress(protocol_sink,
                BINDSTATUS_MIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08lx\n", hres);

        hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_CACHEFILENAMEAVAILABLE, use_cache_file ? cache_file_name : cache_fileW);
        ok(hres == S_OK, "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08lx\n", hres);

        bscf |= BSCF_FIRSTDATANOTIFICATION;
        break;
    }
    case 2:
    case 3:
        bscf = BSCF_INTERMEDIATEDATANOTIFICATION;
        break;
    }

    hres = IInternetProtocolSink_ReportData(protocol_sink, bscf, 100, 400);
    ok(hres == S_OK, "ReportData failed: %08lx\n", hres);

    if(prot_state != 2 || !test_abort)
        SET_EXPECT(Read);
    switch(prot_state) {
    case 0:
        hres = IInternetProtocolSink_ReportResult(protocol_sink, S_OK, 0, NULL);
        ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);
        SET_EXPECT(OnProgress_SENDINGREQUEST);
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        SET_EXPECT(LockRequest);
        SET_EXPECT(OnStopBinding);
        break;
    case 1:
        if(bind_to_object) {
            SET_EXPECT(Obj_OnProgress_MIMETYPEAVAILABLE);
            SET_EXPECT(Obj_OnProgress_BEGINDOWNLOADDATA);
            SET_EXPECT(Obj_OnProgress_CLASSIDAVAILABLE);
            SET_EXPECT(Obj_OnProgress_BEGINSYNCOPERATION);
            SET_EXPECT(CreateInstance);
            SET_EXPECT(PutProperty_MIMETYPEPROP);
            SET_EXPECT(PutProperty_CLASSIDPROP);
            SET_EXPECT(Load);
            SET_EXPECT(Obj_OnProgress_ENDSYNCOPERATION);
            SET_EXPECT(OnObjectAvailable);
            SET_EXPECT(Obj_OnStopBinding);
        }else if(!no_callback) {
            SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
            SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
            SET_EXPECT(LockRequest);
        }else {
            SET_EXPECT(LockRequest);
        }
        break;
    case 2:
        if(!no_callback)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        break;
    case 3:
        SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    }
    if(!no_callback) {
        if((!bind_to_object || prot_state >= 2) && (!test_abort || prot_state != 2))
            SET_EXPECT(OnDataAvailable);
        if(prot_state == 3 || (test_abort && prot_state == 2))
            SET_EXPECT(OnStopBinding);
    }
    return S_OK;
}

static HRESULT WINAPI Protocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    HRESULT hres;

    CHECK_EXPECT(Abort);

    ok(hrReason == E_ABORT, "hrReason = %08lx\n", hrReason);
    ok(!dwOptions, "dwOptions = %lx\n", dwOptions);

    hres = IInternetProtocolSink_ReportResult(protocol_sink, E_ABORT, ERROR_SUCCESS, NULL);
    ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);

    return S_OK;
}

static HRESULT WINAPI Protocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    CHECK_EXPECT(Terminate);

    ok(dwOptions == 0, "dwOptions=%ld, expected 0\n", dwOptions);

    if(protocol_sink) {
        IInternetProtocolSink_Release(protocol_sink);
        protocol_sink = NULL;
    }

    if(no_callback)
        SetEvent(complete_event);
    return S_OK;
}

static HRESULT WINAPI Protocol_Suspend(IInternetProtocol *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Resume(IInternetProtocol *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    HRESULT hres;

    static const char data[] = "<HTML></HTML>";

    CHECK_EXPECT2(Read);

    ok(pv != NULL, "pv == NULL\n");
    ok(cb != 0, "cb == 0\n");
    ok(pcbRead != NULL, "pcbRead == NULL\n");

    if(async_switch) {
        if(prot_state++ > 1) {
            *pcbRead = 0;
            return S_FALSE;
        } else {
            memset(pv, '?', cb);
            *pcbRead = cb;
            return S_OK;
        }
    }

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST) {
        static BOOL pending = TRUE;

        pending = !pending;

        switch(prot_state) {
        case 1:
        case 2:
            if(pending) {
                *pcbRead = 10;
                memset(pv, '?', 10);
                if(prot_state == 2 && no_callback)
                    SetEvent(complete_event);
                return E_PENDING;
            }else {
                memset(pv, '?', cb);
                *pcbRead = cb;
                nread++;
                return S_OK;
            }
        case 3:
            prot_state++;

            *pcbRead = 0;

            hres = IInternetProtocolSink_ReportData(protocol_sink,
                    BSCF_LASTDATANOTIFICATION|BSCF_INTERMEDIATEDATANOTIFICATION, 2000, 2000);
            ok(hres == S_OK, "ReportData failed: %08lx\n", hres);

            hres = IInternetProtocolSink_ReportResult(protocol_sink, S_OK, 0, NULL);
            ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);

            return S_FALSE;
        case 4:
            *pcbRead = 0;
            return S_FALSE;
        }
    }

    if(nread) {
        *pcbRead = 0;
        return S_FALSE;
    }

    if(test_protocol == WINETEST_SYNC_TEST) {
        hres = IInternetProtocolSink_ReportResult(protocol_sink, S_OK, 0, NULL);
        ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);

        SET_EXPECT(OnStopBinding);
    }

    ok(*pcbRead == 0, "*pcbRead=%ld, expected 0\n", *pcbRead);
    nread += *pcbRead = sizeof(data)-1;
    memcpy(pv, data, sizeof(data));
    return S_OK;
}

static HRESULT WINAPI Protocol_Seek(IInternetProtocol *iface,
        LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    CHECK_EXPECT(LockRequest);
    if(no_callback)
        SetEvent(complete_event);
    return S_OK;
}

static HRESULT WINAPI Protocol_UnlockRequest(IInternetProtocol *iface)
{
    CHECK_EXPECT(UnlockRequest);
    return S_OK;
}

static const IInternetProtocolVtbl ProtocolVtbl = {
    Protocol_QueryInterface,
    Protocol_AddRef,
    Protocol_Release,
    Protocol_Start,
    Protocol_Continue,
    Protocol_Abort,
    Protocol_Terminate,
    Protocol_Suspend,
    Protocol_Resume,
    Protocol_Read,
    Protocol_Seek,
    Protocol_LockRequest,
    Protocol_UnlockRequest
};

static IInternetProtocol Protocol = { &ProtocolVtbl };

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate2 *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)
            || IsEqualGUID(&IID_IHttpNegotiate, riid)
            || IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate2 *iface)
{
    return 2;
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate2 *iface)
{
    return 1;
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate2 *iface, LPCWSTR szURL,
        LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    if(onsecurityproblem_hres == S_OK)
        CHECK_EXPECT2(BeginningTransaction);
    else
        CHECK_EXPECT(BeginningTransaction);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    ok(!lstrcmpW(szURL, current_url), "szURL != current_url\n");
    ok(!dwReserved, "dwReserved=%ld, expected 0\n", dwReserved);
    ok(pszAdditionalHeaders != NULL, "pszAdditionalHeaders == NULL\n");
    if(pszAdditionalHeaders)
        ok(*pszAdditionalHeaders == NULL, "*pszAdditionalHeaders != NULL\n");

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    CHECK_EXPECT(OnResponse);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    ok(dwResponseCode == 200, "dwResponseCode=%ld, expected 200\n", dwResponseCode);
    ok(szResponseHeaders != NULL, "szResponseHeaders == NULL\n");
    ok(szRequestHeaders == NULL, "szRequestHeaders != NULL\n");
    /* Note: in protocol.c tests, OnResponse pszAdditionalRequestHeaders _is_ NULL */
    ok(pszAdditionalRequestHeaders != NULL, "pszAdditionalHeaders == NULL\n");
    if(pszAdditionalRequestHeaders)
        ok(*pszAdditionalRequestHeaders == NULL, "*pszAdditionalHeaders != NULL\n");

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_GetRootSecurityId(IHttpNegotiate2 *iface,
        BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    static const BYTE sec_id[] = {'h','t','t','p',':','t','e','s','t',1,0,0,0};

    CHECK_EXPECT(GetRootSecurityId);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    ok(!dwReserved, "dwReserved=%Id, expected 0\n", dwReserved);
    ok(pbSecurityId != NULL, "pbSecurityId == NULL\n");
    ok(pcbSecurityId != NULL, "pcbSecurityId == NULL\n");

    if(pbSecurityId == (void*)0xdeadbeef)
        return E_NOTIMPL;

    if(pcbSecurityId) {
        ok(*pcbSecurityId == 512, "*pcbSecurityId=%ld, expected 512\n", *pcbSecurityId);
        *pcbSecurityId = sizeof(sec_id);
    }

    if(pbSecurityId)
        memcpy(pbSecurityId, sec_id, sizeof(sec_id));

    return E_FAIL;
}

static IHttpNegotiate2Vtbl HttpNegotiateVtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse,
    HttpNegotiate_GetRootSecurityId
};

static IHttpNegotiate2 HttpNegotiate = { &HttpNegotiateVtbl };

static HRESULT WINAPI HttpSecurity_QueryInterface(IHttpSecurity *iface, REFIID riid, void **ppv)
{
    ok(0, "Unexpected call\n");
    *ppv = NULL;
    if(IsEqualGUID(&IID_IHttpSecurity, riid) ||
       IsEqualGUID(&IID_IWindowForBindingUI, riid) ||
       IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "Unexpected interface requested.\n");

    return E_NOINTERFACE;
}

static ULONG WINAPI HttpSecurity_AddRef(IHttpSecurity *iface)
{
    return 2;
}

static ULONG WINAPI HttpSecurity_Release(IHttpSecurity *iface)
{
    return 1;
}

static HRESULT WINAPI HttpSecurity_GetWindow(IHttpSecurity *iface, REFGUID rguidReason, HWND *phwnd)
{
    if(IsEqualGUID(rguidReason, &IID_IHttpSecurity))
        CHECK_EXPECT(GetWindow_IHttpSecurity);
    else if(IsEqualGUID(rguidReason, &IID_IWindowForBindingUI))
        CHECK_EXPECT2(GetWindow_IWindowForBindingUI);
    else if(IsEqualGUID(rguidReason, &IID_ICodeInstall))
        CHECK_EXPECT(GetWindow_ICodeInstall);
    else
        ok(0, "Unexpected rguidReason: %s\n", wine_dbgstr_guid(rguidReason));

    *phwnd = NULL;
    return S_OK;
}

static HRESULT WINAPI HttpSecurity_OnSecurityProblem(IHttpSecurity *iface, DWORD dwProblem)
{
    CHECK_EXPECT(OnSecurityProblem);
    if(!security_problem) {
        ok(dwProblem == ERROR_INTERNET_SEC_CERT_CN_INVALID ||
           broken(dwProblem == ERROR_INTERNET_SEC_CERT_ERRORS) /* Some versions of IE6 */,
           "Got problem: %ld\n", dwProblem);
        security_problem = dwProblem;

        if(dwProblem == ERROR_INTERNET_SEC_CERT_ERRORS)
            binding_hres = INET_E_SECURITY_PROBLEM;
    }else
        ok(dwProblem == security_problem, "Got problem: %ld\n", dwProblem);

    return onsecurityproblem_hres;
}

static const IHttpSecurityVtbl HttpSecurityVtbl = {
    HttpSecurity_QueryInterface,
    HttpSecurity_AddRef,
    HttpSecurity_Release,
    HttpSecurity_GetWindow,
    HttpSecurity_OnSecurityProblem
};

static IHttpSecurity HttpSecurity = { &HttpSecurityVtbl };

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    if (winetest_debug > 1)
        trace("IServiceProvider::QueryService(service %s, iid %s)\n",
                debugstr_guid(guidService), debugstr_guid(riid));

    if(IsEqualGUID(&IID_IAuthenticate, guidService)) {
        CHECK_EXPECT(QueryService_IAuthenticate);
        return E_NOTIMPL;
    }

    if(IsEqualGUID(&IID_IInternetProtocol, guidService)) {
        CHECK_EXPECT2(QueryService_IInternetProtocol);
        return E_NOTIMPL;
    }

    if(IsEqualGUID(&IID_IInternetBindInfo, guidService)) {
        CHECK_EXPECT(QueryService_IInternetBindInfo);
        return E_NOTIMPL;
    }

    if(IsEqualGUID(&IID_IWindowForBindingUI, guidService)) {
        CHECK_EXPECT2(QueryService_IWindowForBindingUI);
        *ppv = &HttpSecurity;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IHttpSecurity, guidService)) {
        CHECK_EXPECT(QueryService_IHttpSecurity);
        *ppv = &HttpSecurity;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };

static IBindStatusCallbackEx objbsc;

static void test_WinInetHttpInfo(IWinInetHttpInfo *http_info, DWORD progress)
{
    DWORD status, size;
    HRESULT hres, expect;

    /* QueryInfo changes its behavior during this request */
    if(progress == BINDSTATUS_SENDINGREQUEST)
        return;

    if(test_protocol==FTP_TEST && download_state==BEFORE_DOWNLOAD
            && progress!=BINDSTATUS_MIMETYPEAVAILABLE)
        expect = E_FAIL;
    else if(test_protocol == FTP_TEST)
        expect = S_FALSE;
    else
        expect = S_OK;

    size = sizeof(DWORD);
    hres = IWinInetHttpInfo_QueryInfo(http_info, HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
            &status, &size, NULL, NULL);
    ok(hres == expect || ((progress == BINDSTATUS_COOKIE_SENT || progress == BINDSTATUS_PROXYDETECTING) && hres == S_FALSE),
       "progress %lu: hres = %lx, expected %lx\n", progress, hres, expect);
    if(hres == S_OK) {
        if(download_state == BEFORE_DOWNLOAD && progress != BINDSTATUS_MIMETYPEAVAILABLE && progress != BINDSTATUS_DECODING)
            ok(status == 0, "progress %lu: status = %ld\n", progress, status);
        else
            ok(status == HTTP_STATUS_OK, "progress %lu: status = %ld\n", progress, status);
        ok(size == sizeof(DWORD), "size = %ld\n", size);
    }

    size = sizeof(DWORD);
    hres = IWinInetHttpInfo_QueryOption(http_info, INTERNET_OPTION_HANDLE_TYPE, &status, &size);
    if(test_protocol == FTP_TEST) {
        if(download_state==BEFORE_DOWNLOAD && progress!=BINDSTATUS_MIMETYPEAVAILABLE)
            ok(hres == E_FAIL, "hres = %lx\n", hres);
        else
            ok(hres == S_OK, "hres = %lx\n", hres);

        if(hres == S_OK)
            ok(status == INTERNET_HANDLE_TYPE_FTP_FILE, "status = %ld\n", status);
    } else {
        ok(hres == S_OK, "hres = %lx\n", hres);
        ok(status == INTERNET_HANDLE_TYPE_HTTP_REQUEST, "status = %ld\n", status);
    }
}

static HRESULT WINAPI statusclb_QueryInterface(IBindStatusCallbackEx *iface, REFIID riid, void **ppv)
{
    if (winetest_debug > 1) trace("IBindStatusCallback::QueryInterface(%s)\n", debugstr_guid(riid));

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        CHECK_EXPECT2(QueryInterface_IInternetProtocol);
        if(emulate_protocol) {
            *ppv = &Protocol;
            return S_OK;
        }else {
            return E_NOINTERFACE;
        }
    }else if (IsEqualGUID(&IID_IServiceProvider, riid)) {
        CHECK_EXPECT2(QueryInterface_IServiceProvider);
        *ppv = &ServiceProvider;
        return S_OK;
    }else if (IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        CHECK_EXPECT2(QueryInterface_IHttpNegotiate);
        *ppv = &HttpNegotiate;
        return S_OK;
    }else if (IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        CHECK_EXPECT(QueryInterface_IHttpNegotiate2);
        *ppv = &HttpNegotiate;
        return S_OK;
    }else if (IsEqualGUID(&IID_IAuthenticate, riid)) {
        CHECK_EXPECT(QueryInterface_IAuthenticate);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        if(strict_bsc_qi)
            CHECK_EXPECT2(QueryInterface_IBindStatusCallback);
        *ppv = iface;
        return S_OK;
    }else if(IsEqualGUID(&IID_IBindStatusCallbackHolder, riid)) {
        CHECK_EXPECT2(QueryInterface_IBindStatusCallbackHolder);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IBindStatusCallbackEx, riid)) {
        CHECK_EXPECT(QueryInterface_IBindStatusCallbackEx);
        if(!use_bscex)
            return E_NOINTERFACE;
        *ppv = iface;
        return S_OK;
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        /* TODO */
    }else if(IsEqualGUID(&IID_IWindowForBindingUI, riid)) {
        CHECK_EXPECT2(QueryInterface_IWindowForBindingUI);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IHttpSecurity, riid)) {
        CHECK_EXPECT2(QueryInterface_IHttpSecurity);
        return E_NOINTERFACE;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI statusclb_AddRef(IBindStatusCallbackEx *iface)
{
    return 2;
}

static ULONG WINAPI statusclb_Release(IBindStatusCallbackEx *iface)
{
    return 1;
}

static HRESULT WINAPI statusclb_OnStartBinding(IBindStatusCallbackEx *iface, DWORD dwReserved,
        IBinding *pib)
{
    IWinInetHttpInfo *http_info;
    HRESULT hres;
    IMoniker *mon;
    DWORD res;
    CLSID clsid;
    LPOLESTR res_str;

    if(iface == &objbsc)
        CHECK_EXPECT(Obj_OnStartBinding);
    else
        CHECK_EXPECT(OnStartBinding);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    ok(pib != NULL, "pib should not be NULL\n");
    ok(dwReserved == 0xff, "dwReserved=%lx\n", dwReserved);

    if(pib == (void*)0xdeadbeef)
        return S_OK;

    current_binding = pib;

    hres = IBinding_QueryInterface(pib, &IID_IMoniker, (void**)&mon);
    ok(hres == E_NOINTERFACE, "IBinding should not have IMoniker interface\n");
    if(SUCCEEDED(hres))
        IMoniker_Release(mon);

    hres = IBinding_QueryInterface(pib, &IID_IWinInetHttpInfo, (void**)&http_info);
    ok(hres == E_NOINTERFACE, "Could not get IID_IWinInetHttpInfo: %08lx\n", hres);

    if(0) { /* crashes with native urlmon */
        hres = IBinding_GetBindResult(pib, NULL, &res, &res_str, NULL);
        ok(hres == E_INVALIDARG, "GetBindResult failed: %08lx\n", hres);
    }
    hres = IBinding_GetBindResult(pib, &clsid, NULL, &res_str, NULL);
    ok(hres == E_INVALIDARG, "GetBindResult failed: %08lx\n", hres);
    hres = IBinding_GetBindResult(pib, &clsid, &res, NULL, NULL);
    ok(hres == E_INVALIDARG, "GetBindResult failed: %08lx\n", hres);
    hres = IBinding_GetBindResult(pib, &clsid, &res, &res_str, (void*)0xdeadbeef);
    ok(hres == E_INVALIDARG, "GetBindResult failed: %08lx\n", hres);

    hres = IBinding_GetBindResult(pib, &clsid, &res, &res_str, NULL);
    ok(hres == S_OK, "GetBindResult failed: %08lx, expected S_OK\n", hres);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "incorrect clsid: %s\n", wine_dbgstr_guid(&clsid));
    ok(!res, "incorrect res: %lx\n", res);
    ok(!res_str, "incorrect res_str: %s\n", wine_dbgstr_w(res_str));

    if(abort_start) {
        binding_hres = abort_hres;
        return abort_hres;
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_GetPriority(IBindStatusCallbackEx *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnLowResource(IBindStatusCallbackEx *iface, DWORD reserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnProgress(IBindStatusCallbackEx *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    if (winetest_debug > 1)
        trace("IBindStatusCallbackEx::OnProgress(progress %lu/%lu, code %lu, text %s)\n",
                ulProgress, ulProgressMax, ulStatusCode, debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_FINDINGRESOURCE:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_FINDINGRESOURCE);
        else if(test_protocol == FTP_TEST)
            todo_wine CHECK_EXPECT(OnProgress_FINDINGRESOURCE);
        else if(test_protocol == HTTPS_TEST && !bindtest_flags)
            todo_wine CHECK_EXPECT(OnProgress_FINDINGRESOURCE);
        else
            CHECK_EXPECT(OnProgress_FINDINGRESOURCE);
        if(emulate_protocol && (test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST))
            SetEvent(complete_event);
        break;
    case BINDSTATUS_CONNECTING:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_CONNECTING);
        else if(test_protocol == FTP_TEST)
            todo_wine CHECK_EXPECT(OnProgress_CONNECTING);
        else if(onsecurityproblem_hres == S_OK)
            CHECK_EXPECT2(OnProgress_CONNECTING);
        else
            CHECK_EXPECT(OnProgress_CONNECTING);
        if(emulate_protocol && (test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST))
            SetEvent(complete_event);
        break;
    case BINDSTATUS_REDIRECTING:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_REDIRECTING);
        else
            CHECK_EXPECT(OnProgress_REDIRECTING);
        ok(!lstrcmpW(szStatusText, winetest_data_urlW), "unexpected status text %s\n",
           wine_dbgstr_w(szStatusText));
        if(emulate_protocol && (test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST)
                && (!bind_to_object || iface == &objbsc))
            SetEvent(complete_event);
        break;
    case BINDSTATUS_SENDINGREQUEST:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_SENDINGREQUEST);
        else if(test_protocol == FTP_TEST)
            CHECK_EXPECT2(OnProgress_SENDINGREQUEST);
        else
            CHECK_EXPECT(OnProgress_SENDINGREQUEST);
        if(emulate_protocol && (test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST))
            SetEvent(complete_event);

        if(abort_progress) {
            if(filedwl_api)
                binding_hres = E_ABORT;
            return E_ABORT;
        }

        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_MIMETYPEAVAILABLE);
        else
            CHECK_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        if(!bind_to_object)
            ok(download_state == BEFORE_DOWNLOAD, "Download state was %d, expected BEFORE_DOWNLOAD\n",
               download_state);
        WideCharToMultiByte(CP_ACP, 0, szStatusText, -1, mime_type, sizeof(mime_type)-1, NULL, NULL);
        break;
    case BINDSTATUS_BEGINDOWNLOADDATA:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_BEGINDOWNLOADDATA);
        else
            CHECK_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText) {
            if(filedwl_api) {
                /* FIXME */
            }else {
                ok(!lstrcmpW(szStatusText, current_url), "wrong szStatusText %s\n", wine_dbgstr_w(szStatusText));
            }
        }
        if(!bind_to_object)
            ok(download_state == BEFORE_DOWNLOAD, "Download state was %d, expected BEFORE_DOWNLOAD\n",
               download_state);
        download_state = DOWNLOADING;
        break;
    case BINDSTATUS_DOWNLOADINGDATA:
        CHECK_EXPECT2(OnProgress_DOWNLOADINGDATA);
        ok(iface != &objbsc, "unexpected call\n");
        ok(download_state == DOWNLOADING, "Download state was %d, expected DOWNLOADING\n",
           download_state);
        if(test_abort) {
            HRESULT hres;

            SET_EXPECT(Abort);
            hres = IBinding_Abort(current_binding);
            ok(hres == S_OK, "Abort failed: %08lx\n", hres);
            CHECK_CALLED(Abort);

            hres = IBinding_Abort(current_binding);
            ok(hres == E_FAIL, "Abort failed: %08lx\n", hres);

            binding_hres = E_ABORT;
        }
        break;
    case BINDSTATUS_ENDDOWNLOADDATA:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_ENDDOWNLOADDATA);
        else
            CHECK_EXPECT(OnProgress_ENDDOWNLOADDATA);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText) {
            if(filedwl_api) {
                /* FIXME */
            }else {
                ok(!lstrcmpW(szStatusText, current_url), "wrong szStatusText %s\n", wine_dbgstr_w(szStatusText));
            }
        }
        ok(download_state == DOWNLOADING, "Download state was %d, expected DOWNLOADING\n",
           download_state);
        download_state = END_DOWNLOAD;
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        if(test_protocol != HTTP_TEST && test_protocol != HTTPS_TEST && test_protocol != WINETEST_TEST) {
            if(iface == &objbsc)
                CHECK_EXPECT(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
            else
                CHECK_EXPECT(OnProgress_CACHEFILENAMEAVAILABLE);
        }else {  /* FIXME */
            CLEAR_CALLED(OnProgress_CACHEFILENAMEAVAILABLE);
            CLEAR_CALLED(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
        }

        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText && test_protocol == FILE_TEST)
            ok(!lstrcmpW(file_url+7, szStatusText), "wrong szStatusText %s\n", wine_dbgstr_w(szStatusText));
        break;
    case BINDSTATUS_CLASSIDAVAILABLE:
    {
        CLSID clsid;
        HRESULT hr;
        if(iface != &objbsc)
            ok(0, "unexpected call\n");
        else
            CHECK_EXPECT(Obj_OnProgress_CLASSIDAVAILABLE);
        hr = CLSIDFromString((LPCOLESTR)szStatusText, &clsid);
        ok(hr == S_OK, "CLSIDFromString failed with error 0x%08lx\n", hr);
        ok(IsEqualCLSID(&clsid, &CLSID_HTMLDocument),
            "Expected clsid to be CLSID_HTMLDocument instead of %s\n", wine_dbgstr_guid(&clsid));
        break;
    }
    case BINDSTATUS_BEGINSYNCOPERATION:
        CHECK_EXPECT(Obj_OnProgress_BEGINSYNCOPERATION);
        if(iface != &objbsc)
            ok(0, "unexpected call\n");
        ok(szStatusText == NULL, "Expected szStatusText to be NULL\n");
        break;
    case BINDSTATUS_ENDSYNCOPERATION:
        CHECK_EXPECT(Obj_OnProgress_ENDSYNCOPERATION);
        if(iface != &objbsc)
            ok(0, "unexpected call\n");
        ok(szStatusText == NULL, "Expected szStatusText to be NULL\n");
        break;
    case BINDSTATUS_PROXYDETECTING:
    case BINDSTATUS_COOKIE_SENT:
    case BINDSTATUS_DECODING:
        break;
    default:
        ok(0, "unexpected code %ld\n", ulStatusCode);
    };

    if(current_binding) {
        IWinInetHttpInfo *http_info;
        HRESULT hres;

        hres = IBinding_QueryInterface(current_binding, &IID_IWinInetHttpInfo, (void**)&http_info);
        if(!emulate_protocol && test_protocol != FILE_TEST && is_urlmon_protocol(test_protocol)) {
            ok(hres == S_OK, "Could not get IWinInetHttpInfo iface: %08lx\n", hres);
            test_WinInetHttpInfo(http_info, ulStatusCode);
        } else
            ok(hres == E_NOINTERFACE,
               "QueryInterface(IID_IWinInetHttpInfo) returned: %08lx, expected E_NOINTERFACE\n", hres);
        if(SUCCEEDED(hres))
            IWinInetHttpInfo_Release(http_info);
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_OnStopBinding(IBindStatusCallbackEx *iface, HRESULT hresult, LPCWSTR szError)
{
    if(iface == &objbsc) {
        CHECK_EXPECT(Obj_OnStopBinding);
        stopped_obj_binding = TRUE;
    }else {
        CHECK_EXPECT(OnStopBinding);
        stopped_binding = TRUE;
    }

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    if(only_check_prot_args) {
        todo_wine ok(hresult == S_OK, "Got %08lx\n", hresult);
        return S_OK;
    }

    /* ignore DNS failure */
    if (hresult == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
        return S_OK;

    if(filedwl_api) {
        if(!abort_progress && !abort_start)
            ok(SUCCEEDED(hresult), "binding failed: %08lx\n", hresult);
        else if(abort_start && abort_hres == E_NOTIMPL)
            todo_wine ok(hresult == S_FALSE, "binding failed: %08lx, expected S_FALSE\n", hresult);
        else
            ok(hresult == E_ABORT, "binding failed: %08lx, expected E_ABORT\n", hresult);
    } else
        ok(hresult == binding_hres, "binding failed: %08lx, expected %08lx\n", hresult, binding_hres);
    ok(szError == NULL, "szError should be NULL\n");

    if(current_binding) {
        CLSID clsid;
        DWORD res;
        LPOLESTR res_str;
        HRESULT hres;

        hres = IBinding_GetBindResult(current_binding, &clsid, &res, &res_str, NULL);
        ok(hres == S_OK, "GetBindResult failed: %08lx, expected S_OK\n", hres);
        ok(res == hresult, "res = %08lx, expected %08lx\n", res, binding_hres);
        ok(!res_str, "incorrect res_str = %s\n", wine_dbgstr_w(res_str));

        if(hresult==S_OK || (abort_start && hresult!=S_FALSE) || hresult == REGDB_E_CLASSNOTREG) {
            ok(IsEqualCLSID(&clsid, &CLSID_NULL),
                    "incorrect protocol CLSID: %s, expected CLSID_NULL\n",
                    wine_dbgstr_guid(&clsid));
        }else if(emulate_protocol) {
            todo_wine ok(IsEqualCLSID(&clsid, &CLSID_FtpProtocol),
                    "incorrect protocol CLSID: %s, expected CLSID_FtpProtocol\n",
                    wine_dbgstr_guid(&clsid));
        }else if(test_protocol == FTP_TEST) {
            ok(IsEqualCLSID(&clsid, &CLSID_FtpProtocol),
                    "incorrect protocol CLSID: %s, expected CLSID_FtpProtocol\n",
                    wine_dbgstr_guid(&clsid));
        }else if(test_protocol == FILE_TEST) {
            ok(IsEqualCLSID(&clsid, &CLSID_FileProtocol),
                    "incorrect protocol CLSID: %s, expected CLSID_FileProtocol\n",
                    wine_dbgstr_guid(&clsid));
        }else if(test_protocol == HTTP_TEST) {
            ok(IsEqualCLSID(&clsid, &CLSID_HttpProtocol),
                    "incorrect protocol CLSID: %s, expected CLSID_HttpProtocol\n",
                    wine_dbgstr_guid(&clsid));
        }else if(test_protocol == HTTPS_TEST) {
            ok(IsEqualCLSID(&clsid, &CLSID_HttpSProtocol),
                    "incorrect protocol CLSID: %s, expected CLSID_HttpSProtocol\n",
                    wine_dbgstr_guid(&clsid));
        }else if(test_protocol == ABOUT_TEST) {
            ok(IsEqualCLSID(&clsid, &CLSID_AboutProtocol),
                    "incorrect protocol CLSID: %s, expected CLSID_AboutProtocol\n",
                    wine_dbgstr_guid(&clsid));
        }else {
            ok(0, "unexpected (%d)\n", test_protocol);
        }
    }

    if((test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST) && emulate_protocol) {
        SetEvent(complete_event);
        if(iface != &objbsc)
            ok( WaitForSingleObject(complete_event2, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
    }

    if(test_protocol == HTTP_TEST && !emulate_protocol && http_cache_file[0]) {
        HANDLE file = CreateFileW(http_cache_file, DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file == INVALID_HANDLE_VALUE, "expected INVALID_HANDLE_VALUE, got %p\n", file);
        ok(GetLastError() == ERROR_SHARING_VIOLATION, "expected ERROR_SHARING_VIOLATION, got %lu\n", GetLastError());
        http_cache_file[0] = 0;
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_GetBindInfo(IBindStatusCallbackEx *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DWORD cbSize;

    if(iface == &objbsc)
        CHECK_EXPECT(Obj_GetBindInfo);
    else
        CHECK_EXPECT(GetBindInfo);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    *grfBINDF = bindf;
    cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize);
    pbindinfo->cbSize = cbSize;

    return S_OK;
}

static void test_stream_seek(IStream *stream)
{
    ULARGE_INTEGER new_pos;
    LARGE_INTEGER pos;
    HRESULT hres;

    pos.QuadPart = 0;
    new_pos.QuadPart = 0xdeadbeef;
    hres = IStream_Seek(stream, pos, STREAM_SEEK_SET, &new_pos);
    ok(hres == S_OK, "Seek failed: %08lx\n", hres);
    ok(!new_pos.QuadPart, "new_pos.QuadPart != 0\n");

    pos.QuadPart = 0;
    new_pos.QuadPart = 0xdeadbeef;
    hres = IStream_Seek(stream, pos, STREAM_SEEK_END, &new_pos);
    ok(hres == S_OK, "Seek failed: %08lx\n", hres);
    ok(new_pos.QuadPart, "new_pos.QuadPart = 0\n");

    pos.QuadPart = 0;
    new_pos.QuadPart = 0xdeadbeef;
    hres = IStream_Seek(stream, pos, 100, &new_pos);
    ok(hres == E_FAIL, "Seek failed: %08lx\n", hres);
    ok(new_pos.QuadPart == 0xdeadbeef, "unexpected new_pos.QuadPart\n");
}

static HRESULT WINAPI statusclb_OnDataAvailable(IBindStatusCallbackEx *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    HRESULT hres;
    DWORD read;
    BYTE buf[512];
    CHAR clipfmt[512];

    if(iface == &objbsc)
        ok(0, "unexpected call\n");

    CHECK_EXPECT2(OnDataAvailable);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    ok(download_state == DOWNLOADING || download_state == END_DOWNLOAD,
       "Download state was %d, expected DOWNLOADING or END_DOWNLOAD\n",
       download_state);
    data_available = TRUE;

    if(bind_to_object && !is_async_prot)
        ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION), "grfBSCF = %lx\n", grfBSCF);

    ok(pformatetc != NULL, "pformatetc == NULL\n");
    if(pformatetc) {
        if (mime_type[0]) {
            INT ret;
            clipfmt[0] = 0;
            ret = GetClipboardFormatNameA(pformatetc->cfFormat, clipfmt, sizeof(clipfmt)-1);
            ok(ret, "GetClipboardFormatName failed, error %ld\n", GetLastError());
            ok(!strcmp(clipfmt, mime_type), "clipformat %x != mime_type, \"%s\" != \"%s\"\n",
               pformatetc->cfFormat, clipfmt, mime_type);
        } else {
            ok(pformatetc->cfFormat == 0, "clipformat=%x\n", pformatetc->cfFormat);
        }
        ok(pformatetc->ptd == NULL, "ptd = %p\n", pformatetc->ptd);
        ok(pformatetc->dwAspect == 1, "dwAspect=%lu\n", pformatetc->dwAspect);
        ok(pformatetc->lindex == -1, "lindex=%ld\n", pformatetc->lindex);
        ok(pformatetc->tymed == tymed, "tymed=%lu, expected %lu\n", pformatetc->tymed, tymed);
    }

    ok(pstgmed != NULL, "stgmeg == NULL\n");
    ok(pstgmed->tymed == tymed, "tymed=%lu, expected %lu\n", pstgmed->tymed, tymed);
    ok(pstgmed->pUnkForRelease != NULL, "pUnkForRelease == NULL\n");

    switch(pstgmed->tymed) {
    case TYMED_ISTREAM: {
        IStream *stream = pstgmed->pstm;

        ok(stream != NULL, "pstgmed->pstm == NULL\n");

        if(grfBSCF & BSCF_FIRSTDATANOTIFICATION) {
            STATSTG stat;

            hres = IStream_Write(stream, buf, 10, NULL);
            ok(hres == STG_E_ACCESSDENIED,
               "Write failed: %08lx, expected STG_E_ACCESSDENIED\n", hres);

            hres = IStream_Commit(stream, 0);
            ok(hres == E_NOTIMPL, "Commit failed: %08lx, expected E_NOTIMPL\n", hres);

            hres = IStream_Revert(stream);
            ok(hres == E_NOTIMPL, "Revert failed: %08lx, expected E_NOTIMPL\n", hres);

            hres = IStream_Stat(stream, NULL, STATFLAG_NONAME);
            ok(hres == E_FAIL, "hres = %lx\n", hres);
            if(use_cache_file && emulate_protocol) {
                hres = IStream_Stat(stream, &stat, STATFLAG_DEFAULT);
                ok(hres == S_OK, "hres = %lx\n", hres);
                ok(!lstrcmpW(stat.pwcsName, cache_file_name),
                        "stat.pwcsName = %s, cache_file_name = %s\n",
                        wine_dbgstr_w(stat.pwcsName), wine_dbgstr_w(cache_file_name));
                CoTaskMemFree(stat.pwcsName);
                ok(stat.cbSize.LowPart == (bindf&BINDF_ASYNCHRONOUS?0:6500),
                        "stat.cbSize.LowPart = %lu\n", stat.cbSize.LowPart);
            } else {
                hres = IStream_Stat(stream, &stat, STATFLAG_NONAME);
                ok(hres == S_OK, "hres = %lx\n", hres);
                ok(!stat.pwcsName || broken(stat.pwcsName!=NULL),
                        "stat.pwcsName = %s\n", wine_dbgstr_w(stat.pwcsName));
            }
            ok(stat.type == STGTY_STREAM, "stat.type = %lx\n", stat.type);
            ok(stat.cbSize.HighPart == 0, "stat.cbSize.HighPart != 0\n");
            ok(stat.grfMode == (stat.cbSize.LowPart?GENERIC_READ:0), "stat.grfMode = %lx\n", stat.grfMode);
            ok(stat.grfLocksSupported == 0, "stat.grfLocksSupported = %lx\n", stat.grfLocksSupported);
            ok(stat.grfStateBits == 0, "stat.grfStateBits = %lx\n", stat.grfStateBits);
            ok(stat.reserved == 0, "stat.reserved = %lx\n", stat.reserved);
        }

        if(callback_read) {
            do {
                hres = IStream_Read(stream, buf, 512, &read);
                if(test_protocol == HTTP_TEST && emulate_protocol && read)
                    ok(buf[0] == (use_cache_file && !(bindf&BINDF_ASYNCHRONOUS) ? 'X' : '?'), "buf[0] = '%c'\n", buf[0]);
            }while(hres == S_OK);
            ok(hres == S_FALSE || hres == E_PENDING, "IStream_Read returned %08lx\n", hres);
        }

        if(use_cache_file && (grfBSCF & BSCF_FIRSTDATANOTIFICATION) && !(bindf & BINDF_PULLDATA))
            test_stream_seek(stream);
        break;
    }
    case TYMED_FILE:
        if(test_protocol == FILE_TEST)
            ok(!lstrcmpW(pstgmed->lpszFileName, file_url+7),
               "unexpected file name %s\n", wine_dbgstr_w(pstgmed->lpszFileName));
        else if(emulate_protocol)
            ok(!lstrcmpW(pstgmed->lpszFileName, cache_fileW),
               "unexpected file name %s\n", wine_dbgstr_w(pstgmed->lpszFileName));
        else if(test_protocol == HTTP_TEST)
            lstrcpyW(http_cache_file, pstgmed->lpszFileName);
        else
            ok(pstgmed->lpszFileName != NULL, "lpszFileName == NULL\n");
    }

    if((test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST)
       && emulate_protocol && prot_state < 4 && (!bind_to_object || prot_state > 1))
        SetEvent(complete_event);

    return S_OK;
}

static HRESULT WINAPI statusclb_OnObjectAvailable(IBindStatusCallbackEx *iface, REFIID riid, IUnknown *punk)
{
    CHECK_EXPECT(OnObjectAvailable);

    if(iface != &objbsc)
        ok(0, "unexpected call\n");

    ok(IsEqualGUID(&IID_IUnknown, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok(punk != NULL, "punk == NULL\n");

    return S_OK;
}

static HRESULT WINAPI statusclb_GetBindInfoEx(IBindStatusCallbackEx *iface, DWORD *grfBINDF, BINDINFO *pbindinfo,
        DWORD *grfBINDF2, DWORD *pdwReserved)
{
    CHECK_EXPECT(GetBindInfoEx);

    ok(grfBINDF != NULL, "grfBINDF == NULL\n");
    ok(grfBINDF2 != NULL, "grfBINDF2 == NULL\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pdwReserved != NULL, "dwReserved == NULL\n");

    return S_OK;
}

static const IBindStatusCallbackExVtbl BindStatusCallbackVtbl = {
    statusclb_QueryInterface,
    statusclb_AddRef,
    statusclb_Release,
    statusclb_OnStartBinding,
    statusclb_GetPriority,
    statusclb_OnLowResource,
    statusclb_OnProgress,
    statusclb_OnStopBinding,
    statusclb_GetBindInfo,
    statusclb_OnDataAvailable,
    statusclb_OnObjectAvailable,
    statusclb_GetBindInfoEx
};

static IBindStatusCallbackEx bsc = { &BindStatusCallbackVtbl };
static IBindStatusCallbackEx bsc2 = { &BindStatusCallbackVtbl };
static IBindStatusCallbackEx objbsc = { &BindStatusCallbackVtbl };

static HRESULT WINAPI MonikerProp_QueryInterface(IMonikerProp *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MonikerProp_AddRef(IMonikerProp *iface)
{
    return 2;
}

static ULONG WINAPI MonikerProp_Release(IMonikerProp *iface)
{
    return 1;
}

static HRESULT WINAPI MonikerProp_PutProperty(IMonikerProp *iface, MONIKERPROPERTY mkp, LPCWSTR val)
{
    switch(mkp) {
    case MIMETYPEPROP:
        CHECK_EXPECT(PutProperty_MIMETYPEPROP);
        ok(!lstrcmpW(val, wszTextHtml), "val = %s\n", wine_dbgstr_w(val));
        break;
    case CLASSIDPROP:
        CHECK_EXPECT(PutProperty_CLASSIDPROP);
        break;
    default:
        break;
    }

    return S_OK;
}

static const IMonikerPropVtbl MonikerPropVtbl = {
    MonikerProp_QueryInterface,
    MonikerProp_AddRef,
    MonikerProp_Release,
    MonikerProp_PutProperty
};

static IMonikerProp MonikerProp = { &MonikerPropVtbl };

static HRESULT WINAPI PersistMoniker_QueryInterface(IPersistMoniker *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IPersistMoniker, riid))
        *ppv = iface;
    else if(IsEqualGUID(&IID_IMonikerProp, riid))
        *ppv = &MonikerProp;

    if(*ppv)
        return S_OK;

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI PersistMoniker_AddRef(IPersistMoniker *iface)
{
    return 2;
}

static ULONG WINAPI PersistMoniker_Release(IPersistMoniker *iface)
{
    return 1;
}

static HRESULT WINAPI PersistMoniker_GetClassID(IPersistMoniker *iface, CLSID *pClassID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMoniker_IsDirty(IPersistMoniker *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMoniker_Load(IPersistMoniker *iface, BOOL fFullyAvailable,
                                          IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    IUnknown *unk;
    HRESULT hres;

    static WCHAR cbinding_contextW[] =
        {'C','B','i','n','d','i','n','g',' ','C','o','n','t','e','x','t',0};

    CHECK_EXPECT(Load);
    ok(GetCurrentThreadId() == thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
        ok(!fFullyAvailable, "fFullyAvailable = %x\n", fFullyAvailable);
    else
        ok(fFullyAvailable, "fFullyAvailable = %x\n", fFullyAvailable);
    ok(pimkName != NULL, "pimkName == NULL\n");
    ok(pibc != NULL, "pibc == NULL\n");
    ok(grfMode == 0x12, "grfMode = %lx\n", grfMode);

    hres = IBindCtx_GetObjectParam(pibc, cbinding_contextW, &unk);
    ok(hres == S_OK, "GetObjectParam(CBinding Context) failed: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        IBinding *binding;

        hres = IUnknown_QueryInterface(unk, &IID_IBinding, (void**)&binding);
        ok(hres == S_OK, "Could not get IBinding: %08lx\n", hres);

        IBinding_Release(binding);
        IUnknown_Release(unk);
    }

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = RegisterBindStatusCallback(pibc, (IBindStatusCallback*)&bsc, NULL, 0);
    ok(hres == S_OK, "RegisterBindStatusCallback failed: %08lx\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    SET_EXPECT(QueryInterface_IBindStatusCallbackEx);
    SET_EXPECT(GetBindInfo);
    SET_EXPECT(OnStartBinding);
    if(test_redirect)
        SET_EXPECT(OnProgress_REDIRECTING);
    SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
    if(test_protocol == FILE_TEST)
        SET_EXPECT(OnProgress_CACHEFILENAMEAVAILABLE);
    if(test_protocol != HTTP_TEST && test_protocol != HTTPS_TEST)
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    SET_EXPECT(LockRequest);
    SET_EXPECT(OnDataAvailable);
    if(test_protocol != HTTP_TEST && test_protocol != HTTPS_TEST)
        SET_EXPECT(OnStopBinding);

    hres = IMoniker_BindToStorage(pimkName, pibc, NULL, &IID_IStream, (void**)&unk);
    ok(hres == S_OK, "Load failed: %08lx\n", hres);

    CLEAR_CALLED(QueryInterface_IBindStatusCallbackEx); /* IE 8 */
    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(OnStartBinding);
    if(test_redirect)
        CHECK_CALLED(OnProgress_REDIRECTING);
    CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
    if(test_protocol == FILE_TEST)
        CHECK_CALLED(OnProgress_CACHEFILENAMEAVAILABLE);
    if(test_protocol != HTTP_TEST && test_protocol != HTTPS_TEST)
        CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
    CHECK_CALLED(LockRequest);
    CHECK_CALLED(OnDataAvailable);
    if(test_protocol != HTTP_TEST && test_protocol != HTTPS_TEST)
        CHECK_CALLED(OnStopBinding);

    if(unk)
        IUnknown_Release(unk);

    return S_OK;
}

static HRESULT WINAPI PersistMoniker_Save(IPersistMoniker *iface, IMoniker *pimkName, LPBC pbc, BOOL fRemember)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMoniker_SaveCompleted(IPersistMoniker *iface, IMoniker *pimkName, LPBC pibc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMoniker_GetCurMoniker(IPersistMoniker *iface, IMoniker **pimkName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IPersistMonikerVtbl PersistMonikerVtbl = {
    PersistMoniker_QueryInterface,
    PersistMoniker_AddRef,
    PersistMoniker_Release,
    PersistMoniker_GetClassID,
    PersistMoniker_IsDirty,
    PersistMoniker_Load,
    PersistMoniker_Save,
    PersistMoniker_SaveCompleted,
    PersistMoniker_GetCurMoniker
};

static IPersistMoniker PersistMoniker = { &PersistMonikerVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IMarshal, riid))
        return E_NOINTERFACE;
    if(IsEqualGUID(&CLSID_IdentityUnmarshal, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CHECK_EXPECT(CreateInstance);
    ok(!outer, "outer = %p\n", outer);
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = &PersistMoniker;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory mime_cf = { &ClassFactoryVtbl };

static HRESULT WINAPI ProtocolCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IInternetProtocolInfo, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolCF_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IInternetProtocolInfo, riid))
        return E_NOINTERFACE;

    ok(outer != NULL, "outer == NULL\n");
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = &Protocol;
    return S_OK;
}

static const IClassFactoryVtbl ProtocolCFVtbl = {
    ProtocolCF_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ProtocolCF_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory protocol_cf = { &ProtocolCFVtbl };

static void test_CreateAsyncBindCtx(void)
{
    IBindCtx *bctx = (IBindCtx*)0x0ff00ff0;
    IUnknown *unk;
    HRESULT hres;
    ULONG ref;
    BIND_OPTS bindopts;

    hres = CreateAsyncBindCtx(0, NULL, NULL, &bctx);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed. expected: E_INVALIDARG, got: %08lx\n", hres);
    ok(bctx == (IBindCtx*)0x0ff00ff0, "bctx should not be changed\n");

    hres = CreateAsyncBindCtx(0, NULL, NULL, NULL);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed. expected: E_INVALIDARG, got: %08lx\n", hres);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, (IBindStatusCallback*)&bsc, NULL, &bctx);
    ok(hres == S_OK, "CreateAsyncBindCtx failed: %08lx\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    bindopts.cbStruct = sizeof(bindopts);
    hres = IBindCtx_GetBindOptions(bctx, &bindopts);
    ok(hres == S_OK, "IBindCtx_GetBindOptions failed: %08lx\n", hres);
    ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08lx, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
    ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08lx, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
    ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08lx, expected: 0\n", bindopts.dwTickCountDeadline);

    hres = IBindCtx_QueryInterface(bctx, &IID_IAsyncBindCtx, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IAsyncBindCtx) failed: %08lx, expected E_NOINTERFACE\n", hres);
    if(SUCCEEDED(hres))
        IUnknown_Release(unk);

    ref = IBindCtx_Release(bctx);
    ok(ref == 0, "bctx should be destroyed here\n");
}

static void test_CreateAsyncBindCtxEx(void)
{
    IBindCtx *bctx = NULL, *bctx2 = NULL, *bctx_arg = NULL;
    IUnknown *unk;
    BIND_OPTS bindopts;
    HRESULT hres;

    static WCHAR testW[] = {'t','e','s','t',0};

    if (!pCreateAsyncBindCtxEx) {
        win_skip("CreateAsyncBindCtxEx not present\n");
        return;
    }

    hres = pCreateAsyncBindCtxEx(NULL, 0, NULL, NULL, NULL, 0);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = pCreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        bindopts.cbStruct = sizeof(bindopts);
        hres = IBindCtx_GetBindOptions(bctx, &bindopts);
        ok(hres == S_OK, "IBindCtx_GetBindOptions failed: %08lx\n", hres);
        ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08lx, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
        ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08lx, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
        ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08lx, expected: 0\n", bindopts.dwTickCountDeadline);

        IBindCtx_Release(bctx);
    }

    CreateBindCtx(0, &bctx_arg);
    hres = pCreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        bindopts.cbStruct = sizeof(bindopts);
        hres = IBindCtx_GetBindOptions(bctx, &bindopts);
        ok(hres == S_OK, "IBindCtx_GetBindOptions failed: %08lx\n", hres);
        ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08lx, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
        ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08lx, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
        ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08lx, expected: 0\n", bindopts.dwTickCountDeadline);

        IBindCtx_Release(bctx);
    }

    IBindCtx_Release(bctx_arg);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = pCreateAsyncBindCtxEx(NULL, 0, (IBindStatusCallback*)&bsc, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08lx\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    hres = IBindCtx_QueryInterface(bctx, &IID_IAsyncBindCtx, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IAsyncBindCtx) failed: %08lx\n", hres);
    if(SUCCEEDED(hres))
        IUnknown_Release(unk);

    IBindCtx_Release(bctx);

    hres = CreateBindCtx(0, &bctx2);
    ok(hres == S_OK, "CreateBindCtx failed: %08lx\n", hres);

    hres = pCreateAsyncBindCtxEx(bctx2, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08lx\n", hres);

    hres = IBindCtx_RegisterObjectParam(bctx2, testW, (IUnknown*)&Protocol);
    ok(hres == S_OK, "RegisterObjectParam failed: %08lx\n", hres);

    hres = IBindCtx_GetObjectParam(bctx, testW, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08lx\n", hres);
    ok(unk == (IUnknown*)&Protocol, "unexpected unk %p\n", unk);

    IBindCtx_Release(bctx);
    IBindCtx_Release(bctx2);
}

static void test_GetBindInfoEx(IBindStatusCallback *holder)
{
    IBindStatusCallbackEx *bscex;
    BINDINFO bindinfo = {sizeof(bindinfo)};
    DWORD bindf, bindf2, dw;
    HRESULT hres;

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IBindStatusCallbackEx, (void**)&bscex);
    if(FAILED(hres)) {
        win_skip("IBindStatusCallbackEx not supported\n");
        return;
    }

    use_bscex = TRUE;

    bindf = 0;
    SET_EXPECT(QueryInterface_IBindStatusCallbackEx);
    SET_EXPECT(GetBindInfoEx);
    hres = IBindStatusCallback_GetBindInfo(holder, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    CHECK_CALLED(QueryInterface_IBindStatusCallbackEx);
    CHECK_CALLED(GetBindInfoEx);

    bindf = bindf2 = dw = 0;
    SET_EXPECT(QueryInterface_IBindStatusCallbackEx);
    SET_EXPECT(GetBindInfoEx);
    hres = IBindStatusCallbackEx_GetBindInfoEx(bscex, &bindf, &bindinfo, &bindf2, &dw);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    CHECK_CALLED(QueryInterface_IBindStatusCallbackEx);
    CHECK_CALLED(GetBindInfoEx);

    use_bscex = FALSE;

    bindf = bindf2 = dw = 0xdeadbeef;
    SET_EXPECT(QueryInterface_IBindStatusCallbackEx);
    SET_EXPECT(GetBindInfo);
    hres = IBindStatusCallbackEx_GetBindInfoEx(bscex, &bindf, &bindinfo, &bindf2, &dw);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    CHECK_CALLED(QueryInterface_IBindStatusCallbackEx);
    CHECK_CALLED(GetBindInfo);
    ok(bindf2 == 0xdeadbeef, "bindf2 = %lx\n", bindf2);
    ok(dw == 0xdeadbeef, "dw = %lx\n", dw);

    IBindStatusCallbackEx_Release(bscex);
}

static BOOL test_bscholder(IBindStatusCallback *holder)
{
    IServiceProvider *serv_prov;
    IHttpNegotiate *http_negotiate, *http_negotiate_serv;
    IHttpNegotiate2 *http_negotiate2, *http_negotiate2_serv;
    IAuthenticate *authenticate, *authenticate_serv;
    IInternetBindInfo *bind_info;
    IInternetProtocol *protocol;
    BINDINFO bindinfo = {sizeof(bindinfo)};
    BOOL ret = TRUE;
    LPWSTR wstr;
    DWORD dw;
    HRESULT hres;

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IServiceProvider, (void**)&serv_prov);
    ok(hres == S_OK, "Could not get IServiceProvider interface: %08lx\n", hres);

    dw = 0xdeadbeef;
    SET_EXPECT(QueryInterface_IBindStatusCallbackEx);
    SET_EXPECT(GetBindInfo);
    hres = IBindStatusCallback_GetBindInfo(holder, &dw, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    CLEAR_CALLED(QueryInterface_IBindStatusCallbackEx); /* IE 8 */
    CHECK_CALLED(GetBindInfo);

    test_GetBindInfoEx(holder);

    SET_EXPECT(OnStartBinding);
    hres = IBindStatusCallback_OnStartBinding(holder, 0, (void*)0xdeadbeef);
    ok(hres == S_OK, "OnStartBinding failed: %08lx\n", hres);
    CHECK_CALLED(OnStartBinding);

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IHttpNegotiate, (void**)&http_negotiate);
    ok(hres == S_OK, "Could not get IHttpNegotiate interface: %08lx\n", hres);

    SET_EXPECT(QueryInterface_IHttpNegotiate);
    hres = IServiceProvider_QueryService(serv_prov, &IID_IHttpNegotiate, &IID_IHttpNegotiate,
                                         (void**)&http_negotiate_serv);
    ok(hres == S_OK, "Could not get IHttpNegotiate service: %08lx\n", hres);
    CLEAR_CALLED(QueryInterface_IHttpNegotiate); /* IE <8 */

    ok(http_negotiate == http_negotiate_serv, "http_negotiate != http_negotiate_serv\n");

    wstr = (void*)0xdeadbeef;
    SET_EXPECT(QueryInterface_IHttpNegotiate);
    SET_EXPECT(BeginningTransaction);
    hres = IHttpNegotiate_BeginningTransaction(http_negotiate_serv, current_url, emptyW, 0, &wstr);
    CHECK_CALLED_BROKEN(QueryInterface_IHttpNegotiate); /* IE8 */
    CHECK_CALLED(BeginningTransaction);
    ok(hres == S_OK, "BeginningTransaction failed: %08lx\n", hres);
    ok(wstr == NULL, "wstr = %p\n", wstr);

    IHttpNegotiate_Release(http_negotiate_serv);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IHttpNegotiate, &IID_IHttpNegotiate,
                                         (void**)&http_negotiate_serv);
    ok(hres == S_OK, "Could not get IHttpNegotiate service: %08lx\n", hres);
    ok(http_negotiate == http_negotiate_serv, "http_negotiate != http_negotiate_serv\n");
    IHttpNegotiate_Release(http_negotiate_serv);

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IHttpNegotiate2, (void**)&http_negotiate2);
    if(SUCCEEDED(hres)) {
        have_IHttpNegotiate2 = TRUE;

        SET_EXPECT(QueryInterface_IHttpNegotiate2);
        hres = IServiceProvider_QueryService(serv_prov, &IID_IHttpNegotiate2, &IID_IHttpNegotiate2,
                                             (void**)&http_negotiate2_serv);
        ok(hres == S_OK, "Could not get IHttpNegotiate2 service: %08lx\n", hres);
        CLEAR_CALLED(QueryInterface_IHttpNegotiate2); /* IE <8 */
        ok(http_negotiate2 == http_negotiate2_serv, "http_negotiate != http_negotiate_serv\n");

        SET_EXPECT(QueryInterface_IHttpNegotiate2);
        SET_EXPECT(GetRootSecurityId);
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, (void*)0xdeadbeef, (void*)0xdeadbeef, 0);
        ok(hres == E_NOTIMPL, "GetRootSecurityId failed: %08lx\n", hres);
        CHECK_CALLED_BROKEN(QueryInterface_IHttpNegotiate2); /* IE8 */
        CHECK_CALLED(GetRootSecurityId);

        IHttpNegotiate2_Release(http_negotiate2_serv);
        IHttpNegotiate2_Release(http_negotiate2);
    }else {
        skip("Could not get IHttpNegotiate2\n");
        ret = FALSE;
    }

    SET_EXPECT(OnProgress_FINDINGRESOURCE);
    hres = IBindStatusCallback_OnProgress(holder, 0, 0, BINDSTATUS_FINDINGRESOURCE, NULL);
    ok(hres == S_OK, "OnProgress failed: %08lx\n", hres);
    CHECK_CALLED(OnProgress_FINDINGRESOURCE);

    SET_EXPECT(QueryInterface_IHttpNegotiate);
    SET_EXPECT(OnResponse);
    wstr = (void*)0xdeadbeef;
    hres = IHttpNegotiate_OnResponse(http_negotiate, 200, emptyW, NULL, NULL);
    ok(hres == S_OK, "OnResponse failed: %08lx\n", hres);
    CHECK_CALLED_BROKEN(QueryInterface_IHttpNegotiate); /* IE8 */
    CHECK_CALLED(OnResponse);

    IHttpNegotiate_Release(http_negotiate);

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IAuthenticate, (void**)&authenticate);
    ok(hres == S_OK, "Could not get IAuthenticate interface: %08lx\n", hres);

    SET_EXPECT(QueryInterface_IAuthenticate);
    SET_EXPECT(QueryService_IAuthenticate);
    hres = IServiceProvider_QueryService(serv_prov, &IID_IAuthenticate, &IID_IAuthenticate,
                                         (void**)&authenticate_serv);
    ok(hres == S_OK, "Could not get IAuthenticate service: %08lx\n", hres);
    CLEAR_CALLED(QueryInterface_IAuthenticate); /* IE <8 */
    CLEAR_CALLED(QueryService_IAuthenticate); /* IE <8 */
    ok(authenticate == authenticate_serv, "authenticate != authenticate_serv\n");
    IAuthenticate_Release(authenticate_serv);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IAuthenticate, &IID_IAuthenticate,
                                         (void**)&authenticate_serv);
    ok(hres == S_OK, "Could not get IAuthenticate service: %08lx\n", hres);
    ok(authenticate == authenticate_serv, "authenticate != authenticate_serv\n");

    IAuthenticate_Release(authenticate);
    IAuthenticate_Release(authenticate_serv);

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IInternetBindInfo, (void**)&bind_info);
    ok(hres == S_OK || broken(hres == E_NOINTERFACE /* win2k */), "Could not get IInternetBindInfo interface: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        hres = IInternetBindInfo_GetBindString(bind_info, BINDSTRING_USER_AGENT, &wstr, 1, &dw);
        ok(hres == E_NOINTERFACE, "GetBindString(BINDSTRING_USER_AGENT) failed: %08lx\n", hres);

        IInternetBindInfo_Release(bind_info);
    }

    SET_EXPECT(OnStopBinding);
    hres = IBindStatusCallback_OnStopBinding(holder, S_OK, NULL);
    ok(hres == S_OK, "OnStopBinding failed: %08lx\n", hres);
    CHECK_CALLED(OnStopBinding);

    SET_EXPECT(QueryInterface_IInternetProtocol);
    SET_EXPECT(QueryService_IInternetProtocol);
    hres = IServiceProvider_QueryService(serv_prov, &IID_IInternetProtocol, &IID_IInternetProtocol,
                                         (void**)&protocol);
    ok(hres == E_NOINTERFACE, "QueryService(IInternetProtocol) failed: %08lx\n", hres);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    CHECK_CALLED(QueryService_IInternetProtocol);

    IServiceProvider_Release(serv_prov);
    return ret;
}

static BOOL test_RegisterBindStatusCallback(void)
{
    IBindStatusCallback *prevbsc, *clb, *prev_clb;
    IBindCtx *bindctx;
    BOOL ret = TRUE;
    IUnknown *unk;
    HRESULT hres;

    strict_bsc_qi = TRUE;

    hres = CreateBindCtx(0, &bindctx);
    ok(hres == S_OK, "BindCtx failed: %08lx\n", hres);

    SET_EXPECT(QueryInterface_IServiceProvider);

    hres = IBindCtx_RegisterObjectParam(bindctx, BSCBHolder, (IUnknown*)&bsc);
    ok(hres == S_OK, "RegisterObjectParam failed: %08lx\n", hres);

    SET_EXPECT(QueryInterface_IBindStatusCallback);
    SET_EXPECT(QueryInterface_IBindStatusCallbackHolder);
    prevbsc = (void*)0xdeadbeef;
    hres = RegisterBindStatusCallback(bindctx, (IBindStatusCallback*)&bsc, &prevbsc, 0);
    ok(hres == S_OK, "RegisterBindStatusCallback failed: %08lx\n", hres);
    ok(prevbsc == (IBindStatusCallback*)&bsc, "prevbsc=%p\n", prevbsc);
    CHECK_CALLED(QueryInterface_IBindStatusCallback);
    CHECK_CALLED(QueryInterface_IBindStatusCallbackHolder);

    CHECK_CALLED(QueryInterface_IServiceProvider);

    hres = IBindCtx_GetObjectParam(bindctx, BSCBHolder, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IBindStatusCallback, (void**)&clb);
    IUnknown_Release(unk);
    ok(hres == S_OK, "QueryInterface(IID_IBindStatusCallback) failed: %08lx\n", hres);
    ok(clb != (IBindStatusCallback*)&bsc, "bsc == clb\n");

    if(!test_bscholder(clb))
        ret = FALSE;

    IBindStatusCallback_Release(clb);

    hres = RevokeBindStatusCallback(bindctx, (IBindStatusCallback*)&bsc);
    ok(hres == S_OK, "RevokeBindStatusCallback failed: %08lx\n", hres);

    unk = (void*)0xdeadbeef;
    hres = IBindCtx_GetObjectParam(bindctx, BSCBHolder, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08lx\n", hres);
    ok(unk == NULL, "unk != NULL\n");

    if(unk)
        IUnknown_Release(unk);

    hres = RevokeBindStatusCallback(bindctx, (void*)0xdeadbeef);
    ok(hres == S_OK, "RevokeBindStatusCallback failed: %08lx\n", hres);

    hres = RevokeBindStatusCallback(NULL, (void*)0xdeadbeef);
    ok(hres == E_INVALIDARG, "RevokeBindStatusCallback failed: %08lx\n", hres);

    hres = RevokeBindStatusCallback(bindctx, NULL);
    ok(hres == E_INVALIDARG, "RevokeBindStatusCallback failed: %08lx\n", hres);

    SET_EXPECT(QueryInterface_IServiceProvider);
    prevbsc = (void*)0xdeadbeef;
    hres = RegisterBindStatusCallback(bindctx, (IBindStatusCallback*)&bsc, &prevbsc, 0);
    ok(hres == S_OK, "RegisterBindStatusCallback failed: %08lx\n", hres);
    ok(!prevbsc, "prevbsc=%p\n", prevbsc);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    hres = IBindCtx_GetObjectParam(bindctx, BSCBHolder, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IBindStatusCallback, (void**)&prev_clb);
    IUnknown_Release(unk);
    ok(hres == S_OK, "QueryInterface(IID_IBindStatusCallback) failed: %08lx\n", hres);
    ok(prev_clb != (IBindStatusCallback*)&bsc, "bsc == clb\n");

    SET_EXPECT(QueryInterface_IServiceProvider);
    prevbsc = (void*)0xdeadbeef;
    hres = RegisterBindStatusCallback(bindctx, (IBindStatusCallback*)&bsc2, &prevbsc, 0);
    ok(hres == S_OK, "RegisterBindStatusCallback failed: %08lx\n", hres);
    ok(prevbsc == (IBindStatusCallback*)&bsc, "prevbsc != bsc\n");
    CHECK_CALLED(QueryInterface_IServiceProvider);

    hres = IBindCtx_GetObjectParam(bindctx, BSCBHolder, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IBindStatusCallback, (void**)&clb);
    IUnknown_Release(unk);
    ok(hres == S_OK, "QueryInterface(IID_IBindStatusCallback) failed: %08lx\n", hres);
    ok(prev_clb  == clb, "bsc != clb\n");

    IBindStatusCallback_Release(clb);
    IBindStatusCallback_Release(prev_clb);

    IBindCtx_Release(bindctx);

    strict_bsc_qi = FALSE;
    return ret;
}

#define BINDTEST_EMULATE            0x0001
#define BINDTEST_TOOBJECT           0x0002
#define BINDTEST_FILEDWLAPI         0x0004
#define BINDTEST_HTTPRESPONSE       0x0008
#define BINDTEST_REDIRECT           0x0010
#define BINDTEST_USE_CACHE          0x0020
#define BINDTEST_NO_CALLBACK_READ   0x0040
#define BINDTEST_NO_CALLBACK        0x0080
#define BINDTEST_ABORT              0x0100
#define BINDTEST_INVALID_CN         0x0200
#define BINDTEST_ABORT_START        0x0400
#define BINDTEST_ABORT_PROGRESS     0x0800
#define BINDTEST_ASYNC_SWITCH       0x1000
#define BINDTEST_ALLOW_FINDINGRESOURCE  0x2000

static void init_bind_test(int protocol, DWORD flags, DWORD t)
{
    const char *url_a = NULL;

    test_protocol = protocol;
    bindtest_flags = flags;
    emulate_protocol = (flags & BINDTEST_EMULATE) != 0;
    download_state = BEFORE_DOWNLOAD;
    stopped_binding = FALSE;
    stopped_obj_binding = FALSE;
    data_available = FALSE;
    mime_type[0] = 0;
    binding_hres = S_OK;
    bind_to_object = (flags & BINDTEST_TOOBJECT) != 0;
    tymed = t;
    filedwl_api = (flags & BINDTEST_FILEDWLAPI) != 0;
    post_test = (flags & BINDTEST_HTTPRESPONSE) != 0;

    switch(protocol) {
    case HTTP_TEST:
        if(post_test)
            url_a = "http://test.winehq.org/tests/post.php";
        else
            lstrcpyW(current_url, winetest_data_urlW);
        break;
    case ABOUT_TEST:
        url_a = "about:blank";
        break;
    case FILE_TEST:
        lstrcpyW(current_url, file_url);
        break;
    case MK_TEST:
        url_a = "mk:@MSITStore:test.chm::/blank.html";
        break;
    case ITS_TEST:
        url_a = "its:test.chm::/blank.html";
        break;
    case HTTPS_TEST:
        url_a = (flags & BINDTEST_INVALID_CN) ? "https://" WINEHQ_IP "/robots.txt" : "https://test.winehq.org/tests/hello.html";
        break;
    case FTP_TEST:
        url_a = "ftp://ftp.winehq.org/welcome%2emsg";
        break;
    default:
        url_a = "winetest:test";
    }

    if(url_a)
        MultiByteToWideChar(CP_ACP, 0, url_a, -1, current_url, ARRAY_SIZE(current_url));

    test_redirect = (flags & BINDTEST_REDIRECT) != 0;
    use_cache_file = (flags & BINDTEST_USE_CACHE) != 0;
    callback_read = !(flags & BINDTEST_NO_CALLBACK_READ);
    no_callback = (flags & BINDTEST_NO_CALLBACK) != 0;
    test_abort = (flags & BINDTEST_ABORT) != 0;
    abort_start = (flags & BINDTEST_ABORT_START) != 0;
    abort_progress = (flags & BINDTEST_ABORT_PROGRESS) != 0;
    async_switch = (flags & BINDTEST_ASYNC_SWITCH) != 0;
    is_async_prot = protocol == HTTP_TEST || protocol == HTTPS_TEST || protocol == FTP_TEST || protocol == WINETEST_TEST;
    prot_state = 0;
    ResetEvent(complete_event);

    if (winetest_debug > 1) trace("URL: %s\n", wine_dbgstr_w(current_url));
}

static void test_MonikerComposeWith(void)
{
    static const struct test_data
    {
       const WCHAR *l, *r, *expected;
    } data[] =
    {
        {L"https://test.winehq.org/", L"/tests/hello.html", L"https://test.winehq.org/tests/hello.html"},
        {L"url1", L"url2", L"url2"},
        {L"http://", L"tests.winehq.org", L"http:///tests.winehq.org"},
        {L"http://a/b/c/d;p?q#f", L"g", L"http://a/b/c/g"},
        {L"http://a/b/c/d;p?q#f", L"./g", L"http://a/b/c/g"},
        {L"http://a/b/c/d;p?q#f", L"g/", L"http://a/b/c/g/"},
        {L"http://a/b/c/d;p?q#f", L"/g", L"http://a/g"},
        {L"http://a/b/c/d;p?q#f", L"//g", L"http://g/"},
        {L"http://a/b/c/d;p?q#f", L"?y", L"http://a/b/c/d;p?y"},
        {L"http://a/b/c/d;p?q#f", L"g?y", L"http://a/b/c/g?y"},
        {L"http://a/b/c/d;p?q#f", L"g?y/./x", L"http://a/b/c/g?y/./x"},
        {L"http://a/b/c/d;p?q#f", L"#s", L"http://a/b/c/d;p?q#s"},
        {L"http://a/b/c/d;p?q#f", L"g#s", L"http://a/b/c/g#s"},
        {L"http://a/b/c/d;p?q#f", L"g#s/./x", L"http://a/b/c/g#s/./x"},
        {L"http://a/b/c/d;p?q#f", L"g?y#s", L"http://a/b/c/g?y#s"},
        {L"http://a/b/c/d;p?q#f", L";x", L"http://a/b/c/;x"},
        {L"http://a/b/c/d;p?q#f", L"g;x", L"http://a/b/c/g;x"},
        {L"http://a/b/c/d;p?q#f", L"g;x?y#s", L"http://a/b/c/g;x?y#s"},
        {L"http://a/b/c/d;p?q#f", L".", L"http://a/b/c/"},
        {L"http://a/b/c/d;p?q#f", L"./", L"http://a/b/c/"},
        {L"http://a/b/c/d;p?q#f", L"..", L"http://a/b/"},
        {L"http://a/b/c/d;p?q#f", L"../", L"http://a/b/"},
        {L"http://a/b/c/d;p?q#f", L"../g", L"http://a/b/g"},
        {L"http://a/b/c/d;p?q#f", L"../..", L"http://a/"},
        {L"http://a/b/c/d;p?q#f", L"../../", L"http://a/"},
        {L"http://a/b/c/d;p?q#f", L"../../g", L"http://a/g"},
        {L"http://a/b/c/d;p?q#f", L"../../../g", L"http://a/g"},
        {L"http://a/b/c/d;p?q#f", L"../../../../g", L"http://a/g"},
        {L"http://a/b/c/d;p?q#f", L"/./g", L"http://a/g"},
        {L"http://a/b/c/d;p?q#f", L"/../g", L"http://a/g"},
        {L"http://a/b/c/d;p?q#f", L"g.", L"http://a/b/c/g."},
        {L"http://a/b/c/d;p?q#f", L".g", L"http://a/b/c/.g"},
        {L"http://a/b/c/d;p?q#f", L"g..", L"http://a/b/c/g.."},
        {L"http://a/b/c/d;p?q#f", L"..g", L"http://a/b/c/..g"},
        {L"http://a/b/c/d;p?q#f", L"./../g", L"http://a/b/g"},
        {L"http://a/b/c/d;p?q#f", L"./g/.", L"http://a/b/c/g/"},
        {L"http://a/b/c/d;p?q#f", L"g/./h", L"http://a/b/c/g/h"},
        {L"http://a/b/c/d;p?q#f", L"g/../h", L"http://a/b/c/h"},
        {L"file://a/b/c/d;p?q#f", L"http:g", L"http:g"},
        {L"http://a/b/c/d;p?q#f", L"http://g/h", L"http://g/h"},
    };
    IMoniker *lmon, *rmon, *outmon, *filemon;
    HRESULT hres;
    LPOLESTR urlpath;
    CLSID clsid;
    int i;
    IBindCtx* bind;

    CreateBindCtx(0, &bind);
    for(i = 0; i < ARRAY_SIZE( data ); i++){
        hres = CreateURLMoniker(NULL, data[i].l, &lmon);
        ok(hres == S_OK, "failed to create moniker: %08lx\n", hres);
        hres = CreateURLMoniker(NULL, data[i].r, &rmon);
        ok(hres == S_OK, "failed to create moniker: %08lx\n", hres);

        hres = IMoniker_ComposeWith(lmon, rmon, FALSE, &outmon);
        ok(hres == S_OK,
            "ComposeWith failed: l: %s, r: %s, ret: %08lx\n", wine_dbgstr_w(data[i].l), wine_dbgstr_w(data[i].r), hres);

        IMoniker_GetClassID(outmon, &clsid);
        ok(IsEqualCLSID(&clsid, &CLSID_StdURLMoniker),
            "ComposeWith error: expected URL CLSID, got %s\n", wine_dbgstr_guid(&clsid));
        IMoniker_GetDisplayName(outmon, bind, NULL, &urlpath);
        ok(lstrcmpW(data[i].expected, urlpath) == S_OK,
            "ComposeWith expected %s got %s\n", wine_dbgstr_w(data[i].expected), wine_dbgstr_w(urlpath));

        IMoniker_Release(lmon);
        IMoniker_Release(rmon);
        IMoniker_Release(outmon);
        CoTaskMemFree(urlpath);
    }
    IBindCtx_Release(bind);

    hres = CreateURLMoniker(NULL, data[0].l, &lmon);
    ok(hres == S_OK, "failed to create moniker: %08lx\n", hres);
    hres = CreateURLMoniker(NULL, data[0].r, &rmon);
    ok(hres == S_OK, "failed to create moniker: %08lx\n", hres);
    hres = CreateFileMoniker(L"/a/b", &filemon);
    ok(hres == S_OK, "failed to create moniker: %08lx\n", hres);

    hres = IMoniker_ComposeWith(lmon, NULL, FALSE, &outmon);
    ok(hres == E_INVALIDARG, "ComposeWith error: %08lx\n", hres);

    hres = IMoniker_ComposeWith(lmon, rmon, FALSE, NULL);
    ok(hres == E_INVALIDARG, "ComposeWith error: %08lx\n", hres);

    hres = IMoniker_ComposeWith(lmon, filemon, TRUE, &outmon);
    ok(hres == MK_E_NEEDGENERIC, "ComposeWith error: %08lx\n", hres);
    hres = IMoniker_ComposeWith(lmon, filemon, FALSE, &outmon);
    ok(hres == S_OK, "ComposeWith error: %08lx\n", hres);

    IMoniker_GetClassID(outmon, &clsid);
    ok(IsEqualCLSID(&clsid, &CLSID_CompositeMoniker),
        "ComposeWith: got CLSID %s, not generic\n", wine_dbgstr_guid(&clsid));

    IMoniker_Release(lmon);
    IMoniker_Release(rmon);
    IMoniker_Release(filemon);
    IMoniker_Release(outmon);
}

static void test_BindToStorage(int protocol, DWORD flags, DWORD t)
{
    IMoniker *mon;
    HRESULT hres;
    LPOLESTR display_name;
    IBindCtx *bctx = NULL;
    MSG msg;
    IBindStatusCallback *previousclb;
    IUnknown *unk = (IUnknown*)0x00ff00ff;
    BOOL allow_finding_resource;
    IBinding *bind;

    init_bind_test(protocol, flags, t);
    allow_finding_resource = (flags & BINDTEST_ALLOW_FINDINGRESOURCE) != 0;

    if(no_callback) {
        hres = CreateBindCtx(0, &bctx);
        ok(hres == S_OK, "CreateBindCtx failed: %08lx\n", hres);
    }else {
        SET_EXPECT(QueryInterface_IServiceProvider);
        hres = CreateAsyncBindCtx(0, (IBindStatusCallback*)&bsc, NULL, &bctx);
        ok(hres == S_OK, "CreateAsyncBindCtx failed: %08lx\n\n", hres);
        CHECK_CALLED(QueryInterface_IServiceProvider);
        if(FAILED(hres))
            return;

        SET_EXPECT(QueryInterface_IServiceProvider);
        hres = RegisterBindStatusCallback(bctx, (IBindStatusCallback*)&bsc, &previousclb, 0);
        ok(hres == S_OK, "RegisterBindStatusCallback failed: %08lx\n", hres);
        ok(previousclb == (IBindStatusCallback*)&bsc, "previousclb(%p) != sclb(%p)\n", previousclb, &bsc);
        CHECK_CALLED(QueryInterface_IServiceProvider);
        if(previousclb)
            IBindStatusCallback_Release(previousclb);
    }

    hres = CreateURLMoniker(NULL, current_url, &mon);
    ok(hres == S_OK, "failed to create moniker: %08lx\n", hres);
    if(FAILED(hres))
        return;

    if(protocol == FTP_TEST)
    {
        /* FTP urls don't have any escape characters so convert the url to what is expected */
        DWORD size = 0;
        UrlUnescapeW(current_url, NULL, &size, URL_UNESCAPE_INPLACE);
    }

    hres = IMoniker_QueryInterface(mon, &IID_IBinding, (void**)&bind);
    ok(hres == E_NOINTERFACE, "IMoniker should not have IBinding interface\n");
    if(SUCCEEDED(hres))
        IBinding_Release(bind);

    hres = IMoniker_GetDisplayName(mon, bctx, NULL, &display_name);
    ok(hres == S_OK, "GetDisplayName failed %08lx\n", hres);
    ok(!lstrcmpW(display_name, current_url), "GetDisplayName got wrong name %s, expected %s\n",
       wine_dbgstr_w(display_name), wine_dbgstr_w(current_url));
    CoTaskMemFree(display_name);

    if(tymed == TYMED_FILE && (test_protocol == ABOUT_TEST || test_protocol == ITS_TEST))
        binding_hres = INET_E_DATA_NOT_AVAILABLE;
    if((flags & BINDTEST_INVALID_CN) && !invalid_cn_accepted &&
       (onsecurityproblem_hres != S_OK || security_problem == ERROR_INTERNET_SEC_CERT_ERRORS)) {
        if(security_problem == ERROR_INTERNET_SEC_CERT_ERRORS)
            binding_hres = INET_E_SECURITY_PROBLEM;
        else
            binding_hres = INET_E_INVALID_CERTIFICATE;
    }


    if(only_check_prot_args)
        SET_EXPECT(OnStopBinding);
    if(!no_callback) {
        SET_EXPECT(QueryInterface_IBindStatusCallbackEx);
        SET_EXPECT(GetBindInfo);
        SET_EXPECT(QueryInterface_IInternetProtocol);
        if(!emulate_protocol)
            SET_EXPECT(QueryService_IInternetProtocol);
        SET_EXPECT(OnStartBinding);
    }
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            SET_EXPECT(SetPriority);
        SET_EXPECT(Start);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST
           || test_protocol == WINETEST_SYNC_TEST)
            SET_EXPECT(Terminate);
        if(tymed != TYMED_FILE || (test_protocol != ABOUT_TEST && test_protocol != ITS_TEST))
            SET_EXPECT(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST) {
            SET_EXPECT(QueryService_IInternetBindInfo);
            if(!abort_start)
                SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(QueryInterface_IWindowForBindingUI);
            SET_EXPECT(QueryService_IWindowForBindingUI);
            SET_EXPECT(GetWindow_IWindowForBindingUI);
            if(!abort_start) {
                SET_EXPECT(BeginningTransaction);
                SET_EXPECT(QueryInterface_IHttpNegotiate2);
                SET_EXPECT(GetRootSecurityId);
                if(http_is_first || allow_finding_resource)
                    SET_EXPECT(OnProgress_FINDINGRESOURCE);
                SET_EXPECT(OnProgress_CONNECTING);
            }
            if(flags & BINDTEST_INVALID_CN) {
                SET_EXPECT(QueryInterface_IHttpSecurity);
                SET_EXPECT(QueryService_IHttpSecurity);
                SET_EXPECT(OnSecurityProblem);
                if(SUCCEEDED(onsecurityproblem_hres))
                    SET_EXPECT(GetWindow_IHttpSecurity);
            }
        }
        if(!no_callback) {
            if((test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FTP_TEST
               || test_protocol == FILE_TEST || test_protocol == WINETEST_TEST) && !abort_start)
                SET_EXPECT(OnProgress_SENDINGREQUEST);
            if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST) {
                SET_EXPECT(QueryInterface_IHttpNegotiate);
                SET_EXPECT(OnResponse);
            }
            if(!abort_start) {
                SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
                SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
            }
            if(test_protocol == FILE_TEST)
                SET_EXPECT(OnProgress_CACHEFILENAMEAVAILABLE);
            if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FTP_TEST || test_protocol == WINETEST_TEST)
                SET_EXPECT(OnProgress_DOWNLOADINGDATA);
            if(!abort_start)
                SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
            if((tymed != TYMED_FILE || test_protocol != ABOUT_TEST) && !abort_start)
                SET_EXPECT(OnDataAvailable);
            SET_EXPECT(OnStopBinding);
        }
    }

    hres = IMoniker_BindToStorage(mon, bctx, NULL, tymed == TYMED_ISTREAM ? &IID_IStream : &IID_IUnknown, (void**)&unk);
    if ((test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
        && hres == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
    {
        skip("Network unreachable, skipping tests\n");
        return;
    }

    if(only_check_prot_args) {
        ok(hres == E_FAIL, "Got %08lx\n", hres);
        CHECK_CALLED(OnStopBinding);
    } else if(abort_start)
        ok(hres == abort_hres, "IMoniker_BindToStorage failed: %08lx, expected %08lx\n", hres, abort_hres);
    else if(abort_progress)
        ok(hres == MK_S_ASYNCHRONOUS, "IMoniker_BindToStorage failed: %08lx\n", hres);
    else if(no_callback) {
        if(emulate_protocol)
            ok( WaitForSingleObject(complete_event2, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08lx\n", hres);
        ok(unk != NULL, "unk == NULL\n");
    }else if(!(bindf & BINDF_ASYNCHRONOUS) && tymed == TYMED_FILE) {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08lx\n", hres);
        ok(unk == NULL, "unk != NULL\n");
    }else if(((bindf & BINDF_ASYNCHRONOUS) && !data_available)
       || (tymed == TYMED_FILE && test_protocol == FILE_TEST)) {
        ok(hres == MK_S_ASYNCHRONOUS, "IMoniker_BindToStorage failed: %08lx\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else if(tymed == TYMED_FILE && test_protocol == ABOUT_TEST) {
        ok(hres == INET_E_DATA_NOT_AVAILABLE,
           "IMoniker_BindToStorage failed: %08lx, expected INET_E_DATA_NOT_AVAILABLE\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else if((flags & BINDTEST_INVALID_CN) && binding_hres != S_OK) {
        ok(hres == binding_hres, "Got %08lx\n", hres);
        ok(unk == NULL, "Got %p\n", unk);
    }else if((flags & BINDTEST_INVALID_CN) && invalid_cn_accepted) {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08lx\n", hres);
        ok(unk != NULL, "unk == NULL\n");
        if(unk == NULL) {
            ok(0, "Expected security problem to be ignored.\n");
            invalid_cn_accepted = FALSE;
            binding_hres = INET_E_INVALID_CERTIFICATE;
        }
    }else {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08lx\n", hres);
        ok(unk != NULL, "unk == NULL\n");
    }
    if(unk && callback_read && !no_callback) {
        IUnknown_Release(unk);
        unk = NULL;
    }

    if(FAILED(hres) && !(flags & BINDTEST_INVALID_CN) && !(flags & BINDTEST_ABORT_START))
        return;

    if((bindf & BINDF_ASYNCHRONOUS) && !no_callback) {
        while(!stopped_binding && GetMessageA(&msg,NULL,0,0)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if(async_switch) {
        CHECK_CALLED(OnProgress_SENDINGREQUEST);
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        CHECK_CALLED(LockRequest);
        CHECK_CALLED(OnStopBinding);
    }
    if(!no_callback) {
        CLEAR_CALLED(QueryInterface_IBindStatusCallbackEx); /* IE 8 */
        CHECK_CALLED(GetBindInfo);
        todo_wine_if(abort_start)
            CHECK_CALLED(QueryInterface_IInternetProtocol);
        if(!emulate_protocol) {
            todo_wine_if(abort_start)
                CHECK_CALLED(QueryService_IInternetProtocol);
        }
        CHECK_CALLED(OnStartBinding);
    }
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            CLEAR_CALLED(SetPriority); /* Not called by IE11 */
        CHECK_CALLED(Start);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST
           || test_protocol == WINETEST_SYNC_TEST) {
            if(tymed == TYMED_FILE)
                CLEAR_CALLED(Read);
            CHECK_CALLED(Terminate);
        }
        if(tymed != TYMED_FILE || (test_protocol != ABOUT_TEST && test_protocol != ITS_TEST))
            CHECK_CALLED(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST) {
            CLEAR_CALLED(QueryService_IInternetBindInfo);
            if(!abort_start)
                CHECK_CALLED(QueryInterface_IHttpNegotiate);
            CLEAR_CALLED(QueryInterface_IWindowForBindingUI);
            CLEAR_CALLED(QueryService_IWindowForBindingUI);
            CLEAR_CALLED(GetWindow_IWindowForBindingUI);
            if(!abort_start)
                CHECK_CALLED(BeginningTransaction);
            if (have_IHttpNegotiate2 && !abort_start)
            {
                CHECK_CALLED(QueryInterface_IHttpNegotiate2);
                CHECK_CALLED(GetRootSecurityId);
            }
            if(http_is_first) {
                if (! proxy_active())
                {
                    CHECK_CALLED(OnProgress_FINDINGRESOURCE);
                    CHECK_CALLED(OnProgress_CONNECTING);
                }
                else
                {
                    CLEAR_CALLED(OnProgress_FINDINGRESOURCE);
                    CLEAR_CALLED(OnProgress_CONNECTING);
                }
            }else if(!abort_start) {
                if(allow_finding_resource)
                    CLEAR_CALLED(OnProgress_FINDINGRESOURCE);
                /* IE7 does call this */
                CLEAR_CALLED(OnProgress_CONNECTING);
            }
            if((flags & BINDTEST_INVALID_CN) && !invalid_cn_accepted)  {
                CHECK_CALLED(QueryInterface_IHttpSecurity);
                CHECK_CALLED(QueryService_IHttpSecurity);
                CHECK_CALLED(OnSecurityProblem);
            }else {
                CHECK_NOT_CALLED(QueryInterface_IHttpSecurity);
                CHECK_NOT_CALLED(QueryService_IHttpSecurity);
                CHECK_NOT_CALLED(OnSecurityProblem);
            }
        }
        if(!no_callback) {
            if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FILE_TEST || test_protocol == WINETEST_TEST) {
                if(flags & BINDTEST_INVALID_CN)
                    CLEAR_CALLED(OnProgress_SENDINGREQUEST);
                else if(!abort_start)
                    CHECK_CALLED(OnProgress_SENDINGREQUEST);
            } else if(test_protocol == FTP_TEST)
                todo_wine CHECK_CALLED(OnProgress_SENDINGREQUEST);
            if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == WINETEST_TEST) {
                CLEAR_CALLED(QueryInterface_IHttpNegotiate);
                if((!(flags & BINDTEST_INVALID_CN) || (binding_hres == S_OK)) && !abort_start) {
                    CHECK_CALLED(OnResponse);
                }
            }
            if((!(flags & BINDTEST_INVALID_CN) || binding_hres == S_OK) && !abort_start) {
                CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
                CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
                CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
            }
            if(test_protocol == FILE_TEST)
                CHECK_CALLED(OnProgress_CACHEFILENAMEAVAILABLE);
            if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FTP_TEST  || test_protocol == WINETEST_TEST)
                CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
            if((flags & BINDTEST_INVALID_CN)) {
                if(binding_hres == S_OK)
                    CHECK_CALLED(OnDataAvailable);
                else
                    CHECK_NOT_CALLED(OnDataAvailable);
            }else if((tymed != TYMED_FILE || test_protocol != ABOUT_TEST) && !abort_start)
                CHECK_CALLED(OnDataAvailable);
            CHECK_CALLED(OnStopBinding);
        }
    }

    ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");
    if(bctx)
        ok(IBindCtx_Release(bctx) == 0, "bctx should be destroyed here\n");

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
        http_is_first = FALSE;

    if((flags & BINDTEST_INVALID_CN) && onsecurityproblem_hres == S_OK && security_problem != ERROR_INTERNET_SEC_CERT_ERRORS)
        invalid_cn_accepted = TRUE;

    if(unk) {
        BYTE buf[512];
        DWORD read;
        IStream *stream;

        hres = IUnknown_QueryInterface(unk, &IID_IStream, (void**)&stream);
        ok(hres == S_OK, "Could not get IStream iface: %08lx\n", hres);
        IUnknown_Release(unk);

        do {
            read = 0xdeadbeef;
            hres = IStream_Read(stream, buf, sizeof(buf), &read);
            ok(read != 0xdeadbeef, "read = 0xdeadbeef\n");
            if(emulate_protocol && test_protocol == HTTP_TEST && read)
                ok(buf[0] == (use_cache_file && !(bindf&BINDF_ASYNCHRONOUS) ? 'X' : '?'), "buf[0] = '%c'\n", buf[0]);
        }while(hres == S_OK);
        ok(hres == S_FALSE, "IStream_Read returned %08lx\n", hres);
        ok(!read, "read = %ld\n", read);

        IStream_Release(stream);
    }
}

static void test_BindToObject(int protocol, DWORD flags, HRESULT exhres)
{
    IMoniker *mon;
    HRESULT hres;
    LPOLESTR display_name;
    IBindCtx *bctx;
    DWORD regid;
    MSG msg;
    IUnknown *unk = (IUnknown*)0x00ff00ff;
    IBinding *bind;

    init_bind_test(protocol, BINDTEST_TOOBJECT|flags, TYMED_ISTREAM);
    binding_hres = exhres;

    if(emulate_protocol)
        CoRegisterClassObject(&CLSID_HTMLDocument, (IUnknown *)&mime_cf,
                              CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, (IBindStatusCallback*)&objbsc, NULL, &bctx);
    ok(hres == S_OK, "CreateAsyncBindCtx failed: %08lx\n\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    if(FAILED(hres))
        return;

    hres = CreateURLMoniker(NULL, current_url, &mon);
    ok(hres == S_OK, "failed to create moniker: %08lx\n", hres);
    if(FAILED(hres)) {
        IBindCtx_Release(bctx);
        return;
    }

    hres = IMoniker_QueryInterface(mon, &IID_IBinding, (void**)&bind);
    ok(hres == E_NOINTERFACE, "IMoniker should not have IBinding interface\n");
    if(SUCCEEDED(hres))
        IBinding_Release(bind);

    hres = IMoniker_GetDisplayName(mon, bctx, NULL, &display_name);
    ok(hres == S_OK, "GetDisplayName failed %08lx\n", hres);
    ok(!lstrcmpW(display_name, current_url), "GetDisplayName got wrong name\n");
    CoTaskMemFree(display_name);

    SET_EXPECT(QueryInterface_IBindStatusCallbackEx);
    SET_EXPECT(Obj_GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    if(!emulate_protocol)
        SET_EXPECT(QueryService_IInternetProtocol);
    SET_EXPECT(Obj_OnStartBinding);
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            SET_EXPECT(SetPriority);
        SET_EXPECT(Start);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(Terminate);
        if(test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(BeginningTransaction);
            SET_EXPECT(QueryInterface_IHttpNegotiate2);
            SET_EXPECT(GetRootSecurityId);
            if(http_is_first)
                SET_EXPECT(Obj_OnProgress_FINDINGRESOURCE);
            SET_EXPECT(Obj_OnProgress_CONNECTING);
            SET_EXPECT(QueryInterface_IWindowForBindingUI);
            SET_EXPECT(QueryService_IWindowForBindingUI);
            SET_EXPECT(GetWindow_IWindowForBindingUI);
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FILE_TEST)
            SET_EXPECT(Obj_OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(OnResponse);
        }
        SET_EXPECT(Obj_OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(Obj_OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            SET_EXPECT(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(Obj_OnProgress_ENDDOWNLOADDATA);
        if(SUCCEEDED(hres))
            SET_EXPECT(Obj_OnProgress_CLASSIDAVAILABLE);
        SET_EXPECT(Obj_OnProgress_BEGINSYNCOPERATION);
        if(exhres == REGDB_E_CLASSNOTREG) {
            SET_EXPECT(QueryInterface_IWindowForBindingUI);
            SET_EXPECT(QueryService_IWindowForBindingUI);
            SET_EXPECT(GetWindow_ICodeInstall);
        }
        SET_EXPECT(Obj_OnProgress_ENDSYNCOPERATION);
        if(SUCCEEDED(hres))
            SET_EXPECT(OnObjectAvailable);
        SET_EXPECT(Obj_OnStopBinding);
    }

    hres = IMoniker_BindToObject(mon, bctx, NULL, &IID_IUnknown, (void**)&unk);

    if ((test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
        && hres == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
    {
        skip( "Network unreachable, skipping tests\n" );
        return;
    }

    if(FAILED(exhres)) {
        ok(hres == exhres, "BindToObject failed:  %08lx, expected %08lx\n", hres, exhres);
        ok(!unk, "unk = %p, expected NULL\n", unk);
    }else if(bindf & BINDF_ASYNCHRONOUS) {
        ok(hres == MK_S_ASYNCHRONOUS, "IMoniker_BindToObject failed: %08lx\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08lx\n", hres);
        ok(unk != NULL, "unk == NULL\n");
        if(emulate_protocol)
            ok(unk == (IUnknown*)&PersistMoniker, "unk != PersistMoniker\n");
    }
    if(unk)
        IUnknown_Release(unk);

    while((bindf & BINDF_ASYNCHRONOUS) &&
          !((!emulate_protocol || stopped_binding) && stopped_obj_binding) && GetMessageA(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    CLEAR_CALLED(QueryInterface_IBindStatusCallbackEx);
    CHECK_CALLED(Obj_GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    if(!emulate_protocol)
        CHECK_CALLED(QueryService_IInternetProtocol);
    CHECK_CALLED(Obj_OnStartBinding);
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            CLEAR_CALLED(SetPriority); /* Not called by IE11 */
        CHECK_CALLED(Start);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CHECK_CALLED(Terminate);
        if(test_protocol == FILE_TEST)
            CLEAR_CALLED(OnProgress_MIMETYPEAVAILABLE); /* not called in IE7 */
        CHECK_CALLED(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            CHECK_CALLED(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(BeginningTransaction);
            if (have_IHttpNegotiate2)
            {
                CHECK_CALLED(QueryInterface_IHttpNegotiate2);
                CHECK_CALLED(GetRootSecurityId);
            }
            if(http_is_first) {
                CHECK_CALLED(Obj_OnProgress_FINDINGRESOURCE);
                CHECK_CALLED(Obj_OnProgress_CONNECTING);
            }else {
                /* IE7 does call this */
                CLEAR_CALLED(Obj_OnProgress_CONNECTING);
            }
            CLEAR_CALLED(QueryInterface_IWindowForBindingUI);
            CLEAR_CALLED(QueryService_IWindowForBindingUI);
            CLEAR_CALLED(GetWindow_IWindowForBindingUI);
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FILE_TEST) {
            if(post_test)
                CLEAR_CALLED(Obj_OnProgress_SENDINGREQUEST);
            else
                CHECK_CALLED(Obj_OnProgress_SENDINGREQUEST);
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            CLEAR_CALLED(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(OnResponse);
        }
        CHECK_CALLED(Obj_OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            CHECK_CALLED(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
        CLEAR_CALLED(Obj_OnProgress_ENDDOWNLOADDATA);
        if(SUCCEEDED(hres))
            CHECK_CALLED(Obj_OnProgress_CLASSIDAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_BEGINSYNCOPERATION);
        if(exhres == REGDB_E_CLASSNOTREG) {
            todo_wine CHECK_CALLED(QueryInterface_IWindowForBindingUI);
            todo_wine CHECK_CALLED(QueryService_IWindowForBindingUI);
            todo_wine CHECK_CALLED(GetWindow_ICodeInstall);
        }
        CHECK_CALLED(Obj_OnProgress_ENDSYNCOPERATION);
        if(SUCCEEDED(hres))
            CHECK_CALLED(OnObjectAvailable);
        CHECK_CALLED(Obj_OnStopBinding);
    }

    ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");
    IBindCtx_Release(bctx);

    if(emulate_protocol)
        CoRevokeClassObject(regid);

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
        http_is_first = FALSE;
}

static void test_URLDownloadToFile(DWORD prot, BOOL emul)
{
    BOOL res;
    HRESULT hres;

    init_bind_test(prot, BINDTEST_FILEDWLAPI | (emul ? BINDTEST_EMULATE : 0), TYMED_FILE);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    SET_EXPECT(QueryInterface_IServiceProvider);
    if(!emulate_protocol)
        SET_EXPECT(QueryService_IInternetProtocol);
    SET_EXPECT(OnStartBinding);
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            SET_EXPECT(SetPriority);
        SET_EXPECT(Start);
        SET_EXPECT(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(BeginningTransaction);
            SET_EXPECT(QueryInterface_IHttpNegotiate2);
            SET_EXPECT(GetRootSecurityId);
            SET_EXPECT(QueryInterface_IWindowForBindingUI);
            SET_EXPECT(OnProgress_CONNECTING);
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(OnResponse);
        }
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
        SET_EXPECT(OnStopBinding);
    }

    hres = URLDownloadToFileW(NULL, current_url, dwl_htmlW, 0, (IBindStatusCallback*)&bsc);
    ok(hres == S_OK, "URLDownloadToFile failed: %08lx\n", hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    if(!emulate_protocol) {
        CHECK_CALLED(QueryInterface_IServiceProvider);
        CHECK_CALLED(QueryService_IInternetProtocol);
    }else {
        CLEAR_CALLED(QueryInterface_IServiceProvider);
    }
    CHECK_CALLED(OnStartBinding);
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            CLEAR_CALLED(SetPriority); /* Not called by IE11 */
        CHECK_CALLED(Start);
        CHECK_CALLED(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            CHECK_CALLED(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(BeginningTransaction);
            if (have_IHttpNegotiate2)
            {
                CHECK_CALLED(QueryInterface_IHttpNegotiate2);
                CHECK_CALLED(GetRootSecurityId);
            }
            CLEAR_CALLED(QueryInterface_IWindowForBindingUI);
            CLEAR_CALLED(OnProgress_CONNECTING);
        }
        if(test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
        else if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CLEAR_CALLED(OnProgress_SENDINGREQUEST); /* not called by IE7 */
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            CLEAR_CALLED(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(OnResponse);
        }
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
        CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
        CHECK_CALLED(OnStopBinding);
    }

    res = DeleteFileA(dwl_htmlA);
    ok(res, "DeleteFile failed: %lu\n", GetLastError());

    if(prot != FILE_TEST || emul)
        return;

    hres = URLDownloadToFileW(NULL, current_url, dwl_htmlW, 0, NULL);
    ok(hres == S_OK, "URLDownloadToFile failed: %08lx\n", hres);

    res = DeleteFileA(dwl_htmlA);
    ok(res, "DeleteFile failed: %lu\n", GetLastError());
}

static void test_URLDownloadToFile_abort(void)
{
    HRESULT hres;

    init_bind_test(HTTP_TEST, BINDTEST_FILEDWLAPI|BINDTEST_ABORT_PROGRESS, TYMED_FILE);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    SET_EXPECT(QueryInterface_IServiceProvider);
    SET_EXPECT(QueryService_IInternetProtocol);
    SET_EXPECT(OnStartBinding);
    SET_EXPECT(QueryInterface_IHttpNegotiate);
    SET_EXPECT(QueryInterface_IHttpNegotiate2);
    SET_EXPECT(BeginningTransaction);
    SET_EXPECT(GetRootSecurityId);
    SET_EXPECT(QueryInterface_IWindowForBindingUI);
    SET_EXPECT(OnProgress_CONNECTING);
    SET_EXPECT(OnProgress_SENDINGREQUEST);
    SET_EXPECT(OnStopBinding);

    hres = URLDownloadToFileW(NULL, current_url, dwl_htmlW, 0, (IBindStatusCallback*)&bsc);
    ok(hres == E_ABORT, "URLDownloadToFile failed: %08lx, expected E_ABORT\n", hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    CHECK_CALLED(QueryService_IInternetProtocol);
    CHECK_CALLED(OnStartBinding);
    CHECK_CALLED(QueryInterface_IHttpNegotiate);
    CHECK_CALLED(QueryInterface_IHttpNegotiate2);
    CHECK_CALLED(BeginningTransaction);
    CHECK_CALLED(GetRootSecurityId);
    CLEAR_CALLED(QueryInterface_IWindowForBindingUI);
    CHECK_CALLED(OnProgress_SENDINGREQUEST);
    CLEAR_CALLED(OnProgress_CONNECTING);
    CHECK_CALLED(OnStopBinding);

    init_bind_test(HTTP_TEST, BINDTEST_FILEDWLAPI|BINDTEST_ABORT_START, TYMED_FILE);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    SET_EXPECT(QueryInterface_IServiceProvider);
    SET_EXPECT(QueryService_IInternetProtocol);
    SET_EXPECT(OnStartBinding);
    SET_EXPECT(OnStopBinding);

    abort_hres = E_ABORT;
    hres = URLDownloadToFileW(NULL, current_url, dwl_htmlW, 0, (IBindStatusCallback*)&bsc);
    ok(hres == E_ABORT, "URLDownloadToFile failed: %08lx, expected E_ABORT\n", hres);

    CHECK_CALLED(GetBindInfo);
    todo_wine CHECK_CALLED(QueryInterface_IInternetProtocol);
    todo_wine CHECK_CALLED(QueryInterface_IServiceProvider);
    todo_wine CHECK_CALLED(QueryService_IInternetProtocol);
    CHECK_CALLED(OnStartBinding);
    CHECK_CALLED(OnStopBinding);

    init_bind_test(HTTP_TEST, BINDTEST_FILEDWLAPI|BINDTEST_ABORT_START, TYMED_FILE);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    SET_EXPECT(QueryInterface_IServiceProvider);
    SET_EXPECT(QueryService_IInternetProtocol);
    SET_EXPECT(OnStartBinding);
    SET_EXPECT(QueryInterface_IHttpNegotiate);
    SET_EXPECT(QueryInterface_IHttpNegotiate2);
    SET_EXPECT(BeginningTransaction);
    SET_EXPECT(GetRootSecurityId);
    SET_EXPECT(QueryInterface_IWindowForBindingUI);
    SET_EXPECT(OnResponse);
    SET_EXPECT(OnProgress_CONNECTING);
    SET_EXPECT(OnProgress_SENDINGREQUEST);
    SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
    SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
    SET_EXPECT(OnProgress_DOWNLOADINGDATA);
    SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    SET_EXPECT(OnStopBinding);

    /* URLDownloadToFile doesn't abort if E_NOTIMPL is returned from the
     * IBindStatusCallback's OnStartBinding function.
     */
    abort_hres = E_NOTIMPL;
    hres = URLDownloadToFileW(NULL, current_url, dwl_htmlW, 0, (IBindStatusCallback*)&bsc);
    ok(hres == S_OK, "URLDownloadToFile failed: %08lx\n", hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    CHECK_CALLED(QueryService_IInternetProtocol);
    CHECK_CALLED(OnStartBinding);
    CHECK_CALLED(QueryInterface_IHttpNegotiate);
    CHECK_CALLED(QueryInterface_IHttpNegotiate2);
    CHECK_CALLED(BeginningTransaction);
    CHECK_CALLED(GetRootSecurityId);
    CLEAR_CALLED(QueryInterface_IWindowForBindingUI);
    CHECK_CALLED(OnResponse);
    CLEAR_CALLED(OnProgress_CONNECTING);
    CHECK_CALLED(OnProgress_SENDINGREQUEST);
    CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
    CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
    CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
    CHECK_CALLED(OnStopBinding);

    DeleteFileA(dwl_htmlA);
}

static void set_file_url(char *path)
{
    CHAR file_urlA[INTERNET_MAX_URL_LENGTH];

    lstrcpyA(file_urlA, "file://");
    lstrcatA(file_urlA, path);
    MultiByteToWideChar(CP_ACP, 0, file_urlA, -1, file_url, INTERNET_MAX_URL_LENGTH);
}

static void create_file(const char *file_name, const char *content)
{
    HANDLE file;
    DWORD size;
    CHAR path[MAX_PATH];

    file = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return;

    if(test_file)
        DeleteFileA(test_file);
    test_file = file_name;
    WriteFile(file, content, strlen(content), &size, NULL);
    CloseHandle(file);

    GetCurrentDirectoryA(MAX_PATH, path);
    lstrcatA(path, "\\");
    lstrcatA(path, file_name);
    set_file_url(path);
}

static void create_html_file(void)
{
    create_file(wszIndexHtmlA, "<HTML></HTML>");
}

static void create_cache_file(void)
{
    char buf[6500], curdir[MAX_PATH];
    HANDLE file;
    DWORD size;

    file = CreateFileA(test_txtA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return;

    memset(buf, 'X', sizeof(buf));
    WriteFile(file, buf, sizeof(buf), &size, NULL);
    CloseHandle(file);

    memset(curdir, 0, sizeof(curdir));
    GetCurrentDirectoryA(MAX_PATH, curdir);
    lstrcatA(curdir, "\\");
    lstrcatA(curdir, test_txtA);

    MultiByteToWideChar(CP_ACP, 0, curdir, -1, cache_file_name, MAX_PATH);
}

static void test_ReportResult(HRESULT exhres)
{
    IMoniker *mon = NULL;
    IBindCtx *bctx = NULL;
    IUnknown *unk = (void*)0xdeadbeef;
    HRESULT hres;

    init_bind_test(ABOUT_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);
    binding_hres = exhres;

    hres = CreateURLMoniker(NULL, about_blankW, &mon);
    ok(hres == S_OK, "CreateURLMoniker failed: %08lx\n", hres);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, (IBindStatusCallback*)&bsc, NULL, &bctx);
    ok(hres == S_OK, "CreateAsyncBindCtx failed: %08lx\n\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    SET_EXPECT(QueryInterface_IBindStatusCallbackEx);
    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    SET_EXPECT(OnStartBinding);
    if(is_urlmon_protocol(test_protocol))
        SET_EXPECT(SetPriority);
    SET_EXPECT(Start);

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
    if(SUCCEEDED(exhres))
        ok(hres == S_OK || hres == MK_S_ASYNCHRONOUS, "BindToStorage failed: %08lx\n", hres);
    else
        ok(hres == exhres || hres == MK_S_ASYNCHRONOUS,
           "BindToStorage failed: %08lx, expected %08lx or MK_S_ASYNCHRONOUS\n", hres, exhres);

    CLEAR_CALLED(QueryInterface_IBindStatusCallbackEx); /* IE 8 */
    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    CHECK_CALLED(OnStartBinding);
    if(is_urlmon_protocol(test_protocol))
        CLEAR_CALLED(SetPriority); /* Not called by IE11 */
    CHECK_CALLED(Start);

    ok(unk == NULL, "unk=%p\n", unk);

    IBindCtx_Release(bctx);
    IMoniker_Release(mon);
}

static void test_BindToStorage_fail(void)
{
    IMoniker *mon = NULL;
    IBindCtx *bctx = NULL;
    IUnknown *unk;
    HRESULT hres;

    hres = CreateURLMoniker(NULL, about_blankW, &mon);
    ok(hres == S_OK, "CreateURLMoniker failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = pCreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08lx\n", hres);

    unk = (void*)0xdeadbeef;
    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
    ok(hres == MK_E_SYNTAX || hres == INET_E_DATA_NOT_AVAILABLE,
       "hres=%08lx, expected MK_E_SYNTAX or INET_E_DATA_NOT_AVAILABLE\n", hres);
    ok(unk == NULL, "got %p\n", unk);

    IBindCtx_Release(bctx);

    IMoniker_Release(mon);

    test_ReportResult(E_NOTIMPL);
    test_ReportResult(S_FALSE);
}

static void test_StdURLMoniker(void)
{
    IMoniker *mon, *async_mon;
    LPOLESTR display_name;
    IBindCtx *bctx;
    IUnknown *unk;
    HRESULT hres;

    hres = CoCreateInstance(&IID_IInternet, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IMoniker, (void**)&mon);
    ok(hres == S_OK, "Could not create IInternet instance: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IMoniker_QueryInterface(mon, &IID_IAsyncMoniker, (void**)&async_mon);
    ok(hres == S_OK, "Could not get IAsyncMoniker iface: %08lx\n", hres);
    ok(mon == async_mon, "mon != async_mon\n");
    IMoniker_Release(async_mon);

    hres = IMoniker_GetDisplayName(mon, NULL, NULL, &display_name);
    ok(hres == E_OUTOFMEMORY, "GetDisplayName failed: %08lx, expected E_OUTOFMEMORY\n", hres);

    if(pCreateUri) {
      IUriContainer *uri_container;
      IUri *uri;

      hres = IMoniker_QueryInterface(mon, &IID_IUriContainer, (void**)&uri_container);
      ok(hres == S_OK, "Could not get IUriMoniker iface: %08lx\n", hres);


      uri = (void*)0xdeadbeef;
      hres = IUriContainer_GetIUri(uri_container, &uri);
      ok(hres == S_FALSE, "GetIUri failed: %08lx\n", hres);
      ok(!uri, "uri = %p, expected NULL\n", uri);

      IUriContainer_Release(uri_container);
    }

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, (IBindStatusCallback*)&bsc, NULL, &bctx);
    ok(hres == S_OK, "CreateAsyncBindCtx failed: %08lx\n\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    if(pCreateUri) { /* Skip these tests on old IEs */
        unk = (void*)0xdeadbeef;
        hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
        ok(hres == MK_E_SYNTAX, "BindToStorage failed: %08lx, expected MK_E_SYNTAX\n", hres);
        ok(!unk, "unk = %p\n", unk);

        unk = (void*)0xdeadbeef;
        hres = IMoniker_BindToObject(mon, bctx, NULL, &IID_IUnknown, (void**)&unk);
        ok(hres == MK_E_SYNTAX, "BindToStorage failed: %08lx, expected MK_E_SYNTAX\n", hres);
        ok(!unk, "unk = %p\n", unk);
    }

    IMoniker_Release(mon);
}

static void register_protocols(void)
{
    IInternetSession *session;
    HRESULT hres;

    static const WCHAR winetestW[] = {'w','i','n','e','t','e','s','t',0};

    hres = CoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetSession_RegisterNameSpace(session, &protocol_cf, &IID_NULL,
            winetestW, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

    IInternetSession_Release(session);
}

static BOOL can_do_https(void)
{
    HINTERNET ses, con, req;
    BOOL ret;

    ses = InternetOpenA("winetest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ok(ses != NULL, "InternetOpen failed\n");

    con = InternetConnectA(ses, "gitlab.winehq.org", INTERNET_DEFAULT_HTTPS_PORT,
            NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != NULL, "InternetConnect failed\n");

    req = HttpOpenRequestA(con, "GET", "/robots.txt", NULL, NULL, NULL,
            INTERNET_FLAG_SECURE, 0);
    ok(req != NULL, "HttpOpenRequest failed\n");

    ret = HttpSendRequestA(req, NULL, 0, NULL, 0);
    ok(ret || broken(GetLastError() == ERROR_INTERNET_CANNOT_CONNECT)
           || broken(GetLastError() == ERROR_INTERNET_SECURITY_CHANNEL_ERROR) /* WinXP */,
        "request failed: %lu\n", GetLastError());

    InternetCloseHandle(req);
    InternetCloseHandle(con);
    InternetCloseHandle(ses);
    return ret;
}

START_TEST(url)
{
    HMODULE hurlmon;

    hurlmon = GetModuleHandleA("urlmon.dll");
    pCreateAsyncBindCtxEx = (void*) GetProcAddress(hurlmon, "CreateAsyncBindCtxEx");

    if(!GetProcAddress(hurlmon, "CompareSecurityIds")) {
        win_skip("Too old IE\n");
        return;
    }

    pCreateUri = (void*) GetProcAddress(hurlmon, "CreateUri");
    if(!pCreateUri)
        win_skip("IUri not supported\n");

    complete_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    complete_event2 = CreateEventW(NULL, FALSE, FALSE, NULL);
    thread_id = GetCurrentThreadId();
    create_html_file();
    create_cache_file();
    register_protocols();

    test_create();

    trace("test CreateAsyncBindCtx...\n");
    test_CreateAsyncBindCtx();

    trace("test CreateAsyncBindCtxEx...\n");
    test_CreateAsyncBindCtxEx();

    trace("test RegisterBindStatusCallback...\n");
    if(test_RegisterBindStatusCallback()) {
        trace("test BindToStorage failures...\n");
        test_BindToStorage_fail();

        trace("synchronous http test (COM not initialised)...\n");
        test_BindToStorage(HTTP_TEST, 0, TYMED_ISTREAM);

        CoInitialize(NULL);

        trace("test StdURLMoniker...\n");
        test_StdURLMoniker();

        trace("test URLMonikerComposeWith...\n");
        test_MonikerComposeWith();

        trace("synchronous http test...\n");
        test_BindToStorage(HTTP_TEST, 0, TYMED_ISTREAM);

        trace("emulated synchronous http test (to file)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE, TYMED_FILE);

        trace("synchronous http test (to object)...\n");
        test_BindToObject(HTTP_TEST, 0, S_OK);

        trace("emulated synchronous http test (with cache)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE|BINDTEST_USE_CACHE, TYMED_ISTREAM);

        trace("emulated synchronous http test (with cache, no read)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE|BINDTEST_USE_CACHE|BINDTEST_NO_CALLBACK_READ, TYMED_ISTREAM);

        trace("synchronous http test (with cache, no read)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_USE_CACHE|BINDTEST_NO_CALLBACK_READ, TYMED_ISTREAM);

        trace("synchronous file test...\n");
        test_BindToStorage(FILE_TEST, 0, TYMED_ISTREAM);

        trace("emulated synchronous file test (to file)...\n");
        test_BindToStorage(FILE_TEST, BINDTEST_EMULATE, TYMED_FILE);

        trace("synchronous file test (to object)...\n");
        test_BindToObject(FILE_TEST, 0, S_OK);

        trace("bind to an object of not registered MIME type...\n");
        create_file("test.winetest", "\x01\x02\x03xxxxxxxxxxxxxxxxxxxxxxxxx");
        test_BindToObject(FILE_TEST, 0, REGDB_E_CLASSNOTREG);
        create_html_file();

        trace("file test (no callback)...\n");
        test_BindToStorage(FILE_TEST, BINDTEST_NO_CALLBACK, TYMED_ISTREAM);

        if(can_do_https()) {
            trace("synchronous https test (invalid CN, dialog)\n");
            onsecurityproblem_hres = S_FALSE;
            http_is_first = TRUE;
            test_BindToStorage(HTTPS_TEST, BINDTEST_INVALID_CN, TYMED_ISTREAM);

            bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

            trace("asynchronous https test (invalid CN, fail)\n");
            onsecurityproblem_hres = E_FAIL;
            test_BindToStorage(HTTPS_TEST, BINDTEST_INVALID_CN, TYMED_ISTREAM);

            trace("asynchronous https test (invalid CN, accept)\n");
            onsecurityproblem_hres = S_OK;
            test_BindToStorage(HTTPS_TEST, BINDTEST_INVALID_CN, TYMED_ISTREAM);

            trace("asynchronous https test (invalid CN, dialog 2)\n");
            onsecurityproblem_hres = S_FALSE;
            test_BindToStorage(HTTPS_TEST, BINDTEST_INVALID_CN, TYMED_ISTREAM);
            invalid_cn_accepted = FALSE;

            trace("asynchronous https test...\n");
            test_BindToStorage(HTTPS_TEST, 0, TYMED_ISTREAM);
        }else {
            win_skip("Skipping https tests\n");
        }

        bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

        trace("winetest test (async switch)...\n");
        test_BindToStorage(WINETEST_TEST, BINDTEST_EMULATE|BINDTEST_ASYNC_SWITCH, TYMED_ISTREAM);

        trace("about test (no read)...\n");
        test_BindToStorage(ABOUT_TEST, BINDTEST_NO_CALLBACK_READ, TYMED_ISTREAM);

        trace("http test...\n");
        test_BindToStorage(HTTP_TEST, 0, TYMED_ISTREAM);

        trace("http test (to file)...\n");
        test_BindToStorage(HTTP_TEST, 0, TYMED_FILE);

        trace("http test (to object)...\n");
        test_BindToObject(HTTP_TEST, 0, S_OK);

        trace("http test (short response)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_HTTPRESPONSE|BINDTEST_ALLOW_FINDINGRESOURCE, TYMED_ISTREAM);

        trace("http test (short response, to object)...\n");
        test_BindToObject(HTTP_TEST, 0, S_OK);

        trace("http test (abort start binding E_NOTIMPL)...\n");
        abort_hres = E_NOTIMPL;
        test_BindToStorage(HTTP_TEST, BINDTEST_ABORT_START, TYMED_FILE);

        trace("http test (abort start binding E_ABORT)...\n");
        abort_hres = E_ABORT;
        test_BindToStorage(HTTP_TEST, BINDTEST_ABORT_START, TYMED_FILE);

        trace("http test (abort progress)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_ABORT_PROGRESS|BINDTEST_ALLOW_FINDINGRESOURCE, TYMED_FILE);

        trace("emulated http test...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);

        trace("emulated http test (to object)...\n");
        test_BindToObject(HTTP_TEST, BINDTEST_EMULATE, S_OK);

        trace("emulated http test (to object, redirect)...\n");
        test_BindToObject(HTTP_TEST, BINDTEST_EMULATE|BINDTEST_REDIRECT, S_OK);

        trace("emulated http test (to file)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE, TYMED_FILE);

        trace("emulated http test (redirect)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE|BINDTEST_REDIRECT, TYMED_ISTREAM);

        trace("emulated http test (with cache)...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE|BINDTEST_USE_CACHE, TYMED_ISTREAM);

        trace("winetest test (no callback)...\n");
        test_BindToStorage(WINETEST_TEST, BINDTEST_EMULATE|BINDTEST_NO_CALLBACK|BINDTEST_USE_CACHE, TYMED_ISTREAM);

        trace("emulated https test...\n");
        test_BindToStorage(HTTPS_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);

        trace("about test...\n");
        test_BindToStorage(ABOUT_TEST, 0, TYMED_ISTREAM);

        trace("about test (to file)...\n");
        test_BindToStorage(ABOUT_TEST, 0, TYMED_FILE);

        trace("about test (to object)...\n");
        test_BindToObject(ABOUT_TEST, 0, S_OK);

        trace("emulated about test...\n");
        test_BindToStorage(ABOUT_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);

        trace("emulated about test (to file)...\n");
        test_BindToStorage(ABOUT_TEST, BINDTEST_EMULATE, TYMED_FILE);

        trace("emulated about test (to object)...\n");
        test_BindToObject(ABOUT_TEST, BINDTEST_EMULATE, S_OK);

        trace("emulated test reporting result in read...\n");
        test_BindToStorage(WINETEST_SYNC_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);

        trace("file test...\n");
        test_BindToStorage(FILE_TEST, 0, TYMED_ISTREAM);

        trace("file test (to file)...\n");
        test_BindToStorage(FILE_TEST, 0, TYMED_FILE);

        trace("file test (to object)...\n");
        test_BindToObject(FILE_TEST, 0, S_OK);

        trace("emulated file test...\n");
        test_BindToStorage(FILE_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);

        trace("emulated file test (to file)...\n");
        test_BindToStorage(FILE_TEST, BINDTEST_EMULATE, TYMED_FILE);

        trace("emulated file test (to object)...\n");
        test_BindToObject(FILE_TEST, BINDTEST_EMULATE, S_OK);

        trace("emulated its test...\n");
        test_BindToStorage(ITS_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);

        trace("emulated its test (to file)...\n");
        test_BindToStorage(ITS_TEST, BINDTEST_EMULATE, TYMED_FILE);

        trace("emulated mk test...\n");
        test_BindToStorage(MK_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);

        trace("test URLDownloadToFile for file protocol...\n");
        test_URLDownloadToFile(FILE_TEST, FALSE);

        trace("test URLDownloadToFile for emulated file protocol...\n");
        test_URLDownloadToFile(FILE_TEST, TRUE);

        trace("test URLDownloadToFile for http protocol...\n");
        test_URLDownloadToFile(HTTP_TEST, FALSE);

        trace("test URLDownloadToFile abort...\n");
        test_URLDownloadToFile_abort();

        trace("test emulated http abort...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE|BINDTEST_ABORT, TYMED_ISTREAM);

        bindf |= BINDF_NOWRITECACHE;

        trace("ftp test...\n");
        test_BindToStorage(FTP_TEST, 0, TYMED_ISTREAM);

        trace("test failures...\n");
        test_BindToStorage_fail();

        bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE;
        only_check_prot_args = TRUE; /* Fail after checking arguments to Protocol_Start */

        trace("check emulated http protocol arguments...\n");
        test_BindToStorage(HTTP_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);
    }

    DeleteFileA(test_file);
    DeleteFileA(test_txtA);
    CloseHandle(complete_event);
    CloseHandle(complete_event2);
    CoUninitialize();
}
