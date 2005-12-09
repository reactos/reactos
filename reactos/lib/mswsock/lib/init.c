/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

BOOL SockProcessTerminating;
LONG SockProcessPendingAPCCount;
HINSTANCE SockModuleHandle;

/* FUNCTIONS *****************************************************************/

BOOL
WSPAPI
MSWSOCK_Initialize(VOID)
{
    SYSTEM_INFO SystemInfo;

    /* If our heap is already initialized, we can skip everything */
    if (SockAllocateHeapRoutine) return TRUE;

    /* Make sure nobody thinks we're terminating */
    SockProcessTerminating = FALSE;

    /* Get the system information */
    GetSystemInfo(&SystemInfo);

    /* Check if this is an MP machine */
    if (SystemInfo.dwNumberOfProcessors > 1)
    {
        /* Use our own heap on MP, to reduce locks */
        SockAllocateHeapRoutine = SockInitializeHeap;
        SockPrivateHeap = NULL;
    }
    else
    {
        /* Use process heap */
        SockAllocateHeapRoutine = RtlAllocateHeap;
        SockPrivateHeap = RtlGetProcessHeap();
    }

    /* Initialize WSM data */
    gWSM_NSPStartupRef = -1;
    gWSM_NSPCallRef = 0;

    /* Initialize the helper listhead */
    InitializeListHead(&SockHelperDllListHead);

    /* Initialize the global lock */
    SockInitializeRwLockAndSpinCount(&SocketGlobalLock, 1000);

    /* Initialize the socket lock */
    InitializeCriticalSection(&MSWSOCK_SocketLock);

    /* Initialize RnR locks and other RnR data */
    Rnr_ProcessInit();
    
    /* Return success */
    return TRUE;
}

BOOL 
APIENTRY 
DllMain(HANDLE hModule, 
        DWORD dwReason, 
        LPVOID lpReserved)
{
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;
    PWINSOCK_TEB_DATA ThreadData;

    /* Check what's going on */
    switch (dwReason)
    {
        /* Process attaching */
        case DLL_PROCESS_ATTACH:

            /* Save module handles */
            SockModuleHandle = hModule;
            NlsMsgSourcemModuleHandle = hModule;

            /* Initialize us */
            MSWSOCK_Initialize();
            break;

        /* Detaching */
        case DLL_PROCESS_DETACH:
            
            /* Did we initialize yet? */
            if (!SockAllocateHeapRoutine) break;

            /* Fail all future calls */
            SockProcessTerminating = TRUE;

            /* Is this a FreeLibrary? */
            if (!lpReserved)
            {
                /* Cleanup RNR */
                Rnr_ProcessCleanup();

                /* Delete the socket lock */
                DeleteCriticalSection(&MSWSOCK_SocketLock);

                /* Check if we have an Async Queue Port */
                if (SockAsyncQueuePort)
                {
                    /* Unprotect the handle */
                    HandleInfo.ProtectFromClose = FALSE;
                    HandleInfo.Inherit = FALSE;
                    NtSetInformationObject(SockAsyncQueuePort,
                                           ObjectHandleInformation,
                                           &HandleInfo,
                                           sizeof(HandleInfo));

                    /* Close it, and clear the port */
                    NtClose(SockAsyncQueuePort);
                    SockAsyncQueuePort = NULL;
                }

                /* Check if we have a context table */
                if (SockContextTable)
                {
                    /* Destroy it */
                    WahDestroyHandleContextTable(SockContextTable);
                    SockContextTable = NULL;
                }

                /* Delete the global lock as well */
                SockDeleteRwLock(&SocketGlobalLock);

                /* Check if we have a buffer keytable */
                if (SockBufferKeyTable)
                {
                    /* Free it */
                    VirtualFree(SockBufferKeyTable, 0, MEM_RELEASE);
                }
            }

            /* Check if we have to a SAN cleanup event */
            if (SockSanCleanUpCompleteEvent)
            {
                /* Close the event handle */
                CloseHandle(SockSanCleanUpCompleteEvent);
            }

        /* Thread detaching */
        case DLL_THREAD_DETACH:

            /* Set the context to NULL for thread detach */
            if (dwReason == DLL_THREAD_DETACH) lpReserved = NULL;
            
            /* Check if this is a normal thread detach */
            if (!lpReserved)
            {
                /* Do RnR Thread cleanup */
                Rnr_ThreadCleanup();

                /* Get thread data */
                ThreadData = NtCurrentTeb()->WinSockData;
                if (ThreadData)
                {
                    /* Check if any APCs are pending */
                    if (ThreadData->PendingAPCs)
                    {
                        /* Save the value */
                        InterlockedExchangeAdd(&SockProcessPendingAPCCount,
                                               -(ThreadData->PendingAPCs));

                        /* Close the evnet handle */
                        NtClose(ThreadData->EventHandle);

                        /* Free the thread data and set it to null */
                        RtlFreeHeap(GetProcessHeap(), 0, (PVOID)ThreadData);
                        NtCurrentTeb()->WinSockData = NULL;
                    }
                }
            }

            /* Check if this is a process detach fallthrough */
            if (dwReason == DLL_PROCESS_DETACH && !lpReserved)
            {
                /* Check if we're using a private heap */
                if (SockPrivateHeap != RtlGetProcessHeap())
                {
                    /* Destroy it */
                    RtlDestroyHeap(SockPrivateHeap);
                }
                SockAllocateHeapRoutine = NULL;
            }
            break;

        case DLL_THREAD_ATTACH:
            break;
    }

    /* Return */
    return TRUE;
}