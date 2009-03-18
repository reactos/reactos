#include "priv.h"

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
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
                               L"{146F1A80-4791-11D0-A5D6-28DB04C10000}", //KSNAME_Pin
                               (PVOID)Connect,
                               ConnectSize,
                               DesiredAccess,
                               ConnectionHandle);
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsValidateConnectRequest(
    IN  PIRP Irp,
    IN  ULONG DescriptorsCount,
    IN  KSPIN_DESCRIPTOR* Descriptor,
    OUT PKSPIN_CONNECT* Connect)
{
    PIO_STACK_LOCATION IoStack;
    PKSPIN_CONNECT ConnectDetails;
    LPWSTR PinName = L"{146F1A80-4791-11D0-A5D6-28DB04C10000}\\";
    PKSDATAFORMAT DataFormat;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (!IoStack->FileObject->FileName.Buffer)
        return STATUS_INVALID_PARAMETER;

    if (wcsncmp(IoStack->FileObject->FileName.Buffer, PinName, wcslen(PinName)))
        return STATUS_INVALID_PARAMETER;

    ConnectDetails = (PKSPIN_CONNECT)(IoStack->FileObject->FileName.Buffer + wcslen(PinName));

    if (ConnectDetails->PinToHandle != NULL)
    {
        UNIMPLEMENTED
        return STATUS_NOT_IMPLEMENTED;
    }

    if (IoStack->FileObject->FileName.Length < wcslen(PinName) + sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT))
        return STATUS_INVALID_PARAMETER;

    ConnectDetails = (PKSPIN_CONNECT)(IoStack->FileObject->FileName.Buffer + wcslen(PinName));

    if (ConnectDetails->PinId >= DescriptorsCount)
        return STATUS_INVALID_PARAMETER;

#if 0
    if (!IsEqualGUIDAligned(&ConnectDetails->Interface.Set, &KSINTERFACESETID_Standard) &&
         ConnectDetails->Interface.Id != KSINTERFACE_STANDARD_STREAMING)
    {
         //FIXME
         // validate provided interface set
         DPRINT1("FIXME\n");
    }

    if (!IsEqualGUIDAligned(&ConnectDetails->Medium.Set, &KSMEDIUMSETID_Standard) &&
         ConnectDetails->Medium.Id != KSMEDIUM_TYPE_ANYINSTANCE)
    {
         //FIXME
         // validate provided medium set
         DPRINT1("FIXME\n");
    }
#endif

    /// FIXME
    /// implement format checking

    DataFormat = (PKSDATAFORMAT) (ConnectDetails + 1);
    *Connect = ConnectDetails;

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
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
    ULONG Size, Index;
    PVOID Buffer;
    PKSDATARANGE_AUDIO *WaveFormatOut;
    PKSDATAFORMAT_WAVEFORMATEX WaveFormatIn;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Irp->UserBuffer;

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
            Size = sizeof(KSMULTIPLE_ITEM);
            for (Index = 0; Index < Descriptor[Pin->PinId].DataRangesCount; Index++)
            {
                Size += Descriptor[Pin->PinId].DataRanges[Index]->FormatSize;
            }

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Buffer;
            Item->Size = Size;
            Item->Count = Descriptor[Pin->PinId].DataRangesCount;

            Data = (PUCHAR)(Item +1);
            for (Index = 0; Index < Descriptor[Pin->PinId].DataRangesCount; Index++)
            {
                RtlMoveMemory(Data, Descriptor[Pin->PinId].DataRanges[Index], Descriptor[Pin->PinId].DataRanges[Index]->FormatSize);
                Data = ((PUCHAR)Data + Descriptor[Pin->PinId].DataRanges[Index]->FormatSize);
            }

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
        case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
            Pin = (KSP_PIN*)Property;
            if (Pin->PinId >= DescriptorsCount)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                break;
            }
            Size = sizeof(KSDATAFORMAT);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(KSDATAFORMAT_WAVEFORMATEX))
            {
                UNIMPLEMENTED
                Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                Irp->IoStatus.Information = 0;
                return STATUS_NOT_IMPLEMENTED;
            }

            WaveFormatIn = (PKSDATAFORMAT_WAVEFORMATEX)Buffer;
            if (!Descriptor[Pin->PinId].DataRanges || !Descriptor[Pin->PinId].DataRangesCount)
            {
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;
                return STATUS_UNSUCCESSFUL;
            }
            WaveFormatOut = (PKSDATARANGE_AUDIO*)Descriptor[Pin->PinId].DataRanges;
            for(Index = 0; Index < Descriptor[Pin->PinId].DataRangesCount; Index++)
            {
                if (WaveFormatOut[Index]->DataRange.FormatSize != sizeof(KSDATARANGE_AUDIO))
                {
                    UNIMPLEMENTED
                    continue;
                }

                if (WaveFormatOut[Index]->MinimumSampleFrequency > WaveFormatIn->WaveFormatEx.nSamplesPerSec ||
                    WaveFormatOut[Index]->MaximumSampleFrequency < WaveFormatIn->WaveFormatEx.nSamplesPerSec ||
                    WaveFormatOut[Index]->MinimumBitsPerSample > WaveFormatIn->WaveFormatEx.wBitsPerSample ||
                    WaveFormatOut[Index]->MaximumBitsPerSample < WaveFormatIn->WaveFormatEx.wBitsPerSample ||
                    WaveFormatOut[Index]->MaximumChannels < WaveFormatIn->WaveFormatEx.nChannels)
                {
                    Irp->IoStatus.Status = STATUS_NO_MATCH;
                    Irp->IoStatus.Information = 0;
                    return STATUS_NO_MATCH;
                }
                else
                {
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = 0;
                    return STATUS_SUCCESS;
                }
            }
            Irp->IoStatus.Status = STATUS_NO_MATCH;
            Irp->IoStatus.Information = 0;
            return STATUS_NO_MATCH;
        default:
            DPRINT1("Unhandled property request %x\n", Property->Id);
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            Irp->IoStatus.Information = 0;
    }

    return Irp->IoStatus.Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsPinDataIntersection(
    IN  PIRP Irp,
    IN  PKSP_PIN Pin,
    OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor,
    IN  PFNKSINTERSECTHANDLER IntersectHandler)
{
    KSMULTIPLE_ITEM * Item;
    KSDATARANGE * DataRange;
    PIO_STACK_LOCATION IoStack;
    ULONG Size;
    ULONG Index;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Size = sizeof(KSP_PIN) + sizeof(KSMULTIPLE_ITEM) + sizeof(KSDATARANGE);
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < Size)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Pin->PinId >= DescriptorsCount)
    {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    Item = (KSMULTIPLE_ITEM*)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    DataRange = (KSDATARANGE*)(Item + 1);

    for(Index = 0; Index < Item->Count; Index++, DataRange++)
    {
        Status = IntersectHandler(Irp, Pin, DataRange, Data);
        if (NT_SUCCESS(Status))
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSDATARANGE))
            {
                Irp->IoStatus.Information = sizeof(KSDATARANGE);
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                return STATUS_BUFFER_TOO_SMALL;
            }
            RtlMoveMemory(Irp->UserBuffer, DataRange, sizeof(KSDATARANGE));
            Irp->IoStatus.Information = sizeof(KSDATARANGE);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            return STATUS_SUCCESS;
        }

    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_NO_MATCH;
    return STATUS_NO_MATCH;
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
