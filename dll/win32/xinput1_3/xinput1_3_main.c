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

#include <config.h>
//#include <assert.h>
//#include <stdarg.h>
//#include <string.h>

#include <wine/debug.h>
//#include "windef.h"
#include <winbase.h>
//#include "winerror.h"

#include <xinput.h>

/* Not defined in the headers, used only by XInputGetStateEx */
#define XINPUT_GAMEPAD_GUIDE 0x0400

WINE_DEFAULT_DEBUG_CHANNEL(xinput);

struct
{
    BOOL connected;
} controllers[XUSER_MAX_COUNT];

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        break;
    }
    return TRUE;
}

void WINAPI DECLSPEC_HOTPATCH XInputEnable(BOOL enable)
{
    /* Setting to false will stop messages from XInputSetState being sent
    to the controllers. Setting to true will send the last vibration
    value (sent to XInputSetState) to the controller and allow messages to
    be sent */
    FIXME("(enable %d) Stub!\n", enable);
}

DWORD WINAPI XInputSetState(DWORD index, XINPUT_VIBRATION* vibration)
{
    FIXME("(index %u, vibration %p) Stub!\n", index, vibration);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetState(DWORD index, XINPUT_STATE* state)
{
    union
    {
        XINPUT_STATE state;
        XINPUT_STATE_EX state_ex;
    } xinput;
    DWORD ret;
    static int warn_once;

    if (!warn_once++)
        FIXME("(index %u, state %p) Stub!\n", index, state);

    ret = XInputGetStateEx(index, &xinput.state_ex);
    if (ret != ERROR_SUCCESS)
        return ret;

    /* The main difference between this and the Ex version is the media guide button */
    xinput.state.Gamepad.wButtons &= ~XINPUT_GAMEPAD_GUIDE;
    *state = xinput.state;

    return ERROR_SUCCESS;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetStateEx(DWORD index, XINPUT_STATE_EX* state_ex)
{
    static int warn_once;

    if (!warn_once++)
        FIXME("(index %u, state %p) Stub!\n", index, state_ex);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI XInputGetKeystroke(DWORD index, DWORD reserved, PXINPUT_KEYSTROKE keystroke)
{
    static int warn_once;

    if (!warn_once++)
        FIXME("(index %u, reserved %u, keystroke %p) Stub!\n", index, reserved, keystroke);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI XInputGetCapabilities(DWORD index, DWORD flags, XINPUT_CAPABILITIES* capabilities)
{
    static int warn_once;

    if (!warn_once++)
        FIXME("(index %u, flags 0x%x, capabilities %p) Stub!\n", index, flags, capabilities);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD index, GUID* render_guid, GUID* capture_guid)
{
    FIXME("(index %u, render guid %p, capture guid %p) Stub!\n", index, render_guid, capture_guid);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI XInputGetBatteryInformation(DWORD index, BYTE type, XINPUT_BATTERY_INFORMATION* battery)
{
    FIXME("(index %u, type %u, battery %p) Stub!\n", index, type, battery);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}
