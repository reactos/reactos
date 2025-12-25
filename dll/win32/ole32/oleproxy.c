/*
 *	OLE32 proxy/stub handler
 *
 *  Copyright 2002  Marcus Meissner
 *  Copyright 2001  Ove Kåven, TransGaming Technologies
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "rpc.h"

#include "compobj_private.h"
#include "moniker.h"
#include "comcat.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%x)\n", fLock);
    return S_OK;
}

static const IClassFactoryVtbl FileMonikerCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    FileMoniker_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory FileMonikerCF = { &FileMonikerCFVtbl };

static const IClassFactoryVtbl ItemMonikerCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ItemMoniker_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ItemMonikerCF = { &ItemMonikerCFVtbl };

static const IClassFactoryVtbl AntiMonikerCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    AntiMoniker_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory AntiMonikerCF = { &AntiMonikerCFVtbl };

static const IClassFactoryVtbl CompositeMonikerCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    CompositeMoniker_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory CompositeMonikerCF = { &CompositeMonikerCFVtbl };

static const IClassFactoryVtbl ClassMonikerCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassMoniker_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ClassMonikerCF = { &ClassMonikerCFVtbl };

static const IClassFactoryVtbl PointerMonikerCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    PointerMoniker_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory PointerMonikerCF = { &PointerMonikerCFVtbl };

static const IClassFactoryVtbl ObjrefMonikerCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ObjrefMoniker_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ObjrefMonikerCF = { &ObjrefMonikerCFVtbl };

static const IClassFactoryVtbl ComCatCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ComCat_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ComCatCF = { &ComCatCFVtbl };

static const IClassFactoryVtbl GlobalInterfaceTableCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    GlobalInterfaceTable_CreateInstance,
    ClassFactory_LockServer
};

IClassFactory GlobalInterfaceTableCF = { &GlobalInterfaceTableCFVtbl };

static const IClassFactoryVtbl ManualResetEventCFVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ManualResetEvent_CreateInstance,
    ClassFactory_LockServer
};

IClassFactory ManualResetEventCF = { &ManualResetEventCFVtbl };

/***********************************************************************
 *           DllGetClassObject [OLE32.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{
    HRESULT hr;

    *ppv = NULL;
    if (IsEqualIID(rclsid,&CLSID_DfMarshal)&&(
		IsEqualIID(iid,&IID_IClassFactory) ||
		IsEqualIID(iid,&IID_IUnknown)
	)
    )
	return MARSHAL_GetStandardMarshalCF(ppv);
    if (IsEqualCLSID(rclsid, &CLSID_StdGlobalInterfaceTable))
        return IClassFactory_QueryInterface(&GlobalInterfaceTableCF, iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_ManualResetEvent))
        return IClassFactory_QueryInterface(&ManualResetEventCF, iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_FileMoniker))
        return IClassFactory_QueryInterface(&FileMonikerCF, iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_ItemMoniker))
        return IClassFactory_QueryInterface(&ItemMonikerCF, iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_AntiMoniker))
        return IClassFactory_QueryInterface(&AntiMonikerCF, iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_CompositeMoniker))
        return IClassFactory_QueryInterface(&CompositeMonikerCF, iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_ClassMoniker))
        return IClassFactory_QueryInterface(&ClassMonikerCF, iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_ObjrefMoniker))
        return IClassFactory_QueryInterface(&ObjrefMonikerCF, iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_PointerMoniker))
        return IClassFactory_QueryInterface(&PointerMonikerCF, iid, ppv);
    if (IsEqualGUID(rclsid, &CLSID_StdComponentCategoriesMgr))
        return IClassFactory_QueryInterface(&ComCatCF, iid, ppv);

    hr = OLE32_DllGetClassObject(rclsid, iid, ppv);
    if (SUCCEEDED(hr))
        return hr;

    return Handler_DllGetClassObject(rclsid, iid, ppv);
}

/***********************************************************************
 *           Ole32DllGetClassObject [OLE32.@]
 */
HRESULT WINAPI Ole32DllGetClassObject(REFCLSID rclsid, REFIID riid, void **obj)
{
    if (IsEqualCLSID(rclsid, &CLSID_StdGlobalInterfaceTable))
        return IClassFactory_QueryInterface(&GlobalInterfaceTableCF, riid, obj);
    else if (IsEqualCLSID(rclsid, &CLSID_ManualResetEvent))
        return IClassFactory_QueryInterface(&ManualResetEventCF, riid, obj);
    else if (IsEqualCLSID(rclsid, &CLSID_InProcFreeMarshaler))
        return FTMarshalCF_Create(riid, obj);
    else
        return CLASS_E_CLASSNOTAVAILABLE;
}
