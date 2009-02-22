/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wdmaud.drv/wdmaud.c
 *
 * PURPOSE:     WDM Audio Driver (User-mode part)
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
 *
 * NOTES:       Looking for wodMessage & co? You won't find them here. Try
 *              the MME Buddy library, which is where these routines are
 *              actually implemented.
 *
 */

#include <windows.h>
#include <ntddsnd.h>
#include <sndtypes.h>
#include <mmddk.h>
#include <mmebuddy.h>

#define KERNEL_DEVICE_NAME      L"\\\\Device\\wdmaud"

HANDLE KernelHandle = INVALID_HANDLE_VALUE;

APIENTRY LONG
DriverProc(
    DWORD DriverId,
    HANDLE DriverHandle,
    UINT Message,
    LONG Parameter1,
    LONG Parameter2)
{
    MMRESULT Result;

    switch ( Message )
    {
        case DRV_LOAD :
        {
            SND_TRACE(L"DRV_LOAD\n");

            Result = InitEntrypointMutexes();

            if ( ! MMSUCCESS(Result) )
                return 0L;

            KernelHandle = CreateFile(KERNEL_DEVICE_NAME,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_WRITE, // ok?
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OVERLAPPED,
                                      NULL);

            if ( KernelHandle == INVALID_HANDLE_VALUE )
            {
                SND_ERR(L"Failed to open %s\n", KERNEL_DEVICE_NAME);
                CleanupEntrypointMutexes();

                UnlistAllSoundDevices();

                return 0L;
            }

            SND_TRACE(L"Initialisation complete\n");

            return 1L;
        }

        case DRV_FREE :
        {
            SND_TRACE(L"DRV_FREE\n");

            if ( KernelHandle != INVALID_HANDLE_VALUE )
            {
                CloseHandle(KernelHandle);
                KernelHandle = INVALID_HANDLE_VALUE;
            }

            /* TODO: Clean up the path names! */
            UnlistAllSoundDevices();

            CleanupEntrypointMutexes();

            SND_TRACE(L"Unfreed memory blocks: %d\n",
                      GetMemoryAllocationCount());

            return 1L;
        }

        case DRV_ENABLE :
        case DRV_DISABLE :
        {
            SND_TRACE(L"DRV_ENABLE / DRV_DISABLE\n");
            return 1L;
        }

        case DRV_OPEN :
        case DRV_CLOSE :
        {
            SND_TRACE(L"DRV_OPEN / DRV_CLOSE\n");
            return 1L;
        }

        case DRV_QUERYCONFIGURE :
        {
            SND_TRACE(L"DRV_QUERYCONFIGURE\n");
            return 0L;
        }
        case DRV_CONFIGURE :
            return DRVCNF_OK;

        default :
            SND_TRACE(L"Unhandled message %d\n", Message);
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
            SND_TRACE(L"WDMAUD.DRV - Process attached\n");
            break;
        case DLL_PROCESS_DETACH :
            SND_TRACE(L"WDMAUD.DRV - Process detached\n");
            break;
        case DLL_THREAD_ATTACH :
            SND_TRACE(L"WDMAUD.DRV - Thread attached\n");
            break;
        case DLL_THREAD_DETACH :
            SND_TRACE(L"WDMAUD.DRV - Thread detached\n");
            break;
    }

    return TRUE;
}
