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
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"
#include "wininet.h"
#include "urlmon.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

struct bsc_t {
    IBindStatusCallback IBindStatusCallback_iface;

    LONG ref;

    void *obj;
    HRESULT (*onDataAvailable)(void*,char*,DWORD);

    IBinding *binding;
    IStream *memstream;
    HRESULT hres;
};

static inline bsc_t *impl_from_IBindStatusCallback( IBindStatusCallback *iface )
{
    return CONTAINING_RECORD(iface, bsc_t, IBindStatusCallback_iface);
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

    TRACE("interface %s not implemented\n", debugstr_guid(riid));
    *ppobj = NULL;
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
        if (This->binding)   IBinding_Release(This->binding);
        if (This->memstream) IStream_Release(This->memstream);
        heap_free(This);
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
    IBinding_AddRef(pib);

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

            This->hres = This->onDataAvailable(This->obj, ptr, len);

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

HRESULT create_uri(const WCHAR *url, IUri **uri)
{
    WCHAR fileUrl[INTERNET_MAX_URL_LENGTH];

    TRACE("%s\n", debugstr_w(url));

    if (!PathIsURLW(url))
    {
        WCHAR fullpath[MAX_PATH];
        DWORD needed = ARRAY_SIZE(fileUrl);

        if (!PathSearchAndQualifyW(url, fullpath, ARRAY_SIZE(fullpath)))
        {
            WARN("can't find path\n");
            return E_FAIL;
        }

        if (FAILED(UrlCreateFromPathW(fullpath, fileUrl, &needed, 0)))
        {
            ERR("can't create url from path\n");
            return E_FAIL;
        }
        url = fileUrl;
    }

    return CreateUri(url, Uri_CREATE_ALLOW_RELATIVE | Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, uri);
}

HRESULT create_moniker_from_url(LPCWSTR url, IMoniker **mon)
{
    HRESULT hr;
    IUri *uri;

    TRACE("%s\n", debugstr_w(url));

    if (FAILED(hr = create_uri(url, &uri)))
        return hr;

    hr = CreateURLMonikerEx2(NULL, uri, mon, 0);
    IUri_Release(uri);
    return hr;
}

HRESULT bind_url(IMoniker *mon, HRESULT (*onDataAvailable)(void*,char*,DWORD),
        void *obj, bsc_t **ret)
{
    bsc_t *bsc;
    IBindCtx *pbc;
    HRESULT hr;

    TRACE("%p\n", mon);

    hr = CreateBindCtx(0, &pbc);
    if(FAILED(hr))
        return hr;

    bsc = heap_alloc(sizeof(bsc_t));

    bsc->IBindStatusCallback_iface.lpVtbl = &bsc_vtbl;
    bsc->ref = 1;
    bsc->obj = obj;
    bsc->onDataAvailable = onDataAvailable;
    bsc->binding = NULL;
    bsc->memstream = NULL;
    bsc->hres = S_OK;

    hr = RegisterBindStatusCallback(pbc, &bsc->IBindStatusCallback_iface, NULL, 0);
    if(SUCCEEDED(hr))
    {
        IStream *stream;
        hr = IMoniker_BindToStorage(mon, pbc, NULL, &IID_IStream, (LPVOID*)&stream);
        if(stream)
            IStream_Release(stream);
        IBindCtx_Release(pbc);
    }

    if(FAILED(hr))
    {
        IBindStatusCallback_Release(&bsc->IBindStatusCallback_iface);
        bsc = NULL;
    }

    *ret = bsc;
    return hr;
}

HRESULT detach_bsc(bsc_t *bsc)
{
    HRESULT hres;

    if(bsc->binding)
        IBinding_Abort(bsc->binding);

    bsc->obj = NULL;
    hres = bsc->hres;
    IBindStatusCallback_Release(&bsc->IBindStatusCallback_iface);

    return hres;
}
