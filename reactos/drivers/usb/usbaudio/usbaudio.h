#pragma once

#include <ntddk.h>
#include <portcls.h>
#include <ksmedia.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
#include <usb.h>
#include <usbdlib.h>
#include <debug.h>

#define USBAUDIO_TAG 'AbsU'
#define USB_AUDIO_CONTROL_TERMINAL_DESCRIPTOR_TYPE (0x24)

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

#define USB_AUDIO_INPUT_TERMINAL (0x02)
#define USB_AUDIO_OUTPUT_TERMINAL (0x03)

#include <pshpack1.h>

typedef struct
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDescriptorSubtype;
    USHORT bcdADC;
    USHORT wTotalLength;
    UCHAR bInCollection;
    UCHAR baInterfaceNr;
}USB_AUDIO_CONTROL_INTERFACE_HEADER_DESCRIPTOR, *PUSB_AUDIO_CONTROL_INTERFACE_HEADER_DESCRIPTOR;

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
    UCHAR tSamFreq[1];
}USB_AUDIO_STREAMING_FORMAT_TYPE_DESCRIPTOR, *PUSB_AUDIO_STREAMING_FORMAT_TYPE_DESCRIPTOR;

#include <poppack.h>

typedef struct __DEVICE_EXTENSION__
{
    PDEVICE_OBJECT LowerDevice;                                  /* lower device*/
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;       /* usb configuration descriptor */
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;                     /* usb device descriptor */
    PUSBD_INTERFACE_INFORMATION InterfaceInfo;                   /* interface information */
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;               /* configuration handle */

}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* filter.c */

NTSTATUS
NTAPI
USBAudioCreateFilterContext(
    PKSDEVICE Device);

/* pool.c */
PVOID
NTAPI
AllocFunction(
    IN ULONG ItemSize);

VOID
NTAPI
FreeFunction(
    IN PVOID Item);

/* usbaudio.c */

NTSTATUS
NTAPI
USBAudioAddDevice(
  _In_ PKSDEVICE Device
);

NTSTATUS
NTAPI
USBAudioPnPStart(
  _In_     PKSDEVICE         Device,
  _In_     PIRP              Irp,
  _In_opt_ PCM_RESOURCE_LIST TranslatedResourceList,
  _In_opt_ PCM_RESOURCE_LIST UntranslatedResourceList
);

NTSTATUS
NTAPI
USBAudioPnPQueryStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

VOID
NTAPI
USBAudioPnPCancelStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

VOID
NTAPI
USBAudioPnPStop(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

NTSTATUS
NTAPI
USBAudioPnPQueryRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

VOID
NTAPI
USBAudioPnPCancelRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

VOID
NTAPI
USBAudioPnPRemove(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

NTSTATUS
NTAPI
USBAudioPnPQueryCapabilities(
  _In_    PKSDEVICE            Device,
  _In_    PIRP                 Irp,
  _Inout_ PDEVICE_CAPABILITIES Capabilities
);

VOID
NTAPI
USBAudioPnPSurpriseRemoval(
  _In_ PKSDEVICE Device,
  _In_ PIRP      Irp
);

NTSTATUS
NTAPI
USBAudioPnPQueryPower(
  _In_ PKSDEVICE          Device,
  _In_ PIRP               Irp,
  _In_ DEVICE_POWER_STATE DeviceTo,
  _In_ DEVICE_POWER_STATE DeviceFrom,
  _In_ SYSTEM_POWER_STATE SystemTo,
  _In_ SYSTEM_POWER_STATE SystemFrom,
  _In_ POWER_ACTION       Action
);

VOID
NTAPI
USBAudioPnPSetPower(
  _In_ PKSDEVICE          Device,
  _In_ PIRP               Irp,
  _In_ DEVICE_POWER_STATE To,
  _In_ DEVICE_POWER_STATE From
);

/* pin.c*/

NTSTATUS
NTAPI
USBAudioPinCreate(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
USBAudioPinClose(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
USBAudioPinProcess(
    _In_ PKSPIN Pin);

VOID
NTAPI
USBAudioPinReset(
    _In_ PKSPIN Pin);

NTSTATUS
NTAPI
USBAudioPinSetDataFormat(
    _In_ PKSPIN Pin,
    _In_opt_ PKSDATAFORMAT OldFormat,
    _In_opt_ PKSMULTIPLE_ITEM OldAttributeList,
    _In_ const KSDATARANGE* DataRange,
    _In_opt_ const KSATTRIBUTE_LIST* AttributeRange);

NTSTATUS
NTAPI
USBAudioPinSetDeviceState(
    _In_ PKSPIN Pin,
    _In_ KSSTATE ToState,
    _In_ KSSTATE FromState);

