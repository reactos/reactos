/*
 * Component Object Tests
 *
 * Copyright 2005 Robert Shearman
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

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlguid.h"
#include "urlmon.h" /* for CLSID_FileProtocol */

#include "wine/test.h"

/* functions that are not present on all versions of Windows */
HRESULT (WINAPI * pCoInitializeEx)(LPVOID lpReserved, DWORD dwCoInit);
HRESULT (WINAPI * pCoGetObjectContext)(REFIID riid, LPVOID *ppv);

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)
#define ok_more_than_one_lock() ok(cLocks > 0, "Number of locks should be > 0, but actually is %d\n", cLocks)
#define ok_no_locks() ok(cLocks == 0, "Number of locks should be 0, but actually is %d\n", cLocks)

static const CLSID CLSID_non_existent =   { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const CLSID CLSID_CDeviceMoniker = { 0x4315d437, 0x5b8c, 0x11d0, { 0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86 } };
static WCHAR devicedotone[] = {'d','e','v','i','c','e','.','1',0};
static const WCHAR wszNonExistent[] = {'N','o','n','E','x','i','s','t','e','n','t',0};
static WCHAR wszCLSID_CDeviceMoniker[] =
{
    '{',
    '4','3','1','5','d','4','3','7','-',
    '5','b','8','c','-',
    '1','1','d','0','-',
    'b','d','3','b','-',
    '0','0','a','0','c','9','1','1','c','e','8','6',
    '}',0
};

static const IID IID_IWineTest =
{
    0x5201163f,
    0x8164,
    0x4fd0,
    {0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd}
}; /* 5201163f-8164-4fd0-a1a2-5d5a3654d3bd */
static const CLSID CLSID_WineOOPTest = {
    0x5201163f,
    0x8164,
    0x4fd0,
    {0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd}
}; /* 5201163f-8164-4fd0-a1a2-5d5a3654d3bd */

static LONG cLocks;

static void LockModule(void)
{
    InterlockedIncrement(&cLocks);
}

static void UnlockModule(void)
{
    InterlockedDecrement(&cLocks);
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
        *ppvObj = (LPVOID)iface;
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
    *ppvObj = NULL;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;
    return E_NOINTERFACE;
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

static void test_ProgIDFromCLSID(void)
{
    LPWSTR progid;
    HRESULT hr = ProgIDFromCLSID(&CLSID_CDeviceMoniker, &progid);
    ok(hr == S_OK, "ProgIDFromCLSID failed with error 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok(!lstrcmpiW(progid, devicedotone), "Didn't get expected prog ID\n");
        CoTaskMemFree(progid);
    }

    progid = (LPWSTR)0xdeadbeef;
    hr = ProgIDFromCLSID(&CLSID_non_existent, &progid);
    ok(hr == REGDB_E_CLASSNOTREG, "ProgIDFromCLSID returned %08x\n", hr);
    ok(progid == NULL, "ProgIDFromCLSID returns with progid %p\n", progid);

    hr = ProgIDFromCLSID(&CLSID_CDeviceMoniker, NULL);
    ok(hr == E_INVALIDARG, "ProgIDFromCLSID should return E_INVALIDARG instead of 0x%08x\n", hr);
}

static void test_CLSIDFromProgID(void)
{
    CLSID clsid;
    HRESULT hr = CLSIDFromProgID(devicedotone, &clsid);
    ok(hr == S_OK, "CLSIDFromProgID failed with error 0x%08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");

    hr = CLSIDFromString(devicedotone, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");

    /* test some failure cases */

    hr = CLSIDFromProgID(wszNonExistent, NULL);
    ok(hr == E_INVALIDARG, "CLSIDFromProgID should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CLSIDFromProgID(NULL, &clsid);
    ok(hr == E_INVALIDARG, "CLSIDFromProgID should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    memset(&clsid, 0xcc, sizeof(clsid));
    hr = CLSIDFromProgID(wszNonExistent, &clsid);
    ok(hr == CO_E_CLASSSTRING, "CLSIDFromProgID on nonexistent ProgID should have returned CO_E_CLASSSTRING instead of 0x%08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "CLSIDFromProgID should have set clsid to all-zeros on failure\n");
}

static void test_CLSIDFromString(void)
{
    CLSID clsid;
    HRESULT hr = CLSIDFromString(wszCLSID_CDeviceMoniker, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_CDeviceMoniker), "clsid wasn't equal to CLSID_CDeviceMoniker\n");

    hr = CLSIDFromString(NULL, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "clsid wasn't equal to CLSID_NULL\n");
}

static void test_StringFromGUID2(void)
{
  WCHAR str[50];
  int len;
  /* Test corner cases for buffer size */
  len = StringFromGUID2(&CLSID_CDeviceMoniker,str,50);
  ok(len == 39, "len: %d (expected 39)\n", len);
  ok(!lstrcmpiW(str, wszCLSID_CDeviceMoniker),"string wan't equal for CLSID_CDeviceMoniker\n");

  memset(str,0,sizeof str);
  len = StringFromGUID2(&CLSID_CDeviceMoniker,str,39);
  ok(len == 39, "len: %d (expected 39)\n", len);
  ok(!lstrcmpiW(str, wszCLSID_CDeviceMoniker),"string wan't equal for CLSID_CDeviceMoniker\n");

  len = StringFromGUID2(&CLSID_CDeviceMoniker,str,38);
  ok(len == 0, "len: %d (expected 0)\n", len);

  len = StringFromGUID2(&CLSID_CDeviceMoniker,str,30);
  ok(len == 0, "len: %d (expected 0)\n", len);
}

static void test_CoCreateInstance(void)
{
    REFCLSID rclsid = &CLSID_MyComputer;
    IUnknown *pUnk = (IUnknown *)0xdeadbeef;
    HRESULT hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);
    ok(pUnk == NULL, "CoCreateInstance should have changed the passed in pointer to NULL, instead of %p\n", pUnk);

    OleInitialize(NULL);
    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok_ole_success(hr, "CoCreateInstance");
    if(pUnk) IUnknown_Release(pUnk);
    OleUninitialize();

    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);
}

static void test_CoGetClassObject(void)
{
    IUnknown *pUnk = (IUnknown *)0xdeadbeef;
    HRESULT hr = CoGetClassObject(&CLSID_MyComputer, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoGetClassObject should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);
    ok(pUnk == NULL, "CoGetClassObject should have changed the passed in pointer to NULL, instead of %p\n", pUnk);

    hr = CoGetClassObject(&CLSID_MyComputer, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, NULL);
    ok(hr == E_INVALIDARG ||
       broken(hr == CO_E_NOTINITIALIZED), /* win9x */
       "CoGetClassObject should have returned E_INVALIDARG instead of 0x%08x\n", hr);
}

static ATOM register_dummy_class(void)
{
    WNDCLASS wc =
    {
        0,
        DefWindowProc,
        0,
        0,
        GetModuleHandle(NULL),
        NULL,
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        TEXT("WineOleTestClass"),
    };

    return RegisterClass(&wc);
}

static void test_ole_menu(void)
{
	HWND hwndFrame;
	HRESULT hr;

	hwndFrame = CreateWindow(MAKEINTATOM(register_dummy_class()), "Test", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	hr = OleSetMenuDescriptor(NULL, hwndFrame, NULL, NULL, NULL);
	todo_wine ok_ole_success(hr, "OleSetMenuDescriptor");

	DestroyWindow(hwndFrame);
}


static HRESULT WINAPI MessageFilter_QueryInterface(IMessageFilter *iface, REFIID riid, void ** ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = (LPVOID)iface;
        IMessageFilter_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI MessageFilter_AddRef(IMessageFilter *iface)
{
    return 2; /* non-heap object */
}

static ULONG WINAPI MessageFilter_Release(IMessageFilter *iface)
{
    return 1; /* non-heap object */
}

static DWORD WINAPI MessageFilter_HandleInComingCall(
  IMessageFilter *iface,
  DWORD dwCallType,
  HTASK threadIDCaller,
  DWORD dwTickCount,
  LPINTERFACEINFO lpInterfaceInfo)
{
    trace("HandleInComingCall\n");
    return SERVERCALL_ISHANDLED;
}

static DWORD WINAPI MessageFilter_RetryRejectedCall(
  IMessageFilter *iface,
  HTASK threadIDCallee,
  DWORD dwTickCount,
  DWORD dwRejectType)
{
    trace("RetryRejectedCall\n");
    return 0;
}

static DWORD WINAPI MessageFilter_MessagePending(
  IMessageFilter *iface,
  HTASK threadIDCallee,
  DWORD dwTickCount,
  DWORD dwPendingType)
{
    trace("MessagePending\n");
    return PENDINGMSG_WAITNOPROCESS;
}

static const IMessageFilterVtbl MessageFilter_Vtbl =
{
    MessageFilter_QueryInterface,
    MessageFilter_AddRef,
    MessageFilter_Release,
    MessageFilter_HandleInComingCall,
    MessageFilter_RetryRejectedCall,
    MessageFilter_MessagePending
};

static IMessageFilter MessageFilter = { &MessageFilter_Vtbl };

static void test_CoRegisterMessageFilter(void)
{
    HRESULT hr;
    IMessageFilter *prev_filter;

    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok(hr == CO_E_NOT_SUPPORTED,
        "CoRegisterMessageFilter should have failed with CO_E_NOT_SUPPORTED instead of 0x%08x\n",
        hr);

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);
    prev_filter = (IMessageFilter *)0xdeadbeef;
    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok(hr == CO_E_NOT_SUPPORTED,
        "CoRegisterMessageFilter should have failed with CO_E_NOT_SUPPORTED instead of 0x%08x\n",
        hr);
    ok(prev_filter == (IMessageFilter *)0xdeadbeef,
        "prev_filter should have been set to %p\n", prev_filter);
    CoUninitialize();

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok_ole_success(hr, "CoRegisterMessageFilter");

    prev_filter = (IMessageFilter *)0xdeadbeef;
    hr = CoRegisterMessageFilter(NULL, &prev_filter);
    ok_ole_success(hr, "CoRegisterMessageFilter");
    ok(prev_filter == NULL, "prev_filter should have been set to NULL instead of %p\n", prev_filter);

    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok_ole_success(hr, "CoRegisterMessageFilter");
    ok(prev_filter == NULL, "prev_filter should have been set to NULL instead of %p\n", prev_filter);

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok_ole_success(hr, "CoRegisterMessageFilter");

    CoUninitialize();
}

static HRESULT WINAPI Test_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWineTest))
    {
        *ppvObj = (LPVOID)iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IUnknown_AddRef(LPUNKNOWN iface)
{
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IUnknown_Release(LPUNKNOWN iface)
{
    return 1; /* non-heap-based object */
}

static const IUnknownVtbl TestUnknown_Vtbl =
{
    Test_IUnknown_QueryInterface,
    Test_IUnknown_AddRef,
    Test_IUnknown_Release,
};

static IUnknown Test_Unknown = { &TestUnknown_Vtbl };

static HRESULT WINAPI PSFactoryBuffer_QueryInterface(
    IPSFactoryBuffer * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPSFactoryBuffer))
    {
        *ppvObject = This;
        IPSFactoryBuffer_AddRef(This);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI PSFactoryBuffer_AddRef(
    IPSFactoryBuffer * This)
{
    return 2;
}

static ULONG WINAPI PSFactoryBuffer_Release(
    IPSFactoryBuffer * This)
{
    return 1;
}

static HRESULT WINAPI PSFactoryBuffer_CreateProxy(
    IPSFactoryBuffer * This,
    /* [in] */ IUnknown *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [out] */ IRpcProxyBuffer **ppProxy,
    /* [out] */ void **ppv)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI PSFactoryBuffer_CreateStub(
    IPSFactoryBuffer * This,
    /* [in] */ REFIID riid,
    /* [unique][in] */ IUnknown *pUnkServer,
    /* [out] */ IRpcStubBuffer **ppStub)
{
    return E_NOTIMPL;
}

static IPSFactoryBufferVtbl PSFactoryBufferVtbl =
{
    PSFactoryBuffer_QueryInterface,
    PSFactoryBuffer_AddRef,
    PSFactoryBuffer_Release,
    PSFactoryBuffer_CreateProxy,
    PSFactoryBuffer_CreateStub
};

static IPSFactoryBuffer PSFactoryBuffer = { &PSFactoryBufferVtbl };

static const CLSID CLSID_WineTestPSFactoryBuffer =
{
    0x52011640,
    0x8164,
    0x4fd0,
    {0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd}
}; /* 52011640-8164-4fd0-a1a2-5d5a3654d3bd */

static void test_CoRegisterPSClsid(void)
{
    HRESULT hr;
    DWORD dwRegistrationKey;
    IStream *stream;
    CLSID clsid;

    hr = CoRegisterPSClsid(&IID_IWineTest, &CLSID_WineTestPSFactoryBuffer);
    ok(hr == CO_E_NOTINITIALIZED, "CoRegisterPSClsid should have returened CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterClassObject(&CLSID_WineTestPSFactoryBuffer, (IUnknown *)&PSFactoryBuffer,
        CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &dwRegistrationKey);
    ok_ole_success(hr, "CoRegisterClassObject");

    hr = CoRegisterPSClsid(&IID_IWineTest, &CLSID_WineTestPSFactoryBuffer);
    ok_ole_success(hr, "CoRegisterPSClsid");

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = CoMarshalInterface(stream, &IID_IWineTest, (IUnknown *)&Test_Unknown, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_NOTIMPL, "CoMarshalInterface should have returned E_NOTIMPL instead of 0x%08x\n", hr);
    IStream_Release(stream);

    hr = CoRevokeClassObject(dwRegistrationKey);
    ok_ole_success(hr, "CoRevokeClassObject");

    CoUninitialize();

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok(hr == REGDB_E_IIDNOTREG, "CoGetPSClsid should have returned REGDB_E_IIDNOTREG instead of 0x%08x\n", hr);

    CoUninitialize();
}

static void test_CoGetPSClsid(void)
{
    HRESULT hr;
    CLSID clsid;

    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok(hr == CO_E_NOTINITIALIZED,
       "CoGetPSClsid should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n",
       hr);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok(hr == REGDB_E_IIDNOTREG,
       "CoGetPSClsid for random IID returned 0x%08x instead of REGDB_E_IIDNOTREG\n",
       hr);

    hr = CoGetPSClsid(&IID_IClassFactory, NULL);
    ok(hr == E_INVALIDARG,
       "CoGetPSClsid for null clsid returned 0x%08x instead of E_INVALIDARG\n",
       hr);

    CoUninitialize();
}

/* basic test, mainly for invalid arguments. see marshal.c for more */
static void test_CoUnmarshalInterface(void)
{
    IUnknown *pProxy;
    IStream *pStream;
    HRESULT hr;

    hr = CoUnmarshalInterface(NULL, &IID_IUnknown, (void **)&pProxy);
    ok(hr == E_INVALIDARG, "CoUnmarshalInterface should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    todo_wine
    ok(hr == CO_E_NOTINITIALIZED, "CoUnmarshalInterface should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    ok(hr == STG_E_READFAULT, "CoUnmarshalInterface should have returned STG_E_READFAULT instead of 0x%08x\n", hr);

    CoUninitialize();

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, NULL);
    ok(hr == E_INVALIDARG, "CoUnmarshalInterface should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    IStream_Release(pStream);
}

static void test_CoGetInterfaceAndReleaseStream(void)
{
    HRESULT hr;
    IUnknown *pUnk;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetInterfaceAndReleaseStream(NULL, &IID_IUnknown, (void**)&pUnk);
    ok(hr == E_INVALIDARG, "hr %08x\n", hr);

    CoUninitialize();
}

/* basic test, mainly for invalid arguments. see marshal.c for more */
static void test_CoMarshalInterface(void)
{
    IStream *pStream;
    HRESULT hr;
    static const LARGE_INTEGER llZero;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = CoMarshalInterface(pStream, &IID_IUnknown, NULL, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_INVALIDARG, "CoMarshalInterface should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CoMarshalInterface(NULL, &IID_IUnknown, (IUnknown *)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_INVALIDARG, "CoMarshalInterface should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CoMarshalInterface(pStream, &IID_IUnknown, (IUnknown *)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, "CoMarshalInterface");

    /* stream not rewound */
    hr = CoReleaseMarshalData(pStream);
    ok(hr == STG_E_READFAULT, "CoReleaseMarshalData should have returned STG_E_READFAULT instead of 0x%08x\n", hr);

    hr = IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");

    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, "CoReleaseMarshalData");

    IStream_Release(pStream);

    CoUninitialize();
}

static void test_CoMarshalInterThreadInterfaceInStream(void)
{
    IStream *pStream;
    HRESULT hr;
    IClassFactory *pProxy;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    cLocks = 0;

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, (IUnknown *)&Test_ClassFactory, NULL);
    ok(hr == E_INVALIDARG, "CoMarshalInterThreadInterfaceInStream should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, NULL, &pStream);
    ok(hr == E_INVALIDARG, "CoMarshalInterThreadInterfaceInStream should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    ok_no_locks();

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, (IUnknown *)&Test_ClassFactory, &pStream);
    ok_ole_success(hr, "CoMarshalInterThreadInterfaceInStream");

    ok_more_than_one_lock();

    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, "CoUnmarshalInterface");

    IClassFactory_Release(pProxy);
    IStream_Release(pStream);

    ok_no_locks();

    CoUninitialize();
}

static void test_CoRegisterClassObject(void)
{
    DWORD cookie;
    HRESULT hr;
    IClassFactory *pcf;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* CLSCTX_INPROC_SERVER */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_MULTI_SEPARATE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* CLSCTX_LOCAL_SERVER */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_MULTI_SEPARATE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* test whether registered class becomes invalid when apartment is destroyed */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    CoUninitialize();
    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL,
                          &IID_IClassFactory, (void **)&pcf);
    ok(hr == REGDB_E_CLASSNOTREG, "object registered in an apartment shouldn't accessible after it is destroyed\n");

    /* crashes with at least win9x DCOM! */
    if (0)
        hr = CoRevokeClassObject(cookie);

    CoUninitialize();
}

static HRESULT get_class_object(CLSCTX clsctx)
{
    HRESULT hr;
    IClassFactory *pcf;

    hr = CoGetClassObject(&CLSID_WineOOPTest, clsctx, NULL, &IID_IClassFactory,
                          (void **)&pcf);

    if (SUCCEEDED(hr))
        IClassFactory_Release(pcf);

    return hr;
}

static DWORD CALLBACK get_class_object_thread(LPVOID pv)
{
    CLSCTX clsctx = (CLSCTX)(DWORD_PTR)pv;
    HRESULT hr;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = get_class_object(clsctx);

    CoUninitialize();

    return hr;
}

static DWORD CALLBACK get_class_object_proxy_thread(LPVOID pv)
{
    CLSCTX clsctx = (CLSCTX)(DWORD_PTR)pv;
    HRESULT hr;
    IClassFactory *pcf;
    IMultiQI *pMQI;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetClassObject(&CLSID_WineOOPTest, clsctx, NULL, &IID_IClassFactory,
                          (void **)&pcf);

    if (SUCCEEDED(hr))
    {
        hr = IClassFactory_QueryInterface(pcf, &IID_IMultiQI, (void **)&pMQI);
        if (SUCCEEDED(hr))
            IMultiQI_Release(pMQI);
        IClassFactory_Release(pcf);
    }

    CoUninitialize();

    return hr;
}

static DWORD CALLBACK register_class_object_thread(LPVOID pv)
{
    HRESULT hr;
    DWORD cookie;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);

    CoUninitialize();

    return hr;
}

static DWORD CALLBACK revoke_class_object_thread(LPVOID pv)
{
    DWORD cookie = (DWORD_PTR)pv;
    HRESULT hr;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRevokeClassObject(cookie);

    CoUninitialize();

    return hr;
}

static void test_registered_object_thread_affinity(void)
{
    HRESULT hr;
    DWORD cookie;
    HANDLE thread;
    DWORD tid;
    DWORD exitcode;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* CLSCTX_INPROC_SERVER */

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    thread = CreateThread(NULL, 0, get_class_object_thread, (LPVOID)CLSCTX_INPROC_SERVER, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    WaitForSingleObject(thread, INFINITE);
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == REGDB_E_CLASSNOTREG, "CoGetClassObject on inproc object "
       "registered in different thread should return REGDB_E_CLASSNOTREG "
       "instead of 0x%08x\n", hr);

    hr = get_class_object(CLSCTX_INPROC_SERVER);
    ok(hr == S_OK, "CoGetClassObject on inproc object registered in same "
       "thread should return S_OK instead of 0x%08x\n", hr);

    thread = CreateThread(NULL, 0, register_class_object_thread, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    WaitForSingleObject(thread, INFINITE);
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "CoRegisterClassObject with same CLSID but in different thread should return S_OK instead of 0x%08x\n", hr);

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* CLSCTX_LOCAL_SERVER */

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    thread = CreateThread(NULL, 0, get_class_object_proxy_thread, (LPVOID)CLSCTX_LOCAL_SERVER, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) == WAIT_OBJECT_0 + 1)
    {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "CoGetClassObject on local server object "
       "registered in different thread should return S_OK "
       "instead of 0x%08x\n", hr);

    hr = get_class_object(CLSCTX_LOCAL_SERVER);
    ok(hr == S_OK, "CoGetClassObject on local server object registered in same "
       "thread should return S_OK instead of 0x%08x\n", hr);

    thread = CreateThread(NULL, 0, revoke_class_object_thread, (LPVOID)cookie, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    WaitForSingleObject(thread, INFINITE);
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == RPC_E_WRONG_THREAD, "CoRevokeClassObject called from different "
       "thread to where registered should return RPC_E_WRONG_THREAD instead of 0x%08x\n", hr);

    thread = CreateThread(NULL, 0, register_class_object_thread, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    WaitForSingleObject(thread, INFINITE);
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "CoRegisterClassObject with same CLSID but in different "
        "thread should return S_OK instead of 0x%08x\n", hr);

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    CoUninitialize();
}

static DWORD CALLBACK free_libraries_thread(LPVOID p)
{
    CoFreeUnusedLibraries();
    return 0;
}

static inline BOOL is_module_loaded(const char *module)
{
    return GetModuleHandle(module) ? TRUE : FALSE;
}

static void test_CoFreeUnusedLibraries(void)
{
    HRESULT hr;
    IUnknown *pUnk;
    DWORD tid;
    HANDLE thread;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    ok(!is_module_loaded("urlmon.dll"), "urlmon.dll shouldn't be loaded\n");

    hr = CoCreateInstance(&CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        trace("IE not installed so can't run CoFreeUnusedLibraries test\n");
        return;
    }
    ok_ole_success(hr, "CoCreateInstance");

    ok(is_module_loaded("urlmon.dll"), "urlmon.dll should be loaded\n");

    ok(pUnk != NULL ||
       broken(pUnk == NULL), /* win9x */
       "Expected a valid pointer\n");
    if (pUnk)
        IUnknown_Release(pUnk);

    ok(is_module_loaded("urlmon.dll"), "urlmon.dll should be loaded\n");

    thread = CreateThread(NULL, 0, free_libraries_thread, NULL, 0, &tid);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ok(is_module_loaded("urlmon.dll"), "urlmon.dll should be loaded\n");

    CoFreeUnusedLibraries();

    ok(!is_module_loaded("urlmon.dll"), "urlmon.dll shouldn't be loaded\n");

    CoUninitialize();
}

static void test_CoGetObjectContext(void)
{
    HRESULT hr;
    ULONG refs;
    IComThreadingInfo *pComThreadingInfo;
    APTTYPE apttype;
    THDTYPE thdtype;

    if (!pCoGetObjectContext)
    {
        skip("CoGetObjectContext not present\n");
        return;
    }

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok(hr == CO_E_NOTINITIALIZED, "CoGetObjectContext should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);
    ok(pComThreadingInfo == NULL, "pComThreadingInfo should have been set to NULL\n");

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok_ole_success(hr, "CoGetObjectContext");

    hr = IComThreadingInfo_GetCurrentApartmentType(pComThreadingInfo, &apttype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentApartmentType");
    ok(apttype == APTTYPE_MAINSTA, "apartment type should be APTTYPE_MAINSTA instead of %d\n", apttype);

    hr = IComThreadingInfo_GetCurrentThreadType(pComThreadingInfo, &thdtype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentThreadType");
    ok(thdtype == THDTYPE_PROCESSMESSAGES, "thread type should be THDTYPE_PROCESSMESSAGES instead of %d\n", thdtype);

    refs = IComThreadingInfo_Release(pComThreadingInfo);
    ok(refs == 0, "pComThreadingInfo should have 0 refs instead of %d refs\n", refs);

    CoUninitialize();

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok_ole_success(hr, "CoGetObjectContext");

    hr = IComThreadingInfo_GetCurrentApartmentType(pComThreadingInfo, &apttype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentApartmentType");
    ok(apttype == APTTYPE_MTA, "apartment type should be APTTYPE_MTA instead of %d\n", apttype);

    hr = IComThreadingInfo_GetCurrentThreadType(pComThreadingInfo, &thdtype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentThreadType");
    ok(thdtype == THDTYPE_BLOCKMESSAGES, "thread type should be THDTYPE_BLOCKMESSAGES instead of %d\n", thdtype);

    refs = IComThreadingInfo_Release(pComThreadingInfo);
    ok(refs == 0, "pComThreadingInfo should have 0 refs instead of %d refs\n", refs);

    CoUninitialize();
}

START_TEST(compobj)
{
    skip("Skipping compobj tests\n");
    return;

    HMODULE hOle32 = GetModuleHandle("ole32");
    pCoGetObjectContext = (void*)GetProcAddress(hOle32, "CoGetObjectContext");
    if (!(pCoInitializeEx = (void*)GetProcAddress(hOle32, "CoInitializeEx")))
    {
        trace("You need DCOM95 installed to run this test\n");
        return;
    }

    test_ProgIDFromCLSID();
    test_CLSIDFromProgID();
    test_CLSIDFromString();
    test_StringFromGUID2();
    test_CoCreateInstance();
    test_ole_menu();
    test_CoGetClassObject();
    test_CoRegisterMessageFilter();
    test_CoRegisterPSClsid();
    test_CoGetPSClsid();
    test_CoUnmarshalInterface();
    test_CoGetInterfaceAndReleaseStream();
    test_CoMarshalInterface();
    test_CoMarshalInterThreadInterfaceInStream();
    test_CoRegisterClassObject();
    test_registered_object_thread_affinity();
    test_CoFreeUnusedLibraries();
    test_CoGetObjectContext();
}
