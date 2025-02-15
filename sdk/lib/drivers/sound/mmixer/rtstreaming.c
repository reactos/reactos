/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/rtstreaming.c
 * PURPOSE:         RTWave Handling Functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

// #define NDEBUG
#define YDEBUG
#include <debug.h>

const GUID KSPROPSETID_RTAudio = {0xa855a48c, 0x2f78, 0x4729, {0x90, 0x51, 0x19, 0x68, 0x74, 0x6b, 0x9e, 0xef}};

MIXER_STATUS
MMixerInitializeRTStreamingBuffer(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE PinHandle,
    IN ULONG RequestedBufferSize,
    IN ULONG NotificationCount,
    OUT PVOID* RTStreamingBuffer,
    OUT PULONG RTStreamingBufferLength)
{
    KSRTAUDIO_BUFFER_PROPERTY_WITH_NOTIFICATION Property;
    KSRTAUDIO_BUFFER OutData;
    MIXER_STATUS Status;
    ULONG Length;

    /* Validate mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
        return Status;

    Property.Property.Id = KSPROPERTY_RTAUDIO_BUFFER_WITH_NOTIFICATION;
    Property.Property.Set = KSPROPSETID_RTAudio;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.BaseAddress = NULL;
    Property.RequestedBufferSize = RequestedBufferSize;
    Property.NotificationCount = NotificationCount;

    Status = MixerContext->Control(
        PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(KSRTAUDIO_BUFFER_PROPERTY_WITH_NOTIFICATION), &OutData,
        sizeof(KSRTAUDIO_BUFFER), &Length);
    DPRINT1("Status %x\n", Status);
    if (Status == MM_STATUS_SUCCESS)
    {
        // return result
        *RTStreamingBuffer = OutData.BufferAddress;
        *RTStreamingBufferLength = OutData.ActualBufferSize;
    }
    DPRINT1("MMixerInitializeRTStreamingBuffer status %x\n", Status);
    return Status;
}

MIXER_STATUS
MMixerRegisterRTStreamingEvent(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE PinHandle,
    IN HANDLE StreamingEvent)
{
    KSRTAUDIO_NOTIFICATION_EVENT_PROPERTY Property;
    MIXER_STATUS Status;
    ULONG Length;

    /* Validate mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
        return Status;

    Property.Property.Id = KSPROPERTY_RTAUDIO_REGISTER_NOTIFICATION_EVENT;
    Property.Property.Set = KSPROPSETID_RTAudio;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.NotificationEvent = StreamingEvent;

    Status = MixerContext->Control(
        PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(KSRTAUDIO_NOTIFICATION_EVENT_PROPERTY), NULL, 0, &Length);
    return Status;
}

MIXER_STATUS
MMixerUnregisterRTStreamingEvent(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE PinHandle,
    IN HANDLE StreamingEvent)
{
    KSRTAUDIO_NOTIFICATION_EVENT_PROPERTY Property;
    MIXER_STATUS Status;
    ULONG Length;

    /* Validate mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
        return Status;

    Property.Property.Id = KSPROPERTY_RTAUDIO_UNREGISTER_NOTIFICATION_EVENT;
    Property.Property.Set = KSPROPSETID_RTAudio;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.NotificationEvent = StreamingEvent;

    Status = MixerContext->Control(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(Property), NULL, 0, &Length);
    return Status;
}
