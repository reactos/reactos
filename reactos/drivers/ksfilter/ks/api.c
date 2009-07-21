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
    KeInitializeSpinLock(&Header->ItemListLock); //FIXME

    /* initialize device mutex */
    KeInitializeMutex(&Header->DeviceMutex, 0);

    /* initialize target device list */
    InitializeListHead(&Header->TargetDeviceList);
    /* initialize power dispatch list */
    InitializeListHead(&Header->PowerDispatchList);

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
            /* store provided create items */
            Header->ItemList[Index].CreateItem = &ItemsList[Index];
        }
        Header->MaxItems = ItemsCount;
    }

    /* store result */
    *OutHeader = Header;

    return STATUS_SUCCESS;
}

/*
    @unimplemented
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
        ObjectHeader->ItemCount = ItemsCount;
        ObjectHeader->CreateItem = ItemsList;
    }

    /* store the object in the file object */
    ASSERT(IoStack->FileObject->FsContext == NULL);
    IoStack->FileObject->FsContext = ObjectHeader;

    /* the object header is for a audio filter */
    ASSERT(DeviceHeader->DeviceIndex < DeviceHeader->MaxItems);

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

    /* FIXME free create items */

    /* free object header */
    ExFreePoolWithTag(ObjectHeader, TAG_DEVICE_HEADER);

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
        ASSERT(Header->ItemList[Index].CreateItem);

        if (Header->ItemList[Index].CreateItem->Create == NULL)
        {
            FreeIndex = Index;
            break;
        }

        if (!wcsicmp(ObjectClass, Header->ItemList[Index].CreateItem->ObjectClass.Buffer))
        {
            /* the same object class already exists */
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }
    /* found a free index */
    if (FreeIndex == (ULONG)-1)
    {
        /* no empty space found */
        return STATUS_ALLOTTED_SPACE_EXCEEDED;
    }

    /* initialize create item */
    Header->ItemList[FreeIndex].CreateItem->Create = Create;
    Header->ItemList[FreeIndex].CreateItem->Context = Context;
    RtlInitUnicodeString(&Header->ItemList[FreeIndex].CreateItem->ObjectClass, ObjectClass);
    Header->ItemList[FreeIndex].CreateItem->SecurityDescriptor = SecurityDescriptor;


    return STATUS_SUCCESS;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
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
KsAllocateObjectCreateItem(
    IN  KSDEVICE_HEADER DevHeader,
    IN  PKSOBJECT_CREATE_ITEM CreateItem,
    IN  BOOLEAN AllocateEntry,
    IN  PFNKSITEMFREECALLBACK ItemFreeCallback OPTIONAL)
{
    PKSIDEVICE_HEADER Header;
    PKSOBJECT_CREATE_ITEM Item;
    PDEVICE_ITEM ItemList;
    KIRQL OldLevel;

    Header = (PKSIDEVICE_HEADER)DevHeader;

    if (!DevHeader)
        return STATUS_INVALID_PARAMETER_1;

    if (!CreateItem)
        return STATUS_INVALID_PARAMETER_2;

    /* acquire list lock */
    KeAcquireSpinLock(&Header->ItemListLock, &OldLevel);

    ItemList = ExAllocatePool(NonPagedPool, sizeof(DEVICE_ITEM) * (Header->MaxItems + 1));
    if (!ItemList)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (AllocateEntry)
    {
        if (!ItemFreeCallback)
        {
            /* caller must be notified */
            ExFreePool(ItemList);
            /* release lock */
            KeReleaseSpinLock(&Header->ItemListLock, OldLevel);

            return STATUS_INVALID_PARAMETER_4;
        }
        /* allocate create item */
        Item = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
        if (!Item)
        {
            /* no memory */
            ExFreePool(ItemList);
            /* release lock */
            KeReleaseSpinLock(&Header->ItemListLock, OldLevel);

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
            ExFreePool(Item);
            ExFreePool(ItemList);

            /* release lock */
            KeReleaseSpinLock(&Header->ItemListLock, OldLevel);

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyUnicodeString(&Item->ObjectClass, &CreateItem->ObjectClass);
    }
    else
    {
        if (ItemFreeCallback)
        {
            /* callback is only accepted when the create item is copied */
            ExFreePool(ItemList);
            /* release lock */
            KeReleaseSpinLock(&Header->ItemListLock, OldLevel);

            return STATUS_INVALID_PARAMETER_4;
        }

        Item = CreateItem;
    }


    if (Header->MaxItems)
    {
        /* copy old create items */
        RtlMoveMemory(ItemList, Header->ItemList, sizeof(DEVICE_ITEM) * Header->MaxItems);
    }

    /* initialize item entry */
    ItemList[Header->MaxItems].CreateItem = Item;
    ItemList[Header->MaxItems].ItemFreeCallback = ItemFreeCallback;


    /* free old item list */
    ExFreePool(Header->ItemList);

    Header->ItemList = ItemList;
    Header->MaxItems++;

    /* release lock */
    KeReleaseSpinLock(&Header->ItemListLock, OldLevel);

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
        DPRINT("Handled %u Status %x Length %u\n", Status, IoStatusBlock.Status, IoStatusBlock.Information);
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

