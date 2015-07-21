/*
 * Copyright (c) 2005 Robert Reif
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

#define DIRECTINPUT_VERSION 0x0700

#define COBJMACROS
//#include <windows.h>

//#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>

#include <wine/test.h>
//#include "windef.h"
//#include "wingdi.h"
#include <dinput.h>

static void acquire_tests(IDirectInputA *pDI, HWND hwnd)
{
    HRESULT hr;
    IDirectInputDeviceA *pKeyboard;
    BYTE kbd_state[256];
    LONG custom_state[6];
    int i;
    DIOBJECTDATAFORMAT dodf[] =
        {
            { &GUID_Key, sizeof(LONG) * 0, DIDFT_MAKEINSTANCE(DIK_Q)|DIDFT_BUTTON, 0 },
            { &GUID_Key, sizeof(LONG) * 1, DIDFT_MAKEINSTANCE(DIK_W)|DIDFT_BUTTON, 0 },
            { &GUID_Key, sizeof(LONG) * 2, DIDFT_MAKEINSTANCE(DIK_E)|DIDFT_BUTTON, 0 },
            { &GUID_Key, sizeof(LONG) * 4, DIDFT_MAKEINSTANCE(DIK_R)|DIDFT_BUTTON, 0 },
        };

    DIDATAFORMAT df;
    df.dwSize = sizeof( df );
    df.dwObjSize = sizeof( DIOBJECTDATAFORMAT );
    df.dwFlags = DIDF_RELAXIS;
    df.dwDataSize = sizeof( custom_state );
    df.dwNumObjs = sizeof( dodf )/sizeof( dodf[0] );
    df.rgodf = dodf;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKeyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInputDevice_SetDataFormat(pKeyboard, &c_dfDIKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %08x\n", hr);
    hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, NULL, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetCooperativeLevel() failed: %08x\n", hr);
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, 10, kbd_state);
    ok(hr == DIERR_NOTACQUIRED, "IDirectInputDevice_GetDeviceState(10,) should have failed: %08x\n", hr);
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(kbd_state), kbd_state);
    ok(hr == DIERR_NOTACQUIRED, "IDirectInputDevice_GetDeviceState() should have failed: %08x\n", hr);
    hr = IDirectInputDevice_Unacquire(pKeyboard);
    ok(hr == S_FALSE, "IDirectInputDevice_Unacquire() should have failed: %08x\n", hr);
    hr = IDirectInputDevice_Acquire(pKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %08x\n", hr);
    hr = IDirectInputDevice_Acquire(pKeyboard);
    ok(hr == S_FALSE, "IDirectInputDevice_Acquire() should have failed: %08x\n", hr);
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, 10, kbd_state);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInputDevice_GetDeviceState(10,) should have failed: %08x\n", hr);
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(kbd_state), kbd_state);
    ok(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceState() failed: %08x\n", hr);
    hr = IDirectInputDevice_Unacquire(pKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Uncquire() failed: %08x\n", hr);
    hr = IDirectInputDevice_SetDataFormat( pKeyboard , &df );
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %08x\n", hr);
    hr = IDirectInputDevice_Acquire(pKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %08x\n", hr);
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(custom_state), custom_state);
    ok(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceState(4,) failed: %08x\n", hr);
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(kbd_state), kbd_state);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInputDevice_GetDeviceState(256,) should have failed: %08x\n", hr);

    memset(custom_state, 0x56, sizeof(custom_state));
    IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(custom_state), custom_state);
    for (i = 0; i < sizeof(custom_state) / sizeof(custom_state[0]); i++)
        ok(custom_state[i] == 0, "Should be zeroed, got 0x%08x\n", custom_state[i]);

    /* simulate some keyboard input */
    SetFocus(hwnd);
    keybd_event('Q', 0, 0, 0);
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(custom_state), custom_state);
    ok(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceState() failed: %08x\n", hr);
    if (!custom_state[0])
        win_skip("Keyboard event not processed, skipping test\n");
    else
    {
        /* unacquiring should reset the device state */
        hr = IDirectInputDevice_Unacquire(pKeyboard);
        ok(SUCCEEDED(hr), "IDirectInputDevice_Unacquire() failed: %08x\n", hr);
        hr = IDirectInputDevice_Acquire(pKeyboard);
        ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %08x\n", hr);
        hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(custom_state), custom_state);
        ok(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceState failed: %08x\n", hr);
        for (i = 0; i < sizeof(custom_state) / sizeof(custom_state[0]); i++)
            ok(custom_state[i] == 0, "Should be zeroed, got 0x%08x\n", custom_state[i]);
    }
    keybd_event('Q', 0, KEYEVENTF_KEYUP, 0);

    if (pKeyboard) IUnknown_Release(pKeyboard);
}

static const HRESULT SetCoop_null_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static const HRESULT SetCoop_real_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, S_OK,         S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_NOTIMPL,    S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static const HRESULT SetCoop_child_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static void test_set_coop(IDirectInputA *pDI, HWND hwnd)
{
    HRESULT hr;
    IDirectInputDeviceA *pKeyboard = NULL;
    int i;
    HWND child;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKeyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, NULL, i);
        ok(hr == SetCoop_null_window[i], "SetCooperativeLevel(NULL, %d): %08x\n", i, hr);
    }
    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, hwnd, i);
        ok(hr == SetCoop_real_window[i], "SetCooperativeLevel(hwnd, %d): %08x\n", i, hr);
    }

    child = CreateWindowA("static", "Title", WS_CHILD | WS_VISIBLE, 10, 10, 50, 50, hwnd, NULL,
                          NULL, NULL);
    ok(child != NULL, "err: %d\n", GetLastError());

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, child, i);
        ok(hr == SetCoop_child_window[i], "SetCooperativeLevel(child, %d): %08x\n", i, hr);
    }

    DestroyWindow(child);
    if (pKeyboard) IUnknown_Release(pKeyboard);
}

static void test_get_prop(IDirectInputA *pDI, HWND hwnd)
{
    HRESULT hr;
    IDirectInputDeviceA *pKeyboard = NULL;
    DIPROPRANGE diprg;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKeyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    memset(&diprg, 0, sizeof(diprg));
    diprg.diph.dwSize       = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow        = DIPH_DEVICE;
    diprg.diph.dwObj        = 0;

    hr = IDirectInputDevice_GetProperty(pKeyboard, DIPROP_RANGE, &diprg.diph);
    ok(hr == DIERR_UNSUPPORTED, "IDirectInputDevice_GetProperty() did not return DIPROP_RANGE but: %08x\n", hr);

    if (pKeyboard) IUnknown_Release(pKeyboard);
}

static void test_capabilities(IDirectInputA *pDI, HWND hwnd)
{
    HRESULT hr;
    IDirectInputDeviceA *pKeyboard = NULL;
    DIDEVCAPS caps;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKeyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    caps.dwSize = sizeof(caps);
    hr = IDirectInputDevice_GetCapabilities(pKeyboard, &caps);

    ok (SUCCEEDED(hr), "GetCapabilities failed: 0x%08x\n", hr);
    ok (caps.dwFlags & DIDC_ATTACHED, "GetCapabilities dwFlags: 0x%08x\n", caps.dwFlags);
    ok (LOWORD(LOBYTE(caps.dwDevType)) == DIDEVTYPE_KEYBOARD,
        "GetCapabilities invalid device type for dwDevType: 0x%08x\n", caps.dwDevType);
    ok (LOWORD(HIBYTE(caps.dwDevType)) != DIDEVTYPEKEYBOARD_UNKNOWN,
        "GetCapabilities invalid device subtype for dwDevType: 0x%08x\n", caps.dwDevType);

    IUnknown_Release(pKeyboard);
}

static void keyboard_tests(DWORD version)
{
    HRESULT hr;
    IDirectInputA *pDI = NULL;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    HWND hwnd;
    ULONG ref = 0;

    hr = DirectInputCreateA(hInstance, version, &pDI, NULL);
    if (hr == DIERR_OLDDIRECTINPUTVERSION)
    {
        skip("Tests require a newer dinput version\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInputCreateA() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 10, 200, 200,
                         NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());

    if (hwnd)
    {
        acquire_tests(pDI, hwnd);
        test_set_coop(pDI, hwnd);
        test_get_prop(pDI, hwnd);
        test_capabilities(pDI, hwnd);
    }

    DestroyWindow(hwnd);
    if (pDI) ref = IUnknown_Release(pDI);
    ok(!ref, "IDirectInput_Release() reference count = %d\n", ref);
}

START_TEST(keyboard)
{
    CoInitialize(NULL);

    keyboard_tests(0x0700);

    CoUninitialize();
}
