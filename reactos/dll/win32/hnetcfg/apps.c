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

#include "hnetcfg_private.h"

#include <winnls.h>
#include <ole2.h>

typedef struct fw_app
{
    INetFwAuthorizedApplication INetFwAuthorizedApplication_iface;
    LONG refs;
    BSTR filename;
} fw_app;

static inline fw_app *impl_from_INetFwAuthorizedApplication( INetFwAuthorizedApplication *iface )
{
    return CONTAINING_RECORD(iface, fw_app, INetFwAuthorizedApplication_iface);
}

static ULONG WINAPI fw_app_AddRef(
    INetFwAuthorizedApplication *iface )
{
    fw_app *fw_app = impl_from_INetFwAuthorizedApplication( iface );
    return InterlockedIncrement( &fw_app->refs );
}

static ULONG WINAPI fw_app_Release(
    INetFwAuthorizedApplication *iface )
{
    fw_app *fw_app = impl_from_INetFwAuthorizedApplication( iface );
    LONG refs = InterlockedDecrement( &fw_app->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_app);
        if (fw_app->filename) SysFreeString( fw_app->filename );
        HeapFree( GetProcessHeap(), 0, fw_app );
    }
    return refs;
}

static HRESULT WINAPI fw_app_QueryInterface(
    INetFwAuthorizedApplication *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwAuthorizedApplication ) ||
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
    INetFwAuthorizedApplication_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_app_GetTypeInfoCount(
    INetFwAuthorizedApplication *iface,
    UINT *pctinfo )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static ITypeLib *typelib;
static ITypeInfo *typeinfo[last_tid];

static REFIID tid_id[] =
{
    &IID_INetFwAuthorizedApplication,
    &IID_INetFwAuthorizedApplications,
    &IID_INetFwMgr,
    &IID_INetFwOpenPort,
    &IID_INetFwOpenPorts,
    &IID_INetFwPolicy,
    &IID_INetFwProfile
};

HRESULT get_typeinfo( enum type_id tid, ITypeInfo **ret )
{
    HRESULT hr;

    if (!typelib)
    {
        ITypeLib *lib;

        hr = LoadRegTypeLib( &LIBID_NetFwPublicTypeLib, 1, 0, LOCALE_SYSTEM_DEFAULT, &lib );
        if (FAILED(hr))
        {
            ERR("LoadRegTypeLib failed: %08x\n", hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)&typelib, lib, NULL ))
            ITypeLib_Release( lib );
    }
    if (!typeinfo[tid])
    {
        ITypeInfo *info;

        hr = ITypeLib_GetTypeInfoOfGuid( typelib, tid_id[tid], &info );
        if (FAILED(hr))
        {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(tid_id[tid]), hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)(typeinfo + tid), info, NULL ))
            ITypeInfo_Release( info );
    }
    *ret = typeinfo[tid];
    ITypeInfo_AddRef(typeinfo[tid]);
    return S_OK;
}

void release_typelib(void)
{
    unsigned i;

    for (i = 0; i < sizeof(typeinfo)/sizeof(*typeinfo); i++)
        if (typeinfo[i])
            ITypeInfo_Release(typeinfo[i]);

    if (typelib)
        ITypeLib_Release(typelib);
}

static HRESULT WINAPI fw_app_GetTypeInfo(
    INetFwAuthorizedApplication *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwAuthorizedApplication_tid, ppTInfo );
}

static HRESULT WINAPI fw_app_GetIDsOfNames(
    INetFwAuthorizedApplication *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwAuthorizedApplication_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_app_Invoke(
    INetFwAuthorizedApplication *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwAuthorizedApplication_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwAuthorizedApplication_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_app_get_Name(
    INetFwAuthorizedApplication *iface,
    BSTR *name )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %p\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_app_put_Name(
    INetFwAuthorizedApplication *iface,
    BSTR name )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %s\n", This, debugstr_w(name));
    return S_OK;
}

static HRESULT WINAPI fw_app_get_ProcessImageFileName(
    INetFwAuthorizedApplication *iface,
    BSTR *imageFileName )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %p\n", This, imageFileName);

    if (!imageFileName)
        return E_INVALIDARG;

    if (!This->filename)
    {
        *imageFileName = NULL;
        return S_OK;
    }

    *imageFileName = SysAllocString( This->filename );
    return *imageFileName ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI fw_app_put_ProcessImageFileName(
    INetFwAuthorizedApplication *iface,
    BSTR imageFileName )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %s\n", This, debugstr_w(imageFileName));

    if (!imageFileName)
    {
        This->filename = NULL;
        return S_OK;
    }

    This->filename = SysAllocString( imageFileName );
    return This->filename ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI fw_app_get_IpVersion(
    INetFwAuthorizedApplication *iface,
    NET_FW_IP_VERSION *ipVersion )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    TRACE("%p, %p\n", This, ipVersion);

    if (!ipVersion)
        return E_POINTER;
    *ipVersion = NET_FW_IP_VERSION_ANY;
    return S_OK;
}

static HRESULT WINAPI fw_app_put_IpVersion(
    INetFwAuthorizedApplication *iface,
    NET_FW_IP_VERSION ipVersion )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    TRACE("%p, %u\n", This, ipVersion);
    return S_OK;
}

static HRESULT WINAPI fw_app_get_Scope(
    INetFwAuthorizedApplication *iface,
    NET_FW_SCOPE *scope )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %p\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_app_put_Scope(
    INetFwAuthorizedApplication *iface,
    NET_FW_SCOPE scope )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %u\n", This, scope);
    return S_OK;
}

static HRESULT WINAPI fw_app_get_RemoteAddresses(
    INetFwAuthorizedApplication *iface,
    BSTR *remoteAddrs )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %p\n", This, remoteAddrs);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_app_put_RemoteAddresses(
    INetFwAuthorizedApplication *iface,
    BSTR remoteAddrs )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %s\n", This, debugstr_w(remoteAddrs));
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_app_get_Enabled(
    INetFwAuthorizedApplication *iface,
    VARIANT_BOOL *enabled )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %p\n", This, enabled);

    *enabled = VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI fw_app_put_Enabled(
    INetFwAuthorizedApplication *iface,
    VARIANT_BOOL enabled )
{
    fw_app *This = impl_from_INetFwAuthorizedApplication( iface );

    FIXME("%p, %d\n", This, enabled);
    return S_OK;
}

static const struct INetFwAuthorizedApplicationVtbl fw_app_vtbl =
{
    fw_app_QueryInterface,
    fw_app_AddRef,
    fw_app_Release,
    fw_app_GetTypeInfoCount,
    fw_app_GetTypeInfo,
    fw_app_GetIDsOfNames,
    fw_app_Invoke,
    fw_app_get_Name,
    fw_app_put_Name,
    fw_app_get_ProcessImageFileName,
    fw_app_put_ProcessImageFileName,
    fw_app_get_IpVersion,
    fw_app_put_IpVersion,
    fw_app_get_Scope,
    fw_app_put_Scope,
    fw_app_get_RemoteAddresses,
    fw_app_put_RemoteAddresses,
    fw_app_get_Enabled,
    fw_app_put_Enabled
};

HRESULT NetFwAuthorizedApplication_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_app *fa;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fa = HeapAlloc( GetProcessHeap(), 0, sizeof(*fa) );
    if (!fa) return E_OUTOFMEMORY;

    fa->INetFwAuthorizedApplication_iface.lpVtbl = &fw_app_vtbl;
    fa->refs = 1;
    fa->filename = NULL;

    *ppObj = &fa->INetFwAuthorizedApplication_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
typedef struct fw_apps
{
    INetFwAuthorizedApplications INetFwAuthorizedApplications_iface;
    LONG refs;
} fw_apps;

static inline fw_apps *impl_from_INetFwAuthorizedApplications( INetFwAuthorizedApplications *iface )
{
    return CONTAINING_RECORD(iface, fw_apps, INetFwAuthorizedApplications_iface);
}

static ULONG WINAPI fw_apps_AddRef(
    INetFwAuthorizedApplications *iface )
{
    fw_apps *fw_apps = impl_from_INetFwAuthorizedApplications( iface );
    return InterlockedIncrement( &fw_apps->refs );
}

static ULONG WINAPI fw_apps_Release(
    INetFwAuthorizedApplications *iface )
{
    fw_apps *fw_apps = impl_from_INetFwAuthorizedApplications( iface );
    LONG refs = InterlockedDecrement( &fw_apps->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_apps);
        HeapFree( GetProcessHeap(), 0, fw_apps );
    }
    return refs;
}

static HRESULT WINAPI fw_apps_QueryInterface(
    INetFwAuthorizedApplications *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwAuthorizedApplications ) ||
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
    INetFwAuthorizedApplications_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_apps_GetTypeInfoCount(
    INetFwAuthorizedApplications *iface,
    UINT *pctinfo )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );

    FIXME("%p %p\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_apps_GetTypeInfo(
    INetFwAuthorizedApplications *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwAuthorizedApplications_tid, ppTInfo );
}

static HRESULT WINAPI fw_apps_GetIDsOfNames(
    INetFwAuthorizedApplications *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwAuthorizedApplications_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_apps_Invoke(
    INetFwAuthorizedApplications *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwAuthorizedApplications_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwAuthorizedApplications_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_apps_get_Count(
    INetFwAuthorizedApplications *iface,
    LONG *count )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );

    FIXME("%p, %p\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_apps_Add(
    INetFwAuthorizedApplications *iface,
    INetFwAuthorizedApplication *app )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );

    FIXME("%p, %p\n", This, app);
    return S_OK;
}

static HRESULT WINAPI fw_apps_Remove(
    INetFwAuthorizedApplications *iface,
    BSTR imageFileName )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );

    FIXME("%p, %s\n", This, debugstr_w(imageFileName));
    return S_OK;
}

static HRESULT WINAPI fw_apps_Item(
    INetFwAuthorizedApplications *iface,
    BSTR imageFileName,
    INetFwAuthorizedApplication **app )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );

    TRACE("%p, %s, %p\n", This, debugstr_w(imageFileName), app);
    return NetFwAuthorizedApplication_create( NULL, (void **)app );
}

static HRESULT WINAPI fw_apps_get__NewEnum(
    INetFwAuthorizedApplications *iface,
    IUnknown **newEnum )
{
    fw_apps *This = impl_from_INetFwAuthorizedApplications( iface );

    FIXME("%p, %p\n", This, newEnum);
    return E_NOTIMPL;
}

static const struct INetFwAuthorizedApplicationsVtbl fw_apps_vtbl =
{
    fw_apps_QueryInterface,
    fw_apps_AddRef,
    fw_apps_Release,
    fw_apps_GetTypeInfoCount,
    fw_apps_GetTypeInfo,
    fw_apps_GetIDsOfNames,
    fw_apps_Invoke,
    fw_apps_get_Count,
    fw_apps_Add,
    fw_apps_Remove,
    fw_apps_Item,
    fw_apps_get__NewEnum
};

HRESULT NetFwAuthorizedApplications_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_apps *fa;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fa = HeapAlloc( GetProcessHeap(), 0, sizeof(*fa) );
    if (!fa) return E_OUTOFMEMORY;

    fa->INetFwAuthorizedApplications_iface.lpVtbl = &fw_apps_vtbl;
    fa->refs = 1;

    *ppObj = &fa->INetFwAuthorizedApplications_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
