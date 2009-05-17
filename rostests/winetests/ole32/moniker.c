/*
 * Moniker Tests
 *
 * Copyright 2004 Robert Shearman
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

#define _WIN32_DCOM
#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "comcat.h"
#include "olectl.h"

#include "wine/test.h"

#define ok_more_than_one_lock() ok(cLocks > 0, "Number of locks should be > 0, but actually is %d\n", cLocks)
#define ok_no_locks() ok(cLocks == 0, "Number of locks should be 0, but actually is %d\n", cLocks)
#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08x\n", hr)
#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))

#define CHECK_EXPECTED_METHOD(method_name) \
do { \
    trace("%s\n", method_name); \
        ok(*expected_method_list != NULL, "Extra method %s called\n", method_name); \
            if (*expected_method_list) \
            { \
                ok(!strcmp(*expected_method_list, method_name), "Expected %s to be called instead of %s\n", \
                   *expected_method_list, method_name); \
                       expected_method_list++; \
            } \
} while(0)

static char const * const *expected_method_list;
static const WCHAR wszFileName1[] = {'c',':','\\','w','i','n','d','o','w','s','\\','t','e','s','t','1','.','d','o','c',0};
static const WCHAR wszFileName2[] = {'c',':','\\','w','i','n','d','o','w','s','\\','t','e','s','t','2','.','d','o','c',0};

static const CLSID CLSID_WineTest =
{ /* 9474ba1a-258b-490b-bc13-516e9239ace0 */
    0x9474ba1a,
    0x258b,
    0x490b,
    {0xbc, 0x13, 0x51, 0x6e, 0x92, 0x39, 0xac, 0xe0}
};

static const CLSID CLSID_TestMoniker =
{ /* b306bfbc-496e-4f53-b93e-2ff9c83223d7 */
    0xb306bfbc,
    0x496e,
    0x4f53,
    {0xb9, 0x3e, 0x2f, 0xf9, 0xc8, 0x32, 0x23, 0xd7}
};

static LONG cLocks;

static void LockModule(void)
{
    InterlockedIncrement(&cLocks);
}

static void UnlockModule(void)
{
    InterlockedDecrement(&cLocks);
}

static SIZE_T round_global_size(SIZE_T size)
{
    static SIZE_T global_size_alignment = -1;
    if (global_size_alignment == -1)
    {
        void *p = GlobalAlloc(GMEM_FIXED, 1);
        global_size_alignment = GlobalSize(p);
        GlobalFree(p);
    }

    return ((size + global_size_alignment - 1) & ~(global_size_alignment - 1));
}

static HRESULT WINAPI Test_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    LockModule();
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IClassFactory_Release(LPCLASSFACTORY iface)
{
    UnlockModule();
    return 1; /* non-heap-based object */
}

static HRESULT WINAPI Test_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI Test_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    return S_OK;
}

static const IClassFactoryVtbl TestClassFactory_Vtbl =
{
    Test_IClassFactory_QueryInterface,
    Test_IClassFactory_AddRef,
    Test_IClassFactory_Release,
    Test_IClassFactory_CreateInstance,
    Test_IClassFactory_LockServer
};

static IClassFactory Test_ClassFactory = { &TestClassFactory_Vtbl };

typedef struct
{
    const IUnknownVtbl *lpVtbl;
    ULONG refs;
} HeapUnknown;

static HRESULT WINAPI HeapUnknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI HeapUnknown_AddRef(IUnknown *iface)
{
    HeapUnknown *This = (HeapUnknown *)iface;
    return InterlockedIncrement((LONG*)&This->refs);
}

static ULONG WINAPI HeapUnknown_Release(IUnknown *iface)
{
    HeapUnknown *This = (HeapUnknown *)iface;
    ULONG refs = InterlockedDecrement((LONG*)&This->refs);
    if (!refs) HeapFree(GetProcessHeap(), 0, This);
    return refs;
}

static const IUnknownVtbl HeapUnknown_Vtbl =
{
    HeapUnknown_QueryInterface,
    HeapUnknown_AddRef,
    HeapUnknown_Release
};

static HRESULT WINAPI
MonikerNoROTData_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid)      ||
        IsEqualIID(&IID_IPersist, riid)      ||
        IsEqualIID(&IID_IPersistStream,riid) ||
        IsEqualIID(&IID_IMoniker, riid))
        *ppvObject = iface;
    if (IsEqualIID(&IID_IROTData, riid))
        CHECK_EXPECTED_METHOD("Moniker_QueryInterface(IID_IROTData)");

    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    IMoniker_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI
Moniker_AddRef(IMoniker* iface)
{
    return 2;
}

static ULONG WINAPI
Moniker_Release(IMoniker* iface)
{
    return 1;
}

static HRESULT WINAPI
Moniker_GetClassID(IMoniker* iface, CLSID *pClassID)
{
    CHECK_EXPECTED_METHOD("Moniker_GetClassID");

    *pClassID = CLSID_TestMoniker;

    return S_OK;
}

static HRESULT WINAPI
Moniker_IsDirty(IMoniker* iface)
{
    CHECK_EXPECTED_METHOD("Moniker_IsDirty");

    return S_FALSE;
}

static HRESULT WINAPI
Moniker_Load(IMoniker* iface, IStream* pStm)
{
    CHECK_EXPECTED_METHOD("Moniker_Load");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    CHECK_EXPECTED_METHOD("Moniker_Save");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    CHECK_EXPECTED_METHOD("Moniker_GetSizeMax");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_BindToObject(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                             REFIID riid, VOID** ppvResult)
{
    CHECK_EXPECTED_METHOD("Moniker_BindToObject");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_BindToStorage(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                              REFIID riid, VOID** ppvObject)
{
    CHECK_EXPECTED_METHOD("Moniker_BindToStorage");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
                       IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    CHECK_EXPECTED_METHOD("Moniker_Reduce");

    if (ppmkReduced==NULL)
        return E_POINTER;

    IMoniker_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI
Moniker_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
                            BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{
    CHECK_EXPECTED_METHOD("Moniker_ComposeWith");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    CHECK_EXPECTED_METHOD("Moniker_Enum");

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

static HRESULT WINAPI
Moniker_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    CHECK_EXPECTED_METHOD("Moniker_IsEqual");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Hash(IMoniker* iface,DWORD* pdwHash)
{
    CHECK_EXPECTED_METHOD("Moniker_Hash");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_IsRunning(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                          IMoniker* pmkNewlyRunning)
{
    CHECK_EXPECTED_METHOD("Moniker_IsRunning");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc,
                                    IMoniker* pmkToLeft, FILETIME* pFileTime)
{
    CHECK_EXPECTED_METHOD("Moniker_GetTimeOfLastChange");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    CHECK_EXPECTED_METHOD("Moniker_Inverse");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    CHECK_EXPECTED_METHOD("Moniker_CommonPrefixWith");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    CHECK_EXPECTED_METHOD("Moniker_RelativePathTo");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_GetDisplayName(IMoniker* iface, IBindCtx* pbc,
                               IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
    static const WCHAR wszDisplayName[] = {'*','*','G','e','m','m','a',0};
    CHECK_EXPECTED_METHOD("Moniker_GetDisplayName");
    *ppszDisplayName = CoTaskMemAlloc(sizeof(wszDisplayName));
    memcpy(*ppszDisplayName, wszDisplayName, sizeof(wszDisplayName));
    return S_OK;
}

static HRESULT WINAPI
Moniker_ParseDisplayName(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                     LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
    CHECK_EXPECTED_METHOD("Moniker_ParseDisplayName");
    return E_NOTIMPL;
}

static HRESULT WINAPI
Moniker_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    CHECK_EXPECTED_METHOD("Moniker_IsSystemMoniker");

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_NONE;

    return S_FALSE;
}

static const IMonikerVtbl MonikerNoROTDataVtbl =
{
    MonikerNoROTData_QueryInterface,
    Moniker_AddRef,
    Moniker_Release,
    Moniker_GetClassID,
    Moniker_IsDirty,
    Moniker_Load,
    Moniker_Save,
    Moniker_GetSizeMax,
    Moniker_BindToObject,
    Moniker_BindToStorage,
    Moniker_Reduce,
    Moniker_ComposeWith,
    Moniker_Enum,
    Moniker_IsEqual,
    Moniker_Hash,
    Moniker_IsRunning,
    Moniker_GetTimeOfLastChange,
    Moniker_Inverse,
    Moniker_CommonPrefixWith,
    Moniker_RelativePathTo,
    Moniker_GetDisplayName,
    Moniker_ParseDisplayName,
    Moniker_IsSystemMoniker
};

static IMoniker MonikerNoROTData = { &MonikerNoROTDataVtbl };

static IMoniker Moniker;

static HRESULT WINAPI
ROTData_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{
    return IMoniker_QueryInterface(&Moniker, riid, ppvObject);
}

static ULONG WINAPI
ROTData_AddRef(IROTData *iface)
{
    return 2;
}

static ULONG WINAPI
ROTData_Release(IROTData* iface)
{
    return 1;
}

static HRESULT WINAPI
ROTData_GetComparisonData(IROTData* iface, BYTE* pbData,
                                          ULONG cbMax, ULONG* pcbData)
{
    CHECK_EXPECTED_METHOD("ROTData_GetComparisonData");

    *pcbData = 1;
    if (cbMax < *pcbData)
        return E_OUTOFMEMORY;

    *pbData = 0xde;

    return S_OK;
}

static IROTDataVtbl ROTDataVtbl =
{
    ROTData_QueryInterface,
    ROTData_AddRef,
    ROTData_Release,
    ROTData_GetComparisonData
};

static IROTData ROTData = { &ROTDataVtbl };

static HRESULT WINAPI
Moniker_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid)      ||
        IsEqualIID(&IID_IPersist, riid)      ||
        IsEqualIID(&IID_IPersistStream,riid) ||
        IsEqualIID(&IID_IMoniker, riid))
        *ppvObject = iface;
    if (IsEqualIID(&IID_IROTData, riid))
    {
        CHECK_EXPECTED_METHOD("Moniker_QueryInterface(IID_IROTData)");
        *ppvObject = &ROTData;
    }

    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    IMoniker_AddRef(iface);

    return S_OK;
}

static const IMonikerVtbl MonikerVtbl =
{
    Moniker_QueryInterface,
    Moniker_AddRef,
    Moniker_Release,
    Moniker_GetClassID,
    Moniker_IsDirty,
    Moniker_Load,
    Moniker_Save,
    Moniker_GetSizeMax,
    Moniker_BindToObject,
    Moniker_BindToStorage,
    Moniker_Reduce,
    Moniker_ComposeWith,
    Moniker_Enum,
    Moniker_IsEqual,
    Moniker_Hash,
    Moniker_IsRunning,
    Moniker_GetTimeOfLastChange,
    Moniker_Inverse,
    Moniker_CommonPrefixWith,
    Moniker_RelativePathTo,
    Moniker_GetDisplayName,
    Moniker_ParseDisplayName,
    Moniker_IsSystemMoniker
};

static IMoniker Moniker = { &MonikerVtbl };

static void test_ROT(void)
{
    static const WCHAR wszFileName[] = {'B','E','2','0','E','2','F','5','-',
        '1','9','0','3','-','4','A','A','E','-','B','1','A','F','-',
        '2','0','4','6','E','5','8','6','C','9','2','5',0};
    HRESULT hr;
    IMoniker *pMoniker = NULL;
    IRunningObjectTable *pROT = NULL;
    DWORD dwCookie;
    static const char *methods_register_no_ROTData[] =
    {
        "Moniker_Reduce",
        "Moniker_GetTimeOfLastChange",
        "Moniker_QueryInterface(IID_IROTData)",
        "Moniker_GetDisplayName",
        "Moniker_GetClassID",
        NULL
    };
    static const char *methods_register[] =
    {
        "Moniker_Reduce",
        "Moniker_GetTimeOfLastChange",
        "Moniker_QueryInterface(IID_IROTData)",
        "ROTData_GetComparisonData",
        NULL
    };
    static const char *methods_isrunning_no_ROTData[] =
    {
        "Moniker_Reduce",
        "Moniker_QueryInterface(IID_IROTData)",
        "Moniker_GetDisplayName",
        "Moniker_GetClassID",
        NULL
    };
    static const char *methods_isrunning[] =
    {
        "Moniker_Reduce",
        "Moniker_QueryInterface(IID_IROTData)",
        "ROTData_GetComparisonData",
        NULL
    };

    cLocks = 0;

    hr = GetRunningObjectTable(0, &pROT);
    ok_ole_success(hr, GetRunningObjectTable);

    expected_method_list = methods_register_no_ROTData;
    /* try with our own moniker that doesn't support IROTData */
    hr = IRunningObjectTable_Register(pROT, ROTFLAGS_REGISTRATIONKEEPSALIVE,
        (IUnknown*)&Test_ClassFactory, &MonikerNoROTData, &dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Register);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    ok_more_than_one_lock();

    expected_method_list = methods_isrunning_no_ROTData;
    hr = IRunningObjectTable_IsRunning(pROT, &MonikerNoROTData);
    ok_ole_success(hr, IRunningObjectTable_IsRunning);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    ok_no_locks();

    expected_method_list = methods_register;
    /* try with our own moniker */
    hr = IRunningObjectTable_Register(pROT, ROTFLAGS_REGISTRATIONKEEPSALIVE,
        (IUnknown*)&Test_ClassFactory, &Moniker, &dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Register);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    ok_more_than_one_lock();

    expected_method_list = methods_isrunning;
    hr = IRunningObjectTable_IsRunning(pROT, &Moniker);
    ok_ole_success(hr, IRunningObjectTable_IsRunning);
    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    ok_no_locks();

    hr = CreateFileMoniker(wszFileName, &pMoniker);
    ok_ole_success(hr, CreateClassMoniker);

    /* test flags: 0 */
    hr = IRunningObjectTable_Register(pROT, 0, (IUnknown*)&Test_ClassFactory,
                                      pMoniker, &dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Register);

    ok_more_than_one_lock();

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    ok_no_locks();

    /* test flags: ROTFLAGS_REGISTRATIONKEEPSALIVE */
    hr = IRunningObjectTable_Register(pROT, ROTFLAGS_REGISTRATIONKEEPSALIVE,
        (IUnknown*)&Test_ClassFactory, pMoniker, &dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Register);

    ok_more_than_one_lock();

    hr = IRunningObjectTable_Revoke(pROT, dwCookie);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    ok_no_locks();

    /* test flags: ROTFLAGS_REGISTRATIONKEEPSALIVE|ROTFLAGS_ALLOWANYCLIENT */
    /* only succeeds when process is started by SCM and has LocalService
     * or RunAs AppId values */
    hr = IRunningObjectTable_Register(pROT,
        ROTFLAGS_REGISTRATIONKEEPSALIVE|ROTFLAGS_ALLOWANYCLIENT,
        (IUnknown*)&Test_ClassFactory, pMoniker, &dwCookie);
    todo_wine {
    ok(hr == CO_E_WRONG_SERVER_IDENTITY ||
       broken(hr == S_OK) /* Win9x */,
       "IRunningObjectTable_Register should have returned CO_E_WRONG_SERVER_IDENTITY instead of 0x%08x\n", hr);
    }
    if (hr == S_OK) IRunningObjectTable_Revoke(pROT, dwCookie);

    hr = IRunningObjectTable_Register(pROT, 0xdeadbeef,
        (IUnknown*)&Test_ClassFactory, pMoniker, &dwCookie);
    ok(hr == E_INVALIDARG, "IRunningObjectTable_Register should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    IMoniker_Release(pMoniker);

    IRunningObjectTable_Release(pROT);
}

static void test_ROT_multiple_entries(void)
{
    HRESULT hr;
    IMoniker *pMoniker = NULL;
    IRunningObjectTable *pROT = NULL;
    DWORD dwCookie1, dwCookie2;
    IUnknown *pObject = NULL;
    static const WCHAR moniker_path[] =
        {'\\', 'w','i','n','d','o','w','s','\\','s','y','s','t','e','m','\\','t','e','s','t','1','.','d','o','c',0};

    hr = GetRunningObjectTable(0, &pROT);
    ok_ole_success(hr, GetRunningObjectTable);

    hr = CreateFileMoniker(moniker_path, &pMoniker);
    ok_ole_success(hr, CreateFileMoniker);

    hr = IRunningObjectTable_Register(pROT, 0, (IUnknown *)&Test_ClassFactory, pMoniker, &dwCookie1);
    ok_ole_success(hr, IRunningObjectTable_Register);

    hr = IRunningObjectTable_Register(pROT, 0, (IUnknown *)&Test_ClassFactory, pMoniker, &dwCookie2);
    ok(hr == MK_S_MONIKERALREADYREGISTERED, "IRunningObjectTable_Register should have returned MK_S_MONIKERALREADYREGISTERED instead of 0x%08x\n", hr);

    ok(dwCookie1 != dwCookie2, "cookie returned for registering duplicate object shouldn't match cookie of original object (0x%x)\n", dwCookie1);

    hr = IRunningObjectTable_GetObject(pROT, pMoniker, &pObject);
    ok_ole_success(hr, IRunningObjectTable_GetObject);
    IUnknown_Release(pObject);

    hr = IRunningObjectTable_Revoke(pROT, dwCookie1);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    hr = IRunningObjectTable_GetObject(pROT, pMoniker, &pObject);
    ok_ole_success(hr, IRunningObjectTable_GetObject);
    IUnknown_Release(pObject);

    hr = IRunningObjectTable_Revoke(pROT, dwCookie2);
    ok_ole_success(hr, IRunningObjectTable_Revoke);

    IMoniker_Release(pMoniker);

    IRunningObjectTable_Release(pROT);
}

static HRESULT WINAPI ParseDisplayName_QueryInterface(IParseDisplayName *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IParseDisplayName))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ParseDisplayName_AddRef(IParseDisplayName *iface)
{
    return 2;
}

static ULONG WINAPI ParseDisplayName_Release(IParseDisplayName *iface)
{
    return 1;
}

static LPCWSTR expected_display_name;

static HRESULT WINAPI ParseDisplayName_ParseDisplayName(IParseDisplayName *iface,
                                                        IBindCtx *pbc,
                                                        LPOLESTR pszDisplayName,
                                                        ULONG *pchEaten,
                                                        IMoniker **ppmkOut)
{
    char display_nameA[256];
    WideCharToMultiByte(CP_ACP, 0, pszDisplayName, -1, display_nameA, sizeof(display_nameA), NULL, NULL);
    ok(!lstrcmpW(pszDisplayName, expected_display_name), "unexpected display name \"%s\"\n", display_nameA);
    ok(pszDisplayName == expected_display_name, "pszDisplayName should be the same pointer as passed into MkParseDisplayName\n");
    *pchEaten = lstrlenW(pszDisplayName);
    return CreateAntiMoniker(ppmkOut);
}

static const IParseDisplayNameVtbl ParseDisplayName_Vtbl =
{
    ParseDisplayName_QueryInterface,
    ParseDisplayName_AddRef,
    ParseDisplayName_Release,
    ParseDisplayName_ParseDisplayName
};

static IParseDisplayName ParseDisplayName = { &ParseDisplayName_Vtbl };

static int count_moniker_matches(IBindCtx * pbc, IEnumMoniker * spEM)
{
    IMoniker * spMoniker;
    int monCnt=0, matchCnt=0;

    while ((IEnumMoniker_Next(spEM, 1, &spMoniker, NULL)==S_OK))
    {
        HRESULT hr;
        WCHAR * szDisplayn;
        monCnt++;
        hr=IMoniker_GetDisplayName(spMoniker, pbc, NULL, &szDisplayn);
        if (SUCCEEDED(hr))
        {
            if (!lstrcmpiW(szDisplayn, wszFileName1) || !lstrcmpiW(szDisplayn, wszFileName2))
                matchCnt++;
            CoTaskMemFree(szDisplayn);
        }
    }
    trace("Total number of monikers is %i\n", monCnt);
    return matchCnt;
}

static void test_MkParseDisplayName(void)
{
    IBindCtx * pbc = NULL;
    HRESULT hr;
    IMoniker * pmk  = NULL;
    IMoniker * pmk1 = NULL;
    IMoniker * pmk2 = NULL;
    ULONG eaten;
    int matchCnt;
    IUnknown * object = NULL;

    IUnknown *lpEM1;

    IEnumMoniker *spEM1  = NULL;
    IEnumMoniker *spEM2  = NULL;
    IEnumMoniker *spEM3  = NULL;

    DWORD pdwReg1=0;
    DWORD grflags=0;
    DWORD pdwReg2=0;
    DWORD moniker_type;
    IRunningObjectTable * pprot=NULL;

    /* CLSID of My Computer */
    static const WCHAR wszDisplayName[] = {'c','l','s','i','d',':',
        '2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8','-','0','8','0','0','2','B','3','0','3','0','9','D',':',0};
    static const WCHAR wszDisplayNameClsid[] = {'c','l','s','i','d',':',0};
    static const WCHAR wszNonExistentProgId[] = {'N','o','n','E','x','i','s','t','e','n','t','P','r','o','g','I','d',':',0};
    static const WCHAR wszDisplayNameRunning[] = {'W','i','n','e','T','e','s','t','R','u','n','n','i','n','g',0};
    static const WCHAR wszDisplayNameProgId1[] = {'S','t','d','F','o','n','t',':',0};
    static const WCHAR wszDisplayNameProgId2[] = {'@','S','t','d','F','o','n','t',0};
    static const WCHAR wszDisplayNameProgIdFail[] = {'S','t','d','F','o','n','t',0};
    char szDisplayNameFile[256];
    WCHAR wszDisplayNameFile[256];

    hr = CreateBindCtx(0, &pbc);
    ok_ole_success(hr, CreateBindCtx);

    hr = MkParseDisplayName(pbc, wszNonExistentProgId, &eaten, &pmk);
    ok(hr == MK_E_SYNTAX || hr == MK_E_CANTOPENFILE /* Win9x */,
        "MkParseDisplayName should have failed with MK_E_SYNTAX or MK_E_CANTOPENFILE instead of 0x%08x\n", hr);

    /* no special handling of "clsid:" without the string form of the clsid
     * following */
    hr = MkParseDisplayName(pbc, wszDisplayNameClsid, &eaten, &pmk);
    ok(hr == MK_E_SYNTAX || hr == MK_E_CANTOPENFILE /* Win9x */,
        "MkParseDisplayName should have failed with MK_E_SYNTAX or MK_E_CANTOPENFILE instead of 0x%08x\n", hr);

    /* shows clsid has higher precedence than a running object */
    hr = CreateFileMoniker(wszDisplayName, &pmk);
    ok_ole_success(hr, CreateFileMoniker);
    hr = IBindCtx_GetRunningObjectTable(pbc, &pprot);
    ok_ole_success(hr, IBindCtx_GetRunningObjectTable);
    hr = IRunningObjectTable_Register(pprot, 0, (IUnknown *)&Test_ClassFactory, pmk, &pdwReg1);
    ok_ole_success(hr, IRunningObjectTable_Register);
    IMoniker_Release(pmk);
    pmk = NULL;
    hr = MkParseDisplayName(pbc, wszDisplayName, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    if (pmk)
    {
        IMoniker_IsSystemMoniker(pmk, &moniker_type);
        ok(moniker_type == MKSYS_CLASSMONIKER, "moniker_type was %d instead of MKSYS_CLASSMONIKER\n", moniker_type);
        IMoniker_Release(pmk);
    }
    hr = IRunningObjectTable_Revoke(pprot, pdwReg1);
    ok_ole_success(hr, IRunningObjectTable_Revoke);
    IRunningObjectTable_Release(pprot);

    hr = CreateFileMoniker(wszDisplayNameRunning, &pmk);
    ok_ole_success(hr, CreateFileMoniker);
    hr = IBindCtx_GetRunningObjectTable(pbc, &pprot);
    ok_ole_success(hr, IBindCtx_GetRunningObjectTable);
    hr = IRunningObjectTable_Register(pprot, 0, (IUnknown *)&Test_ClassFactory, pmk, &pdwReg1);
    ok_ole_success(hr, IRunningObjectTable_Register);
    IMoniker_Release(pmk);
    pmk = NULL;
    hr = MkParseDisplayName(pbc, wszDisplayNameRunning, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    if (pmk)
    {
        IMoniker_IsSystemMoniker(pmk, &moniker_type);
        ok(moniker_type == MKSYS_FILEMONIKER, "moniker_type was %d instead of MKSYS_FILEMONIKER\n", moniker_type);
        IMoniker_Release(pmk);
    }
    hr = IRunningObjectTable_Revoke(pprot, pdwReg1);
    ok_ole_success(hr, IRunningObjectTable_Revoke);
    IRunningObjectTable_Release(pprot);

    hr = CoRegisterClassObject(&CLSID_StdFont, (IUnknown *)&ParseDisplayName, CLSCTX_INPROC_SERVER, REGCLS_MULTI_SEPARATE, &pdwReg1);
    ok_ole_success(hr, CoRegisterClassObject);

    expected_display_name = wszDisplayNameProgId1;
    hr = MkParseDisplayName(pbc, wszDisplayNameProgId1, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    if (pmk)
    {
        IMoniker_IsSystemMoniker(pmk, &moniker_type);
        ok(moniker_type == MKSYS_ANTIMONIKER, "moniker_type was %d instead of MKSYS_ANTIMONIKER\n", moniker_type);
        IMoniker_Release(pmk);
    }

    expected_display_name = wszDisplayNameProgId2;
    hr = MkParseDisplayName(pbc, wszDisplayNameProgId2, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    if (pmk)
    {
        IMoniker_IsSystemMoniker(pmk, &moniker_type);
        ok(moniker_type == MKSYS_ANTIMONIKER, "moniker_type was %d instead of MKSYS_ANTIMONIKER\n", moniker_type);
        IMoniker_Release(pmk);
    }

    hr = MkParseDisplayName(pbc, wszDisplayNameProgIdFail, &eaten, &pmk);
    ok(hr == MK_E_SYNTAX || hr == MK_E_CANTOPENFILE /* Win9x */,
        "MkParseDisplayName with ProgId without marker should fail with MK_E_SYNTAX or MK_E_CANTOPENFILE instead of 0x%08x\n", hr);

    hr = CoRevokeClassObject(pdwReg1);
    ok_ole_success(hr, CoRevokeClassObject);

    GetSystemDirectoryA(szDisplayNameFile, sizeof(szDisplayNameFile));
    strcat(szDisplayNameFile, "\\kernel32.dll");
    MultiByteToWideChar(CP_ACP, 0, szDisplayNameFile, -1, wszDisplayNameFile, sizeof(wszDisplayNameFile)/sizeof(wszDisplayNameFile[0]));
    hr = MkParseDisplayName(pbc, wszDisplayNameFile, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);
    if (pmk)
    {
        IMoniker_IsSystemMoniker(pmk, &moniker_type);
        ok(moniker_type == MKSYS_FILEMONIKER, "moniker_type was %d instead of MKSYS_FILEMONIKER\n", moniker_type);
        IMoniker_Release(pmk);
    }

    hr = MkParseDisplayName(pbc, wszDisplayName, &eaten, &pmk);
    ok_ole_success(hr, MkParseDisplayName);

    if (pmk)
    {
        hr = IMoniker_BindToObject(pmk, pbc, NULL, &IID_IUnknown, (LPVOID*)&object);
        ok_ole_success(hr, IMoniker_BindToObject);

        if (SUCCEEDED(hr))
            IUnknown_Release(object);
        IMoniker_Release(pmk);
    }
    IBindCtx_Release(pbc);

    /* Test the EnumMoniker interface */
    hr = CreateBindCtx(0, &pbc);
    ok_ole_success(hr, CreateBindCtx);

    hr = CreateFileMoniker(wszFileName1, &pmk1);
    ok(hr==0, "CreateFileMoniker for file hr=%08x\n", hr);
    hr = CreateFileMoniker(wszFileName2, &pmk2);
    ok(hr==0, "CreateFileMoniker for file hr=%08x\n", hr);
    hr = IBindCtx_GetRunningObjectTable(pbc, &pprot);
    ok(hr==0, "IBindCtx_GetRunningObjectTable hr=%08x\n", hr);

    /* Check EnumMoniker before registering */
    hr = IRunningObjectTable_EnumRunning(pprot, &spEM1);
    ok(hr==0, "IRunningObjectTable_EnumRunning hr=%08x\n", hr);
    hr = IEnumMoniker_QueryInterface(spEM1, &IID_IUnknown, (void*) &lpEM1);
    /* Register a couple of Monikers and check is ok */
    ok(hr==0, "IEnumMoniker_QueryInterface hr %08x %p\n", hr, lpEM1);
    hr = MK_E_NOOBJECT;
    
    matchCnt = count_moniker_matches(pbc, spEM1);
    trace("Number of matches is %i\n", matchCnt);

    grflags= grflags | ROTFLAGS_REGISTRATIONKEEPSALIVE;
    hr = IRunningObjectTable_Register(pprot, grflags, lpEM1, pmk1, &pdwReg1);
    ok(hr==0, "IRunningObjectTable_Register hr=%08x %p %08x %p %p %d\n",
        hr, pprot, grflags, lpEM1, pmk1, pdwReg1);

    trace("IROT::Register\n");
    grflags=0;
    grflags= grflags | ROTFLAGS_REGISTRATIONKEEPSALIVE;
    hr = IRunningObjectTable_Register(pprot, grflags, lpEM1, pmk2, &pdwReg2);
    ok(hr==0, "IRunningObjectTable_Register hr=%08x %p %08x %p %p %d\n", hr,
       pprot, grflags, lpEM1, pmk2, pdwReg2);

    hr = IRunningObjectTable_EnumRunning(pprot, &spEM2);
    ok(hr==0, "IRunningObjectTable_EnumRunning hr=%08x\n", hr);

    matchCnt = count_moniker_matches(pbc, spEM2);
    ok(matchCnt==2, "Number of matches should be equal to 2 not %i\n", matchCnt);

    trace("IEnumMoniker::Clone\n");
    IEnumMoniker_Clone(spEM2, &spEM3);

    matchCnt = count_moniker_matches(pbc, spEM3);
    ok(matchCnt==0, "Number of matches should be equal to 0 not %i\n", matchCnt);
    trace("IEnumMoniker::Reset\n");
    IEnumMoniker_Reset(spEM3);

    matchCnt = count_moniker_matches(pbc, spEM3);
    ok(matchCnt==2, "Number of matches should be equal to 2 not %i\n", matchCnt);

    IRunningObjectTable_Revoke(pprot,pdwReg1);
    IRunningObjectTable_Revoke(pprot,pdwReg2);
    IUnknown_Release(lpEM1);
    IEnumMoniker_Release(spEM1);
    IEnumMoniker_Release(spEM2);
    IEnumMoniker_Release(spEM3);
    IMoniker_Release(pmk1);
    IMoniker_Release(pmk2);
    IRunningObjectTable_Release(pprot);

    IBindCtx_Release(pbc);
}

static const LARGE_INTEGER llZero;

static const BYTE expected_class_moniker_marshal_data[] =
{
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x1a,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
    0x05,0xe0,0x02,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,
};

static const BYTE expected_class_moniker_saved_data[] =
{
     0x05,0xe0,0x02,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,
};

static const BYTE expected_class_moniker_comparison_data[] =
{
     0x1a,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x05,0xe0,0x02,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
};

static const WCHAR expected_class_moniker_display_name[] =
{
    'c','l','s','i','d',':','0','0','0','2','E','0','0','5','-','0','0','0',
    '0','-','0','0','0','0','-','C','0','0','0','-','0','0','0','0','0','0',
    '0','0','0','0','4','6',':',0
};

static const BYTE expected_item_moniker_comparison_data[] =
{
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x21,0x00,0x54,0x00,0x45,0x00,0x53,0x00,
     0x54,0x00,0x00,0x00,
};

static const BYTE expected_item_moniker_saved_data[] =
{
     0x02,0x00,0x00,0x00,0x21,0x00,0x05,0x00,
     0x00,0x00,0x54,0x65,0x73,0x74,0x00,
};

static const BYTE expected_item_moniker_marshal_data[] =
{
     0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
     0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,
     0x02,0x00,0x00,0x00,0x21,0x00,0x05,0x00,
     0x00,0x00,0x54,0x65,0x73,0x74,0x00,
};

static const BYTE expected_anti_moniker_marshal_data[] =
{
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x05,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,
};

static const BYTE expected_anti_moniker_saved_data[] =
{
    0x01,0x00,0x00,0x00,
};

static const BYTE expected_anti_moniker_comparison_data[] =
{
    0x05,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x01,0x00,0x00,0x00,
};

static const BYTE expected_gc_moniker_marshal_data[] =
{
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x09,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x2c,0x01,0x00,0x00,
    0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x21,0x00,0x05,0x00,
    0x00,0x00,0x54,0x65,0x73,0x74,0x00,0x4d,
    0x45,0x4f,0x57,0x04,0x00,0x00,0x00,0x0f,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,
    0x00,0x00,0x00,0x00,0x00,0x00,0x46,0x04,
    0x03,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,
    0x00,0x00,0x00,0x00,0x00,0x00,0x46,0x00,
    0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x02,
    0x00,0x00,0x00,0x23,0x00,0x05,0x00,0x00,
    0x00,0x57,0x69,0x6e,0x65,0x00,
};

static const BYTE expected_gc_moniker_saved_data[] =
{
    0x02,0x00,0x00,0x00,0x04,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x46,0x02,0x00,0x00,0x00,
    0x21,0x00,0x05,0x00,0x00,0x00,0x54,0x65,
    0x73,0x74,0x00,0x04,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x46,0x02,0x00,0x00,0x00,0x23,
    0x00,0x05,0x00,0x00,0x00,0x57,0x69,0x6e,
    0x65,0x00,
};

static const BYTE expected_gc_moniker_comparison_data[] =
{
    0x09,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x21,0x00,0x54,0x00,0x45,0x00,0x53,0x00,
    0x54,0x00,0x00,0x00,0x04,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x46,0x23,0x00,0x57,0x00,
    0x49,0x00,0x4e,0x00,0x45,0x00,0x00,0x00,
};

static void test_moniker(
    const char *testname, IMoniker *moniker,
    const BYTE *expected_moniker_marshal_data, unsigned int sizeof_expected_moniker_marshal_data,
    const BYTE *expected_moniker_saved_data, unsigned int sizeof_expected_moniker_saved_data,
    const BYTE *expected_moniker_comparison_data, unsigned int sizeof_expected_moniker_comparison_data,
    LPCWSTR expected_display_name)
{
    IStream * stream;
    IROTData * rotdata;
    HRESULT hr;
    HGLOBAL hglobal;
    LPBYTE moniker_data;
    DWORD moniker_size;
    DWORD i;
    BOOL same = TRUE;
    BYTE buffer[128];
    IMoniker * moniker_proxy;
    LPOLESTR display_name;
    IBindCtx *bindctx;

    hr = IMoniker_IsDirty(moniker);
    ok(hr == S_FALSE, "%s: IMoniker_IsDirty should return S_FALSE, not 0x%08x\n", testname, hr);

    /* Display Name */

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    hr = IMoniker_GetDisplayName(moniker, bindctx, NULL, &display_name);
    ok_ole_success(hr, IMoniker_GetDisplayName);
    ok(!lstrcmpW(display_name, expected_display_name), "%s: display name wasn't what was expected\n", testname);

    CoTaskMemFree(display_name);
    IBindCtx_Release(bindctx);

    hr = IMoniker_IsDirty(moniker);
    ok(hr == S_FALSE, "%s: IMoniker_IsDirty should return S_FALSE, not 0x%08x\n", testname, hr);

    /* IROTData::GetComparisonData test */

    hr = IMoniker_QueryInterface(moniker, &IID_IROTData, (void **)&rotdata);
    ok_ole_success(hr, IMoniker_QueryInterface_IID_IROTData);

    hr = IROTData_GetComparisonData(rotdata, buffer, sizeof(buffer), &moniker_size);
    ok_ole_success(hr, IROTData_GetComparisonData);

    if (hr != S_OK) moniker_size = 0;

    /* first check we have the right amount of data */
    ok(moniker_size == sizeof_expected_moniker_comparison_data,
        "%s: Size of comparison data differs (expected %d, actual %d)\n",
        testname, sizeof_expected_moniker_comparison_data, moniker_size);

    /* then do a byte-by-byte comparison */
    for (i = 0; i < min(moniker_size, sizeof_expected_moniker_comparison_data); i++)
    {
        if (expected_moniker_comparison_data[i] != buffer[i])
        {
            same = FALSE;
            break;
        }
    }

    ok(same, "%s: Comparison data differs\n", testname);
    if (!same)
    {
        for (i = 0; i < moniker_size; i++)
        {
            if (i % 8 == 0) printf("     ");
            printf("0x%02x,", buffer[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    IROTData_Release(rotdata);
  
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
  
    /* Saving */

    hr = IMoniker_Save(moniker, stream, TRUE);
    ok_ole_success(hr, IMoniker_Save);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_ole_success(hr, GetHGlobalFromStream);

    moniker_size = GlobalSize(hglobal);

    moniker_data = GlobalLock(hglobal);

    /* first check we have the right amount of data */
    ok(moniker_size == round_global_size(sizeof_expected_moniker_saved_data),
        "%s: Size of saved data differs (expected %d, actual %d)\n",
        testname, (DWORD)round_global_size(sizeof_expected_moniker_saved_data), moniker_size);

    /* then do a byte-by-byte comparison */
    for (i = 0; i < min(moniker_size, round_global_size(sizeof_expected_moniker_saved_data)); i++)
    {
        if (expected_moniker_saved_data[i] != moniker_data[i])
        {
            same = FALSE;
            break;
        }
    }

    ok(same, "%s: Saved data differs\n", testname);
    if (!same)
    {
        for (i = 0; i < moniker_size; i++)
        {
            if (i % 8 == 0) printf("     ");
            printf("0x%02x,", moniker_data[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    GlobalUnlock(hglobal);

    IStream_Release(stream);

    /* Marshaling tests */

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_ole_success(hr, GetHGlobalFromStream);

    moniker_size = GlobalSize(hglobal);

    moniker_data = GlobalLock(hglobal);

    /* first check we have the right amount of data */
    ok(moniker_size == round_global_size(sizeof_expected_moniker_marshal_data),
        "%s: Size of marshaled data differs (expected %d, actual %d)\n",
        testname, (DWORD)round_global_size(sizeof_expected_moniker_marshal_data), moniker_size);

    /* then do a byte-by-byte comparison */
    if (expected_moniker_marshal_data)
    {
        for (i = 0; i < min(moniker_size, round_global_size(sizeof_expected_moniker_marshal_data)); i++)
        {
            if (expected_moniker_marshal_data[i] != moniker_data[i])
            {
                same = FALSE;
                break;
            }
        }
    }

    ok(same, "%s: Marshaled data differs\n", testname);
    if (!same)
    {
        for (i = 0; i < moniker_size; i++)
        {
            if (i % 8 == 0) printf("     ");
            printf("0x%02x,", moniker_data[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    GlobalUnlock(hglobal);

    IStream_Seek(stream, llZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&moniker_proxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    IStream_Release(stream);
    IMoniker_Release(moniker_proxy);
}

static void test_class_moniker(void)
{
    HRESULT hr;
    IMoniker *moniker;
    DWORD moniker_type;
    DWORD hash;
    IBindCtx *bindctx;
    IMoniker *inverse;
    IUnknown *unknown;
    FILETIME filetime;

    hr = CreateClassMoniker(&CLSID_StdComponentCategoriesMgr, &moniker);
    ok_ole_success(hr, CreateClassMoniker);
    if (!moniker) return;

    test_moniker("class moniker", moniker, 
        expected_class_moniker_marshal_data, sizeof(expected_class_moniker_marshal_data),
        expected_class_moniker_saved_data, sizeof(expected_class_moniker_saved_data),
        expected_class_moniker_comparison_data, sizeof(expected_class_moniker_comparison_data),
        expected_class_moniker_display_name);

    /* Hashing */

    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);

    ok(hash == CLSID_StdComponentCategoriesMgr.Data1,
        "Hash value != Data1 field of clsid, instead was 0x%08x\n",
        hash);

    /* IsSystemMoniker test */

    hr = IMoniker_IsSystemMoniker(moniker, &moniker_type);
    ok_ole_success(hr, IMoniker_IsSystemMoniker);

    ok(moniker_type == MKSYS_CLASSMONIKER,
        "dwMkSys != MKSYS_CLASSMONIKER, instead was 0x%08x\n",
        moniker_type);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, NULL, NULL, NULL);
    ok(hr == E_NOTIMPL, "IMoniker_IsRunning should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == E_NOTIMPL, "IMoniker_IsRunning should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == MK_E_UNAVAILABLE, "IMoniker_GetTimeOfLastChange should return MK_E_UNAVAILABLE, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok_ole_success(hr, IMoniker_BindToObject);
    IUnknown_Release(unknown);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok_ole_success(hr, IMoniker_BindToStorage);
    IUnknown_Release(unknown);

    IBindCtx_Release(bindctx);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok_ole_success(hr, IMoniker_Inverse);
    IMoniker_Release(inverse);

    IMoniker_Release(moniker);
}

static void test_file_moniker(WCHAR* path)
{
    IStream *stream;
    IMoniker *moniker1 = NULL, *moniker2 = NULL;
    HRESULT hr;

    hr = CreateFileMoniker(path, &moniker1);
    ok_ole_success(hr, CreateFileMoniker); 

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);

    /* Marshal */
    hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker1, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);
    
    /* Rewind */
    hr = IStream_Seek(stream, llZero, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, IStream_Seek);

    /* Unmarshal */
    hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void**)&moniker2);
    ok_ole_success(hr, CoUnmarshalInterface);

    hr = IMoniker_IsEqual(moniker1, moniker2);
    ok_ole_success(hr, IsEqual);

    IStream_Release(stream);
    if (moniker1) 
        IMoniker_Release(moniker1);
    if (moniker2) 
        IMoniker_Release(moniker2);
}

static void test_file_monikers(void)
{
    static WCHAR wszFile[][30] = {
        {'\\', 'w','i','n','d','o','w','s','\\','s','y','s','t','e','m','\\','t','e','s','t','1','.','d','o','c',0},
        {'\\', 'a','b','c','d','e','f','g','\\','h','i','j','k','l','\\','m','n','o','p','q','r','s','t','u','.','m','n','o',0},
        /* These map to themselves in Windows-1252 & 932 (Shift-JIS) */
        {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0},
        /* U+2020 = DAGGER     = 0x86 (1252) = 0x813f (932)
         * U+20AC = EURO SIGN  = 0x80 (1252) =  undef (932)
         * U+0100 .. = Latin extended-A
         */ 
        {0x20ac, 0x2020, 0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108, 0x109, 0x10a, 0x10b, 0x10c,  0},
        };

    int i; 

    trace("ACP is %u\n", GetACP());

    for (i = 0; i < COUNTOF(wszFile); ++i)
    {
        int j ;
        for (j = lstrlenW(wszFile[i]); j > 0; --j)
        {
            wszFile[i][j] = 0;
            test_file_moniker(wszFile[i]);
        }
    }
}

static void test_item_moniker(void)
{
    HRESULT hr;
    IMoniker *moniker;
    DWORD moniker_type;
    DWORD hash;
    IBindCtx *bindctx;
    IMoniker *inverse;
    IUnknown *unknown;
    static const WCHAR wszDelimeter[] = {'!',0};
    static const WCHAR wszObjectName[] = {'T','e','s','t',0};
    static const WCHAR expected_display_name[] = { '!','T','e','s','t',0 };

    hr = CreateItemMoniker(wszDelimeter, wszObjectName, &moniker);
    ok_ole_success(hr, CreateItemMoniker);

    test_moniker("item moniker", moniker, 
        expected_item_moniker_marshal_data, sizeof(expected_item_moniker_marshal_data),
        expected_item_moniker_saved_data, sizeof(expected_item_moniker_saved_data),
        expected_item_moniker_comparison_data, sizeof(expected_item_moniker_comparison_data),
        expected_display_name);

    /* Hashing */

    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);

    ok(hash == 0x73c,
        "Hash value != 0x73c, instead was 0x%08x\n",
        hash);

    /* IsSystemMoniker test */

    hr = IMoniker_IsSystemMoniker(moniker, &moniker_type);
    ok_ole_success(hr, IMoniker_IsSystemMoniker);

    ok(moniker_type == MKSYS_ITEMMONIKER,
        "dwMkSys != MKSYS_ITEMMONIKER, instead was 0x%08x\n",
        moniker_type);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, NULL, NULL, NULL);
    todo_wine
    ok(hr == E_INVALIDARG, "IMoniker_IsRunning should return E_INVALIDARG, not 0x%08x\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == S_FALSE, "IMoniker_IsRunning should return S_FALSE, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_INVALIDARG, "IMoniker_BindToStorage should return E_INVALIDARG, not 0x%08x\n", hr);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_INVALIDARG, "IMoniker_BindToObject should return E_INVALIDARG, not 0x%08x\n", hr);

    IBindCtx_Release(bindctx);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok_ole_success(hr, IMoniker_Inverse);
    IMoniker_Release(inverse);

    IMoniker_Release(moniker);
}

static void test_anti_moniker(void)
{
    HRESULT hr;
    IMoniker *moniker;
    DWORD moniker_type;
    DWORD hash;
    IBindCtx *bindctx;
    FILETIME filetime;
    IMoniker *inverse;
    IUnknown *unknown;
    static const WCHAR expected_display_name[] = { '\\','.','.',0 };

    hr = CreateAntiMoniker(&moniker);
    ok_ole_success(hr, CreateAntiMoniker);
    if (!moniker) return;

    test_moniker("anti moniker", moniker, 
        expected_anti_moniker_marshal_data, sizeof(expected_anti_moniker_marshal_data),
        expected_anti_moniker_saved_data, sizeof(expected_anti_moniker_saved_data),
        expected_anti_moniker_comparison_data, sizeof(expected_anti_moniker_comparison_data),
        expected_display_name);

    /* Hashing */
    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);
    ok(hash == 0x80000001,
        "Hash value != 0x80000001, instead was 0x%08x\n",
        hash);

    /* IsSystemMoniker test */
    hr = IMoniker_IsSystemMoniker(moniker, &moniker_type);
    ok_ole_success(hr, IMoniker_IsSystemMoniker);
    ok(moniker_type == MKSYS_ANTIMONIKER,
        "dwMkSys != MKSYS_ANTIMONIKER, instead was 0x%08x\n",
        moniker_type);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok(hr == MK_E_NOINVERSE, "IMoniker_Inverse should have returned MK_E_NOINVERSE instead of 0x%08x\n", hr);
    ok(inverse == NULL, "inverse should have been set to NULL instead of %p\n", inverse);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == S_FALSE, "IMoniker_IsRunning should return S_FALSE, not 0x%08x\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == E_NOTIMPL, "IMoniker_GetTimeOfLastChange should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_NOTIMPL, "IMoniker_BindToObject should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_NOTIMPL, "IMoniker_BindToStorage should return E_NOTIMPL, not 0x%08x\n", hr);

    IBindCtx_Release(bindctx);

    IMoniker_Release(moniker);
}

static void test_generic_composite_moniker(void)
{
    HRESULT hr;
    IMoniker *moniker;
    IMoniker *moniker1;
    IMoniker *moniker2;
    DWORD moniker_type;
    DWORD hash;
    IBindCtx *bindctx;
    FILETIME filetime;
    IMoniker *inverse;
    IUnknown *unknown;
    static const WCHAR wszDelimeter1[] = {'!',0};
    static const WCHAR wszObjectName1[] = {'T','e','s','t',0};
    static const WCHAR wszDelimeter2[] = {'#',0};
    static const WCHAR wszObjectName2[] = {'W','i','n','e',0};
    static const WCHAR expected_display_name[] = { '!','T','e','s','t','#','W','i','n','e',0 };

    hr = CreateItemMoniker(wszDelimeter1, wszObjectName1, &moniker1);
    ok_ole_success(hr, CreateItemMoniker);
    hr = CreateItemMoniker(wszDelimeter2, wszObjectName2, &moniker2);
    ok_ole_success(hr, CreateItemMoniker);
    hr = CreateGenericComposite(moniker1, moniker2, &moniker);
    ok_ole_success(hr, CreateGenericComposite);

    test_moniker("generic composite moniker", moniker, 
        expected_gc_moniker_marshal_data, sizeof(expected_gc_moniker_marshal_data),
        expected_gc_moniker_saved_data, sizeof(expected_gc_moniker_saved_data),
        expected_gc_moniker_comparison_data, sizeof(expected_gc_moniker_comparison_data),
        expected_display_name);

    /* Hashing */

    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);

    ok(hash == 0xd87,
        "Hash value != 0xd87, instead was 0x%08x\n",
        hash);

    /* IsSystemMoniker test */

    hr = IMoniker_IsSystemMoniker(moniker, &moniker_type);
    ok_ole_success(hr, IMoniker_IsSystemMoniker);

    ok(moniker_type == MKSYS_GENERICCOMPOSITE,
        "dwMkSys != MKSYS_GENERICCOMPOSITE, instead was 0x%08x\n",
        moniker_type);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, NULL, NULL, NULL);
    todo_wine
    ok(hr == E_INVALIDARG, "IMoniker_IsRunning should return E_INVALIDARG, not 0x%08x\n", hr);

    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    todo_wine
    ok(hr == S_FALSE, "IMoniker_IsRunning should return S_FALSE, not 0x%08x\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == MK_E_NOTBINDABLE, "IMoniker_GetTimeOfLastChange should return MK_E_NOTBINDABLE, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    todo_wine
    ok(hr == E_INVALIDARG, "IMoniker_BindToObject should return E_INVALIDARG, not 0x%08x\n", hr);

    todo_wine
    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_INVALIDARG, "IMoniker_BindToStorage should return E_INVALIDARG, not 0x%08x\n", hr);

    IBindCtx_Release(bindctx);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok_ole_success(hr, IMoniker_Inverse);
    IMoniker_Release(inverse);

    IMoniker_Release(moniker);
}

static void test_pointer_moniker(void)
{
    HRESULT hr;
    IMoniker *moniker;
    DWORD moniker_type;
    DWORD hash;
    IBindCtx *bindctx;
    FILETIME filetime;
    IMoniker *inverse;
    IUnknown *unknown;
    IStream *stream;
    IROTData *rotdata;
    LPOLESTR display_name;

    cLocks = 0;

    hr = CreatePointerMoniker((IUnknown *)&Test_ClassFactory, NULL);
    ok(hr == E_INVALIDARG, "CreatePointerMoniker(x, NULL) should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreatePointerMoniker((IUnknown *)&Test_ClassFactory, &moniker);
    ok_ole_success(hr, CreatePointerMoniker);
    if (!moniker) return;

    ok_more_than_one_lock();

    /* Display Name */

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    hr = IMoniker_GetDisplayName(moniker, bindctx, NULL, &display_name);
    ok(hr == E_NOTIMPL, "IMoniker_GetDisplayName should have returned E_NOTIMPL instead of 0x%08x\n", hr);

    IBindCtx_Release(bindctx);

    hr = IMoniker_IsDirty(moniker);
    ok(hr == S_FALSE, "IMoniker_IsDirty should return S_FALSE, not 0x%08x\n", hr);

    /* IROTData::GetComparisonData test */

    hr = IMoniker_QueryInterface(moniker, &IID_IROTData, (void **)&rotdata);
    ok(hr == E_NOINTERFACE, "IMoniker_QueryInterface(IID_IROTData) should have returned E_NOINTERFACE instead of 0x%08x\n", hr);

    /* Saving */

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    hr = IMoniker_Save(moniker, stream, TRUE);
    ok(hr == E_NOTIMPL, "IMoniker_Save should have returned E_NOTIMPL instead of 0x%08x\n", hr);

    IStream_Release(stream);

    /* Hashing */
    hr = IMoniker_Hash(moniker, &hash);
    ok_ole_success(hr, IMoniker_Hash);
    ok(hash == (DWORD)&Test_ClassFactory,
        "Hash value should have been 0x%08x, instead of 0x%08x\n",
        (DWORD)&Test_ClassFactory, hash);

    /* IsSystemMoniker test */
    hr = IMoniker_IsSystemMoniker(moniker, &moniker_type);
    ok_ole_success(hr, IMoniker_IsSystemMoniker);
    ok(moniker_type == MKSYS_POINTERMONIKER,
        "dwMkSys != MKSYS_POINTERMONIKER, instead was 0x%08x\n",
        moniker_type);

    hr = IMoniker_Inverse(moniker, &inverse);
    ok_ole_success(hr, IMoniker_Inverse);
    IMoniker_Release(inverse);

    hr = CreateBindCtx(0, &bindctx);
    ok_ole_success(hr, CreateBindCtx);

    /* IsRunning test */
    hr = IMoniker_IsRunning(moniker, bindctx, NULL, NULL);
    ok(hr == S_OK, "IMoniker_IsRunning should return S_OK, not 0x%08x\n", hr);

    hr = IMoniker_GetTimeOfLastChange(moniker, bindctx, NULL, &filetime);
    ok(hr == E_NOTIMPL, "IMoniker_GetTimeOfLastChange should return E_NOTIMPL, not 0x%08x\n", hr);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok_ole_success(hr, IMoniker_BindToObject);
    IUnknown_Release(unknown);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok_ole_success(hr, IMoniker_BindToStorage);
    IUnknown_Release(unknown);

    IMoniker_Release(moniker);

    ok_no_locks();

    hr = CreatePointerMoniker(NULL, &moniker);
    ok_ole_success(hr, CreatePointerMoniker);

    hr = IMoniker_BindToObject(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_UNEXPECTED, "IMoniker_BindToObject should have returned E_UNEXPECTED instead of 0x%08x\n", hr);

    hr = IMoniker_BindToStorage(moniker, bindctx, NULL, &IID_IUnknown, (void **)&unknown);
    ok(hr == E_UNEXPECTED, "IMoniker_BindToStorage should have returned E_UNEXPECTED instead of 0x%08x\n", hr);

    IBindCtx_Release(bindctx);

    IMoniker_Release(moniker);
}

static void test_bind_context(void)
{
    HRESULT hr;
    IBindCtx *pBindCtx;
    IEnumString *pEnumString;
    BIND_OPTS2 bind_opts;
    HeapUnknown *unknown;
    HeapUnknown *unknown2;
    IUnknown *param_obj;
    ULONG refs;
    static const WCHAR wszParamName[] = {'G','e','m','m','a',0};
    static const WCHAR wszNonExistent[] = {'N','o','n','E','x','i','s','t','e','n','t',0};

    hr = CreateBindCtx(0, NULL);
    ok(hr == E_INVALIDARG, "CreateBindCtx with NULL ppbc should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreateBindCtx(0xdeadbeef, &pBindCtx);
    ok(hr == E_INVALIDARG, "CreateBindCtx with reserved value non-zero should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreateBindCtx(0, &pBindCtx);
    ok_ole_success(hr, "CreateBindCtx");

    bind_opts.cbStruct = -1;
    hr = IBindCtx_GetBindOptions(pBindCtx, (BIND_OPTS *)&bind_opts);
    ok_ole_success(hr, "IBindCtx_GetBindOptions");
    ok(bind_opts.cbStruct == sizeof(bind_opts) ||
       bind_opts.cbStruct == sizeof(bind_opts) + sizeof(void*), /* Vista */
       "bind_opts.cbStruct was %d\n", bind_opts.cbStruct);

    bind_opts.cbStruct = sizeof(BIND_OPTS);
    hr = IBindCtx_GetBindOptions(pBindCtx, (BIND_OPTS *)&bind_opts);
    ok_ole_success(hr, "IBindCtx_GetBindOptions");
    ok(bind_opts.cbStruct == sizeof(BIND_OPTS), "bind_opts.cbStruct was %d\n", bind_opts.cbStruct);

    bind_opts.cbStruct = sizeof(bind_opts);
    hr = IBindCtx_GetBindOptions(pBindCtx, (BIND_OPTS *)&bind_opts);
    ok_ole_success(hr, "IBindCtx_GetBindOptions");
    ok(bind_opts.cbStruct == sizeof(bind_opts), "bind_opts.cbStruct was %d\n", bind_opts.cbStruct);
    ok(bind_opts.grfFlags == 0, "bind_opts.grfFlags was 0x%x instead of 0\n", bind_opts.grfFlags);
    ok(bind_opts.grfMode == STGM_READWRITE, "bind_opts.grfMode was 0x%x instead of STGM_READWRITE\n", bind_opts.grfMode);
    ok(bind_opts.dwTickCountDeadline == 0, "bind_opts.dwTickCountDeadline was %d instead of 0\n", bind_opts.dwTickCountDeadline);
    ok(bind_opts.dwTrackFlags == 0, "bind_opts.dwTrackFlags was 0x%x instead of 0\n", bind_opts.dwTrackFlags);
    ok(bind_opts.dwClassContext == (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER),
        "bind_opts.dwClassContext should have been 0x15 instead of 0x%x\n", bind_opts.dwClassContext);
    ok(bind_opts.locale == GetThreadLocale(), "bind_opts.locale should have been 0x%x instead of 0x%x\n", GetThreadLocale(), bind_opts.locale);
    ok(bind_opts.pServerInfo == NULL, "bind_opts.pServerInfo should have been NULL instead of %p\n", bind_opts.pServerInfo);

    bind_opts.cbStruct = -1;
    hr = IBindCtx_SetBindOptions(pBindCtx, (BIND_OPTS *)&bind_opts);
    ok(hr == E_INVALIDARG, "IBindCtx_SetBindOptions with bad cbStruct should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = IBindCtx_RegisterObjectParam(pBindCtx, (WCHAR *)wszParamName, NULL);
    ok(hr == E_INVALIDARG, "IBindCtx_RegisterObjectParam should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    unknown = HeapAlloc(GetProcessHeap(), 0, sizeof(*unknown));
    unknown->lpVtbl = &HeapUnknown_Vtbl;
    unknown->refs = 1;
    hr = IBindCtx_RegisterObjectParam(pBindCtx, (WCHAR *)wszParamName, (IUnknown *)&unknown->lpVtbl);
    ok_ole_success(hr, "IBindCtx_RegisterObjectParam");

    hr = IBindCtx_GetObjectParam(pBindCtx, (WCHAR *)wszParamName, &param_obj);
    ok_ole_success(hr, "IBindCtx_GetObjectParam");
    IUnknown_Release(param_obj);

    hr = IBindCtx_GetObjectParam(pBindCtx, (WCHAR *)wszNonExistent, &param_obj);
    ok(hr == E_FAIL, "IBindCtx_GetObjectParam with nonexistent key should have failed with E_FAIL instead of 0x%08x\n", hr);
    ok(param_obj == NULL, "IBindCtx_GetObjectParam with nonexistent key should have set output parameter to NULL instead of %p\n", param_obj);

    hr = IBindCtx_RevokeObjectParam(pBindCtx, (WCHAR *)wszNonExistent);
    ok(hr == E_FAIL, "IBindCtx_RevokeObjectParam with nonexistent key should have failed with E_FAIL instead of 0x%08x\n", hr);

    hr = IBindCtx_EnumObjectParam(pBindCtx, &pEnumString);
    ok(hr == E_NOTIMPL, "IBindCtx_EnumObjectParam should have returned E_NOTIMPL instead of 0x%08x\n", hr);
    ok(!pEnumString, "pEnumString should be NULL\n");

    hr = IBindCtx_RegisterObjectBound(pBindCtx, NULL);
    ok_ole_success(hr, "IBindCtx_RegisterObjectBound(NULL)");

    hr = IBindCtx_RevokeObjectBound(pBindCtx, NULL);
    ok(hr == E_INVALIDARG, "IBindCtx_RevokeObjectBound(NULL) should have return E_INVALIDARG instead of 0x%08x\n", hr);

    unknown2 = HeapAlloc(GetProcessHeap(), 0, sizeof(*unknown));
    unknown2->lpVtbl = &HeapUnknown_Vtbl;
    unknown2->refs = 1;
    hr = IBindCtx_RegisterObjectBound(pBindCtx, (IUnknown *)&unknown2->lpVtbl);
    ok_ole_success(hr, "IBindCtx_RegisterObjectBound");

    hr = IBindCtx_RevokeObjectBound(pBindCtx, (IUnknown *)&unknown2->lpVtbl);
    ok_ole_success(hr, "IBindCtx_RevokeObjectBound");

    hr = IBindCtx_RevokeObjectBound(pBindCtx, (IUnknown *)&unknown2->lpVtbl);
    ok(hr == MK_E_NOTBOUND, "IBindCtx_RevokeObjectBound with not bound object should have returned MK_E_NOTBOUND instead of 0x%08x\n", hr);

    IBindCtx_Release(pBindCtx);

    refs = IUnknown_Release((IUnknown *)&unknown->lpVtbl);
    ok(!refs, "object param should have been destroyed, instead of having %d refs\n", refs);

    refs = IUnknown_Release((IUnknown *)&unknown2->lpVtbl);
    ok(!refs, "bound object should have been destroyed, instead of having %d refs\n", refs);
}

START_TEST(moniker)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_ROT();
    test_ROT_multiple_entries();
    test_MkParseDisplayName();
    test_class_moniker();
    test_file_monikers();
    test_item_moniker();
    test_anti_moniker();
    test_generic_composite_moniker();
    test_pointer_moniker();

    /* FIXME: test moniker creation funcs and parsing other moniker formats */

    test_bind_context();

    CoUninitialize();
}
