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
    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = sizeof(KSMULTIPLE_ITEM) + sizeof(KSJACK_DESCRIPTION);
        return STATUS_BUFFER_OVERFLOW;
    }

    PUNKNOWN UnknownMiniport = (PUNKNOWN)PropertyRequest->MajorTarget;
    if (!UnknownMiniport)
        return STATUS_INVALID_PARAMETER;

    CMiniportTopology *Miniport;
    NTSTATUS Status = UnknownMiniport->QueryInterface(IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status) || !Miniport)
        return Status;

    CFunctionGroupNode *Node = (CFunctionGroupNode*)Miniport->GetNode();
    if (!Node)
        return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        PIN_CONFIGURATION_DEFAULT PinConfiguration;
        Status = Node->GetPinConfigurationDefault(Node->GetStartNodeId(), &PinConfiguration);
        if (!NT_SUCCESS(Status))
            return Status;

        PKSMULTIPLE_ITEM MultipleItem = (PKSMULTIPLE_ITEM)PropertyRequest->Value;
        MultipleItem->Size = sizeof(KSMULTIPLE_ITEM) + sizeof(KSJACK_DESCRIPTION);
        MultipleItem->Count = 1;
        PKSJACK_DESCRIPTION JackDescription = (PKSJACK_DESCRIPTION)(MultipleItem + 1);
        JackDescription->ChannelMapping = KSAUDIO_SPEAKER_STEREO; // FIXME
        JackDescription->Color = PinConfiguration.Color;
        JackDescription->ConnectionType = (EPcxConnectionType)PinConfiguration.ConnectionType;
        JackDescription->GeoLocation = (EPcxGeoLocation)PinConfiguration.Location;
        JackDescription->GenLocation = (EPcxGenLocation)(PinConfiguration.Location << 4);
        JackDescription->PortConnection = (EPxcPortConnection)PinConfiguration.PortConnectivity;
        JackDescription->IsConnected = TRUE;
        return STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        PULONG AccessFlags = (PULONG)PropertyRequest->Value;
        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET;
        PropertyRequest->ValueSize = sizeof(KSMULTIPLE_ITEM) + sizeof(KSJACK_DESCRIPTION);
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PropertyHandler_ChannelConfig(IN PPCPROPERTY_REQUEST PropertyRequest)
{
    if (PropertyRequest->Node == (ULONG)-1)
        return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->ValueSize < sizeof(LONG))
        return STATUS_BUFFER_TOO_SMALL;

    PUNKNOWN UnknownMiniport = (PUNKNOWN)PropertyRequest->MajorTarget;
    if (!UnknownMiniport)
        return STATUS_INVALID_PARAMETER;

    CMiniportWaveRT *Miniport;
    NTSTATUS Status = UnknownMiniport->QueryInterface(IID_IMiniportWaveRT, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status) || !Miniport)
        return Status;

    CFunctionGroupNode *Node = (CFunctionGroupNode*)Miniport->GetNode();
    if (!Node)
        return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        *(PLONG)PropertyRequest->Value = KSAUDIO_SPEAKER_STEREO;
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
PropertyHandler_SpeakerGeometry(IN PPCPROPERTY_REQUEST PropertyRequest)
{
    if (PropertyRequest->Node == (ULONG)-1)
        return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->ValueSize < sizeof(LONG))
        return STATUS_BUFFER_TOO_SMALL;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        *(PLONG)PropertyRequest->Value = -1;
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

    CMiniportTopology *Miniport;
    NTSTATUS Status = UnknownMiniport->QueryInterface(IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status) || !Miniport)
        return Status;

    CFunctionGroupNode *Node = (CFunctionGroupNode*)Miniport->GetNode();
    if (!Node)
        return STATUS_INVALID_PARAMETER;

    AMPLIFIER_CAPABILITIES AmplifierDetails;
    Status = Node->GetAmplifierDetails(Node->GetStartNodeId(), FALSE /* FIXME */, &AmplifierDetails);
    if (!NT_SUCCESS(Status))
        return Status;

    PLONG Value = (PLONG)PropertyRequest->Value;
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        LONG Volume;
        UCHAR Direct;
        Status = Node->GetVolume(Node->GetStartNodeId(), &Direct, &Volume);
        *Value = AmplifierDetails.Offset - AmplifierDetails.Steps * Volume;
        return Status;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        LONG Volume = *Value;
        if (Volume > AmplifierDetails.Offset)
            Volume = AmplifierDetails.Offset;
        if (Volume < AmplifierDetails.Offset - AmplifierDetails.Steps * AmplifierDetails.NumSteps)
            Volume = AmplifierDetails.Offset - AmplifierDetails.Steps * AmplifierDetails.NumSteps;
        Status = Node->SetVolume(Node->GetStartNodeId(), 1 /* FIXME */, Volume);
        return Status;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        PULONG AccessFlags = (PULONG)PropertyRequest->Value;
        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
        PropertyRequest->ValueSize = sizeof(LONG);
        return STATUS_SUCCESS;
    }
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

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        *(PBOOL)PropertyRequest->Value = FALSE;
        return STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        UNIMPLEMENTED;
        return STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        PULONG AccessFlags = (PULONG)PropertyRequest->Value;
        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
        PropertyRequest->ValueSize = sizeof(BOOL);
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
EventHandler_Volume(IN PPCEVENT_REQUEST EventRequest)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
}
