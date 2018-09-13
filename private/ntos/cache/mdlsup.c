/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mdlsup.c

Abstract:

    This module implements the Mdl support routines for the Cache subsystem.

Author:

    Tom Miller      [TomM]      4-May-1990

Revision History:

--*/

#include "cc.h"

//
//  Debug Trace Level
//

#define me                               (0x00000010)

VOID
CcMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This routine attempts to lock the specified file data in the cache
    and return a description of it in an Mdl along with the correct
    I/O status.  It is *not* safe to call this routine from Dpc level.

    This routine is synchronous, and raises on errors.

    As each call returns, the pages described by the Mdl are
    locked in memory, but not mapped in system space.  If the caller
    needs the pages mapped in system space, then it must map them.

    Note that each call is a "single shot" which should be followed by
    a call to CcMdlReadComplete.  To resume an Mdl-based transfer, the
    caller must form one or more subsequent calls to CcMdlRead with
    appropriately adjusted parameters.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    MdlChain - On output it returns a pointer to an Mdl chain describing
               the desired data.  Note that even if FALSE is returned,
               one or more Mdls may have been allocated, as may be ascertained
               by the IoStatus.Information field (see below).

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.  (STATUS_SUCCESS guaranteed for cache
               hits, otherwise the actual I/O status is returned.)  The
               I/O Information Field indicates how many bytes have been
               successfully locked down in the Mdl Chain.

Return Value:

    None

Raises:

    STATUS_INSUFFICIENT_RESOURCES - If a pool allocation failure occurs.

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PPRIVATE_CACHE_MAP PrivateCacheMap;
    PVOID CacheBuffer;
    LARGE_INTEGER FOffset;
    PMDL Mdl = NULL;
    PMDL MdlTemp;
    PETHREAD Thread = PsGetCurrentThread();
    ULONG SavedState = 0;
    ULONG OriginalLength = Length;
    ULONG Information = 0;
    PVACB Vacb = NULL;
    ULONG SavedMissCounter = 0;

    KIRQL OldIrql;
    ULONG ActivePage;
    ULONG PageIsDirty;
    PVACB ActiveVacb = NULL;

    DebugTrace(+1, me, "CcMdlRead\n", 0 );
    DebugTrace( 0, me, "    FileObject = %08lx\n", FileObject );
    DebugTrace2(0, me, "    FileOffset = %08lx, %08lx\n", FileOffset->LowPart,
                                                          FileOffset->HighPart );
    DebugTrace( 0, me, "    Length = %08lx\n", Length );

    //
    //  Save the current readahead hints.
    //

    MmSavePageFaultReadAhead( Thread, &SavedState );

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    PrivateCacheMap = FileObject->PrivateCacheMap;

    //
    //  See if we have an active Vacb, that we need to free.
    //

    GetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

    //
    //  If there is an end of a page to be zeroed, then free that page now,
    //  so we don't send Greg the uninitialized data...
    //

    if ((ActiveVacb != NULL) || (SharedCacheMap->NeedToZero != NULL)) {

        CcFreeActiveVacb( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );
    }

    //
    //  If read ahead is enabled, then do the read ahead here so it
    //  overlaps with the copy (otherwise we will do it below).
    //  Note that we are assuming that we will not get ahead of our
    //  current transfer - if read ahead is working it should either
    //  already be in memory or else underway.
    //

    if (PrivateCacheMap->ReadAheadEnabled && (PrivateCacheMap->ReadAheadLength[1] == 0)) {
        CcScheduleReadAhead( FileObject, FileOffset, Length );
    }

    //
    //  Increment performance counters
    //

    CcMdlReadWait += 1;

    //
    //  This is not an exact solution, but when IoPageRead gets a miss,
    //  it cannot tell whether it was CcCopyRead or CcMdlRead, but since
    //  the miss should occur very soon, by loading the pointer here
    //  probably the right counter will get incremented, and in any case,
    //  we hope the errrors average out!
    //

    CcMissCounter = &CcMdlReadWaitMiss;

    FOffset = *FileOffset;

    //
    //  Check for read past file size, the caller must filter this case out.
    //

    ASSERT( ( FOffset.QuadPart + (LONGLONG)Length ) <= SharedCacheMap->FileSize.QuadPart );

    //
    //  Put try-finally around the loop to deal with any exceptions
    //

    try {

        //
        //  Not all of the transfer will come back at once, so we have to loop
        //  until the entire transfer is complete.
        //

        while (Length != 0) {

            ULONG ReceivedLength;
            LARGE_INTEGER BeyondLastByte;

            //
            //  Map the data and read it in (if necessary) with the
            //  MmProbeAndLockPages call below.
            //

            CacheBuffer = CcGetVirtualAddress( SharedCacheMap,
                                               FOffset,
                                               &Vacb,
                                               &ReceivedLength );

            if (ReceivedLength > Length) {
                ReceivedLength = Length;
            }

            BeyondLastByte.QuadPart = FOffset.QuadPart + (LONGLONG)ReceivedLength;

            //
            //  Now attempt to allocate an Mdl to describe the mapped data.
            //

            DebugTrace( 0, mm, "IoAllocateMdl:\n", 0 );
            DebugTrace( 0, mm, "    BaseAddress = %08lx\n", CacheBuffer );
            DebugTrace( 0, mm, "    Length = %08lx\n", ReceivedLength );

            Mdl = IoAllocateMdl( CacheBuffer,
                                 ReceivedLength,
                                 FALSE,
                                 FALSE,
                                 NULL );

            DebugTrace( 0, mm, "    <Mdl = %08lx\n", Mdl );

            if (Mdl == NULL) {
                DebugTrace( 0, 0, "Failed to allocate Mdl\n", 0 );

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            DebugTrace( 0, mm, "MmProbeAndLockPages:\n", 0 );
            DebugTrace( 0, mm, "    Mdl = %08lx\n", Mdl );

            //
            //  Set to see if the miss counter changes in order to
            //  detect when we should turn on read ahead.
            //

            SavedMissCounter += CcMdlReadWaitMiss;

            MmSetPageFaultReadAhead( Thread, COMPUTE_PAGES_SPANNED( CacheBuffer, ReceivedLength ) - 1);
            MmProbeAndLockPages( Mdl, KernelMode, IoReadAccess );

            SavedMissCounter -= CcMdlReadWaitMiss;

            //
            //  Unmap the data now, now that the pages are locked down.
            //

            CcFreeVirtualAddress( Vacb );
            Vacb = NULL;

            //
            //  Now link the Mdl into the caller's chain
            //

            if ( *MdlChain == NULL ) {
                *MdlChain = Mdl;
            } else {
                MdlTemp = CONTAINING_RECORD( *MdlChain, MDL, Next );
                while (MdlTemp->Next != NULL) {
                    MdlTemp = MdlTemp->Next;
                }
                MdlTemp->Next = Mdl;
            }
            Mdl = NULL;

            //
            //  Assume we did not get all the data we wanted, and set FOffset
            //  to the end of the returned data.
            //

            FOffset = BeyondLastByte;

            //
            //  Update number of bytes transferred.
            //

            Information += ReceivedLength;

            //
            //  Calculate length left to transfer.
            //

            Length -= ReceivedLength;
        }
    }
    finally {

        CcMissCounter = &CcThrowAway;

        //
        //  Restore the readahead hints.
        //

        MmResetPageFaultReadAhead( Thread, SavedState );

        if (AbnormalTermination()) {

            //
            //  We may have failed to allocate an Mdl while still having
            //  data mapped.
            //

            if (Vacb != NULL) {
                CcFreeVirtualAddress( Vacb );
            }

            if (Mdl != NULL) {
                IoFreeMdl( Mdl );
            }

            //
            //  Otherwise loop to deallocate the Mdls
            //

            while (*MdlChain != NULL) {
                MdlTemp = (*MdlChain)->Next;

                DebugTrace( 0, mm, "MmUnlockPages/IoFreeMdl:\n", 0 );
                DebugTrace( 0, mm, "    Mdl = %08lx\n", *MdlChain );

                MmUnlockPages( *MdlChain );
                IoFreeMdl( *MdlChain );

                *MdlChain = MdlTemp;
            }

            DebugTrace(-1, me, "CcMdlRead -> Unwinding\n", 0 );

        }
        else {

            //
            //  Now enable read ahead if it looks like we got any misses, and do
            //  the first one.
            //

            if (!FlagOn( FileObject->Flags, FO_RANDOM_ACCESS ) &&
                !PrivateCacheMap->ReadAheadEnabled &&
                (SavedMissCounter != 0)) {

                PrivateCacheMap->ReadAheadEnabled = TRUE;
                CcScheduleReadAhead( FileObject, FileOffset, OriginalLength );
            }

            //
            //  Now that we have described our desired read ahead, let's
            //  shift the read history down.
            //

            PrivateCacheMap->FileOffset1 = PrivateCacheMap->FileOffset2;
            PrivateCacheMap->BeyondLastByte1 = PrivateCacheMap->BeyondLastByte2;
            PrivateCacheMap->FileOffset2 = *FileOffset;
            PrivateCacheMap->BeyondLastByte2.QuadPart =
                                FileOffset->QuadPart + (LONGLONG)OriginalLength;

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = Information;
        }
    }


    DebugTrace( 0, me, "    <MdlChain = %08lx\n", *MdlChain );
    DebugTrace2(0, me, "    <IoStatus = %08lx, %08lx\n", IoStatus->Status,
                                                         IoStatus->Information );
    DebugTrace(-1, me, "CcMdlRead -> VOID\n", 0 );

    return;
}


//
//  First we have the old routine which checks for an entry in the FastIo vector.
//  This routine becomes obsolete for every component that compiles with the new
//  definition of FsRtlMdlReadComplete in fsrtl.h.
//

VOID
CcMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain
    )

{
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlWriteComplete)) &&
        (FastIoDispatch->MdlReadComplete != NULL) &&
        FastIoDispatch->MdlReadComplete( FileObject, MdlChain, DeviceObject )) {

        NOTHING;

    } else {
        CcMdlReadComplete2( FileObject, MdlChain );
    }
}

VOID
CcMdlReadComplete2 (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain
    )

/*++

Routine Description:

    This routine must be called at IPL0 after a call to CcMdlRead.  The
    caller must simply supply the address of the MdlChain returned in
    CcMdlRead.

    This call does the following:

        Deletes the MdlChain

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    MdlChain - same as returned from corresponding call to CcMdlRead.

Return Value:

    None.
--*/

{
    PMDL MdlNext;

    DebugTrace(+1, me, "CcMdlReadComplete\n", 0 );
    DebugTrace( 0, me, "    FileObject = %08lx\n", FileObject );
    DebugTrace( 0, me, "    MdlChain = %08lx\n", MdlChain );

    //
    //  Deallocate the Mdls
    //

    while (MdlChain != NULL) {

        MdlNext = MdlChain->Next;

        DebugTrace( 0, mm, "MmUnlockPages/IoFreeMdl:\n", 0 );
        DebugTrace( 0, mm, "    Mdl = %08lx\n", MdlChain );

        MmUnlockPages( MdlChain );

        IoFreeMdl( MdlChain );

        MdlChain = MdlNext;
    }

    DebugTrace(-1, me, "CcMdlReadComplete -> VOID\n", 0 );
}


VOID
CcPrepareMdlWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This routine attempts to lock the specified file data in the cache
    and return a description of it in an Mdl along with the correct
    I/O status.  Pages to be completely overwritten may be satisfied
    with emtpy pages.  It is *not* safe to call this routine from Dpc level.

    This call is synchronous and raises on error.

    When this call returns, the caller may immediately begin
    to transfer data into the buffers via the Mdl.

    When the call returns with TRUE, the pages described by the Mdl are
    locked in memory, but not mapped in system space.  If the caller
    needs the pages mapped in system space, then it must map them.
    On the subsequent call to CcMdlWriteComplete the pages will be
    unmapped if they were mapped, and in any case unlocked and the Mdl
    deallocated.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    MdlChain - On output it returns a pointer to an Mdl chain describing
               the desired data.  Note that even if FALSE is returned,
               one or more Mdls may have been allocated, as may be ascertained
               by the IoStatus.Information field (see below).

    IoStatus - Pointer to standard I/O status block to receive the status
               for the in-transfer of the data.  (STATUS_SUCCESS guaranteed
               for cache hits, otherwise the actual I/O status is returned.)
               The I/O Information Field indicates how many bytes have been
               successfully locked down in the Mdl Chain.

Return Value:

    None

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PVOID CacheBuffer;
    LARGE_INTEGER FOffset;
    PMDL Mdl = NULL;
    PMDL MdlTemp;
    LARGE_INTEGER Temp;
    ULONG SavedState = 0;
    ULONG ZeroFlags = 0;
    ULONG Information = 0;

    KIRQL OldIrql;
    ULONG ActivePage;
    ULONG PageIsDirty;
    PVACB Vacb = NULL;

    DebugTrace(+1, me, "CcPrepareMdlWrite\n", 0 );
    DebugTrace( 0, me, "    FileObject = %08lx\n", FileObject );
    DebugTrace2(0, me, "    FileOffset = %08lx, %08lx\n", FileOffset->LowPart,
                                                          FileOffset->HighPart );
    DebugTrace( 0, me, "    Length = %08lx\n", Length );

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    //  See if we have an active Vacb, that we need to free.
    //

    GetActiveVacb( SharedCacheMap, OldIrql, Vacb, ActivePage, PageIsDirty );

    //
    //  If there is an end of a page to be zeroed, then free that page now,
    //  so it does not cause our data to get zeroed.  If there is an active
    //  page, free it so we have the correct ValidDataGoal.
    //

    if ((Vacb != NULL) || (SharedCacheMap->NeedToZero != NULL)) {

        CcFreeActiveVacb( SharedCacheMap, Vacb, ActivePage, PageIsDirty );
        Vacb = NULL;
    }

    FOffset = *FileOffset;

    //
    //  Put try-finally around the loop to deal with exceptions
    //

    try {

        //
        //  Not all of the transfer will come back at once, so we have to loop
        //  until the entire transfer is complete.
        //

        while (Length != 0) {

            ULONG ReceivedLength;
            LARGE_INTEGER BeyondLastByte;

            //
            //  Map and see how much we could potentially access at this
            //  FileOffset, then cut it down if it is more than we need.
            //

            CacheBuffer = CcGetVirtualAddress( SharedCacheMap,
                                               FOffset,
                                               &Vacb,
                                               &ReceivedLength );

            if (ReceivedLength > Length) {
                ReceivedLength = Length;
            }

            BeyondLastByte.QuadPart = FOffset.QuadPart + (LONGLONG)ReceivedLength;

            //
            //  At this point we can calculate the ZeroFlags.
            //

            //
            //  We can always zero middle pages, if any.
            //

            ZeroFlags = ZERO_MIDDLE_PAGES;

            //
            //  See if we are completely overwriting the first or last page.
            //

            if (((FOffset.LowPart & (PAGE_SIZE - 1)) == 0) &&
                (ReceivedLength >= PAGE_SIZE)) {
                ZeroFlags |= ZERO_FIRST_PAGE;
            }

            if ((BeyondLastByte.LowPart & (PAGE_SIZE - 1)) == 0) {
                ZeroFlags |= ZERO_LAST_PAGE;
            }

            //
            //  See if the entire transfer is beyond valid data length,
            //  or at least starting from the second page.
            //

            Temp = FOffset;
            Temp.LowPart &= ~(PAGE_SIZE -1);
            ExAcquireFastLock( &SharedCacheMap->BcbSpinLock, &OldIrql );
            Temp.QuadPart = SharedCacheMap->ValidDataGoal.QuadPart - Temp.QuadPart;
            ExReleaseFastLock( &SharedCacheMap->BcbSpinLock, OldIrql );

            if (Temp.QuadPart <= 0) {
                ZeroFlags |= ZERO_FIRST_PAGE | ZERO_MIDDLE_PAGES | ZERO_LAST_PAGE;
            } else if ((Temp.HighPart == 0) && (Temp.LowPart <= PAGE_SIZE)) {
                ZeroFlags |= ZERO_MIDDLE_PAGES | ZERO_LAST_PAGE;
            }

            (VOID)CcMapAndRead( SharedCacheMap,
                                &FOffset,
                                ReceivedLength,
                                ZeroFlags,
                                TRUE,
                                CacheBuffer );

            //
            //  Now attempt to allocate an Mdl to describe the mapped data.
            //

            DebugTrace( 0, mm, "IoAllocateMdl:\n", 0 );
            DebugTrace( 0, mm, "    BaseAddress = %08lx\n", CacheBuffer );
            DebugTrace( 0, mm, "    Length = %08lx\n", ReceivedLength );

            Mdl = IoAllocateMdl( CacheBuffer,
                                 ReceivedLength,
                                 FALSE,
                                 FALSE,
                                 NULL );

            DebugTrace( 0, mm, "    <Mdl = %08lx\n", Mdl );

            if (Mdl == NULL) {
                DebugTrace( 0, 0, "Failed to allocate Mdl\n", 0 );

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            DebugTrace( 0, mm, "MmProbeAndLockPages:\n", 0 );
            DebugTrace( 0, mm, "    Mdl = %08lx\n", Mdl );

            MmDisablePageFaultClustering(&SavedState);
            MmProbeAndLockPages( Mdl, KernelMode, IoWriteAccess );
            MmEnablePageFaultClustering(SavedState);
            SavedState = 0;

            //
            //  Now that some data (maybe zeros) is locked in memory and
            //  set dirty, it is safe, and necessary for us to advance
            //  valid data goal, so that we will not subsequently ask
            //  for a zero page.  Note if we are extending valid data,
            //  our caller has the file exclusive.
            //

            ExAcquireFastLock( &SharedCacheMap->BcbSpinLock, &OldIrql );
            if (BeyondLastByte.QuadPart > SharedCacheMap->ValidDataGoal.QuadPart) {
                SharedCacheMap->ValidDataGoal = BeyondLastByte;
            }
            ExReleaseFastLock( &SharedCacheMap->BcbSpinLock, OldIrql );

            //
            //  Unmap the data now, now that the pages are locked down.
            //

            CcFreeVirtualAddress( Vacb );
            Vacb = NULL;

            //
            //  Now link the Mdl into the caller's chain
            //

            if ( *MdlChain == NULL ) {
                *MdlChain = Mdl;
            } else {
                MdlTemp = CONTAINING_RECORD( *MdlChain, MDL, Next );
                while (MdlTemp->Next != NULL) {
                    MdlTemp = MdlTemp->Next;
                }
                MdlTemp->Next = Mdl;
            }
            Mdl = NULL;

            //
            //  Assume we did not get all the data we wanted, and set FOffset
            //  to the end of the returned data.
            //

            FOffset = BeyondLastByte;

            //
            //  Update number of bytes transferred.
            //

            Information += ReceivedLength;

            //
            //  Calculate length left to transfer.
            //

            Length -= ReceivedLength;
        }
    }
    finally {

        if (AbnormalTermination()) {

            if (SavedState != 0) {
                MmEnablePageFaultClustering(SavedState);
            }

            if (Vacb != NULL) {
                CcFreeVirtualAddress( Vacb );
            }
            
            if (Mdl != NULL) {
                IoFreeMdl( Mdl );
            }

            //
            //  Otherwise loop to deallocate the Mdls
            //

            FOffset = *FileOffset;
            while (*MdlChain != NULL) {
                MdlTemp = (*MdlChain)->Next;

                DebugTrace( 0, mm, "MmUnlockPages/IoFreeMdl:\n", 0 );
                DebugTrace( 0, mm, "    Mdl = %08lx\n", *MdlChain );

                MmUnlockPages( *MdlChain );

                //
                //  Extract the File Offset for this part of the transfer, and
                //  tell the lazy writer to write these pages, since we have
                //  marked them dirty.  Ignore the only exception (allocation
                //  error), and console ourselves for having tried.
                //

                CcSetDirtyInMask( SharedCacheMap, &FOffset, (*MdlChain)->ByteCount );

                FOffset.QuadPart = FOffset.QuadPart + (LONGLONG)((*MdlChain)->ByteCount);

                IoFreeMdl( *MdlChain );

                *MdlChain = MdlTemp;
            }

            DebugTrace(-1, me, "CcPrepareMdlWrite -> Unwinding\n", 0 );
        }
        else {

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = Information;

            //
            //  Make sure the SharedCacheMap does not go away while
            //  the Mdl write is in progress.  We decrment below.
            //

            CcAcquireMasterLock( &OldIrql );
            CcIncrementOpenCount( SharedCacheMap, 'ldmP' );
            CcReleaseMasterLock( OldIrql );
        }
    }

    DebugTrace( 0, me, "    <MdlChain = %08lx\n", *MdlChain );
    DebugTrace(-1, me, "CcPrepareMdlWrite -> VOID\n", 0 );

    return;
}


//
//  First we have the old routine which checks for an entry in the FastIo vector.
//  This routine becomes obsolete for every component that compiles with the new
//  definition of FsRtlMdlWriteComplete in fsrtl.h.
//

VOID
CcMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain
    )

{
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlWriteComplete)) &&
        (FastIoDispatch->MdlWriteComplete != NULL) &&
        FastIoDispatch->MdlWriteComplete( FileObject, FileOffset, MdlChain, DeviceObject )) {

        NOTHING;

    } else {
        CcMdlWriteComplete2( FileObject, FileOffset, MdlChain );
    }
}

VOID
CcMdlWriteComplete2 (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain
    )

/*++

Routine Description:

    This routine must be called at IPL0 after a call to CcPrepareMdlWrite.
    The caller supplies the ActualLength of data that it actually wrote
    into the buffer, which may be less than or equal to the Length specified
    in CcPrepareMdlWrite.

    This call does the following:

        Makes sure the data up to ActualLength eventually gets written.
        If WriteThrough is FALSE, the data will not be written immediately.
        If WriteThrough is TRUE, then the data is written synchronously.

        Unmaps the pages (if mapped), unlocks them and deletes the MdlChain

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Original file offset read above.

    MdlChain - same as returned from corresponding call to CcPrepareMdlWrite.

Return Value:

    None

--*/

{
    PMDL MdlNext;
    PSHARED_CACHE_MAP SharedCacheMap;
    LARGE_INTEGER FOffset;
    IO_STATUS_BLOCK IoStatus;
    KIRQL OldIrql;
    NTSTATUS StatusToRaise = STATUS_SUCCESS;

    DebugTrace(+1, me, "CcMdlWriteComplete\n", 0 );
    DebugTrace( 0, me, "    FileObject = %08lx\n", FileObject );
    DebugTrace( 0, me, "    MdlChain = %08lx\n", MdlChain );

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    //  Deallocate the Mdls
    //

    FOffset.QuadPart = *(LONGLONG UNALIGNED *)FileOffset;
    while (MdlChain != NULL) {

        MdlNext = MdlChain->Next;

        DebugTrace( 0, mm, "MmUnlockPages/IoFreeMdl:\n", 0 );
        DebugTrace( 0, mm, "    Mdl = %08lx\n", MdlChain );

        //
        //  Now clear the dirty bits in the Pte and set them in the
        //  Pfn.
        //

        MmUnlockPages( MdlChain );

        //
        //  Extract the File Offset for this part of the transfer.
        //

        if (FlagOn(FileObject->Flags, FO_WRITE_THROUGH)) {

            MmFlushSection ( FileObject->SectionObjectPointer,
                             &FOffset,
                             MdlChain->ByteCount,
                             &IoStatus,
                             TRUE );

            //
            //  If we got an I/O error, remember it.
            //

            if (!NT_SUCCESS(IoStatus.Status)) {
                StatusToRaise = IoStatus.Status;
            }

        } else {

            //
            //  Ignore the only exception (allocation error), and console
            //  ourselves for having tried.
            //

            CcSetDirtyInMask( SharedCacheMap, &FOffset, MdlChain->ByteCount );
        }

        FOffset.QuadPart = FOffset.QuadPart + (LONGLONG)(MdlChain->ByteCount);

        IoFreeMdl( MdlChain );

        MdlChain = MdlNext;
    }

    //
    //  Now release our open count.
    //

    CcAcquireMasterLock( &OldIrql );

    CcDecrementOpenCount( SharedCacheMap, 'ldmC' );

    if ((SharedCacheMap->OpenCount == 0) &&
        !FlagOn(SharedCacheMap->Flags, WRITE_QUEUED) &&
        (SharedCacheMap->DirtyPages == 0)) {

        //
        //  Move to the dirty list.
        //

        RemoveEntryList( &SharedCacheMap->SharedCacheMapLinks );
        InsertTailList( &CcDirtySharedCacheMapList.SharedCacheMapLinks,
                        &SharedCacheMap->SharedCacheMapLinks );

        //
        //  Make sure the Lazy Writer will wake up, because we
        //  want him to delete this SharedCacheMap.
        //

        LazyWriter.OtherWork = TRUE;
        if (!LazyWriter.ScanActive) {
            CcScheduleLazyWriteScan();
        }
    }

    CcReleaseMasterLock( OldIrql );

    //
    //  If we got an I/O error, raise it now.
    //

    if (!NT_SUCCESS(StatusToRaise)) {
        FsRtlNormalizeNtstatus( StatusToRaise,
                                STATUS_UNEXPECTED_IO_ERROR );
    }

    DebugTrace(-1, me, "CcMdlWriteComplete -> TRUE\n", 0 );

    return;
}



