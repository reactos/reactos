
#include "precomp.h"

const GUID KSPROPSETID_BdaPinControl = {0xded49d5, 0xa8b7, 0x4d5d, {0x97, 0xa1, 0x12, 0xb0, 0xc1, 0x95, 0x87, 0x4d}};
const GUID KSMETHODSETID_BdaDeviceConfiguration = {0x71985f45, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID KSPROPSETID_BdaTopology = {0xa14ee835, 0x0a23, 0x11d3, {0x9c, 0xc7, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};

BDA_GLOBAL g_Settings =
{
    0,
    0,
    {NULL, NULL}
};

KSPROPERTY_ITEM FilterPropertyItem[] =
{
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_TYPES(BdaPropertyNodeTypes, NULL),
    DEFINE_KSPROPERTY_ITEM_BDA_PIN_TYPES( BdaPropertyPinTypes, NULL),
    DEFINE_KSPROPERTY_ITEM_BDA_TEMPLATE_CONNECTIONS(BdaPropertyTemplateConnections, NULL),
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_METHODS(BdaPropertyNodeMethods, NULL),
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_PROPERTIES(BdaPropertyNodeProperties, NULL),
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_EVENTS(BdaPropertyNodeEvents, NULL),
    DEFINE_KSPROPERTY_ITEM_BDA_CONTROLLING_PIN_ID(BdaPropertyGetControllingPinId, NULL),
    DEFINE_KSPROPERTY_ITEM_BDA_NODE_DESCRIPTORS(BdaPropertyNodeDescriptors, NULL)
};


KSPROPERTY_SET FilterPropertySet =
{
    &KSPROPSETID_BdaTopology,
    8,
    FilterPropertyItem,
    0,
    NULL
};

KSMETHOD_ITEM FilterMethodItem[] =
{
    //DEFINE_KSMETHOD_ITEM_BDA_CREATE_PIN_FACTORY(BdaMethodCreatePin, NULL),
    DEFINE_KSMETHOD_ITEM_BDA_CREATE_TOPOLOGY(BdaMethodCreateTopology, NULL)
};

KSMETHOD_SET FilterMethodSet =
{
    &KSMETHODSETID_BdaDeviceConfiguration,
    2,
    FilterMethodItem,
    0,
    NULL
};

KSAUTOMATION_TABLE FilterAutomationTable =
{
    1,
    sizeof(KSPROPERTY_ITEM),
    &FilterPropertySet,
    1,
    sizeof(KSMETHOD_ITEM),
    &FilterMethodSet,
    0,
    sizeof(KSEVENT_ITEM),
    NULL
};

KSPROPERTY_ITEM PinPropertyItem[] =
{
    DEFINE_KSPROPERTY_ITEM_BDA_PIN_ID(BdaPropertyGetPinControl, NULL),
    DEFINE_KSPROPERTY_ITEM_BDA_PIN_TYPE(BdaPropertyGetPinControl, NULL)
};

KSPROPERTY_SET PinPropertySet =
{
    &KSPROPSETID_BdaPinControl,
    2,
    PinPropertyItem,
    0,
    NULL
};

KSAUTOMATION_TABLE PinAutomationTable =
{
    1,
    sizeof(KSPROPERTY_ITEM),
    &PinPropertySet,
    0,
    sizeof(KSMETHOD_ITEM),
    NULL,
    0,
    sizeof(KSEVENT_ITEM),
    NULL
};


PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    PVOID Item = ExAllocatePool(PoolType, NumberOfBytes);
    if (!Item)
        return Item;

    RtlZeroMemory(Item, NumberOfBytes);
    return Item;
}

VOID
FreeItem(
    IN PVOID Item)
{
    ExFreePool(Item);
}


PBDA_FILTER_INSTANCE_ENTRY
GetFilterInstanceEntry(
    IN PKSFILTERFACTORY FilterFactory)
{
    PBDA_FILTER_INSTANCE_ENTRY InstanceEntry = NULL;
    PLIST_ENTRY Entry;
    KIRQL OldLevel;

    /* acquire list lock */
    KeAcquireSpinLock(&g_Settings.FilterFactoryInstanceListLock, &OldLevel);

    /* point to first entry */
    Entry = g_Settings.FilterFactoryInstanceList.Flink;

    while(Entry != &g_Settings.FilterFactoryInstanceList)
    {
        /* get instance entry from list entry offset */
        InstanceEntry = (PBDA_FILTER_INSTANCE_ENTRY)CONTAINING_RECORD(Entry, BDA_FILTER_INSTANCE_ENTRY, Entry);

        /* is the instance entry the requested one */
        if (InstanceEntry->FilterFactoryInstance == FilterFactory)
            break;

        /* move to next entry */
        Entry = Entry->Flink;
        /* set to null as it has not been found */
        InstanceEntry = NULL;
    }


    /* release spin lock */
    KeReleaseSpinLock(&g_Settings.FilterFactoryInstanceListLock, OldLevel);

    /* return result */
    return InstanceEntry;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
DllInitialize(
    PUNICODE_STRING  RegistryPath)
{
    DPRINT("BDASUP::DllInitialize\n");

    KeInitializeSpinLock(&g_Settings.FilterFactoryInstanceListLock);
    InitializeListHead(&g_Settings.FilterFactoryInstanceList);
    g_Settings.Initialized = TRUE;

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaCheckChanges(IN PIRP  Irp)
{
    DPRINT("BdaCheckChanges\n");

    if (!Irp)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaCommitChanges(IN PIRP  Irp)
{
    DPRINT("BdaCommitChanges\n");

    if (!Irp)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaCreateFilterFactory(
    IN PKSDEVICE  pKSDevice,
    IN const KSFILTER_DESCRIPTOR *pFilterDescriptor,
    IN const BDA_FILTER_TEMPLATE *pBdaFilterTemplate)
{
    return BdaCreateFilterFactoryEx(pKSDevice, pFilterDescriptor, pBdaFilterTemplate, NULL);
}

VOID
NTAPI
FreeFilterInstance(
    IN PVOID Context)
{
    PBDA_FILTER_INSTANCE_ENTRY InstanceEntry = NULL;
    PLIST_ENTRY Entry;
    KIRQL OldLevel;

    /* acquire list lock */
    KeAcquireSpinLock(&g_Settings.FilterFactoryInstanceListLock, &OldLevel);

    /* point to first entry */
    Entry = g_Settings.FilterFactoryInstanceList.Flink;

    while(Entry != &g_Settings.FilterFactoryInstanceList)
    {
        /* get instance entry from list entry offset */
        InstanceEntry = (PBDA_FILTER_INSTANCE_ENTRY)CONTAINING_RECORD(Entry, BDA_FILTER_INSTANCE_ENTRY, Entry);

        /* is the instance entry the requested one */
        if (InstanceEntry == (PBDA_FILTER_INSTANCE_ENTRY)Context)
        {
            RemoveEntryList(&InstanceEntry->Entry);
            FreeItem(InstanceEntry);
            break;
        }

        /* move to next entry */
        Entry = Entry->Flink;
    }

    /* release spin lock */
    KeReleaseSpinLock(&g_Settings.FilterFactoryInstanceListLock, OldLevel);
}


/*
    @implemented
*/
NTSTATUS
NTAPI
BdaCreateFilterFactoryEx(
    IN  PKSDEVICE pKSDevice,
    IN  const KSFILTER_DESCRIPTOR *pFilterDescriptor,
    IN  const BDA_FILTER_TEMPLATE *BdaFilterTemplate,
    OUT PKSFILTERFACTORY  *ppKSFilterFactory)
{
    PKSFILTERFACTORY FilterFactory;
    PBDA_FILTER_INSTANCE_ENTRY FilterInstance;
    KIRQL OldLevel;
    NTSTATUS Status;
    PKSFILTER_DESCRIPTOR FilterDescriptor;

    DPRINT("BdaCreateFilterFactoryEx\n");

    FilterDescriptor = AllocateItem(NonPagedPool, sizeof(KSFILTER_DESCRIPTOR));
    if (!FilterDescriptor)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* copy filter descriptor template */
    RtlMoveMemory(FilterDescriptor, pFilterDescriptor, sizeof(KSFILTER_DESCRIPTOR));

    /* erase pin / nodes / connections from filter descriptor */
    FilterDescriptor->PinDescriptorsCount = 0;
    FilterDescriptor->PinDescriptors = NULL;
    FilterDescriptor->NodeDescriptorsCount = 0;
    FilterDescriptor->NodeDescriptors = NULL;
    FilterDescriptor->ConnectionsCount = 0;
    FilterDescriptor->Connections = NULL;

    /* merge the automation tables */
    Status = KsMergeAutomationTables((PKSAUTOMATION_TABLE*)&FilterDescriptor->AutomationTable, (PKSAUTOMATION_TABLE)pFilterDescriptor->AutomationTable, &FilterAutomationTable, NULL);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsMergeAutomationTables failed with %lx\n", Status);
        FreeItem(FilterDescriptor);
        return Status;
    }

    /* allocate filter instance */
    FilterInstance = AllocateItem(NonPagedPool, sizeof(BDA_FILTER_INSTANCE_ENTRY));
    if (!FilterInstance)
    {
        /* not enough memory */
        FreeItem(FilterDescriptor);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* create the filter factory */
    Status = KsCreateFilterFactory(pKSDevice->FunctionalDeviceObject, FilterDescriptor, NULL, NULL, 0, NULL, NULL, &FilterFactory);

    /* check for success */
    if (NT_SUCCESS(Status))
    {
        if (FilterDescriptor->AutomationTable != &FilterAutomationTable)
        {
            /* add the item to filter object bag */
            KsAddItemToObjectBag(FilterFactory->Bag, (PVOID)FilterDescriptor->AutomationTable, FreeFilterInstance);
        }
        else
        {
            /* make sure the automation table is not-read only */
            Status = _KsEdit(FilterFactory->Bag, (PVOID*)&FilterDescriptor->AutomationTable, sizeof(KSAUTOMATION_TABLE), sizeof(KSAUTOMATION_TABLE), 0);

            /* sanity check */
            ASSERT(Status == STATUS_SUCCESS);

            /* add to object bag */
            KsAddItemToObjectBag(FilterFactory->Bag, (PVOID)FilterDescriptor->AutomationTable, FreeFilterInstance);
        }

        /* initialize filter instance entry */
        FilterInstance->FilterFactoryInstance = FilterFactory;
        FilterInstance->FilterTemplate = (BDA_FILTER_TEMPLATE *)BdaFilterTemplate;

        /* acquire list lock */
        KeAcquireSpinLock(&g_Settings.FilterFactoryInstanceListLock, &OldLevel);

        /* insert factory at the end */
        InsertTailList(&g_Settings.FilterFactoryInstanceList, &FilterInstance->Entry);

        /* release spin lock */
        KeReleaseSpinLock(&g_Settings.FilterFactoryInstanceListLock, OldLevel);


        if (ppKSFilterFactory)
        {
            /* store result */
            *ppKSFilterFactory = FilterFactory;
        }
    }
    else
    {
        /* failed to create filter factory */
        FreeItem(FilterInstance);
        FreeItem(FilterDescriptor);
    }

    /* done */
    DPRINT("BdaCreateFilterFactoryEx Status %x\n", Status);
    return Status;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaCreatePin(
    IN PKSFILTER pKSFilter,
    IN ULONG ulPinType,
    OUT ULONG *pulPinId)
{
    PKSPIN_DESCRIPTOR_EX PinDescriptor;
    PKSFILTERFACTORY FilterFactory;
    PBDA_FILTER_INSTANCE_ENTRY InstanceEntry;
    NTSTATUS Status;
    ULONG PinId;
    PKSPIN_DESCRIPTOR_EX NewPinDescriptor;

    DPRINT("BdaCreatePin\n");

    if (!pulPinId || !pKSFilter)
        return STATUS_INVALID_PARAMETER;

    /* get parent filter factory */
    FilterFactory = KsFilterGetParentFilterFactory(pKSFilter);

    /* sanity check */
    ASSERT(FilterFactory);

    /* find instance entry */
    InstanceEntry = GetFilterInstanceEntry(FilterFactory);

    if (!InstanceEntry)
    {
        /* the filter was not initialized with BDA */
        return STATUS_NOT_FOUND;
    }

    /* sanity checks */
    ASSERT(InstanceEntry->FilterTemplate);
    ASSERT(InstanceEntry->FilterTemplate->pFilterDescriptor);

    /* does the filter support any pins */
    if (!InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount)
    {
        /* no pins supported */
        DPRINT("BdaCreatePin NoPins supported\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* is pin factory still existing */
    if (InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount <= ulPinType)
    {
        /* pin request is out of bounds */
        DPRINT("BdaCreatePin ulPinType %lu >= PinDescriptorCount %lu\n", ulPinType, InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount);
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME custom pin descriptors */
    ASSERT(InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX));

    /* get pin descriptor */
    PinDescriptor = (PKSPIN_DESCRIPTOR_EX)&InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptors[ulPinType];

    /* allocate pin descriptor */
    NewPinDescriptor = AllocateItem(NonPagedPool, sizeof(KSPIN_DESCRIPTOR_EX));
    if (!NewPinDescriptor)
    {
        /* no memory */
        DPRINT("BdaCreatePin OutOfMemory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

   /* make a copy of the pin descriptor */
   RtlMoveMemory(NewPinDescriptor, PinDescriptor, sizeof(KSPIN_DESCRIPTOR_EX));

    /* merge the automation tables */
    Status = KsMergeAutomationTables((PKSAUTOMATION_TABLE*)&NewPinDescriptor->AutomationTable, (PKSAUTOMATION_TABLE)PinDescriptor->AutomationTable, &PinAutomationTable, pKSFilter->Bag);

    /* check for success */
    if (NT_SUCCESS(Status))
    {
        /* create the pin factory */
        Status = KsFilterCreatePinFactory(pKSFilter, NewPinDescriptor, &PinId);

        /* check for success */
        if (NT_SUCCESS(Status))
        {
            /* store result */
            *pulPinId = PinId;
        }
    }


    DPRINT("BdaCreatePin Result %x PinId %u\n", Status, PinId);
    return Status;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaMethodCreatePin(
    IN PIRP Irp,
    IN KSMETHOD *pKSMethod,
    OUT ULONG *pulPinFactoryID)
{
    PKSM_PIN Pin;
    PKSFILTER Filter;

    DPRINT("BdaMethodCreatePin\n");

    if (!Irp)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* get filter from irp */
    Filter = KsGetFilterFromIrp(Irp);

    /* sanity check */
    ASSERT(Filter);
    ASSERT(pKSMethod);

    /* get method request */
    Pin = (PKSM_PIN)pKSMethod;

    /* create the pin */
    return BdaCreatePin(Filter, Pin->PinId, pulPinFactoryID);
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaInitFilter(
    IN PKSFILTER pKSFilter,
    IN const BDA_FILTER_TEMPLATE *pBdaFilterTemplate)
{
    PBDA_FILTER_INSTANCE_ENTRY InstanceEntry;
    PKSFILTERFACTORY FilterFactory;
    ULONG Index, PinId;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("BdaInitFilter %p\n", pBdaFilterTemplate);

    /* check input parameters */
    if (!pKSFilter)
        return STATUS_INVALID_PARAMETER;

    /* get parent filter factory */
    FilterFactory = KsFilterGetParentFilterFactory(pKSFilter);

    /* sanity check */
    ASSERT(FilterFactory);

    /* find instance entry */
    InstanceEntry = GetFilterInstanceEntry(FilterFactory);

    /* sanity check */
    ASSERT(InstanceEntry);

    if (!pBdaFilterTemplate)
    {
        /* use template from BdaCreateFilterFactoryEx */
        pBdaFilterTemplate = InstanceEntry->FilterTemplate;
    }

    /* now create the pins */
    for(Index = 0; Index < pBdaFilterTemplate->pFilterDescriptor->PinDescriptorsCount; Index++)
    {
        /* create the pin */
        Status = BdaCreatePin(pKSFilter, Index, &PinId);

        /* check for success */
        if (!NT_SUCCESS(Status))
            break;
    }

    /* done */
    return Status;
}



/*
    @implemented
*/
NTSTATUS
NTAPI
BdaCreateTopology(
    IN PKSFILTER pKSFilter,
    IN ULONG InputPinId,
    IN ULONG OutputPinId)
{
    PBDA_FILTER_INSTANCE_ENTRY InstanceEntry;
    PKSFILTERFACTORY FilterFactory;
    KSTOPOLOGY_CONNECTION Connection;

    DPRINT("BdaCreateTopology\n");

    /* check input parameters */
    if (!pKSFilter)
        return STATUS_INVALID_PARAMETER;

    /* get parent filter factory */
    FilterFactory = KsFilterGetParentFilterFactory(pKSFilter);

    /* sanity check */
    ASSERT(FilterFactory);

    /* find instance entry */
    InstanceEntry = GetFilterInstanceEntry(FilterFactory);

    if (!InstanceEntry)
    {
        /* the filter was not initialized with BDA */
        return STATUS_NOT_FOUND;
    }

    if (InputPinId >= InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount ||
        OutputPinId >= InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount)
    {
        /* invalid pin id */
        return STATUS_INVALID_PARAMETER;
    }

    /* initialize topology connection */
    Connection.FromNode = KSFILTER_NODE;
    Connection.ToNode = KSFILTER_NODE;
    Connection.FromNodePin = InputPinId;
    Connection.ToNodePin = OutputPinId;

    /* add the connection */
    return KsFilterAddTopologyConnections(pKSFilter, 1, &Connection);
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaDeletePin(
    IN PKSFILTER pKSFilter,
    IN ULONG *pulPinId)
{
    UNIMPLEMENTED;
    DPRINT("BdaDeletePin\n");
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaFilterFactoryUpdateCacheData(
    IN PKSFILTERFACTORY FilterFactory,
    IN const KSFILTER_DESCRIPTOR *FilterDescriptor OPTIONAL)
{
    DPRINT("BdaFilterFactoryUpdateCacheData\n");
    return KsFilterFactoryUpdateCacheData(FilterFactory, FilterDescriptor);
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaGetChangeState(
    IN PIRP Irp,
    OUT BDA_CHANGE_STATE *ChangeState)
{
    DPRINT("BdaGetChangeState\n");

    if (Irp && ChangeState)
    {
        *ChangeState = BDA_CHANGES_COMPLETE;
        return STATUS_SUCCESS;
    }

    /* invalid parameters supplied */
    return STATUS_INVALID_PARAMETER;

}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaMethodCreateTopology(
    IN PIRP Irp,
    IN KSMETHOD *pKSMethod,
    OPTIONAL PVOID pvIgnored)
{
    PKSFILTER Filter;
    PKSP_BDA_NODE_PIN Node;

    DPRINT("BdaMethodCreateTopology\n");

    /* check input parameters */
    if (!Irp || !pKSMethod)
        return STATUS_INVALID_PARAMETER;

    /* get filter */
    Filter = KsGetFilterFromIrp(Irp);

    /* sanity check */
    ASSERT(Filter);

    /* get method request */
    Node = (PKSP_BDA_NODE_PIN)pKSMethod;

    /* create the topology */
    return BdaCreateTopology(Filter, Node->ulInputPinId, Node->ulOutputPinId);
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaMethodDeletePin(
    IN PIRP Irp,
    IN KSMETHOD *pKSMethod,
    OPTIONAL PVOID pvIgnored)
{
    DPRINT("BdaMethodDeletePin\n");

    if (!Irp)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyGetControllingPinId(
    IN PIRP Irp,
    IN KSP_BDA_NODE_PIN *pProperty,
    OUT ULONG *pulControllingPinId)
{
    UNIMPLEMENTED;
    DPRINT("BdaPropertyGetControllingPinId\n");
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaPropertyGetPinControl(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    PKSPIN Pin;
    PKSFILTER Filter;
    PKSFILTERFACTORY FilterFactory;
    PBDA_FILTER_INSTANCE_ENTRY InstanceEntry;

    DPRINT("BdaPropertyGetPinControl\n");

    /* first get the pin */
    Pin = KsGetPinFromIrp(Irp);
    ASSERT(Pin);

    /* now get the parent filter */
    Filter = KsPinGetParentFilter(Pin);
    ASSERT(Filter);

    /* get parent filter factory */
    FilterFactory = KsFilterGetParentFilterFactory(Filter);
    ASSERT(FilterFactory);

    /* find instance entry */
    InstanceEntry = GetFilterInstanceEntry(FilterFactory);
    ASSERT(InstanceEntry);

    /* sanity check */
    pKSProperty++;
    ASSERT(pKSProperty->Id == KSPROPERTY_BDA_PIN_TYPE);

    /* store pin id */
    *pulProperty = Pin->Id;

    return STATUS_SUCCESS;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeDescriptors(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT BDANODE_DESCRIPTOR *pNodeDescriptorProperty)
{
    UNIMPLEMENTED;
    DPRINT("BdaPropertyNodeDescriptors\n");
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeEvents(
    IN PIRP Irp,
    IN KSP_NODE *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED;
    DPRINT("BdaPropertyNodeEvents\n");
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeMethods(
    IN PIRP Irp,
    IN KSP_NODE *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED;
    DPRINT("BdaPropertyNodeMethods\n");
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeProperties(
    IN PIRP Irp,
    IN KSP_NODE *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED;
    DPRINT("BdaPropertyNodeProperties\n");
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeTypes(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    PBDA_FILTER_INSTANCE_ENTRY InstanceEntry;
    PKSFILTERFACTORY FilterFactory;
    PKSFILTER pKSFilter;
    PIO_STACK_LOCATION IoStack;
    ULONG Index;

    DPRINT("BdaPropertyNodeTypes\n");

    /* check input parameter */
    if (!Irp || !pKSProperty)
        return STATUS_INVALID_PARAMETER;

    /* first get the filter */
    pKSFilter = KsGetFilterFromIrp(Irp);

    /* sanity check */
    ASSERT(pKSFilter);

    /* get parent filter factory */
    FilterFactory = KsFilterGetParentFilterFactory(pKSFilter);

    /* sanity check */
    ASSERT(FilterFactory);

    /* find instance entry */
    InstanceEntry = GetFilterInstanceEntry(FilterFactory);
    ASSERT(InstanceEntry);

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* are there node types provided */
    if (!pulProperty)
    {
        /* no node entry array provided */
        Irp->IoStatus.Information = InstanceEntry->FilterTemplate->pFilterDescriptor->NodeDescriptorsCount * sizeof(ULONG);
        Irp->IoStatus.Status = STATUS_MORE_ENTRIES;
        return STATUS_MORE_ENTRIES;
    }

    if (InstanceEntry->FilterTemplate->pFilterDescriptor->NodeDescriptorsCount * sizeof(ULONG) > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
    {
        /* buffer too small */
        Irp->IoStatus.Information = InstanceEntry->FilterTemplate->pFilterDescriptor->NodeDescriptorsCount * sizeof(ULONG);
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* now copy all descriptors */
    for(Index = 0; Index < InstanceEntry->FilterTemplate->pFilterDescriptor->NodeDescriptorsCount; Index++)
    {
        /* use the index as the type */
        pulProperty[Index] = Index;
    }

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaPropertyPinTypes(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    PBDA_FILTER_INSTANCE_ENTRY InstanceEntry;
    PKSFILTERFACTORY FilterFactory;
    PKSFILTER pKSFilter;
    PIO_STACK_LOCATION IoStack;
    ULONG Index;

    DPRINT("BdaPropertyPinTypes\n");

    /* check input parameter */
    if (!Irp || !pKSProperty)
        return STATUS_INVALID_PARAMETER;

    /* first get the filter */
    pKSFilter = KsGetFilterFromIrp(Irp);

    /* sanity check */
    ASSERT(pKSFilter);

    /* get parent filter factory */
    FilterFactory = KsFilterGetParentFilterFactory(pKSFilter);

    /* sanity check */
    ASSERT(FilterFactory);

    /* find instance entry */
    InstanceEntry = GetFilterInstanceEntry(FilterFactory);
    ASSERT(InstanceEntry);

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* are there node types provided */
    if (!pKSProperty)
    {
        /* no node entry array provided */
        Irp->IoStatus.Information = InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount * sizeof(ULONG);
        Irp->IoStatus.Status = STATUS_MORE_ENTRIES;
        return STATUS_MORE_ENTRIES;
    }

    if (InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount * sizeof(ULONG) > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
    {
        /* buffer too small */
        Irp->IoStatus.Information = InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount * sizeof(ULONG);
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* now copy all descriptors */
    for(Index = 0; Index < InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount; Index++)
    {
        /* use the index as the type */
        pulProperty[Index] = Index;
    }

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaPropertyTemplateConnections(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT KSTOPOLOGY_CONNECTION *pConnectionProperty)
{
    PBDA_FILTER_INSTANCE_ENTRY FilterInstance;
    PKSFILTER Filter;
    PIO_STACK_LOCATION IoStack;
    ULONG Index;

    DPRINT("BdaPropertyTemplateConnections\n");

    /* validate parameters */
    if (!Irp || !pKSProperty)
        return STATUS_INVALID_PARAMETER;

    /* first get the filter */
    Filter = KsGetFilterFromIrp(Irp);

    /* sanity check */
    ASSERT(Filter);

    /* verify filter has been registered with BDA */
    FilterInstance = GetFilterInstanceEntry(KsFilterGetParentFilterFactory(Filter));

    if (!FilterInstance)
        return STATUS_INVALID_PARAMETER;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (!pConnectionProperty)
    {
        /* caller needs the size first */
        Irp->IoStatus.Information = FilterInstance->FilterTemplate->pFilterDescriptor->ConnectionsCount * sizeof(KSTOPOLOGY_CONNECTION);
        Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
        return STATUS_BUFFER_OVERFLOW;
    }

    /* sanity check */
    ASSERT(FilterInstance->FilterTemplate->pFilterDescriptor->ConnectionsCount * sizeof(KSTOPOLOGY_CONNECTION) <= IoStack->Parameters.DeviceIoControl.OutputBufferLength);

    for(Index = 0; Index < FilterInstance->FilterTemplate->pFilterDescriptor->ConnectionsCount; Index++)
    {
        /* sanity check */
        ASSERT(FilterInstance->FilterTemplate->pFilterDescriptor->Connections);

        /* copy connection */
        RtlMoveMemory(pConnectionProperty, &FilterInstance->FilterTemplate->pFilterDescriptor->Connections[Index], sizeof(KSTOPOLOGY_CONNECTION));
    }

    /* store result */
    Irp->IoStatus.Information = FilterInstance->FilterTemplate->pFilterDescriptor->ConnectionsCount * sizeof(KSTOPOLOGY_CONNECTION);
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* done */
    return STATUS_SUCCESS;

}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaStartChanges(IN PIRP Irp)
{
    DPRINT("BdaStartChanges\n");

    if (Irp)
        return STATUS_SUCCESS;
    else
        return STATUS_INVALID_PARAMETER;

}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaUninitFilter(IN PKSFILTER pKSFilter)
{
    DPRINT("BdaUninitFilter\n");
    return STATUS_SUCCESS;
}

/*
    @implemented
*/
NTSTATUS
NTAPI
BdaValidateNodeProperty(
    IN PIRP Irp,
    IN KSPROPERTY *KSProperty)
{
    DPRINT("BdaValidateNodeProperty\n");

    /* check for valid parameter */
    if (Irp && KSProperty)
        return STATUS_SUCCESS;

    return STATUS_INVALID_PARAMETER;
}
