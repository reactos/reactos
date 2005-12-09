/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

BOOLEAN g_fSocketLockInit;
CRITICAL_SECTION RNRPROV_SocketLock;

/* FUNCTIONS *****************************************************************/

VOID
WSPAPI
Rnr_ProcessInit(VOID)
{
    /* Initialize the RnR Locks */
    InitializeCriticalSection(&RNRPROV_SocketLock);
    g_fSocketLockInit = TRUE;
    InitializeCriticalSection(&g_RnrLock);
    g_fRnrLockInit = TRUE;
}

BOOLEAN
WSPAPI
Rnr_ThreadInit(VOID)
{
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;
    PRNR_TEB_DATA RnrThreadData;

    /* Check if we have Thread Data */
    if (!ThreadData)
    {
        /* Initialize the entire DLL */
        if (!MSAFD_SockThreadInitialize()) return FALSE;
    }

    /* Allocate the thread data */
    RnrThreadData = RtlAllocateHeap(RtlGetProcessHeap(),
                                    0,
                                    sizeof(RNR_TEB_DATA));
    if (RnrThreadData)
    {
        /* Zero it out */
        RtlZeroMemory(RnrThreadData, sizeof(RNR_TEB_DATA));

        /* Link it */
        ThreadData->RnrThreadData = RnrThreadData;

        /* Return success */
        return TRUE;
    }

    /* If we got here, we failed */
    return FALSE;
}

VOID
WSPAPI
Rnr_ProcessCleanup(VOID)
{
    /* Check if the RnR Lock is initalized */
    if (g_fRnrLockInit)
    {
        /* It is, so do NSP cleanup */
        Nsp_GlobalCleanup();
        
        /* Free the lock if it's still there */
        if (g_fSocketLockInit) DeleteCriticalSection(&g_RnrLock);
        g_fRnrLockInit = FALSE;
    }

    /* Free the socket lock if it's there */
    if (g_fSocketLockInit) DeleteCriticalSection(&RNRPROV_SocketLock);
    g_fRnrLockInit = FALSE;
}

VOID
WSPAPI
Rnr_ThreadCleanup(VOID)
{
    /* Clean something in the TEB.. */
}