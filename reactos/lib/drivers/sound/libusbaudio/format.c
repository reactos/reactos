/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/libusbaudio/format.c
 * PURPOSE:         USB AUDIO Parser
 * PROGRAMMER:      Johannes Anderwald
 */

#include "priv.h"

KSDATARANGE StandardDataRange =
{
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        {STATIC_KSDATAFORMAT_TYPE_AUDIO},
        {STATIC_KSDATAFORMAT_SUBTYPE_ANALOG},
        {STATIC_KSDATAFORMAT_SPECIFIER_NONE}
    }
};

GUID DataFormatTypeAudio = {STATIC_KSDATAFORMAT_TYPE_AUDIO};
GUID DataFormatSubTypePCM = {STATIC_KSDATAFORMAT_SUBTYPE_PCM};
GUID DataFormatSpecifierWaveFormat = {STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX};

USBAUDIO_STATUS
UsbAudio_AssignDataRanges(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorSize,
    IN PKSFILTER_DESCRIPTOR FilterDescriptor, 
    IN PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors, 
    IN ULONG InterfaceCount, 
    IN ULONG InterfaceIndex, 
    IN PULONG TerminalIds)
{
    ULONG Count, Index;
    USBAUDIO_STATUS Status;
    PUSB_COMMON_DESCRIPTOR * Descriptors;
    PUSB_AUDIO_STREAMING_INTERFACE_DESCRIPTOR HeaderDescriptor;
    PKSDATAFORMAT_WAVEFORMATEX WaveFormat;
    PKSDATARANGE *DataRanges;
    PKSPIN_DESCRIPTOR PinDescriptor;
    PUSB_AUDIO_STREAMING_FORMAT_TYPE_1 FormatType;

    /* count audio descriptors */
    Status = UsbAudio_CountAudioDescriptors(ConfigurationDescriptor, ConfigurationDescriptorSize, InterfaceDescriptors, InterfaceCount, InterfaceIndex, &Count);
    if (Status != UA_STATUS_SUCCESS || Count < 2)
    {
        /* ignore failure */
        DPRINT1("[LIBUSBAUDIO] Failed to count descriptors with %x Count %lx for InterfaceIndex %lx InterfaceCount %lx\n", Status, Count, InterfaceIndex, InterfaceCount);
        return UA_STATUS_SUCCESS;
    }

    /* create descriptor array */
    Status = UsbAudio_CreateAudioDescriptorArray(Context, ConfigurationDescriptor, ConfigurationDescriptorSize, InterfaceDescriptors, InterfaceCount, InterfaceIndex, Count, &Descriptors);
    if (Status != UA_STATUS_SUCCESS)
    {
        /* ignore failure */
        DPRINT1("[LIBUSBAUDIO] Failed to create audio descriptor array Count %lx Status %x\n", Count, Status);
        return UA_STATUS_SUCCESS;
    }

    /* get header */
    HeaderDescriptor = (PUSB_AUDIO_STREAMING_INTERFACE_DESCRIPTOR)Descriptors[0];
    if (!HeaderDescriptor || HeaderDescriptor->bDescriptorType != USB_AUDIO_STREAMING_INTERFACE_INTERFACE_DESCRIPTOR_TYPE ||
        HeaderDescriptor->bDescriptorSubtype != 0x01)
    {
        /* header missing or mis-aligned */
        DPRINT1("[LIBUSBAUDIO] Failed to retrieve audio header %p\n", HeaderDescriptor);
        return UA_STATUS_SUCCESS;
    }

    /* FIXME: only PCM is supported */
    if (HeaderDescriptor->wFormatTag != WAVE_FORMAT_PCM)
    {
        /* not supported  */
        DPRINT1("[LIBUSBAUDIO] Only PCM is currenly supported wFormatTag %x\n", HTONS(HeaderDescriptor->wFormatTag));
        return UA_STATUS_SUCCESS;
    }

    /* check format descriptor */
    FormatType = (PUSB_AUDIO_STREAMING_FORMAT_TYPE_1)Descriptors[1];
    if (!FormatType || FormatType->bDescriptorType != USB_AUDIO_STREAMING_INTERFACE_INTERFACE_DESCRIPTOR_TYPE || 
        FormatType->bDescriptorSubtype != 0x02 || FormatType->bFormatType != 0x01 || FormatType->bSamFreqType != 1)
    {
        /* unexpected format descriptor */
        DPRINT1("[LIBUSBAUDIO] Unexpected format descriptor %p bDescriptorType %x bDescriptorSubtype %x bFormatType %x bSamFreqType %x\n", FormatType,
                FormatType->bDescriptorType, FormatType->bDescriptorSubtype, FormatType->bFormatType, FormatType->bSamFreqType);
        return UA_STATUS_SUCCESS;
    }


    /* now search pin position */
    for(Index = 0; Index < FilterDescriptor->PinDescriptorsCount; Index++)
    {
        //DPRINT("bTerminalLink %x Ids %lx\n", HeaderDescriptor->bTerminalLink, TerminalIds[Index]);
        if (HeaderDescriptor->bTerminalLink == TerminalIds[Index])
        {
            /* alloc wave format */
            WaveFormat = (PKSDATAFORMAT_WAVEFORMATEX)Context->Alloc(sizeof(KSDATAFORMAT_WAVEFORMATEX));
            if (!WaveFormat)
                return UA_STATUS_NO_MEMORY;

            /* init wave format */
            WaveFormat->WaveFormatEx.cbSize = sizeof(WAVEFORMATEX);
            WaveFormat->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat->WaveFormatEx.nChannels  = FormatType->bNrChannels;
            WaveFormat->WaveFormatEx.wBitsPerSample = FormatType->bBitResolution;
            WaveFormat->WaveFormatEx.nSamplesPerSec = ((FormatType->tSamFreq[2] & 0xFF) << 16) | ((FormatType->tSamFreq[1] & 0xFF)<< 8) | (FormatType->tSamFreq[0] & 0xFF);
            WaveFormat->WaveFormatEx.nBlockAlign = (WaveFormat->WaveFormatEx.nChannels * WaveFormat->WaveFormatEx.wBitsPerSample) / 8; 
            WaveFormat->WaveFormatEx.nAvgBytesPerSec = WaveFormat->WaveFormatEx.nSamplesPerSec * WaveFormat->WaveFormatEx.nBlockAlign;

            /* FIXME apply padding */
            WaveFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
            Context->Copy(&WaveFormat->DataFormat.MajorFormat, &DataFormatTypeAudio, sizeof(GUID));
            Context->Copy(&WaveFormat->DataFormat.SubFormat, &DataFormatSubTypePCM, sizeof(GUID));
            Context->Copy(&WaveFormat->DataFormat.Specifier, &DataFormatSpecifierWaveFormat, sizeof(GUID));

            //C_ASSERT(sizeof(WAVEFORMATEX) + sizeof(KSDATAFORMAT) == 82);

            /* get corresponding pin descriptor */
            PinDescriptor = (PKSPIN_DESCRIPTOR)&FilterDescriptor->PinDescriptors[Index].PinDescriptor;

            /* alloc data range */
            DataRanges = (PKSDATARANGE*)Context->Alloc(sizeof(PKSDATARANGE) * (PinDescriptor->DataRangesCount+1));
            if (!DataRanges)
            {
                Context->Free(WaveFormat);
                return UA_STATUS_NO_MEMORY;
            }

            if (PinDescriptor->DataRangesCount)
            {
                /* copy old range */
                Context->Copy(DataRanges, (PVOID)PinDescriptor->DataRanges, sizeof(PKSDATARANGE) *  PinDescriptor->DataRangesCount);

                /* free old range */
                Context->Free((PVOID)PinDescriptor->DataRanges);
            }

            /* assign new range */
            PinDescriptor->DataRanges = DataRanges;
            DataRanges[PinDescriptor->DataRangesCount] = (PKSDATARANGE)WaveFormat;
            PinDescriptor->DataRangesCount++;
            return UA_STATUS_SUCCESS;
        }

    }
    return UA_STATUS_SUCCESS;
}


