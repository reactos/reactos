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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "initguid.h"
#include "objbase.h"
#include "wbemcli.h"
#include "wmiutils.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include "wmiutils_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmiutils);

typedef HRESULT (*fnCreateInstance)( LPVOID *ppObj );

typedef struct
{
    IClassFactory IClassFactory_iface;
    fnCreateInstance pfnCreateInstance;
} wmiutils_cf;

static inline wmiutils_cf *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD(iface, wmiutils_cf, IClassFactory_iface);
}

static HRESULT WINAPI wmiutils_cf_QueryInterface( IClassFactory *iface, REFIID riid, LPVOID *ppobj )
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

static ULONG WINAPI wmiutils_cf_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI wmiutils_cf_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI wmiutils_cf_CreateInstance( IClassFactory *iface, LPUNKNOWN pOuter,
                                                  REFIID riid, LPVOID *ppobj )
{
    wmiutils_cf *This = impl_from_IClassFactory( iface );
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

static HRESULT WINAPI wmiutils_cf_LockServer( IClassFactory *iface, BOOL dolock )
{
    FIXME("(%p)->(%d)\n", iface, dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl wmiutils_cf_vtbl =
{
    wmiutils_cf_QueryInterface,
    wmiutils_cf_AddRef,
    wmiutils_cf_Release,
    wmiutils_cf_CreateInstance,
    wmiutils_cf_LockServer
};

static wmiutils_cf status_code_cf = { { &wmiutils_cf_vtbl }, WbemStatusCodeText_create };
static wmiutils_cf path_cf = { { &wmiutils_cf_vtbl }, WbemPath_create };

HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *ppv )
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (IsEqualGUID( rclsid, &CLSID_WbemStatusCode ))
    {
       cf = &status_code_cf.IClassFactory_iface;
    }
    else if (IsEqualGUID( rclsid, &CLSID_WbemDefPath ))
    {
        cf = &path_cf.IClassFactory_iface;
    }
    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;
    return IClassFactory_QueryInterface( cf, iid, ppv );
}
