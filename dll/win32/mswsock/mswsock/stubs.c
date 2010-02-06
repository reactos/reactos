/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

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
  OutputDebugStringW(L"AcceptEx is UNIMPLEMENTED\n");

  return FALSE;
}

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
  OutputDebugStringW(L"GetAcceptExSockaddrs is UNIMPLEMENTED\n");
}

INT
WINAPI
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
  OutputDebugStringW(L"GetAddressByNameA is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
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
  OutputDebugStringW(L"GetAddressByNameW is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
GetServiceA(DWORD NameSpace,
            LPGUID Guid,
            LPSTR ServiceName,
            DWORD Properties,
            LPVOID Buffer,
            LPDWORD BufferSize,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo)
{
  OutputDebugStringW(L"GetServiceA is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
GetServiceW(DWORD NameSpace,
            LPGUID Guid,
            LPWSTR ServiceName,
            DWORD Properties,
            LPVOID Buffer,
            LPDWORD BufferSize,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo)
{
  OutputDebugStringW(L"GetServiceW is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
GetTypeByNameA(LPSTR ServiceName,
               LPGUID ServiceType)
{
  OutputDebugStringW(L"GetTypeByNameA is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
GetTypeByNameW(LPWSTR ServiceName,
               LPGUID ServiceType)
{
  OutputDebugStringW(L"GetTypeByNameW is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
MigrateWinsockConfiguration(DWORD Unknown1,
                            DWORD Unknown2,
                            DWORD Unknown3)
{
  OutputDebugStringW(L"MigrateWinsockConfiguration is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
SetServiceA(DWORD NameSpace,
            DWORD Operation,
            DWORD Flags,
            LPSERVICE_INFOA ServiceInfo,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
            LPDWORD dwStatusFlags)
{
  OutputDebugStringW(L"SetServiceA is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
SetServiceW(DWORD NameSpace,
            DWORD Operation,
            DWORD Flags,
            LPSERVICE_INFOW ServiceInfo,
            LPSERVICE_ASYNC_INFO ServiceAsyncInfo,
            LPDWORD dwStatusFlags)
{
  OutputDebugStringW(L"SetServiceW is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

int
WINAPI
WSARecvEx(SOCKET Sock,
          char *Buf,
          int Len,
          int *Flags)
{
  OutputDebugStringW(L"WSARecvEx is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

int
WINAPI
dn_expand(unsigned char *MessagePtr,
          unsigned char *EndofMesOrig,
          unsigned char *CompDomNam,
          unsigned char *ExpandDomNam,
          int Length)
{
  OutputDebugStringW(L"dn_expand is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

struct netent *
WINAPI
getnetbyname(const char *name)
{
  OutputDebugStringW(L"getnetbyname is UNIMPLEMENTED\n");

  return NULL;
}

UINT
WINAPI
inet_network(const char *cp)
{
  OutputDebugStringW(L"inet_network is UNIMPLEMENTED\n");

  return INADDR_NONE;
}

SOCKET
WINAPI
rcmd(char **AHost,
     USHORT InPort,
     char *LocUser,
     char *RemUser,
     char *Cmd,
     int *Fd2p)
{
  OutputDebugStringW(L"rcmd is UNIMPLEMENTED\n");

  return INVALID_SOCKET;
}

SOCKET
WINAPI
rexec(char **AHost,
      int InPort,
      char *User,
      char *Passwd,
      char *Cmd,
      int *Fd2p)
{
  OutputDebugStringW(L"rexec is UNIMPLEMENTED\n");

  return INVALID_SOCKET;
}

SOCKET
WINAPI
rresvport(int *port)
{
  OutputDebugStringW(L"rresvport is UNIMPLEMENTED\n");

  return INVALID_SOCKET;
}

void
WINAPI
s_perror(const char *str)
{
  OutputDebugStringW(L"s_perror is UNIMPLEMENTED\n");
}

int
WINAPI
sethostname(char *Name, int NameLen)
{
  OutputDebugStringW(L"sethostname is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
GetNameByTypeA(LPGUID lpServiceType, LPSTR lpServiceName, DWORD dwNameLength)
{
  OutputDebugStringW(L"GetNameByTypeA is UNIMPLEMENTED\n");

  return 0;
}

INT
WINAPI
GetNameByTypeW(LPGUID lpServiceType, LPWSTR lpServiceName, DWORD dwNameLength)
{
  OutputDebugStringW(L"GetNameByTypeW is UNIMPLEMENTED\n");

  return 0;
}

VOID
WINAPI
StartWsdpService()
{
  OutputDebugStringW(L"StartWsdpService is UNIMPLEMENTED\n");
}

VOID
WINAPI
StopWsdpService()
{
  OutputDebugStringW(L"StopWsdpService is UNIMPLEMENTED\n");
}

DWORD
WINAPI
SvchostPushServiceGlobals(DWORD Value)
{
  OutputDebugStringW(L"SvchostPushServiceGlobals is UNIMPLEMENTED\n");

  return 0;
}

VOID
WINAPI
ServiceMain(DWORD Unknown1, DWORD Unknown2)
{
  OutputDebugStringW(L"ServiceMain is UNIMPLEMENTED\n");
}

INT
WINAPI
EnumProtocolsA(LPINT ProtocolCount,
               LPVOID ProtocolBuffer,
               LPDWORD BufferLength)
{
  OutputDebugStringW(L"EnumProtocolsA is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
EnumProtocolsW(LPINT ProtocolCount,
               LPVOID ProtocolBuffer,
               LPDWORD BufferLength)
{
  OutputDebugStringW(L"EnumProtocolsW is UNIMPLEMENTED\n");

  return SOCKET_ERROR;
}

INT
WINAPI
NPLoadNameSpaces(
    IN OUT LPDWORD lpdwVersion,
    IN OUT LPNS_ROUTINE nsrBuffer,
    IN OUT LPDWORD lpdwBufferLength)
{
  OutputDebugStringW(L"NPLoadNameSpaces is UNIMPLEMENTED\n");

  return 0;
}

