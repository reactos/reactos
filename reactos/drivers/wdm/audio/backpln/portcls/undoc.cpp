/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/api.cpp
 * PURPOSE:         Port api functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

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
    PKSOBJECT_CREATE_ITEM CreateItem;

    // access the create item
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    // IIrpTarget is stored in Context member
    return (IIrpTarget*)CreateItem->Context;
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
    PIO_STACK_LOCATION IoStack;
    PKSP_NODE Property;
    PPCNODE_DESCRIPTOR Node;
    PPCPROPERTY_ITEM PropertyItem;
    ULONG Index;
    LPGUID Buffer;
    //PULONG Flags;
    PPCPROPERTY_REQUEST PropertyRequest;

    KSPROPERTY_ITEM_IRP_STORAGE(Irp) = (PKSPROPERTY_ITEM)SubDeviceDescriptor;

    /* try first KsPropertyHandler */
    Status = KsPropertyHandler(Irp, PropertySetCount, PropertySet);

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access property
    Property = (PKSP_NODE)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    // check if this a GUID_NULL request
    if (Status == STATUS_NOT_FOUND)
    {
        if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSP_NODE))
            return Status;

        // check if its a request for a topology node
        if (IsEqualGUIDAligned(Property->Property.Set, GUID_NULL) && Property->Property.Id == 0 && Property->Property.Flags == (KSPROPERTY_TYPE_SETSUPPORT | KSPROPERTY_TYPE_TOPOLOGY))
        {
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


            ULONG Count = 0, SubIndex;
            PropertyItem = (PCPROPERTY_ITEM*)Node->AutomationTable->Properties;
            for (Index = 0; Index < Node->AutomationTable->PropertyCount; Index++)
            {
                BOOL Found = FALSE;
                for (SubIndex = 0; SubIndex < Count; Index++)
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
        else /*if (Property->Property.Flags == (KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY) ||
                 Property->Property.Flags == (KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY) ||
                 Property->Property.Flags == (KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY)) */
        {
            //UNICODE_STRING GuidString;

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
                return STATUS_NOT_FOUND;
            }

            PC_ASSERT(Node->AutomationTable);
            PC_ASSERT(Node->AutomationTable->PropertyCount);
            PC_ASSERT(Node->AutomationTable->PropertyItemSize);

            PropertyItem = (PCPROPERTY_ITEM*)Node->AutomationTable->Properties;

            for(Index = 0; Index < Node->AutomationTable->PropertyCount; Index++)
            {
                if (IsEqualGUIDAligned(*PropertyItem->Set, Property->Property.Set) && PropertyItem->Id == Property->Property.Id)
                {
                    if (Property->Property.Flags & KSPROPERTY_TYPE_BASICSUPPORT)
                    {
                        if (!(PropertyItem->Flags & KSPROPERTY_TYPE_BASICSUPPORT))
                        {
                            PC_ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG));
                            PULONG Flags = (PULONG)Irp->UserBuffer;

                            /* reset flags */
                            *Flags = 0;

                            if (PropertyItem->Flags & KSPROPERTY_TYPE_SET)
                                *Flags |= KSPROPERTY_TYPE_SET;

                            if (PropertyItem->Flags & KSPROPERTY_TYPE_GET)
                                *Flags |= KSPROPERTY_TYPE_GET;

                            Irp->IoStatus.Information = sizeof(ULONG);

                            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(KSPROPERTY_DESCRIPTION))
                            {
                                /* get output buffer */
                                PKSPROPERTY_DESCRIPTION Description = (PKSPROPERTY_DESCRIPTION)Irp->UserBuffer;

                                /* store result */
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
                    }


                    PropertyRequest = (PPCPROPERTY_REQUEST)AllocateItem(NonPagedPool, sizeof(PCPROPERTY_REQUEST), TAG_PORTCLASS);
                    if (!PropertyRequest)
                        return STATUS_INSUFFICIENT_RESOURCES;

                    PC_ASSERT(SubDeviceDescriptor->UnknownMiniport);
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
                        //DPRINT1("Status %x ValueSize %u 

                        Irp->IoStatus.Information = PropertyRequest->ValueSize;
                        ExFreePool(PropertyRequest);
                    }
#if 0
                    RtlStringFromGUID(Property->Property.Set, &GuidString);
                    DPRINT1("Id %u Flags %x Set %S FlagsItem %x Status %x\n", Property->Property.Id, Property->Property.Flags, GuidString.Buffer, PropertyItem->Flags, Status);
                    RtlFreeUnicodeString(&GuidString);
#endif
                    return Status;
                }
                PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + Node->AutomationTable->PropertyItemSize);
            }
#if 0
            RtlStringFromGUID(Property->Property.Set, &GuidString);
            DPRINT1("Id %u Flags %x Set %S Status %x\n", Property->Property.Id, Property->Property.Flags, GuidString.Buffer, Status);
            RtlFreeUnicodeString(&GuidString);
#endif
        }
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
    ULONG Index;
    PPCPROPERTY_ITEM PropertyItem;
    UNICODE_STRING GuidString;

    DPRINT1("======================\n");
    DPRINT1("Descriptor Automation Table%p\n",FilterDescription->AutomationTable);

    if (FilterDescription->AutomationTable)
    {
        DPRINT1("FilterPropertiesCount %u FilterPropertySize %u Expected %u\n", FilterDescription->AutomationTable->PropertyCount, FilterDescription->AutomationTable->PropertyItemSize, sizeof(PCPROPERTY_ITEM));
        if (FilterDescription->AutomationTable->PropertyCount)
        {
            PropertyItem = (PPCPROPERTY_ITEM)FilterDescription->AutomationTable->Properties;

            for(Index = 0; Index < FilterDescription->AutomationTable->PropertyCount; Index++)
            {
                RtlStringFromGUID(*PropertyItem->Set, &GuidString);
                DPRINT1("Index %u GUID %S Id %u Flags %x\n", Index, GuidString.Buffer, PropertyItem->Id, PropertyItem->Flags);

                PropertyItem = (PPCPROPERTY_ITEM)((ULONG_PTR)PropertyItem + FilterDescription->AutomationTable->PropertyItemSize);
            }
        }
    }


    DPRINT1("======================\n");
    DbgBreakPoint();
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

       //DumpFilterDescriptor(FilterDescription);

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
        DPRINT("Size %u Expected %u Ex Size %u\n", FilterDescription->PinSize, sizeof(KSPIN_DESCRIPTOR), sizeof(KSPIN_DESCRIPTOR_EX));

        // copy pin factories
        for(Index = 0; Index < FilterDescription->PinCount; Index++)
        {
            RtlMoveMemory(&Descriptor->Factory.KsPinDescriptor[Index], &SrcDescriptor->KsPinDescriptor, sizeof(KSPIN_DESCRIPTOR));

            Descriptor->Factory.Instances[Index].CurrentPinInstanceCount = 0;
            Descriptor->Factory.Instances[Index].MaxFilterInstanceCount = FilterDescription->Pins[Index].MaxFilterInstanceCount;
            Descriptor->Factory.Instances[Index].MaxGlobalInstanceCount = FilterDescription->Pins[Index].MaxGlobalInstanceCount;
            Descriptor->Factory.Instances[Index].MinFilterInstanceCount = FilterDescription->Pins[Index].MinFilterInstanceCount;
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

