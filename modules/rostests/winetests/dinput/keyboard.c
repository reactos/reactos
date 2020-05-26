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

#define DIRECTINPUT_VERSION 0x0700

#define COBJMACROS
#include <windows.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "wingdi.h"
#include "dinput.h"

/* to make things easier with PSDK without a dinput.lib */
static HRESULT (WINAPI *pDirectInputCreateA)(HINSTANCE,DWORD,IDirectInputA **,IUnknown *);

static void pump_messages(void)
{
    MSG msg;

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static HKL activate_keyboard_layout(LANGID langid, HKL *hkl_orig)
{
    HKL hkl, hkl_current;
    char hkl_name[64];

    sprintf(hkl_name, "%08x", langid);
    trace("Loading keyboard layout %s\n", hkl_name);
    hkl = LoadKeyboardLayoutA(hkl_name, 0);
    if (!hkl)
    {
        win_skip("Unable to load keyboard layout %s\n", hkl_name);
        return 0;
    }
    *hkl_orig = ActivateKeyboardLayout(hkl, 0);
    ok(*hkl_orig != 0, "Unable to activate keyboard layout %s\n", hkl_name);
    if (!*hkl_orig) return 0;

    hkl_current = GetKeyboardLayout(0);
    if (LOWORD(hkl_current) != langid)
    {
        /* FIXME: Wine can't activate different keyboard layouts.
         * for testing purposes use this workaround:
         * setxkbmap us && LANG=en_US.UTF-8 make test
         * setxkbmap fr && LANG=fr_FR.UTF-8 make test
         * setxkbmap de && LANG=de_DE.UTF-8 make test
         */
        skip("current %08x != langid %08x\n", LOWORD(hkl_current), langid);
        return 0;
    }

    return hkl;
}

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
    HKL hkl, hkl_orig;
    UINT prev_raw_devices_count, raw_devices_count;

    hkl = activate_keyboard_layout(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &hkl_orig);
    if (!hkl) return;

    df.dwSize = sizeof( df );
    df.dwObjSize = sizeof( DIOBJECTDATAFORMAT );
    df.dwFlags = DIDF_RELAXIS;
    df.dwDataSize = sizeof( custom_state );
    df.dwNumObjs = ARRAY_SIZE(dodf);
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
    for (i = 0; i < ARRAY_SIZE(custom_state); i++)
        ok(custom_state[i] == 0, "Should be zeroed, got 0x%08x\n", custom_state[i]);

    /* simulate some keyboard input */
    SetFocus(hwnd);
    pump_messages();

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
        for (i = 0; i < ARRAY_SIZE(custom_state); i++)
            ok(custom_state[i] == 0, "Should be zeroed, got 0x%08x\n", custom_state[i]);
    }
    keybd_event('Q', 0, KEYEVENTF_KEYUP, 0);

    prev_raw_devices_count = 0;
    GetRegisteredRawInputDevices(NULL, &prev_raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(prev_raw_devices_count == 0 || broken(prev_raw_devices_count == 1) /* wxppro, w2003std */,
       "Unexpected raw devices registered: %d\n", prev_raw_devices_count);

    hr = IDirectInputDevice_Acquire(pKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %08x\n", hr);

    raw_devices_count = 0;
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(raw_devices_count == prev_raw_devices_count,
       "Unexpected raw devices registered: %d\n", raw_devices_count);

    hr = IDirectInputDevice_Unacquire(pKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Unacquire() failed: %08x\n", hr);

    if (pKeyboard) IUnknown_Release(pKeyboard);

    ActivateKeyboardLayout(hkl_orig, 0);
    UnloadKeyboardLayout(hkl);
}

static const HRESULT SetCoop_null_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static const HRESULT SetCoop_invalid_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
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
        hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, (HWND)0x400000, i);
        ok(hr == SetCoop_invalid_window[i], "SetCooperativeLevel(invalid, %d): %08x\n", i, hr);
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
    int kbd_type, kbd_subtype, dev_subtype;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKeyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    caps.dwSize = sizeof(caps);
    hr = IDirectInputDevice_GetCapabilities(pKeyboard, &caps);

    ok (SUCCEEDED(hr), "GetCapabilities failed: 0x%08x\n", hr);
    ok (caps.dwFlags & DIDC_ATTACHED, "GetCapabilities dwFlags: 0x%08x\n", caps.dwFlags);
    ok (GET_DIDEVICE_TYPE(caps.dwDevType) == DIDEVTYPE_KEYBOARD,
        "GetCapabilities invalid device type for dwDevType: 0x%08x\n", caps.dwDevType);
    kbd_type = GetKeyboardType(0);
    kbd_subtype = GetKeyboardType(1);
    dev_subtype = GET_DIDEVICE_SUBTYPE(caps.dwDevType);
    if (kbd_type == 4 || (kbd_type == 7 && kbd_subtype == 0))
        ok (dev_subtype == DIDEVTYPEKEYBOARD_PCENH,
            "GetCapabilities invalid device subtype for dwDevType: 0x%08x (%04x:%04x)\n",
            caps.dwDevType, kbd_type, kbd_subtype);
    else if (kbd_type == 7 && kbd_subtype == 2)
        ok (dev_subtype == DIDEVTYPEKEYBOARD_JAPAN106,
            "GetCapabilities invalid device subtype for dwDevType: 0x%08x (%04x:%04x)\n",
            caps.dwDevType, kbd_type, kbd_subtype);
    else
        ok (dev_subtype != DIDEVTYPEKEYBOARD_UNKNOWN,
            "GetCapabilities invalid device subtype for dwDevType: 0x%08x (%04x:%04x)\n",
            caps.dwDevType, kbd_type, kbd_subtype);

    IUnknown_Release(pKeyboard);
}

static void test_dik_codes(IDirectInputA *dI, HWND hwnd, LANGID langid)
{
    static const struct key2dik
    {
        BYTE key, dik, todo;
    } key2dik_en[] =
    {
        {'Q',DIK_Q}, {'W',DIK_W}, {'E',DIK_E}, {'R',DIK_R}, {'T',DIK_T}, {'Y',DIK_Y},
        {'[',DIK_LBRACKET}, {']',DIK_RBRACKET}, {'.',DIK_PERIOD}
    },
    key2dik_fr[] =
    {
        {'A',DIK_Q}, {'Z',DIK_W}, {'E',DIK_E}, {'R',DIK_R}, {'T',DIK_T}, {'Y',DIK_Y},
        {'^',DIK_LBRACKET}, {'$',DIK_RBRACKET}, {':',DIK_PERIOD}
    },
    key2dik_de[] =
    {
        {'Q',DIK_Q}, {'W',DIK_W}, {'E',DIK_E}, {'R',DIK_R}, {'T',DIK_T}, {'Z',DIK_Y},
        {'\xfc',DIK_LBRACKET,1}, {'+',DIK_RBRACKET}, {'.',DIK_PERIOD}
    },
    key2dik_ja[] =
    {
        {'Q',DIK_Q}, {'W',DIK_W}, {'E',DIK_E}, {'R',DIK_R}, {'T',DIK_T}, {'Y',DIK_Y},
        {'@',DIK_AT}, {']',DIK_RBRACKET}, {'.',DIK_PERIOD}
    };
    static const struct
    {
        LANGID langid;
        const struct key2dik *map;
        DWORD type;
    } expected[] =
    {
        { MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
          key2dik_en, DIDEVTYPEKEYBOARD_PCENH },
        { MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH),
          key2dik_fr, DIDEVTYPEKEYBOARD_PCENH },
        { MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN),
          key2dik_de, DIDEVTYPEKEYBOARD_PCENH },
        { MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN),
          key2dik_ja, DIDEVTYPEKEYBOARD_JAPAN106 }
    };
    const struct key2dik *map = NULL;
    UINT i;
    HRESULT hr;
    IDirectInputDeviceA *device;
    DIDEVCAPS caps;
    HKL hkl, hkl_orig;
    MSG msg;

    for (i = 0; i < ARRAY_SIZE(expected); i++)
    {
        if (expected[i].langid == langid)
        {
            map = expected[i].map;
            break;
        }
    }
    ok(map != NULL, "can't find mapping for langid %04x\n", langid);
    if (!map) return;

    hr = IDirectInput_CreateDevice(dI, &GUID_SysKeyboard, &device, NULL);
    ok(hr == S_OK, "CreateDevice() failed: %08x\n", hr);
    hr = IDirectInputDevice_SetDataFormat(device, &c_dfDIKeyboard);
    ok(hr == S_OK, "SetDataFormat() failed: %08x\n", hr);
    hr = IDirectInputDevice_Acquire(device);
    ok(hr == S_OK, "Acquire() failed: %08x\n", hr);
    caps.dwSize = sizeof( caps );
    hr = IDirectInputDevice_GetCapabilities(device, &caps);
    ok(hr == S_OK, "GetDeviceInstance() failed: %08x\n", hr);
    if (expected[i].type != GET_DIDEVICE_SUBTYPE(caps.dwDevType)) {
        skip("Keyboard type(%u) doesn't match for lang %04x\n",
             GET_DIDEVICE_SUBTYPE(caps.dwDevType), langid);
        goto fail;
    }

    hkl = activate_keyboard_layout(langid, &hkl_orig);
    if (!hkl) goto fail;

    SetFocus(hwnd);
    pump_messages();

    for (i = 0; i < ARRAY_SIZE(key2dik_en); i++)
    {
        BYTE kbd_state[256];
        UINT n;
        WORD vkey, scan;
        INPUT in;

        n = VkKeyScanExW(map[i].key, hkl);
        todo_wine_if(map[i].todo & 1)
        ok(n != 0xffff, "%u: failed to get virtual key value for %c(%02x)\n", i, map[i].key, map[i].key);
        vkey = LOBYTE(n);
        n = MapVirtualKeyExA(vkey, MAPVK_VK_TO_CHAR, hkl) & 0xff;
        todo_wine_if(map[i].todo & 1)
        ok(n == map[i].key, "%u: expected %c(%02x), got %c(%02x)\n", i, map[i].key, map[i].key, n, n);
        scan = MapVirtualKeyExA(vkey, MAPVK_VK_TO_VSC, hkl);
        /* scan codes match the DIK_ codes on US keyboard.
           however, it isn't true for symbols and punctuations in other layouts. */
        if (isalpha(map[i].key) || langid == MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT))
            ok(scan == map[i].dik, "%u: expected %02x, got %02x\n", i, map[i].dik, n);
        else
            todo_wine_if(map[i].todo & 1)
            ok(scan, "%u: fail to get scan code value, expected %02x (vkey=%02x)\n",
               i, map[i].dik, vkey);

        in.type = INPUT_KEYBOARD;
        U(in).ki.wVk = vkey;
        U(in).ki.wScan = scan;
        U(in).ki.dwFlags = 0;
        U(in).ki.dwExtraInfo = 0;
        U(in).ki.time = 0;
        n = SendInput(1, &in, sizeof(in));
        ok(n == 1, "got %u\n", n);

        if (!PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE))
        {
            U(in).ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &in, sizeof(in));
            win_skip("failed to queue keyboard event\n");
            break;
        }
        ok(msg.message == WM_KEYDOWN, "expected WM_KEYDOWN, got %04x\n", msg.message);
        DispatchMessageA(&msg);

        n = MapVirtualKeyExA(msg.wParam, MAPVK_VK_TO_CHAR, hkl);
        trace("keydown wParam: %#08lx (%c) lParam: %#08lx, MapVirtualKey(MAPVK_VK_TO_CHAR) = %c\n",
              msg.wParam, isprint(LOWORD(msg.wParam)) ? LOWORD(msg.wParam) : '?',
              msg.lParam, isprint(n) ? n : '?');

        pump_messages();

        hr = IDirectInputDevice_GetDeviceState(device, sizeof(kbd_state), kbd_state);
        ok(hr == S_OK, "GetDeviceState() failed: %08x\n", hr);

        /* this never happens on real hardware but tesbot VMs seem to have timing issues */
        if (i == 0 && kbd_state[map[0].dik] != 0x80)
        {
            win_skip("dinput failed to handle keyboard event\n");
            break;
        }

        todo_wine_if(map[i].todo)
        ok(kbd_state[map[i].dik] == 0x80, "DI key %#x has state %#x\n", map[i].dik, kbd_state[map[i].dik]);

        U(in).ki.dwFlags = KEYEVENTF_KEYUP;
        n = SendInput(1, &in, sizeof(in));
        ok(n == 1, "got %u\n", n);

        pump_messages();
    }

    ActivateKeyboardLayout(hkl_orig, 0);
    UnloadKeyboardLayout(hkl);
fail:
    IDirectInputDevice_Unacquire(device);
    IUnknown_Release(device);
}

static void test_GetDeviceInfo(IDirectInputA *pDI)
{
    HRESULT hr;
    IDirectInputDeviceA *pKey = NULL;
    DIDEVICEINSTANCEA instA;
    DIDEVICEINSTANCE_DX3A inst3A;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKey, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    instA.dwSize = sizeof(instA);
    hr = IDirectInputDevice_GetDeviceInfo(pKey, &instA);
    ok(SUCCEEDED(hr), "got %08x\n", hr);

    inst3A.dwSize = sizeof(inst3A);
    hr = IDirectInputDevice_GetDeviceInfo(pKey, (DIDEVICEINSTANCEA *)&inst3A);
    ok(SUCCEEDED(hr), "got %08x\n", hr);

    ok(instA.dwSize != inst3A.dwSize, "got %d, %d \n", instA.dwSize, inst3A.dwSize);
    ok(IsEqualGUID(&instA.guidInstance, &inst3A.guidInstance), "got %s, %s\n",
            wine_dbgstr_guid(&instA.guidInstance), wine_dbgstr_guid(&inst3A.guidInstance) );
    ok(IsEqualGUID(&instA.guidProduct, &inst3A.guidProduct), "got %s, %s\n",
            wine_dbgstr_guid(&instA.guidProduct), wine_dbgstr_guid(&inst3A.guidProduct) );
    ok(instA.dwDevType == inst3A.dwDevType, "got %d, %d\n", instA.dwDevType, inst3A.dwDevType);

    IUnknown_Release(pKey);
}

static void keyboard_tests(DWORD version)
{
    HRESULT hr;
    IDirectInputA *pDI = NULL;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    HWND hwnd;
    ULONG ref = 0;

    hr = pDirectInputCreateA(hInstance, version, &pDI, NULL);
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
    SetForegroundWindow( hwnd );

    if (hwnd)
    {
        pump_messages();

        acquire_tests(pDI, hwnd);
        test_set_coop(pDI, hwnd);
        test_get_prop(pDI, hwnd);
        test_capabilities(pDI, hwnd);
        test_GetDeviceInfo(pDI);

        test_dik_codes(pDI, hwnd, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT));
        test_dik_codes(pDI, hwnd, MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH));
        test_dik_codes(pDI, hwnd, MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN));
        test_dik_codes(pDI, hwnd, MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN));
    }

    DestroyWindow(hwnd);
    if (pDI) ref = IUnknown_Release(pDI);
    ok(!ref, "IDirectInput_Release() reference count = %d\n", ref);
}

START_TEST(keyboard)
{
    pDirectInputCreateA = (void *)GetProcAddress(GetModuleHandleA("dinput.dll"), "DirectInputCreateA");

    CoInitialize(NULL);

    keyboard_tests(0x0700);

    CoUninitialize();
}
