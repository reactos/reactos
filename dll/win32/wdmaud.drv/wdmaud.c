/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wdmaud.drv/wdmaud.c
 *
 * PURPOSE:     WDM Audio Driver (User-mode part)
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
 */

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>

LONG
APIENTRY
DriverProc(
    DWORD DriverId,
    HANDLE DriverHandle,
    UINT Message,
    LONG Parameter1,
    LONG Parameter2)
{
    switch ( Message )
    {
        case DRV_LOAD :
        {
            MMRESULT Result;
#ifndef USE_MMIXER_LIB
            PWDMAUD_DEVICE_INFO DeviceInfo;
#endif
            DPRINT("DRV_LOAD\n");

            Result = InitEntrypointMutexes();

            if ( ! MMSUCCESS(Result) )
                return 0L;

            Result = FUNC_NAME(WdmAudOpenKernelSoundDevice)();

            if ( Result != MMSYSERR_NOERROR )
            {
                DPRINT1("Failed to open \\\\.\\wdmaud with %d\n", GetLastError());

                return 0L;
            }

#ifndef USE_MMIXER_LIB
            DeviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_INFO));
            if (!DeviceInfo)
            {
                /* No memory */
                DPRINT1("Failed to allocate WDMAUD_DEVICE_INFO structure\n");
                return 0L;
            }

            /* Initialize wdmaud.sys */
            DeviceInfo->DeviceType = AUX_DEVICE_TYPE;

            Result = WdmAudIoControl(DeviceInfo, 0, NULL, IOCTL_INIT_WDMAUD);
            HeapFree(GetProcessHeap(), 0, DeviceInfo);
            if ( ! MMSUCCESS( Result ) )
            {
                DPRINT1("Call to IOCTL_INIT_WDMAUD failed with %d\n", GetLastError());

                return 0L;
            }
#endif

            DPRINT("Initialization completed\n");

            return 1L;
        }

        case DRV_FREE :
        {
#ifndef USE_MMIXER_LIB
            MMRESULT Result;
            PWDMAUD_DEVICE_INFO DeviceInfo;
#endif
            DPRINT("DRV_FREE\n");
#ifndef USE_MMIXER_LIB
            DeviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_INFO));
            if (!DeviceInfo)
            {
                /* No memory */
                DPRINT1("Failed to allocate WDMAUD_DEVICE_INFO structure\n");
                return 0L;
            }

            /* Unload wdmaud.sys */
            DeviceInfo->DeviceType = AUX_DEVICE_TYPE;

            Result = WdmAudIoControl(DeviceInfo, 0, NULL, IOCTL_EXIT_WDMAUD);
            HeapFree(GetProcessHeap(), 0, DeviceInfo);
            if ( ! MMSUCCESS( Result ) )
            {
                DPRINT1("Call to IOCTL_EXIT_WDMAUD failed with %d\n", GetLastError());

                return 0L;
            }
#endif

            FUNC_NAME(WdmAudCleanup)();

            CleanupEntrypointMutexes();

            return 1L;
        }

        case DRV_ENABLE :
        case DRV_DISABLE :
        {
            DPRINT("DRV_ENABLE / DRV_DISABLE\n");
            return 1L;
        }

        case DRV_OPEN :
        case DRV_CLOSE :
        {
            DPRINT("DRV_OPEN / DRV_CLOSE\n");
            return 1L;
        }

        case DRV_QUERYCONFIGURE :
        {
            DPRINT("DRV_QUERYCONFIGURE\n");
            return 0L;
        }
        case DRV_CONFIGURE :
            return DRVCNF_OK;

        default :
            DPRINT("Unhandled message %d\n", Message);
            return DefDriverProc(DriverId,
                                 DriverHandle,
                                 Message,
                                 Parameter1,
                                 Parameter2);
    }
}


BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch ( fdwReason )
    {
        case DLL_PROCESS_ATTACH :
            DPRINT("WDMAUD.DRV - Process attached\n");
            break;
        case DLL_PROCESS_DETACH :
            DPRINT("WDMAUD.DRV - Process detached\n");
            break;
        case DLL_THREAD_ATTACH :
            DPRINT("WDMAUD.DRV - Thread attached\n");
            break;
        case DLL_THREAD_DETACH :
            DPRINT("WDMAUD.DRV - Thread detached\n");
            break;
    }

    return TRUE;
}
