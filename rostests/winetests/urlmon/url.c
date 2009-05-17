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
#define NONAMELESSUNION
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "urlmon.h"
#include "wininet.h"
#include "mshtml.h"

#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(CLSID_IdentityUnmarshal,0x0000001b,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_IBindStatusCallbackHolder,0x79eac9cc,0xbaf9,0x11ce,0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b);

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
DEFINE_EXPECT(QueryInterface_IBindStatusCallbackHolder);
DEFINE_EXPECT(QueryInterface_IInternetBindInfo);
DEFINE_EXPECT(QueryInterface_IAuthenticate);
DEFINE_EXPECT(QueryInterface_IInternetProtocol);
DEFINE_EXPECT(QueryService_IAuthenticate);
DEFINE_EXPECT(QueryService_IInternetProtocol);
DEFINE_EXPECT(QueryService_IInternetBindInfo);
DEFINE_EXPECT(BeginningTransaction);
DEFINE_EXPECT(OnResponse);
DEFINE_EXPECT(QueryInterface_IHttpNegotiate2);
DEFINE_EXPECT(GetRootSecurityId);
DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(OnStartBinding);
DEFINE_EXPECT(OnProgress_FINDINGRESOURCE);
DEFINE_EXPECT(OnProgress_CONNECTING);
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
DEFINE_EXPECT(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
DEFINE_EXPECT(Start);
DEFINE_EXPECT(Read);
DEFINE_EXPECT(LockRequest);
DEFINE_EXPECT(Terminate);
DEFINE_EXPECT(UnlockRequest);
DEFINE_EXPECT(Continue);
DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(Load);
DEFINE_EXPECT(PutProperty_MIMETYPEPROP);
DEFINE_EXPECT(PutProperty_CLASSIDPROP);
DEFINE_EXPECT(SetPriority);

static const WCHAR TEST_URL_1[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','\0'};
static const WCHAR TEST_PART_URL_1[] = {'/','t','e','s','t','/','\0'};

static const WCHAR WINE_ABOUT_URL[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.',
                                       'o','r','g','/','s','i','t','e','/','a','b','o','u','t',0};
static const WCHAR SHORT_RESPONSE_URL[] =
        {'h','t','t','p',':','/','/','c','r','o','s','s','o','v','e','r','.',
         'c','o','d','e','w','e','a','v','e','r','s','.','c','o','m','/',
         'p','o','s','t','t','e','s','t','.','p','h','p',0};
static const WCHAR ABOUT_BLANK[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
static WCHAR INDEX_HTML[MAX_PATH];
static const WCHAR ITS_URL[] =
    {'i','t','s',':','t','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR MK_URL[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
    't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR https_urlW[] =
    {'h','t','t','p','s',':','/','/','w','w','w','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',
     '/','t','e','s','t','.','h','t','m','l',0};
static const WCHAR ftp_urlW[] = {'f','t','p',':','/','/','f','t','p','.','w','i','n','e','h','q','.','o','r','g',
    '/','p','u','b','/','o','t','h','e','r','/',
    'w','i','n','e','l','o','g','o','.','x','c','f','.','t','a','r','.','b','z','2',0};


static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};

static WCHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

static const WCHAR wszWineHQSite[] =
    {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR wszWineHQIP[] =
    {'2','0','9','.','3','2','.','1','4','1','.','3',0};
static const CHAR wszIndexHtmlA[] = "index.html";
static const WCHAR wszIndexHtml[] = {'i','n','d','e','x','.','h','t','m','l',0};
static const WCHAR cache_fileW[] = {'c',':','\\','c','a','c','h','e','.','h','t','m',0};
static const CHAR dwl_htmlA[] = "dwl.html";
static const WCHAR dwl_htmlW[] = {'d','w','l','.','h','t','m','l',0};
static const WCHAR emptyW[] = {0};

static BOOL stopped_binding = FALSE, stopped_obj_binding = FALSE, emulate_protocol = FALSE,
    data_available = FALSE, http_is_first = TRUE, bind_to_object = FALSE, filedwl_api;
static DWORD read = 0, bindf = 0, prot_state = 0, thread_id, tymed;
static CHAR mime_type[512];
static IInternetProtocolSink *protocol_sink = NULL;
static IBinding *current_binding;
static HANDLE complete_event, complete_event2;
static HRESULT binding_hres;
static BOOL have_IHttpNegotiate2;

static LPCWSTR urls[] = {
    WINE_ABOUT_URL,
    ABOUT_BLANK,
    INDEX_HTML,
    ITS_URL,
    MK_URL,
    https_urlW,
    ftp_urlW
};

static WCHAR file_url[INTERNET_MAX_URL_LENGTH];

static enum {
    HTTP_TEST,
    ABOUT_TEST,
    FILE_TEST,
    ITS_TEST,
    MK_TEST,
    HTTPS_TEST,
    FTP_TEST
} test_protocol;

static enum {
    BEFORE_DOWNLOAD,
    DOWNLOADING,
    END_DOWNLOAD
} download_state;

static const char *debugstr_w(LPCWSTR str)
{
    static char buf[1024];
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
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

    hr = CreateURLMoniker(NULL, url1, &mon1);
    ok(SUCCEEDED(hr), "failed to create moniker: 0x%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = CreateURLMoniker(mon1, url2, &mon2);
        ok(SUCCEEDED(hr), "failed to create moniker: 0x%08x\n", hr);
    }
    if(mon1) IMoniker_Release(mon1);
    if(mon2) IMoniker_Release(mon2);
}

static void test_create(void)
{
    test_CreateURLMoniker(TEST_URL_1, TEST_PART_URL_1);
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
    ok(!nPriority, "nPriority = %d\n", nPriority);
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

    if(IsEqualGUID(&IID_IInternetProtocolEx, riid))
        return E_NOINTERFACE; /* TODO */

    ok(0, "unexpected call %s\n", debugstr_guid(riid));
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

static DWORD WINAPI thread_proc(PVOID arg)
{
    PROTOCOLDATA protocoldata;
    HRESULT hres;

    if(bind_to_object)
        SET_EXPECT(Obj_OnProgress_FINDINGRESOURCE);
    else
        SET_EXPECT(OnProgress_FINDINGRESOURCE);
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_FINDINGRESOURCE, wszWineHQSite);
    ok(hres == S_OK, "ReportProgress failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    if(bind_to_object)
        CHECK_CALLED(Obj_OnProgress_FINDINGRESOURCE);
    else
        CHECK_CALLED(OnProgress_FINDINGRESOURCE);

    if(bind_to_object)
        SET_EXPECT(Obj_OnProgress_CONNECTING);
    else
        SET_EXPECT(OnProgress_CONNECTING);
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_CONNECTING, wszWineHQIP);
    ok(hres == S_OK, "ReportProgress failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    if(bind_to_object)
        CHECK_CALLED(Obj_OnProgress_CONNECTING);
    else
        CHECK_CALLED(OnProgress_CONNECTING);

    if(bind_to_object)
        SET_EXPECT(Obj_OnProgress_SENDINGREQUEST);
    else
        SET_EXPECT(OnProgress_SENDINGREQUEST);
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_SENDINGREQUEST, NULL);
    ok(hres == S_OK, "ReportProxgress failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    if(bind_to_object)
        CHECK_CALLED(Obj_OnProgress_SENDINGREQUEST);
    else
        CHECK_CALLED(OnProgress_SENDINGREQUEST);

    SET_EXPECT(Continue);
    prot_state = 1;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);

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
    }else {
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        CHECK_CALLED(LockRequest);
        CHECK_CALLED(OnDataAvailable);
    }

    SET_EXPECT(Continue);
    prot_state = 2;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_CALLED(Continue);
    CHECK_CALLED(Read);
    CHECK_CALLED(OnProgress_DOWNLOADINGDATA);
    CHECK_CALLED(OnDataAvailable);

    SET_EXPECT(Continue);
    prot_state = 2;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_CALLED(Continue);
    CHECK_CALLED(Read);
    CHECK_CALLED(OnProgress_DOWNLOADINGDATA);
    CHECK_CALLED(OnDataAvailable);

    SET_EXPECT(Continue);
    prot_state = 3;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_CALLED(Continue);
    CHECK_CALLED(Read);
    CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
    CHECK_CALLED(OnDataAvailable);
    CHECK_CALLED(OnStopBinding);

    SET_EXPECT(Read);

    SetEvent(complete_event2);
    return 0;
}

static HRESULT WINAPI Protocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    BINDINFO bindinfo;
    DWORD bindf, bscf = BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION;
    HRESULT hres;

    static const STGMEDIUM stgmed_zero = {0};
    static const SECURITY_ATTRIBUTES sa_zero = {0};

    CHECK_EXPECT(Start);

    read = 0;

    if(!filedwl_api) /* FIXME */
        ok(szUrl && !lstrcmpW(szUrl, urls[test_protocol]), "wrong url %s\n", debugstr_w(szUrl));
    ok(pOIProtSink != NULL, "pOIProtSink == NULL\n");
    ok(pOIBindInfo != NULL, "pOIBindInfo == NULL\n");
    ok(grfPI == 0, "grfPI=%d, expected 0\n", grfPI);
    ok(dwReserved == 0, "dwReserved=%lx, expected 0\n", dwReserved);

    if(!filedwl_api && binding_hres != S_OK) {
        SET_EXPECT(OnStopBinding);
        SET_EXPECT(Terminate);
        hres = IInternetProtocolSink_ReportResult(pOIProtSink, binding_hres, 0, NULL);
        ok(hres == S_OK, "ReportResult failed: %08x\n", hres);
        CHECK_CALLED(OnStopBinding);
        CHECK_CALLED(Terminate);

        return S_OK;
    }

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08x\n", hres);

    if(filedwl_api) {
        ok(bindf == (BINDF_PULLDATA|BINDF_FROMURLMON|BINDF_NEEDFILE), "bindf=%08x\n", bindf);
    }else if(tymed == TYMED_ISTREAM && is_urlmon_protocol(test_protocol)) {
        ok(bindf == (BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA
                     |BINDF_FROMURLMON),
           "bindf=%08x\n", bindf);
    }else {
        ok(bindf == (BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA
                     |BINDF_FROMURLMON|BINDF_NEEDFILE),
           "bindf=%08x\n", bindf);
    }

    ok(bindinfo.cbSize == sizeof(bindinfo), "bindinfo.cbSize = %d\n", bindinfo.cbSize);
    ok(!bindinfo.szExtraInfo, "bindinfo.szExtraInfo = %p\n", bindinfo.szExtraInfo);
    ok(!memcmp(&bindinfo.stgmedData, &stgmed_zero, sizeof(STGMEDIUM)), "wrong stgmedData\n");
    ok(!bindinfo.grfBindInfoF, "bindinfo.grfBindInfoF = %d\n", bindinfo.grfBindInfoF);
    ok(!bindinfo.dwBindVerb, "bindinfo.dwBindVerb = %d\n", bindinfo.dwBindVerb);
    ok(!bindinfo.szCustomVerb, "bindinfo.szCustomVerb = %p\n", bindinfo.szCustomVerb);
    ok(!bindinfo.cbstgmedData, "bindinfo.cbstgmedData = %d\n", bindinfo.cbstgmedData);
    ok(bindinfo.dwOptions == (bind_to_object ? 0x100000 : 0), "bindinfo.dwOptions = %x\n", bindinfo.dwOptions);
    ok(!bindinfo.dwOptionsFlags, "bindinfo.dwOptionsFlags = %d\n", bindinfo.dwOptionsFlags);
    ok(!bindinfo.dwCodePage, "bindinfo.dwCodePage = %d\n", bindinfo.dwCodePage);
    ok(!memcmp(&bindinfo.securityAttributes, &sa_zero, sizeof(sa_zero)), "wrong bindinfo.securityAttributes\n");
    ok(IsEqualGUID(&bindinfo.iid, &IID_NULL), "wrong bindinfo.iid\n");
    ok(!bindinfo.pUnk, "bindinfo.pUnk = %p\n", bindinfo.pUnk);
    ok(!bindinfo.dwReserved, "bindinfo.dwReserved = %d\n", bindinfo.dwReserved);

    switch(test_protocol) {
    case MK_TEST:
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_DIRECTBIND, NULL);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08x\n", hres);

    case FILE_TEST:
    case ITS_TEST:
        if(bind_to_object)
            SET_EXPECT(Obj_OnProgress_SENDINGREQUEST);
        else
            SET_EXPECT(OnProgress_SENDINGREQUEST);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_SENDINGREQUEST, emptyW);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08x\n", hres);
        if(bind_to_object)
            CHECK_CALLED(Obj_OnProgress_SENDINGREQUEST);
        else
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
    default:
        break;
    }

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
        IServiceProvider *service_provider;
        IHttpNegotiate *http_negotiate;
        IHttpNegotiate2 *http_negotiate2;
        LPWSTR ua = (LPWSTR)0xdeadbeef, accept_mimes[256];
        LPWSTR additional_headers = (LPWSTR)0xdeadbeef;
        BYTE sec_id[100];
        DWORD fetched = 256, size = 100;
        DWORD tid;

        static const WCHAR wszMimes[] = {'*','/','*',0};

        SET_EXPECT(QueryInterface_IInternetBindInfo);
        SET_EXPECT(QueryService_IInternetBindInfo);
        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_USER_AGENT,
                                               &ua, 1, &fetched);
        todo_wine {
        CHECK_CALLED(QueryInterface_IInternetBindInfo);
        CHECK_CALLED(QueryService_IInternetBindInfo);
        }
        ok(hres == E_NOINTERFACE,
           "GetBindString(BINDSTRING_USER_AGETNT) failed: %08x\n", hres);
        ok(fetched == 256, "fetched = %d, expected 254\n", fetched);
        ok(ua == (LPWSTR)0xdeadbeef, "ua =  %p\n", ua);

        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                               accept_mimes, 256, &fetched);
        ok(hres == S_OK,
           "GetBindString(BINDSTRING_ACCEPT_MIMES) failed: %08x\n", hres);
        ok(fetched == 1, "fetched = %d, expected 1\n", fetched);
        ok(!lstrcmpW(wszMimes, accept_mimes[0]), "unexpected mimes\n");

        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                               NULL, 256, &fetched);
        ok(hres == E_INVALIDARG,
           "GetBindString(BINDSTRING_ACCEPT_MIMES) failed: %08x\n", hres);

        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                               accept_mimes, 256, NULL);
        ok(hres == E_INVALIDARG,
           "GetBindString(BINDSTRING_ACCEPT_MIMES) failed: %08x\n", hres);

        hres = IInternetBindInfo_QueryInterface(pOIBindInfo, &IID_IServiceProvider,
                                                (void**)&service_provider);
        ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);

        SET_EXPECT(QueryInterface_IHttpNegotiate);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
                &IID_IHttpNegotiate, (void**)&http_negotiate);
        CHECK_CALLED(QueryInterface_IHttpNegotiate);
        ok(hres == S_OK, "QueryService failed: %08x\n", hres);

        SET_EXPECT(BeginningTransaction);
        hres = IHttpNegotiate_BeginningTransaction(http_negotiate, urls[test_protocol],
                                                   NULL, 0, &additional_headers);
        CHECK_CALLED(BeginningTransaction);
        IHttpNegotiate_Release(http_negotiate);
        ok(hres == S_OK, "BeginningTransction failed: %08x\n", hres);
        ok(additional_headers == NULL, "additional_headers=%p\n", additional_headers);

        SET_EXPECT(QueryInterface_IHttpNegotiate2);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate2,
                &IID_IHttpNegotiate2, (void**)&http_negotiate2);
        CHECK_CALLED(QueryInterface_IHttpNegotiate2);
        ok(hres == S_OK, "QueryService failed: %08x\n", hres);

        size = 512;
        SET_EXPECT(GetRootSecurityId);
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, sec_id, &size, 0);
        CHECK_CALLED(GetRootSecurityId);
        IHttpNegotiate2_Release(http_negotiate2);
        ok(hres == E_FAIL, "GetRootSecurityId failed: %08x, expected E_FAIL\n", hres);
        ok(size == 13, "size=%d\n", size);

        IServiceProvider_Release(service_provider);

        IInternetProtocolSink_AddRef(pOIProtSink);
        protocol_sink = pOIProtSink;
        CreateThread(NULL, 0, thread_proc, NULL, 0, &tid);

        return S_OK;
    }

    if(test_protocol == FILE_TEST) {
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_CACHEFILENAMEAVAILABLE, file_url+8);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08x\n", hres);

        if(bind_to_object)
            SET_EXPECT(Obj_OnProgress_MIMETYPEAVAILABLE);
        else
            SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE) failed: %08x\n", hres);
        if(bind_to_object)
            CHECK_CALLED(Obj_OnProgress_MIMETYPEAVAILABLE);
        else
            CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    }else {
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_MIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08x\n", hres);
    }

    if(test_protocol == ABOUT_TEST)
        bscf |= BSCF_DATAFULLYAVAILABLE;
    if(test_protocol == ITS_TEST)
        bscf = BSCF_FIRSTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE;

    SET_EXPECT(Read);
    if(bind_to_object) {
        if(test_protocol != FILE_TEST && test_protocol != MK_TEST)
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
        SET_EXPECT(OnStopBinding);
    }

    hres = IInternetProtocolSink_ReportData(pOIProtSink, bscf, 13, 13);
    ok(hres == S_OK, "ReportData failed: %08x\n", hres);

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
        if(test_protocol != FILE_TEST && test_protocol != MK_TEST)
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
        ok(hres == S_OK, "ReportProgress(BINDSTATUS_BEGINDOWNLOADDATA) failed: %08x\n", hres);
        CHECK_CALLED(Read);
    }

    SET_EXPECT(Terminate);
    hres = IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);
    ok(hres == S_OK, "ReportResult failed: %08x\n", hres);
    CHECK_CALLED(Terminate);

    return S_OK;
}

static HRESULT WINAPI Protocol_Continue(IInternetProtocol *iface,
        PROTOCOLDATA *pProtocolData)
{
    DWORD bscf = 0;
    HRESULT hres;

    CHECK_EXPECT(Continue);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    ok(pProtocolData != NULL, "pProtocolData == NULL\n");
    if(!pProtocolData)
        return S_OK;

    switch(prot_state) {
    case 1: {
        IServiceProvider *service_provider;
        IHttpNegotiate *http_negotiate;
        static WCHAR header[] = {'?',0};

        hres = IInternetProtocolSink_QueryInterface(protocol_sink, &IID_IServiceProvider,
                                                    (void**)&service_provider);
        ok(hres == S_OK, "Could not get IServiceProvicder\n");

        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
                                             &IID_IHttpNegotiate, (void**)&http_negotiate);
        ok(hres == S_OK, "Could not get IHttpNegotiate\n");

        SET_EXPECT(OnResponse);
        hres = IHttpNegotiate_OnResponse(http_negotiate, 200, header, NULL, NULL);
        CHECK_CALLED(OnResponse);
        IHttpNegotiate_Release(http_negotiate);
        ok(hres == S_OK, "OnResponse failed: %08x\n", hres);

        if(test_protocol == HTTPS_TEST) {
            hres = IInternetProtocolSink_ReportProgress(protocol_sink, BINDSTATUS_ACCEPTRANGES, NULL);
            ok(hres == S_OK, "ReportProgress(BINDSTATUS_ACCEPTRANGES) failed: %08x\n", hres);
        }

        hres = IInternetProtocolSink_ReportProgress(protocol_sink,
                BINDSTATUS_MIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08x\n", hres);

        if(tymed == TYMED_FILE) {
            hres = IInternetProtocolSink_ReportProgress(protocol_sink,
                    BINDSTATUS_CACHEFILENAMEAVAILABLE, cache_fileW);
            ok(hres == S_OK,
                   "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08x\n", hres);
        }

        bscf |= BSCF_FIRSTDATANOTIFICATION;
        break;
    }
    case 2:
    case 3:
        bscf = BSCF_INTERMEDIATEDATANOTIFICATION;
        break;
    }

    hres = IInternetProtocolSink_ReportData(protocol_sink, bscf, 100, 400);
    ok(hres == S_OK, "ReportData failed: %08x\n", hres);

    SET_EXPECT(Read);
    switch(prot_state) {
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
        }else {
            SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
            SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
            SET_EXPECT(LockRequest);
        }
        break;
    case 2:
        SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        break;
    case 3:
        SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    }
    if(!bind_to_object || prot_state >= 2)
        SET_EXPECT(OnDataAvailable);
    if(prot_state == 3)
        SET_EXPECT(OnStopBinding);

    return S_OK;
}

static HRESULT WINAPI Protocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    CHECK_EXPECT(Terminate);

    ok(dwOptions == 0, "dwOptions=%d, expected 0\n", dwOptions);

    if(protocol_sink) {
        IInternetProtocolSink_Release(protocol_sink);
        protocol_sink = NULL;
    }

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
    static const char data[] = "<HTML></HTML>";

    CHECK_EXPECT2(Read);

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
        HRESULT hres;

        static BOOL pending = TRUE;

        pending = !pending;

        switch(prot_state) {
        case 1:
        case 2:
            if(pending) {
                *pcbRead = 10;
                memset(pv, '?', 10);
                return E_PENDING;
            }else {
                memset(pv, '?', cb);
                *pcbRead = cb;
                read++;
                return S_OK;
            }
        case 3:
            prot_state++;

            *pcbRead = 0;

            hres = IInternetProtocolSink_ReportData(protocol_sink,
                    BSCF_LASTDATANOTIFICATION|BSCF_INTERMEDIATEDATANOTIFICATION, 2000, 2000);
            ok(hres == S_OK, "ReportData failed: %08x\n", hres);

            hres = IInternetProtocolSink_ReportResult(protocol_sink, S_OK, 0, NULL);
            ok(hres == S_OK, "ReportResult failed: %08x\n", hres);

            return S_FALSE;
        case 4:
            *pcbRead = 0;
            return S_FALSE;
        }
    }

    if(read) {
        *pcbRead = 0;
        return S_FALSE;
    }

    ok(pv != NULL, "pv == NULL\n");
    ok(cb != 0, "cb == 0\n");
    ok(pcbRead != NULL, "pcbRead == NULL\n");
    if(pcbRead) {
        ok(*pcbRead == 0, "*pcbRead=%d, expected 0\n", *pcbRead);
        read += *pcbRead = sizeof(data)-1;
    }
    if(pv)
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
    CHECK_EXPECT(BeginningTransaction);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    ok(!lstrcmpW(szURL, urls[test_protocol]), "szURL != urls[test_protocol]\n");
    ok(!dwReserved, "dwReserved=%d, expected 0\n", dwReserved);
    ok(pszAdditionalHeaders != NULL, "pszAdditionalHeaders == NULL\n");
    if(pszAdditionalHeaders)
        ok(*pszAdditionalHeaders == NULL, "*pszAdditionalHeaders != NULL\n");

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    CHECK_EXPECT(OnResponse);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    ok(dwResponseCode == 200, "dwResponseCode=%d, expected 200\n", dwResponseCode);
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

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    ok(!dwReserved, "dwReserved=%ld, expected 0\n", dwReserved);
    ok(pbSecurityId != NULL, "pbSecurityId == NULL\n");
    ok(pcbSecurityId != NULL, "pcbSecurityId == NULL\n");

    if(pbSecurityId == (void*)0xdeadbeef)
        return E_NOTIMPL;

    if(pcbSecurityId) {
        ok(*pcbSecurityId == 512, "*pcbSecurityId=%d, expected 512\n", *pcbSecurityId);
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

    ok(0, "unexpected service %s\n", debugstr_guid(guidService));
    return E_NOINTERFACE;
}

static IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };

static IBindStatusCallback objbsc;

static HRESULT WINAPI statusclb_QueryInterface(IBindStatusCallback *iface, REFIID riid, void **ppv)
{
    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        CHECK_EXPECT2(QueryInterface_IInternetProtocol);
        if(emulate_protocol) {
            *ppv = &Protocol;
            return S_OK;
        }else {
            return E_NOINTERFACE;
        }
    }
    else if (IsEqualGUID(&IID_IServiceProvider, riid))
    {
        CHECK_EXPECT2(QueryInterface_IServiceProvider);
        *ppv = &ServiceProvider;
        return S_OK;
    }
    else if (IsEqualGUID(&IID_IHttpNegotiate, riid))
    {
        CHECK_EXPECT(QueryInterface_IHttpNegotiate);
        *ppv = &HttpNegotiate;
        return S_OK;
    }
    else if (IsEqualGUID(&IID_IHttpNegotiate2, riid))
    {
        CHECK_EXPECT(QueryInterface_IHttpNegotiate2);
        *ppv = &HttpNegotiate;
        return S_OK;
    }
    else if (IsEqualGUID(&IID_IAuthenticate, riid))
    {
        CHECK_EXPECT(QueryInterface_IAuthenticate);
        return E_NOINTERFACE;
    }
    else if(IsEqualGUID(&IID_IBindStatusCallback, riid))
    {
        CHECK_EXPECT2(QueryInterface_IBindStatusCallback);
        *ppv = iface;
        return S_OK;
    }
    else if(IsEqualGUID(&IID_IBindStatusCallbackHolder, riid))
    {
        CHECK_EXPECT2(QueryInterface_IBindStatusCallbackHolder);
        return E_NOINTERFACE;
    }
    else if(IsEqualGUID(&IID_IInternetBindInfo, riid))
    {
        /* TODO */
        CHECK_EXPECT2(QueryInterface_IInternetBindInfo);
    }
    else
    {
        ok(0, "unexpected interface %s\n", debugstr_guid(riid));
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI statusclb_AddRef(IBindStatusCallback *iface)
{
    return 2;
}

static ULONG WINAPI statusclb_Release(IBindStatusCallback *iface)
{
    return 1;
}

static HRESULT WINAPI statusclb_OnStartBinding(IBindStatusCallback *iface, DWORD dwReserved,
        IBinding *pib)
{
    IWinInetHttpInfo *http_info;
    HRESULT hres;
    IMoniker *mon;

    if(iface == &objbsc)
        CHECK_EXPECT(Obj_OnStartBinding);
    else
        CHECK_EXPECT(OnStartBinding);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    ok(pib != NULL, "pib should not be NULL\n");
    ok(dwReserved == 0xff, "dwReserved=%x\n", dwReserved);

    if(pib == (void*)0xdeadbeef)
        return S_OK;

    current_binding = pib;

    hres = IBinding_QueryInterface(pib, &IID_IMoniker, (void**)&mon);
    ok(hres == E_NOINTERFACE, "IBinding should not have IMoniker interface\n");
    if(SUCCEEDED(hres))
        IMoniker_Release(mon);

    hres = IBinding_QueryInterface(pib, &IID_IWinInetHttpInfo, (void**)&http_info);
    ok(hres == E_NOINTERFACE, "Could not get IID_IWinInetHttpInfo: %08x\n", hres);

    return S_OK;
}

static HRESULT WINAPI statusclb_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    switch(ulStatusCode) {
    case BINDSTATUS_FINDINGRESOURCE:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_FINDINGRESOURCE);
        else if(test_protocol == FTP_TEST)
            todo_wine CHECK_EXPECT(OnProgress_FINDINGRESOURCE);
        else
            CHECK_EXPECT(OnProgress_FINDINGRESOURCE);
        if((bindf & BINDF_ASYNCHRONOUS) && emulate_protocol)
            SetEvent(complete_event);
        break;
    case BINDSTATUS_CONNECTING:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_CONNECTING);
        else if(test_protocol == FTP_TEST)
            todo_wine CHECK_EXPECT(OnProgress_CONNECTING);
        else
            CHECK_EXPECT(OnProgress_CONNECTING);
        if((bindf & BINDF_ASYNCHRONOUS) && emulate_protocol)
            SetEvent(complete_event);
        break;
    case BINDSTATUS_SENDINGREQUEST:
        if(iface == &objbsc)
            CHECK_EXPECT(Obj_OnProgress_SENDINGREQUEST);
        else if(test_protocol == FTP_TEST)
            CHECK_EXPECT2(OnProgress_SENDINGREQUEST);
        else
            CHECK_EXPECT(OnProgress_SENDINGREQUEST);
        if((bindf & BINDF_ASYNCHRONOUS) && emulate_protocol)
            SetEvent(complete_event);
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
                ok(!lstrcmpW(szStatusText, urls[test_protocol]), "wrong szStatusText %s\n", debugstr_w(szStatusText));
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
                ok(!lstrcmpW(szStatusText, urls[test_protocol]), "wrong szStatusText %s\n", debugstr_w(szStatusText));
            }
        }
        ok(download_state == DOWNLOADING, "Download state was %d, expected DOWNLOADING\n",
           download_state);
        download_state = END_DOWNLOAD;
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        if(test_protocol != HTTP_TEST && test_protocol != HTTPS_TEST) {
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
            ok(!lstrcmpW(file_url+8, szStatusText), "wrong szStatusText %s\n", debugstr_w(szStatusText));
        break;
    case BINDSTATUS_CLASSIDAVAILABLE:
    {
        CLSID clsid;
        HRESULT hr;
        if(iface != &objbsc)
            ok(0, "unexpected call\n");
        else if(1||emulate_protocol)
            CHECK_EXPECT(Obj_OnProgress_CLASSIDAVAILABLE);
        else
            todo_wine CHECK_EXPECT(Obj_OnProgress_CLASSIDAVAILABLE);
        hr = CLSIDFromString((LPOLESTR)szStatusText, &clsid);
        ok(hr == S_OK, "CLSIDFromString failed with error 0x%08x\n", hr);
        ok(IsEqualCLSID(&clsid, &CLSID_HTMLDocument),
            "Expected clsid to be CLSID_HTMLDocument instead of %s\n", debugstr_guid(&clsid));
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
        trace("BINDSTATUS_PROXYDETECTING\n");
        break;
    case BINDSTATUS_COOKIE_SENT:
        trace("BINDSTATUS_COOKIE_SENT\n");
        break;
    default:
        ok(0, "unexpected code %d\n", ulStatusCode);
    };

    if(current_binding) {
        IWinInetHttpInfo *http_info;
        HRESULT hres;

        hres = IBinding_QueryInterface(current_binding, &IID_IWinInetHttpInfo, (void**)&http_info);
        if(!emulate_protocol && test_protocol != FILE_TEST && is_urlmon_protocol(test_protocol))
            ok(hres == S_OK, "Could not get IWinInetHttpInfo iface: %08x\n", hres);
        else
            ok(hres == E_NOINTERFACE,
               "QueryInterface(IID_IWinInetHttpInfo) returned: %08x, expected E_NOINTERFACE\n", hres);
        if(SUCCEEDED(hres))
            IWinInetHttpInfo_Release(http_info);
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    if(iface == &objbsc) {
        CHECK_EXPECT(Obj_OnStopBinding);
        stopped_obj_binding = TRUE;
    }else {
        CHECK_EXPECT(OnStopBinding);
        stopped_binding = TRUE;
    }

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    /* ignore DNS failure */
    if (hresult == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
        return S_OK;

    if(filedwl_api)
        ok(SUCCEEDED(hresult), "binding failed: %08x\n", hresult);
    else
        ok(hresult == binding_hres, "binding failed: %08x, expected %08x\n", hresult, binding_hres);
    ok(szError == NULL, "szError should be NULL\n");

    if((test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) && emulate_protocol) {
        SetEvent(complete_event);
        if(iface != &objbsc)
            WaitForSingleObject(complete_event2, INFINITE);
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DWORD cbSize;

    if(iface == &objbsc)
        CHECK_EXPECT(Obj_GetBindInfo);
    else
        CHECK_EXPECT(GetBindInfo);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    *grfBINDF = bindf;
    cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize);
    pbindinfo->cbSize = cbSize;

    return S_OK;
}

static HRESULT WINAPI statusclb_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    HRESULT hres;
    DWORD readed;
    BYTE buf[512];
    CHAR clipfmt[512];

    if(iface == &objbsc)
        ok(0, "unexpected call\n");

    CHECK_EXPECT2(OnDataAvailable);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    ok(download_state == DOWNLOADING || download_state == END_DOWNLOAD,
       "Download state was %d, expected DOWNLOADING or END_DOWNLOAD\n",
       download_state);
    data_available = TRUE;

    ok(pformatetc != NULL, "pformatetx == NULL\n");
    if(pformatetc) {
        if (mime_type[0]) {
            clipfmt[0] = 0;
            ok(GetClipboardFormatName(pformatetc->cfFormat, clipfmt, sizeof(clipfmt)-1),
               "GetClipboardFormatName failed, error %d\n", GetLastError());
            ok(!lstrcmp(clipfmt, mime_type), "clipformat %x != mime_type, \"%s\" != \"%s\"\n",
               pformatetc->cfFormat, clipfmt, mime_type);
        } else {
            ok(pformatetc->cfFormat == 0, "clipformat=%x\n", pformatetc->cfFormat);
        }
        ok(pformatetc->ptd == NULL, "ptd = %p\n", pformatetc->ptd);
        ok(pformatetc->dwAspect == 1, "dwAspect=%u\n", pformatetc->dwAspect);
        ok(pformatetc->lindex == -1, "lindex=%d\n", pformatetc->lindex);
        ok(pformatetc->tymed == tymed, "tymed=%u, expected %u\n", pformatetc->tymed, tymed);
    }

    ok(pstgmed != NULL, "stgmeg == NULL\n");
    ok(pstgmed->tymed == tymed, "tymed=%u, expected %u\n", pstgmed->tymed, tymed);
    ok(pstgmed->pUnkForRelease != NULL, "pUnkForRelease == NULL\n");

    switch(pstgmed->tymed) {
    case TYMED_ISTREAM:
        if(grfBSCF & BSCF_FIRSTDATANOTIFICATION) {
            hres = IStream_Write(U(*pstgmed).pstm, buf, 10, NULL);
            ok(hres == STG_E_ACCESSDENIED,
               "Write failed: %08x, expected STG_E_ACCESSDENIED\n", hres);

            hres = IStream_Commit(U(*pstgmed).pstm, 0);
            ok(hres == E_NOTIMPL, "Commit failed: %08x, expected E_NOTIMPL\n", hres);

            hres = IStream_Revert(U(*pstgmed).pstm);
            ok(hres == E_NOTIMPL, "Revert failed: %08x, expected E_NOTIMPL\n", hres);
        }

        ok(U(*pstgmed).pstm != NULL, "U(*pstgmed).pstm == NULL\n");
        do hres = IStream_Read(U(*pstgmed).pstm, buf, 512, &readed);
        while(hres == S_OK);
        ok(hres == S_FALSE || hres == E_PENDING, "IStream_Read returned %08x\n", hres);
        break;

    case TYMED_FILE:
        if(test_protocol == FILE_TEST)
            ok(!lstrcmpW(pstgmed->u.lpszFileName, INDEX_HTML+7),
               "unexpected file name %s\n", debugstr_w(pstgmed->u.lpszFileName));
        else if(emulate_protocol)
            ok(!lstrcmpW(pstgmed->u.lpszFileName, cache_fileW),
               "unexpected file name %s\n", debugstr_w(pstgmed->u.lpszFileName));
        else
            ok(pstgmed->u.lpszFileName != NULL, "lpszFileName == NULL\n");
    }

    if((test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
       && emulate_protocol && prot_state < 4 && (!bind_to_object || prot_state > 1))
        SetEvent(complete_event);

    return S_OK;
}

static HRESULT WINAPI statusclb_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    CHECK_EXPECT(OnObjectAvailable);

    if(iface != &objbsc)
        ok(0, "unexpected call\n");

    ok(IsEqualGUID(&IID_IUnknown, riid), "riid = %s\n", debugstr_guid(riid));
    ok(punk != NULL, "punk == NULL\n");

    return S_OK;
}

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
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
    statusclb_OnObjectAvailable
};

static IBindStatusCallback bsc = { &BindStatusCallbackVtbl };
static IBindStatusCallback objbsc = { &BindStatusCallbackVtbl };

static HRESULT WINAPI MonikerProp_QueryInterface(IMonikerProp *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected riid %s\n", debugstr_guid(riid));
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
        ok(!lstrcmpW(val, wszTextHtml), "val = %s\n", debugstr_w(val));
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

    ok(0, "unexpected riid %s\n", debugstr_guid(riid));
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
    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
        ok(!fFullyAvailable, "fFulyAvailable = %x\n", fFullyAvailable);
    else
        ok(fFullyAvailable, "fFulyAvailable = %x\n", fFullyAvailable);
    ok(pimkName != NULL, "pimkName == NULL\n");
    ok(pibc != NULL, "pibc == NULL\n");
    ok(grfMode == 0x12, "grfMode = %x\n", grfMode);

    hres = IBindCtx_GetObjectParam(pibc, cbinding_contextW, &unk);
    ok(hres == S_OK, "GetObjectParam(CBinding Context) failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        IBinding *binding;

        hres = IUnknown_QueryInterface(unk, &IID_IBinding, (void**)&binding);
        ok(hres == S_OK, "Could not get IBinding: %08x\n", hres);

        IBinding_Release(binding);
        IUnknown_Release(unk);
    }

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = RegisterBindStatusCallback(pibc, &bsc, NULL, 0);
    ok(hres == S_OK, "RegisterBindStatusCallback failed: %08x\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(OnStartBinding);
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
    ok(hres == S_OK, "Load failed: %08x\n", hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(OnStartBinding);
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

    ok(0, "unexpected riid %s\n", debugstr_guid(riid));
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
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", debugstr_guid(riid));
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

static void test_CreateAsyncBindCtx(void)
{
    IBindCtx *bctx = (IBindCtx*)0x0ff00ff0;
    IUnknown *unk;
    HRESULT hres;
    ULONG ref;
    BIND_OPTS bindopts;

    hres = CreateAsyncBindCtx(0, NULL, NULL, &bctx);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed. expected: E_INVALIDARG, got: %08x\n", hres);
    ok(bctx == (IBindCtx*)0x0ff00ff0, "bctx should not be changed\n");

    hres = CreateAsyncBindCtx(0, NULL, NULL, NULL);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed. expected: E_INVALIDARG, got: %08x\n", hres);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, &bsc, NULL, &bctx);
    ok(hres == S_OK, "CreateAsyncBindCtx failed: %08x\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    bindopts.cbStruct = sizeof(bindopts);
    hres = IBindCtx_GetBindOptions(bctx, &bindopts);
    ok(SUCCEEDED(hres), "IBindCtx_GetBindOptions failed: %08x\n", hres);
    ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08x, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
    ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08x, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
    ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08x, expected: 0\n", bindopts.dwTickCountDeadline);

    hres = IBindCtx_QueryInterface(bctx, &IID_IAsyncBindCtx, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IAsyncBindCtx) failed: %08x, expected E_NOINTERFACE\n", hres);
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

    hres = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, NULL, 0);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed: %08x, expected E_INVALIDARG\n", hres);

    hres = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        bindopts.cbStruct = sizeof(bindopts);
        hres = IBindCtx_GetBindOptions(bctx, &bindopts);
        ok(SUCCEEDED(hres), "IBindCtx_GetBindOptions failed: %08x\n", hres);
        ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08x, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
        ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08x, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
        ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08x, expected: 0\n", bindopts.dwTickCountDeadline);

        IBindCtx_Release(bctx);
    }

    CreateBindCtx(0, &bctx_arg);
    hres = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        bindopts.cbStruct = sizeof(bindopts);
        hres = IBindCtx_GetBindOptions(bctx, &bindopts);
        ok(SUCCEEDED(hres), "IBindCtx_GetBindOptions failed: %08x\n", hres);
        ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08x, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
        ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08x, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
        ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08x, expected: 0\n", bindopts.dwTickCountDeadline);

        IBindCtx_Release(bctx);
    }

    IBindCtx_Release(bctx_arg);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtxEx(NULL, 0, &bsc, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    hres = IBindCtx_QueryInterface(bctx, &IID_IAsyncBindCtx, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IAsyncBindCtx) failed: %08x\n", hres);
    if(SUCCEEDED(hres))
        IUnknown_Release(unk);

    IBindCtx_Release(bctx);

    hres = CreateBindCtx(0, &bctx2);
    ok(hres == S_OK, "CreateBindCtx failed: %08x\n", hres);

    hres = CreateAsyncBindCtxEx(bctx2, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);

    hres = IBindCtx_RegisterObjectParam(bctx2, testW, (IUnknown*)&Protocol);
    ok(hres == S_OK, "RegisterObjectParam failed: %08x\n", hres);

    hres = IBindCtx_GetObjectParam(bctx, testW, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08x\n", hres);
    ok(unk == (IUnknown*)&Protocol, "unexpected unk %p\n", unk);

    IBindCtx_Release(bctx);
    IBindCtx_Release(bctx2);
}

static BOOL test_bscholder(IBindStatusCallback *holder)
{
    IServiceProvider *serv_prov;
    IHttpNegotiate *http_negotiate, *http_negotiate_serv;
    IHttpNegotiate2 *http_negotiate2, *http_negotiate2_serv;
    IAuthenticate *authenticate, *authenticate_serv;
    IInternetProtocol *protocol;
    BINDINFO bindinfo = {sizeof(bindinfo)};
    BOOL ret = TRUE;
    LPWSTR wstr;
    DWORD dw;
    HRESULT hres;

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IServiceProvider, (void**)&serv_prov);
    ok(hres == S_OK, "Could not get IServiceProvider interface: %08x\n", hres);

    dw = 0xdeadbeef;
    SET_EXPECT(GetBindInfo);
    hres = IBindStatusCallback_GetBindInfo(holder, &dw, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08x\n", hres);
    CHECK_CALLED(GetBindInfo);

    SET_EXPECT(OnStartBinding);
    hres = IBindStatusCallback_OnStartBinding(holder, 0, (void*)0xdeadbeef);
    ok(hres == S_OK, "OnStartBinding failed: %08x\n", hres);
    CHECK_CALLED(OnStartBinding);

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IHttpNegotiate, (void**)&http_negotiate);
    ok(hres == S_OK, "Could not get IHttpNegotiate interface: %08x\n", hres);

    wstr = (void*)0xdeadbeef;
    hres = IHttpNegotiate_BeginningTransaction(http_negotiate, urls[test_protocol], (void*)0xdeadbeef, 0xff, &wstr);
    ok(hres == S_OK, "BeginningTransaction failed: %08x\n", hres);
    ok(wstr == NULL, "wstr = %p\n", wstr);

    SET_EXPECT(QueryInterface_IHttpNegotiate);
    hres = IServiceProvider_QueryService(serv_prov, &IID_IHttpNegotiate, &IID_IHttpNegotiate,
                                         (void**)&http_negotiate_serv);
    ok(hres == S_OK, "Could not get IHttpNegotiate service: %08x\n", hres);
    CHECK_CALLED(QueryInterface_IHttpNegotiate);

    ok(http_negotiate == http_negotiate_serv, "http_negotiate != http_negotiate_serv\n");

    wstr = (void*)0xdeadbeef;
    SET_EXPECT(BeginningTransaction);
    hres = IHttpNegotiate_BeginningTransaction(http_negotiate_serv, urls[test_protocol], emptyW, 0, &wstr);
    CHECK_CALLED(BeginningTransaction);
    ok(hres == S_OK, "BeginningTransaction failed: %08x\n", hres);
    ok(wstr == NULL, "wstr = %p\n", wstr);

    IHttpNegotiate_Release(http_negotiate_serv);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IHttpNegotiate, &IID_IHttpNegotiate,
                                         (void**)&http_negotiate_serv);
    ok(hres == S_OK, "Could not get IHttpNegotiate service: %08x\n", hres);
    ok(http_negotiate == http_negotiate_serv, "http_negotiate != http_negotiate_serv\n");
    IHttpNegotiate_Release(http_negotiate_serv);

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IHttpNegotiate2, (void**)&http_negotiate2);
    if(SUCCEEDED(hres)) {
        have_IHttpNegotiate2 = TRUE;
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, (void*)0xdeadbeef, (void*)0xdeadbeef, 0);
        ok(hres == E_FAIL, "GetRootSecurityId failed: %08x\n", hres);

        SET_EXPECT(QueryInterface_IHttpNegotiate2);
        hres = IServiceProvider_QueryService(serv_prov, &IID_IHttpNegotiate2, &IID_IHttpNegotiate2,
                                             (void**)&http_negotiate2_serv);
        ok(hres == S_OK, "Could not get IHttpNegotiate2 service: %08x\n", hres);
        CHECK_CALLED(QueryInterface_IHttpNegotiate2);
        ok(http_negotiate2 == http_negotiate2_serv, "http_negotiate != http_negotiate_serv\n");

        SET_EXPECT(GetRootSecurityId);
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, (void*)0xdeadbeef, (void*)0xdeadbeef, 0);
        ok(hres == E_NOTIMPL, "GetRootSecurityId failed: %08x\n", hres);
        CHECK_CALLED(GetRootSecurityId);

        IHttpNegotiate_Release(http_negotiate2_serv);
        IHttpNegotiate_Release(http_negotiate2);
    }else {
        skip("Could not get IHttpNegotiate2\n");
        ret = FALSE;
    }

    SET_EXPECT(OnProgress_FINDINGRESOURCE);
    hres = IBindStatusCallback_OnProgress(holder, 0, 0, BINDSTATUS_FINDINGRESOURCE, NULL);
    ok(hres == S_OK, "OnProgress failed: %08x\n", hres);
    CHECK_CALLED(OnProgress_FINDINGRESOURCE);

    SET_EXPECT(OnResponse);
    wstr = (void*)0xdeadbeef;
    hres = IHttpNegotiate_OnResponse(http_negotiate, 200, emptyW, NULL, NULL);
    ok(hres == S_OK, "OnResponse failed: %08x\n", hres);
    CHECK_CALLED(OnResponse);

    IHttpNegotiate_Release(http_negotiate);

    hres = IBindStatusCallback_QueryInterface(holder, &IID_IAuthenticate, (void**)&authenticate);
    ok(hres == S_OK, "Could not get IAuthenticate interface: %08x\n", hres);

    SET_EXPECT(QueryInterface_IAuthenticate);
    SET_EXPECT(QueryService_IAuthenticate);
    hres = IServiceProvider_QueryService(serv_prov, &IID_IAuthenticate, &IID_IAuthenticate,
                                         (void**)&authenticate_serv);
    ok(hres == S_OK, "Could not get IAuthenticate service: %08x\n", hres);
    CHECK_CALLED(QueryInterface_IAuthenticate);
    CHECK_CALLED(QueryService_IAuthenticate);
    ok(authenticate == authenticate_serv, "authenticate != authenticate_serv\n");
    IAuthenticate_Release(authenticate_serv);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IAuthenticate, &IID_IAuthenticate,
                                         (void**)&authenticate_serv);
    ok(hres == S_OK, "Could not get IAuthenticate service: %08x\n", hres);
    ok(authenticate == authenticate_serv, "authenticate != authenticate_serv\n");

    IAuthenticate_Release(authenticate);
    IAuthenticate_Release(authenticate_serv);

    SET_EXPECT(OnStopBinding);
    hres = IBindStatusCallback_OnStopBinding(holder, S_OK, NULL);
    ok(hres == S_OK, "OnStopBinding failed: %08x\n", hres);
    CHECK_CALLED(OnStopBinding);

    SET_EXPECT(QueryInterface_IInternetProtocol);
    SET_EXPECT(QueryService_IInternetProtocol);
    hres = IServiceProvider_QueryService(serv_prov, &IID_IInternetProtocol, &IID_IInternetProtocol,
                                         (void**)&protocol);
    ok(hres == E_NOINTERFACE, "QueryService(IInternetProtocol) failed: %08x\n", hres);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    CHECK_CALLED(QueryService_IInternetProtocol);

    IServiceProvider_Release(serv_prov);
    return ret;
}

static BOOL test_RegisterBindStatusCallback(void)
{
    IBindStatusCallback *prevbsc, *clb;
    IBindCtx *bindctx;
    BOOL ret = TRUE;
    IUnknown *unk;
    HRESULT hres;

    hres = CreateBindCtx(0, &bindctx);
    ok(hres == S_OK, "BindCtx failed: %08x\n", hres);

    SET_EXPECT(QueryInterface_IServiceProvider);

    hres = IBindCtx_RegisterObjectParam(bindctx, BSCBHolder, (IUnknown*)&bsc);
    ok(hres == S_OK, "RegisterObjectParam failed: %08x\n", hres);

    SET_EXPECT(QueryInterface_IBindStatusCallback);
    SET_EXPECT(QueryInterface_IBindStatusCallbackHolder);
    prevbsc = (void*)0xdeadbeef;
    hres = RegisterBindStatusCallback(bindctx, &bsc, &prevbsc, 0);
    ok(hres == S_OK, "RegisterBindStatusCallback failed: %08x\n", hres);
    ok(prevbsc == &bsc, "prevbsc=%p\n", prevbsc);
    CHECK_CALLED(QueryInterface_IBindStatusCallback);
    CHECK_CALLED(QueryInterface_IBindStatusCallbackHolder);

    CHECK_CALLED(QueryInterface_IServiceProvider);

    hres = IBindCtx_GetObjectParam(bindctx, BSCBHolder, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IBindStatusCallback, (void**)&clb);
    IUnknown_Release(unk);
    ok(hres == S_OK, "QueryInterface(IID_IBindStatusCallback) failed: %08x\n", hres);
    ok(clb != &bsc, "bsc == clb\n");

    if(!test_bscholder(clb))
        ret = FALSE;

    IBindStatusCallback_Release(clb);

    hres = RevokeBindStatusCallback(bindctx, &bsc);
    ok(hres == S_OK, "RevokeBindStatusCallback failed: %08x\n", hres);

    unk = (void*)0xdeadbeef;
    hres = IBindCtx_GetObjectParam(bindctx, BSCBHolder, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08x\n", hres);
    ok(unk == NULL, "unk != NULL\n");

    if(unk)
        IUnknown_Release(unk);

    hres = RevokeBindStatusCallback(bindctx, (void*)0xdeadbeef);
    ok(hres == S_OK, "RevokeBindStatusCallback failed: %08x\n", hres);

    hres = RevokeBindStatusCallback(NULL, (void*)0xdeadbeef);
    ok(hres == E_INVALIDARG, "RevokeBindStatusCallback failed: %08x\n", hres);

    hres = RevokeBindStatusCallback(bindctx, NULL);
    ok(hres == E_INVALIDARG, "RevokeBindStatusCallback failed: %08x\n", hres);

    IBindCtx_Release(bindctx);
    return ret;
}

#define BINDTEST_EMULATE     1
#define BINDTEST_TOOBJECT    2
#define BINDTEST_FILEDWLAPI  4

static void init_bind_test(int protocol, DWORD flags, DWORD t)
{
    test_protocol = protocol;
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
}

static void test_BindToStorage(int protocol, BOOL emul, DWORD t)
{
    IMoniker *mon;
    HRESULT hres;
    LPOLESTR display_name;
    IBindCtx *bctx;
    MSG msg;
    IBindStatusCallback *previousclb;
    IUnknown *unk = (IUnknown*)0x00ff00ff;
    IBinding *bind;

    init_bind_test(protocol, emul ? BINDTEST_EMULATE : 0, t);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, &bsc, NULL, &bctx);
    ok(hres == S_OK, "CreateAsyncBindCtx failed: %08x\n\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    if(FAILED(hres))
        return;

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = RegisterBindStatusCallback(bctx, &bsc, &previousclb, 0);
    ok(hres == S_OK, "RegisterBindStatusCallback failed: %08x\n", hres);
    ok(previousclb == &bsc, "previousclb(%p) != sclb(%p)\n", previousclb, &bsc);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    if(previousclb)
        IBindStatusCallback_Release(previousclb);

    hres = CreateURLMoniker(NULL, test_protocol == FILE_TEST ? file_url : urls[test_protocol], &mon);
    ok(SUCCEEDED(hres), "failed to create moniker: %08x\n", hres);
    if(FAILED(hres)) {
        IBindCtx_Release(bctx);
        return;
    }

    hres = IMoniker_QueryInterface(mon, &IID_IBinding, (void**)&bind);
    ok(hres == E_NOINTERFACE, "IMoniker should not have IBinding interface\n");
    if(SUCCEEDED(hres))
        IBinding_Release(bind);

    hres = IMoniker_GetDisplayName(mon, bctx, NULL, &display_name);
    ok(hres == S_OK, "GetDisplayName failed %08x\n", hres);
    ok(!lstrcmpW(display_name, urls[test_protocol]),
       "GetDisplayName got wrong name %s\n", debugstr_w(display_name));
    CoTaskMemFree(display_name);

    if(tymed == TYMED_FILE && (test_protocol == ABOUT_TEST || test_protocol == ITS_TEST))
        binding_hres = INET_E_DATA_NOT_AVAILABLE;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    if(!emulate_protocol)
        SET_EXPECT(QueryService_IInternetProtocol);
    SET_EXPECT(OnStartBinding);
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            SET_EXPECT(SetPriority);
        SET_EXPECT(Start);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(Terminate);
        if(tymed != TYMED_FILE || (test_protocol != ABOUT_TEST && test_protocol != ITS_TEST))
            SET_EXPECT(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            SET_EXPECT(QueryInterface_IInternetBindInfo);
            SET_EXPECT(QueryService_IInternetBindInfo);
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(BeginningTransaction);
            SET_EXPECT(QueryInterface_IHttpNegotiate2);
            SET_EXPECT(GetRootSecurityId);
            SET_EXPECT(OnProgress_FINDINGRESOURCE);
            SET_EXPECT(OnProgress_CONNECTING);
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FTP_TEST
           || test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(OnResponse);
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FTP_TEST)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
        if(tymed != TYMED_FILE || test_protocol != ABOUT_TEST)
            SET_EXPECT(OnDataAvailable);
        SET_EXPECT(OnStopBinding);
    }

    hres = IMoniker_BindToStorage(mon, bctx, NULL, tymed == TYMED_ISTREAM ? &IID_IStream : &IID_IUnknown, (void**)&unk);
    if ((test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
        && hres == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
    {
        skip("Network unreachable, skipping tests\n");
        return;
    }

    if(((bindf & BINDF_ASYNCHRONOUS) && !data_available)
       || (tymed == TYMED_FILE && test_protocol == FILE_TEST)) {
        ok(hres == MK_S_ASYNCHRONOUS, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else if(tymed == TYMED_FILE && test_protocol == ABOUT_TEST) {
        ok(hres == INET_E_DATA_NOT_AVAILABLE,
           "IMoniker_BindToStorage failed: %08x, expected INET_E_DATA_NOT_AVAILABLE\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk != NULL, "unk == NULL\n");
    }
    if(unk)
        IUnknown_Release(unk);

    if(FAILED(hres))
        return;

    while((bindf & BINDF_ASYNCHRONOUS) &&
          !stopped_binding && GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    if(!emulate_protocol)
        CHECK_CALLED(QueryService_IInternetProtocol);
    CHECK_CALLED(OnStartBinding);
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            CHECK_CALLED(SetPriority);
        CHECK_CALLED(Start);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            if(tymed == TYMED_FILE)
                CLEAR_CALLED(Read);
            CHECK_CALLED(Terminate);
        }
        if(tymed != TYMED_FILE || (test_protocol != ABOUT_TEST && test_protocol != ITS_TEST))
            CHECK_CALLED(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST) {
            CLEAR_CALLED(QueryInterface_IInternetBindInfo);
            CLEAR_CALLED(QueryService_IInternetBindInfo);
            CHECK_CALLED(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(BeginningTransaction);
            if (have_IHttpNegotiate2)
            {
                CHECK_CALLED(QueryInterface_IHttpNegotiate2);
                CHECK_CALLED(GetRootSecurityId);
            }
            if(http_is_first || test_protocol == HTTPS_TEST) {
                CHECK_CALLED(OnProgress_FINDINGRESOURCE);
                CHECK_CALLED(OnProgress_CONNECTING);
            }else todo_wine {
                CHECK_NOT_CALLED(OnProgress_FINDINGRESOURCE);
                /* IE7 does call this */
                CLEAR_CALLED(OnProgress_CONNECTING);
            }
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
        else if(test_protocol == FTP_TEST)
            todo_wine CHECK_CALLED(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CHECK_CALLED(OnResponse);
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FTP_TEST)
            CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
        CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
        if(tymed != TYMED_FILE || test_protocol != ABOUT_TEST)
            CHECK_CALLED(OnDataAvailable);
        CHECK_CALLED(OnStopBinding);
    }

    ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");
    ok(IBindCtx_Release(bctx) == 0, "bctx should be destroyed here\n");

    if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
        http_is_first = FALSE;
}

static void test_BindToObject(int protocol, BOOL emul)
{
    IMoniker *mon;
    HRESULT hres;
    LPOLESTR display_name;
    IBindCtx *bctx;
    DWORD regid;
    MSG msg;
    IUnknown *unk = (IUnknown*)0x00ff00ff;
    IBinding *bind;

    init_bind_test(protocol, BINDTEST_TOOBJECT | (emul ? BINDTEST_EMULATE : 0), TYMED_ISTREAM);

    if(emul)
        CoRegisterClassObject(&CLSID_HTMLDocument, (IUnknown *)&mime_cf,
                              CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, &objbsc, NULL, &bctx);
    ok(SUCCEEDED(hres), "CreateAsyncBindCtx failed: %08x\n\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    if(FAILED(hres))
        return;

    hres = CreateURLMoniker(NULL, test_protocol == FILE_TEST ? file_url : urls[test_protocol], &mon);
    ok(SUCCEEDED(hres), "failed to create moniker: %08x\n", hres);
    if(FAILED(hres)) {
        IBindCtx_Release(bctx);
        return;
    }

    hres = IMoniker_QueryInterface(mon, &IID_IBinding, (void**)&bind);
    ok(hres == E_NOINTERFACE, "IMoniker should not have IBinding interface\n");
    if(SUCCEEDED(hres))
        IBinding_Release(bind);

    hres = IMoniker_GetDisplayName(mon, bctx, NULL, &display_name);
    ok(hres == S_OK, "GetDisplayName failed %08x\n", hres);
    ok(!lstrcmpW(display_name, urls[test_protocol]), "GetDisplayName got wrong name\n");

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
            SET_EXPECT(Obj_OnProgress_FINDINGRESOURCE);
            SET_EXPECT(Obj_OnProgress_CONNECTING);
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FILE_TEST)
            SET_EXPECT(Obj_OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(OnResponse);
        SET_EXPECT(Obj_OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(Obj_OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            SET_EXPECT(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(Obj_OnProgress_ENDDOWNLOADDATA);
        SET_EXPECT(Obj_OnProgress_CLASSIDAVAILABLE);
        SET_EXPECT(Obj_OnProgress_BEGINSYNCOPERATION);
        SET_EXPECT(Obj_OnProgress_ENDSYNCOPERATION);
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

    /* no point testing the calls if binding didn't even work */
    if (FAILED(hres)) return;

    if(bindf & BINDF_ASYNCHRONOUS) {
        ok(hres == MK_S_ASYNCHRONOUS, "IMoniker_BindToObject failed: %08x\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk != NULL, "unk == NULL\n");
        if(emul)
            ok(unk == (IUnknown*)&PersistMoniker, "unk != PersistMoniker\n");
    }
    if(unk)
        IUnknown_Release(unk);

    while((bindf & BINDF_ASYNCHRONOUS) &&
          !((!emul || stopped_binding) && stopped_obj_binding) && GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CHECK_CALLED(Obj_GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    if(!emulate_protocol)
        CHECK_CALLED(QueryService_IInternetProtocol);
    CHECK_CALLED(Obj_OnStartBinding);
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            CHECK_CALLED(SetPriority);
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
            }else todo_wine {
                CHECK_NOT_CALLED(Obj_OnProgress_FINDINGRESOURCE);
                /* IE7 does call this */
                CLEAR_CALLED(Obj_OnProgress_CONNECTING);
            }
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FILE_TEST) {
            if(urls[test_protocol] == SHORT_RESPONSE_URL)
                CLEAR_CALLED(Obj_OnProgress_SENDINGREQUEST);
            else
                CHECK_CALLED(Obj_OnProgress_SENDINGREQUEST);
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CHECK_CALLED(OnResponse);
        CHECK_CALLED(Obj_OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            CHECK_CALLED(Obj_OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
        CLEAR_CALLED(Obj_OnProgress_ENDDOWNLOADDATA);
        CHECK_CALLED(Obj_OnProgress_CLASSIDAVAILABLE);
        CHECK_CALLED(Obj_OnProgress_BEGINSYNCOPERATION);
        CHECK_CALLED(Obj_OnProgress_ENDSYNCOPERATION);
        CHECK_CALLED(OnObjectAvailable);
        CHECK_CALLED(Obj_OnStopBinding);
    }

    if(test_protocol != HTTP_TEST || test_protocol == HTTPS_TEST || emul || urls[test_protocol] == SHORT_RESPONSE_URL) {
        ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");
        ok(IBindCtx_Release(bctx) == 0, "bctx should be destroyed here\n");
    }else {
        todo_wine ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");

        if(bindf & BINDF_ASYNCHRONOUS)
            IBindCtx_Release(bctx);
        else
            todo_wine ok(IBindCtx_Release(bctx) == 0, "bctx should be destroyed here\n");
    }

    if(emul)
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
    if(!emulate_protocol) {
        SET_EXPECT(QueryInterface_IServiceProvider);
        SET_EXPECT(QueryService_IInternetProtocol);
    }
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
        }
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST || test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(OnResponse);
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_CACHEFILENAMEAVAILABLE);
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
        SET_EXPECT(OnStopBinding);
    }

    hres = URLDownloadToFileW(NULL, test_protocol == FILE_TEST ? file_url : urls[test_protocol], dwl_htmlW, 0, &bsc);
    ok(hres == S_OK, "URLDownloadToFile failed: %08x\n", hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    if(!emulate_protocol) {
        CHECK_CALLED(QueryInterface_IServiceProvider);
        CHECK_CALLED(QueryService_IInternetProtocol);
    }
    CHECK_CALLED(OnStartBinding);
    if(emulate_protocol) {
        if(is_urlmon_protocol(test_protocol))
            CHECK_CALLED(SetPriority);
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
        }
        if(test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
        else if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CLEAR_CALLED(OnProgress_SENDINGREQUEST); /* not called by IE7 */
        if(test_protocol == HTTP_TEST || test_protocol == HTTPS_TEST)
            CHECK_CALLED(OnResponse);
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
    ok(res, "DeleteFile failed: %u\n", GetLastError());

    if(prot != FILE_TEST || emul)
        return;

    hres = URLDownloadToFileW(NULL, urls[test_protocol], dwl_htmlW, 0, NULL);
    ok(hres == S_OK, "URLDownloadToFile failed: %08x\n", hres);

    res = DeleteFileA(dwl_htmlA);
    ok(res, "DeleteFile failed: %u\n", GetLastError());
}

static void set_file_url(char *path)
{
    CHAR file_urlA[INTERNET_MAX_URL_LENGTH];
    CHAR INDEX_HTMLA[MAX_PATH];

    lstrcpyA(file_urlA, "file:///");
    lstrcatA(file_urlA, path);
    MultiByteToWideChar(CP_ACP, 0, file_urlA, -1, file_url, INTERNET_MAX_URL_LENGTH);

    lstrcpyA(INDEX_HTMLA, "file://");
    lstrcatA(INDEX_HTMLA, path);
    MultiByteToWideChar(CP_ACP, 0, INDEX_HTMLA, -1, INDEX_HTML, MAX_PATH);
}

static void create_file(void)
{
    HANDLE file;
    DWORD size;
    CHAR path[MAX_PATH];

    static const char html_doc[] = "<HTML></HTML>";

    file = CreateFileA(wszIndexHtmlA, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, html_doc, sizeof(html_doc)-1, &size, NULL);
    CloseHandle(file);

    GetCurrentDirectoryA(MAX_PATH, path);
    lstrcatA(path, "\\");
    lstrcatA(path, wszIndexHtmlA);
    set_file_url(path);
}

static void test_ReportResult(HRESULT exhres)
{
    IMoniker *mon = NULL;
    IBindCtx *bctx = NULL;
    IUnknown *unk = (void*)0xdeadbeef;
    HRESULT hres;

    init_bind_test(ABOUT_TEST, BINDTEST_EMULATE, TYMED_ISTREAM);
    binding_hres = exhres;

    hres = CreateURLMoniker(NULL, ABOUT_BLANK, &mon);
    ok(hres == S_OK, "CreateURLMoniker failed: %08x\n", hres);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, &bsc, NULL, &bctx);
    ok(hres == S_OK, "CreateAsyncBindCtx failed: %08x\n\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    SET_EXPECT(OnStartBinding);
    if(is_urlmon_protocol(test_protocol))
        SET_EXPECT(SetPriority);
    SET_EXPECT(Start);

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
    if(SUCCEEDED(exhres))
        ok(hres == S_OK || hres == MK_S_ASYNCHRONOUS, "BindToStorage failed: %08x\n", hres);
    else
        ok(hres == exhres || hres == MK_S_ASYNCHRONOUS,
           "BindToStorage failed: %08x, expected %08x or MK_S_ASYNCHRONOUS\n", hres, exhres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(QueryInterface_IInternetProtocol);
    CHECK_CALLED(OnStartBinding);
    if(is_urlmon_protocol(test_protocol))
        CHECK_CALLED(SetPriority);
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

    hres = CreateURLMoniker(NULL, ABOUT_BLANK, &mon);
    ok(hres == S_OK, "CreateURLMoniker failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
    ok(hres == MK_E_SYNTAX || hres == INET_E_DATA_NOT_AVAILABLE,
       "hres=%08x, expected MK_E_SYNTAX or INET_E_DATA_NOT_AVAILABLE\n", hres);

    IBindCtx_Release(bctx);

    IMoniker_Release(mon);

    test_ReportResult(E_NOTIMPL);
    test_ReportResult(S_FALSE);
}

static void test_StdURLMoniker(void)
{
    IMoniker *mon, *async_mon;
    LPOLESTR display_name;
    HRESULT hres;

    hres = CoCreateInstance(&IID_IInternet, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IMoniker, (void**)&mon);
    ok(hres == S_OK, "Could not create IInternet instance: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IMoniker_QueryInterface(mon, &IID_IAsyncMoniker, (void**)&async_mon);
    ok(hres == S_OK, "Could not get IAsyncMoniker iface: %08x\n", hres);
    ok(mon == async_mon, "mon != async_mon\n");
    IMoniker_Release(async_mon);

    hres = IMoniker_GetDisplayName(mon, NULL, NULL, &display_name);
    ok(hres == E_OUTOFMEMORY, "GetDisplayName failed: %08x, expected E_OUTOFMEMORY\n", hres);

    IMoniker_Release(mon);
}

static void gecko_installer_workaround(BOOL disable)
{
    HKEY hkey;
    DWORD res;

    static BOOL has_url = FALSE;
    static char url[2048];

    if(!disable && !has_url)
        return;

    res = RegOpenKey(HKEY_CURRENT_USER, "Software\\Wine\\MSHTML", &hkey);
    if(res != ERROR_SUCCESS)
        return;

    if(disable) {
        DWORD type, size = sizeof(url);

        res = RegQueryValueEx(hkey, "GeckoUrl", NULL, &type, (PVOID)url, &size);
        if(res == ERROR_SUCCESS && type == REG_SZ)
            has_url = TRUE;

        RegDeleteValue(hkey, "GeckoUrl");
    }else {
        RegSetValueEx(hkey, "GeckoUrl", 0, REG_SZ, (PVOID)url, lstrlenA(url)+1);
    }

    RegCloseKey(hkey);
}

START_TEST(url)
{
    gecko_installer_workaround(TRUE);

    complete_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    complete_event2 = CreateEvent(NULL, FALSE, FALSE, NULL);
    thread_id = GetCurrentThreadId();
    create_file();

    test_create();
    test_CreateAsyncBindCtx();
    test_CreateAsyncBindCtxEx();

    if(test_RegisterBindStatusCallback()) {
        test_BindToStorage_fail();

        trace("synchronous http test (COM not initialised)...\n");
        test_BindToStorage(HTTP_TEST, FALSE, TYMED_ISTREAM);

        CoInitialize(NULL);

        trace("test StdURLMoniker...\n");
        test_StdURLMoniker();

        trace("synchronous http test...\n");
        test_BindToStorage(HTTP_TEST, FALSE, TYMED_ISTREAM);

        trace("synchronous http test (to object)...\n");
        test_BindToObject(HTTP_TEST, FALSE);

        trace("synchronous file test...\n");
        test_BindToStorage(FILE_TEST, FALSE, TYMED_ISTREAM);

        trace("synchronous file test (to object)...\n");
        test_BindToObject(FILE_TEST, FALSE);

        bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

        trace("http test...\n");
        test_BindToStorage(HTTP_TEST, FALSE, TYMED_ISTREAM);

        trace("http test (to file)...\n");
        test_BindToStorage(HTTP_TEST, FALSE, TYMED_FILE);

        trace("http test (to object)...\n");
        test_BindToObject(HTTP_TEST, FALSE);

        trace("http test (short response)...\n");
        http_is_first = TRUE;
        urls[HTTP_TEST] = SHORT_RESPONSE_URL;
        test_BindToStorage(HTTP_TEST, FALSE, TYMED_ISTREAM);

        trace("http test (short response, to object)...\n");
        test_BindToObject(HTTP_TEST, FALSE);

        trace("emulated http test...\n");
        test_BindToStorage(HTTP_TEST, TRUE, TYMED_ISTREAM);

        trace("emulated http test (to object)...\n");
        test_BindToObject(HTTP_TEST, TRUE);

        trace("emulated http test (to file)...\n");
        test_BindToStorage(HTTP_TEST, TRUE, TYMED_FILE);

        trace("asynchronous https test...\n");
        test_BindToStorage(HTTPS_TEST, FALSE, TYMED_ISTREAM);

        trace("emulated https test...\n");
        test_BindToStorage(HTTPS_TEST, TRUE, TYMED_ISTREAM);

        trace("about test...\n");
        test_BindToStorage(ABOUT_TEST, FALSE, TYMED_ISTREAM);

        trace("about test (to file)...\n");
        test_BindToStorage(ABOUT_TEST, FALSE, TYMED_FILE);

        trace("about test (to object)...\n");
        test_BindToObject(ABOUT_TEST, FALSE);

        trace("emulated about test...\n");
        test_BindToStorage(ABOUT_TEST, TRUE, TYMED_ISTREAM);

        trace("emulated about test (to file)...\n");
        test_BindToStorage(ABOUT_TEST, TRUE, TYMED_FILE);

        trace("emulated about test (to object)...\n");
        test_BindToObject(ABOUT_TEST, TRUE);

        trace("file test...\n");
        test_BindToStorage(FILE_TEST, FALSE, TYMED_ISTREAM);

        trace("file test (to file)...\n");
        test_BindToStorage(FILE_TEST, FALSE, TYMED_FILE);

        trace("file test (to object)...\n");
        test_BindToObject(FILE_TEST, FALSE);

        trace("emulated file test...\n");
        test_BindToStorage(FILE_TEST, TRUE, TYMED_ISTREAM);

        trace("emulated file test (to file)...\n");
        test_BindToStorage(FILE_TEST, TRUE, TYMED_FILE);

        trace("emulated file test (to object)...\n");
        test_BindToObject(FILE_TEST, TRUE);

        trace("emulated its test...\n");
        test_BindToStorage(ITS_TEST, TRUE, TYMED_ISTREAM);

        trace("emulated its test (to file)...\n");
        test_BindToStorage(ITS_TEST, TRUE, TYMED_FILE);

        trace("emulated mk test...\n");
        test_BindToStorage(MK_TEST, TRUE, TYMED_ISTREAM);

        trace("test URLDownloadToFile for file protocol...\n");
        test_URLDownloadToFile(FILE_TEST, FALSE);

        trace("test URLDownloadToFile for emulated file protocol...\n");
        test_URLDownloadToFile(FILE_TEST, TRUE);

        trace("test URLDownloadToFile for http protocol...\n");
        test_URLDownloadToFile(HTTP_TEST, FALSE);

        bindf |= BINDF_NOWRITECACHE;

        trace("ftp test...\n");
        test_BindToStorage(FTP_TEST, FALSE, TYMED_ISTREAM);

        trace("test failures...\n");
        test_BindToStorage_fail();
    }

    DeleteFileA(wszIndexHtmlA);
    CloseHandle(complete_event);
    CloseHandle(complete_event2);
    CoUninitialize();

    gecko_installer_workaround(FALSE);
}
