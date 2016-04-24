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

#include "wbemdisp_private.h"

#include <rpcproxy.h>
#include <wmiutils.h>
#include <wbemdisp_classes.h>

static HINSTANCE instance;

struct moniker
{
    IMoniker IMoniker_iface;
    LONG refs;
    IUnknown *obj;
};

static inline struct moniker *impl_from_IMoniker(
    IMoniker *iface )
{
    return CONTAINING_RECORD( iface, struct moniker, IMoniker_iface );
}

static ULONG WINAPI moniker_AddRef(
    IMoniker *iface )
{
    struct moniker *moniker = impl_from_IMoniker( iface );
    return InterlockedIncrement( &moniker->refs );
}

static ULONG WINAPI moniker_Release(
    IMoniker *iface )
{
    struct moniker *moniker = impl_from_IMoniker( iface );
    LONG refs = InterlockedDecrement( &moniker->refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", moniker );
        IUnknown_Release( moniker->obj );
        heap_free( moniker );
    }
    return refs;
}

static HRESULT WINAPI moniker_QueryInterface(
    IMoniker *iface, REFIID riid, void **ppvObject )
{
    struct moniker *moniker = impl_from_IMoniker( iface );

    TRACE( "%p, %s, %p\n", moniker, debugstr_guid( riid ), ppvObject );

    if (IsEqualGUID( riid, &IID_IMoniker ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }
    IMoniker_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI moniker_GetClassID(
    IMoniker *iface, CLSID *pClassID )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_IsDirty(
    IMoniker *iface )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_Load(
    IMoniker *iface, IStream *pStm )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_Save(
    IMoniker *iface, IStream *pStm, BOOL fClearDirty )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_GetSizeMax(
    IMoniker *iface, ULARGE_INTEGER *pcbSize )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_BindToObject(
    IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riidResult, void **ppvResult )
{
    struct moniker *moniker = impl_from_IMoniker( iface );

    TRACE( "%p, %p, %p, %s, %p\n", iface, pbc, pmkToLeft, debugstr_guid(riidResult), ppvResult );
    return IUnknown_QueryInterface( moniker->obj, riidResult, ppvResult );
}

static HRESULT WINAPI moniker_BindToStorage(
    IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppvObj )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_Reduce(
    IMoniker *iface, IBindCtx *pbc, DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_ComposeWith(
    IMoniker *iface, IMoniker *pmkRight, BOOL fOnlyIfNotGeneric, IMoniker **ppmkComposite )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_Enum(
    IMoniker *iface, BOOL fForward, IEnumMoniker **ppenumMoniker )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_IsEqual(
    IMoniker *iface, IMoniker *pmkOtherMoniker )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_Hash(
    IMoniker *iface, DWORD *pdwHash )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_IsRunning(
    IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_GetTimeOfLastChange(
    IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_Inverse(
    IMoniker *iface, IMoniker **ppmk )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_CommonPrefixWith(
    IMoniker *iface, IMoniker *pmkOther, IMoniker **ppmkPrefix )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_RelativePathTo(
    IMoniker *iface, IMoniker *pmkOther, IMoniker **ppmkRelPath )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_GetDisplayName(
    IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_ParseDisplayName(
    IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR pszDisplayName, ULONG *pchEaten,
    IMoniker **ppmkOut )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static HRESULT WINAPI moniker_IsSystemMoniker(
    IMoniker *iface, DWORD *pdwMksys )
{
    FIXME( "\n" );
    return E_NOTIMPL;
}

static const IMonikerVtbl moniker_vtbl =
{
    moniker_QueryInterface,
    moniker_AddRef,
    moniker_Release,
    moniker_GetClassID,
    moniker_IsDirty,
    moniker_Load,
    moniker_Save,
    moniker_GetSizeMax,
    moniker_BindToObject,
    moniker_BindToStorage,
    moniker_Reduce,
    moniker_ComposeWith,
    moniker_Enum,
    moniker_IsEqual,
    moniker_Hash,
    moniker_IsRunning,
    moniker_GetTimeOfLastChange,
    moniker_Inverse,
    moniker_CommonPrefixWith,
    moniker_RelativePathTo,
    moniker_GetDisplayName,
    moniker_ParseDisplayName,
    moniker_IsSystemMoniker
};

static HRESULT Moniker_create( IUnknown *unk, IMoniker **obj )
{
    struct moniker *moniker;

    TRACE( "%p, %p\n", unk, obj );

    if (!(moniker = heap_alloc( sizeof(*moniker) ))) return E_OUTOFMEMORY;
    moniker->IMoniker_iface.lpVtbl = &moniker_vtbl;
    moniker->refs = 1;
    moniker->obj = unk;
    IUnknown_AddRef( moniker->obj );

    *obj = &moniker->IMoniker_iface;
    TRACE( "returning iface %p\n", *obj );
    return S_OK;
}

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
    static const WCHAR prefixW[] = {'w','i','n','m','g','m','t','s',':',0};
    const DWORD prefix_len = sizeof(prefixW) / sizeof(prefixW[0]) - 1;
    ISWbemLocator *locator = NULL;
    ISWbemServices *services = NULL;
    ISWbemObject *obj = NULL;
    BSTR server, namespace, relative;
    WCHAR *p;
    HRESULT hr;

    TRACE( "%p, %p, %s, %p, %p\n", iface, pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut );

    if (strncmpiW( pszDisplayName, prefixW, prefix_len )) return MK_E_SYNTAX;

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

    if (!relative || !*relative) Moniker_create( (IUnknown *)services, ppmkOut );
    else
    {
        hr = ISWbemServices_Get( services, relative, 0, NULL, &obj );
        if (hr != S_OK) goto done;
        hr = Moniker_create( (IUnknown *)obj, ppmkOut );
    }

done:
    if (obj) ISWbemObject_Release( obj );
    if (services) ISWbemServices_Release( services );
    if (locator) ISWbemLocator_Release( locator );
    SysFreeString( server );
    SysFreeString( namespace );
    SysFreeString( relative );
    if (hr == S_OK) *pchEaten = strlenW( pszDisplayName );
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
    FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
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
static struct factory winmgmts_cf = { { &factory_vtbl }, WinMGMTS_create };

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            instance = hinst;
            DisableThreadLibraryCalls( hinst );
            break;
    }
    return TRUE;
}

HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *obj )
{
    IClassFactory *cf = NULL;

    TRACE( "%s, %s, %p\n", debugstr_guid(rclsid), debugstr_guid(iid), obj );

    if (IsEqualGUID( rclsid, &CLSID_SWbemLocator ))
        cf = &swbem_locator_cf.IClassFactory_iface;
    else if (IsEqualGUID( rclsid, &CLSID_WinMGMTS ))
        cf = &winmgmts_cf.IClassFactory_iface;
    else
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, iid, obj );
}

/***********************************************************************
 *      DllCanUnloadNow (WBEMDISP.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *      DllRegisterServer (WBEMDISP.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *      DllUnregisterServer (WBEMDISP.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
