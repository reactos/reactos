/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Storage device properties
 * COPYRIGHT:   2020 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

HINSTANCE hInstance = NULL;


/*
 * @implemented
 */
BOOL
WINAPI
VolumePropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("VolumePropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}


BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hinstDll,
    _In_ DWORD dwReason,
    _In_ LPVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDll);
            hInstance = hinstDll;
            break;

        case DLL_PROCESS_DETACH:
            hInstance = NULL;
            break;
    }

   return TRUE;
}

/* EOF */
