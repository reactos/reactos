/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Imagehlp Libary
 * FILE:            lib/imagehlp/imagehlp_main.c
 * PURPOSE:         DLL Entrypoint
 * PROGRAMMER:      Patrik Stridvall
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

//#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

HANDLE IMAGEHLP_hHeap = NULL;

/* FUNCTIONS *****************************************************************/

BOOL
IMAGEAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            IMAGEHLP_hHeap = HeapCreate(0, 0x10000, 0);
            break;
        case DLL_PROCESS_DETACH:
            HeapDestroy(IMAGEHLP_hHeap);
            IMAGEHLP_hHeap = NULL;
            break;
        default:
            break;
    }
    
    return TRUE;
}
