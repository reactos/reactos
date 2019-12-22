/*
 * IAutomaticUpdates implementation
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

typedef struct _automatic_updates
{
    IAutomaticUpdates IAutomaticUpdates_iface;
    LONG refs;
} automatic_updates;

static inline automatic_updates *impl_from_IAutomaticUpdates( IAutomaticUpdates *iface )
{
    return CONTAINING_RECORD(iface, automatic_updates, IAutomaticUpdates_iface);
}

static ULONG WINAPI automatic_updates_AddRef(
    IAutomaticUpdates *iface )
{
    automatic_updates *automatic_updates = impl_from_IAutomaticUpdates( iface );
    return InterlockedIncrement( &automatic_updates->refs );
}

static ULONG WINAPI automatic_updates_Release(
    IAutomaticUpdates *iface )
{
    automatic_updates *automatic_updates = impl_from_IAutomaticUpdates( iface );
    LONG refs = InterlockedDecrement( &automatic_updates->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", automatic_updates);
        HeapFree( GetProcessHeap(), 0, automatic_updates );
    }
    return refs;
}

static HRESULT WINAPI automatic_updates_QueryInterface(
    IAutomaticUpdates *iface,
    REFIID riid,
    void **ppvObject )
{
    automatic_updates *This = impl_from_IAutomaticUpdates( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IAutomaticUpdates ) ||
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
    IAutomaticUpdates_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI automatic_updates_GetTypeInfoCount(
    IAutomaticUpdates *iface,
    UINT *pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI automatic_updates_GetTypeInfo(
    IAutomaticUpdates *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI automatic_updates_GetIDsOfNames(
    IAutomaticUpdates *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI automatic_updates_Invoke(
    IAutomaticUpdates *iface,
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

static HRESULT WINAPI automatic_updates_DetectNow(
    IAutomaticUpdates *This )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI automatic_updates_Pause(
    IAutomaticUpdates *This )
{
    FIXME("\n");
    return S_OK;
}

static HRESULT WINAPI automatic_updates_Resume(
    IAutomaticUpdates *This )
{
    FIXME("\n");
    return S_OK;
}

static HRESULT WINAPI automatic_updates_ShowSettingsDialog(
    IAutomaticUpdates *This )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI automatic_updates_EnableService(
    IAutomaticUpdates *This )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI automatic_updates_get_ServiceEnabled(
    IAutomaticUpdates *This,
    VARIANT_BOOL *retval )
{
    FIXME("%p\n", retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI automatic_updates_get_Settings(
    IAutomaticUpdates *This,
    IAutomaticUpdatesSettings **retval )
{
    FIXME("%p\n", retval);
    return E_NOTIMPL;
}

static const struct IAutomaticUpdatesVtbl automatic_updates_vtbl =
{
    automatic_updates_QueryInterface,
    automatic_updates_AddRef,
    automatic_updates_Release,
    automatic_updates_GetTypeInfoCount,
    automatic_updates_GetTypeInfo,
    automatic_updates_GetIDsOfNames,
    automatic_updates_Invoke,
    automatic_updates_DetectNow,
    automatic_updates_Pause,
    automatic_updates_Resume,
    automatic_updates_ShowSettingsDialog,
    automatic_updates_get_Settings,
    automatic_updates_get_ServiceEnabled,
    automatic_updates_EnableService
};

HRESULT AutomaticUpdates_create( LPVOID *ppObj )
{
    automatic_updates *updates;

    TRACE("(%p)\n", ppObj);

    updates = HeapAlloc( GetProcessHeap(), 0, sizeof(*updates) );
    if (!updates) return E_OUTOFMEMORY;

    updates->IAutomaticUpdates_iface.lpVtbl = &automatic_updates_vtbl;
    updates->refs = 1;

    *ppObj = &updates->IAutomaticUpdates_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
