/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/api.cpp
 * PURPOSE:         Port api functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"


KSPIN_INTERFACE PinInterfaces[] = 
{
    {
        {STATIC_KSINTERFACESETID_Standard},
        KSINTERFACE_STANDARD_STREAMING,
        0
    },
    {
        {STATIC_KSINTERFACESETID_Standard},
        KSINTERFACE_STANDARD_LOOPED_STREAMING,
        0
    }
};


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
PcHandleGuidNullRequest(
    IN OUT PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor)
{
    PPCNODE_DESCRIPTOR Node;
    PPCPROPERTY_ITEM PropertyItem;
    PIO_STACK_LOCATION IoStack;
    PKSP_NODE Property;
    LPGUID Buffer;
    ULONG Count = 0, SubIndex, Index;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);


    // access property
    Property = (PKSP_NODE)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (Property->NodeId >= SubDeviceDescriptor->DeviceDescriptor->NodeCount)
    {
        // request is out of bounds
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    Node = (PPCNODE_DESCRIPTOR)((ULONG_PTR)SubDeviceDescriptor->DeviceDescriptor->Nodes + (Property->NodeId * SubDeviceDescriptor->DeviceDescriptor->NodeSize));

    if (!Node->AutomationTable)
    {
        // request is out of bounds
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    PC_ASSERT(Node->AutomationTable);
    PC_ASSERT(Node->AutomationTable->PropertyCount);
    PC_ASSERT(Node->AutomationTable->PropertyItemSize);

    Buffer = (LPGUID)AllocateItem(NonPagedPool,  sizeof (GUID) * Node->AutomationTable->PropertyCount, TAG_PORTCLASS);
    if  (!Buffer)
        return  STATUS_INSUFFICIENT_RESOURCES;

     PropertyItem = (PCPROPERTY_ITEM*)Node->AutomationTable->Properties;
     for (Index = 0; Index < Node->AutomationTable->PropertyCount; Index++)
     {
         BOOL Found = FALSE;
         for (SubIndex = 0; SubIndex < Count; SubIndex++)
         {
             if  (IsEqualGUIDAligned(Buffer[SubIndex], *PropertyItem->Set))
             {
                 Found = TRUE;
                 break;
             }
         }
         if (!Found)
         {
             RtlMoveMemory(&Buffer[Count], PropertyItem->Set, sizeof (GUID));
             Count++;
         }
         PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + Node->AutomationTable->PropertyItemSize);
     }

     // store result length
     Irp->IoStatus.Information =  sizeof (GUID) * Count;
     if  (IoStack->Parameters.DeviceIoControl.OutputBufferLength <  sizeof (GUID) * Count)
     {
         // buffer too small
         FreeItem(Buffer, TAG_PORTCLASS);
         return  STATUS_MORE_ENTRIES;
     }

     RtlMoveMemory(Irp->UserBuffer, Buffer,  sizeof (GUID) * Count);
     FreeItem(Buffer, TAG_PORTCLASS);
     return STATUS_SUCCESS;
}

NTSTATUS
PcFindNodePropertyHandler(
    PIRP Irp,
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor,
    OUT PPCPROPERTY_ITEM * OutPropertyItem)
{
    PPCNODE_DESCRIPTOR Node;
    PPCPROPERTY_ITEM PropertyItem;
    PIO_STACK_LOCATION IoStack;
    PKSP_NODE Property;
    ULONG Index;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access property
    Property = (PKSP_NODE)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (Property->NodeId >= SubDeviceDescriptor->DeviceDescriptor->NodeCount)
    {
        // request is out of bounds
        DPRINT("InvalidIndex %u %u\n", Property->NodeId, SubDeviceDescriptor->DeviceDescriptor->NodeCount);
        return STATUS_INVALID_PARAMETER;
    }

    Node = (PPCNODE_DESCRIPTOR)((ULONG_PTR)SubDeviceDescriptor->DeviceDescriptor->Nodes + (Property->NodeId * SubDeviceDescriptor->DeviceDescriptor->NodeSize));

    if (!Node->AutomationTable)
    {
        // request is out of bounds
        Irp->IoStatus.Information = 0;
        return STATUS_NOT_FOUND;
    }

    // sanity checks
    PC_ASSERT(Node->AutomationTable);
    PC_ASSERT(Node->AutomationTable->PropertyCount);
    PC_ASSERT(Node->AutomationTable->PropertyItemSize);

    PropertyItem = (PCPROPERTY_ITEM*)Node->AutomationTable->Properties;

    DPRINT("NodeId %u PropertyCount %u\n", Property->NodeId, Node->AutomationTable->PropertyCount);
    for(Index = 0; Index < Node->AutomationTable->PropertyCount; Index++)
    {
        if (IsEqualGUIDAligned(*PropertyItem->Set, Property->Property.Set) && PropertyItem->Id == Property->Property.Id)
        {
            //found property handler
            *OutPropertyItem = PropertyItem;
            return STATUS_SUCCESS;
        }
        PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + Node->AutomationTable->PropertyItemSize);
    }

    // no handler yet found
    DPRINT("NotFound\n");
    return STATUS_NOT_FOUND;
}

NTSTATUS
PcNodeBasicSupportHandler(
    PIRP Irp,
    PPCPROPERTY_ITEM PropertyItem)
{
    PULONG Flags;
    PIO_STACK_LOCATION IoStack;
    PKSPROPERTY_DESCRIPTION Description;
    PKSP_NODE Property;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access property
    Property = (PKSP_NODE)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    PC_ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG));
    Flags= (PULONG)Irp->UserBuffer;

    // reset flags
    *Flags = 0;

    if (PropertyItem->Flags & KSPROPERTY_TYPE_SET)
        *Flags |= KSPROPERTY_TYPE_SET;

    if (PropertyItem->Flags & KSPROPERTY_TYPE_GET)
        *Flags |= KSPROPERTY_TYPE_GET;

    // store result length
    Irp->IoStatus.Information = sizeof(ULONG);

    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(KSPROPERTY_DESCRIPTION))
    {
        // get output buffer
        Description = (PKSPROPERTY_DESCRIPTION)Irp->UserBuffer;

        // store result
        Description->DescriptionSize = sizeof(KSPROPERTY_DESCRIPTION);
        Description->PropTypeSet.Set = KSPROPTYPESETID_General;
        Description->PropTypeSet.Id = 0;
        Description->PropTypeSet.Flags = 0;
        Description->MembersListCount = 0;
        Description->Reserved = 0;

        Irp->IoStatus.Information = sizeof(KSPROPERTY_DESCRIPTION);
    }
    return STATUS_SUCCESS;
}


NTSTATUS
PcHandleNodePropertyRequest(
    PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor)
{
    PIO_STACK_LOCATION IoStack;
    PPCPROPERTY_ITEM  PropertyItem;
    PPCPROPERTY_REQUEST PropertyRequest;
    PKSP_NODE Property;
    NTSTATUS Status;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSP_NODE))
    {
        // certainly not a node property request
        return STATUS_NOT_FOUND;
    }

    // access property
    Property = (PKSP_NODE)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (IsEqualGUIDAligned(Property->Property.Set, GUID_NULL) && Property->Property.Id == 0 && Property->Property.Flags == (KSPROPERTY_TYPE_SETSUPPORT | KSPROPERTY_TYPE_TOPOLOGY))
    {
        return PcHandleGuidNullRequest(Irp, SubDeviceDescriptor);
    }

    // find property handler
    Status = PcFindNodePropertyHandler(Irp, SubDeviceDescriptor, &PropertyItem);

    // check for success
    if (!NT_SUCCESS(Status))
    {
        // might not be a node property request
        DPRINT("NotFound\n");
        return STATUS_NOT_FOUND;
    }

    if (Property->Property.Flags & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        // caller issued a basic property request
        if (!(PropertyItem->Flags & KSPROPERTY_TYPE_BASICSUPPORT))
        {
            // driver does not have a basic support handler
            return PcNodeBasicSupportHandler(Irp, PropertyItem);
        }
    }

    // sanity check
    PC_ASSERT(SubDeviceDescriptor->UnknownMiniport);

    // allocate a property request
    PropertyRequest = (PPCPROPERTY_REQUEST)AllocateItem(NonPagedPool, sizeof(PCPROPERTY_REQUEST), TAG_PORTCLASS);
    if (!PropertyRequest)
        return STATUS_INSUFFICIENT_RESOURCES;

     // initialize property request
     PropertyRequest->MajorTarget = SubDeviceDescriptor->UnknownMiniport;
     PropertyRequest->MinorTarget = SubDeviceDescriptor->UnknownStream;
     PropertyRequest->Irp = Irp;
     PropertyRequest->Node = Property->NodeId;
     PropertyRequest->PropertyItem = PropertyItem;
     PropertyRequest->Verb = Property->Property.Flags;
     PropertyRequest->InstanceSize = IoStack->Parameters.DeviceIoControl.InputBufferLength - sizeof(KSNODEPROPERTY);
     PropertyRequest->Instance = (PVOID)((ULONG_PTR)IoStack->Parameters.DeviceIoControl.Type3InputBuffer + sizeof(KSNODEPROPERTY));
     PropertyRequest->ValueSize = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
     PropertyRequest->Value = Irp->UserBuffer;

     Status = PropertyItem->Handler(PropertyRequest);

     if (Status != STATUS_PENDING)
     {
         //request completed
         Irp->IoStatus.Information = PropertyRequest->ValueSize;
         FreeItem(PropertyRequest, TAG_PORTCLASS);
     }

     // done
     DPRINT("Status %x\n", Status);
     return Status;
}

NTSTATUS
NTAPI
PcHandlePropertyWithTable(
    IN PIRP Irp,
    IN ULONG PropertySetCount,
    IN PKSPROPERTY_SET PropertySet,
    IN PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor)
{
    NTSTATUS Status;

    // try handle it as node property request
    Status = PcHandleNodePropertyRequest(Irp, SubDeviceDescriptor);

    if (Status == STATUS_NOT_FOUND)
    {
        // store device descriptor
        KSPROPERTY_ITEM_IRP_STORAGE(Irp) = (PKSPROPERTY_ITEM)SubDeviceDescriptor;

        /* then try KsPropertyHandler */
        Status = KsPropertyHandler(Irp, PropertySetCount, PropertySet);
    }

    return Status;
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
PcAddToPropertyTable(
    PVOID Ptr,
    LONG Unknown,
    LONG Unknown2,
    LONG Unknown3,
    CHAR Unknown4)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
DumpFilterDescriptor(
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    ULONG Index, SubIndex;
    PPCPROPERTY_ITEM PropertyItem;
    PPCEVENT_ITEM EventItem;
    PPCNODE_DESCRIPTOR NodeDescriptor;
    UNICODE_STRING GuidString;



    DPRINT("======================\n");
    DPRINT("Descriptor Automation Table %p\n",FilterDescription->AutomationTable);

    if (FilterDescription->AutomationTable)
    {
        DPRINT("FilterPropertiesCount %u FilterPropertySize %u Expected %u Events %u EventItemSize %u expected %u\n", FilterDescription->AutomationTable->PropertyCount, FilterDescription->AutomationTable->PropertyItemSize, sizeof(PCPROPERTY_ITEM),
                FilterDescription->AutomationTable->EventCount, FilterDescription->AutomationTable->EventItemSize, sizeof(PCEVENT_ITEM));
        if (FilterDescription->AutomationTable->PropertyCount)
        {
            PropertyItem = (PPCPROPERTY_ITEM)FilterDescription->AutomationTable->Properties;

            for(Index = 0; Index < FilterDescription->AutomationTable->PropertyCount; Index++)
            {
                RtlStringFromGUID(*PropertyItem->Set, &GuidString);
                DPRINT("Property Index %u GUID %S Id %u Flags %x\n", Index, GuidString.Buffer, PropertyItem->Id, PropertyItem->Flags);

                PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + FilterDescription->AutomationTable->PropertyItemSize);
            }

            EventItem = (PPCEVENT_ITEM)FilterDescription->AutomationTable->Events;
            for(Index = 0; Index < FilterDescription->AutomationTable->EventCount; Index++)
            {
                RtlStringFromGUID(*EventItem->Set, &GuidString);
                DPRINT("EventIndex %u GUID %S Id %u Flags %x\n", Index, GuidString.Buffer, EventItem->Id, EventItem->Flags);

                EventItem = (PPCEVENT_ITEM)((ULONG_PTR)EventItem + FilterDescription->AutomationTable->EventItemSize);
            }

        }
    }

    if (FilterDescription->Nodes)
    {
        DPRINT("NodeCount %u NodeSize %u expected %u\n", FilterDescription->NodeCount, FilterDescription->NodeSize, sizeof(PCNODE_DESCRIPTOR));
        NodeDescriptor = (PPCNODE_DESCRIPTOR)FilterDescription->Nodes;
        for(Index = 0; Index < FilterDescription->NodeCount; Index++)
        {
            DPRINT("Index %u AutomationTable %p\n", Index, NodeDescriptor->AutomationTable);

            if (NodeDescriptor->AutomationTable)
            {
                DPRINT(" Index %u EventCount %u\n", Index, NodeDescriptor->AutomationTable->EventCount);
                EventItem = (PPCEVENT_ITEM)NodeDescriptor->AutomationTable->Events;
                for(SubIndex = 0; SubIndex < NodeDescriptor->AutomationTable->EventCount; SubIndex++)
                {
                    RtlStringFromGUID(*EventItem->Set, &GuidString);
                    DPRINT("  EventIndex %u GUID %S Id %u Flags %x\n", SubIndex, GuidString.Buffer, EventItem->Id, EventItem->Flags);

                    EventItem = (PPCEVENT_ITEM)((ULONG_PTR)EventItem + NodeDescriptor->AutomationTable->EventItemSize);
                }

            }


            NodeDescriptor = (PPCNODE_DESCRIPTOR)((ULONG_PTR)NodeDescriptor + FilterDescription->NodeSize);
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
    ULONG Index;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PPCPIN_DESCRIPTOR SrcDescriptor;

    Descriptor = (PSUBDEVICE_DESCRIPTOR)AllocateItem(NonPagedPool, sizeof(SUBDEVICE_DESCRIPTOR), TAG_PORTCLASS);
    if (!Descriptor)
        return STATUS_INSUFFICIENT_RESOURCES;

    // initialize physical / symbolic link connection list 
    InitializeListHead(&Descriptor->SymbolicLinkList);
    InitializeListHead(&Descriptor->PhysicalConnectionList);

    Descriptor->Interfaces = (GUID*)AllocateItem(NonPagedPool, sizeof(GUID) * InterfaceCount, TAG_PORTCLASS);
    if (!Descriptor->Interfaces)
        goto cleanup;

    // copy interface guids
    RtlCopyMemory(Descriptor->Interfaces, InterfaceGuids, sizeof(GUID) * InterfaceCount);
    Descriptor->InterfaceCount = InterfaceCount;

    if (FilterPropertiesCount)
    {
       /// FIXME
       /// handle driver properties

       DumpFilterDescriptor(FilterDescription);

       Descriptor->FilterPropertySet = (PKSPROPERTY_SET)AllocateItem(NonPagedPool, sizeof(KSPROPERTY_SET) * FilterPropertiesCount, TAG_PORTCLASS);
       if (! Descriptor->FilterPropertySet)
           goto cleanup;

       Descriptor->FilterPropertySetCount = FilterPropertiesCount;
       for(Index = 0; Index < FilterPropertiesCount; Index++)
       {
           RtlMoveMemory(&Descriptor->FilterPropertySet[Index], &FilterProperties[Index], sizeof(KSPROPERTY_SET));
       }
    }

    Descriptor->Topology = (PKSTOPOLOGY)AllocateItem(NonPagedPool, sizeof(KSTOPOLOGY), TAG_PORTCLASS);
    if (!Descriptor->Topology)
        goto cleanup;

    if (FilterDescription->ConnectionCount)
    {
        Descriptor->Topology->TopologyConnections = (PKSTOPOLOGY_CONNECTION)AllocateItem(NonPagedPool, sizeof(KSTOPOLOGY_CONNECTION) * FilterDescription->ConnectionCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyConnections)
            goto cleanup;

        RtlMoveMemory((PVOID)Descriptor->Topology->TopologyConnections, FilterDescription->Connections, FilterDescription->ConnectionCount * sizeof(PCCONNECTION_DESCRIPTOR));
        Descriptor->Topology->TopologyConnectionsCount = FilterDescription->ConnectionCount;
    }

    if (FilterDescription->NodeCount)
    {
        Descriptor->Topology->TopologyNodes = (const GUID *)AllocateItem(NonPagedPool, sizeof(GUID) * FilterDescription->NodeCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyNodes)
            goto cleanup;

        Descriptor->Topology->TopologyNodesNames = (const GUID *)AllocateItem(NonPagedPool, sizeof(GUID) * FilterDescription->NodeCount, TAG_PORTCLASS);
        if (!Descriptor->Topology->TopologyNodesNames)
            goto cleanup;

        for(Index = 0; Index < FilterDescription->NodeCount; Index++)
        {
            if (FilterDescription->Nodes[Index].Type)
            {
                RtlMoveMemory((PVOID)&Descriptor->Topology->TopologyNodes[Index], FilterDescription->Nodes[Index].Type, sizeof(GUID));
            }
            if (FilterDescription->Nodes[Index].Name)
            {
                RtlMoveMemory((PVOID)&Descriptor->Topology->TopologyNodesNames[Index], FilterDescription->Nodes[Index].Name, sizeof(GUID));
            }
        }
        Descriptor->Topology->TopologyNodesCount = FilterDescription->NodeCount;
    }

    if (FilterDescription->PinCount)
    {
        Descriptor->Factory.KsPinDescriptor = (PKSPIN_DESCRIPTOR)AllocateItem(NonPagedPool, sizeof(KSPIN_DESCRIPTOR) * FilterDescription->PinCount, TAG_PORTCLASS);
        if (!Descriptor->Factory.KsPinDescriptor)
            goto cleanup;

        Descriptor->Factory.Instances = (PPIN_INSTANCE_INFO)AllocateItem(NonPagedPool, FilterDescription->PinCount * sizeof(PIN_INSTANCE_INFO), TAG_PORTCLASS);
        if (!Descriptor->Factory.Instances)
            goto cleanup;

        Descriptor->Factory.PinDescriptorCount = FilterDescription->PinCount;
        Descriptor->Factory.PinDescriptorSize = sizeof(KSPIN_DESCRIPTOR);

        SrcDescriptor = (PPCPIN_DESCRIPTOR)FilterDescription->Pins;
        DPRINT("Size %u Expected %u\n", FilterDescription->PinSize, sizeof(PCPIN_DESCRIPTOR));

        // copy pin factories
        for(Index = 0; Index < FilterDescription->PinCount; Index++)
        {
            RtlMoveMemory(&Descriptor->Factory.KsPinDescriptor[Index], &SrcDescriptor->KsPinDescriptor, sizeof(KSPIN_DESCRIPTOR));

            Descriptor->Factory.KsPinDescriptor[Index].Interfaces = PinInterfaces;
            Descriptor->Factory.KsPinDescriptor[Index].InterfacesCount = sizeof(PinInterfaces) / sizeof(KSPIN_INTERFACE);

            DPRINT("Index %u DataRangeCount %u\n", Index, SrcDescriptor->KsPinDescriptor.DataRangesCount);

            Descriptor->Factory.Instances[Index].CurrentPinInstanceCount = 0;
            Descriptor->Factory.Instances[Index].MaxFilterInstanceCount = SrcDescriptor->MaxFilterInstanceCount;
            Descriptor->Factory.Instances[Index].MaxGlobalInstanceCount = SrcDescriptor->MaxGlobalInstanceCount;
            Descriptor->Factory.Instances[Index].MinFilterInstanceCount = SrcDescriptor->MinFilterInstanceCount;
            SrcDescriptor = (PPCPIN_DESCRIPTOR)((ULONG_PTR)SrcDescriptor + FilterDescription->PinSize);
        }
    }

    Descriptor->DeviceDescriptor = FilterDescription;
    *OutSubdeviceDescriptor = Descriptor;
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

