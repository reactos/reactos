/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "initguid.h"

#include "jscript.h"

#include "winreg.h"
#include "advpub.h"
#include "activaut.h"
#include "objsafe.h"
#include "mshtmhst.h"
#include "rpcproxy.h"
#include "jscript_classes.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

LONG module_ref = 0;

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

HINSTANCE jscript_hinstance;

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

    if(fLock)
        lock_module();
    else
        unlock_module();

    return S_OK;
}

static HRESULT WINAPI JScriptFactory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    if(outer) {
        *ppv = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    return create_jscript_object(FALSE, riid, ppv);
}

static const IClassFactoryVtbl JScriptFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    JScriptFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory JScriptFactory = { &JScriptFactoryVtbl };

static HRESULT WINAPI JScriptEncodeFactory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    if(outer) {
        *ppv = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    return create_jscript_object(TRUE, riid, ppv);
}

static const IClassFactoryVtbl JScriptEncodeFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    JScriptEncodeFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory JScriptEncodeFactory = { &JScriptEncodeFactoryVtbl };

/******************************************************************
 *              DllMain (jscript.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("(%p %d %p)\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        jscript_hinstance = hInstDLL;
        if(!init_strings())
            return FALSE;
        break;
    case DLL_PROCESS_DETACH:
        if (lpv) break;
        free_strings();
    }

    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject	(jscript.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_JScript, rclsid)) {
        TRACE("(CLSID_JScript %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&JScriptFactory, riid, ppv);
    }

    if(IsEqualGUID(&CLSID_JScriptEncode, rclsid)) {
        TRACE("(CLSID_JScriptEncode %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&JScriptEncodeFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *          DllCanUnloadNow (jscript.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("() ref=%d\n", module_ref);

    return module_ref ? S_FALSE : S_OK;
}

/***********************************************************************
 *          DllRegisterServer (jscript.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return __wine_register_resources(jscript_hinstance);
}

/***********************************************************************
 *          DllUnregisterServer (jscript.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("()\n");
    return __wine_unregister_resources(jscript_hinstance);
}
