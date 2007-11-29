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
    const IInternetPriorityVtbl  *lpInternetPriorityVtbl;

    HANDLE file;
    LONG priority;

    LONG ref;
} FileProtocol;

#define PROTOCOL(x)  ((IInternetProtocol*)  &(x)->lpInternetProtocolVtbl)
#define PRIORITY(x)  ((IInternetPriority*)  &(x)->lpInternetPriorityVtbl)

#define PROTOCOL_THIS(iface) DEFINE_THIS(FileProtocol, InternetProtocol, iface)

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

static ULONG WINAPI FileProtocol_AddRef(IInternetProtocol *iface)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI FileProtocol_Release(IInternetProtocol *iface)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->file)
            CloseHandle(This->file);
        urlmon_free(This);

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
    LPCWSTR file_name;
    WCHAR null_char = 0;
    BOOL first_call = FALSE;
    HRESULT hres;

    static const WCHAR wszFile[]  = {'f','i','l','e',':'};

    TRACE("(%p)->(%s %p %p %08x %d)\n", This, debugstr_w(szUrl), pOIProtSink,
            pOIBindInfo, grfPI, dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(BINDINFO);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &grfBINDF, &bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08x\n", hres);
        return hres;
    }

    ReleaseBindInfo(&bindinfo);

    if(lstrlenW(szUrl) < sizeof(wszFile)/sizeof(WCHAR)
            || memcmp(szUrl, wszFile, sizeof(wszFile)))
        return MK_E_SYNTAX;

    len = lstrlenW(szUrl)+16;
    url = urlmon_alloc(len*sizeof(WCHAR));
    hres = CoInternetParseUrl(szUrl, PARSE_ENCODE, 0, url, len, &len, 0);
    if(FAILED(hres)) {
        urlmon_free(url);
        return hres;
    }

    if(!(grfBINDF & BINDF_FROMURLMON))
        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_DIRECTBIND, NULL);

    if(!This->file) {
        first_call = TRUE;

        IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_SENDINGREQUEST, &null_char);

        file_name = url+sizeof(wszFile)/sizeof(WCHAR);
        if(file_name[0] == '/' && file_name[1] == '/')
            file_name += 2;
        if(*file_name == '/')
            file_name++;

        This->file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(This->file == INVALID_HANDLE_VALUE) {
            This->file = NULL;
            IInternetProtocolSink_ReportResult(pOIProtSink, INET_E_RESOURCE_NOT_FOUND,
                    GetLastError(), NULL);
            urlmon_free(url);
            return INET_E_RESOURCE_NOT_FOUND;
        }

        IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_CACHEFILENAMEAVAILABLE, file_name);

        hres = FindMimeFromData(NULL, url, NULL, 0, NULL, 0, &mime, 0);
        if(SUCCEEDED(hres)) {
            IInternetProtocolSink_ReportProgress(pOIProtSink,
                    (grfBINDF & BINDF_FROMURLMON) ?
                    BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE : BINDSTATUS_MIMETYPEAVAILABLE,
                    mime);
            CoTaskMemFree(mime);
        }
    }

    urlmon_free(url);

    if(GetFileSizeEx(This->file, &size))
        IInternetProtocolSink_ReportData(pOIProtSink,
                BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION,
                size.u.LowPart, size.u.LowPart);

    if(first_call)
        IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);

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
    FIXME("(%p)->(%08x %08x)\n", This, hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    FileProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

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

    TRACE("(%p)->(%p %u %p)\n", This, pv, cb, pcbRead);

    if(!This->file)
        return INET_E_DATA_NOT_AVAILABLE;

    ReadFile(This->file, pv, cb, &read, NULL);

    if(pcbRead)
        *pcbRead = read;
    
    return cb == read ? S_OK : S_FALSE;
}

static HRESULT WINAPI FileProtocol_Seek(IInternetProtocol *iface, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    FileProtocol *This = PROTOCOL_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI FileProtocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    FileProtocol *This = PROTOCOL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwOptions);

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

#define PRIORITY_THIS(iface) DEFINE_THIS(FileProtocol, InternetPriority, iface)

static HRESULT WINAPI FilePriority_QueryInterface(IInternetPriority *iface,
                                                  REFIID riid, void **ppv)
{
    FileProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_QueryInterface(PROTOCOL(This), riid, ppv);
}

static ULONG WINAPI FilePriority_AddRef(IInternetPriority *iface)
{
    FileProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_AddRef(PROTOCOL(This));
}

static ULONG WINAPI FilePriority_Release(IInternetPriority *iface)
{
    FileProtocol *This = PRIORITY_THIS(iface);
    return IInternetProtocol_Release(PROTOCOL(This));
}

static HRESULT WINAPI FilePriority_SetPriority(IInternetPriority *iface, LONG nPriority)
{
    FileProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%d)\n", This, nPriority);

    This->priority = nPriority;
    return S_OK;
}

static HRESULT WINAPI FilePriority_GetPriority(IInternetPriority *iface, LONG *pnPriority)
{
    FileProtocol *This = PRIORITY_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pnPriority);

    *pnPriority = This->priority;
    return S_OK;
}

#undef PRIORITY_THIS

static const IInternetPriorityVtbl FilePriorityVtbl = {
    FilePriority_QueryInterface,
    FilePriority_AddRef,
    FilePriority_Release,
    FilePriority_SetPriority,
    FilePriority_GetPriority
};

HRESULT FileProtocol_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    FileProtocol *ret;

    TRACE("(%p %p)\n", pUnkOuter, ppobj);

    URLMON_LockModule();

    ret = urlmon_alloc(sizeof(FileProtocol));

    ret->lpInternetProtocolVtbl = &FileProtocolVtbl;
    ret->lpInternetPriorityVtbl = &FilePriorityVtbl;
    ret->file = NULL;
    ret->priority = 0;
    ret->ref = 1;

    *ppobj = PROTOCOL(ret);
    
    return S_OK;
}
