/*
 * Copyright 2005-2007 Jacek Caban for CodeWeavers
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
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportResult);
DEFINE_EXPECT(GetBindString_ACCEPT_MIMES);
DEFINE_EXPECT(GetBindString_USER_AGENT);
DEFINE_EXPECT(GetBindString_POST_COOKIE);
DEFINE_EXPECT(QueryService_HttpNegotiate);
DEFINE_EXPECT(QueryService_InternetProtocol);
DEFINE_EXPECT(QueryService_HttpSecurity);
DEFINE_EXPECT(BeginningTransaction);
DEFINE_EXPECT(GetRootSecurityId);
DEFINE_EXPECT(OnResponse);
DEFINE_EXPECT(Switch);
DEFINE_EXPECT(Continue);
DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(Start);
DEFINE_EXPECT(Terminate);
DEFINE_EXPECT(Read);
DEFINE_EXPECT(SetPriority);
DEFINE_EXPECT(LockRequest);
DEFINE_EXPECT(UnlockRequest);

static const WCHAR wszIndexHtml[] = {'i','n','d','e','x','.','h','t','m','l',0};
static const WCHAR index_url[] =
    {'f','i','l','e',':','i','n','d','e','x','.','h','t','m','l',0};

static const WCHAR acc_mimeW[] = {'*','/','*',0};
static const WCHAR user_agentW[] = {'W','i','n','e',0};
static const WCHAR text_htmlW[] = {'t','e','x','t','/','h','t','m','l',0};
static const WCHAR hostW[] = {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR winehq_ipW[] = {'2','0','9','.','4','6','.','2','5','.','1','3','4',0};
static const WCHAR emptyW[] = {0};

static HRESULT expect_hrResult;
static LPCWSTR file_name, http_url, expect_wsz;
static IInternetProtocol *async_protocol = NULL;
static BOOL first_data_notif = FALSE, http_is_first = FALSE,
    http_post_test = FALSE;
static int state = 0, prot_state;
static DWORD bindf = 0, ex_priority = 0;
static IInternetProtocol *binding_protocol;
static IInternetBindInfo *prot_bind_info;
static IInternetProtocolSink *binding_sink;
static void *expect_pv;
static HANDLE event_complete, event_complete2;
static BOOL binding_test;
static PROTOCOLDATA protocoldata, *pdata;
static DWORD prot_read;
static BOOL security_problem = FALSE;

static enum {
    FILE_TEST,
    HTTP_TEST,
    HTTPS_TEST,
    FTP_TEST,
    MK_TEST,
    BIND_TEST
} tested_protocol;

static const WCHAR protocol_names[][10] = {
    {'f','i','l','e',0},
    {'h','t','t','p',0},
    {'h','t','t','p','s',0},
    {'f','t','p',0},
    {'m','k',0},
    {'t','e','s','t',0}
};

static const WCHAR binding_urls[][130] = {
    {'f','i','l','e',':','t','e','s','t','.','h','t','m','l',0},
    {'h','t','t','p',':','/','/','t','e','s','t','/','t','e','s','t','.','h','t','m','l',0},
    {'h','t','t','p','s',':','/','/','w','w','w','.','c','o','d','e','w','e','a','v','e','r','s',
     '.','c','o','m','/','t','e','s','t','.','h','t','m','l',0},
    {'f','t','p',':','/','/','f','t','p','.','w','i','n','e','h','q','.','o','r','g',
     '/','p','u','b','/','o','t','h','e','r',
     '/','w','i','n','e','l','o','g','o','.','x','c','f','.','t','a','r','.','b','z','2',0},
    {'m','k',':','t','e','s','t',0},
    {'t','e','s','t',':','/','/','f','i','l','e','.','h','t','m','l',0}
};

static const char *debugstr_w(LPCWSTR str)
{
    static char buf[512];
    if(!str)
        return "(null)";
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

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    WCHAR buf[512];
    MultiByteToWideChar(CP_ACP, 0, stra, -1, buf, sizeof(buf)/sizeof(WCHAR));
    return lstrcmpW(strw, buf);
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
    trace("HttpSecurity_GetWindow\n");

    return S_FALSE;
}

static HRESULT WINAPI HttpSecurity_OnSecurityProblem(IHttpSecurity *iface, DWORD dwProblem)
{
    trace("Security problem: %u\n", dwProblem);
    ok(dwProblem == ERROR_INTERNET_SEC_CERT_REV_FAILED, "Expected ERROR_INTERNET_SEC_CERT_REV_FAILED got %u\n", dwProblem);

    /* Only retry once */
    if (security_problem)
        return E_ABORT;

    security_problem = TRUE;
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
    ok(!dwReserved, "dwReserved=%d, expected 0\n", dwReserved);
    ok(pszAdditionalHeaders != NULL, "pszAdditionalHeaders == NULL\n");
    if(pszAdditionalHeaders)
    {
        ok(*pszAdditionalHeaders == NULL, "*pszAdditionalHeaders != NULL\n");
        if (http_post_test)
        {
            addl_headers = CoTaskMemAlloc(sizeof(wszHeaders));
            if (!addl_headers)
            {
                http_post_test = FALSE;
                skip("Out of memory\n");
                return E_OUTOFMEMORY;
            }
            lstrcpyW(addl_headers, wszHeaders);
            *pszAdditionalHeaders = addl_headers;
        }
    }

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    CHECK_EXPECT(OnResponse);

    ok(dwResponseCode == 200, "dwResponseCode=%d, expected 200\n", dwResponseCode);
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

    ok(!dwReserved, "dwReserved=%ld, expected 0\n", dwReserved);
    ok(pbSecurityId != NULL, "pbSecurityId == NULL\n");
    ok(pcbSecurityId != NULL, "pcbSecurityId == NULL\n");

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

static IHttpNegotiate2 http_negotiate = { &HttpNegotiateVtbl };

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

    ok(0, "unexpected service %s\n", debugstr_guid(guidService));
    return E_FAIL;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider service_provider = { &ServiceProviderVtbl };

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

static HRESULT WINAPI ProtocolSink_Switch(IInternetProtocolSink *iface, PROTOCOLDATA *pProtocolData)
{
    HRESULT hres;

    if(tested_protocol == FTP_TEST)
      CHECK_EXPECT2(Switch);
    else
      CHECK_EXPECT(Switch);
    ok(pProtocolData != NULL, "pProtocolData == NULL\n");

    pdata = pProtocolData;

    if(binding_test) {
        SetEvent(event_complete);
        WaitForSingleObject(event_complete2, INFINITE);
        return S_OK;
    }

    if (!state) {
        if(tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST || tested_protocol == FTP_TEST) {
            if (http_is_first) {
                CLEAR_CALLED(ReportProgress_FINDINGRESOURCE);
                CLEAR_CALLED(ReportProgress_CONNECTING);
                CLEAR_CALLED(ReportProgress_PROXYDETECTING);
            } else todo_wine {
                    CHECK_NOT_CALLED(ReportProgress_FINDINGRESOURCE);
                    /* IE7 does call this */
                    CLEAR_CALLED(ReportProgress_CONNECTING);
                }
        }
        if(tested_protocol == FTP_TEST)
            todo_wine CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        else if (tested_protocol != HTTPS_TEST)
            CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        if(tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST) {
            SET_EXPECT(OnResponse);
            if(tested_protocol == HTTPS_TEST)
                SET_EXPECT(ReportProgress_ACCEPTRANGES);
            SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
            if(bindf & BINDF_NEEDFILE)
                SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        }
    }

    SET_EXPECT(ReportData);
    hres = IInternetProtocol_Continue(async_protocol, pProtocolData);
    ok(hres == S_OK, "Continue failed: %08x\n", hres);
    if(tested_protocol == FTP_TEST)
        CLEAR_CALLED(ReportData);
    else if (! security_problem)
        CHECK_CALLED(ReportData);

    if (!state) {
        if (! security_problem)
        {
            state = 1;
            if(tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST) {
                CHECK_CALLED(OnResponse);
                if(tested_protocol == HTTPS_TEST)
                    CHECK_CALLED(ReportProgress_ACCEPTRANGES);
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
    }

    SetEvent(event_complete);

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportProgress(IInternetProtocolSink *iface, ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    static const WCHAR null_guid[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-',
        '0','0','0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0','0','}',0};
    static const WCHAR text_plain[] = {'t','e','x','t','/','p','l','a','i','n',0};

    switch(ulStatusCode) {
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText) {
            if(tested_protocol == BIND_TEST)
                ok(szStatusText == expect_wsz, "unexpected szStatusText\n");
            else if (http_post_test)
                ok(lstrlenW(text_plain) <= lstrlenW(szStatusText) &&
                   !memcmp(szStatusText, text_plain, lstrlenW(text_plain)*sizeof(WCHAR)),
                   "szStatusText != text/plain\n");
            else
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
                ok(!lstrcmpW(szStatusText, file_name), "szStatusText = \"%s\"\n", debugstr_w(szStatusText));
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
        if(tested_protocol == FILE_TEST) {
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            if(szStatusText)
                ok(!*szStatusText, "wrong szStatusText\n");
        }
        break;
    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, text_htmlW), "szStatusText != text/html\n");
        break;
    case BINDSTATUS_PROTOCOLCLASSID:
        CHECK_EXPECT(ReportProgress_PROTOCOLCLASSID);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        ok(!lstrcmpW(szStatusText, null_guid), "unexpected szStatusText\n");
        break;
    case BINDSTATUS_COOKIE_SENT:
        CHECK_EXPECT(ReportProgress_COOKIE_SENT);
        ok(szStatusText == NULL, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_REDIRECTING:
        CHECK_EXPECT(ReportProgress_REDIRECTING);
        ok(szStatusText == NULL, "szStatusText = %s\n", debugstr_w(szStatusText));
        break;
    case BINDSTATUS_ENCODING:
        CHECK_EXPECT(ReportProgress_ENCODING);
        ok(!strcmp_wa(szStatusText, "gzip"), "szStatusText = %s\n", debugstr_w(szStatusText));
        break;
    case BINDSTATUS_ACCEPTRANGES:
        CHECK_EXPECT(ReportProgress_ACCEPTRANGES);
        ok(!szStatusText, "szStatusText = %s\n", debugstr_w(szStatusText));
        break;
    case BINDSTATUS_PROXYDETECTING:
        CHECK_EXPECT(ReportProgress_PROXYDETECTING);
        SET_EXPECT(ReportProgress_CONNECTING);
        ok(!szStatusText, "szStatusText = %s\n", debugstr_w(szStatusText));
        break;
    default:
        ok(0, "Unexpected status %d\n", ulStatusCode);
    };

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF,
        ULONG ulProgress, ULONG ulProgressMax)
{
    if(tested_protocol == FILE_TEST) {
        CHECK_EXPECT2(ReportData);

        ok(ulProgress == ulProgressMax, "ulProgress (%d) != ulProgressMax (%d)\n",
           ulProgress, ulProgressMax);
        ok(ulProgressMax == 13, "ulProgressMax=%d, expected 13\n", ulProgressMax);
        /* BSCF_SKIPDRAINDATAFORFILEURLS added in IE8 */
        ok((grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION)) ||
           (grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_SKIPDRAINDATAFORFILEURLS)),
                "grcfBSCF = %08x\n", grfBSCF);
    }else if(!binding_test && (tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST || tested_protocol == FTP_TEST)) {
        if(!(grfBSCF & BSCF_LASTDATANOTIFICATION) || (grfBSCF & BSCF_DATAFULLYAVAILABLE))
            CHECK_EXPECT(ReportData);
        else if (http_post_test)
            ok(ulProgress == 13, "Read %u bytes instead of 13\n", ulProgress);

        ok(ulProgress, "ulProgress == 0\n");

        if(first_data_notif) {
            ok(grfBSCF == BSCF_FIRSTDATANOTIFICATION
               || grfBSCF == (BSCF_LASTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE),
               "grcfBSCF = %08x\n", grfBSCF);
            first_data_notif = FALSE;
        } else {
            ok(grfBSCF == BSCF_INTERMEDIATEDATANOTIFICATION
               || grfBSCF == (BSCF_LASTDATANOTIFICATION|BSCF_INTERMEDIATEDATANOTIFICATION)
               || broken(grfBSCF == (BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION)),
               "grcfBSCF = %08x\n", grfBSCF);
        }

        if(!(bindf & BINDF_FROMURLMON) &&
           !(grfBSCF & BSCF_LASTDATANOTIFICATION)) {
            if(!state) {
                state = 1;
                if(http_is_first) {
                    CHECK_CALLED(ReportProgress_FINDINGRESOURCE);
                    CHECK_CALLED(ReportProgress_CONNECTING);
                } else todo_wine {
                    CHECK_NOT_CALLED(ReportProgress_FINDINGRESOURCE);
                    CHECK_NOT_CALLED(ReportProgress_CONNECTING);
                }
                CHECK_CALLED(ReportProgress_SENDINGREQUEST);
                CHECK_CALLED(OnResponse);
                CHECK_CALLED(ReportProgress_RAWMIMETYPE);
            }
            SetEvent(event_complete);
        }
    }else {
        BYTE buf[1000];
        ULONG read;
        HRESULT hres;

        CHECK_EXPECT(ReportData);

        if(tested_protocol == BIND_TEST)
            return S_OK;

        do {
            SET_EXPECT(Read);
            hres = IInternetProtocol_Read(binding_protocol, expect_pv=buf, sizeof(buf), &read);
            CHECK_CALLED(Read);
        }while(hres == S_OK);
    }

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult,
        DWORD dwError, LPCWSTR szResult)
{
    CHECK_EXPECT(ReportResult);

    if(tested_protocol == FTP_TEST)
        ok(hrResult == E_PENDING || hrResult == S_OK, "hrResult = %08x, expected E_PENDING or S_OK\n", hrResult);
    else
        ok(hrResult == expect_hrResult, "hrResult = %08x, expected: %08x\n",
           hrResult, expect_hrResult);
    if(SUCCEEDED(hrResult) || tested_protocol == FTP_TEST)
        ok(dwError == ERROR_SUCCESS, "dwError = %d, expected ERROR_SUCCESS\n", dwError);
    else
        ok(dwError != ERROR_SUCCESS ||
           broken(tested_protocol == MK_TEST), /* Win9x, WinME and NT4 */
           "dwError == ERROR_SUCCESS\n");
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

static IInternetProtocolSink protocol_sink = { &protocol_sink_vtbl };

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocolSink, riid))
        *ppv = &protocol_sink;
    if(IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &service_provider;

    if(*ppv)
        return S_OK;

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

    static const CHAR szPostData[] = "mode=Test";

    CHECK_EXPECT(GetBindInfo);

    ok(grfBINDF != NULL, "grfBINDF == NULL\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %d\n", pbindinfo->cbSize);

    *grfBINDF = bindf;
    if(binding_test)
        *grfBINDF |= BINDF_FROMURLMON;
    cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize);
    pbindinfo->cbSize = cbSize;

    if (http_post_test)
    {
        /* Must be GMEM_FIXED, GMEM_MOVABLE does not work properly
         * with urlmon on native (Win98 and WinXP) */
        U(pbindinfo->stgmedData).hGlobal = GlobalAlloc(GPTR, sizeof(szPostData));
        if (!U(pbindinfo->stgmedData).hGlobal)
        {
            http_post_test = FALSE;
            skip("Out of memory\n");
            return E_OUTOFMEMORY;
        }
        lstrcpy((LPSTR)U(pbindinfo->stgmedData).hGlobal, szPostData);
        pbindinfo->cbstgmedData = sizeof(szPostData)-1;
        pbindinfo->dwBindVerb = BINDVERB_POST;
        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
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
        ok(cEl == 256, "cEl=%d, expected 256\n", cEl);
        if(pcElFetched) {
            ok(*pcElFetched == 256, "*pcElFetched=%d, expected 256\n", *pcElFetched);
            *pcElFetched = 1;
        }
        if(ppwzStr) {
            *ppwzStr = CoTaskMemAlloc(sizeof(acc_mimeW));
            memcpy(*ppwzStr, acc_mimeW, sizeof(acc_mimeW));
        }
        return S_OK;
    case BINDSTRING_USER_AGENT:
        CHECK_EXPECT(GetBindString_USER_AGENT);
        ok(cEl == 1, "cEl=%d, expected 1\n", cEl);
        if(pcElFetched) {
            ok(*pcElFetched == 0, "*pcElFetch=%d, expectd 0\n", *pcElFetched);
            *pcElFetched = 1;
        }
        if(ppwzStr) {
            *ppwzStr = CoTaskMemAlloc(sizeof(user_agentW));
            memcpy(*ppwzStr, user_agentW, sizeof(user_agentW));
        }
        return S_OK;
    case BINDSTRING_POST_COOKIE:
        CHECK_EXPECT(GetBindString_POST_COOKIE);
        ok(cEl == 1, "cEl=%d, expected 1\n", cEl);
        if(pcElFetched)
            ok(*pcElFetched == 0, "*pcElFetch=%d, expectd 0\n", *pcElFetched);
        return S_OK;
    default:
        ok(0, "unexpected call\n");
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

static HRESULT WINAPI InternetPriority_QueryInterface(IInternetPriority *iface,
                                                  REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetPriority_AddRef(IInternetPriority *iface)
{
    return 2;
}

static ULONG WINAPI InternetPriority_Release(IInternetPriority *iface)
{
    return 1;
}

static HRESULT WINAPI InternetPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    CHECK_EXPECT(SetPriority);
    ok(nPriority == ex_priority, "nPriority=%d\n", nPriority);
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

static IInternetPriority InternetPriority = { &InternetPriorityVtbl };

static HRESULT WINAPI Protocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocol, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IInternetProtocolEx, riid)) {
        trace("IID_IInternetProtocolEx not supported\n");
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        *ppv = &InternetPriority;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", debugstr_guid(riid));
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
    HRESULT hres;

    memset(&protocoldata, -1, sizeof(protocoldata));

    prot_state = 0;

    SET_EXPECT(ReportProgress_FINDINGRESOURCE);
    hres = IInternetProtocolSink_ReportProgress(binding_sink,
            BINDSTATUS_FINDINGRESOURCE, hostW);
    CHECK_CALLED(ReportProgress_FINDINGRESOURCE);
    ok(hres == S_OK, "ReportProgress failed: %08x\n", hres);

    SET_EXPECT(ReportProgress_CONNECTING);
    hres = IInternetProtocolSink_ReportProgress(binding_sink,
            BINDSTATUS_CONNECTING, winehq_ipW);
    CHECK_CALLED(ReportProgress_CONNECTING);
    ok(hres == S_OK, "ReportProgress failed: %08x\n", hres);

    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    hres = IInternetProtocolSink_ReportProgress(binding_sink,
            BINDSTATUS_SENDINGREQUEST, NULL);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    ok(hres == S_OK, "ReportProgress failed: %08x\n", hres);

    prot_state = 1;
    SET_EXPECT(Switch);
    hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
    CHECK_CALLED(Switch);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);

    prot_state = 2;
    SET_EXPECT(Switch);
    hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
    CHECK_CALLED(Switch);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);

    prot_state = 2;
    SET_EXPECT(Switch);
    hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
    CHECK_CALLED(Switch);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);

    prot_state = 3;
    SET_EXPECT(Switch);
    hres = IInternetProtocolSink_Switch(binding_sink, &protocoldata);
    CHECK_CALLED(Switch);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);

    SetEvent(event_complete);

    return 0;
}

static HRESULT WINAPI Protocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    BINDINFO bindinfo, exp_bindinfo;
    DWORD cbindf = 0;
    HRESULT hres;

    CHECK_EXPECT(Start);

    ok(pOIProtSink != NULL, "pOIProtSink == NULL\n");
    ok(pOIBindInfo != NULL, "pOIBindInfo == NULL\n");
    ok(pOIProtSink != &protocol_sink, "unexpected pOIProtSink\n");
    ok(pOIBindInfo != &bind_info, "unexpected pOIBindInfo\n");
    ok(!grfPI, "grfPI = %x\n", grfPI);
    ok(!dwReserved, "dwReserved = %lx\n", dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    memcpy(&exp_bindinfo, &bindinfo, sizeof(bindinfo));
    SET_EXPECT(GetBindInfo);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &cbindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08x\n", hres);
    CHECK_CALLED(GetBindInfo);
    ok(cbindf == (bindf|BINDF_FROMURLMON), "bindf = %x, expected %x\n",
       cbindf, (bindf|BINDF_FROMURLMON));
    ok(!memcmp(&exp_bindinfo, &bindinfo, sizeof(bindinfo)), "unexpected bindinfo\n");
    ReleaseBindInfo(&bindinfo);

    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_SENDINGREQUEST, emptyW);
    ok(hres == S_OK, "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08x\n", hres);
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
        ok(hres == S_OK, "GetBindString(BINDSTRING_USER_AGETNT) failed: %08x\n", hres);
        ok(fetched == 1, "fetched = %d, expected 254\n", fetched);
        ok(ua != NULL, "ua =  %p\n", ua);
        ok(!lstrcmpW(ua, user_agentW), "unexpected user agent %s\n", debugstr_w(ua));
        CoTaskMemFree(ua);

        fetched = 256;
        SET_EXPECT(GetBindString_ACCEPT_MIMES);
        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                               accept_mimes, 256, &fetched);
        CHECK_CALLED(GetBindString_ACCEPT_MIMES);

        ok(hres == S_OK,
           "GetBindString(BINDSTRING_ACCEPT_MIMES) failed: %08x\n", hres);
        ok(fetched == 1, "fetched = %d, expected 1\n", fetched);
        ok(!lstrcmpW(acc_mimeW, accept_mimes[0]), "unexpected mimes %s\n", debugstr_w(accept_mimes[0]));

        hres = IInternetBindInfo_QueryInterface(pOIBindInfo, &IID_IServiceProvider,
                                                (void**)&service_provider);
        ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);

        SET_EXPECT(QueryService_HttpNegotiate);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
                &IID_IHttpNegotiate, (void**)&http_negotiate);
        CHECK_CALLED(QueryService_HttpNegotiate);
        ok(hres == S_OK, "QueryService failed: %08x\n", hres);

        SET_EXPECT(BeginningTransaction);
        hres = IHttpNegotiate_BeginningTransaction(http_negotiate, binding_urls[tested_protocol],
                                                   NULL, 0, &additional_headers);
        CHECK_CALLED(BeginningTransaction);
        IHttpNegotiate_Release(http_negotiate);
        ok(hres == S_OK, "BeginningTransction failed: %08x\n", hres);
        ok(additional_headers == NULL, "additional_headers=%p\n", additional_headers);

        SET_EXPECT(QueryService_HttpNegotiate);
        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate2,
                &IID_IHttpNegotiate2, (void**)&http_negotiate2);
        CHECK_CALLED(QueryService_HttpNegotiate);
        ok(hres == S_OK, "QueryService failed: %08x\n", hres);

        size = 512;
        SET_EXPECT(GetRootSecurityId);
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, sec_id, &size, 0);
        CHECK_CALLED(GetRootSecurityId);
        IHttpNegotiate2_Release(http_negotiate2);
        ok(hres == E_FAIL, "GetRootSecurityId failed: %08x, expected E_FAIL\n", hres);
        ok(size == 13, "size=%d\n", size);

        IServiceProvider_Release(service_provider);

        CreateThread(NULL, 0, thread_proc, NULL, 0, &tid);

        return S_OK;
    }

    SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
    hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
            BINDSTATUS_CACHEFILENAMEAVAILABLE, expect_wsz = emptyW);
    ok(hres == S_OK, "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08x\n", hres);
    CHECK_CALLED(ReportProgress_CACHEFILENAMEAVAILABLE);

    SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE,
                                                expect_wsz = text_htmlW);
    ok(hres == S_OK,
       "ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE) failed: %08x\n", hres);
    CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);

    SET_EXPECT(ReportData);
    hres = IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION, 13, 13);
    ok(hres == S_OK, "ReportData failed: %08x\n", hres);
    CHECK_CALLED(ReportData);

    if(tested_protocol == BIND_TEST) {
        hres = IInternetProtocol_Terminate(binding_protocol, 0);
        ok(hres == E_FAIL, "Termiante failed: %08x\n", hres);
    }

    SET_EXPECT(ReportResult);
    hres = IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);
    ok(hres == S_OK, "ReportResult failed: %08x\n", hres);
    CHECK_CALLED(ReportResult);

    return S_OK;
}

static HRESULT WINAPI Protocol_Continue(IInternetProtocol *iface,
        PROTOCOLDATA *pProtocolData)
{
    DWORD bscf = 0;
    HRESULT hres;

    CHECK_EXPECT(Continue);

    ok(pProtocolData != NULL, "pProtocolData == NULL\n");
    if(!pProtocolData || tested_protocol == BIND_TEST)
        return S_OK;

    switch(prot_state) {
    case 1: {
        IServiceProvider *service_provider;
        IHttpNegotiate *http_negotiate;
        static WCHAR header[] = {'?',0};

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
        ok(hres == S_OK, "OnResponse failed: %08x\n", hres);

        SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        hres = IInternetProtocolSink_ReportProgress(binding_sink,
                BINDSTATUS_MIMETYPEAVAILABLE, text_htmlW);
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
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

    SET_EXPECT(ReportData);
    hres = IInternetProtocolSink_ReportData(binding_sink, bscf, 100, 400);
    CHECK_CALLED(ReportData);
    ok(hres == S_OK, "ReportData failed: %08x\n", hres);

    if(prot_state == 3)
        prot_state = 4;

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
    ok(!dwOptions, "dwOptions=%d\n", dwOptions);
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
    static BOOL b = TRUE;

    CHECK_EXPECT(Read);

    ok(pv == expect_pv, "pv != expect_pv\n");
    ok(cb == 1000, "cb=%d\n", cb);
    ok(pcbRead != NULL, "pcbRead == NULL\n");
    ok(!*pcbRead, "*pcbRead = %d\n", *pcbRead);

    if(prot_state == 3) {
        HRESULT hres;

        SET_EXPECT(ReportResult);
        hres = IInternetProtocolSink_ReportResult(binding_sink, S_OK, ERROR_SUCCESS, NULL);
        CHECK_CALLED(ReportResult);

        return S_FALSE;
    }

    if((b = !b))
        return tested_protocol == HTTP_TEST || tested_protocol == HTTPS_TEST ? E_PENDING : S_FALSE;

    memset(pv, 'x', 100);
    prot_read += *pcbRead = 100;
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
    ok(dwOptions == 0, "dwOptions=%x\n", dwOptions);
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

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
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
    CHECK_EXPECT(CreateInstance);

    ok(pOuter == (IUnknown*)prot_bind_info, "pOuter != protocol_unk\n");
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", debugstr_guid(riid));
    ok(ppv != NULL, "ppv == NULL\n");

    *ppv = &Protocol;
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

static void test_priority(IInternetProtocol *protocol)
{
    IInternetPriority *priority;
    LONG pr;
    HRESULT hres;

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority,
                                            (void**)&priority);
    ok(hres == S_OK, "QueryInterface(IID_IInternetPriority) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetPriority_GetPriority(priority, &pr);
    ok(hres == S_OK, "GetPriority failed: %08x\n", hres);
    ok(pr == 0, "pr=%d, expected 0\n", pr);

    hres = IInternetPriority_SetPriority(priority, 1);
    ok(hres == S_OK, "SetPriority failed: %08x\n", hres);

    hres = IInternetPriority_GetPriority(priority, &pr);
    ok(hres == S_OK, "GetPriority failed: %08x\n", hres);
    ok(pr == 1, "pr=%d, expected 1\n", pr);

    IInternetPriority_Release(priority);
}

static BOOL file_protocol_start(IInternetProtocol *protocol, LPCWSTR url, BOOL is_first)
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

    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    if(hres == INET_E_RESOURCE_NOT_FOUND) {
        win_skip("Start failed\n");
        return FALSE;
    }
    ok(hres == S_OK, "Start failed: %08x\n", hres);

    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
       CHECK_CALLED(ReportProgress_DIRECTBIND);
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
    HRESULT hres;

    hres = CoGetClassObject(&CLSID_FileProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
            "Could not get IInternetProtocolInfo interface: %08x, expected E_NOINTERFACE\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        IInternetProtocol *protocol;
        BYTE buf[512];
        ULONG cb;
        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);

        if(SUCCEEDED(hres)) {
            if(file_protocol_start(protocol, url, TRUE)) {
                hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
                ok(hres == S_OK, "Read failed: %08x\n", hres);
                ok(cb == 2, "cb=%u expected 2\n", cb);
                hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
                ok(hres == S_FALSE, "Read failed: %08x\n", hres);
                hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
                ok(hres == S_FALSE, "Read failed: %08x expected S_FALSE\n", hres);
                ok(cb == 0, "cb=%u expected 0\n", cb);
                hres = IInternetProtocol_UnlockRequest(protocol);
                ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
            }

            if(file_protocol_start(protocol, url, FALSE)) {
                hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
                ok(hres == S_FALSE, "Read failed: %08x\n", hres);
                hres = IInternetProtocol_LockRequest(protocol, 0);
                ok(hres == S_OK, "LockRequest failed: %08x\n", hres);
                hres = IInternetProtocol_UnlockRequest(protocol);
                ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
            }

            IInternetProtocol_Release(protocol);
        }

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);

        if(SUCCEEDED(hres)) {
            if(file_protocol_start(protocol, url, TRUE)) {
                hres = IInternetProtocol_LockRequest(protocol, 0);
                ok(hres == S_OK, "LockRequest failed: %08x\n", hres);
                hres = IInternetProtocol_Terminate(protocol, 0);
                ok(hres == S_OK, "Terminate failed: %08x\n", hres);
                hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
                ok(hres == S_OK, "Read failed: %08x\n\n", hres);
                hres = IInternetProtocol_UnlockRequest(protocol);
                ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
                hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
                ok(hres == S_OK, "Read failed: %08x\n", hres);
                hres = IInternetProtocol_Terminate(protocol, 0);
                ok(hres == S_OK, "Terminate failed: %08x\n", hres);
            }

            IInternetProtocol_Release(protocol);
        }

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);

        if(SUCCEEDED(hres)) {
            if(file_protocol_start(protocol, url, TRUE)) {
                hres = IInternetProtocol_Terminate(protocol, 0);
                ok(hres == S_OK, "Terminate failed: %08x\n", hres);
                hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
                ok(hres == S_OK, "Read failed: %08x\n", hres);
                ok(cb == 2, "cb=%u expected 2\n", cb);
            }

            IInternetProtocol_Release(protocol);
        }

        IClassFactory_Release(factory);
    }

    IUnknown_Release(unk);
}

static void test_file_protocol_fail(void)
{
    IInternetProtocol *protocol;
    HRESULT hres;

    static const WCHAR index_url2[] =
        {'f','i','l','e',':','/','/','i','n','d','e','x','.','h','t','m','l',0};

    hres = CoCreateInstance(&CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(GetBindInfo);
    expect_hrResult = MK_E_SYNTAX;
    hres = IInternetProtocol_Start(protocol, wszIndexHtml, &protocol_sink, &bind_info, 0, 0);
    ok(hres == MK_E_SYNTAX ||
       hres == E_INVALIDARG,
       "Start failed: %08x, expected MK_E_SYNTAX or E_INVALIDARG\n", hres);
    CLEAR_CALLED(GetBindInfo); /* GetBindInfo not called in IE7 */

    SET_EXPECT(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportResult);
    expect_hrResult = INET_E_RESOURCE_NOT_FOUND;
    hres = IInternetProtocol_Start(protocol, index_url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == INET_E_RESOURCE_NOT_FOUND,
            "Start failed: %08x expected INET_E_RESOURCE_NOT_FOUND\n", hres);
    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        CHECK_CALLED(ReportProgress_DIRECTBIND);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    CHECK_CALLED(ReportResult);

    IInternetProtocol_Release(protocol);

    hres = CoCreateInstance(&CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
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
            "Start failed: %08x, expected INET_E_RESOURCE_NOT_FOUND\n", hres);
    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        CHECK_CALLED(ReportProgress_DIRECTBIND);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    CHECK_CALLED(ReportResult);

    SET_EXPECT(GetBindInfo);
    hres = IInternetProtocol_Start(protocol, NULL, &protocol_sink, &bind_info, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08x, expected E_INVALIDARG\n", hres);
    CLEAR_CALLED(GetBindInfo); /* GetBindInfo not called in IE7 */

    SET_EXPECT(GetBindInfo);
    hres = IInternetProtocol_Start(protocol, emptyW, &protocol_sink, &bind_info, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08x, expected E_INVALIDARG\n", hres);
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
    static const char html_doc[] = "<HTML></HTML>";

    trace("Testing file protocol...\n");
    tested_protocol = FILE_TEST;

    SetLastError(0xdeadbeef);
    file = CreateFileW(wszIndexHtml, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    if(!file && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Detected Win9x or WinMe\n");
        return;
    }
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
    len = sizeof(wszFile)/sizeof(WCHAR)-1;
    len += GetCurrentDirectoryW(sizeof(buf)/sizeof(WCHAR)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + sizeof(wszFile)/sizeof(WCHAR)-1;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    memcpy(buf, wszFile2, sizeof(wszFile2));
    len = GetCurrentDirectoryW(sizeof(file_name_buf)/sizeof(WCHAR), file_name_buf);
    file_name_buf[len++] = '\\';
    memcpy(file_name_buf+len, wszIndexHtml, sizeof(wszIndexHtml));
    lstrcpyW(buf+sizeof(wszFile2)/sizeof(WCHAR)-1, file_name_buf);
    file_name = file_name_buf;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    buf[sizeof(wszFile2)/sizeof(WCHAR)] = '|';
    test_file_protocol_url(buf);

    memcpy(buf, wszFile3, sizeof(wszFile3));
    len = sizeof(wszFile3)/sizeof(WCHAR)-1;
    len += GetCurrentDirectoryW(sizeof(buf)/sizeof(WCHAR)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + sizeof(wszFile3)/sizeof(WCHAR)-1;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    DeleteFileW(wszIndexHtml);

    bindf = 0;
    test_file_protocol_fail();
    bindf = BINDF_FROMURLMON;
    test_file_protocol_fail();
}

static BOOL http_protocol_start(LPCWSTR url, BOOL is_first)
{
    static BOOL got_user_agent = FALSE;
    HRESULT hres;

    first_data_notif = TRUE;
    state = 0;

    SET_EXPECT(GetBindInfo);
    if (!(bindf & BINDF_FROMURLMON))
        SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(GetBindString_USER_AGENT);
    SET_EXPECT(GetBindString_ACCEPT_MIMES);
    SET_EXPECT(QueryService_HttpNegotiate);
    SET_EXPECT(BeginningTransaction);
    SET_EXPECT(GetRootSecurityId);
    if (http_post_test)
        SET_EXPECT(GetBindString_POST_COOKIE);

    hres = IInternetProtocol_Start(async_protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08x\n", hres);
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
    else todo_wine
    {
        /* user agent only retrieved once, even with different URLs */
        CHECK_NOT_CALLED(GetBindString_USER_AGENT);
    }
    CHECK_CALLED(GetBindString_ACCEPT_MIMES);
    CHECK_CALLED(QueryService_HttpNegotiate);
    CHECK_CALLED(BeginningTransaction);
    /* GetRootSecurityId called on WinXP but not on Win98 */
    CLEAR_CALLED(GetRootSecurityId);
    if (http_post_test)
        CHECK_CALLED(GetBindString_POST_COOKIE);

    return TRUE;
}

static void test_protocol_terminate(IInternetProtocol *protocol)
{
    BYTE buf[3600];
    DWORD cb;
    HRESULT hres;

    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08x\n", hres);

    hres = IInternetProtocol_Read(protocol, buf, 1, &cb);
    ok(hres == S_FALSE, "Read failed: %08x\n", hres);

    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08x\n", hres);

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
    ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
}

static void test_http_info(IInternetProtocol *protocol)
{
    IWinInetHttpInfo *info;
    HRESULT hres;

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IWinInetHttpInfo, (void**)&info);
    ok(hres == S_OK, "Could not get IWinInterHttpInfo iface: %08x\n", hres);

    /* TODO */

    IWinInetHttpInfo_Release(info);
}

/* is_first refers to whether this is the first call to this function
 * _for this url_ */
static void test_http_protocol_url(LPCWSTR url, BOOL is_https, BOOL is_first)
{
    IInternetProtocolInfo *protocol_info;
    IClassFactory *factory;
    IUnknown *unk;
    HRESULT hres;

    http_url = url;
    http_is_first = is_first;

    hres = CoGetClassObject(is_https ? &CLSID_HttpSProtocol : &CLSID_HttpProtocol,
            CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
        "Could not get IInternetProtocolInfo interface: %08x, expected E_NOINTERFACE\n",
        hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                        (void**)&async_protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        BYTE buf[3600];
        DWORD cb;
        ULONG ref;

        test_priority(async_protocol);
        test_http_info(async_protocol);

        SET_EXPECT(ReportProgress_FINDINGRESOURCE);
        SET_EXPECT(ReportProgress_CONNECTING);
        SET_EXPECT(ReportProgress_SENDINGREQUEST);
        SET_EXPECT(ReportProgress_PROXYDETECTING);
        if(! is_https)
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

        if(!http_protocol_start(url, is_first))
            return;

        SET_EXPECT(ReportResult);
        expect_hrResult = S_OK;

        hres = IInternetProtocol_Read(async_protocol, buf, 1, &cb);
        ok((hres == E_PENDING && cb==0) ||
           (hres == S_OK && cb==1), "Read failed: %08x (%d bytes)\n", hres, cb);

        WaitForSingleObject(event_complete, INFINITE);
        if(bindf & BINDF_FROMURLMON)
            CHECK_CALLED(Switch);
        else
            CHECK_CALLED(ReportData);
        if (is_https)
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
                   (hres == S_OK && cb==1), "Read failed: %08x (%d bytes)\n", hres, cb);
                WaitForSingleObject(event_complete, INFINITE);
                if(bindf & BINDF_FROMURLMON)
                    CHECK_CALLED(Switch);
                else
                    CHECK_CALLED(ReportData);
            }else {
                if(bindf & BINDF_FROMURLMON)
                    CHECK_NOT_CALLED(Switch);
                else
                    CHECK_NOT_CALLED(ReportData);
                if(cb == 0) break;
            }
        }
        ok(hres == S_FALSE, "Read failed: %08x\n", hres);
        CHECK_CALLED(ReportResult);
        if (is_https)
            CLEAR_CALLED(ReportProgress_SENDINGREQUEST);

        test_protocol_terminate(async_protocol);
        ref = IInternetProtocol_Release(async_protocol);
        ok(!ref, "ref=%x\n", hres);
    }

    IClassFactory_Release(factory);
}

static void test_http_protocol(void)
{
    static const WCHAR winehq_url[] =
        {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.',
            'o','r','g','/','s','i','t','e','/','a','b','o','u','t',0};
    static const WCHAR posttest_url[] =
        {'h','t','t','p',':','/','/','c','r','o','s','s','o','v','e','r','.',
         'c','o','d','e','w','e','a','v','e','r','s','.','c','o','m','/',
         'p','o','s','t','t','e','s','t','.','p','h','p',0};

    trace("Testing http protocol (not from urlmon)...\n");
    tested_protocol = HTTP_TEST;
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    test_http_protocol_url(winehq_url, FALSE, TRUE);

    trace("Testing http protocol (from urlmon)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON;
    test_http_protocol_url(winehq_url, FALSE, FALSE);

    trace("Testing http protocol (to file)...\n");
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NEEDFILE;
    test_http_protocol_url(winehq_url, FALSE, FALSE);

    trace("Testing http protocol (post data)...\n");
    http_post_test = TRUE;
    /* Without this flag we get a ReportProgress_CACHEFILENAMEAVAILABLE
     * notification with BINDVERB_POST */
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    test_http_protocol_url(posttest_url, FALSE, TRUE);
    http_post_test = FALSE;
}

static void test_https_protocol(void)
{
    static const WCHAR codeweavers_url[] =
        {'h','t','t','p','s',':','/','/','w','w','w','.','c','o','d','e','w','e','a','v','e','r','s',
         '.','c','o','m','/','t','e','s','t','.','h','t','m','l',0};

    trace("Testing https protocol (from urlmon)...\n");
    tested_protocol = HTTPS_TEST;
    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    test_http_protocol_url(codeweavers_url, TRUE, TRUE);
}


static void test_ftp_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IClassFactory *factory;
    IUnknown *unk;
    BYTE buf[4096];
    ULONG ref;
    DWORD cb;
    HRESULT hres;

    static const WCHAR ftp_urlW[] = {'f','t','p',':','/','/','f','t','p','.','w','i','n','e','h','q','.','o','r','g',
    '/','p','u','b','/','o','t','h','e','r','/',
    'w','i','n','e','l','o','g','o','.','x','c','f','.','t','a','r','.','b','z','2',0};

    trace("Testing ftp protocol...\n");

    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NOWRITECACHE;
    state = 0;
    tested_protocol = FTP_TEST;
    first_data_notif = TRUE;
    expect_hrResult = E_PENDING;

    hres = CoGetClassObject(&CLSID_FtpProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE, "Could not get IInternetProtocolInfo interface: %08x, expected E_NOINTERFACE\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                        (void**)&async_protocol);
    IClassFactory_Release(factory);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);

    test_priority(async_protocol);
    test_http_info(async_protocol);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(GetBindString_USER_AGENT);
    SET_EXPECT(ReportProgress_FINDINGRESOURCE);
    SET_EXPECT(ReportProgress_CONNECTING);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(Switch);

    hres = IInternetProtocol_Start(async_protocol, ftp_urlW, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08x\n", hres);
    CHECK_CALLED(GetBindInfo);
    todo_wine CHECK_NOT_CALLED(GetBindString_USER_AGENT);

    SET_EXPECT(ReportResult);

    hres = IInternetProtocol_Read(async_protocol, buf, 1, &cb);
    ok((hres == E_PENDING && cb==0) ||
       (hres == S_OK && cb==1), "Read failed: %08x (%d bytes)\n", hres, cb);

    WaitForSingleObject(event_complete, INFINITE);

    while(1) {

        hres = IInternetProtocol_Read(async_protocol, buf, sizeof(buf), &cb);
        if(hres == E_PENDING)
            WaitForSingleObject(event_complete, INFINITE);
        else
            if(cb == 0) break;
    }

    ok(hres == S_FALSE, "Read failed: %08x\n", hres);
    CHECK_CALLED(ReportResult);
    CHECK_CALLED(Switch);

    test_protocol_terminate(async_protocol);

    ref = IInternetProtocol_Release(async_protocol);
    ok(!ref, "ref=%d\n", ref);
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
       hres == REGDB_E_CLASSNOTREG, /* Gopher protocol has been removed as of Vista */
       "CoGetClassObject failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE, "Could not get IInternetProtocolInfo interface: %08x, expected E_NOINTERFACE\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                        (void**)&async_protocol);
    IClassFactory_Release(factory);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);

    test_priority(async_protocol);

    IInternetProtocol_Release(async_protocol);
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
    tested_protocol = MK_TEST;

    hres = CoGetClassObject(&CLSID_MkProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
        "Could not get IInternetProtocolInfo interface: %08x, expected E_NOINTERFACE\n",
        hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                        (void**)&protocol);
    IClassFactory_Release(factory);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);

    SET_EXPECT(GetBindInfo);
    hres = IInternetProtocol_Start(protocol, wrong_url1, &protocol_sink, &bind_info, 0, 0);
    ok(hres == MK_E_SYNTAX || hres == INET_E_INVALID_URL,
       "Start failed: %08x, expected MK_E_SYNTAX or INET_E_INVALID_URL\n", hres);
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
       "Start failed: %08x, expected INET_E_RESOURCE_NOT_FOUND or INET_E_INVALID_URL\n", hres);

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
    IInternetProtocolSink *sink;
    IInternetProtocol *protocol;
    IInternetPriority *priority;
    IInternetSession *session;
    LONG p;
    BYTE buf[1000];
    DWORD read;
    HRESULT hres;

    static const WCHAR test_url[] =
        {'t','e','s','t',':','/','/','f','i','l','e','.','h','t','m','l',0};
    static const WCHAR wsz_test[] = {'t','e','s','t',0};

    trace("Testing CreateBinding...\n");
    tested_protocol = BIND_TEST;
    binding_test = TRUE;

    hres = CoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08x\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &ClassFactory, &IID_NULL, wsz_test, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08x\n", hres);

    hres = IInternetSession_CreateBinding(session, NULL, test_url, NULL, NULL, &protocol, 0);
    binding_protocol = protocol;
    ok(hres == S_OK, "CreateBinding failed: %08x\n", hres);
    ok(protocol != NULL, "protocol == NULL\n");

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetBindInfo, (void**)&prot_bind_info);
    ok(hres == S_OK, "QueryInterface(IID_IInternetBindInfo) failed: %08x\n", hres);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetProtocolSink, (void**)&sink);
    ok(hres == S_OK, "Could not get IInternetProtocolSink: %08x\n", hres);

    hres = IInternetProtocol_Start(protocol, test_url, NULL, &bind_info, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08x, expected E_INVALIDARG\n", hres);
    hres = IInternetProtocol_Start(protocol, test_url, &protocol_sink, NULL, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08x, expected E_INVALIDARG\n", hres);
    hres = IInternetProtocol_Start(protocol, NULL, &protocol_sink, &bind_info, 0, 0);
    ok(hres == E_INVALIDARG, "Start failed: %08x, expected E_INVALIDARG\n", hres);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority, (void**)&priority);
    ok(hres == S_OK, "QueryInterface(IID_IInternetPriority) failed: %08x\n", hres);

    p = 0xdeadbeef;
    hres = IInternetPriority_GetPriority(priority, &p);
    ok(hres == S_OK, "GetPriority failed: %08x\n", hres);
    ok(!p, "p=%d\n", p);

    ex_priority = 100;
    hres = IInternetPriority_SetPriority(priority, 100);
    ok(hres == S_OK, "SetPriority failed: %08x\n", hres);

    p = 0xdeadbeef;
    hres = IInternetPriority_GetPriority(priority, &p);
    ok(hres == S_OK, "GetPriority failed: %08x\n", hres);
    ok(p == 100, "p=%d\n", p);

    SET_EXPECT(QueryService_InternetProtocol);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(ReportProgress_PROTOCOLCLASSID);
    SET_EXPECT(SetPriority);
    SET_EXPECT(Start);

    expect_hrResult = S_OK;
    hres = IInternetProtocol_Start(protocol, test_url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08x\n", hres);

    CHECK_CALLED(QueryService_InternetProtocol);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(ReportProgress_PROTOCOLCLASSID);
    CHECK_CALLED(SetPriority);
    CHECK_CALLED(Start);

    SET_EXPECT(Read);
    read = 0xdeadbeef;
    hres = IInternetProtocol_Read(protocol, expect_pv = buf, sizeof(buf), &read);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(read == 100, "read = %d\n", read);
    CHECK_CALLED(Read);

    SET_EXPECT(Read);
    read = 0xdeadbeef;
    hres = IInternetProtocol_Read(protocol, expect_pv = buf, sizeof(buf), &read);
    ok(hres == S_FALSE, "Read failed: %08x\n", hres);
    ok(!read, "read = %d\n", read);
    CHECK_CALLED(Read);

    p = 0xdeadbeef;
    hres = IInternetPriority_GetPriority(priority, &p);
    ok(hres == S_OK, "GetPriority failed: %08x\n", hres);
    ok(p == 100, "p=%d\n", p);

    hres = IInternetPriority_SetPriority(priority, 101);
    ok(hres == S_OK, "SetPriority failed: %08x\n", hres);

    SET_EXPECT(Terminate);
    hres = IInternetProtocol_Terminate(protocol, 0xdeadbeef);
    ok(hres == S_OK, "Terminate failed: %08x\n", hres);
    CHECK_CALLED(Terminate);

    SET_EXPECT(Continue);
    hres = IInternetProtocolSink_Switch(sink, &protocoldata);
    ok(hres == S_OK, "Switch failed: %08x\n", hres);
    CHECK_CALLED(Continue);

    hres = IInternetProtocolSink_ReportProgress(sink,
            BINDSTATUS_CACHEFILENAMEAVAILABLE, expect_wsz = emptyW);
    ok(hres == S_OK, "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08x\n", hres);

    hres = IInternetProtocolSink_ReportResult(sink, S_OK, ERROR_SUCCESS, NULL);
    ok(hres == E_FAIL, "ReportResult failed: %08x, expected E_FAIL\n", hres);

    hres = IInternetProtocolSink_ReportData(sink, 0, 0, 0);
    ok(hres == S_OK, "ReportData failed: %08x\n", hres);

    IInternetProtocolSink_Release(sink);
    IInternetPriority_Release(priority);
    IInternetBindInfo_Release(prot_bind_info);
    IInternetProtocol_Release(protocol);
    IInternetSession_Release(session);
}

static void test_binding(int prot)
{
    IInternetProtocol *protocol;
    IInternetSession *session;
    ULONG ref;
    HRESULT hres;

    trace("Testing %s binding...\n", debugstr_w(protocol_names[prot]));

    tested_protocol = prot;
    binding_test = TRUE;
    first_data_notif = TRUE;
    prot_read = 0;

    hres = CoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08x\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &ClassFactory, &IID_NULL, protocol_names[prot], 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08x\n", hres);

    hres = IInternetSession_CreateBinding(session, NULL, binding_urls[prot], NULL, NULL, &protocol, 0);
    binding_protocol = protocol;
    IInternetSession_Release(session);
    ok(hres == S_OK, "CreateBinding failed: %08x\n", hres);
    ok(protocol != NULL, "protocol == NULL\n");

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetBindInfo, (void**)&prot_bind_info);
    ok(hres == S_OK, "QueryInterface(IID_IInternetBindInfo) failed: %08x\n", hres);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetProtocolSink, (void**)&binding_sink);
    ok(hres == S_OK, "QueryInterface(IID_IInternetProtocolSink) failed: %08x\n", hres);

    ex_priority = 0;
    SET_EXPECT(QueryService_InternetProtocol);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(ReportProgress_PROTOCOLCLASSID);
    SET_EXPECT(SetPriority);
    SET_EXPECT(Start);

    expect_hrResult = S_OK;
    hres = IInternetProtocol_Start(protocol, binding_urls[prot], &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08x\n", hres);

    CHECK_CALLED(QueryService_InternetProtocol);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(ReportProgress_PROTOCOLCLASSID);
    CHECK_CALLED(SetPriority);
    CHECK_CALLED(Start);

    if(prot == HTTP_TEST || prot == HTTPS_TEST) {
        while(prot_state < 4) {
            WaitForSingleObject(event_complete, INFINITE);
            SET_EXPECT(Continue);
            IInternetProtocol_Continue(protocol, pdata);
            CHECK_CALLED(Continue);
            SetEvent(event_complete2);
        }

        WaitForSingleObject(event_complete, INFINITE);
    }else {
        SET_EXPECT(LockRequest);
        hres = IInternetProtocol_LockRequest(protocol, 0);
        ok(hres == S_OK, "LockRequest failed: %08x\n", hres);
        CHECK_CALLED(LockRequest);

        SET_EXPECT(UnlockRequest);
        hres = IInternetProtocol_UnlockRequest(protocol);
        ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
        CHECK_CALLED(UnlockRequest);
    }

    SET_EXPECT(Terminate);
    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08x\n", hres);
    CHECK_CALLED(Terminate);

    IInternetBindInfo_Release(prot_bind_info);
    IInternetProtocolSink_Release(binding_sink);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "ref=%u, expected 0\n", ref);
}

START_TEST(protocol)
{
    OleInitialize(NULL);

    event_complete = CreateEvent(NULL, FALSE, FALSE, NULL);
    event_complete2 = CreateEvent(NULL, FALSE, FALSE, NULL);

    test_file_protocol();
    test_http_protocol();
    test_https_protocol();
    test_ftp_protocol();
    test_gopher_protocol();
    test_mk_protocol();
    test_CreateBinding();
    test_binding(FILE_TEST);
    test_binding(HTTP_TEST);

    CloseHandle(event_complete);
    CloseHandle(event_complete2);

    OleUninitialize();
}
