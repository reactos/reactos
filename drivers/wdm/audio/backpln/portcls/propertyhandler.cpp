/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/propertyhandler.cpp
 * PURPOSE:         Pin property handler
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

NTSTATUS
HandlePropertyInstances(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN BOOL Global,
    IN ISubdevice *SubDevice)
{
    KSPIN_CINSTANCES * Instances;
    KSP_PIN * Pin = (KSP_PIN*)Request;
    ULONG FilterNecessary, FilterCurrent, FilterPossible, GlobalCurrent, GlobalPossible;
    NTSTATUS Status;

    if (Pin->PinId >= Descriptor->Factory.PinDescriptorCount)
    {
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    Instances = (KSPIN_CINSTANCES*)Data;

    // check if the miniport supports the IPinCount interface
    Status = SubDevice->PinCount(Pin->PinId, &FilterNecessary, &FilterCurrent, &FilterPossible, &GlobalCurrent, &GlobalPossible);

    if (NT_SUCCESS(Status))
    {
         if (Global)
         {
             Instances->PossibleCount = GlobalPossible;
             Instances->CurrentCount = GlobalCurrent;
         }
         else
         {
             Instances->PossibleCount = FilterPossible;
             Instances->CurrentCount = FilterCurrent;
         }
    }
    else
    {
        if (Global)
            Instances->PossibleCount = Descriptor->Factory.Instances[Pin->PinId].MaxGlobalInstanceCount;
        else
            Instances->PossibleCount = Descriptor->Factory.Instances[Pin->PinId].MaxFilterInstanceCount;

        Instances->CurrentCount = Descriptor->Factory.Instances[Pin->PinId].CurrentPinInstanceCount;
    }

    IoStatus->Information = sizeof(KSPIN_CINSTANCES);
    IoStatus->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
HandleNecessaryPropertyInstances(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN ISubdevice *SubDevice)
{
    PULONG Result;
    KSP_PIN * Pin = (KSP_PIN*)Request;
    ULONG FilterNecessary, FilterCurrent, FilterPossible, GlobalCurrent, GlobalPossible;
    NTSTATUS Status;

    if (Pin->PinId >= Descriptor->Factory.PinDescriptorCount)
    {
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    Result = (PULONG)Data;

    // check if the miniport supports the IPinCount interface
    Status = SubDevice->PinCount(Pin->PinId, &FilterNecessary, &FilterCurrent, &FilterPossible, &GlobalCurrent, &GlobalPossible);

    if (NT_SUCCESS(Status))
    {
        *Result = FilterNecessary;
    }
    else
    {
        *Result = Descriptor->Factory.Instances[Pin->PinId].MinFilterInstanceCount;
    }

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

    // Access parameters
    MultipleItem = (PKSMULTIPLE_ITEM)(Pin + 1);
    DataRange = (PKSDATARANGE)(MultipleItem + 1);

    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        // Call miniport's proprietary handler
        PC_ASSERT(Descriptor->Factory.KsPinDescriptor[Pin->PinId].DataRangesCount);
        PC_ASSERT(Descriptor->Factory.KsPinDescriptor[Pin->PinId].DataRanges[0]);
        Status = SubDevice->DataRangeIntersection(Pin->PinId, DataRange, (PKSDATARANGE)Descriptor->Factory.KsPinDescriptor[Pin->PinId].DataRanges[0],
                                                  DataLength, Data, &Length);

        if (Status == STATUS_SUCCESS)
        {
            IoStatus->Information = Length;
            break;
        }
        DataRange = (PKSDATARANGE)((PUCHAR)DataRange + DataRange->FormatSize);
    }

    IoStatus->Status = Status;
    return Status;
}

NTSTATUS
HandlePhysicalConnection(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER Request,
    IN ULONG RequestLength,
    IN OUT PVOID  Data,
    IN ULONG DataLength,
    IN PSUBDEVICE_DESCRIPTOR Descriptor)
{
    PKSP_PIN Pin;
    PLIST_ENTRY Entry;
    //PKSPIN_PHYSICALCONNECTION Connection;
    PPHYSICAL_CONNECTION_ENTRY ConEntry;

    // get pin
    Pin = (PKSP_PIN)Request;

    if (RequestLength < sizeof(KSP_PIN))
    {
        // input buffer must be at least sizeof KSP_PIN
        DPRINT("input length too small\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (IsListEmpty(&Descriptor->PhysicalConnectionList))
    {
        DPRINT("no connection\n");
        return STATUS_NOT_FOUND;
    }

    // get first item
    Entry = Descriptor->PhysicalConnectionList.Flink;

    do
    {
        ConEntry = (PPHYSICAL_CONNECTION_ENTRY)CONTAINING_RECORD(Entry, PHYSICAL_CONNECTION_ENTRY, Entry);

        if (ConEntry->FromPin == Pin->PinId)
        {
            //Connection = (PKSPIN_PHYSICALCONNECTION)Data;
            DPRINT("FoundEntry %S Size %u\n", ConEntry->Connection.SymbolicLinkName, ConEntry->Connection.Size);
            IoStatus->Information = ConEntry->Connection.Size;

            if (!DataLength)
            {
                IoStatus->Information = ConEntry->Connection.Size;
                return STATUS_MORE_ENTRIES;
            }

            if (DataLength < ConEntry->Connection.Size)
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

            RtlMoveMemory(Data, &ConEntry->Connection, ConEntry->Connection.Size);
            return STATUS_SUCCESS;
       }

        // move to next item
        Entry = Entry->Flink;
    }while(Entry != &Descriptor->PhysicalConnectionList);

    IoStatus->Information = 0;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PIO_STACK_LOCATION IoStack;
    //PKSOBJECT_CREATE_ITEM CreateItem;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    IIrpTarget * IrpTarget;
    IPort *Port;
    ISubdevice *SubDevice;
    PDISPATCH_CONTEXT DispatchContext;

    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);
    PC_ASSERT(Descriptor);

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;

    // Get the IrpTarget
    IrpTarget = DispatchContext->Target;
    PC_ASSERT(IrpTarget);

    // Get the parent
    Status = IrpTarget->QueryInterface(IID_IPort, (PVOID*)&Port);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to obtain IPort interface from filter\n");
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    // Get private ISubdevice interface
    Status = Port->QueryInterface(IID_ISubdevice, (PVOID*)&SubDevice);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to obtain ISubdevice interface from port driver\n");
        DbgBreakPoint();
        while(TRUE);
    }

    // get current stack location
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
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
            Status = KsPinPropertyHandler(Irp, Request, Data, Descriptor->Factory.PinDescriptorCount, Descriptor->Factory.KsPinDescriptor);
            break;
        case KSPROPERTY_PIN_GLOBALCINSTANCES:
            Status = HandlePropertyInstances(&Irp->IoStatus, Request, Data, Descriptor, TRUE, SubDevice);
            break;
        case KSPROPERTY_PIN_CINSTANCES:
            Status = HandlePropertyInstances(&Irp->IoStatus, Request, Data, Descriptor, FALSE, SubDevice);
            break;
        case KSPROPERTY_PIN_NECESSARYINSTANCES:
            Status = HandleNecessaryPropertyInstances(&Irp->IoStatus, Request, Data, Descriptor, SubDevice);
            break;

        case KSPROPERTY_PIN_DATAINTERSECTION:
            Status = HandleDataIntersection(&Irp->IoStatus, Request, Data, IoStack->Parameters.DeviceIoControl.OutputBufferLength, Descriptor, SubDevice);
            break;
        case KSPROPERTY_PIN_PHYSICALCONNECTION:
            Status = HandlePhysicalConnection(&Irp->IoStatus, Request, IoStack->Parameters.DeviceIoControl.InputBufferLength, Data, IoStack->Parameters.DeviceIoControl.OutputBufferLength, Descriptor);
            break;
        default:
            UNIMPLEMENTED;
            Status = STATUS_UNSUCCESSFUL;
    }

    // Release reference
    Port->Release();

    // Release subdevice reference
    SubDevice->Release();

    return Status;
}

NTSTATUS
NTAPI
TopologyPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PSUBDEVICE_DESCRIPTOR Descriptor;

    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    return KsTopologyPropertyHandler(Irp, Request, Data, Descriptor->Topology);
}
