/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/genral/themes.c
 * PURPOSE:         Theme related shims
 * PROGRAMMER:      Mark Jansen
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
        SetThemeAppProperties(0);
    }
    return TRUE;
}

#include <implement_shim.inl>

