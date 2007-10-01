/*
 * browseui - Internet Explorer / Windows Explorer standard UI
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2004 Mike McCormack (for CodeWeavers)
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

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "shlguid.h"

#include "initguid.h"

#include "browseui.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

LONG BROWSEUI_refCount = 0;

HINSTANCE browseui_hinstance = 0;

typedef HRESULT (WINAPI *LPFNCONSTRUCTOR)(IUnknown *pUnkOuter, IUnknown **ppvOut);

static const struct {
    REFCLSID clsid;
    LPFNCONSTRUCTOR ctor;
} ClassesTable[] = {
    {&CLSID_ACLMulti, ACLMulti_Constructor},
    {NULL, NULL}
};

typedef struct tagClassFactory
{
    const IClassFactoryVtbl *vtbl;
    LONG   ref;
    LPFNCONSTRUCTOR ctor;
} ClassFactory;
static const IClassFactoryVtbl ClassFactoryVtbl;

static HRESULT ClassFactory_Constructor(LPFNCONSTRUCTOR ctor, LPVOID *ppvOut)
{
    ClassFactory *This = CoTaskMemAlloc(sizeof(ClassFactory));
    This->vtbl = &ClassFactoryVtbl;
    This->ref = 1;
    This->ctor = ctor;
    *ppvOut = (LPVOID)This;
    TRACE("Created class factory %p\n", This);
    BROWSEUI_refCount++;
    return S_OK;
}

static void ClassFactory_Destructor(ClassFactory *This)
{
    TRACE("Destroying class factory %p\n", This);
    CoTaskMemFree(This);
    BROWSEUI_refCount--;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown)) {
        IClassFactory_AddRef(iface);
        *ppvOut = iface;
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    ClassFactory *This = (ClassFactory *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    ClassFactory *This = (ClassFactory *)iface;
    ULONG ret = InterlockedDecrement(&This->ref);

    if (ret == 0)
        ClassFactory_Destructor(This);
    return ret;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *punkOuter, REFIID iid, LPVOID *ppvOut)
{
    ClassFactory *This = (ClassFactory *)iface;
    HRESULT ret;
    IUnknown *obj;

    TRACE("(%p, %p, %s, %p)\n", iface, punkOuter, debugstr_guid(iid), ppvOut);
    ret = This->ctor(punkOuter, &obj);
    if (FAILED(ret))
        return ret;
    ret = IUnknown_QueryInterface(obj, iid, ppvOut);
    IUnknown_Release(obj);
    return ret;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    ClassFactory *This = (ClassFactory *)iface;

    TRACE("(%p)->(%x)\n", This, fLock);

    if(fLock)
        InterlockedIncrement(&BROWSEUI_refCount);
    else
        InterlockedDecrement(&BROWSEUI_refCount);

    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    /* IUnknown */
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,

    /* IClassFactory*/
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

/*************************************************************************
 * BROWSEUI DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;   /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            browseui_hinstance = hinst;
            break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (BROWSEUI.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return BROWSEUI_refCount ? S_FALSE : S_OK;
}

/***********************************************************************
 *              DllGetVersion (BROWSEUI.@)
 */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *info)
{
    if (info->cbSize != sizeof(DLLVERSIONINFO)) FIXME("support DLLVERSIONINFO2\n");

    /* this is what IE6 on Windows 98 reports */
    info->dwMajorVersion = 6;
    info->dwMinorVersion = 0;
    info->dwBuildNumber = 2600;
    info->dwPlatformID = DLLVER_PLATFORM_WINDOWS;

    return NOERROR;
}

/***********************************************************************
 *              DllGetClassObject (BROWSEUI.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppvOut)
{
    int i;

    *ppvOut = NULL;
    if (!IsEqualIID(iid, &IID_IUnknown) && !IsEqualIID(iid, &IID_IClassFactory))
        return E_NOINTERFACE;

    for (i = 0; ClassesTable[i].clsid != NULL; i++)
        if (IsEqualCLSID(ClassesTable[i].clsid, clsid)) {
            return ClassFactory_Constructor(ClassesTable[i].ctor, ppvOut);
        }
    FIXME("CLSID %s not supported\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
