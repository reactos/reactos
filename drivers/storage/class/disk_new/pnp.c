/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    pnp.c

Abstract:

    SCSI disk class driver

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "disk.h"

extern NTSYSAPI ULONG InitSafeBootMode;

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, DiskAddDevice)
#pragma alloc_text(PAGE, DiskInitFdo)
#pragma alloc_text(PAGE, DiskInitPdo)
#pragma alloc_text(PAGE, DiskStartFdo)
#pragma alloc_text(PAGE, DiskStartPdo)
#pragma alloc_text(PAGE, DiskQueryId)
#pragma alloc_text(PAGE, DiskGenerateDeviceName)
#pragma alloc_text(PAGE, DiskCreateSymbolicLinks)
#pragma alloc_text(PAGE, DiskDeleteSymbolicLinks)
#pragma alloc_text(PAGE, DiskRemoveDevice)

#endif

NTSTATUS
NTAPI
DiskAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine gets a port drivers capabilities, obtains the
    inquiry data, searches the SCSI bus for the port driver and creates
    the device objects for the disks found.

Arguments:

    DriverObject - Pointer to driver object created by system.

    Pdo - Device object use to send requests to port driver.

Return Value:

    True is returned if one disk was found and successfully created.

--*/

{
    ULONG rootPartitionMountable = FALSE;

    PCONFIGURATION_INFORMATION configurationInformation;
    ULONG diskCount;

    NTSTATUS status;

    PAGED_CODE();

    //
    // See if we should be allowing file systems to mount on partition zero.
    //

    TRY {
        HANDLE deviceKey;

        UNICODE_STRING diskKeyName;
        OBJECT_ATTRIBUTES objectAttributes;
        HANDLE diskKey;

        RTL_QUERY_REGISTRY_TABLE queryTable[2];

        status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         KEY_READ,
                                         &deviceKey);

        if(!NT_SUCCESS(status)) {
            DebugPrint((1, "DiskAddDevice: Error %#08lx opening device key "
                           "for pdo %#08lx\n",
                        status, PhysicalDeviceObject));
            LEAVE;
        }

        RtlInitUnicodeString(&diskKeyName, L"Disk");
        InitializeObjectAttributes(&objectAttributes,
                                   &diskKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   deviceKey,
                                   NULL);

        status = ZwOpenKey(&diskKey, KEY_READ, &objectAttributes);
        ZwClose(deviceKey);

        if(!NT_SUCCESS(status)) {
            DebugPrint((1, "DiskAddDevice: Error %#08lx opening disk key "
                           "for pdo %#08lx device key %#x\n",
                        status, PhysicalDeviceObject, deviceKey));
            LEAVE;
        }

        RtlZeroMemory(queryTable, sizeof(queryTable));

        queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[0].Name = L"RootPartitionMountable";
        queryTable[0].EntryContext = &(rootPartitionMountable);

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        diskKey,
                                        queryTable,
                                        NULL,
                                        NULL);

        if(!NT_SUCCESS(status)) {
            DebugPrint((1, "DiskAddDevice: Error %#08lx reading value from "
                           "disk key %#x for pdo %#08lx\n",
                        status, diskKey, PhysicalDeviceObject));
        }

        ZwClose(diskKey);

    } FINALLY {

        //
        // Do nothing.
        //

        if(!NT_SUCCESS(status)) {
            DebugPrint((1, "DiskAddDevice: Will %sallow file system to mount on "
                           "partition zero of disk %#08lx\n",
                        (rootPartitionMountable ? "" : "not "),
                        PhysicalDeviceObject));
        }
    }

    //
    // Create device objects for disk
    //

    diskCount = 0;

    status = DiskCreateFdo(
                 DriverObject,
                 PhysicalDeviceObject,
                 &diskCount,
                 (BOOLEAN) !rootPartitionMountable
                 );

    //
    // Get the number of disks already initialized.
    //

    configurationInformation = IoGetConfigurationInformation();

    if (NT_SUCCESS(status)) {

        //
        // Increment system disk device count.
        //

        configurationInformation->DiskCount++;

    }

    return status;

} // end DiskAddDevice()

NTSTATUS
NTAPI
DiskInitFdo(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine is called to do one-time initialization of new device objects


Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PDISK_DATA diskData = (PDISK_DATA) fdoExtension->CommonExtension.DriverData;

    //ULONG srbFlags = 0;

    ULONG timeOut = 0;

    ULONG bytesPerSector;
    //UCHAR sectorShift;

    //BOOLEAN dmActive = FALSE;
    PULONG dmSkew;
    //ULONG dmByteSkew;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Build the lookaside list for srb's for the physical disk. Should only
    // need a couple.  If this fails then we don't have an emergency SRB so
    // fail the call to initialize.
    //

    ClassInitializeSrbLookasideList((PCOMMON_DEVICE_EXTENSION) fdoExtension,
                                    PARTITION0_LIST_SIZE);

    //
    // Because all requests share a common sense buffer, it is possible
    // for the buffer to be overwritten if the port driver completes
    // multiple failed requests that require a request sense before the
    // class driver's completion routine can consume the data in the buffer.
    // To prevent this, we allow the port driver to allocate a unique sense
    // buffer each time it needs one.  We are responsible for freeing this
    // buffer.  This also allows the adapter to be configured to support
    // additional sense data beyond the minimum 18 bytes.
    //

    fdoExtension->SrbFlags = SRB_FLAGS_PORT_DRIVER_ALLOCSENSE;

    //
    // Initialize the srb flags.
    //

    if (fdoExtension->DeviceDescriptor->CommandQueueing &&
        fdoExtension->AdapterDescriptor->CommandQueueing) {

        fdoExtension->SrbFlags = SRB_FLAGS_QUEUE_ACTION_ENABLE;

    }

    if (!TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {
        SET_FLAG(fdoExtension->DeviceFlags, DEV_SAFE_START_UNIT);
    }

    //
    // Look for controllers that require special flags.
    //

    ClassScanForSpecial(fdoExtension, DiskBadControllers, DiskSetSpecialHacks);

    //
    // Look into the registry to see if this device
    // requires special attention - [ like a hack ]
    //

    DiskScanRegistryForSpecial(fdoExtension);

    //srbFlags = fdoExtension->SrbFlags;

    //
    // Clear buffer for drive geometry.
    //

    RtlZeroMemory(&(fdoExtension->DiskGeometry),
                  sizeof(DISK_GEOMETRY));

    //
    // Allocate request sense buffer.
    //

    fdoExtension->SenseData = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                                    SENSE_BUFFER_SIZE,
                                                    DISK_TAG_START);

    if (fdoExtension->SenseData == NULL) {

        //
        // The buffer can not be allocated.
        //

        DebugPrint((1, "DiskInitFdo: Can not allocate request sense buffer\n"));

        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Physical device object will describe the entire
    // device, starting at byte offset 0.
    //

    fdoExtension->CommonExtension.StartingOffset.QuadPart = (LONGLONG)(0);

    //
    // Set timeout value in seconds.
    //

    timeOut = ClassQueryTimeOutRegistryValue(Fdo);
    if (timeOut) {
        fdoExtension->TimeOutValue = timeOut;
    } else {
        fdoExtension->TimeOutValue = SCSI_DISK_TIMEOUT;
    }

    //
    // If this is a removable drive, build an entry in devicemap\scsi
    // indicating it's physicaldriveN name, set up the appropriate
    // update partitions routine and set the flags correctly.
    // note: only do this after the timeout value is set, above.
    //

    if (fdoExtension->DeviceDescriptor->RemovableMedia) {
        ClassUpdateInformationInRegistry( Fdo,
                                          "PhysicalDrive",
                                          fdoExtension->DeviceNumber,
                                          NULL,
                                          0);
        //
        // Enable media change notification for removable disks
        //
        ClassInitializeMediaChangeDetection(fdoExtension,
                                            "Disk");

        SET_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA);
        diskData->UpdatePartitionRoutine = DiskUpdateRemovablePartitions;

    } else {

        SET_FLAG(fdoExtension->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
        diskData->UpdatePartitionRoutine = DiskUpdatePartitions;

    }

    //
    // Read the drive capacity.  Don't use the disk version of the routine here
    // since we don't know the disk signature yet - the disk version will
    // attempt to determine the BIOS reported geometry.
    //

    status = ClassReadDriveCapacity(Fdo);

    //
    // If the read capacity failed then just return, unless this is a
    // removable disk where a device object partition needs to be created.
    //

    if (!NT_SUCCESS(status) &&
        !(Fdo->Characteristics & FILE_REMOVABLE_MEDIA)) {

        DebugPrint((1,
            "DiskInitFdo: Can't read capacity for device %p\n",
            Fdo));

        if (fdoExtension->DeviceDescriptor->RemovableMedia) {
            fdoExtension->DiskGeometry.MediaType = RemovableMedia;
            Fdo->Flags &= ~DO_VERIFY_VOLUME;
        } else {
            fdoExtension->DiskGeometry.MediaType = FixedMedia;
        }

        status = STATUS_SUCCESS;
    }

    //
    // Set up sector size fields.
    //
    // Stack variables will be used to update
    // the partition device extensions.
    //
    // The device extension field SectorShift is
    // used to calculate sectors in I/O transfers.
    //
    // The DiskGeometry structure is used to service
    // IOCTls used by the format utility.
    //

    bytesPerSector = fdoExtension->DiskGeometry.BytesPerSector;

    //
    // Make sure sector size is not zero.
    //

    if (bytesPerSector == 0) {

        //
        // Default sector size for disk is 512.
        //

        bytesPerSector = fdoExtension->DiskGeometry.BytesPerSector = 512;
    }

    //sectorShift = fdoExtension->SectorShift;

    //
    // Determine is DM Driver is loaded on an IDE drive that is
    // under control of Atapi - this could be either a crashdump or
    // an Atapi device is sharing the controller with an IDE disk.
    //

    HalExamineMBR(fdoExtension->CommonExtension.DeviceObject,
                  fdoExtension->DiskGeometry.BytesPerSector,
                  (ULONG)0x54,
                  (PVOID*)&dmSkew);

    if (dmSkew) {

        //
        // Update the device extension, so that the call to IoReadPartitionTable
        // will get the correct information. Any I/O to this disk will have
        // to be skewed by *dmSkew sectors aka DMByteSkew.
        //

        fdoExtension->DMSkew = *dmSkew;
        fdoExtension->DMActive = TRUE;
        fdoExtension->DMByteSkew = fdoExtension->DMSkew * bytesPerSector;

        //
        // Save away the information that we need, since this deviceExtension will soon be
        // blown away.
        //

        //dmActive = TRUE;
        //dmByteSkew = fdoExtension->DMByteSkew;

    }

#if defined(_X86_)
    //
    // Try to read the signature off the disk and determine the correct drive
    // geometry based on that.  This requires rereading the disk size to get
    // the cylinder count updated correctly.
    //

    if(fdoExtension->DeviceDescriptor->RemovableMedia == FALSE) {
        DiskReadSignature(Fdo);
        DiskReadDriveCapacity(Fdo);
    }
#endif

    //
    // Register interfaces for this device
    //
    {
        UNICODE_STRING interfaceName;

        RtlInitUnicodeString(&interfaceName, NULL);

        status = IoRegisterDeviceInterface(fdoExtension->LowerPdo,
                                           (LPGUID) &DiskClassGuid,
                                           NULL,
                                           &interfaceName);

        if(NT_SUCCESS(status)) {

            diskData->DiskInterfaceString = interfaceName;
            status = IoSetDeviceInterfaceState(&interfaceName, TRUE);

        } else {
            interfaceName.Buffer = NULL;
        }

        if(!NT_SUCCESS(status)) {

            DebugPrint((1, "DiskInitFdo: Unable to register or set disk DCA "
                           "for fdo %p [%lx]\n", Fdo, status));

            RtlFreeUnicodeString(&interfaceName);
            RtlInitUnicodeString(&(diskData->DiskInterfaceString), NULL);
        }
    }

    DiskCreateSymbolicLinks(Fdo);

    //
    // Determine the type of disk and enable failure prediction in the hardware
    // and enable failure prediction polling.
    //

    if (InitSafeBootMode == 0)
    {
        DiskDetectFailurePrediction(fdoExtension,
                                  &diskData->FailurePredictionCapability);

        if (diskData->FailurePredictionCapability != FailurePredictionNone)
        {
            //
            // Cool, we've got some sort of failure prediction, enable it
            // at the hardware and then enable polling for it
            //

            //
            // By default we allow performance to be degradeded if failure
            // prediction is enabled.
            //
            // TODO: Make a registry entry ?
            //

            diskData->AllowFPPerfHit = TRUE;

            //
            // Enable polling only after Atapi and SBP2 add support for the new
            // SRB flag that indicates that the request should not reset the
            // drive spin down idle timer.
            //

            status = DiskEnableDisableFailurePredictPolling(fdoExtension,
                                          TRUE,
                                          DISK_DEFAULT_FAILURE_POLLING_PERIOD);

            DebugPrint((3, "DiskInitFdo: Failure Prediction Poll enabled as "
                           "%d for device %p\n",
                     diskData->FailurePredictionCapability,
                     Fdo));
        }
    } else {

        //
        // In safe boot mode we do not enable failure prediction, as perhaps
        // it is the reason why normal boot does not work
        //

        diskData->FailurePredictionCapability = FailurePredictionNone;

    }

    //
    // Initialize the verify mutex
    //

    KeInitializeMutex(&diskData->VerifyMutex, MAX_SECTORS_PER_VERIFY);

    return(STATUS_SUCCESS);

} // end DiskInitFdo()

NTSTATUS
NTAPI
DiskInitPdo(
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This routine will create the well known names for a PDO and register
    it's device interfaces.

--*/

{
    PCOMMON_DEVICE_EXTENSION pdoExtension = Pdo->DeviceExtension;
    PDISK_DATA diskData = pdoExtension->DriverData;

    UNICODE_STRING interfaceName;

    NTSTATUS status;

    PAGED_CODE();

    DiskCreateSymbolicLinks(Pdo);

    //
    // Register interfaces for this device
    //

    RtlInitUnicodeString(&interfaceName, NULL);

    status = IoRegisterDeviceInterface(Pdo,
                                       (LPGUID) &PartitionClassGuid,
                                       NULL,
                                       &interfaceName);

    if(NT_SUCCESS(status)) {

        diskData->PartitionInterfaceString = interfaceName;
        status = IoSetDeviceInterfaceState(&interfaceName, TRUE);

    } else {
        interfaceName.Buffer = NULL;
    }

    if(!NT_SUCCESS(status)) {
        DebugPrint((1, "DiskInitPdo: Unable to register partition DCA for "
                    "pdo %p [%lx]\n", Pdo, status));

        RtlFreeUnicodeString(&interfaceName);
        RtlInitUnicodeString(&(diskData->PartitionInterfaceString), NULL);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DiskStartPdo(
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This routine will create the well known names for a PDO and register
    it's device interfaces.

--*/

{
    PAGED_CODE();

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DiskStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )

{
    PFUNCTIONAL_DEVICE_EXTENSION fdo = DeviceObject->DeviceExtension;

    if(fdo->CommonExtension.IsFdo) {
        DiskAcquirePartitioningLock(fdo);
        DiskInvalidatePartitionTable(fdo, TRUE);
        DiskReleasePartitioningLock(fdo);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DiskQueryId(
    IN PDEVICE_OBJECT Pdo,
    IN BUS_QUERY_ID_TYPE IdType,
    IN PUNICODE_STRING UnicodeIdString
    )

/*++

Routine Description:

    This routine generates the PNP id's for the disk's "children".  If the
    specified ID isn't one that the routine can generate it must return
    STATUS_NOT_IMPLEMENTED so classpnp will know not to do anything with the
    PNP request's status.

    This routine allocates the buffer for the UnicodeIdString.  It is the
    caller's responsibility to free the buffer when it's done.

Arguments:

    Pdo - a pointer to the PDO we are to generate an ID for

    IdType - the type of ID to be generated

    UnicodeIdString - a string to put the results into.

Return Value:

    STATUS_SUCCCESS if successful

    STATUS_NOT_IMPLEMENTED if the IdType is not one supported by this routine

    error status otherwise.

--*/

{
    ANSI_STRING ansiIdString;

    //NTSTATUS status;

    PAGED_CODE();
    ASSERT_PDO(Pdo);

    if(IdType == BusQueryDeviceID) {

        if((Pdo->Characteristics & FILE_REMOVABLE_MEDIA) == 0) {
            RtlInitAnsiString(&ansiIdString, "STORAGE\\Partition");
            return RtlAnsiStringToUnicodeString(UnicodeIdString, &ansiIdString, TRUE);
        }

        RtlInitAnsiString(&ansiIdString,
                          "STORAGE\\RemovableMedia");

        return RtlAnsiStringToUnicodeString(UnicodeIdString, &ansiIdString, TRUE);
    }

    if(IdType == BusQueryInstanceID) {

        //PPHYSICAL_DEVICE_EXTENSION pdoExtension = Pdo->DeviceExtension;
        PCOMMON_DEVICE_EXTENSION commonExtension = Pdo->DeviceExtension;
        PDISK_DATA diskData = commonExtension->PartitionZeroExtension->CommonExtension.DriverData;

        UCHAR string[64];

        if((Pdo->Characteristics & FILE_REMOVABLE_MEDIA) == 0) {

            if (diskData->PartitionStyle == PARTITION_STYLE_MBR) {
                sprintf(string, "S%08lx_O%I64x_L%I64x",
                        diskData->Mbr.Signature,
                        commonExtension->StartingOffset.QuadPart,
                        commonExtension->PartitionLength.QuadPart);
            } else {
                sprintf(string,
                        "S%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02xS_O%I64x_L%I64x",
                        diskData->Efi.DiskId.Data1,
                        diskData->Efi.DiskId.Data2,
                        diskData->Efi.DiskId.Data3,
                        diskData->Efi.DiskId.Data4[0],
                        diskData->Efi.DiskId.Data4[1],
                        diskData->Efi.DiskId.Data4[2],
                        diskData->Efi.DiskId.Data4[3],
                        diskData->Efi.DiskId.Data4[4],
                        diskData->Efi.DiskId.Data4[5],
                        diskData->Efi.DiskId.Data4[6],
                        diskData->Efi.DiskId.Data4[7],
                        commonExtension->StartingOffset.QuadPart,
                        commonExtension->PartitionLength.QuadPart);
            }
        } else {
            sprintf(string, "RM");
        }

        RtlInitAnsiString(&ansiIdString, string);

        return RtlAnsiStringToUnicodeString(UnicodeIdString, &ansiIdString, TRUE);
    }

    if((IdType == BusQueryHardwareIDs) || (IdType == BusQueryCompatibleIDs)) {

        RtlInitAnsiString(&ansiIdString, "STORAGE\\Volume");

        UnicodeIdString->MaximumLength = (USHORT) RtlAnsiStringToUnicodeSize(&ansiIdString) + sizeof(UNICODE_NULL);

        UnicodeIdString->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                        UnicodeIdString->MaximumLength,
                                                        DISK_TAG_PNP_ID);

        if(UnicodeIdString->Buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(UnicodeIdString->Buffer, UnicodeIdString->MaximumLength);

        return RtlAnsiStringToUnicodeString(UnicodeIdString,
                                            &ansiIdString,
                                            FALSE);
    }

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DiskGenerateDeviceName(
    IN BOOLEAN IsFdo,
    IN ULONG DeviceNumber,
    IN OPTIONAL ULONG PartitionNumber,
    IN OPTIONAL PLARGE_INTEGER StartingOffset,
    IN OPTIONAL PLARGE_INTEGER PartitionLength,
    OUT PUCHAR *RawName
    )

/*++

Routine Description:

    This routine will allocate a unicode string buffer and then fill it in
    with a generated name for the specified device object.

    It is the responsibility of the user to allocate a UNICODE_STRING structure
    to pass in and to free UnicodeName->Buffer when done with it.

Arguments:

    DeviceObject - a pointer to the device object

    UnicodeName - a unicode string to put the name buffer into

Return Value:

    status

--*/

//#define PDO_NAME_FORMAT "\\Device\\Harddisk%d\\DP(%d)%d"
#define PDO_NAME_FORMAT "\\Device\\Harddisk%d\\DP(%d)%#I64x-%#I64x+%lx"
#define FDO_NAME_FORMAT "\\Device\\Harddisk%d\\DR%d"

//#define PDO_NAME_FORMAT (PDO_BASE_NAME "+%#I64x+%#I64x+%#lx")

{
    UCHAR rawName[64];
    static ULONG diskDeviceSequenceNumber = 0;

    PAGED_CODE();

    if(!IsFdo) {

        ASSERT(ARGUMENT_PRESENT((PVOID)(ULONG_PTR) PartitionNumber));
        ASSERT(ARGUMENT_PRESENT(PartitionLength));
        ASSERT(ARGUMENT_PRESENT(StartingOffset));

        sprintf(rawName, PDO_NAME_FORMAT, DeviceNumber, PartitionNumber,
                                          StartingOffset->QuadPart,
                                          PartitionLength->QuadPart,
                                          diskDeviceSequenceNumber++);
    } else {

        ASSERT(!ARGUMENT_PRESENT((PVOID)(ULONG_PTR) PartitionNumber));
        ASSERT(!ARGUMENT_PRESENT(PartitionLength));
        ASSERT(!ARGUMENT_PRESENT(StartingOffset));

        sprintf(rawName, FDO_NAME_FORMAT, DeviceNumber,
                                          diskDeviceSequenceNumber++);

    }

    *RawName = ExAllocatePoolWithTag(PagedPool,
                                     strlen(rawName) + 1,
                                     DISK_TAG_NAME);

    if(*RawName == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    strcpy(*RawName, rawName);

    DebugPrint((2, "DiskGenerateDeviceName: generated \"%s\"\n", rawName));

    return STATUS_SUCCESS;
}

VOID
NTAPI
DiskCreateSymbolicLinks(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine will generate a symbolic link for the specified device object
    using the well known form \\Device\HarddiskX\PartitionY, where X and Y are
    filled in using the partition information in the device object's extension.

    This routine will not try to delete any previous symbolic link for the
    same generated name - the caller must make sure the symbolic link has
    been broken before calling this routine.

Arguments:

    DeviceObject - the device object to make a well known name for

Return Value:

    STATUS

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PDISK_DATA diskData = commonExtension->DriverData;

    WCHAR wideSourceName[64];
    UNICODE_STRING unicodeSourceName;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Build the destination for the link first using the device name
    // stored in the device object
    //

    ASSERT(commonExtension->DeviceName.Buffer);

    if(!diskData->LinkStatus.WellKnownNameCreated) {
        //
        // Put together the source name using the partition and device number
        // in the device extension and disk data segment
        //

        swprintf(wideSourceName, L"\\Device\\Harddisk%d\\Partition%d",
                 commonExtension->PartitionZeroExtension->DeviceNumber,
                 (commonExtension->IsFdo ?
                    0 :
                    commonExtension->PartitionNumber));

        RtlInitUnicodeString(&unicodeSourceName, wideSourceName);

        DebugPrint((1, "DiskCreateSymbolicLink: Linking %wZ to %wZ\n",
                   &unicodeSourceName,
                   &commonExtension->DeviceName));

        status = IoCreateSymbolicLink(&unicodeSourceName,
                                      &commonExtension->DeviceName);

    #if DBG

        if((status == STATUS_OBJECT_NAME_EXISTS) ||
           (status == STATUS_OBJECT_NAME_COLLISION)) {

            DebugPrint((1, "DiskCreateSymbolicLink: name %wZ already exists\n",
                        &unicodeSourceName));
        }
    #endif

        if(NT_SUCCESS(status)){
            diskData->LinkStatus.WellKnownNameCreated = TRUE;
        }
    }

    if((!diskData->LinkStatus.PhysicalDriveLinkCreated) &&
       (commonExtension->IsFdo)) {

        //
        // Create a physical drive N link using the device number we saved
        // away during AddDevice.
        //

        swprintf(wideSourceName,
                 L"\\DosDevices\\PhysicalDrive%d",
                 commonExtension->PartitionZeroExtension->DeviceNumber);

        RtlInitUnicodeString(&unicodeSourceName, wideSourceName);

        DebugPrint((1, "DiskCreateSymbolicLink: Linking %wZ to %wZ\n",
                    &unicodeSourceName,
                    &(commonExtension->DeviceName)));

        status = IoCreateSymbolicLink(&unicodeSourceName,
                                      &(commonExtension->DeviceName));

#if DBG

        if((status == STATUS_OBJECT_NAME_EXISTS) ||
           (status == STATUS_OBJECT_NAME_COLLISION)) {

            DebugPrint((1, "DiskCreateSymbolicLink: name %wZ already exists\n",
                        &unicodeSourceName));
        }
#endif

        if(NT_SUCCESS(status)) {
            diskData->LinkStatus.PhysicalDriveLinkCreated = TRUE;
        }
    } else if(commonExtension->IsFdo == FALSE) {
        diskData->LinkStatus.PhysicalDriveLinkCreated = FALSE;
    }

    return;
}

VOID
NTAPI
DiskDeleteSymbolicLinks(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine will delete the well known name (symlink) for the specified
    device.  It generates the link name using information stored in the
    device extension

Arguments:

    DeviceObject - the device object we are unlinking

Return Value:

    status

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = commonExtension->DriverData;

    WCHAR wideLinkName[64];
    UNICODE_STRING unicodeLinkName;

    PAGED_CODE();

    if(diskData->LinkStatus.WellKnownNameCreated) {

        swprintf(wideLinkName,
                 L"\\Device\\Harddisk%d\\Partition%d",
                 commonExtension->PartitionZeroExtension->DeviceNumber,
                 (commonExtension->IsFdo ? 0 :
                                           commonExtension->PartitionNumber));

        RtlInitUnicodeString(&unicodeLinkName, wideLinkName);

        IoDeleteSymbolicLink(&unicodeLinkName);

        diskData->LinkStatus.WellKnownNameCreated = FALSE;
    }

    if(diskData->LinkStatus.PhysicalDriveLinkCreated) {

        ASSERT_FDO(DeviceObject);

        swprintf(wideLinkName,
                 L"\\DosDevices\\PhysicalDrive%d",
                 commonExtension->PartitionZeroExtension->DeviceNumber);

        RtlInitUnicodeString(&unicodeLinkName, wideLinkName);

        IoDeleteSymbolicLink(&unicodeLinkName);

        diskData->LinkStatus.PhysicalDriveLinkCreated = FALSE;
    }

    return;
}

NTSTATUS
NTAPI
DiskRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )

/*++

Routine Description:

    This routine will release any resources the device may have allocated for
    this device object and return.

Arguments:

    DeviceObject - the device object being removed

Return Value:

    status

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = commonExtension->DriverData;

    PAGED_CODE();

    //
    // Handle query and cancel
    //

    if((Type == IRP_MN_QUERY_REMOVE_DEVICE) ||
       (Type == IRP_MN_CANCEL_REMOVE_DEVICE))  {
        return STATUS_SUCCESS;
    }

    if(commonExtension->IsFdo) {

        PFUNCTIONAL_DEVICE_EXTENSION fdoExtension =
            DeviceObject->DeviceExtension;

        //
        // Purge the cached partition table (if any).
        //

        DiskAcquirePartitioningLock(fdoExtension);
        DiskInvalidatePartitionTable(fdoExtension, TRUE);
        DiskReleasePartitioningLock(fdoExtension);

        //
        // Delete our object directory.
        //

        if(fdoExtension->AdapterDescriptor) {
            ExFreePool(fdoExtension->AdapterDescriptor);
            fdoExtension->AdapterDescriptor = NULL;
        }

        if(fdoExtension->DeviceDescriptor) {
            ExFreePool(fdoExtension->DeviceDescriptor);
            fdoExtension->DeviceDescriptor = NULL;
        }

        if(fdoExtension->SenseData) {
            ExFreePool(fdoExtension->SenseData);
            fdoExtension->SenseData = NULL;
        }

        if(fdoExtension->DeviceDirectory != NULL) {
            ZwMakeTemporaryObject(fdoExtension->DeviceDirectory);
            ZwClose(fdoExtension->DeviceDirectory);
            fdoExtension->DeviceDirectory = NULL;
        }

        if(Type == IRP_MN_REMOVE_DEVICE) {
            IoGetConfigurationInformation()->DiskCount--;
        }

    } else {

        //PPHYSICAL_DEVICE_EXTENSION pdoExtension = DeviceObject->DeviceExtension;

    }

    DiskDeleteSymbolicLinks(DeviceObject);

    //
    // Release the mounted device interface if we've set it.
    //

    if(diskData->PartitionInterfaceString.Buffer != NULL) {
        IoSetDeviceInterfaceState(&(diskData->PartitionInterfaceString), FALSE);
        RtlFreeUnicodeString(&(diskData->PartitionInterfaceString));
        RtlInitUnicodeString(&(diskData->PartitionInterfaceString), NULL);
    }
    if(diskData->DiskInterfaceString.Buffer != NULL) {
        IoSetDeviceInterfaceState(&(diskData->DiskInterfaceString), FALSE);
        RtlFreeUnicodeString(&(diskData->DiskInterfaceString));
        RtlInitUnicodeString(&(diskData->DiskInterfaceString), NULL);
    }

    ClassDeleteSrbLookasideList(commonExtension);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DiskStartFdo(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine will query the underlying device for any information necessary
    to complete initialization of the device.  This will include physical
    disk geometry, mode sense information and such.

    This routine does not perform partition enumeration - that is left to the
    re-enumeration routine

    If this routine fails it will return an error value.  It does not clean up
    any resources - that is left for the Stop/Remove routine.

Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = &(fdoExtension->CommonExtension);
    PDISK_DATA diskData = commonExtension->DriverData;
    STORAGE_HOTPLUG_INFO hotplugInfo;
    ULONG writeCacheOverride = DiskWriteCacheDefault;
    DISK_CACHE_INFORMATION cacheInfo;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Get the hotplug information, so we can turn off write cache if needed
    //
    // NOTE: Capabilities info is not good enough to determine hotplugedness
    //       as  we cannot determine device relations  information and other
    //       dependencies. Get the hotplug info instead
    //

    {
        PIRP irp;
        KEVENT event;
        IO_STATUS_BLOCK statusBlock;

        KeInitializeEvent(&event, SynchronizationEvent, FALSE);
        RtlZeroMemory(&hotplugInfo, sizeof(STORAGE_HOTPLUG_INFO));

        irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_HOTPLUG_INFO,
                                            Fdo,
                                            NULL,
                                            0L,
                                            &hotplugInfo,
                                            sizeof(STORAGE_HOTPLUG_INFO),
                                            FALSE,
                                            &event,
                                            &statusBlock);

        if (irp != NULL) {

            // send to self -- classpnp handles this
            status = IoCallDriver(Fdo, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                status = statusBlock.Status;
            }
        }
    }

    //
    // Clear the DEV_WRITE_CACHE flag now  and set
    // it below only if we read that from the disk
    //

    CLEAR_FLAG(fdoExtension->DeviceFlags, DEV_WRITE_CACHE);

    if (TEST_FLAG(fdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_DISABLE_WRITE_CACHE))
    {
        //
        // This flag overrides the user's  setting, because  faulty firmware
        // may cause the filesystem to refuse to format media on this device
        //
        DebugPrint((1,
                    "DiskStartFdo: Shutting off write cache for %p due to %s\n",
                    Fdo,
                    "Possible Firmware Issue"));

        writeCacheOverride = DiskWriteCacheDisable;
    }
    else
    {
        //
        // Look into the registry to  see if the user
        // has chosen to override the default setting
        //
        ClassGetDeviceParameter(fdoExtension,
                                DiskDeviceParameterSubkey,
                                DiskDeviceUserWriteCacheSetting,
                                &writeCacheOverride);

        if (writeCacheOverride == DiskWriteCacheDefault)
        {
            //
            // The user has not overridden the default settings
            //
            if (hotplugInfo.DeviceHotplug && !hotplugInfo.WriteCacheEnableOverride)
            {
                DebugPrint((1,
                            "DiskStartFdo: Shutting off write cache for %p due to %s\n",
                            Fdo,
                            "Hotplug Device"));

                writeCacheOverride = DiskWriteCacheDisable;
            }
            else if (hotplugInfo.MediaHotplug)
            {
                DebugPrint((1,
                            "DiskStartFdo: Shutting off write cache for %p due to %s\n",
                            Fdo,
                            "Hotplug (unlockable) Media"));

                writeCacheOverride = DiskWriteCacheDisable;
            }
            else
            {
                //
                // We enable write cache if this device has no specific issues
                //
                writeCacheOverride = DiskWriteCacheEnable;
            }
        }
    }

    //
    // Query the disk to see if write cache is enabled
    // and  set the DEV_WRITE_CACHE flag appropriately
    //

    RtlZeroMemory(&cacheInfo, sizeof(DISK_CACHE_INFORMATION));

    status = DiskGetCacheInformation(fdoExtension, &cacheInfo);

    if (NT_SUCCESS(status))
    {
        if (cacheInfo.WriteCacheEnabled != FALSE)
        {
            if (writeCacheOverride == DiskWriteCacheDisable)
            {
                //
                // Write cache is currently enabled on this
                // device, but we would like to turn it off
                //
                cacheInfo.WriteCacheEnabled = FALSE;

                status = DiskSetCacheInformation(fdoExtension, &cacheInfo);
            }
            else
            {
                //
                // The write cache setting either matches
                // our needs or we don't care
                //
                SET_FLAG(fdoExtension->DeviceFlags, DEV_WRITE_CACHE);
            }
        }
        else
        {
            if (writeCacheOverride == DiskWriteCacheEnable)
            {
                //
                // Write cache is currently disabled on this
                // device, but we  would  like to turn it on
                //
                cacheInfo.WriteCacheEnabled = TRUE;

                status = DiskSetCacheInformation(fdoExtension, &cacheInfo);

                SET_FLAG(fdoExtension->DeviceFlags, DEV_WRITE_CACHE);
            }
        }
    }

    //
    // In the event that there's a cached partition table flush it now.
    //

    DiskAcquirePartitioningLock(fdoExtension);
    DiskInvalidatePartitionTable(fdoExtension, TRUE);
    DiskReleasePartitioningLock(fdoExtension);

    //
    // Get the SCSI address if it's available for use with SMART ioctls.
    //

    {
        PIRP irp;
        KEVENT event;
        IO_STATUS_BLOCK statusBlock;

        KeInitializeEvent(&event, SynchronizationEvent, FALSE);

        irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_ADDRESS,
                                            commonExtension->LowerDeviceObject,
                                            NULL,
                                            0L,
                                            &(diskData->ScsiAddress),
                                            sizeof(SCSI_ADDRESS),
                                            FALSE,
                                            &event,
                                            &statusBlock);

        if(irp != NULL) {


            status = IoCallDriver(commonExtension->LowerDeviceObject, irp);

            if(status == STATUS_PENDING) {
                KeWaitForSingleObject(&event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                status = statusBlock.Status;
            }
        }
    }

    return STATUS_SUCCESS;

} // end DiskStartFdo()
