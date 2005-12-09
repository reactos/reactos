/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

BOOLEAN
WINAPI
RNRPROV_SockEnterApi(VOID)
{
    PWINSOCK_TEB_DATA ThreadData;

    /* Make sure we're not terminating */
    if (SockProcessTerminating)
    {
        SetLastError(WSANOTINITIALISED);
        return FALSE;
    }

    /* Check if we already intialized */
    ThreadData = NtCurrentTeb()->WinSockData;
    if (!(ThreadData) || !(ThreadData->RnrThreadData))
    {
        /* Initialize the thread */
        if (!Rnr_ThreadInit())
        {
            /* Fail */
            SetLastError(WSAENOBUFS);
            return FALSE;
        }
    }

    /* Return success */
    return TRUE;
}