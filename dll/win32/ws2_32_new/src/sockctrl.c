/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        socktrl.c
 * PURPOSE:     Socket Control/State Support
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#include "ws2_32.h"

//#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
INT
WSAAPI
connect(IN SOCKET s,
        IN CONST struct sockaddr *name,
        IN INT namelen)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSSOCKET Socket;
    INT ErrorCode, OldErrorCode;
    INT Status;
    BOOLEAN TryAgain = TRUE;
    DPRINT("connect: %lx, %p, %lx\n", s, name, namelen);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            while (TRUE)
            {
                /* Make the call */
                Status = Socket->Provider->Service.lpWSPConnect(s,
                                                                name,
                                                                namelen,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                &ErrorCode);

                /* Check if error code was due to the host not being found */
                if ((Status == SOCKET_ERROR) &&
                    (ErrorCode == WSAEHOSTUNREACH) &&
                    (ErrorCode == WSAENETUNREACH))
                {
                    /* Check if we can try again */
                    if (TryAgain)
                    {
                        /* Save the old error code */
                        OldErrorCode = ErrorCode;

                        /* Make sure we don't retry 3 times */
                        TryAgain = FALSE;

                        /* Make the RAS Auto-dial attempt */
                        if (WSAttemptAutodialAddr(name, namelen)) continue;
                    }
                    else
                    {
                        /* Restore the error code */
                        ErrorCode = OldErrorCode;
                    }
                }

                /* Break out of the loop */
                break;
            }

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

    /* If this is Winsock 1.1, normalize the error code */
    if ((ErrorCode == WSAEALREADY) && (LOBYTE(Process->Version) == 1))
    {
        /* WS 1.1 apps expect this */
        ErrorCode = WSAEINVAL;
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
listen(IN SOCKET s,
       IN INT backlog)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("connect: %lx, %lx\n", s, backlog);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPListen(s,
                                                           backlog,
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
getpeername(IN SOCKET s,
            OUT LPSOCKADDR name,
            IN OUT INT FAR* namelen)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("getpeername: %lx, %p, %lx\n", s, name, namelen);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPGetPeerName(s,
                                                                name,
                                                                namelen,
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
getsockname(IN SOCKET s,
            OUT LPSOCKADDR name,
            IN OUT INT FAR* namelen)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("getsockname: %lx, %p, %lx\n", s, name, namelen);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPGetSockName(s,
                                                                name,
                                                                namelen,
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
getsockopt(IN SOCKET s,
           IN INT level,
           IN INT optname,
           OUT CHAR FAR* optval,
           IN OUT INT FAR* optlen)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSSOCKET Socket;
    INT ErrorCode;
    INT Status;
    WSAPROTOCOL_INFOW ProtocolInfo;
    PCHAR OldOptVal = NULL;
    INT OldOptLen = 0;
    DPRINT("getsockopt: %lx, %lx, %lx\n", s, level, optname);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Check if we're getting the open type */
        if ((level == SOL_SOCKET) && (optname == SO_OPENTYPE))
        {
            /* Validate size */
            if (!(optlen) || (*optlen < sizeof(INT)))
            {
                /* Fail */
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }

            /* Set the open type */
            *optval = (CHAR)Thread->OpenType;
            *optlen = sizeof(INT);
            return ERROR_SUCCESS;
        }

        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Check if ANSI data was requested */
            if ((level == SOL_SOCKET) && (optname == SO_PROTOCOL_INFOA))
            {
                /* Validate size and pointers */
                if(!(optval) ||
                   !(optlen) ||
                   (*optlen < sizeof(WSAPROTOCOL_INFOA)))
                {
                    /* Set return size */
                    *optlen = sizeof(WSAPROTOCOL_INFOA);

                    /* Dereference the socket and fail */
                    WsSockDereference(Socket);
                    SetLastError(WSAEFAULT);
                    return SOCKET_ERROR;
                }

                /* It worked. Save the values */
                OldOptLen = *optlen;
                OldOptVal = optval;

                /* Hack them so WSP will know how to deal with it */
                *optlen = sizeof(WSAPROTOCOL_INFOW);
                optval = (PCHAR)&ProtocolInfo;
                optname = SO_PROTOCOL_INFOW;
            }

            /* Make the call */
            Status = Socket->Provider->Service.lpWSPGetSockOpt(s,
                                                               level,
                                                               optname,
                                                               optval,
                                                               optlen,
                                                               &ErrorCode);

            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Check provider value */
            if (Status == ERROR_SUCCESS)
            {
                /* Did we use the A->W hack? */
                if (!OldOptVal) return Status;

                /* We did, so we have to convert the unicode info to ansi */
                ErrorCode = MapUnicodeProtocolInfoToAnsi(&ProtocolInfo,
                                                         (LPWSAPROTOCOL_INFOA)
                                                         OldOptVal);

                /* Return the length */
                *optlen = OldOptLen;

                /* Return success if this worked */
                if (ErrorCode == ERROR_SUCCESS) return Status;
            }

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
setsockopt(IN SOCKET s,
           IN INT level,
           IN INT optname,
           IN CONST CHAR FAR* optval,
           IN INT optlen)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSSOCKET Socket;
    INT ErrorCode;
    INT Status;
    DPRINT("setsockopt: %lx, %lx, %lx\n", s, level, optname);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Check if we're changing the open type */
        if (level == SOL_SOCKET && optname == SO_OPENTYPE)
        {
            /* Validate size */
            if (optlen < sizeof(INT))
            {
                /* Fail */
                SetLastError(WSAEFAULT);
                return SOCKET_ERROR;
            }

            /* Set the open type */
            Thread->OpenType = *optval;
            return ERROR_SUCCESS;
        }

        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPSetSockOpt(s,
                                                               level,
                                                               optname,
                                                               optval,
                                                               optlen,
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
shutdown(IN SOCKET s,
         IN INT how)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("shutdown: %lx, %lx\n", s, how);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPShutdown(s, how, &ErrorCode);

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
WSAConnect(IN SOCKET s,
           IN CONST struct sockaddr *name,
           IN INT namelen,
           IN LPWSABUF lpCallerData,
           OUT LPWSABUF lpCalleeData,
           IN LPQOS lpSQOS,
           IN LPQOS lpGQOS)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("WSAConnect: %lx, %lx, %lx, %p\n", s, name, namelen, lpCallerData);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPConnect(s,
                                                            name,
                                                            namelen,
                                                            lpCallerData,
                                                            lpCalleeData,
                                                            lpSQOS,
                                                            lpGQOS,
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
BOOL
WSAAPI
WSAGetOverlappedResult(IN SOCKET s,
                       IN LPWSAOVERLAPPED lpOverlapped,
                       OUT LPDWORD lpcbTransfer,
                       IN BOOL fWait,
                       OUT LPDWORD lpdwFlags)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("WSAGetOverlappedResult: %lx, %lx\n", s, lpOverlapped);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPGetOverlappedResult(s,
                                                                        lpOverlapped,
                                                                        lpcbTransfer,
                                                                        fWait,
                                                                        lpdwFlags,
                                                                        &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status) return Status;
        }
        else
        {
            /* No Socket Context Found */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with an Error */
    SetLastError(ErrorCode);
    return FALSE;
}
