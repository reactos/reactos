/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/api.c
 * PURPOSE:         KS API functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

const GUID GUID_NULL              = {0x00000000L, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
const GUID KSMEDIUMSETID_Standard = {0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAcquireResetValue(
    IN  PIRP Irp,
    OUT KSRESET* ResetValue)
{
    PIO_STACK_LOCATION IoStack;
    KSRESET* Value;
    NTSTATUS Status = STATUS_SUCCESS;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* check if there is reset value provided */
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSRESET))
        return STATUS_INVALID_PARAMETER;

    if (Irp->RequestorMode == UserMode)
    {
        /* need to probe the buffer */
        _SEH2_TRY
        {
            ProbeForRead(IoStack->Parameters.DeviceIoControl.Type3InputBuffer, sizeof(KSRESET), sizeof(UCHAR));
            Value = (KSRESET*)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
            *ResetValue = *Value;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Exception, get the error code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        Value = (KSRESET*)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
        *ResetValue = *Value;
    }

    return Status;
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
    PKSIDEVICE_HEADER Header = (PKSIDEVICE_HEADER)DevHeader;

    KeEnterCriticalRegion();

    if (Exclusive)
    {
        ExAcquireResourceExclusiveLite(&Header->SecurityLock, TRUE);
    }
    else
    {
        ExAcquireResourceSharedLite(&Header->SecurityLock, TRUE);
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

    DPRINT("KsReleaseDevice\n");

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
    //PIO_STACK_LOCATION IoStack;
    PLIST_ENTRY ListEntry;
    NTSTATUS Status;

    /* get current irp stack */
    //IoStack = IoGetCurrentIrpStackLocation(Irp);

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
    //PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* get current irp stack */
    //IoStack = IoGetCurrentIrpStackLocation(Irp);

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
    @implemented
*/
KSDDKAPI
ACCESS_MASK
NTAPI
KsQueryObjectAccessMask(
    IN KSOBJECT_HEADER Header)
{
    PKSIOBJECT_HEADER ObjectHeader = (PKSIOBJECT_HEADER)Header;

    /* return access mask */
    return ObjectHeader->AccessMask;

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
    @implemented
*/
KSDDKAPI
PKSOBJECT_CREATE_ITEM
NTAPI
KsQueryObjectCreateItem(
    IN KSOBJECT_HEADER Header)
{
    PKSIOBJECT_HEADER ObjectHeader = (PKSIOBJECT_HEADER)Header;
    return ObjectHeader->OriginalCreateItem;
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
        //ASSERT(Entry->ReferenceCount == 0);
        //ASSERT(IsListEmpty(&Entry->ObjectItemList));

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
    Header = AllocateItem(PagedPool, sizeof(KSIDEVICE_HEADER));

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
    /* initialize object bag lists */
    InitializeListHead(&Header->ObjectBags);

    /* initialize create item list */
    InitializeListHead(&Header->ItemList);

    /* initialize basic header */
    Header->BasicHeader.Type = KsObjectTypeDevice;
    Header->BasicHeader.KsDevice = &Header->KsDevice;
    Header->BasicHeader.Parent.KsDevice = &Header->KsDevice;

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
    FreeItem(Header);
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
    //PDEVICE_EXTENSION DeviceExtension;
    //PKSIDEVICE_HEADER DeviceHeader;
    PKSIOBJECT_HEADER ObjectHeader;
    //PKSOBJECT_CREATE_ITEM CreateItem;
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
    //DeviceExtension = (PDEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;
    /* get device header */
    //DeviceHeader = DeviceExtension->DeviceHeader;

    /* sanity check */
    ASSERT(IoStack->FileObject);
    /* check for an file object */

    /* allocate the object header */
    ObjectHeader = AllocateItem(NonPagedPool, sizeof(KSIOBJECT_HEADER));
    if (!ObjectHeader)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize object header */
    RtlZeroMemory(ObjectHeader, sizeof(KSIOBJECT_HEADER));

    /* initialize create item list */
    InitializeListHead(&ObjectHeader->ItemList);

    /* get create item */
    //CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    /* do we have a name */
    if (IoStack->FileObject->FileName.Buffer)
    {
        /* copy object class */
        ObjectHeader->ObjectClass.MaximumLength = IoStack->FileObject->FileName.MaximumLength;
        ObjectHeader->ObjectClass.Buffer = AllocateItem(NonPagedPool, ObjectHeader->ObjectClass.MaximumLength);
        if (!ObjectHeader->ObjectClass.Buffer)
        {
            FreeItem(ObjectHeader);
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
    IoStack->FileObject->FsContext2 = ObjectHeader;

    /* store parent device */
    ObjectHeader->ParentDeviceObject = IoGetRelatedDeviceObject(IoStack->FileObject);

    /* store originating create item */
    ObjectHeader->OriginalCreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    /* FIXME store access mask see KsQueryObjectAccessMask */
    ObjectHeader->AccessMask = IoStack->Parameters.Create.SecurityContext->DesiredAccess;


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

    DPRINT("KsFreeObjectHeader Header %p Class %wZ\n", Header, &ObjectHeader->ObjectClass);

    if (ObjectHeader->ObjectClass.Buffer)
    {
        /* release object class buffer */
        FreeItem(ObjectHeader->ObjectClass.Buffer);
    }

    if (ObjectHeader->Unknown)
    {
        /* release associated object */
        ObjectHeader->Unknown->lpVtbl->Release(ObjectHeader->Unknown);
    }

    /* free create items */
    KspFreeCreateItems(&ObjectHeader->ItemList);

    /* free object header */
    FreeItem(ObjectHeader);

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

    DPRINT("KsAddObjectCreateItemToDeviceHeader entered\n");

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
    DPRINT("KsAddObjectCreateItemToDeviceHeader Status %x\n", Status);
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

    DPRINT("KsAddObjectCreateItemToDeviceHeader entered\n");

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
    CreateEntry = AllocateItem(NonPagedPool, sizeof(CREATE_ITEM_ENTRY));

    /* check for allocation success */
    if (!CreateEntry)
    {
        /* not enough resources */
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    if (AllocateEntry)
    {
        /* allocate create item */
        Item = AllocateItem(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
        if (!Item)
        {
            /* no memory */
            FreeItem(CreateEntry);
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
        Item->ObjectClass.Buffer = AllocateItem(NonPagedPool, Item->ObjectClass.MaximumLength);
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
    UNIMPLEMENTED;
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
    ObjectHeader = (PKSIOBJECT_HEADER)FileObject->FsContext2;

    /* check if there is fast device io function */
    if (ObjectHeader && ObjectHeader->DispatchTable.FastDeviceIoControl)
    {
        IoStatusBlock.Status = STATUS_UNSUCCESSFUL;
        IoStatusBlock.Information = 0;

        /* send the request */
        Status = ObjectHeader->DispatchTable.FastDeviceIoControl(FileObject, TRUE, InBuffer, InSize, OutBuffer, OutSize, IoControl, &IoStatusBlock, DeviceObject);
        /* check if the request was handled */
        //DPRINT("Handled %u Status %x Length %u\n", Status, IoStatusBlock.Status, IoStatusBlock.Information);
        if (Status)
        {
            /* store bytes returned */
            *BytesReturned = (ULONG)IoStatusBlock.Information;
            /* return status */
            return IoStatusBlock.Status;
        }
    }

    /* initialize the event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* create the irp */
    Irp =  IoBuildDeviceIoControlRequest(IoControl, DeviceObject, InBuffer, InSize, OutBuffer, OutSize, FALSE, &Event, &IoStatusBlock);

    if (!Irp)
    {
        /* no memory to allocate the irp */
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    /* Store Fileobject */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->FileObject = FileObject;

    if (IoControl == IOCTL_KS_WRITE_STREAM)
    {
        Irp->AssociatedIrp.SystemBuffer = OutBuffer;
    }
    else if (IoControl == IOCTL_KS_READ_STREAM)
    {
        Irp->AssociatedIrp.SystemBuffer = InBuffer;
    }

    IoSetCompletionRoutine(Irp, KspSynchronousIoControlDeviceCompletion, (PVOID)&IoStatusBlock, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, RequestorMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    *BytesReturned = (ULONG)IoStatusBlock.Information;
    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsUnserializeObjectPropertiesFromRegistry(
    IN PFILE_OBJECT FileObject,
    IN HANDLE ParentKey OPTIONAL,
    IN PUNICODE_STRING RegistryPath OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsCacheMedium(
    IN  PUNICODE_STRING SymbolicLink,
    IN  PKSPIN_MEDIUM Medium,
    IN  ULONG PinDirection)
{
    HANDLE hKey;
    UNICODE_STRING Path;
    UNICODE_STRING BasePath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\MediumCache\\");
    UNICODE_STRING GuidString;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN PathAdjusted = FALSE;
    ULONG Value = 0;

    /* first check if the medium is standard */
    if (IsEqualGUIDAligned(&KSMEDIUMSETID_Standard, &Medium->Set) ||
        IsEqualGUIDAligned(&GUID_NULL, &Medium->Set))
    {
        /* no need to cache that */
        return STATUS_SUCCESS;
    }

    /* convert guid to string */
    Status = RtlStringFromGUID(&Medium->Set, &GuidString);
    if (!NT_SUCCESS(Status))
        return Status;

    /* allocate path buffer */
    Path.Length = 0;
    Path.MaximumLength = BasePath.MaximumLength + GuidString.MaximumLength + 10 * sizeof(WCHAR);
    Path.Buffer = AllocateItem(PagedPool, Path.MaximumLength);
    if (!Path.Buffer)
    {
        /* not enough resources */
        RtlFreeUnicodeString(&GuidString);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeStringToString(&Path, &BasePath);
    RtlAppendUnicodeStringToString(&Path, &GuidString);
    RtlAppendUnicodeToString(&Path, L"-");
    /* FIXME append real instance id */
    RtlAppendUnicodeToString(&Path, L"0");
    RtlAppendUnicodeToString(&Path, L"-");
    /* FIXME append real instance id */
    RtlAppendUnicodeToString(&Path, L"0");

    /* free guid string */
    RtlFreeUnicodeString(&GuidString);

    /* initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes, &Path, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    /* create the key */
    Status = ZwCreateKey(&hKey, GENERIC_WRITE, &ObjectAttributes, 0, NULL, 0, NULL);

    /* free path buffer */
    FreeItem(Path.Buffer);

    if (NT_SUCCESS(Status))
    {
        /* store symbolic link */
        if (SymbolicLink->Buffer[1] == L'?' && SymbolicLink->Buffer[2] == L'?')
        {
            /* replace kernel path with user mode path */
            SymbolicLink->Buffer[1] = L'\\';
            PathAdjusted = TRUE;
        }

        /* store the key */
        Status = ZwSetValueKey(hKey, SymbolicLink, 0, REG_DWORD, &Value, sizeof(ULONG));

        if (PathAdjusted)
        {
            /* restore kernel path */
            SymbolicLink->Buffer[1] = L'?';
        }

        ZwClose(hKey);
    }

    /* done */
    return Status;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
DllInitialize(
    PUNICODE_STRING  RegistryPath)
{
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
KopDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKO_OBJECT_HEADER Header;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get ko object header */
    Header = (PKO_OBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* free ks object header */
    KsFreeObjectHeader(Header->ObjectHeader);

    /* free ko object header */
    FreeItem(Header);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* release bus object */
    KsDereferenceBusObject((KSDEVICE_HEADER)DeviceExtension->DeviceHeader);

    /* complete request */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    CompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}



static KSDISPATCH_TABLE KoDispatchTable =
{
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KopDispatchClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastReadFailure,
};


NTSTATUS
NTAPI
KopDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKO_OBJECT_HEADER Header = NULL;
    PIO_STACK_LOCATION IoStack;
    PKO_DRIVER_EXTENSION DriverObjectExtension;
    NTSTATUS Status;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (!IoStack->FileObject)
    {
        DPRINT1("FileObject not attached!\n");
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    /* get driver object extension */
    DriverObjectExtension = (PKO_DRIVER_EXTENSION)IoGetDriverObjectExtension(DeviceObject->DriverObject, (PVOID)KoDriverInitialize);
    if (!DriverObjectExtension)
    {
        DPRINT1("No DriverObjectExtension!\n");
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    /* allocate ko object header */
    Header = (PKO_OBJECT_HEADER)AllocateItem(NonPagedPool, sizeof(KO_OBJECT_HEADER));
    if (!Header)
    {
        DPRINT1("failed to allocate KO_OBJECT_HEADER\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    /* initialize create item */
    Header->CreateItem.Create = KopDispatchCreate;
    RtlInitUnicodeString(&Header->CreateItem.ObjectClass, KOSTRING_CreateObject);


    /* now allocate the object header */
    Status = KsAllocateObjectHeader(&Header->ObjectHeader, 1, &Header->CreateItem, Irp, &KoDispatchTable);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        goto cleanup;
    }

    /* FIXME
     * extract clsid and interface id from irp
     * call the standard create handler
     */

    UNIMPLEMENTED;

    IoStack->FileObject->FsContext2 = (PVOID)Header;

    Irp->IoStatus.Status = Status;
    CompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

cleanup:

    if (Header && Header->ObjectHeader)
        KsFreeObjectHeader(Header->ObjectHeader);

    if (Header)
        FreeItem(Header);

    Irp->IoStatus.Status = Status;
    CompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}



NTSTATUS
NTAPI
KopAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status = STATUS_DEVICE_REMOVED;
    PDEVICE_OBJECT FunctionalDeviceObject= NULL;
    PDEVICE_OBJECT NextDeviceObject;
    PDEVICE_EXTENSION DeviceExtension;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* create the device object */
    Status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_KS, FILE_DEVICE_SECURE_OPEN, FALSE, &FunctionalDeviceObject);
    if (!NT_SUCCESS(Status))
        return Status;

    /* allocate the create item */
    CreateItem = AllocateItem(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));

    if (!CreateItem)
    {
        /* not enough memory */
        IoDeleteDevice(FunctionalDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize create item */
    CreateItem->Create = KopDispatchCreate;
    RtlInitUnicodeString(&CreateItem->ObjectClass, KOSTRING_CreateObject);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)FunctionalDeviceObject->DeviceExtension;

    /* now allocate the device header */
    Status = KsAllocateDeviceHeader((KSDEVICE_HEADER*)&DeviceExtension->DeviceHeader, 1, CreateItem);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        IoDeleteDevice(FunctionalDeviceObject);
        FreeItem(CreateItem);
        return Status;
    }

    /* now attach to device stack */
    NextDeviceObject = IoAttachDeviceToDeviceStack(FunctionalDeviceObject, PhysicalDeviceObject);
    if (NextDeviceObject)
    {
        /* store pnp base object */
         KsSetDevicePnpAndBaseObject((KSDEVICE_HEADER)DeviceExtension->DeviceHeader, NextDeviceObject, FunctionalDeviceObject);
        /* set device flags */
        FunctionalDeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
        FunctionalDeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;
    }
    else
    {
        /* failed */
        KsFreeDeviceHeader((KSDEVICE_HEADER)DeviceExtension->DeviceHeader);
        FreeItem(CreateItem);
        IoDeleteDevice(FunctionalDeviceObject);
        Status =  STATUS_DEVICE_REMOVED;
    }

    /* return result */
    return Status;
}


/*
    @implemented
*/
COMDDKAPI
NTSTATUS
NTAPI
KoDeviceInitialize(
    IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    return KsAddObjectCreateItemToDeviceHeader((KSDEVICE_HEADER)DeviceExtension->DeviceHeader, KopDispatchCreate, NULL, KOSTRING_CreateObject, NULL);
}

/*
    @implemented
*/
COMDDKAPI
NTSTATUS
NTAPI
KoDriverInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName,
    IN KoCreateObjectHandler CreateObjectHandler)
{
    PKO_DRIVER_EXTENSION DriverObjectExtension;
    NTSTATUS Status;

    /* allocate driver object extension */
    Status = IoAllocateDriverObjectExtension(DriverObject, (PVOID)KoDriverInitialize, sizeof(KO_DRIVER_EXTENSION), (PVOID*)&DriverObjectExtension);

    /* did it work */
    if (NT_SUCCESS(Status))
    {
        /* store create handler */
        DriverObjectExtension->CreateObjectHandler = CreateObjectHandler;

         /* Setting our IRP handlers */
        DriverObject->MajorFunction[IRP_MJ_PNP] = KsDefaultDispatchPnp;
        DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;

        /* The driver unload routine */
        DriverObject->DriverUnload = KsNullDriverUnload;

        /* The driver-supplied AddDevice */
        DriverObject->DriverExtension->AddDevice = KopAddDevice;

        /* KS handles these */
        DPRINT1("Setting KS function handlers\n");
        KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
        KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
        KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);

    }

    return Status;
}

/*
    @unimplemented
*/
COMDDKAPI
VOID
NTAPI
KoRelease(
    IN REFCLSID ClassId)
{

}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsAcquireControl(
    IN PVOID Object)
{
    PKSBASIC_HEADER BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Object - sizeof(KSBASIC_HEADER));

    /* sanity check */
    ASSERT(BasicHeader->Type == KsObjectTypeFilter || BasicHeader->Type == KsObjectTypePin);

    KeWaitForSingleObject(BasicHeader->ControlMutex, Executive, KernelMode, FALSE, NULL);

}

/*
    @implemented
*/
VOID
NTAPI
KsReleaseControl(
    IN PVOID  Object)
{
    PKSBASIC_HEADER BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Object - sizeof(KSBASIC_HEADER));

    /* sanity check */
    ASSERT(BasicHeader->Type == KsObjectTypeFilter || BasicHeader->Type == KsObjectTypePin);

    KeReleaseMutex(BasicHeader->ControlMutex, FALSE);
}



/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsAcquireDevice(
    IN PKSDEVICE Device)
{
    IKsDevice *KsDevice;
    PKSIDEVICE_HEADER DeviceHeader;

    DPRINT("KsAcquireDevice\n");
    DeviceHeader = (PKSIDEVICE_HEADER)CONTAINING_RECORD(Device, KSIDEVICE_HEADER, KsDevice);

    /* get device interface*/
    KsDevice = (IKsDevice*)&DeviceHeader->BasicHeader.OuterUnknown;

    /* acquire device mutex */
    KsDevice->lpVtbl->AcquireDevice(KsDevice);
}

/*
    @implemented
*/
VOID
NTAPI
KsReleaseDevice(
    IN PKSDEVICE  Device)
{
    IKsDevice *KsDevice;
    PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)CONTAINING_RECORD(Device, KSIDEVICE_HEADER, KsDevice);

    /* get device interface*/
    KsDevice = (IKsDevice*)&DeviceHeader->BasicHeader.OuterUnknown;

    /* release device mutex */
    KsDevice->lpVtbl->ReleaseDevice(KsDevice);
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsTerminateDevice(
    IN PDEVICE_OBJECT DeviceObject)
{
    IKsDevice *KsDevice;
    PKSIDEVICE_HEADER DeviceHeader;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* get device interface*/
    KsDevice = (IKsDevice*)&DeviceHeader->BasicHeader.OuterUnknown;

    /* now free device header */
    KsFreeDeviceHeader((KSDEVICE_HEADER)DeviceHeader);

    /* release interface when available */
    if (KsDevice)
    {
        /* delete IKsDevice interface */
        KsDevice->lpVtbl->Release(KsDevice);
    }
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsCompletePendingRequest(
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(Irp->IoStatus.Status != STATUS_PENDING);

    if (IoStack->MajorFunction != IRP_MJ_CLOSE)
    {
        /* can be completed immediately */
        CompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }

    /* did close operation fail */
    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        /* closing failed, complete irp */
        CompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }

    /* FIXME
     * delete object / device header
     * remove dead pin / filter instance
     */
    UNIMPLEMENTED;

}

NTSTATUS
NTAPI
KspSetGetBusDataCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    /* signal completion */
    KeSetEvent((PRKEVENT)Context, IO_NO_INCREMENT, FALSE);

    /* more work needs be done, so dont free the irp */
    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
KspDeviceSetGetBusData(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    IN BOOL bGet)
{
    PIO_STACK_LOCATION IoStack;
    PIRP Irp;
    NTSTATUS Status;
    KEVENT Event;

    /* allocate the irp */
    Irp = IoAllocateIrp(1, /*FIXME */
                        FALSE);

    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize the event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* get next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);

    /* setup a completion routine */
    IoSetCompletionRoutine(Irp, KspSetGetBusDataCompletion, (PVOID)&Event, TRUE, TRUE, TRUE);

    /* setup parameters */
    IoStack->Parameters.ReadWriteConfig.Buffer = Buffer;
    IoStack->Parameters.ReadWriteConfig.Length = Length;
    IoStack->Parameters.ReadWriteConfig.Offset = Offset;
    IoStack->Parameters.ReadWriteConfig.WhichSpace = DataType;
    /* setup function code */
    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = (bGet ? IRP_MN_READ_CONFIG : IRP_MN_WRITE_CONFIG);

    /* lets call the driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* is the request still pending */
    if (Status == STATUS_PENDING)
    {
        /* have a nap */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        /* update status */
        Status = Irp->IoStatus.Status;
    }

    /* free the irp */
    IoFreeIrp(Irp);
    /* done */
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
ULONG
NTAPI
KsDeviceSetBusData(
    IN PKSDEVICE Device,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length)
{
    return KspDeviceSetGetBusData(Device->PhysicalDeviceObject, /* is this right? */
                                  DataType, Buffer, Offset, Length, FALSE);
}


/*
    @implemented
*/
KSDDKAPI
ULONG
NTAPI
KsDeviceGetBusData(
    IN PKSDEVICE Device,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length)
{
    return KspDeviceSetGetBusData(Device->PhysicalDeviceObject, /* is this right? */
                                  DataType, Buffer, Offset, Length, TRUE);

}

/*
    @implemented
*/
KSDDKAPI
void
NTAPI
KsDeviceRegisterAdapterObject(
    IN PKSDEVICE Device,
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG MaxMappingsByteCount,
    IN ULONG MappingTableStride)
{
    PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)CONTAINING_RECORD(Device, KSIDEVICE_HEADER, KsDevice);

    DeviceHeader->AdapterObject = AdapterObject;
    DeviceHeader->MaxMappingsByteCount = MaxMappingsByteCount;
    DeviceHeader->MappingTableStride = MappingTableStride;

}


/*
    @implemented
*/
KSDDKAPI
PVOID
NTAPI
KsGetFirstChild(
    IN PVOID Object)
{
    PKSBASIC_HEADER BasicHeader;

    /* get the basic header */
    BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Object - sizeof(KSBASIC_HEADER));

    /* type has to be either a device or a filter factory */
    ASSERT(BasicHeader->Type == KsObjectTypeDevice || BasicHeader->Type == KsObjectTypeFilterFactory);

    return (PVOID)BasicHeader->FirstChild.Filter;
}

/*
    @implemented
*/
KSDDKAPI
PVOID
NTAPI
KsGetNextSibling(
    IN PVOID Object)
{
    PKSBASIC_HEADER BasicHeader;

    /* get the basic header */
    BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Object - sizeof(KSBASIC_HEADER));

    ASSERT(BasicHeader->Type == KsObjectTypeDevice || BasicHeader->Type == KsObjectTypeFilterFactory ||
           BasicHeader->Type == KsObjectTypeFilter || BasicHeader->Type == KsObjectTypePin);

    return (PVOID)BasicHeader->Next.Pin;
}

ULONG
KspCountMethodSets(
    IN PKSAUTOMATION_TABLE  AutomationTableA OPTIONAL,
    IN PKSAUTOMATION_TABLE  AutomationTableB OPTIONAL)
{
    ULONG Index, SubIndex, Count;
    BOOL bFound;

    if (!AutomationTableA)
        return AutomationTableB->MethodSetsCount;

    if (!AutomationTableB)
        return AutomationTableA->MethodSetsCount;


    DPRINT("AutomationTableA MethodItemSize %lu MethodSetsCount %lu\n", AutomationTableA->MethodItemSize, AutomationTableA->MethodSetsCount);
    DPRINT("AutomationTableB MethodItemSize %lu MethodSetsCount %lu\n", AutomationTableB->MethodItemSize, AutomationTableB->MethodSetsCount);

    if (AutomationTableA->MethodItemSize && AutomationTableB->MethodItemSize)
    {
        /* sanity check */
        ASSERT(AutomationTableA->MethodItemSize  == AutomationTableB->MethodItemSize);
    }

    /* now iterate all property sets and compare their guids */
    Count = AutomationTableA->MethodSetsCount;

    for(Index = 0; Index < AutomationTableB->MethodSetsCount; Index++)
    {
        /* set found to false */
        bFound = FALSE;

        for(SubIndex = 0; SubIndex < AutomationTableA->MethodSetsCount; SubIndex++)
        {
            if (IsEqualGUIDAligned(AutomationTableB->MethodSets[Index].Set, AutomationTableA->MethodSets[SubIndex].Set))
            {
                /* same property set found */
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
            Count++;
    }

    return Count;
}

ULONG
KspCountEventSets(
    IN PKSAUTOMATION_TABLE  AutomationTableA OPTIONAL,
    IN PKSAUTOMATION_TABLE  AutomationTableB OPTIONAL)
{
    ULONG Index, SubIndex, Count;
    BOOL bFound;

    if (!AutomationTableA)
        return AutomationTableB->EventSetsCount;

    if (!AutomationTableB)
        return AutomationTableA->EventSetsCount;

    DPRINT("AutomationTableA EventItemSize %lu EventSetsCount %lu\n", AutomationTableA->EventItemSize, AutomationTableA->EventSetsCount);
    DPRINT("AutomationTableB EventItemSize %lu EventSetsCount %lu\n", AutomationTableB->EventItemSize, AutomationTableB->EventSetsCount);

    if (AutomationTableA->EventItemSize && AutomationTableB->EventItemSize)
    {
        /* sanity check */
        ASSERT(AutomationTableA->EventItemSize == AutomationTableB->EventItemSize);
    }

    /* now iterate all Event sets and compare their guids */
    Count = AutomationTableA->EventSetsCount;

    for(Index = 0; Index < AutomationTableB->EventSetsCount; Index++)
    {
        /* set found to false */
        bFound = FALSE;

        for(SubIndex = 0; SubIndex < AutomationTableA->EventSetsCount; SubIndex++)
        {
            if (IsEqualGUIDAligned(AutomationTableB->EventSets[Index].Set, AutomationTableA->EventSets[SubIndex].Set))
            {
                /* same Event set found */
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
            Count++;
    }

    return Count;
}


ULONG
KspCountPropertySets(
    IN PKSAUTOMATION_TABLE  AutomationTableA OPTIONAL,
    IN PKSAUTOMATION_TABLE  AutomationTableB OPTIONAL)
{
    ULONG Index, SubIndex, Count;
    BOOL bFound;

    if (!AutomationTableA)
        return AutomationTableB->PropertySetsCount;

    if (!AutomationTableB)
        return AutomationTableA->PropertySetsCount;

    /* sanity check */
    DPRINT("AutomationTableA PropertyItemSize %lu PropertySetsCount %lu\n", AutomationTableA->PropertyItemSize, AutomationTableA->PropertySetsCount);
    DPRINT("AutomationTableB PropertyItemSize %lu PropertySetsCount %lu\n", AutomationTableB->PropertyItemSize, AutomationTableB->PropertySetsCount);
    ASSERT(AutomationTableA->PropertyItemSize == AutomationTableB->PropertyItemSize);

    /* now iterate all property sets and compare their guids */
    Count = AutomationTableA->PropertySetsCount;

    for(Index = 0; Index < AutomationTableB->PropertySetsCount; Index++)
    {
        /* set found to false */
        bFound = FALSE;

        for(SubIndex = 0; SubIndex < AutomationTableA->PropertySetsCount; SubIndex++)
        {
            if (IsEqualGUIDAligned(AutomationTableB->PropertySets[Index].Set, AutomationTableA->PropertySets[SubIndex].Set))
            {
                /* same property set found */
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
            Count++;
    }

    return Count;
}

NTSTATUS
KspCopyMethodSets(
    OUT PKSAUTOMATION_TABLE  Table,
    IN PKSAUTOMATION_TABLE  AutomationTableA OPTIONAL,
    IN PKSAUTOMATION_TABLE  AutomationTableB OPTIONAL)
{
    ULONG Index, SubIndex, Count;
    BOOL bFound;

    if (!AutomationTableA)
    {
        /* copy of property set */
        RtlMoveMemory((PVOID)Table->MethodSets, AutomationTableB->MethodSets, sizeof(KSMETHOD_SET) * AutomationTableB->MethodSetsCount);
        return STATUS_SUCCESS;
    }
    else if (!AutomationTableB)
    {
        /* copy of property set */
        RtlMoveMemory((PVOID)Table->MethodSets, AutomationTableA->MethodSets, sizeof(KSMETHOD_SET) * AutomationTableA->MethodSetsCount);
        return STATUS_SUCCESS;
    }

    /* first copy all property items from dominant table */
    RtlMoveMemory((PVOID)Table->MethodSets, AutomationTableA->MethodSets, sizeof(KSMETHOD_SET) * AutomationTableA->MethodSetsCount);
    /* set counter */
    Count = AutomationTableA->MethodSetsCount;

    /* now copy entries which aren't available in the dominant table */
    for(Index = 0; Index < AutomationTableB->MethodSetsCount; Index++)
    {
        /* set found to false */
        bFound = FALSE;

        for(SubIndex = 0; SubIndex < AutomationTableA->MethodSetsCount; SubIndex++)
        {
            if (IsEqualGUIDAligned(AutomationTableB->MethodSets[Index].Set, AutomationTableA->MethodSets[SubIndex].Set))
            {
                /* same property set found */
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            /* copy new property item set */
            RtlMoveMemory((PVOID)&Table->MethodSets[Count], &AutomationTableB->MethodSets[Index], sizeof(KSMETHOD_SET));
            Count++;
        }
    }

    return STATUS_SUCCESS;
}

VOID
KspAddPropertyItem(
    OUT PKSPROPERTY_SET OutPropertySet,
    IN PKSPROPERTY_ITEM PropertyItem,
    IN ULONG PropertyItemSize)
{
    PKSPROPERTY_ITEM CurrentPropertyItem;
    ULONG Index;

    // check if the property item is already present
    CurrentPropertyItem = (PKSPROPERTY_ITEM)OutPropertySet->PropertyItem;
    for(Index = 0; Index < OutPropertySet->PropertiesCount; Index++)
    {
        if (CurrentPropertyItem->PropertyId == PropertyItem->PropertyId)
        {
            // item already present
            return;
        }

        // next item
        CurrentPropertyItem = (PKSPROPERTY_ITEM)((ULONG_PTR)CurrentPropertyItem + PropertyItemSize);
    }
    // add item
    RtlCopyMemory(CurrentPropertyItem, PropertyItem, PropertyItemSize);
    OutPropertySet->PropertiesCount++;
}

NTSTATUS
KspMergePropertySet(
    OUT PKSAUTOMATION_TABLE  Table,
    OUT PKSPROPERTY_SET OutPropertySet,
    IN PKSPROPERTY_SET PropertySetA,
    IN PKSPROPERTY_SET PropertySetB,
    IN KSOBJECT_BAG  Bag OPTIONAL)
{
    ULONG PropertyCount, Index;
    PKSPROPERTY_ITEM PropertyItem, CurrentPropertyItem;
    NTSTATUS Status;

    // max properties
    PropertyCount = PropertySetA->PropertiesCount + PropertySetB->PropertiesCount;

    // allocate items
    PropertyItem = AllocateItem(NonPagedPool, Table->PropertyItemSize * PropertyCount);
    if (!PropertyItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (Bag)
    {
        /* add table to object bag */
        Status = KsAddItemToObjectBag(Bag, PropertyItem, NULL);
        /* check for success */
        if (!NT_SUCCESS(Status))
        {
            /* free table */
            FreeItem(Table);
            return Status;
        }
    }

    // copy entries from dominant table
    RtlCopyMemory(PropertyItem, PropertySetA->PropertyItem, Table->PropertyItemSize * PropertySetA->PropertiesCount);

    // init property set
    OutPropertySet->PropertiesCount = PropertySetA->PropertiesCount;
    OutPropertySet->PropertyItem = PropertyItem;

    // copy other entries
    CurrentPropertyItem = (PKSPROPERTY_ITEM)PropertySetB->PropertyItem;
    for(Index = 0; Index < PropertySetB->PropertiesCount; Index++)
    {

        // add entries
        KspAddPropertyItem(OutPropertySet, CurrentPropertyItem, Table->PropertyItemSize);

        // next entry
        CurrentPropertyItem = (PKSPROPERTY_ITEM)((ULONG_PTR)CurrentPropertyItem + Table->PropertyItemSize);
    }

    // done
    return STATUS_SUCCESS;
}


NTSTATUS
KspCopyPropertySets(
    OUT PKSAUTOMATION_TABLE  Table,
    IN PKSAUTOMATION_TABLE  AutomationTableA OPTIONAL,
    IN PKSAUTOMATION_TABLE  AutomationTableB OPTIONAL,
    IN KSOBJECT_BAG  Bag OPTIONAL)
{
    ULONG Index, SubIndex, Count;
    BOOL bFound;
    NTSTATUS Status;

    if (!AutomationTableA)
    {
        /* copy of property set */
        RtlMoveMemory((PVOID)Table->PropertySets, AutomationTableB->PropertySets, sizeof(KSPROPERTY_SET) * AutomationTableB->PropertySetsCount);
        return STATUS_SUCCESS;
    }
    else if (!AutomationTableB)
    {
        /* copy of property set */
        RtlMoveMemory((PVOID)Table->PropertySets, AutomationTableA->PropertySets, sizeof(KSPROPERTY_SET) * AutomationTableA->PropertySetsCount);
        return STATUS_SUCCESS;
    }

    /* first copy all property items from dominant table */
    RtlMoveMemory((PVOID)Table->PropertySets, AutomationTableA->PropertySets, sizeof(KSPROPERTY_SET) * AutomationTableA->PropertySetsCount);
    /* set counter */
    Count = AutomationTableA->PropertySetsCount;

    /* now copy entries which aren't available in the dominant table */
    for(Index = 0; Index < AutomationTableB->PropertySetsCount; Index++)
    {
        /* set found to false */
        bFound = FALSE;

        for(SubIndex = 0; SubIndex < AutomationTableA->PropertySetsCount; SubIndex++)
        {
            if (IsEqualGUIDAligned(AutomationTableB->PropertySets[Index].Set, AutomationTableA->PropertySets[SubIndex].Set))
            {
                /* same property set found */
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            /* copy new property item set */
            RtlMoveMemory((PVOID)&Table->PropertySets[Count], &AutomationTableB->PropertySets[Index], sizeof(KSPROPERTY_SET));
            Count++;
        }
        else
        {
            // merge property sets
            Status = KspMergePropertySet(Table, (PKSPROPERTY_SET)&Table->PropertySets[SubIndex], (PKSPROPERTY_SET)&AutomationTableA->PropertySets[SubIndex], (PKSPROPERTY_SET)&AutomationTableB->PropertySets[Index], Bag);
            if (!NT_SUCCESS(Status))
            {
                // failed to merge
                DPRINT1("[KS] Failed to merge %x\n", Status);
                return Status;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KspCopyEventSets(
    OUT PKSAUTOMATION_TABLE  Table,
    IN PKSAUTOMATION_TABLE  AutomationTableA OPTIONAL,
    IN PKSAUTOMATION_TABLE  AutomationTableB OPTIONAL)
{
    ULONG Index, SubIndex, Count;
    BOOL bFound;

    if (!AutomationTableA)
    {
        /* copy of Event set */
        RtlMoveMemory((PVOID)Table->EventSets, AutomationTableB->EventSets, sizeof(KSEVENT_SET) * AutomationTableB->EventSetsCount);
        return STATUS_SUCCESS;
    }
    else if (!AutomationTableB)
    {
        /* copy of Event set */
        RtlMoveMemory((PVOID)Table->EventSets, AutomationTableA->EventSets, sizeof(KSEVENT_SET) * AutomationTableA->EventSetsCount);
        return STATUS_SUCCESS;
    }

    /* first copy all Event items from dominant table */
    RtlMoveMemory((PVOID)Table->EventSets, AutomationTableA->EventSets, sizeof(KSEVENT_SET) * AutomationTableA->EventSetsCount);
    /* set counter */
    Count = AutomationTableA->EventSetsCount;

    /* now copy entries which aren't available in the dominant table */
    for(Index = 0; Index < AutomationTableB->EventSetsCount; Index++)
    {
        /* set found to false */
        bFound = FALSE;

        for(SubIndex = 0; SubIndex < AutomationTableA->EventSetsCount; SubIndex++)
        {
            if (IsEqualGUIDAligned(AutomationTableB->EventSets[Index].Set, AutomationTableA->EventSets[SubIndex].Set))
            {
                /* same Event set found */
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            /* copy new Event item set */
            RtlMoveMemory((PVOID)&Table->EventSets[Count], &AutomationTableB->EventSets[Index], sizeof(KSEVENT_SET));
            Count++;
        }
    }

    return STATUS_SUCCESS;
}


/*
    @implemented
*/
NTSTATUS
NTAPI
KsMergeAutomationTables(
    OUT PKSAUTOMATION_TABLE  *AutomationTableAB,
    IN PKSAUTOMATION_TABLE  AutomationTableA OPTIONAL,
    IN PKSAUTOMATION_TABLE  AutomationTableB OPTIONAL,
    IN KSOBJECT_BAG  Bag OPTIONAL)
{
    PKSAUTOMATION_TABLE Table;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!AutomationTableA && !AutomationTableB)
    {
        /* nothing to merge */
        return STATUS_SUCCESS;
    }

    /* allocate an automation table */
    Table = AllocateItem(NonPagedPool, sizeof(KSAUTOMATION_TABLE));
    if (!Table)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (Bag)
    {
        /* add table to object bag */
        Status = KsAddItemToObjectBag(Bag, Table, NULL);
        /* check for success */
        if (!NT_SUCCESS(Status))
        {
            /* free table */
            FreeItem(Table);
            return Status;
        }
    }

    /* count property sets */
    Table->PropertySetsCount = KspCountPropertySets(AutomationTableA, AutomationTableB);

    if (Table->PropertySetsCount)
    {
        if (AutomationTableA)
        {
            /* use item size from dominant automation table */
            Table->PropertyItemSize = AutomationTableA->PropertyItemSize;
        }
        else
        {
            /* use item size from 2nd automation table */
            Table->PropertyItemSize = AutomationTableB->PropertyItemSize;
        }

        if (AutomationTableA && AutomationTableB)
        {
            // FIXME handle different property item sizes
            ASSERT(AutomationTableA->PropertyItemSize == AutomationTableB->PropertyItemSize);
        }

        /* now allocate the property sets */
        Table->PropertySets = AllocateItem(NonPagedPool, sizeof(KSPROPERTY_SET) * Table->PropertySetsCount);

        if (!Table->PropertySets)
        {
            /* not enough memory */
            goto cleanup;
        }

        if (Bag)
        {
            /* add set to property bag */
            Status = KsAddItemToObjectBag(Bag, (PVOID)Table->PropertySets, NULL);
            /* check for success */
            if (!NT_SUCCESS(Status))
            {
                /* cleanup table */
                goto cleanup;
            }
        }
        /* now copy the property sets */
        Status = KspCopyPropertySets(Table, AutomationTableA, AutomationTableB, Bag);
        if(!NT_SUCCESS(Status))
            goto cleanup;

    }

    /* now count the method sets */
    Table->MethodSetsCount = KspCountMethodSets(AutomationTableA, AutomationTableB);

    if (Table->MethodSetsCount)
    {
        if (AutomationTableA)
        {
            /* use item size from dominant automation table */
            Table->MethodItemSize  = AutomationTableA->MethodItemSize;
        }
        else
        {
            /* use item size from 2nd automation table */
            Table->MethodItemSize = AutomationTableB->MethodItemSize;
        }

        /* now allocate the property sets */
        Table->MethodSets = AllocateItem(NonPagedPool, sizeof(KSMETHOD_SET) * Table->MethodSetsCount);

        if (!Table->MethodSets)
        {
            /* not enough memory */
            goto cleanup;
        }

        if (Bag)
        {
            /* add set to property bag */
            Status = KsAddItemToObjectBag(Bag, (PVOID)Table->MethodSets, NULL);
            /* check for success */
            if (!NT_SUCCESS(Status))
            {
                /* cleanup table */
                goto cleanup;
            }
        }
        /* now copy the property sets */
        Status = KspCopyMethodSets(Table, AutomationTableA, AutomationTableB);
        if(!NT_SUCCESS(Status))
            goto cleanup;
    }


    /* now count the event sets */
    Table->EventSetsCount = KspCountEventSets(AutomationTableA, AutomationTableB);

    if (Table->EventSetsCount)
    {
        if (AutomationTableA)
        {
            /* use item size from dominant automation table */
            Table->EventItemSize  = AutomationTableA->EventItemSize;
        }
        else
        {
            /* use item size from 2nd automation table */
            Table->EventItemSize = AutomationTableB->EventItemSize;
        }

        /* now allocate the property sets */
        Table->EventSets = AllocateItem(NonPagedPool, sizeof(KSEVENT_SET) * Table->EventSetsCount);

        if (!Table->EventSets)
        {
            /* not enough memory */
            goto cleanup;
        }

        if (Bag)
        {
            /* add set to property bag */
            Status = KsAddItemToObjectBag(Bag, (PVOID)Table->EventSets, NULL);
            /* check for success */
            if (!NT_SUCCESS(Status))
            {
                /* cleanup table */
                goto cleanup;
            }
        }
        /* now copy the property sets */
        Status = KspCopyEventSets(Table, AutomationTableA, AutomationTableB);
        if(!NT_SUCCESS(Status))
            goto cleanup;
    }

    /* store result */
    *AutomationTableAB = Table;

    return Status;


cleanup:

    if (Table)
    {
        if (Table->PropertySets)
        {
            /* clean property sets */
            if (!Bag || !NT_SUCCESS(KsRemoveItemFromObjectBag(Bag, (PVOID)Table->PropertySets, TRUE)))
                FreeItem((PVOID)Table->PropertySets);
        }

        if (Table->MethodSets)
        {
            /* clean property sets */
            if (!Bag || !NT_SUCCESS(KsRemoveItemFromObjectBag(Bag, (PVOID)Table->MethodSets, TRUE)))
                FreeItem((PVOID)Table->MethodSets);
        }

        if (Table->EventSets)
        {
            /* clean property sets */
            if (!Bag || !NT_SUCCESS(KsRemoveItemFromObjectBag(Bag, (PVOID)Table->EventSets, TRUE)))
                FreeItem((PVOID)Table->EventSets);
        }

        if (!Bag || !NT_SUCCESS(KsRemoveItemFromObjectBag(Bag, Table, TRUE)))
                FreeItem(Table);
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

/*
    @unimplemented
*/
KSDDKAPI
PUNKNOWN
NTAPI
KsRegisterAggregatedClientUnknown(
    IN PVOID  Object,
    IN PUNKNOWN  ClientUnknown)
{
    PKSBASIC_HEADER BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Object - sizeof(KSBASIC_HEADER));

    /* sanity check */
    ASSERT(BasicHeader->Type == KsObjectTypeDevice || BasicHeader->Type == KsObjectTypeFilterFactory ||
           BasicHeader->Type == KsObjectTypeFilter || BasicHeader->Type == KsObjectTypePin);

    if (BasicHeader->ClientAggregate)
    {
        /* release existing aggregate */
        BasicHeader->ClientAggregate->lpVtbl->Release(BasicHeader->ClientAggregate);
    }

    /* increment reference count */
    ClientUnknown->lpVtbl->AddRef(ClientUnknown);

    /* store client aggregate */
    BasicHeader->ClientAggregate = ClientUnknown;

    /* return objects outer unknown */
    return BasicHeader->OuterUnknown;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsRegisterFilterWithNoKSPins(
    IN PDEVICE_OBJECT  DeviceObject,
    IN const GUID*  InterfaceClassGUID,
    IN ULONG  PinCount,
    IN BOOL*  PinDirection,
    IN KSPIN_MEDIUM*  MediumList,
    IN GUID*  CategoryList OPTIONAL)
{
    ULONG Size, Index;
    NTSTATUS Status;
    PWSTR SymbolicLinkList;
    //PUCHAR Buffer;
    HANDLE hKey;
    UNICODE_STRING InterfaceString;
    //UNICODE_STRING FilterData = RTL_CONSTANT_STRING(L"FilterData");

    if (!InterfaceClassGUID || !PinCount || !PinDirection || !MediumList)
    {
        /* all these parameters are required */
        return STATUS_INVALID_PARAMETER;
    }

    /* calculate filter data value size */
    Size = PinCount * sizeof(KSPIN_MEDIUM);
    if (CategoryList)
    {
        /* add category list */
        Size += PinCount * sizeof(GUID);
    }

    /* FIXME generate filter data blob */
    UNIMPLEMENTED;

    /* get symbolic link list */
    Status = IoGetDeviceInterfaces(InterfaceClassGUID, DeviceObject, DEVICE_INTERFACE_INCLUDE_NONACTIVE, &SymbolicLinkList);
    if (NT_SUCCESS(Status))
    {
        /* initialize first symbolic link */
        RtlInitUnicodeString(&InterfaceString, SymbolicLinkList);

        /* open first device interface registry key */
        Status = IoOpenDeviceInterfaceRegistryKey(&InterfaceString, GENERIC_WRITE, &hKey);

        if (NT_SUCCESS(Status))
        {
            /* write filter data */
            //Status = ZwSetValueKey(hKey, &FilterData, 0, REG_BINARY, Buffer, Size);

            /* close the key */
            ZwClose(hKey);
        }

        if (PinCount)
        {
            /* update medium cache */
            for(Index = 0; Index < PinCount; Index++)
            {
                KsCacheMedium(&InterfaceString, &MediumList[Index], PinDirection[Index]);
            }
        }

        /* free the symbolic link list */
        FreeItem(SymbolicLinkList);
    }

    return Status;
}
