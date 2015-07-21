/*
 * Copyright (c) 2004-2005 Robert Reif
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
#include <stdio.h>
//#include <stdlib.h>

#include <wine/test.h>
//#include "windef.h"
//#include "wingdi.h"
#include <winnls.h>
#include <dinput.h>

#define numObjects(x) (sizeof(x) / sizeof(x[0]))

typedef struct tagUserData {
    IDirectInputA *pDI;
    DWORD version;
} UserData;

static const DIOBJECTDATAFORMAT dfDIJoystickTest[] = {
  { &GUID_XAxis,DIJOFS_X,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_YAxis,DIJOFS_Y,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_ZAxis,DIJOFS_Z,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RxAxis,DIJOFS_RX,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RyAxis,DIJOFS_RY,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RzAxis,DIJOFS_RZ,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(0),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(1),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(2),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(3),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(4),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(5),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(6),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(7),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(8),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(9),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_Button,DIJOFS_BUTTON(10),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
};

static const DIDATAFORMAT c_dfDIJoystickTest = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(DIJOYSTATE2),
    numObjects(dfDIJoystickTest),
    (LPDIOBJECTDATAFORMAT)dfDIJoystickTest
};

static HWND get_hwnd(void)
{
    HWND hwnd=GetForegroundWindow();
    if (!hwnd)
        hwnd=GetDesktopWindow();
    return hwnd;
}

typedef struct tagJoystickInfo
{
    IDirectInputDeviceA *pJoystick;
    DWORD axis;
    DWORD pov;
    DWORD button;
    LONG  lMin, lMax;
    DWORD dZone;
} JoystickInfo;

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef( object );
    return IUnknown_Release( object );
}

static BOOL CALLBACK EnumAxes(const DIDEVICEOBJECTINSTANCEA *pdidoi, void *pContext)
{
    HRESULT hr;
    JoystickInfo * info = pContext;

    if (IsEqualIID(&pdidoi->guidType, &GUID_XAxis) ||
        IsEqualIID(&pdidoi->guidType, &GUID_YAxis) ||
        IsEqualIID(&pdidoi->guidType, &GUID_ZAxis) ||
        IsEqualIID(&pdidoi->guidType, &GUID_RxAxis) ||
        IsEqualIID(&pdidoi->guidType, &GUID_RyAxis) ||
        IsEqualIID(&pdidoi->guidType, &GUID_RzAxis) ||
        IsEqualIID(&pdidoi->guidType, &GUID_Slider))
    {
        DIPROPRANGE diprg;
        DIPROPDWORD dipdw;

        diprg.diph.dwSize       = sizeof(DIPROPRANGE);
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprg.diph.dwHow        = DIPH_BYID;
        diprg.diph.dwObj        = pdidoi->dwType;

        dipdw.diph.dwSize       = sizeof(dipdw);
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwHow        = DIPH_BYID;
        dipdw.diph.dwObj        = pdidoi->dwType;

        hr = IDirectInputDevice_GetProperty(info->pJoystick, DIPROP_RANGE, &diprg.diph);
        ok(SUCCEEDED(hr), "IDirectInputDevice_GetProperty() failed: %08x\n", hr);
        ok(info->lMin == diprg.lMin && info->lMax == diprg.lMax, "Min/Max range invalid: "
           "expected %d..%d got %d..%d\n", info->lMin, info->lMax, diprg.lMin, diprg.lMax);

        diprg.lMin = -2000;
        diprg.lMax = +2000;

        hr = IDirectInputDevice_SetProperty(info->pJoystick, DIPROP_RANGE, NULL);
        ok(hr==E_INVALIDARG,"IDirectInputDevice_SetProperty() should have returned "
           "E_INVALIDARG, returned: %08x\n", hr);

        hr = IDirectInputDevice_SetProperty(info->pJoystick, DIPROP_RANGE, &diprg.diph);
        ok(hr==DI_OK,"IDirectInputDevice_SetProperty() failed: %08x\n", hr);

        /* dead zone */
        hr = IDirectInputDevice_GetProperty(info->pJoystick, DIPROP_DEADZONE, &dipdw.diph);
        ok(SUCCEEDED(hr), "IDirectInputDevice_GetProperty() failed: %08x\n", hr);
        ok(info->dZone == dipdw.dwData, "deadzone invalid: expected %d got %d\n",
           info->dZone, dipdw.dwData);

        dipdw.dwData = 123;

        hr = IDirectInputDevice_SetProperty(info->pJoystick, DIPROP_DEADZONE, &dipdw.diph);
        ok(hr==DI_OK,"IDirectInputDevice_SetProperty() failed: %08x\n", hr);

        info->axis++;
    } else if (IsEqualIID(&pdidoi->guidType, &GUID_POV))
        info->pov++;
    else if (IsEqualIID(&pdidoi->guidType, &GUID_Button))
        info->button++;

    return DIENUM_CONTINUE;
}

static const HRESULT SetCoop_null_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static const HRESULT SetCoop_real_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, S_OK,         S_OK,         E_INVALIDARG,
    E_INVALIDARG, S_OK,         S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static BOOL CALLBACK EnumJoysticks(const DIDEVICEINSTANCEA *lpddi, void *pvRef)
{
    HRESULT hr;
    UserData * data = pvRef;
    IDirectInputDeviceA *pJoystick;
    DIDATAFORMAT format;
    DIDEVCAPS caps;
    DIJOYSTATE2 js;
    JoystickInfo info;
    int i, count;
    ULONG ref;
    DIDEVICEINSTANCEA inst;
    DIDEVICEINSTANCE_DX3A inst3;
    DIPROPDWORD dipw;
    DIPROPSTRING dps;
    DIPROPGUIDANDPATH dpg;
    WCHAR nameBuffer[MAX_PATH];
    HWND hWnd = get_hwnd();
    char oldstate[248], curstate[248];

    ok(data->version > 0x0300, "Joysticks not supported in version 0x%04x\n", data->version);
 
    hr = IDirectInput_CreateDevice(data->pDI, &lpddi->guidInstance, NULL, NULL);
    ok(hr==E_POINTER,"IDirectInput_CreateDevice() should have returned "
       "E_POINTER, returned: %08x\n", hr);

    hr = IDirectInput_CreateDevice(data->pDI, NULL, &pJoystick, NULL);
    ok(hr==E_POINTER,"IDirectInput_CreateDevice() should have returned "
       "E_POINTER, returned: %08x\n", hr);

    hr = IDirectInput_CreateDevice(data->pDI, NULL, NULL, NULL);
    ok(hr==E_POINTER,"IDirectInput_CreateDevice() should have returned "
       "E_POINTER, returned: %08x\n", hr);

    hr = IDirectInput_CreateDevice(data->pDI, &lpddi->guidInstance,
                                   &pJoystick, NULL);
    ok(hr==DI_OK,"IDirectInput_CreateDevice() failed: %08x\n", hr);
    if (hr!=DI_OK)
        goto DONE;

    trace("---- %s ----\n", lpddi->tszProductName);

    /* Test for joystick ID property */
    ZeroMemory(&dipw, sizeof(dipw));
    dipw.diph.dwSize = sizeof(DIPROPDWORD);
    dipw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipw.diph.dwObj = 0;
    dipw.diph.dwHow = DIPH_DEVICE;

    hr = IDirectInputDevice_GetProperty(pJoystick, DIPROP_JOYSTICKID, &dipw.diph);
    ok(SUCCEEDED(hr), "IDirectInputDevice_GetProperty() for DIPROP_JOYSTICKID failed\n");

    /* Test for INSTANCENAME property */
    memset(&dps, 0, sizeof(dps));
    dps.diph.dwSize = sizeof(DIPROPSTRING);
    dps.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dps.diph.dwHow = DIPH_DEVICE;

    hr = IDirectInputDevice_GetProperty(pJoystick, DIPROP_INSTANCENAME, &dps.diph);
    ok(SUCCEEDED(hr), "IDirectInput_GetProperty() for DIPROP_INSTANCENAME failed: %08x\n", hr);

    /* Test if instance name is the same as present in DIDEVICEINSTANCE */
    MultiByteToWideChar(CP_ACP, 0, lpddi->tszInstanceName, -1, nameBuffer, MAX_PATH);
    ok(!lstrcmpW(nameBuffer, dps.wsz), "DIPROP_INSTANCENAME returned is wrong. Expected: %s Got: %s\n",
                 wine_dbgstr_w(nameBuffer), wine_dbgstr_w(dps.wsz));

    /* Test for GUIDPATH properties */
    memset(&dpg, 0, sizeof(dpg));
    dpg.diph.dwSize = sizeof(DIPROPGUIDANDPATH);
    dpg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dpg.diph.dwHow = DIPH_DEVICE;

    hr = IDirectInputDevice_GetProperty(pJoystick, DIPROP_GUIDANDPATH, &dpg.diph);
    todo_wine ok(SUCCEEDED(hr), "IDirectInput_GetProperty() for DIPROP_GUIDANDPATH failed: %08x\n", hr);

    hr = IDirectInputDevice_SetDataFormat(pJoystick, NULL);
    ok(hr==E_POINTER,"IDirectInputDevice_SetDataFormat() should have returned "
       "E_POINTER, returned: %08x\n", hr);

    ZeroMemory(&format, sizeof(format));
    hr = IDirectInputDevice_SetDataFormat(pJoystick, &format);
    ok(hr==DIERR_INVALIDPARAM,"IDirectInputDevice_SetDataFormat() should have "
       "returned DIERR_INVALIDPARAM, returned: %08x\n", hr);

    /* try the default formats */
    hr = IDirectInputDevice_SetDataFormat(pJoystick, &c_dfDIJoystick);
    ok(hr==DI_OK,"IDirectInputDevice_SetDataFormat() failed: %08x\n", hr);

    hr = IDirectInputDevice_SetDataFormat(pJoystick, &c_dfDIJoystick2);
    ok(hr==DI_OK,"IDirectInputDevice_SetDataFormat() failed: %08x\n", hr);

    /* try an alternate format */
    hr = IDirectInputDevice_SetDataFormat(pJoystick, &c_dfDIJoystickTest);
    ok(hr==DI_OK,"IDirectInputDevice_SetDataFormat() failed: %08x\n", hr);

    hr = IDirectInputDevice_SetDataFormat(pJoystick, &c_dfDIJoystick2);
    ok(hr==DI_OK,"IDirectInputDevice_SetDataFormat() failed: %08x\n", hr);
    if (hr != DI_OK)
        goto RELEASE;

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pJoystick, NULL, i);
        ok(hr == SetCoop_null_window[i], "SetCooperativeLevel(NULL, %d): %08x\n", i, hr);
    }
    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pJoystick, hWnd, i);
        ok(hr == SetCoop_real_window[i], "SetCooperativeLevel(hwnd, %d): %08x\n", i, hr);
    }

    hr = IDirectInputDevice_SetCooperativeLevel(pJoystick, hWnd,
                                                DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    ok(hr==DI_OK,"IDirectInputDevice_SetCooperativeLevel() failed: %08x\n", hr);

    /* get capabilities */
    hr = IDirectInputDevice_GetCapabilities(pJoystick, NULL);
    ok(hr==E_POINTER,"IDirectInputDevice_GetCapabilities() "
       "should have returned E_POINTER, returned: %08x\n", hr);

    ZeroMemory(&caps, sizeof(caps));
    hr = IDirectInputDevice_GetCapabilities(pJoystick, &caps);
    ok(hr==DIERR_INVALIDPARAM,"IDirectInputDevice_GetCapabilities() "
       "should have returned DIERR_INVALIDPARAM, returned: %08x\n", hr);

    caps.dwSize = sizeof(caps);
    hr = IDirectInputDevice_GetCapabilities(pJoystick, &caps);
    ok(hr==DI_OK,"IDirectInputDevice_GetCapabilities() failed: %08x\n", hr);

    ZeroMemory(&info, sizeof(info));
    info.pJoystick = pJoystick;

    /* default min/max limits */
    info.lMin = 0;
    info.lMax = 0xffff;
    /* enumerate objects */
    hr = IDirectInputDevice_EnumObjects(pJoystick, EnumAxes, &info, DIDFT_ALL);
    ok(hr==DI_OK,"IDirectInputDevice_EnumObjects() failed: %08x\n", hr);

    ok(caps.dwAxes == info.axis, "Number of enumerated axes (%d) doesn't match capabilities (%d)\n", info.axis, caps.dwAxes);
    ok(caps.dwButtons == info.button, "Number of enumerated buttons (%d) doesn't match capabilities (%d)\n", info.button, caps.dwButtons);
    ok(caps.dwPOVs == info.pov, "Number of enumerated POVs (%d) doesn't match capabilities (%d)\n", info.pov, caps.dwPOVs);

    /* Set format and check limits again */
    hr = IDirectInputDevice_SetDataFormat(pJoystick, &c_dfDIJoystick2);
    ok(hr==DI_OK,"IDirectInputDevice_SetDataFormat() failed: %08x\n", hr);
    info.lMin = -2000;
    info.lMax = +2000;
    info.dZone= 123;
    hr = IDirectInputDevice_EnumObjects(pJoystick, EnumAxes, &info, DIDFT_ALL);
    ok(hr==DI_OK,"IDirectInputDevice_EnumObjects() failed: %08x\n", hr);

    hr = IDirectInputDevice_GetDeviceInfo(pJoystick, 0);
    ok(hr==E_POINTER, "IDirectInputDevice_GetDeviceInfo() "
       "should have returned E_POINTER, returned: %08x\n", hr);

    ZeroMemory(&inst, sizeof(inst));
    ZeroMemory(&inst3, sizeof(inst3));

    hr = IDirectInputDevice_GetDeviceInfo(pJoystick, &inst);
    ok(hr==DIERR_INVALIDPARAM, "IDirectInputDevice_GetDeviceInfo() "
       "should have returned DIERR_INVALIDPARAM, returned: %08x\n", hr);

    inst.dwSize = sizeof(inst);
    hr = IDirectInputDevice_GetDeviceInfo(pJoystick, &inst);
    ok(hr==DI_OK,"IDirectInputDevice_GetDeviceInfo() failed: %08x\n", hr);

    inst3.dwSize = sizeof(inst3);
    hr = IDirectInputDevice_GetDeviceInfo(pJoystick, (DIDEVICEINSTANCEA*)&inst3);
    ok(hr==DI_OK,"IDirectInputDevice_GetDeviceInfo() failed: %08x\n", hr);

    hr = IDirectInputDevice_Unacquire(pJoystick);
    ok(hr == S_FALSE, "IDirectInputDevice_Unacquire() should have returned S_FALSE, got: %08x\n", hr);

    hr = IDirectInputDevice_Acquire(pJoystick);
    ok(hr==DI_OK,"IDirectInputDevice_Acquire() failed: %08x\n", hr);
    if (hr != DI_OK)
        goto RELEASE;

    hr = IDirectInputDevice_Acquire(pJoystick);
    ok(hr == S_FALSE, "IDirectInputDevice_Acquire() should have returned S_FALSE, got: %08x\n", hr);

    if (info.pov < 4)
    {
        hr = IDirectInputDevice_GetDeviceState(pJoystick, sizeof(DIJOYSTATE2), &js);
        ok(hr == DI_OK, "IDirectInputDevice_GetDeviceState() failed: %08x\n", hr);
        ok(js.rgdwPOV[3] == -1, "Default for unassigned POV should be -1 not: %d\n", js.rgdwPOV[3]);
    }

    if (caps.dwFlags & DIDC_FORCEFEEDBACK)
    {
        DWORD axes[2] = {DIJOFS_X, DIJOFS_Y};
        LONG  direction[2] = {0, 0};
        DICONSTANTFORCE force = {0};
        DIEFFECT eff;
        LPDIRECTINPUTEFFECT effect = NULL;
        LONG cnt1, cnt2;
        HWND real_hWnd;
        HINSTANCE hInstance = GetModuleHandleW(NULL);
        DIPROPDWORD dip_gain_set, dip_gain_get;

        trace("Testing force-feedback\n");
        memset(&eff, 0, sizeof(eff));
        eff.dwSize                = sizeof(eff);
        eff.dwFlags               = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
        eff.dwDuration            = INFINITE;
        eff.dwGain                = DI_FFNOMINALMAX;
        eff.dwTriggerButton       = DIEB_NOTRIGGER;
        eff.cAxes                 = sizeof(axes) / sizeof(axes[0]);
        eff.rgdwAxes              = axes;
        eff.rglDirection          = direction;
        eff.cbTypeSpecificParams  = sizeof(force);
        eff.lpvTypeSpecificParams = &force;

        /* Sending effects to joystick requires
         * calling IDirectInputEffect_Initialize, which requires
         * having exclusive access to the device, which requires
         * - not having acquired the joystick when calling
         *   IDirectInputDevice_SetCooperativeLevel
         * - a visible window
         */
        real_hWnd = CreateWindowExA(0, "EDIT", "Test text", 0, 10, 10, 300, 300, NULL, NULL,
                                    hInstance, NULL);
        ok(real_hWnd!=0,"CreateWindowExA failed: %p\n", real_hWnd);
        ShowWindow(real_hWnd, SW_SHOW);
        hr = IDirectInputDevice_Unacquire(pJoystick);
        ok(hr==DI_OK,"IDirectInputDevice_Unacquire() failed: %08x\n", hr);
        hr = IDirectInputDevice_SetCooperativeLevel(pJoystick, real_hWnd,
                                                    DISCL_EXCLUSIVE | DISCL_FOREGROUND);
        ok(hr==DI_OK,"IDirectInputDevice_SetCooperativeLevel() failed: %08x\n", hr);
        hr = IDirectInputDevice_Acquire(pJoystick);
        ok(hr==DI_OK,"IDirectInputDevice_Acquire() failed: %08x\n", hr);

        cnt1 = get_refcount((IUnknown*)pJoystick);

        hr = IDirectInputDevice2_CreateEffect((IDirectInputDevice2A*)pJoystick, &GUID_ConstantForce,
                                              &eff, &effect, NULL);
        ok(hr == DI_OK, "IDirectInputDevice_CreateEffect() failed: %08x\n", hr);
        cnt2 = get_refcount((IUnknown*)pJoystick);
        ok(cnt1 == cnt2, "Ref count is wrong %d != %d\n", cnt1, cnt2);

        if (effect)
        {
            DWORD effect_status;
            struct DIPROPDWORD diprop_word;
            GUID guid = {0};

            hr = IDirectInputEffect_Initialize(effect, hInstance, data->version,
                                               &GUID_ConstantForce);
            ok(hr==DI_OK,"IDirectInputEffect_Initialize failed: %08x\n", hr);
            hr = IDirectInputEffect_SetParameters(effect, &eff, DIEP_AXES | DIEP_DIRECTION |
                                                  DIEP_TYPESPECIFICPARAMS);
            ok(hr==DI_OK,"IDirectInputEffect_SetParameters failed: %08x\n", hr);
            if (hr==DI_OK) {
                /* Test that upload, unacquire, acquire still permits updating
                 * uploaded effect. */
                hr = IDirectInputDevice_Unacquire(pJoystick);
                ok(hr==DI_OK,"IDirectInputDevice_Unacquire() failed: %08x\n", hr);
                hr = IDirectInputDevice_Acquire(pJoystick);
                ok(hr==DI_OK,"IDirectInputDevice_Acquire() failed: %08x\n", hr);
                hr = IDirectInputEffect_SetParameters(effect, &eff, DIEP_GAIN);
                ok(hr==DI_OK,"IDirectInputEffect_SetParameters failed: %08x\n", hr);
            }

            /* Check effect status.
             * State: initially stopped
             * start
             * State: started
             * unacquire, acquire, download
             * State: stopped
             * start
             * State: started
             *
             * Shows that:
             * - effects are stopped after Unacquire + Acquire
             * - effects are preserved (Download + Start doesn't complain
             *   about incomplete effect)
             */
            hr = IDirectInputEffect_GetEffectStatus(effect, &effect_status);
            ok(hr==DI_OK,"IDirectInputEffect_GetEffectStatus() failed: %08x\n", hr);
            ok(effect_status==0,"IDirectInputEffect_GetEffectStatus() reported effect as started\n");
            hr = IDirectInputEffect_SetParameters(effect, &eff, DIEP_START);
            ok(hr==DI_OK,"IDirectInputEffect_SetParameters failed: %08x\n", hr);
            hr = IDirectInputEffect_GetEffectStatus(effect, &effect_status);
            ok(hr==DI_OK,"IDirectInputEffect_GetEffectStatus() failed: %08x\n", hr);
            todo_wine ok(effect_status!=0,"IDirectInputEffect_GetEffectStatus() reported effect as stopped\n");
            hr = IDirectInputDevice_Unacquire(pJoystick);
            ok(hr==DI_OK,"IDirectInputDevice_Unacquire() failed: %08x\n", hr);
            hr = IDirectInputDevice_Acquire(pJoystick);
            ok(hr==DI_OK,"IDirectInputDevice_Acquire() failed: %08x\n", hr);
            hr = IDirectInputEffect_Download(effect);
            ok(hr==DI_OK,"IDirectInputEffect_Download() failed: %08x\n", hr);
            hr = IDirectInputEffect_GetEffectStatus(effect, &effect_status);
            ok(hr==DI_OK,"IDirectInputEffect_GetEffectStatus() failed: %08x\n", hr);
            ok(effect_status==0,"IDirectInputEffect_GetEffectStatus() reported effect as started\n");
            hr = IDirectInputEffect_Start(effect, 1, 0);
            ok(hr==DI_OK,"IDirectInputEffect_Start() failed: %08x\n", hr);
            hr = IDirectInputEffect_GetEffectStatus(effect, &effect_status);
            ok(hr==DI_OK,"IDirectInputEffect_GetEffectStatus() failed: %08x\n", hr);
            todo_wine ok(effect_status!=0,"IDirectInputEffect_GetEffectStatus() reported effect as stopped\n");
            hr = IDirectInputEffect_GetEffectGuid(effect, &guid);
            ok(hr==DI_OK,"IDirectInputEffect_GetEffectGuid() failed: %08x\n", hr);
            ok(IsEqualGUID(&GUID_ConstantForce, &guid), "Wrong guid returned\n");

            /* Check autocenter status
             * State: initially stopped
             * enable
             * State: enabled
             * acquire
             * State: enabled
             * unacquire
             * State: enabled
             *
             * IDirectInputDevice2_SetProperty(DIPROP_AUTOCENTER) can only be
             * executed when the device is released.
             *
             * If Executed interactively, user can feel that autocenter is
             * only disabled when the joystick is acquired.
             */
            diprop_word.diph.dwSize = sizeof(diprop_word);
            diprop_word.diph.dwHeaderSize = sizeof(diprop_word.diph);
            diprop_word.diph.dwObj = 0;
            diprop_word.diph.dwHow = DIPH_DEVICE;
            hr = IDirectInputDevice_Unacquire(pJoystick);
            ok(hr==DI_OK,"IDirectInputDevice_Unacquire() failed: %08x\n", hr);
            hr = IDirectInputDevice2_GetProperty(pJoystick, DIPROP_AUTOCENTER, &diprop_word.diph);
            ok(hr==DI_OK,"IDirectInputDevice2_GetProperty() failed: %08x\n", hr);
            ok(diprop_word.dwData==DIPROPAUTOCENTER_ON,"IDirectInputDevice2_GetProperty() reported autocenter as disabled\n");
            diprop_word.dwData = DIPROPAUTOCENTER_OFF;
            hr = IDirectInputDevice2_SetProperty(pJoystick, DIPROP_AUTOCENTER, &diprop_word.diph);
            ok(hr==DI_OK,"IDirectInputDevice2_SetProperty() failed: %08x\n", hr);
            hr = IDirectInputDevice2_GetProperty(pJoystick, DIPROP_AUTOCENTER, &diprop_word.diph);
            ok(hr==DI_OK,"IDirectInputDevice2_GetProperty() failed: %08x\n", hr);
            ok(diprop_word.dwData==DIPROPAUTOCENTER_OFF,"IDirectInputDevice2_GetProperty() reported autocenter as enabled\n");
            if (winetest_interactive) {
                trace("Acquiring in 2s, autocenter will be disabled.\n");
                Sleep(2000);
            }
            hr = IDirectInputDevice_Acquire(pJoystick);
            ok(hr==DI_OK,"IDirectInputDevice_Acquire() failed: %08x\n", hr);
            if (winetest_interactive)
                trace("Acquired.\n");
            hr = IDirectInputDevice2_GetProperty(pJoystick, DIPROP_AUTOCENTER, &diprop_word.diph);
            ok(hr==DI_OK,"IDirectInputDevice2_GetProperty() failed: %08x\n", hr);
            ok(diprop_word.dwData==DIPROPAUTOCENTER_OFF,"IDirectInputDevice2_GetProperty() reported autocenter as enabled\n");
            if (winetest_interactive) {
                trace("Releasing in 2s, autocenter will be re-enabled.\n");
                Sleep(2000);
            }
            hr = IDirectInputDevice_Unacquire(pJoystick);
            ok(hr==DI_OK,"IDirectInputDevice_Unacquire() failed: %08x\n", hr);
            if (winetest_interactive)
                trace("Released\n");
            hr = IDirectInputDevice2_GetProperty(pJoystick, DIPROP_AUTOCENTER, &diprop_word.diph);
            ok(hr==DI_OK,"IDirectInputDevice2_GetProperty() failed: %08x\n", hr);
            ok(diprop_word.dwData==DIPROPAUTOCENTER_OFF,"IDirectInputDevice2_GetProperty() reported autocenter as enabled\n");
            hr = IDirectInputDevice_Acquire(pJoystick);
            ok(hr==DI_OK,"IDirectInputDevice_Acquire() failed: %08x\n", hr);
            hr = IDirectInputDevice2_GetProperty(pJoystick, DIPROP_AUTOCENTER, &diprop_word.diph);
            ok(hr==DI_OK,"IDirectInputDevice2_GetProperty() failed: %08x\n", hr);

            /* Device gain (DIPROP_FFGAIN).
             * From MSDN:
             *  0..10000 range, otherwise DIERR_INVALIDPARAM.
             *  Can be changed even if device is acquired.
             * Difference found by tests:
             *  <0 is refused, >10000 is accepted
             */
            dip_gain_set.diph.dwSize       = sizeof(DIPROPDWORD);
            dip_gain_set.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            dip_gain_set.diph.dwObj        = 0;
            dip_gain_set.diph.dwHow        = DIPH_DEVICE;
            dip_gain_get.diph.dwSize       = sizeof(DIPROPDWORD);
            dip_gain_get.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            dip_gain_get.diph.dwObj        = 0;
            dip_gain_get.diph.dwHow        = DIPH_DEVICE;
            dip_gain_get.dwData            = 0;

            /* Test device is acquisition (non)impact. */
            hr = IDirectInputDevice_Unacquire(pJoystick);
            ok(hr == DI_OK, "IDirectInputDevice_Unacquire() should have returned S_FALSE, got: %08x\n", hr);
            dip_gain_set.dwData = 1;
            hr = IDirectInputDevice_SetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_set.diph);
            ok(hr==DI_OK, "IDirectInputDevice_SetProperty() failed: %08x\n", hr);
            hr = IDirectInputDevice_GetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_get.diph);
            ok(hr==DI_OK, "IDirectInputDevice_GetProperty() failed: %08x\n", hr);
            ok(dip_gain_get.dwData==dip_gain_set.dwData, "Gain not updated: %i\n", dip_gain_get.dwData);
            hr = IDirectInputDevice_Acquire(pJoystick);
            ok(hr==DI_OK,"IDirectInputDevice_Acquire() failed: %08x\n", hr);
            dip_gain_set.dwData = 2;
            hr = IDirectInputDevice_SetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_set.diph);
            ok(hr==DI_OK, "IDirectInputDevice_SetProperty() failed: %08x\n", hr);
            hr = IDirectInputDevice_GetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_get.diph);
            ok(hr==DI_OK, "IDirectInputDevice_GetProperty() failed: %08x\n", hr);
            ok(dip_gain_get.dwData==dip_gain_set.dwData, "Gain not updated: %i\n", dip_gain_get.dwData);
            /* Test range and internal clamping. */
            dip_gain_set.dwData = -1;
            hr = IDirectInputDevice_SetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_set.diph);
            todo_wine ok(hr==DIERR_INVALIDPARAM, "IDirectInputDevice_SetProperty() should have returned %08x: %08x\n", DIERR_INVALIDPARAM, hr);
            dip_gain_set.dwData = 0;
            hr = IDirectInputDevice_SetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_set.diph);
            ok(hr==DI_OK, "IDirectInputDevice_SetProperty() failed: %08x\n", hr);
            hr = IDirectInputDevice_GetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_get.diph);
            ok(hr==DI_OK, "IDirectInputDevice_GetProperty() failed: %08x\n", hr);
            ok(dip_gain_get.dwData==dip_gain_set.dwData, "Gain not updated: %i\n", dip_gain_get.dwData);
            dip_gain_set.dwData = 10000;
            hr = IDirectInputDevice_SetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_set.diph);
            ok(hr==DI_OK, "IDirectInputDevice_SetProperty() failed: %08x\n", hr);
            hr = IDirectInputDevice_GetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_get.diph);
            ok(hr==DI_OK, "IDirectInputDevice_GetProperty() failed: %08x\n", hr);
            ok(dip_gain_get.dwData==dip_gain_set.dwData, "Gain not updated: %i\n", dip_gain_get.dwData);
            /* WARNING: This call succeeds, on the contrary of what is stated on MSDN. */
            dip_gain_set.dwData = 10001;
            hr = IDirectInputDevice_SetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_set.diph);
            ok(hr==DI_OK, "IDirectInputDevice_SetProperty() failed: %08x\n", hr);
            hr = IDirectInputDevice_GetProperty(pJoystick, DIPROP_FFGAIN, &dip_gain_get.diph);
            ok(hr==DI_OK, "IDirectInputDevice_GetProperty() failed: %08x\n", hr);
            ok(dip_gain_get.dwData==dip_gain_set.dwData, "Gain not updated: %i\n", dip_gain_get.dwData);

            ref = IUnknown_Release(effect);
            ok(ref == 0, "IDirectInputDevice_Release() reference count = %d\n", ref);
        }
        cnt1 = get_refcount((IUnknown*)pJoystick);
        ok(cnt1 == cnt2, "Ref count is wrong %d != %d\n", cnt1, cnt2);

        /* Before destroying the window, release joystick to revert to
         * non-exclusive, background cooperative level. */
        hr = IDirectInputDevice_Unacquire(pJoystick);
        ok(hr==DI_OK,"IDirectInputDevice_Unacquire() failed: %08x\n", hr);
        hr = IDirectInputDevice_SetCooperativeLevel(pJoystick, hWnd,
                                                    DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        ok(hr==DI_OK,"IDirectInputDevice_SetCooperativeLevel() failed: %08x\n", hr);
        DestroyWindow (real_hWnd);
        hr = IDirectInputDevice_Acquire(pJoystick);
        ok(hr==DI_OK,"IDirectInputDevice_Acquire() failed: %08x\n", hr);
    }

    if (winetest_interactive) {
        trace("You have 30 seconds to test all axes, sliders, POVs and buttons\n");
        count = 300;
    } else
        count = 1;

    trace("\n");
    oldstate[0]='\0';
    for (i = 0; i < count; i++) {
        hr = IDirectInputDevice_GetDeviceState(pJoystick, sizeof(DIJOYSTATE2), &js);
        ok(hr==DI_OK,"IDirectInputDevice_GetDeviceState() failed: %08x\n", hr);
        if (hr != DI_OK)
            break;
        sprintf(curstate, "X%5d Y%5d Z%5d Rx%5d Ry%5d Rz%5d "
              "S0%5d S1%5d POV0%5d POV1%5d POV2%5d POV3%5d "
              "B %d %d %d %d %d %d %d %d %d %d %d %d\n",
              js.lX, js.lY, js.lZ, js.lRx, js.lRy, js.lRz,
              js.rglSlider[0], js.rglSlider[1],
              js.rgdwPOV[0], js.rgdwPOV[1], js.rgdwPOV[2], js.rgdwPOV[3],
              js.rgbButtons[0]>>7, js.rgbButtons[1]>>7, js.rgbButtons[2]>>7,
              js.rgbButtons[3]>>7, js.rgbButtons[4]>>7, js.rgbButtons[5]>>7,
              js.rgbButtons[6]>>7, js.rgbButtons[7]>>7, js.rgbButtons[8]>>7,
              js.rgbButtons[9]>>7, js.rgbButtons[10]>>7, js.rgbButtons[11]>>7);
        if (strcmp(oldstate, curstate) != 0)
        {
            trace("%s\n", curstate);
            strcpy(oldstate, curstate);
        }
        Sleep(100);
    }
    trace("\n");

    hr = IDirectInputDevice_Unacquire(pJoystick);
    ok(hr==DI_OK,"IDirectInputDevice_Unacquire() failed: %08x\n", hr);

RELEASE:
    ref = IDirectInputDevice_Release(pJoystick);
    ok(ref==0,"IDirectInputDevice_Release() reference count = %d\n", ref);

DONE:
    return DIENUM_CONTINUE;
}

static void joystick_tests(DWORD version)
{
    HRESULT hr;
    IDirectInputA *pDI;
    ULONG ref;
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    trace("-- Testing Direct Input Version 0x%04x --\n", version);
    hr = DirectInputCreateA(hInstance, version, &pDI, NULL);
    ok(hr==DI_OK||hr==DIERR_OLDDIRECTINPUTVERSION, "DirectInputCreateA() failed: %08x\n", hr);
    if (hr==DI_OK && pDI!=0) {
        UserData data;
        data.pDI = pDI;
        data.version = version;
        hr = IDirectInput_EnumDevices(pDI, DIDEVTYPE_JOYSTICK, EnumJoysticks,
                                      &data, DIEDFL_ALLDEVICES);
        ok(hr==DI_OK,"IDirectInput_EnumDevices() failed: %08x\n", hr);
        ref = IDirectInput_Release(pDI);
        ok(ref==0,"IDirectInput_Release() reference count = %d\n", ref);
    } else if (hr==DIERR_OLDDIRECTINPUTVERSION)
        trace("  Version Not Supported\n");
}

START_TEST(joystick)
{
    CoInitialize(NULL);

    joystick_tests(0x0700);
    joystick_tests(0x0500);
    joystick_tests(0x0300);

    CoUninitialize();
}
