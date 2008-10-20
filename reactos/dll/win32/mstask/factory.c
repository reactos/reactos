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

#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

static HRESULT WINAPI MSTASK_IClassFactory_QueryInterface(
        LPCLASSFACTORY iface,
        REFIID riid,
        LPVOID *ppvObj)
{
    ClassFactoryImpl *This = (ClassFactoryImpl *)iface;

    TRACE("IID: %s\n",debugstr_guid(riid));
    if (ppvObj == NULL)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = &This->lpVtbl;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unknown interface: %s\n", debugstr_guid(riid));
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    TRACE("\n");
    InterlockedIncrement(&dll_ref);
    return 2;
}

static ULONG WINAPI MSTASK_IClassFactory_Release(LPCLASSFACTORY iface)
{
    TRACE("\n");
    InterlockedDecrement(&dll_ref);
    return 1;
}

static HRESULT WINAPI MSTASK_IClassFactory_CreateInstance(
        LPCLASSFACTORY iface,
        LPUNKNOWN pUnkOuter,
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

    res = ITaskScheduler_QueryInterface(punk, riid, ppvObj);
    ITaskScheduler_Release(punk);
    return res;
}

static HRESULT WINAPI MSTASK_IClassFactory_LockServer(
        LPCLASSFACTORY iface,
        BOOL fLock)
{
    TRACE("\n");

    if (fLock != FALSE)
        MSTASK_IClassFactory_AddRef(iface);
    else
        MSTASK_IClassFactory_Release(iface);
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

ClassFactoryImpl MSTASK_ClassFactory = { &IClassFactory_Vtbl };
