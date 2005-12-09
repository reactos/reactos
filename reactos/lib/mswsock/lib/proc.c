/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

PRTL_HEAP_ALLOCATE SockAllocateHeapRoutine;
HANDLE SockPrivateHeap;
CRITICAL_SECTION MSWSOCK_SocketLock;
PWAH_HANDLE_TABLE SockContextTable;

/* FUNCTIONS *****************************************************************/

PVOID
WSPAPI
SockInitializeHeap(IN HANDLE Heap,
                   IN ULONG Flags,
                   IN ULONG Size)
{
    /* Create the heap */
    Heap = RtlCreateHeap(HEAP_GROWABLE,
                         NULL,
                         0,
                         0,
                         NULL,
                         NULL);

    /* Check if we created it successfully */
    if (Heap)
    {
        /* Write its pointer */
        if (InterlockedCompareExchangePointer(&SockPrivateHeap, Heap, NULL))
        {
            /* Someone already allocated it, destroy ours */
            RtlDestroyHeap(Heap);
        }
    }
    else
    {
        /* Write the default heap */
        InterlockedCompareExchangePointer(&SockPrivateHeap,
                                          RtlGetProcessHeap(),
                                          NULL);
    }

    /* Set the reap heap routine now */
    SockAllocateHeapRoutine = RtlAllocateHeap;

    /* Call it */
    return SockAllocateHeapRoutine(SockPrivateHeap, Flags, Size);                        
}

INT
WSPAPI
SockEnterApiSlow(OUT PWINSOCK_TEB_DATA *ThreadData)
{
    /* Check again if we're terminating */
    if (SockProcessTerminating) return WSANOTINITIALISED;

    /* Check if WSPStartup wasn't called */
    if (SockWspStartupCount <= 0) return WSANOTINITIALISED;

    /* Get the thread data */
    *ThreadData = NtCurrentTeb()->WinSockData;
    if (!(*ThreadData))
    {
        /* Try to initialize the thread */
        if (!MSAFD_SockThreadInitialize()) return WSAENOBUFS;

        /* Get the thread data again */
        *ThreadData = NtCurrentTeb()->WinSockData;
    }

    /* Return */
    return NO_ERROR;
}

BOOL
WSPAPI
MSAFD_SockThreadInitialize(VOID)
{
    NTSTATUS Status;
    HANDLE EventHandle;
    PWINSOCK_TEB_DATA TebData;

    /* Initialize the event handle */
    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Allocate the thread data */
    TebData = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(WINSOCK_TEB_DATA));
    if (!TebData) return FALSE;

    /* Set it and zero its contents */
    NtCurrentTeb()->WinSockData = TebData;
    RtlZeroMemory(TebData, sizeof(WINSOCK_TEB_DATA));

    /* Set the event handle */
    TebData->EventHandle = EventHandle;

    /* Return success */
    return TRUE;
}
