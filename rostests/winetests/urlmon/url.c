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
#include "urlmon.h"
#include "wininet.h"
#include "mshtml.h"

#include "wine/test.h"

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
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
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
DEFINE_EXPECT(OnProgress_CLASSIDAVAILABLE);
DEFINE_EXPECT(OnProgress_BEGINSYNCOPERATION);
DEFINE_EXPECT(OnProgress_ENDSYNCOPERATION);
DEFINE_EXPECT(OnStopBinding);
DEFINE_EXPECT(OnDataAvailable);
DEFINE_EXPECT(OnObjectAvailable);
DEFINE_EXPECT(Start);
DEFINE_EXPECT(Read);
DEFINE_EXPECT(LockRequest);
DEFINE_EXPECT(Terminate);
DEFINE_EXPECT(UnlockRequest);
DEFINE_EXPECT(Continue);

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

static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};

static WCHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

static const WCHAR wszWineHQSite[] =
    {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR wszWineHQIP[] =
    {'2','0','9','.','3','2','.','1','4','1','.','3',0};
static const WCHAR wszIndexHtml[] = {'i','n','d','e','x','.','h','t','m','l',0};

static BOOL stopped_binding = FALSE, emulate_protocol = FALSE,
    data_available = FALSE, http_is_first = TRUE;
static DWORD read = 0, bindf = 0, prot_state = 0, thread_id;
static CHAR mime_type[512];
static IInternetProtocolSink *protocol_sink = NULL;
static HANDLE complete_event, complete_event2;

extern IID IID_IBindStatusCallbackHolder;

static LPCWSTR urls[] = {
    WINE_ABOUT_URL,
    ABOUT_BLANK,
    INDEX_HTML,
    ITS_URL,
    MK_URL
};

static enum {
    HTTP_TEST,
    ABOUT_TEST,
    FILE_TEST,
    ITS_TEST,
    MK_TEST
} test_protocol;

static enum {
    BEFORE_DOWNLOAD,
    DOWNLOADING,
    END_DOWNLOAD
} download_state;

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
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

static HRESULT WINAPI Protocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocol, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
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

    SET_EXPECT(OnProgress_FINDINGRESOURCE);
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_FINDINGRESOURCE, wszWineHQSite);
    ok(hres == S_OK, "ReportProgress failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_CALLED(OnProgress_FINDINGRESOURCE);

    SET_EXPECT(OnProgress_CONNECTING);
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_CONNECTING, wszWineHQIP);
    ok(hres == S_OK, "ReportProgress failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_CALLED(OnProgress_CONNECTING);

    SET_EXPECT(OnProgress_SENDINGREQUEST);
    hres = IInternetProtocolSink_ReportProgress(protocol_sink,
            BINDSTATUS_SENDINGREQUEST, NULL);
    ok(hres == S_OK, "ReportProgress failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_CALLED(OnProgress_SENDINGREQUEST);

    SET_EXPECT(Continue);
    prot_state = 1;
    hres = IInternetProtocolSink_Switch(protocol_sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);
    WaitForSingleObject(complete_event, INFINITE);
    CHECK_CALLED(Continue);
    CHECK_CALLED(Read);
    CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
    CHECK_CALLED(LockRequest);
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

    SetEvent(complete_event2);

    return 0;
}

static HRESULT WINAPI Protocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    BINDINFO bindinfo, bi = {sizeof(bi), 0};
    DWORD bindf, bscf = BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION;
    WCHAR null_char = 0;
    HRESULT hres;

    CHECK_EXPECT(Start);

    read = 0;

    ok(szUrl && !lstrcmpW(szUrl, urls[test_protocol]), "wrong url\n");
    ok(pOIProtSink != NULL, "pOIProtSink == NULL\n");
    ok(pOIBindInfo != NULL, "pOIBindInfo == NULL\n");
    ok(grfPI == 0, "grfPI=%d, expected 0\n", grfPI);
    ok(dwReserved == 0, "dwReserved=%d, expected 0\n", dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08x\n", hres);

    if(test_protocol == FILE_TEST || test_protocol == MK_TEST || test_protocol == HTTP_TEST) {
        ok(bindf == (BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA
                     |BINDF_FROMURLMON),
           "bindf=%08x\n", bindf);
    }else {
        ok(bindf == (BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA|
                     BINDF_FROMURLMON|BINDF_NEEDFILE),
           "bindf=%08x\n", bindf);
    }

    ok(!memcmp(&bindinfo, &bi, sizeof(bindinfo)), "wrong bindinfo\n");

    switch(test_protocol) {
    case MK_TEST:
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_DIRECTBIND, NULL);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08x\n", hres);

    case FILE_TEST:
    case ITS_TEST:
        SET_EXPECT(OnProgress_SENDINGREQUEST);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_SENDINGREQUEST, &null_char);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08x\n", hres);
        CHECK_CALLED(OnProgress_SENDINGREQUEST);
    default:
        break;
    }

    if(test_protocol == HTTP_TEST) {
        IServiceProvider *service_provider;
        IHttpNegotiate *http_negotiate;
        IHttpNegotiate2 *http_negotiate2;
        LPWSTR ua = (LPWSTR)0xdeadbeef, accept_mimes[256];
        LPWSTR additional_headers = (LPWSTR)0xdeadbeef;
        BYTE sec_id[100];
        DWORD fetched = 256, size = 100;

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
        CreateThread(NULL, 0, thread_proc, NULL, 0, NULL);

        return S_OK;
    }

    if(test_protocol == FILE_TEST) {
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_CACHEFILENAMEAVAILABLE, &null_char);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08x\n", hres);

        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE) failed: %08x\n", hres);
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
    if(test_protocol != FILE_TEST && test_protocol != MK_TEST)
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
    SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
    SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    SET_EXPECT(LockRequest);
    SET_EXPECT(OnDataAvailable);
    SET_EXPECT(OnStopBinding);

    hres = IInternetProtocolSink_ReportData(pOIProtSink, bscf, 13, 13);
    ok(hres == S_OK, "ReportData failed: %08x\n", hres);

    CHECK_CALLED(Read);
    if(test_protocol != FILE_TEST && test_protocol != MK_TEST)
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
    CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
    CHECK_CALLED(LockRequest);
    CHECK_CALLED(OnDataAvailable);
    CHECK_CALLED(OnStopBinding);

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

        hres = IInternetProtocolSink_ReportProgress(protocol_sink,
                BINDSTATUS_MIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08x\n", hres);

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
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        SET_EXPECT(LockRequest);
        break;
    case 2:
        SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        break;
    case 3:
        SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    }
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

    if(test_protocol == HTTP_TEST) {
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
    HRESULT hres;
    IMoniker *mon;

    CHECK_EXPECT(OnStartBinding);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    ok(pib != NULL, "pib should not be NULL\n");
    ok(dwReserved == 0xff, "dwReserved=%x\n", dwReserved);

    if(pib == (void*)0xdeadbeef)
        return S_OK;

    hres = IBinding_QueryInterface(pib, &IID_IMoniker, (void**)&mon);
    ok(hres == E_NOINTERFACE, "IBinding should not have IMoniker interface\n");
    if(SUCCEEDED(hres))
        IMoniker_Release(mon);

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
        CHECK_EXPECT(OnProgress_FINDINGRESOURCE);
        if((bindf & BINDF_ASYNCHRONOUS) && emulate_protocol)
            SetEvent(complete_event);
        break;
    case BINDSTATUS_CONNECTING:
        CHECK_EXPECT(OnProgress_CONNECTING);
        if((bindf & BINDF_ASYNCHRONOUS) && emulate_protocol)
            SetEvent(complete_event);
        break;
    case BINDSTATUS_SENDINGREQUEST:
        CHECK_EXPECT(OnProgress_SENDINGREQUEST);
        if((bindf & BINDF_ASYNCHRONOUS) && emulate_protocol)
            SetEvent(complete_event);
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        ok(download_state == BEFORE_DOWNLOAD, "Download state was %d, expected BEFORE_DOWNLOAD\n",
           download_state);
        WideCharToMultiByte(CP_ACP, 0, szStatusText, -1, mime_type, sizeof(mime_type)-1, NULL, NULL);
        break;
    case BINDSTATUS_BEGINDOWNLOADDATA:
        CHECK_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, urls[test_protocol]), "wrong szStatusText\n");
        ok(download_state == BEFORE_DOWNLOAD, "Download state was %d, expected BEFORE_DOWNLOAD\n",
           download_state);
        download_state = DOWNLOADING;
        break;
    case BINDSTATUS_DOWNLOADINGDATA:
        CHECK_EXPECT2(OnProgress_DOWNLOADINGDATA);
        ok(download_state == DOWNLOADING, "Download state was %d, expected DOWNLOADING\n",
           download_state);
        break;
    case BINDSTATUS_ENDDOWNLOADDATA:
        CHECK_EXPECT(OnProgress_ENDDOWNLOADDATA);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, urls[test_protocol]), "wrong szStatusText\n");
        ok(download_state == DOWNLOADING, "Download state was %d, expected DOWNLOADING\n",
           download_state);
        download_state = END_DOWNLOAD;
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText && test_protocol == FILE_TEST)
            ok(!lstrcmpW(INDEX_HTML+7, szStatusText), "wrong szStatusText\n");
        break;
    case BINDSTATUS_CLASSIDAVAILABLE:
    {
        CLSID clsid;
        HRESULT hr;
        CHECK_EXPECT(OnProgress_CLASSIDAVAILABLE);
        hr = CLSIDFromString((LPOLESTR)szStatusText, &clsid);
        ok(hr == S_OK, "CLSIDFromString failed with error 0x%08x\n", hr);
        ok(IsEqualCLSID(&clsid, &CLSID_HTMLDocument),
            "Expected clsid to be CLSID_HTMLDocument instead of {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
            clsid.Data1, clsid.Data2, clsid.Data3,
            clsid.Data4[0], clsid.Data4[1], clsid.Data4[2], clsid.Data4[3],
            clsid.Data4[4], clsid.Data4[5], clsid.Data4[6], clsid.Data4[7]);
        break;
    }
    case BINDSTATUS_BEGINSYNCOPERATION:
        CHECK_EXPECT(OnProgress_BEGINSYNCOPERATION);
        ok(szStatusText == NULL, "Expected szStatusText to be NULL\n");
        break;
    case BINDSTATUS_ENDSYNCOPERATION:
        CHECK_EXPECT(OnProgress_ENDSYNCOPERATION);
        ok(szStatusText == NULL, "Expected szStatusText to be NULL\n");
        break;
    default:
        ok(0, "unexpexted code %d\n", ulStatusCode);
    };
    return S_OK;
}

static HRESULT WINAPI statusclb_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    CHECK_EXPECT(OnStopBinding);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    stopped_binding = TRUE;

    /* ignore DNS failure */
    if (hresult == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
        return S_OK;

    ok(hresult == S_OK, "binding failed: %08x\n", hresult);
    ok(szError == NULL, "szError should be NULL\n");

    if(test_protocol == HTTP_TEST && emulate_protocol) {
        SetEvent(complete_event);
        WaitForSingleObject(complete_event2, INFINITE);
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DWORD cbSize;

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

    CHECK_EXPECT2(OnDataAvailable);

    ok(GetCurrentThreadId() == thread_id, "wrong thread %d\n", GetCurrentThreadId());

    ok(download_state == DOWNLOADING || download_state == END_DOWNLOAD,
       "Download state was %d, expected DOWNLOADING or END_DOWNLOAD\n",
       download_state);
    data_available = TRUE;

    ok(pformatetc != NULL, "pformatetx == NULL\n");
    if(pformatetc) {
        if (mime_type[0]) todo_wine {
            clipfmt[0] = 0;
            ok(GetClipboardFormatName(pformatetc->cfFormat, clipfmt, sizeof(clipfmt)-1),
               "GetClipboardFormatName failed, error %d\n", GetLastError());
            ok(!lstrcmp(clipfmt, mime_type), "clipformat != mime_type, \"%s\" != \"%s\"\n",
               clipfmt, mime_type);
        } else {
            ok(pformatetc->cfFormat == 0, "clipformat=%x\n", pformatetc->cfFormat);
        }
        ok(pformatetc->ptd == NULL, "ptd = %p\n", pformatetc->ptd);
        ok(pformatetc->dwAspect == 1, "dwAspect=%u\n", pformatetc->dwAspect);
        ok(pformatetc->lindex == -1, "lindex=%d\n", pformatetc->lindex);
        ok(pformatetc->tymed == TYMED_ISTREAM, "tymed=%u\n", pformatetc->tymed);
    }

    ok(pstgmed != NULL, "stgmeg == NULL\n");
    if(pstgmed) {
        ok(pstgmed->tymed == TYMED_ISTREAM, "tymed=%u\n", pstgmed->tymed);
        ok(U(*pstgmed).pstm != NULL, "pstm == NULL\n");
        ok(pstgmed->pUnkForRelease != NULL, "pUnkForRelease == NULL\n");
    }

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

    if(U(*pstgmed).pstm) {
        do hres = IStream_Read(U(*pstgmed).pstm, buf, 512, &readed);
        while(hres == S_OK);
        ok(hres == S_FALSE || hres == E_PENDING, "IStream_Read returned %08x\n", hres);
    }

    if(test_protocol == HTTP_TEST && emulate_protocol && prot_state < 4)
        SetEvent(complete_event);

    return S_OK;
}

static HRESULT WINAPI statusclb_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    CHECK_EXPECT(OnObjectAvailable);
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
    IBindCtx *bctx = NULL, *bctx_arg = NULL;
    IUnknown *unk;
    BIND_OPTS bindopts;
    HRESULT hres;

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

    if(SUCCEEDED(hres))
        IBindCtx_Release(bctx);
}

static void test_bscholder(IBindStatusCallback *holder)
{
    IServiceProvider *serv_prov;
    IHttpNegotiate *http_negotiate, *http_negotiate_serv;
    IHttpNegotiate2 *http_negotiate2, *http_negotiate2_serv;
    IAuthenticate *authenticate, *authenticate_serv;
    IInternetProtocol *protocol;
    BINDINFO bindinfo = {sizeof(bindinfo)};
    LPWSTR wstr;
    DWORD dw;
    HRESULT hres;

    static const WCHAR emptyW[] = {0};

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
    ok(hres == S_OK, "Could not get IHttpNegotiate2 interface: %08x\n", hres);

    hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, (void*)0xdeadbeef, (void*)0xdeadbeef, 0);
    ok(hres == E_FAIL, "GetRootSecurityId failed: %08x\n", hres);

    IHttpNegotiate_Release(http_negotiate2);

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
}

static void test_RegisterBindStatusCallback(void)
{
    IBindStatusCallback *prevbsc, *clb;
    IBindCtx *bindctx;
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

    test_bscholder(clb);

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
}

static void test_BindToStorage(int protocol, BOOL emul)
{
    IMoniker *mon;
    HRESULT hres;
    LPOLESTR display_name;
    IBindCtx *bctx;
    MSG msg;
    IBindStatusCallback *previousclb;
    IUnknown *unk = (IUnknown*)0x00ff00ff;
    IBinding *bind;

    test_protocol = protocol;
    emulate_protocol = emul;
    download_state = BEFORE_DOWNLOAD;
    stopped_binding = FALSE;
    data_available = FALSE;
    mime_type[0] = 0;

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

    hres = CreateURLMoniker(NULL, urls[test_protocol], &mon);
    ok(SUCCEEDED(hres), "failed to create moniker: %08x\n", hres);
    if(FAILED(hres)) {
        IBindCtx_Release(bctx);
        return;
    }

    if(test_protocol == FILE_TEST && INDEX_HTML[7] == '/')
        memmove(INDEX_HTML+7, INDEX_HTML+8, lstrlenW(INDEX_HTML+7)*sizeof(WCHAR));

    hres = IMoniker_QueryInterface(mon, &IID_IBinding, (void**)&bind);
    ok(hres == E_NOINTERFACE, "IMoniker should not have IBinding interface\n");
    if(SUCCEEDED(hres))
        IBinding_Release(bind);

    hres = IMoniker_GetDisplayName(mon, bctx, NULL, &display_name);
    ok(hres == S_OK, "GetDisplayName failed %08x\n", hres);
    ok(!lstrcmpW(display_name, urls[test_protocol]), "GetDisplayName got wrong name\n");

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(QueryInterface_IInternetProtocol);
    if(!emulate_protocol)
        SET_EXPECT(QueryService_IInternetProtocol);
    SET_EXPECT(OnStartBinding);
    if(emulate_protocol) {
        SET_EXPECT(Start);
        if(test_protocol == HTTP_TEST)
            SET_EXPECT(Terminate);
        SET_EXPECT(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST) {
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(BeginningTransaction);
            SET_EXPECT(QueryInterface_IHttpNegotiate2);
            SET_EXPECT(GetRootSecurityId);
            SET_EXPECT(OnProgress_FINDINGRESOURCE);
            SET_EXPECT(OnProgress_CONNECTING);
        }
        if(test_protocol == HTTP_TEST || test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST)
            SET_EXPECT(OnResponse);
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == HTTP_TEST)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
        SET_EXPECT(OnDataAvailable);
        SET_EXPECT(OnStopBinding);
    }

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
    if (test_protocol == HTTP_TEST && hres == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
    {
        trace( "Network unreachable, skipping tests\n" );
        return;
    }
    if (!SUCCEEDED(hres)) return;

    if((bindf & BINDF_ASYNCHRONOUS) && !data_available) {
        ok(hres == MK_S_ASYNCHRONOUS, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk != NULL, "unk == NULL\n");
    }
    if(unk)
        IUnknown_Release(unk);

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
        CHECK_CALLED(Start);
        if(test_protocol == HTTP_TEST)
            CHECK_CALLED(Terminate);
        CHECK_CALLED(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST) {
            CHECK_CALLED(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(BeginningTransaction);
            /* QueryInterface_IHttpNegotiate2 and GetRootSecurityId
             * called on WinXP but not on Win98 */
            CLEAR_CALLED(QueryInterface_IHttpNegotiate2);
            CLEAR_CALLED(GetRootSecurityId);
            if(http_is_first) {
                CHECK_CALLED(OnProgress_FINDINGRESOURCE);
                CHECK_CALLED(OnProgress_CONNECTING);
            }else todo_wine {
                CHECK_NOT_CALLED(OnProgress_FINDINGRESOURCE);
                CHECK_NOT_CALLED(OnProgress_CONNECTING);
            }
        }
        if(test_protocol == HTTP_TEST || test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST)
            CHECK_CALLED(OnResponse);
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == HTTP_TEST)
            CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
        CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
        CHECK_CALLED(OnDataAvailable);
        CHECK_CALLED(OnStopBinding);
    }

    ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");
    ok(IBindCtx_Release(bctx) == 0, "bctx should be destroyed here\n");

    if(test_protocol == HTTP_TEST)
        http_is_first = FALSE;
}

static void test_BindToObject(int protocol, BOOL emul)
{
    IMoniker *mon;
    HRESULT hres;
    LPOLESTR display_name;
    IBindCtx *bctx;
    MSG msg;
    IBindStatusCallback *previousclb;
    IUnknown *unk = (IUnknown*)0x00ff00ff;
    IBinding *bind;

    test_protocol = protocol;
    emulate_protocol = emul;
    download_state = BEFORE_DOWNLOAD;
    stopped_binding = FALSE;
    data_available = FALSE;
    mime_type[0] = 0;

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, &bsc, NULL, &bctx);
    ok(SUCCEEDED(hres), "CreateAsyncBindCtx failed: %08x\n\n", hres);
    if(FAILED(hres))
        return;
    CHECK_CALLED(QueryInterface_IServiceProvider);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = RegisterBindStatusCallback(bctx, &bsc, &previousclb, 0);
    ok(SUCCEEDED(hres), "RegisterBindStatusCallback failed: %08x\n", hres);
    ok(previousclb == &bsc, "previousclb(%p) != sclb(%p)\n", previousclb, &bsc);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    if(previousclb)
        IBindStatusCallback_Release(previousclb);

    hres = CreateURLMoniker(NULL, urls[test_protocol], &mon);
    ok(SUCCEEDED(hres), "failed to create moniker: %08x\n", hres);
    if(FAILED(hres)) {
        IBindCtx_Release(bctx);
        return;
    }

    if(test_protocol == FILE_TEST && INDEX_HTML[7] == '/')
        memmove(INDEX_HTML+7, INDEX_HTML+8, lstrlenW(INDEX_HTML+7)*sizeof(WCHAR));

    hres = IMoniker_QueryInterface(mon, &IID_IBinding, (void**)&bind);
    ok(hres == E_NOINTERFACE, "IMoniker should not have IBinding interface\n");
    if(SUCCEEDED(hres))
        IBinding_Release(bind);

    hres = IMoniker_GetDisplayName(mon, bctx, NULL, &display_name);
    ok(hres == S_OK, "GetDisplayName failed %08x\n", hres);
    ok(!lstrcmpW(display_name, urls[test_protocol]), "GetDisplayName got wrong name\n");

    SET_EXPECT(QueryInterface_IServiceProvider);
    SET_EXPECT(GetBindInfo);
    SET_EXPECT(OnStartBinding);
    if(emulate_protocol) {
        SET_EXPECT(Start);
        SET_EXPECT(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST) {
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(BeginningTransaction);
            SET_EXPECT(QueryInterface_IHttpNegotiate2);
            SET_EXPECT(GetRootSecurityId);
            SET_EXPECT(OnProgress_FINDINGRESOURCE);
            SET_EXPECT(OnProgress_CONNECTING);
        }
        if(test_protocol == HTTP_TEST || test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST)
            SET_EXPECT(OnResponse);
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == HTTP_TEST)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
        SET_EXPECT(OnProgress_CLASSIDAVAILABLE);
        SET_EXPECT(OnProgress_BEGINSYNCOPERATION);
        SET_EXPECT(OnProgress_ENDSYNCOPERATION);
        SET_EXPECT(OnObjectAvailable);
        SET_EXPECT(OnStopBinding);
    }

    hres = IMoniker_BindToObject(mon, bctx, NULL, &IID_IUnknown, (void**)&unk);
    if (test_protocol == HTTP_TEST && hres == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
    {
        trace( "Network unreachable, skipping tests\n" );
        return;
    }
    todo_wine ok(SUCCEEDED(hres), "IMoniker_BindToObject failed with error 0x%08x\n", hres);
    /* no point testing the calls if binding didn't even work */
    if (!SUCCEEDED(hres)) return;

    if((bindf & BINDF_ASYNCHRONOUS) && !data_available) {
        ok(hres == MK_S_ASYNCHRONOUS, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk != NULL, "unk == NULL\n");
    }
    if(unk)
        IUnknown_Release(unk);

    while((bindf & BINDF_ASYNCHRONOUS) &&
          !stopped_binding && GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    todo_wine CHECK_NOT_CALLED(QueryInterface_IServiceProvider);
    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(OnStartBinding);
    if(emulate_protocol) {
        CHECK_CALLED(Start);
        CHECK_CALLED(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST) {
            CHECK_CALLED(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(BeginningTransaction);
            /* QueryInterface_IHttpNegotiate2 and GetRootSecurityId
             * called on WinXP but not on Win98 */
            CLEAR_CALLED(QueryInterface_IHttpNegotiate2);
            CLEAR_CALLED(GetRootSecurityId);
            if(http_is_first) {
                CHECK_CALLED(OnProgress_FINDINGRESOURCE);
                CHECK_CALLED(OnProgress_CONNECTING);
            }else todo_wine {
                CHECK_NOT_CALLED(OnProgress_FINDINGRESOURCE);
                CHECK_NOT_CALLED(OnProgress_CONNECTING);
            }
        }
        if(test_protocol == HTTP_TEST || test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST)
            CHECK_CALLED(OnResponse);
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == HTTP_TEST)
            CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
        CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
        CHECK_CALLED(OnProgress_CLASSIDAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINSYNCOPERATION);
        CHECK_CALLED(OnProgress_ENDSYNCOPERATION);
        CHECK_CALLED(OnObjectAvailable);
        CHECK_CALLED(OnStopBinding);
    }

    ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");
    ok(IBindCtx_Release(bctx) == 0, "bctx should be destroyed here\n");

    if(test_protocol == HTTP_TEST)
        http_is_first = FALSE;
}

static void set_file_url(void)
{
    int len;

    static const WCHAR wszFile[] = {'f','i','l','e',':','/','/'};

    memcpy(INDEX_HTML, wszFile, sizeof(wszFile));
    len = sizeof(wszFile)/sizeof(WCHAR);
    INDEX_HTML[len++] = '/';
    len += GetCurrentDirectoryW(sizeof(INDEX_HTML)/sizeof(WCHAR)-len, INDEX_HTML+len);
    INDEX_HTML[len++] = '\\';
    memcpy(INDEX_HTML+len, wszIndexHtml, sizeof(wszIndexHtml));
}

static void create_file(void)
{
    HANDLE file;
    DWORD size;

    static const char html_doc[] = "<HTML></HTML>";

    file = CreateFileW(wszIndexHtml, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, html_doc, sizeof(html_doc)-1, &size, NULL);
    CloseHandle(file);

    set_file_url();
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
    ok(hres == MK_E_SYNTAX, "hres=%08x, expected INET_E_SYNTAX\n", hres);

    IBindCtx_Release(bctx);

    IMoniker_Release(mon);
}

START_TEST(url)
{
    complete_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    complete_event2 = CreateEvent(NULL, FALSE, FALSE, NULL);
    thread_id = GetCurrentThreadId();

    test_create();
    test_CreateAsyncBindCtx();
    test_CreateAsyncBindCtxEx();
    test_RegisterBindStatusCallback();

    trace("synchronous http test (COM not initialised)...\n");
    test_BindToStorage(HTTP_TEST, FALSE);
    test_BindToStorage_fail();

    CoInitialize(NULL);

    trace("synchronous http test...\n");
    test_BindToStorage(HTTP_TEST, FALSE);
    test_BindToObject(HTTP_TEST, FALSE);

    trace("synchronous file test...\n");
    create_file();
    test_BindToStorage(FILE_TEST, FALSE);
    test_BindToObject(FILE_TEST, FALSE);
    DeleteFileW(wszIndexHtml);

    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

    trace("http test...\n");
    test_BindToStorage(HTTP_TEST, FALSE);
    test_BindToObject(HTTP_TEST, FALSE);

    trace("http test (short response)...\n");
    http_is_first = TRUE;
    urls[HTTP_TEST] = SHORT_RESPONSE_URL;
    test_BindToStorage(HTTP_TEST, FALSE);
    test_BindToObject(HTTP_TEST, FALSE);

    trace("emulated http test...\n");
    test_BindToStorage(HTTP_TEST, TRUE);

    trace("about test...\n");
    test_BindToStorage(ABOUT_TEST, FALSE);
    test_BindToObject(ABOUT_TEST, FALSE);

    trace("emulated about test...\n");
    test_BindToStorage(ABOUT_TEST, TRUE);

    trace("file test...\n");
    create_file();
    test_BindToStorage(FILE_TEST, FALSE);
    test_BindToObject(FILE_TEST, FALSE);
    DeleteFileW(wszIndexHtml);

    trace("emulated file test...\n");
    set_file_url();
    test_BindToStorage(FILE_TEST, TRUE);

    trace("emulated its test...\n");
    test_BindToStorage(ITS_TEST, TRUE);

    trace("emulated mk test...\n");
    test_BindToStorage(MK_TEST, TRUE);

    test_BindToStorage_fail();

    CloseHandle(complete_event);
    CloseHandle(complete_event2);
    CoUninitialize();
}
