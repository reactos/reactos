/*
 * Copyright (c) 2011 Andrew Nguyen
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

#define DIRECTINPUT_VERSION 0x0800

#define COBJMACROS
#include <initguid.h>
#include <windows.h>
#include <dinput.h>
#include <dinputd.h>

#include "wine/test.h"

HINSTANCE hInstance;

static BOOL CALLBACK dummy_callback(const DIDEVICEINSTANCEA *instance, void *context)
{
    ok(0, "Callback was invoked with parameters (%p, %p)\n", instance, context);
    return DIENUM_STOP;
}

static void test_preinitialization(void)
{
    static const struct
    {
        REFGUID rguid;
        BOOL pdev;
        HRESULT expected_hr;
    } create_device_tests[] =
    {
        {NULL, FALSE, E_POINTER},
        {NULL, TRUE, E_POINTER},
        {&GUID_Unknown, FALSE, E_POINTER},
        {&GUID_Unknown, TRUE, DIERR_NOTINITIALIZED},
        {&GUID_SysMouse, FALSE, E_POINTER},
        {&GUID_SysMouse, TRUE, DIERR_NOTINITIALIZED},
    };

    static const struct
    {
        DWORD dwDevType;
        LPDIENUMDEVICESCALLBACKA lpCallback;
        DWORD dwFlags;
        HRESULT expected_hr;
        int todo;
    } enum_devices_tests[] =
    {
        {0, NULL, 0, DIERR_INVALIDPARAM},
        {0, NULL, ~0u, DIERR_INVALIDPARAM},
        {0, dummy_callback, 0, DIERR_NOTINITIALIZED},
        {0, dummy_callback, ~0u, DIERR_INVALIDPARAM},
        {0xdeadbeef, NULL, 0, DIERR_INVALIDPARAM},
        {0xdeadbeef, NULL, ~0u, DIERR_INVALIDPARAM},
        {0xdeadbeef, dummy_callback, 0, DIERR_INVALIDPARAM},
        {0xdeadbeef, dummy_callback, ~0u, DIERR_INVALIDPARAM},
    };

    IDirectInput8A *pDI;
    HRESULT hr;
    int i;
    IDirectInputDevice8A *pDID;

    hr = CoCreateInstance(&CLSID_DirectInput8, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectInput8A, (void **)&pDI);
    if (FAILED(hr))
    {
        skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(create_device_tests); i++)
    {
        if (create_device_tests[i].pdev) pDID = (void *)0xdeadbeef;
        hr = IDirectInput8_CreateDevice(pDI, create_device_tests[i].rguid,
                                            create_device_tests[i].pdev ? &pDID : NULL,
                                            NULL);
        ok(hr == create_device_tests[i].expected_hr, "[%d] IDirectInput8_CreateDevice returned 0x%08x\n", i, hr);
        if (create_device_tests[i].pdev)
            ok(pDID == NULL, "[%d] Output interface pointer is %p\n", i, pDID);
    }

    for (i = 0; i < ARRAY_SIZE(enum_devices_tests); i++)
    {
        hr = IDirectInput8_EnumDevices(pDI, enum_devices_tests[i].dwDevType,
                                           enum_devices_tests[i].lpCallback,
                                           NULL,
                                           enum_devices_tests[i].dwFlags);
        todo_wine_if(enum_devices_tests[i].todo)
            ok(hr == enum_devices_tests[i].expected_hr, "[%d] IDirectInput8_EnumDevice returned 0x%08x\n", i, hr);
    }

    hr = IDirectInput8_GetDeviceStatus(pDI, NULL);
    ok(hr == E_POINTER, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_GetDeviceStatus(pDI, &GUID_Unknown);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_GetDeviceStatus(pDI, &GUID_SysMouse);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, NULL, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, (HWND)0xdeadbeef, 0);
    ok(hr == E_HANDLE, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, (HWND)0xdeadbeef, ~0u);
    ok(hr == E_HANDLE, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    IDirectInput8_Release(pDI);
}

static void test_DirectInput8Create(void)
{
    static const struct
    {
        BOOL hinst;
        DWORD dwVersion;
        REFIID riid;
        BOOL ppdi;
        HRESULT expected_hr;
    } invalid_param_list[] =
    {
        {FALSE, 0,                       &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, 0,                       &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, 0,                       &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, 0,                       &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {TRUE,  0,                       &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  0,                       &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  0,                       &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  0,                       &IID_IDirectInput8A, TRUE,  DIERR_NOTINITIALIZED},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, TRUE,  DIERR_BETADIRECTINPUTVERSION},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, TRUE,  DIERR_OLDDIRECTINPUTVERSION},
    };

    static REFIID no_interface_list[] = {&IID_IDirectInputA, &IID_IDirectInputW,
                                         &IID_IDirectInput2A, &IID_IDirectInput2W,
                                         &IID_IDirectInput7A, &IID_IDirectInput7W,
                                         &IID_IDirectInputDeviceA, &IID_IDirectInputDeviceW,
                                         &IID_IDirectInputDevice2A, &IID_IDirectInputDevice2W,
                                         &IID_IDirectInputDevice7A, &IID_IDirectInputDevice7W,
                                         &IID_IDirectInputDevice8A, &IID_IDirectInputDevice8W,
                                         &IID_IDirectInputEffect};

    static REFIID iid_list[] = {&IID_IUnknown, &IID_IDirectInput8A, &IID_IDirectInput8W};

    int i;
    IUnknown *pUnk;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(invalid_param_list); i++)
    {
        if (invalid_param_list[i].ppdi) pUnk = (void *)0xdeadbeef;
        hr = DirectInput8Create(invalid_param_list[i].hinst ? hInstance : NULL,
                                invalid_param_list[i].dwVersion,
                                invalid_param_list[i].riid,
                                invalid_param_list[i].ppdi ? (void **)&pUnk : NULL,
                                NULL);
        ok(hr == invalid_param_list[i].expected_hr, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        if (invalid_param_list[i].ppdi)
            ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    for (i = 0; i < ARRAY_SIZE(no_interface_list); i++)
    {
        pUnk = (void *)0xdeadbeef;
        hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, no_interface_list[i], (void **)&pUnk, NULL);
        ok(hr == DIERR_NOINTERFACE, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    for (i = 0; i < ARRAY_SIZE(iid_list); i++)
    {
        pUnk = NULL;
        hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, iid_list[i], (void **)&pUnk, NULL);
        ok(hr == DI_OK, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
        if (pUnk)
            IUnknown_Release(pUnk);
    }
}

static void test_QueryInterface(void)
{
    static REFIID iid_list[] = {&IID_IUnknown, &IID_IDirectInput8A, &IID_IDirectInput8W, &IID_IDirectInputJoyConfig8};

    static REFIID no_interface_list[] =
    {
        &IID_IDirectInputA,
        &IID_IDirectInputW,
        &IID_IDirectInput2A,
        &IID_IDirectInput2W,
        &IID_IDirectInput7A,
        &IID_IDirectInput7W,
        &IID_IDirectInputDeviceA,
        &IID_IDirectInputDeviceW,
        &IID_IDirectInputDevice2A,
        &IID_IDirectInputDevice2W,
        &IID_IDirectInputDevice7A,
        &IID_IDirectInputDevice7W,
        &IID_IDirectInputDevice8A,
        &IID_IDirectInputDevice8W,
        &IID_IDirectInputEffect,
    };

    IDirectInput8A *pDI;
    HRESULT hr;
    IUnknown *pUnk;
    int i;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_QueryInterface(pDI, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput8_QueryInterface returned 0x%08x\n", hr);

    pUnk = (void *)0xdeadbeef;
    hr = IDirectInput8_QueryInterface(pDI, NULL, (void **)&pUnk);
    ok(hr == E_POINTER, "IDirectInput8_QueryInterface returned 0x%08x\n", hr);
    ok(pUnk == (void *)0xdeadbeef, "Output interface pointer is %p\n", pUnk);

    hr = IDirectInput8_QueryInterface(pDI, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "IDirectInput8_QueryInterface returned 0x%08x\n", hr);

    for (i = 0; i < ARRAY_SIZE(iid_list); i++)
    {
        pUnk = NULL;
        hr = IDirectInput8_QueryInterface(pDI, iid_list[i], (void **)&pUnk);
        ok(hr == S_OK, "[%d] IDirectInput8_QueryInterface returned 0x%08x\n", i, hr);
        ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
        if (pUnk)
        {
            int j;
            for (j = 0; j < ARRAY_SIZE(iid_list); j++)
            {
                IUnknown *pUnk1 = NULL;
                hr = IDirectInput8_QueryInterface(pUnk, iid_list[j], (void **)&pUnk1);
                ok(hr == S_OK, "[%d] IDirectInput8_QueryInterface(pUnk) returned 0x%08x\n", j, hr);
                ok(pUnk1 != NULL, "[%d] Output interface pointer is NULL\n", i);
                if (pUnk1) IUnknown_Release(pUnk1);
            }
            IUnknown_Release(pUnk);
        }
    }

    for (i = 0; i < ARRAY_SIZE(no_interface_list); i++)
    {
        pUnk = (void *)0xdeadbeef;
        hr = IDirectInput8_QueryInterface(pDI, no_interface_list[i], (void **)&pUnk);

        ok(hr == E_NOINTERFACE, "[%d] IDirectInput8_QueryInterface returned 0x%08x\n", i, hr);
        ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    IDirectInput8_Release(pDI);
}

static void test_CreateDevice(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;
    IDirectInputDevice8A *pDID;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_CreateDevice(pDI, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);

    pDID = (void *)0xdeadbeef;
    hr = IDirectInput8_CreateDevice(pDI, NULL, &pDID, NULL);
    ok(hr == E_POINTER, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);
    ok(pDID == NULL, "Output interface pointer is %p\n", pDID);

    hr = IDirectInput8_CreateDevice(pDI, &GUID_Unknown, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);

    pDID = (void *)0xdeadbeef;
    hr = IDirectInput8_CreateDevice(pDI, &GUID_Unknown, &pDID, NULL);
    ok(hr == DIERR_DEVICENOTREG, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);
    ok(pDID == NULL, "Output interface pointer is %p\n", pDID);

    hr = IDirectInput8_CreateDevice(pDI, &GUID_SysMouse, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);

    hr = IDirectInput8_CreateDevice(pDI, &GUID_SysMouse, &pDID, NULL);
    ok(hr == DI_OK, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);

    IDirectInputDevice_Release(pDID);
    IDirectInput8_Release(pDI);
}

struct enum_devices_test
{
    unsigned int device_count;
    BOOL return_value;
};

static BOOL CALLBACK enum_devices_callback(const DIDEVICEINSTANCEA *instance, void *context)
{
    struct enum_devices_test *enum_test = context;

    trace("---- Device Information ----\n"
          "Product Name  : %s\n"
          "Instance Name : %s\n"
          "devType       : 0x%08x\n"
          "GUID Product  : %s\n"
          "GUID Instance : %s\n"
          "HID Page      : 0x%04x\n"
          "HID Usage     : 0x%04x\n",
          instance->tszProductName,
          instance->tszInstanceName,
          instance->dwDevType,
          wine_dbgstr_guid(&instance->guidProduct),
          wine_dbgstr_guid(&instance->guidInstance),
          instance->wUsagePage,
          instance->wUsage);

    if ((instance->dwDevType & 0xff) == DI8DEVTYPE_KEYBOARD ||
           (instance->dwDevType & 0xff) == DI8DEVTYPE_MOUSE) {
        const char *device = ((instance->dwDevType & 0xff) ==
                                   DI8DEVTYPE_KEYBOARD) ? "Keyboard" : "Mouse";
        ok(IsEqualGUID(&instance->guidInstance, &instance->guidProduct),
           "%s guidInstance (%s) does not match guidProduct (%s)\n",
           device, wine_dbgstr_guid(&instance->guidInstance),
           wine_dbgstr_guid(&instance->guidProduct));
    }

    enum_test->device_count++;
    return enum_test->return_value;
}

static void test_EnumDevices(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;
    struct enum_devices_test enum_test, enum_test_return;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_EnumDevices(pDI, 0, NULL, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput8_EnumDevices(pDI, 0, NULL, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    /* Test crashes on Wine. */
    if (0)
    {
        hr = IDirectInput8_EnumDevices(pDI, 0, enum_devices_callback, NULL, ~0u);
        ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);
    }

    hr = IDirectInput8_EnumDevices(pDI, 0xdeadbeef, NULL, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput8_EnumDevices(pDI, 0xdeadbeef, NULL, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput8_EnumDevices(pDI, 0xdeadbeef, enum_devices_callback, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput8_EnumDevices(pDI, 0xdeadbeef, enum_devices_callback, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    enum_test.device_count = 0;
    enum_test.return_value = DIENUM_CONTINUE;
    hr = IDirectInput8_EnumDevices(pDI, 0, enum_devices_callback, &enum_test, 0);
    ok(hr == DI_OK, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test.device_count != 0, "Device count is %u\n", enum_test.device_count);

    /* Enumeration only stops with an explicit DIENUM_STOP. */
    enum_test_return.device_count = 0;
    enum_test_return.return_value = 42;
    hr = IDirectInput8_EnumDevices(pDI, 0, enum_devices_callback, &enum_test_return, 0);
    ok(hr == DI_OK, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test_return.device_count == enum_test.device_count,
       "Device count is %u vs. %u\n", enum_test_return.device_count, enum_test.device_count);

    enum_test.device_count = 0;
    enum_test.return_value = DIENUM_STOP;
    hr = IDirectInput8_EnumDevices(pDI, 0, enum_devices_callback, &enum_test, 0);
    ok(hr == DI_OK, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test.device_count == 1, "Device count is %u\n", enum_test.device_count);

    IDirectInput8_Release(pDI);
}

struct enum_semantics_test
{
    unsigned int device_count;
    DWORD first_remaining;
    BOOL mouse;
    BOOL keyboard;
    DIACTIONFORMATA *lpdiaf;
    const char* username;
};

static DIACTIONA actionMapping[]=
{
  /* axis */
  { 0, 0x01008A01 /* DIAXIS_DRIVINGR_STEER */,      0, { "Steer.\0" }   },
  /* button */
  { 1, 0x01000C01 /* DIBUTTON_DRIVINGR_SHIFTUP */,  0, { "Upshift.\0" } },
  /* keyboard key */
  { 2, DIKEYBOARD_SPACE,                            0, { "Missile.\0" } },
  /* mouse button */
  { 3, DIMOUSE_BUTTON0,                             0, { "Select\0" }   },
  /* mouse axis */
  { 4, DIMOUSE_YAXIS,                               0, { "Y Axis\0" }   }
};
/* By placing the memory pointed to by lptszActionName right before memory with PAGE_NOACCESS
 * one can find out that the regular ansi string termination is not respected by EnumDevicesBySemantics.
 * Adding a double termination, making it a valid wide string termination, made the test succeed.
 * Therefore it looks like ansi version of EnumDevicesBySemantics forwards the string to
 * the wide variant without conversation. */

static BOOL CALLBACK enum_semantics_callback(const DIDEVICEINSTANCEA *lpddi, IDirectInputDevice8A *lpdid, DWORD dwFlags, DWORD dwRemaining, void *context)
{
    struct enum_semantics_test *data = context;

    if (context == NULL) return DIENUM_STOP;

    if (!data->device_count) {
        data->first_remaining = dwRemaining;
    }
    ok (dwRemaining == data->first_remaining - data->device_count,
        "enum semantics remaining devices is wrong, expected %d, had %d\n",
        data->first_remaining - data->device_count, dwRemaining);
    data->device_count++;

    if (IsEqualGUID(&lpddi->guidInstance, &GUID_SysKeyboard)) data->keyboard = TRUE;

    if (IsEqualGUID(&lpddi->guidInstance, &GUID_SysMouse)) data->mouse = TRUE;

    return DIENUM_CONTINUE;
}

static BOOL CALLBACK set_action_map_callback(const DIDEVICEINSTANCEA *lpddi, IDirectInputDevice8A *lpdid, DWORD dwFlags, DWORD dwRemaining, void *context)
{
    HRESULT hr;
    struct enum_semantics_test *data = context;

    /* Building and setting an action map */
    /* It should not use any pre-stored mappings so we use DIDBAM_INITIALIZE */
    hr = IDirectInputDevice8_BuildActionMap(lpdid, data->lpdiaf, NULL, DIDBAM_INITIALIZE);
    ok (SUCCEEDED(hr), "BuildActionMap failed hr=%08x\n", hr);

    hr = IDirectInputDevice8_SetActionMap(lpdid, data->lpdiaf, data->username, 0);
    ok (SUCCEEDED(hr), "SetActionMap failed hr=%08x\n", hr);

    return DIENUM_CONTINUE;
}

static void test_EnumDevicesBySemantics(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;
    DIACTIONFORMATA diaf;
    const GUID ACTION_MAPPING_GUID = { 0x1, 0x2, 0x3, { 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb } };
    struct enum_semantics_test data = { 0, 0, FALSE, FALSE, &diaf, NULL };
    int device_total = 0;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    memset (&diaf, 0, sizeof(diaf));
    diaf.dwSize = sizeof(diaf);
    diaf.dwActionSize = sizeof(DIACTIONA);
    diaf.dwNumActions = ARRAY_SIZE(actionMapping);
    diaf.dwDataSize = 4 * diaf.dwNumActions;
    diaf.rgoAction = actionMapping;
    diaf.guidActionMap = ACTION_MAPPING_GUID;
    diaf.dwGenre = 0x01000000; /* DIVIRTUAL_DRIVING_RACE */
    diaf.dwBufferSize = 32;

    /* Test enumerating all attached and installed devices */
    data.keyboard = FALSE;
    data.mouse = FALSE;
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &diaf, enum_semantics_callback, &data, DIEDBSFL_ATTACHEDONLY);
    ok (data.device_count > 0, "EnumDevicesBySemantics did not call the callback hr=%08x\n", hr);
    ok (data.keyboard, "EnumDevicesBySemantics should enumerate the keyboard\n");
    ok (data.mouse, "EnumDevicesBySemantics should enumerate the mouse\n");

    /* Enumerate Force feedback devices. We should get no mouse nor keyboard */
    data.keyboard = FALSE;
    data.mouse = FALSE;
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &diaf, enum_semantics_callback, &data, DIEDBSFL_FORCEFEEDBACK);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%08x\n", hr);
    ok (!data.keyboard, "Keyboard should not be enumerated when asking for forcefeedback\n");
    ok (!data.mouse, "Mouse should not be enumerated when asking for forcefeedback\n");

    /* Enumerate available devices. That is devices not owned by any user.
       Before setting the action map for all devices we still have them available. */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &diaf, enum_semantics_callback, &data, DIEDBSFL_AVAILABLEDEVICES);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%08x\n", hr);
    ok (data.device_count > 0, "There should be devices available before action mapping available=%d\n", data.device_count);

    /* Keep the device total */
    device_total = data.device_count;

    /* There should be no devices for any user. No device should be enumerated with DIEDBSFL_THISUSER.
       MSDN defines that all unowned devices are also enumerated but this doesn't seem to be happening. */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, "Sh4d0w M4g3", &diaf, enum_semantics_callback, &data, DIEDBSFL_THISUSER);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%08x\n", hr);
    ok (data.device_count == 0, "No devices should be assigned for this user assigned=%d\n", data.device_count);

    /* This enumeration builds and sets the action map for all devices with a NULL username */
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &diaf, set_action_map_callback, &data, DIEDBSFL_ATTACHEDONLY);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed: hr=%08x\n", hr);

    /* After a successful action mapping we should have no devices available */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &diaf, enum_semantics_callback, &data, DIEDBSFL_AVAILABLEDEVICES);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%08x\n", hr);
    ok (data.device_count == 0, "No device should be available after action mapping available=%d\n", data.device_count);

    /* Now we'll give all the devices to a specific user */
    data.username = "Sh4d0w M4g3";
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &diaf, set_action_map_callback, &data, DIEDBSFL_ATTACHEDONLY);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed: hr=%08x\n", hr);

    /* Testing with the default user, DIEDBSFL_THISUSER has no effect */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &diaf, enum_semantics_callback, &data, DIEDBSFL_THISUSER);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%08x\n", hr);
    ok (data.device_count == device_total, "THISUSER has no effect with NULL username owned=%d, expected=%d\n", data.device_count, device_total);

    /* Using an empty user string is the same as passing NULL, DIEDBSFL_THISUSER has no effect */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, "", &diaf, enum_semantics_callback, &data, DIEDBSFL_THISUSER);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%08x\n", hr);
    ok (data.device_count == device_total, "THISUSER has no effect with \"\" as username owned=%d, expected=%d\n", data.device_count, device_total);

    /* Testing with a user with no ownership of the devices */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, "Ninja Brian", &diaf, enum_semantics_callback, &data, DIEDBSFL_THISUSER);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%08x\n", hr);
    ok (data.device_count == 0, "This user should own no devices owned=%d\n", data.device_count);

    /* Sh4d0w M4g3 has ownership of all devices */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, "Sh4d0w M4g3", &diaf, enum_semantics_callback, &data, DIEDBSFL_THISUSER);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%08x\n", hr);
    ok (data.device_count == device_total, "This user should own %d devices owned=%d\n", device_total, data.device_count);

    /* The call fails with a zeroed GUID */
    memset(&diaf.guidActionMap, 0, sizeof(GUID));
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &diaf, enum_semantics_callback, NULL, 0);
    todo_wine ok(FAILED(hr), "EnumDevicesBySemantics succeeded with invalid GUID hr=%08x\n", hr);

    IDirectInput8_Release(pDI);
}

static void test_GetDeviceStatus(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_GetDeviceStatus(pDI, NULL);
    ok(hr == E_POINTER, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_GetDeviceStatus(pDI, &GUID_Unknown);
    todo_wine
    ok(hr == DIERR_DEVICENOTREG, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_GetDeviceStatus(pDI, &GUID_SysMouse);
    ok(hr == DI_OK, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    IDirectInput8_Release(pDI);
}

static void test_RunControlPanel(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    if (winetest_interactive)
    {
        hr = IDirectInput8_RunControlPanel(pDI, NULL, 0);
        ok(hr == S_OK, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

        hr = IDirectInput8_RunControlPanel(pDI, GetDesktopWindow(), 0);
        ok(hr == S_OK, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);
    }

    hr = IDirectInput8_RunControlPanel(pDI, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, (HWND)0xdeadbeef, 0);
    ok(hr == E_HANDLE, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, (HWND)0xdeadbeef, ~0u);
    ok(hr == E_HANDLE, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    IDirectInput8_Release(pDI);
}

static void test_Initialize(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_Initialize(pDI, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    hr = IDirectInput8_Initialize(pDI, NULL, DIRECTINPUT_VERSION);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    hr = IDirectInput8_Initialize(pDI, hInstance, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    /* Invalid DirectInput versions less than DIRECTINPUT_VERSION yield DIERR_BETADIRECTINPUTVERSION. */
    hr = IDirectInput8_Initialize(pDI, hInstance, DIRECTINPUT_VERSION - 1);
    ok(hr == DIERR_BETADIRECTINPUTVERSION, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    /* Invalid DirectInput versions greater than DIRECTINPUT_VERSION yield DIERR_BETADIRECTINPUTVERSION. */
    hr = IDirectInput8_Initialize(pDI, hInstance, DIRECTINPUT_VERSION + 1);
    ok(hr == DIERR_OLDDIRECTINPUTVERSION, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    hr = IDirectInput8_Initialize(pDI, hInstance, DIRECTINPUT_VERSION);
    ok(hr == DI_OK, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    /* Parameters are still validated after successful initialization. */
    hr = IDirectInput8_Initialize(pDI, hInstance, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    IDirectInput8_Release(pDI);
}

START_TEST(dinput)
{
    hInstance = GetModuleHandleA(NULL);

    CoInitialize(NULL);
    test_preinitialization();
    test_DirectInput8Create();
    test_QueryInterface();
    test_CreateDevice();
    test_EnumDevices();
    test_EnumDevicesBySemantics();
    test_GetDeviceStatus();
    test_RunControlPanel();
    test_Initialize();
    CoUninitialize();
}
