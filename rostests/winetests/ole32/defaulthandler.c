/*
 * Default Handler Tests
 *
 * Copyright 2008 Huw Davies
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

#define COBJMACROS
#define CONST_VTABLE

//#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <ole2.h>
//#include "objbase.h"

#include <wine/test.h>

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(CF_QueryInterface_ClassFactory);
DEFINE_EXPECT(CF_CreateInstance);

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static HRESULT create_storage(IStorage **stg)
{
    HRESULT hr;
    ILockBytes *lock_bytes;

    hr = CreateILockBytesOnHGlobal(NULL, TRUE, &lock_bytes);
    if(SUCCEEDED(hr))
    {
        hr = StgCreateDocfileOnILockBytes(lock_bytes,
                  STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, stg);
        ILockBytes_Release(lock_bytes);
    }
    return hr;
}

typedef struct
{
    DWORD version;
    DWORD flags;
    DWORD link_update_opt;
    DWORD res;
    DWORD moniker_size;
} ole_stream_header_t;

static void test_olestream(void)
{
    HRESULT hr;
    const CLSID non_existent_class = {0xa5f1772f, 0x3772, 0x490f, {0x9e, 0xc6, 0x77, 0x13, 0xe8, 0xb3, 0x4b, 0x5d}};
    IOleObject *ole_obj;
    IPersistStorage *persist;
    IStorage *stg;
    IStream *stm;
    static const WCHAR olestream[] = {1,'O','l','e',0};
    ULONG read;
    ole_stream_header_t header;

    hr = create_storage(&stg);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IStorage_OpenStream(stg, olestream, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &stm);
    ok(hr == STG_E_FILENOTFOUND, "got %08x\n", hr);

    hr = OleCreateDefaultHandler(&non_existent_class, 0, &IID_IOleObject, (void**)&ole_obj);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IOleObject_QueryInterface(ole_obj, &IID_IPersistStorage, (void**)&persist);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IPersistStorage_InitNew(persist, stg);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IStorage_OpenStream(stg, olestream, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &stm);
    ok(hr == S_OK, "got %08x\n", hr);
    hr = IStream_Read(stm, &header, sizeof(header), &read);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(read == sizeof(header), "read %d\n", read);
    ok(header.version == 0x02000001, "got version %08x\n", header.version);
    ok(header.flags == 0x0, "got flags %08x\n", header.flags);
    ok(header.link_update_opt == 0x0, "got link update option %08x\n", header.link_update_opt);
    ok(header.res == 0x0, "got reserved %08x\n", header.res);
    ok(header.moniker_size == 0x0, "got moniker size %08x\n", header.moniker_size);

    IStream_Release(stm);

    IPersistStorage_Release(persist);
    IOleObject_Release(ole_obj);

    IStorage_Release(stg);
}

static HRESULT WINAPI test_class_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = iface;
        return S_OK;
    }else if(IsEqualGUID(riid, &IID_IOleObject)) {
        ok(0, "unexpected query for IOleObject interface\n");
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_class_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI test_class_Release(IUnknown *iface)
{
    return 1;
}

static IUnknownVtbl test_class_vtbl = {
    test_class_QueryInterface,
    test_class_AddRef,
    test_class_Release,
};

static IUnknown test_class = { &test_class_vtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = iface;
        return S_OK;
    }else if(IsEqualGUID(riid, &IID_IMarshal)) {
        *ppv = NULL;
        return E_NOINTERFACE;
    }else if(IsEqualGUID(riid, &IID_IClassFactory)) {
        CHECK_EXPECT(CF_QueryInterface_ClassFactory);
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected interface: %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface,
        IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    CHECK_EXPECT(CF_CreateInstance);

    ok(pUnkOuter == NULL, "pUnkOuter != NULL\n");
    todo_wine ok(IsEqualGUID(riid, &IID_IUnknown), "riid = %s\n", debugstr_guid(riid));
    if(IsEqualGUID(riid, &IID_IOleObject)) {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    *ppv = &test_class;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ClassFactory = { &ClassFactoryVtbl };

static void test_default_handler_run(void)
{
    const CLSID test_server_clsid = {0x0f77e570,0x80c3,0x11e2,{0x9e,0x96,0x08,0x00,0x20,0x0c,0x9a,0x66}};

    IUnknown *unk;
    IRunnableObject *ro;
    DWORD class_reg;
    HRESULT hres;

    if(!GetProcAddress(GetModuleHandle("ole32"), "CoRegisterSurrogateEx")) {
        win_skip("skipping OleCreateDefaultHandler tests\n");
        return;
    }

    hres = CoRegisterClassObject(&test_server_clsid, (IUnknown*)&ClassFactory,
            CLSCTX_INPROC_SERVER, 0, &class_reg);
    ok(hres == S_OK, "CoRegisterClassObject failed: %x\n", hres);

    hres = OleCreateDefaultHandler(&test_server_clsid, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "OleCreateDefaultHandler failed: %x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IRunnableObject, (void**)&ro);
    ok(hres == S_OK, "QueryInterface(IRunnableObject) failed: %x\n", hres);
    IUnknown_Release(unk);

    hres = IRunnableObject_Run(ro, NULL);
    ok(hres == REGDB_E_CLASSNOTREG, "Run returned: %x, expected REGDB_E_CLASSNOTREG\n", hres);
    IRunnableObject_Release(ro);

    CoRevokeClassObject(class_reg);

    hres = CoRegisterClassObject(&test_server_clsid, (IUnknown*)&ClassFactory,
            CLSCTX_LOCAL_SERVER, 0, &class_reg);
    ok(hres == S_OK, "CoRegisterClassObject failed: %x\n", hres);

    hres = OleCreateDefaultHandler(&test_server_clsid, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "OleCreateDefaultHandler failed: %x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IRunnableObject, (void**)&ro);
    ok(hres == S_OK, "QueryInterface(IRunnableObject) failed: %x\n", hres);
    IUnknown_Release(unk);

    SET_EXPECT(CF_QueryInterface_ClassFactory);
    SET_EXPECT(CF_CreateInstance);
    hres = IRunnableObject_Run(ro, NULL);
    todo_wine ok(hres == S_OK, "Run failed: %x\n", hres);
    CHECK_CALLED(CF_QueryInterface_ClassFactory);
    CHECK_CALLED(CF_CreateInstance);
    IRunnableObject_Release(ro);

    CoRevokeClassObject(class_reg);
}

START_TEST(defaulthandler)
{
    OleInitialize(NULL);

    test_olestream();
    test_default_handler_run();

    OleUninitialize();
}
