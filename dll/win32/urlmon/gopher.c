/*
 * Copyright 2009 Jacek Caban for CodeWeavers
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    Protocol base;

    IInternetProtocol IInternetProtocol_iface;
    IInternetPriority IInternetPriority_iface;

    LONG ref;
} GopherProtocol;

static inline GopherProtocol *impl_from_Protocol(Protocol *prot)
{
    return CONTAINING_RECORD(prot, GopherProtocol, base);
}

static HRESULT GopherProtocol_open_request(Protocol *prot, IUri *uri, DWORD request_flags,
        HINTERNET internet_session, IInternetBindInfo *bind_info)
{
    GopherProtocol *This = impl_from_Protocol(prot);
    BSTR url;
    HRESULT hres;

    hres = IUri_GetAbsoluteUri(uri, &url);
    if(FAILED(hres))
        return hres;

    This->base.request = InternetOpenUrlW(internet_session, url, NULL, 0,
            request_flags, (DWORD_PTR)&This->base);
    SysFreeString(url);
    if (!This->base.request && GetLastError() != ERROR_IO_PENDING) {
        WARN("InternetOpenUrl failed: %ld\n", GetLastError());
        return INET_E_RESOURCE_NOT_FOUND;
    }

    return S_OK;
}

static HRESULT GopherProtocol_end_request(Protocol *prot)
{
    return E_NOTIMPL;
}

static HRESULT GopherProtocol_start_downloading(Protocol *prot)
{
    return S_OK;
}

static void GopherProtocol_close_connection(Protocol *prot)
{
}

static void GopherProtocol_on_error(Protocol *prot, DWORD error)
{
    FIXME("(%p) %ld - stub\n", prot, error);
}

static const ProtocolVtbl AsyncProtocolVtbl = {
    GopherProtocol_open_request,
    GopherProtocol_end_request,
    GopherProtocol_start_downloading,
    GopherProtocol_close_connection,
    GopherProtocol_on_error
};

static inline GopherProtocol *impl_from_IInternetProtocol(IInternetProtocol *iface)
{
    return CONTAINING_RECORD(iface, GopherProtocol, IInternetProtocol_iface);
}

static HRESULT WINAPI GopherProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = &This->IInternetProtocol_iface;
    }else if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        TRACE("(%p)->(IID_IInternetPriority %p)\n", This, ppv);
        *ppv = &This->IInternetPriority_iface;
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI GopherProtocol_AddRef(IInternetProtocol *iface)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI GopherProtocol_Release(IInternetProtocol *iface)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI GopherProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %08lx %Ix)\n", This, debugstr_w(szUrl), pOIProtSink,
          pOIBindInfo, grfPI, dwReserved);

    hres = CreateUri(szUrl, 0, 0, &uri);
    if(FAILED(hres))
        return hres;

    hres = protocol_start(&This->base, &This->IInternetProtocol_iface, uri, pOIProtSink,
            pOIBindInfo);

    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI GopherProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return protocol_continue(&This->base, pProtocolData);
}

static HRESULT WINAPI GopherProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);

    return protocol_abort(&This->base, hrReason);
}

static HRESULT WINAPI GopherProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    protocol_close_connection(&This->base);
    return S_OK;
}

static HRESULT WINAPI GopherProtocol_Suspend(IInternetProtocol *iface)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI GopherProtocol_Resume(IInternetProtocol *iface)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI GopherProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    return protocol_read(&This->base, pv, cb, pcbRead);
}

static HRESULT WINAPI GopherProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI GopherProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return protocol_lock_request(&This->base);
}

static HRESULT WINAPI GopherProtocol_UnlockRequest(IInternetProtocol *iface)
{
    GopherProtocol *This = impl_from_IInternetProtocol(iface);

    TRACE("(%p)\n", This);

    return protocol_unlock_request(&This->base);
}

static const IInternetProtocolVtbl GopherProtocolVtbl = {
    GopherProtocol_QueryInterface,
    GopherProtocol_AddRef,
    GopherProtocol_Release,
    GopherProtocol_Start,
    GopherProtocol_Continue,
    GopherProtocol_Abort,
    GopherProtocol_Terminate,
    GopherProtocol_Suspend,
    GopherProtocol_Resume,
    GopherProtocol_Read,
    GopherProtocol_Seek,
    GopherProtocol_LockRequest,
    GopherProtocol_UnlockRequest
};

static inline GopherProtocol *impl_from_IInternetPriority(IInternetPriority *iface)
{
    return CONTAINING_RECORD(iface, GopherProtocol, IInternetPriority_iface);
}

static HRESULT WINAPI GopherPriority_QueryInterface(IInternetPriority *iface, REFIID riid, void **ppv)
{
    GopherProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocol_QueryInterface(&This->IInternetProtocol_iface, riid, ppv);
}

static ULONG WINAPI GopherPriority_AddRef(IInternetPriority *iface)
{
    GopherProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocol_AddRef(&This->IInternetProtocol_iface);
}

static ULONG WINAPI GopherPriority_Release(IInternetPriority *iface)
{
    GopherProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocol_Release(&This->IInternetProtocol_iface);
}

static HRESULT WINAPI GopherPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    GopherProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%ld)\n", This, nPriority);

    This->base.priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI GopherPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    GopherProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->base.priority;
    return S_OK;
}

static const IInternetPriorityVtbl GopherPriorityVtbl = {
    GopherPriority_QueryInterface,
    GopherPriority_AddRef,
    GopherPriority_Release,
    GopherPriority_SetPriority,
    GopherPriority_GetPriority
};

HRESULT GopherProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    GopherProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = calloc(1, sizeof(GopherProtocol));

    ret->base.vtbl = &AsyncProtocolVtbl;
    ret->IInternetProtocol_iface.lpVtbl = &GopherProtocolVtbl;
    ret->IInternetPriority_iface.lpVtbl = &GopherPriorityVtbl;
    ret->ref = 1;

    *ppobj = &ret->IInternetProtocol_iface;

    return S_OK;
}
