/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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
#include "objbase.h"
#include "wmiutils.h"
#include "wbemdisp.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include "wbemdisp_private.h"
#include "wbemdisp_classes.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemdisp);

static HRESULT WINAPI WinMGMTS_QueryInterface(IParseDisplayName *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IParseDisplayName)) {
        TRACE("(IID_IParseDisplayName %p)\n", ppv);
        *ppv = iface;
    }else {
        WARN("Unsupported riid %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WinMGMTS_AddRef(IParseDisplayName *iface)
{
    return 2;
}

static ULONG WINAPI WinMGMTS_Release(IParseDisplayName *iface)
{
    return 1;
}

static HRESULT parse_path( const WCHAR *str, BSTR *server, BSTR *namespace, BSTR *relative )
{
    IWbemPath *path;
    ULONG len;
    HRESULT hr;

    *server = *namespace = *relative = NULL;

    hr = CoCreateInstance( &CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemPath, (void **)&path );
    if (hr != S_OK) return hr;

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, str );
    if (hr != S_OK) goto done;

    len = 0;
    hr = IWbemPath_GetServer( path, &len, NULL );
    if (hr == S_OK)
    {
        if (!(*server = SysAllocStringLen( NULL, len )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        hr = IWbemPath_GetServer( path, &len, *server );
        if (hr != S_OK) goto done;
    }

    len = 0;
    hr = IWbemPath_GetText( path, WBEMPATH_GET_NAMESPACE_ONLY, &len, NULL );
    if (hr == S_OK)
    {
        if (!(*namespace = SysAllocStringLen( NULL, len )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        hr = IWbemPath_GetText( path, WBEMPATH_GET_NAMESPACE_ONLY, &len, *namespace );
        if (hr != S_OK) goto done;
    }
    len = 0;
    hr = IWbemPath_GetText( path, WBEMPATH_GET_RELATIVE_ONLY, &len, NULL );
    if (hr == S_OK)
    {
        if (!(*relative = SysAllocStringLen( NULL, len )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        hr = IWbemPath_GetText( path, WBEMPATH_GET_RELATIVE_ONLY, &len, *relative );
    }

done:
    IWbemPath_Release( path );
    if (hr != S_OK)
    {
        SysFreeString( *server );
        SysFreeString( *namespace );
        SysFreeString( *relative );
    }
    return hr;
}

static HRESULT WINAPI WinMGMTS_ParseDisplayName(IParseDisplayName *iface, IBindCtx *pbc, LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut)
{
    const DWORD prefix_len = ARRAY_SIZE(L"winmgmts:") - 1;
    ISWbemLocator *locator = NULL;
    ISWbemServices *services = NULL;
    ISWbemObject *obj = NULL;
    BSTR server, namespace, relative;
    WCHAR *p;
    HRESULT hr;

    TRACE( "%p, %p, %s, %p, %p\n", iface, pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut );

    if (wcsnicmp( pszDisplayName, L"winmgmts:", prefix_len )) return MK_E_SYNTAX;

    p = pszDisplayName + prefix_len;
    if (*p == '{')
    {
        FIXME( "ignoring security settings\n" );
        while (*p && *p != '}') p++;
        if (*p == '}') p++;
        if (*p == '!') p++;
    }
    hr = parse_path( p, &server, &namespace, &relative );
    if (hr != S_OK) return hr;

    hr = SWbemLocator_create( (void **)&locator );
    if (hr != S_OK) goto done;

    hr = ISWbemLocator_ConnectServer( locator, server, namespace, NULL, NULL, NULL, NULL, 0, NULL, &services );
    if (hr != S_OK) goto done;

    if (!relative || !*relative) CreatePointerMoniker( (IUnknown *)services, ppmkOut );
    else
    {
        hr = ISWbemServices_Get( services, relative, 0, NULL, &obj );
        if (hr != S_OK) goto done;
        hr = CreatePointerMoniker( (IUnknown *)obj, ppmkOut );
    }

done:
    if (obj) ISWbemObject_Release( obj );
    if (services) ISWbemServices_Release( services );
    if (locator) ISWbemLocator_Release( locator );
    SysFreeString( server );
    SysFreeString( namespace );
    SysFreeString( relative );
    if (hr == S_OK) *pchEaten = lstrlenW( pszDisplayName );
    return hr;
}

static const IParseDisplayNameVtbl WinMGMTSVtbl = {
    WinMGMTS_QueryInterface,
    WinMGMTS_AddRef,
    WinMGMTS_Release,
    WinMGMTS_ParseDisplayName
};

static IParseDisplayName winmgmts = { &WinMGMTSVtbl };

static HRESULT WinMGMTS_create(void **ppv)
{
    *ppv = &winmgmts;
    return S_OK;
}

struct factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*fnCreateInstance)( LPVOID * );
};

static inline struct factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct factory, IClassFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IClassFactory *iface, REFIID riid, LPVOID *obj )
{
    if (IsEqualGUID( riid, &IID_IUnknown ) || IsEqualGUID( riid, &IID_IClassFactory ))
    {
        IClassFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }
    WARN( "interface %s not implemented\n", debugstr_guid(riid) );
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI factory_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI factory_CreateInstance( IClassFactory *iface, LPUNKNOWN outer, REFIID riid,
                                              LPVOID *obj )
{
    struct factory *factory = impl_from_IClassFactory( iface );
    IUnknown *unk;
    HRESULT hr;

    TRACE( "%p, %s, %p\n", outer, debugstr_guid(riid), obj );

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    hr = factory->fnCreateInstance( (LPVOID *)&unk );
    if (FAILED( hr ))
        return hr;

    hr = IUnknown_QueryInterface( unk, riid, obj );
    IUnknown_Release( unk );
    return hr;
}

static HRESULT WINAPI factory_LockServer( IClassFactory *iface, BOOL lock )
{
    FIXME( "%p, %d\n", iface, lock );
    return S_OK;
}

static const struct IClassFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    factory_CreateInstance,
    factory_LockServer
};

static struct factory swbem_locator_cf = { { &factory_vtbl }, SWbemLocator_create };
static struct factory swbem_namedvalueset_cf = { { &factory_vtbl }, SWbemNamedValueSet_create };
static struct factory winmgmts_cf = { { &factory_vtbl }, WinMGMTS_create };

HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *obj )
{
    IClassFactory *cf = NULL;

    TRACE( "%s, %s, %p\n", debugstr_guid(rclsid), debugstr_guid(iid), obj );

    if (IsEqualGUID( rclsid, &CLSID_SWbemLocator ))
        cf = &swbem_locator_cf.IClassFactory_iface;
    else if (IsEqualGUID( rclsid, &CLSID_WinMGMTS ))
        cf = &winmgmts_cf.IClassFactory_iface;
    else if (IsEqualGUID( rclsid, &CLSID_SWbemNamedValueSet ))
        cf = &swbem_namedvalueset_cf.IClassFactory_iface;
    else
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, iid, obj );
}
