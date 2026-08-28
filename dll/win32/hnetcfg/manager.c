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

typedef struct fw_manager
{
    INetFwMgr INetFwMgr_iface;
    LONG refs;
} fw_manager;

static inline fw_manager *impl_from_INetFwMgr( INetFwMgr *iface )
{
    return CONTAINING_RECORD(iface, fw_manager, INetFwMgr_iface);
}

static ULONG WINAPI fw_manager_AddRef(
    INetFwMgr *iface )
{
    fw_manager *fw_manager = impl_from_INetFwMgr( iface );
    return InterlockedIncrement( &fw_manager->refs );
}

static ULONG WINAPI fw_manager_Release(
    INetFwMgr *iface )
{
    fw_manager *fw_manager = impl_from_INetFwMgr( iface );
    LONG refs = InterlockedDecrement( &fw_manager->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_manager);
        free( fw_manager );
    }
    return refs;
}

static HRESULT WINAPI fw_manager_QueryInterface(
    INetFwMgr *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_manager *This = impl_from_INetFwMgr( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwMgr ) ||
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
    INetFwMgr_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_manager_GetTypeInfoCount(
    INetFwMgr *iface,
    UINT *pctinfo )
{
    fw_manager *This = impl_from_INetFwMgr( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fw_manager_GetTypeInfo(
    INetFwMgr *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_manager *This = impl_from_INetFwMgr( iface );

    TRACE("%p %u %lu %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwMgr_tid, ppTInfo );
}

static HRESULT WINAPI fw_manager_GetIDsOfNames(
    INetFwMgr *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_manager *This = impl_from_INetFwMgr( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %lu %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwMgr_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_manager_Invoke(
    INetFwMgr *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_manager *This = impl_from_INetFwMgr( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %ld %s %ld %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwMgr_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwMgr_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_manager_get_LocalPolicy(
    INetFwMgr *iface,
    INetFwPolicy **localPolicy )
{
    fw_manager *This = impl_from_INetFwMgr( iface );

    TRACE("%p, %p\n", This, localPolicy);
    return NetFwPolicy_create( NULL, (void **)localPolicy );
}

static HRESULT WINAPI fw_manager_get_CurrentProfileType(
    INetFwMgr *iface,
    NET_FW_PROFILE_TYPE *profileType )
{
    fw_manager *This = impl_from_INetFwMgr( iface );

    FIXME("%p, %p\n", This, profileType);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_manager_RestoreDefaults(
    INetFwMgr *iface )
{
    fw_manager *This = impl_from_INetFwMgr( iface );

    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_manager_IsPortAllowed(
    INetFwMgr *iface,
    BSTR imageFileName,
    NET_FW_IP_VERSION ipVersion,
    LONG portNumber,
    BSTR localAddress,
    NET_FW_IP_PROTOCOL ipProtocol,
    VARIANT *allowed,
    VARIANT *restricted )
{
    fw_manager *This = impl_from_INetFwMgr( iface );

    FIXME("%p, %s, %u, %ld, %s, %u, %p, %p\n", This, debugstr_w(imageFileName),
          ipVersion, portNumber, debugstr_w(localAddress), ipProtocol, allowed, restricted);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_manager_IsIcmpTypeAllowed(
    INetFwMgr *iface,
    NET_FW_IP_VERSION ipVersion,
    BSTR localAddress,
    BYTE type,
    VARIANT *allowed,
    VARIANT *restricted )
{
    fw_manager *This = impl_from_INetFwMgr( iface );

    FIXME("%p, %u, %s, %u, %p, %p\n", This, ipVersion, debugstr_w(localAddress),
          type, allowed, restricted);
    return E_NOTIMPL;
}

static const struct INetFwMgrVtbl fw_manager_vtbl =
{
    fw_manager_QueryInterface,
    fw_manager_AddRef,
    fw_manager_Release,
    fw_manager_GetTypeInfoCount,
    fw_manager_GetTypeInfo,
    fw_manager_GetIDsOfNames,
    fw_manager_Invoke,
    fw_manager_get_LocalPolicy,
    fw_manager_get_CurrentProfileType,
    fw_manager_RestoreDefaults,
    fw_manager_IsPortAllowed,
    fw_manager_IsIcmpTypeAllowed
};

HRESULT NetFwMgr_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_manager *fm;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fm = malloc( sizeof(*fm) );
    if (!fm) return E_OUTOFMEMORY;

    fm->INetFwMgr_iface.lpVtbl = &fw_manager_vtbl;
    fm->refs = 1;

    *ppObj = &fm->INetFwMgr_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
