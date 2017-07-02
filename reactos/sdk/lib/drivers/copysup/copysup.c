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
