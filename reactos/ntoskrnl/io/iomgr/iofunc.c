/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iofunc.c
 * PURPOSE:         Generic I/O Functions that build IRPs for various operations
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gunnar Dalsnes
 *                  Filip Navara (navaraf@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "internal/io_i.h"

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
IopCleanupAfterException(IN PFILE_OBJECT FileObject,
                         IN PIRP Irp,
                         IN PKEVENT Event OPTIONAL,
                         IN PKEVENT LocalEvent OPTIONAL)
{
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "IRP: %p. FO: %p \n", Irp, FileObject);

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
    IOTRACE(IO_API_DEBUG, "IRP: %p. Status: %lx \n", Irp, SynchStatus);

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
    _SEH_TRY
    {
        *IoStatusBlock = *KernelIosb;
    }
    _SEH_HANDLE
    {
        /* Get the exception code */
        FinalStatus = _SEH_GetExceptionCode();
    }
    _SEH_END;

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
    PVOID NormalContext;
    KIRQL OldIrql;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "IRP: %p. DO: %p. FO: %p \n",
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
                                            FO_ALERTABLE_IO),
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
    NTSTATUS Status = STATUS_SUCCESS;
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
    IOTRACE(IO_CTL_DEBUG, "Handle: %lx. CTL: %lx. Type: %lx \n",
            DeviceHandle, IoControlCode, IsDevIoCtl);

    /* Get the access type */
    AccessType = IO_METHOD_FROM_CTL_CODE(IoControlCode);

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
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
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
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
        IopLockFileObject(FileObject);

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

    /* Clear the event */
    KeClearEvent(&FileObject->Event);

    /* Allocate IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, Event, NULL);

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
    StackPtr->MinorFunction = 0;
    StackPtr->Control = 0;
    StackPtr->Flags = 0;
    StackPtr->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    /* Set the IOCTL Data */
    StackPtr->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    StackPtr->Parameters.DeviceIoControl.OutputBufferLength =
        OutputBufferLength;

    /* Handle the Methods */
    switch (AccessType)
    {
        /* Buffered I/O */
        case METHOD_BUFFERED:

            /* Enter SEH for allocations */
            _SEH_TRY
            {
                /* Select the right Buffer Length */
                BufferLength = (InputBufferLength > OutputBufferLength) ?
                                InputBufferLength : OutputBufferLength;

                /* Make sure there is one */
                if (BufferLength)
                {
                    /* Allocate the System Buffer */
                    Irp->AssociatedIrp.SystemBuffer =
                        ExAllocatePoolWithTag(NonPagedPool,
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
            _SEH_HANDLE
            {
                /* Cleanup after exception */
                IopCleanupAfterException(FileObject, Irp, Event, NULL);
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            if (!NT_SUCCESS(Status)) return Status;
            break;

        /* Direct I/O */
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:

            /* Enter SEH */
            _SEH_TRY
            {
                /* Check if we got an input buffer */
                if ((InputBufferLength) && (InputBuffer))
                {
                    /* Allocate the System Buffer */
                    Irp->AssociatedIrp.SystemBuffer =
                        ExAllocatePoolWithTag(NonPagedPool,
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
                if (OutputBuffer)
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
            _SEH_HANDLE
            {
                /* Cleanup after exception */
                IopCleanupAfterException(FileObject, Irp, Event, NULL);
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            if (!NT_SUCCESS(Status)) return Status;
            break;

        case METHOD_NEITHER:

            /* Just save the Buffer */
            Irp->UserBuffer = OutputBuffer;
            StackPtr->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
    }

    /* Use deferred completion for FS I/O */
    Irp->Flags |= (!IsDevIoCtl) ? IRP_DEFER_IO_COMPLETION : 0;

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
    IOTRACE(IO_API_DEBUG, "Handle: %p. CTL: %lx. Type: %lx \n",
            FileObject, InformationClass, File);

    /* Reference the object */
    ObReferenceObject(FileObject);

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        IopLockFileObject(FileObject);

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
        /* Check if the requet is pending */
        if (Status == STATUS_PENDING)
        {
            /* Wait on the file object */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           KernelMode,
                                           FileObject->Flags & FO_ALERTABLE_IO,
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
    *ReturnedLength = IoStatusBlock.Information;
    return Status;
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
    IOTRACE(IO_API_DEBUG, "FileObject: %p. Mdl: %p. Offset: %p \n",
            FileObject, Mdl, Offset);

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
    IOTRACE(IO_API_DEBUG, "FileObject: %p. Mdl: %p. Offset: %p \n",
            FileObject, Mdl, Offset);

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
    IOTRACE(IO_API_DEBUG, "FileObject: %p. Class: %lx. Length: %lx \n",
            FileObject, FileInformationClass, Length);

    /* Reference the object */
    ObReferenceObject(FileObject);

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        IopLockFileObject(FileObject);

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
        /* Check if the requet is pending */
        if (Status == STATUS_PENDING)
        {
            /* Wait on the file object */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           KernelMode,
                                           FileObject->Flags & FO_ALERTABLE_IO,
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
    NTSTATUS Status = STATUS_SUCCESS;
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
        _SEH_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Return exception code, if any */
        if (!NT_SUCCESS(Status)) return Status;
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
        IopLockFileObject(FileObject);
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
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
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN LockedForSync = FALSE;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Probe the I/O STatus block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the buffer */
            if (BufferSize) ProbeForWrite(Buffer, BufferSize, sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Check if probing failed */
        if (!NT_SUCCESS(Status)) return Status;
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
        if (Status != STATUS_SUCCESS) return Status;
        KeClearEvent(Event);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        IopLockFileObject(FileObject);
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
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_HANDLE_INFORMATION HandleInformation;
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
        _SEH_TRY
        {
            /* Probe the I/O STatus block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe and capture the large integers */
            CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            CapturedLength = ProbeForReadLargeInteger(Length);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Check if probing failed */
        if (!NT_SUCCESS(Status))
        {
            /* Dereference the object and return exception code */
            ObDereferenceObject(FileObject);
            return Status;
        }
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

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        IopLockFileObject(FileObject);
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

    /* Enter SEH */
    _SEH_TRY
    {
        /* Allocate local buffer */
        LocalLength = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(LARGE_INTEGER),
                                            TAG_LOCK);

        /* Set the length */
        *LocalLength = CapturedLength;
        Irp->Tail.Overlay.AuxiliaryBuffer = (PVOID)LocalLength;
        StackPtr->Parameters.LockControl.Length = LocalLength;
    }
    _SEH_HANDLE
    {
        /* Allocating failed, clean up */
        IopCleanupAfterException(FileObject, Irp, Event, NULL);
        if (LocalLength) ExFreePool(LocalLength);

        /* Get status */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if (!NT_SUCCESS(Status)) return Status;

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
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN LockedForSynch = FALSE;
    PKEVENT Event = NULL;
    PVOID AuxBuffer = NULL;
    PMDL Mdl;
    UNICODE_STRING CapturedFileName;
    PUNICODE_STRING SearchPattern;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
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
        _SEH_HANDLE
        {
            /* Get exception code and free the buffer */
            if (AuxBuffer) ExFreePool(AuxBuffer);
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Return status on failure */
        if (!NT_SUCCESS(Status)) return Status;
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
        if (AuxBuffer) ExFreePool(AuxBuffer);
        return Status;
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
        IopLockFileObject(FileObject);

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
        /* Enter SEH */
        _SEH_TRY
        {
            /* Allocate a buffer */
            Irp->AssociatedIrp.SystemBuffer =
                ExAllocatePoolWithTag(NonPagedPool,
                                      Length,
                                      TAG_SYSB);
        }
        _SEH_HANDLE
        {
            /* Allocating failed, clean up */
            IopCleanupAfterException(FileObject, Irp, Event, NULL);
            if (AuxBuffer) ExFreePool(AuxBuffer);

            /* Get status */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;

        /* Set the buffer and flags */
        Irp->UserBuffer = FileInformation;
        Irp->Flags = (IRP_BUFFERED_IO |
                      IRP_DEALLOCATE_BUFFER |
                      IRP_INPUT_OPERATION);
    }
    else if (DeviceObject->Flags & DO_DIRECT_IO)
    {
        _SEH_TRY
        {
            /* Allocate an MDL */
            Mdl = IoAllocateMdl(FileInformation, Length, FALSE, TRUE, Irp);
            MmProbeAndLockPages(Mdl, PreviousMode, IoWriteAccess);
        }
        _SEH_HANDLE
        {
            /* Allocating failed, clean up */
            IopCleanupAfterException(FileObject, Irp, Event, NULL);
            Status = _SEH_GetExceptionCode();
            _SEH_YIELD(return Status);
        }
        _SEH_END;
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
    NTSTATUS Status = STATUS_SUCCESS;
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
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the information class */
        if ((FileInformationClass >= FileMaximumInformation) ||
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
        _SEH_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the information */
            ProbeForWrite(FileInformation, Length, sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Validate the information class */
        if ((FileInformationClass >= FileMaximumInformation) ||
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
        IopLockFileObject(FileObject);

        /* Check if the caller just wants the position */
        if (FileInformationClass == FilePositionInformation)
        {
            /* Protect write in SEH */
            _SEH_TRY
            {
                /* Write the offset */
                ((PFILE_POSITION_INFORMATION)FileInformation)->
                    CurrentByteOffset = FileObject->CurrentByteOffset;

                /* Fill out the I/O Status Block */
                IoStatusBlock->Information = sizeof(FILE_POSITION_INFORMATION);
                Status = IoStatusBlock->Status = STATUS_SUCCESS;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

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
        KeInitializeEvent(Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
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
    _SEH_TRY
    {
        /* Allocate a buffer */
        Irp->AssociatedIrp.SystemBuffer =
            ExAllocatePoolWithTag(NonPagedPool,
                                  Length,
                                  TAG_SYSB);
    }
    _SEH_HANDLE
    {
        /* Allocating failed, clean up */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if (!NT_SUCCESS(Status)) return Status;

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

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
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
            _SEH_TRY
            {
                /* Write it back to the caller */
                *IoStatusBlock = KernelIosb;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            /* Free the event */
            ExFreePool(Event);
        }
        else
        {
            /* Wait for the IRP */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           PreviousMode,
                                           FileObject->Flags & FO_ALERTABLE_IO,
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
    NTSTATUS Status = STATUS_SUCCESS;
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
    PAGED_CODE();
    CapturedByteOffset.QuadPart = 0;
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Validate User-Mode Buffers */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
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

            /* Capture and probe the key */
            if (Key) CapturedKey = ProbeForReadUlong(Key);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Check for probe failure */
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Kernel mode: capture directly */
        if (ByteOffset) CapturedByteOffset = *ByteOffset;
        if (Key) CapturedKey = *Key;
    }

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_READ_DATA,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

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
        IopLockFileObject(FileObject);

        /* Check if we don't have a byte offset avilable */
        if (!(ByteOffset) ||
            ((CapturedByteOffset.u.LowPart == FILE_USE_FILE_POINTER_POSITION) &&
             (CapturedByteOffset.u.HighPart == -1)))
        {
            /* Use the Current Byte Offset instead */
            CapturedByteOffset = FileObject->CurrentByteOffset;
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

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Clear the File Object's event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, NULL);

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
            _SEH_TRY
            {
                /* Allocate a buffer */
                Irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithTag(NonPagedPool,
                                          Length,
                                          TAG_SYSB);
            }
            _SEH_HANDLE
            {
                /* Allocating failed, clean up */
                IopCleanupAfterException(FileObject, Irp, NULL, Event);
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            if (!NT_SUCCESS(Status)) return Status;

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
            _SEH_TRY
            {
                /* Allocate an MDL */
                Mdl = IoAllocateMdl(Buffer, Length, FALSE, TRUE, Irp);
                MmProbeAndLockPages(Mdl, PreviousMode, IoWriteAccess);
            }
            _SEH_HANDLE
            {
                /* Allocating failed, clean up */
                IopCleanupAfterException(FileObject, Irp, Event, NULL);
                Status = _SEH_GetExceptionCode();
                _SEH_YIELD(return Status);
            }
            _SEH_END;

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
#if 0
    /* FIXME: VFAT SUCKS */
    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) Irp->Flags |= IRP_NOCACHE;
#endif

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
    NTSTATUS Status = STATUS_SUCCESS;
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
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the information class */
        if ((FileInformationClass >= FileMaximumInformation) ||
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
        _SEH_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the information */
            ProbeForRead(FileInformation,
                         Length,
                         (Length == sizeof(BOOLEAN)) ?
                         sizeof(BOOLEAN) : sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Check if probing failed */
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Validate the information class */
        if ((FileInformationClass >= FileMaximumInformation) ||
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

    /* Check if this is a file that was opened for Synch I/O */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        IopLockFileObject(FileObject);

        /* Check if the caller just wants the position */
        if (FileInformationClass == FilePositionInformation)
        {
            /* Protect write in SEH */
            _SEH_TRY
            {
                /* Write the offset */
                FileObject->CurrentByteOffset =
                    ((PFILE_POSITION_INFORMATION)FileInformation)->
                    CurrentByteOffset;

                /* Fill out the I/O Status Block */
                IoStatusBlock->Information = 0;
                Status = IoStatusBlock->Status = STATUS_SUCCESS;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

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
    _SEH_TRY
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
    _SEH_HANDLE
    {
        /* Allocating failed, clean up */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if (!NT_SUCCESS(Status)) return Status;

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
                    if (InterlockedCompareExchangePointer(&FileObject->
                                                          CompletionContext,
                                                          Context,
                                                          NULL))
                    {
                        /*
                         * Someone else set the completion port in the
                         * meanwhile, so dereference the port and fail.
                         */
                        ExFreePool(Context);
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
            _SEH_TRY
            {
                /* Write it back to the caller */
                *IoStatusBlock = KernelIosb;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            /* Free the event */
            ExFreePool(Event);
        }
        else
        {
            /* Wait for the IRP */
            Status = KeWaitForSingleObject(&FileObject->Event,
                                           Executive,
                                           PreviousMode,
                                           FileObject->Flags & FO_ALERTABLE_IO,
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
            ExFreePool(Event);
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
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    IO_STATUS_BLOCK KernelIosb;
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
        _SEH_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe and capture the large integers */
            CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            CapturedLength = ProbeForReadLargeInteger(Length);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Check if probing failed */
        if (!NT_SUCCESS(Status))
        {
            /* Dereference the object and return exception code */
            ObDereferenceObject(FileObject);
            return Status;
        }
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

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        IopLockFileObject(FileObject);
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
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
    _SEH_TRY
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
    _SEH_HANDLE
    {
        /* Allocating failed, clean up */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        if (LocalLength) ExFreePool(LocalLength);

        /* Get exception status */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if (!NT_SUCCESS(Status)) return Status;

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
    NTSTATUS Status = STATUS_SUCCESS;
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
    PAGED_CODE();
    CapturedByteOffset.QuadPart = 0;
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &ObjectHandleInfo);
    if (!NT_SUCCESS(Status)) return Status;

    /* Validate User-Mode Buffers */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /*
             * Check if the handle has either FILE_WRITE_DATA or
             * FILE_APPEND_DATA granted. However, if this is a named pipe,
             * make sure we don't ask for FILE_APPEND_DATA as it interferes
             * with the FILE_CREATE_PIPE_INSTANCE access right!
             */
            if (!(ObjectHandleInfo.GrantedAccess &
                 ((!(FileObject->Flags & FO_NAMED_PIPE) ?
                   FILE_APPEND_DATA : 0) | FILE_WRITE_DATA)))
            {
                /* We failed */
                ObDereferenceObject(FileObject);
                _SEH_YIELD(return STATUS_ACCESS_DENIED);
            }

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

            /* Capture and probe the key */
            if (Key) CapturedKey = ProbeForReadUlong(Key);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Check for probe failure */
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Kernel mode: capture directly */
        if (ByteOffset) CapturedByteOffset = *ByteOffset;
        if (Key) CapturedKey = *Key;
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
        IopLockFileObject(FileObject);

        /* Check if we don't have a byte offset avilable */
        if (!(ByteOffset) ||
            ((CapturedByteOffset.u.LowPart == FILE_USE_FILE_POINTER_POSITION) &&
             (CapturedByteOffset.u.HighPart == -1)))
        {
            /* Use the Current Byte Offset instead */
            CapturedByteOffset = FileObject->CurrentByteOffset;
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

    /* Get the device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Clear the File Object's event */
    KeClearEvent(&FileObject->Event);

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return IopCleanupFailedIrp(FileObject, NULL, NULL);

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
            _SEH_TRY
            {
                /* Allocate a buffer */
                Irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithTag(NonPagedPool,
                                          Length,
                                          TAG_SYSB);

                /* Copy the data into it */
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Buffer, Length);
            }
            _SEH_HANDLE
            {
                /* Allocating failed, clean up */
                IopCleanupAfterException(FileObject, Irp, Event, NULL);
                Status = _SEH_GetExceptionCode();
                _SEH_YIELD(return Status);
            }
            _SEH_END;

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
            _SEH_TRY
            {
                /* Allocate an MDL */
                Mdl = IoAllocateMdl(Buffer, Length, FALSE, TRUE, Irp);
                MmProbeAndLockPages(Mdl, PreviousMode, IoReadAccess);
            }
            _SEH_HANDLE
            {
                /* Allocating failed, clean up */
                IopCleanupAfterException(FileObject, Irp, Event, NULL);
                Status = _SEH_GetExceptionCode();
                _SEH_YIELD(return Status);
            }
            _SEH_END;
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
#if 0
    /* FIXME: VFAT SUCKS */
    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) Irp->Flags |= IRP_NOCACHE;
#endif

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
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK KernelIosb;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the information class */
        if ((FsInformationClass >= FileFsMaximumInformation) ||
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
        _SEH_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the information */
            ProbeForWrite(FsInformation, Length, sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
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

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        IopLockFileObject(FileObject);
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
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
    _SEH_TRY
    {
        /* Allocate a buffer */
        Irp->AssociatedIrp.SystemBuffer =
            ExAllocatePoolWithTag(NonPagedPool,
                                  Length,
                                  TAG_SYSB);
    }
    _SEH_HANDLE
    {
        /* Allocating failed, clean up */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if (!NT_SUCCESS(Status)) return Status;

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
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK KernelIosb;
    PAGED_CODE();
    IOTRACE(IO_API_DEBUG, "FileHandle: %p\n", FileHandle);

    /* Check if we're called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the information class */
        if ((FsInformationClass >= FileFsMaximumInformation) ||
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
        _SEH_TRY
        {
            /* Probe the I/O Status block */
            ProbeForWriteIoStatusBlock(IoStatusBlock);

            /* Probe the information */
            ProbeForRead(FsInformation, Length, sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (!NT_SUCCESS(Status)) return Status;
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

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Lock it */
        IopLockFileObject(FileObject);
    }
    else
    {
        /* Use local event */
        Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), TAG_IO);
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
    StackPtr->MajorFunction = IRP_MJ_SET_VOLUME_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Enter SEH */
    _SEH_TRY
    {
        /* Allocate a buffer */
        Irp->AssociatedIrp.SystemBuffer =
            ExAllocatePoolWithTag(NonPagedPool,
                                  Length,
                                  TAG_SYSB);

        /* Copy the data into it */
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, FsInformation, Length);
    }
    _SEH_HANDLE
    {
        /* Allocating failed, clean up */
        IopCleanupAfterException(FileObject, Irp, NULL, Event);
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if (!NT_SUCCESS(Status)) return Status;

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
