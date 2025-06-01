/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/dllmain.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Ariadne (ariadne@xs4all.nl)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static BOOL DllInitialized = FALSE;

/* FUNCTIONS *****************************************************************/

BOOL
WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    DPRINT("DllMain(hInst %p, dwReason %lu)\n",
           hDll, dwReason);
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            /* Insert more dll attach stuff here! */
            DllInitialized = TRUE;
            break;
        }

        case DLL_PROCESS_DETACH:
        {
        }
        default:
            break;
    }

    return TRUE;
}

/* EOF */
