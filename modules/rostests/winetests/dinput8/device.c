/*
 * Copyright (c) 2011 Lucas Fialho Zawacki
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

#define DIRECTINPUT_VERSION 0x0800

#define COBJMACROS
#include <windows.h>

#include "wine/test.h"
#include "windef.h"
#include "dinput.h"

struct enum_data {
    IDirectInput8A *pDI;
    DIACTIONFORMATA *lpdiaf;
    IDirectInputDevice8A *keyboard;
    IDirectInputDevice8A *mouse;
    const char* username;
    int ndevices;
};

/* Dummy GUID */
static const GUID ACTION_MAPPING_GUID = { 0x1, 0x2, 0x3, { 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb } };

static const GUID NULL_GUID = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

enum {
    DITEST_AXIS,
    DITEST_BUTTON,
    DITEST_KEYBOARDSPACE,
    DITEST_MOUSEBUTTON0,
    DITEST_YAXIS
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

static void flush_events(void)
{
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        diff = time - GetTickCount();
        min_timeout = 50;
    }
}

static void test_device_input(IDirectInputDevice8A *lpdid, DWORD event_type, DWORD event, DWORD expected)
{
    HRESULT hr;
    DIDEVICEOBJECTDATA obj_data;
    DWORD data_size = 1;
    int i;

    hr = IDirectInputDevice8_Acquire(lpdid);
    ok (SUCCEEDED(hr), "Failed to acquire device hr=%08x\n", hr);

    if (event_type == INPUT_KEYBOARD)
        keybd_event( event, DIK_SPACE, 0, 0);

    if (event_type == INPUT_MOUSE)
        mouse_event( event, 0, 0, 0, 0);

    flush_events();
    IDirectInputDevice8_Poll(lpdid);
    hr = IDirectInputDevice8_GetDeviceData(lpdid, sizeof(obj_data), &obj_data, &data_size, 0);

    if (data_size != 1)
    {
        win_skip("We're not able to inject input into Windows dinput8 with events\n");
        IDirectInputDevice_Unacquire(lpdid);
        return;
    }

    ok (obj_data.uAppData == expected, "Retrieval of action failed uAppData=%lu expected=%d\n", obj_data.uAppData, expected);

    /* Check for buffer overflow */
    for (i = 0; i < 17; i++)
        if (event_type == INPUT_KEYBOARD)
        {
            keybd_event( VK_SPACE, DIK_SPACE, 0, 0);
            keybd_event( VK_SPACE, DIK_SPACE, KEYEVENTF_KEYUP, 0);
        }
        else if (event_type == INPUT_MOUSE)
        {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 1, 1, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 1, 1, 0, 0);
        }

    flush_events();
    IDirectInputDevice8_Poll(lpdid);

    data_size = 1;
    hr = IDirectInputDevice8_GetDeviceData(lpdid, sizeof(obj_data), &obj_data, &data_size, 0);
    ok(hr == DI_BUFFEROVERFLOW, "GetDeviceData() failed: %08x\n", hr);
    data_size = 1;
    hr = IDirectInputDevice8_GetDeviceData(lpdid, sizeof(obj_data), &obj_data, &data_size, 0);
    ok(hr == DI_OK && data_size == 1, "GetDeviceData() failed: %08x cnt:%d\n", hr, data_size);

    IDirectInputDevice_Unacquire(lpdid);
}

static void test_build_action_map(IDirectInputDevice8A *lpdid, DIACTIONFORMATA *lpdiaf,
                                  int action_index, DWORD expected_type, DWORD expected_inst)
{
    HRESULT hr;
    DIACTIONA *actions;
    DWORD instance, type, how;
    GUID assigned_to;
    DIDEVICEINSTANCEA ddi;

    ddi.dwSize = sizeof(ddi);
    IDirectInputDevice_GetDeviceInfo(lpdid, &ddi);

    hr = IDirectInputDevice8_BuildActionMap(lpdid, lpdiaf, NULL, DIDBAM_HWDEFAULTS);
    ok (SUCCEEDED(hr), "BuildActionMap failed hr=%08x\n", hr);

    actions = lpdiaf->rgoAction;
    instance = DIDFT_GETINSTANCE(actions[action_index].dwObjID);
    type = DIDFT_GETTYPE(actions[action_index].dwObjID);
    how = actions[action_index].dwHow;
    assigned_to = actions[action_index].guidInstance;

    ok (how == DIAH_USERCONFIG || how == DIAH_DEFAULT, "Action was not set dwHow=%08x\n", how);
    ok (instance == expected_inst, "Action not mapped correctly instance=%08x expected=%08x\n", instance, expected_inst);
    ok (type == expected_type, "Action type not mapped correctly type=%08x expected=%08x\n", type, expected_type);
    ok (IsEqualGUID(&assigned_to, &ddi.guidInstance), "Action and device GUID do not match action=%d\n", action_index);
}

static BOOL CALLBACK enumeration_callback(const DIDEVICEINSTANCEA *lpddi, IDirectInputDevice8A *lpdid,
                                          DWORD dwFlags, DWORD dwRemaining, LPVOID pvRef)
{
    HRESULT hr;
    DIPROPDWORD dp;
    DIPROPRANGE dpr;
    DIPROPSTRING dps;
    WCHAR usernameW[MAX_PATH];
    DWORD username_size = MAX_PATH;
    struct enum_data *data = pvRef;
    DWORD cnt;
    DIDEVICEOBJECTDATA buffer[5];
    IDirectInputDevice8A *lpdid2;

    if (!data) return DIENUM_CONTINUE;

    data->ndevices++;

    /* Convert username to WCHAR */
    if (data->username != NULL)
    {
        username_size = MultiByteToWideChar(CP_ACP, 0, data->username, -1, usernameW, 0);
        MultiByteToWideChar(CP_ACP, 0, data->username, -1, usernameW, username_size);
    }
    else
        GetUserNameW(usernameW, &username_size);

    /* collect the mouse and keyboard */
    if (IsEqualGUID(&lpddi->guidInstance, &GUID_SysKeyboard))
    {
        IDirectInputDevice_AddRef(lpdid);
        data->keyboard = lpdid;

        ok (dwFlags & DIEDBS_MAPPEDPRI1, "Keyboard should be mapped as pri1 dwFlags=%08x\n", dwFlags);
    }

    if (IsEqualGUID(&lpddi->guidInstance, &GUID_SysMouse))
    {
        IDirectInputDevice_AddRef(lpdid);
        data->mouse = lpdid;

        ok (dwFlags & DIEDBS_MAPPEDPRI1, "Mouse should be mapped as pri1 dwFlags=%08x\n", dwFlags);
    }

    /* Creating second device object to check if it has the same username */
    hr = IDirectInput_CreateDevice(data->pDI, &lpddi->guidInstance, &lpdid2, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);

    /* Building and setting an action map */
    /* It should not use any pre-stored mappings so we use DIDBAM_HWDEFAULTS */
    hr = IDirectInputDevice8_BuildActionMap(lpdid, data->lpdiaf, NULL, DIDBAM_HWDEFAULTS);
    ok (SUCCEEDED(hr), "BuildActionMap failed hr=%08x\n", hr);

    /* Device has no data format and thus can't be acquired */
    hr = IDirectInputDevice8_Acquire(lpdid);
    ok (hr == DIERR_INVALIDPARAM, "Device was acquired before SetActionMap hr=%08x\n", hr);

    hr = IDirectInputDevice8_SetActionMap(lpdid, data->lpdiaf, data->username, 0);
    ok (SUCCEEDED(hr), "SetActionMap failed hr=%08x\n", hr);

    /* Some joysticks may have no suitable actions and thus should not be tested */
    if (hr == DI_NOEFFECT) return DIENUM_CONTINUE;

    /* Test username after SetActionMap */
    dps.diph.dwSize = sizeof(dps);
    dps.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dps.diph.dwObj = 0;
    dps.diph.dwHow  = DIPH_DEVICE;
    dps.wsz[0] = '\0';

    hr = IDirectInputDevice_GetProperty(lpdid, DIPROP_USERNAME, &dps.diph);
    ok (SUCCEEDED(hr), "GetProperty failed hr=%08x\n", hr);
    ok (!lstrcmpW(usernameW, dps.wsz), "Username not set correctly expected=%s, got=%s\n", wine_dbgstr_w(usernameW), wine_dbgstr_w(dps.wsz));

    dps.wsz[0] = '\0';
    hr = IDirectInputDevice_GetProperty(lpdid2, DIPROP_USERNAME, &dps.diph);
    ok (SUCCEEDED(hr), "GetProperty failed hr=%08x\n", hr);
    ok (!lstrcmpW(usernameW, dps.wsz), "Username not set correctly expected=%s, got=%s\n", wine_dbgstr_w(usernameW), wine_dbgstr_w(dps.wsz));

    /* Test buffer size */
    memset(&dp, 0, sizeof(dp));
    dp.diph.dwSize = sizeof(dp);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwHow  = DIPH_DEVICE;

    hr = IDirectInputDevice_GetProperty(lpdid, DIPROP_BUFFERSIZE, &dp.diph);
    ok (SUCCEEDED(hr), "GetProperty failed hr=%08x\n", hr);
    ok (dp.dwData == data->lpdiaf->dwBufferSize, "SetActionMap must set the buffer, buffersize=%d\n", dp.dwData);

    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(lpdid, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DIERR_NOTACQUIRED, "GetDeviceData() failed hr=%08x\n", hr);

    /* Test axis range */
    memset(&dpr, 0, sizeof(dpr));
    dpr.diph.dwSize = sizeof(dpr);
    dpr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dpr.diph.dwHow  = DIPH_DEVICE;

    hr = IDirectInputDevice_GetProperty(lpdid, DIPROP_RANGE, &dpr.diph);
    /* Only test if device supports the range property */
    if (SUCCEEDED(hr))
    {
        ok (dpr.lMin == data->lpdiaf->lAxisMin, "SetActionMap must set the min axis range expected=%d got=%d\n", data->lpdiaf->lAxisMin, dpr.lMin);
        ok (dpr.lMax == data->lpdiaf->lAxisMax, "SetActionMap must set the max axis range expected=%d got=%d\n", data->lpdiaf->lAxisMax, dpr.lMax);
    }

    /* SetActionMap has set the data format so now it should work */
    hr = IDirectInputDevice8_Acquire(lpdid);
    ok (SUCCEEDED(hr), "Acquire failed hr=%08x\n", hr);

    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(lpdid, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed hr=%08x\n", hr);

    /* SetActionMap should not work on an acquired device */
    hr = IDirectInputDevice8_SetActionMap(lpdid, data->lpdiaf, NULL, 0);
    ok (hr == DIERR_ACQUIRED, "SetActionMap succeeded with an acquired device hr=%08x\n", hr);

    IDirectInputDevice_Release(lpdid2);

    return DIENUM_CONTINUE;
}

static void test_action_mapping(void)
{
    HRESULT hr;
    HINSTANCE hinst = GetModuleHandleA(NULL);
    IDirectInput8A *pDI = NULL;
    DIACTIONFORMATA af;
    DIPROPSTRING dps;
    struct enum_data data =  {pDI, &af, NULL, NULL, NULL, 0};
    HWND hwnd;

    hr = CoCreateInstance(&CLSID_DirectInput8, 0, CLSCTX_INPROC_SERVER, &IID_IDirectInput8A, (LPVOID*)&pDI);
    if (hr == DIERR_OLDDIRECTINPUTVERSION ||
        hr == DIERR_BETADIRECTINPUTVERSION ||
        hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("ActionMapping requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8 Create failed: hr=%08x\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInput8_Initialize(pDI,hinst, DIRECTINPUT_VERSION);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_BETADIRECTINPUTVERSION)
    {
        win_skip("ActionMapping requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8 Initialize failed: hr=%08x\n", hr);
    if (FAILED(hr)) return;

    memset (&af, 0, sizeof(af));
    af.dwSize = sizeof(af);
    af.dwActionSize = sizeof(DIACTIONA);
    af.dwDataSize = 4 * ARRAY_SIZE(actionMapping);
    af.dwNumActions = ARRAY_SIZE(actionMapping);
    af.rgoAction = actionMapping;
    af.guidActionMap = ACTION_MAPPING_GUID;
    af.dwGenre = 0x01000000; /* DIVIRTUAL_DRIVING_RACE */
    af.dwBufferSize = 32;

    /* This enumeration builds and sets the action map for all devices */
    data.pDI = pDI;
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, 0, &af, enumeration_callback, &data, DIEDBSFL_ATTACHEDONLY);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed: hr=%08x\n", hr);

    if (data.keyboard)
        IDirectInputDevice_Release(data.keyboard);

    if (data.mouse)
        IDirectInputDevice_Release(data.mouse);

    /* Repeat tests with a non NULL user */
    data.username = "Ninja Brian";
    hr = IDirectInput8_EnumDevicesBySemantics(pDI, NULL, &af, enumeration_callback, &data, DIEDBSFL_ATTACHEDONLY);
    ok (SUCCEEDED(hr), "EnumDevicesBySemantics failed: hr=%08x\n", hr);

    hwnd = CreateWindowExA(WS_EX_TOPMOST, "static", "dinput",
            WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create window\n");
    SetCursorPos(50, 50);

    if (data.keyboard != NULL)
    {
        /* Test keyboard BuildActionMap */
        test_build_action_map(data.keyboard, data.lpdiaf, DITEST_KEYBOARDSPACE, DIDFT_PSHBUTTON, DIK_SPACE);
        /* Test keyboard input */
        test_device_input(data.keyboard, INPUT_KEYBOARD, VK_SPACE, 2);

        /* Test BuildActionMap with no suitable actions for a device */
        IDirectInputDevice_Unacquire(data.keyboard);
        af.dwDataSize = 4 * DITEST_KEYBOARDSPACE;
        af.dwNumActions = DITEST_KEYBOARDSPACE;

        hr = IDirectInputDevice8_BuildActionMap(data.keyboard, data.lpdiaf, NULL, DIDBAM_HWDEFAULTS);
        ok (hr == DI_NOEFFECT, "BuildActionMap should have no effect with no actions hr=%08x\n", hr);

        hr = IDirectInputDevice8_SetActionMap(data.keyboard, data.lpdiaf, NULL, 0);
        ok (hr == DI_NOEFFECT, "SetActionMap should have no effect with no actions to map hr=%08x\n", hr);

        /* Test that after changing actionformat SetActionMap has effect and that second
         * SetActionMap call with same empty actionformat has no effect */
        af.dwDataSize = 4 * 1;
        af.dwNumActions = 1;

        hr = IDirectInputDevice8_SetActionMap(data.keyboard, data.lpdiaf, NULL, 0);
        ok (hr != DI_NOEFFECT, "SetActionMap should have effect as actionformat has changed hr=%08x\n", hr);

        hr = IDirectInputDevice8_SetActionMap(data.keyboard, data.lpdiaf, NULL, 0);
        ok (hr == DI_NOEFFECT, "SetActionMap should have no effect with no actions to map hr=%08x\n", hr);

        af.dwDataSize = 4 * ARRAY_SIZE(actionMapping);
        af.dwNumActions = ARRAY_SIZE(actionMapping);

        /* test DIDSAM_NOUSER */
        dps.diph.dwSize = sizeof(dps);
        dps.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dps.diph.dwObj = 0;
        dps.diph.dwHow = DIPH_DEVICE;
        dps.wsz[0] = '\0';

        hr = IDirectInputDevice_GetProperty(data.keyboard, DIPROP_USERNAME, &dps.diph);
        ok (SUCCEEDED(hr), "GetProperty failed hr=%08x\n", hr);
        ok (dps.wsz[0] != 0, "Expected any username, got=%s\n", wine_dbgstr_w(dps.wsz));

        hr = IDirectInputDevice8_SetActionMap(data.keyboard, data.lpdiaf, NULL, DIDSAM_NOUSER);
        ok (SUCCEEDED(hr), "SetActionMap failed hr=%08x\n", hr);

        dps.diph.dwSize = sizeof(dps);
        dps.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dps.diph.dwObj = 0;
        dps.diph.dwHow = DIPH_DEVICE;
        dps.wsz[0] = '\0';

        hr = IDirectInputDevice_GetProperty(data.keyboard, DIPROP_USERNAME, &dps.diph);
        ok (SUCCEEDED(hr), "GetProperty failed hr=%08x\n", hr);
        ok (dps.wsz[0] == 0, "Expected empty username, got=%s\n", wine_dbgstr_w(dps.wsz));

        IDirectInputDevice_Release(data.keyboard);
    }

    if (data.mouse != NULL)
    {
        /* Test mouse BuildActionMap */
        test_build_action_map(data.mouse, data.lpdiaf, DITEST_MOUSEBUTTON0, DIDFT_PSHBUTTON, 0x03);
        test_build_action_map(data.mouse, data.lpdiaf, DITEST_YAXIS, DIDFT_RELAXIS, 0x01);

        test_device_input(data.mouse, INPUT_MOUSE, MOUSEEVENTF_LEFTDOWN, 3);

        IDirectInputDevice_Release(data.mouse);
    }

    DestroyWindow(hwnd);
    IDirectInput_Release(pDI);
}

static void test_save_settings(void)
{
    HRESULT hr;
    HINSTANCE hinst = GetModuleHandleA(NULL);
    IDirectInput8A *pDI = NULL;
    DIACTIONFORMATA af;
    IDirectInputDevice8A *pKey;

    static const GUID mapping_guid = { 0xcafecafe, 0x2, 0x3, { 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb } };
    static const GUID other_guid = { 0xcafe, 0xcafe, 0x3, { 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb } };

    static DIACTIONA actions[] = {
        { 0, DIKEYBOARD_A , 0, { "Blam" } },
        { 1, DIKEYBOARD_B , 0, { "Kapow"} }
    };
    static const DWORD results[] = {
        DIDFT_MAKEINSTANCE(DIK_A) | DIDFT_PSHBUTTON,
        DIDFT_MAKEINSTANCE(DIK_B) | DIDFT_PSHBUTTON
    };
    static const DWORD other_results[] = {
        DIDFT_MAKEINSTANCE(DIK_C) | DIDFT_PSHBUTTON,
        DIDFT_MAKEINSTANCE(DIK_D) | DIDFT_PSHBUTTON
    };

    hr = CoCreateInstance(&CLSID_DirectInput8, 0, CLSCTX_INPROC_SERVER, &IID_IDirectInput8A, (LPVOID*)&pDI);
    if (hr == DIERR_OLDDIRECTINPUTVERSION ||
        hr == DIERR_BETADIRECTINPUTVERSION ||
        hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("ActionMapping requires dinput8\n");
        return;
    }
    ok (SUCCEEDED(hr), "DirectInput8 Create failed: hr=%08x\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInput8_Initialize(pDI,hinst, DIRECTINPUT_VERSION);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_BETADIRECTINPUTVERSION)
    {
        win_skip("ActionMapping requires dinput8\n");
        return;
    }
    ok (SUCCEEDED(hr), "DirectInput8 Initialize failed: hr=%08x\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKey, NULL);
    ok (SUCCEEDED(hr), "IDirectInput_Create device failed hr: 0x%08x\n", hr);
    if (FAILED(hr)) return;

    memset (&af, 0, sizeof(af));
    af.dwSize = sizeof(af);
    af.dwActionSize = sizeof(DIACTIONA);
    af.dwDataSize = 4 * ARRAY_SIZE(actions);
    af.dwNumActions = ARRAY_SIZE(actions);
    af.rgoAction = actions;
    af.guidActionMap = mapping_guid;
    af.dwGenre = 0x01000000; /* DIVIRTUAL_DRIVING_RACE */
    af.dwBufferSize = 32;

    /* Easy case. Ask for default mapping, save, ask for previous map and read it back */
    hr = IDirectInputDevice8_BuildActionMap(pKey, &af, NULL, DIDBAM_HWDEFAULTS);
    ok (SUCCEEDED(hr), "BuildActionMap failed hr=%08x\n", hr);
    ok (results[0] == af.rgoAction[0].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", results[0], af.rgoAction[0].dwObjID);

    ok (results[1] == af.rgoAction[1].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", results[1], af.rgoAction[1].dwObjID);

    hr = IDirectInputDevice8_SetActionMap(pKey, &af, NULL, DIDSAM_FORCESAVE);
    ok (SUCCEEDED(hr), "SetActionMap failed hr=%08x\n", hr);

    if (hr == DI_SETTINGSNOTSAVED)
    {
        skip ("Can't test saving settings if SetActionMap returns DI_SETTINGSNOTSAVED\n");
        return;
    }

    af.rgoAction[0].dwObjID = 0;
    af.rgoAction[1].dwObjID = 0;
    memset(&af.rgoAction[0].guidInstance, 0, sizeof(GUID));
    memset(&af.rgoAction[1].guidInstance, 0, sizeof(GUID));

    hr = IDirectInputDevice8_BuildActionMap(pKey, &af, NULL, 0);
    ok (SUCCEEDED(hr), "BuildActionMap failed hr=%08x\n", hr);

    ok (results[0] == af.rgoAction[0].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", results[0], af.rgoAction[0].dwObjID);
    ok (IsEqualGUID(&GUID_SysKeyboard, &af.rgoAction[0].guidInstance), "Action should be mapped to keyboard\n");

    ok (results[1] == af.rgoAction[1].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", results[1], af.rgoAction[1].dwObjID);
    ok (IsEqualGUID(&GUID_SysKeyboard, &af.rgoAction[1].guidInstance), "Action should be mapped to keyboard\n");

    /* Test that a different action map with no pre-stored settings, in spite of the flags,
       does not try to load mappings and instead applies the default mapping */
    af.guidActionMap = other_guid;

    af.rgoAction[0].dwObjID = 0;
    af.rgoAction[1].dwObjID = 0;
    memset(&af.rgoAction[0].guidInstance, 0, sizeof(GUID));
    memset(&af.rgoAction[1].guidInstance, 0, sizeof(GUID));

    hr = IDirectInputDevice8_BuildActionMap(pKey, &af, NULL, 0);
    ok (SUCCEEDED(hr), "BuildActionMap failed hr=%08x\n", hr);

    ok (results[0] == af.rgoAction[0].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", results[0], af.rgoAction[0].dwObjID);
    ok (IsEqualGUID(&GUID_SysKeyboard, &af.rgoAction[0].guidInstance), "Action should be mapped to keyboard\n");

    ok (results[1] == af.rgoAction[1].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", results[1], af.rgoAction[1].dwObjID);
    ok (IsEqualGUID(&GUID_SysKeyboard, &af.rgoAction[1].guidInstance), "Action should be mapped to keyboard\n");

    af.guidActionMap = mapping_guid;
    /* Hard case. Customized mapping, save, ask for previous map and read it back */
    af.rgoAction[0].dwObjID = other_results[0];
    af.rgoAction[0].dwHow = DIAH_USERCONFIG;
    af.rgoAction[0].guidInstance = GUID_SysKeyboard;
    af.rgoAction[1].dwObjID = other_results[1];
    af.rgoAction[1].dwHow = DIAH_USERCONFIG;
    af.rgoAction[1].guidInstance = GUID_SysKeyboard;

    hr = IDirectInputDevice8_SetActionMap(pKey, &af, NULL, DIDSAM_FORCESAVE);
    ok (SUCCEEDED(hr), "SetActionMap failed hr=%08x\n", hr);

    if (hr == DI_SETTINGSNOTSAVED)
    {
        skip ("Can't test saving settings if SetActionMap returns DI_SETTINGSNOTSAVED\n");
        return;
    }

    af.rgoAction[0].dwObjID = 0;
    af.rgoAction[1].dwObjID = 0;
    memset(&af.rgoAction[0].guidInstance, 0, sizeof(GUID));
    memset(&af.rgoAction[1].guidInstance, 0, sizeof(GUID));

    hr = IDirectInputDevice8_BuildActionMap(pKey, &af, NULL, 0);
    ok (SUCCEEDED(hr), "BuildActionMap failed hr=%08x\n", hr);

    ok (other_results[0] == af.rgoAction[0].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", other_results[0], af.rgoAction[0].dwObjID);
    ok (IsEqualGUID(&GUID_SysKeyboard, &af.rgoAction[0].guidInstance), "Action should be mapped to keyboard\n");

    ok (other_results[1] == af.rgoAction[1].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", other_results[1], af.rgoAction[1].dwObjID);
    ok (IsEqualGUID(&GUID_SysKeyboard, &af.rgoAction[1].guidInstance), "Action should be mapped to keyboard\n");

    /* Save and load empty mapping */
    af.rgoAction[0].dwObjID = 0;
    af.rgoAction[0].dwHow = 0;
    memset(&af.rgoAction[0].guidInstance, 0, sizeof(GUID));
    af.rgoAction[1].dwObjID = 0;
    af.rgoAction[1].dwHow = 0;
    memset(&af.rgoAction[1].guidInstance, 0, sizeof(GUID));

    hr = IDirectInputDevice8_SetActionMap(pKey, &af, NULL, DIDSAM_FORCESAVE);
    ok (SUCCEEDED(hr), "SetActionMap failed hr=%08x\n", hr);

    if (hr == DI_SETTINGSNOTSAVED)
    {
        skip ("Can't test saving settings if SetActionMap returns DI_SETTINGSNOTSAVED\n");
        return;
    }

    af.rgoAction[0].dwObjID = other_results[0];
    af.rgoAction[0].dwHow = DIAH_USERCONFIG;
    af.rgoAction[0].guidInstance = GUID_SysKeyboard;
    af.rgoAction[1].dwObjID = other_results[1];
    af.rgoAction[1].dwHow = DIAH_USERCONFIG;
    af.rgoAction[1].guidInstance = GUID_SysKeyboard;

    hr = IDirectInputDevice8_BuildActionMap(pKey, &af, NULL, 0);
    ok (SUCCEEDED(hr), "BuildActionMap failed hr=%08x\n", hr);

    ok (other_results[0] == af.rgoAction[0].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", other_results[0], af.rgoAction[0].dwObjID);
    ok (af.rgoAction[0].dwHow == DIAH_UNMAPPED, "dwHow should have been DIAH_UNMAPPED\n");
    ok (IsEqualGUID(&NULL_GUID, &af.rgoAction[0].guidInstance), "Action should not be mapped\n");

    ok (other_results[1] == af.rgoAction[1].dwObjID,
        "Mapped incorrectly expected: 0x%08x got: 0x%08x\n", other_results[1], af.rgoAction[1].dwObjID);
    ok (af.rgoAction[1].dwHow == DIAH_UNMAPPED, "dwHow should have been DIAH_UNMAPPED\n");
    ok (IsEqualGUID(&NULL_GUID, &af.rgoAction[1].guidInstance), "Action should not be mapped\n");

    IDirectInputDevice_Release(pKey);
    IDirectInput_Release(pDI);
}

static void test_mouse_keyboard(void)
{
    HRESULT hr;
    HWND hwnd, di_hwnd = INVALID_HANDLE_VALUE;
    IDirectInput8A *di = NULL;
    IDirectInputDevice8A *di_mouse, *di_keyboard;
    UINT raw_devices_count;
    RAWINPUTDEVICE raw_devices[3];

    hwnd = CreateWindowExA(WS_EX_TOPMOST, "static", "dinput", WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowExA failed\n");

    hr = CoCreateInstance(&CLSID_DirectInput8, 0, CLSCTX_INPROC_SERVER, &IID_IDirectInput8A, (LPVOID*)&di);
    if (hr == DIERR_OLDDIRECTINPUTVERSION ||
        hr == DIERR_BETADIRECTINPUTVERSION ||
        hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("test_mouse_keyboard requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInput8Create failed: %08x\n", hr);

    hr = IDirectInput8_Initialize(di, GetModuleHandleA(NULL), DIRECTINPUT_VERSION);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_BETADIRECTINPUTVERSION)
    {
        win_skip("test_mouse_keyboard requires dinput8\n");
        return;
    }
    ok(SUCCEEDED(hr), "IDirectInput8_Initialize failed: %08x\n", hr);

    hr = IDirectInput8_CreateDevice(di, &GUID_SysMouse, &di_mouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput8_CreateDevice failed: %08x\n", hr);
    hr = IDirectInputDevice8_SetDataFormat(di_mouse, &c_dfDIMouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetDataFormat failed: %08x\n", hr);

    hr = IDirectInput8_CreateDevice(di, &GUID_SysKeyboard, &di_keyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput8_CreateDevice failed: %08x\n", hr);
    hr = IDirectInputDevice8_SetDataFormat(di_keyboard, &c_dfDIKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_SetDataFormat failed: %08x\n", hr);

    raw_devices_count = ARRAY_SIZE(raw_devices);
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(raw_devices_count == 0, "Unexpected raw devices registered: %d\n", raw_devices_count);

    hr = IDirectInputDevice8_Acquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %08x\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    memset(raw_devices, 0, sizeof(raw_devices));
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(hr == 1, "GetRegisteredRawInputDevices returned %d, raw_devices_count: %d\n", hr, raw_devices_count);
    todo_wine
    ok(raw_devices[0].usUsagePage == 1, "Unexpected raw device usage page: %x\n", raw_devices[0].usUsagePage);
    todo_wine
    ok(raw_devices[0].usUsage == 6, "Unexpected raw device usage: %x\n", raw_devices[0].usUsage);
    todo_wine
    ok(raw_devices[0].dwFlags == RIDEV_INPUTSINK, "Unexpected raw device flags: %x\n", raw_devices[0].dwFlags);
    todo_wine
    ok(raw_devices[0].hwndTarget != NULL, "Unexpected raw device target: %p\n", raw_devices[0].hwndTarget);
    hr = IDirectInputDevice8_Unacquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %08x\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(raw_devices_count == 0, "Unexpected raw devices registered: %d\n", raw_devices_count);

    if (raw_devices[0].hwndTarget != NULL)
    {
        WCHAR di_hwnd_class[] = {'D','I','E','m','W','i','n',0};
        WCHAR str[16];
        int i;

        di_hwnd = raw_devices[0].hwndTarget;
        i = GetClassNameW(di_hwnd, str, ARRAY_SIZE(str));
        ok(i == lstrlenW(di_hwnd_class), "GetClassName returned incorrect length\n");
        ok(!lstrcmpW(di_hwnd_class, str), "GetClassName returned incorrect name for this window's class\n");

        i = GetWindowTextW(di_hwnd, str, ARRAY_SIZE(str));
        ok(i == lstrlenW(di_hwnd_class), "GetClassName returned incorrect length\n");
        ok(!lstrcmpW(di_hwnd_class, str), "GetClassName returned incorrect name for this window's class\n");
    }

    hr = IDirectInputDevice8_Acquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %08x\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    memset(raw_devices, 0, sizeof(raw_devices));
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(hr == 1, "GetRegisteredRawInputDevices returned %d, raw_devices_count: %d\n", hr, raw_devices_count);
    todo_wine
    ok(raw_devices[0].usUsagePage == 1, "Unexpected raw device usage page: %x\n", raw_devices[0].usUsagePage);
    todo_wine
    ok(raw_devices[0].usUsage == 2, "Unexpected raw device usage: %x\n", raw_devices[0].usUsage);
    todo_wine
    ok(raw_devices[0].dwFlags == RIDEV_INPUTSINK, "Unexpected raw device flags: %x\n", raw_devices[0].dwFlags);
    todo_wine
    ok(raw_devices[0].hwndTarget == di_hwnd, "Unexpected raw device target: %p\n", raw_devices[0].hwndTarget);
    hr = IDirectInputDevice8_Unacquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %08x\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(raw_devices_count == 0, "Unexpected raw devices registered: %d\n", raw_devices_count);

    /* expect dinput8 to take over any activated raw input devices */
    raw_devices[0].usUsagePage = 0x01;
    raw_devices[0].usUsage = 0x05;
    raw_devices[0].dwFlags = 0;
    raw_devices[0].hwndTarget = hwnd;
    raw_devices[1].usUsagePage = 0x01;
    raw_devices[1].usUsage = 0x06;
    raw_devices[1].dwFlags = 0;
    raw_devices[1].hwndTarget = hwnd;
    raw_devices[2].usUsagePage = 0x01;
    raw_devices[2].usUsage = 0x02;
    raw_devices[2].dwFlags = 0;
    raw_devices[2].hwndTarget = hwnd;
    raw_devices_count = ARRAY_SIZE(raw_devices);
    hr = RegisterRawInputDevices(raw_devices, raw_devices_count, sizeof(RAWINPUTDEVICE));
    ok(hr == TRUE, "RegisterRawInputDevices failed\n");

    hr = IDirectInputDevice8_Acquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %08x\n", hr);
    hr = IDirectInputDevice8_Acquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %08x\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    memset(raw_devices, 0, sizeof(raw_devices));
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(hr == 3, "GetRegisteredRawInputDevices returned %d, raw_devices_count: %d\n", hr, raw_devices_count);
    todo_wine
    ok(raw_devices[0].usUsagePage == 1, "Unexpected raw device usage page: %x\n", raw_devices[0].usUsagePage);
    todo_wine
    ok(raw_devices[0].usUsage == 2, "Unexpected raw device usage: %x\n", raw_devices[0].usUsage);
    todo_wine
    ok(raw_devices[0].dwFlags == RIDEV_INPUTSINK, "Unexpected raw device flags: %x\n", raw_devices[0].dwFlags);
    todo_wine
    ok(raw_devices[0].hwndTarget == di_hwnd, "Unexpected raw device target: %p\n", raw_devices[0].hwndTarget);
    todo_wine
    ok(raw_devices[1].usUsagePage == 1, "Unexpected raw device usage page: %x\n", raw_devices[1].usUsagePage);
    todo_wine
    ok(raw_devices[1].usUsage == 5, "Unexpected raw device usage: %x\n", raw_devices[1].usUsage);
    ok(raw_devices[1].dwFlags == 0, "Unexpected raw device flags: %x\n", raw_devices[1].dwFlags);
    todo_wine
    ok(raw_devices[1].hwndTarget == hwnd, "Unexpected raw device target: %p\n", raw_devices[1].hwndTarget);
    todo_wine
    ok(raw_devices[2].usUsagePage == 1, "Unexpected raw device usage page: %x\n", raw_devices[1].usUsagePage);
    todo_wine
    ok(raw_devices[2].usUsage == 6, "Unexpected raw device usage: %x\n", raw_devices[1].usUsage);
    todo_wine
    ok(raw_devices[2].dwFlags == RIDEV_INPUTSINK, "Unexpected raw device flags: %x\n", raw_devices[1].dwFlags);
    todo_wine
    ok(raw_devices[2].hwndTarget == di_hwnd, "Unexpected raw device target: %p\n", raw_devices[1].hwndTarget);
    hr = IDirectInputDevice8_Unacquire(di_keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %08x\n", hr);
    hr = IDirectInputDevice8_Unacquire(di_mouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice8_Acquire failed: %08x\n", hr);
    raw_devices_count = ARRAY_SIZE(raw_devices);
    GetRegisteredRawInputDevices(NULL, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(raw_devices_count == 1, "Unexpected raw devices registered: %d\n", raw_devices_count);

    raw_devices_count = ARRAY_SIZE(raw_devices);
    hr = GetRegisteredRawInputDevices(raw_devices, &raw_devices_count, sizeof(RAWINPUTDEVICE));
    todo_wine
    ok(hr == 1, "GetRegisteredRawInputDevices returned %d, raw_devices_count: %d\n", hr, raw_devices_count);
    todo_wine
    ok(raw_devices[0].usUsagePage == 1, "Unexpected raw device usage page: %x\n", raw_devices[0].usUsagePage);
    todo_wine
    ok(raw_devices[0].usUsage == 5, "Unexpected raw device usage: %x\n", raw_devices[0].usUsage);
    ok(raw_devices[0].dwFlags == 0, "Unexpected raw device flags: %x\n", raw_devices[0].dwFlags);
    todo_wine
    ok(raw_devices[0].hwndTarget == hwnd, "Unexpected raw device target: %p\n", raw_devices[0].hwndTarget);

    IDirectInputDevice8_Release(di_mouse);
    IDirectInputDevice8_Release(di_keyboard);
    IDirectInput8_Release(di);

    DestroyWindow(hwnd);
}

START_TEST(device)
{
    CoInitialize(NULL);

    test_action_mapping();
    test_save_settings();
    test_mouse_keyboard();

    CoUninitialize();
}
