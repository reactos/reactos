/*
 *	ClassFactory implementation for comcat.dll
 *
 * Copyright (C) 2002 John K. Hohm
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

#include "comcat_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static ULONG WINAPI COMCAT_IClassFactory_AddRef(LPCLASSFACTORY iface);

/**********************************************************************
 * COMCAT_IClassFactory_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI COMCAT_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ClassFactoryImpl *This = (ClassFactoryImpl *)iface;
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IClassFactory))
    {
	*ppvObj = (LPVOID)iface;
	COMCAT_IClassFactory_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

/**********************************************************************
 * COMCAT_IClassFactory_AddRef (also IUnknown)
 */
static ULONG WINAPI COMCAT_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    ClassFactoryImpl *This = (ClassFactoryImpl *)iface;
    ULONG ref;

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    ref = InterlockedIncrement(&This->ref);
    if (ref == 1) {
	InterlockedIncrement(&dll_ref);
    }
    return ref;
}

/**********************************************************************
 * COMCAT_IClassFactory_Release (also IUnknown)
 */
static ULONG WINAPI COMCAT_IClassFactory_Release(LPCLASSFACTORY iface)
{
    ClassFactoryImpl *This = (ClassFactoryImpl *)iface;
    ULONG ref;

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
	InterlockedDecrement(&dll_ref);
    }
    return ref;
}

/**********************************************************************
 * COMCAT_IClassFactory_CreateInstance
 */
static HRESULT WINAPI COMCAT_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    ClassFactoryImpl *This = (ClassFactoryImpl *)iface;
    HRESULT res;
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    /* Don't support aggregation (Windows doesn't) */
    if (pUnkOuter != NULL) return CLASS_E_NOAGGREGATION;

    res = IUnknown_QueryInterface((LPUNKNOWN)&COMCAT_ComCatMgr, riid, ppvObj);
    if (SUCCEEDED(res)) {
	return res;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

/**********************************************************************
 * COMCAT_IClassFactory_LockServer
 */
static HRESULT WINAPI COMCAT_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    TRACE("\n");

    if (fLock != FALSE) {
	IClassFactory_AddRef(iface);
    } else {
	IClassFactory_Release(iface);
    }
    return S_OK;
}

/**********************************************************************
 * IClassFactory_Vtbl
 */
static const IClassFactoryVtbl IClassFactory_Vtbl =
{
    COMCAT_IClassFactory_QueryInterface,
    COMCAT_IClassFactory_AddRef,
    COMCAT_IClassFactory_Release,
    COMCAT_IClassFactory_CreateInstance,
    COMCAT_IClassFactory_LockServer
};

/**********************************************************************
 * static ClassFactory instance
 */
ClassFactoryImpl COMCAT_ClassFactory = { &IClassFactory_Vtbl, 0 };
