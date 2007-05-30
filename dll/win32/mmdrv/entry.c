/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/mmdrv/entry.c
 * PURPOSE:              Multimedia User Mode Driver
 * PROGRAMMER:           Andrew Greenwood
 *                       Aleksey Bragin
 * UPDATE HISTORY:
 *                       Jan 30, 2004: Imported into ReactOS tree (Greenwood)
 *                       Mar 16, 2004: Cleaned up a bit (Bragin)
 */


#include "mmdrv.h"

#define NDEBUG
#include <debug.h>

#define EXPORT __declspec(dllexport)

CRITICAL_SECTION DriverSection;

APIENTRY LONG DriverProc(DWORD DriverID, HANDLE DriverHandle, UINT Message,
                LONG Param1, LONG Param2)
{
    DPRINT("DriverProc\n");

//    HINSTANCE Module;

    switch(Message)
    {
        case DRV_LOAD :
            DPRINT("DRV_LOAD\n");
            return TRUE; // dont need to do any more
/*            
            Module = GetDriverModuleHandle(DriverHandle);

        // Create our process heap
        Heap = GetProcessHeap();
        if (Heap == NULL)
            return FALSE;

        DisableThreadLibraryCalls(Module);
        InitializeCriticalSection(&CS);

        //
        // Load our device list
        //

//        if (sndFindDevices() != MMSYSERR_NOERROR) {
//            DeleteCriticalSection(&mmDrvCritSec);
//            return FALSE;
//        }

    return TRUE;
*/
//            return 1L;

        case DRV_FREE :
            DPRINT("DRV_FREE\n");

//            TerminateMidi();
//            TerminateWave();

//            DeleteCriticalSection(&CS);
            return 1L;

        case DRV_OPEN :
            DPRINT("DRV_OPEN\n");
            return 1L;

        case DRV_CLOSE :
            DPRINT("DRV_CLOSE\n");
            return 1L;

        case DRV_ENABLE :
            DPRINT("DRV_ENABLE\n");
            return 1L;

        case DRV_DISABLE :
            DPRINT("DRV_DISABLE\n");
            return 1L;

        case DRV_QUERYCONFIGURE :
            DPRINT("DRV_QUERYCONFIGURE\n");
            return 0L;

        case DRV_CONFIGURE :
            DPRINT("DRV_CONFIGURE\n");
            return 0L;

        case DRV_INSTALL :
            DPRINT("DRV_INSTALL\n");
            return DRVCNF_RESTART;

        default :
            DPRINT("?\n");
            return DefDriverProc(DriverID, DriverHandle, Message, Param1, Param2);
    };
}


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
    DPRINT("DllMain called!\n");

    if (Reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);

        // Create our heap
        Heap = HeapCreate(0, 800, 0);
        if (Heap == NULL)
            return FALSE;

        InitializeCriticalSection(&CS);

        // OK to do this now??        
        FindDevices();

    }
    else if (Reason == DLL_PROCESS_DETACH)
    {
        // We need to do cleanup here...
//        TerminateMidi();
//        TerminateWave();

        DeleteCriticalSection(&CS);
        HeapDestroy(Heap);
    }
    
    return TRUE;
}

/* EOF */
