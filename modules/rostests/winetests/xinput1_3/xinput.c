/*
 * The Wine project - Xinput Joystick Library
 * Copyright 2008 Andrew Fenn
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

#include <windows.h>
#include <stdio.h>

#include "xinput.h"
#include "wine/test.h"

static DWORD (WINAPI *pXInputGetState)(DWORD, XINPUT_STATE*);
static DWORD (WINAPI *pXInputGetStateEx)(DWORD, XINPUT_STATE*);
static DWORD (WINAPI *pXInputGetCapabilities)(DWORD,DWORD,XINPUT_CAPABILITIES*);
static DWORD (WINAPI *pXInputSetState)(DWORD, XINPUT_VIBRATION*);
static void  (WINAPI *pXInputEnable)(BOOL);
static DWORD (WINAPI *pXInputGetKeystroke)(DWORD, DWORD, PXINPUT_KEYSTROKE);
static DWORD (WINAPI *pXInputGetDSoundAudioDeviceGuids)(DWORD, GUID*, GUID*);
static DWORD (WINAPI *pXInputGetBatteryInformation)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);

static void dump_gamepad(XINPUT_GAMEPAD *data)
{
    trace("-- Gamepad Variables --\n");
    trace("Gamepad.wButtons: %#x\n", data->wButtons);
    trace("Gamepad.bLeftTrigger: %d\n", data->bLeftTrigger);
    trace("Gamepad.bRightTrigger: %d\n", data->bRightTrigger);
    trace("Gamepad.sThumbLX: %d\n", data->sThumbLX);
    trace("Gamepad.sThumbLY: %d\n", data->sThumbLY);
    trace("Gamepad.sThumbRX: %d\n", data->sThumbRX);
    trace("Gamepad.sThumbRY: %d\n\n", data->sThumbRY);
}

static void test_set_state(void)
{
    XINPUT_VIBRATION vibrator;
    DWORD controllerNum;
    DWORD result;

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&vibrator, sizeof(XINPUT_VIBRATION));

        vibrator.wLeftMotorSpeed = 32767;
        vibrator.wRightMotorSpeed = 32767;
        result = pXInputSetState(controllerNum, &vibrator);
        if (result == ERROR_DEVICE_NOT_CONNECTED) continue;

        Sleep(250);
        vibrator.wLeftMotorSpeed = 0;
        vibrator.wRightMotorSpeed = 0;
        result = pXInputSetState(controllerNum, &vibrator);
        ok(result == ERROR_SUCCESS, "XInputSetState failed with (%d)\n", result);

        /* Disabling XInput here, queueing a vibration and then re-enabling XInput
         * is used to prove that vibrations are auto enabled when resuming XInput.
         * If XInputEnable(1) is removed below the vibration will never play. */
        if (pXInputEnable) pXInputEnable(0);

        Sleep(250);
        vibrator.wLeftMotorSpeed = 65535;
        vibrator.wRightMotorSpeed = 65535;
        result = pXInputSetState(controllerNum, &vibrator);
        ok(result == ERROR_SUCCESS, "XInputSetState failed with (%d)\n", result);

        if (pXInputEnable) pXInputEnable(1);
        Sleep(250);

        vibrator.wLeftMotorSpeed = 0;
        vibrator.wRightMotorSpeed = 0;
        result = pXInputSetState(controllerNum, &vibrator);
        ok(result == ERROR_SUCCESS, "XInputSetState failed with (%d)\n", result);
    }

    result = pXInputSetState(XUSER_MAX_COUNT+1, &vibrator);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputSetState returned (%d)\n", result);
}

static void test_get_state(void)
{
    XINPUT_STATE state;
    DWORD controllerNum, i, result, good = XUSER_MAX_COUNT;

    for (i = 0; i < (pXInputGetStateEx ? 2 : 1); i++)
    {
        for (controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
        {
            ZeroMemory(&state, sizeof(state));

            if (i == 0)
                result = pXInputGetState(controllerNum, &state);
            else
                result = pXInputGetStateEx(controllerNum, &state);
            ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED,
               "%s failed with (%d)\n", i == 0 ? "XInputGetState" : "XInputGetStateEx", result);

            if (ERROR_DEVICE_NOT_CONNECTED == result)
            {
                skip("Controller %d is not connected\n", controllerNum);
                continue;
            }

            trace("-- Results for controller %d --\n", controllerNum);
            if (i == 0)
            {
                good = controllerNum;
                trace("XInputGetState: %d\n", result);
            }
            else
                trace("XInputGetStateEx: %d\n", result);
            trace("State->dwPacketNumber: %d\n", state.dwPacketNumber);
            dump_gamepad(&state.Gamepad);
        }
    }

    result = pXInputGetState(0, NULL);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned (%d)\n", result);

    result = pXInputGetState(XUSER_MAX_COUNT, &state);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned (%d)\n", result);

    result = pXInputGetState(XUSER_MAX_COUNT+1, &state);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned (%d)\n", result);
    if (pXInputGetStateEx)
    {
        result = pXInputGetStateEx(XUSER_MAX_COUNT, &state);
        ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned (%d)\n", result);

        result = pXInputGetStateEx(XUSER_MAX_COUNT+1, &state);
        ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned (%d)\n", result);
    }

    if (winetest_interactive && good < XUSER_MAX_COUNT)
    {
        DWORD now = GetTickCount(), packet = 0;
        XINPUT_GAMEPAD *game = &state.Gamepad;

        trace("You have 20 seconds to test the joystick freely\n");
        do
        {
            Sleep(100);
            pXInputGetState(good, &state);
            if (state.dwPacketNumber == packet)
                continue;

            packet = state.dwPacketNumber;
            trace("Buttons 0x%04X Triggers %3d/%3d LT %6d/%6d RT %6d/%6d\n",
                  game->wButtons, game->bLeftTrigger, game->bRightTrigger,
                  game->sThumbLX, game->sThumbLY, game->sThumbRX, game->sThumbRY);
        }
        while(GetTickCount() - now < 20000);
        trace("Test over...\n");
    }
}

static void test_get_keystroke(void)
{
    XINPUT_KEYSTROKE keystroke;
    DWORD controllerNum;
    DWORD result;

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&keystroke, sizeof(XINPUT_KEYSTROKE));

        result = pXInputGetKeystroke(controllerNum, XINPUT_FLAG_GAMEPAD, &keystroke);
        ok(result == ERROR_EMPTY || result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED,
           "XInputGetKeystroke failed with (%d)\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            skip("Controller %d is not connected\n", controllerNum);
        }
    }

    ZeroMemory(&keystroke, sizeof(XINPUT_KEYSTROKE));
    result = pXInputGetKeystroke(XUSER_MAX_COUNT+1, XINPUT_FLAG_GAMEPAD, &keystroke);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetKeystroke returned (%d)\n", result);
}

static void test_get_capabilities(void)
{
    XINPUT_CAPABILITIES capabilities;
    DWORD controllerNum;
    DWORD result;

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&capabilities, sizeof(XINPUT_CAPABILITIES));

        result = pXInputGetCapabilities(controllerNum, XINPUT_FLAG_GAMEPAD, &capabilities);
        ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED, "XInputGetCapabilities failed with (%d)\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            skip("Controller %d is not connected\n", controllerNum);
            continue;
        }

        /* Important to show that the results changed between 1.3 and 1.4 XInput version */
        dump_gamepad(&capabilities.Gamepad);
    }

    ZeroMemory(&capabilities, sizeof(XINPUT_CAPABILITIES));
    result = pXInputGetCapabilities(XUSER_MAX_COUNT+1, XINPUT_FLAG_GAMEPAD, &capabilities);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetCapabilities returned (%d)\n", result);
}

static void test_get_dsoundaudiodevice(void)
{
    DWORD controllerNum;
    DWORD result;
    GUID soundRender, soundCapture;
    GUID testGuid = {0xFFFFFFFF, 0xFFFF, 0xFFFF, {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}};
    GUID emptyGuid = {0x0, 0x0, 0x0, {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}};

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        soundRender = soundCapture = testGuid;
        result = pXInputGetDSoundAudioDeviceGuids(controllerNum, &soundRender, &soundCapture);
        ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED, "XInputGetDSoundAudioDeviceGuids failed with (%d)\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            skip("Controller %d is not connected\n", controllerNum);
            continue;
        }

        if (!IsEqualGUID(&soundRender, &emptyGuid))
            ok(!IsEqualGUID(&soundRender, &testGuid), "Broken GUID returned for sound render device\n");
        else
            trace("Headset phone not attached\n");

        if (!IsEqualGUID(&soundCapture, &emptyGuid))
            ok(!IsEqualGUID(&soundCapture, &testGuid), "Broken GUID returned for sound capture device\n");
        else
            trace("Headset microphone not attached\n");
    }

    result = pXInputGetDSoundAudioDeviceGuids(XUSER_MAX_COUNT+1, &soundRender, &soundCapture);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetDSoundAudioDeviceGuids returned (%d)\n", result);
}

static void test_get_batteryinformation(void)
{
    DWORD controllerNum;
    DWORD result;
    XINPUT_BATTERY_INFORMATION batteryInfo;

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&batteryInfo, sizeof(XINPUT_BATTERY_INFORMATION));

        result = pXInputGetBatteryInformation(controllerNum, BATTERY_DEVTYPE_GAMEPAD, &batteryInfo);
        ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED, "XInputGetBatteryInformation failed with (%d)\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            ok(batteryInfo.BatteryLevel == BATTERY_TYPE_DISCONNECTED, "Failed to report device as being disconnected.\n");
            skip("Controller %d is not connected\n", controllerNum);
        }
    }

    result = pXInputGetBatteryInformation(XUSER_MAX_COUNT+1, BATTERY_DEVTYPE_GAMEPAD, &batteryInfo);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetBatteryInformation returned (%d)\n", result);
}

START_TEST(xinput)
{
    struct
    {
        const char *name;
        int version;
    } libs[] = {
        { "xinput1_1.dll",   1 },
        { "xinput1_2.dll",   2 },
        { "xinput1_3.dll",   3 },
        { "xinput1_4.dll",   4 },
        { "xinput9_1_0.dll", 0 } /* legacy for XP/Vista */
    };
    HMODULE hXinput;
    void *pXInputGetStateEx_Ordinal;
    int i;

    for (i = 0; i < ARRAY_SIZE(libs); i++)
    {
        hXinput = LoadLibraryA( libs[i].name );

        if (!hXinput)
        {
            win_skip("Could not load %s\n", libs[i].name);
            continue;
        }
        trace("Testing %s\n", libs[i].name);

        pXInputEnable = (void*)GetProcAddress(hXinput, "XInputEnable");
        pXInputSetState = (void*)GetProcAddress(hXinput, "XInputSetState");
        pXInputGetState = (void*)GetProcAddress(hXinput, "XInputGetState");
        pXInputGetStateEx = (void*)GetProcAddress(hXinput, "XInputGetStateEx"); /* Win >= 8 */
        pXInputGetStateEx_Ordinal = (void*)GetProcAddress(hXinput, (LPCSTR) 100);
        pXInputGetKeystroke = (void*)GetProcAddress(hXinput, "XInputGetKeystroke");
        pXInputGetCapabilities = (void*)GetProcAddress(hXinput, "XInputGetCapabilities");
        pXInputGetDSoundAudioDeviceGuids = (void*)GetProcAddress(hXinput, "XInputGetDSoundAudioDeviceGuids");
        pXInputGetBatteryInformation = (void*)GetProcAddress(hXinput, "XInputGetBatteryInformation");

        /* XInputGetStateEx may not be present by name, use ordinal in this case */
        if (!pXInputGetStateEx)
            pXInputGetStateEx = pXInputGetStateEx_Ordinal;

        test_set_state();
        test_get_state();
        test_get_capabilities();

        if (libs[i].version != 4)
            test_get_dsoundaudiodevice();
        else
            ok(!pXInputGetDSoundAudioDeviceGuids, "XInputGetDSoundAudioDeviceGuids exists in %s\n", libs[i].name);

        if (libs[i].version > 2)
        {
            test_get_keystroke();
            test_get_batteryinformation();
            ok(pXInputGetStateEx != NULL, "XInputGetStateEx not found in %s\n", libs[i].name);
        }
        else
        {
            ok(!pXInputGetKeystroke, "XInputGetKeystroke exists in %s\n", libs[i].name);
            ok(!pXInputGetStateEx, "XInputGetStateEx exists in %s\n", libs[i].name);
            ok(!pXInputGetBatteryInformation, "XInputGetBatteryInformation exists in %s\n", libs[i].name);
            if (libs[i].version == 0)
                ok(!pXInputEnable, "XInputEnable exists in %s\n", libs[i].name);
        }

        FreeLibrary(hXinput);
    }
}
