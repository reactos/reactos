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
#define WS2TCPIP_INLINE static inline
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
  _In_opt_ PCSTR pNodeName,
  _In_opt_ PCSTR pServiceName,
  _In_opt_ const ADDRINFOA *pHints,
  _Outptr_ PADDRINFOA *ppResult);

#if (NTDDI_VERSION >= NTDDI_WINXPSP2) || (_WIN32_WINNT >= 0x0502)

WINSOCK_API_LINKAGE
INT
WSAAPI
GetAddrInfoW(
  _In_opt_ PCWSTR pNodeName,
  _In_opt_ PCWSTR pServiceName,
  _In_opt_ const ADDRINFOW *pHints,
  _Outptr_ PADDRINFOW *ppResult);

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
  _In_opt_ PCSTR pNodeName,
  _In_opt_ PCSTR pServiceName,
  _In_opt_ const ADDRINFOA *pHints,
  _Outptr_ PADDRINFOA *ppResult);

typedef INT
(WSAAPI *LPFN_GETADDRINFOW)(
  _In_opt_ PCWSTR pNodeName,
  _In_opt_ PCWSTR pServiceName,
  _In_opt_ const ADDRINFOW *pHints,
  _Outptr_ PADDRINFOW *ppResult);

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
  _In_ DWORD dwError,
  _In_ DWORD dwBytes,
  _In_ LPWSAOVERLAPPED lpOverlapped);

WINSOCK_API_LINKAGE
INT
WSAAPI
GetAddrInfoExA(
  _In_opt_ PCSTR pName,
  _In_opt_ PCSTR pServiceName,
  _In_ DWORD dwNameSpace,
  _In_opt_ LPGUID lpNspId,
  _In_opt_ const ADDRINFOEXA *hints,
  _Outptr_ PADDRINFOEXA *ppResult,
  _In_opt_ struct timeval *timeout,
  _In_opt_ LPOVERLAPPED lpOverlapped,
  _In_opt_ LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine,
  _Out_opt_ LPHANDLE lpNameHandle);

WINSOCK_API_LINKAGE
INT
WSAAPI
GetAddrInfoExW(
  _In_opt_ PCWSTR pName,
  _In_opt_ PCWSTR pServiceName,
  _In_ DWORD dwNameSpace,
  _In_opt_ LPGUID lpNspId,
  _In_opt_ const ADDRINFOEXW *hints,
  _Outptr_ PADDRINFOEXW *ppResult,
  _In_opt_ struct timeval *timeout,
  _In_opt_ LPOVERLAPPED lpOverlapped,
  _In_opt_ LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine,
  _Out_opt_ LPHANDLE lpHandle);

#ifdef UNICODE
#define GetAddrInfoEx GetAddrInfoExW
#else
#define GetAddrInfoEx GetAddrInfoExA
#endif

#if INCL_WINSOCK_API_TYPEDEFS

typedef INT
(WSAAPI *LPFN_GETADDRINFOEXA)(
  _In_ PCSTR pName,
  _In_opt_ PCSTR pServiceName,
  _In_ DWORD dwNameSpace,
  _In_opt_ LPGUID lpNspId,
  _In_opt_ const ADDRINFOEXA *hints,
  _Outptr_ PADDRINFOEXA *ppResult,
  _In_opt_ struct timeval *timeout,
  _In_opt_ LPOVERLAPPED lpOverlapped,
  _In_opt_ LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine,
  _Out_opt_ LPHANDLE lpNameHandle);

typedef INT
(WSAAPI *LPFN_GETADDRINFOEXW)(
  _In_ PCWSTR pName,
  _In_opt_ PCWSTR pServiceName,
  _In_ DWORD dwNameSpace,
  _In_opt_ LPGUID lpNspId,
  _In_opt_ const ADDRINFOEXW *hints,
  _Outptr_ PADDRINFOEXW *ppResult,
  _In_opt_ struct timeval *timeout,
  _In_opt_ LPOVERLAPPED lpOverlapped,
  _In_opt_ LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine,
  _Out_opt_ LPHANDLE lpHandle);

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
  _In_ PCSTR pName,
  _In_opt_ PCSTR pServiceName,
  _In_opt_ SOCKET_ADDRESS *pAddresses,
  _In_ DWORD dwAddressCount,
  _In_opt_ LPBLOB lpBlob,
  _In_ DWORD dwFlags,
  _In_ DWORD dwNameSpace,
  _In_opt_ LPGUID lpNspId,
  _In_opt_ struct timeval *timeout,
  _In_opt_ LPOVERLAPPED lpOverlapped,
  _In_opt_ LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine,
  _Out_opt_ LPHANDLE lpNameHandle);

WINSOCK_API_LINKAGE
INT
WSAAPI
SetAddrInfoExW(
  _In_ PCWSTR pName,
  _In_opt_ PCWSTR pServiceName,
  _In_opt_ SOCKET_ADDRESS *pAddresses,
  _In_ DWORD dwAddressCount,
  _In_opt_ LPBLOB lpBlob,
  _In_ DWORD dwFlags,
  _In_ DWORD dwNameSpace,
  _In_opt_ LPGUID lpNspId,
  _In_opt_ struct timeval *timeout,
  _In_opt_ LPOVERLAPPED lpOverlapped,
  _In_opt_ LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine,
  _Out_opt_ LPHANDLE lpNameHandle);

#ifdef UNICODE
#define SetAddrInfoEx SetAddrInfoExW
#else
#define SetAddrInfoEx SetAddrInfoExA
#endif

#if INCL_WINSOCK_API_TYPEDEFS

typedef INT
(WSAAPI *LPFN_SETADDRINFOEXA)(
  _In_ PCSTR pName,
  _In_opt_ PCSTR pServiceName,
  _In_opt_ SOCKET_ADDRESS *pAddresses,
  _In_ DWORD dwAddressCount,
  _In_opt_ LPBLOB lpBlob,
  _In_ DWORD dwFlags,
  _In_ DWORD dwNameSpace,
  _In_opt_ LPGUID lpNspId,
  _In_opt_ struct timeval *timeout,
  _In_opt_ LPOVERLAPPED lpOverlapped,
  _In_opt_ LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine,
  _Out_opt_ LPHANDLE lpNameHandle);

typedef INT
(WSAAPI *LPFN_SETADDRINFOEXW)(
  _In_ PCWSTR pName,
  _In_opt_ PCWSTR pServiceName,
  _In_opt_ SOCKET_ADDRESS *pAddresses,
  _In_ DWORD dwAddressCount,
  _In_opt_ LPBLOB lpBlob,
  _In_ DWORD dwFlags,
  _In_ DWORD dwNameSpace,
  _In_opt_ LPGUID lpNspId,
  _In_opt_ struct timeval *timeout,
  _In_opt_ LPOVERLAPPED lpOverlapped,
  _In_opt_ LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine,
  _Out_opt_ LPHANDLE lpNameHandle);

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
  _In_opt_ PADDRINFOA pAddrInfo);

#if (NTDDI_VERSION >= NTDDI_WINXPSP2) || (_WIN32_WINNT >= 0x0502)

WINSOCK_API_LINKAGE
VOID
WSAAPI
FreeAddrInfoW(
  _In_opt_ PADDRINFOW pAddrInfo);

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
  _In_opt_ PADDRINFOA pAddrInfo);

typedef VOID
(WSAAPI *LPFN_FREEADDRINFOW)(
  _In_opt_ PADDRINFOW pAddrInfo);

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
  _In_opt_ PADDRINFOEXA pAddrInfoEx);

WINSOCK_API_LINKAGE
void
WSAAPI
FreeAddrInfoExW(
  _In_opt_ PADDRINFOEXW pAddrInfoEx);

#define FreeAddrInfoExA FreeAddrInfoEx

#ifdef UNICODE
#define FreeAddrInfoEx FreeAddrInfoExW
#endif

#ifdef INCL_WINSOCK_API_TYPEDEFS

typedef void
(WSAAPI *LPFN_FREEADDRINFOEXA)(
  _In_ PADDRINFOEXA pAddrInfoEx);

typedef void
(WSAAPI *LPFN_FREEADDRINFOEXW)(
  _In_ PADDRINFOEXW pAddrInfoEx);


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
  _In_reads_bytes_(SockaddrLength) const SOCKADDR *pSockaddr,
  _In_ socklen_t SockaddrLength,
  _Out_writes_opt_(NodeBufferSize) PCHAR pNodeBuffer,
  _In_ DWORD NodeBufferSize,
  _Out_writes_opt_(ServiceBufferSize) PCHAR pServiceBuffer,
  _In_ DWORD ServiceBufferSize,
  _In_ INT Flags);

#if (NTDDI_VERSION >= NTDDI_WINXPSP2) || (_WIN32_WINNT >= 0x0502)

WINSOCK_API_LINKAGE
INT
WSAAPI
GetNameInfoW(
  _In_reads_bytes_(SockaddrLength) const SOCKADDR *pSockaddr,
  _In_ socklen_t SockaddrLength,
  _Out_writes_opt_(NodeBufferSize) PWCHAR pNodeBuffer,
  _In_ DWORD NodeBufferSize,
  _Out_writes_opt_(ServiceBufferSize) PWCHAR pServiceBuffer,
  _In_ DWORD ServiceBufferSize,
  _In_ INT Flags);

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
  _In_reads_bytes_(SockaddrLength) const SOCKADDR *pSockaddr,
  _In_ socklen_t SockaddrLength,
  _Out_writes_opt_(NodeBufferSize) PCHAR pNodeBuffer,
  _In_ DWORD NodeBufferSize,
  _Out_writes_opt_(ServiceBufferSize) PCHAR pServiceBuffer,
  _In_ DWORD ServiceBufferSize,
  _In_ INT Flags);

typedef INT
(WSAAPI *LPFN_GETNAMEINFOW)(
  _In_reads_bytes_(SockaddrLength) const SOCKADDR *pSockaddr,
  _In_ socklen_t SockaddrLength,
  _Out_writes_opt_(NodeBufferSize) PWCHAR pNodeBuffer,
  _In_ DWORD NodeBufferSize,
  _Out_writes_opt_(ServiceBufferSize) PWCHAR pServiceBuffer,
  _In_ DWORD ServiceBufferSize,
  _In_ INT Flags);

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
  _In_ INT Family,
  _In_ PCSTR pszAddrString,
  _Out_writes_bytes_(sizeof(IN6_ADDR)) PVOID pAddrBuf);

INT
WSAAPI
InetPtonW(
  _In_ INT Family,
  _In_ PCWSTR pszAddrString,
  _Out_writes_bytes_(sizeof(IN6_ADDR)) PVOID pAddrBuf);

PCSTR
WSAAPI
inet_ntop(
  _In_ INT Family,
  _In_ const VOID *pAddr,
  _Out_writes_(StringBufSize) PSTR pStringBuf,
  _In_ size_t StringBufSize);

PCWSTR
WSAAPI
InetNtopW(
  _In_ INT Family,
  _In_ const VOID *pAddr,
  _Out_writes_(StringBufSize) PWSTR pStringBuf,
  _In_ size_t StringBufSize);

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
  _In_ INT Family,
  _In_ PCSTR pszAddrString,
  _Out_writes_bytes_(sizeof(IN6_ADDR)) PVOID pAddrBuf);

typedef INT
(WSAAPI *LPFN_INET_PTONW)(
  _In_ INT Family,
  _In_ PCWSTR pszAddrString,
  _Out_writes_bytes_(sizeof(IN6_ADDR)) PVOID pAddrBuf);

typedef PCSTR
(WSAAPI *LPFN_INET_NTOPA)(
  _In_ INT Family,
  _In_ PVOID pAddr,
  _Out_writes_(StringBufSize) PSTR pStringBuf,
  _In_ size_t StringBufSize);

typedef PCWSTR
(WSAAPI *LPFN_INET_NTOPW)(
  _In_ INT Family,
  _In_ PVOID pAddr,
  _Out_writes_(StringBufSize) PWSTR pStringBuf,
  _In_ size_t StringBufSize);

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
  _In_ int ecode)
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
  _In_ int ecode)
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
  _In_ SOCKET Socket,
  _In_ IN_ADDR Interface,
  _In_ IN_ADDR Group,
  _In_ MULTICAST_MODE_TYPE FilterMode,
  _In_ ULONG SourceCount,
  _In_reads_(SourceCount) CONST IN_ADDR *SourceList)
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

_Success_(return == 0)
WS2TCPIP_INLINE
int
getipv4sourcefilter(
  _In_ SOCKET Socket,
  _In_ IN_ADDR Interface,
  _In_ IN_ADDR Group,
  _Out_ MULTICAST_MODE_TYPE *FilterMode,
  _Inout_ ULONG *SourceCount,
  _Out_writes_(*SourceCount) IN_ADDR *SourceList)
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
  _In_ SOCKET Socket,
  _In_ ULONG Interface,
  _In_ CONST SOCKADDR *Group,
  _In_ int GroupLength,
  _In_ MULTICAST_MODE_TYPE FilterMode,
  _In_ ULONG SourceCount,
  _In_reads_(SourceCount) CONST SOCKADDR_STORAGE *SourceList)
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

_Success_(return == 0)
WS2TCPIP_INLINE
int
getsourcefilter(
  _In_ SOCKET Socket,
  _In_ ULONG Interface,
  _In_ CONST SOCKADDR *Group,
  _In_ int GroupLength,
  _Out_ MULTICAST_MODE_TYPE *FilterMode,
  _Inout_ ULONG *SourceCount,
  _Out_writes_(*SourceCount) SOCKADDR_STORAGE *SourceList)
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
  _In_ SOCKET s,
  _Out_ ULONG *pISB)
{
  DWORD bytes;

  return WSAIoctl(s, SIO_IDEAL_SEND_BACKLOG_QUERY,
                  NULL, 0, pISB, sizeof(*pISB), &bytes, NULL, NULL);
}

WS2TCPIP_INLINE
int
idealsendbacklognotify(
  _In_ SOCKET s,
  _In_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
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
  _In_ SOCKET Socket,
  _In_reads_bytes_opt_(SecuritySettingsLen) const SOCKET_SECURITY_SETTINGS *SecuritySettings,
  _In_ ULONG SecuritySettingsLen,
  _In_opt_ LPWSAOVERLAPPED Overlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAQuerySocketSecurity(
  _In_ SOCKET Socket,
  _In_reads_bytes_opt_(SecurityQueryTemplateLen) const SOCKET_SECURITY_QUERY_TEMPLATE *SecurityQueryTemplate,
  _In_ ULONG SecurityQueryTemplateLen,
  _Out_writes_bytes_to_opt_(*SecurityQueryInfoLen, *SecurityQueryInfoLen) SOCKET_SECURITY_QUERY_INFO* SecurityQueryInfo,
  _Inout_ ULONG *SecurityQueryInfoLen,
  _In_opt_ LPWSAOVERLAPPED Overlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSASetSocketPeerTargetName(
  _In_ SOCKET Socket,
  _In_reads_bytes_(PeerTargetNameLen) const SOCKET_PEER_TARGET_NAME *PeerTargetName,
  _In_ ULONG PeerTargetNameLen,
  _In_opt_ LPWSAOVERLAPPED Overlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSADeleteSocketPeerTargetName(
  _In_ SOCKET Socket,
  _In_reads_bytes_(PeerAddrLen) const struct sockaddr *PeerAddr,
  _In_ ULONG PeerAddrLen,
  _In_opt_ LPWSAOVERLAPPED Overlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAImpersonateSocketPeer(
  _In_ SOCKET Socket,
  _In_reads_bytes_opt_(PeerAddrLen) const struct sockaddr *PeerAddr,
  _In_ ULONG PeerAddrLen);

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
