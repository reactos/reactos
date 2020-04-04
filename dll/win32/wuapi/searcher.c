/*
 * IUpdateSearcher implementation
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
#include "wuapi.h"
#include "wuapi_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wuapi);

typedef struct _update_searcher
{
    IUpdateSearcher IUpdateSearcher_iface;
    LONG refs;
} update_searcher;

static inline update_searcher *impl_from_IUpdateSearcher( IUpdateSearcher *iface )
{
    return CONTAINING_RECORD(iface, update_searcher, IUpdateSearcher_iface);
}

static ULONG WINAPI update_searcher_AddRef(
    IUpdateSearcher *iface )
{
    update_searcher *update_searcher = impl_from_IUpdateSearcher( iface );
    return InterlockedIncrement( &update_searcher->refs );
}

static ULONG WINAPI update_searcher_Release(
    IUpdateSearcher *iface )
{
    update_searcher *update_searcher = impl_from_IUpdateSearcher( iface );
    LONG refs = InterlockedDecrement( &update_searcher->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", update_searcher);
        HeapFree( GetProcessHeap(), 0, update_searcher );
    }
    return refs;
}

static HRESULT WINAPI update_searcher_QueryInterface(
    IUpdateSearcher *iface,
    REFIID riid,
    void **ppvObject )
{
    update_searcher *This = impl_from_IUpdateSearcher( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IUpdateSearcher ) ||
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
    IUpdateSearcher_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI update_searcher_GetTypeInfoCount(
    IUpdateSearcher *iface,
    UINT *pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_GetTypeInfo(
    IUpdateSearcher *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_GetIDsOfNames(
    IUpdateSearcher *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_Invoke(
    IUpdateSearcher *iface,
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

static HRESULT WINAPI update_searcher_get_CanAutomaticallyUpgradeService(
    IUpdateSearcher *This,
    VARIANT_BOOL *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_put_CanAutomaticallyUpgradeService(
    IUpdateSearcher *This,
    VARIANT_BOOL value )
{
    FIXME("%p, %d\n", This, value);
    return S_OK;
}

static HRESULT WINAPI update_searcher_get_ClientApplicationID(
    IUpdateSearcher *This,
    BSTR *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_put_ClientApplicationID(
    IUpdateSearcher *This,
    BSTR value )
{
    FIXME("%p, %s\n", This, debugstr_w(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_get_IncludePotentiallySupersededUpdates(
    IUpdateSearcher *This,
    VARIANT_BOOL *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_put_IncludePotentiallySupersededUpdates(
    IUpdateSearcher *This,
    VARIANT_BOOL value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_get_ServerSelection(
    IUpdateSearcher *This,
    ServerSelection *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_put_ServerSelection(
    IUpdateSearcher *This,
    ServerSelection value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_BeginSearch(
    IUpdateSearcher *This,
    BSTR criteria,
    IUnknown *onCompleted,
    VARIANT state,
    ISearchJob **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_EndSearch(
    IUpdateSearcher *This,
    ISearchJob *searchJob,
    ISearchResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_EscapeString(
    IUpdateSearcher *This,
    BSTR unescaped,
    BSTR *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_QueryHistory(
    IUpdateSearcher *This,
    LONG startIndex,
    LONG count,
    IUpdateHistoryEntryCollection **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_Search(
    IUpdateSearcher *This,
    BSTR criteria,
    ISearchResult **retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_get_Online(
    IUpdateSearcher *This,
    VARIANT_BOOL *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_put_Online(
    IUpdateSearcher *This,
    VARIANT_BOOL value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_GetTotalHistoryCount(
    IUpdateSearcher *This,
    LONG *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_get_ServiceID(
    IUpdateSearcher *This,
    BSTR *retval )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI update_searcher_put_ServiceID(
    IUpdateSearcher *This,
    BSTR value )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IUpdateSearcherVtbl update_searcher_vtbl =
{
    update_searcher_QueryInterface,
    update_searcher_AddRef,
    update_searcher_Release,
    update_searcher_GetTypeInfoCount,
    update_searcher_GetTypeInfo,
    update_searcher_GetIDsOfNames,
    update_searcher_Invoke,
    update_searcher_get_CanAutomaticallyUpgradeService,
    update_searcher_put_CanAutomaticallyUpgradeService,
    update_searcher_get_ClientApplicationID,
    update_searcher_put_ClientApplicationID,
    update_searcher_get_IncludePotentiallySupersededUpdates,
    update_searcher_put_IncludePotentiallySupersededUpdates,
    update_searcher_get_ServerSelection,
    update_searcher_put_ServerSelection,
    update_searcher_BeginSearch,
    update_searcher_EndSearch,
    update_searcher_EscapeString,
    update_searcher_QueryHistory,
    update_searcher_Search,
    update_searcher_get_Online,
    update_searcher_put_Online,
    update_searcher_GetTotalHistoryCount,
    update_searcher_get_ServiceID,
    update_searcher_put_ServiceID
};

HRESULT UpdateSearcher_create( LPVOID *ppObj )
{
    update_searcher *searcher;

    TRACE("(%p)\n", ppObj);

    searcher = HeapAlloc( GetProcessHeap(), 0, sizeof(*searcher) );
    if (!searcher) return E_OUTOFMEMORY;

    searcher->IUpdateSearcher_iface.lpVtbl = &update_searcher_vtbl;
    searcher->refs = 1;

    *ppObj = &searcher->IUpdateSearcher_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
