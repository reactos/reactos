/*
 * Unit tests for ITfInputProcessor
 *
 * Copyright 2009 Aric Stewart, CodeWeavers
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

#include <stdio.h>

#define COBJMACROS
#define CONST_VTABLE
#include "wine/test.h"
#include "winuser.h"
#include "initguid.h"
#include "shlwapi.h"
#include "shlguid.h"
#include "comcat.h"
#include "msctf.h"
#include "olectl.h"

static ITfInputProcessorProfiles* g_ipp;
static LANGID gLangid;
static ITfCategoryMgr * g_cm = NULL;
static ITfThreadMgr* g_tm = NULL;
static ITfDocumentMgr *g_dm = NULL;
static TfClientId cid = 0;
static TfClientId tid = 0;

static ITextStoreACPSink *ACPSink;

#define SINK_UNEXPECTED 0
#define SINK_EXPECTED 1
#define SINK_FIRED 2
#define SINK_IGNORE 3
#define SINK_OPTIONAL 4
#define SINK_SAVE 5

#define SINK_ACTION_MASK 0xff
#define SINK_OPTION_MASK 0xff00
#define SINK_EXPECTED_COUNT_MASK 0xff0000

#define SINK_OPTION_TODO      0x0100

#define FOCUS_IGNORE    (ITfDocumentMgr*)0xffffffff
#define FOCUS_SAVE      (ITfDocumentMgr*)0xfffffffe

static BOOL test_ShouldActivate = FALSE;
static BOOL test_ShouldDeactivate = FALSE;

static DWORD tmSinkCookie;
static DWORD tmSinkRefCount;
static DWORD documentStatus;
static ITfDocumentMgr *test_CurrentFocus = NULL;
static ITfDocumentMgr *test_PrevFocus = NULL;
static ITfDocumentMgr *test_LastCurrentFocus = FOCUS_SAVE;
static ITfDocumentMgr *test_FirstPrevFocus = FOCUS_SAVE;
static INT  test_OnSetFocus = SINK_UNEXPECTED;
static INT  test_OnInitDocumentMgr = SINK_UNEXPECTED;
static INT  test_OnPushContext = SINK_UNEXPECTED;
static INT  test_OnPopContext = SINK_UNEXPECTED;
static INT  test_KEV_OnSetFocus = SINK_UNEXPECTED;
static INT  test_ACP_AdviseSink = SINK_UNEXPECTED;
static INT  test_ACP_GetStatus = SINK_UNEXPECTED;
static INT  test_ACP_RequestLock = SINK_UNEXPECTED;
static INT  test_ACP_GetEndACP = SINK_UNEXPECTED;
static INT  test_ACP_GetSelection = SINK_UNEXPECTED;
static INT  test_DoEditSession = SINK_UNEXPECTED;
static INT  test_ACP_InsertTextAtSelection = SINK_UNEXPECTED;
static INT  test_ACP_SetSelection = SINK_UNEXPECTED;
static INT  test_OnEndEdit = SINK_UNEXPECTED;


static inline int expected_count(int *sink)
{
    return (*sink & SINK_EXPECTED_COUNT_MASK)>>16;
}

static inline void _sink_fire_ok(INT *sink, const CHAR* name)
{
    int count;
    int todo = *sink & SINK_OPTION_TODO;
    int action = *sink & SINK_ACTION_MASK;

    if (winetest_interactive)
        winetest_trace("firing %s\n",name);

    switch (action)
    {
        case SINK_OPTIONAL:
        case SINK_EXPECTED:
            count = expected_count(sink);
            if (count > 1)
            {
                count --;
                *sink = (*sink & ~SINK_EXPECTED_COUNT_MASK) + (count << 16);
                return;
            }
            break;
        case SINK_IGNORE:
            winetest_trace("Ignoring %s\n",name);
            return;
        case SINK_SAVE:
            count = expected_count(sink) + 1;
            *sink = (*sink & ~SINK_EXPECTED_COUNT_MASK) + (count << 16);
            return;
        default:
            if (todo)
                todo_wine winetest_ok(0, "Unexpected %s sink\n",name);
            else
                winetest_ok(0, "Unexpected %s sink\n",name);
    }
    *sink = SINK_FIRED;
}

#define sink_fire_ok(a,b) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _sink_fire_ok(a,b)

static inline void _sink_check_ok(INT *sink, const CHAR* name)
{
    int action = *sink & SINK_ACTION_MASK;
    int todo = *sink & SINK_OPTION_TODO;
    int count = expected_count(sink);

    switch (action)
    {
        case SINK_OPTIONAL:
            if (winetest_interactive)
                winetest_trace("optional sink %s not fired\n",name);
        case SINK_FIRED:
            break;
        case SINK_IGNORE:
            return;
        case SINK_SAVE:
            if (count == 0 && winetest_interactive)
                winetest_trace("optional sink %s not fired\n",name);
            break;
        default:
            if (todo)
                todo_wine winetest_ok(0, "%s not fired as expected, in state %x\n",name,*sink);
            else
                winetest_ok(0, "%s not fired as expected, in state %x\n",name,*sink);
    }
    *sink = SINK_UNEXPECTED;
}

#define sink_check_ok(a,b) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _sink_check_ok(a,b)

static inline void _sink_check_saved(INT *sink, ITfDocumentMgr *PrevFocus, ITfDocumentMgr *CurrentFocus, const CHAR* name)
{
    int count = expected_count(sink);
    _sink_check_ok(sink, name);
    if (PrevFocus != FOCUS_IGNORE && count != 0)
        winetest_ok(PrevFocus == test_FirstPrevFocus, "%s expected prev focus %p got %p\n", name, PrevFocus, test_FirstPrevFocus);
    if (CurrentFocus != FOCUS_IGNORE && count != 0)
        winetest_ok(CurrentFocus == test_LastCurrentFocus, "%s expected current focus %p got %p\n", name, CurrentFocus, test_LastCurrentFocus);
    test_FirstPrevFocus = FOCUS_SAVE;
    test_LastCurrentFocus = FOCUS_SAVE;
}

#define sink_check_saved(s,p,c,n) (winetest_set_location(__FILE__,__LINE__), 0) ? 0 : _sink_check_saved(s,p,c,n)

/**********************************************************************
 * ITextStoreACP
 **********************************************************************/
typedef struct tagTextStoreACP
{
    ITextStoreACP ITextStoreACP_iface;
    LONG refCount;

} TextStoreACP;

static inline TextStoreACP *impl_from_ITextStoreACP(ITextStoreACP *iface)
{
    return CONTAINING_RECORD(iface, TextStoreACP, ITextStoreACP_iface);
}

static void TextStoreACP_Destructor(TextStoreACP *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI TextStoreACP_QueryInterface(ITextStoreACP *iface, REFIID iid, LPVOID *ppvOut)
{
    TextStoreACP *This = impl_from_ITextStoreACP(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITextStoreACP))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        ITextStoreACP_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TextStoreACP_AddRef(ITextStoreACP *iface)
{
    TextStoreACP *This = impl_from_ITextStoreACP(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI TextStoreACP_Release(ITextStoreACP *iface)
{
    TextStoreACP *This = impl_from_ITextStoreACP(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        TextStoreACP_Destructor(This);
    return ret;
}

static HRESULT WINAPI TextStoreACP_AdviseSink(ITextStoreACP *iface,
    REFIID riid, IUnknown *punk, DWORD dwMask)
{
    HRESULT hr;

    sink_fire_ok(&test_ACP_AdviseSink,"TextStoreACP_AdviseSink");

    hr = IUnknown_QueryInterface(punk, &IID_ITextStoreACPSink,(LPVOID*)(&ACPSink));
    ok(SUCCEEDED(hr),"Unable to QueryInterface on sink\n");
    return S_OK;
}

static HRESULT WINAPI TextStoreACP_UnadviseSink(ITextStoreACP *iface,
    IUnknown *punk)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI TextStoreACP_RequestLock(ITextStoreACP *iface,
    DWORD dwLockFlags, HRESULT *phrSession)
{
    sink_fire_ok(&test_ACP_RequestLock,"TextStoreACP_RequestLock");
    *phrSession = ITextStoreACPSink_OnLockGranted(ACPSink, dwLockFlags);
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetStatus(ITextStoreACP *iface,
    TS_STATUS *pdcs)
{
    sink_fire_ok(&test_ACP_GetStatus,"TextStoreACP_GetStatus");
    pdcs->dwDynamicFlags = documentStatus;
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_QueryInsert(ITextStoreACP *iface,
    LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG *pacpResultStart,
    LONG *pacpResultEnd)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetSelection(ITextStoreACP *iface,
    ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched)
{
    sink_fire_ok(&test_ACP_GetSelection,"TextStoreACP_GetSelection");

    pSelection->acpStart = 10;
    pSelection->acpEnd = 20;
    pSelection->style.fInterimChar = 0;
    pSelection->style.ase = TS_AE_NONE;
    *pcFetched = 1;

    return S_OK;
}
static HRESULT WINAPI TextStoreACP_SetSelection(ITextStoreACP *iface,
    ULONG ulCount, const TS_SELECTION_ACP *pSelection)
{
    sink_fire_ok(&test_ACP_SetSelection,"TextStoreACP_SetSelection");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetText(ITextStoreACP *iface,
    LONG acpStart, LONG acpEnd, WCHAR *pchPlain, ULONG cchPlainReq,
    ULONG *pcchPlainRet, TS_RUNINFO *prgRunInfo, ULONG cRunInfoReq,
    ULONG *pcRunInfoRet, LONG *pacpNext)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_SetText(ITextStoreACP *iface,
    DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR *pchText,
    ULONG cch, TS_TEXTCHANGE *pChange)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetFormattedText(ITextStoreACP *iface,
    LONG acpStart, LONG acpEnd, IDataObject **ppDataObject)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetEmbedded(ITextStoreACP *iface,
    LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_QueryInsertEmbedded(ITextStoreACP *iface,
    const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_InsertEmbedded(ITextStoreACP *iface,
    DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject,
    TS_TEXTCHANGE *pChange)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_InsertTextAtSelection(ITextStoreACP *iface,
    DWORD dwFlags, const WCHAR *pchText, ULONG cch, LONG *pacpStart,
    LONG *pacpEnd, TS_TEXTCHANGE *pChange)
{
    sink_fire_ok(&test_ACP_InsertTextAtSelection,"TextStoreACP_InsertTextAtSelection");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_InsertEmbeddedAtSelection(ITextStoreACP *iface,
    DWORD dwFlags, IDataObject *pDataObject, LONG *pacpStart, LONG *pacpEnd,
    TS_TEXTCHANGE *pChange)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_RequestSupportedAttrs(ITextStoreACP *iface,
    DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_RequestAttrsAtPosition(ITextStoreACP *iface,
    LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs,
    DWORD dwFlags)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_RequestAttrsTransitioningAtPosition(ITextStoreACP *iface,
    LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs,
    DWORD dwFlags)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_FindNextAttrTransition(ITextStoreACP *iface,
    LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs,
    DWORD dwFlags, LONG *pacpNext, BOOL *pfFound, LONG *plFoundOffset)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_RetrieveRequestedAttrs(ITextStoreACP *iface,
    ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetEndACP(ITextStoreACP *iface,
    LONG *pacp)
{
    sink_fire_ok(&test_ACP_GetEndACP,"TextStoreACP_GetEndACP");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetActiveView(ITextStoreACP *iface,
    TsViewCookie *pvcView)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetACPFromPoint(ITextStoreACP *iface,
    TsViewCookie vcView, const POINT *ptScreen, DWORD dwFlags,
    LONG *pacp)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetTextExt(ITextStoreACP *iface,
    TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc,
    BOOL *pfClipped)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetScreenExt(ITextStoreACP *iface,
    TsViewCookie vcView, RECT *prc)
{
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetWnd(ITextStoreACP *iface,
    TsViewCookie vcView, HWND *phwnd)
{
    trace("\n");
    return S_OK;
}

static const ITextStoreACPVtbl TextStoreACP_TextStoreACPVtbl =
{
    TextStoreACP_QueryInterface,
    TextStoreACP_AddRef,
    TextStoreACP_Release,

    TextStoreACP_AdviseSink,
    TextStoreACP_UnadviseSink,
    TextStoreACP_RequestLock,
    TextStoreACP_GetStatus,
    TextStoreACP_QueryInsert,
    TextStoreACP_GetSelection,
    TextStoreACP_SetSelection,
    TextStoreACP_GetText,
    TextStoreACP_SetText,
    TextStoreACP_GetFormattedText,
    TextStoreACP_GetEmbedded,
    TextStoreACP_QueryInsertEmbedded,
    TextStoreACP_InsertEmbedded,
    TextStoreACP_InsertTextAtSelection,
    TextStoreACP_InsertEmbeddedAtSelection,
    TextStoreACP_RequestSupportedAttrs,
    TextStoreACP_RequestAttrsAtPosition,
    TextStoreACP_RequestAttrsTransitioningAtPosition,
    TextStoreACP_FindNextAttrTransition,
    TextStoreACP_RetrieveRequestedAttrs,
    TextStoreACP_GetEndACP,
    TextStoreACP_GetActiveView,
    TextStoreACP_GetACPFromPoint,
    TextStoreACP_GetTextExt,
    TextStoreACP_GetScreenExt,
    TextStoreACP_GetWnd
};

static HRESULT TextStoreACP_Constructor(IUnknown **ppOut)
{
    TextStoreACP *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TextStoreACP));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ITextStoreACP_iface.lpVtbl = &TextStoreACP_TextStoreACPVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)This;
    return S_OK;
}

/**********************************************************************
 * ITfThreadMgrEventSink
 **********************************************************************/
typedef struct tagThreadMgrEventSink
{
    ITfThreadMgrEventSink ITfThreadMgrEventSink_iface;
    LONG refCount;
} ThreadMgrEventSink;

static inline ThreadMgrEventSink *impl_from_ITfThreadMgrEventSink(ITfThreadMgrEventSink *iface)
{
    return CONTAINING_RECORD(iface, ThreadMgrEventSink, ITfThreadMgrEventSink_iface);
}

static void ThreadMgrEventSink_Destructor(ThreadMgrEventSink *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI ThreadMgrEventSink_QueryInterface(ITfThreadMgrEventSink *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgrEventSink *This = impl_from_ITfThreadMgrEventSink(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfThreadMgrEventSink))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        ITfThreadMgrEventSink_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ThreadMgrEventSink_AddRef(ITfThreadMgrEventSink *iface)
{
    ThreadMgrEventSink *This = impl_from_ITfThreadMgrEventSink(iface);
    ok (tmSinkRefCount == This->refCount,"ThreadMgrEventSink refcount off %i vs %i\n",This->refCount,tmSinkRefCount);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ThreadMgrEventSink_Release(ITfThreadMgrEventSink *iface)
{
    ThreadMgrEventSink *This = impl_from_ITfThreadMgrEventSink(iface);
    ULONG ret;

    ok (tmSinkRefCount == This->refCount,"ThreadMgrEventSink refcount off %i vs %i\n",This->refCount,tmSinkRefCount);
    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ThreadMgrEventSink_Destructor(This);
    return ret;
}

static HRESULT WINAPI ThreadMgrEventSink_OnInitDocumentMgr(ITfThreadMgrEventSink *iface,
ITfDocumentMgr *pdim)
{
    sink_fire_ok(&test_OnInitDocumentMgr,"ThreadMgrEventSink_OnInitDocumentMgr");
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnUninitDocumentMgr(ITfThreadMgrEventSink *iface,
ITfDocumentMgr *pdim)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnSetFocus(ITfThreadMgrEventSink *iface,
ITfDocumentMgr *pdimFocus, ITfDocumentMgr *pdimPrevFocus)
{
    sink_fire_ok(&test_OnSetFocus,"ThreadMgrEventSink_OnSetFocus");
    if (test_CurrentFocus == FOCUS_SAVE)
        test_LastCurrentFocus = pdimFocus;
    else if (test_CurrentFocus != FOCUS_IGNORE)
        ok(pdimFocus == test_CurrentFocus,"Sink reports wrong focus\n");
    if (test_PrevFocus == FOCUS_SAVE)
    {
        if (test_FirstPrevFocus == FOCUS_SAVE)
            test_FirstPrevFocus = pdimPrevFocus;
    }
    else if (test_PrevFocus != FOCUS_IGNORE)
        ok(pdimPrevFocus == test_PrevFocus,"Sink reports wrong previous focus\n");
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnPushContext(ITfThreadMgrEventSink *iface,
ITfContext *pic)
{
    HRESULT hr;
    ITfDocumentMgr *docmgr;
    ITfContext *test;

    hr = ITfContext_GetDocumentMgr(pic,&docmgr);
    ok(SUCCEEDED(hr),"GetDocumentMgr failed\n");
    test = (ITfContext*)0xdeadbeef;
    ITfDocumentMgr_Release(docmgr);
    hr = ITfDocumentMgr_GetTop(docmgr,&test);
    ok(SUCCEEDED(hr),"GetTop failed\n");
    ok(test == pic, "Wrong context is on top\n");
    if (test)
        ITfContext_Release(test);

    sink_fire_ok(&test_OnPushContext,"ThreadMgrEventSink_OnPushContext");
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnPopContext(ITfThreadMgrEventSink *iface,
ITfContext *pic)
{
    HRESULT hr;
    ITfDocumentMgr *docmgr;
    ITfContext *test;

    hr = ITfContext_GetDocumentMgr(pic,&docmgr);
    ok(SUCCEEDED(hr),"GetDocumentMgr failed\n");
    ITfDocumentMgr_Release(docmgr);
    test = (ITfContext*)0xdeadbeef;
    hr = ITfDocumentMgr_GetTop(docmgr,&test);
    ok(SUCCEEDED(hr),"GetTop failed\n");
    ok(test == pic, "Wrong context is on top\n");
    if (test)
        ITfContext_Release(test);

    sink_fire_ok(&test_OnPopContext,"ThreadMgrEventSink_OnPopContext");
    return S_OK;
}

static const ITfThreadMgrEventSinkVtbl ThreadMgrEventSink_ThreadMgrEventSinkVtbl =
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

static HRESULT ThreadMgrEventSink_Constructor(IUnknown **ppOut)
{
    ThreadMgrEventSink *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ThreadMgrEventSink));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ITfThreadMgrEventSink_iface.lpVtbl = &ThreadMgrEventSink_ThreadMgrEventSinkVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)This;
    return S_OK;
}


/********************************************************************************************
 * Stub text service for testing
 ********************************************************************************************/

static LONG TS_refCount;
static IClassFactory *cf;
static DWORD regid;

typedef HRESULT (*LPFNCONSTRUCTOR)(IUnknown *pUnkOuter, IUnknown **ppvOut);

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

typedef struct tagTextService
{
    ITfTextInputProcessor ITfTextInputProcessor_iface;
    LONG refCount;
} TextService;

static inline TextService *impl_from_ITfTextInputProcessor(ITfTextInputProcessor *iface)
{
    return CONTAINING_RECORD(iface, TextService, ITfTextInputProcessor_iface);
}

static void ClassFactory_Destructor(ClassFactory *This)
{
    HeapFree(GetProcessHeap(),0,This);
    TS_refCount--;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
    if (IsEqualIID(riid, &IID_IClassFactory) || IsEqualIID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *ppvOut = iface;
        return S_OK;
    }

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

    ret = This->ctor(punkOuter, &obj);
    if (FAILED(ret))
        return ret;
    ret = IUnknown_QueryInterface(obj, iid, ppvOut);
    IUnknown_Release(obj);
    return ret;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    if(fLock)
        InterlockedIncrement(&TS_refCount);
    else
        InterlockedDecrement(&TS_refCount);

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
    *ppvOut = (LPVOID)This;
    TS_refCount++;
    return S_OK;
}

static void TextService_Destructor(TextService *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI TextService_QueryInterface(ITfTextInputProcessor *iface, REFIID iid, LPVOID *ppvOut)
{
    TextService *This = impl_from_ITfTextInputProcessor(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfTextInputProcessor))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        ITfTextInputProcessor_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TextService_AddRef(ITfTextInputProcessor *iface)
{
    TextService *This = impl_from_ITfTextInputProcessor(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI TextService_Release(ITfTextInputProcessor *iface)
{
    TextService *This = impl_from_ITfTextInputProcessor(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        TextService_Destructor(This);
    return ret;
}

static HRESULT WINAPI TextService_Activate(ITfTextInputProcessor *iface,
        ITfThreadMgr *ptim, TfClientId id)
{
    trace("TextService_Activate\n");
    ok(test_ShouldActivate,"Activation came unexpectedly\n");
    tid = id;
    return S_OK;
}

static HRESULT WINAPI TextService_Deactivate(ITfTextInputProcessor *iface)
{
    trace("TextService_Deactivate\n");
    ok(test_ShouldDeactivate,"Deactivation came unexpectedly\n");
    return S_OK;
}

static const ITfTextInputProcessorVtbl TextService_TextInputProcessorVtbl=
{
    TextService_QueryInterface,
    TextService_AddRef,
    TextService_Release,

    TextService_Activate,
    TextService_Deactivate
};

static HRESULT TextService_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    TextService *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TextService));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ITfTextInputProcessor_iface.lpVtbl = &TextService_TextInputProcessorVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)This;
    return S_OK;
}

static HRESULT RegisterTextService(REFCLSID rclsid)
{
    ClassFactory_Constructor( TextService_Constructor ,(LPVOID*)&cf);
    return CoRegisterClassObject(rclsid, (IUnknown*) cf, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
}

static HRESULT UnregisterTextService(void)
{
    return CoRevokeClassObject(regid);
}

/*
 * The tests
 */

DEFINE_GUID(CLSID_FakeService, 0xEDE1A7AD,0x66DE,0x47E0,0xB6,0x20,0x3E,0x92,0xF8,0x24,0x6B,0xF3);
DEFINE_GUID(CLSID_TF_InputProcessorProfiles, 0x33c53a50,0xf456,0x4884,0xb0,0x49,0x85,0xfd,0x64,0x3e,0xcf,0xed);
DEFINE_GUID(CLSID_TF_CategoryMgr,         0xA4B544A1,0x438D,0x4B41,0x93,0x25,0x86,0x95,0x23,0xE2,0xD6,0xC7);
DEFINE_GUID(GUID_TFCAT_TIP_KEYBOARD,     0x34745c63,0xb2f0,0x4784,0x8b,0x67,0x5e,0x12,0xc8,0x70,0x1a,0x31);
DEFINE_GUID(GUID_TFCAT_TIP_SPEECH,       0xB5A73CD1,0x8355,0x426B,0xA1,0x61,0x25,0x98,0x08,0xF2,0x6B,0x14);
DEFINE_GUID(GUID_TFCAT_TIP_HANDWRITING,  0x246ecb87,0xc2f2,0x4abe,0x90,0x5b,0xc8,0xb3,0x8a,0xdd,0x2c,0x43);
DEFINE_GUID (GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,  0x046B8C80,0x1647,0x40F7,0x9B,0x21,0xB9,0x3B,0x81,0xAA,0xBC,0x1B);
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(CLSID_TF_ThreadMgr,           0x529a9e6b,0x6587,0x4f23,0xab,0x9e,0x9c,0x7d,0x68,0x3e,0x3c,0x50);
DEFINE_GUID(CLSID_PreservedKey,           0xA0ED8E55,0xCD3B,0x4274,0xB2,0x95,0xF6,0xC9,0xBA,0x2B,0x84,0x72);
DEFINE_GUID(GUID_COMPARTMENT_KEYBOARD_DISABLED,     0x71a5b253,0x1951,0x466b,0x9f,0xbc,0x9c,0x88,0x08,0xfa,0x84,0xf2);
DEFINE_GUID(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,    0x58273aad,0x01bb,0x4164,0x95,0xc6,0x75,0x5b,0xa0,0xb5,0x16,0x2d);
DEFINE_GUID(GUID_COMPARTMENT_HANDWRITING_OPENCLOSE, 0xf9ae2c6b,0x1866,0x4361,0xaf,0x72,0x7a,0xa3,0x09,0x48,0x89,0x0e);
DEFINE_GUID(GUID_COMPARTMENT_SPEECH_DISABLED,       0x56c5c607,0x0703,0x4e59,0x8e,0x52,0xcb,0xc8,0x4e,0x8b,0xbe,0x35);
DEFINE_GUID(GUID_COMPARTMENT_SPEECH_OPENCLOSE,      0x544d6a63,0xe2e8,0x4752,0xbb,0xd1,0x00,0x09,0x60,0xbc,0xa0,0x83);
DEFINE_GUID(GUID_COMPARTMENT_SPEECH_GLOBALSTATE,    0x2a54fe8e,0x0d08,0x460c,0xa7,0x5d,0x87,0x03,0x5f,0xf4,0x36,0xc5);
DEFINE_GUID(GUID_COMPARTMENT_PERSISTMENUENABLED,    0x575f3783,0x70c8,0x47c8,0xae,0x5d,0x91,0xa0,0x1a,0x1f,0x75,0x92);
DEFINE_GUID(GUID_COMPARTMENT_EMPTYCONTEXT,          0xd7487dbf,0x804e,0x41c5,0x89,0x4d,0xad,0x96,0xfd,0x4e,0xea,0x13);
DEFINE_GUID(GUID_COMPARTMENT_TIPUISTATUS,           0x148ca3ec,0x0366,0x401c,0x8d,0x75,0xed,0x97,0x8d,0x85,0xfb,0xc9);

static HRESULT initialize(void)
{
    HRESULT hr;
    CoInitialize(NULL);
    hr = CoCreateInstance (&CLSID_TF_InputProcessorProfiles, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfInputProcessorProfiles, (void**)&g_ipp);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance (&CLSID_TF_CategoryMgr, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfCategoryMgr, (void**)&g_cm);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance (&CLSID_TF_ThreadMgr, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfThreadMgr, (void**)&g_tm);
    return hr;
}

static void cleanup(void)
{
    if (g_ipp)
        ITfInputProcessorProfiles_Release(g_ipp);
    if (g_cm)
        ITfCategoryMgr_Release(g_cm);
    if (g_tm)
        ITfThreadMgr_Release(g_tm);
    CoUninitialize();
}

static void test_Register(void)
{
    HRESULT hr;

    static const WCHAR szDesc[] = {'F','a','k','e',' ','W','i','n','e',' ','S','e','r','v','i','c','e',0};
    static const WCHAR szFile[] = {'F','a','k','e',' ','W','i','n','e',' ','S','e','r','v','i','c','e',' ','F','i','l','e',0};

    hr = ITfInputProcessorProfiles_GetCurrentLanguage(g_ipp,&gLangid);
    ok(SUCCEEDED(hr),"Unable to get current language id\n");
    trace("Current Language %x\n",gLangid);

    hr = RegisterTextService(&CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to register COM for TextService\n");
    hr = ITfInputProcessorProfiles_Register(g_ipp, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to register text service(%x)\n",hr);
    hr = ITfInputProcessorProfiles_AddLanguageProfile(g_ipp, &CLSID_FakeService, gLangid, &CLSID_FakeService, szDesc, sizeof(szDesc)/sizeof(WCHAR), szFile, sizeof(szFile)/sizeof(WCHAR), 1);
    ok(SUCCEEDED(hr),"Unable to add Language Profile (%x)\n",hr);
}

static void test_Unregister(void)
{
    HRESULT hr;
    hr = ITfInputProcessorProfiles_Unregister(g_ipp, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to unregister text service(%x)\n",hr);
    UnregisterTextService();
}

static void test_EnumInputProcessorInfo(void)
{
    IEnumGUID *ppEnum;
    BOOL found = FALSE;

    if (SUCCEEDED(ITfInputProcessorProfiles_EnumInputProcessorInfo(g_ipp, &ppEnum)))
    {
        ULONG fetched;
        GUID g;
        while (IEnumGUID_Next(ppEnum, 1, &g, &fetched) == S_OK)
        {
            if(IsEqualGUID(&g,&CLSID_FakeService))
                found = TRUE;
        }
        IEnumGUID_Release(ppEnum);
    }
    ok(found,"Did not find registered text service\n");
}

static void test_EnumLanguageProfiles(void)
{
    BOOL found = FALSE;
    IEnumTfLanguageProfiles *ppEnum;
    if (SUCCEEDED(ITfInputProcessorProfiles_EnumLanguageProfiles(g_ipp,gLangid,&ppEnum)))
    {
        TF_LANGUAGEPROFILE profile;
        while (IEnumTfLanguageProfiles_Next(ppEnum,1,&profile,NULL)==S_OK)
        {
            if (IsEqualGUID(&profile.clsid,&CLSID_FakeService))
            {
                found = TRUE;
                ok(profile.langid == gLangid, "LangId Incorrect\n");
                ok(IsEqualGUID(&profile.catid,&GUID_TFCAT_TIP_KEYBOARD) ||
                   broken(IsEqualGUID(&profile.catid,&GUID_NULL) /* Win8 */), "CatId Incorrect\n");
                ok(IsEqualGUID(&profile.guidProfile,&CLSID_FakeService), "guidProfile Incorrect\n");
            }
        }
        IEnumTfLanguageProfiles_Release(ppEnum);
    }
    ok(found,"Registered text service not found\n");
}

static void test_RegisterCategory(void)
{
    HRESULT hr;
    hr = ITfCategoryMgr_RegisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_TIP_KEYBOARD, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_RegisterCategory failed\n");
    hr = ITfCategoryMgr_RegisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_RegisterCategory failed\n");
}

static void test_UnregisterCategory(void)
{
    HRESULT hr;
    hr = ITfCategoryMgr_UnregisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_TIP_KEYBOARD, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_UnregisterCategory failed\n");
    hr = ITfCategoryMgr_UnregisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_UnregisterCategory failed\n");
}

static void test_FindClosestCategory(void)
{
    GUID output;
    HRESULT hr;
    const GUID *list[3] = {&GUID_TFCAT_TIP_SPEECH, &GUID_TFCAT_TIP_KEYBOARD, &GUID_TFCAT_TIP_HANDWRITING};

    hr = ITfCategoryMgr_FindClosestCategory(g_cm, &CLSID_FakeService, &output, NULL, 0);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_FindClosestCategory failed (%x)\n",hr);
    ok(IsEqualGUID(&output,&GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER),"Wrong GUID\n");

    hr = ITfCategoryMgr_FindClosestCategory(g_cm, &CLSID_FakeService, &output, list, 1);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_FindClosestCategory failed (%x)\n",hr);
    ok(IsEqualGUID(&output,&GUID_NULL),"Wrong GUID\n");

    hr = ITfCategoryMgr_FindClosestCategory(g_cm, &CLSID_FakeService, &output, list, 3);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_FindClosestCategory failed (%x)\n",hr);
    ok(IsEqualGUID(&output,&GUID_TFCAT_TIP_KEYBOARD),"Wrong GUID\n");
}

static void test_Enable(void)
{
    HRESULT hr;
    BOOL enabled = FALSE;

    hr = ITfInputProcessorProfiles_EnableLanguageProfile(g_ipp,&CLSID_FakeService, gLangid, &CLSID_FakeService, TRUE);
    ok(SUCCEEDED(hr),"Failed to enable text service\n");
    hr = ITfInputProcessorProfiles_IsEnabledLanguageProfile(g_ipp,&CLSID_FakeService, gLangid, &CLSID_FakeService, &enabled);
    ok(SUCCEEDED(hr),"Failed to get enabled state\n");
    ok(enabled == TRUE,"enabled state incorrect\n");
}

static void test_Disable(void)
{
    HRESULT hr;

    trace("Disabling\n");
    hr = ITfInputProcessorProfiles_EnableLanguageProfile(g_ipp,&CLSID_FakeService, gLangid, &CLSID_FakeService, FALSE);
    ok(SUCCEEDED(hr),"Failed to disable text service\n");
}

static void test_ThreadMgrAdviseSinks(void)
{
    ITfSource *source = NULL;
    HRESULT hr;
    IUnknown *sink;

    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfSource, (LPVOID*)&source);
    ok(SUCCEEDED(hr),"Failed to get IID_ITfSource for ThreadMgr\n");
    if (!source)
        return;

    hr = ThreadMgrEventSink_Constructor(&sink);
    ok(hr == S_OK, "got %08x\n", hr);
    if(FAILED(hr)) return;

    tmSinkRefCount = 1;
    tmSinkCookie = 0;
    hr = ITfSource_AdviseSink(source,&IID_ITfThreadMgrEventSink, sink, &tmSinkCookie);
    ok(SUCCEEDED(hr),"Failed to Advise Sink\n");
    ok(tmSinkCookie!=0,"Failed to get sink cookie\n");

    /* Advising the sink adds a ref, Releasing here lets the object be deleted
       when unadvised */
    tmSinkRefCount = 2;
    IUnknown_Release(sink);
    ITfSource_Release(source);
}

static void test_ThreadMgrUnadviseSinks(void)
{
    ITfSource *source = NULL;
    HRESULT hr;

    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfSource, (LPVOID*)&source);
    ok(SUCCEEDED(hr),"Failed to get IID_ITfSource for ThreadMgr\n");
    if (!source)
        return;

    tmSinkRefCount = 1;
    hr = ITfSource_UnadviseSink(source, tmSinkCookie);
    ok(SUCCEEDED(hr),"Failed to unadvise Sink\n");
    ITfSource_Release(source);
}

/**********************************************************************
 * ITfKeyEventSink
 **********************************************************************/
typedef struct tagKeyEventSink
{
    ITfKeyEventSink ITfKeyEventSink_iface;
    LONG refCount;
} KeyEventSink;

static inline KeyEventSink *impl_from_ITfKeyEventSink(ITfKeyEventSink *iface)
{
    return CONTAINING_RECORD(iface, KeyEventSink, ITfKeyEventSink_iface);
}

static void KeyEventSink_Destructor(KeyEventSink *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI KeyEventSink_QueryInterface(ITfKeyEventSink *iface, REFIID iid, LPVOID *ppvOut)
{
    KeyEventSink *This = impl_from_ITfKeyEventSink(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfKeyEventSink))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        ITfKeyEventSink_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI KeyEventSink_AddRef(ITfKeyEventSink *iface)
{
    KeyEventSink *This = impl_from_ITfKeyEventSink(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI KeyEventSink_Release(ITfKeyEventSink *iface)
{
    KeyEventSink *This = impl_from_ITfKeyEventSink(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        KeyEventSink_Destructor(This);
    return ret;
}

static HRESULT WINAPI KeyEventSink_OnSetFocus(ITfKeyEventSink *iface,
        BOOL fForeground)
{
    sink_fire_ok(&test_KEV_OnSetFocus,"KeyEventSink_OnSetFocus");
    return S_OK;
}

static HRESULT WINAPI KeyEventSink_OnTestKeyDown(ITfKeyEventSink *iface,
        ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI KeyEventSink_OnTestKeyUp(ITfKeyEventSink *iface,
        ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI KeyEventSink_OnKeyDown(ITfKeyEventSink *iface,
        ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI KeyEventSink_OnKeyUp(ITfKeyEventSink *iface,
        ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten)
{
    trace("\n");
    return S_OK;
}

static HRESULT WINAPI KeyEventSink_OnPreservedKey(ITfKeyEventSink *iface,
    ITfContext *pic, REFGUID rguid, BOOL *pfEaten)
{
    trace("\n");
    return S_OK;
}

static const ITfKeyEventSinkVtbl KeyEventSink_KeyEventSinkVtbl =
{
    KeyEventSink_QueryInterface,
    KeyEventSink_AddRef,
    KeyEventSink_Release,

    KeyEventSink_OnSetFocus,
    KeyEventSink_OnTestKeyDown,
    KeyEventSink_OnTestKeyUp,
    KeyEventSink_OnKeyDown,
    KeyEventSink_OnKeyUp,
    KeyEventSink_OnPreservedKey
};

static HRESULT KeyEventSink_Constructor(ITfKeyEventSink **ppOut)
{
    KeyEventSink *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(KeyEventSink));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ITfKeyEventSink_iface.lpVtbl = &KeyEventSink_KeyEventSinkVtbl;
    This->refCount = 1;

    *ppOut = &This->ITfKeyEventSink_iface;
    return S_OK;
}


static void test_KeystrokeMgr(void)
{
    ITfKeystrokeMgr *keymgr= NULL;
    HRESULT hr;
    TF_PRESERVEDKEY tfpk;
    BOOL preserved;
    ITfKeyEventSink *sink = NULL;

    KeyEventSink_Constructor(&sink);

    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfKeystrokeMgr, (LPVOID*)&keymgr);
    ok(SUCCEEDED(hr),"Failed to get IID_ITfKeystrokeMgr for ThreadMgr\n");

    tfpk.uVKey = 'A';
    tfpk.uModifiers = TF_MOD_SHIFT;

    test_KEV_OnSetFocus = SINK_EXPECTED;
    hr = ITfKeystrokeMgr_AdviseKeyEventSink(keymgr,tid,sink,TRUE);
    ok(SUCCEEDED(hr),"ITfKeystrokeMgr_AdviseKeyEventSink failed\n");
    sink_check_ok(&test_KEV_OnSetFocus,"KeyEventSink_OnSetFocus");
    hr = ITfKeystrokeMgr_AdviseKeyEventSink(keymgr,tid,sink,TRUE);
    ok(hr == CONNECT_E_ADVISELIMIT,"Wrong return, expected CONNECT_E_ADVISELIMIT\n");
    hr = ITfKeystrokeMgr_AdviseKeyEventSink(keymgr,cid,sink,TRUE);
    ok(hr == E_INVALIDARG,"Wrong return, expected E_INVALIDARG\n");

    hr =ITfKeystrokeMgr_PreserveKey(keymgr, 0, &CLSID_PreservedKey, &tfpk, NULL, 0);
    ok(hr==E_INVALIDARG,"ITfKeystrokeMgr_PreserveKey inproperly succeeded\n");

    hr =ITfKeystrokeMgr_PreserveKey(keymgr, tid, &CLSID_PreservedKey, &tfpk, NULL, 0);
    ok(SUCCEEDED(hr),"ITfKeystrokeMgr_PreserveKey failed\n");

    hr =ITfKeystrokeMgr_PreserveKey(keymgr, tid, &CLSID_PreservedKey, &tfpk, NULL, 0);
    ok(hr == TF_E_ALREADY_EXISTS,"ITfKeystrokeMgr_PreserveKey inproperly succeeded\n");

    preserved = FALSE;
    hr = ITfKeystrokeMgr_IsPreservedKey(keymgr, &CLSID_PreservedKey, &tfpk, &preserved);
    ok(hr == S_OK, "ITfKeystrokeMgr_IsPreservedKey failed\n");
    if (hr == S_OK) ok(preserved == TRUE,"misreporting preserved key\n");

    hr = ITfKeystrokeMgr_UnpreserveKey(keymgr, &CLSID_PreservedKey,&tfpk);
    ok(SUCCEEDED(hr),"ITfKeystrokeMgr_UnpreserveKey failed\n");

    hr = ITfKeystrokeMgr_IsPreservedKey(keymgr, &CLSID_PreservedKey, &tfpk, &preserved);
    ok(hr == S_FALSE, "ITfKeystrokeMgr_IsPreservedKey failed\n");
    if (hr == S_FALSE) ok(preserved == FALSE,"misreporting preserved key\n");

    hr = ITfKeystrokeMgr_UnpreserveKey(keymgr, &CLSID_PreservedKey,&tfpk);
    ok(hr==CONNECT_E_NOCONNECTION,"ITfKeystrokeMgr_UnpreserveKey inproperly succeeded\n");

    hr = ITfKeystrokeMgr_UnadviseKeyEventSink(keymgr,tid);
    ok(SUCCEEDED(hr),"ITfKeystrokeMgr_UnadviseKeyEventSink failed\n");

    ITfKeystrokeMgr_Release(keymgr);
    ITfKeyEventSink_Release(sink);
}

static void test_Activate(void)
{
    HRESULT hr;

    test_ShouldActivate = TRUE; /* Win7 */
    hr = ITfInputProcessorProfiles_ActivateLanguageProfile(g_ipp,&CLSID_FakeService,gLangid,&CLSID_FakeService);
    ok(SUCCEEDED(hr),"Failed to Activate text service\n");
    test_ShouldActivate = FALSE;
}


static void test_EnumContexts(ITfDocumentMgr *dm, ITfContext *search)
{
    HRESULT hr;
    IEnumTfContexts* pEnum;
    BOOL found = FALSE;

    hr = ITfDocumentMgr_EnumContexts(dm,&pEnum);
    ok(SUCCEEDED(hr),"EnumContexts failed\n");
    if (SUCCEEDED(hr))
    {
        ULONG fetched;
        ITfContext *cxt;
        while (IEnumTfContexts_Next(pEnum, 1, &cxt, &fetched) == S_OK)
        {
            if (!search)
                found = TRUE;
            else if (search == cxt)
                found = TRUE;
            ITfContext_Release(cxt);
        }
        IEnumTfContexts_Release(pEnum);
    }
    if (search)
        ok(found,"Did not find proper ITfContext\n");
    else
        ok(!found,"Found an ITfContext we should should not have\n");
}

static void test_EnumDocumentMgr(ITfThreadMgr *tm, ITfDocumentMgr *search, ITfDocumentMgr *absent)
{
    HRESULT hr;
    IEnumTfDocumentMgrs* pEnum;
    BOOL found = FALSE;
    BOOL notfound = TRUE;

    hr = ITfThreadMgr_EnumDocumentMgrs(tm,&pEnum);
    ok(SUCCEEDED(hr),"EnumDocumentMgrs failed\n");
    if (SUCCEEDED(hr))
    {
        ULONG fetched;
        ITfDocumentMgr *dm;
        while (IEnumTfDocumentMgrs_Next(pEnum, 1, &dm, &fetched) == S_OK)
        {
            if (!search)
                found = TRUE;
            else if (search == dm)
                found = TRUE;
            if (absent && dm == absent)
                notfound = FALSE;
            ITfDocumentMgr_Release(dm);
        }
        IEnumTfDocumentMgrs_Release(pEnum);
    }
    if (search)
        ok(found,"Did not find proper ITfDocumentMgr\n");
    else
        ok(!found,"Found an ITfDocumentMgr we should should not have\n");
    if (absent)
        ok(notfound,"Found an ITfDocumentMgr we believe should be absent\n");
}

static inline int check_context_refcount(ITfContext *iface)
{
    ITfContext_AddRef(iface);
    return ITfContext_Release(iface);
}


/**********************************************************************
 * ITfTextEditSink
 **********************************************************************/
typedef struct tagTextEditSink
{
    ITfTextEditSink ITfTextEditSink_iface;
    LONG refCount;
} TextEditSink;

static inline TextEditSink *impl_from_ITfTextEditSink(ITfTextEditSink *iface)
{
    return CONTAINING_RECORD(iface, TextEditSink, ITfTextEditSink_iface);
}

static void TextEditSink_Destructor(TextEditSink *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI TextEditSink_QueryInterface(ITfTextEditSink *iface, REFIID iid, LPVOID *ppvOut)
{
    TextEditSink *This = impl_from_ITfTextEditSink(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfTextEditSink))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        ITfTextEditSink_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TextEditSink_AddRef(ITfTextEditSink *iface)
{
    TextEditSink *This = impl_from_ITfTextEditSink(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI TextEditSink_Release(ITfTextEditSink *iface)
{
    TextEditSink *This = impl_from_ITfTextEditSink(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        TextEditSink_Destructor(This);
    return ret;
}

static HRESULT WINAPI TextEditSink_OnEndEdit(ITfTextEditSink *iface,
    ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord)
{
    sink_fire_ok(&test_OnEndEdit,"TextEditSink_OnEndEdit");
    return S_OK;
}

static const ITfTextEditSinkVtbl TextEditSink_TextEditSinkVtbl =
{
    TextEditSink_QueryInterface,
    TextEditSink_AddRef,
    TextEditSink_Release,

    TextEditSink_OnEndEdit
};

static HRESULT TextEditSink_Constructor(ITfTextEditSink **ppOut)
{
    TextEditSink *This;

    *ppOut = NULL;
    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TextEditSink));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ITfTextEditSink_iface.lpVtbl = &TextEditSink_TextEditSinkVtbl;
    This->refCount = 1;

    *ppOut = &This->ITfTextEditSink_iface;
    return S_OK;
}

static void test_startSession(void)
{
    HRESULT hr;
    DWORD cnt;
    DWORD editCookie;
    ITfDocumentMgr *dmtest;
    ITfContext *cxt,*cxt2,*cxt3,*cxtTest;
    ITextStoreACP *ts;
    TfClientId cid2 = 0;
    ITfThreadMgrEx *tmex;

    hr = ITfThreadMgr_Deactivate(g_tm);
    ok(hr == E_UNEXPECTED,"Deactivate should have failed with E_UNEXPECTED\n");

    test_ShouldActivate = TRUE;
    hr  = ITfThreadMgr_Activate(g_tm,&cid);
    ok(SUCCEEDED(hr),"Failed to Activate\n");
    ok(cid != tid,"TextService id mistakenly matches Client id\n");

    test_ShouldActivate = FALSE;
    hr = ITfThreadMgr_Activate(g_tm,&cid2);
    ok(SUCCEEDED(hr),"Failed to Activate\n");
    ok(cid == cid2, "Second activate client ID does not match\n");

    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfThreadMgrEx, (void **)&tmex);
    ok(SUCCEEDED(hr), "Unable to acquire ITfThreadMgrEx interface\n");

    hr = ITfThreadMgrEx_ActivateEx(tmex, &cid2, 0);
    ok(SUCCEEDED(hr), "Failed to Activate\n");
    ok(cid == cid2, "ActivateEx client ID does not match\n");

    ITfThreadMgrEx_Release(tmex);

    hr = ITfThreadMgr_Deactivate(g_tm);
    ok(SUCCEEDED(hr), "Failed to Deactivate\n");
    hr = ITfThreadMgr_Deactivate(g_tm);
    ok(SUCCEEDED(hr), "Failed to Deactivate\n");

    test_EnumDocumentMgr(g_tm,NULL,NULL);

    hr = ITfThreadMgr_CreateDocumentMgr(g_tm,&g_dm);
    ok(SUCCEEDED(hr),"CreateDocumentMgr failed\n");

    test_EnumDocumentMgr(g_tm,g_dm,NULL);

    hr = ITfThreadMgr_CreateDocumentMgr(g_tm,&dmtest);
    ok(SUCCEEDED(hr),"CreateDocumentMgr failed\n");

    test_EnumDocumentMgr(g_tm,dmtest,NULL);

    ITfDocumentMgr_Release(dmtest);
    test_EnumDocumentMgr(g_tm,g_dm,dmtest);

    hr = ITfThreadMgr_GetFocus(g_tm,&dmtest);
    ok(SUCCEEDED(hr),"GetFocus Failed\n");
    ok(dmtest == NULL,"Initial focus not null\n");

    test_CurrentFocus = g_dm;
    test_PrevFocus = NULL;
    test_OnSetFocus  = SINK_OPTIONAL; /* Doesn't always fire on Win7 */
    hr = ITfThreadMgr_SetFocus(g_tm,g_dm);
    ok(SUCCEEDED(hr),"SetFocus Failed\n");
    sink_check_ok(&test_OnSetFocus,"OnSetFocus");

    hr = ITfThreadMgr_GetFocus(g_tm,&dmtest);
    ok(SUCCEEDED(hr),"GetFocus Failed\n");
    ok(g_dm == dmtest,"Expected DocumentMgr not focused\n");

    ITfDocumentMgr_Release(g_dm);

    hr = ITfThreadMgr_GetFocus(g_tm,&dmtest);
    ok(SUCCEEDED(hr),"GetFocus Failed\n");
    ok(g_dm == dmtest,"Expected DocumentMgr not focused\n");
    ITfDocumentMgr_Release(dmtest);

    hr = TextStoreACP_Constructor((IUnknown**)&ts);
    if (SUCCEEDED(hr))
    {
        hr = ITfDocumentMgr_CreateContext(g_dm, cid, 0, (IUnknown*)ts, &cxt, &editCookie);
        ok(SUCCEEDED(hr),"CreateContext Failed\n");
    }

    hr = ITfDocumentMgr_CreateContext(g_dm, cid, 0, NULL, &cxt2, &editCookie);
    ok(SUCCEEDED(hr),"CreateContext Failed\n");

    hr = ITfDocumentMgr_CreateContext(g_dm, cid, 0, NULL, &cxt3, &editCookie);
    ok(SUCCEEDED(hr),"CreateContext Failed\n");

    test_EnumContexts(g_dm, NULL);

    hr = ITfContext_GetDocumentMgr(cxt,&dmtest);
    ok(hr == S_OK, "ITfContext_GetDocumentMgr failed with %x\n",hr);
    ok(dmtest == g_dm, "Wrong documentmgr\n");
    ITfDocumentMgr_Release(dmtest);

    cnt = check_context_refcount(cxt);
    test_OnPushContext = SINK_EXPECTED;
    test_ACP_AdviseSink = SINK_EXPECTED;
    test_OnInitDocumentMgr = SINK_EXPECTED;
    hr = ITfDocumentMgr_Push(g_dm, cxt);
    ok(SUCCEEDED(hr),"Push Failed\n");
    ok(check_context_refcount(cxt) > cnt, "Ref count did not increase\n");
    sink_check_ok(&test_OnPushContext,"OnPushContext");
    sink_check_ok(&test_OnInitDocumentMgr,"OnInitDocumentMgr");
    sink_check_ok(&test_ACP_AdviseSink,"TextStoreACP_AdviseSink");

    test_EnumContexts(g_dm, cxt);

    hr = ITfDocumentMgr_GetTop(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetTop Failed\n");
    ok(cxtTest == cxt, "Wrong context on top\n");
    ok(check_context_refcount(cxt) > cnt, "Ref count did not increase\n");
    cnt = ITfContext_Release(cxtTest);

    hr = ITfDocumentMgr_GetBase(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetBase Failed\n");
    ok(cxtTest == cxt, "Wrong context on Base\n");
    ok(check_context_refcount(cxt) > cnt, "Ref count did not increase\n");
    ITfContext_Release(cxtTest);

    check_context_refcount(cxt2);
    test_OnPushContext = SINK_EXPECTED;
    hr = ITfDocumentMgr_Push(g_dm, cxt2);
    ok(SUCCEEDED(hr),"Push Failed\n");
    sink_check_ok(&test_OnPushContext,"OnPushContext");

    cnt = check_context_refcount(cxt2);
    hr = ITfDocumentMgr_GetTop(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetTop Failed\n");
    ok(cxtTest == cxt2, "Wrong context on top\n");
    ok(check_context_refcount(cxt2) > cnt, "Ref count did not increase\n");
    ITfContext_Release(cxtTest);

    cnt = check_context_refcount(cxt);
    hr = ITfDocumentMgr_GetBase(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetBase Failed\n");
    ok(cxtTest == cxt, "Wrong context on Base\n");
    ok(check_context_refcount(cxt) > cnt, "Ref count did not increase\n");
    ITfContext_Release(cxtTest);

    cnt = check_context_refcount(cxt3);
    hr = ITfDocumentMgr_Push(g_dm, cxt3);
    ok(FAILED(hr),"Push Succeeded\n");
    ok(check_context_refcount(cxt3) == cnt, "Ref changed\n");

    cnt = check_context_refcount(cxt2);
    hr = ITfDocumentMgr_GetTop(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetTop Failed\n");
    ok(cxtTest == cxt2, "Wrong context on top\n");
    ok(check_context_refcount(cxt2) > cnt, "Ref count did not increase\n");
    ITfContext_Release(cxtTest);

    cnt = check_context_refcount(cxt);
    hr = ITfDocumentMgr_GetBase(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetBase Failed\n");
    ok(cxtTest == cxt, "Wrong context on Base\n");
    ok(check_context_refcount(cxt) > cnt, "Ref count did not increase\n");
    ITfContext_Release(cxtTest);

    cnt = check_context_refcount(cxt2);
    test_OnPopContext = SINK_EXPECTED;
    hr = ITfDocumentMgr_Pop(g_dm, 0);
    ok(SUCCEEDED(hr),"Pop Failed\n");
    ok(check_context_refcount(cxt2) < cnt, "Ref count did not decrease\n");
    sink_check_ok(&test_OnPopContext,"OnPopContext");

    dmtest = (void *)0xfeedface;
    hr = ITfContext_GetDocumentMgr(cxt2,&dmtest);
    ok(hr == S_FALSE, "ITfContext_GetDocumentMgr wrong rc %x\n",hr);
    ok(dmtest == NULL,"returned documentmgr should be null\n");

    hr = ITfDocumentMgr_GetTop(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetTop Failed\n");
    ok(cxtTest == cxt, "Wrong context on top\n");
    ITfContext_Release(cxtTest);

    hr = ITfDocumentMgr_GetBase(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetBase Failed\n");
    ok(cxtTest == cxt, "Wrong context on base\n");
    ITfContext_Release(cxtTest);

    hr = ITfDocumentMgr_Pop(g_dm, 0);
    ok(FAILED(hr),"Pop Succeeded\n");

    hr = ITfDocumentMgr_GetTop(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetTop Failed\n");
    ok(cxtTest == cxt, "Wrong context on top\n");
    ITfContext_Release(cxtTest);

    hr = ITfDocumentMgr_GetBase(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetBase Failed\n");
    ok(cxtTest == cxt, "Wrong context on base\n");
    ITfContext_Release(cxtTest);

    ITfContext_Release(cxt);
    ITfContext_Release(cxt2);
    ITfContext_Release(cxt3);
}

static void test_endSession(void)
{
    HRESULT hr;
    test_ShouldDeactivate = TRUE;
    test_CurrentFocus = NULL;
    test_PrevFocus = g_dm;
    test_OnSetFocus  = SINK_OPTIONAL; /* Doesn't fire on Win7 */
    hr = ITfThreadMgr_Deactivate(g_tm);
    ok(SUCCEEDED(hr),"Failed to Deactivate\n");
    sink_check_ok(&test_OnSetFocus,"OnSetFocus");
    test_OnSetFocus  = SINK_UNEXPECTED;
}

static void test_TfGuidAtom(void)
{
    GUID gtest,g1;
    HRESULT hr;
    TfGuidAtom atom1,atom2;
    BOOL equal;

    CoCreateGuid(&gtest);

    /* msdn reports this should return E_INVALIDARG.  However my test show it crashing (winxp)*/
    /*
    hr = ITfCategoryMgr_RegisterGUID(g_cm,&gtest,NULL);
    ok(hr==E_INVALIDARG,"ITfCategoryMgr_RegisterGUID should have failed\n");
    */
    hr = ITfCategoryMgr_RegisterGUID(g_cm,&gtest,&atom1);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_RegisterGUID failed\n");
    hr = ITfCategoryMgr_RegisterGUID(g_cm,&gtest,&atom2);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_RegisterGUID failed\n");
    ok(atom1 == atom2,"atoms do not match\n");
    hr = ITfCategoryMgr_GetGUID(g_cm,atom2,NULL);
    ok(hr==E_INVALIDARG,"ITfCategoryMgr_GetGUID should have failed\n");
    hr = ITfCategoryMgr_GetGUID(g_cm,atom2,&g1);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_GetGUID failed\n");
    ok(IsEqualGUID(&g1,&gtest),"guids do not match\n");
    hr = ITfCategoryMgr_IsEqualTfGuidAtom(g_cm,atom1,&gtest,NULL);
    ok(hr==E_INVALIDARG,"ITfCategoryMgr_IsEqualTfGuidAtom should have failed\n");
    hr = ITfCategoryMgr_IsEqualTfGuidAtom(g_cm,atom1,&gtest,&equal);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_IsEqualTfGuidAtom failed\n");
    ok(equal == TRUE,"Equal value invalid\n");

    /* show that cid and tid TfClientIds are also TfGuidAtoms */
    hr = ITfCategoryMgr_IsEqualTfGuidAtom(g_cm,tid,&CLSID_FakeService,&equal);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_IsEqualTfGuidAtom failed\n");
    ok(equal == TRUE,"Equal value invalid\n");
    hr = ITfCategoryMgr_GetGUID(g_cm,cid,&g1);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_GetGUID failed\n");
    ok(!IsEqualGUID(&g1,&GUID_NULL),"guid should not be NULL\n");
}

static void test_ClientId(void)
{
    ITfClientId *pcid;
    TfClientId id1,id2;
    HRESULT hr;
    GUID g2;

    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfClientId, (LPVOID*)&pcid);
    ok(SUCCEEDED(hr),"Unable to acquire ITfClientId interface\n");

    CoCreateGuid(&g2);

    hr = ITfClientId_GetClientId(pcid,&GUID_NULL,&id1);
    ok(SUCCEEDED(hr),"GetClientId failed\n");
    hr = ITfClientId_GetClientId(pcid,&GUID_NULL,&id2);
    ok(SUCCEEDED(hr),"GetClientId failed\n");
    ok(id1==id2,"Id's for GUID_NULL do not match\n");
    hr = ITfClientId_GetClientId(pcid,&CLSID_FakeService,&id2);
    ok(SUCCEEDED(hr),"GetClientId failed\n");
    ok(id2!=id1,"Id matches GUID_NULL\n");
    ok(id2==tid,"Id for CLSID_FakeService not matching tid\n");
    ok(id2!=cid,"Id for CLSID_FakeService matching cid\n");
    hr = ITfClientId_GetClientId(pcid,&g2,&id2);
    ok(SUCCEEDED(hr),"GetClientId failed\n");
    ok(id2!=id1,"Id matches GUID_NULL\n");
    ok(id2!=tid,"Id for random guid matching tid\n");
    ok(id2!=cid,"Id for random guid matching cid\n");
    ITfClientId_Release(pcid);
}

/**********************************************************************
 * ITfEditSession
 **********************************************************************/
typedef struct tagEditSession
{
    ITfEditSession ITfEditSession_iface;
    LONG refCount;
} EditSession;

static inline EditSession *impl_from_ITfEditSession(ITfEditSession *iface)
{
    return CONTAINING_RECORD(iface, EditSession, ITfEditSession_iface);
}

static void EditSession_Destructor(EditSession *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI EditSession_QueryInterface(ITfEditSession *iface, REFIID iid, LPVOID *ppvOut)
{
    EditSession *This = impl_from_ITfEditSession(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfEditSession))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        ITfEditSession_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI EditSession_AddRef(ITfEditSession *iface)
{
    EditSession *This = impl_from_ITfEditSession(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI EditSession_Release(ITfEditSession *iface)
{
    EditSession *This = impl_from_ITfEditSession(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        EditSession_Destructor(This);
    return ret;
}

static void test_InsertAtSelection(TfEditCookie ec, ITfContext *cxt)
{
    HRESULT hr;
    ITfInsertAtSelection *iis;
    ITfRange *range=NULL;
    static const WCHAR txt[] = {'H','e','l','l','o',' ','W','o','r','l','d',0};

    hr = ITfContext_QueryInterface(cxt, &IID_ITfInsertAtSelection , (LPVOID*)&iis);
    ok(SUCCEEDED(hr),"Failed to get ITfInsertAtSelection interface\n");
    test_ACP_InsertTextAtSelection = SINK_EXPECTED;
    hr = ITfInsertAtSelection_InsertTextAtSelection(iis, ec, 0, txt, 11, &range);
    ok(SUCCEEDED(hr),"ITfInsertAtSelection_InsertTextAtSelection failed %x\n",hr);
    sink_check_ok(&test_ACP_InsertTextAtSelection,"InsertTextAtSelection");
    ok(range != NULL,"No range returned\n");
    ITfRange_Release(range);
    ITfInsertAtSelection_Release(iis);
}

static HRESULT WINAPI EditSession_DoEditSession(ITfEditSession *iface,
TfEditCookie ec)
{
    ITfContext *cxt;
    ITfDocumentMgr *dm;
    ITfRange *range;
    TF_SELECTION selection;
    ULONG fetched;
    HRESULT hr;

    sink_fire_ok(&test_DoEditSession,"EditSession_DoEditSession");
    sink_check_ok(&test_ACP_RequestLock,"RequestLock");

    ITfThreadMgr_GetFocus(g_tm, &dm);
    ITfDocumentMgr_GetTop(dm,&cxt);

    hr = ITfContext_GetStart(cxt,ec,NULL);
    ok(hr == E_INVALIDARG,"Unexpected return code %x\n",hr);

    range = (ITfRange*)0xdeaddead;
    hr = ITfContext_GetStart(cxt,0xdeadcafe,&range);
    ok(hr == TF_E_NOLOCK,"Unexpected return code %x\n",hr);
    ok(range == NULL,"Range not set to NULL\n");

    hr = ITfContext_GetStart(cxt,ec,&range);
    ok(SUCCEEDED(hr),"Unexpected return code %x\n",hr);
    ok(range != NULL,"Range set to NULL\n");

    ITfRange_Release(range);

    hr = ITfContext_GetEnd(cxt,ec,NULL);
    ok(hr == E_INVALIDARG,"Unexpected return code %x\n",hr);

    range = (ITfRange*)0xdeaddead;
    hr = ITfContext_GetEnd(cxt,0xdeadcafe,&range);
    ok(hr == TF_E_NOLOCK,"Unexpected return code %x\n",hr);
    ok(range == NULL,"Range not set to NULL\n");

    test_ACP_GetEndACP = SINK_EXPECTED;
    hr = ITfContext_GetEnd(cxt,ec,&range);
    ok(SUCCEEDED(hr),"Unexpected return code %x\n",hr);
    ok(range != NULL,"Range set to NULL\n");
    sink_check_ok(&test_ACP_GetEndACP,"GetEndACP");

    ITfRange_Release(range);

    selection.range = NULL;
    test_ACP_GetSelection = SINK_EXPECTED;
    hr = ITfContext_GetSelection(cxt, ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
    ok(SUCCEEDED(hr),"ITfContext_GetSelection failed\n");
    ok(fetched == 1,"fetched incorrect\n");
    ok(selection.range != NULL,"NULL range\n");
    sink_check_ok(&test_ACP_GetSelection,"ACP_GetSepection");
    ITfRange_Release(selection.range);

    test_InsertAtSelection(ec, cxt);

    test_ACP_GetEndACP = SINK_EXPECTED;
    hr = ITfContext_GetEnd(cxt,ec,&range);
    ok(SUCCEEDED(hr),"Unexpected return code %x\n",hr);
    ok(range != NULL,"Range set to NULL\n");
    sink_check_ok(&test_ACP_GetEndACP,"GetEndACP");

    selection.range = range;
    selection.style.ase = TF_AE_NONE;
    selection.style.fInterimChar = FALSE;
    test_ACP_SetSelection = SINK_EXPECTED;
    hr = ITfContext_SetSelection(cxt, ec, 1, &selection);
    ok(SUCCEEDED(hr),"ITfContext_SetSelection failed\n");
    sink_check_ok(&test_ACP_SetSelection,"SetSelection");
    ITfRange_Release(range);

    ITfContext_Release(cxt);
    ITfDocumentMgr_Release(dm);
    return 0xdeadcafe;
}

static const ITfEditSessionVtbl EditSession_EditSessionVtbl =
{
    EditSession_QueryInterface,
    EditSession_AddRef,
    EditSession_Release,

    EditSession_DoEditSession
};

static HRESULT EditSession_Constructor(ITfEditSession **ppOut)
{
    EditSession *This;

    *ppOut = NULL;
    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(EditSession));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ITfEditSession_iface.lpVtbl = &EditSession_EditSessionVtbl;
    This->refCount = 1;

    *ppOut = &This->ITfEditSession_iface;
    return S_OK;
}

static void test_TStoApplicationText(void)
{
    HRESULT hr, hrSession;
    ITfEditSession *es;
    ITfContext *cxt;
    ITfDocumentMgr *dm;
    ITfTextEditSink *sink;
    ITfSource *source = NULL;
    DWORD editSinkCookie = -1;

    ITfThreadMgr_GetFocus(g_tm, &dm);
    EditSession_Constructor(&es);
    ITfDocumentMgr_GetTop(dm,&cxt);

    TextEditSink_Constructor(&sink);
    hr = ITfContext_QueryInterface(cxt,&IID_ITfSource,(LPVOID*)&source);
    ok(SUCCEEDED(hr),"Failed to get IID_ITfSource for Context\n");
    if (source)
    {
        hr = ITfSource_AdviseSink(source, &IID_ITfTextEditSink, (LPVOID)sink, &editSinkCookie);
        ok(SUCCEEDED(hr),"Failed to advise Sink\n");
        ok(editSinkCookie != -1,"Failed to get sink cookie\n");
    }

    hrSession = 0xfeedface;
    /* Test no permissions flags */
    hr = ITfContext_RequestEditSession(cxt, tid, es, TF_ES_SYNC, &hrSession);
    ok(hr == E_INVALIDARG,"RequestEditSession should have failed with %x not %x\n",E_INVALIDARG,hr);
    ok(hrSession == E_FAIL,"hrSession should be %x not %x\n",E_FAIL,hrSession);

    documentStatus = TS_SD_READONLY;
    hrSession = 0xfeedface;
    test_ACP_GetStatus = SINK_EXPECTED;
    hr = ITfContext_RequestEditSession(cxt, tid, es, TF_ES_SYNC|TF_ES_READWRITE, &hrSession);
    ok(SUCCEEDED(hr),"ITfContext_RequestEditSession failed\n");
    ok(hrSession == TS_E_READONLY,"Unexpected hrSession (%x)\n",hrSession);
    sink_check_ok(&test_ACP_GetStatus,"GetStatus");

    /* signal a change to allow readwrite sessions */
    documentStatus = 0;
    test_ACP_RequestLock = SINK_EXPECTED;
    ITextStoreACPSink_OnStatusChange(ACPSink,documentStatus);
    sink_check_ok(&test_ACP_RequestLock,"RequestLock");

    test_ACP_GetStatus = SINK_EXPECTED;
    test_ACP_RequestLock = SINK_EXPECTED;
    test_DoEditSession = SINK_EXPECTED;
    hrSession = 0xfeedface;
    test_OnEndEdit = SINK_EXPECTED;
    hr = ITfContext_RequestEditSession(cxt, tid, es, TF_ES_SYNC|TF_ES_READWRITE, &hrSession);
    ok(SUCCEEDED(hr),"ITfContext_RequestEditSession failed\n");
    sink_check_ok(&test_OnEndEdit,"OnEndEdit");
    sink_check_ok(&test_DoEditSession,"DoEditSession");
    sink_check_ok(&test_ACP_GetStatus,"GetStatus");
    ok(hrSession == 0xdeadcafe,"Unexpected hrSession (%x)\n",hrSession);

    if (source)
    {
        hr = ITfSource_UnadviseSink(source, editSinkCookie);
        ok(SUCCEEDED(hr),"Failed to unadvise Sink\n");
        ITfSource_Release(source);
    }
    ITfTextEditSink_Release(sink);
    ITfContext_Release(cxt);
    ITfDocumentMgr_Release(dm);
    ITfEditSession_Release(es);
}

static void enum_compartments(ITfCompartmentMgr *cmpmgr, REFGUID present, REFGUID absent)
{
    BOOL found,found2;
    IEnumGUID *ppEnum;
    found = FALSE;
    found2 = FALSE;
    if (SUCCEEDED(ITfCompartmentMgr_EnumCompartments(cmpmgr, &ppEnum)))
    {
        ULONG fetched;
        GUID g;
        while (IEnumGUID_Next(ppEnum, 1, &g, &fetched) == S_OK)
        {
            WCHAR str[50];
            CHAR strA[50];
            StringFromGUID2(&g,str,sizeof(str)/sizeof(str[0]));
            WideCharToMultiByte(CP_ACP,0,str,-1,strA,sizeof(strA),0,0);
            trace("found %s\n",strA);
            if (present && IsEqualGUID(present,&g))
                found = TRUE;
            if (absent && IsEqualGUID(absent, &g))
                found2 = TRUE;
        }
        IEnumGUID_Release(ppEnum);
    }
    if (present)
        ok(found,"Did not find compartment\n");
    if (absent)
        ok(!found2,"Found compartment that should be absent\n");
}

static void test_Compartments(void)
{
    ITfContext *cxt;
    ITfDocumentMgr *dm;
    ITfCompartmentMgr *cmpmgr;
    ITfCompartment *cmp;
    HRESULT hr;

    ITfThreadMgr_GetFocus(g_tm, &dm);
    ITfDocumentMgr_GetTop(dm,&cxt);

    /* Global */
    hr = ITfThreadMgr_GetGlobalCompartment(g_tm, &cmpmgr);
    ok(SUCCEEDED(hr),"GetGlobalCompartment failed\n");
    hr = ITfCompartmentMgr_GetCompartment(cmpmgr, &GUID_COMPARTMENT_SPEECH_OPENCLOSE, &cmp);
    ok(SUCCEEDED(hr),"GetCompartment failed\n");
    ITfCompartment_Release(cmp);
    enum_compartments(cmpmgr,&GUID_COMPARTMENT_SPEECH_OPENCLOSE,NULL);
    ITfCompartmentMgr_Release(cmpmgr);

    /* Thread */
    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfCompartmentMgr, (LPVOID*)&cmpmgr);
    ok(SUCCEEDED(hr),"ThreadMgr QI for IID_ITfCompartmentMgr failed\n");
    hr = ITfCompartmentMgr_GetCompartment(cmpmgr, &CLSID_FakeService, &cmp);
    ok(SUCCEEDED(hr),"GetCompartment failed\n");
    enum_compartments(cmpmgr,&CLSID_FakeService,&GUID_COMPARTMENT_SPEECH_OPENCLOSE);
    ITfCompartmentMgr_ClearCompartment(cmpmgr,tid,&CLSID_FakeService);
    enum_compartments(cmpmgr,NULL,&CLSID_FakeService);
    ITfCompartmentMgr_Release(cmpmgr);
    ITfCompartment_Release(cmp);

    /* DocumentMgr */
    hr = ITfDocumentMgr_QueryInterface(dm, &IID_ITfCompartmentMgr, (LPVOID*)&cmpmgr);
    ok(SUCCEEDED(hr),"DocumentMgr QI for IID_ITfCompartmentMgr failed\n");

    hr = ITfCompartmentMgr_GetCompartment(cmpmgr, &GUID_COMPARTMENT_PERSISTMENUENABLED, &cmp);
    ok(SUCCEEDED(hr),"GetCompartment failed\n");
    enum_compartments(cmpmgr,&GUID_COMPARTMENT_PERSISTMENUENABLED,&GUID_COMPARTMENT_SPEECH_OPENCLOSE);
    ITfCompartmentMgr_Release(cmpmgr);

    /* Context */
    hr = ITfContext_QueryInterface(cxt, &IID_ITfCompartmentMgr, (LPVOID*)&cmpmgr);
    ok(SUCCEEDED(hr),"Context QI for IID_ITfCompartmentMgr failed\n");
    enum_compartments(cmpmgr,NULL,&GUID_COMPARTMENT_PERSISTMENUENABLED);
    ITfCompartmentMgr_Release(cmpmgr);

    ITfContext_Release(cxt);
    ITfDocumentMgr_Release(dm);
}

static void processPendingMessages(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        diff = time - GetTickCount();
    }
}

static void test_AssociateFocus(void)
{
    ITfDocumentMgr *dm1, *dm2, *olddm, *dmcheck, *dmorig;
    HWND wnd1, wnd2, wnd3;
    HRESULT hr;

    ITfThreadMgr_GetFocus(g_tm, &dmorig);
    test_CurrentFocus = NULL;
    test_PrevFocus = dmorig;
    test_OnSetFocus  = SINK_OPTIONAL; /* Doesn't always fire on Win7 */
    test_ACP_GetStatus = SINK_OPTIONAL;
    hr = ITfThreadMgr_SetFocus(g_tm,NULL);
    ok(SUCCEEDED(hr),"ITfThreadMgr_SetFocus failed\n");
    sink_check_ok(&test_OnSetFocus,"OnSetFocus");
    test_ACP_GetStatus = SINK_UNEXPECTED;
    ITfDocumentMgr_Release(dmorig);

    hr = ITfThreadMgr_CreateDocumentMgr(g_tm,&dm1);
    ok(SUCCEEDED(hr),"CreateDocumentMgr failed\n");

    hr = ITfThreadMgr_CreateDocumentMgr(g_tm,&dm2);
    ok(SUCCEEDED(hr),"CreateDocumentMgr failed\n");

    wnd1 = CreateWindowA("edit",NULL,WS_POPUP,0,0,200,60,NULL,NULL,NULL,NULL);
    ok(wnd1!=NULL,"Unable to create window 1\n");
    wnd2 = CreateWindowA("edit",NULL,WS_POPUP,0,65,200,60,NULL,NULL,NULL,NULL);
    ok(wnd2!=NULL,"Unable to create window 2\n");
    wnd3 = CreateWindowA("edit",NULL,WS_POPUP,0,130,200,60,NULL,NULL,NULL,NULL);
    ok(wnd3!=NULL,"Unable to create window 3\n");

    processPendingMessages();

    test_OnInitDocumentMgr = SINK_OPTIONAL; /* Vista and greater */
    test_OnPushContext = SINK_OPTIONAL; /* Vista and greater */
    test_OnSetFocus = SINK_OPTIONAL; /* Win7 */
    test_PrevFocus = NULL;
    test_CurrentFocus = FOCUS_IGNORE;

    ShowWindow(wnd1,SW_SHOWNORMAL);
    test_OnSetFocus = SINK_UNEXPECTED;
    SetFocus(wnd1);
    sink_check_ok(&test_OnInitDocumentMgr,"OnInitDocumentMgr");
    sink_check_ok(&test_OnPushContext,"OnPushContext");

    test_OnSetFocus  = SINK_OPTIONAL; /* Vista and greater */
    test_ACP_RequestLock = SINK_OPTIONAL; /* Win7 x64 */
    test_ACP_GetSelection = SINK_OPTIONAL; /* Win7 x64 */
    ITfThreadMgr_GetFocus(g_tm, &test_PrevFocus);
    test_CurrentFocus = FOCUS_IGNORE; /* This is a default system context */
    processPendingMessages();
    sink_check_ok(&test_OnSetFocus,"OnSetFocus");
    test_ACP_RequestLock = SINK_UNEXPECTED;
    test_ACP_GetSelection = SINK_UNEXPECTED;

    test_CurrentFocus = dm1;
    test_PrevFocus = FOCUS_IGNORE;
    test_OnSetFocus  = SINK_OPTIONAL;
    test_ShouldDeactivate = SINK_OPTIONAL;
    hr = ITfThreadMgr_AssociateFocus(g_tm,wnd1,dm1,&olddm);
    ok(SUCCEEDED(hr),"AssociateFocus failed\n");
    sink_check_ok(&test_OnSetFocus,"OnSetFocus");
    test_ShouldDeactivate = SINK_UNEXPECTED;

    processPendingMessages();

    ITfThreadMgr_GetFocus(g_tm, &dmcheck);
    ok(dmcheck == dm1 || broken(dmcheck == dmorig /* Win7+ */), "Expected DocumentMgr not focused\n");
    ITfDocumentMgr_Release(dmcheck);

    /* We need to explicitly set focus on Win7+ */
    test_CurrentFocus = dm1;
    test_PrevFocus = FOCUS_IGNORE;
    test_OnSetFocus = SINK_OPTIONAL; /* Doesn't always fire on Win7+ */
    ITfThreadMgr_SetFocus(g_tm, dm1);
    sink_check_ok(&test_OnSetFocus, "OnSetFocus");

    hr = ITfThreadMgr_AssociateFocus(g_tm,wnd2,dm2,&olddm);
    ok(SUCCEEDED(hr),"AssociateFocus failed\n");
    processPendingMessages();
    ITfThreadMgr_GetFocus(g_tm, &dmcheck);
    ok(dmcheck == dm1, "Expected DocumentMgr not focused\n");
    ITfDocumentMgr_Release(dmcheck);

    hr = ITfThreadMgr_AssociateFocus(g_tm,wnd3,dm2,&olddm);
    ok(SUCCEEDED(hr),"AssociateFocus failed\n");
    processPendingMessages();
    ITfThreadMgr_GetFocus(g_tm, &dmcheck);
    ok(dmcheck == dm1, "Expected DocumentMgr not focused\n");
    ITfDocumentMgr_Release(dmcheck);

    test_CurrentFocus = FOCUS_SAVE;
    test_PrevFocus = FOCUS_SAVE;
    test_OnSetFocus = SINK_SAVE;
    ShowWindow(wnd2,SW_SHOWNORMAL);
    SetFocus(wnd2);
    sink_check_saved(&test_OnSetFocus,dm1,dm2,"OnSetFocus");
    test_CurrentFocus = FOCUS_IGNORE; /* occasional wine race */
    test_PrevFocus = FOCUS_IGNORE; /* occasional wine race */
    test_OnSetFocus = SINK_IGNORE; /* occasional wine race */
    processPendingMessages();

    ShowWindow(wnd3,SW_SHOWNORMAL);
    SetFocus(wnd3);
    processPendingMessages();

    test_CurrentFocus = FOCUS_SAVE;
    test_PrevFocus = FOCUS_SAVE;
    test_OnSetFocus = SINK_SAVE;
    SetFocus(wnd1);
    processPendingMessages();
    sink_check_saved(&test_OnSetFocus,dm2,dm1,"OnSetFocus");

    hr = ITfThreadMgr_AssociateFocus(g_tm,wnd3,NULL,&olddm);
    ok(SUCCEEDED(hr),"AssociateFocus failed\n");
    ok(olddm == dm2, "incorrect old DocumentMgr returned\n");
    ITfDocumentMgr_Release(olddm);

    test_CurrentFocus = dmorig;
    test_PrevFocus = dm1;
    test_OnSetFocus  = SINK_OPTIONAL; /* Doesn't always fire on Win7+ */
    test_ACP_GetStatus = SINK_IGNORE;
    ITfThreadMgr_SetFocus(g_tm,dmorig);
    sink_check_ok(&test_OnSetFocus,"OnSetFocus");

    test_CurrentFocus = FOCUS_SAVE;
    test_PrevFocus = FOCUS_SAVE;
    test_OnSetFocus = SINK_SAVE;
    SetFocus(wnd3);
    processPendingMessages();
    sink_check_saved(&test_OnSetFocus,dmorig,FOCUS_IGNORE,"OnSetFocus"); /* CurrentFocus NULL on XP, system default on Vista */

    hr = ITfThreadMgr_AssociateFocus(g_tm,wnd2,NULL,&olddm);
    ok(SUCCEEDED(hr),"AssociateFocus failed\n");
    ok(olddm == dm2, "incorrect old DocumentMgr returned\n");
    ITfDocumentMgr_Release(olddm);
    hr = ITfThreadMgr_AssociateFocus(g_tm,wnd1,NULL,&olddm);
    ok(SUCCEEDED(hr),"AssociateFocus failed\n");
    ok(olddm == dm1, "incorrect old DocumentMgr returned\n");
    ITfDocumentMgr_Release(olddm);

    test_OnSetFocus = SINK_IGNORE; /* OnSetFocus fires a couple of times on Win7 */
    test_CurrentFocus = FOCUS_IGNORE;
    test_PrevFocus = FOCUS_IGNORE;
    SetFocus(wnd2);
    processPendingMessages();
    SetFocus(wnd1);
    processPendingMessages();
    test_OnSetFocus = SINK_UNEXPECTED;

    ITfDocumentMgr_Release(dm1);
    ITfDocumentMgr_Release(dm2);

    test_CurrentFocus = dmorig;
    test_PrevFocus = FOCUS_IGNORE;
    test_OnSetFocus  = SINK_OPTIONAL;
    test_ACP_GetStatus = SINK_IGNORE;
    ITfThreadMgr_SetFocus(g_tm,dmorig);
    sink_check_ok(&test_OnSetFocus,"OnSetFocus");

    test_OnSetFocus = SINK_IGNORE; /* OnSetFocus fires a couple of times on Win7 */
    test_CurrentFocus = FOCUS_IGNORE;
    test_PrevFocus = FOCUS_IGNORE;
    DestroyWindow(wnd1);
    DestroyWindow(wnd2);
    test_OnPopContext = SINK_OPTIONAL; /* Vista and greater */
    test_OnSetFocus = SINK_OPTIONAL; /* Vista and greater */
    ITfThreadMgr_GetFocus(g_tm, &test_PrevFocus);
    test_CurrentFocus = NULL;
    test_ShouldDeactivate = TRUE; /* Win7 */
    DestroyWindow(wnd3);
    test_ShouldDeactivate = FALSE;
    sink_check_ok(&test_OnSetFocus,"OnSetFocus");
    sink_check_ok(&test_OnPopContext,"OnPopContext");
}

static void test_profile_mgr(void)
{
    IEnumTfInputProcessorProfiles *enum_profiles;
    ITfInputProcessorProfileMgr *ipp_mgr;
    HRESULT hres;

    hres = ITfInputProcessorProfiles_QueryInterface(g_ipp, &IID_ITfInputProcessorProfileMgr, (void**)&ipp_mgr);
    if (hres != S_OK)
    {
        win_skip("ITfInputProcessorProfileMgr is not supported.\n");
        return;
    }
    ok(hres == S_OK, "Could not get ITfInputProcessorProfileMgr iface: %08x\n", hres);

    hres = ITfInputProcessorProfileMgr_EnumProfiles(ipp_mgr, 0, &enum_profiles);
    ok(hres == S_OK, "EnumProfiles failed: %08x\n", hres);

    IEnumTfInputProcessorProfiles_Release(enum_profiles);

    ITfInputProcessorProfileMgr_Release(ipp_mgr);
}

START_TEST(inputprocessor)
{
    if (SUCCEEDED(initialize()))
    {
        test_Register();
        test_RegisterCategory();
        Sleep(2000); /* Win7 needs some time before the registrations become active */
        processPendingMessages();
        test_EnumLanguageProfiles();
        test_EnumInputProcessorInfo();
        test_Enable();
        test_ThreadMgrAdviseSinks();
        test_Activate();
        test_startSession();
        test_TfGuidAtom();
        test_ClientId();
        test_KeystrokeMgr();
        test_TStoApplicationText();
        test_Compartments();
        test_AssociateFocus();
        test_endSession();
        test_FindClosestCategory();
        test_Disable();
        test_ThreadMgrUnadviseSinks();
        test_UnregisterCategory();
        test_Unregister();
        test_profile_mgr();
    }
    else
        skip("Unable to create InputProcessor\n");
    cleanup();
}
