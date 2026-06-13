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
#include "jsdisp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

LONG module_ref = 0;

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

HINSTANCE jscript_hinstance;
static DWORD jscript_tls;
static ITypeInfo *dispatch_typeinfo;

static int weak_refs_compare(const void *key, const struct rb_entry *entry)
{
    const struct weak_refs_entry *weak_refs_entry = RB_ENTRY_VALUE(entry, const struct weak_refs_entry, entry);
    ULONG_PTR a = (ULONG_PTR)key, b = (ULONG_PTR)LIST_ENTRY(weak_refs_entry->list.next, struct weakmap_entry, weak_refs_entry)->key;
    return (a > b) - (a < b);
}

struct thread_data *get_thread_data(void)
{
    struct thread_data *thread_data = TlsGetValue(jscript_tls);

    if(!thread_data) {
        thread_data = calloc(1, sizeof(struct thread_data));
        if(!thread_data)
            return NULL;
        thread_data->thread_id = GetCurrentThreadId();
        list_init(&thread_data->objects);
        rb_init(&thread_data->weak_refs, weak_refs_compare);
        TlsSetValue(jscript_tls, thread_data);
    }

    thread_data->ref++;
    return thread_data;
}

void release_thread_data(struct thread_data *thread_data)
{
    if(--thread_data->ref)
        return;

    free(thread_data);
    TlsSetValue(jscript_tls, NULL);
}

HRESULT get_dispatch_typeinfo(ITypeInfo **out)
{
    ITypeInfo *typeinfo;
    ITypeLib *typelib;
    HRESULT hr;

    if (!dispatch_typeinfo)
    {
        hr = LoadRegTypeLib(&IID_StdOle, STDOLE_MAJORVERNUM, STDOLE_MINORVERNUM, STDOLE_LCID, &typelib);
        if (FAILED(hr)) return hr;

        hr = ITypeLib_GetTypeInfoOfGuid(typelib, &IID_IDispatch, &typeinfo);
        ITypeLib_Release(typelib);
        if (FAILED(hr)) return hr;

        if (InterlockedCompareExchangePointer((void**)&dispatch_typeinfo, typeinfo, NULL))
            ITypeInfo_Release(typeinfo);
    }

    *out = dispatch_typeinfo;
    return S_OK;
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
    TRACE("(%p %ld %p)\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        jscript_hinstance = hInstDLL;
        jscript_tls = TlsAlloc();
        if(jscript_tls == TLS_OUT_OF_INDEXES || !init_strings())
            return FALSE;
        break;
    case DLL_PROCESS_DETACH:
        if (lpv) break;
        if (dispatch_typeinfo) ITypeInfo_Release(dispatch_typeinfo);
        if(jscript_tls != TLS_OUT_OF_INDEXES) TlsFree(jscript_tls);
        free_strings();
        break;
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
    TRACE("() ref=%ld\n", module_ref);

    return module_ref ? S_FALSE : S_OK;
}
