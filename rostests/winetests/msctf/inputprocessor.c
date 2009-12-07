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

#define SINK_UNEXPECTED 0
#define SINK_EXPECTED 1
#define SINK_FIRED 2

static BOOL test_ShouldActivate = FALSE;
static BOOL test_ShouldDeactivate = FALSE;

static DWORD tmSinkCookie;
static DWORD tmSinkRefCount;
static ITfDocumentMgr *test_CurrentFocus = NULL;
static ITfDocumentMgr *test_PrevFocus = NULL;
static INT  test_OnSetFocus = SINK_UNEXPECTED;
static INT  test_OnInitDocumentMgr = SINK_UNEXPECTED;
static INT  test_OnPushContext = SINK_UNEXPECTED;
static INT  test_OnPopContext = SINK_UNEXPECTED;
static INT  test_KEV_OnSetFocus = SINK_UNEXPECTED;

HRESULT RegisterTextService(REFCLSID rclsid);
HRESULT UnregisterTextService();
HRESULT ThreadMgrEventSink_Constructor(IUnknown **ppOut);
HRESULT TextStoreACP_Constructor(IUnknown **ppOut);

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
                ok(IsEqualGUID(&profile.catid,&GUID_TFCAT_TIP_KEYBOARD), "CatId Incorrect\n");
                ok(IsEqualGUID(&profile.guidProfile,&CLSID_FakeService), "guidProfile Incorrect\n");
            }
        }
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

    ThreadMgrEventSink_Constructor(&sink);

    tmSinkRefCount = 1;
    tmSinkCookie = 0;
    hr = ITfSource_AdviseSink(source,&IID_ITfThreadMgrEventSink, sink, &tmSinkCookie);
    ok(SUCCEEDED(hr),"Failed to Advise Sink\n");
    ok(tmSinkCookie!=0,"Failed to get sink cookie\n");

    /* Advising the sink adds a ref, Relesing here lets the object be deleted
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
    const ITfKeyEventSinkVtbl *KeyEventSinkVtbl;
    LONG refCount;
} KeyEventSink;

static void KeyEventSink_Destructor(KeyEventSink *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI KeyEventSink_QueryInterface(ITfKeyEventSink *iface, REFIID iid, LPVOID *ppvOut)
{
    KeyEventSink *This = (KeyEventSink *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfKeyEventSink))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI KeyEventSink_AddRef(ITfKeyEventSink *iface)
{
    KeyEventSink *This = (KeyEventSink *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI KeyEventSink_Release(ITfKeyEventSink *iface)
{
    KeyEventSink *This = (KeyEventSink *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        KeyEventSink_Destructor(This);
    return ret;
}

static HRESULT WINAPI KeyEventSink_OnSetFocus(ITfKeyEventSink *iface,
        BOOL fForeground)
{
    ok(test_KEV_OnSetFocus == SINK_EXPECTED,"Unexpected KeyEventSink_OnSetFocus\n");
    test_KEV_OnSetFocus = SINK_FIRED;
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

HRESULT KeyEventSink_Constructor(ITfKeyEventSink **ppOut)
{
    KeyEventSink *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(KeyEventSink));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->KeyEventSinkVtbl = &KeyEventSink_KeyEventSinkVtbl;
    This->refCount = 1;

    *ppOut = (ITfKeyEventSink*)This;
    return S_OK;
}


static void test_KeystrokeMgr(void)
{
    ITfKeystrokeMgr *keymgr= NULL;
    HRESULT hr;
    TF_PRESERVEDKEY tfpk;
    BOOL preserved;
    ITfKeyEventSink *sink;

    KeyEventSink_Constructor(&sink);

    hr = ITfThreadMgr_QueryInterface(g_tm, &IID_ITfKeystrokeMgr, (LPVOID*)&keymgr);
    ok(SUCCEEDED(hr),"Failed to get IID_ITfKeystrokeMgr for ThreadMgr\n");

    tfpk.uVKey = 'A';
    tfpk.uModifiers = TF_MOD_SHIFT;

    test_KEV_OnSetFocus = SINK_EXPECTED;
    hr = ITfKeystrokeMgr_AdviseKeyEventSink(keymgr,tid,sink,TRUE);
    todo_wine ok(SUCCEEDED(hr),"ITfKeystrokeMgr_AdviseKeyEventSink failed\n");
    todo_wine ok(test_KEV_OnSetFocus == SINK_FIRED, "KeyEventSink_OnSetFocus not fired as expected\n");

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
    todo_wine ok(SUCCEEDED(hr),"ITfKeystrokeMgr_UnadviseKeyEventSink failed\n");

    ITfKeystrokeMgr_Release(keymgr);
    ITfKeyEventSink_Release(sink);
}

static void test_Activate(void)
{
    HRESULT hr;

    hr = ITfInputProcessorProfiles_ActivateLanguageProfile(g_ipp,&CLSID_FakeService,gLangid,&CLSID_FakeService);
    ok(SUCCEEDED(hr),"Failed to Activate text service\n");
}

static inline int check_context_refcount(ITfContext *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
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

    hr = ITfThreadMgr_Deactivate(g_tm);
    ok(hr == E_UNEXPECTED,"Deactivate should have failed with E_UNEXPECTED\n");

    test_ShouldActivate = TRUE;
    hr  = ITfThreadMgr_Activate(g_tm,&cid);
    ok(SUCCEEDED(hr),"Failed to Activate\n");
    ok(cid != tid,"TextService id mistakenly matches Client id\n");

    test_ShouldActivate = FALSE;
    hr = ITfThreadMgr_Activate(g_tm,&cid2);
    ok(SUCCEEDED(hr),"Failed to Activate\n");
    ok (cid == cid2, "Second activate client ID does not match\n");

    hr = ITfThreadMgr_Deactivate(g_tm);
    ok(SUCCEEDED(hr),"Failed to Deactivate\n");

    hr = ITfThreadMgr_CreateDocumentMgr(g_tm,&g_dm);
    ok(SUCCEEDED(hr),"CreateDocumentMgr failed\n");

    hr = ITfThreadMgr_GetFocus(g_tm,&dmtest);
    ok(SUCCEEDED(hr),"GetFocus Failed\n");
    ok(dmtest == NULL,"Initial focus not null\n");

    test_CurrentFocus = g_dm;
    test_PrevFocus = NULL;
    test_OnSetFocus  = SINK_EXPECTED;
    hr = ITfThreadMgr_SetFocus(g_tm,g_dm);
    ok(SUCCEEDED(hr),"SetFocus Failed\n");
    ok(test_OnSetFocus == SINK_FIRED, "OnSetFocus sink not called\n");
    test_OnSetFocus  = SINK_UNEXPECTED;

    hr = ITfThreadMgr_GetFocus(g_tm,&dmtest);
    ok(SUCCEEDED(hr),"GetFocus Failed\n");
    ok(g_dm == dmtest,"Expected DocumentMgr not focused\n");

    cnt = ITfDocumentMgr_Release(g_dm);
    ok(cnt == 2,"DocumentMgr refcount not expected (2 vs %i)\n",cnt);

    hr = ITfThreadMgr_GetFocus(g_tm,&dmtest);
    ok(SUCCEEDED(hr),"GetFocus Failed\n");
    ok(g_dm == dmtest,"Expected DocumentMgr not focused\n");

    TextStoreACP_Constructor((IUnknown**)&ts);

    hr = ITfDocumentMgr_CreateContext(g_dm, cid, 0, (IUnknown*)ts, &cxt, &editCookie);
    ok(SUCCEEDED(hr),"CreateContext Failed\n");

    hr = ITfDocumentMgr_CreateContext(g_dm, cid, 0, NULL, &cxt2, &editCookie);
    ok(SUCCEEDED(hr),"CreateContext Failed\n");

    hr = ITfDocumentMgr_CreateContext(g_dm, cid, 0, NULL, &cxt3, &editCookie);
    ok(SUCCEEDED(hr),"CreateContext Failed\n");

    cnt = check_context_refcount(cxt);
    test_OnPushContext = SINK_EXPECTED;
    test_OnInitDocumentMgr = SINK_EXPECTED;
    hr = ITfDocumentMgr_Push(g_dm, cxt);
    ok(SUCCEEDED(hr),"Push Failed\n");
    ok(check_context_refcount(cxt) > cnt, "Ref count did not increase\n");
    ok(test_OnPushContext == SINK_FIRED, "OnPushContext sink not fired\n");
    ok(test_OnInitDocumentMgr == SINK_FIRED, "OnInitDocumentMgr sink not fired\n");

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
    ok(test_OnPushContext == SINK_FIRED, "OnPushContext sink not fired\n");

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
    ok(!SUCCEEDED(hr),"Push Succeeded\n");
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
    ok(test_OnPopContext == SINK_FIRED, "OnPopContext sink not fired\n");

    hr = ITfDocumentMgr_GetTop(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetTop Failed\n");
    ok(cxtTest == cxt, "Wrong context on top\n");
    ITfContext_Release(cxtTest);

    hr = ITfDocumentMgr_GetBase(g_dm, &cxtTest);
    ok(SUCCEEDED(hr),"GetBase Failed\n");
    ok(cxtTest == cxt, "Wrong context on base\n");
    ITfContext_Release(cxtTest);

    hr = ITfDocumentMgr_Pop(g_dm, 0);
    ok(!SUCCEEDED(hr),"Pop Succeeded\n");

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
    test_OnSetFocus  = SINK_EXPECTED;
    hr = ITfThreadMgr_Deactivate(g_tm);
    ok(SUCCEEDED(hr),"Failed to Deactivate\n");
    ok(test_OnSetFocus == SINK_FIRED, "OnSetFocus sink not called\n");
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
    ok(SUCCEEDED(hr),"Unable to aquire ITfClientId interface\n");

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

START_TEST(inputprocessor)
{
    if (SUCCEEDED(initialize()))
    {
        test_Register();
        test_RegisterCategory();
        test_EnumInputProcessorInfo();
        test_Enable();
        test_ThreadMgrAdviseSinks();
        test_Activate();
        test_startSession();
        test_TfGuidAtom();
        test_ClientId();
        test_KeystrokeMgr();
        test_endSession();
        test_EnumLanguageProfiles();
        test_FindClosestCategory();
        test_Disable();
        test_ThreadMgrUnadviseSinks();
        test_UnregisterCategory();
        test_Unregister();
    }
    else
        skip("Unable to create InputProcessor\n");
    cleanup();
}

/**********************************************************************
 * ITextStoreACP
 **********************************************************************/
typedef struct tagTextStoreACP
{
    const ITextStoreACPVtbl *TextStoreACPVtbl;
    LONG refCount;
} TextStoreACP;

static void TextStoreACP_Destructor(TextStoreACP *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI TextStoreACP_QueryInterface(ITextStoreACP *iface, REFIID iid, LPVOID *ppvOut)
{
    TextStoreACP *This = (TextStoreACP *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITextStoreACP))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TextStoreACP_AddRef(ITextStoreACP *iface)
{
    TextStoreACP *This = (TextStoreACP *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI TextStoreACP_Release(ITextStoreACP *iface)
{
    TextStoreACP *This = (TextStoreACP *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        TextStoreACP_Destructor(This);
    return ret;
}

static HRESULT WINAPI TextStoreACP_AdviseSink(ITextStoreACP *iface,
    REFIID riid, IUnknown *punk, DWORD dwMask)
{
    trace("\n");
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
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_GetStatus(ITextStoreACP *iface,
    TS_STATUS *pdcs)
{
    trace("\n");
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
    trace("\n");
    return S_OK;
}
static HRESULT WINAPI TextStoreACP_SetSelection(ITextStoreACP *iface,
    ULONG ulCount, const TS_SELECTION_ACP *pSelection)
{
    trace("\n");
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
    trace("\n");
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
    trace("\n");
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

HRESULT TextStoreACP_Constructor(IUnknown **ppOut)
{
    TextStoreACP *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TextStoreACP));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->TextStoreACPVtbl = &TextStoreACP_TextStoreACPVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)This;
    return S_OK;
}

/**********************************************************************
 * ITfThreadMgrEventSink
 **********************************************************************/
typedef struct tagThreadMgrEventSink
{
    const ITfThreadMgrEventSinkVtbl *ThreadMgrEventSinkVtbl;
    LONG refCount;
} ThreadMgrEventSink;

static void ThreadMgrEventSink_Destructor(ThreadMgrEventSink *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI ThreadMgrEventSink_QueryInterface(ITfThreadMgrEventSink *iface, REFIID iid, LPVOID *ppvOut)
{
    ThreadMgrEventSink *This = (ThreadMgrEventSink *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfThreadMgrEventSink))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ThreadMgrEventSink_AddRef(ITfThreadMgrEventSink *iface)
{
    ThreadMgrEventSink *This = (ThreadMgrEventSink *)iface;
    ok (tmSinkRefCount == This->refCount,"ThreadMgrEventSink refcount off %i vs %i\n",This->refCount,tmSinkRefCount);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ThreadMgrEventSink_Release(ITfThreadMgrEventSink *iface)
{
    ThreadMgrEventSink *This = (ThreadMgrEventSink *)iface;
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
    ok(test_OnInitDocumentMgr == SINK_EXPECTED, "Unexpected OnInitDocumentMgr sink\n");
    test_OnInitDocumentMgr = SINK_FIRED;
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
    ok(test_OnSetFocus == SINK_EXPECTED, "Unexpected OnSetFocus sink\n");
    ok(pdimFocus == test_CurrentFocus,"Sink reports wrong focus\n");
    ok(pdimPrevFocus == test_PrevFocus,"Sink reports wrong previous focus\n");
    test_OnSetFocus = SINK_FIRED;
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnPushContext(ITfThreadMgrEventSink *iface,
ITfContext *pic)
{
    ok(test_OnPushContext == SINK_EXPECTED, "Unexpected OnPushContext sink\n");
    test_OnPushContext = SINK_FIRED;
    return S_OK;
}

static HRESULT WINAPI ThreadMgrEventSink_OnPopContext(ITfThreadMgrEventSink *iface,
ITfContext *pic)
{
    ok(test_OnPopContext == SINK_EXPECTED, "Unexpected OnPopContext sink\n");
    test_OnPopContext = SINK_FIRED;
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

HRESULT ThreadMgrEventSink_Constructor(IUnknown **ppOut)
{
    ThreadMgrEventSink *This;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ThreadMgrEventSink));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ThreadMgrEventSinkVtbl = &ThreadMgrEventSink_ThreadMgrEventSinkVtbl;
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
    const IClassFactoryVtbl *vtbl;
    LONG   ref;
    LPFNCONSTRUCTOR ctor;
} ClassFactory;

typedef struct tagTextService
{
    const ITfTextInputProcessorVtbl *TextInputProcessorVtbl;
    LONG refCount;
} TextService;

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
    This->vtbl = &ClassFactoryVtbl;
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
    TextService *This = (TextService *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfTextInputProcessor))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI TextService_AddRef(ITfTextInputProcessor *iface)
{
    TextService *This = (TextService *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI TextService_Release(ITfTextInputProcessor *iface)
{
    TextService *This = (TextService *)iface;
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

HRESULT TextService_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    TextService *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(TextService));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->TextInputProcessorVtbl= &TextService_TextInputProcessorVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)This;
    return S_OK;
}

HRESULT RegisterTextService(REFCLSID rclsid)
{
    ClassFactory_Constructor( TextService_Constructor ,(LPVOID*)&cf);
    return CoRegisterClassObject(rclsid, (IUnknown*) cf, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
}

HRESULT UnregisterTextService()
{
    return CoRevokeClassObject(regid);
}
