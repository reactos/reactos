/*
 * usb.h
 *
 * USB support.
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

#ifndef __USB_H
#define __USB_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __USBDI_H
#error usbdi.h cannot be included with usb.h
#else

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"
#include "usb100.h"

typedef LONG USBD_STATUS;
typedef PVOID USBD_PIPE_HANDLE;
typedef PVOID USBD_CONFIGURATION_HANDLE;
typedef PVOID USBD_INTERFACE_HANDLE;

#define USBD_STATUS_SUCCESS               ((USBD_STATUS)0x00000000L)
#define USBD_STATUS_PENDING               ((USBD_STATUS)0x40000000L)
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
#define USBD_STATUS_XACT_ERROR            ((USBD_STATUS)0xC0000011L)
#define USBD_STATUS_BABBLE_DETECTED       ((USBD_STATUS)0xC0000012L)
#define USBD_STATUS_DATA_BUFFER_ERROR     ((USBD_STATUS)0xC0000013L)
#define USBD_STATUS_ENDPOINT_HALTED       ((USBD_STATUS)0xC0000030L)
#define USBD_STATUS_INVALID_URB_FUNCTION  ((USBD_STATUS)0x80000200L)
#define USBD_STATUS_INVALID_PARAMETER     ((USBD_STATUS)0x80000300L)
#define USBD_STATUS_ERROR_BUSY            ((USBD_STATUS)0x80000400L)
#define USBD_STATUS_INVALID_PIPE_HANDLE   ((USBD_STATUS)0x80000600L)
#define USBD_STATUS_NO_BANDWIDTH          ((USBD_STATUS)0x80000700L)
#define USBD_STATUS_INTERNAL_HC_ERROR     ((USBD_STATUS)0x80000800L)
#define USBD_STATUS_ERROR_SHORT_TRANSFER  ((USBD_STATUS)0x80000900L)
#define USBD_STATUS_BAD_START_FRAME       ((USBD_STATUS)0xC0000A00L)
#define USBD_STATUS_ISOCH_REQUEST_FAILED  ((USBD_STATUS)0xC0000B00L)
#define USBD_STATUS_FRAME_CONTROL_OWNED   ((USBD_STATUS)0xC0000C00L)
#define USBD_STATUS_FRAME_CONTROL_NOT_OWNED \
                                          ((USBD_STATUS)0xC0000D00L)
#define USBD_STATUS_NOT_SUPPORTED         ((USBD_STATUS)0xC0000E00L)
#define USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR \
                                          ((USBD_STATUS)0xC0000F00L)
#define USBD_STATUS_INSUFFICIENT_RESOURCES \
                                          ((USBD_STATUS)0xC0001000L)                                          
#define USBD_STATUS_SET_CONFIG_FAILED     ((USBD_STATUS)0xC0002000L)
#define USBD_STATUS_BUFFER_TOO_SMALL      ((USBD_STATUS)0xC0003000L)
#define USBD_STATUS_INTERFACE_NOT_FOUND   ((USBD_STATUS)0xC0004000L)
#define USBD_STATUS_INAVLID_PIPE_FLAGS    ((USBD_STATUS)0xC0005000L)
#define USBD_STATUS_TIMEOUT               ((USBD_STATUS)0xC0006000L)
#define USBD_STATUS_DEVICE_GONE           ((USBD_STATUS)0xC0007000L)
#define USBD_STATUS_STATUS_NOT_MAPPED     ((USBD_STATUS)0xC0008000L)
#define USBD_STATUS_CANCELED              ((USBD_STATUS)0xC0010000L)
#define USBD_STATUS_ISO_NOT_ACCESSED_BY_HW \
                                          ((USBD_STATUS)0xC0020000L)   
#define USBD_STATUS_ISO_TD_ERROR          ((USBD_STATUS)0xC0030000L)   
#define USBD_STATUS_ISO_NA_LATE_USBPORT   ((USBD_STATUS)0xC0040000L) 
#define USBD_STATUS_ISO_NOT_ACCESSED_LATE ((USBD_STATUS)0xC0050000L)

#define USBD_SUCCESS(Status) ((USBD_STATUS)(Status) >= 0)
#define USBD_PENDING(Status) ((ULONG)(Status) >> 30 == 1)
#define USBD_ERROR(Status) ((USBD_STATUS)(Status) < 0)

/* URB TransferFlags constants */
#define USBD_TRANSFER_DIRECTION(x)        ((x) & USBD_TRANSFER_DIRECTION_IN)
#define USBD_TRANSFER_DIRECTION_OUT       0   
#define USBD_TRANSFER_DIRECTION_BIT       0
#define USBD_TRANSFER_DIRECTION_IN        (1 << USBD_TRANSFER_DIRECTION_BIT)
#define USBD_SHORT_TRANSFER_OK_BIT        1
#define USBD_SHORT_TRANSFER_OK            (1 << USBD_SHORT_TRANSFER_OK_BIT)
#define USBD_START_ISO_TRANSFER_ASAP_BIT  2
#define USBD_START_ISO_TRANSFER_ASAP      (1 << USBD_START_ISO_TRANSFER_ASAP_BIT)
#define USBD_DEFAULT_PIPE_TRANSFER_BIT    3
#define USBD_DEFAULT_PIPE_TRANSFER        (1 << USBD_DEFAULT_PIPE_TRANSFER_BIT)

#define USBD_TRANSFER_DIRECTION_FLAG(flags) ((flags) & USBD_TRANSFER_DIRECTION)

#define VALID_TRANSFER_FLAGS_MASK \
  (USBD_SHORT_TRANSFER_OK | \
  USBD_TRANSFER_DIRECTION | \
  USBD_START_ISO_TRANSFER_ASAP | \
  USBD_DEFAULT_PIPE_TRANSFER)

#define USB_DEFAULT_DEVICE_ADDRESS        0
#define USB_DEFAULT_ENDPOINT_ADDRESS      0
#define USB_DEFAULT_MAX_PACKET            64
#define USBD_ISO_START_FRAME_RANGE        1024
#define USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE  0xFFFFFFFF

typedef enum _USB_CONTROLLER_FLAVOR {
	USB_HcGeneric = 0,
	OHCI_Generic = 100,
	OHCI_Hydra,
	OHCI_NEC,
	UHCI_Generic = 200,
	UHCI_Piix4,
	UHCI_Piix3,
	UHCI_Ich2_1,
	UHCI_Ich2_2,
	UHCI_Ich1,
	UHCI_VIA = 250,
	EHCI_Generic = 1000,
	EHCI_NEC = 2000,
	EHCI_Lucent = 3000
} USB_CONTROLLER_FLAVOR;

struct _URB_HEADER { 
	USHORT  Length;
	USHORT  Function;
	USBD_STATUS  Status;
	PVOID  UsbdDeviceHandle;
	ULONG  UsbdFlags;
};

typedef struct _USBD_VERSION_INFORMATION {
	ULONG  USBDI_Version;
	ULONG  Supported_USB_Version;
} USBD_VERSION_INFORMATION, *PUSBD_VERSION_INFORMATION;

struct _URB_HCD_AREA {
  PVOID  Reserved8[8];
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
  struct _URB *UrbLink;
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
  USHORT Reserved1;
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
  PMDL TransferBufferMDL;
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

typedef enum _USBD_PIPE_TYPE {
	UsbdPipeTypeControl,
	UsbdPipeTypeIsochronous,
	UsbdPipeTypeBulk,
	UsbdPipeTypeInterrupt
} USBD_PIPE_TYPE;

typedef struct _USBD_PIPE_INFORMATION {
  USHORT  MaximumPacketSize;
  UCHAR  EndpointAddress;
  UCHAR  Interval;
  USBD_PIPE_TYPE  PipeType;
  USBD_PIPE_HANDLE  PipeHandle;
  ULONG  MaximumTransferSize;
  ULONG  PipeFlags;
} USBD_PIPE_INFORMATION, *PUSBD_PIPE_INFORMATION ;

#define USBD_PIPE_DIRECTION_IN(pipeInformation) \
  ((pipeInformation)->EndpointAddress & USB_ENDPOINT_DIRECTION_MASK) 

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
  USBD_PIPE_INFORMATION Pipes[1];
} USBD_INTERFACE_INFORMATION, *PUSBD_INTERFACE_INFORMATION;

struct _URB_SELECT_INTERFACE { 
  struct _URB_HEADER  Hdr;
  USBD_CONFIGURATION_HANDLE  ConfigurationHandle;
  USBD_INTERFACE_INFORMATION  Interface;
};

struct _URB_SELECT_CONFIGURATION { 
  struct _URB_HEADER  Hdr;
  PUSB_CONFIGURATION_DESCRIPTOR  ConfigurationDescriptor;
  USBD_CONFIGURATION_HANDLE  ConfigurationHandle;
  USBD_INTERFACE_INFORMATION  Interface;
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

typedef struct _URB { 
  _ANONYMOUS_UNION union {
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
  } DUMMYUNIONNAME;
} URB, *PURB;

#define URB_FROM_IRP(Irp) ((IoGetCurrentIrpStackLocation(Irp))->Parameters.Others.Argument1)

#define URB_FUNCTION_SELECT_CONFIGURATION            0x0000
#define URB_FUNCTION_SELECT_INTERFACE                0x0001
#define URB_FUNCTION_ABORT_PIPE                      0x0002
#define URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL       0x0003
#define URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL    0x0004
#define URB_FUNCTION_GET_FRAME_LENGTH                0x0005
#define URB_FUNCTION_SET_FRAME_LENGTH                0x0006
#define URB_FUNCTION_GET_CURRENT_FRAME_NUMBER        0x0007
#define URB_FUNCTION_CONTROL_TRANSFER                0x0008
#define URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER      0x0009
#define URB_FUNCTION_ISOCH_TRANSFER                  0x000A
#define URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE      0x000B
#define URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE        0x000C
#define URB_FUNCTION_SET_FEATURE_TO_DEVICE           0x000D
#define URB_FUNCTION_SET_FEATURE_TO_INTERFACE        0x000E
#define URB_FUNCTION_SET_FEATURE_TO_ENDPOINT         0x000F
#define URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE         0x0010
#define URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE      0x0011
#define URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT       0x0012
#define URB_FUNCTION_GET_STATUS_FROM_DEVICE          0x0013
#define URB_FUNCTION_GET_STATUS_FROM_INTERFACE       0x0014
#define URB_FUNCTION_GET_STATUS_FROM_ENDPOINT        0x0015
#define URB_FUNCTION_RESERVED_0X0016                 0x0016
#define URB_FUNCTION_VENDOR_DEVICE                   0x0017
#define URB_FUNCTION_VENDOR_INTERFACE                0x0018
#define URB_FUNCTION_VENDOR_ENDPOINT                 0x0019
#define URB_FUNCTION_CLASS_DEVICE                    0x001A
#define URB_FUNCTION_CLASS_INTERFACE                 0x001B
#define URB_FUNCTION_CLASS_ENDPOINT                  0x001C
#define URB_FUNCTION_RESERVE_0X001D                  0x001D
#define URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL 0x001E
#define URB_FUNCTION_CLASS_OTHER                     0x001F
#define URB_FUNCTION_VENDOR_OTHER                    0x0020
#define URB_FUNCTION_GET_STATUS_FROM_OTHER           0x0021
#define URB_FUNCTION_CLEAR_FEATURE_TO_OTHER          0x0022
#define URB_FUNCTION_SET_FEATURE_TO_OTHER            0x0023
#define URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT    0x0024
#define URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT      0x0025
#define URB_FUNCTION_GET_CONFIGURATION               0x0026
#define URB_FUNCTION_GET_INTERFACE                   0x0027
#define URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE   0x0028
#define URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE     0x0029
#define URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR       0x002A
#define URB_FUNCTION_RESERVE_0X002B                  0x002B
#define URB_FUNCTION_RESERVE_0X002C                  0x002C
#define URB_FUNCTION_RESERVE_0X002D                  0x002D
#define URB_FUNCTION_RESERVE_0X002E                  0x002E
#define URB_FUNCTION_RESERVE_0X002F                  0x002F
#define URB_FUNCTION_SYNC_RESET_PIPE                 0x0030
#define URB_FUNCTION_SYNC_CLEAR_STALL                0x0031

#define USBD_PF_CHANGE_MAX_PACKET         0x00000001
#define USBD_PF_SHORT_PACKET_OPT          0x00000002 
#define USBD_PF_ENABLE_RT_THREAD_ACCESS   0x00000004 
#define USBD_PF_MAP_ADD_TRANSFERS         0x00000008 

#define USBD_PF_VALID_MASK (USBD_PF_CHANGE_MAX_PACKET | \
                            USBD_PF_SHORT_PACKET_OPT | \
                            USBD_PF_ENABLE_RT_THREAD_ACCESS | \
                            USBD_PF_MAP_ADD_TRANSFERS)

#define OS_STRING_DESCRIPTOR_INDEX        0xEE

#define MS_GENRE_DESCRIPTOR_INDEX         0x0001
#define MS_POWER_DESCRIPTOR_INDEX         0x0002

#define MS_OS_STRING_SIGNATURE            L"MSFT100"

typedef struct _OS_STRING {
	UCHAR  bLength;
	UCHAR  bDescriptorType;
	WCHAR  MicrosoftString[7];
	UCHAR  bVendorCode;
	UCHAR  bPad;
} OS_STRING, *POS_STRING;

#ifdef __cplusplus
}
#endif

#endif /* defined __USBDI_H */
 
#endif /* __USB_H */
