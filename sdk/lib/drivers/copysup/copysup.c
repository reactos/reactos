/*
 *  ReactOS kernel
 *  Copyright (C) 2017 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             sdk/lib/drivers/copysup/copysup.c
 * PURPOSE:          CopySup library
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "copysup.h"
#include <pseh/pseh2.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOLEAN
FsRtlCopyRead2(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID TopLevelContext)
{
    BOOLEAN Ret;
    ULONG PageCount;
    LARGE_INTEGER FinalOffset;
    PFSRTL_COMMON_FCB_HEADER Fcb;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDEVICE_OBJECT RelatedDeviceObject;

    PAGED_CODE();

    Ret = TRUE;
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(FileOffset, Length);

    /* Null-length read is always OK */
    if (Length == 0)
    {
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_SUCCESS;

        return TRUE;
    }

    /* Check we don't overflow */
    FinalOffset.QuadPart = FileOffset->QuadPart + Length;
    if (FinalOffset.QuadPart <= 0)
    {
        return FALSE;
    }

    /* Get the FCB (at least, its header) */
    Fcb = FileObject->FsContext;

    FsRtlEnterFileSystem();

    /* Acquire its resource (shared) */
    if (Wait)
    {
        ExAcquireResourceSharedLite(Fcb->Resource, TRUE);
    }
    else
    {
        if (!ExAcquireResourceSharedLite(Fcb->Resource, FALSE))
        {
            Ret = FALSE;
            goto CriticalSection;
        }
    }

    /* If cache wasn't initialized, or FastIO isn't possible, fail */
    if (FileObject->PrivateCacheMap == NULL || Fcb->IsFastIoPossible == FastIoIsNotPossible)
    {
            Ret = FALSE;
            goto Resource;
    }

    /* If FastIO is questionable, then, question! */
    if (Fcb->IsFastIoPossible == FastIoIsQuestionable)
    {
        RelatedDeviceObject = IoGetRelatedDeviceObject(FileObject);
        FastIoDispatch = RelatedDeviceObject->DriverObject->FastIoDispatch;
        ASSERT(FastIoDispatch != NULL);
        ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

        /* If it's not possible, then fail */
        if (!FastIoDispatch->FastIoCheckIfPossible(FileObject, FileOffset, Length,
                                                   Wait, LockKey, TRUE, IoStatus, RelatedDeviceObject))
        {
            Ret = FALSE;
            goto Resource;
        }
    }

    /* If we get beyond file end... */
    if (FinalOffset.QuadPart > Fcb->FileSize.QuadPart)
    {
        /* Fail if the offset was already beyond file end */
        if (FileOffset->QuadPart >= Fcb->FileSize.QuadPart)
        {
            IoStatus->Information = 0;
            IoStatus->Status = STATUS_END_OF_FILE;
            goto Resource;
        }

        /* Otherwise, just fix read length */
        Length = (ULONG)(Fcb->FileSize.QuadPart - FileOffset->QuadPart);
    }

    /* Set caller provided context as TLI */
    IoSetTopLevelIrp(TopLevelContext);

    _SEH2_TRY
    {
        /* If we cannot wait, or if file is bigger than 4GB */
        if (!Wait || (FinalOffset.HighPart | Fcb->FileSize.HighPart) != 0)
        {
            /* Forward to Cc */
            Ret = CcCopyRead(FileObject, FileOffset, Length, Wait, Buffer, IoStatus);
            SetFlag(FileObject->Flags, FO_FILE_FAST_IO_READ);

            /* Validate output */
            ASSERT(!Ret || (IoStatus->Status == STATUS_END_OF_FILE) || (((ULONGLONG)FileOffset->QuadPart + IoStatus->Information) <= (ULONGLONG)Fcb->FileSize.QuadPart));
        }
        else
        {
            /* Forward to Cc */
            CcFastCopyRead(FileObject, FileOffset->LowPart, Length, PageCount, Buffer, IoStatus);
            SetFlag(FileObject->Flags, FO_FILE_FAST_IO_READ);

            /* Validate output */
            ASSERT((IoStatus->Status == STATUS_END_OF_FILE) || ((FileOffset->LowPart + IoStatus->Information) <= Fcb->FileSize.LowPart));
        }

        /* If read was successful, update the byte offset in the FO */
        if (Ret)
        {
            FileObject->CurrentByteOffset.QuadPart = FileOffset->QuadPart + IoStatus->Information;
        }
    }
    _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        Ret = FALSE;
    }
    _SEH2_END;

    /* Reset TLI */
    IoSetTopLevelIrp(NULL);

Resource:
    ExReleaseResourceLite(Fcb->Resource);
CriticalSection:
    FsRtlExitFileSystem();

    return Ret;
}

/*
 * @implemented
 */
BOOLEAN
FsRtlCopyWrite2(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID TopLevelContext)
{
    IO_STATUS_BLOCK LocalIoStatus;
    PFSRTL_ADVANCED_FCB_HEADER Fcb;
    BOOLEAN WriteToEof, AcquiredShared, FileSizeChanged, Ret;
    LARGE_INTEGER WriteOffset, LastOffset, InitialFileSize, InitialValidDataLength;

    PAGED_CODE();

    /* First, check whether we're writing to EOF */
    WriteToEof = ((FileOffset->LowPart == FILE_WRITE_TO_END_OF_FILE) &&
                  (FileOffset->HighPart == -1));

    /* If Cc says we cannot write, fail now */
    if (!CcCanIWrite(FileObject, Length, Wait, FALSE))
    {
        return FALSE;
    }

    /* Write through means no cache */
    if (BooleanFlagOn(FileObject->Flags, FO_WRITE_THROUGH))
    {
        return FALSE;
    }

    /* If write is > 64Kb, don't use FastIO */
    if (!CcCopyWriteWontFlush(FileObject, FileOffset, Length))
    {
        return FALSE;
    }

    /* Initialize the IO_STATUS_BLOCK */
    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = Length;

    /* No length, it's already written! */
    if (Length == 0)
    {
        return TRUE;
    }

    AcquiredShared = FALSE;
    FileSizeChanged = FALSE;
    Fcb = FileObject->FsContext;

    FsRtlEnterFileSystem();

    /* If we cannot wait, or deal with files bigger then 4GB */
    if (!Wait || (Fcb->AllocationSize.HighPart != 0))
    {
        /* If we're to extend the file, then, acquire exclusively */
        if (WriteToEof || FileOffset->QuadPart + Length > Fcb->ValidDataLength.QuadPart)
        {
            if (!ExAcquireResourceExclusiveLite(Fcb->Resource, Wait))
            {
                FsRtlExitFileSystem();
                return FALSE;
            }
        }
        /* Otherwise, a shared lock is enough */
        else
        {
            if (!ExAcquireResourceSharedLite(Fcb->Resource, Wait))
            {
                FsRtlExitFileSystem();
                return FALSE;
            }

            AcquiredShared = TRUE;
        }

        /* Get first write offset, and last */
        if (WriteToEof)
        {
            WriteOffset.QuadPart = Fcb->FileSize.QuadPart;
            LastOffset.QuadPart = WriteOffset.QuadPart + Length;
        }
        else
        {
            WriteOffset.QuadPart = FileOffset->QuadPart;
            LastOffset.QuadPart = WriteOffset.QuadPart + Length;
        }

        /* If cache wasn't initialized, fail */
        if (FileObject->PrivateCacheMap == NULL ||
            Fcb->IsFastIoPossible == FastIoIsNotPossible)
        {
            ExReleaseResourceLite(Fcb->Resource);
            FsRtlExitFileSystem();

            return FALSE;
        }

        /* If we're to write beyond allocation size, it's no go,
         * same is we create a hole bigger than 8kb
         */
        if ((Fcb->ValidDataLength.QuadPart + 0x2000 <= WriteOffset.QuadPart) ||
            (Length > MAXLONGLONG - WriteOffset.QuadPart) ||
            (Fcb->AllocationSize.QuadPart < LastOffset.QuadPart))
        {
            ExReleaseResourceLite(Fcb->Resource);
            FsRtlExitFileSystem();

            return FALSE;
        }

        /* If we have to extend the VDL, shared lock isn't enough */
        if (AcquiredShared && LastOffset.QuadPart > Fcb->ValidDataLength.QuadPart)
        {
            /* So release, and attempt to acquire exclusively */
            ExReleaseResourceLite(Fcb->Resource);
            if (!ExAcquireResourceExclusiveLite(Fcb->Resource, Wait))
            {
                FsRtlExitFileSystem();
                return FALSE;
            }

            /* Get again EOF, in case file size changed in between */
            if (WriteToEof)
            {
                WriteOffset.QuadPart = Fcb->FileSize.QuadPart;
                LastOffset.QuadPart = WriteOffset.QuadPart + Length;
            }

            /* Make sure caching is still enabled */
            if (FileObject->PrivateCacheMap == NULL ||
                Fcb->IsFastIoPossible == FastIoIsNotPossible)
            {
                ExReleaseResourceLite(Fcb->Resource);
                FsRtlExitFileSystem();

                return FALSE;
            }

            /* And that we're not writing beyond allocation size */
            if (Fcb->AllocationSize.QuadPart < LastOffset.QuadPart)
            {
                ExReleaseResourceLite(Fcb->Resource);
                FsRtlExitFileSystem();

                return FALSE;
            }
        }

        /* If FastIO is questionable, then question */
        if (Fcb->IsFastIoPossible == FastIoIsQuestionable)
        {
            PFAST_IO_DISPATCH FastIoDispatch;
            PDEVICE_OBJECT RelatedDeviceObject;

            RelatedDeviceObject = IoGetRelatedDeviceObject(FileObject);
            FastIoDispatch = RelatedDeviceObject->DriverObject->FastIoDispatch;
            ASSERT(FastIoDispatch != NULL);
            ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

            if (!FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                       &WriteOffset,
                                                       Length, Wait, LockKey,
                                                       FALSE, &LocalIoStatus,
                                                       RelatedDeviceObject))
            {
                ExReleaseResourceLite(Fcb->Resource);
                FsRtlExitFileSystem();

                return FALSE;
            }
        }

        /* If we write beyond EOF, then, save previous sizes (in case of failure)
         * and update file size, to allow writing
         */
        if (LastOffset.QuadPart > Fcb->FileSize.QuadPart)
        {
            FileSizeChanged = TRUE;
            InitialFileSize.QuadPart = Fcb->FileSize.QuadPart;
            InitialValidDataLength.QuadPart = Fcb->ValidDataLength.QuadPart;

            if (LastOffset.HighPart != Fcb->FileSize.HighPart &&
                Fcb->PagingIoResource != NULL)
            {
                ExAcquireResourceExclusiveLite(Fcb->PagingIoResource, TRUE);
                Fcb->FileSize.QuadPart = LastOffset.QuadPart;
                ExReleaseResourceLite(Fcb->PagingIoResource);
            }
            else
            {
                Fcb->FileSize.QuadPart = LastOffset.QuadPart;
            }
        }

        /* Set caller provided context as top level IRP */
        IoSetTopLevelIrp(TopLevelContext);

        Ret = TRUE;

        /* And perform the writing */
        _SEH2_TRY
        {
            /* Check whether we've to create a hole first */
            if (LastOffset.QuadPart > Fcb->ValidDataLength.QuadPart)
            {
                Ret = CcZeroData(FileObject, &Fcb->ValidDataLength,
                                 &WriteOffset, Wait);
            }

            /* If not needed, or if it worked, write data */
            if (Ret)
            {
                Ret = CcCopyWrite(FileObject, &WriteOffset,
                                  Length, Wait, Buffer);
            }
        }
        _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                                             EXCEPTION_EXECUTE_HANDLER :
                                             EXCEPTION_CONTINUE_SEARCH)
        {
            Ret = FALSE;
        }
        _SEH2_END;

        /* Restore top level IRP */
        IoSetTopLevelIrp(NULL);

        /* If writing succeed */
        if (Ret)
        {
            /* If we wrote beyond VDL, update it */
            if (LastOffset.QuadPart > Fcb->ValidDataLength.QuadPart)
            {
                if (LastOffset.HighPart != Fcb->ValidDataLength.HighPart &&
                    Fcb->PagingIoResource != NULL)
                {
                    ExAcquireResourceExclusiveLite(Fcb->PagingIoResource, TRUE);
                    Fcb->ValidDataLength.QuadPart = LastOffset.QuadPart;
                    ExReleaseResourceLite(Fcb->PagingIoResource);
                }
                else
                {
                    Fcb->ValidDataLength.QuadPart = LastOffset.QuadPart;
                }
            }

            /* File was obviously modified */
            SetFlag(FileObject->Flags, FO_FILE_MODIFIED);

            /* And if we increased it, modify size in Cc and update FO */
            if (FileSizeChanged)
            {
                (*CcGetFileSizePointer(FileObject)).QuadPart = LastOffset.QuadPart;
                SetFlag(FileObject->Flags, FO_FILE_SIZE_CHANGED);
            }

            /* Update offset */
            FileObject->CurrentByteOffset.QuadPart = WriteOffset.QuadPart + Length;
        }
        else
        {
            /* We failed, we need to restore previous sizes */
            if (FileSizeChanged)
            {
                if (Fcb->PagingIoResource != NULL)
                {
                    ExAcquireResourceExclusiveLite(Fcb->PagingIoResource, TRUE);
                    Fcb->FileSize.QuadPart = InitialFileSize.QuadPart;
                    Fcb->ValidDataLength.QuadPart = InitialValidDataLength.QuadPart;
                    ExReleaseResourceLite(Fcb->PagingIoResource);
                }
                else
                {
                    Fcb->FileSize.QuadPart = InitialFileSize.QuadPart;
                    Fcb->ValidDataLength.QuadPart = InitialValidDataLength.QuadPart;
                }
            }
        }
    }
    else
    {
        BOOLEAN AboveFour;

        WriteOffset.HighPart = 0;
        LastOffset.HighPart = 0;

        /* If we're to extend the file, then, acquire exclusively
         * Here, easy stuff, we know we can wait, no return to check!
         */
        if (WriteToEof || FileOffset->QuadPart + Length > Fcb->ValidDataLength.QuadPart)
        {
            ExAcquireResourceExclusiveLite(Fcb->Resource, TRUE);
        }
        /* Otherwise, a shared lock is enough */
        else
        {
            ExAcquireResourceSharedLite(Fcb->Resource, TRUE);
            AcquiredShared = TRUE;
        }

        /* Get first write offset, and last
         * Also check whether our writing will bring us
         * beyond the 4GB
         */
        if (WriteToEof)
        {
            WriteOffset.LowPart = Fcb->FileSize.LowPart;
            LastOffset.LowPart = WriteOffset.LowPart + Length;
            AboveFour = (LastOffset.LowPart < Fcb->FileSize.LowPart);
        }
        else
        {
            WriteOffset.LowPart = FileOffset->LowPart;
            LastOffset.LowPart = WriteOffset.LowPart + Length;
            AboveFour = (LastOffset.LowPart < FileOffset->LowPart) ||
                         (FileOffset->HighPart != 0);
        }

        /* If cache wasn't initialized, fail */
        if (FileObject->PrivateCacheMap == NULL ||
            Fcb->IsFastIoPossible == FastIoIsNotPossible)
        {
            ExReleaseResourceLite(Fcb->Resource);
            FsRtlExitFileSystem();

            return FALSE;
        }

        /* If we're to write beyond allocation size, it's no go,
         * same is we create a hole bigger than 8kb
         * same if we end writing beyond 4GB
         */
        if ((Fcb->AllocationSize.LowPart < LastOffset.LowPart) ||
            (WriteOffset.LowPart >= Fcb->ValidDataLength.LowPart + 0x2000) ||
            AboveFour)
        {
            ExReleaseResourceLite(Fcb->Resource);
            FsRtlExitFileSystem();

            return FALSE;
        }

        /* If we have to extend the VDL, shared lock isn't enough */
        if (AcquiredShared && LastOffset.LowPart > Fcb->ValidDataLength.LowPart)
        {
            /* So release, and acquire exclusively */
            ExReleaseResourceLite(Fcb->Resource);
            ExAcquireResourceExclusiveLite(Fcb->Resource, TRUE);

            /* Get again EOF, in case file size changed in between and
             * recheck we won't go beyond 4GB
             */
            if (WriteToEof)
            {
                WriteOffset.LowPart = Fcb->FileSize.LowPart;
                LastOffset.LowPart = WriteOffset.LowPart + Length;
                AboveFour = (LastOffset.LowPart < Fcb->FileSize.LowPart);
            }

            /* Make sure caching is still enabled */
            if (FileObject->PrivateCacheMap == NULL ||
                Fcb->IsFastIoPossible == FastIoIsNotPossible)
            {
                ExReleaseResourceLite(Fcb->Resource);
                FsRtlExitFileSystem();

                return FALSE;
            }

            /* And that we're not writing beyond allocation size
             * and that we're not going above 4GB
             */
            if ((Fcb->AllocationSize.LowPart < LastOffset.LowPart) ||
                (Fcb->AllocationSize.HighPart != 0) || AboveFour)
            {
                ExReleaseResourceLite(Fcb->Resource);
                FsRtlExitFileSystem();

                return FALSE;
            }
        }

        /* If FastIO is questionable, then question */
        if (Fcb->IsFastIoPossible == FastIoIsQuestionable)
        {
            PFAST_IO_DISPATCH FastIoDispatch;
            PDEVICE_OBJECT RelatedDeviceObject;

            RelatedDeviceObject = IoGetRelatedDeviceObject(FileObject);
            FastIoDispatch = RelatedDeviceObject->DriverObject->FastIoDispatch;
            ASSERT(FastIoDispatch != NULL);
            ASSERT(FastIoDispatch->FastIoCheckIfPossible != NULL);

            if (!FastIoDispatch->FastIoCheckIfPossible(FileObject,
                                                       &WriteOffset,
                                                       Length, Wait, LockKey,
                                                       FALSE, &LocalIoStatus,
                                                       RelatedDeviceObject))
            {
                ExReleaseResourceLite(Fcb->Resource);
                FsRtlExitFileSystem();

                return FALSE;
            }
        }

        /* If we write beyond EOF, then, save previous sizes (in case of failure)
         * and update file size, to allow writing
         */
        if (LastOffset.LowPart > Fcb->FileSize.LowPart)
        {
            FileSizeChanged = TRUE;
            InitialFileSize.LowPart = Fcb->FileSize.LowPart;
            InitialValidDataLength.LowPart = Fcb->ValidDataLength.LowPart;
            Fcb->FileSize.LowPart = LastOffset.LowPart;
        }

        /* Set caller provided context as top level IRP */
        IoSetTopLevelIrp(TopLevelContext);

        Ret = TRUE;

        /* And perform the writing */
        _SEH2_TRY
        {
            /* Check whether we've to create a hole first -
             * it cannot fail, we can wait
             */
            if (LastOffset.LowPart > Fcb->ValidDataLength.LowPart)
            {
                CcZeroData(FileObject, &Fcb->ValidDataLength, &WriteOffset, TRUE);
            }

            /* Write data */
            CcFastCopyWrite(FileObject, WriteOffset.LowPart, Length, Buffer);
        }
        _SEH2_EXCEPT(FsRtlIsNtstatusExpected(_SEH2_GetExceptionCode()) ?
                                             EXCEPTION_EXECUTE_HANDLER :
                                             EXCEPTION_CONTINUE_SEARCH)
        {
            Ret = FALSE;
        }
        _SEH2_END;

        /* Restore top level IRP */
        IoSetTopLevelIrp(NULL);

        /* If writing succeed */
        if (Ret)
        {
            /* If we wrote beyond VDL, update it */
            if (LastOffset.LowPart > Fcb->ValidDataLength.LowPart)
            {
                Fcb->ValidDataLength.LowPart = LastOffset.LowPart;
            }

            /* File was obviously modified */
            SetFlag(FileObject->Flags, FO_FILE_MODIFIED);

            /* And if we increased it, modify size in Cc and update FO */
            if (FileSizeChanged)
            {
                (*CcGetFileSizePointer(FileObject)).LowPart = LastOffset.LowPart;
                SetFlag(FileObject->Flags, FO_FILE_SIZE_CHANGED);
            }

            /* Update offset - we're still below 4GB, so high part must be 0 */
            FileObject->CurrentByteOffset.LowPart = WriteOffset.LowPart + Length;
            FileObject->CurrentByteOffset.HighPart = 0;
        }
        else
        {
            /* We failed, we need to restore previous sizes */
            if (FileSizeChanged)
            {
                if (Fcb->PagingIoResource != NULL)
                {
                    ExAcquireResourceExclusiveLite(Fcb->PagingIoResource, TRUE);
                    Fcb->FileSize.LowPart = InitialFileSize.LowPart;
                    Fcb->ValidDataLength.LowPart = InitialValidDataLength.LowPart;
                    ExReleaseResourceLite(Fcb->PagingIoResource);
                }
                else
                {
                    Fcb->FileSize.LowPart = InitialFileSize.LowPart;
                    Fcb->ValidDataLength.LowPart = InitialValidDataLength.LowPart;
                }
            }
        }
    }

    /* Release our resource and leave */
    ExReleaseResourceLite(Fcb->Resource);

    FsRtlExitFileSystem();

    return Ret;
}
