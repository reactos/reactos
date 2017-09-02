/*
 *  ITfThreadMgr implementation
 *
 *  Copyright 2008 Aric Stewart, CodeWeavers
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

typedef struct tagPreservedKey
{
    struct list     entry;
    GUID            guid;
    TF_PRESERVEDKEY prekey;
    LPWSTR          description;
    TfClientId      tid;
} PreservedKey;

typedef struct tagDocumentMgrs
{
    struct list     entry;
    ITfDocumentMgr  *docmgr;
} DocumentMgrEntry;

typedef struct tagAssociatedWindow
{
    struct list     entry;
    HWND            hwnd;
    ITfDocumentMgr  *docmgr;
} AssociatedWindow;

typedef struct tagACLMulti {
    ITfThreadMgrEx ITfThreadMgrEx_iface;
    ITfSource ITfSource_iface;
    ITfKeystrokeMgr ITfKeystrokeMgr_iface;
    ITfMessagePump ITfMessagePump_iface;
    ITfClientId ITfClientId_iface;
    /* const ITfThreadMgrExVtbl *ThreadMgrExVtbl; */
    /* const ITfConfigureSystemKeystrokeFeedVtbl *ConfigureSystemKeystrokeFeedVtbl; */
    /* const ITfLangBarItemMgrVtbl *LangBarItemMgrVtbl; */
    ITfUIElementMgr ITfUIElementMgr_iface;
    ITfSourceSingle ITfSourceSingle_iface;
    LONG refCount;

    /* Aggregation */
    ITfCompartmentMgr  *CompartmentMgr;

    ITfThreadMgrEventSink ITfThreadMgrEventSink_iface; /* internal */

    ITfDocumentMgr *focus;
    LONG activationCount;

    ITfKeyEventSink *foregroundKeyEventSink;
    CLSID foregroundTextService;

    struct list CurrentPreservedKeys;
    struct list CreatedDocumentMgrs;

    struct list AssociatedFocusWindows;
    HHOOK  focusHook;

    /* kept as separate lists to reduce unnecessary iterations */
    struct list     ActiveLanguageProfileNotifySink;
    struct list     DisplayAttributeNotifySink;
    struct list     KeyTraceEventSink;
    struct list     PreservedKeyNotifySink;
    struct list     ThreadFocusSink;
    struct list     ThreadMgrEventSink;
} ThreadMgr;

typedef struct tagEnumTfDocumentMgr {
    IEnumTfDocumentMgrs IEnumTfDocumentMgrs_iface;
    LONG refCount;

    struct list *index;
    struct list *head;
} EnumTfDocumentMgr;

static HRESULT EnumTfDocumentMgr_Constructor(struct list* head, IEnumTfDocumentMgrs **ppOut);

static inline ThreadMgr *impl_from_ITfThreadMgrEx(ITfThreadMgrEx *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgr, ITfThreadMgrEx_iface);
}

static inline ThreadMgr *impl_from_ITfSource(ITfSource *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgr, ITfSource_iface);
}

static inline ThreadMgr *impl_from_ITfKeystrokeMgr(ITfKeystrokeMgr *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgr, ITfKeystrokeMgr_iface);
}

static inline ThreadMgr *impl_from_ITfMessagePump(ITfMessagePump *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgr, ITfMessagePump_iface);
}

static inline ThreadMgr *impl_from_ITfClientId(ITfClientId *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgr, ITfClientId_iface);
}

static inline ThreadMgr *impl_from_ITfThreadMgrEventSink(ITfThreadMgrEventSink *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgr, ITfThreadMgrEventSink_iface);
}

static inline ThreadMgr *impl_from_ITfUIElementMgr(ITfUIElementMgr *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgr, ITfUIElementMgr_iface);
}

static inline ThreadMgr *impl_from_ITfSourceSingle(ITfSourceSingle *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgr, ITfSourceSingle_iface);
}

static inline EnumTfDocumentMgr *impl_from_IEnumTfDocumentMgrs(IEnumTfDocumentMgrs *iface)
{
    return CONTAINING_RECORD(iface, EnumTfDocumentMgr, IEnumTfDocumentMgrs_iface);
}

static void ThreadMgr_Destructor(ThreadMgr *This)
{
    struct list *cursor, *cursor2;

    /* unhook right away */
    if (This->focusHook)
        UnhookWindowsHookEx(This->focusHook);

    TlsSetValue(tlsIndex,NULL);
    TRACE("destroying %p\n", This);
    if (This->focus)
        ITfDocumentMgr_Release(This->focus);

    free_sinks(&This->ActiveLanguageProfileNotifySink);
    free_sinks(&This->DisplayAttributeNotifySink);
    free_sinks(&This->KeyTraceEventSink);
    free_sinks(&This->PreservedKeyNotifySink);
    free_sinks(&This->ThreadFocusSink);
    free_sinks(&This->ThreadMgrEventSink);

    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->CurrentPreservedKeys)
    {
        PreservedKey* key = LIST_ENTRY(cursor,PreservedKey,entry);
        list_remove(cursor);
        HeapFree(GetProcessHeap(),0,key->description);
        HeapFree(GetProcessHeap(),0,key);
    }

    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->CreatedDocumentMgrs)
    {
        DocumentMgrEntry *mgr = LIST_ENTRY(cursor,DocumentMgrEntry,entry);
        list_remove(cursor);
        FIXME("Left Over ITfDocumentMgr.  Should we do something with it?\n");
        HeapFree(GetProcessHeap(),0,mgr);
    }

    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->AssociatedFocusWindows)
    {
        AssociatedWindow *wnd = LIST_ENTRY(cursor,AssociatedWindow,entry);
        list_remove(cursor);
        HeapFree(GetProcessHeap(),0,wnd);
    }

    CompartmentMgr_Destructor(This->CompartmentMgr);

    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI ThreadMgr_QueryInterface(ITfThreadMgrEx *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfThreadMgr)
        || IsEqualIID(iid, &IID_ITfThreadMgrEx))
    {
        *ppvOut = &This->ITfThreadMgrEx_iface;
    }
    else if (IsEqualIID(iid, &IID_ITfSource))
    {
        *ppvOut = &This->ITfSource_iface;
    }
    else if (IsEqualIID(iid, &IID_ITfKeystrokeMgr))
    {
        *ppvOut = &This->ITfKeystrokeMgr_iface;
    }
    else if (IsEqualIID(iid, &IID_ITfMessagePump))
    {
        *ppvOut = &This->ITfMessagePump_iface;
    }
    else if (IsEqualIID(iid, &IID_ITfClientId))
    {
        *ppvOut = &This->ITfClientId_iface;
    }
    else if (IsEqualIID(iid, &IID_ITfCompartmentMgr))
    {
        *ppvOut = This->CompartmentMgr;
    }
    else if (IsEqualIID(iid, &IID_ITfUIElementMgr))
    {
        *ppvOut = &This->ITfUIElementMgr_iface;
    }
    else if (IsEqualIID(iid, &IID_ITfSourceSingle))
    {
        *ppvOut = &This->ITfSourceSingle_iface;
    }

    if (*ppvOut)
    {
        ITfThreadMgrEx_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ThreadMgr_AddRef(ITfThreadMgrEx *iface)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ThreadMgr_Release(ITfThreadMgrEx *iface)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ThreadMgr_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfThreadMgr functions
 *****************************************************/

static HRESULT WINAPI ThreadMgr_Activate(ITfThreadMgrEx *iface, TfClientId *id)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);

    TRACE("(%p) %p\n", This, id);
    return ITfThreadMgrEx_ActivateEx(iface, id, 0);
}

static HRESULT WINAPI ThreadMgr_Deactivate(ITfThreadMgrEx *iface)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    TRACE("(%p)\n",This);

    if (This->activationCount == 0)
        return E_UNEXPECTED;

    This->activationCount --;

    if (This->activationCount == 0)
    {
        if (This->focus)
        {
            ITfThreadMgrEventSink_OnSetFocus(&This->ITfThreadMgrEventSink_iface, 0, This->focus);
            ITfDocumentMgr_Release(This->focus);
            This->focus = 0;
        }
    }

    deactivate_textservices();

    return S_OK;
}

static HRESULT WINAPI ThreadMgr_CreateDocumentMgr(ITfThreadMgrEx *iface, ITfDocumentMgr **ppdim)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    DocumentMgrEntry *mgrentry;
    HRESULT hr;

    TRACE("(%p)\n",iface);
    mgrentry = HeapAlloc(GetProcessHeap(),0,sizeof(DocumentMgrEntry));
    if (mgrentry == NULL)
        return E_OUTOFMEMORY;

    hr = DocumentMgr_Constructor(&This->ITfThreadMgrEventSink_iface, ppdim);

    if (SUCCEEDED(hr))
    {
        mgrentry->docmgr = *ppdim;
        list_add_head(&This->CreatedDocumentMgrs,&mgrentry->entry);
    }
    else
        HeapFree(GetProcessHeap(),0,mgrentry);

    return hr;
}

static HRESULT WINAPI ThreadMgr_EnumDocumentMgrs(ITfThreadMgrEx *iface, IEnumTfDocumentMgrs **ppEnum)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    TRACE("(%p) %p\n",This,ppEnum);

    if (!ppEnum)
        return E_INVALIDARG;

    return EnumTfDocumentMgr_Constructor(&This->CreatedDocumentMgrs, ppEnum);
}

static HRESULT WINAPI ThreadMgr_GetFocus(ITfThreadMgrEx *iface, ITfDocumentMgr **ppdimFocus)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    TRACE("(%p)\n",This);

    if (!ppdimFocus)
        return E_INVALIDARG;

    *ppdimFocus = This->focus;

    TRACE("->%p\n",This->focus);

    if (This->focus == NULL)
        return S_FALSE;

    ITfDocumentMgr_AddRef(This->focus);

    return S_OK;
}

static HRESULT WINAPI ThreadMgr_SetFocus(ITfThreadMgrEx *iface, ITfDocumentMgr *pdimFocus)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    ITfDocumentMgr *check;

    TRACE("(%p) %p\n",This,pdimFocus);

    if (!pdimFocus)
        check = NULL;
    else if (FAILED(ITfDocumentMgr_QueryInterface(pdimFocus,&IID_ITfDocumentMgr,(LPVOID*) &check)))
        return E_INVALIDARG;

    ITfThreadMgrEventSink_OnSetFocus(&This->ITfThreadMgrEventSink_iface, check, This->focus);

    if (This->focus)
        ITfDocumentMgr_Release(This->focus);

    This->focus = check;
    return S_OK;
}

static LRESULT CALLBACK ThreadFocusHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    ThreadMgr *This;

    This = TlsGetValue(tlsIndex);
    if (!This)
    {
        ERR("Hook proc but no ThreadMgr for this thread. Serious Error\n");
        return 0;
    }
    if (!This->focusHook)
    {
        ERR("Hook proc but no ThreadMgr focus Hook. Serious Error\n");
        return 0;
    }

    if (nCode == HCBT_SETFOCUS) /* focus change within our thread */
    {
        struct list *cursor;

        LIST_FOR_EACH(cursor, &This->AssociatedFocusWindows)
        {
            AssociatedWindow *wnd = LIST_ENTRY(cursor,AssociatedWindow,entry);
            if (wnd->hwnd == (HWND)wParam)
            {
                TRACE("Triggering Associated window focus\n");
                if (This->focus != wnd->docmgr)
                    ThreadMgr_SetFocus(&This->ITfThreadMgrEx_iface, wnd->docmgr);
                break;
            }
        }
    }

    return CallNextHookEx(This->focusHook, nCode, wParam, lParam);
}

static HRESULT SetupWindowsHook(ThreadMgr *This)
{
    if (!This->focusHook)
    {
        This->focusHook = SetWindowsHookExW(WH_CBT, ThreadFocusHookProc, 0,
                             GetCurrentThreadId());
        if (!This->focusHook)
        {
            ERR("Unable to set focus hook\n");
            return E_FAIL;
        }
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI ThreadMgr_AssociateFocus(ITfThreadMgrEx *iface, HWND hwnd,
ITfDocumentMgr *pdimNew, ITfDocumentMgr **ppdimPrev)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    struct list *cursor, *cursor2;
    AssociatedWindow *wnd;

    TRACE("(%p) %p %p %p\n",This,hwnd,pdimNew,ppdimPrev);

    if (!ppdimPrev)
        return E_INVALIDARG;

    *ppdimPrev = NULL;

    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->AssociatedFocusWindows)
    {
        wnd = LIST_ENTRY(cursor,AssociatedWindow,entry);
        if (wnd->hwnd == hwnd)
        {
            if (wnd->docmgr)
                ITfDocumentMgr_AddRef(wnd->docmgr);
            *ppdimPrev = wnd->docmgr;
            wnd->docmgr = pdimNew;
            if (GetFocus() == hwnd)
                ThreadMgr_SetFocus(iface,pdimNew);
            return S_OK;
        }
    }

    wnd = HeapAlloc(GetProcessHeap(),0,sizeof(AssociatedWindow));
    wnd->hwnd = hwnd;
    wnd->docmgr = pdimNew;
    list_add_head(&This->AssociatedFocusWindows,&wnd->entry);

    if (GetFocus() == hwnd)
        ThreadMgr_SetFocus(iface,pdimNew);

    SetupWindowsHook(This);

    return S_OK;
}

static HRESULT WINAPI ThreadMgr_IsThreadFocus(ITfThreadMgrEx *iface, BOOL *pfThreadFocus)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    HWND focus;

    TRACE("(%p) %p\n",This,pfThreadFocus);
    focus = GetFocus();
    *pfThreadFocus = (focus == NULL);
    return S_OK;
}

static HRESULT WINAPI ThreadMgr_GetFunctionProvider(ITfThreadMgrEx *iface, REFCLSID clsid,
ITfFunctionProvider **ppFuncProv)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_EnumFunctionProviders(ITfThreadMgrEx *iface,
IEnumTfFunctionProviders **ppEnum)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_GetGlobalCompartment(ITfThreadMgrEx *iface,
ITfCompartmentMgr **ppCompMgr)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);
    HRESULT hr;
    TRACE("(%p) %p\n",This, ppCompMgr);

    if (!ppCompMgr)
        return E_INVALIDARG;

    if (!globalCompartmentMgr)
    {
        hr = CompartmentMgr_Constructor(NULL,&IID_ITfCompartmentMgr,(IUnknown**)&globalCompartmentMgr);
        if (FAILED(hr))
            return hr;
    }

    ITfCompartmentMgr_AddRef(globalCompartmentMgr);
    *ppCompMgr = globalCompartmentMgr;
    return S_OK;
}

static HRESULT WINAPI ThreadMgr_ActivateEx(ITfThreadMgrEx *iface, TfClientId *id, DWORD flags)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);

    TRACE("(%p) %p, %#x\n", This, id, flags);

    if (!id)
        return E_INVALIDARG;

    if (flags)
        FIXME("Unimplemented flags %#x\n", flags);

    if (!processId)
    {
        GUID guid;
        CoCreateGuid(&guid);
        ITfClientId_GetClientId(&This->ITfClientId_iface, &guid, &processId);
    }

    activate_textservices(iface);
    This->activationCount++;
    *id = processId;
    return S_OK;
}

static HRESULT WINAPI ThreadMgr_GetActiveFlags(ITfThreadMgrEx *iface, DWORD *flags)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx(iface);

    FIXME("STUB:(%p)\n", This);
    return E_NOTIMPL;
}

static const ITfThreadMgrExVtbl ThreadMgrExVtbl =
{
    ThreadMgr_QueryInterface,
    ThreadMgr_AddRef,
    ThreadMgr_Release,
    ThreadMgr_Activate,
    ThreadMgr_Deactivate,
    ThreadMgr_CreateDocumentMgr,
    ThreadMgr_EnumDocumentMgrs,
    ThreadMgr_GetFocus,
    ThreadMgr_SetFocus,
    ThreadMgr_AssociateFocus,
    ThreadMgr_IsThreadFocus,
    ThreadMgr_GetFunctionProvider,
    ThreadMgr_EnumFunctionProviders,
    ThreadMgr_GetGlobalCompartment,

    ThreadMgr_ActivateEx,
    ThreadMgr_GetActiveFlags
};

static HRESULT WINAPI Source_QueryInterface(ITfSource *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfSource(iface);
    return ITfThreadMgrEx_QueryInterface(&This->ITfThreadMgrEx_iface, iid, ppvOut);
}

static ULONG WINAPI Source_AddRef(ITfSource *iface)
{
    ThreadMgr *This = impl_from_ITfSource(iface);
    return ITfThreadMgrEx_AddRef(&This->ITfThreadMgrEx_iface);
}

static ULONG WINAPI Source_Release(ITfSource *iface)
{
    ThreadMgr *This = impl_from_ITfSource(iface);
    return ITfThreadMgrEx_Release(&This->ITfThreadMgrEx_iface);
}

/*****************************************************
 * ITfSource functions
 *****************************************************/
static HRESULT WINAPI ThreadMgrSource_AdviseSink(ITfSource *iface,
        REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    ThreadMgr *This = impl_from_ITfSource(iface);

    TRACE("(%p) %s %p %p\n",This,debugstr_guid(riid),punk,pdwCookie);

    if (!riid || !punk || !pdwCookie)
        return E_INVALIDARG;

    if (IsEqualIID(riid, &IID_ITfThreadMgrEventSink))
        return advise_sink(&This->ThreadMgrEventSink, &IID_ITfThreadMgrEventSink, COOKIE_MAGIC_TMSINK, punk, pdwCookie);

    if (IsEqualIID(riid, &IID_ITfThreadFocusSink))
    {
        WARN("semi-stub for ITfThreadFocusSink: sink won't be used.\n");
        return advise_sink(&This->ThreadFocusSink, &IID_ITfThreadFocusSink, COOKIE_MAGIC_THREADFOCUSSINK, punk, pdwCookie);
    }

    FIXME("(%p) Unhandled Sink: %s\n",This,debugstr_guid(riid));
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgrSource_UnadviseSink(ITfSource *iface, DWORD pdwCookie)
{
    ThreadMgr *This = impl_from_ITfSource(iface);

    TRACE("(%p) %x\n",This,pdwCookie);

    if (get_Cookie_magic(pdwCookie) != COOKIE_MAGIC_TMSINK && get_Cookie_magic(pdwCookie) != COOKIE_MAGIC_THREADFOCUSSINK)
        return E_INVALIDARG;

    return unadvise_sink(pdwCookie);
}

static const ITfSourceVtbl ThreadMgrSourceVtbl =
{
    Source_QueryInterface,
    Source_AddRef,
    Source_Release,
    ThreadMgrSource_AdviseSink,
    ThreadMgrSource_UnadviseSink,
};

/*****************************************************
 * ITfKeystrokeMgr functions
 *****************************************************/

static HRESULT WINAPI KeystrokeMgr_QueryInterface(ITfKeystrokeMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    return ITfThreadMgrEx_QueryInterface(&This->ITfThreadMgrEx_iface, iid, ppvOut);
}

static ULONG WINAPI KeystrokeMgr_AddRef(ITfKeystrokeMgr *iface)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    return ITfThreadMgrEx_AddRef(&This->ITfThreadMgrEx_iface);
}

static ULONG WINAPI KeystrokeMgr_Release(ITfKeystrokeMgr *iface)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    return ITfThreadMgrEx_Release(&This->ITfThreadMgrEx_iface);
}

static HRESULT WINAPI KeystrokeMgr_AdviseKeyEventSink(ITfKeystrokeMgr *iface,
        TfClientId tid, ITfKeyEventSink *pSink, BOOL fForeground)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    CLSID textservice;
    ITfKeyEventSink *check = NULL;

    TRACE("(%p) %x %p %i\n",This,tid,pSink,fForeground);

    if (!tid || !pSink)
        return E_INVALIDARG;

    textservice = get_textservice_clsid(tid);
    if (IsEqualCLSID(&GUID_NULL,&textservice))
        return E_INVALIDARG;

    get_textservice_sink(tid, &IID_ITfKeyEventSink, (IUnknown**)&check);
    if (check != NULL)
        return CONNECT_E_ADVISELIMIT;

    if (FAILED(ITfKeyEventSink_QueryInterface(pSink,&IID_ITfKeyEventSink,(LPVOID*) &check)))
        return E_INVALIDARG;

    set_textservice_sink(tid, &IID_ITfKeyEventSink, (IUnknown*)check);

    if (fForeground)
    {
        if (This->foregroundKeyEventSink)
        {
            ITfKeyEventSink_OnSetFocus(This->foregroundKeyEventSink, FALSE);
            ITfKeyEventSink_Release(This->foregroundKeyEventSink);
        }
        ITfKeyEventSink_AddRef(check);
        ITfKeyEventSink_OnSetFocus(check, TRUE);
        This->foregroundKeyEventSink = check;
        This->foregroundTextService = textservice;
    }
    return S_OK;
}

static HRESULT WINAPI KeystrokeMgr_UnadviseKeyEventSink(ITfKeystrokeMgr *iface,
        TfClientId tid)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    CLSID textservice;
    ITfKeyEventSink *check = NULL;
    TRACE("(%p) %x\n",This,tid);

    if (!tid)
        return E_INVALIDARG;

    textservice = get_textservice_clsid(tid);
    if (IsEqualCLSID(&GUID_NULL,&textservice))
        return E_INVALIDARG;

    get_textservice_sink(tid, &IID_ITfKeyEventSink, (IUnknown**)&check);

    if (!check)
        return CONNECT_E_NOCONNECTION;

    set_textservice_sink(tid, &IID_ITfKeyEventSink, NULL);
    ITfKeyEventSink_Release(check);

    if (This->foregroundKeyEventSink == check)
    {
        ITfKeyEventSink_Release(This->foregroundKeyEventSink);
        This->foregroundKeyEventSink = NULL;
        This->foregroundTextService = GUID_NULL;
    }
    return S_OK;
}

static HRESULT WINAPI KeystrokeMgr_GetForeground(ITfKeystrokeMgr *iface,
        CLSID *pclsid)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    TRACE("(%p) %p\n",This,pclsid);
    if (!pclsid)
        return E_INVALIDARG;

    if (IsEqualCLSID(&This->foregroundTextService,&GUID_NULL))
        return S_FALSE;

    *pclsid = This->foregroundTextService;
    return S_OK;
}

static HRESULT WINAPI KeystrokeMgr_TestKeyDown(ITfKeystrokeMgr *iface,
        WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    FIXME("STUB:(%p)\n",This);
    *pfEaten = FALSE;
    return S_OK;
}

static HRESULT WINAPI KeystrokeMgr_TestKeyUp(ITfKeystrokeMgr *iface,
        WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    FIXME("STUB:(%p)\n",This);
    *pfEaten = FALSE;
    return S_OK;
}

static HRESULT WINAPI KeystrokeMgr_KeyDown(ITfKeystrokeMgr *iface,
        WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_KeyUp(ITfKeystrokeMgr *iface,
        WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_GetPreservedKey(ITfKeystrokeMgr *iface,
        ITfContext *pic, const TF_PRESERVEDKEY *pprekey, GUID *pguid)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_IsPreservedKey(ITfKeystrokeMgr *iface,
        REFGUID rguid, const TF_PRESERVEDKEY *pprekey, BOOL *pfRegistered)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    struct list *cursor;

    TRACE("(%p) %s (%x %x) %p\n",This,debugstr_guid(rguid), (pprekey)?pprekey->uVKey:0, (pprekey)?pprekey->uModifiers:0, pfRegistered);

    if (!rguid || !pprekey || !pfRegistered)
        return E_INVALIDARG;

    LIST_FOR_EACH(cursor, &This->CurrentPreservedKeys)
    {
        PreservedKey* key = LIST_ENTRY(cursor,PreservedKey,entry);
        if (IsEqualGUID(rguid,&key->guid) && pprekey->uVKey == key->prekey.uVKey && pprekey->uModifiers == key->prekey.uModifiers)
        {
            *pfRegistered = TRUE;
            return S_OK;
        }
    }

    *pfRegistered = FALSE;
    return S_FALSE;
}

static HRESULT WINAPI KeystrokeMgr_PreserveKey(ITfKeystrokeMgr *iface,
        TfClientId tid, REFGUID rguid, const TF_PRESERVEDKEY *prekey,
        const WCHAR *pchDesc, ULONG cchDesc)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    struct list *cursor;
    PreservedKey *newkey;

    TRACE("(%p) %x %s (%x,%x) %s\n",This,tid, debugstr_guid(rguid),(prekey)?prekey->uVKey:0,(prekey)?prekey->uModifiers:0,debugstr_wn(pchDesc,cchDesc));

    if (!tid || ! rguid || !prekey || (cchDesc && !pchDesc))
        return E_INVALIDARG;

    LIST_FOR_EACH(cursor, &This->CurrentPreservedKeys)
    {
        PreservedKey* key = LIST_ENTRY(cursor,PreservedKey,entry);
        if (IsEqualGUID(rguid,&key->guid) && prekey->uVKey == key->prekey.uVKey && prekey->uModifiers == key->prekey.uModifiers)
            return TF_E_ALREADY_EXISTS;
    }

    newkey = HeapAlloc(GetProcessHeap(),0,sizeof(PreservedKey));
    if (!newkey)
        return E_OUTOFMEMORY;

    newkey->guid  = *rguid;
    newkey->prekey = *prekey;
    newkey->tid = tid;
    newkey->description = NULL;
    if (cchDesc)
    {
        newkey->description = HeapAlloc(GetProcessHeap(),0,cchDesc * sizeof(WCHAR));
        if (!newkey->description)
        {
            HeapFree(GetProcessHeap(),0,newkey);
            return E_OUTOFMEMORY;
        }
        memcpy(newkey->description, pchDesc, cchDesc*sizeof(WCHAR));
    }

    list_add_head(&This->CurrentPreservedKeys,&newkey->entry);

    return S_OK;
}

static HRESULT WINAPI KeystrokeMgr_UnpreserveKey(ITfKeystrokeMgr *iface,
        REFGUID rguid, const TF_PRESERVEDKEY *pprekey)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    PreservedKey* key = NULL;
    struct list *cursor;
    TRACE("(%p) %s (%x %x)\n",This,debugstr_guid(rguid),(pprekey)?pprekey->uVKey:0, (pprekey)?pprekey->uModifiers:0);

    if (!pprekey || !rguid)
        return E_INVALIDARG;

    LIST_FOR_EACH(cursor, &This->CurrentPreservedKeys)
    {
        key = LIST_ENTRY(cursor,PreservedKey,entry);
        if (IsEqualGUID(rguid,&key->guid) && pprekey->uVKey == key->prekey.uVKey && pprekey->uModifiers == key->prekey.uModifiers)
            break;
        key = NULL;
    }

    if (!key)
        return CONNECT_E_NOCONNECTION;

    list_remove(&key->entry);
    HeapFree(GetProcessHeap(),0,key->description);
    HeapFree(GetProcessHeap(),0,key);

    return S_OK;
}

static HRESULT WINAPI KeystrokeMgr_SetPreservedKeyDescription(ITfKeystrokeMgr *iface,
        REFGUID rguid, const WCHAR *pchDesc, ULONG cchDesc)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_GetPreservedKeyDescription(ITfKeystrokeMgr *iface,
        REFGUID rguid, BSTR *pbstrDesc)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_SimulatePreservedKey(ITfKeystrokeMgr *iface,
        ITfContext *pic, REFGUID rguid, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfKeystrokeMgrVtbl KeystrokeMgrVtbl =
{
    KeystrokeMgr_QueryInterface,
    KeystrokeMgr_AddRef,
    KeystrokeMgr_Release,
    KeystrokeMgr_AdviseKeyEventSink,
    KeystrokeMgr_UnadviseKeyEventSink,
    KeystrokeMgr_GetForeground,
    KeystrokeMgr_TestKeyDown,
    KeystrokeMgr_TestKeyUp,
    KeystrokeMgr_KeyDown,
    KeystrokeMgr_KeyUp,
    KeystrokeMgr_GetPreservedKey,
    KeystrokeMgr_IsPreservedKey,
    KeystrokeMgr_PreserveKey,
    KeystrokeMgr_UnpreserveKey,
    KeystrokeMgr_SetPreservedKeyDescription,
    KeystrokeMgr_GetPreservedKeyDescription,
    KeystrokeMgr_SimulatePreservedKey
};

/*****************************************************
 * ITfMessagePump functions
 *****************************************************/

static HRESULT WINAPI MessagePump_QueryInterface(ITfMessagePump *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfMessagePump(iface);
    return ITfThreadMgrEx_QueryInterface(&This->ITfThreadMgrEx_iface, iid, ppvOut);
}

static ULONG WINAPI MessagePump_AddRef(ITfMessagePump *iface)
{
    ThreadMgr *This = impl_from_ITfMessagePump(iface);
    return ITfThreadMgrEx_AddRef(&This->ITfThreadMgrEx_iface);
}

static ULONG WINAPI MessagePump_Release(ITfMessagePump *iface)
{
    ThreadMgr *This = impl_from_ITfMessagePump(iface);
    return ITfThreadMgrEx_Release(&This->ITfThreadMgrEx_iface);
}

static HRESULT WINAPI MessagePump_PeekMessageA(ITfMessagePump *iface,
        LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax,
        UINT wRemoveMsg, BOOL *pfResult)
{
    if (!pfResult)
        return E_INVALIDARG;
    *pfResult = PeekMessageA(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    return S_OK;
}

static HRESULT WINAPI MessagePump_GetMessageA(ITfMessagePump *iface,
        LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax,
        BOOL *pfResult)
{
    if (!pfResult)
        return E_INVALIDARG;
    *pfResult = GetMessageA(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax);
    return S_OK;
}

static HRESULT WINAPI MessagePump_PeekMessageW(ITfMessagePump *iface,
        LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax,
        UINT wRemoveMsg, BOOL *pfResult)
{
    if (!pfResult)
        return E_INVALIDARG;
    *pfResult = PeekMessageW(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    return S_OK;
}

static HRESULT WINAPI MessagePump_GetMessageW(ITfMessagePump *iface,
        LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin, UINT wMsgFilterMax,
        BOOL *pfResult)
{
    if (!pfResult)
        return E_INVALIDARG;
    *pfResult = GetMessageW(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax);
    return S_OK;
}

static const ITfMessagePumpVtbl MessagePumpVtbl =
{
    MessagePump_QueryInterface,
    MessagePump_AddRef,
    MessagePump_Release,
    MessagePump_PeekMessageA,
    MessagePump_GetMessageA,
    MessagePump_PeekMessageW,
    MessagePump_GetMessageW
};

/*****************************************************
 * ITfClientId functions
 *****************************************************/

static HRESULT WINAPI ClientId_QueryInterface(ITfClientId *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfClientId(iface);
    return ITfThreadMgrEx_QueryInterface(&This->ITfThreadMgrEx_iface, iid, ppvOut);
}

static ULONG WINAPI ClientId_AddRef(ITfClientId *iface)
{
    ThreadMgr *This = impl_from_ITfClientId(iface);
    return ITfThreadMgrEx_AddRef(&This->ITfThreadMgrEx_iface);
}

static ULONG WINAPI ClientId_Release(ITfClientId *iface)
{
    ThreadMgr *This = impl_from_ITfClientId(iface);
    return ITfThreadMgrEx_Release(&This->ITfThreadMgrEx_iface);
}

static HRESULT WINAPI ClientId_GetClientId(ITfClientId *iface,
    REFCLSID rclsid, TfClientId *ptid)

{
    ThreadMgr *This = impl_from_ITfClientId(iface);
    HRESULT hr;
    ITfCategoryMgr *catmgr;

    TRACE("(%p) %s\n",This,debugstr_guid(rclsid));

    CategoryMgr_Constructor(NULL,(IUnknown**)&catmgr);
    hr = ITfCategoryMgr_RegisterGUID(catmgr,rclsid,ptid);
    ITfCategoryMgr_Release(catmgr);

    return hr;
}

static const ITfClientIdVtbl ClientIdVtbl =
{
    ClientId_QueryInterface,
    ClientId_AddRef,
    ClientId_Release,
    ClientId_GetClientId
};

/*****************************************************
 * ITfThreadMgrEventSink functions  (internal)
 *****************************************************/
static HRESULT WINAPI ThreadMgrEventSink_QueryInterface(ITfThreadMgrEventSink *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    return ITfThreadMgrEx_QueryInterface(&This->ITfThreadMgrEx_iface, iid, ppvOut);
}

static ULONG WINAPI ThreadMgrEventSink_AddRef(ITfThreadMgrEventSink *iface)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    return ITfThreadMgrEx_AddRef(&This->ITfThreadMgrEx_iface);
}

static ULONG WINAPI ThreadMgrEventSink_Release(ITfThreadMgrEventSink *iface)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    return ITfThreadMgrEx_Release(&This->ITfThreadMgrEx_iface);
}


static HRESULT WINAPI ThreadMgrEventSink_OnInitDocumentMgr(
        ITfThreadMgrEventSink *iface,ITfDocumentMgr *pdim)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p\n",This,pdim);

    SINK_FOR_EACH(cursor, &This->ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        ITfThreadMgrEventSink_OnInitDocumentMgr(sink, pdim);
    }

    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnUninitDocumentMgr(
        ITfThreadMgrEventSink *iface, ITfDocumentMgr *pdim)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p\n",This,pdim);

    SINK_FOR_EACH(cursor, &This->ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        ITfThreadMgrEventSink_OnUninitDocumentMgr(sink, pdim);
    }

    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnSetFocus(
        ITfThreadMgrEventSink *iface, ITfDocumentMgr *pdimFocus,
        ITfDocumentMgr *pdimPrevFocus)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p %p\n",This,pdimFocus, pdimPrevFocus);

    SINK_FOR_EACH(cursor, &This->ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        ITfThreadMgrEventSink_OnSetFocus(sink, pdimFocus, pdimPrevFocus);
    }

    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnPushContext(
        ITfThreadMgrEventSink *iface, ITfContext *pic)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p\n",This,pic);

    SINK_FOR_EACH(cursor, &This->ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        ITfThreadMgrEventSink_OnPushContext(sink, pic);
    }

    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnPopContext(
        ITfThreadMgrEventSink *iface, ITfContext *pic)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    ITfThreadMgrEventSink *sink;
    struct list *cursor;

    TRACE("(%p) %p\n",This,pic);

    SINK_FOR_EACH(cursor, &This->ThreadMgrEventSink, ITfThreadMgrEventSink, sink)
    {
        ITfThreadMgrEventSink_OnPopContext(sink, pic);
    }

    return S_OK;
}

static const ITfThreadMgrEventSinkVtbl ThreadMgrEventSinkVtbl =
{
    ThreadMgrEventSink_QueryInterface,
    ThreadMgrEventSink_AddRef,
    ThreadMgrEventSink_Release,
    ThreadMgrEventSink_OnInitDocumentMgr,
    ThreadMgrEventSink_OnUninitDocumentMgr,
    ThreadMgrEventSink_OnSetFocus,
    ThreadMgrEventSink_OnPushContext,
    ThreadMgrEventSink_OnPopContext
};

/*****************************************************
 * ITfUIElementMgr functions
 *****************************************************/
static HRESULT WINAPI UIElementMgr_QueryInterface(ITfUIElementMgr *iface, REFIID iid, void **ppvOut)
{
    ThreadMgr *This = impl_from_ITfUIElementMgr(iface);

    return ITfThreadMgrEx_QueryInterface(&This->ITfThreadMgrEx_iface, iid, ppvOut);
}

static ULONG WINAPI UIElementMgr_AddRef(ITfUIElementMgr *iface)
{
    ThreadMgr *This = impl_from_ITfUIElementMgr(iface);

    return ITfThreadMgrEx_AddRef(&This->ITfThreadMgrEx_iface);
}

static ULONG WINAPI UIElementMgr_Release(ITfUIElementMgr *iface)
{
    ThreadMgr *This = impl_from_ITfUIElementMgr(iface);

    return ITfThreadMgrEx_Release(&This->ITfThreadMgrEx_iface);
}

static HRESULT WINAPI UIElementMgr_BeginUIElement(ITfUIElementMgr *iface, ITfUIElement *element,
                                                  BOOL *show, DWORD *id)
{
    ThreadMgr *This = impl_from_ITfUIElementMgr(iface);

    FIXME("STUB:(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI UIElementMgr_UpdateUIElement(ITfUIElementMgr *iface, DWORD id)
{
    ThreadMgr *This = impl_from_ITfUIElementMgr(iface);

    FIXME("STUB:(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI UIElementMgr_EndUIElement(ITfUIElementMgr *iface, DWORD id)
{
    ThreadMgr *This = impl_from_ITfUIElementMgr(iface);

    FIXME("STUB:(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI UIElementMgr_GetUIElement(ITfUIElementMgr *iface, DWORD id,
                                                ITfUIElement **element)
{
    ThreadMgr *This = impl_from_ITfUIElementMgr(iface);

    FIXME("STUB:(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI UIElementMgr_EnumUIElements(ITfUIElementMgr *iface,
                                                  IEnumTfUIElements **enum_elements)
{
    ThreadMgr *This = impl_from_ITfUIElementMgr(iface);

    FIXME("STUB:(%p)\n", This);
    return E_NOTIMPL;
}

static const ITfUIElementMgrVtbl ThreadMgrUIElementMgrVtbl =
{
    UIElementMgr_QueryInterface,
    UIElementMgr_AddRef,
    UIElementMgr_Release,

    UIElementMgr_BeginUIElement,
    UIElementMgr_UpdateUIElement,
    UIElementMgr_EndUIElement,
    UIElementMgr_GetUIElement,
    UIElementMgr_EnumUIElements
};

/*****************************************************
 * ITfSourceSingle functions
 *****************************************************/
static HRESULT WINAPI ThreadMgrSourceSingle_QueryInterface(ITfSourceSingle *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfSourceSingle(iface);
    return ITfThreadMgrEx_QueryInterface(&This->ITfThreadMgrEx_iface, iid, ppvOut);
}

static ULONG WINAPI ThreadMgrSourceSingle_AddRef(ITfSourceSingle *iface)
{
    ThreadMgr *This = impl_from_ITfSourceSingle(iface);
    return ITfThreadMgrEx_AddRef(&This->ITfThreadMgrEx_iface);
}

static ULONG WINAPI ThreadMgrSourceSingle_Release(ITfSourceSingle *iface)
{
    ThreadMgr *This = impl_from_ITfSourceSingle(iface);
    return ITfThreadMgrEx_Release(&This->ITfThreadMgrEx_iface);
}

static HRESULT WINAPI ThreadMgrSourceSingle_AdviseSingleSink( ITfSourceSingle *iface,
    TfClientId tid, REFIID riid, IUnknown *punk)
{
    ThreadMgr *This = impl_from_ITfSourceSingle(iface);
    FIXME("STUB:(%p) %i %s %p\n",This, tid, debugstr_guid(riid),punk);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgrSourceSingle_UnadviseSingleSink( ITfSourceSingle *iface,
    TfClientId tid, REFIID riid)
{
    ThreadMgr *This = impl_from_ITfSourceSingle(iface);
    FIXME("STUB:(%p) %i %s\n",This, tid, debugstr_guid(riid));
    return E_NOTIMPL;
}

static const ITfSourceSingleVtbl SourceSingleVtbl =
{
    ThreadMgrSourceSingle_QueryInterface,
    ThreadMgrSourceSingle_AddRef,
    ThreadMgrSourceSingle_Release,
    ThreadMgrSourceSingle_AdviseSingleSink,
    ThreadMgrSourceSingle_UnadviseSingleSink
};

HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ThreadMgr *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Only 1 ThreadMgr is created per thread */
    This = TlsGetValue(tlsIndex);
    if (This)
    {
        ThreadMgr_AddRef(&This->ITfThreadMgrEx_iface);
        *ppOut = (IUnknown*)&This->ITfThreadMgrEx_iface;
        return S_OK;
    }

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ThreadMgr));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ITfThreadMgrEx_iface.lpVtbl = &ThreadMgrExVtbl;
    This->ITfSource_iface.lpVtbl = &ThreadMgrSourceVtbl;
    This->ITfKeystrokeMgr_iface.lpVtbl = &KeystrokeMgrVtbl;
    This->ITfMessagePump_iface.lpVtbl = &MessagePumpVtbl;
    This->ITfClientId_iface.lpVtbl = &ClientIdVtbl;
    This->ITfThreadMgrEventSink_iface.lpVtbl = &ThreadMgrEventSinkVtbl;
    This->ITfUIElementMgr_iface.lpVtbl = &ThreadMgrUIElementMgrVtbl;
    This->ITfSourceSingle_iface.lpVtbl = &SourceSingleVtbl;
    This->refCount = 1;
    TlsSetValue(tlsIndex,This);

    CompartmentMgr_Constructor((IUnknown*)&This->ITfThreadMgrEx_iface, &IID_IUnknown, (IUnknown**)&This->CompartmentMgr);

    list_init(&This->CurrentPreservedKeys);
    list_init(&This->CreatedDocumentMgrs);
    list_init(&This->AssociatedFocusWindows);

    list_init(&This->ActiveLanguageProfileNotifySink);
    list_init(&This->DisplayAttributeNotifySink);
    list_init(&This->KeyTraceEventSink);
    list_init(&This->PreservedKeyNotifySink);
    list_init(&This->ThreadFocusSink);
    list_init(&This->ThreadMgrEventSink);

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)&This->ITfThreadMgrEx_iface;
    return S_OK;
}

/**************************************************
 * IEnumTfDocumentMgrs implementation
 **************************************************/
static void EnumTfDocumentMgr_Destructor(EnumTfDocumentMgr *This)
{
    TRACE("destroying %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI EnumTfDocumentMgr_QueryInterface(IEnumTfDocumentMgrs *iface, REFIID iid, LPVOID *ppvOut)
{
    EnumTfDocumentMgr *This = impl_from_IEnumTfDocumentMgrs(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IEnumTfDocumentMgrs))
    {
        *ppvOut = &This->IEnumTfDocumentMgrs_iface;
    }

    if (*ppvOut)
    {
        IEnumTfDocumentMgrs_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumTfDocumentMgr_AddRef(IEnumTfDocumentMgrs *iface)
{
    EnumTfDocumentMgr *This = impl_from_IEnumTfDocumentMgrs(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI EnumTfDocumentMgr_Release(IEnumTfDocumentMgrs *iface)
{
    EnumTfDocumentMgr *This = impl_from_IEnumTfDocumentMgrs(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        EnumTfDocumentMgr_Destructor(This);
    return ret;
}

static HRESULT WINAPI EnumTfDocumentMgr_Next(IEnumTfDocumentMgrs *iface,
    ULONG ulCount, ITfDocumentMgr **rgDocumentMgr, ULONG *pcFetched)
{
    EnumTfDocumentMgr *This = impl_from_IEnumTfDocumentMgrs(iface);
    ULONG fetched = 0;

    TRACE("(%p)\n",This);

    if (rgDocumentMgr == NULL) return E_POINTER;

    while (fetched < ulCount)
    {
        DocumentMgrEntry *mgrentry;
        if (This->index == NULL)
            break;

        mgrentry = LIST_ENTRY(This->index,DocumentMgrEntry,entry);
        if (mgrentry == NULL)
            break;

        *rgDocumentMgr = mgrentry->docmgr;
        ITfDocumentMgr_AddRef(*rgDocumentMgr);

        This->index = list_next(This->head, This->index);
        ++fetched;
        ++rgDocumentMgr;
    }

    if (pcFetched) *pcFetched = fetched;
    return fetched == ulCount ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumTfDocumentMgr_Skip( IEnumTfDocumentMgrs* iface, ULONG celt)
{
    EnumTfDocumentMgr *This = impl_from_IEnumTfDocumentMgrs(iface);
    ULONG i;

    TRACE("(%p)\n",This);
    for(i = 0; i < celt && This->index != NULL; i++)
        This->index = list_next(This->head, This->index);
    return S_OK;
}

static HRESULT WINAPI EnumTfDocumentMgr_Reset( IEnumTfDocumentMgrs* iface)
{
    EnumTfDocumentMgr *This = impl_from_IEnumTfDocumentMgrs(iface);
    TRACE("(%p)\n",This);
    This->index = list_head(This->head);
    return S_OK;
}

static HRESULT WINAPI EnumTfDocumentMgr_Clone( IEnumTfDocumentMgrs *iface,
    IEnumTfDocumentMgrs **ppenum)
{
    EnumTfDocumentMgr *This = impl_from_IEnumTfDocumentMgrs(iface);
    HRESULT res;

    TRACE("(%p)\n",This);

    if (ppenum == NULL) return E_POINTER;

    res = EnumTfDocumentMgr_Constructor(This->head, ppenum);
    if (SUCCEEDED(res))
    {
        EnumTfDocumentMgr *new_This = impl_from_IEnumTfDocumentMgrs(*ppenum);
        new_This->index = This->index;
    }
    return res;
}

static const IEnumTfDocumentMgrsVtbl EnumTfDocumentMgrsVtbl =
{
    EnumTfDocumentMgr_QueryInterface,
    EnumTfDocumentMgr_AddRef,
    EnumTfDocumentMgr_Release,
    EnumTfDocumentMgr_Clone,
    EnumTfDocumentMgr_Next,
    EnumTfDocumentMgr_Reset,
    EnumTfDocumentMgr_Skip
};

static HRESULT EnumTfDocumentMgr_Constructor(struct list* head, IEnumTfDocumentMgrs **ppOut)
{
    EnumTfDocumentMgr *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(EnumTfDocumentMgr));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->IEnumTfDocumentMgrs_iface.lpVtbl= &EnumTfDocumentMgrsVtbl;
    This->refCount = 1;
    This->head = head;
    This->index = list_head(This->head);

    TRACE("returning %p\n", &This->IEnumTfDocumentMgrs_iface);
    *ppOut = &This->IEnumTfDocumentMgrs_iface;
    return S_OK;
}

void ThreadMgr_OnDocumentMgrDestruction(ITfThreadMgr *iface, ITfDocumentMgr *mgr)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEx((ITfThreadMgrEx *)iface);
    struct list *cursor;
    LIST_FOR_EACH(cursor, &This->CreatedDocumentMgrs)
    {
        DocumentMgrEntry *mgrentry = LIST_ENTRY(cursor,DocumentMgrEntry,entry);
        if (mgrentry->docmgr == mgr)
        {
            list_remove(cursor);
            HeapFree(GetProcessHeap(),0,mgrentry);
            return;
        }
    }
    FIXME("ITfDocumentMgr %p not found in this thread\n",mgr);
}
