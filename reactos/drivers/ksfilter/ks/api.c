/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/api.c
 * PURPOSE:         KS API functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"


/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
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
KsAcquireDeviceSecurityLock(
    IN KSDEVICE_HEADER DevHeader,
    IN BOOLEAN Exclusive)
{
    NTSTATUS Status;
    PKSIDEVICE_HEADER Header = (PKSIDEVICE_HEADER)DevHeader;

    KeEnterCriticalRegion();

    if (Exclusive)
    {
        Status = ExAcquireResourceExclusiveLite(&Header->SecurityLock, TRUE);
    }
    else
    {
        Status = ExAcquireResourceSharedLite(&Header->SecurityLock, TRUE);
    }
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsReleaseDeviceSecurityLock(
    IN KSDEVICE_HEADER DevHeader)
{
    PKSIDEVICE_HEADER Header = (PKSIDEVICE_HEADER)DevHeader;

    ExReleaseResourceLite(&Header->SecurityLock);
    KeLeaveCriticalRegion();
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDefaultDispatchPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT PnpDeviceObject;
    NTSTATUS Status;
    ULONG MinorFunction;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* caller wants to add the target device */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get device header */
    DeviceHeader = (PKSIDEVICE_HEADER)DeviceExtension->DeviceHeader;

    /* backup PnpBaseObject */
    PnpDeviceObject = DeviceHeader->PnpDeviceObject;


    /* backup minor function code */
    MinorFunction = IoStack->MinorFunction;

    if(MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        /* remove the device */
        KsFreeDeviceHeader((KSDEVICE_HEADER)DeviceHeader);
    }

    /* skip current irp stack */
    IoSkipCurrentIrpStackLocation(Irp);

    /* call attached pnp device object */
    Status = IoCallDriver(PnpDeviceObject, Irp);

    if (MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        /* time is over */
        IoDetachDevice(PnpDeviceObject);
        /* delete device */
        IoDeleteDevice(DeviceObject);
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
KsDefaultDispatchPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PKSIOBJECT_HEADER ObjectHeader;
    PIO_STACK_LOCATION IoStack;
    PLIST_ENTRY ListEntry;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* caller wants to add the target device */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get device header */
    DeviceHeader = (PKSIDEVICE_HEADER)DeviceExtension->DeviceHeader;

    /* FIXME locks */

    /* loop our power dispatch list and call registered notification functions */
    ListEntry = DeviceHeader->PowerDispatchList.Flink;
    /* let's go */
    while(ListEntry != &DeviceHeader->PowerDispatchList)
    {
        /* get object header */
        ObjectHeader = (PKSIOBJECT_HEADER)CONTAINING_RECORD(ListEntry, KSIOBJECT_HEADER, PowerDispatchEntry);

        /* does it have still a cb */
        if (ObjectHeader->PowerDispatch)
        {
            /* call the power cb */
            Status = ObjectHeader->PowerDispatch(ObjectHeader->PowerContext, Irp);
            ASSERT(NT_SUCCESS(Status));
        }

        /* iterate to next entry */
        ListEntry = ListEntry->Flink;
    }

    /* start next power irp */
    PoStartNextPowerIrp(Irp);

    /* skip current irp stack location */
    IoSkipCurrentIrpStackLocation(Irp);

    /* let's roll */
    Status = PoCallDriver(DeviceHeader->PnpDeviceObject, Irp);

    /* done */
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDefaultForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* caller wants to add the target device */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get device header */
    DeviceHeader = (PKSIDEVICE_HEADER)DeviceExtension->DeviceHeader;

    /* forward the request to the PDO */
    Status = IoCallDriver(DeviceHeader->PnpDeviceObject, Irp);

    return Status;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsSetDevicePnpAndBaseObject(
    IN  KSDEVICE_HEADER Header,
    IN  PDEVICE_OBJECT PnpDeviceObject,
    IN  PDEVICE_OBJECT BaseDevice)
{
    PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)Header;

    DeviceHeader->PnpDeviceObject = PnpDeviceObject;
    DeviceHeader->BaseDevice = BaseDevice;
}

/*
    @implemented
*/
KSDDKAPI
PDEVICE_OBJECT
NTAPI
KsQueryDevicePnpObject(
    IN  KSDEVICE_HEADER Header)
{
    PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)Header;

    /* return PnpDeviceObject */
    return DeviceHeader->PnpDeviceObject;

}

/*
    @unimplemented
*/
KSDDKAPI
ACCESS_MASK
NTAPI
KsQueryObjectAccessMask(
    IN KSOBJECT_HEADER Header)
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
KsRecalculateStackDepth(
    IN  KSDEVICE_HEADER Header,
    IN  BOOLEAN ReuseStackLocation)
{
    UNIMPLEMENTED;
}


/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsSetTargetState(
    IN  KSOBJECT_HEADER Header,
    IN  KSTARGET_STATE TargetState)
{
    PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)Header;

    /* set target state */
    DeviceHeader->TargetState = TargetState;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsSetTargetDeviceObject(
    IN  KSOBJECT_HEADER Header,
    IN  PDEVICE_OBJECT TargetDevice OPTIONAL)
{
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PKSIOBJECT_HEADER ObjectHeader = (PKSIOBJECT_HEADER)Header;

    if(ObjectHeader->TargetDevice)
    {
        /* there is already a target device set */
        if (!TargetDevice)
        {
            /* caller wants to remove the target device */
            DeviceExtension = (PDEVICE_EXTENSION)ObjectHeader->TargetDevice->DeviceExtension;

            /* get device header */
            DeviceHeader = (PKSIDEVICE_HEADER)DeviceExtension->DeviceHeader;

            /* acquire lock */
            KsAcquireDeviceSecurityLock((KSDEVICE_HEADER)DeviceHeader, FALSE);

            /* remove entry */
            RemoveEntryList(&ObjectHeader->TargetDeviceListEntry);

            /* remove device pointer */
            ObjectHeader->TargetDevice = NULL;

            /* release lock */
            KsReleaseDeviceSecurityLock((KSDEVICE_HEADER)DeviceHeader);
        }
    }
    else
    {
        /* no target device yet set */
        if (TargetDevice)
        {
            /* caller wants to add the target device */
            DeviceExtension = (PDEVICE_EXTENSION)TargetDevice->DeviceExtension;

            /* get device header */
            DeviceHeader = (PKSIDEVICE_HEADER)DeviceExtension->DeviceHeader;

            /* acquire lock */
            KsAcquireDeviceSecurityLock((KSDEVICE_HEADER)DeviceHeader, FALSE);

            /* insert list entry */
            InsertTailList(&DeviceHeader->TargetDeviceList, &ObjectHeader->TargetDeviceListEntry);

            /* store target device */
            ObjectHeader->TargetDevice = TargetDevice;

            /* release lock */
            KsReleaseDeviceSecurityLock((KSDEVICE_HEADER)DeviceHeader);
        }
    }

}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsSetPowerDispatch(
    IN  KSOBJECT_HEADER Header,
    IN  PFNKSCONTEXT_DISPATCH PowerDispatch OPTIONAL,
    IN  PVOID PowerContext OPTIONAL)
{
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PKSIOBJECT_HEADER ObjectHeader = (PKSIOBJECT_HEADER)Header;

    /* caller wants to add the target device */
    DeviceExtension = (PDEVICE_EXTENSION)ObjectHeader->ParentDeviceObject->DeviceExtension;

    /* get device header */
    DeviceHeader = (PKSIDEVICE_HEADER)DeviceExtension->DeviceHeader;

    /* acquire lock */
    KsAcquireDeviceSecurityLock((KSDEVICE_HEADER)DeviceHeader, FALSE);

    if (PowerDispatch)
    {
        /* add power dispatch entry */
        InsertTailList(&DeviceHeader->PowerDispatchList, &ObjectHeader->PowerDispatchEntry);

       /* store function and context */
       ObjectHeader->PowerDispatch = PowerDispatch;
       ObjectHeader->PowerContext = PowerContext;
    }
    else
    {
        /* remove power dispatch entry */
        RemoveEntryList(&ObjectHeader->PowerDispatchEntry);

       /* store function and context */
       ObjectHeader->PowerDispatch = NULL;
       ObjectHeader->PowerContext = NULL;

    }

    /* release lock */
    KsReleaseDeviceSecurityLock((KSDEVICE_HEADER)DeviceHeader);
}


/*
    @unimplemented
*/
KSDDKAPI
PKSOBJECT_CREATE_ITEM
NTAPI
KsQueryObjectCreateItem(
    IN KSOBJECT_HEADER Header)
{
    UNIMPLEMENTED;
    return NULL;
}

NTSTATUS
KspAddCreateItemToList(
    OUT PLIST_ENTRY ListHead,
    IN ULONG ItemsCount,
    IN  PKSOBJECT_CREATE_ITEM ItemsList)
{
    ULONG Index;
    PCREATE_ITEM_ENTRY Entry;

    /* add the items */
    for(Index = 0; Index < ItemsCount; Index++)
    {
        /* allocate item */
        Entry = AllocateItem(NonPagedPool, sizeof(CREATE_ITEM_ENTRY));
        if (!Entry)
        {
            /* no memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* initialize entry */
        InitializeListHead(&Entry->ObjectItemList);
        Entry->CreateItem = &ItemsList[Index];
        Entry->ReferenceCount = 0;
        Entry->ItemFreeCallback = NULL;

        InsertTailList(ListHead, &Entry->Entry);
    }
    return STATUS_SUCCESS;
}

VOID
KspFreeCreateItems(
    PLIST_ENTRY ListHead)
{
    PCREATE_ITEM_ENTRY Entry;

    while(!IsListEmpty(ListHead))
    {
        /* remove create item from list */
        Entry = (PCREATE_ITEM_ENTRY)CONTAINING_RECORD(RemoveHeadList(ListHead), CREATE_ITEM_ENTRY, Entry);

        /* caller shouldnt have any references */
        ASSERT(Entry->ReferenceCount == 0);
        ASSERT(IsListEmpty(&Entry->ObjectItemList));

        /* does the creator wish notification */
        if (Entry->ItemFreeCallback)
        {
            /* notify creator */
            Entry->ItemFreeCallback(Entry->CreateItem);
        }

        /* free create item entry */
         FreeItem(Entry);
    }

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
    NTSTATUS Status = STATUS_SUCCESS;
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

    /* initialize device mutex */
    KeInitializeMutex(&Header->DeviceMutex, 0);

    /* initialize target device list */
    InitializeListHead(&Header->TargetDeviceList);
    /* initialize power dispatch list */
    InitializeListHead(&Header->PowerDispatchList);

    /* initialize create item list */
    InitializeListHead(&Header->ItemList);

    /* are there any create items provided */
    if (ItemsCount && ItemsList)
    {
        Status = KspAddCreateItemToList(&Header->ItemList, ItemsCount, ItemsList);

        if (NT_SUCCESS(Status))
        {
            /* store item count */
            Header->ItemListCount = ItemsCount;
        }
        else
        {
            /* release create items */
            KspFreeCreateItems(&Header->ItemList);
        }
    }

    /* store result */
    *OutHeader = Header;

    return Status;
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

    KspFreeCreateItems(&Header->ItemList);
    ExFreePoolWithTag(Header, TAG_DEVICE_HEADER);
}

/*
    @implemented
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
    PKSOBJECT_CREATE_ITEM CreateItem;
    NTSTATUS Status;

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

    /* sanity check */
    ASSERT(IoStack->FileObject);
    /* check for an file object */

    /* allocate the object header */
    ObjectHeader = ExAllocatePoolWithTag(NonPagedPool, sizeof(KSIOBJECT_HEADER), TAG_DEVICE_HEADER);
    if (!ObjectHeader)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize object header */
    RtlZeroMemory(ObjectHeader, sizeof(KSIOBJECT_HEADER));

    /* initialize create item list */
    InitializeListHead(&ObjectHeader->ItemList);

    /* get create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    /* do we have a name */
    if (IoStack->FileObject->FileName.Buffer)
    {
        /* copy object class */
        ObjectHeader->ObjectClass.MaximumLength = IoStack->FileObject->FileName.MaximumLength;
        ObjectHeader->ObjectClass.Buffer = ExAllocatePoolWithTag(NonPagedPool, ObjectHeader->ObjectClass.MaximumLength, TAG_DEVICE_HEADER);
        if (!ObjectHeader->ObjectClass.Buffer)
        {
            ExFreePoolWithTag(ObjectHeader, TAG_DEVICE_HEADER);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyUnicodeString(&ObjectHeader->ObjectClass, &IoStack->FileObject->FileName);
    }

    /* copy dispatch table */
    RtlCopyMemory(&ObjectHeader->DispatchTable, Table, sizeof(KSDISPATCH_TABLE));

    /* store create items */
    if (ItemsCount && ItemsList)
    {
        Status = KspAddCreateItemToList(&ObjectHeader->ItemList, ItemsCount, ItemsList);

        if (NT_SUCCESS(Status))
        {
            /* store item count */
            ObjectHeader->ItemListCount = ItemsCount;
        }
        else
        {
            /* destroy header*/
            KsFreeObjectHeader(ObjectHeader);
            return Status;
        }
    }
    /* store the object in the file object */
    ASSERT(IoStack->FileObject->FsContext == NULL);
    IoStack->FileObject->FsContext = ObjectHeader;

    /* store parent device */
    ObjectHeader->ParentDeviceObject = IoGetRelatedDeviceObject(IoStack->FileObject);

    /* store result */
    *Header = ObjectHeader;

    DPRINT("KsAllocateObjectHeader ObjectClass %S FileObject %p, ObjectHeader %p\n", ObjectHeader->ObjectClass.Buffer, IoStack->FileObject, ObjectHeader);

    return STATUS_SUCCESS;

}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFreeObjectHeader(
    IN  PVOID Header)
{
    PKSIOBJECT_HEADER ObjectHeader = (PKSIOBJECT_HEADER) Header;

    if (ObjectHeader->ObjectClass.Buffer)
    {
        /* release object class buffer */
        ExFreePoolWithTag(ObjectHeader->ObjectClass.Buffer, TAG_DEVICE_HEADER);
    }

    if (ObjectHeader->Unknown)
    {
        /* release associated object */
        ObjectHeader->Unknown->lpVtbl->Release(ObjectHeader->Unknown);
    }

    /* free create items */
    KspFreeCreateItems(&ObjectHeader->ItemList);

    /* free object header */
    ExFreePoolWithTag(ObjectHeader, TAG_DEVICE_HEADER);

}

NTSTATUS
KspAddObjectCreateItemToList(
    PLIST_ENTRY ListHead,
    IN  PDRIVER_DISPATCH Create,
    IN  PVOID Context,
    IN  PWCHAR ObjectClass,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   PLIST_ENTRY Entry;
   PCREATE_ITEM_ENTRY CreateEntry;

    /* point to first entry */
    Entry = ListHead->Flink;

    while(Entry != ListHead)
    {
        /* get create entry */
        CreateEntry = (PCREATE_ITEM_ENTRY)CONTAINING_RECORD(Entry, CREATE_ITEM_ENTRY, Entry);
        /* if the create item has no create routine, then it is free to use */
        if (CreateEntry->CreateItem->Create == NULL)
        {
            /* sanity check */
            ASSERT(IsListEmpty(&CreateEntry->ObjectItemList));
            ASSERT(CreateEntry->ReferenceCount == 0);
            /* use free entry */
            CreateEntry->CreateItem->Context = Context;
            CreateEntry->CreateItem->Create = Create;
            RtlInitUnicodeString(&CreateEntry->CreateItem->ObjectClass, ObjectClass);
            CreateEntry->CreateItem->SecurityDescriptor = SecurityDescriptor;

            return STATUS_SUCCESS;
        }

        if (!wcsicmp(ObjectClass, CreateEntry->CreateItem->ObjectClass.Buffer))
        {
            /* the same object class already exists */
            return STATUS_OBJECT_NAME_COLLISION;
        }

        /* iterate to next entry */
        Entry = Entry->Flink;
    }
    return STATUS_ALLOTTED_SPACE_EXCEEDED;
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
    NTSTATUS Status;

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

    /* let others do the work */
    Status = KspAddObjectCreateItemToList(&Header->ItemList, Create, Context, ObjectClass, SecurityDescriptor);

    if (NT_SUCCESS(Status))
    {
        /* increment create item count */
        InterlockedIncrement(&Header->ItemListCount);
    }

    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAddObjectCreateItemToObjectHeader(
    IN  KSOBJECT_HEADER ObjectHeader,
    IN  PDRIVER_DISPATCH Create,
    IN  PVOID Context,
    IN  PWCHAR ObjectClass,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    PKSIOBJECT_HEADER Header;
    NTSTATUS Status;

    Header = (PKSIOBJECT_HEADER)ObjectHeader;

    DPRINT1("KsAddObjectCreateItemToDeviceHeader entered\n");

     /* check if a device header has been provided */
    if (!Header)
        return STATUS_INVALID_PARAMETER_1;

    /* check if a create item has been provided */
    if (!Create)
        return STATUS_INVALID_PARAMETER_2;

    /* check if a object class has been provided */
    if (!ObjectClass)
        return STATUS_INVALID_PARAMETER_4;

    /* let's work */
    Status = KspAddObjectCreateItemToList(&Header->ItemList, Create, Context, ObjectClass, SecurityDescriptor);

    if (NT_SUCCESS(Status))
    {
        /* increment create item count */
        InterlockedIncrement(&Header->ItemListCount);
    }

    return Status;
}

/*
    @implemented
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
    PCREATE_ITEM_ENTRY CreateEntry;
    PKSIDEVICE_HEADER Header;
    PKSOBJECT_CREATE_ITEM Item;

    Header = (PKSIDEVICE_HEADER)DevHeader;

    if (!DevHeader)
        return STATUS_INVALID_PARAMETER_1;

    if (!CreateItem)
        return STATUS_INVALID_PARAMETER_2;

    /* first allocate a create entry */
    CreateEntry = AllocateItem(NonPagedPool, sizeof(PCREATE_ITEM_ENTRY));

    /* check for allocation success */
    if (!CreateEntry)
    {
        /* not enough resources */
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    if (AllocateEntry)
    {
        /* allocate create item */
        Item = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
        if (!Item)
        {
            /* no memory */
            ExFreePool(CreateEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* initialize descriptor */
        Item->Context = CreateItem->Context;
        Item->Create = CreateItem->Create;
        Item->Flags = CreateItem->Flags;
        Item->SecurityDescriptor = CreateItem->SecurityDescriptor;
        Item->ObjectClass.Length = 0;
        Item->ObjectClass.MaximumLength = CreateItem->ObjectClass.MaximumLength;

        /* copy object class */
        Item->ObjectClass.Buffer = ExAllocatePool(NonPagedPool, Item->ObjectClass.MaximumLength);
        if (!Item->ObjectClass.Buffer)
        {
            /* release resources */
            FreeItem(Item);
            FreeItem(CreateEntry);

            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyUnicodeString(&Item->ObjectClass, &CreateItem->ObjectClass);
    }
    else
    {
        if (ItemFreeCallback)
        {
            /* callback is only accepted when the create item is copied */
            ItemFreeCallback = NULL;
        }
        /* use passed create item */
        Item = CreateItem;
    }

    /* initialize create item entry */
    InitializeListHead(&CreateEntry->ObjectItemList);
    CreateEntry->ItemFreeCallback = ItemFreeCallback;
    CreateEntry->CreateItem = Item;
    CreateEntry->ReferenceCount = 0;

    /* now insert the create item entry */
    InsertTailList(&Header->ItemList, &CreateEntry->Entry);

    /* increment item count */
    InterlockedIncrement(&Header->ItemListCount);

    return STATUS_SUCCESS;
}

NTSTATUS
KspObjectFreeCreateItems(
    IN  KSDEVICE_HEADER Header,
    IN  PKSOBJECT_CREATE_ITEM CreateItem)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFreeObjectCreateItem(
    IN  KSDEVICE_HEADER Header,
    IN  PUNICODE_STRING CreateItem)
{
    KSOBJECT_CREATE_ITEM Item;

    RtlZeroMemory(&Item, sizeof(KSOBJECT_CREATE_ITEM));
    RtlInitUnicodeString(&Item.ObjectClass, CreateItem->Buffer);

    return KspObjectFreeCreateItems(Header, &Item);
}


/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFreeObjectCreateItemsByContext(
    IN  KSDEVICE_HEADER Header,
    IN  PVOID Context)
{
    KSOBJECT_CREATE_ITEM Item;

    RtlZeroMemory(&Item, sizeof(KSOBJECT_CREATE_ITEM));

    Item.Context = Context;

    return KspObjectFreeCreateItems(Header, &Item);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreateDefaultSecurity(
    IN PSECURITY_DESCRIPTOR ParentSecurity OPTIONAL,
    OUT PSECURITY_DESCRIPTOR* DefaultSecurity)
{
    PGENERIC_MAPPING Mapping;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    NTSTATUS Status;

    /* start capturing security context of calling thread */
    SeCaptureSubjectContext(&SubjectContext);
    /* get generic mapping */
    Mapping = IoGetFileObjectGenericMapping();
    /* build new descriptor */
    Status = SeAssignSecurity(ParentSecurity, NULL, DefaultSecurity, FALSE, &SubjectContext, Mapping, NonPagedPool);
    /* release security descriptor */
    SeReleaseSubjectContext(&SubjectContext);
    /* done */
    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
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
KSDDKAPI
NTSTATUS
NTAPI
KsForwardAndCatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PFILE_OBJECT FileObject,
    IN  KSSTACK_USE StackUse)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
NTAPI
KspSynchronousIoControlDeviceCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    PIO_STATUS_BLOCK IoStatusBlock = (PIO_STATUS_BLOCK)Context;

    IoStatusBlock->Information = Irp->IoStatus.Information;
    IoStatusBlock->Status = Irp->IoStatus.Status;

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsSynchronousIoControlDevice(
    IN  PFILE_OBJECT FileObject,
    IN  KPROCESSOR_MODE RequestorMode,
    IN  ULONG IoControl,
    IN  PVOID InBuffer,
    IN  ULONG InSize,
    OUT PVOID OutBuffer,
    IN  ULONG OutSize,
    OUT PULONG BytesReturned)
{
    PKSIOBJECT_HEADER ObjectHeader;
    PDEVICE_OBJECT DeviceObject;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* check for valid file object */
    if (!FileObject)
        return STATUS_INVALID_PARAMETER;

    /* get device object to send the request to */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject)
        return STATUS_UNSUCCESSFUL;


    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER)FileObject->FsContext;

    /* check if there is fast device io function */
    if (ObjectHeader && ObjectHeader->DispatchTable.FastDeviceIoControl)
    {
        IoStatusBlock.Status = STATUS_UNSUCCESSFUL;
        IoStatusBlock.Information = 0;

        /* it is send the request */
        Status = ObjectHeader->DispatchTable.FastDeviceIoControl(FileObject, TRUE, InBuffer, InSize, OutBuffer, OutSize, IoControl, &IoStatusBlock, DeviceObject);
        /* check if the request was handled */
        //DPRINT("Handled %u Status %x Length %u\n", Status, IoStatusBlock.Status, IoStatusBlock.Information);
        if (Status)
        {
            /* store bytes returned */
            *BytesReturned = IoStatusBlock.Information;
            /* return status */
            return IoStatusBlock.Status;
        }
    }

    /* initialize the event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* create the irp */
    Irp =  IoBuildDeviceIoControlRequest(IoControl, DeviceObject, InBuffer, InSize, OutBuffer, OutSize, FALSE, &Event, &IoStatusBlock);

    /* HACK */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->FileObject = FileObject;

    IoSetCompletionRoutine(Irp, KspSynchronousIoControlDeviceCompletion, (PVOID)&IoStatusBlock, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, RequestorMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    *BytesReturned = IoStatusBlock.Information;
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsUnserializeObjectPropertiesFromRegistry(
    IN PFILE_OBJECT FileObject,
    IN HANDLE ParentKey OPTIONAL,
    IN PUNICODE_STRING RegistryPath OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsCacheMedium(
    IN  PUNICODE_STRING SymbolicLink,
    IN  PKSPIN_MEDIUM Medium,
    IN  ULONG PinDirection)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

