/*
    ReactOS Kernel Streaming
    IRP Helpers
*/

#include "priv.h"

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsAcquireResetValue(
    IN  PIRP Irp,
    OUT KSRESET* ResetValue)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
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
KSDDKAPI NTSTATUS NTAPI
KsAddObjectCreateItemToObjectHeader(
    IN  KSOBJECT_HEADER Header,
    IN  PDRIVER_DISPATCH Create,
    IN  PVOID Context,
    IN  PWCHAR ObjectClass,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor)
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
KsAddObjectCreateItemToDeviceHeader(
    IN  KSDEVICE_HEADER DevHeader,
    IN  PDRIVER_DISPATCH Create,
    IN  PVOID Context,
    IN  PWCHAR ObjectClass,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    PKSIDEVICE_HEADER Header;
    PKSOBJECT_CREATE_ITEM ItemList;
    PKSIOBJECT_HEADER ObjectList;

    Header = (PKSIDEVICE_HEADER)DevHeader;

    if (!DevHeader)
        return STATUS_INVALID_PARAMETER_1;

    if (!Create)
        return STATUS_INVALID_PARAMETER_2;

    if (!ObjectClass)
        return STATUS_INVALID_PARAMETER_4;

    if (Header->FreeIndex >= Header->MaxItems && Header->ItemsListProvided)
        return STATUS_ALLOTTED_SPACE_EXCEEDED;

    if (Header->FreeIndex >= Header->MaxItems)
    {
        ItemList = ExAllocatePoolWithTag(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM) * (Header->MaxItems + 1), TAG('H','D','S','K'));
        if (!ItemList)
            return STATUS_INSUFFICIENT_RESOURCES;

        ObjectList = ExAllocatePoolWithTag(PagedPool, sizeof(KSIOBJECT_HEADER) * (Header->MaxItems + 1), TAG('H','D','S','K'));
        if (!ObjectList)
        {
            ExFreePoolWithTag(ItemList, TAG('H','D','S','K'));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlMoveMemory(ItemList, Header->ItemsList, Header->MaxItems * sizeof(KSOBJECT_CREATE_ITEM));
        ExFreePoolWithTag(Header->ItemsList, TAG('H','D','S','K'));

        RtlMoveMemory(ObjectList, Header->ObjectList, Header->MaxItems * sizeof(KSIOBJECT_HEADER));
        ExFreePoolWithTag(Header->ObjectList, TAG('H','D','S','K'));

        Header->MaxItems++;
        Header->ItemsList = ItemList;
    }

    if (Header->FreeIndex < Header->MaxItems)
    {
        Header->ItemsList[Header->FreeIndex].Context = Context;
        Header->ItemsList[Header->FreeIndex].Create = Create;
        Header->ItemsList[Header->FreeIndex].Flags = 0;
        RtlInitUnicodeString(&Header->ItemsList[Header->FreeIndex].ObjectClass, ObjectClass);
        Header->ItemsList[Header->FreeIndex].SecurityDescriptor = SecurityDescriptor;

        Header->FreeIndex++;
        return STATUS_SUCCESS;
    }

    return STATUS_ALLOTTED_SPACE_EXCEEDED;
}

/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsAllocateDeviceHeader(
    OUT KSDEVICE_HEADER* OutHeader,
    IN  ULONG ItemsCount,
    IN  PKSOBJECT_CREATE_ITEM ItemsList OPTIONAL)
{
    PKSIDEVICE_HEADER Header;

    if (!OutHeader)
        return STATUS_INVALID_PARAMETER;

    Header = ExAllocatePoolWithTag(PagedPool, sizeof(KSIDEVICE_HEADER), TAG('H','D','S','K'));

    if (!Header)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Header, sizeof(KSIDEVICE_HEADER));

    if (ItemsCount)
    {
        Header->ObjectList = ExAllocatePoolWithTag(PagedPool, sizeof(KSIOBJECT_HEADER) * ItemsCount, TAG('H','D','S','K'));
        if (!Header->ObjectList)
        {
            ExFreePoolWithTag(Header, TAG('H','D','S','K'));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(Header->ObjectList, sizeof(KSIOBJECT_HEADER) * ItemsCount);
    }

    Header->MaxItems = ItemsCount;
    Header->FreeIndex = 0;
    Header->ItemsList = ItemsList;
    Header->ItemsListProvided = (ItemsList != NULL) ? TRUE : FALSE;

    *OutHeader = Header;

    return STATUS_SUCCESS;
}

/*
    @unimplemented

    http://www.osronline.com/DDKx/stream/ksfunc_3sc3.htm
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateObjectCreateItem(
    IN  KSDEVICE_HEADER DevHeader,
    IN  PKSOBJECT_CREATE_ITEM CreateItem,
    IN  BOOLEAN AllocateEntry,
    IN  PFNKSITEMFREECALLBACK ItemFreeCallback OPTIONAL)
{
    PKSIDEVICE_HEADER Header;

    Header = (PKSIDEVICE_HEADER)DevHeader;

    if (!DevHeader)
        return STATUS_INVALID_PARAMETER_1;

    if (!CreateItem)
        return STATUS_INVALID_PARAMETER_2;

    //FIXME
    //handle ItemFreeCallback
    //

    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}


/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsFreeDeviceHeader(
    IN  KSDEVICE_HEADER Header)
{
    if (!Header)
        return;

    /* TODO: Free content first */

    ExFreePoolWithTag(Header, TAG('H','D','S','K'));
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsAllocateExtraData(
    IN  PIRP Irp,
    IN  ULONG ExtraSize,
    OUT PVOID* ExtraBuffer)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented

    Initialize the required file context header.
    Allocates KSOBJECT_HEADER structure.
    Irp is an IRP_MJ_CREATE structure.
    Driver must allocate KSDISPATCH_TABLE and initialize it first.

    http://www.osronline.com/DDKx/stream/ksfunc_0u2b.htm
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateObjectHeader(
    OUT KSOBJECT_HEADER *Header,
    IN  ULONG ItemsCount,
    IN  PKSOBJECT_CREATE_ITEM ItemsList OPTIONAL,
    IN  PIRP Irp,
    IN  KSDISPATCH_TABLE* Table)
{
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG Index;

    if (!Header)
        return STATUS_INVALID_PARAMETER_1;

    if (!Irp)
        return STATUS_INVALID_PARAMETER_4;

    DeviceHeader = (PKSIDEVICE_HEADER)Irp->Tail.Overlay.DriverContext[3];
    Index = (ULONG)Irp->Tail.Overlay.DriverContext[2];

    RtlCopyMemory(&DeviceHeader->ObjectList[Index].DispatchTable, Table, sizeof(KSDISPATCH_TABLE));
    DeviceHeader->ObjectList[Index].CreateItem = ItemsList;
    DeviceHeader->ObjectList[Index].Initialized = TRUE;

    *Header = &DeviceHeader->ObjectList[Index];
    return STATUS_SUCCESS;

}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsFreeObjectHeader(
    IN  PVOID Header)
{
    ExFreePoolWithTag(Header, TAG('H','O','S','K'));

    /* TODO */

    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsCancelIo(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsCancelRoutine(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsDefaultDeviceIoCompletion(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI BOOLEAN NTAPI
KsDispatchFastIoDeviceControlFailure(
    IN  PFILE_OBJECT FileObject,
    IN  BOOLEAN Wait,
    IN  PVOID InputBuffer  OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer  OPTIONAL,
    IN  ULONG OutputBufferLength,
    IN  ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject)   /* always return false */
{
    return FALSE;
}

/*
    @unimplemented
*/
KSDDKAPI BOOLEAN NTAPI
KsDispatchFastReadFailure(
    IN  PFILE_OBJECT FileObject,
    IN  PLARGE_INTEGER FileOffset,
    IN  ULONG Length,
    IN  BOOLEAN Wait,
    IN  ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject)   /* always return false */
{
    return FALSE;
}

/*
    Used in dispatch table entries that aren't handled and need to return
    STATUS_INVALID_DEVICE_REQUEST.
*/
KSDDKAPI NTSTATUS NTAPI
KsDispatchInvalidDeviceRequest(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_INVALID_DEVICE_REQUEST;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsDispatchSpecificMethod(
    IN  PIRP Irp,
    IN  PFNKSHANDLER Handler)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
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
KSDDKAPI NTSTATUS NTAPI
KsForwardAndCatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PFILE_OBJECT FileObject,
    IN  KSSTACK_USE StackUse)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsForwardIrp(
    IN  PIRP Irp,
    IN  PFILE_OBJECT FileObject,
    IN  BOOLEAN ReuseStackLocation)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsGetChildCreateParameter(
    IN  PIRP Irp,
    OUT PVOID* CreateParameter)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
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
KSDDKAPI NTSTATUS NTAPI
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
KSDDKAPI NTSTATUS NTAPI
KsQueryInformationFile(
    IN  PFILE_OBJECT FileObject,
    OUT PVOID FileInformation,
    IN  ULONG Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI ACCESS_MASK NTAPI
KsQueryObjectAccessMask(
    IN KSOBJECT_HEADER Header)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI PKSOBJECT_CREATE_ITEM NTAPI
KsQueryObjectCreateItem(
    IN KSOBJECT_HEADER Header)
{
    UNIMPLEMENTED;
/*    return STATUS_UNSUCCESSFUL; */
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
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
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsReleaseIrpOnCancelableQueue(
    IN  PIRP Irp,
    IN  PDRIVER_CANCEL DriverCancel OPTIONAL)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI PIRP NTAPI
KsRemoveIrpFromCancelableQueue(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  KSIRP_REMOVAL_OPERATION RemovalOperation)
{
    UNIMPLEMENTED;
    return NULL;
    /*return STATUS_UNSUCCESSFUL; */
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsRemoveSpecificIrpFromCancelableQueue(
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsSetInformationFile(
    IN  PFILE_OBJECT FileObject,
    IN  PVOID FileInformation,
    IN  ULONG Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}



NTAPI
NTSTATUS
KsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG Index;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DeviceHeader = DeviceExtension->DeviceHeader;

    DPRINT1("KS / Create\n");

    /* first call all create handlers */
    for(Index = 0; Index < DeviceHeader->FreeIndex; Index++)
    {
        KSCREATE_ITEM_IRP_STORAGE(Irp) = &DeviceHeader->ItemsList[Index];
        
        Irp->Tail.Overlay.DriverContext[3] = (PVOID)DeviceHeader;
        Irp->Tail.Overlay.DriverContext[2] = (PVOID)Index;

        DeviceHeader->ObjectList[Index].Initialized = FALSE;
        Status = DeviceHeader->ItemsList[Index].Create(DeviceObject, Irp);
        if (!NT_SUCCESS(Status))
        {
            DeviceHeader->ObjectList[Index].Initialized = FALSE;
        }
    }


    return STATUS_SUCCESS;
}

static NTAPI
NTSTATUS
KsClose(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG Index;
    NTSTATUS Status;

    DPRINT1("KS / CLOSE\n");

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DeviceHeader = DeviceExtension->DeviceHeader;

    for(Index = 0; Index < DeviceHeader->FreeIndex; Index++)
    {
        if (DeviceHeader->ObjectList[Index].Initialized)
        {
            Status = DeviceHeader->ObjectList->DispatchTable.Close(DeviceObject, Irp);
            DeviceHeader->ObjectList[Index].Initialized = FALSE;
        }
    }

    return STATUS_SUCCESS;
}

static NTAPI
NTSTATUS
KsDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG Index;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DeviceHeader = DeviceExtension->DeviceHeader;

    DPRINT1("KS / DeviceControl NumDevices %x\n", DeviceHeader->FreeIndex);

    for(Index = 0; Index < DeviceHeader->FreeIndex; Index++)
    {
        if (DeviceHeader->ObjectList[Index].Initialized)
        {
            DPRINT1("Calling DeviceIoControl\n");
            Status = DeviceHeader->ObjectList->DispatchTable.DeviceIoControl(DeviceObject, Irp);
        }
    }

    return STATUS_SUCCESS;
}

static NTAPI
NTSTATUS
KsRead(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG Index;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DeviceHeader = DeviceExtension->DeviceHeader;

    DPRINT1("KS / Read\n");

    for(Index = 0; Index < DeviceHeader->FreeIndex; Index++)
    {
        if (DeviceHeader->ObjectList[Index].Initialized)
        {
            Status = DeviceHeader->ObjectList->DispatchTable.Read(DeviceObject, Irp);
        }
    }

    return STATUS_SUCCESS;
}

static NTAPI
NTSTATUS
KsWrite(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG Index;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DeviceHeader = DeviceExtension->DeviceHeader;

    DPRINT1("KS / Write\n");

    for(Index = 0; Index < DeviceHeader->FreeIndex; Index++)
    {
        if (DeviceHeader->ObjectList[Index].Initialized)
        {
            Status = DeviceHeader->ObjectList->DispatchTable.Write(DeviceObject, Irp);
        }
    }
    return STATUS_SUCCESS;
}

static NTAPI
NTSTATUS
KsFlushBuffers(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT1("KS / FlushBuffers\n");
    return STATUS_UNSUCCESSFUL;
}

static NTAPI
NTSTATUS
KsQuerySecurity(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT1("KS / QuerySecurity\n");
    return STATUS_UNSUCCESSFUL;
}

static NTAPI
NTSTATUS
KsSetSecurity(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT1("KS / SetSecurity\n");
    return STATUS_UNSUCCESSFUL;
}

/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsSetMajorFunctionHandler(
    IN  PDRIVER_OBJECT DriverObject,
    IN  ULONG MajorFunction)
{
    /*
        Sets a DriverObject's major function handler to point to an internal
        function we implement.

        TODO: Deal with KSDISPATCH_FASTIO
    */

    switch ( MajorFunction )
    {
        case IRP_MJ_CREATE:
            DriverObject->MajorFunction[MajorFunction] = KsCreate;
            break;
        case IRP_MJ_CLOSE:
            DriverObject->MajorFunction[MajorFunction] = KsClose;
            break;
        case IRP_MJ_DEVICE_CONTROL:
            DriverObject->MajorFunction[MajorFunction] = KsDeviceControl;
            break;
        case IRP_MJ_READ:
            DriverObject->MajorFunction[MajorFunction] = KsRead;
            break;
        case IRP_MJ_WRITE:
            DriverObject->MajorFunction[MajorFunction] = KsWrite;
            break;
        case IRP_MJ_FLUSH_BUFFERS :
            DriverObject->MajorFunction[MajorFunction] = KsFlushBuffers;
            break;
        case IRP_MJ_QUERY_SECURITY:
            DriverObject->MajorFunction[MajorFunction] = KsQuerySecurity;
            break;
        case IRP_MJ_SET_SECURITY:
            DriverObject->MajorFunction[MajorFunction] = KsSetSecurity;
            break;

        default:
            return STATUS_INVALID_PARAMETER;    /* is this right? */
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

    /* Calls a dispatch routine corresponding to the function code of the IRP */
    /*
        First we need to get the dispatch table. An opaque header is pointed to by
        FsContext. The first element points to this table. This table is the key
        to dispatching the IRP correctly.
    */

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT1("KsDispatchIrp %x\n", IoStack->MajorFunction);

    switch (IoStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            return KsCreate(DeviceObject, Irp);
        case IRP_MJ_CLOSE:
            return KsClose(DeviceObject, Irp);
            break;
        case IRP_MJ_DEVICE_CONTROL:
            return KsDeviceControl(DeviceObject, Irp);
            break;
        case IRP_MJ_READ:
            return KsRead(DeviceObject, Irp);
            break;
        case IRP_MJ_WRITE:
            return KsWrite(DeviceObject, Irp);
            break;
        case IRP_MJ_FLUSH_BUFFERS:
            return KsFlushBuffers(DeviceObject, Irp);
            break;
        case IRP_MJ_QUERY_SECURITY:
            return KsQuerySecurity(DeviceObject, Irp);
            break;
        case IRP_MJ_SET_SECURITY:
            return KsSetSecurity(DeviceObject, Irp);
            break;
        default:
            return STATUS_INVALID_PARAMETER;    /* is this right? */
    };
}


/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
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
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
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
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsDefaultForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

