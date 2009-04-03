/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/dllmain.c
 * PURPOSE:         Library main function
 * PROGRAMMER:      ???
 * UPDATE HISTORY:
 *                  Created ???
 */

#include <advapi32.h>
#define NDEBUG
#include <debug.h>

extern BOOL RegInitialize(VOID);
extern BOOL RegCleanup(VOID);
extern VOID UnloadNtMarta(VOID);

BOOL WINAPI
DllMain(HINSTANCE hinstDll,
    DWORD dwReason,
    LPVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDll);
            RegInitialize();
            break;
        case DLL_PROCESS_DETACH:
            RegCleanup();
            UnloadNtMarta();
            break;
    }

   return TRUE;
}

/* EOF */
