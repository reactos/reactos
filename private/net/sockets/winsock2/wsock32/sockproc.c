/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockProc.c

Abstract:

    This module contains support routines for the WinSock DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"

#include <ctype.h>
#include <stdarg.h>
#include <wincon.h>

#define MAX_BLOCKING_HOOK_CALLS 1000

#if DBG
BOOLEAN WsaStartupWarning = FALSE;
#endif


BOOLEAN
SockEnterApi (
    IN BOOLEAN MustBeStarted,
    IN BOOLEAN BlockingIllegal,
    IN BOOLEAN GetXByYCall
    )
{
    PWINSOCK_TLS_DATA tlsData;

    //
    // Bail if we're already detached from the process.
    //

    if( SockProcessTerminating ) {

        IF_DEBUG(ENTER) {
            WS_PRINT(( "SockEnterApi: process terminating\n" ));
        }

        SetLastError( WSANOTINITIALISED );
        return FALSE;
    }

    //
    // Make sure that WSAStartup has been called, if necessary.
    //

    if ( MustBeStarted && (SockWsaStartupCount == 0 || SockTerminating) ) {

        IF_DEBUG(ENTER) {
            WS_PRINT(( "SockEnterApi: WSAStartup() not called!\n" ));
        }

        SetLastError( WSANOTINITIALISED );
        return FALSE;
    }

    //
    // If this thread has not been initialized, do it now.
    //

    tlsData = GET_THREAD_DATA();

    if ( tlsData == NULL ) {

        if ( !SockThreadInitialize() ) {

            IF_DEBUG(ENTER) {
                WS_PRINT(( "SockEnterApi: SockThreadInitialize failed.\n" ));
            }

            SetLastError( WSAENOBUFS );
            return FALSE;
        }

        tlsData = GET_THREAD_DATA();
    }

    //
    // Make sure that we're not in a blocking call, if appropriate.
    //

    if ( BlockingIllegal && WSAIsBlocking() ) {

        IF_DEBUG(ENTER) {
            WS_PRINT(( "SockEnterApi: in blocking call.\n" ));
        }

        SetLastError( WSAEINPROGRESS );
        return FALSE;
    }

    //
    // If this is a GetXByY call, set up thread variables.
    //

    if ( GetXByYCall ) {
        SockThreadGetXByYCancelled = FALSE;
        SockThreadProcessingGetXByY = TRUE;
    }

    //
    // Everything's cool.  Proceed.
    //

    return TRUE;

} // SockEnterApi

#if DBG

VOID
WsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    )
{
    BOOL ok;
    CHAR choice[16];
    DWORD bytes;
    DWORD error;

    IF_DEBUG(CONSOLE) {
        WS_PRINT(( "\n failed: %s\n  at line %ld of %s\n",
                    FailedAssertion, LineNumber, FileName ));
        do {
            WS_PRINT(( "[B]reak/[I]gnore? " ));
            bytes = sizeof(choice);
            ok = ReadFile(
                    GetStdHandle(STD_INPUT_HANDLE),
                    &choice,
                    bytes,
                    &bytes,
                    NULL
                    );
            if ( ok ) {
                if ( toupper(choice[0]) == 'I' ) {
                    break;
                }
                if ( toupper(choice[0]) == 'B' ) {
                    DbgUserBreakPoint( );
                }
            } else {
                error = GetLastError( );
            }
        } while ( TRUE );

        return;
    }

    RtlAssert( FailedAssertion, FileName, LineNumber, NULL );

} // WsAssert

BOOLEAN ConsoleInitialized = FALSE;

HANDLE DebugFileHandle = INVALID_HANDLE_VALUE;
PCHAR DebugFileName = "wsdebug.log";


VOID
WsPrintf (
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;
    BOOL ret;

    length = (ULONG)wsprintfA( OutputBuffer, "WSOCK32: " );

    va_start( arglist, Format );

    wvsprintfA( OutputBuffer + length, Format, arglist );

    va_end( arglist );

    IF_DEBUG(DEBUGGER) {
        DbgPrint( "%s", OutputBuffer );
    }

    IF_DEBUG(CONSOLE) {

        if ( !ConsoleInitialized ) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            COORD coord;

            ConsoleInitialized = TRUE;
            (VOID)AllocConsole( );
            (VOID)GetConsoleScreenBufferInfo(
                    GetStdHandle(STD_OUTPUT_HANDLE),
                    &csbi
                    );
            coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
            coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
            (VOID)SetConsoleScreenBufferSize(
                    GetStdHandle(STD_OUTPUT_HANDLE),
                    coord
                    );
        }

        length = strlen( OutputBuffer );

        ret = WriteFile(
                  GetStdHandle(STD_OUTPUT_HANDLE),
                  (LPVOID )OutputBuffer,
                  length,
                  &length,
                  NULL
                  );
        if ( !ret ) {
            DbgPrint( "WsPrintf: console WriteFile failed: %ld\n",
                          GetLastError( ) );
        }
    }

    IF_DEBUG(FILE) {

        if ( DebugFileHandle == INVALID_HANDLE_VALUE ) {
            DebugFileHandle = CreateFile(
                                  DebugFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  CREATE_ALWAYS,
                                  0,
                                  NULL
                                  );
        }

        if ( DebugFileHandle == INVALID_HANDLE_VALUE ) {

            DbgPrint( "WsPrintf: Failed to open winsock debug log file %s: %ld\n",
                          DebugFileName, GetLastError( ) );

        } else {

            length = strlen( OutputBuffer );

            ret = WriteFile(
                      DebugFileHandle,
                      (LPVOID )OutputBuffer,
                      length,
                      &length,
                      NULL
                      );
            if ( !ret ) {
                DbgPrint( "WsPrintf: file WriteFile failed: %ld\n",
                              GetLastError( ) );
            }
        }
    }

} // WsPrintf

#endif

#if DBG

VOID
WsEnterApiCall (
    IN PCHAR RoutineName,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3,
    IN PVOID Arg4
    )
{
    ULONG i;

    CHECK_HEAP;

    //
    // If this thread has not been initialized, do it now.  This is
    // duplicated in SockEnterApi(), but we need it here to
    // access SockIndentLevel below.
    //

    if ( GET_THREAD_DATA() == NULL ) {
        if ( SockProcessTerminating ||
             !SockThreadInitialize() ) {
            return;
        }
    }

    IF_DEBUG(ENTER) {
        for ( i = 0; i < SockIndentLevel; i++ ) {
            WS_PRINT(( "    " ));
        }
        WS_PRINT(( "---> %s() args 0x%lx 0x%lx 0x%lx 0x%lx\n",
                      RoutineName, Arg1, Arg2, Arg3, Arg4 ));
    }

    SockIndentLevel++;

    return;

} // WsEnter

struct _ERROR_STRINGS {
    INT ErrorCode;
    PCHAR ErrorString;
} ErrorStrings[] = {
    (WSABASEERR+4),   "WSAEINTR",
    (WSABASEERR+9),   "WSAEBADF",
    (WSABASEERR+13),  "WSAEACCES",
    (WSABASEERR+14),  "WSAEFAULT",
    (WSABASEERR+22),  "WSAEINVAL",
    (WSABASEERR+24),  "WSAEMFILE",
    (WSABASEERR+35),  "WSAEWOULDBLOCK",
    (WSABASEERR+36),  "WSAEINPROGRESS",
    (WSABASEERR+37),  "WSAEALREADY",
    (WSABASEERR+38),  "WSAENOTSOCK",
    (WSABASEERR+39),  "WSAEDESTADDRREQ",
    (WSABASEERR+40),  "WSAEMSGSIZE",
    (WSABASEERR+41),  "WSAEPROTOTYPE",
    (WSABASEERR+42),  "WSAENOPROTOOPT",
    (WSABASEERR+43),  "WSAEPROTONOSUPPORT",
    (WSABASEERR+44),  "WSAESOCKTNOSUPPORT",
    (WSABASEERR+45),  "WSAEOPNOTSUPP",
    (WSABASEERR+46),  "WSAEPFNOSUPPORT",
    (WSABASEERR+47),  "WSAEAFNOSUPPORT",
    (WSABASEERR+48),  "WSAEADDRINUSE",
    (WSABASEERR+49),  "WSAEADDRNOTAVAIL",
    (WSABASEERR+50),  "WSAENETDOWN",
    (WSABASEERR+51),  "WSAENETUNREACH",
    (WSABASEERR+52),  "WSAENETRESET",
    (WSABASEERR+53),  "WSAECONNABORTED",
    (WSABASEERR+54),  "WSAECONNRESET",
    (WSABASEERR+55),  "WSAENOBUFS",
    (WSABASEERR+56),  "WSAEISCONN",
    (WSABASEERR+57),  "WSAENOTCONN",
    (WSABASEERR+58),  "WSAESHUTDOWN",
    (WSABASEERR+59),  "WSAETOOMANYREFS",
    (WSABASEERR+60),  "WSAETIMEDOUT",
    (WSABASEERR+61),  "WSAECONNREFUSED",
    (WSABASEERR+62),  "WSAELOOP",
    (WSABASEERR+63),  "WSAENAMETOOLONG",
    (WSABASEERR+64),  "WSAEHOSTDOWN",
    (WSABASEERR+65),  "WSAEHOSTUNREACH",
    (WSABASEERR+66),  "WSAENOTEMPTY",
    (WSABASEERR+67),  "WSAEPROCLIM",
    (WSABASEERR+68),  "WSAEUSERS",
    (WSABASEERR+69),  "WSAEDQUOT",
    (WSABASEERR+70),  "WSAESTALE",
    (WSABASEERR+71),  "WSAEREMOTE",
    (WSABASEERR+101), "WSAEDISCON",
    (WSABASEERR+91),  "WSASYSNOTREADY",
    (WSABASEERR+92),  "WSAVERNOTSUPPORTED",
    (WSABASEERR+93),  "WSANOTINITIALISED",
    NO_ERROR,         "NO_ERROR"
};


PCHAR
WsGetErrorString (
    IN INT Error
    )
{
    INT i;

    for ( i = 0; ErrorStrings[i].ErrorCode != NO_ERROR; i++ ) {
        if ( ErrorStrings[i].ErrorCode == Error ) {
            return ErrorStrings[i].ErrorString;
        }
    }

    return "Unknown";

} // WsGetErrorString


VOID
WsExitApiCall (
    IN PCHAR RoutineName,
    IN INT_PTR ReturnValue,
    IN BOOLEAN Failed

    )
{
    ULONG i;
    INT error = GetLastError( );

    if( SockProcessTerminating ||
        GET_THREAD_DATA() == NULL ) {
        SetLastError( error );
        return;
    }

    CHECK_HEAP;

    SockIndentLevel--;

    IF_DEBUG(EXIT) {
        for ( i = 0; i < SockIndentLevel; i++ ) {
            WS_PRINT(( "    " ));
        }
        if ( !Failed ) {
            WS_PRINT(( "<--- %s() returning %ld (0x%p)\n",
                           RoutineName, ReturnValue, ReturnValue ));
        } else {

            PSZ errorString = WsGetErrorString( error );

            WS_PRINT(( "<--- %s() FAILED--error %ld (0x%lx) == %s\n",
                           RoutineName, error, error, errorString ));
        }
    }

    SetLastError( error );

    return;

} // WsExitApiCall

LIST_ENTRY SockHeapListHead;
ULONG SockTotalAllocations = 0;
ULONG SockTotalFrees = 0;
ULONG SockTotalBytesAllocated = 0;
RTL_RESOURCE SocketHeapLock;
BOOLEAN SockHeapDebugInitialized = FALSE;
BOOLEAN SockDebugHeap = FALSE;

PVOID SockHeap = NULL;
PVOID SockCaller1;
PVOID SockCaller2;
BOOLEAN SockDoHeapCheck = TRUE;
BOOLEAN SockDoubleHeapCheck = FALSE;

#define WINSOCK_HEAP_CODE_1 0xabcdef00
#define WINSOCK_HEAP_CODE_2 0x12345678
#define WINSOCK_HEAP_CODE_3 0x87654321
#define WINSOCK_HEAP_CODE_4 0x00fedcba
#define WINSOCK_HEAP_CODE_5 0xa1b2c3d4

typedef struct _SOCK_HEAP_HEADER {
    ULONG HeapCode1;
    ULONG HeapCode2;
    LIST_ENTRY GlobalHeapListEntry;
    PCHAR FileName;
    ULONG LineNumber;
    ULONG Size;
    ULONG Pad;
} SOCK_HEAP_HEADER, *PSOCK_HEAP_HEADER;

typedef struct _SOCK_HEAP_TAIL {
    PSOCK_HEAP_HEADER Header;
    ULONG HeapCode3;
    ULONG HeapCode4;
    ULONG HeapCode5;
} SOCK_HEAP_TAIL, *PSOCK_HEAP_TAIL;

#define FREE_LIST_SIZE 64
SOCK_HEAP_HEADER SockRecentFreeList[FREE_LIST_SIZE];
ULONG SockRecentFreeListIndex = 0;


VOID
SockInitializeDebugData (
    VOID
    )
{
    RtlInitializeResource( &SocketHeapLock );
    InitializeListHead( &SockHeapListHead );

} // SockInitializeDebugData


PVOID
SockAllocateHeap (
    IN ULONG NumberOfBytes,
    PCHAR FileName,
    ULONG LineNumber
    )
{
    PSOCK_HEAP_HEADER header;
    SOCK_HEAP_TAIL UNALIGNED *tail;

    //WS_ASSERT( !SockProcessTerminating );
    WS_ASSERT( (NumberOfBytes & 0xF0000000) == 0 );
    WS_ASSERT( SockPrivateHeap != NULL );

    SockCheckHeap( );

    RtlAcquireResourceExclusive( &SocketHeapLock, TRUE );

    header = RtlAllocateHeap( SockPrivateHeap, 0,
                              NumberOfBytes + sizeof(*header) + sizeof(*tail) );
    if ( header == NULL ) {
        RtlReleaseResource( &SocketHeapLock );

        if( SockDoubleHeapCheck ) {
            SockCheckHeap();
        }

        return NULL;
    }

    header->HeapCode1 = WINSOCK_HEAP_CODE_1;
    header->HeapCode2 = WINSOCK_HEAP_CODE_2;
    header->FileName = FileName;
    header->LineNumber = LineNumber;
    header->Size = NumberOfBytes;

    tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + NumberOfBytes );
    tail->Header = header;
    tail->HeapCode3 = WINSOCK_HEAP_CODE_3;
    tail->HeapCode4 = WINSOCK_HEAP_CODE_4;
    tail->HeapCode5 = WINSOCK_HEAP_CODE_5;

    InsertTailList( &SockHeapListHead, &header->GlobalHeapListEntry );
    SockTotalAllocations++;
    SockTotalBytesAllocated += header->Size;

    RtlReleaseResource( &SocketHeapLock );

    if( SockDoubleHeapCheck ) {
        SockCheckHeap();
    }

    return (PVOID)(header + 1);

} // SockAllocateHeap


VOID
SockFreeHeap (
    IN PVOID Pointer
    )
{
    PSOCK_HEAP_HEADER header = (PSOCK_HEAP_HEADER)Pointer - 1;
    SOCK_HEAP_TAIL UNALIGNED * tail;

    //WS_ASSERT( !SockProcessTerminating );
    WS_ASSERT( SockPrivateHeap != NULL );

    SockCheckHeap( );

    tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + header->Size );

    if ( !SockHeapDebugInitialized ) {
        SockInitializeDebugData( );
        SockHeapDebugInitialized = TRUE;
    }

    RtlAcquireResourceExclusive( &SocketHeapLock, TRUE );

    WS_ASSERT( header->HeapCode1 == WINSOCK_HEAP_CODE_1 );
    WS_ASSERT( header->HeapCode2 == WINSOCK_HEAP_CODE_2 );
    WS_ASSERT( tail->HeapCode3 == WINSOCK_HEAP_CODE_3 );
    WS_ASSERT( tail->HeapCode4 == WINSOCK_HEAP_CODE_4 );
    WS_ASSERT( tail->HeapCode5 == WINSOCK_HEAP_CODE_5 );
    WS_ASSERT( tail->Header == header );

    RemoveEntryList( &header->GlobalHeapListEntry );
    SockTotalFrees++;
    SockTotalBytesAllocated -= header->Size;

    //RtlMoveMemory( &SockRecentFreeList[SockRecentFreeListIndex], header, sizeof(*header ) );
    //SockRecentFreeListIndex++;
    //if ( SockRecentFreeListIndex >= FREE_LIST_SIZE ) {
    //    SockRecentFreeListIndex = 0;
    //}

    RtlZeroMemory( header, sizeof(*header) );

    header->HeapCode1 = (ULONG)~WINSOCK_HEAP_CODE_1;
    header->HeapCode2 = (ULONG)~WINSOCK_HEAP_CODE_2;
    tail->HeapCode3 = (ULONG)~WINSOCK_HEAP_CODE_3;
    tail->HeapCode4 = (ULONG)~WINSOCK_HEAP_CODE_4;
    tail->HeapCode5 = (ULONG)~WINSOCK_HEAP_CODE_5;
    tail->Header = NULL;

    RtlReleaseResource( &SocketHeapLock );

    RtlFreeHeap( SockPrivateHeap, 0, (PVOID)header );

    if( SockDoubleHeapCheck ) {
        SockCheckHeap();
    }

} // SockFreeHeap


VOID
SockCheckHeap (
    VOID
    )
{
    PLIST_ENTRY listEntry;
    PLIST_ENTRY lastListEntry = NULL;
    PSOCK_HEAP_HEADER header;
    SOCK_HEAP_TAIL UNALIGNED *tail;

    if ( !SockHeapDebugInitialized ) {
        SockInitializeDebugData( );
        SockHeapDebugInitialized = TRUE;
        //SockHeap = RtlCreateHeap( HEAP_GROWABLE, 0, 0, 0, 0, NULL );
        //WS_ASSERT( SockHeap != NULL );
    }

    if ( !SockDoHeapCheck ) {
        return;
    }

    RtlValidateHeap( SockPrivateHeap, 0, NULL );

    RtlAcquireResourceExclusive( &SocketHeapLock, TRUE );

    for ( listEntry = SockHeapListHead.Flink;
          listEntry != &SockHeapListHead;
          listEntry = listEntry->Flink ) {

        if ( listEntry == NULL ) {
            DbgPrint( "listEntry == NULL, lastListEntry == %lx\n", lastListEntry );
            DbgBreakPoint( );
        }

        header = CONTAINING_RECORD( listEntry, SOCK_HEAP_HEADER, GlobalHeapListEntry );
        tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + header->Size );

        if ( header->HeapCode1 != WINSOCK_HEAP_CODE_1 ) {
            DbgPrint( "SockCheckHeap, fail 1, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( header->HeapCode2 != WINSOCK_HEAP_CODE_2 ) {
            DbgPrint( "SockCheckHeap, fail 2, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( tail->HeapCode3 != WINSOCK_HEAP_CODE_3 ) {
            DbgPrint( "SockCheckHeap, fail 3, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( tail->HeapCode4 != WINSOCK_HEAP_CODE_4 ) {
            DbgPrint( "SockCheckHeap, fail 4, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( tail->HeapCode5 != WINSOCK_HEAP_CODE_5 ) {
            DbgPrint( "SockCheckHeap, fail 5, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        if ( tail->Header != header ) {
            DbgPrint( "SockCheckHeap, fail 6, header %lx tail %lx\n", header, tail );
            DbgBreakPoint( );
        }

        lastListEntry = listEntry;
    }

    RtlGetCallersAddress( &SockCaller1, &SockCaller2 );

    RtlReleaseResource( &SocketHeapLock );

} // SockCheckHeap

PVOID GlobalLockCaller;
PVOID GlobalLockCallersCaller;
BOOLEAN GlobalLockHeld = FALSE;


VOID
SockAcquireGlobalLockExclusive (
    VOID
    )
{

    WS_ASSERT( !SockProcessTerminating );

    EnterCriticalSection( pSocketLock );
    RtlGetCallersAddress( &GlobalLockCaller, &GlobalLockCallersCaller );
    GlobalLockHeld = TRUE;

} // SockAcquireGlobalLockExclusive

VOID
SockReleaseGlobalLock (
    VOID
    )
{

    WS_ASSERT( !SockProcessTerminating );

    GlobalLockHeld = TRUE;
    LeaveCriticalSection( pSocketLock );

} // SockReleaseGlobalLock

#endif // if DBG

BOOL
SockThreadInitialize(
    VOID
    )
{

    PWINSOCK_TLS_DATA data;
    NTSTATUS status;

    IF_DEBUG(INIT) {
        WS_PRINT(( "SockThreadInitialize: TEB = %lx\n",
                       NtCurrentTeb( ) ));
    }

    //
    // Allocate space for per-thread data the DLL will have.
    //

    data = ALLOCATE_HEAP( sizeof(*data) );
    if ( data == NULL ) {
        WS_PRINT(( "SockThreadInitialize: unable to allocate thread data.\n" ));
        return FALSE;
    }

    //
    // Store a pointer to this data area in TLS.
    //

    if( !SET_THREAD_DATA(data) ) {

        WS_PRINT(( "SockThreadInitialize: TlsSetValue failed: %ld\n", GetLastError( ) ));
#if !defined(USE_TEB_FIELD)
        SockTlsSlot = 0xFFFFFFFF;
#endif  // !USE_TEB_FIELD
        return FALSE;
    }

    //
    // Initialize the thread data.
    //

    RtlZeroMemory( data, sizeof(*data) );

    data->R_INIT__res.retrans = RES_TIMEOUT;
    data->R_INIT__res.retry = 4;
    data->R_INIT__res.options = RES_DEFAULT;

#if DBG
    SockIndentLevel = 0;
#endif

    return TRUE;

}   // SockThreadInitialize

