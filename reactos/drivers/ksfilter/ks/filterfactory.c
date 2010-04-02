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
    KMUTEX ControlMutex;

}IKsFilterFactoryImpl;

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
    IKsFilterFactoryImpl * Factory;
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
    Factory = (IKsFilterFactoryImpl*)CONTAINING_RECORD(CreateItem->Context, IKsFilterFactoryImpl, FilterFactory);

    /* get interface */
    iface = (IKsFilterFactory*)&Factory->lpVtbl;

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

VOID
IKsFilterFactory_AttachFilterFactoryToDeviceHeader(
    IKsFilterFactoryImpl * This,
    PKSIDEVICE_HEADER DeviceHeader)
{
    PKSBASIC_HEADER BasicHeader;
    PKSFILTERFACTORY FilterFactory;

    if (DeviceHeader->BasicHeader.FirstChild.FilterFactory == NULL)
    {
        /* first attached filter factory */
        DeviceHeader->BasicHeader.FirstChild.FilterFactory = &This->FilterFactory;
        return;
    }

    /* set to first entry */
    FilterFactory = DeviceHeader->BasicHeader.FirstChild.FilterFactory;

    do
    {
        /* get basic header */
        BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)FilterFactory - sizeof(KSBASIC_HEADER));
        /* sanity check */
        ASSERT(BasicHeader->Type == KsObjectTypeFilterFactory);

        if (BasicHeader->Next.FilterFactory)
        {
            /* iterate to next filter factory */
            FilterFactory = BasicHeader->Next.FilterFactory;
        }
        else
        {
            /* found last entry */
            break;
        }
    }while(FilterFactory);

    /* attach filter factory */
    BasicHeader->Next.FilterFactory = &This->FilterFactory;
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
    IKsDevice * KsDevice;

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

    /* initialize filter factory control mutex */
    This->Header.ControlMutex = &This->ControlMutex;
    KeInitializeMutex(This->Header.ControlMutex, 0);

    /* unused fields */
    InitializeListHead(&This->Header.EventList);
    KeInitializeSpinLock(&This->Header.EventListLock);

    InitializeListHead(&This->SymbolicLinkList);

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

    DPRINT("IKsFilterFactory_fnInitialize CategoriesCount %u ReferenceString '%S'\n", Descriptor->CategoriesCount,ReferenceString.Buffer);

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

        /* create a object bag for the filter factory */
        This->FilterFactory.Bag = AllocateItem(NonPagedPool, sizeof(KSIOBJECT_BAG));
        if (This->FilterFactory.Bag)
        {
            /* initialize object bag */
            KsDevice = (IKsDevice*)&DeviceExtension->DeviceHeader->lpVtblIKsDevice;
            KsDevice->lpVtbl->InitializeObjectBag(KsDevice, (PKSIOBJECT_BAG)This->FilterFactory.Bag, NULL);
        }
    }

    /* attach filterfactory to device header */
    IKsFilterFactory_AttachFilterFactoryToDeviceHeader(This, DeviceExtension->DeviceHeader);

    /* return result */
    return Status;
}

static IKsFilterFactoryVtbl vt_IKsFilterFactoryVtbl =
{
    IKsFilterFactory_fnQueryInterface,
    IKsFilterFactory_fnAddRef,
    IKsFilterFactory_fnRelease,
    IKsFilterFactory_fnGetStruct,
    IKsFilterFactory_fnSetDeviceClassesState,
    IKsFilterFactory_fnInitialize
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

    DPRINT("KsCreateFilterFactory\n");

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
    DPRINT("KsCreateFilterFactory %x\n", Status);
    /* sanity check */
    ASSERT(Status == STATUS_SUCCESS);

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
    IKsFilterFactory * Factory;
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(FilterFactory, IKsFilterFactoryImpl, FilterFactory);

    Factory = (IKsFilterFactory*)&This->lpVtbl;
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
    DPRINT("KsFilterFactoryUpdateCacheData %p\n", FilterDescriptor);

    return STATUS_SUCCESS;
}	

