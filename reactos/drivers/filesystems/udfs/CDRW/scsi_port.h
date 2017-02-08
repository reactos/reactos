////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*

Module Name:

    scsi_port.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the SCSI port adapters.

Environment:

    kernel mode only
 */

#ifndef __SCSI_PORT_H__
#define __SCSI_PORT_H__

#ifndef WIN64

#include "srb.h"

#ifdef SCSIPORT_API
#undef SCSIPORT_API
#endif

// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.

#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER

#define DD_SCSI_DEVICE_NAME "\\Device\\ScsiPort"


// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!

#define IOCTL_SCSI_PASS_THROUGH         CTL_CODE(IOCTL_SCSI_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_MINIPORT             CTL_CODE(IOCTL_SCSI_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_GET_INQUIRY_DATA     CTL_CODE(IOCTL_SCSI_BASE, 0x0403, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_GET_CAPABILITIES     CTL_CODE(IOCTL_SCSI_BASE, 0x0404, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH_DIRECT  CTL_CODE(IOCTL_SCSI_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_GET_ADDRESS          CTL_CODE(IOCTL_SCSI_BASE, 0x0406, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_RESCAN_BUS           CTL_CODE(IOCTL_SCSI_BASE, 0x0407, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_GET_DUMP_POINTERS    CTL_CODE(IOCTL_SCSI_BASE, 0x0408, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Define the SCSI pass through structure.

typedef struct _SCSI_PASS_THROUGH {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;

// Define the SCSI pass through direct structure.

typedef struct _SCSI_PASS_THROUGH_DIRECT {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;

// Define SCSI information.
// Used with the IOCTL_SCSI_GET_INQUIRY_DATA IOCTL.

typedef struct _SCSI_BUS_DATA {
    UCHAR NumberOfLogicalUnits;
    UCHAR InitiatorBusId;
    ULONG InquiryDataOffset;
}SCSI_BUS_DATA, *PSCSI_BUS_DATA;

// Define SCSI adapter bus information structure..
// Used with the IOCTL_SCSI_GET_INQUIRY_DATA IOCTL.

typedef struct _SCSI_ADAPTER_BUS_INFO {
    UCHAR NumberOfBuses;
    SCSI_BUS_DATA BusData[1];
} SCSI_ADAPTER_BUS_INFO, *PSCSI_ADAPTER_BUS_INFO;

// Define SCSI adapter bus information.
// Used with the IOCTL_SCSI_GET_INQUIRY_DATA IOCTL.

typedef struct _SCSI_INQUIRY_DATA {
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    BOOLEAN DeviceClaimed;
    ULONG InquiryDataLength;
    ULONG NextInquiryDataOffset;
    UCHAR InquiryData[1];
}SCSI_INQUIRY_DATA, *PSCSI_INQUIRY_DATA;

// Define header for I/O control SRB.

typedef struct _SRB_IO_CONTROL {
        ULONG HeaderLength;
        UCHAR Signature[8];
        ULONG Timeout;
        ULONG ControlCode;
        ULONG ReturnCode;
        ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

// SCSI port driver capabilities structure.

typedef struct _IO_SCSI_CAPABILITIES {
    ULONG Length;    // Length of this structure
    ULONG MaximumTransferLength;    // Maximum transfer size in single SRB (obsolete)
    ULONG MaximumPhysicalPages;    // Maximum number of physical pages per data buffer
    ULONG SupportedAsynchronousEvents;    // Async calls from port to class
    ULONG AlignmentMask;    // Alignment mask for data transfers.
    BOOLEAN TaggedQueuing;    // Supports tagged queuing
    BOOLEAN AdapterScansDown;    // Host adapter scans down for bios devices.
    BOOLEAN AdapterUsesPio;    // The host adapter uses programmed I/O.
} IO_SCSI_CAPABILITIES, *PIO_SCSI_CAPABILITIES;

typedef struct _SCSI_ADDRESS {
    ULONG Length;
    UCHAR PortNumber;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
}SCSI_ADDRESS, *PSCSI_ADDRESS;

// Define structure for returning crash dump pointers.

struct _ADAPTER_OBJECT;

typedef struct _DUMP_POINTERS {
    struct _ADAPTER_OBJECT *AdapterObject;
    PVOID MappedRegisterBase;
    PVOID PortConfiguration;
    PVOID CommonBufferVa;
    LARGE_INTEGER CommonBufferPa;
    ULONG CommonBufferSize;
} DUMP_POINTERS, *PDUMP_POINTERS;

#endif //WIN64


// Define values for pass-through DataIn field.

#define SCSI_IOCTL_DATA_OUT          0
#define SCSI_IOCTL_DATA_IN           1
#define SCSI_IOCTL_DATA_UNSPECIFIED  2

#define CLASS_SPECIAL_CAUSE_NOT_REPORTABLE_HACK     0x00000020

#define SCSIPORT_API

#ifndef USER_MODE
extern NTSTATUS ScsiClassGetInquiryData(IN PDEVICE_OBJECT PortDeviceObject,
                                        IN PSCSI_ADAPTER_BUS_INFO *ConfigInfo);
extern NTSTATUS ScsiClassReadDeviceInquiryData(IN PDEVICE_OBJECT DeviceObject,
                                               IN BOOLEAN DirectAccess);
extern NTSTATUS ScsiClassReadDriveCapacity(IN PDEVICE_OBJECT DeviceObject);

#ifndef CDRW_W32

// Class dll routines called by class drivers

extern NTSTATUS ScsiClassGetCapabilities(IN PDEVICE_OBJECT PortDeviceObject,
                                         OUT PIO_SCSI_CAPABILITIES *PortCapabilities);
extern NTSTATUS ScsiClassGetAddress(IN PDEVICE_OBJECT ClassDeviceObject,
                                    OUT PSCSI_ADDRESS Address);
extern VOID     ScsiClassReleaseQueue(IN PDEVICE_OBJECT DeviceObject);
extern NTSTATUS ScsiClassRemoveDevice(IN PDEVICE_OBJECT PortDeviceObject, IN UCHAR PathId,
                                      IN UCHAR TargetId, IN UCHAR Lun);
extern NTSTATUS ScsiClassClaimDevice(IN PDEVICE_OBJECT PortDeviceObject,
                                     IN PSCSI_INQUIRY_DATA LunInfo, IN BOOLEAN Release,
                                     OUT PDEVICE_OBJECT *NewPortDeviceObject OPTIONAL);
extern NTSTATUS ScsiClassInternalIoControl (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
extern NTSTATUS ScsiClassIoCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                                      IN PVOID Context);
extern NTSTATUS ScsiClassSendSrbSynchronous(IN PDEVICE_OBJECT TargetDeviceObject,
                                            IN PSCSI_REQUEST_BLOCK _srb,
                                            IN PKEVENT event);

#endif //CDRW_W32

extern VOID     ScsiClassInitSrbBusAddr(PSCSI_REQUEST_BLOCK Srb,
                                        PVOID DeviceExtension);

extern NTSTATUS
DbgWaitForSingleObject_(
    IN PVOID Object,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );
#endif //USER_MODE

#endif //__SCSI_PORT_H__
