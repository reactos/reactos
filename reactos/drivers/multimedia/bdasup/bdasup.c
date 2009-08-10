
#include "precomp.h"

BDA_GLOBAL g_Settings =
{
    0,
    0,
    {NULL, NULL}
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
BdaCheckChanges(IN PIRP  Irp)
{
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

    /* FIXME provide a default automation table
     * to handle requests which the driver doesnt implement
     */

    /* allocate filter instance */
    FilterInstance = AllocateItem(NonPagedPool, sizeof(BDA_FILTER_INSTANCE_ENTRY));
    if (!FilterInstance)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* create the filter factory */
    Status = KsCreateFilterFactory(pKSDevice->FunctionalDeviceObject, pFilterDescriptor, NULL, NULL, 0, NULL, NULL, &FilterFactory);

    /* check for success */
    if (NT_SUCCESS(Status))
    {

        /* add the item to filter object bag */
        Status = KsAddItemToObjectBag(FilterFactory->Bag, FilterInstance, FreeFilterInstance);
        if (!NT_SUCCESS(Status))
        {
            /* destroy filter instance */
            FreeItem(FilterInstance);
            KsDeleteFilterFactory(FilterFactory);
            return Status;
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
    }

    /* done */
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

    if (!pulPinId || !pKSFilter)
        return STATUS_INVALID_PARAMETER;


    /* FIXME provide a default automation table
     * to handle requests which the driver doesnt implement
     */

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
        return STATUS_UNSUCCESSFUL;
    }

    /* is pin factory still existing */
    if (InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorsCount <= ulPinType)
    {
        /* pin request is out of bounds */
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME custom pin descriptors */
    ASSERT(InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX));

    /* get pin descriptor */
    PinDescriptor = (PKSPIN_DESCRIPTOR_EX)&InstanceEntry->FilterTemplate->pFilterDescriptor->PinDescriptors[ulPinType];

    /* create the pin factory */
    Status = KsFilterCreatePinFactory(pKSFilter, PinDescriptor, &PinId);

    /* check for success */
    if (NT_SUCCESS(Status))
    {
        /* store result */
        *pulPinId = PinId;
    }

    DPRINT("BdaCreatePin Result %x\n", Status);
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
    @unimplemented
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
    ASSERT(InstanceEntry->FilterTemplate == pBdaFilterTemplate);

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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyGetPinControl(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeDescriptors(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeEvents(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeMethods(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeProperties(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT GUID *pguidProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyNodeTypes(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
BdaPropertyPinTypes(
    IN PIRP Irp,
    IN KSPROPERTY *pKSProperty,
    OUT ULONG *pulProperty)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
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
    /* check for valid parameter */
    if (Irp && KSProperty)
        return STATUS_SUCCESS;

    return STATUS_INVALID_PARAMETER;
}
