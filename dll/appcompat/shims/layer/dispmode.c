/*
 * PROJECT:     ReactOS 'Layers' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Display settings related shims
 * COPYRIGHT:   Copyright 2016,2017 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <shimlib.h>


#define SHIM_NS         Force8BitColor
#include <setup_shim.inl>

#define SHIM_NUM_HOOKS  0
#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)

BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    if (fdwReason == SHIM_REASON_INIT)
    {
        DEVMODEA dm = { { 0 } };
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm) &&
            dm.dmBitsPerPel != 8)
        {
            dm.dmBitsPerPel = 8;
            dm.dmFields |= DM_BITSPERPEL;
            ChangeDisplaySettingsA(&dm, CDS_FULLSCREEN);
        }
    }
    return TRUE;
}

#include <implement_shim.inl>



#define SHIM_NS         Force640x480
#include <setup_shim.inl>

#define SHIM_NUM_HOOKS  0
#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)

BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    if (fdwReason == SHIM_REASON_INIT)
    {
        DEVMODEA dm = { { 0 } };
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm) &&
            (dm.dmPelsWidth != 640 || dm.dmPelsHeight != 480))
        {
            dm.dmPelsWidth = 640;
            dm.dmPelsHeight = 480;
            dm.dmFields |= (DM_PELSWIDTH | DM_PELSHEIGHT);
            ChangeDisplaySettingsA(&dm, CDS_FULLSCREEN);
        }
    }
    return TRUE;
}

#include <implement_shim.inl>


