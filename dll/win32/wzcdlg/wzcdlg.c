/*
 * PROJECT:          ReactOS System Libraries
 * LICENSE:          GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:             dll/win32/wzcdlg/wzcdlg.c
 * PURPOSE:          ReactOS Wireless Zero Configuration Dialogs
 * COPYRIGHT:        Copyright 2020 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

/* INCLUDES *****************************************************************/

#include <windef.h>
#include <winbase.h>

/* FUNCTIONS ****************************************************************/

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD     fdwReason,
        LPVOID    lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

/* EOF */
