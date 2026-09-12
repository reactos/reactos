/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/blockdev.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Trevor Thompson
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/* The run list uses storage embedded in the caller's stack frame and only
 * spills into pool for heavily fragmented files. */

VOID
NtfsInitIoRunList(OUT PNTFS_IO_RUN_LIST RunList)
{
    RunList->Runs = RunList->StackRuns;
    RunList->Capacity = NTFS_MAX_IO_RUNS_ON_STACK;
    RunList->Count = 0;
    RunList->TotalLength = 0;
}

/**
* @name NtfsAddIoRun
* @implemented
*
* Appends one contiguous piece to a request's run list.
*
* @param RunList
* The list being built, previously initialized by NtfsInitIoRunList()
*
* @param Lbo
* Byte offset on the volume of this piece, or NTFS_SPARSE_LBO if the piece has
* no backing storage
*
* @param ByteCount
* Size of this piece, in bytes
*
* @return
* STATUS_SUCCESS, or STATUS_INSUFFICIENT_RESOURCES if the list had to grow and
* the allocation failed.
*
* @remarks Runs tile the caller's buffer in the order they are added, so the
* buffer offset is implied. A piece continuing the previous one is merged into
* it, keeping contiguous files down to a single IRP.
*
*/
NTSTATUS
NtfsAddIoRun(IN OUT PNTFS_IO_RUN_LIST RunList,
             IN LONGLONG Lbo,
             IN ULONG ByteCount)
{
    PNTFS_IO_RUN LastRun;
    PNTFS_IO_RUN NewRuns;
    ULONG NewCapacity;

    if (ByteCount == 0)
        return STATUS_SUCCESS;

    /* Does this piece simply continue the previous one? */
    if (RunList->Count != 0)
    {
        LastRun = &RunList->Runs[RunList->Count - 1];

        if (LastRun->ByteCount <= MAXULONG - ByteCount &&
            ((Lbo == NTFS_SPARSE_LBO && LastRun->Lbo == NTFS_SPARSE_LBO) ||
             (Lbo != NTFS_SPARSE_LBO && LastRun->Lbo != NTFS_SPARSE_LBO &&
              LastRun->Lbo + LastRun->ByteCount == Lbo)))
        {
            LastRun->ByteCount += ByteCount;
            RunList->TotalLength += ByteCount;
            return STATUS_SUCCESS;
        }
    }

    if (RunList->Count == RunList->Capacity)
    {
        NewCapacity = RunList->Capacity * 2;

        /* Non-paged: a run list stays live across the transfer, and a request
         * on the paging path must not fault while walking it. */
        NewRuns = ExAllocatePoolWithTag(NonPagedPool,
                                        NewCapacity * sizeof(NTFS_IO_RUN),
                                        TAG_IO_RUNS);
        if (NewRuns == NULL)
        {
            DPRINT1("Not enough memory to grow the I/O run list!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(NewRuns, RunList->Runs, RunList->Count * sizeof(NTFS_IO_RUN));

        if (RunList->Runs != RunList->StackRuns)
        {
            ExFreePoolWithTag(RunList->Runs, TAG_IO_RUNS);
        }

        RunList->Runs = NewRuns;
        RunList->Capacity = NewCapacity;
    }

    LastRun = &RunList->Runs[RunList->Count];
    LastRun->Lbo = Lbo;
    LastRun->BufferOffset = RunList->TotalLength;
    LastRun->ByteCount = ByteCount;
    LastRun->SavedIrp = NULL;

    RunList->Count++;
    RunList->TotalLength += ByteCount;

    return STATUS_SUCCESS;
}

VOID
NtfsFreeIoRunList(IN OUT PNTFS_IO_RUN_LIST RunList)
{
    if (RunList->Runs != RunList->StackRuns && RunList->Runs != NULL)
    {
        ExFreePoolWithTag(RunList->Runs, TAG_IO_RUNS);
    }

    RunList->Runs = NULL;
    RunList->Count = 0;
    RunList->Capacity = 0;
    RunList->TotalLength = 0;
}

/*
 * Completion routine shared by every run of a request. Frees the IRP and its
 * partial MDL, which are ours, and returns STATUS_MORE_PROCESSING_REQUIRED so
 * the I/O manager stops touching them. The last run to complete wakes the
 * issuer. Called at up to DISPATCH_LEVEL, in an arbitrary thread.
 */
static
NTSTATUS
NTAPI
NtfsIoRunCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp,
                           IN PVOID Context)
{
    PNTFS_IO_CONTEXT IoContext = (PNTFS_IO_CONTEXT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (NT_SUCCESS(Irp->IoStatus.Status))
    {
        InterlockedExchangeAdd(&IoContext->BytesTransferred,
                               (LONG)Irp->IoStatus.Information);
    }
    else
    {
        /* First error wins; STATUS_SUCCESS is the "nothing failed yet" sentinel. */
        InterlockedCompareExchange(&IoContext->Status,
                                   Irp->IoStatus.Status,
                                   STATUS_SUCCESS);
    }

    /* The master MDL belongs to the issuer and outlives every run built from it. */
    if (Irp->MdlAddress != NULL && Irp->MdlAddress != IoContext->MasterMdl)
    {
        IoFreeMdl(Irp->MdlAddress);
    }

    IoFreeIrp(Irp);

    if (InterlockedDecrement(&IoContext->IrpCount) == 0)
    {
        KeSetEvent(&IoContext->SyncEvent, IO_NO_INCREMENT, FALSE);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* Builds, but does not issue, the IRP for a single run. Every IRP is built
 * before any is issued, so a failure part-way through unwinds with no I/O in
 * flight. */
static
NTSTATUS
NtfsBuildIoRunIrp(IN PDEVICE_OBJECT DeviceObject,
                  IN PNTFS_IO_CONTEXT IoContext,
                  IN UCHAR MajorFunction,
                  IN PUCHAR Buffer,
                  IN ULONG BufferLength,
                  IN OUT PNTFS_IO_RUN Run,
                  IN BOOLEAN Override)
{
    PIO_STACK_LOCATION Stack;
    PIRP Irp;
    PMDL Mdl;

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (Irp == NULL)
    {
        DPRINT1("IoAllocateIrp failed!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Run->BufferOffset == 0 && Run->ByteCount == BufferLength)
    {
        Mdl = IoContext->MasterMdl;
    }
    else
    {
        PUCHAR MasterVa = (PUCHAR)IoContext->MasterVa + Run->BufferOffset;

        Mdl = IoAllocateMdl(MasterVa,
                            Run->ByteCount,
                            FALSE,
                            FALSE,
                            NULL);
        if (Mdl == NULL)
        {
            DPRINT1("IoAllocateMdl failed!\n");
            IoFreeIrp(Irp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoBuildPartialMdl(IoContext->MasterMdl,
                          Mdl,
                          MasterVa,
                          Run->ByteCount);
    }

    Irp->MdlAddress = Mdl;
    Irp->Flags |= IRP_NOCACHE;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = MajorFunction;
    Stack->Parameters.Read.Length = Run->ByteCount;
    Stack->Parameters.Read.ByteOffset.QuadPart = Run->Lbo;

    if (Override)
    {
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    IoSetCompletionRoutine(Irp,
                           NtfsIoRunCompletionRoutine,
                           IoContext,
                           TRUE,
                           TRUE,
                           TRUE);

    Run->SavedIrp = Irp;

    return STATUS_SUCCESS;
}

static
NTSTATUS
NtfsIssueIoRuns(IN PDEVICE_OBJECT DeviceObject,
                IN UCHAR MajorFunction,
                IN OUT PUCHAR Buffer,
                IN PMDL BorrowedMdl OPTIONAL,
                IN PNTFS_IO_RUN_LIST RunList,
                IN BOOLEAN Override,
                OUT PULONG BytesTransferred)
{
    NTFS_IO_CONTEXT IoContext;
    NTSTATUS Status = STATUS_SUCCESS;
    PNTFS_IO_RUN Run;
    ULONG DiskRunCount = 0;
    ULONG Index;
    ULONG BuiltCount = 0;
    PIRP Irp;

    *BytesTransferred = 0;

    RtlZeroMemory(&IoContext, sizeof(IoContext));
    KeInitializeEvent(&IoContext.SyncEvent, NotificationEvent, FALSE);
    IoContext.Status = STATUS_SUCCESS;

    /* Satisfy the sparse runs, and see if any real I/O is left */
    for (Index = 0; Index < RunList->Count; Index++)
    {
        Run = &RunList->Runs[Index];

        if (Run->Lbo != NTFS_SPARSE_LBO)
        {
            DiskRunCount++;
            continue;
        }

        if (MajorFunction != IRP_MJ_READ)
        {
            DPRINT1("FIXME: Writing to sparse files is not supported yet!\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        RtlZeroMemory(Buffer + Run->BufferOffset, Run->ByteCount);
        *BytesTransferred += Run->ByteCount;
    }

    if (DiskRunCount == 0)
    {
        return STATUS_SUCCESS;
    }

    if (BorrowedMdl != NULL)
    {
        IoContext.MasterMdl = BorrowedMdl;
        IoContext.MasterVa = MmGetMdlVirtualAddress(BorrowedMdl);
        IoContext.OwnsMasterMdl = FALSE;
    }
    else
    {
        /* Our own buffer: describe and lock it once */
        IoContext.MasterMdl = IoAllocateMdl(Buffer, RunList->TotalLength, FALSE, FALSE, NULL);
        if (IoContext.MasterMdl == NULL)
        {
            DPRINT1("IoAllocateMdl failed!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        _SEH2_TRY
        {
            MmProbeAndLockPages(IoContext.MasterMdl,
                                KernelMode,
                                (MajorFunction == IRP_MJ_READ) ? IoWriteAccess : IoReadAccess);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("MmProbeAndLockPages failed (Status %lx)\n", Status);
            IoFreeMdl(IoContext.MasterMdl);
            return Status;
        }

        IoContext.MasterVa = Buffer;
        IoContext.OwnsMasterMdl = TRUE;
    }

    /* Build everything before issuing anything */
    for (Index = 0; Index < RunList->Count; Index++)
    {
        Run = &RunList->Runs[Index];

        if (Run->Lbo == NTFS_SPARSE_LBO)
            continue;

        Status = NtfsBuildIoRunIrp(DeviceObject,
                                   &IoContext,
                                   MajorFunction,
                                   Buffer,
                                   RunList->TotalLength,
                                   Run,
                                   Override);
        if (!NT_SUCCESS(Status))
            break;

        BuiltCount++;
    }

    if (!NT_SUCCESS(Status))
    {
        for (Index = 0; Index < RunList->Count; Index++)
        {
            Run = &RunList->Runs[Index];

            if (Run->SavedIrp == NULL)
                continue;

            if (Run->SavedIrp->MdlAddress != NULL &&
                Run->SavedIrp->MdlAddress != IoContext.MasterMdl)
            {
                IoFreeMdl(Run->SavedIrp->MdlAddress);
            }

            IoFreeIrp(Run->SavedIrp);
            Run->SavedIrp = NULL;
        }

        if (IoContext.OwnsMasterMdl)
        {
            MmUnlockPages(IoContext.MasterMdl);
            IoFreeMdl(IoContext.MasterMdl);
        }

        return Status;
    }

    /* Publish the count before the first completion routine can run. */
    IoContext.IrpCount = BuiltCount;

    for (Index = 0; Index < RunList->Count; Index++)
    {
        Run = &RunList->Runs[Index];

        if (Run->SavedIrp == NULL)
            continue;

        /* The completion routine frees the IRP, so drop our pointer first */
        Irp = Run->SavedIrp;
        Run->SavedIrp = NULL;

        /* A failure here is completed by the lower driver and picked up by
         * our completion routine like any other I/O error */
        (VOID)IoCallDriver(DeviceObject, Irp);
    }

    KeWaitForSingleObject(&IoContext.SyncEvent, Executive, KernelMode, FALSE, NULL);

    if (IoContext.OwnsMasterMdl)
    {
        MmUnlockPages(IoContext.MasterMdl);
        IoFreeMdl(IoContext.MasterMdl);
    }

    Status = (NTSTATUS)IoContext.Status;
    if (NT_SUCCESS(Status))
    {
        *BytesTransferred += (ULONG)IoContext.BytesTransferred;
    }

    return Status;
}

/**
* @name NtfsPerformIoRuns
* @implemented
*
* Reads or writes the pieces described by RunList, all in parallel.
*
* @param DeviceObject
* Storage device to transfer to or from
*
* @param MajorFunction
* IRP_MJ_READ or IRP_MJ_WRITE
*
* @param SectorSize
* Sector size the storage device requires transfers to be aligned to
*
* @param Buffer
* System-space buffer of RunList->TotalLength bytes, tiled by the runs
*
* @param RunList
* The pieces to transfer. Consumed by this call: the runs are rewritten in
* place to satisfy the device's alignment requirements.
*
* @param Override
* Whether to set SL_OVERRIDE_VERIFY_VOLUME on the requests
*
* @param BytesTransferred
* Optionally receives how much of the request was satisfied
*
* @return
* STATUS_SUCCESS on success, STATUS_INSUFFICIENT_RESOURCES if an allocation
* failed, STATUS_NOT_IMPLEMENTED for a write to a sparse run, since that's
* probably not happening for this driver
*/
static
NTSTATUS
NtfsPerformIoRunsInternal(IN PDEVICE_OBJECT DeviceObject,
                          IN UCHAR MajorFunction,
                          IN ULONG SectorSize,
                          IN OUT PUCHAR Buffer,
                          IN PMDL BorrowedMdl OPTIONAL,
                          IN PNTFS_IO_RUN_LIST RunList,
                          IN BOOLEAN Override,
                          OUT PULONG BytesTransferred OPTIONAL)
{
    NTSTATUS Status;
    PNTFS_IO_RUN FirstRun;
    PNTFS_IO_RUN LastRun;
    PUCHAR BounceBuffer;
    LONGLONG EndLbo;
    ULONG FrontPad = 0;
    ULONG BackPad = 0;
    ULONG BounceLength;
    ULONG Transferred = 0;
    ULONG Index;
    ULONG Length;

    if (BytesTransferred != NULL)
        *BytesTransferred = 0;

    if (RunList->Count == 0 || RunList->TotalLength == 0)
        return STATUS_SUCCESS;

    ASSERT(SectorSize != 0);

    Length = RunList->TotalLength;
    FirstRun = &RunList->Runs[0];
    LastRun = &RunList->Runs[RunList->Count - 1];

    /* Only the first run can start unaligned and only the last can end
     * unaligned; the rest sit on cluster boundaries */
    if (FirstRun->Lbo != NTFS_SPARSE_LBO)
    {
        FrontPad = (ULONG)(FirstRun->Lbo % SectorSize);
    }

    if (LastRun->Lbo != NTFS_SPARSE_LBO)
    {
        EndLbo = LastRun->Lbo + LastRun->ByteCount;
        BackPad = (ULONG)(ROUND_UP(EndLbo, (LONGLONG)SectorSize) - EndLbo);
    }

    if (FrontPad == 0 && BackPad == 0)
    {
        Status = NtfsIssueIoRuns(DeviceObject,
                                 MajorFunction,
                                 Buffer,
                                 BorrowedMdl,
                                 RunList,
                                 Override,
                                 &Transferred);

        if (BytesTransferred != NULL)
            *BytesTransferred = min(Transferred, Length);

        return Status;
    }

    BounceLength = FrontPad + Length + BackPad;

    BounceBuffer = ExAllocatePoolWithTag(NonPagedPool, BounceLength, TAG_NTFS);
    if (BounceBuffer == NULL)
    {
        DPRINT1("Not enough memory for an unaligned transfer!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Grow the request out to whole sectors; everything after the first run
     * slides along by what we prepended */
    for (Index = 1; Index < RunList->Count; Index++)
    {
        RunList->Runs[Index].BufferOffset += FrontPad;
    }

    FirstRun->BufferOffset = 0;
    FirstRun->ByteCount += FrontPad;
    if (FirstRun->Lbo != NTFS_SPARSE_LBO)
        FirstRun->Lbo -= FrontPad;

    LastRun->ByteCount += BackPad;
    RunList->TotalLength = BounceLength;

    Status = STATUS_SUCCESS;

    if (MajorFunction != IRP_MJ_READ)
    {
        if (FrontPad != 0)
        {
            Status = NtfsReadDisk(DeviceObject,
                                  FirstRun->Lbo,
                                  SectorSize,
                                  SectorSize,
                                  BounceBuffer,
                                  Override);
        }

        /* Skip only if the read above already fetched this same sector */
        if (NT_SUCCESS(Status) &&
            BackPad != 0 &&
            (FrontPad == 0 || BounceLength > SectorSize))
        {
            Status = NtfsReadDisk(DeviceObject,
                                  LastRun->Lbo + LastRun->ByteCount - SectorSize,
                                  SectorSize,
                                  SectorSize,
                                  BounceBuffer + BounceLength - SectorSize,
                                  Override);
        }

        if (NT_SUCCESS(Status))
        {
            RtlCopyMemory(BounceBuffer + FrontPad, Buffer, Length);
        }
    }

    if (NT_SUCCESS(Status))
    {
        /* The bounce buffer is ours, so there is no MDL to borrow here. */
        Status = NtfsIssueIoRuns(DeviceObject,
                                 MajorFunction,
                                 BounceBuffer,
                                 NULL,
                                 RunList,
                                 Override,
                                 &Transferred);

        if (NT_SUCCESS(Status))
        {
            if (MajorFunction == IRP_MJ_READ)
            {
                RtlCopyMemory(Buffer, BounceBuffer + FrontPad, Length);
            }

            if (BytesTransferred != NULL)
            {
                /* The padding is ours, not the caller's. */
                Transferred = (Transferred > FrontPad) ? Transferred - FrontPad : 0;
                *BytesTransferred = min(Transferred, Length);
            }
        }
    }

    /* The bounce buffer can hold user data; do not leave it in pool. */
    RtlSecureZeroMemory(BounceBuffer, BounceLength);
    ExFreePoolWithTag(BounceBuffer, TAG_NTFS);

    return Status;
}

/* Completion routine for the caller's own IRP, which we must not free.
 * STATUS_MORE_PROCESSING_REQUIRED keeps ownership with us so the dispatch path
 * can complete it as usual. */
static
NTSTATUS
NTAPI
NtfsForwardedIrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp,
                                  IN PVOID Context)
{
    PNTFS_IO_CONTEXT IoContext = (PNTFS_IO_CONTEXT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    IoContext->Status = Irp->IoStatus.Status;
    IoContext->BytesTransferred = (LONG)Irp->IoStatus.Information;

    KeSetEvent(&IoContext->SyncEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* Hands the caller's IRP straight to the storage stack for one contiguous
 * run: no allocation, no copy, the disk transfers directly into the pages the
 * caller's MDL describes. Same as fastfat's FatSingleAsync(). */
static
NTSTATUS
NtfsForwardIrpToRun(IN PDEVICE_OBJECT DeviceObject,
                    IN UCHAR MajorFunction,
                    IN PIRP Irp,
                    IN PNTFS_IO_RUN Run,
                    IN BOOLEAN Override,
                    OUT PULONG BytesTransferred)
{
    NTFS_IO_CONTEXT IoContext;
    PIO_STACK_LOCATION Stack;

    DPRINT("Forwarding IRP %p for a single run at %I64x, %lu bytes\n",
           Irp, Run->Lbo, Run->ByteCount);

    RtlZeroMemory(&IoContext, sizeof(IoContext));
    KeInitializeEvent(&IoContext.SyncEvent, NotificationEvent, FALSE);
    IoContext.Status = STATUS_SUCCESS;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = MajorFunction;
    Stack->MinorFunction = 0;
    Stack->FileObject = NULL;
    Stack->Parameters.Read.Length = Run->ByteCount;
    Stack->Parameters.Read.ByteOffset.QuadPart = Run->Lbo;
    Stack->Flags = Override ? SL_OVERRIDE_VERIFY_VOLUME : 0;

    /* Must come last: it takes over Control on this same stack location. */
    IoSetCompletionRoutine(Irp,
                           NtfsForwardedIrpCompletionRoutine,
                           &IoContext,
                           TRUE,
                           TRUE,
                           TRUE);

    (VOID)IoCallDriver(DeviceObject, Irp);

    KeWaitForSingleObject(&IoContext.SyncEvent, Executive, KernelMode, FALSE, NULL);

    *BytesTransferred = (ULONG)IoContext.BytesTransferred;

    return (NTSTATUS)IoContext.Status;
}

/**
* @name NtfsPerformIrpIoRuns
* @implemented
*
* Transfers the runs of a request that originated from an IRP, forwarding that
* IRP straight to the storage stack when the request allows it.
*
* @param DeviceObject
* Storage device to transfer to or from
*
* @param MajorFunction
* IRP_MJ_READ or IRP_MJ_WRITE
*
* @param SectorSize
* Sector size the storage device requires transfers to be aligned to
*
* @param Irp
* The request being served. May be NULL, in which case this is exactly
* NtfsPerformIoRuns().
*
* @param Buffer
* System-space mapping of the IRP's buffer, used whenever the request cannot
* simply be forwarded
*
* @param RunList
* The pieces to transfer
*
* @param Override
* Whether to set SL_OVERRIDE_VERIFY_VOLUME on the requests
*
* @param BytesTransferred
* Receives how much of the request was satisfied
*
* @return
* STATUS_SUCCESS on success, otherwise the status of the failing transfer.
*
*/
NTSTATUS
NtfsPerformIrpIoRuns(IN PDEVICE_OBJECT DeviceObject,
                     IN UCHAR MajorFunction,
                     IN ULONG SectorSize,
                     IN PIRP Irp,
                     IN OUT PUCHAR Buffer,
                     IN PNTFS_IO_RUN_LIST RunList,
                     IN BOOLEAN Override,
                     OUT PULONG BytesTransferred)
{
    PMDL BorrowedMdl = NULL;

    if (Irp != NULL &&
        Irp->MdlAddress != NULL &&
        RunList->TotalLength <= MmGetMdlByteCount(Irp->MdlAddress))
    {
        /* Buffer maps exactly the pages this MDL describes, so it can stand
         * in for one of our own, and on the paging path it must */
        BorrowedMdl = Irp->MdlAddress;

        /* One aligned run covering the lot goes straight down to the disk */
        if (RunList->Count == 1 &&
            RunList->Runs[0].Lbo != NTFS_SPARSE_LBO &&
            (RunList->Runs[0].Lbo % SectorSize) == 0 &&
            (RunList->Runs[0].ByteCount % SectorSize) == 0)
        {
            return NtfsForwardIrpToRun(DeviceObject,
                                       MajorFunction,
                                       Irp,
                                       &RunList->Runs[0],
                                       Override,
                                       BytesTransferred);
        }
    }

    return NtfsPerformIoRunsInternal(DeviceObject,
                                     MajorFunction,
                                     SectorSize,
                                     Buffer,
                                     BorrowedMdl,
                                     RunList,
                                     Override,
                                     BytesTransferred);
}

NTSTATUS
NtfsPerformIoRuns(IN PDEVICE_OBJECT DeviceObject,
                  IN UCHAR MajorFunction,
                  IN ULONG SectorSize,
                  IN OUT PUCHAR Buffer,
                  IN PNTFS_IO_RUN_LIST RunList,
                  IN BOOLEAN Override,
                  OUT PULONG BytesTransferred OPTIONAL)
{
    return NtfsPerformIoRunsInternal(DeviceObject,
                                     MajorFunction,
                                     SectorSize,
                                     Buffer,
                                     NULL,
                                     RunList,
                                     Override,
                                     BytesTransferred);
}

/**
* @name NtfsReadDisk
* @implemented
*
* Reads a single contiguous range from the given DeviceObject.
*
* @param DeviceObject
* Device to read from
*
* @param StartingOffset
* Offset, in bytes, from the start of the device object
*
* @param Length
* How much data to read, in bytes
*
* @param SectorSize
* Size of the sector on the disk that the read must be aligned to
*
* @param Buffer
* System-space buffer receiving the data
*
* @param Override
* Whether to set SL_OVERRIDE_VERIFY_VOLUME on the request
*
* @return
* STATUS_SUCCESS in case of success, STATUS_INSUFFICIENT_RESOURCES if a memory
* allocation failed, or whatever status the storage stack returned.
*
*/
NTSTATUS
NtfsReadDisk(IN PDEVICE_OBJECT DeviceObject,
             IN LONGLONG StartingOffset,
             IN ULONG Length,
             IN ULONG SectorSize,
             IN OUT PUCHAR Buffer,
             IN BOOLEAN Override)
{
    NTFS_IO_RUN_LIST RunList;
    NTSTATUS Status;
    ULONG Transferred;

    DPRINT("NtfsReadDisk(%p, %I64x, %lu, %lu, %p, %d)\n",
           DeviceObject, StartingOffset, Length, SectorSize, Buffer, Override);

    if (Length == 0)
        return STATUS_SUCCESS;

    NtfsInitIoRunList(&RunList);

    Status = NtfsAddIoRun(&RunList, StartingOffset, Length);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtfsPerformIoRuns(DeviceObject,
                               IRP_MJ_READ,
                               SectorSize,
                               Buffer,
                               &RunList,
                               Override,
                               &Transferred);

    NtfsFreeIoRunList(&RunList);

    if (NT_SUCCESS(Status) && Transferred != Length)
    {
        DPRINT1("Short read: asked for %lu bytes, got %lu\n", Length, Transferred);
        Status = STATUS_UNEXPECTED_IO_ERROR;
    }

    DPRINT("NtfsReadDisk() done (Status %x)\n", Status);

    return Status;
}

/**
* @name NtfsWriteDisk
* @implemented
*
* Writes data from the given buffer to the given DeviceObject.
*
* @param DeviceObject
* Device to write to
*
* @param StartingOffset
* Offset, in bytes, from the start of the device object where the data will be written
*
* @param Length
* How much data will be written, in bytes
*
* @param SectorSize
* Size of the sector on the disk that the write must be aligned to
*
* @param Buffer
* The data that's being written to the device
*
* @return
* STATUS_SUCCESS in case of success, STATUS_INSUFFICIENT_RESOURCES if a memory
* allocation failed, or whatever status the storage stack returned.
*
*/
NTSTATUS
NtfsWriteDisk(IN PDEVICE_OBJECT DeviceObject,
              IN LONGLONG StartingOffset,
              IN ULONG Length,
              IN ULONG SectorSize,
              IN const PUCHAR Buffer)
{
    NTFS_IO_RUN_LIST RunList;
    NTSTATUS Status;
    ULONG Transferred;

    DPRINT("NtfsWriteDisk(%p, %I64x, %lu, %lu, %p)\n",
           DeviceObject, StartingOffset, Length, SectorSize, Buffer);

    if (Length == 0)
        return STATUS_SUCCESS;

    NtfsInitIoRunList(&RunList);

    Status = NtfsAddIoRun(&RunList, StartingOffset, Length);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtfsPerformIoRuns(DeviceObject,
                               IRP_MJ_WRITE,
                               SectorSize,
                               Buffer,
                               &RunList,
                               FALSE,
                               &Transferred);

    NtfsFreeIoRunList(&RunList);

    if (NT_SUCCESS(Status) && Transferred != Length)
    {
        DPRINT1("Short write: asked for %lu bytes, wrote %lu\n", Length, Transferred);
        Status = STATUS_UNEXPECTED_IO_ERROR;
    }

    DPRINT("NtfsWriteDisk() done (Status %x)\n", Status);

    return Status;
}

/**
* @name NtfsFlushDevice
* @implemented
*
* Asks the storage stack to push everything it is holding out to the media.
*
* @param DeviceObject
* Device to flush
*
* @return
* STATUS_SUCCESS on success, or whatever the storage stack returned.
*
*/
NTSTATUS
NtfsFlushDevice(IN PDEVICE_OBJECT DeviceObject)
{
    NTFS_IO_CONTEXT IoContext;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;
    PIRP Irp;

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (Irp == NULL)
    {
        DPRINT1("IoAllocateIrp failed!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(&IoContext, sizeof(IoContext));
    KeInitializeEvent(&IoContext.SyncEvent, NotificationEvent, FALSE);
    IoContext.Status = STATUS_SUCCESS;
    IoContext.IrpCount = 1;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_FLUSH_BUFFERS;

    IoSetCompletionRoutine(Irp,
                           NtfsIoRunCompletionRoutine,
                           &IoContext,
                           TRUE,
                           TRUE,
                           TRUE);

    (VOID)IoCallDriver(DeviceObject, Irp);

    KeWaitForSingleObject(&IoContext.SyncEvent, Executive, KernelMode, FALSE, NULL);

    Status = (NTSTATUS)IoContext.Status;

    if (Status == STATUS_INVALID_DEVICE_REQUEST ||
        Status == STATUS_NOT_SUPPORTED)
    {
        Status = STATUS_SUCCESS;
    }

    DPRINT("NtfsFlushDevice() done (Status %lx)\n", Status);

    return Status;
}

NTSTATUS
NtfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
                IN ULONG DiskSector,
                IN ULONG SectorCount,
                IN ULONG SectorSize,
                IN OUT PUCHAR Buffer,
                IN BOOLEAN Override)
{
    LONGLONG Offset;
    ULONG BlockSize;

    Offset = (LONGLONG)DiskSector * (LONGLONG)SectorSize;
    BlockSize = SectorCount * SectorSize;

    return NtfsReadDisk(DeviceObject, Offset, BlockSize, SectorSize, Buffer, Override);
}


NTSTATUS
NtfsDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
                    IN ULONG ControlCode,
                    IN PVOID InputBuffer,
                    IN ULONG InputBufferSize,
                    IN OUT PVOID OutputBuffer,
                    IN OUT PULONG OutputBufferSize,
                    IN BOOLEAN Override)
{
    PIO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoStatus.Status = STATUS_SUCCESS;
    IoStatus.Information = 0;

    DPRINT("Building device I/O control request ...\n");
    Irp = IoBuildDeviceIoControlRequest(ControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferSize,
                                        OutputBuffer,
                                        (OutputBufferSize) ? *OutputBufferSize : 0,
                                        FALSE,
                                        &Event,
                                        &IoStatus);
    if (Irp == NULL)
    {
        DPRINT("IoBuildDeviceIoControlRequest() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Override)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    DPRINT("Calling IO Driver... with irp %p\n", Irp);
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (OutputBufferSize)
    {
        *OutputBufferSize = IoStatus.Information;
    }

    return Status;
}

/* EOF */
