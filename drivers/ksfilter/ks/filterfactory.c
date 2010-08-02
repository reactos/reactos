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
    iface = (IKsFilterFactory*)&Factory->Header.OuterUnknown;

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
    NTSTATUS Status;

    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, Header.OuterUnknown);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->Header.OuterUnknown;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    if (This->Header.ClientAggregate)
    {
         /* using client aggregate */
         Status = This->Header.ClientAggregate->lpVtbl->QueryInterface(This->Header.ClientAggregate, refiid, Output);

         if (NT_SUCCESS(Status))
         {
             /* client aggregate supports interface */
             return Status;
         }
    }

    DPRINT("IKsFilterFactory_fnQueryInterface no interface\n");
    return STATUS_NOT_SUPPORTED;
}

ULONG
NTAPI
IKsFilterFactory_fnAddRef(
    IKsFilterFactory * iface)
{
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, Header.OuterUnknown);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsFilterFactory_fnRelease(
    IKsFilterFactory * iface)
{
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, Header.OuterUnknown);

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
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, Header.OuterUnknown);

    return &This->FilterFactory;
}

NTSTATUS
NTAPI
IKsFilterFactory_fnSetDeviceClassesState(
    IKsFilterFactory * iface,
    IN BOOLEAN Enable)
{
    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, Header.OuterUnknown);

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

    IKsFilterFactoryImpl * This = (IKsFilterFactoryImpl*)CONTAINING_RECORD(iface, IKsFilterFactoryImpl, Header.OuterUnknown);

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
            KsDevice = (IKsDevice*)&DeviceExtension->DeviceHeader->BasicHeader.OuterUnknown;
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
    This->Header.OuterUnknown = (PUNKNOWN)&vt_IKsFilterFactoryVtbl;

    /* map to com object */
    Filter = (IKsFilterFactory*)&This->Header.OuterUnknown;

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

    Factory = (IKsFilterFactory*)&This->Header.OuterUnknown;
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

ULONG
KspCacheAddData(
    PKSPCACHE_DESCRIPTOR Descriptor,
    LPCVOID Data,
    ULONG Length)
{
    ULONG Index;

    for(Index = 0; Index < Descriptor->DataOffset; Index++)
    {
        if (RtlCompareMemory(Descriptor->DataCache, Data, Length) == Length)
        {
            if (Index + Length > Descriptor->DataOffset)
            {
                /* adjust used space */
                Descriptor->DataOffset = Index + Length;
                /* return absolute offset */
                return Descriptor->DataLength + Index;
            }
        }
    }

    /* sanity check */
    ASSERT(Descriptor->DataOffset + Length < Descriptor->DataLength);

    /* copy to data blob */
    RtlMoveMemory((Descriptor->DataCache + Descriptor->DataOffset), Data, Length);

    /* backup offset */
    Index = Descriptor->DataOffset;

    /* adjust used space */
    Descriptor->DataOffset += Length;

    /* return absolute offset */
    return Descriptor->DataLength + Index;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterFactoryUpdateCacheData(
    IN PKSFILTERFACTORY  FilterFactory,
    IN const KSFILTER_DESCRIPTOR*  FilterDescriptor OPTIONAL)
{
    KSPCACHE_DESCRIPTOR Descriptor;
    PKSPCACHE_FILTER_HEADER FilterHeader;
    UNICODE_STRING FilterData = RTL_CONSTANT_STRING(L"FilterData");
    PKSPCACHE_PIN_HEADER PinHeader;
    ULONG Index, SubIndex;
    PLIST_ENTRY Entry;
    PSYMBOLIC_LINK_ENTRY SymEntry;
    BOOLEAN Found;
    HKEY hKey;
    NTSTATUS Status = STATUS_SUCCESS;

    IKsFilterFactoryImpl * Factory = (IKsFilterFactoryImpl*)CONTAINING_RECORD(FilterFactory, IKsFilterFactoryImpl, FilterFactory);

    DPRINT("KsFilterFactoryUpdateCacheData %p\n", FilterDescriptor);

    if (!FilterDescriptor)
        FilterDescriptor = Factory->FilterFactory.FilterDescriptor;

    ASSERT(FilterDescriptor);

    /* initialize cache descriptor */
    RtlZeroMemory(&Descriptor, sizeof(KSPCACHE_DESCRIPTOR));

    /* calculate filter data size */
    Descriptor.FilterLength = sizeof(KSPCACHE_FILTER_HEADER);

    /* FIXME support variable size pin descriptors */
    ASSERT(FilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX));

    for(Index = 0; Index < FilterDescriptor->PinDescriptorsCount; Index++)
    {
        /* add filter descriptor */
        Descriptor.FilterLength += sizeof(KSPCACHE_PIN_HEADER);

        if (FilterDescriptor->PinDescriptors[Index].PinDescriptor.Category)
        {
            /* add extra ULONG for offset to category */
            Descriptor.FilterLength += sizeof(ULONG);

            /* add size for clsid */
            Descriptor.DataLength += sizeof(CLSID);
        }

        /* add space for formats */
        Descriptor.FilterLength += FilterDescriptor->PinDescriptors[Index].PinDescriptor.DataRangesCount * sizeof(KSPCACHE_DATARANGE);

        /* add space for MajorFormat / MinorFormat */
        Descriptor.DataLength += FilterDescriptor->PinDescriptors[Index].PinDescriptor.DataRangesCount * sizeof(CLSID) * 2;

        /* add space for mediums */
        Descriptor.FilterLength += FilterDescriptor->PinDescriptors[Index].PinDescriptor.MediumsCount * sizeof(ULONG);

        /* add space for the data */
        Descriptor.DataLength += FilterDescriptor->PinDescriptors[Index].PinDescriptor.MediumsCount * sizeof(KSPCACHE_MEDIUM);
    }

    /* now allocate the space */
    Descriptor.FilterData = (PUCHAR)AllocateItem(NonPagedPool, Descriptor.DataLength + Descriptor.FilterLength);
    if (!Descriptor.FilterData)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize data cache */
    Descriptor.DataCache = (PUCHAR)((ULONG_PTR)Descriptor.FilterData + Descriptor.FilterLength);

    /* setup filter header */
    FilterHeader = (PKSPCACHE_FILTER_HEADER)Descriptor.FilterData;

    FilterHeader->dwVersion = 2;
    FilterHeader->dwMerit = MERIT_DO_NOT_USE;
    FilterHeader->dwUnused = 0;
    FilterHeader->dwPins = FilterDescriptor->PinDescriptorsCount;

    Descriptor.FilterOffset = sizeof(KSPCACHE_FILTER_HEADER);

    /* write pin headers */
    for(Index = 0; Index < FilterDescriptor->PinDescriptorsCount; Index++)
    {
        /* get offset to pin */
        PinHeader = (PKSPCACHE_PIN_HEADER)((ULONG_PTR)Descriptor.FilterData + Descriptor.FilterOffset);

        /* write pin header */
        PinHeader->Signature = 0x33697030 + Index;
        PinHeader->Flags = 0;
        PinHeader->Instances = FilterDescriptor->PinDescriptors[Index].InstancesPossible;
        if (PinHeader->Instances > 1)
            PinHeader->Flags |= REG_PINFLAG_B_MANY;


        PinHeader->MediaTypes = FilterDescriptor->PinDescriptors[Index].PinDescriptor.DataRangesCount;
        PinHeader->Mediums = FilterDescriptor->PinDescriptors[Index].PinDescriptor.MediumsCount;
        PinHeader->Category = (FilterDescriptor->PinDescriptors[Index].PinDescriptor.Category ? TRUE : FALSE);

        Descriptor.FilterOffset += sizeof(KSPCACHE_PIN_HEADER);

        if (PinHeader->Category)
        {
            /* get category offset */
            PULONG Category = (PULONG)(PinHeader + 1);

            /* write category offset */
            *Category = KspCacheAddData(&Descriptor, FilterDescriptor->PinDescriptors[Index].PinDescriptor.Category, sizeof(CLSID));

            /* adjust offset */
            Descriptor.FilterOffset += sizeof(ULONG);
        }

        /* add dataranges */
        for(SubIndex = 0; SubIndex < FilterDescriptor->PinDescriptors[Index].PinDescriptor.DataRangesCount; SubIndex++)
        {
            /* get datarange offset */
            PKSPCACHE_DATARANGE DataRange = (PKSPCACHE_DATARANGE)((ULONG_PTR)Descriptor.FilterData + Descriptor.FilterOffset);

            /* initialize data range */
            DataRange->Signature = 0x33797430 + SubIndex;
            DataRange->dwUnused = 0;
            DataRange->OffsetMajor = KspCacheAddData(&Descriptor, &FilterDescriptor->PinDescriptors[Index].PinDescriptor.DataRanges[SubIndex]->MajorFormat, sizeof(CLSID));
            DataRange->OffsetMinor = KspCacheAddData(&Descriptor, &FilterDescriptor->PinDescriptors[Index].PinDescriptor.DataRanges[SubIndex]->SubFormat, sizeof(CLSID));

            /* adjust offset */
            Descriptor.FilterOffset += sizeof(KSPCACHE_DATARANGE);
        }

        /* add mediums */
        for(SubIndex = 0; SubIndex < FilterDescriptor->PinDescriptors[Index].PinDescriptor.MediumsCount; SubIndex++)
        {
            KSPCACHE_MEDIUM Medium;
            PULONG MediumOffset;

            /* get pin medium offset */
            MediumOffset = (PULONG)((ULONG_PTR)Descriptor.FilterData + Descriptor.FilterOffset);

            /* copy medium guid */
            RtlMoveMemory(&Medium.Medium, &FilterDescriptor->PinDescriptors[Index].PinDescriptor.Mediums[SubIndex].Set, sizeof(GUID));
            Medium.dw1 = FilterDescriptor->PinDescriptors[Index].PinDescriptor.Mediums[SubIndex].Id; /* FIXME verify */
            Medium.dw2 = 0;

            *MediumOffset = KspCacheAddData(&Descriptor, &Medium, sizeof(KSPCACHE_MEDIUM));

            /* adjust offset */
            Descriptor.FilterOffset += sizeof(ULONG);
        }
    }

    /* sanity checks */
    ASSERT(Descriptor.FilterOffset == Descriptor.FilterLength);
    ASSERT(Descriptor.DataOffset <= Descriptor.DataLength);


    /* now go through all entries and update 'FilterData' key */
    for(Index = 0; Index < FilterDescriptor->CategoriesCount; Index++)
    {
        /* get first entry */
        Entry = Factory->SymbolicLinkList.Flink;

        /* set status to not found */
        Found = FALSE;
        /* loop list until the the current category is found */
        while(Entry != &Factory->SymbolicLinkList)
        {
            /* fetch symbolic link entry */
            SymEntry = (PSYMBOLIC_LINK_ENTRY)CONTAINING_RECORD(Entry, SYMBOLIC_LINK_ENTRY, Entry);

            if (IsEqualGUIDAligned(&SymEntry->DeviceInterfaceClass, &FilterDescriptor->Categories[Index]))
            {
                /* found category */
                Found = TRUE;
                break;
            }

            /* move to next entry */
            Entry = Entry->Flink;
        }

        if (!Found)
        {
            /* filter category is not present */
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        /* now open device interface */
        Status = IoOpenDeviceInterfaceRegistryKey(&SymEntry->SymbolicLink, KEY_WRITE, &hKey);
        if (!NT_SUCCESS(Status))
        {
            /* failed to open interface key */
            break;
        }

        /* update filterdata key */
        Status = ZwSetValueKey(hKey, &FilterData, 0, REG_BINARY, Descriptor.FilterData, Descriptor.FilterLength + Descriptor.DataOffset);

        /* close filterdata key */
        ZwClose(hKey);

        if (!NT_SUCCESS(Status))
        {
            /* failed to set key value */
            break;
        }
    }
    /* free filter data */
    FreeItem(Descriptor.FilterData);

    /* done */
    return Status;
}	

