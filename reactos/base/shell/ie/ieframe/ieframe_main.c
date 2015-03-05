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

#include "ieframe.h"

#include <rpcproxy.h>

LONG module_ref = 0;
HINSTANCE ieframe_instance;

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static REFIID tid_ids[] = {
#define XIID(iface) &IID_ ## iface,
#define XCLSID(class) &CLSID_ ## class,
TID_LIST
#undef XIID
#undef XCLSID
};

static HRESULT load_typelib(void)
{
    HRESULT hres;
    ITypeLib *tl;

    hres = LoadRegTypeLib(&LIBID_SHDocVw, 1, 1, LOCALE_SYSTEM_DEFAULT, &tl);
    if(FAILED(hres)) {
        ERR("LoadRegTypeLib failed: %08x\n", hres);
        return hres;
    }

    if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hres;
}

HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if(!typelib)
        hres = load_typelib();
    if(!typelib)
        return hres;

    if(!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    return S_OK;
}

static void release_typelib(void)
{
    unsigned i;

    if(!typelib)
        return;

    for(i=0; i < sizeof(typeinfos)/sizeof(*typeinfos); i++) {
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);
    }

    ITypeLib_Release(typelib);
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

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
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

static const IClassFactoryVtbl WebBrowserFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    WebBrowser_Create,
    ClassFactory_LockServer
};

static IClassFactory WebBrowserFactory = { &WebBrowserFactoryVtbl };

static const IClassFactoryVtbl WebBrowserV1FactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    WebBrowserV1_Create,
    ClassFactory_LockServer
};

static IClassFactory WebBrowserV1Factory = { &WebBrowserV1FactoryVtbl };

static const IClassFactoryVtbl InternetShortcutFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InternetShortcut_Create,
    ClassFactory_LockServer
};

static IClassFactory InternetShortcutFactory = { &InternetShortcutFactoryVtbl };

static const IClassFactoryVtbl CUrlHistoryFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    CUrlHistory_Create,
    ClassFactory_LockServer
};

static IClassFactory CUrlHistoryFactory = { &CUrlHistoryFactoryVtbl };

/******************************************************************
 *              DllMain (ieframe.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("(%p %d %p)\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        ieframe_instance = hInstDLL;
        register_iewindow_class();
        DisableThreadLibraryCalls(ieframe_instance);
        break;
    case DLL_PROCESS_DETACH:
        if (lpv) break;
        unregister_iewindow_class();
        release_typelib();
    }

    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject	(ieframe.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_WebBrowser, rclsid)) {
        TRACE("(CLSID_WebBrowser %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&WebBrowserFactory, riid, ppv);
    }

    if(IsEqualGUID(&CLSID_WebBrowser_V1, rclsid)) {
        TRACE("(CLSID_WebBrowser_V1 %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&WebBrowserV1Factory, riid, ppv);
    }

    if(IsEqualGUID(rclsid, &CLSID_InternetShortcut)) {
        TRACE("(CLSID_InternetShortcut %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&InternetShortcutFactory, riid, ppv);
    }

    if(IsEqualGUID(&CLSID_CUrlHistory, rclsid)) {
        TRACE("(CLSID_CUrlHistory %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&CUrlHistoryFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

static const IClassFactoryVtbl InternetExplorerFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InternetExplorer_Create,
    ClassFactory_LockServer
};

static IClassFactory InternetExplorerFactory = { &InternetExplorerFactoryVtbl };

HRESULT register_class_object(BOOL do_reg)
{
    HRESULT hres;

    static DWORD cookie;

    if(do_reg) {
        hres = CoRegisterClassObject(&CLSID_InternetExplorer,
                (IUnknown*)&InternetExplorerFactory, CLSCTX_SERVER,
                REGCLS_MULTIPLEUSE|REGCLS_SUSPENDED, &cookie);
        if (FAILED(hres)) {
            ERR("failed to register object %08x\n", hres);
            return hres;
        }

        hres = CoResumeClassObjects();
        if(SUCCEEDED(hres))
            return hres;

        ERR("failed to resume object %08x\n", hres);
    }

    return CoRevokeClassObject(cookie);
}

/***********************************************************************
 *          DllCanUnloadNow (ieframe.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("()\n");
    return module_ref ? S_FALSE : S_OK;
}

/***********************************************************************
 *          DllRegisterServer (ieframe.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return __wine_register_resources(ieframe_instance);
}

/***********************************************************************
 *          DllUnregisterServer (ieframe.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("()\n");
    return __wine_unregister_resources(ieframe_instance);
}

/***********************************************************************
 *          IEGetWriteableHKCU (ieframe.@)
 */
HRESULT WINAPI IEGetWriteableHKCU(HKEY *pkey)
{
    FIXME("(%p) stub\n", pkey);
    return E_NOTIMPL;
}
