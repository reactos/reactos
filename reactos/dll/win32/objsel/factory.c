/*
 *	ClassFactory implementation for OBJSEL.dll
 *
 * Copyright (C) 2002 John K. Hohm
 * Copyright (C) 2002 Robert Shearman
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

#include "objsel_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(objsel);


/**********************************************************************
 * OBJSEL_IClassFactory_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI OBJSEL_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IClassFactory))
    {
	*ppvObj = (LPVOID)iface;
	IClassFactory_AddRef(iface);
	return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IDsObjectPicker))
    {
        return IClassFactory_CreateInstance(iface, NULL, riid, ppvObj);
    }

    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}


/**********************************************************************
 * OBJSEL_IClassFactory_AddRef (also IUnknown)
 */
static ULONG WINAPI OBJSEL_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    ClassFactoryImpl *This = (ClassFactoryImpl *)iface;
    ULONG ref;

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    ref = InterlockedIncrement(&This->ref);

    if (ref == 1)
    {
        InterlockedIncrement(&dll_refs);
    }

    return ref;
}


/**********************************************************************
 * OBJSEL_IClassFactory_Release (also IUnknown)
 */
static ULONG WINAPI OBJSEL_IClassFactory_Release(LPCLASSFACTORY iface)
{
    ClassFactoryImpl *This = (ClassFactoryImpl *)iface;
    ULONG ref;

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
    {
        InterlockedDecrement(&dll_refs);
    }

    return ref;
}


/**********************************************************************
 * OBJSEL_IClassFactory_CreateInstance
 */
static HRESULT WINAPI OBJSEL_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    /* Don't support aggregation (Windows doesn't) */
    if (pUnkOuter != NULL) return CLASS_E_NOAGGREGATION;

    if (IsEqualGUID(&IID_IDsObjectPicker, riid))
    {
        return OBJSEL_IDsObjectPicker_Create(ppvObj);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}


/**********************************************************************
 * OBJSEL_IClassFactory_LockServer
 */
static HRESULT WINAPI OBJSEL_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    TRACE("\n");

    if (fLock)
        IClassFactory_AddRef(iface);
    else
        IClassFactory_Release(iface);
    return S_OK;
}


/**********************************************************************
 * IClassFactory_Vtbl
 */
static IClassFactoryVtbl IClassFactory_Vtbl =
{
    OBJSEL_IClassFactory_QueryInterface,
    OBJSEL_IClassFactory_AddRef,
    OBJSEL_IClassFactory_Release,
    OBJSEL_IClassFactory_CreateInstance,
    OBJSEL_IClassFactory_LockServer
};


/**********************************************************************
 * static ClassFactory instance
 */

ClassFactoryImpl OBJSEL_ClassFactory = { &IClassFactory_Vtbl, 0 };
