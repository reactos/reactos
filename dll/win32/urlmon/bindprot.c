/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
    const IInternetProtocolVtbl *lpInternetProtocolVtbl;
    const IInternetBindInfoVtbl *lpInternetBindInfoVtbl;
    const IInternetPriorityVtbl *lpInternetPriorityVtbl;
    const IServiceProviderVtbl  *lpServiceProviderVtbl;
    const IInternetProtocolSinkVtbl *lpInternetProtocolSinkVtbl;

    LONG ref;

    IInternetProtocol *protocol;
    IInternetBindInfo *bind_info;
    IInternetProtocolSink *protocol_sink;
    IServiceProvider *service_provider;

    LONG priority;

    BOOL reported_result;
    BOOL from_urlmon;
} BindProtocol;

#define PROTOCOL(x)  ((IInternetProtocol*) &(x)->lpInternetProtocolVtbl)
#define BINDINFO(x)  ((IInternetBindInfo*) &(x)->lpInternetBindInfoVtbl)
#define PRIORITY(x)  ((IInternetPriority*) &(x)->lpInternetPriorityVtbl)
#define SERVPROV(x)  ((IServiceProvider*)  &(x)->lpServiceProviderVtbl)
#define PROTSINK(x)  ((IInternetProtocolSink*) &(x)->lpInternetProtocolSinkVtbl)

#define PROTOCOL_THIS(iface) DEFINE_THIS(BindProtocol, InternetProtocol, iface)

static HRESULT WINAPI BindProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

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
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINFO(This);
    }else if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        TRACE("(%p)->(IID_IInternetPriority %p)\n", This, ppv);
        *ppv = PRIORITY(This);
    }else if(IsEqualGUID(&IID_IAuthenticate, riid)) {
        FIXME("(%p)->(IID_IAuthenticate %p)\n", This, ppv);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
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

static ULONG WINAPI BindProtocol_AddRef(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI BindProtocol_Release(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->protocol)
            IInternetProtocol_Release(This->protocol);
        if(This->bind_info)
            IInternetBindInfo_Release(This->bind_info);

        set_binding_sink(PROTOCOL(This), NULL);
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI BindProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    IInternetProtocol *protocol = NULL;
    IInternetPriority *priority;
    IServiceProvider *service_provider;
    CLSID clsid = IID_NULL;
    LPOLESTR clsid_str;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %08x %d)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    if(!szUrl || !pOIProtSink || !pOIBindInfo)
        return E_INVALIDARG;

    hres = IInternetProtocolSink_QueryInterface(pOIProtSink, &IID_IServiceProvider,
                                                (void**)&service_provider);
    if(SUCCEEDED(hres)) {
        /* FIXME: What's protocol CLSID here? */
        IServiceProvider_QueryService(service_provider, &IID_IInternetProtocol,
                                      &IID_IInternetProtocol, (void**)&protocol);
        IServiceProvider_Release(service_provider);
    }

    if(!protocol) {
        IClassFactory *cf;
        IUnknown *unk;

        hres = get_protocol_handler(szUrl, &clsid, &cf);
        if(FAILED(hres))
            return hres;

        if(This->from_urlmon) {
            hres = IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocol, (void**)&protocol);
            IClassFactory_Release(cf);
            if(FAILED(hres))
                return hres;
        }else {
            hres = IClassFactory_CreateInstance(cf, (IUnknown*)BINDINFO(This),
                    &IID_IUnknown, (void**)&unk);
            IClassFactory_Release(cf);
            if(FAILED(hres))
                return hres;

            hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocol, (void**)&protocol);
            IUnknown_Release(unk);
            if(FAILED(hres))
                return hres;
        }
    }

    StringFromCLSID(&clsid, &clsid_str);
    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_PROTOCOLCLASSID, clsid_str);
    CoTaskMemFree(clsid_str);

    This->protocol = protocol;

    IInternetBindInfo_AddRef(pOIBindInfo);
    This->bind_info = pOIBindInfo;

    set_binding_sink(PROTOCOL(This), pOIProtSink);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority, (void**)&priority);
    if(SUCCEEDED(hres)) {
        IInternetPriority_SetPriority(priority, This->priority);
        IInternetPriority_Release(priority);
    }

    return IInternetProtocol_Start(protocol, szUrl, PROTSINK(This), BINDINFO(This), 0, 0);
}

static HRESULT WINAPI BindProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    return IInternetProtocol_Continue(This->protocol, pProtocolData);
}

static HRESULT WINAPI BindProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    if(!This->reported_result)
        return E_FAIL;

    IInternetProtocol_Terminate(This->protocol, 0);

    set_binding_sink(PROTOCOL(This), NULL);

    if(This->bind_info) {
        IInternetBindInfo_Release(This->bind_info);
        This->bind_info = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI BindProtocol_Suspend(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_Resume(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    ULONG read = 0;
    HRESULT hres;

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    hres = IInternetProtocol_Read(This->protocol, pv, cb, &read);

    *pcbRead = read;
    return hres;
}

static HRESULT WINAPI BindProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    BindProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return IInternetProtocol_LockRequest(This->protocol, dwOptions);
}

static HRESULT WINAPI BindProtocol_UnlockRequest(IInternetProtocol *iface)
{
    BindProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)\n", This);

    return IInternetProtocol_UnlockRequest(This->protocol);
}

void set_binding_sink(IInternetProtocol *bind_protocol, IInternetProtocolSink *sink)
{
    BindProtocol *This = PROTOCOL_THIS(bind_protocol);
    IInternetProtocolSink *prev_sink;
    IServiceProvider *service_provider = NULL;

    if(sink)
        IInternetProtocolSink_AddRef(sink);
    prev_sink = InterlockedExchangePointer((void**)&This->protocol_sink, sink);
    if(prev_sink)
        IInternetProtocolSink_Release(prev_sink);

    if(sink)
        IInternetProtocolSink_QueryInterface(sink, &IID_IServiceProvider, (void**)&service_provider);
    service_provider = InterlockedExchangePointer((void**)&This->service_provider, service_provider);
    if(service_provider)
        IServiceProvider_Release(service_provider);
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl BindProtocolVtbl = {
    BindProtocol_QueryInterface,
    BindProtocol_AddRef,
    BindProtocol_Release,
    BindProtocol_Start,
    BindProtocol_Continue,
    BindProtocol_Abort,
    BindProtocol_Terminate,
    BindProtocol_Suspend,
    BindProtocol_Resume,
    BindProtocol_Read,
    BindProtocol_Seek,
    BindProtocol_LockRequest,
    BindProtocol_UnlockRequest
};

#define BINDINFO_THIS(iface) DEFINE_THIS(BindProtocol, InternetBindInfo, iface)

static HRESULT WINAPI BindInfo_QueryInterface(IInternetBindInfo *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = BINDINFO_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI BindInfo_AddRef(IInternetBindInfo *iface)
{
    BindProtocol *This = BINDINFO_THIS(iface);
    return IBinding_AddRef(PROTOCOL(This));
}

static ULONG WINAPI BindInfo_Release(IInternetBindInfo *iface)
{
    BindProtocol *This = BINDINFO_THIS(iface);
    return IBinding_Release(PROTOCOL(This));
}

static HRESULT WINAPI BindInfo_GetBindInfo(IInternetBindInfo *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BindProtocol *This = BINDINFO_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    hres = IInternetBindInfo_GetBindInfo(This->bind_info, grfBINDF, pbindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08x\n", hres);
        return hres;
    }

    *grfBINDF |= BINDF_FROMURLMON;
    return hres;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    BindProtocol *This = BINDINFO_THIS(iface);

    TRACE("(%p)->(%d %p %d %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);

    return IInternetBindInfo_GetBindString(This->bind_info, ulStringType, ppwzStr, cEl, pcElFetched);
}

#undef BINDFO_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    BindInfo_QueryInterface,
    BindInfo_AddRef,
    BindInfo_Release,
    BindInfo_GetBindInfo,
    BindInfo_GetBindString
};

#define PRIORITY_THIS(iface) DEFINE_THIS(BindProtocol, InternetPriority, iface)

static HRESULT WINAPI InternetPriority_QueryInterface(IInternetPriority *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI InternetPriority_AddRef(IInternetPriority *iface)
{
    BindProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI InternetPriority_Release(IInternetPriority *iface)
{
    BindProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI InternetPriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    BindProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%d)\n", This, nPriority);

    This->priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI InternetPriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    BindProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->priority;
    return S_OK;
}

#undef PRIORITY_THIS

static const IInternetPriorityVtbl InternetPriorityVtbl = {
    InternetPriority_QueryInterface,
    InternetPriority_AddRef,
    InternetPriority_Release,
    InternetPriority_SetPriority,
    InternetPriority_GetPriority

};

#define PROTSINK_THIS(iface) DEFINE_THIS(BindProtocol, InternetProtocolSink, iface)

static HRESULT WINAPI BPInternetProtocolSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = PROTSINK_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI BPInternetProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    BindProtocol *This = PROTSINK_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI BPInternetProtocolSink_Release(IInternetProtocolSink *iface)
{
    BindProtocol *This = PROTSINK_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI BPInternetProtocolSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    BindProtocol *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pProtocolData);

    TRACE("flags %x state %x data %p cb %u\n", pProtocolData->grfFlags, pProtocolData->dwState,
          pProtocolData->pData, pProtocolData->cbData);

    if(!This->protocol_sink) {
        IInternetProtocol_Continue(This->protocol, pProtocolData);
        return S_OK;
    }

    return IInternetProtocolSink_Switch(This->protocol_sink, pProtocolData);
}

static HRESULT WINAPI BPInternetProtocolSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BindProtocol *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%u %s)\n", This, ulStatusCode, debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_FINDINGRESOURCE:
    case BINDSTATUS_CONNECTING:
    case BINDSTATUS_BEGINDOWNLOADDATA:
    case BINDSTATUS_SENDINGREQUEST:
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
    case BINDSTATUS_DIRECTBIND:
    case BINDSTATUS_MIMETYPEAVAILABLE:
        if(!This->protocol_sink)
            return S_OK;
        return IInternetProtocolSink_ReportProgress(This->protocol_sink,
                ulStatusCode, szStatusText);

    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        if(!This->protocol_sink)
            return S_OK;
        return IInternetProtocolSink_ReportProgress(This->protocol_sink,
                This->from_urlmon ? BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE : BINDSTATUS_MIMETYPEAVAILABLE,
                                                    szStatusText);
    default:
        FIXME("unsupported ulStatusCode %u\n", ulStatusCode);
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI BPInternetProtocolSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    BindProtocol *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%d %u %u)\n", This, grfBSCF, ulProgress, ulProgressMax);

    if(!This->protocol_sink)
        return S_OK;

    return IInternetProtocolSink_ReportData(This->protocol_sink, grfBSCF, ulProgress, ulProgressMax);
}

static HRESULT WINAPI BPInternetProtocolSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    BindProtocol *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%08x %d %s)\n", This, hrResult, dwError, debugstr_w(szResult));

    if(!This->protocol_sink)
        return E_FAIL;

    This->reported_result = TRUE;

    return IInternetProtocolSink_ReportResult(This->protocol_sink, hrResult, dwError, szResult);
}

#undef PROTSINK_THIS

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    BPInternetProtocolSink_QueryInterface,
    BPInternetProtocolSink_AddRef,
    BPInternetProtocolSink_Release,
    BPInternetProtocolSink_Switch,
    BPInternetProtocolSink_ReportProgress,
    BPInternetProtocolSink_ReportData,
    BPInternetProtocolSink_ReportResult
};

#define SERVPROV_THIS(iface) DEFINE_THIS(BindProtocol, ServiceProvider, iface)

static HRESULT WINAPI BPServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    BindProtocol *This = SERVPROV_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI BPServiceProvider_AddRef(IServiceProvider *iface)
{
    BindProtocol *This = SERVPROV_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI BPServiceProvider_Release(IServiceProvider *iface)
{
    BindProtocol *This = SERVPROV_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI BPServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    BindProtocol *This = SERVPROV_THIS(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(!This->service_provider)
        return E_NOINTERFACE;

    return IServiceProvider_QueryService(This->service_provider, guidService, riid, ppv);
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    BPServiceProvider_QueryInterface,
    BPServiceProvider_AddRef,
    BPServiceProvider_Release,
    BPServiceProvider_QueryService
};

HRESULT create_binding_protocol(LPCWSTR url, BOOL from_urlmon, IInternetProtocol **protocol)
{
    BindProtocol *ret = heap_alloc_zero(sizeof(BindProtocol));

    ret->lpInternetProtocolVtbl = &BindProtocolVtbl;
    ret->lpInternetBindInfoVtbl = &InternetBindInfoVtbl;
    ret->lpInternetPriorityVtbl = &InternetPriorityVtbl;
    ret->lpServiceProviderVtbl  = &ServiceProviderVtbl;
    ret->lpInternetProtocolSinkVtbl = &InternetProtocolSinkVtbl;

    ret->ref = 1;
    ret->from_urlmon = from_urlmon;

    URLMON_LockModule();

    *protocol = PROTOCOL(ret);
    return S_OK;
}
