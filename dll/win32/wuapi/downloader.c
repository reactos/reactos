/*
 * IUpdateDownloader implementation
 *
 * Copyright 2008 Hans Leidekker
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "initguid.h"
#include "wuapi.h"
#include "wuapi_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wuapi);

typedef struct _update_downloader
{
    IUpdateDownloader IUpdateDownloader_iface;
    LONG refs;
} update_downloader;

static inline update_downloader *impl_from_IUpdateDownloader( IUpdateDownloader *iface )
{
    return CONTAINING_RECORD(iface, update_downloader, IUpdateDownloader_iface);
}

static ULONG WINAPI update_downloader_AddRef(
    IUpdateDownloader *iface )
{
    update_downloader *update_downloader = impl_from_IUpdateDownloader( iface );
    return InterlockedIncrement( &update_downloader->refs );
}

static ULONG WINAPI update_downloader_Release(
    IUpdateDownloader *iface )
{
    update_downloader *update_downloader = impl_from_IUpdateDownloader( iface );
    LONG refs = InterlockedDecrement( &update_downloader->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", update_downloader);
        HeapFree( GetProcessHeap(), 0, update_downloader );
    }
    return refs;
}

static HRESULT WINAPI update_downloader_QueryInterface(
    IUpdateDownloader *iface,
    REFIID riid,
    void **ppvObject )
{
    update_downloader *This = impl_from_IUpdateDownloader( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IUpdateDownloader ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IUpdateDownloader_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI update_downloader_GetTypeInfoCount(
    IUpdateDownloader *iface,
    UINT *pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_GetTypeInfo(
    IUpdateDownloader *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_GetIDsOfNames(
    IUpdateDownloader *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_Invoke(
    IUpdateDownloader *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_get_IsForced(
    IUpdateDownloader *This,
    VARIANT_BOOL *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_put_IsForced(
    IUpdateDownloader *This,
    VARIANT_BOOL value )
{
    FIXME("%p, %d\n", This, value);
    return S_OK;
}

static HRESULT WINAPI update_downloader_get_ClientApplicationID(
    IUpdateDownloader *This,
    BSTR *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_put_ClientApplicationID(
    IUpdateDownloader *This,
    BSTR value )
{
    FIXME("%p, %s\n", This, debugstr_w(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_get_Priority(
    IUpdateDownloader *This,
    DownloadPriority *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_put_Priority(
    IUpdateDownloader *This,
    DownloadPriority value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_get_Updates(
    IUpdateDownloader *This,
    IUpdateCollection **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_put_Updates(
    IUpdateDownloader *This,
    IUpdateCollection *value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_BeginDownload(
    IUpdateDownloader *This,
    IUnknown *onProgressChanged,
    IUnknown *onCompleted,
    VARIANT state,
    IDownloadJob **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_Download(
    IUpdateDownloader *This,
    IDownloadResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_downloader_EndDownload(
    IUpdateDownloader *This,
    IDownloadJob *value,
    IDownloadResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IUpdateDownloaderVtbl update_downloader_vtbl =
{
    update_downloader_QueryInterface,
    update_downloader_AddRef,
    update_downloader_Release,
    update_downloader_GetTypeInfoCount,
    update_downloader_GetTypeInfo,
    update_downloader_GetIDsOfNames,
    update_downloader_Invoke,
    update_downloader_get_ClientApplicationID,
    update_downloader_put_ClientApplicationID,
    update_downloader_get_IsForced,
    update_downloader_put_IsForced,
    update_downloader_get_Priority,
    update_downloader_put_Priority,
    update_downloader_get_Updates,
    update_downloader_put_Updates,
    update_downloader_BeginDownload,
    update_downloader_Download,
    update_downloader_EndDownload
};

HRESULT UpdateDownloader_create( LPVOID *ppObj )
{
    update_downloader *downloader;

    TRACE("(%p)\n", ppObj);

    downloader = HeapAlloc( GetProcessHeap(), 0, sizeof(*downloader) );
    if (!downloader) return E_OUTOFMEMORY;

    downloader->IUpdateDownloader_iface.lpVtbl = &update_downloader_vtbl;
    downloader->refs = 1;

    *ppObj = &downloader->IUpdateDownloader_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
