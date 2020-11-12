/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    floppy.c

Abstract:

    SCSI floppy class driver

Author:

    Jeff Havens (jhavens)

Environment:

    kernel mode only

Notes:

Revision History:
02/28/96    georgioc    Merged this code with code developed by compaq in
                        parallel with microsoft, for 120MB floppy support.

01/17/96    georgioc    Made code PNP aware (uses the new \storage\classpnp/scsiport)

--*/

#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(disable:4214) // nonstandard extension used : bit field types other than int
#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#endif

#include <stddef.h>
#include <ntddk.h>
#ifndef __REACTOS__
#include <winerror.h>
#endif
#include <scsi.h>
#include <classpnp.h>
#include <initguid.h>
#include <ntddstor.h>

#include <ntstrsafe.h>
#include <intsafe.h>

#ifdef __REACTOS__
// Downgrade unsupported NT6.2+ features.
#define NonPagedPoolNx NonPagedPool
#define NonPagedPoolNxCacheAligned NonPagedPoolCacheAligned
#endif

#define MODE_DATA_SIZE      192
#define SCSI_FLOPPY_TIMEOUT  20
#define SFLOPPY_SRB_LIST_SIZE 4
//
// Define all possible drive/media combinations, given drives listed above
// and media types in ntdddisk.h.
//
// These values are used to index the DriveMediaConstants table.
//

#define NUMBER_OF_DRIVE_TYPES              7
#define DRIVE_TYPE_120M                    4    //120MB Floptical
#define DRIVE_TYPE_NONE                    NUMBER_OF_DRIVE_TYPES

//
// This array describes all media types we support.
// It should be arranged in the increasing order of density
//
// For a given drive, we list all the mediatypes that will
// work with that drive. For instance, a 120MB drive will
// take 720KB media, 1.44MB media, and 120MB media.
//
// Note that, DriveMediaConstants given below is grouped
// as drive and media combination
//
typedef enum _DRIVE_MEDIA_TYPE {
    Drive360Media160,                      // 5.25"  360k  drive;  160k   media
    Drive360Media180,                      // 5.25"  360k  drive;  180k   media
    Drive360Media320,                      // 5.25"  360k  drive;  320k   media
    Drive360Media32X,                      // 5.25"  360k  drive;  320k 1k secs
    Drive360Media360,                      // 5.25"  360k  drive;  360k   media
    Drive720Media720,                      // 3.5"   720k  drive;  720k   media
    Drive120Media160,                      // 5.25" 1.2Mb  drive;  160k   media
    Drive120Media180,                      // 5.25" 1.2Mb  drive;  180k   media
    Drive120Media320,                      // 5.25" 1.2Mb  drive;  320k   media
    Drive120Media32X,                      // 5.25" 1.2Mb  drive;  320k 1k secs
    Drive120Media360,                      // 5.25" 1.2Mb  drive;  360k   media
    Drive120Media120,                      // 5.25" 1.2Mb  drive; 1.2Mb   media
    Drive144Media720,                      // 3.5"  1.44Mb drive;  720k   media
    Drive144Media144,                      // 3.5"  1.44Mb drive; 1.44Mb  media
    Drive288Media720,                      // 3.5"  2.88Mb drive;  720k   media
    Drive288Media144,                      // 3.5"  2.88Mb drive; 1.44Mb  media
    Drive288Media288,                      // 3.5"  2.88Mb drive; 2.88Mb  media
    Drive2080Media720,                     // 3.5"  20.8Mb drive;  720k   media
    Drive2080Media144,                     // 3.5"  20.8Mb drive; 1.44Mb  media
    Drive2080Media2080,                    // 3.5"  20.8Mb drive; 20.8Mb  media
    Drive32MMedia32M,                      // 3.5"  32Mb drive; 32MB    media
    Drive120MMedia720,                     // 3.5"  120Mb drive; 720k  media
    Drive120MMedia144,                     // 3.5"  120Mb drive; 1.44Mb  media
    Drive120MMedia120M,                    // 3.5"  120Mb drive; 120Mb  media
    Drive240MMedia144M,                    // 3.5"  240Mb drive; 1.44Mb  media
    Drive240MMedia120M,                    // 3.5"  240Mb drive; 120Mb  media
    Drive240MMedia240M                     // 3.5"  240Mb drive; 240Mb  media
} DRIVE_MEDIA_TYPE;

//
// When we want to determine the media type in a drive, we will first
// guess that the media with highest possible density is in the drive,
// and keep trying lower densities until we can successfully read from
// the drive.
//
// These values are used to select a DRIVE_MEDIA_TYPE value.
//
// The following table defines ranges that apply to the DRIVE_MEDIA_TYPE
// enumerated values when trying media types for a particular drive type.
// Note that for this to work, the DRIVE_MEDIA_TYPE values must be sorted
// by ascending densities within drive types.  Also, for maximum track
// size to be determined properly, the drive types must be in ascending
// order.
//

typedef struct _DRIVE_MEDIA_LIMITS {
    DRIVE_MEDIA_TYPE HighestDriveMediaType;
    DRIVE_MEDIA_TYPE LowestDriveMediaType;
} DRIVE_MEDIA_LIMITS, *PDRIVE_MEDIA_LIMITS;

#if 0
DRIVE_MEDIA_LIMITS DriveMediaLimits[NUMBER_OF_DRIVE_TYPES] = {

    { Drive360Media360, Drive360Media160 }, // DRIVE_TYPE_0360
    { Drive120Media120, Drive120Media160 }, // DRIVE_TYPE_1200
    { Drive720Media720, Drive720Media720 }, // DRIVE_TYPE_0720
    { Drive144Media144, Drive144Media720 }, // DRIVE_TYPE_1440
    { Drive288Media288, Drive288Media720 }, // DRIVE_TYPE_2880
    { Drive2080Media2080, Drive2080Media720 }
};
#else
DRIVE_MEDIA_LIMITS DriveMediaLimits[NUMBER_OF_DRIVE_TYPES] = {

    { Drive720Media720, Drive720Media720 }, // DRIVE_TYPE_0720
    { Drive144Media144,  Drive144Media720}, // DRIVE_TYPE_1440
    { Drive288Media288,  Drive288Media720}, // DRIVE_TYPE_2880
    { Drive2080Media2080, Drive2080Media720 },
    { Drive32MMedia32M, Drive32MMedia32M }, // DRIVE_TYPE_32M
    { Drive120MMedia120M, Drive120MMedia720 }, // DRIVE_TYPE_120M
    { Drive240MMedia240M, Drive240MMedia144M } // DRIVE_TYPE_240M
};

#endif
//
// For each drive/media combination, define important constants.
//

typedef struct _DRIVE_MEDIA_CONSTANTS {
    MEDIA_TYPE MediaType;
    USHORT     BytesPerSector;
    UCHAR      SectorsPerTrack;
    USHORT     MaximumTrack;
    UCHAR      NumberOfHeads;
} DRIVE_MEDIA_CONSTANTS, *PDRIVE_MEDIA_CONSTANTS;

//
// Magic value to add to the SectorLengthCode to use it as a shift value
// to determine the sector size.
//

#define SECTORLENGTHCODE_TO_BYTESHIFT      7

//
// The following values were gleaned from many different sources, which
// often disagreed with each other.  Where numbers were in conflict, I
// chose the more conservative or most-often-selected value.
//

DRIVE_MEDIA_CONSTANTS DriveMediaConstants[] =
    {

    { F5_160_512,   0x200, 0x08, 0x27, 0x1 },
    { F5_180_512,   0x200, 0x09, 0x27, 0x1 },
    { F5_320_1024,  0x400, 0x04, 0x27, 0x2 },
    { F5_320_512,   0x200, 0x08, 0x27, 0x2 },
    { F5_360_512,   0x200, 0x09, 0x27, 0x2 },

    { F3_720_512,   0x200, 0x09, 0x4f, 0x2 },

    { F5_160_512,   0x200, 0x08, 0x27, 0x1 },
    { F5_180_512,   0x200, 0x09, 0x27, 0x1 },
    { F5_320_1024,  0x400, 0x04, 0x27, 0x2 },
    { F5_320_512,   0x200, 0x08, 0x27, 0x2 },
    { F5_360_512,   0x200, 0x09, 0x27, 0x2 },
    { F5_1Pt2_512,  0x200, 0x0f, 0x4f, 0x2 },

    { F3_720_512,   0x200, 0x09, 0x4f, 0x2 },
    { F3_1Pt44_512, 0x200, 0x12, 0x4f, 0x2 },

    { F3_720_512,   0x200, 0x09, 0x4f, 0x2 },
    { F3_1Pt44_512, 0x200, 0x12, 0x4f, 0x2 },
    { F3_2Pt88_512, 0x200, 0x24, 0x4f, 0x2 },

    { F3_720_512,   0x200, 0x09, 0x4f, 0x2 },
    { F3_1Pt44_512, 0x200, 0x12, 0x4f, 0x2 },
    { F3_20Pt8_512, 0x200, 0x1b, 0xfa, 0x6 },

    { F3_32M_512,   0x200, 0x20, 0x3ff,0x2},

    { F3_720_512,   0x200, 0x09, 0x4f, 0x2 },
    { F3_1Pt44_512, 0x200, 0x12, 0x4f, 0x2 },
    { F3_120M_512,  0x200, 0x20, 0x3c2,0x8 },

    { F3_1Pt44_512, 0x200, 0x12, 0x4f, 0x2 },
    { F3_120M_512,  0x200, 0x20, 0x3c2,0x8 },
    { F3_240M_512,  0x200, 0x38, 0x105,0x20}
};


#define NUMBER_OF_DRIVE_MEDIA_COMBINATIONS sizeof(DriveMediaConstants)/sizeof(DRIVE_MEDIA_CONSTANTS)

//
// floppy device data
//

typedef struct _DISK_DATA {
    ULONG DriveType;
    BOOLEAN IsDMF;
    // BOOLEAN EnableDMF;
    UNICODE_STRING FloppyInterfaceString;
} DISK_DATA, *PDISK_DATA;

//
// The FloppyCapacities and FloppyGeometries arrays are used by the
// USBFlopGetMediaTypes() and USBFlopFormatTracks() routines.

// The FloppyCapacities and FloppyGeometries arrays must be kept in 1:1 sync,
// i.e. each FloppyGeometries[i] must correspond to each FloppyCapacities[i].

// Also, the arrays must be kept in sorted ascending order so that they
// are returned in sorted ascending order by IOCTL_DISK_GET_MEDIA_TYPES.
//

typedef struct _FORMATTED_CAPACITY
{
    ULONG       NumberOfBlocks;

    ULONG       BlockLength;

    BOOLEAN     CanFormat;      // return for IOCTL_DISK_GET_MEDIA_TYPES ?

} FORMATTED_CAPACITY, *PFORMATTED_CAPACITY;


FORMATTED_CAPACITY FloppyCapacities[] =
{
    // Blocks  BlockLen CanFormat H   T  B/S S/T
    {0x000500, 0x0200,  TRUE}, // 2  80  512   8   640 KB  F5_640_512
    {0x0005A0, 0x0200,  TRUE}, // 2  80  512   9   720 KB  F3_720_512
    {0x000960, 0x0200,  TRUE}, // 2  80  512  15  1.20 MB  F3_1Pt2_512   (Toshiba)
    {0x0004D0, 0x0400,  TRUE}, // 2  77 1024   8  1.23 MB  F3_1Pt23_1024 (NEC)
    {0x000B40, 0x0200,  TRUE}, // 2  80  512  18  1.44 MB  F3_1Pt44_512
    {0x000D20, 0x0200, FALSE}, // 2  80  512  21  1.70 MB  DMF
    {0x010000, 0x0200,  TRUE},  // 2  1024 512  32   32 MB    F3_32M_512
    {0x03C300, 0x0200,  TRUE}, // 8 963  512  32   120 MB  F3_120M_512
    {0x0600A4, 0x0200,  TRUE}, // 13 890  512  34   200 MB  F3_200Mb_512 (HiFD)
    {0x072A00, 0x0200,  TRUE}  // 32 262  512  56   240 MB  F3_240M_512
};

DISK_GEOMETRY FloppyGeometries[] =
{
    // Cyl      MediaType       Trk/Cyl Sec/Trk Bytes/Sec
#ifndef __REACTOS__
    {{80,0},    F3_640_512,     2,      8,      512},
    {{80,0},    F3_720_512,     2,      9,      512},
    {{80,0},    F3_1Pt2_512,    2,      15,     512},
    {{77,0},    F3_1Pt23_1024,  2,      8,      1024},
    {{80,0},    F3_1Pt44_512,   2,      18,     512},
    {{80,0},    F3_1Pt44_512,   2,      21,     512},   // DMF -> F3_1Pt44_512
    {{1024,0},  F3_32M_512,     2,      32,     512},
    {{963,0},   F3_120M_512,    8,      32,     512},
    {{890,0},   F3_200Mb_512,   13,     34,     512},
    {{262,0},   F3_240M_512,    32,     56,     512}
#else
    {{{80,0}},    F3_640_512,     2,      8,      512},
    {{{80,0}},    F3_720_512,     2,      9,      512},
    {{{80,0}},    F3_1Pt2_512,    2,      15,     512},
    {{{77,0}},    F3_1Pt23_1024,  2,      8,      1024},
    {{{80,0}},    F3_1Pt44_512,   2,      18,     512},
    {{{80,0}},    F3_1Pt44_512,   2,      21,     512},   // DMF -> F3_1Pt44_512
    {{{1024,0}},  F3_32M_512,     2,      32,     512},
    {{{963,0}},   F3_120M_512,    8,      32,     512},
    {{{890,0}},   F3_200Mb_512,   13,     34,     512},
    {{{262,0}},   F3_240M_512,    32,     56,     512}
#endif
};

#define FLOPPY_CAPACITIES (sizeof(FloppyCapacities)/sizeof(FloppyCapacities[0]))

C_ASSERT((sizeof(FloppyGeometries)/sizeof(FloppyGeometries[0])) == FLOPPY_CAPACITIES);

//
// The following structures are used by USBFlopFormatTracks()
//

#pragma pack (push, 1)

typedef struct _CDB12FORMAT
{
    UCHAR   OperationCode;
    UCHAR   DefectListFormat : 3;
    UCHAR   CmpList : 1;
    UCHAR   FmtData : 1;
    UCHAR   LogicalUnitNumber : 3;
    UCHAR   TrackNumber;
    UCHAR   InterleaveMsb;
    UCHAR   InterleaveLsb;
    UCHAR   Reserved1[2];
    UCHAR   ParameterListLengthMsb;
    UCHAR   ParameterListLengthLsb;
    UCHAR   Reserved2[3];
} CDB12FORMAT, *PCDB12FORMAT;


typedef struct _DEFECT_LIST_HEADER
{
    UCHAR   Reserved1;
    UCHAR   Side : 1;
    UCHAR   Immediate : 1;
    UCHAR   Reserved2 : 2;
    UCHAR   SingleTrack : 1;
    UCHAR   DisableCert : 1;
    UCHAR   Reserved3 : 1;
    UCHAR   FormatOptionsValid : 1;
    UCHAR   DefectListLengthMsb;
    UCHAR   DefectListLengthLsb;
} DEFECT_LIST_HEADER, *PDEFECT_LIST_HEADER;

typedef struct _FORMAT_UNIT_PARAMETER_LIST
{
    DEFECT_LIST_HEADER DefectListHeader;
    FORMATTED_CAPACITY_DESCRIPTOR FormatDescriptor;
} FORMAT_UNIT_PARAMETER_LIST, *PFORMAT_UNIT_PARAMETER_LIST;

#pragma pack (pop)

DRIVER_INITIALIZE DriverEntry;

DRIVER_UNLOAD     ScsiFlopUnload;

DRIVER_ADD_DEVICE ScsiFlopAddDevice;

NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopInitDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopStartDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopRemoveDevice(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR Type
    );

NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopStopDevice(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR Type
    );

BOOLEAN
FindScsiFlops(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN PCLASS_INIT_DATA InitializationData,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber
    );

NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
IsFloppyDevice(
    PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
CreateFlopDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG DeviceCount
    );

NTSTATUS
DetermineMediaType(
    PDEVICE_OBJECT DeviceObject
    );

ULONG
DetermineDriveType(
    PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
FlCheckFormatParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFORMAT_PARAMETERS FormatParameters
    );

NTSTATUS
FormatMedia(
    PDEVICE_OBJECT DeviceObject,
    MEDIA_TYPE MediaType
    );

NTSTATUS
FlopticalFormatMedia(
    PDEVICE_OBJECT DeviceObject,
    PFORMAT_PARAMETERS Format
    );

VOID
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

NTSTATUS
USBFlopGetMediaTypes(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
USBFlopFormatTracks(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#pragma alloc_text(PAGE, ScsiFlopUnload)
#pragma alloc_text(PAGE, ScsiFlopAddDevice)
#pragma alloc_text(PAGE, CreateFlopDeviceObject)
#pragma alloc_text(PAGE, ScsiFlopStartDevice)
#pragma alloc_text(PAGE, ScsiFlopRemoveDevice)
#pragma alloc_text(PAGE, IsFloppyDevice)
#pragma alloc_text(PAGE, DetermineMediaType)
#pragma alloc_text(PAGE, DetermineDriveType)
#pragma alloc_text(PAGE, FlCheckFormatParameters)
#pragma alloc_text(PAGE, FormatMedia)
#pragma alloc_text(PAGE, FlopticalFormatMedia)
#pragma alloc_text(PAGE, USBFlopGetMediaTypes)
#pragma alloc_text(PAGE, USBFlopFormatTracks)

#endif


NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the system initialization routine for installable drivers.
    It calls the SCSI class driver initialization routine.

Arguments:

    DriverObject - Pointer to driver object created by system.

Return Value:

    NTSTATUS

--*/

{
    CLASS_INIT_DATA InitializationData;

    //
    // Zero InitData
    //

    RtlZeroMemory (&InitializationData, sizeof(CLASS_INIT_DATA));

    //
    // Set sizes
    //

    InitializationData.InitializationDataSize = sizeof(CLASS_INIT_DATA);
    InitializationData.FdoData.DeviceExtensionSize =
        sizeof(FUNCTIONAL_DEVICE_EXTENSION) + sizeof(DISK_DATA);

    InitializationData.FdoData.DeviceType = FILE_DEVICE_DISK;
    InitializationData.FdoData.DeviceCharacteristics = FILE_REMOVABLE_MEDIA | FILE_FLOPPY_DISKETTE;

    //
    // Set entry points
    //

    InitializationData.FdoData.ClassInitDevice = ScsiFlopInitDevice;
    InitializationData.FdoData.ClassStartDevice = ScsiFlopStartDevice;
    InitializationData.FdoData.ClassStopDevice = ScsiFlopStopDevice;
    InitializationData.FdoData.ClassRemoveDevice = ScsiFlopRemoveDevice;

    InitializationData.FdoData.ClassReadWriteVerification = ScsiFlopReadWriteVerification;
    InitializationData.FdoData.ClassDeviceControl = ScsiFlopDeviceControl;

    InitializationData.FdoData.ClassShutdownFlush = NULL;
    InitializationData.FdoData.ClassCreateClose = NULL;
    InitializationData.FdoData.ClassError = ScsiFlopProcessError;
    InitializationData.ClassStartIo = NULL;

    InitializationData.ClassAddDevice = ScsiFlopAddDevice;
    InitializationData.ClassUnload = ScsiFlopUnload;
    //
    // Call the class init routine
    //

    return ClassInitialize( DriverObject, RegistryPath, &InitializationData);


} // end DriverEntry()

VOID
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);
    return;
}

//
// AddDevice operation is performed in CreateFlopDeviceObject function which
// is called by ScsiFlopAddDevice (The AddDevice routine for sfloppy.sys).
// DO_DEVICE_INITIALIZING flag is cleard upon successfully processing AddDevice
// operation in CreateFlopDeviceObject. But PREFAST is currently unable to
// detect that DO_DEVICE_INITIALIZING is indeed cleard in CreateFlopDeviceObject
// and it raises Warning 28152 (The return from an AddDevice-like function
// unexpectedly did not clear DO_DEVICE_INITIALIZING). Suppress that warning
// using #pragma.
//

#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:28152)
#endif

NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopAddDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:

    This routine creates and initializes a new FDO for the corresponding
    PDO.  It may perform property queries on the FDO but cannot do any
    media access operations.

Arguments:

    DriverObject - Scsiscan class driver object.

    Pdo - the physical device object we are being added to

Return Value:

    status

--*/
{
    NTSTATUS status;
    ULONG floppyCount = IoGetConfigurationInformation()->FloppyCount;

    PAGED_CODE();

    //
    // Get the number of disks already initialized.
    //

    status = CreateFlopDeviceObject(DriverObject, Pdo, floppyCount);

    if (NT_SUCCESS(status)) {

        //
        // Increment system floppy device count.
        //

        IoGetConfigurationInformation()->FloppyCount++;
    }

    return status;
}

NTSTATUS
CreateFlopDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG DeviceCount
    )

/*++

Routine Description:

    This routine creates an object for the device and then calls the
    SCSI port driver for media capacity and sector size.

Arguments:

    DriverObject - Pointer to driver object created by system.
    PortDeviceObject - to connect to SCSI port driver.
    DeviceCount - Number of previously installed Floppys.
    AdapterDescriptor - Pointer to structure returned by SCSI port
                        driver describing adapter capabilites (and limitations).
    DeviceDescriptor - Pointer to configuration information for this device.

Return Value:

--*/
{
    NTSTATUS        status;
    PDEVICE_OBJECT  deviceObject = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;
    PDISK_DATA  diskData;

    PAGED_CODE();

    DebugPrint((3,"CreateFlopDeviceObject: Enter routine\n"));

    //
    // Try to claim the device.
    //

    status = ClassClaimDevice(Pdo,FALSE);

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    DeviceCount--;

    do {
        UCHAR name[256];

        //
        // Create device object for this device.
        //

        DeviceCount++;

        status = RtlStringCbPrintfA((PCCHAR) name,
                                    sizeof(name),
                                    "\\Device\\Floppy%u",
                                    DeviceCount);
        if (NT_SUCCESS(status)) {

            status = ClassCreateDeviceObject(DriverObject,
                                             (PCCHAR) name,
                                             Pdo,
                                             TRUE,
                                             &deviceObject);
        }
    } while ((status == STATUS_OBJECT_NAME_COLLISION) ||
             (status == STATUS_OBJECT_NAME_EXISTS));

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"CreateFlopDeviceObjects: Can not create device\n"));
        goto CreateFlopDeviceObjectExit;
    }

    //
    // Indicate that IRPs should include MDLs.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    fdoExtension = deviceObject->DeviceExtension;

    //
    // Back pointer to device object.
    //

    fdoExtension->CommonExtension.DeviceObject = deviceObject;

    //
    // This is the physical device.
    //

    fdoExtension->CommonExtension.PartitionZeroExtension = fdoExtension;

    //
    // Reset the drive type.
    //

    diskData = (PDISK_DATA) fdoExtension->CommonExtension.DriverData;
    diskData->DriveType = DRIVE_TYPE_NONE;
    diskData->IsDMF = FALSE;
    // diskData->EnableDMF = TRUE;

    //
    // Initialize lock count to zero. The lock count is used to
    // disable the ejection mechanism when media is mounted.
    //

    fdoExtension->LockCount = 0;

    //
    // Save system floppy number
    //

    fdoExtension->DeviceNumber = DeviceCount;

    //
    // Set the alignment requirements for the device based on the
    // host adapter requirements
    //

    if (Pdo->AlignmentRequirement > deviceObject->AlignmentRequirement) {
        deviceObject->AlignmentRequirement = Pdo->AlignmentRequirement;
    }

    //
    // Clear the SrbFlags and disable synchronous transfers
    //

    fdoExtension->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Finally, attach to the PDO
    //

    fdoExtension->LowerPdo = Pdo;

    fdoExtension->CommonExtension.LowerDeviceObject =
        IoAttachDeviceToDeviceStack(deviceObject, Pdo);

    if(fdoExtension->CommonExtension.LowerDeviceObject == NULL) {

        status = STATUS_UNSUCCESSFUL;
        goto CreateFlopDeviceObjectExit;
    }

    deviceObject->StackSize++;

    //
    // The device is initialized properly - mark it as such.
    //

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

CreateFlopDeviceObjectExit:

    if (deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }

    return status;

} // end CreateFlopDeviceObject()
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(pop)
#endif

NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopInitDevice(
    IN PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PDISK_DATA diskData = commonExtension->DriverData;

    PVOID senseData = NULL;
    ULONG timeOut;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Allocate request sense buffer.
    //

    senseData = ExAllocatePool(NonPagedPoolNxCacheAligned, SENSE_BUFFER_SIZE);

    if (senseData == NULL) {

        //
        // The buffer cannot be allocated.
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Set the sense data pointer in the device extension.
    //

    fdoExtension->SenseData = senseData;

    //
    // Build the lookaside list for srb's for this device.
    //

    ClassInitializeSrbLookasideList((PCOMMON_DEVICE_EXTENSION)fdoExtension,
                                    SFLOPPY_SRB_LIST_SIZE);

    //
    // Register for media change notification
    //
    ClassInitializeMediaChangeDetection(fdoExtension,
                                        (PUCHAR) "SFloppy");

    //
    // Set timeout value in seconds.
    //

    timeOut = ClassQueryTimeOutRegistryValue(Fdo);
    if (timeOut) {
        fdoExtension->TimeOutValue = timeOut;
    } else {
        fdoExtension->TimeOutValue = SCSI_FLOPPY_TIMEOUT;
    }

    //
    // Floppies are not partitionable so starting offset is 0.
    //

    fdoExtension->CommonExtension.StartingOffset.QuadPart = (LONGLONG)0;

#if 0
    if (!IsFloppyDevice(Fdo) ||
        !(Fdo->Characteristics & FILE_REMOVABLE_MEDIA) ||
        (fdoExtension->DeviceDescriptor->DeviceType != DIRECT_ACCESS_DEVICE)) {

        ExFreePool(senseData);
        status = STATUS_NO_SUCH_DEVICE;
        return status;
    }
#endif

    RtlZeroMemory(&(fdoExtension->DiskGeometry),
                  sizeof(DISK_GEOMETRY));

    //
    // Determine the media type if possible. Set the current media type to
    // Unknown so that determine media type will check the media.
    //

    fdoExtension->DiskGeometry.MediaType = Unknown;

    //
    // Register interfaces for this device.
    //

    {
        UNICODE_STRING interfaceName;

        RtlInitUnicodeString(&interfaceName, NULL);

        status = IoRegisterDeviceInterface(fdoExtension->LowerPdo,
                                           (LPGUID) &GUID_DEVINTERFACE_FLOPPY,
                                           NULL,
                                           &interfaceName);

        if(NT_SUCCESS(status)) {
            diskData->FloppyInterfaceString = interfaceName;
        } else {
            RtlInitUnicodeString(&(diskData->FloppyInterfaceString), NULL);
            DebugPrint((1, "ScsiFlopStartDevice: Unable to register device "
                           "interface for fdo %p [%08lx]\n",
                        Fdo, status));
        }
    }

    return (STATUS_SUCCESS);
}

#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(suppress:6262) // This function uses 1096 bytes of stack which exceed default value of 1024 bytes used by Code Analysis for flagging as warning
#endif
#ifdef __REACTOS__
NTSTATUS NTAPI ScsiFlopStartDevice(
#else
NTSTATUS ScsiFlopStartDevice(
#endif
    IN PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;

    PIRP        irp;
    IO_STATUS_BLOCK ioStatus;

    SCSI_ADDRESS    scsiAddress;

    WCHAR   ntNameBuffer[256];
    UNICODE_STRING  ntUnicodeString;

    WCHAR   arcNameBuffer[256];
    UNICODE_STRING  arcUnicodeString;

    KEVENT event;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    KeInitializeEvent(&event,SynchronizationEvent,FALSE);

    DetermineMediaType(Fdo); // ignore unsuccessful here

    //
    // Create device object for this device.
    //

    RtlStringCbPrintfW(ntNameBuffer,
                       sizeof(ntNameBuffer),
                       L"\\Device\\Floppy%u",
                       fdoExtension->DeviceNumber);

    //
    // Create local copy of unicode string
    //
    RtlInitUnicodeString(&ntUnicodeString,ntNameBuffer);

    //
    // Create a symbolic link from the disk name to the corresponding
    // ARC name, to be used if we're booting off the disk.  This will
    // fail if it's not system initialization time; that's fine.  The
    // ARC name looks something like \ArcName\scsi(0)Flop(0)fdisk(0).
    // In order to get the address, we need to send a IOCTL_SCSI_GET_ADDRESS...
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_ADDRESS,
                                        Fdo,
                                        NULL,
                                        0,
                                        &scsiAddress,
                                        sizeof(scsiAddress),
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // IOCTL_SCSI_GET_ADDRESS might not be supported by the port driver and
    // hence may fail. But it is not a fatal error. Do not fail PnP start
    // if this IOCTL fails.
    //
    if (NT_SUCCESS(status)) {

        RtlStringCbPrintfW(arcNameBuffer,
                           sizeof(arcNameBuffer),
                           L"\\ArcName\\scsi(%u)disk(%u)fdisk(%u)",
                           scsiAddress.PortNumber,
                           scsiAddress.TargetId,
                           scsiAddress.Lun);

        RtlInitUnicodeString(&arcUnicodeString, arcNameBuffer);

        IoAssignArcName(&arcUnicodeString, &ntUnicodeString);
    }

    status = STATUS_SUCCESS;

    //
    // Create the multi() arc name -- Create the "fake"
    // name of multi(0)disk(0)fdisk(#) to handle the case where the
    // SCSI floppy is the only floppy in the system.  If this fails
    // it doesn't matter because the previous scsi() based ArcName
    // will work.  This name is necessary for installation.
    //

    RtlStringCbPrintfW(arcNameBuffer,
                       sizeof(arcNameBuffer),
                       L"\\ArcName\\multi(%u)disk(%u)fdisk(%u)",
                       0,
                       0,
                       fdoExtension->DeviceNumber);

    RtlInitUnicodeString(&arcUnicodeString, arcNameBuffer);

    IoAssignArcName(&arcUnicodeString, &ntUnicodeString);

    //
    // Set our interface state.
    //

    {
        PDISK_DATA diskData = commonExtension->DriverData;

        if(diskData->FloppyInterfaceString.Buffer != NULL) {

            status = IoSetDeviceInterfaceState(
                        &(diskData->FloppyInterfaceString),
                        TRUE);

            if(!NT_SUCCESS(status)) {
                DebugPrint((1, "ScsiFlopStartDevice: Unable to set device "
                               "interface state to TRUE for fdo %p "
                               "[%08lx]\n",
                            Fdo, status));
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

Return Value:

    NT Status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Make sure that the number of bytes to transfer is a multiple of the sector size
    //
    if ((irpSp->Parameters.Read.Length & (fdoExtension->DiskGeometry.BytesPerSector - 1)) != 0)
    {
        status = STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Status = status;

    return status;
}


NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

Arguments:

Return Value:

    Status is returned.

--*/

{
    KIRQL currentIrql;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    NTSTATUS status;
    PDISK_GEOMETRY outputBuffer;
    ULONG outputBufferLength;
    ULONG i;
    DRIVE_MEDIA_TYPE lowestDriveMediaType;
    DRIVE_MEDIA_TYPE highestDriveMediaType;
    PFORMAT_PARAMETERS formatParameters;
    PMODE_PARAMETER_HEADER modeData;
    ULONG length;

    //
    // Initialize the information field
    //
    Irp->IoStatus.Information = 0;

    srb = ExAllocatePool(NonPagedPoolNx, SCSI_REQUEST_BLOCK_SIZE);

    if (srb == NULL) {

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        if (IoIsErrorUserInduced(Irp->IoStatus.Status)) {

            IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        }

        KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, 0);
        KeLowerIrql(currentIrql);

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Write zeros to Srb.
    //

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

    cdb = (PCDB)srb->Cdb;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {


    case IOCTL_DISK_VERIFY: {

       PVERIFY_INFORMATION verifyInfo = Irp->AssociatedIrp.SystemBuffer;
       LARGE_INTEGER byteOffset;
       ULONG         sectorOffset;
       USHORT        sectorCount;

       //
       // Validate buffer length.
       //

       if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
           sizeof(VERIFY_INFORMATION)) {

           status = STATUS_INFO_LENGTH_MISMATCH;
           break;
       }

       //
       // Perform a bounds check on the sector range
       //
       if ((verifyInfo->StartingOffset.QuadPart > fdoExtension->CommonExtension.PartitionLength.QuadPart) ||
           (verifyInfo->StartingOffset.QuadPart < 0))
       {
           status = STATUS_NONEXISTENT_SECTOR;
           break;
       }
       else
       {
           ULONGLONG bytesRemaining = fdoExtension->CommonExtension.PartitionLength.QuadPart - verifyInfo->StartingOffset.QuadPart;

           if ((ULONGLONG)verifyInfo->Length > bytesRemaining)
           {
               status = STATUS_NONEXISTENT_SECTOR;
               break;
           }
       }

       //
       // Verify sectors
       //

       srb->CdbLength = 10;

       cdb->CDB10.OperationCode = SCSIOP_VERIFY;

       //
       // Add disk offset to starting sector.
       //

       byteOffset.QuadPart = fdoExtension->CommonExtension.StartingOffset.QuadPart +
                       verifyInfo->StartingOffset.QuadPart;

       //
       // Convert byte offset to sector offset.
       //

       sectorOffset = (ULONG)(byteOffset.QuadPart >> fdoExtension->SectorShift);

       //
       // Convert ULONG byte count to USHORT sector count.
       //

       sectorCount = (USHORT)(verifyInfo->Length >> fdoExtension->SectorShift);

       //
       // Move little endian values into CDB in big endian format.
       //

       cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&sectorOffset)->Byte3;
       cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&sectorOffset)->Byte2;
       cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&sectorOffset)->Byte1;
       cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&sectorOffset)->Byte0;

       cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&sectorCount)->Byte1;
       cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&sectorCount)->Byte0;

       //
       // The verify command is used by the NT FORMAT utility and
       // requests are sent down for 5% of the volume size. The
       // request timeout value is calculated based on the number of
       // sectors verified.
       //

       srb->TimeOutValue = ((sectorCount + 0x7F) >> 7) *
                             fdoExtension->TimeOutValue;

       status = ClassSendSrbAsynchronous(DeviceObject,
                                         srb,
                                         Irp,
                                         NULL,
                                         0,
                                         FALSE);
       return(status);

    }

    case IOCTL_DISK_GET_PARTITION_INFO: {

        if (fdoExtension->AdapterDescriptor->BusType == BusTypeUsb) {

            USBFlopGetMediaTypes(DeviceObject, NULL);

            // Don't need to propagate any error if one occurs
            //
            status = STATUS_SUCCESS;

        } else {

            status = DetermineMediaType(DeviceObject);
        }

        if (!NT_SUCCESS(status)) {
            // so will propogate error
            NOTHING;
        } else if (fdoExtension->DiskGeometry.MediaType == F3_120M_512) {
            //so that the format code will not try to partition it.
            status = STATUS_INVALID_DEVICE_REQUEST;
        } else {
           //
           // Free the Srb, since it is not needed.
           //

           ExFreePool(srb);

           //
           // Pass the request to the common device control routine.
           //

           return(ClassDeviceControl(DeviceObject, Irp));
        }
        break;
    }

    case IOCTL_DISK_GET_DRIVE_GEOMETRY: {

        DebugPrint((3,"ScsiDeviceIoControl: Get drive geometry\n"));

        if (fdoExtension->AdapterDescriptor->BusType == BusTypeUsb)
        {
            status = USBFlopGetMediaTypes(DeviceObject,
                                          Irp);
            break;
        }

        //
        // If there's not enough room to write the
        // data, then fail the request.
        //

        if ( irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof( DISK_GEOMETRY ) ) {

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        status = DetermineMediaType(DeviceObject);

        if (!NT_SUCCESS(status)) {

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = status;

        } else {

            //
            // Copy drive geometry information from device extension.
            //

            RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                          &(fdoExtension->DiskGeometry),
                          sizeof(DISK_GEOMETRY));

            Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
            status = STATUS_SUCCESS;

        }

        break;
    }

    case IOCTL_DISK_GET_MEDIA_TYPES: {

        if (fdoExtension->AdapterDescriptor->BusType == BusTypeUsb)
        {
            status = USBFlopGetMediaTypes(DeviceObject,
                                          Irp);
            break;
        }

        i = DetermineDriveType(DeviceObject);

        if (i == DRIVE_TYPE_NONE) {
            status = STATUS_UNRECOGNIZED_MEDIA;
            break;
        }

        lowestDriveMediaType = DriveMediaLimits[i].LowestDriveMediaType;
        highestDriveMediaType = DriveMediaLimits[i].HighestDriveMediaType;

        outputBufferLength =
        irpStack->Parameters.DeviceIoControl.OutputBufferLength;

        //
        // Make sure that the input buffer has enough room to return
        // at least one descriptions of a supported media type.
        //

        if ( outputBufferLength < ( sizeof( DISK_GEOMETRY ) ) ) {

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Assume success, although we might modify it to a buffer
        // overflow warning below (if the buffer isn't big enough
        // to hold ALL of the media descriptions).
        //

        status = STATUS_SUCCESS;

        if (outputBufferLength < ( sizeof( DISK_GEOMETRY ) *
            ( highestDriveMediaType - lowestDriveMediaType + 1 ) ) ) {

            //
            // The buffer is too small for all of the descriptions;
            // calculate what CAN fit in the buffer.
            //

            status = STATUS_BUFFER_OVERFLOW;

            highestDriveMediaType = (DRIVE_MEDIA_TYPE)( ( lowestDriveMediaType - 1 ) +
                ( outputBufferLength /
                sizeof( DISK_GEOMETRY ) ) );
        }

        outputBuffer = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;

        for (i = (UCHAR)lowestDriveMediaType;i <= (UCHAR)highestDriveMediaType;i++ ) {

             outputBuffer->MediaType = DriveMediaConstants[i].MediaType;
             outputBuffer->Cylinders.LowPart =
                 DriveMediaConstants[i].MaximumTrack + 1;
             outputBuffer->Cylinders.HighPart = 0;
             outputBuffer->TracksPerCylinder =
                 DriveMediaConstants[i].NumberOfHeads;
             outputBuffer->SectorsPerTrack =
                 DriveMediaConstants[i].SectorsPerTrack;
             outputBuffer->BytesPerSector =
                 DriveMediaConstants[i].BytesPerSector;
             outputBuffer++;

             Irp->IoStatus.Information += sizeof( DISK_GEOMETRY );
        }

        break;
    }

    case IOCTL_DISK_FORMAT_TRACKS: {

        if (fdoExtension->AdapterDescriptor->BusType == BusTypeUsb)
        {
            status = USBFlopFormatTracks(DeviceObject,
                                         Irp);
            break;
        }

        //
        // Make sure that we got all the necessary format parameters.
        //

        if ( irpStack->Parameters.DeviceIoControl.InputBufferLength <sizeof( FORMAT_PARAMETERS ) ) {

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        formatParameters = (PFORMAT_PARAMETERS) Irp->AssociatedIrp.SystemBuffer;

        //
        // Make sure the parameters we got are reasonable.
        //

        if ( !FlCheckFormatParameters(DeviceObject, formatParameters)) {

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // If this request is for a 20.8 MB floppy then call a special
        // floppy format routine.
        //

        if (formatParameters->MediaType == F3_20Pt8_512) {
            status = FlopticalFormatMedia(DeviceObject,
                                          formatParameters
                                          );

            break;
        }

        //
        // All the work is done in the pass.  If this is not the first pass,
        // then complete the request and return;
        //

        if (formatParameters->StartCylinderNumber != 0 || formatParameters->StartHeadNumber != 0) {

            status = STATUS_SUCCESS;
            break;
        }

        status = FormatMedia( DeviceObject, formatParameters->MediaType);
        break;
    }

    case IOCTL_DISK_IS_WRITABLE: {

        if ((fdoExtension->DiskGeometry.MediaType) == F3_32M_512) {

            //
            // 32MB media is READ ONLY. Just return
            // STATUS_MEDIA_WRITE_PROTECTED
            //

            status = STATUS_MEDIA_WRITE_PROTECTED;

            break;
        }

        //
        // Determine if the device is writable.
        //

        modeData = ExAllocatePool(NonPagedPoolNxCacheAligned, MODE_DATA_SIZE);

        if (modeData == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory(modeData, MODE_DATA_SIZE);

        length = ClassModeSense(DeviceObject,
                                (PCHAR) modeData,
                                MODE_DATA_SIZE,
                                MODE_SENSE_RETURN_ALL);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            //
            // Retry the request in case of a check condition.
            //

            length = ClassModeSense(DeviceObject,
                                    (PCHAR) modeData,
                                    MODE_DATA_SIZE,
                                    MODE_SENSE_RETURN_ALL);

            if (length < sizeof(MODE_PARAMETER_HEADER)) {
                status = STATUS_IO_DEVICE_ERROR;
                ExFreePool(modeData);
                break;
            }
        }

        if (modeData->DeviceSpecificParameter & MODE_DSP_WRITE_PROTECT) {
            status = STATUS_MEDIA_WRITE_PROTECTED;
        } else {
            status = STATUS_SUCCESS;
        }

        DebugPrint((2,"IOCTL_DISK_IS_WRITABLE returns %08X\n", status));

        ExFreePool(modeData);
        break;
    }

    default: {

        DebugPrint((3,"ScsiIoDeviceControl: Unsupported device IOCTL\n"));

        //
        // Free the Srb, since it is not needed.
        //

        ExFreePool(srb);

        //
        // Pass the request to the common device control routine.
        //

        return(ClassDeviceControl(DeviceObject, Irp));

        break;
    }

    } // end switch( ...

    //
    // Check if SL_OVERRIDE_VERIFY_VOLUME flag is set in the IRP.
    // If so, do not return STATUS_VERIFY_REQUIRED
    //
    if ((status == STATUS_VERIFY_REQUIRED) &&
        (TEST_FLAG(irpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME))) {

        status = STATUS_IO_DEVICE_ERROR;

    }

    Irp->IoStatus.Status = status;

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {

        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
    }

    KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, 0);
    KeLowerIrql(currentIrql);

    ExFreePool(srb);

    return status;

} // end ScsiFlopDeviceControl()

#if 0

BOOLEAN
IsFloppyDevice(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    The routine performs the necessary funcitons to deterime if the device is
    really a floppy rather than a harddisk.  This is done by a mode sense
    command.  First a check is made to see if the medimum type is set.  Second
    a check is made for the flexible parameters mode page.

Arguments:

    DeviceObject - Supplies the device object to be tested.

Return Value:

    Return TRUE if the indicated device is a floppy.

--*/
{

    PVOID modeData;
    PUCHAR pageData;
    ULONG length;

    modeData = ExAllocatePool(NonPagedPoolNxCacheAligned, MODE_DATA_SIZE);

    if (modeData == NULL) {
        return(FALSE);
    }

    RtlZeroMemory(modeData, MODE_DATA_SIZE);

    length = ClassModeSense(DeviceObject, modeData, MODE_DATA_SIZE, MODE_SENSE_RETURN_ALL);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        length = ClassModeSense(DeviceObject,
                    modeData,
                    MODE_DATA_SIZE,
                    MODE_SENSE_RETURN_ALL);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            ExFreePool(modeData);
            return(FALSE);

        }
    }

#if 0
    //
    // Some drives incorrectly report this.  In particular the SONY RMO-S350
    // when in disk mode.
    //

    if (((PMODE_PARAMETER_HEADER) modeData)->MediumType >= MODE_FD_SINGLE_SIDE
        && ((PMODE_PARAMETER_HEADER) modeData)->MediumType <= MODE_FD_MAXIMUM_TYPE) {

        DebugPrint((1, "ScsiFlop: MediumType value %2x, This is a floppy.\n", ((PMODE_PARAMETER_HEADER) modeData)->MediumType));
        ExFreePool(modeData);
        return(TRUE);
    }

#endif

    //
    // If the length is greater than length indiated by the mode data reset
    // the data to the mode data.
    //
    if (length > (ULONG)((PMODE_PARAMETER_HEADER) modeData)->ModeDataLength + 1) {
        length = (ULONG)((PMODE_PARAMETER_HEADER) modeData)->ModeDataLength + 1;

    }

    //
    // Look for the flexible disk mode page.
    //

    pageData = ClassFindModePage( modeData, length, MODE_PAGE_FLEXIBILE, TRUE);

    if (pageData != NULL) {

        DebugPrint((1, "ScsiFlop: Flexible disk page found, This is a floppy.\n"));

        //
        // As a special case for the floptical driver do a magic mode sense to
        // enable the drive.
        //

        ClassModeSense(DeviceObject, modeData, 0x2a, 0x2e);

        ExFreePool(modeData);
        return(TRUE);

    }

    ExFreePool(modeData);
    return(FALSE);

}
#endif


NTSTATUS
DetermineMediaType(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine determines the floppy media type based on the size of the
    device.  The geometry information is set for the device object.

Arguments:

    DeviceObject - Supplies the device object to be tested.

Return Value:

    None

--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_GEOMETRY geometry;
    LONG index;
    NTSTATUS status;

    PAGED_CODE();

    geometry = &(fdoExtension->DiskGeometry);

    //
    // Issue ReadCapacity to update device extension
    // with information for current media.
    //

    status = ClassReadDriveCapacity(DeviceObject);

    if (!NT_SUCCESS(status)) {

       //
       // Set the media type to unknow and zero the geometry information.
       //

       geometry->MediaType = Unknown;

       return status;

    }

    //
    // Look at the capcity of disk to determine its type.
    //

    for (index = NUMBER_OF_DRIVE_MEDIA_COMBINATIONS - 1; index >= 0; index--) {

        //
        // Walk the table backward untill the drive capacity holds all of the
        // data and the bytes per setor are equal
        //

         if ((ULONG) (DriveMediaConstants[index].NumberOfHeads *
             (DriveMediaConstants[index].MaximumTrack + 1) *
             DriveMediaConstants[index].SectorsPerTrack *
             DriveMediaConstants[index].BytesPerSector) <=
             fdoExtension->CommonExtension.PartitionLength.LowPart &&
             DriveMediaConstants[index].BytesPerSector ==
             geometry->BytesPerSector) {

             geometry->MediaType = DriveMediaConstants[index].MediaType;
             geometry->TracksPerCylinder = DriveMediaConstants[index].NumberOfHeads;
             geometry->SectorsPerTrack = DriveMediaConstants[index].SectorsPerTrack;
             geometry->Cylinders.LowPart = DriveMediaConstants[index].MaximumTrack+1;
             break;
         }
    }

    if (index == -1) {

        //
        // Set the media type to unknow and zero the geometry information.
        //

        geometry->MediaType = Unknown;


    } else {
        //
        // DMF check breaks the insight SCSI floppy, so its disabled for that case
        //
        PDISK_DATA diskData = (PDISK_DATA) fdoExtension->CommonExtension.DriverData;

        // if (diskData->EnableDMF == TRUE) {

            //
            //check to see if DMF
            //

            PSCSI_REQUEST_BLOCK srb;
            PVOID               readData;

            //
            // Allocate a Srb for the read command.
            //

            readData = ExAllocatePool(NonPagedPoolNx, geometry->BytesPerSector);
            if (readData == NULL) {
                return STATUS_NO_MEMORY;
            }

            srb = ExAllocatePool(NonPagedPoolNx, SCSI_REQUEST_BLOCK_SIZE);

            if (srb == NULL) {

                ExFreePool(readData);
                return STATUS_NO_MEMORY;
            }

            RtlZeroMemory(readData, geometry->BytesPerSector);
            RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

            srb->CdbLength = 10;
            srb->Cdb[0] = SCSIOP_READ;
            srb->Cdb[5] = 0;
            srb->Cdb[8] = (UCHAR) 1;

            //
            // Set timeout value.
            //

            srb->TimeOutValue = fdoExtension->TimeOutValue;

            //
            // Send the mode select data.
            //

            status = ClassSendSrbSynchronous(DeviceObject,
                      srb,
                      readData,
                      geometry->BytesPerSector,
                      FALSE
                      );

            if (NT_SUCCESS(status)) {
                char *pchar = (char *)readData;

                pchar += 3; //skip 3 bytes jump code

                // If the MSDMF3. signature is there then mark it as DMF diskette
                if (RtlCompareMemory(pchar, "MSDMF3.", 7) == 7) {
                    diskData->IsDMF = TRUE;
                }

            }
            ExFreePool(readData);
            ExFreePool(srb);
        // }// else
    }
    return status;
}

ULONG
DetermineDriveType(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    The routine determines the device type so that the supported medias can be
    determined.  It does a mode sense for the default parameters.  This code
    assumes that the returned values are for the maximum device size.

Arguments:

    DeviceObject - Supplies the device object to be tested.

Return Value:

    None

--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PVOID modeData;
    PDISK_DATA diskData = (PDISK_DATA) fdoExtension->CommonExtension.DriverData;
    PMODE_FLEXIBLE_DISK_PAGE pageData;
    ULONG length;
    LONG index;
    UCHAR numberOfHeads;
    UCHAR sectorsPerTrack;
    USHORT maximumTrack;
    BOOLEAN applyFix = FALSE;

    PAGED_CODE();

    if (diskData->DriveType != DRIVE_TYPE_NONE) {
        return(diskData->DriveType);
    }

    modeData = ExAllocatePool(NonPagedPoolNxCacheAligned, MODE_DATA_SIZE);

    if (modeData == NULL) {
        return(DRIVE_TYPE_NONE);
    }

    RtlZeroMemory(modeData, MODE_DATA_SIZE);

    length = ClassModeSense(DeviceObject,
                            modeData,
                            MODE_DATA_SIZE,
                            MODE_PAGE_FLEXIBILE);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request one more time
        // in case of a check condition.
        //
        length = ClassModeSense(DeviceObject,
                                modeData,
                                MODE_DATA_SIZE,
                                MODE_PAGE_FLEXIBILE);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            ExFreePool(modeData);
            return(DRIVE_TYPE_NONE);
        }
    }

    //
    // Look for the flexible disk mode page.
    //

    pageData = ClassFindModePage( modeData,
                                  length,
                                  MODE_PAGE_FLEXIBILE,
                                  TRUE);

    //
    // Make sure the page is returned and is large enough.
    //

    if ((pageData != NULL) &&
        (pageData->PageLength + 2 >=
         (UCHAR)offsetof(MODE_FLEXIBLE_DISK_PAGE, StartWritePrecom))) {

       //
       // Pull out the heads, cylinders, and sectors.
       //

       numberOfHeads = pageData->NumberOfHeads;
       maximumTrack = pageData->NumberOfCylinders[1];
       maximumTrack |= pageData->NumberOfCylinders[0] << 8;
       sectorsPerTrack = pageData->SectorsPerTrack;


       //
       // Convert from number of cylinders to maximum track.
       //

       maximumTrack--;

       //
       // Search for the maximum supported media. Based on the number of heads,
       // sectors per track and number of cylinders
       //
       for (index = 0; index < NUMBER_OF_DRIVE_MEDIA_COMBINATIONS; index++) {

            //
            // Walk the table forward until the drive capacity holds all of the
            // data and the bytes per setor are equal
            //

            if (DriveMediaConstants[index].NumberOfHeads == numberOfHeads &&
                DriveMediaConstants[index].MaximumTrack == maximumTrack &&
                DriveMediaConstants[index].SectorsPerTrack ==sectorsPerTrack) {

                ExFreePool(modeData);

                //
                // index is now a drive media combination.  Compare this to
                // the maximum drive media type in the drive media table.
                //

                for (length = 0; length < NUMBER_OF_DRIVE_TYPES; length++) {

                    if (DriveMediaLimits[length].HighestDriveMediaType == index) {
                        return(length);
                    }
                }
                return(DRIVE_TYPE_NONE);
           }
       }

       // If the maximum track is greater than 8 bits then divide the
       // number of tracks by 3 and multiply the number of heads by 3.
       // This is a special case for the 20.8 MB floppy.
       //

       if (!applyFix && maximumTrack >= 0x0100) {
           maximumTrack++;
           maximumTrack /= 3;
           maximumTrack--;
           numberOfHeads *= 3;
       } else {
           ExFreePool(modeData);
           return(DRIVE_TYPE_NONE);
       }

    }

    ExFreePool(modeData);
    return(DRIVE_TYPE_NONE);
}


BOOLEAN
FlCheckFormatParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFORMAT_PARAMETERS FormatParameters
    )

/*++

Routine Description:

    This routine checks the supplied format parameters to make sure that
    they'll work on the drive to be formatted.

Arguments:

    DeviceObject - Pointer to the device object to be formated.

    FormatParameters - a pointer to the caller's parameters for the FORMAT.

Return Value:

    TRUE if parameters are OK.
    FALSE if the parameters are bad.

--*/

{
    PDRIVE_MEDIA_CONSTANTS driveMediaConstants;
    DRIVE_MEDIA_TYPE driveMediaType;
    ULONG index;

    PAGED_CODE();

    //
    // Get the device type.
    //

    index = DetermineDriveType(DeviceObject);

    if (index == DRIVE_TYPE_NONE) {

        //
        // If the determine device type failed then just use the media type
        // and try the parameters.
        //

        driveMediaType = Drive360Media160;

        while (( DriveMediaConstants[driveMediaType].MediaType !=
               FormatParameters->MediaType ) &&
               ( driveMediaType < Drive288Media288) ) {

               driveMediaType++;
        }

    } else {

        //
        // Figure out which entry in the DriveMediaConstants table to use.
        //

        driveMediaType =
            DriveMediaLimits[index].HighestDriveMediaType;

        while ( ( DriveMediaConstants[driveMediaType].MediaType !=
            FormatParameters->MediaType ) &&
            ( driveMediaType > DriveMediaLimits[index].
            LowestDriveMediaType ) ) {

            driveMediaType--;
        }

    }


     // driveMediaType is bounded below by DriveMediaLimits[].LowestDriveMediaType
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:33010) // 33010: Enum used as array index may be negative
#endif
    if ( DriveMediaConstants[driveMediaType].MediaType !=
        FormatParameters->MediaType ) {
        return FALSE;

    } else {

        driveMediaConstants = &DriveMediaConstants[driveMediaType];

        if ( ( FormatParameters->StartHeadNumber >
            (ULONG)( driveMediaConstants->NumberOfHeads - 1 ) ) ||
            ( FormatParameters->EndHeadNumber >
            (ULONG)( driveMediaConstants->NumberOfHeads - 1 ) ) ||
            ( FormatParameters->StartCylinderNumber >
            driveMediaConstants->MaximumTrack ) ||
            ( FormatParameters->EndCylinderNumber >
            driveMediaConstants->MaximumTrack ) ||
            ( FormatParameters->EndCylinderNumber <
            FormatParameters->StartCylinderNumber ) ) {

            return FALSE;

        } else {

            return TRUE;
        }
    }
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma warning(pop)
#endif
}

NTSTATUS
FormatMedia(
    PDEVICE_OBJECT DeviceObject,
    MEDIA_TYPE MediaType
    )
/*++

Routine Description:

    This routine formats the floppy disk.  The entire floppy is formated in
    one shot.

Arguments:

    DeviceObject - Supplies the device object to be tested.

    Irp - Supplies a pointer to the requesting Irp.

    MediaType - Supplies the media type format the device for.

Return Value:

    Returns a status for the operation.

--*/
{
    PVOID modeData;
    PSCSI_REQUEST_BLOCK srb;
    PMODE_FLEXIBLE_DISK_PAGE pageData;
    DRIVE_MEDIA_TYPE driveMediaType;
    PDRIVE_MEDIA_CONSTANTS driveMediaConstants;
    ULONG length;
    NTSTATUS status;

    PAGED_CODE();

    modeData = ExAllocatePool(NonPagedPoolNxCacheAligned, MODE_DATA_SIZE);

    if (modeData == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(modeData, MODE_DATA_SIZE);

    length = ClassModeSense(DeviceObject,
                            modeData,
                            MODE_DATA_SIZE,
                            MODE_PAGE_FLEXIBILE);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {
        ExFreePool(modeData);
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    //
    // Look for the flexible disk mode page.
    //

    pageData = ClassFindModePage( modeData, length, MODE_PAGE_FLEXIBILE, TRUE);

    //
    // Make sure the page is returned and is large enough.
    //

    if ((pageData == NULL) ||
        (pageData->PageLength + 2 <
         (UCHAR)offsetof(MODE_FLEXIBLE_DISK_PAGE, StartWritePrecom))) {

        ExFreePool(modeData);
        return(STATUS_INVALID_DEVICE_REQUEST);

    }

    //
    // Look for a drive media type which matches the requested media type.
    //
    //
    //start from Drive120MMedia120M instead of Drive2080Media2080
    //
    for (driveMediaType = Drive120MMedia120M;
    DriveMediaConstants[driveMediaType].MediaType != MediaType;
    driveMediaType--) {
         if (driveMediaType == Drive360Media160) {

             ExFreePool(modeData);
             return(STATUS_INVALID_PARAMETER);

         }
    }

    driveMediaConstants = &DriveMediaConstants[driveMediaType];

    if ((pageData->NumberOfHeads != driveMediaConstants->NumberOfHeads) ||
        (pageData->SectorsPerTrack != driveMediaConstants->SectorsPerTrack) ||
        ((pageData->NumberOfCylinders[0] != (UCHAR)((driveMediaConstants->MaximumTrack+1) >> 8)) &&
         (pageData->NumberOfCylinders[1] != (UCHAR)driveMediaConstants->MaximumTrack+1)) ||
        (pageData->BytesPerSector[0] != driveMediaConstants->BytesPerSector >> 8 )) {

        //
        // Update the flexible parameters page with the new parameters.
        //

        pageData->NumberOfHeads = driveMediaConstants->NumberOfHeads;
        pageData->SectorsPerTrack = driveMediaConstants->SectorsPerTrack;
        pageData->NumberOfCylinders[0] = (UCHAR)((driveMediaConstants->MaximumTrack+1) >> 8);
        pageData->NumberOfCylinders[1] = (UCHAR)driveMediaConstants->MaximumTrack+1;
        pageData->BytesPerSector[0] = driveMediaConstants->BytesPerSector >> 8;

        //
        // Clear the mode parameter header.
        //

        RtlZeroMemory(modeData, sizeof(MODE_PARAMETER_HEADER));

        //
        // Set the length equal to the length returned for the flexible page.
        //

        length = pageData->PageLength + 2;

        //
        // Copy the page after the mode parameter header.
        //

        RtlMoveMemory((PCHAR) modeData + sizeof(MODE_PARAMETER_HEADER),
                pageData,
                length
                );
            length += sizeof(MODE_PARAMETER_HEADER);


        //
        // Allocate a Srb for the format command.
        //

        srb = ExAllocatePool(NonPagedPoolNx, SCSI_REQUEST_BLOCK_SIZE);

        if (srb == NULL) {

            ExFreePool(modeData);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

        srb->CdbLength = 6;
        srb->Cdb[0] = SCSIOP_MODE_SELECT;
        srb->Cdb[4] = (UCHAR) length;

        //
        // Set the PF bit.
        //

        srb->Cdb[1] |= 0x10;

        //
        // Set timeout value.
        //

        srb->TimeOutValue = 2;

        //
        // Send the mode select data.
        //

        status = ClassSendSrbSynchronous(DeviceObject,
                  srb,
                  modeData,
                  length,
                  TRUE
                  );

        //
        // The mode data not needed any more so free it.
        //

        ExFreePool(modeData);

        if (!NT_SUCCESS(status)) {
            ExFreePool(srb);
            return(status);
        }

    } else {

        //
        // The mode data not needed any more so free it.
        //

        ExFreePool(modeData);

        //
        // Allocate a Srb for the format command.
        //

        srb = ExAllocatePool(NonPagedPoolNx, SCSI_REQUEST_BLOCK_SIZE);

        if (srb == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

    srb->CdbLength = 6;

    srb->Cdb[0] = SCSIOP_FORMAT_UNIT;

    //
    // Set timeout value.
    //

    srb->TimeOutValue = 10 * 60;

    status = ClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     NULL,
                                     0,
                                     FALSE
                                     );
    ExFreePool(srb);

    return(status);

}

VOID
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )
/*++

Routine Description:

   This routine checks the type of error.  If the error indicate the floppy
   controller needs to be reinitialize a command is made to do it.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - Status with which the IRP will be completed.

    Retry - Indication of whether the request will be retried.

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA) fdoExtension->CommonExtension.DriverData;
    PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PSCSI_REQUEST_BLOCK srb;
    LARGE_INTEGER largeInt;
    PCOMPLETION_CONTEXT context;
    PCDB cdb;
    ULONG_PTR alignment;
    ULONG majorFunction;

    UNREFERENCED_PARAMETER(Status);
    UNREFERENCED_PARAMETER(Retry);

    largeInt.QuadPart = 1;

    //
    // Check the status.  The initialization command only needs to be sent
    // if UNIT ATTENTION or LUN NOT READY is returned.
    //

    if (!(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)) {

        //
        // The drive does not require reinitialization.
        //

        return;
    }

    //
    // Reset the drive type.
    //

    diskData->DriveType = DRIVE_TYPE_NONE;
    diskData->IsDMF = FALSE;

    fdoExtension->DiskGeometry.MediaType = Unknown;

    if (fdoExtension->AdapterDescriptor->BusType == BusTypeUsb) {

        // FLPYDISK.SYS never returns a non-zero value for the ChangeCount
        // on an IOCTL_DISK_CHECK_VERIFY.  Some things seem to work better
        // if we do the same.  In particular, FatVerifyVolume() can exit between
        // the IOCTL_DISK_CHECK_VERIFY and the IOCTL_DISK_GET_DRIVE_GEOMETRY
        // if a non-zero ChangeCount is returned, and this appears to cause
        // issues formatting unformatted media in some situations.
        //
        // This is something that should probably be revisited at some point.
        //
        fdoExtension->MediaChangeCount = 0;

        if (((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_UNIT_ATTENTION) &&
            (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_MEDIUM_CHANGED)) {

            struct _START_STOP *startStopCdb;

            DebugPrint((2,"Sending SCSIOP_START_STOP_UNIT\n"));

            context = ExAllocatePool(NonPagedPoolNx,
                                     sizeof(COMPLETION_CONTEXT));

            if (context == NULL) {

                return;
            }
#if (NTDDI_VERSION >= NTDDI_WIN8)
            srb = &context->Srb.Srb;
#else
            srb = &context->Srb;
#endif

            RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

            srb->SrbFlags = SRB_FLAGS_DISABLE_AUTOSENSE;

            srb->CdbLength = 6;

            startStopCdb = (struct _START_STOP *)srb->Cdb;

            startStopCdb->OperationCode = SCSIOP_START_STOP_UNIT;
            startStopCdb->Start = 1;

            // A Start Stop Unit request has no transfer buffer.
            // Set the request to IRP_MJ_FLUSH_BUFFERS when calling
            // IoBuildAsynchronousFsdRequest() so that it ignores
            // the buffer pointer and buffer length parameters.
            //
            majorFunction = IRP_MJ_FLUSH_BUFFERS;

        } else if ((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_MEDIUM_ERROR) {

            // Return ERROR_UNRECOGNIZED_MEDIA instead of
            // STATUS_DEVICE_DATA_ERROR to make shell happy.
            //
            *Status = STATUS_UNRECOGNIZED_MEDIA;
            return;

        } else {

            return;
        }

#ifndef __REACTOS__
    } else if (((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_NOT_READY) &&
             senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_INIT_COMMAND_REQUIRED ||
             (senseBuffer->SenseKey & 0xf) == SCSI_SENSE_UNIT_ATTENTION) {
#else
    } else if ((((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_NOT_READY) &&
             senseBuffer->AdditionalSenseCodeQualifier == SCSI_SENSEQ_INIT_COMMAND_REQUIRED) ||
             (senseBuffer->SenseKey & 0xf) == SCSI_SENSE_UNIT_ATTENTION) {
#endif

        ULONG sizeNeeded;
        ULONG tmpSize;
        BOOLEAN overFlow;

        DebugPrint((1, "ScsiFlopProcessError: Reinitializing the floppy.\n"));

        //
        // Send the special mode sense command to enable writes on the
        // floptical drive.
        //

        alignment = DeviceObject->AlignmentRequirement ?
            DeviceObject->AlignmentRequirement : 1;

        sizeNeeded = 0;
        overFlow = TRUE;
        if (SUCCEEDED(ULongAdd(sizeof(COMPLETION_CONTEXT), 0x2a, &tmpSize))) {

            if (SUCCEEDED(ULongAdd(tmpSize, (ULONG) alignment, &sizeNeeded))) {
                overFlow = FALSE;
            }
        }

        context = NULL;

        if (!overFlow) {
            context = ExAllocatePool(NonPagedPoolNx, sizeNeeded);
        }

        if (context == NULL) {

            //
            // If there is not enough memory to fulfill this request,
            // simply return. A subsequent retry will fail and another
            // chance to start the unit.
            //

            return;
        }

#if (NTDDI_VERSION >= NTDDI_WIN8)
        srb = &context->Srb.Srb;
#else
        srb = &context->Srb;
#endif

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

        //
        // Set the transfer length.
        //

        srb->DataTransferLength = 0x2a;
        srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_AUTOSENSE | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

        //
        // The data buffer must be aligned.
        //

        srb->DataBuffer = (PVOID) (((ULONG_PTR) (context + 1) + (alignment - 1)) &
            ~(alignment - 1));


        //
        // Build the start unit CDB.
        //

        srb->CdbLength = 6;
        cdb = (PCDB)srb->Cdb;
        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.PageCode = 0x2e;
        cdb->MODE_SENSE.AllocationLength = 0x2a;

        majorFunction = IRP_MJ_READ;

    } else {

        return;
    }

    context->DeviceObject = DeviceObject;

    //
    // Write length to SRB.
    //

    srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->TimeOutValue = fdoExtension->TimeOutValue;

    //
    // Build the asynchronous request
    // to be sent to the port driver.
    //

    irp = IoBuildAsynchronousFsdRequest(majorFunction,
                       DeviceObject,
                       srb->DataBuffer,
                       srb->DataTransferLength,
                       &largeInt,
                       NULL);

    if(irp == NULL) {
        ExFreePool(context);
        return;
    }


    IoSetCompletionRoutine(irp,
           (PIO_COMPLETION_ROUTINE)ClassAsynchronousCompletion,
           context,
           TRUE,
           TRUE,
           TRUE);

    ClassAcquireRemoveLock(DeviceObject, irp);

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;

    srb->OriginalRequest = irp;

    //
    // Save SRB address in next stack for port driver.
    //

    irpStack->Parameters.Others.Argument1 = (PVOID)srb;

    //
    // Can't release the remove lock yet - let ClassAsynchronousCompletion
    // take care of that for us.
    //

    (VOID)IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

    return;
}

NTSTATUS
FlopticalFormatMedia(
    PDEVICE_OBJECT DeviceObject,
    PFORMAT_PARAMETERS Format
    )
/*++

Routine Description:

    This routine is used to do perform a format tracks for the 20.8 MB
    floppy.  Because the device does not support format tracks and the full
    format takes a long time a write of zeros is done instead.

Arguments:

    DeviceObject - Supplies the device object to be tested.

    Format - Supplies the format parameters.

Return Value:

    Returns a status for the operation.

--*/
{
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    KEVENT event;
    LARGE_INTEGER offset;
    ULONG length;
    PVOID buffer;
    PDRIVE_MEDIA_CONSTANTS driveMediaConstants;
    NTSTATUS status;

    PAGED_CODE();

    driveMediaConstants = &DriveMediaConstants[Drive2080Media2080];

    //
    // Calculate the length of the buffer.
    //

    length = ((Format->EndCylinderNumber - Format->StartCylinderNumber) *
        driveMediaConstants->NumberOfHeads +
        Format->EndHeadNumber - Format->StartHeadNumber + 1) *
        driveMediaConstants->SectorsPerTrack *
        driveMediaConstants->BytesPerSector;

    buffer = ExAllocatePool(NonPagedPoolNxCacheAligned, length);

    if (buffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(buffer, length);

    offset.QuadPart =
    (Format->StartCylinderNumber * driveMediaConstants->NumberOfHeads +
    Format->StartHeadNumber) * driveMediaConstants->SectorsPerTrack *
    driveMediaConstants->BytesPerSector;

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build the synchronous request with data transfer.
    //

    irp = IoBuildSynchronousFsdRequest(
       IRP_MJ_WRITE,
       DeviceObject,
       buffer,
       length,
       &offset,
       &event,
       &ioStatus);

    if (irp != NULL) {
        status = IoCallDriver(DeviceObject, irp);

        if (status == STATUS_PENDING) {

            //
            // Wait for the request to complete if necessary.
            //

            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        }

        //
        // If the call  driver suceeded then set the status to the status block.
        //

        if (NT_SUCCESS(status)) {
            status = ioStatus.Status;
        }
    } else {
       status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ExFreePool(buffer);

    return(status);

}


NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )
/*++

Routine Description:

    This routine is responsible for releasing any resources in use by the
    sfloppy driver.  This routine is called
    when all outstanding requests have been completed and the driver has
    disappeared - no requests may be issued to the lower drivers.

Arguments:

    DeviceObject - the device object being removed

    Type - the type of remove operation (QUERY, REMOVE or CANCEL)

Return Value:

    for a query - success if the device can be removed or a failure code
                  indiciating why not.

    for a remove or cancel - STATUS_SUCCESS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension =
        DeviceObject->DeviceExtension;
    PDISK_DATA diskData = deviceExtension->CommonExtension.DriverData;
    NTSTATUS status;

    PAGED_CODE();

    if((Type == IRP_MN_QUERY_REMOVE_DEVICE) ||
       (Type == IRP_MN_CANCEL_REMOVE_DEVICE)) {
        return STATUS_SUCCESS;
    }

    if (Type == IRP_MN_REMOVE_DEVICE){
        if(deviceExtension->DeviceDescriptor) {
            ExFreePool(deviceExtension->DeviceDescriptor);
            deviceExtension->DeviceDescriptor = NULL;
        }

        if(deviceExtension->AdapterDescriptor) {
            ExFreePool(deviceExtension->AdapterDescriptor);
            deviceExtension->AdapterDescriptor = NULL;
        }

        if(deviceExtension->SenseData) {
            ExFreePool(deviceExtension->SenseData);
            deviceExtension->SenseData = NULL;
        }

        ClassDeleteSrbLookasideList(&deviceExtension->CommonExtension);
    }

    if(diskData->FloppyInterfaceString.Buffer != NULL) {
        
        status = IoSetDeviceInterfaceState(
                   &(diskData->FloppyInterfaceString),
                   FALSE);

        if (!NT_SUCCESS(status)) {
            // Failed to disable device interface during removal. Not a fatal error.
            DebugPrint((1, "ScsiFlopRemoveDevice: Unable to set device "
                           "interface state to FALSE for fdo %p "
                           "[%08lx]\n",
                        DeviceObject, status));
        }

        RtlFreeUnicodeString(&(diskData->FloppyInterfaceString));
        RtlInitUnicodeString(&(diskData->FloppyInterfaceString), NULL);
    }

    if(Type == IRP_MN_REMOVE_DEVICE) {
        IoGetConfigurationInformation()->FloppyCount--;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
#ifdef __REACTOS__
NTAPI
#endif
ScsiFlopStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Type);

    return STATUS_SUCCESS;
}


NTSTATUS
USBFlopGetMediaTypes(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
/*++

Routine Description:

    This routines determines the current or default geometry of the drive
    for IOCTL_DISK_GET_DRIVE_GEOMETRY, or all currently supported geometries
    of the drive (which is determined by its currently inserted media) for
    IOCTL_DISK_GET_MEDIA_TYPES.

    The returned geometries are determined by issuing a Read Format Capacities
    request and then matching the returned {Number of Blocks, Block Length}
    pairs in a table of known floppy geometries.

Arguments:

    DeviceObject - Supplies the device object.

    Irp - A IOCTL_DISK_GET_DRIVE_GEOMETRY or a IOCTL_DISK_GET_MEDIA_TYPES Irp.
          If NULL, the device geometry is updated with the current device
          geometry.

Return Value:

    Status is returned.

--*/
    PFUNCTIONAL_DEVICE_EXTENSION    fdoExtension;
    PIO_STACK_LOCATION              irpStack;
    ULONG                           ioControlCode;
    PDISK_GEOMETRY                  outputBuffer;
    PDISK_GEOMETRY                  outputBufferEnd;
    ULONG                           outputBufferLength;
    PSCSI_REQUEST_BLOCK             srb;
    PVOID                           dataBuffer;
    ULONG                           dataTransferLength;
    struct _READ_FORMATTED_CAPACITIES *cdb;
    PFORMATTED_CAPACITY_LIST        capList;
    NTSTATUS                        status;

    PAGED_CODE();

    fdoExtension = DeviceObject->DeviceExtension;

    if (Irp != NULL) {

        // Get the Irp parameters
        //
        irpStack = IoGetCurrentIrpStackLocation(Irp);

        ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

        Irp->IoStatus.Information = 0;

        outputBuffer = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;

        outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

        if (outputBufferLength < sizeof(DISK_GEOMETRY))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        // Pointer arithmetic to allow multiple DISK_GEOMETRY's to be returned.
        // Rounds BufferEnd down to integral multiple of DISK_GEOMETRY structs.
        //
        outputBufferEnd = outputBuffer +
                          outputBufferLength / sizeof(DISK_GEOMETRY);

    } else {

        // No Irp to return the result in, just update the current geometry
        // in the device extension.
        //
        ioControlCode = IOCTL_DISK_GET_DRIVE_GEOMETRY;

        outputBuffer        = NULL;

        outputBufferEnd     = NULL;

        outputBufferLength  = 0;
    }

    if (ioControlCode == IOCTL_DISK_GET_DRIVE_GEOMETRY) {

        fdoExtension->DiskGeometry.MediaType = Unknown;

        status = ClassReadDriveCapacity(DeviceObject);

        if (!NT_SUCCESS(status))
        {
            // If the media is not recongized, we want to return the default
            // geometry so that the media can be formatted.  Unrecognized media
            // causes SCSI_SENSE_MEDIUM_ERROR, which gets reported as
            // STATUS_DEVICE_DATA_ERROR.  Ignore these errors, but return other
            // errors, such as STATUS_NO_MEDIA_IN_DEVICE.
            //
            if (status != STATUS_UNRECOGNIZED_MEDIA)
            {
                DebugPrint((2,"IOCTL_DISK_GET_DRIVE_GEOMETRY returns %08X\n", status));

                return status;
            }
        }
    }

    // Allocate an SRB for the SCSIOP_READ_FORMATTED_CAPACITY request
    //
    srb = ExAllocatePool(NonPagedPoolNx, SCSI_REQUEST_BLOCK_SIZE);

    if (srb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate a transfer buffer for the SCSIOP_READ_FORMATTED_CAPACITY request
    // The length of the returned descriptor array is limited to a byte field
    // in the capacity list header.
    //
    dataTransferLength = sizeof(FORMATTED_CAPACITY_LIST) +
                         31 * sizeof(FORMATTED_CAPACITY_DESCRIPTOR);

    ASSERT(dataTransferLength < 0x100);

    dataBuffer = ExAllocatePool(NonPagedPoolNx, dataTransferLength);

    if (dataBuffer == NULL)
    {
        ExFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the SRB and CDB
    //
    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

    RtlZeroMemory(dataBuffer, dataTransferLength);

    srb->CdbLength = sizeof(struct _READ_FORMATTED_CAPACITIES);

    srb->TimeOutValue = fdoExtension->TimeOutValue;

    cdb = (struct _READ_FORMATTED_CAPACITIES *)srb->Cdb;

    cdb->OperationCode = SCSIOP_READ_FORMATTED_CAPACITY;
    cdb->AllocationLength[1] = (UCHAR)dataTransferLength;

    //
    // Send down the SCSIOP_READ_FORMATTED_CAPACITY request
    //
    status = ClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     dataBuffer,
                                     dataTransferLength,
                                     FALSE);

    capList = (PFORMATTED_CAPACITY_LIST)dataBuffer;

    // If we don't get as much data as requested, it is not an error.
    //
    if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN)
    {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status) &&
        srb->DataTransferLength >= sizeof(FORMATTED_CAPACITY_LIST) &&
        capList->CapacityListLength &&
        capList->CapacityListLength % sizeof(FORMATTED_CAPACITY_DESCRIPTOR) == 0)
    {
        ULONG   NumberOfBlocks;
        ULONG   BlockLength;
        ULONG   count;
        ULONG   i, j;
        LONG    currentGeometry;
        BOOLEAN capacityMatches[FLOPPY_CAPACITIES];

        // Subtract the size of the Capacity List Header to get
        // just the size of the Capacity List Descriptor array.
        //
        srb->DataTransferLength -= sizeof(FORMATTED_CAPACITY_LIST);

        // Only look at the Capacity List Descriptors that were actually
        // returned.
        //
        if (srb->DataTransferLength < capList->CapacityListLength)
        {
            count = srb->DataTransferLength /
                    sizeof(FORMATTED_CAPACITY_DESCRIPTOR);
        }
        else
        {
            count = capList->CapacityListLength /
                    sizeof(FORMATTED_CAPACITY_DESCRIPTOR);
        }

        // Updated only if a match is found for the first Capacity List
        // Descriptor returned by the device.
        //
        currentGeometry = -1;

        // Initialize the array of capacities that hit a match.
        //
        RtlZeroMemory(capacityMatches, sizeof(capacityMatches));

        // Iterate over each Capacity List Descriptor returned from the device
        // and record matching capacities in the capacity match array.
        //
        for (i = 0; i < count; i++)
        {
            NumberOfBlocks = (capList->Descriptors[i].NumberOfBlocks[0] << 24) +
                             (capList->Descriptors[i].NumberOfBlocks[1] << 16) +
                             (capList->Descriptors[i].NumberOfBlocks[2] <<  8) +
                             (capList->Descriptors[i].NumberOfBlocks[3]);

            BlockLength = (capList->Descriptors[i].BlockLength[0] << 16) +
                          (capList->Descriptors[i].BlockLength[1] <<  8) +
                          (capList->Descriptors[i].BlockLength[2]);

            // Given the {NumberOfBlocks, BlockLength} from this Capacity List
            // Descriptor, find a matching entry in FloppyCapacities[].
            //
            for (j = 0; j < FLOPPY_CAPACITIES; j++)
            {
                if (NumberOfBlocks == FloppyCapacities[j].NumberOfBlocks &&
                    BlockLength    == FloppyCapacities[j].BlockLength)
                {
                    // A matching capacity was found, record it.
                    //
                    capacityMatches[j] = TRUE;

                    // A match was found for the first Capacity List
                    // Descriptor returned by the device.
                    //
                    if (i == 0)
                    {
                        currentGeometry = j;
                    }
                } else if ((capList->Descriptors[i].Valid) &&
                           (BlockLength == FloppyCapacities[j].BlockLength)) {

                    ULONG inx;
                    ULONG mediaInx;

                    //
                    // Check if this is 32MB media type. 32MB media
                    // reports variable NumberOfBlocks. So we cannot
                    // use that to determine the drive type
                    //
                    inx = DetermineDriveType(DeviceObject);
                    if (inx != DRIVE_TYPE_NONE) {
                        mediaInx = DriveMediaLimits[inx].HighestDriveMediaType;
                        if ((DriveMediaConstants[mediaInx].MediaType)
                             == F3_32M_512) {
                            capacityMatches[j] = TRUE;

                            if (i == 0) {
                                currentGeometry = j;
                            }
                        }
                    }
                }
            }
        }

        // Default status is STATUS_UNRECOGNIZED_MEDIA, unless we return
        // either STATUS_SUCCESS or STATUS_BUFFER_OVERFLOW.
        //
        status = STATUS_UNRECOGNIZED_MEDIA;

        if (ioControlCode == IOCTL_DISK_GET_DRIVE_GEOMETRY) {

            if (currentGeometry != -1)
            {
                // Update the current device geometry
                //
                fdoExtension->DiskGeometry = FloppyGeometries[currentGeometry];

                //
                // Calculate sector to byte shift.
                //

                WHICH_BIT(fdoExtension->DiskGeometry.BytesPerSector,
                          fdoExtension->SectorShift);

                fdoExtension->CommonExtension.PartitionLength.QuadPart =
                    (LONGLONG)FloppyCapacities[currentGeometry].NumberOfBlocks *
                    FloppyCapacities[currentGeometry].BlockLength;

                DebugPrint((2,"geometry  is: %3d %2d %d %2d %4d  %2d  %08X\n",
                            fdoExtension->DiskGeometry.Cylinders.LowPart,
                            fdoExtension->DiskGeometry.MediaType,
                            fdoExtension->DiskGeometry.TracksPerCylinder,
                            fdoExtension->DiskGeometry.SectorsPerTrack,
                            fdoExtension->DiskGeometry.BytesPerSector,
                            fdoExtension->SectorShift,
                            fdoExtension->CommonExtension.PartitionLength.LowPart));

                // Return the current device geometry
                //
                if (Irp != NULL)
                {
                    *outputBuffer = FloppyGeometries[currentGeometry];

                    Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
                }

                status = STATUS_SUCCESS;
            }

        } else {

            // Iterate over the capacities and return the geometry
            // corresponding to each matching Capacity List Descriptor
            // returned from the device.
            //
            // The resulting list should be in sorted ascending order,
            // assuming that the FloppyGeometries[] array is in sorted
            // ascending order.
            //
            for (i = 0; i < FLOPPY_CAPACITIES; i++)
            {
                if (capacityMatches[i] && FloppyCapacities[i].CanFormat)
                {
                    if (outputBuffer < outputBufferEnd)
                    {
                        *outputBuffer++ = FloppyGeometries[i];

                        Irp->IoStatus.Information += sizeof(DISK_GEOMETRY);

                        DebugPrint((2,"geometry    : %3d %2d %d %2d %4d\n",
                                    FloppyGeometries[i].Cylinders.LowPart,
                                    FloppyGeometries[i].MediaType,
                                    FloppyGeometries[i].TracksPerCylinder,
                                    FloppyGeometries[i].SectorsPerTrack,
                                    FloppyGeometries[i].BytesPerSector));

                        status = STATUS_SUCCESS;
                    }
                    else
                    {
                        // We ran out of output buffer room before we ran out
                        // geometries to return.
                        //
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
        }
    }
    else if (NT_SUCCESS(status))
    {
        // The SCSIOP_READ_FORMATTED_CAPACITY request was successful, but
        // returned data does not appear valid.
        //
        status = STATUS_UNSUCCESSFUL;
    }

    ExFreePool(dataBuffer);
    ExFreePool(srb);

    return status;
}


NTSTATUS
USBFlopFormatTracks(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
/*++

Routine Description:

    This routines formats the specified tracks.  If multiple tracks are
    specified, each is formatted with a separate Format Unit request.

Arguments:

    DeviceObject - Supplies the device object.

    Irp - A IOCTL_DISK_FORMAT_TRACKS Irp.

Return Value:

    Status is returned.

--*/
    PFUNCTIONAL_DEVICE_EXTENSION    fdoExtension;
    PIO_STACK_LOCATION              irpStack;
    PFORMAT_PARAMETERS              formatParameters;
    PDISK_GEOMETRY                  geometry;
    PFORMATTED_CAPACITY             capacity;
    PSCSI_REQUEST_BLOCK             srb;
    PFORMAT_UNIT_PARAMETER_LIST     parameterList;
    PCDB12FORMAT                    cdb;
    ULONG                           i;
    ULONG                           cylinder, head;
    NTSTATUS                        status = STATUS_SUCCESS;

    PAGED_CODE();

    fdoExtension = DeviceObject->DeviceExtension;

    // Get the Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(FORMAT_PARAMETERS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    formatParameters = (PFORMAT_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;

    // Find the geometry / capacity entries corresponding to the format
    // parameters MediaType
    //
    geometry = NULL;
    capacity = NULL;

    for (i=0; i<FLOPPY_CAPACITIES; i++)
    {
        if (FloppyGeometries[i].MediaType == formatParameters->MediaType)
        {
            geometry = &FloppyGeometries[i];
            capacity = &FloppyCapacities[i];

            break;
        }
    }

    if (geometry == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Check if the format parameters are valid
    //
    if ((formatParameters->StartCylinderNumber >
         geometry->Cylinders.LowPart - 1)       ||

        (formatParameters->EndCylinderNumber >
         geometry->Cylinders.LowPart - 1)       ||

        (formatParameters->StartHeadNumber >
         geometry->TracksPerCylinder - 1)       ||

        (formatParameters->EndHeadNumber >
         geometry->TracksPerCylinder - 1)       ||

        (formatParameters->StartCylinderNumber >
         formatParameters->EndCylinderNumber)   ||

        (formatParameters->StartHeadNumber >
         formatParameters->EndHeadNumber))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Don't low level format LS-120 media, Imation says it's best to not
    // do this.
    //
    if ((formatParameters->MediaType == F3_120M_512) ||
        (formatParameters->MediaType == F3_240M_512) ||
        (formatParameters->MediaType == F3_32M_512))
    {
        return STATUS_SUCCESS;
    }

    // Allocate an SRB for the SCSIOP_FORMAT_UNIT request
    //
    srb = ExAllocatePool(NonPagedPoolNx, SCSI_REQUEST_BLOCK_SIZE);

    if (srb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate a transfer buffer for the SCSIOP_FORMAT_UNIT parameter list
    //
    parameterList = ExAllocatePool(NonPagedPoolNx,
                                   sizeof(FORMAT_UNIT_PARAMETER_LIST));

    if (parameterList == NULL)
    {
        ExFreePool(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize the parameter list
    //
    RtlZeroMemory(parameterList, sizeof(FORMAT_UNIT_PARAMETER_LIST));

    parameterList->DefectListHeader.SingleTrack = 1;
    parameterList->DefectListHeader.DisableCert = 1;  // TEAC requires this set
    parameterList->DefectListHeader.FormatOptionsValid = 1;
    parameterList->DefectListHeader.DefectListLengthLsb = 8;

    parameterList->FormatDescriptor.NumberOfBlocks[0] =
        (UCHAR)((capacity->NumberOfBlocks >> 24) & 0xFF);

    parameterList->FormatDescriptor.NumberOfBlocks[1] =
        (UCHAR)((capacity->NumberOfBlocks >> 16) & 0xFF);

    parameterList->FormatDescriptor.NumberOfBlocks[2] =
        (UCHAR)((capacity->NumberOfBlocks >> 8) & 0xFF);

    parameterList->FormatDescriptor.NumberOfBlocks[3] =
        (UCHAR)(capacity->NumberOfBlocks & 0xFF);

    parameterList->FormatDescriptor.BlockLength[0] =
        (UCHAR)((capacity->BlockLength >> 16) & 0xFF);

    parameterList->FormatDescriptor.BlockLength[1] =
        (UCHAR)((capacity->BlockLength >> 8) & 0xFF);

    parameterList->FormatDescriptor.BlockLength[2] =
        (UCHAR)(capacity->BlockLength & 0xFF);


    for (cylinder =  formatParameters->StartCylinderNumber;
         cylinder <= formatParameters->EndCylinderNumber;
         cylinder++)
    {
        for (head =  formatParameters->StartHeadNumber;
             head <= formatParameters->EndHeadNumber;
             head++)
        {
            // Initialize the SRB and CDB
            //
            RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

            srb->CdbLength = sizeof(CDB12FORMAT);

            srb->TimeOutValue = fdoExtension->TimeOutValue;

            cdb = (PCDB12FORMAT)srb->Cdb;

            cdb->OperationCode = SCSIOP_FORMAT_UNIT;
            cdb->DefectListFormat = 7;
            cdb->FmtData = 1;
            cdb->TrackNumber = (UCHAR)cylinder;
            cdb->ParameterListLengthLsb = sizeof(FORMAT_UNIT_PARAMETER_LIST);

            parameterList->DefectListHeader.Side = (UCHAR)head;

            //
            // Send down the SCSIOP_FORMAT_UNIT request
            //
            status = ClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             parameterList,
                                             sizeof(FORMAT_UNIT_PARAMETER_LIST),
                                             TRUE);

            if (!NT_SUCCESS(status))
            {
                break;
            }
        }
        if (!NT_SUCCESS(status))
        {
            break;
        }
    }

    if (NT_SUCCESS(status) && formatParameters->StartCylinderNumber == 0)
    {
        // Update the device geometry
        //

        DebugPrint((2,"geometry was: %3d %2d %d %2d %4d  %2d  %08X\n",
                    fdoExtension->DiskGeometry.Cylinders.LowPart,
                    fdoExtension->DiskGeometry.MediaType,
                    fdoExtension->DiskGeometry.TracksPerCylinder,
                    fdoExtension->DiskGeometry.SectorsPerTrack,
                    fdoExtension->DiskGeometry.BytesPerSector,
                    fdoExtension->SectorShift,
                    fdoExtension->CommonExtension.PartitionLength.LowPart));

        fdoExtension->DiskGeometry = *geometry;

        //
        // Calculate sector to byte shift.
        //

        WHICH_BIT(fdoExtension->DiskGeometry.BytesPerSector,
                  fdoExtension->SectorShift);

        fdoExtension->CommonExtension.PartitionLength.QuadPart =
            (LONGLONG)capacity->NumberOfBlocks *
            capacity->BlockLength;

        DebugPrint((2,"geometry  is: %3d %2d %d %2d %4d  %2d  %08X\n",
                    fdoExtension->DiskGeometry.Cylinders.LowPart,
                    fdoExtension->DiskGeometry.MediaType,
                    fdoExtension->DiskGeometry.TracksPerCylinder,
                    fdoExtension->DiskGeometry.SectorsPerTrack,
                    fdoExtension->DiskGeometry.BytesPerSector,
                    fdoExtension->SectorShift,
                    fdoExtension->CommonExtension.PartitionLength.LowPart));
    }

    // Free everything we allocated
    //
    ExFreePool(parameterList);
    ExFreePool(srb);

    return status;
}
