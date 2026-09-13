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

#include "urlmon_main.h"
#include "wininet.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    Protocol base;

    IUnknown            IUnknown_inner;
    IInternetProtocolEx IInternetProtocolEx_iface;
    IInternetPriority   IInternetPriority_iface;
    IWinInetHttpInfo    IWinInetHttpInfo_iface;

    BOOL https;
    IHttpNegotiate *http_negotiate;
    WCHAR *full_header;

    LONG ref;
    IUnknown *outer;
} HttpProtocol;

static inline HttpProtocol *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, HttpProtocol, IUnknown_inner);
}

static inline HttpProtocol *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, HttpProtocol, IInternetProtocolEx_iface);
}

static inline HttpProtocol *impl_from_IInternetPriority(IInternetPriority *iface)
{
    return CONTAINING_RECORD(iface, HttpProtocol, IInternetPriority_iface);
}

static inline HttpProtocol *impl_from_IWinInetHttpInfo(IWinInetHttpInfo *iface)
{
    return CONTAINING_RECORD(iface, HttpProtocol, IWinInetHttpInfo_iface);
}

static const WCHAR default_headersW[] = L"Accept-Encoding: gzip, deflate";

static LPWSTR query_http_info(HttpProtocol *This, DWORD option)
{
    LPWSTR ret = NULL;
    DWORD len = 0;
    BOOL res;

    res = HttpQueryInfoW(This->base.request, option, NULL, &len, NULL);
    if (!res && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        ret = malloc(len);
        res = HttpQueryInfoW(This->base.request, option, ret, &len, NULL);
    }
    if(!res) {
        TRACE("HttpQueryInfoW(%ld) failed: %08lx\n", option, GetLastError());
        free(ret);
        return NULL;
    }

    return ret;
}

static inline BOOL set_security_flag(HttpProtocol *This, DWORD flags)
{
    BOOL res;

    res = InternetSetOptionW(This->base.request, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
    if(!res)
        ERR("Failed to set security flags: %lx\n", flags);

    return res;
}

static inline HRESULT internet_error_to_hres(DWORD error)
{
    switch(error)
    {
    case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
    case ERROR_INTERNET_SEC_CERT_CN_INVALID:
    case ERROR_INTERNET_INVALID_CA:
    case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED:
    case ERROR_INTERNET_SEC_INVALID_CERT:
    case ERROR_INTERNET_SEC_CERT_ERRORS:
    case ERROR_INTERNET_SEC_CERT_REV_FAILED:
    case ERROR_INTERNET_SEC_CERT_NO_REV:
    case ERROR_INTERNET_SEC_CERT_REVOKED:
        return INET_E_INVALID_CERTIFICATE;
    case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
    case ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR:
    case ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION:
        return INET_E_REDIRECT_FAILED;
    default:
        return INET_E_DOWNLOAD_FAILURE;
    }
}

static HRESULT handle_http_error(HttpProtocol *This, DWORD error)
{
    IServiceProvider *serv_prov;
    IWindowForBindingUI *wfb_ui;
    IHttpSecurity *http_security;
    BOOL security_problem;
    DWORD dlg_flags;
    HWND hwnd;
    DWORD res;
    HRESULT hres;

    TRACE("(%p %lu)\n", This, error);

    switch(error) {
    case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
    case ERROR_INTERNET_SEC_CERT_CN_INVALID:
    case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
    case ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR:
    case ERROR_INTERNET_INVALID_CA:
    case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED:
    case ERROR_INTERNET_SEC_INVALID_CERT:
    case ERROR_INTERNET_SEC_CERT_ERRORS:
    case ERROR_INTERNET_SEC_CERT_REV_FAILED:
    case ERROR_INTERNET_SEC_CERT_NO_REV:
    case ERROR_INTERNET_SEC_CERT_REVOKED:
    case ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION:
        security_problem = TRUE;
        break;
    default:
        security_problem = FALSE;
    }

    hres = IInternetProtocolSink_QueryInterface(This->base.protocol_sink, &IID_IServiceProvider,
                                                (void**)&serv_prov);
    if(FAILED(hres)) {
        ERR("Failed to get IServiceProvider.\n");
        return E_ABORT;
    }

    if(security_problem) {
        hres = IServiceProvider_QueryService(serv_prov, &IID_IHttpSecurity, &IID_IHttpSecurity,
                                             (void**)&http_security);
        if(SUCCEEDED(hres)) {
            hres = IHttpSecurity_OnSecurityProblem(http_security, error);
            IHttpSecurity_Release(http_security);

            TRACE("OnSecurityProblem returned %08lx\n", hres);

            if(hres != S_FALSE)
            {
                BOOL res = FALSE;

                IServiceProvider_Release(serv_prov);

                if(hres == S_OK) {
                    if(error == ERROR_INTERNET_SEC_CERT_DATE_INVALID)
                        res = set_security_flag(This, SECURITY_FLAG_IGNORE_CERT_DATE_INVALID);
                    else if(error == ERROR_INTERNET_SEC_CERT_CN_INVALID)
                        res = set_security_flag(This, SECURITY_FLAG_IGNORE_CERT_CN_INVALID);
                    else if(error == ERROR_INTERNET_INVALID_CA)
                        res = set_security_flag(This, SECURITY_FLAG_IGNORE_UNKNOWN_CA);

                    if(res)
                        return RPC_E_RETRY;

                    FIXME("Don't know how to ignore error %ld\n", error);
                    return E_ABORT;
                }

                if(hres == E_ABORT)
                    return E_ABORT;
                if(hres == RPC_E_RETRY)
                    return RPC_E_RETRY;

                return internet_error_to_hres(error);
            }
        }
    }

    switch(error) {
    case ERROR_INTERNET_SEC_CERT_REV_FAILED:
        if(hres != S_FALSE) {
            /* Silently ignore the error. We will get more detailed error from wininet anyway. */
            set_security_flag(This, SECURITY_FLAG_IGNORE_REVOCATION);
            hres = RPC_E_RETRY;
            break;
        }
        /* fallthrough */
    default:
        hres = IServiceProvider_QueryService(serv_prov, &IID_IWindowForBindingUI, &IID_IWindowForBindingUI, (void**)&wfb_ui);
        if(SUCCEEDED(hres)) {
            const IID *iid_reason;

            if(security_problem)
                iid_reason = &IID_IHttpSecurity;
            else if(error == ERROR_INTERNET_INCORRECT_PASSWORD)
                iid_reason = &IID_IAuthenticate;
            else
                iid_reason = &IID_IWindowForBindingUI;

            hres = IWindowForBindingUI_GetWindow(wfb_ui, iid_reason, &hwnd);
            IWindowForBindingUI_Release(wfb_ui);
        }

        if(FAILED(hres)) hwnd = NULL;

        dlg_flags = FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA;
        if(This->base.bindf & BINDF_NO_UI)
            dlg_flags |= FLAGS_ERROR_UI_FLAGS_NO_UI;

        res = InternetErrorDlg(hwnd, This->base.request, error, dlg_flags, NULL);
        hres = res == ERROR_INTERNET_FORCE_RETRY || res == ERROR_SUCCESS ? RPC_E_RETRY : internet_error_to_hres(error);
    }

    IServiceProvider_Release(serv_prov);
    return hres;
}

static ULONG send_http_request(HttpProtocol *This)
{
    INTERNET_BUFFERSW send_buffer = {sizeof(INTERNET_BUFFERSW)};
    BOOL res;

    send_buffer.lpcszHeader = This->full_header;
    send_buffer.dwHeadersLength = send_buffer.dwHeadersTotal = lstrlenW(This->full_header);

    if(This->base.bind_info.dwBindVerb != BINDVERB_GET) {
        switch(This->base.bind_info.stgmedData.tymed) {
        case TYMED_HGLOBAL:
            /* Native does not use GlobalLock/GlobalUnlock, so we won't either */
            send_buffer.lpvBuffer = This->base.bind_info.stgmedData.hGlobal;
            send_buffer.dwBufferLength = send_buffer.dwBufferTotal = This->base.bind_info.cbstgmedData;
            break;
        case TYMED_ISTREAM: {
            LARGE_INTEGER offset;

            send_buffer.dwBufferTotal = This->base.bind_info.cbstgmedData;
            if(!This->base.post_stream) {
                This->base.post_stream = This->base.bind_info.stgmedData.pstm;
                IStream_AddRef(This->base.post_stream);
            }

            offset.QuadPart = 0;
            IStream_Seek(This->base.post_stream, offset, STREAM_SEEK_SET, NULL);
            break;
        }
        default:
            FIXME("Unsupported This->base.bind_info.stgmedData.tymed %ld\n", This->base.bind_info.stgmedData.tymed);
        }
    }

    if(This->base.post_stream)
        res = HttpSendRequestExW(This->base.request, &send_buffer, NULL, 0, 0);
    else
        res = HttpSendRequestW(This->base.request, send_buffer.lpcszHeader, send_buffer.dwHeadersLength,
                send_buffer.lpvBuffer, send_buffer.dwBufferLength);

    return res ? 0 : GetLastError();
}

static inline HttpProtocol *impl_from_Protocol(Protocol *prot)
{
    return CONTAINING_RECORD(prot, HttpProtocol, base);
}

static HRESULT HttpProtocol_open_request(Protocol *prot, IUri *uri, DWORD request_flags,
        HINTERNET internet_session, IInternetBindInfo *bind_info)
{
    HttpProtocol *This = impl_from_Protocol(prot);
    WCHAR *addl_header = NULL, *post_cookie = NULL, *rootdoc_url = NULL;
    IServiceProvider *service_provider = NULL;
    IHttpNegotiate2 *http_negotiate2 = NULL;
    BSTR url, host, user, pass, path;
    LPOLESTR accept_mimes[257];
    const WCHAR **accept_types;
    BYTE security_id[512];
    DWORD len, port, flags;
    ULONG num, error;
    BOOL res, b;
    HRESULT hres;

    static const WCHAR wszBindVerb[BINDVERB_CUSTOM][5] =
        {L"GET",
         L"POST",
         L"PUT"};

    hres = IUri_GetPort(uri, &port);
    if(FAILED(hres))
        return hres;

    hres = IUri_GetHost(uri, &host);
    if(FAILED(hres))
        return hres;

    hres = IUri_GetUserName(uri, &user);
    if(SUCCEEDED(hres)) {
        hres = IUri_GetPassword(uri, &pass);

        if(SUCCEEDED(hres)) {
            This->base.connection = InternetConnectW(internet_session, host, port, user, pass,
                    INTERNET_SERVICE_HTTP, This->https ? INTERNET_FLAG_SECURE : 0, (DWORD_PTR)&This->base);
            SysFreeString(pass);
        }
        SysFreeString(user);
    }
    SysFreeString(host);
    if(FAILED(hres))
        return hres;
    if(!This->base.connection) {
        WARN("InternetConnect failed: %ld\n", GetLastError());
        return INET_E_CANNOT_CONNECT;
    }

    num = 0;
    hres = IInternetBindInfo_GetBindString(bind_info, BINDSTRING_ROOTDOC_URL, &rootdoc_url, 1, &num);
    if(hres == S_OK && num) {
        FIXME("Use root doc URL %s\n", debugstr_w(rootdoc_url));
        CoTaskMemFree(rootdoc_url);
    }

    num = ARRAY_SIZE(accept_mimes) - 1;
    hres = IInternetBindInfo_GetBindString(bind_info, BINDSTRING_ACCEPT_MIMES, accept_mimes, num, &num);
    if(hres == INET_E_USE_DEFAULT_SETTING) {
        static const WCHAR *default_accept_mimes[] = {L"*/*", NULL};

        accept_types = default_accept_mimes;
        num = 0;
    }else if(hres == S_OK) {
        accept_types = (const WCHAR**)accept_mimes;
    }else {
        WARN("GetBindString BINDSTRING_ACCEPT_MIMES failed: %08lx\n", hres);
        return INET_E_NO_VALID_MEDIA;
    }
    accept_mimes[num] = 0;

    if(This->https)
        request_flags |= INTERNET_FLAG_SECURE;

    hres = IUri_GetPathAndQuery(uri, &path);
    if(SUCCEEDED(hres)) {
        This->base.request = HttpOpenRequestW(This->base.connection,
                This->base.bind_info.dwBindVerb < BINDVERB_CUSTOM
                    ? wszBindVerb[This->base.bind_info.dwBindVerb] : This->base.bind_info.szCustomVerb,
                path, NULL, NULL, accept_types, request_flags, (DWORD_PTR)&This->base);
        SysFreeString(path);
    }
    while(num--)
        CoTaskMemFree(accept_mimes[num]);
    if(FAILED(hres))
        return hres;
    if (!This->base.request) {
        WARN("HttpOpenRequest failed: %ld\n", GetLastError());
        return INET_E_RESOURCE_NOT_FOUND;
    }

    hres = IInternetProtocolSink_QueryInterface(This->base.protocol_sink, &IID_IServiceProvider,
            (void **)&service_provider);
    if (hres != S_OK) {
        WARN("IInternetProtocolSink_QueryInterface IID_IServiceProvider failed: %08lx\n", hres);
        return hres;
    }

    hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate,
            &IID_IHttpNegotiate, (void **)&This->http_negotiate);
    if (hres != S_OK) {
        WARN("IServiceProvider_QueryService IID_IHttpNegotiate failed: %08lx\n", hres);
        IServiceProvider_Release(service_provider);
        return hres;
    }

    hres = IUri_GetAbsoluteUri(uri, &url);
    if(FAILED(hres)) {
        IServiceProvider_Release(service_provider);
        return hres;
    }

    hres = IHttpNegotiate_BeginningTransaction(This->http_negotiate, url, default_headersW,
            0, &addl_header);
    SysFreeString(url);
    if(hres != S_OK) {
        WARN("IHttpNegotiate_BeginningTransaction failed: %08lx\n", hres);
        IServiceProvider_Release(service_provider);
        return hres;
    }

    len = addl_header ? lstrlenW(addl_header) : 0;

    This->full_header = malloc(len * sizeof(WCHAR) + sizeof(default_headersW));
    if(!This->full_header) {
        IServiceProvider_Release(service_provider);
        return E_OUTOFMEMORY;
    }

    if(len)
        memcpy(This->full_header, addl_header, len*sizeof(WCHAR));
    CoTaskMemFree(addl_header);
    memcpy(This->full_header+len, default_headersW, sizeof(default_headersW));

    hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate2,
            &IID_IHttpNegotiate2, (void **)&http_negotiate2);
    IServiceProvider_Release(service_provider);
    if(hres != S_OK) {
        WARN("IServiceProvider_QueryService IID_IHttpNegotiate2 failed: %08lx\n", hres);
        /* No goto done as per native */
    }else {
        len = ARRAY_SIZE(security_id);
        hres = IHttpNegotiate2_GetRootSecurityId(http_negotiate2, security_id, &len, 0);
        IHttpNegotiate2_Release(http_negotiate2);
        if (hres != S_OK)
            WARN("IHttpNegotiate2_GetRootSecurityId failed: %08lx\n", hres);
    }

    /* FIXME: Handle security_id. Native calls undocumented function IsHostInProxyBypassList. */

    if(This->base.bind_info.dwBindVerb == BINDVERB_POST) {
        num = 0;
        hres = IInternetBindInfo_GetBindString(bind_info, BINDSTRING_POST_COOKIE, &post_cookie, 1, &num);
        if(hres == S_OK && num) {
            if(!InternetSetOptionW(This->base.request, INTERNET_OPTION_SECONDARY_CACHE_KEY,
                                   post_cookie, lstrlenW(post_cookie)))
                WARN("InternetSetOption INTERNET_OPTION_SECONDARY_CACHE_KEY failed: %ld\n", GetLastError());
            CoTaskMemFree(post_cookie);
        }
    }

    flags = INTERNET_ERROR_MASK_COMBINED_SEC_CERT;
    res = InternetSetOptionW(This->base.request, INTERNET_OPTION_ERROR_MASK, &flags, sizeof(flags));
    if(!res)
        WARN("InternetSetOption(INTERNET_OPTION_ERROR_MASK) failed: %lu\n", GetLastError());

    b = TRUE;
    res = InternetSetOptionW(This->base.request, INTERNET_OPTION_HTTP_DECODING, &b, sizeof(b));
    if(!res)
        WARN("InternetSetOption(INTERNET_OPTION_HTTP_DECODING) failed: %lu\n", GetLastError());

    do {
        error = send_http_request(This);

        switch(error) {
        case ERROR_IO_PENDING:
            return S_OK;
        case ERROR_SUCCESS:
            /*
             * If sending response ended synchronously, it means that we have the whole data
             * available locally (most likely in cache).
             */
            return protocol_syncbinding(&This->base);
        default:
            hres = handle_http_error(This, error);
        }
    } while(hres == RPC_E_RETRY);

    WARN("HttpSendRequest failed: %ld\n", error);
    return hres;
}

static HRESULT HttpProtocol_end_request(Protocol *protocol)
{
    BOOL res;

    res = HttpEndRequestW(protocol->request, NULL, 0, 0);
    if(!res && GetLastError() != ERROR_IO_PENDING) {
        FIXME("HttpEndRequest failed: %lu\n", GetLastError());
        return E_FAIL;
    }

    return S_OK;
}

static BOOL is_redirect_response(DWORD status_code)
{
    switch(status_code) {
    case HTTP_STATUS_REDIRECT:
    case HTTP_STATUS_MOVED:
    case HTTP_STATUS_REDIRECT_KEEP_VERB:
    case HTTP_STATUS_REDIRECT_METHOD:
        return TRUE;
    }
    return FALSE;
}

static HRESULT HttpProtocol_start_downloading(Protocol *prot)
{
    HttpProtocol *This = impl_from_Protocol(prot);
    LPWSTR content_type, content_length, ranges;
    DWORD len = sizeof(DWORD);
    DWORD status_code;
    BOOL res;
    HRESULT hres;

    if(!This->http_negotiate) {
        WARN("Expected IHttpNegotiate pointer to be non-NULL\n");
        return S_OK;
    }

    res = HttpQueryInfoW(This->base.request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &status_code, &len, NULL);
    if(res) {
        WCHAR *response_headers;

        if((This->base.bind_info.dwOptions & BINDINFO_OPTIONS_DISABLEAUTOREDIRECTS) && is_redirect_response(status_code)) {
            WCHAR *location;

            TRACE("Got redirect with disabled auto redirects\n");

            location = query_http_info(This, HTTP_QUERY_LOCATION);
            This->base.flags |= FLAG_RESULT_REPORTED | FLAG_LAST_DATA_REPORTED;
            IInternetProtocolSink_ReportResult(This->base.protocol_sink, INET_E_REDIRECT_FAILED, 0, location);
            free(location);
            return INET_E_REDIRECT_FAILED;
        }

        response_headers = query_http_info(This, HTTP_QUERY_RAW_HEADERS_CRLF);
        if(response_headers) {
            hres = IHttpNegotiate_OnResponse(This->http_negotiate, status_code, response_headers,
                    NULL, NULL);
            free(response_headers);
            if (hres != S_OK) {
                WARN("IHttpNegotiate_OnResponse failed: %08lx\n", hres);
                return S_OK;
            }
        }
    }else {
        WARN("HttpQueryInfo failed: %ld\n", GetLastError());
    }

    ranges = query_http_info(This, HTTP_QUERY_ACCEPT_RANGES);
    if(ranges) {
        IInternetProtocolSink_ReportProgress(This->base.protocol_sink, BINDSTATUS_ACCEPTRANGES, NULL);
        free(ranges);
    }

    content_type = query_http_info(This, HTTP_QUERY_CONTENT_TYPE);
    if(content_type) {
        /* remove the charset, if present */
        LPWSTR p = wcschr(content_type, ';');
        if (p) *p = '\0';

        IInternetProtocolSink_ReportProgress(This->base.protocol_sink,
                (This->base.bindf & BINDF_FROMURLMON)
                 ? BINDSTATUS_MIMETYPEAVAILABLE : BINDSTATUS_RAWMIMETYPE,
                 content_type);
        free(content_type);
    }else {
        WARN("HttpQueryInfo failed: %ld\n", GetLastError());
        IInternetProtocolSink_ReportProgress(This->base.protocol_sink,
                 (This->base.bindf & BINDF_FROMURLMON)
                  ? BINDSTATUS_MIMETYPEAVAILABLE : BINDSTATUS_RAWMIMETYPE, L"text/html");
    }

    content_length = query_http_info(This, HTTP_QUERY_CONTENT_LENGTH);
    if(content_length) {
        This->base.content_length = wcstol(content_length, NULL, 10);
        free(content_length);
    }

    return S_OK;
}

static void HttpProtocol_close_connection(Protocol *prot)
{
    HttpProtocol *This = impl_from_Protocol(prot);

    if(This->http_negotiate) {
        IHttpNegotiate_Release(This->http_negotiate);
        This->http_negotiate = NULL;
    }

    free(This->full_header);
    This->full_header = NULL;
}

static void HttpProtocol_on_error(Protocol *prot, DWORD error)
{
    HttpProtocol *This = impl_from_Protocol(prot);
    HRESULT hres;

    TRACE("(%p) %ld\n", prot, error);

    if(prot->flags & FLAG_FIRST_CONTINUE_COMPLETE) {
        FIXME("Not handling error %ld\n", error);
        return;
    }

    while((hres = handle_http_error(This, error)) == RPC_E_RETRY) {
        error = send_http_request(This);

        if(error == ERROR_IO_PENDING || error == ERROR_SUCCESS)
            return;
    }

    protocol_abort(prot, hres);
    protocol_close_connection(prot);
    return;
}

static const ProtocolVtbl AsyncProtocolVtbl = {
    HttpProtocol_open_request,
    HttpProtocol_end_request,
    HttpProtocol_start_downloading,
    HttpProtocol_close_connection,
    HttpProtocol_on_error
};

static HRESULT WINAPI HttpProtocolUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    HttpProtocol *This = impl_from_IUnknown(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IUnknown_inner;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolEx, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolEx %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        TRACE("(%p)->(IID_IInternetPriority %p)\n", This, ppv);
        *ppv = &This->IInternetPriority_iface;
    }else if(IsEqualGUID(&IID_IWinInetInfo, riid)) {
        TRACE("(%p)->(IID_IWinInetInfo %p)\n", This, ppv);
        *ppv = &This->IWinInetHttpInfo_iface;
    }else if(IsEqualGUID(&IID_IWinInetHttpInfo, riid)) {
        TRACE("(%p)->(IID_IWinInetHttpInfo %p)\n", This, ppv);
        *ppv = &This->IWinInetHttpInfo_iface;
    }else {
        *ppv = NULL;
        WARN("not supported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HttpProtocolUnk_AddRef(IUnknown *iface)
{
    HttpProtocol *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI HttpProtocolUnk_Release(IUnknown *iface)
{
    HttpProtocol *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        protocol_close_connection(&This->base);
        free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static const IUnknownVtbl HttpProtocolUnkVtbl = {
    HttpProtocolUnk_QueryInterface,
    HttpProtocolUnk_AddRef,
    HttpProtocolUnk_Release
};

static HRESULT WINAPI HttpProtocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI HttpProtocol_AddRef(IInternetProtocolEx *iface)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)\n", This);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI HttpProtocol_Release(IInternetProtocolEx *iface)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)\n", This);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI HttpProtocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %08lx %Ix)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    hres = CreateUri(szUrl, 0, 0, &uri);
    if(FAILED(hres))
        return hres;

    hres = IInternetProtocolEx_StartEx(&This->IInternetProtocolEx_iface, uri, pOIProtSink,
            pOIBindInfo, grfPI, (HANDLE*)dwReserved);

    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI HttpProtocol_Continue(IInternetProtocolEx *iface, PROTOCOLDATA *pProtocolData)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return protocol_continue(&This->base, pProtocolData);
}

static HRESULT WINAPI HttpProtocol_Abort(IInternetProtocolEx *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);

    return protocol_abort(&This->base, hrReason);
}

static HRESULT WINAPI HttpProtocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    protocol_close_connection(&This->base);
    return S_OK;
}

static HRESULT WINAPI HttpProtocol_Suspend(IInternetProtocolEx *iface)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Resume(IInternetProtocolEx *iface)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Read(IInternetProtocolEx *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    return protocol_read(&This->base, pv, cb, pcbRead);
}

static HRESULT WINAPI HttpProtocol_Seek(IInternetProtocolEx *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return protocol_lock_request(&This->base);
}

static HRESULT WINAPI HttpProtocol_UnlockRequest(IInternetProtocolEx *iface)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)\n", This);

    return protocol_unlock_request(&This->base);
}

static HRESULT WINAPI HttpProtocol_StartEx(IInternetProtocolEx *iface, IUri *pUri,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE *dwReserved)
{
    HttpProtocol *This = impl_from_IInternetProtocolEx(iface);
    DWORD scheme = 0;
    HRESULT hres;

    TRACE("(%p)->(%p %p %p %08lx %p)\n", This, pUri, pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    hres = IUri_GetScheme(pUri, &scheme);
    if(FAILED(hres))
        return hres;
    if(scheme != (This->https ? URL_SCHEME_HTTPS : URL_SCHEME_HTTP))
        return MK_E_SYNTAX;

    return protocol_start(&This->base, (IInternetProtocol*)&This->IInternetProtocolEx_iface, pUri,
                          pOIProtSink, pOIBindInfo);
}

static const IInternetProtocolExVtbl HttpProtocolVtbl = {
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
    HttpProtocol_UnlockRequest,
    HttpProtocol_StartEx
};

static HRESULT WINAPI HttpPriority_QueryInterface(IInternetPriority *iface, REFIID riid, void **ppv)
{
    HttpProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI HttpPriority_AddRef(IInternetPriority *iface)
{
    HttpProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI HttpPriority_Release(IInternetPriority *iface)
{
    HttpProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI HttpPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    HttpProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%ld)\n", This, nPriority);

    This->base.priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI HttpPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    HttpProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->base.priority;
    return S_OK;
}

static const IInternetPriorityVtbl HttpPriorityVtbl = {
    HttpPriority_QueryInterface,
    HttpPriority_AddRef,
    HttpPriority_Release,
    HttpPriority_SetPriority,
    HttpPriority_GetPriority
};

static HRESULT WINAPI HttpInfo_QueryInterface(IWinInetHttpInfo *iface, REFIID riid, void **ppv)
{
    HttpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI HttpInfo_AddRef(IWinInetHttpInfo *iface)
{
    HttpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI HttpInfo_Release(IWinInetHttpInfo *iface)
{
    HttpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI HttpInfo_QueryOption(IWinInetHttpInfo *iface, DWORD dwOption,
        void *pBuffer, DWORD *pcbBuffer)
{
    HttpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    TRACE("(%p)->(%lx %p %p)\n", This, dwOption, pBuffer, pcbBuffer);

    if(!This->base.request)
        return E_FAIL;

    if(!InternetQueryOptionW(This->base.request, dwOption, pBuffer, pcbBuffer))
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI HttpInfo_QueryInfo(IWinInetHttpInfo *iface, DWORD dwOption,
        void *pBuffer, DWORD *pcbBuffer, DWORD *pdwFlags, DWORD *pdwReserved)
{
    HttpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    TRACE("(%p)->(%lx %p %p %p %p)\n", This, dwOption, pBuffer, pcbBuffer, pdwFlags, pdwReserved);

    if(!This->base.request)
        return E_FAIL;

    if(!HttpQueryInfoA(This->base.request, dwOption, pBuffer, pcbBuffer, pdwFlags))
        return S_FALSE;

    return S_OK;
}

static const IWinInetHttpInfoVtbl WinInetHttpInfoVtbl = {
    HttpInfo_QueryInterface,
    HttpInfo_AddRef,
    HttpInfo_Release,
    HttpInfo_QueryOption,
    HttpInfo_QueryInfo
};

static HRESULT create_http_protocol(BOOL https, IUnknown *outer, void **ppobj)
{
    HttpProtocol *ret;

    ret = calloc(1, sizeof(HttpProtocol));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->base.vtbl = &AsyncProtocolVtbl;
    ret->IUnknown_inner.lpVtbl            = &HttpProtocolUnkVtbl;
    ret->IInternetProtocolEx_iface.lpVtbl = &HttpProtocolVtbl;
    ret->IInternetPriority_iface.lpVtbl   = &HttpPriorityVtbl;
    ret->IWinInetHttpInfo_iface.lpVtbl    = &WinInetHttpInfoVtbl;

    ret->https = https;
    ret->ref = 1;
    ret->outer = outer ? outer : &ret->IUnknown_inner;

    *ppobj = &ret->IUnknown_inner;

    URLMON_LockModule();
    return S_OK;
}

HRESULT HttpProtocol_Construct(IUnknown *outer, void **ppv)
{
    TRACE("(%p %p)\n", outer, ppv);

    return create_http_protocol(FALSE, outer, ppv);
}

HRESULT HttpSProtocol_Construct(IUnknown *outer, void **ppv)
{
    TRACE("(%p %p)\n", outer, ppv);

    return create_http_protocol(TRUE, outer, ppv);
}
