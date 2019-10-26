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

#define DIRECTINPUT_VERSION 0x0700

#define COBJMACROS
#include <initguid.h>
#include <windows.h>
#include <dinput.h>
#include <dinputd.h>

#include "wine/test.h"

HINSTANCE hInstance;

enum directinput_versions
{
    DIRECTINPUT_VERSION_300 = 0x0300,
    DIRECTINPUT_VERSION_500 = 0x0500,
    DIRECTINPUT_VERSION_50A = 0x050A,
    DIRECTINPUT_VERSION_5B2 = 0x05B2,
    DIRECTINPUT_VERSION_602 = 0x0602,
    DIRECTINPUT_VERSION_61A = 0x061A,
    DIRECTINPUT_VERSION_700 = 0x0700,
};

static const DWORD directinput_version_list[] =
{
    DIRECTINPUT_VERSION_300,
    DIRECTINPUT_VERSION_500,
    DIRECTINPUT_VERSION_50A,
    DIRECTINPUT_VERSION_5B2,
    DIRECTINPUT_VERSION_602,
    DIRECTINPUT_VERSION_61A,
    DIRECTINPUT_VERSION_700,
};

static HRESULT (WINAPI *pDirectInputCreateEx)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);

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

    IDirectInputA *pDI;
    HRESULT hr;
    int i;
    IDirectInputDeviceA *pDID;

    hr = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectInputA, (void **)&pDI);
    if (FAILED(hr))
    {
        skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(create_device_tests); i++)
    {
        if (create_device_tests[i].pdev) pDID = (void *)0xdeadbeef;
        hr = IDirectInput_CreateDevice(pDI, create_device_tests[i].rguid,
                                            create_device_tests[i].pdev ? &pDID : NULL,
                                            NULL);
        ok(hr == create_device_tests[i].expected_hr, "[%d] IDirectInput_CreateDevice returned 0x%08x\n", i, hr);
        if (create_device_tests[i].pdev)
            ok(pDID == NULL, "[%d] Output interface pointer is %p\n", i, pDID);
    }

    for (i = 0; i < ARRAY_SIZE(enum_devices_tests); i++)
    {
        hr = IDirectInput_EnumDevices(pDI, enum_devices_tests[i].dwDevType,
                                           enum_devices_tests[i].lpCallback,
                                           NULL,
                                           enum_devices_tests[i].dwFlags);
        todo_wine_if(enum_devices_tests[i].todo)
            ok(hr == enum_devices_tests[i].expected_hr, "[%d] IDirectInput_EnumDevice returned 0x%08x\n", i, hr);
    }

    hr = IDirectInput_GetDeviceStatus(pDI, NULL);
    ok(hr == E_POINTER, "IDirectInput_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput_GetDeviceStatus(pDI, &GUID_Unknown);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput_GetDeviceStatus(pDI, &GUID_SysMouse);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput_RunControlPanel(pDI, NULL, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput_RunControlPanel(pDI, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput_RunControlPanel(pDI, (HWND)0xdeadbeef, 0);
    ok(hr == E_HANDLE, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput_RunControlPanel(pDI, (HWND)0xdeadbeef, ~0u);
    ok(hr == E_HANDLE, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    IDirectInput_Release(pDI);
}

static void test_DirectInputCreateEx(void)
{
    static const struct
    {
        BOOL hinst;
        DWORD dwVersion;
        REFIID riid;
        BOOL ppdi;
        HRESULT expected_hr;
        IUnknown *expected_ppdi;
    } invalid_param_list[] =
    {
        {FALSE, 0,                       &IID_IUnknown,  FALSE, DIERR_NOINTERFACE},
        {FALSE, 0,                       &IID_IUnknown,  TRUE,  DIERR_NOINTERFACE, (void *)0xdeadbeef},
        {FALSE, 0,                       &IID_IDirectInputA, FALSE, E_POINTER},
        {FALSE, 0,                       &IID_IDirectInputA, TRUE,  DIERR_INVALIDPARAM, NULL},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IUnknown,  FALSE, DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IUnknown,  TRUE,  DIERR_NOINTERFACE, (void *)0xdeadbeef},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInputA, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInputA, TRUE,  DIERR_INVALIDPARAM, NULL},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IUnknown,  FALSE, DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IUnknown,  TRUE,  DIERR_NOINTERFACE, (void *)0xdeadbeef},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInputA, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInputA, TRUE,  DIERR_INVALIDPARAM, NULL},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IUnknown,  FALSE, DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IUnknown,  TRUE,  DIERR_NOINTERFACE, (void *)0xdeadbeef},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInputA, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInputA, TRUE,  DIERR_INVALIDPARAM, NULL},
        {TRUE,  0,                       &IID_IUnknown,  FALSE, DIERR_NOINTERFACE},
        {TRUE,  0,                       &IID_IUnknown,  TRUE,  DIERR_NOINTERFACE, (void *)0xdeadbeef},
        {TRUE,  0,                       &IID_IDirectInputA, FALSE, E_POINTER},
        {TRUE,  0,                       &IID_IDirectInputA, TRUE,  DIERR_NOTINITIALIZED, NULL},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IUnknown,  FALSE, DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IUnknown,  TRUE,  DIERR_NOINTERFACE, (void *)0xdeadbeef},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInputA, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IUnknown,  FALSE, DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IUnknown,  TRUE,  DIERR_NOINTERFACE, (void *)0xdeadbeef},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInputA, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInputA, TRUE,  DIERR_BETADIRECTINPUTVERSION, NULL},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IUnknown,  FALSE, DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IUnknown,  TRUE,  DIERR_NOINTERFACE, (void *)0xdeadbeef},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInputA, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInputA, TRUE,  DIERR_OLDDIRECTINPUTVERSION, NULL},
    };

    static REFIID no_interface_list[] = {&IID_IUnknown, &IID_IDirectInput8A,
                                         &IID_IDirectInput8W, &IID_IDirectInputDeviceA,
                                         &IID_IDirectInputDeviceW, &IID_IDirectInputDevice2A,
                                         &IID_IDirectInputDevice2W, &IID_IDirectInputDevice7A,
                                         &IID_IDirectInputDevice7W, &IID_IDirectInputDevice8A,
                                         &IID_IDirectInputDevice8W, &IID_IDirectInputEffect};

    static REFIID iid_list[] = {&IID_IDirectInputA, &IID_IDirectInputW,
                                &IID_IDirectInput2A, &IID_IDirectInput2W,
                                &IID_IDirectInput7A, &IID_IDirectInput7W};

    int i, j;
    IUnknown *pUnk;
    HRESULT hr;

    if (!pDirectInputCreateEx)
    {
        win_skip("DirectInputCreateEx is not available\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(invalid_param_list); i++)
    {
        if (invalid_param_list[i].ppdi) pUnk = (void *)0xdeadbeef;
        hr = pDirectInputCreateEx(invalid_param_list[i].hinst ? hInstance : NULL,
                                  invalid_param_list[i].dwVersion,
                                  invalid_param_list[i].riid,
                                  invalid_param_list[i].ppdi ? (void **)&pUnk : NULL,
                                  NULL);
        ok(hr == invalid_param_list[i].expected_hr, "[%d] DirectInputCreateEx returned 0x%08x\n", i, hr);
        if (invalid_param_list[i].ppdi)
            ok(pUnk == invalid_param_list[i].expected_ppdi, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    for (i = 0; i < ARRAY_SIZE(no_interface_list); i++)
    {
        pUnk = (void *)0xdeadbeef;
        hr = pDirectInputCreateEx(hInstance, DIRECTINPUT_VERSION, no_interface_list[i], (void **)&pUnk, NULL);
        ok(hr == DIERR_NOINTERFACE, "[%d] DirectInputCreateEx returned 0x%08x\n", i, hr);
        ok(pUnk == (void *)0xdeadbeef, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    for (i = 0; i < ARRAY_SIZE(iid_list); i++)
    {
        pUnk = NULL;
        hr = pDirectInputCreateEx(hInstance, DIRECTINPUT_VERSION, iid_list[i], (void **)&pUnk, NULL);
        ok(hr == DI_OK, "[%d] DirectInputCreateEx returned 0x%08x\n", i, hr);
        ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
        if (pUnk)
            IUnknown_Release(pUnk);
    }

    /* Examine combinations of requested interfaces and version numbers. */
    for (i = 0; i < ARRAY_SIZE(directinput_version_list); i++)
    {
        for (j = 0; j < ARRAY_SIZE(iid_list); j++)
        {
            pUnk = NULL;
            hr = pDirectInputCreateEx(hInstance, directinput_version_list[i], iid_list[j], (void **)&pUnk, NULL);
            ok(hr == DI_OK, "[%d/%d] DirectInputCreateEx returned 0x%08x\n", i, j, hr);
            ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
            if (pUnk)
                IUnknown_Release(pUnk);
        }
    }
}

static void test_QueryInterface(void)
{
    static REFIID iid_list[] = {&IID_IUnknown, &IID_IDirectInputA, &IID_IDirectInputW,
                                &IID_IDirectInput2A, &IID_IDirectInput2W,
                                &IID_IDirectInput7A, &IID_IDirectInput7W};

    static REFIID no_interface_list[] =
    {
        &IID_IDirectInput8A,
        &IID_IDirectInput8W,
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

    IDirectInputA *pDI;
    HRESULT hr;
    IUnknown *pUnk;
    int i;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput_QueryInterface(pDI, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput_QueryInterface returned 0x%08x\n", hr);

    pUnk = (void *)0xdeadbeef;
    hr = IDirectInput_QueryInterface(pDI, NULL, (void **)&pUnk);
    ok(hr == E_POINTER, "IDirectInput_QueryInterface returned 0x%08x\n", hr);
    ok(pUnk == (void *)0xdeadbeef, "Output interface pointer is %p\n", pUnk);

    hr = IDirectInput_QueryInterface(pDI, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "IDirectInput_QueryInterface returned 0x%08x\n", hr);

    for (i = 0; i < ARRAY_SIZE(iid_list); i++)
    {
        pUnk = NULL;
        hr = IDirectInput_QueryInterface(pDI, iid_list[i], (void **)&pUnk);
        ok(hr == S_OK, "[%d] IDirectInput_QueryInterface returned 0x%08x\n", i, hr);
        ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
        if (pUnk) IUnknown_Release(pUnk);
    }

    for (i = 0; i < ARRAY_SIZE(no_interface_list); i++)
    {
        pUnk = (void *)0xdeadbeef;
        hr = IDirectInput_QueryInterface(pDI, no_interface_list[i], (void **)&pUnk);
        ok(hr == E_NOINTERFACE, "[%d] IDirectInput_QueryInterface returned 0x%08x\n", i, hr);
        ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    IDirectInput_Release(pDI);
}

static void test_CreateDevice(void)
{
    IDirectInputA *pDI;
    HRESULT hr;
    IDirectInputDeviceA *pDID;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput_CreateDevice(pDI, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput_CreateDevice returned 0x%08x\n", hr);

    pDID = (void *)0xdeadbeef;
    hr = IDirectInput_CreateDevice(pDI, NULL, &pDID, NULL);
    ok(hr == E_POINTER, "IDirectInput_CreateDevice returned 0x%08x\n", hr);
    ok(pDID == NULL, "Output interface pointer is %p\n", pDID);

    hr = IDirectInput_CreateDevice(pDI, &GUID_Unknown, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput_CreateDevice returned 0x%08x\n", hr);

    pDID = (void *)0xdeadbeef;
    hr = IDirectInput_CreateDevice(pDI, &GUID_Unknown, &pDID, NULL);
    ok(hr == DIERR_DEVICENOTREG, "IDirectInput_CreateDevice returned 0x%08x\n", hr);
    ok(pDID == NULL, "Output interface pointer is %p\n", pDID);

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput_CreateDevice returned 0x%08x\n", hr);

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pDID, NULL);
    ok(hr == DI_OK, "IDirectInput_CreateDevice returned 0x%08x\n", hr);

    IDirectInputDevice_Release(pDID);
    IDirectInput_Release(pDI);
}

struct enum_devices_test
{
    unsigned int device_count;
    BOOL return_value;
};

static BOOL CALLBACK enum_devices_callback(const DIDEVICEINSTANCEA *instance, void *context)
{
    struct enum_devices_test *enum_test = context;

    if ((instance->dwDevType & 0xff) == DIDEVTYPE_KEYBOARD ||
           (instance->dwDevType & 0xff) == DIDEVTYPE_MOUSE) {
        const char *device = ((instance->dwDevType & 0xff) ==
                                   DIDEVTYPE_KEYBOARD) ? "Keyboard" : "Mouse";
        ok(IsEqualGUID(&instance->guidInstance, &instance->guidProduct),
           "%s guidInstance (%s) does not match guidProduct (%s)\n",
           device, wine_dbgstr_guid(&instance->guidInstance),
           wine_dbgstr_guid(&instance->guidProduct));
    }

    if ((instance->dwDevType & 0xff) == DIDEVTYPE_KEYBOARD)
        ok(IsEqualGUID(&instance->guidProduct, &GUID_SysKeyboard),
           "Keyboard guidProduct (%s) does not match GUID_SysKeyboard (%s)\n",
           wine_dbgstr_guid(&instance->guidProduct),
           wine_dbgstr_guid(&GUID_SysMouse));
    else if ((instance->dwDevType & 0xff) == DIDEVTYPE_MOUSE)
        ok(IsEqualGUID(&instance->guidProduct, &GUID_SysMouse),
           "Mouse guidProduct (%s) does not match GUID_SysMouse (%s)\n",
           wine_dbgstr_guid(&instance->guidProduct),
           wine_dbgstr_guid(&GUID_SysMouse));
    else {
        /* Non-keyboard/mouse devices use the "PIDVID" guidProduct */
        static const GUID pidvid_product_guid = { /* device_pidvid-0000-0000-0000-504944564944 "PIDVID" */
          0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x50, 0x49, 0x44, 0x56, 0x49, 0x44}
        };

        ok(instance->guidProduct.Data2 == pidvid_product_guid.Data2,
           "guidProduct.Data2 is %04x\n", instance->guidProduct.Data2);
        ok(instance->guidProduct.Data3 == pidvid_product_guid.Data3,
           "guidProduct.Data3 is %04x\n", instance->guidProduct.Data3);
        ok(!memcmp(instance->guidProduct.Data4, pidvid_product_guid.Data4, sizeof(pidvid_product_guid.Data4)),
           "guidProduct.Data4 does not match: %s\n", wine_dbgstr_guid(&instance->guidProduct));
    }

    enum_test->device_count++;
    return enum_test->return_value;
}

static void test_EnumDevices(void)
{
    IDirectInputA *pDI;
    HRESULT hr;
    struct enum_devices_test enum_test, enum_test_return;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput_EnumDevices(pDI, 0, NULL, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput_EnumDevices(pDI, 0, NULL, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_EnumDevices returned 0x%08x\n", hr);

    /* Test crashes on Wine. */
    if (0)
    {
        hr = IDirectInput_EnumDevices(pDI, 0, enum_devices_callback, NULL, ~0u);
        ok(hr == DIERR_INVALIDPARAM, "IDirectInput_EnumDevices returned 0x%08x\n", hr);
    }

    hr = IDirectInput_EnumDevices(pDI, 0xdeadbeef, NULL, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput_EnumDevices(pDI, 0xdeadbeef, NULL, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput_EnumDevices(pDI, 0xdeadbeef, enum_devices_callback, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput_EnumDevices(pDI, 0xdeadbeef, enum_devices_callback, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_EnumDevices returned 0x%08x\n", hr);

    enum_test.device_count = 0;
    enum_test.return_value = DIENUM_CONTINUE;
    hr = IDirectInput_EnumDevices(pDI, 0, enum_devices_callback, &enum_test, 0);
    ok(hr == DI_OK, "IDirectInput_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test.device_count != 0, "Device count is %u\n", enum_test.device_count);

    /* Enumeration only stops with an explicit DIENUM_STOP. */
    enum_test_return.device_count = 0;
    enum_test_return.return_value = 42;
    hr = IDirectInput_EnumDevices(pDI, 0, enum_devices_callback, &enum_test_return, 0);
    ok(hr == DI_OK, "IDirectInput_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test_return.device_count == enum_test.device_count,
       "Device count is %u vs. %u\n", enum_test_return.device_count, enum_test.device_count);

    enum_test.device_count = 0;
    enum_test.return_value = DIENUM_STOP;
    hr = IDirectInput_EnumDevices(pDI, 0, enum_devices_callback, &enum_test, 0);
    ok(hr == DI_OK, "IDirectInput_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test.device_count == 1, "Device count is %u\n", enum_test.device_count);

    IDirectInput_Release(pDI);
}

static void test_GetDeviceStatus(void)
{
    IDirectInputA *pDI;
    HRESULT hr;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput_GetDeviceStatus(pDI, NULL);
    ok(hr == E_POINTER, "IDirectInput_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput_GetDeviceStatus(pDI, &GUID_Unknown);
    todo_wine
    ok(hr == DIERR_DEVICENOTREG, "IDirectInput_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput_GetDeviceStatus(pDI, &GUID_SysMouse);
    ok(hr == DI_OK, "IDirectInput_GetDeviceStatus returned 0x%08x\n", hr);

    IDirectInput_Release(pDI);
}

static void test_Initialize(void)
{
    IDirectInputA *pDI;
    HRESULT hr;
    int i;

    hr = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectInputA, (void **)&pDI);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput_Initialize(pDI, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_Initialize returned 0x%08x\n", hr);

    hr = IDirectInput_Initialize(pDI, NULL, DIRECTINPUT_VERSION);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_Initialize returned 0x%08x\n", hr);

    hr = IDirectInput_Initialize(pDI, hInstance, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput_Initialize returned 0x%08x\n", hr);

    /* Invalid DirectInput versions less than 0x0700 yield DIERR_BETADIRECTINPUTVERSION. */
    hr = IDirectInput_Initialize(pDI, hInstance, 0x0123);
    ok(hr == DIERR_BETADIRECTINPUTVERSION, "IDirectInput_Initialize returned 0x%08x\n", hr);

    /* Invalid DirectInput versions greater than 0x0700 yield DIERR_BETADIRECTINPUTVERSION. */
    hr = IDirectInput_Initialize(pDI, hInstance, 0xcafe);
    ok(hr == DIERR_OLDDIRECTINPUTVERSION, "IDirectInput_Initialize returned 0x%08x\n", hr);

    for (i = 0; i < ARRAY_SIZE(directinput_version_list); i++)
    {
        hr = IDirectInput_Initialize(pDI, hInstance, directinput_version_list[i]);
        ok(hr == DI_OK, "IDirectInput_Initialize returned 0x%08x\n", hr);
    }

    /* Parameters are still validated after successful initialization. */
    hr = IDirectInput_Initialize(pDI, hInstance, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput_Initialize returned 0x%08x\n", hr);

    IDirectInput_Release(pDI);
}

static void test_RunControlPanel(void)
{
    IDirectInputA *pDI;
    HRESULT hr;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    if (winetest_interactive)
    {
        hr = IDirectInput_RunControlPanel(pDI, NULL, 0);
        ok(hr == S_OK, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

        hr = IDirectInput_RunControlPanel(pDI, GetDesktopWindow(), 0);
        ok(hr == S_OK, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);
    }

    hr = IDirectInput_RunControlPanel(pDI, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput_RunControlPanel(pDI, (HWND)0xdeadbeef, 0);
    ok(hr == E_HANDLE, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput_RunControlPanel(pDI, (HWND)0xdeadbeef, ~0u);
    ok(hr == E_HANDLE, "IDirectInput_RunControlPanel returned 0x%08x\n", hr);

    IDirectInput_Release(pDI);
}

static void test_DirectInputJoyConfig8(void)
{
    IDirectInputA *pDI;
    IDirectInputDeviceA *pDID;
    IDirectInputJoyConfig8 *pDIJC;
    DIJOYCONFIG info;
    HRESULT hr;
    int i;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput_QueryInterface(pDI, &IID_IDirectInputJoyConfig8, (void **)&pDIJC);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputJoyConfig8 instance: 0x%08x\n", hr);
        return;
    }

    info.dwSize = sizeof(info);
    hr = DI_OK;
    i = 0;

    /* Enumerate all connected joystick GUIDs and try to create the respective devices */
    for (i = 0; SUCCEEDED(hr); i++)
    {
        hr = IDirectInputJoyConfig8_GetConfig(pDIJC, i, &info, DIJC_GUIDINSTANCE);

        ok (hr == DI_OK || hr == DIERR_NOMOREITEMS,
           "IDirectInputJoyConfig8_GetConfig returned 0x%08x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDirectInput_CreateDevice(pDI, &info.guidInstance, &pDID, NULL);
            ok (SUCCEEDED(hr), "IDirectInput_CreateDevice failed with guid from GetConfig hr = 0x%08x\n", hr);
            IDirectInputDevice_Release(pDID);
        }
    }

    IDirectInputJoyConfig8_Release(pDIJC);
    IDirectInput_Release(pDI);
}

START_TEST(dinput)
{
    HMODULE dinput_mod = GetModuleHandleA("dinput.dll");

    hInstance = GetModuleHandleA(NULL);

    pDirectInputCreateEx = (void *)GetProcAddress(dinput_mod, "DirectInputCreateEx");

    CoInitialize(NULL);
    test_preinitialization();
    test_DirectInputCreateEx();
    test_QueryInterface();
    test_CreateDevice();
    test_EnumDevices();
    test_GetDeviceStatus();
    test_Initialize();
    test_RunControlPanel();
    test_DirectInputJoyConfig8();
    CoUninitialize();
}
