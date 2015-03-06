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

typedef struct {
    IInternetProtocolEx IInternetProtocolEx_iface;

    LONG ref;

    IStream *stream;
} MkProtocol;

static inline MkProtocol *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, MkProtocol, IInternetProtocolEx_iface);
}

static HRESULT WINAPI MkProtocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);

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
    }

    if(*ppv) {
        IInternetProtocolEx_AddRef(iface);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MkProtocol_AddRef(IInternetProtocolEx *iface)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI MkProtocol_Release(IInternetProtocolEx *iface)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
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

static HRESULT WINAPI MkProtocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    HRESULT hres;
    IUri *uri;

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
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

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

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if(!This->stream)
        return E_FAIL;

    return IStream_Read(This->stream, pv, cb, pcbRead);
}

static HRESULT WINAPI MkProtocol_Seek(IInternetProtocolEx *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI MkProtocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    MkProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

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
    DWORD path_size = INTERNET_MAX_URL_LENGTH;
    DWORD bindf=0, eaten=0, scheme=0, len;
    BSTR url, path_tmp, path = NULL;
    IParseDisplayName *pdn;
    BINDINFO bindinfo;
    STATSTG statstg;
    IMoniker *mon;
    HRESULT hres;
    CLSID clsid;

    TRACE("(%p)->(%p %p %p %08x %p)\n", This, pUri, pOIProtSink,
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
        WARN("GetBindInfo failed: %08x\n", hres);
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

    hres = IUri_GetPath(pUri, &path_tmp);
    if(FAILED(hres))
        return hres;
    path = heap_alloc(path_size);
    hres = UrlUnescapeW((LPWSTR)path_tmp, path, &path_size, 0);
    SysFreeString(path_tmp);
    if (FAILED(hres))
    {
        heap_free(path);
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, ERROR_INVALID_PARAMETER);
    }
    progid = path+1; /* skip '@' symbol */
    colon_ptr = strchrW(path, ':');
    if(!colon_ptr)
    {
        heap_free(path);
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, ERROR_INVALID_PARAMETER);
    }

    len = strlenW(path);
    display_name = heap_alloc((len+1)*sizeof(WCHAR));
    memcpy(display_name, path, (len+1)*sizeof(WCHAR));

    progid[colon_ptr-progid] = 0; /* overwrite ':' with NULL terminator */
    hres = CLSIDFromProgID(progid, &clsid);
    heap_free(path);
    if(FAILED(hres))
    {
        heap_free(display_name);
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, ERROR_INVALID_PARAMETER);
    }

    hres = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IParseDisplayName, (void**)&pdn);
    if(FAILED(hres)) {
        WARN("Could not create object %s\n", debugstr_guid(&clsid));
        heap_free(display_name);
        return report_result(pOIProtSink, hres, ERROR_INVALID_PARAMETER);
    }

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

HRESULT MkProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    MkProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = heap_alloc(sizeof(MkProtocol));

    ret->IInternetProtocolEx_iface.lpVtbl = &MkProtocolVtbl;
    ret->ref = 1;
    ret->stream = NULL;

    /* NOTE:
     * Native returns NULL ppobj and S_OK in CreateInstance if called with IID_IUnknown riid.
     */
    *ppobj = &ret->IInternetProtocolEx_iface;

    return S_OK;
}
