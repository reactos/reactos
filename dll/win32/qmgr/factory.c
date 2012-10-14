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

#define COBJMACROS

#include "qmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

static ULONG WINAPI
BITS_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    return 2;
}

static HRESULT WINAPI
BITS_IClassFactory_QueryInterface(LPCLASSFACTORY iface, REFIID riid,
                                  LPVOID *ppvObj)
{
    ClassFactoryImpl *This = (ClassFactoryImpl *) iface;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = &This->lpVtbl;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI
BITS_IClassFactory_Release(LPCLASSFACTORY iface)
{
    return 1;
}

static HRESULT WINAPI
BITS_IClassFactory_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pUnkOuter,
                                  REFIID riid, LPVOID *ppvObj)
{
    HRESULT res;
    IUnknown *punk = NULL;

    TRACE("IID: %s\n", debugstr_guid(riid));

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    res = BackgroundCopyManagerConstructor(pUnkOuter, (LPVOID*) &punk);
    if (FAILED(res))
        return res;

    res = IUnknown_QueryInterface(punk, riid, ppvObj);
    IUnknown_Release(punk);
    return res;
}

static HRESULT WINAPI
BITS_IClassFactory_LockServer(LPCLASSFACTORY iface, BOOL fLock)
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
    &BITS_IClassFactory_Vtbl
};
