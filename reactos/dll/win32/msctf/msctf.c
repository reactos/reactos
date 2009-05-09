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
#include "wine/list.h"
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

typedef struct
{
    DWORD id;
    DWORD magic;
    LPVOID data;
} CookieInternal;

typedef struct {
    TF_LANGUAGEPROFILE      LanguageProfile;
    ITfTextInputProcessor   *pITfTextInputProcessor;
    ITfThreadMgr            *pITfThreadMgr;
    TfClientId              tid;
} ActivatedTextService;

typedef struct
{
    struct list entry;
    ActivatedTextService *ats;
} AtsEntry;

static CookieInternal *cookies;
static UINT id_last;
static UINT array_size;

static struct list AtsList = LIST_INIT(AtsList);
static UINT activated = 0;

DWORD tlsIndex = 0;
TfClientId processId = 0;

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
 * DWORD Cookie Management
 */
DWORD generate_Cookie(DWORD magic, LPVOID data)
{
    int i;

    /* try to reuse IDs if possible */
    for (i = 0; i < id_last; i++)
        if (cookies[i].id == 0) break;

    if (i == array_size)
    {
        if (!array_size)
        {
            cookies = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CookieInternal) * 10);
            if (!cookies)
            {
                ERR("Out of memory, Unable to alloc cookies array\n");
                return 0;
            }
            array_size = 10;
        }
        else
        {
            CookieInternal *new_cookies = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cookies,
                                                      sizeof(CookieInternal) * (array_size * 2));
            if (!new_cookies)
            {
                ERR("Out of memory, Unable to realloc cookies array\n");
                return 0;
            }
            cookies = new_cookies;
            array_size *= 2;
        }
    }

    cookies[i].id = i + 1; /* a return of 0 is used for failure */
    cookies[i].magic = magic;
    cookies[i].data = data;

    if (i == id_last)
        id_last++;

    return cookies[i].id;
}

DWORD get_Cookie_magic(DWORD id)
{
    UINT index = id - 1;

    if (index >= id_last)
        return 0;

    if (cookies[index].id == 0)
        return 0;

    return cookies[index].magic;
}

LPVOID get_Cookie_data(DWORD id)
{
    UINT index = id - 1;

    if (index >= id_last)
        return NULL;

    if (cookies[index].id == 0)
        return NULL;

    return cookies[index].data;
}

LPVOID remove_Cookie(DWORD id)
{
    UINT index = id - 1;

    if (index >= id_last)
        return NULL;

    if (cookies[index].id == 0)
        return NULL;

    cookies[index].id = 0;
    return cookies[index].data;
}

DWORD enumerate_Cookie(DWORD magic, DWORD *index)
{
    int i;
    for (i = *index; i < id_last; i++)
        if (cookies[i].id != 0 && cookies[i].magic == magic)
        {
            *index = (i+1);
            return cookies[i].id;
        }
    return 0x0;
}

/*****************************************************************************
 * Active Text Service Management
 *****************************************************************************/
static HRESULT activate_given_ts(ActivatedTextService *actsvr, ITfThreadMgr* tm)
{
    HRESULT hr;

    /* Already Active? */
    if (actsvr->pITfTextInputProcessor)
        return S_OK;

    hr = CoCreateInstance (&actsvr->LanguageProfile.clsid, NULL, CLSCTX_INPROC_SERVER,
        &IID_ITfTextInputProcessor, (void**)&actsvr->pITfTextInputProcessor);
    if (FAILED(hr)) return hr;

    hr = ITfTextInputProcessor_Activate(actsvr->pITfTextInputProcessor, tm, actsvr->tid);
    if (FAILED(hr))
    {
        ITfTextInputProcessor_Release(actsvr->pITfTextInputProcessor);
        actsvr->pITfTextInputProcessor = NULL;
        return hr;
    }

    actsvr->pITfThreadMgr = tm;
    ITfThreadMgr_AddRef(tm);
    return hr;
}

static HRESULT deactivate_given_ts(ActivatedTextService *actsvr)
{
    HRESULT hr = S_OK;

    if (actsvr->pITfTextInputProcessor)
    {
        hr = ITfTextInputProcessor_Deactivate(actsvr->pITfTextInputProcessor);
        ITfTextInputProcessor_Release(actsvr->pITfTextInputProcessor);
        ITfThreadMgr_Release(actsvr->pITfThreadMgr);
        actsvr->pITfTextInputProcessor = NULL;
        actsvr->pITfThreadMgr = NULL;
    }

    return hr;
}

static void deactivate_remove_conflicting_ts(REFCLSID catid)
{
    AtsEntry *ats, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(ats, cursor2, &AtsList, AtsEntry, entry)
    {
        if (IsEqualCLSID(catid,&ats->ats->LanguageProfile.catid))
        {
            deactivate_given_ts(ats->ats);
            list_remove(&ats->entry);
            HeapFree(GetProcessHeap(),0,ats->ats);
            HeapFree(GetProcessHeap(),0,ats);
            /* we are guarenteeing there is only 1 */
            break;
        }
    }
}

HRESULT add_active_textservice(TF_LANGUAGEPROFILE *lp)
{
    ActivatedTextService *actsvr;
    ITfCategoryMgr *catmgr;
    AtsEntry *entry;
    ITfThreadMgr *tm = (ITfThreadMgr*)TlsGetValue(tlsIndex);
    ITfClientId *clientid;

    if (!tm) return E_UNEXPECTED;

    actsvr = HeapAlloc(GetProcessHeap(),0,sizeof(ActivatedTextService));
    if (!actsvr) return E_OUTOFMEMORY;

    entry = HeapAlloc(GetProcessHeap(),0,sizeof(AtsEntry));

    if (!entry)
    {
        HeapFree(GetProcessHeap(),0,actsvr);
        return E_OUTOFMEMORY;
    }

    ITfThreadMgr_QueryInterface(tm,&IID_ITfClientId,(LPVOID)&clientid);
    ITfClientId_GetClientId(clientid, &lp->clsid, &actsvr->tid);
    ITfClientId_Release(clientid);

    if (!actsvr->tid)
    {
        HeapFree(GetProcessHeap(),0,actsvr);
        return E_OUTOFMEMORY;
    }

    actsvr->pITfTextInputProcessor = NULL;
    actsvr->LanguageProfile = *lp;
    actsvr->LanguageProfile.fActive = TRUE;

    /* get TIP category */
    if (SUCCEEDED(CategoryMgr_Constructor(NULL,(IUnknown**)&catmgr)))
    {
        static const GUID *list[3] = {&GUID_TFCAT_TIP_SPEECH, &GUID_TFCAT_TIP_KEYBOARD, &GUID_TFCAT_TIP_HANDWRITING};

        ITfCategoryMgr_FindClosestCategory(catmgr,
                &actsvr->LanguageProfile.clsid, &actsvr->LanguageProfile.catid,
                list, 3);

        ITfCategoryMgr_Release(catmgr);
    }
    else
    {
        ERR("CategoryMgr construction failed\n");
        actsvr->LanguageProfile.catid = GUID_NULL;
    }

    if (!IsEqualGUID(&actsvr->LanguageProfile.catid,&GUID_NULL))
        deactivate_remove_conflicting_ts(&actsvr->LanguageProfile.catid);

    if (activated > 0)
        activate_given_ts(actsvr, tm);

    entry->ats = actsvr;
    list_add_head(&AtsList, &entry->entry);

    return S_OK;
}

BOOL get_active_textservice(REFCLSID rclsid, TF_LANGUAGEPROFILE *profile)
{
    AtsEntry *ats;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
    {
        if (IsEqualCLSID(rclsid,&ats->ats->LanguageProfile.clsid))
        {
            if (profile)
                *profile = ats->ats->LanguageProfile;
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT activate_textservices(ITfThreadMgr *tm)
{
    HRESULT hr = S_OK;
    AtsEntry *ats;

    activated ++;
    if (activated > 1)
        return S_OK;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
    {
        hr = activate_given_ts(ats->ats, tm);
        if (FAILED(hr))
            FIXME("Failed to activate text service\n");
    }
    return hr;
}

HRESULT deactivate_textservices(void)
{
    AtsEntry *ats;

    if (activated > 0)
        activated --;

    if (activated == 0)
        LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
            deactivate_given_ts(ats->ats);

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
