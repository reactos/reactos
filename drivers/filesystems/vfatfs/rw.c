/*
 * PROJECT:     VFAT Filesystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Reading and writing routines
 * COPYRIGHT:   Copyright 1998 Jason Filby <jasonfilby@yahoo.com>
 *              Copyright 2008-2018 Pierre Schweitzer <pierre@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/*
 * Uncomment to enable strict verification of cluster/offset pair
 * caching. If this option is enabled you lose all the benefits of
 * the caching and the read/write operations will actually be
 * slower. It's meant only for debugging!!!
 * - Filip Navara, 26/07/2004
 */
/* #define DEBUG_VERIFY_OFFSET_CACHING */

/* Arbitrary, taken from MS FastFAT, should be
 * refined given what we experience in common
 * out of stack operations
 */
#define OVERFLOW_READ_THRESHHOLD 0xE00

/* FUNCTIONS *****************************************************************/

/*
 * Return the next cluster in a FAT chain, possibly extending the chain if
 * necessary
 */
NTSTATUS
NextCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG FirstCluster,
    PULONG CurrentCluster,
    BOOLEAN Extend)
{
    if (FirstCluster == 1)
    {
        (*CurrentCluster) += DeviceExt->FatInfo.SectorsPerCluster;
        return STATUS_SUCCESS;
    }
    else
    {
        if (Extend)
            return GetNextClusterExtend(DeviceExt, (*CurrentCluster), CurrentCluster);
        else
            return GetNextCluster(DeviceExt, (*CurrentCluster), CurrentCluster);
    }
}

NTSTATUS
OffsetToCluster(
    PDEVICE_EXTENSION DeviceExt,
    ULONG FirstCluster,
    ULONG FileOffset,
    PULONG Cluster,
    BOOLEAN Extend)
{
    ULONG CurrentCluster;
    ULONG i;
    NTSTATUS Status;
/*
    DPRINT("OffsetToCluster(DeviceExt %x, Fcb %x, FirstCluster %x,"
           " FileOffset %x, Cluster %x, Extend %d)\n", DeviceExt,
           Fcb, FirstCluster, FileOffset, Cluster, Extend);
*/
    if (FirstCluster == 0)
    {
        DbgPrint("OffsetToCluster is called with FirstCluster = 0!\n");
        ASSERT(FALSE);
    }

    if (FirstCluster == 1)
    {
        /* root of FAT16 or FAT12 */
        *Cluster = DeviceExt->FatInfo.rootStart + FileOffset
            / (DeviceExt->FatInfo.BytesPerCluster) * DeviceExt->FatInfo.SectorsPerCluster;
        return STATUS_SUCCESS;
    }
    else
    {
        CurrentCluster = FirstCluster;
        if (Extend)
        {
            for (i = 0; i < FileOffset / DeviceExt->FatInfo.BytesPerCluster; i++)
            {
                Status = GetNextClusterExtend (DeviceExt, CurrentCluster, &CurrentCluster);
                if (!NT_SUCCESS(Status))
                    return Status;
            }
            *Cluster = CurrentCluster;
        }
        else
        {
            for (i = 0; i < FileOffset / DeviceExt->FatInfo.BytesPerCluster; i++)
            {
                Status = GetNextCluster (DeviceExt, CurrentCluster, &CurrentCluster);
                if (!NT_SUCCESS(Status))
                    return Status;
            }
            *Cluster = CurrentCluster;
       }
       return STATUS_SUCCESS;
   }
}

/*
 * FUNCTION: Reads data from a file
 */
static
NTSTATUS
VfatReadFileData(
    PVFAT_IRP_CONTEXT IrpContext,
    ULONG Length,
    LARGE_INTEGER ReadOffset,
    PULONG LengthRead)
{
    ULONG CurrentCluster;
    ULONG FirstCluster;
    ULONG StartCluster;
    ULONG ClusterCount;
    LARGE_INTEGER StartOffset;
    PDEVICE_EXTENSION DeviceExt;
    BOOLEAN First = TRUE;
    PVFATFCB Fcb;
    NTSTATUS Status;
    ULONG BytesDone;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONG LastCluster;
    ULONG LastOffset;

    /* PRECONDITION */
    ASSERT(IrpContext);
    DeviceExt = IrpContext->DeviceExt;
    ASSERT(DeviceExt);
    ASSERT(DeviceExt->FatInfo.BytesPerCluster);
    ASSERT(IrpContext->FileObject);
    ASSERT(IrpContext->FileObject->FsContext2 != NULL);

    DPRINT("VfatReadFileData(DeviceExt %p, FileObject %p, "
           "Length %u, ReadOffset 0x%I64x)\n", DeviceExt,
           IrpContext->FileObject, Length, ReadOffset.QuadPart);

    *LengthRead = 0;

    Fcb = IrpContext->FileObject->FsContext;
    BytesPerSector = DeviceExt->FatInfo.BytesPerSector;
    BytesPerCluster = DeviceExt->FatInfo.BytesPerCluster;

    ASSERT(ReadOffset.QuadPart + Length <= ROUND_UP_64(Fcb->RFCB.FileSize.QuadPart, BytesPerSector));
    ASSERT(ReadOffset.u.LowPart % BytesPerSector == 0);
    ASSERT(Length % BytesPerSector == 0);

    /* Is this a read of the FAT? */
    if (BooleanFlagOn(Fcb->Flags, FCB_IS_FAT))
    {
        ReadOffset.QuadPart += DeviceExt->FatInfo.FATStart * BytesPerSector;
        Status = VfatReadDiskPartial(IrpContext, &ReadOffset, Length, 0, TRUE);

        if (NT_SUCCESS(Status))
        {
            *LengthRead = Length;
        }
        else
        {
            DPRINT1("FAT reading failed, Status %x\n", Status);
        }
        return Status;
    }

    /* Is this a read of the Volume ? */
    if (BooleanFlagOn(Fcb->Flags, FCB_IS_VOLUME))
    {
        Status = VfatReadDiskPartial(IrpContext, &ReadOffset, Length, 0, TRUE);
        if (NT_SUCCESS(Status))
        {
            *LengthRead = Length;
        }
        else
        {
            DPRINT1("Volume reading failed, Status %x\n", Status);
        }
        return Status;
    }

    /* Find the first cluster */
    FirstCluster =
    CurrentCluster = vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);

    if (FirstCluster == 1)
    {
        /* Directory of FAT12/16 needs a special handling */
        if (ReadOffset.u.LowPart + Length > DeviceExt->FatInfo.rootDirectorySectors * BytesPerSector)
        {
            Length = DeviceExt->FatInfo.rootDirectorySectors * BytesPerSector - ReadOffset.u.LowPart;
        }
        ReadOffset.u.LowPart += DeviceExt->FatInfo.rootStart * BytesPerSector;

        /* Fire up the read command */
        Status = VfatReadDiskPartial (IrpContext, &ReadOffset, Length, 0, TRUE);
        if (NT_SUCCESS(Status))
        {
            *LengthRead = Length;
        }
        return Status;
    }

    ExAcquireFastMutex(&Fcb->LastMutex);
    LastCluster = Fcb->LastCluster;
    LastOffset = Fcb->LastOffset;
    ExReleaseFastMutex(&Fcb->LastMutex);

    /* Find the cluster to start the read from */
    if (LastCluster > 0 && ReadOffset.u.LowPart >= LastOffset)
    {
        Status = OffsetToCluster(DeviceExt, LastCluster,
                                 ROUND_DOWN(ReadOffset.u.LowPart, BytesPerCluster) -
                                 LastOffset,
                                 &CurrentCluster, FALSE);
#ifdef DEBUG_VERIFY_OFFSET_CACHING
        /* DEBUG VERIFICATION */
        {
            ULONG CorrectCluster;
            OffsetToCluster(DeviceExt, FirstCluster,
                            ROUND_DOWN(ReadOffset.u.LowPart, BytesPerCluster),
                            &CorrectCluster, FALSE);
            if (CorrectCluster != CurrentCluster)
                KeBugCheck(FAT_FILE_SYSTEM);
        }
#endif
    }
    else
    {
        Status = OffsetToCluster(DeviceExt, FirstCluster,
                                 ROUND_DOWN(ReadOffset.u.LowPart, BytesPerCluster),
                                 &CurrentCluster, FALSE);
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ExAcquireFastMutex(&Fcb->LastMutex);
    Fcb->LastCluster = CurrentCluster;
    Fcb->LastOffset = ROUND_DOWN (ReadOffset.u.LowPart, BytesPerCluster);
    ExReleaseFastMutex(&Fcb->LastMutex);

    KeInitializeEvent(&IrpContext->Event, NotificationEvent, FALSE);
    IrpContext->RefCount = 1;

    while (Length > 0 && CurrentCluster != 0xffffffff)
    {
        StartCluster = CurrentCluster;
        StartOffset.QuadPart = ClusterToSector(DeviceExt, StartCluster) * BytesPerSector;
        BytesDone = 0;
        ClusterCount = 0;

        do
        {
            ClusterCount++;
            if (First)
            {
                BytesDone =  min (Length, BytesPerCluster - (ReadOffset.u.LowPart % BytesPerCluster));
                StartOffset.QuadPart += ReadOffset.u.LowPart % BytesPerCluster;
                First = FALSE;
            }
            else
            {
                if (Length - BytesDone > BytesPerCluster)
                {
                    BytesDone += BytesPerCluster;
                }
                else
                {
                    BytesDone = Length;
                }
            }
            Status = NextCluster(DeviceExt, FirstCluster, &CurrentCluster, FALSE);
        }
        while (StartCluster + ClusterCount == CurrentCluster && NT_SUCCESS(Status) && Length > BytesDone);
        DPRINT("start %08x, next %08x, count %u\n",
               StartCluster, CurrentCluster, ClusterCount);

        ExAcquireFastMutex(&Fcb->LastMutex);
        Fcb->LastCluster = StartCluster + (ClusterCount - 1);
        Fcb->LastOffset = ROUND_DOWN(ReadOffset.u.LowPart, BytesPerCluster) + (ClusterCount - 1) * BytesPerCluster;
        ExReleaseFastMutex(&Fcb->LastMutex);

        /* Fire up the read command */
        Status = VfatReadDiskPartial (IrpContext, &StartOffset, BytesDone, *LengthRead, FALSE);
        if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
        {
            break;
        }
        *LengthRead += BytesDone;
        Length -= BytesDone;
        ReadOffset.u.LowPart += BytesDone;
    }

    if (InterlockedDecrement((PLONG)&IrpContext->RefCount) != 0)
    {
        KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
    }

    if (NT_SUCCESS(Status) || Status == STATUS_PENDING)
    {
        if (Length > 0)
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            Status = IrpContext->Irp->IoStatus.Status;
        }
    }

    return Status;
}

static
NTSTATUS
VfatWriteFileData(
    PVFAT_IRP_CONTEXT IrpContext,
    ULONG Length,
    LARGE_INTEGER WriteOffset)
{
    PDEVICE_EXTENSION DeviceExt;
    PVFATFCB Fcb;
    ULONG Count;
    ULONG FirstCluster;
    ULONG CurrentCluster;
    ULONG BytesDone;
    ULONG StartCluster;
    ULONG ClusterCount;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN First = TRUE;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    LARGE_INTEGER StartOffset;
    ULONG BufferOffset;
    ULONG LastCluster;
    ULONG LastOffset;

    /* PRECONDITION */
    ASSERT(IrpContext);
    DeviceExt = IrpContext->DeviceExt;
    ASSERT(DeviceExt);
    ASSERT(DeviceExt->FatInfo.BytesPerCluster);
    ASSERT(IrpContext->FileObject);
    ASSERT(IrpContext->FileObject->FsContext2 != NULL);

    Fcb = IrpContext->FileObject->FsContext;
    BytesPerCluster = DeviceExt->FatInfo.BytesPerCluster;
    BytesPerSector = DeviceExt->FatInfo.BytesPerSector;

    DPRINT("VfatWriteFileData(DeviceExt %p, FileObject %p, "
           "Length %u, WriteOffset 0x%I64x), '%wZ'\n", DeviceExt,
           IrpContext->FileObject, Length, WriteOffset.QuadPart,
           &Fcb->PathNameU);

    ASSERT(WriteOffset.QuadPart + Length <= Fcb->RFCB.AllocationSize.QuadPart);
    ASSERT(WriteOffset.u.LowPart % BytesPerSector == 0);
    ASSERT(Length % BytesPerSector == 0);

    /* Is this a write of the volume? */
    if (BooleanFlagOn(Fcb->Flags, FCB_IS_VOLUME))
    {
        Status = VfatWriteDiskPartial(IrpContext, &WriteOffset, Length, 0, TRUE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Volume writing failed, Status %x\n", Status);
        }
        return Status;
    }

    /* Is this a write to the FAT? */
    if (BooleanFlagOn(Fcb->Flags, FCB_IS_FAT))
    {
        WriteOffset.u.LowPart += DeviceExt->FatInfo.FATStart * BytesPerSector;
        IrpContext->RefCount = 1;
        for (Count = 0; Count < DeviceExt->FatInfo.FATCount; Count++)
        {
            Status = VfatWriteDiskPartial(IrpContext, &WriteOffset, Length, 0, FALSE);
            if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
            {
                DPRINT1("FAT writing failed, Status %x\n", Status);
                break;
            }
            WriteOffset.u.LowPart += Fcb->RFCB.FileSize.u.LowPart;
        }

        if (InterlockedDecrement((PLONG)&IrpContext->RefCount) != 0)
        {
            KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
        }

        if (NT_SUCCESS(Status) || Status == STATUS_PENDING)
        {
            Status = IrpContext->Irp->IoStatus.Status;
        }
        return Status;
    }

    /*
     * Find the first cluster
     */
    FirstCluster =
    CurrentCluster = vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);

    if (FirstCluster == 1)
    {
        ASSERT(WriteOffset.u.LowPart + Length <= DeviceExt->FatInfo.rootDirectorySectors * BytesPerSector);
        // Directory of FAT12/16 needs a special handling
        WriteOffset.u.LowPart += DeviceExt->FatInfo.rootStart * BytesPerSector;
        // Fire up the write command
        Status = VfatWriteDiskPartial (IrpContext, &WriteOffset, Length, 0, TRUE);
        return Status;
    }

    ExAcquireFastMutex(&Fcb->LastMutex);
    LastCluster = Fcb->LastCluster;
    LastOffset = Fcb->LastOffset;
    ExReleaseFastMutex(&Fcb->LastMutex);

    /*
     * Find the cluster to start the write from
     */
    if (LastCluster > 0 && WriteOffset.u.LowPart >= LastOffset)
    {
        Status = OffsetToCluster(DeviceExt, LastCluster,
                                 ROUND_DOWN(WriteOffset.u.LowPart, BytesPerCluster) -
                                 LastOffset,
                                 &CurrentCluster, FALSE);
#ifdef DEBUG_VERIFY_OFFSET_CACHING
        /* DEBUG VERIFICATION */
        {
            ULONG CorrectCluster;
            OffsetToCluster(DeviceExt, FirstCluster,
                            ROUND_DOWN(WriteOffset.u.LowPart, BytesPerCluster),
                            &CorrectCluster, FALSE);
            if (CorrectCluster != CurrentCluster)
                KeBugCheck(FAT_FILE_SYSTEM);
        }
#endif
    }
    else
    {
        Status = OffsetToCluster(DeviceExt, FirstCluster,
                                 ROUND_DOWN(WriteOffset.u.LowPart, BytesPerCluster),
                                 &CurrentCluster, FALSE);
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ExAcquireFastMutex(&Fcb->LastMutex);
    Fcb->LastCluster = CurrentCluster;
    Fcb->LastOffset = ROUND_DOWN (WriteOffset.u.LowPart, BytesPerCluster);
    ExReleaseFastMutex(&Fcb->LastMutex);

    IrpContext->RefCount = 1;
    BufferOffset = 0;

    while (Length > 0 && CurrentCluster != 0xffffffff)
    {
        StartCluster = CurrentCluster;
        StartOffset.QuadPart = ClusterToSector(DeviceExt, StartCluster) * BytesPerSector;
        BytesDone = 0;
        ClusterCount = 0;

        do
        {
            ClusterCount++;
            if (First)
            {
                BytesDone =  min (Length, BytesPerCluster - (WriteOffset.u.LowPart % BytesPerCluster));
                StartOffset.QuadPart += WriteOffset.u.LowPart % BytesPerCluster;
                First = FALSE;
            }
            else
            {
                if (Length - BytesDone > BytesPerCluster)
                {
                    BytesDone += BytesPerCluster;
                }
                else
                {
                    BytesDone = Length;
                }
            }
            Status = NextCluster(DeviceExt, FirstCluster, &CurrentCluster, FALSE);
        }
        while (StartCluster + ClusterCount == CurrentCluster && NT_SUCCESS(Status) && Length > BytesDone);
        DPRINT("start %08x, next %08x, count %u\n",
               StartCluster, CurrentCluster, ClusterCount);

        ExAcquireFastMutex(&Fcb->LastMutex);
        Fcb->LastCluster = StartCluster + (ClusterCount - 1);
        Fcb->LastOffset = ROUND_DOWN(WriteOffset.u.LowPart, BytesPerCluster) + (ClusterCount - 1) * BytesPerCluster;
        ExReleaseFastMutex(&Fcb->LastMutex);

        // Fire up the write command
        Status = VfatWriteDiskPartial (IrpContext, &StartOffset, BytesDone, BufferOffset, FALSE);
        if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
        {
            break;
        }
        BufferOffset += BytesDone;
        Length -= BytesDone;
        WriteOffset.u.LowPart += BytesDone;
    }

    if (InterlockedDecrement((PLONG)&IrpContext->RefCount) != 0)
    {
        KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
    }

    if (NT_SUCCESS(Status) || Status == STATUS_PENDING)
    {
        if (Length > 0)
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            Status = IrpContext->Irp->IoStatus.Status;
        }
    }

    return Status;
}

NTSTATUS
VfatCommonRead(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PVFATFCB Fcb;
    PVOID Buffer;
    NTSTATUS Status;
    ULONG Length = 0;
    ULONG BytesPerSector;
    LARGE_INTEGER ByteOffset;
    ULONG ReturnedLength = 0;
    BOOLEAN PagingIo, CanWait, IsVolume, NoCache;

    PagingIo = BooleanFlagOn(IrpContext->Irp->Flags, IRP_PAGING_IO);
    CanWait = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT);
    NoCache = BooleanFlagOn(IrpContext->Irp->Flags, IRP_NOCACHE);
    Fcb = IrpContext->FileObject->FsContext;
    IsVolume = BooleanFlagOn(Fcb->Flags, FCB_IS_VOLUME);

    ByteOffset = IrpContext->Stack->Parameters.Read.ByteOffset;
    Length = IrpContext->Stack->Parameters.Read.Length;
    BytesPerSector = IrpContext->DeviceExt->FatInfo.BytesPerSector;

    if (!PagingIo &&
        FsRtlAreThereCurrentFileLocks(&Fcb->FileLock))
    {
        if (!FsRtlCheckLockForReadAccess(&Fcb->FileLock, IrpContext->Irp))
        {
            return STATUS_FILE_LOCK_CONFLICT;
        }
    }

    Buffer = VfatGetUserBuffer(IrpContext->Irp, PagingIo);

    if (!PagingIo && !NoCache && !IsVolume)
    {
        // cached read
        Status = STATUS_SUCCESS;
        if (ByteOffset.u.LowPart + Length > Fcb->RFCB.FileSize.u.LowPart)
        {
            Length = Fcb->RFCB.FileSize.u.LowPart - ByteOffset.u.LowPart;
            Status = /*STATUS_END_OF_FILE*/STATUS_SUCCESS;
        }

        vfatAddToStat(IrpContext->DeviceExt, Base.UserFileReads, 1);
        vfatAddToStat(IrpContext->DeviceExt, Base.UserFileReadBytes, Length);

        _SEH2_TRY
        {
            if (IrpContext->FileObject->PrivateCacheMap == NULL)
            {
                CcInitializeCacheMap(IrpContext->FileObject,
                                     (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
                                     FALSE,
                                     &(VfatGlobalData->CacheMgrCallbacks),
                                     Fcb);
            }

            if (!CcCopyRead(IrpContext->FileObject,
                            &ByteOffset,
                            Length,
                            CanWait,
                            Buffer,
                            &IrpContext->Irp->IoStatus))
            {
                ASSERT(!CanWait);
                Status = STATUS_PENDING;
                goto ByeBye;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            goto ByeBye;
        }
        _SEH2_END;

        if (!NT_SUCCESS(IrpContext->Irp->IoStatus.Status))
        {
            Status = IrpContext->Irp->IoStatus.Status;
        }
    }
    else
    {
        // non cached read
        Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoWriteAccess);
        if (!NT_SUCCESS(Status))
        {
            goto ByeBye;
        }

        if (ByteOffset.QuadPart + Length > ROUND_UP_64(Fcb->RFCB.FileSize.QuadPart, BytesPerSector))
        {
            Length = (ULONG)(ROUND_UP_64(Fcb->RFCB.FileSize.QuadPart, BytesPerSector) - ByteOffset.QuadPart);
        }

        if (!IsVolume)
        {
            vfatAddToStat(IrpContext->DeviceExt, Fat.NonCachedReads, 1);
            vfatAddToStat(IrpContext->DeviceExt, Fat.NonCachedReadBytes, Length);
        }
        else
        {
            vfatAddToStat(IrpContext->DeviceExt, Base.MetaDataReads, 1);
            vfatAddToStat(IrpContext->DeviceExt, Base.MetaDataReadBytes, Length);
        }

        Status = VfatReadFileData(IrpContext, Length, ByteOffset, &ReturnedLength);
        if (NT_SUCCESS(Status))
        {
            IrpContext->Irp->IoStatus.Information = ReturnedLength;
        }
    }

ByeBye:
    return Status;
}

VOID
NTAPI
VfatStackOverflowRead(
    PVOID Context,
    IN PKEVENT Event)
{
    PVFAT_IRP_CONTEXT IrpContext;

    IrpContext = Context;
    /* In a separate thread, we can wait and resources got locked */
    SetFlag(IrpContext->Flags, IRPCONTEXT_CANWAIT);

    /* Perform the read operation */
    DPRINT1("Performing posted read\n");
    VfatCommonRead(IrpContext);

    KeSetEvent(Event, 0, FALSE);
}

VOID
VfatPostRead(
    PVFAT_IRP_CONTEXT IrpContext,
    PERESOURCE Lock,
    BOOLEAN PagingIo)
{
    KEVENT Event;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    ExAcquireResourceSharedLite(Lock, TRUE);

    /* If paging IO, call the non failing but blocking routine */
    if (PagingIo)
    {
        FsRtlPostPagingFileStackOverflow(IrpContext, &Event, VfatStackOverflowRead);
    }
    else
    {
        FsRtlPostStackOverflow(IrpContext, &Event, VfatStackOverflowRead);
    }

    /* Wait till it's done */
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
}

NTSTATUS
VfatRead(
    PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;
    PVFATFCB Fcb;
    ULONG Length = 0;
    PERESOURCE Resource = NULL;
    LARGE_INTEGER ByteOffset;
    ULONG BytesPerSector;
    BOOLEAN PagingIo, CanWait, IsVolume, NoCache;

    ASSERT(IrpContext);

    DPRINT("VfatRead(IrpContext %p)\n", IrpContext);

    ASSERT(IrpContext->DeviceObject);

    PagingIo = BooleanFlagOn(IrpContext->Irp->Flags, IRP_PAGING_IO);
    CanWait = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT);
    NoCache = BooleanFlagOn(IrpContext->Irp->Flags, IRP_NOCACHE);

    // This request is not allowed on the main device object
    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        DPRINT("VfatRead is called with the main device object.\n");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto ByeBye;
    }

    ASSERT(IrpContext->DeviceExt);
    ASSERT(IrpContext->FileObject);
    Fcb = IrpContext->FileObject->FsContext;
    ASSERT(Fcb);

    IsVolume = BooleanFlagOn(Fcb->Flags, FCB_IS_VOLUME);

    if (BooleanFlagOn(Fcb->Flags, FCB_IS_PAGE_FILE))
    {
        PFATINFO FatInfo = &IrpContext->DeviceExt->FatInfo;
        IrpContext->Stack->Parameters.Read.ByteOffset.QuadPart += FatInfo->dataStart * FatInfo->BytesPerSector;
        IoSkipCurrentIrpStackLocation(IrpContext->Irp);
        IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;
        DPRINT("Read from page file, disk offset %I64x\n", IrpContext->Stack->Parameters.Read.ByteOffset.QuadPart);
        Status = IoCallDriver(IrpContext->DeviceExt->StorageDevice, IrpContext->Irp);
        return Status;
    }

    DPRINT("<%wZ>\n", &Fcb->PathNameU);

    ByteOffset = IrpContext->Stack->Parameters.Read.ByteOffset;
    Length = IrpContext->Stack->Parameters.Read.Length;
    BytesPerSector = IrpContext->DeviceExt->FatInfo.BytesPerSector;

    /* fail if file is a directory and no paged read */
    if (vfatFCBIsDirectory(Fcb) && !PagingIo)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto ByeBye;
    }

    DPRINT("'%wZ', Offset: %u, Length %u\n", &Fcb->PathNameU, ByteOffset.u.LowPart, Length);

    if (ByteOffset.u.HighPart && !IsVolume)
    {
       Status = STATUS_INVALID_PARAMETER;
       goto ByeBye;
    }

    if (Length == 0)
    {
        IrpContext->Irp->IoStatus.Information = 0;
        Status = STATUS_SUCCESS;
        goto ByeBye;
    }

    if (ByteOffset.QuadPart >= Fcb->RFCB.FileSize.QuadPart)
    {
       IrpContext->Irp->IoStatus.Information = 0;
       Status = STATUS_END_OF_FILE;
       goto ByeBye;
    }

    if (NoCache || PagingIo || IsVolume)
    {
        if (ByteOffset.u.LowPart % BytesPerSector != 0 || Length % BytesPerSector != 0)
        {
            DPRINT("%u %u\n", ByteOffset.u.LowPart, Length);
            // non cached read must be sector aligned
            Status = STATUS_INVALID_PARAMETER;
            goto ByeBye;
        }
    }

    if (IsVolume)
    {
        Resource = &IrpContext->DeviceExt->DirResource;
    }
    else if (PagingIo)
    {
        Resource = &Fcb->PagingIoResource;
    }
    else
    {
        Resource = &Fcb->MainResource;
    }

    /* Are we out of stack for the rest of the operation? */
    if (IoGetRemainingStackSize() < OVERFLOW_READ_THRESHHOLD)
    {
        /* Lock the buffer */
        Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoWriteAccess);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* And post the read to the overflow thread */
        VfatPostRead(IrpContext, Resource, PagingIo);

        /* Return the appropriate status */
        return IrpContext->Irp->IoStatus.Status;
    }

    if (!ExAcquireResourceSharedLite(Resource, CanWait))
    {
        Resource = NULL;
        Status = STATUS_PENDING;
        goto ByeBye;
    }

    Status = VfatCommonRead(IrpContext);

ByeBye:
    if (Resource)
    {
        ExReleaseResourceLite(Resource);
    }

    if (Status == STATUS_PENDING)
    {
        Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoWriteAccess);
        if (NT_SUCCESS(Status))
        {
            Status = VfatMarkIrpContextForQueue(IrpContext);
        }
    }
    else
    {
        IrpContext->Irp->IoStatus.Status = Status;
        if (BooleanFlagOn(IrpContext->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
            !PagingIo &&
            (NT_SUCCESS(Status) || Status == STATUS_END_OF_FILE))
        {
            IrpContext->FileObject->CurrentByteOffset.QuadPart =
                ByteOffset.QuadPart + IrpContext->Irp->IoStatus.Information;
        }

        if (NT_SUCCESS(Status))
            IrpContext->PriorityBoost = IO_DISK_INCREMENT;
    }
    DPRINT("%x\n", Status);
    return Status;
}

NTSTATUS
VfatWrite(
    PVFAT_IRP_CONTEXT *pIrpContext)
{
    PVFAT_IRP_CONTEXT IrpContext = *pIrpContext;
    PVFATFCB Fcb;
    PERESOURCE Resource = NULL;
    LARGE_INTEGER ByteOffset;
    LARGE_INTEGER OldFileSize;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0;
    PVOID Buffer;
    ULONG BytesPerSector;
    BOOLEAN PagingIo, CanWait, IsVolume, IsFAT, NoCache;

    ASSERT(IrpContext);

    DPRINT("VfatWrite(IrpContext %p)\n", IrpContext);

    ASSERT(IrpContext->DeviceObject);

    PagingIo = BooleanFlagOn(IrpContext->Irp->Flags, IRP_PAGING_IO);
    CanWait = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT);
    NoCache = BooleanFlagOn(IrpContext->Irp->Flags, IRP_NOCACHE);

    // This request is not allowed on the main device object
    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        DPRINT("VfatWrite is called with the main device object.\n");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto ByeBye;
    }

    ASSERT(IrpContext->DeviceExt);
    ASSERT(IrpContext->FileObject);
    Fcb = IrpContext->FileObject->FsContext;
    ASSERT(Fcb);

    IsVolume = BooleanFlagOn(Fcb->Flags, FCB_IS_VOLUME);
    IsFAT = BooleanFlagOn(Fcb->Flags, FCB_IS_FAT);

    if (BooleanFlagOn(Fcb->Flags, FCB_IS_PAGE_FILE))
    {
        PFATINFO FatInfo = &IrpContext->DeviceExt->FatInfo;
        IrpContext->Stack->Parameters.Write.ByteOffset.QuadPart += FatInfo->dataStart * FatInfo->BytesPerSector;
        IoSkipCurrentIrpStackLocation(IrpContext->Irp);
        IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;
        DPRINT("Write to page file, disk offset %I64x\n", IrpContext->Stack->Parameters.Write.ByteOffset.QuadPart);
        Status = IoCallDriver(IrpContext->DeviceExt->StorageDevice, IrpContext->Irp);
        return Status;
    }

    DPRINT("<%wZ>\n", &Fcb->PathNameU);

    /* fail if file is a directory and no paged read */
    if (vfatFCBIsDirectory(Fcb) && !PagingIo)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto ByeBye;
    }

    ByteOffset = IrpContext->Stack->Parameters.Write.ByteOffset;
    if (ByteOffset.u.LowPart == FILE_WRITE_TO_END_OF_FILE &&
        ByteOffset.u.HighPart == -1)
    {
        ByteOffset.QuadPart = Fcb->RFCB.FileSize.QuadPart;
    }
    Length = IrpContext->Stack->Parameters.Write.Length;
    BytesPerSector = IrpContext->DeviceExt->FatInfo.BytesPerSector;

    if (ByteOffset.u.HighPart && !IsVolume)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto ByeBye;
    }

    if (IsFAT || IsVolume ||
        vfatDirEntryGetFirstCluster(IrpContext->DeviceExt, &Fcb->entry) == 1)
    {
        if (ByteOffset.QuadPart + Length > Fcb->RFCB.FileSize.QuadPart)
        {
            // we can't extend the FAT, the volume or the root on FAT12/FAT16
            Status = STATUS_END_OF_FILE;
            goto ByeBye;
        }
    }

    if (PagingIo || NoCache || IsVolume)
    {
        if (ByteOffset.u.LowPart % BytesPerSector != 0 || Length % BytesPerSector != 0)
        {
            // non cached write must be sector aligned
            Status = STATUS_INVALID_PARAMETER;
            goto ByeBye;
        }
    }

    OldFileSize = Fcb->RFCB.FileSize;

    if (Length == 0)
    {
        /* Update last write time */
        IrpContext->Irp->IoStatus.Information = 0;
        Status = STATUS_SUCCESS;
        goto Metadata;
    }

    if (PagingIo)
    {
        if (ByteOffset.u.LowPart + Length > Fcb->RFCB.AllocationSize.u.LowPart)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto ByeBye;
        }

        if (ByteOffset.u.LowPart + Length > ROUND_UP(Fcb->RFCB.AllocationSize.u.LowPart, BytesPerSector))
        {
            Length =  ROUND_UP(Fcb->RFCB.FileSize.u.LowPart, BytesPerSector) - ByteOffset.u.LowPart;
        }
    }

    if (!NoCache && !CcCanIWrite(IrpContext->FileObject, Length, CanWait,
                                 BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_DEFERRED_WRITE)))
    {
        BOOLEAN Retrying;

        Retrying = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_DEFERRED_WRITE);
        SetFlag(IrpContext->Flags, IRPCONTEXT_DEFERRED_WRITE);

        IoMarkIrpPending(IrpContext->Irp);
        Status = STATUS_PENDING;

        DPRINT1("Deferring write for Irp %p, context %p!\n", IrpContext->Irp, IrpContext);
        CcDeferWrite(IrpContext->FileObject, VfatHandleDeferredWrite,
                     IrpContext, NULL, Length, Retrying);
        *pIrpContext = NULL;

        return Status;
    }

    if (IsVolume)
    {
        Resource = &IrpContext->DeviceExt->DirResource;
    }
    else if (PagingIo)
    {
        Resource = &Fcb->PagingIoResource;
    }
    else
    {
        Resource = &Fcb->MainResource;
    }

    if (PagingIo)
    {
        if (!ExAcquireResourceSharedLite(Resource, CanWait))
        {
            Resource = NULL;
            Status = STATUS_PENDING;
            goto ByeBye;
        }
    }
    else
    {
        if (!ExAcquireResourceExclusiveLite(Resource, CanWait))
        {
            Resource = NULL;
            Status = STATUS_PENDING;
            goto ByeBye;
        }
    }

    if (!PagingIo &&
        FsRtlAreThereCurrentFileLocks(&Fcb->FileLock))
    {
        if (!FsRtlCheckLockForWriteAccess(&Fcb->FileLock, IrpContext->Irp))
        {
            Status = STATUS_FILE_LOCK_CONFLICT;
            goto ByeBye;
        }
    }

    if (!CanWait && !IsVolume)
    {
        if (ByteOffset.u.LowPart + Length > Fcb->RFCB.AllocationSize.u.LowPart)
        {
            Status = STATUS_PENDING;
            goto ByeBye;
        }
    }

    Buffer = VfatGetUserBuffer(IrpContext->Irp, PagingIo);

    if (!IsFAT && !IsVolume && !PagingIo &&
        ByteOffset.u.LowPart + Length > Fcb->RFCB.FileSize.u.LowPart)
    {
        LARGE_INTEGER AllocationSize;

        if (!ExAcquireResourceExclusiveLite(&IrpContext->DeviceExt->DirResource, CanWait))
        {
            Status = STATUS_PENDING;
            goto ByeBye;
        }

        AllocationSize.QuadPart = ByteOffset.u.LowPart + Length;
        Status = VfatSetAllocationSizeInformation(IrpContext->FileObject, Fcb,
                                                  IrpContext->DeviceExt, &AllocationSize);

        ExReleaseResourceLite(&IrpContext->DeviceExt->DirResource);

        if (!NT_SUCCESS (Status))
        {
            goto ByeBye;
        }
    }

    if (!NoCache && !PagingIo && !IsVolume)
    {
        // cached write

        vfatAddToStat(IrpContext->DeviceExt, Base.UserFileWrites, 1);
        vfatAddToStat(IrpContext->DeviceExt, Base.UserFileWriteBytes, Length);

        _SEH2_TRY
        {
            if (IrpContext->FileObject->PrivateCacheMap == NULL)
            {
                CcInitializeCacheMap(IrpContext->FileObject,
                                     (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
                                     FALSE,
                                     &VfatGlobalData->CacheMgrCallbacks,
                                     Fcb);
            }

            if (ByteOffset.QuadPart > OldFileSize.QuadPart)
            {
                CcZeroData(IrpContext->FileObject, &OldFileSize, &ByteOffset, TRUE);
            }

            if (CcCopyWrite(IrpContext->FileObject,
                            &ByteOffset,
                            Length,
                            TRUE /*CanWait*/,
                            Buffer))
            {
                IrpContext->Irp->IoStatus.Information = Length;
                Status = STATUS_SUCCESS;
            }
            else
            {
                ASSERT(FALSE /*!CanWait*/);
                Status = STATUS_UNSUCCESSFUL;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        // non cached write
        Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoReadAccess);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_USER_BUFFER;
            goto ByeBye;
        }

        if (ByteOffset.QuadPart > OldFileSize.QuadPart)
        {
            CcZeroData(IrpContext->FileObject, &OldFileSize, &ByteOffset, TRUE);
        }

        if (!IsVolume)
        {
            vfatAddToStat(IrpContext->DeviceExt, Fat.NonCachedWrites, 1);
            vfatAddToStat(IrpContext->DeviceExt, Fat.NonCachedWriteBytes, Length);
        }
        else
        {
            vfatAddToStat(IrpContext->DeviceExt, Base.MetaDataWrites, 1);
            vfatAddToStat(IrpContext->DeviceExt, Base.MetaDataWriteBytes, Length);
        }

        Status = VfatWriteFileData(IrpContext, Length, ByteOffset);
        if (NT_SUCCESS(Status))
        {
            IrpContext->Irp->IoStatus.Information = Length;
        }
    }

Metadata:
    if (!PagingIo && !IsFAT && !IsVolume)
    {
        if(!vfatFCBIsDirectory(Fcb))
        {
            LARGE_INTEGER SystemTime;
            ULONG Filter;

            // set dates and times
            KeQuerySystemTime (&SystemTime);
            if (vfatVolumeIsFatX(IrpContext->DeviceExt))
            {
                FsdSystemTimeToDosDateTime(IrpContext->DeviceExt,
                                           &SystemTime, &Fcb->entry.FatX.UpdateDate,
                                           &Fcb->entry.FatX.UpdateTime);
                Fcb->entry.FatX.AccessDate = Fcb->entry.FatX.UpdateDate;
                Fcb->entry.FatX.AccessTime = Fcb->entry.FatX.UpdateTime;
            }
            else
            {
                FsdSystemTimeToDosDateTime(IrpContext->DeviceExt,
                                           &SystemTime, &Fcb->entry.Fat.UpdateDate,
                                           &Fcb->entry.Fat.UpdateTime);
                Fcb->entry.Fat.AccessDate = Fcb->entry.Fat.UpdateDate;
            }
            /* set date and times to dirty */
            Fcb->Flags |= FCB_IS_DIRTY;

            /* Time to notify the OS */
            Filter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES;
            if (ByteOffset.QuadPart != OldFileSize.QuadPart) Filter |= FILE_NOTIFY_CHANGE_SIZE;

            vfatReportChange(IrpContext->DeviceExt, Fcb, Filter, FILE_ACTION_MODIFIED);
        }
    }

ByeBye:
    if (Resource)
    {
        ExReleaseResourceLite(Resource);
    }

    if (Status == STATUS_PENDING)
    {
        Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoReadAccess);
        if (NT_SUCCESS(Status))
        {
            Status = VfatMarkIrpContextForQueue(IrpContext);
        }
    }
    else
    {
        IrpContext->Irp->IoStatus.Status = Status;
        if (BooleanFlagOn(IrpContext->FileObject->Flags, FO_SYNCHRONOUS_IO) &&
            !PagingIo && NT_SUCCESS(Status))
        {
            IrpContext->FileObject->CurrentByteOffset.QuadPart =
                ByteOffset.QuadPart + IrpContext->Irp->IoStatus.Information;
        }

        if (NT_SUCCESS(Status))
            IrpContext->PriorityBoost = IO_DISK_INCREMENT;
    }
    DPRINT("%x\n", Status);
    return Status;
}
