/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FastIo.c

Abstract:

    The Fast I/O path is used to avoid calling the file systems directly to
    do a cached read.  This module is only used if the file object indicates
    that caching is enabled (i.e., the private cache map is not null).

Author:

    Gary Kimura     [GaryKi]    25-Feb-1991

Revision History:

    Tom Miller      [TomM]      14-Apr-1991 Added Fast Write routines

--*/

#include "FsRtlP.h"

//
//  Trace level for the module
//

#define Dbg                              (0x04000000)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlCopyRead)
#pragma alloc_text(PAGE, FsRtlCopyWrite)
#pragma alloc_text(PAGE, FsRtlMdlRead)
#pragma alloc_text(PAGE, FsRtlMdlReadDev)
#pragma alloc_text(PAGE, FsRtlPrepareMdlWrite)
#pragma alloc_text(PAGE, FsRtlPrepareMdlWriteDev)
#pragma alloc_text(PAGE, FsRtlMdlWriteComplete)
#pragma alloc_text(PAGE, FsRtlMdlWriteCompleteDev)
#pragma alloc_text(PAGE, FsRtlAcquireFileForModWrite)
#pragma alloc_text(PAGE, FsRtlReleaseFileForModWrite)
#pragma alloc_text(PAGE, FsRtlAcquireFileForCcFlush)
#pragma alloc_text(PAGE, FsRtlReleaseFileForCcFlush)
#pragma alloc_text(PAGE, FsRtlAcquireFileExclusive)
#pragma alloc_text(PAGE, FsRtlReleaseFile)
#pragma alloc_text(PAGE, FsRtlGetFileSize)
#pragma alloc_text(PAGE, FsRtlSetFileSize)
#endif


BOOLEAN
FsRtlCopyRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.  For a complete description of the arguments
    see CcCopyRead.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise

    Buffer - Pointer to output buffer to which data should be copied.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not delivered, or
        if there is an I/O error.

    TRUE - if the data is being delivered

--*/

{
    PFSRTL_COMMON_FCB_HEADER Header;
    BOOLEAN Status = TRUE;
    ULONG PageCount = COMPUTE_PAGES_SPANNED( FileOffset->QuadPart, Length );
    LARGE_INTEGER BeyondLastByte;
    PDEVICE_OBJECT targetVdo;

    PAGED_CODE();

    //
    //  Special case a read of zero length
    //

    if (Length != 0) {

        //
        //  Check for overflow. Returning false here will re-route this request through the
        //  IRP based path, but this isn't performance critical.
        //

        if (MAXLONGLONG - FileOffset->QuadPart < (LONGLONG)Length) {

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            IoStatus->Information = 0;
            
            return FALSE;
        }
        
        BeyondLastByte.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
        Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

        //
        //  Enter the file system
        //

        FsRtlEnterFileSystem();

        //
        //  Increment performance counters and get the resource
        //

        if (Wait) {

            HOT_STATISTIC(CcFastReadWait) += 1;

            //
            //  Acquired shared on the common fcb header
            //

            (VOID)ExAcquireResourceShared( Header->Resource, TRUE );

        } else {

            HOT_STATISTIC(CcFastReadNoWait) += 1;

            //
            //  Acquired shared on the common fcb header, and return if we
            //  don't get it
            //

            if (!ExAcquireResourceShared( Header->Resource, FALSE )) {

                FsRtlExitFileSystem();

                CcFastReadResourceMiss += 1;

                return FALSE;
            }
        }

        //
        //  Now that the File is acquired shared, we can safely test if it
        //  is really cached and if we can do fast i/o and if not, then
        //  release the fcb and return.
        //

        if ((FileObject->PrivateCacheMap == NULL) ||
            (Header->IsFastIoPossible == FastIoIsNotPossible)) {

            ExReleaseResource( Header->Resource );
            FsRtlExitFileSystem();

            HOT_STATISTIC(CcFastReadNotPossible) += 1;

            return FALSE;
        }

        //
        //  Check if fast I/O is questionable and if so then go ask the
        //  file system the answer
        //

        if (Header->IsFastIoPossible == FastIoIsQuestionable) {

            PFAST_IO_DISPATCH FastIoDispatch;

            ASSERT(!KeIsExecutingDpc());

            targetVdo = IoGetRelatedDeviceObject( FileObject );
            FastIoDispatch = targetVdo->DriverObject->FastIoDispatch;


            //
            //  All file systems that set "Is Questionable" had better support
            // fast I/O
            //

            ASSERT(FastIoDispatch != NULL);
            ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

            //
            //  Call the file system to check for fast I/O.  If the answer is
            //  anything other than GoForIt then we cannot take the fast I/O
            //  path.
            //

            if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                        FileOffset,
                                                        Length,
                                                        Wait,
                                                        LockKey,
                                                        TRUE, // read operation
                                                        IoStatus,
                                                        targetVdo )) {

                //
                //  Fast I/O is not possible so release the Fcb and return.
                //

                ExReleaseResource( Header->Resource );
                FsRtlExitFileSystem();

                HOT_STATISTIC(CcFastReadNotPossible) += 1;

                return FALSE;
            }
        }

        //
        //  Check for read past file size.
        //

        if ( BeyondLastByte.QuadPart > Header->FileSize.QuadPart ) {

            if ( FileOffset->QuadPart >= Header->FileSize.QuadPart ) {
                IoStatus->Status = STATUS_END_OF_FILE;
                IoStatus->Information = 0;

                ExReleaseResource( Header->Resource );
                FsRtlExitFileSystem();

                return TRUE;
            }

            Length = (ULONG)( Header->FileSize.QuadPart - FileOffset->QuadPart );
        }

        //
        //  We can do fast i/o so call the cc routine to do the work and then
        //  release the fcb when we've done.  If for whatever reason the
        //  copy read fails, then return FALSE to our caller.
        //
        //  Also mark this as the top level "Irp" so that lower file system
        //  levels will not attempt a pop-up
        //

        PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

        try {

            if (Wait && ((BeyondLastByte.HighPart | Header->FileSize.HighPart) == 0)) {

                CcFastCopyRead( FileObject,
                                FileOffset->LowPart,
                                Length,
                                PageCount,
                                Buffer,
                                IoStatus );

                FileObject->Flags |= FO_FILE_FAST_IO_READ;

                ASSERT( (IoStatus->Status == STATUS_END_OF_FILE) ||
                        ((FileOffset->LowPart + IoStatus->Information) <= Header->FileSize.LowPart));

            } else {

                Status = CcCopyRead( FileObject,
                                     FileOffset,
                                     Length,
                                     Wait,
                                     Buffer,
                                     IoStatus );

                FileObject->Flags |= FO_FILE_FAST_IO_READ;

                ASSERT( !Status || (IoStatus->Status == STATUS_END_OF_FILE) ||
                        ((LONGLONG)(FileOffset->QuadPart + IoStatus->Information) <= Header->FileSize.QuadPart));
            }

            if (Status) {

                FileObject->CurrentByteOffset.QuadPart = FileOffset->QuadPart + IoStatus->Information;
            }

        } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                        ? EXCEPTION_EXECUTE_HANDLER
                                        : EXCEPTION_CONTINUE_SEARCH ) {

            Status = FALSE;
        }

        PsGetCurrentThread()->TopLevelIrp = 0;

        ExReleaseResource( Header->Resource );
        FsRtlExitFileSystem();
        return Status;

    } else {

        //
        //  A zero length transfer was requested.
        //

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;

        return TRUE;
    }
}


BOOLEAN
FsRtlCopyWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached write bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy write
    of a cached file object.  For a complete description of the arguments
    see CcCopyWrite.

Arguments:

    FileObject - Pointer to the file object being write.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise

    Buffer - Pointer to output buffer to which data should be copied.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not delivered, or
        if there is an I/O error.

    TRUE - if the data is being delivered

--*/

{
    PFSRTL_COMMON_FCB_HEADER Header;
    BOOLEAN AcquiredShared = FALSE;
    BOOLEAN Status = TRUE;
    BOOLEAN FileSizeChanged = FALSE;
    BOOLEAN WriteToEndOfFile = (BOOLEAN)((FileOffset->LowPart == FILE_WRITE_TO_END_OF_FILE) &&
                                         (FileOffset->HighPart == -1));

    PAGED_CODE();

    //
    //  Get a real pointer to the common fcb header
    //

    Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    //  Do we need to verify the volume?  If so, we must go to the file
    //  system.  Also return FALSE if FileObject is write through, the
    //  File System must do that.
    //

    if (CcCanIWrite( FileObject, Length, Wait, FALSE ) &&
        !FlagOn(FileObject->Flags, FO_WRITE_THROUGH) &&
        CcCopyWriteWontFlush(FileObject, FileOffset, Length)) {

        //
        //  Assume our transfer will work
        //

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = Length;

        //
        //  Special case the zero byte length
        //

        if (Length != 0) {

            //
            //  Enter the file system
            //

            FsRtlEnterFileSystem();

            //
            //  Split into separate paths for increased performance.  First
            //  we have the faster path which only supports Wait == TRUE and
            //  32 bits.  We will make an unsafe test on whether the fast path
            //  is ok, then just return FALSE later if we were wrong.  This
            //  should virtually never happen.
            //
            //  IMPORTANT NOTE: It is very important that any changes made to
            //                  this path also be applied to the 64-bit path
            //                  which is the else of this test!
            //

            if (Wait && (Header->AllocationSize.HighPart == 0)) {

                ULONG Offset, NewFileSize;
                ULONG OldFileSize;
                ULONG OldValidDataLength;
                BOOLEAN Wrapped;

                //
                //  Make our best guess on whether we need the file exclusive
                //  or shared.  Note that we do not check FileOffset->HighPart
                //  until below.
                //

                NewFileSize = FileOffset->LowPart + Length;

                if (WriteToEndOfFile || (NewFileSize > Header->ValidDataLength.LowPart)) {

                    //
                    //  Acquired shared on the common fcb header
                    //

                    ExAcquireResourceExclusive( Header->Resource, TRUE );

                } else {

                    //
                    //  Acquired shared on the common fcb header
                    //

                    ExAcquireResourceShared( Header->Resource, TRUE );

                    AcquiredShared = TRUE;
                }

                //
                //  We have the fcb shared now check if we can do fast i/o
                //  and if the file space is allocated, and if not then
                //  release the fcb and return.
                //

                if (WriteToEndOfFile) {

                    Offset = Header->FileSize.LowPart;
                    NewFileSize = Header->FileSize.LowPart + Length;
                    Wrapped = NewFileSize < Header->FileSize.LowPart;

                } else {

                    Offset = FileOffset->LowPart;
                    NewFileSize = FileOffset->LowPart + Length;
                    Wrapped = (NewFileSize < FileOffset->LowPart) || (FileOffset->HighPart != 0);
                }

                //
                //  Now that the File is acquired shared, we can safely test
                //  if it is really cached and if we can do fast i/o and we
                //  do not have to extend. If not then release the fcb and
                //  return.
                //
                //  Get out if we have too much to zero.  This case is not important
                //  for performance, and a file system supporting sparseness may have
                //  a way to do this more efficiently.
                //

                if ((FileObject->PrivateCacheMap == NULL) ||
                    (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                    (NewFileSize > Header->AllocationSize.LowPart) ||
                    (Offset >= (Header->ValidDataLength.LowPart + 0x2000)) ||
                    (Header->AllocationSize.HighPart != 0) || Wrapped) {

                    ExReleaseResource( Header->Resource );
                    FsRtlExitFileSystem();

                    return FALSE;
                }

                //
                //  If we will be extending ValidDataLength, we will have to
                //  get the Fcb exclusive, and make sure that FastIo is still
                //  possible.  We should only execute this block of code very
                //  rarely, when the unsafe test for ValidDataLength failed
                //  above.
                //

                if (AcquiredShared && (NewFileSize > Header->ValidDataLength.LowPart)) {

                    ExReleaseResource( Header->Resource );

                    ExAcquireResourceExclusive( Header->Resource, TRUE );

                    //
                    // If writing to end of file, we must recalculate new size.
                    //

                    if (WriteToEndOfFile) {

                        Offset = Header->FileSize.LowPart;
                        NewFileSize = Header->FileSize.LowPart + Length;
                        Wrapped = NewFileSize < Header->FileSize.LowPart;
                    }

                    if ((FileObject->PrivateCacheMap == NULL) ||
                        (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                        (NewFileSize > Header->AllocationSize.LowPart) ||
                        (Header->AllocationSize.HighPart != 0) || Wrapped) {

                        ExReleaseResource( Header->Resource );
                        FsRtlExitFileSystem();

                        return FALSE;
                    }
                }

                //
                //  Check if fast I/O is questionable and if so then go ask
                //  the file system the answer
                //

                if (Header->IsFastIoPossible == FastIoIsQuestionable) {

                    PDEVICE_OBJECT targetVdo = IoGetRelatedDeviceObject( FileObject );
                    PFAST_IO_DISPATCH FastIoDispatch = targetVdo->DriverObject->FastIoDispatch;
                    IO_STATUS_BLOCK IoStatus;

                    //
                    //  All file system then set "Is Questionable" had better
                    //  support fast I/O
                    //

                    ASSERT(FastIoDispatch != NULL);
                    ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

                    //
                    //  Call the file system to check for fast I/O.  If the
                    //  answer is anything other than GoForIt then we cannot
                    //  take the fast I/O path.
                    //

                    ASSERT(FILE_WRITE_TO_END_OF_FILE == 0xffffffff);

                    if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                                FileOffset->QuadPart != (LONGLONG)-1 ?
                                                                  FileOffset : &Header->FileSize,
                                                                Length,
                                                                TRUE,
                                                                LockKey,
                                                                FALSE, // write operation
                                                                &IoStatus,
                                                                targetVdo )) {

                        //
                        //  Fast I/O is not possible so release the Fcb and
                        //  return.
                        //

                        ExReleaseResource( Header->Resource );
                        FsRtlExitFileSystem();

                        return FALSE;
                    }
                }

                //
                //  Now see if we will change FileSize.  We have to do it now
                //  so that our reads are not nooped.
                //

                if (NewFileSize > Header->FileSize.LowPart) {

                    FileSizeChanged = TRUE;
                    OldFileSize = Header->FileSize.LowPart;
                    OldValidDataLength = Header->ValidDataLength.LowPart;
                    Header->FileSize.LowPart = NewFileSize;
                }

                //
                //  We can do fast i/o so call the cc routine to do the work
                //  and then release the fcb when we've done.  If for whatever
                //  reason the copy write fails, then return FALSE to our
                //  caller.
                //
                //  Also mark this as the top level "Irp" so that lower file
                //  system levels will not attempt a pop-up
                //

                PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

                try {

                    //
                    //  See if we have to do some zeroing
                    //

                    if (Offset > Header->ValidDataLength.LowPart) {

                        LARGE_INTEGER ZeroEnd;

                        ZeroEnd.LowPart = Offset;
                        ZeroEnd.HighPart = 0;

                        CcZeroData( FileObject,
                                    &Header->ValidDataLength,
                                    &ZeroEnd,
                                    TRUE );
                    }

                    CcFastCopyWrite( FileObject,
                                     Offset,
                                     Length,
                                     Buffer );

                } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                                ? EXCEPTION_EXECUTE_HANDLER
                                                : EXCEPTION_CONTINUE_SEARCH ) {

                    Status = FALSE;
                }

                PsGetCurrentThread()->TopLevelIrp = 0;

                //
                //  If we succeeded, see if we have to update FileSize or
                //  ValidDataLength.
                //

                if (Status) {

                    //
                    //  In the case of ValidDataLength, we really have to
                    //  check again since we did not do this when we acquired
                    //  the resource exclusive.
                    //

                    if (NewFileSize > Header->ValidDataLength.LowPart) {

                        Header->ValidDataLength.LowPart = NewFileSize;
                    }

                    //
                    //  Set this handle as having modified the file
                    //

                    FileObject->Flags |= FO_FILE_MODIFIED;

                    if (FileSizeChanged) {

                        CcGetFileSizePointer(FileObject)->LowPart = NewFileSize;

                        FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                    }

                    //
                    //  Also update the file position pointer
                    //

                    FileObject->CurrentByteOffset.LowPart = Offset + Length;
                    FileObject->CurrentByteOffset.HighPart = 0;

                //
                //  If we did not succeed, then we must restore the original
                //  FileSize while holding the PagingIoResource exclusive if
                //  it exists.
                //

                } else if (FileSizeChanged) {

                    if ( Header->PagingIoResource != NULL ) {

                        (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );
                        Header->FileSize.LowPart = OldFileSize;
                        Header->ValidDataLength.LowPart = OldValidDataLength;
                        ExReleaseResource( Header->PagingIoResource );

                    } else {

                        Header->FileSize.LowPart = OldFileSize;
                        Header->ValidDataLength.LowPart = OldValidDataLength;
                    }
                }

            //
            //  Here is the 64-bit or no-wait path.
            //

            } else {

                LARGE_INTEGER Offset, NewFileSize;
                LARGE_INTEGER OldFileSize;
                LARGE_INTEGER OldValidDataLength;

                ASSERT(!KeIsExecutingDpc());

                //
                //  Make our best guess on whether we need the file exclusive
                //  or shared.
                //

                NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;

                if (WriteToEndOfFile || (NewFileSize.QuadPart > Header->ValidDataLength.QuadPart)) {

                    //
                    //  Acquired shared on the common fcb header, and return
                    //  if we don't get it.
                    //

                    if (!ExAcquireResourceExclusive( Header->Resource, Wait )) {

                        FsRtlExitFileSystem();

                        return FALSE;
                    }

                } else {

                    //
                    //  Acquired shared on the common fcb header, and return
                    //  if we don't get it.
                    //

                    if (!ExAcquireResourceShared( Header->Resource, Wait )) {

                        FsRtlExitFileSystem();

                        return FALSE;
                    }

                    AcquiredShared = TRUE;
                }


                //
                //  We have the fcb shared now check if we can do fast i/o
                //  and if the file space is allocated, and if not then
                //  release the fcb and return.
                //

                if (WriteToEndOfFile) {

                    Offset = Header->FileSize;
                    NewFileSize.QuadPart = Header->FileSize.QuadPart + (LONGLONG)Length;

                } else {

                    Offset = *FileOffset;
                    NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
                }

                //
                //  Now that the File is acquired shared, we can safely test
                //  if it is really cached and if we can do fast i/o and we
                //  do not have to extend. If not then release the fcb and
                //  return.
                //
                //  Get out if we are about to zero too much as well, as commented above.
                //  Likewise, for NewFileSizes that exceed MAXLONGLONG.
                //

                if ((FileObject->PrivateCacheMap == NULL) ||
                    (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                      (Offset.QuadPart >= (Header->ValidDataLength.QuadPart + 0x2000)) ||
                      (MAXLONGLONG - Offset.QuadPart < (LONGLONG)Length) ||
                      (NewFileSize.QuadPart > Header->AllocationSize.QuadPart) ) {

                    ExReleaseResource( Header->Resource );
                    FsRtlExitFileSystem();

                    return FALSE;
                }

                //
                //  If we will be extending ValidDataLength, we will have to
                //  get the Fcb exclusive, and make sure that FastIo is still
                //  possible.  We should only execute this block of code very
                //  rarely, when the unsafe test for ValidDataLength failed
                //  above.
                //

                if (AcquiredShared && ( NewFileSize.QuadPart > Header->ValidDataLength.QuadPart )) {

                    ExReleaseResource( Header->Resource );

                    if (!ExAcquireResourceExclusive( Header->Resource, Wait )) {

                        FsRtlExitFileSystem();

                        return FALSE;
                    }

                    //
                    // If writing to end of file, we must recalculate new size.
                    //

                    if (WriteToEndOfFile) {

                        Offset = Header->FileSize;
                        NewFileSize.QuadPart = Header->FileSize.QuadPart + (LONGLONG)Length;
                    }

                    if ((FileObject->PrivateCacheMap == NULL) ||
                        (Header->IsFastIoPossible == FastIoIsNotPossible) ||
                        ( NewFileSize.QuadPart > Header->AllocationSize.QuadPart ) ) {

                        ExReleaseResource( Header->Resource );
                        FsRtlExitFileSystem();

                        return FALSE;
                    }
                }

                //
                //  Check if fast I/O is questionable and if so then go ask
                //  the file system the answer
                //

                if (Header->IsFastIoPossible == FastIoIsQuestionable) {

                    PFAST_IO_DISPATCH FastIoDispatch = IoGetRelatedDeviceObject( FileObject )->DriverObject->FastIoDispatch;
                    IO_STATUS_BLOCK IoStatus;

                    //
                    //  All file system then set "Is Questionable" had better
                    //  support fast I/O
                    //

                    ASSERT(FastIoDispatch != NULL);
                    ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

                    //
                    //  Call the file system to check for fast I/O.  If the
                    //  answer is anything other than GoForIt then we cannot
                    //  take the fast I/O path.
                    //

                    ASSERT(FILE_WRITE_TO_END_OF_FILE == 0xffffffff);

                    if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                                FileOffset->QuadPart != (LONGLONG)-1 ?
                                                                  FileOffset : &Header->FileSize,
                                                                Length,
                                                                Wait,
                                                                LockKey,
                                                                FALSE, // write operation
                                                                &IoStatus,
                                                                DeviceObject )) {

                        //
                        //  Fast I/O is not possible so release the Fcb and
                        //  return.
                        //

                        ExReleaseResource( Header->Resource );
                        FsRtlExitFileSystem();

                        return FALSE;
                    }
                }

                //
                //  Now see if we will change FileSize.  We have to do it now
                //  so that our reads are not nooped.
                //

                if ( NewFileSize.QuadPart > Header->FileSize.QuadPart ) {

                    FileSizeChanged = TRUE;
                    OldFileSize = Header->FileSize;
                    OldValidDataLength = Header->ValidDataLength;

                    //
                    //  Deal with an extremely rare pathalogical case here the
                    //  file size wraps.
                    //

                    if ( (Header->FileSize.HighPart != NewFileSize.HighPart) &&
                         (Header->PagingIoResource != NULL) ) {

                        (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );
                        Header->FileSize = NewFileSize;
                        ExReleaseResource( Header->PagingIoResource );

                    } else {

                        Header->FileSize = NewFileSize;
                    }
                }

                //
                //  We can do fast i/o so call the cc routine to do the work
                //  and then release the fcb when we've done.  If for whatever
                //  reason the copy write fails, then return FALSE to our
                //  caller.
                //
                //  Also mark this as the top level "Irp" so that lower file
                //  system levels will not attempt a pop-up
                //

                PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

                try {

                    //
                    //  See if we have to do some zeroing
                    //

                    if ( Offset.QuadPart > Header->ValidDataLength.QuadPart ) {

                        Status = CcZeroData( FileObject,
                                             &Header->ValidDataLength,
                                             &Offset,
                                             Wait );
                    }

                    if (Status) {

                        Status = CcCopyWrite( FileObject,
                                              &Offset,
                                              Length,
                                              Wait,
                                              Buffer );
                    }

                } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                                ? EXCEPTION_EXECUTE_HANDLER
                                                : EXCEPTION_CONTINUE_SEARCH ) {

                    Status = FALSE;
                }

                PsGetCurrentThread()->TopLevelIrp = 0;

                //
                //  If we succeeded, see if we have to update FileSize or
                //  ValidDataLength.
                //

                if (Status) {

                    //
                    //  In the case of ValidDataLength, we really have to
                    //  check again since we did not do this when we acquired
                    //  the resource exclusive.
                    //

                    if ( NewFileSize.QuadPart > Header->ValidDataLength.QuadPart ) {

                        //
                        //  Deal with an extremely rare pathalogical case here
                        //  the ValidDataLength wraps.
                        //

                        if ( (Header->ValidDataLength.HighPart != NewFileSize.HighPart) &&
                             (Header->PagingIoResource != NULL) ) {

                            (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );
                            Header->ValidDataLength = NewFileSize;
                            ExReleaseResource( Header->PagingIoResource );

                        } else {

                            Header->ValidDataLength = NewFileSize;
                        }
                    }

                    //
                    //  Set this handle as having modified the file
                    //

                    FileObject->Flags |= FO_FILE_MODIFIED;

                    if (FileSizeChanged) {

                        *CcGetFileSizePointer(FileObject) = NewFileSize;

                        FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                    }

                    //
                    //  Also update the current file position pointer
                    //

                    FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Length;

                //
                // If we did not succeed, then we must restore the original
                // FileSize while holding the PagingIoResource exclusive if
                // it exists.
                //

                } else if (FileSizeChanged) {

                    if ( Header->PagingIoResource != NULL ) {

                        (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );
                        Header->FileSize = OldFileSize;
                        Header->ValidDataLength = OldValidDataLength;
                        ExReleaseResource( Header->PagingIoResource );

                    } else {

                        Header->FileSize = OldFileSize;
                        Header->ValidDataLength = OldValidDataLength;
                    }
                }

            }

            ExReleaseResource( Header->Resource );
            FsRtlExitFileSystem();

            return Status;

        } else {

            //
            //  A zero length transfer was requested.
            //

            return TRUE;
        }

    } else {

        //
        // The volume must be verified or the file is write through.
        //

        return FALSE;
    }
}


BOOLEAN
FsRtlMdlReadDev (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.  For a complete description of the arguments
    see CcMdlRead.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    MdlChain - On output it returns a pointer to an MDL chain describing
        the desired data.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

    DeviceObject - Supplies DeviceObject for callee.

Return Value:

    FALSE - if the data was not delivered, or if there is an I/O error.

    TRUE - if the data is being delivered

--*/

{
    PFSRTL_COMMON_FCB_HEADER Header;
    BOOLEAN Status = TRUE;
    LARGE_INTEGER BeyondLastByte;

    PAGED_CODE();

    //
    //  Special case a read of zero length
    //

    if (Length == 0) {

        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;

        return TRUE;
    }

    //
    //  Overflows should've been handled by caller.
    //

    ASSERT(MAXLONGLONG - FileOffset->QuadPart >= (LONGLONG)Length);

       
    //
    //  Get a real pointer to the common fcb header
    //

    BeyondLastByte.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
    Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    //  Enter the file system
    //

    FsRtlEnterFileSystem();

    CcFastMdlReadWait += 1;

    //
    //  Acquired shared on the common fcb header
    //

    (VOID)ExAcquireResourceShared( Header->Resource, TRUE );

    //
    //  Now that the File is acquired shared, we can safely test if it is
    //  really cached and if we can do fast i/o and if not
    //  then release the fcb and return.
    //

    if ((FileObject->PrivateCacheMap == NULL) ||
        (Header->IsFastIoPossible == FastIoIsNotPossible)) {

        ExReleaseResource( Header->Resource );
        FsRtlExitFileSystem();

        CcFastMdlReadNotPossible += 1;

        return FALSE;
    }

    //
    //  Check if fast I/O is questionable and if so then go ask the file system
    //  the answer
    //

    if (Header->IsFastIoPossible == FastIoIsQuestionable) {

        PFAST_IO_DISPATCH FastIoDispatch;

        ASSERT(!KeIsExecutingDpc());

        FastIoDispatch = IoGetRelatedDeviceObject( FileObject )->DriverObject->FastIoDispatch;


        //
        //  All file system then set "Is Questionable" had better support fast I/O
        //

        ASSERT(FastIoDispatch != NULL);
        ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

        //
        //  Call the file system to check for fast I/O.  If the answer is anything
        //  other than GoForIt then we cannot take the fast I/O path.
        //

        if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                    FileOffset,
                                                    Length,
                                                    TRUE,
                                                    LockKey,
                                                    TRUE, // read operation
                                                    IoStatus,
                                                    IoGetRelatedDeviceObject( FileObject ) )) {

            //
            //  Fast I/O is not possible so release the Fcb and return.
            //

            ExReleaseResource( Header->Resource );
            FsRtlExitFileSystem();

            CcFastMdlReadNotPossible += 1;

            return FALSE;
        }
    }

    //
    //  Check for read past file size.
    //

    if ( BeyondLastByte.QuadPart > Header->FileSize.QuadPart ) {

        if ( FileOffset->QuadPart >= Header->FileSize.QuadPart ) {
            IoStatus->Status = STATUS_END_OF_FILE;
            IoStatus->Information = 0;

            ExReleaseResource( Header->Resource );
            FsRtlExitFileSystem();

            return TRUE;
        }

        Length = (ULONG)( Header->FileSize.QuadPart - FileOffset->QuadPart );
    }

    //
    //  We can do fast i/o so call the cc routine to do the work and then
    //  release the fcb when we've done.  If for whatever reason the
    //  mdl read fails, then return FALSE to our caller.
    //
    //
    //  Also mark this as the top level "Irp" so that lower file system levels
    //  will not attempt a pop-up
    //

    PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

    try {

        CcMdlRead( FileObject, FileOffset, Length, MdlChain, IoStatus );

        FileObject->Flags |= FO_FILE_FAST_IO_READ;

    } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                   ? EXCEPTION_EXECUTE_HANDLER
                                   : EXCEPTION_CONTINUE_SEARCH ) {

        Status = FALSE;
    }

    PsGetCurrentThread()->TopLevelIrp = 0;

    ExReleaseResource( Header->Resource );
    FsRtlExitFileSystem();

    return Status;
}


//
//  The old routine will either dispatch or call FsRtlMdlReadDev
//

BOOLEAN
FsRtlMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.  For a complete description of the arguments
    see CcMdlRead.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    MdlChain - On output it returns a pointer to an MDL chain describing
        the desired data.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

Return Value:

    FALSE - if the data was not delivered, or if there is an I/O error.

    TRUE - if the data is being delivered

--*/

{
    PDEVICE_OBJECT DeviceObject, VolumeDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    //
    //  See if the (top-level) FileSystem has a FastIo routine, and if so, call it.
    //

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlRead)) &&
        (FastIoDispatch->MdlRead != NULL)) {

        return FastIoDispatch->MdlRead( FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject );

    } else {

        //
        //  Get the DeviceObject for the volume.  If that DeviceObject is different, and
        //  it specifies the FastIo routine, then we have to return FALSE here and cause
        //  an Irp to get generated.
        //

        VolumeDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );
        if ((VolumeDeviceObject != DeviceObject) &&
            (FastIoDispatch = VolumeDeviceObject->DriverObject->FastIoDispatch) &&
            (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlRead)) &&
            (FastIoDispatch->MdlRead != NULL)) {

            return FALSE;

        //
        //  Otherwise, call the default routine.
        //

        } else {

            return FsRtlMdlReadDev( FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject );
        }
    }
}


//
//  The old routine will either dispatch or call FsRtlMdlReadCompleteDev
//

BOOLEAN
FsRtlMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.

Arguments:

    FileObject - Pointer to the file object being read.

    MdlChain - Supplies a pointer to an MDL chain returned from CcMdlRead.

Return Value:

    None

--*/

{
    PDEVICE_OBJECT DeviceObject, VolumeDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    //
    //  See if the (top-level) FileSystem has a FastIo routine, and if so, call it.
    //

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlReadComplete)) &&
        (FastIoDispatch->MdlReadComplete != NULL)) {

        return FastIoDispatch->MdlReadComplete( FileObject, MdlChain, DeviceObject );

    } else {

        //
        //  Get the DeviceObject for the volume.  If that DeviceObject is different, and
        //  it specifies the FastIo routine, then we have to return FALSE here and cause
        //  an Irp to get generated.
        //

        VolumeDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );
        if ((VolumeDeviceObject != DeviceObject) &&
            (FastIoDispatch = VolumeDeviceObject->DriverObject->FastIoDispatch) &&
            (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlReadComplete)) &&
            (FastIoDispatch->MdlReadComplete != NULL)) {

            return FALSE;

        //
        //  Otherwise, call the default routine.
        //

        } else {

            return FsRtlMdlReadCompleteDev( FileObject, MdlChain, DeviceObject );
        }
    }
}


BOOLEAN
FsRtlMdlReadCompleteDev (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.

Arguments:

    FileObject - Pointer to the file object being read.

    MdlChain - Supplies a pointer to an MDL chain returned from CcMdlRead.

    DeviceObject - Supplies the DeviceObject for the callee.

Return Value:

    None

--*/


{
    CcMdlReadComplete2( FileObject, MdlChain );
    return TRUE;
}


BOOLEAN
FsRtlPrepareMdlWriteDev (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.  For a complete description of the arguments
    see CcMdlRead.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    MdlChain - On output it returns a pointer to an MDL chain describing
        the desired data.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

    DeviceObject - Supplies the DeviceObject for the callee.

Return Value:

    FALSE - if the data was not written, or if there is an I/O error.

    TRUE - if the data is being written

--*/

{
    PFSRTL_COMMON_FCB_HEADER Header;
    LARGE_INTEGER Offset, NewFileSize;
    LARGE_INTEGER OldFileSize;
    LARGE_INTEGER OldValidDataLength;
    BOOLEAN Status = TRUE;
    BOOLEAN AcquiredShared = FALSE;
    BOOLEAN FileSizeChanged = FALSE;
    BOOLEAN WriteToEndOfFile = (BOOLEAN)((FileOffset->LowPart == FILE_WRITE_TO_END_OF_FILE) &&
                                         (FileOffset->HighPart == -1));

    PAGED_CODE();

    //
    //  Call CcCanIWrite.  Also return FALSE if FileObject is write through,
    //  the File System must do that.
    //

    if ( !CcCanIWrite( FileObject, Length, TRUE, FALSE ) ||
         FlagOn( FileObject->Flags, FO_WRITE_THROUGH )) {

        return FALSE;
    }

    //
    //  Assume our transfer will work
    //

    IoStatus->Status = STATUS_SUCCESS;

    //
    //  Special case the zero byte length
    //

    if (Length == 0) {

        return TRUE;
    }

    //
    //  Get a real pointer to the common fcb header
    //

    Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    //  Enter the file system
    //

    FsRtlEnterFileSystem();

    //
    //  Make our best guess on whether we need the file exclusive or
    //  shared.
    //

    NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;

    if (WriteToEndOfFile || (NewFileSize.QuadPart > Header->ValidDataLength.QuadPart)) {

        //
        //  Acquired exclusive on the common fcb header, and return if we don't
        //  get it.
        //

        ExAcquireResourceExclusive( Header->Resource, TRUE );

    } else {

        //
        //  Acquired shared on the common fcb header, and return if we don't
        //  get it.
        //

        ExAcquireResourceShared( Header->Resource, TRUE );

        AcquiredShared = TRUE;
    }


    //
    //  We have the fcb shared now check if we can do fast i/o  and if the file
    //  space is allocated, and if not then release the fcb and return.
    //

    if (WriteToEndOfFile) {

        Offset = Header->FileSize;
        NewFileSize.QuadPart = Header->FileSize.QuadPart + (LONGLONG)Length;

    } else {

        Offset = *FileOffset;
        NewFileSize.QuadPart = FileOffset->QuadPart + (LONGLONG)Length;
    }

    //
    //  Now that the File is acquired shared, we can safely test if it is
    //  really cached and if we can do fast i/o and we do not have to extend.
    //  If not then release the fcb and return.
    //

    if ((FileObject->PrivateCacheMap == NULL) ||
        (Header->IsFastIoPossible == FastIoIsNotPossible) ||
        (MAXLONGLONG - Offset.QuadPart < (LONGLONG)Length) ||
        ( NewFileSize.QuadPart > Header->AllocationSize.QuadPart ) ) {

        ExReleaseResource( Header->Resource );
        FsRtlExitFileSystem();

        return FALSE;
    }

    //
    //  If we will be extending ValidDataLength, we will have to get the
    //  Fcb exclusive, and make sure that FastIo is still possible.
    //

    if (AcquiredShared && ( NewFileSize.QuadPart > Header->ValidDataLength.QuadPart )) {

        ExReleaseResource( Header->Resource );

        ExAcquireResourceExclusive( Header->Resource, TRUE );

        AcquiredShared = FALSE;

        //
        //  If writing to end of file, we must recalculate new size.
        //

        if (WriteToEndOfFile) {

            Offset = Header->FileSize;
            NewFileSize.QuadPart = Header->FileSize.QuadPart + (LONGLONG)Length;
        }

        if ((FileObject->PrivateCacheMap == NULL) ||
            (Header->IsFastIoPossible == FastIoIsNotPossible) ||
            ( NewFileSize.QuadPart > Header->AllocationSize.QuadPart )) {

            ExReleaseResource( Header->Resource );
            FsRtlExitFileSystem();

            return FALSE;
        }
    }

    //
    //  Check if fast I/O is questionable and if so then go ask the file system
    //  the answer
    //

    if (Header->IsFastIoPossible == FastIoIsQuestionable) {

        PFAST_IO_DISPATCH FastIoDispatch = IoGetRelatedDeviceObject( FileObject )->DriverObject->FastIoDispatch;

        //
        //  All file system then set "Is Questionable" had better support fast I/O
        //

        ASSERT(FastIoDispatch != NULL);
        ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

        //
        //  Call the file system to check for fast I/O.  If the answer is anything
        //  other than GoForIt then we cannot take the fast I/O path.
        //

        if (!FastIoDispatch->FastIoCheckIfPossible( FileObject,
                                                    FileOffset,
                                                    Length,
                                                    TRUE,
                                                    LockKey,
                                                    FALSE, // write operation
                                                    IoStatus,
                                                    IoGetRelatedDeviceObject( FileObject ) )) {

            //
            //  Fast I/O is not possible so release the Fcb and return.
            //

            ExReleaseResource( Header->Resource );
            FsRtlExitFileSystem();

            return FALSE;
        }
    }

    //
    // Now see if we will change FileSize.  We have to do it now so that our
    // reads are not nooped.
    //

    if ( NewFileSize.QuadPart > Header->FileSize.QuadPart ) {

        FileSizeChanged = TRUE;
        OldFileSize = Header->FileSize;
        OldValidDataLength = Header->ValidDataLength;

        //
        //  Deal with an extremely rare pathalogical case here the file
        //  size wraps.
        //

        if ( (Header->FileSize.HighPart != NewFileSize.HighPart) &&
             (Header->PagingIoResource != NULL) ) {

            (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );
            Header->FileSize = NewFileSize;
            ExReleaseResource( Header->PagingIoResource );

        } else {

            Header->FileSize = NewFileSize;
        }
    }

    //
    //  We can do fast i/o so call the cc routine to do the work and then
    //  release the fcb when we've done.  If for whatever reason the
    //  copy write fails, then return FALSE to our caller.
    //
    //
    //  Also mark this as the top level "Irp" so that lower file system levels
    //  will not attempt a pop-up
    //

    PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

    try {

        //
        //  See if we have to do some zeroing
        //

        if ( Offset.QuadPart > Header->ValidDataLength.QuadPart ) {

            Status = CcZeroData( FileObject,
                                 &Header->ValidDataLength,
                                 &Offset,
                                 TRUE );
        }

        if (Status) {

            CcPrepareMdlWrite( FileObject, &Offset, Length, MdlChain, IoStatus );
        }

    } except( FsRtlIsNtstatusExpected(GetExceptionCode())
                                    ? EXCEPTION_EXECUTE_HANDLER
                                    : EXCEPTION_CONTINUE_SEARCH ) {

        Status = FALSE;
    }

    PsGetCurrentThread()->TopLevelIrp = 0;

    //
    //  If we succeeded, see if we have to update FileSize or ValidDataLength.
    //

    if (Status) {

        //
        // In the case of ValidDataLength, we really have to check again
        // since we did not do this when we acquired the resource exclusive.
        //

        if ( NewFileSize.QuadPart > Header->ValidDataLength.QuadPart ) {

            //
            //  Deal with an extremely rare pathalogical case here the
            //  ValidDataLength wraps.
            //

            if ( (Header->ValidDataLength.HighPart != NewFileSize.HighPart) &&
                 (Header->PagingIoResource != NULL) ) {

                (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );
                Header->ValidDataLength = NewFileSize;
                ExReleaseResource( Header->PagingIoResource );

            } else {

                Header->ValidDataLength = NewFileSize;
            }
        }

        //
        //  Set this handle as having modified the file
        //

        FileObject->Flags |= FO_FILE_MODIFIED;

        if (FileSizeChanged) {

            *CcGetFileSizePointer(FileObject) = NewFileSize;

            FileObject->Flags |= FO_FILE_SIZE_CHANGED;
        }

    //
    //  If we did not succeed, then we must restore the original FileSize
    //  and release the resource.  In the success path, the cache manager
    //  will release the resource.
    //

    } else {

        if (FileSizeChanged) {

            if ( Header->PagingIoResource != NULL ) {

                (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );
                Header->FileSize = OldFileSize;
                Header->ValidDataLength = OldValidDataLength;
                ExReleaseResource( Header->PagingIoResource );

            } else {

                Header->FileSize = OldFileSize;
                Header->ValidDataLength = OldValidDataLength;
            }
        }
    }

    //
    //  Now we can release the resource.
    //

    ExReleaseResource( Header->Resource );

    FsRtlExitFileSystem();

    return Status;
}


//
//  The old routine will either dispatch or call FsRtlPrepareMdlWriteDev
//

BOOLEAN
FsRtlPrepareMdlWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This routine does a fast cached mdl read bypassing the usual file system
    entry routine (i.e., without the Irp).  It is used to do a copy read
    of a cached file object.  For a complete description of the arguments
    see CcMdlRead.

Arguments:

    FileObject - Pointer to the file object being read.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    MdlChain - On output it returns a pointer to an MDL chain describing
        the desired data.

    IoStatus - Pointer to standard I/O status block to receive the status
               for the transfer.

Return Value:

    FALSE - if the data was not written, or if there is an I/O error.

    TRUE - if the data is being written

--*/

{
    PDEVICE_OBJECT DeviceObject, VolumeDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    //
    //  See if the (top-level) FileSystem has a FastIo routine, and if so, call it.
    //

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, PrepareMdlWrite)) &&
        (FastIoDispatch->PrepareMdlWrite != NULL)) {

        return FastIoDispatch->PrepareMdlWrite( FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject );

    } else {

        //
        //  Get the DeviceObject for the volume.  If that DeviceObject is different, and
        //  it specifies the FastIo routine, then we have to return FALSE here and cause
        //  an Irp to get generated.
        //

        VolumeDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );
        if ((VolumeDeviceObject != DeviceObject) &&
            (FastIoDispatch = VolumeDeviceObject->DriverObject->FastIoDispatch) &&
            (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, PrepareMdlWrite)) &&
            (FastIoDispatch->PrepareMdlWrite != NULL)) {

            return FALSE;

        //
        //  Otherwise, call the default routine.
        //

        } else {

            return FsRtlPrepareMdlWriteDev( FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject );
        }
    }
}


//
//  The old routine will either dispatch or call FsRtlMdlWriteCompleteDev
//

BOOLEAN
FsRtlMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain
    )

/*++

Routine Description:

    This routine completes an Mdl write.

Arguments:

    FileObject - Pointer to the file object being read.

    MdlChain - Supplies a pointer to an MDL chain returned from CcMdlPrepareMdlWrite.

Return Value:



--*/

{
    PDEVICE_OBJECT DeviceObject, VolumeDeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    //
    //  See if the (top-level) FileSystem has a FastIo routine, and if so, call it.
    //

    if ((FastIoDispatch != NULL) &&
        (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlWriteComplete)) &&
        (FastIoDispatch->MdlWriteComplete != NULL)) {

        return FastIoDispatch->MdlWriteComplete( FileObject, FileOffset, MdlChain, DeviceObject );

    } else {

        //
        //  Get the DeviceObject for the volume.  If that DeviceObject is different, and
        //  it specifies the FastIo routine, then we have to return FALSE here and cause
        //  an Irp to get generated.
        //

        VolumeDeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );
        if ((VolumeDeviceObject != DeviceObject) &&
            (FastIoDispatch = VolumeDeviceObject->DriverObject->FastIoDispatch) &&
            (FastIoDispatch->SizeOfFastIoDispatch > FIELD_OFFSET(FAST_IO_DISPATCH, MdlWriteComplete)) &&
            (FastIoDispatch->MdlWriteComplete != NULL)) {

            return FALSE;

        //
        //  Otherwise, call the default routine.
        //

        } else {

            return FsRtlMdlWriteCompleteDev( FileObject, FileOffset, MdlChain, DeviceObject );
        }
    }
}


BOOLEAN
FsRtlMdlWriteCompleteDev (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine completes an Mdl write.

Arguments:

    FileObject - Pointer to the file object being read.

    MdlChain - Supplies a pointer to an MDL chain returned from CcMdlPrepareMdlWrite.

    DeviceObject - Supplies the DeviceObject for the callee.

Return Value:



--*/


{
    //
    //  Do not support WRITE_THROUGH in the fast path call.
    //

    if (FlagOn( FileObject->Flags, FO_WRITE_THROUGH )) {
        return FALSE;
    }

    CcMdlWriteComplete2( FileObject, FileOffset, MdlChain );
    return TRUE;
}


NTKERNELAPI
BOOLEAN
FsRtlAcquireFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease
    )

/*++

Routine Description:

    This routine decides which file system resource the modified page
    writer should acquire and acquires it if possible.  Wait is always
    specified as FALSE.  We pass back the resource Mm has to release
    when the write completes.

Arguments:

    FileObject - Pointer to the file object being written.

    EndingOffset - The offset of the last byte being written + 1.

    ByteCount - Length of data in bytes.

    ResourceToRelease - Returns the resource to release.  Not defined if
        FALSE is returned.

Return Value:

    FALSE - The resource could not be acquired without waiting.

    TRUE - The returned resource has been acquired.

--*/

{
    PFSRTL_COMMON_FCB_HEADER Header;
    PERESOURCE ResourceAcquired;
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;

    BOOLEAN AcquireExclusive;

    PAGED_CODE();

    Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    //  First see if we have to call the file system.
    //

    DeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;
    if ((FastIoDispatch->SizeOfFastIoDispatch >
         FIELD_OFFSET( FAST_IO_DISPATCH, AcquireForModWrite )) &&
        (FastIoDispatch->AcquireForModWrite != NULL)) {

        NTSTATUS Status;

        Status = FastIoDispatch->AcquireForModWrite(FileObject,
                                                  EndingOffset,
                                                  ResourceToRelease,
                                                  DeviceObject);

        if (Status == STATUS_SUCCESS) {
            return( TRUE );
        } else if (Status == STATUS_CANT_WAIT) {
            return( FALSE );
        } else {

            //
            // Fall through. When dealing with layered file systems, it might
            // be the case that the layered file system has the above dispatch
            // routine, but the FS it is layered on top of does not. In that
            // case, the layered file system will return an error code other
            // than STATUS_SUCCESS or STATUS_CANT_WAIT, and we simply handle
            // it as if the file system did not have the dispatch routine to
            // begin with.
            //

            NOTHING;
        }
    }

    //
    //  We follow the following rules to determine which resource
    //  to acquire.  We use the flags in the common header.  These
    //  flags can't change once we have acquired any resource.
    //  This means we can do an unsafe test and optimisticly
    //  acquire a resource.  At that point we can test the bits
    //  to see if we have what we want.
    //
    //  0 - If there is no main resource, acquire nothing.
    //
    //  1 - Acquire the main resource exclusively if the
    //      ACQUIRE_MAIN_RSRC_EX flag is set or we are extending
    //      valid data.
    //
    //  2 - Acquire the main resource shared if there is
    //      no paging io resource or the
    //      ACQUIRE_MAIN_RSRC_SH flag is set.
    //
    //  3 - Otherwise acquire the paging io resource shared.
    //

    if (Header->Resource == NULL) {

        *ResourceToRelease = NULL;

        return TRUE;
    }

    if (FlagOn( Header->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX ) ||
        (EndingOffset->QuadPart > Header->ValidDataLength.QuadPart &&
         Header->ValidDataLength.QuadPart != Header->FileSize.QuadPart)) {

        ResourceAcquired = Header->Resource;
        AcquireExclusive = TRUE;

    } else if (FlagOn( Header->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH ) ||
               Header->PagingIoResource == NULL) {

        ResourceAcquired = Header->Resource;
        AcquireExclusive = FALSE;

    } else {

        ResourceAcquired = Header->PagingIoResource;
        AcquireExclusive = FALSE;
    }

    //
    //  Perform the following in a loop in case we need to back and
    //  check the state of the resource acquisition.  In most cases
    //  the initial checks will succeed and we can proceed immediately.
    //  We have to worry about the two FsRtl bits changing but
    //  if there is no paging io resource before there won't ever be
    //  one.
    //

    while (TRUE) {

        //
        //  Now acquire the desired resource.
        //

        if (AcquireExclusive) {

            if (!ExAcquireResourceExclusive( ResourceAcquired, FALSE )) {

                return FALSE;
            }

        } else if (!ExAcquireSharedWaitForExclusive( ResourceAcquired, FALSE )) {

            return FALSE;
        }

        //
        //  If the valid data length is changing or the exclusive bit is
        //  set and we don't have the main resource exclusive then
        //  release the current resource and acquire the main resource
        //  exclusively and move to the top of the loop.
        //

        if (FlagOn( Header->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX ) ||
            (EndingOffset->QuadPart > Header->ValidDataLength.QuadPart &&
             Header->ValidDataLength.QuadPart != Header->FileSize.QuadPart)) {

            //
            //  If we don't have the main resource exclusively then
            //  release the current resource and attempt to acquire
            //  the main resource exclusively.
            //

            if (!AcquireExclusive) {

                ExReleaseResource( ResourceAcquired );
                AcquireExclusive = TRUE;
                ResourceAcquired = Header->Resource;
                continue;
            }

            //
            //  We have the correct resource.  Exit the loop.
            //

        //
        //  If we should be acquiring the main resource shared then move
        //  to acquire the correct resource and proceed to the top of the loop.
        //

        } else if (FlagOn( Header->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH )) {

            //
            //  If we have the main resource exclusively then downgrade to
            //  shared and exit the loop.
            //

            if (AcquireExclusive) {

                ExConvertExclusiveToShared( ResourceAcquired );

            //
            //  If we have the paging io resource then give up this resource
            //  and acquire the main resource exclusively.  This is going
            //  at it with a large hammer but is guaranteed to be resolved
            //  in the next pass through the loop.
            //

            } else if (ResourceAcquired != Header->Resource) {

                ExReleaseResource( ResourceAcquired );
                ResourceAcquired = Header->Resource;
                AcquireExclusive = TRUE;
                continue;
            }

            //
            //  We have the correct resource.  Exit the loop.
            //

        //
        //  At this point we should have the paging Io resource shared
        //  if it exists.  If not then acquire it shared and release the
        //  other resource and exit the loop.
        //

        } else if (Header->PagingIoResource != NULL
                   && ResourceAcquired != Header->PagingIoResource) {

            ResourceAcquired = NULL;

            if (ExAcquireSharedWaitForExclusive( Header->PagingIoResource, FALSE )) {

                ResourceAcquired = Header->PagingIoResource;
            }

            ExReleaseResource( Header->Resource );

            if (ResourceAcquired == NULL) {

                return FALSE;
            }

            //
            //  We now have the correct resource.  Exit the loop.
            //

        //
        //  We should have the main resource shared.  If we don't then
        //  degrade our lock to shared access.
        //

        } else if (AcquireExclusive) {

            ExConvertExclusiveToShared( ResourceAcquired );

            //
            //  We now have the correct resource.  Exit the loop.
            //
        }

        //
        //  We have the correct resource.  Exit the loop.
        //

        break;
    }

    *ResourceToRelease = ResourceAcquired;

    return TRUE;
}


NTKERNELAPI
VOID
FsRtlReleaseFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PERESOURCE ResourceToRelease
    )

/*++

Routine Description:

    This routine releases a file system resource previously acquired for
    the modified page writer.

Arguments:

    FileObject - Pointer to the file object being written.

    ResourceToRelease - Supplies the resource to release.  Not defined if
        FALSE is returned.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE();

    //
    //  First see if we have to call the file system. Note that in the case
    //  of layered file systems, the layered file system might have the
    //  dispatch routine, but the file system on which it is layered on may
    //  not. In that case, the layered file system will return
    //  STATUS_INVALID_DEVICE_REQUEST.
    //

    DeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;
    if ((FastIoDispatch->SizeOfFastIoDispatch >
         FIELD_OFFSET( FAST_IO_DISPATCH, ReleaseForModWrite )) &&
        (FastIoDispatch->ReleaseForModWrite != NULL)) {

        Status = FastIoDispatch->ReleaseForModWrite( FileObject, ResourceToRelease, DeviceObject );
    }

    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_INVALID_DEVICE_REQUEST) );

    if (Status == STATUS_INVALID_DEVICE_REQUEST) {
        ExReleaseResource( ResourceToRelease );
    }
}


NTKERNELAPI
VOID
FsRtlAcquireFileForCcFlush (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine acquires a file system resource prior to a call to CcFlush.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE();

    //
    //  First see if we have to call the file system. Note that in the case
    //  of layered file systems, the layered file system might have the
    //  dispatch routine, but the file system on which it is layered on may
    //  not. In that case, the layered file system will return
    //  STATUS_INVALID_DEVICE_REQUEST.
    //

    DeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    FsRtlEnterFileSystem();

    if ((FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch) &&
        (FastIoDispatch->SizeOfFastIoDispatch >
         FIELD_OFFSET( FAST_IO_DISPATCH, AcquireForCcFlush )) &&
        (FastIoDispatch->AcquireForCcFlush != NULL)) {

        Status = FastIoDispatch->AcquireForCcFlush( FileObject, DeviceObject );

    }


    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_INVALID_DEVICE_REQUEST) );

    if (Status == STATUS_INVALID_DEVICE_REQUEST) {

        PFSRTL_COMMON_FCB_HEADER Header = FileObject->FsContext;

        //
        //  If not already owned get the main resource exclusive because me may
        //  extend ValidDataLength.  Otherwise acquire it one more time recursively.
        //

        if (Header->Resource != NULL) {
            if (!ExIsResourceAcquiredShared(Header->Resource)) {
                ExAcquireResourceExclusive( Header->Resource, TRUE );
            } else {
                ExAcquireResourceShared( Header->Resource, TRUE );
            }
        }

        //
        //  Also get the paging I/O resource ahead of any MM resources.
        //

        if (Header->PagingIoResource != NULL) {
            ExAcquireResourceShared( Header->PagingIoResource, TRUE );
        }
    }
}


NTKERNELAPI
VOID
FsRtlReleaseFileForCcFlush (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine releases a file system resource previously acquired for
    the CcFlush.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE();

    //
    //  First see if we have to call the file system. Note that in the case
    //  of layered file systems, the layered file system might have the
    //  dispatch routine, but the file system on which it is layered on may
    //  not. In that case, the layered file system will return
    //  STATUS_INVALID_DEVICE_REQUEST.
    //

    DeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    if ((FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch) &&
        (FastIoDispatch->SizeOfFastIoDispatch >
         FIELD_OFFSET( FAST_IO_DISPATCH, ReleaseForCcFlush )) &&
        (FastIoDispatch->ReleaseForCcFlush != NULL)) {

        Status = FastIoDispatch->ReleaseForCcFlush( FileObject, DeviceObject );

    }

    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_INVALID_DEVICE_REQUEST) );

    if (Status == STATUS_INVALID_DEVICE_REQUEST) {

        PFSRTL_COMMON_FCB_HEADER Header = FileObject->FsContext;

        //
        //  Free whatever we acquired.
        //

        if (Header->PagingIoResource != NULL) {
            ExReleaseResource( Header->PagingIoResource );
        }

        if (Header->Resource != NULL) {
            ExReleaseResource( Header->Resource );
        }
    }

    FsRtlExitFileSystem();
}


NTKERNELAPI
VOID
FsRtlAcquireFileExclusive (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is used by NtCreateSection to pre-acquire file system
    resources in order to avoid deadlocks.  If there is a FastIo entry
    for AcquireFileForNtCreateSection then that routine will be called.
    Otherwise, we will simply acquire the main file resource exclusive.
    If there is no main resource then we acquire nothing and return
    FALSE.  In the cases that we acquire a resource, we also set the
    TopLevelIrp field in the thread local storage to indicate to file
    systems beneath us that we have acquired file system resources.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    NONE

--*/

{
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    PFSRTL_COMMON_FCB_HEADER Header;

    PAGED_CODE();

    //
    //  First see if we have to call the file system.
    //

    DeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    if ((FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch) &&
        (FastIoDispatch->SizeOfFastIoDispatch >
         FIELD_OFFSET( FAST_IO_DISPATCH, AcquireFileForNtCreateSection )) &&
        (FastIoDispatch->AcquireFileForNtCreateSection != NULL)) {

        FsRtlEnterFileSystem();
        FastIoDispatch->AcquireFileForNtCreateSection( FileObject );

        return;
    }

    //
    //  If there is a main file resource, acquire that.
    //

    if ((Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext) &&
        (Header->Resource != NULL)) {

        FsRtlEnterFileSystem();
        ExAcquireResourceExclusive( Header->Resource, TRUE );

        return;
    }

    //
    //  Nothing to acquire.
    //

    return;
}

NTKERNELAPI
VOID
FsRtlReleaseFile (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine releases resources acquired by FsRtlAcquireFileExclusive.

Arguments:

    FileObject - Pointer to the file object being written.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    PFSRTL_COMMON_FCB_HEADER Header;

    PAGED_CODE();

    //
    //  First see if we have to call the file system.
    //

    DeviceObject = IoGetBaseFileSystemDeviceObject( FileObject );

    if ((FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch) &&
        (FastIoDispatch->SizeOfFastIoDispatch >
         FIELD_OFFSET( FAST_IO_DISPATCH, ReleaseFileForNtCreateSection )) &&
        (FastIoDispatch->ReleaseFileForNtCreateSection != NULL)) {

        FastIoDispatch->ReleaseFileForNtCreateSection( FileObject );
        FsRtlExitFileSystem();
        return;
    }

    //
    //  If there is a main file resource, release that.
    //

    if ((Header = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext) &&
        (Header->Resource != NULL)) {

        ExReleaseResource( Header->Resource );
        FsRtlExitFileSystem();
        return;
    }

    //
    //  Nothing to release.
    //

    return;
}

NTSTATUS
FsRtlGetFileSize(
    IN PFILE_OBJECT FileObject,
    IN OUT PLARGE_INTEGER FileSize
    )

/*++

Routine Description:

    This routine is used to call the File System to get the FileSize
    for a file.

    It does this without acquiring the file object lock on synchronous file
    objects.  This routine is therefore safe to call if you already own
    file system resources, while IoQueryFileInformation could (and does)
    lead to deadlocks.

Arguments:

    FileObject - The file to query
    FileSize - Receives the file size.

Return Value:

    NTSTATUS - The final I/O status of the operation.  If the FileObject
        refers to a directory, STATUS_FILE_IS_A_DIRECTORY is returned.

--*/
{
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    FILE_STANDARD_INFORMATION FileInformation;

    PAGED_CODE();

    //
    // Get the address of the target device object.
    //

    DeviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Try the fast query call if it exists.
    //

    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    if (FastIoDispatch &&
        FastIoDispatch->FastIoQueryStandardInfo &&
        FastIoDispatch->FastIoQueryStandardInfo( FileObject,
                                                 TRUE,
                                                 &FileInformation,
                                                 &IoStatus,
                                                 DeviceObject )) {
        //
        //  Cool, it worked.
        //

    } else {

        //
        //  Life's tough, take the long path.
        //

        PIRP Irp;
        KEVENT Event;
        NTSTATUS Status;
        PIO_STACK_LOCATION IrpSp;

        //
        //  Initialize the event.
        //

        KeInitializeEvent( &Event, NotificationEvent, FALSE );

        //
        //  Allocate an I/O Request Packet (IRP) for this in-page operation.
        //

        Irp = IoAllocateIrp( DeviceObject->StackSize, FALSE );
        if (Irp == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Get a pointer to the first stack location in the packet.  This location
        //  will be used to pass the function codes and parameters to the first
        //  driver.
        //

        IrpSp = IoGetNextIrpStackLocation( Irp );

        //
        //  Fill in the IRP according to this request, setting the flags to
        //  just cause IO to set the event and deallocate the Irp.
        //

        Irp->Flags = IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO;
        Irp->RequestorMode = KernelMode;
        Irp->UserIosb = &IoStatus;
        Irp->UserEvent = &Event;
        Irp->Tail.Overlay.OriginalFileObject = FileObject;
        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
        Irp->AssociatedIrp.SystemBuffer = &FileInformation;

        //
        //  Fill in the normal query parameters.
        //

        IrpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
        IrpSp->FileObject = FileObject;
        IrpSp->DeviceObject = DeviceObject;
        IrpSp->Parameters.SetFile.Length = sizeof(FILE_STANDARD_INFORMATION);
        IrpSp->Parameters.SetFile.FileInformationClass = FileStandardInformation;

        //
        //  Queue the packet to the appropriate driver based.  This routine
        //  should not raise.
        //

        Status = IoCallDriver( DeviceObject, Irp );

        //
        //  If pending is returned (which is a successful status),
        //  we must wait for the request to complete.
        //

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)NULL);
        }

        //
        //  If we got an error back in Status, then the Iosb
        //  was not written, so we will just copy the status
        //  there, then test the final status after that.
        //

        if (!NT_SUCCESS(Status)) {
            IoStatus.Status = Status;
        }
    }

    //
    //  If the call worked, check to make sure it wasn't a directory and
    //  if not, fill in the FileSize parameter.
    //

    if (NT_SUCCESS(IoStatus.Status)) {

        if (FileInformation.Directory) {

            //
            // Can't get file size for a directory. Return error.
            //

            IoStatus.Status = STATUS_FILE_IS_A_DIRECTORY;

        } else {

            *FileSize = FileInformation.EndOfFile;
        }
    }

    return IoStatus.Status;
}

NTSTATUS
FsRtlSetFileSize(
    IN PFILE_OBJECT FileObject,
    IN OUT PLARGE_INTEGER FileSize
    )

/*++

Routine Description:

    This routine is used to call the File System to update FileSize
    for a file.

    It does this without acquiring the file object lock on synchronous file
    objects.  This routine is therefore safe to call if you already own
    file system resources, while IoSetInformation could (and does) lead
    to deadlocks.

Arguments:

    FileObject - A pointer to a referenced file object.

    ValidDataLength - Pointer to new FileSize.

Return Value:

    Status of operation.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    FILE_END_OF_FILE_INFORMATION Buffer;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    PIRP Irp;

    PAGED_CODE();

    //
    //  Copy FileSize to our buffer.
    //

    Buffer.EndOfFile = *FileSize;

    //
    //  Initialize the event.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Begin by getting a pointer to the device object that the file resides
    //  on.
    //

    DeviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    //  Allocate an I/O Request Packet (IRP) for this in-page operation.
    //

    Irp = IoAllocateIrp( DeviceObject->StackSize, FALSE );
    if (Irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Get a pointer to the first stack location in the packet.  This location
    //  will be used to pass the function codes and parameters to the first
    //  driver.
    //

    IrpSp = IoGetNextIrpStackLocation( Irp );

    //
    //  Fill in the IRP according to this request, setting the flags to
    //  just cause IO to set the event and deallocate the Irp.
    //

    Irp->Flags = IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO;
    Irp->RequestorMode = KernelMode;
    Irp->UserIosb = &IoStatus;
    Irp->UserEvent = &Event;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->AssociatedIrp.SystemBuffer = &Buffer;

    //
    //  Fill in the normal set file parameters.
    //

    IrpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
    IrpSp->FileObject = FileObject;
    IrpSp->DeviceObject = DeviceObject;
    IrpSp->Parameters.SetFile.Length = sizeof(FILE_END_OF_FILE_INFORMATION);
    IrpSp->Parameters.SetFile.FileInformationClass = FileEndOfFileInformation;

    //
    //  Queue the packet to the appropriate driver based on whether or not there
    //  is a VPB associated with the device.  This routine should not raise.
    //

    Status = IoCallDriver( DeviceObject, Irp );

    //
    //  If pending is returned (which is a successful status),
    //  we must wait for the request to complete.
    //

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject( &Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);
    }

    //
    //  If we got an error back in Status, then the Iosb
    //  was not written, so we will just copy the status
    //  there, then test the final status after that.
    //

    if (!NT_SUCCESS(Status)) {
        IoStatus.Status = Status;
    }

    return IoStatus.Status;
}
