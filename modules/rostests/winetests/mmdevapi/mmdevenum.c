/*
 * Copyright 2009 Maarten Lankhorst
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

#include "wine/test.h"

#define COBJMACROS

#include "initguid.h"
#ifndef __REACTOS__
#include "endpointvolume.h"
#endif
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "audiopolicy.h"
#include "dshow.h"
#include "dsound.h"
#include "devpkey.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

/* Some of the QueryInterface tests are really just to check if I got the IIDs right :) */

/* IMMDeviceCollection appears to have no QueryInterface method and instead forwards to mme */
static void test_collection(IMMDeviceEnumerator *mme, IMMDeviceCollection *col)
{
    IMMDeviceCollection *col2;
    IMMDeviceEnumerator *mme2;
    IUnknown *unk;
    HRESULT hr;
    ULONG ref;
    UINT numdev;
    IMMDevice *dev;

    /* collection doesn't keep a ref on parent */
    IMMDeviceEnumerator_AddRef(mme);
    ref = IMMDeviceEnumerator_Release(mme);
    ok(ref == 2, "Reference count on parent is %u\n", ref);

    ref = IMMDeviceCollection_AddRef(col);
    IMMDeviceCollection_Release(col);
    ok(ref == 2, "Invalid reference count %u on collection\n", ref);

    hr = IMMDeviceCollection_QueryInterface(col, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "Null ppv returns %08x\n", hr);

    hr = IMMDeviceCollection_QueryInterface(col, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Cannot query for IID_IUnknown: 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok((IUnknown*)col == unk, "Pointers are not identical %p/%p/%p\n", col, unk, mme);
        IUnknown_Release(unk);
    }

    hr = IMMDeviceCollection_QueryInterface(col, &IID_IMMDeviceCollection, (void**)&col2);
    ok(hr == S_OK, "Cannot query for IID_IMMDeviceCollection: 0x%08x\n", hr);
    if (hr == S_OK)
        IMMDeviceCollection_Release(col2);

    hr = IMMDeviceCollection_QueryInterface(col, &IID_IMMDeviceEnumerator, (void**)&mme2);
    ok(hr == E_NOINTERFACE, "Query for IID_IMMDeviceEnumerator returned: 0x%08x\n", hr);
    if (hr == S_OK)
        IMMDeviceEnumerator_Release(mme2);

    hr = IMMDeviceCollection_GetCount(col, NULL);
    ok(hr == E_POINTER, "GetCount returned 0x%08x\n", hr);

    hr = IMMDeviceCollection_GetCount(col, &numdev);
    ok(hr == S_OK, "GetCount returned 0x%08x\n", hr);

    dev = (void*)(LONG_PTR)0x12345678;
    hr = IMMDeviceCollection_Item(col, numdev, &dev);
    ok(hr == E_INVALIDARG, "Asking for too high device returned 0x%08x\n", hr);
    ok(dev == NULL, "Returned non-null device\n");

    if (numdev)
    {
        hr = IMMDeviceCollection_Item(col, 0, NULL);
        ok(hr == E_POINTER, "Query with null pointer returned 0x%08x\n", hr);

        hr = IMMDeviceCollection_Item(col, 0, &dev);
        ok(hr == S_OK, "Valid Item returned 0x%08x\n", hr);
        ok(dev != NULL, "Device is null!\n");
        if (dev != NULL)
        {
            char temp[128];
            WCHAR *id = NULL;
            if (IMMDevice_GetId(dev, &id) == S_OK)
            {
                IMMDevice *dev2;

                temp[sizeof(temp)-1] = 0;
                WideCharToMultiByte(CP_ACP, 0, id, -1, temp, sizeof(temp)-1, NULL, NULL);
                trace("Device found: %s\n", temp);

                hr = IMMDeviceEnumerator_GetDevice(mme, id, &dev2);
                ok(hr == S_OK, "GetDevice failed: %08x\n", hr);

                IMMDevice_Release(dev2);

                CoTaskMemFree(id);
            }
        }
        if (dev)
            IMMDevice_Release(dev);
    }
    IMMDeviceCollection_Release(col);
}

static HRESULT WINAPI notif_QueryInterface(IMMNotificationClient *iface,
        const GUID *riid, void **obj)
{
    ok(0, "Unexpected QueryInterface call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI notif_AddRef(IMMNotificationClient *iface)
{
    ok(0, "Unexpected AddRef call\n");
    return 2;
}

static ULONG WINAPI notif_Release(IMMNotificationClient *iface)
{
    ok(0, "Unexpected Release call\n");
    return 1;
}

static HRESULT WINAPI notif_OnDeviceStateChanged(IMMNotificationClient *iface,
        const WCHAR *device_id, DWORD new_state)
{
    ok(0, "Unexpected OnDeviceStateChanged call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI notif_OnDeviceAdded(IMMNotificationClient *iface,
        const WCHAR *device_id)
{
    ok(0, "Unexpected OnDeviceAdded call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI notif_OnDeviceRemoved(IMMNotificationClient *iface,
        const WCHAR *device_id)
{
    ok(0, "Unexpected OnDeviceRemoved call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI notif_OnDefaultDeviceChanged(IMMNotificationClient *iface,
        EDataFlow flow, ERole role, const WCHAR *device_id)
{
    ok(0, "Unexpected OnDefaultDeviceChanged call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI notif_OnPropertyValueChanged(IMMNotificationClient *iface,
        const WCHAR *device_id, const PROPERTYKEY key)
{
    ok(0, "Unexpected OnPropertyValueChanged call\n");
    return E_NOTIMPL;
}

static IMMNotificationClientVtbl notif_vtbl = {
    notif_QueryInterface,
    notif_AddRef,
    notif_Release,
    notif_OnDeviceStateChanged,
    notif_OnDeviceAdded,
    notif_OnDeviceRemoved,
    notif_OnDefaultDeviceChanged,
    notif_OnPropertyValueChanged
};

static IMMNotificationClient notif = { &notif_vtbl };

/* Only do parameter tests here, the actual MMDevice testing should be a separate test */
START_TEST(mmdevenum)
{
    static const WCHAR not_a_deviceW[] = {'n','o','t','a','d','e','v','i','c','e',0};

    HRESULT hr;
    IUnknown *unk = NULL;
    IMMDeviceEnumerator *mme, *mme2;
    ULONG ref;
    IMMDeviceCollection *col;
    IMMDevice *dev;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08x\n", hr);
        return;
    }

    /* Odd behavior.. bug? */
    ref = IMMDeviceEnumerator_AddRef(mme);
    ok(ref == 3, "Invalid reference count after incrementing: %u\n", ref);
    IMMDeviceEnumerator_Release(mme);

    hr = IMMDeviceEnumerator_QueryInterface(mme, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "returned 0x%08x\n", hr);
    if (hr != S_OK) return;

    ok( (LONG_PTR)mme == (LONG_PTR)unk, "Pointers are unequal %p/%p\n", unk, mme);
    IUnknown_Release(unk);

    /* Proving that it is static.. */
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme2);
    ok(hr == S_OK, "CoCreateInstance failed: 0x%08x\n", hr);
    IMMDeviceEnumerator_Release(mme2);
    ok(mme == mme2, "Pointers are not equal!\n");

    hr = IMMDeviceEnumerator_QueryInterface(mme, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "Null pointer on QueryInterface returned %08x\n", hr);

    hr = IMMDeviceEnumerator_QueryInterface(mme, &GUID_NULL, (void**)&unk);
    ok(!unk, "Unk not reset to null after invalid QI\n");
    ok(hr == E_NOINTERFACE, "Invalid hr %08x returned on IID_NULL\n", hr);

    hr = IMMDeviceEnumerator_GetDevice(mme, not_a_deviceW, NULL);
    ok(hr == E_POINTER, "GetDevice gave wrong error: %08x\n", hr);

    hr = IMMDeviceEnumerator_GetDevice(mme, NULL, &dev);
    ok(hr == E_POINTER, "GetDevice gave wrong error: %08x\n", hr);

    hr = IMMDeviceEnumerator_GetDevice(mme, not_a_deviceW, &dev);
    ok(hr == E_INVALIDARG, "GetDevice gave wrong error: %08x\n", hr);

    col = (void*)(LONG_PTR)0x12345678;
    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, 0xffff, DEVICE_STATEMASK_ALL, &col);
    ok(hr == E_INVALIDARG, "Setting invalid data flow returned 0x%08x\n", hr);
    ok(col == NULL, "Collection pointer non-null on failure\n");

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL+1, &col);
    ok(hr == E_INVALIDARG, "Setting invalid mask returned 0x%08x\n", hr);

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL, NULL);
    ok(hr == E_POINTER, "Invalid pointer returned: 0x%08x\n", hr);

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(mme, eAll, DEVICE_STATEMASK_ALL, &col);
    ok(hr == S_OK, "Valid EnumAudioEndpoints returned 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok(!!col, "Returned null pointer\n");
        if (col)
            test_collection(mme, col);
    }

    hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(mme, NULL);
    ok(hr == E_POINTER, "RegisterEndpointNotificationCallback failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(mme, &notif);
    ok(hr == S_OK, "RegisterEndpointNotificationCallback failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(mme, &notif);
    ok(hr == S_OK, "RegisterEndpointNotificationCallback failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, NULL);
    ok(hr == E_POINTER, "UnregisterEndpointNotificationCallback failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, (IMMNotificationClient*)0xdeadbeef);
    ok(hr == E_NOTFOUND, "UnregisterEndpointNotificationCallback failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, &notif);
    ok(hr == S_OK, "UnregisterEndpointNotificationCallback failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, &notif);
    ok(hr == S_OK, "UnregisterEndpointNotificationCallback failed: %08x\n", hr);

    hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(mme, &notif);
    ok(hr == E_NOTFOUND, "UnregisterEndpointNotificationCallback failed: %08x\n", hr);

    IMMDeviceEnumerator_Release(mme);
}
