/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/drivers/hdaudio/hdaudio.cpp
 * PURPOSE:         HDAudio Driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

#define YDEBUG
#include <debug.h>

GUID KSCATEGORY_RangeAudio = {STATIC_KSCATEGORY_AUDIO};
GUID KSCATEGORY_Audio = {STATIC_KSCATEGORY_AUDIO};
GUID KSAUDFNAME_Pc_Speaker = {STATIC_KSAUDFNAME_PC_SPEAKER};
GUID PINNAME_Capture = {STATIC_PINNAME_CAPTURE};
GUID KSAUDFNAME_Recording_Control = {STATIC_KSAUDFNAME_RECORDING_CONTROL};

KSPIN_INTERFACE StandardPinInterface = {{STATIC_KSINTERFACESETID_Standard}, KSINTERFACE_STANDARD_STREAMING, 0};

KSPIN_MEDIUM StandardPinMedium = {{STATIC_KSMEDIUMSETID_Standard}, KSMEDIUM_TYPE_ANYINSTANCE, 0};
PCNODE_DESCRIPTOR DacNode[] = {{0, NULL, &KSNODETYPE_DAC, NULL}};

PCNODE_DESCRIPTOR AdcNode[] = {{0, NULL, &KSNODETYPE_ADC, NULL}};

PCCONNECTION_DESCRIPTOR DacConnections[] = {{KSFILTER_NODE, 0, 0, 1}, {0, 0, KSFILTER_NODE, 1}};

PCCONNECTION_DESCRIPTOR AdcConnections[] = {{KSFILTER_NODE, 1, 0, 1}, {0, 0, KSFILTER_NODE, 0}};

GUID AdcCategories[] = {KSCATEGORY_AUDIO, KSCATEGORY_CAPTURE};

GUID DacCategories[] = {KSCATEGORY_AUDIO, KSCATEGORY_RENDER};

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
    ExFreePool(ptr);
}

void __cdecl
operator delete(PVOID ptr, UINT_PTR Unused)
{
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
CAdapterCommon::QueryInterface(IN REFIID refiid, OUT PVOID *Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IAdapterPowerManagement) || IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PADAPTERPOWERMANAGEMENT(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("CAdapterCommon::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}
//----------------------------------------------------------------------------
VOID NTAPI
CAdapterCommon::PowerChangeState(IN POWER_STATE NewState)
{
    UNIMPLEMENTED_ONCE;
}
NTSTATUS
NTAPI
CAdapterCommon::QueryDeviceCapabilities(IN PDEVICE_CAPABILITIES PowerDeviceCaps)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
}
NTSTATUS
NTAPI
CAdapterCommon::QueryPowerChangeState(IN POWER_STATE NewStateQuery)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
}
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
CAdapterCommon::AcquireBusInterface(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION Stack;

    DPRINT1("AcquireBusInterface\n");
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
CAdapterCommon::GetVendorRevisionId()
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
        RevisionId = (Verbs[1].Input.Response >> 8) & 0xFFFF;
    }
    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::ReadRegistryKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN LPWSTR KeyName,
    IN ULONG BufferLength,
    OUT PREGISTRYKEY *ParentKey,
    OUT PULONG Type,
    OUT PVOID Value)
{
    NTSTATUS Status;
    PREGISTRYKEY RegKey, SubKey;
    UNICODE_STRING Name, SubKeyNameString;
    ULONG Index, ResultLength;
    PKEY_BASIC_INFORMATION BasicInformation;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInformation;
    LPWSTR SubKeyName;

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
            DPRINT1("HDAUDIO: EnumerateKey failed with %x\n");
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
        DPRINT1("HDAUDIO: QueryValueKey Status %x\n", Status);
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
                RegKey->Release();
                return STATUS_SUCCESS;
            }
        }
        Index++;
    } while (TRUE);
    return STATUS_NOT_FOUND;
}

VOID NTAPI
HDAUDIO_UnsolicitedResponseCallback(HDAUDIO_CODEC_RESPONSE Param, PVOID Context)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
CAdapterCommon::TransferInitVerbs(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    ULONG NumVerbs;
    ULONG Index;
    WCHAR Buffer[64];
    ULONG Type, ResultLength;
    PREGISTRYKEY Key;
    UNICODE_STRING KeyName;

    // FIXME examine setup unsolicated responses
    for (ULONG Index = 0; Index < 5; Index++)
    {
        Status = Interface.RegisterEventCallback(
            Interface.Context, HDAUDIO_UnsolicitedResponseCallback, (PVOID)this, &m_Tags[Index]);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    // get number of verbs
    Status = ReadRegistryKey(DeviceObject, L"NumVerbs", sizeof(ULONG), &Key, &Type, &NumVerbs);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Warning failed to read NumVerbs key with %x\n", Status);
        return STATUS_SUCCESS;
    }
    for (Index = 0; Index < NumVerbs; Index++)
    {
        RtlStringCchPrintfW(Buffer, sizeof(Buffer), L"%04d", Index);
        RtlInitUnicodeString(&KeyName, Buffer);

        // read key
        Status = Key->QueryValueKey(&KeyName, KeyValuePartialInformation, NULL, 0, &ResultLength);
        DPRINT1("Status %x\n", Status);
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            PKEY_VALUE_PARTIAL_INFORMATION PartialInformation =
                (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolZero(PagedPool, ResultLength, TAG_HDAUDIO);
            Status = Key->QueryValueKey(
                &KeyName, KeyValuePartialInformation, PartialInformation, ResultLength, &ResultLength);
            if (NT_SUCCESS(Status))
            {
                ULONG Verb = 0;
                Verb |= PartialInformation->Data[0] << 24;
                Verb |= PartialInformation->Data[1] << 16;
                Verb |= PartialInformation->Data[2] << 8;
                Verb |= PartialInformation->Data[3] << 0;
                ExFreePool(PartialInformation);
                DPRINT1("KeyName %wZ Value %x\n", &KeyName, Verb);
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
CAdapterCommon::ChooseSubdeviceName(UCHAR DefaultDevice, UCHAR Wave, OUT LPWSTR *OutSubdeviceName)
{
    WCHAR Buffer[64];

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

    if (Wave)
    {
        RtlStringCchPrintfW(Buffer, sizeof(Buffer), L"%s%s", DeviceTypes[DefaultDevice], L"Wave");
    }
    else
    {
        RtlStringCchPrintfW(Buffer, sizeof(Buffer), L"%s%s", DeviceTypes[DefaultDevice], L"Topo");
    }
    LPWSTR Name = (LPWSTR)ExAllocatePoolZero(NonPagedPool, (wcslen(Buffer) + 1) * sizeof(WCHAR), TAG_HDAUDIO);
    if (!Name)
    {
        // no mem
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(Name, Buffer, wcslen(Buffer) * sizeof(WCHAR));
    *OutSubdeviceName = Name;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::InstallDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN LPWSTR Name,
    IN ULONG AssociatedPinsCount,
    IN PULONG AssociatedPinIds,
    IN PVOID FunctionGroupNode,
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    NTSTATUS Status;
    PPORT Port;
    PMINIPORTWAVERT Miniport;
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)FunctionGroupNode;

    // allocate new port
    Status = PcNewPort(&Port, CLSID_PortWaveRT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: PcNewPort failed with %x\n", Status);
        return Status;
    }

    // allocate new miniport
    Status =
        HDAUDIO_NewMiniportWaveRT(&Miniport, AssociatedPinsCount, AssociatedPinIds, OutNode, this, FilterDescription);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: HDAUDIO_NewMiniportWaveRT failed with %x\n", Status);
        Port->Release();
        return Status;
    }

    // initialize the port driver
    Status = Port->Init(DeviceObject, Irp, Miniport, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Init failed for Name %S with %x\n", Name, Status);
        Port->Release();
        Miniport->Release();
        return Status;
    }

    // register as subdevice
    Status = PcRegisterSubdevice(DeviceObject, Name, Port);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: PcRegisterSubdevice failed for Name %S with %x\n", Name, Status);
        Port->Release();
        Miniport->Release();
        return Status;
    }

    // done
    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::FindConnectedWidgets(
    IN ULONG NodeType,
    IN ULONG RecursionLevel,
    IN ULONG NodeId,
    IN PVOID Node,
    IN ULONG NodeIndex,
    IN ULONG NodeCount,
    OUT PULONG OutNodesCount,
    OUT PULONG OutNodes,
    IN UCHAR Digital)
{
    NTSTATUS Status;
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;

    // DPRINT(
    //     "RecursionLevel %u NodeId %u Node %p NodeIndex %u NodeCount %u\n",
    //     RecursionLevel, NodeId, Node, NodeIndex, NodeCount);

    PNODE_CONTEXT NodeContext = OutNode->FindNodeId(NodeId);
    if (!NodeContext)
    {
        // invalid pin found
        DPRINT1("HDAUDIO: No Pin Node found with id %u\n", NodeId);
        return STATUS_UNSUCCESSFUL;
    }
    if (NodeContext->Visited)
    {
        // already processed
        *OutNodesCount = 0;
        return STATUS_SUCCESS;
    }

    NodeContext->Visited = TRUE;

    ULONG TotalFoundNodes = 0;
    if (NodeContext->NodeType == NodeType && NodeContext->Digital == Digital)
    {
        // found target node type
        ASSERT(NodeIndex < NodeCount);
        for (ULONG Index = 0; Index < NodeCount; Index++)
        {
            if (OutNodes[Index] == NodeContext->NodeId)
            {
                // already added
                *OutNodesCount = 0;
                return STATUS_SUCCESS;
            }
        }
        TotalFoundNodes += 1;
        OutNodes[NodeIndex] = NodeContext->NodeId;
        NodeIndex++;
        DPRINT1("HDAUDIO: Adding Node %u Type %u\n", NodeContext->NodeId, NodeType);
    }

    ULONG ConnectionCount = NodeContext->ConnectionCount;
    for (ULONG CNodeIndex = 0; CNodeIndex < ConnectionCount; CNodeIndex++)
    {
        ULONG ConnectedNodeId = NodeContext->Connections[CNodeIndex];
        PNODE_CONTEXT ConnectedNode = OutNode->FindNodeId(ConnectedNodeId);
        if (!ConnectedNode || ConnectedNode->Visited)
            continue;

        ULONG FoundNodes = 0;
        Status = FindConnectedWidgets(
            NodeType, RecursionLevel + 1, ConnectedNodeId, Node, NodeIndex, NodeCount, &FoundNodes, OutNodes, Digital);
        if (NT_SUCCESS(Status))
        {
            if (FoundNodes)
            {
                NodeIndex += FoundNodes;
                TotalFoundNodes += FoundNodes;
            }
        }
    }
    *OutNodesCount = TotalFoundNodes;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::BuildFilter(
    IN PVOID Node,
    IN ULONG NodeCount,
    IN PULONG TargetWidgets,
    OUT PPCFILTER_DESCRIPTOR *OutDescription)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;

    ULONG ChannelCount = 0;
    ULONG MinimumBitsPerSample = (ULONG)-1;
    ULONG MaximumBitsPerSample = 0;
    ULONG MinimumSampleFrequency = (ULONG)-1;
    ULONG MaximumSampleFrequency = 0;
    UCHAR FormatPCMSupported = 0;
    UCHAR FormatFloatSupported = 0;
    UCHAR FormatAC3Supported = 0;
    NTSTATUS Status;

    for (ULONG NodeIndex = 0; NodeIndex < NodeCount; NodeIndex++)
    {
        PNODE_CONTEXT ConnectedNode = OutNode->FindNodeId(TargetWidgets[NodeIndex]);
        ASSERT(ConnectedNode);
        // output / input node
        ASSERT(ConnectedNode->NodeType == 0x00 || ConnectedNode->NodeType == 0x01);
        ChannelCount += ConnectedNode->ChannelCount;

        NODE_PCM_RATES NodePcmRates;
        Status = OutNode->GetSupportedPCMSizeRates(ConnectedNode->NodeId, &NodePcmRates);
        if (NT_SUCCESS(Status))
        {
            if (NodePcmRates.AudioFormatSupported8Bit)
            {
                MinimumBitsPerSample = 8;
                MaximumBitsPerSample = max(8, MaximumBitsPerSample);
            }
            if (NodePcmRates.AudioFormatSupported16Bit)
            {
                MinimumBitsPerSample = min(16, MinimumBitsPerSample);
                MaximumBitsPerSample = max(16, MaximumBitsPerSample);
            }
            if (NodePcmRates.AudioFormatSupported20Bit)
            {
                MinimumBitsPerSample = min(20, MinimumBitsPerSample);
                MaximumBitsPerSample = max(20, MaximumBitsPerSample);
            }
            if (NodePcmRates.AudioFormatSupported24Bit)
            {
                MinimumBitsPerSample = min(24, MinimumBitsPerSample);
                MaximumBitsPerSample = max(24, MaximumBitsPerSample);
            }
            if (NodePcmRates.AudioFormatSupported32Bit)
            {
                MinimumBitsPerSample = min(32, MinimumBitsPerSample);
                MaximumBitsPerSample = max(32, MaximumBitsPerSample);
            }
            if (NodePcmRates.Supported8Khz)
            {
                MinimumSampleFrequency = 8000;
                MaximumSampleFrequency = max(8000, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported11Khz)
            {
                MinimumSampleFrequency = min(11025, MinimumSampleFrequency);
                MaximumSampleFrequency = max(11025, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported16Khz)
            {
                MinimumSampleFrequency = min(16000, MinimumSampleFrequency);
                MaximumSampleFrequency = max(16000, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported22Khz)
            {
                MinimumSampleFrequency = min(22050, MinimumSampleFrequency);
                MaximumSampleFrequency = max(22050, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported32Khz)
            {
                MinimumSampleFrequency = min(32000, MinimumSampleFrequency);
                MaximumSampleFrequency = max(32000, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported44Khz)
            {
                MinimumSampleFrequency = min(44100, MinimumSampleFrequency);
                MaximumSampleFrequency = max(44100, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported48Khz)
            {
                MinimumSampleFrequency = min(48000, MinimumSampleFrequency);
                MaximumSampleFrequency = max(48000, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported88Khz)
            {
                MinimumSampleFrequency = min(88200, MinimumSampleFrequency);
                MaximumSampleFrequency = max(88200, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported96Khz)
            {
                MinimumSampleFrequency = min(96000, MinimumSampleFrequency);
                MaximumSampleFrequency = max(96000, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported176Khz)
            {
                MinimumSampleFrequency = min(176400, MinimumSampleFrequency);
                MaximumSampleFrequency = max(176400, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported192Khz)
            {
                MinimumSampleFrequency = min(192000, MinimumSampleFrequency);
                MaximumSampleFrequency = max(192000, MaximumSampleFrequency);
            }
            if (NodePcmRates.Supported384Khz)
            {
                MinimumSampleFrequency = min(384000, MinimumSampleFrequency);
                MaximumSampleFrequency = max(384000, MaximumSampleFrequency);
            }

            if (NodePcmRates.PCMFormatSupported)
            {
                FormatPCMSupported = TRUE;
            }
            if (NodePcmRates.Float32FormatSupported)
            {
                FormatFloatSupported = TRUE;
            }

            if (NodePcmRates.AC3FormatSupported)
            {
                FormatAC3Supported = TRUE;
            }
        }
        else
        {
            DPRINT1("Failed to get supported pcm rates for node %u Status %x\n", TargetWidgets[NodeIndex], Status);
        }
    }

    ULONG NodeType = (ULONG)-1;
    ULONG Digital = 0xFF;
    for (ULONG Index = 0; Index < NodeCount; Index++)
    {
        DPRINT1("I/O NodeIndex %u\n", TargetWidgets[Index]);
        PNODE_CONTEXT Node = OutNode->FindNodeId(TargetWidgets[Index]);
        ASSERT(Node);
        if (Node)
        {
            if (!Index)
            {
                NodeType = Node->NodeType;
                Digital = Node->Digital;
            }
            else
            {
                // node type should be same in the collection
                ASSERT(NodeType == Node->NodeType);
                ASSERT(Digital == Node->Digital);
            }
        }
    }

    DPRINT1("Digital %x\n", Digital);
    DPRINT1("NodeType %u\n", NodeType);
    DPRINT1("ChannelCount %u\n", ChannelCount);
    DPRINT1("MinimumBitsPerSample %u\n", MinimumBitsPerSample);
    DPRINT1("MaximumBitsPerSample %u\n", MaximumBitsPerSample);
    DPRINT1("MinimumSampleFrequency %u\n", MinimumSampleFrequency);
    DPRINT1("MaximumSampleFrequency %u\n", MaximumSampleFrequency);

    ULONG FormatsSupported = FormatPCMSupported + FormatFloatSupported + FormatAC3Supported;
    if (FormatsSupported == 0)
    {
        // ignore pin
        DPRINT1("HDAUDIO: NO Formats supported\n");
        return STATUS_UNSUCCESSFUL;
    }

    PPCFILTER_DESCRIPTOR Description =
        (PPCFILTER_DESCRIPTOR)ExAllocatePoolZero(NonPagedPool, sizeof(PCFILTER_DESCRIPTOR), TAG_HDAUDIO);
    if (!Description)
    {
        // no memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Description->Version = 0;
    Description->PinSize = sizeof(PCPIN_DESCRIPTOR);
    Description->PinCount = 2;
    PCPIN_DESCRIPTOR *Pins =
        (PCPIN_DESCRIPTOR *)ExAllocatePoolZero(NonPagedPool, sizeof(PCPIN_DESCRIPTOR) * 2, TAG_HDAUDIO);
    if (!Pins)
    {
        // no memory
        ExFreePool(Description);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Description->Pins = Pins;

    Pins[0].MaxGlobalInstanceCount = 1;
    Pins[0].MaxFilterInstanceCount = 1;
    Pins[0].MinFilterInstanceCount = 0;
    Pins[0].KsPinDescriptor.InterfacesCount = 1;
    Pins[0].KsPinDescriptor.Interfaces = &StandardPinInterface;
    Pins[0].KsPinDescriptor.MediumsCount = 1;
    Pins[0].KsPinDescriptor.Mediums = &StandardPinMedium;
    Pins[0].KsPinDescriptor.DataRangesCount = 1;

    Pins[0].KsPinDescriptor.DataRanges =
        (const PKSDATARANGE *)ExAllocatePoolZero(NonPagedPool, sizeof(PKSDATARANGE *) * 1, TAG_HDAUDIO);
    if (!Pins[0].KsPinDescriptor.DataRanges)
    {
        ExFreePool(Pins);
        ExFreePool(Description);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // FIXME handle multiple formats
    ASSERT(FormatsSupported == 1);

    PKSDATARANGE_AUDIO AudioFormat =
        (PKSDATARANGE_AUDIO)ExAllocatePoolZero(NonPagedPool, sizeof(KSDATARANGE_AUDIO) * FormatsSupported, TAG_HDAUDIO);
    if (!AudioFormat)
    {
        ExFreePool((PVOID)Pins[0].KsPinDescriptor.DataRanges);
        ExFreePool(Pins);
        ExFreePool(Description);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    reinterpret_cast<PKSDATARANGE>(Pins[0].KsPinDescriptor.DataRanges[0]) = (PKSDATARANGE)AudioFormat;

    AudioFormat->DataRange.FormatSize = sizeof(KSDATARANGE_AUDIO);
    AudioFormat->DataRange.Flags = 0;
    AudioFormat->DataRange.SampleSize = 0;
    AudioFormat->DataRange.Reserved = 0;

    if (FormatPCMSupported)
    {
        AudioFormat->DataRange.MajorFormat = {STATIC_KSDATAFORMAT_TYPE_AUDIO};
        AudioFormat->DataRange.SubFormat = {STATIC_KSDATAFORMAT_SUBTYPE_PCM};
        AudioFormat->DataRange.Specifier = {STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX};
    }
    else if (FormatAC3Supported)
    {
        AudioFormat->DataRange.MajorFormat = {STATIC_KSDATAFORMAT_TYPE_AUDIO};
        // FIXME add to header
        // AudioFormat->DataRange.SubFormat = {KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF};
        AudioFormat->DataRange.Specifier = {STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX};
    }
    else
    {
        // FIXME configure float
        ASSERT(FALSE);
    }

    AudioFormat->MaximumChannels = ChannelCount;
    AudioFormat->MinimumBitsPerSample = MinimumBitsPerSample;
    AudioFormat->MaximumBitsPerSample = MaximumBitsPerSample;
    AudioFormat->MinimumSampleFrequency = MinimumSampleFrequency;
    AudioFormat->MaximumSampleFrequency = MaximumSampleFrequency;

    if (NodeType == 0x00)
    {
        Pins[0].KsPinDescriptor.DataFlow = KSPIN_DATAFLOW_IN;
        Pins[0].KsPinDescriptor.Communication = KSPIN_COMMUNICATION_SINK;
        Pins[0].KsPinDescriptor.Category = &KSCATEGORY_Audio;
        Pins[0].KsPinDescriptor.Name = &KSAUDFNAME_Pc_Speaker;
    }
    else
    {
        Pins[0].KsPinDescriptor.DataFlow = KSPIN_DATAFLOW_OUT;
        Pins[0].KsPinDescriptor.Communication = KSPIN_COMMUNICATION_SINK;
        Pins[0].KsPinDescriptor.Category = &PINNAME_Capture;
        Pins[0].KsPinDescriptor.Name = &KSAUDFNAME_Recording_Control;
    }

    // now init the bridge pin
    Pins[1].MaxGlobalInstanceCount = 0;
    Pins[1].MaxFilterInstanceCount = 1;
    Pins[1].MinFilterInstanceCount = 0;
    Pins[1].KsPinDescriptor.InterfacesCount = 1;
    Pins[1].KsPinDescriptor.Interfaces = &StandardPinInterface;
    Pins[1].KsPinDescriptor.MediumsCount = 1;
    Pins[1].KsPinDescriptor.Mediums = &StandardPinMedium;
    Pins[1].KsPinDescriptor.DataRangesCount = 1;
    Pins[1].KsPinDescriptor.DataRanges =
        (PKSDATARANGE *)ExAllocatePoolZero(NonPagedPool, sizeof(PKSDATARANGE *) * 1, TAG_HDAUDIO);
    if (!Pins[1].KsPinDescriptor.DataRanges)
    {
        ExFreePool(Pins);
        ExFreePool(Description);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PKSDATARANGE BridgeAudioFormat =
        (PKSDATARANGE)ExAllocatePoolZero(NonPagedPool, sizeof(KSDATARANGE_AUDIO), TAG_HDAUDIO);
    if (!BridgeAudioFormat)
    {
        ExFreePool(Pins);
        ExFreePool(Description);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    reinterpret_cast<PKSDATARANGE>(Pins[1].KsPinDescriptor.DataRanges[0]) = BridgeAudioFormat;

    BridgeAudioFormat->FormatSize = sizeof(KSDATARANGE);
    BridgeAudioFormat->Flags = 0;
    BridgeAudioFormat->SampleSize = 0;
    BridgeAudioFormat->Reserved = 0;
    BridgeAudioFormat->MajorFormat = {STATIC_KSDATAFORMAT_TYPE_AUDIO};
    BridgeAudioFormat->SubFormat = {STATIC_KSDATAFORMAT_SUBTYPE_ANALOG};
    BridgeAudioFormat->Specifier = {STATIC_KSDATAFORMAT_SPECIFIER_NONE};
    if (NodeType == 0x0)
    {
        Pins[1].KsPinDescriptor.DataFlow = KSPIN_DATAFLOW_OUT;
    }
    else
    {
        Pins[1].KsPinDescriptor.DataFlow = KSPIN_DATAFLOW_IN;
    }
    Pins[1].KsPinDescriptor.Communication = KSPIN_COMMUNICATION_NONE;
    Pins[1].KsPinDescriptor.Category = &KSCATEGORY_RangeAudio;
    Pins[1].KsPinDescriptor.Name = NULL;

    Description->NodeSize = sizeof(PCNODE_DESCRIPTOR);
    Description->NodeCount = 1;

    if (NodeType == 0x00)
    {
        Description->Nodes = DacNode;
        Description->ConnectionCount = sizeof(DacConnections) / sizeof(PCCONNECTION_DESCRIPTOR);
        Description->Connections = DacConnections;

        Description->CategoryCount = 2;
        Description->Categories = DacCategories;
    }
    else
    {
        Description->Nodes = AdcNode;
        Description->ConnectionCount = sizeof(AdcConnections) / sizeof(PCCONNECTION_DESCRIPTOR);
        Description->Connections = AdcConnections;

        Description->CategoryCount = 2;
        Description->Categories = AdcCategories;
    }

    *OutDescription = Description;
    return STATUS_SUCCESS;
}
NTSTATUS
NTAPI
CAdapterCommon::BuildWaveInFilter(
    IN ULONG AssociatedPinCount,
    IN PULONG AssociatedPins,
    IN PVOID Node,
    OUT PPCFILTER_DESCRIPTOR *OutDescription)
{
    return BuildFilter(Node, AssociatedPinCount, AssociatedPins, OutDescription);
}

NTSTATUS
NTAPI
CAdapterCommon::BuildWaveOutFilter(
    IN ULONG AssociatedPinCount,
    IN PULONG AssociatedPins,
    IN PVOID Node,
    OUT PPCFILTER_DESCRIPTOR *OutDescription)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    ULONG NodeIndex;
    NTSTATUS Status;

    ULONG TotalInputOutputWidgets = 0;
    Status = OutNode->GetNodesWithType(0x00, &TotalInputOutputWidgets, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    for (ULONG Index = 0; Index < AssociatedPinCount; Index++)
    {
        DPRINT1("AssociatedNode %u\n", AssociatedPins[Index]);
    }
    PULONG TargetWidgets =
        (PULONG)ExAllocatePoolZero(NonPagedPool, sizeof(ULONG) * TotalInputOutputWidgets, TAG_HDAUDIO);

    if (!TargetWidgets)
        return STATUS_INSUFFICIENT_RESOURCES;

    ULONG SubNodeCount = 0;
    for (NodeIndex = 0; NodeIndex < AssociatedPinCount; NodeIndex++)
    {
        // FIXME support digital pins
        ULONG NodesFound = 0;
        Status = FindConnectedWidgets(
            0x00, 0, AssociatedPins[NodeIndex], Node, SubNodeCount, TotalInputOutputWidgets, &NodesFound, TargetWidgets,
            FALSE);
        if (NT_SUCCESS(Status))
        {
            SubNodeCount += NodesFound;
        }
    }
    DPRINT1("Total I/O Nodes Found %u\n", SubNodeCount);
    if (SubNodeCount == 0)
    {
        // nothing found or already processed
        return STATUS_UNSUCCESSFUL;
    }
    return BuildFilter(Node, SubNodeCount, TargetWidgets, OutDescription);
}
VOID NTAPI
CAdapterCommon::ClearRef(IN ULONG RefValue, IN ULONG NodeCount, IN PULONG Nodes)
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
CAdapterCommon::Initialize(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status;

    // first acquire HDAUDIO bus interface from hdaudbus
    Status = AcquireBusInterface(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Failed to acquire bus interface Status %x\n", Status);
        return Status;
    }
    // get resource information
    GetResourceInformation();
    DPRINT1("HDAUDIO: CodecAddress %u FunctionGroupStartNode %u\n", m_CodecAddress, m_FunctionGroupStartNode);

    // get vendor revision
    Status = GetVendorRevisionId();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Failed to get vendor revision %x\n", Status);
        return Status;
    }
#if 1
    // transfer init verbs
    Status = TransferInitVerbs(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: TransferInitVerbs failed %x\n", Status);
        return Status;
    }
#endif
    // now enum widgets
    CFunctionGroupNode *OutNode = nullptr;
    Status = HDAUDIO_EnumFunctionGroupWidgets(m_CodecAddress, m_FunctionGroupStartNode, this, &OutNode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: HDAUDIO_EnumFunctionGroupWidgets failed %x\n", Status);
        return Status;
    }
    // now get number of input / output widgets
    ULONG OutputNodeCount = 0;
    ULONG InputNodeCount = 0;
    PULONG OutputNodes = 0;
    PULONG InputNodes = 0;
    Status = OutNode->GetNodesWithType(0x00, &OutputNodeCount, &OutputNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed for output node with %x\n", Status);
        delete OutNode;
        return Status;
    }

    Status = OutNode->GetNodesWithType(0x01, &InputNodeCount, &InputNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed for input node with %x\n", Status);
        delete OutNode;
        ExFreePool(OutputNodes);
        return Status;
    }
    ULONG PinNodeCount = 0;
    PULONG PinNodes = NULL;
    Status = OutNode->GetNodesWithType(0x04, &PinNodeCount, &PinNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed for pin node with %x\n", Status);
        delete OutNode;
        ExFreePool(InputNodes);
        ExFreePool(OutputNodes);
        return Status;
    }
    DPRINT1(
        "HDAUDIO: InputNodeCount %u OutputNodeCount %u PinNodeCount %u\n", InputNodeCount, OutputNodeCount,
        PinNodeCount);

    OutNode->ClearVisitedState();
    Status = ProcessOutputNodes(DeviceObject, Irp, PinNodeCount, PinNodes, (PVOID)OutNode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: ProcessOutputNodes failed for input node with %x\n", Status);
        delete OutNode;
        ExFreePool(InputNodes);
        ExFreePool(OutputNodes);
        ExFreePool(PinNodes);
        return Status;
    }
    OutNode->ClearVisitedState();
    DPRINT1("ProcessInputNodes----------------------------------\n");
    Status = ProcessInputNodes(DeviceObject, Irp, PinNodeCount, PinNodes, (PVOID)OutNode, InputNodeCount, InputNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: ProcessInputNodes failed for input node with %x\n", Status);
        delete OutNode;
        ExFreePool(InputNodes);
        ExFreePool(OutputNodes);
        ExFreePool(PinNodes);
        return Status;
    }
    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::ProcessOutputNodes(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG PinNodeCount,
    IN PULONG PinNodes,
    IN PVOID Node)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;

    for (ULONG NodeIndex = 0; NodeIndex < PinNodeCount; NodeIndex++)
    {
        if (PinNodes[NodeIndex] == (ULONG)-1)
        {
            // already handled by association
            DPRINT1("PinNode %u already handled\n", PinNodes[NodeIndex]);
            continue;
        }
        PNODE_CONTEXT NodeContext = OutNode->FindNodeId(PinNodes[NodeIndex]);
        if (NodeContext && NodeContext->ConnectionCount)
        {
#if 0
                for (ULONG CIndex = 0; CIndex < NodeContext->ConnectionCount; CIndex++)
                {
                    DPRINT1(
                        "HDAUDIO PinNode %u TotalConnectionCount %u connected to %u\n", PinNodes[NodeIndex],
                        NodeContext->ConnectionCount, NodeContext->Connections[CIndex]);
                }
#endif
        }
        else
        {
            // pin node not connected to anything
            // ignore
            DPRINT1("PinNode %u not connected\n", PinNodes[NodeIndex]);
            continue;
        }
        PIN_CAPABILITIES PinCaps;
        Status = OutNode->GetPinCapabilities(PinNodes[NodeIndex], &PinCaps);
        ULONG DevicePresent = 0;
        if (NT_SUCCESS(Status))
        {
            DPRINT1(
                "HDAUDIO: PinNode %u EAPD % x Input %x Output %x PresenceDetectCapable %x\n", PinNodes[NodeIndex],
                PinCaps.EAPDCapable, PinCaps.InputCapable, PinCaps.OutputCapable, PinCaps.PresenceDetectCapable);

            if (PinCaps.PresenceDetectCapable)
            {
                Status = OutNode->GetPinSense(PinNodes[NodeIndex], &DevicePresent);
                if (NT_SUCCESS(Status))
                {
                    DPRINT1("HDAUDIO: PinNode %u DevicePresent %x\n", PinNodes[NodeIndex], DevicePresent);
                    if (!DevicePresent)
                    {
                        // FIXME ignoring device
                        // setup unsolicited response pinnode to activate ondemand
                        // continue;
                    }
                }
            }
            if (PinCaps.EAPDCapable)
            {
                // enable EAPD
                DPRINT1("Enable EAPD...\n");
                Status = OutNode->SetEAPD(PinNodes[NodeIndex], TRUE);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Warning: SetEAPD failed with %x\n", Status);
                }
            }
        }
        else
        {
            DPRINT1("PinNode %u failed to retrieve pin caps Staus %x\n", PinNodes[NodeIndex], Status);
            continue;
        }
        // retrieve pin configuration
        PIN_CONFIGURATION_DEFAULT PinConfiguration;
        Status = OutNode->GetPinConfigurationDefault(PinNodes[NodeIndex], &PinConfiguration);
        if (NT_SUCCESS(Status))
        {
            if (PinConfiguration.PortConnectivity == 0x1)
            {
                // no connection
                DPRINT1("Ignoring PinNode %u\n", PinNodes[NodeIndex]);
                ClearRef(PinNodes[NodeIndex], PinNodeCount, PinNodes);
                continue;
            }

            DPRINT1(
                "HDAUDIO: PinNode %u PortConnectivity %x DefaultDevice %x DefaultAssociation %x Sequence %x\n",
                PinNodes[NodeIndex], PinConfiguration.PortConnectivity, PinConfiguration.DefaultDevice,
                PinConfiguration.DefaultAssociation, PinConfiguration.Sequence);
            // FIXME support digital pins
            Status = AssociatePins(
                DeviceObject, Irp, PinConfiguration.DefaultAssociation, (PVOID)OutNode, PinNodeCount, PinNodes,
                FALSE);

        }
        else
        {
            DPRINT1("PinNode %u failed to retrieve config defaults Status %x\n", PinNodes[NodeIndex], Status);
        }
    }
    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::IsInputNodeConnectedToPin(
    IN PVOID Node,
    IN UCHAR Digital,
    ULONG InputNodeId,
    ULONG PinNodeId,
    ULONG PinNodeCount,
    OUT PULONG Result)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;

    PULONG ConnectedPins = (PULONG)ExAllocatePoolZero(NonPagedPool, sizeof(ULONG) * PinNodeCount, TAG_HDAUDIO);
    if (!ConnectedPins)
        return STATUS_INSUFFICIENT_RESOURCES;

    OutNode->ClearVisitedState();    
    ULONG FoundNodes = 0;
    NTSTATUS Status = FindConnectedWidgets(0x04, 0, InputNodeId, Node, 0, PinNodeCount, &FoundNodes, ConnectedPins, Digital);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(ConnectedPins);
        return Status;
    }
    *Result = 0;
    DPRINT1("Searched for %u FoundNodes %u\n", PinNodeId, FoundNodes);
    for (ULONG Index = 0; Index < FoundNodes; Index++)
    {
        if (ConnectedPins[Index] == PinNodeId)
        {
            DPRINT1("Found\n");
            *Result = 1;
            break;
        }
    }
    ExFreePool(ConnectedPins);
    return STATUS_SUCCESS;
}

VOID
NTAPI
CAdapterCommon::CollectPinWithType(
    IN ULONG DeviceType,
    IN ULONG PinNodeCount,
    IN PULONG PinIds,
    IN PULONG DefaultDeviceTypeList,
    OUT PULONG ResultListCount,
    OUT PULONG ResultList)
{
    ULONG Count = 0;
    for (ULONG Index = 0; Index < PinNodeCount; Index++)
    {
        if (DefaultDeviceTypeList[Index] == DeviceType)
            Count++;
    }
    *ResultListCount = Count;
    if (ResultList)
    {
        Count = 0;
        for (ULONG Index = 0; Index < PinNodeCount; Index++)
        {
            if (DefaultDeviceTypeList[Index] == DeviceType)
            {
                ResultList[Count] = PinIds[Index];
                Count++;
            }
        }
    }
}

NTSTATUS
NTAPI
CAdapterCommon::ProcessInputNodes(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG PinNodeCount,
    IN PULONG PinNodes,
    IN PVOID Node,
    ULONG InputNodeCount,
    PULONG InputNodes)
{
    NTSTATUS Status;
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;

    PULONG FilteredPinNodes = (PULONG)ExAllocatePoolZero(NonPagedPool, sizeof(ULONG) * PinNodeCount, TAG_HDAUDIO);
    if (!FilteredPinNodes)
        return STATUS_INSUFFICIENT_RESOURCES;

    PULONG DefaultDeviceList = (PULONG)ExAllocatePoolZero(NonPagedPool, sizeof(ULONG) * PinNodeCount, TAG_HDAUDIO);
    if (!DefaultDeviceList)
    {
        ExFreePool(FilteredPinNodes);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ULONG FilteredPinNodeCount = 0;

    // filter pin nodes which are not input capable or default device is not input
    for (ULONG NodeIndex = 0; NodeIndex < PinNodeCount; NodeIndex++)
    {
        if (PinNodes[NodeIndex] == (ULONG)-1)
            continue;

        PIN_CAPABILITIES PinCaps;
        Status = OutNode->GetPinCapabilities(PinNodes[NodeIndex], &PinCaps);
        if (!NT_SUCCESS(Status) || !PinCaps.InputCapable)
        {
            // ignoring
            DPRINT1("Ignoring Pin %u\n", PinNodes[NodeIndex]);
            continue;
        }
        // retrieve pin configuration
        PIN_CONFIGURATION_DEFAULT PinConfiguration;
        Status = OutNode->GetPinConfigurationDefault(PinNodes[NodeIndex], &PinConfiguration);
        if (!NT_SUCCESS(Status) || PinConfiguration.DefaultDevice < 0x8)
        {
            // ignoring
            DPRINT1("Ignoring Pin %u DefaultDevice %u\n", PinNodes[NodeIndex], PinConfiguration.DefaultDevice);
            continue;
        }
        DPRINT1("Adding Input Pin %u\n", PinNodes[NodeIndex]);
        FilteredPinNodes[FilteredPinNodeCount] = PinNodes[NodeIndex];
        DefaultDeviceList[FilteredPinNodeCount] = PinConfiguration.DefaultDevice;
        FilteredPinNodeCount++;
    }

    for (ULONG DeviceType = 8; DeviceType < 16; DeviceType++)
    {
        PULONG PinsOfSameType = (PULONG)ExAllocatePoolZero(NonPagedPool, sizeof(ULONG) * FilteredPinNodeCount, TAG_HDAUDIO);
        if (!PinsOfSameType)
        {
            ExFreePool(FilteredPinNodes);
            ExFreePool(DefaultDeviceList);
            return Status;
        }
        ULONG PinsOfSameTypeCount = 0;
        CollectPinWithType(
            DeviceType, FilteredPinNodeCount, FilteredPinNodes, DefaultDeviceList, &PinsOfSameTypeCount,
            PinsOfSameType);
        if (!PinsOfSameTypeCount)
        {
            // no pins of that type
            DPRINT1("No Pins of Type %u\n", DeviceType);
            ExFreePool(PinsOfSameType);
            continue;
        }

        PULONG FilteredInputNodes = (PULONG)ExAllocatePoolZero(NonPagedPool, sizeof(ULONG) * InputNodeCount, TAG_HDAUDIO);
        if (!FilteredInputNodes)
        {
            ExFreePool(PinsOfSameType);
            ExFreePool(FilteredPinNodes);
            ExFreePool(DefaultDeviceList);
            return Status;
        }
        ULONG FilteredInputNodeCount = 0;
        for (ULONG InputIndex = 0; InputIndex < InputNodeCount; InputIndex++)
        {
            ULONG Result = 0;
            for (ULONG PinIndex = 0; PinIndex < PinsOfSameTypeCount; PinIndex++)
            {
                Status = IsInputNodeConnectedToPin(
                    OutNode,
                    0x0, // no digital yet
                    InputNodes[InputIndex], PinsOfSameType[PinIndex], PinNodeCount, &Result);
                if (!NT_SUCCESS(Status))
                {
                    ExFreePool(PinsOfSameType);
                    ExFreePool(FilteredPinNodes);
                    ExFreePool(DefaultDeviceList);
                    ExFreePool(FilteredInputNodes);
                    return Status;
                }

                if (Result)
                    break;
            }
            if (Result)
            {
                FilteredInputNodes[FilteredInputNodeCount] = InputNodes[InputIndex];
                FilteredInputNodeCount++;
            }
        }
        if (FilteredInputNodeCount && PinsOfSameTypeCount)
        {
            Status = BuildInstallFilter(
                DeviceObject, Irp, FALSE, OutNode, FilteredInputNodeCount, FilteredInputNodes, PinsOfSameTypeCount,
                PinsOfSameType);
        }
        else
        {
            DPRINT1("FilteredInputNodeCount %u PinsOfSameTypeCount %u\n", FilteredInputNodeCount, PinsOfSameTypeCount);
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CAdapterCommon::BuildInstallFilter(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG bOutput,
    IN PVOID Node,
    IN ULONG AssociatedPinCount,
    IN PULONG AssociatedPins,
    IN ULONG PinNodeCount,
    IN PULONG Pins)
{
    PPCFILTER_DESCRIPTOR FilterDescription = NULL;
    NTSTATUS Status;
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;

    if (bOutput)
    {
        Status = BuildWaveOutFilter(AssociatedPinCount, AssociatedPins, (PVOID)OutNode, &FilterDescription);
    }
    else
    {
        Status = BuildWaveInFilter(AssociatedPinCount, AssociatedPins, (PVOID)OutNode, &FilterDescription);
    }

    if (NT_SUCCESS(Status))
    {
        UCHAR DefaultDevice = 0;
        UCHAR bSet = FALSE;
        PIN_CONFIGURATION_DEFAULT PinConfiguration;
        ULONG Count = bOutput ? AssociatedPinCount : PinNodeCount;
        for (ULONG Index = 0; Index < Count; Index++)
        {

            if (bOutput)
            {
                if (AssociatedPins[Index] == (ULONG)-1)
                    continue;

                Status = OutNode->GetPinConfigurationDefault(AssociatedPins[Index], &PinConfiguration);
            }
            else
            {
                if (Pins[Index] == (ULONG)-1)
                    continue;
                Status = OutNode->GetPinConfigurationDefault(Pins[Index], &PinConfiguration);
            }
            if (!NT_SUCCESS(Status))
                continue;

            if (bSet)
            {
                ASSERT(DefaultDevice == PinConfiguration.DefaultDevice);
            }
            else
            {
                bSet = TRUE;
                DefaultDevice = PinConfiguration.DefaultDevice;
            }
        }

        // lets build a subdevice name
        LPWSTR SubdeviceWave = NULL;
        Status = ChooseSubdeviceName(DefaultDevice, TRUE, &SubdeviceWave);
        if (NT_SUCCESS(Status))
        {
            // construct wavert port
            DPRINT1("HDAUDIO: SubdeviceName %S\n", SubdeviceWave);
            Status = InstallDevice(
                DeviceObject, Irp, SubdeviceWave, AssociatedPinCount, AssociatedPins, (PVOID)OutNode,
                FilterDescription);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("HDAUDIO: InstallDevice failed with %x\n", Status);
                return Status;
            }
            // lets build a subdevice name
            LPWSTR SubdeviceTopology = NULL;
            Status = ChooseSubdeviceName(DefaultDevice, FALSE, &SubdeviceTopology);
            if (NT_SUCCESS(Status))
            {
                // build topology port
                // InstallTopology(DeviceObject, Irp, SubdeviceTopology);
            }
        }
    }

    return Status;
}

NTSTATUS
NTAPI
CAdapterCommon::AssociatePins(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN UCHAR DefaultAssociation,
    IN PVOID Node,
    IN ULONG PinNodeCount,
    IN PULONG PinNodes,
    IN UCHAR Digital)
{
    CFunctionGroupNode *OutNode = (CFunctionGroupNode *)Node;
    NTSTATUS Status;

    ULONG AssociatedPinCount = 0;
    PULONG AssociatedPins = NULL;

    Status =
        OutNode->GetPinNodesWithDefaultAssociation(DefaultAssociation, Digital, &AssociatedPinCount, &AssociatedPins);
    if (NT_SUCCESS(Status))
    {
        Status = BuildInstallFilter(DeviceObject, Irp, TRUE, OutNode, AssociatedPinCount, AssociatedPins, 0, NULL);
    }
    return Status;
}

NTSTATUS
HDAUDIO_AllocateCommonAdapter(OUT CAdapterCommon **OutAdapter)
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
HDAUDIO_InitializeCommonAdapter(CAdapterCommon *CommonAdapter, IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status;

    // init device
    Status = CommonAdapter->Initialize(DeviceObject, Irp);

    // done
    return Status;
}
