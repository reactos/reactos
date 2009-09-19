/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/connectivity.c
 * PURPOSE:         KS Pin functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

KSPIN_INTERFACE StandardPinInterface = 
{
    {STATIC_KSINTERFACESETID_Standard},
    KSINTERFACE_STANDARD_STREAMING,
    0
};

KSPIN_MEDIUM StandardPinMedium =
{
    {STATIC_KSMEDIUMSETID_Standard},
    KSMEDIUM_TYPE_ANYINSTANCE,
    0
};


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
                               KSSTRING_Pin,
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
    PKSPIN_CONNECT ConnectDetails;
    PKSPIN_INTERFACE Interface;
    PKSPIN_MEDIUM Medium;
    ULONG Size;
    NTSTATUS Status;
    ULONG Index;
    ULONG Count;
    BOOLEAN Found;

    /* did the caller miss the connect parameter */
    if (!Connect)
        return STATUS_INVALID_PARAMETER;

    /* set create param  size */
    Size = sizeof(KSPIN_CONNECT);

    /* fetch create parameters */
    Status = KspCopyCreateRequest(Irp,
                                  KSSTRING_Pin,
                                  &Size,
                                  (PVOID*)&ConnectDetails);

    /* check for success */
    if (!NT_SUCCESS(Status))
        return Status;

    /* is pin id out of bounds */
    if (ConnectDetails->PinId >= DescriptorsCount)
        return STATUS_INVALID_PARAMETER;

    /* does the pin have interface details filled in */
    if (Descriptor[ConnectDetails->PinId].InterfacesCount && Descriptor[ConnectDetails->PinId].Interfaces)
    {
        /* use provided pin interface count */
        Count = Descriptor[ConnectDetails->PinId].InterfacesCount;
        Interface = (PKSPIN_INTERFACE)Descriptor[ConnectDetails->PinId].Interfaces;
    }
    else
    {
        /* use standard pin interface */
        Count = 1;
        Interface = &StandardPinInterface;
    }

    /* now check the interface */
    Found = FALSE;
    Index = 0;
    do
    {
        if (IsEqualGUIDAligned(&Interface[Index].Set, &ConnectDetails->Interface.Set) &&
                               Interface[Index].Id == ConnectDetails->Interface.Id)
        {
            /* found a matching interface */
            Found = TRUE;
            break;
        }
        /* iterate to next interface */
        Index++;
    }while(Index < Count);

    if (!Found)
    {
        /* pin doesnt support this interface */
        return STATUS_NO_MATCH;
    }

    /* does the pin have medium details filled in */
    if (Descriptor[ConnectDetails->PinId].MediumsCount && Descriptor[ConnectDetails->PinId].Mediums)
    {
        /* use provided pin interface count */
        Count = Descriptor[ConnectDetails->PinId].MediumsCount;
        Medium = (PKSPIN_MEDIUM)Descriptor[ConnectDetails->PinId].Mediums;
    }
    else
    {
        /* use standard pin interface */
        Count = 1;
        Medium = &StandardPinMedium;
    }

    /* now check the interface */
    Found = FALSE;
    Index = 0;
    do
    {
        if (IsEqualGUIDAligned(&Medium[Index].Set, &ConnectDetails->Medium.Set) &&
                               Medium[Index].Id == ConnectDetails->Medium.Id)
        {
            /* found a matching interface */
            Found = TRUE;
            break;
        }
        /* iterate to next medium */
        Index++;
    }while(Index < Count);

    if (!Found)
    {
        /* pin doesnt support this medium */
        return STATUS_NO_MATCH;
    }

    /// FIXME
    /// implement format checking

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
    PULONG GuidBuffer;
    static WCHAR Speaker[] = {L"PC-Speaker"};

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
                Irp->IoStatus.Status = STATUS_MORE_ENTRIES;
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

            /* calculate size */
            Size = sizeof(KSMULTIPLE_ITEM);
            Size += max(1, Descriptor[Pin->PinId].InterfacesCount) * sizeof(KSPIN_INTERFACE);

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_MORE_ENTRIES;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Buffer;
            Item->Size = Size;

            if (Descriptor[Pin->PinId].InterfacesCount)
            {
                Item->Count = Descriptor[Pin->PinId].InterfacesCount;
                RtlMoveMemory((PVOID)(Item + 1), Descriptor[Pin->PinId].Interfaces, Descriptor[Pin->PinId].InterfacesCount * sizeof(KSPIN_INTERFACE));
            }
            else
            {
                Item->Count = 1;
                RtlMoveMemory((PVOID)(Item + 1), &StandardPinInterface, sizeof(KSPIN_INTERFACE));
            }

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

            /* calculate size */
            Size = sizeof(KSMULTIPLE_ITEM);
            Size += max(1, Descriptor[Pin->PinId].MediumsCount) * sizeof(KSPIN_MEDIUM);

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_MORE_ENTRIES;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Buffer;
            Item->Size = Size;

            if (Descriptor[Pin->PinId].MediumsCount)
            {
                Item->Count = Descriptor[Pin->PinId].MediumsCount;
                RtlMoveMemory((PVOID)(Item + 1), Descriptor[Pin->PinId].Mediums, Descriptor[Pin->PinId].MediumsCount * sizeof(KSPIN_MEDIUM));
            }
            else
            {
                Item->Count = 1;
                RtlMoveMemory((PVOID)(Item + 1), &StandardPinMedium, sizeof(KSPIN_MEDIUM));
            }

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

            GuidBuffer = Buffer;
            Size = sizeof(Speaker);

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Irp->IoStatus.Status = STATUS_MORE_ENTRIES;
                break;
            }

            RtlMoveMemory(GuidBuffer, Speaker, sizeof(Speaker));

            //RtlMoveMemory(Buffer, &Descriptor[Pin->PinId].Name, sizeof(GUID));
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

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* calculate minimum data size */
    Size = sizeof(KSP_PIN) + sizeof(KSMULTIPLE_ITEM) + sizeof(KSDATARANGE);
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < Size)
    {
        /* buffer too small */
        Irp->IoStatus.Information = Size;
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return STATUS_BUFFER_TOO_SMALL;
    }
    /* is pin id out of bounds */
    if (Pin->PinId >= DescriptorsCount)
    {
        /* it is */
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    /* get start item */
    Item = (KSMULTIPLE_ITEM*)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    /* get first data range */
    DataRange = (KSDATARANGE*)(Item + 1);
    /* iterate through all data ranges */
    for(Index = 0; Index < Item->Count; Index++, DataRange++)
    {
        /* call intersect handler */
        Status = IntersectHandler(Irp, Pin, DataRange, Data);
        if (NT_SUCCESS(Status))
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < DataRange->FormatSize)
            {
                /* buffer is too small */
                Irp->IoStatus.Information = DataRange->FormatSize;
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

/*
    @implemented
*/

KSDDKAPI
NTSTATUS
NTAPI
KsHandleSizedListQuery(
    IN  PIRP Irp,
    IN  ULONG DataItemsCount,
    IN  ULONG DataItemSize,
    IN  const VOID* DataItems)
{
    ULONG Size;
    PIO_STACK_LOCATION IoStack;
    PKSMULTIPLE_ITEM Item;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Size = DataItemSize * DataItemsCount + sizeof(KSMULTIPLE_ITEM);


    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < Size)
    {
        /* buffer too small */
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Information = Size;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* get multiple item */
    Item = (PKSMULTIPLE_ITEM)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    Item->Count = DataItemsCount;
    Item->Size = DataItemSize;
    /* copy items */
    RtlMoveMemory((PVOID)(Item + 1), DataItems, DataItemSize * DataItemsCount);
    /* store result */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = Size;
    /* done */
    return STATUS_SUCCESS;
}

