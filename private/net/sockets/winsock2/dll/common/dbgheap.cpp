/*++

    Copyright (c) 1996 Microsoft Corporation

Module Name:

    dbgheap.cpp

Abstract:

    This module contains special debug versions of the new and delete
    C++ operators.

    The following functions are exported by this module:

        operator new
        operator delete

Author:

    Keith Moore keithmo@microsoft.com 10-May-1996

Revision History:

--*/

#if DBG

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

}	// extern "C"

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
BOOLEAN SockDoubleHeapCheck = TRUE;

#define WINSOCK_HEAP_CODE_1 0xabcdef00
#define WINSOCK_HEAP_CODE_2 0x12345678
#define WINSOCK_HEAP_CODE_3 0x87654321
#define WINSOCK_HEAP_CODE_4 0x00fedcba
#define WINSOCK_HEAP_CODE_5 0xa1b2c3d4

#define MAX_STACK_BACKTRACE 3

//
// N.B. This header MUST be quadword aligned!
//

typedef struct _SOCK_HEAP_HEADER {
    ULONG HeapCode1;
    ULONG HeapCode2;
    LIST_ENTRY GlobalHeapListEntry;
    ULONG Size;
    PVOID Stack[MAX_STACK_BACKTRACE];
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
SockInitializeDebugData(
    VOID
    )
{
    RtlInitializeResource( &SocketHeapLock );
    InitializeListHead( &SockHeapListHead );

} // SockInitializeDebugData


VOID
SockCheckHeap(
    VOID
    )
{
    PLIST_ENTRY listEntry;
    PLIST_ENTRY lastListEntry = NULL;
    PSOCK_HEAP_HEADER header;
    SOCK_HEAP_TAIL UNALIGNED *tail;

    if( !SockHeapDebugInitialized ) {
        SockInitializeDebugData();
        SockHeapDebugInitialized = TRUE;
    }

    if( !SockDoHeapCheck ) {
        return;
    }

    RtlValidateHeap(
        RtlProcessHeap(),
        0,
        NULL
        );

    RtlAcquireResourceExclusive(
        &SocketHeapLock,
        TRUE
        );

    for( listEntry = SockHeapListHead.Flink;
         listEntry != &SockHeapListHead;
         listEntry = listEntry->Flink ) {

        if( listEntry == NULL ) {
            DbgPrint( "listEntry == NULL, lastListEntry == %lx\n", lastListEntry );
            DbgBreakPoint();
        }

        header = CONTAINING_RECORD( listEntry, SOCK_HEAP_HEADER, GlobalHeapListEntry );
        tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + header->Size );

        if( header->HeapCode1 != WINSOCK_HEAP_CODE_1 ) {
            DbgPrint( "SockCheckHeap, fail 1, header %lx tail %lx\n", header, tail );
            DbgBreakPoint();
        }

        if( header->HeapCode2 != WINSOCK_HEAP_CODE_2 ) {
            DbgPrint( "SockCheckHeap, fail 2, header %lx tail %lx\n", header, tail );
            DbgBreakPoint();
        }

        if( tail->HeapCode3 != WINSOCK_HEAP_CODE_3 ) {
            DbgPrint( "SockCheckHeap, fail 3, header %lx tail %lx\n", header, tail );
            DbgBreakPoint();
        }

        if( tail->HeapCode4 != WINSOCK_HEAP_CODE_4 ) {
            DbgPrint( "SockCheckHeap, fail 4, header %lx tail %lx\n", header, tail );
            DbgBreakPoint();
        }

        if( tail->HeapCode5 != WINSOCK_HEAP_CODE_5 ) {
            DbgPrint( "SockCheckHeap, fail 5, header %lx tail %lx\n", header, tail );
            DbgBreakPoint();
        }

        if( tail->Header != header ) {
            DbgPrint( "SockCheckHeap, fail 6, header %lx tail %lx\n", header, tail );
            DbgBreakPoint();
        }

        lastListEntry = listEntry;
    }

    RtlGetCallersAddress(
        &SockCaller1,
        &SockCaller2
        );

    RtlReleaseResource( &SocketHeapLock );

}   // SockCheckHeap


PVOID
SockAllocateHeap(
    IN ULONG NumberOfBytes
    )
{
    PSOCK_HEAP_HEADER header;
    SOCK_HEAP_TAIL UNALIGNED *tail;

    SockCheckHeap();

    ASSERT( (NumberOfBytes & 0xF0000000) == 0 );

    RtlAcquireResourceExclusive(
        &SocketHeapLock,
        TRUE
        );

    header = (PSOCK_HEAP_HEADER)RtlAllocateHeap(
                 RtlProcessHeap(),
                 0,
                 NumberOfBytes + sizeof(*header) + sizeof(*tail)
                 );

    if( header == NULL ) {
        RtlReleaseResource( &SocketHeapLock );

        if( SockDoubleHeapCheck ) {
            SockCheckHeap();
        }

        return NULL;
    }

    header->HeapCode1 = WINSOCK_HEAP_CODE_1;
    header->HeapCode2 = WINSOCK_HEAP_CODE_2;
    header->Size = NumberOfBytes;

    tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + NumberOfBytes );
    tail->Header = header;
    tail->HeapCode3 = WINSOCK_HEAP_CODE_3;
    tail->HeapCode4 = WINSOCK_HEAP_CODE_4;
    tail->HeapCode5 = WINSOCK_HEAP_CODE_5;

    InsertTailList( &SockHeapListHead, &header->GlobalHeapListEntry );
    SockTotalAllocations++;
    SockTotalBytesAllocated += header->Size;

#if i386
    {
        ULONG hash;

        RtlCaptureStackBackTrace(
            2,
            MAX_STACK_BACKTRACE,
            (PVOID *)&header->Stack,
            &hash
            );
    }
#endif  // i386

    RtlReleaseResource( &SocketHeapLock );

    if( SockDoubleHeapCheck ) {
        SockCheckHeap();
    }

    return (PVOID)(header + 1);

}   // SockAllocateHeap


VOID
SockFreeHeap(
    IN PVOID Pointer
    )
{
    PSOCK_HEAP_HEADER header = (PSOCK_HEAP_HEADER)Pointer - 1;
    SOCK_HEAP_TAIL UNALIGNED * tail;

    SockCheckHeap();

    tail = (SOCK_HEAP_TAIL UNALIGNED *)( (PCHAR)(header + 1) + header->Size );

    RtlAcquireResourceExclusive(
        &SocketHeapLock,
        TRUE
        );

    ASSERT( header->HeapCode1 == WINSOCK_HEAP_CODE_1 );
    ASSERT( header->HeapCode2 == WINSOCK_HEAP_CODE_2 );
    ASSERT( tail->HeapCode3 == WINSOCK_HEAP_CODE_3 );
    ASSERT( tail->HeapCode4 == WINSOCK_HEAP_CODE_4 );
    ASSERT( tail->HeapCode5 == WINSOCK_HEAP_CODE_5 );
    ASSERT( tail->Header == header );

    RemoveEntryList( &header->GlobalHeapListEntry );
    SockTotalFrees++;
    SockTotalBytesAllocated -= header->Size;

    header->HeapCode1 = (ULONG)~WINSOCK_HEAP_CODE_1;
    header->HeapCode2 = (ULONG)~WINSOCK_HEAP_CODE_2;
    tail->HeapCode3 = (ULONG)~WINSOCK_HEAP_CODE_3;
    tail->HeapCode4 = (ULONG)~WINSOCK_HEAP_CODE_4;
    tail->HeapCode5 = (ULONG)~WINSOCK_HEAP_CODE_5;
    tail->Header = NULL;

    RtlReleaseResource( &SocketHeapLock );

    RtlFreeHeap(
        RtlProcessHeap(),
        0,
        (PVOID)header
        );

    if( SockDoubleHeapCheck ) {
        SockCheckHeap();
    }

} // SockFreeHeap


void *
__cdecl
operator new(
    size_t size
    )
{
    return (void *)SockAllocateHeap( (ULONG)size );

}   // operator new


void
__cdecl
operator delete(
    void *ptr
    )
{
    SockFreeHeap( (PVOID)ptr );

}   // operator delete

#endif  // DBG

