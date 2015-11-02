/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/send.c
 * PURPOSE:     Socket Sending Support.
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
INT
WSAAPI
send(IN SOCKET s, 
     IN CONST CHAR FAR* buf, 
     IN INT len, 
     IN INT flags)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    WSABUF Buffers;
    DWORD BytesSent;
    DPRINT("send: %lx, %lx, %lx, %p\n", s, flags, len, buf);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Setup the buffers */
            Buffers.buf = (PCHAR)buf;
            Buffers.len = len;

            /* Make the call */
            Status = Socket->Provider->Service.lpWSPSend(s,
                                                         &Buffers, 
                                                         1,
                                                         &BytesSent, 
                                                         (DWORD)flags, 
                                                         NULL,
                                                         NULL, 
                                                         ThreadId, 
                                                         &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS) return BytesSent;

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
sendto(IN SOCKET s,
       IN CONST CHAR FAR* buf,
       IN INT len,
       IN INT flags,
       IN CONST struct sockaddr *to, 
       IN INT tolen)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    WSABUF Buffers;
    DWORD BytesSent;
    DPRINT("send: %lx, %lx, %lx, %p\n", s, flags, len, buf);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Setup the buffers */
            Buffers.buf = (PCHAR)buf;
            Buffers.len = len;

            /* Make the call */
            Status = Socket->Provider->Service.lpWSPSendTo(s,
                                                           &Buffers, 
                                                           1,
                                                           &BytesSent, 
                                                           (DWORD)flags,
                                                           to,
                                                           tolen,
                                                           NULL,
                                                           NULL, 
                                                           ThreadId, 
                                                           &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS) return BytesSent;

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
WSASend(IN SOCKET s,
        IN LPWSABUF lpBuffers,
        IN DWORD dwBufferCount,
        OUT LPDWORD lpNumberOfBytesSent,
        IN DWORD dwFlags,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    DPRINT("WSARecvFrom: %lx, %lx, %lx, %p\n", s, dwFlags, dwBufferCount, lpBuffers);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPSend(s,
                                                         lpBuffers, 
                                                         dwBufferCount,
                                                         lpNumberOfBytesSent, 
                                                         dwFlags, 
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
WSASendDisconnect(IN SOCKET s,
                  IN LPWSABUF lpOutboundDisconnectData)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSSOCKET Socket;
    INT ErrorCode;
    INT Status;
    DPRINT("WSASendDisconnect: %lx %p\n", s, lpOutboundDisconnectData);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPSendDisconnect(s,
                                                                   lpOutboundDisconnectData,
                                                                   &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS) return ERROR_SUCCESS;

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
WSASendTo(IN SOCKET s,
          IN LPWSABUF lpBuffers,
          IN DWORD dwBufferCount,
          OUT LPDWORD lpNumberOfBytesSent,
          IN DWORD dwFlags,
          IN CONST struct sockaddr *lpTo,
          IN INT iToLen,
          IN LPWSAOVERLAPPED lpOverlapped,
          IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    DPRINT("WSASendTo: %lx, %lx, %lx, %p\n", s, dwFlags, dwBufferCount, lpBuffers);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPSendTo(s,
                                                           lpBuffers, 
                                                           dwBufferCount,
                                                           lpNumberOfBytesSent, 
                                                           dwFlags, 
                                                           lpTo,
                                                           iToLen,
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
