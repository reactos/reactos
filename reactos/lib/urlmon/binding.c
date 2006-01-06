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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "urlmon.h"
#include "urlmon_main.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    const IBindingVtbl               *lpBindingVtbl;
    const IInternetProtocolSinkVtbl  *lpInternetProtocolSinkVtbl;
    const IInternetBindInfoVtbl      *lpInternetBindInfoVtbl;
    const IServiceProviderVtbl       *lpServiceProviderVtbl;

    LONG ref;

    IBindStatusCallback *callback;
    IInternetProtocol *protocol;
    IStream *stream;

    BINDINFO bindinfo;
    DWORD bindf;
    LPWSTR mime;
    LPWSTR url;
} Binding;

#define BINDING(x)   ((IBinding*)               &(x)->lpBindingVtbl)
#define PROTSINK(x)  ((IInternetProtocolSink*)  &(x)->lpInternetProtocolSinkVtbl)
#define BINDINF(x)   ((IInternetBindInfo*)      &(x)->lpInternetBindInfoVtbl)
#define SERVPROV(x)  ((IServiceProvider*)       &(x)->lpServiceProviderVtbl)


#define BINDING_THIS(iface) DEFINE_THIS(Binding, Binding, iface)

static HRESULT WINAPI Binding_QueryInterface(IBinding *iface, REFIID riid, void **ppv)
{
    Binding *This = BINDING_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = BINDING(This);
    }else if(IsEqualGUID(&IID_IBinding, riid)) {
        TRACE("(%p)->(IID_IBinding %p)\n", This, ppv);
        *ppv = BINDING(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = PROTSINK(This);
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINF(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }

    if(*ppv)
        return S_OK;

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Binding_AddRef(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI Binding_Release(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->callback)
            IBindStatusCallback_Release(This->callback);
        if(This->protocol)
            IInternetProtocol_Release(This->protocol);
        if(This->stream)
            IStream_Release(This->stream);

        ReleaseBindInfo(&This->bindinfo);
        HeapFree(GetProcessHeap(), 0, This->mime);
        HeapFree(GetProcessHeap(), 0, This->url);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI Binding_Abort(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Suspend(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Resume(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_SetPriority(IBinding *iface, LONG nPriority)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, nPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetPriority(IBinding *iface, LONG *pnPriority)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetBindResult(IBinding *iface, CLSID *pclsidProtocol,
        DWORD *pdwResult, LPOLESTR *pszResult, DWORD *pdwReserved)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%p %p %p %p)\n", This, pclsidProtocol, pdwResult, pszResult, pdwReserved);
    return E_NOTIMPL;
}

#undef BINDING_THIS

static const IBindingVtbl BindingVtbl = {
    Binding_QueryInterface,
    Binding_AddRef,
    Binding_Release,
    Binding_Abort,
    Binding_Suspend,
    Binding_Resume,
    Binding_SetPriority,
    Binding_GetPriority,
    Binding_GetBindResult
};

#define PROTSINK_THIS(iface) DEFINE_THIS(Binding, InternetProtocolSink, iface)

static HRESULT WINAPI InternetProtocolSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI InternetProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI InternetProtocolSink_Release(IInternetProtocolSink *iface)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI InternetProtocolSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    Binding *This = PROTSINK_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetProtocolSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%lu %s)\n", This, ulStatusCode, debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_MIMETYPEAVAILABLE: {
        int len = strlenW(szStatusText)+1;
        This->mime = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        memcpy(This->mime, szStatusText, len*sizeof(WCHAR));
        break;
    }
    case BINDSTATUS_SENDINGREQUEST:
        IBindStatusCallback_OnProgress(This->callback, 0, 0, BINDSTATUS_SENDINGREQUEST,
                                       szStatusText);
        break;
    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        IBindStatusCallback_OnProgress(This->callback, 0, 0,
                                       BINDSTATUS_MIMETYPEAVAILABLE, szStatusText);
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        break;
    default:
        FIXME("Unhandled status code %ld\n", ulStatusCode);
        return E_NOTIMPL;
    };

    return S_OK;
}

static HRESULT WINAPI InternetProtocolSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    Binding *This = PROTSINK_THIS(iface);
    DWORD read = 0, cread;
    STGMEDIUM stgmed;
    BYTE buf[1024];

    TRACE("(%p)->(%ld %lu %lu)\n", This, grfBSCF, ulProgress, ulProgressMax);

    if(grfBSCF & BSCF_FIRSTDATANOTIFICATION) {
        if(This->mime)
            IBindStatusCallback_OnProgress(This->callback, ulProgress, ulProgressMax,
                                           BINDSTATUS_MIMETYPEAVAILABLE, This->mime);
        IBindStatusCallback_OnProgress(This->callback, ulProgress, ulProgressMax,
                                       BINDSTATUS_BEGINDOWNLOADDATA, This->url);
    }

    if(grfBSCF & BSCF_LASTDATANOTIFICATION)
        IBindStatusCallback_OnProgress(This->callback, ulProgress, ulProgressMax,
                                       BINDSTATUS_ENDDOWNLOADDATA, This->url);

    if(grfBSCF & BSCF_FIRSTDATANOTIFICATION)
        IInternetProtocol_LockRequest(This->protocol, 0);
    do {
        cread = 0;
        IInternetProtocol_Read(This->protocol, buf, sizeof(buf), &cread);
        IStream_Write(This->stream, buf, read, NULL);
        read += cread;
    }while(cread);

    stgmed.tymed = TYMED_ISTREAM;
    stgmed.u.pstm = This->stream;

    IBindStatusCallback_OnDataAvailable(This->callback, grfBSCF, read,
            NULL /* FIXME */, &stgmed);

    if(grfBSCF & BSCF_LASTDATANOTIFICATION)
        IBindStatusCallback_OnStopBinding(This->callback, S_OK, NULL);

    return S_OK;
}

static HRESULT WINAPI InternetProtocolSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    Binding *This = PROTSINK_THIS(iface);
    FIXME("(%p)->(%08lx %ld %s)\n", This, hrResult, dwError, debugstr_w(szResult));
    return E_NOTIMPL;
}

#undef PROTSINK_THIS

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    InternetProtocolSink_QueryInterface,
    InternetProtocolSink_AddRef,
    InternetProtocolSink_Release,
    InternetProtocolSink_Switch,
    InternetProtocolSink_ReportProgress,
    InternetProtocolSink_ReportData,
    InternetProtocolSink_ReportResult
};

#define BINDINF_THIS(iface) DEFINE_THIS(Binding, InternetBindInfo, iface)

static HRESULT WINAPI InternetBindInfo_QueryInterface(IInternetBindInfo *iface,
        REFIID riid, void **ppv)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI InternetBindInfo_AddRef(IInternetBindInfo *iface)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI InternetBindInfo_Release(IInternetBindInfo *iface)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI InternetBindInfo_GetBindInfo(IInternetBindInfo *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    Binding *This = BINDINF_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = This->bindf;

    memcpy(pbindinfo, &This->bindinfo, sizeof(BINDINFO));

    if(pbindinfo->szExtraInfo || pbindinfo->szCustomVerb)
        FIXME("copy strings\n");

    if(pbindinfo->pUnk)
        IUnknown_AddRef(pbindinfo->pUnk);

    return S_OK;
}

static HRESULT WINAPI InternetBindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    Binding *This = BINDINF_THIS(iface);
    FIXME("(%p)->(%ld %p %ld %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);
    return E_NOTIMPL;
}

#undef BINDF_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    InternetBindInfo_QueryInterface,
    InternetBindInfo_AddRef,
    InternetBindInfo_Release,
    InternetBindInfo_GetBindInfo,
    InternetBindInfo_GetBindString
};

#define SERVPROV_THIS(iface) DEFINE_THIS(Binding, ServiceProvider, iface)

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    Binding *This = SERVPROV_THIS(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static HRESULT get_callback(IBindCtx *pbc, IBindStatusCallback **callback)
{
    HRESULT hres;

    static WCHAR wszBSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

    hres = IBindCtx_GetObjectParam(pbc, wszBSCBHolder, (IUnknown**)callback);
    if(FAILED(hres))
        return MK_E_SYNTAX;

    return S_OK;
}

static HRESULT get_protocol(Binding *This, LPCWSTR url)
{
    IUnknown *unk = NULL;
    IClassFactory *cf = NULL;
    HRESULT hres;

    hres = IBindStatusCallback_QueryInterface(This->callback, &IID_IInternetProtocol,
            (void**)&This->protocol);
    if(SUCCEEDED(hres))
        return S_OK;

    hres = get_protocol_iface(url, &unk);
    if(FAILED(hres))
        return hres;

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&cf);
    IUnknown_Release(unk);
    if(FAILED(hres))
        return hres;

    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IInternetProtocol, (void**)&This->protocol);
    IClassFactory_Release(cf);

    return hres;
}

static HRESULT Binding_Create(LPCWSTR url, IBindCtx *pbc, REFIID riid, Binding **binding)
{
    Binding *ret;
    int len;
    HRESULT hres;

    static const WCHAR wszFile[] = {'f','i','l','e',':'};

    if(!IsEqualGUID(&IID_IStream, riid)) {
        FIXME("Unsupported riid %s\n", debugstr_guid(riid));
        return E_NOTIMPL;
    }

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(Binding));

    ret->lpBindingVtbl              = &BindingVtbl;
    ret->lpInternetProtocolSinkVtbl = &InternetProtocolSinkVtbl;
    ret->lpInternetBindInfoVtbl     = &InternetBindInfoVtbl;
    ret->lpServiceProviderVtbl      = &ServiceProviderVtbl;

    ret->ref = 1;

    ret->callback = NULL;
    ret->protocol = NULL;
    ret->stream = NULL;
    ret->mime = NULL;
    ret->url = NULL;

    memset(&ret->bindinfo, 0, sizeof(BINDINFO));
    ret->bindinfo.cbSize = sizeof(BINDINFO);
    ret->bindf = 0;

    hres = get_callback(pbc, &ret->callback);
    if(FAILED(hres)) {
        WARN("Could not get IBindStatusCallback\n");
        IBinding_Release(BINDING(ret));
        return hres;
    }

    hres = get_protocol(ret, url);
    if(FAILED(hres)) {
        WARN("Could not get protocol handler\n");
        IBinding_Release(BINDING(ret));
        return hres;
    }

    hres = IBindStatusCallback_GetBindInfo(ret->callback, &ret->bindf, &ret->bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08lx\n", hres);
        IBinding_Release(BINDING(ret));
        return hres;
    }

    ret->bindf |= BINDF_FROMURLMON;

    len = strlenW(url)+1;

    if(len < sizeof(wszFile)/sizeof(WCHAR) || memcmp(wszFile, url, sizeof(wszFile)))
        ret->bindf |= BINDF_NEEDFILE;

    ret->url = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    memcpy(ret->url, url, len*sizeof(WCHAR));

    CreateStreamOnHGlobal(NULL, TRUE, &ret->stream);

    *binding = ret;
    return S_OK;
}

HRESULT start_binding(LPCWSTR url, IBindCtx *pbc, REFIID riid, void **ppv)
{
    Binding *binding = NULL;
    HRESULT hres;

    *ppv = NULL;

    hres = Binding_Create(url, pbc, riid, &binding);
    if(FAILED(hres))
        return hres;

    hres = IBindStatusCallback_OnStartBinding(binding->callback, 0, BINDING(binding));
    if(FAILED(hres)) {
        WARN("OnStartBinding failed: %08lx\n", hres);
        IBindStatusCallback_OnStopBinding(binding->callback, 0x800c0008, NULL);
        IBinding_Release(BINDING(binding));
        return hres;
    }

    hres = IInternetProtocol_Start(binding->protocol, url, PROTSINK(binding),
             BINDINF(binding), 0, 0);
    IInternetProtocol_Terminate(binding->protocol, 0);

    if(SUCCEEDED(hres)) {
        IInternetProtocol_UnlockRequest(binding->protocol);
    }else {
        WARN("Start failed: %08lx\n", hres);
        IBindStatusCallback_OnStopBinding(binding->callback, S_OK, NULL);
    }

    IStream_AddRef(binding->stream);
    *ppv = binding->stream;

    IBinding_Release(BINDING(binding));

    return hres;
}
