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

START_TEST(propstore)
{
    CoInitialize(NULL);

    test_inmemorystore();
    test_persistserialized();

    CoUninitialize();
}
