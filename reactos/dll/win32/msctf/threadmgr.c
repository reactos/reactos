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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"
#include "olectl.h"

#include "wine/unicode.h"
#include "wine/list.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagThreadMgrSink {
    struct list         entry;
    union {
        /* ThreadMgr Sinks */
        IUnknown            *pIUnknown;
        /* ITfActiveLanguageProfileNotifySink *pITfActiveLanguageProfileNotifySink; */
        /* ITfDisplayAttributeNotifySink *pITfDisplayAttributeNotifySink; */
        /* ITfKeyTraceEventSink *pITfKeyTraceEventSink; */
        /* ITfPreservedKeyNotifySink *pITfPreservedKeyNotifySink; */
        /* ITfThreadFocusSink *pITfThreadFocusSink; */
        ITfThreadMgrEventSink *pITfThreadMgrEventSink;
    } interfaces;
} ThreadMgrSink;

typedef struct tagACLMulti {
    const ITfThreadMgrVtbl *ThreadMgrVtbl;
    const ITfSourceVtbl *SourceVtbl;
    const ITfKeystrokeMgrVtbl *KeystrokeMgrVtbl;
    LONG refCount;

    const ITfThreadMgrEventSinkVtbl *ThreadMgrEventSinkVtbl; /* internal */

    ITfDocumentMgr *focus;

    /* kept as separate lists to reduce unnecessary iterations */
    struct list     ActiveLanguageProfileNotifySink;
    struct list     DisplayAttributeNotifySink;
    struct list     KeyTraceEventSink;
    struct list     PreservedKeyNotifySink;
    struct list     ThreadFocusSink;
    struct list     ThreadMgrEventSink;
} ThreadMgr;

static inline ThreadMgr *impl_from_ITfSourceVtbl(ITfSource *iface)
{
    return (ThreadMgr *)((char *)iface - FIELD_OFFSET(ThreadMgr,SourceVtbl));
}

static inline ThreadMgr *impl_from_ITfKeystrokeMgrVtbl(ITfKeystrokeMgr *iface)
{
    return (ThreadMgr *)((char *)iface - FIELD_OFFSET(ThreadMgr,KeystrokeMgrVtbl));
}

static inline ThreadMgr *impl_from_ITfThreadMgrEventSink(ITfThreadMgrEventSink *iface)
{
    return (ThreadMgr *)((char *)iface - FIELD_OFFSET(ThreadMgr,ThreadMgrEventSinkVtbl));
}

static void free_sink(ThreadMgrSink *sink)
{
        IUnknown_Release(sink->interfaces.pIUnknown);
        HeapFree(GetProcessHeap(),0,sink);
}

static void ThreadMgr_Destructor(ThreadMgr *This)
{
    struct list *cursor, *cursor2;

    TlsSetValue(tlsIndex,NULL);
    TRACE("destroying %p\n", This);
    if (This->focus)
        ITfDocumentMgr_Release(This->focus);

    /* free sinks */
    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->ActiveLanguageProfileNotifySink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        list_remove(cursor);
        free_sink(sink);
    }
    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->DisplayAttributeNotifySink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        list_remove(cursor);
        free_sink(sink);
    }
    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->KeyTraceEventSink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        list_remove(cursor);
        free_sink(sink);
    }
    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->PreservedKeyNotifySink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        list_remove(cursor);
        free_sink(sink);
    }
    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->ThreadFocusSink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        list_remove(cursor);
        free_sink(sink);
    }
    LIST_FOR_EACH_SAFE(cursor, cursor2, &This->ThreadMgrEventSink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        list_remove(cursor);
        free_sink(sink);
    }

    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI ThreadMgr_QueryInterface(ITfThreadMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfThreadMgr))
    {
        *ppvOut = This;
    }
    else if (IsEqualIID(iid, &IID_ITfSource))
    {
        *ppvOut = &This->SourceVtbl;
    }
    else if (IsEqualIID(iid, &IID_ITfKeystrokeMgr))
    {
        *ppvOut = &This->KeystrokeMgrVtbl;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ThreadMgr_AddRef(ITfThreadMgr *iface)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ThreadMgr_Release(ITfThreadMgr *iface)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ThreadMgr_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfThreadMgr functions
 *****************************************************/

static HRESULT WINAPI ThreadMgr_fnActivate( ITfThreadMgr* iface, TfClientId *ptid)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_fnDeactivate( ITfThreadMgr* iface)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);

    if (This->focus)
    {
        ITfThreadMgrEventSink_OnSetFocus((ITfThreadMgrEventSink*)&This->ThreadMgrEventSinkVtbl, 0, This->focus);
        ITfDocumentMgr_Release(This->focus);
        This->focus = 0;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_CreateDocumentMgr( ITfThreadMgr* iface, ITfDocumentMgr
**ppdim)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    TRACE("(%p)\n",iface);
    return DocumentMgr_Constructor((ITfThreadMgrEventSink*)&This->ThreadMgrEventSinkVtbl, ppdim);
}

static HRESULT WINAPI ThreadMgr_EnumDocumentMgrs( ITfThreadMgr* iface, IEnumTfDocumentMgrs
**ppEnum)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_GetFocus( ITfThreadMgr* iface, ITfDocumentMgr
**ppdimFocus)
{
    ThreadMgr *This = (ThreadMgr *)iface;
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

static HRESULT WINAPI ThreadMgr_SetFocus( ITfThreadMgr* iface, ITfDocumentMgr *pdimFocus)
{
    ITfDocumentMgr *check;
    ThreadMgr *This = (ThreadMgr *)iface;

    TRACE("(%p) %p\n",This,pdimFocus);

    if (!pdimFocus || FAILED(IUnknown_QueryInterface(pdimFocus,&IID_ITfDocumentMgr,(LPVOID*) &check)))
        return E_INVALIDARG;

    ITfThreadMgrEventSink_OnSetFocus((ITfThreadMgrEventSink*)&This->ThreadMgrEventSinkVtbl, check, This->focus);

    if (This->focus)
        ITfDocumentMgr_Release(This->focus);

    This->focus = check;
    return S_OK;
}

static HRESULT WINAPI ThreadMgr_AssociateFocus( ITfThreadMgr* iface, HWND hwnd,
ITfDocumentMgr *pdimNew, ITfDocumentMgr **ppdimPrev)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_IsThreadFocus( ITfThreadMgr* iface, BOOL *pfThreadFocus)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_GetFunctionProvider( ITfThreadMgr* iface, REFCLSID clsid,
ITfFunctionProvider **ppFuncProv)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_EnumFunctionProviders( ITfThreadMgr* iface,
IEnumTfFunctionProviders **ppEnum)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ThreadMgr_GetGlobalCompartment( ITfThreadMgr* iface,
ITfCompartmentMgr **ppCompMgr)
{
    ThreadMgr *This = (ThreadMgr *)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfThreadMgrVtbl ThreadMgr_ThreadMgrVtbl =
{
    ThreadMgr_QueryInterface,
    ThreadMgr_AddRef,
    ThreadMgr_Release,

    ThreadMgr_fnActivate,
    ThreadMgr_fnDeactivate,
    ThreadMgr_CreateDocumentMgr,
    ThreadMgr_EnumDocumentMgrs,
    ThreadMgr_GetFocus,
    ThreadMgr_SetFocus,
    ThreadMgr_AssociateFocus,
    ThreadMgr_IsThreadFocus,
    ThreadMgr_GetFunctionProvider,
    ThreadMgr_EnumFunctionProviders,
    ThreadMgr_GetGlobalCompartment
};


static HRESULT WINAPI Source_QueryInterface(ITfSource *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfSourceVtbl(iface);
    return ThreadMgr_QueryInterface((ITfThreadMgr *)This, iid, *ppvOut);
}

static ULONG WINAPI Source_AddRef(ITfSource *iface)
{
    ThreadMgr *This = impl_from_ITfSourceVtbl(iface);
    return ThreadMgr_AddRef((ITfThreadMgr*)This);
}

static ULONG WINAPI Source_Release(ITfSource *iface)
{
    ThreadMgr *This = impl_from_ITfSourceVtbl(iface);
    return ThreadMgr_Release((ITfThreadMgr *)This);
}

/*****************************************************
 * ITfSource functions
 *****************************************************/
static WINAPI HRESULT ThreadMgrSource_AdviseSink(ITfSource *iface,
        REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    ThreadMgrSink *tms;
    ThreadMgr *This = impl_from_ITfSourceVtbl(iface);

    TRACE("(%p) %s %p %p\n",This,debugstr_guid(riid),punk,pdwCookie);

    if (!riid || !punk || !pdwCookie)
        return E_INVALIDARG;

    if (IsEqualIID(riid, &IID_ITfThreadMgrEventSink))
    {
        tms = HeapAlloc(GetProcessHeap(),0,sizeof(ThreadMgrSink));
        if (!tms)
            return E_OUTOFMEMORY;
        if (!SUCCEEDED(IUnknown_QueryInterface(punk, riid, (LPVOID*)&tms->interfaces.pITfThreadMgrEventSink)))
        {
            HeapFree(GetProcessHeap(),0,tms);
            return CONNECT_E_CANNOTCONNECT;
        }
        list_add_head(&This->ThreadMgrEventSink,&tms->entry);
        *pdwCookie = generate_Cookie(COOKIE_MAGIC_TMSINK, tms);
    }
    else
    {
        FIXME("(%p) Unhandled Sink: %s\n",This,debugstr_guid(riid));
        return E_NOTIMPL;
    }

    TRACE("cookie %x\n",*pdwCookie);

    return S_OK;
}

static WINAPI HRESULT ThreadMgrSource_UnadviseSink(ITfSource *iface, DWORD pdwCookie)
{
    ThreadMgrSink *sink;
    ThreadMgr *This = impl_from_ITfSourceVtbl(iface);

    TRACE("(%p) %x\n",This,pdwCookie);

    if (get_Cookie_magic(pdwCookie)!=COOKIE_MAGIC_TMSINK)
        return E_INVALIDARG;

    sink = (ThreadMgrSink*)remove_Cookie(pdwCookie);
    if (!sink)
        return CONNECT_E_NOCONNECTION;

    list_remove(&sink->entry);
    free_sink(sink);

    return S_OK;
}

static const ITfSourceVtbl ThreadMgr_SourceVtbl =
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
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    return ThreadMgr_QueryInterface((ITfThreadMgr *)This, iid, *ppvOut);
}

static ULONG WINAPI KeystrokeMgr_AddRef(ITfKeystrokeMgr *iface)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    return ThreadMgr_AddRef((ITfThreadMgr*)This);
}

static ULONG WINAPI KeystrokeMgr_Release(ITfKeystrokeMgr *iface)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    return ThreadMgr_Release((ITfThreadMgr *)This);
}

static HRESULT WINAPI KeystrokeMgr_AdviseKeyEventSink(ITfKeystrokeMgr *iface,
        TfClientId tid, ITfKeyEventSink *pSink, BOOL fForeground)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_UnadviseKeyEventSink(ITfKeystrokeMgr *iface,
        TfClientId tid)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_GetForeground(ITfKeystrokeMgr *iface,
        CLSID *pclsid)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_TestKeyDown(ITfKeystrokeMgr *iface,
        WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_TestKeyUp(ITfKeystrokeMgr *iface,
        WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_KeyDown(ITfKeystrokeMgr *iface,
        WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_KeyUp(ITfKeystrokeMgr *iface,
        WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_GetPreservedKey(ITfKeystrokeMgr *iface,
        ITfContext *pic, const TF_PRESERVEDKEY *pprekey, GUID *pguid)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_IsPreservedKey(ITfKeystrokeMgr *iface,
        REFGUID rguid, const TF_PRESERVEDKEY *pprekey, BOOL *pfRegistered)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_PreserveKey(ITfKeystrokeMgr *iface,
        TfClientId tid, REFGUID rguid, const TF_PRESERVEDKEY *prekey,
        const WCHAR *pchDesc, ULONG cchDesc)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_UnpreserveKey(ITfKeystrokeMgr *iface,
        REFGUID rguid, const TF_PRESERVEDKEY *pprekey)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_SetPreservedKeyDescription(ITfKeystrokeMgr *iface,
        REFGUID rguid, const WCHAR *pchDesc, ULONG cchDesc)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_GetPreservedKeyDescription(ITfKeystrokeMgr *iface,
        REFGUID rguid, BSTR *pbstrDesc)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI KeystrokeMgr_SimulatePreservedKey(ITfKeystrokeMgr *iface,
        ITfContext *pic, REFGUID rguid, BOOL *pfEaten)
{
    ThreadMgr *This = impl_from_ITfKeystrokeMgrVtbl(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfKeystrokeMgrVtbl ThreadMgr_KeystrokeMgrVtbl =
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
 * ITfThreadMgrEventSink functions  (internal)
 *****************************************************/
static HRESULT WINAPI ThreadMgrEventSink_QueryInterface(ITfThreadMgrEventSink *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    return ThreadMgr_QueryInterface((ITfThreadMgr *)This, iid, *ppvOut);
}

static ULONG WINAPI ThreadMgrEventSink_AddRef(ITfThreadMgrEventSink *iface)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    return ThreadMgr_AddRef((ITfThreadMgr*)This);
}

static ULONG WINAPI ThreadMgrEventSink_Release(ITfThreadMgrEventSink *iface)
{
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);
    return ThreadMgr_Release((ITfThreadMgr *)This);
}


static WINAPI HRESULT ThreadMgrEventSink_OnInitDocumentMgr(
        ITfThreadMgrEventSink *iface,ITfDocumentMgr *pdim)
{
    struct list *cursor;
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);

    TRACE("(%p) %p\n",This,pdim);

    LIST_FOR_EACH(cursor, &This->ThreadMgrEventSink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        ITfThreadMgrEventSink_OnInitDocumentMgr(sink->interfaces.pITfThreadMgrEventSink,pdim);
    }

    return S_OK;
}

static WINAPI HRESULT ThreadMgrEventSink_OnUninitDocumentMgr(
        ITfThreadMgrEventSink *iface, ITfDocumentMgr *pdim)
{
    struct list *cursor;
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);

    TRACE("(%p) %p\n",This,pdim);

    LIST_FOR_EACH(cursor, &This->ThreadMgrEventSink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        ITfThreadMgrEventSink_OnUninitDocumentMgr(sink->interfaces.pITfThreadMgrEventSink,pdim);
    }

    return S_OK;
}

static WINAPI HRESULT ThreadMgrEventSink_OnSetFocus(
        ITfThreadMgrEventSink *iface, ITfDocumentMgr *pdimFocus,
        ITfDocumentMgr *pdimPrevFocus)
{
    struct list *cursor;
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);

    TRACE("(%p) %p %p\n",This,pdimFocus, pdimPrevFocus);

    LIST_FOR_EACH(cursor, &This->ThreadMgrEventSink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        ITfThreadMgrEventSink_OnSetFocus(sink->interfaces.pITfThreadMgrEventSink, pdimFocus, pdimPrevFocus);
    }

    return S_OK;
}

static WINAPI HRESULT ThreadMgrEventSink_OnPushContext(
        ITfThreadMgrEventSink *iface, ITfContext *pic)
{
    struct list *cursor;
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);

    TRACE("(%p) %p\n",This,pic);

    LIST_FOR_EACH(cursor, &This->ThreadMgrEventSink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        ITfThreadMgrEventSink_OnPushContext(sink->interfaces.pITfThreadMgrEventSink,pic);
    }

    return S_OK;
}

static WINAPI HRESULT ThreadMgrEventSink_OnPopContext(
        ITfThreadMgrEventSink *iface, ITfContext *pic)
{
    struct list *cursor;
    ThreadMgr *This = impl_from_ITfThreadMgrEventSink(iface);

    TRACE("(%p) %p\n",This,pic);

    LIST_FOR_EACH(cursor, &This->ThreadMgrEventSink)
    {
        ThreadMgrSink* sink = LIST_ENTRY(cursor,ThreadMgrSink,entry);
        ITfThreadMgrEventSink_OnPopContext(sink->interfaces.pITfThreadMgrEventSink,pic);
    }

    return S_OK;
}

static const ITfThreadMgrEventSinkVtbl ThreadMgr_ThreadMgrEventSinkVtbl =
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

HRESULT ThreadMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ThreadMgr *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Only 1 ThreadMgr is created per thread */
    This = TlsGetValue(tlsIndex);
    if (This)
    {
        ThreadMgr_AddRef((ITfThreadMgr*)This);
        *ppOut = (IUnknown*)This;
        return S_OK;
    }

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ThreadMgr));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ThreadMgrVtbl= &ThreadMgr_ThreadMgrVtbl;
    This->SourceVtbl = &ThreadMgr_SourceVtbl;
    This->KeystrokeMgrVtbl= &ThreadMgr_KeystrokeMgrVtbl;
    This->ThreadMgrEventSinkVtbl = &ThreadMgr_ThreadMgrEventSinkVtbl;
    This->refCount = 1;
    TlsSetValue(tlsIndex,This);

    list_init(&This->ActiveLanguageProfileNotifySink);
    list_init(&This->DisplayAttributeNotifySink);
    list_init(&This->KeyTraceEventSink);
    list_init(&This->PreservedKeyNotifySink);
    list_init(&This->ThreadFocusSink);
    list_init(&This->ThreadMgrEventSink);

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    return S_OK;
}
