#include "priv.h"

KSDDKAPI NTSTATUS NTAPI
KsCreatePin(
    IN  HANDLE FilterHandle,
    IN  PKSPIN_CONNECT Connect,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE ConnectionHandle)
{
    UINT ConnectSize = sizeof(KSPIN_CONNECT);

    PKSDATAFORMAT_WAVEFORMATEX Format = (PKSDATAFORMAT_WAVEFORMATEX)(Connect + 1);
    if (Format->DataFormat.FormatSize == sizeof(KSDATAFORMAT) ||
        Format->DataFormat.FormatSize == sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX))
    {
        ConnectSize += Format->DataFormat.FormatSize;
    }

    return KspCreateObjectType(FilterHandle,
                               L"{146F1A80-4791-11D0-A5D6-28DB04C10000}",
                               (PVOID)Connect,
                               ConnectSize,
                               DesiredAccess,
                               ConnectionHandle);
}

KSDDKAPI NTSTATUS NTAPI
KsValidateConnectRequest(
    IN  PIRP Irp,
    IN  ULONG DescriptorsCount,
    IN  KSPIN_DESCRIPTOR* Descriptor,
    OUT PKSPIN_CONNECT* Connect)
{
    return STATUS_SUCCESS;
}

KSDDKAPI
NTSTATUS
NTAPI
KsPinPropertyHandler(
    IN  PIRP Irp,
    IN  PKSPROPERTY Property,
    IN  OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor)
{
    KSP_PIN * Pin;
    KSMULTIPLE_ITEM * Item;
    PIO_STACK_LOCATION IoStack;
    ULONG Size;
    PVOID Buffer;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Irp->UserBuffer;

    if (Property->Flags != KSPROPERTY_TYPE_GET)
    {
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        Irp->IoStatus.Information = 0;
        return STATUS_NOT_IMPLEMENTED;
    }

    switch(Property->Id)
    {
        case KSPROPERTY_PIN_CTYPES:
            (*(PULONG)Buffer) = DescriptorsCount;
            Irp->IoStatus.Information = sizeof(ULONG);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case KSPROPERTY_PIN_DATAFLOW:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= DescriptorsCount)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }
            Size = sizeof(KSPIN_DATAFLOW);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *((KSPIN_DATAFLOW*)Buffer) = Descriptor[Pin->PinId].DataFlow;
            Irp->IoStatus.Information = sizeof(KSPIN_DATAFLOW);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case KSPROPERTY_PIN_DATARANGES:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= DescriptorsCount)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            Size = sizeof(KSMULTIPLE_ITEM) + sizeof(KSDATARANGE) * Descriptor[Pin->PinId].DataRangesCount;

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Buffer;
            Item->Size = Size;
            Item->Count = Descriptor[Pin->PinId].DataRangesCount;
            RtlMoveMemory((PVOID)(Item + 1), Descriptor[Pin->PinId].DataRanges, Descriptor[Pin->PinId].DataRangesCount * sizeof(KSDATARANGE));

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;
        case KSPROPERTY_PIN_INTERFACES:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= DescriptorsCount)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            Size = sizeof(KSMULTIPLE_ITEM) + sizeof(KSPIN_INTERFACE) * Descriptor[Pin->PinId].InterfacesCount;

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Buffer;
            Item->Size = Size;
            Item->Count = Descriptor[Pin->PinId].InterfacesCount;
            RtlMoveMemory((PVOID)(Item + 1), Descriptor[Pin->PinId].Interfaces, Descriptor[Pin->PinId].InterfacesCount * sizeof(KSDATARANGE));

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;
        case KSPROPERTY_PIN_MEDIUMS:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= DescriptorsCount)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            Size = sizeof(KSMULTIPLE_ITEM) + sizeof(KSPIN_MEDIUM) * Descriptor[Pin->PinId].MediumsCount;
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Buffer;
            Item->Size = Size;
            Item->Count = Descriptor[Pin->PinId].MediumsCount;
            RtlMoveMemory((PVOID)(Item + 1), Descriptor[Pin->PinId].Mediums, Descriptor[Pin->PinId].MediumsCount * sizeof(KSDATARANGE));

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;

        case KSPROPERTY_PIN_COMMUNICATION:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= DescriptorsCount)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            Size = sizeof(KSPIN_COMMUNICATION);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *((KSPIN_COMMUNICATION*)Buffer) = Descriptor[Pin->PinId].Communication;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;

        case KSPROPERTY_PIN_CATEGORY:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= DescriptorsCount)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            Size = sizeof(GUID);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            RtlMoveMemory(Buffer, &Descriptor[Pin->PinId].Category, sizeof(GUID));
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;

        case KSPROPERTY_PIN_NAME:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= DescriptorsCount)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }

            Size = sizeof(GUID);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }


            RtlMoveMemory(Buffer, &Descriptor[Pin->PinId].Name, sizeof(GUID));
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;
        default:
            DPRINT1("Unhandled property request %x\n", Property->Id);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            Irp->IoStatus.Information = 0;
    }

    return Irp->IoStatus.Status;
}

KSDDKAPI NTSTATUS NTAPI
KsPinDataIntersection(
    IN  PIRP Irp,
    IN  PKSPIN Pin,
    OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor,
    IN  PFNKSINTERSECTHANDLER IntersectHandler)
{
    return STATUS_SUCCESS;
}

/* Does this belong here? */

KSDDKAPI NTSTATUS NTAPI
KsHandleSizedListQuery(
    IN  PIRP Irp,
    IN  ULONG DataItemsCount,
    IN  ULONG DataItemSize,
    IN  const VOID* DataItems)
{
    return STATUS_SUCCESS;
}
