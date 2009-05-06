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

#include "config.h"
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "xinput.h"

WINE_DEFAULT_DEBUG_CHANNEL(xinput);

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE; /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void WINAPI XInputEnable(BOOL enable)
{
    /* Setting to false will stop messages from XInputSetState being sent
    to the controllers. Setting to true will send the last vibration
    value (sent to XInputSetState) to the controller and allow messages to
    be sent */
    FIXME("(%d) Stub!\n", enable);
}

DWORD WINAPI XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
    FIXME("(%d %p) Stub!\n", dwUserIndex, pVibration);

    if (dwUserIndex < XUSER_MAX_COUNT)
    {
        return ERROR_DEVICE_NOT_CONNECTED;
        /* If controller exists then return ERROR_SUCCESS */
    }
    return ERROR_BAD_ARGUMENTS;
}

DWORD WINAPI XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    FIXME("(%u %p)\n", dwUserIndex, pState);

    if (dwUserIndex < XUSER_MAX_COUNT)
    {
        return ERROR_DEVICE_NOT_CONNECTED;
        /* If controller exists then return ERROR_SUCCESS */
    }
    return ERROR_BAD_ARGUMENTS;
}

DWORD WINAPI XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserve, PXINPUT_KEYSTROKE pKeystroke)
{
    FIXME("(%d %d %p) Stub!\n", dwUserIndex, dwReserve, pKeystroke);

    if (dwUserIndex < XUSER_MAX_COUNT)
    {
        return ERROR_DEVICE_NOT_CONNECTED;
        /* If controller exists then return ERROR_SUCCESS */
    }
    return ERROR_BAD_ARGUMENTS;
}

DWORD WINAPI XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities)
{
    FIXME("(%d %d %p)\n", dwUserIndex, dwFlags, pCapabilities);

    if (dwUserIndex < XUSER_MAX_COUNT)
    {
        return ERROR_DEVICE_NOT_CONNECTED;
        /* If controller exists then return ERROR_SUCCESS */
    }
    return ERROR_BAD_ARGUMENTS;
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid)
{
    FIXME("(%d %s %s) Stub!\n", dwUserIndex, debugstr_guid(pDSoundRenderGuid), debugstr_guid(pDSoundCaptureGuid));

    if (dwUserIndex < XUSER_MAX_COUNT)
    {
        return ERROR_DEVICE_NOT_CONNECTED;
        /* If controller exists then return ERROR_SUCCESS */
    }
    return ERROR_BAD_ARGUMENTS;
}

DWORD WINAPI XInputGetBatteryInformation(DWORD dwUserIndex, BYTE deviceType, XINPUT_BATTERY_INFORMATION* pBatteryInfo)
{
    FIXME("(%d %u %p) Stub!\n", dwUserIndex, deviceType, pBatteryInfo);

    if (dwUserIndex < XUSER_MAX_COUNT)
    {
        return ERROR_DEVICE_NOT_CONNECTED;
        /* If controller exists then return ERROR_SUCCESS */
    }
    return ERROR_BAD_ARGUMENTS;
}
