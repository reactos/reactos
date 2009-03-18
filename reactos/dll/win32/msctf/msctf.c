/*
 * MSCTF Server DLL
 *
 * Copyright 2008 Aric Stewart, CodeWeavers
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
#include "comcat.h"
#include "initguid.h"
#include "msctf.h"

#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

static LONG MSCTF_refCount;

static HINSTANCE MSCTF_hinstance;

DWORD tlsIndex = 0;

const WCHAR szwSystemTIPKey[] = {'S','O','F','T','W','A','R','E','\\','M','i','c','r','o','s','o','f','t','\\','C','T','F','\\','T','I','P',0};

typedef HRESULT (*LPFNCONSTRUCTOR)(IUnknown *pUnkOuter, IUnknown **ppvOut);

static const struct {
    REFCLSID clsid;
    LPFNCONSTRUCTOR ctor;
} ClassesTable[] = {
    {&CLSID_TF_ThreadMgr, ThreadMgr_Constructor},
    {&CLSID_TF_InputProcessorProfiles, InputProcessorProfiles_Constructor},
    {&CLSID_TF_CategoryMgr, CategoryMgr_Constructor},
    {NULL, NULL}
};

typedef struct tagClassFactory
{
    const IClassFactoryVtbl *vtbl;
    LONG   ref;
    LPFNCONSTRUCTOR ctor;
} ClassFactory;

static void ClassFactory_Destructor(ClassFactory *This)
{
    TRACE("Destroying class factory %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
    MSCTF_refCount--;
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
        InterlockedIncrement(&MSCTF_refCount);
    else
        InterlockedDecrement(&MSCTF_refCount);

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

static HRESULT ClassFactory_Constructor(LPFNCONSTRUCTOR ctor, LPVOID *ppvOut)
{
    ClassFactory *This = HeapAlloc(GetProcessHeap(),0,sizeof(ClassFactory));
    This->vtbl = &ClassFactoryVtbl;
    This->ref = 1;
    This->ctor = ctor;
    *ppvOut = This;
    TRACE("Created class factory %p\n", This);
    MSCTF_refCount++;
    return S_OK;
}

/*************************************************************************
 * MSCTF DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinst, fdwReason, fImpLoad);
    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;   /* prefer native version */
        case DLL_PROCESS_ATTACH:
            MSCTF_hinstance = hinst;
            tlsIndex = TlsAlloc();
            break;
        case DLL_PROCESS_DETACH:
            TlsFree(tlsIndex);
            break;
    }
    return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (MSCTF.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return MSCTF_refCount ? S_FALSE : S_OK;
}

/***********************************************************************
 *              DllGetClassObject (MSCTF.@)
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

/***********************************************************************
 *              TF_CreateThreadMgr (MSCTF.@)
 */
HRESULT WINAPI TF_CreateThreadMgr(ITfThreadMgr **pptim)
{
    TRACE("\n");
    return ThreadMgr_Constructor(NULL,(IUnknown**)pptim);
}

/***********************************************************************
 *              TF_GetThreadMgr (MSCTF.@)
 */
HRESULT WINAPI TF_GetThreadMgr(ITfThreadMgr **pptim)
{
    TRACE("\n");
    *pptim = TlsGetValue(tlsIndex);

    if (*pptim)
        ITfThreadMgr_AddRef(*pptim);

    return S_OK;
}

/***********************************************************************
 *              SetInputScope(MSCTF.@)
 */
HRESULT WINAPI SetInputScope(HWND hwnd, INT inputscope)
{
    FIXME("STUB: %p %i\n",hwnd,inputscope);
    return S_OK;
}

/***********************************************************************
 *              SetInputScopes(MSCTF.@)
 */
HRESULT WINAPI SetInputScopes(HWND hwnd, const INT *pInputScopes,
                              UINT cInputScopes, WCHAR **ppszPhraseList,
                              UINT cPhrases, WCHAR *pszRegExp, WCHAR *pszSRGS)
{
    int i;
    FIXME("STUB: %p ... %s %s\n",hwnd, debugstr_w(pszRegExp), debugstr_w(pszSRGS));
    for (i = 0; i < cInputScopes; i++)
        TRACE("\tScope[%i] = %i\n",i,pInputScopes[i]);
    for (i = 0; i < cPhrases; i++)
        TRACE("\tPhrase[%i] = %s\n",i,debugstr_w(ppszPhraseList[i]));

    return S_OK;
}
