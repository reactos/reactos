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

        LONG ChannelMapping;
        if (NodeContext->Digital || PinConfiguration.DefaultDevice > 0x7)
        {
            // not an output pin or digital pin
            ChannelMapping = 0;
        }
        else
        {
            // output (analog) pin
            switch (NodeContext->ChannelCount)
            {
                case 1:
                    ChannelMapping = KSAUDIO_SPEAKER_MONO;
                    break;
                case 2:
                    ChannelMapping = KSAUDIO_SPEAKER_STEREO;
                    break;
                case 4:
                    ChannelMapping = KSAUDIO_SPEAKER_QUAD;
                    break;
                case 6:
                    ChannelMapping = KSAUDIO_SPEAKER_5POINT1;
                    break;
                case 8:
                    ChannelMapping = KSAUDIO_SPEAKER_7POINT1;
                    break;
                default:
                    ChannelMapping = KSAUDIO_SPEAKER_DIRECTOUT;
                    break;
            }
        }

        JackDescription->ChannelMapping = ChannelMapping;

        ULONG Color;
        switch (PinConfiguration.Color)
        {
            case 0x1:
                Color = 0x00000000; // black
                break;
            case 0x2:
                Color = 0x80808000; // grey
                break;
            case 0x3:
                Color = 0x0000FF00; // blue
                break;
            case 0x4:
                Color = 0x00800000; // green
                break;
            case 0x5:
                Color = 0xFF000000; // red
                break;
            case 0x6:
                Color = 0xFFA50000; // orange
                break;
            case 0x7:
                Color = 0xFFFF0000; // yellow
                break;
            case 0x8:
                Color = 0x80008000; // purple
                break;
            case 0x9:
                Color = 0xFF80ED00; // pink
                break;
            case 0xE:
                Color = 0xFFFFFF00; // white
                break;
            case 0x0:
            case 0xF:
            default:
                Color = 0x00000000; // unknown (black)
                break;
        }

        JackDescription->Color = Color;
        JackDescription->ConnectionType = (EPcxConnectionType)PinConfiguration.ConnectionType;
        JackDescription->GeoLocation = (EPcxGeoLocation)PinConfiguration.Location;
        JackDescription->GenLocation = (EPcxGenLocation)(PinConfiguration.Location >> 4);

        EPxcPortConnection PortConnection;
        switch (PinConfiguration.PortConnectivity)
        {
            case 0:
                PortConnection = ePortConnJack; // jack
                break;
            case 2:
                PortConnection = ePortConnIntegratedDevice; // integrated device
                break;
            case 3:
                PortConnection = ePortConnBothIntegratedAndJack; // both integrated and jack
                break;
            case 1:
            default:
                PortConnection = ePortConnUnknown; // no connection / unknown connection
                break;
        }

        JackDescription->PortConnection = PortConnection;
        JackDescription->IsConnected = TRUE; // PinConfiguration.PortConnectivity != 0x1; // FIXME
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
        if (PropertyRequest->Node == 0)
        {
            ChannelConfig->ActiveSpeakerPositions = KSAUDIO_SPEAKER_STEREO;
            Miniport->Release();
            return STATUS_SUCCESS;
        }

        PNODE_CONTEXT NodeContext = Node->FindNodeId(PropertyRequest->Node);
        if (!NodeContext)
        {
            DPRINT1("HDAUDIO: no node context on node %u\n", PropertyRequest->Node);
            Miniport->Release();
            return STATUS_UNSUCCESSFUL;
        }

        LONG Value;
        switch (NodeContext->ChannelCount)
        {
            case 1:
                Value = KSAUDIO_SPEAKER_MONO;
                break;
            case 2:
                Value = KSAUDIO_SPEAKER_STEREO;
                break;
            case 4:
                Value = KSAUDIO_SPEAKER_QUAD;
                break;
            case 6:
                Value = KSAUDIO_SPEAKER_5POINT1;
                break;
            case 8:
                Value = KSAUDIO_SPEAKER_7POINT1;
                break;
            default:
                Value = KSAUDIO_SPEAKER_DIRECTOUT;
                break;
        }

        ChannelConfig->ActiveSpeakerPositions = Value;
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
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        PULONG AccessFlags = (PULONG)PropertyRequest->Value;
        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }

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

    ULONG OutputNodeCount = 0;
    PULONG OutputNodes = 0;
    Status = Node->GetNodesWithType(0x00, &OutputNodeCount, &OutputNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed with %x\n", Status);
        Miniport->Release();
        return Status;
    }

    for (ULONG NodeIndex = 0; NodeIndex < OutputNodeCount; NodeIndex++)
    {
        AMPLIFIER_CAPABILITIES AmplifierCapabilities;
        Status = Node->GetAmplifierDetails(OutputNodes[NodeIndex], 0, &AmplifierCapabilities);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HDAUDIO: GetAmplifierDetails failed status %x, Node %d\n", Status, OutputNodes[NodeIndex]);
            continue;
        }

        double DBStep = (AmplifierCapabilities.Steps + 1) * 0.25;
        double DBMax = (AmplifierCapabilities.NumSteps - AmplifierCapabilities.Offset) * DBStep;
        double DBMin = -AmplifierCapabilities.Offset * DBStep;
        double DBRange = DBMax - DBMin;
        DPRINT1("Step %f, Max %f, Min %f, Range %f\n", DBStep, DBMax, DBMin, DBRange);
        UCHAR Gain, Mute;
        PLONG Value = (PLONG)PropertyRequest->Value;
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            Status = Node->GetAmplifierGainMute(OutputNodes[NodeIndex], FALSE, &Mute, &Gain);
            DPRINT1("GetAmplifierGainMute Status %x, Node %d, Mute %x, Gain %d\n", Status, OutputNodes[NodeIndex], Mute, Gain);
            double DBVol = DBMin + ((double)Gain / (double)AmplifierCapabilities.NumSteps * DBRange);
            *Value = (LONG)(DBVol * 0x10000);
            DPRINT1("Resulting value %d\n", *Value);
            PropertyRequest->ValueSize = sizeof(LONG);
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            DPRINT1("Incoming value %d\n", *Value);
            double DBVol = (double)*Value / 0x10000;
            double PercentVol = (DBVol - DBMin) / DBRange;
            LONG Step = (LONG)(PercentVol * AmplifierCapabilities.NumSteps + 0.5);
            Gain = (UCHAR)Step;
            AMPLIFIER_GAIN_MUTE_SET Set;
            Set.Output = 0x1;
            Set.Input = 0x1;
            Set.Left = 0x1;
            Set.Right = 0x1;
            Set.Index = 0;
            Set.Mute = 0x0;
            Set.Gain = Gain;
            Status = Node->SetAmplifierGainMute(OutputNodes[NodeIndex], &Set);
            DPRINT1("SetAmplifierGainMute Status %x, Node %d, Gain %d\n", Status, OutputNodes[NodeIndex], Gain);
        }
    }
    Miniport->Release();
    return Status;
}

NTSTATUS
NTAPI
PropertyHandler_Mute(IN PPCPROPERTY_REQUEST PropertyRequest)
{
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        PULONG AccessFlags = (PULONG)PropertyRequest->Value;
        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }

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

    ULONG OutputNodeCount = 0;
    PULONG OutputNodes = 0;
    Status = Node->GetNodesWithType(0x00, &OutputNodeCount, &OutputNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed with %x\n", Status);
        Miniport->Release();
        return Status;
    }

    for (ULONG NodeIndex = 0; NodeIndex < OutputNodeCount; NodeIndex++)
    {
        AMPLIFIER_CAPABILITIES AmplifierCapabilities;
        Status = Node->GetAmplifierDetails(OutputNodes[NodeIndex], 0, &AmplifierCapabilities);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("GetAmplifierDetails Status %x, Node %d\n", Status, OutputNodes[NodeIndex]);
            continue;
        }

        if (!AmplifierCapabilities.MuteCapable)
        {
            DPRINT1("HDAUDIO: Mute bit is not supported by hardware, Node %d\n", OutputNodes[NodeIndex]);
            Status = STATUS_NOT_SUPPORTED;
            continue;
        }

        UCHAR Gain, Mute;
        PBOOL Value = (PBOOL)PropertyRequest->Value;
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            Status = Node->GetAmplifierGainMute(OutputNodes[NodeIndex], FALSE, &Mute, &Gain);
            DPRINT1("GetAmplifierGainMute Status %x, Node %d, Mute %x, Gain %d\n", Status, OutputNodes[NodeIndex], Mute, Gain);
            *Value = Mute;
            PropertyRequest->ValueSize = sizeof(BOOL);
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            AMPLIFIER_GAIN_MUTE_SET Set;
            Mute = *Value;
            Set.Output = 0x1;
            Set.Input = 0x1;
            Set.Left = 0x1;
            Set.Right = 0x1;
            Set.Index = 0;
            Set.Mute = Mute;
            Set.Gain = Mute ? 0 : AmplifierCapabilities.NumSteps;
            Status = Node->SetAmplifierGainMute(OutputNodes[NodeIndex], &Set);
            DPRINT1("SetAmplifierGainMute Status %x, Node %d, Mute %x\n", Status, OutputNodes[NodeIndex], Mute);
        }
    }
    Miniport->Release();
    return Status;
}

NTSTATUS
NTAPI
EventHandler_Volume(IN PPCEVENT_REQUEST EventRequest)
{
    PUNKNOWN UnknownMiniport = (PUNKNOWN)EventRequest->MajorTarget;
    if (!UnknownMiniport)
        return STATUS_INVALID_PARAMETER;

    CMiniportTopology *Miniport = NULL;
    NTSTATUS Status = UnknownMiniport->QueryInterface(IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status) || !Miniport)
        return Status;

    switch (EventRequest->Verb)
    {
        case PCEVENT_VERB_ADD:
            if (EventRequest->EventEntry)
            {
                Miniport->AddEventToEventList(EventRequest->EventEntry);
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_UNSUCCESSFUL;
            }
            break;
        case PCEVENT_VERB_REMOVE:
        case PCEVENT_VERB_SUPPORT:
            Status = STATUS_SUCCESS;
            break;
        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    Miniport->Release();
    return Status;
}
