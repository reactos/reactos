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

BOOL
WSPAPI
WSPGetQOSByName(IN SOCKET Handle, 
                IN OUT LPWSABUF lpQOSName, 
                OUT LPQOS lpQOS, 
                OUT LPINT lpErrno)
{
    PWINSOCK_TEB_DATA ThreadData;
    INT ErrorCode;
    DWORD BytesReturned;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Call WSPIoctl for the job */
    WSPIoctl(Handle,
             SIO_GET_QOS,
             NULL,
             0,
             lpQOS,
             sizeof(QOS),
             &BytesReturned,
             NULL,
             NULL,
             NULL,
             &ErrorCode);

    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return FALSE;
    }

    /* Success */
    return TRUE;
}

INT
WSPAPI
WSPCancelBlockingCall(OUT LPINT lpErrno)
{
    return 0;
}

BOOL
WSPAPI
WSPGetOverlappedResult(IN SOCKET s,
                       IN LPWSAOVERLAPPED lpOverlapped,
                       OUT LPDWORD lpcbTransfer,
                       IN BOOL fWait,
                       OUT LPDWORD lpdwFlags,
                       OUT LPINT lpErrno)
{
    return FALSE;
}

INT
WSPAPI
WSPDuplicateSocket(IN SOCKET s,
                   IN DWORD dwProcessId,
                   OUT LPWSAPROTOCOL_INFOW lpProtocolInfo,
                   OUT LPINT lpErrno)
{
    return 0;
}