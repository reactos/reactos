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

#define PROTOCOL_THIS(iface) DEFINE_THIS(ProtocolProxy, IInternetProtocol, iface)

static HRESULT WINAPI ProtocolProxy_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);

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
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = PROTSINK(This);
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolProxy_AddRef(IInternetProtocol *iface)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI ProtocolProxy_Release(IInternetProtocol *iface)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->protocol_sink)
            IInternetProtocolSink_Release(This->protocol_sink);
        if(This->protocol)
            IInternetProtocol_Release(This->protocol);

        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI ProtocolProxy_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
          pOIBindInfo, grfPI, dwReserved);

    return IInternetProtocol_Start(This->protocol, szUrl, pOIProtSink, pOIBindInfo, grfPI, dwReserved);
}

static HRESULT WINAPI ProtocolProxy_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return IInternetProtocol_Continue(This->protocol, pProtocolData);
}

static HRESULT WINAPI ProtocolProxy_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolProxy_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return IInternetProtocol_Terminate(This->protocol, dwOptions);
}

static HRESULT WINAPI ProtocolProxy_Suspend(IInternetProtocol *iface)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolProxy_Resume(IInternetProtocol *iface)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolProxy_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    return IInternetProtocol_Read(This->protocol, pv, cb, pcbRead);
}

static HRESULT WINAPI ProtocolProxy_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolProxy_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return IInternetProtocol_LockRequest(This->protocol, dwOptions);
}

static HRESULT WINAPI ProtocolProxy_UnlockRequest(IInternetProtocol *iface)
{
    ProtocolProxy *This = PROTOCOL_THIS(iface);

    TRACE("(%p)\n", This);

    return IInternetProtocol_UnlockRequest(This->protocol);
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl ProtocolProxyVtbl = {
    ProtocolProxy_QueryInterface,
    ProtocolProxy_AddRef,
    ProtocolProxy_Release,
    ProtocolProxy_Start,
    ProtocolProxy_Continue,
    ProtocolProxy_Abort,
    ProtocolProxy_Terminate,
    ProtocolProxy_Suspend,
    ProtocolProxy_Resume,
    ProtocolProxy_Read,
    ProtocolProxy_Seek,
    ProtocolProxy_LockRequest,
    ProtocolProxy_UnlockRequest
};

#define PROTSINK_THIS(iface) DEFINE_THIS(ProtocolProxy, IInternetProtocolSink, iface)

static HRESULT WINAPI ProtocolProxySink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    ProtocolProxy *This = PROTSINK_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI ProtocolProxySink_AddRef(IInternetProtocolSink *iface)
{
    ProtocolProxy *This = PROTSINK_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI ProtocolProxySink_Release(IInternetProtocolSink *iface)
{
    ProtocolProxy *This = PROTSINK_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI ProtocolProxySink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    ProtocolProxy *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return IInternetProtocolSink_Switch(This->protocol_sink, pProtocolData);
}

static HRESULT WINAPI ProtocolProxySink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    ProtocolProxy *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%u %s)\n", This, ulStatusCode, debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        IInternetProtocolSink_ReportProgress(This->protocol_sink, BINDSTATUS_MIMETYPEAVAILABLE, szStatusText);
        break;
    default:
        IInternetProtocolSink_ReportProgress(This->protocol_sink, ulStatusCode, szStatusText);
    }

    return S_OK;
}

static HRESULT WINAPI ProtocolProxySink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    ProtocolProxy *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%d %u %u)\n", This, grfBSCF, ulProgress, ulProgressMax);

    return IInternetProtocolSink_ReportData(This->protocol_sink, grfBSCF, ulProgress, ulProgressMax);
}

static HRESULT WINAPI ProtocolProxySink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    ProtocolProxy *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%08x %d %s)\n", This, hrResult, dwError, debugstr_w(szResult));

    return IInternetProtocolSink_ReportResult(This->protocol_sink, hrResult, dwError, szResult);
}

#undef PROTSINK_THIS

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    ProtocolProxySink_QueryInterface,
    ProtocolProxySink_AddRef,
    ProtocolProxySink_Release,
    ProtocolProxySink_Switch,
    ProtocolProxySink_ReportProgress,
    ProtocolProxySink_ReportData,
    ProtocolProxySink_ReportResult
};

HRESULT create_protocol_proxy(IInternetProtocol *protocol, IInternetProtocolSink *protocol_sink, ProtocolProxy **ret)
{
    ProtocolProxy *sink;

    sink = heap_alloc(sizeof(ProtocolProxy));
    if(!sink)
        return E_OUTOFMEMORY;

    sink->lpIInternetProtocolVtbl     = &ProtocolProxyVtbl;
    sink->lpIInternetProtocolSinkVtbl = &InternetProtocolSinkVtbl;
    sink->ref = 1;

    IInternetProtocol_AddRef(protocol);
    sink->protocol = protocol;

    IInternetProtocolSink_AddRef(protocol_sink);
    sink->protocol_sink = protocol_sink;

    *ret = sink;
    return S_OK;
}
