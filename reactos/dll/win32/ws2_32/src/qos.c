/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/qos.c
 * PURPOSE:     QoS Support
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
BOOL
WSAAPI
WSAGetQOSByName(IN SOCKET s,
                IN OUT LPWSABUF lpQOSName,
                OUT LPQOS lpQOS)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("WSAGetQOSByName: %lx, %p\n", s, lpQOSName);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPGetQOSByName(s,
                                                                 lpQOSName,
                                                                 lpQOS,
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
    return FALSE;
}
