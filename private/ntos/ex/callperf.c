/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    callperf.c

Abstract:

   This module implements the functions necessary to collect call data.

Author:

    David N. Cutler (davec) 22-May-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

VOID
ExInitializeCallData (
    IN PCALL_PERFORMANCE_DATA CallData
    )

/*++

Routine Description:

    This function initializes a call performance data structure.

Arguments:

    CallData - Supplies a pointer to the call performance data structure
        that is initialized.

Return Value:

    None.

--*/

{

    ULONG Index;

    //
    // Initialize the spinlock and listheads for the call performance
    // data structure.
    //

    KeInitializeSpinLock(&CallData->SpinLock);
    for (Index = 0; Index < CALL_HASH_TABLE_SIZE; Index += 1) {
        InitializeListHead(&CallData->HashTable[Index]);
    }
}

VOID
ExRecordCallerInHashTable (
    IN PCALL_PERFORMANCE_DATA CallData,
    IN PVOID CallersAddress,
    IN PVOID CallersCaller
    )

/*++

Routine Description:

    This function records call data in the specified call performance
    data structure.

Arguments:

    CallData - Supplies a pointer to the call performance data structure
        in which the call data is recorded.

    CallersAddress - Supplies the address of the caller of a fucntion.

    CallersCaller - Supplies the address of the caller of a caller of
        a function.

Return Value:

    None.

--*/

{

    PCALL_HASH_ENTRY HashEntry;
    ULONG Hash;
    PCALL_HASH_ENTRY MatchEntry;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;

    //
    // If the initialization phase is not zero, then collect call performance
    // data.
    //

    if (InitializationPhase != 0) {

        //
        // Acquire the call performance data structure spinlock.
        //

        ExAcquireSpinLock(&CallData->SpinLock, &OldIrql);

        //
        // Lookup the callers address in call performance data hash table. If
        // the address does not exist in the table, then create a new entry.
        //

        Hash = (ULONG)((ULONG_PTR)CallersAddress ^ (ULONG_PTR)CallersCaller);
        Hash = ((Hash > 24) ^ (Hash > 16) ^ (Hash > 8) ^ (Hash)) & (CALL_HASH_TABLE_SIZE - 1);
        MatchEntry = NULL;
        NextEntry = CallData->HashTable[Hash].Flink;
        while (NextEntry != &CallData->HashTable[Hash]) {
            HashEntry = CONTAINING_RECORD(NextEntry,
                                          CALL_HASH_ENTRY,
                                          ListEntry);

            if ((HashEntry->CallersAddress == CallersAddress) &&
                (HashEntry->CallersCaller == CallersCaller)) {
                MatchEntry = HashEntry;
                break;
            }

            NextEntry = NextEntry->Flink;
        }

        //
        // If a matching caller address was found, then update the call site
        // statistics. Otherwise, allocate a new hash entry and initialize
        // call site statistics.
        //

        if (MatchEntry != NULL) {
            MatchEntry->CallCount += 1;

        } else {
            MatchEntry = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(CALL_HASH_ENTRY),
                                              'CdHe');

            if (MatchEntry != NULL) {
                MatchEntry->CallersAddress = CallersAddress;
                MatchEntry->CallersCaller = CallersCaller;
                MatchEntry->CallCount = 1;
                InsertTailList(&CallData->HashTable[Hash],
                               &MatchEntry->ListEntry);
            }
        }

        //
        // Release the call performance data structure spinlock.
        //

        ExReleaseSpinLock(&CallData->SpinLock, OldIrql);
    }

    return;
}
