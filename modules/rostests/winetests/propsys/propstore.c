/*
 * Unit tests for IPropertyStore and related interfaces
 *
 * Copyright 2012 Vincent Povirk
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

#include <stdarg.h>
#include <stdio.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "propsys.h"
#include "wine/test.h"

#include "initguid.h"

DEFINE_GUID(PKEY_WineTest, 0x7b317433, 0xdfa3, 0x4c44, 0xad, 0x3e, 0x2f, 0x80, 0x4b, 0x90, 0xdb, 0xf4);
DEFINE_GUID(DUMMY_GUID1, 0x12345678, 0x1234,0x1234, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19);

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown *)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %d, got %d\n", ref, rc);
}

static void test_inmemorystore(void)
{
    IPropertyStoreCache *propcache;
    HRESULT hr;
    PROPERTYKEY pkey;
    PROPVARIANT propvar;
    DWORD count;
    PSC_STATE state;

    hr = CoCreateInstance(&CLSID_InMemoryPropertyStore, NULL, CLSCTX_INPROC_SERVER,
        &IID_IPropertyStoreCache, (void**)&propcache);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);

    if (FAILED(hr))
    {
        skip("CLSID_InMemoryPropertyStore not supported\n");
        return;
    }

    hr = IPropertyStoreCache_GetCount(propcache, NULL);
    ok(hr == E_POINTER, "GetCount failed, hr=%x\n", hr);

    hr = IPropertyStoreCache_GetCount(propcache, &count);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(count == 0, "GetCount returned %i, expected 0\n", count);

    hr = IPropertyStoreCache_Commit(propcache);
    ok(hr == S_OK, "Commit failed, hr=%x\n", hr);

    hr = IPropertyStoreCache_Commit(propcache);
    ok(hr == S_OK, "Commit failed, hr=%x\n", hr);

    hr = IPropertyStoreCache_GetAt(propcache, 0, &pkey);
    ok(hr == E_INVALIDARG, "GetAt failed, hr=%x\n", hr);

    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 4;

    memset(&propvar, 0, sizeof(propvar));
    propvar.vt = VT_I4;
    propvar.u.lVal = 12345;

    if (0)
    {
        /* Crashes on Windows 7 */
        hr = IPropertyStoreCache_SetValue(propcache, NULL, &propvar);
        ok(hr == E_POINTER, "SetValue failed, hr=%x\n", hr);

        hr = IPropertyStoreCache_SetValue(propcache, &pkey, NULL);
        ok(hr == E_POINTER, "SetValue failed, hr=%x\n", hr);
    }

    hr = IPropertyStoreCache_SetValue(propcache, &pkey, &propvar);
    ok(hr == S_OK, "SetValue failed, hr=%x\n", hr);

    hr = IPropertyStoreCache_GetCount(propcache, &count);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(count == 1, "GetCount returned %i, expected 0\n", count);

    memset(&pkey, 0, sizeof(pkey));

    hr = IPropertyStoreCache_GetAt(propcache, 0, &pkey);
    ok(hr == S_OK, "GetAt failed, hr=%x\n", hr);
    ok(IsEqualGUID(&pkey.fmtid, &PKEY_WineTest), "got wrong pkey\n");
    ok(pkey.pid == 4, "got pid of %i, expected 4\n", pkey.pid);

    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 4;

    memset(&propvar, 0, sizeof(propvar));

    if (0)
    {
        /* Crashes on Windows 7 */
        hr = IPropertyStoreCache_GetValue(propcache, NULL, &propvar);
        ok(hr == E_POINTER, "GetValue failed, hr=%x\n", hr);
    }

    hr = IPropertyStoreCache_GetValue(propcache, &pkey, NULL);
    ok(hr == E_POINTER, "GetValue failed, hr=%x\n", hr);

    hr = IPropertyStoreCache_GetValue(propcache, &pkey, &propvar);
    ok(hr == S_OK, "GetValue failed, hr=%x\n", hr);
    ok(propvar.vt == VT_I4, "expected VT_I4, got %d\n", propvar.vt);
    ok(propvar.u.lVal == 12345, "expected 12345, got %d\n", propvar.u.lVal);

    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 10;

    /* Get information for field that isn't set yet */
    propvar.vt = VT_I2;
    hr = IPropertyStoreCache_GetValue(propcache, &pkey, &propvar);
    ok(hr == S_OK, "GetValue failed, hr=%x\n", hr);
    ok(propvar.vt == VT_EMPTY, "expected VT_EMPTY, got %d\n", propvar.vt);

    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetState(propcache, &pkey, &state);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetState failed, hr=%x\n", hr);
    ok(state == PSC_NORMAL, "expected PSC_NORMAL, got %d\n", state);

    propvar.vt = VT_I2;
    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetValueAndState(propcache, &pkey, &propvar, &state);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetValueAndState failed, hr=%x\n", hr);
    ok(propvar.vt == VT_EMPTY, "expected VT_EMPTY, got %d\n", propvar.vt);
    ok(state == PSC_NORMAL, "expected PSC_NORMAL, got %d\n", state);

    /* Set state on an unset field */
    hr = IPropertyStoreCache_SetState(propcache, &pkey, PSC_NORMAL);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "SetState failed, hr=%x\n", hr);

    /* Manipulate state on already set field */
    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 4;

    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetState(propcache, &pkey, &state);
    ok(hr == S_OK, "GetState failed, hr=%x\n", hr);
    ok(state == PSC_NORMAL, "expected PSC_NORMAL, got %d\n", state);

    hr = IPropertyStoreCache_SetState(propcache, &pkey, 10);
    ok(hr == S_OK, "SetState failed, hr=%x\n", hr);

    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetState(propcache, &pkey, &state);
    ok(hr == S_OK, "GetState failed, hr=%x\n", hr);
    ok(state == 10, "expected 10, got %d\n", state);

    propvar.vt = VT_I4;
    propvar.u.lVal = 12346;
    hr = IPropertyStoreCache_SetValueAndState(propcache, &pkey, &propvar, 5);
    ok(hr == S_OK, "SetValueAndState failed, hr=%x\n", hr);

    memset(&propvar, 0, sizeof(propvar));
    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetValueAndState(propcache, &pkey, &propvar, &state);
    ok(hr == S_OK, "GetValueAndState failed, hr=%x\n", hr);
    ok(propvar.vt == VT_I4, "expected VT_I4, got %d\n", propvar.vt);
    ok(propvar.u.lVal == 12346, "expected 12346, got %d\n", propvar.vt);
    ok(state == 5, "expected 5, got %d\n", state);

    /* Set new field with state */
    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 8;

    propvar.vt = VT_I4;
    propvar.u.lVal = 12347;
    hr = IPropertyStoreCache_SetValueAndState(propcache, &pkey, &propvar, PSC_DIRTY);
    ok(hr == S_OK, "SetValueAndState failed, hr=%x\n", hr);

    memset(&propvar, 0, sizeof(propvar));
    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetValueAndState(propcache, &pkey, &propvar, &state);
    ok(hr == S_OK, "GetValueAndState failed, hr=%x\n", hr);
    ok(propvar.vt == VT_I4, "expected VT_I4, got %d\n", propvar.vt);
    ok(propvar.u.lVal == 12347, "expected 12347, got %d\n", propvar.vt);
    ok(state == PSC_DIRTY, "expected PSC_DIRTY, got %d\n", state);

    IPropertyStoreCache_Release(propcache);
}

static void test_persistserialized(void)
{
    IPropertyStore *propstore;
    IPersistSerializedPropStorage *serialized;
    HRESULT hr;
    SERIALIZEDPROPSTORAGE *result;
    DWORD result_size;

    hr = CoCreateInstance(&CLSID_InMemoryPropertyStore, NULL, CLSCTX_INPROC_SERVER,
        &IID_IPropertyStore, (void**)&propstore);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);

    hr = IPropertyStore_QueryInterface(propstore, &IID_IPersistSerializedPropStorage,
        (void**)&serialized);
    todo_wine ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    if (FAILED(hr))
    {
        IPropertyStore_Release(propstore);
        skip("IPersistSerializedPropStorage not supported\n");
        return;
    }

    hr = IPersistSerializedPropStorage_GetPropertyStorage(serialized, NULL, &result_size);
    ok(hr == E_POINTER, "GetPropertyStorage failed, hr=%x\n", hr);

    hr = IPersistSerializedPropStorage_GetPropertyStorage(serialized, &result, NULL);
    ok(hr == E_POINTER, "GetPropertyStorage failed, hr=%x\n", hr);

    hr = IPersistSerializedPropStorage_GetPropertyStorage(serialized, &result, &result_size);
    ok(hr == S_OK, "GetPropertyStorage failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        ok(result_size == 0, "expected 0 bytes, got %i\n", result_size);

        CoTaskMemFree(result);
    }

    hr = IPersistSerializedPropStorage_SetPropertyStorage(serialized, NULL, 4);
    ok(hr == E_POINTER, "SetPropertyStorage failed, hr=%x\n", hr);

    hr = IPersistSerializedPropStorage_SetPropertyStorage(serialized, NULL, 0);
    ok(hr == S_OK, "SetPropertyStorage failed, hr=%x\n", hr);

    hr = IPropertyStore_GetCount(propstore, &result_size);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(result_size == 0, "expecting 0, got %d\n", result_size);

    IPropertyStore_Release(propstore);
    IPersistSerializedPropStorage_Release(serialized);
}

static void test_PSCreateMemoryPropertyStore(void)
{
    IPropertyStore *propstore, *propstore1;
    IPersistSerializedPropStorage *serialized;
    IPropertyStoreCache *propstorecache;
    HRESULT hr;

    /* PSCreateMemoryPropertyStore(&IID_IPropertyStore, NULL); crashes */

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStore, (void **)&propstore);
    ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08x.\n", hr);
    ok(propstore != NULL, "got %p.\n", propstore);
    EXPECT_REF(propstore, 1);

    hr = PSCreateMemoryPropertyStore(&IID_IPersistSerializedPropStorage, (void **)&serialized);
    todo_wine ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08x.\n", hr);
    todo_wine ok(serialized != NULL, "got %p.\n", serialized);
    EXPECT_REF(propstore, 1);
    if(serialized)
    {
        EXPECT_REF(serialized, 1);
        IPersistSerializedPropStorage_Release(serialized);
    }

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStoreCache, (void **)&propstorecache);
    ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08x.\n", hr);
    ok(propstorecache != NULL, "got %p.\n", propstore);
    ok(propstorecache != (IPropertyStoreCache *)propstore, "pointer are equal: %p, %p.\n", propstorecache, propstore);
    EXPECT_REF(propstore, 1);
    EXPECT_REF(propstorecache, 1);

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStore, (void **)&propstore1);
    ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08x.\n", hr);
    ok(propstore1 != NULL, "got %p.\n", propstore);
    ok(propstore1 != propstore, "pointer are equal: %p, %p.\n", propstore1, propstore);
    EXPECT_REF(propstore, 1);
    EXPECT_REF(propstore1, 1);
    EXPECT_REF(propstorecache, 1);

    IPropertyStore_Release(propstore1);
    IPropertyStore_Release(propstore);
    IPropertyStoreCache_Release(propstorecache);
}

static void  test_propertystore(void)
{
    IPropertyStore *propstore;
    HRESULT hr;
    PROPVARIANT propvar, ret_propvar;
    PROPERTYKEY propkey;
    DWORD count = 0;

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStore, (void **)&propstore);
    ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08x.\n", hr);
    ok(propstore != NULL, "got %p.\n", propstore);

    hr = IPropertyStore_GetCount(propstore, &count);
    ok(hr == S_OK, "IPropertyStore_GetCount failed: 0x%08x.\n", hr);
    ok(!count, "got wrong property count: %d, expected 0.\n", count);

    PropVariantInit(&propvar);
    propvar.vt = VT_I4;
    U(propvar).lVal = 123;
    propkey.fmtid = DUMMY_GUID1;
    propkey.pid = PID_FIRST_USABLE;
    hr = IPropertyStore_SetValue(propstore, &propkey, &propvar);
    ok(hr == S_OK, "IPropertyStore_SetValue failed: 0x%08x.\n", hr);
    hr = IPropertyStore_Commit(propstore);
    ok(hr == S_OK, "IPropertyStore_Commit failed: 0x%08x.\n", hr);
    hr = IPropertyStore_GetCount(propstore, &count);
    ok(hr == S_OK, "IPropertyStore_GetCount failed: 0x%08x.\n", hr);
    ok(count == 1, "got wrong property count: %d, expected 1.\n", count);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = VT_I4;
    hr = IPropertyStore_GetValue(propstore, &propkey, &ret_propvar);
    ok(hr == S_OK, "IPropertyStore_GetValue failed: 0x%08x.\n", hr);
    ok(ret_propvar.vt == VT_I4, "got wrong property type: %x.\n", ret_propvar.vt);
    ok(U(ret_propvar).lVal == 123, "got wrong value: %d, expected 123.\n", U(ret_propvar).lVal);
    PropVariantClear(&propvar);
    PropVariantClear(&ret_propvar);

    PropVariantInit(&propvar);
    propkey.fmtid = DUMMY_GUID1;
    propkey.pid = PID_FIRST_USABLE;
    hr = IPropertyStore_SetValue(propstore, &propkey, &propvar);
    ok(hr == S_OK, "IPropertyStore_SetValue failed: 0x%08x.\n", hr);
    hr = IPropertyStore_Commit(propstore);
    ok(hr == S_OK, "IPropertyStore_Commit failed: 0x%08x.\n", hr);
    hr = IPropertyStore_GetCount(propstore, &count);
    ok(hr == S_OK, "IPropertyStore_GetCount failed: 0x%08x.\n", hr);
    ok(count == 1, "got wrong property count: %d, expected 1.\n", count);
    PropVariantInit(&ret_propvar);
    hr = IPropertyStore_GetValue(propstore, &propkey, &ret_propvar);
    ok(hr == S_OK, "IPropertyStore_GetValue failed: 0x%08x.\n", hr);
    ok(ret_propvar.vt == VT_EMPTY, "got wrong property type: %x.\n", ret_propvar.vt);
    ok(!U(ret_propvar).lVal, "got wrong value: %d, expected 0.\n", U(ret_propvar).lVal);
    PropVariantClear(&propvar);
    PropVariantClear(&ret_propvar);

    IPropertyStore_Release(propstore);
}

START_TEST(propstore)
{
    CoInitialize(NULL);

    test_inmemorystore();
    test_persistserialized();
    test_PSCreateMemoryPropertyStore();
    test_propertystore();

    CoUninitialize();
}
