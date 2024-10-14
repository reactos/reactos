/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/connectivity.c
 * PURPOSE:         KS Pin functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

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

const GUID KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT = {0xf4aeb342, 0x0329, 0x4fdd, {0xa8, 0xfd, 0x4a, 0xff, 0x49, 0x26, 0xc9, 0x78}};

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

NTSTATUS
KspValidateConnectRequest(
    IN PIRP Irp,
    IN ULONG DescriptorsCount,
    IN PVOID Descriptors,
    IN ULONG DescriptorSize,
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
    PKSPIN_DESCRIPTOR Descriptor;
    UNICODE_STRING GuidString2;

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
    {
        FreeItem(ConnectDetails);
        return STATUS_INVALID_PARAMETER;
    }

    if (DescriptorSize == sizeof(KSPIN_DESCRIPTOR))
    {
        /* standard pin descriptor */
        Descriptor = (PKSPIN_DESCRIPTOR)((ULONG_PTR)Descriptors + sizeof(KSPIN_DESCRIPTOR) * ConnectDetails->PinId);
    }
    else
    {
        /* extended / variable pin descriptor */
        Descriptor = &((PKSPIN_DESCRIPTOR_EX)((ULONG_PTR)Descriptors + DescriptorSize * ConnectDetails->PinId))->PinDescriptor;
    }


    /* does the pin have interface details filled in */
    if (Descriptor->InterfacesCount && Descriptor->Interfaces)
    {
        /* use provided pin interface count */
        Count = Descriptor->InterfacesCount;
        Interface = (PKSPIN_INTERFACE)Descriptor->Interfaces;
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
    RtlStringFromGUID(&ConnectDetails->Interface.Set, &GuidString2);
    do
    {
        UNICODE_STRING GuidString;
        RtlStringFromGUID(&Interface[Index].Set, &GuidString);

        DPRINT("Driver Interface %S Id %u\n", GuidString.Buffer, Interface[Index].Id);
        DPRINT("Connect Interface %S Id %u\n", GuidString2.Buffer, ConnectDetails->Interface.Id);

        RtlFreeUnicodeString(&GuidString);

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
    RtlFreeUnicodeString(&GuidString2);

    if (!Found)
    {
        /* pin doesnt support this interface */
        FreeItem(ConnectDetails);
        return STATUS_NO_MATCH;
    }

    /* does the pin have medium details filled in */
    if (Descriptor->MediumsCount && Descriptor->Mediums)
    {
        /* use provided pin interface count */
        Count = Descriptor->MediumsCount;
        Medium = (PKSPIN_MEDIUM)Descriptor->Mediums;
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
    RtlStringFromGUID(&ConnectDetails->Medium.Set, &GuidString2);
    do
    {
        UNICODE_STRING GuidString;
        RtlStringFromGUID(&Medium[Index].Set, &GuidString);

        DPRINT("Driver Medium %S Id %u\n", GuidString.Buffer, Medium[Index].Id);
        DPRINT("Connect Medium %S Id %u\n", GuidString2.Buffer, ConnectDetails->Medium.Id);

        RtlFreeUnicodeString(&GuidString);

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
    RtlFreeUnicodeString(&GuidString2);

    if (!Found)
    {
        /* pin doesnt support this medium */
        FreeItem(ConnectDetails);
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
KsValidateConnectRequest(
    IN  PIRP Irp,
    IN  ULONG DescriptorsCount,
    IN  KSPIN_DESCRIPTOR* Descriptor,
    OUT PKSPIN_CONNECT* Connect)
{
    return KspValidateConnectRequest(Irp, DescriptorsCount, Descriptor, sizeof(KSPIN_DESCRIPTOR), Connect);
}

NTSTATUS
KspReadMediaCategory(
    IN LPGUID Category,
    PKEY_VALUE_PARTIAL_INFORMATION *OutInformation)
{
    UNICODE_STRING MediaPath = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\MediaCategories\\");
    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"Name");
    UNICODE_STRING GuidString, Path;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey;
    ULONG Size;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;

    /* convert the guid to string */
    Status = RtlStringFromGUID(Category, &GuidString);
    if (!NT_SUCCESS(Status))
        return Status;

    /* allocate buffer for the registry key */
    Path.Length = 0;
    Path.MaximumLength = MediaPath.MaximumLength + GuidString.MaximumLength;
    Path.Buffer = AllocateItem(NonPagedPool, Path.MaximumLength);
    if (!Path.Buffer)
    {
        /* not enough memory */
        RtlFreeUnicodeString(&GuidString);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeStringToString(&Path, &MediaPath);
    RtlAppendUnicodeStringToString(&Path, &GuidString);

    /* free guid string */
    RtlFreeUnicodeString(&GuidString);

    /* initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes, &Path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    /* open the key */
    Status = ZwOpenKey(&hKey, GENERIC_READ, &ObjectAttributes);

    DPRINT("ZwOpenKey() status 0x%08lx %wZ\n", Status, &Path);

    /* free path buffer */
    FreeItem(Path.Buffer);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwOpenKey() failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* query the name size */
    Status = ZwQueryValueKey(hKey, &Name, KeyValuePartialInformation, NULL, 0, &Size);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
    {
        /* failed to query for name key */
        ZwClose(hKey);
        return Status;
    }

    /* allocate buffer to read key info */
    KeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION) AllocateItem(NonPagedPool, Size);
    if (!KeyInfo)
    {
        /* not enough memory */
        ZwClose(hKey);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* now read the info */
    Status = ZwQueryValueKey(hKey, &Name, KeyValuePartialInformation, (PVOID)KeyInfo, Size, &Size);

    /* close the key */
    ZwClose(hKey);

    if (!NT_SUCCESS(Status))
    {
        /* failed to read key */
        FreeItem(KeyInfo);
        return Status;
    }

    /* store key information */
    *OutInformation = KeyInfo;
    return Status;
}

KSDDKAPI
NTSTATUS
NTAPI
KspPinPropertyHandler(
    IN  PIRP Irp,
    IN  PKSPROPERTY Property,
    IN  OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptors,
    IN  ULONG DescriptorSize)
{
    KSP_PIN * Pin;
    KSMULTIPLE_ITEM * Item;
    PIO_STACK_LOCATION IoStack;
    ULONG Size, Index;
    PVOID Buffer;
    PKSDATARANGE_AUDIO *WaveFormatOut;
    PKSDATAFORMAT_WAVEFORMATEX WaveFormatIn;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    const KSPIN_DESCRIPTOR *Descriptor;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    ULONG Count;
    const PKSDATARANGE* DataRanges;
    LPGUID Guid;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Data;

    //DPRINT("KsPinPropertyHandler Irp %p Property %p Data %p DescriptorsCount %u Descriptor %p OutputLength %u Id %u\n", Irp, Property, Data, DescriptorsCount, Descriptor, IoStack->Parameters.DeviceIoControl.OutputBufferLength, Property->Id);

    /* convert to PKSP_PIN */
    Pin = (KSP_PIN*)Property;

    if (Property->Id != KSPROPERTY_PIN_CTYPES)
    {
        if (Pin->PinId >= DescriptorsCount)
        {
            /* invalid parameter */
            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        (*(PULONG)Buffer) = DescriptorsCount;
        Irp->IoStatus.Information = sizeof(ULONG);
        return STATUS_SUCCESS;
    }


    if (DescriptorSize == sizeof(KSPIN_DESCRIPTOR))
    {
        /* it is simple pin descriptor */
        Descriptor = &Descriptors[Pin->PinId];
    }
    else
    {
        /* get offset to pin descriptor */
        Descriptor = &(((PKSPIN_DESCRIPTOR_EX)((ULONG_PTR)Descriptors + Pin->PinId * DescriptorSize))->PinDescriptor);
    }

    switch(Property->Id)
    {
        case KSPROPERTY_PIN_DATAFLOW:

            Size = sizeof(KSPIN_DATAFLOW);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *((KSPIN_DATAFLOW*)Buffer) = Descriptor->DataFlow;
            Irp->IoStatus.Information = sizeof(KSPIN_DATAFLOW);
            Status = STATUS_SUCCESS;
            break;

        case KSPROPERTY_PIN_DATARANGES:
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:

            Size = sizeof(KSMULTIPLE_ITEM);
            DPRINT("Id %lu PinId %lu DataRangesCount %lu ConstrainedDataRangesCount %lu\n", Property->Id, Pin->PinId, Descriptor->DataRangesCount, Descriptor->ConstrainedDataRangesCount);

            if (Property->Id == KSPROPERTY_PIN_DATARANGES || Descriptor->ConstrainedDataRangesCount == 0)
            {
                DataRanges = Descriptor->DataRanges;
                Count = Descriptor->DataRangesCount;
            }
            else
            {
                DataRanges = Descriptor->ConstrainedDataRanges;
                Count = Descriptor->ConstrainedDataRangesCount;
            }

            for (Index = 0; Index < Count; Index++)
            {
                Size += ((DataRanges[Index]->FormatSize + 0x7) & ~0x7);
            }

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == 0)
            {
                /* buffer too small */
                Irp->IoStatus.Information = Size;
                Status = STATUS_BUFFER_OVERFLOW;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Buffer;

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(ULONG))
            {
                /* store the result size */
                Item->Size = Size;
                Irp->IoStatus.Information = sizeof(ULONG);
                Status = STATUS_SUCCESS;
                break;
            }

            /* store descriptor size */
            Item->Size = Size;
            Item->Count = Count;

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(KSMULTIPLE_ITEM))
            {
                Irp->IoStatus.Information = sizeof(KSMULTIPLE_ITEM);
                Status = STATUS_SUCCESS;
                break;
            }

            /* now copy all dataranges */
            Data = (PUCHAR)(Item +1);

            /* alignment assert */
            ASSERT(((ULONG_PTR)Data & 0x7) == 0);

            for (Index = 0; Index < Count; Index++)
            {
                UNICODE_STRING GuidString;
                /* convert the guid to string */
                RtlStringFromGUID(&DataRanges[Index]->MajorFormat, &GuidString);
                DPRINT("Index %lu MajorFormat %S\n", Index, GuidString.Buffer);
                RtlStringFromGUID(&DataRanges[Index]->SubFormat, &GuidString);
                DPRINT("Index %lu SubFormat %S\n", Index, GuidString.Buffer);
                RtlStringFromGUID(&DataRanges[Index]->Specifier, &GuidString);
                DPRINT("Index %lu Specifier %S\n", Index, GuidString.Buffer);
                RtlStringFromGUID(&DataRanges[Index]->Specifier, &GuidString);
                DPRINT("Index %lu FormatSize %lu Flags %lu SampleSize %lu Reserved %lu KSDATAFORMAT %lu\n", Index,
                       DataRanges[Index]->FormatSize, DataRanges[Index]->Flags, DataRanges[Index]->SampleSize, DataRanges[Index]->Reserved, sizeof(KSDATAFORMAT));

                RtlMoveMemory(Data, DataRanges[Index], DataRanges[Index]->FormatSize);
                Data = ((PUCHAR)Data + DataRanges[Index]->FormatSize);
                /* alignment assert */
                ASSERT(((ULONG_PTR)Data & 0x7) == 0);
                Data = (PVOID)(((ULONG_PTR)Data + 0x7) & ~0x7);
            }

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;
        case KSPROPERTY_PIN_INTERFACES:

            if (Descriptor->Interfaces)
            {
                /* use mediums provided by driver */
                return KsHandleSizedListQuery(Irp, Descriptor->InterfacesCount, sizeof(KSPIN_MEDIUM), Descriptor->Interfaces);
            }
            else
            {
                /* use standard medium */
                return KsHandleSizedListQuery(Irp, 1, sizeof(KSPIN_INTERFACE), &StandardPinInterface);
            }
            break;

        case KSPROPERTY_PIN_MEDIUMS:

            if (Descriptor->MediumsCount)
            {
                /* use mediums provided by driver */
                return KsHandleSizedListQuery(Irp, Descriptor->MediumsCount, sizeof(KSPIN_MEDIUM), Descriptor->Mediums);
            }
            else
            {
                /* use standard medium */
                return KsHandleSizedListQuery(Irp, 1, sizeof(KSPIN_MEDIUM), &StandardPinMedium);
            }
            break;

        case KSPROPERTY_PIN_COMMUNICATION:

            Size = sizeof(KSPIN_COMMUNICATION);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *((KSPIN_COMMUNICATION*)Buffer) = Descriptor->Communication;

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;

        case KSPROPERTY_PIN_CATEGORY:

            if (!Descriptor->Category)
            {
                /* no pin category */
                return STATUS_NOT_FOUND;
            }

            /* check size */
            Size = sizeof(GUID);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                /* buffer too small */
                Irp->IoStatus.Information = Size;
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* copy category guid */
            RtlMoveMemory(Buffer, Descriptor->Category, sizeof(GUID));

            /* save result */
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Size;
            break;

        case KSPROPERTY_PIN_NAME:

            if (Descriptor->Name)
            {
                /* use pin name */
                Guid = (LPGUID)Descriptor->Name;
            }
            else
            {
                /* use pin category as fallback */
                Guid = (LPGUID)Descriptor->Category;
            }

            if (!Guid)
            {
                /* no friendly name available */
                return STATUS_NOT_FOUND;
            }

            /* read friendly name category name */
            Status = KspReadMediaCategory(Guid, &KeyInfo);
            if (!NT_SUCCESS(Status))
            {
                /* failed to read category */
                Irp->IoStatus.Information = 0;
                break;
            }

            /* store required length */
            Irp->IoStatus.Information = KeyInfo->DataLength + sizeof(WCHAR);

            /* check if buffer is too small */
            if (KeyInfo->DataLength + sizeof(WCHAR) > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
            {
                /* buffer too small */
                Status = STATUS_BUFFER_OVERFLOW;
                FreeItem(KeyInfo);
                break;
            }

            /* copy result */
            RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer, &KeyInfo->Data, KeyInfo->DataLength);

            /* null terminate name */
            ((LPWSTR)Irp->AssociatedIrp.SystemBuffer)[KeyInfo->DataLength / sizeof(WCHAR)] = L'\0';

            /* free key info */
            FreeItem(KeyInfo);
            break;
        case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
            Size = sizeof(KSDATAFORMAT);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(KSDATAFORMAT_WAVEFORMATEX))
            {
                UNIMPLEMENTED;
                Status = STATUS_NOT_IMPLEMENTED;
                Irp->IoStatus.Information = 0;
                break;
            }

            WaveFormatIn = (PKSDATAFORMAT_WAVEFORMATEX)Buffer;
            if (!Descriptor->DataRanges || !Descriptor->DataRangesCount)
            {
                Status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;
                break;
            }
            WaveFormatOut = (PKSDATARANGE_AUDIO*)Descriptor->DataRanges;
            for(Index = 0; Index < Descriptor->DataRangesCount; Index++)
            {
                if (WaveFormatOut[Index]->DataRange.FormatSize != sizeof(KSDATARANGE_AUDIO))
                {
                    UNIMPLEMENTED;
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
            Status = STATUS_NO_MATCH;
            Irp->IoStatus.Information = 0;
            break;
        default:
            DPRINT1("Unhandled property request %x\n", Property->Id);
            Status = STATUS_NOT_IMPLEMENTED;
            Irp->IoStatus.Information = 0;
    }

    return Status;
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
    return KspPinPropertyHandler(Irp, Property, Data, DescriptorsCount, Descriptor, sizeof(KSPIN_DESCRIPTOR));
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsPinDataIntersectionEx(
    IN  PIRP Irp,
    IN  PKSP_PIN Pin,
    OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor,
    IN  ULONG DescriptorSize,
    IN  PFNKSINTERSECTHANDLEREX IntersectHandler OPTIONAL,
    IN  PVOID HandlerContext OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
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
    Item = (KSMULTIPLE_ITEM*)(Pin + 1);
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

    /* calculate size */
    Size = DataItemSize * DataItemsCount + sizeof(KSMULTIPLE_ITEM);

    /* get multiple item */
    Item = (PKSMULTIPLE_ITEM)Irp->AssociatedIrp.SystemBuffer;

    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == 0)
    {
        /* buffer too small */
        Irp->IoStatus.Information = Size;

        return STATUS_BUFFER_OVERFLOW;
    }

    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(ULONG))
    {
        /* store just the size */
        Item->Size = Size;
        Irp->IoStatus.Information = sizeof(ULONG);

        return STATUS_SUCCESS;
    }


    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSMULTIPLE_ITEM))
    {
        /* buffer too small */
        return STATUS_BUFFER_TOO_SMALL;
    }

    Item->Count = DataItemsCount;
    Item->Size = DataItemSize;

    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(KSMULTIPLE_ITEM))
    {
        /* buffer can only hold the length descriptor */
        Irp->IoStatus.Information = sizeof(KSMULTIPLE_ITEM);
        return STATUS_SUCCESS;
    }

    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength >= Size)
    {
        /* copy items */
        RtlMoveMemory((PVOID)(Item + 1), DataItems, DataItemSize * DataItemsCount);
        /* store result */
        Irp->IoStatus.Information = Size;
        /* done */
        return STATUS_SUCCESS;
    }
    else
    {
        /* buffer too small */
        return STATUS_BUFFER_TOO_SMALL;
    }
}
