/*
 *
 * Copyright 2009 Austin English
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
#include "wbemcli.h"
#include "wbemprov.h"
#include "rpcproxy.h"

#include "wbemprox_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static HINSTANCE instance;

struct list *table_list;

typedef HRESULT (*fnCreateInstance)( LPVOID *ppObj );

typedef struct
{
    IClassFactory IClassFactory_iface;
    fnCreateInstance pfnCreateInstance;
} wbemprox_cf;

static inline wbemprox_cf *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD(iface, wbemprox_cf, IClassFactory_iface);
}

static HRESULT WINAPI wbemprox_cf_QueryInterface( IClassFactory *iface, REFIID riid, LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }
    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI wbemprox_cf_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI wbemprox_cf_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI wbemprox_cf_CreateInstance( IClassFactory *iface, LPUNKNOWN pOuter,
                                                  REFIID riid, LPVOID *ppobj )
{
    wbemprox_cf *This = impl_from_IClassFactory( iface );
    HRESULT r;
    IUnknown *punk;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    r = This->pfnCreateInstance( (LPVOID *)&punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    IUnknown_Release( punk );
    return r;
}

static HRESULT WINAPI wbemprox_cf_LockServer( IClassFactory *iface, BOOL dolock )
{
    FIXME("(%p)->(%d)\n", iface, dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl wbemprox_cf_vtbl =
{
    wbemprox_cf_QueryInterface,
    wbemprox_cf_AddRef,
    wbemprox_cf_Release,
    wbemprox_cf_CreateInstance,
    wbemprox_cf_LockServer
};

static wbemprox_cf wbem_locator_cf = { { &wbemprox_cf_vtbl }, WbemLocator_create };

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            instance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            init_table_list();
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *ppv )
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (IsEqualGUID( rclsid, &CLSID_WbemLocator ) ||
        IsEqualGUID( rclsid, &CLSID_WbemAdministrativeLocator ))
    {
       cf = &wbem_locator_cf.IClassFactory_iface;
    }
    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;
    return IClassFactory_QueryInterface( cf, iid, ppv );
}

/***********************************************************************
 *              DllCanUnloadNow (WBEMPROX.@)
 */
HRESULT WINAPI DllCanUnloadNow( void )
{
    return S_FALSE;
}

/***********************************************************************
 *		DllRegisterServer (WBEMPROX.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (WBEMPROX.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
