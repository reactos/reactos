/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#include "ws2tcpip.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "httprequest.h"
#include "winhttp.h"

#include "wine/debug.h"
#include "winhttp_private.h"

HINSTANCE winhttp_instance;

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

/******************************************************************
 *              DllMain (winhttp.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        winhttp_instance = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        if (lpv) break;
        netconn_unload();
        release_typelib();
        break;
    }
    return TRUE;
}

typedef HRESULT (*fnCreateInstance)( void **obj );

struct winhttp_cf
{
    IClassFactory IClassFactory_iface;
    fnCreateInstance pfnCreateInstance;
};

static inline struct winhttp_cf *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct winhttp_cf, IClassFactory_iface );
}

static HRESULT WINAPI requestcf_QueryInterface(
    IClassFactory *iface,
    REFIID riid,
    void **obj )
{
    if (IsEqualGUID( riid, &IID_IUnknown ) ||
        IsEqualGUID( riid, &IID_IClassFactory ))
    {
        IClassFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }
    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI requestcf_AddRef(
    IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI requestcf_Release(
    IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI requestcf_CreateInstance(
    IClassFactory *iface,
    LPUNKNOWN outer,
    REFIID riid,
    void **obj )
{
    struct winhttp_cf *cf = impl_from_IClassFactory( iface );
    IUnknown *unknown;
    HRESULT hr;

    TRACE("%p, %s, %p\n", outer, debugstr_guid(riid), obj);

    *obj = NULL;
    if (outer)
        return CLASS_E_NOAGGREGATION;

    hr = cf->pfnCreateInstance( (void **)&unknown );
    if (FAILED(hr))
        return hr;

    hr = IUnknown_QueryInterface( unknown, riid, obj );
    IUnknown_Release( unknown );
    return hr;
}

static HRESULT WINAPI requestcf_LockServer(
    IClassFactory *iface,
    BOOL dolock )
{
    FIXME("%p, %d\n", iface, dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl winhttp_cf_vtbl =
{
    requestcf_QueryInterface,
    requestcf_AddRef,
    requestcf_Release,
    requestcf_CreateInstance,
    requestcf_LockServer
};

static struct winhttp_cf request_cf = { { &winhttp_cf_vtbl }, WinHttpRequest_create };

/******************************************************************
 *		DllGetClassObject (winhttp.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    IClassFactory *cf = NULL;

    TRACE("%s, %s, %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (IsEqualGUID( rclsid, &CLSID_WinHttpRequest ))
    {
       cf = &request_cf.IClassFactory_iface;
    }
    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;
    return IClassFactory_QueryInterface( cf, riid, ppv );
}
