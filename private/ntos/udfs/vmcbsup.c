/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VmcbSup.c

Abstract:

    Historical note: this package was originally written for HPFS (pinball)
    and is now resurrected for UDFS.  Since UDFS is readonly in initial
    versions we will snip by #ifdef the write support, leaving it visible
    for the future - this code has not been changed (nearly) whatsoever and
    is left named as Pb (pinball) code.

    The VMCB routines provide support for maintaining a mapping between
    LBNs and VBNs for a virtual volume file.  The volume file is all
    of the sectors that make up the on-disk structures.  A file system
    uses this package to map LBNs for on-disk structure to VBNs in a volume
    file.  This when used in conjunction with Memory Management and the
    Cache Manager will treat the volume file as a simple mapped file.  A
    variable of type VMCB is used to store the mapping information and one
    is needed for every mounted volume.

    The main idea behind this package is to allow the user to dynamically
    read in new disk structure sectors (e.g., File Entries).  The user assigns
    the new sector a VBN in the Volume file and has memory management fault
    the page containing the sector into memory.  To do this Memory management
    will call back into the file system to read the page from the volume file
    passing in the appropriate VBN.  Now the file system takes the VBN and
    maps it back to its LBN and does the read.

    The granularity of mapping is one a per page basis.  That is if
    a mapping for LBN 8 is added to the VMCB structure and the page size
    is 8 sectors then the VMCB routines will actually assign a mapping for
    LBNS 8 through 15, and they will be assigned to a page aligned set of
    VBNS.  This function is needed to allow us to work efficiently with
    memory management.  This means that some sectors in some pages might
    actually contain regular file data and not volume information, and so
    when writing the page out we must only write the sectors that are really
    in use by the volume file.  To help with this we provide a set
    of routines to keep track of dirty volume file sectors.
    That way, when the file system is called to write a page to the volume
    file, it will only write the sectors that are dirty.

    Concurrent access the VMCB structure is control by this package.

    The functions provided in this package are as follows:

      o  UdfInitializeVmcb - Initialize a new VMCB structure.

      o  UdfUninitializeVmcb - Uninitialize an existing VMCB structure.

      o  UdfSetMaximumLbnVmcb - Sets/Resets the maximum allowed LBN
         for the specified VMCB structure.

      o  UdfAddVmcbMapping - This routine takes an LBN and assigns to it
         a VBN.  If the LBN already was assigned to an VBN it simply returns
         the old VBN and does not do a new assignemnt.

      o  UdfRemoveVmcbMapping - This routine takes an LBN and removes its
         mapping from the VMCB structure.

      o  UdfVmcbVbnToLbn - This routine takes a VBN and returns the
         LBN it maps to.

      o  UdfVmcbLbnToVbn - This routine takes an LBN and returns the
         VBN its maps to.

#if VMCB_WRITE_SUPPORT

      o  PbSetDirtyVmcb - This routine is used to mark sectors dirty
         in the volume file.

      o  PbSetCleanVmcb - This routine is used to mark sectors clean
         in the volume file.

      o  PbGetDirtySectorsVmcb - This routine is used to retrieve the
         dirty sectors for a page in the volume file.

      o  PbGetAndCleanDirtyVmcb - This routine is used to retrieve the
         dirty sectors for a page in the volume file and atomically clear
         the dirty sectors.

#endif // VMCB_WRITE_SUPPORT

Authors:

    Gary Kimura     [GaryKi]    4-Apr-1990
    Dan Lovinger    [DanLo]     10-Sep-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_VMCBSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_VMCBSUP)

//
//  Turn off write/dirty/clean sector support
//

#define VMCB_WRITE_SUPPORT 0


//
//  The following macro is used to calculate the number of pages (in terms of
//  sectors) needed to contain a given sector count.  For example,
//
//      PageAlign( 0 Sectors ) = 0 Pages = 0 Sectors
//      PageAlign( 1 Sectors ) = 1 Page  = 8 Sectors
//      PageAlign( 2 Sectors ) = 1 Page  = 8 Sectors
//

#define PageAlign(V, L) ((((L)+((PAGE_SIZE/(V)->SectorSize)-1))/(PAGE_SIZE/(V)->SectorSize))*(PAGE_SIZE/(V)->SectorSize))


#if VMCB_WRITE_SUPPORT

//
//  The following constant is a bit mask, with one bit set for each sector
//  that'll fit in a page (4K page, 8 bits; 8K page, 16 bits, etc)
//

#define SECTOR_MASK ((1 << (PAGE_SIZE / sizeof (SECTOR))) - 1)

//
//  The Dirty Page structure are elements in the dirty table generic table.
//  This is followed by the procedure prototypes for the local generic table
//  routines
//

typedef struct _DIRTY_PAGE {
    ULONG LbnPageNumber;
    ULONG DirtyMask;
} DIRTY_PAGE;
typedef DIRTY_PAGE *PDIRTY_PAGE;

RTL_GENERIC_COMPARE_RESULTS
PbCompareDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    );

PVOID
PbAllocateDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN CLONG ByteSize
    );

VOID
PbDeallocateDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN PVOID Buffer
    );

ULONG
PbDumpDirtyVmcb (
    IN PVMCB Vmcb
    );

#endif // VMCB_WRITE_SUPPORT

//
//  Local Routines.
//

BOOLEAN
UdfVmcbLookupMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL,
    OUT PULONG Index OPTIONAL
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfAddVmcbMapping)

#if VMCB_WRITE_SUPPORT
#pragma alloc_text(PAGE, PbAllocateDirtyVmcb)
#pragma alloc_text(PAGE, PbDeallocateDirtyVmcb)
#pragma alloc_text(PAGE, PbDumpDirtyVmcb)
#pragma alloc_text(PAGE, PbGetAndCleanDirtyVmcb)
#endif // VMCB_WRITE_SUPPORT

#pragma alloc_text(PAGE, UdfInitializeVmcb)
#pragma alloc_text(PAGE, UdfRemoveVmcbMapping)
#pragma alloc_text(PAGE, UdfResetVmcb)

#if VMCB_WRITE_SUPPORT
#pragma alloc_text(PAGE, PbSetCleanVmcb)
#pragma alloc_text(PAGE, PbSetDirtyVmcb)
#endif // VMCB_WRITE_SUPPORT

#pragma alloc_text(PAGE, UdfSetMaximumLbnVmcb)
#pragma alloc_text(PAGE, UdfUninitializeVmcb)
#pragma alloc_text(PAGE, UdfVmcbLbnToVbn)
#pragma alloc_text(PAGE, UdfVmcbLookupMcbEntry)
#pragma alloc_text(PAGE, UdfVmcbVbnToLbn)
#endif


VOID
UdfInitializeVmcb (
    IN PVMCB Vmcb,
    IN POOL_TYPE PoolType,
    IN ULONG MaximumLbn,
    IN ULONG SectorSize
    )

/*++

Routine Description:

    This routine initializes a new Vmcb Structure.  The caller must
    supply the memory for the structure.  This must precede all other calls
    that set/query the volume file mapping.

    If pool is not available this routine will raise a status value
    indicating insufficient resources.

Arguments:

    Vmcb - Supplies a pointer to the volume file structure to initialize.

    PoolType - Supplies the pool type to use when allocating additional
        internal structures.

    MaximumLbn - Supplies the maximum Lbn value that is valid for this
        volume.

    LbSize - Size of a sector on this volume

Return Value:

    None

--*/

{
    BOOLEAN VbnInitialized;
    BOOLEAN LbnInitialized;

    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfInitializeVmcb, Vmcb = %08x\n", Vmcb ));

    VbnInitialized = FALSE;
    LbnInitialized = FALSE;

    try {

        //
        //  Initialize the fields in the vmcb structure
        //

        KeInitializeMutex( &Vmcb->Mutex, 0 );

        FsRtlInitializeMcb( &Vmcb->VbnIndexed, PoolType );
        VbnInitialized = TRUE;

        FsRtlInitializeMcb( &Vmcb->LbnIndexed, PoolType );
        LbnInitialized = TRUE;

        Vmcb->MaximumLbn = MaximumLbn;

        Vmcb->SectorSize = SectorSize;

#if VMCB_WRITE_SUPPORT

        //
        //  For the dirty table we store in the table context field the pool
        //  type to use for allocating additional structures
        //

        RtlInitializeGenericTable( &Vmcb->DirtyTable,
                                   PbCompareDirtyVmcb,
                                   PbAllocateDirtyVmcb,
                                   PbDeallocateDirtyVmcb,
                                   (PVOID)PoolType );

#endif // VMCB_WRITE_SUPPORT

    } finally {

        //
        //  If this is an abnormal termination then check if we need to
        //  uninitialize the mcb structures
        //

        if (AbnormalTermination()) {

            if (VbnInitialized) { FsRtlUninitializeMcb( &Vmcb->VbnIndexed ); }
            if (LbnInitialized) { FsRtlUninitializeMcb( &Vmcb->LbnIndexed ); }
        }

        DebugUnwind("UdfInitializeVmcb");
        DebugTrace(( -1, Dbg, "UdfInitializeVmcb -> VOID\n" ));
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
UdfUninitializeVmcb (
    IN PVMCB Vmcb
    )

/*++

Routine Description:

    This routine uninitializes an existing VMCB structure.  After calling
    this routine the input VMCB structure must be re-initialized before
    being used again.

Arguments:

    Vmcb - Supplies a pointer to the VMCB structure to uninitialize.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfUninitializeVmcb, Vmcb = %08x\n", Vmcb ));

    //
    //  Unitialize the fields in the Vmcb structure
    //

    FsRtlUninitializeMcb( &Vmcb->VbnIndexed );
    FsRtlUninitializeMcb( &Vmcb->LbnIndexed );

    //
    //  And return to our caller
    //

    DebugTrace(( -1, Dbg, "UdfUninitializeVmcb -> VOID\n" ));

    return;
}


VOID
UdfResetVmcb (
    IN PVMCB Vmcb
    )

/*++

Routine Description:

    This routine resets the mappings in an existing VMCB structure.

Arguments:

    Vmcb - Supplies a pointer to the VMCB structure to reset.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfResetVmcb, Vmcb = %08x\n", Vmcb ));

    //
    //  Unitialize the fields in the Vmcb structure
    //

    FsRtlResetLargeMcb( (PLARGE_MCB) &Vmcb->VbnIndexed, TRUE );
    FsRtlResetLargeMcb( (PLARGE_MCB) &Vmcb->LbnIndexed, TRUE );

    //
    //  And return to our caller
    //

    DebugTrace(( -1, Dbg, "UdfResetVmcb -> VOID\n" ));

    return;
}


VOID
UdfSetMaximumLbnVmcb (
    IN PVMCB Vmcb,
    IN ULONG MaximumLbn
    )

/*++

Routine Description:

    This routine sets/resets the maximum allowed LBN for the specified
    Vmcb structure.  The Vmcb structure must already have been initialized
    by calling UdfInitializeVmcb.

Arguments:

    Vmcb - Supplies a pointer to the volume file structure to initialize.

    MaximumLbn - Supplies the maximum Lbn value that is valid for this
        volume.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfSetMaximumLbnVmcb, Vmcb = %08x\n", Vmcb ));

    //
    //  Set the field
    //

    Vmcb->MaximumLbn = MaximumLbn;

    //
    //  And return to our caller
    //

    DebugTrace(( -1, Dbg, "UdfSetMaximumLbnVmcb -> VOID\n" ));

    return;
}


BOOLEAN
UdfVmcbVbnToLbn (
    IN PVMCB Vmcb,
    IN VBN Vbn,
    IN PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL
    )

/*++

Routine Description:

    This routine translates a VBN to an LBN.

Arguments:

    Vmcb - Supplies the VMCB structure being queried.

    Vbn - Supplies the VBN to translate from.

    Lbn - Receives the LBN mapped by the input Vbn.  This value is only valid
        if the function result is TRUE.

    SectorCount - Optionally receives the number of sectors corresponding
        to the run.

Return Value:

    BOOLEAN - TRUE if he Vbn has a valid mapping and FALSE otherwise.

--*/

{
    BOOLEAN Result;

    DebugTrace(( +1, Dbg, "UdfVmcbVbnToLbn, Vbn = %08x\n", Vbn ));

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        Result = UdfVmcbLookupMcbEntry( &Vmcb->VbnIndexed,
                                        Vbn,
                                        Lbn,
                                        SectorCount,
                                        NULL );

        DebugTrace(( 0, Dbg, "*Lbn = %08x\n", *Lbn ));

        //
        //  If the returned Lbn is greater than the maximum allowed Lbn
        //  then return FALSE
        //

        if (Result && (*Lbn > Vmcb->MaximumLbn)) {

            try_leave( Result = FALSE );
        }

        //
        //  If the last returned Lbn is greater than the maximum allowed Lbn
        //  then bring in the sector count
        //

        if (Result &&
            ARGUMENT_PRESENT(SectorCount) &&
            (*Lbn+*SectorCount-1 > Vmcb->MaximumLbn)) {

            *SectorCount = (Vmcb->MaximumLbn - *Lbn + 1);
        }

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugUnwind("UdfVmcbVbnToLbn");
        DebugTrace(( -1, Dbg, "UdfVmcbVbnToLbn -> Result = %08x\n", Result ));
    }


    return Result;
}


BOOLEAN
UdfVmcbLbnToVbn (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    OUT PVBN Vbn,
    OUT PULONG SectorCount OPTIONAL
    )

/*++

Routine Description:

    This routine translates an LBN to a VBN.

Arguments:

    Vmcb - Supplies the VMCB structure being queried.

    Lbn - Supplies the LBN to translate from.

    Vbn - Recieves the VBN mapped by the input LBN.  This value is
        only valid if the function result is TRUE.

    SectorCount - Optionally receives the number of sectors corresponding
        to the run.

Return Value:

    BOOLEAN - TRUE if the mapping is valid and FALSE otherwise.

--*/

{
    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfVmcbLbnToVbn, Lbn = %08x\n", Lbn ));

    //
    //  If the requested Lbn is greater than the maximum allowed Lbn
    //  then the result is FALSE
    //

    if (Lbn > Vmcb->MaximumLbn) {

        DebugTrace(( -1, Dbg, "Lbn too large, UdfVmcbLbnToVbn -> FALSE\n" ));

        return FALSE;
    }

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        Result = UdfVmcbLookupMcbEntry( &Vmcb->LbnIndexed,
                                        Lbn,
                                        Vbn,
                                        SectorCount,
                                        NULL );

        DebugTrace(( 0, Dbg, "*Vbn = %08x\n", *Vbn ));

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );


        DebugUnwind("UdfVmcbLbnToVbn");
        DebugTrace(( -1, Dbg, "UdfVmcbLbnToVbn -> Result = %08x\n", Result ));
    }

    return Result;
}


BOOLEAN
UdfAddVmcbMapping (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN BOOLEAN ExactEnd,
    OUT PVBN Vbn,
    OUT PULONG AlignedSectorCount
    )

/*++

Routine Description:

    This routine adds a new LBN to VBN mapping to the VMCB structure.  When
    a new LBN is added to the structure it does it only on page aligned
    boundaries.

    If pool is not available to store the information this routine will
    raise a status value indicating insufficient resources.

Arguments:

    Vmcb - Supplies the VMCB being updated.

    Lbn - Supplies the starting LBN to add to VMCB.

    SectorCount - Supplies the number of Sectors in the run
    
    ExactEnd - Indicates that instead of aligning to map sectors beyond
        the end of the request, use a hole.  Implies trying to look at 
        these sectors could be undesireable.

    Vbn - Receives the assigned VBN
    
    AlignedSectorCount - Receives the actual sector count created in the
        Vmcb for page alignment purposes. Vbn+AlignedSectorCount-1 == LastVbn.

Return Value:

    BOOLEAN - TRUE if this is a new mapping and FALSE if the mapping
        for the LBN already exists.  If it already exists then the
        sector count for this new addition must already be in the
        VMCB structure

--*/

{

    BOOLEAN Result;

    BOOLEAN VbnMcbAdded;
    BOOLEAN LbnMcbAdded;

    LBN LocalLbn;
    VBN LocalVbn;
    ULONG LocalCount;

    PAGED_CODE();

    DebugTrace(( +1, Dbg, "UdfAddVmcbMapping, Lbn = %08x\n", Lbn ));
    DebugTrace(( 0, Dbg, " SectorCount = %08x\n", SectorCount ));

    ASSERT( SectorCount != 0 );

    VbnMcbAdded = FALSE;
    LbnMcbAdded = FALSE;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        //
        //  Check if the Lbn is already mapped, which means we find an entry
        //  with a non zero mapping Vbn value.
        //

        if (UdfVmcbLookupMcbEntry( &Vmcb->LbnIndexed,
                                   Lbn,
                                   Vbn,
                                   &LocalCount,
                                   NULL )) {

            //
            //  It is already mapped so now the sector count must not exceed
            //  the count already in the run
            //

            if (SectorCount <= LocalCount) {

                try_leave( Result = FALSE );
            }
        }

        //
        //  At this point, we did not find a full existing mapping for the
        //  Lbn and count.  But there might be some overlapping runs that we'll
        //  need to now remove from the vmcb structure.  So for each Lbn in
        //  the range we're after, check to see if it is mapped and remove the
        //  mapping.  We only need to do this test if the sector count is less
        //  than or equal to a page size.  Because those are the only
        //  structures that we know we'll try an remove/overwrite.
        //

        if (SectorCount <= PageAlign(Vmcb, 1)) {

            if (UdfVmcbLookupMcbEntry( &Vmcb->LbnIndexed,
                                       Lbn,
                                       Vbn,
                                       &LocalCount,
                                       NULL )) {

                UdfRemoveVmcbMapping( Vmcb, *Vbn, PageAlign(Vmcb, 1) );
            }
        }

        //
        //  We need to add this new run at the end of the Vbns.  To do this we
        //  need to look up the last mcb entry or use a vbn for the second
        //  page, if the mcb is empty.  We'll also special case the situation
        //  where the last lbn of the mapping and the mapping we're adding
        //  simply flow into each other in which case we'll not bother bumping
        //  the vbn to a page alignment
        //

        if (FsRtlLookupLastMcbEntry( &Vmcb->VbnIndexed, &LocalVbn, &LocalLbn )) {

            if (LocalLbn + 1 == Lbn) {

                LocalVbn = LocalVbn + 1;
                LocalLbn = LocalLbn + 1;

            } else {

                //
                //  Get the next available Vbn Page, and calculate the
                //  Lbn for the page containing the Lbn
                //

                LocalVbn = PageAlign( Vmcb, LocalVbn + 1 );
                LocalLbn = PageAlign( Vmcb, Lbn + 1 ) - PageAlign( Vmcb, 1 );
            }

        } else {

            //
            //  Get the first available Vbn page, and calculate the
            //  Lbn for the page containing the Lbn.
            //


            LocalVbn = 0;
            LocalLbn = PageAlign( Vmcb, Lbn + 1 ) - PageAlign( Vmcb, 1 );
        }

        //
        //  Calculate the number of sectors that we need to map to keep
        //  everything on a page granularity.
        //

        LocalCount = PageAlign( Vmcb, SectorCount + (Lbn - LocalLbn) );

        //
        //  See if we should use a hole to map the alignment at the end of the request.
        //
        
        if (ExactEnd && Lbn + SectorCount < LocalLbn + LocalCount) {

            LocalCount = SectorCount + (Lbn - LocalLbn);
        }
        
        //
        //  Add the double mapping
        //

        FsRtlAddMcbEntry( &Vmcb->VbnIndexed,
                          LocalVbn,
                          LocalLbn,
                          LocalCount );

        VbnMcbAdded = TRUE;

        FsRtlAddMcbEntry( &Vmcb->LbnIndexed,
                          LocalLbn,
                          LocalVbn,
                          LocalCount );

        LbnMcbAdded = TRUE;

        *Vbn = LocalVbn + (Lbn - LocalLbn);
        *AlignedSectorCount = LocalCount - (Lbn - LocalLbn);

        try_leave( Result = TRUE );

    } finally {

        //
        //  If this is an abnormal termination then clean up any mcb's that we
        //  might have modified.
        //

        if (AbnormalTermination()) {

            if (VbnMcbAdded) { FsRtlRemoveMcbEntry( &Vmcb->VbnIndexed, LocalVbn, LocalCount ); }
            if (LbnMcbAdded) { FsRtlRemoveMcbEntry( &Vmcb->LbnIndexed, LocalLbn, LocalCount ); }
        }

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugUnwind("UdfAddVmcbMapping");
        DebugTrace(( 0, Dbg, " LocalVbn   = %08x\n", LocalVbn ));
        DebugTrace(( 0, Dbg, " LocalLbn   = %08x\n", LocalLbn ));
        DebugTrace(( 0, Dbg, " LocalCount = %08x\n", LocalCount ));
        DebugTrace(( 0, Dbg, " *Vbn                = %08x\n", *Vbn ));
        DebugTrace(( 0, Dbg, " *AlignedSectorCount = %08x\n", *AlignedSectorCount ));
        DebugTrace((-1, Dbg, "UdfAddVmcbMapping -> %08x\n", Result ));
    }

    return Result;
}


VOID
UdfRemoveVmcbMapping (
    IN PVMCB Vmcb,
    IN VBN Vbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine removes a Vmcb mapping.

    If pool is not available to store the information this routine will
    raise a status value indicating insufficient resources.

Arguments:

    Vmcb - Supplies the Vmcb being updated.

    Vbn - Supplies the VBN to remove

    SectorCount - Supplies the number of sectors to remove.

Return Value:

    None.

--*/

{
    LBN Lbn;
    ULONG LocalCount;
    ULONG i;

    PAGED_CODE();

    DebugTrace((+1, Dbg, "UdfRemoveVmcbMapping, Vbn = %08x\n", Vbn ));
    DebugTrace(( 0, Dbg, " SectorCount = %08x\n", SectorCount ));

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        for (i = 0; i < SectorCount; i += 1) {

            //
            //  Lookup the Vbn so we can get its current Lbn mapping
            //

            if (!UdfVmcbLookupMcbEntry( &Vmcb->VbnIndexed,
                                        Vbn + i,
                                        &Lbn,
                                        &LocalCount,
                                        NULL )) {

                UdfBugCheck( 0, 0, 0 );
            }

            FsRtlRemoveMcbEntry( &Vmcb->VbnIndexed,
                                 Vbn + i,
                                 1 );

            FsRtlRemoveMcbEntry( &Vmcb->LbnIndexed,
                                 Lbn,
                                 1 );
        }

        {
            DebugTrace(( 0, Dbg, "VbnIndex:\n", 0 ));
            DebugTrace(( 0, Dbg, "LbnIndex:\n", 0 ));
        }

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugUnwind( "UdfRemoveVmcbMapping" );
        DebugTrace(( -1, Dbg, "UdfRemoveVmcbMapping -> VOID\n" ));
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
UdfVmcbLookupMcbEntry (
    IN PMCB Mcb,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL,
    OUT PULONG Index OPTIONAL
    )

/*++

Routine Description:

    This routine retrieves the mapping of a Vbn to an Lbn from an Mcb.
    It indicates if the mapping exists and the size of the run.
    
    The only difference betweent this and the regular FsRtlLookupMcbEntry
    is that we undo the behavior of returning TRUE in holes in the allocation.
    This is because we don't want to avoid mapping at Lbn 0, which is how the
    emulated behavior of the small Mcb package tells callers that there is no
    mapping at that location in a hole.  We have holes all over our Vbn space
    in the VbnIndexed map.
    
    The small Mcb package was able to get away with this because Lbn 0 was the
    boot sector (or similar magic location) on the disc.  In our metadata stream,
    we wish to use Vbn 0 (remember this is a double map).

Arguments:

    Mcb - Supplies the Mcb being examined.

    Vbn - Supplies the Vbn to lookup.

    Lbn - Receives the Lbn corresponding to the Vbn.  A value of -1 is
        returned if the Vbn does not have a corresponding Lbn.

    SectorCount - Receives the number of sectors that map from the Vbn to
        contiguous Lbn values beginning with the input Vbn.

    Index - Receives the index of the run found.

Return Value:

    BOOLEAN - TRUE if the Vbn is within the range of VBNs mapped by the
        MCB (not if it corresponds to a hole in the mapping), and FALSE
        if the Vbn is beyond the range of the MCB's mapping.

        For example, if an MCB has a mapping for VBNs 5 and 7 but not for
        6, then a lookup on Vbn 5 or 7 will yield a non zero Lbn and a sector
        count of 1.  A lookup for Vbn 6 will return FALSE with an Lbn value of
        0, and lookup for Vbn 8 or above will return FALSE.

--*/

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

    if ((ULONG)LiLbn == -1) {

        *Lbn = 0;
        Results = FALSE;
    
    } else {

        *Lbn = (ULONG)LiLbn;
    }

    if (ARGUMENT_PRESENT(SectorCount)) { *SectorCount = ((ULONG)LiSectorCount); }

    return Results;
}


#if VMCB_WRITE_SUPPORT
VOID
PbSetDirtyVmcb (
    IN PVMCB Vmcb,
    IN ULONG LbnPageNumber,
    IN ULONG Mask
    )

/*++

Routine Description:

    This routine sets the sectors within a page as dirty based on the input
    mask.

    If pool is not available to store the information this routine will
    raise a status value indicating insufficient resources.

Arguments:

    Vmcb - Supplies the Vmcb being manipulated.

    LbnPageNumber - Supplies the Page Number (LBN based) of the page being
        modified.  For example, with a page size of 8 a page number of 0
        corresponds to LBN values 0 through 7, a page number of 1 corresponds
        to 8 through 15, and so on.

    Mask - Supplies the mask of dirty sectors to set for the Page (a 1 bit
        means to set it dirty).  For example to set LBN 9 dirty on a system
        with a page size of 8 the LbnPageNumber will be 1, and the mask will
        be 0x00000002.

Return Value:

    None.

--*/

{
    DIRTY_PAGE Key;
    PDIRTY_PAGE Entry;

    PAGED_CODE();

    DebugTrace(+1, Dbg, ( "UdfSetDirtyVmcb\n", 0 ) );
    DebugTrace( 0, Dbg, ( " LbnPageNumber = %08x\n", LbnPageNumber ) );
    DebugTrace( 0, Dbg, ( " Mask          = %08x\n", Mask ) );

    Key.LbnPageNumber = LbnPageNumber;
    Key.DirtyMask = 0;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        Entry = RtlInsertElementGenericTable( &Vmcb->DirtyTable,
                                              &Key,
                                              sizeof(DIRTY_PAGE),
                                              NULL );

        Entry->DirtyMask = (Entry->DirtyMask | Mask) & (SECTOR_MASK); //**** change to manifest constant

        DebugTrace(0, Dbg, ( "DirtyMask = %08x\n", Entry->DirtyMask ) );

        {
            DebugTrace(0, Dbg, ( "", PbDumpDirtyVmcb(Vmcb) ) );
        }

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugUnwind("UdfSetDirtyVmcb");
        DebugTrace(-1, Dbg, ( "UdfSetDirtyVmcb -> VOID\n", 0 ) );
    }

    return;
}
#endif // VMCB_WRITE_SUPPORT


#if VMCB_WRITE_SUPPORT
VOID
PbSetCleanVmcb (
    IN PVMCB Vmcb,
    IN ULONG LbnPageNumber,
    IN ULONG Mask
    )

/*++

Routine Description:

    This routine sets all of the sectors within a page as clean.  All
    of the sectors in a page whether they are dirty or not are set clean
    by this procedure.

Arguments:

    Vmcb - Supplies the Vmcb being manipulated.

    LbnPageNumber - Supplies the Page Number (Lbn based) of page being
        modified.  For example, with a page size of 8 a page number of 0
        corresponds to LBN values 0 through 7, a page number of 1 corresponds
        to 8 through 15, and so on.

    Mask - Supplies the mask of clean sectors to set for the Page (a 1 bit
        means to set it clean).  For example to set LBN 9 clean on a system
        with a page size of 8 the LbnPageNumber will be 1, and the mask will
        be 0x00000002.

Return Value:

    None.

--*/

{
    DIRTY_PAGE Key;
    PDIRTY_PAGE Entry;

    PAGED_CODE();

    DebugTrace(+1, Dbg, ( "UdfSetCleanVmcb\n", 0 ) );
    DebugTrace( 0, Dbg, ( " LbnPageNumber = %08x\n", LbnPageNumber ) );
    DebugTrace( 0, Dbg, ( " Mask          = %08x\n", Mask ) );

    Key.LbnPageNumber = LbnPageNumber;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        //
        // If the page is not in the table, it is already all clean
        //

        if (Entry = RtlLookupElementGenericTable( &Vmcb->DirtyTable, &Key )) {

            Entry->DirtyMask &= ~Mask;

            DebugTrace(0, Dbg, ( "DirtyMask = %08x\n", Entry->DirtyMask ) );

            //
            // If the mask is all clean now, delete the entry
            //

            if (Entry->DirtyMask == 0) {

                (VOID)RtlDeleteElementGenericTable( &Vmcb->DirtyTable, &Key );
            }
        }

        {
            DebugTrace(0, Dbg, ( "", PbDumpDirtyVmcb(Vmcb) ) );
        }

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugTrace(-1, Dbg, ( "UdfSetCleanVcmb -> VOID\n", 0 ) );
    }

    return;
}
#endif // VMCB_WRITE_SUPPORT


#if VMCB_WRITE_SUPPORT
ULONG
PbGetDirtySectorsVmcb (
    IN PVMCB Vmcb,
    IN ULONG LbnPageNumber
    )

/*++

Routine Description:

    This routine returns to its caller a mask of dirty sectors within a page.

Arguments:

    Vmcb - Supplies the Vmcb being manipulated

    LbnPageNumber - Supplies the Page Number (Lbn based) of page being
        modified.  For example, with a page size of 8 a page number of 0
        corresponds to LBN values 0 through 7, a page number of 1 corresponds
        to 8 through 15, and so on.

Return Value:

    ULONG - Receives a mask of dirty sectors within the specified page.
        (a 1 bit indicates that the sector is dirty).

--*/

{
    DIRTY_PAGE Key;
    PDIRTY_PAGE Entry;
    ULONG Mask;

    DebugTrace(+1, Dbg, ( "UdfGetDirtySectorsVmcb\n", 0 ) );
    DebugTrace( 0, Dbg, ( " LbnPageNumber = %08x\n", LbnPageNumber ) );

    Key.LbnPageNumber = LbnPageNumber;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        if ((Entry = RtlLookupElementGenericTable( &Vmcb->DirtyTable,
                                                   &Key )) == NULL) {

            DebugTrace(0, Dbg, ( "Entry not found\n", 0 ) );

            try_leave( Mask = 0 );
        }

        Mask = Entry->DirtyMask & (SECTOR_MASK); //**** change to manifest constant

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugTrace(-1, Dbg, ( "UdfGetDirtySectorsVmcb -> %08x\n", Mask ) );
    }

    return Mask;
}
#endif // VMCB_WRITE_SUPPORT


#if VMCB_WRITE_SUPPORT
ULONG
PbGetAndCleanDirtyVmcb (
    IN PVMCB Vmcb,
    IN ULONG LbnPageNumber
    )

/*++

Routine Description:

    This routine returns to its caller a mask of dirty sectors within a page,
    and atomically clear the bits.

Arguments:

    Vmcb - Supplies the Vmcb being manipulated

    LbnPageNumber - Supplies the Page Number (Lbn based) of page being
        modified.  For example, with a page size of 8 a page number of 0
        corresponds to LBN values 0 through 7, a page number of 1 corresponds
        to 8 through 15, and so on.

Return Value:

    ULONG - Receives a mask of dirty sectors within the specified page.
        (a 1 bit indicates that the sector is dirty).

--*/

{
    DIRTY_PAGE Key;
    PDIRTY_PAGE Entry;
    ULONG Mask;

    PAGED_CODE();

    DebugTrace(+1, Dbg, ( "UdfGetAndCleanDirtyVmcb\n", 0 ) );
    DebugTrace( 0, Dbg, ( " LbnPageNumber = %08x\n", LbnPageNumber ) );

    Key.LbnPageNumber = LbnPageNumber;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        //
        //  Locate the dirty page within the dirty table
        //

        if ((Entry = RtlLookupElementGenericTable( &Vmcb->DirtyTable,
                                                   &Key )) == NULL) {

            DebugTrace(0, Dbg, ( "Entry not found\n", 0 ) );

            try_leave( Mask = 0 );
        }

        //
        //  We found a page so generate a proper mask and then
        //  delete the dirty page
        //

        Mask = Entry->DirtyMask & (SECTOR_MASK); //**** change to manifest constant

        (VOID) RtlDeleteElementGenericTable( &Vmcb->DirtyTable, &Key );

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugTrace(-1, Dbg, ( "UdfGetAndCleanDirtyVmcb -> %08x\n", Mask ) );
    }

    return Mask;
}
#endif // VMCB_WRITE_SUPPORT


//
//  Local support routines
//

#if VMCB_WRITE_SUPPORT
RTL_GENERIC_COMPARE_RESULTS
PbCompareDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )

/*++

Routine Description:

    This generic table support routine compares two dirty page structures

Arguments:

    DirtyTable - Supplies the generic table being queried

    FirstStruct - Really supplies the first structure to compare

    SecondStruct - Really supplies the second structure to compare

Return Value:

    RTL_GENERIDC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{

    PDIRTY_PAGE DirtyPage1 = FirstStruct;
    PDIRTY_PAGE DirtyPage2 = SecondStruct;

    UNREFERENCED_PARAMETER( DirtyTable );

    PAGED_CODE();

    if (DirtyPage1->LbnPageNumber < DirtyPage2->LbnPageNumber) {

        return GenericLessThan;

    } else if (DirtyPage1->LbnPageNumber > DirtyPage2->LbnPageNumber) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }
}
#endif // VMCB_WRITE_SUPPORT


//
//  Local support routines
//

#if VMCB_WRITE_SUPPORT
PVOID
PbAllocateDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN CLONG ByteSize
    )

/*++

Routine Description:

    This generic table support routine allocates memory

Arguments:

    DirtyTable - Supplies the generic table being modified

    ByteSize - Supplies the size, in bytes, to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    PAGED_CODE();

    return FsRtlAllocatePoolWithTag( (POOL_TYPE)DirtyTable->TableContext, ByteSize, 'bcmV' );
}
#endif // VMCB_WRITE_SUPPORT


//
//  Local support routines
//

#if VMCB_WRITE_SUPPORT
VOID
PbDeallocateDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This generic table support routine deallocates memory

Arguments:

    DirtyTable - Supplies the generic table being modified

    Buffer - Supplies the buffer being deallocated

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( DirtyTable );

    PAGED_CODE();

    ExFreePool( Buffer );

    return;
}
#endif // VMCB_WRITE_SUPPORT


//
//  Local support routines
//

#if VMCB_WRITE_SUPPORT
ULONG
PbDumpDirtyVmcb (
    IN PVMCB Vmcb
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PDIRTY_PAGE Ptr;

    PAGED_CODE();

    KdPrint((" Dump Dirty Vmcb\n"));

    for (Ptr = RtlEnumerateGenericTable( &Vmcb->DirtyTable, TRUE );
         Ptr != NULL;
         Ptr = RtlEnumerateGenericTable( &Vmcb->DirtyTable, FALSE )) {

        KdPrint(("        LbnPageNumber = %08x, ", Ptr->LbnPageNumber ));
        KdPrint(("DirtyMask = %08x\n", Ptr->DirtyMask ));
    }

    return 0;
}
#endif // VMCB_WRITE_SUPPORT


#if VMCB_WRITE_SUPPORT
NTSTATUS
PbFlushVolumeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    The function carefully flushes the entire volume file.  It is nessecary
    to dance around a bit because of complicated synchronization reasons.

Arguments:

    Vcb - Supplies the Vcb being flushed

Return Value:

    NTSTATUS - The status of the flush operation

--*/

{
    ULONG ElementNumber;
    ULONG NumberOfDirtyPages;
    PULONG VbnsToFlush;

    LBN Lbn;
    PDIRTY_PAGE Ptr;

    NTSTATUS ReturnStatus = STATUS_SUCCESS;

    PVMCB Vmcb = (PNONOPAQUE_VMCB)&Vcb->Vmcb;

    //
    //  The only way we have to correctly synchronize things is to
    //  repin stuff, and then unpin repin it with WriteThrough as TRUE.
    //
    //  Grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    NumberOfDirtyPages = RtlNumberGenericTableElements(&Vmcb->DirtyTable);

    //
    //  If there are no dirty sectors, no need to flush.
    //

    if (NumberOfDirtyPages == 0) {

        (VOID)KeReleaseMutex( &Vmcb->Mutex, FALSE );
        return STATUS_SUCCESS;
    }

    try {

        VbnsToFlush = FsRtlAllocatePoolWithTag( PagedPool, NumberOfDirtyPages * sizeof(ULONG), 'bcmV' );

    } finally {

        if (AbnormalTermination()) {

            (VOID)KeReleaseMutex( &Vmcb->Mutex, FALSE );
        }
    }

    for (Ptr = RtlEnumerateGenericTable( &Vmcb->DirtyTable, TRUE ),
         ElementNumber = 0;
         Ptr != NULL;
         Ptr = RtlEnumerateGenericTable( &Vmcb->DirtyTable, FALSE ),
         ElementNumber += 1) {

        VBN Vbn;
        BOOLEAN Result;

        //
        //  Lbn pages always map to Vbn pages.  Thus any sector in an Lbn
        //  page will map to the same Vbn page.  So it suffices to map the
        //  first Lbn in the page to a Vbn and flush that page.
        //

        Lbn = Ptr->LbnPageNumber * (PAGE_SIZE / 512);

        ASSERT(Ptr->DirtyMask != 0);

        Result = PbVmcbLbnToVbn( &Vcb->Vmcb, Lbn, &Vbn, NULL );

        //
        //  This lookup must work as the LBN page was dirty.
        //

        if (!Result) {

            PbBugCheck( 0, 0, 0 );
        }

        //
        //  Bring store this Vbn away for flushing later.
        //

        ASSERT( ElementNumber < NumberOfDirtyPages );
        ASSERT( (Vbn & (PAGE_SIZE/512 - 1)) == 0 );

        VbnsToFlush[ElementNumber] = Vbn;
    }

    ASSERT( ElementNumber == NumberOfDirtyPages );

    //
    //  Now drop the mutex and walk through the dirty Vbn list generated
    //  above.  We cannot hold the mutex while doing IO as this will cause
    //  a deadlock with the cache manager.
    //

    (VOID)KeReleaseMutex( &Vmcb->Mutex, FALSE );

    for ( ElementNumber = 0;
          ElementNumber < NumberOfDirtyPages;
          ElementNumber += 1) {

        PBCB Bcb;
        PVOID DontCare;
        LARGE_INTEGER Offset;
        IO_STATUS_BLOCK Iosb;

        //
        //  This page is dirty.  Flush it by writing it though.
        //

        Offset.QuadPart = VbnsToFlush[ElementNumber] << 9;

        try {

            (VOID)CcPinRead( Vcb->VirtualVolumeFile,
                             &Offset,
                             PAGE_SIZE,
                             TRUE,
                             &Bcb,
                             &DontCare );

            CcSetDirtyPinnedData( Bcb, NULL );
            CcRepinBcb( Bcb );
            CcUnpinData( Bcb );
            CcUnpinRepinnedBcb( Bcb, TRUE, &Iosb );

            if (!NT_SUCCESS(Iosb.Status)) {

                ReturnStatus = Iosb.Status;
            }

        } except(PbExceptionFilter(IrpContext, GetExceptionInformation())) {

            ReturnStatus = IrpContext->ExceptionStatus;
        }
    }

    ExFreePool( VbnsToFlush );

    return ReturnStatus;
}
#endif // VMCB_WRITE_SUPPORT

