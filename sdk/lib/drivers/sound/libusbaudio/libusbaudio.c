/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/libusbaudio/libusbaudio.c
 * PURPOSE:         USB AUDIO Parser
 * PROGRAMMER:      Johannes Anderwald
 */
#include "priv.h"

GUID NodeTypeMicrophone = {STATIC_KSNODETYPE_MICROPHONE};
GUID NodeTypeDesktopMicrophone = {STATIC_KSNODETYPE_DESKTOP_MICROPHONE};
GUID NodeTypePersonalMicrophone = {STATIC_KSNODETYPE_PERSONAL_MICROPHONE};
GUID NodeTypeOmmniMicrophone = {STATIC_KSNODETYPE_OMNI_DIRECTIONAL_MICROPHONE};
GUID NodeTypeArrayMicrophone = {STATIC_KSNODETYPE_MICROPHONE_ARRAY};
GUID NodeTypeProcessingArrayMicrophone = {STATIC_KSNODETYPE_PROCESSING_MICROPHONE_ARRAY};
GUID NodeTypeSpeaker = {STATIC_KSNODETYPE_SPEAKER};
GUID NodeTypeHeadphonesSpeaker = {STATIC_KSNODETYPE_HEADPHONES};
GUID NodeTypeHMDA = {STATIC_KSNODETYPE_HEAD_MOUNTED_DISPLAY_AUDIO};
GUID NodeTypeDesktopSpeaker = {STATIC_KSNODETYPE_DESKTOP_SPEAKER};
GUID NodeTypeRoomSpeaker = {STATIC_KSNODETYPE_ROOM_SPEAKER};
GUID NodeTypeCommunicationSpeaker = {STATIC_KSNODETYPE_COMMUNICATION_SPEAKER};
GUID NodeTypeSubwoofer = {STATIC_KSNODETYPE_LOW_FREQUENCY_EFFECTS_SPEAKER};
GUID NodeTypeCapture = {STATIC_PINNAME_CAPTURE};
GUID NodeTypePlayback = {STATIC_KSCATEGORY_AUDIO};


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

USBAUDIO_STATUS
UsbAudio_InitializeContext(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUSBAUDIO_ALLOC Alloc,
    IN PUSBAUDIO_FREE Free,
    IN PUSBAUDIO_COPY Copy)
{

    /* verify parameters */
    if (!Context || !Alloc || !Free || !Copy)
    {
        /* invalid parameter */
        return UA_STATUS_INVALID_PARAMETER;
    }

    /* initialize context */
    Context->Size = sizeof(USBAUDIO_CONTEXT);
    Context->Alloc = Alloc;
    Context->Free = Free;
    Context->Copy = Copy;

    /* done */
    return UA_STATUS_SUCCESS;
}

LPGUID
UsbAudio_GetPinCategoryFromTerminalDescriptor(
    IN PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR TerminalDescriptor)
{
    if (TerminalDescriptor->wTerminalType == USB_AUDIO_MICROPHONE_TERMINAL_TYPE)
        return &NodeTypeMicrophone;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_DESKTOP_MICROPHONE_TERMINAL_TYPE)
        return &NodeTypeDesktopMicrophone;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_PERSONAL_MICROPHONE_TERMINAL_TYPE)
        return &NodeTypePersonalMicrophone;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_OMMNI_MICROPHONE_TERMINAL_TYPE)
        return &NodeTypeOmmniMicrophone;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_ARRAY_MICROPHONE_TERMINAL_TYPE)
        return &NodeTypeArrayMicrophone;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_ARRAY_PROCESSING_MICROPHONE_TERMINAL_TYPE)
        return &NodeTypeProcessingArrayMicrophone;

    /* playback types */
    if (TerminalDescriptor->wTerminalType == USB_AUDIO_SPEAKER_TERMINAL_TYPE)
        return &NodeTypeSpeaker;
    else if (TerminalDescriptor->wTerminalType == USB_HEADPHONES_SPEAKER_TERMINAL_TYPE)
        return &NodeTypeHeadphonesSpeaker;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_HMDA_TERMINAL_TYPE)
        return &NodeTypeHMDA;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_DESKTOP_SPEAKER_TERMINAL_TYPE)
        return &NodeTypeDesktopSpeaker;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_ROOM_SPEAKER_TERMINAL_TYPE)
        return &NodeTypeRoomSpeaker;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_COMMUNICATION_SPEAKER_TERMINAL_TYPE)
        return &NodeTypeCommunicationSpeaker;
    else if (TerminalDescriptor->wTerminalType == USB_AUDIO_SUBWOOFER_TERMINAL_TYPE)
        return &NodeTypeSubwoofer;

    if (TerminalDescriptor->wTerminalType == USB_AUDIO_STREAMING_TERMINAL_TYPE)
    {
        if (TerminalDescriptor->bDescriptorSubtype == USB_AUDIO_OUTPUT_TERMINAL)
            return &NodeTypeCapture;
        else if (TerminalDescriptor->bDescriptorSubtype == USB_AUDIO_INPUT_TERMINAL)
            return &NodeTypePlayback;

    }
    return NULL;
}

USBAUDIO_STATUS
UsbAudio_InitPinDescriptor(
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN PKSPIN_DESCRIPTOR_EX PinDescriptor, 
    IN ULONG TerminalCount,
    IN PUSB_COMMON_DESCRIPTOR * Descriptors,
    IN ULONG TerminalId)
{
    PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR TerminalDescriptor;

    TerminalDescriptor = UsbAudio_GetTerminalDescriptorById(Descriptors, TerminalCount, TerminalId);
    if (!TerminalDescriptor)
    {
        /* failed to find terminal descriptor */
        return UA_STATUS_UNSUCCESSFUL;
    }

    /* init pin descriptor */
    PinDescriptor->PinDescriptor.InterfacesCount = 1;
    PinDescriptor->PinDescriptor.Interfaces = &StandardPinInterface;
    PinDescriptor->PinDescriptor.MediumsCount = 1;
    PinDescriptor->PinDescriptor.Mediums = &StandardPinMedium;
    PinDescriptor->PinDescriptor.Category = UsbAudio_GetPinCategoryFromTerminalDescriptor(TerminalDescriptor);

    if (TerminalDescriptor->wTerminalType == USB_AUDIO_STREAMING_TERMINAL_TYPE)
    {
        if (TerminalDescriptor->bDescriptorSubtype == USB_AUDIO_OUTPUT_TERMINAL)
        {
            PinDescriptor->PinDescriptor.Communication = KSPIN_COMMUNICATION_SINK;
            PinDescriptor->PinDescriptor.DataFlow = KSPIN_DATAFLOW_OUT;
        }
        else if (TerminalDescriptor->bDescriptorSubtype == USB_AUDIO_INPUT_TERMINAL)
        {
            PinDescriptor->PinDescriptor.Communication = KSPIN_COMMUNICATION_SINK;
            PinDescriptor->PinDescriptor.DataFlow = KSPIN_DATAFLOW_IN;
        }

        /* irp sinks / sources can be instantiated */
        PinDescriptor->InstancesPossible = 1;

    }
    else if (TerminalDescriptor->bDescriptorSubtype == USB_AUDIO_INPUT_TERMINAL)
    {
        PinDescriptor->PinDescriptor.Communication = KSPIN_COMMUNICATION_BRIDGE;
        PinDescriptor->PinDescriptor.DataFlow = KSPIN_DATAFLOW_IN;
    }
    else if (TerminalDescriptor->bDescriptorSubtype == USB_AUDIO_OUTPUT_TERMINAL)
    {
        PinDescriptor->PinDescriptor.Communication = KSPIN_COMMUNICATION_BRIDGE;
        PinDescriptor->PinDescriptor.DataFlow = KSPIN_DATAFLOW_OUT;
    }

    return UA_STATUS_SUCCESS;
}

USBAUDIO_STATUS
UsbAudio_ParseConfigurationDescriptor(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorSize)
{
    USBAUDIO_STATUS Status;
    PKSFILTER_DESCRIPTOR FilterDescriptor;
    PUSB_COMMON_DESCRIPTOR * Descriptors;
    PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors;
    PULONG TerminalIds;
    ULONG Index, AudioControlInterfaceIndex;
    ULONG InterfaceCount, DescriptorCount, NewDescriptorCount;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;


    if (!Context || !ConfigurationDescriptor || !ConfigurationDescriptorSize)
    {
        /* invalid parameter */
        return UA_STATUS_INVALID_PARAMETER;
    }

    /* count usb interface descriptors */
    Status = UsbAudio_CountInterfaceDescriptors(ConfigurationDescriptor, ConfigurationDescriptorSize, &InterfaceCount);
    if (Status != UA_STATUS_SUCCESS || InterfaceCount == 0)
    {
        /* invalid parameter */
        return UA_STATUS_INVALID_PARAMETER;
    }

    /* construct interface array */
    Status = UsbAudio_CreateInterfaceDescriptorsArray(Context, ConfigurationDescriptor, ConfigurationDescriptorSize, InterfaceCount, &InterfaceDescriptors);
    if (Status != UA_STATUS_SUCCESS)
    {
        /* invalid parameter */
        return UA_STATUS_INVALID_PARAMETER;
    }

    /* get audio control interface index */
    AudioControlInterfaceIndex = UsbAudio_GetAudioControlInterfaceIndex(InterfaceDescriptors, InterfaceCount);
    if (AudioControlInterfaceIndex == MAXULONG)
    {
        /* invalid configuration descriptor */
        Context->Free(InterfaceDescriptors);
        return UA_STATUS_INVALID_PARAMETER;
    }

    /* count audio terminal descriptors */
    Status = UsbAudio_CountAudioDescriptors(ConfigurationDescriptor, ConfigurationDescriptorSize, InterfaceDescriptors, InterfaceCount, AudioControlInterfaceIndex, &DescriptorCount);
    if (Status != UA_STATUS_SUCCESS || DescriptorCount == 0)
    {
        /* invalid parameter */
        Context->Free(InterfaceDescriptors);
        return UA_STATUS_INVALID_PARAMETER;
    }

    /* construct terminal descriptor array */
    Status = UsbAudio_CreateAudioDescriptorArray(Context, ConfigurationDescriptor, ConfigurationDescriptorSize, InterfaceDescriptors, InterfaceCount, AudioControlInterfaceIndex, DescriptorCount, &Descriptors);
    if (Status != UA_STATUS_SUCCESS)
    {
        /* no memory */
        Context->Free(InterfaceDescriptors);
        //DPRINT("[LIBUSBAUDIO] Failed to create descriptor array with %x\n", Status);
        return Status;
    }

    /* construct filter */
    FilterDescriptor = (PKSFILTER_DESCRIPTOR)Context->Alloc(sizeof(KSFILTER_DESCRIPTOR));
    if (!FilterDescriptor)
    {
        /* no memory */
        Context->Free(InterfaceDescriptors);
        Context->Free(Descriptors);
        return UA_STATUS_NO_MEMORY;
    }


    /* construct pin id array */
    TerminalIds = (PULONG)Context->Alloc(sizeof(ULONG) * DescriptorCount);
    if (!TerminalIds)
    {
        /* no memory */
        Context->Free(InterfaceDescriptors);
        Context->Free(FilterDescriptor);
        Context->Free(Descriptors);
        return UA_STATUS_NO_MEMORY;
    }

    /* now assign terminal ids */
    Status = UsbAudio_AssignTerminalIds(Context, DescriptorCount, Descriptors, TerminalIds, &NewDescriptorCount);
    if(Status != UA_STATUS_SUCCESS || NewDescriptorCount == 0)
    {
        /* failed to initialize */
        Context->Free(InterfaceDescriptors);
        Context->Free(FilterDescriptor);
        Context->Free(Descriptors);
        Context->Free(TerminalIds);
        DPRINT1("[LIBUSBAUDIO] Failed to assign terminal ids with %x DescriptorCount %lx\n", Status, DescriptorCount);
        return UA_STATUS_UNSUCCESSFUL;
    }


    /* init filter */
    FilterDescriptor->Version = KSFILTER_DESCRIPTOR_VERSION;
    FilterDescriptor->Flags = 0; /* FIXME */
    FilterDescriptor->PinDescriptorsCount = NewDescriptorCount;
    FilterDescriptor->PinDescriptorSize = sizeof(KSPIN_DESCRIPTOR_EX);
    FilterDescriptor->PinDescriptors = Context->Alloc(sizeof(KSPIN_DESCRIPTOR_EX) * FilterDescriptor->PinDescriptorsCount);
    if (!FilterDescriptor->PinDescriptors)
    {
        /* no memory */
        Context->Free(InterfaceDescriptors);
        Context->Free(FilterDescriptor);
        Context->Free(Descriptors);
        Context->Free(TerminalIds);
        return UA_STATUS_NO_MEMORY;
    }

    /* now init pin properties */
    for(Index = 0; Index < FilterDescriptor->PinDescriptorsCount; Index++)
    {
        /* now init every pin descriptor */
        Status = UsbAudio_InitPinDescriptor(ConfigurationDescriptor, ConfigurationDescriptorSize, (PKSPIN_DESCRIPTOR_EX)&FilterDescriptor->PinDescriptors[Index], DescriptorCount, Descriptors, TerminalIds[Index]);
        if (Status != UA_STATUS_SUCCESS)
            break;
    }

    if (Status != UA_STATUS_SUCCESS)
    {
        /* failed to init pin descriptor */
        Context->Free(InterfaceDescriptors);
        Context->Free((PVOID)FilterDescriptor->PinDescriptors);
        Context->Free(FilterDescriptor);
        Context->Free(Descriptors);
        Context->Free(TerminalIds);
        DPRINT1("[LIBUSBAUDIO] Failed to init pin with %x\n", Status);
        return Status;
    }


    /* now assign data ranges to the pins */
    for(Index = 0; Index < InterfaceCount; Index++)
    {
        /* get descriptor */
        InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)InterfaceDescriptors[Index];

        /* sanity check */
        ASSERT(InterfaceDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE);
        DPRINT1("InterfaceNumber %d bInterfaceClass %x bInterfaceSubClass %x\n", InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bInterfaceClass, InterfaceDescriptor->bInterfaceSubClass);

        if (InterfaceDescriptor->bInterfaceClass != 0x01 && InterfaceDescriptor->bInterfaceSubClass != 0x02)
            continue;

        /* assign data ranges */
        Status = UsbAudio_AssignDataRanges(Context, ConfigurationDescriptor, ConfigurationDescriptorSize, FilterDescriptor, InterfaceDescriptors, InterfaceCount, Index, TerminalIds);
        if (Status != UA_STATUS_SUCCESS)
            break;
    }

    if (Status != UA_STATUS_SUCCESS)
    {
        /* failed to init pin descriptor */
        Context->Free(InterfaceDescriptors);
        Context->Free((PVOID)FilterDescriptor->PinDescriptors);
        Context->Free(FilterDescriptor);
        Context->Free(Descriptors);
        Context->Free(TerminalIds);
        return Status;
    }



    if (Status != UA_STATUS_SUCCESS)
    {
        /* failed to init pin descriptor */
        Context->Free(InterfaceDescriptors);
        Context->Free((PVOID)FilterDescriptor->PinDescriptors);
        Context->Free(FilterDescriptor);
        Context->Free(Descriptors);
        Context->Free(TerminalIds);
        return Status;
    }

    Context->Context = FilterDescriptor;
    return Status;

}

USBAUDIO_STATUS
UsbAudio_GetFilter(
    IN PUSBAUDIO_CONTEXT Context,
    OUT PVOID * OutFilterDescriptor)
{
    if (!OutFilterDescriptor)
        return UA_STATUS_INVALID_PARAMETER;

    *OutFilterDescriptor = Context->Context;
    return UA_STATUS_SUCCESS;
}