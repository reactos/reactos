/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/event.c
 * PURPOSE:     Socket Events
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WSAAPI
WSACloseEvent(IN WSAEVENT hEvent)
{
    BOOL Success;

    /* Let the OS handle it */
    Success = CloseHandle(hEvent);

    /* We need a special WSA return error */
    if (!Success) WSASetLastError(WSA_INVALID_HANDLE);

    /* Return the Win32 Error */
    return Success;
}

/*
 * @implemented
 */
WSAEVENT
WSAAPI
WSACreateEvent(VOID)
{
    /* CreateEventW can only return the Event or 0 (WSA_INVALID_EVENT) */
    return CreateEventW(NULL, TRUE, FALSE, NULL);
}

/*
 * @implemented
 */
BOOL
WSAAPI
WSAResetEvent(IN WSAEVENT hEvent)
{
    /* Call Win32 */
    return ResetEvent(hEvent);
}

/*
 * @implemented
 */
BOOL
WSAAPI
WSASetEvent(IN WSAEVENT hEvent)
{
    /* Call Win32 */
    return SetEvent(hEvent);
}

/*
 * @implemented
 */
DWORD
WSAAPI
WSAWaitForMultipleEvents(IN DWORD cEvents,
                         IN CONST WSAEVENT FAR* lphEvents,
                         IN BOOL fWaitAll,
                         IN DWORD dwTimeout,
                         IN BOOL fAlertable)
{
    /* Call Win32 */
    return WaitForMultipleObjectsEx(cEvents, 
                                    lphEvents, 
                                    fWaitAll, 
                                    dwTimeout, 
                                    fAlertable);
}

/*
 * @implemented
 */
INT
WSAAPI
WSAEnumNetworkEvents(IN SOCKET s,
                     IN WSAEVENT hEventObject,
                     OUT LPWSANETWORKEVENTS lpNetworkEvents)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("WSAEnumNetworkEvents: %lx\n", s);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPEnumNetworkEvents(s,
                                                              hEventObject,
                                                              lpNetworkEvents,
                                                              &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS) return Status;
        }
        else
        {
            /* No Socket Context Found */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with an Error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

