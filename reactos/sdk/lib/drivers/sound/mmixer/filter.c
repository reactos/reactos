/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/filter.c
 * PURPOSE:         Mixer Filter Functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define YDEBUG
#include <debug.h>

ULONG
MMixerGetFilterPinCount(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer)
{
    KSPROPERTY Pin;
    MIXER_STATUS Status;
    ULONG NumPins, BytesReturned;

    /* setup property request */
    Pin.Flags = KSPROPERTY_TYPE_GET;
    Pin.Set = KSPROPSETID_Pin;
    Pin.Id = KSPROPERTY_PIN_CTYPES;

    /* query pin count */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&NumPins, sizeof(ULONG), (PULONG)&BytesReturned);

    /* check for success */
    if (Status != MM_STATUS_SUCCESS)
        return 0;

    return NumPins;
}

MIXER_STATUS
MMixerGetFilterTopologyProperty(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG PropertyId,
    OUT PKSMULTIPLE_ITEM * OutMultipleItem)
{
    KSPROPERTY Property;
    PKSMULTIPLE_ITEM MultipleItem;
    MIXER_STATUS Status;
    ULONG BytesReturned;

    /* setup property request */
    Property.Id = PropertyId;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Set = KSPROPSETID_Topology;

    /* query for the size */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &BytesReturned);

    if (Status != MM_STATUS_MORE_ENTRIES)
        return Status;

    /* sanity check */
    ASSERT(BytesReturned);

    /* allocate an result buffer */
    MultipleItem = (PKSMULTIPLE_ITEM)MixerContext->Alloc(BytesReturned);

    if (!MultipleItem)
    {
        /* not enough memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* query again with allocated buffer */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)MultipleItem, BytesReturned, &BytesReturned);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed */
        MixerContext->Free((PVOID)MultipleItem);
        return Status;
    }

    /* store result */
    *OutMultipleItem = MultipleItem;

    /* done */
    return Status;
}

MIXER_STATUS
MMixerGetPhysicalConnection(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG PinId,
    OUT PKSPIN_PHYSICALCONNECTION *OutConnection)
{
    KSP_PIN Pin;
    MIXER_STATUS Status;
    ULONG BytesReturned;
    PKSPIN_PHYSICALCONNECTION Connection;

    /* setup the request */
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Id = KSPROPERTY_PIN_PHYSICALCONNECTION;
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.PinId = PinId;

    /* query the pin for the physical connection */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (Status == MM_STATUS_UNSUCCESSFUL)
    {
        /* pin does not have a physical connection */
        return Status;
    }
    DPRINT("Status %u BytesReturned %lu\n", Status, BytesReturned);
    Connection = (PKSPIN_PHYSICALCONNECTION)MixerContext->Alloc(BytesReturned);
    if (!Connection)
    {
        /* not enough memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* query the pin for the physical connection */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)Connection, BytesReturned, &BytesReturned);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to query the physical connection */
        MixerContext->Free(Connection);
        return Status;
    }

    // store connection
    *OutConnection = Connection;
    return Status;
}

ULONG
MMixerGetControlTypeFromTopologyNode(
    IN LPGUID NodeType)
{
    if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_AGC))
    {
        /* automatic gain control */
        return MIXERCONTROL_CONTROLTYPE_ONOFF;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_LOUDNESS))
    {
        /* loudness control */
        return MIXERCONTROL_CONTROLTYPE_LOUDNESS;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUTE))
    {
        /* mute control */
        return MIXERCONTROL_CONTROLTYPE_MUTE;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_TONE))
    {
        /* tone control
         * FIXME
         * MIXERCONTROL_CONTROLTYPE_ONOFF if KSPROPERTY_AUDIO_BASS_BOOST is supported
         * MIXERCONTROL_CONTROLTYPE_BASS if KSPROPERTY_AUDIO_BASS is supported
         * MIXERCONTROL_CONTROLTYPE_TREBLE if KSPROPERTY_AUDIO_TREBLE is supported
         */
        UNIMPLEMENTED;
        return MIXERCONTROL_CONTROLTYPE_ONOFF;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_VOLUME))
    {
        /* volume control */
        return MIXERCONTROL_CONTROLTYPE_VOLUME;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_PEAKMETER))
    {
        /* peakmeter control */
        return MIXERCONTROL_CONTROLTYPE_PEAKMETER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUX))
    {
        /* mux control */
        return MIXERCONTROL_CONTROLTYPE_MUX;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_MUX))
    {
        /* mux control */
        return MIXERCONTROL_CONTROLTYPE_MUX;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_STEREO_WIDE))
    {
        /* stero wide control */
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_CHORUS))
    {
        /* chorus control */
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_REVERB))
    {
        /* reverb control */
        return MIXERCONTROL_CONTROLTYPE_FADER;
    }
    else if (IsEqualGUIDAligned(NodeType, (LPGUID)&KSNODETYPE_SUPERMIX))
    {
        /* supermix control
         * MIXERCONTROL_CONTROLTYPE_MUTE if KSPROPERTY_AUDIO_MUTE is supported 
         */
        UNIMPLEMENTED;
        return MIXERCONTROL_CONTROLTYPE_VOLUME;
    }
    /* TODO
     * check for other supported node types
     */
    //UNIMPLEMENTED;
    return 0;
}

MIXER_STATUS
MMixerSetGetControlDetails(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hMixer,
    IN ULONG NodeId,
    IN ULONG bSet,
    IN ULONG PropertyId,
    IN ULONG Channel,
    IN PLONG InputValue)
{
    KSNODEPROPERTY_AUDIO_CHANNEL Property;
    MIXER_STATUS Status;
    LONG Value;
    ULONG BytesReturned;

    if (bSet)
        Value = *InputValue;

    /* setup the request */
    RtlZeroMemory(&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL));

    Property.NodeProperty.NodeId = NodeId;
    Property.NodeProperty.Property.Id = PropertyId;
    Property.NodeProperty.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY;
    Property.NodeProperty.Property.Set = KSPROPSETID_Audio;
    Property.Channel = Channel;
    Property.Reserved = 0;

    if (bSet)
        Property.NodeProperty.Property.Flags |= KSPROPERTY_TYPE_SET;
    else
        Property.NodeProperty.Property.Flags |= KSPROPERTY_TYPE_GET;

    /* send the request */
    Status = MixerContext->Control(hMixer, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSNODEPROPERTY_AUDIO_CHANNEL), (PVOID)&Value, sizeof(LONG), &BytesReturned);

    if (!bSet && Status == MM_STATUS_SUCCESS)
    {
        *InputValue = Value;
    }

    DPRINT("Status %x bSet %u NodeId %u Value %d PropertyId %u\n", Status, bSet, NodeId, Value, PropertyId);
    return Status;
}

ULONG
MMixerGetPinInstanceCount(
    PMIXER_CONTEXT MixerContext,
    HANDLE hFilter,
    ULONG PinId)
{
    KSP_PIN PinRequest;
    KSPIN_CINSTANCES PinInstances;
    ULONG BytesReturned;
    MIXER_STATUS Status;

    /* query the instance count */
    PinRequest.Reserved = 0;
    PinRequest.PinId = PinId;
    PinRequest.Property.Set = KSPROPSETID_Pin;
    PinRequest.Property.Flags = KSPROPERTY_TYPE_GET;
    PinRequest.Property.Id = KSPROPERTY_PIN_CINSTANCES;

    Status = MixerContext->Control(hFilter, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&PinInstances, sizeof(KSPIN_CINSTANCES), &BytesReturned);
    ASSERT(Status == MM_STATUS_SUCCESS);
    return PinInstances.CurrentCount;
}
