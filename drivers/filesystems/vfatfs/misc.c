/*
 * PROJECT:     VFAT Filesystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Misc routines
 * COPYRIGHT:   Copyright 1998 Jason Filby <jasonfilby@yahoo.com>
 *              Copyright 2015-2018 Pierre Schweitzer <pierre@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

const char* MajorFunctionNames[] =
{
    "IRP_MJ_CREATE",
    "IRP_MJ_CREATE_NAMED_PIPE",
    "IRP_MJ_CLOSE",
    "IRP_MJ_READ",
    "IRP_MJ_WRITE",
    "IRP_MJ_QUERY_INFORMATION",
    "IRP_MJ_SET_INFORMATION",
    "IRP_MJ_QUERY_EA",
    "IRP_MJ_SET_EA",
    "IRP_MJ_FLUSH_BUFFERS",
    "IRP_MJ_QUERY_VOLUME_INFORMATION",
    "IRP_MJ_SET_VOLUME_INFORMATION",
    "IRP_MJ_DIRECTORY_CONTROL",
    "IRP_MJ_FILE_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CONTROL",
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",
    "IRP_MJ_SHUTDOWN",
    "IRP_MJ_LOCK_CONTROL",
    "IRP_MJ_CLEANUP",
    "IRP_MJ_CREATE_MAILSLOT",
    "IRP_MJ_QUERY_SECURITY",
    "IRP_MJ_SET_SECURITY",
    "IRP_MJ_POWER",
    "IRP_MJ_SYSTEM_CONTROL",
    "IRP_MJ_DEVICE_CHANGE",
    "IRP_MJ_QUERY_QUOTA",
    "IRP_MJ_SET_QUOTA",
    "IRP_MJ_PNP",
    "IRP_MJ_MAXIMUM_FUNCTION"
};

static LONG QueueCount = 0;

static VOID VfatFreeIrpContext(PVFAT_IRP_CONTEXT);
static PVFAT_IRP_CONTEXT VfatAllocateIrpContext(PDEVICE_OBJECT, PIRP);
static NTSTATUS VfatQueueRequest(PVFAT_IRP_CONTEXT);

/* FUNCTIONS ****************************************************************/

static
NTSTATUS
VfatLockControl(
    IN PVFAT_IRP_CONTEXT IrpContext)
{
    PVFATFCB Fcb;
    NTSTATUS Status;

    DPRINT("VfatLockControl(IrpContext %p)\n", IrpContext);

    ASSERT(IrpContext);

    Fcb = (PVFATFCB)IrpContext->FileObject->FsContext;

    if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (vfatFCBIsDirectory(Fcb))
    {
        return STATUS_INVALID_PARAMETER;
    }

    IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;
    Status = FsRtlProcessFileLock(&Fcb->FileLock,
                                  IrpContext->Irp,
                                  NULL);
    return Status;
}

static
NTSTATUS
VfatDeviceControl(
    IN PVFAT_IRP_CONTEXT IrpContext)
{
    IoSkipCurrentIrpStackLocation(IrpContext->Irp);

    IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;

    return IoCallDriver(IrpContext->DeviceExt->StorageDevice, IrpContext->Irp);
}

static
NTSTATUS
VfatDispatchRequest(
    IN PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;
    BOOLEAN QueueIrp, CompleteIrp;

    DPRINT("VfatDispatchRequest (IrpContext %p), is called for %s\n", IrpContext,
           IrpContext->MajorFunction >= IRP_MJ_MAXIMUM_FUNCTION ? "????" : MajorFunctionNames[IrpContext->MajorFunction]);

    ASSERT(IrpContext);

    FsRtlEnterFileSystem();

    switch (IrpContext->MajorFunction)
    {
        case IRP_MJ_CLOSE:
            Status = VfatClose(IrpContext);
            break;

        case IRP_MJ_CREATE:
            Status = VfatCreate(IrpContext);
            break;

        case IRP_MJ_READ:
            Status = VfatRead(IrpContext);
            break;

        case IRP_MJ_WRITE:
            Status = VfatWrite(&IrpContext);
            break;

        case IRP_MJ_FILE_SYSTEM_CONTROL:
            Status = VfatFileSystemControl(IrpContext);
            break;

        case IRP_MJ_QUERY_INFORMATION:
            Status = VfatQueryInformation(IrpContext);
            break;

        case IRP_MJ_SET_INFORMATION:
            Status = VfatSetInformation(IrpContext);
            break;

        case IRP_MJ_DIRECTORY_CONTROL:
            Status = VfatDirectoryControl(IrpContext);
            break;

        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            Status = VfatQueryVolumeInformation(IrpContext);
            break;

        case IRP_MJ_SET_VOLUME_INFORMATION:
            Status = VfatSetVolumeInformation(IrpContext);
            break;

        case IRP_MJ_LOCK_CONTROL:
            Status = VfatLockControl(IrpContext);
            break;

        case IRP_MJ_DEVICE_CONTROL:
            Status = VfatDeviceControl(IrpContext);
            break;

        case IRP_MJ_CLEANUP:
            Status = VfatCleanup(IrpContext);
            break;

        case IRP_MJ_FLUSH_BUFFERS:
            Status = VfatFlush(IrpContext);
            break;

        case IRP_MJ_PNP:
            Status = VfatPnp(IrpContext);
            break;

        default:
            DPRINT1("Unexpected major function %x\n", IrpContext->MajorFunction);
            Status = STATUS_DRIVER_INTERNAL_ERROR;
    }

    if (IrpContext != NULL)
    {
        QueueIrp = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_QUEUE);
        CompleteIrp = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_COMPLETE);

        ASSERT((!CompleteIrp && !QueueIrp) ||
               (CompleteIrp && !QueueIrp) ||
               (!CompleteIrp && QueueIrp));

        if (CompleteIrp)
        {
            IrpContext->Irp->IoStatus.Status = Status;
            IoCompleteRequest(IrpContext->Irp, IrpContext->PriorityBoost);
        }

        if (QueueIrp)
        {
            /* Reset our status flags before queueing the IRP */
            IrpContext->Flags |= IRPCONTEXT_COMPLETE;
            IrpContext->Flags &= ~IRPCONTEXT_QUEUE;
            Status = VfatQueueRequest(IrpContext);
        }
        else
        {
            /* Unless the IRP was queued, always free the IRP context */
            VfatFreeIrpContext(IrpContext);
        }
    }

    FsRtlExitFileSystem();

    return Status;
}

VOID
NTAPI
VfatHandleDeferredWrite(
    IN PVOID IrpContext,
    IN PVOID Unused)
{
    VfatDispatchRequest((PVFAT_IRP_CONTEXT)IrpContext);
}

NTSTATUS
NTAPI
VfatBuildRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    PVFAT_IRP_CONTEXT IrpContext;

    DPRINT("VfatBuildRequest (DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    ASSERT(DeviceObject);
    ASSERT(Irp);

    IrpContext = VfatAllocateIrpContext(DeviceObject, Irp);
    if (IrpContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        Status = VfatDispatchRequest(IrpContext);
    }
    return Status;
}

static
VOID
VfatFreeIrpContext(
    PVFAT_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);
    ExFreeToNPagedLookasideList(&VfatGlobalData->IrpContextLookasideList, IrpContext);
}

static
PVFAT_IRP_CONTEXT
VfatAllocateIrpContext(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PVFAT_IRP_CONTEXT IrpContext;
    /*PIO_STACK_LOCATION Stack;*/
    UCHAR MajorFunction;

    DPRINT("VfatAllocateIrpContext(DeviceObject %p, Irp %p)\n", DeviceObject, Irp);

    ASSERT(DeviceObject);
    ASSERT(Irp);

    IrpContext = ExAllocateFromNPagedLookasideList(&VfatGlobalData->IrpContextLookasideList);
    if (IrpContext)
    {
        RtlZeroMemory(IrpContext, sizeof(VFAT_IRP_CONTEXT));
        IrpContext->Irp = Irp;
        IrpContext->DeviceObject = DeviceObject;
        IrpContext->DeviceExt = DeviceObject->DeviceExtension;
        IrpContext->Stack = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpContext->Stack);
        MajorFunction = IrpContext->MajorFunction = IrpContext->Stack->MajorFunction;
        IrpContext->MinorFunction = IrpContext->Stack->MinorFunction;
        IrpContext->FileObject = IrpContext->Stack->FileObject;
        IrpContext->Flags = IRPCONTEXT_COMPLETE;

        /* Easy cases that can wait */
        if (MajorFunction == IRP_MJ_CLEANUP ||
            MajorFunction == IRP_MJ_CREATE ||
            MajorFunction == IRP_MJ_SHUTDOWN ||
            MajorFunction == IRP_MJ_CLOSE /* likely to be fixed */)
        {
            SetFlag(IrpContext->Flags, IRPCONTEXT_CANWAIT);
        }
        /* Cases that can wait if synchronous IRP */
        else if ((MajorFunction == IRP_MJ_DEVICE_CONTROL ||
                  MajorFunction == IRP_MJ_QUERY_INFORMATION ||
                  MajorFunction == IRP_MJ_SET_INFORMATION ||
                  MajorFunction == IRP_MJ_FLUSH_BUFFERS ||
                  MajorFunction == IRP_MJ_LOCK_CONTROL ||
                  MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION ||
                  MajorFunction == IRP_MJ_SET_VOLUME_INFORMATION ||
                  MajorFunction == IRP_MJ_DIRECTORY_CONTROL ||
                  MajorFunction == IRP_MJ_WRITE ||
                  MajorFunction == IRP_MJ_READ) &&
                 IoIsOperationSynchronous(Irp))
        {
            SetFlag(IrpContext->Flags, IRPCONTEXT_CANWAIT);
        }
        /* Cases that can wait if synchronous or if no FO */
        else if ((MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
                  MajorFunction == IRP_MJ_PNP) &&
                 (IoGetCurrentIrpStackLocation(Irp)->FileObject == NULL ||
                  IoIsOperationSynchronous(Irp)))
        {
            SetFlag(IrpContext->Flags, IRPCONTEXT_CANWAIT);
        }

        KeInitializeEvent(&IrpContext->Event, NotificationEvent, FALSE);
        IrpContext->RefCount = 0;
        IrpContext->PriorityBoost = IO_NO_INCREMENT;
    }
    return IrpContext;
}

static WORKER_THREAD_ROUTINE VfatDoRequest;

static
VOID
NTAPI
VfatDoRequest(
    PVOID Context)
{
    PVFAT_IRP_CONTEXT IrpContext = Context;
    PDEVICE_EXTENSION DeviceExt;
    KIRQL OldIrql;

    InterlockedDecrement(&QueueCount);

    if (IrpContext->Stack->FileObject != NULL)
    {
        DeviceExt = IrpContext->Stack->DeviceObject->DeviceExtension;
        ObReferenceObject(DeviceExt->VolumeDevice);
    }

    do
    {
        DPRINT("VfatDoRequest(IrpContext %p), MajorFunction %x, %d\n",
               IrpContext, IrpContext->MajorFunction, QueueCount);
        VfatDispatchRequest(IrpContext);
        IrpContext = NULL;

        /* Now process any overflow items */
        if (DeviceExt != NULL)
        {
            KeAcquireSpinLock(&DeviceExt->OverflowQueueSpinLock, &OldIrql);
            if (DeviceExt->OverflowQueueCount != 0)
            {
                IrpContext = CONTAINING_RECORD(RemoveHeadList(&DeviceExt->OverflowQueue),
                                               VFAT_IRP_CONTEXT,
                                               WorkQueueItem.List);
                DeviceExt->OverflowQueueCount--;
                DPRINT("Processing overflow item for IRP %p context %p (%lu)\n",
                       IrpContext->Irp, IrpContext, DeviceExt->OverflowQueueCount);
            }
            else
            {
                ASSERT(IsListEmpty(&DeviceExt->OverflowQueue));
                DeviceExt->PostedRequestCount--;
            }
            KeReleaseSpinLock(&DeviceExt->OverflowQueueSpinLock, OldIrql);
        }
    } while (IrpContext != NULL);

    if (DeviceExt != NULL)
    {
        ObDereferenceObject(DeviceExt->VolumeDevice);
    }
}

static
NTSTATUS
VfatQueueRequest(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExt;
    KIRQL OldIrql;
    BOOLEAN Overflow;

    InterlockedIncrement(&QueueCount);
    DPRINT("VfatQueueRequest(IrpContext %p), %d\n", IrpContext, QueueCount);

    ASSERT(IrpContext != NULL);
    ASSERT(IrpContext->Irp != NULL);
    ASSERT(!(IrpContext->Flags & IRPCONTEXT_QUEUE) &&
           (IrpContext->Flags & IRPCONTEXT_COMPLETE));

    Overflow = FALSE;
    IrpContext->Flags |= IRPCONTEXT_CANWAIT;
    IoMarkIrpPending(IrpContext->Irp);

    /* We should not block more than two worker threads per volume,
     * or we might stop Cc from doing the work to unblock us.
     * Add additional requests into the overflow queue instead and process
     * them all in an existing worker thread (see VfatDoRequest above).
     */
    if (IrpContext->Stack->FileObject != NULL)
    {
        DeviceExt = IrpContext->Stack->DeviceObject->DeviceExtension;
        KeAcquireSpinLock(&DeviceExt->OverflowQueueSpinLock, &OldIrql);
        if (DeviceExt->PostedRequestCount > 2)
        {
            DeviceExt->OverflowQueueCount++;
            DPRINT("Queue overflow. Adding IRP %p context %p to overflow queue (%lu)\n",
                   IrpContext->Irp, IrpContext, DeviceExt->OverflowQueueCount);
            InsertTailList(&DeviceExt->OverflowQueue,
                           &IrpContext->WorkQueueItem.List);
            Overflow = TRUE;
        }
        else
        {
            DeviceExt->PostedRequestCount++;
        }
        KeReleaseSpinLock(&DeviceExt->OverflowQueueSpinLock, OldIrql);
    }

    if (!Overflow)
    {
        ExInitializeWorkItem(&IrpContext->WorkQueueItem, VfatDoRequest, IrpContext);
        ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
    }

    return STATUS_PENDING;
}

PVOID
VfatGetUserBuffer(
    IN PIRP Irp,
    IN BOOLEAN Paging)
{
    ASSERT(Irp);

    if (Irp->MdlAddress)
    {
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, (Paging ? HighPagePriority : NormalPagePriority));
    }
    else
    {
        return Irp->UserBuffer;
    }
}

NTSTATUS
VfatLockUserBuffer(
    IN PIRP Irp,
    IN ULONG Length,
    IN LOCK_OPERATION Operation)
{
    ASSERT(Irp);

    if (Irp->MdlAddress)
    {
        return STATUS_SUCCESS;
    }

    IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);

    if (!Irp->MdlAddress)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY
    {
        MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

BOOLEAN
VfatCheckForDismount(
    IN PDEVICE_EXTENSION DeviceExt,
    IN BOOLEAN Force)
{
    KIRQL OldIrql;
    ULONG UnCleanCount;
    PVPB Vpb;
    BOOLEAN Delete;

    DPRINT1("VfatCheckForDismount(%p, %u)\n", DeviceExt, Force);

    /* If the VCB is OK (not under uninitialization) and we don't force dismount, do nothing */
    if (BooleanFlagOn(DeviceExt->Flags, VCB_GOOD) && !Force)
    {
        return FALSE;
    }

    /*
     * NOTE: In their *CheckForDismount() function, our 3rd-party FS drivers
     * as well as MS' fastfat, perform a comparison check of the current VCB's
     * VPB ReferenceCount with some sort of "dangling"/"residual" open count,
     * depending on whether or not we are in IRP_MJ_CREATE.
     * It seems to be related to the fact that the volume root directory as
     * well as auxiliary data stream(s) are still opened, and only these are
     * allowed to be opened at that moment. After analysis it appears that for
     * the ReactOS' vfatfs, this number is equal to "2".
     */
    UnCleanCount = 2;

    /* Lock VPB */
    IoAcquireVpbSpinLock(&OldIrql);

    /* Reference it and check if a create is being done */
    Vpb = DeviceExt->IoVPB;
    DPRINT("Vpb->ReferenceCount = %d\n", Vpb->ReferenceCount);
    if (Vpb->ReferenceCount != UnCleanCount || DeviceExt->OpenHandleCount != 0)
    {
        /* If we force-unmount, copy the VPB to our local own to prepare later dismount */
        if (Force && Vpb->RealDevice->Vpb == Vpb && DeviceExt->SpareVPB != NULL)
        {
            RtlZeroMemory(DeviceExt->SpareVPB, sizeof(VPB));
            DeviceExt->SpareVPB->Type = IO_TYPE_VPB;
            DeviceExt->SpareVPB->Size = sizeof(VPB);
            DeviceExt->SpareVPB->RealDevice = DeviceExt->IoVPB->RealDevice;
            DeviceExt->SpareVPB->DeviceObject = NULL;
            DeviceExt->SpareVPB->Flags = DeviceExt->IoVPB->Flags & VPB_REMOVE_PENDING;
            DeviceExt->IoVPB->RealDevice->Vpb = DeviceExt->SpareVPB;
            DeviceExt->SpareVPB = NULL;
            DeviceExt->IoVPB->Flags |= VPB_PERSISTENT;

            /* We are uninitializing, the VCB cannot be used anymore */
            ClearFlag(DeviceExt->Flags, VCB_GOOD);
        }

        /* Don't do anything for now */
        Delete = FALSE;
    }
    else
    {
        /* Otherwise, delete the volume */
        Delete = TRUE;

        /* Swap the VPB with our local own */
        if (Vpb->RealDevice->Vpb == Vpb && DeviceExt->SpareVPB != NULL)
        {
            RtlZeroMemory(DeviceExt->SpareVPB, sizeof(VPB));
            DeviceExt->SpareVPB->Type = IO_TYPE_VPB;
            DeviceExt->SpareVPB->Size = sizeof(VPB);
            DeviceExt->SpareVPB->RealDevice = DeviceExt->IoVPB->RealDevice;
            DeviceExt->SpareVPB->DeviceObject = NULL;
            DeviceExt->SpareVPB->Flags = DeviceExt->IoVPB->Flags & VPB_REMOVE_PENDING;
            DeviceExt->IoVPB->RealDevice->Vpb = DeviceExt->SpareVPB;
            DeviceExt->SpareVPB = NULL;
            DeviceExt->IoVPB->Flags |= VPB_PERSISTENT;

            /* We are uninitializing, the VCB cannot be used anymore */
            ClearFlag(DeviceExt->Flags, VCB_GOOD);
        }

        /*
         * We defer setting the VPB's DeviceObject to NULL for later because
         * we want to handle the closing of the internal opened meta-files.
         */

        /* Clear the mounted and locked flags in the VPB */
        ClearFlag(Vpb->Flags, VPB_MOUNTED | VPB_LOCKED);
    }

    /* Release lock and return status */
    IoReleaseVpbSpinLock(OldIrql);

    /* If we were to delete, delete volume */
    if (Delete)
    {
        LARGE_INTEGER Zero = {{0,0}};
        PVFATFCB Fcb;

        /* We are uninitializing, the VCB cannot be used anymore */
        ClearFlag(DeviceExt->Flags, VCB_GOOD);

        /* Invalidate and close the internal opened meta-files */
        if (DeviceExt->RootFcb)
        {
            Fcb = DeviceExt->RootFcb;
            CcUninitializeCacheMap(Fcb->FileObject,
                                   &Zero,
                                   NULL);
            ObDereferenceObject(Fcb->FileObject);
            DeviceExt->RootFcb = NULL;
            vfatDestroyFCB(Fcb);
        }
        if (DeviceExt->VolumeFcb)
        {
            Fcb = DeviceExt->VolumeFcb;
#ifndef VOLUME_IS_NOT_CACHED_WORK_AROUND_IT
            CcUninitializeCacheMap(Fcb->FileObject,
                                   &Zero,
                                   NULL);
            ObDereferenceObject(Fcb->FileObject);
#endif
            DeviceExt->VolumeFcb = NULL;
            vfatDestroyFCB(Fcb);
        }
        if (DeviceExt->FATFileObject)
        {
            Fcb = DeviceExt->FATFileObject->FsContext;
            CcUninitializeCacheMap(DeviceExt->FATFileObject,
                                   &Zero,
                                   NULL);
            DeviceExt->FATFileObject->FsContext = NULL;
            ObDereferenceObject(DeviceExt->FATFileObject);
            DeviceExt->FATFileObject = NULL;
            vfatDestroyFCB(Fcb);
        }

        ASSERT(DeviceExt->OverflowQueueCount == 0);
        ASSERT(IsListEmpty(&DeviceExt->OverflowQueue));
        ASSERT(DeviceExt->PostedRequestCount == 0);

        /*
         * Now that the closing of the internal opened meta-files has been
         * handled, we can now set the VPB's DeviceObject to NULL.
         */
        Vpb->DeviceObject = NULL;

        /* If we have a local VPB, we'll have to delete it
         * but we won't dismount us - something went bad before
         */
        if (DeviceExt->SpareVPB)
        {
            ExFreePool(DeviceExt->SpareVPB);
        }
        /* Otherwise, delete any of the available VPB if its reference count is zero */
        else if (DeviceExt->IoVPB->ReferenceCount == 0)
        {
            ExFreePool(DeviceExt->IoVPB);
        }

        /* Remove the volume from the list */
        ExAcquireResourceExclusiveLite(&VfatGlobalData->VolumeListLock, TRUE);
        RemoveEntryList(&DeviceExt->VolumeListEntry);
        ExReleaseResourceLite(&VfatGlobalData->VolumeListLock);

        /* Uninitialize the notify synchronization object */
        FsRtlNotifyUninitializeSync(&DeviceExt->NotifySync);

        /* Release resources */
        ExFreePoolWithTag(DeviceExt->Statistics, TAG_STATS);
        ExDeleteResourceLite(&DeviceExt->DirResource);
        ExDeleteResourceLite(&DeviceExt->FatResource);

        /* Dismount our device if possible */
        ObDereferenceObject(DeviceExt->StorageDevice);
        IoDeleteDevice(DeviceExt->VolumeDevice);
    }

    return Delete;
}
