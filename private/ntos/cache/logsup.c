/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    logsup.c

Abstract:

    This module implements the special cache manager support for logging
    file systems.

Author:

    Tom Miller      [TomM]      30-Jul-1991

Revision History:

--*/

#include "cc.h"

//
//  Define our debug constant
//

#define me 0x0000040


VOID
CcSetAdditionalCacheAttributes (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN DisableReadAhead,
    IN BOOLEAN DisableWriteBehind
    )

/*++

Routine Description:

    This routine supports the setting of disable read ahead or disable write
    behind flags to control Cache Manager operation.  This routine may be
    called any time after calling CcInitializeCacheMap.  Initially both
    read ahead and write behind are enabled.  Note that the state of both
    of these flags must be specified on each call to this routine.

Arguments:

    FileObject - File object for which the respective flags are to be set.

    DisableReadAhead - FALSE to enable read ahead, TRUE to disable it.

    DisableWriteBehind - FALSE to enable write behind, TRUE to disable it.

Return Value:

    None.

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    KIRQL OldIrql;

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    //  Now set the flags and return.
    //

    CcAcquireMasterLock( &OldIrql );
    if (DisableReadAhead) {
        SetFlag(SharedCacheMap->Flags, DISABLE_READ_AHEAD);
    } else {
        ClearFlag(SharedCacheMap->Flags, DISABLE_READ_AHEAD);
    }
    if (DisableWriteBehind) {
        SetFlag(SharedCacheMap->Flags, DISABLE_WRITE_BEHIND | MODIFIED_WRITE_DISABLED);
    } else {
        ClearFlag(SharedCacheMap->Flags, DISABLE_WRITE_BEHIND);
    }
    CcReleaseMasterLock( OldIrql );
}


VOID
CcSetLogHandleForFile (
    IN PFILE_OBJECT FileObject,
    IN PVOID LogHandle,
    IN PFLUSH_TO_LSN FlushToLsnRoutine
    )

/*++

Routine Description:

    This routine may be called to instruct the Cache Manager to store the
    specified log handle with the shared cache map for a file, to support
    subsequent calls to the other routines in this module which effectively
    perform an associative search for files by log handle.

Arguments:

    FileObject - File for which the log handle should be stored.

    LogHandle - Log Handle to store.

    FlushToLsnRoutine - A routine to call before flushing buffers for this
                        file, to insure a log file is flushed to the most
                        recent Lsn for any Bcb being flushed.

Return Value:

    None.

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    //  Now set the log file handle and flush routine
    //

    SharedCacheMap->LogHandle = LogHandle;
    SharedCacheMap->FlushToLsnRoutine = FlushToLsnRoutine;
}


LARGE_INTEGER
CcGetDirtyPages (
    IN PVOID LogHandle,
    IN PDIRTY_PAGE_ROUTINE DirtyPageRoutine,
    IN PVOID Context1,
    IN PVOID Context2
    )

/*++

Routine Description:

    This routine may be called to return all of the dirty pages in all files
    for a given log handle.  Each page is returned by an individual call to
    the Dirty Page Routine.  The Dirty Page Routine is defined by a prototype
    in ntos\inc\cache.h.

Arguments:

    LogHandle - Log Handle which must match the log handle previously stored
                for all files which are to be returned.

    DirtyPageRoutine -- The routine to call as each dirty page for this log
                        handle is found.

    Context1 - First context parameter to be passed to the Dirty Page Routine.

    Context2 - First context parameter to be passed to the Dirty Page Routine.

Return Value:

    LARGE_INTEGER - Oldest Lsn found of all the dirty pages, or 0 if no dirty pages

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PBCB Bcb, BcbToUnpin = NULL;
    KIRQL OldIrql;
    NTSTATUS ExceptionStatus;
    LARGE_INTEGER SavedFileOffset, SavedOldestLsn, SavedNewestLsn;
    ULONG SavedByteLength;
    LARGE_INTEGER OldestLsn = {0,0};

    //
    //  Synchronize with changes to the SharedCacheMap list.
    //

    CcAcquireMasterLock( &OldIrql );

    SharedCacheMap = CONTAINING_RECORD( CcDirtySharedCacheMapList.SharedCacheMapLinks.Flink,
                                        SHARED_CACHE_MAP,
                                        SharedCacheMapLinks );

    //
    //  Use try/finally for cleanup.  The only spot where we can raise is out of the
    //  filesystem callback, but we have the exception handler out here so we aren't
    //  constantly setting/unsetting it.
    //

    try {

        while (&SharedCacheMap->SharedCacheMapLinks != &CcDirtySharedCacheMapList.SharedCacheMapLinks) {

            //
            //  Skip over cursors, SharedCacheMaps for other LogHandles, and ones with
            //  no dirty pages
            //

            if (!FlagOn(SharedCacheMap->Flags, IS_CURSOR) && (SharedCacheMap->LogHandle == LogHandle) &&
                (SharedCacheMap->DirtyPages != 0)) {

                //
                //  This SharedCacheMap should stick around for a while in the dirty list.
                //

                CcIncrementOpenCount( SharedCacheMap, 'pdGS' );
                SharedCacheMap->DirtyPages += 1;
                CcReleaseMasterLock( OldIrql );

                //
                //  Set our initial resume point and point to first Bcb in List.
                //

                ExAcquireFastLock( &SharedCacheMap->BcbSpinLock, &OldIrql );
                Bcb = CONTAINING_RECORD( SharedCacheMap->BcbList.Flink, BCB, BcbLinks );

                //
                //  Scan to the end of the Bcb list.
                //

                while (&Bcb->BcbLinks != &SharedCacheMap->BcbList) {

                    //
                    //  If the Bcb is dirty, then capture the inputs for the
                    //  callback routine so we can call without holding a spinlock.
                    //

                    if ((Bcb->NodeTypeCode == CACHE_NTC_BCB) && Bcb->Dirty) {

                        SavedFileOffset = Bcb->FileOffset;
                        SavedByteLength = Bcb->ByteLength;
                        SavedOldestLsn = Bcb->OldestLsn;
                        SavedNewestLsn = Bcb->NewestLsn;

                        //
                        //  Increment PinCount so the Bcb sticks around
                        //

                        Bcb->PinCount += 1;

                        ExReleaseFastLock( &SharedCacheMap->BcbSpinLock, OldIrql );

                        //
                        //  Any Bcb to unpin from a previous loop?
                        //

                        if (BcbToUnpin != NULL) {
                            CcUnpinFileData( BcbToUnpin, TRUE, UNREF );
                            BcbToUnpin = NULL;
                        }

                        //
                        //  Call the file system.  This callback may raise status.
                        //

                        (*DirtyPageRoutine)( SharedCacheMap->FileObject,
                                             &SavedFileOffset,
                                             SavedByteLength,
                                             &SavedOldestLsn,
                                             &SavedNewestLsn,
                                             Context1,
                                             Context2 );

                        //
                        //  Possibly update OldestLsn
                        //

                        if ((SavedOldestLsn.QuadPart != 0) &&
                            ((OldestLsn.QuadPart == 0) || (SavedOldestLsn.QuadPart < OldestLsn.QuadPart ))) {
                            OldestLsn = SavedOldestLsn;
                        }

                        //
                        //  Now reacquire the spinlock and scan from the resume point
                        //  point to the next Bcb to return in the descending list.
                        //

                        ExAcquireFastLock( &SharedCacheMap->BcbSpinLock, &OldIrql );

                        //
                        //  Normally the Bcb can stay around a while, but if not,
                        //  we will just remember it for the next time we do not
                        //  have the spin lock.  We cannot unpin it now, because
                        //  we would lose our place in the list.
                        //

                        if (Bcb->PinCount > 1) {
                            Bcb->PinCount -= 1;
                        } else {
                            BcbToUnpin = Bcb;
                        }
                    }

                    Bcb = CONTAINING_RECORD( Bcb->BcbLinks.Flink, BCB, BcbLinks );
                }
                ExReleaseFastLock( &SharedCacheMap->BcbSpinLock, OldIrql );

                //
                //  We need to unpin any Bcb we are holding before moving on to
                //  the next SharedCacheMap, or else CcDeleteSharedCacheMap will
                //  also delete this Bcb.
                //

                if (BcbToUnpin != NULL) {

                    CcUnpinFileData( BcbToUnpin, TRUE, UNREF );
                    BcbToUnpin = NULL;
                }

                CcAcquireMasterLock( &OldIrql );

                //
                //  Now release the SharedCacheMap, leaving it in the dirty list.
                //

                CcDecrementOpenCount( SharedCacheMap, 'pdGF' );
                SharedCacheMap->DirtyPages -= 1;
            }

            //
            //  Now loop back for the next cache map.
            //

            SharedCacheMap =
                CONTAINING_RECORD( SharedCacheMap->SharedCacheMapLinks.Flink,
                                   SHARED_CACHE_MAP,
                                   SharedCacheMapLinks );
        }

        CcReleaseMasterLock( OldIrql );

    } finally {

        //
        //  Drop the Bcb if we are being ejected.  We are guaranteed that the
        //  only raise is from the callback, at which point we have an incremented
        //  pincount.
        //

        if (AbnormalTermination()) {

            CcUnpinFileData( Bcb, TRUE, UNPIN );
        }
    }

    return OldestLsn;
}


BOOLEAN
CcIsThereDirtyData (
    IN PVPB Vpb
    )

/*++

Routine Description:

    This routine returns TRUE if the specified Vcb has any unwritten dirty
    data in the cache.

Arguments:

    Vpb - specifies Vpb to check for

Return Value:

    FALSE - if the Vpb has no dirty data
    TRUE - if the Vpb has dirty data

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    KIRQL OldIrql;
    ULONG LoopsWithLockHeld = 0;

    //
    //  Synchronize with changes to the SharedCacheMap list.
    //

    CcAcquireMasterLock( &OldIrql );

    SharedCacheMap = CONTAINING_RECORD( CcDirtySharedCacheMapList.SharedCacheMapLinks.Flink,
                                        SHARED_CACHE_MAP,
                                        SharedCacheMapLinks );

    while (&SharedCacheMap->SharedCacheMapLinks != &CcDirtySharedCacheMapList.SharedCacheMapLinks) {

        //
        //  Look at this one if the Vpb matches and if there is dirty data.
        //  For what it's worth, don't worry about dirty data in temporary files,
        //  as that should not concern the caller if it wants to dismount.
        //

        if (!FlagOn(SharedCacheMap->Flags, IS_CURSOR) &&
            (SharedCacheMap->FileObject->Vpb == Vpb) &&
            (SharedCacheMap->DirtyPages != 0) &&
            !FlagOn(SharedCacheMap->FileObject->Flags, FO_TEMPORARY_FILE)) {

            CcReleaseMasterLock( OldIrql );
            return TRUE;
        }

        //
        //  Make sure we occassionally drop the lock.  Set WRITE_QUEUED
        //  to keep the guy from going away, and increment DirtyPages to
        //  keep in in this list.
        //

        if ((++LoopsWithLockHeld >= 20) &&
            !FlagOn(SharedCacheMap->Flags, WRITE_QUEUED | IS_CURSOR)) {

            SetFlag( (volatile ULONG) SharedCacheMap->Flags, WRITE_QUEUED);
            (volatile ULONG) SharedCacheMap->DirtyPages += 1;
            CcReleaseMasterLock( OldIrql );
            LoopsWithLockHeld = 0;
            CcAcquireMasterLock( &OldIrql );
            ClearFlag( (volatile ULONG) SharedCacheMap->Flags, WRITE_QUEUED);
            (volatile ULONG) SharedCacheMap->DirtyPages -= 1;
        }

        //
        //  Now loop back for the next cache map.
        //

        SharedCacheMap =
            CONTAINING_RECORD( SharedCacheMap->SharedCacheMapLinks.Flink,
                               SHARED_CACHE_MAP,
                               SharedCacheMapLinks );
    }

    CcReleaseMasterLock( OldIrql );

    return FALSE;
}

LARGE_INTEGER
CcGetLsnForFileObject(
    IN PFILE_OBJECT FileObject,
    OUT PLARGE_INTEGER OldestLsn OPTIONAL
    )

/*++

Routine Description:

    This routine returns the  oldest and newest LSNs for a file object.

Arguments:

    FileObject - File for which the log handle should be stored.

    OldestLsn - pointer to location to store oldest LSN for file object.

Return Value:

    The newest LSN for the file object.

--*/

{
    PBCB Bcb;
    KIRQL OldIrql;
    LARGE_INTEGER Oldest, Newest;
    PSHARED_CACHE_MAP SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    // initialize lsn variables
    //

    Oldest.LowPart = 0;
    Oldest.HighPart = 0;
    Newest.LowPart = 0;
    Newest.HighPart = 0;

    if(SharedCacheMap == NULL) {
        return Oldest;
    }

    ExAcquireFastLock(&SharedCacheMap->BcbSpinLock, &OldIrql);

    //
    //  Now point to first Bcb in List, and loop through it.
    //

    Bcb = CONTAINING_RECORD( SharedCacheMap->BcbList.Flink, BCB, BcbLinks );

    while (&Bcb->BcbLinks != &SharedCacheMap->BcbList) {

        //
        //  If the Bcb is dirty then capture the oldest and newest lsn
        //


        if ((Bcb->NodeTypeCode == CACHE_NTC_BCB) && Bcb->Dirty) {

            LARGE_INTEGER BcbLsn, BcbNewest;

            BcbLsn = Bcb->OldestLsn;
            BcbNewest = Bcb->NewestLsn;

            if ((BcbLsn.QuadPart != 0) &&
                ((Oldest.QuadPart == 0) ||
                 (BcbLsn.QuadPart < Oldest.QuadPart))) {

                 Oldest = BcbLsn;
            }

            if ((BcbLsn.QuadPart != 0) && (BcbNewest.QuadPart > Newest.QuadPart)) {

                Newest = BcbNewest;
            }
        }


        Bcb = CONTAINING_RECORD( Bcb->BcbLinks.Flink, BCB, BcbLinks );
    }

    //
    //  Now release the spin lock for this Bcb list and generate a callback
    //  if we got something.
    //

    ExReleaseFastLock( &SharedCacheMap->BcbSpinLock, OldIrql );

    if (ARGUMENT_PRESENT(OldestLsn)) {

        *OldestLsn = Oldest;
    }

    return Newest;
}
