/* $Id: stubs.c,v 1.1 2003/09/12 17:51:47 vizzini Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock DLL
 * FILE:        stubs.c
 * PURPOSE:     Stub functions
 * PROGRAMMERS: Ge van Geldorp (ge@gse.nl)
 * REVISIONS:
 */

#include <windows.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2spi.h>

/*
 * @unimplemented
 */
BOOL
STDCALL
AcceptEx(SOCKET ListenSocket,
         SOCKET AcceptSocket,
         PVOID OutputBuffer,
         DWORD ReceiveDataLength,
         DWORD LocalAddressLength,
         DWORD RemoteAddressLength,
         LPDWORD BytesReceived,
         LPOVERLAPPED Overlapped)
{
  OutputDebugStringW(L"w32sock AcceptEx stub called\n");

  return FALSE;
}


/*
 * @unimplemented
 */
INT
STDCALL
EnumProtocolsA(LPINT ProtocolCount,
               LPVOID ProtocolBuffer,
               LPDWORD BufferLength)
{
  OutputDebugStringW(L"w32sock EnumProtocolsA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
EnumProtocolsW(LPINT ProtocolCount,
               LPVOID ProtocolBuffer,
               LPDWORD BufferLength)
{
  OutputDebugStringW(L"w32sock EnumProtocolsW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
VOID
STDCALL
GetAcceptExSockaddrs(PVOID OutputBuffer,
                     DWORD ReceiveDataLength,
                     DWORD LocalAddressLength,
                     DWORD RemoteAddressLength,
                     LPSOCKADDR* LocalSockaddr,
                     LPINT LocalSockaddrLength,
                     LPSOCKADDR* RemoteSockaddr,
                     LPINT RemoteSockaddrLength)
{
  OutputDebugStringW(L"w32sock GetAcceptExSockaddrs stub called\n");
}


/*
 * @unimplemented
 */
INT
STDCALL
GetAddressByNameA(DWORD NameSpace,
                  LPGUID ServiceType,
                  LPSTR ServiceName,
                  LPINT Protocols,
                  DWORD Resolution,
                  LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
                  LPVOID CsaddrBuffer,
                  LPDWORD BufferLength,
                  LPSTR AliasBuffer,
                  LPDWORD AliasBufferLength)
{
  OutputDebugStringW(L"w32sock GetAddressByNameA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetAddressByNameW(DWORD NameSpace,
                  LPGUID ServiceType,
                  LPWSTR ServiceName,
                  LPINT Protocols,
                  DWORD Resolution,
                  LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
                  LPVOID CsaddrBuffer,
                  LPDWORD BufferLength,
                  LPWSTR AliasBuffer,
                  LPDWORD AliasBufferLength)
{
  OutputDebugStringW(L"w32sock GetAddressByNameW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetServiceA(DWORD NameSpace,
            LPGUID Guid,
            LPSTR ServiceName,
            DWORD Properties,
            LPVOID Buffer,
            LPDWORD BufferSize,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo)
{
  OutputDebugStringW(L"w32sock GetServiceA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetServiceW(DWORD NameSpace,
            LPGUID Guid,
            LPWSTR ServiceName,
            DWORD Properties,
            LPVOID Buffer,
            LPDWORD BufferSize,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo)
{
  OutputDebugStringW(L"w32sock GetServiceW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetTypeByNameA(LPSTR ServiceName,
               LPGUID ServiceType)
{
  OutputDebugStringW(L"w32sock GetTypeByNameA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
GetTypeByNameW(LPWSTR ServiceName,
               LPGUID ServiceType)
{
  OutputDebugStringW(L"w32sock GetTypeByNameW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
SetServiceA(DWORD NameSpace,
            DWORD Operation,
            DWORD Flags,
            LPSERVICE_INFOA ServiceInfo,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
            LPDWORD dwStatusFlags)
{
  OutputDebugStringW(L"w32sock SetServiceA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
STDCALL
SetServiceW(DWORD NameSpace,
            DWORD Operation,
            DWORD Flags,
            LPSERVICE_INFOW ServiceInfo,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
            LPDWORD dwStatusFlags)
{
  OutputDebugStringW(L"w32sock SetServiceW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
TransmitFile(SOCKET Socket,
             HANDLE File,
             DWORD NumberOfBytesToWrite,
             DWORD NumberOfBytesPerSend,
             LPOVERLAPPED Overlapped,
             LPTRANSMIT_FILE_BUFFERS TransmitBuffers,
             DWORD Flags)
{
  OutputDebugStringW(L"w32sock TransmitFile stub called\n");

  return FALSE;
}

/*
 * @unimplemented
 */
int
STDCALL
WSARecvEx(SOCKET Sock,
          char *Buf,
          int Len,
          int *Flags)
{
  OutputDebugStringW(L"w32sock WSARecvEx stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
int
STDCALL
dn_expand(unsigned char *MessagePtr,
          unsigned char *EndofMesOrig,
          unsigned char *CompDomNam,
          unsigned char *ExpandDomNam,
          int Length)
{
  OutputDebugStringW(L"w32sock dn_expand stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
struct netent *
STDCALL
getnetbyname(const char *name)
{
  OutputDebugStringW(L"w32sock getnetbyname stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
UINT
STDCALL
inet_network(const char *cp)
{
  OutputDebugStringW(L"w32sock inet_network stub called\n");

  return INADDR_NONE;
}


/*
 * @unimplemented
 */
SOCKET
STDCALL
rcmd(char **AHost,
     USHORT InPort,
     char *LocUser,
     char *RemUser,
     char *Cmd,
     int *Fd2p)
{
  OutputDebugStringW(L"w32sock rcmd stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
SOCKET
STDCALL
rexec(char **AHost,
      int InPort,
      char *User,
      char *Passwd,
      char *Cmd,
      int *Fd2p)
{
  OutputDebugStringW(L"w32sock rexec stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
SOCKET
STDCALL
rresvport(int *port)
{
  OutputDebugStringW(L"w32sock rresvport stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
void
STDCALL
s_perror(const char *str)
{
  OutputDebugStringW(L"w32sock s_perror stub called\n");
}


/*
 * @unimplemented
 */
int
STDCALL
sethostname(char *Name, int NameLen)
{
  OutputDebugStringW(L"w32sock sethostname stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DllMain(HINSTANCE InstDLL,
        DWORD Reason,
        LPVOID Reserved)
{
  return TRUE;
}

/*
 * @unimplemented
 */
INT
STDCALL
GetNameByTypeA(LPGUID lpServiceType,LPSTR lpServiceName,DWORD dwNameLength)
{
  OutputDebugStringW(L"w32sock GetNameByTypeA stub called\n");
  return TRUE;
}

/*
 * @unimplemented
 */
INT
STDCALL
GetNameByTypeW(LPGUID lpServiceType,LPWSTR lpServiceName,DWORD dwNameLength)
{
  OutputDebugStringW(L"w32sock GetNameByTypeW stub called\n");
  return TRUE;
}

/*
 * @unimplemented
 */
INT
STDCALL
NSPStartup(
    LPGUID lpProviderId,
    LPNSP_ROUTINE lpnspRoutines
    )
{
  return TRUE;
}

/*
 * @unimplemented
 */
int
STDCALL
WSPStartup(
    IN WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN WSPUPCALLTABLE UpcallTable,
    OUT LPWSPPROC_TABLE lpProcTable
    )
{
  return TRUE;
}

/*
 * @unimplemented
 */
INT
STDCALL
NPLoadNameSpaces (
    IN OUT LPDWORD         lpdwVersion,
    IN OUT LPNS_ROUTINE    nsrBuffer,
    IN OUT LPDWORD         lpdwBufferLength
    )
{
  return TRUE;
}
