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

#define DEFINE_KSPROPERTY_ITEM_AUDIO_VOLUME(Handler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_AUDIO_VOLUMELEVEL,\
        (Handler),\
        sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),\
        sizeof(LONG),\
        (Handler), NULL, 0, NULL, NULL, 0)


#define DEFINE_KSPROPERTY_TABLE_AUDIO_VOLUME(TopologySet, Handler)\
DEFINE_KSPROPERTY_TABLE(TopologySet) {\
    DEFINE_KSPROPERTY_ITEM_AUDIO_VOLUME(Handler)\
}

#define DEFINE_KSPROPERTY_ITEM_AUDIO_MUTE(Handler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_AUDIO_MUTE,\
        (Handler),\
        sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),\
        sizeof(BOOL),\
        (Handler), NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_TABLE_AUDIO_MUTE(TopologySet, Handler)\
DEFINE_KSPROPERTY_TABLE(TopologySet) {\
    DEFINE_KSPROPERTY_ITEM_AUDIO_MUTE(Handler)\
}

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
    UCHAR bUnitID;
    UCHAR bSourceID;
    UCHAR bControlSize;
    UCHAR bmaControls[1];
    UCHAR iFeature;
}USB_AUDIO_CONTROL_FEATURE_UNIT_DESCRIPTOR, *PUSB_AUDIO_CONTROL_FEATURE_UNIT_DESCRIPTOR;

typedef struct
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDescriptorSubtype;
    UCHAR bUnitID;
    UCHAR bNrInPins;
    UCHAR baSourceID[1];
    UCHAR bNrChannels;
    USHORT wChannelConfig;
    UCHAR iChannelNames;
    UCHAR bmControls;
    UCHAR iMixer;
}USB_AUDIO_CONTROL_MIXER_UNIT_DESCRIPTOR, *PUSB_AUDIO_CONTROL_MIXER_UNIT_DESCRIPTOR;

typedef struct
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDescriptorSubtype;
    UCHAR bUnitID;
    UCHAR bNrInPins;
    UCHAR baSourceID[1];
    UCHAR iSelector;
}USB_AUDIO_CONTROL_SELECTOR_UNIT_DESCRIPTOR, *PUSB_AUDIO_CONTROL_SELECTOR_UNIT_DESCRIPTOR;


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

typedef struct
{
    PUSB_COMMON_DESCRIPTOR Descriptor;
    ULONG NodeCount;
    ULONG Nodes[20];
}NODE_CONTEXT, *PNODE_CONTEXT;

typedef struct __DEVICE_EXTENSION__
{
    PDEVICE_OBJECT LowerDevice;                                  /* lower device*/
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;       /* usb configuration descriptor */
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;                     /* usb device descriptor */
    PUSBD_INTERFACE_INFORMATION InterfaceInfo;                   /* interface information */
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;               /* configuration handle */
    PNODE_CONTEXT NodeContext;                                   /* node context */
    ULONG NodeContextCount;                                      /* node context count */
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct
{
    PDEVICE_EXTENSION DeviceExtension;                           /* device extension */
    PDEVICE_OBJECT LowerDevice;                                  /* lower device*/

}FILTER_CONTEXT, *PFILTER_CONTEXT;

typedef struct
{
    PDEVICE_EXTENSION DeviceExtension;                           /* device extension */
    PDEVICE_OBJECT LowerDevice;                                  /* lower device*/
    LIST_ENTRY IrpListHead;                                      /* irp list*/
    LIST_ENTRY DoneIrpListHead;                                  /* irp done list head */
    KSPIN_LOCK IrpListLock;                                      /* irp list lock*/
    PUCHAR Buffer;                                               /* iso buffer*/
    ULONG BufferSize;                                            /* iso buffer size */
    ULONG BufferOffset;                                          /* buffer offset */
    ULONG BufferLength;                                          /* remaining render bytes */
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;               /* interface descriptor */
    WORK_QUEUE_ITEM  CaptureWorkItem;                            /* work item */
    PKSWORKER        CaptureWorker;                              /* capture worker */
    WORK_QUEUE_ITEM  StarvationWorkItem;                            /* work item */
    PKSWORKER        StarvationWorker;                              /* capture worker */
}PIN_CONTEXT, *PPIN_CONTEXT;

/* filter.c */

NTSTATUS
NTAPI
USBAudioCreateFilterContext(
    PKSDEVICE Device);

PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR
UsbAudioGetStreamingTerminalDescriptorByIndex(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN ULONG Index);

/* pool.c */
PVOID
NTAPI
AllocFunction(
    IN ULONG ItemSize);

VOID
NTAPI
FreeFunction(
    IN PVOID Item);

VOID
NTAPI
CountTerminalUnits(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    OUT PULONG NonStreamingTerminalDescriptorCount,
    OUT PULONG TotalTerminalDescriptorCount);

/* usbaudio.c */

NTSTATUS
SubmitUrbSync(
    IN PDEVICE_OBJECT Device,
    IN PURB Urb);

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
UsbAudioPinDataIntersect(
    _In_  PVOID        Context,
    _In_  PIRP         Irp,
    _In_  PKSP_PIN     Pin,
    _In_  PKSDATARANGE DataRange,
    _In_  PKSDATARANGE MatchingDataRange,
    _In_  ULONG        DataBufferSize,
    _Out_ PVOID        Data,
    _Out_ PULONG       DataSize);

NTSTATUS
NTAPI
UsbAudioCaptureComplete(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context);

NTSTATUS
NTAPI
UsbAudioRenderComplete(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context);

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

