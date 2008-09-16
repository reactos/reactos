/*
 * Copyright 2008 Piotr Caban
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

#define COBJMACROS
#define NONAMELESSUNION

#include "config.h"

#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml2.h"
#include "wininet.h"
#include "urlmon.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

struct bsc_t {
    const struct IBindStatusCallbackVtbl *lpVtbl;

    LONG ref;

    void *obj;
    HRESULT (*onDataAvailable)(void*,char*,DWORD);

    IBinding *binding;
    IStream *memstream;
};

static inline bsc_t *impl_from_IBindStatusCallback( IBindStatusCallback *iface )
{
    return (bsc_t *)((char*)iface - FIELD_OFFSET(bsc_t, lpVtbl));
}

static HRESULT WINAPI bsc_QueryInterface(
    IBindStatusCallback *iface,
    REFIID riid,
    LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IBindStatusCallback))
    {
        IBindStatusCallback_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI bsc_AddRef(
    IBindStatusCallback *iface )
{
    bsc_t *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI bsc_Release(
    IBindStatusCallback *iface )
{
    bsc_t *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->binding)
            IBinding_Release(This->binding);
        if(This->memstream)
            IStream_Release(This->memstream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI bsc_OnStartBinding(
        IBindStatusCallback* iface,
        DWORD dwReserved,
        IBinding* pib)
{
    bsc_t *This = impl_from_IBindStatusCallback(iface);
    HRESULT hr;

    TRACE("(%p)->(%x %p)\n", This, dwReserved, pib);

    This->binding = pib;
    IBindStatusCallback_AddRef(pib);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &This->memstream);
    if(FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT WINAPI bsc_GetPriority(
        IBindStatusCallback* iface,
        LONG* pnPriority)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnLowResource(
        IBindStatusCallback* iface,
        DWORD reserved)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnProgress(
        IBindStatusCallback* iface,
        ULONG ulProgress,
        ULONG ulProgressMax,
        ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    return S_OK;
}

static HRESULT WINAPI bsc_OnStopBinding(
        IBindStatusCallback* iface,
        HRESULT hresult,
        LPCWSTR szError)
{
    bsc_t *This = impl_from_IBindStatusCallback(iface);
    HRESULT hr = S_OK;

    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));

    if(This->binding) {
        IBinding_Release(This->binding);
        This->binding = NULL;
    }

    if(This->obj && SUCCEEDED(hresult)) {
        HGLOBAL hglobal;
        hr = GetHGlobalFromStream(This->memstream, &hglobal);
        if(SUCCEEDED(hr))
        {
            DWORD len = GlobalSize(hglobal);
            char *ptr = GlobalLock(hglobal);

            hr = This->onDataAvailable(This->obj, ptr, len);

            GlobalUnlock(hglobal);
        }
    }

    return hr;
}

static HRESULT WINAPI bsc_GetBindInfo(
        IBindStatusCallback* iface,
        DWORD* grfBINDF,
        BINDINFO* pbindinfo)
{
    *grfBINDF = BINDF_GETNEWESTVERSION|BINDF_PULLDATA|BINDF_RESYNCHRONIZE|BINDF_PRAGMA_NO_CACHE;

    return S_OK;
}

static HRESULT WINAPI bsc_OnDataAvailable(
        IBindStatusCallback* iface,
        DWORD grfBSCF,
        DWORD dwSize,
        FORMATETC* pformatetc,
        STGMEDIUM* pstgmed)
{
    bsc_t *This = impl_from_IBindStatusCallback(iface);
    BYTE buf[4096];
    DWORD read, written;
    HRESULT hr;

    TRACE("(%p)->(%x %d %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    do
    {
        hr = IStream_Read(pstgmed->u.pstm, buf, sizeof(buf), &read);
        if(FAILED(hr))
            break;

        hr = IStream_Write(This->memstream, buf, read, &written);
    } while(SUCCEEDED(hr) && written != 0 && read != 0);

    return S_OK;
}

static HRESULT WINAPI bsc_OnObjectAvailable(
        IBindStatusCallback* iface,
        REFIID riid,
        IUnknown* punk)
{
    return S_OK;
}

static const struct IBindStatusCallbackVtbl bsc_vtbl =
{
    bsc_QueryInterface,
    bsc_AddRef,
    bsc_Release,
    bsc_OnStartBinding,
    bsc_GetPriority,
    bsc_OnLowResource,
    bsc_OnProgress,
    bsc_OnStopBinding,
    bsc_GetBindInfo,
    bsc_OnDataAvailable,
    bsc_OnObjectAvailable
};

HRESULT bind_url(LPCWSTR url, HRESULT (*onDataAvailable)(void*,char*,DWORD), void *obj, bsc_t **ret)
{
    WCHAR fileUrl[INTERNET_MAX_URL_LENGTH];
    bsc_t *bsc;
    IBindCtx *pbc;
    HRESULT hr;

    TRACE("%s\n", debugstr_w(url));

    if(!PathIsURLW(url))
    {
        WCHAR fullpath[MAX_PATH];
        DWORD needed = sizeof(fileUrl)/sizeof(WCHAR);

        if(!PathSearchAndQualifyW(url, fullpath, sizeof(fullpath)/sizeof(WCHAR)))
        {
            WARN("can't find path\n");
            return E_FAIL;
        }

        if(FAILED(UrlCreateFromPathW(url, fileUrl, &needed, 0)))
        {
            ERR("can't create url from path\n");
            return E_FAIL;
        }
        url = fileUrl;
    }

    hr = CreateBindCtx(0, &pbc);
    if(FAILED(hr))
        return hr;

    bsc = HeapAlloc(GetProcessHeap(), 0, sizeof(bsc_t));

    bsc->lpVtbl = &bsc_vtbl;
    bsc->ref = 1;
    bsc->obj = obj;
    bsc->onDataAvailable = onDataAvailable;
    bsc->binding = NULL;
    bsc->memstream = NULL;

    hr = RegisterBindStatusCallback(pbc, (IBindStatusCallback*)&bsc->lpVtbl, NULL, 0);
    if(SUCCEEDED(hr))
    {
        IMoniker *moniker;

        hr = CreateURLMoniker(NULL, url, &moniker);
        if(SUCCEEDED(hr))
        {
            IStream *stream;
            hr = IMoniker_BindToStorage(moniker, pbc, NULL, &IID_IStream, (LPVOID*)&stream);
            IMoniker_Release(moniker);
            if(stream)
                IStream_Release(stream);
        }
        IBindCtx_Release(pbc);
    }

    *ret = bsc;
    return hr;
}

void detach_bsc(bsc_t *bsc)
{
    if(bsc->binding)
        IBinding_Abort(bsc->binding);

    bsc->obj = NULL;
    IBindStatusCallback_Release((IBindStatusCallback*)&bsc->lpVtbl);
}
