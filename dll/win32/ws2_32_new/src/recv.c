/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/recv.c
 * PURPOSE:     Socket Receive Support
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
recv(IN SOCKET s,
     OUT CHAR FAR* buf,
     IN INT len,
     IN INT flags)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    WSABUF Buffers;
    DWORD BytesReceived;
    DPRINT("recv: %lx, %lx, %lx, %p\n", s, flags, len, buf);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Setup the buffers */
            Buffers.buf = buf;
            Buffers.len = len;

            /* Make the call */
            Status = Socket->Provider->Service.lpWSPRecv(s,
                                                         &Buffers, 
                                                         1,
                                                         &BytesReceived, 
                                                         (LPDWORD)&flags, 
                                                         NULL,
                                                         NULL, 
                                                         ThreadId, 
                                                         &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS)
            {
                /* Handle OOB */
                if (!(flags & MSG_PARTIAL)) return BytesReceived;
                ErrorCode = WSAEMSGSIZE;
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
recvfrom(IN SOCKET s,
         OUT CHAR FAR* buf,
         IN INT len,
         IN INT flags,
         OUT LPSOCKADDR from,
         IN OUT INT FAR* fromlen)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    WSABUF Buffers;
    DWORD BytesReceived;
    DPRINT("recvfrom: %lx, %lx, %lx, %p\n", s, flags, len, buf);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Setup the buffers */
            Buffers.buf = buf;
            Buffers.len = len;

            /* Make the call */
            Status = Socket->Provider->Service.lpWSPRecvFrom(s,
                                                             &Buffers, 
                                                             1,
                                                             &BytesReceived, 
                                                             (LPDWORD)&flags,
                                                             from,
                                                             fromlen,
                                                             NULL,
                                                             NULL, 
                                                             ThreadId, 
                                                             &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS)
            {
                /* Handle OOB */
                if (!(flags & MSG_PARTIAL)) return BytesReceived;
                ErrorCode = WSAEMSGSIZE;
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
WSARecv(IN SOCKET s,
        IN OUT LPWSABUF lpBuffers,
        IN DWORD dwBufferCount,
        OUT LPDWORD lpNumberOfBytesRecvd,
        IN OUT LPDWORD lpFlags,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    DPRINT("WSARecv: %lx, %lx, %lx, %p\n", s, lpFlags, dwBufferCount, lpBuffers);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPRecv(s,
                                                         lpBuffers, 
                                                         dwBufferCount,
                                                         lpNumberOfBytesRecvd, 
                                                         lpFlags, 
                                                         lpOverlapped,
                                                         lpCompletionRoutine, 
                                                         ThreadId, 
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
WSARecvDisconnect(IN SOCKET s,
                  OUT LPWSABUF lpInboundDisconnectData)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSSOCKET Socket;
    INT ErrorCode;
    INT Status;
    DPRINT("WSARecvDisconnect: %lx %p\n", s, lpInboundDisconnectData);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPRecvDisconnect(s,
                                                                   lpInboundDisconnectData,
                                                                   &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS) return ERROR_SUCCESS;
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
WSARecvFrom(IN SOCKET s,
            IN OUT LPWSABUF lpBuffers,
            IN DWORD dwBufferCount,
            OUT LPDWORD lpNumberOfBytesRecvd,
            IN OUT LPDWORD lpFlags,
            OUT LPSOCKADDR lpFrom,
            IN OUT LPINT lpFromlen,
            IN LPWSAOVERLAPPED lpOverlapped,
            IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    DPRINT("WSARecvFrom: %lx, %lx, %lx, %p\n", s, lpFlags, dwBufferCount, lpBuffers);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPRecvFrom(s,
                                                             lpBuffers, 
                                                             dwBufferCount,
                                                             lpNumberOfBytesRecvd, 
                                                             lpFlags, 
                                                             lpFrom,
                                                             lpFromlen,
                                                             lpOverlapped,
                                                             lpCompletionRoutine,
                                                             ThreadId, 
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
