/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockProc.h

Abstract:

    This module contains prototypes for WinSock support routines.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#ifndef _SOCKPROC_
#define _SOCKPROC_

//
// Routines for interacting with the asynchronous processing thread.
//

PWINSOCK_CONTEXT_BLOCK
SockAllocateContextBlock (
    VOID
    );

BOOLEAN
SockCheckAndInitAsyncThread (
    VOID
    );

VOID
SockFreeContextBlock (
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    );

VOID
SockQueueRequestToAsyncThread(
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    );

VOID
SockProcessAsyncGetHost (
    IN DWORD TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN int Length,
    IN int Type,
    IN char FAR *Buffer,
    IN int BufferLength
    );

VOID
SockProcessAsyncGetProto (
    IN DWORD TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Buffer,
    IN int BufferLength
    );

VOID
SockProcessAsyncGetServ (
    IN DWORD TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Protocol,
    IN char FAR *Buffer,
    IN int BufferLength
    );

VOID
SockTerminateAsyncThread (
    VOID
    );

//
// Routine called at every entrypoint of the winsock DLL.
//

BOOLEAN
SockEnterApi (
    IN BOOLEAN MustBeStarted,
    IN BOOLEAN BlockingIllegal,
    IN BOOLEAN GetXByYCall
    );

//
// Blocking hook stuff.
//

BOOL
SockDefaultBlockingHook (
    VOID
    );

BOOL
SockBlockingHookHelper(
    DWORD Context
    );

//
// DLL initialization routines.
//

BOOL
SockInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    );

BOOL
SockThreadInitialize(
    VOID
    );

//
// Resolver subroutines.
//

int
dn_expand(
    IN  unsigned char *msg,
    IN  unsigned char *eomorig,
    IN  unsigned char *comp_dn,
    OUT unsigned char *exp_dn,
    IN  int            length
    );

static
int
dn_find(
    unsigned char  *exp_dn,
    unsigned char  *msg,
    unsigned char **dnptrs,
    unsigned char **lastdnptr
    );

int
dn_skipname(
    unsigned char *comp_dn,
    unsigned char *eom
    );

void
fp_query(
    char *msg,
    FILE *file
    );

int
gethostname(
    OUT char *name,
    IN int namelen
    );

void
p_query(
    char *msg
    );

extern
void
putshort(
    u_short s,
    u_char *msgp
    );

void
putlong(
    u_long l,
    u_char *msgp
    );

int
strcasecmp(
    char *s1,
    char *s2
    );

int
strncasecmp(
    char *s1,
    char *s2,
    int   n
    );

//
// DNR stuff.
//

#if PACKETSZ > 1024
#define MAXPACKET       PACKETSZ
#else
#define MAXPACKET       1024
#endif

typedef union {
    HEADER hdr;
    unsigned char buf[MAXPACKET];
} querybuf;

typedef union {
    long al;
    char ac;
} align;


struct hostent *
myhostent (
    void
    );

struct hostent *
myhostent_W (
    void
    );

struct hostent *
localhostent (
    void
    );

struct hostent *
localhostent_W (
    void
    );

struct hostent *
dnshostent (
    void
    );

BOOL
querydnsaddrs (
    IN LPDWORD *Array,
    IN PVOID Buffer
    );

DWORD
BytesInHostent (
    PHOSTENT Hostent
    );

DWORD
CopyHostentToBuffer (
    char FAR *Buffer,
    int BufferLength,
    PHOSTENT Hostent
    );

struct hostent *
_gethtbyname (
    IN char *name
    );

ULONG
SockNbtResolveName (
    IN  PCHAR    Name,
    OUT WORD *   pwIpCount OPTIONAL,
    OUT PULONG * pIpArray  OPTIONAL
    );

PHOSTENT
QueryHostentCache (
    IN LPSTR Name OPTIONAL,
    IN DWORD IpAddress OPTIONAL
    );


//
// DNS Caching Resolver Service helper routine
//

struct hostent *
QueryDnsCache(
    IN  LPSTR    pszName,
    IN  WORD     wType,
    IN  DWORD    Options,
    IN  PVOID * pMsg OPTIONAL
    );

struct hostent *
QueryDnsCache_W(
    IN  LPWSTR   pszName,
    IN  WORD     wType,
    IN  DWORD    Options,
    IN  PVOID * pMsg OPTIONAL
    );

struct hostent *
QueryNbtWins(
    IN  LPSTR    pszOemName,
    IN  LPSTR    pszAnsiName
    );


#endif // ndef _SOCKPROC_


