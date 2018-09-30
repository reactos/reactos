/*
 * Copyright (c) 2006 Vitaliy Margolen
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

#define DIRECTINPUT_VERSION 0x0700

#define COBJMACROS
#include <windows.h>

#include "wine/test.h"
#include "windef.h"
#include "dinput.h"

static const DIOBJECTDATAFORMAT obj_data_format[] = {
  { &GUID_YAxis, 16, DIDFT_OPTIONAL|DIDFT_AXIS  |DIDFT_MAKEINSTANCE(1), 0},
  { &GUID_Button,15, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(3), 0},
  { &GUID_Key,    0, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(16),0},
  { &GUID_Key,    1, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(17),0},
  { &GUID_Key,    2, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(18),0},
  { &GUID_Key,    3, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(19),0},
  { &GUID_Key,    4, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(20),0},
  { &GUID_Key,    5, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(21),0},
  { &GUID_Key,    6, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(22),0},
  { &GUID_Key,    7, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(23),0},
  { &GUID_Key,    8, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(24),0},
  { &GUID_Key,    9, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(25),0},
  { &GUID_Key,   10, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(26),0},
  { &GUID_Key,   11, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(27),0},
  { &GUID_Key,   12, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(28),0},
  { NULL,        13, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(5),0},

  { &GUID_Button,14, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(32),0}
};

static const DIDATAFORMAT data_format = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    32,
    ARRAY_SIZE(obj_data_format),
    (LPDIOBJECTDATAFORMAT)obj_data_format
};

static BOOL CALLBACK enum_callback(const DIDEVICEOBJECTINSTANCEA *oi, void *info)
{
    if (winetest_debug > 1)
        trace(" Type:%4x Ofs:%3d Flags:%08x Name:%s\n",
              oi->dwType, oi->dwOfs, oi->dwFlags, oi->tszName);
    (*(int*)info)++;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK enum_type_callback(const DIDEVICEOBJECTINSTANCEA *oi, void *info)
{
    DWORD expected = *(DWORD*)info;
    ok (expected & DIDFT_GETTYPE(oi->dwType), "EnumObjects() enumerated wrong type for obj %s, expected: %08x got: %08x\n", oi->tszName, expected, oi->dwType);
    return DIENUM_CONTINUE;
}

static void test_object_info(IDirectInputDeviceA *device, HWND hwnd)
{
    HRESULT hr;
    DIPROPDWORD dp;
    DIDEVICEOBJECTINSTANCEA obj_info;
    DWORD obj_types[] = {DIDFT_BUTTON, DIDFT_AXIS, DIDFT_POV};
    int type_index;
    int cnt1 = 0;
    DWORD cnt = 0;
    DIDEVICEOBJECTDATA buffer[5];

    hr = IDirectInputDevice_EnumObjects(device, enum_callback, &cnt, DIDFT_ALL);
    ok(SUCCEEDED(hr), "EnumObjects() failed: %08x\n", hr);

    hr = IDirectInputDevice_SetDataFormat(device, &data_format);
    ok(SUCCEEDED(hr), "SetDataFormat() failed: %08x\n", hr);

    hr = IDirectInputDevice_EnumObjects(device, enum_callback, &cnt1, DIDFT_ALL);
    ok(SUCCEEDED(hr), "EnumObjects() failed: %08x\n", hr);
    if (0) /* fails for joystick only */
    ok(cnt == cnt1, "Enum count changed from %d to %d\n", cnt, cnt1);

    /* Testing EnumObjects with different types of device objects */
    for (type_index=0; type_index < ARRAY_SIZE(obj_types); type_index++)
    {
        hr = IDirectInputDevice_EnumObjects(device, enum_type_callback, &obj_types[type_index], obj_types[type_index]);
        ok(SUCCEEDED(hr), "EnumObjects() failed: %08x\n", hr);
    }

    /* Test buffered mode */
    memset(&dp, 0, sizeof(dp));
    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwHow = DIPH_DEVICE;
    dp.diph.dwObj = 0;
    dp.dwData = 0;

    hr = IDirectInputDevice_SetProperty(device, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&dp.diph);
    ok(hr == DI_OK, "SetProperty() failed: %08x\n", hr);
    cnt = 5;
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK && cnt == 5, "GetDeviceData() failed: %08x cnt: %d\n", hr, cnt);
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(DIDEVICEOBJECTDATA_DX3), buffer, &cnt, 0);
    ok(hr == DIERR_NOTBUFFERED, "GetDeviceData() should have failed: %08x\n", hr);
    IDirectInputDevice_Acquire(device);
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(DIDEVICEOBJECTDATA_DX3), buffer, &cnt, 0);
    ok(hr == DIERR_NOTBUFFERED, "GetDeviceData() should have failed: %08x\n", hr);
    IDirectInputDevice_Unacquire(device);

    dp.dwData = 20;
    hr = IDirectInputDevice_SetProperty(device, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&dp.diph);
    ok(hr == DI_OK, "SetProperty() failed: %08x\n", hr);
    cnt = 5;
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed: %08x\n", hr);
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(DIDEVICEOBJECTDATA_DX3), buffer, &cnt, 0);
    ok(hr == DIERR_NOTACQUIRED, "GetDeviceData() should have failed: %08x\n", hr);
    hr = IDirectInputDevice_Acquire(device);
    ok(hr == DI_OK, "Acquire() failed: %08x\n", hr);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed: %08x\n", hr);
    hr = IDirectInputDevice_Unacquire(device);
    ok(hr == DI_OK, "Unacquire() failed: %08x\n", hr);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed: %08x\n", hr);

    /* No need to test devices without axis */
    obj_info.dwSize = sizeof(obj_info);
    hr = IDirectInputDevice_GetObjectInfo(device, &obj_info, 16, DIPH_BYOFFSET);
    if (SUCCEEDED(hr))
    {
        /* No device supports per axis relative/absolute mode */
        dp.diph.dwHow = DIPH_BYOFFSET;
        dp.diph.dwObj = 16;
        dp.dwData = DIPROPAXISMODE_ABS;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_UNSUPPORTED, "SetProperty() returned: %08x\n", hr);
        dp.diph.dwHow = DIPH_DEVICE;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_INVALIDPARAM, "SetProperty() returned: %08x\n", hr);
        dp.diph.dwObj = 0;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DI_OK, "SetProperty() failed: %08x\n", hr);

        /* Cannot change mode while acquired */
        hr = IDirectInputDevice_Acquire(device);
        ok(hr == DI_OK, "Acquire() failed: %08x\n", hr);

        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_ACQUIRED, "SetProperty() returned: %08x\n", hr);
        hr = IDirectInputDevice_Unacquire(device);
        ok(hr == DI_OK, "Unacquire() failed: %08x\n", hr);
    }
}

struct enum_data
{
    IDirectInputA *pDI;
    HWND hwnd;
};

static BOOL CALLBACK enum_devices(const DIDEVICEINSTANCEA *lpddi, void *pvRef)
{
    struct enum_data *data = pvRef;
    IDirectInputDeviceA *device, *obj = NULL;
    HRESULT hr;

    hr = IDirectInput_GetDeviceStatus(data->pDI, &lpddi->guidInstance);
    ok(hr == DI_OK, "IDirectInput_GetDeviceStatus() failed: %08x\n", hr);

    if (hr == DI_OK)
    {
        hr = IDirectInput_CreateDevice(data->pDI, &lpddi->guidInstance, &device, NULL);
        ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
        trace("Testing device %p \"%s\"\n", device, lpddi->tszInstanceName);

        hr = IUnknown_QueryInterface(device, &IID_IDirectInputDevice2A, (LPVOID*)&obj);
        ok(SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDevice7A) failed: %08x\n", hr);
        test_object_info(obj, data->hwnd);
        if (obj) IUnknown_Release(obj);
        obj = NULL;

        hr = IUnknown_QueryInterface(device, &IID_IDirectInputDevice2W, (LPVOID*)&obj);
        ok(SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDevice7W) failed: %08x\n", hr);
        test_object_info(obj, data->hwnd);
        if (obj) IUnknown_Release(obj);

        IUnknown_Release(device);
    }
    return DIENUM_CONTINUE;
}

static void device_tests(void)
{
    HRESULT hr;
    IDirectInputA *pDI = NULL, *obj = NULL;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    HWND hwnd;
    struct enum_data data;

    hr = CoCreateInstance(&CLSID_DirectInput, 0, 1, &IID_IDirectInput2A, (LPVOID*)&pDI);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_DEVICENOTREG)
    {
        skip("Tests require a newer dinput version\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInputCreateA() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInput_Initialize(pDI, hInstance, DIRECTINPUT_VERSION);
    ok(SUCCEEDED(hr), "Initialize() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    hr = IUnknown_QueryInterface(pDI, &IID_IDirectInput2W, (LPVOID*)&obj);
    ok(SUCCEEDED(hr), "QueryInterface(IDirectInput7W) failed: %08x\n", hr);

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
                         NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());
    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);

        data.pDI = pDI;
        data.hwnd = hwnd;
        hr = IDirectInput_EnumDevices(pDI, 0, enum_devices, &data, DIEDFL_ALLDEVICES);
        ok(SUCCEEDED(hr), "IDirectInput_EnumDevices() failed: %08x\n", hr);


        /* If GetDeviceStatus returns DI_OK the device must exist */
        hr = IDirectInput_GetDeviceStatus(pDI, &GUID_Joystick);
        if (hr == DI_OK)
        {
            IDirectInputDeviceA *device = NULL;

            hr = IDirectInput_CreateDevice(pDI, &GUID_Joystick, &device, NULL);
            ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
            if (device) IUnknown_Release(device);
        }

        DestroyWindow(hwnd);
    }
    if (obj) IUnknown_Release(obj);
    if (pDI) IUnknown_Release(pDI);
}

START_TEST(device)
{
    CoInitialize(NULL);

    device_tests();

    CoUninitialize();
}
