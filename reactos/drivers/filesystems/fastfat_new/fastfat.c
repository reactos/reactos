/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fastfat.c
 * PURPOSE:         Initialization routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* GLOBALS ******************************************************************/

FAT_GLOBAL_DATA FatGlobalData;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Fat");
    NTSTATUS Status;

    /* Create a device object */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status)) return Status;

    /* Zero global storage */
    RtlZeroMemory(&FatGlobalData, sizeof(FAT_GLOBAL_DATA));
    FatGlobalData.DriverObject = DriverObject;
    FatGlobalData.DiskDeviceObject = DeviceObject;

    /* Fill major function handlers */
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FatClose;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FatCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = FatRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = FatWrite;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = FatFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = FatQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = FatSetInformation;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = FatDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = FatQueryVolumeInfo;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = FatSetVolumeInfo;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = FatShutdown;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = FatLockControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FatDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FatCleanup;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = FatFlushBuffers;
    //DriverObject->MajorFunction[IRP_MJ_QUERY_EA]
    //DriverObject->MajorFunction[IRP_MJ_SET_EA]
    //DriverObject->MajorFunction[IRP_MJ_PNP]

    DriverObject->DriverUnload = NULL;

    /* Initialize cache manager callbacks */
    FatGlobalData.CacheMgrCallbacks.AcquireForLazyWrite = FatAcquireForLazyWrite;
    FatGlobalData.CacheMgrCallbacks.ReleaseFromLazyWrite = FatReleaseFromLazyWrite;
    FatGlobalData.CacheMgrCallbacks.AcquireForReadAhead = FatAcquireForReadAhead;
    FatGlobalData.CacheMgrCallbacks.ReleaseFromReadAhead = FatReleaseFromReadAhead;

    FatGlobalData.CacheMgrCallbacks.AcquireForLazyWrite = FatNoopAcquire;
    FatGlobalData.CacheMgrCallbacks.ReleaseFromLazyWrite = FatNoopRelease;
    FatGlobalData.CacheMgrCallbacks.AcquireForReadAhead = FatNoopAcquire;
    FatGlobalData.CacheMgrCallbacks.ReleaseFromReadAhead = FatNoopRelease;

    /* Initialize Fast I/O dispatchers */
    FatInitFastIoRoutines(&FatGlobalData.FastIoDispatch);
    DriverObject->FastIoDispatch = &FatGlobalData.FastIoDispatch;

    /* Initialize lookaside lists */
    ExInitializeNPagedLookasideList(&FatGlobalData.NonPagedFcbList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FCB),
                                    TAG_FCB,
                                    0);

    ExInitializeNPagedLookasideList(&FatGlobalData.ResourceList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(ERESOURCE),
                                    TAG_CCB,
                                    0);

    ExInitializeNPagedLookasideList(&FatGlobalData.IrpContextList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FAT_IRP_CONTEXT),
                                    TAG_IRP,
                                    0);

    /* Initialize synchronization resource for the global data */
    ExInitializeResourceLite(&FatGlobalData.Resource);

    /* Register and reference our filesystem */
    IoRegisterFileSystem(DeviceObject);
    ObReferenceObject(DeviceObject);

    return STATUS_SUCCESS;
}

PFAT_IRP_CONTEXT
NTAPI
FatBuildIrpContext(PIRP Irp,
                   BOOLEAN CanWait)
{
    PIO_STACK_LOCATION IrpSp;
    PFAT_IRP_CONTEXT IrpContext;
    PVOLUME_DEVICE_OBJECT VolumeObject;

    /* Get current IRP stack location */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Allocate memory for the Irp context */
    IrpContext = ExAllocateFromNPagedLookasideList(&FatGlobalData.IrpContextList);

    /* Zero init memory */
    RtlZeroMemory(IrpContext, sizeof(FAT_IRP_CONTEXT));

    /* Save IRP, MJ and MN */
    IrpContext->Irp = Irp;
    IrpContext->MajorFunction = IrpSp->MajorFunction;
    IrpContext->MinorFunction = IrpSp->MinorFunction;

    /* Set DeviceObject */
    if (IrpSp->FileObject)
    {
        IrpContext->DeviceObject = IrpSp->FileObject->DeviceObject;

        /* Save VCB pointer */
        VolumeObject = (PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject;
        IrpContext->Vcb = &VolumeObject->Vcb;

        /* TODO: Handle write-through */
    }
    else if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)
    {
        /* Handle FSCTRL case */
        IrpContext->DeviceObject = IrpSp->Parameters.MountVolume.Vpb->RealDevice;
    }

    /* Set Wait flag */
    if (CanWait) IrpContext->Flags |= IRPCONTEXT_CANWAIT;

    /* Return prepared context */
    return IrpContext;
}

VOID
NTAPI
FatDestroyIrpContext(PFAT_IRP_CONTEXT IrpContext)
{
    PAGED_CODE();

    /* Make sure it has no pinned stuff */
    ASSERT(IrpContext->PinCount == 0);

    /* If there is a FatIo context associated with it - free it */
    if (IrpContext->FatIoContext)
    {
        if (!(IrpContext->Flags & IRPCONTEXT_STACK_IO_CONTEXT))
        {
            /* If a zero mdl was allocated - free it */
            if (IrpContext->FatIoContext->ZeroMdl)
                IoFreeMdl(IrpContext->FatIoContext->ZeroMdl);

            /* Free memory of FatIo context */
            ExFreePool(IrpContext->FatIoContext);
        }
    }

    /* Free memory */
    ExFreeToNPagedLookasideList(&FatGlobalData.IrpContextList, IrpContext);
}

VOID
NTAPI
FatCompleteRequest(PFAT_IRP_CONTEXT IrpContext OPTIONAL,
                   PIRP Irp OPTIONAL,
                   NTSTATUS Status)
{
    PAGED_CODE();

    if (IrpContext)
    {
        /* TODO: Unpin repinned BCBs */
        //ASSERT(IrpContext->Repinned.Bcb[0] == NULL);
        //FatUnpinRepinnedBcbs( IrpContext );

        /* Destroy IRP context */
        FatDestroyIrpContext(IrpContext);
    }

    /* Complete the IRP */
    if (Irp)
    {
        /* Cleanup IoStatus.Information in case of error input operation */
        if (NT_ERROR(Status) && (Irp->Flags & IRP_INPUT_OPERATION))
        {
            Irp->IoStatus.Information = 0;
        }

        /* Save status and complete this IRP */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    }
}


/* EOF */
