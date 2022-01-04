/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        dll/win32/ws2help/dllmain.c
 * PURPOSE:     WinSock 2 DLL header
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

/* DATA **********************************************************************/

HANDLE GlobalHeap;
BOOL Ws2helpInitialized = FALSE;
CRITICAL_SECTION StartupSynchronization;
HINSTANCE LibraryHdl;

/* FUNCTIONS *****************************************************************/

VOID
WINAPI
NewCtxInit(VOID)
{
    NT_PRODUCT_TYPE ProductType = NtProductWinNt;
    SYSTEM_INFO SystemInfo;
    DWORD NumHandleBuckets;
    HKEY KeyHandle;
    DWORD RegSize = sizeof(DWORD);
    DWORD RegType;
    DWORD Mask;

    /* Try to figure out if this is a workstation or server install */
    RtlGetNtProductType(&ProductType);

    /* Get the system info */
    GetSystemInfo(&SystemInfo);

    /* If this is an MP machine, set the default spinlock */
    if (SystemInfo.dwNumberOfProcessors > 1) gdwSpinCount = 2000;

    /* Figure how many "Handle Buckets" we'll use. Start with the default */
    NumHandleBuckets = ProductType == NtProductWinNt ? 8 : 32;

    /* Open the registry settings */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "System\\CurrentControlSet\\Services\\Winsock2\\Parameters",
                     0,
                     KEY_QUERY_VALUE,
                     &KeyHandle) == ERROR_SUCCESS)
    {
        /* Query the key */
        RegQueryValueEx(KeyHandle,
                        "Ws2_32NumHandleBuckets",
                        0,
                        &RegType,
                        (LPBYTE)&NumHandleBuckets,
                        &RegSize);

        /* Are we on MP? */
        if (SystemInfo.dwNumberOfProcessors > 1)
        {
            /* Also check for a custom spinlock setting */
            RegQueryValueEx(KeyHandle,
                            "Ws2_32SpinCount",
                            0,
                            &RegType,
                            (LPBYTE)&gdwSpinCount,
                            &RegSize);
        }

        /* Close the key, we're done */
        RegCloseKey(KeyHandle);
    }

    /* Now get the bucket count and normalize it to be log2 and within 256 */
    for (Mask = 256; !(Mask & NumHandleBuckets); Mask >>= 1);
    NumHandleBuckets = Mask;

    /* Normalize it again, to be within OS parameters */
    if (ProductType == NtProductWinNt)
    {
        /* Is it within norms for non-server editions? */
        if (NumHandleBuckets > 32) NumHandleBuckets = 32;
        else if (NumHandleBuckets < 8) NumHandleBuckets = 8;
    }
    else
    {
        /* Is it within norms for server editions? */
        if (NumHandleBuckets > 256) NumHandleBuckets = 256;
        else if (NumHandleBuckets < 32) NumHandleBuckets = 32;
    }

    /* Normalize the spincount */
    if (gdwSpinCount > 8000) gdwSpinCount = 8000;

    /* Set the final mask */
    gHandleToIndexMask = NumHandleBuckets -1;
}

DWORD
WINAPI
Ws2helpInitialize(VOID)
{
    /* Enter the startup CS */
    EnterCriticalSection(&StartupSynchronization);

    /* Check again for init */
    if (!Ws2helpInitialized)
    {
        /* Initialize us */
        NewCtxInit();
        Ws2helpInitialized = TRUE;
    }

    /* Leave the CS and return */
    LeaveCriticalSection(&StartupSynchronization);
    return ERROR_SUCCESS;
}

BOOL
APIENTRY
DllMain(HANDLE hModule,
        DWORD  dwReason,
        LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:

            /* Save our handle */
            LibraryHdl = hModule;

            /* Improve Performance */
            DisableThreadLibraryCalls(hModule);

            /* Initialize startup CS */
            InitializeCriticalSection(&StartupSynchronization);

            /* Get Global Heap */
            GlobalHeap = GetProcessHeap();
        	break;

        case DLL_THREAD_ATTACH:
	    case DLL_THREAD_DETACH:
            break;

	    case DLL_PROCESS_DETACH:

            /* Make sure we loaded */
            if (!LibraryHdl) break;

            /* Check if we are cleaning up */
            if (lpReserved)
            {
                /* Free the security descriptor */
                if (pSDPipe) HeapFree(GlobalHeap, 0, pSDPipe);

                /* Close the event */
                if (ghWriterEvent) CloseHandle(ghWriterEvent);

                /* Delete the startup CS */
                DeleteCriticalSection(&StartupSynchronization);
                Ws2helpInitialized = FALSE;
            }
			break;
    }

    return TRUE;
}
