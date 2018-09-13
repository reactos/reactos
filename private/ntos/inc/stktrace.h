/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    stktrace.h

Abstract:

    This header file defines the format of the stack trace data base
    used to track caller backtraces.  This is a header file so debugger
    extensions can lookup entries in the database remotely.

Author:

    Steve Wood (stevewo) 13-Sep-1992

Revision History:

--*/


typedef struct _RTL_STACK_TRACE_ENTRY {
    struct _RTL_STACK_TRACE_ENTRY *HashChain;
    ULONG TraceCount;
    USHORT Index;
    USHORT Depth;
    PVOID BackTrace[ MAX_STACK_DEPTH ];
} RTL_STACK_TRACE_ENTRY, *PRTL_STACK_TRACE_ENTRY;

typedef struct _STACK_TRACE_DATABASE {
    union {
        RTL_CRITICAL_SECTION CriticalSection;
        ERESOURCE Resource;
    } Lock;

    PRTL_ACQUIRE_LOCK_ROUTINE AcquireLockRoutine;
    PRTL_RELEASE_LOCK_ROUTINE ReleaseLockRoutine;
    PRTL_OKAY_TO_LOCK_ROUTINE OkayToLockRoutine;

    BOOLEAN PreCommitted;
    BOOLEAN DumpInProgress;
    PVOID CommitBase;
    PVOID CurrentLowerCommitLimit;
    PVOID CurrentUpperCommitLimit;
    PCHAR NextFreeLowerMemory;
    PCHAR NextFreeUpperMemory;
    ULONG NumberOfEntriesLookedUp;
    ULONG NumberOfEntriesAdded;
    PRTL_STACK_TRACE_ENTRY *EntryIndexArray;    // Indexed by [-1 .. -NumberOfEntriesAdded]
    ULONG NumberOfBuckets;
    PRTL_STACK_TRACE_ENTRY Buckets[ 1 ];
} STACK_TRACE_DATABASE, *PSTACK_TRACE_DATABASE;

PSTACK_TRACE_DATABASE
RtlpAcquireStackTraceDataBase( VOID );

VOID
RtlpReleaseStackTraceDataBase( VOID );
