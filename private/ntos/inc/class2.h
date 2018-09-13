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
#include "ntddscsi.h"
#include <stdio.h>

#if DBG

#define DebugPrint(x) ScsiDebugPrint x

#else

#define DebugPrint(x)

#endif // DBG

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'HscS')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'HscS')
#endif

#define MAXIMUM_RETRIES 4

struct _CLASS_INIT_DATA;

typedef
VOID
(*PCLASS_ERROR) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT NTSTATUS *Status,
    IN OUT BOOLEAN *Retry
    );

typedef
BOOLEAN
(*PCLASS_DEVICE_CALLBACK) (
    IN PINQUIRYDATA
    );

typedef
NTSTATUS
(*PCLASS_READ_WRITE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
BOOLEAN
(*PCLASS_FIND_DEVICES) (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN struct _CLASS_INIT_DATA *InitializationData,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber
    );

typedef
NTSTATUS
(*PCLASS_DEVICE_CONTROL) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PCLASS_SHUTDOWN_FLUSH) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PCLASS_CREATE_CLOSE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef struct _CLASS_INIT_DATA {

    //
    // This structure size - version checking.
    //

    ULONG InitializationDataSize;

    //
    // Bytes needed by the class driver
    // for it's extension.
    //

    ULONG DeviceExtensionSize;

    DEVICE_TYPE DeviceType;

    //
    // Device Characteristics flags
    //  eg.:
    //
    //  FILE_REMOVABLE_MEDIA
    //  FILE_READ_ONLY_DEVICE
    //  FILE_FLOPPY_DISKETTE
    //  FILE_WRITE_ONCE_MEDIA
    //  FILE_REMOTE_DEVICE
    //  FILE_DEVICE_IS_MOUNTED
    //  FILE_VIRTUAL_VOLUME
    //

    ULONG DeviceCharacteristics;

    //
    // Device-specific driver routines
    //

    PCLASS_ERROR           ClassError;
    PCLASS_READ_WRITE      ClassReadWriteVerification;
    PCLASS_DEVICE_CALLBACK ClassFindDeviceCallBack;
    PCLASS_FIND_DEVICES    ClassFindDevices;
    PCLASS_DEVICE_CONTROL  ClassDeviceControl;
    PCLASS_SHUTDOWN_FLUSH  ClassShutdownFlush;
    PCLASS_CREATE_CLOSE    ClassCreateClose;
    PDRIVER_STARTIO        ClassStartIo;

} CLASS_INIT_DATA, *PCLASS_INIT_DATA;

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
    // Class driver routines
    //

    PCLASS_ERROR ClassError;
    PCLASS_READ_WRITE ClassReadWriteVerification;
    PCLASS_FIND_DEVICES ClassFindDevices;
    PCLASS_DEVICE_CONTROL ClassDeviceControl;
    PCLASS_SHUTDOWN_FLUSH ClassShutdownFlush;
    PCLASS_CREATE_CLOSE ClassCreateClose;
    PDRIVER_STARTIO     ClassStartIo;

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
    // If this is equal to DeviceObject then this is the physical device
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
    // Lookaside listhead for srbs.
    //

    NPAGED_LOOKASIDE_LIST SrbLookasideListHead;

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
    UCHAR   ReservedByte;

    //
    // Values for the flags are below.
    //

    USHORT  DeviceFlags;

    //
    // Pointer to the media change event.  If MediaChange is TRUE, then
    // this event is to be signaled on all media change check conditions.
    // If MediaChangeNoMedia is true then we've already determined that there
    // isn't any media in the drive so we shouldn't signal the event again
    //

    PKEVENT MediaChangeEvent;
    HANDLE  MediaChangeEventHandle;
    BOOLEAN MediaChangeNoMedia;

    //
    // Count of media changes.  This field is only valid for the root partition
    // (ie. if PhysicalDevice == NULL).
    //

    ULONG MediaChangeCount;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Indicates that the device has write caching enabled.
//

#define DEV_WRITE_CACHE     0x00000001


//
// Build SCSI 1 or SCSI 2 CDBs
//

#define DEV_USE_SCSI1       0x00000002

//
// Indicates whether is is safe to send StartUnit commands
// to this device. It will only be off for some removeable devices.
//

#define DEV_SAFE_START_UNIT 0x00000004

//
// Indicates whether it is unsafe to send SCSIOP_MECHANISM_STATUS commands to
// this device.  Some devices don't like these 12 byte commands
//

#define DEV_NO_12BYTE_CDB 0x00000008

//
// Define context structure for asynchronous completions.
//

typedef struct _COMPLETION_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    SCSI_REQUEST_BLOCK Srb;
}COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

#ifndef _NTDDK_
#define SCSIPORT_API DECLSPEC_IMPORT
#else
#define SCSIPORT_API
#endif

//
// Class dll routines called by class drivers
//

SCSIPORT_API
ULONG
ScsiClassInitialize(
    IN  PVOID            Argument1,
    IN  PVOID            Argument2,
    IN  PCLASS_INIT_DATA InitializationData
    );

SCSIPORT_API
NTSTATUS
ScsiClassCreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PCCHAR ObjectNameBuffer,
    IN OPTIONAL PDEVICE_OBJECT PhysicalDeviceObject,
    IN OUT PDEVICE_OBJECT *DeviceObject,
    IN PCLASS_INIT_DATA InitializationData
    );

SCSIPORT_API
ULONG
ScsiClassFindUnclaimedDevices(
    IN PCLASS_INIT_DATA InitializationData,
    IN PSCSI_ADAPTER_BUS_INFO  AdapterInformation
    );

SCSIPORT_API
NTSTATUS
ScsiClassGetCapabilities(
    IN PDEVICE_OBJECT PortDeviceObject,
    OUT PIO_SCSI_CAPABILITIES *PortCapabilities
    );

SCSIPORT_API
NTSTATUS
ScsiClassGetInquiryData(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PSCSI_ADAPTER_BUS_INFO *ConfigInfo
    );

SCSIPORT_API
NTSTATUS
ScsiClassReadDriveCapacity(
    IN PDEVICE_OBJECT DeviceObject
    );

SCSIPORT_API
VOID
ScsiClassReleaseQueue(
    IN PDEVICE_OBJECT DeviceObject
    );

SCSIPORT_API
NTSTATUS
ScsiClassRemoveDevice(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    );

SCSIPORT_API
NTSTATUS
ScsiClassAsynchronousCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

SCSIPORT_API
VOID
ScsiClassSplitRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG MaximumBytes
    );

SCSIPORT_API
NTSTATUS
ScsiClassDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

SCSIPORT_API
NTSTATUS
ScsiClassIoComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

SCSIPORT_API
NTSTATUS
ScsiClassCheckVerifyComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

SCSIPORT_API
NTSTATUS
ScsiClassIoCompleteAssociated(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

SCSIPORT_API
BOOLEAN
ScsiClassInterpretSenseInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN UCHAR MajorFunctionCode,
    IN ULONG IoDeviceCode,
    IN ULONG RetryCount,
    OUT NTSTATUS *Status
    );

SCSIPORT_API
NTSTATUS
ScsiClassSendSrbSynchronous(
        PDEVICE_OBJECT DeviceObject,
        PSCSI_REQUEST_BLOCK Srb,
        PVOID BufferAddress,
        ULONG BufferLength,
        BOOLEAN WriteToDevice
        );

SCSIPORT_API
NTSTATUS
ScsiClassSendSrbAsynchronous(
        PDEVICE_OBJECT DeviceObject,
        PSCSI_REQUEST_BLOCK Srb,
        PIRP Irp,
        PVOID BufferAddress,
        ULONG BufferLength,
        BOOLEAN WriteToDevice
        );

SCSIPORT_API
VOID
ScsiClassBuildRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

SCSIPORT_API
ULONG
ScsiClassModeSense(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
    );

SCSIPORT_API
BOOLEAN
ScsiClassModeSelect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCHAR ModeSelectBuffer,
    IN ULONG Length,
    IN BOOLEAN SavePage
    );

SCSIPORT_API
PVOID
ScsiClassFindModePage(
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode,
    IN BOOLEAN Use6Byte
    );

SCSIPORT_API
NTSTATUS
ScsiClassClaimDevice(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PSCSI_INQUIRY_DATA LunInfo,
    IN BOOLEAN Release,
    OUT PDEVICE_OBJECT *NewPortDeviceObject OPTIONAL
    );

SCSIPORT_API
NTSTATUS
ScsiClassInternalIoControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

SCSIPORT_API
VOID
ScsiClassInitializeSrbLookasideList(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG NumberElements
    );

SCSIPORT_API
ULONG
ScsiClassQueryTimeOutRegistryValue(
    IN PUNICODE_STRING RegistryPath
    );

SCSIPORT_API
BOOLEAN
ScsiClassIsFloppyDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_INQUIRY_DATA LunInfo
    );

#endif /* _CLASS_ */
