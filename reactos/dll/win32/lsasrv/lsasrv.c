/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/lsasrv.c
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2006-2009 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


/* FUNCTIONS ***************************************************************/

NTSTATUS WINAPI
LsapInitLsa(VOID)
{
    HANDLE hEvent;
    DWORD dwError;

    TRACE("LsapInitLsa() called\n");

    /* Initialize the well known SIDs */
    LsapInitSids();

    /* Initialize the LSA database */
    LsapInitDatabase();

    /* Start the RPC server */
    LsarStartRpcServer();

    TRACE("Creating notification event!\n");
    /* Notify the service manager */
    hEvent = CreateEventW(NULL,
                          TRUE,
                          FALSE,
                          L"LSA_RPC_SERVER_ACTIVE");
    if (hEvent == NULL)
    {
        dwError = GetLastError();
        TRACE("Failed to create the notication event (Error %lu)\n", dwError);

        if (dwError == ERROR_ALREADY_EXISTS)
        {
            hEvent = OpenEventW(GENERIC_WRITE,
                                FALSE,
                                L"LSA_RPC_SERVER_ACTIVE");
            if (hEvent == NULL)
            {
               ERR("Could not open the notification event (Error %lu)\n", GetLastError());
               return STATUS_UNSUCCESSFUL;
            }
        }
    }

    TRACE("Set notification event!\n");
    SetEvent(hEvent);

    /* NOTE: Do not close the event handle!!!! */

    StartAuthenticationPort();

    return STATUS_SUCCESS;
}


NTSTATUS WINAPI
ServiceInit(VOID)
{
    TRACE("ServiceInit() called\n");
    return STATUS_SUCCESS;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, ptr);
}

/* EOF */
