/*
 *    Gameux library IClassFactory implementation
 *
 * Copyright (C) 2010 Mariusz Pluciński
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


#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "shobjidl.h"
#include "initguid.h"
#include "gameux.h"
#include "gameux_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gameux);

typedef HRESULT (*fnCreateInstance)(IUnknown *pUnkOuter, IUnknown **ppObj);

/***************************************************************
 * gameux ClassFactory
 */
typedef struct _gameuxcf
{
    IClassFactory IClassFactory_iface;
    fnCreateInstance pfnCreateInstance;
} gameuxcf;

static inline gameuxcf *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, gameuxcf, IClassFactory_iface);
}

static HRESULT WINAPI gameuxcf_QueryInterface(
        IClassFactory *iface,
        REFIID riid,
        LPVOID *ppObj)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppObj);

    *ppObj = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *ppObj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI gameuxcf_AddRef(
        IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI gameuxcf_Release(
        IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI gameuxcf_CreateInstance(
        IClassFactory *iface,
        LPUNKNOWN pUnkOuter,
        REFIID riid,
        LPVOID *ppObj)
{
    gameuxcf *This = impl_from_IClassFactory(iface);
    HRESULT hr;
    IUnknown *pUnk;

    TRACE("(%p, %p, %s, %p)\n", iface, pUnkOuter, debugstr_guid(riid), ppObj);

    *ppObj = NULL;

    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    hr = This->pfnCreateInstance(pUnkOuter, &pUnk);
    if(FAILED(hr))
        return hr;

    hr = IUnknown_QueryInterface(pUnk, riid, ppObj);
    IUnknown_Release(pUnk);
    return hr;
}

static HRESULT WINAPI gameuxcf_LockServer(
        IClassFactory *iface,
        BOOL dolock)
{
    gameuxcf *This = impl_from_IClassFactory(iface);
    TRACE("(%p, %d)\n", This, dolock);
    FIXME("stub\n");
    return S_OK;
}

static const struct IClassFactoryVtbl gameuxcf_vtbl =
{
    gameuxcf_QueryInterface,
    gameuxcf_AddRef,
    gameuxcf_Release,
    gameuxcf_CreateInstance,
    gameuxcf_LockServer
};

static gameuxcf gameexplorercf = { { &gameuxcf_vtbl }, GameExplorer_create };
static gameuxcf gamestatisticscf  = { { &gameuxcf_vtbl }, GameStatistics_create };

/***************************************************************
 * gameux ClassFactory
 */
HRESULT WINAPI DllGetClassObject(
        REFCLSID rclsid,
        REFIID riid,
        LPVOID *ppv)
{
    IClassFactory *cf = NULL;

    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(IsEqualCLSID(rclsid, &CLSID_GameExplorer))
    {
        cf = &gameexplorercf.IClassFactory_iface;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_GameStatistics ))
    {
        cf = &gamestatisticscf.IClassFactory_iface;
    }

    if(!cf)
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface(cf, riid, ppv);
}
