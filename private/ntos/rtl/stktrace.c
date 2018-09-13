/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stktrace.c

Abstract:

    This module implements routines to snapshot a set of stack back traces
    in a data base.  Useful for heap allocators to track allocation requests
    cheaply.

Author:

    Steve Wood (stevewo) 29-Jan-1992

Revision History:

    17-May-1999 (silviuc) : added RtlWalkFrameChain that replaces the
    unsafe RtlCaptureStackBackTrace.

--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>
#include <stktrace.h>
#include <heap.h>
#include <heappriv.h>

BOOLEAN
NtdllOkayToLockRoutine(
    IN PVOID Lock
    );

#if !defined(RtlGetCallersAddress) && defined(_X86_) && (!NTOS_KERNEL_RUNTIME)

VOID
RtlGetCallersAddress(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
    )
/*++

Routine Description:

    This routine returns the first to callers on the current stack. It should be
    noted that the function can miss some of the callers in the presence of FPO
    optimization.

Arguments:

    CallersAddress - address to save the first caller.
    
    CallersCaller - address to save the second caller.

Return Value:

    None. If the function does not succeed in finding the two callers
    it will zero the addresses where it was supposed to write them.

Environment:

    X86, user mode and w/o having a macro with same name defined.

--*/

{
    PVOID BackTrace[ 2 ];
    ULONG Hash;
    USHORT Count;

    Count = RtlCaptureStackBackTrace(
        2,
        2,
        BackTrace,
        &Hash
        );

    if (ARGUMENT_PRESENT( CallersAddress )) {
        if (Count >= 1) {
            *CallersAddress = BackTrace[ 0 ];
        }
        else {
            *CallersAddress = NULL;
        }
    }

    if (ARGUMENT_PRESENT( CallersCaller )) {
        if (Count >= 2) {
            *CallersCaller = BackTrace[ 1 ];
        }
        else {
            *CallersCaller = NULL;
        }
    }

    return;
}

#endif // !defined(RtlGetCallersAddress) && defined(_X86_) && (!NTOS_KERNEL_RUNTIME)

// bugbug (silviuc): I do not think !FPO is correct below
// The reason is ExtendDb function uses VirtualAlloc.
#if defined(_X86_) && (!NTOS_KERNEL_RUNTIME || !FPO)

//
// Global per process stack trace database.
//

PSTACK_TRACE_DATABASE RtlpStackTraceDataBase;


PRTL_STACK_TRACE_ENTRY
RtlpExtendStackTraceDataBase(
    IN PRTL_STACK_TRACE_ENTRY InitialValue,
    IN ULONG Size
    );


NTSTATUS
RtlInitStackTraceDataBaseEx(
    IN PVOID CommitBase,
    IN ULONG CommitSize,
    IN ULONG ReserveSize,
    IN PRTL_INITIALIZE_LOCK_ROUTINE InitializeLockRoutine,
    IN PRTL_ACQUIRE_LOCK_ROUTINE AcquireLockRoutine,
    IN PRTL_RELEASE_LOCK_ROUTINE ReleaseLockRoutine,
    IN PRTL_OKAY_TO_LOCK_ROUTINE OkayToLockRoutine
    );

NTSTATUS
RtlInitStackTraceDataBaseEx(
    IN PVOID CommitBase,
    IN ULONG CommitSize,
    IN ULONG ReserveSize,
    IN PRTL_INITIALIZE_LOCK_ROUTINE InitializeLockRoutine,
    IN PRTL_ACQUIRE_LOCK_ROUTINE AcquireLockRoutine,
    IN PRTL_RELEASE_LOCK_ROUTINE ReleaseLockRoutine,
    IN PRTL_OKAY_TO_LOCK_ROUTINE OkayToLockRoutine
    )
{
    NTSTATUS Status;
    PSTACK_TRACE_DATABASE DataBase;

    DataBase = (PSTACK_TRACE_DATABASE)CommitBase;
    if (CommitSize == 0) {
        CommitSize = PAGE_SIZE;
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&CommitBase,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                      Status
                   ));
            return Status;
            }

        DataBase->PreCommitted = FALSE;
        }
    else
    if (CommitSize == ReserveSize) {
        RtlZeroMemory( DataBase, sizeof( *DataBase ) );
        DataBase->PreCommitted = TRUE;
        }
    else {
        return STATUS_INVALID_PARAMETER;
        }

    DataBase->CommitBase = CommitBase;
    DataBase->NumberOfBuckets = 37;
    DataBase->NextFreeLowerMemory = (PCHAR)
        (&DataBase->Buckets[ DataBase->NumberOfBuckets ]);
    DataBase->NextFreeUpperMemory = (PCHAR)CommitBase + ReserveSize;

    if(!DataBase->PreCommitted) {
        DataBase->CurrentLowerCommitLimit = (PCHAR)CommitBase + CommitSize;
        DataBase->CurrentUpperCommitLimit = (PCHAR)CommitBase + ReserveSize;
        }
    else {
        RtlZeroMemory( &DataBase->Buckets[ 0 ],
                       DataBase->NumberOfBuckets * sizeof( DataBase->Buckets[ 0 ] )
                     );
        }

    DataBase->EntryIndexArray = (PRTL_STACK_TRACE_ENTRY *)DataBase->NextFreeUpperMemory;

    DataBase->AcquireLockRoutine = AcquireLockRoutine;
    DataBase->ReleaseLockRoutine = ReleaseLockRoutine;
    DataBase->OkayToLockRoutine = OkayToLockRoutine;

    Status = (InitializeLockRoutine)( &DataBase->Lock.CriticalSection );
    if (!NT_SUCCESS( Status )) {
        KdPrint(( "RTL: Unable to initialize stack trace data base CriticalSection,  Status = %lx\n",
                  Status
               ));
        return( Status );
        }

    RtlpStackTraceDataBase = DataBase;
    return( STATUS_SUCCESS );
}

NTSTATUS
RtlInitializeStackTraceDataBase(
    IN PVOID CommitBase,
    IN ULONG CommitSize,
    IN ULONG ReserveSize
    )
{
#ifdef NTOS_KERNEL_RUNTIME

BOOLEAN
ExOkayToLockRoutine(
    IN PVOID Lock
    );

    return RtlInitStackTraceDataBaseEx(
                CommitBase,
                CommitSize,
                ReserveSize,
                ExInitializeResource,
                (PRTL_RELEASE_LOCK_ROUTINE)ExAcquireResourceExclusive,
                (PRTL_RELEASE_LOCK_ROUTINE)ExReleaseResourceLite,
                ExOkayToLockRoutine
                );
#else // #ifdef NTOS_KERNEL_RUNTIME

    return RtlInitStackTraceDataBaseEx(
                CommitBase,
                CommitSize,
                ReserveSize,
                RtlInitializeCriticalSection,
                RtlEnterCriticalSection,
                RtlLeaveCriticalSection,
                NtdllOkayToLockRoutine
                );
#endif // #ifdef NTOS_KERNEL_RUNTIME
}


PSTACK_TRACE_DATABASE
RtlpAcquireStackTraceDataBase( VOID )
{
    if (RtlpStackTraceDataBase != NULL) {
        if (RtlpStackTraceDataBase->DumpInProgress ||
            !(RtlpStackTraceDataBase->OkayToLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection )
           ) {
            return( NULL );
            }

        (RtlpStackTraceDataBase->AcquireLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection );
        }

    return( RtlpStackTraceDataBase );
}

VOID
RtlpReleaseStackTraceDataBase( VOID )
{
    (RtlpStackTraceDataBase->ReleaseLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection );
    return;
}

PRTL_STACK_TRACE_ENTRY
RtlpExtendStackTraceDataBase(
    IN PRTL_STACK_TRACE_ENTRY InitialValue,
    IN ULONG Size
    )
/*++

Routine Description:

    This routine extends the stack trace database in order to accomodate
    the new stack trace that has to be saved.

Arguments:

    InitialValue - stack trace to be saved.
    
    Size - size of the stack trace in bytes. Note that this is not the
        depth of the trace but rather `Depth * sizeof(PVOID)'.

Return Value:

    The address of the just saved stack trace or null in case we have hit
    the maximum size of the database or we get commit errors.

Environment:

    User mode. 
    
    Note. In order to make all this code work in kernel mode we have to
    rewrite this function that relies on VirtualAlloc.

--*/

{
    NTSTATUS Status;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG CommitSize;
    PSTACK_TRACE_DATABASE DataBase;

    DataBase = RtlpStackTraceDataBase;

    //
    // We will try to find space for one stack trace entry in the
    // upper part of the database.
    //

    pp = (PRTL_STACK_TRACE_ENTRY *)DataBase->NextFreeUpperMemory;

    if ((! DataBase->PreCommitted) &&
        ((PCHAR)(pp - 1) < (PCHAR)DataBase->CurrentUpperCommitLimit)) {

        //
        // No more committed space in the upper part of the database.
        // We need to extend it downwards.
        //

        DataBase->CurrentUpperCommitLimit = 
            (PVOID)((PCHAR)DataBase->CurrentUpperCommitLimit - PAGE_SIZE);

        if (DataBase->CurrentUpperCommitLimit < DataBase->CurrentLowerCommitLimit) {

            //
            // No more space at all. We have got over the lower part of the db.
            // We failed therefore increase back the UpperCommitLimit pointer.
            //

            DataBase->CurrentUpperCommitLimit = 
                (PVOID)((PCHAR)DataBase->CurrentUpperCommitLimit + PAGE_SIZE);

            return( NULL );
        }

        CommitSize = PAGE_SIZE;
        Status = ZwAllocateVirtualMemory( 
            NtCurrentProcess(),
            (PVOID *)&DataBase->CurrentUpperCommitLimit,
            0,
            &CommitSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (!NT_SUCCESS( Status )) {

            //
            // We tried to increase the upper part of the db by one page.
            // We failed therefore increase back the UpperCommitLimit pointer
            //

            DataBase->CurrentUpperCommitLimit = 
                (PVOID)((PCHAR)DataBase->CurrentUpperCommitLimit + PAGE_SIZE);

            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                Status
                ));
            return( NULL );
        }
    }

    //
    // We managed to make sure we have usable space in the upper part
    // therefore we take out one stack trace entry address.
    //

    DataBase->NextFreeUpperMemory -= sizeof( *pp );

    //
    // Now we will try to find space in the lower part of the database for
    // for the eactual stack trace.
    //

    p = (PRTL_STACK_TRACE_ENTRY)DataBase->NextFreeLowerMemory;

    if ((! DataBase->PreCommitted) &&
        (((PCHAR)p + Size) > (PCHAR)DataBase->CurrentLowerCommitLimit)) {

        //
        // We need to extend the lower part.
        //

        if (DataBase->CurrentLowerCommitLimit >= DataBase->CurrentUpperCommitLimit) {

            //
            // We have hit the maximum size of the database.
            //

            return( NULL );
        }

        //
        // Extend the lower part of the database by one page.
        //

        CommitSize = Size;
        Status = ZwAllocateVirtualMemory( 
            NtCurrentProcess(),
            (PVOID *)&DataBase->CurrentLowerCommitLimit,
            0,
            &CommitSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (! NT_SUCCESS( Status )) {
            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                Status
                ));
            return( NULL );
        }

        DataBase->CurrentLowerCommitLimit =
            (PCHAR)DataBase->CurrentLowerCommitLimit + CommitSize;
    }

    //
    // Take out the space for the stack trace.
    //

    DataBase->NextFreeLowerMemory += Size;

    //
    // Deal with a precommitted database case. If the lower and upper
    // pointers have crossed each other then rollback and return failure.
    //

    if (DataBase->PreCommitted &&
        DataBase->NextFreeLowerMemory >= DataBase->NextFreeUpperMemory) {

        DataBase->NextFreeUpperMemory += sizeof( *pp );
        DataBase->NextFreeLowerMemory -= Size;
        return( NULL );
    }

    //
    // Save the stack trace in the database
    //

    RtlMoveMemory( p, InitialValue, Size );
    p->HashChain = NULL;
    p->TraceCount = 0;
    p->Index = (USHORT)(++DataBase->NumberOfEntriesAdded);

    //
    // Save the address of the new stack trace entry in the
    // upper part of the databse.
    //

    *--pp = p;

    //
    // Return address of the saved stack trace entry.
    //

    return( p );
}

USHORT
RtlLogStackBackTrace( 
    VOID 
    )
/*++

Routine Description:

    This routine will capture the current stacktrace (skipping the 
    present function) and will save it in the global (per process) 
    stack trace database. It should be noted that we do not save
    duplicate traces.

Arguments:

    None.

Return Value:

    Index of the stack trace saved. The index can be used by tools
    to access quickly the trace data. This is the reason at the end of
    the database we save downwards a list of pointers to trace entries.
    This index can be used to find this pointer in constant time.
    
    A zero index will be returned for error conditions (e.g. stack 
    trace database not initialized).

Environment:

    User mode. 

--*/

{
    PSTACK_TRACE_DATABASE DataBase;
    RTL_STACK_TRACE_ENTRY StackTrace;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG Hash, RequestedSize, DepthSize;

    if (RtlpStackTraceDataBase == NULL) {
        return 0;
        }

    Hash = 0;

    //
    // Capture stack trace. The try/except was useful
    // in the old days when the function did not validate
    // the stack frame chain. We keep it just ot be defensive.
    //

    try {
        StackTrace.Depth = RtlCaptureStackBackTrace(
            1,
            MAX_STACK_DEPTH,
            StackTrace.BackTrace,
            &Hash
            );
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        StackTrace.Depth = 0;
    }

    if (StackTrace.Depth == 0) {
        return 0;
    }

    //
    // Lock the global per-process stack trace database.
    //

    DataBase = RtlpAcquireStackTraceDataBase();
    
    if (DataBase == NULL) {
        return( 0 );
    }

    DataBase->NumberOfEntriesLookedUp++;

    try {

        //
        // We will try to find out if the trace has been saved in the past.
        // We find the right hash chain and then traverse it.
        //

        DepthSize = StackTrace.Depth * sizeof( StackTrace.BackTrace[ 0 ] );
        pp = &DataBase->Buckets[ Hash % DataBase->NumberOfBuckets ];

        while (p = *pp) {
            if (p->Depth == StackTrace.Depth &&
                RtlCompareMemory( &p->BackTrace[ 0 ],
                &StackTrace.BackTrace[ 0 ],
                DepthSize
                ) == DepthSize
                ) {
                break;
            }
            else {
                pp = &p->HashChain;
            }
        }

        if (p == NULL) {

            //
            // We did not find the stack trace. We will extend the database
            // and save the new trace.
            //

            RequestedSize = FIELD_OFFSET( RTL_STACK_TRACE_ENTRY, BackTrace ) +
                DepthSize;

            p = RtlpExtendStackTraceDataBase( &StackTrace, RequestedSize );
            
            //
            // If we managed to stack the trace we need to link it as the last
            // element in the proper hash chain.
            //

            if (p != NULL) {
                *pp = p;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // bugbug (silviuc): We should not be here. Right?
        //

        p = NULL;
    }

    //
    // Release global trace db.
    //

    RtlpReleaseStackTraceDataBase();

    if (p != NULL) {
        p->TraceCount++;
        return( p->Index );
        }
    else {
        return( 0 );
        }

    return 0;
}
#endif // defined(_X86_) && !NTOS_KERNEL_RUNTIME


#if defined(_X86_)

USHORT
RtlCaptureStackBackTrace(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
    )
/*++

Routine Description:

    This routine walks up the stack frames, capturing the return address from
    each frame requested. This used to be implemented in assembly language and
    used to be unsafe in special contexts (DPC level). Right now it uses
    RtlWalkFrameChain that validates the chain of pointers and it guarantees
    not to take exceptions.

Arguments:

    FramesToSkip - frames detected but not included in the stack trace

    FramesToCapture - frames to be captured in the stack trace buffer. 
        One of the frames will be for RtlCaptureStackBackTrace.

    BackTrace - stack trace buffer

    BackTraceHash - very simple hash value that can be used to organize
      hash tables. It is just an arithmetic sum of the pointers in the
      stack trace buffer.

Return Value:

     Number of return addresses returned in the stack trace buffer.

--*/
{
    PVOID Trace [2 * MAX_STACK_DEPTH];
    ULONG FramesFound;
    ULONG HashValue;
    ULONG Index;

    //
    // One more frame to skip for the "capture" function (WalkFrameChain).
    //

    FramesToSkip++;

    //
    // Sanity checks.
    //

    if (FramesToCapture + FramesToSkip >= 2 * MAX_STACK_DEPTH) {
        return 0;
    }

    FramesFound = RtlWalkFrameChain (
        Trace,
        FramesToCapture + FramesToSkip,
        0);

    if (FramesFound <= FramesToSkip) {
        return 0;
    }

    for (HashValue = 0, Index = 0; Index < FramesToCapture; Index++) {

        if (FramesToSkip + Index >= FramesFound) {
            break;
        }

        BackTrace[Index] = Trace[FramesToSkip + Index];
        HashValue += PtrToUlong(BackTrace[Index]);
    }

    *BackTraceHash = HashValue;
    return (USHORT)Index;
}
#endif



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// RtlWalkFrameChain
/////////////////////////////////////////////////////////////////////

//
// This section contains an algorithm for getting stack traces.
// It works only on x86. It is an improvement of
// RtlCaptureStackBackTrace which for reasons that escape me is written
// in assembly and it is unsafe (can raise exceptions). The new function
// RtlWalkFrameChain is guaranteed to not take exceptions whatever the
// call context.
//
// Note. It might be a good idea to not BBT this code. Especially I am concerned
// about the only assembly instruction used in the whole code that saves
// the value of the EBP register.
//

#ifdef NTOS_KERNEL_RUNTIME
#define _KERNEL_MODE_STACK_TRACES_       1
#define _COLLECT_FRAME_WALK_STATISTICS_  0
#else
#define _KERNEL_MODE_STACK_TRACES_       0
#define _COLLECT_FRAME_WALK_STATISTICS_  0
#endif

#define SIZE_1_KB  ((ULONG_PTR) 0x400)
#define SIZE_1_GB  ((ULONG_PTR) 0x40000000)

#define PAGE_START(address) (((ULONG_PTR)address) & ~((ULONG_PTR)PAGE_SIZE - 1))

VOID CollectFrameWalkStatistics (ULONG Index);

#if (( i386 ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif

ULONG
RtlWalkFrameChain (

    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags)

/*++

Routine Description:

    RtlWalkFrameChain

Description:

    This function tries to walk the EBP chain and fill out a vector of
    return addresses. The function works only on x86. It is possible that
    the function cannot fill the requested number of callers because somewhere
    on the stack we have a function compiled FPO (the frame register (EBP) is
    used as a normal register. In this case the function will just return with
    a less then requested count. In kernel mode the function should not take
    any exceptions (page faults) because it can be called at all sorts of
    irql levels.

    The `Flags' parameter is used for future extensions. A zero value will be
    compatible with new stack walking algorithms.

    Note. The algorithm can be somewhat improved by unassembling the return
    addresses identified. However this is impractical in kernel mode because
    the function might get called at high irql levels where page faults are
    not allowed.

Return value:

    The number of identified return addresses on the stack. This can be less
    then the Count requested if the stack ends or we encounter a FPO compiled
    function.

--*/

{
#if defined(_X86_)

    ULONG_PTR Fp, NewFp, ReturnAddress;
    ULONG Index;
    ULONG_PTR StackEnd, StackStart;
    BOOLEAN Result;

    //
    // Get the current EBP pointer which is supposed to
    // be the start of the EBP chain.
    //

    _asm mov Fp, EBP;

    StackStart = Fp;

#if _KERNEL_MODE_STACK_TRACES_

    StackEnd = (ULONG_PTR)(KeGetCurrentThread()->StackBase);

    //
    // bugbug: find a reliable way to get the stack limit in kernel mode.
    // `StackBase' is not a reliable way to get the stack end in kernel
    // mode because we might execute a DPC routine on thread's behalf.
    // There are a few other reasons why we cannot trust this completely.
    //
    // Note. The condition `PAGE_START(StackEnd) - PAGE_START(StackStart) > PAGE_SIZE'
    // is not totally safe. We can encounter a situation where in this case we
    // do not have the same stack. Can we?
    //
    // The DPC stack is actually the stack of the idle thread corresponding to
    // the current processor. Based on that we probably can figure out in almost
    // all contexts what are the real limits of the stack.
    //

    if ((StackStart > StackEnd)
        || (PAGE_START(StackEnd) - PAGE_START(StackStart) > PAGE_SIZE)) {

        StackEnd = (StackStart + PAGE_SIZE) & ~((ULONG_PTR)PAGE_SIZE - 1);
    
        //
        // Try to get one more page if possible. Note that this is not
        // 100% reliable because a non faulting address can fault if
        // appropriate locks are not held.
        //

        if (MmIsAddressValid ((PVOID)StackEnd)) {
            StackEnd += PAGE_SIZE;
        }
    }

#else

    StackEnd = (ULONG_PTR)(NtCurrentTeb()->NtTib.StackBase);

#endif // #if _KERNEL_MODE_STACK_TRACES_

    try {

        for (Index = 0; Index < Count; Index++) {

            if (Fp + sizeof(ULONG_PTR) >= StackEnd) {
                break;
            }

            NewFp = *((PULONG_PTR)(Fp + 0));
            ReturnAddress = *((PULONG_PTR)(Fp + sizeof(ULONG_PTR)));

            //
            // Figure out if the new frame pointer is ok. This validation
            // should avoid all exceptions in kernel mode because we always
            // read within the current thread's stack and the stack is
            // guaranteed to be in memory (no page faults). It is also guaranteed
            // that we do not take random exceptions in user mode because we always
            // keep the frame pointer within stack limits.
            //

            if (! (Fp < NewFp && NewFp < StackEnd)) {
                break;
            }

            //
            // Figure out if the return address is ok. If return address
            // is a stack address or <64k then something is wrong. There is
            // no reason to return garbage to the caller therefore we stop.
            //

            if (StackStart < ReturnAddress && ReturnAddress < StackEnd) {
                break;
            }

            if (ReturnAddress < 64 * SIZE_1_KB) {
                break;
            }

            //
            // Store new fp and return address and move on.
            //

            Fp = NewFp;
            Callers[Index] = (PVOID)ReturnAddress;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // The frame traversal algorithm is written so that we should
        // not get any exception. Therefore if we get some exception
        // we better debug it.
        //
        // bugbug: enable bkpt only on checked builds
        // After we get some coverage on this we should leave it active
        // only on checked builds.
        //

        DbgPrint ("Unexpected exception in RtlWalkFrameChain ...\n");
        DbgBreakPoint ();
    }

    //
    // Return the number of return addresses identified on the stack.
    //

#if _COLLECT_FRAME_WALK_STATISTICS_
    CollectFrameWalkStatistics (Index);
#endif // #if _COLLECT_FRAME_WALK_STATISTICS_

    return Index;

#else

    return 0;

#endif // #if defined(_X86_)
}


#if _COLLECT_FRAME_WALK_STATISTICS_

KSPIN_LOCK FrameWalkStatisticsLock;
ULONG FrameWalkStatisticsCounters [32];
ULONG FrameWalkCollectStatisticsCalls;
BOOLEAN FrameWalkStatisticsInitialized;

VOID
CollectFrameWalkStatistics (

    ULONG Index)

/*++

Routine description:

    CollectFrameWalkStatistics

Description:

    This function computes the distribution of detectable chain
    lengths. This is used only for debugging the frame traversal
    algorithm. It proves that it is worth trying to get stack
    traces on optimized images because only about 8% of the calls
    cannot be resolved to more than two callers. A sample distribution
    computed by calling RtlWalkFrameChain for every call to
    ExAllocatePoolWithTag gave the results below:

         Length       Percentage
         0-2          5%
         3-5          20%
         6-10         40%
         10-16        35%

    With a failure rate of 5% it is worth using it.

--*/

{
    KIRQL PreviousIrql;
    ULONG I;
    ULONG Percentage;
    ULONG TotalPercentage;

    //
    // Spin lock initialization is not safe in the code below
    // but this code is used only during frame walk algorithm
    // development so there is no reason to make it bulletproof.
    //

    if (! FrameWalkStatisticsInitialized) {
        KeInitializeSpinLock (&FrameWalkStatisticsLock);
        FrameWalkStatisticsInitialized = TRUE;
    }

    KeAcquireSpinLock (
        &FrameWalkStatisticsLock,
        &PreviousIrql);

    FrameWalkCollectStatisticsCalls++;

    if (Index < 32) {
        FrameWalkStatisticsCounters[Index]++;
    }

    if (FrameWalkCollectStatisticsCalls != 0
        && (FrameWalkCollectStatisticsCalls % 60000 == 0)) {

        DbgPrint ("FrameWalk: %u calls \n", FrameWalkCollectStatisticsCalls);

        TotalPercentage = 0;

        for (I = 0; I < 32; I++) {

            Percentage = FrameWalkStatisticsCounters[I] * 100
                / FrameWalkCollectStatisticsCalls;

            DbgPrint ("FrameWalk: [%02u] %02u \n", I, Percentage);

            TotalPercentage += Percentage;
        }

        DbgPrint ("FrameWalk: total %u \n", TotalPercentage);
        DbgBreakPoint ();
    }

    KeReleaseSpinLock (
        &FrameWalkStatisticsLock,
        PreviousIrql);
}

#endif // #if _COLLECT_FRAME_WALK_STATISTICS_

