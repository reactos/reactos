/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WinSock DLL
 * FILE:            stubs.c
 * PURPOSE:         WSAIoctl wrappers for Microsoft extensions to Winsock
 * PROGRAMMERS:     KJK::Hyperion <hackbunny@reactos.com>
 * REVISIONS:
 */

#include "precomp.h"

#include <winsock2.h>
#include <mswsock.h>

LPFN_TRANSMITFILE pfnTransmitFile = NULL;
LPFN_GETACCEPTEXSOCKADDRS pfnGetAcceptExSockaddrs = NULL;
LPFN_ACCEPTEX pfnAcceptEx = NULL;
/*
 * @implemented
 */
BOOL
WINAPI
TransmitFile(SOCKET Socket,
             HANDLE File,
             DWORD NumberOfBytesToWrite,
             DWORD NumberOfBytesPerSend,
             LPOVERLAPPED Overlapped,
             LPTRANSMIT_FILE_BUFFERS TransmitBuffers,
             DWORD Flags)
{
    GUID TransmitFileGUID = WSAID_TRANSMITFILE;
    DWORD cbBytesReturned;

    if (WSAIoctl(Socket,
                 SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &TransmitFileGUID,
                 sizeof(TransmitFileGUID),
                 &pfnTransmitFile,
                 sizeof(pfnTransmitFile),
                 &cbBytesReturned,
                 NULL,
                 NULL) == SOCKET_ERROR)
    {
        return FALSE;
    }

    return pfnTransmitFile(Socket,
                           File,
                           NumberOfBytesToWrite,
                           NumberOfBytesPerSend,
                           Overlapped,
                           TransmitBuffers,
                           Flags);
}

/*
* @implemented
*/
BOOL
WINAPI
AcceptEx(SOCKET ListenSocket,
         SOCKET AcceptSocket,
         PVOID OutputBuffer,
         DWORD ReceiveDataLength,
         DWORD LocalAddressLength,
         DWORD RemoteAddressLength,
         LPDWORD BytesReceived,
         LPOVERLAPPED Overlapped)
{
    GUID AcceptExGUID = WSAID_ACCEPTEX;
    GUID GetAcceptExSockaddrsGUID = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD cbBytesReturned;

    if (WSAIoctl(ListenSocket,
                 SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &AcceptExGUID,
                 sizeof(AcceptExGUID),
                 &pfnAcceptEx,
                 sizeof(pfnAcceptEx),
                 &cbBytesReturned,
                 NULL,
                 NULL) == SOCKET_ERROR)
    {
        return FALSE;
    }

    if (WSAIoctl(ListenSocket,
                 SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &GetAcceptExSockaddrsGUID,
                 sizeof(GetAcceptExSockaddrsGUID),
                 &pfnGetAcceptExSockaddrs,
                 sizeof(pfnGetAcceptExSockaddrs),
                 &cbBytesReturned,
                 NULL,
                 NULL) == SOCKET_ERROR)
    {
        pfnAcceptEx = NULL;
        return FALSE;
    }

    return pfnAcceptEx(ListenSocket,
                       AcceptSocket,
                       OutputBuffer,
                       ReceiveDataLength,
                       LocalAddressLength,
                       RemoteAddressLength,
                       BytesReceived,
                       Overlapped);
}


/*
* @implemented
*/
VOID
WINAPI
GetAcceptExSockaddrs(PVOID OutputBuffer,
                     DWORD ReceiveDataLength,
                     DWORD LocalAddressLength,
                     DWORD RemoteAddressLength,
                     LPSOCKADDR* LocalSockaddr,
                     LPINT LocalSockaddrLength,
                     LPSOCKADDR* RemoteSockaddr,
                     LPINT RemoteSockaddrLength)
{
    if (pfnGetAcceptExSockaddrs)
    {
        pfnGetAcceptExSockaddrs(OutputBuffer,
                                ReceiveDataLength,
                                LocalAddressLength,
                                RemoteAddressLength,
                                LocalSockaddr,
                                LocalSockaddrLength,
                                RemoteSockaddr,
                                RemoteSockaddrLength);
    }
}
/* EOF */
