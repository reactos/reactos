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

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(mswsock);

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
    GUID  TransmitFileGUID = WSAID_TRANSMITFILE;
    DWORD cbBytesReturned;
    BOOL  Ret;

    TRACE("TransmitFile %p %p %ld %ld %p %p %lx\n", Socket, File, NumberOfBytesToWrite, NumberOfBytesPerSend, Overlapped, TransmitBuffers, Flags);
    if (!pfnTransmitFile && WSAIoctl(Socket,
                                     SIO_GET_EXTENSION_FUNCTION_POINTER,
                                     &TransmitFileGUID,
                                     sizeof(TransmitFileGUID),
                                     &pfnTransmitFile,
                                     sizeof(pfnTransmitFile),
                                     &cbBytesReturned,
                                     NULL,
                                     NULL) == SOCKET_ERROR)
    {
        ERR("TransmitFile WSAIoctl %lx\n", WSAGetLastError());
        return FALSE;
    }

    Ret = pfnTransmitFile(Socket,
                          File,
                          NumberOfBytesToWrite,
                          NumberOfBytesPerSend,
                          Overlapped,
                          TransmitBuffers,
                          Flags);
    if (!Ret)
    {
        ERR("TransmitFile %lx\n", WSAGetLastError());
    }
    return Ret;
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
    BOOL  Ret;

    TRACE("AcceptEx %p %p %p %ld %ld %ld %p %p\n", ListenSocket, AcceptSocket, OutputBuffer, ReceiveDataLength, LocalAddressLength, RemoteAddressLength, BytesReceived, Overlapped);
    if (!pfnAcceptEx && WSAIoctl(ListenSocket,
                                 SIO_GET_EXTENSION_FUNCTION_POINTER,
                                 &AcceptExGUID,
                                 sizeof(AcceptExGUID),
                                 &pfnAcceptEx,
                                 sizeof(pfnAcceptEx),
                                 &cbBytesReturned,
                                 NULL,
                                 NULL) == SOCKET_ERROR)
    {
        ERR("AcceptEx WSAIoctl %lx\n", WSAGetLastError());
        return FALSE;
    }

    if (!pfnGetAcceptExSockaddrs && WSAIoctl(ListenSocket,
                                             SIO_GET_EXTENSION_FUNCTION_POINTER,
                                             &GetAcceptExSockaddrsGUID,
                                             sizeof(GetAcceptExSockaddrsGUID),
                                             &pfnGetAcceptExSockaddrs,
                                             sizeof(pfnGetAcceptExSockaddrs),
                                             &cbBytesReturned,
                                             NULL,
                                             NULL) == SOCKET_ERROR)
    {
        ERR("GetAcceptExSockaddrs WSAIoctl %lx\n", WSAGetLastError());
        pfnAcceptEx = NULL;
        return FALSE;
    }

    Ret = pfnAcceptEx(ListenSocket,
                      AcceptSocket,
                      OutputBuffer,
                      ReceiveDataLength,
                      LocalAddressLength,
                      RemoteAddressLength,
                      BytesReceived,
                      Overlapped);
    if (!Ret)
    {
        ERR("AcceptEx %lx\n", WSAGetLastError());
    }
    return Ret;
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
    TRACE("AcceptEx %p %ld %ld %ld %p %p %p %p\n", OutputBuffer, ReceiveDataLength, LocalAddressLength, RemoteAddressLength, LocalSockaddr, LocalSockaddrLength, RemoteSockaddr, RemoteSockaddrLength);
    if (!pfnGetAcceptExSockaddrs)
    {
        ERR("GetAcceptExSockaddrs is NULL\n");
        return;
    }
    pfnGetAcceptExSockaddrs(OutputBuffer,
                            ReceiveDataLength,
                            LocalAddressLength,
                            RemoteAddressLength,
                            LocalSockaddr,
                            LocalSockaddrLength,
                            RemoteSockaddr,
                            RemoteSockaddrLength);
}
/* EOF */
