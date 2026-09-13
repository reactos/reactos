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

#define NO_SHLWAPI_REG
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    IUnknown            IUnknown_inner;
    IInternetProtocolEx IInternetProtocolEx_iface;

    LONG ref;
    IUnknown *outer;

    IStream *stream;
} MkProtocol;

static inline MkProtocol *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, MkProtocol, IUnknown_inner);
}

static inline MkProtocol *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, MkProtocol, IInternetProtocolEx_iface);
}

static HRESULT WINAPI MkProtocolUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    MkProtocol *This = impl_from_IUnknown(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IUnknown_inner;
    }else if(IsEqualGUID(&IID_IInternetProtocolRoot, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolRoot %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        TRACE("(%p)->(IID_IInternetProtocol %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IInternetProtocolEx, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolEx %p)\n", This, ppv);
        *ppv = &This->IInternetProtocolEx_iface;
    }else {
        *ppv = NULL;
        WARN("not supported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MkProtocolUnk_AddRef(IUnknown *iface)
{
    MkProtocol *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI MkProtocolUnk_Release(IUnknown *iface)
{
    MkProtocol *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->stream)
            IStream_Release(This->stream);

        free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static const IUnknownVtbl MkProtocolUnkVtbl = {
    MkProtocolUnk_QueryInterface,
    MkProtocolUnk_AddRef,
    MkProtocolUnk_Release
};

static HRESULT WINAPI MkProtocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI MkProtocol_AddRef(IInternetProtocolEx *iface)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)\n", This);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI MkProtocol_Release(IInternetProtocolEx *iface)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)\n", This);
    return IUnknown_Release(This->outer);
}

static HRESULT report_result(IInternetProtocolSink *sink, HRESULT hres, DWORD dwError)
{
    IInternetProtocolSink_ReportResult(sink, hres, dwError, NULL);
    return hres;
}

static HRESULT WINAPI MkProtocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    HRESULT hres;
    IUri *uri;

    TRACE("(%p)->(%s %p %p %08lx %Ix)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    hres = CreateUri(szUrl, 0, 0, &uri);
    if(FAILED(hres))
        return hres;

    hres = IInternetProtocolEx_StartEx(&This->IInternetProtocolEx_iface, uri, pOIProtSink,
            pOIBindInfo, grfPI, (HANDLE*)dwReserved);

    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI MkProtocol_Continue(IInternetProtocolEx *iface, PROTOCOLDATA *pProtocolData)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Abort(IInternetProtocolEx *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI MkProtocol_Suspend(IInternetProtocolEx *iface)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Resume(IInternetProtocolEx *iface)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Read(IInternetProtocolEx *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    if(!This->stream)
        return E_FAIL;

    return IStream_Read(This->stream, pv, cb, pcbRead);
}

static HRESULT WINAPI MkProtocol_Seek(IInternetProtocolEx *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI MkProtocol_UnlockRequest(IInternetProtocolEx *iface)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

static HRESULT WINAPI MkProtocol_StartEx(IInternetProtocolEx *iface, IUri *pUri,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE *dwReserved)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    LPWSTR mime, progid, display_name, colon_ptr;
    DWORD bindf=0, eaten=0, scheme=0, len;
    BSTR url, path = NULL;
    IParseDisplayName *pdn;
    BINDINFO bindinfo;
    STATSTG statstg;
    IMoniker *mon;
    HRESULT hres;
    CLSID clsid;

    TRACE("(%p)->(%p %p %p %08lx %p)\n", This, pUri, pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    hres = IUri_GetScheme(pUri, &scheme);
    if(FAILED(hres))
        return hres;
    if(scheme != URL_SCHEME_MK)
        return INET_E_INVALID_URL;

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08lx\n", hres);
        return hres;
    }

    ReleaseBindInfo(&bindinfo);

    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_SENDINGREQUEST, NULL);

    hres = IUri_GetDisplayUri(pUri, &url);
    if(FAILED(hres))
        return hres;
    hres = FindMimeFromData(NULL, url, NULL, 0, NULL, 0, &mime, 0);
    SysFreeString(url);
    if(SUCCEEDED(hres)) {
        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_MIMETYPEAVAILABLE, mime);
        CoTaskMemFree(mime);
    }

    hres = IUri_GetPath(pUri, &path);
    if(FAILED(hres))
        return hres;
    len = SysStringLen(path)+1;
    hres = UrlUnescapeW(path, NULL, &len, URL_UNESCAPE_INPLACE);
    if (FAILED(hres)) {
        SysFreeString(path);
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, ERROR_INVALID_PARAMETER);
    }

    progid = path+1; /* skip '@' symbol */
    colon_ptr = wcschr(path, ':');
    if(!colon_ptr) {
        SysFreeString(path);
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, ERROR_INVALID_PARAMETER);
    }

    len = lstrlenW(path);
    display_name = malloc((len + 1) * sizeof(WCHAR));
    memcpy(display_name, path, (len+1)*sizeof(WCHAR));

    progid[colon_ptr-progid] = 0; /* overwrite ':' with NULL terminator */
    hres = CLSIDFromProgID(progid, &clsid);
    SysFreeString(path);
    if(FAILED(hres))
    {
        free(display_name);
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, ERROR_INVALID_PARAMETER);
    }

    hres = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IParseDisplayName, (void**)&pdn);
    if(FAILED(hres)) {
        WARN("Could not create object %s\n", debugstr_guid(&clsid));
        free(display_name);
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

    hres = IParseDisplayName_ParseDisplayName(pdn, NULL /* FIXME */, display_name, &eaten, &mon);
    free(display_name);
    IParseDisplayName_Release(pdn);
    if(FAILED(hres)) {
        WARN("ParseDisplayName failed: %08lx\n", hres);
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

    if(This->stream) {
        IStream_Release(This->stream);
        This->stream = NULL;
    }

    hres = IMoniker_BindToStorage(mon, NULL /* FIXME */, NULL, &IID_IStream, (void**)&This->stream);
    IMoniker_Release(mon);
    if(FAILED(hres)) {
        WARN("BindToStorage failed: %08lx\n", hres);
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

    hres = IStream_Stat(This->stream, &statstg, STATFLAG_NONAME);
    if(FAILED(hres)) {
        WARN("Stat failed: %08lx\n", hres);
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

    IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION,
            statstg.cbSize.u.LowPart, statstg.cbSize.u.LowPart);
    return report_result(pOIProtSink, S_OK, ERROR_SUCCESS);
}

static const IInternetProtocolExVtbl MkProtocolVtbl = {
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
    MkProtocol_UnlockRequest,
    MkProtocol_StartEx
};

HRESULT MkProtocol_Construct(IUnknown *outer, void **ppv)
{
    MkProtocol *ret;

    TRACE("(%p %p)\n", outer, ppv);

    URLMON_LockModule();

    ret = malloc(sizeof(MkProtocol));

    ret->IUnknown_inner.lpVtbl = &MkProtocolUnkVtbl;
    ret->IInternetProtocolEx_iface.lpVtbl = &MkProtocolVtbl;
    ret->ref = 1;
    ret->outer = outer ? outer : &ret->IUnknown_inner;
    ret->stream = NULL;

    /* NOTE:
     * Native returns NULL ppobj and S_OK in CreateInstance if called with IID_IUnknown riid and no outer.
     */
    *ppv = &ret->IUnknown_inner;
    return S_OK;
}
