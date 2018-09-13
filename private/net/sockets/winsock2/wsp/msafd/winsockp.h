/*++

Copyright (c) 1992-1996 Microsoft Corporation

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
// System include files.
//

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
#include <wchar.h>

#include <windef.h>
#include <winbase.h>
#include <tdi.h>

//
// Global sockets include files.
//

#include <winsock2.h>
#include <ws2spi.h>
#include <ws2help.h>
#include <afd.h>
#include <wsahelp.h>
#include <mswsock.h>

#include <afdsan.h>
#ifdef _AFD_SAN_SWITCH_
#include <ws2san.h>

#if DBG
#define DEBUG_TRACING	1		// enable DT_DLL tracing
#endif // DBG

#endif //_AFD_SAN_SWITCH_

#define MAX_FAST_TDI_ADDRESS        32
#define MAX_FAST_LISTEN_RESPONSE    32
#define MAX_FAST_HANDLE_CONTEXT     256
#define MAX_FAST_AFD_OPEN_PACKET    96
#define MAX_FAST_AFD_POLL           (sizeof(AFD_POLL_INFO) + \
                                        3*sizeof(AFD_POLL_HANDLE_INFO))
#define MAX_ASYNC_THREAD_IDLE_TIME  (5*60)  // seconds

//
// Local sockets include files.
//

#include "socktype.h"
#include "sockproc.h"
#include "sockdata.h"

#ifdef _AFD_SAN_SWITCH_
#include "sandebug.h"
#include "dt_dll.h"
#include "sandthk.h"
#endif //_AFD_SAN_SWITCH_

#define IS_DGRAM_SOCK(s)  ((s->ServiceFlags1 & XP1_CONNECTIONLESS) != 0)

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
#define WINSOCK_DEBUG_B              0x00000040
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
#ifdef _AFD_SAN_SWITCH_
#define WINSOCK_DEBUG_SAN_PROVIDER   0x02000000
#else //_AFD_SAN_SWITCH_
#define WINSOCK_DEBUG_ASYNC_GETXBYY  0x02000000
#endif //_AFD_SAN_SWITCH_
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
    IN LONG_PTR ReturnCode,
    IN BOOLEAN Failed,
    IN INT error
    );
#define WS_EXIT(a,b,c) WsExitApiCall(a,b,c,err)

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
// Exception filter.
//

LONG
SockExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR SourceFile,
    LONG LineNumber
    );

#define SOCK_EXCEPTION_FILTER()                                             \
            SockExceptionFilter(                                            \
                GetExceptionInformation(),                                  \
                (LPSTR)__FILE__,                                            \
                (LONG)__LINE__                                              \
                )

#else // if DBG

#define IF_DEBUG(a) if (0)
#define WS_PRINT(a)
#define WS_ENTER(a,b,c,d,e)
#define WS_EXIT(a,b,c)
#define WS_ASSERT(a)

#define ALLOCATE_HEAP(a) RtlAllocateHeap( SockPrivateHeap, 0, a )
#define FREE_HEAP(a) RtlFreeHeap( SockPrivateHeap, 0, a )
#define CHECK_HEAP

#define SOCK_EXCEPTION_FILTER() EXCEPTION_EXECUTE_HANDLER

#endif // if DBG

//
// Per-thread data allocation routines.
//
// Note that the per-thread data is always allocated from the process
// heap, never from any private heap created for this DLL. This prevents
// a nasty problem where an app loads the winsock DLLs (creating the private
// heap), a thread issues a winsock API (creating the per-thread data),
// the app then unloads winsock (destroying the private heap), reloads
// winsock (creating a new private heap), then the same thread from above
// issues another winsock API. The problem is that the winsock value in
// the threads TEB contains a "stale" pointer into the old private heap.
// This can lead to some nasty access violations and heap corruption.
//
// To prevent this, we always allocate the per-thread data from the
// process heap, which doesn't get destroyed until the process exits.
//

#define ALLOCATE_THREAD_DATA(a) RtlAllocateHeap( RtlProcessHeap(), 0, a )
#define FREE_THREAD_DATA(a) RtlFreeHeap( RtlProcessHeap(), 0, a )

VOID
WsPrintSockaddr (
    IN PSOCKADDR Sockaddr,
    IN PINT SockaddrLength
    );

//
// Alignment macros.
//

#define ALIGN_DOWN(count,size) \
            ((ULONG_PTR)(count) & ~((ULONG_PTR)(size) - 1))

#define ALIGN_UP(count,size) \
            (ALIGN_DOWN( (ULONG_PTR)(count)+(ULONG_PTR)(size)-1, (ULONG_PTR)(size) ))

#define ALIGN_8(count) \
            (ALIGN_UP( (ULONG_PTR)(count), 8 ))

//
// All available multipoint flags.
//

#define ALL_MULTIPOINT_FLAGS ( WSA_FLAG_MULTIPOINT_C_ROOT | \
                               WSA_FLAG_MULTIPOINT_C_LEAF | \
                               WSA_FLAG_MULTIPOINT_D_ROOT | \
                               WSA_FLAG_MULTIPOINT_D_LEAF )

//
// Don't do the debug stuff on the per-socket locks, as the debug data
// really bloats out the critical section and we don't want to pay
// the penalty on every socket.
//

#define SockAcquireSocketLockExclusive(_Socket) EnterCriticalSection( &_Socket->Lock )
#define SockReleaseSocketLock(_Socket) LeaveCriticalSection( &_Socket->Lock )

#endif // ndef _WINSOCKP_
