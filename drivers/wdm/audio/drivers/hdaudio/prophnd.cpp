/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Property handlers
 * COPYRIGHT:       Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

// FIXME: halfplemented

NTSTATUS
NTAPI
PropertyHandler_JackDescription(IN PPCPROPERTY_REQUEST PropertyRequest)
{
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        // handle basic support
        if (PropertyRequest->ValueSize < sizeof(ULONG))
        {
            PropertyRequest->ValueSize = sizeof(ULONG);
            return STATUS_BUFFER_TOO_SMALL;
        }

        PULONG AccessFlags = (PULONG)PropertyRequest->Value;
        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET;
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }

    // only get request is supported
    if (!(PropertyRequest->Verb & KSPROPERTY_TYPE_GET))
        return STATUS_NOT_SUPPORTED;

    PUNKNOWN UnknownMiniport = (PUNKNOWN)PropertyRequest->MajorTarget;
    if (!UnknownMiniport)
        return STATUS_INVALID_PARAMETER;

    CMiniportTopology *Miniport = NULL;
    NTSTATUS Status = UnknownMiniport->QueryInterface(IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status) || !Miniport)
        return Status;

    CFunctionGroupNode *Node = (CFunctionGroupNode*)Miniport->GetNode();
    if (!Node)
    {
        Miniport->Release();
        return STATUS_INVALID_PARAMETER;
    }

    ULONG PinNodeCount;
    PULONG PinNodes;
    Node->ClearVisitedState();
    Status = Node->GetNodesWithType(0x04, &PinNodeCount, &PinNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed with %x\n", Status);
        Miniport->Release();
        return Status;
    }

    const ULONG JackDescriptionSize = sizeof(KSMULTIPLE_ITEM) + PinNodeCount * sizeof(KSJACK_DESCRIPTION);

    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = JackDescriptionSize;
        ExFreePool(PinNodes);
        Miniport->Release();
        return STATUS_BUFFER_OVERFLOW;
    }

    if (PropertyRequest->ValueSize < JackDescriptionSize)
    {
        PropertyRequest->ValueSize = JackDescriptionSize;
        ExFreePool(PinNodes);
        Miniport->Release();
        return STATUS_BUFFER_TOO_SMALL;
    }

    PKSMULTIPLE_ITEM MultipleItem = (PKSMULTIPLE_ITEM)PropertyRequest->Value;
    MultipleItem->Size = JackDescriptionSize;
    MultipleItem->Count = PinNodeCount;
    PKSJACK_DESCRIPTION JackDescription = (PKSJACK_DESCRIPTION)(MultipleItem + 1);

    for (ULONG NodeIndex = 0; NodeIndex < PinNodeCount; NodeIndex++)
    {
        PIN_CONFIGURATION_DEFAULT PinConfiguration;
        Status = Node->GetPinConfigurationDefault(PinNodes[NodeIndex], &PinConfiguration);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: GetPinConfigurationDefault failed with %x for node %u\n", Status, PinNodes[NodeIndex]);
            JackDescription++;
            continue;
        }

        PNODE_CONTEXT NodeContext = Node->FindNodeId(PinNodes[NodeIndex]);
        if (!NodeContext)
        {
            DPRINT1("HDAUDIO: no node context for node %u\n", PinNodes[NodeIndex]);
            JackDescription++;
            continue;
        }

        if (NodeContext->NodeType != 0x04)
        {
            DPRINT1("HDAUDIO: node %u is not a pin node\n", PinNodes[NodeIndex]);
            JackDescription++;
            continue;
        }

        if (NodeContext->Digital || PinConfiguration.DefaultDevice > 0x7)
        {
            // not an output pin or digital pin
            JackDescription->ChannelMapping = 0;
        }
        else
        {
            // output (analog) pin
            JackDescription->ChannelMapping = KSAUDIO_SPEAKER_STEREO; // FIXME
        }

        JackDescription->Color = PinConfiguration.Color;
        JackDescription->ConnectionType = (EPcxConnectionType)PinConfiguration.ConnectionType;
        JackDescription->GeoLocation = (EPcxGeoLocation)PinConfiguration.Location;
        JackDescription->GenLocation = (EPcxGenLocation)(PinConfiguration.Location << 4);
        JackDescription->PortConnection = (EPxcPortConnection)PinConfiguration.PortConnectivity;
        DPRINT1("PinConfiguration.PortConnectivity %x\n", PinConfiguration.PortConnectivity);
        JackDescription->IsConnected = TRUE; // FIXME
        JackDescription++;
    }

    PropertyRequest->ValueSize = JackDescriptionSize;

    Miniport->Release();
    return Status;
}

NTSTATUS
NTAPI
PropertyHandler_ChannelConfig(IN PPCPROPERTY_REQUEST PropertyRequest)
{
    if (PropertyRequest->Node == (ULONG)-1)
        return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->ValueSize < sizeof(KSAUDIO_CHANNEL_CONFIG))
        return STATUS_BUFFER_TOO_SMALL;

    PUNKNOWN UnknownMiniport = (PUNKNOWN)PropertyRequest->MajorTarget;
    if (!UnknownMiniport)
        return STATUS_INVALID_PARAMETER;

    CMiniportWaveRT *Miniport = NULL;
    NTSTATUS Status = UnknownMiniport->QueryInterface(IID_IMiniportWaveRT, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status) || !Miniport)
        return Status;

    CFunctionGroupNode *Node = (CFunctionGroupNode*)Miniport->GetNode();
    if (!Node)
    {
        Miniport->Release();
        return STATUS_INVALID_PARAMETER;
    }

    PKSAUDIO_CHANNEL_CONFIG ChannelConfig = (PKSAUDIO_CHANNEL_CONFIG)PropertyRequest->Value;
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        UNIMPLEMENTED;
        ChannelConfig->ActiveSpeakerPositions = KSAUDIO_SPEAKER_7POINT1;
        Miniport->Release();
        return STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        UNIMPLEMENTED;
        Miniport->Release();
        return STATUS_SUCCESS;
    }
    Miniport->Release();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PropertyHandler_SpeakerGeometry(IN PPCPROPERTY_REQUEST PropertyRequest)
{
    if (PropertyRequest->Node == (ULONG)-1)
        return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->ValueSize < sizeof(LONG))
        return STATUS_BUFFER_TOO_SMALL;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        UNIMPLEMENTED;
        *(PLONG)PropertyRequest->Value = -1;
        PropertyRequest->ValueSize = sizeof(LONG);
        return STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        UNIMPLEMENTED;
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PropertyHandler_Volume(IN PPCPROPERTY_REQUEST PropertyRequest)
{
    if (PropertyRequest->Node == (ULONG)-1)
        return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->ValueSize < sizeof(LONG))
        return STATUS_BUFFER_TOO_SMALL;

    PUNKNOWN UnknownMiniport = (PUNKNOWN)PropertyRequest->MajorTarget;
    if (!UnknownMiniport)
        return STATUS_INVALID_PARAMETER;

    CMiniportTopology *Miniport = NULL;
    NTSTATUS Status = UnknownMiniport->QueryInterface(IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status) || !Miniport)
        return Status;

    CFunctionGroupNode *Node = (CFunctionGroupNode*)Miniport->GetNode();
    if (!Node)
    {
        Miniport->Release();
        return STATUS_INVALID_PARAMETER;
    }

    //PLONG Value = (PLONG)PropertyRequest->Value;
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
#if 0
        UCHAR Direct, Volume;
        Status = Node->GetVolumeKnob(PropertyRequest->Node, &Direct, &Volume);
        DPRINT1("GetVolumeKnob Status %x, Node %d, Direct %x Volume %x\n", Status, PropertyRequest->Node, Direct, Volume);
        PropertyRequest->ValueSize = sizeof(LONG);
        *Value = Volume;
#else
        Status = STATUS_NOT_IMPLEMENTED;
#endif
        Miniport->Release();
        return Status;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
#if 0
        UCHAR Volume = *Value;
        Status = Node->SetVolumeKnob(PropertyRequest->Node, 0, Volume);
        DPRINT1("SetVolumeKnob Status %x, Node %d, Volume %x\n", Status, PropertyRequest->Node, Volume);
#else
        Status = STATUS_NOT_IMPLEMENTED;
#endif
        Miniport->Release();
        return Status;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        PULONG AccessFlags = (PULONG)PropertyRequest->Value;
        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
        PropertyRequest->ValueSize = sizeof(ULONG);
        Miniport->Release();
        return STATUS_SUCCESS;
    }
    Miniport->Release();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PropertyHandler_Mute(IN PPCPROPERTY_REQUEST PropertyRequest)
{
    if (PropertyRequest->Node == (ULONG)-1)
        return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->ValueSize < sizeof(BOOL))
        return STATUS_BUFFER_TOO_SMALL;

    PUNKNOWN UnknownMiniport = (PUNKNOWN)PropertyRequest->MajorTarget;
    if (!UnknownMiniport)
        return STATUS_INVALID_PARAMETER;

    CMiniportTopology *Miniport = NULL;
    NTSTATUS Status = UnknownMiniport->QueryInterface(IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status) || !Miniport)
        return Status;

    CFunctionGroupNode *Node = (CFunctionGroupNode*)Miniport->GetNode();
    if (!Node)
    {
        Miniport->Release();
        return STATUS_INVALID_PARAMETER;
    }

#if 0
    AMPLIFIER_CAPABILITIES AmplifierCapabilities;
    Status = Node->GetAmplifierDetails(PropertyRequest->Node, 0, &AmplifierCapabilities);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetAmplifierDetails Status %x, Node %d\n", Status, PropertyRequest->Node);
        Miniport->Release();
        return Status;
    }

    if (!AmplifierCapabilities.MuteCapable)
    {
        DPRINT1("HDAUDIO: Mute bit is not supported by hardware, Node %d\n", PropertyRequest->Node);
        Miniport->Release();
        return STATUS_NOT_SUPPORTED;
    }
#endif

    //PBOOL Value = (PBOOL)PropertyRequest->Value;
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
#if 0
        UCHAR Gain, Mute;
        Status = Node->GetAmplifierGainMute(PropertyRequest->Node, 0, 0, &Mute, &Gain);
        DPRINT1("GetAmplifierGainMute Status %x, Node %d, Mute %x, Gain %x\n", Status, PropertyRequest->Node, Mute, Gain);
        PropertyRequest->ValueSize = sizeof(BOOL);
        *Value = Mute;
#else
        Status = STATUS_NOT_IMPLEMENTED;
#endif
        Miniport->Release();
        return Status;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
#if 0
        UCHAR Mute = *Value;
        Status = Node->SetAmplifierGainMute(PropertyRequest->Node, 0, 0, Mute, 0);
        DPRINT1("SetAmplifierGainMute Status %x, Node %d, Mute %x\n", Status, PropertyRequest->Node, Mute);
#else
        Status = STATUS_NOT_IMPLEMENTED;
#endif
        Miniport->Release();
        return Status;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        PULONG AccessFlags = (PULONG)PropertyRequest->Value;
        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
        PropertyRequest->ValueSize = sizeof(BOOL);
        Miniport->Release();
        return STATUS_SUCCESS;
    }
    Miniport->Release();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
EventHandler_Volume(IN PPCEVENT_REQUEST EventRequest)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
