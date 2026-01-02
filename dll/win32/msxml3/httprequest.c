/*
 *    IXMLHTTPRequest implementation
 *
 * Copyright 2008 Alistair Leslie-Hughes
 * Copyright 2010-2012 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wininet.h"
#include "winreg.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtml.h"
#include "msxml6.h"
#include "objsafe.h"
#include "docobj.h"
#include "shlwapi.h"

#include "msxml_dispex.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

static const WCHAR colspaceW[] = {':',' ',0};
static const WCHAR crlfW[] = {'\r','\n',0};
static const DWORD safety_supported_options =
    INTERFACESAFE_FOR_UNTRUSTED_CALLER |
    INTERFACESAFE_FOR_UNTRUSTED_DATA   |
    INTERFACE_USES_SECURITY_MANAGER;

typedef struct BindStatusCallback BindStatusCallback;

struct httpheader
{
    struct list entry;
    BSTR header;
    BSTR value;
};

typedef struct
{
    IXMLHTTPRequest IXMLHTTPRequest_iface;
    IObjectWithSite IObjectWithSite_iface;
    IObjectSafety   IObjectSafety_iface;
    ISupportErrorInfo ISupportErrorInfo_iface;
    LONG ref;

    READYSTATE state;
    IDispatch *sink;

    /* request */
    BINDVERB verb;
    BSTR custom;
    IUri *uri;
    IUri *base_uri;
    BOOL async;
    struct list reqheaders;
    /* cached resulting custom request headers string length in WCHARs */
    LONG reqheader_size;
    /* use UTF-8 content type */
    BOOL use_utf8_content;

    /* response headers */
    struct list respheaders;
    BSTR raw_respheaders;

    /* credentials */
    BSTR user;
    BSTR password;

    /* bind callback */
    BindStatusCallback *bsc;
    LONG status;
    BSTR status_text;

    /* IObjectWithSite*/
    IUnknown *site;

    /* IObjectSafety */
    DWORD safeopt;
} httprequest;

typedef struct
{
    httprequest req;
    IServerXMLHTTPRequest IServerXMLHTTPRequest_iface;
} serverhttp;

static inline httprequest *impl_from_IXMLHTTPRequest( IXMLHTTPRequest *iface )
{
    return CONTAINING_RECORD(iface, httprequest, IXMLHTTPRequest_iface);
}

static inline httprequest *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, httprequest, IObjectWithSite_iface);
}

static inline httprequest *impl_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, httprequest, IObjectSafety_iface);
}

static inline httprequest *impl_from_ISupportErrorInfo(ISupportErrorInfo *iface)
{
    return CONTAINING_RECORD(iface, httprequest, ISupportErrorInfo_iface);
}

static inline serverhttp *impl_from_IServerXMLHTTPRequest(IServerXMLHTTPRequest *iface)
{
    return CONTAINING_RECORD(iface, serverhttp, IServerXMLHTTPRequest_iface);
}

static void httprequest_setreadystate(httprequest *This, READYSTATE state)
{
    READYSTATE last = This->state;
    static const char* readystates[] = {
        "READYSTATE_UNINITIALIZED",
        "READYSTATE_LOADING",
        "READYSTATE_LOADED",
        "READYSTATE_INTERACTIVE",
        "READYSTATE_COMPLETE"};

    This->state = state;

    TRACE("state %s\n", readystates[state]);

    if (This->sink && last != state)
    {
        DISPPARAMS params;

        memset(&params, 0, sizeof(params));
        IDispatch_Invoke(This->sink, 0, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, 0, 0, 0);
    }
}

static void free_response_headers(httprequest *This)
{
    struct httpheader *header, *header2;

    LIST_FOR_EACH_ENTRY_SAFE(header, header2, &This->respheaders, struct httpheader, entry)
    {
        list_remove(&header->entry);
        SysFreeString(header->header);
        SysFreeString(header->value);
        free(header);
    }

    SysFreeString(This->raw_respheaders);
    This->raw_respheaders = NULL;
}

static void free_request_headers(httprequest *This)
{
    struct httpheader *header, *header2;

    LIST_FOR_EACH_ENTRY_SAFE(header, header2, &This->reqheaders, struct httpheader, entry)
    {
        list_remove(&header->entry);
        SysFreeString(header->header);
        SysFreeString(header->value);
        free(header);
    }
}

struct BindStatusCallback
{
    IBindStatusCallback IBindStatusCallback_iface;
    IHttpNegotiate      IHttpNegotiate_iface;
    IAuthenticate       IAuthenticate_iface;
    LONG ref;

    IBinding *binding;
    httprequest *request;

    /* response data */
    IStream *stream;

    /* request body data */
    HGLOBAL body;
};

static inline BindStatusCallback *impl_from_IBindStatusCallback( IBindStatusCallback *iface )
{
    return CONTAINING_RECORD(iface, BindStatusCallback, IBindStatusCallback_iface);
}

static inline BindStatusCallback *impl_from_IHttpNegotiate( IHttpNegotiate *iface )
{
    return CONTAINING_RECORD(iface, BindStatusCallback, IHttpNegotiate_iface);
}

static inline BindStatusCallback *impl_from_IAuthenticate( IAuthenticate *iface )
{
    return CONTAINING_RECORD(iface, BindStatusCallback, IAuthenticate_iface);
}

static void BindStatusCallback_Detach(BindStatusCallback *bsc)
{
    if (bsc)
    {
        if (bsc->binding) IBinding_Abort(bsc->binding);
        bsc->request->bsc = NULL;
        bsc->request = NULL;
        IBindStatusCallback_Release(&bsc->IBindStatusCallback_iface);
    }
}

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    *ppv = NULL;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IBindStatusCallback, riid))
    {
        *ppv = &This->IBindStatusCallback_iface;
    }
    else if (IsEqualGUID(&IID_IHttpNegotiate, riid))
    {
        *ppv = &This->IHttpNegotiate_iface;
    }
    else if (IsEqualGUID(&IID_IAuthenticate, riid))
    {
        *ppv = &This->IAuthenticate_iface;
    }
    else if (IsEqualGUID(&IID_IServiceProvider, riid) ||
             IsEqualGUID(&IID_IBindStatusCallbackEx, riid) ||
             IsEqualGUID(&IID_IInternetProtocol, riid) ||
             IsEqualGUID(&IID_IHttpNegotiate2, riid))
    {
        return E_NOINTERFACE;
    }

    if (*ppv)
    {
        IBindStatusCallback_AddRef(iface);
        return S_OK;
    }

    FIXME("Unsupported riid = %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);

    if (!ref)
    {
        if (This->binding) IBinding_Release(This->binding);
        if (This->stream) IStream_Release(This->stream);
        if (This->body) GlobalFree(This->body);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD reserved, IBinding *pbind)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("%p, %ld, %p.\n", iface, reserved, pbind);

    if (!pbind) return E_INVALIDARG;

    This->binding = pbind;
    IBinding_AddRef(pbind);

    httprequest_setreadystate(This->request, READYSTATE_LOADED);

    return CreateStreamOnHGlobal(NULL, TRUE, &This->stream);
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pPriority)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%p)\n", This, pPriority);

    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    TRACE("%p, %ld.\n", iface, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    TRACE("%p, %lu, %lu, %lu, %s.\n", iface, ulProgress, ulProgressMax, ulStatusCode,
            debugstr_w(szStatusText));

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hr, LPCWSTR error)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("%p, %#lx, %s.\n", iface, hr, debugstr_w(error));

    if (This->binding)
    {
        IBinding_Release(This->binding);
        This->binding = NULL;
    }

    if (hr == S_OK)
    {
        BindStatusCallback_Detach(This->request->bsc);
        This->request->bsc = This;
        httprequest_setreadystate(This->request, READYSTATE_COMPLETE);
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *bind_flags, BINDINFO *pbindinfo)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%p %p)\n", This, bind_flags, pbindinfo);

    *bind_flags = 0;
    if (This->request->async) *bind_flags |= BINDF_ASYNCHRONOUS;

    if (This->request->verb != BINDVERB_GET && This->body)
    {
        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.hGlobal = This->body;
        pbindinfo->cbstgmedData = GlobalSize(This->body);
        /* callback owns passed body pointer */
        IBindStatusCallback_QueryInterface(iface, &IID_IUnknown, (void**)&pbindinfo->stgmedData.pUnkForRelease);
    }

    pbindinfo->dwBindVerb = This->request->verb;
    if (This->request->verb == BINDVERB_CUSTOM)
    {
        pbindinfo->szCustomVerb = CoTaskMemAlloc(SysStringByteLen(This->request->custom)+sizeof(WCHAR));
        lstrcpyW(pbindinfo->szCustomVerb, This->request->custom);
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD flags, DWORD size, FORMATETC *format, STGMEDIUM *stgmed)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    DWORD read, written;
    BYTE buf[4096];
    HRESULT hr;

    TRACE("%p, %#lx, %lu, %p, %p.\n", iface, flags, size, format, stgmed);

    do
    {
        hr = IStream_Read(stgmed->pstm, buf, sizeof(buf), &read);
        if (hr != S_OK) break;

        hr = IStream_Write(This->stream, buf, read, &written);
    } while((hr == S_OK) && written != 0 && read != 0);

    httprequest_setreadystate(This->request, READYSTATE_INTERACTIVE);

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), punk);

    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
    BindStatusCallback_QueryInterface,
    BindStatusCallback_AddRef,
    BindStatusCallback_Release,
    BindStatusCallback_OnStartBinding,
    BindStatusCallback_GetPriority,
    BindStatusCallback_OnLowResource,
    BindStatusCallback_OnProgress,
    BindStatusCallback_OnStopBinding,
    BindStatusCallback_GetBindInfo,
    BindStatusCallback_OnDataAvailable,
    BindStatusCallback_OnObjectAvailable
};

static HRESULT WINAPI BSCHttpNegotiate_QueryInterface(IHttpNegotiate *iface,
        REFIID riid, void **ppv)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI BSCHttpNegotiate_AddRef(IHttpNegotiate *iface)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI BSCHttpNegotiate_Release(IHttpNegotiate *iface)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI BSCHttpNegotiate_BeginningTransaction(IHttpNegotiate *iface,
        LPCWSTR url, LPCWSTR headers, DWORD reserved, LPWSTR *add_headers)
{
    static const WCHAR content_type_utf8W[] = {'C','o','n','t','e','n','t','-','T','y','p','e',':',' ',
        't','e','x','t','/','p','l','a','i','n',';','c','h','a','r','s','e','t','=','u','t','f','-','8','\r','\n',0};
    static const WCHAR refererW[] = {'R','e','f','e','r','e','r',':',' ',0};

    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    const struct httpheader *entry;
    BSTR base_uri = NULL;
    WCHAR *buff, *ptr;
    int size = 0;

    TRACE("%p, %s, %s, %ld, %p.\n", iface, debugstr_w(url), debugstr_w(headers), reserved, add_headers);

    *add_headers = NULL;

    if (This->request->use_utf8_content)
        size = sizeof(content_type_utf8W);

    if (!list_empty(&This->request->reqheaders))
        size += This->request->reqheader_size*sizeof(WCHAR);

    if (This->request->base_uri)
    {
        IUri_GetRawUri(This->request->base_uri, &base_uri);
        size += SysStringLen(base_uri)*sizeof(WCHAR) + sizeof(refererW) + sizeof(crlfW);
    }

    if (!size)
    {
        SysFreeString(base_uri);
        return S_OK;
    }

    buff = CoTaskMemAlloc(size);
    if (!buff)
    {
        SysFreeString(base_uri);
        return E_OUTOFMEMORY;
    }

    ptr = buff;
    if (This->request->use_utf8_content)
    {
        lstrcpyW(ptr, content_type_utf8W);
        ptr += ARRAY_SIZE(content_type_utf8W) - 1;
    }

    if (base_uri)
    {
        lstrcpyW(ptr, refererW);
        lstrcatW(ptr, base_uri);
        lstrcatW(ptr, crlfW);
        ptr += lstrlenW(refererW) + SysStringLen(base_uri) + lstrlenW(crlfW);
        SysFreeString(base_uri);
    }

    /* user headers */
    LIST_FOR_EACH_ENTRY(entry, &This->request->reqheaders, struct httpheader, entry)
    {
        lstrcpyW(ptr, entry->header);
        ptr += SysStringLen(entry->header);

        lstrcpyW(ptr, colspaceW);
        ptr += ARRAY_SIZE(colspaceW) - 1;

        lstrcpyW(ptr, entry->value);
        ptr += SysStringLen(entry->value);

        lstrcpyW(ptr, crlfW);
        ptr += ARRAY_SIZE(crlfW) - 1;
    }

    *add_headers = buff;

    return S_OK;
}

static void add_response_header(httprequest *This, const WCHAR *data, int len)
{
    struct httpheader *entry;
    const WCHAR *ptr = data;
    BSTR header, value;

    while (*ptr)
    {
        if (*ptr == ':')
        {
            header = SysAllocStringLen(data, ptr-data);
            /* skip leading spaces for a value */
            while (*++ptr == ' ')
                ;
            value = SysAllocStringLen(ptr, len-(ptr-data));
            break;
        }
        ptr++;
    }

    if (!*ptr) return;

    /* new header */
    TRACE("got header %s:%s\n", debugstr_w(header), debugstr_w(value));

    entry = malloc(sizeof(*entry));
    entry->header = header;
    entry->value  = value;
    list_add_head(&This->respheaders, &entry->entry);
}

static HRESULT WINAPI BSCHttpNegotiate_OnResponse(IHttpNegotiate *iface, DWORD code,
        LPCWSTR resp_headers, LPCWSTR req_headers, LPWSTR *add_reqheaders)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);

    TRACE("%p, %ld, %s, %s, %p.\n", iface, code, debugstr_w(resp_headers),
          debugstr_w(req_headers), add_reqheaders);

    This->request->status = code;
    /* store headers and status text */
    free_response_headers(This->request);
    SysFreeString(This->request->status_text);
    This->request->status_text = NULL;
    if (resp_headers)
    {
        const WCHAR *ptr, *line, *status_text;

        ptr = line = resp_headers;

        /* skip HTTP-Version */
        ptr = wcschr(ptr, ' ');
        if (ptr)
        {
            /* skip Status-Code */
            ptr = wcschr(++ptr, ' ');
            if (ptr)
            {
                status_text = ++ptr;
                /* now it supposed to end with CRLF */
                while (*ptr)
                {
                    if (*ptr == '\r' && *(ptr+1) == '\n')
                    {
                        line = ptr + 2;
                        This->request->status_text = SysAllocStringLen(status_text, ptr-status_text);
                        TRACE("status text %s\n", debugstr_w(This->request->status_text));
                        break;
                    }
                    ptr++;
                }
            }
        }

        /* store as unparsed string for now */
        This->request->raw_respheaders = SysAllocString(line);
    }

    return S_OK;
}

static const IHttpNegotiateVtbl BSCHttpNegotiateVtbl = {
    BSCHttpNegotiate_QueryInterface,
    BSCHttpNegotiate_AddRef,
    BSCHttpNegotiate_Release,
    BSCHttpNegotiate_BeginningTransaction,
    BSCHttpNegotiate_OnResponse
};

static HRESULT WINAPI Authenticate_QueryInterface(IAuthenticate *iface,
        REFIID riid, void **ppv)
{
    BindStatusCallback *This = impl_from_IAuthenticate(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI Authenticate_AddRef(IAuthenticate *iface)
{
    BindStatusCallback *This = impl_from_IAuthenticate(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI Authenticate_Release(IAuthenticate *iface)
{
    BindStatusCallback *This = impl_from_IAuthenticate(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI Authenticate_Authenticate(IAuthenticate *iface,
    HWND *hwnd, LPWSTR *username, LPWSTR *password)
{
    BindStatusCallback *This = impl_from_IAuthenticate(iface);
    httprequest *request = This->request;

    TRACE("(%p)->(%p %p %p)\n", This, hwnd, username, password);

    if (request->user && *request->user)
    {
        if (hwnd) *hwnd = NULL;
        *username = CoTaskMemAlloc(SysStringByteLen(request->user)+sizeof(WCHAR));
        *password = CoTaskMemAlloc(SysStringByteLen(request->password)+sizeof(WCHAR));
        if (!*username || !*password)
        {
            CoTaskMemFree(*username);
            CoTaskMemFree(*password);
            return E_OUTOFMEMORY;
        }

        memcpy(*username, request->user, SysStringByteLen(request->user)+sizeof(WCHAR));
        memcpy(*password, request->password, SysStringByteLen(request->password)+sizeof(WCHAR));
    }

    return S_OK;
}

static const IAuthenticateVtbl AuthenticateVtbl = {
    Authenticate_QueryInterface,
    Authenticate_AddRef,
    Authenticate_Release,
    Authenticate_Authenticate
};

static HRESULT BindStatusCallback_create(httprequest* This, BindStatusCallback **obj, const VARIANT *body)
{
    BindStatusCallback *bsc;
    IBindCtx *pbc = NULL;
    HRESULT hr;
    LONG size;

    if (!(bsc = malloc(sizeof(*bsc))))
        return E_OUTOFMEMORY;

    bsc->IBindStatusCallback_iface.lpVtbl = &BindStatusCallbackVtbl;
    bsc->IHttpNegotiate_iface.lpVtbl = &BSCHttpNegotiateVtbl;
    bsc->IAuthenticate_iface.lpVtbl = &AuthenticateVtbl;
    bsc->ref = 1;
    bsc->request = This;
    bsc->binding = NULL;
    bsc->stream = NULL;
    bsc->body = NULL;

    TRACE("(%p)->(%p)\n", This, bsc);

    This->use_utf8_content = FALSE;

    if (This->verb != BINDVERB_GET)
    {
        void *send_data, *ptr;
        SAFEARRAY *sa = NULL;

        if (V_VT(body) == (VT_VARIANT|VT_BYREF))
            body = V_VARIANTREF(body);

        switch (V_VT(body))
        {
        case VT_BSTR:
        {
            int len = SysStringLen(V_BSTR(body));
            const WCHAR *str = V_BSTR(body);
            UINT i, cp = CP_ACP;

            for (i = 0; i < len; i++)
            {
                if (str[i] > 127)
                {
                    cp = CP_UTF8;
                    break;
                }
            }

            size = WideCharToMultiByte(cp, 0, str, len, NULL, 0, NULL, NULL);
            if (!(ptr = malloc(size)))
            {
                free(bsc);
                return E_OUTOFMEMORY;
            }
            WideCharToMultiByte(cp, 0, str, len, ptr, size, NULL, NULL);
            if (cp == CP_UTF8) This->use_utf8_content = TRUE;
            break;
        }
        case VT_ARRAY|VT_UI1:
        {
            sa = V_ARRAY(body);
            if ((hr = SafeArrayAccessData(sa, &ptr)) != S_OK)
            {
                free(bsc);
                return hr;
            }
            if ((hr = SafeArrayGetUBound(sa, 1, &size)) != S_OK)
            {
                SafeArrayUnaccessData(sa);
                free(bsc);
                return hr;
            }
            size++;
            break;
        }
        default:
            FIXME("unsupported body data type %d\n", V_VT(body));
            /* fall through */
        case VT_EMPTY:
        case VT_ERROR:
        case VT_NULL:
            ptr = NULL;
            size = 0;
            break;
        }

        if (size)
        {
            bsc->body = GlobalAlloc(GMEM_FIXED, size);
            if (!bsc->body)
            {
                if (V_VT(body) == VT_BSTR)
                    free(ptr);
                else if (V_VT(body) == (VT_ARRAY|VT_UI1))
                    SafeArrayUnaccessData(sa);

                free(bsc);
                return E_OUTOFMEMORY;
            }

            send_data = GlobalLock(bsc->body);
            memcpy(send_data, ptr, size);
            GlobalUnlock(bsc->body);
        }

        if (V_VT(body) == VT_BSTR)
            free(ptr);
        else if (V_VT(body) == (VT_ARRAY|VT_UI1))
            SafeArrayUnaccessData(sa);
    }

    hr = CreateBindCtx(0, &pbc);
    if (hr == S_OK)
        hr = RegisterBindStatusCallback(pbc, &bsc->IBindStatusCallback_iface, NULL, 0);
    if (hr == S_OK)
    {
        IMoniker *moniker;

        hr = CreateURLMonikerEx2(NULL, This->uri, &moniker, URL_MK_UNIFORM);
        if (hr == S_OK)
        {
            IStream *stream;

            hr = IMoniker_BindToStorage(moniker, pbc, NULL, &IID_IStream, (void**)&stream);
            IMoniker_Release(moniker);
            if (stream) IStream_Release(stream);
        }
    }

    if (pbc)
        IBindCtx_Release(pbc);

    if (FAILED(hr))
    {
        IBindStatusCallback_Release(&bsc->IBindStatusCallback_iface);
        bsc = NULL;
    }

    *obj = bsc;
    return hr;
}

static HRESULT verify_uri(httprequest *This, IUri *uri)
{
    DWORD scheme, base_scheme;
    BSTR host, base_host;
    HRESULT hr;

    if(!(This->safeopt & INTERFACESAFE_FOR_UNTRUSTED_DATA))
        return S_OK;

    if(!This->base_uri)
        return E_ACCESSDENIED;

    hr = IUri_GetScheme(uri, &scheme);
    if(FAILED(hr))
        return hr;

    hr = IUri_GetScheme(This->base_uri, &base_scheme);
    if(FAILED(hr))
        return hr;

    if(scheme != base_scheme) {
        WARN("Schemes don't match\n");
        return E_ACCESSDENIED;
    }

    if(scheme == INTERNET_SCHEME_UNKNOWN) {
        FIXME("Unknown scheme\n");
        return E_ACCESSDENIED;
    }

    hr = IUri_GetHost(uri, &host);
    if(FAILED(hr))
        return hr;

    hr = IUri_GetHost(This->base_uri, &base_host);
    if(SUCCEEDED(hr)) {
        if(wcsicmp(host, base_host)) {
            WARN("Hosts don't match\n");
            hr = E_ACCESSDENIED;
        }
        SysFreeString(base_host);
    }

    SysFreeString(host);
    return hr;
}

static HRESULT httprequest_open(httprequest *This, BSTR method, BSTR url,
        VARIANT async, VARIANT user, VARIANT password)
{
    static const WCHAR MethodHeadW[] = {'H','E','A','D',0};
    static const WCHAR MethodGetW[] = {'G','E','T',0};
    static const WCHAR MethodPutW[] = {'P','U','T',0};
    static const WCHAR MethodPostW[] = {'P','O','S','T',0};
    static const WCHAR MethodDeleteW[] = {'D','E','L','E','T','E',0};
    static const WCHAR MethodPropFindW[] = {'P','R','O','P','F','I','N','D',0};
    VARIANT str, is_async;
    IUri *uri;
    HRESULT hr;

    if (!method || !url) return E_INVALIDARG;

    /* free previously set data */
    if(This->uri) {
        IUri_Release(This->uri);
        This->uri = NULL;
    }

    SysFreeString(This->user);
    SysFreeString(This->password);
    This->user = This->password = NULL;
    free_request_headers(This);

    if (!wcsicmp(method, MethodGetW))
    {
        This->verb = BINDVERB_GET;
    }
    else if (!wcsicmp(method, MethodPutW))
    {
        This->verb = BINDVERB_PUT;
    }
    else if (!wcsicmp(method, MethodPostW))
    {
        This->verb = BINDVERB_POST;
    }
    else if (!wcsicmp(method, MethodDeleteW) ||
             !wcsicmp(method, MethodHeadW) ||
             !wcsicmp(method, MethodPropFindW))
    {
        This->verb = BINDVERB_CUSTOM;
        SysReAllocString(&This->custom, method);
    }
    else
    {
        FIXME("unsupported request type %s\n", debugstr_w(method));
        This->verb = -1;
        return E_FAIL;
    }

    if(This->base_uri)
        hr = CoInternetCombineUrlEx(This->base_uri, url, 0, &uri, 0);
    else
        hr = CreateUri(url, 0, 0, &uri);
    if(FAILED(hr)) {
        WARN("Could not create IUri object, hr %#lx.\n", hr);
        return hr;
    }

    hr = verify_uri(This, uri);
    if(FAILED(hr)) {
        IUri_Release(uri);
        return hr;
    }

    VariantInit(&str);
    hr = VariantChangeType(&str, &user, 0, VT_BSTR);
    if (hr == S_OK)
        This->user = V_BSTR(&str);

    VariantInit(&str);
    hr = VariantChangeType(&str, &password, 0, VT_BSTR);
    if (hr == S_OK)
        This->password = V_BSTR(&str);

    /* add authentication info */
    if (This->user && *This->user)
    {
        IUriBuilder *builder;

        hr = CreateIUriBuilder(uri, 0, 0, &builder);
        if (hr == S_OK)
        {
            IUri *full_uri;

            IUriBuilder_SetUserName(builder, This->user);
            IUriBuilder_SetPassword(builder, This->password);
            hr = IUriBuilder_CreateUri(builder, -1, 0, 0, &full_uri);
            if (hr == S_OK)
            {
                IUri_Release(uri);
                uri = full_uri;
            }
            else
                WARN("failed to create modified uri, hr %#lx.\n", hr);
            IUriBuilder_Release(builder);
        }
        else
            WARN("IUriBuilder creation failed, hr %#lx.\n", hr);
    }

    This->uri = uri;

    VariantInit(&is_async);
    hr = VariantChangeType(&is_async, &async, 0, VT_BOOL);
    This->async = hr == S_OK && V_BOOL(&is_async);

    httprequest_setreadystate(This, READYSTATE_LOADING);

    return S_OK;
}

static HRESULT httprequest_setRequestHeader(httprequest *This, BSTR header, BSTR value)
{
    struct httpheader *entry;

    if (!header || !*header) return E_INVALIDARG;
    if (This->state != READYSTATE_LOADING) return E_FAIL;
    if (!value) return E_INVALIDARG;

    /* replace existing header value if already added */
    LIST_FOR_EACH_ENTRY(entry, &This->reqheaders, struct httpheader, entry)
    {
        if (wcscmp(entry->header, header) == 0)
        {
            LONG length = SysStringLen(entry->value);
            HRESULT hr;

            hr = SysReAllocString(&entry->value, value) ? S_OK : E_OUTOFMEMORY;

            if (hr == S_OK)
                This->reqheader_size += (SysStringLen(entry->value) - length);

            return hr;
        }
    }

    entry = malloc(sizeof(*entry));
    if (!entry) return E_OUTOFMEMORY;

    /* new header */
    entry->header = SysAllocString(header);
    entry->value  = SysAllocString(value);

    /* header length including null terminator */
    This->reqheader_size += SysStringLen(entry->header) + ARRAY_SIZE(colspaceW) +
        SysStringLen(entry->value) + ARRAY_SIZE(crlfW) - 1;

    list_add_head(&This->reqheaders, &entry->entry);

    return S_OK;
}

static HRESULT httprequest_getResponseHeader(httprequest *This, BSTR header, BSTR *value)
{
    struct httpheader *entry;

    if (!header) return E_INVALIDARG;
    if (!value) return E_POINTER;

    if (This->raw_respheaders && list_empty(&This->respheaders))
    {
        WCHAR *ptr, *line;

        ptr = line = This->raw_respheaders;
        while (*ptr)
        {
            if (*ptr == '\r' && *(ptr+1) == '\n')
            {
                add_response_header(This, line, ptr-line);
                ptr++; line = ++ptr;
                continue;
            }
            ptr++;
        }
    }

    LIST_FOR_EACH_ENTRY(entry, &This->respheaders, struct httpheader, entry)
    {
        if (!wcsicmp(entry->header, header))
        {
            *value = SysAllocString(entry->value);
            TRACE("header value %s\n", debugstr_w(*value));
            return S_OK;
        }
    }

    return S_FALSE;
}

static HRESULT httprequest_getAllResponseHeaders(httprequest *This, BSTR *respheaders)
{
    if (!respheaders) return E_POINTER;

    *respheaders = SysAllocString(This->raw_respheaders);

    return S_OK;
}

static HRESULT httprequest_send(httprequest *This, VARIANT body)
{
    BindStatusCallback *bsc = NULL;
    HRESULT hr;

    if (This->state != READYSTATE_LOADING) return E_FAIL;

    hr = BindStatusCallback_create(This, &bsc, &body);
    if (FAILED(hr))
        /* success path to detach it is OnStopBinding call */
        BindStatusCallback_Detach(bsc);

    return hr;
}

static HRESULT httprequest_abort(httprequest *This)
{
    BindStatusCallback_Detach(This->bsc);

    httprequest_setreadystate(This, READYSTATE_UNINITIALIZED);

    return S_OK;
}

static HRESULT httprequest_get_status(httprequest *This, LONG *status)
{
    if (!status) return E_POINTER;

    *status = This->status;

    return This->state == READYSTATE_COMPLETE ? S_OK : E_FAIL;
}

static HRESULT httprequest_get_statusText(httprequest *This, BSTR *status)
{
    if (!status) return E_POINTER;
    if (This->state != READYSTATE_COMPLETE) return E_FAIL;

    *status = SysAllocString(This->status_text);

    return S_OK;
}

enum response_encoding
{
    RESPONSE_ENCODING_NONE,
    RESPONSE_ENCODING_UCS4BE,
    RESPONSE_ENCODING_UCS4LE,
    RESPONSE_ENCODING_UCS4_2143,
    RESPONSE_ENCODING_UCS4_3412,
    RESPONSE_ENCODING_EBCDIC,
    RESPONSE_ENCODING_UTF8,
    RESPONSE_ENCODING_UTF16LE,
    RESPONSE_ENCODING_UTF16BE,
};

static unsigned int detect_response_encoding(const BYTE *in, unsigned int len)
{
    if (len >= 4)
    {
        if (in[0] == 0 && in[1] == 0 && in[2] == 0 && in[3] == 0x3c)
            return RESPONSE_ENCODING_UCS4BE;
        if (in[0] == 0x3c && in[1] == 0 && in[2] == 0 && in[3] == 0)
            return RESPONSE_ENCODING_UCS4LE;
        if (in[0] == 0 && in[1] == 0 && in[2] == 0x3c && in[3] == 0)
            return RESPONSE_ENCODING_UCS4_2143;
        if (in[0] == 0 && in[1] == 0x3c && in[2] == 0 && in[3] == 0)
            return RESPONSE_ENCODING_UCS4_3412;
        if (in[0] == 0x4c && in[1] == 0x6f && in[2] == 0xa7 && in[3] == 0x94)
            return RESPONSE_ENCODING_EBCDIC;
        if (in[0] == 0x3c && in[1] == 0x3f && in[2] == 0x78 && in[3] == 0x6d)
            return RESPONSE_ENCODING_UTF8;
        if (in[0] == 0x3c && in[1] == 0 && in[2] == 0x3f && in[3] == 0)
            return RESPONSE_ENCODING_UTF16LE;
        if (in[0] == 0 && in[1] == 0x3c && in[2] == 0 && in[3] == 0x3f)
            return RESPONSE_ENCODING_UTF16BE;
    }

    if (len >= 3)
    {
        if (in[0] == 0xef && in[1] == 0xbb && in[2] == 0xbf)
            return RESPONSE_ENCODING_UTF8;
    }

    if (len >= 2)
    {
        if (in[0] == 0xfe && in[1] == 0xff)
            return RESPONSE_ENCODING_UTF16BE;
        if (in[0] == 0xff && in[1] == 0xfe)
            return RESPONSE_ENCODING_UTF16LE;
    }

    return RESPONSE_ENCODING_NONE;
}

static HRESULT httprequest_get_responseText(httprequest *This, BSTR *body)
{
    HGLOBAL hglobal;
    HRESULT hr;

    if (!body) return E_POINTER;
    if (This->state != READYSTATE_COMPLETE) return E_FAIL;

    hr = GetHGlobalFromStream(This->bsc->stream, &hglobal);
    if (hr == S_OK)
    {
        const char *ptr = GlobalLock(hglobal);
        DWORD size = GlobalSize(hglobal);
        unsigned int encoding = RESPONSE_ENCODING_NONE;

        /* try to determine data encoding */
        if (size >= 4)
        {
            encoding = detect_response_encoding((const BYTE *)ptr, 4);
            TRACE("detected encoding: %u.\n", encoding);

            if (encoding != RESPONSE_ENCODING_UTF8 &&
                    encoding != RESPONSE_ENCODING_UTF16LE &&
                    encoding != RESPONSE_ENCODING_NONE )
            {
                FIXME("unsupported response encoding: %u.\n", encoding);
                GlobalUnlock(hglobal);
                return E_FAIL;
            }
        }

        /* without BOM assume UTF-8 */
        if (encoding == RESPONSE_ENCODING_UTF8 || encoding == RESPONSE_ENCODING_NONE)
        {
            DWORD length = MultiByteToWideChar(CP_UTF8, 0, ptr, size, NULL, 0);

            *body = SysAllocStringLen(NULL, length);
            if (*body)
                MultiByteToWideChar( CP_UTF8, 0, ptr, size, *body, length);
        }
        else
            *body = SysAllocStringByteLen((LPCSTR)ptr, size);

        if (!*body) hr = E_OUTOFMEMORY;
        GlobalUnlock(hglobal);
    }

    return hr;
}

static HRESULT httprequest_get_responseXML(httprequest *This, IDispatch **body)
{
    IXMLDOMDocument3 *doc;
    HRESULT hr;
    BSTR str;

    if (!body) return E_INVALIDARG;
    if (This->state != READYSTATE_COMPLETE) return E_FAIL;

    hr = dom_document_create(MSXML_DEFAULT, (void**)&doc);
    if (hr != S_OK) return hr;

    hr = httprequest_get_responseText(This, &str);
    if (hr == S_OK)
    {
        VARIANT_BOOL ok;

        hr = IXMLDOMDocument3_loadXML(doc, str, &ok);
        SysFreeString(str);
    }

    IXMLDOMDocument3_QueryInterface(doc, &IID_IDispatch, (void**)body);
    IXMLDOMDocument3_Release(doc);

    return hr;
}

static HRESULT httprequest_get_responseBody(httprequest *This, VARIANT *body)
{
    HGLOBAL hglobal;
    HRESULT hr;

    if (!body) return E_INVALIDARG;
    V_VT(body) = VT_EMPTY;

    if (This->state != READYSTATE_COMPLETE) return E_PENDING;

    hr = GetHGlobalFromStream(This->bsc->stream, &hglobal);
    if (hr == S_OK)
    {
        void *ptr = GlobalLock(hglobal);
        DWORD size = GlobalSize(hglobal);

        SAFEARRAYBOUND bound;
        SAFEARRAY *array;

        bound.lLbound = 0;
        bound.cElements = size;
        array = SafeArrayCreate(VT_UI1, 1, &bound);

        if (array)
        {
            void *dest;

            V_VT(body) = VT_ARRAY | VT_UI1;
            V_ARRAY(body) = array;

            hr = SafeArrayAccessData(array, &dest);
            if (hr == S_OK)
            {
                memcpy(dest, ptr, size);
                SafeArrayUnaccessData(array);
            }
            else
            {
                VariantClear(body);
            }
        }
        else
            hr = E_FAIL;

        GlobalUnlock(hglobal);
    }

    return hr;
}

static HRESULT httprequest_get_responseStream(httprequest *This, VARIANT *body)
{
    LARGE_INTEGER move;
    IStream *stream;
    HRESULT hr;

    if (!body) return E_INVALIDARG;
    V_VT(body) = VT_EMPTY;

    if (This->state != READYSTATE_COMPLETE) return E_PENDING;

    hr = IStream_Clone(This->bsc->stream, &stream);

    move.QuadPart = 0;
    IStream_Seek(stream, move, STREAM_SEEK_SET, NULL);

    V_VT(body) = VT_UNKNOWN;
    V_UNKNOWN(body) = (IUnknown*)stream;

    return hr;
}

static HRESULT httprequest_get_readyState(httprequest *This, LONG *state)
{
    if (!state) return E_POINTER;

    *state = This->state;
    return S_OK;
}

static HRESULT httprequest_put_onreadystatechange(httprequest *This, IDispatch *sink)
{
    if (This->sink) IDispatch_Release(This->sink);
    if ((This->sink = sink)) IDispatch_AddRef(This->sink);

    return S_OK;
}

static void httprequest_release(httprequest *This)
{
    if (This->site)
        IUnknown_Release( This->site );
    if (This->uri)
        IUri_Release(This->uri);
    if (This->base_uri)
        IUri_Release(This->base_uri);

    SysFreeString(This->custom);
    SysFreeString(This->user);
    SysFreeString(This->password);

    /* cleanup headers lists */
    free_request_headers(This);
    free_response_headers(This);
    SysFreeString(This->status_text);

    /* detach callback object */
    BindStatusCallback_Detach(This->bsc);

    if (This->sink) IDispatch_Release(This->sink);
}

static HRESULT WINAPI XMLHTTPRequest_QueryInterface(IXMLHTTPRequest *iface, REFIID riid, void **ppvObject)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLHTTPRequest) ||
         IsEqualGUID( riid, &IID_IDispatch) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *ppvObject = iface;
    }
    else if (IsEqualGUID(&IID_IObjectWithSite, riid))
    {
        *ppvObject = &This->IObjectWithSite_iface;
    }
    else if (IsEqualGUID(&IID_IObjectSafety, riid))
    {
        *ppvObject = &This->IObjectSafety_iface;
    }
    else if (IsEqualGUID(&IID_ISupportErrorInfo, riid))
    {
        *ppvObject = &This->ISupportErrorInfo_iface;
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppvObject);

    return S_OK;
}

static ULONG WINAPI XMLHTTPRequest_AddRef(IXMLHTTPRequest *iface)
{
    httprequest *request = impl_from_IXMLHTTPRequest(iface);
    ULONG ref = InterlockedIncrement(&request->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI XMLHTTPRequest_Release(IXMLHTTPRequest *iface)
{
    httprequest *request = impl_from_IXMLHTTPRequest(iface);
    ULONG ref = InterlockedDecrement(&request->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        httprequest_release(request);
        free(request);
    }

    return ref;
}

static HRESULT WINAPI XMLHTTPRequest_GetTypeInfoCount(IXMLHTTPRequest *iface, UINT *pctinfo)
{
    TRACE("%p, %p.\n", iface, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI XMLHTTPRequest_GetTypeInfo(IXMLHTTPRequest *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("%p, %u, %lx,%p.\n", iface, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IXMLHTTPRequest_tid, ppTInfo);
}

static HRESULT WINAPI XMLHTTPRequest_GetIDsOfNames(IXMLHTTPRequest *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLHTTPRequest_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI XMLHTTPRequest_Invoke(IXMLHTTPRequest *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLHTTPRequest_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI XMLHTTPRequest_open(IXMLHTTPRequest *iface, BSTR method, BSTR url,
        VARIANT async, VARIANT user, VARIANT password)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%s %s %s)\n", This, debugstr_w(method), debugstr_w(url),
        debugstr_variant(&async));
    return httprequest_open(This, method, url, async, user, password);
}

static HRESULT WINAPI XMLHTTPRequest_setRequestHeader(IXMLHTTPRequest *iface, BSTR header, BSTR value)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%s %s)\n", This, debugstr_w(header), debugstr_w(value));
    return httprequest_setRequestHeader(This, header, value);
}

static HRESULT WINAPI XMLHTTPRequest_getResponseHeader(IXMLHTTPRequest *iface, BSTR header, BSTR *value)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(header), value);
    return httprequest_getResponseHeader(This, header, value);
}

static HRESULT WINAPI XMLHTTPRequest_getAllResponseHeaders(IXMLHTTPRequest *iface, BSTR *respheaders)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, respheaders);
    return httprequest_getAllResponseHeaders(This, respheaders);
}

static HRESULT WINAPI XMLHTTPRequest_send(IXMLHTTPRequest *iface, VARIANT body)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&body));
    return httprequest_send(This, body);
}

static HRESULT WINAPI XMLHTTPRequest_abort(IXMLHTTPRequest *iface)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)\n", This);
    return httprequest_abort(This);
}

static HRESULT WINAPI XMLHTTPRequest_get_status(IXMLHTTPRequest *iface, LONG *status)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, status);
    return httprequest_get_status(This, status);
}

static HRESULT WINAPI XMLHTTPRequest_get_statusText(IXMLHTTPRequest *iface, BSTR *status)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, status);
    return httprequest_get_statusText(This, status);
}

static HRESULT WINAPI XMLHTTPRequest_get_responseXML(IXMLHTTPRequest *iface, IDispatch **body)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, body);
    return httprequest_get_responseXML(This, body);
}

static HRESULT WINAPI XMLHTTPRequest_get_responseText(IXMLHTTPRequest *iface, BSTR *body)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, body);
    return httprequest_get_responseText(This, body);
}

static HRESULT WINAPI XMLHTTPRequest_get_responseBody(IXMLHTTPRequest *iface, VARIANT *body)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, body);
    return httprequest_get_responseBody(This, body);
}

static HRESULT WINAPI XMLHTTPRequest_get_responseStream(IXMLHTTPRequest *iface, VARIANT *body)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, body);
    return httprequest_get_responseStream(This, body);
}

static HRESULT WINAPI XMLHTTPRequest_get_readyState(IXMLHTTPRequest *iface, LONG *state)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, state);
    return httprequest_get_readyState(This, state);
}

static HRESULT WINAPI XMLHTTPRequest_put_onreadystatechange(IXMLHTTPRequest *iface, IDispatch *sink)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, sink);
    return httprequest_put_onreadystatechange(This, sink);
}

static const struct IXMLHTTPRequestVtbl XMLHTTPRequestVtbl =
{
    XMLHTTPRequest_QueryInterface,
    XMLHTTPRequest_AddRef,
    XMLHTTPRequest_Release,
    XMLHTTPRequest_GetTypeInfoCount,
    XMLHTTPRequest_GetTypeInfo,
    XMLHTTPRequest_GetIDsOfNames,
    XMLHTTPRequest_Invoke,
    XMLHTTPRequest_open,
    XMLHTTPRequest_setRequestHeader,
    XMLHTTPRequest_getResponseHeader,
    XMLHTTPRequest_getAllResponseHeaders,
    XMLHTTPRequest_send,
    XMLHTTPRequest_abort,
    XMLHTTPRequest_get_status,
    XMLHTTPRequest_get_statusText,
    XMLHTTPRequest_get_responseXML,
    XMLHTTPRequest_get_responseText,
    XMLHTTPRequest_get_responseBody,
    XMLHTTPRequest_get_responseStream,
    XMLHTTPRequest_get_readyState,
    XMLHTTPRequest_put_onreadystatechange
};

/* IObjectWithSite */
static HRESULT WINAPI
httprequest_ObjectWithSite_QueryInterface( IObjectWithSite* iface, REFIID riid, void** ppvObject )
{
    httprequest *This = impl_from_IObjectWithSite(iface);
    return IXMLHTTPRequest_QueryInterface(&This->IXMLHTTPRequest_iface, riid, ppvObject);
}

static ULONG WINAPI httprequest_ObjectWithSite_AddRef( IObjectWithSite* iface )
{
    httprequest *This = impl_from_IObjectWithSite(iface);
    return IXMLHTTPRequest_AddRef(&This->IXMLHTTPRequest_iface);
}

static ULONG WINAPI httprequest_ObjectWithSite_Release( IObjectWithSite* iface )
{
    httprequest *This = impl_from_IObjectWithSite(iface);
    return IXMLHTTPRequest_Release(&This->IXMLHTTPRequest_iface);
}

static HRESULT WINAPI httprequest_ObjectWithSite_GetSite( IObjectWithSite *iface, REFIID iid, void **ppvSite )
{
    httprequest *This = impl_from_IObjectWithSite(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid( iid ), ppvSite );

    if ( !This->site )
        return E_FAIL;

    return IUnknown_QueryInterface( This->site, iid, ppvSite );
}

IUri *get_base_uri(IUnknown *site)
{
    IServiceProvider *provider;
    IHTMLDocument2 *doc;
    IUri *uri;
    BSTR url;
    HRESULT hr;

    hr = IUnknown_QueryInterface(site, &IID_IServiceProvider, (void**)&provider);
    if(FAILED(hr))
        return NULL;

    hr = IServiceProvider_QueryService(provider, &SID_SContainerDispatch, &IID_IHTMLDocument2, (void**)&doc);
    if(FAILED(hr))
        hr = IServiceProvider_QueryService(provider, &SID_SInternetHostSecurityManager, &IID_IHTMLDocument2, (void**)&doc);
    IServiceProvider_Release(provider);
    if(FAILED(hr))
        return NULL;

    hr = IHTMLDocument2_get_URL(doc, &url);
    IHTMLDocument2_Release(doc);
    if(FAILED(hr) || !url || !*url)
        return NULL;

    TRACE("host url %s\n", debugstr_w(url));

    hr = CreateUri(url, 0, 0, &uri);
    SysFreeString(url);
    if(FAILED(hr))
        return NULL;

    return uri;
}

static HRESULT WINAPI httprequest_ObjectWithSite_SetSite( IObjectWithSite *iface, IUnknown *punk )
{
    httprequest *This = impl_from_IObjectWithSite(iface);

    TRACE("(%p)->(%p)\n", This, punk);

    if(This->site)
        IUnknown_Release( This->site );
    if(This->base_uri)
        IUri_Release(This->base_uri);

    This->site = punk;

    if (punk)
    {
        IUnknown_AddRef( punk );
        This->base_uri = get_base_uri(This->site);
    }

    return S_OK;
}

static const IObjectWithSiteVtbl ObjectWithSiteVtbl =
{
    httprequest_ObjectWithSite_QueryInterface,
    httprequest_ObjectWithSite_AddRef,
    httprequest_ObjectWithSite_Release,
    httprequest_ObjectWithSite_SetSite,
    httprequest_ObjectWithSite_GetSite
};

/* IObjectSafety */
static HRESULT WINAPI httprequest_Safety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    httprequest *This = impl_from_IObjectSafety(iface);
    return IXMLHTTPRequest_QueryInterface(&This->IXMLHTTPRequest_iface, riid, ppv);
}

static ULONG WINAPI httprequest_Safety_AddRef(IObjectSafety *iface)
{
    httprequest *This = impl_from_IObjectSafety(iface);
    return IXMLHTTPRequest_AddRef(&This->IXMLHTTPRequest_iface);
}

static ULONG WINAPI httprequest_Safety_Release(IObjectSafety *iface)
{
    httprequest *This = impl_from_IObjectSafety(iface);
    return IXMLHTTPRequest_Release(&This->IXMLHTTPRequest_iface);
}

static HRESULT WINAPI httprequest_Safety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *supported, DWORD *enabled)
{
    httprequest *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), supported, enabled);

    if(!supported || !enabled) return E_POINTER;

    *supported = safety_supported_options;
    *enabled = This->safeopt;

    return S_OK;
}

static HRESULT WINAPI httprequest_Safety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD mask, DWORD enabled)
{
    httprequest *request = impl_from_IObjectSafety(iface);

    TRACE("%p, %s, %lx, %lx.\n", iface, debugstr_guid(riid), mask, enabled);

    if ((mask & ~safety_supported_options))
        return E_FAIL;

    request->safeopt = (request->safeopt & ~mask) | (mask & enabled);

    return S_OK;
}

static const IObjectSafetyVtbl ObjectSafetyVtbl = {
    httprequest_Safety_QueryInterface,
    httprequest_Safety_AddRef,
    httprequest_Safety_Release,
    httprequest_Safety_GetInterfaceSafetyOptions,
    httprequest_Safety_SetInterfaceSafetyOptions
};

static HRESULT WINAPI SupportErrorInfo_QueryInterface(ISupportErrorInfo *iface, REFIID riid, void **obj)
{
    httprequest *This = impl_from_ISupportErrorInfo(iface);
    return IXMLHTTPRequest_QueryInterface(&This->IXMLHTTPRequest_iface, riid, obj);
}

static ULONG WINAPI SupportErrorInfo_AddRef(ISupportErrorInfo *iface)
{
    httprequest *This = impl_from_ISupportErrorInfo(iface);
    return IXMLHTTPRequest_AddRef(&This->IXMLHTTPRequest_iface);
}

static ULONG WINAPI SupportErrorInfo_Release(ISupportErrorInfo *iface)
{
    httprequest *This = impl_from_ISupportErrorInfo(iface);
    return IXMLHTTPRequest_Release(&This->IXMLHTTPRequest_iface);
}

static HRESULT WINAPI SupportErrorInfo_InterfaceSupportsErrorInfo(ISupportErrorInfo *iface, REFIID riid)
{
    httprequest *This = impl_from_ISupportErrorInfo(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_guid(riid));

    return E_NOTIMPL;
}

static const ISupportErrorInfoVtbl SupportErrorInfoVtbl =
{
    SupportErrorInfo_QueryInterface,
    SupportErrorInfo_AddRef,
    SupportErrorInfo_Release,
    SupportErrorInfo_InterfaceSupportsErrorInfo,
};

/* IServerXMLHTTPRequest */
static HRESULT WINAPI ServerXMLHTTPRequest_QueryInterface(IServerXMLHTTPRequest *iface, REFIID riid, void **obj)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if ( IsEqualGUID( riid, &IID_IServerXMLHTTPRequest) ||
         IsEqualGUID( riid, &IID_IXMLHTTPRequest) ||
         IsEqualGUID( riid, &IID_IDispatch) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *obj = iface;
    }
    else if ( IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        *obj = &This->req.ISupportErrorInfo_iface;
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *)*obj );

    return S_OK;
}

static ULONG WINAPI ServerXMLHTTPRequest_AddRef(IServerXMLHTTPRequest *iface)
{
    serverhttp *request = impl_from_IServerXMLHTTPRequest(iface);
    ULONG ref = InterlockedIncrement(&request->req.ref);
    TRACE("%p, refcount %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI ServerXMLHTTPRequest_Release(IServerXMLHTTPRequest *iface)
{
    serverhttp *request = impl_from_IServerXMLHTTPRequest(iface);
    ULONG ref = InterlockedDecrement(&request->req.ref);

    TRACE("%p, refcount %lu.\n", iface, ref );

    if (!ref)
    {
        httprequest_release(&request->req);
        free(request);
    }

    return ref;
}

static HRESULT WINAPI ServerXMLHTTPRequest_GetTypeInfoCount(IServerXMLHTTPRequest *iface, UINT *pctinfo)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI ServerXMLHTTPRequest_GetTypeInfo(IServerXMLHTTPRequest *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("%p, %u, %lx, %p.\n", iface, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IServerXMLHTTPRequest_tid, ppTInfo);
}

static HRESULT WINAPI ServerXMLHTTPRequest_GetIDsOfNames(IServerXMLHTTPRequest *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IServerXMLHTTPRequest_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ServerXMLHTTPRequest_Invoke(IServerXMLHTTPRequest *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IServerXMLHTTPRequest_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ServerXMLHTTPRequest_open(IServerXMLHTTPRequest *iface, BSTR method, BSTR url,
        VARIANT async, VARIANT user, VARIANT password)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%s %s %s)\n", This, debugstr_w(method), debugstr_w(url),
        debugstr_variant(&async));
    return httprequest_open(&This->req, method, url, async, user, password);
}

static HRESULT WINAPI ServerXMLHTTPRequest_setRequestHeader(IServerXMLHTTPRequest *iface, BSTR header, BSTR value)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%s %s)\n", This, debugstr_w(header), debugstr_w(value));
    return httprequest_setRequestHeader(&This->req, header, value);
}

static HRESULT WINAPI ServerXMLHTTPRequest_getResponseHeader(IServerXMLHTTPRequest *iface, BSTR header, BSTR *value)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(header), value);
    return httprequest_getResponseHeader(&This->req, header, value);
}

static HRESULT WINAPI ServerXMLHTTPRequest_getAllResponseHeaders(IServerXMLHTTPRequest *iface, BSTR *respheaders)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, respheaders);
    return httprequest_getAllResponseHeaders(&This->req, respheaders);
}

static HRESULT WINAPI ServerXMLHTTPRequest_send(IServerXMLHTTPRequest *iface, VARIANT body)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&body));
    return httprequest_send(&This->req, body);
}

static HRESULT WINAPI ServerXMLHTTPRequest_abort(IServerXMLHTTPRequest *iface)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)\n", This);
    return httprequest_abort(&This->req);
}

static HRESULT WINAPI ServerXMLHTTPRequest_get_status(IServerXMLHTTPRequest *iface, LONG *status)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, status);
    return httprequest_get_status(&This->req, status);
}

static HRESULT WINAPI ServerXMLHTTPRequest_get_statusText(IServerXMLHTTPRequest *iface, BSTR *status)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, status);
    return httprequest_get_statusText(&This->req, status);
}

static HRESULT WINAPI ServerXMLHTTPRequest_get_responseXML(IServerXMLHTTPRequest *iface, IDispatch **body)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, body);
    return httprequest_get_responseXML(&This->req, body);
}

static HRESULT WINAPI ServerXMLHTTPRequest_get_responseText(IServerXMLHTTPRequest *iface, BSTR *body)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, body);
    return httprequest_get_responseText(&This->req, body);
}

static HRESULT WINAPI ServerXMLHTTPRequest_get_responseBody(IServerXMLHTTPRequest *iface, VARIANT *body)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, body);
    return httprequest_get_responseBody(&This->req, body);
}

static HRESULT WINAPI ServerXMLHTTPRequest_get_responseStream(IServerXMLHTTPRequest *iface, VARIANT *body)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, body);
    return httprequest_get_responseStream(&This->req, body);
}

static HRESULT WINAPI ServerXMLHTTPRequest_get_readyState(IServerXMLHTTPRequest *iface, LONG *state)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, state);
    return httprequest_get_readyState(&This->req, state);
}

static HRESULT WINAPI ServerXMLHTTPRequest_put_onreadystatechange(IServerXMLHTTPRequest *iface, IDispatch *sink)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    TRACE("(%p)->(%p)\n", This, sink);
    return httprequest_put_onreadystatechange(&This->req, sink);
}

static HRESULT WINAPI ServerXMLHTTPRequest_setTimeouts(IServerXMLHTTPRequest *iface, LONG resolveTimeout, LONG connectTimeout,
    LONG sendTimeout, LONG receiveTimeout)
{
    FIXME("%p, %ld, %ld, %ld, %ld: stub\n", iface, resolveTimeout, connectTimeout, sendTimeout, receiveTimeout);
    return S_OK;
}

static HRESULT WINAPI ServerXMLHTTPRequest_waitForResponse(IServerXMLHTTPRequest *iface, VARIANT timeout, VARIANT_BOOL *isSuccessful)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_variant(&timeout), isSuccessful);
    return E_NOTIMPL;
}

static HRESULT WINAPI ServerXMLHTTPRequest_getOption(IServerXMLHTTPRequest *iface, SERVERXMLHTTP_OPTION option, VARIANT *value)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    FIXME("(%p)->(%d %p): stub\n", This, option, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI ServerXMLHTTPRequest_setOption(IServerXMLHTTPRequest *iface, SERVERXMLHTTP_OPTION option, VARIANT value)
{
    serverhttp *This = impl_from_IServerXMLHTTPRequest( iface );
    FIXME("(%p)->(%d %s): stub\n", This, option, debugstr_variant(&value));
    return E_NOTIMPL;
}

static const struct IServerXMLHTTPRequestVtbl ServerXMLHTTPRequestVtbl =
{
    ServerXMLHTTPRequest_QueryInterface,
    ServerXMLHTTPRequest_AddRef,
    ServerXMLHTTPRequest_Release,
    ServerXMLHTTPRequest_GetTypeInfoCount,
    ServerXMLHTTPRequest_GetTypeInfo,
    ServerXMLHTTPRequest_GetIDsOfNames,
    ServerXMLHTTPRequest_Invoke,
    ServerXMLHTTPRequest_open,
    ServerXMLHTTPRequest_setRequestHeader,
    ServerXMLHTTPRequest_getResponseHeader,
    ServerXMLHTTPRequest_getAllResponseHeaders,
    ServerXMLHTTPRequest_send,
    ServerXMLHTTPRequest_abort,
    ServerXMLHTTPRequest_get_status,
    ServerXMLHTTPRequest_get_statusText,
    ServerXMLHTTPRequest_get_responseXML,
    ServerXMLHTTPRequest_get_responseText,
    ServerXMLHTTPRequest_get_responseBody,
    ServerXMLHTTPRequest_get_responseStream,
    ServerXMLHTTPRequest_get_readyState,
    ServerXMLHTTPRequest_put_onreadystatechange,
    ServerXMLHTTPRequest_setTimeouts,
    ServerXMLHTTPRequest_waitForResponse,
    ServerXMLHTTPRequest_getOption,
    ServerXMLHTTPRequest_setOption
};

static void init_httprequest(httprequest *req)
{
    req->IXMLHTTPRequest_iface.lpVtbl = &XMLHTTPRequestVtbl;
    req->IObjectWithSite_iface.lpVtbl = &ObjectWithSiteVtbl;
    req->IObjectSafety_iface.lpVtbl = &ObjectSafetyVtbl;
    req->ISupportErrorInfo_iface.lpVtbl = &SupportErrorInfoVtbl;
    req->ref = 1;

    req->async = FALSE;
    req->verb = -1;
    req->custom = NULL;
    req->uri = req->base_uri = NULL;
    req->user = req->password = NULL;

    req->state = READYSTATE_UNINITIALIZED;
    req->sink = NULL;

    req->bsc = NULL;
    req->status = 0;
    req->status_text = NULL;
    req->reqheader_size = 0;
    req->raw_respheaders = NULL;
    req->use_utf8_content = FALSE;

    list_init(&req->reqheaders);
    list_init(&req->respheaders);

    req->site = NULL;
    req->safeopt = 0;
}

HRESULT XMLHTTPRequest_create(void **obj)
{
    httprequest *req;

    TRACE("(%p)\n", obj);

    req = malloc(sizeof(*req));
    if( !req )
        return E_OUTOFMEMORY;

    init_httprequest(req);
    *obj = &req->IXMLHTTPRequest_iface;

    TRACE("returning iface %p\n", *obj);

    return S_OK;
}

HRESULT ServerXMLHTTP_create(void **obj)
{
    serverhttp *req;

    TRACE("(%p)\n", obj);

    req = malloc(sizeof(*req));
    if( !req )
        return E_OUTOFMEMORY;

    init_httprequest(&req->req);
    req->IServerXMLHTTPRequest_iface.lpVtbl = &ServerXMLHTTPRequestVtbl;

    *obj = &req->IServerXMLHTTPRequest_iface;

    TRACE("returning iface %p\n", *obj);

    return S_OK;
}
