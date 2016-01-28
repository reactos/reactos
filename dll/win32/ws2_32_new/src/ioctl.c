/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/ioctl.c
 * PURPOSE:     Socket I/O Control Code support.
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
ioctlsocket(IN SOCKET s,
            IN LONG cmd,
            IN OUT ULONG FAR* argp)
{
    DWORD Dummy;

    /* Let WSA do it */
    return WSAIoctl(s,
                    cmd,
                    argp,
                    sizeof(ULONG),
                    argp,
                    sizeof(ULONG),
                    &Dummy,
                    NULL,
                    NULL);
}

/*
 * @implemented
 */
INT
WSAAPI
WSAIoctl(IN SOCKET s,
         IN DWORD dwIoControlCode,
         IN LPVOID lpvInBuffer,
         IN DWORD cbInBuffer,
         OUT LPVOID lpvOutBuffer,
         IN DWORD cbOutBuffer,
         OUT LPDWORD lpcbBytesReturned,
         IN LPWSAOVERLAPPED lpOverlapped,
         IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
    DPRINT("WSAIoctl: %lx, %lx\n", s, dwIoControlCode);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPIoctl(s,
                                                  dwIoControlCode,
                                                  lpvInBuffer,
                                                  cbInBuffer,
                                                  lpvOutBuffer,
                                                  cbOutBuffer,
                                                  lpcbBytesReturned,
                                                  lpOverlapped,
                                                  lpCompletionRoutine,
                                                  ThreadId,
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
