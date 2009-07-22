/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/factory.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsReferenceBusObject(
    IN  KSDEVICE_HEADER Header)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsDereferenceBusObject(
    IN  KSDEVICE_HEADER Header)
{
    UNIMPLEMENTED;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchQuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKSOBJECT_CREATE_ITEM CreateItem;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    ULONG Length;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    if (!CreateItem || !CreateItem->SecurityDescriptor)
    {
        /* no create item */
        Irp->IoStatus.Status = STATUS_NO_SECURITY_ON_OBJECT;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SECURITY_ON_OBJECT;
    }


    /* get input length */
    Length = IoStack->Parameters.QuerySecurity.Length;

    /* clone the security descriptor */
    Status = SeQuerySecurityDescriptorInfo(&IoStack->Parameters.QuerySecurity.SecurityInformation, (PSECURITY_DESCRIPTOR)Irp->UserBuffer, &Length, &CreateItem->SecurityDescriptor);

    DPRINT("SeQuerySecurityDescriptorInfo Status %x\n", Status);
    /* store result */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKSOBJECT_CREATE_ITEM CreateItem;
    PIO_STACK_LOCATION IoStack;
    PGENERIC_MAPPING Mapping;
    PSECURITY_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    if (!CreateItem || !CreateItem->SecurityDescriptor)
    {
        /* no create item */
        Irp->IoStatus.Status = STATUS_NO_SECURITY_ON_OBJECT;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SECURITY_ON_OBJECT;
    }

    /* backup old descriptor */
    Descriptor = CreateItem->SecurityDescriptor;

    /* get generic mapping */
    Mapping = IoGetFileObjectGenericMapping();

    /* change security descriptor */
    Status = SeSetSecurityDescriptorInfo(NULL, /*FIXME */
                                         &IoStack->Parameters.SetSecurity.SecurityInformation,
                                         IoStack->Parameters.SetSecurity.SecurityDescriptor,
                                         &CreateItem->SecurityDescriptor,
                                         NonPagedPool,
                                         Mapping);

    if (NT_SUCCESS(Status))
    {
        /* free old descriptor */
        ExFreePool(Descriptor);

       /* mark create item as changed */
       CreateItem->Flags |= KSCREATE_ITEM_SECURITYCHANGED;
    }

    /* store result */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSpecificProperty(
    IN  PIRP Irp,
    IN  PFNKSHANDLER Handler)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSpecificMethod(
    IN  PIRP Irp,
    IN  PFNKSHANDLER Handler)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsReadFile(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN  ULONG Length,
    IN  ULONG Key OPTIONAL,
    IN  KPROCESSOR_MODE RequestorMode)
{
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    NTSTATUS Status;
    BOOLEAN Result;
    KEVENT LocalEvent;

    if (Event)
    {
        /* make sure event is reset */
        KeClearEvent(Event);
    }

    if (RequestorMode == UserMode)
    {
        /* probe the user buffer */
        _SEH2_TRY
        {
            ProbeForWrite(Buffer, Length, sizeof(UCHAR));
            Status = STATUS_SUCCESS;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Exception, get the error code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

         if (!NT_SUCCESS(Status))
         {
             DPRINT1("Invalid user buffer provided\n");
             return Status;
         }
    }

    /* get corresponding device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* fast-io read is only available for kernel mode clients */
    if (RequestorMode == KernelMode && ExGetPreviousMode() == KernelMode &&
        DeviceObject->DriverObject->FastIoDispatch->FastIoRead)
    {
        /* call fast io write */
        Result = DeviceObject->DriverObject->FastIoDispatch->FastIoRead(FileObject, &FileObject->CurrentByteOffset, Length, TRUE, Key, Buffer, IoStatusBlock, DeviceObject);

        if (Result && NT_SUCCESS(IoStatusBlock->Status))
        {
            /* request was handeled and succeeded */
            return STATUS_SUCCESS;
        }
    }

    /* do the slow way */
    if (!Event)
    {
        /* initialize temp event */
        KeInitializeEvent(&LocalEvent, NotificationEvent, FALSE);
        Event = &LocalEvent;
    }

    /* build the irp packet */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, DeviceObject, Buffer, Length, &FileObject->CurrentByteOffset, Event, IoStatusBlock);
    if (!Irp)
    {
        /* not enough resources */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* send the packet */
    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        /* operation is pending, is sync file object */
        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
        {
            /* it is so wait */
            KeWaitForSingleObject(Event, Executive, RequestorMode, FALSE, NULL);
            Status = IoStatusBlock->Status;
        }
    }
    /* return result */
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsWriteFile(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PVOID Buffer,
    IN  ULONG Length,
    IN  ULONG Key OPTIONAL,
    IN  KPROCESSOR_MODE RequestorMode)
{
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    NTSTATUS Status;
    BOOLEAN Result;
    KEVENT LocalEvent;

    if (Event)
    {
        /* make sure event is reset */
        KeClearEvent(Event);
    }

    if (RequestorMode == UserMode)
    {
        /* probe the user buffer */
        _SEH2_TRY
        {
            ProbeForRead(Buffer, Length, sizeof(UCHAR));
            Status = STATUS_SUCCESS;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Exception, get the error code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

         if (!NT_SUCCESS(Status))
         {
             DPRINT1("Invalid user buffer provided\n");
             return Status;
         }
    }

    /* get corresponding device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* fast-io write is only available for kernel mode clients */
    if (RequestorMode == KernelMode && ExGetPreviousMode() == KernelMode &&
        DeviceObject->DriverObject->FastIoDispatch->FastIoWrite)
    {
        /* call fast io write */
        Result = DeviceObject->DriverObject->FastIoDispatch->FastIoWrite(FileObject, &FileObject->CurrentByteOffset, Length, TRUE, Key, Buffer, IoStatusBlock, DeviceObject);

        if (Result && NT_SUCCESS(IoStatusBlock->Status))
        {
            /* request was handeled and succeeded */
            return STATUS_SUCCESS;
        }
    }

    /* do the slow way */
    if (!Event)
    {
        /* initialize temp event */
        KeInitializeEvent(&LocalEvent, NotificationEvent, FALSE);
        Event = &LocalEvent;
    }

    /* build the irp packet */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, DeviceObject, Buffer, Length, &FileObject->CurrentByteOffset, Event, IoStatusBlock);
    if (!Irp)
    {
        /* not enough resources */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* send the packet */
    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        /* operation is pending, is sync file object */
        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
        {
            /* it is so wait */
            KeWaitForSingleObject(Event, Executive, RequestorMode, FALSE, NULL);
            Status = IoStatusBlock->Status;
        }
    }
    /* return result */
    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsQueryInformationFile(
    IN  PFILE_OBJECT FileObject,
    OUT PVOID FileInformation,
    IN  ULONG Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass)
{
    PDEVICE_OBJECT DeviceObject;
    PFAST_IO_DISPATCH FastIoDispatch;
    IO_STATUS_BLOCK IoStatus;

    /* get related file object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* get fast i/o table */
    FastIoDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* is there a fast table */
    if (FastIoDispatch)
    {
        /* check the class */
        if (FileInformationClass == FileBasicInformation)
        {
            /* use FastIoQueryBasicInfo routine */
            if (FastIoDispatch->FastIoQueryBasicInfo)
            {
                return FastIoDispatch->FastIoQueryBasicInfo(FileObject, TRUE, (PFILE_BASIC_INFORMATION)FileInformation, &IoStatus, DeviceObject);
            }
        }
        else if (FileInformationClass == FileStandardInformation)
        {
            /* use FastIoQueryBasicInfo routine */
            if (FastIoDispatch->FastIoQueryBasicInfo)
            {
                return FastIoDispatch->FastIoQueryStandardInfo(FileObject, TRUE, (PFILE_STANDARD_INFORMATION)FileInformation, &IoStatus, DeviceObject);
            }
        }
    }

    /* Implement Me */

    return STATUS_UNSUCCESSFUL;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsSetInformationFile(
    IN  PFILE_OBJECT FileObject,
    IN  PVOID FileInformation,
    IN  ULONG Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PVOID Buffer;
    KEVENT Event;
    LARGE_INTEGER Offset;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;

    /* get related device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* copy file information */
    Buffer = AllocateItem(NonPagedPool, Length);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    _SEH2_TRY
    {
        ProbeForRead(Buffer, Length, sizeof(UCHAR));
        RtlMoveMemory(Buffer, FileInformation, Length);
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Exception, get the error code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        /* invalid user buffer */
        FreeItem(Buffer);
        return Status;
    }

    /* initialize the event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* zero offset */
    Offset.QuadPart = 0LL;

    /* build the irp */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SET_INFORMATION, DeviceObject, NULL, 0, &Offset, &Event, &IoStatus);

    if (!Irp)
    {
        /* failed to allocate irp */
        FreeItem(Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);

    /* set irp parameters */
    IoStack->Parameters.SetFile.FileInformationClass = FileInformationClass;
    IoStack->Parameters.SetFile.Length = Length;
    IoStack->Parameters.SetFile.FileObject = FileObject;
    Irp->AssociatedIrp.SystemBuffer = Buffer;
    Irp->UserBuffer = FileInformation;

    /* dispatch the irp */
    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        /* wait untill the operation has completed */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        /* is a sync file object */
        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
            Status = FileObject->FinalStatus;
        else
            Status = IoStatus.Status;
    }
    /* done */
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamIo(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    IN  PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN  PVOID CompletionContext OPTIONAL,
    IN  KSCOMPLETION_INVOCATION CompletionInvocationFlags OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  OUT PVOID StreamHeaders,
    IN  ULONG Length,
    IN  ULONG Flags,
    IN  KPROCESSOR_MODE RequestorMode)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT DeviceObject;
    ULONG Code;
    NTSTATUS Status;
    LARGE_INTEGER Offset;
    PKSIOBJECT_HEADER ObjectHeader;


    if (Flags == KSSTREAM_READ)
        Code = IRP_MJ_READ;
    else if (Flags == KSSTREAM_WRITE)
        Code = IRP_MJ_WRITE;
    else
        return STATUS_INVALID_PARAMETER;

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject)
        return STATUS_INVALID_PARAMETER;

    if (Event)
    {
        KeResetEvent(Event);
    }

    //ASSERT(DeviceObject->DeviceType == FILE_DEVICE_KS);
    ObjectHeader = (PKSIOBJECT_HEADER)FileObject->FsContext;
    ASSERT(ObjectHeader);
    if (Code == IRP_MJ_READ)
    {
        if (ObjectHeader->DispatchTable.FastRead)
        {
            if (ObjectHeader->DispatchTable.FastRead(FileObject, NULL, Length, FALSE, 0, StreamHeaders, IoStatusBlock, DeviceObject))
            {
                return STATUS_SUCCESS;
            }
        }
    }
    else
    {
        if (ObjectHeader->DispatchTable.FastWrite)
        {
            if (ObjectHeader->DispatchTable.FastWrite(FileObject, NULL, Length, FALSE, 0, StreamHeaders, IoStatusBlock, DeviceObject))
            {
                return STATUS_SUCCESS;
            }
        }
    }

    Offset.QuadPart = 0LL;
    Irp = IoBuildSynchronousFsdRequest(Code, DeviceObject, (PVOID)StreamHeaders, Length, &Offset, Event, IoStatusBlock);
    if (!Irp)
    {
        return STATUS_UNSUCCESSFUL;
    }


    if (CompletionRoutine)
    {
        IoSetCompletionRoutine(Irp,
                               CompletionRoutine,
                               CompletionContext,
                               (CompletionInvocationFlags & KsInvokeOnSuccess),
                               (CompletionInvocationFlags & KsInvokeOnError),
                               (CompletionInvocationFlags & KsInvokeOnCancel));
    }

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->FileObject = FileObject;

    Status = IoCallDriver(DeviceObject, Irp);
    return Status;
}


/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsProbeStreamIrp(
    IN  PIRP Irp,
    IN  ULONG ProbeFlags,
    IN  ULONG HeaderSize)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateExtraData(
    IN  PIRP Irp,
    IN  ULONG ExtraSize,
    OUT PVOID* ExtraBuffer)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsNullDriverUnload(
    IN  PDRIVER_OBJECT DriverObject)
{
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchInvalidDeviceRequest(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_INVALID_DEVICE_REQUEST;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDefaultDeviceIoCompletion(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY && 
        IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_METHOD &&
        IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_RESET_STATE)
        {
            /* fake success */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* request unsupported */
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else
    {
        /* property / method / event not found */
        Status = STATUS_PROPSET_NOT_FOUND;
    }

    /* complete request */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);


    return Status;
}

/*
    @implemented
*/
KSDDKAPI
BOOLEAN
NTAPI
KsDispatchFastIoDeviceControlFailure(
    IN  PFILE_OBJECT FileObject,
    IN  BOOLEAN Wait,
    IN  PVOID InputBuffer  OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer  OPTIONAL,
    IN  ULONG OutputBufferLength,
    IN  ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}

/*
    @implemented
*/
KSDDKAPI
BOOLEAN
NTAPI
KsDispatchFastReadFailure(
    IN  PFILE_OBJECT FileObject,
    IN  PLARGE_INTEGER FileOffset,
    IN  ULONG Length,
    IN  BOOLEAN Wait,
    IN  ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}


/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsCancelIo(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsReleaseIrpOnCancelableQueue(
    IN  PIRP Irp,
    IN  PDRIVER_CANCEL DriverCancel OPTIONAL)
{
    UNIMPLEMENTED;
}

/*
    @implemented
*/
KSDDKAPI
PIRP
NTAPI
KsRemoveIrpFromCancelableQueue(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  KSIRP_REMOVAL_OPERATION RemovalOperation)
{
    PQUEUE_ENTRY Entry = NULL;
    PIRP Irp;
    KIRQL OldIrql;

    if (!QueueHead || !SpinLock)
        return NULL;

    if (ListLocation != KsListEntryTail && ListLocation != KsListEntryHead)
        return NULL;

    if (RemovalOperation != KsAcquireOnly && RemovalOperation != KsAcquireAndRemove)
        return NULL;

    KeAcquireSpinLock(SpinLock, &OldIrql);

    if (!IsListEmpty(QueueHead))
    {
        if (RemovalOperation == KsAcquireOnly)
        {
            if (ListLocation == KsListEntryHead)
                Entry = (PQUEUE_ENTRY)QueueHead->Flink;
            else
                Entry = (PQUEUE_ENTRY)QueueHead->Blink;
        }
        else if (RemovalOperation == KsAcquireAndRemove)
        {
            if (ListLocation == KsListEntryTail)
                Entry = (PQUEUE_ENTRY)RemoveTailList(QueueHead);
            else
                Entry = (PQUEUE_ENTRY)RemoveHeadList(QueueHead);
        }
    }
    KeReleaseSpinLock(SpinLock, OldIrql);

    if (!Entry)
        return NULL;

    Irp = Entry->Irp;

    if (RemovalOperation == KsAcquireAndRemove)
        ExFreePool(Entry);

    return Irp;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsMoveIrpsOnCancelableQueue(
    IN  OUT PLIST_ENTRY SourceList,
    IN  PKSPIN_LOCK SourceLock,
    IN  OUT PLIST_ENTRY DestinationList,
    IN  PKSPIN_LOCK DestinationLock OPTIONAL,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  PFNKSIRPLISTCALLBACK ListCallback,
    IN  PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsRemoveSpecificIrpFromCancelableQueue(
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
}


/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsAddIrpToCancelableQueue(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock,
    IN  PIRP Irp,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  PDRIVER_CANCEL DriverCancel OPTIONAL)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsCancelRoutine(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsGetChildCreateParameter(
    IN  PIRP Irp,
    OUT PVOID* CreateParameter)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
FindMatchingCreateItem(
    PLIST_ENTRY ListHead,
    ULONG BufferSize,
    LPWSTR Buffer,
    OUT PCREATE_ITEM_ENTRY *OutCreateItem)
{
    PLIST_ENTRY Entry;
    PCREATE_ITEM_ENTRY CreateItemEntry;

    /* remove '\' slash */
    Buffer++;
    BufferSize -= sizeof(WCHAR);

    /* point to first entry */
    Entry = ListHead->Flink;

    /* loop all device items */
    while(Entry != ListHead)
    {
        /* get create item entry */
        CreateItemEntry = (PCREATE_ITEM_ENTRY)CONTAINING_RECORD(Entry, CREATE_ITEM_ENTRY, Entry);

        ASSERT(CreateItemEntry->CreateItem);

        if(CreateItemEntry->CreateItem->Flags & KSCREATE_ITEM_WILDCARD)
        {
            /* create item is default */
            *OutCreateItem = CreateItemEntry;
            return STATUS_SUCCESS;
        }

        if (!CreateItemEntry->CreateItem->Create)
        {
            /* skip free create item */
            Entry = Entry->Flink;
            continue;
        }

        ASSERT(CreateItemEntry->CreateItem->ObjectClass.Buffer);

        DPRINT1("CreateItem %S Length %u Request %S %u\n", CreateItemEntry->CreateItem->ObjectClass.Buffer,
                                                           CreateItemEntry->CreateItem->ObjectClass.Length,
                                                           Buffer,
                                                           BufferSize);

        if (CreateItemEntry->CreateItem->ObjectClass.Length > BufferSize)
        {
            /* create item doesnt match in length */
            Entry = Entry->Flink;
            continue;
        }

         /* now check if the object class is the same */
        if (RtlCompareMemory(CreateItemEntry->CreateItem->ObjectClass.Buffer, Buffer, CreateItemEntry->CreateItem->ObjectClass.Length) == CreateItemEntry->CreateItem->ObjectClass.Length)
        {
            /* found matching create item */
            *OutCreateItem = CreateItemEntry;
            return STATUS_SUCCESS;
        }
        /* iterate to next */
        Entry = Entry->Flink;
    }

    return STATUS_NOT_FOUND;
}

NTAPI
NTSTATUS
KspCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PCREATE_ITEM_ENTRY CreateItemEntry;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PKSIOBJECT_HEADER ObjectHeader;
    NTSTATUS Status;

    DPRINT("KS / CREATE\n");
    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    if (IoStack->FileObject->FileName.Buffer == NULL && DeviceHeader->ItemListCount == 1)
    {
        /* hack for bug 4566 */
        ASSERT(!IsListEmpty(&DeviceHeader->ItemList));
        /* get create item entry */
        CreateItemEntry = (PCREATE_ITEM_ENTRY)CONTAINING_RECORD(DeviceHeader->ItemList.Flink, CREATE_ITEM_ENTRY, Entry);

        ASSERT(CreateItemEntry->CreateItem);

        if (!CreateItemEntry->CreateItem->Create)
        {
            /* no valid create item */
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            /* return status */
            return STATUS_UNSUCCESSFUL;
        }

        /* set object create item */
        KSCREATE_ITEM_IRP_STORAGE(Irp) = CreateItemEntry->CreateItem;

        /* call create function */
        Status = CreateItemEntry->CreateItem->Create(DeviceObject, Irp);

        if (NT_SUCCESS(Status))
        {
            /* increment create item reference count */
            InterlockedIncrement(&CreateItemEntry->ReferenceCount);
        }
        /* return result */
        return Status;
    }


    /* hack for bug 4566 */
    if (IoStack->FileObject->FileName.Buffer == NULL)
    {
        DPRINT("Using reference string hack\n");
        Irp->IoStatus.Information = 0;
        /* set return status */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }


    if (IoStack->FileObject->RelatedFileObject != NULL)
    {
        /* request is to instantiate a pin / node / clock / allocator */
        ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->RelatedFileObject->FsContext;

        /* sanity check */
        ASSERT(ObjectHeader);

        /* find a matching a create item */
        Status = FindMatchingCreateItem(&ObjectHeader->ItemList, IoStack->FileObject->FileName.Length, IoStack->FileObject->FileName.Buffer, &CreateItemEntry);
    }
    else
    {
        /* request to create a filter */
        Status = FindMatchingCreateItem(&DeviceHeader->ItemList, IoStack->FileObject->FileName.Length, IoStack->FileObject->FileName.Buffer, &CreateItemEntry);
    }

    if (NT_SUCCESS(Status))
    {
        /* set object create item */
        KSCREATE_ITEM_IRP_STORAGE(Irp) = CreateItemEntry->CreateItem;

        /* call create function */
        Status = CreateItemEntry->CreateItem->Create(DeviceObject, Irp);

        if (NT_SUCCESS(Status))
        {
            /* increment create item reference count */
            InterlockedIncrement(&CreateItemEntry->ReferenceCount);
        }
        return Status;
    }


    Irp->IoStatus.Information = 0;
    /* set return status */
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
RosDeviceInterfaceReferenceStringHack(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIDEVICE_HEADER DeviceHeader;
    PDEVICE_EXTENSION DeviceExtension;
    PCREATE_ITEM_ENTRY CreateItemEntry;
    PLIST_ENTRY Entry;
    LPWSTR Buffer;
    ULONG Length;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* retrieve all available reference strings registered */
    Length = 0;
    Entry = DeviceHeader->ItemList.Flink;
    while(Entry != &DeviceHeader->ItemList)
    {
        CreateItemEntry = (PCREATE_ITEM_ENTRY)CONTAINING_RECORD(Entry, CREATE_ITEM_ENTRY, Entry);

        ASSERT(CreateItemEntry->CreateItem);
        if (CreateItemEntry->CreateItem->Create && CreateItemEntry->CreateItem->ObjectClass.Buffer)
            Length += wcslen(CreateItemEntry->CreateItem->ObjectClass.Buffer) + 1;

        Entry = Entry->Flink;
    }

    /* add extra zero */
    Length += 1;

    /* allocate the buffer */
    Buffer = ExAllocatePool(NonPagedPool, Length * sizeof(WCHAR));
    if (!Buffer)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    *((LPWSTR*)Irp->UserBuffer) = Buffer;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(LPWSTR);

    Entry = DeviceHeader->ItemList.Flink;
    while(Entry != &DeviceHeader->ItemList)
    {
        CreateItemEntry = (PCREATE_ITEM_ENTRY)CONTAINING_RECORD(Entry, CREATE_ITEM_ENTRY, Entry);

        ASSERT(CreateItemEntry->CreateItem);
        if (CreateItemEntry->CreateItem->Create && CreateItemEntry->CreateItem->ObjectClass.Buffer)
        {
            wcscpy(Buffer, CreateItemEntry->CreateItem->ObjectClass.Buffer);
            Buffer += wcslen(Buffer) + 1;
        }
        Entry = Entry->Flink;
    }



    *Buffer = L'\0';
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

}

NTAPI
NTSTATUS
KspDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    PKSIDEVICE_HEADER DeviceHeader;
    PDEVICE_EXTENSION DeviceExtension;



    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    if (IoStack->MajorFunction == IRP_MJ_DEVICE_CONTROL && IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_OBJECT_CLASS)
    {
        /* hack for bug 4566 */
        return RosDeviceInterfaceReferenceStringHack(DeviceObject, Irp);
    }

    ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

    ASSERT(ObjectHeader);
    //KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;

    return ObjectHeader->DispatchTable.DeviceIoControl(DeviceObject, Irp);
}

NTAPI
NTSTATUS
KspDispatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIOBJECT_HEADER ObjectHeader;
    PKSIDEVICE_HEADER DeviceHeader;
    PDRIVER_DISPATCH Dispatch;
    NTSTATUS Status;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

    if (!ObjectHeader)
    {
        /* hack for bug 4566 */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        /* complete and forget */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    /* sanity check */
    ASSERT(ObjectHeader);
    /* store create item */
    //KSCREATE_ITEM_IRP_STORAGE(Irp) = (PKSOBJECT_CREATE_ITEM)0x12345678; //ObjectHeader->CreateItem;

    /* retrieve matching dispatch function */
    switch(IoStack->MajorFunction)
    {
        case IRP_MJ_CLOSE:
            Dispatch = ObjectHeader->DispatchTable.Close;
            break;
        case IRP_MJ_DEVICE_CONTROL:
            Dispatch = ObjectHeader->DispatchTable.DeviceIoControl;
            break;
        case IRP_MJ_READ:
            Dispatch = ObjectHeader->DispatchTable.Read;
            break;
        case IRP_MJ_WRITE:
            Dispatch = ObjectHeader->DispatchTable.Write;
            break;
        case IRP_MJ_FLUSH_BUFFERS :
            Dispatch = ObjectHeader->DispatchTable.Flush;
            break;
        case IRP_MJ_QUERY_SECURITY:
            Dispatch = ObjectHeader->DispatchTable.QuerySecurity;
            break;
        case IRP_MJ_SET_SECURITY:
            Dispatch = ObjectHeader->DispatchTable.SetSecurity;
            break;
        case IRP_MJ_PNP:
            Dispatch = KsDefaultDispatchPnp;
        default:
            Dispatch = NULL;
    }

    /* is the request supported */
    if (Dispatch)
    {
        /* now call the dispatch function */
        Status = Dispatch(DeviceObject, Irp);
    }
    else
    {
        /* not supported request */
        Status = KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
    }

    /* done */
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsSetMajorFunctionHandler(
    IN  PDRIVER_OBJECT DriverObject,
    IN  ULONG MajorFunction)
{
    switch ( MajorFunction )
    {
        case IRP_MJ_CREATE:
            DriverObject->MajorFunction[MajorFunction] = KspCreate;
            break;
            break;
        case IRP_MJ_DEVICE_CONTROL:
            DriverObject->MajorFunction[MajorFunction] = KspDeviceControl;
            break;
        case IRP_MJ_CLOSE:
        case IRP_MJ_READ:
        case IRP_MJ_WRITE:
        case IRP_MJ_FLUSH_BUFFERS :
        case IRP_MJ_QUERY_SECURITY:
        case IRP_MJ_SET_SECURITY:
            DriverObject->MajorFunction[MajorFunction] = KspDispatchIrp;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
    };

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIDEVICE_HEADER DeviceHeader;
    PDEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;


    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->MajorFunction <= IRP_MJ_DEVICE_CONTROL)
    {
        if (IoStack->MajorFunction == IRP_MJ_CREATE)
        {
            /* check internal type */
            if (DeviceHeader->lpVtblIKsDevice) /* FIXME improve check */
            {
                /* AVStream client */
                return IKsDevice_Create(DeviceObject, Irp);
            }
            else
            {
                /* external client (portcls) */
                return KspCreate(DeviceObject, Irp);
            }
        }

        if (IoStack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
        {
            /* handle device requests */
            return KspDeviceControl(DeviceObject, Irp);
        }

        switch (IoStack->MajorFunction)
        {
            case IRP_MJ_CLOSE:
            case IRP_MJ_READ:
            case IRP_MJ_WRITE:
            case IRP_MJ_FLUSH_BUFFERS:
            case IRP_MJ_QUERY_SECURITY:
            case IRP_MJ_SET_SECURITY:
            case IRP_MJ_PNP:
                return KspDispatchIrp(DeviceObject, Irp);
            default:
                return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
        }
    }

    /* dispatch power */
    if (IoStack->MajorFunction == IRP_MJ_POWER)
    {
        /* check internal type */
        if (DeviceHeader->lpVtblIKsDevice) /* FIXME improve check */
        {
            /* AVStream client */
            return IKsDevice_Power(DeviceObject, Irp);
        }
        else
        {
            /* external client (portcls) */
            return KsDefaultDispatchPower(DeviceObject, Irp);
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_PNP) /* dispatch pnp */
    {
        /* check internal type */
        if (DeviceHeader->lpVtblIKsDevice) /* FIXME improve check */
        {
            /* AVStream client */
            return IKsDevice_Pnp(DeviceObject, Irp);
        }
        else
        {
            /* external client (portcls) */
            return KsDefaultDispatchPnp(DeviceObject, Irp);
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_SYSTEM_CONTROL)
    {
        /* forward irp */
        return KsDefaultForwardIrp(DeviceObject, Irp);
    }
    else
    {
        /* not supported */
        return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
    }
}
