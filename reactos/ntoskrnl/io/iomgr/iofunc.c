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
#include <internal/debug.h>

#if 0
    IOTRACE(IO_IRP_DEBUG,
            "%s - Queueing IRP %p\n",
            __FUNCTION__,
            Irp);
#endif

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
IopQueryDirectoryFileCompletion(IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp,
                                IN PVOID Context)
{
    ExFreePool(Context);
    return STATUS_SUCCESS;
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
                     BOOLEAN IsDevIoCtl)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PKEVENT EventObject = NULL;
    BOOLEAN LocalEvent = FALSE;
    ULONG AccessType;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    AccessType = IO_METHOD_FROM_CTL_CODE(IoControlCode);

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));

            /* probe the input and output buffers if needed */
            if (AccessType == METHOD_BUFFERED)
            {
                if (OutputBuffer != NULL)
                {
                    ProbeForWrite(OutputBuffer, OutputBufferLength, 1);
                }
                else
                {
                    /* make sure the caller can't fake this as we depend on this */
                    OutputBufferLength = 0;
                }
            }

            if (AccessType != METHOD_NEITHER)
            {
                if (InputBuffer != NULL)
                {
                    ProbeForRead(InputBuffer, InputBufferLength, 1);
                }
                else
                {
                    /* make sure the caller can't fake this as we depend on this */
                    InputBufferLength = 0;
                }
            }
        }
        _SEH_HANDLE
        {
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
                                       (PVOID *) &FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check for sufficient access rights */
    if (PreviousMode != KernelMode)
    {
        ACCESS_MASK DesiredAccess = (ACCESS_MASK)((IoControlCode >> 14) & 3);
        if (DesiredAccess != FILE_ANY_ACCESS &&
            !RtlAreAllAccessesGranted(HandleInformation.GrantedAccess,
                                      (ACCESS_MASK)((IoControlCode >> 14) & 3)))
        {
            ObDereferenceObject (FileObject);
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
            ObDereferenceObject (FileObject);
            return Status;
        }

        /* Clear it */
        KeClearEvent(EventObject);
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
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        LocalEvent = TRUE;
    }

    /* Build the IRP */
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE,
                                        EventObject,
                                        IoStatusBlock);
    if (!Irp)
    {
        if (EventObject) ObDereferenceObject(EventObject);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set some extra settings */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = UserApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = UserApcContext;
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->MajorFunction = IsDevIoCtl ?
                              IRP_MJ_DEVICE_CONTROL :
                              IRP_MJ_FILE_SYSTEM_CONTROL;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (!LocalEvent)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
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
    return IofCallDriver(DeviceObject, Irp);
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
    return IofCallDriver(DeviceObject, Irp);
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
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    BOOLEAN LocalEvent = FALSE;
    KEVENT Event;
    NTSTATUS Status;

    Status = ObReferenceObjectByPointer(FileObject,
                                        FILE_READ_ATTRIBUTES,
                                        IoFileObjectType,
                                        KernelMode);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = KernelMode;
    Irp->AssociatedIrp.SystemBuffer = FileInformation;
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set the Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Set Parameters */
    StackPtr->Parameters.QueryFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.QueryFile.Length = Length;

    /* Call the Driver */
    Status = IoCallDriver(FileObject->DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  KernelMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Length and Status. ReturnedLength is NOT optional */
    *ReturnedLength = IoStatusBlock.Information;
    return Status;
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
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    NTSTATUS Status;

    Status = ObReferenceObjectByPointer(FileObject,
                                        FILE_READ_ATTRIBUTES,
                                        IoFileObjectType,
                                        KernelMode);
    if (!NT_SUCCESS(Status)) return(Status);

    DeviceObject = FileObject->DeviceObject;

    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!Irp)
    {
        ObDereferenceObject(FileObject);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    /* Trigger FileObject/Event dereferencing */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = KernelMode;
    Irp->AssociatedIrp.SystemBuffer = FsInformation;
    KeResetEvent( &FileObject->Event );
    Irp->UserEvent = &FileObject->Event;
    Irp->UserIosb = &IoStatusBlock;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
    StackPtr->MinorFunction = 0;
    StackPtr->Flags = 0;
    StackPtr->Control = 0;
    StackPtr->DeviceObject = DeviceObject;
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.QueryVolume.Length = Length;
    StackPtr->Parameters.QueryVolume.FsInformationClass =
        FsInformationClass;

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&FileObject->Event,
                              UserRequest,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    if (ReturnedLength) *ReturnedLength = IoStatusBlock.Information;
    return Status;
}

/*
 * @unimplemented
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
    NTSTATUS Status;

    if (FileInformationClass == FileCompletionInformation)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    Status = ObReferenceObjectByPointer(FileObject,
                                        0, /* FIXME - depends on the information class */
                                        IoFileObjectType,
                                        KernelMode);
    if (!NT_SUCCESS(Status)) return(Status);


    DeviceObject = FileObject->DeviceObject;

    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!Irp)
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Trigger FileObject/Event dereferencing */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = KernelMode;
    Irp->AssociatedIrp.SystemBuffer = FileInformation;
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = &FileObject->Event;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    KeResetEvent( &FileObject->Event );

    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_SET_INFORMATION;
    StackPtr->MinorFunction = 0;
    StackPtr->Flags = 0;
    StackPtr->Control = 0;
    StackPtr->DeviceObject = DeviceObject;
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.SetFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.SetFile.Length = Length;

    Status = IoCallDriver(FileObject->DeviceObject, Irp);
    if (Status==STATUS_PENDING)
    {
        KeWaitForSingleObject(&FileObject->Event,
                              Executive,
                              KernelMode,
                              FileObject->Flags & FO_ALERTABLE_IO,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

/* NATIVE SERVICES ***********************************************************/

/**
 * @name NtCancelIoFile
 *
 * Cancel all pending I/O operations in the current thread for specified
 * file object.
 *
 * @param FileHandle
 *        Handle to file object to cancel requests for. No specific
 *        access rights are needed.
 * @param IoStatusBlock
 *        Pointer to status block which is filled with final completition
 *        status on successful return.
 *
 * @return Status.
 *
 * @implemented
 */
NTSTATUS
NTAPI
NtCancelIoFile(IN HANDLE FileHandle,
               OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    PFILE_OBJECT FileObject;
    PETHREAD Thread;
    PIRP Irp;
    KIRQL OldIrql;
    BOOLEAN OurIrpsInList = FALSE;
    LARGE_INTEGER Interval;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status)) return Status;
    }

    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* IRP cancellations are synchronized at APC_LEVEL. */
    OldIrql = KfRaiseIrql(APC_LEVEL);

    /*
    * Walk the list of active IRPs and cancel the ones that belong to
    * our file object.
    */

    Thread = PsGetCurrentThread();

    LIST_FOR_EACH(Irp, &Thread->IrpList, IRP, ThreadListEntry)
    {
        if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
        {
            IoCancelIrp(Irp);
            /* Don't break here, we want to cancel all IRPs for the file object. */
            OurIrpsInList = TRUE;
        }
    }

    KfLowerIrql(OldIrql);

    while (OurIrpsInList)
    {
        OurIrpsInList = FALSE;

        /* Wait a short while and then look if all our IRPs were completed. */
        Interval.QuadPart = -1000000; /* 100 milliseconds */
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);

        OldIrql = KfRaiseIrql(APC_LEVEL);

        /*
        * Look in the list if all IRPs for the specified file object
        * are completed (or cancelled). If someone sends a new IRP
        * for our file object while we're here we can happily loop
        * forever.
        */

        LIST_FOR_EACH(Irp, &Thread->IrpList, IRP, ThreadListEntry)
        {
            if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
            {
                OurIrpsInList = TRUE;
                break;
            }
        }

        KfLowerIrql(OldIrql);
    }

    _SEH_TRY
    {
        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = 0;
        Status = STATUS_SUCCESS;
    }
    _SEH_HANDLE
    {
    
    }
    _SEH_END;

    ObDereferenceObject(FileObject);
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    UNIMPLEMENTED;
    return(STATUS_NOT_IMPLEMENTED);
}

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
    PFILE_OBJECT FileObject = NULL;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject;
    KEVENT Event;
    BOOLEAN LocalEvent = FALSE;
    ACCESS_MASK DesiredAccess = FILE_WRITE_DATA;
    OBJECT_HANDLE_INFORMATION ObjectHandleInfo;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PAGED_CODE();

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Get the File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &ObjectHandleInfo);
    if (!NT_SUCCESS(Status)) return(Status);

    /* check if the handle has either FILE_WRITE_DATA or FILE_APPEND_DATA was
       granted. However, if this is a named pipe, make sure we don't ask for
       FILE_APPEND_DATA as it interferes with the FILE_CREATE_PIPE_INSTANCE
       access right! */
    if (!(FileObject->Flags & FO_NAMED_PIPE))
        DesiredAccess |= FILE_APPEND_DATA;
    if (!RtlAreAnyAccessesGranted(ObjectHandleInfo.GrantedAccess,
                                  DesiredAccess))
    {
        ObDereferenceObject(FileObject);
        return STATUS_ACCESS_DENIED;
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
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set up the IRP */
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    StackPtr->FileObject = FileObject;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock->Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
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
                            IN HANDLE Event OPTIONAL,
                            IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                            IN PVOID ApcContext OPTIONAL,
                            OUT PIO_STATUS_BLOCK IoStatusBlock,
                            OUT PVOID Buffer,
                            IN ULONG BufferSize,
                            IN ULONG CompletionFilter,
                            IN BOOLEAN WatchTree)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IoStack;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            if(BufferSize) ProbeForWrite(Buffer, BufferSize, sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_LIST_DIRECTORY,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    if (Status != STATUS_SUCCESS) return(Status);

    DeviceObject = FileObject->DeviceObject;

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        ObDereferenceObject(FileObject);
        return STATUS_UNSUCCESSFUL;
    }

    if (!Event) Event = &FileObject->Event;

    /* Trigger FileObject/Event dereferencing */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->UserEvent = Event;
    KeResetEvent( Event );
    Irp->UserBuffer = Buffer;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    IoStack->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
    IoStack->Flags = 0;
    IoStack->Control = 0;
    IoStack->DeviceObject = DeviceObject;
    IoStack->FileObject = FileObject;

    if (WatchTree) IoStack->Flags = SL_WATCH_TREE;

    IoStack->Parameters.NotifyDirectory.CompletionFilter = CompletionFilter;
    IoStack->Parameters.NotifyDirectory.Length = BufferSize;

    Status = IoCallDriver(FileObject->DeviceObject,Irp);

    /* FIXME: Should we wait here or not for synchronously opened files? */

    return Status;
}

/*
 * @unimplemented
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
    PFILE_OBJECT FileObject = NULL;
    PLARGE_INTEGER LocalLength = NULL;
    PIRP Irp = NULL;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    LARGE_INTEGER CapturedByteOffset, CapturedLength;
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    PAGED_CODE();

    CapturedByteOffset.QuadPart = 0;
    CapturedLength.QuadPart = 0;

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        /* Must have either FILE_READ_DATA or FILE_WRITE_DATA access unless
           we're in KernelMode! */
        if (!(HandleInformation.GrantedAccess & (FILE_WRITE_DATA | FILE_READ_DATA)))
        {
            DPRINT1("Invalid access rights\n");
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }

        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            CapturedLength = ProbeForReadLargeInteger(Length);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        CapturedByteOffset = *ByteOffset;
        CapturedLength = *Length;
    }

    /* Get Event Object */
    if (EventHandle)
    {
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID *)&Event,
                                           NULL);
        if (Status != STATUS_SUCCESS) return(Status);
        KeClearEvent(Event);
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
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate local buffer */
    LocalLength = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(LARGE_INTEGER),
                                        TAG_LOCK);
    if (!LocalLength)
    {
        IoFreeIrp(Irp);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *LocalLength = CapturedLength;

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_LOCK_CONTROL;
    StackPtr->MinorFunction = IRP_MN_LOCK;
    StackPtr->FileObject = FileObject;

    /* Set Parameters */
    StackPtr->Parameters.LockControl.Length = LocalLength;
    StackPtr->Parameters.LockControl.ByteOffset = CapturedByteOffset;
    StackPtr->Parameters.LockControl.Key = Key;

    /* Set Flags */
    if (FailImmediately) StackPtr->Flags = SL_FAIL_IMMEDIATELY;
    if (ExclusiveLock) StackPtr->Flags |= SL_EXCLUSIVE_LOCK;

    /* Call the Driver */
    FileObject->LockOperation = TRUE;
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (!LocalEvent)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryDirectoryFile(IN HANDLE FileHandle,
                     IN HANDLE PEvent OPTIONAL,
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
    PFILE_OBJECT FileObject = NULL;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN LocalEvent = FALSE;
    PKEVENT Event = NULL;
    PUNICODE_STRING SearchPattern = NULL;
    PAGED_CODE();

    /* Validate User-Mode Buffers */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            ProbeForWrite(FileInformation,
                          Length,
                          sizeof(ULONG));
            if (FileName)
            {
                UNICODE_STRING CapturedFileName;

                CapturedFileName = ProbeForReadUnicodeString(FileName);
                ProbeForRead(CapturedFileName.Buffer,
                             CapturedFileName.MaximumLength,
                             1);
                SearchPattern = ExAllocatePool(NonPagedPool, CapturedFileName.Length + sizeof(WCHAR) + sizeof(UNICODE_STRING));
                if (SearchPattern == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH_LEAVE;
                }
                SearchPattern->Buffer = (PWCHAR)((ULONG_PTR)SearchPattern + sizeof(UNICODE_STRING));
                SearchPattern->MaximumLength = CapturedFileName.Length + sizeof(WCHAR);
                RtlCopyUnicodeString(SearchPattern, &CapturedFileName);
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) 
        {
            goto Cleanup;
        }
    }

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_LIST_DIRECTORY,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Get Event Object */
    if (PEvent)
    {
        Status = ObReferenceObjectByHandle(PEvent,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID *)&Event,
                                           NULL);
        if (!NT_SUCCESS(Status)) 
        {
            goto Cleanup;
        }

        KeClearEvent(Event);
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
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->UserBuffer = FileInformation;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    StackPtr->MinorFunction = IRP_MN_QUERY_DIRECTORY;

    /* Set Parameters */
    StackPtr->Parameters.QueryDirectory.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.QueryDirectory.FileName = SearchPattern ? SearchPattern : FileName;
    StackPtr->Parameters.QueryDirectory.FileIndex = 0;
    StackPtr->Parameters.QueryDirectory.Length = Length;
    StackPtr->Flags = 0;
    if (RestartScan) StackPtr->Flags = SL_RESTART_SCAN;
    if (ReturnSingleEntry) StackPtr->Flags |= SL_RETURN_SINGLE_ENTRY;

    if (SearchPattern)
    {
        IoSetCompletionRoutine(Irp,
                               IopQueryDirectoryFileCompletion,
                               SearchPattern,
                               TRUE,
                               TRUE,
                               TRUE);
    }

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (!LocalEvent)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    return Status;

Cleanup:
    if (FileObject) ObDereferenceObject(FileObject);
    if (Event) ObDereferenceObject(Event);
    if (SearchPattern) ExFreePool(SearchPattern);

    /* Return the Status */
    return Status;
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
                       IN PIO_STATUS_BLOCK IoStatusBlock,
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
    KEVENT Event;
    BOOLEAN LocalEvent = FALSE;
    BOOLEAN Failed = FALSE;

    /* Reference the Handle */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check information class specific access rights */
    switch (FileInformationClass)
    {
        case FileBasicInformation:
            if (!(HandleInformation.GrantedAccess & FILE_READ_ATTRIBUTES))
                Failed = TRUE;
            break;

        case FilePositionInformation:
            if (!(HandleInformation.GrantedAccess & (FILE_READ_DATA | FILE_WRITE_DATA)) ||
                !(FileObject->Flags & FO_SYNCHRONOUS_IO))
                Failed = TRUE;
            break;

        default:
            break;
    }

    if (Failed)
    {
        ObDereferenceObject(FileObject);
        return STATUS_ACCESS_DENIED;
    }

    if (FileInformationClass == FilePositionInformation)
    {
       if (Length < sizeof(FILE_POSITION_INFORMATION))
       {
          Status = STATUS_BUFFER_OVERFLOW;
       }
       else
       {
          _SEH_TRY
          {
             ((PFILE_POSITION_INFORMATION)FileInformation)->CurrentByteOffset = FileObject->CurrentByteOffset;
             IoStatusBlock->Information = sizeof(FILE_POSITION_INFORMATION);
             Status = IoStatusBlock->Status = STATUS_SUCCESS;
          }
          _SEH_HANDLE
          {
             Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
       }
       ObDereferenceObject(FileObject);
       return Status;
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

    if (FileInformationClass == FileAlignmentInformation)
    {
       if (Length < sizeof(FILE_ALIGNMENT_INFORMATION))
       {
          Status = STATUS_BUFFER_OVERFLOW;
       }
       else
       {
          _SEH_TRY
          {
             ((PFILE_ALIGNMENT_INFORMATION)FileInformation)->AlignmentRequirement = DeviceObject->AlignmentRequirement;
             IoStatusBlock->Information = sizeof(FILE_ALIGNMENT_INFORMATION);
             Status = IoStatusBlock->Status = STATUS_SUCCESS;
          }
          _SEH_HANDLE
          {
             Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
       }
       ObDereferenceObject(FileObject);
       return Status;
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate the System Buffer */
    Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                            Length,
                                                            TAG_SYSB);
    if (!Irp->AssociatedIrp.SystemBuffer)
    {
        IoFreeIrp(Irp);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set up the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->UserBuffer = FileInformation;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;
    Irp->Flags |= (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Set the Parameters */
    StackPtr->Parameters.QueryFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.QueryFile.Length = Length;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock->Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
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
    PIRP Irp = NULL;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PKEVENT EventObject = NULL;
    LARGE_INTEGER CapturedByteOffset;
    ULONG CapturedKey = 0;
    BOOLEAN Synchronous = FALSE;
    PAGED_CODE();

    CapturedByteOffset.QuadPart = 0;

    /* Validate User-Mode Buffers */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            ProbeForWrite(Buffer,
                          Length,
                          1);

            if (ByteOffset)
            {
                CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            }

            if (Key) CapturedKey = ProbeForReadUlong(Key);

            /* FIXME - probe other pointers and capture information */
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
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

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        if (ByteOffset == NULL ||
            (CapturedByteOffset.u.LowPart == FILE_USE_FILE_POINTER_POSITION &&
             CapturedByteOffset.u.HighPart == -1))
        {
            /* Use the Current Byte OFfset */
            CapturedByteOffset = FileObject->CurrentByteOffset;
        }

        Synchronous = TRUE;
    }
    else if (ByteOffset == NULL && !(FileObject->Flags & FO_NAMED_PIPE))
    {
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
            ObDereferenceObject(FileObject);
            return Status;
        }
        KeClearEvent(EventObject);
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

    KeClearEvent(&FileObject->Event);

    /* Create the IRP */
    _SEH_TRY
    {
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                           DeviceObject,
                                           Buffer,
                                           Length,
                                           &CapturedByteOffset,
                                           EventObject,
                                           IoStatusBlock);

        if (Irp == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Cleanup if IRP Allocation Failed */
    if (!NT_SUCCESS(Status))
    {
        if (Event) ObDereferenceObject(EventObject);
        ObDereferenceObject(FileObject);
        return Status;
    }

    /* Set up IRP Data */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
    Irp->Flags |= IRP_READ_OPERATION;
#if 0
    /* FIXME:
     *    Vfat doesn't handle non cached files correctly.
     */     
    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) Irp->Flags |= IRP_NOCACHE;
#endif      

    /* Setup Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.Read.Key = CapturedKey;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (Synchronous)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}

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
NtSetInformationFile(HANDLE FileHandle,
                     PIO_STATUS_BLOCK IoStatusBlock,
                     PVOID FileInformation,
                     ULONG Length,
                     FILE_INFORMATION_CLASS FileInformationClass)
{
    OBJECT_HANDLE_INFORMATION HandleInformation;
    PIO_STACK_LOCATION StackPtr;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    KEVENT Event;
    BOOLEAN LocalEvent = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    BOOLEAN Failed = FALSE;

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            if (IoStatusBlock)
            {
                ProbeForWrite(IoStatusBlock,
                              sizeof(IO_STATUS_BLOCK),
                              sizeof(ULONG));
            }

            if (Length) ProbeForRead(FileInformation, Length, 1);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Get the file object from the file handle */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check information class specific access rights */
    switch (FileInformationClass)
    {
        case FileBasicInformation:
            if (!(HandleInformation.GrantedAccess & FILE_WRITE_ATTRIBUTES))
                Failed = TRUE;
            break;

        case FileDispositionInformation:
            if (!(HandleInformation.GrantedAccess & DELETE))
                Failed = TRUE;
            break;

        case FilePositionInformation:
            if (!(HandleInformation.GrantedAccess & (FILE_READ_DATA | FILE_WRITE_DATA)) ||
                !(FileObject->Flags & FO_SYNCHRONOUS_IO))
                Failed = TRUE;
            break;

        case FileEndOfFileInformation:
            if (!(HandleInformation.GrantedAccess & FILE_WRITE_DATA))
                Failed = TRUE;
            break;

        default:
            break;
    }

    if (Failed)
    {
        ObDereferenceObject(FileObject);
        return STATUS_ACCESS_DENIED;
    }

    if (FileInformationClass == FilePositionInformation)
    {
       if (Length < sizeof(FILE_POSITION_INFORMATION))
       {
          Status = STATUS_BUFFER_OVERFLOW;
       }
       else
       {
          _SEH_TRY
          {
             FileObject->CurrentByteOffset = ((PFILE_POSITION_INFORMATION)FileInformation)->CurrentByteOffset;
             IoStatusBlock->Information = 0;
             Status = IoStatusBlock->Status = STATUS_SUCCESS;
          }
          _SEH_HANDLE
          {
             Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
       }
       ObDereferenceObject(FileObject);
       return Status;
    }

    /* FIXME: Later, we can implement a lot of stuff here and avoid a driver call */
    /* Handle IO Completion Port quickly */
    if (FileInformationClass == FileCompletionInformation)
    {
        PVOID Queue;
        PFILE_COMPLETION_INFORMATION CompletionInfo = FileInformation;
        PIO_COMPLETION_CONTEXT Context;
        
        if (FileObject->Flags & FO_SYNCHRONOUS_IO || FileObject->CompletionContext != NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            if (Length < sizeof(FILE_COMPLETION_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                /* Reference the Port */
                Status = ObReferenceObjectByHandle(CompletionInfo->Port, /* FIXME - protect with SEH! */
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
                                                    TAG('I', 'o', 'C', 'p'));

                    if (Context != NULL)
                    {
                        /* Set the Data */
                        Context->Key = CompletionInfo->Key; /* FIXME - protect with SEH! */
                        Context->Port = Queue;
                        
                        if (InterlockedCompareExchangePointer(&FileObject->CompletionContext,
                                                              Context,
                                                              NULL) != NULL)
                        {
                            /* someone else set the completion port in the
                               meanwhile, fail */
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
        }

        /* Complete the I/O */
        ObDereferenceObject(FileObject);
        return Status;
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
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate the System Buffer */
    Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                            Length,
                                                            TAG_SYSB);
    if (!Irp->AssociatedIrp.SystemBuffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto failfreeirp;
    }

    /* Copy the data inside */
    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* no need to probe again */
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                          FileInformation,
                          Length);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(Irp->AssociatedIrp.SystemBuffer,
                              TAG_SYSB);
            Irp->AssociatedIrp.SystemBuffer = NULL;
failfreeirp:
            IoFreeIrp(Irp);
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                      FileInformation,
                      Length);
    }

    /* Set up the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
    Irp->Flags |= (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_SET_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Set the Parameters */
    StackPtr->Parameters.SetFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.SetFile.Length = Length;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            _SEH_TRY
            {
                Status = IoStatusBlock->Status;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            _SEH_TRY
            {
                Status = FileObject->FinalStatus;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtSetQuotaInformationFile(HANDLE FileHandle,
                          PIO_STATUS_BLOCK IoStatusBlock,
                          PVOID Buffer,
                          ULONG BufferLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtUnlockFile(IN HANDLE FileHandle,
             OUT PIO_STATUS_BLOCK IoStatusBlock,
             IN PLARGE_INTEGER ByteOffset,
             IN PLARGE_INTEGER Length,
             IN ULONG Key OPTIONAL)
{
    PFILE_OBJECT FileObject = NULL;
    PLARGE_INTEGER LocalLength = NULL;
    PIRP Irp = NULL;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    KEVENT Event;
    BOOLEAN LocalEvent = FALSE;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER CapturedByteOffset, CapturedLength;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    PAGED_CODE();

    CapturedByteOffset.QuadPart = 0;
    CapturedLength.QuadPart = 0;

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        /* Must have either FILE_READ_DATA or FILE_WRITE_DATA access unless we're
           in KernelMode! */
        if (!(HandleInformation.GrantedAccess & (FILE_WRITE_DATA | FILE_READ_DATA)))
        {
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }

        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            CapturedLength = ProbeForReadLargeInteger(Length);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
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
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate local buffer */
    LocalLength = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(LARGE_INTEGER),
                                        TAG_LOCK);
    if (!LocalLength)
    {
        IoFreeIrp(Irp);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *LocalLength = CapturedLength;

    /* Set up the IRP */
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_LOCK_CONTROL;
    StackPtr->MinorFunction = IRP_MN_UNLOCK_SINGLE;
    StackPtr->FileObject = FileObject;

    /* Set Parameters */
    StackPtr->Parameters.LockControl.Length = LocalLength;
    StackPtr->Parameters.LockControl.ByteOffset = CapturedByteOffset;
    StackPtr->Parameters.LockControl.Key = Key;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock->Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
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
    OBJECT_HANDLE_INFORMATION ObjectHandleInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject;
    PIRP Irp = NULL;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    BOOLEAN Synchronous = FALSE;
    PKEVENT EventObject = NULL;
    LARGE_INTEGER CapturedByteOffset;
    ULONG CapturedKey = 0;
    ACCESS_MASK DesiredAccess = FILE_WRITE_DATA;
    PAGED_CODE();

    CapturedByteOffset.QuadPart = 0;

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &ObjectHandleInfo);
    if (!NT_SUCCESS(Status)) return Status;

    /* If this is a named pipe, make sure we don't ask for FILE_APPEND_DATA as it
       overlaps with the FILE_CREATE_PIPE_INSTANCE access right! */
    if (!(FileObject->Flags & FO_NAMED_PIPE))
        DesiredAccess |= FILE_APPEND_DATA;

    /* Validate User-Mode Buffers */
    if (PreviousMode != KernelMode)
    {
        /* check if the handle has either FILE_WRITE_DATA or FILE_APPEND_DATA was
           granted. */
        if (!RtlAreAnyAccessesGranted(ObjectHandleInfo.GrantedAccess,
                                      DesiredAccess))
        {
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }

        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            ProbeForRead(Buffer, Length, 1);

            if (ByteOffset)
            {
                CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            }

            if (Key) CapturedKey = ProbeForReadUlong(Key);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        if (ByteOffset) CapturedByteOffset = *ByteOffset;
        if (Key) CapturedKey = *Key;
    }

    /* check if this is an append operation */
    if ((ObjectHandleInfo.GrantedAccess & DesiredAccess) == FILE_APPEND_DATA)
    {
        /* Give the drivers something to understand */
        CapturedByteOffset.u.LowPart = FILE_WRITE_TO_END_OF_FILE;
        CapturedByteOffset.u.HighPart = -1;
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        if (ByteOffset == NULL ||
            (CapturedByteOffset.u.LowPart == FILE_USE_FILE_POINTER_POSITION &&
             CapturedByteOffset.u.HighPart == -1))
        {
            /* Use the Current Byte OFfset */
            CapturedByteOffset = FileObject->CurrentByteOffset;
        }

        Synchronous = TRUE;
    }
    else if (ByteOffset == NULL && !(FileObject->Flags & FO_NAMED_PIPE))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we got an event */
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
            ObDereferenceObject(FileObject);
            return Status;
        }
        KeClearEvent(EventObject);
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

    KeClearEvent(&FileObject->Event);

    /* Build the IRP */
    _SEH_TRY
    {
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                           DeviceObject,
                                           Buffer,
                                           Length,
                                           &CapturedByteOffset,
                                           EventObject,
                                           IoStatusBlock);
        if (Irp == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Cleanup on failure */
    if (!NT_SUCCESS(Status))
    {
        if (Event) ObDereferenceObject(&EventObject);
        ObDereferenceObject(FileObject);
        return Status;
    }

   /* Set up IRP Data */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
    Irp->Flags |= IRP_WRITE_OPERATION;
#if 0    
    /* FIXME:
     *    Vfat doesn't handle non cached files correctly.
     */     
    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) Irp->Flags |= IRP_NOCACHE;
#endif    

    /* Setup Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.Write.Key = CapturedKey;
    if (FileObject->Flags & FO_WRITE_THROUGH) StackPtr->Flags = SL_WRITE_THROUGH;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (Synchronous)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
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
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION StackPtr;
    PVOID SystemBuffer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            if (IoStatusBlock)
            {
                ProbeForWrite(IoStatusBlock,
                              sizeof(IO_STATUS_BLOCK),
                              sizeof(ULONG));
            }

            if (Length) ProbeForWrite(FsInformation, Length, 1);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    Status = ObReferenceObjectByHandle(FileHandle,
                                       0, /* FIXME - depends on the information class! */
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    DeviceObject = FileObject->DeviceObject;

    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (!Irp)
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_SYSB);
    if (!SystemBuffer)
    {
        IoFreeIrp(Irp);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Trigger FileObject/Event dereferencing */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    Irp->RequestorMode = PreviousMode;
    Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
    KeResetEvent( &FileObject->Event );
    Irp->UserEvent = &FileObject->Event;
    Irp->UserIosb = IoStatusBlock;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
    StackPtr->MinorFunction = 0;
    StackPtr->Flags = 0;
    StackPtr->Control = 0;
    StackPtr->DeviceObject = DeviceObject;
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.QueryVolume.Length = Length;
    StackPtr->Parameters.QueryVolume.FsInformationClass =
    FsInformationClass;

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&FileObject->Event,
                              UserRequest,
                              PreviousMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock->Status;
     }

    if (NT_SUCCESS(Status))
        {
        _SEH_TRY
        {
            RtlCopyMemory(FsInformation,
                          SystemBuffer,
                          IoStatusBlock->Information);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
     }

    ExFreePool(SystemBuffer);
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
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION StackPtr;
    PVOID SystemBuffer;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            if (IoStatusBlock)
            {
                ProbeForWrite(IoStatusBlock,
                              sizeof(IO_STATUS_BLOCK),
                              sizeof(ULONG));
            }

            if (Length) ProbeForRead(FsInformation, Length, 1);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status)) return Status;
    }

    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_WRITE_ATTRIBUTES,
                                       NULL,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (Status != STATUS_SUCCESS) return Status;

    DeviceObject = FileObject->DeviceObject;

    Irp = IoAllocateIrp(DeviceObject->StackSize,TRUE);
    if (!Irp)
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SystemBuffer = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_SYSB);
    if (!SystemBuffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto failfreeirp;
    }

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* no need to probe again */
            RtlCopyMemory(SystemBuffer, FsInformation, Length);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(SystemBuffer, TAG_SYSB);
failfreeirp:
            IoFreeIrp(Irp);
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        RtlCopyMemory(SystemBuffer, FsInformation, Length);
    }

    /* Trigger FileObject/Event dereferencing */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->AssociatedIrp.SystemBuffer = SystemBuffer;
    KeResetEvent( &FileObject->Event );
    Irp->UserEvent = &FileObject->Event;
    Irp->UserIosb = IoStatusBlock;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_SET_VOLUME_INFORMATION;
    StackPtr->MinorFunction = 0;
    StackPtr->Flags = 0;
    StackPtr->Control = 0;
    StackPtr->DeviceObject = DeviceObject;
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.SetVolume.Length = Length;
    StackPtr->Parameters.SetVolume.FsInformationClass =
        FsInformationClass;

    Status = IoCallDriver(DeviceObject,Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&FileObject->Event,
                              UserRequest,
                              PreviousMode,
                              FALSE,
                              NULL);
        _SEH_TRY
        {
            Status = IoStatusBlock->Status;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    ExFreePool(SystemBuffer);
    return Status;
}
