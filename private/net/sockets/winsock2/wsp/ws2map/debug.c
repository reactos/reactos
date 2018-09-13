/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains debug-specific functions used by the Winsock 2
    to Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockInitializeDebugData()
        SockEnterApiCall()
        SockExitApiCall()
        SockPrintf()
        SockAssert()
        SockAllocateHeap()
        SockFreeHeap()
        SockCheckHeap()
        SockExceptionFilter()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


#if DBG


//
// The (PCHAR) casts in the following macro force the compiler to assume
// only BYTE alignment.
//

#define SockCopyMemory(d,s,l) CopyMemory( (PCHAR)(d), (PCHAR)(s), (l) )

struct _SOCK_ERROR_STRINGS {
    INT ErrorCode;
    LPSTR ErrorString;
} SockErrorStrings[] = {
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
    (WSABASEERR+91),  "WSASYSNOTREADY",
    (WSABASEERR+92),  "WSAVERNOTSUPPORTED",
    (WSABASEERR+93),  "WSANOTINITIALISED",
    (WSABASEERR+101), "WSAEDISCON",
    (WSABASEERR+102), "WSAENOMORE",
    (WSABASEERR+103), "WSAECANCELLED",
    (WSABASEERR+104), "WSAEINVALIDPROCTABLE",
    (WSABASEERR+105), "WSAEINVALIDPROVIDER",
    (WSABASEERR+106), "WSAEPROVIDERFAILEDINIT",
    (WSABASEERR+107), "WSAESYSCALLFAILURE",
    (WSABASEERR+108), "WSAESERVICE_NOT_FOUND",
    (WSABASEERR+109), "WSAETYPE_NOT_FOUND",
    (WSABASEERR+110), "WSA_E_NO_MORE",
    (WSABASEERR+111), "WSA_E_CANCELLED",
    (WSABASEERR+112), "WSAEREFUSED",
    NO_ERROR,         "NO_ERROR"
};

HANDLE SockDebugFileHandle = INVALID_HANDLE_VALUE;
PCHAR SockDebugFileName = "ws2map.log";

LIST_ENTRY SockHeapListHead;
ULONG SockTotalAllocations = 0;
ULONG SockTotalFrees = 0;
ULONG SockTotalBytesAllocated = 0;
CRITICAL_SECTION SocketHeapLock;
BOOL SockHeapDebugInitialized = FALSE;
BOOL SockDebugHeap = FALSE;

PVOID SockHeap = NULL;
BOOL SockDoHeapCheck = TRUE;
BOOL SockDoubleHeapCheck = FALSE;

#define WINSOCK_HEAP_CODE_1 0x00010203
#define WINSOCK_HEAP_CODE_2 0x44556677
#define WINSOCK_HEAP_CODE_3 0x08090A0B
#define WINSOCK_HEAP_CODE_4 0xCCDDEEFF
#define WINSOCK_HEAP_CODE_5 0x18293A4B

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


//
// Private prototypes.
//

LPSTR
SockGetErrorString(
    IN INT Error
    );


//
// Public functions.
//


VOID
SockInitializeDebugData(
    VOID
    )

/*++

Routine Description:

    Initializes global debug-specific data.

Arguments:

    None.

Return Value:

    None.

--*/

{

    SockHeap = GetProcessHeap();
    InitializeCriticalSection( &SocketHeapLock );
    InitializeListHead( &SockHeapListHead );

} // SockInitializeDebugData



VOID
SockEnterApiCall(
    IN PCHAR RoutineName,
    IN PVOID Arg1,
    IN PVOID Arg2,
    IN PVOID Arg3,
    IN PVOID Arg4
    )

/*++

Routine Description:

    Displays the name of an SPI entrypoint and the first four arguments.
    This routine is called at the beginning of very SPI entrypoint.

Arguments:

    RoutineName - The name of the SPI entrypoint.

    Arg1 - Argument #1.

    Arg2 - Argument #2.

    Arg3 - Argument #3.

Return Value:

    None.

--*/

{

    SOCK_CHECK_HEAP();

    IF_DEBUG(ENTER) {

        SOCK_PRINT((
            "---> %s() args 0x%lx 0x%lx 0x%lx 0x%lx\n",
            RoutineName,
            Arg1,
            Arg2,
            Arg3,
            Arg4
            ));

    }

}   // SockEnterApiCall



VOID
SockExitApiCall(
    IN PCHAR RoutineName,
    IN LONG_PTR ReturnCode,
    IN LPINT ErrorCode
    )

/*++

Routine Description:

    Displays the SPI entrypoint name and the completion status. This routine
    is called just before every SPI entrypoint returns.

Arguments:

    RoutineName - The name of the SPI entrypoint.

    ReturnCode - The return code for the entrypoint.

    ErrorCode - Points to an error code if this API is failing, NULL
        if the API is successful.

Return Value:

    None.

--*/

{

    if( SockProcessTerminating ) {

        return;

    }

    SOCK_CHECK_HEAP();

    IF_DEBUG(EXIT) {

        if( ErrorCode != NULL ) {

            LPSTR errorString;

            errorString = SockGetErrorString( *ErrorCode );

            SOCK_PRINT((
                "<--- %s() FAILED--error %ld (0x%lx) == %s\n",
                RoutineName,
                *ErrorCode,
                *ErrorCode,
                errorString
                ));

        } else {

            SOCK_PRINT((
                "<--- %s() returning %ld (0x%lx)\n",
                RoutineName,
                ReturnCode,
                ReturnCode
                ));

        }

    }

}   // SockExitApiCall



VOID
SockPrintf(
    char *Format,
    ...
    )

/*++

Routine Description:

    Printf-like routine for dumping debug data.

Arguments:

    Format - Printf-like format string.

    ... - Any additional arguments required by the format string.

Return Value:

    None.

--*/

{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;
    BOOL ret;

    length = (ULONG)wsprintfA( OutputBuffer, "WS2MAP: " );

    va_start( arglist, Format );

    wvsprintfA( OutputBuffer + length, Format, arglist );

    va_end( arglist );

    IF_DEBUG(DEBUGGER) {

        OutputDebugString( OutputBuffer );

    }

    IF_DEBUG(FILE) {

        if ( SockDebugFileHandle == INVALID_HANDLE_VALUE ) {
            SockDebugFileHandle = CreateFile(
                                  SockDebugFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  CREATE_ALWAYS,
                                  0,
                                  NULL
                                  );
        }

        if ( SockDebugFileHandle == INVALID_HANDLE_VALUE ) {

            OutputDebugString(
                "SockPrintf: Failed to open winsock debug log file\n"
                );

        } else {

            length = strlen( OutputBuffer );

            ret = WriteFile(
                      SockDebugFileHandle,
                      (LPVOID )OutputBuffer,
                      length,
                      &length,
                      NULL
                      );

            if ( !ret ) {

                OutputDebugString(
                    "SockPrintf: file WriteFile failed\n"
                    );

            }

        }

    }

}   // SockPrintf



VOID
SockAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    )

/*++

Routine Description:

    Assertion routine invoked when the parameter to SOCK_ASSERT or
    SOCK_REQUIRE evaluates to FALSE.

Arguments:

    FailedAssertion - The text of the failed assertion. This is the
        "stringized" version of the SOCK_ASSERT/SOCK_REQUIRE parameter.

    FileName - The name of the file containing the failed assertion.

    LineNumber - The line number of the failed assertion.

Return Value:

    None.

--*/

{

    CHAR buffer[MAX_PATH * 2];

    wsprintf(
        buffer,
        "\n*** Assertion failed: %s\n***   Source File: %s, line %ld\n\n",
        FailedAssertion,
        FileName,
        LineNumber
        );

    OutputDebugString( buffer );
    DebugBreak();

}   // SockAssert



PVOID
SockAllocateHeap(
    IN ULONG NumberOfBytes,
    PCHAR FileName,
    ULONG LineNumber
    )

/*++

Routine Description:

    A debug heap allocator. Keeps various statistics, and maintains all
    heap blocks on a global list.

Arguments:

    NumberOfBytes - Number of bytes for the allocated block.

    FileName - The name of the file allocating the block.

    LineNumber - The line number of the routine allocating the block.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, NULL
        otherwise.

--*/

{

    PSOCK_HEAP_HEADER header;
    SOCK_HEAP_TAIL UNALIGNED *tail;
    SOCK_HEAP_TAIL localTail;

    SockCheckHeap();

    SOCK_ASSERT( (NumberOfBytes & 0xF0000000) == 0 );
    SOCK_ASSERT( SockHeap != NULL );

    EnterCriticalSection( &SocketHeapLock );

    header = HeapAlloc(
                 SockHeap,
                 0,
                 NumberOfBytes + sizeof(*header) + sizeof(*tail)
                 );

    if ( header == NULL ) {

        LeaveCriticalSection( &SocketHeapLock );

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

    localTail.Header = header;
    localTail.HeapCode3 = WINSOCK_HEAP_CODE_3;
    localTail.HeapCode4 = WINSOCK_HEAP_CODE_4;
    localTail.HeapCode5 = WINSOCK_HEAP_CODE_5;

    SockCopyMemory(
        tail,
        &localTail,
        sizeof(localTail)
        );

    InsertTailList( &SockHeapListHead, &header->GlobalHeapListEntry );
    SockTotalAllocations++;
    SockTotalBytesAllocated += header->Size;

    LeaveCriticalSection( &SocketHeapLock );

    if( SockDoubleHeapCheck ) {
        SockCheckHeap();
    }

    return (PVOID)(header + 1);

} // SockAllocateHeap



VOID
SockFreeHeap(
    IN PVOID Pointer
    )

/*++

Routine Description:

    Frees a heap block allocated with SockAllocateHeap().

Arguments:

    Pointer - A pointer to an allocated heap block.

Return Value:

    None.

--*/

{

    PSOCK_HEAP_HEADER header = (PSOCK_HEAP_HEADER)Pointer - 1;
    SOCK_HEAP_TAIL UNALIGNED * tail;
    SOCK_HEAP_TAIL localTail;

    SockCheckHeap();

    SOCK_ASSERT( SockHeap != NULL );

    tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + header->Size );

    if ( !SockHeapDebugInitialized ) {
        SockInitializeDebugData();
        SockHeapDebugInitialized = TRUE;
    }

    EnterCriticalSection( &SocketHeapLock );

    SockCopyMemory(
        &localTail,
        tail,
        sizeof(localTail)
        );

    SOCK_ASSERT( header->HeapCode1 == WINSOCK_HEAP_CODE_1 );
    SOCK_ASSERT( header->HeapCode2 == WINSOCK_HEAP_CODE_2 );
    SOCK_ASSERT( localTail.HeapCode3 == WINSOCK_HEAP_CODE_3 );
    SOCK_ASSERT( localTail.HeapCode4 == WINSOCK_HEAP_CODE_4 );
    SOCK_ASSERT( localTail.HeapCode5 == WINSOCK_HEAP_CODE_5 );
    SOCK_ASSERT( localTail.Header == header );

    RemoveEntryList( &header->GlobalHeapListEntry );
    SockTotalFrees++;
    SockTotalBytesAllocated -= header->Size;

    ZeroMemory( header, sizeof(*header) );

    header->HeapCode1 = (ULONG)~WINSOCK_HEAP_CODE_1;
    header->HeapCode2 = (ULONG)~WINSOCK_HEAP_CODE_2;
    localTail.HeapCode3 = (ULONG)~WINSOCK_HEAP_CODE_3;
    localTail.HeapCode4 = (ULONG)~WINSOCK_HEAP_CODE_4;
    localTail.HeapCode5 = (ULONG)~WINSOCK_HEAP_CODE_5;
    localTail.Header = NULL;

    SockCopyMemory(
        tail,
        &localTail,
        sizeof(localTail)
        );

    LeaveCriticalSection( &SocketHeapLock );

    HeapFree(
        SockHeap,
        0,
        (LPVOID)header
        );

    if( SockDoubleHeapCheck ) {
        SockCheckHeap();
    }

} // SockFreeHeap



VOID
SockCheckHeap(
    VOID
    )

/*++

Routine Description:

    Performs sanity checking on the heap.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PLIST_ENTRY lastListEntry = NULL;
    PSOCK_HEAP_HEADER header;
    SOCK_HEAP_TAIL UNALIGNED *tail;
    SOCK_HEAP_TAIL localTail;

    if ( !SockHeapDebugInitialized ) {
        SockInitializeDebugData();
        SockHeapDebugInitialized = TRUE;
    }

    if ( !SockDoHeapCheck ) {
        return;
    }

    SOCK_ASSERT( HeapValidate( SockHeap, 0, NULL ) );

    EnterCriticalSection( &SocketHeapLock );

    for ( listEntry = SockHeapListHead.Flink;
          listEntry != &SockHeapListHead;
          listEntry = listEntry->Flink ) {

        if ( listEntry == NULL ) {

            SockPrintf(
                "listEntry == NULL, lastListEntry == %lx\n",
                lastListEntry
                );

            DebugBreak();

        }

        header = CONTAINING_RECORD( listEntry, SOCK_HEAP_HEADER, GlobalHeapListEntry );
        tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + header->Size );

        SockCopyMemory(
            &localTail,
            tail,
            sizeof(localTail)
            );

        if ( header->HeapCode1 != WINSOCK_HEAP_CODE_1 ) {

            SockPrintf(
                "SockCheckHeap, fail 1, header %lx tail %lx\n",
                header,
                tail
                );

            DebugBreak();

        }

        if ( header->HeapCode2 != WINSOCK_HEAP_CODE_2 ) {

            SockPrintf(
                "SockCheckHeap, fail 2, header %lx tail %lx\n",
                header,
                tail
                );

            DebugBreak();

        }

        if ( localTail.HeapCode3 != WINSOCK_HEAP_CODE_3 ) {

            SockPrintf(
                "SockCheckHeap, fail 3, header %lx tail %lx\n",
                header,
                tail
                );

            DebugBreak();

        }

        if ( localTail.HeapCode4 != WINSOCK_HEAP_CODE_4 ) {

            SockPrintf(
                "SockCheckHeap, fail 4, header %lx tail %lx\n",
                header,
                tail
                );

            DebugBreak();

        }

        if ( localTail.HeapCode5 != WINSOCK_HEAP_CODE_5 ) {

            SockPrintf(
                "SockCheckHeap, fail 5, header %lx tail %lx\n",
                header,
                tail
                );

            DebugBreak();

        }

        if ( localTail.Header != header ) {

            SockPrintf(
                "SockCheckHeap, fail 6, header %lx tail %lx\n",
                header,
                tail
                );

            DebugBreak();

        }

        lastListEntry = listEntry;

    }

    LeaveCriticalSection( &SocketHeapLock );

} // SockCheckHeap



LONG
SockExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR SourceFile,
    LONG LineNumber
    )

/*++

Routine Description:

    An exception filter. This routine is invoked within the except()
    clause of a try/except block, giving it the opportunity to print
    useful debugging information about the exception.

Arguments:

    ExceptionPointers - Pointer to an EXCEPTION_POINTERS structure
        containing information about the exception.

    SourceFile - The name of the file that caught the exception.

    LineNumber - The line number where the exception was caught.

Return Value:

    LONG - One of the EXCEPTION_* values indicating the manner in
        which the exception should be handled. This routine always
        returns EXCEPTION_EXECUTE_HANDLER.

--*/

{

    LPSTR fileName;

    //
    // Protect ourselves in case the process is totally screwed.
    //

    try {

        //
        // Exceptions should never be thrown in a properly functioning
        // system, so this is bad. To ensure that someone will see this,
        // forcibly enable debugger output.
        //

        SockDebugFlags |= SOCK_DEBUG_DEBUGGER;

        //
        // Strip off the path from the source file.
        //

        fileName = strrchr( SourceFile, '\\' );

        if( fileName == NULL ) {
            fileName = SourceFile;
        } else {
            fileName++;
        }

        //
        // Whine about the exception.
        //

        SOCK_PRINT((
            "SockExceptionFilter: exception %08lx @ %08lx, caught in %s:%d\n",
            ExceptionPointers->ExceptionRecord->ExceptionCode,
            ExceptionPointers->ExceptionRecord->ExceptionAddress,
            fileName,
            LineNumber
            ));

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Not much we can do here...
        //

    }

    return EXCEPTION_EXECUTE_HANDLER;

}   // SockExceptionFilter


//
// Private functions.
//


LPSTR
SockGetErrorString(
    IN INT Error
    )

/*++

Routine Description:

    Maps a socket error to a displayable string.

Arguments:

    Error - The error code to map.

Return Value:

    LPSTR - The displayable string.

--*/

{
    INT i;

    for( i = 0; SockErrorStrings[i].ErrorCode != NO_ERROR; i++ ) {

        if( SockErrorStrings[i].ErrorCode == Error ) {

            return SockErrorStrings[i].ErrorString;

        }

    }

    return "Unknown";

}   // SockGetErrorString


#endif  // DBG

