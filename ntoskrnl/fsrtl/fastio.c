/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/fastio.c
 * PURPOSE:         Provides Fast I/O entrypoints to the Cache Manager
 * PROGRAMMERS:     Dominique Cote (buzdelabuz2@gmail.com)
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
FsRtlIncrementCcFastReadResourceMiss(VOID)
{
    CcFastReadResourceMiss++;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlIncrementCcFastReadNotPossible(VOID)
{
    CcFastReadNotPossible++;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlIncrementCcFastReadWait(VOID)
{
    CcFastReadWait++;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlIncrementCcFastReadNoWait(VOID)
{
    CcFastReadNoWait++;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlCopyRead(IN PFILE_OBJECT FileObject,
              IN PLARGE_INTEGER FileOffset,
              IN ULONG Length,
              IN BOOLEAN Wait,
              IN ULONG LockKey,
              OUT PVOID Buffer,
              OUT PIO_STATUS_BLOCK IoStatus,
              IN PDEVICE_OBJECT DeviceObject)
{

    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    LARGE_INTEGER Offset;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT Device;
    BOOLEAN Result = TRUE;
    ULONG PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(FileOffset, Length);

    PAGED_CODE();
    ASSERT(FileObject);
    ASSERT(FileObject->FsContext);

    /* No actual read */
    if (!Length)
    {
        /* Return success */
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;
        return TRUE;
    }

    if (Length > MAXLONGLONG - FileOffset->QuadPart)
    {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;
        return FALSE;
    }

    /* Get the offset and FCB header */
    Offset.QuadPart = FileOffset->QuadPart + Length;
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    if (Wait)
    {
        /* Use a Resource Acquire */
        FsRtlEnterFileSystem();
        CcFastReadWait++;
        ExAcquireResourceSharedLite(FcbHeader->Resource, TRUE);
    }
    else
    {
        /* Acquire the resource without blocking. Return false and the I/O manager
         * will retry using the standard IRP method. Use a Resource Acquire.
         */
        FsRtlEnterFileSystem();
        if (!ExAcquireResourceSharedLite(FcbHeader->Resource, FALSE))
        {
            FsRtlExitFileSystem();
            FsRtlIncrementCcFastReadResourceMiss();
            return FALSE;
        }
    }

    /* Check if this is a fast I/O cached file */
    if (!(FileObject->PrivateCacheMap) ||
        (FcbHeader->IsFastIoPossible == FastIoIsNotPossible))
    {
        /* It's not, so fail */
        Result = FALSE;
        goto Cleanup;
    }

    /* Check if we need to find out if fast I/O is available */
    if (FcbHeader->IsFastIoPossible == FastIoIsQuestionable)
    {
        /* Sanity check */
        ASSERT(!KeIsExecutingDpc());

        /* Get the Fast I/O table */
        Device = IoGetRelatedDeviceObject(FileObject);
        FastIoDispatch = Device->DriverObject->FastIoDispatch;

        /* Sanity check */
        ASSERT(FastIoDispatch != NULL);
        ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

        /* Ask the driver if we can do it */
        if (!FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                   FileOffset,
                                                   Length,
                                                   TRUE,
                                                   LockKey,
                                                   TRUE,
                                                   IoStatus,
                                                   Device))
        {
            /* It's not, fail */
            Result = FALSE;
            goto Cleanup;
        }
    }

    /* Check if we read too much */
    if (Offset.QuadPart > FcbHeader->FileSize.QuadPart)
    {
        /* We did, check if the file offset is past the end */
        if (FileOffset->QuadPart >= FcbHeader->FileSize.QuadPart)
        {
            /* Set end of file */
            IoStatus->Status = STATUS_END_OF_FILE;
            IoStatus->Information = 0;
            goto Cleanup;
        }

        /* Otherwise, just normalize the length */
        Length = (ULONG)(FcbHeader->FileSize.QuadPart - FileOffset->QuadPart);
    }

    /* Set this as top-level IRP */
    PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

    _SEH2_TRY
    {
        /* Make sure the IO and file size is below 4GB */
        if (Wait && !(Offset.HighPart | FcbHeader->FileSize.HighPart))
        {

            /* Call the cache controller */
            CcFastCopyRead(FileObject,
                           FileOffset->LowPart,
                           Length,
                           PageCount,
                           Buffer,
                           IoStatus);

            /* File was accessed */
            FileObject->Flags |= FO_FILE_FAST_IO_READ;

            if (IoStatus->Status != STATUS_END_OF_FILE)
            {
                ASSERT((ULONGLONG)FcbHeader->FileSize.QuadPart >=
                      ((ULONGLONG)FileOffset->QuadPart + IoStatus->Information));
            }
        }
        else
        {

            /* Call the cache controller */
            Result = CcCopyRead(FileObject,
                                FileOffset,
                                Length,
                                Wait,
                                Buffer,
                                IoStatus);

            /* File was accessed */
            FileObject->Flags |= FO_FILE_FAST_IO_READ;

            if (Result != FALSE)
            {
                ASSERT((IoStatus->Status == STATUS_END_OF_FILE) ||
                       (((ULONGLONG)FileOffset->QuadPart + IoStatus->Information) <=
                        (ULONGLONG)FcbHeader->FileSize.QuadPart));
            }
        }

        /* Update the current file offset */
        if (Result != FALSE)
        {
            FileObject->CurrentByteOffset.QuadPart = FileOffset->QuadPart + IoStatus->Information;
        }
    }
    _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        Result = FALSE;
    }
    _SEH2_END;

    PsGetCurrentThread()->TopLevelIrp = 0;

    /* Return to caller */
Cleanup:

    ExReleaseResourceLite(FcbHeader->Resource);
    FsRtlExitFileSystem();

    if (Result == FALSE)
    {
        CcFastReadNotPossible += 1;
    }

    return Result;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlCopyWrite(IN PFILE_OBJECT FileObject,
               IN PLARGE_INTEGER FileOffset,
               IN ULONG Length,
               IN BOOLEAN Wait,
               IN ULONG LockKey,
               OUT PVOID Buffer,
               OUT PIO_STATUS_BLOCK IoStatus,
               IN PDEVICE_OBJECT DeviceObject)
{
    BOOLEAN Result = TRUE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT Device;
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PSHARED_CACHE_MAP SharedCacheMap;

    /* WDK doc.
     * Offset == 0xffffffffffffffff indicates append to the end of file.
     */
    BOOLEAN FileOffsetAppend = (FileOffset->HighPart == (LONG)0xffffffff) &&
                               (FileOffset->LowPart == 0xffffffff);

    BOOLEAN ResourceAcquiredShared = FALSE;
    BOOLEAN b_4GB = FALSE;
    BOOLEAN FileSizeModified = FALSE;
    LARGE_INTEGER OldFileSize;
    LARGE_INTEGER OldValidDataLength;
    LARGE_INTEGER NewSize;
    LARGE_INTEGER Offset;

    PAGED_CODE();

    ASSERT(FileObject);
    ASSERT(FileObject->FsContext);

    /* Initialize some of the vars and pointers */
    NewSize.QuadPart = 0;
    Offset.QuadPart = FileOffset->QuadPart + Length;
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    /* Nagar p.544.
     * Check with Cc if we can write and check if the IO > 64kB (WDK macro).
     */
    if ((CcCanIWrite(FileObject, Length, Wait, FALSE) == FALSE) ||
        (CcCopyWriteWontFlush(FileObject, FileOffset, Length) == FALSE) ||
        ((FileObject->Flags & FO_WRITE_THROUGH)))
    {
        return FALSE;
    }

    /* Already init IO_STATUS_BLOCK */
    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = Length;

    /* No actual read */
    if (!Length)
    {
        return TRUE;
    }

    FsRtlEnterFileSystem();

    /* Nagar p.544/545.
     * The CcFastCopyWrite doesn't deal with filesize beyond 4GB.
     */
    if (Wait && (FcbHeader->AllocationSize.HighPart == 0))
    {
        /* If the file offset is not past the file size,
         * then we can acquire the lock shared.
         */
        if ((FileOffsetAppend == FALSE) &&
            (Offset.LowPart <= FcbHeader->ValidDataLength.LowPart))
        {
            ExAcquireResourceSharedLite(FcbHeader->Resource, TRUE);
            ResourceAcquiredShared = TRUE;
        }
        else
        {
            ExAcquireResourceExclusiveLite(FcbHeader->Resource, TRUE);
        }

        /* Nagar p.544/545.
         * If we append, use the file size as offset.
         * Also, check that we aren't crossing the 4GB boundary.
         */
        if (FileOffsetAppend != FALSE)
        {
            Offset.LowPart = FcbHeader->FileSize.LowPart;
            NewSize.LowPart = FcbHeader->FileSize.LowPart + Length;
            b_4GB = (NewSize.LowPart < FcbHeader->FileSize.LowPart);

        }
        else
        {
            Offset.LowPart = FileOffset->LowPart;
            NewSize.LowPart = FileOffset->LowPart + Length;
            b_4GB = (NewSize.LowPart < FileOffset->LowPart) ||
                    (FileOffset->HighPart != 0);
        }

        /* Nagar p.544/545.
         * Make sure that caching is initated.
         * That fast are allowed for this file stream.
         * That we are not extending past the allocated size.
         * That we are not creating a hole bigger than 8k.
         * That we are not crossing the 4GB boundary.
         */
        if ((FileObject->PrivateCacheMap != NULL) &&
            (FcbHeader->IsFastIoPossible != FastIoIsNotPossible) &&
            (FcbHeader->AllocationSize.LowPart >= NewSize.LowPart) &&
            (Offset.LowPart < FcbHeader->ValidDataLength.LowPart + 0x2000) &&
            !b_4GB)
        {
            /* If we are extending past the file size, we need to
             * release the lock and acquire it exclusively, because
             * we are going to need to update the FcbHeader.
             */
            if (ResourceAcquiredShared &&
                (NewSize.LowPart > FcbHeader->ValidDataLength.LowPart + 0x2000))
            {
                /* Then we need to acquire the resource exclusive */
                ExReleaseResourceLite(FcbHeader->Resource);
                ExAcquireResourceExclusiveLite(FcbHeader->Resource, TRUE);
                if (FileOffsetAppend != FALSE)
                {
                    Offset.LowPart = FcbHeader->FileSize.LowPart; // ??
                    NewSize.LowPart = FcbHeader->FileSize.LowPart + Length;

                    /* Make sure we don't cross the 4GB boundary */
                    b_4GB = (NewSize.LowPart < Offset.LowPart);
                }

                /* Recheck some of the conditions since we let the lock go */
                if ((FileObject->PrivateCacheMap != NULL) &&
                    (FcbHeader->IsFastIoPossible != FastIoIsNotPossible) &&
                    (FcbHeader->AllocationSize.LowPart >= NewSize.LowPart) &&
                    (FcbHeader->AllocationSize.HighPart == 0) &&
                    !b_4GB)
                {
                    /* Do nothing? */
                }
                else
                {
                    goto FailAndCleanup;
                }
            }

        }
        else
        {
            goto FailAndCleanup;
        }

        /* Check if we need to find out if fast I/O is available */
        if (FcbHeader->IsFastIoPossible == FastIoIsQuestionable)
        {
            IO_STATUS_BLOCK FastIoCheckIfPossibleStatus;

            /* Sanity check */
            ASSERT(!KeIsExecutingDpc());

            /* Get the Fast I/O table */
            Device = IoGetRelatedDeviceObject(FileObject);
            FastIoDispatch = Device->DriverObject->FastIoDispatch;

            /* Sanity check */
            ASSERT(FastIoDispatch != NULL);
            ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

            /* Ask the driver if we can do it */
            if (!FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                       FileOffsetAppend ?
                                                        &FcbHeader->FileSize :
                                                        FileOffset,
                                                       Length,
                                                       TRUE,
                                                       LockKey,
                                                       FALSE,
                                                       &FastIoCheckIfPossibleStatus,
                                                       Device))
            {
                /* It's not, fail */
                goto FailAndCleanup;
            }
        }

        /* If we are going to extend the file then save
         * the old file size in case the operation fails.
         */
        if (NewSize.LowPart > FcbHeader->FileSize.LowPart)
        {
            FileSizeModified = TRUE;
            OldFileSize.LowPart = FcbHeader->FileSize.LowPart;
            OldValidDataLength.LowPart = FcbHeader->ValidDataLength.LowPart;
            FcbHeader->FileSize.LowPart = NewSize.LowPart;
        }

        /* Set this as top-level IRP */
        PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

        _SEH2_TRY
        {
            if (Offset.LowPart > FcbHeader->ValidDataLength.LowPart)
            {
                LARGE_INTEGER OffsetVar;
                OffsetVar.LowPart = Offset.LowPart;
                OffsetVar.HighPart = 0;
                CcZeroData(FileObject, &FcbHeader->ValidDataLength, &OffsetVar, TRUE);
            }

            /* Call the cache manager */
            CcFastCopyWrite(FileObject, Offset.LowPart, Length, Buffer);
        }
        _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                                             EXCEPTION_EXECUTE_HANDLER :
                                             EXCEPTION_CONTINUE_SEARCH)
        {
            Result = FALSE;
        }
        _SEH2_END;

        /* Remove ourselves at the top level component after the IO is done */
        PsGetCurrentThread()->TopLevelIrp = 0;

        /* Did the operation succeed? */
        if (Result != FALSE)
        {
            /* Update the valid file size if necessary */
            if (NewSize.LowPart > FcbHeader->ValidDataLength.LowPart)
            {
                FcbHeader->ValidDataLength.LowPart = NewSize.LowPart;
            }

            /* Flag the file as modified */
            FileObject->Flags |= FO_FILE_MODIFIED;

            /* Update the strucutres if the file size changed */
            if (FileSizeModified)
            {
                SharedCacheMap =
                    (PSHARED_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
                SharedCacheMap->FileSize.LowPart = NewSize.LowPart;
                FileObject->Flags |= FO_FILE_SIZE_CHANGED;
            }

            /* Update the file object current file offset */
            FileObject->CurrentByteOffset.QuadPart = NewSize.LowPart;

        }
        else
        {
            /* Result == FALSE if we get here */
            if (FileSizeModified)
            {
                /* If the file size was modified then restore the old file size */
                if (FcbHeader->PagingIoResource != NULL)
                {
                    /* Nagar P.544.
                     * Restore the old file size if operation didn't succeed.
                     */
                    ExAcquireResourceExclusiveLite(FcbHeader->PagingIoResource, TRUE);
                    FcbHeader->FileSize.LowPart = OldFileSize.LowPart;
                    FcbHeader->ValidDataLength.LowPart = OldValidDataLength.LowPart;
                    ExReleaseResourceLite(FcbHeader->PagingIoResource);
                }
                else
                {
                    /* If there is no lock and do it without */
                    FcbHeader->FileSize.LowPart = OldFileSize.LowPart;
                    FcbHeader->ValidDataLength.LowPart = OldValidDataLength.LowPart;
                }
            }
            else
            {
                /* Do nothing? */
            }
        }

        goto Cleanup;
    }
    else
    {
        LARGE_INTEGER OldFileSize;

        /* Sanity check */
        ASSERT(!KeIsExecutingDpc());

        /* Nagar P.544.
         * Check if we need to acquire the resource exclusive.
         */
        if ((FileOffsetAppend == FALSE) &&
            (FileOffset->QuadPart + Length <= FcbHeader->ValidDataLength.QuadPart))
        {
            /* Acquire the resource shared */
            if (!ExAcquireResourceSharedLite(FcbHeader->Resource, Wait))
            {
                goto LeaveCriticalAndFail;
            }
            ResourceAcquiredShared = TRUE;
        }
        else
        {
            /* Acquire the resource exclusive */
            if (!ExAcquireResourceExclusiveLite(FcbHeader->Resource, Wait))
            {
                goto LeaveCriticalAndFail;
            }
        }

        /* Check if we are appending */
        if (FileOffsetAppend != FALSE)
        {
            Offset.QuadPart = FcbHeader->FileSize.QuadPart;
            NewSize.QuadPart = FcbHeader->FileSize.QuadPart + Length;
        }
        else
        {
            Offset.QuadPart = FileOffset->QuadPart;
            NewSize.QuadPart += FileOffset->QuadPart + Length;
        }

        /* Nagar p.544/545.
         * Make sure that caching is initated.
         * That fast are allowed for this file stream.
         * That we are not extending past the allocated size.
         * That we are not creating a hole bigger than 8k.
         */
        if ((FileObject->PrivateCacheMap != NULL) &&
            (FcbHeader->IsFastIoPossible != FastIoIsNotPossible) &&
            (FcbHeader->ValidDataLength.QuadPart + 0x2000 > Offset.QuadPart) &&
            (Length <= MAXLONGLONG - Offset.QuadPart) &&
            (FcbHeader->AllocationSize.QuadPart >= NewSize.QuadPart))
        {
            /* Check if we can keep the lock shared */
            if (ResourceAcquiredShared &&
                (NewSize.QuadPart > FcbHeader->ValidDataLength.QuadPart))
            {
                ExReleaseResourceLite(FcbHeader->Resource);
                if (!ExAcquireResourceExclusiveLite(FcbHeader->Resource, Wait))
                {
                    goto LeaveCriticalAndFail;
                }

                /* Compute the offset and the new filesize */
                if (FileOffsetAppend)
                {
                    Offset.QuadPart = FcbHeader->FileSize.QuadPart;
                    NewSize.QuadPart = FcbHeader->FileSize.QuadPart + Length;
                }

                /* Recheck the above points since we released and reacquire the lock */
                if ((FileObject->PrivateCacheMap != NULL) &&
                    (FcbHeader->IsFastIoPossible != FastIoIsNotPossible) &&
                    (FcbHeader->AllocationSize.QuadPart > NewSize.QuadPart))
                {
                    /* Do nothing */
                }
                else
                {
                    goto FailAndCleanup;
                }
            }

            /* Check if we need to find out if fast I/O is available */
            if (FcbHeader->IsFastIoPossible == FastIoIsQuestionable)
            {
                IO_STATUS_BLOCK FastIoCheckIfPossibleStatus;

                /* Sanity check */
                ASSERT(!KeIsExecutingDpc());

                /* Get the Fast I/O table */
                Device = IoGetRelatedDeviceObject(FileObject);
                FastIoDispatch = Device->DriverObject->FastIoDispatch;

                /* Sanity check */
                ASSERT(FastIoDispatch != NULL);
                ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

                /* Ask the driver if we can do it */
                if (!FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                           FileOffsetAppend ?
                                                            &FcbHeader->FileSize :
                                                            FileOffset,
                                                           Length,
                                                           Wait,
                                                           LockKey,
                                                           FALSE,
                                                           &FastIoCheckIfPossibleStatus,
                                                           Device))
                {
                    /* It's not, fail */
                    goto FailAndCleanup;
                }
            }

            /* If we are going to modify the filesize,
             * save the old fs in case the operation fails.
             */
            if (NewSize.QuadPart > FcbHeader->FileSize.QuadPart)
            {
                FileSizeModified = TRUE;
                OldFileSize.QuadPart = FcbHeader->FileSize.QuadPart;
                OldValidDataLength.QuadPart = FcbHeader->ValidDataLength.QuadPart;

                /* If the high part of the filesize is going
                 * to change, grab the Paging IoResouce.
                 */
                if (NewSize.HighPart != FcbHeader->FileSize.HighPart &&
                    FcbHeader->PagingIoResource)
                {
                    ExAcquireResourceExclusiveLite(FcbHeader->PagingIoResource, TRUE);
                    FcbHeader->FileSize.QuadPart = NewSize.QuadPart;
                    ExReleaseResourceLite(FcbHeader->PagingIoResource);
                }
                else
                {
                    FcbHeader->FileSize.QuadPart = NewSize.QuadPart;
                }
            }

            /* Nagar p.544.
             * Set ourselves as top component.
             */
            PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

            _SEH2_TRY
            {
                BOOLEAN CallCc = TRUE;

                /* Check if there is a gap between the end of the file
                 * and the offset. If yes, then we have to zero the data.
                 */
                if (Offset.QuadPart > FcbHeader->ValidDataLength.QuadPart)
                {
                    if (!(Result = CcZeroData(FileObject,
                                              &FcbHeader->ValidDataLength,
                                              &Offset,
                                              Wait)))
                    {
                        /* If this operation fails, then we have to exit. We can't jump
                         * outside the SEH, so I am using a variable to exit normally.
                         */
                        CallCc = FALSE;
                    }
                }

                /* Unless the CcZeroData failed, call the cache manager */
                if (CallCc)
                {
                    Result = CcCopyWrite(FileObject, &Offset, Length, Wait, Buffer);
                }
            }
            _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                                                 EXCEPTION_EXECUTE_HANDLER :
                                                 EXCEPTION_CONTINUE_SEARCH)
            {
                Result = FALSE;
            }
            _SEH2_END;

            /* Reset the top component */
            PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

            /* Did the operation suceeded */
            if (Result)
            {
                /* Check if we need to update the filesize */
                if (NewSize.QuadPart > FcbHeader->ValidDataLength.QuadPart)
                {
                    if (NewSize.HighPart != FcbHeader->ValidDataLength.HighPart &&
                        FcbHeader->PagingIoResource)
                    {
                        ExAcquireResourceExclusiveLite(FcbHeader->PagingIoResource, TRUE);
                        FcbHeader->ValidDataLength.QuadPart = NewSize.QuadPart;
                        ExReleaseResourceLite(FcbHeader->PagingIoResource);
                    }
                    else
                    {
                        FcbHeader->ValidDataLength.QuadPart = NewSize.QuadPart;
                    }
                }

                /* Flag the file as modified */
                FileObject->Flags |= FO_FILE_MODIFIED;

                /* Check if the filesize has changed */
                if (FileSizeModified)
                {
                    /* Update the file sizes */
                    SharedCacheMap =
                        (PSHARED_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
                    SharedCacheMap->FileSize.QuadPart = NewSize.QuadPart;
                    FileObject->Flags |= FO_FILE_SIZE_CHANGED;
                }

                /* Update the current FO byte offset */
                FileObject->CurrentByteOffset.QuadPart = NewSize.QuadPart;
            }
            else
            {
                /* The operation did not succeed.
                 * Reset the file size to what it should be.
                 */
                if (FileSizeModified)
                {
                    if (FcbHeader->PagingIoResource)
                    {
                        ExAcquireResourceExclusiveLite(FcbHeader->PagingIoResource, TRUE);
                        FcbHeader->FileSize.QuadPart = OldFileSize.QuadPart;
                        FcbHeader->ValidDataLength.QuadPart = OldValidDataLength.QuadPart;
                        ExReleaseResourceLite(FcbHeader->PagingIoResource);
                    }
                    else
                    {
                        FcbHeader->FileSize.QuadPart = OldFileSize.QuadPart;
                        FcbHeader->ValidDataLength.QuadPart = OldValidDataLength.QuadPart;
                    }
                }
            }

            goto Cleanup;
        }
        else
        {
            goto FailAndCleanup;
        }
    }

LeaveCriticalAndFail:

    FsRtlExitFileSystem();
    return FALSE;

FailAndCleanup:

    ExReleaseResourceLite(FcbHeader->Resource);
    FsRtlExitFileSystem();
    return FALSE;

Cleanup:

    ExReleaseResourceLite(FcbHeader->Resource);
    FsRtlExitFileSystem();
    return Result;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlGetFileSize(IN PFILE_OBJECT FileObject,
                 IN OUT PLARGE_INTEGER FileSize)
{
    FILE_STANDARD_INFORMATION Info;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    KEVENT Event;
    PIO_STACK_LOCATION IoStackLocation;
    PIRP Irp;
    BOOLEAN OldHardError;

    PAGED_CODE();

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check FastIoQueryStandardInfo.
     * Call the function and see if it succeeds.
     */
    if (!FastDispatch ||
        !FastDispatch->FastIoQueryStandardInfo ||
        !FastDispatch->FastIoQueryStandardInfo(FileObject,
                                               TRUE,
                                               &Info,
                                               &IoStatus,
                                               DeviceObject))
    {
        /* If any of the above failed, then we are going to send an
         * IRP to the device object. Initialize the event for the IO.
         */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* Allocate the IRP */
        Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

        if (Irp == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Don't process hard error */
        OldHardError = IoSetThreadHardErrorMode(FALSE);

        /* Setup the IRP */
        Irp->UserIosb = &IoStatus;
        Irp->UserEvent = &Event;
        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
        Irp->Flags = IRP_SYNCHRONOUS_PAGING_IO | IRP_PAGING_IO;
        Irp->RequestorMode = KernelMode;
        Irp->Tail.Overlay.OriginalFileObject = FileObject;
        Irp->AssociatedIrp.SystemBuffer = &Info;

        /* Setup out stack location */
        IoStackLocation = Irp->Tail.Overlay.CurrentStackLocation;
        IoStackLocation--;
        IoStackLocation->MajorFunction = IRP_MJ_QUERY_INFORMATION;
        IoStackLocation->FileObject = FileObject;
        IoStackLocation->DeviceObject = DeviceObject;
        IoStackLocation->Parameters.QueryFile.Length =
            sizeof(FILE_STANDARD_INFORMATION);
        IoStackLocation->Parameters.QueryFile.FileInformationClass =
            FileStandardInformation;

        /* Send the IRP to the related device object */
        Status = IoCallDriver(DeviceObject, Irp);

        /* Standard DDK IRP result processing */
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }

        /* If there was a synchronous error, signal it */
        if (!NT_SUCCESS(Status))
        {
            IoStatus.Status = Status;
        }

        IoSetThreadHardErrorMode(OldHardError);
    }

    /* Check the sync/async IO result */
    if (NT_SUCCESS(IoStatus.Status))
    {
        /* Was the request for a directory? */
        if (Info.Directory)
        {
            IoStatus.Status = STATUS_FILE_IS_A_DIRECTORY;
        }
        else
        {
            FileSize->QuadPart = Info.EndOfFile.QuadPart;
        }
    }

    return IoStatus.Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlRead(IN PFILE_OBJECT FileObject,
             IN PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             IN ULONG LockKey,
             OUT PMDL *MdlChain,
             OUT PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlRead)
    {
        /* Use the fast path */
        return FastDispatch->MdlRead(FileObject,
                                     FileOffset,
                                     Length,
                                     LockKey,
                                     MdlChain,
                                     IoStatus,
                                     DeviceObject);
    }

    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;

    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->MdlRead && BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }

    /* No fast path, use slow path */
    return FsRtlMdlReadDev(FileObject,
                           FileOffset,
                           Length,
                           LockKey,
                           MdlChain,
                           IoStatus,
                           DeviceObject);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlReadComplete(IN PFILE_OBJECT FileObject,
                     IN OUT PMDL MdlChain)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlReadComplete)
    {
        /* Use the fast path */
        return FastDispatch->MdlReadComplete(FileObject, MdlChain, DeviceObject);
    }

    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;

    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if ((BaseDeviceObject != DeviceObject) &&
        FastDispatch &&
        FastDispatch->MdlReadComplete)
    {
        return FALSE;
    }

    /* No fast path, use slow path */
    return FsRtlMdlReadCompleteDev(FileObject, MdlChain, DeviceObject);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlReadCompleteDev(IN PFILE_OBJECT FileObject,
                        IN PMDL MemoryDescriptorList,
                        IN PDEVICE_OBJECT DeviceObject)
{
    /* Call the Cache Manager */
    CcMdlReadComplete2(FileObject, MemoryDescriptorList);
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlReadDev(IN PFILE_OBJECT FileObject,
                IN PLARGE_INTEGER FileOffset,
                IN ULONG Length,
                IN ULONG LockKey,
                OUT PMDL *MdlChain,
                OUT PIO_STATUS_BLOCK IoStatus,
                IN PDEVICE_OBJECT DeviceObject)
{
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    BOOLEAN Result = TRUE;
    LARGE_INTEGER Offset;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT Device;
    PAGED_CODE();

    /* No actual read */
    if (!Length)
    {
        /* Return success */
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;
        return TRUE;
    }

    /* Sanity check */
    ASSERT(MAXLONGLONG - FileOffset->QuadPart >= (LONGLONG)Length);

    /* Get the offset and FCB header */
    Offset.QuadPart = FileOffset->QuadPart + Length;
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    /* Enter the FS */
    FsRtlEnterFileSystem();
    CcFastMdlReadWait++;

    /* Lock the FCB */
    ExAcquireResourceShared(FcbHeader->Resource, TRUE);

    /* Check if this is a fast I/O cached file */
    if (!(FileObject->PrivateCacheMap) ||
        (FcbHeader->IsFastIoPossible == FastIoIsNotPossible))
    {
        /* It's not, so fail */
        CcFastMdlReadNotPossible += 1;
        Result = FALSE;
        goto Cleanup;
    }

    /* Check if we need to find out if fast I/O is available */
    if (FcbHeader->IsFastIoPossible == FastIoIsQuestionable)
    {
        /* Get the Fast I/O table */
        Device = IoGetRelatedDeviceObject(FileObject);
        FastIoDispatch = Device->DriverObject->FastIoDispatch;

        /* Sanity check */
        ASSERT(!KeIsExecutingDpc());
        ASSERT(FastIoDispatch != NULL);
        ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

        /* Ask the driver if we can do it */
        if (!FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                   FileOffset,
                                                   Length,
                                                   TRUE,
                                                   LockKey,
                                                   TRUE,
                                                   IoStatus,
                                                   Device))
        {
            /* It's not, fail */
            CcFastMdlReadNotPossible += 1;
            Result = FALSE;
            goto Cleanup;
        }
    }

    /* Check if we read too much */
    if (Offset.QuadPart > FcbHeader->FileSize.QuadPart)
    {
        /* We did, check if the file offset is past the end */
        if (FileOffset->QuadPart >= FcbHeader->FileSize.QuadPart)
        {
            /* Set end of file */
            IoStatus->Status = STATUS_END_OF_FILE;
            IoStatus->Information = 0;
            goto Cleanup;
        }

        /* Otherwise, just normalize the length */
        Length = (ULONG)(FcbHeader->FileSize.QuadPart - FileOffset->QuadPart);
    }

    /* Set this as top-level IRP */
    PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;

    _SEH2_TRY
    {
        /* Attempt a read */
        CcMdlRead(FileObject, FileOffset, Length, MdlChain, IoStatus);
        FileObject->Flags |= FO_FILE_FAST_IO_READ;
    }
    _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                                         EXCEPTION_EXECUTE_HANDLER :
                                         EXCEPTION_CONTINUE_SEARCH)
    {
        Result = FALSE;
    }
    _SEH2_END;


    /* Remove the top-level IRP flag */
    PsGetCurrentThread()->TopLevelIrp = 0;

    /* Return to caller */
Cleanup:

    ExReleaseResourceLite(FcbHeader->Resource);
    FsRtlExitFileSystem();

    return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlWriteComplete(IN PFILE_OBJECT FileObject,
                      IN PLARGE_INTEGER FileOffset,
                      IN PMDL MdlChain)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlWriteComplete)
    {
        /* Use the fast path */
        return FastDispatch->MdlWriteComplete(FileObject,
                                              FileOffset,
                                              MdlChain,
                                              DeviceObject);
    }

    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;

    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch &&
        FastDispatch->MdlWriteComplete &&
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }

    /* No fast path, use slow path */
    return FsRtlMdlWriteCompleteDev(FileObject,
                                    FileOffset,
                                    MdlChain,
                                    DeviceObject);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlMdlWriteCompleteDev(IN PFILE_OBJECT FileObject,
                         IN PLARGE_INTEGER FileOffset,
                         IN PMDL MdlChain,
                         IN PDEVICE_OBJECT DeviceObject)
{
    if (FileObject->Flags & FO_WRITE_THROUGH)
    {
        return FALSE;
    }

    /* Call the Cache Manager */
    CcMdlWriteComplete2(FileObject, FileOffset, MdlChain);
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlPrepareMdlWrite(IN PFILE_OBJECT FileObject,
                     IN PLARGE_INTEGER FileOffset,
                     IN ULONG Length,
                     IN ULONG LockKey,
                     OUT PMDL *MdlChain,
                     OUT PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->PrepareMdlWrite)
    {
        /* Use the fast path */
        return FastDispatch->PrepareMdlWrite(FileObject,
                                             FileOffset,
                                             Length,
                                             LockKey,
                                             MdlChain,
                                             IoStatus,
                                             DeviceObject);
    }

    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;

    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch &&
        FastDispatch->PrepareMdlWrite &&
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }

    /* No fast path, use slow path */
    return FsRtlPrepareMdlWriteDev(FileObject,
                                   FileOffset,
                                   Length,
                                   LockKey,
                                   MdlChain,
                                   IoStatus,
                                   DeviceObject);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlPrepareMdlWriteDev(IN PFILE_OBJECT FileObject,
                        IN PLARGE_INTEGER FileOffset,
                        IN ULONG Length,
                        IN ULONG LockKey,
                        OUT PMDL *MdlChain,
                        OUT PIO_STATUS_BLOCK IoStatus,
                        IN PDEVICE_OBJECT DeviceObject)
{
    BOOLEAN Result = TRUE;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT Device;
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PSHARED_CACHE_MAP SharedCacheMap;

    LARGE_INTEGER OldFileSize;
    LARGE_INTEGER OldValidDataLength;
    LARGE_INTEGER NewSize;
    LARGE_INTEGER Offset;

    /* WDK doc.
     * Offset == 0xffffffffffffffff indicates append to the end of file.
     */
    BOOLEAN FileOffsetAppend = (FileOffset->HighPart == (LONG)0xffffffff) &&
                               (FileOffset->LowPart == 0xffffffff);

    BOOLEAN FileSizeModified = FALSE;
    BOOLEAN ResourceAcquiredShared = FALSE;

    /* Initialize some of the vars and pointers */
    OldFileSize.QuadPart = 0;
    OldValidDataLength.QuadPart = 0;

    PAGED_CODE();

    Offset.QuadPart = FileOffset->QuadPart + Length;

    /* Nagar p.544.
     * Check with Cc if we can write.
     */
    if (!CcCanIWrite(FileObject, Length, TRUE, FALSE) ||
        (FileObject->Flags & FO_WRITE_THROUGH))
    {
        return FALSE;
    }

    IoStatus->Status = STATUS_SUCCESS;

    /* No actual read */
    if (!Length)
    {
        return TRUE;
    }

    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
    FsRtlEnterFileSystem();

    /* Check we are going to extend the file */
    if ((FileOffsetAppend == FALSE) &&
        (FileOffset->QuadPart + Length <= FcbHeader->ValidDataLength.QuadPart))
    {
        /* Acquire the resource shared */
        ExAcquireResourceSharedLite(FcbHeader->Resource, TRUE);
        ResourceAcquiredShared = TRUE;
    }
    else
    {
        /* Acquire the resource exclusive */
        ExAcquireResourceExclusiveLite(FcbHeader->Resource, TRUE);
    }

    /* Check if we are appending */
    if (FileOffsetAppend != FALSE)
    {
        Offset.QuadPart = FcbHeader->FileSize.QuadPart;
        NewSize.QuadPart = FcbHeader->FileSize.QuadPart + Length;
    }
    else
    {
        Offset.QuadPart = FileOffset->QuadPart;
        NewSize.QuadPart = FileOffset->QuadPart + Length;
    }

    if ((FileObject->PrivateCacheMap) &&
        (FcbHeader->IsFastIoPossible) &&
        (Length <= MAXLONGLONG - FileOffset->QuadPart) &&
        (NewSize.QuadPart <= FcbHeader->AllocationSize.QuadPart))
    {
        /* Check if we can keep the lock shared */
        if (ResourceAcquiredShared &&
            (NewSize.QuadPart > FcbHeader->ValidDataLength.QuadPart))
        {
            ExReleaseResourceLite(FcbHeader->Resource);
            ExAcquireResourceExclusiveLite(FcbHeader->Resource, TRUE);

            /* Compute the offset and the new filesize */
            if (FileOffsetAppend)
            {
                Offset.QuadPart = FcbHeader->FileSize.QuadPart;
                NewSize.QuadPart = FcbHeader->FileSize.QuadPart + Length;
            }

            /* Recheck the above points since we released and reacquire the lock */
            if ((FileObject->PrivateCacheMap != NULL) &&
                (FcbHeader->IsFastIoPossible != FastIoIsNotPossible) &&
                (FcbHeader->AllocationSize.QuadPart > NewSize.QuadPart))
            {
                /* Do nothing */
            }
            else
            {
                goto FailAndCleanup;
            }
        }

        /* Check if we need to find out if fast I/O is available */
        if (FcbHeader->IsFastIoPossible == FastIoIsQuestionable)
        {
            /* Sanity check */
            /* ASSERT(!KeIsExecutingDpc()); */

            /* Get the Fast I/O table */
            Device = IoGetRelatedDeviceObject(FileObject);
            FastIoDispatch = Device->DriverObject->FastIoDispatch;

            /* Sanity check */
            ASSERT(FastIoDispatch != NULL);
            ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

            /* Ask the driver if we can do it */
            if (!FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                       FileOffset,
                                                       Length,
                                                       TRUE,
                                                       LockKey,
                                                       FALSE,
                                                       IoStatus,
                                                       Device))
            {
                /* It's not, fail */
                goto FailAndCleanup;
            }
        }

        /* If we are going to modify the filesize,
         * save the old fs in case the operation fails.
         */
        if (NewSize.QuadPart > FcbHeader->FileSize.QuadPart)
        {
            FileSizeModified = TRUE;
            OldFileSize.QuadPart = FcbHeader->FileSize.QuadPart;
            OldValidDataLength.QuadPart = FcbHeader->ValidDataLength.QuadPart;

            /* If the high part of the filesize is going
             * to change, grab the Paging IoResouce.
             */
            if (NewSize.HighPart != FcbHeader->FileSize.HighPart &&
                FcbHeader->PagingIoResource)
            {
                ExAcquireResourceExclusiveLite(FcbHeader->PagingIoResource, TRUE);
                FcbHeader->FileSize.QuadPart = NewSize.QuadPart;
                ExReleaseResourceLite(FcbHeader->PagingIoResource);
            }
            else
            {
                FcbHeader->FileSize.QuadPart = NewSize.QuadPart;
            }
        }


        /* Nagar p.544.
         * Set ourselves as top component.
         */
        PsGetCurrentThread()->TopLevelIrp = FSRTL_FAST_IO_TOP_LEVEL_IRP;
        _SEH2_TRY
        {
            /* Check if there is a gap between the end of the file and the offset.
             * If yes, then we have to zero the data.
             */
            if (Offset.QuadPart > FcbHeader->ValidDataLength.QuadPart)
            {
                Result = CcZeroData(FileObject,
                                    &FcbHeader->ValidDataLength,
                                    &Offset,
                                    TRUE);
                if (Result)
                {
                    CcPrepareMdlWrite(FileObject,
                                      &Offset,
                                      Length,
                                      MdlChain,
                                      IoStatus);
                }
            }
            else
            {
                CcPrepareMdlWrite(FileObject, &Offset, Length, MdlChain, IoStatus);
            }

        }
        _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                                             EXCEPTION_EXECUTE_HANDLER :
                                             EXCEPTION_CONTINUE_SEARCH)
        {
            Result = FALSE;
        }
        _SEH2_END;

        /* Reset the top component */
        PsGetCurrentThread()->TopLevelIrp = 0;

        /* Did the operation suceeded */
        if (Result)
        {
            /* Check if we need to update the filesize */
            if (NewSize.QuadPart > FcbHeader->ValidDataLength.QuadPart)
            {
                if (NewSize.HighPart != FcbHeader->ValidDataLength.HighPart &&
                    FcbHeader->PagingIoResource)
                {
                    ExAcquireResourceExclusiveLite(FcbHeader->PagingIoResource, TRUE);
                    FcbHeader->ValidDataLength.QuadPart = NewSize.QuadPart;
                    ExReleaseResourceLite(FcbHeader->PagingIoResource);
                }
                else
                {
                    FcbHeader->ValidDataLength.QuadPart = NewSize.QuadPart;
                }
            }

            /* Flag the file as modified */
            FileObject->Flags |= FO_FILE_MODIFIED;

            /* Check if the filesize has changed */
            if (FileSizeModified)
            {
                SharedCacheMap =
                    (PSHARED_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
                SharedCacheMap->FileSize.QuadPart = NewSize.QuadPart;
                FileObject->Flags |= FO_FILE_SIZE_CHANGED;
            }
        }
        else
        {
            /* The operation did not succeed.
             * Reset the file size to what it should be.
             */
            if (FileSizeModified)
            {
                if (FcbHeader->PagingIoResource)
                {
                    ExAcquireResourceExclusiveLite(FcbHeader->PagingIoResource, TRUE);
                    FcbHeader->FileSize.QuadPart = OldFileSize.QuadPart;
                    FcbHeader->ValidDataLength.QuadPart = OldValidDataLength.QuadPart;
                    ExReleaseResourceLite(FcbHeader->PagingIoResource);
                }
                else
                {
                    FcbHeader->FileSize.QuadPart = OldFileSize.QuadPart;
                    FcbHeader->ValidDataLength.QuadPart = OldValidDataLength.QuadPart;
                }
            }
        }

        goto Cleanup;
    }
    else
    {
        goto FailAndCleanup;
    }

FailAndCleanup:

    ExReleaseResourceLite(FcbHeader->Resource);
    FsRtlExitFileSystem();
    return FALSE;

Cleanup:

    ExReleaseResourceLite(FcbHeader->Resource);
    FsRtlExitFileSystem();
    return Result;
}

NTSTATUS
NTAPI
FsRtlAcquireFileExclusiveCommon(IN PFILE_OBJECT FileObject,
                                IN FS_FILTER_SECTION_SYNC_TYPE SyncType,
                                IN ULONG Reserved)
{
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    PEXTENDED_DRIVER_EXTENSION DriverExtension;
    PFS_FILTER_CALLBACKS FilterCallbacks;

    /* Get Device Object and Fast Calls */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Get master FsRtl lock */
    FsRtlEnterFileSystem();

    DriverExtension = (PEXTENDED_DRIVER_EXTENSION)DeviceObject->DriverObject->DriverExtension;
    FilterCallbacks = DriverExtension->FsFilterCallbacks;

    /* Check if Filter Cllbacks are supported */
    if (FilterCallbacks && FilterCallbacks->PreAcquireForSectionSynchronization)
    {
        NTSTATUS Status;
        PVOID CompletionContext;

        FS_FILTER_CALLBACK_DATA CbData;

        RtlZeroMemory(&CbData, sizeof(CbData));

        CbData.SizeOfFsFilterCallbackData = sizeof(CbData);
        CbData.Operation = FS_FILTER_ACQUIRE_FOR_SECTION_SYNCHRONIZATION;
        CbData.DeviceObject = DeviceObject;
        CbData.FileObject = FileObject;
        CbData.Parameters.AcquireForSectionSynchronization.PageProtection = Reserved;
        CbData.Parameters.AcquireForSectionSynchronization.SyncType = SyncType;

        Status = FilterCallbacks->PreAcquireForSectionSynchronization(&CbData, &CompletionContext);
        if (!NT_SUCCESS(Status))
        {
            FsRtlExitFileSystem();
            return Status;
        }

        /* Should we do something in-between ? */

        if (FilterCallbacks->PostAcquireForSectionSynchronization)
        {
            FilterCallbacks->PostAcquireForSectionSynchronization(&CbData, Status, CompletionContext);
        }

        /* Return here when the status is based on the synchonization type and write access to the file */
        if (Status == STATUS_FSFILTER_OP_COMPLETED_SUCCESSFULLY ||
            Status == STATUS_FILE_LOCKED_WITH_ONLY_READERS ||
            Status == STATUS_FILE_LOCKED_WITH_WRITERS)
        {
            return Status;
        }
    }

    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if Fast Calls are supported, and check AcquireFileForNtCreateSection */
    if (FastDispatch &&
        FastDispatch->AcquireFileForNtCreateSection)
    {
        /* Call the AcquireFileForNtCreateSection FastIo handler */
        FastDispatch->AcquireFileForNtCreateSection(FileObject);
    }
    else
    {
        /* No FastIo handler, acquire file's resource exclusively */
        if (FcbHeader && FcbHeader->Resource) ExAcquireResourceExclusiveLite(FcbHeader->Resource, TRUE);
    }

    return STATUS_SUCCESS;
}

/*
* @implemented
*/
VOID
NTAPI
FsRtlAcquireFileExclusive(IN PFILE_OBJECT FileObject)
{
    PAGED_CODE();

    /* Call the common routine. Don't care about the result */
    (VOID)FsRtlAcquireFileExclusiveCommon(FileObject, SyncTypeOther, 0);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
FsRtlAcquireToCreateMappedSection(_In_ PFILE_OBJECT FileObject,
                                  _In_ ULONG SectionPageProtection)
{
    PAGED_CODE();

    return FsRtlAcquireFileExclusiveCommon(FileObject, SyncTypeCreateSection, SectionPageProtection);
}
/*
* @implemented
*/
VOID
NTAPI
FsRtlReleaseFile(IN PFILE_OBJECT FileObject)
{
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Device Object and Fast Calls */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if Fast Calls are supported and check ReleaseFileForNtCreateSection */
    if (FastDispatch &&
        FastDispatch->ReleaseFileForNtCreateSection)
    {
        /* Call the ReleaseFileForNtCreateSection FastIo handler */
        FastDispatch->ReleaseFileForNtCreateSection(FileObject);
    }
    else
    {
        /* No FastIo handler, release file's resource */
        if (FcbHeader && FcbHeader->Resource) ExReleaseResourceLite(FcbHeader->Resource);
    }

    /* Release master FsRtl lock */
    FsRtlExitFileSystem();
}

/*
* @implemented
*/
NTSTATUS
NTAPI
FsRtlAcquireFileForCcFlushEx(IN PFILE_OBJECT FileObject)
{
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    NTSTATUS Status;

    /* Get the Base File System (Volume) and Fast Calls */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Get master FsRtl lock */
    FsRtlEnterFileSystem();

    /* Check if Fast Calls are supported, and check AcquireForCcFlush */
    if (FastDispatch &&
        FastDispatch->AcquireForCcFlush)
    {
        /* Call the AcquireForCcFlush FastIo handler */
        Status = FastDispatch->AcquireForCcFlush(FileObject, BaseDeviceObject);

        /* Return either success or inability to wait.
           In case of other failure - fall through */
        if (NT_SUCCESS(Status))
            return Status;

        if (Status == STATUS_CANT_WAIT)
        {
            DPRINT1("STATUS_CANT_WAIT\n");
            FsRtlExitFileSystem();
            return Status;
        }
    }

    /* No FastIo handler (or it failed). Acquire Main resource */
    if (FcbHeader->Resource)
    {
        /* Acquire it - either shared if it's already acquired
            or exclusively if we are the first */
        if (ExIsResourceAcquiredSharedLite(FcbHeader->Resource))
            ExAcquireResourceSharedLite(FcbHeader->Resource, TRUE);
        else
            ExAcquireResourceExclusiveLite(FcbHeader->Resource, TRUE);
    }

    /* Also acquire its PagingIO resource */
    if (FcbHeader->PagingIoResource)
        ExAcquireResourceSharedLite(FcbHeader->PagingIoResource, TRUE);

    return STATUS_SUCCESS;
}

/*
* @implemented
*/
VOID
NTAPI
FsRtlAcquireFileForCcFlush(IN PFILE_OBJECT FileObject)
{
    PAGED_CODE();

    /* Call the common routine. Don't care about the result */
    (VOID)FsRtlAcquireFileForCcFlushEx(FileObject);
}


/*
* @implemented
*/
VOID
NTAPI
FsRtlReleaseFileForCcFlush(IN PFILE_OBJECT FileObject)
{
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    /* Get Device Object and Fast Calls */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if Fast Calls are supported, and check ReleaseForCcFlush */
    if (FastDispatch &&
        FastDispatch->ReleaseForCcFlush)
    {
        /* Call the ReleaseForCcFlush FastIo handler */
        Status = FastDispatch->ReleaseForCcFlush(FileObject, BaseDeviceObject);
    }

    if (!NT_SUCCESS(Status))
    {
        /* No FastIo handler (or it failed). Release PagingIO resource and
           then Main resource */
        if (FcbHeader->PagingIoResource) ExReleaseResourceLite(FcbHeader->PagingIoResource);
        if (FcbHeader->Resource) ExReleaseResourceLite(FcbHeader->Resource);
    }

    /* Release master FsRtl lock */
    FsRtlExitFileSystem();
}

/**
 * @brief Get the resource to acquire when Mod Writer flushes data to disk
 *
 * @param FcbHeader - FCB header from the file object
 * @param EndingOffset - The end offset of the write to be done
 * @param ResourceToAcquire - Pointer receiving the resource to acquire before doing the write
 *
 * @return BOOLEAN specifying whether the resource must be acquired exclusively
 */
static
BOOLEAN
FsRtlpGetResourceForModWrite(_In_ PFSRTL_COMMON_FCB_HEADER FcbHeader,
                             _In_ PLARGE_INTEGER EndingOffset,
                             _Outptr_result_maybenull_ PERESOURCE* ResourceToAcquire)
{
    /*
     * Decide on type of locking and type of resource based on
     *  - Flags
     *  - Whether we're extending ValidDataLength
     */
    if (FlagOn(FcbHeader->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX))
    {
        /* Acquire main resource, exclusive */
        *ResourceToAcquire = FcbHeader->Resource;
        return TRUE;
    }

    /* We will acquire shared. Which one ? */
    if (FlagOn(FcbHeader->Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH))
    {
        *ResourceToAcquire = FcbHeader->Resource;
    }
    else
    {
        *ResourceToAcquire = FcbHeader->PagingIoResource;
    }

    /* We force exclusive lock if this write modifies the valid data length */
    return (EndingOffset->QuadPart > FcbHeader->ValidDataLength.QuadPart);
}

/**
 * @brief Lock a file object before flushing pages to disk. 
 *        To be called by the Modified Page Writer (MPW)
 *
 * @param FileObject - The file object to lock
 * @param EndingOffset - The end offset of the write to be done
 * @param ResourceToRelease - Pointer receiving the resource to release after the write
 *
 * @return Relevant NTSTATUS value
 */
_Check_return_
NTSTATUS
NTAPI
FsRtlAcquireFileForModWriteEx(_In_ PFILE_OBJECT FileObject,
                              _In_ PLARGE_INTEGER EndingOffset,
                              _Outptr_result_maybenull_ PERESOURCE *ResourceToRelease)
{
    PFSRTL_COMMON_FCB_HEADER FcbHeader;
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    PERESOURCE ResourceToAcquire = NULL;
    BOOLEAN Exclusive;
    BOOLEAN Result;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Get Device Object and Fast Calls */
    FcbHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if Fast Calls are supported, and check AcquireForModWrite */
    if (FastDispatch &&
        FastDispatch->AcquireForModWrite)
    {
        /* Call the AcquireForModWrite FastIo handler */
        Status = FastDispatch->AcquireForModWrite(FileObject,
                                                  EndingOffset,
                                                  ResourceToRelease,
                                                  BaseDeviceObject);

        /* Return either success or inability to wait.
           In case of other failure - fall through */
        if (Status == STATUS_SUCCESS ||
            Status == STATUS_CANT_WAIT)
        {
            return Status;
        }
    }

    /* Check what and how we should acquire */
    Exclusive = FsRtlpGetResourceForModWrite(FcbHeader, EndingOffset, &ResourceToAcquire);

    /* Acquire the resource and loop until we're sure we got this right. */
    while (TRUE)
    {
        BOOLEAN OldExclusive;
        PERESOURCE OldResourceToAcquire;

        if (ResourceToAcquire == NULL)
        {
            /* 
             * There's nothing to acquire, we can simply return success
             */

            break;
        }

        if (Exclusive)
        {
            Result = ExAcquireResourceExclusiveLite(ResourceToAcquire, FALSE);
        }
        else
        {
            Result = ExAcquireSharedWaitForExclusive(ResourceToAcquire, FALSE);
        }

        if (!Result)
        {
            return STATUS_CANT_WAIT;
        }

        /* Does this still hold true? */
        OldExclusive = Exclusive;
        OldResourceToAcquire = ResourceToAcquire;
        Exclusive = FsRtlpGetResourceForModWrite(FcbHeader, EndingOffset, &ResourceToAcquire);

        if ((OldExclusive == Exclusive) && (OldResourceToAcquire == ResourceToAcquire))
        {
            /* We're good */
            break;
        }

        /* Can we fix this situation? */
        if ((OldResourceToAcquire == ResourceToAcquire) && !Exclusive)
        {
            /* We can easily do so */
            ExConvertExclusiveToSharedLite(ResourceToAcquire);
            break;
        }

        /* Things have changed since we acquired the lock. Start again */
        ExReleaseResourceLite(OldResourceToAcquire);
    }

    /* If we're here, this means that we succeeded */
    *ResourceToRelease = ResourceToAcquire;
    return STATUS_SUCCESS;
}

/**
 * @brief Unlock a file object after flushing pages to disk. 
 *        To be called by the Modified Page Writer (MPW) after a succesful call to
 *        FsRtlAcquireFileForModWriteEx
 *
 * @param FileObject - The file object to unlock
 * @param ResourceToRelease - The resource to release
 */
VOID
NTAPI
FsRtlReleaseFileForModWrite(_In_ PFILE_OBJECT FileObject,
                            _In_ PERESOURCE ResourceToRelease)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if Fast Calls are supported and check ReleaseForModWrite */
    if (FastDispatch &&
        FastDispatch->ReleaseForModWrite)
    {
        /* Call the ReleaseForModWrite FastIo handler */
        Status = FastDispatch->ReleaseForModWrite(FileObject,
                                                  ResourceToRelease,
                                                  BaseDeviceObject);
    }

    /* Just release the resource if previous op failed */
    if (!NT_SUCCESS(Status))
    {
        ExReleaseResourceLite(ResourceToRelease);
    }
}


/*++
 * @name FsRtlRegisterFileSystemFilterCallbacks
 * @unimplemented
 *
 * FILLME
 *
 * @param FilterDriverObject
 *        FILLME
 *
 * @param Callbacks
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
FsRtlRegisterFileSystemFilterCallbacks(
    PDRIVER_OBJECT FilterDriverObject,
    PFS_FILTER_CALLBACKS Callbacks)
{
    PFS_FILTER_CALLBACKS NewCallbacks;
    PEXTENDED_DRIVER_EXTENSION DriverExtension;
    PAGED_CODE();

    /* Verify parameters */
    if ((FilterDriverObject == NULL) || (Callbacks == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate a buffer for a copy of the callbacks */
    NewCallbacks = ExAllocatePoolWithTag(NonPagedPool,
                                         Callbacks->SizeOfFsFilterCallbacks,
                                         'gmSF');
    if (NewCallbacks == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the callbacks */
    RtlCopyMemory(NewCallbacks, Callbacks, Callbacks->SizeOfFsFilterCallbacks);

    /* Set the callbacks in the driver extension */
    DriverExtension = (PEXTENDED_DRIVER_EXTENSION)FilterDriverObject->DriverExtension;
    DriverExtension->FsFilterCallbacks = NewCallbacks;

    return STATUS_SUCCESS;
}
