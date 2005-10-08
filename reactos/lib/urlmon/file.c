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
    const IInternetProtocolVtbl  *lpInternetProtocolVtbl;

    HANDLE file;

    LONG ref;
} FileProtocol;

#define PROTOCOL_THIS(iface) DEFINE_THIS(FileProtocol, InternetProtocol, iface)

#define PROTOCOL(x)  ((IInternetProtocol*)  &(x)->lpInternetProtocolVtbl)

static HRESULT WINAPI FileProtocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    FileProtocol *This = PROTOCOL_THIS(iface);

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

static ULONG WINAPI FileProtocol_AddRef(IInternetProtocol *iface)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI FileProtocol_Release(IInternetProtocol *iface)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->file)
            CloseHandle(This->file);
        HeapFree(GetProcessHeap(), 0, This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI FileProtocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    BINDINFO bindinfo;
    DWORD grfBINDF = 0;
    LARGE_INTEGER size;
    DWORD len;
    LPWSTR url, mime = NULL;
    HRESULT hres;

    static const WCHAR wszFile[]  = {'f','i','l','e',':'};

    TRACE("(%p)->(%s %p %p %08lx %ld)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    IInternetBindInfo_GetBindInfo(pOIBindInfo, &grfBINDF, &bindinfo);

    if(lstrlenW(szUrl) < sizeof(wszFile)/sizeof(WCHAR)
            || memcmp(szUrl, wszFile, sizeof(wszFile)))
        return MK_E_SYNTAX;

    len = lstrlenW(szUrl)+16;
    url = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    hres = CoInternetParseUrl(szUrl, PARSE_ENCODE, 0, url, len, &len, 0);
    if(FAILED(hres)) {
        HeapFree(GetProcessHeap(), 0, url);
        return hres;
    }

    hres = FindMimeFromData(NULL, url, NULL, 0, NULL, 0, &mime, 0);
    if(SUCCEEDED(hres))
        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_DIRECTBIND, mime);

    if(!This->file) {
        This->file = CreateFileW(url+sizeof(wszFile)/sizeof(WCHAR), GENERIC_READ, FILE_SHARE_READ,
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(This->file == INVALID_HANDLE_VALUE) {
            This->file = NULL;
            IInternetProtocolSink_ReportResult(pOIProtSink, INET_E_RESOURCE_NOT_FOUND,
                    GetLastError(), NULL);
            HeapFree(GetProcessHeap(), 0, url);
            CoTaskMemFree(mime);
            return INET_E_RESOURCE_NOT_FOUND;
        }

        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_CACHEFILENAMEAVAILABLE,
                url+sizeof(wszFile)/sizeof(WCHAR));
        if(mime)
            IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_MIMETYPEAVAILABLE, mime);
        IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);
    }

    CoTaskMemFree(mime);
    HeapFree(GetProcessHeap(), 0, url);

    if(GetFileSizeEx(This->file, &size))
        IInternetProtocolSink_ReportData(pOIProtSink,
                BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION,
                size.u.LowPart, size.u.LowPart);

    return S_OK;
}

static HRESULT WINAPI FileProtocol_Continue(IInternetProtocol *iface, PROTOCOLDATA *pProtocolData)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pProtocolData);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%08lx %08lx)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    FileProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI FileProtocol_Suspend(IInternetProtocol *iface)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Resume(IInternetProtocol *iface)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    DWORD read = 0;

    TRACE("(%p)->(%p %lu %p)\n", This, pv, cb, pcbRead);

    if(!This->file)
        return INET_E_DATA_NOT_AVAILABLE;

    ReadFile(This->file, pv, cb, &read, NULL);

    if(pcbRead)
        *pcbRead = read;
    
    return cb == read ? S_OK : S_FALSE;
}

static HRESULT WINAPI FileProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrgin, ULARGE_INTEGER *plibNewPosition)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrgin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    FileProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08lx)\n", This, dwOptions);

    return S_OK;
}

static HRESULT WINAPI FileProtocol_UnlockRequest(IInternetProtocol *iface)
{
    FileProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)\n", This);

    return S_OK;
}

#undef PROTOCOL_THIS

static const IInternetProtocolVtbl FileProtocolVtbl = {
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
    FileProtocol_UnlockRequest
};

HRESULT FileProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    FileProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(FileProtocol));

    ret->lpInternetProtocolVtbl = &FileProtocolVtbl;
    ret->file = NULL;
    ret->ref = 1;

    *ppobj = PROTOCOL(ret);
    
    return S_OK;
}
