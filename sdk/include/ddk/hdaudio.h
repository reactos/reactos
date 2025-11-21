#ifndef _HDAUDIO_H_
#define _HDAUDIO_H_

#ifdef _MSC_VER
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#endif

DEFINE_GUID(GUID_HDAUDIO_BUS_INTERFACE, 0xd2eaf88b, 0xab18, 0x41a8, 0xb6, 0x64, 0x8d, 0x59, 0x21, 0x67, 0x67, 0x1b);
DEFINE_GUID(GUID_HDAUDIO_BUS_INTERFACE_BDL, 0xb4d65397, 0x5634, 0x40b0, 0xb0, 0x68, 0xf5, 0xb9, 0xf8, 0xb9, 0x67, 0xa5);
DEFINE_GUID(GUID_HDAUDIO_BUS_INTERFACE_V2, 0xb52af5fb, 0x424b, 0x4bb9, 0xa1, 0x60, 0x5b, 0x38, 0xbe, 0x94, 0xe5, 0x68);
DEFINE_GUID(GUID_HDAUDIO_BUS_INTERFACE_V3, 0xdc25da12, 0xbcd1, 0x4c7f, 0x84, 0xe1, 0x02, 0xad, 0xb6, 0xdc, 0xdc, 0xbf);
DEFINE_GUID(GUID_HDAUDIO_BUS_CLASS, 0xbbd1a745, 0xadd6, 0x4575, 0x9c, 0x2e, 0x9b, 0x42, 0x8d, 0x1c, 0x32, 0x66);

#ifndef _HDAUDIO_CODEC_TRANSFER_
#define _HDAUDIO_CODEC_TRANSFER_

typedef struct _HDAUDIO_CODEC_COMMAND {
  union {
    struct {
      ULONG Data:8;
      ULONG VerbId:12;
      ULONG Node:8;
      ULONG CodecAddress:4;
    } Verb8;
    struct {
      ULONG Data:16;
      ULONG VerbId:4;
      ULONG Node:8;
      ULONG CodecAddress:4;
    } Verb16;
    ULONG Command;
  };
} HDAUDIO_CODEC_COMMAND, *PHDAUDIO_CODEC_COMMAND;

typedef struct _HDAUDIO_CODEC_RESPONSE {
  union {
    struct {
      union {
        struct {
          ULONG Response:26;
          ULONG Tag:6;
          } Unsolicited;
          ULONG Response;
      };
      ULONG SDataIn:4;
      ULONG IsUnsolicitedResponse:1;
      ULONG :25;
      ULONG HasFifoOverrun:1;
      ULONG IsValid:1;
    };
    ULONGLONG CompleteResponse;
  };
} HDAUDIO_CODEC_RESPONSE, *PHDAUDIO_CODEC_RESPONSE;

typedef struct _HDAUDIO_CODEC_TRANSFER {
  HDAUDIO_CODEC_COMMAND Output;
  HDAUDIO_CODEC_RESPONSE Input;
} HDAUDIO_CODEC_TRANSFER, *PHDAUDIO_CODEC_TRANSFER;

#endif /* _HDAUDIO_CODEC_TRANSFER_ */

typedef struct _HDAUDIO_STREAM_FORMAT {
  ULONG SampleRate;
  USHORT ValidBitsPerSample;
  USHORT ContainerSize;
  USHORT NumberOfChannels;
} HDAUDIO_STREAM_FORMAT, *PHDAUDIO_STREAM_FORMAT;

typedef struct _HDAUDIO_CONVERTER_FORMAT {
  union {
    struct {
      USHORT NumberOfChannels:4;
      USHORT BitsPerSample:3;
      USHORT :1;
      USHORT SampleRate:7;
      USHORT StreamType:1;
    };
    USHORT ConverterFormat;
  };
} HDAUDIO_CONVERTER_FORMAT, *PHDAUDIO_CONVERTER_FORMAT;

typedef enum _HDAUDIO_STREAM_STATE {
  ResetState = 0,
  StopState = 1,
  PauseState = 1,
  RunState = 2
} HDAUDIO_STREAM_STATE, *PHDAUDIO_STREAM_STATE;

typedef VOID
(NTAPI *PHDAUDIO_TRANSFER_COMPLETE_CALLBACK)(
  HDAUDIO_CODEC_TRANSFER *,
  PVOID);

typedef VOID
(NTAPI *PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK)(
  HDAUDIO_CODEC_RESPONSE,
  PVOID);

typedef VOID
(NTAPI * PHDAUDIO_DMA_NOTIFICATION_CALLBACK)(
    PVOID Context,
    LARGE_INTEGER Progress);

typedef struct _HDAUDIO_DEVICE_INFORMATION {
  USHORT Size; /* size of this structure */
  USHORT DeviceVersion; /* maj.min (maj is high byte, min is low byte) */
  USHORT DriverVersion; /* maj.min (maj is high byte, min is low byte) */
  USHORT CodecsDetected; /* mask of codecs present. Bit number == SDI line number */
  BOOLEAN IsStripingSupported; /* TRUE if striping (2 SDO lines) is supported */
} HDAUDIO_DEVICE_INFORMATION, *PHDAUDIO_DEVICE_INFORMATION;

typedef struct _HDAUDIO_DEVICE_INFORMATION_V2 {
 USHORT  Size;
 USHORT  DeviceVersion;
 USHORT  DriverVersion;
 USHORT  CodecsDetected;
 BOOLEAN IsStripingSupported;
 UCHAR   CtrlRevision;
 USHORT  CtrlVendorId;
 USHORT  CtrlDeviceId;
} HDAUDIO_DEVICE_INFORMATION_V2, *PHDAUDIO_DEVICE_INFORMATION_V2;

typedef struct _HDAUDIO_BUFFER_DESCRIPTOR {
  PHYSICAL_ADDRESS Address;
  ULONG Length;
  ULONG InterruptOnCompletion;
} HDAUDIO_BUFFER_DESCRIPTOR, *PHDAUDIO_BUFFER_DESCRIPTOR;

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PTRANSFER_CODEC_VERBS)(
  _In_ PVOID _context,
  _In_ ULONG Count,
  _Inout_updates_(Count) PHDAUDIO_CODEC_TRANSFER CodecTransfer,
  _In_opt_ PHDAUDIO_TRANSFER_COMPLETE_CALLBACK Callback,
  _In_opt_ PVOID Context);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PALLOCATE_CAPTURE_DMA_ENGINE)(
  _In_ PVOID _context,
  _In_ UCHAR CodecAddress,
  _In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
  _Out_ PHANDLE Handle,
  _Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PALLOCATE_RENDER_DMA_ENGINE)(
  _In_ PVOID _context,
  _In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
  _In_ BOOLEAN Stripe,
  _Out_ PHANDLE Handle,
  _Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PCHANGE_BANDWIDTH_ALLOCATION)(
  _In_ PVOID _context,
  _In_ HANDLE Handle,
  _In_ PHDAUDIO_STREAM_FORMAT StreamFormat,
  _Out_ PHDAUDIO_CONVERTER_FORMAT ConverterFormat);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PALLOCATE_DMA_BUFFER)(
  _In_ PVOID _context,
  _In_ HANDLE Handle,
  _In_ SIZE_T RequestedBufferSize,
  _Out_ PMDL *BufferMdl,
  _Out_ PSIZE_T AllocatedBufferSize,
  _Out_ PUCHAR StreamId,
  _Out_ PULONG FifoSize);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFREE_DMA_BUFFER)(
  _In_ PVOID _context,
  _In_ HANDLE Handle);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFREE_DMA_ENGINE)(
  _In_ PVOID _context,
  _In_ HANDLE Handle);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PSET_DMA_ENGINE_STATE)(
  _In_ PVOID _context,
  _In_ HDAUDIO_STREAM_STATE StreamState,
  _In_ ULONG NumberOfHandles,
  _In_reads_(NumberOfHandles) PHANDLE Handles);

typedef VOID
(NTAPI *PGET_WALL_CLOCK_REGISTER)(
  _In_ PVOID _context,
  _Out_ PULONG *Wallclock);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PGET_LINK_POSITION_REGISTER)(
  _In_ PVOID _context,
  _In_ HANDLE Handle,
  _Out_ PULONG *Position);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PREGISTER_EVENT_CALLBACK)(
  _In_ PVOID _context,
  _In_ PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK Routine,
  _In_opt_ PVOID Context,
  _Out_ PUCHAR Tag);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PUNREGISTER_EVENT_CALLBACK)(
  _In_ PVOID _context,
  _In_ UCHAR Tag);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PGET_DEVICE_INFORMATION)(
  _In_ PVOID _context,
  _Out_ PHDAUDIO_DEVICE_INFORMATION DeviceInformation);

typedef VOID
(NTAPI *PGET_RESOURCE_INFORMATION)(
  _In_ PVOID _context,
  _Out_ PUCHAR CodecAddress,
  _Out_ PUCHAR FunctionGroupStartNode);

typedef NTSTATUS
(NTAPI *PREGISTER_NOTIFICATION_CALLBACK)(
  PVOID _context,
  HANDLE Handle,
  PDEVICE_OBJECT Fdo,
  PHDAUDIO_DMA_NOTIFICATION_CALLBACK NotificationCallback,
  PVOID CallbackContext);

typedef NTSTATUS
(NTAPI *PUNREGISTER_NOTIFICATION_CALLBACK)(
  PVOID _context,
  HANDLE Handle,
  PHDAUDIO_DMA_NOTIFICATION_CALLBACK NotificationCallback,
  PVOID CallbackContext);


typedef struct _HDAUDIO_BUS_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PTRANSFER_CODEC_VERBS TransferCodecVerbs;
  PALLOCATE_CAPTURE_DMA_ENGINE AllocateCaptureDmaEngine;
  PALLOCATE_RENDER_DMA_ENGINE AllocateRenderDmaEngine;
  PCHANGE_BANDWIDTH_ALLOCATION ChangeBandwidthAllocation;
  PALLOCATE_DMA_BUFFER AllocateDmaBuffer;
  PFREE_DMA_BUFFER FreeDmaBuffer;
  PFREE_DMA_ENGINE FreeDmaEngine;
  PSET_DMA_ENGINE_STATE SetDmaEngineState;
  PGET_WALL_CLOCK_REGISTER GetWallClockRegister;
  PGET_LINK_POSITION_REGISTER GetLinkPositionRegister;
  PREGISTER_EVENT_CALLBACK RegisterEventCallback;
  PUNREGISTER_EVENT_CALLBACK UnregisterEventCallback;
  PGET_DEVICE_INFORMATION GetDeviceInformation;
  PGET_RESOURCE_INFORMATION GetResourceInformation;
} HDAUDIO_BUS_INTERFACE, *PHDAUDIO_BUS_INTERFACE;

typedef void
(NTAPI *PHDAUDIO_BDL_ISR)(
  _In_ VOID *Context,
  _In_ ULONG InterruptBitMask);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PALLOCATE_CONTIGUOUS_DMA_BUFFER)(
  _In_ PVOID _context,
  _In_ HANDLE Handle,
  ULONG RequestedBufferSize,
  _Out_ PVOID *DataBuffer,
  _Out_ PHDAUDIO_BUFFER_DESCRIPTOR *BdlBuffer);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFREE_CONTIGUOUS_DMA_BUFFER)(
  _In_ PVOID _context,
  _In_ HANDLE Handle);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PSETUP_DMA_ENGINE_WITH_BDL)(
  _In_reads_bytes_(BufferLength) PVOID _context,
  _In_ HANDLE Handle,
  _In_ ULONG BufferLength,
  _In_ ULONG Lvi,
  _In_ PHDAUDIO_BDL_ISR Isr,
  _In_ PVOID Context,
  _Out_ PUCHAR StreamId,
  _Out_ PULONG FifoSize);

typedef struct _HDAUDIO_BUS_INTERFACE_BDL {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PTRANSFER_CODEC_VERBS TransferCodecVerbs;
  PALLOCATE_CAPTURE_DMA_ENGINE AllocateCaptureDmaEngine;
  PALLOCATE_RENDER_DMA_ENGINE AllocateRenderDmaEngine;
  PCHANGE_BANDWIDTH_ALLOCATION ChangeBandwidthAllocation;
  PALLOCATE_CONTIGUOUS_DMA_BUFFER AllocateContiguousDmaBuffer;
  PSETUP_DMA_ENGINE_WITH_BDL SetupDmaEngineWithBdl;
  PFREE_CONTIGUOUS_DMA_BUFFER FreeContiguousDmaBuffer;
  PFREE_DMA_ENGINE FreeDmaEngine;
  PSET_DMA_ENGINE_STATE SetDmaEngineState;
  PGET_WALL_CLOCK_REGISTER GetWallClockRegister;
  PGET_LINK_POSITION_REGISTER GetLinkPositionRegister;
  PREGISTER_EVENT_CALLBACK RegisterEventCallback;
  PUNREGISTER_EVENT_CALLBACK UnregisterEventCallback;
  PGET_DEVICE_INFORMATION GetDeviceInformation;
  PGET_RESOURCE_INFORMATION GetResourceInformation;
} HDAUDIO_BUS_INTERFACE_BDL, *PHDAUDIO_BUS_INTERFACE_BDL;

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PALLOCATE_DMA_BUFFER_WITH_NOTIFICATION)(
  _In_ PVOID _context,
  _In_ HANDLE Handle,
  _In_ ULONG NotificationCount,
  _In_ SIZE_T RequestedBufferSize,
  _Out_ PMDL *BufferMdl,
  _Out_ PSIZE_T AllocatedBufferSize,
  _Out_ PSIZE_T OffsetFromFirstPage,
  _Out_ PUCHAR StreamId,
  _Out_ PULONG FifoSize);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFREE_DMA_BUFFER_WITH_NOTIFICATION)(
  _In_ PVOID _context,
  _In_ HANDLE Handle,
  _In_ PMDL BufferMdl,
  _In_ SIZE_T BufferSize);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PREGISTER_NOTIFICATION_EVENT)(
  _In_ PVOID _context,
  _In_ HANDLE Handle,
  _In_ PKEVENT NotificationEvent);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PUNREGISTER_NOTIFICATION_EVENT)(
  _In_ PVOID _context,
  _In_ HANDLE Handle,
  _In_ PKEVENT NotificationEvent);

typedef struct _HDAUDIO_BUS_INTERFACE_V2 {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PTRANSFER_CODEC_VERBS TransferCodecVerbs;
  PALLOCATE_CAPTURE_DMA_ENGINE AllocateCaptureDmaEngine;
  PALLOCATE_RENDER_DMA_ENGINE AllocateRenderDmaEngine;
  PCHANGE_BANDWIDTH_ALLOCATION ChangeBandwidthAllocation;
  PALLOCATE_DMA_BUFFER AllocateDmaBuffer;
  PFREE_DMA_BUFFER FreeDmaBuffer;
  PFREE_DMA_ENGINE FreeDmaEngine;
  PSET_DMA_ENGINE_STATE SetDmaEngineState;
  PGET_WALL_CLOCK_REGISTER GetWallClockRegister;
  PGET_LINK_POSITION_REGISTER GetLinkPositionRegister;
  PREGISTER_EVENT_CALLBACK RegisterEventCallback;
  PUNREGISTER_EVENT_CALLBACK UnregisterEventCallback;
  PGET_DEVICE_INFORMATION GetDeviceInformation;
  PGET_RESOURCE_INFORMATION GetResourceInformation;
  PALLOCATE_DMA_BUFFER_WITH_NOTIFICATION AllocateDmaBufferWithNotification;
  PFREE_DMA_BUFFER_WITH_NOTIFICATION FreeDmaBufferWithNotification;
  PREGISTER_NOTIFICATION_EVENT RegisterNotificationEvent;
  PUNREGISTER_NOTIFICATION_EVENT UnregisterNotificationEvent;
} HDAUDIO_BUS_INTERFACE_V2, *PHDAUDIO_BUS_INTERFACE_V2;

typedef struct _HDAUDIO_BUS_INTERFACE_V3 {
  USHORT                                 Size;
  USHORT                                 Version;
  PVOID                                  Context;
  PINTERFACE_REFERENCE                   InterfaceReference;
  PINTERFACE_DEREFERENCE                 InterfaceDereference;
  PTRANSFER_CODEC_VERBS                  TransferCodecVerbs;
  PALLOCATE_CAPTURE_DMA_ENGINE           AllocateCaptureDmaEngine;
  PALLOCATE_RENDER_DMA_ENGINE            AllocateRenderDmaEngine;
  PCHANGE_BANDWIDTH_ALLOCATION           ChangeBandwidthAllocation;
  PALLOCATE_DMA_BUFFER                   AllocateDmaBuffer;
  PFREE_DMA_BUFFER                       FreeDmaBuffer;
  PFREE_DMA_ENGINE                       FreeDmaEngine;
  PSET_DMA_ENGINE_STATE                  SetDmaEngineState;
  PGET_WALL_CLOCK_REGISTER               GetWallClockRegister;
  PGET_LINK_POSITION_REGISTER            GetLinkPositionRegister;
  PREGISTER_EVENT_CALLBACK               RegisterEventCallback;
  PUNREGISTER_EVENT_CALLBACK             UnregisterEventCallback;
  PGET_DEVICE_INFORMATION                GetDeviceInformation;
  PGET_RESOURCE_INFORMATION              GetResourceInformation;
  PALLOCATE_DMA_BUFFER_WITH_NOTIFICATION AllocateDmaBufferWithNotification;
  PFREE_DMA_BUFFER_WITH_NOTIFICATION     FreeDmaBufferWithNotification;
  PREGISTER_NOTIFICATION_EVENT           RegisterNotificationEvent;
  PUNREGISTER_NOTIFICATION_EVENT         UnregisterNotificationEvent;
  PREGISTER_NOTIFICATION_CALLBACK        RegisterNotificationCallback;
  PUNREGISTER_NOTIFICATION_CALLBACK      UnregisterNotificationCallback;
}HDAUDIO_BUS_INTERFACE_V3, *PHDAUDIO_BUS_INTERFACE_V3;

#ifdef _MSC_VER
#pragma warning(default:4201)
#pragma warning(default:4214)
#endif

#endif /* _HDAUDIO_H_ */
