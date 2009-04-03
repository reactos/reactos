/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/blockdev.c
 * PURPOSE:         Temporary sector reading support
 * PROGRAMMERS:     Alexey Vlasov
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* FUNCTIONS ***************************************************************/
//FIXME: There is a conflicting function FatPerformDevIoCtrl doing same thing!
NTSTATUS 
FatDiskIoControl_(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoCtlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferSize,
    IN OUT PVOID OutputBuffer,
    IN OUT PULONG OutputBufferSize OPTIONAL)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    ULONG OutBufferSize;
    IO_STATUS_BLOCK IoStatus;

    /* Set out buffer size if it was supplied. */
    OutBufferSize = (ARGUMENT_PRESENT(OutputBufferSize)
        ? 0 : *OutputBufferSize);

    /* Initialize event if the operation will be pended. */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Build the Irp. */
    Irp = IoBuildDeviceIoControlRequest(IoCtlCode, DeviceObject,
        InputBuffer, InputBufferSize, OutputBuffer, OutBufferSize,
        FALSE, &Event, &IoStatus);
    if (Irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Send IRP to Disk Device */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    /* Return output buffer length if required. */
    if (ARGUMENT_PRESENT(OutputBufferSize))
        *OutputBufferSize = IoStatus.Information;
    return Status;
}

PVOID
FatMapUserBuffer(
    IN OUT PIRP Irp)
/*
 * FUNCTION:
 *      
 *      
 * ARGUMENTS:
 *      IrpContext = Pointer to FCB structure for the file.
 *      Irp = Pointer to the IRP structure
 * RETURNS: Status Value.
 * NOTES:
 */
{
    PVOID Address;

    if (Irp->MdlAddress == NULL)
        return Irp->UserBuffer;
    Address = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    if (Address == NULL)
        ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
    return Address;
}

NTSTATUS
FatLockUserBuffer (
    IN PFAT_IRP_CONTEXT IrpContext,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength)
/*
 * FUNCTION:
 *      
 *      
 * ARGUMENTS:
 *      IrpContext = Pointer to FCB structure for the file.
 *      Operation = Type of lock operation.
 *      BufferLength = Buffer length to be locked.
 * RETURNS: Status Value.
 * NOTES:
 */
{
    PMDL Mdl;
    PIRP Irp;
    NTSTATUS Status;

    Mdl = NULL;
    Irp = IrpContext->Irp;
    Status = STATUS_SUCCESS;
    if (Irp->MdlAddress == NULL)
    {
        NTSTATUS Status;
        Mdl = IoAllocateMdl(Irp->UserBuffer,
        BufferLength, FALSE, FALSE, Irp);
        if (Mdl == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
        _SEH2_TRY
        {
            MmProbeAndLockPages(Mdl,
            Irp->RequestorMode, Operation);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            IoFreeMdl( Mdl );
            Irp->MdlAddress = NULL;
        } _SEH2_END
    }
    return Status;
}

NTSTATUS
FatIoSyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PFAT_IO_CONTEXT IoContext;

    IoContext = (PFAT_IO_CONTEXT) Context;

   /* Check if this is an associated irp. */
    if (Irp != IoContext->Irp)
    {
        if (!NT_SUCCESS(Irp->IoStatus.Status))
            IoContext->Irp->IoStatus = Irp->IoStatus;
        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);
        if (InterlockedDecrement(&IoContext->RunCount) != 0)
            return STATUS_MORE_PROCESSING_REQUIRED;
        /* This was the last run, update master irp. */
        if (NT_SUCCESS(IoContext->Irp->IoStatus.Status))
            IoContext->Irp->IoStatus.Information = IoContext->Length;
    }

    /* This is the last associated irp or a single irp IO. */
    if (NT_SUCCESS(IoContext->Irp->IoStatus.Status) &&
        !FlagOn(IoContext->Irp->Flags, IRP_PAGING_IO))
    {
        /* Maintain FileObject CurrentByteOffset */
        IoContext->FileObject->CurrentByteOffset.QuadPart =
            IoContext->Offset + IoContext->Irp->IoStatus.Information;
    }

    /* Signal about completion. */
    KeSetEvent(&IoContext->Wait.SyncEvent, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
FatIoAsyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PFAT_IO_CONTEXT IoContext;

    IoContext = (PFAT_IO_CONTEXT) Context;

   /* Check if this is an associated irp. */
    if (Irp != IoContext->Irp)
    {
        if (!NT_SUCCESS(Irp->IoStatus.Status))
            IoContext->Irp->IoStatus = Irp->IoStatus;
        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);
        if (InterlockedDecrement(&IoContext->RunCount) != 0)
            return STATUS_MORE_PROCESSING_REQUIRED;
        /* This was the last run, update master irp. */
        if (NT_SUCCESS(IoContext->Irp->IoStatus.Status))
            IoContext->Irp->IoStatus.Information = IoContext->Length;
    }


    /* This is the last associated irp or a single irp IO. */
    if (NT_SUCCESS(IoContext->Irp->IoStatus.Status) &&
        !FlagOn(IoContext->Irp->Flags, IRP_PAGING_IO))
    {
        /* Maintain FileObject Flags */
        if (IoGetCurrentIrpStackLocation(IoContext->Irp)->MajorFunction
                == IRP_MJ_READ)
        {
            SetFlag(IoContext->FileObject->Flags, FO_FILE_FAST_IO_READ);
        }
        else
        {
            SetFlag(IoContext->FileObject->Flags, FO_FILE_MODIFIED);
        }
    }
    if (IoContext->Wait.Async.Resource != NULL)
        ExReleaseResourceForThreadLite(
            IoContext->Wait.Async.Resource,
            IoContext->Wait.Async.ResourceThreadId);

    if (IoContext->Wait.Async.PagingIoResource != NULL)
        ExReleaseResourceForThreadLite(
            IoContext->Wait.Async.PagingIoResource,
            IoContext->Wait.Async.ResourceThreadId);

    IoMarkIrpPending(Irp);
    ExFreePool(Context);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FatPerformLboIo(
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PLARGE_INTEGER Offset,
    IN SIZE_T Length)
{
    BOOLEAN CanWait, ReadOperation;
    PIO_STACK_LOCATION IoStack;

    CanWait = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT);
    ReadOperation = (IrpContext->Stack->MajorFunction == IRP_MJ_READ);

    /* Allocate completion context */
    IrpContext->FatIoContext = FsRtlAllocatePoolWithTag(
        NonPagedPool, sizeof(FAT_IO_CONTEXT), (ULONG) 'xCoI');

    if (IrpContext->FatIoContext == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(IrpContext->FatIoContext,
        sizeof(FAT_IO_CONTEXT));

    /* Initialize event if we are supposed to wait. */
    if (CanWait)
    {
        KeInitializeEvent(
            &IrpContext->FatIoContext->Wait.SyncEvent,
            NotificationEvent, FALSE);
    }

    /* Set the completion routine depending on wait semantics. */
    IoSetCompletionRoutine(IrpContext->Irp,
        (CanWait
            ? FatIoSyncCompletionRoutine
            : FatIoAsyncCompletionRoutine),
        IrpContext->FatIoContext, TRUE, TRUE, TRUE);

    /* Setup stack location. */
    IoStack = IoGetNextIrpStackLocation(IrpContext->Irp);
    IoStack->MajorFunction = IrpContext->MajorFunction;
    IoStack->Parameters.Read.Length = (ULONG) Length;
    IoStack->Parameters.Read.ByteOffset = *Offset;
    if (FlagOn(IrpContext->Flags, IRPCONTEXT_WRITETHROUGH))
        SetFlag(IoStack->Flags, SL_WRITE_THROUGH);

    IoCallDriver(
        IrpContext->Vcb->TargetDeviceObject,
        IrpContext->Irp);
    if (CanWait)
    {
        KeWaitForSingleObject(
            &IrpContext->FatIoContext->Wait.SyncEvent,
            Executive, KernelMode, FALSE, NULL);
        return IrpContext->Irp->IoStatus.Status;
    }
    SetFlag(IrpContext->Flags, IRPCONTEXT_STACK_IO_CONTEXT);
    return STATUS_PENDING;
}



NTSTATUS
FatPerformVirtualNonCachedIo(
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLARGE_INTEGER Offset,
    IN SIZE_T Length)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    LONGLONG Vbo, Lbo, RunLength;
    ULONG RunCount, CleanupIndex, FirstIndex, BeyoundLastIndex;
    BOOLEAN CanWait, ReadOperation;
    PIRP* RunIrp;
    PMDL Mdl;

    ASSERT(IrpContext->FatIoContext == NULL);


    FirstIndex = CleanupIndex = 0;
    CanWait = BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT);
    ReadOperation = (IrpContext->Stack->MajorFunction == IRP_MJ_READ);
    Status = FatLockUserBuffer(IrpContext,
        (ReadOperation ? IoWriteAccess : IoReadAccess),
        Length);
    if (!NT_SUCCESS(Status))
        goto FatIoPerformNonCachedCleanup;
    Vbo = Offset->QuadPart;
    RunLength = Length;
    _SEH2_TRY
    {
        BeyoundLastIndex = FatScanFat(Fcb, Vbo,
            &Lbo, &RunLength, &FirstIndex, CanWait);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        _SEH2_YIELD(goto FatIoPerformNonCachedCleanup;)
    }_SEH2_END
    RunCount = BeyoundLastIndex - FirstIndex;
    if (RunCount == 0)
    {
        Status = STATUS_END_OF_FILE;
        goto FatIoPerformNonCachedCleanup;
    }
    Length = sizeof(FAT_IO_CONTEXT);
    if (RunCount > 0x1)
        Length += RunCount * sizeof(PIRP);
    IrpContext->FatIoContext = FsRtlAllocatePoolWithTag(
        NonPagedPool, Length, (ULONG) 'xCoI');
    if (IrpContext->FatIoContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FatIoPerformNonCachedCleanup;
    }
    RtlZeroMemory(IrpContext->FatIoContext, Length);
    if (CanWait)
    {
        KeInitializeEvent(
            &IrpContext->FatIoContext->Wait.SyncEvent,
            NotificationEvent, FALSE);
    }
    if (RunCount == 0x1)
    {
        IoSetCompletionRoutine(IrpContext->Irp,
            (CanWait ? FatIoSyncCompletionRoutine
            : FatIoAsyncCompletionRoutine),
        IrpContext->FatIoContext, TRUE, TRUE, TRUE);
        IoStack = IoGetNextIrpStackLocation(IrpContext->Irp);
        IoStack->MajorFunction = IrpContext->Stack->MajorFunction;
        IoStack->Parameters.Read.Length = (ULONG) RunLength;
        IoStack->Parameters.Read.ByteOffset.QuadPart = Lbo;
        IoStack->Flags = FlagOn(
            IrpContext->Stack->Flags,
            SL_WRITE_THROUGH);
        Status = IoCallDriver(
            IrpContext->Vcb->TargetDeviceObject,
            IrpContext->Irp);
        goto FatIoPerformNonCachedComplete;
    }
   /*
    * We already have the first run retrieved by FatiScanFat.
    */
    for (RunIrp = &IrpContext->FatIoContext->Irp,
            CleanupIndex = FirstIndex;
        CleanupIndex < BeyoundLastIndex;
        CleanupIndex ++, RunIrp ++)
    {
#if DBG
        LONGLONG NextVbo = Vbo + RunLength;
        BOOLEAN RunExists;
#endif
       /*
        * Allocate Irp for the run.
        */
        *RunIrp = IoMakeAssociatedIrp(IrpContext->Irp,
            (CCHAR)(IrpContext->Vcb->TargetDeviceObject->StackSize + 1));
        if (*RunIrp == NULL) 
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FatIoPerformNonCachedCleanup;
        }
        CleanupIndex ++;
        /*
        * Build Mdl for the run range.
        */
        Mdl = IoAllocateMdl(
            Add2Ptr(IrpContext->Irp->UserBuffer, Vbo, PVOID),
            (ULONG) RunLength, FALSE, FALSE, *RunIrp);
        if (Mdl == NULL) 
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FatIoPerformNonCachedCleanup;
        }
        IoBuildPartialMdl(IrpContext->Irp->MdlAddress, Mdl,
            Add2Ptr(IrpContext->Irp->UserBuffer, Vbo, PVOID),
            (ULONG) RunLength);
        /*
        * Setup IRP for each run.
        */
        IoSetCompletionRoutine(IrpContext->Irp,
            (CanWait ? FatIoSyncCompletionRoutine
                : FatIoAsyncCompletionRoutine),
        IrpContext->FatIoContext, TRUE, TRUE, TRUE);
        IoStack = IoGetNextIrpStackLocation(*RunIrp);
        IoStack->MajorFunction = IrpContext->Stack->MajorFunction;
        IoStack->Parameters.Read.Length = (ULONG) RunLength;
        IoStack->Parameters.Read.ByteOffset.QuadPart = Lbo;

        /*
        * Propagate write-through to the associated IRPs
        */
        if (FlagOn(IrpContext->Flags, IRPCONTEXT_WRITETHROUGH))
            SetFlag(IoStack->Flags, SL_WRITE_THROUGH);
        /*
        * Prepare for next iteration:
        */
    #if DBG
        RunExists =
    #endif
        FsRtlGetNextLargeMcbEntry(&Fcb->Mcb, CleanupIndex, &Vbo, &Lbo, &RunLength);
        ASSERT(RunExists);
        ASSERT(NextVbo == Vbo);
    }
   /*
    * Send all IRPs to the volume device, we don't need to check
    * status code because cleanup will be done
    * by the completion routine in any case.
    */
    for (RunIrp = &IrpContext->FatIoContext->Irp,
        CleanupIndex = FirstIndex;
        CleanupIndex < BeyoundLastIndex;
        CleanupIndex ++, RunIrp ++)
    {
        IoCallDriver(IrpContext->Vcb->TargetDeviceObject, *RunIrp);
    }

FatIoPerformNonCachedComplete:
    if (CanWait)
    {
        KeWaitForSingleObject(
            &IrpContext->FatIoContext->Wait.SyncEvent,
            Executive, KernelMode, FALSE, NULL);
        return IrpContext->Irp->IoStatus.Status;
    }
    SetFlag(IrpContext->Flags, IRPCONTEXT_STACK_IO_CONTEXT);
    return STATUS_PENDING;
   /*
    * The following block of code implements unwind logic
    */
FatIoPerformNonCachedCleanup:
    if (IrpContext->FatIoContext != NULL)
    {
        RunIrp = &IrpContext->FatIoContext->Irp;
        while (FirstIndex < CleanupIndex)
        {
            if ((*RunIrp)->MdlAddress != NULL)
                IoFreeMdl((*RunIrp)->MdlAddress);
            IoFreeIrp(*RunIrp);
            FirstIndex ++;
            RunIrp ++;
        }
        ExFreePool(IrpContext->FatIoContext);
        IrpContext->FatIoContext = NULL;
    }
    return Status;
}

/* EOF */
