/*
 * Copyright (C) 2008 Google (Roy Shea)
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
#include "taskschd.h"
#include "mstask.h"
#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

struct ClassFactoryImpl
{
    IClassFactory IClassFactory_iface;
    LONG ref;
};

static inline ClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI MSTASK_IClassFactory_QueryInterface(
        LPCLASSFACTORY iface,
        REFIID riid,
        LPVOID *ppvObj)
{
    ClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("IID: %s\n",debugstr_guid(riid));
    if (ppvObj == NULL)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = &This->IClassFactory_iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unknown interface: %s\n", debugstr_guid(riid));
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_IClassFactory_AddRef(IClassFactory *face)
{
    TRACE("\n");
    InterlockedIncrement(&dll_ref);
    return 2;
}

static ULONG WINAPI MSTASK_IClassFactory_Release(IClassFactory *iface)
{
    TRACE("\n");
    InterlockedDecrement(&dll_ref);
    return 1;
}

static HRESULT WINAPI MSTASK_IClassFactory_CreateInstance(
        IClassFactory *iface,
        IUnknown *pUnkOuter,
        REFIID riid,
        LPVOID *ppvObj)
{
    HRESULT res;
    IUnknown *punk = NULL;
    *ppvObj = NULL;

    TRACE("IID: %s\n",debugstr_guid(riid));

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    res = TaskSchedulerConstructor((LPVOID*) &punk);
    if (FAILED(res))
        return res;

    res = IUnknown_QueryInterface(punk, riid, ppvObj);
    IUnknown_Release(punk);
    return res;
}

static HRESULT WINAPI MSTASK_IClassFactory_LockServer(
        IClassFactory *iface,
        BOOL fLock)
{
    TRACE("\n");

    if (fLock)
        IClassFactory_AddRef(iface);
    else
        IClassFactory_Release(iface);
    return S_OK;
}

static const IClassFactoryVtbl IClassFactory_Vtbl =
{
    MSTASK_IClassFactory_QueryInterface,
    MSTASK_IClassFactory_AddRef,
    MSTASK_IClassFactory_Release,
    MSTASK_IClassFactory_CreateInstance,
    MSTASK_IClassFactory_LockServer
};

ClassFactoryImpl MSTASK_ClassFactory = { { &IClassFactory_Vtbl } };
