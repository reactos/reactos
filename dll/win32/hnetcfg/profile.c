/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "netfw.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "hnetcfg_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hnetcfg);

typedef struct fw_profile
{
    const INetFwProfileVtbl *vtbl;
    LONG refs;
} fw_profile;

static inline fw_profile *impl_from_INetFwProfile( INetFwProfile *iface )
{
    return (fw_profile *)((char *)iface - FIELD_OFFSET( fw_profile, vtbl ));
}

static ULONG WINAPI fw_profile_AddRef(
    INetFwProfile *iface )
{
    fw_profile *fw_profile = impl_from_INetFwProfile( iface );
    return InterlockedIncrement( &fw_profile->refs );
}

static ULONG WINAPI fw_profile_Release(
    INetFwProfile *iface )
{
    fw_profile *fw_profile = impl_from_INetFwProfile( iface );
    LONG refs = InterlockedDecrement( &fw_profile->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_profile);
        HeapFree( GetProcessHeap(), 0, fw_profile );
    }
    return refs;
}

static HRESULT WINAPI fw_profile_QueryInterface(
    INetFwProfile *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwProfile ) ||
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
    INetFwProfile_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_profile_GetTypeInfoCount(
    INetFwProfile *iface,
    UINT *pctinfo )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p %p\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_GetTypeInfo(
    INetFwProfile *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_GetIDsOfNames(
    INetFwProfile *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_Invoke(
    INetFwProfile *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_get_Type(
    INetFwProfile *iface,
    NET_FW_PROFILE_TYPE *type )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %p\n", This, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_get_FirewallEnabled(
    INetFwProfile *iface,
    VARIANT_BOOL *enabled )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %p\n", This, enabled);

    *enabled = VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI fw_profile_put_FirewallEnabled(
    INetFwProfile *iface,
    VARIANT_BOOL enabled )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %d\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_get_ExceptionsNotAllowed(
    INetFwProfile *iface,
    VARIANT_BOOL *notAllowed )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %p\n", This, notAllowed);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_put_ExceptionsNotAllowed(
    INetFwProfile *iface,
    VARIANT_BOOL notAllowed )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %d\n", This, notAllowed);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_get_NotificationsDisabled(
    INetFwProfile *iface,
    VARIANT_BOOL *disabled )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %p\n", This, disabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_put_NotificationsDisabled(
    INetFwProfile *iface,
    VARIANT_BOOL disabled )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %d\n", This, disabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_get_UnicastResponsesToMulticastBroadcastDisabled(
    INetFwProfile *iface,
    VARIANT_BOOL *disabled )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %p\n", This, disabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_put_UnicastResponsesToMulticastBroadcastDisabled(
    INetFwProfile *iface,
    VARIANT_BOOL disabled )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %d\n", This, disabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_get_RemoteAdminSettings(
    INetFwProfile *iface,
    INetFwRemoteAdminSettings **remoteAdminSettings )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %p\n", This, remoteAdminSettings);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_get_IcmpSettings(
    INetFwProfile *iface,
    INetFwIcmpSettings **icmpSettings )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    FIXME("%p, %p\n", This, icmpSettings);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_profile_get_GloballyOpenPorts(
    INetFwProfile *iface,
    INetFwOpenPorts **openPorts )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    TRACE("%p, %p\n", This, openPorts);
    return NetFwOpenPorts_create( NULL, (void **)openPorts );
}

static HRESULT WINAPI fw_profile_get_Services(
    INetFwProfile *iface,
    INetFwServices **Services )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    TRACE("%p, %p\n", This, Services);
    return NetFwServices_create( NULL, (void **)Services );
}

static HRESULT WINAPI fw_profile_get_AuthorizedApplications(
    INetFwProfile *iface,
    INetFwAuthorizedApplications **apps )
{
    fw_profile *This = impl_from_INetFwProfile( iface );

    TRACE("%p, %p\n", This, apps);
    return NetFwAuthorizedApplications_create( NULL, (void **)apps );
}

static const struct INetFwProfileVtbl fw_profile_vtbl =
{
    fw_profile_QueryInterface,
    fw_profile_AddRef,
    fw_profile_Release,
    fw_profile_GetTypeInfoCount,
    fw_profile_GetTypeInfo,
    fw_profile_GetIDsOfNames,
    fw_profile_Invoke,
    fw_profile_get_Type,
    fw_profile_get_FirewallEnabled,
    fw_profile_put_FirewallEnabled,
    fw_profile_get_ExceptionsNotAllowed,
    fw_profile_put_ExceptionsNotAllowed,
    fw_profile_get_NotificationsDisabled,
    fw_profile_put_NotificationsDisabled,
    fw_profile_get_UnicastResponsesToMulticastBroadcastDisabled,
    fw_profile_put_UnicastResponsesToMulticastBroadcastDisabled,
    fw_profile_get_RemoteAdminSettings,
    fw_profile_get_IcmpSettings,
    fw_profile_get_GloballyOpenPorts,
    fw_profile_get_Services,
    fw_profile_get_AuthorizedApplications
};

HRESULT NetFwProfile_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_profile *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = HeapAlloc( GetProcessHeap(), 0, sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->vtbl = &fw_profile_vtbl;
    fp->refs = 1;

    *ppObj = &fp->vtbl;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
