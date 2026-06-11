/*
 * Copyright 2005-2011 Jacek Caban for CodeWeavers
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
#define CONST_VTABLE

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"
#include "wininet.h"

static HRESULT (WINAPI *pCoInternetGetSession)(DWORD, IInternetSession **, DWORD);
static HRESULT (WINAPI *pReleaseBindInfo)(BINDINFO*);
static HRESULT (WINAPI *pCreateUri)(LPCWSTR, DWORD, DWORD_PTR, IUri**);

#define CHARS_IN_GUID 39

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func  "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func);     \
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

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(ReportProgress_DIRECTBIND);
DEFINE_EXPECT(ReportProgress_RAWMIMETYPE);
DEFINE_EXPECT(ReportProgress_FINDINGRESOURCE);
DEFINE_EXPECT(ReportProgress_CONNECTING);
DEFINE_EXPECT(ReportProgress_SENDINGREQUEST);
DEFINE_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
DEFINE_EXPECT(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
DEFINE_EXPECT(ReportProgress_PROTOCOLCLASSID);
DEFINE_EXPECT(ReportProgress_COOKIE_SENT);
DEFINE_EXPECT(ReportProgress_REDIRECTING);
DEFINE_EXPECT(ReportProgress_ENCODING);
DEFINE_EXPECT(ReportProgress_ACCEPTRANGES);
DEFINE_EXPECT(ReportProgress_PROXYDETECTING);
DEFINE_EXPECT(ReportProgress_LOADINGMIMEHANDLER);
DEFINE_EXPECT(ReportProgress_DECODING);
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportData2);
DEFINE_EXPECT(ReportResult);
DEFINE_EXPECT(GetBindString_ACCEPT_MIMES);
DEFINE_EXPECT(GetBindString_USER_AGENT);
DEFINE_EXPECT(GetBindString_POST_COOKIE);
DEFINE_EXPECT(GetBindString_URL);
DEFINE_EXPECT(GetBindString_ROOTDOC_URL);
DEFINE_EXPECT(GetBindString_SAMESITE_COOKIE_LEVEL);
DEFINE_EXPECT(QueryService_HttpNegotiate);
DEFINE_EXPECT(QueryService_InternetProtocol);
DEFINE_EXPECT(QueryService_HttpSecurity);
DEFINE_EXPECT(QueryService_IBindCallbackRedirect);
DEFINE_EXPECT(QueryInterface_IWinInetInfo);
DEFINE_EXPECT(QueryInterface_IWinInetHttpInfo);
DEFINE_EXPECT(BeginningTransaction);
DEFINE_EXPECT(GetRootSecurityId);
DEFINE_EXPECT(OnResponse);
DEFINE_EXPECT(Switch);
DEFINE_EXPECT(Continue);
DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(CreateInstance_no_aggregation);
DEFINE_EXPECT(Start);
DEFINE_EXPECT(StartEx);
DEFINE_EXPECT(Terminate);
DEFINE_EXPECT(Read);
DEFINE_EXPECT(Read2);
DEFINE_EXPECT(SetPriority);
DEFINE_EXPECT(LockRequest);
DEFINE_EXPECT(UnlockRequest);
DEFINE_EXPECT(Abort);
DEFINE_EXPECT(MimeFilter_CreateInstance);
DEFINE_EXPECT(MimeFilter_Start);
DEFINE_EXPECT(MimeFilter_ReportData);
DEFINE_EXPECT(MimeFilter_ReportResult);
DEFINE_EXPECT(MimeFilter_Terminate);
DEFINE_EXPECT(MimeFilter_LockRequest);
DEFINE_EXPECT(MimeFilter_UnlockRequest);
DEFINE_EXPECT(MimeFilter_Read);
DEFINE_EXPECT(MimeFilter_Switch);
DEFINE_EXPECT(MimeFilter_Continue);
DEFINE_EXPECT(Stream_Seek);
DEFINE_EXPECT(Stream_Read);
DEFINE_EXPECT(Redirect);
DEFINE_EXPECT(outer_QI_test);
DEFINE_EXPECT(Protocol_destructor);

static const WCHAR wszIndexHtml[] = {'i','n','d','e','x','.','h','t','m','l',0};
static const WCHAR index_url[] =
    {'f','i','l','e',':','i','n','d','e','x','.','h','t','m','l',0};

static const WCHAR acc_mimeW[] = {'*','/','*',0};
static const WCHAR user_agentW[] = {'W','i','n','e',0};
static const WCHAR text_htmlW[] = {'t','e','x','t','/','h','t','m','l',0};
static const WCHAR hostW[] = {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR winehq_ipW[] = {'2','0','9','.','4','6','.','2','5','.','1','3','4',0};
static const WCHAR emptyW[] = {0};
static const WCHAR pjpegW[] = {'i','m','a','g','e','/','p','j','p','e','g',0};
static const WCHAR gifW[] = {'i','m','a','g','e','/','g','i','f',0};
static const WCHAR null_guid[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-',
    '0','0','0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0','0','}',0};
static WCHAR protocol_clsid[CHARS_IN_GUID];

static HRESULT expect_hrResult;
static LPCWSTR file_name, http_url, expect_wsz;
static IInternetProtocol *async_protocol = NULL;
static BOOL first_data_notif, http_is_first, test_redirect, redirect_on_continue;
static int prot_state, read_report_data, post_stream_read;
static DWORD bindf, ex_priority , pi, bindinfo_options;
static IInternetProtocol *binding_protocol, *filtered_protocol;
static IInternetBindInfo *prot_bind_info;
static IInternetProtocolSink *binding_sink, *filtered_sink;
static void *expect_pv;
static HANDLE event_complete, event_complete2, event_continue, event_continue_done;
static BOOL binding_test;
static PROTOCOLDATA protocoldata, *pdata, continue_protdata;
static DWORD prot_read, filter_state, http_post_test, thread_id;
static BOOL security_problem, test_async_req, impl_protex;
static BOOL async_read_pending, mimefilter_test, direct_read, wait_for_switch, emulate_prot, short_read, test_abort;
static BOOL empty_file, no_mime, bind_from_cache, file_with_hash, reuse_protocol_thread;
static BOOL no_aggregation, result_from_lock;

enum {
    STATE_CONNECTING,
    STATE_SENDINGREQUEST,
    STATE_STARTDOWNLOADING,
    STATE_DOWNLOADING
} state;

static enum {
    FILE_TEST,
    HTTP_TEST,
    HTTPS_TEST,
    FTP_TEST,
    MK_TEST,
    ITS_TEST,
    BIND_TEST
} tested_protocol;

typedef struct {
    IUnknown IUnknown_inner;
    IInternetProtocolEx IInternetProtocolEx_iface;
    IInternetPriority IInternetPriority_iface;
    IUnknown *outer;
    LONG inner_ref;
    LONG outer_ref;
} Protocol;

static Protocol *protocol_emul;

static const WCHAR protocol_names[][10] = {
    {'f','i','l','e',0},
    {'h','t','t','p',0},
    {'h','t','t','p','s',0},
    {'f','t','p',0},
    {'m','k',0},
    {'i','t','s',0},
    {'t','e','s','t',0}
};

static const WCHAR binding_urls[][130] = {
    {'f','i','l','e',':','t','e','s','t','.','h','t','m','l',0},
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.',
     'o','r','g','/','s','i','t','e','/','a','b','o','u','t',0},
    {'h','t','t','p','s',':','/','/','w','w','w','.','c','o','d','e','w','e','a','v','e','r','s',
     '.','c','o','m','/','t','e','s','t','.','h','t','m','l',0},
    {'f','t','p',':','/','/','f','t','p','.','w','i','n','e','h','q','.','o','r','g',
     '/','p','u','b','/','o','t','h','e','r',
     '/','w','i','n','e','l','o','g','o','.','x','c','f','.','t','a','r','.','b','z','2',0},
    {'m','k',':','t','e','s','t',0},
    {'i','t','s',':','t','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0},
    {'t','e','s','t',':','/','/','f','i','l','e','.','h','t','m','l',0}
};

static const CHAR post_data[] = "mode=Test";

static LONG obj_refcount(void *obj)
{
    IUnknown_AddRef((IUnknown *)obj);
    return IUnknown_Release((IUnknown *)obj);
}

static const char *w2a(LPCWSTR str)
{
    static char buf[INTERNET_MAX_URL_LENGTH];
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static HRESULT WINAPI HttpSecurity_QueryInterface(IHttpSecurity *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)
            || IsEqualGUID(&IID_IHttpSecurity, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
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

static  HRESULT WINAPI HttpSecurity_GetWindow(IHttpSecurity* iface, REFGUID rguidReason, HWND *phwnd)
{
    if(winetest_debug > 1) trace("HttpSecurity_GetWindow\n");

    return S_FALSE;
}

static HRESULT WINAPI HttpSecurity_OnSecurityProblem(IHttpSecurity *iface, DWORD dwProblem)
{
    win_skip("Security problem: %lu\n", dwProblem);
    ok(dwProblem == ERROR_INTERNET_SEC_CERT_REV_FAILED || dwProblem == ERROR_INTERNET_INVALID_CA,
       "Expected got %lu security problem\n", dwProblem);

    /* Only retry once */
    if (security_problem)
        return E_ABORT;

    security_problem = TRUE;
    if(dwProblem == ERROR_INTERNET_INVALID_CA)
        return E_ABORT;
    SET_EXPECT(BeginningTransaction);

    return RPC_E_RETRY;
}

static IHttpSecurityVtbl HttpSecurityVtbl = {
    HttpSecurity_QueryInterface,
    HttpSecurity_AddRef,
    HttpSecurity_Release,
    HttpSecurity_GetWindow,
    HttpSecurity_OnSecurityProblem
};

static IHttpSecurity http_security = { &HttpSecurityVtbl };

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
    LPWSTR addl_headers;

    static const WCHAR wszHeaders[] =
        {'C','o','n','t','e','n','t','-','T','y','p','e',':',' ','a','p','p','l','i','c','a','t',
         'i','o','n','/','x','-','w','w','w','-','f','o','r','m','-','u','r','l','e','n','c','o',
         'd','e','d','\r','\n',0};

    CHECK_EXPECT(BeginningTransaction);

    if(binding_test)
        ok(!lstrcmpW(szURL, binding_urls[tested_protocol]), "szURL != http_url\n");
    else
        ok(!lstrcmpW(szURL, http_url), "szURL != http_url\n");
    ok(!dwReserved, "dwReserved=%ld, expected 0\n", dwReserved);
    ok(pszAdditionalHeaders != NULL, "pszAdditionalHeaders == NULL\n");
    if(pszAdditionalHeaders)
    {
        ok(*pszAdditionalHeaders == NULL, "*pszAdditionalHeaders != NULL\n");
        if (http_post_test)
        {
            addl_headers = CoTaskMemAlloc(sizeof(wszHeaders));
            memcpy(addl_headers, wszHeaders, sizeof(wszHeaders));
            *pszAdditionalHeaders = addl_headers;
        }
    }

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    CHECK_EXPECT(OnResponse);

    ok(dwResponseCode == 200, "dwResponseCode=%ld, expected 200\n", dwResponseCode);
    ok(szResponseHeaders != NULL, "szResponseHeaders == NULL\n");
    ok(szRequestHeaders == NULL, "szRequestHeaders != NULL\n");
    ok(pszAdditionalRequestHeaders == NULL, "pszAdditionalHeaders != NULL\n");

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_GetRootSecurityId(IHttpNegotiate2 *iface,
        BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    static const BYTE sec_id[] = {'h','t','t','p',':','t','e','s','t',1,0,0,0};

    CHECK_EXPECT(GetRootSecurityId);

    ok(!dwReserved, "dwReserved=%Id, expected 0\n", dwReserved);
    ok(pbSecurityId != NULL, "pbSecurityId == NULL\n");
    ok(pcbSecurityId != NULL, "pcbSecurityId == NULL\n");

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

static IHttpNegotiate2 http_negotiate = { &HttpNegotiateVtbl };

static HRESULT WINAPI BindCallbackRedirect_QueryInterface(IBindCallbackRedirect *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BindCallbackRedirect_AddRef(IBindCallbackRedirect *iface)
{
    return 2;
}

static ULONG WINAPI BindCallbackRedirect_Release(IBindCallbackRedirect *iface)
{
    return 1;
}

static HRESULT WINAPI BindCallbackRedirect_Redirect(IBindCallbackRedirect *iface, const WCHAR *url, VARIANT_BOOL *cancel)
{
    CHECK_EXPECT(Redirect);
    *cancel = VARIANT_FALSE;
    return S_OK;
}

static const IBindCallbackRedirectVtbl BindCallbackRedirectVtbl = {
    BindCallbackRedirect_QueryInterface,
    BindCallbackRedirect_AddRef,
    BindCallbackRedirect_Release,
    BindCallbackRedirect_Redirect
};

static IBindCallbackRedirect redirect_callback = { &BindCallbackRedirectVtbl };

static HRESULT QueryInterface(REFIID,void**);

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IHttpNegotiate, guidService) || IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        CHECK_EXPECT2(QueryService_HttpNegotiate);
        return IHttpNegotiate2_QueryInterface(&http_negotiate, riid, ppv);
    }

    if(IsEqualGUID(&IID_IInternetProtocol, guidService)) {
        ok(IsEqualGUID(&IID_IInternetProtocol, riid), "unexpected riid\n");
        CHECK_EXPECT(QueryService_InternetProtocol);
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(&IID_IHttpSecurity, guidService)) {
        ok(IsEqualGUID(&IID_IHttpSecurity, riid), "unexpected riid\n");
        CHECK_EXPECT(QueryService_HttpSecurity);
        return IHttpSecurity_QueryInterface(&http_security, riid, ppv);
    }

    if(IsEqualGUID(&IID_IBindCallbackRedirect, guidService)) {
        CHECK_EXPECT(QueryService_IBindCallbackRedirect);
        ok(IsEqualGUID(&IID_IBindCallbackRedirect, riid), "riid = %s\n", wine_dbgstr_guid(riid));
        *ppv = &redirect_callback;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IGetBindHandle, guidService)) {
        if(winetest_debug > 1) trace("QueryService(IID_IGetBindHandle)\n");
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(&IID_IWindowForBindingUI, guidService)) {
        if(winetest_debug > 1) trace("QueryService(IID_IWindowForBindingUI)\n");
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ok(0, "unexpected service %s\n", wine_dbgstr_guid(guidService));
    return E_FAIL;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider service_provider = { &ServiceProviderVtbl };

static HRESULT WINAPI Stream_QueryInterface(IStream *iface, REFIID riid, void **ppv)
{
    static const IID IID_strm_unknown = {0x2f68429a,0x199a,0x4043,{0x93,0x11,0xf2,0xfe,0x7c,0x13,0xcc,0xb9}};

    if(!IsEqualGUID(&IID_strm_unknown, riid)) /* IE11 */
        ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Stream_AddRef(IStream *iface)
{
    return 2;
}

static ULONG WINAPI Stream_Release(IStream *iface)
{
    return 1;
}

static HRESULT WINAPI Stream_Read(IStream *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    CHECK_EXPECT2(Stream_Read);

    ok(GetCurrentThreadId() != thread_id, "wrong thread %ld\n", GetCurrentThreadId());

    ok(pv != NULL, "pv == NULL\n");
    ok(cb == 0x20000 || broken(cb == 0x2000), "cb = %ld\n", cb);
    ok(pcbRead != NULL, "pcbRead == NULL\n");

    if(post_stream_read) {
        *pcbRead = 0;
        return S_FALSE;
    }

    memcpy(pv, post_data, sizeof(post_data)-1);
    post_stream_read += *pcbRead = sizeof(post_data)-1;
    return S_OK;
}

static HRESULT WINAPI Stream_Write(IStream *iface, const void *pv,
        ULONG cb, ULONG *pcbWritten)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Seek(IStream *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    CHECK_EXPECT(Stream_Seek);

    ok(!dlibMove.QuadPart, "dlibMove != 0\n");
    ok(dwOrigin == STREAM_SEEK_SET, "dwOrigin = %ld\n", dwOrigin);
    ok(!plibNewPosition, "plibNewPosition == NULL\n");

    return S_OK;
}

static HRESULT WINAPI Stream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_CopyTo(IStream *iface, IStream *pstm,
        ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Revert(IStream *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_UnlockRegion(IStream *iface,
        ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Stat(IStream *iface, STATSTG *pstatstg,
        DWORD dwStatFlag)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Clone(IStream *iface, IStream **ppstm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IStreamVtbl StreamVtbl = {
    Stream_QueryInterface,
    Stream_AddRef,
    Stream_Release,
    Stream_Read,
    Stream_Write,
    Stream_Seek,
    Stream_SetSize,
    Stream_CopyTo,
    Stream_Commit,
    Stream_Revert,
    Stream_LockRegion,
    Stream_UnlockRegion,
    Stream_Stat,
    Stream_Clone
};

static IStream Stream = { &StreamVtbl };

static HRESULT WINAPI ProtocolSink_QueryInterface(IInternetProtocolSink *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    return 2;
}

static ULONG WINAPI ProtocolSink_Release(IInternetProtocolSink *iface)
{
    return 1;
}

static void call_continue(PROTOCOLDATA *protocol_data)
{
    HRESULT hres;

    if (winetest_debug > 1)
        if(winetest_debug > 1) trace("continue in state %d\n", state);

    if(state == STATE_CONNECTING) {
        if(tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST || tested_protocol == FTP_TEST) {
            if (http_is_first){
                CLEAR_CALLED(ReportProgress_FINDINGRESOURCE);
                CLEAR_CALLED(ReportProgress_PROXYDETECTING);
            }
            CLEAR_CALLED(ReportProgress_CONNECTING);
        }
        if(tested_protocol == FTP_TEST)
            todo_wine CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        else if (tested_protocol != HTTPS_TEST)
            CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        if(test_redirect && !(bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS)) {
            CHECK_CALLED(ReportProgress_REDIRECTING);
            CLEAR_CALLED(GetBindString_SAMESITE_COOKIE_LEVEL); /* New in IE11 */
        }
        state = test_async_req ? STATE_SENDINGREQUEST : STATE_STARTDOWNLOADING;
    }

    switch(state) {
    case STATE_SENDINGREQUEST:
        SET_EXPECT(Stream_Read);
        SET_EXPECT(ReportProgress_SENDINGREQUEST);
        break;
    case STATE_STARTDOWNLOADING:
        if((tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST)
           && (!test_redirect || !(bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS))) {
            SET_EXPECT(OnResponse);
            if(tested_protocol == HTTPS_TEST || test_redirect || test_abort || empty_file)
                SET_EXPECT(ReportProgress_ACCEPTRANGES);
            SET_EXPECT(ReportProgress_ENCODING);
            SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
            if(bindf & BINDF_NEEDFILE)
                SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        }
    default:
        break;
    }

    if(state != STATE_SENDINGREQUEST && (!test_redirect || !(bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS)))
        SET_EXPECT(ReportData);
    hres = IInternetProtocol_Continue(async_protocol, protocol_data);
    ok(hres == S_OK, "Continue failed: %08lx\n", hres);
    if(tested_protocol == FTP_TEST || security_problem)
        CLEAR_CALLED(ReportData);
    else if(state != STATE_SENDINGREQUEST && (!test_redirect || !(bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS)))
        CHECK_CALLED(ReportData);

    switch(state) {
    case STATE_SENDINGREQUEST:
        CHECK_CALLED(Stream_Read);
        CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        state = STATE_STARTDOWNLOADING;
        break;
    case STATE_STARTDOWNLOADING:
        if(!security_problem) {
            state = STATE_DOWNLOADING;
            if((tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST)
               && (!test_redirect || !(bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS))) {
                CHECK_CALLED(OnResponse);
                if(tested_protocol == HTTPS_TEST || empty_file)
                    CHECK_CALLED(ReportProgress_ACCEPTRANGES);
                else if(test_redirect || test_abort)
                    CLEAR_CALLED(ReportProgress_ACCEPTRANGES);
                CLEAR_CALLED(ReportProgress_ENCODING);
                CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
                if(bindf & BINDF_NEEDFILE)
                    CHECK_CALLED(ReportProgress_CACHEFILENAMEAVAILABLE);
            }
        }
        else
        {
            security_problem = FALSE;
            SET_EXPECT(ReportProgress_CONNECTING);
        }
    default:
        break;
    }
}

static HRESULT WINAPI ProtocolSink_Switch(IInternetProtocolSink *iface, PROTOCOLDATA *pProtocolData)
{
    if(tested_protocol == FTP_TEST)
        CHECK_EXPECT2(Switch);
    else
        CHECK_EXPECT(Switch);

    ok(pProtocolData != NULL, "pProtocolData == NULL\n");
    if(binding_test) {
        ok(pProtocolData != &protocoldata, "pProtocolData == &protocoldata\n");
        ok(pProtocolData->grfFlags == protocoldata.grfFlags, "grfFlags wrong %lx/%lx\n",
           pProtocolData->grfFlags, protocoldata.grfFlags );
        ok(pProtocolData->dwState == protocoldata.dwState, "dwState wrong %lx/%lx\n",
           pProtocolData->dwState, protocoldata.dwState );
        ok(pProtocolData->pData == protocoldata.pData, "pData wrong %p/%p\n",
           pProtocolData->pData, protocoldata.pData );
        ok(pProtocolData->cbData == protocoldata.cbData, "cbData wrong %lx/%lx\n",
           pProtocolData->cbData, protocoldata.cbData );
    }

    pdata = pProtocolData;

    if(binding_test) {
        SetEvent(event_complete);
        ok( WaitForSingleObject(event_complete2, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
        return S_OK;
    }if(direct_read) {
        continue_protdata = *pProtocolData;
        SetEvent(event_continue);
        ok( WaitForSingleObject(event_continue_done, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
    }else {
        call_continue(pProtocolData);
        SetEvent(event_complete);
    }

    return S_OK;
}

static const char *status_names[] =
{
    "0",
    "FINDINGRESOURCE",
    "CONNECTING",
    "REDIRECTING",
    "BEGINDOWNLOADDATA",
    "DOWNLOADINGDATA",
    "ENDDOWNLOADDATA",
    "BEGINDOWNLOADCOMPONENTS",
    "INSTALLINGCOMPONENTS",
    "ENDDOWNLOADCOMPONENTS",
    "USINGCACHEDCOPY",
    "SENDINGREQUEST",
    "CLASSIDAVAILABLE",
    "MIMETYPEAVAILABLE",
    "CACHEFILENAMEAVAILABLE",
    "BEGINSYNCOPERATION",
    "ENDSYNCOPERATION",
    "BEGINUPLOADDATA",
    "UPLOADINGDATA",
    "ENDUPLOADINGDATA",
    "PROTOCOLCLASSID",
    "ENCODING",
    "VERIFIEDMIMETYPEAVAILABLE",
    "CLASSINSTALLLOCATION",
    "DECODING",
    "LOADINGMIMEHANDLER",
    "CONTENTDISPOSITIONATTACH",
    "FILTERREPORTMIMETYPE",
    "CLSIDCANINSTANTIATE",
    "IUNKNOWNAVAILABLE",
    "DIRECTBIND",
    "RAWMIMETYPE",
    "PROXYDETECTING",
    "ACCEPTRANGES",
    "COOKIE_SENT",
    "COMPACT_POLICY_RECEIVED",
    "COOKIE_SUPPRESSED",
    "COOKIE_STATE_UNKNOWN",
    "COOKIE_STATE_ACCEPT",
    "COOKIE_STATE_REJECT",
    "COOKIE_STATE_PROMPT",
    "COOKIE_STATE_LEASH",
    "COOKIE_STATE_DOWNGRADE",
    "POLICY_HREF",
    "P3P_HEADER",
    "SESSION_COOKIE_RECEIVED",
    "PERSISTENT_COOKIE_RECEIVED",
    "SESSION_COOKIES_ALLOWED",
    "CACHECONTROL",
    "CONTENTDISPOSITIONFILENAME",
    "MIMETEXTPLAINMISMATCH",
    "PUBLISHERAVAILABLE",
    "DISPLAYNAMEAVAILABLE"
};

static HRESULT WINAPI ProtocolSink_ReportProgress(IInternetProtocolSink *iface, ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    static const WCHAR text_plain[] = {'t','e','x','t','/','p','l','a','i','n',0};

    if (winetest_debug > 1)
    {
        if (ulStatusCode < ARRAY_SIZE(status_names))
            trace( "progress: %s %s\n", status_names[ulStatusCode], wine_dbgstr_w(szStatusText) );
        else
            trace( "progress: %lu %s\n", ulStatusCode, wine_dbgstr_w(szStatusText) );
    }

    switch(ulStatusCode) {
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT2(ReportProgress_MIMETYPEAVAILABLE);
        if(tested_protocol != FILE_TEST && tested_protocol != ITS_TEST && !mimefilter_test && (pi & PI_MIMEVERIFICATION)) {
            if(!short_read || !direct_read)
                CHECK_CALLED(Read); /* set in Continue */
            else if(short_read)
                CHECK_CALLED(Read2); /* set in Read */
        }
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText) {
            if(tested_protocol == BIND_TEST)
                ok(!lstrcmpW(szStatusText, expect_wsz), "unexpected szStatusText %s\n", wine_dbgstr_w(szStatusText));
            else if (http_post_test)
                ok(lstrlenW(text_plain) <= lstrlenW(szStatusText) &&
                   !memcmp(szStatusText, text_plain, lstrlenW(text_plain)*sizeof(WCHAR)),
                   "szStatusText != text/plain\n");
            else if(empty_file)
                ok(!lstrcmpW(szStatusText, L"text/javascript"), "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
            else if((pi & PI_MIMEVERIFICATION) && emulate_prot && !mimefilter_test
                    && tested_protocol==HTTP_TEST && !short_read)
                ok(lstrlenW(gifW) <= lstrlenW(szStatusText) &&
                   !memcmp(szStatusText, gifW, lstrlenW(gifW)*sizeof(WCHAR)),
                   "szStatusText != image/gif\n");
            else if(!mimefilter_test)
                ok(lstrlenW(text_htmlW) <= lstrlenW(szStatusText) &&
                   !memcmp(szStatusText, text_htmlW, lstrlenW(text_htmlW)*sizeof(WCHAR)),
                   "szStatusText != text/html\n");
        }
        break;
    case BINDSTATUS_DIRECTBIND:
        CHECK_EXPECT2(ReportProgress_DIRECTBIND);
        ok(szStatusText == NULL, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_RAWMIMETYPE:
        CHECK_EXPECT2(ReportProgress_RAWMIMETYPE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(lstrlenW(szStatusText) < lstrlenW(text_htmlW) ||
               !memcmp(szStatusText, text_htmlW, lstrlenW(text_htmlW)*sizeof(WCHAR)),
               "szStatusText != text/html\n");
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        CHECK_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText) {
            if(binding_test)
                ok(!lstrcmpW(szStatusText, expect_wsz), "unexpected szStatusText\n");
            else if(tested_protocol == FILE_TEST)
                ok(!lstrcmpW(szStatusText, file_name), "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
            else
                ok(szStatusText != NULL, "szStatusText == NULL\n");
        }
        break;
    case BINDSTATUS_FINDINGRESOURCE:
        CHECK_EXPECT2(ReportProgress_FINDINGRESOURCE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        break;
    case BINDSTATUS_CONNECTING:
        CHECK_EXPECT2(ReportProgress_CONNECTING);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        break;
    case BINDSTATUS_SENDINGREQUEST:
        CHECK_EXPECT2(ReportProgress_SENDINGREQUEST);
        if(tested_protocol == FILE_TEST || tested_protocol == ITS_TEST) {
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            if(szStatusText)
                ok(!*szStatusText, "wrong szStatusText\n");
        }
        break;
    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, L"text/html"), "szStatusText != text/html\n");
        break;
    case BINDSTATUS_PROTOCOLCLASSID:
        CHECK_EXPECT(ReportProgress_PROTOCOLCLASSID);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        ok(!lstrcmpW(szStatusText, protocol_clsid), "unexpected classid %s\n", wine_dbgstr_w(szStatusText));
        break;
    case BINDSTATUS_COOKIE_SENT:
        CHECK_EXPECT2(ReportProgress_COOKIE_SENT);
        ok(szStatusText == NULL, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_REDIRECTING:
        CHECK_EXPECT(ReportProgress_REDIRECTING);
        if(test_redirect)
            ok(!lstrcmpW(szStatusText, L"http://test.winehq.org/tests/hello.html"), "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
        else
            ok(szStatusText == NULL, "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
        break;
    case BINDSTATUS_ENCODING:
        CHECK_EXPECT(ReportProgress_ENCODING);
        ok(!lstrcmpW(szStatusText, L"gzip"), "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
        break;
    case BINDSTATUS_ACCEPTRANGES:
        CHECK_EXPECT(ReportProgress_ACCEPTRANGES);
        ok(!szStatusText, "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
        break;
    case BINDSTATUS_PROXYDETECTING:
        if(!called_ReportProgress_PROXYDETECTING)
            SET_EXPECT(ReportProgress_CONNECTING);
        CHECK_EXPECT2(ReportProgress_PROXYDETECTING);
        ok(!szStatusText, "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
        break;
    case BINDSTATUS_LOADINGMIMEHANDLER:
        CHECK_EXPECT(ReportProgress_LOADINGMIMEHANDLER);
        ok(!szStatusText, "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
        break;
    case BINDSTATUS_DECODING:
        CHECK_EXPECT(ReportProgress_DECODING);
        ok(!lstrcmpW(szStatusText, pjpegW), "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
        break;
    case BINDSTATUS_RESERVED_7:
        if(winetest_debug > 1) trace("BINDSTATUS_RESERVED_7\n");
        break;
    case BINDSTATUS_RESERVED_8:
        if(winetest_debug > 1) trace("BINDSTATUS_RESERVED_8\n");
        break;
    default:
        ok(0, "Unexpected status %ld (%ld)\n", ulStatusCode, ulStatusCode-BINDSTATUS_LAST);
    };

    return S_OK;
}

static void test_http_info(IInternetProtocol *protocol)
{
    IWinInetHttpInfo *info;
    char buf[1024];
    DWORD size, len;
    HRESULT hres;

    static const WCHAR connectionW[] = {'c','o','n','n','e','c','t','i','o','n',0};

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IWinInetHttpInfo, (void**)&info);
    ok(hres == S_OK, "Could not get IWinInterHttpInfo iface: %08lx\n", hres);

    size = sizeof(buf);
    strcpy(buf, "connection");
    hres = IWinInetHttpInfo_QueryInfo(info, HTTP_QUERY_CUSTOM, buf, &size, NULL, NULL);
    if(tested_protocol != FTP_TEST) {
        ok(hres == S_OK, "QueryInfo failed: %08lx\n", hres);

        ok(!strcmp(buf, "Keep-Alive") || !strcmp(buf, "Upgrade, Keep-Alive"), "buf = %s\n", buf);
        len = strlen(buf);
        ok(size == len, "size = %lu, expected %lu\n", size, len);

        size = sizeof(buf);
        memcpy(buf, connectionW, sizeof(connectionW));
        hres = IWinInetHttpInfo_QueryInfo(info, HTTP_QUERY_CUSTOM, buf, &size, NULL, NULL);
        ok(hres == S_FALSE, "QueryInfo returned %08lx\n", hres);
    }else {
        ok(hres == S_FALSE, "QueryInfo failed: %08lx\n", hres);
    }

    IWinInetHttpInfo_Release(info);
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF,
        ULONG ulProgress, ULONG ulProgressMax)
{
    HRESULT hres;

    static int rec_depth;
    rec_depth++;

    if(!mimefilter_test && (tested_protocol == FILE_TEST || tested_protocol == ITS_TEST)) {
        CHECK_EXPECT2(ReportData);

        ok(ulProgress == ulProgressMax, "ulProgress (%ld) != ulProgressMax (%ld)\n",
           ulProgress, ulProgressMax);
        if(!file_with_hash)
            ok(ulProgressMax == 13, "ulProgressMax=%ld, expected 13\n", ulProgressMax);
        /* BSCF_SKIPDRAINDATAFORFILEURLS added in IE8 */
        if(tested_protocol == FILE_TEST)
            ok((grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION)) ||
               (grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_SKIPDRAINDATAFORFILEURLS)),
               "grcfBSCF = %08lx\n", grfBSCF);
        else
            ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE), "grcfBSCF = %08lx\n", grfBSCF);
    }else if(bind_from_cache) {
        CHECK_EXPECT(ReportData);

        ok(grfBSCF == (BSCF_LASTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE), "grcfBSCF = %08lx\n", grfBSCF);
        ok(ulProgress == 1000, "ulProgress = %lu\n", ulProgress);
        ok(!ulProgressMax, "ulProgressMax = %lu\n", ulProgressMax);
    }else if(direct_read) {
        BYTE buf[14096];
        ULONG read;

        if(!read_report_data && rec_depth == 1) {
            BOOL reported_all_data = called_ReportData2;

            CHECK_EXPECT2(ReportData);

            if(short_read) {
                ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE)
                   || grfBSCF == BSCF_FIRSTDATANOTIFICATION, /* < IE8 */
                   "grcfBSCF = %08lx\n", grfBSCF);
                CHECK_CALLED(Read); /* Set in Continue */
                first_data_notif = FALSE;
            }else if(first_data_notif) {
                ok(grfBSCF == BSCF_FIRSTDATANOTIFICATION, "grcfBSCF = %08lx\n", grfBSCF);
                first_data_notif = FALSE;
            }else if(reported_all_data) {
                ok(grfBSCF == (BSCF_LASTDATANOTIFICATION|BSCF_INTERMEDIATEDATANOTIFICATION),
                   "grcfBSCF = %08lx\n", grfBSCF);
            }else if(!direct_read) {
                ok(grfBSCF == BSCF_INTERMEDIATEDATANOTIFICATION, "grcfBSCF = %08lx\n", grfBSCF);
            }

            do {
                read = 0;
                if(emulate_prot)
                    SET_EXPECT(Read);
                else
                    SET_EXPECT(ReportData2);
                SET_EXPECT(ReportResult);
                if(!emulate_prot)
                    SET_EXPECT(Switch);
                hres = IInternetProtocol_Read(binding_test ? binding_protocol : async_protocol, expect_pv = buf, sizeof(buf), &read);
                ok(hres == E_PENDING || hres == S_FALSE || hres == S_OK, "Read failed: %08lx\n", hres);
                if(hres == S_OK)
                    ok(read, "read == 0\n");
                if(reported_all_data)
                    ok(hres == S_FALSE, "Read failed: %08lx, expected S_FALSE\n", hres);
                if(!emulate_prot && hres != E_PENDING)
                    CHECK_NOT_CALLED(Switch); /* otherwise checked in wait_for_switch loop */
                if(emulate_prot)
                    CHECK_CALLED(Read);
                if(!reported_all_data && called_ReportData2) {
                    if(!emulate_prot)
                        CHECK_CALLED(ReportData2);
                    CHECK_CALLED(ReportResult);
                    reported_all_data = TRUE;
                }else {
                    if(!emulate_prot)
                        CHECK_NOT_CALLED(ReportData2);
                    CHECK_NOT_CALLED(ReportResult);
                }
            }while(hres == S_OK);
            if(hres == S_FALSE)
                wait_for_switch = FALSE;
        }else {
            CHECK_EXPECT(ReportData2);

            ok(grfBSCF & BSCF_LASTDATANOTIFICATION, "grfBSCF = %08lx\n", grfBSCF);

            read = 0xdeadbeef;
            if(emulate_prot)
                SET_EXPECT(Read2);
            hres = IInternetProtocol_Read(binding_test ? binding_protocol : async_protocol, expect_pv = buf, sizeof(buf), &read);
            if(emulate_prot)
                CHECK_CALLED(Read2);
            ok(hres == S_FALSE, "Read returned: %08lx, expected E_FALSE\n", hres);
            ok(!read, "read = %ld\n", read);
        }
    }else if(!binding_test && (tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST
            || tested_protocol == FTP_TEST)) {
        if(empty_file)
            CHECK_EXPECT2(ReportData);
        else if(!(grfBSCF & BSCF_LASTDATANOTIFICATION) || (grfBSCF & BSCF_DATAFULLYAVAILABLE))
            CHECK_EXPECT(ReportData);
        else if (http_post_test)
            ok(ulProgress == 13, "Read %lu bytes instead of 13\n", ulProgress);

        if(empty_file) {
            ok(!ulProgress, "ulProgress = %ld\n", ulProgress);
            ok(!ulProgressMax, "ulProgressMax = %ld\n", ulProgressMax);
        }else {
            ok(ulProgress, "ulProgress == 0\n");
        }

        if(empty_file) {
            ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION),
               "grcfBSCF = %08lx\n", grfBSCF);
            first_data_notif = FALSE;
        }else if(first_data_notif) {
            ok(grfBSCF == BSCF_FIRSTDATANOTIFICATION
               || grfBSCF == (BSCF_LASTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE),
               "grcfBSCF = %08lx\n", grfBSCF);
            first_data_notif = FALSE;
        } else {
            ok(grfBSCF == BSCF_INTERMEDIATEDATANOTIFICATION
               || grfBSCF == (BSCF_LASTDATANOTIFICATION|BSCF_INTERMEDIATEDATANOTIFICATION)
               || broken(grfBSCF == (BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION)),
               "grcfBSCF = %08lx\n", grfBSCF);
        }

        if((grfBSCF & BSCF_FIRSTDATANOTIFICATION) && !binding_test)
            test_http_info(async_protocol);

        if(!(bindf & BINDF_FROMURLMON) &&
           !(grfBSCF & BSCF_LASTDATANOTIFICATION)) {
            if(state == STATE_CONNECTING) {
                state = STATE_DOWNLOADING;
                if(http_is_first) {
                    CHECK_CALLED(ReportProgress_FINDINGRESOURCE);
                    CHECK_CALLED(ReportProgress_CONNECTING);
                }
                CHECK_CALLED(ReportProgress_SENDINGREQUEST);
                CHECK_CALLED(OnResponse);
                CHECK_CALLED(ReportProgress_RAWMIMETYPE);
            }
            SetEvent(event_complete);
        }
    }else if(!read_report_data) {
        BYTE buf[1000];
        ULONG read;
        HRESULT hres;

        CHECK_EXPECT(ReportData);

        if(tested_protocol != BIND_TEST) {
            do {
                if(mimefilter_test)
                    SET_EXPECT(MimeFilter_Read);
                else if(rec_depth > 1)
                    SET_EXPECT(Read2);
                else
                    SET_EXPECT(Read);
                hres = IInternetProtocol_Read(binding_protocol, expect_pv=buf, sizeof(buf), &read);
                if(mimefilter_test)
                    CHECK_CALLED(MimeFilter_Read);
                else if(rec_depth > 1)
                    CHECK_CALLED(Read2);
                else
                    CHECK_CALLED(Read);
            }while(hres == S_OK);
        }
    }

    if(result_from_lock) {
        SET_EXPECT(LockRequest);
        hres = IInternetProtocol_LockRequest(binding_protocol, 0);
        ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
        CHECK_CALLED(LockRequest);

        /* ReportResult is called before ReportData returns */
        SET_EXPECT(ReportResult);
    }

    rec_depth--;
    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult,
        DWORD dwError, LPCWSTR szResult)
{
    CHECK_EXPECT(ReportResult);

    if(security_problem)
        return S_OK;

    if(tested_protocol == FTP_TEST)
        ok(hrResult == E_PENDING || hrResult == S_OK, "hrResult = %08lx, expected E_PENDING or S_OK\n", hrResult);
    else
        ok(hrResult == expect_hrResult || (test_abort && hrResult == S_OK),  /* result can come in before the abort */
           "hrResult = %08lx, expected: %08lx\n", hrResult, expect_hrResult);
    if(SUCCEEDED(hrResult) || tested_protocol == FTP_TEST || test_abort || hrResult == INET_E_REDIRECT_FAILED)
        ok(dwError == ERROR_SUCCESS, "dwError = %ld, expected ERROR_SUCCESS\n", dwError);
    else
        ok(dwError != ERROR_SUCCESS ||
           broken(tested_protocol == MK_TEST), /* WinME and NT4 */
           "dwError == ERROR_SUCCESS\n");

    if(hrResult == INET_E_REDIRECT_FAILED)
        ok(!lstrcmpW(szResult, L"http://test.winehq.org/tests/hello.html"), "szResult = %s\n", wine_dbgstr_w(szResult));
    else
        ok(!szResult, "szResult = %s\n", wine_dbgstr_w(szResult));

    if(direct_read)
        SET_EXPECT(ReportData); /* checked after main loop */

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

static IInternetProtocolSink protocol_sink = { &protocol_sink_vtbl };

static HRESULT WINAPI MimeProtocolSink_QueryInterface(IInternetProtocolSink *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)
            || IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI MimeProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    return 2;
}

static ULONG WINAPI MimeProtocolSink_Release(IInternetProtocolSink *iface)
{
    return 1;
}

static HRESULT WINAPI MimeProtocolSink_Switch(IInternetProtocolSink *iface, PROTOCOLDATA *pProtocolData)
{
    HRESULT hres;

    CHECK_EXPECT(MimeFilter_Switch);

    SET_EXPECT(Switch);
    hres = IInternetProtocolSink_Switch(filtered_sink, pProtocolData);
    ok(hres == S_OK, "Switch failed: %08lx\n", hres);
    CHECK_CALLED(Switch);

    return S_OK;
}

static HRESULT WINAPI MimeProtocolSink_ReportProgress(IInternetProtocolSink *iface, ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    switch(ulStatusCode) {
    case BINDSTATUS_LOADINGMIMEHANDLER:
        /*
         * IE9 for some reason (bug?) calls this on mime handler's protocol sink instead of the
         * main protocol sink. We check ReportProgress_LOADINGMIMEHANDLER both here and in
         * ProtocolSink_ReportProgress to workaround it.
         */
        CHECK_EXPECT(ReportProgress_LOADINGMIMEHANDLER);
        ok(!szStatusText, "szStatusText = %s\n", wine_dbgstr_w(szStatusText));
        break;
    default:
        ok(0, "Unexpected status code %ld\n", ulStatusCode);
    }

    return S_OK;
}

static HRESULT WINAPI MimeProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF,
        ULONG ulProgress, ULONG ulProgressMax)
{
    DWORD read = 0;
    BYTE buf[8192];
    HRESULT hres;
    BOOL report_mime = FALSE;

    CHECK_EXPECT(MimeFilter_ReportData);

    if(!filter_state && !no_mime) {
        SET_EXPECT(Read);
        hres = IInternetProtocol_Read(filtered_protocol, buf, sizeof(buf), &read);
        if(tested_protocol == HTTP_TEST)
            ok(hres == S_OK || hres == E_PENDING || hres == S_FALSE, "Read failed: %08lx\n", hres);
        else
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
        CHECK_CALLED(Read);

        SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        hres = IInternetProtocolSink_ReportProgress(filtered_sink, BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, text_htmlW);
        ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);

        SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        hres = IInternetProtocolSink_ReportProgress(filtered_sink, BINDSTATUS_MIMETYPEAVAILABLE, text_htmlW);
        ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);

        /* FIXME: test BINDSTATUS_CACHEFILENAMEAVAILABLE */
    }

    if(no_mime && prot_read<200) {
        SET_EXPECT(Read);
    }else if(no_mime && prot_read<300) {
        report_mime = TRUE;
        SET_EXPECT(Read);
        SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(ReportData);
    }else if(!read_report_data) {
        SET_EXPECT(ReportData);
    }
    hres = IInternetProtocolSink_ReportData(filtered_sink, grfBSCF, ulProgress, ulProgressMax);
    ok(hres == S_OK, "ReportData failed: %08lx\n", hres);
    if(no_mime && prot_read<=200) {
        CHECK_CALLED(Read);
    }else if(report_mime) {
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(ReportData);
    }else if(!read_report_data) {
        CHECK_CALLED(ReportData);
    }

    if(!filter_state)
        filter_state = 1;

    return S_OK;
}

static HRESULT WINAPI MimeProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult,
        DWORD dwError, LPCWSTR szResult)
{
    HRESULT hres;

    CHECK_EXPECT(MimeFilter_ReportResult);

    ok(hrResult == S_OK, "hrResult = %08lx\n", hrResult);
    ok(dwError == ERROR_SUCCESS, "dwError = %lu\n", dwError);
    ok(!szResult, "szResult = %s\n", wine_dbgstr_w(szResult));

    SET_EXPECT(ReportResult);
    hres = IInternetProtocolSink_ReportResult(filtered_sink, hrResult, dwError, szResult);
    ok(SUCCEEDED(hres), "ReportResult failed: %08lx\n", hres);
    CHECK_CALLED(ReportResult);

    return S_OK;
}

static IInternetProtocolSinkVtbl mime_protocol_sink_vtbl = {
    MimeProtocolSink_QueryInterface,
    MimeProtocolSink_AddRef,
    MimeProtocolSink_Release,
    MimeProtocolSink_Switch,
    MimeProtocolSink_ReportProgress,
    MimeProtocolSink_ReportData,
    MimeProtocolSink_ReportResult
};

static IInternetProtocolSink mime_protocol_sink = { &mime_protocol_sink_vtbl };

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    static const IID IID_undocumented = {0x58DFC7D0,0x5381,0x43E5,{0x9D,0x72,0x4C,0xDD,0xE4,0xCB,0x0F,0x1A}};
    static const IID IID_undocumentedIE10 = {0xc28722e5,0xbc1a,0x4c55,{0xa6,0x8d,0x33,0x21,0x9f,0x69,0x89,0x10}};

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocolSink, riid))
        *ppv = &protocol_sink;
    if(IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &service_provider;
    if(IsEqualGUID(&IID_IUriContainer, riid))
        return E_NOINTERFACE; /* TODO */

    /* NOTE: IE8 queries for undocumented {58DFC7D0-5381-43E5-9D72-4CDDE4CB0F1A} interface. */
    if(IsEqualGUID(&IID_undocumented, riid))
        return E_NOINTERFACE;
    /* NOTE: IE10 queries for undocumented {c28722e5-bc1a-4c55-a68d-33219f698910} interface. */
    if(IsEqualGUID(&IID_undocumentedIE10, riid))
        return E_NOINTERFACE;

    if(*ppv)
        return S_OK;

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

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
    DWORD cbSize;

    CHECK_EXPECT(GetBindInfo);

    ok(grfBINDF != NULL, "grfBINDF == NULL\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %ld\n", pbindinfo->cbSize);

    *grfBINDF = bindf;
    if(binding_test)
        *grfBINDF |= BINDF_FROMURLMON;
    cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize);
    pbindinfo->cbSize = cbSize;
    pbindinfo->dwOptions = bindinfo_options;

    if(http_post_test)
    {
        pbindinfo->cbstgmedData = sizeof(post_data)-1;
        pbindinfo->dwBindVerb = BINDVERB_POST;
        pbindinfo->stgmedData.tymed = http_post_test;

        if(http_post_test == TYMED_HGLOBAL) {
            HGLOBAL data;

            /* Must be GMEM_FIXED, GMEM_MOVABLE does not work properly */
            data = GlobalAlloc(GPTR, sizeof(post_data));
            memcpy(data, post_data, sizeof(post_data));
            pbindinfo->stgmedData.hGlobal = data;
        }else {
            pbindinfo->stgmedData.pstm = &Stream;
        }
    }

    return S_OK;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface, ULONG ulStringType,
        LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    ok(ppwzStr != NULL, "ppwzStr == NULL\n");
    ok(pcElFetched != NULL, "pcElFetched == NULL\n");

    switch(ulStringType) {
    case BINDSTRING_ACCEPT_MIMES:
        CHECK_EXPECT(GetBindString_ACCEPT_MIMES);
        ok(cEl == 256, "cEl=%ld, expected 256\n", cEl);
        if(pcElFetched) {
            ok(*pcElFetched == 256, "*pcElFetched=%ld, expected 256\n", *pcElFetched);
            *pcElFetched = 1;
        }
        if(ppwzStr) {
            *ppwzStr = CoTaskMemAlloc(sizeof(acc_mimeW));
            memcpy(*ppwzStr, acc_mimeW, sizeof(acc_mimeW));
        }
        return S_OK;
    case BINDSTRING_USER_AGENT:
        CHECK_EXPECT(GetBindString_USER_AGENT);
        ok(cEl == 1, "cEl=%ld, expected 1\n", cEl);
        if(pcElFetched) {
            ok(*pcElFetched == 0, "*pcElFetch=%ld, expected 0\n", *pcElFetched);
            *pcElFetched = 1;
        }
        if(ppwzStr) {
            *ppwzStr = CoTaskMemAlloc(sizeof(user_agentW));
            memcpy(*ppwzStr, user_agentW, sizeof(user_agentW));
        }
        return S_OK;
    case BINDSTRING_POST_COOKIE:
        CHECK_EXPECT(GetBindString_POST_COOKIE);
        ok(cEl == 1, "cEl=%ld, expected 1\n", cEl);
        if(pcElFetched)
            ok(*pcElFetched == 0, "*pcElFetch=%ld, expected 0\n", *pcElFetched);
        return S_OK;
    case BINDSTRING_URL: {
        DWORD size;

        CHECK_EXPECT(GetBindString_URL);
        ok(cEl == 1, "cEl=%ld, expected 1\n", cEl);
        ok(*pcElFetched == 0, "*pcElFetch=%ld, expected 0\n", *pcElFetched);
        *pcElFetched = 1;

        size = (lstrlenW(binding_urls[tested_protocol])+1)*sizeof(WCHAR);
        *ppwzStr = CoTaskMemAlloc(size);
        memcpy(*ppwzStr, binding_urls[tested_protocol], size);
        return S_OK;
    }
    case BINDSTRING_ROOTDOC_URL:
        CHECK_EXPECT(GetBindString_ROOTDOC_URL);
        ok(cEl == 1, "cEl=%ld, expected 1\n", cEl);
        return E_NOTIMPL;
    case BINDSTRING_ENTERPRISE_ID:
        ok(cEl == 1, "cEl=%ld, expected 1\n", cEl);
        return E_NOTIMPL;
    case BINDSTRING_SAMESITE_COOKIE_LEVEL:
        CHECK_EXPECT(GetBindString_SAMESITE_COOKIE_LEVEL);
        ok(cEl == 1, "cEl=%ld, expected 1\n", cEl);
        return E_NOTIMPL;
    default:
        ok(0, "unexpected ulStringType %ld\n", ulStringType);
    }

    return E_NOTIMPL;
}

static IInternetBindInfoVtbl bind_info_vtbl = {
    BindInfo_QueryInterface,
    BindInfo_AddRef,
    BindInfo_Release,
    BindInfo_GetBindInfo,
    BindInfo_GetBindString
};

static IInternetBindInfo bind_info = { &bind_info_vtbl };

static Protocol *impl_from_IInternetPriority(IInternetPriority *iface)
{
    return CONTAINING_RECORD(iface, Protocol, IInternetPriority_iface);
}

static HRESULT WINAPI InternetPriority_QueryInterface(IInternetPriority *iface,
                                                  REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetPriority_AddRef(IInternetPriority *iface)
{
    Protocol *This = impl_from_IInternetPriority(iface);
    if (This->outer)
    {
        This->outer_ref++;
        return IUnknown_AddRef(This->outer);
    }
    return IUnknown_AddRef(&This->IUnknown_inner);
}

static ULONG WINAPI InternetPriority_Release(IInternetPriority *iface)
{
    Protocol *This = impl_from_IInternetPriority(iface);
    if (This->outer)
    {
        This->outer_ref--;
        return IUnknown_Release(This->outer);
    }
    return IUnknown_Release(&This->IUnknown_inner);
}

static HRESULT WINAPI InternetPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    CHECK_EXPECT(SetPriority);
    ok(nPriority == ex_priority, "nPriority=%ld\n", nPriority);
    return S_OK;
}

static HRESULT WINAPI InternetPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}


static const IInternetPriorityVtbl InternetPriorityVtbl = {
    InternetPriority_QueryInterface,
    InternetPriority_AddRef,
    InternetPriority_Release,
    InternetPriority_SetPriority,
    InternetPriority_GetPriority
};

static ULONG WINAPI Protocol_AddRef(IInternetProtocolEx *iface)
{
    return 2;
}

static ULONG WINAPI Protocol_Release(IInternetProtocolEx *iface)
{
    return 1;
}

static HRESULT WINAPI Protocol_Abort(IInternetProtocolEx *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    HRESULT hres;

    CHECK_EXPECT(Abort);

    SET_EXPECT(ReportResult);
    hres = IInternetProtocolSink_ReportResult(binding_sink, S_OK, ERROR_SUCCESS, NULL);
    ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);
    CHECK_CALLED(ReportResult);

    return S_OK;
}

static HRESULT WINAPI Protocol_Suspend(IInternetProtocolEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Resume(IInternetProtocolEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Seek(IInternetProtocolEx *iface,
        LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static Protocol *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, Protocol, IInternetProtocolEx_iface);
}

static HRESULT WINAPI ProtocolEmul_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    Protocol *This = impl_from_IInternetProtocolEx(iface);

    static const IID unknown_iid = {0x7daf9908,0x8415,0x4005,{0x95,0xae, 0xbd,0x27,0xf6,0xe3,0xdc,0x00}};
    static const IID unknown_iid2 = {0x5b7ebc0c,0xf630,0x4cea,{0x89,0xd3,0x5a,0xf0,0x38,0xed,0x05,0x5c}};

    if(IsEqualGUID(riid, &IID_IInternetProtocolEx)) {
        *ppv = &This->IInternetProtocolEx_iface;
        IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
        return S_OK;
    }

    /* FIXME: Why is it calling here instead of outer IUnknown? */
    if(IsEqualGUID(riid, &IID_IInternetPriority)) {
        *ppv = &This->IInternetPriority_iface;
        IInternetPriority_AddRef(&This->IInternetPriority_iface);
        return S_OK;
    }
    if(!IsEqualGUID(riid, &unknown_iid) && !IsEqualGUID(riid, &unknown_iid2)) /* IE10 */
        ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolEmul_AddRef(IInternetProtocolEx *iface)
{
    Protocol *This = impl_from_IInternetProtocolEx(iface);
    if (This->outer)
    {
        This->outer_ref++;
        return IUnknown_AddRef(This->outer);
    }
    return IUnknown_AddRef(&This->IUnknown_inner);
}

static ULONG WINAPI ProtocolEmul_Release(IInternetProtocolEx *iface)
{
    Protocol *This = impl_from_IInternetProtocolEx(iface);
    if (This->outer)
    {
        This->outer_ref--;
        return IUnknown_Release(This->outer);
    }
    return IUnknown_Release(&This->IUnknown_inner);
}

static DWORD WINAPI thread_proc(PVOID arg)
{
    BOOL redirect = redirect_on_continue;
    HRESULT hres;

    memset(&protocoldata, -1, sizeof(protocoldata));

    while(1) {
        prot_state = 0;

        SET_EXPECT(ReportProgress_FINDINGRESOURCE);
        hres = IInternetProtocolSink_ReportProgress(binding_sink,
                BINDSTATUS_FINDINGRESOURCE, hostW);
        CHECK_CALLED(ReportProgress_FINDINGRESOURCE);
        ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);

        SET_EXPECT(ReportProgress_CONNECTING);
        hres = IInternetProtocolSink_ReportProgress(binding_sink,
                BINDSTATUS_CONNECTING, winehq_ipW);
        CHECK_CALLED(ReportProgress_CONNECTING);
        ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);

        SET_EXPECT(ReportProgress_SENDINGREQUEST);
        hres = IInternetProtocolSink_ReportProgress(binding_sink,
                BINDSTATUS_SENDINGREQUEST, NULL);
        CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        ok(hres == S_OK, "ReportProgress failed: %08lx\n", hres);

        prot_state = 1;
        SET_EXPECT(Switch);
        hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
        CHECK_CALLED(Switch);
        ok(hres == S_OK, "Switch failed: %08lx\n", hres);

        if(!redirect)
            break;
        redirect = FALSE;
    }

    if(!short_read) {
        prot_state = 2;
        if(mimefilter_test)
            SET_EXPECT(MimeFilter_Switch);
        else
            SET_EXPECT(Switch);
        hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
        ok(hres == S_OK, "Switch failed: %08lx\n", hres);
        if(mimefilter_test)
            CHECK_CALLED(MimeFilter_Switch);
        else
            CHECK_CALLED(Switch);

        if(test_abort) {
            SetEvent(event_complete);
            return 0;
        }

        prot_state = 2;
        if(mimefilter_test)
            SET_EXPECT(MimeFilter_Switch);
        else
            SET_EXPECT(Switch);
        hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
        ok(hres == S_OK, "Switch failed: %08lx\n", hres);
        if(mimefilter_test)
            CHECK_CALLED(MimeFilter_Switch);
        else
            CHECK_CALLED(Switch);

        prot_state = 3;
        if(mimefilter_test)
            SET_EXPECT(MimeFilter_Switch);
        else
            SET_EXPECT(Switch);
        hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
        ok(hres == S_OK, "Switch failed: %08lx\n", hres);
        if(mimefilter_test)
            CHECK_CALLED(MimeFilter_Switch);
        else
            CHECK_CALLED(Switch);
    }

    SetEvent(event_complete);

    return 0;
}

static void protocol_start(IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo, DWORD pi)
{
    BINDINFO bindinfo, exp_bindinfo;
    DWORD cbindf = 0;
    HRESULT hres;

    ok(pOIProtSink != NULL, "pOIProtSink == NULL\n");
    ok(pOIBindInfo != NULL, "pOIBindInfo == NULL\n");
    ok(pOIProtSink != &protocol_sink, "unexpected pOIProtSink\n");
    ok(pOIBindInfo != &bind_info, "unexpected pOIBindInfo\n");
    ok(!pi, "pi = %lx\n", pi);

    if(binding_test)
        ok(pOIProtSink == binding_sink, "pOIProtSink != binding_sink\n");

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    memcpy(&exp_bindinfo, &bindinfo, sizeof(bindinfo));
    if(test_redirect)
        exp_bindinfo.dwOptions = bindinfo_options;
    SET_EXPECT(GetBindInfo);
    if(redirect_on_continue && (bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS))
        SET_EXPECT(QueryService_IBindCallbackRedirect);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &cbindf, &bindinfo);
    if(redirect_on_continue && (bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS))
        CHECK_CALLED(QueryService_IBindCallbackRedirect);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    CHECK_CALLED(GetBindInfo);
    ok(cbindf == (bindf|BINDF_FROMURLMON), "bindf = %lx, expected %lx\n",
       cbindf, (bindf|BINDF_FROMURLMON));
    ok(!memcmp(&exp_bindinfo, &bindinfo, sizeof(bindinfo)), "unexpected bindinfo\n");
    pReleaseBindInfo(&bindinfo);

    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_SENDINGREQUEST, emptyW);
    ok(hres == S_OK, "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08lx\n", hres);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);

    if(tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST) {
        IServiceProvider *service_provider;
        IHttpNegotiate *http_negotiate;
        IHttpNegotiate2 *http_negotiate2;
        LPWSTR ua = (LPWSTR)0xdeadbeef, accept_mimes[256];
        LPWSTR additional_headers = NULL;
        BYTE sec_id[100];
        DWORD fetched = 0, size = 100;
        DWORD tid;

        SET_EXPECT(GetBindString_USER_AGENT);
        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_USER_AGENT,
                                               &ua, 1, &fetched);
        CHECK_CALLED(GetBindString_USER_AGENT);
        ok(hres == S_OK, "GetBindString(BINDSTRING_USER_AGETNT) failed: %08lx\n", hres);
        ok(fetched == 1, "fetched = %ld, expected 254\n", fetched);
        ok(ua != NULL, "ua =  %p\n", ua);
        ok(!lstrcmpW(ua, user_agentW), "unexpected user agent %s\n", wine_dbgstr_w(ua));
        CoTaskMemFree(ua);

        fetched = 256;
        SET_EXPECT(GetBindString_ACCEPT_MIMES);
        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                               accept_mimes, 256, &fetched);
        CHECK_CALLED(GetBindString_ACCEPT_MIMES);

        ok(hres == S_OK,
           "GetBindString(BINDSTRING_ACCEPT_MIMES) failed: %08lx\n", hres);
        ok(fetched == 1, "fetched = %ld, expected 1\n", fetched);
        ok(!lstrcmpW(acc_mimeW, accept_mimes[0]), "unexpected mimes %s\n", wine_dbgstr_w(accept_mimes[0]));
        CoTaskMemFree(accept_mimes[0]);

        hres = IInternetBindInfo_QueryInterface(pOIBindInfo, &IID_IServiceProvider,
                                                (void**)&service_provider);
        ok(hres == S_OK, "QueryInterface failed: %08lx\n", hres);

        SET_EXPECT(QueryService_HttpNegotiate);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
                &IID_IHttpNegotiate, (void**)&http_negotiate);
        CHECK_CALLED(QueryService_HttpNegotiate);
        ok(hres == S_OK, "QueryService failed: %08lx\n", hres);

        SET_EXPECT(BeginningTransaction);
        hres = IHttpNegotiate_BeginningTransaction(http_negotiate, binding_urls[tested_protocol],
                                                   NULL, 0, &additional_headers);
        CHECK_CALLED(BeginningTransaction);
        IHttpNegotiate_Release(http_negotiate);
        ok(hres == S_OK, "BeginningTransction failed: %08lx\n", hres);
        ok(additional_headers == NULL, "additional_headers=%p\n", additional_headers);

        SET_EXPECT(QueryService_HttpNegotiate);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate2,
                &IID_IHttpNegotiate2, (void**)&http_negotiate2);
        CHECK_CALLED(QueryService_HttpNegotiate);
        ok(hres == S_OK, "QueryService failed: %08lx\n", hres);

        size = 512;
        SET_EXPECT(GetRootSecurityId);
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, sec_id, &size, 0);
        CHECK_CALLED(GetRootSecurityId);
        IHttpNegotiate2_Release(http_negotiate2);
        ok(hres == E_FAIL, "GetRootSecurityId failed: %08lx, expected E_FAIL\n", hres);
        ok(size == 13, "size=%ld\n", size);

        IServiceProvider_Release(service_provider);

        if(!reuse_protocol_thread)
            CreateThread(NULL, 0, thread_proc, NULL, 0, &tid);
        return;
    }

    SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
    hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
            BINDSTATUS_CACHEFILENAMEAVAILABLE, expect_wsz = emptyW);
    ok(hres == S_OK, "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08lx\n", hres);
    CHECK_CALLED(ReportProgress_CACHEFILENAMEAVAILABLE);

    if(mimefilter_test) {
        SET_EXPECT(MimeFilter_CreateInstance);
        SET_EXPECT(MimeFilter_Start);
        SET_EXPECT(ReportProgress_LOADINGMIMEHANDLER);
    }
    SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE,
            mimefilter_test ? pjpegW : (expect_wsz = text_htmlW));
    ok(hres == S_OK,
       "ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE) failed: %08lx\n", hres);
    if(mimefilter_test) {
        CHECK_CALLED(MimeFilter_CreateInstance);
        CHECK_CALLED(MimeFilter_Start);
        CHECK_CALLED(ReportProgress_LOADINGMIMEHANDLER);
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
    }else {
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
    }

    if(mimefilter_test)
        SET_EXPECT(MimeFilter_ReportData);
    else
        SET_EXPECT(ReportData);
    hres = IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION | (tested_protocol == ITS_TEST ? BSCF_DATAFULLYAVAILABLE : BSCF_LASTDATANOTIFICATION),
            13, 13);
    ok(hres == S_OK, "ReportData failed: %08lx\n", hres);
    if(mimefilter_test)
        CHECK_CALLED(MimeFilter_ReportData);
    else
        CHECK_CALLED(ReportData);

    if(result_from_lock) {
        /* set in ProtocolSink_ReportData */
        CHECK_CALLED(ReportResult);
        return;
    }

    if(tested_protocol == ITS_TEST) {
        SET_EXPECT(ReportData);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_BEGINDOWNLOADDATA, NULL);
        ok(hres == S_OK, "ReportProgress(BINDSTATUS_BEGINDOWNLOADDATA) failed: %08lx\n", hres);
        CHECK_CALLED(ReportData);
    }

    if(tested_protocol == BIND_TEST) {
        hres = IInternetProtocol_Terminate(binding_protocol, 0);
        ok(hres == E_FAIL, "Termiante failed: %08lx\n", hres);
    }

    if(mimefilter_test)
        SET_EXPECT(MimeFilter_ReportResult);
    else
        SET_EXPECT(ReportResult);
    hres = IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);
    ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);
    if(mimefilter_test)
        CHECK_CALLED(MimeFilter_ReportResult);
    else
        CHECK_CALLED(ReportResult);
}

static HRESULT WINAPI ProtocolEmul_Start(IInternetProtocolEx *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    CHECK_EXPECT(Start);

    ok(!dwReserved, "dwReserved = %Ix\n", dwReserved);
    protocol_start(pOIProtSink, pOIBindInfo, grfPI);
    return S_OK;
}

static HRESULT WINAPI ProtocolEmul_Continue(IInternetProtocolEx *iface,
        PROTOCOLDATA *pProtocolData)
{
    DWORD bscf = 0, pr;
    HRESULT hres;

    CHECK_EXPECT(Continue);

    ok(pProtocolData != NULL, "pProtocolData == NULL\n");
    if(!pProtocolData || tested_protocol == BIND_TEST)
        return S_OK;
    if(binding_test) {
        ok(pProtocolData != &protocoldata, "pProtocolData == &protocoldata\n");
        ok(pProtocolData->grfFlags == protocoldata.grfFlags, "grfFlags wrong %lx/%lx\n",
           pProtocolData->grfFlags, protocoldata.grfFlags );
        ok(pProtocolData->dwState == protocoldata.dwState, "dwState wrong %lx/%lx\n",
           pProtocolData->dwState, protocoldata.dwState );
        ok(pProtocolData->pData == protocoldata.pData, "pData wrong %p/%p\n",
           pProtocolData->pData, protocoldata.pData );
        ok(pProtocolData->cbData == protocoldata.cbData, "cbData wrong %lx/%lx\n",
           pProtocolData->cbData, protocoldata.cbData );
    }

    switch(prot_state) {
    case 1: {
        IServiceProvider *service_provider;
        IHttpNegotiate *http_negotiate;
        static const WCHAR header[] = {'?',0};
        static const WCHAR redirect_urlW[] = {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',
                                              '/','t','e','s','t','s','/','h','e','l','l','o','.','h','t','m','l',0};

        if(redirect_on_continue) {
            redirect_on_continue = FALSE;
            reuse_protocol_thread = TRUE;

            if(bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS)
                SET_EXPECT(Redirect);
            SET_EXPECT(ReportProgress_REDIRECTING);
            SET_EXPECT(Terminate);
            SET_EXPECT(Protocol_destructor);
            SET_EXPECT(QueryService_InternetProtocol);
            SET_EXPECT(CreateInstance);
            SET_EXPECT(ReportProgress_PROTOCOLCLASSID);
            SET_EXPECT(SetPriority);
            SET_EXPECT(Start);
            hres = IInternetProtocolSink_ReportResult(binding_sink, INET_E_REDIRECT_FAILED, ERROR_SUCCESS, redirect_urlW);
            ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);
            if(bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS)
                CHECK_CALLED(Redirect);
            CHECK_CALLED(ReportProgress_REDIRECTING);
            CHECK_CALLED(Terminate);
            CHECK_CALLED(Protocol_destructor);
            CHECK_CALLED(QueryService_InternetProtocol);
            CHECK_CALLED(CreateInstance);
            CHECK_CALLED(ReportProgress_PROTOCOLCLASSID);
            todo_wine CHECK_NOT_CALLED(SetPriority);
            CHECK_CALLED(Start);

            return S_OK;
        }

        hres = IInternetProtocolSink_QueryInterface(binding_sink, &IID_IServiceProvider,
                                                    (void**)&service_provider);
        ok(hres == S_OK, "Could not get IServiceProvicder\n");

        SET_EXPECT(QueryService_HttpNegotiate);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
                                             &IID_IHttpNegotiate, (void**)&http_negotiate);
        IServiceProvider_Release(service_provider);
        CHECK_CALLED(QueryService_HttpNegotiate);
        ok(hres == S_OK, "Could not get IHttpNegotiate\n");

        SET_EXPECT(OnResponse);
        hres = IHttpNegotiate_OnResponse(http_negotiate, 200, header, NULL, NULL);
        IHttpNegotiate_Release(http_negotiate);
        CHECK_CALLED(OnResponse);
        IHttpNegotiate_Release(http_negotiate);
        ok(hres == S_OK, "OnResponse failed: %08lx\n", hres);

        if(mimefilter_test) {
            SET_EXPECT(MimeFilter_CreateInstance);
            SET_EXPECT(MimeFilter_Start);
            SET_EXPECT(ReportProgress_LOADINGMIMEHANDLER);
        }else if(!(pi & PI_MIMEVERIFICATION)) {
            SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        }
        hres = IInternetProtocolSink_ReportProgress(binding_sink,
                BINDSTATUS_MIMETYPEAVAILABLE, mimefilter_test ? pjpegW : text_htmlW);
        if(mimefilter_test) {
            CHECK_CALLED(MimeFilter_CreateInstance);
            CHECK_CALLED(MimeFilter_Start);
            CHECK_CALLED(ReportProgress_LOADINGMIMEHANDLER);
        }else if(!(pi & PI_MIMEVERIFICATION)) {
            CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        }
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08lx\n", hres);

        bscf |= BSCF_FIRSTDATANOTIFICATION;
        break;
    }
    case 2:
    case 3:
        bscf = BSCF_INTERMEDIATEDATANOTIFICATION;
        break;
    }

    pr = prot_read;
    if(mimefilter_test)
        SET_EXPECT(MimeFilter_ReportData);
    if((!mimefilter_test || no_mime) && (pi & PI_MIMEVERIFICATION)) {
        if(pr < 200)
            SET_EXPECT(Read); /* checked in ReportData for short_read */
        if(pr == 200) {
            if(!mimefilter_test)
                SET_EXPECT(Read); /* checked in BINDSTATUS_MIMETYPEAVAILABLE or ReportData */
            SET_EXPECT(GetBindInfo);
            SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        }
        if(pr >= 200)
            SET_EXPECT(ReportData);
    }else {
        SET_EXPECT(ReportData);
    }

    hres = IInternetProtocolSink_ReportData(binding_sink, bscf, pr, 400);
    ok(hres == S_OK, "ReportData failed: %08lx\n", hres);

    if(mimefilter_test) {
        SET_EXPECT(MimeFilter_ReportData);
    }else if(pi & PI_MIMEVERIFICATION) {
        if(!short_read && pr < 200)
            CHECK_CALLED(Read);
        if(pr == 200) {
            CLEAR_CALLED(GetBindInfo); /* IE9 */
            CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        }
    }else {
        CHECK_CALLED(ReportData);
    }

    if(prot_state == 3)
        prot_state = 4;

    return S_OK;
}

static HRESULT WINAPI ProtocolEmul_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    CHECK_EXPECT(Terminate);
    ok(!dwOptions, "dwOptions=%ld\n", dwOptions);
    return S_OK;
}

static HRESULT WINAPI ProtocolEmul_Read(IInternetProtocolEx *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    if(read_report_data)
        CHECK_EXPECT2(Read2);

    if(mimefilter_test || short_read) {
        if(!read_report_data)
            CHECK_EXPECT2(Read);
    }else if((pi & PI_MIMEVERIFICATION)) {
        if(!read_report_data)
            CHECK_EXPECT2(Read);

        if(prot_read < 300) {
            ok(pv != expect_pv, "pv == expect_pv\n");
            if(prot_read < 300)
                ok(cb == 2048-prot_read, "cb=%ld\n", cb);
            else
                ok(cb == 700, "cb=%ld\n", cb);
        }else {
            ok(expect_pv <= pv && (BYTE*)pv < (BYTE*)expect_pv + cb, "pv != expect_pv\n");
        }
    }else {
        if(!read_report_data)
            CHECK_EXPECT(Read);

        ok(pv == expect_pv, "pv != expect_pv\n");
        ok(cb == 1000, "cb=%ld\n", cb);
        ok(!*pcbRead, "*pcbRead = %ld\n", *pcbRead);
    }
    ok(pcbRead != NULL, "pcbRead == NULL\n");

    if(prot_state == 3 || (short_read && prot_state != 4)) {
        HRESULT hres;

        prot_state = 4;
        if(short_read) {
            SET_EXPECT(Read2); /* checked in BINDSTATUS_MIMETYPEAVAILABLE */
            SET_EXPECT(GetBindInfo);
            SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        }
        if(mimefilter_test)
            SET_EXPECT(MimeFilter_ReportData);
        else if(direct_read)
            SET_EXPECT(ReportData2);
        read_report_data++;
        hres = IInternetProtocolSink_ReportData(binding_sink,
                BSCF_LASTDATANOTIFICATION|BSCF_INTERMEDIATEDATANOTIFICATION, 0, 0);
        read_report_data--;
        ok(hres == S_OK, "ReportData failed: %08lx\n", hres);
        if(short_read) {
            CLEAR_CALLED(GetBindInfo); /* IE9 */
            CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        }
        if(mimefilter_test)
            CHECK_CALLED(MimeFilter_ReportData);
        else if(direct_read)
            CHECK_CALLED(ReportData2);

        if(mimefilter_test)
            SET_EXPECT(MimeFilter_ReportResult);
        else
            SET_EXPECT(ReportResult);
        hres = IInternetProtocolSink_ReportResult(binding_sink, S_OK, ERROR_SUCCESS, NULL);
        ok(hres == S_OK, "ReportResult failed: %08lx\n", hres);
        if(mimefilter_test)
            CHECK_CALLED(MimeFilter_ReportResult);
        else
            CHECK_CALLED(ReportResult);

        if(cb > 100)
            cb = 100;
        memset(pv, 'x', cb);
        if(cb>6)
            memcpy(pv, "gif87a", 6);
        prot_read += *pcbRead = cb;
        return S_OK;
    }

    if(prot_state == 4) {
        *pcbRead = 0;
        return S_FALSE;
    }

    if((async_read_pending = !async_read_pending)) {
        *pcbRead = 0;
        return tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST ? E_PENDING : S_FALSE;
    }

    if(cb > 100)
        cb = 100;
    memset(pv, 'x', cb);
    if(cb>6)
        memcpy(pv, "gif87a", 6);
    prot_read += *pcbRead = cb;
    return S_OK;
}

static HRESULT WINAPI ProtocolEmul_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    CHECK_EXPECT(LockRequest);
    ok(dwOptions == 0, "dwOptions=%lx\n", dwOptions);
    if(result_from_lock) {
        HRESULT hres;
        hres = IInternetProtocolSink_ReportResult(binding_sink, S_OK, ERROR_SUCCESS, NULL);
        ok(hres == S_OK, "ReportResult failed: %08lx, expected E_FAIL\n", hres);
    }
    return S_OK;
}

static HRESULT WINAPI ProtocolEmul_UnlockRequest(IInternetProtocolEx *iface)
{
    CHECK_EXPECT(UnlockRequest);
    return S_OK;
}

static HRESULT WINAPI ProtocolEmul_StartEx(IInternetProtocolEx *iface, IUri *pUri,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE *dwReserved)
{
    CHECK_EXPECT(StartEx);
    ok(!dwReserved, "dwReserved = %p\n", dwReserved);
    protocol_start(pOIProtSink, pOIBindInfo, grfPI);
    return S_OK;
}

static const IInternetProtocolExVtbl ProtocolVtbl = {
    ProtocolEmul_QueryInterface,
    ProtocolEmul_AddRef,
    ProtocolEmul_Release,
    ProtocolEmul_Start,
    ProtocolEmul_Continue,
    Protocol_Abort,
    ProtocolEmul_Terminate,
    Protocol_Suspend,
    Protocol_Resume,
    ProtocolEmul_Read,
    Protocol_Seek,
    ProtocolEmul_LockRequest,
    ProtocolEmul_UnlockRequest,
    ProtocolEmul_StartEx
};

static Protocol *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, Protocol, IUnknown_inner);
}

static HRESULT WINAPI ProtocolUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    static const IID IID_undocumentedIE10 = {0x7daf9908,0x8415,0x4005,{0x95,0xae,0xbd,0x27,0xf6,0xe3,0xdc,0x00}};
    Protocol *This = impl_from_IUnknown(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        if(winetest_debug > 1) trace("QI(IUnknown)\n");
        *ppv = &This->IUnknown_inner;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        if(winetest_debug > 1) trace("QI(InternetProtocol)\n");
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolEx, riid)) {
        if(winetest_debug > 1) trace("QI(InternetProtocolEx)\n");
        if(!impl_protex) {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        if(winetest_debug > 1) trace("QI(InternetPriority)\n");
        *ppv = &This->IInternetPriority_iface;
    }else if(IsEqualGUID(&IID_IWinInetInfo, riid)) {
        if(winetest_debug > 1) trace("QI(IWinInetInfo)\n");
        CHECK_EXPECT(QueryInterface_IWinInetInfo);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IWinInetHttpInfo, riid)) {
        if(winetest_debug > 1) trace("QI(IWinInetHttpInfo)\n");
        CHECK_EXPECT(QueryInterface_IWinInetHttpInfo);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_undocumentedIE10, riid)) {
        if(winetest_debug > 1) trace("QI(%s)\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }else {
        ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ProtocolUnk_AddRef(IUnknown *iface)
{
    Protocol *This = impl_from_IUnknown(iface);
    return ++This->inner_ref;
}

static ULONG WINAPI ProtocolUnk_Release(IUnknown *iface)
{
    Protocol *This = impl_from_IUnknown(iface);
    LONG ref = --This->inner_ref;
    if(!ref) {
        /* IE9 is broken on redirects. It will cause -1 outer_ref on original protocol handler
         * and 1 on redirected handler. */
        ok(!This->outer_ref
           || broken(test_redirect && (This->outer_ref == -1 || This->outer_ref == 1)),
           "outer_ref = %ld\n", This->outer_ref);
        if(This->outer_ref)
            trace("outer_ref %ld\n", This->outer_ref);
        CHECK_EXPECT(Protocol_destructor);
        free(This);
    }
    return ref;
}

static const IUnknownVtbl ProtocolUnkVtbl = {
    ProtocolUnk_QueryInterface,
    ProtocolUnk_AddRef,
    ProtocolUnk_Release
};

static HRESULT WINAPI MimeProtocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocol, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        *ppv = &mime_protocol_sink;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI MimeProtocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    PROTOCOLFILTERDATA *data;
    LPOLESTR url_str = NULL;
    DWORD fetched = 0;
    BINDINFO bindinfo;
    DWORD cbindf = 0;
    HRESULT hres;

    CHECK_EXPECT(MimeFilter_Start);

    ok(!lstrcmpW(szUrl, pjpegW), "wrong url %s\n", wine_dbgstr_w(szUrl));
    ok(grfPI == (PI_FILTER_MODE|PI_FORCE_ASYNC), "grfPI=%lx, expected PI_FILTER_MODE|PI_FORCE_ASYNC\n", grfPI);
    ok(dwReserved, "dwReserved == 0\n");
    ok(pOIProtSink != NULL, "pOIProtSink == NULL\n");
    ok(pOIBindInfo != NULL, "pOIBindInfo == NULL\n");

    if(binding_test) {
        ok(pOIProtSink != binding_sink, "pOIProtSink == protocol_sink\n");
        ok(pOIBindInfo == prot_bind_info, "pOIBindInfo != bind_info\n");
    }else {
        ok(pOIProtSink == &protocol_sink, "pOIProtSink != protocol_sink\n");
        ok(pOIBindInfo == &bind_info, "pOIBindInfo != bind_info\n");
    }

    data = (void*)dwReserved;
    ok(data->cbSize == sizeof(*data), "data->cbSize = %ld\n", data->cbSize);
    ok(!data->pProtocolSink, "data->pProtocolSink != NULL\n");
    ok(data->pProtocol != NULL, "data->pProtocol == NULL\n");
    ok(!data->pUnk, "data->pUnk != NULL\n");
    ok(!data->dwFilterFlags, "data->dwProtocolFlags = %lx\n", data->dwFilterFlags);
    if(binding_test) {
        IInternetProtocolSink *prot_sink;

        IInternetProtocol_QueryInterface(data->pProtocol, &IID_IInternetProtocolSink, (void**)&prot_sink);
        ok(prot_sink == pOIProtSink, "QI(data->pProtocol, IID_IInternetProtocolSink) != pOIProtSink\n");
        IInternetProtocolSink_Release(prot_sink);

        ok(data->pProtocol != binding_protocol, "data->pProtocol == binding_protocol\n");

        filtered_protocol = data->pProtocol;
        IInternetProtocol_AddRef(filtered_protocol);
    }else {
        IInternetProtocol *prot;

        IInternetProtocol_QueryInterface(data->pProtocol, &IID_IInternetProtocol, (void**)&prot);
        ok(prot == async_protocol, "QI(data->pProtocol, IID_IInternetProtocol) != async_protocol\n");
        IInternetProtocol_Release(prot);

        ok(data->pProtocol != async_protocol, "data->pProtocol == async_protocol\n");
    }

    filtered_sink = pOIProtSink;

    SET_EXPECT(ReportProgress_DECODING);
    hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_DECODING, pjpegW);
    ok(hres == S_OK, "ReportProgress(BINDSTATUS_DECODING) failed: %08lx\n", hres);
    CHECK_CALLED(ReportProgress_DECODING);

    SET_EXPECT(GetBindInfo);
    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &cbindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);
    ok(cbindf == (bindf|BINDF_FROMURLMON), "cbindf = %lx, expected %lx\n", cbindf, bindf);
    CHECK_CALLED(GetBindInfo);

    SET_EXPECT(GetBindString_URL);
    hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_URL, &url_str, 1, &fetched);
    ok(hres == S_OK, "GetBindString(BINDSTRING_URL) failed: %08lx\n", hres);
    ok(fetched == 1, "fetched = %ld\n", fetched);
    ok(!lstrcmpW(url_str, binding_urls[tested_protocol]), "wrong url_str %s\n", wine_dbgstr_w(url_str));
    CoTaskMemFree(url_str);
    CHECK_CALLED(GetBindString_URL);

    return S_OK;
}

static HRESULT WINAPI Protocol_Continue(IInternetProtocolEx *iface,
        PROTOCOLDATA *pProtocolData)
{
    CHECK_EXPECT(MimeFilter_Continue);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeProtocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    HRESULT hres;

    CHECK_EXPECT(MimeFilter_Terminate);

    ok(!dwOptions, "dwOptions = %lx\n", dwOptions);

    SET_EXPECT(Terminate);
    hres = IInternetProtocol_Terminate(filtered_protocol, dwOptions);
    ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
    CHECK_CALLED(Terminate);

    return S_OK;
}

static HRESULT WINAPI MimeProtocol_Read(IInternetProtocolEx *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    BYTE buf[2096];
    DWORD read = 0;
    HRESULT hres;

    CHECK_EXPECT(MimeFilter_Read);

    ok(pv != NULL, "pv == NULL\n");
    ok(cb != 0, "cb == 0\n");
    ok(pcbRead != NULL, "pcbRead == NULL\n");

    if(read_report_data)
        SET_EXPECT(Read2);
    else
        SET_EXPECT(Read);
    hres = IInternetProtocol_Read(filtered_protocol, buf, sizeof(buf), &read);
    ok(hres == S_OK || hres == S_FALSE || hres == E_PENDING, "Read failed: %08lx\n", hres);
    if(read_report_data)
        CHECK_CALLED(Read2);
    else
        CHECK_CALLED(Read);

    if(pcbRead) {
        ok(*pcbRead == 0, "*pcbRead=%ld, expected 0\n", *pcbRead);
        *pcbRead = read;
    }

    memset(pv, 'x', read);
    return hres;
}

static HRESULT WINAPI MimeProtocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    HRESULT hres;

    CHECK_EXPECT(MimeFilter_LockRequest);

    ok(!dwOptions, "dwOptions = %lx\n", dwOptions);

    SET_EXPECT(LockRequest);
    hres = IInternetProtocol_LockRequest(filtered_protocol, dwOptions);
    ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
    CHECK_CALLED(LockRequest);

    return S_OK;
}

static HRESULT WINAPI MimeProtocol_UnlockRequest(IInternetProtocolEx *iface)
{
    HRESULT hres;

    CHECK_EXPECT(MimeFilter_UnlockRequest);

    SET_EXPECT(UnlockRequest);
    hres = IInternetProtocol_UnlockRequest(filtered_protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
    CHECK_CALLED(UnlockRequest);

    return S_OK;
}

static const IInternetProtocolExVtbl MimeProtocolVtbl = {
    MimeProtocol_QueryInterface,
    Protocol_AddRef,
    Protocol_Release,
    MimeProtocol_Start,
    Protocol_Continue,
    Protocol_Abort,
    MimeProtocol_Terminate,
    Protocol_Suspend,
    Protocol_Resume,
    MimeProtocol_Read,
    Protocol_Seek,
    MimeProtocol_LockRequest,
    MimeProtocol_UnlockRequest
};

static IInternetProtocolEx MimeProtocol = { &MimeProtocolVtbl };

static HRESULT WINAPI InternetProtocolInfo_QueryInterface(IInternetProtocolInfo *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetProtocolInfo_AddRef(IInternetProtocolInfo *iface)
{
    return 2;
}

static ULONG WINAPI InternetProtocolInfo_Release(IInternetProtocolInfo *iface)
{
    return 1;
}

static HRESULT WINAPI InternetProtocolInfo_ParseUrl(IInternetProtocolInfo *iface, LPCWSTR pwzUrl,
        PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult,
        DWORD *pcchResult, DWORD dwReserved)
{
    ok(0, "unexpected call %d\n", ParseAction);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_CombineUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,
        LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_CompareUrl(IInternetProtocolInfo *iface,
        LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolInfo_QueryInfo(IInternetProtocolInfo *iface,
        LPCWSTR pwzUrl, QUERYOPTION OueryOption, DWORD dwQueryFlags, LPVOID pBuffer,
        DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IInternetProtocolInfoVtbl InternetProtocolInfoVtbl = {
    InternetProtocolInfo_QueryInterface,
    InternetProtocolInfo_AddRef,
    InternetProtocolInfo_Release,
    InternetProtocolInfo_ParseUrl,
    InternetProtocolInfo_CombineUrl,
    InternetProtocolInfo_CompareUrl,
    InternetProtocolInfo_QueryInfo
};

static IInternetProtocolInfo protocol_info = { &InternetProtocolInfoVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IInternetProtocolInfo, riid)) {
        *ppv = &protocol_info;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                        REFIID riid, void **ppv)
{
    Protocol *ret;

    ok(ppv != NULL, "ppv == NULL\n");

    if(!pOuter) {
        CHECK_EXPECT(CreateInstance_no_aggregation);
        ok(IsEqualGUID(&IID_IInternetProtocol, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    }else {
        CHECK_EXPECT(CreateInstance);
        ok(pOuter == (IUnknown*)prot_bind_info, "pOuter != protocol_unk\n");
        ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        if (no_aggregation) {
            *ppv = NULL;
            return CLASS_E_NOAGGREGATION;
        }
    }

    ret = malloc(sizeof(*ret));
    ret->IUnknown_inner.lpVtbl = &ProtocolUnkVtbl;
    ret->IInternetProtocolEx_iface.lpVtbl = &ProtocolVtbl;
    ret->IInternetPriority_iface.lpVtbl = &InternetPriorityVtbl;
    ret->outer = pOuter;
    ret->inner_ref = 1;
    ret->outer_ref = 0;

    protocol_emul = ret;
    if (!pOuter)
        *ppv = &ret->IInternetProtocolEx_iface;
    else
        *ppv = &ret->IUnknown_inner;
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

static IClassFactory ClassFactory = { &ClassFactoryVtbl };

static HRESULT WINAPI MimeFilter_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CHECK_EXPECT(MimeFilter_CreateInstance);

    ok(!outer, "outer = %p\n", outer);
    ok(IsEqualGUID(&IID_IInternetProtocol, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));

    *ppv = &MimeProtocol;
    return S_OK;
}

static const IClassFactoryVtbl MimeFilterCFVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    MimeFilter_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory mimefilter_cf = { &MimeFilterCFVtbl };

#define TEST_BINDING     0x0001
#define TEST_FILTER      0x0002
#define TEST_FIRST_HTTP  0x0004
#define TEST_DIRECT_READ 0x0008
#define TEST_POST        0x0010
#define TEST_EMULATEPROT 0x0020
#define TEST_SHORT_READ  0x0040
#define TEST_REDIRECT    0x0080
#define TEST_ABORT       0x0100
#define TEST_ASYNCREQ    0x0200
#define TEST_USEIURI     0x0400
#define TEST_IMPLPROTEX  0x0800
#define TEST_EMPTY       0x1000
#define TEST_NOMIME      0x2000
#define TEST_FROMCACHE   0x4000
#define TEST_DISABLEAUTOREDIRECT  0x8000
#define TEST_RESULTFROMLOCK      0x10000
#define TEST_USEBINDING  0x20000

static void register_filter(BOOL do_register)
{
    IInternetSession *session;
    HRESULT hres;

    hres = pCoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);

    if(do_register) {
        hres = IInternetSession_RegisterMimeFilter(session, &mimefilter_cf, &IID_IInternetProtocol, pjpegW);
        ok(hres == S_OK, "RegisterMimeFilter failed: %08lx\n", hres);
        hres = IInternetSession_RegisterMimeFilter(session, &mimefilter_cf, &IID_IInternetProtocol, gifW);
        ok(hres == S_OK, "RegisterMimeFilter failed: %08lx\n", hres);
    }else {
        hres = IInternetSession_UnregisterMimeFilter(session, &mimefilter_cf, pjpegW);
        ok(hres == S_OK, "RegisterMimeFilter failed: %08lx\n", hres);
        hres = IInternetSession_UnregisterMimeFilter(session, &mimefilter_cf, gifW);
        ok(hres == S_OK, "RegisterMimeFilter failed: %08lx\n", hres);
    }

    IInternetSession_Release(session);
}

static void init_test(int prot, DWORD flags)
{
    tested_protocol = prot;
    binding_test = (flags & TEST_BINDING) != 0;
    first_data_notif = TRUE;
    prot_read = 0;
    prot_state = 0;
    async_read_pending = TRUE;
    mimefilter_test = (flags & TEST_FILTER) != 0;
    no_mime = (flags & TEST_NOMIME) != 0;
    filter_state = 0;
    post_stream_read = 0;
    ResetEvent(event_complete);
    ResetEvent(event_complete2);
    ResetEvent(event_continue);
    ResetEvent(event_continue_done);
    async_protocol = binding_protocol = filtered_protocol = NULL;
    filtered_sink = NULL;
    http_is_first = (flags & TEST_FIRST_HTTP) != 0;
    first_data_notif = TRUE;
    state = STATE_CONNECTING;
    test_async_req = (flags & TEST_ASYNCREQ) != 0;
    direct_read = (flags & TEST_DIRECT_READ) != 0;
    emulate_prot = (flags & TEST_EMULATEPROT) != 0;
    wait_for_switch = TRUE;
    short_read = (flags & TEST_SHORT_READ) != 0;
    http_post_test = TYMED_NULL;
    redirect_on_continue = test_redirect = (flags & TEST_REDIRECT) != 0;
    test_abort = (flags & TEST_ABORT) != 0;
    impl_protex = (flags & TEST_IMPLPROTEX) != 0;
    empty_file = (flags & TEST_EMPTY) != 0;
    bind_from_cache = (flags & TEST_FROMCACHE) != 0;
    result_from_lock = (flags & TEST_RESULTFROMLOCK) != 0;
    file_with_hash = FALSE;
    security_problem = FALSE;
    reuse_protocol_thread = FALSE;
    memcpy(protocol_clsid, null_guid, sizeof(null_guid));

    bindinfo_options = 0;
    if(flags & TEST_DISABLEAUTOREDIRECT)
        bindinfo_options |= BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS;

    register_filter(mimefilter_test);
}

static void test_priority(IInternetProtocol *protocol)
{
    IInternetPriority *priority;
    LONG pr;
    HRESULT hres;

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority,
                                            (void**)&priority);
    ok(hres == S_OK, "QueryInterface(IID_IInternetPriority) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetPriority_GetPriority(priority, &pr);
    ok(hres == S_OK, "GetPriority failed: %08lx\n", hres);
    ok(pr == 0, "pr=%ld, expected 0\n", pr);

    hres = IInternetPriority_SetPriority(priority, 1);
    ok(hres == S_OK, "SetPriority failed: %08lx\n", hres);

    hres = IInternetPriority_GetPriority(priority, &pr);
    ok(hres == S_OK, "GetPriority failed: %08lx\n", hres);
    ok(pr == 1, "pr=%ld, expected 1\n", pr);

    IInternetPriority_Release(priority);
}

static void test_early_abort(const CLSID *clsid)
{
    IInternetProtocol *protocol;
    HRESULT hres;

    hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);

    hres = IInternetProtocol_Abort(protocol, E_ABORT, 0);
    ok(hres == S_OK, "Abort failed: %08lx\n", hres);

    hres = IInternetProtocol_Abort(protocol, E_FAIL, 0);
    ok(hres == S_OK, "Abort failed: %08lx\n", hres);

    IInternetProtocol_Release(protocol);
}

static BOOL file_protocol_start(IInternetProtocol *protocol, LPCWSTR url,
        IInternetProtocolEx *protocolex, IUri *uri, BOOL is_first)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
       SET_EXPECT(ReportProgress_DIRECTBIND);
    if(is_first) {
        SET_EXPECT(ReportProgress_SENDINGREQUEST);
        SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        if(bindf & BINDF_FROMURLMON)
            SET_EXPECT(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
        else
            SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    }
    SET_EXPECT(ReportData);
    if(is_first)
        SET_EXPECT(ReportResult);

    expect_hrResult = S_OK;

    if(protocolex) {
        hres = IInternetProtocolEx_StartEx(protocolex, uri, &protocol_sink, &bind_info, 0, 0);
        ok(hres == S_OK, "StartEx failed: %08lx\n", hres);
    }else {
        hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
        if(hres == INET_E_RESOURCE_NOT_FOUND) {
            win_skip("Start failed\n");
            return FALSE;
        }
        ok(hres == S_OK, "Start failed: %08lx\n", hres);
    }

    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        CLEAR_CALLED(ReportProgress_DIRECTBIND); /* Not called by IE10 */
    if(is_first) {
        CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        CHECK_CALLED(ReportProgress_CACHEFILENAMEAVAILABLE);
        if(bindf & BINDF_FROMURLMON)
            CHECK_CALLED(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
        else
            CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
    }
    CHECK_CALLED(ReportData);
    if(is_first)
        CHECK_CALLED(ReportResult);

    return TRUE;
}

static void test_file_protocol_url(LPCWSTR url)
{
    IInternetProtocolInfo *protocol_info;
    IUnknown *unk;
    IClassFactory *factory;
    IInternetProtocol *protocol;
    BYTE buf[512];
    ULONG cb;
    HRESULT hres;

    hres = CoGetClassObject(&CLSID_FileProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
            "Could not get IInternetProtocolInfo interface: %08lx, expected E_NOINTERFACE\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        if(file_protocol_start(protocol, url, NULL, NULL, TRUE)) {
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == 2, "cb=%lu expected 2\n", cb);
            buf[2] = 0;
            ok(!memcmp(buf, file_with_hash ? "XX" : "<H", 2), "Unexpected data %s\n", buf);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_FALSE, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_FALSE, "Read failed: %08lx expected S_FALSE\n", hres);
            ok(cb == 0, "cb=%lu expected 0\n", cb);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
        }

        if(file_protocol_start(protocol, url, NULL, NULL, FALSE)) {
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_FALSE, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
        }

        IInternetProtocol_Release(protocol);
    }

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        if(file_protocol_start(protocol, url, NULL, NULL, TRUE)) {
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n\n", hres);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            todo_wine_if(file_with_hash) /* FIXME: An effect of UnlockRequest call? */
                ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
        }

        IInternetProtocol_Release(protocol);
    }

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        if(file_protocol_start(protocol, url, NULL, NULL, TRUE)) {
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == 2, "cb=%lu expected 2\n", cb);
        }

        IInternetProtocol_Release(protocol);
    }

    if(pCreateUri) {
        IInternetProtocolEx *protocolex;
        IUri *uri;

        hres = pCreateUri(url, Uri_CREATE_FILE_USE_DOS_PATH, 0, &uri);
        ok(hres == S_OK, "CreateUri failed: %08lx\n", hres);

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocolEx, (void**)&protocolex);
        ok(hres == S_OK, "Could not get IInternetProtocolEx: %08lx\n", hres);

        if(file_protocol_start(NULL, NULL, protocolex, uri, TRUE)) {
            hres = IInternetProtocolEx_Read(protocolex, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocolEx_LockRequest(protocolex, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocolEx_UnlockRequest(protocolex);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
        }

        IUri_Release(uri);
        IInternetProtocolEx_Release(protocolex);

        hres = pCreateUri(url, 0, 0, &uri);
        ok(hres == S_OK, "CreateUri failed: %08lx\n", hres);

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocolEx, (void**)&protocolex);
        ok(hres == S_OK, "Could not get IInternetProtocolEx: %08lx\n", hres);

        if(file_protocol_start(NULL, NULL, protocolex, uri, TRUE)) {
            hres = IInternetProtocolEx_Read(protocolex, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocolEx_LockRequest(protocolex, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocolEx_UnlockRequest(protocolex);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
        }

        IUri_Release(uri);
        IInternetProtocolEx_Release(protocolex);
    }else {
        win_skip("Skipping file protocol StartEx tests\n");
    }

    IClassFactory_Release(factory);
}

static void test_file_protocol_fail(void)
{
    IInternetProtocol *protocol;
    HRESULT hres;

    static const WCHAR index_url2[] =
        {'f','i','l','e',':','/','/','i','n','d','e','x','.','h','t','m','l',0};

    hres = CoCreateInstance(&CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(GetBindInfo);
    expect_hrResult = MK_E_SYNTAX;
    hres = IInternetProtocol_Start(protocol, wszIndexHtml, &protocol_sink, &bind_info, 0, 0);
    ok(hres == MK_E_SYNTAX ||
       hres == E_INVALIDARG,
       "Start failed: %08lx, expected MK_E_SYNTAX or E_INVALIDARG\n", hres);
    CLEAR_CALLED(GetBindInfo); /* GetBindInfo not called in IE7 */

    SET_EXPECT(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportResult);
    expect_hrResult = INET_E_RESOURCE_NOT_FOUND;
    hres = IInternetProtocol_Start(protocol, index_url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == INET_E_RESOURCE_NOT_FOUND,
            "Start failed: %08lx expected INET_E_RESOURCE_NOT_FOUND\n", hres);
    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        CHECK_CALLED(ReportProgress_DIRECTBIND);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    CHECK_CALLED(ReportResult);

    IInternetProtocol_Release(protocol);

    hres = CoCreateInstance(&CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportResult);
    expect_hrResult = INET_E_RESOURCE_NOT_FOUND;

    hres = IInternetProtocol_Start(protocol, index_url2, &protocol_sink, &bind_info, 0, 0);
    ok(hres == INET_E_RESOURCE_NOT_FOUND,
            "Start failed: %08lx, expected INET_E_RESOURCE_NOT_FOUND\n", hres);
    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        CHECK_CALLED(ReportProgress_DIRECTBIND);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    CHECK_CALLED(ReportResult);

    SET_EXPECT(GetBindInfo);
    hres = IInternetProtocol_Start(protocol, NULL, &protocol_sink, &bind_info, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08lx, expected E_INVALIDARG\n", hres);
    CLEAR_CALLED(GetBindInfo); /* GetBindInfo not called in IE7 */

    SET_EXPECT(GetBindInfo);
    hres = IInternetProtocol_Start(protocol, emptyW, &protocol_sink, &bind_info, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08lx, expected E_INVALIDARG\n", hres);
    CLEAR_CALLED(GetBindInfo); /* GetBindInfo not called in IE7 */

    IInternetProtocol_Release(protocol);
}

static void test_file_protocol(void) {
    WCHAR buf[INTERNET_MAX_URL_LENGTH], file_name_buf[MAX_PATH];
    DWORD size;
    ULONG len;
    HANDLE file;

    static const WCHAR wszFile[] = {'f','i','l','e',':',0};
    static const WCHAR wszFile2[] = {'f','i','l','e',':','/','/',0};
    static const WCHAR wszFile3[] = {'f','i','l','e',':','/','/','/',0};
    static const WCHAR wszFile4[] = {'f','i','l','e',':','\\','\\',0};
    static const char html_doc[] = "<HTML></HTML>";
    static const WCHAR fragmentW[] = {'#','f','r','a','g',0};

    trace("Testing file protocol...\n");
    init_test(FILE_TEST, 0);

    SetLastError(0xdeadbeef);
    file = CreateFileW(wszIndexHtml, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return;
    WriteFile(file, html_doc, sizeof(html_doc)-1, &size, NULL);
    CloseHandle(file);

    file_name = wszIndexHtml;
    bindf = 0;
    test_file_protocol_url(index_url);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(index_url);
    bindf = BINDF_FROMURLMON | BINDF_NEEDFILE;
    test_file_protocol_url(index_url);

    memcpy(buf, wszFile, sizeof(wszFile));
    len = ARRAY_SIZE(wszFile)-1;
    len += GetCurrentDirectoryW(ARRAY_SIZE(buf)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + ARRAY_SIZE(wszFile)-1;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    memcpy(buf, wszFile2, sizeof(wszFile2));
    len = GetCurrentDirectoryW(ARRAY_SIZE(file_name_buf), file_name_buf);
    file_name_buf[len++] = '\\';
    memcpy(file_name_buf+len, wszIndexHtml, sizeof(wszIndexHtml));
    lstrcpyW(buf+ARRAY_SIZE(wszFile2)-1, file_name_buf);
    file_name = file_name_buf;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    buf[ARRAY_SIZE(wszFile2)] = '|';
    test_file_protocol_url(buf);

    memcpy(buf, wszFile3, sizeof(wszFile3));
    len = ARRAY_SIZE(wszFile3)-1;
    len += GetCurrentDirectoryW(ARRAY_SIZE(buf)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + ARRAY_SIZE(wszFile3)-1;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    memcpy(buf, wszFile4, sizeof(wszFile4));
    len = GetCurrentDirectoryW(ARRAY_SIZE(file_name_buf), file_name_buf);
    file_name_buf[len++] = '\\';
    memcpy(file_name_buf+len, wszIndexHtml, sizeof(wszIndexHtml));
    lstrcpyW(buf+ARRAY_SIZE(wszFile4)-1, file_name_buf);
    file_name = file_name_buf;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    buf[ARRAY_SIZE(wszFile4)] = '|';
    test_file_protocol_url(buf);

    /* Fragment part of URL is skipped if the file doesn't exist. */
    lstrcatW(buf, fragmentW);
    test_file_protocol_url(buf);

    /* Fragment part is considered a part of the file name, if the file exists. */
    len = lstrlenW(file_name_buf);
    lstrcpyW(file_name_buf+len, fragmentW);
    file = CreateFileW(wszIndexHtml, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    WriteFile(file, "XXX", 3, &size, NULL);
    CloseHandle(file);
    file_name_buf[len] = 0;

    file_with_hash = TRUE;
    test_file_protocol_url(buf);

    DeleteFileW(wszIndexHtml);
    DeleteFileW(file_name_buf);

    bindf = 0;
    test_file_protocol_fail();
    bindf = BINDF_FROMURLMON;
    test_file_protocol_fail();
}

static void create_cache_entry(const WCHAR *urlw)
{
    FILETIME now, tomorrow, yesterday;
    char file_path[MAX_PATH];
    BYTE content[1000];
    ULARGE_INTEGER li;
    const char *url;
    HANDLE file;
    DWORD size;
    unsigned i;
    BOOL res;

    BYTE cache_headers[] = "HTTP/1.1 200 OK\r\n\r\n";

    trace("Testing cache read...\n");

    url = w2a(urlw);

    for(i = 0; i < sizeof(content); i++)
        content[i] = '0' + (i%10);

    GetSystemTimeAsFileTime(&now);
    li.u.HighPart = now.dwHighDateTime;
    li.u.LowPart = now.dwLowDateTime;
    li.QuadPart += (LONGLONG)10000000 * 3600 * 24;
    tomorrow.dwHighDateTime = li.u.HighPart;
    tomorrow.dwLowDateTime = li.u.LowPart;
    li.QuadPart -= (LONGLONG)10000000 * 3600 * 24 * 2;
    yesterday.dwHighDateTime = li.u.HighPart;
    yesterday.dwLowDateTime = li.u.LowPart;

    res = CreateUrlCacheEntryA(url, sizeof(content), "", file_path, 0);
    ok(res, "CreateUrlCacheEntryA failed: %lu\n", GetLastError());

    file = CreateFileA(file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");

    WriteFile(file, content, sizeof(content), &size, NULL);
    CloseHandle(file);

    res = CommitUrlCacheEntryA(url, file_path, tomorrow, yesterday, NORMAL_CACHE_ENTRY,
                               cache_headers, sizeof(cache_headers)-1, "", 0);
    ok(res, "CommitUrlCacheEntryA failed: %lu\n", GetLastError());
}

static BOOL http_protocol_start(LPCWSTR url, BOOL use_iuri)
{
    static BOOL got_user_agent = FALSE;
    IUri *uri = NULL;
    HRESULT hres;

    if(use_iuri && pCreateUri) {
        hres = pCreateUri(url, 0, 0, &uri);
        ok(hres == S_OK, "CreateUri failed: %08lx\n", hres);
    }

    SET_EXPECT(GetBindInfo);
    if (!(bindf & BINDF_FROMURLMON))
        SET_EXPECT(ReportProgress_DIRECTBIND);
    if(!got_user_agent)
        SET_EXPECT(GetBindString_USER_AGENT);
    SET_EXPECT(GetBindString_ROOTDOC_URL);
    SET_EXPECT(GetBindString_ACCEPT_MIMES);
    SET_EXPECT(QueryService_HttpNegotiate);
    SET_EXPECT(BeginningTransaction);
    SET_EXPECT(GetRootSecurityId);
    if(http_post_test) {
        SET_EXPECT(GetBindString_POST_COOKIE);
        if(http_post_test == TYMED_ISTREAM)
            SET_EXPECT(Stream_Seek);
    }
    if(bind_from_cache) {
        SET_EXPECT(OnResponse);
        SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(ReportData);
    }

    if(uri) {
        IInternetProtocolEx *protocolex;

        hres = IInternetProtocol_QueryInterface(async_protocol, &IID_IInternetProtocolEx, (void**)&protocolex);
        ok(hres == S_OK, "Could not get IInternetProtocolEx iface: %08lx\n", hres);

        hres = IInternetProtocolEx_StartEx(protocolex, uri, &protocol_sink, &bind_info, 0, 0);
        ok(hres == S_OK, "Start failed: %08lx\n", hres);

        IInternetProtocolEx_Release(protocolex);
        IUri_Release(uri);
    }else {
        hres = IInternetProtocol_Start(async_protocol, url, &protocol_sink, &bind_info, 0, 0);
        ok(hres == S_OK, "Start failed: %08lx\n", hres);
    }
    if(FAILED(hres))
        return FALSE;

    CHECK_CALLED(GetBindInfo);
    if (!(bindf & BINDF_FROMURLMON))
        CHECK_CALLED(ReportProgress_DIRECTBIND);
    if (!got_user_agent)
    {
        CHECK_CALLED(GetBindString_USER_AGENT);
        got_user_agent = TRUE;
    }
    CLEAR_CALLED(GetBindString_ROOTDOC_URL); /* New in IE11 */
    CHECK_CALLED(GetBindString_ACCEPT_MIMES);
    CHECK_CALLED(QueryService_HttpNegotiate);
    CHECK_CALLED(BeginningTransaction);
    /* GetRootSecurityId called on WinXP but not on Win98 */
    CLEAR_CALLED(GetRootSecurityId);
    if(http_post_test) {
        CHECK_CALLED(GetBindString_POST_COOKIE);
        if(http_post_test == TYMED_ISTREAM)
            CHECK_CALLED(Stream_Seek);
    }
    if(bind_from_cache) {
        CHECK_CALLED(OnResponse);
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(ReportData);
    }

    return TRUE;
}

static void test_protocol_terminate(IInternetProtocol *protocol)
{
    BYTE buf[3600];
    DWORD cb;
    HRESULT hres;

    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);

    hres = IInternetProtocol_Read(protocol, buf, 1, &cb);
    ok(hres == S_FALSE || (test_abort && hres == S_OK), /* result can come in before the abort */
       "Read failed: %08lx\n", hres);

    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08lx\n", hres);

    /* This wait is to give the internet handles being freed in Terminate
     * enough time to actually terminate in all cases. Internet handles
     * terminate asynchronously and native reuses the main InternetOpen
     * handle. The only case in which this seems to be necessary is on
     * wine with native wininet and urlmon, resulting in the next time
     * test_http_protocol_url being called the first data notification actually
     * being an extra last data notification from the previous connection
     * about once out of every ten times. */
    Sleep(100);

    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
}

/* is_first refers to whether this is the first call to this function
 * _for this url_ */
static void test_http_protocol_url(LPCWSTR url, int prot, DWORD flags, DWORD tymed)
{
    IInternetProtocolInfo *protocol_info;
    IClassFactory *factory;
    IInternetSession *session;
    IUnknown *unk;
    HRESULT hres;

    init_test(prot, flags);
    http_url = url;
    http_post_test = tymed;
    if(flags & TEST_FROMCACHE)
        create_cache_entry(url);

    hres = CoGetClassObject(prot == HTTPS_TEST ? &CLSID_HttpSProtocol : &CLSID_HttpProtocol,
            CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
        "Could not get IInternetProtocolInfo interface: %08lx, expected E_NOINTERFACE\n",
        hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    if (flags & TEST_USEBINDING) {
        hres = pCoInternetGetSession(0, &session, 0);
        ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);
        hres = IInternetSession_CreateBinding(session, NULL, url, NULL, NULL, &async_protocol, 0);
    } else
        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                            (void**)&async_protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        BYTE buf[3600];
        DWORD cb;
        ULONG ref;

        test_priority(async_protocol);

        if (flags & TEST_USEBINDING) {
            SET_EXPECT(QueryService_InternetProtocol);
            SET_EXPECT(ReportProgress_PROTOCOLCLASSID);
            StringFromGUID2(&CLSID_HttpProtocol, protocol_clsid, ARRAY_SIZE(protocol_clsid));
            if (flags & TEST_DISABLEAUTOREDIRECT)
                SET_EXPECT(QueryService_IBindCallbackRedirect);
        }

        SET_EXPECT(ReportProgress_COOKIE_SENT);
        if(http_is_first) {
            SET_EXPECT(ReportProgress_FINDINGRESOURCE);
            SET_EXPECT(ReportProgress_CONNECTING);
        }
        SET_EXPECT(ReportProgress_SENDINGREQUEST);
        if(test_redirect && !(bindinfo_options & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS)) {
            SET_EXPECT(ReportProgress_REDIRECTING);
            SET_EXPECT(GetBindString_SAMESITE_COOKIE_LEVEL); /* New in IE11 */
        }
        SET_EXPECT(ReportProgress_PROXYDETECTING);
        if(prot == HTTP_TEST)
            SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        else
            SET_EXPECT(QueryService_HttpSecurity);
        if(!(bindf & BINDF_FROMURLMON)) {
            SET_EXPECT(OnResponse);
            SET_EXPECT(ReportProgress_RAWMIMETYPE);
            SET_EXPECT(ReportData);
        } else {
            SET_EXPECT(Switch);
        }

        if(!http_protocol_start(url, (flags & TEST_USEIURI) != 0)) {
            IInternetProtocol_Abort(async_protocol, E_ABORT, 0);
            IInternetProtocol_Release(async_protocol);
            return;
        }

        if (flags & TEST_USEBINDING) {
            todo_wine CHECK_NOT_CALLED(QueryService_InternetProtocol);
            todo_wine CHECK_NOT_CALLED(ReportProgress_PROTOCOLCLASSID);
            if (flags & TEST_DISABLEAUTOREDIRECT)
                CHECK_CALLED(QueryService_IBindCallbackRedirect);
        }

        if(!direct_read && !bind_from_cache)
            SET_EXPECT(ReportResult);

        if(flags & TEST_DISABLEAUTOREDIRECT)
            expect_hrResult = INET_E_REDIRECT_FAILED;
        else if(test_abort)
            expect_hrResult = E_ABORT;
        else
            expect_hrResult = S_OK;

        if(direct_read) {
            SET_EXPECT(Switch);
            while(wait_for_switch) {
                ok( WaitForSingleObject(event_continue, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
                CHECK_CALLED(Switch); /* Set in ReportData */
                call_continue(&continue_protdata);
                SetEvent(event_continue_done);
            }
        }else if(bind_from_cache) {
            BYTE buf[1500];

            hres = IInternetProtocol_Read(async_protocol, buf, 100, &cb);
            ok(hres == S_OK && cb == 100, "Read failed: %08lx (%ld bytes)\n", hres, cb);

            SET_EXPECT(ReportResult);
            hres = IInternetProtocol_Read(async_protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK && cb == 900, "Read failed: %08lx (%ld bytes)\n", hres, cb);
            CHECK_CALLED(ReportResult);

            hres = IInternetProtocol_Read(async_protocol, buf, sizeof(buf), &cb);
            ok(hres == S_FALSE && !cb, "Read failed: %08lx (%ld bytes)\n", hres, cb);
        }else {
            hres = IInternetProtocol_Read(async_protocol, buf, 1, &cb);
            ok((hres == E_PENDING && cb==0) ||
               (hres == S_OK && cb==1), "Read failed: %08lx (%ld bytes)\n", hres, cb);

            ok( WaitForSingleObject(event_complete, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
            if(bindf & BINDF_FROMURLMON)
                CHECK_CALLED(Switch);
            else
                CHECK_CALLED(ReportData);
            if(prot == HTTPS_TEST)
                CLEAR_CALLED(QueryService_HttpSecurity);

            while(1) {
                if(bindf & BINDF_FROMURLMON)
                    SET_EXPECT(Switch);
                else
                    SET_EXPECT(ReportData);
                hres = IInternetProtocol_Read(async_protocol, buf, sizeof(buf), &cb);
                if(hres == E_PENDING) {
                    hres = IInternetProtocol_Read(async_protocol, buf, 1, &cb);
                    ok((hres == E_PENDING && cb==0) ||
                       (hres == S_OK && cb==1), "Read failed: %08lx (%ld bytes)\n", hres, cb);
                    ok( WaitForSingleObject(event_complete, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
                    if(bindf & BINDF_FROMURLMON)
                        CHECK_CALLED(Switch);
                    else
                        CHECK_CALLED(ReportData);

                    if(test_abort) {
                        HRESULT hres;

                        SET_EXPECT(ReportResult);
                        hres = IInternetProtocol_Abort(async_protocol, E_ABORT, 0);
                        ok(hres == S_OK, "Abort failed: %08lx\n", hres);
                        CHECK_CALLED(ReportResult);

                        hres = IInternetProtocol_Abort(async_protocol, E_ABORT, 0);
                        ok(hres == INET_E_RESULT_DISPATCHED || hres == S_OK /* IE10 */, "Abort failed: %08lx\n", hres);
                        break;
                    }
                }else {
                    if(bindf & BINDF_FROMURLMON)
                        CHECK_NOT_CALLED(Switch);
                    else
                        CHECK_NOT_CALLED(ReportData);
                    if(cb == 0) break;
                }
            }
            if(!test_abort) {
                ok(hres == S_FALSE, "Read failed: %08lx\n", hres);
                CHECK_CALLED(ReportResult);
            }
        }
        if(prot == HTTPS_TEST)
            CLEAR_CALLED(ReportProgress_SENDINGREQUEST);

        if (prot == HTTP_TEST || prot == HTTPS_TEST)
            CLEAR_CALLED(ReportProgress_COOKIE_SENT);

        hres = IInternetProtocol_Abort(async_protocol, E_ABORT, 0);
        ok(hres == INET_E_RESULT_DISPATCHED || hres == S_OK /* IE10 */, "Abort failed: %08lx\n", hres);

        test_protocol_terminate(async_protocol);

        hres = IInternetProtocol_Abort(async_protocol, E_ABORT, 0);
        ok(hres == S_OK, "Abort failed: %08lx\n", hres);

        ref = IInternetProtocol_Release(async_protocol);
        ok(!ref, "ref=%lx\n", ref);
    }

    IClassFactory_Release(factory);

    if(flags & TEST_FROMCACHE) {
        BOOL res;

        res = DeleteUrlCacheEntryW(url);
        ok(res, "DeleteUrlCacheEntryA failed: %lu\n", GetLastError());
    }
}

static void test_http_protocol(void)
{
    static const WCHAR posttest_url[] =
        {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/',
         't','e','s','t','s','/','p','o','s','t','.','p','h','p',0};
    static const WCHAR redirect_url[] =
        {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/',
         't','e','s','t','s','/','r','e','d','i','r','e','c','t',0};
    static const WCHAR winetest_url[] =
        {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/',
         't','e','s','t','s','/','d','a','t','a','.','p','h','p',0};
    static const WCHAR empty_url[] =
        {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/',
         't','e','s','t','s','/','e','m','p','t','y','.','j','s',0};
    static const WCHAR cache_only_url[] =
        {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/',
         't','e','s','t','s','/','c','a','c','h','e','-','o','n','l','y',0};


    trace("Testing http protocol (not from urlmon)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    test_http_protocol_url(winetest_url, HTTP_TEST, TEST_FIRST_HTTP, TYMED_NULL);

    trace("Testing http protocol (from urlmon)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON;
    test_http_protocol_url(winetest_url, HTTP_TEST, 0, TYMED_NULL);

    trace("Testing http protocol (to file)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NEEDFILE;
    test_http_protocol_url(winetest_url, HTTP_TEST, 0, TYMED_NULL);

    trace("Testing http protocol (post data)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON;
    test_http_protocol_url(posttest_url, HTTP_TEST, TEST_FIRST_HTTP|TEST_POST, TYMED_HGLOBAL);

    trace("Testing http protocol (post data stream)...\n");
    test_http_protocol_url(posttest_url, HTTP_TEST, TEST_FIRST_HTTP|TEST_POST|TEST_ASYNCREQ, TYMED_ISTREAM);

    trace("Testing http protocol (direct read)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON;
    test_http_protocol_url(winetest_url, HTTP_TEST, TEST_DIRECT_READ|TEST_USEIURI, TYMED_NULL);

    trace("Testing http protocol (redirected)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    test_http_protocol_url(redirect_url, HTTP_TEST, TEST_REDIRECT, TYMED_NULL);

    trace("Testing http protocol (redirected, disable auto redirect)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    test_http_protocol_url(redirect_url, HTTP_TEST, TEST_REDIRECT | TEST_DISABLEAUTOREDIRECT, TYMED_NULL);

    trace("Testing http protocol empty file...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    test_http_protocol_url(empty_url, HTTP_TEST, TEST_EMPTY, TYMED_NULL);

    trace("Testing http protocol (redirected, binding)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    test_http_protocol_url(redirect_url, HTTP_TEST, TEST_REDIRECT|TEST_USEBINDING, TYMED_NULL);

    /* This is a bit ugly. We unconditionally disable this test on Wine. This won't work until we have
     * support for reading from cache via HTTP layer in wininet. Until then, Wine will fail badly, affecting
     * other, unrelated, tests. Working around it is not worth the trouble, we may simply make sure those
     * tests work on Windows and have them around for the future.
     */
    if(broken(1)) {
    trace("Testing http protocol (from cache)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON;
    test_http_protocol_url(cache_only_url, HTTP_TEST, TEST_FROMCACHE, TYMED_NULL);
    }

    trace("Testing http protocol abort...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    test_http_protocol_url(winetest_url, HTTP_TEST, TEST_ABORT, TYMED_NULL);

    test_early_abort(&CLSID_HttpProtocol);
    test_early_abort(&CLSID_HttpSProtocol);
}

static void test_https_protocol(void)
{
    static const WCHAR https_winehq_url[] =
        {'h','t','t','p','s',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/',
         't','e','s','t','s','/','h','e','l','l','o','.','h','t','m','l',0};

    trace("Testing https protocol (from urlmon)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    test_http_protocol_url(https_winehq_url, HTTPS_TEST, TEST_FIRST_HTTP, TYMED_NULL);
}


static void test_ftp_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IClassFactory *factory;
    IUnknown *unk;
    BYTE buf[4096];
    ULONG ref;
    DWORD cb, ret;
    HRESULT hres;

    static const WCHAR ftp_urlW[] = {'f','t','p',':','/','/','f','t','p','.','w','i','n','e','h','q','.','o','r','g',
    '/','p','u','b','/','o','t','h','e','r','/',
    'w','i','n','e','l','o','g','o','.','x','c','f','.','t','a','r','.','b','z','2',0};

    trace("Testing ftp protocol...\n");

    init_test(FTP_TEST, 0);

    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    state = STATE_STARTDOWNLOADING;
    expect_hrResult = E_PENDING;

    hres = CoGetClassObject(&CLSID_FtpProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE, "Could not get IInternetProtocolInfo interface: %08lx, expected E_NOINTERFACE\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                        (void**)&async_protocol);
    IClassFactory_Release(factory);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

    test_priority(async_protocol);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportProgress_FINDINGRESOURCE);
    SET_EXPECT(ReportProgress_CONNECTING);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(Switch);

    hres = IInternetProtocol_Start(async_protocol, ftp_urlW, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08lx\n", hres);
    CHECK_CALLED(GetBindInfo);

    SET_EXPECT(ReportResult);

    hres = IInternetProtocol_Read(async_protocol, buf, 1, &cb);
    ok((hres == E_PENDING && cb==0) ||
       (hres == S_OK && cb==1), "Read failed: %08lx (%ld bytes)\n", hres, cb);

    ret = WaitForSingleObject(event_complete, 10000);
    if (ret != WAIT_OBJECT_0)
    {
        skip( "FTP protocol timed out\n" );
        IInternetProtocol_Release(async_protocol);
        return;
    }

    while(1) {
        hres = IInternetProtocol_Read(async_protocol, buf, sizeof(buf), &cb);
        if(hres == E_PENDING)
        {
            DWORD ret = WaitForSingleObject(event_complete, 10000);
            ok( ret == WAIT_OBJECT_0, "wait timed out\n" );
            if (ret != WAIT_OBJECT_0) break;
        }
        else
            if(cb == 0) break;
    }

    ok(hres == S_FALSE, "Read failed: %08lx\n", hres);
    CHECK_CALLED(ReportResult);
    CHECK_CALLED(Switch);

    test_protocol_terminate(async_protocol);

    if(pCreateUri) {
        IInternetProtocolEx *protocolex;

        hres = IInternetProtocol_QueryInterface(async_protocol, &IID_IInternetProtocolEx, (void**)&protocolex);
        ok(hres == S_OK, "Could not get IInternetProtocolEx iface: %08lx\n", hres);
        IInternetProtocolEx_Release(protocolex);
    }

    ref = IInternetProtocol_Release(async_protocol);
    ok(!ref, "ref=%ld\n", ref);

    test_early_abort(&CLSID_FtpProtocol);
}

static void test_gopher_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IClassFactory *factory;
    IUnknown *unk;
    HRESULT hres;

    trace("Testing gopher protocol...\n");

    hres = CoGetClassObject(&CLSID_GopherProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK ||
       broken(hres == REGDB_E_CLASSNOTREG || hres == CLASS_E_CLASSNOTAVAILABLE), /* Gopher protocol has been removed as of Vista */
       "CoGetClassObject failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE, "Could not get IInternetProtocolInfo interface: %08lx, expected E_NOINTERFACE\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                        (void**)&async_protocol);
    IClassFactory_Release(factory);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

    test_priority(async_protocol);

    IInternetProtocol_Release(async_protocol);

    test_early_abort(&CLSID_GopherProtocol);
}

static void test_mk_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IInternetProtocol *protocol;
    IClassFactory *factory;
    IUnknown *unk;
    HRESULT hres;

    static const WCHAR wrong_url1[] = {'t','e','s','t',':','@','M','S','I','T','S','t','o','r','e',
                                       ':',':','/','t','e','s','t','.','h','t','m','l',0};
    static const WCHAR wrong_url2[] = {'m','k',':','/','t','e','s','t','.','h','t','m','l',0};

    trace("Testing mk protocol...\n");
    init_test(MK_TEST, 0);

    hres = CoGetClassObject(&CLSID_MkProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
        "Could not get IInternetProtocolInfo interface: %08lx, expected E_NOINTERFACE\n",
        hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                        (void**)&protocol);
    IClassFactory_Release(factory);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

    SET_EXPECT(GetBindInfo);
    hres = IInternetProtocol_Start(protocol, wrong_url1, &protocol_sink, &bind_info, 0, 0);
    ok(hres == MK_E_SYNTAX || hres == INET_E_INVALID_URL,
       "Start failed: %08lx, expected MK_E_SYNTAX or INET_E_INVALID_URL\n", hres);
    CLEAR_CALLED(GetBindInfo);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    SET_EXPECT(ReportResult);
    expect_hrResult = INET_E_RESOURCE_NOT_FOUND;

    hres = IInternetProtocol_Start(protocol, wrong_url2, &protocol_sink, &bind_info, 0, 0);
    ok(hres == INET_E_RESOURCE_NOT_FOUND ||
       hres == INET_E_INVALID_URL, /* win2k3 */
       "Start failed: %08lx, expected INET_E_RESOURCE_NOT_FOUND or INET_E_INVALID_URL\n", hres);

    if (hres == INET_E_RESOURCE_NOT_FOUND) {
        CHECK_CALLED(GetBindInfo);
        CLEAR_CALLED(ReportProgress_DIRECTBIND);
        CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(ReportResult);
    }else {
        CLEAR_CALLED(GetBindInfo);
        CLEAR_CALLED(ReportProgress_DIRECTBIND);
        CLEAR_CALLED(ReportProgress_SENDINGREQUEST);
        CLEAR_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        CLEAR_CALLED(ReportResult);
    }

    IInternetProtocol_Release(protocol);
}

static void test_CreateBinding(void)
{
    IInternetProtocol *protocol;
    IInternetPriority *priority;
    IInternetSession *session;
    IWinInetHttpInfo *http_info;
    IWinInetInfo *inet_info;
    LONG p;
    BYTE buf[1000];
    DWORD read;
    HRESULT hres;

    static const WCHAR test_url[] =
        {'t','e','s','t',':','/','/','f','i','l','e','.','h','t','m','l',0};
    static const WCHAR wsz_test[] = {'t','e','s','t',0};

    trace("Testing CreateBinding%s...\n", no_aggregation ? "(no aggregation)" : "");
    init_test(BIND_TEST, TEST_BINDING);

    hres = pCoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &ClassFactory, &IID_NULL, wsz_test, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

    hres = IInternetSession_CreateBinding(session, NULL, test_url, NULL, NULL, &protocol, 0);
    binding_protocol = protocol;
    ok(hres == S_OK, "CreateBinding failed: %08lx\n", hres);
    ok(protocol != NULL, "protocol == NULL\n");

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetBindInfo, (void**)&prot_bind_info);
    ok(hres == S_OK, "QueryInterface(IID_IInternetBindInfo) failed: %08lx\n", hres);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetProtocolSink, (void**)&binding_sink);
    ok(hres == S_OK, "Could not get IInternetProtocolSink: %08lx\n", hres);

    hres = IInternetProtocol_Start(protocol, test_url, NULL, &bind_info, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08lx, expected E_INVALIDARG\n", hres);
    hres = IInternetProtocol_Start(protocol, test_url, &protocol_sink, NULL, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08lx, expected E_INVALIDARG\n", hres);
    hres = IInternetProtocol_Start(protocol, NULL, &protocol_sink, &bind_info, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority, (void**)&priority);
    ok(hres == S_OK, "QueryInterface(IID_IInternetPriority) failed: %08lx\n", hres);

    p = 0xdeadbeef;
    hres = IInternetPriority_GetPriority(priority, &p);
    ok(hres == S_OK, "GetPriority failed: %08lx\n", hres);
    ok(!p, "p=%ld\n", p);

    ex_priority = 100;
    hres = IInternetPriority_SetPriority(priority, 100);
    ok(hres == S_OK, "SetPriority failed: %08lx\n", hres);

    p = 0xdeadbeef;
    hres = IInternetPriority_GetPriority(priority, &p);
    ok(hres == S_OK, "GetPriority failed: %08lx\n", hres);
    ok(p == 100, "p=%ld\n", p);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IWinInetInfo, (void**)&inet_info);
    ok(hres == E_NOINTERFACE, "Could not get IWinInetInfo protocol: %08lx\n", hres);

    SET_EXPECT(QueryService_InternetProtocol);

    SET_EXPECT(CreateInstance);
    if(no_aggregation) {
        SET_EXPECT(CreateInstance_no_aggregation);
        SET_EXPECT(StartEx);
    }else {
        SET_EXPECT(Start);
    }

    SET_EXPECT(ReportProgress_PROTOCOLCLASSID);
    SET_EXPECT(SetPriority);

    ok(obj_refcount(protocol) == 4, "wrong protocol refcount %ld\n", obj_refcount(protocol));

    expect_hrResult = S_OK;
    hres = IInternetProtocol_Start(protocol, test_url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08lx\n", hres);

    CHECK_CALLED(QueryService_InternetProtocol);

    CHECK_CALLED(CreateInstance);
    if(no_aggregation) {
        CHECK_CALLED(CreateInstance_no_aggregation);
        ok(obj_refcount(protocol) == 4, "wrong protocol refcount %ld\n", obj_refcount(protocol));
        ok(protocol_emul->outer_ref == 0, "protocol_outer_ref = %lu\n", protocol_emul->outer_ref);
    }else {
        ok(obj_refcount(protocol) == 5 || broken(obj_refcount(protocol) == 4) /* before win7 */, "wrong protocol refcount %ld\n",
           obj_refcount(protocol));
        ok(protocol_emul->outer_ref == 1 || broken(protocol_emul->outer_ref == 0) /* before win7 */, "protocol_outer_ref = %lu\n",
           protocol_emul->outer_ref);
    }

    CHECK_CALLED(ReportProgress_PROTOCOLCLASSID);
    CHECK_CALLED(SetPriority);
    if(no_aggregation)
        CHECK_CALLED(StartEx);
    else
        CHECK_CALLED(Start);

    if(!no_aggregation)
        SET_EXPECT(QueryInterface_IWinInetInfo);
    hres = IInternetProtocol_QueryInterface(protocol, &IID_IWinInetInfo, (void**)&inet_info);
    ok(hres == E_NOINTERFACE, "Could not get IWinInetInfo protocol: %08lx\n", hres);
    if(!no_aggregation)
        CHECK_CALLED(QueryInterface_IWinInetInfo);

    if(!no_aggregation)
        SET_EXPECT(QueryInterface_IWinInetInfo);
    hres = IInternetProtocol_QueryInterface(protocol, &IID_IWinInetInfo, (void**)&inet_info);
    ok(hres == E_NOINTERFACE, "Could not get IWinInetInfo protocol: %08lx\n", hres);
    if(!no_aggregation)
        CHECK_CALLED(QueryInterface_IWinInetInfo);

    if(!no_aggregation)
        SET_EXPECT(QueryInterface_IWinInetHttpInfo);
    hres = IInternetProtocol_QueryInterface(protocol, &IID_IWinInetHttpInfo, (void**)&http_info);
    ok(hres == E_NOINTERFACE, "Could not get IWinInetInfo protocol: %08lx\n", hres);
    if(!no_aggregation)
        CHECK_CALLED(QueryInterface_IWinInetHttpInfo);

    SET_EXPECT(Read);
    read = 0xdeadbeef;
    hres = IInternetProtocol_Read(protocol, expect_pv = buf, sizeof(buf), &read);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    ok(read == 100, "read = %ld\n", read);
    CHECK_CALLED(Read);

    SET_EXPECT(Read);
    read = 0xdeadbeef;
    hres = IInternetProtocol_Read(protocol, expect_pv = buf, sizeof(buf), &read);
    ok(hres == S_FALSE, "Read failed: %08lx\n", hres);
    ok(!read, "read = %ld\n", read);
    CHECK_CALLED(Read);

    p = 0xdeadbeef;
    hres = IInternetPriority_GetPriority(priority, &p);
    ok(hres == S_OK, "GetPriority failed: %08lx\n", hres);
    ok(p == 100, "p=%ld\n", p);

    hres = IInternetPriority_SetPriority(priority, 101);
    ok(hres == S_OK, "SetPriority failed: %08lx\n", hres);

    if(no_aggregation) {
        ok(obj_refcount(protocol) == 4, "wrong protocol refcount %ld\n", obj_refcount(protocol));
        ok(protocol_emul->outer_ref == 0, "protocol_outer_ref = %lu\n", protocol_emul->outer_ref);
    }else {
        ok(obj_refcount(protocol) == 5 || broken(obj_refcount(protocol) == 4) /* before win7 */, "wrong protocol refcount %ld\n", obj_refcount(protocol));
        ok(protocol_emul->outer_ref == 1 || broken(protocol_emul->outer_ref == 0) /* before win7 */, "protocol_outer_ref = %lu\n", protocol_emul->outer_ref);
    }

    SET_EXPECT(Terminate);
    hres = IInternetProtocol_Terminate(protocol, 0xdeadbeef);
    ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
    CHECK_CALLED(Terminate);

    ok(obj_refcount(protocol) == 4, "wrong protocol refcount %ld\n", obj_refcount(protocol));
    ok(protocol_emul->outer_ref == 0, "protocol_outer_ref = %lu\n", protocol_emul->outer_ref);

    SET_EXPECT(Continue);
    hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08lx\n", hres);
    CHECK_CALLED(Continue);

    SET_EXPECT(Read);
    read = 0xdeadbeef;
    hres = IInternetProtocol_Read(protocol, expect_pv = buf, sizeof(buf), &read);
    if(no_aggregation) {
        ok(hres == S_OK, "Read failed: %08lx\n", hres);
        ok(read == 100, "read = %ld\n", read);
        CHECK_CALLED(Read);
    }else {
        todo_wine
        ok(hres == E_ABORT, "Read failed: %08lx\n", hres);
        todo_wine
        ok(read == 0, "read = %ld\n", read);
        todo_wine
        CHECK_NOT_CALLED(Read);
    }

    hres = IInternetProtocolSink_ReportProgress(binding_sink,
            BINDSTATUS_CACHEFILENAMEAVAILABLE, expect_wsz = emptyW);
    ok(hres == S_OK, "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08lx\n", hres);

    hres = IInternetProtocolSink_ReportResult(binding_sink, S_OK, ERROR_SUCCESS, NULL);
    ok(hres == E_FAIL, "ReportResult failed: %08lx, expected E_FAIL\n", hres);

    hres = IInternetProtocolSink_ReportData(binding_sink, 0, 0, 0);
    ok(hres == S_OK, "ReportData failed: %08lx\n", hres);

    IInternetProtocolSink_Release(binding_sink);
    IInternetPriority_Release(priority);
    IInternetBindInfo_Release(prot_bind_info);

    ok(obj_refcount(protocol) == 1, "wrong protocol refcount %ld\n", obj_refcount(protocol));

    SET_EXPECT(Protocol_destructor);
    IInternetProtocol_Release(protocol);
    CHECK_CALLED(Protocol_destructor);

    hres = IInternetSession_CreateBinding(session, NULL, test_url, NULL, NULL, &protocol, 0);
    ok(hres == S_OK, "CreateBinding failed: %08lx\n", hres);
    ok(protocol != NULL, "protocol == NULL\n");

    hres = IInternetProtocol_Abort(protocol, E_ABORT, 0);
    ok(hres == S_OK, "Abort failed: %08lx\n", hres);

    hres = IInternetProtocol_Abort(protocol, E_FAIL, 0);
    ok(hres == S_OK, "Abort failed: %08lx\n", hres);

    IInternetProtocol_Release(protocol);

    hres = IInternetSession_UnregisterNameSpace(session, &ClassFactory, wsz_test);
    ok(hres == S_OK, "UnregisterNameSpace failed: %08lx\n", hres);

    hres = IInternetSession_CreateBinding(session, NULL, test_url, NULL, NULL, &protocol, 0);
    ok(hres == S_OK, "CreateBinding failed: %08lx\n", hres);
    ok(protocol != NULL, "protocol == NULL\n");

    SET_EXPECT(QueryService_InternetProtocol);
    hres = IInternetProtocol_Start(protocol, test_url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == MK_E_SYNTAX, "Start failed: %08lx, expected MK_E_SYNTAX\n", hres);
    CHECK_CALLED(QueryService_InternetProtocol);

    IInternetProtocol_Release(protocol);

    IInternetSession_Release(session);
}

static void test_binding(int prot, DWORD grf_pi, DWORD test_flags)
{
    IInternetProtocolEx *protocolex = NULL;
    IInternetProtocol *protocol;
    IInternetSession *session;
    IUri *uri = NULL;
    ULONG ref;
    HRESULT hres;

    pi = grf_pi;

    init_test(prot, test_flags|TEST_BINDING);

    hres = pCoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);

    if(test_flags & TEST_EMULATEPROT) {
        hres = IInternetSession_RegisterNameSpace(session, &ClassFactory, &IID_NULL, protocol_names[prot], 0, NULL, 0);
        ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);
    }

    hres = IInternetSession_CreateBinding(session, NULL, binding_urls[prot], NULL, NULL, &protocol, 0);
    binding_protocol = protocol;
    ok(hres == S_OK, "CreateBinding failed: %08lx\n", hres);
    ok(protocol != NULL, "protocol == NULL\n");

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetBindInfo, (void**)&prot_bind_info);
    ok(hres == S_OK, "QueryInterface(IID_IInternetBindInfo) failed: %08lx\n", hres);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetProtocolSink, (void**)&binding_sink);
    ok(hres == S_OK, "QueryInterface(IID_IInternetProtocolSink) failed: %08lx\n", hres);

    if(test_flags & TEST_USEIURI) {
        hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetProtocolEx, (void**)&protocolex);
        ok(hres == S_OK, "Could not get IInternetProtocolEx iface: %08lx\n", hres);

        hres = pCreateUri(binding_urls[prot], Uri_CREATE_FILE_USE_DOS_PATH, 0, &uri);
        ok(hres == S_OK, "CreateUri failed: %08lx\n", hres);
    }

    ex_priority = 0;
    SET_EXPECT(QueryService_InternetProtocol);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(ReportProgress_PROTOCOLCLASSID);
    SET_EXPECT(SetPriority);
    if(impl_protex)
        SET_EXPECT(StartEx);
    else
        SET_EXPECT(Start);

    expect_hrResult = S_OK;

    if(protocolex) {
        hres = IInternetProtocolEx_StartEx(protocolex, uri, &protocol_sink, &bind_info, pi, 0);
        ok(hres == S_OK, "StartEx failed: %08lx\n", hres);
    }else {
        hres = IInternetProtocol_Start(protocol, binding_urls[prot], &protocol_sink, &bind_info, pi, 0);
        ok(hres == S_OK, "Start failed: %08lx\n", hres);
    }

    CHECK_CALLED(QueryService_InternetProtocol);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(ReportProgress_PROTOCOLCLASSID);
    CLEAR_CALLED(SetPriority); /* IE11 does not call it. */
    if(impl_protex)
        CHECK_CALLED(StartEx);
    else
        CHECK_CALLED(Start);

    if(protocolex)
        IInternetProtocolEx_Release(protocolex);
    if(uri)
        IUri_Release(uri);

    if(prot == HTTP_TEST || prot == HTTPS_TEST) {
        while(prot_state < 4) {
            ok( WaitForSingleObject(event_complete, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
            if(mimefilter_test && filtered_protocol) {
                SET_EXPECT(Continue);
                IInternetProtocol_Continue(filtered_protocol, pdata);
                CHECK_CALLED(Continue);
            }else {
                SET_EXPECT(Continue);
                IInternetProtocol_Continue(protocol, pdata);
                CHECK_CALLED(Continue);
            }
            if(test_abort && prot_state == 2) {
                SET_EXPECT(Abort);
                hres = IInternetProtocol_Abort(protocol, E_ABORT, 0);
                ok(hres == S_OK, "Abort failed: %08lx\n", hres);
                CHECK_CALLED(Abort);

                hres = IInternetProtocol_Abort(protocol, E_ABORT, 0);
                ok(hres == S_OK, "Abort failed: %08lx\n", hres);
                SetEvent(event_complete2);
                break;
            }
            SetEvent(event_complete2);
        }
        if(direct_read)
            CHECK_CALLED(ReportData); /* Set in ReportResult */
        ok( WaitForSingleObject(event_complete, 90000) == WAIT_OBJECT_0, "wait timed out\n" );
    }else {
        if(!result_from_lock) {
            if(mimefilter_test)
                SET_EXPECT(MimeFilter_LockRequest);
            else
                SET_EXPECT(LockRequest);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            if(mimefilter_test)
                CHECK_CALLED(MimeFilter_LockRequest);
            else
                CHECK_CALLED(LockRequest);
        }

        if(mimefilter_test)
            SET_EXPECT(MimeFilter_UnlockRequest);
        else
            SET_EXPECT(UnlockRequest);
        hres = IInternetProtocol_UnlockRequest(protocol);
        ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
        if(mimefilter_test)
            CHECK_CALLED(MimeFilter_UnlockRequest);
        else
            CHECK_CALLED(UnlockRequest);
    }

    if(mimefilter_test)
        SET_EXPECT(MimeFilter_Terminate);
    else
        SET_EXPECT(Terminate);
    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
    if(mimefilter_test)
        CLEAR_CALLED(MimeFilter_Terminate);
    else
        CHECK_CALLED(Terminate);

    if(filtered_protocol)
        IInternetProtocol_Release(filtered_protocol);
    IInternetBindInfo_Release(prot_bind_info);
    IInternetProtocolSink_Release(binding_sink);

    SET_EXPECT(Protocol_destructor);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "ref=%lu, expected 0\n", ref);
    CHECK_CALLED(Protocol_destructor);

    if(test_flags & TEST_EMULATEPROT) {
        hres = IInternetSession_UnregisterNameSpace(session, &ClassFactory, protocol_names[prot]);
        ok(hres == S_OK, "UnregisterNameSpace failed: %08lx\n", hres);
    }

    IInternetSession_Release(session);
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
    ok(hres == CLASS_E_NOAGGREGATION, "CreateInstance returned: %08lx\n", hres);
    ok(!unk, "unk = %p\n", unk);

    IClassFactory_Release(class_factory);
}

START_TEST(protocol)
{
    HMODULE hurlmon;

    hurlmon = GetModuleHandleA("urlmon.dll");
    pCoInternetGetSession = (void*) GetProcAddress(hurlmon, "CoInternetGetSession");
    pReleaseBindInfo = (void*) GetProcAddress(hurlmon, "ReleaseBindInfo");
    pCreateUri = (void*) GetProcAddress(hurlmon, "CreateUri");

    if(!GetProcAddress(hurlmon, "CompareSecurityIds")) {
        win_skip("Various needed functions not present, too old IE\n");
        return;
    }

    if(!pCreateUri)
        win_skip("CreateUri not supported\n");

    OleInitialize(NULL);

    event_complete = CreateEventW(NULL, FALSE, FALSE, NULL);
    event_complete2 = CreateEventW(NULL, FALSE, FALSE, NULL);
    event_continue = CreateEventW(NULL, FALSE, FALSE, NULL);
    event_continue_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    thread_id = GetCurrentThreadId();

    test_file_protocol();
    test_http_protocol();
    if(pCreateUri)
        test_https_protocol();
    else
        win_skip("Skipping https tests on too old platform\n");
    test_ftp_protocol();
    test_gopher_protocol();
    test_mk_protocol();
    test_CreateBinding();
    no_aggregation = TRUE;
    test_CreateBinding();
    no_aggregation = FALSE;

    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_NOWRITECACHE | BINDF_PULLDATA;
    trace("Testing file binding (mime verification, emulate prot)...\n");
    test_binding(FILE_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT);
    trace("Testing http binding (mime verification, emulate prot)...\n");
    test_binding(HTTP_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT);
    trace("Testing its binding (mime verification, emulate prot)...\n");
    test_binding(ITS_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT);
    trace("Testing http binding (mime verification, emulate prot, short read, direct read)...\n");
    test_binding(HTTP_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_SHORT_READ|TEST_DIRECT_READ);
    trace("Testing http binding (mime verification, redirect, emulate prot)...\n");
    test_binding(HTTP_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_REDIRECT);
    trace("Testing http binding (mime verification, redirect, disable auto redirect, emulate prot)...\n");
    test_binding(HTTP_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_REDIRECT|TEST_DISABLEAUTOREDIRECT);
    trace("Testing file binding (mime verification, emulate prot, mime filter)...\n");
    test_binding(FILE_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_FILTER);
    trace("Testing http binding (mime verification, emulate prot, mime filter)...\n");
    test_binding(HTTP_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_FILTER);
    trace("Testing http binding (mime verification, emulate prot, mime filter, no mime)...\n");
    test_binding(HTTP_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_FILTER|TEST_NOMIME);
    trace("Testing http binding (mime verification, emulate prot, direct read)...\n");
    test_binding(HTTP_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_DIRECT_READ);
    trace("Testing http binding (mime verification, emulate prot, abort)...\n");
    test_binding(HTTP_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_ABORT);
    trace("Testing its binding (mime verification, emulate prot, apartment thread)...\n");
    test_binding(ITS_TEST, PI_MIMEVERIFICATION | PI_APARTMENTTHREADED, TEST_EMULATEPROT | TEST_RESULTFROMLOCK);
    if(pCreateUri) {
        trace("Testing file binding (use IUri, mime verification, emulate prot)...\n");
        test_binding(FILE_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_USEIURI);
        trace("Testing file binding (use IUri, impl StartEx, mime verification, emulate prot)...\n");
        test_binding(FILE_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_USEIURI|TEST_IMPLPROTEX);
        trace("Testing file binding (impl StartEx, mime verification, emulate prot)...\n");
        test_binding(FILE_TEST, PI_MIMEVERIFICATION, TEST_EMULATEPROT|TEST_IMPLPROTEX);
    }

    CloseHandle(event_complete);
    CloseHandle(event_complete2);
    CloseHandle(event_continue);
    CloseHandle(event_continue_done);

    test_com_aggregation(&CLSID_FileProtocol);
    test_com_aggregation(&CLSID_HttpProtocol);
    test_com_aggregation(&CLSID_HttpSProtocol);
    test_com_aggregation(&CLSID_FtpProtocol);
    test_com_aggregation(&CLSID_MkProtocol);

    OleUninitialize();
}
