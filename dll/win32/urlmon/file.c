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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "urlmon_main.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct {
    IUnknown            IUnknown_inner;
    IInternetProtocolEx IInternetProtocolEx_iface;
    IInternetPriority   IInternetPriority_iface;

    IUnknown *outer;

    HANDLE file;
    ULONG size;
    LONG priority;

    LONG ref;
} FileProtocol;

static inline FileProtocol *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, FileProtocol, IUnknown_inner);
}

static inline FileProtocol *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, FileProtocol, IInternetProtocolEx_iface);
}

static inline FileProtocol *impl_from_IInternetPriority(IInternetPriority *iface)
{
    return CONTAINING_RECORD(iface, FileProtocol, IInternetPriority_iface);
}

static HRESULT WINAPI FileProtocolUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    FileProtocol *This = impl_from_IUnknown(iface);

    *ppv = NULL;
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
    }else if(IsEqualGUID(&IID_IInternetPriority, riid)) {
        TRACE("(%p)->(IID_IInternetPriority %p)\n", This, ppv);
        *ppv = &This->IInternetPriority_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("not supported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI FileProtocolUnk_AddRef(IUnknown *iface)
{
    FileProtocol *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI FileProtocolUnk_Release(IUnknown *iface)
{
    FileProtocol *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->file != INVALID_HANDLE_VALUE)
            CloseHandle(This->file);
        free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static const IUnknownVtbl FileProtocolUnkVtbl = {
    FileProtocolUnk_QueryInterface,
    FileProtocolUnk_AddRef,
    FileProtocolUnk_Release
};

static HRESULT WINAPI FileProtocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI FileProtocol_AddRef(IInternetProtocolEx *iface)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)\n", This);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI FileProtocol_Release(IInternetProtocolEx *iface)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    TRACE("(%p)\n", This);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI FileProtocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %08lx %Ix)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    hres = CreateUri(szUrl, Uri_CREATE_FILE_USE_DOS_PATH, 0, &uri);
    if(FAILED(hres))
        return hres;

    hres = IInternetProtocolEx_StartEx(&This->IInternetProtocolEx_iface, uri, pOIProtSink,
            pOIBindInfo, grfPI, (HANDLE*)dwReserved);

    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI FileProtocol_Continue(IInternetProtocolEx *iface, PROTOCOLDATA *pProtocolData)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Abort(IInternetProtocolEx *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI FileProtocol_Suspend(IInternetProtocolEx *iface)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Resume(IInternetProtocolEx *iface)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Read(IInternetProtocolEx *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    DWORD read = 0;

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    if (pcbRead)
        *pcbRead = 0;

    if(This->file == INVALID_HANDLE_VALUE)
        return INET_E_DATA_NOT_AVAILABLE;

    if (!ReadFile(This->file, pv, cb, &read, NULL))
        return INET_E_DOWNLOAD_FAILURE;

    if(pcbRead)
        *pcbRead = read;
    
    return cb == read ? S_OK : S_FALSE;
}

static HRESULT WINAPI FileProtocol_Seek(IInternetProtocolEx *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI FileProtocol_UnlockRequest(IInternetProtocolEx *iface)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

static inline HRESULT report_result(IInternetProtocolSink *protocol_sink, HRESULT hres, DWORD res)
{
    IInternetProtocolSink_ReportResult(protocol_sink, hres, res, NULL);
    return hres;
}

static HRESULT WINAPI FileProtocol_StartEx(IInternetProtocolEx *iface, IUri *pUri,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE *dwReserved)
{
    FileProtocol *This = impl_from_IInternetProtocolEx(iface);
    WCHAR path[MAX_PATH], *ptr;
    LARGE_INTEGER file_size;
    HANDLE file_handle;
    BINDINFO bindinfo;
    DWORD grfBINDF = 0;
    DWORD scheme, size;
    LPWSTR mime = NULL;
    BSTR ext;
    HRESULT hres;

    TRACE("(%p)->(%p %p %p %08lx %p)\n", This, pUri, pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    if(!pUri)
        return E_INVALIDARG;

    scheme = 0;
    hres = IUri_GetScheme(pUri, &scheme);
    if(FAILED(hres))
        return hres;
    if(scheme != URL_SCHEME_FILE)
        return E_INVALIDARG;

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &grfBINDF, &bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08lx\n", hres);
        return hres;
    }

    ReleaseBindInfo(&bindinfo);

    if(!(grfBINDF & BINDF_FROMURLMON))
        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_DIRECTBIND, NULL);

    if(This->file != INVALID_HANDLE_VALUE) {
        IInternetProtocolSink_ReportData(pOIProtSink,
                BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION,
                This->size, This->size);
        return S_OK;
    }

    IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_SENDINGREQUEST, L"");

    size = 0;
    hres = CoInternetParseIUri(pUri, PARSE_PATH_FROM_URL, 0, path, ARRAY_SIZE(path), &size, 0);
    if(FAILED(hres)) {
        WARN("CoInternetParseIUri failed: %08lx\n", hres);
        return report_result(pOIProtSink, hres, 0);
    }

    file_handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file_handle == INVALID_HANDLE_VALUE && (ptr = wcsrchr(path, '#'))) {
        /* If path contains fragment part, try without it. */
        *ptr = 0;
        file_handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    if(file_handle == INVALID_HANDLE_VALUE)
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, GetLastError());

    if(!GetFileSizeEx(file_handle, &file_size)) {
        CloseHandle(file_handle);
        return report_result(pOIProtSink, INET_E_RESOURCE_NOT_FOUND, GetLastError());
    }

    This->file = file_handle;
    This->size = file_size.u.LowPart;
    IInternetProtocolSink_ReportProgress(pOIProtSink,  BINDSTATUS_CACHEFILENAMEAVAILABLE, path);

    hres = IUri_GetExtension(pUri, &ext);
    if(SUCCEEDED(hres)) {
        if(hres == S_OK && *ext) {
            if((ptr = wcschr(ext, '#')))
                *ptr = 0;
            hres = find_mime_from_ext(ext, &mime);
            if(SUCCEEDED(hres)) {
                IInternetProtocolSink_ReportProgress(pOIProtSink,
                        (grfBINDF & BINDF_FROMURLMON) ?
                        BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE : BINDSTATUS_MIMETYPEAVAILABLE,
                        mime);
                CoTaskMemFree(mime);
            }
        }
        SysFreeString(ext);
    }

    IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION,
            This->size, This->size);

    return report_result(pOIProtSink, S_OK, 0);
}

static const IInternetProtocolExVtbl FileProtocolExVtbl = {
    FileProtocol_QueryInterface,
    FileProtocol_AddRef,
    FileProtocol_Release,
    FileProtocol_Start,
    FileProtocol_Continue,
    FileProtocol_Abort,
    FileProtocol_Terminate,
    FileProtocol_Suspend,
    FileProtocol_Resume,
    FileProtocol_Read,
    FileProtocol_Seek,
    FileProtocol_LockRequest,
    FileProtocol_UnlockRequest,
    FileProtocol_StartEx
};

static HRESULT WINAPI FilePriority_QueryInterface(IInternetPriority *iface,
                                                  REFIID riid, void **ppv)
{
    FileProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI FilePriority_AddRef(IInternetPriority *iface)
{
    FileProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI FilePriority_Release(IInternetPriority *iface)
{
    FileProtocol *This = impl_from_IInternetPriority(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI FilePriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    FileProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%ld)\n", This, nPriority);

    This->priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI FilePriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    FileProtocol *This = impl_from_IInternetPriority(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->priority;
    return S_OK;
}

static const IInternetPriorityVtbl FilePriorityVtbl = {
    FilePriority_QueryInterface,
    FilePriority_AddRef,
    FilePriority_Release,
    FilePriority_SetPriority,
    FilePriority_GetPriority
};

HRESULT FileProtocol_Construct(IUnknown *outer, LPVOID *ppobj)
{
    FileProtocol *ret;

    TRACE("(%p %p)\n", outer, ppobj);

    URLMON_LockModule();

    ret = malloc(sizeof(FileProtocol));

    ret->IUnknown_inner.lpVtbl = &FileProtocolUnkVtbl;
    ret->IInternetProtocolEx_iface.lpVtbl = &FileProtocolExVtbl;
    ret->IInternetPriority_iface.lpVtbl = &FilePriorityVtbl;
    ret->file = INVALID_HANDLE_VALUE;
    ret->priority = 0;
    ret->ref = 1;
    ret->outer = outer ? outer : &ret->IUnknown_inner;

    *ppobj = &ret->IUnknown_inner;
    return S_OK;
}
