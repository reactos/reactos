/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include "wshom_private.h"

#include "initguid.h"
#include "wshom.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wshom);

static inline struct provideclassinfo *impl_from_IProvideClassInfo(IProvideClassInfo *iface)
{
    return CONTAINING_RECORD(iface, struct provideclassinfo, IProvideClassInfo_iface);
}

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static REFIID tid_ids[] = {
    &IID_NULL,
    &IID_IWshCollection,
    &IID_IWshEnvironment,
    &IID_IWshExec,
    &IID_IWshShell3,
    &IID_IWshShortcut,
    &IID_IWshNetwork2
};

static HRESULT load_typelib(void)
{
    HRESULT hres;
    ITypeLib *tl;

    if(typelib)
        return S_OK;

    hres = LoadRegTypeLib(&LIBID_IWshRuntimeLibrary, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if(FAILED(hres)) {
        ERR("LoadRegTypeLib failed: %#lx.\n", hres);
        return hres;
    }

    if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hres;
}

static HRESULT get_typeinfo_of_guid(const GUID *guid, ITypeInfo **tinfo)
{
    HRESULT hres;

    if(FAILED(hres = load_typelib()))
        return hres;

    return ITypeLib_GetTypeInfoOfGuid(typelib, guid, tinfo);
}

HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if (FAILED(hres = load_typelib()))
        return hres;

    if(!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %#lx.\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    ITypeInfo_AddRef(*typeinfo);
    return S_OK;
}

static
void release_typelib(void)
{
    unsigned i;

    if(!typelib)
        return;

    for(i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

static HRESULT WINAPI provideclassinfo_QueryInterface(IProvideClassInfo *iface, REFIID riid, void **obj)
{
    struct provideclassinfo *This = impl_from_IProvideClassInfo(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IProvideClassInfo)) {
        *obj = iface;
        IProvideClassInfo_AddRef(iface);
        return S_OK;
    }
    else
        return IUnknown_QueryInterface(This->outer, riid, obj);
}

static ULONG WINAPI provideclassinfo_AddRef(IProvideClassInfo *iface)
{
    struct provideclassinfo *This = impl_from_IProvideClassInfo(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI provideclassinfo_Release(IProvideClassInfo *iface)
{
    struct provideclassinfo *This = impl_from_IProvideClassInfo(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI provideclassinfo_GetClassInfo(IProvideClassInfo *iface, ITypeInfo **ti)
{
    struct provideclassinfo *This = impl_from_IProvideClassInfo(iface);

    TRACE("(%p)->(%p)\n", This, ti);

    return get_typeinfo_of_guid(This->guid, ti);
}

static const IProvideClassInfoVtbl provideclassinfovtbl = {
    provideclassinfo_QueryInterface,
    provideclassinfo_AddRef,
    provideclassinfo_Release,
    provideclassinfo_GetClassInfo
};

void init_classinfo(const GUID *guid, IUnknown *outer, struct provideclassinfo *classinfo)
{
    classinfo->IProvideClassInfo_iface.lpVtbl = &provideclassinfovtbl;
    classinfo->outer = outer;
    classinfo->guid = guid;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl WshShellFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    WshShellFactory_CreateInstance,
    ClassFactory_LockServer
};

static const IClassFactoryVtbl WshNetworkFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    WshNetworkFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory WshShellFactory = { &WshShellFactoryVtbl };
static IClassFactory WshNetworkFactory = { &WshNetworkFactoryVtbl };

/******************************************************************
 *              DllMain (wshom.ocx.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("%p, %ld, %p.\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        if (lpv) break;
        release_typelib();
        break;
    }

    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject	(wshom.ocx.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_WshShell, rclsid)) {
        TRACE("(CLSID_WshShell %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&WshShellFactory, riid, ppv);
    }
    else if(IsEqualGUID(&CLSID_WshNetwork, rclsid)) {
        TRACE("(CLSID_WshNetwork %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&WshNetworkFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
