/*
 * Copyright 2005-2009 Jacek Caban for CodeWeavers
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

typedef struct {
    Protocol base;

    IInternetProtocolEx IInternetProtocolEx_iface;
    IInternetPriority   IInternetPriority_iface;
    IWinInetHttpInfo    IWinInetHttpInfo_iface;

    LONG ref;
} FtpProtocol;

static inline FtpProtocol *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, FtpProtocol, IInternetProtocolEx_iface);
}

static inline FtpProtocol *impl_from_IInternetPriority(IInternetPriority *iface)
{
    return CONTAINING_RECORD(iface, FtpProtocol, IInternetPriority_iface);
}
static inline FtpProtocol *impl_from_IWinInetHttpInfo(IWinInetHttpInfo *iface)

{
    return CONTAINING_RECORD(iface, FtpProtocol, IWinInetHttpInfo_iface);
}

static inline FtpProtocol *impl_from_Protocol(Protocol *prot)
{
    return CONTAINING_RECORD(prot, FtpProtocol, base);
}

static HRESULT FtpProtocol_open_request(Protocol *prot, IUri *uri, DWORD request_flags,
        HINTERNET internet_session, IInternetBindInfo *bind_info)
{
    FtpProtocol *This = impl_from_Protocol(prot);
    BSTR url;
    HRESULT hres;

    hres = IUri_GetAbsoluteUri(uri, &url);
    if(FAILED(hres))
        return hres;

    This->base.request = InternetOpenUrlW(internet_session, url, NULL, 0,
            request_flags|INTERNET_FLAG_EXISTING_CONNECT|INTERNET_FLAG_PASSIVE,
            (DWORD_PTR)&This->base);
    SysFreeString(url);
    if (!This->base.request && GetLastError() != ERROR_IO_PENDING) {
        WARN("InternetOpenUrl failed: %d\n", GetLastError());
        return INET_E_RESOURCE_NOT_FOUND;
    }

    return S_OK;
}

static HRESULT FtpProtocol_end_request(Protocol *prot)
{
    return E_NOTIMPL;
}

static HRESULT FtpProtocol_start_downloading(Protocol *prot)
{
    FtpProtocol *This = impl_from_Protocol(prot);
    DWORD size;
    BOOL res;

    res = FtpGetFileSize(This->base.request, &size);
    if(res)
        This->base.content_length = size;
    else
        WARN("FtpGetFileSize failed: %d\n", GetLastError());

    return S_OK;
}

static void FtpProtocol_close_connection(Protocol *prot)
{
}

static void FtpProtocol_on_error(Protocol *prot, DWORD error)
{
    FIXME("(%p) %d - stub\n", prot, error);
}

static const ProtocolVtbl AsyncProtocolVtbl = {
    FtpProtocol_open_request,
    FtpProtocol_end_request,
    FtpProtocol_start_downloading,
    FtpProtocol_close_connection,
    FtpProtocol_on_error
};

static HRESULT WINAPI FtpProtocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
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
    }

    if(*ppv) {
        IInternetProtocolEx_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI FtpProtocol_AddRef(IInternetProtocolEx *iface)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI FtpProtocol_Release(IInternetProtocolEx *iface)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        protocol_close_connection(&This->base);
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI FtpProtocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
          pOIBindInfo, grfPI, dwReserved);

    hres = CreateUri(szUrl, 0, 0, &uri);
    if(FAILED(hres))
        return hres;

    hres = IInternetProtocolEx_StartEx(&This->IInternetProtocolEx_iface, uri, pOIProtSink,
            pOIBindInfo, grfPI, (HANDLE*)dwReserved);

    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI FtpProtocol_Continue(IInternetProtocolEx *iface, PROTOCOLDATA *pProtocolData)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return protocol_continue(&This->base, pProtocolData);
}

static HRESULT WINAPI FtpProtocol_Abort(IInternetProtocolEx *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);

    return protocol_abort(&This->base, hrReason);
}

static HRESULT WINAPI FtpProtocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    protocol_close_connection(&This->base);
    return S_OK;
}

static HRESULT WINAPI FtpProtocol_Suspend(IInternetProtocolEx *iface)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI FtpProtocol_Resume(IInternetProtocolEx *iface)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI FtpProtocol_Read(IInternetProtocolEx *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    return protocol_read(&This->base, pv, cb, pcbRead);
}

static HRESULT WINAPI FtpProtocol_Seek(IInternetProtocolEx *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI FtpProtocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return protocol_lock_request(&This->base);
}

static HRESULT WINAPI FtpProtocol_UnlockRequest(IInternetProtocolEx *iface)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)\n", This);

    return protocol_unlock_request(&This->base);
}

static HRESULT WINAPI FtpProtocol_StartEx(IInternetProtocolEx *iface, IUri *pUri,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE *dwReserved)
{
    FtpProtocol *This = impl_from_IInternetProtocolEx(iface);
    DWORD scheme = 0;
    HRESULT hres;

    TRACE("(%p)->(%p %p %p %08x %p)\n", This, pUri, pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    hres = IUri_GetScheme(pUri, &scheme);
    if(FAILED(hres))
        return hres;
    if(scheme != URL_SCHEME_FTP)
        return MK_E_SYNTAX;

    return protocol_start(&This->base, (IInternetProtocol*)&This->IInternetProtocolEx_iface, pUri,
                          pOIProtSink, pOIBindInfo);
}

static const IInternetProtocolExVtbl FtpProtocolVtbl = {
    FtpProtocol_QueryInterface,
    FtpProtocol_AddRef,
    FtpProtocol_Release,
    FtpProtocol_Start,
    FtpProtocol_Continue,
    FtpProtocol_Abort,
    FtpProtocol_Terminate,
    FtpProtocol_Suspend,
    FtpProtocol_Resume,
    FtpProtocol_Read,
    FtpProtocol_Seek,
    FtpProtocol_LockRequest,
    FtpProtocol_UnlockRequest,
    FtpProtocol_StartEx
};

static HRESULT WINAPI FtpPriority_QueryInterface(IInternetPriority *iface, REFIID riid, void **ppv)
{
    FtpProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI FtpPriority_AddRef(IInternetPriority *iface)
{
    FtpProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI FtpPriority_Release(IInternetPriority *iface)
{
    FtpProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI FtpPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    FtpProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%d)\n", This, nPriority);

    This->base.priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI FtpPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    FtpProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->base.priority;
    return S_OK;
}

static const IInternetPriorityVtbl FtpPriorityVtbl = {
    FtpPriority_QueryInterface,
    FtpPriority_AddRef,
    FtpPriority_Release,
    FtpPriority_SetPriority,
    FtpPriority_GetPriority
};

static HRESULT WINAPI HttpInfo_QueryInterface(IWinInetHttpInfo *iface, REFIID riid, void **ppv)
{
    FtpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI HttpInfo_AddRef(IWinInetHttpInfo *iface)
{
    FtpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI HttpInfo_Release(IWinInetHttpInfo *iface)
{
    FtpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI HttpInfo_QueryOption(IWinInetHttpInfo *iface, DWORD dwOption,
        void *pBuffer, DWORD *pcbBuffer)
{
    FtpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    TRACE("(%p)->(%x %p %p)\n", This, dwOption, pBuffer, pcbBuffer);

    if(!This->base.request)
        return E_FAIL;

    if(!InternetQueryOptionW(This->base.request, dwOption, pBuffer, pcbBuffer))
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI HttpInfo_QueryInfo(IWinInetHttpInfo *iface, DWORD dwOption,
        void *pBuffer, DWORD *pcbBuffer, DWORD *pdwFlags, DWORD *pdwReserved)
{
    FtpProtocol *This = impl_from_IWinInetHttpInfo(iface);
    TRACE("(%p)->(%x %p %p %p %p)\n", This, dwOption, pBuffer, pcbBuffer, pdwFlags, pdwReserved);

    if(!This->base.request)
        return E_FAIL;

    if(!HttpQueryInfoW(This->base.request, dwOption, pBuffer, pcbBuffer, pdwFlags))
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

HRESULT FtpProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    FtpProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = heap_alloc_zero(sizeof(FtpProtocol));

    ret->base.vtbl = &AsyncProtocolVtbl;
    ret->IInternetProtocolEx_iface.lpVtbl = &FtpProtocolVtbl;
    ret->IInternetPriority_iface.lpVtbl   = &FtpPriorityVtbl;
    ret->IWinInetHttpInfo_iface.lpVtbl    = &WinInetHttpInfoVtbl;
    ret->ref = 1;

    *ppobj = &ret->IInternetProtocolEx_iface;

    return S_OK;
}
