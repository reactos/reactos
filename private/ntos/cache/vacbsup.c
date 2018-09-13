/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    vacbsup.c

Abstract:

    This module implements the support routines for the Virtual Address
    Control Block support for the Cache Manager.  These routines are used
    to manage a large number of relatively small address windows to map
    file data for all forms of cache access.

Author:

    Tom Miller      [TomM]      8-Feb-1992

Revision History:

--*/

#include "cc.h"

//
//  Define our debug constant
//

#define me 0x000000040

//
//  Internal Support Routines.
//

VOID
CcUnmapVacb (
    IN PVACB Vacb,
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN BOOLEAN UnmapBehind
    );

PVACB
CcGetVacbMiss (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset,
    IN OUT PKIRQL OldIrql
    );

VOID
CcCalculateVacbLevelLockCount (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVACB *VacbArray,
    IN ULONG Level
    );

PVACB
CcGetVacbLargeOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset
    );

VOID
CcSetVacbLargeOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset,
    IN PVACB Vacb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, CcInitializeVacbs)
#endif

//
//  Define a few macros for manipulating the Vacb array.
//

#define GetVacb(SCM,OFF) (                                                                \
    ((SCM)->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL) ?                            \
    CcGetVacbLargeOffset((SCM),(OFF).QuadPart) :                                          \
    (SCM)->Vacbs[(OFF).LowPart >> VACB_OFFSET_SHIFT]                                      \
)

_inline
VOID
SetVacb (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER Offset,
    IN PVACB Vacb
    )
{
    if (SharedCacheMap->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL) {
        CcSetVacbLargeOffset(SharedCacheMap, Offset.QuadPart, Vacb);
#ifdef VACB_DBG
        ASSERT(Vacb >= VACB_SPECIAL_FIRST_VALID || CcGetVacbLargeOffset(SharedCacheMap, Offset.QuadPart) == Vacb);
#endif // VACB_DBG
    } else if (Vacb < VACB_SPECIAL_FIRST_VALID) {
        SharedCacheMap->Vacbs[Offset.LowPart >> VACB_OFFSET_SHIFT] = Vacb;
    }
#ifdef VACB_DBG
    //
    //  Note, we need a new field if we turn this check on again - ReservedForAlignment
    //  has been stolen for other purposes.
    //

    if (Vacb < VACB_SPECIAL_FIRST_VALID) {
        if (Vacb != NULL) {
            SharedCacheMap->ReservedForAlignment++;
        } else {
            SharedCacheMap->ReservedForAlignment--;
        }
    }
    ASSERT((SharedCacheMap->SectionSize.QuadPart <= VACB_SIZE_OF_FIRST_LEVEL) ||
           (SharedCacheMap->ReservedForAlignment == 0) ||
           IsVacbLevelReferenced( SharedCacheMap, SharedCacheMap->Vacbs, 1 ));
#endif // VACB_DBG
}

//
//  Define the macro for referencing the multilevel Vacb array.
//

_inline
VOID
ReferenceVacbLevel (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVACB *VacbArray,
    IN ULONG Level,
    IN LONG Amount,
    IN BOOLEAN Special
    )
{
    PVACB_LEVEL_REFERENCE VacbReference = VacbLevelReference( SharedCacheMap, VacbArray, Level );

    ASSERT( Amount > 0 ||
            (!Special && VacbReference->Reference >= (0 - Amount)) ||
            ( Special && VacbReference->SpecialReference >= (0 - Amount)));

    if (Special) {
        VacbReference->SpecialReference += Amount;
    } else {
        VacbReference->Reference += Amount;
    }

#ifdef VACB_DBG
    //
    //  For debugging purposes, we can assert that the regular reference count
    //  corresponds to the population of the level.
    //

    {
        LONG Current = VacbReference->Reference;
        CcCalculateVacbLevelLockCount( SharedCacheMap, VacbArray, Level );
        ASSERT( Current == VacbReference->Reference );
    }
#endif // VACB_DBG
}

//
//  Define the macros for moving the VACBs on the LRU list
//

#define CcMoveVacbToReuseHead(V)        RemoveEntryList( &(V)->LruList );                 \
                                        InsertHeadList( &CcVacbLru, &(V)->LruList );

#define CcMoveVacbToReuseTail(V)        RemoveEntryList( &(V)->LruList );                 \
                                        InsertTailList( &CcVacbLru, &(V)->LruList );

//
//  If the HighPart is nonzero, then we will go to a multi-level structure anyway, which is
//  most easily triggered by returning MAXULONG.
//

#define SizeOfVacbArray(LSZ) (                                                            \
    ((LSZ).HighPart != 0) ? MAXULONG :                                                    \
    ((LSZ).LowPart > (PREALLOCATED_VACBS * VACB_MAPPING_GRANULARITY) ?                    \
     (((LSZ).LowPart >> VACB_OFFSET_SHIFT) * sizeof(PVACB)) :                             \
     (PREALLOCATED_VACBS * sizeof(PVACB)))                                                \
)

#define CheckedDec(N) {  \
    ASSERT((N) != 0);    \
    (N) -= 1;            \
}


VOID
CcInitializeVacbs(
)

/*++

Routine Description:

    This routine must be called during Cache Manager initialization to
    initialize the Virtual Address Control Block structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG VacbBytes;
    PVACB NextVacb;

    CcNumberVacbs = (MmSizeOfSystemCacheInPages >> (VACB_OFFSET_SHIFT - PAGE_SHIFT)) - 2;
    VacbBytes = CcNumberVacbs * sizeof(VACB);

    KeInitializeSpinLock( &CcVacbSpinLock );
    CcVacbs = (PVACB)FsRtlAllocatePoolWithTag( NonPagedPool, VacbBytes, 'aVcC' );
    CcBeyondVacbs = (PVACB)((PCHAR)CcVacbs + VacbBytes);
    RtlZeroMemory( CcVacbs, VacbBytes );

    InitializeListHead( &CcVacbLru );

    for (NextVacb = CcVacbs; NextVacb < CcBeyondVacbs; NextVacb++) {

        InsertTailList( &CcVacbLru, &NextVacb->LruList );
    }
}


PVOID
CcGetVirtualAddressIfMapped (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset,
    OUT PVACB *Vacb,
    OUT PULONG ReceivedLength
    )

/*++

Routine Description:

    This routine returns a virtual address for the specified FileOffset,
    iff it is mapped.  Otherwise, it informs the caller that the specified
    virtual address was not mapped.  In the latter case, it still returns
    a ReceivedLength, which may be used to advance to the next view boundary.

Arguments:

    SharedCacheMap - Supplies a pointer to the Shared Cache Map for the file.

    FileOffset - Supplies the desired FileOffset within the file.

    Vach - Returns a Vacb pointer which must be supplied later to free
           this virtual address, or NULL if not mapped.

    ReceivedLength - Returns the number of bytes to the next view boundary,
                     whether the desired file offset is mapped or not.

Return Value:

    The virtual address at which the desired data is mapped, or NULL if it
    is not mapped.

--*/

{
    KIRQL OldIrql;
    ULONG VacbOffset = (ULONG)FileOffset & (VACB_MAPPING_GRANULARITY - 1);
    PVOID Value = NULL;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    //  Generate ReceivedLength return right away.
    //

    *ReceivedLength = VACB_MAPPING_GRANULARITY - VacbOffset;

    //
    //  Acquire the Vacb lock to see if the desired offset is already mapped.
    //

    CcAcquireVacbLock( &OldIrql );

    ASSERT( FileOffset <= SharedCacheMap->SectionSize.QuadPart );

    if ((*Vacb = GetVacb( SharedCacheMap, *(PLARGE_INTEGER)&FileOffset )) != NULL) {

        if ((*Vacb)->Overlay.ActiveCount == 0) {
            SharedCacheMap->VacbActiveCount += 1;
        }

        (*Vacb)->Overlay.ActiveCount += 1;

        //
        //  Move this range away from the front to avoid wasting cycles
        //  looking at it for reuse.
        //

        CcMoveVacbToReuseTail( *Vacb );

        Value = (PVOID)((PCHAR)(*Vacb)->BaseAddress + VacbOffset);
    }

    CcReleaseVacbLock( OldIrql );
    return Value;
}


PVOID
CcGetVirtualAddress (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset,
    OUT PVACB *Vacb,
    IN OUT PULONG ReceivedLength
    )

/*++

Routine Description:

    This is the main routine for Vacb management.  It may be called to acquire
    a virtual address for a given file offset.  If the desired file offset is
    already mapped, this routine does very little work before returning with
    the desired virtual address and Vacb pointer (which must be supplied to
    free the mapping).

    If the desired virtual address is not currently mapped, then this routine
    claims a Vacb from the tail of the Vacb LRU to reuse its mapping.  This Vacb
    is then unmapped if necessary (normally not required), and mapped to the
    desired address.

Arguments:

    SharedCacheMap - Supplies a pointer to the Shared Cache Map for the file.

    FileOffset - Supplies the desired FileOffset within the file.

    Vacb - Returns a Vacb pointer which must be supplied later to free
           this virtual address.

    ReceivedLength - Returns the number of bytes which are contiguously
                     mapped starting at the virtual address returned.

Return Value:

    The virtual address at which the desired data is mapped.

--*/

{
    KIRQL OldIrql;
    PVACB TempVacb;
    ULONG VacbOffset = FileOffset.LowPart & (VACB_MAPPING_GRANULARITY - 1);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    //  Acquire the Vacb lock to see if the desired offset is already mapped.
    //

    CcAcquireVacbLock( &OldIrql );

    ASSERT( FileOffset.QuadPart <= SharedCacheMap->SectionSize.QuadPart );

    if ((TempVacb = GetVacb( SharedCacheMap, FileOffset )) == NULL) {

        TempVacb = CcGetVacbMiss( SharedCacheMap, FileOffset, &OldIrql );

    } else {

        if (TempVacb->Overlay.ActiveCount == 0) {
            SharedCacheMap->VacbActiveCount += 1;
        }

        TempVacb->Overlay.ActiveCount += 1;
    }

    //
    //  Move this range away from the front to avoid wasting cycles
    //  looking at it for reuse.
    //

    CcMoveVacbToReuseTail( TempVacb );

    CcReleaseVacbLock( OldIrql );

    //
    //  Now form all outputs.
    //

    *Vacb = TempVacb;
    *ReceivedLength = VACB_MAPPING_GRANULARITY - VacbOffset;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return (PVOID)((PCHAR)TempVacb->BaseAddress + VacbOffset);
}


PVACB
CcGetVacbMiss (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset,
    IN OUT PKIRQL OldIrql
    )

/*++

Routine Description:

    This is the main routine for Vacb management.  It may be called to acquire
    a virtual address for a given file offset.  If the desired file offset is
    already mapped, this routine does very little work before returning with
    the desired virtual address and Vacb pointer (which must be supplied to
    free the mapping).

    If the desired virtual address is not currently mapped, then this routine
    claims a Vacb from the tail of the Vacb LRU to reuse its mapping.  This Vacb
    is then unmapped if necessary (normally not required), and mapped to the
    desired address.

Arguments:

    SharedCacheMap - Supplies a pointer to the Shared Cache Map for the file.

    FileOffset - Supplies the desired FileOffset within the file.

    OldIrql - Pointer to the OldIrql variable in the caller

Return Value:

    The Vacb.

--*/

{
    PSHARED_CACHE_MAP OldSharedCacheMap;
    PVACB Vacb, TempVacb;
    LARGE_INTEGER MappedLength;
    LARGE_INTEGER NormalOffset;
    NTSTATUS Status;
    ULONG ActivePage;
    ULONG PageIsDirty;
    PVACB ActiveVacb = NULL;
    ULONG VacbOffset = FileOffset.LowPart & (VACB_MAPPING_GRANULARITY - 1);

    NormalOffset = FileOffset;
    NormalOffset.LowPart -= VacbOffset;

    //
    //  For files that are not open for random access, we assume sequential
    //  access and periodically unmap unused views behind us as we go, to
    //  keep from hogging memory.
    //
    //  We used to only do this for pure FO_SEQUENTIAL_ONLY access.  The
    //  sequential flags still has an effect (to put the pages at the front
    //  of the standby lists) but we intend for the majority of the file
    //  cache to live on the standby and are willing to take transition
    //  faults to bring it back.  Granted, this exacerbates the problem that
    //  it is hard to figure out how big the filecache really is since even
    //  less of it is going to be mapped at any given time.  It may also
    //  promote the synchronization bottlenecks in view mapping (MmPfnLock)
    //  to the forefront when significant view thrashing occurs.
    //
    //  This isn't as bad as it seems.  When we see access take a view miss,
    //  it is really likely that it is a result of sequential access.  As long
    //  as the pages go onto the back of the standby, they'll live for a while.
    //  The problem we're dealing with here is that the cache can be filled at
    //  high speed, but the working set manager can't possibly trim it as fast,
    //  intelligently, while we have a pretty good guess where the candidate
    //  pages should come from.  We can't let the filecache size make large
    //  excursions, or we'll kick out a lot of valuable pages in the process.
    //

    if (!FlagOn(SharedCacheMap->Flags, RANDOM_ACCESS_SEEN) &&
        ((NormalOffset.LowPart & (SEQUENTIAL_MAP_LIMIT - 1)) == 0) &&
        (NormalOffset.QuadPart >= (SEQUENTIAL_MAP_LIMIT * 2))) {

        //
        //  Use MappedLength as a scratch variable to form the offset
        //  to start unmapping.  We are not synchronized with these past
        //  views, so it is possible that CcUnmapVacbArray will kick out
        //  early when it sees an active view.  That is why we go back
        //  twice the distance, and effectively try to unmap everything
        //  twice.  The second time should normally do it.  If the file
        //  is truly sequential only, then the only collision expected
        //  might be the previous view if we are being called from readahead,
        //  or there is a small chance that we can collide with the
        //  Lazy Writer during the small window where he briefly maps
        //  the file to push out the dirty bits.
        //

        CcReleaseVacbLock( *OldIrql );
        MappedLength.QuadPart = NormalOffset.QuadPart - (SEQUENTIAL_MAP_LIMIT * 2);
        CcUnmapVacbArray( SharedCacheMap, &MappedLength, (SEQUENTIAL_MAP_LIMIT * 2), TRUE );
        CcAcquireVacbLock( OldIrql );
    }

    //
    //  Scan from the front of the lru for the next victim Vacb
    //

    Vacb = CONTAINING_RECORD( CcVacbLru.Flink, VACB, LruList );

    while (TRUE) {

        //
        //  If this guy is not active, break out and use him.  Also, if
        //  it is an Active Vacb, nuke it now, because the reader may be idle and we
        //  want to clean up.
        //

        OldSharedCacheMap = Vacb->SharedCacheMap;
        if ((Vacb->Overlay.ActiveCount == 0) ||
            ((ActiveVacb == NULL) &&
             (OldSharedCacheMap != NULL) &&
             (OldSharedCacheMap->ActiveVacb == Vacb))) {

            //
            //  The normal case is that the Vacb is no longer mapped
            //  and we can just get out and use it, however, here we
            //  handle the case where it is mapped.
            //

            if (Vacb->BaseAddress != NULL) {


                //
                //  If this Vacb is active, it must be the ActiveVacb.
                //

                if (Vacb->Overlay.ActiveCount != 0) {

                    //
                    //  Get the active Vacb.
                    //

                    GetActiveVacbAtDpcLevel( Vacb->SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );

                //
                //  Otherwise we will break out and use this Vacb.  If it
                //  is still mapped we can now safely increment the open
                //  count.
                //

                } else {

                    //
                    //  Note that if the SharedCacheMap is currently
                    //  being deleted, we need to skip over
                    //  it, otherwise we will become the second
                    //  deleter.  CcDeleteSharedCacheMap clears the
                    //  pointer in the SectionObjectPointer.
                    //

                    CcAcquireMasterLockAtDpcLevel();
                    if (Vacb->SharedCacheMap->FileObject->SectionObjectPointer->SharedCacheMap ==
                        Vacb->SharedCacheMap) {

                        CcIncrementOpenCount( Vacb->SharedCacheMap, 'mvGS' );
                        CcReleaseMasterLockFromDpcLevel();
                        break;
                    }
                    CcReleaseMasterLockFromDpcLevel();
                }
            } else {
                break;
            }
        }

        //
        //  Advance to the next guy if we haven't scanned
        //  the entire list.
        //

        if (Vacb->LruList.Flink != &CcVacbLru) {

            Vacb = CONTAINING_RECORD( Vacb->LruList.Flink, VACB, LruList );

        } else {

            CcReleaseVacbLock( *OldIrql );

            //
            //  If we found an active vacb, then free it and go back and
            //  try again.  Else it's time to bail.
            //

            if (ActiveVacb != NULL) {
                CcFreeActiveVacb( ActiveVacb->SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );
                ActiveVacb = NULL;

                //
                //  Reacquire spinlocks to loop back and position ourselves at the head
                //  of the LRU for the next pass.
                //

                CcAcquireVacbLock( OldIrql );

                Vacb = CONTAINING_RECORD( CcVacbLru.Flink, VACB, LruList );

            } else {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }
        }
    }

    //
    //  Unlink it from the other SharedCacheMap, so the other
    //  guy will not try to use it when we free the spin lock.
    //

    if (Vacb->SharedCacheMap != NULL) {

        OldSharedCacheMap = Vacb->SharedCacheMap;
        SetVacb( OldSharedCacheMap, Vacb->Overlay.FileOffset, NULL );
        Vacb->SharedCacheMap = NULL;
    }

    //
    //  Mark it in use so no one else will muck with it after
    //  we release the spin lock.
    //

    Vacb->Overlay.ActiveCount = 1;
    SharedCacheMap->VacbActiveCount += 1;

    CcReleaseVacbLock( *OldIrql );

    //
    //  If the Vacb is already mapped, then unmap it.
    //

    if (Vacb->BaseAddress != NULL) {

        //
        //  Check to see if we need to drain the zone.
        //

        CcDrainVacbLevelZone();

        CcUnmapVacb( Vacb, OldSharedCacheMap, FALSE );

        //
        //  Now we can decrement the open count as we normally
        //  do, possibly deleting the guy.
        //

        CcAcquireMasterLock( OldIrql );

        //
        //  Now release our open count.
        //

        CcDecrementOpenCount( OldSharedCacheMap, 'mvGF' );

        if ((OldSharedCacheMap->OpenCount == 0) &&
            !FlagOn(OldSharedCacheMap->Flags, WRITE_QUEUED) &&
            (OldSharedCacheMap->DirtyPages == 0)) {

            //
            //  Move to the dirty list.
            //

            RemoveEntryList( &OldSharedCacheMap->SharedCacheMapLinks );
            InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                            &OldSharedCacheMap->SharedCacheMapLinks );

            //
            //  Make sure the Lazy Writer will wake up, because we
            //  want him to delete this SharedCacheMap.
            //

            LazyWriter.OtherWork = TRUE;
            if (!LazyWriter.ScanActive) {
                CcScheduleLazyWriteScan();
            }
        }

        CcReleaseMasterLock( *OldIrql );
    }

    //
    //  Use try-finally to return this guy to the list if we get an
    //  exception.
    //

    try {

        //
        //  Assume we are mapping to the end of the section, but
        //  reduce to our normal mapping granularity if the section
        //  is too large.
        //

        MappedLength.QuadPart = SharedCacheMap->SectionSize.QuadPart - NormalOffset.QuadPart;

        if ((MappedLength.HighPart != 0) ||
            (MappedLength.LowPart > VACB_MAPPING_GRANULARITY)) {

            MappedLength.LowPart = VACB_MAPPING_GRANULARITY;
        }

        //
        //  Now map this one in the system cache.
        //

        DebugTrace( 0, mm, "MmMapViewInSystemCache:\n", 0 );
        DebugTrace( 0, mm, "    Section = %08lx\n", SharedCacheMap->Section );
        DebugTrace2(0, mm, "    Offset = %08lx, %08lx\n",
                                NormalOffset.LowPart,
                                NormalOffset.HighPart );
        DebugTrace( 0, mm, "    ViewSize = %08lx\n", MappedLength.LowPart );

        Status =
          MmMapViewInSystemCache( SharedCacheMap->Section,
                                  &Vacb->BaseAddress,
                                  &NormalOffset,
                                  &MappedLength.LowPart );

        DebugTrace( 0, mm, "    <BaseAddress = %08lx\n", Vacb->BaseAddress );
        DebugTrace( 0, mm, "    <ViewSize = %08lx\n", MappedLength.LowPart );

        if (!NT_SUCCESS( Status )) {

            DebugTrace( 0, 0, "Error from Map, Status = %08lx\n", Status );

            ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                   STATUS_UNEXPECTED_MM_MAP_ERROR ));
        }

    } finally {

        //
        //  Take this opportunity to free the active vacb.
        //

        if (ActiveVacb != NULL) {

            CcFreeActiveVacb( ActiveVacb->SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );
        }

        //
        //  On abnormal termination, get this guy back in the list.
        //

        if (AbnormalTermination()) {

            CcAcquireVacbLock( OldIrql );

            //
            //  This is like the unlucky case below.  Just back out the stuff
            //  we did and put the guy at the tail of the list.  Basically
            //  only the Map should fail, and we clear BaseAddress accordingly.
            //

            Vacb->BaseAddress = NULL;

            CheckedDec(Vacb->Overlay.ActiveCount);
            CheckedDec(SharedCacheMap->VacbActiveCount);

            //
            //  If there is someone waiting for this count to go to zero,
            //  wake them here.
            //

            if (SharedCacheMap->WaitOnActiveCount != NULL) {
                KeSetEvent( SharedCacheMap->WaitOnActiveCount, 0, FALSE );
            }

            CcReleaseVacbLock( *OldIrql );
        }
    }

    //
    //  Make sure the zone contains the worst case number of entries.
    //

    if (SharedCacheMap->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL) {

        //
        //  Raise if we cannot preallocate enough buffers.
        //

        if (!CcPrefillVacbLevelZone( CcMaxVacbLevelsSeen - 1,
                                     OldIrql,
                                     FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED) )) {

            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

    } else {

        CcAcquireVacbLock( OldIrql );
    }

    //
    //  Finish filling in the Vacb, and store its address in the array in
    //  the Shared Cache Map.  (We have to rewrite the ActiveCount
    //  since it is overlaid.)  To do this we must reacquire the
    //  spin lock one more time.  Note we have to check for the unusual
    //  case that someone beat us to mapping this view, since we had to
    //  drop the spin lock.
    //

    if ((TempVacb = GetVacb( SharedCacheMap, NormalOffset )) == NULL) {

        Vacb->SharedCacheMap = SharedCacheMap;
        Vacb->Overlay.FileOffset = NormalOffset;
        Vacb->Overlay.ActiveCount = 1;

        SetVacb( SharedCacheMap, NormalOffset, Vacb );

    //
    //  This is the unlucky case where we collided with someone else
    //  trying to map the same view.  He can get in because we dropped
    //  the spin lock above.  Rather than allocating events and making
    //  someone wait, considering this case is fairly unlikely, we just
    //  dump this one at the head of the LRU and use the one from the
    //  guy who beat us.
    //

    } else {

        //
        //  Now we have to increment all of the counts for the one that
        //  was already there, then ditch the one we had.
        //

        if (TempVacb->Overlay.ActiveCount == 0) {
            SharedCacheMap->VacbActiveCount += 1;
        }

        TempVacb->Overlay.ActiveCount += 1;

        //
        //  Now unmap the one we mapped and proceed with the other Vacb.
        //  On this path we have to release the spinlock to do the unmap,
        //  and then reacquire the spinlock before cleaning up.
        //

        CcReleaseVacbLock( *OldIrql );

        CcUnmapVacb( Vacb, SharedCacheMap, FALSE );

        CcAcquireVacbLock( OldIrql );
        CheckedDec(Vacb->Overlay.ActiveCount);
        CheckedDec(SharedCacheMap->VacbActiveCount);
        Vacb->SharedCacheMap = NULL;

        CcMoveVacbToReuseHead( Vacb );

        Vacb = TempVacb;
    }

    return Vacb;
}


VOID
FASTCALL
CcFreeVirtualAddress (
    IN PVACB Vacb
    )

/*++

Routine Description:

    This routine must be called once for each call to CcGetVirtualAddress
    to free that virtual address.

Arguments:

    Vacb - Supplies the Vacb which was returned from CcGetVirtualAddress.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PSHARED_CACHE_MAP SharedCacheMap = Vacb->SharedCacheMap;

    CcAcquireVacbLock( &OldIrql );

    CheckedDec(Vacb->Overlay.ActiveCount);

    //
    //  If the count goes to zero, then we want to decrement the global
    //  Active count.
    //

    if (Vacb->Overlay.ActiveCount == 0) {

        //
        //  If the SharedCacheMap address is not NULL, then this one is
        //  in use by a shared cache map, and we have to decrement his
        //  count and see if anyone is waiting.
        //

        if (SharedCacheMap != NULL) {

            CheckedDec(SharedCacheMap->VacbActiveCount);

            //
            //  If there is someone waiting for this count to go to zero,
            //  wake them here.
            //

            if (SharedCacheMap->WaitOnActiveCount != NULL) {
                KeSetEvent( SharedCacheMap->WaitOnActiveCount, 0, FALSE );
            }

            //
            //  Go to the back of the LRU to save this range for a bit
            //

            CcMoveVacbToReuseTail( Vacb );

        } else {

            //
            //  This range is no longer referenced, so make it avaliable
            //

            CcMoveVacbToReuseHead( Vacb );
        }

    } else {

        //
        //  This range is still in use, so move it away from the front
        //  so that it doesn't consume cycles being checked.
        //

        CcMoveVacbToReuseTail( Vacb );
    }

    CcReleaseVacbLock( OldIrql );
}


VOID
CcReferenceFileOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset
    )

/*++

Routine Description:

    This is a special form of reference that insures that the multi-level
    Vacb structures are expanded to cover a given file offset.

Arguments:

    SharedCacheMap - Supplies a pointer to the Shared Cache Map for the file.

    FileOffset - Supplies the desired FileOffset within the file.

Return Value:

    None

--*/

{
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    //  This operation only has meaning if the Vacbs are in the multilevel form.
    //

    if (SharedCacheMap->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL) {

        //
        //  Prefill the level zone so that we can expand the tree if required.
        //

        if (!CcPrefillVacbLevelZone( CcMaxVacbLevelsSeen - 1,
                                     &OldIrql,
                                     FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED) )) {

            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        ASSERT( FileOffset.QuadPart <= SharedCacheMap->SectionSize.QuadPart );

        SetVacb( SharedCacheMap, FileOffset, VACB_SPECIAL_REFERENCE );

        CcReleaseVacbLock( OldIrql );
    }

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return;
}


VOID
CcDereferenceFileOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER FileOffset
    )

/*++

Routine Description:

    This routine must be called once for each call to CcReferenceFileOffset
    to remove the reference.

Arguments:

    SharedCacheMap - Supplies a pointer to the Shared Cache Map for the file.

    FileOffset - Supplies the desired FileOffset within the file.

Return Value:

    None

--*/

{
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    //  This operation only has meaning if the Vacbs are in the multilevel form.
    //

    if (SharedCacheMap->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL) {

        //
        //  Acquire the Vacb lock to synchronize the dereference.
        //

        CcAcquireVacbLock( &OldIrql );

        ASSERT( FileOffset.QuadPart <= SharedCacheMap->SectionSize.QuadPart );

        SetVacb( SharedCacheMap, FileOffset, VACB_SPECIAL_DEREFERENCE );

        CcReleaseVacbLock( OldIrql );
    }

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return;
}


VOID
CcWaitOnActiveCount (
    IN PSHARED_CACHE_MAP SharedCacheMap
    )

/*++

Routine Description:

    This routine may be called to wait for outstanding mappings for
    a given SharedCacheMap to go inactive.  It is intended to be called
    from CcUninitializeCacheMap, which is called by the file systems
    during cleanup processing.  In that case this routine only has to
    wait if the user closed a handle without waiting for all I/Os on the
    handle to complete.

    This routine returns each time the active count is decremented.  The
    caller must recheck his wait conditions on return, either waiting for
    the ActiveCount to go to 0, or for specific views to go inactive
    (CcPurgeCacheSection case).

Arguments:

    SharedCacheMap - Supplies the Shared Cache Map on whose VacbActiveCount
                     we wish to wait.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PKEVENT Event;

    //
    //  In the unusual case that we get a cleanup while I/O is still going
    //  on, we can wait here.  The caller must test the count for nonzero
    //  before calling this routine.
    //
    //  Since we are being called from cleanup, we cannot afford to
    //  fail here.
    //

    CcAcquireVacbLock( &OldIrql );

    //
    //  It is possible that the count went to zero before we acquired the
    //  spinlock, so we must handle two cases here.
    //

    if (SharedCacheMap->VacbActiveCount != 0) {

        if ((Event = SharedCacheMap->WaitOnActiveCount) == NULL) {

            //
            //  If the local event is not being used then we take it.
            //

            Event = InterlockedExchangePointer( &SharedCacheMap->LocalEvent, NULL );

            if (Event == NULL) {

                Event = (PKEVENT)ExAllocatePoolWithTag( NonPagedPoolMustSucceed,
                                                        sizeof(KEVENT),
                                                        'vEcC' );
            }
        }

        KeInitializeEvent( Event,
                           NotificationEvent,
                           FALSE );

        SharedCacheMap->WaitOnActiveCount = Event;

        CcReleaseVacbLock( OldIrql );

        KeWaitForSingleObject( Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);
    } else {

        CcReleaseVacbLock( OldIrql );
    }
}


//
//  Internal Support Routine.
//

VOID
CcUnmapVacb (
    IN PVACB Vacb,
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN BOOLEAN UnmapBehind
    )

/*++

Routine Description:

    This routine may be called to unmap a previously mapped Vacb, and
    clear its BaseAddress field.

Arguments:

    Vacb - Supplies the Vacb which was returned from CcGetVirtualAddress.

    UnmapBehind - If this is a result of our unmap behind logic (the
        only case in which we pay attention to sequential hints)

Return Value:

    None.

--*/

{
    //
    //  Make sure it is mapped.
    //

    ASSERT(SharedCacheMap != NULL);
    ASSERT(Vacb->BaseAddress != NULL);

    //
    //  Call MM to unmap it.
    //

    DebugTrace( 0, mm, "MmUnmapViewInSystemCache:\n", 0 );
    DebugTrace( 0, mm, "    BaseAddress = %08lx\n", Vacb->BaseAddress );

    MmUnmapViewInSystemCache( Vacb->BaseAddress,
                              SharedCacheMap->Section,
                              UnmapBehind &&
                              FlagOn(SharedCacheMap->Flags, ONLY_SEQUENTIAL_ONLY_SEEN) );

    Vacb->BaseAddress = NULL;
}


VOID
FASTCALL
CcCreateVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER NewSectionSize
    )

/*++

Routine Description:

    This routine must be called when a SharedCacheMap is created to create
    and initialize the initial Vacb array.

Arguments:

    SharedCacheMap - Supplies the shared cache map for which the array is
                     to be created.

    NewSectionSize - Supplies the current size of the section which must be
                     covered by the Vacb array.

Return Value:

    None.

--*/

{
    PVACB *NewAddresses;
    ULONG NewSize, SizeToAllocate;
    PLIST_ENTRY BcbListHead;
    BOOLEAN CreateBcbListHeads = FALSE, CreateReference = FALSE;

    NewSize = SizeToAllocate = SizeOfVacbArray(NewSectionSize);

    //
    //  The following limit is greater than the MM limit
    //  (i.e., MM actually only supports even smaller sections).
    //  We have to reject the sign bit, and testing the high byte
    //  for nonzero will surely only catch errors.
    //

    if (NewSectionSize.HighPart & ~(PAGE_SIZE - 1)) {
        ExRaiseStatus(STATUS_SECTION_TOO_BIG);
    }

    //
    //  See if we can use the array inside the shared cache map.
    //

    if (NewSize == (PREALLOCATED_VACBS * sizeof(PVACB))) {

        NewAddresses = &SharedCacheMap->InitialVacbs[0];

    //
    //  Else allocate the array.
    //

    } else {

        //
        //  For large metadata streams, double the size to allocate
        //  an array of Bcb listheads.  Each two Vacb pointers also
        //  gets its own Bcb listhead, thus requiring double the size.
        //

        ASSERT(SIZE_PER_BCB_LIST == (VACB_MAPPING_GRANULARITY * 2));

        //
        //  If this stream is larger than the size for multi-level Vacbs,
        //  then fix the size to allocate the root.
        //

        if (NewSize > VACB_LEVEL_BLOCK_SIZE) {

            ULONG Level = 0;
            ULONG Shift = VACB_OFFSET_SHIFT + VACB_LEVEL_SHIFT;

            NewSize = SizeToAllocate = VACB_LEVEL_BLOCK_SIZE;
            SizeToAllocate += sizeof(VACB_LEVEL_REFERENCE);
            CreateReference = TRUE;

            //
            //  Loop to calculate how many levels we have and how much we have to
            //  shift to index into the first level.
            //

            do {

                Level += 1;
                Shift += VACB_LEVEL_SHIFT;

            } while ((NewSectionSize.QuadPart > ((LONGLONG)1 << Shift)) != 0);

            //
            //  Remember the maximum level ever seen (which is actually Level + 1).
            //

            if (Level >= CcMaxVacbLevelsSeen) {
                ASSERT(Level <= VACB_NUMBER_OF_LEVELS);
                CcMaxVacbLevelsSeen = Level + 1;
            }

        } else {

            //
            //  Does this stream get a Bcb Listhead array?
            //

            if (FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED) &&
                (NewSectionSize.QuadPart > BEGIN_BCB_LIST_ARRAY)) {

                SizeToAllocate *= 2;
                CreateBcbListHeads = TRUE;
            }

            //
            //  Handle the boundary case by giving the proto-level a
            //  reference count.  This will allow us to simply push it
            //  in the expansion case.  In practice, due to pool granularity
            //  this will not change the amount of space allocated
            //

            if (NewSize == VACB_LEVEL_BLOCK_SIZE) {

                SizeToAllocate += sizeof(VACB_LEVEL_REFERENCE);
                CreateReference = TRUE;
            }
        }

        NewAddresses = ExAllocatePoolWithTag( NonPagedPool, SizeToAllocate, 'pVcC' );
        if (NewAddresses == NULL) {
            SharedCacheMap->Status = STATUS_INSUFFICIENT_RESOURCES;
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }
    }

    //
    //  Zero out the Vacb array and the trailing reference counts.
    //

    RtlZeroMemory( (PCHAR)NewAddresses, NewSize );

    if (CreateReference) {

        SizeToAllocate -= sizeof(VACB_LEVEL_REFERENCE);
        RtlZeroMemory( (PCHAR)NewAddresses + SizeToAllocate, sizeof(VACB_LEVEL_REFERENCE) );
    }

    //
    //  Loop to insert the Bcb listheads (if any) in the *descending* order
    //  Bcb list.
    //

    if (CreateBcbListHeads) {

        for (BcbListHead = (PLIST_ENTRY)((PCHAR)NewAddresses + NewSize);
             BcbListHead < (PLIST_ENTRY)((PCHAR)NewAddresses + SizeToAllocate);
             BcbListHead++) {

            InsertHeadList( &SharedCacheMap->BcbList, BcbListHead );
        }
    }

    SharedCacheMap->Vacbs = NewAddresses;
    SharedCacheMap->SectionSize = NewSectionSize;
}


VOID
CcExtendVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LARGE_INTEGER NewSectionSize
    )

/*++

Routine Description:

    This routine must be called any time the section for a shared cache
    map is extended, in order to extend the Vacb array (if necessary).

Arguments:

    SharedCacheMap - Supplies the shared cache map for which the array is
                     to be created.

    NewSectionSize - Supplies the new size of the section which must be
                     covered by the Vacb array.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PVACB *OldAddresses;
    PVACB *NewAddresses;
    ULONG OldSize;
    ULONG NewSize, SizeToAllocate;
    LARGE_INTEGER NextLevelSize;
    BOOLEAN GrowingBcbListHeads = FALSE, CreateReference = FALSE;

    //
    //  The following limit is greater than the MM limit
    //  (i.e., MM actually only supports even smaller sections).
    //  We have to reject the sign bit, and testing the high byte
    //  for nonzero will surely only catch errors.
    //

    if (NewSectionSize.HighPart & ~(PAGE_SIZE - 1)) {
        ExRaiseStatus(STATUS_SECTION_TOO_BIG);
    }

    //
    //  See if we will be growing the Bcb ListHeads, so we can take out the
    //  master lock if so.
    //

    if (FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED) &&
        (NewSectionSize.QuadPart > BEGIN_BCB_LIST_ARRAY)) {

        GrowingBcbListHeads = TRUE;
    }

    //
    //  Is there any work to do?
    //

    if (NewSectionSize.QuadPart > SharedCacheMap->SectionSize.QuadPart) {

        //
        //  Handle the growth of the first level here.
        //

        if (SharedCacheMap->SectionSize.QuadPart < VACB_SIZE_OF_FIRST_LEVEL) {

            NextLevelSize = NewSectionSize;

            //
            //  Limit the growth of this level
            //

            if (NextLevelSize.QuadPart >= VACB_SIZE_OF_FIRST_LEVEL) {
                NextLevelSize.QuadPart = VACB_SIZE_OF_FIRST_LEVEL;
                CreateReference = TRUE;
            }

            //
            //  N.B.: SizeOfVacbArray only calculates the size of the VACB
            //  pointer block.  We must adjust for Bcb listheads and the
            //  multilevel reference count.
            //

            NewSize = SizeToAllocate = SizeOfVacbArray(NextLevelSize);
            OldSize = SizeOfVacbArray(SharedCacheMap->SectionSize);

            //
            //  Only do something if the size is growing.
            //

            if (NewSize > OldSize) {

                //
                //  Does this stream get a Bcb Listhead array?
                //

                if (GrowingBcbListHeads) {
                    SizeToAllocate *= 2;
                }

                //
                //  Do we need space for the reference count?
                //

                if (CreateReference) {
                    SizeToAllocate += sizeof(VACB_LEVEL_REFERENCE);
                }

                NewAddresses = FsRtlAllocatePoolWithTag( NonPagedPool, SizeToAllocate, 'pVcC' );

                //
                //  See if we will be growing the Bcb ListHeads, so we can take out the
                //  master lock if so.
                //

                if (GrowingBcbListHeads) {

                    ExAcquireSpinLock( &SharedCacheMap->BcbSpinLock, &OldIrql );
                    CcAcquireVacbLockAtDpcLevel();

                } else {

                    //
                    //  Acquire the spin lock to serialize with anyone who might like
                    //  to "steal" one of the mappings we are going to move.
                    //

                    CcAcquireVacbLock( &OldIrql );
                }

                OldAddresses = SharedCacheMap->Vacbs;
                if (OldAddresses != NULL) {
                    RtlCopyMemory( NewAddresses, OldAddresses, OldSize );
                } else {
                    OldSize = 0;
                }

                RtlZeroMemory( (PCHAR)NewAddresses + OldSize, NewSize - OldSize );

                if (CreateReference) {

                    SizeToAllocate -= sizeof(VACB_LEVEL_REFERENCE);
                    RtlZeroMemory( (PCHAR)NewAddresses + SizeToAllocate, sizeof(VACB_LEVEL_REFERENCE) );
                }

                //
                //  See if we have to initialize Bcb Listheads.
                //

                if (GrowingBcbListHeads) {

                    LARGE_INTEGER Offset;
                    PLIST_ENTRY BcbListHeadNew, TempEntry;

                    Offset.QuadPart = 0;
                    BcbListHeadNew = (PLIST_ENTRY)((PCHAR)NewAddresses + NewSize );

                    //
                    //  Handle case where the old array had Bcb Listheads.
                    //

                    if ((SharedCacheMap->SectionSize.QuadPart > BEGIN_BCB_LIST_ARRAY) &&
                        (OldAddresses != NULL)) {

                        PLIST_ENTRY BcbListHeadOld;

                        BcbListHeadOld = (PLIST_ENTRY)((PCHAR)OldAddresses + OldSize);

                        //
                        //  Loop to remove each old listhead and insert the new one
                        //  in its place.
                        //

                        do {
                            TempEntry = BcbListHeadOld->Flink;
                            RemoveEntryList( BcbListHeadOld );
                            InsertTailList( TempEntry, BcbListHeadNew );
                            Offset.QuadPart += SIZE_PER_BCB_LIST;
                            BcbListHeadOld += 1;
                            BcbListHeadNew += 1;
                        } while (Offset.QuadPart < SharedCacheMap->SectionSize.QuadPart);

                    //
                    //  Otherwise, handle the case where we are adding Bcb
                    //  Listheads.
                    //

                    } else {

                        TempEntry = SharedCacheMap->BcbList.Blink;

                        //
                        //  Loop through any/all Bcbs to insert the new listheads.
                        //

                        while (TempEntry != &SharedCacheMap->BcbList) {

                            //
                            //  Sit on this Bcb until we have inserted all listheads
                            //  that go before it.
                            //

                            while (Offset.QuadPart <= ((PBCB)CONTAINING_RECORD(TempEntry, BCB, BcbLinks))->FileOffset.QuadPart) {

                                InsertHeadList(TempEntry, BcbListHeadNew);
                                Offset.QuadPart += SIZE_PER_BCB_LIST;
                                BcbListHeadNew += 1;
                            }
                            TempEntry = TempEntry->Blink;
                        }
                    }

                    //
                    //  Now insert the rest of the new listhead entries that were
                    //  not finished in either loop above.
                    //

                    while (Offset.QuadPart < NextLevelSize.QuadPart) {

                        InsertHeadList(&SharedCacheMap->BcbList, BcbListHeadNew);
                        Offset.QuadPart += SIZE_PER_BCB_LIST;
                        BcbListHeadNew += 1;
                    }
                }

                //
                //  These two fields must be changed while still holding the spinlock.
                //

                SharedCacheMap->Vacbs = NewAddresses;
                SharedCacheMap->SectionSize = NextLevelSize;

                //
                //  Now we can free the spinlocks ahead of freeing pool.
                //

                if (GrowingBcbListHeads) {
                    CcReleaseVacbLockFromDpcLevel();
                    ExReleaseSpinLock( &SharedCacheMap->BcbSpinLock, OldIrql );
                } else {
                    CcReleaseVacbLock( OldIrql );
                }

                if ((OldAddresses != &SharedCacheMap->InitialVacbs[0]) &&
                    (OldAddresses != NULL)) {
                    ExFreePool( OldAddresses );
                }
            }

            //
            //  Make sure SectionSize gets updated.  It is ok to fall through here
            //  without a spinlock, so long as either Vacbs was not changed, or it
            //  was changed together with SectionSize under the spinlock(s) above.
            //

            SharedCacheMap->SectionSize = NextLevelSize;
        }

        //
        //  Handle extends up to and within multi-level Vacb arrays here.  This is fairly simple.
        //  If no additional Vacb levels are required, then there is no work to do, otherwise
        //  we just have to push the root one or more levels linked through the first pointer
        //  in the new root(s).
        //

        if (NewSectionSize.QuadPart > SharedCacheMap->SectionSize.QuadPart) {

            PVACB *NextVacbArray;
            ULONG NewLevel;
            ULONG Level = 1;
            ULONG Shift = VACB_OFFSET_SHIFT + VACB_LEVEL_SHIFT;

            //
            //  Loop to calculate how many levels we currently have.
            //

            while (SharedCacheMap->SectionSize.QuadPart > ((LONGLONG)1 << Shift)) {

                Level += 1;
                Shift += VACB_LEVEL_SHIFT;
            }

            NewLevel = Level;

            //
            //  Loop to calculate how many levels we need.
            //

            while (((NewSectionSize.QuadPart - 1) >> Shift) != 0) {

                NewLevel += 1;
                Shift += VACB_LEVEL_SHIFT;
            }

            //
            //  Now see if we have any work to do.
            //

            if (NewLevel > Level) {

                //
                //  Remember the maximum level ever seen (which is actually NewLevel + 1).
                //

                if (NewLevel >= CcMaxVacbLevelsSeen) {
                    ASSERT(NewLevel <= VACB_NUMBER_OF_LEVELS);
                    CcMaxVacbLevelsSeen = NewLevel + 1;
                }

                //
                //  Raise if we cannot preallocate enough buffers.
                //

                if (!CcPrefillVacbLevelZone( NewLevel - Level, &OldIrql, FALSE )) {

                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                //  Now if the current Level of the file is 1, we have not been maintaining
                //  a reference count, so we have to calculate it before pushing.  In the
                //  boundary case we have made sure that the reference space is avaliable.
                //

                if (Level == 1) {

                    //
                    //  We know this is always a leaf-like level right now.
                    //

                    CcCalculateVacbLevelLockCount( SharedCacheMap, SharedCacheMap->Vacbs, 0 );
                }

                //
                //  Finally, if there are any active pointers in the first level, then we
                //  have to create new levels by adding a new root enough times to create
                //  additional levels.  On the other hand, if the pointer count in the top
                //  level is zero, then we must not do any pushes, because we never allow
                //  empty leaves!
                //

                if (IsVacbLevelReferenced( SharedCacheMap, SharedCacheMap->Vacbs, Level - 1 )) {

                    while (NewLevel > Level++) {

                        ASSERT(CcVacbLevelEntries != 0);
                        NextVacbArray = CcAllocateVacbLevel(FALSE);

                        NextVacbArray[0] = (PVACB)SharedCacheMap->Vacbs;
                        ReferenceVacbLevel( SharedCacheMap, NextVacbArray, Level, 1, FALSE );

                        SharedCacheMap->Vacbs = NextVacbArray;
                    }

                } else {

                    //
                    //  We are now possesed of the additional problem that this level has no
                    //  references but may have Bcb listheads due to the boundary case where
                    //  we have expanded up to the multilevel Vacbs above.  This level can't
                    //  remain at the root and needs to be destroyed.  What we need to do is
                    //  replace it with one of our prefilled (non Bcb) levels and unlink the
                    //  Bcb listheads in the old one.
                    //

                    if (Level == 1 && FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED)) {

                        PLIST_ENTRY PredecessorListHead, SuccessorListHead;

                        NextVacbArray = SharedCacheMap->Vacbs;
                        SharedCacheMap->Vacbs = CcAllocateVacbLevel(FALSE);

                        PredecessorListHead = ((PLIST_ENTRY)((PCHAR)NextVacbArray + VACB_LEVEL_BLOCK_SIZE))->Flink;
                        SuccessorListHead = ((PLIST_ENTRY)((PCHAR)NextVacbArray + (VACB_LEVEL_BLOCK_SIZE * 2) - sizeof(LIST_ENTRY)))->Blink;
                        PredecessorListHead->Blink = SuccessorListHead;
                        SuccessorListHead->Flink = PredecessorListHead;

                        CcDeallocateVacbLevel( NextVacbArray, TRUE );
                    }
                }

                //
                //  These two fields (Vacbs and SectionSize) must be changed while still
                //  holding the spinlock.
                //

                SharedCacheMap->SectionSize = NewSectionSize;
                CcReleaseVacbLock( OldIrql );
            }

            //
            //  Make sure SectionSize gets updated.  It is ok to fall through here
            //  without a spinlock, so long as either Vacbs was not changed, or it
            //  was changed together with SectionSize under the spinlock(s) above.
            //

            SharedCacheMap->SectionSize = NewSectionSize;
        }
    }
}


BOOLEAN
FASTCALL
CcUnmapVacbArray (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset OPTIONAL,
    IN ULONG Length,
    IN BOOLEAN UnmapBehind
    )

/*++

Routine Description:

    This routine must be called to do any unmapping and associated
    cleanup for a shared cache map, just before it is deleted.

Arguments:

    SharedCacheMap - Supplies a pointer to the shared cache map
                     which is about to be deleted.

    FileOffset - If supplied, only unmap the specified offset and length

    Length - Completes range to unmap if FileOffset specified.  If FileOffset
             is specified, Length of 0 means unmap to the end of the section.

    UnmapBehind - If this is a result of our unmap behind logic

Return Value:

    FALSE -- if an the unmap was not done due to an active vacb
    TRUE -- if the unmap was done

--*/

{
    PVACB Vacb;
    KIRQL OldIrql;
    LARGE_INTEGER StartingFileOffset = {0,0};
    LARGE_INTEGER EndingFileOffset = SharedCacheMap->SectionSize;

    //
    //  We could be just cleaning up for error recovery.
    //

    if (SharedCacheMap->Vacbs == NULL) {
        return TRUE;
    }

    //
    //  See if a range was specified. Align it to the VACB boundaries so it
    //  works in the loop below  
    //

    if (ARGUMENT_PRESENT(FileOffset)) {
        StartingFileOffset.QuadPart = ((FileOffset->QuadPart) & (~((LONGLONG)VACB_MAPPING_GRANULARITY - 1)));
        if (Length != 0) {

            EndingFileOffset.QuadPart = FileOffset->QuadPart + Length;
                
        }
    }

    //
    //  Acquire the spin lock to
    //

    CcAcquireVacbLock( &OldIrql );

    while (StartingFileOffset.QuadPart < EndingFileOffset.QuadPart) {

        //
        //  Note that the caller with an explicit range may be off the
        //  end of the section (example CcPurgeCacheSection for cache
        //  coherency).  That is the reason for the first part of the
        //  test below.
        //
        //  Check the next cell once without the spin lock, it probably will
        //  not change, but we will handle it if it does not.
        //

        if ((StartingFileOffset.QuadPart < SharedCacheMap->SectionSize.QuadPart) &&
            ((Vacb = GetVacb( SharedCacheMap, StartingFileOffset )) != NULL)) {

            //
            //  Return here if we are unlucky and see an active
            //  Vacb.  It could be Purge calling, and the Lazy Writer
            //  may have done a CcGetVirtualAddressIfMapped!
            //

            if (Vacb->Overlay.ActiveCount != 0) {

                CcReleaseVacbLock( OldIrql );
                return FALSE;
            }

            //
            //  Unlink it from the other SharedCacheMap, so the other
            //  guy will not try to use it when we free the spin lock.
            //

            SetVacb( SharedCacheMap, StartingFileOffset, NULL );
            Vacb->SharedCacheMap = NULL;

            //
            //  Increment the open count so that no one else will
            //  try to unmap or reuse until we are done.
            //

            Vacb->Overlay.ActiveCount += 1;

            //
            //  Release the spin lock.
            //

            CcReleaseVacbLock( OldIrql );

            //
            //  Unmap and free it if we really got it above.
            //

            CcUnmapVacb( Vacb, SharedCacheMap, UnmapBehind );

            //
            //  Reacquire the spin lock so that we can decrment the count.
            //

            CcAcquireVacbLock( &OldIrql );
            Vacb->Overlay.ActiveCount -= 1;

            //
            //  Place this VACB at the head of the LRU
            //

            CcMoveVacbToReuseHead( Vacb );
        }

        StartingFileOffset.QuadPart = StartingFileOffset.QuadPart + VACB_MAPPING_GRANULARITY;
    }

    CcReleaseVacbLock( OldIrql );

    CcDrainVacbLevelZone();

    return TRUE;
}


ULONG
CcPrefillVacbLevelZone (
    IN ULONG NumberNeeded,
    OUT PKIRQL OldIrql,
    IN ULONG NeedBcbListHeads
    )

/*++

Routine Description:

    This routine may be called to prefill the VacbLevelZone with the number of
    entries required, and return with CcVacbSpinLock acquired.  This approach is
    taken so that the pool allocations and RtlZeroMemory calls can occur without
    holding any spinlock, yet the caller may proceed to peform a single indivisible
    operation without error handling, since there is a guaranteed minimum number of
    entries in the zone.

Arguments:

    NumberNeeded - Number of VacbLevel entries needed, not counting the possible
                   one with Bcb listheads.

    OldIrql = supplies a pointer to where OldIrql should be returned upon acquiring
              the spinlock.

    NeedBcbListHeads - Supplies true if a level is also needed which contains listheads.

Return Value:

    FALSE if the buffers could not be preallocated, TRUE otherwise.

Environment:

    No spinlocks should be held upon entry.

--*/

{
    PVACB *NextVacbArray;

    CcAcquireVacbLock( OldIrql );

    //
    //  Loop until there is enough entries, else raise...
    //

    while ((NumberNeeded > CcVacbLevelEntries) ||
           (NeedBcbListHeads && (CcVacbLevelWithBcbsFreeList == NULL))) {


        //
        //  Else release the spinlock so we can do the allocate/zero.
        //

        CcReleaseVacbLock( *OldIrql );

        //
        //  First handle the case where we need a VacbListHead with Bcb Listheads.
        //  The pointer test is unsafe but see below.
        //

        if (NeedBcbListHeads && (CcVacbLevelWithBcbsFreeList == NULL)) {

            //
            //  Allocate and initialize the Vacb block for this level, and store its pointer
            //  back into our parent.  We do not zero the listhead area.
            //

            NextVacbArray =
            (PVACB *)ExAllocatePoolWithTag( NonPagedPool, (VACB_LEVEL_BLOCK_SIZE * 2) + sizeof(VACB_LEVEL_REFERENCE), 'lVcC' );

            if (NextVacbArray == NULL) {
                return FALSE;
            }

            RtlZeroMemory( (PCHAR)NextVacbArray, VACB_LEVEL_BLOCK_SIZE );
            RtlZeroMemory( (PCHAR)NextVacbArray + (VACB_LEVEL_BLOCK_SIZE * 2), sizeof(VACB_LEVEL_REFERENCE) );

            CcAcquireVacbLock( OldIrql );

            NextVacbArray[0] = (PVACB)CcVacbLevelWithBcbsFreeList;
            CcVacbLevelWithBcbsFreeList = NextVacbArray;
            CcVacbLevelWithBcbsEntries += 1;

        } else {

            //
            //  Allocate and initialize the Vacb block for this level, and store its pointer
            //  back into our parent.
            //

            NextVacbArray =
            (PVACB *)ExAllocatePoolWithTag( NonPagedPool, VACB_LEVEL_BLOCK_SIZE + sizeof(VACB_LEVEL_REFERENCE), 'lVcC' );

            if (NextVacbArray == NULL) {
                return FALSE;
            }

            RtlZeroMemory( (PCHAR)NextVacbArray, VACB_LEVEL_BLOCK_SIZE + sizeof(VACB_LEVEL_REFERENCE) );

            CcAcquireVacbLock( OldIrql );

            NextVacbArray[0] = (PVACB)CcVacbLevelFreeList;
            CcVacbLevelFreeList = NextVacbArray;
            CcVacbLevelEntries += 1;
        }
    }

    return TRUE;
}


VOID
CcDrainVacbLevelZone (
    )

/*++

Routine Description:

    This routine should be called any time some entries have been deallocated to
    the VacbLevel zone, and we want to insure the zone is returned to a normal level.

Arguments:

Return Value:

    None.

Environment:

    No spinlocks should be held upon entry.

--*/

{
    KIRQL OldIrql;
    PVACB *NextVacbArray;

    //
    //  This is an unsafe loop to see if it looks like there is stuff to
    //  clean up.
    //

    while ((CcVacbLevelEntries > (CcMaxVacbLevelsSeen * 4)) ||
           (CcVacbLevelWithBcbsEntries > 2)) {

        //
        //  Now go in and try to pick up one entry to free under a FastLock.
        //

        NextVacbArray = NULL;
        CcAcquireVacbLock( &OldIrql );
        if (CcVacbLevelEntries > (CcMaxVacbLevelsSeen * 4)) {
            NextVacbArray = CcVacbLevelFreeList;
            CcVacbLevelFreeList = (PVACB *)NextVacbArray[0];
            CcVacbLevelEntries -= 1;
        } else if (CcVacbLevelWithBcbsEntries > 2) {
            NextVacbArray = CcVacbLevelWithBcbsFreeList;
            CcVacbLevelWithBcbsFreeList = (PVACB *)NextVacbArray[0];
            CcVacbLevelWithBcbsEntries -= 1;
        }
        CcReleaseVacbLock( OldIrql );

        //
        //  Since the loop is unsafe, we may not have gotten anything.
        //

        if (NextVacbArray != NULL) {
            ExFreePool(NextVacbArray);
        }
    }
}


PLIST_ENTRY
CcGetBcbListHeadLargeOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset,
    IN BOOLEAN FailToSuccessor
    )

/*++

Routine Description:

    This routine may be called to return the Bcb listhead for the specified FileOffset.
    It should only be called if the SectionSize is greater than VACB_SIZE_OF_FIRST_LEVEL.

Arguments:

    SharedCacheMap - Supplies the pointer to the SharedCacheMap for which the listhead
                     is desired.

    FileOffset - Supplies the fileOffset corresponding to the desired listhead.

    FailToSuccessor - Instructs whether not finding the exact listhead should cause us to
        return the predecessor or successor Bcb listhead.

Return Value:

    Returns the desired Listhead pointer.  If the desired listhead does not actually exist
    yet, then it returns the appropriate listhead.

Environment:

    The BcbSpinlock should be held on entry.

--*/

{
    ULONG Level, Shift;
    PVACB *VacbArray, *NextVacbArray;
    ULONG Index;
    ULONG SavedIndexes[VACB_NUMBER_OF_LEVELS];
    PVACB *SavedVacbArrays[VACB_NUMBER_OF_LEVELS];
    ULONG SavedLevels = 0;

    //
    //  Initialize variables controlling our descent into the hierarchy.
    //

    Level = 0;
    Shift = VACB_OFFSET_SHIFT + VACB_LEVEL_SHIFT;
    VacbArray = SharedCacheMap->Vacbs;

    //
    //  Caller must have verified that we have a hierarchy, otherwise this routine
    //  would fail.
    //

    ASSERT(SharedCacheMap->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL);

    //
    //  Loop to calculate how many levels we have and how much we have to
    //  shift to index into the first level.
    //

    do {

        Level += 1;
        Shift += VACB_LEVEL_SHIFT;

    } while (SharedCacheMap->SectionSize.QuadPart > ((LONGLONG)1 << Shift));

    //
    //  Our caller could be asking for an offset off the end of section size, so if he
    //  is actually off the size of the level, then return the main listhead.
    //

    if (FileOffset >= ((LONGLONG)1 << Shift)) {
        return &SharedCacheMap->BcbList;
    }

    //
    //  Now descend the tree to the bottom level to get the caller's Bcb ListHead.
    //

    Shift -= VACB_LEVEL_SHIFT;
    do {

        //
        //  Decrement back to the level that describes the size we are within.
        //

        Level -= 1;

        //
        //  Calculate the index into the Vacb block for this level.
        //

        Index = (ULONG)(FileOffset >> Shift);
        ASSERT(Index <= VACB_LAST_INDEX_FOR_LEVEL);

        //
        //  Get block address for next level.
        //

        NextVacbArray = (PVACB *)VacbArray[Index];

        //
        //  If it is NULL then we have to go find the highest Bcb or listhead which
        //  comes before the guy we are looking for, i.e., its predecessor.
        //

        if (NextVacbArray == NULL) {

            //
            //  Back up to look for the highest guy earlier in this tree, i.e., the
            //  predecessor listhead.
            //

            while (TRUE) {

                //
                //  Scan, if we can, in the current array for a non-null index.
                //

                if (FailToSuccessor) {

                    if (Index != VACB_LAST_INDEX_FOR_LEVEL) {

                        while ((Index != VACB_LAST_INDEX_FOR_LEVEL) && (VacbArray[++Index] == NULL)) {
                            continue;
                        }

                        //
                        //  If we found a non-null index, get out and try to return the
                        //  listhead.
                        //

                        if ((NextVacbArray = (PVACB *)VacbArray[Index]) != NULL) {
                            break;
                        }
                    }

                } else {

                    if (Index != 0) {

                        while ((Index != 0) && (VacbArray[--Index] == NULL)) {
                            continue;
                        }

                        //
                        //  If we found a non-null index, get out and try to return the
                        //  listhead.
                        //

                        if ((NextVacbArray = (PVACB *)VacbArray[Index]) != NULL) {
                            break;
                        }
                    }
                }

                //
                //  If there are no saved levels yet, then there is no predecessor or
                //  successor - it is the main listhead.
                //

                if (SavedLevels == 0) {
                    return &SharedCacheMap->BcbList;
                }

                //
                //  Otherwise, we can pop up a level in the tree and start scanning
                //  from that guy for a path to the right listhead.
                //

                Level += 1;
                Index = SavedIndexes[--SavedLevels];
                VacbArray = SavedVacbArrays[SavedLevels];
            }

            //
            //  We have backed up in the hierarchy, so now we are just looking for the
            //  highest/lowest guy in the level we want, i.e., the level-linking listhead.
            //  So smash FileOffset accordingly (we mask the high bits out anyway).
            //

            if (FailToSuccessor) {
                FileOffset = 0;
            } else {
                FileOffset = MAXLONGLONG;
            }
        }

        //
        //  We save Index and VacbArray at each level, for the case that we
        //  have to walk back up the tree to find a predecessor.
        //

        SavedIndexes[SavedLevels] = Index;
        SavedVacbArrays[SavedLevels] = VacbArray;
        SavedLevels += 1;

        //
        //  Now make this one our current pointer, and mask away the extraneous high-order
        //  FileOffset bits for this level.
        //

        VacbArray = NextVacbArray;
        FileOffset &= ((LONGLONG)1 << Shift) - 1;
        Shift -= VACB_LEVEL_SHIFT;

    //
    //  Loop until we hit the bottom level.
    //

    } while (Level != 0);

    //
    //  Now calculate the index for the bottom level and return the appropriate listhead.
    //  (The normal Vacb index indexes to a pointer to a Vacb for a .25MB view, so dropping
    //  the low bit gets you to the even-indexed Vacb pointer which is one block size below
    //  the two-pointer listhead for the Bcbs for that .5MB range...)
    //

    Index = (ULONG)(FileOffset >> Shift);
    return (PLIST_ENTRY)((PCHAR)&VacbArray[Index & ~1] + VACB_LEVEL_BLOCK_SIZE);
}


VOID
CcAdjustVacbLevelLockCount (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset,
    IN LONG Adjustment
    )

/*++

Routine Description:

    This routine may be called to adjust the lock count of the bottom Vacb level when
    Bcbs are inserted or deleted.  If the count goes to zero, the level will be
    eliminated.  The bottom level must exist, or we crash!

Arguments:

    SharedCacheMap - Supplies the pointer to the SharedCacheMap for which the Vacb
                     is desired.

    FileOffset - Supplies the fileOffset corresponding to the desired Vacb.

    Adjustment - Generally -1 or +1.

Return Value:

    None.

Environment:

    CcVacbSpinLock should be held on entry.

--*/

{
    ULONG Level, Shift;
    PVACB *VacbArray;
    LONGLONG OriginalFileOffset = FileOffset;

    //
    //  Initialize variables controlling our descent into the hierarchy.
    //

    Level = 0;
    Shift = VACB_OFFSET_SHIFT + VACB_LEVEL_SHIFT;

    VacbArray = SharedCacheMap->Vacbs;

    //
    //  Caller must have verified that we have a hierarchy, otherwise this routine
    //  would fail.
    //

    ASSERT(SharedCacheMap->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL);

    //
    //  Loop to calculate how many levels we have and how much we have to
    //  shift to index into the first level.
    //

    do {

        Level += 1;
        Shift += VACB_LEVEL_SHIFT;

    } while (SharedCacheMap->SectionSize.QuadPart > ((LONGLONG)1 << Shift));

    //
    //  Now descend the tree to the bottom level to get the caller's Vacb.
    //

    Shift -= VACB_LEVEL_SHIFT;
    do {

        VacbArray = (PVACB *)VacbArray[(ULONG)(FileOffset >> Shift)];

        Level -= 1;

        FileOffset &= ((LONGLONG)1 << Shift) - 1;

        Shift -= VACB_LEVEL_SHIFT;

    } while (Level != 0);

    //
    //  Now we have reached the final level, do the adjustment.
    //

    ReferenceVacbLevel( SharedCacheMap, VacbArray, Level, Adjustment, FALSE );

    //
    //  Now, if we decremented the count to 0, then force the collapse to happen by
    //  upping count and resetting to NULL.  Then smash OriginalFileOffset to be
    //  the first entry so we do not recalculate!
    //

    if (!IsVacbLevelReferenced( SharedCacheMap, VacbArray, Level )) {
        ReferenceVacbLevel( SharedCacheMap, VacbArray, Level, 1, TRUE );
        OriginalFileOffset &= ~(VACB_SIZE_OF_FIRST_LEVEL - 1);
        CcSetVacbLargeOffset( SharedCacheMap, OriginalFileOffset, VACB_SPECIAL_DEREFERENCE );
    }
}


VOID
CcCalculateVacbLevelLockCount (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN PVACB *VacbArray,
    IN ULONG Level
    )

/*++

Routine Description:

    This routine may be called to calculate or recalculate the lock count on a
    given Vacb level array.  It is called, for example, when we are extending a
    section up to the point where we activate multilevel logic and want to start
    keeping the count.

Arguments:

    SharedCacheMap - Supplies the pointer to the SharedCacheMap for which the Vacb
                     is desired.

    VacbArray - The Vacb Level array to recalculate

    Level - Supplies 0 for the bottom level, nonzero otherwise.

Return Value:

    None.

Environment:

    CcVacbSpinLock should be held on entry.

--*/

{
    PBCB Bcb;
    ULONG Index;
    LONG Count = 0;
    PVACB *VacbTemp = VacbArray;
    PVACB_LEVEL_REFERENCE VacbReference;

    //
    //  First loop through to count how many Vacb pointers are in use.
    //

    for (Index = 0; Index <= VACB_LAST_INDEX_FOR_LEVEL; Index++) {
        if (*(VacbTemp++) != NULL) {
            Count += 1;
        }
    }

    //
    //  If this is a metadata stream, we also have to count the Bcbs in the
    //  corresponding listheads.
    //

    if (FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED) && (Level == 0)) {

        //
        //  Pick up the Blink of the first listhead, casting it to a Bcb.
        //

        Bcb = (PBCB)CONTAINING_RECORD(((PLIST_ENTRY)VacbTemp)->Blink, BCB, BcbLinks);
        Index = 0;

        //
        //  Now loop through the list.  For each Bcb we see, increment the count,
        //  and for each listhead, increment Index.  We are done when we hit the
        //  last listhead, which is actually the next listhead past the ones in this
        //  block.
        //

        do {

            if (Bcb->NodeTypeCode == CACHE_NTC_BCB) {
                Count += 1;
            } else {
                Index += 1;
            }

            Bcb = (PBCB)CONTAINING_RECORD(Bcb->BcbLinks.Blink, BCB, BcbLinks);

        } while (Index <= (VACB_LAST_INDEX_FOR_LEVEL / 2));
    }

    //
    //  Store the count and get out... (by hand, don't touch the special count)
    //

    VacbReference = VacbLevelReference( SharedCacheMap, VacbArray, Level );
    VacbReference->Reference = Count;
}


PVACB
CcGetVacbLargeOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset
    )

/*++

Routine Description:

    This routine may be called to return the Vacb for the specified FileOffset.
    It should only be called if the SectionSize is greater than VACB_SIZE_OF_FIRST_LEVEL.

Arguments:

    SharedCacheMap - Supplies the pointer to the SharedCacheMap for which the Vacb
                     is desired.

    FileOffset - Supplies the fileOffset corresponding to the desired Vacb.

Return Value:

    Returns the desired Vacb pointer or NULL if there is none.

Environment:

    CcVacbSpinLock should be held on entry.

--*/

{
    ULONG Level, Shift;
    PVACB *VacbArray;
    PVACB Vacb;

    //
    //  Initialize variables controlling our descent into the hierarchy.
    //

    Level = 0;
    Shift = VACB_OFFSET_SHIFT + VACB_LEVEL_SHIFT;
    VacbArray = SharedCacheMap->Vacbs;

    //
    //  Caller must have verified that we have a hierarchy, otherwise this routine
    //  would fail.
    //

    ASSERT(SharedCacheMap->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL);

    //
    //  Loop to calculate how many levels we have and how much we have to
    //  shift to index into the first level.
    //

    do {

        Level += 1;
        Shift += VACB_LEVEL_SHIFT;

    } while (SharedCacheMap->SectionSize.QuadPart > ((LONGLONG)1 << Shift));

    //
    //  Now descend the tree to the bottom level to get the caller's Vacb.
    //

    Shift -= VACB_LEVEL_SHIFT;
    while (((Vacb = (PVACB)VacbArray[FileOffset >> Shift]) != NULL) && (Level != 0)) {

        Level -= 1;

        VacbArray = (PVACB *)Vacb;
        FileOffset &= ((LONGLONG)1 << Shift) - 1;

        Shift -= VACB_LEVEL_SHIFT;
    }

    //
    //  If the Vacb we exited with is not NULL, we want to make sure it looks OK.
    //

    ASSERT(Vacb == NULL || ((Vacb >= CcVacbs) && (Vacb < CcBeyondVacbs)));

    return Vacb;
}


VOID
CcSetVacbLargeOffset (
    IN PSHARED_CACHE_MAP SharedCacheMap,
    IN LONGLONG FileOffset,
    IN PVACB Vacb
    )

/*++

Routine Description:

    This routine may be called to set the specified Vacb pointer for the specified FileOffset.
    It should only be called if the SectionSize is greater than VACB_SIZE_OF_FIRST_LEVEL.

    For non-null Vacb, intermediate Vacb levels will be added as necessary, and if the lowest
    level has Bcb listheads, these will also be added.  For this case the caller must acquire
    the spinlock by calling CcPrefillVacbLevelZone specifying the worst-case number of levels
    required.

    For a null Vacb pointer, the tree is pruned of all Vacb levels that go empty.  If the lowest
    level has Bcb listheads, then they are removed.  The caller should subsequently call
    CcDrainVacbLevelZone once the spinlock is release to actually free some of this zone to the
    pool.

Arguments:

    SharedCacheMap - Supplies the pointer to the SharedCacheMap for which the Vacb
                     is desired.

    FileOffset - Supplies the fileOffset corresponding to the desired Vacb.

Return Value:

    Returns the desired Vacb pointer or NULL if there is none.

Environment:

    CcVacbSpinLock should be held on entry.

--*/

{
    ULONG Level, Shift;
    PVACB *VacbArray, *NextVacbArray;
    ULONG Index;
    ULONG SavedIndexes[VACB_NUMBER_OF_LEVELS];
    PVACB *SavedVacbArrays[VACB_NUMBER_OF_LEVELS];
    PLIST_ENTRY PredecessorListHead, SuccessorListHead, CurrentListHead;
    BOOLEAN AllocatingBcbListHeads, Special = FALSE;
    LONGLONG OriginalFileOffset = FileOffset;
    ULONG SavedLevels = 0;

    //
    //  Initialize variables controlling our descent into the hierarchy.
    //

    Level = 0;
    Shift = VACB_OFFSET_SHIFT + VACB_LEVEL_SHIFT;
    VacbArray = SharedCacheMap->Vacbs;

    //
    //  Caller must have verified that we have a hierarchy, otherwise this routine
    //  would fail.
    //

    ASSERT(SharedCacheMap->SectionSize.QuadPart > VACB_SIZE_OF_FIRST_LEVEL);

    //
    //  Loop to calculate how many levels we have and how much we have to
    //  shift to index into the first level.
    //

    do {

        Level += 1;
        Shift += VACB_LEVEL_SHIFT;

    } while (SharedCacheMap->SectionSize.QuadPart > ((LONGLONG)1 << Shift));

    //
    //  Now descend the tree to the bottom level to set the caller's Vacb.
    //

    Shift -= VACB_LEVEL_SHIFT;
    do {

        //
        //  Decrement back to the level that describes the size we are within.
        //

        Level -= 1;

        //
        //  Calculate the index into the Vacb block for this level.
        //

        Index = (ULONG)(FileOffset >> Shift);
        ASSERT(Index <= VACB_LAST_INDEX_FOR_LEVEL);

        //
        //  We save Index and VacbArray at each level, for the case that we
        //  are collapsing and deallocating blocks below.
        //

        SavedIndexes[SavedLevels] = Index;
        SavedVacbArrays[SavedLevels] = VacbArray;
        SavedLevels += 1;

        //
        //  Get block address for next level.
        //

        NextVacbArray = (PVACB *)VacbArray[Index];

        //
        //  If it is NULL then we have to allocate the next level to fill it in.
        //

        if (NextVacbArray == NULL) {

            //
            //  We better not be thinking we're dereferencing a level if the level
            //  doesn't currently exist.
            //

            ASSERT( Vacb != VACB_SPECIAL_DEREFERENCE );

            AllocatingBcbListHeads = FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED) && (Level == 0);

            //
            //  This is only valid if we are setting a nonzero pointer!
            //

            ASSERT(Vacb != NULL);

            NextVacbArray = CcAllocateVacbLevel(AllocatingBcbListHeads);

            //
            //  If we allocated Bcb Listheads, we must link them in.
            //

            if (AllocatingBcbListHeads) {

                ULONG i;

                //
                //  Find our predecessor.
                //

                PredecessorListHead = CcGetBcbListHeadLargeOffset( SharedCacheMap, OriginalFileOffset, FALSE );

                //
                //  If he is followed by any Bcbs, they "belong" to him, and we have to
                //  skip over them.
                //

                while (((PBCB)CONTAINING_RECORD(PredecessorListHead->Blink, BCB, BcbLinks))->NodeTypeCode ==
                       CACHE_NTC_BCB) {
                    PredecessorListHead = (PLIST_ENTRY)PredecessorListHead->Blink;
                }

                //
                //  Point to the first newly allocated listhead.
                //

                CurrentListHead = (PLIST_ENTRY)((PCHAR)NextVacbArray + VACB_LEVEL_BLOCK_SIZE);

                //
                //  Link first new listhead to predecessor.
                //

                SuccessorListHead = PredecessorListHead->Blink;
                PredecessorListHead->Blink = CurrentListHead;
                CurrentListHead->Flink = PredecessorListHead;

                //
                //  Now loop to link all of the new listheads together.
                //

                for (i = 0; i < ((VACB_LEVEL_BLOCK_SIZE / sizeof(LIST_ENTRY) - 1)); i++) {

                    CurrentListHead->Blink = CurrentListHead + 1;
                    CurrentListHead += 1;
                    CurrentListHead->Flink = CurrentListHead - 1;
                }

                //
                //  Finally link the last new listhead to the successor.
                //

                CurrentListHead->Blink = SuccessorListHead;
                SuccessorListHead->Flink = CurrentListHead;
            }

            VacbArray[Index] = (PVACB)NextVacbArray;

            //
            //  Increment the reference count.  Note that Level right now properly indicates
            //  what level NextVacbArray is at, not VacbArray.
            //

            ReferenceVacbLevel( SharedCacheMap, VacbArray, Level + 1, 1, FALSE );
        }

        //
        //  Now make this one our current pointer, and mask away the extraneous high-order
        //  FileOffset bits for this level and reduce the shift count.
        //

        VacbArray = NextVacbArray;
        FileOffset &= ((LONGLONG)1 << Shift) - 1;
        Shift -= VACB_LEVEL_SHIFT;

    //
    //  Loop until we hit the bottom level.
    //

    } while (Level != 0);

    if (Vacb < VACB_SPECIAL_FIRST_VALID) {

        //
        //  Now calculate the index for the bottom level and store the caller's Vacb pointer.
        //

        Index = (ULONG)(FileOffset >> Shift);
        VacbArray[Index] = Vacb;

    //
    //  Handle the special actions.
    //

    } else {

        Special = TRUE;

        //
        //  Induce the dereference.
        //

        if (Vacb == VACB_SPECIAL_DEREFERENCE) {

            Vacb = NULL;
        }
    }

    //
    //  If he is storing a nonzero pointer, just reference the level.
    //

    if (Vacb != NULL) {

        ASSERT( !(Special && Level != 0) );

        ReferenceVacbLevel( SharedCacheMap, VacbArray, Level, 1, Special );

    //
    //  Otherwise we are storing a NULL pointer, and we have to see if we can collapse
    //  the tree by deallocating empty blocks of pointers.
    //

    } else {

        //
        //  Loop until doing all possible collapse except for the top level.
        //

        while (TRUE) {

            ReferenceVacbLevel( SharedCacheMap, VacbArray, Level, -1, Special );

            //
            //  If this was a special dereference, then recognize that this was
            //  the only one.  The rest, as we tear up the tree, are regular
            //  (calculable) references.
            //

            Special = FALSE;

            //
            //  Now, if we have an empty block (other than the top one), then we should free the
            //  block and keep looping.
            //

            if (!IsVacbLevelReferenced( SharedCacheMap, VacbArray, Level ) && (SavedLevels != 0)) {

                SavedLevels -= 1;

                //
                //  First see if we have Bcb Listheads to delete and if so, we have to unlink
                //  the whole block first.
                //

                AllocatingBcbListHeads = FALSE;
                if ((Level++ == 0) && FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED)) {

                    AllocatingBcbListHeads = TRUE;
                    PredecessorListHead = ((PLIST_ENTRY)((PCHAR)VacbArray + VACB_LEVEL_BLOCK_SIZE))->Flink;
                    SuccessorListHead = ((PLIST_ENTRY)((PCHAR)VacbArray + (VACB_LEVEL_BLOCK_SIZE * 2) - sizeof(LIST_ENTRY)))->Blink;
                    PredecessorListHead->Blink = SuccessorListHead;
                    SuccessorListHead->Flink = PredecessorListHead;
                }

                //
                //  Free the unused block and then pick up the saved parent pointer array and
                //  index and erase the pointer to this block.
                //

                CcDeallocateVacbLevel( VacbArray, AllocatingBcbListHeads );
                Index = SavedIndexes[SavedLevels];
                VacbArray = SavedVacbArrays[SavedLevels];
                VacbArray[Index] = NULL;

            //
            //  No more collapsing if we hit a block that still has pointers, or we hit the root.
            //

            } else {
                break;
            }
        }
    }
}

