/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Common adapter
 * COPYRIGHT:       Copyright 2025-2026 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

PVOID
__cdecl
operator new(size_t Size, POOL_TYPE PoolType, ULONG Tag)
{
    PVOID P = ExAllocatePoolWithTag(PoolType, Size, Tag);
    if (P)
        RtlZeroMemory(P, Size);
    return P;
}

void __cdecl
operator delete(PVOID ptr)
{
    if (ptr)
        ExFreePool(ptr);
}

void __cdecl
operator delete(PVOID ptr, UINT_PTR Unused)
{
    if (ptr)
        ExFreePool(ptr);
}

NTSTATUS
NTAPI
CAdapterCommon::GetInterface(PHDAUDIO_BUS_INTERFACE_V2 iface)
{
    RtlCopyMemory(iface, &Interface, sizeof(HDAUDIO_BUS_INTERFACE_V2));
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::QueryInterface(_In_ REFIID refiid, _Out_ PVOID *Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IAdapterPowerManagement) ||
        IsEqualGUIDAligned(refiid, IID_IPinCount) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PADAPTERPOWERMANAGEMENT(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT("CAdapterCommon::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

//----------------------------------------------------------------------------

VOID NTAPI
CAdapterCommon::PowerChangeState(_In_ POWER_STATE NewState)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
CAdapterCommon::QueryDeviceCapabilities(_In_ PDEVICE_CAPABILITIES PowerDeviceCaps)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::QueryPowerChangeState(_In_ POWER_STATE NewStateQuery)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------------

NTSTATUS
NTAPI
CAdapterCommon::AcquireBusInterface(_In_ PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION Stack;

    DPRINT("AcquireBusInterface\n");
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (Irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.InterfaceType = &GUID_HDAUDIO_BUS_INTERFACE_V2;
    Stack->Parameters.QueryInterface.Size = sizeof(HDAUDIO_BUS_INTERFACE_V2);
    Stack->Parameters.QueryInterface.Version = 0x0100;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)&Interface;
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Status = PcForwardIrpSynchronous(DeviceObject, Irp);
    IoFreeIrp(Irp);
    return Status;
}

VOID NTAPI
CAdapterCommon::GetResourceInformation()
{
    Interface.GetResourceInformation(Interface.Context, &m_CodecAddress, &m_FunctionGroupStartNode);
}

NTSTATUS
NTAPI
CAdapterCommon::GetHardwareIds()
{
    HDAUDIO_CODEC_TRANSFER Verbs[2];
    UINT32 cmdTmpl = (m_CodecAddress << 28) | (AC_NODE_ROOT << 20) | (AC_VERB_PARAMETERS << 8);
    NTSTATUS Status;

    Verbs[0].Output.Command = cmdTmpl | AC_PAR_VENDOR_ID;
    Verbs[1].Output.Command = cmdTmpl | AC_PAR_REV_ID;

    Status = Interface.TransferCodecVerbs(Interface.Context, 2, Verbs, NULL, NULL);
    if (NT_SUCCESS(Status))
    {
        VendorId = (Verbs[0].Input.Response >> 16) & 0xFFFF;
        DeviceId = (Verbs[0].Input.Response & 0xFFFF);
        RevisionId = (Verbs[1].Input.Response >> 8) & 0xFFFF;
    }
    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::ReadRegistryKey(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ LPWSTR KeyName,
    _In_ ULONG BufferLength,
    _Out_ PREGISTRYKEY *ParentKey,
    _Out_ PULONG Type,
    _Out_ PVOID Value)
{
    NTSTATUS Status;
    PREGISTRYKEY RegKey = NULL, SubKey = NULL;
    UNICODE_STRING Name, SubKeyNameString;
    ULONG Index, ResultLength;
    PKEY_BASIC_INFORMATION BasicInformation;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInformation = NULL;
    LPWSTR SubKeyName = NULL;

    // construct registry key
    Status = PcNewRegistryKey(&RegKey, NULL, DriverRegistryKey, GENERIC_READ, DeviceObject, NULL, NULL, 0, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PcNewRegistryKey failed with %x\n", Status);
        return Status;
    }

    // init key
    RtlInitUnicodeString(&Name, KeyName);

    Index = 0;
    do
    {
        SubKey = NULL;
        SubKeyName = NULL;
        PartialInformation = NULL;

        Status = RegKey->EnumerateKey(Index, KeyBasicInformation, NULL, 0, &ResultLength);
        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        BasicInformation = (PKEY_BASIC_INFORMATION)ExAllocatePoolZero(NonPagedPool, ResultLength, TAG_HDAUDIO);
        if (!BasicInformation)
        {
            RegKey->Release();
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Status = RegKey->EnumerateKey(Index, KeyBasicInformation, (PVOID)BasicInformation, ResultLength, &ResultLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: EnumerateKey failed with %x\n", Status);
            RegKey->Release();
            ExFreePool(BasicInformation);
            return Status;
        }
        SubKeyName =
            (LPWSTR)ExAllocatePoolZero(PagedPool, (BasicInformation->NameLength + 1) * sizeof(WCHAR), TAG_HDAUDIO);
        if (!SubKeyName)
        {
            DPRINT1("HDAUDIO: no resources\n");
            RegKey->Release();
            ExFreePool(BasicInformation);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(SubKeyName, BasicInformation->Name, BasicInformation->NameLength);
        RtlInitUnicodeString(&SubKeyNameString, SubKeyName);
        Status = RegKey->NewSubKey(&SubKey, NULL, GENERIC_READ, &SubKeyNameString, 0, NULL);
        ExFreePool(BasicInformation);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: NewSubKey failed with %x\n", Status);
            RegKey->Release();
            ExFreePool(SubKeyName);
            return Status;
        }
        Status = SubKey->QueryValueKey(&Name, KeyValuePartialInformation, NULL, 0, &ResultLength);
        DPRINT("HDAUDIO: QueryValueKey Status %x\n", Status);
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            PartialInformation =
                (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolZero(PagedPool, ResultLength, TAG_HDAUDIO);
            if (!PartialInformation)
            {
                SubKey->Release();
                ExFreePool(SubKeyName);
                RegKey->Release();
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            Status = SubKey->QueryValueKey(
                &Name, KeyValuePartialInformation, PartialInformation, ResultLength, &ResultLength);
            if (NT_SUCCESS(Status))
            {
                *Type = PartialInformation->Type;
                *ParentKey = SubKey;
                if (PartialInformation->DataLength == BufferLength)
                {
                    RtlCopyMemory(Value, PartialInformation->Data, PartialInformation->DataLength);
                }
                ExFreePool(PartialInformation);
                ExFreePool(SubKeyName);
                RegKey->Release();
                return STATUS_SUCCESS;
            }
            ExFreePool(PartialInformation);
        }
        SubKey->Release();
        ExFreePool(SubKeyName);
        Index++;
    } while (TRUE);
    RegKey->Release();
    return STATUS_NOT_FOUND;
}

NTSTATUS
NTAPI
CAdapterCommon::TransferInitVerbs(_In_ PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    ULONG NumVerbs;
    ULONG Index;
    WCHAR Buffer[64];
    ULONG Type, ResultLength;
    PREGISTRYKEY Key;
    UNICODE_STRING KeyName;

    // get number of verbs
    Status = ReadRegistryKey(DeviceObject, L"NumVerbs", sizeof(ULONG), &Key, &Type, &NumVerbs);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Warning failed to read NumVerbs key with %x\n", Status);
        return STATUS_SUCCESS;
    }
    for (Index = 0; Index < NumVerbs; Index++)
    {
        RtlStringCchPrintfW(Buffer, RTL_NUMBER_OF(Buffer), L"%04d", Index);
        RtlInitUnicodeString(&KeyName, Buffer);

        // read key
        Status = Key->QueryValueKey(&KeyName, KeyValuePartialInformation, NULL, 0, &ResultLength);
        DPRINT("Status %x\n", Status);
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            PKEY_VALUE_PARTIAL_INFORMATION PartialInformation =
                (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolZero(PagedPool, ResultLength, TAG_HDAUDIO);
            if (!PartialInformation)
            {
                Key->Release();
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            Status = Key->QueryValueKey(
                &KeyName, KeyValuePartialInformation, PartialInformation, ResultLength, &ResultLength);
            if (NT_SUCCESS(Status) && PartialInformation->DataLength == sizeof(ULONG))
            {
                ULONG Verb = 0;
                Verb |= PartialInformation->Data[0] << 0;
                Verb |= PartialInformation->Data[1] << 8;
                Verb |= PartialInformation->Data[2] << 16;
                Verb |= PartialInformation->Data[3] << 24;
                ExFreePool(PartialInformation);
                DPRINT("KeyName %wZ Value %x\n", &KeyName, Verb);
                HDAUDIO_CODEC_TRANSFER Transfer;
                Transfer.Output.Command = Verb | m_CodecAddress << 28;
                Status = Interface.TransferCodecVerbs(Interface.Context, 1, &Transfer, NULL, NULL);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("HDAUDIO TransferInitVerbs Verb %u failed with %x\n", Index, Status);
                }
            }
        }
    }
    Key->Release();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::TransferVerb(ULONG Verb, PULONG Response)
{
    HDAUDIO_CODEC_TRANSFER Transfer;
    NTSTATUS Status;

    Transfer.Output.Command = Verb;

    Status = Interface.TransferCodecVerbs(Interface.Context, 1, &Transfer, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO TransferVerb Verb %x failed with %x\n", Verb, Status);
        return Status;
    }
    *Response = Transfer.Input.Response;
    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::ChooseSubdeviceName(UCHAR DefaultDevice, UCHAR Instance, UCHAR Wave, _Out_ LPWSTR *OutSubdeviceName)
{
    WCHAR Buffer[64];
    NTSTATUS Status;

    LPCWSTR DeviceTypes[] = {
        L"eLineOut",          // 0x00
        L"eSpeaker",          // 0x01
        L"eHeadphone",        // 0x02
        L"eCDIn",             // 0x03
        L"eSpdifOut",         // 0x04
        L"eDigitalOtherOut",  // 0x05
        L"eModemLineSide",    // 0x06
        L"eModemHandsetSide", // 0x7
        L"eLineIn",           // 0x8
        L"eAuxIn",            // 0x9
        L"eMicIn",            // 0xA
        L"eTelephony",        // 0xB
        L"eSpdifIn",          // 0xC
        L"eDigitalOtherIn",   // 0xD
        L"eReserved",         // 0xE
        L"eOther"             // 0xF
    };
    LPCWSTR Suffix = Wave ? L"Wave" : L"Topo";

    /* the first filter of a type keeps the name the _In_F refers to,
     * further filters of that type get a number to keep the names unique */
    if (Instance)
    {
        Status = RtlStringCchPrintfW(Buffer, RTL_NUMBER_OF(Buffer), L"%s%u%s",
                                     DeviceTypes[DefaultDevice & 0xF], (ULONG)Instance, Suffix);
    }
    else
    {
        Status = RtlStringCchPrintfW(Buffer, RTL_NUMBER_OF(Buffer), L"%s%s",
                                     DeviceTypes[DefaultDevice & 0xF], Suffix);
    }
    if (!NT_SUCCESS(Status))
        return Status;

    SIZE_T Length = (wcslen(Buffer) + 1) * sizeof(WCHAR);
    LPWSTR Name = (LPWSTR)ExAllocatePoolZero(NonPagedPool, Length, TAG_HDAUDIO);
    if (!Name)
    {
        // no mem
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(Name, Buffer, Length);
    *OutSubdeviceName = Name;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::InstallDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ LPWSTR Name,
    _In_ BOOLEAN bTopology,
    _In_ REFGUID PortClassId,
    _In_ PRESOURCELIST ResourceList,
    _In_ PHDAUDIO_FILTER_CONTEXT FilterContext,
    _In_ PVOID FunctionGroupNode,
    _In_ PPCFILTER_DESCRIPTOR FilterDescription,
    _Out_ PUNKNOWN *OutPortUnknown)
{
    NTSTATUS Status;
    PPORT Port;
    PMINIPORTTOPOLOGY MiniportTopology = NULL;
    PMINIPORTWAVERT MiniportWaveRT = NULL;
    PUNKNOWN Miniport;
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)FunctionGroupNode;

    // allocate new port
    Status = PcNewPort(&Port, PortClassId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: PcNewPort failed with %x\n", Status);
        return Status;
    }

    if (bTopology)
    {
        Status = HDAUDIO_NewMiniportTopology(&MiniportTopology, FilterContext, OutNode, this, FilterDescription);
        Miniport = (PUNKNOWN)MiniportTopology;
    }
    else
    {
        Status = HDAUDIO_NewMiniportWaveRT(&MiniportWaveRT, FilterContext, OutNode, this, FilterDescription);
        Miniport = (PUNKNOWN)MiniportWaveRT;
    }
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: creating the miniport for %S failed with %x\n", Name, Status);
        Port->Release();
        return Status;
    }

    // initialize the port driver
    Status = Port->Init(DeviceObject, Irp, Miniport, this, ResourceList);
    if (NT_SUCCESS(Status))
    {
        // register as subdevice
        Status = PcRegisterSubdevice(DeviceObject, Name, Port);
        if (!NT_SUCCESS(Status))
            DPRINT1("HDAUDIO: PcRegisterSubdevice failed for Name %S with %x\n", Name, Status);
    }
    else
    {
        DPRINT1("HDAUDIO: Init failed for Name %S with %x\n", Name, Status);
    }

    // query output port
    if (NT_SUCCESS(Status) && OutPortUnknown)
    {
        Status = Port->QueryInterface(IID_IUnknown, (PVOID *)OutPortUnknown);
        if (!NT_SUCCESS(Status))
            DPRINT1("HDAUDIO: QueryInterface failed for Name %S with %x\n", Name, Status);
    }

    Port->Release();
    Miniport->Release();
    return Status;
}

PHDAUDIO_FILTER_CONTEXT
HDAUDIO_AllocateFilterContext(
    _In_ BOOLEAN Capture,
    _In_ ULONG PinCount,
    _In_ PULONG Pins,
    _In_ ULONG ConverterCount,
    _In_ PULONG Converters)
{
    PHDAUDIO_FILTER_CONTEXT Context;

    Context = (PHDAUDIO_FILTER_CONTEXT)ExAllocatePoolZero(NonPagedPool, sizeof(HDAUDIO_FILTER_CONTEXT), TAG_HDAUDIO);
    if (!Context)
        return NULL;

    Context->Pins = (PULONG)ExAllocatePoolZero(NonPagedPool, (PinCount + 1) * sizeof(ULONG), TAG_HDAUDIO);
    Context->Converters = (PULONG)ExAllocatePoolZero(NonPagedPool, (ConverterCount + 1) * sizeof(ULONG), TAG_HDAUDIO);
    if (!Context->Pins || !Context->Converters)
    {
        if (Context->Pins)
            ExFreePoolWithTag(Context->Pins, TAG_HDAUDIO);
        if (Context->Converters)
            ExFreePoolWithTag(Context->Converters, TAG_HDAUDIO);
        ExFreePoolWithTag(Context, TAG_HDAUDIO);
        return NULL;
    }

    Context->Ref = 1;
    Context->Capture = Capture;
    Context->PinCount = PinCount;
    Context->ConverterCount = ConverterCount;
    RtlCopyMemory(Context->Pins, Pins, PinCount * sizeof(ULONG));
    RtlCopyMemory(Context->Converters, Converters, ConverterCount * sizeof(ULONG));

    /* every wave filter has its own pin descriptors and data ranges */
    if (Capture)
    {
        RtlCopyMemory(Context->WavePins, WaveInPins, sizeof(WaveInPins));
        Context->WaveDescription = WaveInFilterDescription[0];
    }
    else
    {
        RtlCopyMemory(Context->WavePins, WaveOutPins, sizeof(WaveOutPins));
        Context->WaveDescription = WaveOutFilterDescription[0];
    }
    Context->WaveDescription.Pins = Context->WavePins;
    Context->WavePins[0].KsPinDescriptor.DataRanges = Context->DataRanges;
    Context->WavePins[0].KsPinDescriptor.DataRangesCount = 0;
    return Context;
}

VOID
HDAUDIO_ReferenceFilterContext(
    _In_ PHDAUDIO_FILTER_CONTEXT Context)
{
    InterlockedIncrement(&Context->Ref);
}

VOID
HDAUDIO_ReleaseFilterContext(
    _In_ PHDAUDIO_FILTER_CONTEXT Context)
{
    if (InterlockedDecrement(&Context->Ref) != 0)
        return;

    ExFreePoolWithTag(Context->Pins, TAG_HDAUDIO);
    ExFreePoolWithTag(Context->Converters, TAG_HDAUDIO);
    ExFreePoolWithTag(Context, TAG_HDAUDIO);
}

static const ULONG HDAUDIO_SampleSizes[] = {8, 16, 20, 24, 32};
static const ULONG HDAUDIO_SampleRates[] = {8000, 11025, 16000, 22050, 32000, 44100, 48000,
                                            88200, 96000, 176400, 192000, 384000};

/* the supported sample sizes and rates as bit masks, in the order of the tables above */
static
VOID
HDAUDIO_GetFormatMasks(
    _In_ PNODE_PCM_RATES Rates,
    _Out_ PULONG SizeMask,
    _Out_ PULONG RateMask)
{
    *SizeMask = (Rates->AudioFormatSupported8Bit ? 0x01 : 0) |
                (Rates->AudioFormatSupported16Bit ? 0x02 : 0) |
                (Rates->AudioFormatSupported20Bit ? 0x04 : 0) |
                (Rates->AudioFormatSupported24Bit ? 0x08 : 0) |
                (Rates->AudioFormatSupported32Bit ? 0x10 : 0);

    *RateMask = (Rates->Supported8Khz ? 0x001 : 0) |
                (Rates->Supported11Khz ? 0x002 : 0) |
                (Rates->Supported16Khz ? 0x004 : 0) |
                (Rates->Supported22Khz ? 0x008 : 0) |
                (Rates->Supported32Khz ? 0x010 : 0) |
                (Rates->Supported44Khz ? 0x020 : 0) |
                (Rates->Supported48Khz ? 0x040 : 0) |
                (Rates->Supported88Khz ? 0x080 : 0) |
                (Rates->Supported96Khz ? 0x100 : 0) |
                (Rates->Supported176Khz ? 0x200 : 0) |
                (Rates->Supported192Khz ? 0x400 : 0) |
                (Rates->Supported384Khz ? 0x800 : 0);
}

static
VOID
HDAUDIO_GetMaskRange(
    _In_ ULONG Mask,
    _In_ const ULONG *Values,
    _In_ ULONG ValueCount,
    _Out_ PULONG Minimum,
    _Out_ PULONG Maximum)
{
    *Minimum = 0;
    *Maximum = 0;
    for (ULONG Index = 0; Index < ValueCount; Index++)
    {
        if (!(Mask & (1 << Index)))
            continue;
        if (!*Minimum)
            *Minimum = Values[Index];
        *Maximum = Values[Index];
    }
}

NTSTATUS
NTAPI
CAdapterCommon::BuildWaveFormat(
    _In_ PVOID Node,
    _In_ PHDAUDIO_FILTER_CONTEXT FilterContext)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    ULONG MaximumChannels = 0;
    ULONG SizeMask = (ULONG)-1;
    ULONG RateMask = (ULONG)-1;
    BOOLEAN FormatPCMSupported = TRUE;
    BOOLEAN FormatFloatSupported = TRUE;
    BOOLEAN FormatAC3Supported = TRUE;
    ULONG MinimumBitsPerSample, MaximumBitsPerSample;
    ULONG MinimumSampleFrequency, MaximumSampleFrequency;
    NTSTATUS Status;

    ULONG ConvertersWithFormats = 0;

    /* a stream drives all converters of the filter, so only what all of them support is offered */
    for (ULONG Index = 0; Index < FilterContext->ConverterCount; Index++)
    {
        PNODE_CONTEXT Converter = OutNode->FindNodeId(FilterContext->Converters[Index]);
        if (!Converter)
            continue;

        MaximumChannels += Converter->ChannelCount ? Converter->ChannelCount : 1;

        NODE_PCM_RATES NodePcmRates;
        RtlZeroMemory(&NodePcmRates, sizeof(NodePcmRates));
        Status = OutNode->GetSupportedPCMSizeRates(Converter->NodeId, &NodePcmRates);
        if (!NT_SUCCESS(Status))
        {
            /* the converter does not restrict the formats then */
            DPRINT1("Failed to get supported pcm rates for node %u Status %x\n", Converter->NodeId, Status);
            continue;
        }
        ConvertersWithFormats++;

        ULONG NodeSizeMask, NodeRateMask;
        HDAUDIO_GetFormatMasks(&NodePcmRates, &NodeSizeMask, &NodeRateMask);
        SizeMask &= NodeSizeMask;
        RateMask &= NodeRateMask;
        FormatPCMSupported = FormatPCMSupported && NodePcmRates.PCMFormatSupported;
        FormatFloatSupported = FormatFloatSupported && NodePcmRates.Float32FormatSupported;
        FormatAC3Supported = FormatAC3Supported && NodePcmRates.AC3FormatSupported;
    }

    if (!ConvertersWithFormats)
    {
        DPRINT1("HDAUDIO: no converter reported its formats\n");
        return STATUS_UNSUCCESSFUL;
    }

    HDAUDIO_GetMaskRange(SizeMask, HDAUDIO_SampleSizes, RTL_NUMBER_OF(HDAUDIO_SampleSizes),
                         &MinimumBitsPerSample, &MaximumBitsPerSample);
    HDAUDIO_GetMaskRange(RateMask, HDAUDIO_SampleRates, RTL_NUMBER_OF(HDAUDIO_SampleRates),
                         &MinimumSampleFrequency, &MaximumSampleFrequency);

    DPRINT("Converters %u MaximumChannels %u Bits %u-%u Frequency %u-%u PCM %u Float %u AC3 %u\n",
            FilterContext->ConverterCount, MaximumChannels, MinimumBitsPerSample, MaximumBitsPerSample,
            MinimumSampleFrequency, MaximumSampleFrequency,
            FormatPCMSupported, FormatFloatSupported, FormatAC3Supported);

    ULONG FormatsSupported = FormatPCMSupported + FormatFloatSupported + FormatAC3Supported;
    if (!FormatsSupported || !MaximumBitsPerSample || !MaximumSampleFrequency)
    {
        DPRINT1("HDAUDIO: NO Formats supported\n");
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG Index = 0; Index < FormatsSupported; Index++)
    {
        PKSDATARANGE_AUDIO DataRange = &FilterContext->DataRange[Index];

        DataRange->DataRange.FormatSize = sizeof(KSDATARANGE_AUDIO);
        DataRange->DataRange.Flags = 0;
        DataRange->DataRange.SampleSize = 0;
        DataRange->DataRange.Reserved = 0;
        DataRange->DataRange.MajorFormat = {KSDATAFORMAT_TYPE_AUDIO};
        DataRange->DataRange.Specifier = {KSDATAFORMAT_SPECIFIER_WAVEFORMATEX};

        if (FormatPCMSupported)
        {
            DataRange->DataRange.SubFormat = {KSDATAFORMAT_SUBTYPE_PCM};
            FormatPCMSupported = FALSE;
        }
        else if (FormatAC3Supported)
        {
            DataRange->DataRange.SubFormat = {KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF};
            FormatAC3Supported = FALSE;
        }
        else
        {
            DataRange->DataRange.SubFormat = {KSDATAFORMAT_SUBTYPE_IEEE_FLOAT};
            FormatFloatSupported = FALSE;
        }

        DataRange->MaximumChannels = MaximumChannels;
        DataRange->MinimumBitsPerSample = MinimumBitsPerSample;
        DataRange->MaximumBitsPerSample = MaximumBitsPerSample;
        DataRange->MinimumSampleFrequency = MinimumSampleFrequency;
        DataRange->MaximumSampleFrequency = MaximumSampleFrequency;

        FilterContext->DataRanges[Index] = (PKSDATARANGE)DataRange;
    }

    FilterContext->WavePins[0].KsPinDescriptor.DataRangesCount = FormatsSupported;
    return STATUS_SUCCESS;
}

/*
 * Searches a path through mixers and selectors. The connection list of a widget names
 * its inputs, so a playback path is searched from the pin to a DAC and a recording
 * path from an ADC to the pin. Path[0] is the start node.
 */
static
BOOLEAN
HDAUDIO_FindPath(
    _In_ CFunctionGroupNode *Node,
    _In_ ULONG NodeId,
    _In_ BOOLEAN bOutput,
    _In_ ULONG TargetNodeId,
    _In_ UCHAR Digital,
    _In_ BOOLEAN AllowClaimed,
    _In_ ULONG Depth,
    _Out_ PULONG Path,
    _Out_ PULONG PathLength)
{
    PNODE_CONTEXT NodeContext;

    if (Depth >= HDA_MAX_PATH_LENGTH)
        return FALSE;

    NodeContext = Node->FindNodeId(NodeId);
    if (!NodeContext || NodeContext->Visited)
        return FALSE;

    Path[Depth] = NodeId;
    if (Depth)
    {
        if (bOutput && NodeContext->NodeType == HDA_NODE_OUTPUT)
        {
            if (NodeContext->Digital != Digital || (NodeContext->Claimed && !AllowClaimed))
                return FALSE;

            *PathLength = Depth + 1;
            return TRUE;
        }

        if (!bOutput && NodeId == TargetNodeId)
        {
            *PathLength = Depth + 1;
            return TRUE;
        }

        /* the path only passes mixers and selectors */
        if (NodeContext->NodeType != HDA_NODE_MIXER && NodeContext->NodeType != HDA_NODE_SELECTOR)
            return FALSE;
    }

    NodeContext->Visited = TRUE;
    for (ULONG Index = 0; Index < NodeContext->ConnectionCount; Index++)
    {
        if (HDAUDIO_FindPath(Node, NodeContext->Connections[Index], bOutput, TargetNodeId, Digital,
                             AllowClaimed, Depth + 1, Path, PathLength))
        {
            NodeContext->Visited = FALSE;
            return TRUE;
        }
    }
    NodeContext->Visited = FALSE;
    return FALSE;
}

/* unmutes an amplifier, a gain at the minimum is raised to 0 dB */
static
VOID
HDAUDIO_UnmuteAmplifier(
    _In_ CFunctionGroupNode *Node,
    _In_ ULONG NodeId,
    _In_ BOOLEAN Input,
    _In_ UCHAR Index)
{
    AMPLIFIER_CAPABILITIES Caps;
    AMPLIFIER_GAIN_MUTE_SET Set;
    UCHAR Mute, Gain;

    if (!NT_SUCCESS(Node->GetAmplifierDetails(NodeId, Input, &Caps)))
        return;

    for (UCHAR Left = 0; Left < 2; Left++)
    {
        if (!NT_SUCCESS(Node->GetAmplifierGainMute(NodeId, Input, Left, Index, &Mute, &Gain)) || !Gain)
            Gain = Caps.Offset;

        RtlZeroMemory(&Set, sizeof(Set));
        Set.Output = !Input;
        Set.Input = Input;
        Set.Left = Left;
        Set.Right = !Left;
        Set.Index = Index;
        Set.Mute = 0;
        Set.Gain = Gain & 0x7F;
        Node->SetAmplifierGainMute(NodeId, &Set);
    }
}

NTSTATUS
NTAPI
CAdapterCommon::SetupPath(
    _In_ PVOID Node,
    _In_ BOOLEAN bOutput,
    _In_ ULONG PinNodeId,
    _In_ UCHAR DefaultDevice,
    _In_ UCHAR Digital,
    _Out_ PULONG ConverterNodeId)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    ULONG Path[HDA_MAX_PATH_LENGTH];
    ULONG PathLength = 0;
    BOOLEAN Found = FALSE;
    NTSTATUS Status;

    if (bOutput)
    {
        /* a converter of its own if there is one left, a shared one otherwise */
        for (ULONG Pass = 0; Pass < 2 && !Found; Pass++)
        {
            OutNode->ClearVisitedState();
            Found = HDAUDIO_FindPath(OutNode, PinNodeId, TRUE, 0, Digital, Pass != 0, 0, Path, &PathLength);
        }
    }
    else
    {
        ULONG InputNodeCount = 0;
        PULONG InputNodes = NULL;

        Status = OutNode->GetNodesWithType(HDA_NODE_INPUT, &InputNodeCount, &InputNodes);
        if (!NT_SUCCESS(Status))
            return Status;

        for (ULONG Pass = 0; Pass < 2 && !Found; Pass++)
        {
            for (ULONG Index = 0; Index < InputNodeCount && !Found; Index++)
            {
                PNODE_CONTEXT Converter = OutNode->FindNodeId(InputNodes[Index]);
                if (!Converter || Converter->Digital != Digital || (Converter->Claimed && !Pass))
                    continue;

                OutNode->ClearVisitedState();
                Found = HDAUDIO_FindPath(OutNode, InputNodes[Index], FALSE, PinNodeId, Digital, TRUE, 0, Path, &PathLength);
            }
        }

        if (InputNodes)
            ExFreePool(InputNodes);
    }

    if (!Found)
    {
        /* no route through mixers and selectors, the converter is taken as it is,
         * one of the pin's kind which is not used yet if there is one */
        ULONG ConverterCount = 0;
        PULONG Converters = NULL;

        DPRINT1("HDAUDIO: no converter path for pin %u output %u, using a fallback converter\n", PinNodeId, bOutput);

        Status = OutNode->GetNodesWithType(bOutput ? HDA_NODE_OUTPUT : HDA_NODE_INPUT, &ConverterCount, &Converters);
        if (!NT_SUCCESS(Status))
            return Status;

        for (ULONG Pass = 0; Pass < 3 && !Found; Pass++)
        {
            for (ULONG Index = 0; Index < ConverterCount && !Found; Index++)
            {
                PNODE_CONTEXT Converter = OutNode->FindNodeId(Converters[Index]);
                if (!Converter)
                    continue;
                if (Pass < 2 && Converter->Digital != Digital)
                    continue;
                if (Pass == 0 && Converter->Claimed)
                    continue;

                Path[0] = bOutput ? PinNodeId : Converters[Index];
                Path[1] = bOutput ? Converters[Index] : PinNodeId;
                PathLength = 2;
                Found = TRUE;
            }
        }

        if (Converters)
            ExFreePool(Converters);

        if (!Found)
        {
            DPRINT1("HDAUDIO: no converter for pin %u output %u\n", PinNodeId, bOutput);
            return STATUS_NOT_FOUND;
        }
    }

    for (ULONG Index = 0; Index < PathLength; Index++)
    {
        PNODE_CONTEXT NodeContext = OutNode->FindNodeId(Path[Index]);
        UCHAR Connection = 0;
        BOOLEAN Connected = FALSE;

        if (!NodeContext)
            continue;

        DPRINT("HDAUDIO: pin %u path node %u type %u\n", PinNodeId, Path[Index], NodeContext->NodeType);

        OutNode->SetPowerState(Path[Index], TRUE);

        /* the node takes its input from the next node of the path */
        if (Index + 1 < PathLength)
        {
            for (ULONG CIndex = 0; CIndex < NodeContext->ConnectionCount; CIndex++)
            {
                if (NodeContext->Connections[CIndex] == Path[Index + 1])
                {
                    Connection = (UCHAR)CIndex;
                    Connected = TRUE;
                    break;
                }
            }

            /* a mixer sums all of its inputs, everything else selects one */
            if (Connected && NodeContext->ConnectionCount > 1 && NodeContext->NodeType != HDA_NODE_MIXER)
                OutNode->SetConnectionIndex(Path[Index], Connection);
        }

        if (NodeContext->NodeType == HDA_NODE_P_In_)
        {
            /* the output amplifier drives the jack, the input amplifier boosts it */
            HDAUDIO_UnmuteAmplifier(OutNode, Path[Index], !bOutput, 0);
        }
        else
        {
            HDAUDIO_UnmuteAmplifier(OutNode, Path[Index], FALSE, 0);
            if (Connected)
                HDAUDIO_UnmuteAmplifier(OutNode, Path[Index], TRUE, Connection);
        }
    }

    /* enable the pin in its direction, the reference voltage of a mic stays as it is */
    PIN_CAPABILITIES PinCaps;
    UCHAR PinControl = 0;
    if (!NT_SUCCESS(OutNode->GetPinWidgetControlValue(PinNodeId, &PinControl)))
        PinControl = 0;

    if (bOutput)
    {
        PinControl &= ~AC_PINCTL_IN_EN;
        PinControl |= AC_PINCTL_OUT_EN;
        if (DefaultDevice == 0x2 &&
            NT_SUCCESS(OutNode->GetPinCapabilities(PinNodeId, &PinCaps)) &&
            PinCaps.HeadphoneDriveCapable)
        {
            PinControl |= AC_PINCTL_HP_EN;
        }
    }
    else
    {
        PinControl &= ~(AC_PINCTL_OUT_EN | AC_PINCTL_HP_EN);
        PinControl |= AC_PINCTL_IN_EN;
    }
    OutNode->SetPinWidgetControlValue(PinNodeId, PinControl);

    /* the converter is the last node of a playback path and the first of a recording path */
    ULONG Converter = bOutput ? Path[PathLength - 1] : Path[0];
    PNODE_CONTEXT ConverterContext = OutNode->FindNodeId(Converter);
    ConverterContext->Claimed = TRUE;
    *ConverterNodeId = Converter;

    DPRINT("HDAUDIO: pin %u uses converter %u\n", PinNodeId, Converter);
    return STATUS_SUCCESS;
}

VOID NTAPI
CAdapterCommon::ClearRef(_In_ ULONG RefValue, _In_ ULONG NodeCount, _In_ PULONG Nodes)
{
    for (ULONG Index = 0; Index < NodeCount; Index++)
    {
        if (Nodes[Index] == RefValue)
        {
            Nodes[Index] = (ULONG)-1;
            break;
        }
    }
}

NTSTATUS
NTAPI
CAdapterCommon::Initialize(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PRESOURCELIST ResourceList)
{
    NTSTATUS Status;
    CFunctionGroupNode *OutNode = nullptr;
    ULONG OutputNodeCount = 0;
    ULONG InputNodeCount = 0;
    ULONG PinNodeCount = 0;
    PULONG OutputNodes = NULL;
    PULONG InputNodes = NULL;
    PULONG PinNodes = NULL;

    // first acquire HDAUDIO bus interface from hdaudbus
    Status = AcquireBusInterface(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Failed to acquire bus interface Status %x\n", Status);
        return Status;
    }
    // get resource information
    GetResourceInformation();
    DPRINT("HDAUDIO: CodecAddress %u FunctionGroupStartNode %u\n", m_CodecAddress, m_FunctionGroupStartNode);

    // get hardware IDs (Vendor ID, Device ID and Revision ID)
    Status = GetHardwareIds();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Failed to get hardware IDs %x\n", Status);
        return Status;
    }

    // transfer init verbs
    Status = TransferInitVerbs(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: TransferInitVerbs failed %x\n", Status);
        return Status;
    }

    // now enum widgets
    Status = HDAUDIO_EnumFunctionGroupWidgets(m_CodecAddress, m_FunctionGroupStartNode, this, &OutNode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: HDAUDIO_EnumFunctionGroupWidgets failed %x\n", Status);
        return Status;
    }

    // now get number of input / output widgets
    Status = OutNode->GetNodesWithType(HDA_NODE_OUTPUT, &OutputNodeCount, &OutputNodes);
    if (NT_SUCCESS(Status))
        Status = OutNode->GetNodesWithType(HDA_NODE_INPUT, &InputNodeCount, &InputNodes);
    if (NT_SUCCESS(Status))
        Status = OutNode->GetNodesWithType(HDA_NODE_P_In_, &PinNodeCount, &PinNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed with %x\n", Status);
        delete OutNode;
        goto Cleanup;
    }

    DPRINT(
        "HDAUDIO: InputNodeCount %u OutputNodeCount %u PinNodeCount %u\n", InputNodeCount, OutputNodeCount,
        PinNodeCount);

    // read and cache initial volume and mute states
    UCHAR Gain, Mute;
    if (OutputNodeCount)
    {
        Status = OutNode->GetAmplifierGainMute(OutputNodes[0], FALSE, TRUE, 0, &Mute, &Gain);
        if (NT_SUCCESS(Status))
        {
            DPRINT("HDAUDIO: Initial output gain %d, mute %d\n", Gain, Mute);
            if (Gain == 0)
                Gain = 127;
            if (Mute == 1)
                Mute = 0;
            OutNode->SetCachedOutputGain(Gain);
            OutNode->SetCachedOutputMute(Mute);
        }
    }

    if (InputNodeCount)
    {
        PNODE_CONTEXT NodeContext = OutNode->FindNodeId(InputNodes[0]);
        if (NodeContext)
        {
            Status = OutNode->GetAmplifierGainMute(InputNodes[0], TRUE, TRUE, NodeContext->ConnectionCount ?
                                                                              NodeContext->Connections[0] : 0, &Mute, &Gain);
            if (NT_SUCCESS(Status))
            {
                DPRINT1("HDAUDIO: Initial input gain %d, mute %d\n", Gain, Mute);
                if (Gain == 0)
                    Gain = 127;
                if (Mute == 1)
                    Mute = 0;
                OutNode->SetCachedInputGain(Gain);
                OutNode->SetCachedInputMute(Mute);
            }
        }
    }

    // read and cache initial amplifier caps
    {
        AMPLIFIER_CAPABILITIES AmplifierCapabilities = { 0 };
        if (PinNodeCount)
        {
            Status = OutNode->GetAmplifierDetails(PinNodes[0], FALSE, &AmplifierCapabilities);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("HDAUDIO: GetAmplifierDetails failed status %x, Node %u\n", Status, PinNodes[0]);
            }
            else
            {
                DPRINT("HDAUDIO: AmplifierCapabilities NumSteps %u, StepSize %u, Offset %u\n",
                      AmplifierCapabilities.NumSteps, AmplifierCapabilities.Steps, AmplifierCapabilities.Offset);
            }
        }

        if (AmplifierCapabilities.NumSteps == 0 || AmplifierCapabilities.Steps == 0 || AmplifierCapabilities.Offset == 0)
        {
            DPRINT1("HDAUDIO: No steps count or step size specified\n");
            // fallback to defaults
            AmplifierCapabilities.NumSteps = AmplifierCapabilities.Offset = OutNode->GetCachedOutputGain();
            AmplifierCapabilities.Steps = AmplifierCapabilities.NumSteps == 127 ? 2 : 1;
        }

        OutNode->SetCachedAmplifierCapabilities(&AmplifierCapabilities);
    }

    if (VendorId == 0x111d || // IDT
        VendorId == 0x8384 || // SigmaTel
        VendorId == 0x10ec) // Realtek
    {
        // Execute vendor-specific quirks
        Status = ExecuteVendorSpecificQuirks((PVOID)OutNode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: ExecuteVendorSpecificQuirks failed with %x\n", Status);
            delete OutNode;
            goto Cleanup;
        }
    }

    /* from here on the installed filters refer to the function group node, it stays */
    OutNode->ClearVisitedState();
    DPRINT("ProcessOutputNodes----------------------------------\n");
    Status = ProcessOutputNodes(DeviceObject, Irp, ResourceList, PinNodeCount, PinNodes, (PVOID)OutNode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: ProcessOutputNodes failed with %x\n", Status);
        goto Cleanup;
    }

    if (InputNodeCount && PinNodeCount)
    {
        // the output processing marked its pins, start with a complete list
        ExFreePool(PinNodes);
        PinNodes = NULL;

        Status = OutNode->GetNodesWithType(HDA_NODE_P_In_, &PinNodeCount, &PinNodes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: GetNodesWithType failed for pin node with %x\n", Status);
            goto Cleanup;
        }

        OutNode->ClearVisitedState();
        DPRINT("ProcessInputNodes----------------------------------\n");
        Status = ProcessInputNodes(DeviceObject, Irp, ResourceList, PinNodeCount, PinNodes, (PVOID)OutNode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: ProcessInputNodes failed with %x\n", Status);
        }
    }

Cleanup:
    if (PinNodes)
        ExFreePool(PinNodes);
    if (InputNodes)
        ExFreePool(InputNodes);
    if (OutputNodes)
        ExFreePool(OutputNodes);
    return Status;
}

VOID NTAPI
HDAUDIO_UnsolicitedResponseCallback(HDAUDIO_CODEC_RESPONSE Param, PVOID Context)
{
    UNIMPLEMENTED;
}

/* default devices which take audio from the codec */
static
BOOLEAN
HDAUDIO_IsOutputDevice(
    _In_ UCHAR DefaultDevice)
{
    // line out, speaker, headphone, S/PDIF out, other digital out
    return DefaultDevice <= 0x2 || DefaultDevice == 0x4 || DefaultDevice == 0x5;
}

/* default devices which deliver audio to the codec */
static
BOOLEAN
HDAUDIO_IsInputDevice(
    _In_ UCHAR DefaultDevice)
{
    // CD, line in, aux, mic, telephony, S/PDIF in, other digital in
    return DefaultDevice == 0x3 || (DefaultDevice >= 0x8 && DefaultDevice <= 0xD);
}

/* a pin is in use when the device is built in, plugged in or cannot be detected */
static
BOOLEAN
HDAUDIO_IsPinInUse(
    _In_ CFunctionGroupNode *Node,
    _In_ ULONG PinNodeId,
    _In_ PPIN_CAPABILITIES PinCaps,
    _In_ PPIN_CONFIGURATION_DEFAULT PinConfiguration)
{
    ULONG DevicePresent = 0;

    // the BIOS marks the pin as not connected
    if (PinConfiguration->PortConnectivity == 0x1)
        return FALSE;

    // a built in device is always there, its presence sense means nothing
    if (PinConfiguration->PortConnectivity != 0x0)
        return TRUE;

    // a jack without presence detection, or the detection is overridden
    if (!PinCaps->PresenceDetectCapable || (PinConfiguration->Misc & 0x1))
        return TRUE;

    if (!NT_SUCCESS(Node->GetPinSense(PinNodeId, &DevicePresent)))
        return TRUE;

    DPRINT("HDAUDIO: PinNode %u DevicePresent %x\n", PinNodeId, DevicePresent);
    return DevicePresent != 0;
}

NTSTATUS
NTAPI
CAdapterCommon::ProcessOutputNodes(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PRESOURCELIST ResourceList,
    _In_ ULONG PinNodeCount,
    _In_ PULONG PinNodes,
    _In_ PVOID Node)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;
    ULONG FilterCount = 0;

    /* the pins in use first; without any, the unplugged and unconnected jacks get filters */
    for (ULONG Pass = 0; Pass < 2 && !FilterCount; Pass++)
    {
        for (ULONG NodeIndex = 0; NodeIndex < PinNodeCount; NodeIndex++)
        {
            ULONG PinNodeId = PinNodes[NodeIndex];
            if (PinNodeId == (ULONG)-1)
            {
                // already handled by association
                continue;
            }

            PNODE_CONTEXT NodeContext = OutNode->FindNodeId(PinNodeId);
            if (!NodeContext)
            {
                DPRINT1("HDAUDIO: PinNode %u not found\n", PinNodeId);
                continue;
            }

            PIN_CAPABILITIES PinCaps;
            Status = OutNode->GetPinCapabilities(PinNodeId, &PinCaps);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("PinNode %u failed to retrieve pin caps Status %x\n", PinNodeId, Status);
                continue;
            }

            PIN_CONFIGURATION_DEFAULT PinConfiguration;
            Status = OutNode->GetPinConfigurationDefault(PinNodeId, &PinConfiguration);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("PinNode %u failed to retrieve config defaults Status %x\n", PinNodeId, Status);
                continue;
            }

            DPRINT(
                "HDAUDIO: Pass %u PinNode %u EAPD %x Output %x PresenceDetect %x PortConnectivity %x DefaultDevice %x "
                "DefaultAssociation %x Sequence %x Misc %x\n", Pass, PinNodeId, PinCaps.EAPDCapable,
                PinCaps.OutputCapable, PinCaps.PresenceDetectCapable, PinConfiguration.PortConnectivity,
                PinConfiguration.DefaultDevice, PinConfiguration.DefaultAssociation, PinConfiguration.Sequence,
                PinConfiguration.Misc);

            if (!PinCaps.OutputCapable || !HDAUDIO_IsOutputDevice(PinConfiguration.DefaultDevice))
                continue;

            if (Pass == 0 && !HDAUDIO_IsPinInUse(OutNode, PinNodeId, &PinCaps, &PinConfiguration))
            {
                DPRINT1("HDAUDIO: Warning PinNode %u not in use\n", PinNodeId);
                //continue;
            }

            if (!NodeContext->ConnectionCount)
            {
                // the converter is chosen without a path then
                DPRINT1("Warning: PinNode %u not connected\n", PinNodeId);
            }

            if (PinCaps.EAPDCapable)
            {
                // enable EAPD
                Status = OutNode->SetEAPD(PinNodeId, TRUE);
                if (!NT_SUCCESS(Status))
                    DPRINT1("Warning: SetEAPD failed with %x\n", Status);
            }

            Status = AssociatePins(
                DeviceObject, Irp, ResourceList, PinNodeId, &PinConfiguration, (PVOID)OutNode, PinNodeCount, PinNodes,
                NodeContext->Digital);
            if (NT_SUCCESS(Status))
                FilterCount++;
            else
                DPRINT1("HDAUDIO: no output filter for PinNode %u Status %x\n", PinNodeId, Status);
        }
    }

    DPRINT("HDAUDIO: %u output filters\n", FilterCount);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::ProcessInputNodes(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PRESOURCELIST ResourceList,
    _In_ ULONG PinNodeCount,
    _In_ PULONG PinNodes,
    _In_ PVOID Node)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;
    ULONG FilterCount = 0;

    /* every input jack in use gets a filter of its own; without any, all of them */
    for (ULONG Pass = 0; Pass < 2 && !FilterCount; Pass++)
    {
        for (ULONG NodeIndex = 0; NodeIndex < PinNodeCount; NodeIndex++)
        {
            ULONG PinNodeId = PinNodes[NodeIndex];
            if (PinNodeId == (ULONG)-1)
                continue;

            PNODE_CONTEXT NodeContext = OutNode->FindNodeId(PinNodeId);
            if (!NodeContext)
                continue;

            PIN_CAPABILITIES PinCaps;
            Status = OutNode->GetPinCapabilities(PinNodeId, &PinCaps);
            if (!NT_SUCCESS(Status) || !PinCaps.InputCapable)
                continue;

            PIN_CONFIGURATION_DEFAULT PinConfiguration;
            Status = OutNode->GetPinConfigurationDefault(PinNodeId, &PinConfiguration);
            if (!NT_SUCCESS(Status) || !HDAUDIO_IsInputDevice(PinConfiguration.DefaultDevice))
                continue;

            if (Pass == 0 && !HDAUDIO_IsPinInUse(OutNode, PinNodeId, &PinCaps, &PinConfiguration))
            {
                DPRINT("HDAUDIO: Input PinNode %u not in use\n", PinNodeId);
                continue;
            }

            DPRINT("Adding Input Pin %u DefaultDevice %u PortConnectivity %u\n", PinNodeId,
                   PinConfiguration.DefaultDevice, PinConfiguration.PortConnectivity);
            Status = BuildInstallFilter(
                DeviceObject, Irp, FALSE, OutNode, ResourceList, 1, &PinNodeId, PinConfiguration.DefaultDevice,
                NodeContext->Digital);
            if (NT_SUCCESS(Status))
                FilterCount++;
            else
                DPRINT1("HDAUDIO: no input filter for PinNode %u Status %x\n", PinNodeId, Status);

            PinNodes[NodeIndex] = (ULONG)-1;
        }
    }

    DPRINT("HDAUDIO: %u input filters\n", FilterCount);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::BuildInstallFilter(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ ULONG bOutput,
    _In_ PVOID Node,
    _In_ PRESOURCELIST ResourceList,
    _In_ ULONG AssociatedPinCount,
    _In_ PULONG AssociatedPins,
    _In_ UCHAR DefaultDevice,
    _In_ UCHAR Digital)
{
    NTSTATUS Status;
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    ULONG ConverterCount = 0;
    PUNKNOWN WavePort = NULL;
    PUNKNOWN TopologyPort = NULL;

    PULONG Converters = (PULONG)ExAllocatePoolZero(NonPagedPool, AssociatedPinCount * sizeof(ULONG), TAG_HDAUDIO);
    if (!Converters)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (ULONG PinIndex = 0; PinIndex < AssociatedPinCount; PinIndex++)
    {
        ULONG PinNodeId = AssociatedPins[PinIndex];

        if (VendorId == 0x111d || // IDT
            VendorId == 0x8384 || // SigmaTel
            VendorId == 0x10ec) // Realtek
        {
            PIN_CAPABILITIES PinCaps;

            // Power on the pin and enable the external amplifier
            Status = OutNode->SetPowerState(PinNodeId, TRUE);
            if (!NT_SUCCESS(Status))
                DPRINT1("HDAUDIO: SetPowerState failed with %x node %u\n", Status, PinNodeId);

            if (NT_SUCCESS(OutNode->GetPinCapabilities(PinNodeId, &PinCaps)) && PinCaps.EAPDCapable)
            {
                Status = OutNode->SetEAPD(PinNodeId, TRUE);
                if (!NT_SUCCESS(Status))
                    DPRINT1("Warning: SetEAPD failed with %x node %u\n", Status, PinNodeId);
            }
        }

        // route the pin to a converter, unmute and enable the path
        ULONG Converter;
        Status = SetupPath(OutNode, bOutput ? TRUE : FALSE, PinNodeId, DefaultDevice, Digital, &Converter);
        if (!NT_SUCCESS(Status))
            continue;

        BOOLEAN Known = FALSE;
        for (ULONG Index = 0; Index < ConverterCount; Index++)
        {
            if (Converters[Index] == Converter)
                Known = TRUE;
        }
        if (!Known)
            Converters[ConverterCount++] = Converter;
    }

    if (!ConverterCount)
    {
        DPRINT1("HDAUDIO: no converter for pin %u\n", AssociatedPins[0]);
        ExFreePoolWithTag(Converters, TAG_HDAUDIO);
        return STATUS_NOT_FOUND;
    }

    PHDAUDIO_FILTER_CONTEXT FilterContext =
        HDAUDIO_AllocateFilterContext(bOutput ? FALSE : TRUE, AssociatedPinCount, AssociatedPins, ConverterCount, Converters);
    ExFreePoolWithTag(Converters, TAG_HDAUDIO);
    if (!FilterContext)
        return STATUS_INSUFFICIENT_RESOURCES;

    // build wave format of the converters
    Status = BuildWaveFormat((PVOID)OutNode, FilterContext);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: BuildWaveFormat failed with %x output %x\n", Status, bOutput);
        HDAUDIO_ReleaseFilterContext(FilterContext);
        return Status;
    }

    // the subdevice names are used as object class of the create items, they must stay
    UCHAR Instance = m_SubdeviceCount[DefaultDevice & 0xF];
    LPWSTR SubdeviceWave = NULL;
    LPWSTR SubdeviceTopology = NULL;
    Status = ChooseSubdeviceName(DefaultDevice, Instance, TRUE, &SubdeviceWave);
    if (NT_SUCCESS(Status))
        Status = ChooseSubdeviceName(DefaultDevice, Instance, FALSE, &SubdeviceTopology);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: ChooseSubdeviceName failed with %x\n", Status);
        if (SubdeviceWave)
            ExFreePoolWithTag(SubdeviceWave, TAG_HDAUDIO);
        HDAUDIO_ReleaseFilterContext(FilterContext);
        return Status;
    }
    m_SubdeviceCount[DefaultDevice & 0xF]++;

    // construct wavert port
    DPRINT("HDAUDIO: SubdeviceName %S\n", SubdeviceWave);
    Status = InstallDevice(
        DeviceObject, Irp, SubdeviceWave, FALSE, CLSID_PortWaveRT, ResourceList, FilterContext, (PVOID)OutNode,
        &FilterContext->WaveDescription, &WavePort);
    if (NT_SUCCESS(Status))
    {
        // construct topology port
        DPRINT("HDAUDIO: SubdeviceName %S\n", SubdeviceTopology);
        Status = InstallDevice(
            DeviceObject, Irp, SubdeviceTopology, TRUE, CLSID_PortTopology, ResourceList, FilterContext, (PVOID)OutNode,
            bOutput ? TopoOutFilterDescription : TopoInFilterDescription, &TopologyPort);
    }

    if (NT_SUCCESS(Status))
    {
        if (bOutput)
        {
            // register waveout connection
            Status = PcRegisterPhysicalConnection(DeviceObject, WavePort, 1, TopologyPort, 0);
        }
        else
        {
            // register wavein connection
            Status = PcRegisterPhysicalConnection(DeviceObject, TopologyPort, 0, WavePort, 1);
        }
        if (!NT_SUCCESS(Status))
            DPRINT1("HDAUDIO: PcRegisterPhysicalConnection failed with %x\n", Status);
    }

    if (TopologyPort)
        TopologyPort->Release();
    if (WavePort)
        WavePort->Release();

    // the miniports hold their own references
    HDAUDIO_ReleaseFilterContext(FilterContext);
    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::AssociatePins(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PRESOURCELIST ResourceList,
    _In_ ULONG PinNodeId,
    _In_ PPIN_CONFIGURATION_DEFAULT PinConfiguration,
    _In_ PVOID Node,
    _In_ ULONG PinNodeCount,
    _In_ PULONG PinNodes,
    _In_ UCHAR Digital)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AssociatedPinCount = 0;
    PULONG AssociatedPins = NULL;
    ULONG PinCount = 0;
    UCHAR DefaultDevice = PinConfiguration->DefaultDevice;

    /* association 15 marks a pin of its own, 0 is reserved */
    if (PinConfiguration->DefaultAssociation != 0x0 && PinConfiguration->DefaultAssociation != 0xF)
    {
        Status = OutNode->GetPinNodesWithDefaultAssociation(
            PinConfiguration->DefaultAssociation, Digital, &AssociatedPinCount, &AssociatedPins);
        DPRINT("GetPinNodesWithDefaultAssociation Status %x, AssociatedPinCount %u\n", Status, AssociatedPinCount);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    PULONG Pins = (PULONG)ExAllocatePoolZero(NonPagedPool, (AssociatedPinCount + 1) * sizeof(ULONG), TAG_HDAUDIO);
    PUCHAR Sequences = (PUCHAR)ExAllocatePoolZero(NonPagedPool, AssociatedPinCount + 1, TAG_HDAUDIO);
    if (!Pins || !Sequences)
    {
        if (Pins)
            ExFreePoolWithTag(Pins, TAG_HDAUDIO);
        if (Sequences)
            ExFreePoolWithTag(Sequences, TAG_HDAUDIO);
        if (AssociatedPins)
            ExFreePool(AssociatedPins);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* the output pins of the association which still wait for a filter, by sequence */
    for (ULONG Index = 0; Index < AssociatedPinCount; Index++)
    {
        ULONG Pin = AssociatedPins[Index];
        BOOLEAN Pending = FALSE;
        for (ULONG PinIndex = 0; PinIndex < PinNodeCount; PinIndex++)
        {
            if (PinNodes[PinIndex] == Pin)
                Pending = TRUE;
        }
        if (!Pending)
            continue;

        PIN_CONFIGURATION_DEFAULT Configuration;
        PIN_CAPABILITIES PinCaps;
        if (!NT_SUCCESS(OutNode->GetPinConfigurationDefault(Pin, &Configuration)) ||
            Configuration.DefaultDevice > 0x7 ||
            !NT_SUCCESS(OutNode->GetPinCapabilities(Pin, &PinCaps)) || !PinCaps.OutputCapable)
        {
            continue;
        }

        ULONG Position = PinCount;
        while (Position > 0 && Sequences[Position - 1] > Configuration.Sequence)
        {
            Pins[Position] = Pins[Position - 1];
            Sequences[Position] = Sequences[Position - 1];
            Position--;
        }
        Pins[Position] = Pin;
        Sequences[Position] = Configuration.Sequence;
        PinCount++;

        /* the first pin of the association names the filter */
        if (Position == 0)
            DefaultDevice = Configuration.DefaultDevice;
    }

    if (!PinCount)
    {
        Pins[0] = PinNodeId;
        PinCount = 1;
    }

    Status = BuildInstallFilter(DeviceObject, Irp, TRUE, OutNode, ResourceList, PinCount, Pins, DefaultDevice, Digital);

    /* the pins are done, a failed association is not tried again pin by pin */
    for (ULONG Index = 0; Index < PinCount; Index++)
        ClearRef(Pins[Index], PinNodeCount, PinNodes);
    ClearRef(PinNodeId, PinNodeCount, PinNodes);

    ExFreePoolWithTag(Sequences, TAG_HDAUDIO);
    ExFreePoolWithTag(Pins, TAG_HDAUDIO);
    if (AssociatedPins)
        ExFreePool(AssociatedPins);
    return Status;
}

NTSTATUS
HDAUDIO_AllocateCommonAdapter(_Out_ CAdapterCommon **OutAdapter)
{
    CAdapterCommon *This;

    This = new (NonPagedPool, TAG_HDAUDIO) CAdapterCommon(NULL);
    if (!This)
    {
        // out of memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // add reference
    This->AddRef();

    // return result
    *OutAdapter = This;

    // done
    return STATUS_SUCCESS;
}

NTSTATUS
HDAUDIO_InitializeCommonAdapter(CAdapterCommon *CommonAdapter, _In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PRESOURCELIST ResourceList)
{
    NTSTATUS Status;

    // init device
    Status = CommonAdapter->Initialize(DeviceObject, Irp, ResourceList);

    // done
    return Status;
}
