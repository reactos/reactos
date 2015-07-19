/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
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

#define NONAMELESSUNION
#include "wine/test.h"

#define COBJMACROS

#ifdef STANDALONE
#include "initguid.h"
#endif

#include "unknwn.h"
#include "uuids.h"
#include "mmdeviceapi.h"
#include "devpkey.h"

static BOOL (WINAPI *pIsWow64Process)(HANDLE, BOOL *);

static const WCHAR software_renderW[] =
    { 'S','o','f','t','w','a','r','e','\\',
      'M','i','c','r','o','s','o','f','t','\\',
      'W','i','n','d','o','w','s','\\',
      'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
      'M','M','D','e','v','i','c','e','s','\\',
      'A','u','d','i','o','\\',
      'R','e','n','d','e','r',0 };
static const WCHAR propertiesW[] = {'P','r','o','p','e','r','t','i','e','s',0};


static void test_propertystore(IPropertyStore *store)
{
    HRESULT hr;
    PROPVARIANT pv;
    char temp[128];
    temp[sizeof(temp)-1] = 0;

    pv.vt = VT_EMPTY;
    hr = IPropertyStore_GetValue(store, &PKEY_AudioEndpoint_GUID, &pv);
    ok(hr == S_OK, "Failed with %08x\n", hr);
    ok(pv.vt == VT_LPWSTR, "Value should be %i, is %i\n", VT_LPWSTR, pv.vt);
    if (hr == S_OK && pv.vt == VT_LPWSTR)
    {
        WideCharToMultiByte(CP_ACP, 0, pv.u.pwszVal, -1, temp, sizeof(temp)-1, NULL, NULL);
        trace("guid: %s\n", temp);
        CoTaskMemFree(pv.u.pwszVal);
    }

    pv.vt = VT_EMPTY;
    hr = IPropertyStore_GetValue(store, (const PROPERTYKEY*)&DEVPKEY_DeviceInterface_FriendlyName, &pv);
    ok(hr == S_OK, "Failed with %08x\n", hr);
    ok(pv.vt == VT_EMPTY, "Key should not be found\n");

    pv.vt = VT_EMPTY;
    hr = IPropertyStore_GetValue(store, (const PROPERTYKEY*)&DEVPKEY_DeviceInterface_Enabled, &pv);
    ok(hr == S_OK, "Failed with %08x\n", hr);
    ok(pv.vt == VT_EMPTY, "Key should not be found\n");

    pv.vt = VT_EMPTY;
    hr = IPropertyStore_GetValue(store, (const PROPERTYKEY*)&DEVPKEY_DeviceInterface_ClassGuid, &pv);
    ok(hr == S_OK, "Failed with %08x\n", hr);
    ok(pv.vt == VT_EMPTY, "Key should not be found\n");
}

static void test_deviceinterface(IPropertyStore *store)
{
    HRESULT hr;
    PROPVARIANT pv;

    static const PROPERTYKEY deviceinterface_key = {
        {0x233164c8, 0x1b2c, 0x4c7d, {0xbc, 0x68, 0xb6, 0x71, 0x68, 0x7a, 0x25, 0x67}}, 1
    };

    pv.vt = VT_EMPTY;
    hr = IPropertyStore_GetValue(store, &deviceinterface_key, &pv);
    ok(hr == S_OK, "GetValue failed: %08x\n", hr);
    ok(pv.vt == VT_LPWSTR, "Got wrong variant type: 0x%x\n", pv.vt);
    trace("device interface: %s\n", wine_dbgstr_w(pv.u.pwszVal));
    CoTaskMemFree(pv.u.pwszVal);
}

static void test_getat(IPropertyStore *store)
{
    HRESULT hr;
    DWORD propcount;
    DWORD prop;
    PROPERTYKEY pkey;
    BOOL found_name = FALSE;
    BOOL found_desc = FALSE;
    char temp[128];
    temp[sizeof(temp)-1] = 0;

    hr = IPropertyStore_GetCount(store, &propcount);

    ok(hr == S_OK, "Failed with %08x\n", hr);
    ok(propcount > 0, "Propcount %d should be greather than zero\n", propcount);

    for (prop = 0; prop < propcount; prop++) {
	hr = IPropertyStore_GetAt(store, prop, &pkey);
	ok(hr == S_OK, "Failed with %08x\n", hr);
	if (IsEqualPropertyKey(pkey, DEVPKEY_Device_FriendlyName))
	    found_name = TRUE;
	if (IsEqualPropertyKey(pkey, DEVPKEY_Device_DeviceDesc))
	    found_desc = TRUE;
    }
    ok(found_name || broken(!found_name), "DEVPKEY_Device_FriendlyName not found\n");
    ok(found_desc == TRUE, "DEVPKEY_Device_DeviceDesc not found\n");
}

static void test_setvalue_on_wow64(IPropertyStore *store)
{
    PROPVARIANT pv;
    HRESULT hr;
    LONG ret;
    WCHAR *guidW;
    HKEY root, props, devkey;
    DWORD type, regval, size;

    static const PROPERTYKEY PKEY_Bogus = {
        {0x1da5d803, 0xd492, 0x4edd, {0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x00}}, 0x7f
    };
    static const WCHAR bogusW[] = {'{','1','D','A','5','D','8','0','3','-','D','4','9','2','-','4','E','D','D','-','8','C','2','3','-','E','0','C','0','F','F','E','E','7','F','0','0','}',',','1','2','7',0};

    PropVariantInit(&pv);

    pv.vt = VT_EMPTY;
    hr = IPropertyStore_GetValue(store, &PKEY_AudioEndpoint_GUID, &pv);
    ok(hr == S_OK, "Failed to get Endpoint GUID: %08x\n", hr);

    guidW = pv.u.pwszVal;

    pv.vt = VT_UI4;
    pv.u.ulVal = 0xAB;

    hr = IPropertyStore_SetValue(store, &PKEY_Bogus, &pv);
    ok(hr == S_OK || hr == E_ACCESSDENIED, "SetValue failed: %08x\n", hr);
    if (hr != S_OK)
    {
        win_skip("Missing permission to write to registry\n");
        return;
    }

    pv.u.ulVal = 0x00;

    hr = IPropertyStore_GetValue(store, &PKEY_Bogus, &pv);
    ok(hr == S_OK, "GetValue failed: %08x\n", hr);
    ok(pv.u.ulVal == 0xAB, "Got wrong value: 0x%x\n", pv.u.ulVal);

    /* should find the key in 64-bit view */
    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, software_renderW, 0, KEY_READ|KEY_WOW64_64KEY, &root);
    ok(ret == ERROR_SUCCESS, "Couldn't open mmdevices Render key: %u\n", ret);

    ret = RegOpenKeyExW(root, guidW, 0, KEY_READ|KEY_WOW64_64KEY, &devkey);
    ok(ret == ERROR_SUCCESS, "Couldn't open mmdevice guid key: %u\n", ret);

    ret = RegOpenKeyExW(devkey, propertiesW, 0, KEY_READ|KEY_WOW64_64KEY, &props);
    ok(ret == ERROR_SUCCESS, "Couldn't open mmdevice property key: %u\n", ret);

    /* Note: the registry key exists even without calling IPropStore::Commit */
    size = sizeof(regval);
    ret = RegGetValueW(props, NULL, bogusW, RRF_RT_DWORD, &type, &regval, &size);
    ok(ret == ERROR_SUCCESS, "Couldn't get bogus propertykey value: %u\n", ret);
    ok(type == REG_DWORD, "Got wrong value type: %u\n", type);
    ok(regval == 0xAB, "Got wrong value: 0x%x\n", regval);

    RegCloseKey(props);
    RegCloseKey(devkey);
    RegCloseKey(root);

    CoTaskMemFree(guidW);

    /* should NOT find the key in 32-bit view */
    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, software_renderW, 0, KEY_READ, &root);
    ok(ret == ERROR_FILE_NOT_FOUND, "Wrong error when opening mmdevices Render key: %u\n", ret);
}

START_TEST(propstore)
{
    HRESULT hr;
    IMMDeviceEnumerator *mme = NULL;
    IMMDevice *dev = NULL;
    IPropertyStore *store;
    BOOL is_wow64 = FALSE;
    HMODULE hk32 = GetModuleHandleA("kernel32.dll");

    pIsWow64Process = (void *)GetProcAddress(hk32, "IsWow64Process");

    if (pIsWow64Process)
        pIsWow64Process(GetCurrentProcess(), &is_wow64);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&mme);
    if (FAILED(hr))
    {
        skip("mmdevapi not available: 0x%08x\n", hr);
        goto cleanup;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mme, eRender, eMultimedia, &dev);
    ok(hr == S_OK || hr == E_NOTFOUND, "GetDefaultAudioEndpoint failed: 0x%08x\n", hr);
    if (hr != S_OK)
    {
        if (hr == E_NOTFOUND)
            skip("No sound card available\n");
        else
            skip("GetDefaultAudioEndpoint returns 0x%08x\n", hr);
        goto cleanup;
    }
    store = NULL;
    hr = IMMDevice_OpenPropertyStore(dev, 3, &store);
    ok(hr == E_INVALIDARG, "Wrong hr returned: %08x\n", hr);
    if (hr != S_OK)
        /* It seems on windows returning with E_INVALIDARG doesn't
         * set store to NULL, so just don't set store to non-null
         * before calling this function
         */
        ok(!store, "Store set to non-NULL on failure: %p/%08x\n", store, hr);
    else if (store)
        IPropertyStore_Release(store);
    hr = IMMDevice_OpenPropertyStore(dev, STGM_READ, NULL);
    ok(hr == E_POINTER, "Wrong hr returned: %08x\n", hr);

    store = NULL;
    hr = IMMDevice_OpenPropertyStore(dev, STGM_READWRITE, &store);
    if(hr == E_ACCESSDENIED)
        hr = IMMDevice_OpenPropertyStore(dev, STGM_READ, &store);
    ok(hr == S_OK, "Opening valid store returned %08x\n", hr);
    if (store)
    {
        test_propertystore(store);
        test_deviceinterface(store);
        test_getat(store);
        if (is_wow64)
            test_setvalue_on_wow64(store);
        IPropertyStore_Release(store);
    }
    IMMDevice_Release(dev);
cleanup:
    if (mme)
        IMMDeviceEnumerator_Release(mme);
    CoUninitialize();
}
