/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/api.cpp
 * PURPOSE:         Port api functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
KsoDispatchCreateWithGenericFactory(
    LONG Unknown,
    PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

IIrpTarget *
NTAPI
KsoGetIrpTargetFromFileObject(
    PFILE_OBJECT FileObject)
{
    PC_ASSERT(FileObject);

    // IrpTarget is stored in FsContext
    return (IIrpTarget*)FileObject->FsContext;
}

IIrpTarget *
NTAPI
KsoGetIrpTargetFromIrp(
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // IIrpTarget is stored in Context member
    return (IIrpTarget*)IoStack->FileObject->FsContext;
}

NTSTATUS
NTAPI
PcHandleEnableEventWithTable(
    IN PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR Descriptor)
{
    // store descriptor
    KSEVENT_ITEM_IRP_STORAGE(Irp) = (PKSEVENT_ITEM)Descriptor;

    // FIXME seh probing
    return KsEnableEvent(Irp, Descriptor->EventSetCount, Descriptor->EventSet, NULL, KSEVENTS_NONE, NULL);
}

NTSTATUS
NTAPI
PcHandleDisableEventWithTable(
    IN PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR Descriptor)
{
    // store descriptor
    KSEVENT_ITEM_IRP_STORAGE(Irp) = (PKSEVENT_ITEM)Descriptor;

    // FIXME seh probing

    return KsDisableEvent(Irp, Descriptor->EventList, KSEVENTS_SPINLOCK, (PVOID)Descriptor->EventListLock);
}

NTSTATUS
NTAPI
PcHandlePropertyWithTable(
    IN PIRP Irp,
    IN ULONG PropertySetCount,
    IN PKSPROPERTY_SET PropertySet,
    IN PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor)
{
    PIO_STACK_LOCATION IoStack;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY))
    {
        // certainly an invalid request
        return STATUS_INVALID_PARAMETER;
    }

    // store device descriptor
    KSPROPERTY_ITEM_IRP_STORAGE(Irp) = (PKSPROPERTY_ITEM)SubDeviceDescriptor;

    // then try KsPropertyHandler
    return KsPropertyHandler(Irp, PropertySetCount, PropertySet);
}

VOID
NTAPI
PcAcquireFormatResources(
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    LONG Unknown4)
{
    UNIMPLEMENTED;
}

NTSTATUS
PcAddToEventTable(
    PVOID Ptr,
    LONG Unknown2,
    ULONG Length,
    LONG Unknown3,
    LONG Unknown4,
    LONG Unknown5,
    LONG Unknown6,
    LONG Unknown7)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PropertyItemDispatch(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PPCPROPERTY_REQUEST PropertyRequest;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    PKSPROPERTY Property;
    PPCNODE_DESCRIPTOR NodeDescriptor;
    PKSNODEPROPERTY NodeProperty;
    PKSPROPERTY_SET PropertySet;
    PPCPROPERTY_ITEM PropertyItem;
    PPCAUTOMATION_TABLE NodeAutomation;
    PIO_STACK_LOCATION IoStack;
    ULONG InstanceSize, ValueSize, Index;
    PVOID Instance;
    NTSTATUS Status;

    // allocate a property request
    PropertyRequest = (PPCPROPERTY_REQUEST)AllocateItem(NonPagedPool, sizeof(PCPROPERTY_REQUEST), TAG_PORTCLASS);
    if (!PropertyRequest)
        return STATUS_INSUFFICIENT_RESOURCES;

    // grab device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get input property request
    Property = (PKSPROPERTY)Request;

    // get property set
    PropertySet = (PKSPROPERTY_SET)KSPROPERTY_SET_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);

    // get instance / value size
    InstanceSize = IoStack->Parameters.DeviceIoControl.InputBufferLength;
    Instance = Request;
    ValueSize = IoStack->Parameters.DeviceIoControl.OutputBufferLength;

     // initialize property request
     PropertyRequest->MajorTarget = Descriptor->UnknownMiniport;
     PropertyRequest->MinorTarget = Descriptor->UnknownStream;
     PropertyRequest->Irp = Irp;
     PropertyRequest->Verb = Property->Flags;

    // check if this is filter / pin property request
    if (!(Property->Flags & KSPROPERTY_TYPE_TOPOLOGY))
    {
        // adjust input buffer size
        InstanceSize -= sizeof(KSPROPERTY);
        Instance = (PVOID)((ULONG_PTR)Instance + sizeof(KSPROPERTY));

        // filter / pin property request dont use node field
        PropertyRequest->Node = MAXULONG;
    }
    else
    {
        ASSERT(InstanceSize >= sizeof(KSNODEPROPERTY));
        // request is for a node
        InstanceSize -= sizeof(KSNODEPROPERTY);
        Instance = (PVOID)((ULONG_PTR)Instance + sizeof(KSNODEPROPERTY));

        // cast node property request
        NodeProperty = (PKSNODEPROPERTY)Request;

        // store node id
        PropertyRequest->Node = NodeProperty->NodeId;
    }

    // store instance size
    PropertyRequest->InstanceSize = InstanceSize;
    PropertyRequest->Instance = (InstanceSize != 0 ? Instance : NULL);

    // store value size
    PropertyRequest->ValueSize = ValueSize;
    PropertyRequest->Value = Data;

    // now scan the property set for the attached property set item stored in Relations member
    if (PropertySet && (!(Property->Flags & KSPROPERTY_TYPE_TOPOLOGY)))
    {
        // sanity check
        PC_ASSERT(IsEqualGUIDAligned(Property->Set, *PropertySet->Set));

        for(Index = 0; Index < PropertySet->PropertiesCount; Index++)
        {
            // check if they got the same property id
            if (PropertySet->PropertyItem[Index].PropertyId == Property->Id)
            {
                // found item
                PropertyRequest->PropertyItem = (const PCPROPERTY_ITEM*)PropertySet->PropertyItem[Index].Relations;

                // done
                break;
            }
        }
    }
    else
    {
        // is topology node id valid
        if (PropertyRequest->Node < Descriptor->DeviceDescriptor->NodeCount)
        {
            // get node descriptor
            NodeDescriptor = (PPCNODE_DESCRIPTOR)((ULONG_PTR)Descriptor->DeviceDescriptor->Nodes +
                                                  PropertyRequest->Node * Descriptor->DeviceDescriptor->NodeSize);

            // get node automation table
            NodeAutomation = (PPCAUTOMATION_TABLE)NodeDescriptor->AutomationTable;

            // has it got a automation table
            if (NodeAutomation)
            {
                // now scan the properties and check if it supports this request
                PropertyItem = (PPCPROPERTY_ITEM)NodeAutomation->Properties;
                for (Index = 0; Index < NodeAutomation->PropertyCount; Index++)
                {
                    // are they same property
                    if (IsEqualGUIDAligned(*PropertyItem->Set, Property->Set))
                    {
                        if (PropertyItem->Id == Property->Id)
                        {
                            // found match
                            PropertyRequest->PropertyItem = PropertyItem;
                            DPRINT("Using property item %p\n", PropertyItem);
                            // done
                            break;
                        }
                    }

                    // move to next property item
                    PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + NodeAutomation->PropertyItemSize);
                }
            }
        }
    }

    if (PropertyRequest->PropertyItem && PropertyRequest->PropertyItem->Handler)
    {
        // now call the handler
        UNICODE_STRING GuidBuffer;
        RtlStringFromGUID(Property->Set, &GuidBuffer);
        DPRINT("Calling Verb %x Node %lu MajorTarget %p MinorTarget %p PropertySet %S PropertyId %lu PropertyFlags %lx InstanceSize %lu ValueSize %lu Handler %p PropertyRequest %p PropertyItemFlags %lx PropertyItemId %lu\n",
                PropertyRequest->Verb,
                PropertyRequest->Node, PropertyRequest->MajorTarget, PropertyRequest->MinorTarget, GuidBuffer.Buffer, Property->Id, Property->Flags, PropertyRequest->InstanceSize, PropertyRequest->ValueSize,
                PropertyRequest->PropertyItem->Handler, PropertyRequest, PropertyRequest->PropertyItem->Flags, PropertyRequest->PropertyItem->Id);
        RtlFreeUnicodeString(&GuidBuffer);

        _SEH2_TRY
        {
            Status = PropertyRequest->PropertyItem->Handler(PropertyRequest);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Fail the IRP */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        DPRINT("Status %lx ValueSize %lu Information %lu\n", Status, PropertyRequest->ValueSize, Irp->IoStatus.Information);
        Irp->IoStatus.Information = PropertyRequest->ValueSize;

        if (Status != STATUS_PENDING)
        {
            // free property request
            FreeItem(PropertyRequest, TAG_PORTCLASS);
        }
    }
    else
    {
        FreeItem(PropertyRequest, TAG_PORTCLASS);
        Status = STATUS_NOT_FOUND;
    }

    /* done */
    return Status;
}

NTSTATUS
PcAddToPropertyTable(
    IN PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor,
    IN PPCPROPERTY_ITEM PropertyItem,
    IN ULONG bNode)
{
    ULONG bFound = FALSE;
    ULONG Index, PropertySetIndex, PropertySetItemIndex;
    PKSPROPERTY_SET NewPropertySet;
    PKSPROPERTY_ITEM FilterPropertyItem, NewFilterPropertyItem;
    LPGUID Guid;
    UNICODE_STRING GuidBuffer;

    ASSERT(PropertyItem->Set);
    RtlStringFromGUID(*PropertyItem->Set, &GuidBuffer);
    DPRINT("PcAddToPropertyTable Adding Item Set %S Id %lu Flags %lx\n", GuidBuffer.Buffer, PropertyItem->Id, PropertyItem->Flags);
    RtlFreeUnicodeString(&GuidBuffer);

    //DPRINT1("FilterPropertySetCount %lu\n", SubDeviceDescriptor->FilterPropertySetCount);
    // first step check if the property set is present already
    for(Index = 0; Index < SubDeviceDescriptor->FilterPropertySetCount; Index++)
    {

		//RtlStringFromGUID(*SubDeviceDescriptor->FilterPropertySet[Index].Set, &GuidBuffer);
        //DPRINT1("FilterProperty Set %S PropertyCount %lu\n", GuidBuffer.Buffer, SubDeviceDescriptor->FilterPropertySet[Index].PropertiesCount);
        if (IsEqualGUIDAligned(*SubDeviceDescriptor->FilterPropertySet[Index].Set, *PropertyItem->Set))
        {
            // property set is already present
            bFound = TRUE;
            PropertySetIndex = Index;

            // break out
            break;
        }
    }

    // is the property set present
    if (!bFound)
    {
        // need to allocate a property set
        NewPropertySet = (PKSPROPERTY_SET)AllocateItem(NonPagedPool, (SubDeviceDescriptor->FilterPropertySetCount + 1) * sizeof(KSPROPERTY_SET), TAG_PORTCLASS);
        if (!NewPropertySet)
        {
            // out of memory
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // need to allocate property set guid
        Guid = (LPGUID)AllocateItem(NonPagedPool, sizeof(GUID), TAG_PORTCLASS);
        if (!Guid)
        {
            // out of memory
            FreeItem(NewPropertySet, TAG_PORTCLASS);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // are any existing property sets
        if (SubDeviceDescriptor->FilterPropertySetCount)
        {
            // copy property sets
            RtlMoveMemory(NewPropertySet, SubDeviceDescriptor->FilterPropertySet, SubDeviceDescriptor->FilterPropertySetCount * sizeof(KSPROPERTY_SET));

            // release memory
            FreeItem(SubDeviceDescriptor->FilterPropertySet, TAG_PORTCLASS);
        }

        // store new property set descriptors
        SubDeviceDescriptor->FilterPropertySet = NewPropertySet;

        // store index
        PropertySetIndex = SubDeviceDescriptor->FilterPropertySetCount;

        // increment property set count
        SubDeviceDescriptor->FilterPropertySetCount++;

        // copy property guid
        RtlMoveMemory(Guid, PropertyItem->Set, sizeof(GUID));

        // initialize property set
        SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].Set = Guid;
        SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertiesCount = 0;
    }

    // as the property set has been identified, now search for duplicate property set item entries
    FilterPropertyItem = (PKSPROPERTY_ITEM)SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertyItem;
    bFound = FALSE;

    for(Index = 0; Index < SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertiesCount; Index++)
    {
        // now search for an equal property set item
        if (FilterPropertyItem->PropertyId == PropertyItem->Id)
        {
            // found existing property set item
            bFound = TRUE;
            PropertySetItemIndex = Index;
            break;
        }

        // move to next entry
        FilterPropertyItem++;
    }

    if (!bFound)
    {
        // need to allocate memory for new property set item
        NewFilterPropertyItem = (PKSPROPERTY_ITEM)AllocateItem(NonPagedPool, (SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertiesCount + 1) * sizeof(KSPROPERTY_ITEM), TAG_PORTCLASS);
        if (!NewFilterPropertyItem)
        {
            // out of memory
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // are any existing property set items
        if (SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertiesCount)
        {
            // copy property item sets
            RtlMoveMemory(NewFilterPropertyItem,
                          (PVOID)SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertyItem,
                          SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertiesCount * sizeof(KSPROPERTY_ITEM));

            // release old descriptors
            FreeItem((PVOID)SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertyItem, TAG_PORTCLASS);
        }

        // store new descriptor
        SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertyItem = NewFilterPropertyItem;

        // store index
        PropertySetItemIndex = SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertiesCount;

        // increment property item set count
        SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertiesCount++;

        // now initialize property item
        FilterPropertyItem = (PKSPROPERTY_ITEM)&SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertyItem[PropertySetItemIndex];
        FilterPropertyItem->PropertyId = PropertyItem->Id;
        FilterPropertyItem->MinProperty = sizeof(KSPROPERTY);
        FilterPropertyItem->MinData = 0;

        // are any set operations supported
        if (PropertyItem->Flags & PCPROPERTY_ITEM_FLAG_SET)
        {
            // setup handler
            FilterPropertyItem->SetPropertyHandler = PropertyItemDispatch;
        }

        // are get operations supported
        if (PropertyItem->Flags & PCPROPERTY_ITEM_FLAG_GET)
        {
            // setup handler
            FilterPropertyItem->GetPropertyHandler = PropertyItemDispatch;
        }

        // are basic support operations supported
        if (PropertyItem->Flags & PCPROPERTY_ITEM_FLAG_BASICSUPPORT)
        {
            // setup handler
            FilterPropertyItem->SupportHandler = PropertyItemDispatch;
        }

        if (!bNode)
        {
            // store property item in relations
            // only store property item of filter properties / pin properties
            // because filter & pin properties do not require a specific context
            // on the other hand node properties are specifically bound to a node

            FilterPropertyItem->Relations = (const KSPROPERTY*)PropertyItem;
        }
    }
    else
    {
        // property set item handler already present
        // now replace initialize property item
#if 0
        DPRINT1("Replacing existing handler\n");
        FilterPropertyItem = (PKSPROPERTY_ITEM)&SubDeviceDescriptor->FilterPropertySet[PropertySetIndex]
                                 .PropertyItem[PropertySetItemIndex];
        // are any set operations supported
        if (PropertyItem->Flags & PCPROPERTY_ITEM_FLAG_SET)
        {
            // setup handler
            FilterPropertyItem->SetPropertyHandler = PropertyItemDispatch;
        }

        // are get operations supported
        if (PropertyItem->Flags & PCPROPERTY_ITEM_FLAG_GET)
        {
            // setup handler
            FilterPropertyItem->GetPropertyHandler = PropertyItemDispatch;
        }

        // are basic support operations supported
        if (PropertyItem->Flags & PCPROPERTY_ITEM_FLAG_BASICSUPPORT)
        {
            // setup handler
            FilterPropertyItem->SupportHandler = PropertyItemDispatch;
        }

        if (bNode)
        {
            // filter & pin properties should not be exposed on a node
            ASSERT(SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertyItem[PropertySetItemIndex].Relations == NULL);
        }
        else
        {
            // node properties should not be exposed on a filter & pin
            ASSERT(SubDeviceDescriptor->FilterPropertySet[PropertySetIndex].PropertyItem[PropertySetItemIndex].Relations != NULL);
        }
#endif
    }

    // done
    return STATUS_SUCCESS;
}

NTSTATUS
PcCaptureFormat(
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    LONG Unknown4)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
DumpAutomationTable(
    IN PPCAUTOMATION_TABLE AutomationTable,
    IN LPCWSTR DebugPrefix,
    IN LPCWSTR DebugIndentation)
{
    PPCPROPERTY_ITEM PropertyItem;
    PPCEVENT_ITEM EventItem;
    PPCMETHOD_ITEM MethodItem;
    ULONG Index;
    UNICODE_STRING GuidString;

    if (!AutomationTable)
    {
        // no table
        return;
    }

    DPRINT("=====================================================================\n");
    DPRINT("%S%S AutomationTable %p\n", DebugIndentation, DebugPrefix, AutomationTable);
    DPRINT("%S%S PropertyCount %lu\n", DebugIndentation, DebugPrefix, AutomationTable->PropertyCount);
    DPRINT("%S%S EventCount %lu\n", DebugIndentation, DebugPrefix, AutomationTable->EventCount);
    DPRINT("%S%S MethodCount %lu\n", DebugIndentation, DebugPrefix, AutomationTable->MethodCount);

    // print properties
    if (AutomationTable->PropertyCount)
    {
        if (AutomationTable->PropertyItemSize >= sizeof(PCPROPERTY_ITEM))
        {
            // get property item
            PropertyItem = (PPCPROPERTY_ITEM)AutomationTable->Properties;

            // sanity check
            ASSERT(PropertyItem);

            // display all properties associated
            for(Index = 0; Index < AutomationTable->PropertyCount; Index++)
            {
                // convert to printable string
                RtlStringFromGUID(*PropertyItem->Set, &GuidString);
                DPRINT("%SPropertyItemIndex %lu %p GUID %S Id %u Flags %x\n", DebugIndentation, Index, PropertyItem, GuidString.Buffer, PropertyItem->Id, PropertyItem->Flags);
                RtlFreeUnicodeString(&GuidString);
                // move to next item
                PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + AutomationTable->PropertyItemSize);
            }

        }
        else
        {
            DPRINT1("DRIVER BUG: property item must be at least %lu but got %lu\n", sizeof(PCPROPERTY_ITEM), AutomationTable->PropertyItemSize);
        }
    }

    // print events
    if (AutomationTable->EventCount)
    {
        if (AutomationTable->EventItemSize >= sizeof(PCEVENT_ITEM))
        {
            // get first event item
            EventItem = (PPCEVENT_ITEM)AutomationTable->Events;

            // sanity check
            ASSERT(EventItem);

            for(Index = 0; Index < AutomationTable->EventCount; Index++)
            {
                // convert to printable string
                RtlStringFromGUID(*EventItem->Set, &GuidString);
                DPRINT("%SEventItemIndex %lu %p GUID %S Id %u Flags %x\n", DebugIndentation, Index, EventItem, GuidString.Buffer, EventItem->Id, EventItem->Flags);
                RtlFreeUnicodeString(&GuidString);

                // move to next item
                EventItem = (PPCEVENT_ITEM)((ULONG_PTR)EventItem + AutomationTable->EventItemSize);
            }
        }
        else
        {
            DPRINT1("DRIVER BUG: event item must be at least %lu but got %lu\n", sizeof(PCEVENT_ITEM), AutomationTable->EventItemSize);
        }
    }

    // print methods
    if (AutomationTable->MethodCount)
    {
       if (AutomationTable->MethodItemSize >= sizeof(PCMETHOD_ITEM))
       {
            // get first event item
            MethodItem = (PPCMETHOD_ITEM)AutomationTable->Methods;

            // sanity check
            ASSERT(MethodItem);

            for(Index = 0; Index < AutomationTable->MethodCount; Index++)
            {
                // convert to printable string
                RtlStringFromGUID(*MethodItem->Set, &GuidString);
                DPRINT("%SMethodItemIndex %lu %p GUID %S Id %u Flags %x\n", DebugIndentation, Index, MethodItem, GuidString.Buffer, MethodItem->Id, MethodItem->Flags);
                RtlFreeUnicodeString(&GuidString);

                // move to next item
                MethodItem = (PPCMETHOD_ITEM)((ULONG_PTR)MethodItem + AutomationTable->MethodItemSize);
            }

       }
       else
       {
           DPRINT1("DRIVER BUG: method item must be at least %lu but got %lu\n", sizeof(PCEVENT_ITEM), AutomationTable->MethodItemSize);
       }
    }
    DPRINT("=====================================================================\n");
}

VOID
DumpFilterDescriptor(
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    ULONG Index;
    WCHAR Buffer[30];
    PPCPIN_DESCRIPTOR PinDescriptor;
    PPCNODE_DESCRIPTOR NodeDescriptor;

    DPRINT("======================\n");
    DPRINT("Descriptor Automation Table %p\n",FilterDescription->AutomationTable);
    DPRINT("PinCount %lu PinSize %lu StandardSize %lu\n", FilterDescription->PinCount, FilterDescription->PinSize, sizeof(PCPIN_DESCRIPTOR));
    DPRINT("NodeCount %lu NodeSize %lu StandardSize %lu\n", FilterDescription->NodeCount, FilterDescription->NodeSize, sizeof(PCNODE_DESCRIPTOR));

    // dump filter description table
    DumpAutomationTable((PPCAUTOMATION_TABLE)FilterDescription->AutomationTable, L"Filter", L"");

    if (FilterDescription->PinCount)
    {
        if (FilterDescription->PinSize >= sizeof(PCPIN_DESCRIPTOR))
        {
            // get first pin
            PinDescriptor = (PPCPIN_DESCRIPTOR)FilterDescription->Pins;

            // sanity check
            ASSERT(PinDescriptor);

            for(Index = 0; Index < FilterDescription->PinCount; Index++)
            {
               // print prefix
               swprintf(Buffer, L"PinIndex %lu", Index);

               // dump automation table
               DumpAutomationTable((PPCAUTOMATION_TABLE)PinDescriptor->AutomationTable, Buffer, L"    ");

               // move to next pin descriptor
               PinDescriptor = (PPCPIN_DESCRIPTOR)((ULONG_PTR)PinDescriptor + FilterDescription->PinSize);
            }
        }
        else
        {
            DPRINT1("DRIVER BUG: pin size smaller than minimum size\n");
        }
    }

    if (FilterDescription->Nodes)
    {
        if (FilterDescription->NodeSize >= sizeof(PCNODE_DESCRIPTOR))
        {
            // get first descriptor
            NodeDescriptor = (PPCNODE_DESCRIPTOR)FilterDescription->Nodes;

            // sanity check
            ASSERT(NodeDescriptor);

            for(Index = 0; Index < FilterDescription->NodeCount; Index++)
            {
                // print prefix
                swprintf(Buffer, L"NodeIndex %lu", Index);

                // dump automation table
                DumpAutomationTable((PPCAUTOMATION_TABLE)NodeDescriptor->AutomationTable, Buffer, L"    ");

                // move to next node descriptor
                NodeDescriptor = (PPCNODE_DESCRIPTOR)((ULONG_PTR)NodeDescriptor + FilterDescription->NodeSize);
            }
        }
        else
        {
            DPRINT1("DRIVER BUG: node size smaller than standard descriptor size\n");
        }
    }

    DPRINT("ConnectionCount: %lu\n", FilterDescription->ConnectionCount);

    if (FilterDescription->ConnectionCount)
    {
        DPRINT("------ Start of Nodes Connections ----------------\n");
        for(Index = 0; Index < FilterDescription->ConnectionCount; Index++)
        {
            DPRINT1("Index %ld FromPin %ld FromNode %ld -> ToPin %ld ToNode %ld\n", Index,
                                                                                    FilterDescription->Connections[Index].FromNodePin,
                                                                                    FilterDescription->Connections[Index].FromNode,
                                                                                    FilterDescription->Connections[Index].ToNodePin,
                                                                                    FilterDescription->Connections[Index].ToNode);
        }
        DPRINT("------ End of Nodes Connections----------------\n");
    }
    DPRINT1("======================\n");
}

NTSTATUS
NTAPI
PcCreateSubdeviceDescriptor(
    OUT SUBDEVICE_DESCRIPTOR ** OutSubdeviceDescriptor,
    IN ULONG InterfaceCount,
    IN GUID * InterfaceGuids,
    IN ULONG IdentifierCount,
    IN KSIDENTIFIER *Identifier,
    IN ULONG FilterPropertiesCount,
    IN KSPROPERTY_SET * FilterProperties,
    IN ULONG Unknown1,
    IN ULONG Unknown2,
    IN ULONG PinPropertiesCount,
    IN KSPROPERTY_SET * PinProperties,
    IN ULONG EventSetCount,
    IN KSEVENT_SET * EventSet,
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    SUBDEVICE_DESCRIPTOR * Descriptor;
    ULONG Index, SubIndex;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PPCPIN_DESCRIPTOR SrcDescriptor;
    PPCNODE_DESCRIPTOR NodeDescriptor;
    PPCPROPERTY_ITEM PropertyItem;

    // allocate subdevice descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)AllocateItem(NonPagedPool, sizeof(SUBDEVICE_DESCRIPTOR), TAG_PORTCLASS);
    if (!Descriptor)
        return STATUS_INSUFFICIENT_RESOURCES;

    // initialize physical / symbolic link connection list
    InitializeListHead(&Descriptor->SymbolicLinkList);
    InitializeListHead(&Descriptor->PhysicalConnectionList);

    // add driver category guids
    Descriptor->Interfaces = (GUID*)AllocateItem(NonPagedPool, sizeof(GUID) * (InterfaceCount + FilterDescription->CategoryCount), TAG_PORTCLASS);
    if (!Descriptor->Interfaces)
        goto cleanup;

    // copy interface guids
    RtlCopyMemory(Descriptor->Interfaces, InterfaceGuids, sizeof(GUID) * InterfaceCount);
    RtlCopyMemory(
        &Descriptor->Interfaces[InterfaceCount], FilterDescription->Categories,
        sizeof(GUID) * FilterDescription->CategoryCount);

    Descriptor->InterfaceCount = InterfaceCount + FilterDescription->CategoryCount;

    DumpFilterDescriptor(FilterDescription);

    // are any property sets supported by the portcls
    if (FilterPropertiesCount)
    {
       // first allocate filter properties set
       Descriptor->FilterPropertySet = (PKSPROPERTY_SET)AllocateItem(NonPagedPool, sizeof(KSPROPERTY_SET) * FilterPropertiesCount, TAG_PORTCLASS);
       if (! Descriptor->FilterPropertySet)
           goto cleanup;

       // now copy all filter property sets
       Descriptor->FilterPropertySetCount = FilterPropertiesCount;
       for(Index = 0; Index < FilterPropertiesCount; Index++)
       {
           // copy property set
           RtlMoveMemory(&Descriptor->FilterPropertySet[Index], &FilterProperties[Index], sizeof(KSPROPERTY_SET));

           if (Descriptor->FilterPropertySet[Index].PropertiesCount)
           {
               // copy property items to make sure they are dynamically allocated
               Descriptor->FilterPropertySet[Index].PropertyItem = (PKSPROPERTY_ITEM)AllocateItem(NonPagedPool, FilterProperties[Index].PropertiesCount * sizeof(KSPROPERTY_ITEM), TAG_PORTCLASS);
               if (!Descriptor->FilterPropertySet[Index].PropertyItem)
               {
                   // no memory
                   goto cleanup;
               }

               // copy filter property items
               RtlMoveMemory((PVOID)Descriptor->FilterPropertySet[Index].PropertyItem, FilterProperties[Index].PropertyItem, FilterProperties[Index].PropertiesCount * sizeof(KSPROPERTY_ITEM));
           }
       }
    }

    // now check if the filter descriptor supports filter properties
    if (FilterDescription->AutomationTable)
    {
        // get first entry
        PropertyItem = (PPCPROPERTY_ITEM)FilterDescription->AutomationTable->Properties;

        // copy driver filter property sets
        for(Index = 0; Index < FilterDescription->AutomationTable->PropertyCount; Index++)
        {
            // add the property item
            Status = PcAddToPropertyTable(Descriptor, PropertyItem, FALSE);

            // check for success
            if (Status != STATUS_SUCCESS)
            {
                // goto cleanup
                goto cleanup;
            }

            // move to next entry
            PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + FilterDescription->AutomationTable->PropertyItemSize);
        }
    }

    // check if the filter has pins
    if (FilterDescription->PinCount)
    {
        // allocate pin factory descriptors
        Descriptor->Factory.KsPinDescriptor = (PKSPIN_DESCRIPTOR)AllocateItem(NonPagedPool, sizeof(KSPIN_DESCRIPTOR) * FilterDescription->PinCount, TAG_PORTCLASS);
        if (!Descriptor->Factory.KsPinDescriptor)
            goto cleanup;

        // allocate pin instance info
        Descriptor->Factory.Instances = (PPIN_INSTANCE_INFO)AllocateItem(NonPagedPool, FilterDescription->PinCount * sizeof(PIN_INSTANCE_INFO), TAG_PORTCLASS);
        if (!Descriptor->Factory.Instances)
            goto cleanup;

        // initialize pin factory descriptor
        Descriptor->Factory.PinDescriptorCount = FilterDescription->PinCount;
        Descriptor->Factory.PinDescriptorSize = sizeof(KSPIN_DESCRIPTOR);

        // grab first entry
        SrcDescriptor = (PPCPIN_DESCRIPTOR)FilterDescription->Pins;

        // copy pin factories
        for(Index = 0; Index < FilterDescription->PinCount; Index++)
        {
            // copy pin descriptor
            RtlMoveMemory(&Descriptor->Factory.KsPinDescriptor[Index], &SrcDescriptor->KsPinDescriptor, sizeof(KSPIN_DESCRIPTOR));

            // initialize pin factory instance data
            Descriptor->Factory.Instances[Index].CurrentPinInstanceCount = 0;
            Descriptor->Factory.Instances[Index].MaxFilterInstanceCount = SrcDescriptor->MaxFilterInstanceCount;
            Descriptor->Factory.Instances[Index].MaxGlobalInstanceCount = SrcDescriptor->MaxGlobalInstanceCount;
            Descriptor->Factory.Instances[Index].MinFilterInstanceCount = SrcDescriptor->MinFilterInstanceCount;

            // check if the descriptor has an automation table
            if (SrcDescriptor->AutomationTable)
            {
                // it has, grab first entry
                PropertyItem = (PPCPROPERTY_ITEM)SrcDescriptor->AutomationTable->Properties;

                // now add all supported property items
                for(SubIndex = 0; SubIndex < SrcDescriptor->AutomationTable->PropertyCount; SubIndex++)
                {
                    // add the property item to the table
                    Status = PcAddToPropertyTable(Descriptor, PropertyItem, FALSE);

                    // check for success
                    if (Status != STATUS_SUCCESS)
                    {
                        // goto cleanup
                        goto cleanup;
                    }

                    // move to next entry
                    PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + SrcDescriptor->AutomationTable->PropertyItemSize);
                }
            }

            // move to next entry
            SrcDescriptor = (PPCPIN_DESCRIPTOR)((ULONG_PTR)SrcDescriptor + FilterDescription->PinSize);
        }
    }

    // allocate topology descriptor
    Descriptor->Topology = (PKSTOPOLOGY)AllocateItem(NonPagedPool, sizeof(KSTOPOLOGY), TAG_PORTCLASS);
    if (!Descriptor->Topology)
        goto cleanup;

    // are there any connections
    if (FilterDescription->ConnectionCount)
    {
        // allocate connection descriptor
        Descriptor->Topology->TopologyConnections = (PKSTOPOLOGY_CONNECTION)AllocateItem(NonPagedPool, sizeof(KSTOPOLOGY_CONNECTION) * FilterDescription->ConnectionCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyConnections)
            goto cleanup;

        // copy connection descriptor
        RtlMoveMemory((PVOID)Descriptor->Topology->TopologyConnections, FilterDescription->Connections, FilterDescription->ConnectionCount * sizeof(PCCONNECTION_DESCRIPTOR));

        // store connection count
        Descriptor->Topology->TopologyConnectionsCount = FilterDescription->ConnectionCount;
    }

    // does the filter have nodes
    if (FilterDescription->NodeCount)
    {
        // allocate topology node types array
        Descriptor->Topology->TopologyNodes = (const GUID *)AllocateItem(NonPagedPool, sizeof(GUID) * FilterDescription->NodeCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyNodes)
            goto cleanup;

        // allocate topology node names array
        Descriptor->Topology->TopologyNodesNames = (const GUID *)AllocateItem(NonPagedPool, sizeof(GUID) * FilterDescription->NodeCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyNodesNames)
            goto cleanup;

        // grab first entry
       NodeDescriptor = (PPCNODE_DESCRIPTOR)FilterDescription->Nodes;

       // iterate all nodes and copy node types / names and node properties
        for(Index = 0; Index < FilterDescription->NodeCount; Index++)
        {
            // does it have a type
            if (NodeDescriptor->Type)
            {
                // copy node type
                RtlMoveMemory((PVOID)&Descriptor->Topology->TopologyNodes[Index], NodeDescriptor->Type, sizeof(GUID));
            }

            // does it have a node name
            if (NodeDescriptor->Name)
            {
                // copy node name
                RtlMoveMemory((PVOID)&Descriptor->Topology->TopologyNodesNames[Index], NodeDescriptor->Name, sizeof(GUID));
            }

            // check if has an automation table
            if (NodeDescriptor->AutomationTable)
            {
                // grab first entry
                PropertyItem = (PPCPROPERTY_ITEM)NodeDescriptor->AutomationTable->Properties;

                // copy all node properties into the global property set
                for(SubIndex = 0; SubIndex < NodeDescriptor->AutomationTable->PropertyCount; SubIndex++)
                {
                    // add to property set
                    Status = PcAddToPropertyTable(Descriptor, PropertyItem, TRUE);

                    // check for success
                    if (Status != STATUS_SUCCESS)
                    {
                        // failed
                        goto cleanup;
                    }

                    // move to next property item
                    PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + NodeDescriptor->AutomationTable->PropertyItemSize);
                }
            }

            // move to next descriptor
            NodeDescriptor = (PPCNODE_DESCRIPTOR)((ULONG_PTR)NodeDescriptor + FilterDescription->NodeSize);
        }

        // now store the topology node count
        Descriptor->Topology->TopologyNodesCount = FilterDescription->NodeCount;
    }

    // store descriptor
    Descriptor->DeviceDescriptor = FilterDescription;

    // store result
    *OutSubdeviceDescriptor = Descriptor;
    // done
    return STATUS_SUCCESS;

cleanup:
    if (Descriptor)
    {
        if (Descriptor->Interfaces)
            FreeItem(Descriptor->Interfaces, TAG_PORTCLASS);

        if (Descriptor->Factory.KsPinDescriptor)
            FreeItem(Descriptor->Factory.KsPinDescriptor, TAG_PORTCLASS);

        FreeItem(Descriptor, TAG_PORTCLASS);
    }
    return Status;
}

NTSTATUS
NTAPI
PcValidateConnectRequest(
    IN PIRP Irp,
    IN KSPIN_FACTORY * Factory,
    OUT PKSPIN_CONNECT * Connect)
{
    return KsValidateConnectRequest(Irp, Factory->PinDescriptorCount, Factory->KsPinDescriptor, Connect);
}
