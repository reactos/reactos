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

    const IInternetProtocolVtbl  *lpInternetProtocolVtbl;
    const IInternetPriorityVtbl  *lpInternetPriorityVtbl;

    LONG ref;
} GopherProtocol;

#define PROTOCOL(x)  ((IInternetProtocol*)  &(x)->lpInternetProtocolVtbl)
#define PRIORITY(x)  ((IInternetPriority*)  &(x)->lpInternetPriorityVtbl)

#define ASYNCPROTOCOL_THIS(iface) DEFINE_THIS2(GopherProtocol, base, iface)

static HRESULT GopherProtocol_open_request(Protocol *prot, LPCWSTR url, DWORD request_flags,
                                        IInternetBindInfo *bind_info)
{
    GopherProtocol *This = ASYNCPROTOCOL_THIS(prot);

    This->base.request = InternetOpenUrlW(This->base.internet, url, NULL, 0,
            request_flags, (DWORD_PTR)&This->base);
    if (!This->base.request && GetLastError() != ERROR_IO_PENDING) {
        WARN("InternetOpenUrl failed: %d\n", GetLastError());
        return INET_E_RESOURCE_NOT_FOUND;
    }

    return S_OK;
}

static HRESULT GopherProtocol_start_downloading(Protocol *prot)
{
    return S_OK;
}

static void GopherProtocol_close_connection(Protocol *prot)
{
}

#undef ASYNCPROTOCOL_THIS

static const ProtocolVtbl AsyncProtocolVtbl = {
    GopherProtocol_open_request,
    GopherProtocol_start_downloading,
    GopherProtocol_close_connection
};

#define PROTOCOL_THIS(iface) DEFINE_THIS(GopherProtocol, InternetProtocol, iface)

static HRESULT WINAPI GopherProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);

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

static ULONG WINAPI GopherProtocol_AddRef(IInternetProtocol *iface)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI GopherProtocol_Release(IInternetProtocol *iface)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI GopherProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%s %p %p %08x %d)\n", This, debugstr_w(szUrl), pOIProtSink,
          pOIBindInfo, grfPI, dwReserved);

    return protocol_start(&This->base, PROTOCOL(This), szUrl, pOIProtSink, pOIBindInfo);
}

static HRESULT WINAPI GopherProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return protocol_continue(&This->base, pProtocolData);
}

static HRESULT WINAPI GopherProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI GopherProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    protocol_close_connection(&This->base);
    return S_OK;
}

static HRESULT WINAPI GopherProtocol_Suspend(IInternetProtocol *iface)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI GopherProtocol_Resume(IInternetProtocol *iface)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI GopherProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    return protocol_read(&This->base, pv, cb, pcbRead);
}

static HRESULT WINAPI GopherProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI GopherProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return protocol_lock_request(&This->base);
}

static HRESULT WINAPI GopherProtocol_UnlockRequest(IInternetProtocol *iface)
{
    GopherProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)\n", This);

    return protocol_unlock_request(&This->base);
}

#undef PROTOCOL_THIS

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

#define PRIORITY_THIS(iface) DEFINE_THIS(GopherProtocol, InternetPriority, iface)

static HRESULT WINAPI GopherPriority_QueryInterface(IInternetPriority *iface, REFIID riid, void **ppv)
{
    GopherProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI GopherPriority_AddRef(IInternetPriority *iface)
{
    GopherProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI GopherPriority_Release(IInternetPriority *iface)
{
    GopherProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI GopherPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    GopherProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%d)\n", This, nPriority);

    This->base.priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI GopherPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    GopherProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->base.priority;
    return S_OK;
}

#undef PRIORITY_THIS

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

    ret = heap_alloc_zero(sizeof(GopherProtocol));

    ret->base.vtbl = &AsyncProtocolVtbl;
    ret->lpInternetProtocolVtbl = &GopherProtocolVtbl;
    ret->lpInternetPriorityVtbl = &GopherPriorityVtbl;
    ret->ref = 1;

    *ppobj = PROTOCOL(ret);

    return S_OK;
}
