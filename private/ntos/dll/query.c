/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains the RtlQueryProcessInformation function

Author:

    Steve Wood (stevewo) 01-Apr-1994

Revision History:

--*/

#include <ntos.h>
#include "ldrp.h"
#include <stktrace.h>
#include <heap.h>
#include <stdio.h>

#define AdjustPointer( t, p, d ) (p); if ((p) != NULL) (p) = (t)((ULONG_PTR)(p) + (d))

NTSYSAPI
NTSTATUS
NTAPI
RtlpQueryProcessDebugInformationRemote(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    ULONG i;
    ULONG_PTR Delta;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_HEAP_INFORMATION HeapInfo;

    if (Buffer->EventPairTarget != NULL) {
        Status = NtWaitLowEventPair( Buffer->EventPairTarget );
        }
    else {
        Status = STATUS_SUCCESS;
        }

    while (NT_SUCCESS( Status )) {
        Status = RtlQueryProcessDebugInformation( NtCurrentTeb()->ClientId.UniqueProcess,
                                                  Buffer->Flags,
                                                  Buffer
                                                );
        if (NT_SUCCESS( Status )) {
            if (Delta = Buffer->ViewBaseDelta) {
                //
                // Need to relocate buffer pointers back to client addresses
                //
                AdjustPointer( PRTL_PROCESS_MODULES, Buffer->Modules, Delta );
                AdjustPointer( PRTL_PROCESS_BACKTRACES, Buffer->BackTraces, Delta );
                Heaps = AdjustPointer( PRTL_PROCESS_HEAPS, Buffer->Heaps, Delta );
                if (Heaps != NULL) {
                    for (i=0; i<Heaps->NumberOfHeaps; i++) {
                        HeapInfo = &Heaps->Heaps[ i ];
                        AdjustPointer( PRTL_HEAP_TAG, HeapInfo->Tags, Delta );
                        AdjustPointer( PRTL_HEAP_ENTRY, HeapInfo->Entries, Delta );
                        }
                    }

                AdjustPointer( PRTL_PROCESS_LOCKS, Buffer->Locks, Delta );
                }
            }

        if (Buffer->EventPairTarget == NULL) {
            //
            // If no event pair handle, then exit loop and terminate
            //
            break;
            }

        Status = NtSetHighWaitLowEventPair( Buffer->EventPairTarget );
        if (Buffer->EventPairTarget == NULL) {
            break;
            }
        }

    //
    // All done with buffer, remove from our address space
    // then terminate ourselves so client wakes up.
    //

    NtUnmapViewOfSection( NtCurrentProcess(), Buffer );
    NtTerminateThread( NtCurrentThread(), Status );
    return Status;
}


NTSTATUS
RtlpChangeQueryDebugBufferTarget(
    IN PRTL_DEBUG_INFORMATION Buffer,
    IN HANDLE TargetProcessId,
    OUT PHANDLE ReturnedTargetProcessHandle
    )
{
    NTSTATUS Status;
    CLIENT_ID OldTargetClientId, NewTargetClientId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE OldTargetProcess, NewTargetProcess, NewHandle;
    PHANDLE pOldHandle;
    ULONG DuplicateHandleFlags;

    if (Buffer->EventPairClient != NULL &&
        Buffer->TargetProcessId == TargetProcessId
       ) {
        return STATUS_SUCCESS;
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0,
                                NULL,
                                NULL
                              );

    DuplicateHandleFlags = DUPLICATE_CLOSE_SOURCE |
                           DUPLICATE_SAME_ACCESS |
                           DUPLICATE_SAME_ATTRIBUTES;

    if (Buffer->EventPairClient != NULL) {
        pOldHandle = &Buffer->EventPairTarget;
        }
    else {
        pOldHandle = NULL;
        }

    if (Buffer->TargetProcessId != NULL) {
        OldTargetClientId.UniqueProcess = Buffer->TargetProcessId;
        OldTargetClientId.UniqueThread = 0;
        Status = NtOpenProcess( &OldTargetProcess,
                                PROCESS_ALL_ACCESS,
                                &ObjectAttributes,
                                &OldTargetClientId
                              );
        if (!NT_SUCCESS( Status )) {
            return Status;
            }
        }
    else {
        OldTargetProcess = NtCurrentProcess();
        DuplicateHandleFlags &= ~DUPLICATE_CLOSE_SOURCE;
        if (pOldHandle != NULL) {
            pOldHandle = &Buffer->EventPairClient;
            }
        }

    if (ARGUMENT_PRESENT( TargetProcessId )) {
        NewTargetClientId.UniqueProcess = TargetProcessId;
        NewTargetClientId.UniqueThread = 0;
        Status = NtOpenProcess( &NewTargetProcess,
                                PROCESS_ALL_ACCESS,
                                &ObjectAttributes,
                                &NewTargetClientId
                              );
        if (!NT_SUCCESS( Status )) {
            if (OldTargetProcess != NtCurrentProcess()) {
                NtClose( OldTargetProcess );
                }
            return Status;
            }
        }
    else {
        NewTargetProcess = NULL;
        }

    NewHandle = NULL;
    if (pOldHandle != NULL) {
        Status = NtDuplicateObject( OldTargetProcess,
                                    *pOldHandle,
                                    NewTargetProcess,
                                    &NewHandle,
                                    0,
                                    0,
                                    DuplicateHandleFlags
                                  );
        if (!NT_SUCCESS( Status )) {
            if (OldTargetProcess != NtCurrentProcess()) {
                NtClose( OldTargetProcess );
                }
            if (NewTargetProcess != NULL) {
                NtClose( NewTargetProcess );
                }
            return Status;
            }
        }

    if (OldTargetProcess != NtCurrentProcess()) {
        NtUnmapViewOfSection( OldTargetProcess, Buffer->ViewBaseTarget );
        }
    else {
        Buffer->ViewBaseTarget = Buffer->ViewBaseClient;
        }

    if (NewTargetProcess != NULL) {
        Status = NtMapViewOfSection( Buffer->SectionHandleClient,
                                     NewTargetProcess,
                                     &Buffer->ViewBaseTarget,
                                     0,
                                     0,
                                     NULL,
                                     &Buffer->ViewSize,
                                     ViewUnmap,
                                     0,
                                     PAGE_READWRITE
                                   );
        if (Status == STATUS_CONFLICTING_ADDRESSES) {
            Buffer->ViewBaseTarget = NULL;
            Status = NtMapViewOfSection( Buffer->SectionHandleClient,
                                         NewTargetProcess,
                                         &Buffer->ViewBaseTarget,
                                         0,
                                         0,
                                         NULL,
                                         &Buffer->ViewSize,
                                         ViewUnmap,
                                         0,
                                         PAGE_READWRITE
                                       );
            }

        if (!NT_SUCCESS( Status )) {
            if (NewHandle != NULL) {
                NtDuplicateObject( NewTargetProcess,
                                   &NewHandle,
                                   NULL,
                                   NULL,
                                   0,
                                   0,
                                   DUPLICATE_CLOSE_SOURCE
                                 );
                }

            return Status;
            }

        if (ARGUMENT_PRESENT( ReturnedTargetProcessHandle )) {
            *ReturnedTargetProcessHandle = NewTargetProcess;
            }
        else {
            NtClose( NewTargetProcess );
            }
        }

    Buffer->EventPairTarget = NewHandle;
    Buffer->ViewBaseDelta = (ULONG_PTR)Buffer->ViewBaseClient - (ULONG_PTR)Buffer->ViewBaseTarget;
    return STATUS_SUCCESS;
}


PVOID
RtlpCommitQueryDebugInfo(
    IN PRTL_DEBUG_INFORMATION Buffer,
    IN ULONG Size
    )
{
    NTSTATUS Status;
    PVOID Result;
    PVOID CommitBase;
    SIZE_T CommitSize;
    SIZE_T NeededSize;

    Size = (Size + 3) & ~3;
    NeededSize = Buffer->OffsetFree + Size;
    if (NeededSize > Buffer->CommitSize) {
        if (NeededSize >= Buffer->ViewSize) {
            return NULL;
            }

        CommitBase = (PCHAR)Buffer + Buffer->CommitSize;
        CommitSize =  NeededSize - Buffer->CommitSize;
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &CommitBase,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            return NULL;
            }


        Buffer->CommitSize += CommitSize;
        }

    Result = (PCHAR)Buffer + Buffer->OffsetFree;
    Buffer->OffsetFree = NeededSize;
    return Result;
}

VOID
RtlpDeCommitQueryDebugInfo(
    IN PRTL_DEBUG_INFORMATION Buffer,
    IN PVOID p,
    IN ULONG Size
    )
{
    Size = (Size + 3) & ~3;
    if (p == (PVOID)(Buffer->OffsetFree - Size)) {
        Buffer->OffsetFree -= Size;
        }
}

NTSYSAPI
PRTL_DEBUG_INFORMATION
NTAPI
RtlCreateQueryDebugBuffer(
    IN ULONG MaximumCommit OPTIONAL,
    IN BOOLEAN UseEventPair
    )
{
    NTSTATUS Status;
    HANDLE Section;
    PRTL_DEBUG_INFORMATION Buffer;
    LARGE_INTEGER MaximumSize;
    ULONG_PTR ViewSize, CommitSize;

    if (!ARGUMENT_PRESENT( (PVOID)(ULONG_PTR)MaximumCommit )) { // Sundown Note: ULONG zero-extended.
        MaximumCommit = 4 * 1024 * 1024;
        }
    MaximumSize.QuadPart = MaximumCommit;
    Status = NtCreateSection( &Section,
                              SECTION_ALL_ACCESS,
                              NULL,
                              &MaximumSize,
                              PAGE_READWRITE,
                              SEC_RESERVE,
                              NULL
                            );
    if (!NT_SUCCESS( Status )) {
        return NULL;
        }

    Buffer = NULL;
    ViewSize = MaximumCommit;
    Status = NtMapViewOfSection( Section,
                                 NtCurrentProcess(),
                                 &Buffer,
                                 0,
                                 0,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_READWRITE
                               );
    if (!NT_SUCCESS( Status )) {
        NtClose( Section );
        return NULL;
        }

    CommitSize = 1;
    Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                      &Buffer,
                                      0,
                                      &CommitSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    if (!NT_SUCCESS( Status )) {
        NtUnmapViewOfSection( NtCurrentProcess(), Buffer );
        NtClose( Section );
        return NULL;
        }

    if (UseEventPair) {
        Status = NtCreateEventPair( &Buffer->EventPairClient,
                                    EVENT_PAIR_ALL_ACCESS,
                                    NULL
                                  );
        if (!NT_SUCCESS( Status )) {
            NtUnmapViewOfSection( NtCurrentProcess(), Buffer );
            NtClose( Section );
            return NULL;
            }
        }

    Buffer->SectionHandleClient = Section;
    Buffer->ViewBaseClient = Buffer;
    Buffer->OffsetFree = 0;
    Buffer->CommitSize = CommitSize;
    Buffer->ViewSize = ViewSize;
    return Buffer;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(
    IN PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    HANDLE ProcessHandle, ThreadHandle;
    THREAD_BASIC_INFORMATION BasicInformation;
    PTEB Teb;
    PVOID StackDeallocationBase;
    ULONG Size;
    SIZE_T BigSize;

    RtlpChangeQueryDebugBufferTarget( Buffer, NULL, NULL );

    Status = STATUS_SUCCESS;
    if (Buffer->TargetThreadHandle != NULL) {
        StackDeallocationBase = NULL;
        Status = NtQueryInformationThread( Buffer->TargetThreadHandle,
                                           ThreadBasicInformation,
                                           &BasicInformation,
                                           sizeof( BasicInformation ),
                                           NULL
                                         );
        if (NT_SUCCESS( Status )) {
            if (Teb = BasicInformation.TebBaseAddress) {
                NtReadVirtualMemory( Buffer->TargetProcessHandle,
                                     &Teb->DeallocationStack,
                                     &StackDeallocationBase,
                                     sizeof( StackDeallocationBase ),
                                     &Size
                                   );
                }
            }

        Buffer->EventPairTarget = NULL;
        NtSetLowEventPair( Buffer->EventPairClient );
        NtClose( Buffer->EventPairClient );
        Status = NtWaitForSingleObject( Buffer->TargetThreadHandle,
                                        TRUE,
                                        NULL
                                      );
        if (NT_SUCCESS( Status )) {
            Status = NtQueryInformationThread( Buffer->TargetThreadHandle,
                                               ThreadBasicInformation,
                                               &BasicInformation,
                                               sizeof( BasicInformation ),
                                               NULL
                                             );
            if (NT_SUCCESS( Status )) {
                Status = BasicInformation.ExitStatus;
                }
            }

        BigSize = 0;
        NtFreeVirtualMemory( Buffer->TargetProcessHandle,
                             &StackDeallocationBase,
                             &BigSize,
                             MEM_RELEASE
                           );
        }

    NtClose( Buffer->SectionHandleClient );
    NtUnmapViewOfSection( NtCurrentProcess(), Buffer );
    return Status;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    IN HANDLE UniqueProcessId,
    IN ULONG Flags,
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    HANDLE ProcessHandle, ThreadHandle;
    THREAD_BASIC_INFORMATION BasicInformation;
    PTEB Teb;
    PVOID StackDeallocationBase;
    ULONG Size;
    SIZE_T BigSize;

    Buffer->Flags = Flags;
    if (Buffer->OffsetFree != 0) {
        RtlZeroMemory( (Buffer+1), Buffer->OffsetFree - (SIZE_T)sizeof(*Buffer) );
        }
    Buffer->OffsetFree = sizeof( *Buffer );

    if (NtCurrentTeb()->ClientId.UniqueProcess != UniqueProcessId) {
        //
        // Not querying current process, so do it remotely
        //
        ProcessHandle = NULL;
        Status = RtlpChangeQueryDebugBufferTarget( Buffer, UniqueProcessId, &ProcessHandle );
        if (!NT_SUCCESS( Status )) {
            return Status;
            }

        if (ProcessHandle == NULL) {
waitForDump:
            Status = NtSetLowWaitHighEventPair( Buffer->EventPairClient );
            }
        else {
            //
            // don't let the debugger see this remote thread !
            // This is a very ugly but effective way to prevent
            // the debugger deadlocking with the target process when calling
            // this function.
            //

            Status = RtlCreateUserThread( ProcessHandle,
                                          NULL,
                                          TRUE,
                                          0,
                                          0,
                                          0,
                                          RtlpQueryProcessDebugInformationRemote,
                                          Buffer->ViewBaseTarget,
                                          &ThreadHandle,
                                          NULL
                                        );
            if (NT_SUCCESS( Status )) {

                Status = NtSetInformationThread( ThreadHandle,
                                                 ThreadHideFromDebugger,
                                                 NULL,
                                                 0
                                               );

                if ( !NT_SUCCESS(Status) ) {
                    NtTerminateThread(ThreadHandle,Status);
                    NtClose(ThreadHandle);
                    NtClose(ProcessHandle);
                    return Status;
                    }

                NtResumeThread(ThreadHandle,NULL);

                if (Buffer->EventPairClient != NULL) {
                    Buffer->TargetThreadHandle = ThreadHandle;
                    Buffer->TargetProcessHandle = ProcessHandle;
                    goto waitForDump;
                    }

                StackDeallocationBase = NULL;
                Status = NtQueryInformationThread( ThreadHandle,
                                                   ThreadBasicInformation,
                                                   &BasicInformation,
                                                   sizeof( BasicInformation ),
                                                   NULL
                                                 );
                if (NT_SUCCESS( Status )) {
                    if (Teb = BasicInformation.TebBaseAddress) {
                        NtReadVirtualMemory( ProcessHandle,
                                             &Teb->DeallocationStack,
                                             &StackDeallocationBase,
                                             sizeof( StackDeallocationBase ),
                                             &Size
                                           );
                        }
                    }

                Status = NtWaitForSingleObject( ThreadHandle,
                                                TRUE,
                                                NULL
                                              );

                if (NT_SUCCESS( Status )) {
                    Status = NtQueryInformationThread( ThreadHandle,
                                                       ThreadBasicInformation,
                                                       &BasicInformation,
                                                       sizeof( BasicInformation ),
                                                       NULL
                                                     );
                    if (NT_SUCCESS( Status )) {
                        Status = BasicInformation.ExitStatus;
                        }
                    }

                BigSize = 0;
                NtFreeVirtualMemory( ProcessHandle, &StackDeallocationBase, &BigSize, MEM_RELEASE );
                NtClose( ThreadHandle );

                }


            NtClose( ProcessHandle );
            }
        }
    else {
        if (Flags & RTL_QUERY_PROCESS_MODULES) {
            Status = RtlQueryProcessModuleInformation( Buffer );
            if (Status != STATUS_SUCCESS) {
                return Status;
                }
            }

        if (Flags & RTL_QUERY_PROCESS_BACKTRACES) {
            Status = RtlQueryProcessBackTraceInformation( Buffer );
            if (Status != STATUS_SUCCESS) {
                return Status;
                }
            }

        if (Flags & RTL_QUERY_PROCESS_LOCKS) {
            Status = RtlQueryProcessLockInformation( Buffer );
            if (Status != STATUS_SUCCESS) {
                return Status;
                }
            }

        if (Flags & (RTL_QUERY_PROCESS_HEAP_SUMMARY |
                     RTL_QUERY_PROCESS_HEAP_TAGS |
                     RTL_QUERY_PROCESS_HEAP_ENTRIES
                    )
           ) {
            Status = RtlQueryProcessHeapInformation( Buffer );
            if (Status != STATUS_SUCCESS) {
                return Status;
                }
            }
        }

    return Status;
}


NTSTATUS
NTAPI
RtlQueryProcessModuleInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    ULONG RequiredLength;
    PRTL_PROCESS_MODULES Modules;

    Status = LdrQueryProcessModuleInformation( NULL,
                                               0,
                                               &RequiredLength
                                             );
    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        Modules = RtlpCommitQueryDebugInfo( Buffer, RequiredLength );
        if (Modules != NULL) {
            Status = LdrQueryProcessModuleInformation( Modules,
                                                       RequiredLength,
                                                       &RequiredLength
                                                     );
            if (NT_SUCCESS( Status )) {
                Buffer->Modules = Modules;
                return STATUS_SUCCESS;
                }
            }
        }

    return STATUS_NO_MEMORY;
}

NTSTATUS
RtlQueryProcessBackTraceInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
{
#if i386
    NTSTATUS Status;
    OUT PRTL_PROCESS_BACKTRACES BackTraces;
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    PSTACK_TRACE_DATABASE DataBase;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG n;

    DataBase = RtlpAcquireStackTraceDataBase();
    if (DataBase == NULL) {
        return STATUS_SUCCESS;
        }

    BackTraces = RtlpCommitQueryDebugInfo( Buffer, FIELD_OFFSET( RTL_PROCESS_BACKTRACES, BackTraces ) );
    if (BackTraces == NULL) {
        return STATUS_NO_MEMORY;
        }

    DataBase->DumpInProgress = TRUE;
    RtlpReleaseStackTraceDataBase();
    try {
        BackTraces->CommittedMemory = (ULONG)DataBase->CurrentUpperCommitLimit -
                                      (ULONG)DataBase->CommitBase;
        BackTraces->ReservedMemory =  (ULONG)DataBase->EntryIndexArray -
                                      (ULONG)DataBase->CommitBase;
        BackTraces->NumberOfBackTraceLookups = DataBase->NumberOfEntriesLookedUp;
        BackTraces->NumberOfBackTraces = DataBase->NumberOfEntriesAdded;
        BackTraceInfo = RtlpCommitQueryDebugInfo( Buffer, (sizeof( *BackTraceInfo ) * BackTraces->NumberOfBackTraces) );
        if (BackTraceInfo == NULL) {
            Status = STATUS_NO_MEMORY;
            }
        else {
            Status = STATUS_SUCCESS;
            n = DataBase->NumberOfEntriesAdded;
            pp = DataBase->EntryIndexArray;
            while (n--) {
                p = *--pp;
                BackTraceInfo->SymbolicBackTrace = NULL;
                BackTraceInfo->TraceCount = p->TraceCount;
                BackTraceInfo->Index = p->Index;
                BackTraceInfo->Depth = p->Depth;
                RtlMoveMemory( BackTraceInfo->BackTrace,
                               p->BackTrace,
                               p->Depth * sizeof( PVOID )
                             );
                BackTraceInfo++;
                }
            }
        }
    finally {
        DataBase->DumpInProgress = FALSE;
        }

    if (NT_SUCCESS( Status )) {
        Buffer->BackTraces = BackTraces;
        }

    return Status;
#else
    return STATUS_SUCCESS;
#endif // i386
}


NTSTATUS
RtlpQueryProcessEnumHeapsRoutine(
    PVOID HeapHandle,
    PVOID Parameter
    )
{
    PRTL_DEBUG_INFORMATION Buffer = (PRTL_DEBUG_INFORMATION)Parameter;
    PRTL_PROCESS_HEAPS Heaps = Buffer->Heaps;
    PHEAP Heap = (PHEAP)HeapHandle;
    PRTL_HEAP_INFORMATION HeapInfo;
    PHEAP_SEGMENT Segment;
    UCHAR SegmentIndex;

    HeapInfo = RtlpCommitQueryDebugInfo( Buffer, sizeof( *HeapInfo ) );
    if (HeapInfo == NULL) {
        return STATUS_NO_MEMORY;
        }

    HeapInfo->BaseAddress = Heap;
    HeapInfo->Flags = Heap->Flags;
    HeapInfo->EntryOverhead = sizeof( HEAP_ENTRY );
    HeapInfo->CreatorBackTraceIndex = Heap->AllocatorBackTraceIndex;
    SegmentIndex = HEAP_MAXIMUM_SEGMENTS;
    while (SegmentIndex--) {
        Segment = Heap->Segments[ SegmentIndex ];
        if (Segment) {
            HeapInfo->BytesCommitted += (Segment->NumberOfPages -
                                         Segment->NumberOfUnCommittedPages
                                        ) * PAGE_SIZE;

            }
        }
    HeapInfo->BytesAllocated = HeapInfo->BytesCommitted -
                               (Heap->TotalFreeSize << HEAP_GRANULARITY_SHIFT);
    Heaps->NumberOfHeaps += 1;
    return STATUS_SUCCESS;
}

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessHeapInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    PHEAP Heap;
    BOOLEAN LockAcquired;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_HEAP_INFORMATION HeapInfo;
    ULONG NumberOfHeaps;
    PVOID *ProcessHeaps;
    UCHAR SegmentIndex;
    ULONG i, n, TagIndex;
    PHEAP_SEGMENT Segment;
    PRTL_HEAP_TAG Tags;
    PHEAP_PSEUDO_TAG_ENTRY PseudoTags;
    PRTL_HEAP_ENTRY Entries;
    PHEAP_ENTRY CurrentBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    PRTL_HEAP_ENTRY p;
    PLIST_ENTRY Head, Next;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;
    ULONG Size;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;

    Heaps = RtlpCommitQueryDebugInfo( Buffer, FIELD_OFFSET( RTL_PROCESS_HEAPS, Heaps ) );
    if (Heaps == NULL) {
        return STATUS_NO_MEMORY;
        }

    Buffer->Heaps = Heaps;
    Status = RtlEnumProcessHeaps( RtlpQueryProcessEnumHeapsRoutine, Buffer );
    if (NT_SUCCESS( Status )) {
        if (Buffer->Flags & RTL_QUERY_PROCESS_HEAP_TAGS) {
            Heap = RtlpGlobalTagHeap;
            if (Heap->TagEntries != NULL) {
                HeapInfo = RtlpCommitQueryDebugInfo( Buffer, sizeof( *HeapInfo ) );
                if (HeapInfo == NULL) {
                    return STATUS_NO_MEMORY;
                    }

                HeapInfo->BaseAddress = Heap;
                HeapInfo->Flags = Heap->Flags;
                HeapInfo->EntryOverhead = sizeof( HEAP_ENTRY );
                Heaps->NumberOfHeaps += 1;
                }

            for (i=0; i<Heaps->NumberOfHeaps; i++) {
                HeapInfo = &Heaps->Heaps[ i ];
                if (Buffer->SpecificHeap == NULL ||
                    Buffer->SpecificHeap == HeapInfo->BaseAddress
                   ) {
                    Heap = HeapInfo->BaseAddress;
                    HeapInfo->NumberOfTags = Heap->NextAvailableTagIndex;
                    n = HeapInfo->NumberOfTags * sizeof( RTL_HEAP_TAG );
                    if (Heap->PseudoTagEntries != NULL) {
                        HeapInfo->NumberOfTags += HEAP_MAXIMUM_FREELISTS + 1;
                        n += (HEAP_MAXIMUM_FREELISTS + 1) * sizeof( RTL_HEAP_TAG );
                        }
                    Tags = RtlpCommitQueryDebugInfo( Buffer, n );
                    if (Tags == NULL) {
                        Status = STATUS_NO_MEMORY;
                        break;
                        }
                    HeapInfo->Tags = Tags;
                    if ((PseudoTags = Heap->PseudoTagEntries) != NULL) {
                        HeapInfo->NumberOfPseudoTags = HEAP_NUMBER_OF_PSEUDO_TAG;
                        HeapInfo->PseudoTagGranularity = HEAP_GRANULARITY;
                        for (TagIndex=0; TagIndex<=HEAP_MAXIMUM_FREELISTS; TagIndex++) {
                            Tags->NumberOfAllocations = PseudoTags->Allocs;
                            Tags->NumberOfFrees = PseudoTags->Frees;
                            Tags->BytesAllocated = PseudoTags->Size << HEAP_GRANULARITY_SHIFT;
                            Tags->TagIndex = (USHORT)(TagIndex | HEAP_PSEUDO_TAG_FLAG);
                            if (TagIndex == 0) {
                                swprintf( Tags->TagName, L"Objects>%4u",
                                          HEAP_MAXIMUM_FREELISTS << HEAP_GRANULARITY_SHIFT
                                        );
                                }
                            else
                            if (TagIndex < HEAP_MAXIMUM_FREELISTS) {
                                swprintf( Tags->TagName, L"Objects=%4u",
                                          TagIndex << HEAP_GRANULARITY_SHIFT
                                        );
                                }
                            else {
                                swprintf( Tags->TagName, L"VirtualAlloc",
                                          TagIndex << HEAP_GRANULARITY_SHIFT
                                        );
                                }

                            Tags += 1;
                            PseudoTags += 1;
                            }
                        }

                    RtlMoveMemory( Tags,
                                   Heap->TagEntries,
                                   Heap->NextAvailableTagIndex * sizeof( RTL_HEAP_TAG )
                                 );
                    for (TagIndex=0; TagIndex<Heap->NextAvailableTagIndex; TagIndex++) {
                        Tags->BytesAllocated <<= HEAP_GRANULARITY_SHIFT;
                        Tags += 1;
                        }
                    }
                }
            }
        }
    else {
        Buffer->Heaps = NULL;
        }
    if (NT_SUCCESS( Status )) {
        if (Buffer->Flags & RTL_QUERY_PROCESS_HEAP_ENTRIES) {
            for (i=0; i<Heaps->NumberOfHeaps; i++) {
                HeapInfo = &Heaps->Heaps[ i ];
                Heap = HeapInfo->BaseAddress;
                if (Buffer->SpecificHeap == NULL ||
                    Buffer->SpecificHeap == Heap
                   ) {
                    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {
                        RtlEnterCriticalSection( (PRTL_CRITICAL_SECTION)Heap->LockVariable );
                        LockAcquired = TRUE;
                        }
                    else {
                        LockAcquired = FALSE;
                        }

                    try {
                        for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {
                            Segment = Heap->Segments[ SegmentIndex ];
                            if (!Segment) {
                                continue;
                                }

                            Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                            if (Entries == NULL) {
                                Status = STATUS_NO_MEMORY;
                                break;
                                }
                            else
                            if (HeapInfo->Entries == NULL) {
                                HeapInfo->Entries = Entries;
                                }

                            Entries->Flags = RTL_HEAP_SEGMENT;
                            Entries->AllocatorBackTraceIndex = Segment->AllocatorBackTraceIndex;
                            Entries->Size = Segment->NumberOfPages * PAGE_SIZE;
                            Entries->u.s2.CommittedSize = (Segment->NumberOfPages -
                                                           Segment->NumberOfUnCommittedPages
                                                          ) * PAGE_SIZE;
                            Entries->u.s2.FirstBlock = Segment->FirstEntry;
                            HeapInfo->NumberOfEntries++;

                            UnCommittedRange = Segment->UnCommittedRanges;
                            CurrentBlock = Segment->FirstEntry;
                            while (CurrentBlock < Segment->LastValidEntry) {
                                Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                                if (Entries == NULL) {
                                    Status = STATUS_NO_MEMORY;
                                    break;
                                    }

                                Size = CurrentBlock->Size << HEAP_GRANULARITY_SHIFT;
                                Entries->Size = Size;
                                HeapInfo->NumberOfEntries++;
                                if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {
                                    if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                        ExtraStuff = (PHEAP_ENTRY_EXTRA)(CurrentBlock + CurrentBlock->Size - 1);
#if i386
                                        Entries->AllocatorBackTraceIndex = ExtraStuff->AllocatorBackTraceIndex;
#endif // i386
                                        Entries->Flags |= RTL_HEAP_SETTABLE_VALUE;
                                        Entries->u.s1.Settable = ExtraStuff->Settable;
                                        Entries->u.s1.Tag = ExtraStuff->TagIndex;
                                        }
                                    else {
                                        Entries->u.s1.Tag = CurrentBlock->SmallTagIndex;
                                        }

                                    Entries->Flags |= RTL_HEAP_BUSY | (CurrentBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS);
                                    }
                                else
                                if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                    PHEAP_FREE_ENTRY_EXTRA FreeExtra;

                                    FreeExtra = (PHEAP_FREE_ENTRY_EXTRA)(CurrentBlock + CurrentBlock->Size) - 1;
                                    Entries->u.s1.Tag = FreeExtra->TagIndex;
                                    Entries->AllocatorBackTraceIndex = FreeExtra->FreeBackTraceIndex;
                                    }

                                if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {
                                    CurrentBlock += CurrentBlock->Size;
                                    if (UnCommittedRange == NULL) {
                                        CurrentBlock = Segment->LastValidEntry;
                                        }
                                    else {
                                        Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                                        if (Entries == NULL) {
                                            Status = STATUS_NO_MEMORY;
                                            break;
                                            }

                                        Entries->Flags = RTL_HEAP_UNCOMMITTED_RANGE;
                                        Entries->Size = UnCommittedRange->Size;
                                        HeapInfo->NumberOfEntries++;

                                        CurrentBlock = (PHEAP_ENTRY)
                                            ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);
                                        UnCommittedRange = UnCommittedRange->Next;
                                        }
                                    }
                                else {
                                    CurrentBlock += CurrentBlock->Size;
                                    }
                                }
                            }

                        Head = &Heap->VirtualAllocdBlocks;
                        Next = Head->Flink;
                        while (Head != Next) {
                            VirtualAllocBlock = CONTAINING_RECORD( Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry );
                            CurrentBlock = &VirtualAllocBlock->BusyBlock;
                            Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                            if (Entries == NULL) {
                                Status = STATUS_NO_MEMORY;
                                break;
                                }
                            else
                            if (HeapInfo->Entries == NULL) {
                                HeapInfo->Entries = Entries;
                                }

                            Entries->Flags = RTL_HEAP_SEGMENT;
                            Entries->Size = VirtualAllocBlock->ReserveSize;
                            Entries->u.s2.CommittedSize = VirtualAllocBlock->CommitSize;
                            Entries->u.s2.FirstBlock = CurrentBlock;
                            HeapInfo->NumberOfEntries++;
                            Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                            if (Entries == NULL) {
                                Status = STATUS_NO_MEMORY;
                                break;
                                }

                            Entries->Size = VirtualAllocBlock->CommitSize;
                            Entries->Flags = RTL_HEAP_BUSY | (CurrentBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS);
#if i386
                            Entries->AllocatorBackTraceIndex = VirtualAllocBlock->ExtraStuff.AllocatorBackTraceIndex;
#endif // i386
                            Entries->Flags |= RTL_HEAP_SETTABLE_VALUE;
                            Entries->u.s1.Settable = VirtualAllocBlock->ExtraStuff.Settable;
                            Entries->u.s1.Tag = VirtualAllocBlock->ExtraStuff.TagIndex;
                            HeapInfo->NumberOfEntries++;

                            Next = Next->Flink;
                            }
                        }
                    finally {
                        //
                        // Unlock the heap
                        //

                        if (LockAcquired) {
                            RtlLeaveCriticalSection( (PRTL_CRITICAL_SECTION)Heap->LockVariable );
                            }
                        }
                    }

                if (!NT_SUCCESS( Status )) {
                    break;
                    }
                }
            }
        }

    return Status;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessLockInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PRTL_PROCESS_LOCKS Locks;
    PRTL_PROCESS_LOCK_INFORMATION LockInfo;
    PRTL_CRITICAL_SECTION CriticalSection;
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    PRTL_RESOURCE Resource;
    PRTL_RESOURCE_DEBUG ResourceDebugInfo;

    Locks = RtlpCommitQueryDebugInfo( Buffer, FIELD_OFFSET( RTL_PROCESS_LOCKS, Locks ) );
    if (Locks == NULL) {
        return STATUS_NO_MEMORY;
        }

    RtlEnterCriticalSection( &RtlCriticalSectionLock );
    Head = &RtlCriticalSectionList;
    Next = Head->Flink;
    Status = STATUS_SUCCESS;
    while (Next != Head) {
        DebugInfo = CONTAINING_RECORD( Next,
                                       RTL_CRITICAL_SECTION_DEBUG,
                                       ProcessLocksList
                                     );
        LockInfo = RtlpCommitQueryDebugInfo( Buffer, sizeof( RTL_PROCESS_LOCK_INFORMATION ) );
        if (LockInfo == NULL) {
            Status = STATUS_NO_MEMORY;
            break;
            }

        CriticalSection = DebugInfo->CriticalSection;
        try {
            LockInfo->Address = CriticalSection;
            LockInfo->Type = DebugInfo->Type;
            LockInfo->CreatorBackTraceIndex = DebugInfo->CreatorBackTraceIndex;
            if (LockInfo->Type == RTL_CRITSECT_TYPE) {
                LockInfo->OwningThread = CriticalSection->OwningThread;
                LockInfo->LockCount = CriticalSection->LockCount;
                LockInfo->RecursionCount = CriticalSection->RecursionCount;
                LockInfo->ContentionCount = DebugInfo->ContentionCount;
                LockInfo->EntryCount = DebugInfo->EntryCount;
                }
            else {
                Resource = (PRTL_RESOURCE)CriticalSection;
                ResourceDebugInfo = Resource->DebugInfo;
                LockInfo->ContentionCount = ResourceDebugInfo->ContentionCount;
                LockInfo->OwningThread = Resource->ExclusiveOwnerThread;
                LockInfo->LockCount = Resource->NumberOfActive;
                LockInfo->NumberOfWaitingShared    = Resource->NumberOfWaitingShared;
                LockInfo->NumberOfWaitingExclusive = Resource->NumberOfWaitingExclusive;
                }

            Locks->NumberOfLocks++;
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrint("NTDLL: Lost critical section %08lX\n", CriticalSection);
            RtlpDeCommitQueryDebugInfo( Buffer, LockInfo, sizeof( RTL_PROCESS_LOCK_INFORMATION ) );
            }

        if (Next == Next->Flink) {
            //
            // Bail if list is circular
            //
            break;
            }
        else {
            Next = Next->Flink;
            }
        }

    RtlLeaveCriticalSection( &RtlCriticalSectionLock );

    if (NT_SUCCESS( Status )) {
        Buffer->Locks = Locks;
        }

    return Status;
}
