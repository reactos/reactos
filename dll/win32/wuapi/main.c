/*
 * WUAPI implementation
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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "wuapi.h"

#include "wine/debug.h"
#include "wuapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wuapi);

typedef HRESULT (*fnCreateInstance)( IUnknown *pUnkOuter, LPVOID *ppObj );

typedef struct _wucf
{
    const struct IClassFactoryVtbl *vtbl;
    fnCreateInstance pfnCreateInstance;
} wucf;

static inline wucf *impl_from_IClassFactory( IClassFactory *iface )
{
    return (wucf *)((char *)iface - FIELD_OFFSET( wucf, vtbl ));
}

static HRESULT WINAPI wucf_QueryInterface( IClassFactory *iface, REFIID riid, LPVOID *ppobj )
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

static ULONG WINAPI wucf_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI wucf_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI wucf_CreateInstance( IClassFactory *iface, LPUNKNOWN pOuter,
                                           REFIID riid, LPVOID *ppobj )
{
    wucf *This = impl_from_IClassFactory( iface );
    HRESULT r;
    IUnknown *punk;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    r = This->pfnCreateInstance( pOuter, (LPVOID *)&punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    if (FAILED(r))
        return r;

    IUnknown_Release( punk );
    return r;
}

static HRESULT WINAPI wucf_LockServer( IClassFactory *iface, BOOL dolock )
{
    FIXME("(%p)->(%d)\n", iface, dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl wucf_vtbl =
{
    wucf_QueryInterface,
    wucf_AddRef,
    wucf_Release,
    wucf_CreateInstance,
    wucf_LockServer
};

static wucf sessioncf = { &wucf_vtbl, UpdateSession_create };
static wucf updatescf = { &wucf_vtbl, AutomaticUpdates_create };

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID lpv )
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *ppv )
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (IsEqualGUID( rclsid, &CLSID_UpdateSession ))
    {
       cf = (IClassFactory *)&sessioncf.vtbl;
    }
    else if (IsEqualGUID( rclsid, &CLSID_AutomaticUpdates ))
    {
       cf = (IClassFactory *)&updatescf.vtbl;
    }
    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;
    return IClassFactory_QueryInterface( cf, iid, ppv );
}

HRESULT WINAPI DllCanUnloadNow( void )
{
    FIXME("\n");
    return S_FALSE;
}
