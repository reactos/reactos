/*
 * Copyright (c) 2005 Robert Reif
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

#include <math.h>
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "wingdi.h"
#include "dinput.h"

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
    IDirectInputDeviceA *pMouse = NULL;
    int i;
    HWND child;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pMouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pMouse, NULL, i);
        ok(hr == SetCoop_null_window[i], "SetCooperativeLevel(NULL, %d): %08x\n", i, hr);
    }
    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pMouse, hwnd, i);
        ok(hr == SetCoop_real_window[i], "SetCooperativeLevel(hwnd, %d): %08x\n", i, hr);
    }

    child = CreateWindowA("static", "Title", WS_CHILD | WS_VISIBLE, 10, 10, 50, 50, hwnd, NULL,
                          NULL, NULL);
    ok(child != NULL, "err: %d\n", GetLastError());

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pMouse, child, i);
        ok(hr == SetCoop_child_window[i], "SetCooperativeLevel(child, %d): %08x\n", i, hr);
    }

    DestroyWindow(child);
    if (pMouse) IUnknown_Release(pMouse);
}

static void test_acquire(IDirectInputA *pDI, HWND hwnd)
{
    HRESULT hr;
    IDirectInputDeviceA *pMouse = NULL;
    DIMOUSESTATE m_state;
    HWND hwnd2;
    DIPROPDWORD di_op;
    DIDEVICEOBJECTDATA mouse_state;
    DWORD cnt;
    int i;

    if (! SetForegroundWindow(hwnd))
    {
        skip("Not running as foreground app, skipping acquire tests\n");
        return;
    }

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pMouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInputDevice_SetCooperativeLevel(pMouse, hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    ok(hr == S_OK, "SetCooperativeLevel: %08x\n", hr);

    memset(&di_op, 0, sizeof(di_op));
    di_op.dwData = 5;
    di_op.diph.dwHow = DIPH_DEVICE;
    di_op.diph.dwSize = sizeof(DIPROPDWORD);
    di_op.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    hr = IDirectInputDevice_SetProperty(pMouse, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&di_op);
    ok(hr == S_OK, "SetProperty() failed: %08x\n", hr);

    hr = IDirectInputDevice_SetDataFormat(pMouse, &c_dfDIMouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %08x\n", hr);
    hr = IDirectInputDevice_Unacquire(pMouse);
    ok(hr == S_FALSE, "IDirectInputDevice_Unacquire() should have failed: %08x\n", hr);
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %08x\n", hr);
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_FALSE, "IDirectInputDevice_Acquire() should have failed: %08x\n", hr);

    /* Foreground coop level requires window to have focus */
    /* Create a temporary window, this should make dinput
     * lose mouse input */
    hwnd2 = CreateWindowA("static", "Temporary", WS_VISIBLE, 10, 210, 200, 200, NULL, NULL, NULL,
                          NULL);
    ok(hwnd2 != NULL, "CreateWindowA failed with %u\n", GetLastError());

    hr = IDirectInputDevice_GetDeviceState(pMouse, sizeof(m_state), &m_state);
    ok(hr == DIERR_NOTACQUIRED, "GetDeviceState() should have failed: %08x\n", hr);

    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == DIERR_OTHERAPPHASPRIO, "Acquire() should have failed: %08x\n", hr);

    SetActiveWindow( hwnd );
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_OK, "Acquire() failed: %08x\n", hr);

if (!winetest_interactive)
    skip("ROSTESTS-176/CORE-9710: Skipping randomly failing tests\n");
else {

    mouse_event(MOUSEEVENTF_MOVE, 10, 10, 0, 0);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == S_OK && cnt > 0, "GetDeviceData() failed: %08x cnt:%d\n", hr, cnt);

    mouse_event(MOUSEEVENTF_MOVE, 10, 10, 0, 0);
    hr = IDirectInputDevice_Unacquire(pMouse);
    ok(hr == S_OK, "Failed: %08x\n", hr);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == S_OK && cnt > 0, "GetDeviceData() failed: %08x cnt:%d\n", hr, cnt);

    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_OK, "Failed: %08x\n", hr);
    mouse_event(MOUSEEVENTF_MOVE, 10, 10, 0, 0);
    hr = IDirectInputDevice_Unacquire(pMouse);
    ok(hr == S_OK, "Failed: %08x\n", hr);

    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_OK, "Failed: %08x\n", hr);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == S_OK && cnt > 0, "GetDeviceData() failed: %08x cnt:%d\n", hr, cnt);

    /* Check for buffer overflow */
    for (i = 0; i < 6; i++)
        mouse_event(MOUSEEVENTF_MOVE, 10 + i, 10 + i, 0, 0);

    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed: %08x cnt:%d\n", hr, cnt);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == DI_OK && cnt == 1, "GetDeviceData() failed: %08x cnt:%d\n", hr, cnt);

    /* Check for granularity property using BYOFFSET */
    memset(&di_op, 0, sizeof(di_op));
    di_op.diph.dwHow = DIPH_BYOFFSET;
    di_op.diph.dwObj = DIMOFS_Y;
    di_op.diph.dwSize = sizeof(DIPROPDWORD);
    di_op.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    hr = IDirectInputDevice_GetProperty(pMouse, DIPROP_GRANULARITY, &di_op.diph);
    /* Granularity of Y axis should be 1! */
    ok(hr == S_OK && di_op.dwData == 1, "GetProperty(): %08x, dwData: %i but should be 1.\n", hr, di_op.dwData);

    /* Check for granularity property using BYID */
    memset(&di_op, 0, sizeof(di_op));
    di_op.diph.dwHow = DIPH_BYID;
    /* WINE_MOUSE_Y_AXIS_INSTANCE := 1 */
    di_op.diph.dwObj = (DIDFT_MAKEINSTANCE(1) | DIDFT_RELAXIS);
    di_op.diph.dwSize = sizeof(DIPROPDWORD);
    di_op.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    hr = IDirectInputDevice_GetProperty(pMouse, DIPROP_GRANULARITY, &di_op.diph);
    /* Granularity of Y axis should be 1! */
    ok(hr == S_OK && di_op.dwData == 1, "GetProperty(): %08x, dwData: %i but should be 1.\n", hr, di_op.dwData);
}
    if (pMouse) IUnknown_Release(pMouse);

    DestroyWindow( hwnd2 );
}

static void test_GetDeviceInfo(IDirectInputA *pDI)
{
    HRESULT hr;
    IDirectInputDeviceA *pMouse = NULL;
    DIDEVICEINSTANCEA instA;
    DIDEVICEINSTANCE_DX3A inst3A;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pMouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    instA.dwSize = sizeof(instA);
    hr = IDirectInputDevice_GetDeviceInfo(pMouse, &instA);
    ok(SUCCEEDED(hr), "got %08x\n", hr);

    inst3A.dwSize = sizeof(inst3A);
    hr = IDirectInputDevice_GetDeviceInfo(pMouse, (DIDEVICEINSTANCEA *)&inst3A);
    ok(SUCCEEDED(hr), "got %08x\n", hr);

    ok(instA.dwSize != inst3A.dwSize, "got %d, %d \n", instA.dwSize, inst3A.dwSize);
    ok(IsEqualGUID(&instA.guidInstance, &inst3A.guidInstance), "got %s, %s\n",
            wine_dbgstr_guid(&instA.guidInstance), wine_dbgstr_guid(&inst3A.guidInstance) );
    ok(IsEqualGUID(&instA.guidProduct, &inst3A.guidProduct), "got %s, %s\n",
            wine_dbgstr_guid(&instA.guidProduct), wine_dbgstr_guid(&inst3A.guidProduct) );
    ok(instA.dwDevType == inst3A.dwDevType, "got %d, %d\n", instA.dwDevType, inst3A.dwDevType);

    IUnknown_Release(pMouse);
}

static BOOL CALLBACK EnumAxes(const DIDEVICEOBJECTINSTANCEA *pdidoi, void *pContext)
{
    if (IsEqualIID(&pdidoi->guidType, &GUID_XAxis) ||
        IsEqualIID(&pdidoi->guidType, &GUID_YAxis) ||
        IsEqualIID(&pdidoi->guidType, &GUID_ZAxis))
    {
        ok(pdidoi->dwFlags & DIDOI_ASPECTPOSITION, "Missing DIDOI_ASPECTPOSITION, flags are 0x%x\n",
            pdidoi->dwFlags);
    }
    else
        ok(pdidoi->dwFlags == 0, "Flags are 0x%x\n", pdidoi->dwFlags);

    return DIENUM_CONTINUE;
}

static void test_mouse_EnumObjects(IDirectInputA *pDI)
{
    HRESULT hr;
    IDirectInputDeviceA *pMouse = NULL;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pMouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInputDevice_EnumObjects(pMouse, EnumAxes, NULL, DIDFT_ALL);
    ok(hr==DI_OK,"IDirectInputDevice_EnumObjects() failed: %08x\n", hr);

    if (pMouse) IUnknown_Release(pMouse);
}

static void mouse_tests(void)
{
    HRESULT hr;
    IDirectInputA *pDI = NULL;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    HWND hwnd;
    ULONG ref = 0;

    hr = DirectInputCreateA(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (hr == DIERR_OLDDIRECTINPUTVERSION)
    {
        skip("Tests require a newer dinput version\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInputCreateA() failed: %08x\n", hr);
    if (FAILED(hr)) return;

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
                         NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());
    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);

        test_set_coop(pDI, hwnd);
        test_acquire(pDI, hwnd);
        test_GetDeviceInfo(pDI);
        test_mouse_EnumObjects(pDI);

        DestroyWindow(hwnd);
    }
    if (pDI) ref = IUnknown_Release(pDI);
    ok(!ref, "IDirectInput_Release() reference count = %d\n", ref);
}

START_TEST(mouse)
{
    CoInitialize(NULL);

    mouse_tests();

    CoUninitialize();
}
