/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS Kernel Streaming
* FILE:            lib/drivers/sound/libusbaudio/libusbaudio.c
* PURPOSE:         USB AUDIO Parser
* PROGRAMMER:      Johannes Anderwald
*/

#include "priv.h"

USBAUDIO_STATUS
UsbAudio_CountDescriptors(
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN UCHAR DescriptorType,
    IN PUSB_COMMON_DESCRIPTOR StartPosition,
    IN PUSB_COMMON_DESCRIPTOR EndPosition,
    OUT PULONG DescriptorCount)
{
    PUSB_COMMON_DESCRIPTOR Descriptor;
    ULONG Count = 0;

    /* init result */
    *DescriptorCount = 0;

    /* enumerate descriptors */
    Descriptor = StartPosition;
    if (Descriptor == NULL)
        Descriptor = (PUSB_COMMON_DESCRIPTOR)ConfigurationDescriptor;

    if (EndPosition == NULL)
        EndPosition = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)ConfigurationDescriptor + ConfigurationDescriptorLength);


    while((ULONG_PTR)Descriptor < ((ULONG_PTR)EndPosition))
    {
        if (!Descriptor->bLength || !Descriptor->bDescriptorType)
        {
            /* bogus descriptor */
            return UA_STATUS_UNSUCCESSFUL;
        }

        if (Descriptor->bDescriptorType == DescriptorType)
        {
            /* found descriptor */
            Count++;
        }

        /* move to next descriptor */
        Descriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)Descriptor + Descriptor->bLength);

    }

    /* store result */
    *DescriptorCount = Count;
    return UA_STATUS_SUCCESS;
}

USBAUDIO_STATUS
UsbAudio_GetDescriptors(
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN UCHAR DescriptorType,
    IN ULONG DescriptorCount,
    IN PUSB_COMMON_DESCRIPTOR StartPosition,
    IN PUSB_COMMON_DESCRIPTOR EndPosition,
    OUT PUSB_COMMON_DESCRIPTOR *Descriptors)
{
    PUSB_COMMON_DESCRIPTOR Descriptor;
    ULONG Count = 0;

    /* enumerate descriptors */
    Descriptor = StartPosition;
    if (Descriptor == NULL)
        Descriptor = (PUSB_COMMON_DESCRIPTOR)ConfigurationDescriptor;

    if (EndPosition == NULL)
        EndPosition = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)ConfigurationDescriptor + ConfigurationDescriptorLength);

    while((ULONG_PTR)Descriptor < ((ULONG_PTR)EndPosition))
    {
        if (!Descriptor->bLength || !Descriptor->bDescriptorType)
        {
            /* bogus descriptor */
            return UA_STATUS_UNSUCCESSFUL;
        }

        if (Descriptor->bDescriptorType == DescriptorType)
        {
            /* found descriptor */
            if (Count >= DescriptorCount)
                break;

            /* store result */
            Descriptors[Count] = Descriptor;
            Count++;
        }

        /* move to next descriptor */
        Descriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)Descriptor + Descriptor->bLength);
    }

    /* done */
    return UA_STATUS_SUCCESS;
}

USBAUDIO_STATUS
UsbAudio_CountInterfaceDescriptors(
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    OUT PULONG DescriptorCount)
{
    return UsbAudio_CountDescriptors(ConfigurationDescriptor, ConfigurationDescriptorLength, USB_INTERFACE_DESCRIPTOR_TYPE, NULL, NULL, DescriptorCount);
}

USBAUDIO_STATUS
UsbAudio_CreateDescriptorArray(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN ULONG ArrayLength,
    IN ULONG DescriptorType,
    IN PUSB_COMMON_DESCRIPTOR StartPosition,
    IN PUSB_COMMON_DESCRIPTOR EndPosition,
    OUT PUSB_COMMON_DESCRIPTOR ** Array)
{
    USBAUDIO_STATUS Status;
    PUSB_COMMON_DESCRIPTOR * Descriptors;

    /* zero result */
    *Array = NULL;

    /* first allocate descriptor array */
    Descriptors = (PUSB_COMMON_DESCRIPTOR*)Context->Alloc(sizeof(PUSB_COMMON_DESCRIPTOR) * ArrayLength);
    if (!Descriptors)
    {
        /* no memory */
        return UA_STATUS_NO_MEMORY;
    }

    /* extract control terminal descriptors */
    Status = UsbAudio_GetDescriptors(ConfigurationDescriptor, ConfigurationDescriptorLength, DescriptorType, ArrayLength, StartPosition, EndPosition, Descriptors);
    if (Status != UA_STATUS_SUCCESS)
    {
        /* failed */
        Context->Free(Descriptors);
        return Status;
    }

    /* store result */
    *Array = Descriptors;
    return UA_STATUS_SUCCESS;
}

USBAUDIO_STATUS
UsbAudio_CountAudioDescriptors(
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors,
    IN ULONG InterfaceDescriptorCount,
    IN ULONG InterfaceDescriptorIndex,
    OUT PULONG DescriptorCount)
{
    if (InterfaceDescriptorIndex + 1 == InterfaceDescriptorCount)
        return UsbAudio_CountDescriptors(ConfigurationDescriptor, ConfigurationDescriptorLength, USB_AUDIO_CONTROL_TERMINAL_DESCRIPTOR_TYPE, InterfaceDescriptors[InterfaceDescriptorIndex], NULL, DescriptorCount);
    else
        return UsbAudio_CountDescriptors(ConfigurationDescriptor, ConfigurationDescriptorLength, USB_AUDIO_CONTROL_TERMINAL_DESCRIPTOR_TYPE, InterfaceDescriptors[InterfaceDescriptorIndex], InterfaceDescriptors[InterfaceDescriptorIndex + 1], DescriptorCount);
}

USBAUDIO_STATUS
UsbAudio_CreateInterfaceDescriptorsArray(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN ULONG ArrayLength, 
    OUT PUSB_COMMON_DESCRIPTOR ** Array)
{
    return UsbAudio_CreateDescriptorArray(Context, ConfigurationDescriptor, ConfigurationDescriptorLength, ArrayLength, USB_INTERFACE_DESCRIPTOR_TYPE, NULL, NULL, Array);
}

USBAUDIO_STATUS
UsbAudio_CreateAudioDescriptorArray(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors,
    IN ULONG InterfaceDescriptorCount,
    IN ULONG InterfaceDescriptorIndex,
    IN ULONG ArrayLength,
    OUT PUSB_COMMON_DESCRIPTOR ** Array)
{
    if (InterfaceDescriptorIndex + 1 == InterfaceDescriptorCount)
        return UsbAudio_CreateDescriptorArray(Context, ConfigurationDescriptor, ConfigurationDescriptorLength, ArrayLength, USB_AUDIO_CONTROL_TERMINAL_DESCRIPTOR_TYPE, InterfaceDescriptors[InterfaceDescriptorIndex], NULL, Array);
    else
        return UsbAudio_CreateDescriptorArray(Context, ConfigurationDescriptor, ConfigurationDescriptorLength, ArrayLength, USB_AUDIO_CONTROL_TERMINAL_DESCRIPTOR_TYPE, 
                                              InterfaceDescriptors[InterfaceDescriptorIndex], InterfaceDescriptors[InterfaceDescriptorIndex + 1], Array);
}

PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR
UsbAudio_GetTerminalDescriptorById(
    IN PUSB_COMMON_DESCRIPTOR *Descriptors,
    IN ULONG DescriptorCount,
    IN ULONG TerminalId)
{
    ULONG Index;
    PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR Descriptor;

    for(Index = 0; Index < DescriptorCount; Index++)
    {
        /* get descriptor */
        Descriptor = (PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR)Descriptors[Index];

        /* is it an input / output terminal */
        DPRINT("Descriptor %p Type %x SubType %x TerminalID %x\n", Descriptor, Descriptor->bDescriptorType, Descriptor->bDescriptorSubtype, Descriptor->bTerminalID);
        if (Descriptor->bDescriptorSubtype != USB_AUDIO_INPUT_TERMINAL && Descriptor->bDescriptorSubtype != USB_AUDIO_OUTPUT_TERMINAL)
            continue;

        if (Descriptor->bTerminalID == TerminalId)
            return Descriptor;
    }
    return NULL;
}

ULONG
UsbAudio_GetAudioControlInterfaceIndex(
    IN PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors,
    IN ULONG DescriptorCount)
{
    ULONG Index;
    PUSB_INTERFACE_DESCRIPTOR Descriptor;


    for(Index = 0; Index < DescriptorCount; Index++)
    {
        /* get descriptor */
        Descriptor = (PUSB_INTERFACE_DESCRIPTOR)InterfaceDescriptors[Index];
        ASSERT(Descriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE);
        ASSERT(Descriptor->bLength == sizeof(USB_INTERFACE_DESCRIPTOR));

        /* compare interface class */
        if (Descriptor->bInterfaceClass == 0x01 && Descriptor->bInterfaceSubClass == 0x01)
        {
            /* found audio control class */
            return Index;
        }
    }

    /* not found */
    return MAXULONG;
}


USBAUDIO_STATUS
UsbAudio_FindTerminalDescriptorAtIndexWithSubtypeAndTerminalType(
    IN ULONG DescriptorCount,
    IN PUSB_COMMON_DESCRIPTOR *Descriptors,
    IN ULONG DescriptorIndex,
    IN UCHAR Subtype,
    IN USHORT TerminalType,
    OUT PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR * OutDescriptor,
    OUT PULONG OutDescriptorIndex)
{
    ULONG Index;
    ULONG Count = 0;
    PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR CurrentDescriptor;

    for(Index = 0; Index < DescriptorCount; Index++)
    {
        /* get current descriptor */
        CurrentDescriptor = (PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR)Descriptors[Index];
        ASSERT(CurrentDescriptor->bDescriptorType == USB_AUDIO_CONTROL_TERMINAL_DESCRIPTOR_TYPE);

        if (CurrentDescriptor->bDescriptorSubtype == Subtype && 
            (TerminalType == USB_AUDIO_UNDEFINED_TERMINAL_TYPE || CurrentDescriptor->wTerminalType == TerminalType))
        {
            /* found descriptor */
            if (Count == DescriptorIndex)
            {
                /* store result */
                *OutDescriptor = CurrentDescriptor;
                *OutDescriptorIndex = Index;
                return UA_STATUS_SUCCESS;
            }

            Count++;
        }
    }

    /* not found */
    return UA_STATUS_UNSUCCESSFUL;
}

USBAUDIO_STATUS
UsbAudio_AssignTerminalIds(
    IN PUSBAUDIO_CONTEXT Context,
    IN ULONG TerminalIdsLength,
    IN PUSB_COMMON_DESCRIPTOR * TerminalIds,
    OUT PULONG PinArray,
    OUT PULONG PinArrayCount)
{
    ULONG Consumed = 0;
    ULONG PinIndex = 0;
    ULONG Index, DescriptorIndex;
    PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR Descriptor;
    USBAUDIO_STATUS Status;

    /* FIXME: support more than 32 terminals */
    ASSERT(TerminalIdsLength <= 32);

    /* first search for an output terminal with streaming type */
    Status = UsbAudio_FindTerminalDescriptorAtIndexWithSubtypeAndTerminalType(TerminalIdsLength, TerminalIds, 0, USB_AUDIO_OUTPUT_TERMINAL, USB_AUDIO_STREAMING_TERMINAL_TYPE, &Descriptor, &DescriptorIndex);
    if (Status == UA_STATUS_SUCCESS)
    {
        /* found output terminal */
        PinArray[PinIndex] = Descriptor->bTerminalID;
        Consumed |= 1 << DescriptorIndex;
        DPRINT("Assigned TerminalId %x to PinIndex %lx Consumed %lx DescriptorIndex %lx\n", Descriptor->bTerminalID, PinIndex, Consumed, DescriptorIndex);
        PinIndex++;
    }

    /* now search for an input terminal with streaming type */
    Status = UsbAudio_FindTerminalDescriptorAtIndexWithSubtypeAndTerminalType(TerminalIdsLength, TerminalIds, 0, USB_AUDIO_INPUT_TERMINAL, USB_AUDIO_STREAMING_TERMINAL_TYPE, &Descriptor, &DescriptorIndex);
    if (Status == UA_STATUS_SUCCESS)
    {
        /* found output terminal */
        PinArray[PinIndex] = Descriptor->bTerminalID;
        Consumed |= 1 << DescriptorIndex;
        PinIndex++;
    }

    /* now assign all other input terminals */
    Index = 0;
    do
    {
        /* find an input terminal */
        Status = UsbAudio_FindTerminalDescriptorAtIndexWithSubtypeAndTerminalType(TerminalIdsLength, TerminalIds, Index, USB_AUDIO_INPUT_TERMINAL, USB_AUDIO_UNDEFINED_TERMINAL_TYPE, &Descriptor, &DescriptorIndex);
        if (Status != UA_STATUS_SUCCESS)
        {
            /* no more items */
            break;
        }

        if (Consumed & (1 << DescriptorIndex))
        {
            /* terminal has already been assigned to an pin */
            Index++;
            continue;
        }

        /* assign terminal */
        PinArray[PinIndex] = Descriptor->bTerminalID;
        Consumed |= 1 << DescriptorIndex;
        PinIndex++;
        Index++;

    }while(Status == UA_STATUS_SUCCESS);


    /* now assign all other output terminals */
    Index = 0;
    do
    {
        /* find an input terminal */
        Status = UsbAudio_FindTerminalDescriptorAtIndexWithSubtypeAndTerminalType(TerminalIdsLength, TerminalIds, Index, USB_AUDIO_OUTPUT_TERMINAL, USB_AUDIO_UNDEFINED_TERMINAL_TYPE, &Descriptor, &DescriptorIndex);
        if (Status != UA_STATUS_SUCCESS)
        {
            /* no more items */
            break;
        }

        if (Consumed & (1 << DescriptorIndex))
        {
            /* terminal has already been assigned to an pin */
            Index++;
            continue;
        }

        /* assign terminal */
        PinArray[PinIndex] = Descriptor->bTerminalID;
        Consumed |= 1 << DescriptorIndex;
        PinIndex++;
        Index++;

    }while(Status == UA_STATUS_SUCCESS);

    /* store pin count */
    DPRINT("Consumed %lx PinIndex %lu\n", Consumed, PinIndex);
    *PinArrayCount = PinIndex;
    return UA_STATUS_SUCCESS;
}
