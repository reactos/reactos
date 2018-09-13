/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    WinSockP.h

Abstract:

    This in the local include file for files in the winsock directory.
    It includes all necessary files.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#ifndef _WINSOCKP_
#define _WINSOCKP_

//
// Turn off "declspec" decoration of entrypoints defined in WINSOCK2.H.
//

#define WINSOCK_API_LINKAGE

//
// Map the external (exported) names to our internal names. We must
// do this so we can statically link with WS2_32.DLL, which also exports
// these names.
//


//
// System include files.
//

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <tdi.h>
#include <sys\uio.h>
#include <crt\ctype.h>

//
// Global sockets include files.
//

#include <winsock2.h>
#include <ws2spi.h>
#include <mswsock.h>
#include <afd.h>
#include <wsahelp.h>
#include <sockets\arpa\nameser.h>
#include <sockets\resolv.h>
#include <wsasetup.h>

#define PROTODB_SIZE    (_MAX_PATH + 10)
#define SERVDB_SIZE     (_MAX_PATH + 10)

#define MAX_FAST_TDI_ADDRESS        32
#define MAX_FAST_LISTEN_RESPONSE    32
#define MAX_FAST_HANDLE_CONTEXT     256
#define MAX_FAST_AFD_OPEN_PACKET    96
#define MAX_FAST_AFD_POLL           (sizeof(AFD_POLL_INFO) + \
                                        3*sizeof(AFD_POLL_HANDLE_INFO))

//
// Local sockets include files.
//

#include "socktype.h"
#include "sockproc.h"
#include "sockdata.h"
#include "sockreg.h"

extern char VTCPPARM[];
extern char NTCPPARM[];
extern char TCPPARM[];
extern char TTCPPARM[];

#define bcopy(s, d, c)  memcpy((u_char *)(d), (u_char *)(s), (c))
#define bzero(d, l)     memset((d), '\0', (l))
#define bcmp(s1, s2, l) memcmp((s1), (s2), (l))

#define IS_DGRAM_SOCK(type)  (((type) == SOCK_DGRAM) || ((type) == SOCK_RAW))

//
// Accessors for per-thread data.
//

#if defined(USE_TEB_FIELD)
#define GET_THREAD_DATA()   (NtCurrentTeb()->WinSockData)
#define SET_THREAD_DATA(p)  ((NtCurrentTeb()->WinSockData = (p)),TRUE)
#else   // !USE_TEB_FIELD
#define GET_THREAD_DATA()   TlsGetValue(SockTlsSlot)
#define SET_THREAD_DATA(p)  TlsSetValue(SockTlsSlot, (LPVOID)(p))
#endif  // USE_TEB_FIELD

//
// Manifests for SockWaitForSingleObject().
//

#define SOCK_ALWAYS_CALL_BLOCKING_HOOK        1
#define SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK 2
#define SOCK_NEVER_CALL_BLOCKING_HOOK         3

#define SOCK_NO_TIMEOUT      4
#define SOCK_SEND_TIMEOUT    5
#define SOCK_RECEIVE_TIMEOUT 6

#if DBG

#define WINSOCK_DEBUG_CONSOLE        0x00000001
#define WINSOCK_DEBUG_FILE           0x00000002
#define WINSOCK_DEBUG_DEBUGGER       0x00000004
#define WINSOCK_DEBUG_EVENT_SELECT   0x00000008

#define WINSOCK_DEBUG_ENTER          0x00000010
#define WINSOCK_DEBUG_EXIT           0x00000020
#define WINSOCK_DEBUG_SETUP          0x00000040
#define WINSOCK_DEBUG_C              0x00000080

#define WINSOCK_DEBUG_ACCEPT         0x00000100
#define WINSOCK_DEBUG_BIND           0x00000200
#define WINSOCK_DEBUG_LISTEN         0x00000400
#define WINSOCK_DEBUG_SOCKET         0x00000800

#define WINSOCK_DEBUG_CONNECT        0x00001000
#define WINSOCK_DEBUG_SEND           0x00002000
#define WINSOCK_DEBUG_RECEIVE        0x00004000
#define WINSOCK_DEBUG_SELECT         0x00008000

#define WINSOCK_DEBUG_GETNAME        0x00010000
#define WINSOCK_DEBUG_SOCKOPT        0x00020000
#define WINSOCK_DEBUG_SHUTDOWN       0x00040000
#define WINSOCK_DEBUG_INIT           0x00080000

#define WINSOCK_DEBUG_MISC           0x00100000
#define WINSOCK_DEBUG_CLOSE          0x00200000
#define WINSOCK_DEBUG_CANCEL         0x00400000
#define WINSOCK_DEBUG_BLOCKING       0x00800000

#define WINSOCK_DEBUG_ASYNC          0x01000000
#define WINSOCK_DEBUG_ASYNC_GETXBYY  0x02000000
#define WINSOCK_DEBUG_ASYNC_SELECT   0x04000000
#define WINSOCK_DEBUG_POST           0x08000000

#define WINSOCK_DEBUG_GETXBYY        0x10000000
#define WINSOCK_DEBUG_RESOLVER       0x20000000
#define WINSOCK_DEBUG_REGISTRY       0x40000000
#define WINSOCK_DEBUG_HELPER_DLL     0x80000000

#define IF_DEBUG(a) if ( (WsDebug & WINSOCK_DEBUG_ ## a) != 0 )

VOID
WsPrintf (
    char *Format,
    ...
    );
#define WS_PRINT(a) WsPrintf a

VOID
WsEnterApiCall (
    IN PCHAR RoutineName,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3,
    IN PVOID Arg4
    );
#define WS_ENTER(a,b,c,d,e) WsEnterApiCall(a,b,c,d,e)

VOID
WsExitApiCall (
    IN PCHAR RoutineName,
    IN INT ReturnCode,
    IN BOOLEAN Failed
    );
#define WS_EXIT(a,b,c) WsExitApiCall(a,b,c)

VOID
WsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    );
#define WS_ASSERT(exp) if (!(exp)) WsAssert( #exp, __FILE__, __LINE__ )

PVOID
SockAllocateHeap (
    IN ULONG NumberOfBytes,
    PCHAR FileName,
    ULONG LineNumber
    );

#define ALLOCATE_HEAP(bytes) SockAllocateHeap( bytes, __FILE__, __LINE__ )

VOID
SockFreeHeap (
    IN PVOID Pointer
    );

#define FREE_HEAP(a) SockFreeHeap(a)

VOID
SockCheckHeap (
    VOID
    );

#define CHECK_HEAP SockCheckHeap( )

//
// Wrappers for the lock acquisition/release routines.
//

VOID
SockAcquireGlobalLockExclusive (
    VOID
    );

VOID
SockReleaseGlobalLock (
    VOID
    );

#else // if DBG

#define IF_DEBUG(a) if (0)
#define WS_PRINT(a)
#define WS_ENTER(a,b,c,d,e)
#define WS_EXIT(a,b,c)
#define WS_ASSERT(a)

#define WINSOCK_CONSOLE_DEBUGGING ( FALSE )
#define WINSOCK_DUMP_ENTER ( FALSE )

#define ALLOCATE_HEAP(a) RtlAllocateHeap( SockPrivateHeap, 0, a )
#define FREE_HEAP(a) RtlFreeHeap( SockPrivateHeap, 0, a )
#define CHECK_HEAP

#define SockAcquireGlobalLockExclusive( ) EnterCriticalSection( pSocketLock )
#define SockReleaseGlobalLock( ) LeaveCriticalSection( pSocketLock )

#endif // if DBG

VOID
RnR2Cleanup(                       // final gasp for the RnR2 stuff
    VOID
    );

VOID
GetHostCleanup(
    VOID
    );

#endif // ndef _WINSOCKP_

