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

        PIN_CAPABILITIES PinCaps;
        Status = Node->GetPinCapabilities(PinNodes[NodeIndex], &PinCaps);
        if (!NT_SUCCESS(Status) || (!PinCaps.InputCapable && !PinCaps.OutputCapable))
        {
            DPRINT1("Pin %u not input or output capable\n", PinNodes[NodeIndex]);
            JackDescription++;
            continue;
        }

        ULONG DevicePresent = 0;
        if (PinCaps.PresenceDetectCapable)
        {
            Status = Node->GetPinSense(PinNodes[NodeIndex], &DevicePresent);
            if (NT_SUCCESS(Status))
            {
                DPRINT("HDAUDIO: PinNode %u DevicePresent %x\n", PinNodes[NodeIndex], DevicePresent);
            }
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

        if (NodeContext->ConnectionCount == 0)
        {
            DPRINT("HDAUDIO: node %u is not connected to anything\n", PinNodes[NodeIndex]);
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
        JackDescription->IsConnected = TRUE; //PinCaps.PresenceDetectCapable ? DevicePresent : TRUE; // FIXME
        JackDescription++;
    }

    PropertyRequest->ValueSize = JackDescriptionSize;

    ExFreePool(PinNodes);
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
PropertyHandler_InputVolume(IN PPCPROPERTY_REQUEST PropertyRequest)
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

    AMPLIFIER_CAPABILITIES AmplifierCapabilities = Node->GetCachedAmplifierCapabilities();
    DPRINT("HDAUDIO: Amplifier capabilities NumSteps %u, StepSize %u, Offset %u\n",
           AmplifierCapabilities.NumSteps, AmplifierCapabilities.Steps, AmplifierCapabilities.Offset);

    ULONG InputNodeCount = 0;
    PULONG InputNodes = 0;
    Status = Node->GetNodesWithType(0x01, &InputNodeCount, &InputNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed with %x\n", Status);
        Miniport->Release();
        return Status;
    }

    for (ULONG NodeIndex = 0; NodeIndex < InputNodeCount; NodeIndex++)
    {
        PNODE_CONTEXT NodeContext = Node->FindNodeId(InputNodes[NodeIndex]);
        if (!NodeContext)
        {
            DPRINT1("HDAUDIO: No node context for node %u\n", InputNodes[NodeIndex]);
            Status = STATUS_UNSUCCESSFUL;
            continue;
        }

        double DBStep = (AmplifierCapabilities.Steps + 1) * 0.25;
        double DBMax = (AmplifierCapabilities.NumSteps - AmplifierCapabilities.Offset) * DBStep;
        double DBMin = -AmplifierCapabilities.Offset * DBStep;
        double DBRange = DBMax - DBMin;
        DPRINT("Step %f, Max %f, Min %f, Range %f\n", DBStep, DBMax, DBMin, DBRange);

        UCHAR Gain;
        PLONG Value = (PLONG)PropertyRequest->Value;
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
            {
                const ULONG BasicPropertySize = sizeof(KSPROPERTY_DESCRIPTION) +
                                                sizeof(KSPROPERTY_MEMBERSHEADER) +
                                                sizeof(KSPROPERTY_STEPPING_LONG) * NodeContext->ChannelCount;
                PKSPROPERTY_DESCRIPTION PropertyDescription = (PKSPROPERTY_DESCRIPTION)PropertyRequest->Value;
                PropertyDescription->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
                PropertyDescription->DescriptionSize = BasicPropertySize;
                PropertyDescription->PropTypeSet.Set = KSPROPTYPESETID_General;
                PropertyDescription->PropTypeSet.Id = VT_I4;
                PropertyDescription->PropTypeSet.Flags = 0;
                PropertyDescription->MembersListCount = 1;
                PropertyDescription->Reserved = 0;

                if (PropertyRequest->ValueSize >= BasicPropertySize)
                {
                    PKSPROPERTY_MEMBERSHEADER MembersHeader = (PKSPROPERTY_MEMBERSHEADER)(PropertyDescription + 1);
                    MembersHeader->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
                    MembersHeader->MembersSize = sizeof(KSPROPERTY_STEPPING_LONG);
                    MembersHeader->MembersCount = NodeContext->ChannelCount;
                    MembersHeader->Flags = 0;

                    PKSPROPERTY_STEPPING_LONG Range = (PKSPROPERTY_STEPPING_LONG)(MembersHeader + 1);
                    for (ULONG Channel = 0; Channel < NodeContext->ChannelCount; Channel++)
                    {
                        Range[Channel].Bounds.SignedMinimum = DBMin * 0x10000;
                        Range[Channel].Bounds.SignedMaximum = DBMax * 0x10000;
                        Range[Channel].SteppingDelta = DBStep * 0x10000;
                        Range[Channel].Reserved = 0;
                    }

                    PropertyRequest->ValueSize = BasicPropertySize;
                    Status = STATUS_SUCCESS;
                }
            }
            else if (PropertyRequest->ValueSize >= sizeof(ULONG))
            {
                PULONG AccessFlags = (PULONG)PropertyRequest->Value;
                *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
                PropertyRequest->ValueSize = sizeof(ULONG);
                Status = STATUS_SUCCESS;
            }
            else
            {
                DPRINT1("Buffer too small\n");
                PropertyRequest->ValueSize = 0;
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get cached value
            Gain = Node->GetCachedInputGain();
            DPRINT("Node %u, Gain %d\n", InputNodes[NodeIndex], Gain);
            double DBVol = DBMin + ((double)Gain / (double)AmplifierCapabilities.NumSteps * DBRange);
            *Value = (LONG)(DBVol * 0x10000);
            DPRINT("Resulting value %d\n", *Value);
            PropertyRequest->ValueSize = sizeof(LONG);
            Status = STATUS_SUCCESS;
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            DPRINT("Incoming value %d\n", *Value);
            double DBVol = (double)*Value / 0x10000;
            double PercentVol = (DBVol - DBMin) / DBRange;
            LONG Step = (LONG)(PercentVol * AmplifierCapabilities.NumSteps + 0.5);
            Gain = (UCHAR)Step;

            AMPLIFIER_GAIN_MUTE_SET Set;
            Set.Output = 0x0;
            Set.Input = 0x1;
            Set.Left = 0x1;
            Set.Right = 0x1;
            Set.Index = NodeContext->ConnectionCount ? NodeContext->Connections[0] : 0;
            Set.Mute = 0x0;
            Set.Gain = Gain;
            Status = Node->SetAmplifierGainMute(InputNodes[NodeIndex], &Set);
            DPRINT("SetAmplifierGainMute Status %x, Node %u, Gain %d\n", Status, InputNodes[NodeIndex], Gain);
            if (NT_SUCCESS(Status))
            {
                // cache new value
                Node->SetCachedInputGain(Gain);
            }
        }
    }
    ExFreePool(InputNodes);
    Miniport->Release();
    return Status;
}

NTSTATUS
NTAPI
PropertyHandler_InputMute(IN PPCPROPERTY_REQUEST PropertyRequest)
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

    AMPLIFIER_CAPABILITIES AmplifierCapabilities = Node->GetCachedAmplifierCapabilities();
    DPRINT("HDAUDIO: Amplifier capabilities NumSteps %u, StepSize %u, Offset %u\n",
           AmplifierCapabilities.NumSteps, AmplifierCapabilities.Steps, AmplifierCapabilities.Offset);

    if (!AmplifierCapabilities.MuteCapable)
    {
        DPRINT1("HDAUDIO: Mute bit is not supported by hardware\n");
        Miniport->Release();
        return STATUS_NOT_SUPPORTED;
    }

    ULONG InputNodeCount = 0;
    PULONG InputNodes = 0;
    Status = Node->GetNodesWithType(0x01, &InputNodeCount, &InputNodes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: GetNodesWithType failed with %x\n", Status);
        Miniport->Release();
        return Status;
    }

    for (ULONG NodeIndex = 0; NodeIndex < InputNodeCount; NodeIndex++)
    {
        PNODE_CONTEXT NodeContext = Node->FindNodeId(InputNodes[NodeIndex]);
        if (!NodeContext)
        {
            DPRINT1("HDAUDIO: No node context for node %u\n", InputNodes[NodeIndex]);
            Status = STATUS_UNSUCCESSFUL;
            continue;
        }

        UCHAR Mute;
        PBOOL Value = (PBOOL)PropertyRequest->Value;
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
            {
                const ULONG BasicPropertySize = sizeof(KSPROPERTY_DESCRIPTION) +
                                                sizeof(KSPROPERTY_MEMBERSHEADER) +
                                                sizeof(KSPROPERTY_STEPPING_LONG) * NodeContext->ChannelCount;
                PKSPROPERTY_DESCRIPTION PropertyDescription = (PKSPROPERTY_DESCRIPTION)PropertyRequest->Value;
                PropertyDescription->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
                PropertyDescription->DescriptionSize = BasicPropertySize;
                PropertyDescription->PropTypeSet.Set = KSPROPTYPESETID_General;
                PropertyDescription->PropTypeSet.Id = VT_BOOL;
                PropertyDescription->PropTypeSet.Flags = 0;
                PropertyDescription->MembersListCount = 1;
                PropertyDescription->Reserved = 0;

                if (PropertyRequest->ValueSize >= BasicPropertySize)
                {
                    PKSPROPERTY_MEMBERSHEADER MembersHeader = (PKSPROPERTY_MEMBERSHEADER)(PropertyDescription + 1);
                    MembersHeader->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
                    MembersHeader->MembersSize = sizeof(KSPROPERTY_STEPPING_LONG);
                    MembersHeader->MembersCount = 1;
                    MembersHeader->Flags = 0;

                    PKSPROPERTY_STEPPING_LONG Range = (PKSPROPERTY_STEPPING_LONG)(MembersHeader + 1);
                    Range->Bounds.SignedMinimum = 0;
                    Range->Bounds.SignedMaximum = 1;
                    Range->SteppingDelta = 1;
                    Range->Reserved = 0;

                    PropertyRequest->ValueSize = BasicPropertySize;
                    Status = STATUS_SUCCESS;
                }
            }
            else if (PropertyRequest->ValueSize >= sizeof(ULONG))
            {
                PULONG AccessFlags = (PULONG)PropertyRequest->Value;
                *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
                PropertyRequest->ValueSize = sizeof(ULONG);
                Status = STATUS_SUCCESS;
            }
            else
            {
                DPRINT1("Buffer too small\n");
                PropertyRequest->ValueSize = 0;
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get cached value
            Mute = Node->GetCachedInputMute();
            DPRINT("Node %u, Mute %x\n", InputNodes[NodeIndex], Mute);
            *Value = Mute;
            PropertyRequest->ValueSize = sizeof(BOOL);
            Status = STATUS_SUCCESS;
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            Mute = *Value;

            AMPLIFIER_GAIN_MUTE_SET Set;
            Set.Output = 0x0;
            Set.Input = 0x1;
            Set.Left = 0x1;
            Set.Right = 0x1;
            Set.Index = NodeContext->ConnectionCount ? NodeContext->Connections[0] : 0;
            Set.Mute = Mute;
            Set.Gain = Mute ? 0 : Node->GetCachedInputGain();
            Status = Node->SetAmplifierGainMute(InputNodes[NodeIndex], &Set);
            DPRINT("SetAmplifierGainMute Status %x, Node %u, Mute %x\n", Status, InputNodes[NodeIndex], Mute);
            if (NT_SUCCESS(Status))
            {
                // cache new value
                Node->SetCachedInputMute(Mute);
            }
        }
    }
    ExFreePool(InputNodes);
    Miniport->Release();
    return Status;
}

NTSTATUS
NTAPI
PropertyHandler_OutputVolume(IN PPCPROPERTY_REQUEST PropertyRequest)
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

    AMPLIFIER_CAPABILITIES AmplifierCapabilities = Node->GetCachedAmplifierCapabilities();
    DPRINT("HDAUDIO: Amplifier capabilities NumSteps %u, StepSize %u, Offset %u\n",
           AmplifierCapabilities.NumSteps, AmplifierCapabilities.Steps, AmplifierCapabilities.Offset);

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
        PNODE_CONTEXT NodeContext = Node->FindNodeId(OutputNodes[NodeIndex]);
        if (!NodeContext)
        {
            DPRINT1("HDAUDIO: No node context for node %u\n", OutputNodes[NodeIndex]);
            Status = STATUS_UNSUCCESSFUL;
            continue;
        }

        double DBStep = (AmplifierCapabilities.Steps + 1) * 0.25;
        double DBMax = (AmplifierCapabilities.NumSteps - AmplifierCapabilities.Offset) * DBStep;
        double DBMin = -AmplifierCapabilities.Offset * DBStep;
        double DBRange = DBMax - DBMin;
        DPRINT("Step %f, Max %f, Min %f, Range %f\n", DBStep, DBMax, DBMin, DBRange);

        UCHAR Gain;
        PLONG Value = (PLONG)PropertyRequest->Value;
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
            {
                const ULONG BasicPropertySize = sizeof(KSPROPERTY_DESCRIPTION) +
                                                sizeof(KSPROPERTY_MEMBERSHEADER) +
                                                sizeof(KSPROPERTY_STEPPING_LONG) * NodeContext->ChannelCount;
                PKSPROPERTY_DESCRIPTION PropertyDescription = (PKSPROPERTY_DESCRIPTION)PropertyRequest->Value;
                PropertyDescription->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
                PropertyDescription->DescriptionSize = BasicPropertySize;
                PropertyDescription->PropTypeSet.Set = KSPROPTYPESETID_General;
                PropertyDescription->PropTypeSet.Id = VT_I4;
                PropertyDescription->PropTypeSet.Flags = 0;
                PropertyDescription->MembersListCount = 1;
                PropertyDescription->Reserved = 0;

                if (PropertyRequest->ValueSize >= BasicPropertySize)
                {
                    PKSPROPERTY_MEMBERSHEADER MembersHeader = (PKSPROPERTY_MEMBERSHEADER)(PropertyDescription + 1);
                    MembersHeader->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
                    MembersHeader->MembersSize = sizeof(KSPROPERTY_STEPPING_LONG);
                    MembersHeader->MembersCount = NodeContext->ChannelCount;
                    MembersHeader->Flags = 0;

                    PKSPROPERTY_STEPPING_LONG Range = (PKSPROPERTY_STEPPING_LONG)(MembersHeader + 1);
                    for (ULONG Channel = 0; Channel < NodeContext->ChannelCount; Channel++)
                    {
                        Range[Channel].Bounds.SignedMinimum = DBMin * 0x10000;
                        Range[Channel].Bounds.SignedMaximum = DBMax * 0x10000;
                        Range[Channel].SteppingDelta = DBStep * 0x10000;
                        Range[Channel].Reserved = 0;
                    }

                    PropertyRequest->ValueSize = BasicPropertySize;
                    Status = STATUS_SUCCESS;
                }
            }
            else if (PropertyRequest->ValueSize >= sizeof(ULONG))
            {
                PULONG AccessFlags = (PULONG)PropertyRequest->Value;
                *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
                PropertyRequest->ValueSize = sizeof(ULONG);
                Status = STATUS_SUCCESS;
            }
            else
            {
                DPRINT1("Buffer too small\n");
                PropertyRequest->ValueSize = 0;
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get cached value
            Gain = Node->GetCachedOutputGain();
            DPRINT("Node %u, Gain %d\n", OutputNodes[NodeIndex], Gain);
            double DBVol = DBMin + ((double)Gain / (double)AmplifierCapabilities.NumSteps * DBRange);
            *Value = (LONG)(DBVol * 0x10000);
            DPRINT("Resulting value %d\n", *Value);
            PropertyRequest->ValueSize = sizeof(LONG);
            Status = STATUS_SUCCESS;
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            DPRINT("Incoming value %d\n", *Value);
            double DBVol = (double)*Value / 0x10000;
            double PercentVol = (DBVol - DBMin) / DBRange;
            LONG Step = (LONG)(PercentVol * AmplifierCapabilities.NumSteps + 0.5);
            Gain = (UCHAR)Step;

            AMPLIFIER_GAIN_MUTE_SET Set;
            Set.Output = 0x1;
            Set.Input = 0x0;
            Set.Left = 0x1;
            Set.Right = 0x1;
            Set.Index = 0;
            Set.Mute = 0x0;
            Set.Gain = Gain;
            Status = Node->SetAmplifierGainMute(OutputNodes[NodeIndex], &Set);
            DPRINT("SetAmplifierGainMute Status %x, Node %u, Gain %d\n", Status, OutputNodes[NodeIndex], Gain);
            if (NT_SUCCESS(Status))
            {
                // cache new value
                Node->SetCachedOutputGain(Gain);
            }
        }
    }
    ExFreePool(OutputNodes);
    Miniport->Release();
    return Status;
}

NTSTATUS
NTAPI
PropertyHandler_OutputMute(IN PPCPROPERTY_REQUEST PropertyRequest)
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

    AMPLIFIER_CAPABILITIES AmplifierCapabilities = Node->GetCachedAmplifierCapabilities();
    DPRINT("HDAUDIO: Amplifier capabilities NumSteps %u, StepSize %u, Offset %u\n",
           AmplifierCapabilities.NumSteps, AmplifierCapabilities.Steps, AmplifierCapabilities.Offset);

    if (!AmplifierCapabilities.MuteCapable)
    {
        DPRINT1("HDAUDIO: Mute bit is not supported by hardware\n");
        Miniport->Release();
        return STATUS_NOT_SUPPORTED;
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
        PNODE_CONTEXT NodeContext = Node->FindNodeId(OutputNodes[NodeIndex]);
        if (!NodeContext)
        {
            DPRINT1("HDAUDIO: No node context for node %u\n", OutputNodes[NodeIndex]);
            Status = STATUS_UNSUCCESSFUL;
            continue;
        }

        UCHAR Mute;
        PBOOL Value = (PBOOL)PropertyRequest->Value;
        if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
            {
                const ULONG BasicPropertySize = sizeof(KSPROPERTY_DESCRIPTION) +
                                                sizeof(KSPROPERTY_MEMBERSHEADER) +
                                                sizeof(KSPROPERTY_STEPPING_LONG) * NodeContext->ChannelCount;
                PKSPROPERTY_DESCRIPTION PropertyDescription = (PKSPROPERTY_DESCRIPTION)PropertyRequest->Value;
                PropertyDescription->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
                PropertyDescription->DescriptionSize = BasicPropertySize;
                PropertyDescription->PropTypeSet.Set = KSPROPTYPESETID_General;
                PropertyDescription->PropTypeSet.Id = VT_BOOL;
                PropertyDescription->PropTypeSet.Flags = 0;
                PropertyDescription->MembersListCount = 1;
                PropertyDescription->Reserved = 0;

                if (PropertyRequest->ValueSize >= BasicPropertySize)
                {
                    PKSPROPERTY_MEMBERSHEADER MembersHeader = (PKSPROPERTY_MEMBERSHEADER)(PropertyDescription + 1);
                    MembersHeader->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
                    MembersHeader->MembersSize = sizeof(KSPROPERTY_STEPPING_LONG);
                    MembersHeader->MembersCount = 1;
                    MembersHeader->Flags = 0;

                    PKSPROPERTY_STEPPING_LONG Range = (PKSPROPERTY_STEPPING_LONG)(MembersHeader + 1);
                    Range->Bounds.SignedMinimum = 0;
                    Range->Bounds.SignedMaximum = 1;
                    Range->SteppingDelta = 1;
                    Range->Reserved = 0;

                    PropertyRequest->ValueSize = BasicPropertySize;
                    Status = STATUS_SUCCESS;
                }
            }
            else if (PropertyRequest->ValueSize >= sizeof(ULONG))
            {
                PULONG AccessFlags = (PULONG)PropertyRequest->Value;
                *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
                PropertyRequest->ValueSize = sizeof(ULONG);
                Status = STATUS_SUCCESS;
            }
            else
            {
                DPRINT1("Buffer too small\n");
                PropertyRequest->ValueSize = 0;
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get cached value
            Mute = Node->GetCachedOutputMute();
            DPRINT("Node %u, Mute %x\n", OutputNodes[NodeIndex], Mute);
            *Value = Mute;
            PropertyRequest->ValueSize = sizeof(BOOL);
            Status = STATUS_SUCCESS;
        }
        else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            Mute = *Value;

            AMPLIFIER_GAIN_MUTE_SET Set;
            Set.Output = 0x1;
            Set.Input = 0x0;
            Set.Left = 0x1;
            Set.Right = 0x1;
            Set.Index = 0;
            Set.Mute = Mute;
            Set.Gain = Mute ? 0 : Node->GetCachedOutputGain();
            Status = Node->SetAmplifierGainMute(OutputNodes[NodeIndex], &Set);
            DPRINT("SetAmplifierGainMute Status %x, Node %u, Mute %x\n", Status, OutputNodes[NodeIndex], Mute);
            if (NT_SUCCESS(Status))
            {
                // cache new value
                Node->SetCachedOutputMute(Mute);
            }
        }
    }
    ExFreePool(OutputNodes);
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
