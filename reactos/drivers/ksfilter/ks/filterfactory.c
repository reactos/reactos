/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/filterfactory.c
 * PURPOSE:         KS IKsFilterFactory interface functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

typedef struct
{
    KSBASIC_HEADER Header;
    KSFILTERFACTORY FilterFactory;

    IKsFilterFactoryVtbl *lpVtbl;
    LONG ref;
    PKSIDEVICE_HEADER DeviceHeader;
    PFNKSFILTERFACTORYPOWER SleepCallback;
    PFNKSFILTERFACTORYPOWER WakeCallback;

    LIST_ENTRY SymbolicLinkList;
    LIST_ENTRY FilterInstanceList;
}IKsFilterFactoryImpl;

typedef struct
{
    LIST_ENTRY Entry;
    IKsFilter *FilterInstance;
}FILTER_INSTANCE_ENTRY, *PFILTER_INSTANCE_ENTRY;



VOID
NTAPI
IKsFilterFactory_ItemFreeCb(
    IN PKSOBJECT_CREATE_ITEM  CreateItem)
{
    /* callback when create item is freed in the device header */
    IKsFilterFactory * iface = (IKsFilterFactory*)CONTAINING_RECORD(CreateItem->Context, IKsFilterFactoryImpl, FilterFactory);

    iface->lpVtbl->Release(iface);
}

NTSTATUS
NTAPI
IKsFilterFactory_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKSOBJECT_CREATE_ITEM CreateItem;
    IKsFilterFactory * iface;
    NTSTATUS Status;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    if (!CreateItem)
    {
        DPRINT1("IKsFilterFactory_Create no CreateItem\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* get filter factory interface */
    iface = (IKsFilterFactory*)CONTAINING_RECORD(CreateItem->Context, IKsFilterFactoryImpl, FilterFactory);

    /* create a filter instance */
    Status = KspCreateFilter(DeviceObject, Irp, iface);

    DPRINT("KspCreateFilter Status %x\n", Status);

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}


NTSTATUS
NTAPI
IKsFilterFactory_fnQueryInterface(
    IKsFilterFactory * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, lpVtbl);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IKsFilterFactory_fnAddRef(
    IKsFilterFactory * iface)
{
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, lpVtbl);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsFilterFactory_fnRelease(
    IKsFilterFactory * iface)
{
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, lpVtbl);

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        if (!IsListEmpty(&This->SymbolicLinkList))
        {
            /* disable device interfaces */
            KspSetDeviceInterfacesState(&This->SymbolicLinkList, FALSE);
            /* free device interface strings */
            KspFreeDeviceInterfaces(&This->SymbolicLinkList);
        }

        FreeItem(This);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

KSFILTERFACTORY*
NTAPI
IKsFilterFactory_fnGetStruct(
    IKsFilterFactory * iface)
{
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, lpVtbl);

    return &This->FilterFactory;
}

NTSTATUS
NTAPI
IKsFilterFactory_fnSetDeviceClassesState(
    IKsFilterFactory * iface,
    IN BOOLEAN Enable)
{
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, lpVtbl);

    return KspSetDeviceInterfacesState(&This->SymbolicLinkList, Enable);
}

NTSTATUS
NTAPI
IKsFilterFactory_fnInitialize(
    IKsFilterFactory * iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN const KSFILTER_DESCRIPTOR  *Descriptor,
    IN PWSTR  RefString OPTIONAL,
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor OPTIONAL,
    IN ULONG  CreateItemFlags,
    IN PFNKSFILTERFACTORYPOWER  SleepCallback OPTIONAL,
    IN PFNKSFILTERFACTORYPOWER  WakeCallback OPTIONAL,
    OUT PKSFILTERFACTORY *FilterFactory OPTIONAL)
{
    UNICODE_STRING ReferenceString;
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension;
    KSOBJECT_CREATE_ITEM CreateItem;
    BOOL FreeString = FALSE;

    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, lpVtbl);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* initialize filterfactory */
    This->SleepCallback = SleepCallback;
    This->WakeCallback = WakeCallback;
    This->FilterFactory.FilterDescriptor = Descriptor;
    This->Header.KsDevice = &DeviceExtension->DeviceHeader->KsDevice;
    This->Header.Type = KsObjectTypeFilterFactory;
    This->Header.Parent.KsDevice = &DeviceExtension->DeviceHeader->KsDevice;
    This->DeviceHeader = DeviceExtension->DeviceHeader;

    InitializeListHead(&This->SymbolicLinkList);
    InitializeListHead(&This->FilterInstanceList);

    /* does the device use a reference string */
    if (RefString || !Descriptor->ReferenceGuid)
    {
        /* use device reference string */
        RtlInitUnicodeString(&ReferenceString, RefString);
    }
    else
    {
        /* create reference string from descriptor guid */
        Status = RtlStringFromGUID(Descriptor->ReferenceGuid, &ReferenceString);

        /* check for success */
        if (!NT_SUCCESS(Status))
        {
            /* omg, we failed */
            return Status;
        }

        FreeString = TRUE;
    }

    /* now register the device interface */
    Status = KspRegisterDeviceInterfaces(DeviceExtension->DeviceHeader->KsDevice.PhysicalDeviceObject,
                                         Descriptor->CategoriesCount,
                                         Descriptor->Categories,
                                         &ReferenceString,
                                         &This->SymbolicLinkList);
    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KspRegisterDeviceInterfaces failed with %x\n", Status);

        if (FreeString)
        {
            /* free unicode string */
            RtlFreeUnicodeString(&ReferenceString);
        }

        return Status;
    }

    /* now setup the create item */
    CreateItem.SecurityDescriptor = SecurityDescriptor;
    CreateItem.Flags = CreateItemFlags;
    CreateItem.Create = IKsFilterFactory_Create;
    CreateItem.Context = (PVOID)&This->FilterFactory;
    RtlInitUnicodeString(&CreateItem.ObjectClass, ReferenceString.Buffer);

    /* insert create item to device header */
    Status = KsAllocateObjectCreateItem((KSDEVICE_HEADER)DeviceExtension->DeviceHeader, &CreateItem, TRUE, IKsFilterFactory_ItemFreeCb);

    if (FreeString)
    {
        /* free unicode string */
        RtlFreeUnicodeString(&ReferenceString);
    }

    if (FilterFactory)
    {
        /* return filterfactory */
        *FilterFactory = &This->FilterFactory;

        /*FIXME create object bag */
    }


    /* return result */
    return Status;
}

NTSTATUS
NTAPI
IKsFilterFactory_fnAddFilterInstance(
    IKsFilterFactory * iface,
    IN IKsFilter *FilterInstance)
{
    PFILTER_INSTANCE_ENTRY Entry;
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, lpVtbl);

    /* allocate filter instance entry */
    Entry = AllocateItem(NonPagedPool, sizeof(FILTER_INSTANCE_ENTRY));
    if (!Entry)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize filter instance entry */
    Entry->FilterInstance = FilterInstance;

    /* insert entry */
    InsertTailList(&This->FilterInstanceList, &Entry->Entry);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsFilterFactory_fnRemoveFilterInstance(
    IKsFilterFactory * iface,
    IN IKsFilter *FilterInstance)
{
    PFILTER_INSTANCE_ENTRY InstanceEntry;
    PLIST_ENTRY Entry;
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, lpVtbl);

    /* point to first entry */
    Entry = This->FilterInstanceList.Flink;

    while(Entry != &This->FilterInstanceList)
    {
        InstanceEntry = (PFILTER_INSTANCE_ENTRY)CONTAINING_RECORD(Entry, FILTER_INSTANCE_ENTRY, Entry);
        if (InstanceEntry->FilterInstance == FilterInstance)
        {
            /* found entry */
            RemoveEntryList(&InstanceEntry->Entry);
            FreeItem(InstanceEntry);
            return STATUS_SUCCESS;
        }
    }

    /* entry not in list! */
    return STATUS_NOT_FOUND;
}

static IKsFilterFactoryVtbl vt_IKsFilterFactoryVtbl =
{
    IKsFilterFactory_fnQueryInterface,
    IKsFilterFactory_fnAddRef,
    IKsFilterFactory_fnRelease,
    IKsFilterFactory_fnGetStruct,
    IKsFilterFactory_fnSetDeviceClassesState,
    IKsFilterFactory_fnInitialize,
    IKsFilterFactory_fnAddFilterInstance,
    IKsFilterFactory_fnRemoveFilterInstance
};


NTSTATUS
NTAPI
KspCreateFilterFactory(
    IN PDEVICE_OBJECT  DeviceObject,
    IN const KSFILTER_DESCRIPTOR  *Descriptor,
    IN PWSTR  RefString OPTIONAL,
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor OPTIONAL,
    IN ULONG  CreateItemFlags,
    IN PFNKSFILTERFACTORYPOWER  SleepCallback OPTIONAL,
    IN PFNKSFILTERFACTORYPOWER  WakeCallback OPTIONAL,
    OUT PKSFILTERFACTORY *FilterFactory OPTIONAL)
{
    IKsFilterFactoryImpl * This;
    IKsFilterFactory * Filter;
    NTSTATUS Status;

    /* Lets allocate a filterfactory */
    This = AllocateItem(NonPagedPool, sizeof(IKsFilterFactoryImpl));
    if (!This)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize struct */
    This->ref = 1;
    This->lpVtbl = &vt_IKsFilterFactoryVtbl;

    /* map to com object */
    Filter = (IKsFilterFactory*)&This->lpVtbl;

    /* initialize filter */
    Status = Filter->lpVtbl->Initialize(Filter, DeviceObject, Descriptor, RefString, SecurityDescriptor, CreateItemFlags, SleepCallback, WakeCallback, FilterFactory);
    /* did we succeed */
    if (!NT_SUCCESS(Status))
    {
        /* destroy filterfactory */
        Filter->lpVtbl->Release(Filter);
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
KsCreateFilterFactory(
    IN PDEVICE_OBJECT  DeviceObject,
    IN const KSFILTER_DESCRIPTOR  *Descriptor,
    IN PWSTR  RefString OPTIONAL,
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor OPTIONAL,
    IN ULONG  CreateItemFlags,
    IN PFNKSFILTERFACTORYPOWER  SleepCallback OPTIONAL,
    IN PFNKSFILTERFACTORYPOWER  WakeCallback OPTIONAL,
    OUT PKSFILTERFACTORY  *FilterFactory OPTIONAL)
{
    return KspCreateFilterFactory(DeviceObject, Descriptor, RefString, SecurityDescriptor, CreateItemFlags, SleepCallback, WakeCallback, FilterFactory);

}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterFactorySetDeviceClassesState(
    IN PKSFILTERFACTORY  FilterFactory,
    IN BOOLEAN  NewState)
{
    IKsFilterFactory * Factory = (IKsFilterFactory*)CONTAINING_RECORD(FilterFactory, IKsFilterFactoryImpl, FilterFactory);

    return Factory->lpVtbl->SetDeviceClassesState(Factory, NewState);
}


/*
    @implemented
*/
KSDDKAPI
PUNICODE_STRING
NTAPI
KsFilterFactoryGetSymbolicLink(
    IN PKSFILTERFACTORY  FilterFactory)
{
    PSYMBOLIC_LINK_ENTRY LinkEntry;
    IKsFilterFactoryImpl * Factory = (IKsFilterFactoryImpl*)CONTAINING_RECORD(FilterFactory, IKsFilterFactoryImpl, FilterFactory);

    if (IsListEmpty(&Factory->SymbolicLinkList))
    {
        /* device has not registered any interfaces */
        return NULL;
    }

    /* get first entry */
    LinkEntry = (PSYMBOLIC_LINK_ENTRY)CONTAINING_RECORD(Factory->SymbolicLinkList.Flink, SYMBOLIC_LINK_ENTRY, Entry);

    /* return first link */
    return &LinkEntry->SymbolicLink;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterFactoryAddCreateItem(
    IN PKSFILTERFACTORY  FilterFactory,
    IN PWSTR  RefString,
    IN PSECURITY_DESCRIPTOR  SecurityDescriptor OPTIONAL,
    IN ULONG  CreateItemFlags)
{
    KSOBJECT_CREATE_ITEM CreateItem;

    IKsFilterFactoryImpl * Factory = (IKsFilterFactoryImpl*)CONTAINING_RECORD(FilterFactory, IKsFilterFactoryImpl, FilterFactory);

    /* initialize create item */
    CreateItem.Context = (PVOID)&Factory->FilterFactory;
    CreateItem.Create = IKsFilterFactory_Create;
    CreateItem.Flags = CreateItemFlags;
    CreateItem.SecurityDescriptor = SecurityDescriptor;
    RtlInitUnicodeString(&CreateItem.ObjectClass, RefString);

    /* insert create item to device header */
    return KsAllocateObjectCreateItem((KSDEVICE_HEADER)Factory->DeviceHeader, &CreateItem, TRUE, IKsFilterFactory_ItemFreeCb);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterFactoryUpdateCacheData (
    IN PKSFILTERFACTORY  FilterFactory,
    IN const KSFILTER_DESCRIPTOR*  FilterDescriptor OPTIONAL)
{
    UNIMPLEMENTED

    return STATUS_NOT_IMPLEMENTED;
}	

