/*
 * PROJECT:         ReactOS Storage Stack
 * LICENSE:         DDK - see license.txt in the root dir
 * FILE:            drivers/storage/disk/disk.c
 * PURPOSE:         Disk class driver
 * PROGRAMMERS:     Based on a source code sample from Microsoft NT4 DDK
 */

#include <ntddk.h>
#include <ntdddisk.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <mountdev.h>
#include <mountmgr.h>
#include <include/class2.h>
#include <stdio.h>

//#define NDEBUG
#include <debug.h>

#define IO_WRITE_CACHE_ENABLED  ((NTSTATUS)0x80040020L)
#define IO_WRITE_CACHE_DISABLED ((NTSTATUS)0x80040022L)

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'DscS')
#endif

typedef enum {
    NotInitialized,
    Initializing,
    Initialized
} PARTITION_LIST_STATE;

//
// Disk device data
//

typedef struct _DISK_DATA {

    //
    // Partition chain
    //

    PDEVICE_EXTENSION NextPartition;

    //
    // Disk signature (from MBR)
    //

    ULONG Signature;

    //
    // MBR checksum
    //

    ULONG MbrCheckSum;

    //
    // Number of hidden sectors for BPB.
    //

    ULONG HiddenSectors;

    //
    // Partition number of this device object
    //
    // This field is set during driver initialization or when the partition
    // is created to identify a parition to the system.
    //

    ULONG PartitionNumber;

    //
    // This field is the ordinal of a partition as it appears on a disk.
    //

    ULONG PartitionOrdinal;

    //
    // Partition type of this device object
    //
    // This field is set by:
    //
    //     1)  Initially set according to the partition list entry partition
    //         type returned by IoReadPartitionTable.
    //
    //     2)  Subsequently set by the IOCTL_DISK_SET_PARTITION_INFORMATION
    //         I/O control function when IoSetPartitionInformation function
    //         successfully updates the partition type on the disk.
    //

    UCHAR PartitionType;

    //
    // Boot indicator - indicates whether this partition is a bootable (active)
    // partition for this device
    //
    // This field is set according to the partition list entry boot indicator
    // returned by IoReadPartitionTable.
    //

    BOOLEAN BootIndicator;

    //
    // DriveNotReady - inidicates that the this device is currenly not ready
    // because there is no media in the device.
    //

    BOOLEAN DriveNotReady;

    //
    // State of PartitionList initialization
    //

    PARTITION_LIST_STATE PartitionListState;

} DISK_DATA, *PDISK_DATA;

//
// Define a general structure of identfing disk controllers with bad
// hardware.
//

typedef struct _BAD_CONTROLLER_INFORMATION {
    PCHAR InquiryString;
    BOOLEAN DisableTaggedQueuing;
    BOOLEAN DisableSynchronousTransfers;
    BOOLEAN DisableDisconnects;
    BOOLEAN DisableWriteCache;
}BAD_CONTROLLER_INFORMATION, *PBAD_CONTROLLER_INFORMATION;

BAD_CONTROLLER_INFORMATION const ScsiDiskBadControllers[] = {
    { "TOSHIBA MK538FB         60",   TRUE,  FALSE, FALSE, FALSE },
    { "CONNER  CP3500",               FALSE, TRUE,  FALSE, FALSE },
    { "OLIVETTICP3500",               FALSE, TRUE,  FALSE, FALSE },
    { "SyQuest SQ5110          CHC",  TRUE,  TRUE,  FALSE, FALSE },
    { "SEAGATE ST41601N        0102", FALSE, TRUE,  FALSE, FALSE },
    { "SEAGATE ST3655N",              FALSE, FALSE, FALSE, TRUE  },
    { "SEAGATE ST3390N",              FALSE, FALSE, FALSE, TRUE  },
    { "SEAGATE ST12550N",             FALSE, FALSE, FALSE, TRUE  },
    { "SEAGATE ST32430N",             FALSE, FALSE, FALSE, TRUE  },
    { "SEAGATE ST31230N",             FALSE, FALSE, FALSE, TRUE  },
    { "SEAGATE ST15230N",             FALSE, FALSE, FALSE, TRUE  },
    { "FUJITSU M2652S-512",           TRUE,  FALSE, FALSE, FALSE },
    { "MAXTOR  MXT-540SL       I1.2", TRUE,  FALSE, FALSE, FALSE },
    { "COMPAQ  PD-1",                 FALSE, TRUE,  FALSE, FALSE }
};


#define NUMBER_OF_BAD_CONTROLLERS (sizeof(ScsiDiskBadControllers) / sizeof(BAD_CONTROLLER_INFORMATION))
#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION) + sizeof(DISK_DATA)

#define MODE_DATA_SIZE      192
#define VALUE_BUFFER_SIZE  2048
#define SCSI_DISK_TIMEOUT    10
#define PARTITION0_LIST_SIZE  4


NTSTATUS
STDCALL
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

BOOLEAN
STDCALL
ScsiDiskDeviceVerification(
    IN PINQUIRYDATA InquiryData
    );

BOOLEAN
STDCALL
FindScsiDisks(
    IN PDRIVER_OBJECT DriveObject,
    IN PUNICODE_STRING RegistryPath,
    IN PCLASS_INIT_DATA InitializationData,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber
    );

NTSTATUS
STDCALL
ScsiDiskCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
STDCALL
ScsiDiskReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
STDCALL
ScsiDiskDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
STDCALL
ScsiDiskProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

NTSTATUS
STDCALL
ScsiDiskShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
STDCALL
DisableWriteCache(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_INQUIRY_DATA LunInfo
    );

BOOLEAN
STDCALL
ScsiDiskModeSelect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCHAR ModeSelectBuffer,
    IN ULONG Length,
    IN BOOLEAN SavePage
    );

BOOLEAN
STDCALL
IsFloppyDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
STDCALL
CalculateMbrCheckSum(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PULONG Checksum
    );

BOOLEAN
STDCALL
EnumerateBusKey(
    IN PDEVICE_EXTENSION DeviceExtension,
    HANDLE BusKey,
    PULONG DiskNumber
    );

VOID
STDCALL
UpdateGeometry(
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
STDCALL
UpdateRemovableGeometry (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
STDCALL
CreateDiskDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber,
    IN PULONG DeviceCount,
    IN PIO_SCSI_CAPABILITIES PortCapabilities,
    IN PSCSI_INQUIRY_DATA LunInfo,
    IN PCLASS_INIT_DATA InitData
    );

NTSTATUS
STDCALL
CreatePartitionDeviceObjects(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
STDCALL
UpdateDeviceObjects(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
STDCALL
ScanForSpecial(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_INQUIRY_DATA LunInfo,
    PIO_SCSI_CAPABILITIES PortCapabilities
    );

VOID
STDCALL
ResetScsiBus(
    IN PDEVICE_OBJECT DeviceObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, FindScsiDisks)
#pragma alloc_text(PAGE, CreateDiskDeviceObject)
#pragma alloc_text(PAGE, CalculateMbrCheckSum)
#pragma alloc_text(PAGE, EnumerateBusKey)
#pragma alloc_text(PAGE, UpdateGeometry)
#pragma alloc_text(PAGE, IsFloppyDevice)
#pragma alloc_text(PAGE, ScanForSpecial)
#pragma alloc_text(PAGE, ScsiDiskDeviceControl)
#pragma alloc_text(PAGE, ScsiDiskModeSelect)
#endif


NTSTATUS
STDCALL
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the SCSI hard disk class driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the name of the services node for this driver.

Return Value:

    The function value is the final status from the initialization operation.

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
    InitializationData.DeviceExtensionSize = DEVICE_EXTENSION_SIZE;

    InitializationData.DeviceType = FILE_DEVICE_DISK;
    InitializationData.DeviceCharacteristics = 0;

    //
    // Set entry points
    //

    InitializationData.ClassError = ScsiDiskProcessError;
    InitializationData.ClassReadWriteVerification = ScsiDiskReadWriteVerification;
    InitializationData.ClassFindDevices = FindScsiDisks;
    InitializationData.ClassFindDeviceCallBack = ScsiDiskDeviceVerification;
    InitializationData.ClassDeviceControl = ScsiDiskDeviceControl;
    InitializationData.ClassShutdownFlush = ScsiDiskShutdownFlush;
    InitializationData.ClassCreateClose = NULL;

    //
    // Call the class init routine
    //

    return ScsiClassInitialize( DriverObject, RegistryPath, &InitializationData);

} // end DriverEntry()



BOOLEAN
STDCALL
ScsiDiskDeviceVerification(
    IN PINQUIRYDATA InquiryData
    )

/*++

Routine Description:

    This routine checks InquiryData for the correct device type and qualifier.

Arguments:

    InquiryData - Pointer to the inquiry data for the device in question.

Return Value:

    True is returned if the correct device type is found.

--*/
{

    if (((InquiryData->DeviceType == DIRECT_ACCESS_DEVICE) ||
        (InquiryData->DeviceType == OPTICAL_DEVICE)) &&
        InquiryData->DeviceTypeQualifier == 0) {

        return TRUE;

    } else {
        return FALSE;
    }
}


BOOLEAN
STDCALL
FindScsiDisks(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN PCLASS_INIT_DATA InitializationData,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber
    )

/*++

Routine Description:

    This routine gets a port drivers capabilities, obtains the
    inquiry data, searches the SCSI bus for the port driver and creates
    the device objects for the disks found.

Arguments:

    DriverObject - Pointer to driver object created by system.

    PortDeviceObject - Device object use to send requests to port driver.

    PortNumber - Number for port driver.  Used to pass on to
                 CreateDiskDeviceObjects() and create device objects.

Return Value:

    True is returned if one disk was found and successfully created.

--*/

{
    PIO_SCSI_CAPABILITIES portCapabilities;
    PULONG diskCount;
    PCONFIGURATION_INFORMATION configurationInformation;
    PCHAR buffer;
    PSCSI_INQUIRY_DATA lunInfo;
    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PINQUIRYDATA inquiryData;
    ULONG scsiBus;
    ULONG adapterDisk;
    NTSTATUS status;
    BOOLEAN foundOne = FALSE;

    PAGED_CODE();

    //
    // Call port driver to get adapter capabilities.
    //

    status = ScsiClassGetCapabilities(PortDeviceObject, &portCapabilities);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"FindScsiDevices: ScsiClassGetCapabilities failed\n"));
        return(FALSE);
    }

    //
    // Call port driver to get inquiry information to find disks.
    //

    status = ScsiClassGetInquiryData(PortDeviceObject, (PSCSI_ADAPTER_BUS_INFO *) &buffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"FindScsiDevices: ScsiClassGetInquiryData failed\n"));
        return(FALSE);
    }

    //
    // Do a quick scan of the devices on this adapter to determine how many
    // disks are on this adapter.  This is used to determine the number of
    // SRB zone elements to allocate.
    //

    adapterDisk = 0;
    adapterInfo = (PVOID) buffer;

    adapterDisk = ScsiClassFindUnclaimedDevices(InitializationData, adapterInfo);

    //
    // Allocate a zone of SRB for disks on this adapter.
    //

    if (adapterDisk == 0) {

        //
        // No free disks were found.
        //

        return(FALSE);
    }

    //
    // Get the number of disks already initialized.
    //

    configurationInformation = IoGetConfigurationInformation();
    diskCount = &configurationInformation->DiskCount;

    //
    // For each SCSI bus this adapter supports ...
    //

    for (scsiBus=0; scsiBus < (ULONG)adapterInfo->NumberOfBuses; scsiBus++) {

        //
        // Get the SCSI bus scan data for this bus.
        //

        lunInfo = (PVOID) (buffer + adapterInfo->BusData[scsiBus].InquiryDataOffset);

        //
        // Search list for unclaimed disk devices.
        //

        while (adapterInfo->BusData[scsiBus].InquiryDataOffset) {

            inquiryData = (PVOID)lunInfo->InquiryData;

            if (((inquiryData->DeviceType == DIRECT_ACCESS_DEVICE) ||
                (inquiryData->DeviceType == OPTICAL_DEVICE)) &&
                inquiryData->DeviceTypeQualifier == 0 &&
                (!lunInfo->DeviceClaimed)) {

                DebugPrint((1,
                            "FindScsiDevices: Vendor string is %.24s\n",
                            inquiryData->VendorId));

                //
                // Create device objects for disk
                //

                status = CreateDiskDeviceObject(DriverObject,
                                                RegistryPath,
                                                PortDeviceObject,
                                                PortNumber,
                                                diskCount,
                                                portCapabilities,
                                                lunInfo,
                                                InitializationData);

                if (NT_SUCCESS(status)) {

                    //
                    // Increment system disk device count.
                    //

                    (*diskCount)++;
                    foundOne = TRUE;

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

    //
    // Buffer is allocated by ScsiClassGetInquiryData and must be free returning.
    //

    ExFreePool(buffer);

    return(foundOne);

} // end FindScsiDisks()


NTSTATUS
STDCALL
CreateDiskDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN PDEVICE_OBJECT PortDeviceObject,
    IN ULONG PortNumber,
    IN PULONG DeviceCount,
    IN PIO_SCSI_CAPABILITIES PortCapabilities,
    IN PSCSI_INQUIRY_DATA LunInfo,
    IN PCLASS_INIT_DATA InitData
    )

/*++

Routine Description:

    This routine creates an object for the physical device and then searches
    the device for partitions and creates an object for each partition.

Arguments:

    DriverObject - Pointer to driver object created by system.

    PortDeviceObject - Miniport device object.

    PortNumber   - port number.  Used in creating disk objects.

    DeviceCount  - Number of previously installed devices.

    PortCapabilities - Capabilities of this SCSI port.

    LunInfo      - LUN specific information.

Return Value:

    NTSTATUS

--*/
{
    CCHAR          ntNameBuffer[MAXIMUM_FILENAME_LENGTH];
    STRING         ntNameString;
    UNICODE_STRING ntUnicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE         handle;
    NTSTATUS       status;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_OBJECT physicalDevice;
    PDISK_GEOMETRY diskGeometry = NULL;
    PDEVICE_EXTENSION deviceExtension = NULL;
    PDEVICE_EXTENSION physicalDeviceExtension;
    UCHAR          pathId = LunInfo->PathId;
    UCHAR          targetId = LunInfo->TargetId;
    UCHAR          lun = LunInfo->Lun;
    BOOLEAN        writeCache;
    PVOID          senseData = NULL;
    ULONG          srbFlags;
    ULONG          timeOut = 0;
    BOOLEAN        srbListInitialized = FALSE;


    PAGED_CODE();

    //
    // Set up an object directory to contain the objects for this
    // device and all its partitions.
    //

    sprintf(ntNameBuffer,
            "\\Device\\Harddisk%lu",
            *DeviceCount);

    RtlInitString(&ntNameString,
                  ntNameBuffer);

    status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                          &ntNameString,
                                          TRUE);

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    InitializeObjectAttributes(&objectAttributes,
                               &ntUnicodeString,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);

    status = ZwCreateDirectoryObject(&handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &objectAttributes);

    RtlFreeUnicodeString(&ntUnicodeString);

    if (!NT_SUCCESS(status)) {

        DebugPrint((1,
                    "CreateDiskDeviceObjects: Could not create directory %s\n",
                    ntNameBuffer));

        return(status);
    }

    //
    // Claim the device.
    //

    status = ScsiClassClaimDevice(PortDeviceObject,
                                  LunInfo,
                                  FALSE,
                                  &PortDeviceObject);

    if (!NT_SUCCESS(status)) {
        ZwMakeTemporaryObject(handle);
        ZwClose(handle);
        return status;
    }

    //
    // Create a device object for this device. Each physical disk will
    // have at least one device object. The required device object
    // describes the entire device. Its directory path is
    // \Device\HarddiskN\Partition0, where N = device number.
    //

    sprintf(ntNameBuffer,
            "\\Device\\Harddisk%lu\\Partition0",
            *DeviceCount);


    status = ScsiClassCreateDeviceObject(DriverObject,
                                         ntNameBuffer,
                                         NULL,
                                         &deviceObject,
                                         InitData);

    if (!NT_SUCCESS(status)) {

        DebugPrint((1,
                    "CreateDiskDeviceObjects: Can not create device object %s\n",
                    ntNameBuffer));

        goto CreateDiskDeviceObjectsExit;
    }

    //
    // Indicate that IRPs should include MDLs for data transfers.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    //
    // Check if this is during initialization. If not indicate that
    // system initialization already took place and this disk is ready
    // to be accessed.
    //

    if (!RegistryPath) {
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    //
    // Check for removable media support.
    //

    if (((PINQUIRYDATA)LunInfo->InquiryData)->RemovableMedia) {
        deviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
    }

    //
    // Set up required stack size in device object.
    //

    deviceObject->StackSize = (CCHAR)PortDeviceObject->StackSize + 1;

    deviceExtension = deviceObject->DeviceExtension;

    //
    // Allocate spinlock for split request completion.
    //

    KeInitializeSpinLock(&deviceExtension->SplitRequestSpinLock);

    //
    // Initialize lock count to zero. The lock count is used to
    // disable the ejection mechanism on devices that support
    // removable media. Only the lock count in the physical
    // device extension is used.
    //

    deviceExtension->LockCount = 0;

    //
    // Save system disk number.
    //

    deviceExtension->DeviceNumber = *DeviceCount;

    //
    // Copy port device object pointer to the device extension.
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
    // This is the physical device object.
    //

    physicalDevice = deviceObject;
    physicalDeviceExtension = deviceExtension;

    //
    // Save address of port driver capabilities.
    //

    deviceExtension->PortCapabilities = PortCapabilities;

    //
    // Build the lookaside list for srb's for the physical disk. Should only
    // need a couple.
    //

    ScsiClassInitializeSrbLookasideList(deviceExtension,
                                        PARTITION0_LIST_SIZE);

    srbListInitialized = TRUE;

    //
    // Initialize the srb flags.
    //

    if (((PINQUIRYDATA)LunInfo->InquiryData)->CommandQueue &&
        PortCapabilities->TaggedQueuing) {

        deviceExtension->SrbFlags  = SRB_FLAGS_QUEUE_ACTION_ENABLE;

    } else {

        deviceExtension->SrbFlags  = 0;

    }

    //
    // Allow queued requests if this is not removable media.
    //

    if (!(deviceObject->Characteristics & FILE_REMOVABLE_MEDIA)) {

        deviceExtension->SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;

    }

    //
    // Look for controller that require special flags.
    //

    ScanForSpecial(deviceObject,
                    LunInfo,
                    PortCapabilities);

    srbFlags = deviceExtension->SrbFlags;

    //
    // Allocate buffer for drive geometry.
    //

    diskGeometry = ExAllocatePool(NonPagedPool, sizeof(DISK_GEOMETRY));

    if (diskGeometry == NULL) {

        DebugPrint((1,
           "CreateDiskDeviceObjects: Can not allocate disk geometry buffer\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CreateDiskDeviceObjectsExit;
    }

    deviceExtension->DiskGeometry = diskGeometry;

    //
    // Allocate request sense buffer.
    //

    senseData = ExAllocatePool(NonPagedPoolCacheAligned, SENSE_BUFFER_SIZE);

    if (senseData == NULL) {

        //
        // The buffer can not be allocated.
        //

        DebugPrint((1,
           "CreateDiskDeviceObjects: Can not allocate request sense buffer\n"));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CreateDiskDeviceObjectsExit;
    }

    //
    // Set the sense data pointer in the device extension.
    //

    deviceExtension->SenseData = senseData;

    //
    // Physical device object will describe the entire
    // device, starting at byte offset 0.
    //

    deviceExtension->StartingOffset.QuadPart = (LONGLONG)(0);

    //
    // TargetId/LUN describes a device location on the SCSI bus.
    // This information comes from the inquiry buffer.
    //

    deviceExtension->PortNumber = (UCHAR)PortNumber;
    deviceExtension->PathId = pathId;
    deviceExtension->TargetId = targetId;
    deviceExtension->Lun = lun;

    //
    // Set timeout value in seconds.
    //

    timeOut = ScsiClassQueryTimeOutRegistryValue(RegistryPath);
    if (timeOut) {
        deviceExtension->TimeOutValue = timeOut;
    } else {
        deviceExtension->TimeOutValue = SCSI_DISK_TIMEOUT;
    }

    //
    // Back pointer to device object.
    //

    deviceExtension->DeviceObject = deviceObject;

    //
    // If this is a removable device, then make sure it is not a floppy.
    // Perform a mode sense command to determine the media type. Note
    // IsFloppyDevice also checks for write cache enabled.
    //

    if (IsFloppyDevice(deviceObject) && deviceObject->Characteristics & FILE_REMOVABLE_MEDIA &&
        (((PINQUIRYDATA)LunInfo->InquiryData)->DeviceType == DIRECT_ACCESS_DEVICE)) {

        status = STATUS_NO_SUCH_DEVICE;
        goto CreateDiskDeviceObjectsExit;
    }

    DisableWriteCache(deviceObject,LunInfo);

    writeCache = deviceExtension->DeviceFlags & DEV_WRITE_CACHE;

    //
    // NOTE: At this point one device object has been successfully created.
    // from here on out return success.
    //

    //
    // Do READ CAPACITY. This SCSI command
    // returns the number of bytes on a device.
    // Device extension is updated with device size.
    //

    status = ScsiClassReadDriveCapacity(deviceObject);

    //
    // If the read capcity failed then just return, unless this is a
    // removable disk where a device object partition needs to be created.
    //

    if (!NT_SUCCESS(status) &&
        !(deviceObject->Characteristics & FILE_REMOVABLE_MEDIA)) {

        DebugPrint((1,
            "CreateDiskDeviceObjects: Can't read capacity for device %s\n",
            ntNameBuffer));

        return(STATUS_SUCCESS);

    } else {

        //
        // Make sure the volume verification bit is off so that
        // IoReadPartitionTable will work.
        //

        deviceObject->Flags &= ~DO_VERIFY_VOLUME;
    }

    status = CreatePartitionDeviceObjects(deviceObject, RegistryPath);

    if (NT_SUCCESS(status))
        return STATUS_SUCCESS;


CreateDiskDeviceObjectsExit:

    //
    // Release the device since an error occurred.
    //

    ScsiClassClaimDevice(PortDeviceObject,
                         LunInfo,
                         TRUE,
                         NULL);

    if (diskGeometry != NULL) {
        ExFreePool(diskGeometry);
    }

    if (senseData != NULL) {
        ExFreePool(senseData);
    }

    if (deviceObject != NULL) {

        if (srbListInitialized) {
            ExDeleteNPagedLookasideList(&deviceExtension->SrbLookasideListHead);
        }

        IoDeleteDevice(deviceObject);
    }

    //
    // Delete directory and return.
    //

    if (!NT_SUCCESS(status)) {
        ZwMakeTemporaryObject(handle);
    }

    ZwClose(handle);

    return(status);

} // end CreateDiskDeviceObjects()


NTSTATUS
STDCALL
CreatePartitionDeviceObjects(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    CCHAR          ntNameBuffer[MAXIMUM_FILENAME_LENGTH];
    ULONG          partitionNumber = 0;
    NTSTATUS       status;
    PDEVICE_OBJECT deviceObject = NULL;
    PDISK_GEOMETRY diskGeometry = NULL;
    PDRIVE_LAYOUT_INFORMATION partitionList = NULL;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_EXTENSION physicalDeviceExtension;
    PCLASS_INIT_DATA initData = NULL;
    PDISK_DATA     diskData;
    PDISK_DATA     physicalDiskData;
    ULONG          bytesPerSector;
    UCHAR          sectorShift;
    ULONG          srbFlags;
    ULONG          dmByteSkew = 0;
    PULONG         dmSkew;
    BOOLEAN        dmActive = FALSE;
    ULONG          numberListElements = 0;


    //
    // Get physical device geometry information for partition table reads.
    //

    physicalDeviceExtension = PhysicalDeviceObject->DeviceExtension;
    diskGeometry = physicalDeviceExtension->DiskGeometry;
    bytesPerSector = diskGeometry->BytesPerSector;

    //
    // Make sure sector size is not zero.
    //

    if (bytesPerSector == 0) {

        //
        // Default sector size for disk is 512.
        //

        bytesPerSector = diskGeometry->BytesPerSector = 512;
    }

    sectorShift = physicalDeviceExtension->SectorShift;

    //
    // Set pointer to disk data area that follows device extension.
    //

    diskData = (PDISK_DATA)(physicalDeviceExtension + 1);
    diskData->PartitionListState = Initializing;

    //
    // Determine is DM Driver is loaded on an IDE drive that is
    // under control of Atapi - this could be either a crashdump or
    // an Atapi device is sharing the controller with an IDE disk.
    //

    HalExamineMBR(PhysicalDeviceObject,
                  physicalDeviceExtension->DiskGeometry->BytesPerSector,
                  (ULONG)0x54,
                  (PVOID)&dmSkew);

    if (dmSkew) {

        //
        // Update the device extension, so that the call to IoReadPartitionTable
        // will get the correct information. Any I/O to this disk will have
        // to be skewed by *dmSkew sectors aka DMByteSkew.
        //

        physicalDeviceExtension->DMSkew = *dmSkew;
        physicalDeviceExtension->DMActive = TRUE;
        physicalDeviceExtension->DMByteSkew = physicalDeviceExtension->DMSkew * bytesPerSector;

        //
        // Save away the infomation that we need, since this deviceExtension will soon be
        // blown away.
        //

        dmActive = TRUE;
        dmByteSkew = physicalDeviceExtension->DMByteSkew;

    }

    //
    // Create objects for all the partitions on the device.
    //

    status = IoReadPartitionTable(PhysicalDeviceObject,
                                  physicalDeviceExtension->DiskGeometry->BytesPerSector,
                                  TRUE,
                                  (PVOID)&partitionList);

    //
    // If the I/O read partition table failed and this is a removable device,
    // then fix up the partition list to make it look like there is one
    // zero length partition.
    //
    DPRINT("IoReadPartitionTable() status: 0x%08X\n", status);
    if ((!NT_SUCCESS(status) || partitionList->PartitionCount == 0) &&
        PhysicalDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {

        if (!NT_SUCCESS(status)) {

            //
            // Remember this disk is not ready.
            //

            diskData->DriveNotReady = TRUE;

        } else {

            //
            // Free the partition list allocated by IoReadPartitionTable.
            //

            ExFreePool(partitionList);
        }

        //
        // Allocate and zero a partition list.
        //

        partitionList = ExAllocatePool(NonPagedPool, sizeof(*partitionList ));


        if (partitionList != NULL) {

            RtlZeroMemory( partitionList, sizeof( *partitionList ));

            //
            // Set the partition count to one and the status to success
            // so one device object will be created. Set the partition type
            // to a bogus value.
            //

            partitionList->PartitionCount = 1;

            status = STATUS_SUCCESS;
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Record disk signature.
        //

        diskData->Signature = partitionList->Signature;

        //
        // If disk signature is zero, then calculate the MBR checksum.
        //

        if (!diskData->Signature) {

            if (!CalculateMbrCheckSum(physicalDeviceExtension,
                                      &diskData->MbrCheckSum)) {

                DebugPrint((1,
                            "SCSIDISK: Can't calculate MBR checksum for disk %x\n",
                            physicalDeviceExtension->DeviceNumber));
            } else {

                DebugPrint((2,
                           "SCSIDISK: MBR checksum for disk %x is %x\n",
                           physicalDeviceExtension->DeviceNumber,
                           diskData->MbrCheckSum));
            }
        }

        //
        // Check the registry and determine if the BIOS knew about this drive.  If
        // it did then update the geometry with the BIOS information.
        //

        UpdateGeometry(physicalDeviceExtension);

        srbFlags = physicalDeviceExtension->SrbFlags;

        initData = ExAllocatePool(NonPagedPool, sizeof(CLASS_INIT_DATA));
        if (!initData)
        {
            DebugPrint((1,
                        "Disk.CreatePartionDeviceObjects - Allocation of initData failed\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto CreatePartitionDeviceObjectsExit;
        }

        RtlZeroMemory(initData, sizeof(CLASS_INIT_DATA));

        initData->InitializationDataSize     = sizeof(CLASS_INIT_DATA);
        initData->DeviceExtensionSize        = DEVICE_EXTENSION_SIZE;
        initData->DeviceType                 = FILE_DEVICE_DISK;
        initData->DeviceCharacteristics      = PhysicalDeviceObject->Characteristics;
        initData->ClassError                 = physicalDeviceExtension->ClassError;
        initData->ClassReadWriteVerification = physicalDeviceExtension->ClassReadWriteVerification;
        initData->ClassFindDevices           = physicalDeviceExtension->ClassFindDevices;
        initData->ClassDeviceControl         = physicalDeviceExtension->ClassDeviceControl;
        initData->ClassShutdownFlush         = physicalDeviceExtension->ClassShutdownFlush;
        initData->ClassCreateClose           = physicalDeviceExtension->ClassCreateClose;
        initData->ClassStartIo               = physicalDeviceExtension->ClassStartIo;

        //
        // Create device objects for the device partitions (if any).
        // PartitionCount includes physical device partition 0,
        // so only one partition means no objects to create.
        //

        DebugPrint((2,
                    "CreateDiskDeviceObjects: Number of partitions is %d\n",
                    partitionList->PartitionCount));

        for (partitionNumber = 0; partitionNumber <
            partitionList->PartitionCount; partitionNumber++) {

            //
            // Create partition object and set up partition parameters.
            //

            sprintf(ntNameBuffer,
                    "\\Device\\Harddisk%lu\\Partition%lu",
                    physicalDeviceExtension->DeviceNumber,
                    partitionNumber + 1);

            DebugPrint((2,
                        "CreateDiskDeviceObjects: Create device object %s\n",
                        ntNameBuffer));

            status = ScsiClassCreateDeviceObject(PhysicalDeviceObject->DriverObject,
                                                 ntNameBuffer,
                                                 PhysicalDeviceObject,
                                                 &deviceObject,
                                                 initData);

            if (!NT_SUCCESS(status)) {

                DebugPrint((1, "CreateDiskDeviceObjects: Can't create device object for %s\n", ntNameBuffer));

                break;
            }

            //
            // Set up device object fields.
            //

            deviceObject->Flags |= DO_DIRECT_IO;

            //
            // Check if this is during initialization. If not indicate that
            // system initialization already took place and this disk is ready
            // to be accessed.
            //

            if (!RegistryPath) {
                deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            }

            deviceObject->StackSize = (CCHAR)physicalDeviceExtension->PortDeviceObject->StackSize + 1;

            //
            // Set up device extension fields.
            //

            deviceExtension = deviceObject->DeviceExtension;

            if (dmActive) {

                //
                // Restore any saved DM values.
                //

                deviceExtension->DMByteSkew = dmByteSkew;
                deviceExtension->DMSkew     = *dmSkew;
                deviceExtension->DMActive   = TRUE;

            }

            //
            // Link new device extension to previous disk data
            // to support dynamic partitioning.
            //

            diskData->NextPartition = deviceExtension;

            //
            // Get pointer to new disk data.
            //

            diskData = (PDISK_DATA)(deviceExtension + 1);

            //
            // Set next partition pointer to NULL in case this is the
            // last partition.
            //

            diskData->NextPartition = NULL;

            //
            // Allocate spinlock for zoning for split-request completion.
            //

            KeInitializeSpinLock(&deviceExtension->SplitRequestSpinLock);

            //
            // Copy port device object pointer to device extension.
            //

            deviceExtension->PortDeviceObject = physicalDeviceExtension->PortDeviceObject;

            //
            // Set the alignment requirements for the device based on the
            // host adapter requirements
            //

            if (physicalDeviceExtension->PortDeviceObject->AlignmentRequirement > deviceObject->AlignmentRequirement) {
                deviceObject->AlignmentRequirement = physicalDeviceExtension->PortDeviceObject->AlignmentRequirement;
            }


            if (srbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE) {
                numberListElements = 30;
            } else {
                numberListElements = 8;
            }

            //
            // Build the lookaside list for srb's for this partition based on
            // whether the adapter and disk can do tagged queueing.
            //

            ScsiClassInitializeSrbLookasideList(deviceExtension,
                                                numberListElements);

            deviceExtension->SrbFlags = srbFlags;

            //
            // Set the sense-data pointer in the device extension.
            //

            deviceExtension->SenseData        = physicalDeviceExtension->SenseData;
            deviceExtension->PortCapabilities = physicalDeviceExtension->PortCapabilities;
            deviceExtension->DiskGeometry     = diskGeometry;
            diskData->PartitionOrdinal        = diskData->PartitionNumber = partitionNumber + 1;
            diskData->PartitionType           = partitionList->PartitionEntry[partitionNumber].PartitionType;
            diskData->BootIndicator           = partitionList->PartitionEntry[partitionNumber].BootIndicator;

            DebugPrint((2, "CreateDiskDeviceObjects: Partition type is %x\n",
                diskData->PartitionType));

            deviceExtension->StartingOffset  = partitionList->PartitionEntry[partitionNumber].StartingOffset;
            deviceExtension->PartitionLength = partitionList->PartitionEntry[partitionNumber].PartitionLength;
            diskData->HiddenSectors          = partitionList->PartitionEntry[partitionNumber].HiddenSectors;
            deviceExtension->PortNumber      = physicalDeviceExtension->PortNumber;
            deviceExtension->PathId          = physicalDeviceExtension->PathId;
            deviceExtension->TargetId        = physicalDeviceExtension->TargetId;
            deviceExtension->Lun             = physicalDeviceExtension->Lun;

            //
            // Check for removable media support.
            //

            if (PhysicalDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {
                deviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
            }

            //
            // Set timeout value in seconds.
            //

            deviceExtension->TimeOutValue = physicalDeviceExtension->TimeOutValue;
            deviceExtension->DiskGeometry->BytesPerSector = bytesPerSector;
            deviceExtension->SectorShift  = sectorShift;
            deviceExtension->DeviceObject = deviceObject;
            deviceExtension->DeviceFlags |= physicalDeviceExtension->DeviceFlags;

        } // end for (partitionNumber) ...

        //
        // Free the buffer allocated by reading the
        // partition table.
        //

        ExFreePool(partitionList);

    } else {

CreatePartitionDeviceObjectsExit:

        if (partitionList) {
            ExFreePool(partitionList);
        }
        if (initData) {
            ExFreePool(initData);
        }

        return status;

    } // end if...else


    physicalDiskData = (PDISK_DATA)(physicalDeviceExtension + 1);
    physicalDiskData->PartitionListState = Initialized;

    return(STATUS_SUCCESS);


} // end CreatePartitionDeviceObjects()


NTSTATUS
STDCALL
ScsiDiskReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    I/O System entry for read and write requests to SCSI disks.

Arguments:

    DeviceObject - Pointer to driver object created by system.
    Irp - IRP involved.

Return Value:

    NT Status

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG transferByteCount = currentIrpStack->Parameters.Read.Length;
    LARGE_INTEGER startingOffset;

    //
    // Verify parameters of this request.
    // Check that ending sector is within partition and
    // that number of bytes to transfer is a multiple of
    // the sector size.
    //

    startingOffset.QuadPart = (currentIrpStack->Parameters.Read.ByteOffset.QuadPart +
                               transferByteCount);

    if ((startingOffset.QuadPart > deviceExtension->PartitionLength.QuadPart) ||
        (transferByteCount & (deviceExtension->DiskGeometry->BytesPerSector - 1))) {

        //
        // This error maybe caused by the fact that the drive is not ready.
        //

        if (((PDISK_DATA)(deviceExtension + 1))->DriveNotReady) {

            //
            // Flag this as a user errror so that a popup is generated.
            //

            Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
            IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);

        } else {

            //
            // Note fastfat depends on this parameter to determine when to
            // remount do to a sector size change.
            //

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        }

        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;

} // end ScsiDiskReadWrite()


NTSTATUS
STDCALL
ScsiDiskDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    I/O system entry for device controls to SCSI disks.

Arguments:

    DeviceObject - Pointer to driver object created by system.
    Irp - IRP involved.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION      deviceExtension = DeviceObject->DeviceExtension;
    PDISK_DATA             diskData = (PDISK_DATA)(deviceExtension + 1);
    PSCSI_REQUEST_BLOCK    srb;
    PCDB                   cdb;
    PMODE_PARAMETER_HEADER modeData;
    PIRP                   irp2;
    ULONG                  length;
    NTSTATUS               status;
    KEVENT                 event;
    IO_STATUS_BLOCK        ioStatus;

    PAGED_CODE();

    srb = ExAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

    if (srb == NULL) {

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Write zeros to Srb.
    //

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

    cdb = (PCDB)srb->Cdb;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

    case SMART_GET_VERSION: {

        ULONG_PTR buffer;
        PSRB_IO_CONTROL  srbControl;
        PGETVERSIONINPARAMS versionParams;

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(GETVERSIONINPARAMS)) {
                status = STATUS_INVALID_PARAMETER;
                break;
        }

        //
        // Create notification event object to be used to signal the
        // request completion.
        //

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        srbControl = ExAllocatePool(NonPagedPool,
                                    sizeof(SRB_IO_CONTROL) + sizeof(GETVERSIONINPARAMS));

        if (!srbControl) {
            status =  STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // fill in srbControl fields
        //

        srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
        RtlMoveMemory (srbControl->Signature, "SCSIDISK", 8);
        srbControl->Timeout = deviceExtension->TimeOutValue;
        srbControl->Length = sizeof(GETVERSIONINPARAMS);
        srbControl->ControlCode = IOCTL_SCSI_MINIPORT_SMART_VERSION;

        //
        // Point to the 'buffer' portion of the SRB_CONTROL
        //

        buffer = (ULONG_PTR)srbControl + srbControl->HeaderLength;

        //
        // Ensure correct target is set in the cmd parameters.
        //

        versionParams = (PGETVERSIONINPARAMS)buffer;
        versionParams->bIDEDeviceMap = deviceExtension->TargetId;

        //
        // Copy the IOCTL parameters to the srb control buffer area.
        //

        RtlMoveMemory((PVOID)buffer, Irp->AssociatedIrp.SystemBuffer, sizeof(GETVERSIONINPARAMS));


        irp2 = IoBuildDeviceIoControlRequest(IOCTL_SCSI_MINIPORT,
                                            deviceExtension->PortDeviceObject,
                                            srbControl,
                                            sizeof(SRB_IO_CONTROL) + sizeof(GETVERSIONINPARAMS),
                                            srbControl,
                                            sizeof(SRB_IO_CONTROL) + sizeof(GETVERSIONINPARAMS),
                                            FALSE,
                                            &event,
                                            &ioStatus);

        if (irp2 == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Call the port driver with the request and wait for it to complete.
        //

        status = IoCallDriver(deviceExtension->PortDeviceObject, irp2);

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        //
        // If successful, copy the data received into the output buffer.
        // This should only fail in the event that the IDE driver is older than this driver.
        //

        if (NT_SUCCESS(status)) {

            buffer = (ULONG_PTR)srbControl + srbControl->HeaderLength;

            RtlMoveMemory ( Irp->AssociatedIrp.SystemBuffer, (PVOID)buffer, sizeof(GETVERSIONINPARAMS));
            Irp->IoStatus.Information = sizeof(GETVERSIONINPARAMS);
        }

        ExFreePool(srbControl);
        break;
    }

    case SMART_RCV_DRIVE_DATA: {

        PSENDCMDINPARAMS cmdInParameters = ((PSENDCMDINPARAMS)Irp->AssociatedIrp.SystemBuffer);
        ULONG            controlCode = 0;
        PSRB_IO_CONTROL  srbControl;
        ULONG_PTR        buffer;

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            (sizeof(SENDCMDINPARAMS) - 1)) {
                status = STATUS_INVALID_PARAMETER;
                break;

        } else if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            (sizeof(SENDCMDOUTPARAMS) + 512 - 1)) {
                status = STATUS_INVALID_PARAMETER;
                break;
        }

        //
        // Create notification event object to be used to signal the
        // request completion.
        //

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        if (cmdInParameters->irDriveRegs.bCommandReg == ID_CMD) {

            length = IDENTIFY_BUFFER_SIZE + sizeof(SENDCMDOUTPARAMS);
            controlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;

        } else if (cmdInParameters->irDriveRegs.bCommandReg == SMART_CMD) {
            switch (cmdInParameters->irDriveRegs.bFeaturesReg) {
                case READ_ATTRIBUTES:
                    controlCode = IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS;
                    length = READ_ATTRIBUTE_BUFFER_SIZE + sizeof(SENDCMDOUTPARAMS);
                    break;
                case READ_THRESHOLDS:
                    controlCode = IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS;
                    length = READ_THRESHOLD_BUFFER_SIZE + sizeof(SENDCMDOUTPARAMS);
                    break;
                default:
                    status = STATUS_INVALID_PARAMETER;
                    break;
            }
        } else {

            status = STATUS_INVALID_PARAMETER;
        }

        if (controlCode == 0) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        srbControl = ExAllocatePool(NonPagedPool,
                                    sizeof(SRB_IO_CONTROL) + length);

        if (!srbControl) {
            status =  STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // fill in srbControl fields
        //

        srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
        RtlMoveMemory (srbControl->Signature, "SCSIDISK", 8);
        srbControl->Timeout = deviceExtension->TimeOutValue;
        srbControl->Length = length;
        srbControl->ControlCode = controlCode;

        //
        // Point to the 'buffer' portion of the SRB_CONTROL
        //

        buffer = (ULONG_PTR)srbControl + srbControl->HeaderLength;

        //
        // Ensure correct target is set in the cmd parameters.
        //

        cmdInParameters->bDriveNumber = deviceExtension->TargetId;

        //
        // Copy the IOCTL parameters to the srb control buffer area.
        //

        RtlMoveMemory((PVOID)buffer, Irp->AssociatedIrp.SystemBuffer, sizeof(SENDCMDINPARAMS) - 1);

        irp2 = IoBuildDeviceIoControlRequest(IOCTL_SCSI_MINIPORT,
                                            deviceExtension->PortDeviceObject,
                                            srbControl,
                                            sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1,
                                            srbControl,
                                            sizeof(SRB_IO_CONTROL) + length,
                                            FALSE,
                                            &event,
                                            &ioStatus);

        if (irp2 == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Call the port driver with the request and wait for it to complete.
        //

        status = IoCallDriver(deviceExtension->PortDeviceObject, irp2);

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        //
        // If successful, copy the data received into the output buffer
        //

        buffer = (ULONG_PTR)srbControl + srbControl->HeaderLength;

        if (NT_SUCCESS(status)) {

            RtlMoveMemory ( Irp->AssociatedIrp.SystemBuffer, (PVOID)buffer, length - 1);
            Irp->IoStatus.Information = length - 1;

        } else {

            RtlMoveMemory ( Irp->AssociatedIrp.SystemBuffer, (PVOID)buffer, (sizeof(SENDCMDOUTPARAMS) - 1));
            Irp->IoStatus.Information = sizeof(SENDCMDOUTPARAMS) - 1;

        }

        ExFreePool(srbControl);
        break;

    }

    case SMART_SEND_DRIVE_COMMAND: {

        PSENDCMDINPARAMS cmdInParameters = ((PSENDCMDINPARAMS)Irp->AssociatedIrp.SystemBuffer);
        PSRB_IO_CONTROL  srbControl;
        ULONG            controlCode = 0;
        ULONG_PTR        buffer;

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
               (sizeof(SENDCMDINPARAMS) - 1)) {
                status = STATUS_INVALID_PARAMETER;
                break;

        } else if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                      (sizeof(SENDCMDOUTPARAMS) - 1)) {
                status = STATUS_INVALID_PARAMETER;
                break;
        }

        //
        // Create notification event object to be used to signal the
        // request completion.
        //

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        length = 0;

        if (cmdInParameters->irDriveRegs.bCommandReg == SMART_CMD) {
            switch (cmdInParameters->irDriveRegs.bFeaturesReg) {

                case ENABLE_SMART:
                    controlCode = IOCTL_SCSI_MINIPORT_ENABLE_SMART;
                    break;

                case DISABLE_SMART:
                    controlCode = IOCTL_SCSI_MINIPORT_DISABLE_SMART;
                    break;

                case  RETURN_SMART_STATUS:

                    //
                    // Ensure bBuffer is at least 2 bytes (to hold the values of
                    // cylinderLow and cylinderHigh).
                    //

                    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                        (sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(IDEREGS))) {

                        status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    controlCode = IOCTL_SCSI_MINIPORT_RETURN_STATUS;
                    length = sizeof(IDEREGS);
                    break;

                case ENABLE_DISABLE_AUTOSAVE:
                    controlCode = IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE;
                    break;

                case SAVE_ATTRIBUTE_VALUES:
                    controlCode = IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES;
                    break;

                case EXECUTE_OFFLINE_DIAGS:
                    controlCode = IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS;

                default:
                    status = STATUS_INVALID_PARAMETER;
                    break;
            }
        } else {

            status = STATUS_INVALID_PARAMETER;
        }

        if (controlCode == 0) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        length += (sizeof(SENDCMDOUTPARAMS) > sizeof(SENDCMDINPARAMS)) ? sizeof(SENDCMDOUTPARAMS) : sizeof(SENDCMDINPARAMS);;
        srbControl = ExAllocatePool(NonPagedPool,
                                    sizeof(SRB_IO_CONTROL) + length);

        if (!srbControl) {
            status =  STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // fill in srbControl fields
        //

        srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
        RtlMoveMemory (srbControl->Signature, "SCSIDISK", 8);
        srbControl->Timeout = deviceExtension->TimeOutValue;
        srbControl->Length = length;

        //
        // Point to the 'buffer' portion of the SRB_CONTROL
        //

        buffer = (ULONG_PTR)srbControl + srbControl->HeaderLength;

        //
        // Ensure correct target is set in the cmd parameters.
        //

        cmdInParameters->bDriveNumber = deviceExtension->TargetId;

        //
        // Copy the IOCTL parameters to the srb control buffer area.
        //

        RtlMoveMemory((PVOID)buffer, Irp->AssociatedIrp.SystemBuffer, sizeof(SENDCMDINPARAMS) - 1);

        srbControl->ControlCode = controlCode;

        irp2 = IoBuildDeviceIoControlRequest(IOCTL_SCSI_MINIPORT,
                                            deviceExtension->PortDeviceObject,
                                            srbControl,
                                            sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1,
                                            srbControl,
                                            sizeof(SRB_IO_CONTROL) + length,
                                            FALSE,
                                            &event,
                                            &ioStatus);

        if (irp2 == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Call the port driver with the request and wait for it to complete.
        //

        status = IoCallDriver(deviceExtension->PortDeviceObject, irp2);

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        //
        // Copy the data received into the output buffer. Since the status buffer
        // contains error information also, always perform this copy. IO will will
        // either pass this back to the app, or zero it, in case of error.
        //

        buffer = (ULONG_PTR)srbControl + srbControl->HeaderLength;

        //
        // Update the return buffer size based on the sub-command.
        //

        if (cmdInParameters->irDriveRegs.bFeaturesReg == RETURN_SMART_STATUS) {
            length = sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(IDEREGS);
        } else {
            length = sizeof(SENDCMDOUTPARAMS) - 1;
        }

        RtlMoveMemory ( Irp->AssociatedIrp.SystemBuffer, (PVOID)buffer, length);
        Irp->IoStatus.Information = length;

        ExFreePool(srbControl);
        break;

    }

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        {

        PDEVICE_EXTENSION physicalDeviceExtension;
        PDISK_DATA        physicalDiskData;
        BOOLEAN           removable = FALSE;
        BOOLEAN           listInitialized = FALSE;

        if ( irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof( DISK_GEOMETRY ) ) {

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        status = STATUS_SUCCESS;

        physicalDeviceExtension = deviceExtension->PhysicalDevice->DeviceExtension;
        physicalDiskData = (PDISK_DATA)(physicalDeviceExtension + 1);

        removable = (BOOLEAN)DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA;
        listInitialized = (physicalDiskData->PartitionListState == Initialized);

        if (removable || (!listInitialized))
        {
            //
            // Issue ReadCapacity to update device extension
            // with information for current media.
            //

            status = ScsiClassReadDriveCapacity(deviceExtension->PhysicalDevice);

        }

        if (removable) {

            if (!NT_SUCCESS(status)) {

                //
                // Note the drive is not ready.
                //

                diskData->DriveNotReady = TRUE;

                break;
            }

            //
            // Note the drive is now ready.
            //

            diskData->DriveNotReady = FALSE;

        } else if (NT_SUCCESS(status)) {

            // ReadDriveCapacity was allright, create Partition Objects

            if (physicalDiskData->PartitionListState == NotInitialized) {
                    status = CreatePartitionDeviceObjects(deviceExtension->PhysicalDevice, NULL);
            }
        }

        if (NT_SUCCESS(status)) {

            //
            // Copy drive geometry information from device extension.
            //

            RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                          deviceExtension->DiskGeometry,
                          sizeof(DISK_GEOMETRY));

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        }

        break;

        }

    case IOCTL_DISK_VERIFY:

        {

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
        // Verify sectors
        //

        srb->CdbLength = 10;

        cdb->CDB10.OperationCode = SCSIOP_VERIFY;

        //
        // Add disk offset to starting sector.
        //

        byteOffset.QuadPart = deviceExtension->StartingOffset.QuadPart +
                                        verifyInfo->StartingOffset.QuadPart;

        //
        // Convert byte offset to sector offset.
        //

        sectorOffset = (ULONG)(byteOffset.QuadPart >> deviceExtension->SectorShift);

        //
        // Convert ULONG byte count to USHORT sector count.
        //

        sectorCount = (USHORT)(verifyInfo->Length >> deviceExtension->SectorShift);

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
                                              deviceExtension->TimeOutValue;

        status = ScsiClassSendSrbAsynchronous(DeviceObject,
                                              srb,
                                              Irp,
                                              NULL,
                                              0,
                                              FALSE);

        return(status);

        }

    case IOCTL_DISK_GET_PARTITION_INFO:

        //
        // Return the information about the partition specified by the device
        // object.  Note that no information is ever returned about the size
        // or partition type of the physical disk, as this doesn't make any
        // sense.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARTITION_INFORMATION)) {

            status = STATUS_INFO_LENGTH_MISMATCH;

        }
#if 0 // HACK: ReactOS partition numbers must be wrong
        else if (diskData->PartitionNumber == 0) {

            //
            // Paritition zero is not a partition so this is not a
            // reasonable request.
            //

            status = STATUS_INVALID_DEVICE_REQUEST;

        }
#endif
        else {

            PPARTITION_INFORMATION outputBuffer;

            //
            // Update the geometry in case it has changed.
            //

            status = UpdateRemovableGeometry (DeviceObject, Irp);

            if (!NT_SUCCESS(status)) {

                //
                // Note the drive is not ready.
                //

                diskData->DriveNotReady = TRUE;
                break;
            }

            //
            // Note the drive is now ready.
            //

            diskData->DriveNotReady = FALSE;
// HACK: ReactOS partition numbers must be wrong (>0 part)
            if (diskData->PartitionType == 0 && (diskData->PartitionNumber > 0)) {

                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            outputBuffer =
                    (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

            outputBuffer->PartitionType = diskData->PartitionType;
            outputBuffer->StartingOffset = deviceExtension->StartingOffset;
            outputBuffer->PartitionLength.QuadPart = (diskData->PartitionNumber) ?
                deviceExtension->PartitionLength.QuadPart : 2305843009213693951LL; // HACK
            outputBuffer->HiddenSectors = diskData->HiddenSectors;
            outputBuffer->PartitionNumber = diskData->PartitionNumber;
            outputBuffer->BootIndicator = diskData->BootIndicator;
            outputBuffer->RewritePartition = FALSE;
            outputBuffer->RecognizedPartition =
                IsRecognizedPartition(diskData->PartitionType);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
        }

        break;

    case IOCTL_DISK_SET_PARTITION_INFO:

        if (diskData->PartitionNumber == 0) {

            status = STATUS_UNSUCCESSFUL;

        } else {

            PSET_PARTITION_INFORMATION inputBuffer =
                (PSET_PARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SET_PARTITION_INFORMATION)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            //
            // The HAL routines IoGet- and IoSetPartitionInformation were
            // developed before support of dynamic partitioning and therefore
            // don't distinguish between partition ordinal (that is the order
            // of a partition on a disk) and the partition number. (The
            // partition number is assigned to a partition to identify it to
            // the system.) Use partition ordinals for these legacy calls.
            //

            status = IoSetPartitionInformation(
                          deviceExtension->PhysicalDevice,
                          deviceExtension->DiskGeometry->BytesPerSector,
                          diskData->PartitionOrdinal,
                          inputBuffer->PartitionType);

            if (NT_SUCCESS(status)) {

                diskData->PartitionType = inputBuffer->PartitionType;
            }
        }

        break;

    case IOCTL_DISK_GET_DRIVE_LAYOUT:

        //
        // Return the partition layout for the physical drive.  Note that
        // the layout is returned for the actual physical drive, regardless
        // of which partition was specified for the request.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(DRIVE_LAYOUT_INFORMATION)) {
            status = STATUS_INFO_LENGTH_MISMATCH;

        } else {

            PDRIVE_LAYOUT_INFORMATION partitionList;
            PDEVICE_EXTENSION         physicalExtension = deviceExtension;
            PPARTITION_INFORMATION    partitionEntry;
            PDISK_DATA                diskData;
            ULONG                     tempSize;
            ULONG                     i;

            //
            // Read partition information.
            //

            status = IoReadPartitionTable(deviceExtension->PhysicalDevice,
                              deviceExtension->DiskGeometry->BytesPerSector,
                              FALSE,
                              &partitionList);

            if (!NT_SUCCESS(status)) {
                break;
            }

            //
            // The disk layout has been returned in the partitionList
            // buffer.  Determine its size and, if the data will fit
            // into the intermediatery buffer, return it.
            //

            tempSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION,PartitionEntry[0]);
            tempSize += partitionList->PartitionCount *
                        sizeof(PARTITION_INFORMATION);

            if (tempSize >
               irpStack->Parameters.DeviceIoControl.OutputBufferLength) {

                status = STATUS_BUFFER_TOO_SMALL;
                ExFreePool(partitionList);
                break;
            }

            //
            // Walk partition list to associate partition numbers with
            // partition entries.
            //

            for (i = 0; i < partitionList->PartitionCount; i++) {

                //
                // Walk partition chain anchored at physical disk extension.
                //

                deviceExtension = physicalExtension;
                diskData = (PDISK_DATA)(deviceExtension + 1);

                do {

                    deviceExtension = diskData->NextPartition;

                    //
                    // Check if this is the last partition in the chain.
                    //

                    if (!deviceExtension) {
                       break;
                    }

                    //
                    // Get the partition device extension from disk data.
                    //

                    diskData = (PDISK_DATA)(deviceExtension + 1);

                    //
                    // Check if this partition is not currently being used.
                    //

                    if (!deviceExtension->PartitionLength.QuadPart) {
                       continue;
                    }

                    partitionEntry = &partitionList->PartitionEntry[i];

                    //
                    // Check if empty, or describes extended partiton or hasn't changed.
                    //

                    if (partitionEntry->PartitionType == PARTITION_ENTRY_UNUSED ||
                        IsContainerPartition(partitionEntry->PartitionType)) {
                        continue;
                    }

                    //
                    // Check if new partition starts where this partition starts.
                    //

                    if (partitionEntry->StartingOffset.QuadPart !=
                              deviceExtension->StartingOffset.QuadPart) {
                        continue;
                    }

                    //
                    // Check if partition length is the same.
                    //

                    if (partitionEntry->PartitionLength.QuadPart ==
                              deviceExtension->PartitionLength.QuadPart) {

                        //
                        // Partitions match. Update partition number.
                        //

                        partitionEntry->PartitionNumber =
                            diskData->PartitionNumber;
                        break;
                    }

                } while (TRUE);
            }

            //
            // Copy partition information to system buffer.
            //

            RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                          partitionList,
                          tempSize);
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = tempSize;

            //
            // Finally, free the buffer allocated by reading the
            // partition table.
            //

            ExFreePool(partitionList);
        }

        break;

    case IOCTL_DISK_SET_DRIVE_LAYOUT:

        {

        //
        // Update the disk with new partition information.
        //

        PDRIVE_LAYOUT_INFORMATION partitionList = Irp->AssociatedIrp.SystemBuffer;

        //
        // Validate buffer length.
        //

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(DRIVE_LAYOUT_INFORMATION)) {

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        length = sizeof(DRIVE_LAYOUT_INFORMATION) +
            (partitionList->PartitionCount - 1) * sizeof(PARTITION_INFORMATION);


        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            length) {

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Verify that device object is for physical disk.
        //

        if (deviceExtension->PhysicalDevice->DeviceExtension != deviceExtension) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Walk through partition table comparing partitions to
        // existing partitions to create, delete and change
        // device objects as necessary.
        //

        UpdateDeviceObjects(DeviceObject,
                            Irp);

        //
        // Write changes to disk.
        //

        status = IoWritePartitionTable(
                           deviceExtension->DeviceObject,
                           deviceExtension->DiskGeometry->BytesPerSector,
                           deviceExtension->DiskGeometry->SectorsPerTrack,
                           deviceExtension->DiskGeometry->TracksPerCylinder,
                           partitionList);
        }

        //
        // Update IRP with bytes returned.
        //

        if (NT_SUCCESS(status)) {
            Irp->IoStatus.Information = length;
        }

        break;

    case IOCTL_DISK_REASSIGN_BLOCKS:

        //
        // Map defective blocks to new location on disk.
        //

        {

        PREASSIGN_BLOCKS badBlocks = Irp->AssociatedIrp.SystemBuffer;
        ULONG bufferSize;
        ULONG blockNumber;
        ULONG blockCount;

        //
        // Validate buffer length.
        //

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(REASSIGN_BLOCKS)) {

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        bufferSize = sizeof(REASSIGN_BLOCKS) +
            (badBlocks->Count - 1) * sizeof(ULONG);

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            bufferSize) {

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        //
        // Build the data buffer to be transferred in the input buffer.
        // The format of the data to the device is:
        //
        //      2 bytes Reserved
        //      2 bytes Length
        //      x * 4 btyes Block Address
        //
        // All values are big endian.
        //

        badBlocks->Reserved = 0;
        blockCount = badBlocks->Count;

        //
        // Convert # of entries to # of bytes.
        //

        blockCount *= 4;
        badBlocks->Count = (USHORT) ((blockCount >> 8) & 0XFF);
        badBlocks->Count |= (USHORT) ((blockCount << 8) & 0XFF00);

        //
        // Convert back to number of entries.
        //

        blockCount /= 4;

        for (; blockCount > 0; blockCount--) {

            blockNumber = badBlocks->BlockNumber[blockCount-1];

            REVERSE_BYTES((PFOUR_BYTE) &badBlocks->BlockNumber[blockCount-1],
                          (PFOUR_BYTE) &blockNumber);
        }

        srb->CdbLength = 6;

        cdb->CDB6GENERIC.OperationCode = SCSIOP_REASSIGN_BLOCKS;

        //
        // Set timeout value.
        //

        srb->TimeOutValue = deviceExtension->TimeOutValue;

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             badBlocks,
                                             bufferSize,
                                             TRUE);

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        ExFreePool(srb);
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

        return(status);

    case IOCTL_DISK_IS_WRITABLE:

        //
        // Determine if the device is writable.
        //

        modeData = ExAllocatePool(NonPagedPoolCacheAligned, MODE_DATA_SIZE);

        if (modeData == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory(modeData, MODE_DATA_SIZE);

        length = ScsiClassModeSense(DeviceObject,
                                    (PCHAR) modeData,
                                    MODE_DATA_SIZE,
                                    MODE_SENSE_RETURN_ALL);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            //
            // Retry the request in case of a check condition.
            //

            length = ScsiClassModeSense(DeviceObject,
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

        ExFreePool(modeData);
        break;

    case IOCTL_DISK_INTERNAL_SET_VERIFY:

        //
        // If the caller is kernel mode, set the verify bit.
        //

        if (Irp->RequestorMode == KernelMode) {
            DeviceObject->Flags |= DO_VERIFY_VOLUME;
        }
        status = STATUS_SUCCESS;
        break;

    case IOCTL_DISK_INTERNAL_CLEAR_VERIFY:

        //
        // If the caller is kernel mode, clear the verify bit.
        //

        if (Irp->RequestorMode == KernelMode) {
            DeviceObject->Flags &= ~DO_VERIFY_VOLUME;
        }
        status = STATUS_SUCCESS;
        break;

    case IOCTL_DISK_FIND_NEW_DEVICES:

        //
        // Search for devices that have been powered on since the last
        // device search or system initialization.
        //

        DebugPrint((3,"CdRomDeviceControl: Find devices\n"));
        status = DriverEntry(DeviceObject->DriverObject,
                             NULL);

        Irp->IoStatus.Status = status;
        ExFreePool(srb);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

    case IOCTL_DISK_MEDIA_REMOVAL:

        //
        // If the disk is not removable then don't allow this command.
        //

        if (!(DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        //
        // Fall through and let the class driver process the request.
        //

    default:

        //
        // Free the Srb, since it is not needed.
        //

        ExFreePool(srb);

        //
        // Pass the request to the common device control routine.
        //

        return(ScsiClassDeviceControl(DeviceObject, Irp));

        break;

    } // end switch( ...

    Irp->IoStatus.Status = status;

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {

        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    ExFreePool(srb);
    return(status);

} // end ScsiDiskDeviceControl()

NTSTATUS
STDCALL
ScsiDiskShutdownFlush (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called for a shutdown and flush IRPs.  These are sent by the
    system before it actually shuts down or when the file system does a flush.
    A synchronize cache command is sent to the device if it is write caching.
    If the device is removable an unlock command will be sent. This routine
    will sent a shutdown or flush Srb to the port driver.

Arguments:

    DriverObject - Pointer to device object to being shutdown by system.

    Irp - IRP involved.

Return Value:

    NT Status

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    NTSTATUS status;
    PCDB cdb;

    //
    // Allocate SCSI request block.
    //

    srb = ExAllocatePool(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK));

    if (srb == NULL) {

        //
        // Set the status and complete the request.
        //

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

    //
    // Write length to SRB.
    //

    srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set SCSI bus address.
    //

    srb->PathId = deviceExtension->PathId;
    srb->TargetId = deviceExtension->TargetId;
    srb->Lun = deviceExtension->Lun;

    //
    // Set timeout value and mark the request as not being a tagged request.
    //

    srb->TimeOutValue = deviceExtension->TimeOutValue * 4;
    srb->QueueTag = SP_UNTAGGED;
    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
    srb->SrbFlags = deviceExtension->SrbFlags;

    //
    // If the write cache is enabled then send a synchronize cache request.
    //

    if (deviceExtension->DeviceFlags & DEV_WRITE_CACHE) {

        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->CdbLength = 10;

        srb->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             NULL,
                                             0,
                                             TRUE);

        DebugPrint((1, "ScsiDiskShutdownFlush: Synchonize cache sent. Status = %lx\n", status ));
    }

    //
    // Unlock the device if it is removable and this is a shutdown.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA &&
        irpStack->MajorFunction == IRP_MJ_SHUTDOWN) {

        srb->CdbLength = 6;
        cdb = (PVOID) srb->Cdb;
        cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
        cdb->MEDIA_REMOVAL.Prevent = FALSE;

        //
        // Set timeout value.
        //

        srb->TimeOutValue = deviceExtension->TimeOutValue;
        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             NULL,
                                             0,
                                             TRUE);

        DebugPrint((1, "ScsiDiskShutdownFlush: Unlock device request sent. Status = %lx\n", status ));
    }

    srb->CdbLength = 0;

    //
    // Save a few parameters in the current stack location.
    //

    srb->Function = irpStack->MajorFunction == IRP_MJ_SHUTDOWN ?
        SRB_FUNCTION_SHUTDOWN : SRB_FUNCTION_FLUSH;

    //
    // Set the retry count to zero.
    //

    irpStack->Parameters.Others.Argument4 = (PVOID) 0;

    //
    // Set up IoCompletion routine address.
    //

    IoSetCompletionRoutine(Irp, ScsiClassIoComplete, srb, TRUE, TRUE, TRUE);

    //
    // Get next stack location and
    // set major function code.
    //

    irpStack = IoGetNextIrpStackLocation(Irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Set up SRB for execute scsi request.
    // Save SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = srb;

    //
    // Set up Irp Address.
    //

    srb->OriginalRequest = Irp;

    //
    // Call the port driver to process the request.
    //

    return(IoCallDriver(deviceExtension->PortDeviceObject, Irp));

} // end ScsiDiskShutdown()


BOOLEAN
STDCALL
IsFloppyDevice(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    The routine performs the necessary functions to determine if a device is
    really a floppy rather than a harddisk.  This is done by a mode sense
    command.  First, a check is made to see if the medimum type is set.  Second
    a check is made for the flexible parameters mode page.  Also a check is
    made to see if the write cache is enabled.

Arguments:

    DeviceObject - Supplies the device object to be tested.

Return Value:

    Return TRUE if the indicated device is a floppy.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PVOID modeData;
    PUCHAR pageData;
    ULONG length;

    PAGED_CODE();

    modeData = ExAllocatePool(NonPagedPoolCacheAligned, MODE_DATA_SIZE);

    if (modeData == NULL) {
        return(FALSE);
    }

    RtlZeroMemory(modeData, MODE_DATA_SIZE);

    length = ScsiClassModeSense(DeviceObject,
                                modeData,
                                MODE_DATA_SIZE,
                                MODE_SENSE_RETURN_ALL);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        length = ScsiClassModeSense(DeviceObject,
                                modeData,
                                MODE_DATA_SIZE,
                                MODE_SENSE_RETURN_ALL);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            ExFreePool(modeData);
            return(FALSE);

        }
    }

    //
    // If the length is greater than length indicated by the mode data reset
    // the data to the mode data.
    //

    if (length > (ULONG) ((PMODE_PARAMETER_HEADER) modeData)->ModeDataLength + 1) {
        length = ((PMODE_PARAMETER_HEADER) modeData)->ModeDataLength + 1;
    }

    //
    // Look for the flexible disk mode page.
    //

    pageData = ScsiClassFindModePage( modeData, length, MODE_PAGE_FLEXIBILE, TRUE);

    if (pageData != NULL) {

        DebugPrint((1, "Scsidisk: Flexible disk page found, This is a floppy.\n"));
        ExFreePool(modeData);
        return(TRUE);
    }

    //
    // Check to see if the write cache is enabled.
    //

    pageData = ScsiClassFindModePage( modeData, length, MODE_PAGE_CACHING, TRUE);

    //
    // Assume that write cache is disabled or not supported.
    //

    deviceExtension->DeviceFlags &= ~DEV_WRITE_CACHE;

    //
    // Check if valid caching page exists.
    //

    if (pageData != NULL) {

        //
        // Check if write cache is disabled.
        //

        if (((PMODE_CACHING_PAGE)pageData)->WriteCacheEnable) {

            DebugPrint((1,
                       "SCSIDISK: Disk write cache enabled\n"));

            //
            // Check if forced unit access (FUA) is supported.
            //

            if (((PMODE_PARAMETER_HEADER)modeData)->DeviceSpecificParameter & MODE_DSP_FUA_SUPPORTED) {

                deviceExtension->DeviceFlags |= DEV_WRITE_CACHE;

            } else {

                DebugPrint((1,
                           "SCSIDISK: Disk does not support FUA or DPO\n"));

                //
                // TODO: Log this.
                //

            }
        }
    }

    ExFreePool(modeData);
    return(FALSE);

} // end IsFloppyDevice()


BOOLEAN
STDCALL
ScsiDiskModeSelect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCHAR ModeSelectBuffer,
    IN ULONG Length,
    IN BOOLEAN SavePage
    )

/*++

Routine Description:

    This routine sends a mode select command.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    ModeSelectBuffer - Supplies a buffer containing the page data.

    Length - Supplies the length in bytes of the mode select buffer.

    SavePage - Indicates that parameters should be written to disk.

Return Value:

    Length of the transferred data is returned.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCDB cdb;
    SCSI_REQUEST_BLOCK srb;
    ULONG retries = 1;
    ULONG length2;
    NTSTATUS status;
    ULONG_PTR buffer;
    PMODE_PARAMETER_BLOCK blockDescriptor;

    PAGED_CODE();

    length2 = Length + sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_PARAMETER_BLOCK);

    //
    // Allocate buffer for mode select header, block descriptor, and mode page.
    //

    buffer = (ULONG_PTR)ExAllocatePool(NonPagedPoolCacheAligned,length2);

    RtlZeroMemory((PVOID)buffer, length2);

    //
    // Set length in header to size of mode page.
    //

    ((PMODE_PARAMETER_HEADER)buffer)->BlockDescriptorLength = sizeof(MODE_PARAMETER_BLOCK);

    blockDescriptor = (PMODE_PARAMETER_BLOCK)(buffer + 1);

    //
    // Set size
    //

    blockDescriptor->BlockLength[1]=0x02;

    //
    // Copy mode page to buffer.
    //

    RtlCopyMemory((PVOID)(buffer + 3), ModeSelectBuffer, Length);

    //
    // Zero SRB.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Build the MODE SELECT CDB.
    //

    srb.CdbLength = 6;
    cdb = (PCDB)srb.Cdb;

    //
    // Set timeout value from device extension.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue * 2;

    cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
    cdb->MODE_SELECT.SPBit = SavePage;
    cdb->MODE_SELECT.PFBit = 1;
    cdb->MODE_SELECT.ParameterListLength = (UCHAR)(length2);

Retry:

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         (PVOID)buffer,
                                         length2,
                                         TRUE);


    if (status == STATUS_VERIFY_REQUIRED) {

        //
        // Routine ScsiClassSendSrbSynchronous does not retry requests returned with
        // this status.
        //

        if (retries--) {

            //
            // Retry request.
            //

            goto Retry;
        }

    } else if (SRB_STATUS(srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN) {
        status = STATUS_SUCCESS;
    }

    ExFreePool((PVOID)buffer);

    if (NT_SUCCESS(status)) {
        return(TRUE);
    } else {
        return(FALSE);
    }

} // end SciDiskModeSelect()


VOID
STDCALL
DisableWriteCache(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_INQUIRY_DATA LunInfo
    )

{
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PINQUIRYDATA               InquiryData     = (PINQUIRYDATA)LunInfo->InquiryData;
    BAD_CONTROLLER_INFORMATION const *controller;
    ULONG                      j,length;
    PVOID                      modeData;
    PUCHAR                     pageData;

    for (j = 0; j <  NUMBER_OF_BAD_CONTROLLERS; j++) {

        controller = &ScsiDiskBadControllers[j];

        if (!controller->DisableWriteCache || strncmp(controller->InquiryString, (PCCHAR)InquiryData->VendorId, strlen(controller->InquiryString))) {
            continue;
        }

        DebugPrint((1, "ScsiDisk.DisableWriteCache, Found bad controller! %s\n", controller->InquiryString));

        modeData = ExAllocatePool(NonPagedPoolCacheAligned, MODE_DATA_SIZE);

        if (modeData == NULL) {

            DebugPrint((1,
                        "ScsiDisk.DisableWriteCache: Check for write-cache enable failed\n"));
            return;
        }

        RtlZeroMemory(modeData, MODE_DATA_SIZE);

        length = ScsiClassModeSense(DeviceObject,
                                    modeData,
                                    MODE_DATA_SIZE,
                                    MODE_SENSE_RETURN_ALL);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            //
            // Retry the request in case of a check condition.
            //

            length = ScsiClassModeSense(DeviceObject,
                                    modeData,
                                    MODE_DATA_SIZE,
                                    MODE_SENSE_RETURN_ALL);

            if (length < sizeof(MODE_PARAMETER_HEADER)) {


                DebugPrint((1,
                            "ScsiDisk.DisableWriteCache: Mode Sense failed\n"));

                ExFreePool(modeData);
                return;

            }
        }

        //
        // If the length is greater than length indicated by the mode data reset
        // the data to the mode data.
        //

        if (length > (ULONG) ((PMODE_PARAMETER_HEADER) modeData)->ModeDataLength + 1) {
            length = ((PMODE_PARAMETER_HEADER) modeData)->ModeDataLength + 1;
        }

        //
        // Check to see if the write cache is enabled.
        //

        pageData = ScsiClassFindModePage( modeData, length, MODE_PAGE_CACHING, TRUE);

        //
        // Assume that write cache is disabled or not supported.
        //

        deviceExtension->DeviceFlags &= ~DEV_WRITE_CACHE;

        //
        // Check if valid caching page exists.
        //

        if (pageData != NULL) {

            BOOLEAN savePage = FALSE;

            savePage = (BOOLEAN)(((PMODE_CACHING_PAGE)pageData)->PageSavable);

            //
            // Check if write cache is disabled.
            //

            if (((PMODE_CACHING_PAGE)pageData)->WriteCacheEnable) {

                PIO_ERROR_LOG_PACKET errorLogEntry;
                LONG                 errorCode;


                //
                // Disable write cache and ensure necessary fields are zeroed.
                //

                ((PMODE_CACHING_PAGE)pageData)->WriteCacheEnable = FALSE;
                ((PMODE_CACHING_PAGE)pageData)->Reserved = 0;
                ((PMODE_CACHING_PAGE)pageData)->PageSavable = 0;
                ((PMODE_CACHING_PAGE)pageData)->Reserved2 = 0;

                //
                // Extract length from caching page.
                //

                length = ((PMODE_CACHING_PAGE)pageData)->PageLength;

                //
                // Compensate for page code and page length.
                //

                length += 2;

                //
                // Issue mode select to set the parameter.
                //

                if (ScsiDiskModeSelect(DeviceObject,
                                       (PCHAR)pageData,
                                       length,
                                       savePage)) {

                    DebugPrint((1,
                               "SCSIDISK: Disk write cache disabled\n"));

                    deviceExtension->DeviceFlags &= ~DEV_WRITE_CACHE;
                    errorCode = IO_WRITE_CACHE_DISABLED;

                } else {
                    if (ScsiDiskModeSelect(DeviceObject,
                                           (PCHAR)pageData,
                                           length,
                                           savePage)) {

                        DebugPrint((1,
                                   "SCSIDISK: Disk write cache disabled\n"));


                        deviceExtension->DeviceFlags &= ~DEV_WRITE_CACHE;
                        errorCode = IO_WRITE_CACHE_DISABLED;

                    } else {

                            DebugPrint((1,
                                       "SCSIDISK: Mode select to disable write cache failed\n"));

                            deviceExtension->DeviceFlags |= DEV_WRITE_CACHE;
                            errorCode = IO_WRITE_CACHE_ENABLED;
                    }
                }

                //
                // Log the appropriate informational or error entry.
                //

                errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                                         DeviceObject,
                                                         sizeof(IO_ERROR_LOG_PACKET) + 3
                                                             * sizeof(ULONG));

                if (errorLogEntry != NULL) {

                    errorLogEntry->FinalStatus     = STATUS_SUCCESS;
                    errorLogEntry->ErrorCode       = errorCode;
                    errorLogEntry->SequenceNumber  = 0;
                    errorLogEntry->MajorFunctionCode = IRP_MJ_SCSI;
                    errorLogEntry->IoControlCode   = 0;
                    errorLogEntry->RetryCount      = 0;
                    errorLogEntry->UniqueErrorValue = 0x1;
                    errorLogEntry->DumpDataSize    = 3 * sizeof(ULONG);
                    errorLogEntry->DumpData[0]     = LunInfo->PathId;
                    errorLogEntry->DumpData[1]     = LunInfo->TargetId;
                    errorLogEntry->DumpData[2]     = LunInfo->Lun;

                    //
                    // Write the error log packet.
                    //

                    IoWriteErrorLogEntry(errorLogEntry);
                }
            }
        }

        //
        // Found device so exit the loop and return.
        //

        break;
    }

    return;
}


BOOLEAN
STDCALL
CalculateMbrCheckSum(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PULONG Checksum
    )

/*++

Routine Description:

    Read MBR and calculate checksum.

Arguments:

    DeviceExtension - Supplies a pointer to the device information for disk.
    Checksum - Memory location to return MBR checksum.

Return Value:

    Returns TRUE if checksum is valid.

--*/
{
    LARGE_INTEGER   sectorZero;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT          event;
    NTSTATUS        status;
    ULONG           sectorSize;
    PULONG          mbr;
    ULONG           i;

    PAGED_CODE();
    sectorZero.QuadPart = (LONGLONG) 0;

    //
    // Create notification event object to be used to signal the inquiry
    // request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Get sector size.
    //

    sectorSize = DeviceExtension->DiskGeometry->BytesPerSector;

    //
    // Make sure sector size is at least 512 bytes.
    //

    if (sectorSize < 512) {
        sectorSize = 512;
    }

    //
    // Allocate buffer for sector read.
    //

    mbr = ExAllocatePool(NonPagedPoolCacheAligned, sectorSize);

    if (!mbr) {
        return FALSE;
    }

    //
    // Build IRP to read MBR.
    //

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceExtension->DeviceObject,
                                       mbr,
                                       sectorSize,
                                       &sectorZero,
                                       &event,
                                       &ioStatus );

    if (!irp) {
        ExFreePool(mbr);
        return FALSE;
    }

    //
    // Pass request to port driver and wait for request to complete.
    //

    status = IoCallDriver(DeviceExtension->DeviceObject,
                          irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(mbr);
        return FALSE;
    }

    //
    // Calculate MBR checksum.
    //

    *Checksum = 0;

    for (i = 0; i < 128; i++) {
        *Checksum += mbr[i];
    }

    *Checksum = ~*Checksum + 1;

    ExFreePool(mbr);
    return TRUE;
}


BOOLEAN
STDCALL
EnumerateBusKey(
    IN PDEVICE_EXTENSION DeviceExtension,
    HANDLE BusKey,
    PULONG DiskNumber
    )

/*++

Routine Description:

    The routine queries the registry to determine if this disk is visible to
    the BIOS.  If the disk is visable to the BIOS, then the geometry information
    is updated.

Arguments:

    DeviceExtension - Supplies a pointer to the device information for disk.
    Signature - Unique identifier recorded in MBR.
    BusKey - Handle of bus key.
    DiskNumber - Returns ordinal of disk as BIOS sees it.

Return Value:

    TRUE is disk signature matched.

--*/
{
    PDISK_DATA        diskData = (PDISK_DATA)(DeviceExtension + 1);
    BOOLEAN           diskFound = FALSE;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING    unicodeString;
    UNICODE_STRING    identifier;
    ULONG             busNumber;
    ULONG             adapterNumber;
    ULONG             diskNumber;
    HANDLE            adapterKey;
    HANDLE            spareKey;
    HANDLE            diskKey;
    HANDLE            targetKey;
    NTSTATUS          status;
    STRING            string;
    STRING            anotherString;
    ULONG             length;
    UCHAR             buffer[20];
    PKEY_VALUE_FULL_INFORMATION keyData;

    PAGED_CODE();

    for (busNumber = 0; ; busNumber++) {

        //
        // Open controller name key.
        //

        sprintf((PCHAR)buffer,
                "%lu",
                busNumber);

        RtlInitString(&string,
                      (PCSZ)buffer);

        status = RtlAnsiStringToUnicodeString(&unicodeString,
                                              &string,
                                              TRUE);

        if (!NT_SUCCESS(status)){
            break;
        }

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   BusKey,
                                   (PSECURITY_DESCRIPTOR)NULL);

        status = ZwOpenKey(&spareKey,
                           KEY_READ,
                           &objectAttributes);

        RtlFreeUnicodeString(&unicodeString);

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Open up controller ordinal key.
        //

        RtlInitUnicodeString(&unicodeString, L"DiskController");
        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   spareKey,
                                   (PSECURITY_DESCRIPTOR)NULL);

        status = ZwOpenKey(&adapterKey,
                           KEY_READ,
                           &objectAttributes);

        //
        // This could fail even with additional adapters of this type
        // to search.
        //

        if (!NT_SUCCESS(status)) {
            continue;
        }

        for (adapterNumber = 0; ; adapterNumber++) {

            //
            // Open disk key.
            //

            sprintf((PCHAR)buffer,
                    "%lu\\DiskPeripheral",
                    adapterNumber);

            RtlInitString(&string,
                          (PCSZ)buffer);

            status = RtlAnsiStringToUnicodeString(&unicodeString,
                                                  &string,
                                                  TRUE);

            if (!NT_SUCCESS(status)){
                break;
            }

            InitializeObjectAttributes(&objectAttributes,
                                       &unicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       adapterKey,
                                       (PSECURITY_DESCRIPTOR)NULL);

            status = ZwOpenKey(&diskKey,
                               KEY_READ,
                               &objectAttributes);

            RtlFreeUnicodeString(&unicodeString);

            if (!NT_SUCCESS(status)) {
                break;
            }

            for (diskNumber = 0; ; diskNumber++) {

                sprintf((PCHAR)buffer,
                        "%lu",
                        diskNumber);

                RtlInitString(&string,
                              (PCSZ)buffer);

                status = RtlAnsiStringToUnicodeString(&unicodeString,
                                                      &string,
                                                      TRUE);

                if (!NT_SUCCESS(status)){
                    break;
                }

                InitializeObjectAttributes(&objectAttributes,
                                           &unicodeString,
                                           OBJ_CASE_INSENSITIVE,
                                           diskKey,
                                           (PSECURITY_DESCRIPTOR)NULL);

                status = ZwOpenKey(&targetKey,
                                   KEY_READ,
                                   &objectAttributes);

                RtlFreeUnicodeString(&unicodeString);

                if (!NT_SUCCESS(status)) {
                    break;
                }

                //
                // Allocate buffer for registry query.
                //

                keyData = ExAllocatePool(PagedPool, VALUE_BUFFER_SIZE);

                if (keyData == NULL) {
                    ZwClose(targetKey);
                    continue;
                }

                //
                // Get disk peripheral identifier.
                //

                RtlInitUnicodeString(&unicodeString, L"Identifier");
                status = ZwQueryValueKey(targetKey,
                                         &unicodeString,
                                         KeyValueFullInformation,
                                         keyData,
                                         VALUE_BUFFER_SIZE,
                                         &length);

                ZwClose(targetKey);

                if (!NT_SUCCESS(status)) {
                    continue;
                }

                //
                // Complete unicode string.
                //

                identifier.Buffer =
                    (PWSTR)((PUCHAR)keyData + keyData->DataOffset);
                identifier.Length = (USHORT)keyData->DataLength;
                identifier.MaximumLength = (USHORT)keyData->DataLength;

                //
                // Convert unicode identifier to ansi string.
                //

                status =
                    RtlUnicodeStringToAnsiString(&anotherString,
                                                 &identifier,
                                                 TRUE);

                if (!NT_SUCCESS(status)) {
                    continue;
                }

                //
                // If checksum is zero, then the MBR is valid and
                // the signature is meaningful.
                //

                if (diskData->MbrCheckSum) {

                    //
                    // Convert checksum to ansi string.
                    //

                    sprintf((PCHAR)buffer, "%08lx", diskData->MbrCheckSum);

                } else {

                    //
                    // Convert signature to ansi string.
                    //

                    sprintf((PCHAR)buffer, "%08lx", diskData->Signature);

                    //
                    // Make string point at signature. Can't use scan
                    // functions because they are not exported for driver use.
                    //

                    anotherString.Buffer+=9;
                }

                //
                // Convert to ansi string.
                //

                RtlInitString(&string,
                              (PCSZ)buffer);


                //
                // Make string lengths equal.
                //

                anotherString.Length = string.Length;

                //
                // Check if strings match.
                //

                if (RtlCompareString(&string,
                                     &anotherString,
                                     TRUE) == 0)  {

                    diskFound = TRUE;
                    *DiskNumber = diskNumber;
                }

                ExFreePool(keyData);

                //
                // Readjust indentifier string if necessary.
                //

                if (!diskData->MbrCheckSum) {
                    anotherString.Buffer-=9;
                }

                RtlFreeAnsiString(&anotherString);

                if (diskFound) {
                    break;
                }
            }

            ZwClose(diskKey);
        }

        ZwClose(adapterKey);
    }

    ZwClose(BusKey);
    return diskFound;

} // end EnumerateBusKey()


VOID
STDCALL
UpdateGeometry(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    The routine queries the registry to determine if this disk is visible to
    the BIOS.  If the disk is visable to the BIOS, then the geometry information
    is updated.

Arguments:

    DeviceExtension - Supplies a pointer to the device information for disk.

Return Value:

    None.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    HANDLE hardwareKey;
    HANDLE busKey;
    PCM_INT13_DRIVE_PARAMETER driveParameters;
    PCM_FULL_RESOURCE_DESCRIPTOR resourceDescriptor;
    PKEY_VALUE_FULL_INFORMATION keyData;
    ULONG diskNumber;
    PUCHAR buffer;
    ULONG length;
    ULONG numberOfDrives;
    ULONG cylinders;
    ULONG sectors;
    ULONG sectorsPerTrack;
    ULONG tracksPerCylinder;
    BOOLEAN foundEZHooker;
    PVOID tmpPtr;

    PAGED_CODE();

    //
    // Initialize the object for the key.
    //

    InitializeObjectAttributes(&objectAttributes,
                               DeviceExtension->DeviceObject->DriverObject->HardwareDatabase,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    //
    // Create the hardware base key.
    //

    status =  ZwOpenKey(&hardwareKey,
                        KEY_READ,
                        &objectAttributes);


    if (!NT_SUCCESS(status)) {
        DebugPrint((1, "ScsiDisk UpdateParameters: Cannot open hardware data. Name: %wZ\n", DeviceExtension->DeviceObject->DriverObject->HardwareDatabase));
        return;
    }


    //
    // Get disk BIOS geometry information.
    //

    RtlInitUnicodeString(&unicodeString, L"Configuration Data");

    keyData = ExAllocatePool(PagedPool, VALUE_BUFFER_SIZE);

    if (keyData == NULL) {
        ZwClose(hardwareKey);
        return;
    }

    status = ZwQueryValueKey(hardwareKey,
                             &unicodeString,
                             KeyValueFullInformation,
                             keyData,
                             VALUE_BUFFER_SIZE,
                             &length);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,
                   "SCSIDISK: ExtractBiosGeometry: Can't query configuration data (%x)\n",
                   status));
        ExFreePool(keyData);
        return;
    }

    //
    // Open EISA bus key.
    //

    RtlInitUnicodeString(&unicodeString, L"EisaAdapter");

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               hardwareKey,
                               (PSECURITY_DESCRIPTOR)NULL);

    status = ZwOpenKey(&busKey,
                       KEY_READ,
                       &objectAttributes);

    if (!NT_SUCCESS(status)) {
        goto openMultiKey;
    }

    DebugPrint((3,
               "SCSIDISK: UpdateGeometry: Opened EisaAdapter key\n"));
    if (EnumerateBusKey(DeviceExtension,
                        busKey,
                        &diskNumber)) {

        ZwClose(hardwareKey);
        goto diskMatched;
    }

openMultiKey:

    //
    // Open Multifunction bus key.
    //

    RtlInitUnicodeString(&unicodeString, L"MultifunctionAdapter");

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               hardwareKey,
                               (PSECURITY_DESCRIPTOR)NULL);

    status = ZwOpenKey(&busKey,
                       KEY_READ,
                       &objectAttributes);

    ZwClose(hardwareKey);
    if (NT_SUCCESS(status)) {
        DebugPrint((3,
                   "SCSIDISK: UpdateGeometry: Opened MultifunctionAdapter key\n"));
        if (EnumerateBusKey(DeviceExtension,
                            busKey,
                            &diskNumber)) {

            goto diskMatched;
        }
    }

    ExFreePool(keyData);
    return;

diskMatched:

    resourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PUCHAR)keyData +
        keyData->DataOffset);

    //
    // Check that the data is long enough to hold a full resource descriptor,
    // and that the last resouce list is device-specific and long enough.
    //

    if (keyData->DataLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR) ||
        resourceDescriptor->PartialResourceList.Count == 0 ||
        resourceDescriptor->PartialResourceList.PartialDescriptors[0].Type !=
        CmResourceTypeDeviceSpecific ||
        resourceDescriptor->PartialResourceList.PartialDescriptors[0]
            .u.DeviceSpecificData.DataSize < sizeof(ULONG)) {

        DebugPrint((1, "SCSIDISK: ExtractBiosGeometry: BIOS header data too small or invalid\n"));
        ExFreePool(keyData);
        return;
    }

    length =
        resourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize;

    //
    // Point to the BIOS data. The BIOS data is located after the first
    // partial Resource list which should be device specific data.
    //

    buffer = (PUCHAR) keyData + keyData->DataOffset +
        sizeof(CM_FULL_RESOURCE_DESCRIPTOR);


    numberOfDrives = length / sizeof(CM_INT13_DRIVE_PARAMETER);

    //
    // Use the defaults if the drive number is greater than the
    // number of drives detected by the BIOS.
    //

    if (numberOfDrives <= diskNumber) {
        ExFreePool(keyData);
        return;
    }

    //
    // Point to the array of drive parameters.
    //

    driveParameters = (PCM_INT13_DRIVE_PARAMETER) buffer + diskNumber;
    cylinders = driveParameters->MaxCylinders + 1;
    sectorsPerTrack = driveParameters->SectorsPerTrack;
    tracksPerCylinder = driveParameters->MaxHeads +1;

    //
    // Calculate the actual number of sectors.
    //

    sectors = (ULONG)(DeviceExtension->PartitionLength.QuadPart >>
                                     DeviceExtension->SectorShift);

#if DBG
    if (sectors >= cylinders * tracksPerCylinder * sectorsPerTrack) {
        DebugPrint((1, "ScsiDisk: UpdateGeometry: Disk smaller than BIOS indicated\n"
            "SCSIDISK: Sectors: %x, Cylinders: %x, Track per Cylinder: %x Sectors per track: %x\n",
            sectors, cylinders, tracksPerCylinder, sectorsPerTrack));
    }
#endif

    //
    // Since the BIOS may not report the full drive, recalculate the drive
    // size based on the volume size and the BIOS values for tracks per
    // cylinder and sectors per track..
    //

    length = tracksPerCylinder * sectorsPerTrack;

    if (length == 0) {

        //
        // The BIOS information is bogus.
        //

        DebugPrint((1, "ScsiDisk UpdateParameters: sectorPerTrack zero\n"));
        ExFreePool(keyData);
        return;
    }

    cylinders = sectors / length;

    //
    // Update the actual geometry information.
    //

    DeviceExtension->DiskGeometry->SectorsPerTrack = sectorsPerTrack;
    DeviceExtension->DiskGeometry->TracksPerCylinder = tracksPerCylinder;
    DeviceExtension->DiskGeometry->Cylinders.QuadPart = (LONGLONG)cylinders;

    DebugPrint((3,
               "SCSIDISK: UpdateGeometry: BIOS spt %x, #heads %x, #cylinders %x\n",
               sectorsPerTrack,
               tracksPerCylinder,
               cylinders));

    ExFreePool(keyData);

    foundEZHooker = FALSE;

    if (!DeviceExtension->DMActive) {

        HalExamineMBR(DeviceExtension->DeviceObject,
                      DeviceExtension->DiskGeometry->BytesPerSector,
                      (ULONG)0x55,
                      &tmpPtr
                      );

        if (tmpPtr) {

            ExFreePool(tmpPtr);
            foundEZHooker = TRUE;

        }

    }

    if (DeviceExtension->DMActive || foundEZHooker) {

        while (cylinders > 1024) {

            tracksPerCylinder = tracksPerCylinder*2;
            cylinders = cylinders/2;

        }

        //
        // int 13 values are always 1 less.
        //

        tracksPerCylinder -= 1;
        cylinders -= 1;

        //
        // DM reserves the CE cylinder
        //

        cylinders -= 1;

        DeviceExtension->DiskGeometry->Cylinders.QuadPart = cylinders + 1;
        DeviceExtension->DiskGeometry->TracksPerCylinder = tracksPerCylinder + 1;

        DeviceExtension->PartitionLength.QuadPart =
            DeviceExtension->DiskGeometry->Cylinders.QuadPart *
                DeviceExtension->DiskGeometry->SectorsPerTrack *
                DeviceExtension->DiskGeometry->BytesPerSector *
                DeviceExtension->DiskGeometry->TracksPerCylinder;

        if (DeviceExtension->DMActive) {

            DeviceExtension->DMByteSkew = DeviceExtension->DMSkew * DeviceExtension->DiskGeometry->BytesPerSector;

        }

    } else {

        DeviceExtension->DMByteSkew = 0;

    }

    return;

} // end UpdateGeometry()



NTSTATUS
STDCALL
UpdateRemovableGeometry (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routines updates the size and starting offset of the device.  This is
    used when the media on the device may have changed thereby changing the
    size of the device.  If this is the physical device then a
    ScsiClassReadDriveCapacity is done; otherewise, a read partition table is done.

Arguments:

    DeviceObject - Supplies the device object whos size needs to be updated.

    Irp - Supplies a reference where the status can be updated.

Return Value:

    Returns the status of the opertion.

--*/
{

    PDEVICE_EXTENSION         deviceExtension = DeviceObject->DeviceExtension;
    PDRIVE_LAYOUT_INFORMATION partitionList;
    NTSTATUS                  status;
    PDISK_DATA                diskData;
    ULONG                     partitionNumber;

    //
    // Determine if the size of the partition may have changed because
    // the media has changed.
    //

    if (!(DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)) {

        return(STATUS_SUCCESS);

    }

    //
    // If this request is for partition zero then do a read drive
    // capacity otherwise do a I/O read partition table.
    //

    diskData = (PDISK_DATA) (deviceExtension + 1);

    //
    // Read the drive capcity.  If that fails, give up.
    //

    status = ScsiClassReadDriveCapacity(deviceExtension->PhysicalDevice);

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    //
    // Read the partition table agian.
    //

    status = IoReadPartitionTable(deviceExtension->PhysicalDevice,
                      deviceExtension->DiskGeometry->BytesPerSector,
                      TRUE,
                      &partitionList);


    if (!NT_SUCCESS(status)) {

        //
        // Fail the request.
        //

        return(status);
    }

    if (diskData->PartitionNumber != 0 &&
        diskData->PartitionNumber <= partitionList->PartitionCount ) {

        partitionNumber = diskData->PartitionNumber - 1;

        //
        // Update the partition information for this parition.
        //

        diskData->PartitionType =
            partitionList->PartitionEntry[partitionNumber].PartitionType;

        diskData->BootIndicator =
            partitionList->PartitionEntry[partitionNumber].BootIndicator;

        deviceExtension->StartingOffset =
            partitionList->PartitionEntry[partitionNumber].StartingOffset;

        deviceExtension->PartitionLength =
            partitionList->PartitionEntry[partitionNumber].PartitionLength;

        diskData->HiddenSectors =
            partitionList->PartitionEntry[partitionNumber].HiddenSectors;

        deviceExtension->SectorShift = ((PDEVICE_EXTENSION)
            deviceExtension->PhysicalDevice->DeviceExtension)->SectorShift;

    } else if (diskData->PartitionNumber != 0) {

        //
        // The paritition does not exist.  Zero all the data.
        //

        diskData->PartitionType = 0;
        diskData->BootIndicator = 0;
        diskData->HiddenSectors = 0;
        deviceExtension->StartingOffset.QuadPart  = (LONGLONG)0;
        deviceExtension->PartitionLength.QuadPart = (LONGLONG)0;
    }

    //
    // Free the parition list allocate by I/O read partition table.
    //

    ExFreePool(partitionList);


    return(STATUS_SUCCESS);
}


VOID
STDCALL
ScsiDiskProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )
/*++

Routine Description:

   This routine checks the type of error.  If the error indicates an underrun
   then indicate the request should be retried.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - Status with which the IRP will be completed.

    Retry - Indication of whether the request will be retried.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    if (*Status == STATUS_DATA_OVERRUN &&
        ( Srb->Cdb[0] == SCSIOP_WRITE || Srb->Cdb[0] == SCSIOP_READ)) {

            *Retry = TRUE;

            //
            // Update the error count for the device.
            //

            deviceExtension->ErrorCount++;
    }

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_ERROR &&
        Srb->ScsiStatus == SCSISTAT_BUSY) {

        //
        // The disk drive should never be busy this long. Reset the scsi bus
        // maybe this will clear the condition.
        //

        ResetScsiBus(DeviceObject);

        //
        // Update the error count for the device.
        //

        deviceExtension->ErrorCount++;
    }
}

VOID
STDCALL
ScanForSpecial(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_INQUIRY_DATA LunInfo,
    PIO_SCSI_CAPABILITIES PortCapabilities
    )

/*++

Routine Description:

    This function checks to see if an SCSI logical unit requires speical
    flags to be set.

Arguments:

    DeviceObject - Supplies the device object to be tested.

    InquiryData - Supplies the inquiry data returned by the device of interest.

    PortCapabilities - Supplies the capabilities of the device object.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PINQUIRYDATA               InquiryData     = (PINQUIRYDATA)LunInfo->InquiryData;
    BAD_CONTROLLER_INFORMATION const *controller;
    ULONG                      j;

    for (j = 0; j <  NUMBER_OF_BAD_CONTROLLERS; j++) {

        controller = &ScsiDiskBadControllers[j];

        if (strncmp(controller->InquiryString, (PCCHAR)InquiryData->VendorId, strlen(controller->InquiryString))) {
            continue;
        }

        DebugPrint((1, "ScsiDisk ScanForSpecial, Found bad controller! %s\n", controller->InquiryString));

        //
        // Found a listed controller.  Determine what must be done.
        //

        if (controller->DisableTaggedQueuing) {

            //
            // Disable tagged queuing.
            //

            deviceExtension->SrbFlags &= ~SRB_FLAGS_QUEUE_ACTION_ENABLE;
        }

        if (controller->DisableSynchronousTransfers) {

            //
            // Disable synchronous data transfers.
            //

            deviceExtension->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

        }

        if (controller->DisableDisconnects) {

            //
            // Disable disconnects.
            //

            deviceExtension->SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT;

        }

        //
        // Found device so exit the loop and return.
        //

        break;
    }

    //
    // Set the StartUnit flag appropriately.
    //

    if (DeviceObject->DeviceType == FILE_DEVICE_DISK) {
        deviceExtension->DeviceFlags |= DEV_SAFE_START_UNIT;

        if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {
            if (_strnicmp((PCCHAR)InquiryData->VendorId, "iomega", strlen("iomega"))) {
                deviceExtension->DeviceFlags &= ~DEV_SAFE_START_UNIT;
            }
        }
    }

    return;
}

VOID
STDCALL
ResetScsiBus(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This command sends a reset bus command to the SCSI port driver.

Arguments:

    DeviceObject - The device object for the logical unit with
        hardware problem.

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCOMPLETION_CONTEXT context;

    DebugPrint((1, "ScsiDisk ResetScsiBus: Sending reset bus request to port driver.\n"));

    //
    // Allocate Srb from nonpaged pool.
    //

    context = ExAllocatePool(NonPagedPoolMustSucceed,
                             sizeof(COMPLETION_CONTEXT));

    //
    // Save the device object in the context for use by the completion
    // routine.
    //

    context->DeviceObject = DeviceObject;
    srb = &context->Srb;

    //
    // Zero out srb.
    //

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

    srb->Function = SRB_FUNCTION_RESET_BUS;

    //
    // Build the asynchronous request to be sent to the port driver.
    // Since this routine is called from a DPC the IRP should always be
    // available.
    //

    irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

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
    // Store the SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = srb;

    //
    // Call the port driver with the IRP.
    //

    IoCallDriver(deviceExtension->PortDeviceObject, irp);

    return;

} // end ResetScsiBus()


VOID
STDCALL
UpdateDeviceObjects(
    IN PDEVICE_OBJECT PhysicalDisk,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine creates, deletes and changes device objects when
    the IOCTL_SET_DRIVE_LAYOUT is called.  This routine also updates
    the drive layout information for the user.  It is possible to
    call this routine even in the GET_LAYOUT case because RewritePartition
    will be false.

Arguments:

    DeviceObject - Device object for physical disk.
    Irp - IO Request Packet (IRP).

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION         physicalExtension = PhysicalDisk->DeviceExtension;
    PDRIVE_LAYOUT_INFORMATION partitionList = Irp->AssociatedIrp.SystemBuffer;
    ULONG                     partition;
    ULONG                     partitionNumber;
    ULONG                     partitionCount;
    ULONG                     lastPartition;
    ULONG                     partitionOrdinal;
    PPARTITION_INFORMATION    partitionEntry;
    CCHAR                     ntNameBuffer[MAXIMUM_FILENAME_LENGTH];
    STRING                    ntNameString;
    UNICODE_STRING            ntUnicodeString;
    PDEVICE_OBJECT            deviceObject;
    PDEVICE_EXTENSION         deviceExtension;
    PDISK_DATA                diskData;
    NTSTATUS                  status;
    ULONG                     numberListElements;
    BOOLEAN                   found;

    partitionCount = ((partitionList->PartitionCount + 3) / 4) * 4;

    //
    // Zero all of the partition numbers.
    //

    for (partition = 0; partition < partitionCount; partition++) {
        partitionEntry = &partitionList->PartitionEntry[partition];
        partitionEntry->PartitionNumber = 0;
    }

    //
    // Walk through chain of partitions for this disk to determine
    // which existing partitions have no match.
    //

    deviceExtension = physicalExtension;
    diskData = (PDISK_DATA)(deviceExtension + 1);
    lastPartition = 0;

    do {

        deviceExtension = diskData->NextPartition;

        //
        // Check if this is the last partition in the chain.
        //

        if (!deviceExtension) {
           break;
        }

        //
        // Get the partition device extension from disk data.
        //

        diskData = (PDISK_DATA)(deviceExtension + 1);

        //
        // Check for highest partition number this far.
        //

        if (diskData->PartitionNumber > lastPartition) {
           lastPartition = diskData->PartitionNumber;
        }

        //
        // Check if this partition is not currently being used.
        //

        if (!deviceExtension->PartitionLength.QuadPart) {
           continue;
        }

        //
        // Loop through partition information to look for match.
        //

        found = FALSE;
        partitionOrdinal = 0;

        for (partition = 0; partition < partitionCount; partition++) {

            //
            // Get partition descriptor.
            //

            partitionEntry = &partitionList->PartitionEntry[partition];

            //
            // Check if empty, or describes extended partiton or hasn't changed.
            //

            if (partitionEntry->PartitionType == PARTITION_ENTRY_UNUSED ||
                IsContainerPartition(partitionEntry->PartitionType)) {
                continue;
            }

            //
            // Advance partition ordinal.
            //

            partitionOrdinal++;

            //
            // Check if new partition starts where this partition starts.
            //

            if (partitionEntry->StartingOffset.QuadPart !=
                      deviceExtension->StartingOffset.QuadPart) {
                continue;
            }

            //
            // Check if partition length is the same.
            //

            if (partitionEntry->PartitionLength.QuadPart ==
                      deviceExtension->PartitionLength.QuadPart) {

                DebugPrint((3,
                           "UpdateDeviceObjects: Found match for \\Harddisk%d\\Partition%d\n",
                           physicalExtension->DeviceNumber,
                           diskData->PartitionNumber));

                //
                // Indicate match is found and set partition number
                // in user buffer.
                //

                found = TRUE;
                partitionEntry->PartitionNumber = diskData->PartitionNumber;
                break;
            }
        }

        if (found) {

            //
            // A match is found.
            //

            diskData = (PDISK_DATA)(deviceExtension + 1);

            //
            // If this partition is marked for update then update partition type.
            //

            if (partitionEntry->RewritePartition) {
                diskData->PartitionType = partitionEntry->PartitionType;
            }

            //
            // Update partitional ordinal for calls to HAL routine
            // IoSetPartitionInformation.
            //

            diskData->PartitionOrdinal = partitionOrdinal;

            DebugPrint((1,
                       "UpdateDeviceObjects: Disk %d ordinal %d is partition %d\n",
                       physicalExtension->DeviceNumber,
                       diskData->PartitionOrdinal,
                       diskData->PartitionNumber));

        } else {

            //
            // no match was found, indicate this partition is gone.
            //

            DebugPrint((1,
                       "UpdateDeviceObjects: Deleting \\Device\\Harddisk%x\\Partition%x\n",
                       physicalExtension->DeviceNumber,
                       diskData->PartitionNumber));

            deviceExtension->PartitionLength.QuadPart = (LONGLONG) 0;
        }

    } while (TRUE);

    //
    // Walk through partition loop to find new partitions and set up
    // device extensions to describe them. In some cases new device
    // objects will be created.
    //

    partitionOrdinal = 0;

    for (partition = 0;
         partition < partitionCount;
         partition++) {

        //
        // Get partition descriptor.
        //

        partitionEntry = &partitionList->PartitionEntry[partition];

        //
        // Check if empty, or describes an extended partiton.
        //

        if (partitionEntry->PartitionType == PARTITION_ENTRY_UNUSED ||
            IsContainerPartition(partitionEntry->PartitionType)) {
            continue;
        }

        //
        // Keep track of position on the disk for calls to IoSetPartitionInformation.
        //

        partitionOrdinal++;

        //
        // Check if this entry should be rewritten.
        //

        if (!partitionEntry->RewritePartition) {
            continue;
        }

        if (partitionEntry->PartitionNumber) {

            //
            // Partition is an exact match with an existing partition, but is
            // being written anyway.
            //

            continue;
        }

        //
        // Check first if existing device object is available by
        // walking partition extension list.
        //

        partitionNumber = 0;
        deviceExtension = physicalExtension;
        diskData = (PDISK_DATA)(deviceExtension + 1);

        do {

            //
            // Get next partition device extension from disk data.
            //

            deviceExtension = diskData->NextPartition;

            if (!deviceExtension) {
               break;
            }

            diskData = (PDISK_DATA)(deviceExtension + 1);

            //
            // A device object is free if the partition length is set to zero.
            //

            if (!deviceExtension->PartitionLength.QuadPart) {
               partitionNumber = diskData->PartitionNumber;
               break;
            }

        } while (TRUE);

        //
        // If partition number is still zero then a new device object
        // must be created.
        //

        if (partitionNumber == 0) {

            lastPartition++;
            partitionNumber = lastPartition;

            //
            // Get or create partition object and set up partition parameters.
            //

            sprintf(ntNameBuffer,
                    "\\Device\\Harddisk%lu\\Partition%lu",
                    physicalExtension->DeviceNumber,
                    partitionNumber);

            RtlInitString(&ntNameString,
                          ntNameBuffer);

            status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                                  &ntNameString,
                                                  TRUE);

            if (!NT_SUCCESS(status)) {
                continue;
            }

            DebugPrint((3,
                        "UpdateDeviceObjects: Create device object %s\n",
                        ntNameBuffer));

            //
            // This is a new name. Create the device object to represent it.
            //

            status = IoCreateDevice(PhysicalDisk->DriverObject,
                                    DEVICE_EXTENSION_SIZE,
                                    &ntUnicodeString,
                                    FILE_DEVICE_DISK,
                                    0,
                                    FALSE,
                                    &deviceObject);

            if (!NT_SUCCESS(status)) {
                DebugPrint((1,
                            "UpdateDeviceObjects: Can't create device %s\n",
                            ntNameBuffer));
                RtlFreeUnicodeString(&ntUnicodeString);
                continue;
            }

            //
            // Set up device object fields.
            //

            deviceObject->Flags |= DO_DIRECT_IO;
            deviceObject->StackSize = PhysicalDisk->StackSize;

            //
            // Set up device extension fields.
            //

            deviceExtension = deviceObject->DeviceExtension;

            //
            // Copy physical disk extension to partition extension.
            //

            RtlMoveMemory(deviceExtension,
                          physicalExtension,
                          sizeof(DEVICE_EXTENSION));

            //
            // Initialize the new S-List.
            //

            if (deviceExtension->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE) {
                numberListElements = 30;
            } else {
                numberListElements = 8;
            }

            //
            // Build the lookaside list for srb's for this partition based on
            // whether the adapter and disk can do tagged queueing.
            //

            ScsiClassInitializeSrbLookasideList(deviceExtension,
                                                numberListElements);

            //
            // Allocate spinlock for zoning for split-request completion.
            //

            KeInitializeSpinLock(&deviceExtension->SplitRequestSpinLock);

            //
            // Write back partition number used in creating object name.
            //

            partitionEntry->PartitionNumber = partitionNumber;

            //
            // Clear flags initializing bit.
            //

            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

            //
            // Point back at device object.
            //

            deviceExtension->DeviceObject = deviceObject;

            RtlFreeUnicodeString(&ntUnicodeString);

            //
            // Link to end of partition chain using previous disk data.
            //

            diskData->NextPartition = deviceExtension;

            //
            // Get new disk data and zero next partition pointer.
            //

            diskData = (PDISK_DATA)(deviceExtension + 1);
            diskData->NextPartition = NULL;

        } else {

            //
            // Set pointer to disk data area that follows device extension.
            //

            diskData = (PDISK_DATA)(deviceExtension + 1);

            DebugPrint((1,
                        "UpdateDeviceObjects: Used existing device object \\Device\\Harddisk%x\\Partition%x\n",
                        physicalExtension->DeviceNumber,
                        partitionNumber));
        }

        //
        // Update partition information in partition device extension.
        //

        diskData->PartitionNumber = partitionNumber;
        diskData->PartitionType = partitionEntry->PartitionType;
        diskData->BootIndicator = partitionEntry->BootIndicator;
        deviceExtension->StartingOffset = partitionEntry->StartingOffset;
        deviceExtension->PartitionLength = partitionEntry->PartitionLength;
        diskData->HiddenSectors = partitionEntry->HiddenSectors;
        diskData->PartitionOrdinal = partitionOrdinal;

        DebugPrint((1,
                   "UpdateDeviceObjects: Ordinal %d is partition %d\n",
                   diskData->PartitionOrdinal,
                   diskData->PartitionNumber));

        //
        // Update partition number passed in to indicate the
        // device name for this partition.
        //

        partitionEntry->PartitionNumber = partitionNumber;
    }

} // end UpdateDeviceObjects()
