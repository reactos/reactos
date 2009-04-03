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
    PQUEUE_ENTRY Entry;

    if (!QueueHead || !SpinLock || !Irp)
        return;

    Entry = ExAllocatePool(NonPagedPool, sizeof(QUEUE_ENTRY));
    if (!Entry)
        return;

    ///FIXME
    // setup cancel routine
    //

    Entry->Irp = Irp;

    if (ListLocation == KsListEntryTail)
        ExInterlockedInsertTailList(QueueHead, &Entry->Entry, SpinLock);
    else
        ExInterlockedInsertHeadList(QueueHead, &Entry->Entry, SpinLock);

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
    ULONG FreeIndex, Index;

    Header = (PKSIDEVICE_HEADER)DevHeader;

    DPRINT1("KsAddObjectCreateItemToDeviceHeader entered\n");

     /* check if a device header has been provided */
    if (!DevHeader)
        return STATUS_INVALID_PARAMETER_1;

    /* check if a create item has been provided */
    if (!Create)
        return STATUS_INVALID_PARAMETER_2;

    /* check if a object class has been provided */
    if (!ObjectClass)
        return STATUS_INVALID_PARAMETER_4;

    FreeIndex = (ULONG)-1;
    /* now scan the list and check for a free item */
    for(Index = 0; Index < Header->MaxItems; Index++)
    {
        if (!Header->ItemList[Index].bCreated)
        {
            if (FreeIndex == (ULONG)-1)
                FreeIndex = Index;

            continue;
        }
        else if (!wcsicmp(ObjectClass, Header->ItemList[Index].CreateItem.ObjectClass.Buffer))
        {
            /* the same object class already exists */
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }
    /* found a free index */
    if (FreeIndex == (ULONG)-1)
    {
        /* allocate a new device entry */
        PDEVICE_ITEM Item = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_ITEM) * (Header->MaxItems + 1), TAG_DEVICE_HEADER);
        if (!Item)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlMoveMemory(Item, Header->ItemList, Header->MaxItems * sizeof(DEVICE_ITEM));
        ExFreePoolWithTag(Header->ItemList, TAG_DEVICE_HEADER);
        
        Header->ItemList = Item;
        FreeIndex = Header->MaxItems;
        Header->MaxItems++;
    }

    /* store the new item */
    Header->ItemList[FreeIndex].bCreated = TRUE;
    Header->ItemList[FreeIndex].CreateItem.Create = Create;
    Header->ItemList[FreeIndex].CreateItem.Context = Context;
    RtlInitUnicodeString(&Header->ItemList[FreeIndex].CreateItem.ObjectClass, ObjectClass);
    Header->ItemList[FreeIndex].CreateItem.SecurityDescriptor = SecurityDescriptor;
    Header->ItemList[FreeIndex].CreateItem.Flags = 0;
    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateDeviceHeader(
    OUT KSDEVICE_HEADER* OutHeader,
    IN  ULONG ItemsCount,
    IN  PKSOBJECT_CREATE_ITEM ItemsList OPTIONAL)
{
    ULONG Index = 0;
    PKSIDEVICE_HEADER Header;

    if (!OutHeader)
        return STATUS_INVALID_PARAMETER;

    /* allocate a device header */
    Header = ExAllocatePoolWithTag(PagedPool, sizeof(KSIDEVICE_HEADER), TAG_DEVICE_HEADER);

    /* check for success */
    if (!Header)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* clear all memory */
    RtlZeroMemory(Header, sizeof(KSIDEVICE_HEADER));

    /* initialize spin lock */
    KeInitializeSpinLock(&Header->ItemListLock);

    /* are there any create items provided */
    if (ItemsCount && ItemsList)
    {
        /* allocate space for device item list */
        Header->ItemList = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_ITEM) * ItemsCount, TAG_DEVICE_HEADER);
        if (!Header->ItemList)
        {
            ExFreePoolWithTag(Header, TAG_DEVICE_HEADER);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(Header->ItemList, sizeof(DEVICE_ITEM) * ItemsCount);

        for(Index = 0; Index < ItemsCount; Index++)
        {
            /* copy provided create items */
            RtlMoveMemory(&Header->ItemList[Index].CreateItem, &ItemsList[Index], sizeof(KSOBJECT_CREATE_ITEM));
            if (ItemsList[Index].Create!= NULL)
            {
                Header->ItemList[Index].bCreated = TRUE;
            }
        }
        Header->MaxItems = ItemsCount;
    }

    /* store result */
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
    if (AllocateEntry && ItemFreeCallback)
        DPRINT1("Ignoring ItemFreeCallback\n");

    return KsAddObjectCreateItemToDeviceHeader(DevHeader, CreateItem->Create, CreateItem->Context, CreateItem->ObjectClass.Buffer, CreateItem->SecurityDescriptor);
}


/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFreeDeviceHeader(
    IN  KSDEVICE_HEADER DevHeader)
{
    PKSIDEVICE_HEADER Header;

    Header = (PKSIDEVICE_HEADER)DevHeader;

    if (!DevHeader)
        return;

    ExFreePoolWithTag(Header->ItemList, TAG_DEVICE_HEADER);
    ExFreePoolWithTag(Header, TAG_DEVICE_HEADER);
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
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PKSIOBJECT_HEADER ObjectHeader;
    WCHAR ObjectClass[50];

    if (!Header)
        return STATUS_INVALID_PARAMETER_1;

    if (!Irp)
        return STATUS_INVALID_PARAMETER_4;

    if (!Table)
        return STATUS_INVALID_PARAMETER_5;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    ObjectClass[0] = L'\0';
    /* check for an file object */
    if (IoStack->FileObject != NULL)
    {
        /* validate the file name */
        if (IoStack->FileObject->FileName.Length >= 38)
        {
            RtlMoveMemory(ObjectClass, IoStack->FileObject->FileName.Buffer, 38 * sizeof(WCHAR));
            ObjectClass[38] = L'\0';
            DPRINT("ObjectClass %S\n", ObjectClass);
        }
    }
    /* allocate the object header */
    ObjectHeader = ExAllocatePoolWithTag(NonPagedPool, sizeof(KSIOBJECT_HEADER), TAG_DEVICE_HEADER);
    if (!ObjectHeader)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize object header */
    RtlZeroMemory(ObjectHeader, sizeof(KSIOBJECT_HEADER));

    /* do we have a name */
    if (ObjectClass[0])
    {
        ObjectHeader->ObjectClass = ExAllocatePoolWithTag(NonPagedPool, 40 * sizeof(WCHAR), TAG_DEVICE_HEADER);
        if (ObjectHeader->ObjectClass)
        {
            wcscpy(ObjectHeader->ObjectClass, ObjectClass);
        }
    }

    /* copy dispatch table */
    RtlCopyMemory(&ObjectHeader->DispatchTable, Table, sizeof(KSDISPATCH_TABLE));
    /* store create items */
    if (ItemsCount && ItemsList)
    {
        ObjectHeader->ItemCount = ItemsCount;
        ObjectHeader->CreateItem = ItemsList;
    }

    /* was the request for a pin/clock/node */
    if (IoStack->FileObject)
    {
        /* store the object in the file object */
        ASSERT(IoStack->FileObject->FsContext == NULL);
        IoStack->FileObject->FsContext = ObjectHeader;
    }
    else
    {
        /* the object header is for device */
        ASSERT(DeviceHeader->DeviceIndex < DeviceHeader->MaxItems);
        DeviceHeader->ItemList[DeviceHeader->DeviceIndex].ObjectHeader = ObjectHeader;
    }

    /* store result */
    *Header = ObjectHeader;


    DPRINT("KsAllocateObjectHeader ObjectClass %S FileObject %p, ObjectHeader %p\n", ObjectClass, IoStack->FileObject, ObjectHeader);

    return STATUS_SUCCESS;

}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsFreeObjectHeader(
    IN  PVOID Header)
{


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
    //PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldLevel;

    DPRINT("KS / CREATE\n");
    /* get current stack location */
    //IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* acquire list lock */
    KeAcquireSpinLock(&DeviceHeader->ItemListLock, &OldLevel);
    /* loop all device items */
    for(Index = 0; Index < DeviceHeader->MaxItems; Index++)
    {
        if (DeviceHeader->ItemList[Index].bCreated && DeviceHeader->ItemList[Index].ObjectHeader == NULL)
        {
            DeviceHeader->DeviceIndex = Index;
             /* set object create item */
            KSCREATE_ITEM_IRP_STORAGE(Irp) = &DeviceHeader->ItemList[Index].CreateItem;
            Status = DeviceHeader->ItemList[Index].CreateItem.Create(DeviceObject, Irp);
        }
    }

    /* release lock */
    KeReleaseSpinLock(&DeviceHeader->ItemListLock, OldLevel);
    return Status;
}

static NTAPI
NTSTATUS
KsClose(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / CLOSE\n");

    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.Close(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        return STATUS_SUCCESS;
    }
}

static NTAPI
NTSTATUS
KsDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / DeviceControl\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.DeviceIoControl(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

static NTAPI
NTSTATUS
KsRead(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / Read\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.Read(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

static NTAPI
NTSTATUS
KsWrite(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / Write\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.Write(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header %p\n", IoStack->FileObject);
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

static NTAPI
NTSTATUS
KsFlushBuffers(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / FlushBuffers\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.Flush(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

static NTAPI
NTSTATUS
KsQuerySecurity(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / QuerySecurity\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.QuerySecurity(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

static NTAPI
NTSTATUS
KsSetSecurity(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / SetSecurity\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.SetSecurity(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
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
    DPRINT("KsDispatchIrp %x\n", IoStack->MajorFunction);

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
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

