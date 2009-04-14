/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/propertyhandler.c
 * PURPOSE:         Pin property handler
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

NTSTATUS
HandlePropertyInstances(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN BOOL Global)
{
    KSPIN_CINSTANCES * Instances;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    if (Pin->PinId >= Descriptor->Factory.PinDescriptorCount)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    Instances = (KSPIN_CINSTANCES*)Data;

    if (Global)
        Instances->PossibleCount = Descriptor->Factory.Instances[Pin->PinId].MaxGlobalInstanceCount;
    else
        Instances->PossibleCount = Descriptor->Factory.Instances[Pin->PinId].MaxFilterInstanceCount;

    Instances->CurrentCount = Descriptor->Factory.Instances[Pin->PinId].CurrentPinInstanceCount;

    Irp->IoStatus.Information = sizeof(KSPIN_CINSTANCES);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
HandleNecessaryPropertyInstances(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN PSUBDEVICE_DESCRIPTOR Descriptor)
{
    PULONG Result;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    if (Pin->PinId >= Descriptor->Factory.PinDescriptorCount)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    Result = (PULONG)Data;
    *Result = Descriptor->Factory.Instances[Pin->PinId].MinFilterInstanceCount;

    Irp->IoStatus.Information = sizeof(ULONG);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
HandleDataIntersection(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID  Data,
    IN PSUBDEVICE_DESCRIPTOR Descriptor)
{
    IIrpTarget * IrpTarget;
    IPort *Port;
    ISubdevice *SubDevice;
    KSP_PIN * Pin = (KSP_PIN*)Request;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PKSMULTIPLE_ITEM MultipleItem;
    PKSDATARANGE DataRange;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    ULONG Index, Length;

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

    /* Access parameters */
    MultipleItem = (PKSMULTIPLE_ITEM)(Request + 1);
    DataRange = (PKSDATARANGE)(MultipleItem + 1);

    /* Get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        /* Call miniport's properitary handler */
        Status = SubDevice->lpVtbl->DataRangeIntersection(SubDevice, Pin->PinId, DataRange, (PKSDATARANGE)&Descriptor->Factory.KsPinDescriptor[Pin->PinId].DataRanges[0],
                                                          IoStack->Parameters.DeviceIoControl.OutputBufferLength, Data, &Length);

        if (Status == STATUS_SUCCESS)
        {
            Irp->IoStatus.Information = Length;
            break;
        }
        DataRange =  UlongToPtr(PtrToUlong(DataRange) + DataRange->FormatSize);
    }

    /* Release reference */
    Port->lpVtbl->Release(Port);

    Irp->IoStatus.Status = Status;
    return Status;
}


NTSTATUS
NTAPI
PinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PSUBDEVICE_DESCRIPTOR Descriptor;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);
    ASSERT(Descriptor);

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
            Status = HandlePropertyInstances(Irp, Request, Data, Descriptor, TRUE);
            break;
        case KSPROPERTY_PIN_CINSTANCES:
            Status = HandlePropertyInstances(Irp, Request, Data, Descriptor, FALSE);
            break;
        case KSPROPERTY_PIN_NECESSARYINSTANCES:
            Status = HandleNecessaryPropertyInstances(Irp, Request, Data, Descriptor);
            break;

        case KSPROPERTY_PIN_DATAINTERSECTION:
            Status = HandleDataIntersection(Irp, Request, Data, Descriptor);
            break;
        case KSPROPERTY_PIN_PHYSICALCONNECTION:
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
            DPRINT1("Unhandled %x\n", Request->Id);
            Status = STATUS_SUCCESS;
            break;
        default:
            Status = STATUS_NOT_FOUND;
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
NTAPI
PcPropertyHandler(
    IN PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR Descriptor)
{
    ULONG Index, ItemIndex;
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

    for(Index = 0; Index < Descriptor->FilterPropertySet.FreeKsPropertySetOffset; Index++)
    {
        if (IsEqualGUIDAligned(&Property->Set, Descriptor->FilterPropertySet.Properties[Index].Set))
        {
            for(ItemIndex = 0; ItemIndex < Descriptor->FilterPropertySet.Properties[Index].PropertiesCount; ItemIndex++)
            {
                if (Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].PropertyId == Property->Id)
                {
                    if (Property->Flags & KSPROPERTY_TYPE_SET)
                        PropertyHandler = Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].SetPropertyHandler;

                    if (Property->Flags & KSPROPERTY_TYPE_GET)
                        PropertyHandler = Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].GetPropertyHandler;

                    if (Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].MinProperty > IoStack->Parameters.DeviceIoControl.InputBufferLength)
                    {
                        /* too small input buffer */
                        Irp->IoStatus.Information = Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].MinProperty;
                        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return STATUS_BUFFER_TOO_SMALL;
                    }

                    if (Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].MinData > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
                    {
                        /* too small output buffer */
                        Irp->IoStatus.Information = Descriptor->FilterPropertySet.Properties[Index].PropertyItem[ItemIndex].MinData;
                        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return STATUS_BUFFER_TOO_SMALL;
                    }

                    if (PropertyHandler)
                    {
                        KSPROPERTY_ITEM_IRP_STORAGE(Irp) = (PVOID)Descriptor;
                        DPRINT("Calling property handler %p\n", PropertyHandler);
                        Status = PropertyHandler(Irp, Property, Irp->UserBuffer);
                    }

                    /* the information member is set by the handler */
                    Irp->IoStatus.Status = Status;
                    DPRINT("Result %x Length %u\n", Status, Irp->IoStatus.Information);
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return Status;
                }
            }
        }
    }

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT1("Unhandeled property: Set %S Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    RtlFreeUnicodeString(&GuidString);
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}
