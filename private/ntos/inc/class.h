/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    class.h

Abstract:

    These are the structures and defines that are used in the
    SCSI class drivers.

Author:

    Mike Glass (mglass)
    Jeff Havens (jhavens)

Revision History:

--*/

#ifndef _CLASS_

#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <ntddtape.h>
#include <ntddchgr.h>
#include <ntddstor.h>
#include "ntddscsi.h"
#include <stdio.h>

// begin_ntminitape

#if DBG

#define DebugPrint(x) ScsiDebugPrint x

#else

#define DebugPrint(x)

#endif // DBG

// end_ntminitape

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'HscS')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'HscS')
#endif

#define MAXIMUM_RETRIES 4

typedef
VOID
(*PCLASS_ERROR) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT NTSTATUS *Status,
    IN OUT BOOLEAN *Retry
    );

typedef struct _DEVICE_EXTENSION {

    //
    // Back pointer to device object
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Pointer to port device object
    //

    PDEVICE_OBJECT PortDeviceObject;

    //
    // Length of partition in bytes
    //

    LARGE_INTEGER PartitionLength;

    //
    // Number of bytes before start of partition
    //

    LARGE_INTEGER StartingOffset;

    //
    // Bytes to skew all requests, since DM Driver has been placed on an IDE drive.
    //

    ULONG DMByteSkew;

    //
    // Sectors to skew all requests.
    //

    ULONG DMSkew;

    //
    // Flag to indicate whether DM driver has been located on an IDE drive.
    //

    BOOLEAN DMActive;

    //
    // Pointer to the specific class error routine.
    //

    PCLASS_ERROR ClassError;

    //
    // SCSI port driver capabilities
    //

    PIO_SCSI_CAPABILITIES PortCapabilities;

    //
    // Buffer for drive parameters returned in IO device control.
    //

    PDISK_GEOMETRY DiskGeometry;

    //
    // Back pointer to device object of physical device
    //

    PDEVICE_OBJECT PhysicalDevice;

    //
    // Request Sense Buffer
    //

    PSENSE_DATA SenseData;

    //
    // Request timeout in seconds;
    //

    ULONG TimeOutValue;

    //
    // System device number
    //

    ULONG DeviceNumber;

    //
    // Add default Srb Flags.
    //

    ULONG SrbFlags;

    //
    // Total number of SCSI protocol errors on the device.
    //

    ULONG ErrorCount;

    //
    // Spinlock for split requests
    //

    KSPIN_LOCK SplitRequestSpinLock;

    //
    // Zone header and spin lock for zoned SRB requests.
    //

    PZONE_HEADER SrbZone;

    PKSPIN_LOCK SrbZoneSpinLock;

    //
    // Lock count for removable media.
    //

    LONG LockCount;

    //
    // Scsi port number
    //

    UCHAR PortNumber;

    //
    // SCSI path id
    //

    UCHAR PathId;

    //
    // SCSI bus target id
    //

    UCHAR TargetId;

    //
    // SCSI bus logical unit number
    //

    UCHAR Lun;

    //
    // Log2 of sector size
    //

    UCHAR SectorShift;

    //
    // Flag to indicate that the device has write caching enabled.
    //

    BOOLEAN WriteCache;

    //
    // Build SCSI 1 or SCSI 2 CDBs
    //

    BOOLEAN UseScsi1;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Define context structure for asynchronous completions.
//

typedef struct _COMPLETION_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    SCSI_REQUEST_BLOCK Srb;
}COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;


NTSTATUS
ScsiClassGetCapabilities(
    IN PDEVICE_OBJECT PortDeviceObject,
    OUT PIO_SCSI_CAPABILITIES *PortCapabilities
    );

NTSTATUS
ScsiClassGetInquiryData(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PSCSI_ADAPTER_BUS_INFO *ConfigInfo
    );

NTSTATUS
ScsiClassReadDriveCapacity(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
ScsiClassReleaseQueue(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ScsiClassRemoveDevice(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    );

NTSTATUS
ScsiClassAsynchronousCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

VOID
ScsiClassSplitRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG MaximumBytes
    );

NTSTATUS
ScsiClassDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
ScsiClassIoComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
ScsiClassIoCompleteAssociated(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN
ScsiClassInterpretSenseInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN UCHAR MajorFunctionCode,
    IN ULONG IoDeviceCode,
    IN ULONG RetryCount,
    OUT NTSTATUS *Status
    );

NTSTATUS
ScsiClassSendSrbSynchronous(
        PDEVICE_OBJECT DeviceObject,
        PSCSI_REQUEST_BLOCK Srb,
        PVOID BufferAddress,
        ULONG BufferLength,
        BOOLEAN WriteToDevice
        );

NTSTATUS
ScsiClassSendSrbAsynchronous(
        PDEVICE_OBJECT DeviceObject,
        PSCSI_REQUEST_BLOCK Srb,
        PIRP Irp,
        PVOID BufferAddress,
        ULONG BufferLength,
        BOOLEAN WriteToDevice
        );

VOID
ScsiClassBuildRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

ULONG
ScsiClassModeSense(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
    );

BOOLEAN
ScsiClassModeSelect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCHAR ModeSelectBuffer,
    IN ULONG Length,
    IN BOOLEAN SavePage
    );

PVOID
ScsiClassFindModePage(
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
    );

NTSTATUS
ScsiClassClaimDevice(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PSCSI_INQUIRY_DATA LunInfo,
    IN BOOLEAN Release,
    OUT PDEVICE_OBJECT *NewPortDeviceObject OPTIONAL
    );

NTSTATUS
ScsiClassInternalIoControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif /* _CLASS_ */
