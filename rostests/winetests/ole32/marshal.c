/*
 * Marshaling Tests
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define _WIN32_DCOM
#define COBJMACROS
#define CONST_VTABLE

//#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
#include <ole2.h>
//#include "objbase.h"
//#include "olectl.h"
#include <shlguid.h>
//#include "shobjidl.h"
//#include "initguid.h"

#include <wine/test.h>

DEFINE_GUID(CLSID_StdGlobalInterfaceTable,0x00000323,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_ManualResetEvent,       0x0000032c,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

/* functions that are not present on all versions of Windows */
static HRESULT (WINAPI * pCoInitializeEx)(LPVOID lpReserved, DWORD dwCoInit);
static HRESULT (WINAPI *pDllGetClassObject)(REFCLSID,REFIID,LPVOID);

/* helper macros to make tests a bit leaner */
#define ok_more_than_one_lock() ok(cLocks > 0, "Number of locks should be > 0, but actually is %d\n", cLocks)
#define ok_no_locks() ok(cLocks == 0, "Number of locks should be 0, but actually is %d\n", cLocks)
#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08x\n", hr)
#define ok_non_zero_external_conn() do {if (with_external_conn) ok(external_connections, "got no external connections\n");} while(0);
#define ok_zero_external_conn() do {if (with_external_conn) ok(!external_connections, "got %d external connections\n", external_connections);} while(0);
#define ok_last_release_closes(b) do {if (with_external_conn) ok(last_release_closes == b, "got %d expected %d\n", last_release_closes, b);} while(0);

static const IID IID_IWineTest =
{
    0x5201163f,
    0x8164,
    0x4fd0,
    {0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd}
}; /* 5201163f-8164-4fd0-a1a2-5d5a3654d3bd */

static const IID IID_IRemUnknown =
{
    0x00000131,
    0x0000,
    0x0000,
    {0xc0,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
};

#define EXTENTID_WineTest IID_IWineTest
#define CLSID_WineTest IID_IWineTest

static const CLSID CLSID_WineOOPTest =
{
    0x5201163f,
    0x8164,
    0x4fd0,
    {0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd}
}; /* 5201163f-8164-4fd0-a1a2-5d5a3654d3bd */

static void test_cocreateinstance_proxy(void)
{
    IUnknown *pProxy;
    IMultiQI *pMQI;
    HRESULT hr;

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(&CLSID_ShellDesktop, NULL, CLSCTX_INPROC, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, CoCreateInstance);
    hr = IUnknown_QueryInterface(pProxy, &IID_IMultiQI, (void **)&pMQI);
    ok(hr == S_OK, "created object is not a proxy, so was created in the wrong apartment\n");
    if (hr == S_OK)
        IMultiQI_Release(pMQI);
    IUnknown_Release(pProxy);

    CoUninitialize();
}

static const LARGE_INTEGER ullZero;
static LONG cLocks;

static void LockModule(void)
{
    InterlockedIncrement(&cLocks);
}

static void UnlockModule(void)
{
    InterlockedDecrement(&cLocks);
}

static BOOL with_external_conn;
static DWORD external_connections;
static BOOL last_release_closes;

static HRESULT WINAPI ExternalConnection_QueryInterface(IExternalConnection *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ExternalConnection_AddRef(IExternalConnection *iface)
{
    return 2;
}

static ULONG WINAPI ExternalConnection_Release(IExternalConnection *iface)
{
    return 1;
}

static DWORD WINAPI ExternalConnection_AddConnection(IExternalConnection *iface, DWORD extconn, DWORD reserved)
{
    trace("add connection\n");
    return ++external_connections;
}


static DWORD WINAPI ExternalConnection_ReleaseConnection(IExternalConnection *iface, DWORD extconn,
        DWORD reserved, BOOL fLastReleaseCloses)
{
    trace("release connection %d\n", fLastReleaseCloses);
    last_release_closes = fLastReleaseCloses;
    return --external_connections;
}

static const IExternalConnectionVtbl ExternalConnectionVtbl = {
    ExternalConnection_QueryInterface,
    ExternalConnection_AddRef,
    ExternalConnection_Release,
    ExternalConnection_AddConnection,
    ExternalConnection_ReleaseConnection
};

static IExternalConnection ExternalConnection = { &ExternalConnectionVtbl };


static HRESULT WINAPI Test_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IUnknown_AddRef(LPUNKNOWN iface)
{
    LockModule();
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IUnknown_Release(LPUNKNOWN iface)
{
    UnlockModule();
    return 1; /* non-heap-based object */
}

static const IUnknownVtbl TestUnknown_Vtbl =
{
    Test_IUnknown_QueryInterface,
    Test_IUnknown_AddRef,
    Test_IUnknown_Release,
};

static IUnknown Test_Unknown = { &TestUnknown_Vtbl };

static ULONG WINAPI TestCrash_IUnknown_Release(LPUNKNOWN iface)
{
    UnlockModule();
    if(!cLocks) {
        trace("crashing...\n");
        *(int**)0xc = 0;
    }
    return 1; /* non-heap-based object */
}

static const IUnknownVtbl TestCrashUnknown_Vtbl =
{
    Test_IUnknown_QueryInterface,
    Test_IUnknown_AddRef,
    TestCrash_IUnknown_Release,
};

static IUnknown TestCrash_Unknown = { &TestCrashUnknown_Vtbl };

static HRESULT WINAPI Test_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory) ||
        /* the only other interface Wine is currently able to marshal (for testing two proxies) */
        IsEqualGUID(riid, &IID_IRemUnknown))
    {
        *ppvObj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    if (with_external_conn && IsEqualGUID(riid, &IID_IExternalConnection))
    {
        *ppvObj = &ExternalConnection;
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
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;
    return IUnknown_QueryInterface((IUnknown*)&Test_Unknown, riid, ppvObj);
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

#define RELEASEMARSHALDATA WM_USER

struct host_object_data
{
    IStream *stream;
    IID iid;
    IUnknown *object;
    MSHLFLAGS marshal_flags;
    HANDLE marshal_event;
    IMessageFilter *filter;
};

static DWORD CALLBACK host_object_proc(LPVOID p)
{
    struct host_object_data *data = p;
    HRESULT hr;
    MSG msg;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (data->filter)
    {
        IMessageFilter * prev_filter = NULL;
        hr = CoRegisterMessageFilter(data->filter, &prev_filter);
        if (prev_filter) IMessageFilter_Release(prev_filter);
        ok_ole_success(hr, CoRegisterMessageFilter);
    }

    hr = CoMarshalInterface(data->stream, &data->iid, data->object, MSHCTX_INPROC, NULL, data->marshal_flags);
    ok_ole_success(hr, CoMarshalInterface);

    /* force the message queue to be created before signaling parent thread */
    PeekMessageA(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(data->marshal_event);

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == RELEASEMARSHALDATA)
        {
            CoReleaseMarshalData(data->stream);
            SetEvent((HANDLE)msg.lParam);
        }
        else
            DispatchMessageA(&msg);
    }

    HeapFree(GetProcessHeap(), 0, data);

    CoUninitialize();

    return hr;
}

static DWORD start_host_object2(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, IMessageFilter *filter, HANDLE *thread)
{
    DWORD tid = 0;
    HANDLE marshal_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    struct host_object_data *data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data));

    data->stream = stream;
    data->iid = *riid;
    data->object = object;
    data->marshal_flags = marshal_flags;
    data->marshal_event = marshal_event;
    data->filter = filter;

    *thread = CreateThread(NULL, 0, host_object_proc, data, 0, &tid);

    /* wait for marshaling to complete before returning */
    ok( !WaitForSingleObject(marshal_event, 10000), "wait timed out\n" );
    CloseHandle(marshal_event);

    return tid;
}

static DWORD start_host_object(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, HANDLE *thread)
{
    return start_host_object2(stream, riid, object, marshal_flags, NULL, thread);
}

/* asks thread to release the marshal data because it has to be done by the
 * same thread that marshaled the interface in the first place. */
static void release_host_object(DWORD tid, WPARAM wp)
{
    HANDLE event = CreateEventA(NULL, FALSE, FALSE, NULL);
    PostThreadMessageA(tid, RELEASEMARSHALDATA, wp, (LPARAM)event);
    ok( !WaitForSingleObject(event, 10000), "wait timed out\n" );
    CloseHandle(event);
}

static void end_host_object(DWORD tid, HANDLE thread)
{
    BOOL ret = PostThreadMessageA(tid, WM_QUIT, 0, 0);
    ok(ret, "PostThreadMessage failed with error %d\n", GetLastError());
    /* be careful of races - don't return until hosting thread has terminated */
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);
}

/* tests failure case of interface not having a marshaler specified in the
 * registry */
static void test_no_marshaler(void)
{
    IStream *pStream;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IWineTest, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_NOINTERFACE, "CoMarshalInterface should have returned E_NOINTERFACE instead of 0x%08x\n", hr);

    IStream_Release(pStream);
}

/* tests normal marshal and then release without unmarshaling */
static void test_normal_marshal_and_release(void)
{
    HRESULT hr;
    IStream *pStream = NULL;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);
    IStream_Release(pStream);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);
}

/* tests success case of a same-thread marshal and unmarshal */
static void test_normal_marshal_and_unmarshal(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();
    ok_zero_external_conn();
    ok_last_release_closes(FALSE);

    IUnknown_Release(pProxy);

    ok_no_locks();
}

/* tests failure case of unmarshaling a freed object */
static void test_marshal_and_unmarshal_invalid(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IClassFactory *pProxy = NULL;
    DWORD tid;
    void * dummy;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    todo_wine { ok_ole_success(hr, CoUnmarshalInterface); }

    ok_no_locks();

    if (pProxy)
    {
        hr = IClassFactory_CreateInstance(pProxy, NULL, &IID_IUnknown, &dummy);
        ok(hr == RPC_E_DISCONNECTED, "Remote call should have returned RPC_E_DISCONNECTED, instead of 0x%08x\n", hr);

        IClassFactory_Release(pProxy);
    }

    IStream_Release(pStream);

    end_host_object(tid, thread);
}

static void test_same_apartment_unmarshal_failure(void)
{
    HRESULT hr;
    IStream *pStream;
    IUnknown *pProxy;
    static const LARGE_INTEGER llZero;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    hr = CoMarshalInterface(pStream, &IID_IUnknown, (IUnknown *)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    hr = IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, IStream_Seek);

    hr = CoUnmarshalInterface(pStream, &IID_IParseDisplayName, (void **)&pProxy);
    ok(hr == E_NOINTERFACE, "CoUnmarshalInterface should have returned E_NOINTERFACE instead of 0x%08x\n", hr);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(FALSE);

    IStream_Release(pStream);
}

/* tests success case of an interthread marshal */
static void test_interthread_marshal_and_unmarshal(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IUnknown_Release(pProxy);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    end_host_object(tid, thread);
}

/* the number of external references that Wine's proxy manager normally gives
 * out, so we can test the border case of running out of references */
#define NORMALEXTREFS 5

/* tests success case of an interthread marshal and then marshaling the proxy */
static void test_proxy_marshal_and_unmarshal(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;
    int i;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal the proxy */
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, pProxy, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();

    /* marshal 5 more times to exhaust the normal external references of 5 */
    for (i = 0; i < NORMALEXTREFS; i++)
    {
        hr = CoMarshalInterface(pStream, &IID_IClassFactory, pProxy, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
        ok_ole_success(hr, CoMarshalInterface);
    }

    ok_more_than_one_lock();

    /* release the original proxy to test that we successfully keep the
     * original object alive */
    IUnknown_Release(pProxy);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IUnknown_Release(pProxy2);

    /* unmarshal all of the proxies to check that the object stub still exists */
    for (i = 0; i < NORMALEXTREFS; i++)
    {
        hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
        ok_ole_success(hr, CoUnmarshalInterface);

        IUnknown_Release(pProxy2);
    }

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    IStream_Release(pStream);

    end_host_object(tid, thread);
}

/* tests success case of an interthread marshal and then marshaling the proxy
 * using an iid that hasn't previously been unmarshaled */
static void test_proxy_marshal_and_unmarshal2(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IUnknown, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal the proxy */
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, pProxy, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* unmarshal the second proxy to the object */
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    /* now the proxies should be as follows:
     *  pProxy -> &Test_ClassFactory
     *  pProxy2 -> &Test_ClassFactory
     * they should NOT be as follows:
     *  pProxy -> &Test_ClassFactory
     *  pProxy2 -> pProxy
     * the above can only really be tested by looking in +ole traces
     */

    ok_more_than_one_lock();

    IUnknown_Release(pProxy);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IUnknown_Release(pProxy2);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    end_host_object(tid, thread);
}

/* tests success case of an interthread marshal and then table-weak-marshaling the proxy */
static void test_proxy_marshal_and_unmarshal_weak(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal the proxy */
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, pProxy, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLEWEAK);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    /* release the original proxy to test that we successfully keep the
     * original object alive */
    IUnknown_Release(pProxy);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    todo_wine
    ok(hr == CO_E_OBJNOTREG, "CoUnmarshalInterface should return CO_E_OBJNOTREG instead of 0x%08x\n", hr);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    IStream_Release(pStream);

    end_host_object(tid, thread);
}

/* tests success case of an interthread marshal and then table-strong-marshaling the proxy */
static void test_proxy_marshal_and_unmarshal_strong(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    /* marshal the proxy */
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, pProxy, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLESTRONG);
    ok(hr == S_OK /* WinNT */ || hr == E_INVALIDARG /* Win9x */,
        "CoMarshalInterface should have return S_OK or E_INVALIDARG instead of 0x%08x\n", hr);
    if (FAILED(hr))
    {
        IUnknown_Release(pProxy);
        goto end;
    }

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    /* release the original proxy to test that we successfully keep the
     * original object alive */
    IUnknown_Release(pProxy);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IUnknown_Release(pProxy2);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

end:
    IStream_Release(pStream);

    end_host_object(tid, thread);

    ok_no_locks();
todo_wine {
    ok_zero_external_conn();
    ok_last_release_closes(FALSE);
}
}

/* tests that stubs are released when the containing apartment is destroyed */
static void test_marshal_stub_apartment_shutdown(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    end_host_object(tid, thread);

    ok_no_locks();
todo_wine {
    ok_zero_external_conn();
    ok_last_release_closes(FALSE);
}

    IUnknown_Release(pProxy);

    ok_no_locks();
}

/* tests that proxies are released when the containing apartment is destroyed */
static void test_marshal_proxy_apartment_shutdown(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    CoUninitialize();

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, thread);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

/* tests that proxies are released when the containing mta apartment is destroyed */
static void test_marshal_proxy_mta_apartment_shutdown(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    CoUninitialize();
    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    CoUninitialize();

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, thread);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

struct ncu_params
{
    LPSTREAM stream;
    HANDLE marshal_event;
    HANDLE unmarshal_event;
};

/* helper for test_no_couninitialize_server */
static DWORD CALLBACK no_couninitialize_server_proc(LPVOID p)
{
    struct ncu_params *ncu_params = p;
    HRESULT hr;

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoMarshalInterface(ncu_params->stream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    SetEvent(ncu_params->marshal_event);

    ok( !WaitForSingleObject(ncu_params->unmarshal_event, 10000), "wait timed out\n" );

    /* die without calling CoUninitialize */

    return 0;
}

/* tests apartment that an apartment with a stub is released without deadlock
 * if the owning thread exits */
static void test_no_couninitialize_server(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;
    struct ncu_params ncu_params;

    cLocks = 0;
    external_connections = 0;

    ncu_params.marshal_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    ncu_params.unmarshal_event = CreateEventA(NULL, TRUE, FALSE, NULL);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    ncu_params.stream = pStream;

    thread = CreateThread(NULL, 0, no_couninitialize_server_proc, &ncu_params, 0, &tid);

    ok( !WaitForSingleObject(ncu_params.marshal_event, 10000), "wait timed out\n" );
    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    SetEvent(ncu_params.unmarshal_event);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );

    ok_no_locks();
todo_wine {
    ok_zero_external_conn();
    ok_last_release_closes(FALSE);
}

    CloseHandle(thread);
    CloseHandle(ncu_params.marshal_event);
    CloseHandle(ncu_params.unmarshal_event);

    IUnknown_Release(pProxy);

    ok_no_locks();
}

/* STA -> STA call during DLL_THREAD_DETACH */
static DWORD CALLBACK no_couninitialize_client_proc(LPVOID p)
{
    struct ncu_params *ncu_params = p;
    HRESULT hr;
    IUnknown *pProxy = NULL;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoUnmarshalInterface(ncu_params->stream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(ncu_params->stream);

    ok_more_than_one_lock();

    /* die without calling CoUninitialize */

    return 0;
}

/* tests STA -> STA call during DLL_THREAD_DETACH doesn't deadlock */
static void test_no_couninitialize_client(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    DWORD tid;
    DWORD host_tid;
    HANDLE thread;
    HANDLE host_thread;
    struct ncu_params ncu_params;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    ncu_params.stream = pStream;

    /* NOTE: assumes start_host_object uses an STA to host the object, as MTAs
     * always deadlock when called from within DllMain */
    host_tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown *)&Test_ClassFactory, MSHLFLAGS_NORMAL, &host_thread);
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    thread = CreateThread(NULL, 0, no_couninitialize_client_proc, &ncu_params, 0, &tid);

    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    end_host_object(host_tid, host_thread);
}

static BOOL crash_thread_success;

static DWORD CALLBACK crash_couninitialize_proc(void *p)
{
    IStream *stream;
    HRESULT hr;

    cLocks = 0;

    CoInitialize(NULL);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    hr = CoMarshalInterface(stream, &IID_IUnknown, &TestCrash_Unknown, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    IStream_Seek(stream, ullZero, STREAM_SEEK_SET, NULL);

    hr = CoReleaseMarshalData(stream);
    ok_ole_success(hr, CoReleaseMarshalData);

    ok_no_locks();

    hr = CoMarshalInterface(stream, &IID_IUnknown, &TestCrash_Unknown, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();

    trace("CoUninitialize >>>\n");
    CoUninitialize();
    trace("CoUninitialize <<<\n");

    ok_no_locks();

    IStream_Release(stream);
    crash_thread_success = TRUE;
    return 0;
}

static void test_crash_couninitialize(void)
{
    HANDLE thread;
    DWORD tid;

    if(!GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateActCtxW")) {
        win_skip("Skipping crash tests on win2k.\n");
        return;
    }

    crash_thread_success = FALSE;
    thread = CreateThread(NULL, 0, crash_couninitialize_proc, NULL, 0, &tid);
    ok(!WaitForSingleObject(thread, 10000), "wait timed out\n");
    CloseHandle(thread);
    ok(crash_thread_success, "Crash thread failed\n");
}

/* tests success case of a same-thread table-weak marshal, unmarshal, unmarshal */
static void test_tableweak_marshal_and_unmarshal_twice(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy1 = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_TABLEWEAK, &thread);

    ok_more_than_one_lock();
    ok_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy1);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy1);
    ok_non_zero_external_conn();
    IUnknown_Release(pProxy2);
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    /* When IExternalConnection is present COM's lifetime management
     * behaviour is altered; the remaining weak ref prevents stub shutdown. */
    if (with_external_conn)
    {
        ok_more_than_one_lock();
        IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
        release_host_object(tid, 0);
    }

    /* Without IExternalConnection this line is shows the difference between weak and strong table marshaling
     * weak has cLocks == 0, strong has cLocks > 0. */
    ok_no_locks();

    IStream_Release(pStream);
    end_host_object(tid, thread);
}

/* tests releasing after unmarshaling one object */
static void test_tableweak_marshal_releasedata1(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy1 = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_TABLEWEAK, &thread);

    ok_more_than_one_lock();
    ok_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy1);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    /* release the remaining reference on the object by calling
     * CoReleaseMarshalData in the hosting thread */
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid, 0);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IUnknown_Release(pProxy1);

    if (pProxy2)
    {
        ok_non_zero_external_conn();
        IUnknown_Release(pProxy2);
    }

    /* this line is shows the difference between weak and strong table marshaling:
     *  weak has cLocks == 0
     *  strong has cLocks > 0 */
    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    end_host_object(tid, thread);
}

/* tests releasing after unmarshaling one object */
static void test_tableweak_marshal_releasedata2(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_TABLEWEAK, &thread);

    ok_more_than_one_lock();
    ok_zero_external_conn();

    /* release the remaining reference on the object by calling
     * CoReleaseMarshalData in the hosting thread */
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid, 0);

    ok_no_locks();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    todo_wine
    {
    ok(hr == CO_E_OBJNOTREG,
       "CoUnmarshalInterface should have failed with CO_E_OBJNOTREG, but returned 0x%08x instead\n",
       hr);
    }
    IStream_Release(pStream);

    ok_no_locks();
    ok_zero_external_conn();

    end_host_object(tid, thread);
}

struct duo_marshal_data
{
    MSHLFLAGS marshal_flags1, marshal_flags2;
    IStream *pStream1, *pStream2;
    HANDLE hReadyEvent;
    HANDLE hQuitEvent;
};

static DWORD CALLBACK duo_marshal_thread_proc(void *p)
{
    HRESULT hr;
    struct duo_marshal_data *data = p;
    HANDLE hQuitEvent = data->hQuitEvent;
    MSG msg;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoMarshalInterface(data->pStream1, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, data->marshal_flags1);
    ok_ole_success(hr, "CoMarshalInterface");

    hr = CoMarshalInterface(data->pStream2, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, data->marshal_flags2);
    ok_ole_success(hr, "CoMarshalInterface");

    /* force the message queue to be created before signaling parent thread */
    PeekMessageA(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(data->hReadyEvent);

    while (WAIT_OBJECT_0 + 1 == MsgWaitForMultipleObjects(1, &hQuitEvent, FALSE, 10000, QS_ALLINPUT))
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.hwnd == NULL && msg.message == RELEASEMARSHALDATA)
            {
                CoReleaseMarshalData(msg.wParam == 1 ? data->pStream1 : data->pStream2);
                SetEvent((HANDLE)msg.lParam);
            }
            else
                DispatchMessageA(&msg);
        }
    }
    CloseHandle(hQuitEvent);

    CoUninitialize();

    return 0;
}

/* tests interaction between table-weak and normal marshalling of an object */
static void test_tableweak_and_normal_marshal_and_unmarshal(void)
{
    HRESULT hr;
    IUnknown *pProxyWeak = NULL;
    IUnknown *pProxyNormal = NULL;
    DWORD tid;
    HANDLE thread;
    struct duo_marshal_data data;

    cLocks = 0;
    external_connections = 0;

    data.hReadyEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    data.hQuitEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    data.marshal_flags1 = MSHLFLAGS_TABLEWEAK;
    data.marshal_flags2 = MSHLFLAGS_NORMAL;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &data.pStream1);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CreateStreamOnHGlobal(NULL, TRUE, &data.pStream2);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    thread = CreateThread(NULL, 0, duo_marshal_thread_proc, &data, 0, &tid);
    ok( !WaitForSingleObject(data.hReadyEvent, 10000), "wait timed out\n" );
    CloseHandle(data.hReadyEvent);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    /* weak */
    IStream_Seek(data.pStream1, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(data.pStream1, &IID_IClassFactory, (void **)&pProxyWeak);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    /* normal */
    IStream_Seek(data.pStream2, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(data.pStream2, &IID_IClassFactory, (void **)&pProxyNormal);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IUnknown_Release(pProxyNormal);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IUnknown_Release(pProxyWeak);

    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    /* When IExternalConnection is present COM's lifetime management
     * behaviour is altered; the remaining weak ref prevents stub shutdown. */
    if (with_external_conn)
    {
        ok_more_than_one_lock();
        IStream_Seek(data.pStream1, ullZero, STREAM_SEEK_SET, NULL);
        release_host_object(tid, 1);
    }
    ok_no_locks();

    IStream_Release(data.pStream1);
    IStream_Release(data.pStream2);

    SetEvent(data.hQuitEvent);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);
}

static void test_tableweak_and_normal_marshal_and_releasedata(void)
{
    HRESULT hr;
    DWORD tid;
    HANDLE thread;
    struct duo_marshal_data data;

    cLocks = 0;
    external_connections = 0;

    data.hReadyEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    data.hQuitEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    data.marshal_flags1 = MSHLFLAGS_TABLEWEAK;
    data.marshal_flags2 = MSHLFLAGS_NORMAL;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &data.pStream1);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CreateStreamOnHGlobal(NULL, TRUE, &data.pStream2);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    thread = CreateThread(NULL, 0, duo_marshal_thread_proc, &data, 0, &tid);
    ok( !WaitForSingleObject(data.hReadyEvent, 10000), "wait timed out\n" );
    CloseHandle(data.hReadyEvent);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    /* release normal - which in the non-external conn case will free the object despite the weak ref. */
    IStream_Seek(data.pStream2, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid, 2);

    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    if (with_external_conn)
    {
        ok_more_than_one_lock();
        IStream_Seek(data.pStream1, ullZero, STREAM_SEEK_SET, NULL);
        release_host_object(tid, 1);
    }

    ok_no_locks();

    IStream_Release(data.pStream1);
    IStream_Release(data.pStream2);

    SetEvent(data.hQuitEvent);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);
}

static void test_two_tableweak_marshal_and_releasedata(void)
{
    HRESULT hr;
    DWORD tid;
    HANDLE thread;
    struct duo_marshal_data data;

    cLocks = 0;
    external_connections = 0;

    data.hReadyEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    data.hQuitEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    data.marshal_flags1 = MSHLFLAGS_TABLEWEAK;
    data.marshal_flags2 = MSHLFLAGS_TABLEWEAK;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &data.pStream1);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CreateStreamOnHGlobal(NULL, TRUE, &data.pStream2);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    thread = CreateThread(NULL, 0, duo_marshal_thread_proc, &data, 0, &tid);
    ok( !WaitForSingleObject(data.hReadyEvent, 10000), "wait timed out\n" );
    CloseHandle(data.hReadyEvent);

    ok_more_than_one_lock();
    ok_zero_external_conn();

    /* release one weak ref - the remaining weak ref will keep the obj alive */
    IStream_Seek(data.pStream1, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid, 1);

    ok_more_than_one_lock();

    IStream_Seek(data.pStream2, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid, 2);

    ok_no_locks();

    IStream_Release(data.pStream1);
    IStream_Release(data.pStream2);

    SetEvent(data.hQuitEvent);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);
}

/* tests success case of a same-thread table-strong marshal, unmarshal, unmarshal */
static void test_tablestrong_marshal_and_unmarshal_twice(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy1 = NULL;
    IUnknown *pProxy2 = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_TABLESTRONG, &thread);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy1);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();

    if (pProxy1) IUnknown_Release(pProxy1);
    if (pProxy2) IUnknown_Release(pProxy2);

    /* this line is shows the difference between weak and strong table marshaling:
     *  weak has cLocks == 0
     *  strong has cLocks > 0 */
    ok_more_than_one_lock();

    /* release the remaining reference on the object by calling
     * CoReleaseMarshalData in the hosting thread */
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    release_host_object(tid, 0);
    IStream_Release(pStream);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    end_host_object(tid, thread);
}

/* tests CoLockObjectExternal */
static void test_lock_object_external(void)
{
    HRESULT hr;
    IStream *pStream = NULL;

    cLocks = 0;
    external_connections = 0;

    /* test the stub manager creation aspect of CoLockObjectExternal when the
     * object hasn't been marshaled yet */
    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, TRUE, TRUE);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    CoDisconnectObject((IUnknown*)&Test_ClassFactory, 0);

    ok_no_locks();
    ok_non_zero_external_conn();
    external_connections = 0;

    /* test our empty stub manager being handled correctly in
     * CoMarshalInterface */
    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, TRUE, TRUE);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, TRUE, TRUE);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    ok_last_release_closes(TRUE);

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, FALSE, TRUE);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    ok_last_release_closes(TRUE);

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, FALSE, TRUE);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(TRUE);

    /* test CoLockObjectExternal releases reference to object with
     * fLastUnlockReleases as TRUE and there are only strong references on
     * the object */
    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, TRUE, FALSE);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, FALSE, FALSE);

    ok_no_locks();
    ok_zero_external_conn();
    ok_last_release_closes(FALSE);

    /* test CoLockObjectExternal doesn't release the last reference to an
     * object with fLastUnlockReleases as TRUE and there is a weak reference
     * on the object */
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLEWEAK);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();
    ok_zero_external_conn();

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, TRUE, FALSE);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, FALSE, FALSE);

    ok_more_than_one_lock();
    ok_zero_external_conn();
    ok_last_release_closes(FALSE);

    CoDisconnectObject((IUnknown*)&Test_ClassFactory, 0);

    ok_no_locks();

    IStream_Release(pStream);
}

/* tests disconnecting stubs */
static void test_disconnect_stub(void)
{
    HRESULT hr;
    IStream *pStream = NULL;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_non_zero_external_conn();

    CoLockObjectExternal((IUnknown*)&Test_ClassFactory, TRUE, TRUE);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);
    IStream_Release(pStream);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();

    CoDisconnectObject((IUnknown*)&Test_ClassFactory, 0);

    ok_no_locks();
    ok_non_zero_external_conn();

    hr = CoDisconnectObject(NULL, 0);
    ok( hr == E_INVALIDARG, "wrong status %x\n", hr );
}

/* tests failure case of a same-thread marshal and unmarshal twice */
static void test_normal_marshal_and_unmarshal_twice(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy1 = NULL;
    IUnknown *pProxy2 = NULL;

    cLocks = 0;
    external_connections = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();
    ok_non_zero_external_conn();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy1);
    ok_ole_success(hr, CoUnmarshalInterface);

    ok_more_than_one_lock();
    ok_zero_external_conn();
    ok_last_release_closes(FALSE);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy2);
    ok(hr == CO_E_OBJNOTCONNECTED,
        "CoUnmarshalInterface should have failed with error CO_E_OBJNOTCONNECTED for double unmarshal, instead of 0x%08x\n", hr);

    IStream_Release(pStream);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy1);

    ok_no_locks();
}

/* tests success case of marshaling and unmarshaling an HRESULT */
static void test_hresult_marshaling(void)
{
    HRESULT hr;
    HRESULT hr_marshaled = 0;
    IStream *pStream = NULL;
    static const HRESULT E_DEADBEEF = 0xdeadbeef;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    hr = CoMarshalHresult(pStream, E_DEADBEEF);
    ok_ole_success(hr, CoMarshalHresult);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = IStream_Read(pStream, &hr_marshaled, sizeof(HRESULT), NULL);
    ok_ole_success(hr, IStream_Read);

    ok(hr_marshaled == E_DEADBEEF, "Didn't marshal HRESULT as expected: got value 0x%08x instead\n", hr_marshaled);

    hr_marshaled = 0;
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalHresult(pStream, &hr_marshaled);
    ok_ole_success(hr, CoUnmarshalHresult);

    ok(hr_marshaled == E_DEADBEEF, "Didn't marshal HRESULT as expected: got value 0x%08x instead\n", hr_marshaled);

    IStream_Release(pStream);
}


/* helper for test_proxy_used_in_wrong_thread */
static DWORD CALLBACK bad_thread_proc(LPVOID p)
{
    IClassFactory * cf = p;
    HRESULT hr;
    IUnknown * proxy = NULL;

    hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (LPVOID*)&proxy);
    todo_wine
    ok(hr == CO_E_NOTINITIALIZED,
       "COM should have failed with CO_E_NOTINITIALIZED on using proxy without apartment, but instead returned 0x%08x\n",
       hr);

    hr = IClassFactory_QueryInterface(cf, &IID_IMultiQI, (LPVOID *)&proxy);
    /* Win9x returns S_OK, whilst NT returns RPC_E_WRONG_THREAD */
    trace("call to proxy's QueryInterface for local interface without apartment returned 0x%08x\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(proxy);

    hr = IClassFactory_QueryInterface(cf, &IID_IStream, (LPVOID *)&proxy);
    /* Win9x returns E_NOINTERFACE, whilst NT returns RPC_E_WRONG_THREAD */
    trace("call to proxy's QueryInterface without apartment returned 0x%08x\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(proxy);

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (LPVOID*)&proxy);
    if (proxy) IUnknown_Release(proxy);
    ok(hr == RPC_E_WRONG_THREAD,
        "COM should have failed with RPC_E_WRONG_THREAD on using proxy from wrong apartment, but instead returned 0x%08x\n",
        hr);

    hr = IClassFactory_QueryInterface(cf, &IID_IStream, (LPVOID *)&proxy);
    /* Win9x returns E_NOINTERFACE, whilst NT returns RPC_E_WRONG_THREAD */
    trace("call to proxy's QueryInterface from wrong apartment returned 0x%08x\n", hr);

    /* now be really bad and release the proxy from the wrong apartment */
    IClassFactory_Release(cf);

    CoUninitialize();

    return 0;
}

/* tests failure case of a using a proxy in the wrong apartment */
static void test_proxy_used_in_wrong_thread(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    DWORD tid, tid2;
    HANDLE thread;
    HANDLE host_thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &host_thread);

    ok_more_than_one_lock();
    
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    /* do a call that will fail, but result in IRemUnknown being used by the proxy */
    IUnknown_QueryInterface(pProxy, &IID_IStream, (LPVOID *)&pStream);

    /* create a thread that we can misbehave in */
    thread = CreateThread(NULL, 0, bad_thread_proc, pProxy, 0, &tid2);

    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);

    /* do release statement on Win9x that we should have done above */
    if (!GetProcAddress(GetModuleHandleA("ole32"), "CoRegisterSurrogateEx"))
        IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, host_thread);
}

static HRESULT WINAPI MessageFilter_QueryInterface(IMessageFilter *iface, REFIID riid, void ** ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = iface;
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
    static int callcount = 0;
    DWORD ret;
    trace("HandleInComingCall\n");
    switch (callcount)
    {
    case 0:
        ret = SERVERCALL_REJECTED;
        break;
    case 1:
        ret = SERVERCALL_RETRYLATER;
        break;
    default:
        ret = SERVERCALL_ISHANDLED;
        break;
    }
    callcount++;
    return ret;
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

static void test_message_filter(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IClassFactory *cf = NULL;
    DWORD tid;
    IUnknown *proxy = NULL;
    IMessageFilter *prev_filter = NULL;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object2(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &MessageFilter, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&cf);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (LPVOID*)&proxy);
    ok(hr == RPC_E_CALL_REJECTED, "Call should have returned RPC_E_CALL_REJECTED, but return 0x%08x instead\n", hr);
    if (proxy) IUnknown_Release(proxy);
    proxy = NULL;

    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok_ole_success(hr, CoRegisterMessageFilter);

    hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (LPVOID*)&proxy);
    ok_ole_success(hr, IClassFactory_CreateInstance);

    IUnknown_Release(proxy);

    IClassFactory_Release(cf);

    ok_no_locks();

    end_host_object(tid, thread);

    hr = CoRegisterMessageFilter(prev_filter, NULL);
    ok_ole_success(hr, CoRegisterMessageFilter);
}

/* test failure case of trying to unmarshal from bad stream */
static void test_bad_marshal_stream(void)
{
    HRESULT hr;
    IStream *pStream = NULL;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    hr = CoMarshalInterface(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, CoMarshalInterface);

    ok_more_than_one_lock();

    /* try to read beyond end of stream */
    hr = CoReleaseMarshalData(pStream);
    ok(hr == STG_E_READFAULT, "Should have failed with STG_E_READFAULT, but returned 0x%08x instead\n", hr);

    /* now release for real */
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, CoReleaseMarshalData);

    IStream_Release(pStream);
}

/* tests that proxies implement certain interfaces */
static void test_proxy_interfaces(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pOtherUnknown = NULL;
    DWORD tid;
    HANDLE thread;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();
	
    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    hr = IUnknown_QueryInterface(pProxy, &IID_IUnknown, (LPVOID*)&pOtherUnknown);
    ok_ole_success(hr, IUnknown_QueryInterface IID_IUnknown);
    if (hr == S_OK) IUnknown_Release(pOtherUnknown);

    hr = IUnknown_QueryInterface(pProxy, &IID_IClientSecurity, (LPVOID*)&pOtherUnknown);
    ok_ole_success(hr, IUnknown_QueryInterface IID_IClientSecurity);
    if (hr == S_OK) IUnknown_Release(pOtherUnknown);

    hr = IUnknown_QueryInterface(pProxy, &IID_IMultiQI, (LPVOID*)&pOtherUnknown);
    ok_ole_success(hr, IUnknown_QueryInterface IID_IMultiQI);
    if (hr == S_OK) IUnknown_Release(pOtherUnknown);

    hr = IUnknown_QueryInterface(pProxy, &IID_IMarshal, (LPVOID*)&pOtherUnknown);
    ok_ole_success(hr, IUnknown_QueryInterface IID_IMarshal);
    if (hr == S_OK) IUnknown_Release(pOtherUnknown);

    /* IMarshal2 is also supported on NT-based systems, but is pretty much
     * useless as it has no more methods over IMarshal that it inherits from. */

    IUnknown_Release(pProxy);

    ok_no_locks();

    end_host_object(tid, thread);
}

typedef struct
{
    IUnknown IUnknown_iface;
    ULONG refs;
} HeapUnknown;

static inline HeapUnknown *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, HeapUnknown, IUnknown_iface);
}

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
    HeapUnknown *This = impl_from_IUnknown(iface);
    return InterlockedIncrement((LONG*)&This->refs);
}

static ULONG WINAPI HeapUnknown_Release(IUnknown *iface)
{
    HeapUnknown *This = impl_from_IUnknown(iface);
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

static void test_proxybuffer(REFIID riid)
{
    HRESULT hr;
    IPSFactoryBuffer *psfb;
    IRpcProxyBuffer *proxy;
    LPVOID lpvtbl;
    ULONG refs;
    CLSID clsid;
    HeapUnknown *pUnkOuter = HeapAlloc(GetProcessHeap(), 0, sizeof(*pUnkOuter));

    pUnkOuter->IUnknown_iface.lpVtbl = &HeapUnknown_Vtbl;
    pUnkOuter->refs = 1;

    hr = CoGetPSClsid(riid, &clsid);
    ok_ole_success(hr, CoGetPSClsid);

    hr = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IPSFactoryBuffer, (LPVOID*)&psfb);
    ok_ole_success(hr, CoGetClassObject);

    hr = IPSFactoryBuffer_CreateProxy(psfb, &pUnkOuter->IUnknown_iface, riid, &proxy, &lpvtbl);
    ok_ole_success(hr, IPSFactoryBuffer_CreateProxy);
    ok(lpvtbl != NULL, "IPSFactoryBuffer_CreateProxy succeeded, but returned a NULL vtable!\n");

    /* release our reference to the outer unknown object - the PS factory
     * buffer will have AddRef's it in the CreateProxy call */
    refs = IUnknown_Release(&pUnkOuter->IUnknown_iface);
    ok(refs == 1, "Ref count of outer unknown should have been 1 instead of %d\n", refs);

    /* Not checking return, unreliable on native. Maybe it leaks references? */
    IPSFactoryBuffer_Release(psfb);

    refs = IUnknown_Release((IUnknown *)lpvtbl);
    ok(refs == 0, "Ref-count leak of %d on IRpcProxyBuffer\n", refs);

    refs = IRpcProxyBuffer_Release(proxy);
    ok(refs == 0, "Ref-count leak of %d on IRpcProxyBuffer\n", refs);
}

static void test_stubbuffer(REFIID riid)
{
    HRESULT hr;
    IPSFactoryBuffer *psfb;
    IRpcStubBuffer *stub;
    ULONG refs;
    CLSID clsid;

    cLocks = 0;

    hr = CoGetPSClsid(riid, &clsid);
    ok_ole_success(hr, CoGetPSClsid);

    hr = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IPSFactoryBuffer, (LPVOID*)&psfb);
    ok_ole_success(hr, CoGetClassObject);

    hr = IPSFactoryBuffer_CreateStub(psfb, riid, (IUnknown*)&Test_ClassFactory, &stub);
    ok_ole_success(hr, IPSFactoryBuffer_CreateStub);

    /* Not checking return, unreliable on native. Maybe it leaks references? */
    IPSFactoryBuffer_Release(psfb);

    ok_more_than_one_lock();

    IRpcStubBuffer_Disconnect(stub);

    ok_no_locks();

    refs = IRpcStubBuffer_Release(stub);
    ok(refs == 0, "Ref-count leak of %d on IRpcProxyBuffer\n", refs);
}

static HWND hwnd_app;

static HRESULT WINAPI TestRE_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    DWORD_PTR res;
    if (IsEqualIID(riid, &IID_IWineTest))
    {
        BOOL ret = SendMessageTimeoutA(hwnd_app, WM_NULL, 0, 0, SMTO_BLOCK, 5000, &res);
        ok(ret, "Timed out sending a message to originating window during RPC call\n");
    }
    *ppvObj = NULL;
    return S_FALSE;
}

static const IClassFactoryVtbl TestREClassFactory_Vtbl =
{
    Test_IClassFactory_QueryInterface,
    Test_IClassFactory_AddRef,
    Test_IClassFactory_Release,
    TestRE_IClassFactory_CreateInstance,
    Test_IClassFactory_LockServer
};

static IClassFactory TestRE_ClassFactory = { &TestREClassFactory_Vtbl };

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_USER:
    {
        HRESULT hr;
        IStream *pStream = NULL;
        IClassFactory *proxy = NULL;
        IUnknown *object;
        DWORD tid;
        HANDLE thread;

        cLocks = 0;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
        ok_ole_success(hr, CreateStreamOnHGlobal);
        tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&TestRE_ClassFactory, MSHLFLAGS_NORMAL, &thread);

        ok_more_than_one_lock();

        IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
        hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&proxy);
        ok_ole_success(hr, CoReleaseMarshalData);
        IStream_Release(pStream);

        ok_more_than_one_lock();

        /* note the use of the magic IID_IWineTest value to tell remote thread
         * to try to send a message back to us */
        hr = IClassFactory_CreateInstance(proxy, NULL, &IID_IWineTest, (void **)&object);
        ok(hr == S_FALSE, "expected S_FALSE, got %d\n", hr);

        IClassFactory_Release(proxy);

        ok_no_locks();

        end_host_object(tid, thread);

        PostMessageA(hwnd, WM_QUIT, 0, 0);

        return 0;
    }
    case WM_USER+1:
    {
        HRESULT hr;
        IStream *pStream = NULL;
        IClassFactory *proxy = NULL;
        IUnknown *object;
        DWORD tid;
        HANDLE thread;

        cLocks = 0;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
        ok_ole_success(hr, CreateStreamOnHGlobal);
        tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&TestRE_ClassFactory, MSHLFLAGS_NORMAL, &thread);

        ok_more_than_one_lock();

        IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
        hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&proxy);
        ok_ole_success(hr, CoReleaseMarshalData);
        IStream_Release(pStream);

        ok_more_than_one_lock();

        /* post quit message before a doing a COM call to show that a pending
        * WM_QUIT message doesn't stop the call from succeeding */
        PostMessageA(hwnd, WM_QUIT, 0, 0);
        hr = IClassFactory_CreateInstance(proxy, NULL, &IID_IUnknown, (void **)&object);
	ok(hr == S_FALSE, "IClassFactory_CreateInstance returned 0x%08x, expected S_FALSE\n", hr);

        IClassFactory_Release(proxy);

        ok_no_locks();

        end_host_object(tid, thread);

        return 0;
    }
    case WM_USER+2:
    {
        HRESULT hr;
        IStream *pStream = NULL;
        IClassFactory *proxy = NULL;
        IUnknown *object;
        DWORD tid;
        HANDLE thread;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
        ok_ole_success(hr, CreateStreamOnHGlobal);
        tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

        IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
        hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&proxy);
        ok_ole_success(hr, CoReleaseMarshalData);
        IStream_Release(pStream);

        /* shows that COM calls executed during the processing of sent
         * messages should fail */
        hr = IClassFactory_CreateInstance(proxy, NULL, &IID_IUnknown, (void **)&object);
        ok(hr == RPC_E_CANTCALLOUT_ININPUTSYNCCALL,
           "COM call during processing of sent message should return RPC_E_CANTCALLOUT_ININPUTSYNCCALL instead of 0x%08x\n", hr);

        IClassFactory_Release(proxy);

        end_host_object(tid, thread);

        PostQuitMessage(0);

        return 0;
    }
    default:
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
}

static void register_test_window(void)
{
    WNDCLASSA wndclass;

    memset(&wndclass, 0, sizeof(wndclass));
    wndclass.lpfnWndProc = window_proc;
    wndclass.lpszClassName = "WineCOMTest";
    RegisterClassA(&wndclass);
}

static void test_message_reentrancy(void)
{
    MSG msg;

    hwnd_app = CreateWindowA("WineCOMTest", NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, 0);
    ok(hwnd_app != NULL, "Window creation failed\n");

    /* start message re-entrancy test */
    PostMessageA(hwnd_app, WM_USER, 0, 0);

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    DestroyWindow(hwnd_app);
}

static HRESULT WINAPI TestMsg_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    *ppvObj = NULL;
    SendMessageA(hwnd_app, WM_USER+2, 0, 0);
    return S_OK;
}

static IClassFactoryVtbl TestMsgClassFactory_Vtbl =
{
    Test_IClassFactory_QueryInterface,
    Test_IClassFactory_AddRef,
    Test_IClassFactory_Release,
    TestMsg_IClassFactory_CreateInstance,
    Test_IClassFactory_LockServer
};

static IClassFactory TestMsg_ClassFactory = { &TestMsgClassFactory_Vtbl };

static void test_call_from_message(void)
{
    MSG msg;
    IStream *pStream;
    HRESULT hr;
    IClassFactory *proxy;
    DWORD tid;
    HANDLE thread;
    IUnknown *object;

    hwnd_app = CreateWindowA("WineCOMTest", NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, 0);
    ok(hwnd_app != NULL, "Window creation failed\n");

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&TestMsg_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&proxy);
    ok_ole_success(hr, CoReleaseMarshalData);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    /* start message re-entrancy test */
    hr = IClassFactory_CreateInstance(proxy, NULL, &IID_IUnknown, (void **)&object);
    ok_ole_success(hr, IClassFactory_CreateInstance);

    IClassFactory_Release(proxy);

    ok_no_locks();

    end_host_object(tid, thread);

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    DestroyWindow(hwnd_app);
}

static void test_WM_QUIT_handling(void)
{
    MSG msg;

    hwnd_app = CreateWindowA("WineCOMTest", NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, 0);
    ok(hwnd_app != NULL, "Window creation failed\n");

    /* start WM_QUIT handling test */
    PostMessageA(hwnd_app, WM_USER+1, 0, 0);

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
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

static void test_freethreadedmarshaldata(IStream *pStream, MSHCTX mshctx, void *ptr, DWORD mshlflags)
{
    HGLOBAL hglobal;
    DWORD size;
    char *marshal_data;
    HRESULT hr;

    hr = GetHGlobalFromStream(pStream, &hglobal);
    ok_ole_success(hr, GetHGlobalFromStream);

    size = GlobalSize(hglobal);

    marshal_data = GlobalLock(hglobal);

    if (mshctx == MSHCTX_INPROC)
    {
        DWORD expected_size = round_global_size(3*sizeof(DWORD) + sizeof(GUID));
        ok(size == expected_size ||
           broken(size == (2*sizeof(DWORD))) /* Win9x & NT4 */,
           "size should have been %d instead of %d\n", expected_size, size);

        ok(*(DWORD *)marshal_data == mshlflags, "expected 0x%x, but got 0x%x for mshctx\n", mshlflags, *(DWORD *)marshal_data);
        marshal_data += sizeof(DWORD);
        ok(*(void **)marshal_data == ptr, "expected %p, but got %p for mshctx\n", ptr, *(void **)marshal_data);
        marshal_data += sizeof(void *);
        if (sizeof(void*) == 4 && size >= 3*sizeof(DWORD))
        {
            ok(*(DWORD *)marshal_data == 0, "expected 0x0, but got 0x%x\n", *(DWORD *)marshal_data);
            marshal_data += sizeof(DWORD);
        }
        if (size >= 3*sizeof(DWORD) + sizeof(GUID))
        {
            trace("got guid data: %s\n", wine_dbgstr_guid((GUID *)marshal_data));
        }
    }
    else
    {
        ok(size > sizeof(DWORD), "size should have been > sizeof(DWORD), not %d\n", size);
        ok(*(DWORD *)marshal_data == 0x574f454d /* MEOW */,
            "marshal data should be filled by standard marshal and start with MEOW signature\n");
    }

    GlobalUnlock(hglobal);
}

static void test_freethreadedmarshaler(void)
{
    HRESULT hr;
    IUnknown *pFTUnknown;
    IMarshal *pFTMarshal;
    IStream *pStream;
    IUnknown *pProxy;
    static const LARGE_INTEGER llZero;

    cLocks = 0;
    hr = CoCreateFreeThreadedMarshaler(NULL, &pFTUnknown);
    ok_ole_success(hr, CoCreateFreeThreadedMarshaler);
    hr = IUnknown_QueryInterface(pFTUnknown, &IID_IMarshal, (void **)&pFTMarshal);
    ok_ole_success(hr, IUnknown_QueryInterface);
    IUnknown_Release(pFTUnknown);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);

    /* inproc normal marshaling */

    hr = IMarshal_MarshalInterface(pFTMarshal, pStream, &IID_IClassFactory,
        &Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, IMarshal_MarshalInterface);

    ok_more_than_one_lock();

    test_freethreadedmarshaldata(pStream, MSHCTX_INPROC, &Test_ClassFactory, MSHLFLAGS_NORMAL);

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_UnmarshalInterface(pFTMarshal, pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, IMarshal_UnmarshalInterface);

    IUnknown_Release(pProxy);

    ok_no_locks();

/* native doesn't allow us to unmarshal or release the stream data,
 * presumably because it wants us to call CoMarshalInterface instead */
    if (0)
    {
    /* local normal marshaling */

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_MarshalInterface(pFTMarshal, pStream, &IID_IClassFactory, &Test_ClassFactory, MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, IMarshal_MarshalInterface);

    ok_more_than_one_lock();

    test_freethreadedmarshaldata(pStream, MSHCTX_LOCAL, &Test_ClassFactory, MSHLFLAGS_NORMAL);

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_ReleaseMarshalData(pFTMarshal, pStream);
    ok_ole_success(hr, IMarshal_ReleaseMarshalData);

    ok_no_locks();
    }

    /* inproc table-strong marshaling */

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_MarshalInterface(pFTMarshal, pStream, &IID_IClassFactory,
        (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, (void *)0xdeadbeef,
        MSHLFLAGS_TABLESTRONG);
    ok_ole_success(hr, IMarshal_MarshalInterface);

    ok_more_than_one_lock();

    test_freethreadedmarshaldata(pStream, MSHCTX_INPROC, &Test_ClassFactory, MSHLFLAGS_TABLESTRONG);

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_UnmarshalInterface(pFTMarshal, pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, IMarshal_UnmarshalInterface);

    IUnknown_Release(pProxy);

    ok_more_than_one_lock();

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_ReleaseMarshalData(pFTMarshal, pStream);
    ok_ole_success(hr, IMarshal_ReleaseMarshalData);

    ok_no_locks();

    /* inproc table-weak marshaling */

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_MarshalInterface(pFTMarshal, pStream, &IID_IClassFactory,
        (IUnknown*)&Test_ClassFactory, MSHCTX_INPROC, (void *)0xdeadbeef,
        MSHLFLAGS_TABLEWEAK);
    ok_ole_success(hr, IMarshal_MarshalInterface);

    ok_no_locks();

    test_freethreadedmarshaldata(pStream, MSHCTX_INPROC, &Test_ClassFactory, MSHLFLAGS_TABLEWEAK);

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_UnmarshalInterface(pFTMarshal, pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, IMarshal_UnmarshalInterface);

    ok_more_than_one_lock();

    IUnknown_Release(pProxy);

    ok_no_locks();

    /* inproc normal marshaling (for extraordinary cases) */

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_MarshalInterface(pFTMarshal, pStream, &IID_IClassFactory,
        &Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, IMarshal_MarshalInterface);

    ok_more_than_one_lock();

    /* this call shows that DisconnectObject does nothing */
    hr = IMarshal_DisconnectObject(pFTMarshal, 0);
    ok_ole_success(hr, IMarshal_DisconnectObject);

    ok_more_than_one_lock();

    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_ReleaseMarshalData(pFTMarshal, pStream);
    ok_ole_success(hr, IMarshal_ReleaseMarshalData);

    ok_no_locks();

    /* doesn't enforce marshaling rules here and allows us to unmarshal the
     * interface, even though it was freed above */
    IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    hr = IMarshal_UnmarshalInterface(pFTMarshal, pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, IMarshal_UnmarshalInterface);

    ok_no_locks();

    IStream_Release(pStream);
    IMarshal_Release(pFTMarshal);
}

static HRESULT reg_unreg_wine_test_class(BOOL Register)
{
    HRESULT hr;
    char buffer[256];
    LPOLESTR pszClsid;
    HKEY hkey;
    DWORD dwDisposition;
    DWORD error;

    hr = StringFromCLSID(&CLSID_WineTest, &pszClsid);
    ok_ole_success(hr, "StringFromCLSID");
    strcpy(buffer, "CLSID\\");
    WideCharToMultiByte(CP_ACP, 0, pszClsid, -1, buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), NULL, NULL);
    CoTaskMemFree(pszClsid);
    strcat(buffer, "\\InprocHandler32");
    if (Register)
    {
        error = RegCreateKeyExA(HKEY_CLASSES_ROOT, buffer, 0, NULL, 0, KEY_SET_VALUE, NULL, &hkey, &dwDisposition);
        if (error == ERROR_ACCESS_DENIED)
        {
            skip("Not authorized to modify the Classes key\n");
            return E_FAIL;
        }
        ok(error == ERROR_SUCCESS, "RegCreateKeyEx failed with error %d\n", error);
        if (error != ERROR_SUCCESS) hr = E_FAIL;
        error = RegSetValueExA(hkey, NULL, 0, REG_SZ, (const unsigned char *)"\"ole32.dll\"", strlen("\"ole32.dll\"") + 1);
        ok(error == ERROR_SUCCESS, "RegSetValueEx failed with error %d\n", error);
        if (error != ERROR_SUCCESS) hr = E_FAIL;
        RegCloseKey(hkey);
    }
    else
    {
        RegDeleteKeyA(HKEY_CLASSES_ROOT, buffer);
        *strrchr(buffer, '\\') = '\0';
        RegDeleteKeyA(HKEY_CLASSES_ROOT, buffer);
    }
    return hr;
}

static void test_inproc_handler(void)
{
    HRESULT hr;
    IUnknown *pObject;
    IUnknown *pObject2;

    if (FAILED(reg_unreg_wine_test_class(TRUE)))
        return;

    hr = CoCreateInstance(&CLSID_WineTest, NULL, CLSCTX_INPROC_HANDLER, &IID_IUnknown, (void **)&pObject);
    ok_ole_success(hr, "CoCreateInstance");

    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(pObject, &IID_IWineTest, (void **)&pObject2);
        ok(hr == E_NOINTERFACE, "IUnknown_QueryInterface on handler for invalid interface returned 0x%08x instead of E_NOINTERFACE\n", hr);

        /* it's a handler as it supports IOleObject */
        hr = IUnknown_QueryInterface(pObject, &IID_IOleObject, (void **)&pObject2);
        ok_ole_success(hr, "IUnknown_QueryInterface(&IID_IOleObject)");
        IUnknown_Release(pObject2);

        IUnknown_Release(pObject);
    }

    reg_unreg_wine_test_class(FALSE);
}

static HRESULT WINAPI Test_SMI_QueryInterface(
    IStdMarshalInfo *iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IStdMarshalInfo))
    {
        *ppvObj = iface;
        IStdMarshalInfo_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI Test_SMI_AddRef(IStdMarshalInfo *iface)
{
    LockModule();
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_SMI_Release(IStdMarshalInfo *iface)
{
    UnlockModule();
    return 1; /* non-heap-based object */
}

static HRESULT WINAPI Test_SMI_GetClassForHandler(
    IStdMarshalInfo *iface,
    DWORD dwDestContext,
    void *pvDestContext,
    CLSID *pClsid)
{
    *pClsid = CLSID_WineTest;
    return S_OK;
}

static const IStdMarshalInfoVtbl Test_SMI_Vtbl =
{
    Test_SMI_QueryInterface,
    Test_SMI_AddRef,
    Test_SMI_Release,
    Test_SMI_GetClassForHandler
};

static IStdMarshalInfo Test_SMI = {&Test_SMI_Vtbl};

static void test_handler_marshaling(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IUnknown *pProxy = NULL;
    IUnknown *pObject;
    DWORD tid;
    HANDLE thread;
    static const LARGE_INTEGER ullZero;

    if (FAILED(reg_unreg_wine_test_class(TRUE)))
        return;
    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");
    tid = start_host_object(pStream, &IID_IUnknown, (IUnknown*)&Test_SMI, MSHLFLAGS_NORMAL, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    ok_ole_success(hr, "CoUnmarshalInterface");
    IStream_Release(pStream);

    if(hr == S_OK)
    {
        ok_more_than_one_lock();

        hr = IUnknown_QueryInterface(pProxy, &IID_IWineTest, (void **)&pObject);
        ok(hr == E_NOINTERFACE, "IUnknown_QueryInterface with unknown IID should have returned E_NOINTERFACE instead of 0x%08x\n", hr);

        /* it's a handler as it supports IOleObject */
        hr = IUnknown_QueryInterface(pProxy, &IID_IOleObject, (void **)&pObject);
        todo_wine
        ok_ole_success(hr, "IUnknown_QueryInterface(&IID_IOleObject)");
        if (SUCCEEDED(hr)) IUnknown_Release(pObject);

        IUnknown_Release(pProxy);

        ok_no_locks();
    }

    end_host_object(tid, thread);
    reg_unreg_wine_test_class(FALSE);

    /* FIXME: test IPersist interface has the same effect as IStdMarshalInfo */
}


static void test_client_security(void)
{
    HRESULT hr;
    IStream *pStream = NULL;
    IClassFactory *pProxy = NULL;
    IUnknown *pProxy2 = NULL;
    IUnknown *pUnknown1 = NULL;
    IUnknown *pUnknown2 = NULL;
    IClientSecurity *pCliSec = NULL;
    IMarshal *pMarshal;
    DWORD tid;
    HANDLE thread;
    static const LARGE_INTEGER ullZero;
    DWORD dwAuthnSvc;
    DWORD dwAuthzSvc;
    OLECHAR *pServerPrincName;
    DWORD dwAuthnLevel;
    DWORD dwImpLevel;
    void *pAuthInfo;
    DWORD dwCapabilities;
    void *pv;

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");
    tid = start_host_object(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &thread);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, "CoUnmarshalInterface");
    IStream_Release(pStream);

    hr = IClassFactory_QueryInterface(pProxy, &IID_IUnknown, (LPVOID*)&pUnknown1);
    ok_ole_success(hr, "IUnknown_QueryInterface IID_IUnknown");

    hr = IClassFactory_QueryInterface(pProxy, &IID_IRemUnknown, (LPVOID*)&pProxy2);
    ok_ole_success(hr, "IUnknown_QueryInterface IID_IStream");

    hr = IUnknown_QueryInterface(pProxy2, &IID_IUnknown, (LPVOID*)&pUnknown2);
    ok_ole_success(hr, "IUnknown_QueryInterface IID_IUnknown");

    ok(pUnknown1 == pUnknown2, "both proxy's IUnknowns should be the same - %p, %p\n", pUnknown1, pUnknown2);

    hr = IClassFactory_QueryInterface(pProxy, &IID_IMarshal, (LPVOID*)&pMarshal);
    ok_ole_success(hr, "IUnknown_QueryInterface IID_IMarshal");

    hr = IClassFactory_QueryInterface(pProxy, &IID_IClientSecurity, (LPVOID*)&pCliSec);
    ok_ole_success(hr, "IUnknown_QueryInterface IID_IClientSecurity");

    hr = IClientSecurity_QueryBlanket(pCliSec, (IUnknown *)pProxy, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    todo_wine ok_ole_success(hr, "IClientSecurity_QueryBlanket (all NULLs)");

    hr = IClientSecurity_QueryBlanket(pCliSec, (IUnknown *)pMarshal, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    todo_wine ok(hr == E_NOINTERFACE, "IClientSecurity_QueryBlanket with local interface should have returned E_NOINTERFACE instead of 0x%08x\n", hr);

    hr = IClientSecurity_QueryBlanket(pCliSec, (IUnknown *)pProxy, &dwAuthnSvc, &dwAuthzSvc, &pServerPrincName, &dwAuthnLevel, &dwImpLevel, &pAuthInfo, &dwCapabilities);
    todo_wine ok_ole_success(hr, "IClientSecurity_QueryBlanket");

    hr = IClientSecurity_SetBlanket(pCliSec, (IUnknown *)pProxy, dwAuthnSvc, dwAuthzSvc, pServerPrincName, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, pAuthInfo, dwCapabilities);
    todo_wine ok_ole_success(hr, "IClientSecurity_SetBlanket");

    hr = IClassFactory_CreateInstance(pProxy, NULL, &IID_IWineTest, &pv);
    ok(hr == E_NOINTERFACE, "COM call should have succeeded instead of returning 0x%08x\n", hr);

    hr = IClientSecurity_SetBlanket(pCliSec, (IUnknown *)pMarshal, dwAuthnSvc, dwAuthzSvc, pServerPrincName, dwAuthnLevel, dwImpLevel, pAuthInfo, dwCapabilities);
    todo_wine ok(hr == E_NOINTERFACE, "IClientSecurity_SetBlanket with local interface should have returned E_NOINTERFACE instead of 0x%08x\n", hr);

    hr = IClientSecurity_SetBlanket(pCliSec, (IUnknown *)pProxy, 0xdeadbeef, dwAuthzSvc, pServerPrincName, dwAuthnLevel, dwImpLevel, pAuthInfo, dwCapabilities);
    todo_wine ok(hr == E_INVALIDARG, "IClientSecurity_SetBlanke with invalid dwAuthnSvc should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    CoTaskMemFree(pServerPrincName);

    hr = IClientSecurity_QueryBlanket(pCliSec, pUnknown1, &dwAuthnSvc, &dwAuthzSvc, &pServerPrincName, &dwAuthnLevel, &dwImpLevel, &pAuthInfo, &dwCapabilities);
    todo_wine ok_ole_success(hr, "IClientSecurity_QueryBlanket(IUnknown)");

    CoTaskMemFree(pServerPrincName);

    IClassFactory_Release(pProxy);
    IUnknown_Release(pProxy2);
    IUnknown_Release(pUnknown1);
    IUnknown_Release(pUnknown2);
    IMarshal_Release(pMarshal);
    IClientSecurity_Release(pCliSec);

    end_host_object(tid, thread);
}

static HANDLE heventShutdown;

static void LockModuleOOP(void)
{
    InterlockedIncrement(&cLocks); /* for test purposes only */
    CoAddRefServerProcess();
}

static void UnlockModuleOOP(void)
{
    InterlockedDecrement(&cLocks); /* for test purposes only */
    if (!CoReleaseServerProcess())
        SetEvent(heventShutdown);
}

static HWND hwnd_app;

struct local_server
{
    IPersist IPersist_iface; /* a nice short interface */
};

static HRESULT WINAPI local_server_QueryInterface(IPersist *iface, REFIID iid, void **obj)
{
    *obj = NULL;

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IPersist))
        *obj = iface;

    if (*obj)
    {
        IPersist_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI local_server_AddRef(IPersist *iface)
{
    return 2;
}

static ULONG WINAPI local_server_Release(IPersist *iface)
{
    return 1;
}

static HRESULT WINAPI local_server_GetClassID(IPersist *iface, CLSID *clsid)
{
    HRESULT hr;

    *clsid = IID_IUnknown;

    /* Test calling CoDisconnectObject within a COM call */
    hr = CoDisconnectObject((IUnknown *)iface, 0);
    ok(hr == S_OK, "got %08x\n", hr);

    return S_OK;
}

static const IPersistVtbl local_server_persist_vtbl =
{
    local_server_QueryInterface,
    local_server_AddRef,
    local_server_Release,
    local_server_GetClassID
};

struct local_server local_server_class =
{
    {&local_server_persist_vtbl}
};

static HRESULT WINAPI TestOOP_IClassFactory_QueryInterface(
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

    return E_NOINTERFACE;
}

static ULONG WINAPI TestOOP_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap-based object */
}

static ULONG WINAPI TestOOP_IClassFactory_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap-based object */
}

static HRESULT WINAPI TestOOP_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    IPersist *persist = &local_server_class.IPersist_iface;
    HRESULT hr;
    IPersist_AddRef( persist );
    hr = IPersist_QueryInterface( persist, riid, ppvObj );
    IPersist_Release( persist );
    return hr;
}

static HRESULT WINAPI TestOOP_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    if (fLock)
        LockModuleOOP();
    else
        UnlockModuleOOP();
    return S_OK;
}

static const IClassFactoryVtbl TestClassFactoryOOP_Vtbl =
{
    TestOOP_IClassFactory_QueryInterface,
    TestOOP_IClassFactory_AddRef,
    TestOOP_IClassFactory_Release,
    TestOOP_IClassFactory_CreateInstance,
    TestOOP_IClassFactory_LockServer
};

static IClassFactory TestOOP_ClassFactory = { &TestClassFactoryOOP_Vtbl };

static void test_register_local_server(void)
{
    DWORD cookie;
    HRESULT hr;
    HANDLE ready_event;
    DWORD wait;
    HANDLE handles[2];

    heventShutdown = CreateEventA(NULL, TRUE, FALSE, NULL);
    ready_event = CreateEventA(NULL, FALSE, FALSE, "Wine COM Test Ready Event");
    handles[0] = CreateEventA(NULL, FALSE, FALSE, "Wine COM Test Quit Event");
    handles[1] = CreateEventA(NULL, FALSE, FALSE, "Wine COM Test Repeat Event");

again:
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&TestOOP_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, CoRegisterClassObject);

    SetEvent(ready_event);

    do
    {
        wait = MsgWaitForMultipleObjects(2, handles, FALSE, 30000, QS_ALLINPUT);
        if (wait == WAIT_OBJECT_0+2)
        {
            MSG msg;

            if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
            {
                trace("Message 0x%x\n", msg.message);
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        else if (wait == WAIT_OBJECT_0+1)
        {
            hr = CoRevokeClassObject(cookie);
            ok_ole_success(hr, CoRevokeClassObject);
            goto again;
        }
    }
    while (wait == WAIT_OBJECT_0+2);

    ok( wait == WAIT_OBJECT_0, "quit event wait timed out\n" );
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, CoRevokeClassObject);
    CloseHandle(handles[0]);
    CloseHandle(handles[1]);
}

static HANDLE create_target_process(const char *arg)
{
    char **argv;
    char cmdline[MAX_PATH];
    BOOL ret;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);

    pi.hThread = NULL;
    pi.hProcess = NULL;
    winetest_get_mainargs( &argv );
    sprintf(cmdline, "\"%s\" %s %s", argv[0], argv[1], arg);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed with error: %u\n", GetLastError());
    if (pi.hThread) CloseHandle(pi.hThread);
    return pi.hProcess;
}

/* tests functions commonly used by out of process COM servers */
static void test_local_server(void)
{
    DWORD cookie;
    HRESULT hr;
    IClassFactory * cf;
    IPersist *persist;
    DWORD ret;
    HANDLE process;
    HANDLE quit_event;
    HANDLE ready_event;
    HANDLE repeat_event;
    CLSID clsid;

    heventShutdown = CreateEventA(NULL, TRUE, FALSE, NULL);

    cLocks = 0;

    /* Start the object suspended */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&TestOOP_ClassFactory,
        CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED, &cookie);
    ok_ole_success(hr, CoRegisterClassObject);

    /* ... and CoGetClassObject does not find it and fails when it looks for the
     * class in the registry */
    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER,
        NULL, &IID_IClassFactory, (LPVOID*)&cf);
    ok(hr == REGDB_E_CLASSNOTREG || /* NT */
       hr == S_OK /* Win9x */,
        "CoGetClassObject should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);

    /* Resume the object suspended above ... */
    hr = CoResumeClassObjects();
    ok_ole_success(hr, CoResumeClassObjects);

    /* ... and now it should succeed */
    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER,
        NULL, &IID_IClassFactory, (LPVOID*)&cf);
    ok_ole_success(hr, CoGetClassObject);

    /* Now check the locking is working */
    /* NOTE: we are accessing the class directly, not through a proxy */

    ok_no_locks();

    hr = IClassFactory_LockServer(cf, TRUE);
    ok_ole_success(hr, IClassFactory_LockServer);

    ok_more_than_one_lock();
    
    IClassFactory_LockServer(cf, FALSE);
    ok_ole_success(hr, IClassFactory_LockServer);

    ok_no_locks();

    IClassFactory_Release(cf);

    /* wait for shutdown signal */
    ret = WaitForSingleObject(heventShutdown, 0);
    ok(ret != WAIT_TIMEOUT, "Server didn't shut down\n");

    /* try to connect again after SCM has suspended registered class objects */
    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, NULL,
        &IID_IClassFactory, (LPVOID*)&cf);
    ok(hr == CO_E_SERVER_STOPPING || /* NT */
       hr == REGDB_E_CLASSNOTREG || /* win2k */
       hr == S_OK /* Win9x */,
        "CoGetClassObject should have returned CO_E_SERVER_STOPPING or REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, CoRevokeClassObject);

    CloseHandle(heventShutdown);

    process = create_target_process("-Embedding");
    ok(process != NULL, "couldn't start local server process, error was %d\n", GetLastError());

    ready_event = CreateEventA(NULL, FALSE, FALSE, "Wine COM Test Ready Event");
    ok( !WaitForSingleObject(ready_event, 10000), "wait timed out\n" );

    hr = CoCreateInstance(&CLSID_WineOOPTest, NULL, CLSCTX_LOCAL_SERVER, &IID_IPersist, (void **)&persist);
    ok_ole_success(hr, CoCreateInstance);

    IPersist_Release(persist);

    hr = CoCreateInstance(&CLSID_WineOOPTest, NULL, CLSCTX_LOCAL_SERVER, &IID_IPersist, (void **)&persist);
    ok(hr == REGDB_E_CLASSNOTREG, "Second CoCreateInstance on REGCLS_SINGLEUSE object should have failed\n");

    /* Re-register the class and try calling CoDisconnectObject from within a call to that object */
    repeat_event = CreateEventA(NULL, FALSE, FALSE, "Wine COM Test Repeat Event");
    SetEvent(repeat_event);
    CloseHandle(repeat_event);

    ok( !WaitForSingleObject(ready_event, 10000), "wait timed out\n" );
    CloseHandle(ready_event);

    hr = CoCreateInstance(&CLSID_WineOOPTest, NULL, CLSCTX_LOCAL_SERVER, &IID_IPersist, (void **)&persist);
    ok_ole_success(hr, CoCreateInstance);

    /* GetClassID will call CoDisconnectObject */
    IPersist_GetClassID(persist, &clsid);
    IPersist_Release(persist);

    quit_event = CreateEventA(NULL, FALSE, FALSE, "Wine COM Test Quit Event");
    SetEvent(quit_event);

    winetest_wait_child_process( process );
    CloseHandle(quit_event);
    CloseHandle(process);
}

struct git_params
{
	DWORD cookie;
	IGlobalInterfaceTable *git;
};

static DWORD CALLBACK get_global_interface_proc(LPVOID pv)
{
	HRESULT hr;
	struct git_params *params = pv;
	IClassFactory *cf;

	hr = IGlobalInterfaceTable_GetInterfaceFromGlobal(params->git, params->cookie, &IID_IClassFactory, (void **)&cf);
	ok(hr == CO_E_NOTINITIALIZED ||
		broken(hr == E_UNEXPECTED) /* win2k */ ||
		broken(hr == S_OK) /* NT 4 */,
		"IGlobalInterfaceTable_GetInterfaceFromGlobal should have failed with error CO_E_NOTINITIALIZED or E_UNEXPECTED instead of 0x%08x\n",
		hr);
	if (hr == S_OK)
		IClassFactory_Release(cf);

	CoInitialize(NULL);

	hr = IGlobalInterfaceTable_GetInterfaceFromGlobal(params->git, params->cookie, &IID_IClassFactory, (void **)&cf);
	ok_ole_success(hr, IGlobalInterfaceTable_GetInterfaceFromGlobal);

	IClassFactory_Release(cf);

	CoUninitialize();

	return hr;
}

static void test_globalinterfacetable(void)
{
	HRESULT hr;
	IGlobalInterfaceTable *git;
	DWORD cookie;
	HANDLE thread;
	DWORD tid;
	struct git_params params;
	DWORD ret;
        IUnknown *object;
        IClassFactory *cf;
        ULONG ref;

        trace("test_globalinterfacetable\n");
	cLocks = 0;

	hr = pDllGetClassObject(&CLSID_StdGlobalInterfaceTable, &IID_IClassFactory, (void**)&cf);
	ok(hr == S_OK, "got 0x%08x\n", hr);

	hr = IClassFactory_QueryInterface(cf, &IID_IGlobalInterfaceTable, (void**)&object);
	ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

	IClassFactory_Release(cf);

	hr = CoCreateInstance(&CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, &IID_IGlobalInterfaceTable, (void **)&git);
	ok_ole_success(hr, CoCreateInstance);

	ref = IGlobalInterfaceTable_AddRef(git);
	ok(ref == 1, "ref=%d\n", ref);
	ref = IGlobalInterfaceTable_AddRef(git);
	ok(ref == 1, "ref=%d\n", ref);

	ref = IGlobalInterfaceTable_Release(git);
	ok(ref == 1, "ref=%d\n", ref);
	ref = IGlobalInterfaceTable_Release(git);
	ok(ref == 1, "ref=%d\n", ref);

	hr = IGlobalInterfaceTable_RegisterInterfaceInGlobal(git, (IUnknown *)&Test_ClassFactory, &IID_IClassFactory, &cookie);
	ok_ole_success(hr, IGlobalInterfaceTable_RegisterInterfaceInGlobal);

	ok_more_than_one_lock();

	params.cookie = cookie;
	params.git = git;
	/* note: params is on stack so we MUST wait for get_global_interface_proc
	 * to exit before we can return */
	thread = CreateThread(NULL, 0, get_global_interface_proc, &params, 0, &tid);

	ret = MsgWaitForMultipleObjects(1, &thread, FALSE, 10000, QS_ALLINPUT);
	while (ret == WAIT_OBJECT_0 + 1)
	{
		MSG msg;
		while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
			DispatchMessageA(&msg);
		ret = MsgWaitForMultipleObjects(1, &thread, FALSE, 10000, QS_ALLINPUT);
	}

	CloseHandle(thread);

	/* test getting interface from global with different iid */
	hr = IGlobalInterfaceTable_GetInterfaceFromGlobal(git, cookie, &IID_IUnknown, (void **)&object);
	ok_ole_success(hr, IGlobalInterfaceTable_GetInterfaceFromGlobal);
	IUnknown_Release(object);

	/* test getting interface from global with same iid */
	hr = IGlobalInterfaceTable_GetInterfaceFromGlobal(git, cookie, &IID_IClassFactory, (void **)&object);
	ok_ole_success(hr, IGlobalInterfaceTable_GetInterfaceFromGlobal);
	IUnknown_Release(object);

	hr = IGlobalInterfaceTable_RevokeInterfaceFromGlobal(git, cookie);
	ok_ole_success(hr, IGlobalInterfaceTable_RevokeInterfaceFromGlobal);

	ok_no_locks();

	IGlobalInterfaceTable_Release(git);
}

static void test_manualresetevent(void)
{
    ISynchronizeHandle *sync_handle;
    ISynchronize *psync1, *psync2;
    IUnknown *punk;
    HANDLE handle;
    LONG ref;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ManualResetEvent, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&punk);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(!!punk, "Got NULL.\n");
    IUnknown_Release(punk);

    hr = CoCreateInstance(&CLSID_ManualResetEvent, NULL, CLSCTX_INPROC_SERVER, &IID_ISynchronize, (void**)&psync1);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(!!psync1, "Got NULL.\n");

    hr = ISynchronize_Wait(psync1, 0, 5);
    ok(hr == RPC_S_CALLPENDING, "Got 0x%08x\n", hr);

    hr = ISynchronize_Reset(psync1);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = ISynchronize_Signal(psync1);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = ISynchronize_Wait(psync1, 0, 5);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = ISynchronize_Wait(psync1, 0, 5);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = ISynchronize_Reset(psync1);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = ISynchronize_Wait(psync1, 0, 5);
    ok(hr == RPC_S_CALLPENDING, "Got 0x%08x\n", hr);

    hr = CoCreateInstance(&CLSID_ManualResetEvent, NULL, CLSCTX_INPROC_SERVER, &IID_ISynchronize, (void**)&psync2);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    ok(!!psync2, "Got NULL.\n");
    ok(psync1 != psync2, "psync1 == psync2.\n");

    hr = ISynchronize_QueryInterface(psync2, &IID_ISynchronizeHandle, (void**)&sync_handle);
    ok(hr == S_OK, "QueryInterface(IID_ISynchronizeHandle) failed: %08x\n", hr);

    handle = NULL;
    hr = ISynchronizeHandle_GetHandle(sync_handle, &handle);
    ok(hr == S_OK, "GetHandle failed: %08x\n", hr);
    ok(handle != NULL && handle != INVALID_HANDLE_VALUE, "handle = %p\n", handle);

    ISynchronizeHandle_Release(sync_handle);

    hr = ISynchronize_Wait(psync2, 0, 5);
    ok(hr == RPC_S_CALLPENDING, "Got 0x%08x\n", hr);

    hr = ISynchronize_Reset(psync1);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = ISynchronize_Reset(psync2);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = ISynchronize_Signal(psync1);
    ok(hr == S_OK, "Got 0x%08x\n", hr);
    hr = ISynchronize_Wait(psync2, 0, 5);
    ok(hr == RPC_S_CALLPENDING, "Got 0x%08x\n", hr);

    ref = ISynchronize_AddRef(psync1);
    ok(ref == 2, "Got ref: %d\n", ref);
    ref = ISynchronize_AddRef(psync1);
    ok(ref == 3, "Got ref: %d\n", ref);
    ref = ISynchronize_Release(psync1);
    ok(ref == 2, "Got nonzero ref: %d\n", ref);
    ref = ISynchronize_Release(psync2);
    ok(!ref, "Got nonzero ref: %d\n", ref);
    ref = ISynchronize_Release(psync1);
    ok(ref == 1, "Got nonzero ref: %d\n", ref);
    ref = ISynchronize_Release(psync1);
    ok(!ref, "Got nonzero ref: %d\n", ref);
}

static const char *debugstr_iid(REFIID riid)
{
    static char name[256];
    HKEY hkeyInterface;
    WCHAR bufferW[39];
    char buffer[39];
    LONG name_size = sizeof(name);
    StringFromGUID2(riid, bufferW, sizeof(bufferW)/sizeof(bufferW[0]));
    WideCharToMultiByte(CP_ACP, 0, bufferW, sizeof(bufferW)/sizeof(bufferW[0]), buffer, sizeof(buffer), NULL, NULL);
    if (RegOpenKeyExA(HKEY_CLASSES_ROOT, "Interface", 0, KEY_QUERY_VALUE, &hkeyInterface) != ERROR_SUCCESS)
    {
        memcpy(name, buffer, sizeof(buffer));
        goto done;
    }
    if (RegQueryValueA(hkeyInterface, buffer, name, &name_size) != ERROR_SUCCESS)
    {
        memcpy(name, buffer, sizeof(buffer));
        goto done;
    }
    RegCloseKey(hkeyInterface);
done:
    return name;
}

static HRESULT WINAPI TestChannelHook_QueryInterface(IChannelHook *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IChannelHook))
    {
        *ppv = iface;
        IChannelHook_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TestChannelHook_AddRef(IChannelHook *iface)
{
    return 2;
}

static ULONG WINAPI TestChannelHook_Release(IChannelHook *iface)
{
    return 1;
}

static void WINAPI TestChannelHook_ClientGetSize(
    IChannelHook *iface,
    REFGUID uExtent,
    REFIID  riid,
    ULONG  *pDataSize )
{
    SChannelHookCallInfo *info = (SChannelHookCallInfo *)riid;
    trace("TestChannelHook_ClientGetBuffer\n");
    trace("\t%s method %d\n", debugstr_iid(riid), info->iMethod);
    trace("\tcid: %s\n", debugstr_iid(&info->uCausality));
    ok(info->cbSize == sizeof(*info), "info->cbSize was %d instead of %d\n", info->cbSize, (int)sizeof(*info));
    ok(info->dwServerPid == GetCurrentProcessId(), "info->dwServerPid was 0x%x instead of 0x%x\n", info->dwServerPid, GetCurrentProcessId());
    ok(!info->pObject, "info->pObject should be NULL\n");
    ok(IsEqualGUID(uExtent, &EXTENTID_WineTest), "uExtent wasn't correct\n");

    *pDataSize = 1;
}

static void WINAPI TestChannelHook_ClientFillBuffer(
    IChannelHook *iface,
    REFGUID uExtent,
    REFIID  riid,
    ULONG  *pDataSize,
    void   *pDataBuffer )
{
    SChannelHookCallInfo *info = (SChannelHookCallInfo *)riid;
    trace("TestChannelHook_ClientFillBuffer\n");
    ok(info->cbSize == sizeof(*info), "info->cbSize was %d instead of %d\n", info->cbSize, (int)sizeof(*info));
    ok(info->dwServerPid == GetCurrentProcessId(), "info->dwServerPid was 0x%x instead of 0x%x\n", info->dwServerPid, GetCurrentProcessId());
    ok(!info->pObject, "info->pObject should be NULL\n");
    ok(IsEqualGUID(uExtent, &EXTENTID_WineTest), "uExtent wasn't correct\n");

    *(unsigned char *)pDataBuffer = 0xcc;
    *pDataSize = 1;
}

static void WINAPI TestChannelHook_ClientNotify(
    IChannelHook *iface,
    REFGUID uExtent,
    REFIID  riid,
    ULONG   cbDataSize,
    void   *pDataBuffer,
    DWORD   lDataRep,
    HRESULT hrFault )
{
    SChannelHookCallInfo *info = (SChannelHookCallInfo *)riid;
    trace("TestChannelHook_ClientNotify hrFault = 0x%08x\n", hrFault);
    ok(info->cbSize == sizeof(*info), "info->cbSize was %d instead of %d\n", info->cbSize, (int)sizeof(*info));
    ok(info->dwServerPid == GetCurrentProcessId(), "info->dwServerPid was 0x%x instead of 0x%x\n", info->dwServerPid, GetCurrentProcessId());
    todo_wine {
    ok(info->pObject != NULL, "info->pObject shouldn't be NULL\n");
    }
    ok(IsEqualGUID(uExtent, &EXTENTID_WineTest), "uExtent wasn't correct\n");
}

static void WINAPI TestChannelHook_ServerNotify(
    IChannelHook *iface,
    REFGUID uExtent,
    REFIID  riid,
    ULONG   cbDataSize,
    void   *pDataBuffer,
    DWORD   lDataRep )
{
    SChannelHookCallInfo *info = (SChannelHookCallInfo *)riid;
    trace("TestChannelHook_ServerNotify\n");
    ok(info->cbSize == sizeof(*info), "info->cbSize was %d instead of %d\n", info->cbSize, (int)sizeof(*info));
    ok(info->dwServerPid == GetCurrentProcessId(), "info->dwServerPid was 0x%x instead of 0x%x\n", info->dwServerPid, GetCurrentProcessId());
    ok(info->pObject != NULL, "info->pObject shouldn't be NULL\n");
    ok(cbDataSize == 1, "cbDataSize should have been 1 instead of %d\n", cbDataSize);
    ok(*(unsigned char *)pDataBuffer == 0xcc, "pDataBuffer should have contained 0xcc instead of 0x%x\n", *(unsigned char *)pDataBuffer);
    ok(IsEqualGUID(uExtent, &EXTENTID_WineTest), "uExtent wasn't correct\n");
}

static void WINAPI TestChannelHook_ServerGetSize(
    IChannelHook *iface,
    REFGUID uExtent,
    REFIID  riid,
    HRESULT hrFault,
    ULONG  *pDataSize )
{
    SChannelHookCallInfo *info = (SChannelHookCallInfo *)riid;
    trace("TestChannelHook_ServerGetSize\n");
    trace("\t%s method %d\n", debugstr_iid(riid), info->iMethod);
    ok(info->cbSize == sizeof(*info), "info->cbSize was %d instead of %d\n", info->cbSize, (int)sizeof(*info));
    ok(info->dwServerPid == GetCurrentProcessId(), "info->dwServerPid was 0x%x instead of 0x%x\n", info->dwServerPid, GetCurrentProcessId());
    ok(info->pObject != NULL, "info->pObject shouldn't be NULL\n");
    ok(IsEqualGUID(uExtent, &EXTENTID_WineTest), "uExtent wasn't correct\n");
    if (hrFault != S_OK)
        trace("\thrFault = 0x%08x\n", hrFault);

    *pDataSize = 0;
}

static void WINAPI TestChannelHook_ServerFillBuffer(
    IChannelHook *iface,
    REFGUID uExtent,
    REFIID  riid,
    ULONG  *pDataSize,
    void   *pDataBuffer,
    HRESULT hrFault )
{
    trace("TestChannelHook_ServerFillBuffer\n");
    ok(0, "TestChannelHook_ServerFillBuffer shouldn't be called\n");
}

static const IChannelHookVtbl TestChannelHookVtbl =
{
    TestChannelHook_QueryInterface,
    TestChannelHook_AddRef,
    TestChannelHook_Release,
    TestChannelHook_ClientGetSize,
    TestChannelHook_ClientFillBuffer,
    TestChannelHook_ClientNotify,
    TestChannelHook_ServerNotify,
    TestChannelHook_ServerGetSize,
    TestChannelHook_ServerFillBuffer,
};

static IChannelHook TestChannelHook = { &TestChannelHookVtbl };

static void test_channel_hook(void)
{
    IStream *pStream = NULL;
    IClassFactory *cf = NULL;
    DWORD tid;
    IUnknown *proxy = NULL;
    HANDLE thread;
    HRESULT hr;

    hr = CoRegisterChannelHook(&EXTENTID_WineTest, &TestChannelHook);
    ok_ole_success(hr, CoRegisterChannelHook);

    hr = CoRegisterMessageFilter(&MessageFilter, NULL);
    ok_ole_success(hr, CoRegisterMessageFilter);

    cLocks = 0;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object2(pStream, &IID_IClassFactory, (IUnknown*)&Test_ClassFactory, MSHLFLAGS_NORMAL, &MessageFilter, &thread);

    ok_more_than_one_lock();

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&cf);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    ok_more_than_one_lock();

    hr = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (LPVOID*)&proxy);
    ok_ole_success(hr, IClassFactory_CreateInstance);
    IUnknown_Release(proxy);

    IClassFactory_Release(cf);

    ok_no_locks();

    end_host_object(tid, thread);

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok_ole_success(hr, CoRegisterMessageFilter);
}

START_TEST(marshal)
{
    HMODULE hOle32 = GetModuleHandleA("ole32");
    int argc;
    char **argv;

    if (!GetProcAddress(hOle32, "CoRegisterSurrogateEx")) {
        win_skip("skipping test on win9x\n");
        return;
    }

    pCoInitializeEx = (void*)GetProcAddress(hOle32, "CoInitializeEx");
    pDllGetClassObject = (void*)GetProcAddress(hOle32, "DllGetClassObject");

    argc = winetest_get_mainargs( &argv );
    if (argc > 2 && (!strcmp(argv[2], "-Embedding")))
    {
        pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        test_register_local_server();
        CoUninitialize();

        return;
    }

    register_test_window();

    test_cocreateinstance_proxy();

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* FIXME: test CoCreateInstanceEx */

    /* lifecycle management and marshaling tests */
    do
    {
        test_no_marshaler();
        test_normal_marshal_and_release();
        test_normal_marshal_and_unmarshal();
        test_marshal_and_unmarshal_invalid();
        test_same_apartment_unmarshal_failure();
        test_interthread_marshal_and_unmarshal();
        test_proxy_marshal_and_unmarshal();
        test_proxy_marshal_and_unmarshal2();
        test_proxy_marshal_and_unmarshal_weak();
        test_proxy_marshal_and_unmarshal_strong();
        test_marshal_stub_apartment_shutdown();
        test_marshal_proxy_apartment_shutdown();
        test_marshal_proxy_mta_apartment_shutdown();
        test_no_couninitialize_server();
        test_no_couninitialize_client();
        test_tableweak_marshal_and_unmarshal_twice();
        test_tableweak_marshal_releasedata1();
        test_tableweak_marshal_releasedata2();
        test_tableweak_and_normal_marshal_and_unmarshal();
        test_tableweak_and_normal_marshal_and_releasedata();
        test_two_tableweak_marshal_and_releasedata();
        test_tablestrong_marshal_and_unmarshal_twice();
        test_lock_object_external();
        test_disconnect_stub();
        test_normal_marshal_and_unmarshal_twice();

        with_external_conn = !with_external_conn;
    } while (with_external_conn);

    test_hresult_marshaling();
    test_proxy_used_in_wrong_thread();
    test_message_filter();
    test_bad_marshal_stream();
    test_proxy_interfaces();
    test_stubbuffer(&IID_IClassFactory);
    test_proxybuffer(&IID_IClassFactory);
    test_message_reentrancy();
    test_call_from_message();
    test_WM_QUIT_handling();
    test_freethreadedmarshaler();
    test_inproc_handler();
    test_handler_marshaling();
    test_client_security();

    test_local_server();

    test_globalinterfacetable();
    test_manualresetevent();
    test_crash_couninitialize();

    /* must be last test as channel hooks can't be unregistered */
    test_channel_hook();

    CoUninitialize();
}
