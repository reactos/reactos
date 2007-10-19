/*
 * Copyright 2005 Jacek Caban
 * Copyright 2007 Misha Koshelev
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

/*
 * TODO:
 * - Handle redirects as native.
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "urlmon.h"
#include "wininet.h"
#include "urlmon_main.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

/* Flags are needed for, among other things, return HRESULTs from the Read function
 * to conform to native. For example, Read returns:
 *
 * 1. E_PENDING if called before the request has completed,
 *        (flags = 0)
 * 2. S_FALSE after all data has been read and S_OK has been reported,
 *        (flags = FLAG_REQUEST_COMPLETE | FLAG_ALL_DATA_READ | FLAG_RESULT_REPORTED)
 * 3. INET_E_DATA_NOT_AVAILABLE if InternetQueryDataAvailable fails. The first time
 *    this occurs, INET_E_DATA_NOT_AVAILABLE will also be reported to the sink,
 *        (flags = FLAG_REQUEST_COMPLETE)
 *    but upon subsequent calls to Read no reporting will take place, yet
 *    InternetQueryDataAvailable will still be called, and, on failure,
 *    INET_E_DATA_NOT_AVAILABLE will still be returned.
 *        (flags = FLAG_REQUEST_COMPLETE | FLAG_RESULT_REPORTED)
 *
 * FLAG_FIRST_DATA_REPORTED and FLAG_LAST_DATA_REPORTED are needed for proper
 * ReportData reporting. For example, if OnResponse returns S_OK, Continue will
 * report BSCF_FIRSTDATANOTIFICATION, and when all data has been read Read will
 * report BSCF_INTERMEDIATEDATANOTIFICATION|BSCF_LASTDATANOTIFICATION. However,
 * if OnResponse does not return S_OK, Continue will not report data, and Read
 * will report BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION when all
 * data has been read.
 */
#define FLAG_REQUEST_COMPLETE 0x1
#define FLAG_FIRST_CONTINUE_COMPLETE 0x2
#define FLAG_FIRST_DATA_REPORTED 0x4
#define FLAG_ALL_DATA_READ 0x8
#define FLAG_LAST_DATA_REPORTED 0x10
#define FLAG_RESULT_REPORTED 0x20

typedef struct {
    const IInternetProtocolVtbl *lpInternetProtocolVtbl;
    const IInternetPriorityVtbl *lpInternetPriorityVtbl;

    DWORD flags, grfBINDF;
    BINDINFO bind_info;
    IInternetProtocolSink *protocol_sink;
    IHttpNegotiate *http_negotiate;
    HINTERNET internet, connect, request;
    LPWSTR full_header;
    HANDLE lock;
    ULONG current_position, content_length, available_bytes;
    LONG priority;

    LONG ref;
} HttpProtocol;

/* Default headers from native */
static const WCHAR wszHeaders[] = {'A','c','c','e','p','t','-','E','n','c','o','d','i','n','g',
                                   ':',' ','g','z','i','p',',',' ','d','e','f','l','a','t','e',0};

/*
 * Helpers
 */

static void HTTPPROTOCOL_ReportResult(HttpProtocol *This, HRESULT hres)
{
    if (!(This->flags & FLAG_RESULT_REPORTED) &&
        This->protocol_sink)
    {
        This->flags |= FLAG_RESULT_REPORTED;
        IInternetProtocolSink_ReportResult(This->protocol_sink, hres, 0, NULL);
    }
}

static void HTTPPROTOCOL_ReportData(HttpProtocol *This)
{
    DWORD bscf;
    if (!(This->flags & FLAG_LAST_DATA_REPORTED) &&
        This->protocol_sink)
    {
        if (This->flags & FLAG_FIRST_DATA_REPORTED)
        {
            bscf = BSCF_INTERMEDIATEDATANOTIFICATION;
        }
        else
        {
            This->flags |= FLAG_FIRST_DATA_REPORTED;
            bscf = BSCF_FIRSTDATANOTIFICATION;
        }
        if (This->flags & FLAG_ALL_DATA_READ &&
            !(This->flags & FLAG_LAST_DATA_REPORTED))
        {
            This->flags |= FLAG_LAST_DATA_REPORTED;
            bscf |= BSCF_LASTDATANOTIFICATION;
        }
        IInternetProtocolSink_ReportData(This->protocol_sink, bscf,
                                         This->current_position+This->available_bytes,
                                         This->content_length);
    }
}

static void HTTPPROTOCOL_AllDataRead(HttpProtocol *This)
{
    if (!(This->flags & FLAG_ALL_DATA_READ))
        This->flags |= FLAG_ALL_DATA_READ;
    HTTPPROTOCOL_ReportData(This);
    HTTPPROTOCOL_ReportResult(This, S_OK);
}

static void HTTPPROTOCOL_Close(HttpProtocol *This)
{
    if (This->protocol_sink)
    {
        IInternetProtocolSink_Release(This->protocol_sink);
        This->protocol_sink = 0;
    }
    if (This->http_negotiate)
    {
        IHttpNegotiate_Release(This->http_negotiate);
        This->http_negotiate = 0;
    }
    if (This->request)
    {
        InternetCloseHandle(This->request);
        This->request = 0;
    }
    if (This->connect)
    {
        InternetCloseHandle(This->connect);
        This->connect = 0;
    }
    if (This->internet)
    {
        InternetCloseHandle(This->internet);
        This->internet = 0;
    }
    if (This->full_header)
    {
        if (This->full_header != wszHeaders)
            HeapFree(GetProcessHeap(), 0, This->full_header);
        This->full_header = 0;
    }
    if (This->bind_info.cbSize)
    {
        ReleaseBindInfo(&This->bind_info);
        memset(&This->bind_info, 0, sizeof(This->bind_info));
    }
    This->flags = 0;
}

static void CALLBACK HTTPPROTOCOL_InternetStatusCallback(
    HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus,
    LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
    HttpProtocol *This = (HttpProtocol *)dwContext;
    PROTOCOLDATA data;
    ULONG ulStatusCode;

    switch (dwInternetStatus)
    {
    case INTERNET_STATUS_RESOLVING_NAME:
        ulStatusCode = BINDSTATUS_FINDINGRESOURCE;
        break;
    case INTERNET_STATUS_CONNECTING_TO_SERVER:
        ulStatusCode = BINDSTATUS_CONNECTING;
        break;
    case INTERNET_STATUS_SENDING_REQUEST:
        ulStatusCode = BINDSTATUS_SENDINGREQUEST;
        break;
    case INTERNET_STATUS_REQUEST_COMPLETE:
        This->flags |= FLAG_REQUEST_COMPLETE;
        /* PROTOCOLDATA same as native */
        memset(&data, 0, sizeof(data));
        data.dwState = 0xf1000000;
        if (This->flags & FLAG_FIRST_CONTINUE_COMPLETE)
            data.pData = (LPVOID)BINDSTATUS_ENDDOWNLOADCOMPONENTS;
        else
            data.pData = (LPVOID)BINDSTATUS_DOWNLOADINGDATA;
        if (This->grfBINDF & BINDF_FROMURLMON)
            IInternetProtocolSink_Switch(This->protocol_sink, &data);
        else
            IInternetProtocol_Continue((IInternetProtocol *)This, &data);
        return;
    default:
        WARN("Unhandled Internet status callback %d\n", dwInternetStatus);
        return;
    }

    IInternetProtocolSink_ReportProgress(This->protocol_sink, ulStatusCode, (LPWSTR)lpvStatusInformation);
}

static inline LPWSTR strndupW(LPWSTR string, int len)
{
    LPWSTR ret = NULL;
    if (string &&
        (ret = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR))) != NULL)
    {
        memcpy(ret, string, len*sizeof(WCHAR));
        ret[len] = 0;
    }
    return ret;
}

/*
 * Interface implementations
 */

#define PROTOCOL(x)  ((IInternetProtocol*)  &(x)->lpInternetProtocolVtbl)
#define PRIORITY(x)  ((IInternetPriority*)  &(x)->lpInternetPriorityVtbl)

#define PROTOCOL_THIS(iface) DEFINE_THIS(HttpProtocol, InternetProtocol, iface)

static HRESULT WINAPI HttpProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = PROTOCOL(This);
    }else if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        TRACE("(%p)->(IID_IInternetPriority %p)\n", This, ppv);
        *ppv = PRIORITY(This);
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI HttpProtocol_AddRef(IInternetProtocol *iface)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI HttpProtocol_Release(IInternetProtocol *iface)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        HTTPPROTOCOL_Close(This);
        HeapFree(GetProcessHeap(), 0, This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI HttpProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    URL_COMPONENTSW url;
    DWORD len = 0, request_flags = INTERNET_FLAG_KEEP_CONNECTION;
    ULONG num = 0;
    IServiceProvider *service_provider = 0;
    IHttpNegotiate2 *http_negotiate2 = 0;
    LPWSTR host = 0, path = 0, user = 0, pass = 0, addl_header = 0,
        post_cookie = 0, optional = 0;
    BYTE security_id[512];
    LPOLESTR user_agent, accept_mimes[257];
    HRESULT hres;

    static const WCHAR wszHttp[] = {'h','t','t','p',':'};
    static const WCHAR wszBindVerb[BINDVERB_CUSTOM][5] =
        {{'G','E','T',0},
         {'P','O','S','T',0},
         {'P','U','T',0}};

    TRACE("(%p)->(%s %p %p %08x %d)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    memset(&This->bind_info, 0, sizeof(This->bind_info));
    This->bind_info.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &This->grfBINDF, &This->bind_info);
    if (hres != S_OK)
    {
        WARN("GetBindInfo failed: %08x\n", hres);
        goto done;
    }

    if (lstrlenW(szUrl) < sizeof(wszHttp)/sizeof(WCHAR)
        || memcmp(szUrl, wszHttp, sizeof(wszHttp)))
    {
        hres = MK_E_SYNTAX;
        goto done;
    }

    memset(&url, 0, sizeof(url));
    url.dwStructSize = sizeof(url);
    url.dwSchemeLength = url.dwHostNameLength = url.dwUrlPathLength = url.dwUserNameLength =
        url.dwPasswordLength = 1;
    if (!InternetCrackUrlW(szUrl, 0, 0, &url))
    {
        hres = MK_E_SYNTAX;
        goto done;
    }
    host = strndupW(url.lpszHostName, url.dwHostNameLength);
    path = strndupW(url.lpszUrlPath, url.dwUrlPathLength);
    user = strndupW(url.lpszUserName, url.dwUserNameLength);
    pass = strndupW(url.lpszPassword, url.dwPasswordLength);
    if (!url.nPort)
        url.nPort = INTERNET_DEFAULT_HTTP_PORT;

    if(!(This->grfBINDF & BINDF_FROMURLMON))
        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_DIRECTBIND, NULL);

    hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_USER_AGENT, &user_agent,
                                           1, &num);
    if (hres != S_OK || !num)
    {
        CHAR null_char = 0;
        LPSTR user_agenta = NULL;
        len = 0;
        if ((hres = ObtainUserAgentString(0, &null_char, &len)) != E_OUTOFMEMORY)
        {
            WARN("ObtainUserAgentString failed: %08x\n", hres);
        }
        else if (!(user_agenta = HeapAlloc(GetProcessHeap(), 0, len*sizeof(CHAR))))
        {
            WARN("Out of memory\n");
        }
        else if ((hres = ObtainUserAgentString(0, user_agenta, &len)) != S_OK)
        {
            WARN("ObtainUserAgentString failed: %08x\n", hres);
        }
        else
        {
            if (!(user_agent = CoTaskMemAlloc((len)*sizeof(WCHAR))))
                WARN("Out of memory\n");
            else
                MultiByteToWideChar(CP_ACP, 0, user_agenta, -1, user_agent, len*sizeof(WCHAR));
        }
        HeapFree(GetProcessHeap(), 0, user_agenta);
    }

    This->internet = InternetOpenW(user_agent, 0, NULL, NULL, INTERNET_FLAG_ASYNC);
    if (!This->internet)
    {
        WARN("InternetOpen failed: %d\n", GetLastError());
        hres = INET_E_NO_SESSION;
        goto done;
    }

    IInternetProtocolSink_AddRef(pOIProtSink);
    This->protocol_sink = pOIProtSink;

    /* Native does not check for success of next call, so we won't either */
    InternetSetStatusCallbackW(This->internet, HTTPPROTOCOL_InternetStatusCallback);

    This->connect = InternetConnectW(This->internet, host, url.nPort, user,
                                     pass, INTERNET_SERVICE_HTTP, 0, (DWORD)This);
    if (!This->connect)
    {
        WARN("InternetConnect failed: %d\n", GetLastError());
        hres = INET_E_CANNOT_CONNECT;
        goto done;
    }

    num = sizeof(accept_mimes)/sizeof(accept_mimes[0])-1;
    hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_ACCEPT_MIMES,
                                           accept_mimes,
                                           num, &num);
    if (hres != S_OK)
    {
        WARN("GetBindString BINDSTRING_ACCEPT_MIMES failed: %08x\n", hres);
        hres = INET_E_NO_VALID_MEDIA;
        goto done;
    }
    accept_mimes[num] = 0;

    if (This->grfBINDF & BINDF_NOWRITECACHE)
        request_flags |= INTERNET_FLAG_NO_CACHE_WRITE;
    This->request = HttpOpenRequestW(This->connect, This->bind_info.dwBindVerb < BINDVERB_CUSTOM ?
                                     wszBindVerb[This->bind_info.dwBindVerb] :
                                     This->bind_info.szCustomVerb,
                                     path, NULL, NULL, (LPCWSTR *)accept_mimes,
                                     request_flags, (DWORD)This);
    if (!This->request)
    {
        WARN("HttpOpenRequest failed: %d\n", GetLastError());
        hres = INET_E_RESOURCE_NOT_FOUND;
        goto done;
    }

    hres = IInternetProtocolSink_QueryInterface(pOIProtSink, &IID_IServiceProvider,
                                                (void **)&service_provider);
    if (hres != S_OK)
    {
        WARN("IInternetProtocolSink_QueryInterface IID_IServiceProvider failed: %08x\n", hres);
        goto done;
    }

    hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
                                         &IID_IHttpNegotiate, (void **)&This->http_negotiate);
    if (hres != S_OK)
    {
        WARN("IServiceProvider_QueryService IID_IHttpNegotiate failed: %08x\n", hres);
        goto done;
    }

    hres = IHttpNegotiate_BeginningTransaction(This->http_negotiate, szUrl, wszHeaders,
                                               0, &addl_header);
    if (hres != S_OK)
    {
        WARN("IHttpNegotiate_BeginningTransaction failed: %08x\n", hres);
        goto done;
    }
    else if (addl_header == NULL)
    {
        This->full_header = (LPWSTR)wszHeaders;
    }
    else
    {
        int len_addl_header = lstrlenW(addl_header);
        This->full_header = HeapAlloc(GetProcessHeap(), 0,
                                      len_addl_header*sizeof(WCHAR)+sizeof(wszHeaders));
        if (!This->full_header)
        {
            WARN("Out of memory\n");
            hres = E_OUTOFMEMORY;
            goto done;
        }
        lstrcpyW(This->full_header, addl_header);
        lstrcpyW(&This->full_header[len_addl_header], wszHeaders);
    }

    hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate2,
                                         &IID_IHttpNegotiate2, (void **)&http_negotiate2);
    if (hres != S_OK)
    {
        WARN("IServiceProvider_QueryService IID_IHttpNegotiate2 failed: %08x\n", hres);
        /* No goto done as per native */
    }
    else
    {
        len = sizeof(security_id)/sizeof(security_id[0]);
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, security_id, &len, 0);
        if (hres != S_OK)
        {
            WARN("IHttpNegotiate2_GetRootSecurityId failed: %08x\n", hres);
            /* No goto done as per native */
        }
    }

    /* FIXME: Handle security_id. Native calls undocumented function IsHostInProxyBypassList. */

    if (This->bind_info.dwBindVerb == BINDVERB_POST)
    {
        num = 0;
        hres = IInternetBindInfo_GetBindString(pOIBindInfo, BINDSTRING_POST_COOKIE, &post_cookie,
                                               1, &num);
        if (hres == S_OK && num &&
            !InternetSetOptionW(This->request, INTERNET_OPTION_SECONDARY_CACHE_KEY,
                                post_cookie, lstrlenW(post_cookie)))
        {
            WARN("InternetSetOption INTERNET_OPTION_SECONDARY_CACHE_KEY failed: %d\n",
                 GetLastError());
        }
    }

    if (This->bind_info.dwBindVerb != BINDVERB_GET)
    {
        /* Native does not use GlobalLock/GlobalUnlock, so we won't either */
        if (This->bind_info.stgmedData.tymed != TYMED_HGLOBAL)
            WARN("Expected This->bind_info.stgmedData.tymed to be TYMED_HGLOBAL, not %d\n",
                 This->bind_info.stgmedData.tymed);
        else
            optional = (LPWSTR)This->bind_info.stgmedData.hGlobal;
    }
    if (!HttpSendRequestW(This->request, This->full_header, lstrlenW(This->full_header),
                          optional,
                          optional ? This->bind_info.cbstgmedData : 0) &&
        GetLastError() != ERROR_IO_PENDING)
    {
        WARN("HttpSendRequest failed: %d\n", GetLastError());
        hres = INET_E_DOWNLOAD_FAILURE;
        goto done;
    }

    hres = S_OK;
done:
    if (hres != S_OK)
    {
        IInternetProtocolSink_ReportResult(pOIProtSink, hres, 0, NULL);
        HTTPPROTOCOL_Close(This);
    }

    CoTaskMemFree(post_cookie);
    CoTaskMemFree(addl_header);
    if (http_negotiate2)
        IHttpNegotiate2_Release(http_negotiate2);
    if (service_provider)
        IServiceProvider_Release(service_provider);

    while (num<sizeof(accept_mimes)/sizeof(accept_mimes[0]) &&
           accept_mimes[num])
        CoTaskMemFree(accept_mimes[num++]);
    CoTaskMemFree(user_agent);

    HeapFree(GetProcessHeap(), 0, pass);
    HeapFree(GetProcessHeap(), 0, user);
    HeapFree(GetProcessHeap(), 0, path);
    HeapFree(GetProcessHeap(), 0, host);

    return hres;
}

static HRESULT WINAPI HttpProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    DWORD len = sizeof(DWORD), status_code;
    LPWSTR response_headers = 0, content_type = 0, content_length = 0;

    static const WCHAR wszDefaultContentType[] =
        {'t','e','x','t','/','h','t','m','l',0};

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    if (!pProtocolData)
    {
        WARN("Expected pProtocolData to be non-NULL\n");
        return S_OK;
    }
    else if (!This->request)
    {
        WARN("Expected request to be non-NULL\n");
        return S_OK;
    }
    else if (!This->http_negotiate)
    {
        WARN("Expected IHttpNegotiate pointer to be non-NULL\n");
        return S_OK;
    }
    else if (!This->protocol_sink)
    {
        WARN("Expected IInternetProtocolSink pointer to be non-NULL\n");
        return S_OK;
    }

    if (pProtocolData->pData == (LPVOID)BINDSTATUS_DOWNLOADINGDATA)
    {
        if (!HttpQueryInfoW(This->request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                            &status_code, &len, NULL))
        {
            WARN("HttpQueryInfo failed: %d\n", GetLastError());
        }
        else
        {
            len = 0;
            if ((!HttpQueryInfoW(This->request, HTTP_QUERY_RAW_HEADERS_CRLF, response_headers, &len,
                                 NULL) &&
                 GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
                !(response_headers = HeapAlloc(GetProcessHeap(), 0, len)) ||
                !HttpQueryInfoW(This->request, HTTP_QUERY_RAW_HEADERS_CRLF, response_headers, &len,
                                NULL))
            {
                WARN("HttpQueryInfo failed: %d\n", GetLastError());
            }
            else
            {
                HRESULT hres = IHttpNegotiate_OnResponse(This->http_negotiate, status_code,
                                                         response_headers, NULL, NULL);
                if (hres != S_OK)
                {
                    WARN("IHttpNegotiate_OnResponse failed: %08x\n", hres);
                    goto done;
                }
            }
        }

        len = 0;
        if ((!HttpQueryInfoW(This->request, HTTP_QUERY_CONTENT_TYPE, content_type, &len, NULL) &&
             GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
            !(content_type = HeapAlloc(GetProcessHeap(), 0, len)) ||
            !HttpQueryInfoW(This->request, HTTP_QUERY_CONTENT_TYPE, content_type, &len, NULL))
        {
            WARN("HttpQueryInfo failed: %d\n", GetLastError());
            IInternetProtocolSink_ReportProgress(This->protocol_sink,
                                                 (This->grfBINDF & BINDF_FROMURLMON) ?
                                                 BINDSTATUS_MIMETYPEAVAILABLE :
                                                 BINDSTATUS_RAWMIMETYPE,
                                                 wszDefaultContentType);
        }
        else
        {
            IInternetProtocolSink_ReportProgress(This->protocol_sink,
                                                 (This->grfBINDF & BINDF_FROMURLMON) ?
                                                 BINDSTATUS_MIMETYPEAVAILABLE :
                                                 BINDSTATUS_RAWMIMETYPE,
                                                 content_type);
        }

        len = 0;
        if ((!HttpQueryInfoW(This->request, HTTP_QUERY_CONTENT_LENGTH, content_length, &len, NULL) &&
             GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
            !(content_length = HeapAlloc(GetProcessHeap(), 0, len)) ||
            !HttpQueryInfoW(This->request, HTTP_QUERY_CONTENT_LENGTH, content_length, &len, NULL))
        {
            WARN("HttpQueryInfo failed: %d\n", GetLastError());
            This->content_length = 0;
        }
        else
        {
            This->content_length = atoiW(content_length);
        }

        This->flags |= FLAG_FIRST_CONTINUE_COMPLETE;
    }

    if (pProtocolData->pData >= (LPVOID)BINDSTATUS_DOWNLOADINGDATA)
    {
        if (!InternetQueryDataAvailable(This->request, &This->available_bytes, 0, 0))
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                This->flags &= ~FLAG_REQUEST_COMPLETE;
            }
            else
            {
                WARN("InternetQueryDataAvailable failed: %d\n", GetLastError());
                HTTPPROTOCOL_ReportResult(This, INET_E_DATA_NOT_AVAILABLE);
            }
        }
        else
        {
            HTTPPROTOCOL_ReportData(This);
        }
    }

done:
    HeapFree(GetProcessHeap(), 0, response_headers);
    HeapFree(GetProcessHeap(), 0, content_type);
    HeapFree(GetProcessHeap(), 0, content_length);

    /* Returns S_OK on native */
    return S_OK;
}

static HRESULT WINAPI HttpProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);
    HTTPPROTOCOL_Close(This);

    return S_OK;
}

static HRESULT WINAPI HttpProtocol_Suspend(IInternetProtocol *iface)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Resume(IInternetProtocol *iface)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    ULONG read = 0, len = 0;
    HRESULT hres = S_FALSE;

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if (!(This->flags & FLAG_REQUEST_COMPLETE))
    {
        hres = E_PENDING;
    }
    else while (!(This->flags & FLAG_ALL_DATA_READ) &&
                read < cb)
    {
        if (This->available_bytes == 0)
        {
            /* InternetQueryDataAvailable may immediately fork and perform its asynchronous
             * read, so clear the flag _before_ calling so it does not incorrectly get cleared
             * after the status callback is called */
            This->flags &= ~FLAG_REQUEST_COMPLETE;
            if (!InternetQueryDataAvailable(This->request, &This->available_bytes, 0, 0))
            {
                if (GetLastError() == ERROR_IO_PENDING)
                {
                    hres = E_PENDING;
                }
                else
                {
                    WARN("InternetQueryDataAvailable failed: %d\n", GetLastError());
                    hres = INET_E_DATA_NOT_AVAILABLE;
                    HTTPPROTOCOL_ReportResult(This, hres);
                }
                goto done;
            }
            else if (This->available_bytes == 0)
            {
                HTTPPROTOCOL_AllDataRead(This);
            }
        }
        else
        {
            if (!InternetReadFile(This->request, ((BYTE *)pv)+read,
                                  This->available_bytes > cb-read ?
                                  cb-read : This->available_bytes, &len))
            {
                WARN("InternetReadFile failed: %d\n", GetLastError());
                hres = INET_E_DOWNLOAD_FAILURE;
                HTTPPROTOCOL_ReportResult(This, hres);
                goto done;
            }
            else if (len == 0)
            {
                HTTPPROTOCOL_AllDataRead(This);
            }
            else
            {
                read += len;
                This->current_position += len;
                This->available_bytes -= len;
            }
        }
    }

    /* Per MSDN this should be if (read == cb), but native returns S_OK
     * if any bytes were read, so we will too */
    if (read)
        hres = S_OK;

done:
    if (pcbRead)
        *pcbRead = read;

    if (hres != E_PENDING)
        This->flags |= FLAG_REQUEST_COMPLETE;

    return hres;
}

static HRESULT WINAPI HttpProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    if (!InternetLockRequestFile(This->request, &This->lock))
        WARN("InternetLockRequest failed: %d\n", GetLastError());

    return S_OK;
}

static HRESULT WINAPI HttpProtocol_UnlockRequest(IInternetProtocol *iface)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)\n", This);

    if (This->lock)
    {
        if (!InternetUnlockRequestFile(This->lock))
            WARN("InternetUnlockRequest failed: %d\n", GetLastError());
        This->lock = 0;
    }

    return S_OK;
}

#undef PROTOCOL_THIS

#define PRIORITY_THIS(iface) DEFINE_THIS(HttpProtocol, InternetPriority, iface)

static HRESULT WINAPI HttpPriority_QueryInterface(IInternetPriority *iface, REFIID riid, void **ppv)
{
    HttpProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI HttpPriority_AddRef(IInternetPriority *iface)
{
    HttpProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI HttpPriority_Release(IInternetPriority *iface)
{
    HttpProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI HttpPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    HttpProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%d)\n", This, nPriority);

    This->priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI HttpPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    HttpProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->priority;
    return S_OK;
}

#undef PRIORITY_THIS

static const IInternetPriorityVtbl HttpPriorityVtbl = {
    HttpPriority_QueryInterface,
    HttpPriority_AddRef,
    HttpPriority_Release,
    HttpPriority_SetPriority,
    HttpPriority_GetPriority
};

static const IInternetProtocolVtbl HttpProtocolVtbl = {
    HttpProtocol_QueryInterface,
    HttpProtocol_AddRef,
    HttpProtocol_Release,
    HttpProtocol_Start,
    HttpProtocol_Continue,
    HttpProtocol_Abort,
    HttpProtocol_Terminate,
    HttpProtocol_Suspend,
    HttpProtocol_Resume,
    HttpProtocol_Read,
    HttpProtocol_Seek,
    HttpProtocol_LockRequest,
    HttpProtocol_UnlockRequest
};

HRESULT HttpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    HttpProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(HttpProtocol));

    ret->lpInternetProtocolVtbl = &HttpProtocolVtbl;
    ret->lpInternetPriorityVtbl = &HttpPriorityVtbl;
    ret->flags = ret->grfBINDF = 0;
    memset(&ret->bind_info, 0, sizeof(ret->bind_info));
    ret->protocol_sink = 0;
    ret->http_negotiate = 0;
    ret->internet = ret->connect = ret->request = 0;
    ret->full_header = 0;
    ret->lock = 0;
    ret->current_position = ret->content_length = ret->available_bytes = 0;
    ret->priority = 0;
    ret->ref = 1;

    *ppobj = PROTOCOL(ret);

    return S_OK;
}
