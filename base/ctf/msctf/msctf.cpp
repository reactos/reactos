/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     MSCTF Server DLL
 * COPYRIGHT:   Copyright 2008 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

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

TfClientId g_processId = 0;
ITfCompartmentMgr *g_globalCompartmentMgr = NULL;

typedef HRESULT (*LPFNCONSTRUCTOR)(IUnknown *pUnkOuter, IUnknown **ppvOut);

static const struct {
    const CLSID *clsid;
    LPFNCONSTRUCTOR ctor;
} ClassesTable[] = {
    {&CLSID_TF_ThreadMgr, ThreadMgr_Constructor},
    {&CLSID_TF_InputProcessorProfiles, InputProcessorProfiles_Constructor},
    {&CLSID_TF_CategoryMgr, CategoryMgr_Constructor},
    {&CLSID_TF_LangBarMgr, LangBarMgr_Constructor},
    {&CLSID_TF_DisplayAttributeMgr, DisplayAttributeMgr_Constructor},
    {NULL, NULL}
};

class CClassFactory
    : public IClassFactory
{
public:
    CClassFactory(LPFNCONSTRUCTOR ctor);
    virtual ~CClassFactory();

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IClassFactory methods **
    STDMETHODIMP CreateInstance(
        _In_ IUnknown *pUnkOuter,
        _In_ REFIID riid,
        _Out_ void **ppvObject) override;
    STDMETHODIMP LockServer(_In_ BOOL fLock) override;

protected:
    LONG m_cRefs;
    LPFNCONSTRUCTOR m_ctor;
};

CClassFactory::CClassFactory(LPFNCONSTRUCTOR ctor)
    : m_cRefs(1)
    , m_ctor(ctor)
{
}

CClassFactory::~CClassFactory()
{
    TRACE("Destroying class factory %p\n", this);
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;
    if (riid == IID_IClassFactory || riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = static_cast<IClassFactory *>(this);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    ULONG ret = InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CClassFactory::CreateInstance(
    _In_ IUnknown *pUnkOuter,
    _In_ REFIID riid,
    _Out_ void **ppvObject)
{
    TRACE("(%p, %p, %s, %p)\n", this, pUnkOuter, debugstr_guid(&riid), ppvObject);

    IUnknown *obj;
    HRESULT ret = m_ctor(pUnkOuter, &obj);
    if (FAILED(ret))
        return ret;
    ret = obj->QueryInterface(riid, ppvObject);
    obj->Release();
    return ret;
}

STDMETHODIMP CClassFactory::LockServer(_In_ BOOL fLock)
{
    TRACE("(%p)->(%x)\n", this, fLock);
    return S_OK;
}

static HRESULT ClassFactory_Constructor(LPFNCONSTRUCTOR ctor, LPVOID *ppvOut)
{
    CClassFactory *This = new(cicNoThrow) CClassFactory(ctor);
    *ppvOut = static_cast<IClassFactory *>(This);
    TRACE("Created class factory %p\n", This);
    return S_OK;
}

/*************************************************************************
 * DWORD Cookie Management
 */
EXTERN_C
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
            cookies = (CookieInternal *)cicMemAllocClear(10 * sizeof(CookieInternal));
            if (!cookies)
            {
                ERR("Out of memory, Unable to alloc cookies array\n");
                return 0;
            }
            array_size = 10;
        }
        else
        {
            ERR("cookies: %p, array_size: %d\n", cookies, array_size);
            CookieInternal *new_cookies = (CookieInternal *)
                cicMemReCalloc(cookies, array_size * 2, sizeof(CookieInternal));
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

EXTERN_C
DWORD get_Cookie_magic(DWORD id)
{
    UINT index = id - 1;

    if (index >= id_last)
        return 0;

    if (cookies[index].id == 0)
        return 0;

    return cookies[index].magic;
}

EXTERN_C
LPVOID get_Cookie_data(DWORD id)
{
    UINT index = id - 1;

    if (index >= id_last)
        return NULL;

    if (cookies[index].id == 0)
        return NULL;

    return cookies[index].data;
}

EXTERN_C
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

EXTERN_C
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

EXTERN_C
HRESULT advise_sink(struct list *sink_list, REFIID riid, DWORD cookie_magic, IUnknown *unk, DWORD *cookie)
{
    Sink *sink = (Sink *)cicMemAlloc(sizeof(*sink));
    if (!sink)
        return E_OUTOFMEMORY;

    HRESULT hr = unk->QueryInterface(riid, (void **)&sink->interfaces.pIUnknown);
    if (FAILED(hr))
    {
        cicMemFree(sink);
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
    sink->interfaces.pIUnknown->Release();
    cicMemFree(sink);
}

EXTERN_C
HRESULT unadvise_sink(DWORD cookie)
{
    Sink *sink = (Sink *)remove_Cookie(cookie);
    if (!sink)
        return CONNECT_E_NOCONNECTION;

    free_sink(sink);
    return S_OK;
}

EXTERN_C
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

    hr = CoCreateInstance(actsvr->LanguageProfile.clsid, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITfTextInputProcessor, (void **)&actsvr->pITfTextInputProcessor);
    if (FAILED(hr)) return hr;

    hr = actsvr->pITfTextInputProcessor->Activate((ITfThreadMgr *)tm, actsvr->tid);
    if (FAILED(hr))
    {
        actsvr->pITfTextInputProcessor->Release();
        actsvr->pITfTextInputProcessor = NULL;
        return hr;
    }

    actsvr->pITfThreadMgrEx = tm;
    tm->AddRef();
    return hr;
}

static HRESULT deactivate_given_ts(ActivatedTextService *actsvr)
{
    HRESULT hr = S_OK;

    if (actsvr->pITfTextInputProcessor)
    {
        hr = actsvr->pITfTextInputProcessor->Deactivate();
        actsvr->pITfTextInputProcessor->Release();
        actsvr->pITfThreadMgrEx->Release();
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
        if (catid == ats->ats->LanguageProfile.catid)
        {
            deactivate_given_ts(ats->ats);
            list_remove(&ats->entry);
            cicMemFree(ats->ats);
            cicMemFree(ats);
            /* we are guaranteeing there is only 1 */
            break;
        }
    }
}

EXTERN_C
HRESULT add_active_textservice(TF_LANGUAGEPROFILE *lp)
{
    ActivatedTextService *actsvr;
    ITfCategoryMgr *catmgr;
    AtsEntry *entry;
    ITfThreadMgrEx *tm = (ITfThreadMgrEx *)TlsGetValue(g_dwTLSIndex);
    ITfClientId *clientid;

    if (!tm)
        return E_UNEXPECTED;

    actsvr = (ActivatedTextService *)cicMemAlloc(sizeof(ActivatedTextService));
    if (!actsvr)
        return E_OUTOFMEMORY;

    tm->QueryInterface(IID_ITfClientId, (void **)&clientid);
    clientid->GetClientId(lp->clsid, &actsvr->tid);
    clientid->Release();

    if (!actsvr->tid)
    {
        cicMemFree(actsvr);
        return E_OUTOFMEMORY;
    }

    actsvr->pITfTextInputProcessor = NULL;
    actsvr->LanguageProfile = *lp;
    actsvr->pITfKeyEventSink = NULL;

    /* get TIP category */
    if (SUCCEEDED(CategoryMgr_Constructor(NULL, (IUnknown**)&catmgr)))
    {
        static const GUID *list[3] = {&GUID_TFCAT_TIP_SPEECH, &GUID_TFCAT_TIP_KEYBOARD, &GUID_TFCAT_TIP_HANDWRITING};

        catmgr->FindClosestCategory(actsvr->LanguageProfile.clsid, &actsvr->LanguageProfile.catid, list, 3);
        catmgr->Release();
    }
    else
    {
        ERR("CategoryMgr construction failed\n");
        actsvr->LanguageProfile.catid = GUID_NULL;
    }

    if (actsvr->LanguageProfile.catid != GUID_NULL)
        deactivate_remove_conflicting_ts(actsvr->LanguageProfile.catid);

    if (activated > 0)
        activate_given_ts(actsvr, tm);

    entry = (AtsEntry *)cicMemAlloc(sizeof(AtsEntry));
    if (!entry)
    {
        cicMemFree(actsvr);
        return E_OUTOFMEMORY;
    }

    entry->ats = actsvr;
    list_add_head(&AtsList, &entry->entry);

    return S_OK;
}

EXTERN_C
BOOL get_active_textservice(REFCLSID rclsid, TF_LANGUAGEPROFILE *profile)
{
    AtsEntry *ats;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
    {
        if (rclsid == ats->ats->LanguageProfile.clsid)
        {
            if (profile)
                *profile = ats->ats->LanguageProfile;
            return TRUE;
        }
    }
    return FALSE;
}

EXTERN_C
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

EXTERN_C
HRESULT deactivate_textservices(void)
{
    AtsEntry *ats;

    if (activated > 0)
        activated --;

    if (activated == 0)
    {
        LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
            deactivate_given_ts(ats->ats);
    }
    return S_OK;
}

EXTERN_C
CLSID get_textservice_clsid(TfClientId tid)
{
    AtsEntry *ats;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
    {
        if (ats->ats->tid == tid)
            return ats->ats->LanguageProfile.clsid;
    }
    return GUID_NULL;
}

EXTERN_C
HRESULT get_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown **sink)
{
    AtsEntry *ats;

    if (iid != IID_ITfKeyEventSink)
        return E_NOINTERFACE;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
    {
        if (ats->ats->tid == tid)
        {
            *sink = (IUnknown*)ats->ats->pITfKeyEventSink;
            return S_OK;
        }
    }

    return E_FAIL;
}

EXTERN_C
HRESULT set_textservice_sink(TfClientId tid, REFCLSID iid, IUnknown* sink)
{
    AtsEntry *ats;

    if (iid != IID_ITfKeyEventSink)
        return E_NOINTERFACE;

    LIST_FOR_EACH_ENTRY(ats, &AtsList, AtsEntry, entry)
    {
        if (ats->ats->tid == tid)
        {
            ats->ats->pITfKeyEventSink = (ITfKeyEventSink*)sink;
            return S_OK;
        }
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
        case DLL_PROCESS_ATTACH:
            MSCTF_hinstance = hinst;
            return ProcessAttach(hinst);

        case DLL_PROCESS_DETACH:
            ProcessDetach(hinst);
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
    if (iid != IID_IUnknown && iid != IID_IClassFactory)
        return E_NOINTERFACE;

    for (i = 0; ClassesTable[i].clsid; i++)
    {
        if (*ClassesTable[i].clsid == clsid)
            return ClassFactory_Constructor(ClassesTable[i].ctor, ppvOut);
    }
    FIXME("CLSID %s not supported\n", debugstr_guid(&clsid));
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
    return ThreadMgr_Constructor(NULL, (IUnknown**)pptim);
}

/***********************************************************************
 *              TF_GetThreadMgr (MSCTF.@)
 */
HRESULT WINAPI TF_GetThreadMgr(ITfThreadMgr **pptim)
{
    TRACE("\n");
    *pptim = (ITfThreadMgr *)TlsGetValue(g_dwTLSIndex);

    if (*pptim)
        (*pptim)->AddRef();

    return S_OK;
}

/***********************************************************************
 *              SetInputScope(MSCTF.@)
 */
HRESULT WINAPI SetInputScope(HWND hwnd, InputScope inputscope)
{
    FIXME("STUB: %p %i\n", hwnd, inputscope);
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
    FIXME("STUB: %p ... %s %s\n", hwnd, debugstr_w(pszRegExp), debugstr_w(pszSRGS));
    for (i = 0; i < cInputScopes; i++)
        TRACE("\tScope[%u] = %i\n", i, pInputScopes[i]);
    for (i = 0; i < cPhrases; i++)
        TRACE("\tPhrase[%u] = %s\n", i, debugstr_w(ppszPhraseList[i]));

    return S_OK;
}

/***********************************************************************
 *              TF_CreateInputProcessorProfiles(MSCTF.@)
 */
HRESULT WINAPI TF_CreateInputProcessorProfiles(
                        ITfInputProcessorProfiles **ppipr)
{
    return InputProcessorProfiles_Constructor(NULL, (IUnknown**)ppipr);
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
    return LangBarMgr_Constructor(NULL, (IUnknown**)pppbm);
}

/***********************************************************************
 *              TF_CreateLangBarItemMgr (MSCTF.@)
 */
HRESULT WINAPI TF_CreateLangBarItemMgr(ITfLangBarItemMgr **pplbim)
{
    FIXME("stub %p\n", pplbim);
    *pplbim = NULL;

    return E_NOTIMPL;
}
