/* $Id: stubs.c,v 1.1 2003/04/03 22:44:19 gvg Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock DLL
 * FILE:        stubs.c
 * PURPOSE:     Stub functions
 * PROGRAMMERS: Ge van Geldorp (ge@gse.nl)
 * REVISIONS:
 */

#include <windows.h>
#include <winsock2.h>

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
  OutputDebugString(L"w32sock AcceptEx stub called\n");

  return FALSE;
}

INT
STDCALL
EnumProtocolsA(LPINT ProtocolCount,
               LPVOID ProtocolBuffer,
               LPDWORD BufferLength)
{
  OutputDebugString(L"w32sock EnumProtocolsA stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
EnumProtocolsW(LPINT ProtocolCount,
               LPVOID ProtocolBuffer,
               LPDWORD BufferLength)
{
  OutputDebugString(L"w32sock EnumProtocolsW stub called\n");

  return SOCKET_ERROR;
}

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
  OutputDebugString(L"w32sock GetAcceptExSockaddrs stub called\n");
}

INT
STDCALL
GetAddressByNameA(DWORD NameSpace,
                  LPGUID ServiceType,
                  LPSTR ServiceName,
                  LPINT Protocols,
                  DWORD Resolution,
                  LPVOID /* really LPSERVICE_ASYNC_INFO */ ServiceAsyncInfo,
                  LPVOID CsaddrBuffer,
                  LPDWORD BufferLength,
                  LPSTR AliasBuffer,
                  LPDWORD AliasBufferLength)
{
  OutputDebugString(L"w32sock GetAddressByNameA stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
GetAddressByNameW(DWORD NameSpace,
                  LPGUID ServiceType,
                  LPWSTR ServiceName,
                  LPINT Protocols,
                  DWORD Resolution,
                  LPVOID /* really LPSERVICE_ASYNC_INFO */ ServiceAsyncInfo,
                  LPVOID CsaddrBuffer,
                  LPDWORD BufferLength,
                  LPWSTR AliasBuffer,
                  LPDWORD AliasBufferLength)
{
  OutputDebugString(L"w32sock GetAddressByNameW stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
GetNameByTypeA(LPGUID ServiceType,
               LPSTR ServiceName,
               DWORD NameLength)
{
  OutputDebugString(L"w32sock GetNameByTypeA stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
GetNameByTypeW(LPGUID ServiceType,
               LPWSTR ServiceName,
               DWORD NameLength)
{
  OutputDebugString(L"w32sock GetNameByTypeW stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
GetServiceA(DWORD NameSpace,
            LPGUID Guid,
            LPSTR ServiceName,
            DWORD Properties,
            LPVOID Buffer,
            LPDWORD BufferSize,
            LPVOID /* Really LPSERVICE_ASYNC_INFO */ ServiceAsyncInfo)
{
  OutputDebugString(L"w32sock GetServiceA stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
GetServiceW(DWORD NameSpace,
            LPGUID Guid,
            LPWSTR ServiceName,
            DWORD Properties,
            LPVOID Buffer,
            LPDWORD BufferSize,
            LPVOID /* Really LPSERVICE_ASYNC_INFO */ ServiceAsyncInfo)
{
  OutputDebugString(L"w32sock GetServiceW stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
GetTypeByNameA(LPSTR ServiceName,
               LPGUID ServiceType)
{
  OutputDebugString(L"w32sock GetTypeByNameA stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
GetTypeByNameW(LPWSTR ServiceName,
               LPGUID ServiceType)
{
  OutputDebugString(L"w32sock GetTypeByNameW stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
SetServiceA(DWORD NameSpace,
            DWORD Operation,
            DWORD Flags,
            LPVOID /* Really LPSERVICE_INFO */ ServiceInfo,
            LPVOID /* Really LPSERVICE_ASYNC_INFOA */ ServiceAsyncInfo,
            LPDWORD dwStatusFlags)
{
  OutputDebugString(L"w32sock SetServiceA stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
SetServiceW(DWORD NameSpace,
            DWORD Operation,
            DWORD Flags,
            LPVOID /* Really LPSERVICE_INFO */ ServiceInfo,
            LPVOID /* Really LPSERVICE_ASYNC_INFOW */ ServiceAsyncInfo,
            LPDWORD dwStatusFlags)
{
  OutputDebugString(L"w32sock SetServiceW stub called\n");

  return SOCKET_ERROR;
}

BOOL
STDCALL
TransmitFile(SOCKET Socket,
             HANDLE File,
             DWORD NumberOfBytesToWrite,
             DWORD NumberOfBytesPerSend,
             LPOVERLAPPED Overlapped,
             LPVOID /* really LPTRANSMIT_FILE_BUFFERS */ TransmitBuffers,
             DWORD Flags)
{
  OutputDebugString(L"w32sock TransmitFile stub called\n");

  return FALSE;
}

HANDLE
STDCALL
WSAAsyncGetHostByAddr(HWND Wnd,
                      unsigned int Msg,
                      const char *Addr,
                      int Len,
                      int Type,
                      char *Buf,
                      int BufLen)
{
  OutputDebugString(L"w32sock WSAAsyncGetHostByAddr stub called\n");

  return NULL;
}

HANDLE
STDCALL
WSAAsyncGetHostByName(HWND Wnd,
                      unsigned int Msg,
                      const char *Name,
                      char *Buf,
                      int BufLen)
{
  OutputDebugString(L"w32sock WSAAsyncGetHostByName stub called\n");

  return NULL;
}

HANDLE
STDCALL
WSAAsyncGetProtoByName(HWND Wnd,
                       unsigned int Msg,
                       const char *Name,
                       char *Buf,
                       int Buflen)
{
  OutputDebugString(L"w32sock WSAAsyncGetProtoByName stub called\n");

  return NULL;
}

HANDLE
STDCALL
WSAAsyncGetProtoByNumber(HWND Wnd,
                         unsigned int Msg,
                         int Number,
                         char *Buf,
                         int BufLen)
{
  OutputDebugString(L"w32sock WSAAsyncGetProtoByNumber stub called\n");

  return NULL;
}

HANDLE
STDCALL
WSAAsyncGetServByName(HWND Wnd,
                      unsigned int Msg,
                      const char *Name,
                      const char *Proto,
                      char *Buf,
                      int BufLen)
{
  OutputDebugString(L"w32sock WSAAsyncGetServByName stub called\n");

  return NULL;
}

HANDLE
STDCALL
WSAAsyncGetServByPort(HWND Wnd,
                      unsigned int Msg,
                      int Port,
                      const char *Proto,
                      char *Buf,
                      int BufLen)
{
  OutputDebugString(L"w32sock WSAAsyncGetServByPort stub called\n");

  return NULL;
}

INT
STDCALL
WSAAsyncSelect(SOCKET Sock,
               HWND Wnd,
               UINT Msg,
               LONG Event)
{
  OutputDebugString(L"w32sock WSAAsyncSelect stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
WSACancelAsyncRequest(HANDLE AsyncTaskHandle)
{
  OutputDebugString(L"w32sock WSACancelAsyncRequest stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
WSACancelBlockingCall()
{
  OutputDebugString(L"w32sock WSACancelBlockingCall stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
WSACleanup()
{
  OutputDebugString(L"w32sock WSACleanup stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
WSAGetLastError(void)
{
  OutputDebugString(L"w32sock WSAGetLastError stub called\n");

  return WSANOTINITIALISED;
}

BOOL
STDCALL
WSAIsBlocking(VOID)
{
  OutputDebugString(L"w32sock WSAIsBlocking stub called\n");

  return FALSE;
}

int
STDCALL
WSARecvEx(SOCKET Sock,
          char *Buf,
          int Len,
          int *Flags)
{
  OutputDebugString(L"w32sock WSARecvEx stub called\n");

  return SOCKET_ERROR;
}

FARPROC
STDCALL
WSASetBlockingHook(FARPROC BlockFunc)
{
  OutputDebugString(L"w32sock WSASetBlockingHook stub called\n");

  return NULL;
}

void
STDCALL WSASetLastError(int Error)
{
  OutputDebugString(L"w32sock WSASetLastError stub called\n");
}

int
STDCALL
WSAStartup(WORD VersionRequested,
           LPWSADATA WSAData)
{
  OutputDebugString(L"w32sock WSAStartup stub called\n");

  return WSASYSNOTREADY;
}

int
STDCALL
WSAUnhookBlockingHook(void)
{
  OutputDebugString(L"w32sock WSAUnhookBlockingHook stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
WSApSetPostRoutine(LPVOID /* really LPWPUPOSTMESSAGE */ PostRoutine)
{
  OutputDebugString(L"w32sock WSApSetPostRoutine stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
__WSAFDIsSet(SOCKET Sock,
             fd_set *Set)
{
  OutputDebugString(L"w32sock __WSAFDIsSet stub called\n");

  return 0;
}

SOCKET
STDCALL
accept(SOCKET Sock,
       struct sockaddr *Addr,
       int *AddrLen)
{
  OutputDebugString(L"w32sock accept stub called\n");

  return INVALID_SOCKET;
}

INT
STDCALL
bind(SOCKET Sock,
     CONST LPSOCKADDR Name,
     INT NameLen)
{
  OutputDebugString(L"w32sock bind stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
closesocket(SOCKET Sock)
{
  OutputDebugString(L"w32sock closesocket stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
connect(SOCKET Sock,
        CONST LPSOCKADDR Name,
        INT NameLen)
{
  OutputDebugString(L"w32sock connect stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
dn_expand(unsigned char *MessagePtr,
          unsigned char *EndofMesOrig,
          unsigned char *CompDomNam,
          unsigned char *ExpandDomNam,
          int Length)
{
  OutputDebugString(L"w32sock dn_expand stub called\n");

  return SOCKET_ERROR;
}

LPHOSTENT
STDCALL
gethostbyaddr(CONST CHAR *Addr,
              INT Len,
              INT Type)
{
  OutputDebugString(L"w32sock gethostbyaddr stub called\n");

  return NULL;
}

struct hostent *
STDCALL
gethostbyname(const char *Name)
{
  OutputDebugString(L"w32sock gethostbyname stub called\n");

  return NULL;
}

int
STDCALL
gethostname(char *Name,
            int NameLen)
{
  OutputDebugString(L"w32sock gethostname stub called\n");

  return SOCKET_ERROR;
}

struct netent *
STDCALL
getnetbyname(char *Name)
{
  OutputDebugString(L"w32sock getnetbyname stub called\n");

  return NULL;
}

int
STDCALL
getpeername(SOCKET Sock,
            struct sockaddr *Name,
            int *NameLen)
{
  OutputDebugString(L"w32sock getpeername stub called\n");

  return SOCKET_ERROR;
}

LPPROTOENT
STDCALL
getprotobyname(CONST CHAR *Name)
{
  OutputDebugString(L"w32sock getprotobyname stub called\n");

  return NULL;
}

LPPROTOENT
STDCALL
getprotobynumber(INT Number)
{
  OutputDebugString(L"w32sock getprotobynumber stub called\n");

  return NULL;
}

struct servent *
STDCALL
getservbyname(const char *Name,
              const char *Proto)
{
  OutputDebugString(L"w32sock getservbyname stub called\n");

  return NULL;
}

struct servent *
STDCALL
getservbyport(int Port,
              const char *Proto)
{
  OutputDebugString(L"w32sock getservbyport stub called\n");

  return NULL;
}

int
STDCALL
getsockname(SOCKET Sock,
            struct sockaddr *Name,
            int *NameLen)
{
  OutputDebugString(L"w32sock getsockname stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
getsockopt(SOCKET Sock,
           int Level,
           int OptName,
           char *OptVal,
           int *OptLen)
{
  OutputDebugString(L"w32sock getsockopt stub called\n");

  return SOCKET_ERROR;
}

ULONG
STDCALL
htonl(ULONG HostLong)
{
  return (((HostLong << 24) & 0xff000000) |
          ((HostLong << 8) & 0x00ff0000) |
          ((HostLong >> 8) & 0x0000ff00) |
          ((HostLong >> 24) & 0x000000ff));
}

USHORT
STDCALL
htons(USHORT HostShort)
{
  return (((HostShort << 8) & 0xff00) |
          ((HostShort >> 8) & 0x00ff));
}

ULONG
STDCALL
inet_addr(CONST CHAR *cp)
{
  OutputDebugString(L"w32sock inet_addr stub called\n");

  return INADDR_NONE;
}

unsigned long
STDCALL
inet_network(const char *cp)
{
  OutputDebugString(L"w32sock inet_network stub called\n");

  return INADDR_NONE;
}

char *
STDCALL
inet_ntoa(struct in_addr in)
{
  OutputDebugString(L"w32sock inet_ntoa stub called\n");

  return NULL;
}

INT
STDCALL
ioctlsocket(SOCKET Sock,
            LONG Cmd,
            ULONG *Argp)
{
  OutputDebugString(L"w32sock ioctlsocket stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
listen(SOCKET Sock,
       int BackLog)
{
  OutputDebugString(L"w32sock listen stub called\n");

  return SOCKET_ERROR;
}


ULONG
STDCALL
ntohl(ULONG NetLong)
{
  return (((NetLong << 24) & 0xff000000) |
          ((NetLong << 8) & 0x00ff0000) |
          ((NetLong >> 8) & 0x0000ff00) |
          ((NetLong >> 24) & 0x000000ff));
}

USHORT
STDCALL
ntohs(USHORT NetShort)
{
  return (((NetShort << 8) & 0xff00) |
          ((NetShort >> 8) & 0x00ff));
}

SOCKET
STDCALL
rcmd(char **AHost,
     USHORT InPort,
     char *LocUser,
     char *RemUser,
     char *Cmd,
     int *Fd2p)
{
  OutputDebugString(L"w32sock rcmd stub called\n");

  return INVALID_SOCKET;
}

int
STDCALL
recv(SOCKET Sock,
     char *Buf,
     int Len,
     int Flags)
{
  OutputDebugString(L"w32sock recv stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
recvfrom(SOCKET Sock,
         char *Buf,
         int Len,
         int Flags,
         struct sockaddr *From,
         int *FromLen)
{
  OutputDebugString(L"w32sock recvfrom stub called\n");

  return SOCKET_ERROR;
}

SOCKET
STDCALL
rexec(char **AHost,
      int InPort,
      char *User,
      char *Passwd,
      char *Cmd,
      int *Fd2p)
{
  OutputDebugString(L"w32sock rexec stub called\n");

  return INVALID_SOCKET;
}


SOCKET
STDCALL
rresvport(int *port)
{
  OutputDebugString(L"w32sock rresvport stub called\n");

  return INVALID_SOCKET;
}

void
STDCALL
s_perror(const char *str)
{
  OutputDebugString(L"w32sock s_perror stub called\n");
}

INT
STDCALL
select(INT NumFds, 
       LPFD_SET ReadFds, 
       LPFD_SET WriteFds, 
       LPFD_SET ExceptFds, 
       CONST LPTIMEVAL TimeOut)
{
  OutputDebugString(L"w32sock select stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
send(SOCKET Sock,
     const char *Buf,
     int Len,
     int Flags)
{
  OutputDebugString(L"w32sock send stub called\n");

  return SOCKET_ERROR;
}

INT
STDCALL
sendto(SOCKET Sock,
       CONST CHAR *Buf,
       INT Len,
       INT Flags,
       CONST LPSOCKADDR To, 
       INT ToLen)
{
  OutputDebugString(L"w32sock sendto stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
sethostname(char *Name, int NameLen)
{
  OutputDebugString(L"w32sock sethostname stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
setsockopt(SOCKET Sock,
           int Level,
           int OptName,
           const char *OptVal,
           int OptLen)
{
  OutputDebugString(L"w32sock setsockopt stub called\n");

  return SOCKET_ERROR;
}

int
STDCALL
shutdown(SOCKET Sock,
         int How)
{
  OutputDebugString(L"w32sock shutdown stub called\n");

  return SOCKET_ERROR;
}

SOCKET
STDCALL
socket(int AF,
       int Type,
       int Protocol)
{
  OutputDebugString(L"w32sock socket stub called\n");

  return INVALID_SOCKET;
}

BOOL
STDCALL
DllMain(HINSTANCE InstDLL,
        DWORD Reason,
        LPVOID Reserved)
{
  return TRUE;
}

