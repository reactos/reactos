/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    lookasid.c

Abstract:

    This module implements heap lookaside list function.

Author:

    David N. Cutler (davec) 19-Feb-1995

Revision History:

--*/

#include "ntrtlp.h"
#include "heap.h"
#include "heappriv.h"


//
// Define Minimum lookaside list depth.
//

#define MINIMUM_LOOKASIDE_DEPTH 4

//
// Define minimum allocation threshold.
//

#define MINIMUM_ALLOCATION_THRESHOLD 25

//
// Define forward referenced function prototypes.
//



VOID
RtlpInitializeHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside,
    IN USHORT Depth
    )

/*++

Routine Description:

    This function initializes a heap lookaside list structure

Arguments:

    Lookaside - Supplies a pointer to a heap lookaside list structure.

    Allocate - Supplies a pointer to an allocate function.

    Free - Supplies a pointer to a free function.

    HeapHandle - Supplies a pointer to the heap that backs this lookaside list

    Flags - Supplies a set of heap flags.

    Size - Supplies the size for the lookaside list entries.

    Depth - Supplies the maximum depth of the lookaside list.

Return Value:

    None.

--*/

{

    //
    // Initialize the lookaside list structure.
    //

    RtlpInitializeSListHead(&Lookaside->ListHead);

    Lookaside->Depth = MINIMUM_LOOKASIDE_DEPTH;
    Lookaside->MaximumDepth = 256; //Depth;
    Lookaside->TotalAllocates = 0;
    Lookaside->AllocateMisses = 0;
    Lookaside->TotalFrees = 0;
    Lookaside->FreeMisses = 0;

    Lookaside->LastTotalAllocates = 0;
    Lookaside->LastAllocateMisses = 0;

    return;
}


VOID
RtlpDeleteHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside
    )

/*++

Routine Description:

    This function frees any entries specified by the lookaside structure.

Arguments:

    Lookaside - Supplies a pointer to a heap lookaside list structure.

Return Value:

    None.

--*/

{

    PVOID Entry;

    return;
}


VOID
RtlpAdjustHeapLookasideDepth (
    IN PHEAP_LOOKASIDE Lookaside
    )

/*++

Routine Description:

    This function is called periodically to adjust the maximum depth of
    a single heap lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a heap lookaside list structure.

Return Value:

    None.

--*/

{
    ULONG Allocates;
    ULONG Misses;

    //
    // Compute the total number of allocations and misses for this scan
    // period.
    //

    Allocates = Lookaside->TotalAllocates - Lookaside->LastTotalAllocates;
    Lookaside->LastTotalAllocates = Lookaside->TotalAllocates;
    Misses = Lookaside->AllocateMisses - Lookaside->LastAllocateMisses;
    Lookaside->LastAllocateMisses = Lookaside->AllocateMisses;

    //
    // Compute target depth of lookaside list.
    //

    {
        ULONG Ratio;
        ULONG Target;

        //
        // If the allocate rate is less than the mimimum threshold, then lower
        // the maximum depth of the lookaside list. Otherwise, if the miss rate
        // is less than .5%, then lower the maximum depth. Otherwise, raise the
        // maximum depth based on the miss rate.
        //

        if (Misses >= Allocates) {
            Misses = Allocates;
        }

        if (Allocates == 0) {
            Allocates = 1;
        }

        Ratio = (Misses * 1000) / Allocates;
        Target = Lookaside->Depth;
        if (Allocates < MINIMUM_ALLOCATION_THRESHOLD) {
            if (Target > (MINIMUM_LOOKASIDE_DEPTH + 10)) {
                Target -= 10;

            } else {
                Target = MINIMUM_LOOKASIDE_DEPTH;
            }

        } else if (Ratio < 5) {
            if (Target > (MINIMUM_LOOKASIDE_DEPTH + 1)) {
                Target -= 1;

            } else {
                Target = MINIMUM_LOOKASIDE_DEPTH;
            }

        } else {
            Target += ((Ratio * Lookaside->MaximumDepth) / (1000 * 2)) + 5;
            if (Target > Lookaside->MaximumDepth) {
                Target = Lookaside->MaximumDepth;
            }
        }

        Lookaside->Depth = (USHORT)Target;
    }

    return;
}



PVOID
RtlpAllocateFromHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    heap lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->TotalAllocates += 1;

    //
    //  We need to protect ourselves from a second thread that can cause us
    //  to fault on the pop. If we do fault then we'll just do a regular pop
    //  operation
    //

#if defined(_X86_)

    if (USER_SHARED_DATA->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE]) {

#endif // defined(_X86_)

        try {

            Entry = RtlpInterlockedPopEntrySList(&Lookaside->ListHead);

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Entry = NULL;
        }

        if (Entry != NULL) {

            return Entry;
        }
#if defined(_X86_)

    }

#endif // defined(_X86_)

    Lookaside->AllocateMisses += 1;
    return NULL;
}


BOOLEAN
RtlpFreeToHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    BOOLEAN - TRUE if the entry was put on the lookaside list and FALSE
        otherwise.

--*/

{
    Lookaside->TotalFrees += 1;

#if defined(_X86_)

    if (USER_SHARED_DATA->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE]) {

#endif // defined(_X86_)

        if (RtlpQueryDepthSList(&Lookaside->ListHead) < Lookaside->Depth) {

            //
            //  We need to protect ourselves from a second thread that can
            //  cause us to fault on the push.  If we do fault then we'll
            //  just do a regular free operation
            //

            try {

                RtlpInterlockedPushEntrySList(&Lookaside->ListHead,
                                              (PSINGLE_LIST_ENTRY)Entry);

            } except (EXCEPTION_EXECUTE_HANDLER) {

                Lookaside->FreeMisses += 1;

                return FALSE;
            }

            return TRUE;
        }

#if defined(_X86_)

    }

#endif // defined(_X86_)

    Lookaside->FreeMisses += 1;

    return FALSE;
}

