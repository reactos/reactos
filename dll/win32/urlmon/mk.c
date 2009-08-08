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
    const IInternetProtocolVtbl  *lpInternetProtocolVtbl;

    LONG ref;

    IStream *stream;
} MkProtocol;

#define PROTOCOL_THIS(iface) DEFINE_THIS(MkProtocol, InternetProtocol, iface)

#define PROTOCOL(x)  ((IInternetProtocol*)  &(x)->lpInternetProtocolVtbl)

static HRESULT WINAPI MkProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    MkProtocol *This = PROTOCOL_THIS(iface);

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
    }

    if(*ppv) {
        IInternetProtocol_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MkProtocol_AddRef(IInternetProtocol *iface)
{
    MkProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI MkProtocol_Release(IInternetProtocol *iface)
{
    MkProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->stream)
            IStream_Release(This->stream);

        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT report_result(IInternetProtocolSink *sink, HRESULT hres, DWORD dwError)
{
    IInternetProtocolSink_ReportResult(sink, hres, dwError, NULL);
    return hres;
}

static HRESULT WINAPI MkProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    MkProtocol *This = PROTOCOL_THIS(iface);
    IParseDisplayName *pdn;
    IMoniker *mon;
    LPWSTR mime, progid, display_name;
    LPCWSTR ptr, ptr2;
    BINDINFO bindinfo;
    STATSTG statstg;
    DWORD bindf=0, eaten=0, len;
    CLSID clsid;
    HRESULT hres;

    static const WCHAR wszMK[] = {'m','k',':','@'};

    TRACE("(%p)->(%s %p %p %08x %d)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    if(strncmpiW(szUrl, wszMK, sizeof(wszMK)/sizeof(WCHAR)))
        return INET_E_INVALID_URL;

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08x\n", hres);
        return hres;
    }

    ReleaseBindInfo(&bindinfo);

    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_SENDINGREQUEST, NULL);

    hres = FindMimeFromData(NULL, szUrl, NULL, 0, NULL, 0, &mime, 0);
    if(SUCCEEDED(hres)) {
        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_MIMETYPEAVAILABLE, mime);
        CoTaskMemFree(mime);
    }

    ptr2 = szUrl + sizeof(wszMK)/sizeof(WCHAR);
    ptr = strchrW(ptr2, ':');
    if(!ptr)
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, ERROR_INVALID_PARAMETER);

    progid = heap_alloc((ptr-ptr2+1)*sizeof(WCHAR));
    memcpy(progid, ptr2, (ptr-ptr2)*sizeof(WCHAR));
    progid[ptr-ptr2] = 0;
    hres = CLSIDFromProgID(progid, &clsid);
    heap_free(progid);
    if(FAILED(hres))
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, ERROR_INVALID_PARAMETER);

    hres = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IParseDisplayName, (void**)&pdn);
    if(FAILED(hres)) {
        WARN("Could not create object %s\n", debugstr_guid(&clsid));
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

    len = strlenW(--ptr2);
    display_name = heap_alloc((len+1)*sizeof(WCHAR));
    memcpy(display_name, ptr2, (len+1)*sizeof(WCHAR));
    hres = IParseDisplayName_ParseDisplayName(pdn, NULL /* FIXME */, display_name, &eaten, &mon);
    heap_free(display_name);
    IParseDisplayName_Release(pdn);
    if(FAILED(hres)) {
        WARN("ParseDisplayName failed: %08x\n", hres);
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

    if(This->stream) {
        IStream_Release(This->stream);
        This->stream = NULL;
    }

    hres = IMoniker_BindToStorage(mon, NULL /* FIXME */, NULL, &IID_IStream, (void**)&This->stream);
    IMoniker_Release(mon);
    if(FAILED(hres)) {
        WARN("BindToStorage failed: %08x\n", hres);
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

    hres = IStream_Stat(This->stream, &statstg, STATFLAG_NONAME);
    if(FAILED(hres)) {
        WARN("Stat failed: %08x\n", hres);
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

    IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION,
            statstg.cbSize.u.LowPart, statstg.cbSize.u.LowPart);

    return report_result(pOIProtSink, S_OK, ERROR_SUCCESS);
}

static HRESULT WINAPI MkProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    MkProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    MkProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    MkProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI MkProtocol_Suspend(IInternetProtocol *iface)
{
    MkProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Resume(IInternetProtocol *iface)
{
    MkProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    MkProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if(!This->stream)
        return E_FAIL;

    return IStream_Read(This->stream, pv, cb, pcbRead);
}

static HRESULT WINAPI MkProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    MkProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    MkProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI MkProtocol_UnlockRequest(IInternetProtocol *iface)
{
    MkProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl MkProtocolVtbl = {
    MkProtocol_QueryInterface,
    MkProtocol_AddRef,
    MkProtocol_Release,
    MkProtocol_Start,
    MkProtocol_Continue,
    MkProtocol_Abort,
    MkProtocol_Terminate,
    MkProtocol_Suspend,
    MkProtocol_Resume,
    MkProtocol_Read,
    MkProtocol_Seek,
    MkProtocol_LockRequest,
    MkProtocol_UnlockRequest
};

HRESULT MkProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    MkProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = heap_alloc(sizeof(MkProtocol));

    ret->lpInternetProtocolVtbl = &MkProtocolVtbl;
    ret->ref = 1;
    ret->stream = NULL;

    /* NOTE:
     * Native returns NULL ppobj and S_OK in CreateInstance if called with IID_IUnknown riid.
     */
    *ppobj = PROTOCOL(ret);

    return S_OK;
}
