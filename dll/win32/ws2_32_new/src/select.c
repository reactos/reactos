/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/select.c
 * PURPOSE:     Socket Select Support
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
WSPAPI
__WSAFDIsSet(SOCKET s,
             LPFD_SET set)
{
    INT i = set->fd_count;
    INT Return = FALSE;

    /* Loop until a match is found */
    while (i--) if (set->fd_array[i] == s) Return = TRUE;

    /* Return */
    return Return;
}

/*
 * @implemented
 */
INT
WSAAPI
select(IN INT s, 
       IN OUT LPFD_SET readfds, 
       IN OUT LPFD_SET writefds, 
       IN OUT LPFD_SET exceptfds, 
       IN CONST struct timeval *timeout)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    SOCKET Handle;
    LPWSPSELECT WSPSelect;

    DPRINT("select: %lx %p %p %p %p\n", s, readfds, writefds, exceptfds, timeout);

    /* Check for WSAStartup */
    ErrorCode = WsQuickProlog();

    if (ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Use the first Socket from the first valid set */
    if (readfds && readfds->fd_count)
    {
        Handle = readfds->fd_array[0];
    }
    else if (writefds && writefds->fd_count)
    {
        Handle = writefds->fd_array[0];
    }
    else if (exceptfds && exceptfds->fd_count)
    {
        Handle = exceptfds->fd_array[0];
    }
    else
    {
        /* Invalid handles */
        SetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    /* Get the Socket Context */
    Socket = WsSockGetSocket(Handle);

    if (!Socket)
    {
        /* No Socket Context Found */
        SetLastError(WSAENOTSOCK);
        return SOCKET_ERROR;
    }

    /* Get the select procedure */
    WSPSelect = Socket->Provider->Service.lpWSPSelect;

    /* Make the call */
    Status = WSPSelect(s, readfds, writefds, exceptfds, (struct timeval *)timeout,
                       &ErrorCode);

    /* Deference the Socket Context */
    WsSockDereference(Socket);

    /* Return Provider Value */
    if (Status != SOCKET_ERROR)
        return Status;

    /* If everything seemed fine, then the WSP call failed itself */
    if (ErrorCode == NO_ERROR)
        ErrorCode = WSASYSCALLFAILURE;

    /* Return with an error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WPUFDIsSet(IN SOCKET s,
           IN LPFD_SET set)
{
    UNIMPLEMENTED;
    return (SOCKET)0;
}

/*
 * @implemented
 */
INT
WSAAPI
WSAAsyncSelect(IN SOCKET s,
               IN HWND hWnd,
               IN UINT wMsg,
               IN LONG lEvent)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("WSAAsyncSelect: %lx, %lx, %lx, %lx\n", s, hWnd, wMsg, lEvent);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPAsyncSelect(s,
                                                                hWnd, 
                                                                wMsg,
                                                                lEvent, 
                                                                &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS) return Status;

            /* If everything seemed fine, then the WSP call failed itself */
            if (ErrorCode == NO_ERROR) ErrorCode = WSASYSCALLFAILURE;
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

/*
 * @implemented
 */
INT
WSAAPI
WSAEventSelect(IN SOCKET s,
               IN WSAEVENT hEventObject,
               IN LONG lNetworkEvents)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPEventSelect(s,
                                                        hEventObject,
                                                        lNetworkEvents,
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
