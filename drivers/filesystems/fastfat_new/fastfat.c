/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/fastfat.c
 * PURPOSE:         Initialization routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* GLOBALS ******************************************************************/

FAT_GLOBAL_DATA FatGlobalData;
FAST_MUTEX FatCloseQueueMutex;

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
    FatGlobalData.SystemProcess = PsGetCurrentProcess();

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

    FatGlobalData.CacheMgrNoopCallbacks.AcquireForLazyWrite = FatNoopAcquire;
    FatGlobalData.CacheMgrNoopCallbacks.ReleaseFromLazyWrite = FatNoopRelease;
    FatGlobalData.CacheMgrNoopCallbacks.AcquireForReadAhead = FatNoopAcquire;
    FatGlobalData.CacheMgrNoopCallbacks.ReleaseFromReadAhead = FatNoopRelease;

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

    /* Initialize queued close stuff */
    InitializeListHead(&FatGlobalData.AsyncCloseList);
    InitializeListHead(&FatGlobalData.DelayedCloseList);
    FatGlobalData.FatCloseItem = IoAllocateWorkItem(DeviceObject);
    ExInitializeFastMutex(&FatCloseQueueMutex);

    /* Initialize global VCB list */
    InitializeListHead(&FatGlobalData.VcbListHead);

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
    IrpContext->Stack = IrpSp;
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

VOID
NTAPI
FatDequeueRequest(IN PVOID Context)
{
    PFAT_IRP_CONTEXT IrpContext;

    IrpContext = (PFAT_IRP_CONTEXT) Context;

    /* Enter critical region. */
    FsRtlEnterFileSystem();

    /* Handle top level IRP Correctly. */
    if (!FlagOn(IrpContext->Flags, IRPCONTEXT_TOPLEVEL))
        IoSetTopLevelIrp((PIRP) FSRTL_FSP_TOP_LEVEL_IRP);

    /* Enable Synchronous IO. */
    SetFlag(IrpContext->Flags, IRPCONTEXT_CANWAIT);

    /* Invoke the handler routine. */
    IrpContext->QueuedOperationHandler(IrpContext);

    /* Restore top level IRP. */
	IoSetTopLevelIrp(NULL);

    /* Leave critical region. */
	FsRtlExitFileSystem();
}

VOID
NTAPI
FatQueueRequest(IN PFAT_IRP_CONTEXT IrpContext,
                IN PFAT_OPERATION_HANDLER OperationHandler)
{
    /* Save the worker routine. */
    IrpContext->QueuedOperationHandler = OperationHandler;

    /* Indicate if top level IRP was set. */
    if (IoGetTopLevelIrp() == IrpContext->Irp)
        SetFlag(IrpContext->Flags, IRPCONTEXT_TOPLEVEL);

    /* Initialize work item. */
    ExInitializeWorkItem(&IrpContext->WorkQueueItem,
        FatDequeueRequest,
        IrpContext);
    ExQueueWorkItem(&IrpContext->WorkQueueItem,
        DelayedWorkQueue);
}

TYPE_OF_OPEN
NTAPI
FatDecodeFileObject(IN PFILE_OBJECT FileObject,
                    OUT PVCB *Vcb,
                    OUT PFCB *FcbOrDcb,
                    OUT PCCB *Ccb)
{
    TYPE_OF_OPEN TypeOfOpen = UnopenedFileObject;
    PVOID FsContext = FileObject->FsContext;
    PVOID FsContext2 = FileObject->FsContext2;

    /* If FsContext is NULL, then everything is NULL */
    if (!FsContext)
    {
        *Ccb = NULL;
        *FcbOrDcb = NULL;
        *Vcb = NULL;

        return TypeOfOpen;
    }

    /* CCB is always stored in FsContext2 */
    *Ccb = FsContext2;

    /* Switch according to the NodeType */
    switch (FatNodeType(FsContext))
    {
        /* Volume */
        case FAT_NTC_VCB:
            *FcbOrDcb = NULL;
            *Vcb = FsContext;

            TypeOfOpen = ( *Ccb == NULL ? VirtualVolumeFile : UserVolumeOpen );

            break;

        /* Root or normal directory*/
        case FAT_NTC_ROOT_DCB:
        case FAT_NTC_DCB:
            *FcbOrDcb = FsContext;
            *Vcb = (*FcbOrDcb)->Vcb;

            TypeOfOpen = (*Ccb == NULL ? DirectoryFile : UserDirectoryOpen);

            DPRINT("Referencing a directory: %wZ\n", &(*FcbOrDcb)->FullFileName);
            break;

        /* File */
        case FAT_NTC_FCB:
            *FcbOrDcb = FsContext;
            *Vcb = (*FcbOrDcb)->Vcb;

            TypeOfOpen = (*Ccb == NULL ? EaFile : UserFileOpen);

            DPRINT("Referencing a file: %wZ\n", &(*FcbOrDcb)->FullFileName);

            break;

        default:
            DPRINT1("Unknown node type %x\n", FatNodeType(FsContext));
            ASSERT(FALSE);
    }

    return TypeOfOpen;
}

VOID
NTAPI
FatSetFileObject(PFILE_OBJECT FileObject,
                 TYPE_OF_OPEN TypeOfOpen,
                 PVOID Fcb,
                 PCCB Ccb)
{
    if (Fcb)
    {
        /* Check Fcb's type  */
        if (FatNodeType(Fcb) == FAT_NTC_VCB)
        {
            FileObject->Vpb = ((PVCB)Fcb)->Vpb;
        }
        else
        {
            FileObject->Vpb = ((PFCB)Fcb)->Vcb->Vpb;
        }
    }

    /* Set FsContext */
    if (FileObject)
    {
        FileObject->FsContext  = Fcb;
        FileObject->FsContext2 = Ccb;
    }
}


BOOLEAN
NTAPI
FatAcquireExclusiveVcb(IN PFAT_IRP_CONTEXT IrpContext,
                       IN PVCB Vcb)
{
    /* Acquire VCB's resource if possible */
    if (ExAcquireResourceExclusiveLite(&Vcb->Resource,
                                       BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOLEAN
NTAPI
FatAcquireSharedVcb(IN PFAT_IRP_CONTEXT IrpContext,
                    IN PVCB Vcb)
{
    /* Acquire VCB's resource if possible */
    if (ExAcquireResourceSharedLite(&Vcb->Resource,
                                    BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID
NTAPI
FatReleaseVcb(IN PFAT_IRP_CONTEXT IrpContext,
              IN PVCB Vcb)
{
    /* Release VCB's resource */
    ExReleaseResourceLite(&Vcb->Resource);
}

BOOLEAN
NTAPI
FatAcquireExclusiveFcb(IN PFAT_IRP_CONTEXT IrpContext,
                       IN PFCB Fcb)
{
RetryLockingE:
    /* Try to acquire the exclusive lock*/
    if (ExAcquireResourceExclusiveLite(Fcb->Header.Resource,
        BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        /* Wait same way MS's FASTFAT wait, i.e.
           checking that there are outstanding async writes,
           or someone is waiting on it*/
        if (Fcb->OutstandingAsyncWrites &&
            ((IrpContext->MajorFunction != IRP_MJ_WRITE) ||
             !FlagOn(IrpContext->Irp->Flags, IRP_NOCACHE) ||
             ExGetSharedWaiterCount(Fcb->Header.Resource) ||
             ExGetExclusiveWaiterCount(Fcb->Header.Resource)))
        {
            KeWaitForSingleObject(Fcb->OutstandingAsyncEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            /* Release the lock */
            FatReleaseFcb(IrpContext, Fcb);

            /* Retry */
            goto RetryLockingE;
        }

        /* Return success */
        return TRUE;
    }

    /* Return failure */
    return FALSE;
}

BOOLEAN
NTAPI
FatAcquireSharedFcb(IN PFAT_IRP_CONTEXT IrpContext,
                    IN PFCB Fcb)
{
RetryLockingS:
    /* Try to acquire the shared lock*/
    if (ExAcquireResourceSharedLite(Fcb->Header.Resource,
        BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        /* Wait same way MS's FASTFAT wait, i.e.
           checking that there are outstanding async writes,
           or someone is waiting on it*/
        if (Fcb->OutstandingAsyncWrites &&
            ((IrpContext->MajorFunction != IRP_MJ_WRITE) ||
             !FlagOn(IrpContext->Irp->Flags, IRP_NOCACHE) ||
             ExGetSharedWaiterCount(Fcb->Header.Resource) ||
             ExGetExclusiveWaiterCount(Fcb->Header.Resource)))
        {
            KeWaitForSingleObject(Fcb->OutstandingAsyncEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            /* Release the lock */
            FatReleaseFcb(IrpContext, Fcb);

            /* Retry */
            goto RetryLockingS;
        }

        /* Return success */
        return TRUE;
    }

    /* Return failure */
    return FALSE;
}

VOID
NTAPI
FatReleaseFcb(IN PFAT_IRP_CONTEXT IrpContext,
              IN PFCB Fcb)
{
    /* Release FCB's resource */
    ExReleaseResourceLite(Fcb->Header.Resource);
}

PVOID
FASTCALL
FatMapUserBuffer(PIRP Irp)
{
    if (!Irp->MdlAddress)
        return Irp->UserBuffer;
    else
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
}

BOOLEAN
NTAPI
FatIsTopLevelIrp(IN PIRP Irp)
{
    if (!IoGetTopLevelIrp())
    {
        IoSetTopLevelIrp(Irp);
        return TRUE;
    }

    return FALSE;
}

VOID
NTAPI
FatNotifyReportChange(IN PFAT_IRP_CONTEXT IrpContext,
                      IN PVCB Vcb,
                      IN PFCB Fcb,
                      IN ULONG Filter,
                      IN ULONG Action)
{
    if (Fcb->FullFileName.Buffer == NULL)
        FatSetFullFileNameInFcb(IrpContext, Fcb);

    ASSERT(Fcb->FullFileName.Length != 0 );
    ASSERT(Fcb->FileNameLength != 0 );
    ASSERT(Fcb->FullFileName.Length > Fcb->FileNameLength );
    ASSERT(Fcb->FullFileName.Buffer[(Fcb->FullFileName.Length - Fcb->FileNameLength)/sizeof(WCHAR) - 1] == L'\\' );

    FsRtlNotifyFullReportChange(Vcb->NotifySync,
                                &Vcb->NotifyList,
                                (PSTRING)&Fcb->FullFileName,
                                (USHORT)(Fcb->FullFileName.Length -
                                         Fcb->FileNameLength),
                                NULL,
                                NULL,
                                Filter,
                                Action,
                                NULL);
}

/* EOF */
