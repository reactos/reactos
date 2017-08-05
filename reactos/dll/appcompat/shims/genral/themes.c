/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/genral/themes.c
 * PURPOSE:         Theme related shims
 * PROGRAMMER:      Mark Jansen (mark.jansen@reactos.org)
 */

#include <windows.h>
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

