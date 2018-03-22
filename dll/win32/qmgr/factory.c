/*
 * Class factory interface for Queue Manager (BITS)
 *
 * Copyright (C) 2007 Google (Roy Shea)
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

#include "qmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

static ULONG WINAPI
BITS_IClassFactory_AddRef(IClassFactory *iface)
{
    return 2; /* non-heap based object */
}

static HRESULT WINAPI
BITS_IClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppvObj)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = &BITS_ClassFactory.IClassFactory_iface;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI
BITS_IClassFactory_Release(IClassFactory *iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI
BITS_IClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid,
                                  void **ppvObj)
{
    HRESULT res;
    IUnknown *punk = NULL;

    TRACE("IID: %s\n", debugstr_guid(riid));

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    res = BackgroundCopyManagerConstructor((LPVOID*) &punk);
    if (FAILED(res))
        return res;

    res = IUnknown_QueryInterface(punk, riid, ppvObj);
    IUnknown_Release(punk);
    return res;
}

static HRESULT WINAPI
BITS_IClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static const IClassFactoryVtbl BITS_IClassFactory_Vtbl =
{
    BITS_IClassFactory_QueryInterface,
    BITS_IClassFactory_AddRef,
    BITS_IClassFactory_Release,
    BITS_IClassFactory_CreateInstance,
    BITS_IClassFactory_LockServer
};

ClassFactoryImpl BITS_ClassFactory =
{
    { &BITS_IClassFactory_Vtbl }
};
