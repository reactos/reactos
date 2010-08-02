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
    const IInternetProtocolVtbl      *lpIInternetProtocolVtbl;
    const IInternetProtocolSinkVtbl  *lpIInternetProtocolSinkVtbl;

    LONG ref;
} MimeFilter;

#define PROTOCOL(x)      ((IInternetProtocol*)      &(x)->lpIInternetProtocolVtbl)
#define PROTOCOLSINK(x)  ((IInternetProtocolSink*)  &(x)->lpIInternetProtocolSinkVtbl)

#define PROTOCOL_THIS(iface) DEFINE_THIS(MimeFilter, IInternetProtocol, iface)

static HRESULT WINAPI MimeFilterProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    MimeFilter *This = PROTOCOL_THIS(iface);

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
        *ppv = PROTOCOLSINK(This);
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MimeFilterProtocol_AddRef(IInternetProtocol *iface)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI MimeFilterProtocol_Release(IInternetProtocol *iface)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI MimeFilterProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%s %p %p %08x %lx)\n", This, debugstr_w(szUrl), pOIProtSink,
          pOIBindInfo, grfPI, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Suspend(IInternetProtocol *iface)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Resume(IInternetProtocol *iface)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x)\n", This, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterProtocol_UnlockRequest(IInternetProtocol *iface)
{
    MimeFilter *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl MimeFilterProtocolVtbl = {
    MimeFilterProtocol_QueryInterface,
    MimeFilterProtocol_AddRef,
    MimeFilterProtocol_Release,
    MimeFilterProtocol_Start,
    MimeFilterProtocol_Continue,
    MimeFilterProtocol_Abort,
    MimeFilterProtocol_Terminate,
    MimeFilterProtocol_Suspend,
    MimeFilterProtocol_Resume,
    MimeFilterProtocol_Read,
    MimeFilterProtocol_Seek,
    MimeFilterProtocol_LockRequest,
    MimeFilterProtocol_UnlockRequest
};

#define PROTSINK_THIS(iface) DEFINE_THIS(MimeFilter, IInternetProtocolSink, iface)

static HRESULT WINAPI MimeFilterSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    MimeFilter *This = PROTSINK_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI MimeFilterSink_AddRef(IInternetProtocolSink *iface)
{
    MimeFilter *This = PROTSINK_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI MimeFilterSink_Release(IInternetProtocolSink *iface)
{
    MimeFilter *This = PROTSINK_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI MimeFilterSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    MimeFilter *This = PROTSINK_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    MimeFilter *This = PROTSINK_THIS(iface);
    FIXME("(%p)->(%u %s)\n", This, ulStatusCode, debugstr_w(szStatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    MimeFilter *This = PROTSINK_THIS(iface);
    FIXME("(%p)->(%d %u %u)\n", This, grfBSCF, ulProgress, ulProgressMax);
    return E_NOTIMPL;
}

static HRESULT WINAPI MimeFilterSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    MimeFilter *This = PROTSINK_THIS(iface);
    FIXME("(%p)->(%08x %d %s)\n", This, hrResult, dwError, debugstr_w(szResult));
    return E_NOTIMPL;
}

#undef PROTSINK_THIS

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    MimeFilterSink_QueryInterface,
    MimeFilterSink_AddRef,
    MimeFilterSink_Release,
    MimeFilterSink_Switch,
    MimeFilterSink_ReportProgress,
    MimeFilterSink_ReportData,
    MimeFilterSink_ReportResult
};

HRESULT MimeFilter_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    MimeFilter *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = heap_alloc_zero(sizeof(MimeFilter));

    ret->lpIInternetProtocolVtbl     = &MimeFilterProtocolVtbl;
    ret->lpIInternetProtocolSinkVtbl = &InternetProtocolSinkVtbl;
    ret->ref = 1;

    *ppobj = PROTOCOL(ret);
    return S_OK;
}
