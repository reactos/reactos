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

#include "msctf_internal.h"

#include <rpcproxy.h>
#include <inputscope.h>

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
    ITfThreadMgrEx          *pITfThreadMgrEx;
    ITfKeyEventSink         *pITfKeyEventSink;
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
ITfCompartmentMgr *globalCompartmentMgr = NULL;

const WCHAR szwSystemTIPKey[] = {'S','O','F','T','W','A','R','E','\\','M','i','c','r','o','s','o','f','t','\\','C','T','F','\\','T','I','P',0};
const WCHAR szwSystemCTFKey[] = {'S','O','F','T','W','A','R','E','\\','M','i','c','r','o','s','o','f','t','\\','C','T','F',0};

typedef HRESULT (*LPFNCONSTRUCTOR)(IUnknown *pUnkOuter, IUnknown **ppvOut);

static const struct {
    REFCLSID clsid;
    LPFNCONSTRUCTOR ctor;
} ClassesTable[] = {
    {&CLSID_TF_ThreadMgr, ThreadMgr_Constructor},
    {&CLSID_TF_InputProcessorProfiles, InputProcessorProfiles_Constructor},
    {&CLSID_TF_CategoryMgr, CategoryMgr_Constructor},
    {&CLSID_TF_LangBarMgr, LangBarMgr_Constructor},
    {&CLSID_TF_DisplayAttributeMgr, DisplayAttributeMgr_Constructor},
    {NULL, NULL}
};

typedef struct tagClassFactory
{
    IClassFactory IClassFactory_iface;
    LONG   ref;
    LPFNCONSTRUCTOR ctor;
} ClassFactory;

static inline ClassFactory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ClassFactory, IClassFactory_iface);
}

static void ClassFactory_Destructor(ClassFactory *This)
{
    TRACE("Destroying class factory %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
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
    ClassFactory *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    ClassFactory *This = impl_from_IClassFactory(iface);
    ULONG ret = InterlockedDecrement(&This->ref);

    if (ret == 0)
        ClassFactory_Destructor(This);
    return ret;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *punkOuter, REFIID iid, LPVOID *ppvOut)
{
    ClassFactory *This = impl_from_IClassFactory(iface);
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
    ClassFactory *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%x)\n", This, fLock);

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
    This->IClassFactory_iface.lpVtbl = &ClassFactoryVtbl;
    This->ref = 1;
    This->ctor = ctor;
    *ppvOut = &This->IClassFactory_iface;
    TRACE("Created class factory %p\n", This);
    return S_OK;
}

/*************************************************************************
 * DWORD Cookie Management
 */
DWORD generate_Cookie(DWORD magic, LPVOID data)
{
    UINT i;

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
    unsigned int i;
    for (i = *index; i < id_last; i++)
        if (cookies[i].id != 0 && cookies[i].magic == magic)
        {
            *index = (i+1);
            return cookies[i].id;
        }
    return 0x0;
}

HRESULT advise_sink(struct list *sink_list, REFIID riid, DWORD cookie_magic, IUnknown *unk, DWORD *cookie)
{
    Sink *sink;

    sink = HeapAlloc(GetProcessHeap(), 0, sizeof(*sink));
    if (!sink)
        return E_OUTOFMEMORY;

    if (FAILED(IUnknown_QueryInterface(unk, riid, (void**)&sink->interfaces.pIUnknown)))
    {
        HeapFree(GetProcessHeap(), 0, sink);
        return CONNECT_E_CANNOTCONNECT;
    }

    list_add_head(sink_list, &sink->entry);
    *cookie = generate_Cookie(cookie_magic, sink);
    TRACE("cookie %x\n", *cookie);
    return S_OK;
}

static void free_sink(Sink *sink)
{
    list_remove(&sink->entry);
    IUnknown_Release(sink->interfaces.pIUnknown);
    HeapFree(GetProcessHeap(), 0, sink);
}

HRESULT unadvise_sink(DWORD cookie)
{
    Sink *sink;

    sink = remove_Cookie(cookie);
    if (!sink)
        return CONNECT_E_NOCONNECTION;

    free_sink(sink);
    return S_OK;
}

void free_sinks(struct list *sink_list)
{
    while(!list_empty(sink_list))
    {
        Sink* sink = LIST_ENTRY(sink_list->next, Sink, entry);
        free_sink(sink);
    }
}

/*****************************************************************************
 * Active Text Service Management
 *****************************************************************************/
static HRESULT activate_given_ts(ActivatedTextService *actsvr, ITfThreadMgrEx *tm)
{
    HRESULT hr;

    /* Already Active? */
    if (actsvr->pITfTextInputProcessor)
        return S_OK;

    hr = CoCreateInstance (&actsvr->LanguageProfile.clsid, NULL, CLSCTX_INPROC_SERVER,
        &IID_ITfTextInputProcessor, (void**)&actsvr->pITfTextInputProcessor);
    if (FAILED(hr)) return hr;

    hr = ITfTextInputProcessor_Activate(actsvr->pITfTextInputProcessor, (ITfThreadMgr *)tm, actsvr->tid);
    if (FAILED(hr))
    {
        ITfTextInputProcessor_Release(actsvr->pITfTextInputProcessor);
        actsvr->pITfTextInputProcessor = NULL;
        return hr;
    }

    actsvr->pITfThreadMgrEx = tm;
    ITfThreadMgrEx_AddRef(tm);
    return hr;
}

static HRESULT deactivate_given_ts(ActivatedTextService *actsvr)
{
    HRESULT hr = S_OK;

    if (actsvr->pITfTextInputProcessor)
    {
        hr = ITfTextInputProcessor_Deactivate(actsvr->pITfTextInputProcessor);
        ITfTextInputProcessor_Release(actsvr->pITfTextInputProcessor);
        ITfThreadMgrEx_Release(actsvr->pITfThreadMgrEx);
        actsvr->pITfTextInputProcessor = NULL;
        actsvr->pITfThreadMgrEx = NULL;
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
            /* we are guaranteeing there is only 1 */
            break;
        }
    }
}

HRESULT add_active_textservice(TF_LANGUAGEPROFILE *lp)
{
    ActivatedTextService *actsvr;
    ITfCategoryMgr *catmgr;
    AtsEntry *entry;
    ITfThreadMgrEx *tm = TlsGetValue(tlsIndex);
    ITfClientId *clientid;

    if (!tm) return E_UNEXPECTED;

    actsvr = HeapAlloc(GetProcessHeap(),0,sizeof(ActivatedTextService));
    if (!actsvr) return E_OUTOFMEMORY;

    ITfThreadMgrEx_QueryInterface(tm, &IID_ITfClientId, (void **)&clientid);
    ITfClientId_GetClientId(clientid, &lp->clsid, &actsvr->tid);
    ITfClientId_Release(clientid);

    if (!actsvr->tid)
    {
        HeapFree(GetProcessHeap(),0,actsvr);
        return E_OUTOFMEMORY;
    }

    actsvr->pITfTextInputProcessor = NULL;
    actsvr->LanguageProfile = *lp;
    actsvr->pITfKeyEventSink = NULL;

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

    entry = HeapAlloc(GetProcessHeap(),0,sizeof(AtsEntry));

    if (!entry)
    {
        HeapFree(GetProcessHeap(),0,actsvr);
        return E_OUTOFMEMORY;
    }

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

HRESULT activate_textservices(ITfThreadMgrEx *tm)
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

CLSID get_textservice_clsid(TfClientId tid)
{
    AtsEntry *ats;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
        if (ats->ats->tid == tid)
            return ats->ats->LanguageProfile.clsid;
    return GUID_NULL;
}

HRESULT get_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown **sink)
{
    AtsEntry *ats;

    if (!IsEqualCLSID(iid,&IID_ITfKeyEventSink))
        return E_NOINTERFACE;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
        if (ats->ats->tid == tid)
        {
            *sink = (IUnknown*)ats->ats->pITfKeyEventSink;
            return S_OK;
        }

    return E_FAIL;
}

HRESULT set_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown* sink)
{
    AtsEntry *ats;

    if (!IsEqualCLSID(iid,&IID_ITfKeyEventSink))
        return E_NOINTERFACE;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
        if (ats->ats->tid == tid)
        {
            ats->ats->pITfKeyEventSink = (ITfKeyEventSink*)sink;
            return S_OK;
        }

    return E_FAIL;
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
            if (fImpLoad) break;
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
    return S_FALSE;
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
 *		DllRegisterServer (MSCTF.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( MSCTF_hinstance );
}

/***********************************************************************
 *		DllUnregisterServer (MSCTF.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( MSCTF_hinstance );
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
HRESULT WINAPI SetInputScope(HWND hwnd, InputScope inputscope)
{
    FIXME("STUB: %p %i\n",hwnd,inputscope);
    return S_OK;
}

/***********************************************************************
 *              SetInputScopes(MSCTF.@)
 */
HRESULT WINAPI SetInputScopes(HWND hwnd, const InputScope *pInputScopes,
                              UINT cInputScopes, WCHAR **ppszPhraseList,
                              UINT cPhrases, WCHAR *pszRegExp, WCHAR *pszSRGS)
{
    UINT i;
    FIXME("STUB: %p ... %s %s\n",hwnd, debugstr_w(pszRegExp), debugstr_w(pszSRGS));
    for (i = 0; i < cInputScopes; i++)
        TRACE("\tScope[%u] = %i\n",i,pInputScopes[i]);
    for (i = 0; i < cPhrases; i++)
        TRACE("\tPhrase[%u] = %s\n",i,debugstr_w(ppszPhraseList[i]));

    return S_OK;
}

/***********************************************************************
 *              TF_CreateInputProcessorProfiles(MSCTF.@)
 */
HRESULT WINAPI TF_CreateInputProcessorProfiles(
                        ITfInputProcessorProfiles **ppipr)
{
    return InputProcessorProfiles_Constructor(NULL,(IUnknown**)ppipr);
}

/***********************************************************************
 *              TF_InvalidAssemblyListCacheIfExist(MSCTF.@)
 */
HRESULT WINAPI TF_InvalidAssemblyListCacheIfExist(void)
{
    FIXME("Stub\n");
    return S_OK;
}

/***********************************************************************
 *              TF_CreateLangBarMgr (MSCTF.@)
 */
HRESULT WINAPI TF_CreateLangBarMgr(ITfLangBarMgr **pppbm)
{
    TRACE("\n");
    return LangBarMgr_Constructor(NULL,(IUnknown**)pppbm);
}

HRESULT WINAPI TF_CreateLangBarItemMgr(ITfLangBarItemMgr **pplbim)
{
    FIXME("stub %p\n", pplbim);
    *pplbim = NULL;

    return E_NOTIMPL;
}

/***********************************************************************
 *              TF_InitMlngInfo (MSCTF.@)
 */
HRESULT WINAPI TF_InitMlngInfo(void)
{
    FIXME("stub\n");
    return S_OK;
}
