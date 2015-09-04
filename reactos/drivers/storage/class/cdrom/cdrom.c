/*
 * PROJECT:         ReactOS Storage Stack
 * LICENSE:         DDK - see license.txt in the root dir
 * FILE:            drivers/storage/cdrom/cdrom.c
 * PURPOSE:         CDROM driver
 * PROGRAMMERS:     Based on a source code sample from Microsoft NT4 DDK
 */

#include "precomp.h"

#include <ntddk.h>
#include <scsi.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <include/class2.h>
#include <stdio.h>

//#define NDEBUG
#include <debug.h>

#define CDB12GENERIC_LENGTH 12

typedef struct _XA_CONTEXT {

    //
    // Pointer to the device object.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Pointer to the original request when
    // a mode select must be sent.
    //

    PIRP OriginalRequest;

    //
    // Pointer to the mode select srb.
    //

    PSCSI_REQUEST_BLOCK Srb;
} XA_CONTEXT, *PXA_CONTEXT;

typedef struct _ERROR_RECOVERY_DATA {
    MODE_PARAMETER_HEADER   Header;
    MODE_PARAMETER_BLOCK BlockDescriptor;
    MODE_READ_RECOVERY_PAGE ReadRecoveryPage;
} ERROR_RECOVERY_DATA, *PERROR_RECOVERY_DATA;

typedef struct _ERROR_RECOVERY_DATA10 {
    MODE_PARAMETER_HEADER10 Header10;
    MODE_PARAMETER_BLOCK BlockDescriptor10;
    MODE_READ_RECOVERY_PAGE ReadRecoveryPage10;
} ERROR_RECOVERY_DATA10, *PERROR_RECOVERY_DATA10;

//
// CdRom specific addition to device extension.
//

typedef struct _CDROM_DATA {

    //
    // Indicates whether an audio play operation
    // is currently being performed.
    //

    BOOLEAN PlayActive;

    //
    // Indicates whether the blocksize used for user data
    // is 2048 or 2352.
    //

    BOOLEAN RawAccess;

    //
    // Indicates whether 6 or 10 byte mode sense/select
    // should be used.
    //

    USHORT XAFlags;

    //
    // Storage for the error recovery page. This is used
    // as an easy method to switch block sizes.
    //

    union {
        ERROR_RECOVERY_DATA u1;
        ERROR_RECOVERY_DATA10 u2;
    };


    //
    // Pointer to the original irp for the raw read.
    //

    PIRP SavedReadIrp;

    //
    // Used to protect accesses to the RawAccess flag.
    //

    KSPIN_LOCK FormSpinLock;

    //
    // Even if media change support is requested, there are some devices
    // that are not supported.  This flag will indicate that such a device
    // is present when it is FALSE.
    //

    BOOLEAN MediaChangeSupported;

    //
    // The media change event is being supported.  The media change timer
    // should be running whenever this is true.
    //

    BOOLEAN MediaChange;

    //
    // The timer value to support media change events.  This is a countdown
    // value used to determine when to poll the device for a media change.
    // The max value for the timer is 255 seconds.
    //

    UCHAR MediaChangeCountDown;

#if DBG
    //
    // Second timer to keep track of how long the media change IRP has been
    // in use.  If this value exceeds the timeout (#defined) then we should
    // print out a message to the user and set the MediaChangeIrpLost flag
    //

    SHORT MediaChangeIrpTimeInUse;

    //
    // Set by CdRomTickHandler when we determine that the media change irp has
    // been lost
    //

    BOOLEAN MediaChangeIrpLost;
#endif

    UCHAR PadReserve; // use this for new flags.

    //
    // An IRP is allocated and kept for the duration that media change
    // detection is in effect.  If this is NULL and MediaChange is TRUE,
    // the detection is in progress.  This should always be NULL when
    // MediaChange is FALSE.
    //

    PIRP MediaChangeIrp;

    //
    // The timer work list is a collection of IRPS that are prepared for
    // submission, but need to allow some time to pass before they are
    // run.
    //

    LIST_ENTRY TimerIrpList;
    KSPIN_LOCK TimerIrpSpinLock;

} CDROM_DATA, *PCDROM_DATA;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION) + sizeof(CDROM_DATA)
#define SCSI_CDROM_TIMEOUT          10
#define SCSI_CHANGER_BONUS_TIMEOUT  10
#define HITACHI_MODE_DATA_SIZE      12
#define MODE_DATA_SIZE              64
#define RAW_SECTOR_SIZE           2352
#define COOKED_SECTOR_SIZE        2048
#define MEDIA_CHANGE_DEFAULT_TIME    4
#define CDROM_SRB_LIST_SIZE          4


#if DBG

//
// Used to detect the loss of the autorun irp.  The driver prints out a message
// (debug level 0) if this timeout ever occurs
//
#define MEDIA_CHANGE_TIMEOUT_TIME  300

#endif

#define PLAY_ACTIVE(DeviceExtension) (((PCDROM_DATA)(DeviceExtension + 1))->PlayActive)

#define MSF_TO_LBA(Minutes,Seconds,Frames) \
                (ULONG)((60 * 75 * (Minutes)) + (75 * (Seconds)) + ((Frames) - 150))

#define LBA_TO_MSF(Lba,Minutes,Seconds,Frames)               \
{                                                            \
    (Minutes) = (UCHAR)(Lba  / (60 * 75));                   \
    (Seconds) = (UCHAR)((Lba % (60 * 75)) / 75);             \
    (Frames)  = (UCHAR)((Lba % (60 * 75)) % 75);             \
}

#define DEC_TO_BCD(x) (((x / 10) << 4) + (x % 10))

//
// Define flags for XA, CDDA, and Mode Select/Sense
//

#define XA_USE_6_BYTE             0x01
#define XA_USE_10_BYTE            0x02
#define XA_USE_READ_CD            0x04
#define XA_NOT_SUPPORTED          0x08

#define PLEXTOR_CDDA              0x10
#define NEC_CDDA                  0x20

//
// Sector types for READ_CD
//

#define ANY_SECTOR                0
#define CD_DA_SECTOR              1
#define YELLOW_MODE1_SECTOR       2
#define YELLOW_MODE2_SECTOR       3
#define FORM2_MODE1_SECTOR        4
#define FORM2_MODE2_SECTOR        5


#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'CscS')
#endif

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

BOOLEAN
NTAPI
ScsiCdRomFindDevices(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN PCLASS_INIT_DATA InitializationData,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber
    );

NTSTATUS
NTAPI
ScsiCdRomOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NTAPI
ScsiCdRomReadVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NTAPI
ScsiCdRomSwitchMode(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN PIRP  OriginalRequest
    );

NTSTATUS
NTAPI
CdRomDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

IO_COMPLETION_ROUTINE CdRomDeviceControlCompletion;
NTSTATUS
NTAPI
CdRomDeviceControlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

IO_COMPLETION_ROUTINE CdRomSetVolumeIntermediateCompletion;
NTSTATUS
NTAPI
CdRomSetVolumeIntermediateCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

IO_COMPLETION_ROUTINE CdRomSwitchModeCompletion;
NTSTATUS
NTAPI
CdRomSwitchModeCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

IO_COMPLETION_ROUTINE CdRomXACompletion;
NTSTATUS
NTAPI
CdRomXACompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

IO_COMPLETION_ROUTINE CdRomClassIoctlCompletion;
NTSTATUS
NTAPI
CdRomClassIoctlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
NTAPI
ScsiCdRomStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NTAPI
CdRomTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

BOOLEAN
NTAPI
CdRomCheckRegistryForMediaChangeValue(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber
    );

NTSTATUS
NTAPI
CdRomUpdateCapacity(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP IrpToComplete,
    IN OPTIONAL PKEVENT IoctlEvent
    );

NTSTATUS
NTAPI
CreateCdRomDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber,
    IN PULONG DeviceCount,
    PIO_SCSI_CAPABILITIES PortCapabilities,
    IN PSCSI_INQUIRY_DATA LunInfo,
    IN PCLASS_INIT_DATA   InitializationData,
    IN PUNICODE_STRING    RegistryPath
    );

VOID
NTAPI
ScanForSpecial(
    PDEVICE_OBJECT DeviceObject,
    PINQUIRYDATA InquiryData,
    PIO_SCSI_CAPABILITIES PortCapabilities
    );

BOOLEAN
NTAPI
CdRomIsPlayActive(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
NTAPI
HitachProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

IO_COMPLETION_ROUTINE ToshibaProcessErrorCompletion;
VOID
NTAPI
ToshibaProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

BOOLEAN
NTAPI
IsThisAnAtapiChanger(
    IN  PDEVICE_OBJECT DeviceObject,
    OUT PULONG         DiscsPresent
    );

BOOLEAN
NTAPI
IsThisASanyo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  UCHAR          PathId,
    IN  UCHAR          TargetId
    );

BOOLEAN
NTAPI
IsThisAMultiLunDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT PortDeviceObject
    );

VOID
NTAPI
CdRomCreateNamedEvent(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG DeviceNumber
    );

#ifdef _PPC_
NTSTATUS
FindScsiAdapter (
    IN HANDLE KeyHandle,
    IN UNICODE_STRING ScsiUnicodeString[],
    OUT PUCHAR IntermediateController
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, ScsiCdRomFindDevices)
#pragma alloc_text(PAGE, CreateCdRomDeviceObject)
#pragma alloc_text(PAGE, ScanForSpecial)
//#pragma alloc_text(PAGE, CdRomDeviceControl)
#pragma alloc_text(PAGE, HitachProcessError)
#pragma alloc_text(PAGE, CdRomIsPlayActive)
#pragma alloc_text(PAGE, ScsiCdRomReadVerification)
#pragma alloc_text(INIT, CdRomCheckRegistryForMediaChangeValue)
#pragma alloc_text(INIT, IsThisAnAtapiChanger)
#pragma alloc_text(INIT, IsThisASanyo)
#pragma alloc_text(INIT, IsThisAMultiLunDevice)
#pragma alloc_text(INIT, CdRomCreateNamedEvent)
#ifdef _PPC_
#pragma alloc_text(PAGE, FindScsiAdapter)
#endif
#endif

ULONG NoLoad = 0;

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the cdrom class driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the name of the services node for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    CLASS_INIT_DATA InitializationData;

    if(NoLoad) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Zero InitData
    //

    RtlZeroMemory (&InitializationData, sizeof(CLASS_INIT_DATA));

    //
    // Set sizes
    //

    InitializationData.InitializationDataSize = sizeof(CLASS_INIT_DATA);
    InitializationData.DeviceExtensionSize = DEVICE_EXTENSION_SIZE;

    InitializationData.DeviceType = FILE_DEVICE_CD_ROM;
    InitializationData.DeviceCharacteristics = FILE_REMOVABLE_MEDIA | FILE_READ_ONLY_DEVICE;

    //
    // Set entry points
    //

    InitializationData.ClassReadWriteVerification = ScsiCdRomReadVerification;
    InitializationData.ClassDeviceControl = CdRomDeviceControl;
    InitializationData.ClassFindDevices = ScsiCdRomFindDevices;
    InitializationData.ClassShutdownFlush = NULL;
    InitializationData.ClassCreateClose = NULL;
    InitializationData.ClassStartIo = ScsiCdRomStartIo;

    //
    // Call the class init routine
    //

    return ScsiClassInitialize( DriverObject, RegistryPath, &InitializationData);

} // end DriverEntry()

BOOLEAN
NTAPI
ScsiCdRomFindDevices(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN PCLASS_INIT_DATA InitializationData,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber
    )

/*++

Routine Description:

    Connect to SCSI port driver. Get adapter capabilities and
    SCSI bus configuration information. Search inquiry data
    for CDROM devices to process.

Arguments:

    DriverObject - CDROM class driver object.
    PortDeviceObject - SCSI port driver device object.
    PortNumber - The system ordinal for this scsi adapter.

Return Value:

    TRUE if CDROM device present on this SCSI adapter.

--*/

{
    PIO_SCSI_CAPABILITIES portCapabilities;
    PULONG cdRomCount;
    PCHAR buffer;
    PSCSI_INQUIRY_DATA lunInfo;
    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PINQUIRYDATA inquiryData;
    ULONG scsiBus;
    NTSTATUS status;
    BOOLEAN foundDevice = FALSE;

    //
    // Call port driver to get adapter capabilities.
    //

    status = ScsiClassGetCapabilities(PortDeviceObject, &portCapabilities);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"FindScsiDevices: ScsiClassGetCapabilities failed\n"));
        return foundDevice;
    }

    //
    // Call port driver to get inquiry information to find cdroms.
    //

    status = ScsiClassGetInquiryData(PortDeviceObject, (PSCSI_ADAPTER_BUS_INFO *) &buffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"FindScsiDevices: ScsiClassGetInquiryData failed\n"));
        return foundDevice;
    }

    //
    // Get the address of the count of the number of cdroms already initialized.
    //

    cdRomCount = &IoGetConfigurationInformation()->CdRomCount;
    adapterInfo = (PVOID) buffer;

    //
    // For each SCSI bus this adapter supports ...
    //

    for (scsiBus=0; scsiBus < adapterInfo->NumberOfBuses; scsiBus++) {

        //
        // Get the SCSI bus scan data for this bus.
        //

        lunInfo = (PVOID) (buffer + adapterInfo->BusData[scsiBus].InquiryDataOffset);

        //
        // Search list for unclaimed disk devices.
        //

        while (adapterInfo->BusData[scsiBus].InquiryDataOffset) {

            inquiryData = (PVOID)lunInfo->InquiryData;

            if ((inquiryData->DeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE) &&
                (inquiryData->DeviceTypeQualifier == 0) &&
                (!lunInfo->DeviceClaimed)) {

                DebugPrint((1,"FindScsiDevices: Vendor string is %.24s\n",
                            inquiryData->VendorId));

                //
                // Create device objects for cdrom
                //

                status = CreateCdRomDeviceObject(DriverObject,
                                                 PortDeviceObject,
                                                 PortNumber,
                                                 cdRomCount,
                                                 portCapabilities,
                                                 lunInfo,
                                                 InitializationData,
                                                 RegistryPath);

                if (NT_SUCCESS(status)) {

                    //
                    // Increment system cdrom device count.
                    //

                    (*cdRomCount)++;

                    //
                    // Indicate that a cdrom device was found.
                    //

                    foundDevice = TRUE;
                }
            }

            //
            // Get next LunInfo.
            //

            if (lunInfo->NextInquiryDataOffset == 0) {
                break;
            }

            lunInfo = (PVOID) (buffer + lunInfo->NextInquiryDataOffset);
        }
    }

    ExFreePool(buffer);


    return foundDevice;

} // end FindScsiCdRoms()

VOID
NTAPI
CdRomCreateNamedEvent(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG DeviceNumber
    )

/*++

Routine Description:

    Create the named synchronization event for notification of media change
    events to the system.  The event is reset before this function returns.

Arguments:

    DeviceExtension - the device extension pointer for storage of the event pointer.

Return Value:

    None.

--*/

{
    UNICODE_STRING    unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    CCHAR             eventNameBuffer[MAXIMUM_FILENAME_LENGTH];
    STRING            eventNameString;
    HANDLE            handle;
    NTSTATUS          status;


    sprintf(eventNameBuffer,"\\Device\\MediaChangeEvent%ld",
            DeviceNumber);

    RtlInitString(&eventNameString,
                  eventNameBuffer);

    status = RtlAnsiStringToUnicodeString(&unicodeString,
                                          &eventNameString,
                                          TRUE);

    if (!NT_SUCCESS(status)) {
        return;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeString,
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               NULL,
                               NULL);

    DeviceExtension->MediaChangeEvent = IoCreateSynchronizationEvent(&unicodeString,
                                                                     &handle);
    DeviceExtension->MediaChangeEventHandle = handle;

    KeClearEvent(DeviceExtension->MediaChangeEvent);

    RtlFreeUnicodeString(&unicodeString);
}

NTSTATUS
NTAPI
CreateCdRomDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG          PortNumber,
    IN PULONG         DeviceCount,
    IN PIO_SCSI_CAPABILITIES PortCapabilities,
    IN PSCSI_INQUIRY_DATA LunInfo,
    IN PCLASS_INIT_DATA   InitializationData,
    IN PUNICODE_STRING    RegistryPath
    )

/*++

Routine Description:

    This routine creates an object for the device and then calls the
    SCSI port driver for media capacity and sector size.

Arguments:

    DriverObject - Pointer to driver object created by system.
    PortDeviceObject - to connect to SCSI port driver.
    DeviceCount - Number of previously installed CDROMs.
    PortCapabilities - Pointer to structure returned by SCSI port
        driver describing adapter capabilites (and limitations).
    LunInfo - Pointer to configuration information for this device.

Return Value:

    NTSTATUS

--*/
{
    CHAR ntNameBuffer[64];
    NTSTATUS status;
    BOOLEAN changerDevice;
    SCSI_REQUEST_BLOCK srb;
    ULONG          length;
    PCDROM_DATA    cddata;
    PCDB           cdb;
    PVOID          senseData = NULL;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension = NULL;
    PUCHAR         buffer;
    ULONG          bps;
    ULONG          lastBit;
    ULONG          timeOut;
    BOOLEAN        srbListInitialized = FALSE;

    //
    // Claim the device. Note that any errors after this
    // will goto the generic handler, where the device will
    // be released.
    //

    status = ScsiClassClaimDevice(PortDeviceObject,
                                  LunInfo,
                                  FALSE,
                                  &PortDeviceObject);

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    //
    // Create device object for this device.
    //

    sprintf(ntNameBuffer,
            "\\Device\\CdRom%lu",
            *DeviceCount);

    status = ScsiClassCreateDeviceObject(DriverObject,
                                         ntNameBuffer,
                                         NULL,
                                         &deviceObject,
                                         InitializationData);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"CreateCdRomDeviceObjects: Can not create device %s\n",
                    ntNameBuffer));

        goto CreateCdRomDeviceObjectExit;
    }

    //
    // Indicate that IRPs should include MDLs.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    //
    // Set up required stack size in device object.
    //

    deviceObject->StackSize = PortDeviceObject->StackSize + 2;

    deviceExtension = deviceObject->DeviceExtension;

    //
    // Allocate spinlock for split request completion.
    //

    KeInitializeSpinLock(&deviceExtension->SplitRequestSpinLock);

    //
    // This is the physical device.
    //

    deviceExtension->PhysicalDevice = deviceObject;

    //
    // Initialize lock count to zero. The lock count is used to
    // disable the ejection mechanism when media is mounted.
    //

    deviceExtension->LockCount = 0;

    //
    // Save system cdrom number
    //

    deviceExtension->DeviceNumber = *DeviceCount;

    //
    // Copy port device object to device extension.
    //

    deviceExtension->PortDeviceObject = PortDeviceObject;

    //
    // Set the alignment requirements for the device based on the
    // host adapter requirements
    //

    if (PortDeviceObject->AlignmentRequirement > deviceObject->AlignmentRequirement) {
        deviceObject->AlignmentRequirement = PortDeviceObject->AlignmentRequirement;
    }

    //
    // Save address of port driver capabilities.
    //

    deviceExtension->PortCapabilities = PortCapabilities;

    //
    // Clear SRB flags.
    //

    deviceExtension->SrbFlags = 0;
    deviceExtension->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Allocate request sense buffer.
    //

    senseData = ExAllocatePool(NonPagedPoolCacheAligned, SENSE_BUFFER_SIZE);

    if (senseData == NULL) {

        //
        // The buffer cannot be allocated.
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CreateCdRomDeviceObjectExit;
    }

    //
    // Set the sense data pointer in the device extension.
    //

    deviceExtension->SenseData = senseData;

    //
    // CDROMs are not partitionable so starting offset is 0.
    //

    deviceExtension->StartingOffset.LowPart = 0;
    deviceExtension->StartingOffset.HighPart = 0;

    //
    // Path/TargetId/LUN describes a device location on the SCSI bus.
    // This information comes from the LunInfo buffer.
    //

    deviceExtension->PortNumber = (UCHAR)PortNumber;
    deviceExtension->PathId = LunInfo->PathId;
    deviceExtension->TargetId = LunInfo->TargetId;
    deviceExtension->Lun = LunInfo->Lun;

    //
    // Set timeout value in seconds.
    //

    timeOut = ScsiClassQueryTimeOutRegistryValue(RegistryPath);
    if (timeOut) {
        deviceExtension->TimeOutValue = timeOut;
    } else {
        deviceExtension->TimeOutValue = SCSI_CDROM_TIMEOUT;
    }

    //
    // Build the lookaside list for srb's for the physical disk. Should only
    // need a couple.
    //

    ScsiClassInitializeSrbLookasideList(deviceExtension,
                                        CDROM_SRB_LIST_SIZE);

    srbListInitialized = TRUE;

    //
    // Back pointer to device object.
    //

    deviceExtension->DeviceObject = deviceObject;

    //
    // Allocate buffer for drive geometry.
    //

    deviceExtension->DiskGeometry =
        ExAllocatePool(NonPagedPool, sizeof(DISK_GEOMETRY_EX));

    if (deviceExtension->DiskGeometry == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CreateCdRomDeviceObjectExit;
    }

    //
    // Set up media change support defaults.
    //

    cddata = (PCDROM_DATA)(deviceExtension + 1);

    KeInitializeSpinLock(&cddata->FormSpinLock);
    KeInitializeSpinLock(&cddata->TimerIrpSpinLock);
    InitializeListHead(&cddata->TimerIrpList);

    cddata->MediaChangeCountDown = MEDIA_CHANGE_DEFAULT_TIME;
    cddata->MediaChangeSupported = FALSE;
    cddata->MediaChange = FALSE;

    //
    // Assume that there is initially no media in the device
    // only notify upper layers if there is something there
    //

    deviceExtension->MediaChangeNoMedia = TRUE;
    cddata->MediaChangeIrp = NULL;
#if DBG
    cddata->MediaChangeIrpTimeInUse = 0;
    cddata->MediaChangeIrpLost = FALSE;
#endif

    //
    // Scan for Scsi controllers that require special processing.
    //

    ScanForSpecial(deviceObject,
                   (PINQUIRYDATA) LunInfo->InquiryData,
                   PortCapabilities);

    //
    // Do READ CAPACITY. This SCSI command
    // returns the last sector address on the device
    // and the bytes per sector.
    // These are used to calculate the drive capacity
    // in bytes.
    //

    status = ScsiClassReadDriveCapacity(deviceObject);
    bps = deviceExtension->DiskGeometry->Geometry.BytesPerSector;

    if (!NT_SUCCESS(status) || !bps) {

        DebugPrint((1,
                "CreateCdRomDeviceObjects: Can't read capacity for device %s\n",
                ntNameBuffer));

        //
        // Set disk geometry to default values (per ISO 9660).
        //

        bps = 2048;
        deviceExtension->SectorShift = 11;
        deviceExtension->PartitionLength.QuadPart = (LONGLONG)(0x7fffffff);
    } else {

        //
        // Insure that bytes per sector is a power of 2
        // This corrects a problem with the HP 4020i CDR where it
        // returns an incorrect number for bytes per sector.
        //

        lastBit = (ULONG) -1;
        while (bps) {
            lastBit++;
            bps = bps >> 1;
        }

        bps = 1 << lastBit;
    }
    deviceExtension->DiskGeometry->Geometry.BytesPerSector = bps;
    DebugPrint((2, "CreateCdRomDeviceObject: Calc'd bps = %x\n", bps));

    //
    // Check to see if this is some sort of changer device
    //

    changerDevice = FALSE;

    //
    // Search for devices that have special requirements for media
    // change support.
    //

    if (deviceExtension->Lun > 0) {
        changerDevice = TRUE;
    }

    if (!changerDevice) {
        changerDevice = IsThisASanyo(deviceObject, deviceExtension->PathId,
                                     deviceExtension->TargetId);
    }

    if (!changerDevice) {
        ULONG tmp;
        changerDevice = IsThisAnAtapiChanger(deviceObject, &tmp);
    }

    if (!changerDevice) {
        changerDevice = IsThisAMultiLunDevice(deviceObject, PortDeviceObject);
    }

    //
    // If it is a changer device, increment the timeout to take platter-swapping
    // time into account
    //

    if(changerDevice) {
        deviceExtension->TimeOutValue += SCSI_CHANGER_BONUS_TIMEOUT;
    }

    //
    // Create the media change named event.  If this succeeds then continue
    // initializing the media change support data items.
    //

    CdRomCreateNamedEvent(deviceExtension,*DeviceCount);
    if (deviceExtension->MediaChangeEvent) {

        //
        // If this is not a changer, get an IRP for the timer request
        // and initialize the timer.
        //

        if (!changerDevice) {

            //
            // Not a changer device - continue with media change initialization.
            // Determine if the user actually wants media change events.
            //

            if (CdRomCheckRegistryForMediaChangeValue(RegistryPath, *DeviceCount)) {
                PIO_STACK_LOCATION irpStack;
                PSCSI_REQUEST_BLOCK srb;
                PIRP irp;

                //
                // User wants it - preallocate IRP and SRB.
                //

                irp = IoAllocateIrp((CCHAR)(deviceObject->StackSize+1),
                                    FALSE);
                if (irp) {
                    PVOID buffer;

                    srb = ExAllocatePool(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK));
                    buffer = ExAllocatePool(NonPagedPoolCacheAligned, SENSE_BUFFER_SIZE);

                    if (srb && buffer) {
                        PCDB cdb;

                        //
                        // All resources have been allocated set up the IRP.
                        //

                        IoSetNextIrpStackLocation(irp);
                        irpStack = IoGetCurrentIrpStackLocation(irp);
                        irpStack->DeviceObject = deviceObject;
                        irpStack = IoGetNextIrpStackLocation(irp);
                        cddata->MediaChangeIrp = irp;
                        irpStack->Parameters.Scsi.Srb = srb;

                        //
                        // Initialize the SRB
                        //

                        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

                        srb->CdbLength = 6;
                        srb->TimeOutValue = deviceExtension->TimeOutValue * 2;
                        srb->QueueTag = SP_UNTAGGED;
                        srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
                        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
                        srb->PathId = deviceExtension->PathId;
                        srb->TargetId = deviceExtension->TargetId;
                        srb->Lun = deviceExtension->Lun;
                        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

                        //
                        // Initialize and set up the sense information buffer
                        //

                        RtlZeroMemory(buffer, SENSE_BUFFER_SIZE);
                        srb->SenseInfoBuffer = buffer;
                        srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

                        //
                        // Initialize the CDB
                        //

                        cdb = (PCDB)&srb->Cdb[0];
                        cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
                        cdb->CDB6GENERIC.LogicalUnitNumber = deviceExtension->Lun;

                        //
                        // It is ok to support media change events on this device.
                        //

                        cddata->MediaChangeSupported = TRUE;
                        cddata->MediaChange = TRUE;

                    } else {

                        if (srb) {
                            ExFreePool(srb);
                        }
                        if (buffer) {
                            ExFreePool(buffer);
                        }
                        IoFreeIrp(irp);
                    }
                }
            } else {
                deviceExtension->MediaChangeEvent = NULL;
            }
        } else {
            deviceExtension->MediaChangeEvent = NULL;
        }
    }

    //
    // Assume use of 6-byte mode sense/select for now.
    //

    cddata->XAFlags |= XA_USE_6_BYTE;

    //
    // Build and issue mode sense with Read error recovery page. This will be used to change
    // block size in case of any raw reads (Mode 2, Form 2).
    //

    length = (sizeof(MODE_READ_RECOVERY_PAGE) + MODE_BLOCK_DESC_LENGTH + MODE_HEADER_LENGTH);

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    srb.CdbLength = 6;
    cdb = (PCDB)srb.Cdb;

    //
    // Set timeout value from device extension.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Build the MODE SENSE CDB. The data returned will be kept in the device extension
    // and used to set block size.
    //

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = 0x1;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)length;

    buffer = ExAllocatePool(NonPagedPoolCacheAligned, (sizeof(MODE_READ_RECOVERY_PAGE) + MODE_BLOCK_DESC_LENGTH + MODE_HEADER_LENGTH10));
    if (!buffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CreateCdRomDeviceObjectExit;
    }

    status = ScsiClassSendSrbSynchronous(deviceObject,
                                         &srb,
                                         buffer,
                                         length,
                                         FALSE);
    if (!NT_SUCCESS(status)) {

        //
        // May be Atapi, try 10-byte.
        //

        length = (sizeof(MODE_READ_RECOVERY_PAGE) + MODE_BLOCK_DESC_LENGTH + MODE_HEADER_LENGTH10);

        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        //
        // Build the MODE SENSE CDB.
        //

        srb.CdbLength = 10;
        cdb = (PCDB)srb.Cdb;

        //
        // Set timeout value from device extension.
        //

        srb.TimeOutValue = deviceExtension->TimeOutValue;

        cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
        cdb->MODE_SENSE10.PageCode = 0x1;

        cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(length >> 8);
        cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(length & 0xFF);

        status = ScsiClassSendSrbSynchronous(deviceObject,
                                             &srb,
                                             buffer,
                                             length,
                                             FALSE);
        if (status == STATUS_DATA_OVERRUN) {

            //
            // Build and issue the ReadCd command to ensure that this device supports it.
            //

            RtlZeroMemory(cdb, 12);

            cdb->READ_CD.OperationCode = SCSIOP_READ_CD;

            status = ScsiClassSendSrbSynchronous(deviceObject,
                                                 &srb,
                                                 NULL,
                                                 0,
                                                 FALSE);

            //
            // If the command wasn't rejected then support the READ_CD.
            //

            if (NT_SUCCESS(status) || (status == STATUS_NO_MEDIA_IN_DEVICE)) {

                //
                // Using Read CD precludes issueing a mode select to
                // set the user data size. So, no buffer copy is
                // necessary.
                //

                cddata->XAFlags &= ~XA_USE_6_BYTE;
                cddata->XAFlags |= XA_USE_READ_CD | XA_USE_10_BYTE;
            } else {

                RtlCopyMemory(&cddata->u1.Header, buffer, sizeof(ERROR_RECOVERY_DATA10));
                cddata->u1.Header.ModeDataLength = 0;

                cddata->XAFlags &= ~XA_USE_6_BYTE;
                cddata->XAFlags |= XA_USE_10_BYTE;
            }

        } else if (NT_SUCCESS(status)) {

            RtlCopyMemory(&cddata->u1.Header, buffer, sizeof(ERROR_RECOVERY_DATA10));
            cddata->u1.Header.ModeDataLength = 0;

            cddata->XAFlags &= ~XA_USE_6_BYTE;
            cddata->XAFlags |= XA_USE_10_BYTE;

        } else {
            cddata->XAFlags |= XA_NOT_SUPPORTED;
        }
    } else {
        RtlCopyMemory(&cddata->u1.Header, buffer, sizeof(ERROR_RECOVERY_DATA));
        cddata->u1.Header.ModeDataLength = 0;
    }

    ExFreePool(buffer);

    //
    // Start the timer now regardless of if Autorun is enabled.
    // The timer must run forever since IoStopTimer faults.
    //

    IoInitializeTimer(deviceObject, CdRomTickHandler, NULL);
    IoStartTimer(deviceObject);

    return(STATUS_SUCCESS);

CreateCdRomDeviceObjectExit:

    //
    // Release the device since an error occured.
    //

    ScsiClassClaimDevice(PortDeviceObject,
                         LunInfo,
                         TRUE,
                         NULL);

    if (senseData != NULL) {
        ExFreePool(senseData);
    }

    if (deviceExtension->DiskGeometry != NULL) {
        ExFreePool(deviceExtension->DiskGeometry);
    }

    if (deviceObject != NULL) {
        if (srbListInitialized) {
            ExDeleteNPagedLookasideList(&deviceExtension->SrbLookasideListHead);
        }
        IoDeleteDevice(deviceObject);
    }


    return status;

} // end CreateCdRomDeviceObject()

VOID
NTAPI
ScsiCdRomStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION  nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PIO_STACK_LOCATION  irpStack;
    PIRP                irp2 = NULL;
    ULONG               transferPages;
    ULONG               transferByteCount = currentIrpStack->Parameters.Read.Length;
    ULONG               maximumTransferLength = deviceExtension->PortCapabilities->MaximumTransferLength;
    PCDROM_DATA         cdData;
    PSCSI_REQUEST_BLOCK srb = NULL;
    PCDB                cdb;
    PUCHAR              senseBuffer = NULL;
    PVOID               dataBuffer;
    NTSTATUS            status;
    BOOLEAN             use6Byte;

    //
    // Mark IRP with status pending.
    //

    IoMarkIrpPending(Irp);

    //
    // If the flag is set in the device object, force a verify.
    //

    if (DeviceObject->Flags & DO_VERIFY_VOLUME) {
        DebugPrint((2, "ScsiCdRomStartIo: [%lx] Volume needs verified\n", Irp));
        if (!(currentIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME)) {

            if (Irp->Tail.Overlay.Thread) {
                IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
            }

            Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;

            DebugPrint((2, "ScsiCdRomStartIo: [%lx] Calling UpdateCapcity - "
                           "ioctl event = %lx\n",
                        Irp,
                        nextIrpStack->Parameters.Others.Argument1
                      ));

            //
            // our device control dispatch routine stores an event in the next
            // stack location to signal when startio has completed.  We need to
            // pass this in so that the update capacity completion routine can
            // set it rather than completing the Irp.
            //

            status = CdRomUpdateCapacity(deviceExtension,
                                         Irp,
                                         nextIrpStack->Parameters.Others.Argument1
                                         );

            DebugPrint((2, "ScsiCdRomStartIo: [%lx] UpdateCapacity returned %lx\n", Irp, status));
            ASSERT(status == STATUS_PENDING);
            return;
        }
    }

    cdData = (PCDROM_DATA)(deviceExtension + 1);
    use6Byte = cdData->XAFlags & XA_USE_6_BYTE;

    if (currentIrpStack->MajorFunction == IRP_MJ_READ) {

        //
        // Add partition byte offset to make starting byte relative to
        // beginning of disk. In addition, add in skew for DM Driver, if any.
        //

        currentIrpStack->Parameters.Read.ByteOffset.QuadPart += (deviceExtension->StartingOffset.QuadPart);

        //
        // Calculate number of pages in this transfer.
        //

        transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Irp->MdlAddress),
                                                       currentIrpStack->Parameters.Read.Length);

        //
        // Check if request length is greater than the maximum number of
        // bytes that the hardware can transfer.
        //

        if (cdData->RawAccess) {

            ASSERT(!(cdData->XAFlags & XA_USE_READ_CD));

            //
            // Fire off a mode select to switch back to cooked sectors.
            //

            irp2 = IoAllocateIrp((CCHAR)(deviceExtension->DeviceObject->StackSize+1),
                                  FALSE);

            if (!irp2) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            srb = ExAllocatePool(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK));
            if (!srb) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

            cdb = (PCDB)srb->Cdb;

            //
            // Allocate sense buffer.
            //

            senseBuffer = ExAllocatePool(NonPagedPoolCacheAligned, SENSE_BUFFER_SIZE);

            if (!senseBuffer) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            //
            // Set up the irp.
            //

            IoSetNextIrpStackLocation(irp2);
            irp2->IoStatus.Status = STATUS_SUCCESS;
            irp2->IoStatus.Information = 0;
            irp2->Flags = 0;
            irp2->UserBuffer = NULL;

            //
            // Save the device object and irp in a private stack location.
            //

            irpStack = IoGetCurrentIrpStackLocation(irp2);
            irpStack->DeviceObject = deviceExtension->DeviceObject;
            irpStack->Parameters.Others.Argument2 = (PVOID) Irp;

            //
            // The retry count will be in the real Irp, as the retry logic will
            // recreate our private irp.
            //

            if (!(nextIrpStack->Parameters.Others.Argument1)) {

                //
                // Only jam this in if it doesn't exist. The completion routines can
                // call StartIo directly in the case of retries and resetting it will
                // cause infinite loops.
                //

                nextIrpStack->Parameters.Others.Argument1 = (PVOID) MAXIMUM_RETRIES;
            }

            //
            // Construct the IRP stack for the lower level driver.
            //

            irpStack = IoGetNextIrpStackLocation(irp2);
            irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
            irpStack->Parameters.Scsi.Srb = srb;

            srb->Length = SCSI_REQUEST_BLOCK_SIZE;
            srb->PathId = deviceExtension->PathId;
            srb->TargetId = deviceExtension->TargetId;
            srb->Lun = deviceExtension->Lun;
            srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            srb->Cdb[1] |= deviceExtension->Lun << 5;
            srb->SrbStatus = srb->ScsiStatus = 0;
            srb->NextSrb = 0;
            srb->OriginalRequest = irp2;
            srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
            srb->SenseInfoBuffer = senseBuffer;

            transferByteCount = (use6Byte) ? sizeof(ERROR_RECOVERY_DATA) : sizeof(ERROR_RECOVERY_DATA10);
            dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned, transferByteCount );
            if (!dataBuffer) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            transferByteCount,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            //
            // Prepare the MDL
            //

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);

            srb->DataBuffer = dataBuffer;

            //
            // Set the new block size in the descriptor.
            //

            cdData->u1.BlockDescriptor.BlockLength[0] = (UCHAR)(COOKED_SECTOR_SIZE >> 16) & 0xFF;
            cdData->u1.BlockDescriptor.BlockLength[1] = (UCHAR)(COOKED_SECTOR_SIZE >>  8) & 0xFF;
            cdData->u1.BlockDescriptor.BlockLength[2] = (UCHAR)(COOKED_SECTOR_SIZE & 0xFF);

            //
            // Move error page into dataBuffer.
            //

            RtlCopyMemory(srb->DataBuffer, &cdData->u1.Header, transferByteCount);

            //
            // Build and send a mode select to switch into raw mode.
            //

            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_OUT);
            srb->DataTransferLength = transferByteCount;
            srb->TimeOutValue = deviceExtension->TimeOutValue * 2;

            if (use6Byte) {
                srb->CdbLength = 6;
                cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
                cdb->MODE_SELECT.PFBit = 1;
                cdb->MODE_SELECT.ParameterListLength = (UCHAR)transferByteCount;
            } else {

                srb->CdbLength = 10;
                cdb->MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
                cdb->MODE_SELECT10.PFBit = 1;
                cdb->MODE_SELECT10.ParameterListLength[0] = (UCHAR)(transferByteCount >> 8);
                cdb->MODE_SELECT10.ParameterListLength[1] = (UCHAR)(transferByteCount & 0xFF);
            }

            //
            // Update completion routine.
            //

            IoSetCompletionRoutine(irp2,
                                   CdRomSwitchModeCompletion,
                                   srb,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;
        }

        if ((currentIrpStack->Parameters.Read.Length > maximumTransferLength) ||
            (transferPages >
                deviceExtension->PortCapabilities->MaximumPhysicalPages)) {

            //
            // Request needs to be split. Completion of each portion of the
            // request will fire off the next portion. The final request will
            // signal Io to send a new request.
            //

            transferPages =
                deviceExtension->PortCapabilities->MaximumPhysicalPages - 1;

            if(maximumTransferLength > transferPages << PAGE_SHIFT) {
                maximumTransferLength = transferPages << PAGE_SHIFT;
            }

            //
            // Check that the maximum transfer size is not zero
            //

            if(maximumTransferLength == 0) {
                maximumTransferLength = PAGE_SIZE;
            }

            ScsiClassSplitRequest(DeviceObject, Irp, maximumTransferLength);
            return;

        } else {

            //
            // Build SRB and CDB for this IRP.
            //

            ScsiClassBuildRequest(DeviceObject, Irp);

        }


    } else if (currentIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {

        //
        // Allocate an irp, srb and associated structures.
        //

        irp2 = IoAllocateIrp((CCHAR)(deviceExtension->DeviceObject->StackSize+1),
                              FALSE);

        if (!irp2) {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            IoStartNextPacket(DeviceObject, FALSE);
            DebugPrint((2, "ScsiCdRomStartIo: [%lx] bailing with status %lx at line %s\n", Irp, Irp->IoStatus.Status, __LINE__));
            return;
        }

        srb = ExAllocatePool(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK));
        if (!srb) {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            IoFreeIrp(irp2);
            IoStartNextPacket(DeviceObject, FALSE);
            DebugPrint((2, "ScsiCdRomStartIo: [%lx] bailing with status %lx at line %s\n", Irp, Irp->IoStatus.Status, __LINE__));
            return;
        }

        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        cdb = (PCDB)srb->Cdb;

        //
        // Allocate sense buffer.
        //

        senseBuffer = ExAllocatePool(NonPagedPoolCacheAligned, SENSE_BUFFER_SIZE);

        if (!senseBuffer) {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            ExFreePool(srb);
            IoFreeIrp(irp2);
            IoStartNextPacket(DeviceObject, FALSE);
            DebugPrint((2, "ScsiCdRomStartIo: [%lx] bailing with status %lx at line %s\n", Irp, Irp->IoStatus.Status, __LINE__));
            return;
        }

        //
        // Set up the irp.
        //

        IoSetNextIrpStackLocation(irp2);
        irp2->IoStatus.Status = STATUS_SUCCESS;
        irp2->IoStatus.Information = 0;
        irp2->Flags = 0;
        irp2->UserBuffer = NULL;

        //
        // Save the device object and irp in a private stack location.
        //

        irpStack = IoGetCurrentIrpStackLocation(irp2);
        irpStack->DeviceObject = deviceExtension->DeviceObject;
        irpStack->Parameters.Others.Argument2 = (PVOID) Irp;

        //
        // The retry count will be in the real Irp, as the retry logic will
        // recreate our private irp.
        //

        if (!(nextIrpStack->Parameters.Others.Argument1)) {

            //
            // Only jam this in if it doesn't exist. The completion routines can
            // call StartIo directly in the case of retries and resetting it will
            // cause infinite loops.
            //

            nextIrpStack->Parameters.Others.Argument1 = (PVOID) MAXIMUM_RETRIES;
        }

        //
        // Construct the IRP stack for the lower level driver.
        //

        irpStack = IoGetNextIrpStackLocation(irp2);
        irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
        irpStack->Parameters.Scsi.Srb = srb;

        IoSetCompletionRoutine(irp2,
                               CdRomDeviceControlCompletion,
                               srb,
                               TRUE,
                               TRUE,
                               TRUE);
        //
        // Setup those fields that are generic to all requests.
        //

        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->PathId = deviceExtension->PathId;
        srb->TargetId = deviceExtension->TargetId;
        srb->Lun = deviceExtension->Lun;
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->Cdb[1] |= deviceExtension->Lun << 5;
        srb->SrbStatus = srb->ScsiStatus = 0;
        srb->NextSrb = 0;
        srb->OriginalRequest = irp2;
        srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
        srb->SenseInfoBuffer = senseBuffer;

        switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_CDROM_RAW_READ: {

            //
            // Determine whether the drive is currently in raw or cooked mode,
            // and which command to use to read the data.
            //

            if (!(cdData->XAFlags & XA_USE_READ_CD)) {

                PRAW_READ_INFO rawReadInfo =
                                   (PRAW_READ_INFO)currentIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
                ULONG          maximumTransferLength;
                ULONG          transferPages;

                if (cdData->RawAccess) {

                    ULONG  startingSector;

                    //
                    // Free the recently allocated irp, as we don't need it.
                    //

                    IoFreeIrp(irp2);

                    cdb = (PCDB)srb->Cdb;
                    RtlZeroMemory(cdb, 12);

                    //
                    // Calculate starting offset.
                    //

                    startingSector = (ULONG)(rawReadInfo->DiskOffset.QuadPart >> deviceExtension->SectorShift);
                    transferByteCount  = rawReadInfo->SectorCount * RAW_SECTOR_SIZE;
                    maximumTransferLength = deviceExtension->PortCapabilities->MaximumTransferLength;
                    transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Irp->MdlAddress),
                                                                   transferByteCount);

                    //
                    // Determine if request is within limits imposed by miniport.
                    //

                    if (transferByteCount > maximumTransferLength ||
                        transferPages > deviceExtension->PortCapabilities->MaximumPhysicalPages) {

                        //
                        // The claim is that this won't happen, and is backed up by
                        // ActiveMovie usage, which does unbuffered XA reads of 0x18000, yet
                        // we get only 4 sector requests.
                        //


                        Irp->IoStatus.Information = 0;
                        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                        ExFreePool(senseBuffer);
                        ExFreePool(srb);
                        IoStartNextPacket(DeviceObject, FALSE);
                        return;

                    }

                    srb->OriginalRequest = Irp;
                    srb->SrbFlags = deviceExtension->SrbFlags;
                    srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN);
                    srb->DataTransferLength = transferByteCount;
                    srb->TimeOutValue = deviceExtension->TimeOutValue;
                    srb->CdbLength = 10;
                    srb->DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

                    if (rawReadInfo->TrackMode == CDDA) {
                        if (cdData->XAFlags & PLEXTOR_CDDA) {

                            srb->CdbLength = 12;

                            cdb->PLXTR_READ_CDDA.LogicalUnitNumber = deviceExtension->Lun;
                            cdb->PLXTR_READ_CDDA.LogicalBlockByte3  = (UCHAR) (startingSector & 0xFF);
                            cdb->PLXTR_READ_CDDA.LogicalBlockByte2  = (UCHAR) ((startingSector >>  8) & 0xFF);
                            cdb->PLXTR_READ_CDDA.LogicalBlockByte1  = (UCHAR) ((startingSector >> 16) & 0xFF);
                            cdb->PLXTR_READ_CDDA.LogicalBlockByte0  = (UCHAR) ((startingSector >> 24) & 0xFF);

                            cdb->PLXTR_READ_CDDA.TransferBlockByte3 = (UCHAR) (rawReadInfo->SectorCount & 0xFF);
                            cdb->PLXTR_READ_CDDA.TransferBlockByte2 = (UCHAR) (rawReadInfo->SectorCount >> 8);
                            cdb->PLXTR_READ_CDDA.TransferBlockByte1 = 0;
                            cdb->PLXTR_READ_CDDA.TransferBlockByte0 = 0;

                            cdb->PLXTR_READ_CDDA.SubCode = 0;
                            cdb->PLXTR_READ_CDDA.OperationCode = 0xD8;

                        } else if (cdData->XAFlags & NEC_CDDA) {

                            cdb->NEC_READ_CDDA.LogicalBlockByte3  = (UCHAR) (startingSector & 0xFF);
                            cdb->NEC_READ_CDDA.LogicalBlockByte2  = (UCHAR) ((startingSector >>  8) & 0xFF);
                            cdb->NEC_READ_CDDA.LogicalBlockByte1  = (UCHAR) ((startingSector >> 16) & 0xFF);
                            cdb->NEC_READ_CDDA.LogicalBlockByte0  = (UCHAR) ((startingSector >> 24) & 0xFF);

                            cdb->NEC_READ_CDDA.TransferBlockByte1 = (UCHAR) (rawReadInfo->SectorCount & 0xFF);
                            cdb->NEC_READ_CDDA.TransferBlockByte0 = (UCHAR) (rawReadInfo->SectorCount >> 8);

                            cdb->NEC_READ_CDDA.OperationCode = 0xD4;
                        }
                    } else {

                        cdb->CDB10.LogicalUnitNumber = deviceExtension->Lun;

                        cdb->CDB10.TransferBlocksMsb  = (UCHAR) (rawReadInfo->SectorCount >> 8);
                        cdb->CDB10.TransferBlocksLsb  = (UCHAR) (rawReadInfo->SectorCount & 0xFF);

                        cdb->CDB10.LogicalBlockByte3  = (UCHAR) (startingSector & 0xFF);
                        cdb->CDB10.LogicalBlockByte2  = (UCHAR) ((startingSector >>  8) & 0xFF);
                        cdb->CDB10.LogicalBlockByte1  = (UCHAR) ((startingSector >> 16) & 0xFF);
                        cdb->CDB10.LogicalBlockByte0  = (UCHAR) ((startingSector >> 24) & 0xFF);

                        cdb->CDB10.OperationCode = SCSIOP_READ;
                    }

                    srb->SrbStatus = srb->ScsiStatus = 0;

                    nextIrpStack->MajorFunction = IRP_MJ_SCSI;
                    nextIrpStack->Parameters.Scsi.Srb = srb;

                    if (!(nextIrpStack->Parameters.Others.Argument1)) {

                        //
                        // Only jam this in if it doesn't exist. The completion routines can
                        // call StartIo directly in the case of retries and resetting it will
                        // cause infinite loops.
                        //

                        nextIrpStack->Parameters.Others.Argument1 = (PVOID) MAXIMUM_RETRIES;
                    }

                    //
                    // Set up IoCompletion routine address.
                    //

                    IoSetCompletionRoutine(Irp,
                                           CdRomXACompletion,
                                           srb,
                                           TRUE,
                                           TRUE,
                                           TRUE);

                    IoCallDriver(deviceExtension->PortDeviceObject, Irp);
                    return;

                } else {

                    transferByteCount = (use6Byte) ? sizeof(ERROR_RECOVERY_DATA) : sizeof(ERROR_RECOVERY_DATA10);
                    dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned, transferByteCount );
                    if (!dataBuffer) {
                        Irp->IoStatus.Information = 0;
                        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                        ExFreePool(senseBuffer);
                        ExFreePool(srb);
                        IoFreeIrp(irp2);
                        IoStartNextPacket(DeviceObject, FALSE);
                        return;

                    }

                    irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                                    transferByteCount,
                                                    FALSE,
                                                    FALSE,
                                                    (PIRP) NULL);

                    if (!irp2->MdlAddress) {
                        Irp->IoStatus.Information = 0;
                        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                        ExFreePool(senseBuffer);
                        ExFreePool(srb);
                        ExFreePool(dataBuffer);
                        IoFreeIrp(irp2);
                        IoStartNextPacket(DeviceObject, FALSE);
                        return;
                    }

                    //
                    // Prepare the MDL
                    //

                    MmBuildMdlForNonPagedPool(irp2->MdlAddress);

                    srb->DataBuffer = dataBuffer;

                    //
                    // Set the new block size in the descriptor.
                    //

                    cdData->u1.BlockDescriptor.BlockLength[0] = (UCHAR)(RAW_SECTOR_SIZE >> 16) & 0xFF;
                    cdData->u1.BlockDescriptor.BlockLength[1] = (UCHAR)(RAW_SECTOR_SIZE >>  8) & 0xFF;
                    cdData->u1.BlockDescriptor.BlockLength[2] = (UCHAR)(RAW_SECTOR_SIZE & 0xFF);


                    //
                    // TODO: Set density code, based on operation
                    //

                    cdData->u1.BlockDescriptor.DensityCode = 0;


                    //
                    // Move error page into dataBuffer.
                    //

                    RtlCopyMemory(srb->DataBuffer, &cdData->u1.Header, transferByteCount);


                    //
                    // Build and send a mode select to switch into raw mode.
                    //

                    srb->SrbFlags = deviceExtension->SrbFlags;
                    srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_OUT);
                    srb->DataTransferLength = transferByteCount;
                    srb->TimeOutValue = deviceExtension->TimeOutValue * 2;

                    if (use6Byte) {
                        srb->CdbLength = 6;
                        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
                        cdb->MODE_SELECT.PFBit = 1;
                        cdb->MODE_SELECT.ParameterListLength = (UCHAR)transferByteCount;
                    } else {

                        srb->CdbLength = 10;
                        cdb->MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
                        cdb->MODE_SELECT10.PFBit = 1;
                        cdb->MODE_SELECT10.ParameterListLength[0] = (UCHAR)(transferByteCount >> 8);
                        cdb->MODE_SELECT10.ParameterListLength[1] = (UCHAR)(transferByteCount & 0xFF);
                    }

                    //
                    // Update completion routine.
                    //

                    IoSetCompletionRoutine(irp2,
                                           CdRomSwitchModeCompletion,
                                           srb,
                                           TRUE,
                                           TRUE,
                                           TRUE);

                }

            } else {

                PRAW_READ_INFO rawReadInfo =
                                   (PRAW_READ_INFO)currentIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
                ULONG  startingSector;

                //
                // Free the recently allocated irp, as we don't need it.
                //

                IoFreeIrp(irp2);

                cdb = (PCDB)srb->Cdb;
                RtlZeroMemory(cdb, 12);


                //
                // Calculate starting offset.
                //

                startingSector = (ULONG)(rawReadInfo->DiskOffset.QuadPart >> deviceExtension->SectorShift);
                transferByteCount  = rawReadInfo->SectorCount * RAW_SECTOR_SIZE;


                srb->OriginalRequest = Irp;
                srb->SrbFlags = deviceExtension->SrbFlags;
                srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN);
                srb->DataTransferLength = transferByteCount;
                srb->TimeOutValue = deviceExtension->TimeOutValue;
                srb->DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);
                srb->CdbLength = 12;
                srb->SrbStatus = srb->ScsiStatus = 0;

                //
                // Fill in CDB fields.
                //

                cdb = (PCDB)srb->Cdb;


                cdb->READ_CD.TransferBlocks[2]  = (UCHAR) (rawReadInfo->SectorCount & 0xFF);
                cdb->READ_CD.TransferBlocks[1]  = (UCHAR) (rawReadInfo->SectorCount >> 8 );
                cdb->READ_CD.TransferBlocks[0]  = (UCHAR) (rawReadInfo->SectorCount >> 16);


                cdb->READ_CD.StartingLBA[3]  = (UCHAR) (startingSector & 0xFF);
                cdb->READ_CD.StartingLBA[2]  = (UCHAR) ((startingSector >>  8));
                cdb->READ_CD.StartingLBA[1]  = (UCHAR) ((startingSector >> 16));
                cdb->READ_CD.StartingLBA[0]  = (UCHAR) ((startingSector >> 24));

                //
                // Setup cdb depending upon the sector type we want.
                //

                switch (rawReadInfo->TrackMode) {
                case CDDA:

                    cdb->READ_CD.ExpectedSectorType = CD_DA_SECTOR;
                    cdb->READ_CD.IncludeUserData = 1;
                    cdb->READ_CD.HeaderCode = 3;
                    cdb->READ_CD.IncludeSyncData = 1;
                    break;

                case YellowMode2:

                    cdb->READ_CD.ExpectedSectorType = YELLOW_MODE2_SECTOR;
                    cdb->READ_CD.IncludeUserData = 1;
                    cdb->READ_CD.HeaderCode = 1;
                    cdb->READ_CD.IncludeSyncData = 1;
                    break;

                case XAForm2:

                    cdb->READ_CD.ExpectedSectorType = FORM2_MODE2_SECTOR;
                    cdb->READ_CD.IncludeUserData = 1;
                    cdb->READ_CD.HeaderCode = 3;
                    cdb->READ_CD.IncludeSyncData = 1;
                    break;

                default:
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                    ExFreePool(senseBuffer);
                    ExFreePool(srb);
                    IoStartNextPacket(DeviceObject, FALSE);
                    DebugPrint((2, "ScsiCdRomStartIo: [%lx] bailing with status %lx at line %s\n", Irp, Irp->IoStatus.Status, __LINE__));
                    return;
                }

                cdb->READ_CD.OperationCode = SCSIOP_READ_CD;

                nextIrpStack->MajorFunction = IRP_MJ_SCSI;
                nextIrpStack->Parameters.Scsi.Srb = srb;

                if (!(nextIrpStack->Parameters.Others.Argument1)) {

                    //
                    // Only jam this in if it doesn't exist. The completion routines can
                    // call StartIo directly in the case of retries and resetting it will
                    // cause infinite loops.
                    //

                    nextIrpStack->Parameters.Others.Argument1 = (PVOID) MAXIMUM_RETRIES;
                }

                //
                // Set up IoCompletion routine address.
                //

                IoSetCompletionRoutine(Irp,
                                       CdRomXACompletion,
                                       srb,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                IoCallDriver(deviceExtension->PortDeviceObject, Irp);
                return;

            }

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_GET_DRIVE_GEOMETRY: {

            //
            // Issue ReadCapacity to update device extension
            // with information for current media.
            //

            DebugPrint((3,
                        "CdRomStartIo: Get drive capacity\n"));

            //
            // setup remaining srb and cdb parameters.
            //

            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);
            srb->CdbLength = 10;
            srb->TimeOutValue = deviceExtension->TimeOutValue;

            dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(READ_CAPACITY_DATA));
            if (!dataBuffer) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                DebugPrint((2, "ScsiCdRomStartIo: [%lx] bailing with status %lx at line %s\n", Irp, Irp->IoStatus.Status, __LINE__));
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            sizeof(READ_CAPACITY_DATA),
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                DebugPrint((2, "ScsiCdRomStartIo: [%lx] bailing with status %lx at line %s\n", Irp, Irp->IoStatus.Status, __LINE__));
                return;
            }

            //
            // Prepare the MDL
            //

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);

            srb->DataBuffer = dataBuffer;
            cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_CHECK_VERIFY: {

            //
            // Since a test unit ready is about to be performed, reset the timer
            // value to decrease the opportunities for it to race with this code.
            //

            cdData->MediaChangeCountDown = MEDIA_CHANGE_DEFAULT_TIME;

            //
            // Set up the SRB/CDB
            //

            srb->CdbLength = 6;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
            srb->TimeOutValue = deviceExtension->TimeOutValue * 2;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_DATA_TRANSFER);

            DebugPrint((2, "ScsiCdRomStartIo: [%lx] Sending CHECK_VERIFY irp %lx\n", Irp, irp2));
            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_GET_LAST_SESSION:

            //
            // Set format to return first and last session numbers.
            //

            cdb->READ_TOC.Format = GET_LAST_SESSION;

            //
            // Fall through to READ TOC code.
            //

        case IOCTL_CDROM_READ_TOC: {


            if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_READ_TOC) {

                //
                // Use MSF addressing if not request for session information.
                //

                cdb->READ_TOC.Msf = CDB_USE_MSF;
            }

            //
            // Set size of TOC structure.
            //

            transferByteCount =
                currentIrpStack->Parameters.Read.Length >
                    sizeof(CDROM_TOC) ? sizeof(CDROM_TOC):
                    currentIrpStack->Parameters.Read.Length;

            cdb->READ_TOC.AllocationLength[0] = (UCHAR) (transferByteCount >> 8);
            cdb->READ_TOC.AllocationLength[1] = (UCHAR) (transferByteCount & 0xFF);

            cdb->READ_TOC.Control = 0;

            //
            // Start at beginning of disc.
            //

            cdb->READ_TOC.StartingTrack = 0;

            //
            // setup remaining srb and cdb parameters.
            //

            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = transferByteCount;
            srb->CdbLength = 10;
            srb->TimeOutValue = deviceExtension->TimeOutValue;

            dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned, transferByteCount);
            if (!dataBuffer) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            transferByteCount,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            //
            // Prepare the MDL
            //

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);

            srb->DataBuffer = dataBuffer;
            cdb->READ_TOC.OperationCode = SCSIOP_READ_TOC;

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_PLAY_AUDIO_MSF: {

            PCDROM_PLAY_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            //
            // Set up the SRB/CDB
            //

            srb->CdbLength = 10;
            cdb->PLAY_AUDIO_MSF.OperationCode = SCSIOP_PLAY_AUDIO_MSF;

            cdb->PLAY_AUDIO_MSF.StartingM = inputBuffer->StartingM;
            cdb->PLAY_AUDIO_MSF.StartingS = inputBuffer->StartingS;
            cdb->PLAY_AUDIO_MSF.StartingF = inputBuffer->StartingF;

            cdb->PLAY_AUDIO_MSF.EndingM = inputBuffer->EndingM;
            cdb->PLAY_AUDIO_MSF.EndingS = inputBuffer->EndingS;
            cdb->PLAY_AUDIO_MSF.EndingF = inputBuffer->EndingF;

            srb->TimeOutValue = deviceExtension->TimeOutValue;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_READ_Q_CHANNEL: {

            PCDROM_SUB_Q_DATA_FORMAT inputBuffer =
                             Irp->AssociatedIrp.SystemBuffer;

            //
            // Allocate buffer for subq channel information.
            //

            dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                     sizeof(SUB_Q_CHANNEL_DATA));

            if (!dataBuffer) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            transferByteCount,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            //
            // Prepare the MDL
            //

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);

            srb->DataBuffer = dataBuffer;

            //
            // Always logical unit 0, but only use MSF addressing
            // for IOCTL_CDROM_CURRENT_POSITION
            //

            if (inputBuffer->Format==IOCTL_CDROM_CURRENT_POSITION)
                cdb->SUBCHANNEL.Msf = CDB_USE_MSF;

            //
            // Return subchannel data
            //

            cdb->SUBCHANNEL.SubQ = CDB_SUBCHANNEL_BLOCK;

            //
            // Specify format of informatin to return
            //

            cdb->SUBCHANNEL.Format = inputBuffer->Format;

            //
            // Specify which track to access (only used by Track ISRC reads)
            //

            if (inputBuffer->Format==IOCTL_CDROM_TRACK_ISRC) {
                cdb->SUBCHANNEL.TrackNumber = inputBuffer->Track;
            }

            //
            // Set size of channel data -- however, this is dependent on
            // what information we are requesting (which Format)
            //

            switch( inputBuffer->Format ) {

                case IOCTL_CDROM_CURRENT_POSITION:
                    transferByteCount = sizeof(SUB_Q_CURRENT_POSITION);
                    break;

                case IOCTL_CDROM_MEDIA_CATALOG:
                    transferByteCount = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
                    break;

                case IOCTL_CDROM_TRACK_ISRC:
                    transferByteCount = sizeof(SUB_Q_TRACK_ISRC);
                    break;
            }

            cdb->SUBCHANNEL.AllocationLength[0] = (UCHAR) (transferByteCount >> 8);
            cdb->SUBCHANNEL.AllocationLength[1] = (UCHAR) (transferByteCount &  0xFF);
            cdb->SUBCHANNEL.OperationCode = SCSIOP_READ_SUB_CHANNEL;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = transferByteCount;
            srb->CdbLength = 10;
            srb->TimeOutValue = deviceExtension->TimeOutValue;

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_PAUSE_AUDIO: {

            cdb->PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
            cdb->PAUSE_RESUME.Action = CDB_AUDIO_PAUSE;

            srb->CdbLength = 10;
            srb->TimeOutValue = deviceExtension->TimeOutValue;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_RESUME_AUDIO: {

            cdb->PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
            cdb->PAUSE_RESUME.Action = CDB_AUDIO_RESUME;

            srb->CdbLength = 10;
            srb->TimeOutValue = deviceExtension->TimeOutValue;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_SEEK_AUDIO_MSF: {

            PCDROM_SEEK_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;
            ULONG                 logicalBlockAddress;

            logicalBlockAddress = MSF_TO_LBA(inputBuffer->M, inputBuffer->S, inputBuffer->F);

            cdb->SEEK.OperationCode      = SCSIOP_SEEK;
            cdb->SEEK.LogicalBlockAddress[0] = ((PFOUR_BYTE)&logicalBlockAddress)->Byte3;
            cdb->SEEK.LogicalBlockAddress[1] = ((PFOUR_BYTE)&logicalBlockAddress)->Byte2;
            cdb->SEEK.LogicalBlockAddress[2] = ((PFOUR_BYTE)&logicalBlockAddress)->Byte1;
            cdb->SEEK.LogicalBlockAddress[3] = ((PFOUR_BYTE)&logicalBlockAddress)->Byte0;

            srb->CdbLength                = 10;
            srb->TimeOutValue             = deviceExtension->TimeOutValue;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_STOP_AUDIO: {

            cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
            cdb->START_STOP.Immediate = 1;
            cdb->START_STOP.Start = 0;
            cdb->START_STOP.LoadEject = 0;

            srb->CdbLength = 6;
            srb->TimeOutValue = deviceExtension->TimeOutValue;

            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_GET_CONTROL: {
            //
            // Allocate buffer for volume control information.
            //

            dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                         MODE_DATA_SIZE);

            if (!dataBuffer) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            MODE_DATA_SIZE,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            //
            // Prepare the MDL
            //

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);
            srb->DataBuffer = dataBuffer;

            RtlZeroMemory(dataBuffer, MODE_DATA_SIZE);

            //
            // Setup for either 6 or 10 byte CDBs.
            //

            if (use6Byte) {

                cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
                cdb->MODE_SENSE.PageCode = CDROM_AUDIO_CONTROL_PAGE;
                cdb->MODE_SENSE.AllocationLength = MODE_DATA_SIZE;

                //
                // Disable block descriptors.
                //

                cdb->MODE_SENSE.Dbd = TRUE;

                srb->CdbLength = 6;
            } else {

                cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
                cdb->MODE_SENSE10.PageCode = CDROM_AUDIO_CONTROL_PAGE;
                cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(MODE_DATA_SIZE >> 8);
                cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(MODE_DATA_SIZE & 0xFF);

                //
                // Disable block descriptors.
                //

                cdb->MODE_SENSE10.Dbd = TRUE;

                srb->CdbLength = 10;
            }

            srb->TimeOutValue = deviceExtension->TimeOutValue;
            srb->DataTransferLength = MODE_DATA_SIZE;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN);

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_GET_VOLUME:
        case IOCTL_CDROM_SET_VOLUME: {

            dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                         MODE_DATA_SIZE);

            if (!dataBuffer) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            MODE_DATA_SIZE,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                IoStartNextPacket(DeviceObject, FALSE);
                return;
            }

            //
            // Prepare the MDL
            //

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);
            srb->DataBuffer = dataBuffer;

            RtlZeroMemory(dataBuffer, MODE_DATA_SIZE);


            if (use6Byte) {
                cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
                cdb->MODE_SENSE.PageCode = CDROM_AUDIO_CONTROL_PAGE;
                cdb->MODE_SENSE.AllocationLength = MODE_DATA_SIZE;

                srb->CdbLength = 6;

            } else {

                cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
                cdb->MODE_SENSE10.PageCode = CDROM_AUDIO_CONTROL_PAGE;
                cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(MODE_DATA_SIZE >> 8);
                cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(MODE_DATA_SIZE & 0xFF);

                srb->CdbLength = 10;
            }

            srb->TimeOutValue = deviceExtension->TimeOutValue;
            srb->DataTransferLength = MODE_DATA_SIZE;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN);

            if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_SET_VOLUME) {

                //
                // Setup a different completion routine as the mode sense data is needed in order
                // to send the mode select.
                //

                IoSetCompletionRoutine(irp2,
                                       CdRomSetVolumeIntermediateCompletion,
                                       srb,
                                       TRUE,
                                       TRUE,
                                       TRUE);

            }

            IoCallDriver(deviceExtension->PortDeviceObject, irp2);
            return;

        }

        default:

            //
            // Just complete the request - CdRomClassIoctlCompletion will take
            // care of it for us
            //

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            ExFreePool(senseBuffer);
            ExFreePool(srb);
            IoFreeIrp(irp2);
            return;

        } // end switch()
    }

    //
    // If a read or an unhandled IRP_MJ_XX, end up here. The unhandled IRP_MJ's
    // are expected and composed of AutoRun Irps, at present.
    //

    IoCallDriver(deviceExtension->PortDeviceObject, Irp);
    return;
}


NTSTATUS
NTAPI
ScsiCdRomReadVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the entry called by the I/O system for read requests.
    It builds the SRB and sends it to the port driver.

Arguments:

    DeviceObject - the system object for the device.
    Irp - IRP involved.

Return Value:

    NT Status

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG               transferByteCount = currentIrpStack->Parameters.Read.Length;
    LARGE_INTEGER       startingOffset = currentIrpStack->Parameters.Read.ByteOffset;

    //
    // If the cd is playing music then reject this request.
    //

    if (PLAY_ACTIVE(deviceExtension)) {
        Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        return STATUS_DEVICE_BUSY;
    }

    //
    // Verify parameters of this request.
    // Check that ending sector is on disc and
    // that number of bytes to transfer is a multiple of
    // the sector size.
    //

    startingOffset.QuadPart = currentIrpStack->Parameters.Read.ByteOffset.QuadPart +
                              transferByteCount;

    if (!deviceExtension->DiskGeometry->Geometry.BytesPerSector) {
        deviceExtension->DiskGeometry->Geometry.BytesPerSector = 2048;
    }

    if ((startingOffset.QuadPart > deviceExtension->PartitionLength.QuadPart) ||
        (transferByteCount & (deviceExtension->DiskGeometry->Geometry.BytesPerSector - 1))) {

        DebugPrint((1,"ScsiCdRomRead: Invalid I/O parameters\n"));
        DebugPrint((1, "\toffset %x:%x, Length %x:%x\n",
                    startingOffset.u.HighPart,
                    startingOffset.u.LowPart,
                    deviceExtension->PartitionLength.u.HighPart,
                    deviceExtension->PartitionLength.u.LowPart));
        DebugPrint((1, "\tbps %x\n", deviceExtension->DiskGeometry->Geometry.BytesPerSector));

        //
        // Fail request with status of invalid parameters.
        //

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

        return STATUS_INVALID_PARAMETER;
    }


    return STATUS_SUCCESS;

} // end ScsiCdRomReadVerification()


NTSTATUS
NTAPI
CdRomDeviceControlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION   physicalExtension = deviceExtension->PhysicalDevice->DeviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation(Irp);
    PCDROM_DATA         cdData = (PCDROM_DATA)(deviceExtension + 1);
    BOOLEAN             use6Byte = cdData->XAFlags & XA_USE_6_BYTE;
    PIO_STACK_LOCATION  realIrpStack;
    PIO_STACK_LOCATION  realIrpNextStack;
    PSCSI_REQUEST_BLOCK srb     = Context;
    PIRP                realIrp = NULL;
    NTSTATUS            status;
    BOOLEAN             retry;

    //
    // Extract the 'real' irp from the irpstack.
    //

    realIrp = (PIRP) irpStack->Parameters.Others.Argument2;
    realIrpStack = IoGetCurrentIrpStackLocation(realIrp);
    realIrpNextStack = IoGetNextIrpStackLocation(realIrp);

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DebugPrint((2,
                    "CdRomDeviceControlCompletion: Irp %lx, Srb %lx Real Irp %lx Status %lx\n",
                    Irp,
                    srb,
                    realIrp,
                    srb->SrbStatus));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            DebugPrint((2, "CdRomDeviceControlCompletion: Releasing Queue\n"));
            ScsiClassReleaseQueue(DeviceObject);
        }


        retry = ScsiClassInterpretSenseInfo(DeviceObject,
                                            srb,
                                            irpStack->MajorFunction,
                                            irpStack->Parameters.DeviceIoControl.IoControlCode,
                                            MAXIMUM_RETRIES - ((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1),
                                            &status);

        DebugPrint((2, "CdRomDeviceControlCompletion: IRP will %sbe retried\n",
                    (retry ? "" : "not ")));

        //
        // Some of the Device Controls need special cases on non-Success status's.
        //

        if (realIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
            if ((realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_GET_LAST_SESSION) ||
                (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_READ_TOC)         ||
                (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_GET_CONTROL)      ||
                (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_GET_VOLUME)) {

                if (status == STATUS_DATA_OVERRUN) {
                    status = STATUS_SUCCESS;
                    retry = FALSE;
                }
            }

            if (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_READ_Q_CHANNEL) {
                PLAY_ACTIVE(deviceExtension) = FALSE;
            }
        }

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (realIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME &&
            status == STATUS_VERIFY_REQUIRED) {

            if (((realIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) ||
                (realIrpStack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)) &&
                (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_CHECK_VERIFY)) {

                ExFreePool(srb->SenseInfoBuffer);
                if (srb->DataBuffer) {
                    ExFreePool(srb->DataBuffer);
                }
                ExFreePool(srb);
                if (Irp->MdlAddress) {
                    IoFreeMdl(Irp->MdlAddress);
                }

                IoFreeIrp(Irp);

                //
                // Update the geometry information, as the media could have changed.
                // The completion routine for this will complete the real irp and start
                // the next packet.
                //

                status = CdRomUpdateCapacity(deviceExtension,realIrp, NULL);
                DebugPrint((2, "CdRomDeviceControlCompletion: [%lx] CdRomUpdateCapacity completed with status %lx\n", realIrp, status));
                ASSERT(status == STATUS_PENDING);

                return STATUS_MORE_PROCESSING_REQUIRED;

            } else {

                status = STATUS_IO_DEVICE_ERROR;
                retry = TRUE;
            }

        }

        if (retry && (realIrpNextStack->Parameters.Others.Argument1 = (PVOID)((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1-1))) {


            if (((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1)) {

                //
                // Retry request.
                //

                DebugPrint((1, "Retry request %lx - Calling StartIo\n", Irp));


                ExFreePool(srb->SenseInfoBuffer);
                if (srb->DataBuffer) {
                    ExFreePool(srb->DataBuffer);
                }
                ExFreePool(srb);
                if (Irp->MdlAddress) {
                    IoFreeMdl(Irp->MdlAddress);
                }

                IoFreeIrp(Irp);

                //
                // Call StartIo directly since IoStartNextPacket hasn't been called,
                // the serialisation is still intact.
                //

                ScsiCdRomStartIo(DeviceObject, realIrp);
                return STATUS_MORE_PROCESSING_REQUIRED;

            }

            //
            // Exhausted retries. Fall through and complete the request with the appropriate status.
            //

        }
    } else {

        //
        // Set status for successful request.
        //

        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {

        switch (realIrpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_CDROM_GET_DRIVE_GEOMETRY: {

            PREAD_CAPACITY_DATA readCapacityBuffer = srb->DataBuffer;
            ULONG               lastSector;
            ULONG               bps;
            ULONG               lastBit;
            ULONG               tmp;

            //
            // Swizzle bytes from Read Capacity and translate into
            // the necessary geometry information in the device extension.
            //

            tmp = readCapacityBuffer->BytesPerBlock;
            ((PFOUR_BYTE)&bps)->Byte0 = ((PFOUR_BYTE)&tmp)->Byte3;
            ((PFOUR_BYTE)&bps)->Byte1 = ((PFOUR_BYTE)&tmp)->Byte2;
            ((PFOUR_BYTE)&bps)->Byte2 = ((PFOUR_BYTE)&tmp)->Byte1;
            ((PFOUR_BYTE)&bps)->Byte3 = ((PFOUR_BYTE)&tmp)->Byte0;

            //
            // Insure that bps is a power of 2.
            // This corrects a problem with the HP 4020i CDR where it
            // returns an incorrect number for bytes per sector.
            //

            if (!bps) {
                bps = 2048;
            } else {
                lastBit = (ULONG) -1;
                while (bps) {
                    lastBit++;
                    bps = bps >> 1;
                }

                bps = 1 << lastBit;
            }
            deviceExtension->DiskGeometry->Geometry.BytesPerSector = bps;

            DebugPrint((2,
                        "CdRomDeviceControlCompletion: Calculated bps %#x\n",
                        deviceExtension->DiskGeometry->Geometry.BytesPerSector));

            //
            // Copy last sector in reverse byte order.
            //

            tmp = readCapacityBuffer->LogicalBlockAddress;
            ((PFOUR_BYTE)&lastSector)->Byte0 = ((PFOUR_BYTE)&tmp)->Byte3;
            ((PFOUR_BYTE)&lastSector)->Byte1 = ((PFOUR_BYTE)&tmp)->Byte2;
            ((PFOUR_BYTE)&lastSector)->Byte2 = ((PFOUR_BYTE)&tmp)->Byte1;
            ((PFOUR_BYTE)&lastSector)->Byte3 = ((PFOUR_BYTE)&tmp)->Byte0;

            //
            // Calculate sector to byte shift.
            //

            WHICH_BIT(bps, deviceExtension->SectorShift);

            DebugPrint((2,"SCSI ScsiClassReadDriveCapacity: Sector size is %d\n",
                deviceExtension->DiskGeometry->Geometry.BytesPerSector));

            DebugPrint((2,"SCSI ScsiClassReadDriveCapacity: Number of Sectors is %d\n",
                lastSector + 1));

            //
            // Calculate media capacity in bytes.
            //

            deviceExtension->PartitionLength.QuadPart = (LONGLONG)(lastSector + 1);

            //
            // Calculate number of cylinders.
            //

            deviceExtension->DiskGeometry->Geometry.Cylinders.QuadPart = (LONGLONG)((lastSector + 1)/(32 * 64));

            deviceExtension->PartitionLength.QuadPart =
                (deviceExtension->PartitionLength.QuadPart << deviceExtension->SectorShift);

            if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {

                //
                // This device supports removable media.
                //

                deviceExtension->DiskGeometry->Geometry.MediaType = RemovableMedia;

            } else {

                //
                // Assume media type is fixed disk.
                //

                deviceExtension->DiskGeometry->Geometry.MediaType = FixedMedia;
            }

            //
            // Assume sectors per track are 32;
            //

            deviceExtension->DiskGeometry->Geometry.SectorsPerTrack = 32;

            //
            // Assume tracks per cylinder (number of heads) is 64.
            //

            deviceExtension->DiskGeometry->Geometry.TracksPerCylinder = 64;

            //
            // Copy the device extension's geometry info into the user buffer.
            //

            RtlMoveMemory(realIrp->AssociatedIrp.SystemBuffer,
                          deviceExtension->DiskGeometry,
                          sizeof(DISK_GEOMETRY));

            //
            // update information field.
            //

            realIrp->IoStatus.Information = sizeof(DISK_GEOMETRY);
            break;
        }

        case IOCTL_CDROM_CHECK_VERIFY:

            if((realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_CHECK_VERIFY) &&
               (realIrpStack->Parameters.DeviceIoControl.OutputBufferLength)) {

                *((PULONG)realIrp->AssociatedIrp.SystemBuffer) =
                    physicalExtension->MediaChangeCount;
                realIrp->IoStatus.Information = sizeof(ULONG);
            } else {
                realIrp->IoStatus.Information = 0;
            }

            DebugPrint((2, "CdRomDeviceControlCompletion: [%lx] completing CHECK_VERIFY buddy irp %lx\n", realIrp, Irp));
            break;

        case IOCTL_CDROM_GET_LAST_SESSION:
        case IOCTL_CDROM_READ_TOC: {

                PCDROM_TOC toc = srb->DataBuffer;

                //
                // Copy the device extension's geometry info into the user buffer.
                //

                RtlMoveMemory(realIrp->AssociatedIrp.SystemBuffer,
                              toc,
                              srb->DataTransferLength);

                //
                // update information field.
                //

                realIrp->IoStatus.Information = srb->DataTransferLength;
                break;
            }

        case IOCTL_CDROM_PLAY_AUDIO_MSF:

            PLAY_ACTIVE(deviceExtension) = TRUE;

            break;

        case IOCTL_CDROM_READ_Q_CHANNEL: {

            PSUB_Q_CHANNEL_DATA userChannelData = realIrp->AssociatedIrp.SystemBuffer;
#if DBG
            PCDROM_SUB_Q_DATA_FORMAT inputBuffer = realIrp->AssociatedIrp.SystemBuffer;
#endif
            PSUB_Q_CHANNEL_DATA subQPtr = srb->DataBuffer;

#if DBG
            switch( inputBuffer->Format ) {

            case IOCTL_CDROM_CURRENT_POSITION:
                DebugPrint((2,"CdRomDeviceControlCompletion: Audio Status is %u\n", subQPtr->CurrentPosition.Header.AudioStatus ));
                DebugPrint((2,"CdRomDeviceControlCompletion: ADR = 0x%x\n", subQPtr->CurrentPosition.ADR ));
                DebugPrint((2,"CdRomDeviceControlCompletion: Control = 0x%x\n", subQPtr->CurrentPosition.Control ));
                DebugPrint((2,"CdRomDeviceControlCompletion: Track = %u\n", subQPtr->CurrentPosition.TrackNumber ));
                DebugPrint((2,"CdRomDeviceControlCompletion: Index = %u\n", subQPtr->CurrentPosition.IndexNumber ));
                DebugPrint((2,"CdRomDeviceControlCompletion: Absolute Address = %x\n", *((PULONG)subQPtr->CurrentPosition.AbsoluteAddress) ));
                DebugPrint((2,"CdRomDeviceControlCompletion: Relative Address = %x\n", *((PULONG)subQPtr->CurrentPosition.TrackRelativeAddress) ));
                break;

            case IOCTL_CDROM_MEDIA_CATALOG:
                DebugPrint((2,"CdRomDeviceControlCompletion: Audio Status is %u\n", subQPtr->MediaCatalog.Header.AudioStatus ));
                DebugPrint((2,"CdRomDeviceControlCompletion: Mcval is %u\n", subQPtr->MediaCatalog.Mcval ));
                break;

            case IOCTL_CDROM_TRACK_ISRC:
                DebugPrint((2,"CdRomDeviceControlCompletion: Audio Status is %u\n", subQPtr->TrackIsrc.Header.AudioStatus ));
                DebugPrint((2,"CdRomDeviceControlCompletion: Tcval is %u\n", subQPtr->TrackIsrc.Tcval ));
                break;

            }
#endif

            //
            // Update the play active status.
            //

            if (subQPtr->CurrentPosition.Header.AudioStatus == AUDIO_STATUS_IN_PROGRESS) {

                PLAY_ACTIVE(deviceExtension) = TRUE;

            } else {

                PLAY_ACTIVE(deviceExtension) = FALSE;

            }

            //
            // Check if output buffer is large enough to contain
            // the data.
            //

            if (realIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                srb->DataTransferLength) {

                srb->DataTransferLength =
                    realIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            }

            //
            // Copy our buffer into users.
            //

            RtlMoveMemory(userChannelData,
                          subQPtr,
                          srb->DataTransferLength);

            realIrp->IoStatus.Information = srb->DataTransferLength;
            break;
        }

        case IOCTL_CDROM_PAUSE_AUDIO:

            PLAY_ACTIVE(deviceExtension) = FALSE;
            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_RESUME_AUDIO:

            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_SEEK_AUDIO_MSF:

            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_STOP_AUDIO:

            PLAY_ACTIVE(deviceExtension) = FALSE;

            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_GET_CONTROL: {

            PCDROM_AUDIO_CONTROL audioControl = srb->DataBuffer;
            PAUDIO_OUTPUT        audioOutput;
            ULONG                bytesTransferred;

            audioOutput = ScsiClassFindModePage((PCHAR)audioControl,
                                                srb->DataTransferLength,
                                                CDROM_AUDIO_CONTROL_PAGE,
                                                use6Byte);
            //
            // Verify the page is as big as expected.
            //

            bytesTransferred = (PCHAR) audioOutput - (PCHAR) audioControl +
                               sizeof(AUDIO_OUTPUT);

            if (audioOutput != NULL &&
                srb->DataTransferLength >= bytesTransferred) {

                audioControl->LbaFormat = audioOutput->LbaFormat;

                audioControl->LogicalBlocksPerSecond =
                    (audioOutput->LogicalBlocksPerSecond[0] << (UCHAR)8) |
                    audioOutput->LogicalBlocksPerSecond[1];

                realIrp->IoStatus.Information = sizeof(CDROM_AUDIO_CONTROL);

            } else {
                realIrp->IoStatus.Information = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;
        }

        case IOCTL_CDROM_GET_VOLUME: {

            PAUDIO_OUTPUT audioOutput;
            PVOLUME_CONTROL volumeControl = srb->DataBuffer;
            ULONG i,bytesTransferred;

            audioOutput = ScsiClassFindModePage((PCHAR)volumeControl,
                                                 srb->DataTransferLength,
                                                 CDROM_AUDIO_CONTROL_PAGE,
                                                 use6Byte);

            //
            // Verify the page is as big as expected.
            //

            bytesTransferred = (PCHAR) audioOutput - (PCHAR) volumeControl +
                               sizeof(AUDIO_OUTPUT);

            if (audioOutput != NULL &&
                srb->DataTransferLength >= bytesTransferred) {

                for (i=0; i<4; i++) {
                    volumeControl->PortVolume[i] =
                        audioOutput->PortOutput[i].Volume;
                }

                //
                // Set bytes transferred in IRP.
                //

                realIrp->IoStatus.Information = sizeof(VOLUME_CONTROL);

            } else {
                realIrp->IoStatus.Information = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

            break;
        }

        case IOCTL_CDROM_SET_VOLUME:

            realIrp->IoStatus.Information = sizeof(VOLUME_CONTROL);
            break;

        default:

            ASSERT(FALSE);
            realIrp->IoStatus.Information = 0;
            status = STATUS_INVALID_DEVICE_REQUEST;

        } // end switch()
    }

    //
    // Deallocate srb and sense buffer.
    //

    if (srb) {
        if (srb->DataBuffer) {
            ExFreePool(srb->DataBuffer);
        }
        if (srb->SenseInfoBuffer) {
            ExFreePool(srb->SenseInfoBuffer);
        }
        ExFreePool(srb);
    }

    if (realIrp->PendingReturned) {
        IoMarkIrpPending(realIrp);
    }

    if (Irp->MdlAddress) {
        IoFreeMdl(Irp->MdlAddress);
    }

    IoFreeIrp(Irp);

    //
    // Set status in completing IRP.
    //

    realIrp->IoStatus.Status = status;

    //
    // Set the hard error if necessary.
    //

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {

        //
        // Store DeviceObject for filesystem, and clear
        // in IoStatus.Information field.
        //

        DebugPrint((1, "CdRomDeviceCompletion - Setting Hard Error on realIrp %lx\n",
                    realIrp));
        IoSetHardErrorOrVerifyDevice(realIrp, DeviceObject);
        realIrp->IoStatus.Information = 0;
    }

    IoCompleteRequest(realIrp, IO_DISK_INCREMENT);

    IoStartNextPacket(DeviceObject, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
CdRomSetVolumeIntermediateCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation(Irp);
    PCDROM_DATA         cdData = (PCDROM_DATA)(deviceExtension + 1);
    BOOLEAN             use6Byte = cdData->XAFlags & XA_USE_6_BYTE;
    PIO_STACK_LOCATION  realIrpStack;
    PIO_STACK_LOCATION  realIrpNextStack;
    PSCSI_REQUEST_BLOCK srb     = Context;
    PIRP                realIrp = NULL;
    NTSTATUS            status;
    BOOLEAN             retry;

    //
    // Extract the 'real' irp from the irpstack.
    //

    realIrp = (PIRP) irpStack->Parameters.Others.Argument2;
    realIrpStack = IoGetCurrentIrpStackLocation(realIrp);
    realIrpNextStack = IoGetNextIrpStackLocation(realIrp);

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DebugPrint((2,
                    "CdRomSetVolumeIntermediateCompletion: Irp %lx, Srb %lx Real Irp\n",
                    Irp,
                    srb,
                    realIrp));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ScsiClassReleaseQueue(DeviceObject);
        }


        retry = ScsiClassInterpretSenseInfo(DeviceObject,
                                            srb,
                                            irpStack->MajorFunction,
                                            irpStack->Parameters.DeviceIoControl.IoControlCode,
                                            MAXIMUM_RETRIES - ((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1),
                                            &status);

        if (status == STATUS_DATA_OVERRUN) {
            status = STATUS_SUCCESS;
            retry = FALSE;
        }

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (realIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME &&
            status == STATUS_VERIFY_REQUIRED) {

            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        if (retry && (realIrpNextStack->Parameters.Others.Argument1 = (PVOID)((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1-1))) {

            if (((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1)) {

                //
                // Retry request.
                //

                DebugPrint((1, "Retry request %lx - Calling StartIo\n", Irp));


                ExFreePool(srb->SenseInfoBuffer);
                ExFreePool(srb->DataBuffer);
                ExFreePool(srb);
                if (Irp->MdlAddress) {
                    IoFreeMdl(Irp->MdlAddress);
                }

                IoFreeIrp(Irp);

                //
                // Call StartIo directly since IoStartNextPacket hasn't been called,
                // the serialisation is still intact.
                //
                ScsiCdRomStartIo(DeviceObject, realIrp);
                return STATUS_MORE_PROCESSING_REQUIRED;

            }

            //
            // Exhausted retries. Fall through and complete the request with the appropriate status.
            //

        }
    } else {

        //
        // Set status for successful request.
        //

        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {

        PAUDIO_OUTPUT   audioInput = NULL;
        PAUDIO_OUTPUT   audioOutput;
        PVOLUME_CONTROL volumeControl = realIrp->AssociatedIrp.SystemBuffer;
        ULONG           i,bytesTransferred,headerLength;
        PVOID           dataBuffer;
        PCDB            cdb;

        audioInput = ScsiClassFindModePage((PCHAR)srb->DataBuffer,
                                             srb->DataTransferLength,
                                             CDROM_AUDIO_CONTROL_PAGE,
                                             use6Byte);

        //
        // Check to make sure the mode sense data is valid before we go on
        //

        if(audioInput == NULL) {

            DebugPrint((1, "Mode Sense Page %d not found\n",
                        CDROM_AUDIO_CONTROL_PAGE));

            realIrp->IoStatus.Information = 0;
            realIrp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
            IoCompleteRequest(realIrp, IO_DISK_INCREMENT);
            ExFreePool(srb->SenseInfoBuffer);
            ExFreePool(srb);
            IoFreeMdl(Irp->MdlAddress);
            IoFreeIrp(Irp);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        if (use6Byte) {
            headerLength = sizeof(MODE_PARAMETER_HEADER);
        } else {
            headerLength = sizeof(MODE_PARAMETER_HEADER10);
        }

        bytesTransferred = sizeof(AUDIO_OUTPUT) + headerLength;

        //
        // Allocate a new buffer for the mode select.
        //

        dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned, bytesTransferred);

        if (!dataBuffer) {
            realIrp->IoStatus.Information = 0;
            realIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(realIrp, IO_DISK_INCREMENT);
            ExFreePool(srb->SenseInfoBuffer);
            ExFreePool(srb);
            IoFreeMdl(Irp->MdlAddress);
            IoFreeIrp(Irp);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        RtlZeroMemory(dataBuffer, bytesTransferred);

        //
        // Rebuild the data buffer to include the user requested values.
        //

        audioOutput = (PAUDIO_OUTPUT) ((PCHAR) dataBuffer + headerLength);

        for (i=0; i<4; i++) {
            audioOutput->PortOutput[i].Volume =
                volumeControl->PortVolume[i];
            audioOutput->PortOutput[i].ChannelSelection =
                audioInput->PortOutput[i].ChannelSelection;
        }

        audioOutput->CodePage = CDROM_AUDIO_CONTROL_PAGE;
        audioOutput->ParameterLength = sizeof(AUDIO_OUTPUT) - 2;
        audioOutput->Immediate = MODE_SELECT_IMMEDIATE;

        //
        // Free the old data buffer, mdl.
        //

        ExFreePool(srb->DataBuffer);
        IoFreeMdl(Irp->MdlAddress);

        //
        // rebuild the srb.
        //

        cdb = (PCDB)srb->Cdb;
        RtlZeroMemory(cdb, CDB12GENERIC_LENGTH);

        srb->SrbStatus = srb->ScsiStatus = 0;
        srb->SrbFlags = deviceExtension->SrbFlags;
        srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_OUT);
        srb->DataTransferLength = bytesTransferred;

        if (use6Byte) {

            cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
            cdb->MODE_SELECT.ParameterListLength = (UCHAR) bytesTransferred;
            cdb->MODE_SELECT.PFBit = 1;
            srb->CdbLength = 6;
        } else {

            cdb->MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
            cdb->MODE_SELECT10.ParameterListLength[0] = (UCHAR) (bytesTransferred >> 8);
            cdb->MODE_SELECT10.ParameterListLength[1] = (UCHAR) (bytesTransferred & 0xFF);
            cdb->MODE_SELECT10.PFBit = 1;
            srb->CdbLength = 10;
        }

        //
        // Prepare the MDL
        //

        Irp->MdlAddress = IoAllocateMdl(dataBuffer,
                                        bytesTransferred,
                                        FALSE,
                                        FALSE,
                                        (PIRP) NULL);

        if (!Irp->MdlAddress) {
            realIrp->IoStatus.Information = 0;
            realIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(realIrp, IO_DISK_INCREMENT);
            ExFreePool(srb->SenseInfoBuffer);
            ExFreePool(srb);
            ExFreePool(dataBuffer);
            IoFreeIrp(Irp);
            return STATUS_MORE_PROCESSING_REQUIRED;

        }

        MmBuildMdlForNonPagedPool(Irp->MdlAddress);
        srb->DataBuffer = dataBuffer;

        irpStack = IoGetNextIrpStackLocation(Irp);
        irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
        irpStack->Parameters.Scsi.Srb = srb;

        //
        // reset the irp completion.
        //

        IoSetCompletionRoutine(Irp,
                               CdRomDeviceControlCompletion,
                               srb,
                               TRUE,
                               TRUE,
                               TRUE);
        //
        // Call the port driver.
        //

        IoCallDriver(deviceExtension->PortDeviceObject, Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Deallocate srb and sense buffer.
    //

    if (srb) {
        if (srb->DataBuffer) {
            ExFreePool(srb->DataBuffer);
        }
        if (srb->SenseInfoBuffer) {
            ExFreePool(srb->SenseInfoBuffer);
        }
        ExFreePool(srb);
    }

    if (Irp->PendingReturned) {
      IoMarkIrpPending(Irp);
    }

    if (realIrp->PendingReturned) {
        IoMarkIrpPending(realIrp);
    }

    if (Irp->MdlAddress) {
        IoFreeMdl(Irp->MdlAddress);
    }

    IoFreeIrp(Irp);

    //
    // Set status in completing IRP.
    //

    realIrp->IoStatus.Status = status;

    //
    // Set the hard error if necessary.
    //

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {

        //
        // Store DeviceObject for filesystem, and clear
        // in IoStatus.Information field.
        //

        IoSetHardErrorOrVerifyDevice(realIrp, DeviceObject);
        realIrp->IoStatus.Information = 0;
    }

    IoCompleteRequest(realIrp, IO_DISK_INCREMENT);

    IoStartNextPacket(DeviceObject, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
NTAPI
CdRomSwitchModeCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation(Irp);
    PCDROM_DATA         cdData = (PCDROM_DATA)(deviceExtension + 1);
    PIO_STACK_LOCATION  realIrpStack;
    PIO_STACK_LOCATION  realIrpNextStack;
    PSCSI_REQUEST_BLOCK srb     = Context;
    PIRP                realIrp = NULL;
    NTSTATUS            status;
    BOOLEAN             retry;

    //
    // Extract the 'real' irp from the irpstack.
    //

    realIrp = (PIRP) irpStack->Parameters.Others.Argument2;
    realIrpStack = IoGetCurrentIrpStackLocation(realIrp);
    realIrpNextStack = IoGetNextIrpStackLocation(realIrp);

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DebugPrint((2,
                    "CdRomSetVolumeIntermediateCompletion: Irp %lx, Srb %lx Real Irp\n",
                    Irp,
                    srb,
                    realIrp));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ScsiClassReleaseQueue(DeviceObject);
        }


        retry = ScsiClassInterpretSenseInfo(DeviceObject,
                                            srb,
                                            irpStack->MajorFunction,
                                            irpStack->Parameters.DeviceIoControl.IoControlCode,
                                            MAXIMUM_RETRIES - ((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1),
                                            &status);

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (realIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME &&
            status == STATUS_VERIFY_REQUIRED) {

            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        if (retry && (realIrpNextStack->Parameters.Others.Argument1 = (PVOID)((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1-1))) {

            if (((ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1)) {

                //
                // Retry request.
                //

                DebugPrint((1, "Retry request %lx - Calling StartIo\n", Irp));


                ExFreePool(srb->SenseInfoBuffer);
                ExFreePool(srb->DataBuffer);
                ExFreePool(srb);
                if (Irp->MdlAddress) {
                    IoFreeMdl(Irp->MdlAddress);
                }

                IoFreeIrp(Irp);

                //
                // Call StartIo directly since IoStartNextPacket hasn't been called,
                // the serialisation is still intact.
                //

                ScsiCdRomStartIo(DeviceObject, realIrp);
                return STATUS_MORE_PROCESSING_REQUIRED;

            }

            //
            // Exhausted retries. Fall through and complete the request with the appropriate status.
            //
        }
    } else {

        //
        // Set status for successful request.
        //

        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {

        ULONG sectorSize, startingSector, transferByteCount;
        PCDB cdb;

        //
        // Update device ext. to show which mode we are currently using.
        //

        sectorSize =  cdData->u1.BlockDescriptor.BlockLength[0] << 16;
        sectorSize |= (cdData->u1.BlockDescriptor.BlockLength[1] << 8);
        sectorSize |= (cdData->u1.BlockDescriptor.BlockLength[2]);

        cdData->RawAccess = (sectorSize == RAW_SECTOR_SIZE) ? TRUE : FALSE;

        //
        // Free the old data buffer, mdl.
        //

        ExFreePool(srb->DataBuffer);
        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);

        //
        // rebuild the srb.
        //

        cdb = (PCDB)srb->Cdb;
        RtlZeroMemory(cdb, CDB12GENERIC_LENGTH);


        if (cdData->RawAccess) {

            PRAW_READ_INFO rawReadInfo =
                               (PRAW_READ_INFO)realIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;

            ULONG maximumTransferLength;
            ULONG transferPages;

            //
            // Calculate starting offset.
            //

            startingSector = (ULONG)(rawReadInfo->DiskOffset.QuadPart >> deviceExtension->SectorShift);
            transferByteCount  = rawReadInfo->SectorCount * RAW_SECTOR_SIZE;
            maximumTransferLength = deviceExtension->PortCapabilities->MaximumTransferLength;
            transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(realIrp->MdlAddress),
                                                           transferByteCount);

            //
            // Determine if request is within limits imposed by miniport.
            // If the request is larger than the miniport's capabilities, split it.
            //

            if (transferByteCount > maximumTransferLength ||
                transferPages > deviceExtension->PortCapabilities->MaximumPhysicalPages) {

                realIrp->IoStatus.Information = 0;
                realIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                IoCompleteRequest(realIrp, IO_DISK_INCREMENT);

                ExFreePool(srb->SenseInfoBuffer);
                ExFreePool(srb);

                IoStartNextPacket(DeviceObject, FALSE);

                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            srb->OriginalRequest = realIrp;
            srb->SrbFlags = deviceExtension->SrbFlags;
            srb->SrbFlags |= (SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = transferByteCount;
            srb->TimeOutValue = deviceExtension->TimeOutValue;
            srb->CdbLength = 10;
            srb->DataBuffer = MmGetMdlVirtualAddress(realIrp->MdlAddress);

            if (rawReadInfo->TrackMode == CDDA) {
                if (cdData->XAFlags & PLEXTOR_CDDA) {

                    srb->CdbLength = 12;

                    cdb->PLXTR_READ_CDDA.LogicalUnitNumber = deviceExtension->Lun;
                    cdb->PLXTR_READ_CDDA.LogicalBlockByte3  = (UCHAR) (startingSector & 0xFF);
                    cdb->PLXTR_READ_CDDA.LogicalBlockByte2  = (UCHAR) ((startingSector >>  8) & 0xFF);
                    cdb->PLXTR_READ_CDDA.LogicalBlockByte1  = (UCHAR) ((startingSector >> 16) & 0xFF);
                    cdb->PLXTR_READ_CDDA.LogicalBlockByte0  = (UCHAR) ((startingSector >> 24) & 0xFF);

                    cdb->PLXTR_READ_CDDA.TransferBlockByte3 = (UCHAR) (rawReadInfo->SectorCount & 0xFF);
                    cdb->PLXTR_READ_CDDA.TransferBlockByte2 = (UCHAR) (rawReadInfo->SectorCount >> 8);
                    cdb->PLXTR_READ_CDDA.TransferBlockByte1 = 0;
                    cdb->PLXTR_READ_CDDA.TransferBlockByte0 = 0;

                    cdb->PLXTR_READ_CDDA.SubCode = 0;
                    cdb->PLXTR_READ_CDDA.OperationCode = 0xD8;

                } else if (cdData->XAFlags & NEC_CDDA) {

                    cdb->NEC_READ_CDDA.LogicalBlockByte3  = (UCHAR) (startingSector & 0xFF);
                    cdb->NEC_READ_CDDA.LogicalBlockByte2  = (UCHAR) ((startingSector >>  8) & 0xFF);
                    cdb->NEC_READ_CDDA.LogicalBlockByte1  = (UCHAR) ((startingSector >> 16) & 0xFF);
                    cdb->NEC_READ_CDDA.LogicalBlockByte0  = (UCHAR) ((startingSector >> 24) & 0xFF);

                    cdb->NEC_READ_CDDA.TransferBlockByte1 = (UCHAR) (rawReadInfo->SectorCount & 0xFF);
                    cdb->NEC_READ_CDDA.TransferBlockByte0 = (UCHAR) (rawReadInfo->SectorCount >> 8);

                    cdb->NEC_READ_CDDA.OperationCode = 0xD4;
                }
            } else {
                cdb->CDB10.LogicalUnitNumber = deviceExtension->Lun;

                cdb->CDB10.TransferBlocksMsb  = (UCHAR) (rawReadInfo->SectorCount >> 8);
                cdb->CDB10.TransferBlocksLsb  = (UCHAR) (rawReadInfo->SectorCount & 0xFF);

                cdb->CDB10.LogicalBlockByte3  = (UCHAR) (startingSector & 0xFF);
                cdb->CDB10.LogicalBlockByte2  = (UCHAR) ((startingSector >>  8) & 0xFF);
                cdb->CDB10.LogicalBlockByte1  = (UCHAR) ((startingSector >> 16) & 0xFF);
                cdb->CDB10.LogicalBlockByte0  = (UCHAR) ((startingSector >> 24) & 0xFF);

                cdb->CDB10.OperationCode = SCSIOP_READ;
            }

            srb->SrbStatus = srb->ScsiStatus = 0;


            irpStack = IoGetNextIrpStackLocation(realIrp);
            irpStack->MajorFunction = IRP_MJ_SCSI;
            irpStack->Parameters.Scsi.Srb = srb;

            if (!(irpStack->Parameters.Others.Argument1)) {

                //
                // Only jam this in if it doesn't exist. The completion routines can
                // call StartIo directly in the case of retries and resetting it will
                // cause infinite loops.
                //

                irpStack->Parameters.Others.Argument1 = (PVOID) MAXIMUM_RETRIES;
            }

            //
            // Set up IoCompletion routine address.
            //

            IoSetCompletionRoutine(realIrp,
                                   CdRomXACompletion,
                                   srb,
                                   TRUE,
                                   TRUE,
                                   TRUE);
        } else {

            ULONG maximumTransferLength = deviceExtension->PortCapabilities->MaximumTransferLength;
            ULONG transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(realIrp->MdlAddress),
                                                                 realIrpStack->Parameters.Read.Length);
            //
            // Back to cooked sectors. Build and send a normal read.
            // The real work for setting offsets and checking for splitrequests was
            // done in startio
            //

            if ((realIrpStack->Parameters.Read.Length > maximumTransferLength) ||
                (transferPages > deviceExtension->PortCapabilities->MaximumPhysicalPages)) {

                //
                // Request needs to be split. Completion of each portion of the request will
                // fire off the next portion. The final request will signal Io to send a new request.
                //

                ScsiClassSplitRequest(DeviceObject, realIrp, maximumTransferLength);
                return STATUS_MORE_PROCESSING_REQUIRED;

            } else {

                //
                // Build SRB and CDB for this IRP.
                //

                ScsiClassBuildRequest(DeviceObject, realIrp);

            }
        }

        //
        // Call the port driver.
        //

        IoCallDriver(deviceExtension->PortDeviceObject, realIrp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Update device Extension flags to indicate that XA isn't supported.
    //

    cdData->XAFlags |= XA_NOT_SUPPORTED;

    //
    // Deallocate srb and sense buffer.
    //

    if (srb) {
        if (srb->DataBuffer) {
            ExFreePool(srb->DataBuffer);
        }
        if (srb->SenseInfoBuffer) {
            ExFreePool(srb->SenseInfoBuffer);
        }
        ExFreePool(srb);
    }

    if (Irp->PendingReturned) {
      IoMarkIrpPending(Irp);
    }

    if (realIrp->PendingReturned) {
        IoMarkIrpPending(realIrp);
    }

    if (Irp->MdlAddress) {
        IoFreeMdl(Irp->MdlAddress);
    }

    IoFreeIrp(Irp);

    //
    // Set status in completing IRP.
    //

    realIrp->IoStatus.Status = status;

    //
    // Set the hard error if necessary.
    //

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {

        //
        // Store DeviceObject for filesystem, and clear
        // in IoStatus.Information field.
        //

        IoSetHardErrorOrVerifyDevice(realIrp, DeviceObject);
        realIrp->IoStatus.Information = 0;
    }

    IoCompleteRequest(realIrp, IO_DISK_INCREMENT);

    IoStartNextPacket(DeviceObject, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
CdRomXACompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine executes when the port driver has completed a request.
    It looks at the SRB status in the completing SRB and if not success
    it checks for valid request sense buffer information. If valid, the
    info is used to update status with more precise message of type of
    error. This routine deallocates the SRB.

Arguments:

    DeviceObject - Supplies the device object which represents the logical
        unit.

    Irp - Supplies the Irp which has completed.

    Context - Supplies a pointer to the SRB.

Return Value:

    NT status

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION irpNextStack = IoGetNextIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = Context;
    NTSTATUS status;
    BOOLEAN retry;

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        DebugPrint((2,"ScsiClassIoComplete: IRP %lx, SRB %lx\n", Irp, srb));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ScsiClassReleaseQueue(DeviceObject);
        }

        retry = ScsiClassInterpretSenseInfo(
            DeviceObject,
            srb,
            irpStack->MajorFunction,
            irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL ? irpStack->Parameters.DeviceIoControl.IoControlCode : 0,
            MAXIMUM_RETRIES - ((ULONG_PTR)irpNextStack->Parameters.Others.Argument1),
            &status);

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (irpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME &&
            status == STATUS_VERIFY_REQUIRED) {

            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        if (retry && (irpNextStack->Parameters.Others.Argument1 = (PVOID)((ULONG_PTR)irpNextStack->Parameters.Others.Argument1-1))) {

            if (((ULONG_PTR)irpNextStack->Parameters.Others.Argument1)) {

                //
                // Retry request.
                //

                DebugPrint((1, "CdRomXACompletion: Retry request %lx - Calling StartIo\n", Irp));


                ExFreePool(srb->SenseInfoBuffer);
                ExFreePool(srb->DataBuffer);
                ExFreePool(srb);

                //
                // Call StartIo directly since IoStartNextPacket hasn't been called,
                // the serialisation is still intact.
                //

                ScsiCdRomStartIo(DeviceObject, Irp);
                return STATUS_MORE_PROCESSING_REQUIRED;

            }

            //
            // Exhausted retries. Fall through and complete the request with the appropriate status.
            //
        }
    } else {

        //
        // Set status for successful request.
        //

        status = STATUS_SUCCESS;

    } // end if (SRB_STATUS(srb->SrbStatus) ...

    //
    // Return SRB to nonpaged pool.
    //

    ExFreePool(srb);

    //
    // Set status in completing IRP.
    //

    Irp->IoStatus.Status = status;

    //
    // Set the hard error if necessary.
    //

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {

        //
        // Store DeviceObject for filesystem, and clear
        // in IoStatus.Information field.
        //

        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        Irp->IoStatus.Information = 0;
    }

    //
    // If pending has be returned for this irp then mark the current stack as
    // pending.
    //

    if (Irp->PendingReturned) {
      IoMarkIrpPending(Irp);
    }

    //IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    IoStartNextPacket(DeviceObject, FALSE);

    return status;
}

NTSTATUS
NTAPI
CdRomDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the NT device control handler for CDROMs.

Arguments:

    DeviceObject - for this CDROM

    Irp - IO Request packet

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextStack;
    PKEVENT            deviceControlEvent;
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_DATA        cdData = (PCDROM_DATA)(deviceExtension+1);
    SCSI_REQUEST_BLOCK srb;
    NTSTATUS status;
    KIRQL    irql;

    ULONG ioctlCode;
    ULONG baseCode;
    ULONG functionCode;

RetryControl:

    //
    // Zero the SRB on stack.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    Irp->IoStatus.Information = 0;

    //
    // if this is a class driver ioctl then we need to change the base code
    // to IOCTL_CDROM_BASE so that the switch statement can handle it.
    //
    // WARNING - currently the scsi class ioctl function codes are between
    // 0x200 & 0x300.  this routine depends on that fact
    //

    ioctlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    baseCode = ioctlCode >> 16;
    functionCode = (ioctlCode & (~0xffffc003)) >> 2;

    DebugPrint((1, "CdRomDeviceControl: Ioctl Code = %#08lx, Base Code = %#lx,"
                   " Function Code = %#lx\n",
                ioctlCode,
                baseCode,
                functionCode
              ));

    if((functionCode >= 0x200) && (functionCode <= 0x300)) {

        ioctlCode = (ioctlCode & 0x0000ffff) | CTL_CODE(IOCTL_CDROM_BASE, 0, 0, 0);

        DebugPrint((1, "CdRomDeviceControl: Class Code - new ioctl code is %#08lx\n",
                    ioctlCode));

        irpStack->Parameters.DeviceIoControl.IoControlCode = ioctlCode;

    }

    switch (ioctlCode) {

    case IOCTL_CDROM_RAW_READ: {

        LARGE_INTEGER  startingOffset;
        ULONG          transferBytes;
        PRAW_READ_INFO rawReadInfo = (PRAW_READ_INFO)irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
        PUCHAR         userData = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

        //
        // Ensure that XA reads are supported.
        //

        if (cdData->XAFlags & XA_NOT_SUPPORTED) {

            DebugPrint((1,
                        "CdRomDeviceControl: XA Reads not supported. Flags (%x)\n",
                        cdData->XAFlags));

            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        //
        // Check that ending sector is on disc and buffers are there and of
        // correct size.
        //

        if (rawReadInfo == NULL) {

            //
            //  Called from user space. Validate the buffers.
            //

            rawReadInfo = (PRAW_READ_INFO)userData;
            irpStack->Parameters.DeviceIoControl.Type3InputBuffer = (PVOID)userData;

            if (rawReadInfo == NULL) {

                DebugPrint((1,"CdRomDeviceControl: Invalid I/O parameters for XA Read (No extent info\n"));

                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(RAW_READ_INFO)) {

                DebugPrint((1,"CdRomDeviceControl: Invalid I/O parameters for XA Read (Invalid info buffer\n"));

                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INVALID_PARAMETER;
            }
        }

        startingOffset.QuadPart = rawReadInfo->DiskOffset.QuadPart;
        transferBytes = rawReadInfo->SectorCount * RAW_SECTOR_SIZE;

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < transferBytes) {

            DebugPrint((1,"CdRomDeviceControl: Invalid I/O parameters for XA Read (Bad buffer size)\n"));

            //
            // Fail request with status of invalid parameters.
            //

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INVALID_PARAMETER;
        }

        if ((startingOffset.QuadPart + transferBytes) > deviceExtension->PartitionLength.QuadPart) {

            DebugPrint((1,"CdRomDeviceControl: Invalid I/O parameters for XA Read (Request Out of Bounds)\n"));

            //
            // Fail request with status of invalid parameters.
            //

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_GET_DRIVE_GEOMETRY: {

        DebugPrint((2,"CdRomDeviceControl: Get drive geometry\n"));

        if ( irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof( DISK_GEOMETRY ) ) {

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject,Irp, NULL,NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_GET_LAST_SESSION:
    case IOCTL_CDROM_READ_TOC:  {

        //
        // If the cd is playing music then reject this request.
        //

        if (CdRomIsPlayActive(DeviceObject)) {
            Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_DEVICE_BUSY;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_PLAY_AUDIO_MSF: {

        //
        // Play Audio MSF
        //

        DebugPrint((2,"CdRomDeviceControl: Play audio MSF\n"));

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_PLAY_AUDIO_MSF)) {

            //
            // Indicate unsuccessful status.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_SEEK_AUDIO_MSF: {


        //
        // Seek Audio MSF
        //

        DebugPrint((2,"CdRomDeviceControl: Seek audio MSF\n"));

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_SEEK_AUDIO_MSF)) {

            //
            // Indicate unsuccessful status.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        } else {
            IoMarkIrpPending(Irp);
            IoStartPacket(DeviceObject, Irp, NULL, NULL);

            return STATUS_PENDING;

        }
    }

    case IOCTL_CDROM_PAUSE_AUDIO: {

        //
        // Pause audio
        //

        DebugPrint((2, "CdRomDeviceControl: Pause audio\n"));

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;

        break;
    }

    case IOCTL_CDROM_RESUME_AUDIO: {

        //
        // Resume audio
        //

        DebugPrint((2, "CdRomDeviceControl: Resume audio\n"));

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_READ_Q_CHANNEL: {

        if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_SUB_Q_DATA_FORMAT)) {

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_GET_CONTROL: {

        DebugPrint((2, "CdRomDeviceControl: Get audio control\n"));

        //
        // Verify user buffer is large enough for the data.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(CDROM_AUDIO_CONTROL)) {

            //
            // Indicate unsuccessful status and no data transferred.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;

        } else {

            IoMarkIrpPending(Irp);
            IoStartPacket(DeviceObject, Irp, NULL, NULL);

            return STATUS_PENDING;
        }
    }

    case IOCTL_CDROM_GET_VOLUME: {

        DebugPrint((2, "CdRomDeviceControl: Get volume control\n"));

        //
        // Verify user buffer is large enough for data.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(VOLUME_CONTROL)) {

            //
            // Indicate unsuccessful status and no data transferred.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;

        } else {
            IoMarkIrpPending(Irp);
            IoStartPacket(DeviceObject, Irp, NULL, NULL);

            return STATUS_PENDING;
        }
    }

    case IOCTL_CDROM_SET_VOLUME: {

        DebugPrint((2, "CdRomDeviceControl: Set volume control\n"));

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(VOLUME_CONTROL)) {

            //
            // Indicate unsuccessful status.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        } else {

            IoMarkIrpPending(Irp);
            IoStartPacket(DeviceObject, Irp, NULL, NULL);

            return STATUS_PENDING;
        }
    }

    case IOCTL_CDROM_STOP_AUDIO: {

        //
        // Stop play.
        //

        DebugPrint((2, "CdRomDeviceControl: Stop audio\n"));

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject,Irp, NULL,NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_CHECK_VERIFY: {
        DebugPrint((1, "CdRomDeviceControl: [%lx] Check Verify\n", Irp));
        IoMarkIrpPending(Irp);

        if((irpStack->Parameters.DeviceIoControl.OutputBufferLength) &&
           (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))) {

           DebugPrint((1, "CdRomDeviceControl: Check Verify: media count "
                          "buffer too small\n"));

           status = STATUS_BUFFER_TOO_SMALL;
           break;
        }

        IoStartPacket(DeviceObject,Irp, NULL,NULL);

        return STATUS_PENDING;
    }

    default: {

        //
        // allocate an event and stuff it into our stack location.
        //

        deviceControlEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

        if(!deviceControlEvent) {

            status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            PIO_STACK_LOCATION currentStack;

            KeInitializeEvent(deviceControlEvent, NotificationEvent, FALSE);

            currentStack = IoGetCurrentIrpStackLocation(Irp);
            nextStack = IoGetNextIrpStackLocation(Irp);

            //
            // Copy the stack down a notch
            //

            *nextStack = *currentStack;

            IoSetCompletionRoutine(
                Irp,
                CdRomClassIoctlCompletion,
                deviceControlEvent,
                TRUE,
                TRUE,
                TRUE
                );

            IoSetNextIrpStackLocation(Irp);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;

            //
            // Override volume verifies on this stack location so that we
            // will be forced through the synchronization.  Once this location
            // goes away we get the old value back
            //

            nextStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

            IoMarkIrpPending(Irp);

            IoStartPacket(DeviceObject, Irp, NULL, NULL);

            //
            // Wait for CdRomClassIoctlCompletion to set the event. This
            // ensures serialization remains intact for these unhandled device
            // controls.
            //

            KeWaitForSingleObject(
                deviceControlEvent,
                Suspended,
                KernelMode,
                FALSE,
                NULL);

            ExFreePool(deviceControlEvent);

            DebugPrint((2, "CdRomDeviceControl: irp %#08lx synchronized\n", Irp));

            //
            // If an error occured then propagate that back up - we are no longer
            // guaranteed synchronization and the upper layers will have to
            // retry.
            //
            // If no error occured, call down to the class driver directly
            // then start up the next request.
            //

            if(Irp->IoStatus.Status == STATUS_SUCCESS) {

                status = ScsiClassDeviceControl(DeviceObject, Irp);

                KeRaiseIrql(DISPATCH_LEVEL, &irql);

                IoStartNextPacket(DeviceObject, FALSE);

                KeLowerIrql(irql);
            }
        }

        return status;
    }

    } // end switch()

    if (status == STATUS_VERIFY_REQUIRED) {

        //
        // If the status is verified required and this request
        // should bypass verify required then retry the request.
        //

        if (irpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME) {

            status = STATUS_IO_DEVICE_ERROR;
            goto RetryControl;

        }
    }

    if (IoIsErrorUserInduced(status)) {

        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);

    }

    //
    // Update IRP with completion status.
    //

    Irp->IoStatus.Status = status;

    //
    // Complete the request.
    //

    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    DebugPrint((2, "CdRomDeviceControl: Status is %lx\n", status));
    return status;

} // end ScsiCdRomDeviceControl()

VOID
NTAPI
ScanForSpecial(
    PDEVICE_OBJECT DeviceObject,
    PINQUIRYDATA InquiryData,
    PIO_SCSI_CAPABILITIES PortCapabilities
    )

/*++

Routine Description:

    This function checks to see if an SCSI logical unit requires an special
    initialization or error processing.

Arguments:

    DeviceObject - Supplies the device object to be tested.

    InquiryData - Supplies the inquiry data returned by the device of interest.

    PortCapabilities - Supplies the capabilities of the device object.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_DATA       cdData = (PCDROM_DATA)(deviceExtension+1);

    //
    // Look for a Hitachi CDR-1750. Read-ahead must be disabled in order
    // to get this cdrom drive to work on scsi adapters that use PIO.
    //

    if ((strncmp((PCHAR)InquiryData->VendorId, "HITACHI CDR-1750S", strlen("HITACHI CDR-1750S")) == 0 ||
        strncmp((PCHAR)InquiryData->VendorId, "HITACHI CDR-3650/1650S", strlen("HITACHI CDR-3650/1650S")) == 0)
        && PortCapabilities->AdapterUsesPio) {

        DebugPrint((1, "CdRom ScanForSpecial:  Found Hitachi CDR-1750S.\n"));

        //
        // Setup an error handler to reinitialize the cd rom after it is reset.
        //

        deviceExtension->ClassError = HitachProcessError;

    } else if (( RtlCompareMemory( InquiryData->VendorId,"FUJITSU", 7 ) == 7 ) &&
              (( RtlCompareMemory( InquiryData->ProductId,"FMCD-101", 8 ) == 8 ) ||
               ( RtlCompareMemory( InquiryData->ProductId,"FMCD-102", 8 ) == 8 ))) {

        //
        // When Read command is issued to FMCD-101 or FMCD-102 and there is a music
        // cd in it. It takes longer time than SCSI_CDROM_TIMEOUT before returning
        // error status.
        //

        deviceExtension->TimeOutValue = 20;

    } else if (( RtlCompareMemory( InquiryData->VendorId,"TOSHIBA", 7) == 7) &&
              (( RtlCompareMemory( InquiryData->ProductId,"CD-ROM XM-34", 12) == 12))) {

        SCSI_REQUEST_BLOCK srb;
        PCDB               cdb;
        ULONG              length;
        PUCHAR             buffer;
        NTSTATUS           status;

        //
        // Set the density code and the error handler.
        //

        length = (sizeof(MODE_READ_RECOVERY_PAGE) + MODE_BLOCK_DESC_LENGTH + MODE_HEADER_LENGTH);

        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        //
        // Build the MODE SENSE CDB.
        //

        srb.CdbLength = 6;
        cdb = (PCDB)srb.Cdb;

        //
        // Set timeout value from device extension.
        //

        srb.TimeOutValue = deviceExtension->TimeOutValue;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.PageCode = 0x1;
        cdb->MODE_SENSE.AllocationLength = (UCHAR)length;

        buffer = ExAllocatePool(NonPagedPoolCacheAligned, (sizeof(MODE_READ_RECOVERY_PAGE) + MODE_BLOCK_DESC_LENGTH + MODE_HEADER_LENGTH));
        if (!buffer) {
            return;
        }

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             buffer,
                                             length,
                                             FALSE);

        ((PERROR_RECOVERY_DATA)buffer)->BlockDescriptor.DensityCode = 0x83;
        ((PERROR_RECOVERY_DATA)buffer)->Header.ModeDataLength = 0x0;

        RtlCopyMemory(&cdData->u1.Header, buffer, sizeof(ERROR_RECOVERY_DATA));

        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        //
        // Build the MODE SENSE CDB.
        //

        srb.CdbLength = 6;
        cdb = (PCDB)srb.Cdb;

        //
        // Set timeout value from device extension.
        //

        srb.TimeOutValue = deviceExtension->TimeOutValue;

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = 1;
        cdb->MODE_SELECT.ParameterListLength = (UCHAR)length;

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             buffer,
                                             length,
                                             TRUE);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1,
                        "Cdrom.ScanForSpecial: Setting density code on Toshiba failed [%x]\n",
                        status));
        }

        deviceExtension->ClassError = ToshibaProcessError;

        ExFreePool(buffer);

    }

    //
    // Determine special CD-DA requirements.
    //

    if (RtlCompareMemory( InquiryData->VendorId,"PLEXTOR",7) == 7) {
        cdData->XAFlags |= PLEXTOR_CDDA;
    } else if (RtlCompareMemory ( InquiryData->VendorId,"NEC",3) == 3) {
        cdData->XAFlags |= NEC_CDDA;
    }

    return;
}

VOID
NTAPI
HitachProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )
/*++

Routine Description:

   This routine checks the type of error.  If the error indicates CD-ROM the
   CD-ROM needs to be reinitialized then a Mode sense command is sent to the
   device.  This command disables read-ahead for the device.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - Not used.

    Retry - Not used.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PSENSE_DATA         senseBuffer = Srb->SenseInfoBuffer;
    LARGE_INTEGER       largeInt;
    PUCHAR              modePage;
    PIO_STACK_LOCATION  irpStack;
    PIRP                irp;
    PSCSI_REQUEST_BLOCK srb;
    PCOMPLETION_CONTEXT context;
    PCDB                cdb;
    ULONG               alignment;

    UNREFERENCED_PARAMETER(Status);
    UNREFERENCED_PARAMETER(Retry);

    largeInt.QuadPart = (LONGLONG) 1;

    //
    // Check the status.  The initialization command only needs to be sent
    // if UNIT ATTENTION is returned.
    //

    if (!(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)) {

        //
        // The drive does not require reinitialization.
        //

        return;
    }

    //
    // Found a bad HITACHI cd-rom.  These devices do not work with PIO
    // adapters when read-ahead is enabled.  Read-ahead is disabled by
    // a mode select command.  The mode select page code is zero and the
    // length is 6 bytes.  All of the other bytes should be zero.
    //


    if ((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_UNIT_ATTENTION) {

        DebugPrint((1, "HitachiProcessError: Reinitializing the CD-ROM.\n"));

        //
        // Send the special mode select command to disable read-ahead
        // on the CD-ROM reader.
        //

        alignment = DeviceObject->AlignmentRequirement ?
            DeviceObject->AlignmentRequirement : 1;

        context = ExAllocatePool(
            NonPagedPool,
            sizeof(COMPLETION_CONTEXT) +  HITACHI_MODE_DATA_SIZE + alignment
            );

        if (context == NULL) {

            //
            // If there is not enough memory to fulfill this request,
            // simply return. A subsequent retry will fail and another
            // chance to start the unit.
            //

            return;
        }

        context->DeviceObject = DeviceObject;
        srb = &context->Srb;

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

        //
        // Write length to SRB.
        //

        srb->Length = SCSI_REQUEST_BLOCK_SIZE;

        //
        // Set up SCSI bus address.
        //

        srb->PathId = deviceExtension->PathId;
        srb->TargetId = deviceExtension->TargetId;
        srb->Lun = deviceExtension->Lun;

        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->TimeOutValue = deviceExtension->TimeOutValue;

        //
        // Set the transfer length.
        //

        srb->DataTransferLength = HITACHI_MODE_DATA_SIZE;
        srb->SrbFlags = SRB_FLAGS_DATA_OUT | SRB_FLAGS_DISABLE_AUTOSENSE | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

        //
        // The data buffer must be aligned.
        //

        srb->DataBuffer = (PVOID) (((ULONG_PTR) (context + 1) + (alignment - 1)) &
            ~(alignment - 1));


        //
        // Build the HITACHI read-ahead mode select CDB.
        //

        srb->CdbLength = 6;
        cdb = (PCDB)srb->Cdb;
        cdb->MODE_SENSE.LogicalUnitNumber = srb->Lun;
        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SENSE.AllocationLength = HITACHI_MODE_DATA_SIZE;

        //
        // Initialize the mode sense data.
        //

        modePage = srb->DataBuffer;

        RtlZeroMemory(modePage, HITACHI_MODE_DATA_SIZE);

        //
        // Set the page length field to 6.
        //

        modePage[5] = 6;

        //
        // Build the asynchronous request to be sent to the port driver.
        //

        irp = IoBuildAsynchronousFsdRequest(IRP_MJ_WRITE,
                                           DeviceObject,
                                           srb->DataBuffer,
                                           srb->DataTransferLength,
                                           &largeInt,
                                           NULL);

        IoSetCompletionRoutine(irp,
                   (PIO_COMPLETION_ROUTINE)ScsiClassAsynchronousCompletion,
                   context,
                   TRUE,
                   TRUE,
                   TRUE);

        irpStack = IoGetNextIrpStackLocation(irp);

        irpStack->MajorFunction = IRP_MJ_SCSI;

        srb->OriginalRequest = irp;

        //
        // Save SRB address in next stack for port driver.
        //

        irpStack->Parameters.Scsi.Srb = (PVOID)srb;

        //
        // Set up IRP Address.
        //

        (VOID)IoCallDriver(deviceExtension->PortDeviceObject, irp);

    }
}

NTSTATUS
NTAPI
ToshibaProcessErrorCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:

    Completion routine for the ClassError routine to handle older Toshiba units
    that require setting the density code.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Irp - Pointer to irp created to set the density code.

    Context - Supplies a pointer to the Mode Select Srb.


Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{

    PSCSI_REQUEST_BLOCK srb = Context;

    //
    // Check for a frozen queue.
    //

    if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

        //
        // Unfreeze the queue getting the device object from the context.
        //

        ScsiClassReleaseQueue(DeviceObject);
    }

    //
    // Free all of the allocations.
    //

    ExFreePool(srb->DataBuffer);
    ExFreePool(srb);
    IoFreeMdl(Irp->MdlAddress);
    IoFreeIrp(Irp);

    //
    // Indicate the I/O system should stop processing the Irp completion.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
NTAPI
ToshibaProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )

/*++

Routine Description:

   This routine checks the type of error.  If the error indicates a unit attention,
   the density code needs to be set via a Mode select command.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - Not used.

    Retry - Not used.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_DATA         cdData = (PCDROM_DATA)(deviceExtension+1);
    PSENSE_DATA         senseBuffer = Srb->SenseInfoBuffer;
    PIO_STACK_LOCATION  irpStack;
    PIRP                irp;
    PSCSI_REQUEST_BLOCK srb;
    ULONG               length;
    PCDB                cdb;
    PUCHAR              dataBuffer;


    if (!(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)) {
        return;
    }

    //
    // The Toshiba's require the density code to be set on power up and media changes.
    //

    if ((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_UNIT_ATTENTION) {


        irp = IoAllocateIrp((CCHAR)(deviceExtension->DeviceObject->StackSize+1),
                              FALSE);

        if (!irp) {
            return;
        }

        srb = ExAllocatePool(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK));
        if (!srb) {
            IoFreeIrp(irp);
            return;
        }


        length = sizeof(ERROR_RECOVERY_DATA);
        dataBuffer = ExAllocatePool(NonPagedPoolCacheAligned, length);
        if (!dataBuffer) {
            ExFreePool(srb);
            IoFreeIrp(irp);
            return;
        }

        irp->MdlAddress = IoAllocateMdl(dataBuffer,
                                        length,
                                        FALSE,
                                        FALSE,
                                        (PIRP) NULL);

        if (!irp->MdlAddress) {
            ExFreePool(srb);
            ExFreePool(dataBuffer);
            IoFreeIrp(irp);
            return;
        }

        //
        // Prepare the MDL
        //

        MmBuildMdlForNonPagedPool(irp->MdlAddress);

        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        srb->DataBuffer = dataBuffer;
        cdb = (PCDB)srb->Cdb;

        //
        // Set up the irp.
        //

        IoSetNextIrpStackLocation(irp);
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = 0;
        irp->Flags = 0;
        irp->UserBuffer = NULL;

        //
        // Save the device object and irp in a private stack location.
        //

        irpStack = IoGetCurrentIrpStackLocation(irp);
        irpStack->DeviceObject = deviceExtension->DeviceObject;

        //
        // Construct the IRP stack for the lower level driver.
        //

        irpStack = IoGetNextIrpStackLocation(irp);
        irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_OUT;
        irpStack->Parameters.Scsi.Srb = srb;

        IoSetCompletionRoutine(irp,
                               ToshibaProcessErrorCompletion,
                               srb,
                               TRUE,
                               TRUE,
                               TRUE);

        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->PathId = deviceExtension->PathId;
        srb->TargetId = deviceExtension->TargetId;
        srb->Lun = deviceExtension->Lun;
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->Cdb[1] |= deviceExtension->Lun << 5;
        srb->SrbStatus = srb->ScsiStatus = 0;
        srb->NextSrb = 0;
        srb->OriginalRequest = irp;
        srb->SenseInfoBufferLength = 0;

        //
        // Set the transfer length.
        //

        srb->DataTransferLength = length;
        srb->SrbFlags = SRB_FLAGS_DATA_OUT | SRB_FLAGS_DISABLE_AUTOSENSE | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;


        srb->CdbLength = 6;
        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = 1;
        cdb->MODE_SELECT.ParameterListLength = (UCHAR)length;

        //
        // Copy the Mode page into the databuffer.
        //

        RtlCopyMemory(srb->DataBuffer, &cdData->u1.Header, length);

        //
        // Set the density code.
        //

        ((PERROR_RECOVERY_DATA)srb->DataBuffer)->BlockDescriptor.DensityCode = 0x83;

        IoCallDriver(deviceExtension->PortDeviceObject, irp);
    }
}

BOOLEAN
NTAPI
CdRomIsPlayActive(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine determines if the cd is currently playing music.

Arguments:

    DeviceObject - Device object to test.

Return Value:

    TRUE if the device is playing music.

--*/
{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIRP irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    NTSTATUS status;
    PSUB_Q_CURRENT_POSITION currentBuffer;

    if (!PLAY_ACTIVE(deviceExtension)) {
        return(FALSE);
    }

    currentBuffer = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(SUB_Q_CURRENT_POSITION));

    if (currentBuffer == NULL) {
        return(FALSE);
    }

    ((PCDROM_SUB_Q_DATA_FORMAT) currentBuffer)->Format = IOCTL_CDROM_CURRENT_POSITION;
    ((PCDROM_SUB_Q_DATA_FORMAT) currentBuffer)->Track = 0;

    //
    // Create notification event object to be used to signal the
    // request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build the synchronous request  to be sent to the port driver
    // to perform the request.
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_CDROM_READ_Q_CHANNEL,
                                        deviceExtension->DeviceObject,
                                        currentBuffer,
                                        sizeof(CDROM_SUB_Q_DATA_FORMAT),
                                        currentBuffer,
                                        sizeof(SUB_Q_CURRENT_POSITION),
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp == NULL) {
        ExFreePool(currentBuffer);
        return FALSE;
    }

    //
    // Pass request to port driver and wait for request to complete.
    //

    status = IoCallDriver(deviceExtension->DeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(currentBuffer);
        return FALSE;
    }

    ExFreePool(currentBuffer);

    return(PLAY_ACTIVE(deviceExtension));

}

IO_COMPLETION_ROUTINE CdRomMediaChangeCompletion;
NTSTATUS
NTAPI
CdRomMediaChangeCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:

    This routine handles the completion of the test unit ready irps
    used to determine if the media has changed.  If the media has
    changed, this code signals the named event to wake up other
    system services that react to media change (aka AutoPlay).

Arguments:

    DeviceObject - the object for the completion
    Irp - the IRP being completed
    Context - the SRB from the IRP

Return Value:

    NTSTATUS

--*/

{
    PSCSI_REQUEST_BLOCK srb = (PSCSI_REQUEST_BLOCK) Context;
    PIO_STACK_LOCATION  cdStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION  irpNextStack = IoGetNextIrpStackLocation(Irp);
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_EXTENSION   physicalExtension;
    PSENSE_DATA         senseBuffer;
    PCDROM_DATA         cddata;

    ASSERT(Irp);
    ASSERT(cdStack);
    DeviceObject = cdStack->DeviceObject;
    ASSERT(DeviceObject);

    deviceExtension = DeviceObject->DeviceExtension;
    physicalExtension = deviceExtension->PhysicalDevice->DeviceExtension;
    cddata = (PCDROM_DATA)(deviceExtension + 1);

    ASSERT(cddata->MediaChangeIrp == NULL);

    //
    // If the sense data field is valid, look for a media change.
    // otherwise this iteration of the polling will just assume nothing
    // changed.
    //

    DebugPrint((3, "CdRomMediaChangeHandler: Completing Autorun Irp 0x%lx "
                   "for device %d\n",
                   Irp, deviceExtension->DeviceNumber));

    if (srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
        if (srb->SenseInfoBufferLength >= FIELD_OFFSET(SENSE_DATA, CommandSpecificInformation)) {

            //
            // See if this is a media change.
            //

            senseBuffer = srb->SenseInfoBuffer;
            if ((senseBuffer->SenseKey & 0x0f) == SCSI_SENSE_UNIT_ATTENTION)  {
                if (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_MEDIUM_CHANGED) {

                    DebugPrint((1, "CdRomMediaChangeCompletion: New media inserted "
                                   "into CdRom%d [irp = 0x%lx]\n",
                                deviceExtension->DeviceNumber, Irp));

                    //
                    // Media change event occurred - signal the named event.
                    //

                    KeSetEvent(deviceExtension->MediaChangeEvent,
                               (KPRIORITY) 0,
                               FALSE);

                    deviceExtension->MediaChangeNoMedia = FALSE;

                }

                if (DeviceObject->Vpb->Flags & VPB_MOUNTED) {

                    //
                    // Must remember the media changed and force the
                    // file system to verify on next access
                    //

                    DeviceObject->Flags |= DO_VERIFY_VOLUME;
                }

                physicalExtension->MediaChangeCount++;

            } else if(((senseBuffer->SenseKey & 0x0f) == SCSI_SENSE_NOT_READY)&&
                      (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE)&&
                      (!deviceExtension->MediaChangeNoMedia)){

                //
                // If there was no media in the device then signal the waiters if
                // we haven't already done so before.
                //

                DebugPrint((1, "CdRomMediaChangeCompletion: No media in device"
                               "CdRom%d [irp = 0x%lx]\n",
                            deviceExtension->DeviceNumber, Irp));

                KeSetEvent(deviceExtension->MediaChangeEvent,
                           (KPRIORITY) 0,
                           FALSE);

                deviceExtension->MediaChangeNoMedia = TRUE;

            }
        }
    } else if((srb->SrbStatus == SRB_STATUS_SUCCESS)&&
              (deviceExtension->MediaChangeNoMedia)) {
        //
        // We didn't have any media before and now the requests are succeeding
        // we probably missed the Media change somehow.  Signal the change
        // anyway
        //

        DebugPrint((1, "CdRomMediaChangeCompletion: Request completed normally"
                       "for CdRom%d which was marked w/NoMedia [irp = 0x%lx]\n",
                    deviceExtension->DeviceNumber, Irp));

        KeSetEvent(deviceExtension->MediaChangeEvent,
                   (KPRIORITY) 0,
                   FALSE);

        deviceExtension->MediaChangeNoMedia = FALSE;

    }

    if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
        ScsiClassReleaseQueue(deviceExtension->DeviceObject);
    }

    //
    // Remember the IRP and SRB for use the next time.
    //

    irpNextStack->Parameters.Scsi.Srb = srb;
    cddata->MediaChangeIrp = Irp;

    if (deviceExtension->ClassError) {

        NTSTATUS status;
        BOOLEAN  retry;

        //
        // Throw away the status and retry values. Just give the error routine a chance
        // to do what it needs to.
        //

        deviceExtension->ClassError(DeviceObject,
                                    srb,
                                    &status,
                                    &retry);
    }

    IoStartNextPacket(DeviceObject, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
NTAPI
CdRomTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles the once per second timer provided by the
    Io subsystem.  It is only used when the cdrom device itself is
    a candidate for autoplay support.  It should never be called if
    the cdrom device is a changer device.

Arguments:

    DeviceObject - what to check.
    Context - not used.

Return Value:

    None.

--*/

{
    PIRP              irp;
    PIRP              heldIrpList;
    PIRP              nextIrp;
    PLIST_ENTRY       listEntry;
    PCDROM_DATA       cddata;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    cddata = (PCDROM_DATA)(deviceExtension + 1);

    if (cddata->MediaChange) {
        if (cddata->MediaChangeIrp != NULL) {

            //
            // Media change support is active and the IRP is waiting.
            // Decrement the timer.
            // There is no MP protection on the timer counter.  This
            // code is the only code that will manipulate the timer
            // and only one instance of it should be running at any
            // given time.
            //

            cddata->MediaChangeCountDown--;

#if DBG
            cddata->MediaChangeIrpTimeInUse = 0;
            cddata->MediaChangeIrpLost = FALSE;
#endif

            if (!cddata->MediaChangeCountDown) {
                PSCSI_REQUEST_BLOCK srb;
                PIO_STACK_LOCATION  irpNextStack;
                PCDB cdb;

                //
                // Reset the timer.
                //

                cddata->MediaChangeCountDown = MEDIA_CHANGE_DEFAULT_TIME;

                //
                // Prepare the IRP for the test unit ready
                //

                irp = cddata->MediaChangeIrp;
                cddata->MediaChangeIrp = NULL;

                irp->IoStatus.Status = STATUS_SUCCESS;
                irp->IoStatus.Information = 0;
                irp->Flags = 0;
                irp->UserBuffer = NULL;

                //
                // If the irp is sent down when the volume needs to be
                // verified, CdRomUpdateGeometryCompletion won't complete
                // it since it's not associated with a thread.  Marking
                // it to override the verify causes it always be sent
                // to the port driver
                //

                irpStack = IoGetCurrentIrpStackLocation(irp);
                irpStack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

                irpNextStack = IoGetNextIrpStackLocation(irp);
                irpNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                irpNextStack->Parameters.DeviceIoControl.IoControlCode =
                    IOCTL_SCSI_EXECUTE_NONE;

                //
                // Prepare the SRB for execution.
                //

                srb = irpNextStack->Parameters.Scsi.Srb;
                srb->SrbStatus = srb->ScsiStatus = 0;
                srb->NextSrb = 0;
                srb->Length = SCSI_REQUEST_BLOCK_SIZE;
                srb->PathId = deviceExtension->PathId;
                srb->TargetId = deviceExtension->TargetId;
                srb->Lun = deviceExtension->Lun;
                srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
                srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER |
                                SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
                srb->DataTransferLength = 0;
                srb->OriginalRequest = irp;

                RtlZeroMemory(srb->SenseInfoBuffer, SENSE_BUFFER_SIZE);
                srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

                cdb = (PCDB) &srb->Cdb[0];
                cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
                cdb->CDB6GENERIC.LogicalUnitNumber = srb->Lun;

                //
                // Setup the IRP to perform a test unit ready.
                //

                IoSetCompletionRoutine(irp,
                                       CdRomMediaChangeCompletion,
                                       srb,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                //
                // Issue the request.
                //

                IoStartPacket(DeviceObject, irp, NULL, NULL);
            }
        } else {

#if DBG
            if(cddata->MediaChangeIrpLost == FALSE) {
                if(cddata->MediaChangeIrpTimeInUse++ >
                   MEDIA_CHANGE_TIMEOUT_TIME) {

                    DebugPrint((0, "CdRom%d: AutoPlay has lost it's irp and "
                                   "doesn't know where to find it.  Leave it "
                                   "alone and it'll come home dragging it's "
                                   "stack behind it.\n",
                                   deviceExtension->DeviceNumber));
                    cddata->MediaChangeIrpLost = TRUE;
                }
            }

#endif
        }
    }

    //
    // Process all generic timer IRPS in the timer list.  As IRPs are pulled
    // off of the TimerIrpList they must be remembered in the first loop
    // if they are not sent to the lower driver.  After all items have
    // been pulled off the list, it is possible to put the held IRPs back
    // into the TimerIrpList.
    //

    heldIrpList = NULL;
    if (IsListEmpty(&cddata->TimerIrpList)) {
        listEntry = NULL;
    } else {
        listEntry = ExInterlockedRemoveHeadList(&cddata->TimerIrpList,
                                                &cddata->TimerIrpSpinLock);
    }
    while (listEntry) {

        //
        // There is something in the timer list.  Pick up the IRP and
        // see if it is ready to be submitted.
        //

        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        irpStack = IoGetCurrentIrpStackLocation(irp);

        if (irpStack->Parameters.Others.Argument3) {
            ULONG_PTR count;

            //
            // Decrement the countdown timer and put the IRP back in the list.
            //

            count = (ULONG_PTR) irpStack->Parameters.Others.Argument3;
            count--;
            irpStack->Parameters.Others.Argument3 = (PVOID) count;

            ASSERT(irp->AssociatedIrp.MasterIrp == NULL);
            if (heldIrpList) {
                irp->AssociatedIrp.MasterIrp = (PVOID) heldIrpList;
            }
            heldIrpList = irp;

        } else {

            //
            // Submit this IRP to the lower driver.  This IRP does not
            // need to be remembered here.  It will be handled again when
            // it completes.
            //

            DebugPrint((1, "CdRomTickHandler: Reissuing request %lx (thread = %lx)\n", irp, irp->Tail.Overlay.Thread));

            //
            // feed this to the appropriate port driver
            //

            IoCallDriver (deviceExtension->PortDeviceObject, irp);

        }

        //
        // Pick up the next IRP from the timer list.
        //

        listEntry = ExInterlockedRemoveHeadList(&cddata->TimerIrpList,
                                                &cddata->TimerIrpSpinLock);
    }

    //
    // Move all held IRPs back onto the timer list.
    //

    while (heldIrpList) {

        //
        // Save the single list pointer before queueing this IRP.
        //

        nextIrp = (PIRP) heldIrpList->AssociatedIrp.MasterIrp;
        heldIrpList->AssociatedIrp.MasterIrp = NULL;

        //
        // Return the held IRP to the timer list.
        //

        ExInterlockedInsertTailList(&cddata->TimerIrpList,
                                    &heldIrpList->Tail.Overlay.ListEntry,
                                    &cddata->TimerIrpSpinLock);

        //
        // Continue processing the held IRPs
        //

        heldIrpList = nextIrp;
    }
}

BOOLEAN
NTAPI
CdRomCheckRegistryForMediaChangeValue(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber
    )

/*++

Routine Description:

    The user must specify that AutoPlay is to run on the platform
    by setting the registry value HKEY_LOCAL_MACHINE\System\CurrentControlSet\
    Services\Cdrom\Autorun:REG_DWORD:1.

    The user can override the global setting to enable or disable Autorun on a
    specific cdrom device by setting the key HKEY_LOCAL_MACHINE\System\
    CurrentControlSet\Services\Cdrom\Device<N>\Autorun:REG_DWORD to one or zero.
    (CURRENTLY UNIMPLEMENTED)

    If this registry value does not exist or contains the value zero then
    the timer to check for media change does not run.

Arguments:

    RegistryPath - pointer to the unicode string inside
                   ...\CurrentControlSet\Services\Cdrom
    DeviceNumber - The number of the device object

Return Value:

    TRUE - Autorun is enabled.
    FALSE - no autorun.

--*/

{
#define ITEMS_TO_QUERY 2 /* always 1 greater than what is searched */
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    NTSTATUS          status;
    LONG              zero = 0;

    LONG              tmp = 0;
    LONG              doRun = 0;

    CHAR              buf[32];
    ANSI_STRING       paramNum;

    UNICODE_STRING    paramStr;

    UNICODE_STRING    paramSuffix;
    UNICODE_STRING    paramPath;
    UNICODE_STRING    paramDevPath;

    //
    // First append \Parameters to the passed in registry path
    //

    RtlInitUnicodeString(&paramStr, L"\\Parameters");

    RtlInitUnicodeString(&paramPath, NULL);

    paramPath.MaximumLength = RegistryPath->Length +
                              paramStr.Length +
                              sizeof(WCHAR);

    paramPath.Buffer = ExAllocatePool(PagedPool, paramPath.MaximumLength);

    if(!paramPath.Buffer) {

        DebugPrint((1,"CdRomCheckRegAP: couldn't allocate paramPath\n"));

        return FALSE;
    }

    RtlZeroMemory(paramPath.Buffer, paramPath.MaximumLength);
    RtlAppendUnicodeToString(&paramPath, RegistryPath->Buffer);
    RtlAppendUnicodeToString(&paramPath, paramStr.Buffer);

    DebugPrint((2, "CdRomCheckRegAP: paramPath [%d] = %ws\n",
                paramPath.Length,
                paramPath.Buffer));

    //
    // build a counted ANSI string that contains
    // the suffix for the path
    //

    sprintf(buf, "\\Device%lu", DeviceNumber);
    RtlInitAnsiString(&paramNum, buf);

    //
    // Next convert this into a unicode string
    //

    status = RtlAnsiStringToUnicodeString(&paramSuffix, &paramNum, TRUE);

    if(!NT_SUCCESS(status)) {
        DebugPrint((1,"CdRomCheckRegAP: couldn't convert paramNum to paramSuffix\n"));
        ExFreePool(paramPath.Buffer);
        return FALSE;
    }

    RtlInitUnicodeString(&paramDevPath, NULL);

    //
    // now build the device specific path
    //

    paramDevPath.MaximumLength = paramPath.Length +
                                 paramSuffix.Length +
                                 sizeof(WCHAR);
    paramDevPath.Buffer = ExAllocatePool(PagedPool, paramDevPath.MaximumLength);

    if(!paramDevPath.Buffer) {
        RtlFreeUnicodeString(&paramSuffix);
        ExFreePool(paramPath.Buffer);
        return FALSE;
    }

    RtlZeroMemory(paramDevPath.Buffer, paramDevPath.MaximumLength);
    RtlAppendUnicodeToString(&paramDevPath, paramPath.Buffer);
    RtlAppendUnicodeToString(&paramDevPath, paramSuffix.Buffer);

    DebugPrint((2, "CdRomCheckRegAP: paramDevPath [%d] = %ws\n",
                paramPath.Length,
                paramPath.Buffer));

    parameters = ExAllocatePool(NonPagedPool,
                                sizeof(RTL_QUERY_REGISTRY_TABLE)*ITEMS_TO_QUERY);

    if (parameters) {

        //
        // Check for the Autorun value.
        //

        RtlZeroMemory(parameters,
                      (sizeof(RTL_QUERY_REGISTRY_TABLE)*ITEMS_TO_QUERY));

        parameters[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name          = L"Autorun";
        parameters[0].EntryContext  = &doRun;
        parameters[0].DefaultType   = REG_DWORD;
        parameters[0].DefaultData   = &zero;
        parameters[0].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                        RegistryPath->Buffer,
                                        parameters,
                                        NULL,
                                        NULL);

        DebugPrint((2, "CdRomCheckRegAP: cdrom/Autorun flag = %d\n", doRun));

        RtlZeroMemory(parameters,
                      (sizeof(RTL_QUERY_REGISTRY_TABLE)*ITEMS_TO_QUERY));

        parameters[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name          = L"Autorun";
        parameters[0].EntryContext  = &tmp;
        parameters[0].DefaultType   = REG_DWORD;
        parameters[0].DefaultData   = &doRun;
        parameters[0].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                        paramPath.Buffer,
                                        parameters,
                                        NULL,
                                        NULL);

        DebugPrint((2, "CdRomCheckRegAP: cdrom/parameters/autorun flag = %d\n", tmp));

        RtlZeroMemory(parameters,
                      (sizeof(RTL_QUERY_REGISTRY_TABLE) * ITEMS_TO_QUERY));

        parameters[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name          = L"Autorun";
        parameters[0].EntryContext  = &doRun;
        parameters[0].DefaultType   = REG_DWORD;
        parameters[0].DefaultData   = &tmp;
        parameters[0].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                        paramDevPath.Buffer,
                                        parameters,
                                        NULL,
                                        NULL);

        DebugPrint((1, "CdRomCheckRegAP: cdrom/parameters/device%d/autorun flag = %d\n", DeviceNumber, doRun));

        ExFreePool(parameters);

    }

    ExFreePool(paramPath.Buffer);
    ExFreePool(paramDevPath.Buffer);
    RtlFreeUnicodeString(&paramSuffix);

    DebugPrint((1, "CdRomCheckRegAP: Autoplay for device %d is %s\n",
                DeviceNumber,
                (doRun ? "on" : "off")));

    if(doRun) {
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
NTAPI
IsThisASanyo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  UCHAR          PathId,
    IN  UCHAR          TargetId
    )

/*++

Routine Description:

    This routine is called by DriverEntry to determine whether a Sanyo 3-CD
    changer device is present.

Arguments:

    DeviceObject - Supplies the device object for the 'real' device.

    PathId       -

Return Value:

    TRUE - if an Atapi changer device is found.

--*/

{
    KEVENT                 event;
    PIRP                   irp;
    PCHAR                  inquiryBuffer;
    IO_STATUS_BLOCK        ioStatus;
    NTSTATUS               status;
    PSCSI_ADAPTER_BUS_INFO adapterInfo;
    ULONG                  scsiBus;
    PINQUIRYDATA           inquiryData;
    PSCSI_INQUIRY_DATA     lunInfo;

    inquiryBuffer = ExAllocatePool(NonPagedPool, 2048);
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_INQUIRY_DATA,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        inquiryBuffer,
                                        2048,
                                        FALSE,
                                        &event,
                                        &ioStatus);
    if (!irp) {
        return FALSE;
    }

    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    adapterInfo = (PVOID) inquiryBuffer;

    for (scsiBus=0; scsiBus < (ULONG)adapterInfo->NumberOfBuses; scsiBus++) {

        //
        // Get the SCSI bus scan data for this bus.
        //

        lunInfo = (PVOID) (inquiryBuffer + adapterInfo->BusData[scsiBus].InquiryDataOffset);

        for (;;) {

            if (lunInfo->PathId == PathId && lunInfo->TargetId == TargetId) {

                inquiryData = (PVOID) lunInfo->InquiryData;

                if (RtlCompareMemory(inquiryData->VendorId, "TORiSAN CD-ROM CDR-C", 20) == 20) {
                    ExFreePool(inquiryBuffer);
                    return TRUE;
                }

                ExFreePool(inquiryBuffer);
                return FALSE;
            }

            if (!lunInfo->NextInquiryDataOffset) {
                break;
            }

            lunInfo = (PVOID) (inquiryBuffer + lunInfo->NextInquiryDataOffset);
        }
    }

    ExFreePool(inquiryBuffer);
    return FALSE;
}

BOOLEAN
NTAPI
IsThisAnAtapiChanger(
    IN  PDEVICE_OBJECT DeviceObject,
    OUT PULONG         DiscsPresent
    )

/*++

Routine Description:

    This routine is called by DriverEntry to determine whether an Atapi
    changer device is present.

Arguments:

    DeviceObject    - Supplies the device object for the 'real' device.

    DiscsPresent    - Supplies a pointer to the number of Discs supported by the changer.

Return Value:

    TRUE - if an Atapi changer device is found.

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PMECHANICAL_STATUS_INFORMATION_HEADER mechanicalStatusBuffer;
    NTSTATUS            status;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB) &srb.Cdb[0];
    BOOLEAN             retVal = FALSE;

    *DiscsPresent = 0;

    //
    // Some devices can't handle 12 byte CDB's gracefully
    //

    if(deviceExtension->DeviceFlags & DEV_NO_12BYTE_CDB) {

        return FALSE;

    }

    //
    // Build and issue the mechanical status command.
    //

    mechanicalStatusBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                            sizeof(MECHANICAL_STATUS_INFORMATION_HEADER));

    if (!mechanicalStatusBuffer) {
        retVal = FALSE;
    } else {

        //
        // Build and send the Mechanism status CDB.
        //

        RtlZeroMemory(&srb, sizeof(srb));

        srb.CdbLength = 12;
        srb.TimeOutValue = 20;

        cdb->MECH_STATUS.OperationCode = SCSIOP_MECHANISM_STATUS;
        cdb->MECH_STATUS.AllocationLength[1] = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             mechanicalStatusBuffer,
                                             sizeof(MECHANICAL_STATUS_INFORMATION_HEADER),
                                             FALSE);


        if (status == STATUS_SUCCESS) {

            //
            // Indicate number of slots available
            //

            *DiscsPresent = mechanicalStatusBuffer->NumberAvailableSlots;
            if (*DiscsPresent > 1) {
                retVal = TRUE;
            } else {

                //
                // If only one disc, no need for this driver.
                //

                retVal = FALSE;
            }
        } else {

            //
            // Device doesn't support this command.
            //

            retVal = FALSE;
        }

        ExFreePool(mechanicalStatusBuffer);
    }

    return retVal;
}

BOOLEAN
NTAPI
IsThisAMultiLunDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT PortDeviceObject
    )
/*++

Routine Description:

    This routine is called to determine whether a multi-lun
    device is present.

Arguments:

    DeviceObject    - Supplies the device object for the 'real' device.

Return Value:

    TRUE - if a Multi-lun device is found.

--*/
{
    PCHAR buffer;
    PSCSI_INQUIRY_DATA lunInfo;
    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PINQUIRYDATA inquiryData;
    ULONG scsiBus;
    NTSTATUS status;
    UCHAR lunCount = 0;

    status = ScsiClassGetInquiryData(PortDeviceObject, (PSCSI_ADAPTER_BUS_INFO *) &buffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"IsThisAMultiLunDevice: ScsiClassGetInquiryData failed\n"));
        return FALSE;
    }

    adapterInfo = (PVOID) buffer;

    //
    // For each SCSI bus this adapter supports ...
    //

    for (scsiBus=0; scsiBus < adapterInfo->NumberOfBuses; scsiBus++) {

        //
        // Get the SCSI bus scan data for this bus.
        //

        lunInfo = (PVOID) (buffer + adapterInfo->BusData[scsiBus].InquiryDataOffset);

        while (adapterInfo->BusData[scsiBus].InquiryDataOffset) {

            inquiryData = (PVOID)lunInfo->InquiryData;

            if ((lunInfo->PathId == deviceExtension->PathId) &&
                (lunInfo->TargetId == deviceExtension->TargetId) &&
                (inquiryData->DeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE)) {

                DebugPrint((1,"IsThisAMultiLunDevice: Vendor string is %.24s\n",
                            inquiryData->VendorId));

                //
                // If this device has more than one cdrom-type lun then we
                // won't support autoplay on it
                //

                if (lunCount++) {
                    ExFreePool(buffer);
                    return TRUE;
                }
            }

            //
            // Get next LunInfo.
            //

            if (lunInfo->NextInquiryDataOffset == 0) {
                break;
            }

            lunInfo = (PVOID) (buffer + lunInfo->NextInquiryDataOffset);
        }
    }

    ExFreePool(buffer);
    return FALSE;

}

IO_COMPLETION_ROUTINE CdRomUpdateGeometryCompletion;
NTSTATUS
NTAPI
CdRomUpdateGeometryCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:

    This routine andles the completion of the test unit ready irps
    used to determine if the media has changed.  If the media has
    changed, this code signals the named event to wake up other
    system services that react to media change (aka AutoPlay).

Arguments:

    DeviceObject - the object for the completion
    Irp - the IRP being completed
    Context - the SRB from the IRP

Return Value:

    NTSTATUS

--*/

{
    PSCSI_REQUEST_BLOCK srb = (PSCSI_REQUEST_BLOCK) Context;
    PREAD_CAPACITY_DATA readCapacityBuffer;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    BOOLEAN             retry;
    ULONG_PTR           retryCount;
    ULONG               lastSector;
    PIRP                originalIrp;
    PCDROM_DATA         cddata;

    //
    // Get items saved in the private IRP stack location.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    retryCount = (ULONG_PTR) irpStack->Parameters.Others.Argument1;
    originalIrp = (PIRP) irpStack->Parameters.Others.Argument2;

    if (!DeviceObject) {
        DeviceObject = irpStack->DeviceObject;
    }
    ASSERT(DeviceObject);

    deviceExtension = DeviceObject->DeviceExtension;
    cddata = (PCDROM_DATA) (deviceExtension + 1);
    readCapacityBuffer = srb->DataBuffer;

    if ((NT_SUCCESS(Irp->IoStatus.Status)) && (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS)) {
        PFOUR_BYTE from;
        PFOUR_BYTE to;

        DebugPrint((2, "CdRomUpdateCapacityCompletion: [%lx] successful completion of buddy-irp %lx\n", originalIrp, Irp));
        //
        // Copy sector size from read capacity buffer to device extension
        // in reverse byte order.
        //

        from = (PFOUR_BYTE) &readCapacityBuffer->BytesPerBlock;
        to = (PFOUR_BYTE) &deviceExtension->DiskGeometry->Geometry.BytesPerSector;
        to->Byte0 = from->Byte3;
        to->Byte1 = from->Byte2;
        to->Byte2 = from->Byte1;
        to->Byte3 = from->Byte0;

        //
        // Using the new BytesPerBlock, calculate and store the SectorShift.
        //

        WHICH_BIT(deviceExtension->DiskGeometry->Geometry.BytesPerSector, deviceExtension->SectorShift);

        //
        // Copy last sector in reverse byte order.
        //

        from = (PFOUR_BYTE) &readCapacityBuffer->LogicalBlockAddress;
        to = (PFOUR_BYTE) &lastSector;
        to->Byte0 = from->Byte3;
        to->Byte1 = from->Byte2;
        to->Byte2 = from->Byte1;
        to->Byte3 = from->Byte0;
        deviceExtension->PartitionLength.QuadPart = (LONGLONG)(lastSector + 1);

        //
        // Calculate number of cylinders.
        //

        deviceExtension->DiskGeometry->Geometry.Cylinders.QuadPart = (LONGLONG)((lastSector + 1)/(32 * 64));
        deviceExtension->PartitionLength.QuadPart =
            (deviceExtension->PartitionLength.QuadPart << deviceExtension->SectorShift);
        deviceExtension->DiskGeometry->Geometry.MediaType = RemovableMedia;

        //
        // Assume sectors per track are 32;
        //

        deviceExtension->DiskGeometry->Geometry.SectorsPerTrack = 32;

        //
        // Assume tracks per cylinder (number of heads) is 64.
        //

        deviceExtension->DiskGeometry->Geometry.TracksPerCylinder = 64;

    } else {

        DebugPrint((1, "CdRomUpdateCapacityCompletion: [%lx] unsuccessful completion of buddy-irp %lx (status - %lx)\n", originalIrp, Irp, Irp->IoStatus.Status));

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ScsiClassReleaseQueue(DeviceObject);
        }

        retry = ScsiClassInterpretSenseInfo(DeviceObject,
                                            srb,
                                            IRP_MJ_SCSI,
                                            0,
                                            retryCount,
                                            &status);
        if (retry) {
            retryCount--;
            if (retryCount) {
                PCDB cdb;

                DebugPrint((1, "CdRomUpdateCapacityCompletion: [%lx] Retrying request %lx .. thread is %lx\n", originalIrp, Irp, Irp->Tail.Overlay.Thread));
                //
                // set up a one shot timer to get this process started over
                //

                irpStack->Parameters.Others.Argument1 = (PVOID) retryCount;
                irpStack->Parameters.Others.Argument2 = (PVOID) originalIrp;
                irpStack->Parameters.Others.Argument3 = (PVOID) 2;

                //
                // Setup the IRP to be submitted again in the timer routine.
                //

                irpStack = IoGetNextIrpStackLocation(Irp);
                irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
                irpStack->Parameters.Scsi.Srb = srb;
                IoSetCompletionRoutine(Irp,
                                       CdRomUpdateGeometryCompletion,
                                       srb,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                //
                // Set up the SRB for read capacity.
                //

                srb->CdbLength = 10;
                srb->TimeOutValue = deviceExtension->TimeOutValue;
                srb->SrbStatus = srb->ScsiStatus = 0;
                srb->NextSrb = 0;
                srb->Length = SCSI_REQUEST_BLOCK_SIZE;
                srb->PathId = deviceExtension->PathId;
                srb->TargetId = deviceExtension->TargetId;
                srb->Lun = deviceExtension->Lun;
                srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
                srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
                srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);

                //
                // Set up the CDB
                //

                cdb = (PCDB) &srb->Cdb[0];
                cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;
                cdb->CDB10.LogicalUnitNumber = deviceExtension->Lun;

                //
                // Requests queued onto this list will be sent to the
                // lower level driver during CdRomTickHandler
                //

                ExInterlockedInsertHeadList(&cddata->TimerIrpList,
                                            &Irp->Tail.Overlay.ListEntry,
                                            &cddata->TimerIrpSpinLock);

                return STATUS_MORE_PROCESSING_REQUIRED;
            } else {

                //
                // This has been bounced for a number of times.  Error the
                // original request.
                //

                originalIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                RtlZeroMemory(deviceExtension->DiskGeometry, sizeof(DISK_GEOMETRY_EX));
                deviceExtension->DiskGeometry->Geometry.BytesPerSector = 2048;
                deviceExtension->SectorShift = 11;
                deviceExtension->PartitionLength.QuadPart = (LONGLONG)(0x7fffffff);
                deviceExtension->DiskGeometry->Geometry.MediaType = RemovableMedia;
            }
        } else {

            //
            // Set up reasonable defaults
            //

            RtlZeroMemory(deviceExtension->DiskGeometry, sizeof(DISK_GEOMETRY_EX));
            deviceExtension->DiskGeometry->Geometry.BytesPerSector = 2048;
            deviceExtension->SectorShift = 11;
            deviceExtension->PartitionLength.QuadPart = (LONGLONG)(0x7fffffff);
            deviceExtension->DiskGeometry->Geometry.MediaType = RemovableMedia;
        }
    }

    //
    // Free resources held.
    //

    ExFreePool(srb->SenseInfoBuffer);
    ExFreePool(srb->DataBuffer);
    ExFreePool(srb);
    if (Irp->MdlAddress) {
        IoFreeMdl(Irp->MdlAddress);
    }
    IoFreeIrp(Irp);
    if (originalIrp->Tail.Overlay.Thread) {

        DebugPrint((2, "CdRomUpdateCapacityCompletion: [%lx] completing original IRP\n", originalIrp));
        IoCompleteRequest(originalIrp, IO_DISK_INCREMENT);

    } else {
        DebugPrint((1, "CdRomUpdateCapacityCompletion: [%lx] original irp has "
                       "no thread\n",
                    originalIrp
                  ));
    }

    //
    // It's now safe to either start the next request or let the waiting ioctl
    // request continue along it's merry way
    //

    IoStartNextPacket(DeviceObject, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
CdRomUpdateCapacity(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP IrpToComplete,
    IN OPTIONAL PKEVENT IoctlEvent
    )

/*++

Routine Description:

    This routine updates the capacity of the disk as recorded in the device extension.
    It also completes the IRP given with STATUS_VERIFY_REQUIRED.  This routine is called
    when a media change has occurred and it is necessary to determine the capacity of the
    new media prior to the next access.

Arguments:

    DeviceExtension - the device to update
    IrpToComplete - the request that needs to be completed when done.

Return Value:

    NTSTATUS

--*/

{
    PCDB                cdb;
    PIRP                irp;
    PSCSI_REQUEST_BLOCK srb;
    PREAD_CAPACITY_DATA capacityBuffer;
    PIO_STACK_LOCATION  irpStack;
    PUCHAR              senseBuffer;

    irp = IoAllocateIrp((CCHAR)(DeviceExtension->DeviceObject->StackSize+1),
                        FALSE);

    if (irp) {

        srb = ExAllocatePool(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK));
        if (srb) {
            capacityBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                            sizeof(READ_CAPACITY_DATA));

            if (capacityBuffer) {


                senseBuffer = ExAllocatePool(NonPagedPoolCacheAligned, SENSE_BUFFER_SIZE);

                if (senseBuffer) {

                    irp->MdlAddress = IoAllocateMdl(capacityBuffer,
                                                    sizeof(READ_CAPACITY_DATA),
                                                    FALSE,
                                                    FALSE,
                                                    (PIRP) NULL);

                    if (irp->MdlAddress) {

                        //
                        // Have all resources.  Set up the IRP to send for the capacity.
                        //

                        IoSetNextIrpStackLocation(irp);
                        irp->IoStatus.Status = STATUS_SUCCESS;
                        irp->IoStatus.Information = 0;
                        irp->Flags = 0;
                        irp->UserBuffer = NULL;

                        //
                        // Save the device object and retry count in a private stack location.
                        //

                        irpStack = IoGetCurrentIrpStackLocation(irp);
                        irpStack->DeviceObject = DeviceExtension->DeviceObject;
                        irpStack->Parameters.Others.Argument1 = (PVOID) MAXIMUM_RETRIES;
                        irpStack->Parameters.Others.Argument2 = (PVOID) IrpToComplete;

                        //
                        // Construct the IRP stack for the lower level driver.
                        //

                        irpStack = IoGetNextIrpStackLocation(irp);
                        irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
                        irpStack->Parameters.Scsi.Srb = srb;
                        IoSetCompletionRoutine(irp,
                                               CdRomUpdateGeometryCompletion,
                                               srb,
                                               TRUE,
                                               TRUE,
                                               TRUE);
                        //
                        // Prepare the MDL
                        //

                        MmBuildMdlForNonPagedPool(irp->MdlAddress);


                        //
                        // Set up the SRB for read capacity.
                        //

                        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
                        RtlZeroMemory(senseBuffer, SENSE_BUFFER_SIZE);
                        srb->CdbLength = 10;
                        srb->TimeOutValue = DeviceExtension->TimeOutValue;
                        srb->SrbStatus = srb->ScsiStatus = 0;
                        srb->NextSrb = 0;
                        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
                        srb->PathId = DeviceExtension->PathId;
                        srb->TargetId = DeviceExtension->TargetId;
                        srb->Lun = DeviceExtension->Lun;
                        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
                        srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
                        srb->DataBuffer = capacityBuffer;
                        srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);
                        srb->OriginalRequest = irp;
                        srb->SenseInfoBuffer = senseBuffer;
                        srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

                        //
                        // Set up the CDB
                        //

                        cdb = (PCDB) &srb->Cdb[0];
                        cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;
                        cdb->CDB10.LogicalUnitNumber = DeviceExtension->Lun;

                        //
                        // Set the return value in the IRP that will be completed
                        // upon completion of the read capacity.
                        //

                        IrpToComplete->IoStatus.Status = STATUS_VERIFY_REQUIRED;
                        IoMarkIrpPending(IrpToComplete);

                        IoCallDriver(DeviceExtension->PortDeviceObject, irp);

                        //
                        // status is not checked because the completion routine for this
                        // IRP will always get called and it will free the resources.
                        //

                        return STATUS_PENDING;

                    } else {
                        ExFreePool(senseBuffer);
                        ExFreePool(capacityBuffer);
                        ExFreePool(srb);
                        IoFreeIrp(irp);
                    }
                } else {
                    ExFreePool(capacityBuffer);
                    ExFreePool(srb);
                    IoFreeIrp(irp);
                }
            } else {
                ExFreePool(srb);
                IoFreeIrp(irp);
            }
        } else {
            IoFreeIrp(irp);
        }
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
NTAPI
CdRomClassIoctlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine signals the event used by CdRomDeviceControl to synchronize
    class driver (and lower level driver) ioctls with cdrom's startio routine.
    The irp completion is short-circuited so that CdRomDeviceControl can
    reissue it once it wakes up.

Arguments:

    DeviceObject - the device object
    Irp - the request we are synchronizing
    Context - a PKEVENT that we need to signal

Return Value:

    NTSTATUS

--*/

{
    PKEVENT syncEvent = (PKEVENT) Context;

    DebugPrint((2, "CdRomClassIoctlCompletion: setting event for irp %#08lx\n",
                Irp
                ));

    KeSetEvent(syncEvent, IO_DISK_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}
