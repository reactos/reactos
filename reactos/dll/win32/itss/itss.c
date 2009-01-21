/*
 *    ITSS Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2004 Mike McCormack
 *
 *  see http://bonedaddy.net/pabs3/hhm/#chmspec
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "advpub.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "itsstor.h"

#include "initguid.h"
#include "wine/itss.h"

WINE_DEFAULT_DEBUG_CHANNEL(itss);

static HRESULT ITSS_create(IUnknown *pUnkOuter, LPVOID *ppObj);

LONG dll_count = 0;
static HINSTANCE hInst;

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        hInst = hInstDLL;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/******************************************************************************
 * ITSS ClassFactory
 */
typedef struct {
    const IClassFactoryVtbl *lpVtbl;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

static HRESULT WINAPI
ITSSCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
	*ppobj = This;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI ITSSCF_AddRef(LPCLASSFACTORY iface)
{
    ITSS_LockModule();
    return 2;
}

static ULONG WINAPI ITSSCF_Release(LPCLASSFACTORY iface)
{
    ITSS_UnlockModule();
    return 1;
}


static HRESULT WINAPI ITSSCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
					  REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;
    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
    if (SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI ITSSCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    TRACE("(%p)->(%d)\n", iface, dolock);

    if (dolock)
        ITSS_LockModule();
    else
        ITSS_UnlockModule();

    return S_OK;
}

static const IClassFactoryVtbl ITSSCF_Vtbl =
{
    ITSSCF_QueryInterface,
    ITSSCF_AddRef,
    ITSSCF_Release,
    ITSSCF_CreateInstance,
    ITSSCF_LockServer
};

static const IClassFactoryImpl ITStorage_factory = { &ITSSCF_Vtbl, ITSS_create };
static const IClassFactoryImpl MSITStore_factory = { &ITSSCF_Vtbl, ITS_IParseDisplayName_create };
static const IClassFactoryImpl ITSProtocol_factory = { &ITSSCF_Vtbl, ITSProtocol_create };

/***********************************************************************
 *		DllGetClassObject	(ITSS.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    const IClassFactoryImpl *factory;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (IsEqualGUID(&CLSID_ITStorage, rclsid))
        factory = &ITStorage_factory;
    else if (IsEqualGUID(&CLSID_MSITStore, rclsid))
        factory = &MSITStore_factory;
    else if (IsEqualGUID(&CLSID_ITSProtocol, rclsid))
        factory = &ITSProtocol_factory;
    else
    {
	FIXME("%s: no class found.\n", debugstr_guid(rclsid));
	return CLASS_E_CLASSNOTAVAILABLE;
    }

    return IUnknown_QueryInterface( (IUnknown*) factory, iid, ppv );
}

/*****************************************************************************/

typedef struct {
    const IITStorageVtbl *vtbl_IITStorage;
    LONG ref;
} ITStorageImpl;


static HRESULT WINAPI ITStorageImpl_QueryInterface(
    IITStorage* iface,
    REFIID riid,
    void** ppvObject)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IITStorage))
    {
	IClassFactory_AddRef(iface);
	*ppvObject = This;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI ITStorageImpl_AddRef(
    IITStorage* iface)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITStorageImpl_Release(
    IITStorage* iface)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
        ITSS_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI ITStorageImpl_StgCreateDocfile(
    IITStorage* iface,
    const WCHAR* pwcsName,
    DWORD grfMode,
    DWORD reserved,
    IStorage** ppstgOpen)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;

    TRACE("%p %s %u %u %p\n", This,
          debugstr_w(pwcsName), grfMode, reserved, ppstgOpen );

    return ITSS_StgOpenStorage( pwcsName, NULL, grfMode,
                                0, reserved, ppstgOpen);
}

static HRESULT WINAPI ITStorageImpl_StgCreateDocfileOnILockBytes(
    IITStorage* iface,
    ILockBytes* plkbyt,
    DWORD grfMode,
    DWORD reserved,
    IStorage** ppstgOpen)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_StgIsStorageFile(
    IITStorage* iface,
    const WCHAR* pwcsName)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_StgIsStorageILockBytes(
    IITStorage* iface,
    ILockBytes* plkbyt)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_StgOpenStorage(
    IITStorage* iface,
    const WCHAR* pwcsName,
    IStorage* pstgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage** ppstgOpen)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;

    TRACE("%p %s %p %d %p\n", This, debugstr_w( pwcsName ),
           pstgPriority, grfMode, snbExclude );

    return ITSS_StgOpenStorage( pwcsName, pstgPriority, grfMode,
                                snbExclude, reserved, ppstgOpen);
}

static HRESULT WINAPI ITStorageImpl_StgOpenStorageOnILockBytes(
    IITStorage* iface,
    ILockBytes* plkbyt,
    IStorage* pStgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage** ppstgOpen)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_StgSetTimes(
    IITStorage* iface,
    const WCHAR* lpszName,
    const FILETIME* pctime,
    const FILETIME* patime,
    const FILETIME* pmtime)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_SetControlData(
    IITStorage* iface,
    PITS_Control_Data pControlData)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_DefaultControlData(
    IITStorage* iface,
    PITS_Control_Data* ppControlData)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_Compact(
    IITStorage* iface,
    const WCHAR* pwcsName,
    ECompactionLev iLev)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static const IITStorageVtbl ITStorageImpl_Vtbl =
{
    ITStorageImpl_QueryInterface,
    ITStorageImpl_AddRef,
    ITStorageImpl_Release,
    ITStorageImpl_StgCreateDocfile,
    ITStorageImpl_StgCreateDocfileOnILockBytes,
    ITStorageImpl_StgIsStorageFile,
    ITStorageImpl_StgIsStorageILockBytes,
    ITStorageImpl_StgOpenStorage,
    ITStorageImpl_StgOpenStorageOnILockBytes,
    ITStorageImpl_StgSetTimes,
    ITStorageImpl_SetControlData,
    ITStorageImpl_DefaultControlData,
    ITStorageImpl_Compact,
};

static HRESULT ITSS_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    ITStorageImpl *its;

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    its = HeapAlloc( GetProcessHeap(), 0, sizeof(ITStorageImpl) );
    its->vtbl_IITStorage = &ITStorageImpl_Vtbl;
    its->ref = 1;

    TRACE("-> %p\n", its);
    *ppObj = (LPVOID) its;

    ITSS_LockModule();
    return S_OK;
}

/*****************************************************************************/

HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("dll_count = %u\n", dll_count);
    return dll_count ? S_FALSE : S_OK;
}

#define INF_SET_ID(id)            \
    do                            \
    {                             \
        static CHAR name[] = #id; \
                                  \
        pse[i].pszName = name;    \
        clsids[i++] = &id;        \
    } while (0)

#define INF_SET_CLSID(clsid) INF_SET_ID(CLSID_ ## clsid)

static HRESULT register_server(BOOL do_register)
{
    HRESULT hres;
    HMODULE hAdvpack;
    HRESULT (WINAPI *pRegInstall)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
    STRTABLEA strtable;
    STRENTRYA pse[4];
    static CLSID const *clsids[4];
    DWORD i = 0;

    static const WCHAR wszAdvpack[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    INF_SET_CLSID(ITStorage);
    INF_SET_CLSID(MSFSStore);
    INF_SET_CLSID(MSITStore);
    INF_SET_CLSID(ITSProtocol);

    strtable.cEntries = sizeof(pse)/sizeof(pse[0]);
    strtable.pse = pse;

    for(i=0; i < strtable.cEntries; i++) {
        pse[i].pszValue = HeapAlloc(GetProcessHeap(), 0, 39);
        sprintf(pse[i].pszValue, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsids[i]->Data1, clsids[i]->Data2, clsids[i]->Data3, clsids[i]->Data4[0],
                clsids[i]->Data4[1], clsids[i]->Data4[2], clsids[i]->Data4[3], clsids[i]->Data4[4],
                clsids[i]->Data4[5], clsids[i]->Data4[6], clsids[i]->Data4[7]);
    }

    hAdvpack = LoadLibraryW(wszAdvpack);
    pRegInstall = (void *)GetProcAddress(hAdvpack, "RegInstall");

    hres = pRegInstall(hInst, do_register ? "RegisterDll" : "UnregisterDll", &strtable);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++)
        HeapFree(GetProcessHeap(), 0, pse[i].pszValue);

    return hres;
}

#undef INF_SET_CLSID
#undef INF_SET_ID

/***********************************************************************
 *          DllRegisterServer (ITSS.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return register_server(TRUE);
}

/***********************************************************************
 *          DllUnregisterServer (ITSS.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return register_server(FALSE);
}
