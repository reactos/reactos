/*
 *  ws2tcpip.h : TCP/IP specific extensions in Windows Sockets 2
 *
 * Portions Copyright (c) 1980, 1983, 1988, 1993
 * The Regents of the University of California.  All rights reserved.
 *
 */

#pragma once

#define _WS2TCPIP_H

#include <winsock2.h>
#include <ws2ipdef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UDP_NOCHECKSUM 1
#define UDP_CHECKSUM_COVERAGE 20

#ifdef _MSC_VER
#define WS2TCPIP_INLINE __inline
#else
#define WS2TCPIP_INLINE extern inline
#endif

/* getaddrinfo error codes */
#define EAI_AGAIN WSATRY_AGAIN
#define EAI_BADFLAGS WSAEINVAL
#define EAI_FAIL WSANO_RECOVERY
#define EAI_FAMILY WSAEAFNOSUPPORT
#define EAI_MEMORY WSA_NOT_ENOUGH_MEMORY
#define EAI_NODATA EAI_NONAME
#define EAI_NOSECURENAME WSA_SECURE_HOST_NOT_FOUND
#define EAI_NONAME WSAHOST_NOT_FOUND
#define EAI_SERVICE WSATYPE_NOT_FOUND
#define EAI_SOCKTYPE WSAESOCKTNOSUPPORT
#define EAI_IPSECPOLICY WSA_IPSEC_NAME_POLICY_ERROR

#ifdef UNICODE
typedef ADDRINFOW ADDRINFOT,*PADDRINFOT;
#else
typedef ADDRINFOA ADDRINFOT,*PADDRINFOT;
#endif

typedef ADDRINFOA ADDRINFO, FAR *LPADDRINFO;

#if (_WIN32_WINNT >= 0x0600)

#ifdef UNICODE
typedef ADDRINFOEXW ADDRINFOEX, *PADDRINFOEX;
#else
typedef ADDRINFOEXA ADDRINFOEX, *PADDRINFOEX;
#endif

#endif /* (_WIN32_WINNT >= 0x0600) */

WINSOCK_API_LINKAGE
INT
WSAAPI
getaddrinfo(
  IN PCSTR pNodeName OPTIONAL,
  IN PCSTR pServiceName OPTIONAL,
  IN const ADDRINFOA *pHints OPTIONAL,
  OUT PADDRINFOA *ppResult);

#if (NTDDI_VERSION >= NTDDI_WINXPSP2) || (_WIN32_WINNT >= 0x0502)

WINSOCK_API_LINKAGE
INT
WSAAPI
GetAddrInfoW(
  IN PCWSTR pNodeName OPTIONAL,
  IN PCWSTR pServiceName OPTIONAL,
  IN const ADDRINFOW *pHints OPTIONAL,
  OUT PADDRINFOW *ppResult);

#define GetAddrInfoA getaddrinfo

#ifdef UNICODE
#define GetAddrInfo GetAddrInfoW
#else
#define GetAddrInfo GetAddrInfoA
#endif

#endif /* (NTDDI_VERSION >= NTDDI_WINXPSP2) || (_WIN32_WINNT >= 0x0502) */

#if INCL_WINSOCK_API_TYPEDEFS

typedef INT
(WSAAPI *LPFN_GETADDRINFO)(
  IN PCSTR pNodeName OPTIONAL,
  IN PCSTR pServiceName OPTIONAL,
  IN const ADDRINFOA *pHints OPTIONAL,
  OUT PADDRINFOA *ppResult);

typedef INT
(WSAAPI *LPFN_GETADDRINFOW)(
  IN PCWSTR pNodeName OPTIONAL,
  IN PCWSTR pServiceName OPTIONAL,
  IN const ADDRINFOW *pHints OPTIONAL,
  OUT PADDRINFOW *ppResult);

#define LPFN_GETADDRINFOA LPFN_GETADDRINFO

#ifdef UNICODE
#define LPFN_GETADDRINFOT LPFN_GETADDRINFOW
#else
#define LPFN_GETADDRINFOT LPFN_GETADDRINFOA
#endif

#endif /* INCL_WINSOCK_API_TYPEDEFS */

#if (_WIN32_WINNT >= 0x0600)

typedef void
(CALLBACK *LPLOOKUPSERVICE_COMPLETION_ROUTINE)(
  IN DWORD dwError,
  IN DWORD dwBytes,
  IN LPWSAOVERLAPPED lpOverlapped);

WINSOCK_API_LINKAGE
INT
WSAAPI
GetAddrInfoExA(
  IN PCSTR pName OPTIONAL,
  IN PCSTR pServiceName OPTIONAL,
  IN DWORD dwNameSpace,
  IN LPGUID lpNspId OPTIONAL,
  IN const ADDRINFOEXA *hints,
  OUT PADDRINFOEXA *ppResult,
  IN struct timeval *timeout OPTIONAL,
  IN LPOVERLAPPED lpOverlapped OPTIONAL,
  IN LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  OUT LPHANDLE lpNameHandle OPTIONAL);

WINSOCK_API_LINKAGE
INT
WSAAPI
GetAddrInfoExW(
  IN PCWSTR pName OPTIONAL,
  IN PCWSTR pServiceName OPTIONAL,
  IN DWORD dwNameSpace,
  IN LPGUID lpNspId OPTIONAL,
  IN const ADDRINFOEXW *hints OPTIONAL,
  OUT PADDRINFOEXW *ppResult,
  IN struct timeval *timeout OPTIONAL,
  IN LPOVERLAPPED lpOverlapped OPTIONAL,
  IN LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  OUT LPHANDLE lpHandle OPTIONAL);

#ifdef UNICODE
#define GetAddrInfoEx GetAddrInfoExW
#else
#define GetAddrInfoEx GetAddrInfoExA
#endif

#if INCL_WINSOCK_API_TYPEDEFS

typedef INT
(WSAAPI *LPFN_GETADDRINFOEXA)(
  IN PCSTR pName,
  IN PCSTR pServiceName OPTIONAL,
  IN DWORD dwNameSpace,
  IN LPGUID lpNspId OPTIONAL,
  IN const ADDRINFOEXA *hints OPTIONAL,
  OUT PADDRINFOEXA *ppResult,
  IN struct timeval *timeout OPTIONAL,
  IN LPOVERLAPPED lpOverlapped OPTIONAL,
  IN LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  OUT LPHANDLE lpNameHandle OPTIONAL);

typedef INT
(WSAAPI *LPFN_GETADDRINFOEXW)(
  IN PCWSTR pName,
  IN PCWSTR pServiceName OPTIONAL,
  IN DWORD dwNameSpace,
  IN LPGUID lpNspId OPTIONAL,
  IN const ADDRINFOEXW *hints OPTIONAL,
  OUT PADDRINFOEXW *ppResult,
  IN struct timeval *timeout OPTIONAL,
  IN LPOVERLAPPED lpOverlapped OPTIONAL,
  IN LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  OUT LPHANDLE lpHandle OPTIONAL);

#ifdef UNICODE
#define LPFN_GETADDRINFOEX LPFN_GETADDRINFOEXW
#else
#define LPFN_GETADDRINFOEX LPFN_GETADDRINFOEXA
#endif
#endif

#endif

#if (_WIN32_WINNT >= 0x0600)

WINSOCK_API_LINKAGE
INT
WSAAPI
SetAddrInfoExA(
  IN PCSTR pName,
  IN PCSTR pServiceName OPTIONAL,
  IN SOCKET_ADDRESS *pAddresses OPTIONAL,
  IN DWORD dwAddressCount,
  IN LPBLOB lpBlob OPTIONAL,
  IN DWORD dwFlags,
  IN DWORD dwNameSpace,
  IN LPGUID lpNspId OPTIONAL,
  IN struct timeval *timeout OPTIONAL,
  IN LPOVERLAPPED lpOverlapped OPTIONAL,
  IN LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  OUT LPHANDLE lpNameHandle OPTIONAL);

WINSOCK_API_LINKAGE
INT
WSAAPI
SetAddrInfoExW(
  IN PCWSTR pName,
  IN PCWSTR pServiceName OPTIONAL,
  IN SOCKET_ADDRESS *pAddresses OPTIONAL,
  IN DWORD dwAddressCount,
  IN LPBLOB lpBlob OPTIONAL,
  IN DWORD dwFlags,
  IN DWORD dwNameSpace,
  IN LPGUID lpNspId OPTIONAL,
  IN struct timeval *timeout OPTIONAL,
  IN LPOVERLAPPED lpOverlapped OPTIONAL,
  IN LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  OUT LPHANDLE lpNameHandle OPTIONAL);

#ifdef UNICODE
#define SetAddrInfoEx SetAddrInfoExW
#else
#define SetAddrInfoEx SetAddrInfoExA
#endif

#if INCL_WINSOCK_API_TYPEDEFS

typedef INT
(WSAAPI *LPFN_SETADDRINFOEXA)(
  IN PCSTR pName,
  IN PCSTR pServiceName OPTIONAL,
  IN SOCKET_ADDRESS *pAddresses OPTIONAL,
  IN DWORD dwAddressCount,
  IN LPBLOB lpBlob OPTIONAL,
  IN DWORD dwFlags,
  IN DWORD dwNameSpace,
  IN LPGUID lpNspId OPTIONAL,
  IN struct timeval *timeout OPTIONAL,
  IN LPOVERLAPPED lpOverlapped OPTIONAL,
  IN LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  OUT LPHANDLE lpNameHandle OPTIONAL);

typedef INT
(WSAAPI *LPFN_SETADDRINFOEXW)(
  IN PCWSTR pName,
  IN PCWSTR pServiceName OPTIONAL,
  IN SOCKET_ADDRESS *pAddresses OPTIONAL,
  IN DWORD dwAddressCount,
  IN LPBLOB lpBlob OPTIONAL,
  IN DWORD dwFlags,
  IN DWORD dwNameSpace,
  IN LPGUID lpNspId OPTIONAL,
  IN struct timeval *timeout OPTIONAL,
  IN LPOVERLAPPED lpOverlapped OPTIONAL,
  IN LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  OUT LPHANDLE lpNameHandle OPTIONAL);

#ifdef UNICODE
#define LPFN_SETADDRINFOEX LPFN_SETADDRINFOEXW
#else
#define LPFN_SETADDRINFOEX LPFN_SETADDRINFOEXA
#endif
#endif

#endif

WINSOCK_API_LINKAGE
VOID
WSAAPI
freeaddrinfo(
  IN PADDRINFOA pAddrInfo OPTIONAL);

#if (NTDDI_VERSION >= NTDDI_WINXPSP2) || (_WIN32_WINNT >= 0x0502)

WINSOCK_API_LINKAGE
VOID
WSAAPI
FreeAddrInfoW(
  IN PADDRINFOW pAddrInfo OPTIONAL);

#define FreeAddrInfoA freeaddrinfo

#ifdef UNICODE
#define FreeAddrInfo FreeAddrInfoW
#else
#define FreeAddrInfo FreeAddrInfoA
#endif
#endif

#if INCL_WINSOCK_API_TYPEDEFS

typedef VOID
(WSAAPI *LPFN_FREEADDRINFO)(
  IN PADDRINFOA pAddrInfo OPTIONAL);

typedef VOID
(WSAAPI *LPFN_FREEADDRINFOW)(
  IN PADDRINFOW pAddrInfo OPTIONAL);

#define LPFN_FREEADDRINFOA LPFN_FREEADDRINFO

#ifdef UNICODE
#define LPFN_FREEADDRINFOT LPFN_FREEADDRINFOW
#else
#define LPFN_FREEADDRINFOT LPFN_FREEADDRINFOA
#endif

#endif

#if (_WIN32_WINNT >= 0x0600)

WINSOCK_API_LINKAGE
void
WSAAPI
FreeAddrInfoEx(
  IN PADDRINFOEXA pAddrInfoEx OPTIONAL);

WINSOCK_API_LINKAGE
void
WSAAPI
FreeAddrInfoExW(
  IN PADDRINFOEXW pAddrInfoEx OPTIONAL);

#define FreeAddrInfoExA FreeAddrInfoEx

#ifdef UNICODE
#define FreeAddrInfoEx FreeAddrInfoExW
#endif

#ifdef INCL_WINSOCK_API_TYPEDEFS

typedef void
(WSAAPI *LPFN_FREEADDRINFOEXA)(
  IN PADDRINFOEXA pAddrInfoEx);

typedef void
(WSAAPI *LPFN_FREEADDRINFOEXW)(
  IN PADDRINFOEXW pAddrInfoEx);


#ifdef UNICODE
#define LPFN_FREEADDRINFOEX LPFN_FREEADDRINFOEXW
#else
#define LPFN_FREEADDRINFOEX LPFN_FREEADDRINFOEXA
#endif

#endif
#endif

typedef int socklen_t;

WINSOCK_API_LINKAGE
INT
WSAAPI
getnameinfo(
  IN const SOCKADDR *pSockaddr,
  IN socklen_t SockaddrLength,
  OUT PCHAR pNodeBuffer OPTIONAL,
  IN DWORD NodeBufferSize,
  OUT PCHAR pServiceBuffer,
  IN DWORD ServiceBufferSize,
  IN INT Flags);

#if (NTDDI_VERSION >= NTDDI_WINXPSP2) || (_WIN32_WINNT >= 0x0502)

WINSOCK_API_LINKAGE
INT
WSAAPI
GetNameInfoW(
  IN const SOCKADDR *pSockaddr,
  IN socklen_t SockaddrLength,
  OUT PWCHAR pNodeBuffer,
  IN DWORD NodeBufferSize,
  OUT PWCHAR pServiceBuffer OPTIONAL,
  IN DWORD ServiceBufferSize,
  IN INT Flags);

#define GetNameInfoA getnameinfo

#ifdef UNICODE
#define GetNameInfo GetNameInfoW
#else
#define GetNameInfo GetNameInfoA
#endif

#endif

#if INCL_WINSOCK_API_TYPEDEFS

typedef int
(WSAAPI *LPFN_GETNAMEINFO)(
  IN const SOCKADDR *pSockaddr,
  IN socklen_t SockaddrLength,
  OUT PCHAR pNodeBuffer,
  IN DWORD NodeBufferSize,
  OUT PCHAR pServiceBuffer OPTIONAL,
  IN DWORD ServiceBufferSize,
  IN INT Flags);

typedef INT
(WSAAPI *LPFN_GETNAMEINFOW)(
  IN const SOCKADDR *pSockaddr,
  IN socklen_t SockaddrLength,
  OUT PWCHAR pNodeBuffer,
  IN DWORD NodeBufferSize,
  OUT PWCHAR pServiceBuffer OPTIONAL,
  IN DWORD ServiceBufferSize,
  IN INT Flags);

#define LPFN_GETNAMEINFOA LPFN_GETNAMEINFO

#ifdef UNICODE
#define LPFN_GETNAMEINFOT LPFN_GETNAMEINFOW
#else
#define LPFN_GETNAMEINFOT LPFN_GETNAMEINFOA
#endif
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

WINSOCK_API_LINKAGE
INT
WSAAPI
inet_pton(
  IN INT Family,
  IN PCSTR pszAddrString,
  OUT PVOID pAddrBuf);

INT
WSAAPI
InetPtonW(
  IN INT Family,
  IN PCWSTR pszAddrString,
  OUT PVOID pAddrBuf);

PCSTR
WSAAPI
inet_ntop(
  IN INT Family,
  IN PVOID pAddr,
  OUT PSTR pStringBuf,
  IN size_t StringBufSize);

PCWSTR
WSAAPI
InetNtopW(
  IN INT Family,
  IN PVOID pAddr,
  OUT PWSTR pStringBuf,
  IN size_t StringBufSize);

#define InetPtonA inet_pton
#define InetNtopA inet_ntop

#ifdef UNICODE
#define InetPton InetPtonW
#define InetNtop InetNtopW
#else
#define InetPton InetPtonA
#define InetNtop InetNtopA
#endif

#if INCL_WINSOCK_API_TYPEDEFS

typedef INT
(WSAAPI *LPFN_INET_PTONA)(
  IN INT Family,
  IN PCSTR pszAddrString,
  OUT PVOID pAddrBuf);

typedef INT
(WSAAPI *LPFN_INET_PTONW)(
  IN INT Family,
  IN PCWSTR pszAddrString,
  OUT PVOID pAddrBuf);

typedef PCSTR
(WSAAPI *LPFN_INET_NTOPA)(
  IN INT Family,
  IN PVOID pAddr,
  OUT PSTR pStringBuf,
  IN size_t StringBufSize);

typedef PCWSTR
(WSAAPI *LPFN_INET_NTOPW)(
  IN INT Family,
  IN PVOID pAddr,
  OUT PWSTR pStringBuf,
  IN size_t StringBufSize);

#ifdef UNICODE
#define LPFN_INET_PTON LPFN_INET_PTONW
#define LPFN_INET_NTOP LPFN_INET_NTOPW
#else
#define LPFN_INET_PTON LPFN_INET_PTONA
#define LPFN_INET_NTOP LPFN_INET_NTOPA
#endif

#endif /* TYPEDEFS */
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if INCL_WINSOCK_API_PROTOTYPES

#ifdef UNICODE
#define gai_strerror gai_strerrorW
#else
#define gai_strerror gai_strerrorA
#endif

#define GAI_STRERROR_BUFFER_SIZE 1024

static __inline
char *
gai_strerrorA(
  IN int ecode)
{
  static char buff[GAI_STRERROR_BUFFER_SIZE + 1];

  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM
                           |FORMAT_MESSAGE_IGNORE_INSERTS
                           |FORMAT_MESSAGE_MAX_WIDTH_MASK,
                            NULL,
                            ecode,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPSTR)buff,
                            GAI_STRERROR_BUFFER_SIZE,
                            NULL);

  return buff;
}

static __inline
WCHAR *
gai_strerrorW(
  IN int ecode)
{
  static WCHAR buff[GAI_STRERROR_BUFFER_SIZE + 1];

  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
                           |FORMAT_MESSAGE_IGNORE_INSERTS
                           |FORMAT_MESSAGE_MAX_WIDTH_MASK,
                            NULL,
                            ecode,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPWSTR)buff,
                            GAI_STRERROR_BUFFER_SIZE,
                            NULL);

  return buff;
}

#endif /* INCL_WINSOCK_API_PROTOTYPES */

WS2TCPIP_INLINE
int
setipv4sourcefilter(
  IN SOCKET Socket,
  IN IN_ADDR Interface,
  IN IN_ADDR Group,
  IN MULTICAST_MODE_TYPE FilterMode,
  IN ULONG SourceCount,
  IN CONST IN_ADDR *SourceList)
{
  int Error;
  DWORD Size, Returned;
  PIP_MSFILTER Filter;

  if (SourceCount >
    (((ULONG) (ULONG_MAX - sizeof(*Filter))) / sizeof(*SourceList))) {
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
  }

  Size = IP_MSFILTER_SIZE(SourceCount);
  Filter = (PIP_MSFILTER) HeapAlloc(GetProcessHeap(), 0, Size);
  if (Filter == NULL) {
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
  }

  Filter->imsf_multiaddr = Group;
  Filter->imsf_interface = Interface;
  Filter->imsf_fmode = FilterMode;
  Filter->imsf_numsrc = SourceCount;
  if (SourceCount > 0) {
    CopyMemory(Filter->imsf_slist, SourceList,
               SourceCount * sizeof(*SourceList));
  }

  Error = WSAIoctl(Socket, SIOCSIPMSFILTER, Filter, Size, NULL, 0,
                   &Returned, NULL, NULL);

  HeapFree(GetProcessHeap(), 0, Filter);

  return Error;
}

WS2TCPIP_INLINE
int
getipv4sourcefilter(
  IN SOCKET Socket,
  IN IN_ADDR Interface,
  IN IN_ADDR Group,
  OUT MULTICAST_MODE_TYPE *FilterMode,
  IN OUT ULONG *SourceCount,
  OUT IN_ADDR *SourceList)
{
  int Error;
  DWORD Size, Returned;
  PIP_MSFILTER Filter;

  if (*SourceCount >
      (((ULONG) (ULONG_MAX - sizeof(*Filter))) / sizeof(*SourceList))) {
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
  }

  Size = IP_MSFILTER_SIZE(*SourceCount);
  Filter = (PIP_MSFILTER) HeapAlloc(GetProcessHeap(), 0, Size);
  if (Filter == NULL) {
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
  }

  Filter->imsf_multiaddr = Group;
  Filter->imsf_interface = Interface;
  Filter->imsf_numsrc = *SourceCount;

  Error = WSAIoctl(Socket, SIOCGIPMSFILTER, Filter, Size, Filter, Size,
                   &Returned, NULL, NULL);

  if (Error == 0) {
    if (*SourceCount > 0) {
        CopyMemory(SourceList, Filter->imsf_slist,
                   *SourceCount * sizeof(*SourceList));
        *SourceCount = Filter->imsf_numsrc;
    }
    *FilterMode = Filter->imsf_fmode;
  }

  HeapFree(GetProcessHeap(), 0, Filter);

  return Error;
}

#if (NTDDI_VERSION >= NTDDI_WINXP)

WS2TCPIP_INLINE
int
setsourcefilter(
  IN SOCKET Socket,
  IN ULONG Interface,
  IN CONST SOCKADDR *Group,
  IN int GroupLength,
  IN MULTICAST_MODE_TYPE FilterMode,
  IN ULONG SourceCount,
  IN CONST SOCKADDR_STORAGE *SourceList)
{
  int Error;
  DWORD Size, Returned;
  PGROUP_FILTER Filter;

  if (SourceCount >= (((ULONG) (ULONG_MAX - sizeof(*Filter))) / sizeof(*SourceList))) {
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
  }

  Size = GROUP_FILTER_SIZE(SourceCount);
  Filter = (PGROUP_FILTER) HeapAlloc(GetProcessHeap(), 0, Size);
  if (Filter == NULL) {
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
  }

  Filter->gf_interface = Interface;
  ZeroMemory(&Filter->gf_group, sizeof(Filter->gf_group));
  CopyMemory(&Filter->gf_group, Group, GroupLength);
  Filter->gf_fmode = FilterMode;
  Filter->gf_numsrc = SourceCount;
  if (SourceCount > 0) {
    CopyMemory(Filter->gf_slist, SourceList, SourceCount * sizeof(*SourceList));
  }

  Error = WSAIoctl(Socket, SIOCSMSFILTER, Filter, Size, NULL, 0, &Returned, NULL, NULL);
  HeapFree(GetProcessHeap(), 0, Filter);

  return Error;
}

WS2TCPIP_INLINE
int
getsourcefilter(
  IN SOCKET Socket,
  IN ULONG Interface,
  IN CONST SOCKADDR *Group,
  IN int GroupLength,
  OUT MULTICAST_MODE_TYPE *FilterMode,
  IN OUT ULONG *SourceCount,
  OUT SOCKADDR_STORAGE *SourceList)
{
  int Error;
  DWORD Size, Returned;
  PGROUP_FILTER Filter;

  if (*SourceCount > (((ULONG) (ULONG_MAX - sizeof(*Filter))) / sizeof(*SourceList))) {
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
  }

  Size = GROUP_FILTER_SIZE(*SourceCount);
  Filter = (PGROUP_FILTER) HeapAlloc(GetProcessHeap(), 0, Size);
  if (Filter == NULL) {
    WSASetLastError(WSAENOBUFS);
    return SOCKET_ERROR;
  }

  Filter->gf_interface = Interface;
  ZeroMemory(&Filter->gf_group, sizeof(Filter->gf_group));
  CopyMemory(&Filter->gf_group, Group, GroupLength);
  Filter->gf_numsrc = *SourceCount;

  Error = WSAIoctl(Socket, SIOCGMSFILTER, Filter, Size, Filter, Size, &Returned, NULL, NULL);
  if (Error == 0) {
    if (*SourceCount > 0) {
      CopyMemory(SourceList, Filter->gf_slist, *SourceCount * sizeof(*SourceList));
      *SourceCount = Filter->gf_numsrc;
    }
    *FilterMode = Filter->gf_fmode;
  }

  HeapFree(GetProcessHeap(), 0, Filter);

  return Error;
}
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#ifdef IDEAL_SEND_BACKLOG_IOCTLS

WS2TCPIP_INLINE
int
idealsendbacklogquery(
  IN SOCKET s,
  OUT ULONG *pISB)
{
  DWORD bytes;

  return WSAIoctl(s, SIO_IDEAL_SEND_BACKLOG_QUERY,
                  NULL, 0, pISB, sizeof(*pISB), &bytes, NULL, NULL);
}

WS2TCPIP_INLINE
int
idealsendbacklognotify(
  IN SOCKET s,
  IN LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL)
{
  DWORD bytes;

  return WSAIoctl(s, SIO_IDEAL_SEND_BACKLOG_CHANGE,
                  NULL, 0, NULL, 0, &bytes,
                  lpOverlapped, lpCompletionRoutine);
}

#endif /* IDEAL_SEND_BACKLOG_IOCTLS */

#if (_WIN32_WINNT >= 0x0600)

#ifdef _SECURE_SOCKET_TYPES_DEFINED_

WINSOCK_API_LINKAGE
INT
WSAAPI
WSASetSocketSecurity(
  IN SOCKET Socket,
  IN const SOCKET_SECURITY_SETTINGS *SecuritySettings OPTIONAL,
  IN ULONG SecuritySettingsLen,
  IN LPWSAOVERLAPPED Overlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAQuerySocketSecurity(
  IN SOCKET Socket,
  IN const SOCKET_SECURITY_QUERY_TEMPLATE *SecurityQueryTemplate OPTIONAL,
  IN ULONG SecurityQueryTemplateLen,
  OUT SOCKET_SECURITY_QUERY_INFO* SecurityQueryInfo OPTIONAL,
  IN OUT ULONG *SecurityQueryInfoLen,
  IN LPWSAOVERLAPPED Overlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSASetSocketPeerTargetName(
  IN SOCKET Socket,
  IN const SOCKET_PEER_TARGET_NAME *PeerTargetName,
  IN ULONG PeerTargetNameLen,
  IN LPWSAOVERLAPPED Overlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSADeleteSocketPeerTargetName(
  IN SOCKET Socket,
  IN const struct sockaddr *PeerAddr,
  IN ULONG PeerAddrLen,
  IN LPWSAOVERLAPPED Overlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAImpersonateSocketPeer(
  IN SOCKET Socket,
  IN const struct sockaddr *PeerAddr OPTIONAL,
  IN ULONG PeerAddrLen);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSARevertImpersonation(VOID);

#endif /* _SECURE_SOCKET_TYPES_DEFINED_ */
#endif /* (_WIN32_WINNT >= 0x0600) */

#if !defined(_WIN32_WINNT) || (_WIN32_WINNT <= 0x0500)
#include <wspiapi.h>
#endif

#ifdef __cplusplus
}
#endif
