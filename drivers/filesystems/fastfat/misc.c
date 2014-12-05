/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/misc.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:
 *
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
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto Fail;
    }

    if (*Fcb->Attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Fail;
    }

    Status = FsRtlProcessFileLock(&Fcb->FileLock,
                                  IrpContext->Irp,
                                  NULL);

    VfatFreeIrpContext(IrpContext);
    return Status;

Fail:
    IrpContext->Irp->IoStatus.Status = Status;
    IoCompleteRequest(IrpContext->Irp, (CCHAR)(NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
    VfatFreeIrpContext(IrpContext);
    return Status;
}

static
NTSTATUS
VfatDeviceControl(
    IN PVFAT_IRP_CONTEXT IrpContext)
{
    IoSkipCurrentIrpStackLocation(IrpContext->Irp);

    return IoCallDriver(IrpContext->DeviceExt->StorageDevice, IrpContext->Irp);
}

static
NTSTATUS
VfatDispatchRequest(
    IN PVFAT_IRP_CONTEXT IrpContext)
{
    DPRINT("VfatDispatchRequest (IrpContext %p), is called for %s\n", IrpContext,
           IrpContext->MajorFunction >= IRP_MJ_MAXIMUM_FUNCTION ? "????" : MajorFunctionNames[IrpContext->MajorFunction]);

    ASSERT(IrpContext);

    switch (IrpContext->MajorFunction)
    {
        case IRP_MJ_CLOSE:
            return VfatClose(IrpContext);
        case IRP_MJ_CREATE:
         return VfatCreate(IrpContext);
        case IRP_MJ_READ:
            return VfatRead (IrpContext);
        case IRP_MJ_WRITE:
            return VfatWrite (IrpContext);
        case IRP_MJ_FILE_SYSTEM_CONTROL:
            return VfatFileSystemControl(IrpContext);
        case IRP_MJ_QUERY_INFORMATION:
            return VfatQueryInformation (IrpContext);
        case IRP_MJ_SET_INFORMATION:
            return VfatSetInformation (IrpContext);
        case IRP_MJ_DIRECTORY_CONTROL:
            return VfatDirectoryControl(IrpContext);
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            return VfatQueryVolumeInformation(IrpContext);
        case IRP_MJ_SET_VOLUME_INFORMATION:
            return VfatSetVolumeInformation(IrpContext);
        case IRP_MJ_LOCK_CONTROL:
            return VfatLockControl(IrpContext);
        case IRP_MJ_DEVICE_CONTROL:
            return VfatDeviceControl(IrpContext);
        case IRP_MJ_CLEANUP:
            return VfatCleanup(IrpContext);
        case IRP_MJ_FLUSH_BUFFERS:
            return VfatFlush(IrpContext);
        case IRP_MJ_PNP:
            return VfatPnp(IrpContext);
        default:
            DPRINT1("Unexpected major function %x\n", IrpContext->MajorFunction);
            IrpContext->Irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
            IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
            VfatFreeIrpContext(IrpContext);
            return STATUS_DRIVER_INTERNAL_ERROR;
    }
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
        FsRtlEnterFileSystem();
        Status = VfatDispatchRequest(IrpContext);
        FsRtlExitFileSystem();
    }
    return Status;
}

VOID
VfatFreeIrpContext(
    PVFAT_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);
    ExFreeToNPagedLookasideList(&VfatGlobalData->IrpContextLookasideList, IrpContext);
}

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
        IrpContext->Flags = 0;
        if (MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
            MajorFunction == IRP_MJ_DEVICE_CONTROL ||
            MajorFunction == IRP_MJ_SHUTDOWN)
        {
            IrpContext->Flags |= IRPCONTEXT_CANWAIT;
        }
        else if (MajorFunction != IRP_MJ_CLEANUP &&
                 MajorFunction != IRP_MJ_CLOSE &&
                 IoIsOperationSynchronous(Irp))
        {
            IrpContext->Flags |= IRPCONTEXT_CANWAIT;
        }
        KeInitializeEvent(&IrpContext->Event, NotificationEvent, FALSE);
        IrpContext->RefCount = 0;
    }
    return IrpContext;
}

static WORKER_THREAD_ROUTINE VfatDoRequest;

static
VOID
NTAPI
VfatDoRequest(
    PVOID IrpContext)
{
    InterlockedDecrement(&QueueCount);
    DPRINT("VfatDoRequest(IrpContext %p), MajorFunction %x, %d\n",
           IrpContext, ((PVFAT_IRP_CONTEXT)IrpContext)->MajorFunction, QueueCount);
    FsRtlEnterFileSystem();
    VfatDispatchRequest((PVFAT_IRP_CONTEXT)IrpContext);
    FsRtlExitFileSystem();
}

NTSTATUS
VfatQueueRequest(
    PVFAT_IRP_CONTEXT IrpContext)
{
    InterlockedIncrement(&QueueCount);
    DPRINT("VfatQueueRequest(IrpContext %p), %d\n", IrpContext, QueueCount);

    ASSERT(IrpContext != NULL);
    ASSERT(IrpContext->Irp != NULL);

    IrpContext->Flags |= IRPCONTEXT_CANWAIT;
    IoMarkIrpPending(IrpContext->Irp);
    ExInitializeWorkItem(&IrpContext->WorkQueueItem, VfatDoRequest, IrpContext);
    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
    return STATUS_PENDING;
}

PVOID
VfatGetUserBuffer(
    IN PIRP Irp)
{
    ASSERT(Irp);

    if (Irp->MdlAddress)
    {
        /* This call may be in the paging path, so use maximum priority */
        /* FIXME: call with normal priority in the non-paging path */
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
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

    MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);

    return STATUS_SUCCESS;
}

BOOLEAN
VfatCheckForDismount(
    IN PDEVICE_EXTENSION DeviceExt,
    IN BOOLEAN Create)
{
    KIRQL OldIrql;
    PVPB Vpb;
    BOOLEAN Delete;

    DPRINT1("VfatCheckForDismount(%p, %u)\n", DeviceExt, Create);

    /* Lock VPB */
    IoAcquireVpbSpinLock(&OldIrql);

    /* Reference it and check if a create is being done */
    Vpb = DeviceExt->IoVPB;
    if (Vpb->ReferenceCount != Create)
    {
        /* Copy the VPB to our local own to prepare later dismount */
        if (DeviceExt->SpareVPB != NULL)
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
        }

        /* Don't do anything */
        Delete = FALSE;
    }
    else
    {
        /* Otherwise, delete the volume */
        Delete = TRUE;

        /* Check if it has a VPB and unmount it */
        if (Vpb->RealDevice->Vpb == Vpb)
        {
            Vpb->DeviceObject = NULL;
            Vpb->Flags &= ~VPB_MOUNTED;
        }
    }

    /* Release lock and return status */
    IoReleaseVpbSpinLock(OldIrql);

    /* If we were to delete, delete volume */
    if (Delete)
    {
        PVPB DelVpb;

        /* If we have a local VPB, we'll have to delete it
         * but we won't dismount us - something went bad before
         */
        if (DeviceExt->SpareVPB)
        {
            DelVpb = DeviceExt->SpareVPB;
        }
        /* Otherwise, dismount our device if possible */
        else
        {
            if (DeviceExt->IoVPB->ReferenceCount)
            {
                ObfDereferenceObject(DeviceExt->StorageDevice);
                IoDeleteDevice(DeviceExt->VolumeDevice);
                return Delete;
            }

            DelVpb = DeviceExt->IoVPB;
        }

        /* Delete any of the available VPB and dismount */
        ExFreePool(DelVpb);
        ObfDereferenceObject(DeviceExt->StorageDevice);
        IoDeleteDevice(DeviceExt->VolumeDevice);

        return Delete;
    }

    return Delete;
}        
