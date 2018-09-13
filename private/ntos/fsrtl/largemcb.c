/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LargeMcb.c

Abstract:

    The MCB routines provide support for maintaining an in-memory copy of
    the retrieval mapping information for a file.  The general idea is to
    have the file system lookup the retrieval mapping for a VBN once from
    the disk, add the mapping to the MCB structure, and then utilize the
    MCB to retrieve the mapping for subsequent accesses to the file.  A
    variable of type MCB is used to store the mapping information.

    The routines provided here allow the user to incrementally store some
    or all of the retrieval mapping for a file and to do so in any order.
    That is, the mapping can be inserted to the MCB structure all at once
    starting from the beginning and working to the end of the file, or it
    can be randomly scattered throughout the file.

    The package identifies each contiguous run of sectors mapping VBNs
    and LBNs indenpendent of the order they are added to the MCB
    structure.  For example a user can define a mapping between VBN
    sector 0 and LBN sector 107, and between VBN sector 2 and LBN sector
    109.  The mapping now contains two runs each one sector in length.
    Now if the user adds an additional mapping between VBN sector 1 and
    LBN sector 106 the MCB structure will contain only one run 3 sectors
    in length.

    Concurrent access to the MCB structure is control by this package.

    The following routines are provided by this package:

      o  FsRtlInitializeMcb - Initialize a new MCB structure.  There
         should be one MCB for every opened file.  Each MCB structure
         must be initialized before it can be used by the system.

      o  FsRtlUninitializeMcb - Uninitialize an MCB structure.  This call
         is used to cleanup any anciallary structures allocated and
         maintained by the MCB.  After being uninitialized the MCB must
         again be initialized before it can be used by the system.

      o  FsRtlAddMcbEntry - This routine adds a new range of mappings
         between LBNs and VBNs to the MCB structure.

      o  FsRtlRemoveMcbEntry - This routines removes an existing range of
         mappings between LBNs and VBNs from the MCB structure.

      o  FsRtlLookupMcbEntry - This routine returns the LBN mapped to by
         a VBN, and indicates, in sectors, the length of the run.

      o  FsRtlLookupLastMcbEntry - This routine returns the mapping for
         the largest VBN stored in the structure.

      o  FsRtlLookupLastMcbEntryAndIndex - This routine returns the mapping
         for the largest VBN stored in the structure as well as its index
         Note that calling LookupLastMcbEntry and NumberOfRunsInMcb cannot
         be synchronized except by the caller.

      o  FsRtlNumberOfRunsInMcb - This routine tells the caller total
         number of discontiguous sectors runs stored in the MCB
         structure.

      o  FsRtlGetNextMcbEntry - This routine returns the the caller the
         starting VBN and LBN of a given run stored in the MCB structure.

Author:

    Gary Kimura     [GaryKi]    5-Feb-1990

Revision History:

--*/

#include "FsRtlP.h"

//
//  Trace level for the module
//

#define Dbg                              (0x80000000)


//
//  Retrieval mapping data structures.  The following two structure together
//  are used to map a Vbn to an Lbn.  It is layed out as follows:
//
//
//  MCB:
//      +----------------+----------------+
//      |    PairCount   |MaximumPairCount|
//      +----------------+----------------+
//      |     Mapping    |    PoolType    |
//      +----------------+----------------+
//
//
//  MAPPING:
//      +----------------+----------------+
//      |       Lbn      |    NextVbn     | : 0
//      +----------------+----------------+
//      |                                 |
//      /                                 /
//      /                                 /
//      |                                 |
//      +----------------+----------------+
//      |       Lbn      |    NextVbn     | : PairCount
//      +----------------+----------------+
//      |                                 |
//      /                                 /
//      /                                 /
//      |                                 |
//      +----------------+----------------+
//      |       Lbn      |    NextVbn     |
//      +----------------+----------------+
//
//                                          : MaximumPairCount
//
//  The pairs from 0 to PairCount - 1 are valid.  Given an index between
//  0 and PairCount - 1 (inclusive) it represents the following Vbn
//  to Lbn mapping information
//
//
//                     { if Index == 0 then 0
//      StartingVbn   {
//                     { if Index <> 0 then NextVbn[i-1]
//
//
//      EndingVbn      = NextVbn[i] - 1
//
//
//      StartingLbn    = Lbn[i]
//
//
//  To compute the mapping of a Vbn to an Lbn the following algorithm
//  is used
//
//      1. search through the pairs until we find the slot "i" that contains
//         the Vbn we after.  Report an error if none if found.
//
//      2. Lbn = StartingLbn + (Vbn - StartingVbn);
//
//  A hole in the allocation (i.e., a sparse allocation) is represented by
//  an Lbn value of -1 (note that is is different than Mcb.c).
//

#define UNUSED_LBN                       (-1)

typedef struct _MAPPING {
    VBN NextVbn;
    LBN Lbn;
} MAPPING;
typedef MAPPING *PMAPPING;

typedef struct _NONOPAQUE_MCB {
    PFAST_MUTEX FastMutex;
    ULONG MaximumPairCount;
    ULONG PairCount;
    POOL_TYPE PoolType;
    PMAPPING Mapping;
} NONOPAQUE_MCB;
typedef NONOPAQUE_MCB *PNONOPAQUE_MCB;

//
//  A macro to return the size, in bytes, of a retrieval mapping structure
//

#define SizeOfMapping(MCB) ((sizeof(MAPPING) * (MCB)->MaximumPairCount))

//
//  The parts of a run can be computed as follows:
//
//
//                StartingVbn(MCB,I)           Mapping[I].NextVbn
//                       |                             |
//                       V                             V
//
//        Run-(I-1)---+ +---------Run-(I)-----------+ +---Run-(I+1)
//
//                       A                         A
//                       |                         |
//                 Mapping[I].Lbn            EndingLbn(MCB,I)
//

#define PreviousEndingVbn(MCB,I) (                      \
    (VBN)((I) == 0 ? 0xffffffff : EndingVbn(MCB,(I)-1)) \
)

#define StartingVbn(MCB,I) (                                \
    (VBN)((I) == 0 ? 0 : (((MCB)->Mapping))[(I)-1].NextVbn) \
)

#define EndingVbn(MCB,I) (                     \
    (VBN)((((MCB)->Mapping)[(I)].NextVbn) - 1) \
)

#define NextStartingVbn(MCB,I) (                                \
    (VBN)((I) >= (MCB)->PairCount ? 0 : StartingVbn(MCB,(I)+1)) \
)




#define PreviousEndingLbn(MCB,I) (                      \
    (LBN)((I) == 0 ? UNUSED_LBN : EndingLbn(MCB,(I)-1)) \
)

#define StartingLbn(MCB,I) (         \
    (LBN)(((MCB)->Mapping)[(I)].Lbn) \
)

#define EndingLbn(MCB,I) (                                       \
    (LBN)(StartingLbn(MCB,I) == UNUSED_LBN ?                     \
          UNUSED_LBN :                                           \
          ((MCB)->Mapping[(I)].Lbn +                             \
           (MCB)->Mapping[(I)].NextVbn - StartingVbn(MCB,I) - 1) \
         )                                                       \
)

#define NextStartingLbn(MCB,I) (                                             \
    (LBN)((I) >= (MCB)->PairCount - 1 ? UNUSED_LBN : StartingLbn(MCB,(I)+1)) \
)

#if 0
LBN
NextStartingLbn(
    PNONOPAQUE_MCB Mcb,
    ULONG I
    )
{
    if ( I >= Mcb->PairCount - 1 ) {
        return (LBN)UNUSED_LBN;
        }
    else {
        return StartingLbn(Mcb,I+1);
        }
}
#endif

#define SectorsWithinRun(MCB,I) (                      \
    (ULONG)(EndingVbn(MCB,I) - StartingVbn(MCB,I) + 1) \
)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('mrSF')

VOID
FsRtlRemoveMcbEntryPrivate (
    IN PNONOPAQUE_MCB OpaqueMcb,
    IN ULONG Vbn,
    IN ULONG SectorCount
    );

//
//  A private routine to search a mapping structure for a Vbn
//

BOOLEAN
FsRtlFindLargeIndex (
    IN PNONOPAQUE_MCB Mcb,
    IN VBN Vbn,
    OUT PULONG Index
    );

VOID
FsRtlAddLargeEntry (
    IN PNONOPAQUE_MCB Mcb,
    IN ULONG WhereToAddIndex,
    IN ULONG AmountToAdd
    );

VOID
FsRtlRemoveLargeEntry (
    IN PNONOPAQUE_MCB Mcb,
    IN ULONG WhereToRemoveIndex,
    IN ULONG AmountToRemove
    );

//
//  Some private routines to handle common allocations.
//

#define FsRtlAllocateFirstMapping() \
    (PVOID)ExAllocateFromPagedLookasideList( &FsRtlFirstMappingLookasideList )

#define FsRtlFreeFirstMapping(Mapping) \
    ExFreeToPagedLookasideList( &FsRtlFirstMappingLookasideList, (Mapping) )

#define FsRtlAllocateFastMutex()      \
    (PFAST_MUTEX)ExAllocateFromNPagedLookasideList( &FsRtlFastMutexLookasideList )

#define FsRtlFreeFastMutex(FastMutex) \
    ExFreeToNPagedLookasideList( &FsRtlFastMutexLookasideList, (FastMutex) )

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlInitializeLargeMcbs)
#pragma alloc_text(PAGE, FsRtlInitializeMcb)
#pragma alloc_text(PAGE, FsRtlUninitializeMcb)
#endif


//
//  Define a small cache of free mapping pairs structures and also the
//  initial size of the mapping pair
//

#define INITIAL_MAXIMUM_PAIR_COUNT       (15)

PAGED_LOOKASIDE_LIST FsRtlFirstMappingLookasideList;

//
//  The following lookaside is used to keep all the Fast Mutexes we will need to
//  boot contiguous.
//

NPAGED_LOOKASIDE_LIST FsRtlFastMutexLookasideList;


//
//  The following few routines define the small mcb package which is
//  implemented behind everyones back as large mcbs.  The only funny
//  thing we really need to do here is to make sure that unused Lbns
//  get returned as 0 and not -1.  This is the result of an historical
//  difference between the original Mcb and LargeMcb packages.
//

VOID
FsRtlInitializeMcb (
    IN PMCB Mcb,
    IN POOL_TYPE PoolType
    )
{
    PAGED_CODE();

    FsRtlInitializeLargeMcb( (PLARGE_MCB)Mcb,
                             PoolType );

    return;
}

VOID
FsRtlUninitializeMcb (
    IN PMCB Mcb
    )

{
    PAGED_CODE();

    FsRtlUninitializeLargeMcb( (PLARGE_MCB)Mcb );

    return;
}

VOID
FsRtlTruncateMcb (
    IN PMCB Mcb,
    IN VBN Vbn
    )
{
   PAGED_CODE();

   FsRtlTruncateLargeMcb( (PLARGE_MCB)Mcb,
                          (LONGLONG)(Vbn) );

   return;
}

BOOLEAN
FsRtlAddMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    IN LBN Lbn,
    IN ULONG SectorCount
    )

{
    PAGED_CODE();

    return FsRtlAddLargeMcbEntry( (PLARGE_MCB)Mcb,
                                  (LONGLONG)(Vbn),
                                  (LONGLONG)(Lbn),
                                  (LONGLONG)(SectorCount) );
}

VOID
FsRtlRemoveMcbEntry (
    IN PMCB OpaqueMcb,
    IN VBN Vbn,
    IN ULONG SectorCount
    )

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlRemoveMcbEntry, Mcb = %08lx\n", Mcb );
    DebugTrace( 0, Dbg, " Vbn         = %08lx\n", Vbn );
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount );

    ExAcquireFastMutex( Mcb->FastMutex );

    try {

        FsRtlRemoveMcbEntryPrivate( Mcb,
                                    Vbn,
                                    SectorCount );

    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlRemoveMcbEntry -> VOID\n", 0 );
    }

    return;
}

BOOLEAN
FsRtlLookupMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL,
    OUT PULONG Index OPTIONAL
    )

{
    BOOLEAN Results;
    LONGLONG LiLbn;
    LONGLONG LiSectorCount;

    Results = FsRtlLookupLargeMcbEntry( (PLARGE_MCB)Mcb,
                                        (LONGLONG)(Vbn),
                                        &LiLbn,
                                        ARGUMENT_PRESENT(SectorCount) ? &LiSectorCount : NULL,
                                        NULL,
                                        NULL,
                                        Index );

    *Lbn = (((ULONG)LiLbn) == -1 ? 0 : ((ULONG)LiLbn));

    if (ARGUMENT_PRESENT(SectorCount)) { *SectorCount = ((ULONG)LiSectorCount); }

    return Results;
}

BOOLEAN
FsRtlLookupLastMcbEntry (
    IN PMCB Mcb,
    OUT PVBN Vbn,
    OUT PLBN Lbn
    )

{
    BOOLEAN Results;
    LONGLONG LiVbn;
    LONGLONG LiLbn;

    PAGED_CODE();

    Results = FsRtlLookupLastLargeMcbEntry( (PLARGE_MCB)Mcb,
                                            &LiVbn,
                                            &LiLbn );

    *Vbn = ((ULONG)LiVbn);
    *Lbn = (((ULONG)LiLbn) == -1 ? 0 : ((ULONG)LiLbn));

    return Results;
}

ULONG
FsRtlNumberOfRunsInMcb (
    IN PMCB Mcb
    )

{
    PAGED_CODE();

    return FsRtlNumberOfRunsInLargeMcb( (PLARGE_MCB)Mcb );
}

BOOLEAN
FsRtlGetNextMcbEntry (
    IN PMCB Mcb,
    IN ULONG RunIndex,
    OUT PVBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount
    )

{
    BOOLEAN Results;
    LONGLONG LiVbn;
    LONGLONG LiLbn;
    LONGLONG LiSectorCount;

    PAGED_CODE();

    Results = FsRtlGetNextLargeMcbEntry( (PLARGE_MCB)Mcb,
                                         RunIndex,
                                         &LiVbn,
                                         &LiLbn,
                                         &LiSectorCount );

    *Vbn = ((ULONG)LiVbn);
    *Lbn = (((ULONG)LiLbn) == -1 ? 0 : ((ULONG)LiLbn));
    *SectorCount = ((ULONG)LiSectorCount);

    return Results;
}


VOID
FsRtlInitializeLargeMcbs (
    VOID
    )

/*++

Routine Description:

    This routine initializes the global portion of the large mcb package
    at system initialization time.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    //  Initialize the lookaside of paged initial mapping arrays.
    //

    ExInitializePagedLookasideList( &FsRtlFirstMappingLookasideList,
                                    NULL,
                                    NULL,
                                    POOL_RAISE_IF_ALLOCATION_FAILURE,
                                    sizeof( MAPPING ) * INITIAL_MAXIMUM_PAIR_COUNT,
                                    'miSF',
                                    4 );

    //
    //  Initialize the Fast Mutex lookaside list.
    //

    ExInitializeNPagedLookasideList( &FsRtlFastMutexLookasideList,
                                     NULL,
                                     NULL,
                                     POOL_RAISE_IF_ALLOCATION_FAILURE,
                                     sizeof( FAST_MUTEX),
                                     'mfSF',
                                     32 );


}


VOID
FsRtlInitializeLargeMcb (
    IN PLARGE_MCB OpaqueMcb,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This routine initializes a new Mcb structure.  The caller must
    supply the memory for the Mcb structure.  This call must precede all
    other calls that set/query the Mcb structure.

    If pool is not available this routine will raise a status value
    indicating insufficient resources.

Arguments:

    OpaqueMcb - Supplies a pointer to the Mcb structure to initialize.

    PoolType - Supplies the pool type to use when allocating additional
        internal Mcb memory.

Return Value:

    None.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    DebugTrace(+1, Dbg, "FsRtlInitializeLargeMcb, Mcb = %08lx\n", Mcb );

    //
    //  Preset the following fields to null so we know to deallocate them
    //  during an abnormal termination
    //

    Mcb->FastMutex = NULL;
    Mcb->Mapping = NULL;

    try {

        //
        //  Initialize the fields in the Mcb
        //

        Mcb->FastMutex = FsRtlAllocateFastMutex();

        ExInitializeFastMutex( Mcb->FastMutex );

        Mcb->PairCount = 0;
        Mcb->PoolType = PoolType;

        //
        //  Allocate a new buffer an initial size is one that will hold
        //  16 runs
        //

        if (PoolType == PagedPool) {

            Mcb->Mapping = FsRtlAllocateFirstMapping();

        } else {

            Mcb->Mapping = FsRtlpAllocatePool( Mcb->PoolType, sizeof(MAPPING) * INITIAL_MAXIMUM_PAIR_COUNT );
        }

        //**** RtlZeroMemory( Mcb->Mapping, sizeof(MAPPING) * INITIAL_MAXIMUM_PAIR_COUNT );

        Mcb->MaximumPairCount = INITIAL_MAXIMUM_PAIR_COUNT;

    } finally {

        //
        //  If this is an abnormal termination then we need to deallocate
        //  the FastMutex and/or mapping (but once the mapping is allocated,
        //  we can't raise).
        //

        if (AbnormalTermination()) {

            if (Mcb->FastMutex != NULL) { FsRtlFreeFastMutex( Mcb->FastMutex ); }
        }

        DebugTrace(-1, Dbg, "FsRtlInitializeLargeMcb -> VOID\n", 0 );
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
FsRtlUninitializeLargeMcb (
    IN PLARGE_MCB OpaqueMcb
    )

/*++

Routine Description:

    This routine uninitializes an Mcb structure.  After calling this routine
    the input Mcb structure must be re-initialized before being used again.

Arguments:

    OpaqueMcb - Supplies a pointer to the Mcb structure to uninitialize.

Return Value:

    None.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    DebugTrace(+1, Dbg, "FsRtlUninitializeLargeMcb, Mcb = %08lx\n", Mcb );

    //
    //  Protect against some user calling us to uninitialize an mcb twice
    //

    if (Mcb->FastMutex == NULL) {

        // ASSERTMSG("Being called to uninitialize an Mcb that is already Uninitialized ", FALSE);

        return;
    }

    //
    //  Deallocate the FastMutex and mapping buffer
    //

    FsRtlFreeFastMutex( Mcb->FastMutex );

    Mcb->FastMutex = NULL;

    if ((Mcb->PoolType == PagedPool) && (Mcb->MaximumPairCount == INITIAL_MAXIMUM_PAIR_COUNT)) {

        FsRtlFreeFirstMapping( Mcb->Mapping );

    } else {

        ExFreePool( Mcb->Mapping );
    }

    //
    //  Now zero our all of the fields in the Mcb
    //

    //**** Mcb->MaximumPairCount = 0;
    //**** Mcb->PairCount = 0;
    //**** Mcb->Mapping = NULL;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlUninitializeLargeMcb -> VOID\n", 0 );

    return;
}


VOID
FsRtlTruncateLargeMcb (
    IN PLARGE_MCB OpaqueMcb,
    IN LONGLONG LargeVbn
    )

/*++

Routine Description:

    This routine truncates an Mcb structure to the specified Vbn.
    After calling this routine the Mcb will only contain mappings
    up to and not including the input vbn.

Arguments:

    OpaqueMcb - Supplies a pointer to the Mcb structure to truncate.

    LargeVbn - Specifies the last Vbn at which is no longer to be
      mapped.

Return Value:

    None.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    VBN Vbn = ((ULONG)LargeVbn);
    ULONG Index;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlTruncateLargeMcb, Mcb = %08lx\n", Mcb );

    ExAcquireFastMutex( Mcb->FastMutex );

    ASSERTMSG("LargeInteger not supported yet ", ((((PLARGE_INTEGER)&LargeVbn)->HighPart == 0) ||
                                                  (Mcb->PairCount == 0) ||
                                                  ((((PLARGE_INTEGER)&LargeVbn)->HighPart == 0x7FFFFFFF) &&
                                                   (((ULONG)LargeVbn) == 0xFFFFFFFF))));

    try {

        //
        //  Do a quick test to see if we are truncating the entire Mcb.
        //

        if (Vbn == 0) {

            Mcb->PairCount = 0;

        } else if (Mcb->PairCount > 0) {

            //
            //  Find the index for the entry with the last Vcn we want to keep.
            //  There is nothing to do if the Mcb already ends prior to
            //  this point.
            //

            if (FsRtlFindLargeIndex(Mcb, Vbn - 1, &Index)) {

                //
                //  If this entry currently describes a hole then
                //  truncate to the previous entry.
                //

                if (StartingLbn(Mcb, Index) == UNUSED_LBN) {

                    Mcb->PairCount = Index;

                //
                //  Otherwise we will truncate the Mcb to this point.  Truncate
                //  the number of Vbns of this run if necessary.
                //

                } else {

                    Mcb->PairCount = Index + 1;

                    if (NextStartingVbn(Mcb, Index) > Vbn) {

                        (Mcb->Mapping)[Index].NextVbn = Vbn;
                    }
                }
            }
        }

        //
        //  Now see if we can shrink the allocation for the mapping pairs.
        //  We'll shrink the mapping pair buffer if the new pair count will
        //  fit within a quarter of the current maximum pair count and the
        //  current maximum is greater than the initial pair count.
        //

        if ((Mcb->PairCount < (Mcb->MaximumPairCount / 4)) &&
            (Mcb->MaximumPairCount > INITIAL_MAXIMUM_PAIR_COUNT)) {

            ULONG NewMax;
            PMAPPING Mapping;

            //
            //  We need to allocate a new mapping so compute a new maximum pair
            //  count.  We'll allocate double the current pair count, but never
            //  less than the initial pair count.
            //

            NewMax = Mcb->PairCount * 2;

            if (NewMax < INITIAL_MAXIMUM_PAIR_COUNT) {
                NewMax = INITIAL_MAXIMUM_PAIR_COUNT;
            }

            //
            //  Be careful to trap failures due to resource exhaustion.
            //
                
            try {
                    
                if (NewMax == INITIAL_MAXIMUM_PAIR_COUNT && Mcb->PoolType == PagedPool) {

                    Mapping = FsRtlAllocateFirstMapping();

                } else {
            
                    Mapping = FsRtlpAllocatePool( Mcb->PoolType, sizeof(MAPPING) * NewMax );
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {

                  Mapping = NULL;
            }

            //
            //  Now check if we really got a new buffer
            //

            if (Mapping != NULL) {

                //
                //  Now copy over the old mapping to the new buffer
                //

                RtlCopyMemory( Mapping, Mcb->Mapping, sizeof(MAPPING) * Mcb->PairCount );

                //
                //  Deallocate the old buffer.  This should never be the size of an
                //  initial mapping ...
                //

                ExFreePool( Mcb->Mapping );

                //
                //  And set up the new buffer in the Mcb
                //

                Mcb->Mapping = Mapping;
                Mcb->MaximumPairCount = NewMax;
            }
        }

    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlTruncateLargeMcb -> VOID\n", 0 );

    return;
}


NTKERNELAPI
VOID
FsRtlResetLargeMcb (
    IN PLARGE_MCB OpaqueMcb,
    IN BOOLEAN SelfSynchronized
    )

/*++

Routine Description:

    This routine truncates an Mcb structure to contain zero mapping
    pairs.  It does not shrink the mapping pairs array.

Arguments:

    OpaqueMcb - Supplies a pointer to the Mcb structure to truncate.

    SelfSynchronized - Indicates whether the caller is already synchronized
        with respect to the Mcb.

Return Value:

    None.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    if (SelfSynchronized) {
        
        //
        //  If we are self-synchronized, then all we do is clear out the 
        //  current mapping pair count.
        //
        
        Mcb->PairCount = 0;
    
    } else {
        
        //
        //  Since we are not self-synchronized, we must serialize access to
        //  the Mcb before clearing the pair count
        //
        
        ExAcquireFastMutex( Mcb->FastMutex );
        Mcb->PairCount = 0;
        ExReleaseFastMutex( Mcb->FastMutex );
    
    }

    return;
}


BOOLEAN
FsRtlAddLargeMcbEntry (
    IN PLARGE_MCB OpaqueMcb,
    IN LONGLONG LargeVbn,
    IN LONGLONG LargeLbn,
    IN LONGLONG LargeSectorCount
    )

/*++

Routine Description:

    This routine is used to add a new mapping of VBNs to LBNs to an existing
    Mcb. The information added will map

        Vbn to Lbn,

        Vbn+1 to Lbn+1,...

        Vbn+(SectorCount-1) to Lbn+(SectorCount-1).

    The mapping for the VBNs must not already exist in the Mcb.  If the
    mapping continues a previous run, then this routine will actually coalesce
    them into 1 run.

    If pool is not available to store the information this routine will raise a
    status value indicating insufficient resources.

    An input Lbn value of zero is illegal (i.e., the Mcb structure will never
    map a Vbn to a zero Lbn value).

Arguments:

    OpaqueMcb - Supplies the Mcb in which to add the new mapping.

    Vbn - Supplies the starting Vbn of the new mapping run to add to the Mcb.

    Lbn - Supplies the starting Lbn of the new mapping run to add to the Mcb.

    SectorCount - Supplies the size of the new mapping run (in sectors).

Return Value:

    BOOLEAN - TRUE if the mapping was added successfully (i.e., the new
        Vbns did not collide with existing Vbns), and FALSE otherwise.  If
        FALSE is returned then the Mcb is not changed.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    VBN Vbn = ((ULONG)LargeVbn);
    LBN Lbn = ((ULONG)LargeLbn);
    ULONG SectorCount = ((ULONG)LargeSectorCount);

    ULONG Index;

    VBN LastVbn;

    BOOLEAN Result;

    ASSERTMSG("LargeInteger not supported yet ", ((PLARGE_INTEGER)&LargeVbn)->HighPart == 0);
    ASSERTMSG("LargeInteger not supported yet ", ((PLARGE_INTEGER)&LargeLbn)->HighPart == 0);
    ASSERTMSG("LargeInteger not supported yet ", ((PLARGE_INTEGER)&LargeSectorCount)->HighPart == 0);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlAddLargeMcbEntry, Mcb = %08lx\n", Mcb );
    DebugTrace( 0, Dbg, " Vbn         = %08lx\n", Vbn );
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn );
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount );

    ExAcquireFastMutex( Mcb->FastMutex );

    try {

        if (FsRtlFindLargeIndex(Mcb, Vbn, &Index)) {

            ULONG EndVbn = Vbn + SectorCount - 1;
            ULONG EndIndex;

            //
            //  First check the case where we are adding to an existing mcb run
            //  and if so then we will modify the insertion to complete the run
            //
            //      --ExistingRun--|      ==becomes==>  --ExistingRun--|
            //              |--NewRun--|                               |---|
            //
            //      --ExistingRun----|    ==becomes==> a noop
            //          |--NewRun--|
            //

            if (StartingLbn(Mcb, Index) != UNUSED_LBN) {

                //
                //  Check that the Lbn's line up between the new and existing run
                //

                if (Lbn != (StartingLbn(Mcb, Index) + (Vbn - StartingVbn(Mcb, Index)))) {

                    //
                    //  Let our caller know we couldn't insert the run.
                    //

                    try_return(Result = FALSE);
                }

                //
                //  Check if the new run is contained in the existing run
                //

                if (EndVbn <= EndingVbn(Mcb, Index)) {

                    //
                    //  Do nothing because the run is contained within the existing run
                    //

                    try_return(Result = TRUE);
                }

                //
                //  Otherwise we will simply trim off the request for the new run
                //  to not overlap with the existing run
                //

                Vbn = NextStartingVbn(Mcb, Index);
                Lbn = EndingLbn(Mcb, Index) + 1;

                ASSERT(EndVbn >= Vbn);

                SectorCount = EndVbn - Vbn + 1;

            //
            //  At this point the new run start in a hole, now check that if
            //  crosses into a non hole and if so then adjust new run to fit
            //  in the hole
            //
            //
            //            |--ExistingRun--  ==becomes==>        |--ExistingRun--
            //      |--NewRun--|                          |--New|
            //

            } else if (FsRtlFindLargeIndex(Mcb, EndVbn, &EndIndex) && (Index == (EndIndex-1))) {

                //
                //  Check that the Lbn's line up in the overlap
                //

                if (StartingLbn(Mcb, EndIndex) != Lbn + (StartingVbn(Mcb, EndIndex) - Vbn)) {

                    //
                    //  Let our caller know we couldn't insert the run.
                    //

                    try_return(Result = FALSE);
                }

                //
                //  Truncate the sector count to go up to but not include
                //  the existing run
                //

                SectorCount = StartingVbn(Mcb, EndIndex) - Vbn;
            }
        }

        //
        //  Find the index for the starting Vbn of our new run, if there isn't
        //  a hole found then index will be set to paircount.
        //

        if (((Index = Mcb->PairCount) == 0) ||
            (PreviousEndingVbn(Mcb,Index)+1 <= Vbn) ||
            !FsRtlFindLargeIndex(Mcb, Vbn, &Index)) {

            //
            //  We didn't find a mapping, therefore this new mapping must
            //  go on at the end of the current mapping.
            //
            //  See if we can just grow the last mapping in the current mcb.
            //  We can grow the last entry if (1) the Vbns follow on, and (2)
            //  the Lbns follow on.  We can only grow the last mapping if the
            //  index is not 0.
            //

            if ((Index != 0) &&
                (PreviousEndingVbn(Mcb,Index) + 1 == Vbn) &&
                (PreviousEndingLbn(Mcb,Index) + 1 == Lbn)) {

                //
                //      --LastRun--|---NewRun--|
                //

                //
                //  Extend the last run in the mcb
                //

                DebugTrace( 0, Dbg, "Continuing last run\n", 0);

                (Mcb->Mapping)[Mcb->PairCount-1].NextVbn += SectorCount;

                try_return (Result = TRUE);
            }

            //
            //  We couldn't grow the last mapping, now check to see if
            //  this is a continuation of the last Vbn (i.e., there isn't
            //  going to be a hole in the mapping).  Or if this is the first
            //  run in the mapping
            //

            if ((Vbn == 0) ||
                (PreviousEndingVbn(Mcb,Index) + 1 == Vbn)) {

                //
                //      --LastRun--||---NewRun--|
                //
                //      0:|--NewRun--|
                //

                //
                //  We only need to add one more run to the mcb, so make sure
                //  there is enough room for one.
                //

                DebugTrace( 0, Dbg, "Adding new contiguous last run\n", 0);

                FsRtlAddLargeEntry( Mcb, Index, 1 );

                //
                //  Add the new mapping
                //

                (Mcb->Mapping)[Index].Lbn = Lbn;
                (Mcb->Mapping)[Index].NextVbn = Vbn + SectorCount;

                try_return (Result = TRUE);
            }

            //
            //  If we reach this point then there is going to be a hole in the
            //  mapping. and the mapping gets appended to the end of the current
            //  allocation.  So need to make room for two more runs in the mcb.
            //

            //
            //      --LastRun--|   hole   |---NewRun--|
            //
            //      0:  hole  |--NewRun--|
            //

            DebugTrace( 0, Dbg, "Adding new noncontiguous last run\n", 0);

            FsRtlAddLargeEntry( Mcb, Index, 2 );

            //
            //  Add the hole
            //

            (Mcb->Mapping)[Index].Lbn = (LBN)UNUSED_LBN;
            (Mcb->Mapping)[Index].NextVbn = Vbn;

            //
            //  Add the new mapping
            //

            (Mcb->Mapping)[Index+1].Lbn = Lbn;
            (Mcb->Mapping)[Index+1].NextVbn = Vbn + SectorCount;

            try_return (Result = TRUE);
        }

        //
        //  We found an index for the Vbn therefore we must be trying
        //  to fill up a hole in the mcb.  So first we need to check to make
        //  sure there really is a hole to be filled
        //

        LastVbn = Vbn + SectorCount - 1;

        if ((StartingLbn(Mcb,Index) == UNUSED_LBN) &&
            (StartingVbn(Mcb,Index) <= Vbn) && (LastVbn <= EndingVbn(Mcb,Index))) {

            //
            //  The mapping fits in this hole, but now here are the following
            //  cases we must consider for the new mapping
            //

            if ((StartingVbn(Mcb,Index) < Vbn) && (LastVbn < EndingVbn(Mcb,Index))) {

                //  Leaves a hole are both ends
                //
                //  --PreviousRun--|  hole  |--NewRun--|  hole  |--FollowingRun--
                //
                //  0:  hole  |--NewRun--|  hole  |--FollowingRun--
                //

                DebugTrace( 0, Dbg, "Hole at both ends\n", 0);

                //
                //  Make room for two more entries.  The NextVbn field of the
                //  one we're shifting remains valid.
                //

                FsRtlAddLargeEntry( Mcb, Index, 2 );

                //
                //  Add the first hole
                //

                (Mcb->Mapping)[Index].Lbn = (LBN)UNUSED_LBN;
                (Mcb->Mapping)[Index].NextVbn = Vbn;

                //
                //  Add the new mapping
                //

                (Mcb->Mapping)[Index+1].Lbn = Lbn;
                (Mcb->Mapping)[Index+1].NextVbn = Vbn + SectorCount;

                //
                //  The second hole is already set up by the add entry call, because
                //  that call just shift over the original hole to that slot
                //

                try_return (Result = TRUE);
            }

            if ((StartingVbn(Mcb,Index) == Vbn) && (LastVbn < EndingVbn(Mcb,Index))) {

                if (PreviousEndingLbn(Mcb,Index) + 1 == Lbn) {

                    //
                    //  Leaves a hole at the rear, and continues the earlier run
                    //
                    //  --PreviousRun--|--NewRun--|  hole  |--FollowingRun--
                    //

                    DebugTrace( 0, Dbg, "Hole at rear and continue\n", 0);

                    //
                    //  We just need to extend the previous run
                    //

                    (Mcb->Mapping)[Index-1].NextVbn += SectorCount;

                    try_return (Result = TRUE);

                } else {

                    //
                    //  Leaves a hole at the rear, and does not continue the
                    //  earlier run.  As occurs if index is zero.
                    //
                    //  --PreviousRun--||--NewRun--|  hole  |--FollowingRun--
                    //
                    //  0:|--NewRun--|  hole  |--FollowingRun--
                    //

                    DebugTrace( 0, Dbg, "Hole at rear and not continue\n", 0);

                    //
                    //  Make room for one more entry.  The NextVbn field of the
                    //  one we're shifting remains valid.
                    //

                    FsRtlAddLargeEntry( Mcb, Index, 1 );

                    //
                    //  Add the new mapping
                    //

                    (Mcb->Mapping)[Index].Lbn = Lbn;
                    (Mcb->Mapping)[Index].NextVbn = Vbn + SectorCount;

                    //
                    //  The hole is already set up by the add entry call, because
                    //  that call just shift over the original hole to that slot
                    //

                    try_return (Result = TRUE);
                }
            }

            if ((StartingVbn(Mcb,Index) < Vbn) && (LastVbn == EndingVbn(Mcb,Index))) {

                if (NextStartingLbn(Mcb,Index) == Lbn + SectorCount) {

                    //
                    //  Leaves a hole at the front, and continues the following run
                    //
                    //  --PreviousRun--|  hole  |--NewRun--|--FollowingRun--
                    //
                    //  0:  hole  |--NewRun--|--FollowingRun--
                    //

                    DebugTrace( 0, Dbg, "Hole at front and continue\n", 0);

                    //
                    //  We just need to extend the following run
                    //

                    (Mcb->Mapping)[Index].NextVbn = Vbn;
                    (Mcb->Mapping)[Index+1].Lbn = Lbn;

                    try_return (Result = TRUE);

                } else {

                    //
                    //  Leaves a hole at the front, and does not continue the following
                    //  run
                    //
                    //  --PreviousRun--|  hole  |--NewRun--||--FollowingRun--
                    //
                    //  0:  hole  |--NewRun--||--FollowingRun--
                    //

                    DebugTrace( 0, Dbg, "Hole at front and not continue\n", 0);

                    //
                    //  Make room for one more entry.  The NextVbn field of the
                    //  one we're shifting remains valid.
                    //

                    FsRtlAddLargeEntry( Mcb, Index, 1 );

                    //
                    //  Add the hole
                    //

                    (Mcb->Mapping)[Index].Lbn = (LBN)UNUSED_LBN;
                    (Mcb->Mapping)[Index].NextVbn = Vbn;

                    //
                    //  Add the new mapping
                    //

                    (Mcb->Mapping)[Index+1].Lbn = Lbn;

                    try_return (Result = TRUE);
                }

            }

            if ((PreviousEndingLbn(Mcb,Index) + 1 == Lbn) &&
                (NextStartingLbn(Mcb,Index) == Lbn + SectorCount)) {

                //
                //  Leaves no holes, and continues both runs
                //
                //  --PreviousRun--|--NewRun--|--FollowingRun--
                //

                DebugTrace( 0, Dbg, "No holes, and continues both runs\n", 0);

                //
                //  We need to collapse the current index and the following index
                //  but first we copy the NextVbn of the follwing run into
                //  the NextVbn field of the previous run to so it all becomes
                //  one run
                //

                (Mcb->Mapping)[Index-1].NextVbn = (Mcb->Mapping)[Index+1].NextVbn;

                FsRtlRemoveLargeEntry( Mcb, Index, 2 );

                try_return (Result = TRUE);
            }

            if (NextStartingLbn(Mcb,Index) == Lbn + SectorCount) {

                //
                //  Leaves no holes, and continues only following run
                //
                //  --PreviousRun--||--NewRun--|--FollowingRun--
                //
                //  0:|--NewRun--|--FollowingRun--
                //

                DebugTrace( 0, Dbg, "No holes, and continues following\n", 0);

                //
                //  This index is going away so we need to stretch the
                //  following run to meet up with the previous run
                //

                (Mcb->Mapping)[Index+1].Lbn = Lbn;

                FsRtlRemoveLargeEntry( Mcb, Index, 1 );

                try_return (Result = TRUE);
            }

            if (PreviousEndingLbn(Mcb,Index) + 1 == Lbn) {

                //
                //  Leaves no holes, and continues only earlier run
                //
                //  --PreviousRun--|--NewRun--||--FollowingRun--
                //

                DebugTrace( 0, Dbg, "No holes, and continues earlier\n", 0);

                //
                //  This index is going away so we need to stretch the
                //  previous run to meet up with the following run
                //

                (Mcb->Mapping)[Index-1].NextVbn = (Mcb->Mapping)[Index].NextVbn;

                FsRtlRemoveLargeEntry( Mcb, Index, 1 );

                try_return (Result = TRUE);
            }

            //
            //  Leaves no holes, and continues neither run
            //
            //      --PreviousRun--||--NewRun--||--FollowingRun--
            //
            //      0:|--NewRun--||--FollowingRun--
            //

            DebugTrace( 0, Dbg, "No holes, and continues none\n", 0);

            (Mcb->Mapping)[Index].Lbn = Lbn;

            try_return (Result = TRUE);
        }

        //
        //  We tried to overwrite an existing mapping so we'll have to
        //  tell our caller that it's not possible
        //

        Result = FALSE;

    try_exit: NOTHING;
    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlAddLargeMcbEntry -> %08lx\n", Result );
    }

    return Result;
}


VOID
FsRtlRemoveLargeMcbEntry (
    IN PLARGE_MCB OpaqueMcb,
    IN LONGLONG LargeVbn,
    IN LONGLONG LargeSectorCount
    )

/*++

Routine Description:

    This routine removes a mapping of VBNs to LBNs from an Mcb.  The mappings
    removed are for

        Vbn,

        Vbn+1, to

        Vbn+(SectorCount-1).

    The operation works even if the mapping for a Vbn in the specified range
    does not already exist in the Mcb.  If the specified range of Vbn includes
    the last mapped Vbn in the Mcb then the Mcb mapping shrinks accordingly.

    If pool is not available to store the information this routine will raise
    a status value indicating insufficient resources.

Arguments:

    OpaqueMcb - Supplies the Mcb from which to remove the mapping.

    Vbn - Supplies the starting Vbn of the mappings to remove.

    SectorCount - Supplies the size of the mappings to remove (in sectors).

Return Value:

    None.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    VBN Vbn = ((ULONG)LargeVbn);
    ULONG SectorCount = ((ULONG)LargeSectorCount);

    PAGED_CODE();

    ASSERTMSG("LargeInteger not supported yet ", ((PLARGE_INTEGER)&LargeVbn)->HighPart == 0);

    DebugTrace(+1, Dbg, "FsRtlRemoveLargeMcbEntry, Mcb = %08lx\n", Mcb );
    DebugTrace( 0, Dbg, " Vbn         = %08lx\n", Vbn );
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount );

    ExAcquireFastMutex( Mcb->FastMutex );

    try {

        FsRtlRemoveMcbEntryPrivate( Mcb, Vbn, SectorCount );

    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlRemoveLargeMcbEntry -> VOID\n", 0 );
    }

    return;
}


BOOLEAN
FsRtlLookupLargeMcbEntry (
    IN PLARGE_MCB OpaqueMcb,
    IN LONGLONG LargeVbn,
    OUT PLONGLONG LargeLbn OPTIONAL,
    OUT PLONGLONG LargeSectorCount OPTIONAL,
    OUT PLONGLONG LargeStartingLbn OPTIONAL,
    OUT PLONGLONG LargeCountFromStartingLbn OPTIONAL,
    OUT PULONG Index OPTIONAL
    )

/*++

Routine Description:

    This routine retrieves the mapping of a Vbn to an Lbn from an Mcb.
    It indicates if the mapping exists and the size of the run.

Arguments:

    OpaqueMcb - Supplies the Mcb being examined.

    Vbn - Supplies the Vbn to lookup.

    Lbn - Receives the Lbn corresponding to the Vbn.  A value of -1 is
        returned if the Vbn does not have a corresponding Lbn.

    SectorCount - Receives the number of sectors that map from the Vbn to
        contiguous Lbn values beginning with the input Vbn.

    Index - Receives the index of the run found.

Return Value:

    BOOLEAN - TRUE if the Vbn is within the range of VBNs mapped by the
        MCB (even if it corresponds to a hole in the mapping), and FALSE
        if the Vbn is beyond the range of the MCB's mapping.

        For example, if an MCB has a mapping for VBNs 5 and 7 but not for
        6, then a lookup on Vbn 5 or 7 will yield a non zero Lbn and a sector
        count of 1.  A lookup for Vbn 6 will return TRUE with an Lbn value of
        0, and lookup for Vbn 8 or above will return FALSE.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    BOOLEAN Result;

    ULONG LocalIndex;

    DebugTrace(+1, Dbg, "FsRtlLookupLargeMcbEntry, Mcb = %08lx\n", Mcb );
    DebugTrace( 0, Dbg, "  LargeVbn.LowPart = %08lx\n", LargeVbn.LowPart );

    ExAcquireFastMutex( Mcb->FastMutex );

    ASSERTMSG("LargeInteger not supported yet ", ((((PLARGE_INTEGER)&LargeVbn)->HighPart == 0) ||
                                                  (Mcb->PairCount == 0) ||
                                                  ((((PLARGE_INTEGER)&LargeVbn)->HighPart == 0x7FFFFFFF) &&
                                                   (((ULONG)LargeVbn) == 0xFFFFFFFF))));

    try {

        if (!FsRtlFindLargeIndex(Mcb, ((ULONG)LargeVbn), &LocalIndex)) {

            try_return (Result = FALSE);
        }

        //
        //  Compute the lbn for corresponding to the vbn, the value is the
        //  starting lbn of the run plus the number of sectors offset into the
        //  run.  But if it's a hole then the sector Lbn is zero.
        //

        if (ARGUMENT_PRESENT(LargeLbn)) {

            if (StartingLbn(Mcb,LocalIndex) == UNUSED_LBN) {

                *((PULONG)LargeLbn) = (LBN)UNUSED_LBN;

            } else {

                *((PULONG)LargeLbn) = StartingLbn(Mcb,LocalIndex) + (((ULONG)LargeVbn) - StartingVbn(Mcb,LocalIndex));
            }
        }

        //
        //  If there sector count argument is present then we'll return the number
        //  of sectors remaing in the run.
        //

        if (ARGUMENT_PRESENT(LargeSectorCount)) {

            *((PULONG)LargeSectorCount) = EndingVbn(Mcb,LocalIndex) - ((ULONG)LargeVbn) + 1;
        }

        //
        //  Compute the starting lbn for corresponding to the start of the run, the value is the
        //  starting lbn of the run.  But if it's a hole then the sector Lbn is zero.
        //

        if (ARGUMENT_PRESENT(LargeStartingLbn)) {

            if (StartingLbn(Mcb,LocalIndex) == UNUSED_LBN) {

                *((PULONG)LargeStartingLbn) = (LBN)UNUSED_LBN;

            } else {

                *((PULONG)LargeStartingLbn) = StartingLbn(Mcb,LocalIndex);
            }
        }

        //
        //  If there sector count argument is present then we'll return the number
        //  of sectors in the run.
        //

        if (ARGUMENT_PRESENT(LargeCountFromStartingLbn)) {

            *((PULONG)LargeCountFromStartingLbn) = EndingVbn(Mcb,LocalIndex) - StartingVbn(Mcb,LocalIndex) + 1;
        }

        //
        //  If the caller want to know the Index number, fill it in.
        //

        if (ARGUMENT_PRESENT(Index)) {

            *Index = LocalIndex;
        }

        Result = TRUE;

    try_exit: NOTHING;
    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlLookupLargeMcbEntry -> %08lx\n", Result );
    }

    if (ARGUMENT_PRESENT(LargeLbn)) {
        ((PLARGE_INTEGER)LargeLbn)->HighPart = (*((PULONG)LargeLbn) == UNUSED_LBN ? UNUSED_LBN : 0);
    }

    if (ARGUMENT_PRESENT(LargeSectorCount)) {
        ((PLARGE_INTEGER)LargeSectorCount)->HighPart = 0;
    }

    if (ARGUMENT_PRESENT(LargeStartingLbn)) {
        ((PLARGE_INTEGER)LargeStartingLbn)->HighPart = (*((PULONG)LargeStartingLbn) == UNUSED_LBN ? UNUSED_LBN : 0);
    }

    if (ARGUMENT_PRESENT(LargeCountFromStartingLbn)) {
        ((PLARGE_INTEGER)LargeCountFromStartingLbn)->HighPart = 0;
    }

    return Result;
}


BOOLEAN
FsRtlLookupLastLargeMcbEntry (
    IN PLARGE_MCB OpaqueMcb,
    OUT PLONGLONG LargeVbn,
    OUT PLONGLONG LargeLbn
    )

/*++

Routine Description:

    This routine retrieves the last Vbn to Lbn mapping stored in the Mcb.
    It returns the mapping for the last sector or the last run in the
    Mcb.  The results of this function is useful when extending an existing
    file and needing to a hint on where to try and allocate sectors on the
    disk.

Arguments:

    OpaqueMcb - Supplies the Mcb being examined.

    Vbn - Receives the last Vbn value mapped.

    Lbn - Receives the Lbn corresponding to the Vbn.

Return Value:

    BOOLEAN - TRUE if there is a mapping within the Mcb and FALSE otherwise
        (i.e., the Mcb does not contain any mapping).

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlLookupLastLargeMcbEntry, Mcb = %08lx\n", Mcb );

    ExAcquireFastMutex( Mcb->FastMutex );

    try {

        //
        //  Check to make sure there is at least one run in the mcb
        //

        if (Mcb->PairCount <= 0) {

            try_return (Result = FALSE);
        }

        //
        //  Return the last mapping of the last run
        //

        *((PULONG)LargeLbn) = EndingLbn(Mcb,Mcb->PairCount-1);
        *((PULONG)LargeVbn) = EndingVbn(Mcb,Mcb->PairCount-1);

        Result = TRUE;

    try_exit: NOTHING;
    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlLookupLastLargeMcbEntry -> %08lx\n", Result );
    }

    ((PLARGE_INTEGER)LargeVbn)->HighPart = (*((PULONG)LargeVbn) == UNUSED_LBN ? UNUSED_LBN : 0);
    ((PLARGE_INTEGER)LargeLbn)->HighPart = (*((PULONG)LargeLbn) == UNUSED_LBN ? UNUSED_LBN : 0);

    return Result;
}


BOOLEAN
FsRtlLookupLastLargeMcbEntryAndIndex (
    IN PLARGE_MCB OpaqueMcb,
    OUT PLONGLONG LargeVbn,
    OUT PLONGLONG LargeLbn,
    OUT PULONG Index
    )

/*++

Routine Description:

    This routine retrieves the last Vbn to Lbn mapping stored in the Mcb.
    It returns the mapping for the last sector or the last run in the
    Mcb.  The results of this function is useful when extending an existing
    file and needing to a hint on where to try and allocate sectors on the
    disk.

Arguments:

    OpaqueMcb - Supplies the Mcb being examined.

    Vbn - Receives the last Vbn value mapped.

    Lbn - Receives the Lbn corresponding to the Vbn.
    
    Index - Receives the index of the last run.

Return Value:

    BOOLEAN - TRUE if there is a mapping within the Mcb and FALSE otherwise
        (i.e., the Mcb does not contain any mapping).

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlLookupLastLargeMcbEntryAndIndex, Mcb = %08lx\n", Mcb );

    ExAcquireFastMutex( Mcb->FastMutex );

    try {

        //
        //  Check to make sure there is at least one run in the mcb
        //

        if (Mcb->PairCount <= 0) {

            try_return (Result = FALSE);
        }

        //
        //  Return the last mapping of the last run
        //

        *((PULONG)LargeLbn) = EndingLbn(Mcb,Mcb->PairCount-1);
        *((PULONG)LargeVbn) = EndingVbn(Mcb,Mcb->PairCount-1);

        *Index = Mcb->PairCount - 1;

        Result = TRUE;

    try_exit: NOTHING;
    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlLookupLastLargeMcbEntryAndIndex -> %08lx\n", Result );
    }

    ((PLARGE_INTEGER)LargeVbn)->HighPart = (*((PULONG)LargeVbn) == UNUSED_LBN ? UNUSED_LBN : 0);
    ((PLARGE_INTEGER)LargeLbn)->HighPart = (*((PULONG)LargeLbn) == UNUSED_LBN ? UNUSED_LBN : 0);

    return Result;
}


ULONG
FsRtlNumberOfRunsInLargeMcb (
    IN PLARGE_MCB OpaqueMcb
    )

/*++

Routine Description:

    This routine returns to the its caller the number of distinct runs
    mapped by an Mcb.  Holes (i.e., Vbns that map to Lbn=UNUSED_LBN) are counted
    as runs.  For example, an Mcb containing a mapping for only Vbns 0 and 3
    will have 3 runs, one for the first mapped sector, a second for the
    hole covering Vbns 1 and 2, and a third for Vbn 3.

Arguments:

    OpaqueMcb - Supplies the Mcb being examined.

Return Value:

    ULONG - Returns the number of distinct runs mapped by the input Mcb.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    ULONG Count;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlNumberOfRunsInLargeMcb, Mcb = %08lx\n", Mcb );

    ExAcquireFastMutex( Mcb->FastMutex );

    Count = Mcb->PairCount;

    ExReleaseFastMutex( Mcb->FastMutex );

    DebugTrace(-1, Dbg, "FsRtlNumberOfRunsInLargeMcb -> %08lx\n", Count );

    return Count;
}


BOOLEAN
FsRtlGetNextLargeMcbEntry (
    IN PLARGE_MCB OpaqueMcb,
    IN ULONG RunIndex,
    OUT PLONGLONG LargeVbn,
    OUT PLONGLONG LargeLbn,
    OUT PLONGLONG LargeSectorCount
    )

/*++

Routine Description:

    This routine returns to its caller the Vbn, Lbn, and SectorCount for
    distinct runs mapped by an Mcb.  Holes are counted as runs.  For example,
    to construct to print out all of the runs in a a file is:

//. .   for (i = 0; FsRtlGetNextLargeMcbEntry(Mcb,i,&Vbn,&Lbn,&Count); i++) {
//
//. .       // print out vbn, lbn, and count
//
//. .       }

Arguments:

    OpaqueMcb - Supplies the Mcb being examined.

    RunIndex - Supplies the index of the run (zero based) to return to the
        caller.

    Vbn - Receives the starting Vbn of the returned run, or zero if the
        run does not exist.

    Lbn - Recieves the starting Lbn of the returned run, or zero if the
        run does not exist.

    SectorCount - Receives the number of sectors within the returned run,
        or zero if the run does not exist.

Return Value:

    BOOLEAN - TRUE if the specified run (i.e., RunIndex) exists in the Mcb,
        and FALSE otherwise.  If FALSE is returned then the Vbn, Lbn, and
        SectorCount parameters receive zero.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlGetNextLargeMcbEntry, Mcb = %08lx\n", Mcb );
    DebugTrace( 0, Dbg, " RunIndex = %08lx\n", RunIndex );

    ExAcquireFastMutex( Mcb->FastMutex );

    try {

        //
        //  Make sure the run index is within range
        //

        if (RunIndex >= Mcb->PairCount) {

            try_return (Result = FALSE);
        }

        //
        //  Set the return variables
        //

        *((PULONG)LargeVbn) = StartingVbn(Mcb,RunIndex);
        *((PULONG)LargeLbn) = StartingLbn(Mcb,RunIndex);
        *((PULONG)LargeSectorCount) = SectorsWithinRun(Mcb,RunIndex);

        Result = TRUE;

    try_exit: NOTHING;
    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlGetNextLargeMcbEntry -> %08lx\n", Result );
    }

    ((PLARGE_INTEGER)LargeVbn)->HighPart = (*((PULONG)LargeVbn) == UNUSED_LBN ? UNUSED_LBN : 0);
    ((PLARGE_INTEGER)LargeLbn)->HighPart = (*((PULONG)LargeLbn) == UNUSED_LBN ? UNUSED_LBN : 0);
    ((PLARGE_INTEGER)LargeSectorCount)->HighPart = 0;

    return Result;
}


BOOLEAN
FsRtlSplitLargeMcb (
    IN PLARGE_MCB OpaqueMcb,
    IN LONGLONG LargeVbn,
    IN LONGLONG LargeAmount
    )

/*++

Routine Description:

    This routine is used to create a hole within an MCB, by shifting the
    mapping of Vbns.  All mappings above the input vbn are shifted by the
    amount specified and while keeping their current lbn value.  Pictorially
    we have as input the following MCB

        VBN :       LargeVbn-1 LargeVbn         N
            +-----------------+------------------+
        LBN :             X        Y

    And after the split we have

        VBN :       LargeVbn-1               LargeVbn+Amount    N+Amount
            +-----------------+.............+---------------------------+
        LBN :             X      UnusedLbn       Y

    When doing the split we have a few cases to consider.  They are:

    1. The input Vbn is beyond the last run.  In this case this operation
       is a noop.

    2. The input Vbn is within or adjacent to a existing run of unused Lbns.
       In this case we simply need to extend the size of the existing hole
       and shift succeeding runs.

    3. The input Vbn is between two existing runs, including the an input vbn
       value of zero.  In this case we need to add a new entry for the hole
       and shift succeeding runs.

    4. The input Vbn is within an existing run.  In this case we need to add
       two new entries to contain the split run and the hole.

    If pool is not available to store the information this routine will raise a
    status value indicating insufficient resources.

Arguments:

    OpaqueMcb - Supplies the Mcb in which to add the new mapping.

    Vbn - Supplies the starting Vbn that is to be shifted.

    Amount - Supplies the amount to shift by.

Return Value:

    BOOLEAN - TRUE if the mapping was successfully shifted, and FALSE otherwise.
        If FALSE is returned then the Mcb is not changed.

--*/

{
    PNONOPAQUE_MCB Mcb = (PNONOPAQUE_MCB)OpaqueMcb;

    VBN Vbn = ((ULONG)LargeVbn);
    ULONG Amount = ((ULONG)LargeAmount);

    ULONG Index;

    BOOLEAN Result;

    ULONG i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlSplitLargeMcb, Mcb = %08lx\n", Mcb );
    DebugTrace( 0, Dbg, " Vbn    = %08lx\n", Vbn );
    DebugTrace( 0, Dbg, " Amount = %08lx\n", Amount );

    ExAcquireFastMutex( Mcb->FastMutex );

    ASSERTMSG("LargeInteger not supported yet ", ((((PLARGE_INTEGER)&LargeVbn)->HighPart == 0) ||
                                                  (Mcb->PairCount == 0)));
    ASSERTMSG("LargeInteger not supported yet ", ((((PLARGE_INTEGER)&LargeAmount)->HighPart == 0) ||
                                                  (Mcb->PairCount == 0)));

    try {

        //
        //  First lookup the index for the entry that we are going to split.
        //  If we can't find the entry then there is nothing to split.  This
        //  takes care of the case where the input vbn is beyond the last run
        //  in the mcb
        //

        if (!FsRtlFindLargeIndex( Mcb, Vbn, &Index)) {

            try_return(Result = FALSE);
        }

        //
        //  Now check if the input Vbn is within a hole
        //

        if (StartingLbn(Mcb,Index) == UNUSED_LBN) {

            //
            //  Before: --PreviousRun--||--IndexHole--||--FollowingRun--
            //  After:  --PreviousRun--||----IndexHole----||--FollowingRun--
            //
            //      In this case the vbn is somewhere within the hole and we
            //      simply need to added the amount of each existing run
            //      beyond the hole.
            //

            //
            //  In this case there is really nothing to do here because the
            //  ending code will already shift the runs by proper amount
            //  starting at index
            //

            NOTHING;

        //
        //  Now check if the input vbn is between a hole and an existing run.
        //

        } else if ((StartingVbn(Mcb,Index) == Vbn) && (Index != 0) && (PreviousEndingLbn(Mcb,Index) == UNUSED_LBN)) {

            //
            //  Before: --Hole--||--IndexRun--
            //  After:  --Hole------||--IndexRun--
            //
            //      In this case the vbn points to the start of the existing
            //      run and we need to do the split between the hole and the
            //      existing run by simply adding the amount to each existing
            //      run beyond the hole.
            //

            //
            //  In this case we need to decement the index by 1 and then
            //  fall to the bottom code which will do the shifting for us
            //

            Index -= 1;

        //
        //  Now check if the input vbn is between two existing runs
        //

        } else if (StartingVbn(Mcb,Index) == Vbn) {

            //
            //  Before: --PreviousRun--||--IndexRun--
            //  After:  --PreviousRun--||--NewHole--||--IndexRun--
            //
            //  Before: 0:|--IndexRun--
            //  After:  0:|--NewHole--||--IndexRun--
            //
            //      In this case the vbn points to the start of an existing
            //      run and the preceeding is either a real run or the start
            //      of mapping pairs We simply add a new entry for the hole
            //      and shift succeeding runs.
            //

            FsRtlAddLargeEntry( Mcb, Index, 1 );

            (Mcb->Mapping)[Index].Lbn = (LBN)UNUSED_LBN;
            (Mcb->Mapping)[Index].NextVbn = Vbn + Amount;

            Index += 1;

        //
        //  Otherwise the input vbn is inside an existing run
        //

        } else {

            //
            //  Before: --IndexRun--
            //  After:  --SplitRun--||--NewHole--||--SplitRun--
            //
            //      In this case the vbn points within an existing run
            //      we need to add two new extries for hole and split
            //      run and shift succeeding runs
            //

            FsRtlAddLargeEntry( Mcb, Index, 2 );

            (Mcb->Mapping)[Index].Lbn = (Mcb->Mapping)[Index+2].Lbn;
            (Mcb->Mapping)[Index].NextVbn = Vbn;

            (Mcb->Mapping)[Index+1].Lbn = (LBN)UNUSED_LBN;
            (Mcb->Mapping)[Index+1].NextVbn = Vbn + Amount;

            (Mcb->Mapping)[Index+2].Lbn = (Mcb->Mapping)[Index+2].Lbn +
                                          StartingVbn(Mcb, Index+1) -
                                          StartingVbn(Mcb, Index);

            Index += 2;

        }

        //
        //  At this point we have completed most of the work we now need to
        //  shift existing runs from the index to the end of the mappings
        //  by the specified amount
        //

        for (i = Index; i < Mcb->PairCount; i += 1) {

            (Mcb->Mapping)[i].NextVbn += Amount;
        }

        Result = TRUE;

    try_exit: NOTHING;
    } finally {

        ExReleaseFastMutex( Mcb->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlSplitLargeMcb -> %08lx\n", Result );
    }

    return Result;
}


//
//  Private support routine
//

VOID
FsRtlRemoveMcbEntryPrivate (
    IN PNONOPAQUE_MCB Mcb,
    IN ULONG Vbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This is the work routine for remove large mcb entry.  It does the work
    without taking out the mcb FastMutex.

Arguments:

    Mcb - Supplies the Mcb from which to remove the mapping.

    Vbn - Supplies the starting Vbn of the mappings to remove.

    SectorCount - Supplies the size of the mappings to remove (in sectors).

Return Value:

    None.

--*/

{
    ULONG Index;

    PAGED_CODE();

    //
    //  Do a quick test to see if we are wiping out the entire MCB.
    //

    if ((Vbn == 0) && (Mcb->PairCount > 0) && (SectorCount >= Mcb->Mapping[Mcb->PairCount-1].NextVbn)) {

        Mcb->PairCount = 0;

        return;
    }

    //
    //  While there is some more mapping to remove we'll continue
    //  with our main loop
    //

    while (SectorCount > 0) {

        //
        //  Locate the mapping for the vbn
        //

        if (!FsRtlFindLargeIndex(Mcb, Vbn, &Index)) {

            DebugTrace( 0, Dbg, "FsRtlRemoveLargeMcbEntry, Cannot remove an unmapped Vbn = %08lx\n", Vbn );

            return;
        }

        //
        //  Now that we some something to remove the following cases must
        //  be considered
        //

        if ((StartingVbn(Mcb,Index) == Vbn) &&
            (EndingVbn(Mcb,Index) < Vbn + SectorCount)) {

            ULONG i;

            //
            //  Removes the entire run
            //

            //
            //  Update the amount to remove
            //

            i = SectorsWithinRun(Mcb,Index);
            Vbn += i;
            SectorCount -= i;

            //
            //  If already a hole then leave it alone
            //

            if (StartingLbn(Mcb,Index) == UNUSED_LBN) {

                NOTHING;

            //
            //  Test for last run
            //

            } else if (Index == Mcb->PairCount - 1) {

                if ((PreviousEndingLbn(Mcb,Index) != UNUSED_LBN) ||
                    (Index == 0)) {

                    //
                    //  Previous is not hole, index is last run
                    //
                    //  --Previous--|  Hole
                    //
                    //  0:  Hole
                    //

                    DebugTrace( 0, Dbg, "Entire run, Previous not hole, index is last run\n", 0);

                    //
                    //  Just remove this entry
                    //

                    FsRtlRemoveLargeEntry( Mcb, Index, 1);

                } else {

                    //
                    //  Previous is hole, index is last run
                    //
                    //  --Hole--|  Hole
                    //

                    DebugTrace( 0, Dbg, "Entire run, Previous hole, index is last run\n", 0);

                    //
                    //  Just remove this entry, and preceding entry
                    //

                    FsRtlRemoveLargeEntry( Mcb, Index-1, 2);
                }

            } else if (((PreviousEndingLbn(Mcb,Index) != UNUSED_LBN) || (Index == 0)) &&
                       (NextStartingLbn(Mcb,Index) != UNUSED_LBN)) {

                //
                //  Previous and following are not holes
                //
                //  --Previous--|  Hole  |--Following--
                //
                //  0:  Hole  |--Following--
                //

                DebugTrace( 0, Dbg, "Entire run, Previous & Following not holes\n", 0);

                //
                //  Make this index a hole
                //

                (Mcb->Mapping)[Index].Lbn = (LBN)UNUSED_LBN;

            } else if (((PreviousEndingLbn(Mcb,Index) != UNUSED_LBN) || (Index == 0)) &&
                       (NextStartingLbn(Mcb,Index) == UNUSED_LBN)) {

                //
                //  Following is hole
                //
                //  --Previous--|  Hole  |--Hole--
                //
                //  0:  Hole  |--Hole--
                //

                DebugTrace( 0, Dbg, "Entire run, Following is hole\n", 0);

                //
                //  Simply remove this entry
                //

                FsRtlRemoveLargeEntry( Mcb, Index, 1 );

            } else if ((PreviousEndingLbn(Mcb,Index) == UNUSED_LBN) &&
                       (NextStartingLbn(Mcb,Index) != UNUSED_LBN)) {

                //
                //  Previous is hole
                //
                //  --Hole--|  Hole  |--Following--
                //

                DebugTrace( 0, Dbg, "Entire run, Previous is hole\n", 0);

                //
                //  Mark current entry a hole
                //

                (Mcb->Mapping)[Index].Lbn = (LBN)UNUSED_LBN;

                //
                //  Remove previous entry
                //

                FsRtlRemoveLargeEntry( Mcb, Index - 1, 1 );

            } else {

                //
                //  Previous and following are holes
                //
                //  --Hole--|  Hole  |--Hole--
                //

                DebugTrace( 0, Dbg, "Entire run, Previous & following are holes\n", 0);

                //
                //  Remove previous and this entry
                //

                FsRtlRemoveLargeEntry( Mcb, Index - 1, 2 );
            }

        } else if (StartingVbn(Mcb,Index) == Vbn) {

            //
            //  Removes first part of run
            //

            //
            //  If already a hole then leave it alone
            //

            if (StartingLbn(Mcb,Index) == UNUSED_LBN) {

                NOTHING;

            } else if ((PreviousEndingLbn(Mcb,Index) != UNUSED_LBN) || (Index == 0)) {

                //
                //  Previous is not hole
                //
                //  --Previous--|  Hole  |--Index--||--Following--
                //
                //  0:  Hole  |--Index--||--Following--
                //

                DebugTrace( 0, Dbg, "1st part, Previous is not hole\n", 0);

                //
                //  Make room for one more entry.  The NextVbn field of the
                //  one we're shifting remains valid.
                //

                FsRtlAddLargeEntry( Mcb, Index, 1 );

                //
                //  Set the hole
                //

                (Mcb->Mapping)[Index].Lbn = (LBN)UNUSED_LBN;
                (Mcb->Mapping)[Index].NextVbn = Vbn + SectorCount;

                //
                //  Set the new Lbn for the remaining run
                //

                (Mcb->Mapping)[Index+1].Lbn += SectorCount;

            } else {

                //
                //  Previous is hole
                //
                //  --Hole--|  Hole  |--Index--||--Following--
                //

                DebugTrace( 0, Dbg, "1st part, Previous is hole\n", 0);

                //
                //  Expand the preceding hole
                //

                (Mcb->Mapping)[Index-1].NextVbn += SectorCount;

                //
                //  Set the new Lbn for the remaining run
                //

                (Mcb->Mapping)[Index].Lbn += SectorCount;
            }

            //
            //  Update the amount to remove
            //

            Vbn += SectorCount;
            SectorCount = 0;

        } else if (EndingVbn(Mcb,Index) < Vbn + SectorCount) {

            ULONG AmountToRemove;

            AmountToRemove = EndingVbn(Mcb,Index) - Vbn + 1;

            //
            //  Removes last part of run
            //

            //
            //  If already a hole then leave it alone
            //

            if (StartingLbn(Mcb,Index) == UNUSED_LBN) {

                NOTHING;

            } else if (Index == Mcb->PairCount - 1) {

                //
                //  Index is last run
                //
                //  --Previous--||--Index--|  Hole
                //
                //  0:|--Index--|  Hole
                //

                DebugTrace( 0, Dbg, "last part, Index is last run\n", 0);

                //
                //  Shrink back the size of the current index
                //

                (Mcb->Mapping)[Index].NextVbn -= AmountToRemove;

            } else if (NextStartingLbn(Mcb,Index) == UNUSED_LBN) {

                //
                //  Following is hole
                //
                //  --Previous--||--Index--|  Hole  |--Hole--
                //
                //  0:|--Index--|  Hole  |--Hole--
                //

                DebugTrace( 0, Dbg, "last part, Following is hole\n", 0);

                //
                //  Shrink back the size of the current index
                //

                (Mcb->Mapping)[Index].NextVbn -= AmountToRemove;

            } else {

                //
                //  Following is not hole
                //
                //  --Previous--||--Index--|  Hole  |--Following--
                //
                //
                //  0:|--Index--|  Hole  |--Following--
                //

                DebugTrace( 0, Dbg, "last part, Following is not hole\n", 0);

                //
                //  Make room for one more entry.  The NextVbn field of the
                //  one we're shifting remains valid.
                //

                FsRtlAddLargeEntry( Mcb, Index+1, 1 );

                //
                //  Set the new hole
                //

                (Mcb->Mapping)[Index+1].Lbn = (LBN)UNUSED_LBN;
                (Mcb->Mapping)[Index+1].NextVbn = (Mcb->Mapping)[Index].NextVbn;

                //
                //  Shrink back the size of the current index
                //

                (Mcb->Mapping)[Index].NextVbn -= AmountToRemove;
            }

            //
            //  Update amount to remove
            //

            Vbn += AmountToRemove;
            SectorCount -= AmountToRemove;

        } else {

            //
            //  If already a hole then leave it alone
            //

            if (StartingLbn(Mcb,Index) == UNUSED_LBN) {

                NOTHING;

            } else {

                //
                //  Remove middle of run
                //
                //  --Previous--||--Index--|  Hole  |--Index--||--Following--
                //
                //  0:|--Index--|  Hole  |--Index--||--Following--
                //

                DebugTrace( 0, Dbg, "Middle of run\n", 0);

                //
                //  Make room for two more entries.  The NextVbn field of the
                //  one we're shifting remains valid.
                //

                FsRtlAddLargeEntry( Mcb, Index, 2 );

                //
                //  Set up the first remaining run
                //

                (Mcb->Mapping)[Index].Lbn = (Mcb->Mapping)[Index+2].Lbn;
                (Mcb->Mapping)[Index].NextVbn = Vbn;

                //
                //  Set up the hole
                //

                (Mcb->Mapping)[Index+1].Lbn = (LBN)UNUSED_LBN;
                (Mcb->Mapping)[Index+1].NextVbn = Vbn + SectorCount;

                //
                //  Set up the second remaining run
                //

                (Mcb->Mapping)[Index+2].Lbn += SectorsWithinRun(Mcb,Index) +
                                               SectorsWithinRun(Mcb,Index+1);
            }

            //
            //  Update amount to remove
            //

            Vbn += SectorCount;
            SectorCount = 0;
        }
    }

    return;
}


//
//  Private routine
//

BOOLEAN
FsRtlFindLargeIndex (
    IN PNONOPAQUE_MCB Mcb,
    IN VBN Vbn,
    OUT PULONG Index
    )

/*++

Routine Description:

    This is a private routine that locates a mapping for a Vbn
    in a given mapping array

Arguments:

    Mcb - Supplies the mapping array to examine

    Vbn - Supplies the Vbn to look up

    Index - Receives the index within the mapping array of the mapping
        containing the Vbn.  If none if found then the index is set to
        PairCount.

Return Value:

    BOOLEAN - TRUE if Vbn is found and FALSE otherwise

--*/

{
    LONG MinIndex;
    LONG MaxIndex;
    LONG MidIndex;

    //
    //  We'll just do a binary search for the mapping entry.  Min and max
    //  are our search boundaries
    //

    MinIndex = 0;
    MaxIndex = Mcb->PairCount - 1;

    while (MinIndex <= MaxIndex) {

        //
        //  Compute the middle index to look at
        //

        MidIndex = ((MaxIndex + MinIndex) / 2);

        //
        //  check if the Vbn is less than the mapping at the mid index
        //

        if (Vbn < StartingVbn(Mcb, MidIndex)) {

            //
            //  Vbn is less than the middle index so we need to drop
            //  the max down
            //

            MaxIndex = MidIndex - 1;

        //
        //  check if the Vbn is greater than the mapping at the mid index
        //

        } else if (Vbn > EndingVbn(Mcb, MidIndex)) {

            //
            //  Vbn is greater than the middle index so we need to bring
            //  up the min
            //

            MinIndex = MidIndex + 1;

        //
        //  Otherwise we've found the index containing the Vbn so set the
        //  index and return TRUE.
        //

        } else {

            *Index = MidIndex;

            return TRUE;
        }
    }

    //
    //  A match wasn't found so set index to PairCount and return FALSE
    //

    *Index = Mcb->PairCount;

    return FALSE;
}


//
//  Private Routine
//

VOID
FsRtlAddLargeEntry (
    IN PNONOPAQUE_MCB Mcb,
    IN ULONG WhereToAddIndex,
    IN ULONG AmountToAdd
    )

/*++

Routine Description:

    This routine takes a current Mcb and detemines if there is enough
    room to add the new mapping entries.  If there is not enough room
    it reallocates a new mcb buffer and copies over the current mapping.
    If also will spread out the current mappings to leave the specified
    index slots in the mapping unfilled.  For example, if WhereToAddIndex
    is equal to the current pair count then we don't need to make a hole
    in the mapping, but if the index is less than the current pair count
    then we'll need to slide some of the mappings down to make room
    at the specified index.

Arguments:

    Mcb - Supplies the mcb being checked and modified

    WhereToAddIndex - Supplies the index of where the additional entries
        need to be made

    AmountToAdd - Supplies the number of additional entries needed in the
        mcb

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Check to see if the current buffer is large enough to hold
    //  the additional entries
    //

    if (Mcb->PairCount + AmountToAdd > Mcb->MaximumPairCount) {

        ULONG NewMax;
        PMAPPING Mapping;

        //
        //  We need to allocate a new mapping so compute a new maximum pair
        //  count.  We'll only be asked to grow by at most 2 at a time, so
        //  doubling will definitely make us large enough for the new amount.
        //  But we won't double without bounds we'll stop doubling if the
        //  pair count gets too high.
        //

        if (Mcb->MaximumPairCount < 2048) {

            NewMax = Mcb->MaximumPairCount * 2;

        } else {

            NewMax = Mcb->MaximumPairCount + 2048;
        }

        Mapping = FsRtlpAllocatePool( Mcb->PoolType, sizeof(MAPPING) * NewMax );

        //**** RtlZeroMemory( Mapping, sizeof(MAPPING) * NewMax );

        //
        //  Now copy over the old mapping to the new buffer
        //

        RtlCopyMemory( Mapping, Mcb->Mapping, sizeof(MAPPING) * Mcb->PairCount );

        //
        //  Deallocate the old buffer
        //

        if ((Mcb->PoolType == PagedPool) && (Mcb->MaximumPairCount == INITIAL_MAXIMUM_PAIR_COUNT)) {

            FsRtlFreeFirstMapping( Mcb->Mapping );

        } else {

            ExFreePool( Mcb->Mapping );
        }

        //
        //  And set up the new buffer in the Mcb
        //

        Mcb->Mapping = Mapping;
        Mcb->MaximumPairCount = NewMax;
    }

    //
    //  Now see if we need to shift some entries over according to the
    //  WhereToAddIndex value
    //

    if (WhereToAddIndex < Mcb->PairCount) {

        RtlMoveMemory( &((Mcb->Mapping)[WhereToAddIndex + AmountToAdd]),
                       &((Mcb->Mapping)[WhereToAddIndex]),
                       (Mcb->PairCount - WhereToAddIndex) * sizeof(MAPPING) );
    }

    //
    //  Now zero out the new additions
    //

    //**** RtlZeroMemory( &((Mcb->Mapping)[WhereToAddIndex]), sizeof(MAPPING) * AmountToAdd );

    //
    //  Now increment the PairCount
    //

    Mcb->PairCount += AmountToAdd;

    //
    //  And return to our caller
    //

    return;
}


//
//  Private Routine
//

VOID
FsRtlRemoveLargeEntry (
    IN PNONOPAQUE_MCB Mcb,
    IN ULONG WhereToRemoveIndex,
    IN ULONG AmountToRemove
    )

/*++

Routine Description:

    This routine takes a current Mcb and removes one or more entries.

Arguments:

    Mcb - Supplies the mcb being checked and modified

    WhereToRemoveIndex - Supplies the index of the entries to remove

    AmountToRemove - Supplies the number of entries to remove

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Check to see if we need to shift everything down because the
    //  entries to remove do not include the last entry in the mcb
    //

    if (WhereToRemoveIndex + AmountToRemove < Mcb->PairCount) {

        RtlMoveMemory( &((Mcb->Mapping)[WhereToRemoveIndex]),
                      &((Mcb->Mapping)[WhereToRemoveIndex + AmountToRemove]),
                      (Mcb->PairCount - (WhereToRemoveIndex + AmountToRemove))
                                                           * sizeof(MAPPING) );
    }

    //
    //  Now zero out the entries beyond the part we just shifted down
    //

    //**** RtlZeroMemory( &((Mcb->Mapping)[Mcb->PairCount - AmountToRemove]), AmountToRemove * sizeof(MAPPING) );

    //
    //  Now decrement the PairCount
    //

    Mcb->PairCount -= AmountToRemove;

    //
    //  And return to our caller
    //

    return;
}

