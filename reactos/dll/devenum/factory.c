/*
 *	ClassFactory implementation for DEVENUM.dll
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "devenum_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

/**********************************************************************
 * DEVENUM_IClassFactory_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI DEVENUM_IClassFactory_QueryInterface(
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
    else if (IsEqualGUID(riid, &IID_IParseDisplayName))
    {
        return IClassFactory_CreateInstance(iface, NULL, riid, ppvObj);
    }

    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * DEVENUM_IClassFactory_AddRef (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    TRACE("\n");

    DEVENUM_LockModule();

    return 2; /* non-heap based object */
}

/**********************************************************************
 * DEVENUM_IClassFactory_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IClassFactory_Release(LPCLASSFACTORY iface)
{
    TRACE("\n");

    DEVENUM_UnlockModule();

    return 1; /* non-heap based object */
}

/**********************************************************************
 * DEVENUM_IClassFactory_CreateInstance
 */
static HRESULT WINAPI DEVENUM_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    /* Don't support aggregation (Windows doesn't) */
    if (pUnkOuter != NULL) return CLASS_E_NOAGGREGATION;

    if (IsEqualGUID(&IID_ICreateDevEnum, riid))
    {
        *ppvObj = &DEVENUM_CreateDevEnum;
        return S_OK;
    }
    if (IsEqualGUID(&IID_IParseDisplayName, riid))
    {
        *ppvObj = &DEVENUM_ParseDisplayName;
        return S_OK;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

/**********************************************************************
 * DEVENUM_IClassFactory_LockServer
 */
static HRESULT WINAPI DEVENUM_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    TRACE("\n");

    if (fLock)
        DEVENUM_LockModule();
    else
        DEVENUM_UnlockModule();
    return S_OK;
}

/**********************************************************************
 * IClassFactory_Vtbl
 */
static IClassFactoryVtbl IClassFactory_Vtbl =
{
    DEVENUM_IClassFactory_QueryInterface,
    DEVENUM_IClassFactory_AddRef,
    DEVENUM_IClassFactory_Release,
    DEVENUM_IClassFactory_CreateInstance,
    DEVENUM_IClassFactory_LockServer
};

/**********************************************************************
 * static ClassFactory instance
 */
ClassFactoryImpl DEVENUM_ClassFactory = { &IClassFactory_Vtbl };
