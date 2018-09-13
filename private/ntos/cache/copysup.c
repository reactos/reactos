/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    copysup.c

Abstract:

    This module implements the copy support routines for the Cache subsystem.

Author:

    Tom Miller      [TomM]      4-May-1990

Revision History:

--*/

#include "cc.h"

//
//  Define our debug constant
//

#define me 0x00000004


BOOLEAN
CcCopyRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This routine attempts to copy the specified file data from the cache
    into the output buffer, and deliver the correct I/O status.  It is *not*
    safe to call this routine from Dpc level.

    If the caller does not want to block (such as for disk I/O), then
    Wait should be supplied as FALSE.  If Wait was supplied as FALSE and
    it is currently impossible to supply all of the requested data without
    blocking, then this routine will return FALSE.  However, if the
    data is immediately accessible in the cache and no blocking is
    required, this routine copies the data and returns TRUE.

    If the caller supplies Wait as TRUE, then this routine is guaranteed
    to copy the data and return TRUE.  If the data is immediately
    accessible in the cache, then no blocking will occur.  Otherwise,
    the the data transfer from the file into the cache will be initiated,
    and the caller will be blocked until the data can be returned.

    File system Fsd's should typically supply Wait = TRUE if they are
    processing a synchronous I/O requests, or Wait = FALSE if they are
    processing an asynchronous request.

    File system or Server Fsp threads should supply Wait = TRUE.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise (see description
           above)

    Buffer - Pointer to output buffer to which data should be copied.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.  (STATUS_SUCCESS guaranteed for cache
               hits, otherwise the actual I/O status is returned.)

               Note that even if FALSE is returned, the IoStatus.Information
               field will return the count of any bytes successfully
               transferred before a blocking condition occured.  The caller
               may either choose to ignore this information, or resume
               the copy later accounting for bytes transferred.

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not delivered

    TRUE - if the data is being delivered

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PPRIVATE_CACHE_MAP PrivateCacheMap;
    PVOID CacheBuffer;
    LARGE_INTEGER FOffset;
    PVACB Vacb;
    PBCB Bcb;
    PVACB ActiveVacb;
    ULONG ActivePage;
    ULONG PageIsDirty;
    ULONG SavedState;
    ULONG PagesToGo;
    ULONG MoveLength;
    ULONG LengthToGo;
    KIRQL OldIrql;
    NTSTATUS Status;
    ULONG OriginalLength = Length;
    ULONG PageCount = COMPUTE_PAGES_SPANNED((ULongToPtr(FileOffset->LowPart)), Length);
    PETHREAD Thread = PsGetCurrentThread();
    ULONG GotAMiss = 0;

    DebugTrace(+1, me, "CcCopyRead\n", 0 );

    MmSavePageFaultReadAhead( Thread, &SavedState );

    //
    //  Get pointer to shared and private cache maps
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    PrivateCacheMap = FileObject->PrivateCacheMap;

    //
    //  Check for read past file size, the caller must filter this case out.
    //

    ASSERT( ( FileOffset->QuadPart + (LONGLONG)Length) <= SharedCacheMap->FileSize.QuadPart );

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

    FOffset = *FileOffset;

    //
    //  Increment performance counters
    //

    if (Wait) {
        HOT_STATISTIC(CcCopyReadWait) += 1;

        //
        //  This is not an exact solution, but when IoPageRead gets a miss,
        //  it cannot tell whether it was CcCopyRead or CcMdlRead, but since
        //  the miss should occur very soon, by loading the pointer here
        //  probably the right counter will get incremented, and in any case,
        //  we hope the errrors average out!
        //

        CcMissCounter = &CcCopyReadWaitMiss;

    } else {
        HOT_STATISTIC(CcCopyReadNoWait) += 1;
    }

    //
    //  See if we have an active Vacb, that we can just copy to.
    //

    GetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

    if (ActiveVacb != NULL) {

        if ((ULONG)(FOffset.QuadPart >> VACB_OFFSET_SHIFT) == (ActivePage >> (VACB_OFFSET_SHIFT - PAGE_SHIFT))) {

            ULONG LengthToCopy = VACB_MAPPING_GRANULARITY - (FOffset.LowPart & (VACB_MAPPING_GRANULARITY - 1));

            if (SharedCacheMap->NeedToZero != NULL) {
                CcFreeActiveVacb( SharedCacheMap, NULL, 0, FALSE );
            }

            //
            //  Get the starting point in the view.
            //

            CacheBuffer = (PVOID)((PCHAR)ActiveVacb->BaseAddress +
                                          (FOffset.LowPart & (VACB_MAPPING_GRANULARITY - 1)));

            //
            //  Reduce LengthToCopy if it is greater than our caller's length.
            //

            if (LengthToCopy > Length) {
                LengthToCopy = Length;
            }

            //
            //  Like the logic for the normal case below, we want to spin around
            //  making sure Mm only reads the pages we will need.
            //
            
            PagesToGo = COMPUTE_PAGES_SPANNED( CacheBuffer,
                                               LengthToCopy ) - 1;

            //
            //  Copy the data to the user buffer.
            //

            try {

                if (PagesToGo != 0) {
    
                    LengthToGo = LengthToCopy;
    
                    while (LengthToGo != 0) {
    
                        MoveLength = (ULONG)((PCHAR)(ROUND_TO_PAGES(((PCHAR)CacheBuffer + 1))) -
                                     (PCHAR)CacheBuffer);
    
                        if (MoveLength > LengthToGo) {
                            MoveLength = LengthToGo;
                        }
    
                        //
                        //  Here's hoping that it is cheaper to call Mm to see if
                        //  the page is valid.  If not let Mm know how many pages
                        //  we are after before doing the move.
                        //
    
                        MmSetPageFaultReadAhead( Thread, PagesToGo );
                        GotAMiss |= !MmCheckCachedPageState( CacheBuffer, FALSE );
    
                        RtlCopyBytes( Buffer, CacheBuffer, MoveLength );
    
                        PagesToGo -= 1;
    
                        LengthToGo -= MoveLength;
                        Buffer = (PCHAR)Buffer + MoveLength;
                        CacheBuffer = (PCHAR)CacheBuffer + MoveLength;
                    }
    
                //
                //  Handle the read here that stays on a single page.
                //
    
                } else {
    
                    //
                    //  Here's hoping that it is cheaper to call Mm to see if
                    //  the page is valid.  If not let Mm know how many pages
                    //  we are after before doing the move.
                    //
    
                    MmSetPageFaultReadAhead( Thread, 0 );
                    GotAMiss |= !MmCheckCachedPageState( CacheBuffer, FALSE );
    
                    RtlCopyBytes( Buffer, CacheBuffer, LengthToCopy );
    
                    Buffer = (PCHAR)Buffer + LengthToCopy;
                }
                
            } except( CcCopyReadExceptionFilter( GetExceptionInformation(),
                                                 &Status ) ) {

                MmResetPageFaultReadAhead( Thread, SavedState );

                SetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

                //
                //  If we got an access violation, then the user buffer went
                //  away.  Otherwise we must have gotten an I/O error trying
                //  to bring the data in.
                //

                if (Status == STATUS_ACCESS_VIOLATION) {
                    ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
                }
                else {
                    ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                           STATUS_UNEXPECTED_IO_ERROR ));
                }
            }

            //
            //  Now adjust FOffset and Length by what we copied.
            //

            FOffset.QuadPart = FOffset.QuadPart + (LONGLONG)LengthToCopy;
            Length -= LengthToCopy;

        }

        //
        //  If that was all the data, then remember the Vacb
        //

        if (Length == 0) {

            SetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

        //
        //  Otherwise we must free it because we will map other vacbs below.
        //

        } else {

            CcFreeActiveVacb( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );
        }
    }

    //
    //  Not all of the transfer will come back at once, so we have to loop
    //  until the entire transfer is complete.
    //

    while (Length != 0) {

        ULONG ReceivedLength;
        LARGE_INTEGER BeyondLastByte;

        //
        //  Call local routine to Map or Access the file data, then move the data,
        //  then call another local routine to free the data.  If we cannot map
        //  the data because of a Wait condition, return FALSE.
        //
        //  Note that this call may result in an exception, however, if it
        //  does no Bcb is returned and this routine has absolutely no
        //  cleanup to perform.  Therefore, we do not have a try-finally
        //  and we allow the possibility that we will simply be unwound
        //  without notice.
        //

        if (Wait) {

            CacheBuffer = CcGetVirtualAddress( SharedCacheMap,
                                               FOffset,
                                               &Vacb,
                                               &ReceivedLength );

            BeyondLastByte.QuadPart = FOffset.QuadPart + (LONGLONG)ReceivedLength;

        } else if (!CcPinFileData( FileObject,
                                   &FOffset,
                                   Length,
                                   TRUE,
                                   FALSE,
                                   FALSE,
                                   &Bcb,
                                   &CacheBuffer,
                                   &BeyondLastByte )) {

            DebugTrace(-1, me, "CcCopyRead -> FALSE\n", 0 );

            HOT_STATISTIC(CcCopyReadNoWaitMiss) += 1;

            //
            //  Enable ReadAhead if we missed.
            //

            PrivateCacheMap->ReadAheadEnabled = TRUE;

            return FALSE;

        } else {

            //
            //  Calculate how much data is described by Bcb starting at our desired
            //  file offset.
            //

            ReceivedLength = (ULONG)(BeyondLastByte.QuadPart - FOffset.QuadPart);
        }

        //
        //  If we got more than we need, make sure to only transfer
        //  the right amount.
        //

        if (ReceivedLength > Length) {
            ReceivedLength = Length;
        }

        //
        //  It is possible for the user buffer to become no longer accessible
        //  since it was last checked by the I/O system.  If we fail to access
        //  the buffer we must raise a status that the caller's exception
        //  filter considers as "expected".  Also we unmap the Bcb here, since
        //  we otherwise would have no other reason to put a try-finally around
        //  this loop.
        //

        try {

            PagesToGo = COMPUTE_PAGES_SPANNED( CacheBuffer,
                                               ReceivedLength ) - 1;

            //
            //  We know exactly how much we want to read here, and we do not
            //  want to read any more in case the caller is doing random access.
            //  Our read ahead logic takes care of detecting sequential reads,
            //  and tends to do large asynchronous read aheads.  So far we have
            //  only mapped the data and we have not forced any in.  What we
            //  do now is get into a loop where we copy a page at a time and
            //  just prior to each move, we tell MM how many additional pages
            //  we would like to have read in, in the event that we take a
            //  fault.  With this strategy, for cache hits we never make a single
            //  expensive call to MM to guarantee that the data is in, yet if we
            //  do take a fault, we are guaranteed to only take one fault because
            //  we will read all of the data in for the rest of the transfer.
            //
            //  We test first for the multiple page case, to keep the small
            //  reads faster.
            //

            if (PagesToGo != 0) {

                LengthToGo = ReceivedLength;

                while (LengthToGo != 0) {

                    MoveLength = (ULONG)((PCHAR)(ROUND_TO_PAGES(((PCHAR)CacheBuffer + 1))) -
                                 (PCHAR)CacheBuffer);

                    if (MoveLength > LengthToGo) {
                        MoveLength = LengthToGo;
                    }

                    //
                    //  Here's hoping that it is cheaper to call Mm to see if
                    //  the page is valid.  If not let Mm know how many pages
                    //  we are after before doing the move.
                    //

                    MmSetPageFaultReadAhead( Thread, PagesToGo );
                    GotAMiss |= !MmCheckCachedPageState( CacheBuffer, FALSE );

                    RtlCopyBytes( Buffer, CacheBuffer, MoveLength );

                    PagesToGo -= 1;

                    LengthToGo -= MoveLength;
                    Buffer = (PCHAR)Buffer + MoveLength;
                    CacheBuffer = (PCHAR)CacheBuffer + MoveLength;
                }

            //
            //  Handle the read here that stays on a single page.
            //

            } else {

                //
                //  Here's hoping that it is cheaper to call Mm to see if
                //  the page is valid.  If not let Mm know how many pages
                //  we are after before doing the move.
                //

                MmSetPageFaultReadAhead( Thread, 0 );
                GotAMiss |= !MmCheckCachedPageState( CacheBuffer, FALSE );

                RtlCopyBytes( Buffer, CacheBuffer, ReceivedLength );

                Buffer = (PCHAR)Buffer + ReceivedLength;
            }

        }
        except( CcCopyReadExceptionFilter( GetExceptionInformation(),
                                           &Status ) ) {

            CcMissCounter = &CcThrowAway;

            //
            //  If we get an exception, then we have to renable page fault
            //  clustering and unmap on the way out.
            //

            MmResetPageFaultReadAhead( Thread, SavedState );


            if (Wait) {
                CcFreeVirtualAddress( Vacb );
            } else {
                CcUnpinFileData( Bcb, TRUE, UNPIN );
            }

            //
            //  If we got an access violation, then the user buffer went
            //  away.  Otherwise we must have gotten an I/O error trying
            //  to bring the data in.
            //

            if (Status == STATUS_ACCESS_VIOLATION) {
                ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
            }
            else {
                ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                       STATUS_UNEXPECTED_IO_ERROR ));
            }
        }

        //
        //  Update number of bytes transferred.
        //

        Length -= ReceivedLength;

        //
        //  Unmap the data now, and calculate length left to transfer.
        //

        if (Wait) {

            //
            //  If there is more to go, just free this vacb.
            //

            if (Length != 0) {

                CcFreeVirtualAddress( Vacb );

            //
            //  Otherwise save it for the next time through.
            //

            } else {

                SetActiveVacb( SharedCacheMap, OldIrql, Vacb, (ULONG)(FOffset.QuadPart >> PAGE_SHIFT), 0 );
                break;
            }

        } else {
            CcUnpinFileData( Bcb, TRUE, UNPIN );
        }

        //
        //  Assume we did not get all the data we wanted, and set FOffset
        //  to the end of the returned data.
        //

        FOffset = BeyondLastByte;
    }

    MmResetPageFaultReadAhead( Thread, SavedState );

    CcMissCounter = &CcThrowAway;

    //
    //  Now enable read ahead if it looks like we got any misses, and do
    //  the first one.
    //

    if (GotAMiss &&
        !FlagOn( FileObject->Flags, FO_RANDOM_ACCESS ) &&
        !PrivateCacheMap->ReadAheadEnabled) {

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
    IoStatus->Information = OriginalLength;

    DebugTrace(-1, me, "CcCopyRead -> TRUE\n", 0 );

    return TRUE;
}


VOID
CcFastCopyRead (
    IN PFILE_OBJECT FileObject,
    IN ULONG FileOffset,
    IN ULONG Length,
    IN ULONG PageCount,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This routine attempts to copy the specified file data from the cache
    into the output buffer, and deliver the correct I/O status.

    This is a faster version of CcCopyRead which only supports 32-bit file
    offsets and synchronicity (Wait = TRUE).

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    PageCount - Number of pages spanned by the read.

    Buffer - Pointer to output buffer to which data should be copied.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.  (STATUS_SUCCESS guaranteed for cache
               hits, otherwise the actual I/O status is returned.)

               Note that even if FALSE is returned, the IoStatus.Information
               field will return the count of any bytes successfully
               transferred before a blocking condition occured.  The caller
               may either choose to ignore this information, or resume
               the copy later accounting for bytes transferred.

Return Value:

    None

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PPRIVATE_CACHE_MAP PrivateCacheMap;
    PVOID CacheBuffer;
    LARGE_INTEGER FOffset;
    PVACB Vacb;
    PVACB ActiveVacb;
    ULONG ActivePage;
    ULONG PageIsDirty;
    ULONG SavedState;
    ULONG PagesToGo;
    ULONG MoveLength;
    ULONG LengthToGo;
    KIRQL OldIrql;
    NTSTATUS Status;
    LARGE_INTEGER OriginalOffset;
    ULONG OriginalLength = Length;
    PETHREAD Thread = PsGetCurrentThread();
    ULONG GotAMiss = 0;

    DebugTrace(+1, me, "CcFastCopyRead\n", 0 );

    MmSavePageFaultReadAhead( Thread, &SavedState );

    //
    //  Get pointer to shared and private cache maps
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    PrivateCacheMap = FileObject->PrivateCacheMap;

    //
    //  Check for read past file size, the caller must filter this case out.
    //

    ASSERT( (FileOffset + Length) <= SharedCacheMap->FileSize.LowPart );

    //
    //  If read ahead is enabled, then do the read ahead here so it
    //  overlaps with the copy (otherwise we will do it below).
    //  Note that we are assuming that we will not get ahead of our
    //  current transfer - if read ahead is working it should either
    //  already be in memory or else underway.
    //

    OriginalOffset.LowPart = FileOffset;
    OriginalOffset.HighPart = 0;

    if (PrivateCacheMap->ReadAheadEnabled && (PrivateCacheMap->ReadAheadLength[1] == 0)) {
        CcScheduleReadAhead( FileObject, &OriginalOffset, Length );
    }

    //
    //  This is not an exact solution, but when IoPageRead gets a miss,
    //  it cannot tell whether it was CcCopyRead or CcMdlRead, but since
    //  the miss should occur very soon, by loading the pointer here
    //  probably the right counter will get incremented, and in any case,
    //  we hope the errrors average out!
    //

    CcMissCounter = &CcCopyReadWaitMiss;

    //
    //  Increment performance counters
    //

    HOT_STATISTIC(CcCopyReadWait) += 1;

    //
    //  See if we have an active Vacb, that we can just copy to.
    //

    GetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

    if (ActiveVacb != NULL) {

        if ((FileOffset >> VACB_OFFSET_SHIFT) == (ActivePage >> (VACB_OFFSET_SHIFT - PAGE_SHIFT))) {

            ULONG LengthToCopy = VACB_MAPPING_GRANULARITY - (FileOffset & (VACB_MAPPING_GRANULARITY - 1));

            if (SharedCacheMap->NeedToZero != NULL) {
                CcFreeActiveVacb( SharedCacheMap, NULL, 0, FALSE );
            }

            //
            //  Get the starting point in the view.
            //

            CacheBuffer = (PVOID)((PCHAR)ActiveVacb->BaseAddress +
                                          (FileOffset & (VACB_MAPPING_GRANULARITY - 1)));

            //
            //  Reduce LengthToCopy if it is greater than our caller's length.
            //

            if (LengthToCopy > Length) {
                LengthToCopy = Length;
            }

            //
            //  Like the logic for the normal case below, we want to spin around
            //  making sure Mm only reads the pages we will need.
            //
            
            PagesToGo = COMPUTE_PAGES_SPANNED( CacheBuffer,
                                               LengthToCopy ) - 1;

            //
            //  Copy the data to the user buffer.
            //

            try {

                if (PagesToGo != 0) {
    
                    LengthToGo = LengthToCopy;
    
                    while (LengthToGo != 0) {
    
                        MoveLength = (ULONG)((PCHAR)(ROUND_TO_PAGES(((PCHAR)CacheBuffer + 1))) -
                                     (PCHAR)CacheBuffer);
    
                        if (MoveLength > LengthToGo) {
                            MoveLength = LengthToGo;
                        }
    
                        //
                        //  Here's hoping that it is cheaper to call Mm to see if
                        //  the page is valid.  If not let Mm know how many pages
                        //  we are after before doing the move.
                        //
    
                        MmSetPageFaultReadAhead( Thread, PagesToGo );
                        GotAMiss |= !MmCheckCachedPageState( CacheBuffer, FALSE );
    
                        RtlCopyBytes( Buffer, CacheBuffer, MoveLength );
    
                        PagesToGo -= 1;
    
                        LengthToGo -= MoveLength;
                        Buffer = (PCHAR)Buffer + MoveLength;
                        CacheBuffer = (PCHAR)CacheBuffer + MoveLength;
                    }
    
                //
                //  Handle the read here that stays on a single page.
                //
    
                } else {
    
                    //
                    //  Here's hoping that it is cheaper to call Mm to see if
                    //  the page is valid.  If not let Mm know how many pages
                    //  we are after before doing the move.
                    //
    
                    MmSetPageFaultReadAhead( Thread, 0 );
                    GotAMiss |= !MmCheckCachedPageState( CacheBuffer, FALSE );
    
                    RtlCopyBytes( Buffer, CacheBuffer, LengthToCopy );
    
                    Buffer = (PCHAR)Buffer + LengthToCopy;
                }
                
            } except( CcCopyReadExceptionFilter( GetExceptionInformation(),
                                                 &Status ) ) {

                MmResetPageFaultReadAhead( Thread, SavedState );


                SetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

                //
                //  If we got an access violation, then the user buffer went
                //  away.  Otherwise we must have gotten an I/O error trying
                //  to bring the data in.
                //

                if (Status == STATUS_ACCESS_VIOLATION) {
                    ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
                }
                else {
                    ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                           STATUS_UNEXPECTED_IO_ERROR ));
                }
            }

            //
            //  Now adjust FileOffset and Length by what we copied.
            //

            FileOffset += LengthToCopy;
            Length -= LengthToCopy;
        }

        //
        //  If that was all the data, then remember the Vacb
        //

        if (Length == 0) {

            SetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

        //
        //  Otherwise we must free it because we will map other vacbs below.
        //

        } else {

            CcFreeActiveVacb( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );
        }
    }

    //
    //  Not all of the transfer will come back at once, so we have to loop
    //  until the entire transfer is complete.
    //

    FOffset.HighPart = 0;
    FOffset.LowPart = FileOffset;

    while (Length != 0) {

        ULONG ReceivedLength;
        ULONG BeyondLastByte;

        //
        //  Call local routine to Map or Access the file data, then move the data,
        //  then call another local routine to free the data.  If we cannot map
        //  the data because of a Wait condition, return FALSE.
        //
        //  Note that this call may result in an exception, however, if it
        //  does no Bcb is returned and this routine has absolutely no
        //  cleanup to perform.  Therefore, we do not have a try-finally
        //  and we allow the possibility that we will simply be unwound
        //  without notice.
        //

        CacheBuffer = CcGetVirtualAddress( SharedCacheMap,
                                           FOffset,
                                           &Vacb,
                                           &ReceivedLength );

        BeyondLastByte = FOffset.LowPart + ReceivedLength;

        //
        //  If we got more than we need, make sure to only transfer
        //  the right amount.
        //

        if (ReceivedLength > Length) {
            ReceivedLength = Length;
        }

        //
        //  It is possible for the user buffer to become no longer accessible
        //  since it was last checked by the I/O system.  If we fail to access
        //  the buffer we must raise a status that the caller's exception
        //  filter considers as "expected".  Also we unmap the Bcb here, since
        //  we otherwise would have no other reason to put a try-finally around
        //  this loop.
        //

        try {

            PagesToGo = COMPUTE_PAGES_SPANNED( CacheBuffer,
                                               ReceivedLength ) - 1;

            //
            //  We know exactly how much we want to read here, and we do not
            //  want to read any more in case the caller is doing random access.
            //  Our read ahead logic takes care of detecting sequential reads,
            //  and tends to do large asynchronous read aheads.  So far we have
            //  only mapped the data and we have not forced any in.  What we
            //  do now is get into a loop where we copy a page at a time and
            //  just prior to each move, we tell MM how many additional pages
            //  we would like to have read in, in the event that we take a
            //  fault.  With this strategy, for cache hits we never make a single
            //  expensive call to MM to guarantee that the data is in, yet if we
            //  do take a fault, we are guaranteed to only take one fault because
            //  we will read all of the data in for the rest of the transfer.
            //
            //  We test first for the multiple page case, to keep the small
            //  reads faster.
            //

            if (PagesToGo != 0) {

                LengthToGo = ReceivedLength;

                while (LengthToGo != 0) {

                    MoveLength = (ULONG)((PCHAR)(ROUND_TO_PAGES(((PCHAR)CacheBuffer + 1))) -
                                 (PCHAR)CacheBuffer);

                    if (MoveLength > LengthToGo) {
                        MoveLength = LengthToGo;
                    }

                    //
                    //  Here's hoping that it is cheaper to call Mm to see if
                    //  the page is valid.  If not let Mm know how many pages
                    //  we are after before doing the move.
                    //

                    MmSetPageFaultReadAhead( Thread, PagesToGo );
                    GotAMiss |= !MmCheckCachedPageState( CacheBuffer, FALSE );

                    RtlCopyBytes( Buffer, CacheBuffer, MoveLength );

                    PagesToGo -= 1;

                    LengthToGo -= MoveLength;
                    Buffer = (PCHAR)Buffer + MoveLength;
                    CacheBuffer = (PCHAR)CacheBuffer + MoveLength;
                }

            //
            //  Handle the read here that stays on a single page.
            //

            } else {

                //
                //  Here's hoping that it is cheaper to call Mm to see if
                //  the page is valid.  If not let Mm know how many pages
                //  we are after before doing the move.
                //

                MmSetPageFaultReadAhead( Thread, 0 );
                GotAMiss |= !MmCheckCachedPageState( CacheBuffer, FALSE );

                RtlCopyBytes( Buffer, CacheBuffer, ReceivedLength );

                Buffer = (PCHAR)Buffer + ReceivedLength;
            }
        }
        except( CcCopyReadExceptionFilter( GetExceptionInformation(),
                                           &Status ) ) {

            CcMissCounter = &CcThrowAway;

            //
            //  If we get an exception, then we have to renable page fault
            //  clustering and unmap on the way out.
            //

            MmResetPageFaultReadAhead( Thread, SavedState );


            CcFreeVirtualAddress( Vacb );

            //
            //  If we got an access violation, then the user buffer went
            //  away.  Otherwise we must have gotten an I/O error trying
            //  to bring the data in.
            //

            if (Status == STATUS_ACCESS_VIOLATION) {
                ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
            }
            else {
                ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                       STATUS_UNEXPECTED_IO_ERROR ));
            }
        }

        //
        //  Update number of bytes transferred.
        //

        Length -= ReceivedLength;

        //
        //  Unmap the data now, and calculate length left to transfer.
        //

        if (Length != 0) {

            //
            //  If there is more to go, just free this vacb.
            //

            CcFreeVirtualAddress( Vacb );

        } else {

            //
            //  Otherwise save it for the next time through.
            //

            SetActiveVacb( SharedCacheMap, OldIrql, Vacb, (FOffset.LowPart >> PAGE_SHIFT), 0 );
            break;
        }

        //
        //  Assume we did not get all the data we wanted, and set FOffset
        //  to the end of the returned data.
        //

        FOffset.LowPart = BeyondLastByte;
    }

    MmResetPageFaultReadAhead( Thread, SavedState );

    CcMissCounter = &CcThrowAway;

    //
    //  Now enable read ahead if it looks like we got any misses, and do
    //  the first one.
    //

    if (GotAMiss &&
        !FlagOn( FileObject->Flags, FO_RANDOM_ACCESS ) &&
        !PrivateCacheMap->ReadAheadEnabled) {

        PrivateCacheMap->ReadAheadEnabled = TRUE;
        CcScheduleReadAhead( FileObject, &OriginalOffset, OriginalLength );
    }

    //
    //  Now that we have described our desired read ahead, let's
    //  shift the read history down.
    //

    PrivateCacheMap->FileOffset1.LowPart = PrivateCacheMap->FileOffset2.LowPart;
    PrivateCacheMap->BeyondLastByte1.LowPart = PrivateCacheMap->BeyondLastByte2.LowPart;
    PrivateCacheMap->FileOffset2.LowPart = OriginalOffset.LowPart;
    PrivateCacheMap->BeyondLastByte2.LowPart = OriginalOffset.LowPart + OriginalLength;

    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = OriginalLength;

    DebugTrace(-1, me, "CcFastCopyRead -> VOID\n", 0 );
}


BOOLEAN
CcCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine attempts to copy the specified file data from the specified
    buffer into the Cache, and deliver the correct I/O status.  It is *not*
    safe to call this routine from Dpc level.

    If the caller does not want to block (such as for disk I/O), then
    Wait should be supplied as FALSE.  If Wait was supplied as FALSE and
    it is currently impossible to receive all of the requested data without
    blocking, then this routine will return FALSE.  However, if the
    correct space is immediately accessible in the cache and no blocking is
    required, this routine copies the data and returns TRUE.

    If the caller supplies Wait as TRUE, then this routine is guaranteed
    to copy the data and return TRUE.  If the correct space is immediately
    accessible in the cache, then no blocking will occur.  Otherwise,
    the necessary work will be initiated to read and/or free cache data,
    and the caller will be blocked until the data can be received.

    File system Fsd's should typically supply Wait = TRUE if they are
    processing a synchronous I/O requests, or Wait = FALSE if they are
    processing an asynchronous request.

    File system or Server Fsp threads should supply Wait = TRUE.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file to receive the data.

    Length - Length of data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise (see description
           above)

    Buffer - Pointer to input buffer from which data should be copied.

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not copied.

    TRUE - if the data has been copied.

Raises:

    STATUS_INSUFFICIENT_RESOURCES - If a pool allocation failure occurs.
        This can only occur if Wait was specified as TRUE.  (If Wait is
        specified as FALSE, and an allocation failure occurs, this
        routine simply returns FALSE.)

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PFSRTL_ADVANCED_FCB_HEADER FcbHeader;
    PVACB ActiveVacb;
    ULONG ActivePage;
    PVOID ActiveAddress;
    ULONG PageIsDirty;
    KIRQL OldIrql;
    NTSTATUS Status;
    PVOID CacheBuffer;
    LARGE_INTEGER FOffset;
    PBCB Bcb;
    ULONG ZeroFlags;
    LARGE_INTEGER Temp;

    DebugTrace(+1, me, "CcCopyWrite\n", 0 );

    //
    //  If the caller specified Wait == FALSE, but the FileObject is WriteThrough,
    //  then we need to just get out.
    //

    if ((FileObject->Flags & FO_WRITE_THROUGH) && !Wait) {

        DebugTrace(-1, me, "CcCopyWrite->FALSE (WriteThrough && !Wait)\n", 0 );

        return FALSE;
    }

    //
    //  Get pointer to shared cache map
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    FOffset = *FileOffset;

    //
    //  See if we have an active Vacb, that we can just copy to.
    //

    GetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

    if (ActiveVacb != NULL) {

        //
        //  See if the request starts in the ActivePage.  WriteThrough requests must
        //  go the longer route through CcMapAndCopy, where WriteThrough flushes are
        //  implemented.
        //

        if (((ULONG)(FOffset.QuadPart >> PAGE_SHIFT) == ActivePage) && (Length != 0) &&
            !FlagOn( FileObject->Flags, FO_WRITE_THROUGH )) {

            ULONG LengthToCopy = PAGE_SIZE - (FOffset.LowPart & (PAGE_SIZE - 1));

            //
            //  Reduce LengthToCopy if it is greater than our caller's length.
            //

            if (LengthToCopy > Length) {
                LengthToCopy = Length;
            }

            //
            //  Copy the data to the user buffer.
            //

            try {

                //
                //  If we are copying to a page that is locked down, then
                //  we have to do it under our spinlock, and update the
                //  NeedToZero field.
                //

                OldIrql = 0xFF;

                CacheBuffer = (PVOID)((PCHAR)ActiveVacb->BaseAddress +
                                      (FOffset.LowPart & (VACB_MAPPING_GRANULARITY - 1)));

                if (SharedCacheMap->NeedToZero != NULL) {

                    //
                    //  The FastLock may not write our "flag".
                    //

                    OldIrql = 0;

                    ExAcquireFastLock( &SharedCacheMap->ActiveVacbSpinLock, &OldIrql );

                    //
                    //  Note that the NeedToZero could be cleared, since we
                    //  tested it without the spinlock.
                    //

                    ActiveAddress = SharedCacheMap->NeedToZero;
                    if ((ActiveAddress != NULL) &&
                        (ActiveVacb == SharedCacheMap->NeedToZeroVacb) &&
                        (((PCHAR)CacheBuffer + LengthToCopy) > (PCHAR)ActiveAddress)) {

                        //
                        //  If we are skipping some bytes in the page, then we need
                        //  to zero them.
                        //

                        if ((PCHAR)CacheBuffer > (PCHAR)ActiveAddress) {

                            RtlZeroMemory( ActiveAddress, (PCHAR)CacheBuffer - (PCHAR)ActiveAddress );
                        }
                        SharedCacheMap->NeedToZero = (PVOID)((PCHAR)CacheBuffer + LengthToCopy);
                    }

                    ExReleaseFastLock( &SharedCacheMap->ActiveVacbSpinLock, OldIrql );
                }

                RtlCopyBytes( CacheBuffer, Buffer, LengthToCopy );

            } except( CcCopyReadExceptionFilter( GetExceptionInformation(),
                                                 &Status ) ) {

                //
                //  If we failed to overwrite the uninitialized data,
                //  zero it now (we cannot safely restore NeedToZero).
                //

                if (OldIrql != 0xFF) {
                    RtlZeroBytes( CacheBuffer, LengthToCopy );
                }

                SetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, ACTIVE_PAGE_IS_DIRTY );

                //
                //  If we got an access violation, then the user buffer went
                //  away.  Otherwise we must have gotten an I/O error trying
                //  to bring the data in.
                //

                if (Status == STATUS_ACCESS_VIOLATION) {
                    ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
                }
                else {
                    ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                           STATUS_UNEXPECTED_IO_ERROR ));
                }
            }

            //
            //  Now adjust FOffset and Length by what we copied.
            //

            Buffer = (PVOID)((PCHAR)Buffer + LengthToCopy);
            FOffset.QuadPart = FOffset.QuadPart + (LONGLONG)LengthToCopy;
            Length -= LengthToCopy;

            //
            //  If that was all the data, then get outski...
            //

            if (Length == 0) {

                SetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, ACTIVE_PAGE_IS_DIRTY );
                return TRUE;
            }

            //
            //  Remember that the page is dirty now.
            //

            PageIsDirty |= ACTIVE_PAGE_IS_DIRTY;
        }

        CcFreeActiveVacb( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );

    //
    //  Else someone else could have the active page, and may want to zero
    //  the range we plan to write!
    //

    } else if (SharedCacheMap->NeedToZero != NULL) {

        CcFreeActiveVacb( SharedCacheMap, NULL, 0, FALSE );
    }

    //
    //  At this point we can calculate the ZeroFlags.
    //

    //
    //  We can always zero middle pages, if any.
    //

    ZeroFlags = ZERO_MIDDLE_PAGES;

    if (((FOffset.LowPart & (PAGE_SIZE - 1)) == 0) &&
        (Length >= PAGE_SIZE)) {
        ZeroFlags |= ZERO_FIRST_PAGE;
    }

    if (((FOffset.LowPart + Length) & (PAGE_SIZE - 1)) == 0) {
        ZeroFlags |= ZERO_LAST_PAGE;
    }

    Temp = FOffset;
    Temp.LowPart &= ~(PAGE_SIZE -1);

    //
    //  If there is an advanced header, then we can acquire the FastMutex to
    //  make capturing ValidDataLength atomic.  Currently our other file systems
    //  are either RO or do not really support 64-bits.
    //

    FcbHeader = (PFSRTL_ADVANCED_FCB_HEADER)FileObject->FsContext;
    if (FlagOn(FcbHeader->Flags, FSRTL_FLAG_ADVANCED_HEADER)) {
        ExAcquireFastMutex( FcbHeader->FastMutex );
        Temp.QuadPart = ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->ValidDataLength.QuadPart -
                        Temp.QuadPart;
        ExReleaseFastMutex( FcbHeader->FastMutex );
    } else {
        Temp.QuadPart = ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->ValidDataLength.QuadPart -
                        Temp.QuadPart;
    }

    if (Temp.QuadPart <= 0) {
        ZeroFlags |= ZERO_FIRST_PAGE | ZERO_MIDDLE_PAGES | ZERO_LAST_PAGE;
    } else if ((Temp.HighPart == 0) && (Temp.LowPart <= PAGE_SIZE)) {
        ZeroFlags |= ZERO_MIDDLE_PAGES | ZERO_LAST_PAGE;
    }

    //
    //  Call a routine to map and copy the data in Mm and get out.
    //

    if (Wait) {

        CcMapAndCopy( SharedCacheMap,
                      Buffer,
                      &FOffset,
                      Length,
                      ZeroFlags,
                      BooleanFlagOn( FileObject->Flags, FO_WRITE_THROUGH ));

        return TRUE;
    }

    //
    //  The rest of this routine is the Wait == FALSE case.
    //
    //  Not all of the transfer will come back at once, so we have to loop
    //  until the entire transfer is complete.
    //

    while (Length != 0) {

        ULONG ReceivedLength;
        LARGE_INTEGER BeyondLastByte;

        if (!CcPinFileData( FileObject,
                            &FOffset,
                            Length,
                            FALSE,
                            TRUE,
                            FALSE,
                            &Bcb,
                            &CacheBuffer,
                            &BeyondLastByte )) {

            DebugTrace(-1, me, "CcCopyWrite -> FALSE\n", 0 );

            return FALSE;

        } else {

            //
            //  Calculate how much data is described by Bcb starting at our desired
            //  file offset.
            //

            ReceivedLength = (ULONG)(BeyondLastByte.QuadPart - FOffset.QuadPart);

            //
            //  If we got more than we need, make sure to only transfer
            //  the right amount.
            //

            if (ReceivedLength > Length) {
                ReceivedLength = Length;
            }
        }

        //
        //  It is possible for the user buffer to become no longer accessible
        //  since it was last checked by the I/O system.  If we fail to access
        //  the buffer we must raise a status that the caller's exception
        //  filter considers as "expected".  Also we unmap the Bcb here, since
        //  we otherwise would have no other reason to put a try-finally around
        //  this loop.
        //

        try {

            RtlCopyBytes( CacheBuffer, Buffer, ReceivedLength );

            CcSetDirtyPinnedData( Bcb, NULL );
            CcUnpinFileData( Bcb, FALSE, UNPIN );
        }
        except( CcCopyReadExceptionFilter( GetExceptionInformation(),
                                           &Status ) ) {

            CcUnpinFileData( Bcb, TRUE, UNPIN );

            //
            //  If we got an access violation, then the user buffer went
            //  away.  Otherwise we must have gotten an I/O error trying
            //  to bring the data in.
            //

            if (Status == STATUS_ACCESS_VIOLATION) {
                ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
            }
            else {

                ExRaiseStatus(FsRtlNormalizeNtstatus( Status, STATUS_UNEXPECTED_IO_ERROR ));
            }
        }

        //
        //  Assume we did not get all the data we wanted, and set FOffset
        //  to the end of the returned data and adjust the Buffer and Length.
        //

        FOffset = BeyondLastByte;
        Buffer = (PCHAR)Buffer + ReceivedLength;
        Length -= ReceivedLength;
    }

    DebugTrace(-1, me, "CcCopyWrite -> TRUE\n", 0 );

    return TRUE;
}


VOID
CcFastCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN ULONG FileOffset,
    IN ULONG Length,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine attempts to copy the specified file data from the specified
    buffer into the Cache, and deliver the correct I/O status.

    This is a faster version of CcCopyWrite which only supports 32-bit file
    offsets and synchronicity (Wait = TRUE) and no Write Through.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file to receive the data.

    Length - Length of data in bytes.

    Buffer - Pointer to input buffer from which data should be copied.

Return Value:

    None

Raises:

    STATUS_INSUFFICIENT_RESOURCES - If a pool allocation failure occurs.
        This can only occur if Wait was specified as TRUE.  (If Wait is
        specified as FALSE, and an allocation failure occurs, this
        routine simply returns FALSE.)

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PVOID CacheBuffer;
    PVACB ActiveVacb;
    ULONG ActivePage;
    PVOID ActiveAddress;
    ULONG PageIsDirty;
    KIRQL OldIrql;
    NTSTATUS Status;
    ULONG ZeroFlags;
    ULONG ValidDataLength;
    LARGE_INTEGER FOffset;

    DebugTrace(+1, me, "CcFastCopyWrite\n", 0 );

    //
    //  Get pointer to shared cache map and a copy of valid data length
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    //  See if we have an active Vacb, that we can just copy to.
    //

    GetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, PageIsDirty );

    if (ActiveVacb != NULL) {

        //
        //  See if the request starts in the ActivePage.  WriteThrough requests must
        //  go the longer route through CcMapAndCopy, where WriteThrough flushes are
        //  implemented.
        //

        if (((FileOffset >> PAGE_SHIFT) == ActivePage) && (Length != 0) &&
            !FlagOn( FileObject->Flags, FO_WRITE_THROUGH )) {

            ULONG LengthToCopy = PAGE_SIZE - (FileOffset & (PAGE_SIZE - 1));

            //
            //  Reduce LengthToCopy if it is greater than our caller's length.
            //

            if (LengthToCopy > Length) {
                LengthToCopy = Length;
            }

            //
            //  Copy the data to the user buffer.
            //

            try {

                //
                //  If we are copying to a page that is locked down, then
                //  we have to do it under our spinlock, and update the
                //  NeedToZero field.
                //

                OldIrql = 0xFF;

                CacheBuffer = (PVOID)((PCHAR)ActiveVacb->BaseAddress +
                                      (FileOffset & (VACB_MAPPING_GRANULARITY - 1)));

                if (SharedCacheMap->NeedToZero != NULL) {

                    //
                    //  The FastLock may not write our "flag".
                    //

                    OldIrql = 0;

                    ExAcquireFastLock( &SharedCacheMap->ActiveVacbSpinLock, &OldIrql );

                    //
                    //  Note that the NeedToZero could be cleared, since we
                    //  tested it without the spinlock.
                    //

                    ActiveAddress = SharedCacheMap->NeedToZero;
                    if ((ActiveAddress != NULL) &&
                        (ActiveVacb == SharedCacheMap->NeedToZeroVacb) &&
                        (((PCHAR)CacheBuffer + LengthToCopy) > (PCHAR)ActiveAddress)) {

                        //
                        //  If we are skipping some bytes in the page, then we need
                        //  to zero them.
                        //

                        if ((PCHAR)CacheBuffer > (PCHAR)ActiveAddress) {

                            RtlZeroMemory( ActiveAddress, (PCHAR)CacheBuffer - (PCHAR)ActiveAddress );
                        }
                        SharedCacheMap->NeedToZero = (PVOID)((PCHAR)CacheBuffer + LengthToCopy);
                    }

                    ExReleaseFastLock( &SharedCacheMap->ActiveVacbSpinLock, OldIrql );
                }

                RtlCopyBytes( CacheBuffer, Buffer, LengthToCopy );

            } except( CcCopyReadExceptionFilter( GetExceptionInformation(),
                                                 &Status ) ) {

                //
                //  If we failed to overwrite the uninitialized data,
                //  zero it now (we cannot safely restore NeedToZero).
                //

                if (OldIrql != 0xFF) {
                    RtlZeroBytes( CacheBuffer, LengthToCopy );
                }

                SetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, ACTIVE_PAGE_IS_DIRTY );

                //
                //  If we got an access violation, then the user buffer went
                //  away.  Otherwise we must have gotten an I/O error trying
                //  to bring the data in.
                //

                if (Status == STATUS_ACCESS_VIOLATION) {
                    ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
                }
                else {
                    ExRaiseStatus( FsRtlNormalizeNtstatus( Status,
                                                           STATUS_UNEXPECTED_IO_ERROR ));
                }
            }

            //
            //  Now adjust FileOffset and Length by what we copied.
            //

            Buffer = (PVOID)((PCHAR)Buffer + LengthToCopy);
            FileOffset += LengthToCopy;
            Length -= LengthToCopy;

            //
            //  If that was all the data, then get outski...
            //

            if (Length == 0) {

                SetActiveVacb( SharedCacheMap, OldIrql, ActiveVacb, ActivePage, ACTIVE_PAGE_IS_DIRTY );
                return;
            }

            //
            //  Remember that the page is dirty now.
            //

            PageIsDirty |= ACTIVE_PAGE_IS_DIRTY;
        }

        CcFreeActiveVacb( SharedCacheMap, ActiveVacb, ActivePage, PageIsDirty );

    //
    //  Else someone else could have the active page, and may want to zero
    //  the range we plan to write!
    //

    } else if (SharedCacheMap->NeedToZero != NULL) {

        CcFreeActiveVacb( SharedCacheMap, NULL, 0, FALSE );
    }

    //
    //  Set up for call to CcMapAndCopy
    //

    FOffset.LowPart = FileOffset;
    FOffset.HighPart = 0;

    ValidDataLength = ((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->ValidDataLength.LowPart;

    ASSERT((ValidDataLength == MAXULONG) ||
           (((PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext)->ValidDataLength.HighPart == 0));

    //
    //  At this point we can calculate the ReadOnly flag for
    //  the purposes of whether to use the Bcb resource, and
    //  we can calculate the ZeroFlags.
    //

    //
    //  We can always zero middle pages, if any.
    //

    ZeroFlags = ZERO_MIDDLE_PAGES;

    if (((FileOffset & (PAGE_SIZE - 1)) == 0) &&
        (Length >= PAGE_SIZE)) {
        ZeroFlags |= ZERO_FIRST_PAGE;
    }

    if (((FileOffset + Length) & (PAGE_SIZE - 1)) == 0) {
        ZeroFlags |= ZERO_LAST_PAGE;
    }

    if ((FileOffset & ~(PAGE_SIZE - 1)) >= ValidDataLength) {
        ZeroFlags |= ZERO_FIRST_PAGE | ZERO_MIDDLE_PAGES | ZERO_LAST_PAGE;
    } else if (((FileOffset & ~(PAGE_SIZE - 1)) + PAGE_SIZE) >= ValidDataLength) {
        ZeroFlags |= ZERO_MIDDLE_PAGES | ZERO_LAST_PAGE;
    }

    //
    //  Call a routine to map and copy the data in Mm and get out.
    //

    CcMapAndCopy( SharedCacheMap,
                  Buffer,
                  &FOffset,
                  Length,
                  ZeroFlags,
                  BooleanFlagOn( FileObject->Flags, FO_WRITE_THROUGH ));

    DebugTrace(-1, me, "CcFastCopyWrite -> VOID\n", 0 );
}


LONG
CcCopyReadExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN PNTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine serves as a exception filter and has the special job of
    extracting the "real" I/O error when Mm raises STATUS_IN_PAGE_ERROR
    beneath us.

Arguments:

    ExceptionPointer - A pointer to the exception record that contains
                       the real Io Status.

    ExceptionCode - A pointer to an NTSTATUS that is to receive the real
                    status.

Return Value:

    EXCEPTION_EXECUTE_HANDLER

--*/

{
    *ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    if ( (*ExceptionCode == STATUS_IN_PAGE_ERROR) &&
         (ExceptionPointer->ExceptionRecord->NumberParameters >= 3) ) {

        *ExceptionCode = (NTSTATUS) ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    }

    ASSERT( !NT_SUCCESS(*ExceptionCode) );

    return EXCEPTION_EXECUTE_HANDLER;
}


BOOLEAN
CcCanIWrite (
    IN PFILE_OBJECT FileObject,
    IN ULONG BytesToWrite,
    IN BOOLEAN Wait,
    IN UCHAR Retrying
    )

/*++

Routine Description:

    This routine tests whether it is ok to do a write to the cache
    or not, according to the Thresholds of dirty bytes and available
    pages.  The first time this routine is called for a request (Retrying
    FALSE), we automatically make the new request queue if there are other
    requests in the queue.

    Note that the ListEmpty test is important to prevent small requests from sneaking
    in and starving large requests.

Arguments:

    FileObject - for the file to be written

    BytesToWrite - Number of bytes caller wishes to write to the Cache.

    Wait - TRUE if the caller owns no resources, and can block inside this routine
           until it is ok to write.

    Retrying - Specified as FALSE when the request is first received, and
               otherwise specified as TRUE if this write has already entered
               the queue.  Special non-zero value of MAXUCHAR indicates that
               we were called within the cache manager with a MasterSpinLock held,
               so do not attempt to acquire it here.  MAXUCHAR - 1 means we
               were called within the Cache Manager with some other spinlock
               held.  For either of these two special values, we do not touch
               the FsRtl header.

Return Value:

    TRUE if it is ok to write.
    FALSE if the caller should defer the write via a call to CcDeferWrite.

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    KEVENT Event;
    KIRQL OldIrql;
    ULONG PagesToWrite;
    BOOLEAN ExceededPerFileThreshold;
    DEFERRED_WRITE DeferredWrite;
    PSECTION_OBJECT_POINTERS SectionObjectPointers;

    //
    //  Do a special test here for file objects that keep track of dirty
    //  pages on a per-file basis.  This is used mainly for slow links.
    //

    ExceededPerFileThreshold = FALSE;

    PagesToWrite = ((BytesToWrite < WRITE_CHARGE_THRESHOLD ?
                     BytesToWrite : WRITE_CHARGE_THRESHOLD) + (PAGE_SIZE - 1)) / PAGE_SIZE;

    //
    //  Don't dereference the FsContext field if we were called while holding
    //  a spinlock.
    //

    if ((Retrying >= MAXUCHAR - 1) ||

        FlagOn(((PFSRTL_COMMON_FCB_HEADER)(FileObject->FsContext))->Flags,
               FSRTL_FLAG_LIMIT_MODIFIED_PAGES)) {

        if (Retrying != MAXUCHAR) {
            CcAcquireMasterLock( &OldIrql );
        }

        if (((SectionObjectPointers = FileObject->SectionObjectPointer) != NULL) &&
            ((SharedCacheMap = SectionObjectPointers->SharedCacheMap) != NULL) &&
            (SharedCacheMap->DirtyPageThreshold != 0) &&
            (SharedCacheMap->DirtyPages != 0) &&
            ((PagesToWrite + SharedCacheMap->DirtyPages) >
              SharedCacheMap->DirtyPageThreshold)) {

            ExceededPerFileThreshold = TRUE;
        }

        if (Retrying != MAXUCHAR) {
            CcReleaseMasterLock( OldIrql );
        }
    }

    //
    //  See if it is ok to do the write right now
    //

    if ((Retrying || IsListEmpty(&CcDeferredWrites))

                &&

        (CcTotalDirtyPages + PagesToWrite < CcDirtyPageThreshold)

                &&

        MmEnoughMemoryForWrite()

                &&

        !ExceededPerFileThreshold) {

        return TRUE;
    }

    //
    //  Otherwise, if our caller is synchronous, we will just wait here.
    //

    if (IsListEmpty(&CcDeferredWrites) ) {

        //
        // Get a write scan to occur NOW
        //

        KeSetTimer( &LazyWriter.ScanTimer, CcNoDelay, &LazyWriter.ScanDpc );
    }

    if (Wait) {

        KeInitializeEvent( &Event, NotificationEvent, FALSE );

        //
        //  Fill in the block.  Note that we can access the Fsrtl Common Header
        //  even if it's paged because Wait will be FALSE if called from
        //  within the cache.
        //

        DeferredWrite.NodeTypeCode = CACHE_NTC_DEFERRED_WRITE;
        DeferredWrite.NodeByteSize = sizeof(DEFERRED_WRITE);
        DeferredWrite.FileObject = FileObject;
        DeferredWrite.BytesToWrite = BytesToWrite;
        DeferredWrite.Event = &Event;
        DeferredWrite.LimitModifiedPages = BooleanFlagOn(((PFSRTL_COMMON_FCB_HEADER)(FileObject->FsContext))->Flags,
                                                         FSRTL_FLAG_LIMIT_MODIFIED_PAGES);

        //
        //  Now insert at the appropriate end of the list
        //

        if (Retrying) {
            ExInterlockedInsertHeadList( &CcDeferredWrites,
                                         &DeferredWrite.DeferredWriteLinks,
                                         &CcDeferredWriteSpinLock );
        } else {
            ExInterlockedInsertTailList( &CcDeferredWrites,
                                         &DeferredWrite.DeferredWriteLinks,
                                         &CcDeferredWriteSpinLock );
        }

        while (TRUE) {

            //
            //  Now since we really didn't synchronize anything but the insertion,
            //  we call the post routine to make sure that in some wierd case we
            //  do not leave anyone hanging with no dirty bytes for the Lazy Writer.
            //

            CcPostDeferredWrites();

            //
            //  Finally wait until the event is signalled and we can write
            //  and return to tell the guy he can write.
            //

            if (KeWaitForSingleObject( &Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       &CcIdleDelay ) == STATUS_SUCCESS) {


                return TRUE;
            }
        }

    } else {
        return FALSE;
    }
}


VOID
CcDeferWrite (
    IN PFILE_OBJECT FileObject,
    IN PCC_POST_DEFERRED_WRITE PostRoutine,
    IN PVOID Context1,
    IN PVOID Context2,
    IN ULONG BytesToWrite,
    IN BOOLEAN Retrying
    )

/*++

Routine Description:

    This routine may be called to have the Cache Manager defer posting
    of a write until the Lazy Writer makes some progress writing, or
    there are more available pages.  A file system would normally call
    this routine after receiving FALSE from CcCanIWrite, and preparing
    the request to be posted.

Arguments:

    FileObject - for the file to be written

    PostRoutine - Address of the PostRoutine that the Cache Manager can
                  call to post the request when conditions are right.  Note
                  that it is possible that this routine will be called
                  immediately from this routine.

    Context1 - First context parameter for the post routine.

    Context2 - Secont parameter for the post routine.

    BytesToWrite - Number of bytes that the request is trying to write
                   to the cache.

    Retrying - Supplied as FALSE if the request is being posted for the
               first time, TRUE otherwise.

Return Value:

    None

--*/

{
    PDEFERRED_WRITE DeferredWrite;
    KIRQL OldIrql;

    //
    //  Attempt to allocate a deferred write block, and if we do not get
    //  one, just post it immediately rather than gobbling up must succeed
    //  pool.
    //

    DeferredWrite = ExAllocatePoolWithTag( NonPagedPool, sizeof(DEFERRED_WRITE), 'wDcC' );

    if (DeferredWrite == NULL) {
        (*PostRoutine)( Context1, Context2 );
        return;
    }

    //
    //  Fill in the block.
    //

    DeferredWrite->NodeTypeCode = CACHE_NTC_DEFERRED_WRITE;
    DeferredWrite->NodeByteSize = sizeof(DEFERRED_WRITE);
    DeferredWrite->FileObject = FileObject;
    DeferredWrite->BytesToWrite = BytesToWrite;
    DeferredWrite->Event = NULL;
    DeferredWrite->PostRoutine = PostRoutine;
    DeferredWrite->Context1 = Context1;
    DeferredWrite->Context2 = Context2;
    DeferredWrite->LimitModifiedPages = BooleanFlagOn(((PFSRTL_COMMON_FCB_HEADER)(FileObject->FsContext))->Flags,
                                                      FSRTL_FLAG_LIMIT_MODIFIED_PAGES);

    //
    //  Now insert at the appropriate end of the list
    //

    if (Retrying) {
        ExInterlockedInsertHeadList( &CcDeferredWrites,
                                     &DeferredWrite->DeferredWriteLinks,
                                     &CcDeferredWriteSpinLock );
    } else {
        ExInterlockedInsertTailList( &CcDeferredWrites,
                                     &DeferredWrite->DeferredWriteLinks,
                                     &CcDeferredWriteSpinLock );
    }

    //
    //  Now since we really didn't synchronize anything but the insertion,
    //  we call the post routine to make sure that in some wierd case we
    //  do not leave anyone hanging with no dirty bytes for the Lazy Writer.
    //

    CcPostDeferredWrites();

    //
    //  Schedule the lazy writer in case the reason we're blocking
    //  is that we're waiting for Mm (or some other external flag)
    //  to lower and let this write happen.  He will be the one to
    //  keep coming back and checking if this can proceed, even if
    //  there are no cache manager pages to write.
    //
            
    CcAcquireMasterLock( &OldIrql);
            
    if (!LazyWriter.ScanActive) {
        CcScheduleLazyWriteScan();
    }

    CcReleaseMasterLock( OldIrql);
}


VOID
CcPostDeferredWrites (
    )

/*++

Routine Description:

    This routine may be called to see if any deferred writes should be posted
    now, and to post them.  It should be called any time the status of the
    queue may have changed, such as when a new entry has been added, or the
    Lazy Writer has finished writing out buffers and set them clean.

Arguments:

    None

Return Value:

    None

--*/

{
    PDEFERRED_WRITE DeferredWrite;
    ULONG TotalBytesLetLoose = 0;
    KIRQL OldIrql;

    do {

        //
        //  Initially clear the deferred write structure pointer
        //  and syncrhronize.
        //

        DeferredWrite = NULL;

        ExAcquireSpinLock( &CcDeferredWriteSpinLock, &OldIrql );

        //
        //  If the list is empty we are done.
        //

        if (!IsListEmpty(&CcDeferredWrites)) {

            PLIST_ENTRY Entry;

            Entry = CcDeferredWrites.Flink;

            while (Entry != &CcDeferredWrites) {

                DeferredWrite = CONTAINING_RECORD( Entry,
                                                   DEFERRED_WRITE,
                                                   DeferredWriteLinks );

                //
                //  Check for a paranoid case here that TotalBytesLetLoose
                //  wraps.  We stop processing the list at this time.
                //

                TotalBytesLetLoose += DeferredWrite->BytesToWrite;

                if (TotalBytesLetLoose < DeferredWrite->BytesToWrite) {

                    DeferredWrite = NULL;
                    break;
                }

                //
                //  If it is now ok to post this write, remove him from
                //  the list.
                //

                if (CcCanIWrite( DeferredWrite->FileObject,
                                 TotalBytesLetLoose,
                                 FALSE,
                                 MAXUCHAR - 1 )) {

                    RemoveEntryList( &DeferredWrite->DeferredWriteLinks );
                    break;

                //
                //  Otherwise, it is time to stop processing the list, so
                //  we clear the pointer again unless we throttled this item
                //  because of a private dirty page limit.
                //

                } else {

                    //
                    //  If this was a private throttle, skip over it and
                    //  remove its byte count from the running total.
                    //

                    if (DeferredWrite->LimitModifiedPages) {

                        Entry = Entry->Flink;
                        TotalBytesLetLoose -= DeferredWrite->BytesToWrite;
                        DeferredWrite = NULL;
                        continue;

                    } else {

                        DeferredWrite = NULL;

                        break;
                    }
                }
            }
        }

        ExReleaseSpinLock( &CcDeferredWriteSpinLock, OldIrql );

        //
        //  If we got something, set the event or call the post routine
        //  and deallocate the structure.
        //

        if (DeferredWrite != NULL) {

            if (DeferredWrite->Event != NULL) {

                KeSetEvent( DeferredWrite->Event, 0, FALSE );

            } else {

                (*DeferredWrite->PostRoutine)( DeferredWrite->Context1,
                                               DeferredWrite->Context2 );
                ExFreePool( DeferredWrite );
            }
        }

    //
    //  Loop until we find no more work to do.
    //

    } while (DeferredWrite != NULL);
}
