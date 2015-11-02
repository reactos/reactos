/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/dupsock.c
 * PURPOSE:     Socket Duplication
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
WSADuplicateSocketA(IN SOCKET s,
                    IN DWORD dwProcessId,
                    OUT LPWSAPROTOCOL_INFOA lpProtocolInfo)
{
    WSAPROTOCOL_INFOW ProtocolInfoW;
    INT ErrorCode;
    DPRINT("WSADuplicateSocketA: %lx, %lx, %p\n", s, dwProcessId, lpProtocolInfo);
  
    /* Call the Unicode Function */
    ErrorCode = WSADuplicateSocketW(s, dwProcessId, &ProtocolInfoW);

    /* Check for success */
    if (ErrorCode == ERROR_SUCCESS)
    {                          
        /* Convert Protocol Info to Ansi */
        if (lpProtocolInfo) 
        {    
            /* Convert the information to ANSI */
            ErrorCode = MapUnicodeProtocolInfoToAnsi(&ProtocolInfoW,
                                                     lpProtocolInfo);
        }
        else
        {
            /* Fail */
            ErrorCode = WSAEFAULT;
        }

        /* Check if the conversion failed */
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Set the last error and normalize the error */
            SetLastError(ErrorCode);
            ErrorCode = SOCKET_ERROR;
        }
    }

    /* Return */
    return ErrorCode;
}

/*
 * @implemented
 */
INT
WSAAPI
WSADuplicateSocketW(IN SOCKET s,
                    IN DWORD dwProcessId,
                    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("WSADuplicateSocketW: %lx, %lx, %p\n", s, dwProcessId, lpProtocolInfo);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPDuplicateSocket(s,
                                                            dwProcessId,
                                                            lpProtocolInfo,
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
