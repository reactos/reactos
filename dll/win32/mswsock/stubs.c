/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WinSock DLL
 * FILE:            stubs.c
 * PURPOSE:         Stub functions
 * PROGRAMMERS:     Ge van Geldorp (ge@gse.nl)
 * REVISIONS:
 */

#include "precomp.h"

#include <windef.h>
#include <ws2spi.h>
#include <nspapi.h>
#include <svc.h>

typedef DWORD (* LPFN_NSPAPI)(VOID);
typedef struct _NS_ROUTINE {
    DWORD        dwFunctionCount;
    LPFN_NSPAPI *alpfnFunctions;
    DWORD        dwNameSpace;
    DWORD        dwPriority;
} NS_ROUTINE, *PNS_ROUTINE, * FAR LPNS_ROUTINE;


/*
 * @unimplemented
 */
INT
WINAPI
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
WINAPI
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
  OutputDebugStringW(L"w32sock GetAddressByNameA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
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
  OutputDebugStringW(L"w32sock GetAddressByNameW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
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
  OutputDebugStringW(L"w32sock GetServiceA stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
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
  OutputDebugStringW(L"w32sock GetServiceW stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
WINAPI
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
WINAPI
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
WINAPI
MigrateWinsockConfiguration(DWORD Unknown1,
                            DWORD Unknown2,
                            DWORD Unknown3)
{
  OutputDebugStringW(L"w32sock MigrateWinsockConfiguration stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
INT
WINAPI
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
WINAPI
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
int
WINAPI
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
WINAPI
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
WINAPI
getnetbyname(const char *name)
{
  OutputDebugStringW(L"w32sock getnetbyname stub called\n");

  return NULL;
}


/*
 * @unimplemented
 */
UINT
WINAPI
inet_network(const char *cp)
{
  OutputDebugStringW(L"w32sock inet_network stub called\n");

  return INADDR_NONE;
}


/*
 * @unimplemented
 */
SOCKET
WINAPI
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
WINAPI
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
WINAPI
rresvport(int *port)
{
  OutputDebugStringW(L"w32sock rresvport stub called\n");

  return INVALID_SOCKET;
}


/*
 * @unimplemented
 */
void
WINAPI
s_perror(const char *str)
{
  OutputDebugStringW(L"w32sock s_perror stub called\n");
}


/*
 * @unimplemented
 */
int
WINAPI
sethostname(char *Name, int NameLen)
{
  OutputDebugStringW(L"w32sock sethostname stub called\n");

  return SOCKET_ERROR;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
GetNameByTypeA(LPGUID lpServiceType,LPSTR lpServiceName,DWORD dwNameLength)
{
  OutputDebugStringW(L"w32sock GetNameByTypeA stub called\n");
  return TRUE;
}


/*
 * @unimplemented
 */
INT
WINAPI
GetNameByTypeW(LPGUID lpServiceType,LPWSTR lpServiceName,DWORD dwNameLength)
{
  OutputDebugStringW(L"w32sock GetNameByTypeW stub called\n");
  return TRUE;
}

/*
 * @unimplemented
 */
int
WINAPI
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
WINAPI
NPLoadNameSpaces(
    IN OUT LPDWORD lpdwVersion,
    IN OUT LPNS_ROUTINE nsrBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )
{
  OutputDebugStringW(L"mswsock NPLoadNameSpaces stub called\n");

  *lpdwVersion = 1;

  return TRUE;
}


/*
 * @unimplemented
 */
VOID
WINAPI
StartWsdpService()
{
  OutputDebugStringW(L"mswsock StartWsdpService stub called\n");
}


/*
 * @unimplemented
 */
VOID
WINAPI
StopWsdpService()
{
  OutputDebugStringW(L"mswsock StopWsdpService stub called\n");
}


/*
 * @unimplemented
 *
 * See https://www.geoffchappell.com/studies/windows/win32/services/svchost/dll/svchostpushserviceglobals.htm
 */
VOID
WINAPI
SvchostPushServiceGlobals(SVCHOST_GLOBALS *lpGlobals)
{
  OutputDebugStringW(L"mswsock SvchostPushServiceGlobals stub called\n");
}


/*
 * @unimplemented
 */
VOID
WINAPI
ServiceMain(DWORD Unknown1, DWORD Unknown2)
{
  OutputDebugStringW(L"mswsock ServiceMain stub called\n");
}
