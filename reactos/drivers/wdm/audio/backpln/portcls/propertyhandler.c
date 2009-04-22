/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/propertyhandler.c
 * PURPOSE:         Pin property handler
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

NTSTATUS
FindPropertyHandler(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN PKSPROPERTY Property,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    OUT PFNKSHANDLER *PropertyHandler);

NTSTATUS
HandlePropertyInstances(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN BOOL Global)
{
    KSPIN_CINSTANCES * Instances;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    if (Pin->PinId >= Descriptor->Factory.PinDescriptorCount)
    {
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    Instances = (KSPIN_CINSTANCES*)Data;

    if (Global)
        Instances->PossibleCount = Descriptor->Factory.Instances[Pin->PinId].MaxGlobalInstanceCount;
    else
        Instances->PossibleCount = Descriptor->Factory.Instances[Pin->PinId].MaxFilterInstanceCount;

    Instances->CurrentCount = Descriptor->Factory.Instances[Pin->PinId].CurrentPinInstanceCount;

    IoStatus->Information = sizeof(KSPIN_CINSTANCES);
    IoStatus->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
HandleNecessaryPropertyInstances(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN PSUBDEVICE_DESCRIPTOR Descriptor)
{
    PULONG Result;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    if (Pin->PinId >= Descriptor->Factory.PinDescriptorCount)
    {
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    Result = (PULONG)Data;
    *Result = Descriptor->Factory.Instances[Pin->PinId].MinFilterInstanceCount;

    IoStatus->Information = sizeof(ULONG);
    IoStatus->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
HandleDataIntersection(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID  Data,
    IN ULONG DataLength,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN ISubdevice *SubDevice)
{
    KSP_PIN * Pin = (KSP_PIN*)Request;
    PKSMULTIPLE_ITEM MultipleItem;
    PKSDATARANGE DataRange;
    NTSTATUS Status = STATUS_NO_MATCH;
    ULONG Index, Length;

    /* Access parameters */
    MultipleItem = (PKSMULTIPLE_ITEM)(Pin + 1);
    DataRange = (PKSDATARANGE)(MultipleItem + 1);

    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        /* Call miniport's properitary handler */
        ASSERT(Descriptor->Factory.KsPinDescriptor[Pin->PinId].DataRangesCount);
        ASSERT(Descriptor->Factory.KsPinDescriptor[Pin->PinId].DataRanges[0]);
        Status = SubDevice->lpVtbl->DataRangeIntersection(SubDevice, Pin->PinId, DataRange, (PKSDATARANGE)Descriptor->Factory.KsPinDescriptor[Pin->PinId].DataRanges[0],
                                                          DataLength, Data, &Length);

        if (Status == STATUS_SUCCESS)
        {
            IoStatus->Information = Length;
            break;
        }
        DataRange =  UlongToPtr(PtrToUlong(DataRange) + DataRange->FormatSize);
    }

    IoStatus->Status = Status;
    return Status;
}


NTSTATUS
NTAPI
PinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PKSOBJECT_CREATE_ITEM CreateItem;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;
    IPort *Port;
    ISubdevice *SubDevice;


    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);
    ASSERT(Descriptor);

    /* Access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    /* Get the IrpTarget */
    IrpTarget = (IIrpTarget*)CreateItem->Context;
    /* Get the parent */
    Status = IrpTarget->lpVtbl->QueryInterface(IrpTarget, &IID_IPort, (PVOID*)&Port);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to obtain IPort interface from filter\n");
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    /* Get private ISubdevice interface */
    Status = Port->lpVtbl->QueryInterface(Port, &IID_ISubdevice, (PVOID*)&SubDevice);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to obtain ISubdevice interface from port driver\n");
        KeBugCheck(0);
    }

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(Request->Id)
    {
        case KSPROPERTY_PIN_CTYPES:
        case KSPROPERTY_PIN_DATAFLOW:
        case KSPROPERTY_PIN_DATARANGES:
        case KSPROPERTY_PIN_INTERFACES:
        case KSPROPERTY_PIN_MEDIUMS:
        case KSPROPERTY_PIN_COMMUNICATION:
        case KSPROPERTY_PIN_CATEGORY:
        case KSPROPERTY_PIN_NAME:
        case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
            Status = KsPinPropertyHandler(Irp, Request, Data, Descriptor->Factory.PinDescriptorCount, Descriptor->Factory.KsPinDescriptor);
            break;
        case KSPROPERTY_PIN_GLOBALCINSTANCES:
            Status = HandlePropertyInstances(&Irp->IoStatus, Request, Data, Descriptor, TRUE);
            break;
        case KSPROPERTY_PIN_CINSTANCES:
            Status = HandlePropertyInstances(&Irp->IoStatus, Request, Data, Descriptor, FALSE);
            break;
        case KSPROPERTY_PIN_NECESSARYINSTANCES:
            Status = HandleNecessaryPropertyInstances(&Irp->IoStatus, Request, Data, Descriptor);
            break;

        case KSPROPERTY_PIN_DATAINTERSECTION:
            Status = HandleDataIntersection(&Irp->IoStatus, Request, Data, IoStack->Parameters.DeviceIoControl.OutputBufferLength, Descriptor, SubDevice);
            break;
        case KSPROPERTY_PIN_PHYSICALCONNECTION:
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
            UNIMPLEMENTED
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        default:
            UNIMPLEMENTED
            Status = STATUS_UNSUCCESSFUL;
    }

    /* Release reference */
    Port->lpVtbl->Release(Port);

    return Status;
}

NTSTATUS
NTAPI
FastPropertyHandler(
    IN PFILE_OBJECT  FileObject,
    IN PKSPROPERTY UNALIGNED  Property,
    IN ULONG  PropertyLength,
    IN OUT PVOID UNALIGNED  Data,
    IN ULONG  DataLength,
    OUT PIO_STATUS_BLOCK  IoStatus,
    IN ULONG  PropertySetsCount,
    IN const KSPROPERTY_SET *PropertySet,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN ISubdevice *SubDevice)
{
    PFNKSHANDLER PropertyHandler = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    KSP_PIN * Pin;

    ASSERT(Descriptor);

    if (!IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Pin))
    {
        /* the fast handler only supports pin properties */
        return Status;
    }

    /* property handler is used to verify input parameters */
    Status = FindPropertyHandler(IoStatus, Descriptor, Property, PropertyLength, DataLength, &PropertyHandler);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    switch(Property->Id)
    {
        case KSPROPERTY_PIN_CTYPES:
            (*(PULONG)Data) = Descriptor->Factory.PinDescriptorCount;
            IoStatus->Information = sizeof(ULONG);
            IoStatus->Status = Status = STATUS_SUCCESS;
            break;
        case KSPROPERTY_PIN_DATAFLOW:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= Descriptor->Factory.PinDescriptorCount)
            {
                IoStatus->Status = Status = STATUS_INVALID_PARAMETER;
                IoStatus->Information = 0;
                break;
            }

            *((KSPIN_DATAFLOW*)Data) = Descriptor->Factory.KsPinDescriptor[Pin->PinId].DataFlow;
            IoStatus->Information = sizeof(KSPIN_DATAFLOW);
            IoStatus->Status = Status = STATUS_SUCCESS;
            break;
        case KSPROPERTY_PIN_COMMUNICATION:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= Descriptor->Factory.PinDescriptorCount)
            {
                IoStatus->Status = Status = STATUS_INVALID_PARAMETER;
                IoStatus->Information = 0;
                break;
            }

            *((KSPIN_COMMUNICATION*)Data) = Descriptor->Factory.KsPinDescriptor[Pin->PinId].Communication;
            IoStatus->Status = Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof(KSPIN_COMMUNICATION);
            break;

        case KSPROPERTY_PIN_GLOBALCINSTANCES:
            Status = HandlePropertyInstances(IoStatus, Property, Data, Descriptor, TRUE);
            break;
        case KSPROPERTY_PIN_CINSTANCES:
            Status = HandlePropertyInstances(IoStatus, Property, Data, Descriptor, FALSE);
            break;
        case KSPROPERTY_PIN_NECESSARYINSTANCES:
            Status = HandleNecessaryPropertyInstances(IoStatus, Property, Data, Descriptor);
            break;

        case KSPROPERTY_PIN_DATAINTERSECTION:
            Status = HandleDataIntersection(IoStatus, Property, Data, DataLength, Descriptor, SubDevice);
            break;
        case KSPROPERTY_PIN_PHYSICALCONNECTION:
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
        case KSPROPERTY_PIN_DATARANGES:
        case KSPROPERTY_PIN_INTERFACES:
        case KSPROPERTY_PIN_MEDIUMS:
        case KSPROPERTY_PIN_CATEGORY:
        case KSPROPERTY_PIN_NAME:
        case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
            UNIMPLEMENTED
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        default:
            UNIMPLEMENTED
            Status = STATUS_NOT_IMPLEMENTED;
    }
    return Status;
}


NTSTATUS
NTAPI
TopologyPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    return KsTopologyPropertyHandler(Irp,
                                     Request,
                                     Data,
                                     NULL /* FIXME */);
}

NTSTATUS
FindPropertyHandler(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN PKSPROPERTY Property,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    OUT PFNKSHANDLER *PropertyHandler)
{
    ULONG Index, ItemIndex;

    for(Index = 0; Index < Descriptor->FilterPropertySet.FreeKsPropertySetOffset; Index++)
    {
        if (IsEqualGUIDAligned(&Property->Set, Descriptor->FilterPropertySet.Properties[Index].Set))
        {
            for(ItemIndex = 0; ItemIndex < Descriptor->FilterPropertySet.Properties[Index].PropertiesCount; ItemIndex++)
            {
                if (Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].PropertyId == Property->Id)
                {
                    if (Property->Flags & KSPROPERTY_TYPE_SET)
                        *PropertyHandler = Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].SetPropertyHandler;

                    if (Property->Flags & KSPROPERTY_TYPE_GET)
                        *PropertyHandler = Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].GetPropertyHandler;

                    if (Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].MinProperty > InputBufferLength)
                    {
                        /* too small input buffer */
                        IoStatus->Information = Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].MinProperty;
                        IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
                        return STATUS_BUFFER_TOO_SMALL;
                    }

                    if (Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].MinData > OutputBufferLength)
                    {
                        /* too small output buffer */
                        IoStatus->Information = Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].MinData;
                        IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
                        return STATUS_BUFFER_TOO_SMALL;
                    }
                    return STATUS_SUCCESS;
                }
            }
        }
    }
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
NTAPI
PcPropertyHandler(
    IN PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR Descriptor)
{
    ULONG Index;
    PIO_STACK_LOCATION IoStack;
    PKSPROPERTY Property;
    PFNKSHANDLER PropertyHandler = NULL;
    UNICODE_STRING GuidString;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PPCPROPERTY_REQUEST PropertyRequest;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    ASSERT(Property);

    /* check properties provided by the driver */
    if (Descriptor->DeviceDescriptor->AutomationTable)
    {
        for(Index = 0; Index < Descriptor->DeviceDescriptor->AutomationTable->PropertyCount; Index++)
        {
            if (IsEqualGUID(Descriptor->DeviceDescriptor->AutomationTable->Properties[Index].Set, &Property->Set))
            {
                if (Descriptor->DeviceDescriptor->AutomationTable->Properties[Index].Id == Property->Id)
                {
                    if(Descriptor->DeviceDescriptor->AutomationTable->Properties[Index].Flags & Property->Flags)
                    {
                        PropertyRequest = ExAllocatePool(NonPagedPool, sizeof(PCPROPERTY_REQUEST));
                        if (!PropertyRequest)
                        {
                            /* no memory */
                            Irp->IoStatus.Information = 0;
                            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                            IoCompleteRequest(Irp, IO_NO_INCREMENT);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        RtlZeroMemory(PropertyRequest, sizeof(PCPROPERTY_REQUEST));
                        PropertyRequest->PropertyItem = &Descriptor->DeviceDescriptor->AutomationTable->Properties[Index];
                        PropertyRequest->Verb = Property->Flags;
                        PropertyRequest->Value = Irp->UserBuffer;
                        PropertyRequest->ValueSize = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
                        PropertyRequest->Irp = Irp;

                        DPRINT("Calling handler %p\n", Descriptor->DeviceDescriptor->AutomationTable->Properties[Index].Handler);
                        Status = Descriptor->DeviceDescriptor->AutomationTable->Properties[Index].Handler(PropertyRequest);

                        Irp->IoStatus.Status = Status;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return Status;
                    }
                }
            }
        }
    }

    Status = FindPropertyHandler(&Irp->IoStatus, Descriptor, Property, IoStack->Parameters.DeviceIoControl.InputBufferLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength, &PropertyHandler);
    if (PropertyHandler)
    {
        KSPROPERTY_ITEM_IRP_STORAGE(Irp) = (PVOID)Descriptor;
        DPRINT("Calling property handler %p\n", PropertyHandler);
        Status = PropertyHandler(Irp, Property, Irp->UserBuffer);
    }
    else
    {
        RtlStringFromGUID(&Property->Set, &GuidString);
        DPRINT1("Unhandeled property: Set %S Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
        RtlFreeUnicodeString(&GuidString);
    }

    /* the information member is set by the handler */
    Irp->IoStatus.Status = Status;
    DPRINT("Result %x Length %u\n", Status, Irp->IoStatus.Information);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
