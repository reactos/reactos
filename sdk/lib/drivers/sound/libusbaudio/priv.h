#pragma once

#include <pseh/pseh2.h>
#include <ntddk.h>

#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmreg.h>
#include <mmsystem.h>
#include "Usb100.h"
#include <debug.h>

#include "libusbaudio.h"

#define USB_AUDIO_CONTROL_TERMINAL_DESCRIPTOR_TYPE (0x24)
#define USB_AUDIO_STREAMING_INTERFACE_INTERFACE_DESCRIPTOR_TYPE (0x24)

#define USB_AUDIO_INPUT_TERMINAL (0x02)
#define USB_AUDIO_OUTPUT_TERMINAL (0x03)

/* Universal Serial Bus Device Class Definition for Terminal Types Section A 1.1 */
#define USB_AUDIO_PCM_DATA_FORMAT (0x01)
#define USB_AUDIO_PCM8_DATA_FORMAT (0x02)
#define USB_AUDIO_IEEE_FLOAT_DATA_FORMAT (0x03)
#define USB_AUDIO_ALAW_DATA_FORMAT (0x04)
#define USB_AUDIO_MULAW_DATA_FORMAT (0x05)

/* Universal Serial Bus Device Class Definition for Terminal Types Section A 1.1 */
#define USB_AUDIO_DATA_FORMAT_TYPE_1 (0x01)
#define USB_AUDIO_DATA_FORMAT_TYPE_2 (0x02)
#define USB_AUDIO_DATA_FORMAT_TYPE_3 (0x03)



/* Universal Serial Bus Device Class Definition for Terminal Types Section 2.2 */
#define USB_AUDIO_STREAMING_TERMINAL_TYPE (0x0101)

#define USB_AUDIO_MICROPHONE_TERMINAL_TYPE (0x0201)
#define USB_AUDIO_DESKTOP_MICROPHONE_TERMINAL_TYPE (0x0202)
#define USB_AUDIO_PERSONAL_MICROPHONE_TERMINAL_TYPE (0x0203)
#define USB_AUDIO_OMMNI_MICROPHONE_TERMINAL_TYPE (0x0204)
#define USB_AUDIO_ARRAY_MICROPHONE_TERMINAL_TYPE (0x0205)
#define USB_AUDIO_ARRAY_PROCESSING_MICROPHONE_TERMINAL_TYPE (0x0206)

#define USB_AUDIO_SPEAKER_TERMINAL_TYPE (0x0301)
#define USB_HEADPHONES_SPEAKER_TERMINAL_TYPE (0x0302)
#define USB_AUDIO_HMDA_TERMINAL_TYPE (0x0303)
#define USB_AUDIO_DESKTOP_SPEAKER_TERMINAL_TYPE (0x0304)
#define USB_AUDIO_ROOM_SPEAKER_TERMINAL_TYPE (0x0305)
#define USB_AUDIO_COMMUNICATION_SPEAKER_TERMINAL_TYPE (0x0306)
#define USB_AUDIO_SUBWOOFER_TERMINAL_TYPE (0x0307)
#define USB_AUDIO_UNDEFINED_TERMINAL_TYPE (0xFFFF)

#define HTONS(x) ( ( (((unsigned short)(x)) >>8) & 0xff) | \
                 ((((unsigned short)(x)) & 0xff)<<8) )

#include <pshpack1.h>

typedef struct
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDescriptorSubtype;
    UCHAR bTerminalID;
    USHORT wTerminalType;
    UCHAR bAssocTerminal;
    UCHAR bSourceID;
    UCHAR iTerminal;
}USB_AUDIO_CONTROL_OUTPUT_TERMINAL_DESCRIPTOR, *PUSB_AUDIO_CONTROL_OUTPUT_TERMINAL_DESCRIPTOR;


typedef struct
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDescriptorSubtype;
    UCHAR bTerminalID;
    USHORT wTerminalType;
    UCHAR bAssocTerminal;
    UCHAR bNrChannels;
    USHORT wChannelConfig;
    UCHAR iChannelNames;
    UCHAR iTerminal;
}USB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR, *PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR;

typedef struct
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDescriptorSubtype;
    UCHAR bTerminalLink;
    UCHAR bDelay;
    USHORT wFormatTag;
}USB_AUDIO_STREAMING_INTERFACE_DESCRIPTOR, *PUSB_AUDIO_STREAMING_INTERFACE_DESCRIPTOR;
C_ASSERT(sizeof(USB_AUDIO_STREAMING_INTERFACE_DESCRIPTOR) == 0x07);

typedef struct
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDescriptorSubtype;
    UCHAR bFormatType;
    UCHAR bNrChannels;
    UCHAR bSubframeSize;
    UCHAR bBitResolution;
    UCHAR bSamFreqType;
    UCHAR tSamFreq[3];
}USB_AUDIO_STREAMING_FORMAT_TYPE_1, *PUSB_AUDIO_STREAMING_FORMAT_TYPE_1;
C_ASSERT(sizeof(USB_AUDIO_STREAMING_FORMAT_TYPE_1) == 0x0B);

/* format.c */

USBAUDIO_STATUS
UsbAudio_AssignDataRanges(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorSize,
    IN PKSFILTER_DESCRIPTOR FilterDescriptor, 
    IN PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors, 
    IN ULONG InterfaceCount, 
    IN ULONG InterfaceIndex, 
    IN PULONG TerminalIds);


/* parser.c */

USBAUDIO_STATUS
UsbAudio_CountInterfaceDescriptors(
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    OUT PULONG DescriptorCount);

USBAUDIO_STATUS
UsbAudio_CreateInterfaceDescriptorsArray(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN ULONG ArrayLength, 
    OUT PUSB_COMMON_DESCRIPTOR ** Array);

ULONG
UsbAudio_GetAudioControlInterfaceIndex(
    IN PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors,
    IN ULONG DescriptorCount);

USBAUDIO_STATUS
UsbAudio_CountAudioDescriptors(
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors,
    IN ULONG InterfaceDescriptorCount,
    IN ULONG InterfaceDescriptorIndex,
    OUT PULONG DescriptorCount);

USBAUDIO_STATUS
UsbAudio_CreateAudioDescriptorArray(
    IN PUSBAUDIO_CONTEXT Context,
    IN PUCHAR ConfigurationDescriptor,
    IN ULONG ConfigurationDescriptorLength,
    IN PUSB_COMMON_DESCRIPTOR * InterfaceDescriptors,
    IN ULONG InterfaceDescriptorCount,
    IN ULONG InterfaceDescriptorIndex,
    IN ULONG ArrayLength,
    OUT PUSB_COMMON_DESCRIPTOR ** Array);

USBAUDIO_STATUS
UsbAudio_AssignTerminalIds(
    IN PUSBAUDIO_CONTEXT Context,
    IN ULONG TerminalIdsLength,
    IN PUSB_COMMON_DESCRIPTOR * TerminalIds,
    OUT PULONG PinArray,
    OUT PULONG PinArrayCount);

PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR
UsbAudio_GetTerminalDescriptorById(
    IN PUSB_COMMON_DESCRIPTOR *Descriptors,
    IN ULONG DescriptorCount,
    IN ULONG TerminalId);

