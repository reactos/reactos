/*
 * Copyright 2005 Jacek Caban
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "urlmon.h"
#include "urlmon_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    const IInternetProtocolVtbl *lpInternetProtocolVtbl;
    const IInternetPriorityVtbl *lpInternetPriorityVtbl;

    LONG priority;

    LONG ref;
} HttpProtocol;

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
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI HttpProtocol_Release(IInternetProtocol *iface)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
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
    FIXME("(%p)->(%s %p %p %08lx %ld)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08lx)\n", This, dwOptions);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrgin, ULARGE_INTEGER *plibNewPosition)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrgin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08lx)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpProtocol_UnlockRequest(IInternetProtocol *iface)
{
    HttpProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
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

    TRACE("(%p)->(%ld)\n", This, nPriority);

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

    ret->ref = 1;

    ret->priority = 0;

    *ppobj = PROTOCOL(ret);
    
    return S_OK;
}
