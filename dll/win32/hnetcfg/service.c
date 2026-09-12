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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "netfw.h"

#include "wine/debug.h"
#include "hnetcfg_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hnetcfg);

typedef struct fw_service
{
    INetFwService INetFwService_iface;
    LONG refs;
} fw_service;

static inline fw_service *impl_from_INetFwService( INetFwService *iface )
{
    return CONTAINING_RECORD(iface, fw_service, INetFwService_iface);
}

static ULONG WINAPI fw_service_AddRef(
    INetFwService *iface )
{
    fw_service *fw_service = impl_from_INetFwService( iface );
    return InterlockedIncrement( &fw_service->refs );
}

static ULONG WINAPI fw_service_Release(
    INetFwService *iface )
{
    fw_service *fw_service = impl_from_INetFwService( iface );
    LONG refs = InterlockedDecrement( &fw_service->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_service);
        free( fw_service );
    }
    return refs;
}

static HRESULT WINAPI fw_service_QueryInterface(
    INetFwService *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_service *This = impl_from_INetFwService( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwService ) ||
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
    INetFwService_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_service_GetTypeInfoCount(
    INetFwService *iface,
    UINT *pctinfo )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %p\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_GetTypeInfo(
    INetFwService *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %u %lu %p\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_GetIDsOfNames(
    INetFwService *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %s %p %u %lu %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_Invoke(
    INetFwService *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %ld %s %ld %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_get_Name(
    INetFwService *iface,
    BSTR *name )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %p\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_get_Type(
    INetFwService *iface,
    NET_FW_SERVICE_TYPE *type )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %p\n", This, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_get_Customized(
        INetFwService *iface,
        VARIANT_BOOL *customized )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %p\n", This, customized);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_get_IpVersion(
    INetFwService *iface,
    NET_FW_IP_VERSION *ipVersion )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %p\n", This, ipVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_put_IpVersion(
    INetFwService *iface,
    NET_FW_IP_VERSION ipVersion )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %u\n", This, ipVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_get_Scope(
    INetFwService *iface,
    NET_FW_SCOPE *scope )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %p\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_put_Scope(
    INetFwService *iface,
    NET_FW_SCOPE scope )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %u\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_get_RemoteAddresses(
    INetFwService *iface,
    BSTR *remoteAddrs )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %p\n", This, remoteAddrs);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_put_RemoteAddresses(
    INetFwService *iface,
    BSTR remoteAddrs )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %s\n", This, debugstr_w(remoteAddrs));
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_get_Enabled(
    INetFwService *iface,
    VARIANT_BOOL *enabled )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %p\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_put_Enabled(
    INetFwService *iface,
    VARIANT_BOOL enabled )
{
    fw_service *This = impl_from_INetFwService( iface );

    FIXME("%p %d\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_service_get_GloballyOpenPorts(
    INetFwService *iface,
    INetFwOpenPorts **openPorts )
{
    fw_service *This = impl_from_INetFwService( iface );

    TRACE("%p %p\n", This, openPorts);
    return NetFwOpenPorts_create( NULL, (void **)openPorts );
}

static const struct INetFwServiceVtbl fw_service_vtbl =
{
    fw_service_QueryInterface,
    fw_service_AddRef,
    fw_service_Release,
    fw_service_GetTypeInfoCount,
    fw_service_GetTypeInfo,
    fw_service_GetIDsOfNames,
    fw_service_Invoke,
    fw_service_get_Name,
    fw_service_get_Type,
    fw_service_get_Customized,
    fw_service_get_IpVersion,
    fw_service_put_IpVersion,
    fw_service_get_Scope,
    fw_service_put_Scope,
    fw_service_get_RemoteAddresses,
    fw_service_put_RemoteAddresses,
    fw_service_get_Enabled,
    fw_service_put_Enabled,
    fw_service_get_GloballyOpenPorts
};

static HRESULT NetFwService_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_service *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = malloc( sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwService_iface.lpVtbl = &fw_service_vtbl;
    fp->refs = 1;

    *ppObj = &fp->INetFwService_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

typedef struct fw_services
{
    INetFwServices INetFwServices_iface;
    LONG refs;
} fw_services;

static inline fw_services *impl_from_INetFwServices( INetFwServices *iface )
{
    return CONTAINING_RECORD(iface, fw_services, INetFwServices_iface);
}

static ULONG WINAPI fw_services_AddRef(
    INetFwServices *iface )
{
    fw_services *fw_services = impl_from_INetFwServices( iface );
    return InterlockedIncrement( &fw_services->refs );
}

static ULONG WINAPI fw_services_Release(
    INetFwServices *iface )
{
    fw_services *fw_services = impl_from_INetFwServices( iface );
    LONG refs = InterlockedDecrement( &fw_services->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_services);
        free( fw_services );
    }
    return refs;
}

static HRESULT WINAPI fw_services_QueryInterface(
    INetFwServices *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_services *This = impl_from_INetFwServices( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwServices ) ||
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
    INetFwServices_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_services_GetTypeInfoCount(
    INetFwServices *iface,
    UINT *pctinfo )
{
    fw_services *This = impl_from_INetFwServices( iface );

    FIXME("%p %p\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_services_GetTypeInfo(
    INetFwServices *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_services *This = impl_from_INetFwServices( iface );

    FIXME("%p %u %lu %p\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_services_GetIDsOfNames(
    INetFwServices *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_services *This = impl_from_INetFwServices( iface );

    FIXME("%p %s %p %u %lu %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_services_Invoke(
    INetFwServices *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_services *This = impl_from_INetFwServices( iface );

    FIXME("%p %ld %s %ld %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_services_get_Count(
    INetFwServices *iface,
    LONG *count )
{
    fw_services *This = impl_from_INetFwServices( iface );

    FIXME("%p, %p\n", This, count);

    *count = 0;
    return S_OK;
}

static HRESULT WINAPI fw_services_Item(
    INetFwServices *iface,
    NET_FW_SERVICE_TYPE svcType,
    INetFwService **service )
{
    fw_services *This = impl_from_INetFwServices( iface );

    FIXME("%p, %u, %p\n", This, svcType, service);
    return NetFwService_create( NULL, (void **)service );
}

static HRESULT WINAPI fw_services_get__NewEnum(
    INetFwServices *iface,
    IUnknown **newEnum )
{
    fw_services *This = impl_from_INetFwServices( iface );

    FIXME("%p, %p\n", This, newEnum);
    return E_NOTIMPL;
}

static const struct INetFwServicesVtbl fw_services_vtbl =
{
    fw_services_QueryInterface,
    fw_services_AddRef,
    fw_services_Release,
    fw_services_GetTypeInfoCount,
    fw_services_GetTypeInfo,
    fw_services_GetIDsOfNames,
    fw_services_Invoke,
    fw_services_get_Count,
    fw_services_Item,
    fw_services_get__NewEnum
};

HRESULT NetFwServices_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_services *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = malloc( sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwServices_iface.lpVtbl = &fw_services_vtbl;
    fp->refs = 1;

    *ppObj = &fp->INetFwServices_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
