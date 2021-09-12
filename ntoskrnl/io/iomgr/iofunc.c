/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/iofunc.c
 * PURPOSE:         Generic I/O Functions that build IRPs for various operations
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gunnar Dalsnes
 *                  Filip Navara (navaraf@reactos.org)
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <ioevent.h>
#define NDEBUG
#include <debug.h>
#include "internal/io_i.h"

volatile LONG IoPageReadIrpAllocationFailure = 0;
volatile LONG IoPageReadNonPagefileIrpAllocationFailure = 0;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
IopCleanupAfterException(IN PFILE_OBJECT FileObject,
                         IN PIRP Irp OPTIONAL,
                         IN PKEVENT Event OPTIONAL,
                         IN PKEVENT LocalEvent OPTIONAL)
{
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "IRP: %p. FO: %p\n", Irp, FileObject);

    if (Irp)
    {
        /* Check if we had a buffer */
        if (Irp->AssociatedIrp.SystemBuffer)
        {
            /* Free it */
            ExFreePool(Irp->AssociatedIrp.SystemBuffer);
        }

        /* Free the mdl */
        if (Irp->MdlAddress) IoFreeMdl(Irp->MdlAddress);

        /* Free the IRP */
        IoFreeIrp(Irp);
    }

    /* Check if we had a file lock */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Release it */
        IopUnlockFileObject(FileObject);
    }

    /* Check if we had an event */
    if (Event) ObDereferenceObject(Event);

    /* Check if we had a local event */
    if (LocalEvent) ExFreePool(LocalEvent);

    /* Derefenrce the FO */
    ObDereferenceObject(FileObject);
}

NTSTATUS
NTAPI
IopFinalizeAsynchronousIo(IN NTSTATUS SynchStatus,
                          IN PKEVENT Event,
                          IN PIRP Irp,
                          IN KPROCESSOR_MODE PreviousMode,
                          IN PIO_STATUS_BLOCK KernelIosb,
                          OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    NTSTATUS FinalStatus = SynchStatus;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "IRP: %p. Status: %lx\n", Irp, SynchStatus);

    /* Make sure the IRP was completed, but returned pending */
    if (FinalStatus == STATUS_PENDING)
    {
        /* Wait for the IRP */
        FinalStatus = KeWaitForSingleObject(Event,
                                            Executive,
                                            PreviousMode,
                                            FALSE,
                                            NULL);
        if (FinalStatus == STATUS_USER_APC)
        {
            /* Abort the request */
            IopAbortInterruptedIrp(Event, Irp);
        }

        /* Set the final status */
        FinalStatus = KernelIosb->Status;
    }

    /* Wrap potential user-mode write in SEH */
    _SEH2_TRY
    {
        *IoStatusBlock = *KernelIosb;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the exception code */
        FinalStatus = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Free the event and return status */
    ExFreePool(Event);
    return FinalStatus;
}

NTSTATUS
NTAPI
IopPerformSynchronousRequest(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp,
                             IN PFILE_OBJECT FileObject,
                             IN BOOLEAN Deferred,
                             IN KPROCESSOR_MODE PreviousMode,
                             IN BOOLEAN SynchIo,
                             IN IOP_TRANSFER_TYPE TransferType)
{
    NTSTATUS Status;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext = NULL;
    KIRQL OldIrql;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "IRP: %p. DO: %p. FO: %p\n",
            Irp, DeviceObject, FileObject);

    /* Queue the IRP */
    IopQueueIrpToThread(Irp);

    /* Update operation counts */
    IopUpdateOperationCount(TransferType);

    /* Call the driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Check if we're optimizing this case */
    if (Deferred)
    {
        /* We are! Check if the IRP wasn't completed */
        if (Status != STATUS_PENDING)
        {
            /* Complete it ourselves */
            ASSERT(!Irp->PendingReturned);
            KeRaiseIrql(APC_LEVEL, &OldIrql);
            IopCompleteRequest(&Irp->Tail.Apc,
                               &NormalRoutine,
                               &NormalContext,
                               (PVOID*)&FileObject,
                               &NormalContext);
            KeLowerIrql(OldIrql);
        }
    }

    /* Check if this was synch I/O */
    if (SynchIo)
    {
        /* Make sure the IRP was completed, but returned pending */
        if (Status == STATUS_PENDING)
        {
            /* Wait for the IRP */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           PreviousMode,
                                           (FileObject->Flags &
                                            FO_ALERTABLE_IO) != 0,
                                           NULL);
            if ((Status == STATUS_ALERTED) || (Status == STATUS_USER_APC))
            {
                /* Abort the request */
                IopAbortInterruptedIrp(&FileObject->Event, Irp);
            }

            /* Set the final status */
            Status = FileObject->FinalStatus;
        }

        /* Release the file lock */
        IopUnlockFileObject(FileObject);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
IopDeviceFsIoControl(IN HANDLE DeviceHandle,
                     IN HANDLE Event OPTIONAL,
                     IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
                     IN PVOID UserApcContext OPTIONAL,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN ULONG IoControlCode,
                     IN PVOID InputBuffer,
                     IN ULONG InputBufferLength OPTIONAL,
                     OUT PVOID OutputBuffer,
                     IN ULONG OutputBufferLength OPTIONAL,
                     IN BOOLEAN IsDevIoCtl)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PKEVENT EventObject = NULL;
    BOOLEAN LockedForSynch = FALSE;
    ULONG AccessType;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    ACCESS_MASK DesiredAccess;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ULONG BufferLength;
    POOL_TYPE PoolType;

    PAGED_CODE();

    IOTRACE(IO_CTL_DEBUG, "Handle: %p. CTL: %lx. Type: %lx\n",
            DeviceHandle, IoControlCode, IsDevIoCtl);

    /* Get the access type */
    AccessType = IO_METHOD_FROM_CTL_CODE(IoControlCode);

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Check if this is buffered I/O */
            if (AccessType == METHOD_BUFFERED)
            {
                /* Check if we have an output buffer */
                if (OutputBuffer)
                {
                    /* Probe the output buffer */
                    ProbeForWrite(OutputBuffer,
                                  OutputBufferLength,
                                  sizeof(CHAR));
                }
                else
                {
                    /* Make sure the caller can't fake this as we depend on this */
                    OutputBufferLength = 0;
                }
            }

            /* Check if we we have an input buffer I/O */
            if (AccessType != METHOD_NEITHER)
            {
                /* Check if we have an input buffer */
                if (InputBuffer)
                {
                    /* Probe the input buffer */
                    ProbeForRead(InputBuffer, InputBufferLength, sizeof(CHAR));
                }
                else
                {
                    /* Make sure the caller can't fake this as we depend on this */
                    InputBufferLength = 0;
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Don't check for access rights right now, KernelMode can do anything */
    Status = ObReferenceObjectByHandle(DeviceHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Can't use an I/O completion port and an APC in the same time */
    if ((FileObject->CompletionContext) && (UserApcRoutine))
    {
        /* Fail */
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Get the access mask */
        DesiredAccess = (ACCESS_MASK)((IoControlCode >> 14) & 3);

        /* Check if we can open it */
        if ((DesiredAccess != FILE_ANY_ACCESS) &&
            (HandleInformation.GrantedAccess & DesiredAccess) != DesiredAccess)
        {
            /* Dereference the file object and fail */
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }
    }

    /* Check for an event */
    if (Event)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(Event,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID*)&EventObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Dereference the file object and fail */
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Clear it */
        KeClearEvent(EventObject);
    }

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            if (EventObject) ObDereferenceObject(EventObject);
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Remember to unlock later */
        LockedForSynch = TRUE;
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        /* It's a direct open, get the attached device */
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        /* Otherwise get the related device */
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* If this is a device I/O, try to do it with FastIO path */
    if (IsDevIoCtl)
    {
        PFAST_IO_DISPATCH FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

        /* Check whether FSD is FastIO aware and provide an appropriate routine */
        if (FastIoDispatch != NULL && FastIoDispatch->FastIoDeviceControl != NULL)
        {
            IO_STATUS_BLOCK KernelIosb;

            /* If we have an output buffer coming from usermode */
            if (PreviousMode != KernelMode && OutputBuffer != NULL)
            {
                /* Probe it according to its usage */
                _SEH2_TRY
                {
                    if (AccessType == METHOD_IN_DIRECT)
                    {
                        ProbeForRead(OutputBuffer, OutputBufferLength, sizeof(CHAR));
                    }
                    else if (AccessType == METHOD_OUT_DIRECT)
                    {
                        ProbeForWrite(OutputBuffer, OutputBufferLength, sizeof(CHAR));
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Cleanup after exception and return */
                    IopCleanupAfterException(FileObject, NULL, EventObject, NULL);

                    /* Return the exception code */
                    _SEH2_YIELD(return _SEH2_GetExceptionCode());
                }
                _SEH2_END;
            }

            /* If we are dismounting a volume, increase the dismount count */
            if (IoControlCode == FSCTL_DISMOUNT_VOLUME)
            {
                InterlockedIncrement((PLONG)&SharedUserData->DismountCount);
            }

            /* Call the FSD */
            if (FastIoDispatch->FastIoDeviceControl(FileObject,
                                                    TRUE,
                                                    InputBuffer,
                                                    InputBufferLength,
                                                    OutputBuffer,
                                                    OutputBufferLength,
                                                    IoControlCode,
                                                    &KernelIosb,
                                                    DeviceObject))
            {
                IO_COMPLETION_CONTEXT CompletionInfo = { NULL, NULL };

                /* Write the IOSB back */
                _SEH2_TRY
                {
                    *IoStatusBlock = KernelIosb;

                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    KernelIosb.Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                /* Backup our complete context in case it exists */
                if (FileObject->CompletionContext)
                {
                    CompletionInfo = *(FileObject->CompletionContext);
                }

                /* If we had an event, signal it */
                if (Event)
                {
                    KeSetEvent(EventObject, IO_NO_INCREMENT, FALSE);
                    ObDereferenceObject(EventObject);
                }

                /* If FO was locked, unlock it */
                if (LockedForSynch)
                {
                    IopUnlockFileObject(FileObject);
                }

                /* Set completion if required */
                if (CompletionInfo.Port != NULL && UserApcContext != NULL)
                {
                    if (!NT_SUCCESS(IoSetIoCompletion(CompletionInfo.Port,
                                                      CompletionInfo.Key,
                                                      UserApcContext,
                                                      KernelIosb.Status,
                                                      KernelIosb.Information,
                                                      TRUE)))
                    {
                        KernelIosb.Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                /* We're done with FastIO! */
                ObDereferenceObject(FileObject);
                return KernelIosb.Status;
            }
        }
    }

    /* Clear the event */
    KeClearEvent(&FileObject->Event);

    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, EventObject, NULL);

    /* Setup the IRP */
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = EventObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = UserApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = UserApcContext;
    Irp->Cancel = FALSE;
    Irp->CancelRoutine = NULL;
    Irp->PendingReturned = FALSE;
    Irp->RequestorMode = PreviousMode;
    Irp->MdlAddress = NULL;
    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->Flags = 0;
    Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set stack location settings */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->MajorFunction = IsDevIoCtl ?
                              IRP_MJ_DEVICE_CONTROL :
                              IRP_MJ_FILE_SYSTEM_CONTROL;
    StackPtr->MinorFunction = 0; /* Minor function 0 is IRP_MN_USER_FS_REQUEST */
    StackPtr->Control = 0;
    StackPtr->Flags = 0;
    StackPtr->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    /* Set the IOCTL Data */
    StackPtr->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    StackPtr->Parameters.DeviceIoControl.OutputBufferLength =
        OutputBufferLength;

    PoolType = IsDevIoCtl ? NonPagedPoolCacheAligned : NonPagedPool;

    /* Handle the Methods */
    switch (AccessType)
    {
        /* Buffered I/O */
        case METHOD_BUFFERED:

            /* Enter SEH for allocations */
            _SEH2_TRY
            {
                /* Select the right Buffer Length */
                BufferLength = (InputBufferLength > OutputBufferLength) ?
                                InputBufferLength : OutputBufferLength;

                /* Make sure there is one */
                if (BufferLength)
                {
                    /* Allocate the System Buffer */
                    Irp->AssociatedIrp.SystemBuffer =
                        ExAllocatePoolWithQuotaTag(PoolType,
                                                   BufferLength,
                                                   TAG_SYS_BUF);

                    /* Check if we got a buffer */
                    if (InputBuffer)
                    {
                        /* Copy into the System Buffer */
                        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                                      InputBuffer,
                                      InputBufferLength);
                    }

                    /* Write the flags */
                    Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
                    if (OutputBuffer) Irp->Flags |= IRP_INPUT_OPERATION;

                    /* Save the Buffer */
                    Irp->UserBuffer = OutputBuffer;
                }
                else
                {
                    /* Clear the Flags and Buffer */
                    Irp->UserBuffer = NULL;
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Cleanup after exception and return */
                IopCleanupAfterException(FileObject, Irp, EventObject, NULL);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;
            break;

        /* Direct I/O */
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:

            /* Enter SEH */
            _SEH2_TRY
            {
                /* Check if we got an input buffer */
                if ((InputBufferLength) && (InputBuffer))
                {
                    /* Allocate the System Buffer */
                    Irp->AssociatedIrp.SystemBuffer =
                        ExAllocatePoolWithQuotaTag(PoolType,
                                                   InputBufferLength,
                                                   TAG_SYS_BUF);

                    /* Copy into the System Buffer */
                    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                                  InputBuffer,
                                  InputBufferLength);

                    /* Write the flags */
                    Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
                }

                /* Check if we got an output buffer */
                if (OutputBufferLength)
                {
                    /* Allocate the System Buffer */
                    Irp->MdlAddress = IoAllocateMdl(OutputBuffer,
                                                    OutputBufferLength,
                                                    FALSE,
                                                    FALSE,
                                                    Irp);
                    if (!Irp->MdlAddress)
                    {
                        /* Raise exception we'll catch */
                        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    /* Do the probe */
                    MmProbeAndLockPages(Irp->MdlAddress,
                                        PreviousMode,
                                        (AccessType == METHOD_IN_DIRECT) ?
                                        IoReadAccess : IoWriteAccess);
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Cleanup after exception and return */
                IopCleanupAfterException(FileObject, Irp, EventObject, NULL);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;
            break;

        case METHOD_NEITHER:

            /* Just save the Buffer */
            Irp->UserBuffer = OutputBuffer;
            StackPtr->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
    }

    /* Use deferred completion for FS I/O */
    if (!IsDevIoCtl)
    {
        Irp->Flags |= IRP_DEFER_IO_COMPLETION;
    }

    /* If we are dismounting a volume, increaase the dismount count */
    if (IoControlCode == FSCTL_DISMOUNT_VOLUME)
    {
        InterlockedIncrement((PLONG)&SharedUserData->DismountCount);
    }

    /* Perform the call */
    return IopPerformSynchronousRequest(DeviceObject,
                                        Irp,
                                        FileObject,
                                        !IsDevIoCtl,
                                        PreviousMode,
                                        LockedForSynch,
                                        IopOtherTransfer);
}

NTSTATUS
NTAPI
IopQueryDeviceInformation(IN PFILE_OBJECT FileObject,
                          IN ULONG InformationClass,
                          IN ULONG Length,
                          OUT PVOID Information,
                          OUT PULONG ReturnedLength,
                          IN BOOLEAN File)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    BOOLEAN LocalEvent = FALSE;
    KEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "Handle: %p. CTL: %lx. Type: %lx\n",
            FileObject, InformationClass, File);

    /* Reference the object */
    ObReferenceObject(FileObject);

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        (void)IopLockFileObject(FileObject, KernelMode);

        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, NULL);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = KernelMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->Flags |= IRP_BUFFERED_IO;
    Irp->AssociatedIrp.SystemBuffer = Information;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set the Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = File ? IRP_MJ_QUERY_INFORMATION:
                                     IRP_MJ_QUERY_VOLUME_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Check which type this is */
    if (File)
    {
        /* Set Parameters */
        StackPtr->Parameters.QueryFile.FileInformationClass = InformationClass;
        StackPtr->Parameters.QueryFile.Length = Length;
    }
    else
    {
        /* Set Parameters */
        StackPtr->Parameters.QueryVolume.FsInformationClass = InformationClass;
        StackPtr->Parameters.QueryVolume.Length = Length;
    }

    /* Queue the IRP */
    IopQueueIrpToThread(Irp);

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Check if this was synch I/O */
    if (!LocalEvent)
    {
        /* Check if the request is pending */
        if (Status == STATUS_PENDING)
        {
            /* Wait on the file object */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           KernelMode,
                                           (FileObject->Flags &
                                            FO_ALERTABLE_IO) != 0,
                                           NULL);
            if (Status == STATUS_ALERTED)
            {
                /* Abort the operation */
                IopAbortInterruptedIrp(&FileObject->Event, Irp);
            }

            /* Get the final status */
            Status = FileObject->FinalStatus;
        }

        /* Release the file lock */
        IopUnlockFileObject(FileObject);
    }
    else if (Status == STATUS_PENDING)
    {
        /* Wait on the local event and get the final status */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    /* Return the Length and Status. ReturnedLength is NOT optional */
    *ReturnedLength = (ULONG)IoStatusBlock.Information;
    return Status;
}

NTSTATUS
NTAPI
IopGetFileInformation(IN PFILE_OBJECT FileObject,
                      IN ULONG Length,
                      IN FILE_INFORMATION_CLASS FileInfoClass,
                      OUT PVOID Buffer,
                      OUT PULONG ReturnedLength)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;

    PAGED_CODE();

    /* Allocate an IRP */
    ObReferenceObject(FileObject);
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (Irp == NULL)
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Init event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Setup the IRP */
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = &Event;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->RequestorMode = KernelMode;
    Irp->AssociatedIrp.SystemBuffer = Buffer;
    Irp->Flags = IRP_SYNCHRONOUS_API | IRP_BUFFERED_IO | IRP_OB_QUERY_NAME;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    Stack->FileObject = FileObject;
    Stack->Parameters.QueryFile.FileInformationClass = FileInfoClass;
    Stack->Parameters.QueryFile.Length = Length;


    /* Queue the IRP */
    IopQueueIrpToThread(Irp);

    /* Call the driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    *ReturnedLength = IoStatusBlock.Information;
    return Status;
}

NTSTATUS
NTAPI
IopGetBasicInformationFile(IN PFILE_OBJECT FileObject,
                           OUT PFILE_BASIC_INFORMATION BasicInfo)
{
    ULONG ReturnedLength;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;

    PAGED_CODE();

    /* Try to do it the fast way if possible */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (DeviceObject->DriverObject->FastIoDispatch != NULL &&
        DeviceObject->DriverObject->FastIoDispatch->FastIoQueryBasicInfo != NULL &&
        DeviceObject->DriverObject->FastIoDispatch->FastIoQueryBasicInfo(FileObject,
                                                                         ((FileObject->Flags & FO_SYNCHRONOUS_IO) != 0),
                                                                         BasicInfo,
                                                                         &IoStatusBlock,
                                                                         DeviceObject))
    {
        return IoStatusBlock.Status;
    }

    /* In case it failed, fall back to IRP-based method */
    return IopGetFileInformation(FileObject, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation, BasicInfo, &ReturnedLength);
}

NTSTATUS
NTAPI
IopOpenLinkOrRenameTarget(OUT PHANDLE Handle,
                          IN PIRP Irp,
                          IN PFILE_RENAME_INFORMATION RenameInfo,
                          IN PFILE_OBJECT FileObject)
{
    NTSTATUS Status;
    HANDLE TargetHandle;
    UNICODE_STRING FileName;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT TargetFileObject;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    ACCESS_MASK DesiredAccess = FILE_WRITE_DATA;

    PAGED_CODE();

    /* First, establish whether our target is a directory */
    if (!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN))
    {
        Status = IopGetBasicInformationFile(FileObject, &BasicInfo);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        if (BasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DesiredAccess = FILE_ADD_SUBDIRECTORY;
        }
    }

    /* Setup the string to the target */
    FileName.Buffer = RenameInfo->FileName;
    FileName.Length = RenameInfo->FileNameLength;
    FileName.MaximumLength = RenameInfo->FileNameLength;

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               (FileObject->Flags & FO_OPENED_CASE_SENSITIVE ? 0 : OBJ_CASE_INSENSITIVE) | OBJ_KERNEL_HANDLE,
                               RenameInfo->RootDirectory,
                               NULL);

    /* And open its parent directory
     * Use hint if specified
     */
    if (FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION)
    {
        PFILE_OBJECT_EXTENSION FileObjectExtension;

        ASSERT(!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN));

        FileObjectExtension = FileObject->FileObjectExtension;
        Status = IoCreateFileSpecifyDeviceObjectHint(&TargetHandle,
                                                     DesiredAccess | SYNCHRONIZE,
                                                     &ObjectAttributes,
                                                     &IoStatusBlock,
                                                     NULL,
                                                     0,
                                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                     FILE_OPEN,
                                                     FILE_OPEN_FOR_BACKUP_INTENT,
                                                     NULL,
                                                     0,
                                                     CreateFileTypeNone,
                                                     NULL,
                                                     IO_FORCE_ACCESS_CHECK | IO_OPEN_TARGET_DIRECTORY | IO_NO_PARAMETER_CHECKING,
                                                     FileObjectExtension->TopDeviceObjectHint);
    }
    else
    {
        Status = IoCreateFile(&TargetHandle,
                              DesiredAccess | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              0,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN,
                              FILE_OPEN_FOR_BACKUP_INTENT,
                              NULL,
                              0,
                              CreateFileTypeNone,
                              NULL,
                              IO_FORCE_ACCESS_CHECK | IO_OPEN_TARGET_DIRECTORY | IO_NO_PARAMETER_CHECKING);
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Once open, continue only if:
     * Target exists and we're allowed to overwrite it
     */
    Stack = IoGetNextIrpStackLocation(Irp);
    if (Stack->Parameters.SetFile.FileInformationClass == FileLinkInformation &&
        !RenameInfo->ReplaceIfExists &&
        IoStatusBlock.Information == FILE_EXISTS)
    {
        ObCloseHandle(TargetHandle, KernelMode);
        return STATUS_OBJECT_NAME_COLLISION;
    }

    /* Now, we'll get the associated device of the target, to check for same device location
     * So, get the FO first
     */
    Status = ObReferenceObjectByHandle(TargetHandle,
                                       FILE_WRITE_DATA,
                                       IoFileObjectType,
                                       KernelMode,
                                       (PVOID *)&TargetFileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status))
    {
        ObCloseHandle(TargetHandle, KernelMode);
        return Status;
    }

    /* We can dereference, we have the handle */
    ObDereferenceObject(TargetFileObject);
    /* If we're not on the same device, error out **/
    if (IoGetRelatedDeviceObject(TargetFileObject) != IoGetRelatedDeviceObject(FileObject))
    {
        ObCloseHandle(TargetHandle, KernelMode);
        return STATUS_NOT_SAME_DEVICE;
    }

    /* Return parent directory file object and handle */
    Stack->Parameters.SetFile.FileObject = TargetFileObject;
    *Handle = TargetHandle;

    return STATUS_SUCCESS;
}

static
ULONG
IopGetFileMode(IN PFILE_OBJECT FileObject)
{
    ULONG Mode = 0;

    if (FileObject->Flags & FO_WRITE_THROUGH)
        Mode |= FILE_WRITE_THROUGH;

    if (FileObject->Flags & FO_SEQUENTIAL_ONLY)
        Mode |= FILE_SEQUENTIAL_ONLY;

    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
        Mode |= FILE_NO_INTERMEDIATE_BUFFERING;

    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        if (FileObject->Flags & FO_ALERTABLE_IO)
            Mode |= FILE_SYNCHRONOUS_IO_ALERT;
        else
            Mode |= FILE_SYNCHRONOUS_IO_NONALERT;
    }

    if (FileObject->Flags & FO_DELETE_ON_CLOSE)
        Mode |= FILE_DELETE_ON_CLOSE;

    return Mode;
}

static
BOOLEAN
IopGetMountFlag(IN PDEVICE_OBJECT DeviceObject)
{
    KIRQL OldIrql;
    PVPB Vpb;
    BOOLEAN Mounted;

    /* Assume not mounted */
    Mounted = FALSE;

    /* Check whether we have the mount flag */
    IoAcquireVpbSpinLock(&OldIrql);

    Vpb = DeviceObject->Vpb;
    if (Vpb != NULL &&
        BooleanFlagOn(Vpb->Flags, VPB_MOUNTED))
    {
        Mounted = TRUE;
    }

    IoReleaseVpbSpinLock(OldIrql);

    return Mounted;
}

static
BOOLEAN
IopVerifyDriverObjectOnStack(IN PDEVICE_OBJECT DeviceObject,
                             IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT StackDO;

    /* Browse our whole device stack, trying to find the appropriate driver */
    StackDO = IopGetDeviceAttachmentBase(DeviceObject);
    while (StackDO != NULL)
    {
        /* We've found the driver, return success */
        if (StackDO->DriverObject == DriverObject)
        {
            return TRUE;
        }

        /* Move to the next */
        StackDO = StackDO->AttachedDevice;
    }

    /* We only reach there if driver was not found */
    return FALSE;
}

static
NTSTATUS
IopGetDriverPathInformation(IN PFILE_OBJECT FileObject,
                            IN PFILE_FS_DRIVER_PATH_INFORMATION DriverPathInfo,
                            IN ULONG Length)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    UNICODE_STRING DriverName;
    PDRIVER_OBJECT DriverObject;

    /* Make sure the structure is consistent (ie, driver name fits into the buffer) */
    if (Length - FIELD_OFFSET(FILE_FS_DRIVER_PATH_INFORMATION, DriverName) < DriverPathInfo->DriverNameLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Setup the whole driver name */
    DriverName.Length = DriverPathInfo->DriverNameLength;
    DriverName.MaximumLength = DriverPathInfo->DriverNameLength;
    DriverName.Buffer = &DriverPathInfo->DriverName[0];

    /* Ask Ob for such driver */
    Status = ObReferenceObjectByName(&DriverName,
                                     OBJ_CASE_INSENSITIVE,
                                     NULL,
                                     0,
                                     IoDriverObjectType,
                                     KernelMode,
                                     NULL,
                                     (PVOID*)&DriverObject);
    /* No such driver, bail out */
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Lock the devices database, we'll browse it */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);
    /* If we have a VPB, browse the stack from the volume */
    if (FileObject->Vpb != NULL && FileObject->Vpb->DeviceObject != NULL)
    {
        DriverPathInfo->DriverInPath = IopVerifyDriverObjectOnStack(FileObject->Vpb->DeviceObject, DriverObject);
    }
    /* Otherwise, do it from the normal device */
    else
    {
        DriverPathInfo->DriverInPath = IopVerifyDriverObjectOnStack(FileObject->DeviceObject, DriverObject);
    }
    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);

    /* No longer needed */
    ObDereferenceObject(DriverObject);

    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoSynchronousPageWrite(IN PFILE_OBJECT FileObject,
                       IN PMDL Mdl,
                       IN PLARGE_INTEGER Offset,
                       IN PKEVENT Event,
                       IN PIO_STATUS_BLOCK StatusBlock)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    IOTRACE(IO_API_DEBUG, "FileObject: %p. Mdl: %p. Offset: %p\n",
            FileObject, Mdl, Offset);

    /* Is the write originating from Cc? */
    if (FileObject->SectionObjectPointer != NULL &&
        FileObject->SectionObjectPointer->SharedCacheMap != NULL)
    {
        ++CcDataFlushes;
        CcDataPages += BYTES_TO_PAGES(MmGetMdlByteCount(Mdl));
    }

    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);

    /* Create the IRP Settings */
    Irp->MdlAddress = Mdl;
    Irp->UserBuffer = MmGetMdlVirtualAddress(Mdl);
    Irp->UserIosb = StatusBlock;
    Irp->UserEvent = Event;
    Irp->RequestorMode = KernelMode;
    Irp->Flags = IRP_PAGING_IO | IRP_NOCACHE | IRP_SYNCHRONOUS_PAGING_IO;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set the Stack Settings */
    StackPtr->Parameters.Write.Length = MmGetMdlByteCount(Mdl);
    StackPtr->Parameters.Write.ByteOffset = *Offset;
    StackPtr->MajorFunction = IRP_MJ_WRITE;
    StackPtr->FileObject = FileObject;

    /* Call the Driver */
    return IoCallDriver(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoPageRead(IN PFILE_OBJECT FileObject,
           IN PMDL Mdl,
           IN PLARGE_INTEGER Offset,
           IN PKEVENT Event,
           IN PIO_STATUS_BLOCK StatusBlock)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    IOTRACE(IO_API_DEBUG, "FileObject: %p. Mdl: %p. Offset: %p\n",
            FileObject, Mdl, Offset);

    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    /* If allocation failed, try to see whether we can use
     * the reserve IRP
     */
    if (Irp == NULL)
    {
        /* We will use it only for paging file */
        if (MmIsFileObjectAPagingFile(FileObject))
        {
            InterlockedExchangeAdd(&IoPageReadIrpAllocationFailure, 1);
            Irp = IopAllocateReserveIrp(DeviceObject->StackSize);
        }
        else
        {
            InterlockedExchangeAdd(&IoPageReadNonPagefileIrpAllocationFailure, 1);
        }

        /* If allocation failed (not a paging file or too big stack size)
         * Fail for real
         */
        if (Irp == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* Get the Stack */
    StackPtr = IoGetNextIrpStackLocation(Irp);

    /* Create the IRP Settings */
    Irp->MdlAddress = Mdl;
    Irp->UserBuffer = MmGetMdlVirtualAddress(Mdl);
    Irp->UserIosb = StatusBlock;
    Irp->UserEvent = Event;
    Irp->RequestorMode = KernelMode;
    Irp->Flags = IRP_PAGING_IO |
                 IRP_NOCACHE |
                 IRP_SYNCHRONOUS_PAGING_IO |
                 IRP_INPUT_OPERATION;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set the Stack Settings */
    StackPtr->Parameters.Read.Length = MmGetMdlByteCount(Mdl);
    StackPtr->Parameters.Read.ByteOffset = *Offset;
    StackPtr->MajorFunction = IRP_MJ_READ;
    StackPtr->FileObject = FileObject;

    /* Call the Driver */
    return IoCallDriver(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoQueryFileInformation(IN PFILE_OBJECT FileObject,
                       IN FILE_INFORMATION_CLASS FileInformationClass,
                       IN ULONG Length,
                       OUT PVOID FileInformation,
                       OUT PULONG ReturnedLength)
{
    /* Call the shared routine */
    return IopQueryDeviceInformation(FileObject,
                                     FileInformationClass,
                                     Length,
                                     FileInformation,
                                     ReturnedLength,
                                     TRUE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoQueryVolumeInformation(IN PFILE_OBJECT FileObject,
                         IN FS_INFORMATION_CLASS FsInformationClass,
                         IN ULONG Length,
                         OUT PVOID FsInformation,
                         OUT PULONG ReturnedLength)
{
    /* Call the shared routine */
    return IopQueryDeviceInformation(FileObject,
                                     FsInformationClass,
                                     Length,
                                     FsInformation,
                                     ReturnedLength,
                                     FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoSetInformation(IN PFILE_OBJECT FileObject,
                 IN FILE_INFORMATION_CLASS FileInformationClass,
                 IN ULONG Length,
                 IN PVOID FileInformation)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    BOOLEAN LocalEvent = FALSE;
    KEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileObject: %p. Class: %lx. Length: %lx\n",
            FileObject, FileInformationClass, Length);

    /* Reference the object */
    ObReferenceObject(FileObject);

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        (void)IopLockFileObject(FileObject, KernelMode);

        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, NULL);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = KernelMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->Flags |= IRP_BUFFERED_IO;
    Irp->AssociatedIrp.SystemBuffer = FileInformation;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set the Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_SET_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Set Parameters */
    StackPtr->Parameters.SetFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.SetFile.Length = Length;

    /* Queue the IRP */
    IopQueueIrpToThread(Irp);

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Check if this was synch I/O */
    if (!LocalEvent)
    {
        /* Check if the request is pending */
        if (Status == STATUS_PENDING)
        {
            /* Wait on the file object */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           KernelMode,
                                           (FileObject->Flags &
                                            FO_ALERTABLE_IO) != 0,
                                           NULL);
            if (Status == STATUS_ALERTED)
            {
                /* Abort the operation */
                IopAbortInterruptedIrp(&FileObject->Event, Irp);
            }

            /* Get the final status */
            Status = FileObject->FinalStatus;
        }

        /* Release the file lock */
        IopUnlockFileObject(FileObject);
    }
    else if (Status == STATUS_PENDING)
    {
        /* Wait on the local event and get the final status */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    /* Return the status */
    return Status;
}

/* NATIVE SERVICES ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtDeviceIoControlFile(IN HANDLE DeviceHandle,
                      IN HANDLE Event OPTIONAL,
                      IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
                      IN PVOID UserApcContext OPTIONAL,
                      OUT PIO_STATUS_BLOCK IoStatusBlock,
                      IN ULONG IoControlCode,
                      IN PVOID InputBuffer,
                      IN ULONG InputBufferLength OPTIONAL,
                      OUT PVOID OutputBuffer,
                      IN ULONG OutputBufferLength OPTIONAL)
{
    /* Call the Generic Function */
    return IopDeviceFsIoControl(DeviceHandle,
                                Event,
                                UserApcRoutine,
                                UserApcContext,
                                IoStatusBlock,
                                IoControlCode,
                                InputBuffer,
                                InputBufferLength,
                                OutputBuffer,
                                OutputBufferLength,
                                TRUE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtFsControlFile(IN HANDLE DeviceHandle,
                IN HANDLE Event OPTIONAL,
                IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
                IN PVOID UserApcContext OPTIONAL,
                OUT PIO_STATUS_BLOCK IoStatusBlock,
                IN ULONG IoControlCode,
                IN PVOID InputBuffer,
                IN ULONG InputBufferLength OPTIONAL,
                OUT PVOID OutputBuffer,
                IN ULONG OutputBufferLength OPTIONAL)
{
    /* Call the Generic Function */
    return IopDeviceFsIoControl(DeviceHandle,
                                Event,
                                UserApcRoutine,
                                UserApcContext,
                                IoStatusBlock,
                                IoControlCode,
                                InputBuffer,
                                InputBufferLength,
                                OutputBuffer,
                                OutputBufferLength,
                                FALSE);
}

NTSTATUS
NTAPI
NtFlushBuffersFile(IN HANDLE FileHandle,
                   OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    OBJECT_HANDLE_INFORMATION ObjectHandleInfo;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    IO_STATUS_BLOCK KernelIosb;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    if (PreviousMode != KernelMode)
    {
        /* Protect probes */
        _SEH2_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Get the File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &ObjectHandleInfo);
    if (!NT_SUCCESS(Status)) return Status;

    /*
     * Check if the handle has either FILE_WRITE_DATA or FILE_APPEND_DATA was
     * granted. However, if this is a named pipe, make sure we don't ask for
     * FILE_APPEND_DATA as it interferes with the FILE_CREATE_PIPE_INSTANCE
     * access right!
     */
    if (!(ObjectHandleInfo.GrantedAccess &
         ((!(FileObject->Flags & FO_NAMED_PIPE) ? FILE_APPEND_DATA : 0) |
         FILE_WRITE_DATA)))
    {
        /* We failed */
        ObDereferenceObject(FileObject);
        return STATUS_ACCESS_DENIED;
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
        if (!Event)
        {
            /* We failed */
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeEvent(Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Clear the event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, Event);

    /* Set up the IRP */
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->UserIosb = (LocalEvent) ? &KernelIosb : IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? Event : NULL;
    Irp->RequestorMode = PreviousMode;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    StackPtr->FileObject = FileObject;

    /* Call the Driver */
    Status = IopPerformSynchronousRequest(DeviceObject,
                                          Irp,
                                          FileObject,
                                          FALSE,
                                          PreviousMode,
                                          !LocalEvent,
                                          IopOtherTransfer);

    /* Check if this was async I/O */
    if (LocalEvent)
    {
        /* It was, finalize this request */
        Status = IopFinalizeAsynchronousIo(Status,
                                           Event,
                                           Irp,
                                           PreviousMode,
                                           &KernelIosb,
                                           IoStatusBlock);
    }

    /* Return the Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtNotifyChangeDirectoryFile(IN HANDLE FileHandle,
                            IN HANDLE EventHandle OPTIONAL,
                            IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                            IN PVOID ApcContext OPTIONAL,
                            OUT PIO_STATUS_BLOCK IoStatusBlock,
                            OUT PVOID Buffer,
                            IN ULONG BufferSize,
                            IN ULONG CompletionFilter,
                            IN BOOLEAN WatchTree)
{
    PIRP Irp;
    PKEVENT Event = NULL;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IoStack;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    BOOLEAN LockedForSync = FALSE;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O STatus block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the buffer */
            if (BufferSize) ProbeForWrite(Buffer, BufferSize, sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        /* Check if CompletionFilter is valid */
        if (!CompletionFilter || (CompletionFilter & ~FILE_NOTIFY_VALID_MASK))
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_LIST_DIRECTORY,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we have an event handle */
    if (EventHandle)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID *)&Event,
                                           NULL);
        if (Status != STATUS_SUCCESS)
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
        KeClearEvent(Event);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            if (Event) ObDereferenceObject(Event);
            ObDereferenceObject(FileObject);
            return Status;
        }
        LockedForSync = TRUE;
    }

    /* Clear File Object event */
    KeClearEvent(&FileObject->Event);

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, Event, NULL);

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;
    Irp->UserBuffer = Buffer;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    /* Set up Stack Data */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    IoStack->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
    IoStack->FileObject = FileObject;

    /* Set parameters */
    IoStack->Parameters.NotifyDirectory.CompletionFilter = CompletionFilter;
    IoStack->Parameters.NotifyDirectory.Length = BufferSize;
    if (WatchTree) IoStack->Flags = SL_WATCH_TREE;

    /* Perform the call */
    return IopPerformSynchronousRequest(DeviceObject,
                                        Irp,
                                        FileObject,
                                        FALSE,
                                        PreviousMode,
                                        LockedForSync,
                                        IopOtherTransfer);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtLockFile(IN HANDLE FileHandle,
           IN HANDLE EventHandle OPTIONAL,
           IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
           IN PVOID ApcContext OPTIONAL,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           IN PLARGE_INTEGER ByteOffset,
           IN PLARGE_INTEGER Length,
           IN ULONG  Key,
           IN BOOLEAN FailImmediately,
           IN BOOLEAN ExclusiveLock)
{
    PFILE_OBJECT FileObject;
    PLARGE_INTEGER LocalLength = NULL;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event = NULL;
    BOOLEAN LockedForSync = FALSE;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    LARGE_INTEGER CapturedByteOffset, CapturedLength;
    NTSTATUS Status;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    PFAST_IO_DISPATCH FastIoDispatch;
    PAGED_CODE();
    CapturedByteOffset.QuadPart = 0;
    CapturedLength.QuadPart = 0;
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Must have either FILE_READ_DATA or FILE_WRITE_DATA access */
        if (!(HandleInformation.GrantedAccess &
            (FILE_WRITE_DATA | FILE_READ_DATA)))
        {
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }

        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O STatus block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe and capture the large integers */
            CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            CapturedLength = ProbeForReadLargeInteger(Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Dereference the object and return exception code */
            ObDereferenceObject(FileObject);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Otherwise, capture them directly */
        CapturedByteOffset = *ByteOffset;
        CapturedLength = *Length;
    }

    /* Check if we have an event handle */
    if (EventHandle)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID *)&Event,
                                           NULL);
        if (Status != STATUS_SUCCESS) return Status;
        KeClearEvent(Event);
    }

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Try to do it the FastIO way if possible */
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;
    if (FastIoDispatch != NULL && FastIoDispatch->FastIoLock != NULL)
    {
        IO_STATUS_BLOCK KernelIosb;

        if (FastIoDispatch->FastIoLock(FileObject,
                                       &CapturedByteOffset,
                                       &CapturedLength,
                                       PsGetCurrentProcess(),
                                       Key,
                                       FailImmediately,
                                       ExclusiveLock,
                                       &KernelIosb,
                                       DeviceObject))
        {
            /* Write the IOSB back */
            _SEH2_TRY
            {
                *IoStatusBlock = KernelIosb;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                KernelIosb.Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* If we had an event, signal it */
            if (EventHandle)
            {
                KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
                ObDereferenceObject(Event);
            }

            /* Set completion if required */
            if (FileObject->CompletionContext != NULL && ApcContext != NULL)
            {
                if (!NT_SUCCESS(IoSetIoCompletion(FileObject->CompletionContext->Port,
                                                  FileObject->CompletionContext->Key,
                                                  ApcContext,
                                                  KernelIosb.Status,
                                                  KernelIosb.Information,
                                                  TRUE)))
                {
                    KernelIosb.Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            FileObject->LockOperation = TRUE;

            /* We're done with FastIO! */
            ObDereferenceObject(FileObject);
            return KernelIosb.Status;
        }
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            if (Event) ObDereferenceObject(Event);
            ObDereferenceObject(FileObject);
            return Status;
        }
        LockedForSync = TRUE;
    }

    /* Clear File Object event */
    KeClearEvent(&FileObject->Event);
    FileObject->LockOperation = TRUE;

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, Event, NULL);

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_LOCK_CONTROL;
    StackPtr->MinorFunction = IRP_MN_LOCK;
    StackPtr->FileObject = FileObject;

    /* Allocate local buffer */
    LocalLength = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(LARGE_INTEGER),
                                        TAG_LOCK);
    if (!LocalLength)
    {
        /* Allocating failed, clean up and return failure */
        IopCleanupAfterException(FileObject, Irp, Event, NULL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set the length */
    *LocalLength = CapturedLength;
    Irp->Tail.Overlay.AuxiliaryBuffer = (PVOID)LocalLength;
    StackPtr->Parameters.LockControl.Length = LocalLength;

    /* Set Parameters */
    StackPtr->Parameters.LockControl.ByteOffset = CapturedByteOffset;
    StackPtr->Parameters.LockControl.Key = Key;

    /* Set Flags */
    if (FailImmediately) StackPtr->Flags = SL_FAIL_IMMEDIATELY;
    if (ExclusiveLock) StackPtr->Flags |= SL_EXCLUSIVE_LOCK;

    /* Perform the call */
    return IopPerformSynchronousRequest(DeviceObject,
                                        Irp,
                                        FileObject,
                                        FALSE,
                                        PreviousMode,
                                        LockedForSync,
                                        IopOtherTransfer);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryDirectoryFile(IN HANDLE FileHandle,
                     IN HANDLE EventHandle OPTIONAL,
                     IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                     IN PVOID ApcContext OPTIONAL,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     OUT PVOID FileInformation,
                     IN ULONG Length,
                     IN FILE_INFORMATION_CLASS FileInformationClass,
                     IN BOOLEAN ReturnSingleEntry,
                     IN PUNICODE_STRING FileName OPTIONAL,
                     IN BOOLEAN RestartScan)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    BOOLEAN LockedForSynch = FALSE;
    PKEVENT Event = NULL;
    volatile PVOID AuxBuffer = NULL;
    PMDL Mdl;
    UNICODE_STRING CapturedFileName;
    PUNICODE_STRING SearchPattern;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O Status Block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the file information */
            ProbeForWrite(FileInformation, Length, sizeof(ULONG));

            /* Check if we have a file name */
            if (FileName)
            {
                /* Capture it */
                CapturedFileName = ProbeForReadUnicodeString(FileName);
                if (CapturedFileName.Length)
                {
                    /* Probe its buffer */
                    ProbeForRead(CapturedFileName.Buffer,
                                 CapturedFileName.Length,
                                 1);
                }

                /* Allocate the auxiliary buffer */
                AuxBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                  CapturedFileName.Length +
                                                  sizeof(UNICODE_STRING),
                                                  TAG_SYSB);
                RtlCopyMemory((PVOID)((ULONG_PTR)AuxBuffer +
                                      sizeof(UNICODE_STRING)),
                              CapturedFileName.Buffer,
                              CapturedFileName.Length);

                /* Setup the search pattern */
                SearchPattern = (PUNICODE_STRING)AuxBuffer;
                SearchPattern->Buffer = (PWCHAR)((ULONG_PTR)AuxBuffer +
                                                 sizeof(UNICODE_STRING));
                SearchPattern->Length = CapturedFileName.Length;
                SearchPattern->MaximumLength = CapturedFileName.Length;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Free buffer and return the exception code */
            if (AuxBuffer) ExFreePoolWithTag(AuxBuffer, TAG_SYSB);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Check input parameters */

    switch (FileInformationClass)
    {
#define CHECK_LENGTH(class, struct)                      \
        case class:                                 \
            if (Length < sizeof(struct))                         \
                return STATUS_INFO_LENGTH_MISMATCH; \
            break
        CHECK_LENGTH(FileDirectoryInformation, FILE_DIRECTORY_INFORMATION);
        CHECK_LENGTH(FileFullDirectoryInformation, FILE_FULL_DIR_INFORMATION);
        CHECK_LENGTH(FileIdFullDirectoryInformation, FILE_ID_FULL_DIR_INFORMATION);
        CHECK_LENGTH(FileNamesInformation, FILE_NAMES_INFORMATION);
        CHECK_LENGTH(FileBothDirectoryInformation, FILE_BOTH_DIR_INFORMATION);
        CHECK_LENGTH(FileIdBothDirectoryInformation, FILE_ID_BOTH_DIR_INFORMATION);
        default:
            break;
#undef CHECK_LENGTH
    }

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_LIST_DIRECTORY,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        if (AuxBuffer) ExFreePoolWithTag(AuxBuffer, TAG_SYSB);
        return Status;
    }

    /* Are there two associated completion routines? */
    if (FileObject->CompletionContext != NULL && ApcRoutine != NULL)
    {
        ObDereferenceObject(FileObject);
        if (AuxBuffer) ExFreePoolWithTag(AuxBuffer, TAG_SYSB);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we have an even handle */
    if (EventHandle)
    {
        /* Get its pointer */
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID *)&Event,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            if (AuxBuffer) ExFreePoolWithTag(AuxBuffer, TAG_SYSB);
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Clear it */
        KeClearEvent(Event);
    }

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            if (Event) ObDereferenceObject(Event);
            ObDereferenceObject(FileObject);
            if (AuxBuffer) ExFreePoolWithTag(AuxBuffer, TAG_SYSB);
            return Status;
        }

        /* Remember to unlock later */
        LockedForSynch = TRUE;
    }

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Clear the File Object's event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, EventHandle, AuxBuffer);

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
    Irp->MdlAddress = NULL;
    Irp->Tail.Overlay.AuxiliaryBuffer = AuxBuffer;
    Irp->AssociatedIrp.SystemBuffer = NULL;

    /* Check if this is buffered I/O */
    if (DeviceObject->Flags & DO_BUFFERED_IO)
    {
        /* Allocate a buffer */
        Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                                Length,
                                                                TAG_SYSB);
        if (!Irp->AssociatedIrp.SystemBuffer)
        {
            /* Allocating failed, clean up and return the exception code */
            IopCleanupAfterException(FileObject, Irp, Event, NULL);
            if (AuxBuffer) ExFreePoolWithTag(AuxBuffer, TAG_SYSB);

            /* Return the exception code */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Set the buffer and flags */
        Irp->UserBuffer = FileInformation;
        Irp->Flags = (IRP_BUFFERED_IO |
                      IRP_DEALLOCATE_BUFFER |
                      IRP_INPUT_OPERATION);
    }
    else if (DeviceObject->Flags & DO_DIRECT_IO)
    {
        _SEH2_TRY
        {
            /* Allocate an MDL */
            Mdl = IoAllocateMdl(FileInformation, Length, FALSE, TRUE, Irp);
            if (!Mdl) ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            MmProbeAndLockPages(Mdl, PreviousMode, IoWriteAccess);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Allocating failed, clean up and return the exception code */
            IopCleanupAfterException(FileObject, Irp, Event, NULL);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* No allocation flags, and use the buffer directly */
        Irp->UserBuffer = FileInformation;
    }

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    StackPtr->MinorFunction = IRP_MN_QUERY_DIRECTORY;

    /* Set Parameters */
    StackPtr->Parameters.QueryDirectory.FileInformationClass =
        FileInformationClass;
    StackPtr->Parameters.QueryDirectory.FileName = AuxBuffer;
    StackPtr->Parameters.QueryDirectory.FileIndex = 0;
    StackPtr->Parameters.QueryDirectory.Length = Length;
    StackPtr->Flags = 0;
    if (RestartScan) StackPtr->Flags = SL_RESTART_SCAN;
    if (ReturnSingleEntry) StackPtr->Flags |= SL_RETURN_SINGLE_ENTRY;

    /* Set deferred I/O */
    Irp->Flags |= IRP_DEFER_IO_COMPLETION;

    /* Perform the call */
    return IopPerformSynchronousRequest(DeviceObject,
                                        Irp,
                                        FileObject,
                                        TRUE,
                                        PreviousMode,
                                        LockedForSynch,
                                        IopOtherTransfer);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtQueryEaFile(IN HANDLE FileHandle,
              OUT PIO_STATUS_BLOCK IoStatusBlock,
              OUT PVOID Buffer,
              IN ULONG Length,
              IN BOOLEAN ReturnSingleEntry,
              IN PVOID EaList OPTIONAL,
              IN ULONG EaListLength,
              IN PULONG EaIndex OPTIONAL,
              IN BOOLEAN RestartScan)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationFile(IN HANDLE FileHandle,
                       OUT PIO_STATUS_BLOCK IoStatusBlock,
                       IN PVOID FileInformation,
                       IN ULONG Length,
                       IN FILE_INFORMATION_CLASS FileInformationClass)
{
    OBJECT_HANDLE_INFORMATION HandleInformation;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext;
    KIRQL OldIrql;
    IO_STATUS_BLOCK KernelIosb;
    BOOLEAN CallDriver = TRUE;
    PFILE_ACCESS_INFORMATION AccessBuffer;
    PFILE_MODE_INFORMATION ModeBuffer;
    PFILE_ALIGNMENT_INFORMATION AlignmentBuffer;
    PFILE_ALL_INFORMATION AllBuffer;
    PFAST_IO_DISPATCH FastIoDispatch;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the information class */
        if ((FileInformationClass < 0) ||
            (FileInformationClass >= FileMaximumInformation) ||
            !(IopQueryOperationLength[FileInformationClass]))
        {
            /* Invalid class */
            return STATUS_INVALID_INFO_CLASS;
        }

        /* Validate the length */
        if (Length < IopQueryOperationLength[FileInformationClass])
        {
            /* Invalid length */
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the information */
            ProbeForWrite(FileInformation, Length, sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
#if DBG
    else
    {
        /* Validate the information class */
        if ((FileInformationClass < 0) ||
            (FileInformationClass >= FileMaximumInformation) ||
            !(IopQueryOperationLength[FileInformationClass]))
        {
            /* Invalid class */
            return STATUS_INVALID_INFO_CLASS;
        }

        /* Validate the length */
        if (Length < IopQueryOperationLength[FileInformationClass])
        {
            /* Invalid length */
            return STATUS_INFO_LENGTH_MISMATCH;
        }
    }
#endif

    /* Reference the Handle */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       IopQueryOperationAccess
                                       [FileInformationClass],
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        /* Get the device object */
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        /* Get the device object */
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Check if the caller just wants the position */
        if (FileInformationClass == FilePositionInformation)
        {
            /* Protect write in SEH */
            _SEH2_TRY
            {
                /* Write the offset */
                ((PFILE_POSITION_INFORMATION)FileInformation)->
                    CurrentByteOffset = FileObject->CurrentByteOffset;

                /* Fill out the I/O Status Block */
                IoStatusBlock->Information = sizeof(FILE_POSITION_INFORMATION);
                Status = IoStatusBlock->Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Release the file lock, dereference the file and return */
            IopUnlockFileObject(FileObject);
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
        if (!Event)
        {
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeEvent(Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Check if FastIO is possible for the two available information classes */
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;
    if (FastIoDispatch != NULL &&
        ((FileInformationClass == FileBasicInformation && FastIoDispatch->FastIoQueryBasicInfo != NULL) ||
         (FileInformationClass == FileStandardInformation && FastIoDispatch->FastIoQueryStandardInfo != NULL)))
    {
        BOOLEAN Success = FALSE;

        if (FileInformationClass == FileBasicInformation)
        {
            Success = FastIoDispatch->FastIoQueryBasicInfo(FileObject, TRUE,
                                                           FileInformation,
                                                           &KernelIosb,
                                                           DeviceObject);
        }
        else
        {
            Success = FastIoDispatch->FastIoQueryStandardInfo(FileObject, TRUE,
                                                              FileInformation,
                                                              &KernelIosb,
                                                              DeviceObject);
        }

        /* If call succeed */
        if (Success)
        {
            /* Write the IOSB back */
            _SEH2_TRY
            {
                *IoStatusBlock = KernelIosb;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                KernelIosb.Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Free the event if we had one */
            if (LocalEvent)
            {
                ExFreePoolWithTag(Event, TAG_IO);
            }

            /* If FO was locked, unlock it */
            if (FileObject->Flags & FO_SYNCHRONOUS_IO)
            {
                IopUnlockFileObject(FileObject);
            }

            /* We're done with FastIO! */
            ObDereferenceObject(FileObject);
            return KernelIosb.Status;
        }
    }

    /* Clear the File Object event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, Event);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->UserIosb = (LocalEvent) ? &KernelIosb : IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? Event : NULL;
    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->MdlAddress = NULL;
    Irp->UserBuffer = FileInformation;

    /* Set the Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Allocate a buffer */
        Irp->AssociatedIrp.SystemBuffer =
            ExAllocatePoolWithTag(NonPagedPool,
                                  Length,
                                  TAG_SYSB);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Allocating failed, clean up and return the exception code */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Set the flags */
    Irp->Flags |= (IRP_BUFFERED_IO |
                   IRP_DEALLOCATE_BUFFER |
                   IRP_INPUT_OPERATION |
                   IRP_DEFER_IO_COMPLETION);

    /* Set the Parameters */
    StackPtr->Parameters.QueryFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.QueryFile.Length = Length;

    /* Queue the IRP */
    IopQueueIrpToThread(Irp);

    /* Update operation counts */
    IopUpdateOperationCount(IopOtherTransfer);

    /* Fill in file information before calling the driver.
       See 'File System Internals' page 485.*/
    if (FileInformationClass == FileAccessInformation)
    {
        AccessBuffer = Irp->AssociatedIrp.SystemBuffer;
        AccessBuffer->AccessFlags = HandleInformation.GrantedAccess;
        Irp->IoStatus.Information = sizeof(FILE_ACCESS_INFORMATION);
        CallDriver = FALSE;
    }
    else if (FileInformationClass == FileModeInformation)
    {
        ModeBuffer = Irp->AssociatedIrp.SystemBuffer;
        ModeBuffer->Mode = IopGetFileMode(FileObject);
        Irp->IoStatus.Information = sizeof(FILE_MODE_INFORMATION);
        CallDriver = FALSE;
    }
    else if (FileInformationClass == FileAlignmentInformation)
    {
        AlignmentBuffer = Irp->AssociatedIrp.SystemBuffer;
        AlignmentBuffer->AlignmentRequirement = DeviceObject->AlignmentRequirement;
        Irp->IoStatus.Information = sizeof(FILE_ALIGNMENT_INFORMATION);
        CallDriver = FALSE;
    }
    else if (FileInformationClass == FileAllInformation)
    {
        AllBuffer = Irp->AssociatedIrp.SystemBuffer;
        AllBuffer->AccessInformation.AccessFlags = HandleInformation.GrantedAccess;
        AllBuffer->ModeInformation.Mode = IopGetFileMode(FileObject);
        AllBuffer->AlignmentInformation.AlignmentRequirement = DeviceObject->AlignmentRequirement;
        Irp->IoStatus.Information = sizeof(FILE_ACCESS_INFORMATION) +
                                    sizeof(FILE_MODE_INFORMATION) +
                                    sizeof(FILE_ALIGNMENT_INFORMATION);
    }

    /* Call the Driver */
    if (CallDriver)
    {
        Status = IoCallDriver(DeviceObject, Irp);
    }
    else
    {
        Status = STATUS_SUCCESS;
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    if (Status == STATUS_PENDING)
    {
        /* Check if this was async I/O */
        if (LocalEvent)
        {
            /* Then to a non-alertable wait */
            Status = KeWaitForSingleObject(Event,
                                           Executive,
                                           PreviousMode,
                                           FALSE,
                                           NULL);
            if (Status == STATUS_USER_APC)
            {
                /* Abort the request */
                IopAbortInterruptedIrp(Event, Irp);
            }

            /* Set the final status */
            Status = KernelIosb.Status;

            /* Enter SEH to write the IOSB back */
            _SEH2_TRY
            {
                /* Write it back to the caller */
                *IoStatusBlock = KernelIosb;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Free the event */
            ExFreePoolWithTag(Event, TAG_IO);
        }
        else
        {
            /* Wait for the IRP */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           PreviousMode,
                                           (FileObject->Flags &
                                            FO_ALERTABLE_IO) != 0,
                                           NULL);
            if ((Status == STATUS_USER_APC) || (Status == STATUS_ALERTED))
            {
                /* Abort the request */
                IopAbortInterruptedIrp(&FileObject->Event, Irp);
            }

            /* Set the final status */
            Status = FileObject->FinalStatus;

            /* Release the file lock */
            IopUnlockFileObject(FileObject);
        }
    }
    else
    {
        /* Free the event if we had one */
        if (LocalEvent)
        {
            /* Clear it in the IRP for completion */
            Irp->UserEvent = NULL;
            ExFreePoolWithTag(Event, TAG_IO);
        }

        /* Set the caller IOSB */
        Irp->UserIosb = IoStatusBlock;

        /* The IRP wasn't completed, complete it ourselves */
        KeRaiseIrql(APC_LEVEL, &OldIrql);
        IopCompleteRequest(&Irp->Tail.Apc,
                           &NormalRoutine,
                           &NormalContext,
                           (PVOID*)&FileObject,
                           &NormalContext);
        KeLowerIrql(OldIrql);

        /* Release the file object if we had locked it*/
        if (!LocalEvent) IopUnlockFileObject(FileObject);
    }

    /* Return the Status */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtQueryQuotaInformationFile(IN HANDLE FileHandle,
                            OUT PIO_STATUS_BLOCK IoStatusBlock,
                            OUT PVOID Buffer,
                            IN ULONG Length,
                            IN BOOLEAN ReturnSingleEntry,
                            IN PVOID SidList OPTIONAL,
                            IN ULONG SidListLength,
                            IN PSID StartSid OPTIONAL,
                            IN BOOLEAN RestartScan)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReadFile(IN HANDLE FileHandle,
           IN HANDLE Event OPTIONAL,
           IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
           IN PVOID ApcContext OPTIONAL,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           OUT PVOID Buffer,
           IN ULONG Length,
           IN PLARGE_INTEGER ByteOffset OPTIONAL,
           IN PULONG Key OPTIONAL)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PKEVENT EventObject = NULL;
    LARGE_INTEGER CapturedByteOffset;
    ULONG CapturedKey = 0;
    BOOLEAN Synchronous = FALSE;
    PMDL Mdl;
    PFAST_IO_DISPATCH FastIoDispatch;
    IO_STATUS_BLOCK KernelIosb;
    BOOLEAN Success;

    PAGED_CODE();
    CapturedByteOffset.QuadPart = 0;
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_READ_DATA,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Validate User-Mode Buffers */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the read buffer */
            ProbeForWrite(Buffer, Length, 1);

            /* Check if we got a byte offset */
            if (ByteOffset)
            {
                /* Capture and probe it */
                CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            }

            /* Perform additional checks for non-cached file access */
            if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
            {
                /* Fail if Length is not sector size aligned
                 * Perform a quick check for 2^ sector sizes
                 * If it fails, try a more standard way
                 */
                if ((DeviceObject->SectorSize != 0) &&
                    ((DeviceObject->SectorSize - 1) & Length) != 0)
                {
                    if (Length % DeviceObject->SectorSize != 0)
                    {
                        /* Release the file object and and fail */
                        ObDereferenceObject(FileObject);
                        return STATUS_INVALID_PARAMETER;
                    }
                }

                /* Fail if buffer doesn't match alignment requirements */
                if (((ULONG_PTR)Buffer & DeviceObject->AlignmentRequirement) != 0)
                {
                    /* Release the file object and and fail */
                    ObDereferenceObject(FileObject);
                    return STATUS_INVALID_PARAMETER;
                }

                if (ByteOffset)
                {
                    /* Fail if ByteOffset is not sector size aligned */
                    if ((DeviceObject->SectorSize != 0) &&
                        (CapturedByteOffset.QuadPart % DeviceObject->SectorSize != 0))
                    {
                        /* Release the file object and and fail */
                        ObDereferenceObject(FileObject);
                        return STATUS_INVALID_PARAMETER;
                    }
                }
            }

            /* Capture and probe the key */
            if (Key) CapturedKey = ProbeForReadUlong(Key);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Release the file object and return the exception code */
            ObDereferenceObject(FileObject);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Kernel mode: capture directly */
        if (ByteOffset) CapturedByteOffset = *ByteOffset;
        if (Key) CapturedKey = *Key;
    }

    /* Check for invalid offset */
    if ((CapturedByteOffset.QuadPart < 0) && (CapturedByteOffset.QuadPart != -2))
    {
        /* -2 is FILE_USE_FILE_POINTER_POSITION */
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for event */
    if (Event)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(Event,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID*)&EventObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Otherwise reset the event */
        KeClearEvent(EventObject);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock the file object */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            if (EventObject) ObDereferenceObject(EventObject);
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Check if we don't have a byte offset available */
        if (!(ByteOffset) ||
            ((CapturedByteOffset.u.LowPart == FILE_USE_FILE_POINTER_POSITION) &&
             (CapturedByteOffset.u.HighPart == -1)))
        {
            /* Use the Current Byte Offset instead */
            CapturedByteOffset = FileObject->CurrentByteOffset;
        }

        /* If the file is cached, try fast I/O */
        if (FileObject->PrivateCacheMap)
        {
            /* Perform fast read */
            FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;
            ASSERT(FastIoDispatch != NULL && FastIoDispatch->FastIoRead != NULL);

            Success = FastIoDispatch->FastIoRead(FileObject,
                                                 &CapturedByteOffset,
                                                 Length,
                                                 TRUE,
                                                 CapturedKey,
                                                 Buffer,
                                                 &KernelIosb,
                                                 DeviceObject);

            /* Only accept the result if we got a straightforward status */
            if (Success &&
                (KernelIosb.Status == STATUS_SUCCESS ||
                 KernelIosb.Status == STATUS_BUFFER_OVERFLOW ||
                 KernelIosb.Status == STATUS_END_OF_FILE))
            {
                /* Fast path -- update transfer & operation counts */
                IopUpdateOperationCount(IopReadTransfer);
                IopUpdateTransferCount(IopReadTransfer,
                                       (ULONG)KernelIosb.Information);

                /* Enter SEH to write the IOSB back */
                _SEH2_TRY
                {
                    /* Write it back to the caller */
                    *IoStatusBlock = KernelIosb;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* The caller's IOSB was invalid, so fail */
                    if (EventObject) ObDereferenceObject(EventObject);
                    IopUnlockFileObject(FileObject);
                    ObDereferenceObject(FileObject);
                    _SEH2_YIELD(return _SEH2_GetExceptionCode());
                }
                _SEH2_END;

                /* Signal the completion event */
                if (EventObject)
                {
                    KeSetEvent(EventObject, 0, FALSE);
                    ObDereferenceObject(EventObject);
                }

                /* Clean up */
                IopUnlockFileObject(FileObject);
                ObDereferenceObject(FileObject);
                return KernelIosb.Status;
            }
        }

        /* Remember we are sync */
        Synchronous = TRUE;
    }
    else if (!(ByteOffset) &&
             !(FileObject->Flags & (FO_NAMED_PIPE | FO_MAILSLOT)))
    {
        /* Otherwise, this was async I/O without a byte offset, so fail */
        if (EventObject) ObDereferenceObject(EventObject);
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Clear the File Object's event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, EventObject, NULL);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = EventObject;
    Irp->PendingReturned = FALSE;
    Irp->Cancel = FALSE;
    Irp->CancelRoutine = NULL;
    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->MdlAddress = NULL;

    /* Set the Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_READ;
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.Read.Key = CapturedKey;
    StackPtr->Parameters.Read.Length = Length;
    StackPtr->Parameters.Read.ByteOffset = CapturedByteOffset;

    /* Check if this is buffered I/O */
    if (DeviceObject->Flags & DO_BUFFERED_IO)
    {
        /* Check if we have a buffer length */
        if (Length)
        {
            /* Enter SEH */
            _SEH2_TRY
            {
                /* Allocate a buffer */
                Irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithTag(NonPagedPool,
                                          Length,
                                          TAG_SYSB);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Allocating failed, clean up and return the exception code */
                IopCleanupAfterException(FileObject, Irp, EventObject, NULL);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;

            /* Set the buffer and flags */
            Irp->UserBuffer = Buffer;
            Irp->Flags = (IRP_BUFFERED_IO |
                          IRP_DEALLOCATE_BUFFER |
                          IRP_INPUT_OPERATION);
        }
        else
        {
            /* Not reading anything */
            Irp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
        }
    }
    else if (DeviceObject->Flags & DO_DIRECT_IO)
    {
        /* Check if we have a buffer length */
        if (Length)
        {
            _SEH2_TRY
            {
                /* Allocate an MDL */
                Mdl = IoAllocateMdl(Buffer, Length, FALSE, TRUE, Irp);
                if (!Mdl)
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                MmProbeAndLockPages(Mdl, PreviousMode, IoWriteAccess);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Allocating failed, clean up and return the exception code */
                IopCleanupAfterException(FileObject, Irp, EventObject, NULL);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;

        }

        /* No allocation flags */
        Irp->Flags = 0;
    }
    else
    {
        /* No allocation flags, and use the buffer directly */
        Irp->Flags = 0;
        Irp->UserBuffer = Buffer;
    }

    /* Now set the deferred read flags */
    Irp->Flags |= (IRP_READ_OPERATION | IRP_DEFER_IO_COMPLETION);

    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) Irp->Flags |= IRP_NOCACHE;

    /* Perform the call */
    return IopPerformSynchronousRequest(DeviceObject,
                                        Irp,
                                        FileObject,
                                        TRUE,
                                        PreviousMode,
                                        Synchronous,
                                        IopReadTransfer);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReadFileScatter(IN HANDLE FileHandle,
                  IN HANDLE Event OPTIONAL,
                  IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
                  IN PVOID UserApcContext OPTIONAL,
                  OUT PIO_STATUS_BLOCK UserIoStatusBlock,
                  IN FILE_SEGMENT_ELEMENT BufferDescription [],
                  IN ULONG BufferLength,
                  IN PLARGE_INTEGER  ByteOffset,
                  IN PULONG Key OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtSetEaFile(IN HANDLE FileHandle,
            IN PIO_STATUS_BLOCK IoStatusBlock,
            IN PVOID EaBuffer,
            IN ULONG EaBufferSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetInformationFile(IN HANDLE FileHandle,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN PVOID FileInformation,
                     IN ULONG Length,
                     IN FILE_INFORMATION_CLASS FileInformationClass)
{
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext;
    KIRQL OldIrql;
    IO_STATUS_BLOCK KernelIosb;
    PVOID Queue;
    PFILE_COMPLETION_INFORMATION CompletionInfo = FileInformation;
    PIO_COMPLETION_CONTEXT Context;
    PFILE_RENAME_INFORMATION RenameInfo;
    HANDLE TargetHandle = NULL;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the information class */
        if ((FileInformationClass < 0) ||
            (FileInformationClass >= FileMaximumInformation) ||
            !(IopSetOperationLength[FileInformationClass]))
        {
            /* Invalid class */
            return STATUS_INVALID_INFO_CLASS;
        }

        /* Validate the length */
        if (Length < IopSetOperationLength[FileInformationClass])
        {
            /* Invalid length */
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the information */
            ProbeForRead(FileInformation,
                         Length,
                         (Length == sizeof(BOOLEAN)) ?
                         sizeof(BOOLEAN) : sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Validate the information class */
        if ((FileInformationClass < 0) ||
            (FileInformationClass >= FileMaximumInformation) ||
            !(IopSetOperationLength[FileInformationClass]))
        {
            /* Invalid class */
            return STATUS_INVALID_INFO_CLASS;
        }

        /* Validate the length */
        if (Length < IopSetOperationLength[FileInformationClass])
        {
            /* Invalid length */
            return STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    /* Reference the Handle */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       IopSetOperationAccess
                                       [FileInformationClass],
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        /* Get the device object */
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        /* Get the device object */
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    DPRINT("Will call: %p\n", DeviceObject);
    DPRINT("Associated driver: %p (%wZ)\n", DeviceObject->DriverObject, &DeviceObject->DriverObject->DriverName);

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Check if the caller just wants the position */
        if (FileInformationClass == FilePositionInformation)
        {
            /* Protect write in SEH */
            _SEH2_TRY
            {
                /* Write the offset */
                FileObject->CurrentByteOffset =
                    ((PFILE_POSITION_INFORMATION)FileInformation)->
                    CurrentByteOffset;

                /* Fill out the I/O Status Block */
                IoStatusBlock->Information = 0;
                Status = IoStatusBlock->Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Update transfer count */
            IopUpdateTransferCount(IopOtherTransfer, Length);

            /* Release the file lock, dereference the file and return */
            IopUnlockFileObject(FileObject);
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
        if (!Event)
        {
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Clear the File Object event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, Event);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->UserIosb = (LocalEvent) ? &KernelIosb : IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? Event : NULL;
    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->MdlAddress = NULL;
    Irp->UserBuffer = FileInformation;

    /* Set the Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_SET_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Allocate a buffer */
        Irp->AssociatedIrp.SystemBuffer =
            ExAllocatePoolWithTag(NonPagedPool,
                                  Length,
                                  TAG_SYSB);

        /* Copy the data into it */
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                      FileInformation,
                      Length);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Allocating failed, clean up and return the exception code */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Set the flags */
    Irp->Flags |= (IRP_BUFFERED_IO |
                   IRP_DEALLOCATE_BUFFER |
                   IRP_DEFER_IO_COMPLETION);

    /* Set the Parameters */
    StackPtr->Parameters.SetFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.SetFile.Length = Length;

    /* Queue the IRP */
    IopQueueIrpToThread(Irp);

    /* Update operation counts */
    IopUpdateOperationCount(IopOtherTransfer);

    /* FIXME: Later, we can implement a lot of stuff here and avoid a driver call */
    /* Handle IO Completion Port quickly */
    if (FileInformationClass == FileCompletionInformation)
    {
        /* Check if the file object already has a completion port */
        if ((FileObject->Flags & FO_SYNCHRONOUS_IO) ||
            (FileObject->CompletionContext))
        {
            /* Fail */
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            /* Reference the Port */
            CompletionInfo = Irp->AssociatedIrp.SystemBuffer;
            Status = ObReferenceObjectByHandle(CompletionInfo->Port,
                                               IO_COMPLETION_MODIFY_STATE,
                                               IoCompletionType,
                                               PreviousMode,
                                               (PVOID*)&Queue,
                                               NULL);
            if (NT_SUCCESS(Status))
            {
                /* Allocate the Context */
                Context = ExAllocatePoolWithTag(PagedPool,
                                                sizeof(IO_COMPLETION_CONTEXT),
                                                IOC_TAG);
                if (Context)
                {
                    /* Set the Data */
                    Context->Key = CompletionInfo->Key;
                    Context->Port = Queue;
                    if (InterlockedCompareExchangePointer((PVOID*)&FileObject->
                                                          CompletionContext,
                                                          Context,
                                                          NULL))
                    {
                        /*
                         * Someone else set the completion port in the
                         * meanwhile, so dereference the port and fail.
                         */
                        ExFreePoolWithTag(Context, IOC_TAG);
                        ObDereferenceObject(Queue);
                        Status = STATUS_INVALID_PARAMETER;
                    }
                }
                else
                {
                    /* Dereference the Port now */
                    ObDereferenceObject(Queue);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        /* Set the IRP Status */
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
    }
    else if (FileInformationClass == FileRenameInformation ||
             FileInformationClass == FileLinkInformation ||
             FileInformationClass == FileMoveClusterInformation)
    {
        /* Get associated information */
        RenameInfo = Irp->AssociatedIrp.SystemBuffer;

        /* Only rename if:
         * -> We have a name
         * -> In unicode
         * -> sizes are valid
         */
        if (RenameInfo->FileNameLength != 0 &&
            !(RenameInfo->FileNameLength & 1) &&
            (Length - FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName) >= RenameInfo->FileNameLength))
        {
            /* Properly set information received */
            if (FileInformationClass == FileMoveClusterInformation)
            {
                StackPtr->Parameters.SetFile.ClusterCount = ((PFILE_MOVE_CLUSTER_INFORMATION)RenameInfo)->ClusterCount;
            }
            else
            {
                StackPtr->Parameters.SetFile.ReplaceIfExists = RenameInfo->ReplaceIfExists;
            }

            /* If we got fully path OR relative target, attempt a parent directory open */
            if (RenameInfo->FileName[0] == OBJ_NAME_PATH_SEPARATOR || RenameInfo->RootDirectory)
            {
                Status = IopOpenLinkOrRenameTarget(&TargetHandle, Irp, RenameInfo, FileObject);
                if (!NT_SUCCESS(Status))
                {
                    Irp->IoStatus.Status = Status;
                }
                else
                {
                    /* Call the Driver */
                    Status = IoCallDriver(DeviceObject, Irp);
                }
            }
            else
            {
                /* Call the Driver */
                Status = IoCallDriver(DeviceObject, Irp);
            }
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Status = Status;
        }
    }
    else
    {
        /* Call the Driver */
        Status = IoCallDriver(DeviceObject, Irp);
    }

    /* Check if we're waiting for the IRP to complete */
    if (Status == STATUS_PENDING)
    {
        /* Check if this was async I/O */
        if (LocalEvent)
        {
            /* Then to a non-alertable wait */
            Status = KeWaitForSingleObject(Event,
                                           Executive,
                                           PreviousMode,
                                           FALSE,
                                           NULL);
            if (Status == STATUS_USER_APC)
            {
                /* Abort the request */
                IopAbortInterruptedIrp(Event, Irp);
            }

            /* Set the final status */
            Status = KernelIosb.Status;

            /* Enter SEH to write the IOSB back */
            _SEH2_TRY
            {
                /* Write it back to the caller */
                *IoStatusBlock = KernelIosb;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Free the event */
            ExFreePoolWithTag(Event, TAG_IO);
        }
        else
        {
            /* Wait for the IRP */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           PreviousMode,
                                           (FileObject->Flags &
                                            FO_ALERTABLE_IO) != 0,
                                           NULL);
            if ((Status == STATUS_USER_APC) || (Status == STATUS_ALERTED))
            {
                /* Abort the request */
                IopAbortInterruptedIrp(&FileObject->Event, Irp);
            }

            /* Set the final status */
            Status = FileObject->FinalStatus;

            /* Release the file lock */
            IopUnlockFileObject(FileObject);
        }
    }
    else
    {
        /* Free the event if we had one */
        if (LocalEvent)
        {
            /* Clear it in the IRP for completion */
            Irp->UserEvent = NULL;
            ExFreePoolWithTag(Event, TAG_IO);
        }

        /* Set the caller IOSB */
        Irp->UserIosb = IoStatusBlock;

        /* The IRP wasn't completed, complete it ourselves */
        KeRaiseIrql(APC_LEVEL, &OldIrql);
        IopCompleteRequest(&Irp->Tail.Apc,
                           &NormalRoutine,
                           &NormalContext,
                           (PVOID*)&FileObject,
                           &NormalContext);
        KeLowerIrql(OldIrql);

        /* Release the file object if we had locked it*/
        if (!LocalEvent) IopUnlockFileObject(FileObject);
    }

    if (TargetHandle != NULL)
    {
        ObCloseHandle(TargetHandle, KernelMode);
    }

    /* Return the Status */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtSetQuotaInformationFile(IN HANDLE FileHandle,
                          OUT PIO_STATUS_BLOCK IoStatusBlock,
                          IN PVOID Buffer,
                          IN ULONG BufferLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtUnlockFile(IN HANDLE FileHandle,
             OUT PIO_STATUS_BLOCK IoStatusBlock,
             IN PLARGE_INTEGER ByteOffset,
             IN PLARGE_INTEGER Length,
             IN ULONG Key OPTIONAL)
{
    PFILE_OBJECT FileObject;
    PLARGE_INTEGER LocalLength = NULL;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    LARGE_INTEGER CapturedByteOffset, CapturedLength;
    NTSTATUS Status;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    IO_STATUS_BLOCK KernelIosb;
    PFAST_IO_DISPATCH FastIoDispatch;
    PAGED_CODE();
    CapturedByteOffset.QuadPart = 0;
    CapturedLength.QuadPart = 0;
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Must have either FILE_READ_DATA or FILE_WRITE_DATA access */
        if (!(HandleInformation.GrantedAccess &
            (FILE_WRITE_DATA | FILE_READ_DATA)))
        {
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }

        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe and capture the large integers */
            CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            CapturedLength = ProbeForReadLargeInteger(Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Dereference the object and return exception code */
            ObDereferenceObject(FileObject);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Otherwise, capture them directly */
        CapturedByteOffset = *ByteOffset;
        CapturedLength = *Length;
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Try to do it the FastIO way if possible */
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;
    if (FastIoDispatch != NULL && FastIoDispatch->FastIoUnlockSingle != NULL)
    {
        if (FastIoDispatch->FastIoUnlockSingle(FileObject,
                                               &CapturedByteOffset,
                                               &CapturedLength,
                                               PsGetCurrentProcess(),
                                               Key,
                                               &KernelIosb,
                                               DeviceObject))
        {
            /* Write the IOSB back */
            _SEH2_TRY
            {
                *IoStatusBlock = KernelIosb;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                KernelIosb.Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* We're done with FastIO! */
            ObDereferenceObject(FileObject);
            return KernelIosb.Status;
        }
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
        if (!Event)
        {
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeEvent(Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Clear File Object event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, Event);

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->UserIosb = (LocalEvent) ? &KernelIosb : IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_LOCK_CONTROL;
    StackPtr->MinorFunction = IRP_MN_UNLOCK_SINGLE;
    StackPtr->FileObject = FileObject;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Allocate a buffer */
        LocalLength = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(LARGE_INTEGER),
                                            TAG_LOCK);

        /* Set the length */
        *LocalLength = CapturedLength;
        Irp->Tail.Overlay.AuxiliaryBuffer = (PVOID)LocalLength;
        StackPtr->Parameters.LockControl.Length = LocalLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Allocating failed, clean up and return the exception code */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        if (LocalLength) ExFreePoolWithTag(LocalLength, TAG_LOCK);

        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Set Parameters */
    StackPtr->Parameters.LockControl.ByteOffset = CapturedByteOffset;
    StackPtr->Parameters.LockControl.Key = Key;

    /* Call the Driver */
    Status = IopPerformSynchronousRequest(DeviceObject,
                                          Irp,
                                          FileObject,
                                          FALSE,
                                          PreviousMode,
                                          !LocalEvent,
                                          IopOtherTransfer);

    /* Check if this was async I/O */
    if (LocalEvent)
    {
        /* It was, finalize this request */
        Status = IopFinalizeAsynchronousIo(Status,
                                           Event,
                                           Irp,
                                           PreviousMode,
                                           &KernelIosb,
                                           IoStatusBlock);
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtWriteFile(IN HANDLE FileHandle,
            IN HANDLE Event OPTIONAL,
            IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
            IN PVOID ApcContext OPTIONAL,
            OUT PIO_STATUS_BLOCK IoStatusBlock,
            IN PVOID Buffer,
            IN ULONG Length,
            IN PLARGE_INTEGER ByteOffset OPTIONAL,
            IN PULONG Key OPTIONAL)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PKEVENT EventObject = NULL;
    LARGE_INTEGER CapturedByteOffset;
    ULONG CapturedKey = 0;
    BOOLEAN Synchronous = FALSE;
    PMDL Mdl;
    OBJECT_HANDLE_INFORMATION ObjectHandleInfo;
    PFAST_IO_DISPATCH FastIoDispatch;
    IO_STATUS_BLOCK KernelIosb;
    BOOLEAN Success;

    PAGED_CODE();
    CapturedByteOffset.QuadPart = 0;
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Get File Object for write */
    Status = ObReferenceFileObjectForWrite(FileHandle,
                                           PreviousMode,
                                           &FileObject,
                                           &ObjectHandleInfo);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Validate User-Mode Buffers */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the read buffer */
            ProbeForRead(Buffer, Length, 1);

            /* Check if we got a byte offset */
            if (ByteOffset)
            {
                /* Capture and probe it */
                CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            }

            /* Perform additional checks for non-cached file access */
            if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
            {
                /* Fail if Length is not sector size aligned
                 * Perform a quick check for 2^ sector sizes
                 * If it fails, try a more standard way
                 */
                if ((DeviceObject->SectorSize != 0) &&
                    ((DeviceObject->SectorSize - 1) & Length) != 0)
                {
                    if (Length % DeviceObject->SectorSize != 0)
                    {
                        /* Release the file object and and fail */
                        ObDereferenceObject(FileObject);
                        return STATUS_INVALID_PARAMETER;
                    }
                }

                /* Fail if buffer doesn't match alignment requirements */
                if (((ULONG_PTR)Buffer & DeviceObject->AlignmentRequirement) != 0)
                {
                    /* Release the file object and and fail */
                    ObDereferenceObject(FileObject);
                    return STATUS_INVALID_PARAMETER;
                }

                if (ByteOffset)
                {
                    /* Fail if ByteOffset is not sector size aligned */
                    if ((DeviceObject->SectorSize != 0) &&
                        (CapturedByteOffset.QuadPart % DeviceObject->SectorSize != 0))
                    {
                        /* Only if that's not specific values for synchronous IO */
                        if ((CapturedByteOffset.QuadPart != FILE_WRITE_TO_END_OF_FILE) &&
                            (CapturedByteOffset.QuadPart != FILE_USE_FILE_POINTER_POSITION ||
                             !BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO)))
                        {
                            /* Release the file object and and fail */
                            ObDereferenceObject(FileObject);
                            return STATUS_INVALID_PARAMETER;
                        }
                    }
                }
            }

            /* Capture and probe the key */
            if (Key) CapturedKey = ProbeForReadUlong(Key);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Release the file object and return the exception code */
            ObDereferenceObject(FileObject);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Kernel mode: capture directly */
        if (ByteOffset) CapturedByteOffset = *ByteOffset;
        if (Key) CapturedKey = *Key;
    }

    /* Check for invalid offset */
    if (CapturedByteOffset.QuadPart < -2)
    {
        /* -1 is FILE_WRITE_TO_END_OF_FILE */
        /* -2 is FILE_USE_FILE_POINTER_POSITION */
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if this is an append operation */
    if ((ObjectHandleInfo.GrantedAccess &
        (FILE_APPEND_DATA | FILE_WRITE_DATA)) == FILE_APPEND_DATA)
    {
        /* Give the drivers something to understand */
        CapturedByteOffset.u.LowPart = FILE_WRITE_TO_END_OF_FILE;
        CapturedByteOffset.u.HighPart = -1;
    }

    /* Check for event */
    if (Event)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(Event,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID*)&EventObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Otherwise reset the event */
        KeClearEvent(EventObject);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock the file object */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            if (EventObject) ObDereferenceObject(EventObject);
            ObDereferenceObject(FileObject);
            return Status;
        }

        /* Check if we don't have a byte offset available */
        if (!(ByteOffset) ||
            ((CapturedByteOffset.u.LowPart == FILE_USE_FILE_POINTER_POSITION) &&
             (CapturedByteOffset.u.HighPart == -1)))
        {
            /* Use the Current Byte Offset instead */
            CapturedByteOffset = FileObject->CurrentByteOffset;
        }

        /* If the file is cached, try fast I/O */
        if (FileObject->PrivateCacheMap)
        {
            /* Perform fast write */
            FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;
            ASSERT(FastIoDispatch != NULL && FastIoDispatch->FastIoWrite != NULL);

            Success = FastIoDispatch->FastIoWrite(FileObject,
                                                  &CapturedByteOffset,
                                                  Length,
                                                  TRUE,
                                                  CapturedKey,
                                                  Buffer,
                                                  &KernelIosb,
                                                  DeviceObject);

            /* Only accept the result if it was successful */
            if (Success &&
                KernelIosb.Status == STATUS_SUCCESS)
            {
                /* Fast path -- update transfer & operation counts */
                IopUpdateOperationCount(IopWriteTransfer);
                IopUpdateTransferCount(IopWriteTransfer,
                                       (ULONG)KernelIosb.Information);

                /* Enter SEH to write the IOSB back */
                _SEH2_TRY
                {
                    /* Write it back to the caller */
                    *IoStatusBlock = KernelIosb;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* The caller's IOSB was invalid, so fail */
                    if (EventObject) ObDereferenceObject(EventObject);
                    IopUnlockFileObject(FileObject);
                    ObDereferenceObject(FileObject);
                    _SEH2_YIELD(return _SEH2_GetExceptionCode());
                }
                _SEH2_END;

                /* Signal the completion event */
                if (EventObject)
                {
                    KeSetEvent(EventObject, 0, FALSE);
                    ObDereferenceObject(EventObject);
                }

                /* Clean up */
                IopUnlockFileObject(FileObject);
                ObDereferenceObject(FileObject);
                return KernelIosb.Status;
            }
        }

        /* Remember we are sync */
        Synchronous = TRUE;
    }
    else if (!(ByteOffset) &&
             !(FileObject->Flags & (FO_NAMED_PIPE | FO_MAILSLOT)))
    {
        /* Otherwise, this was async I/O without a byte offset, so fail */
        if (EventObject) ObDereferenceObject(EventObject);
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Clear the File Object's event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, EventObject, NULL);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = EventObject;
    Irp->PendingReturned = FALSE;
    Irp->Cancel = FALSE;
    Irp->CancelRoutine = NULL;
    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->MdlAddress = NULL;

    /* Set the Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_WRITE;
    StackPtr->FileObject = FileObject;
    StackPtr->Flags = FileObject->Flags & FO_WRITE_THROUGH ?
                      SL_WRITE_THROUGH : 0;
    StackPtr->Parameters.Write.Key = CapturedKey;
    StackPtr->Parameters.Write.Length = Length;
    StackPtr->Parameters.Write.ByteOffset = CapturedByteOffset;

    /* Check if this is buffered I/O */
    if (DeviceObject->Flags & DO_BUFFERED_IO)
    {
        /* Check if we have a buffer length */
        if (Length)
        {
            /* Enter SEH */
            _SEH2_TRY
            {
                /* Allocate a buffer */
                Irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithTag(NonPagedPool,
                                          Length,
                                          TAG_SYSB);

                /* Copy the data into it */
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Buffer, Length);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Allocating failed, clean up and return the exception code */
                IopCleanupAfterException(FileObject, Irp, EventObject, NULL);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;

            /* Set the flags */
            Irp->Flags = (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
        }
        else
        {
            /* Not writing anything */
            Irp->Flags = IRP_BUFFERED_IO;
        }
    }
    else if (DeviceObject->Flags & DO_DIRECT_IO)
    {
        /* Check if we have a buffer length */
        if (Length)
        {
            _SEH2_TRY
            {
                /* Allocate an MDL */
                Mdl = IoAllocateMdl(Buffer, Length, FALSE, TRUE, Irp);
                if (!Mdl)
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                MmProbeAndLockPages(Mdl, PreviousMode, IoReadAccess);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Allocating failed, clean up and return the exception code */
                IopCleanupAfterException(FileObject, Irp, EventObject, NULL);
                _SEH2_YIELD(return _SEH2_GetExceptionCode());
            }
            _SEH2_END;
        }

        /* No allocation flags */
        Irp->Flags = 0;
    }
    else
    {
        /* No allocation flags, and use the buffer directly */
        Irp->Flags = 0;
        Irp->UserBuffer = Buffer;
    }

    /* Now set the deferred read flags */
    Irp->Flags |= (IRP_WRITE_OPERATION | IRP_DEFER_IO_COMPLETION);

    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) Irp->Flags |= IRP_NOCACHE;

    /* Perform the call */
    return IopPerformSynchronousRequest(DeviceObject,
                                        Irp,
                                        FileObject,
                                        TRUE,
                                        PreviousMode,
                                        Synchronous,
                                        IopWriteTransfer);
}

NTSTATUS
NTAPI
NtWriteFileGather(IN HANDLE FileHandle,
                  IN HANDLE Event OPTIONAL,
                  IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
                  IN PVOID UserApcContext OPTIONAL,
                  OUT PIO_STATUS_BLOCK UserIoStatusBlock,
                  IN FILE_SEGMENT_ELEMENT BufferDescription [],
                  IN ULONG BufferLength,
                  IN PLARGE_INTEGER ByteOffset,
                  IN PULONG Key OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(IN HANDLE FileHandle,
                             OUT PIO_STATUS_BLOCK IoStatusBlock,
                             OUT PVOID FsInformation,
                             IN ULONG Length,
                             IN FS_INFORMATION_CLASS FsInformationClass)
{
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status;
    IO_STATUS_BLOCK KernelIosb;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the information class */
        if ((FsInformationClass < 0) ||
            (FsInformationClass >= FileFsMaximumInformation) ||
            !(IopQueryFsOperationLength[FsInformationClass]))
        {
            /* Invalid class */
            return STATUS_INVALID_INFO_CLASS;
        }

        /* Validate the length */
        if (Length < IopQueryFsOperationLength[FsInformationClass])
        {
            /* Invalid length */
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the information */
            ProbeForWrite(FsInformation, Length, sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       IopQueryFsOperationAccess
                                       [FsInformationClass],
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Only allow direct device open for FileFsDeviceInformation */
    if (BooleanFlagOn(FileObject->Flags, FO_DIRECT_DEVICE_OPEN) &&
        FsInformationClass != FileFsDeviceInformation)
    {
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
    }

    /*
     * Quick path for FileFsDeviceInformation - the kernel has enough
     * info to reply instead of the driver, excepted for network file systems
     */
    if (FsInformationClass == FileFsDeviceInformation &&
        (BooleanFlagOn(FileObject->Flags, FO_DIRECT_DEVICE_OPEN) || FileObject->DeviceObject->DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM))
    {
        PFILE_FS_DEVICE_INFORMATION FsDeviceInfo = FsInformation;
        DeviceObject = FileObject->DeviceObject;

        _SEH2_TRY
        {
            FsDeviceInfo->DeviceType = DeviceObject->DeviceType;

            /* Complete characteristcs with mount status if relevant */
            FsDeviceInfo->Characteristics = DeviceObject->Characteristics;
            if (IopGetMountFlag(DeviceObject))
            {
                SetFlag(FsDeviceInfo->Characteristics, FILE_DEVICE_IS_MOUNTED);
            }

            IoStatusBlock->Information = sizeof(FILE_FS_DEVICE_INFORMATION);
            IoStatusBlock->Status = STATUS_SUCCESS;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Check if we had a file lock */
            if (BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO))
            {
                /* Release it */
                IopUnlockFileObject(FileObject);
            }

            /* Dereference the FO */
            ObDereferenceObject(FileObject);

            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        /* Check if we had a file lock */
        if (BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO))
        {
            /* Release it */
            IopUnlockFileObject(FileObject);
        }

        /* Dereference the FO */
        ObDereferenceObject(FileObject);

        return STATUS_SUCCESS;
    }
    /* This is to be handled by the kernel, not by FSD */
    else if (FsInformationClass == FileFsDriverPathInformation)
    {
        PFILE_FS_DRIVER_PATH_INFORMATION DriverPathInfo;

        _SEH2_TRY
        {
            /* Allocate our local structure */
            DriverPathInfo = ExAllocatePoolWithQuotaTag(NonPagedPool, Length, TAG_IO);

            /* And copy back caller data */
            RtlCopyMemory(DriverPathInfo, FsInformation, Length);

            /* Is the driver in the IO path? */
            Status = IopGetDriverPathInformation(FileObject, DriverPathInfo, Length);
            /* We failed, don't continue execution */
            if (!NT_SUCCESS(Status))
            {
                RtlRaiseStatus(Status);
            }

            /* We succeed, copy back info */
            ((PFILE_FS_DRIVER_PATH_INFORMATION)FsInformation)->DriverInPath = DriverPathInfo->DriverInPath;

            /* We're done */
            IoStatusBlock->Information = sizeof(FILE_FS_DRIVER_PATH_INFORMATION);
            IoStatusBlock->Status = STATUS_SUCCESS;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Don't leak */
        if (DriverPathInfo != NULL)
        {
            ExFreePoolWithTag(DriverPathInfo, TAG_IO);
        }

        /* Check if we had a file lock */
        if (BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO))
        {
            /* Release it */
            IopUnlockFileObject(FileObject);
        }

        /* Dereference the FO */
        ObDereferenceObject(FileObject);

        return Status;
    }

    if (!BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO))
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
        if (!Event)
        {
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeEvent(Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Clear File Object event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, Event);

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->UserIosb = (LocalEvent) ? &KernelIosb : IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->UserBuffer = FsInformation;
    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->MdlAddress = NULL;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Allocate a buffer */
        Irp->AssociatedIrp.SystemBuffer =
            ExAllocatePoolWithTag(NonPagedPool,
                                  Length,
                                  TAG_SYSB);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Allocating failed, clean up and return the exception code */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Set the flags for this buffered + deferred I/O */
    Irp->Flags |= (IRP_BUFFERED_IO |
                   IRP_DEALLOCATE_BUFFER |
                   IRP_INPUT_OPERATION |
                   IRP_DEFER_IO_COMPLETION);

    /* Set Parameters */
    StackPtr->Parameters.QueryVolume.Length = Length;
    StackPtr->Parameters.QueryVolume.FsInformationClass = FsInformationClass;

    /* Call the Driver */
    Status = IopPerformSynchronousRequest(DeviceObject,
                                          Irp,
                                          FileObject,
                                          TRUE,
                                          PreviousMode,
                                          !LocalEvent,
                                          IopOtherTransfer);

    /* Check if this was async I/O */
    if (LocalEvent)
    {
        /* It was, finalize this request */
        Status = IopFinalizeAsynchronousIo(Status,
                                           Event,
                                           Irp,
                                           PreviousMode,
                                           &KernelIosb,
                                           IoStatusBlock);
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetVolumeInformationFile(IN HANDLE FileHandle,
                           OUT PIO_STATUS_BLOCK IoStatusBlock,
                           IN PVOID FsInformation,
                           IN ULONG Length,
                           IN FS_INFORMATION_CLASS FsInformationClass)
{
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject, TargetDeviceObject;
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status;
    IO_STATUS_BLOCK KernelIosb;
    TARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the information class */
        if ((FsInformationClass < 0) ||
            (FsInformationClass >= FileFsMaximumInformation) ||
            !(IopSetFsOperationLength[FsInformationClass]))
        {
            /* Invalid class */
            return STATUS_INVALID_INFO_CLASS;
        }

        /* Validate the length */
        if (Length < IopSetFsOperationLength[FsInformationClass])
        {
            /* Invalid length */
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the information */
            ProbeForRead(FsInformation, Length, sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       IopSetFsOperationAccess
                                       [FsInformationClass],
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get target device for notification */
    Status = IoGetRelatedTargetDevice(FileObject, &TargetDeviceObject);
    if (!NT_SUCCESS(Status)) TargetDeviceObject = NULL;

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        Status = IopLockFileObject(FileObject, PreviousMode);
        if (Status != STATUS_SUCCESS)
        {
            ObDereferenceObject(FileObject);
            if (TargetDeviceObject) ObDereferenceObject(TargetDeviceObject);
            return Status;
        }
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
        if (!Event)
        {
            ObDereferenceObject(FileObject);
            if (TargetDeviceObject) ObDereferenceObject(TargetDeviceObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KeInitializeEvent(Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Clear File Object event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        if (TargetDeviceObject) ObDereferenceObject(TargetDeviceObject);
        return IopCleanupFailedIrp(FileObject, NULL, Event);
    }

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->UserIosb = (LocalEvent) ? &KernelIosb : IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    Irp->UserBuffer = FsInformation;
    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->MdlAddress = NULL;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_SET_VOLUME_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Allocate a buffer */
        Irp->AssociatedIrp.SystemBuffer =
            ExAllocatePoolWithTag(NonPagedPool,
                                  Length,
                                  TAG_SYSB);

        /* Copy the data into it */
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, FsInformation, Length);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Allocating failed, clean up and return the exception code */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        if (TargetDeviceObject) ObDereferenceObject(TargetDeviceObject);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Set the flags for this buffered + deferred I/O */
    Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);

    /* Set Parameters */
    StackPtr->Parameters.SetVolume.Length = Length;
    StackPtr->Parameters.SetVolume.FsInformationClass = FsInformationClass;

    /* Call the Driver */
    Status = IopPerformSynchronousRequest(DeviceObject,
                                          Irp,
                                          FileObject,
                                          FALSE,
                                          PreviousMode,
                                          !LocalEvent,
                                          IopOtherTransfer);

    /* Check if this was async I/O */
    if (LocalEvent)
    {
        /* It was, finalize this request */
        Status = IopFinalizeAsynchronousIo(Status,
                                           Event,
                                           Irp,
                                           PreviousMode,
                                           &KernelIosb,
                                           IoStatusBlock);
    }

    if (TargetDeviceObject && NT_SUCCESS(Status))
    {
        /* Time to report change */
        NotificationStructure.Version = 1;
        NotificationStructure.Size = sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION);
        NotificationStructure.Event = GUID_IO_VOLUME_NAME_CHANGE;
        NotificationStructure.FileObject = NULL;
        NotificationStructure.NameBufferOffset = - 1;
        Status = IoReportTargetDeviceChange(TargetDeviceObject, &NotificationStructure);
    }

    /* Return status */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(IN HANDLE DeviceHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtRequestDeviceWakeup(IN HANDLE DeviceHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
