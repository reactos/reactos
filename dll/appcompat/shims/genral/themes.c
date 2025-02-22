/*
 * PROJECT:     ReactOS 'General' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Theme related shims
 * COPYRIGHT:   Copyright 2016,2017 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <shimlib.h>
#include <strsafe.h>
#include <uxtheme.h>


#define SHIM_NS         DisableThemes
#include <setup_shim.inl>

#define SHIM_NUM_HOOKS  0
#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)

BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    if (fdwReason == SHIM_REASON_INIT)
    {
        /* Disable themes for non-client, comctl controls and webcontent */
        SetThemeAppProperties(0);
    }
    return TRUE;
}

#include <implement_shim.inl>

