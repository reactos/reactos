/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/bhook.c
 * PURPOSE:     Blocking Hook support for 1.x clients
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
INT
WSAAPI
WSACancelBlockingCall(VOID)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    DPRINT("WSACancelBlockingCall\n");

    /* Call the prolog */
    ErrorCode = WsApiProlog(&Process, &Thread);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Fail */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Make sure this isn't a 2.2 client */
    if (LOBYTE(Process->Version) >= 2)
    {
        /* Only valid for 1.x */
        SetLastError(WSAEOPNOTSUPP);
        return SOCKET_ERROR;
    }

    /* Cancel the call */
    ErrorCode = WsThreadCancelBlockingCall(Thread);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Fail */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Return success */
    return ERROR_SUCCESS;
}

/*
 * @implemented
 */
BOOL
WSAAPI
WSAIsBlocking(VOID)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    DPRINT("WSAIsBlocking\n");

    /* Call the prolog */
    ErrorCode = WsApiProlog(&Process, &Thread);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Fail unless its because we're busy */
        if (ErrorCode != WSAEINPROGRESS) return FALSE;
    }

    /* Return the value from the thread */
    return Thread->Blocking;
}

/*
 * @implemented
 */
FARPROC
WSAAPI
WSASetBlockingHook(IN FARPROC lpBlockFunc)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    DPRINT("WSASetBlockingHook: %p\n", lpBlockFunc);

    /* Call the prolog */
    ErrorCode = WsApiProlog(&Process, &Thread);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Fail */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Make sure this isn't a 2.2 client */
    if (LOBYTE(Process->Version) >= 2)
    {
        /* Only valid for 1.x */
        SetLastError(WSAEOPNOTSUPP);
        return NULL;
    }

    /* Make sure the pointer is safe */
    if (IsBadCodePtr(lpBlockFunc))
    {
        /* Invalid pointer */
        SetLastError(WSAEFAULT);
        return NULL;
    }

    /* Set the blocking hook and return the previous one */
    return WsThreadSetBlockingHook(Thread, lpBlockFunc);
}

/*
 * @implemented
 */
INT
WSAAPI
WSAUnhookBlockingHook(VOID)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    DPRINT("WSAUnhookBlockingHook\n");

    /* Call the prolog */
    ErrorCode = WsApiProlog(&Process, &Thread);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Fail */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Make sure this isn't a 2.2 client */
    if (LOBYTE(Process->Version) >= 2)
    {
        /* Only valid for 1.x */
        SetLastError(WSAEOPNOTSUPP);
        return SOCKET_ERROR;
    }

    /* Set the blocking hook and return the previous one */
    return WsThreadUnhookBlockingHook(Thread);
}
