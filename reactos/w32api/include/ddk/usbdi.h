/*
 * usbdi.h
 *
 * USBD and USB device driver definitions
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __USBDI_H
#define __USBDI_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __USB_H
#error usb.h cannot be included with usbdi.h
#else

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"
#include "usbioctl.h"

#define USBDI_VERSION                     0x300

#define USB_DEFAULT_DEVICE_ADDRESS        0
#define USB_DEFAULT_ENDPOINT_ADDRESS      0
#define USB_DEFAULT_MAX_PACKET            64

#define URB_FROM_IRP(Irp) ((IoGetCurrentIrpStackLocation(Irp))->Parameters.Others.Argument1)

#define URB_FUNCTION_SELECT_CONFIGURATION           0x0000
#define URB_FUNCTION_SELECT_INTERFACE               0x0001
#define URB_FUNCTION_ABORT_PIPE                     0x0002
#define URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL      0x0003
#define URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL   0x0004
#define URB_FUNCTION_GET_FRAME_LENGTH               0x0005
#define URB_FUNCTION_SET_FRAME_LENGTH               0x0006
#define URB_FUNCTION_GET_CURRENT_FRAME_NUMBER       0x0007
#define URB_FUNCTION_CONTROL_TRANSFER               0x0008
#define URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER     0x0009
#define URB_FUNCTION_ISOCH_TRANSFER                 0x000A
#define URB_FUNCTION_RESET_PIPE                     0x001E
#define URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE     0x000B
#define URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT   0x0024
#define URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE  0x0028
#define URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE       0x000C
#define URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT     0x0025
#define URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE    0x0029
#define URB_FUNCTION_SET_FEATURE_TO_DEVICE          0x000D
#define URB_FUNCTION_SET_FEATURE_TO_INTERFACE       0x000E
#define URB_FUNCTION_SET_FEATURE_TO_ENDPOINT        0x000F
#define URB_FUNCTION_SET_FEATURE_TO_OTHER           0x0023
#define URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE        0x0010
#define URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE     0x0011
#define URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT      0x0012
#define URB_FUNCTION_CLEAR_FEATURE_TO_OTHER         0x0022
#define URB_FUNCTION_GET_STATUS_FROM_DEVICE         0x0013
#define URB_FUNCTION_GET_STATUS_FROM_INTERFACE      0x0014
#define URB_FUNCTION_GET_STATUS_FROM_ENDPOINT       0x0015
#define URB_FUNCTION_GET_STATUS_FROM_OTHER          0x0021
#define URB_FUNCTION_RESERVED0                      0x0016
#define URB_FUNCTION_VENDOR_DEVICE                  0x0017
#define URB_FUNCTION_VENDOR_INTERFACE               0x0018
#define URB_FUNCTION_VENDOR_ENDPOINT                0x0019
#define URB_FUNCTION_VENDOR_OTHER                   0x0020
#define URB_FUNCTION_CLASS_DEVICE                   0x001A
#define URB_FUNCTION_CLASS_INTERFACE                0x001B
#define URB_FUNCTION_CLASS_ENDPOINT                 0x001C
#define URB_FUNCTION_CLASS_OTHER                    0x001F
#define URB_FUNCTION_RESERVED                       0x001D
#define URB_FUNCTION_GET_CONFIGURATION              0x0026
#define URB_FUNCTION_GET_INTERFACE                  0x0027
#define URB_FUNCTION_LAST                           0x0029

typedef LONG USBD_STATUS;
typedef PVOID USBD_PIPE_HANDLE;
typedef PVOID USBD_CONFIGURATION_HANDLE;
typedef PVOID USBD_INTERFACE_HANDLE;

#define USBD_ERROR(Status) ((USBD_STATUS)(Status) < 0)
#define USBD_HALTED(Status) ((ULONG)(Status) >> 30 == 3)
#define USBD_PENDING(Status) ((ULONG)(Status) >> 30 == 1)
#define USBD_STATUS(Status) ((ULONG)(Status) & 0x0FFFFFFFL)
#define USBD_SUCCESS(Status) ((USBD_STATUS)(Status) >= 0)

#define USBD_STATUS_SUCCESS               ((USBD_STATUS)0x00000000L)
#define USBD_STATUS_PENDING               ((USBD_STATUS)0x40000000L)
#define USBD_STATUS_HALTED                ((USBD_STATUS)0xC0000000L)
#define USBD_STATUS_ERROR                 ((USBD_STATUS)0x80000000L)
#define USBD_STATUS_NO_MEMORY             ((USBD_STATUS)0x80000100L)
#define USBD_STATUS_INVALID_URB_FUNCTION  ((USBD_STATUS)0x80000200L)
#define USBD_STATUS_INVALID_PARAMETER     ((USBD_STATUS)0x80000300L)
#define USBD_STATUS_ERROR_BUSY            ((USBD_STATUS)0x80000400L)
#define USBD_STATUS_REQUEST_FAILED        ((USBD_STATUS)0x80000500L)
#define USBD_STATUS_INVALID_PIPE_HANDLE   ((USBD_STATUS)0x80000600L)
#define USBD_STATUS_NO_BANDWIDTH          ((USBD_STATUS)0x80000700L)
#define USBD_STATUS_INTERNAL_HC_ERROR     ((USBD_STATUS)0x80000800L)
#define USBD_STATUS_ERROR_SHORT_TRANSFER  ((USBD_STATUS)0x80000900L)
#define USBD_STATUS_CRC                   ((USBD_STATUS)0xC0000001L)
#define USBD_STATUS_BTSTUFF               ((USBD_STATUS)0xC0000002L)
#define USBD_STATUS_DATA_TOGGLE_MISMATCH  ((USBD_STATUS)0xC0000003L)
#define USBD_STATUS_STALL_PID             ((USBD_STATUS)0xC0000004L)
#define USBD_STATUS_DEV_NOT_RESPONDING    ((USBD_STATUS)0xC0000005L)
#define USBD_STATUS_PID_CHECK_FAILURE     ((USBD_STATUS)0xC0000006L)
#define USBD_STATUS_UNEXPECTED_PID        ((USBD_STATUS)0xC0000007L)
#define USBD_STATUS_DATA_OVERRUN          ((USBD_STATUS)0xC0000008L)
#define USBD_STATUS_DATA_UNDERRUN         ((USBD_STATUS)0xC0000009L)
#define USBD_STATUS_RESERVED1             ((USBD_STATUS)0xC000000AL)
#define USBD_STATUS_RESERVED2             ((USBD_STATUS)0xC000000BL)
#define USBD_STATUS_BUFFER_OVERRUN        ((USBD_STATUS)0xC000000CL)
#define USBD_STATUS_BUFFER_UNDERRUN       ((USBD_STATUS)0xC000000DL)
#define USBD_STATUS_NOT_ACCESSED          ((USBD_STATUS)0xC000000FL)
#define USBD_STATUS_FIFO                  ((USBD_STATUS)0xC0000010L)
#define USBD_STATUS_ENDPOINT_HALTED       ((USBD_STATUS)0xC0000030L)
#define USBD_STATUS_BAD_START_FRAME       ((USBD_STATUS)0xC0000A00L)
#define USBD_STATUS_ISOCH_REQUEST_FAILED  ((USBD_STATUS)0xC0000B00L)
#define USBD_STATUS_FRAME_CONTROL_OWNED   ((USBD_STATUS)0xC0000C00L)
#define USBD_STATUS_FRAME_CONTROL_NOT_OWNED \
                                          ((USBD_STATUS)0xC0000D00L)
#define USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR \
                                          ((USBD_STATUS)0xC0000F00L)
#define USBD_STATUS_CANCELING             ((USBD_STATUS)0x00020000L)

#define USBD_PIPE_DIRECTION_IN(pipeInformation) \
  ((pipeInformation)->EndpointAddress & USB_ENDPOINT_DIRECTION_MASK) 

struct _URB_HEADER { 
	USHORT  Length;
	USHORT  Function;
	USBD_STATUS  Status;
	PVOID  UsbdDeviceHandle;
	ULONG  UsbdFlags;
};

struct _URB_HCD_AREA {
	PVOID  HcdEndpoint;
	PIRP  HcdIrp;
	LIST_ENTRY  HcdListEntry;
	LIST_ENTRY  HcdListEntry2;
	PVOID  HcdCurrentIoFlushPointer;
	PVOID  HcdExtension;
};

struct _URB_BULK_OR_INTERRUPT_TRANSFER {
	struct _URB_HEADER  Hdr;
	USBD_PIPE_HANDLE  PipeHandle;
	ULONG  TransferFlags;
	ULONG  TransferBufferLength;
	PVOID  TransferBuffer;
	PMDL  TransferBufferMDL;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
};

struct _URB_CONTROL_DESCRIPTOR_REQUEST {
	struct _URB_HEADER  Hdr;
	PVOID  Reserved;
	ULONG  Reserved0;
	ULONG  TransferBufferLength;
	PVOID  TransferBuffer;
	PMDL  TransferBufferMDL;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
	USHORT  Reserved1;
	UCHAR  Index;
	UCHAR  DescriptorType;
	USHORT  LanguageId;
	USHORT  Reserved2;
};

struct _URB_CONTROL_FEATURE_REQUEST {
	struct _URB_HEADER  Hdr;
	PVOID  Reserved;
	ULONG  Reserved2;
	ULONG  Reserved3;
	PVOID  Reserved4;
	PMDL  Reserved5;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
	USHORT  Reserved0;
	USHORT  FeatureSelector;
	USHORT  Index;
	USHORT  Reserved1;
};

struct _URB_CONTROL_GET_CONFIGURATION_REQUEST {
	struct _URB_HEADER  Hdr;
	PVOID  Reserved;
	ULONG  Reserved0;
	ULONG  TransferBufferLength;
	PVOID  TransferBuffer;
	PMDL  TransferBufferMDL;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
	UCHAR  Reserved1[8];    
};

struct _URB_CONTROL_GET_INTERFACE_REQUEST {
	struct _URB_HEADER  Hdr;
	PVOID  Reserved;
	ULONG  Reserved0;
	ULONG  TransferBufferLength;
	PVOID  TransferBuffer;
	PMDL  TransferBufferMDL;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
	UCHAR  Reserved1[4];
	USHORT  Interface;
	USHORT  Reserved2;
};

struct _URB_CONTROL_GET_STATUS_REQUEST {
	struct _URB_HEADER  Hdr;
	PVOID  Reserved;
	ULONG  Reserved0;
	ULONG  TransferBufferLength;
	PVOID  TransferBuffer;
	PMDL  TransferBufferMDL;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
	UCHAR  Reserved1[4];
	USHORT  Index;
	USHORT  Reserved2;
};

struct _URB_CONTROL_TRANSFER {
	struct _URB_HEADER  Hdr;
	USBD_PIPE_HANDLE  PipeHandle;
	ULONG  TransferFlags;
	ULONG  TransferBufferLength;
	PVOID  TransferBuffer;
	PMDL  TransferBufferMDL;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
	UCHAR  SetupPacket[8];
};

struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST {
	struct _URB_HEADER  Hdr;
	PVOID  Reserved;
	ULONG  TransferFlags;
	ULONG  TransferBufferLength;
	PVOID  TransferBuffer;
	PMDL  TransferBufferMDL;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
	UCHAR  RequestTypeReservedBits;
	UCHAR  Request;
	USHORT  Value;
	USHORT  Index;
	USHORT  Reserved1;
};

struct _URB_FRAME_LENGTH_CONTROL {
	struct _URB_HEADER  Hdr;
};

struct _URB_GET_CURRENT_FRAME_NUMBER {
	struct _URB_HEADER  Hdr;
	ULONG  FrameNumber;
};

struct _URB_GET_FRAME_LENGTH {
	struct _URB_HEADER  Hdr;
	ULONG  FrameLength;
	ULONG  FrameNumber;
};

typedef struct _USBD_ISO_PACKET_DESCRIPTOR {
  ULONG  Offset;
  ULONG  Length;
  USBD_STATUS  Status;
} USBD_ISO_PACKET_DESCRIPTOR, *PUSBD_ISO_PACKET_DESCRIPTOR;

struct _URB_ISOCH_TRANSFER {
	struct _URB_HEADER  Hdr;
	USBD_PIPE_HANDLE  PipeHandle;
	ULONG  TransferFlags;
	ULONG  TransferBufferLength;
	PVOID  TransferBuffer;
	PMDL  TransferBufferMDL;
	struct _URB  *UrbLink;
	struct _URB_HCD_AREA  hca;
	ULONG  StartFrame;
	ULONG  NumberOfPackets;
	ULONG  ErrorCount;
	USBD_ISO_PACKET_DESCRIPTOR  IsoPacket[1]; 
};

struct _URB_PIPE_REQUEST {
	struct _URB_HEADER  Hdr;
	USBD_PIPE_HANDLE  PipeHandle;
	ULONG  Reserved;
};

struct _URB_SET_FRAME_LENGTH {
	struct _URB_HEADER  Hdr;
	LONG  FrameLengthDelta;
};

typedef struct _USBD_DEVICE_INFORMATION {
	ULONG  OffsetNext;
	PVOID  UsbdDeviceHandle;
	USB_DEVICE_DESCRIPTOR  DeviceDescriptor;
} USBD_DEVICE_INFORMATION, *PUSBD_DEVICE_INFORMATION;

typedef enum _USBD_PIPE_TYPE {
	UsbdPipeTypeControl,
	UsbdPipeTypeIsochronous,
	UsbdPipeTypeBulk,
	UsbdPipeTypeInterrupt
} USBD_PIPE_TYPE;

/* USBD_PIPE_INFORMATION.PipeFlags constants */
#define USBD_PF_CHANGE_MAX_PACKET         0x00000001
#define USBD_PF_DOUBLE_BUFFER             0x00000002 
#define USBD_PF_ENABLE_RT_THREAD_ACCESS   0x00000004 
#define USBD_PF_MAP_ADD_TRANSFERS         0x00000008

typedef struct _USBD_PIPE_INFORMATION {
  USHORT  MaximumPacketSize;
  UCHAR  EndpointAddress;
  UCHAR  Interval;
  USBD_PIPE_TYPE  PipeType;
  USBD_PIPE_HANDLE  PipeHandle;
  ULONG  MaximumTransferSize;
  ULONG  PipeFlags;
} USBD_PIPE_INFORMATION, *PUSBD_PIPE_INFORMATION;

typedef struct _USBD_INTERFACE_INFORMATION {
  USHORT  Length;
  UCHAR  InterfaceNumber;
  UCHAR  AlternateSetting;
  UCHAR  Class;
  UCHAR  SubClass;
  UCHAR  Protocol;
  UCHAR  Reserved;
  USBD_INTERFACE_HANDLE  InterfaceHandle;
  ULONG  NumberOfPipes; 
  USBD_PIPE_INFORMATION  Pipes[1];
} USBD_INTERFACE_INFORMATION, *PUSBD_INTERFACE_INFORMATION;

struct _URB_SELECT_CONFIGURATION {
	struct _URB_HEADER  Hdr;
	PUSB_CONFIGURATION_DESCRIPTOR  ConfigurationDescriptor;
	USBD_CONFIGURATION_HANDLE  ConfigurationHandle;
	USBD_INTERFACE_INFORMATION  Interface;
};

struct _URB_SELECT_INTERFACE {
	struct _URB_HEADER  Hdr;
	USBD_CONFIGURATION_HANDLE  ConfigurationHandle;
	USBD_INTERFACE_INFORMATION  Interface;
};

typedef struct _USBD_VERSION_INFORMATION {
	ULONG  USBDI_Version;
	ULONG  Supported_USB_Version;
} USBD_VERSION_INFORMATION, *PUSBD_VERSION_INFORMATION;

typedef struct _URB {
	union {
		struct _URB_HEADER  UrbHeader;
		struct _URB_SELECT_INTERFACE  UrbSelectInterface;
		struct _URB_SELECT_CONFIGURATION  UrbSelectConfiguration;
		struct _URB_PIPE_REQUEST  UrbPipeRequest;
		struct _URB_FRAME_LENGTH_CONTROL  UrbFrameLengthControl;
		struct _URB_GET_FRAME_LENGTH  UrbGetFrameLength;
		struct _URB_SET_FRAME_LENGTH  UrbSetFrameLength;
		struct _URB_GET_CURRENT_FRAME_NUMBER  UrbGetCurrentFrameNumber;
		struct _URB_CONTROL_TRANSFER  UrbControlTransfer;
		struct _URB_BULK_OR_INTERRUPT_TRANSFER  UrbBulkOrInterruptTransfer;
		struct _URB_ISOCH_TRANSFER  UrbIsochronousTransfer;
		struct _URB_CONTROL_DESCRIPTOR_REQUEST  UrbControlDescriptorRequest;
		struct _URB_CONTROL_GET_STATUS_REQUEST  UrbControlGetStatusRequest;
		struct _URB_CONTROL_FEATURE_REQUEST  UrbControlFeatureRequest;
		struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST  UrbControlVendorClassRequest;
		struct _URB_CONTROL_GET_INTERFACE_REQUEST  UrbControlGetInterfaceRequest;
		struct _URB_CONTROL_GET_CONFIGURATION_REQUEST  UrbControlGetConfigurationRequest;
	};
} URB, *PURB;

#ifdef __cplusplus
}
#endif

#endif /* defined __USB_H */

#endif /* __USBDI_H */
