/*--

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    cdrom.c

Abstract:

    The CDROM class driver tranlates IRPs to SRBs with embedded CDBs
    and sends them to its devices through the port driver.

Environment:

    kernel mode only

Notes:

    SCSI Tape, CDRom and Disk class drivers share common routines
    that can be found in the CLASS directory (..\ntos\dd\class).

Revision History:

--*/

#include "cdrom.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)

#pragma alloc_text(PAGE, CdRomUnload)
#pragma alloc_text(PAGE, CdRomAddDevice)
#pragma alloc_text(PAGE, CdRomCreateDeviceObject)
#pragma alloc_text(PAGE, CdRomStartDevice)
#pragma alloc_text(PAGE, ScanForSpecial)
#pragma alloc_text(PAGE, ScanForSpecialHandler)
#pragma alloc_text(PAGE, CdRomRemoveDevice)
#pragma alloc_text(PAGE, CdRomGetDeviceType)
#pragma alloc_text(PAGE, CdRomReadWriteVerification)
#pragma alloc_text(PAGE, CdRomGetDeviceParameter)
#pragma alloc_text(PAGE, CdRomSetDeviceParameter)
#pragma alloc_text(PAGE, CdRomPickDvdRegion)
#pragma alloc_text(PAGE, CdRomIsPlayActive)

#pragma alloc_text(PAGEHITA, HitachiProcessError)
#pragma alloc_text(PAGEHIT2, HitachiProcessErrorGD2000)

#pragma alloc_text(PAGETOSH, ToshibaProcessErrorCompletion)
#pragma alloc_text(PAGETOSH, ToshibaProcessError)

#endif

#define IS_WRITE_REQUEST(irpStack)                                             \
 (irpStack->MajorFunction == IRP_MJ_WRITE)
 
#define IS_READ_WRITE_REQUEST(irpStack)                                        \
((irpStack->MajorFunction == IRP_MJ_READ)  ||                                  \
 (irpStack->MajorFunction == IRP_MJ_WRITE) ||                                  \
 ((irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&                        \
  (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_RAW_READ)))


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
    PCDROM_DRIVER_EXTENSION driverExtension;
    NTSTATUS status;
    
    PAGED_CODE();
    
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceLog((CdromDebugTrace,
                "CDROM.SYS DriverObject %p loading\n", DriverObject));
    
    status = IoAllocateDriverObjectExtension(DriverObject,
                                             CDROM_DRIVER_EXTENSION_ID,
                                             sizeof(CDROM_DRIVER_EXTENSION),
                                             (PVOID*)&driverExtension);    

    if (!NT_SUCCESS(status)) {
        TraceLog((CdromDebugWarning,
                    "DriverEntry !! no DriverObjectExtension %x\n", status));
        return status;
    }

    //
    // always zero the memory, since we are now reloading the driver.
    //

    RtlZeroMemory(driverExtension, sizeof(CDROM_DRIVER_EXTENSION));

    //
    // Zero InitData
    //

    RtlZeroMemory (&InitializationData, sizeof(CLASS_INIT_DATA));

    //
    // Set sizes
    //

    InitializationData.InitializationDataSize = sizeof(CLASS_INIT_DATA);

    InitializationData.FdoData.DeviceExtensionSize = DEVICE_EXTENSION_SIZE;

    InitializationData.FdoData.DeviceType = FILE_DEVICE_CD_ROM;
    InitializationData.FdoData.DeviceCharacteristics =
        FILE_REMOVABLE_MEDIA | FILE_DEVICE_SECURE_OPEN;

    //
    // Set entry points
    //

    InitializationData.FdoData.ClassError = CdRomErrorHandler;
    InitializationData.FdoData.ClassInitDevice = CdRomInitDevice;
    InitializationData.FdoData.ClassStartDevice = CdRomStartDevice;
    InitializationData.FdoData.ClassStopDevice = CdRomStopDevice;
    InitializationData.FdoData.ClassRemoveDevice = CdRomRemoveDevice;

    InitializationData.FdoData.ClassReadWriteVerification = CdRomReadWriteVerification;
    InitializationData.FdoData.ClassDeviceControl = CdRomDeviceControlDispatch;

    InitializationData.FdoData.ClassPowerDevice = ClassSpinDownPowerHandler;
    InitializationData.FdoData.ClassShutdownFlush = CdRomShutdownFlush;
    InitializationData.FdoData.ClassCreateClose = NULL;

    InitializationData.ClassStartIo = CdRomStartIo;
    InitializationData.ClassAddDevice = CdRomAddDevice;

    InitializationData.ClassTick = CdRomTickHandler;
    InitializationData.ClassUnload = CdRomUnload;

    //
    // Call the class init routine
    //
    return ClassInitialize( DriverObject, RegistryPath, &InitializationData);

} // end DriverEntry()

VOID
NTAPI
CdRomUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);
    TraceLog((CdromDebugTrace,
                "CDROM.SYS DriverObject %p unloading\n", DriverObject));
    WPP_CLEANUP(DriverObject);
    return;
} // end CdRomUnload()

NTSTATUS
NTAPI
CdRomAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates and initializes a new FDO for the corresponding
    PDO.  It may perform property queries on the FDO but cannot do any
    media access operations.

Arguments:

    DriverObject - CDROM class driver object.

    Pdo - the physical device object we are being added to

Return Value:

    status

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Get the address of the count of the number of cdroms already initialized.
    //
    DbgPrint("add device\n");

    status = CdRomCreateDeviceObject(DriverObject,
                                     PhysicalDeviceObject);

    //
    // Note: this always increments driver extension counter
    //       it will eventually wrap, and fail additions
    //       if an existing cdrom has the given number.
    //       so unlikely that we won't even bother considering
    //       this case, since the cure is quite likely worse
    //       than the symptoms.
    //

    if(NT_SUCCESS(status)) {

        //
        // keep track of the total number of active cdroms in IoGet(),
        // as some programs use this to determine when they have found
        // all the cdroms in the system.
        //

        TraceLog((CdromDebugTrace, "CDROM.SYS Add succeeded\n"));
        IoGetConfigurationInformation()->CdRomCount++;

    } else {

        TraceLog((CdromDebugWarning,
                    "CDROM.SYS Add failed! %x\n", status));

    }

    return status;
}

NTSTATUS
NTAPI
CdRomCreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
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
    UCHAR ntNameBuffer[64];
    //STRING ntNameString;
    NTSTATUS status;

    PDEVICE_OBJECT lowerDevice = NULL;
    PDEVICE_OBJECT deviceObject = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;
    PCDROM_DATA cdData = NULL;
    PCDROM_DRIVER_EXTENSION driverExtension = NULL;
    ULONG deviceNumber;

    //CCHAR                   dosNameBuffer[64];
    //CCHAR                   deviceNameBuffer[64];
    //STRING                  deviceNameString;
    //STRING                  dosString;
    //UNICODE_STRING          dosUnicodeString;
    //UNICODE_STRING          unicodeString;

    PAGED_CODE();

    //
    // Claim the device. Note that any errors after this
    // will goto the generic handler, where the device will
    // be released.
    //

    lowerDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);

    status = ClassClaimDevice(lowerDevice, FALSE);

    if(!NT_SUCCESS(status)) {

        //
        // Someone already had this device - we're in trouble
        //

        ObDereferenceObject(lowerDevice);
        return status;
    }

    //
    // Create device object for this device by first getting a unique name
    // for the device and then creating it.
    //

    driverExtension = IoGetDriverObjectExtension(DriverObject,
                                                 CDROM_DRIVER_EXTENSION_ID);
    ASSERT(driverExtension != NULL);

    //
    // InterlockedCdRomCounter is biased by 1.
    //

    deviceNumber = InterlockedIncrement(&driverExtension->InterlockedCdRomCounter) - 1;
    sprintf(ntNameBuffer, "\\Device\\CdRom%d", deviceNumber);


    status = ClassCreateDeviceObject(DriverObject,
                                     ntNameBuffer,
                                     PhysicalDeviceObject,
                                     TRUE,
                                     &deviceObject);

    if (!NT_SUCCESS(status)) {
        TraceLog((CdromDebugWarning,
                    "CreateCdRomDeviceObjects: Can not create device %s\n",
                    ntNameBuffer));

        goto CreateCdRomDeviceObjectExit;
    }

    //
    // Indicate that IRPs should include MDLs.
    //

    SET_FLAG(deviceObject->Flags, DO_DIRECT_IO);

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
    // Initialize lock count to zero. The lock count is used to
    // disable the ejection mechanism when media is mounted.
    //

    fdoExtension->LockCount = 0;

    //
    // Save system cdrom number
    //

    fdoExtension->DeviceNumber = deviceNumber;

    //
    // Set the alignment requirements for the device based on the
    // host adapter requirements
    //

    if (lowerDevice->AlignmentRequirement > deviceObject->AlignmentRequirement) {
        deviceObject->AlignmentRequirement = lowerDevice->AlignmentRequirement;
    }

    //
    // Save the device descriptors
    //

    fdoExtension->AdapterDescriptor = NULL;

    fdoExtension->DeviceDescriptor = NULL;

    //
    // Clear the SrbFlags and disable synchronous transfers
    //

    fdoExtension->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Finally, attach to the PDO
    //

    fdoExtension->LowerPdo = PhysicalDeviceObject;

    fdoExtension->CommonExtension.LowerDeviceObject =
        IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

    if(fdoExtension->CommonExtension.LowerDeviceObject == NULL) {

        //
        // Uh - oh, we couldn't attach
        // cleanup and return
        //

        status = STATUS_UNSUCCESSFUL;
        goto CreateCdRomDeviceObjectExit;
    }

    //
    // CdRom uses an extra stack location for synchronizing it's start io
    // routine
    //

    deviceObject->StackSize++;

    //
    // cdData is used a few times below
    //

    cdData = fdoExtension->CommonExtension.DriverData;

    //
    // For NTMS to be able to easily determine drives-drv. letter matches.
    //

    status = CdRomCreateWellKnownName( deviceObject );

    if (!NT_SUCCESS(status)) {
        TraceLog((CdromDebugWarning,
                    "CdromCreateDeviceObjects: unable to create symbolic "
                    "link for device %wZ\n", &fdoExtension->CommonExtension.DeviceName));
        TraceLog((CdromDebugWarning,
                    "CdromCreateDeviceObjects: (non-fatal error)\n"));
    }

    ClassUpdateInformationInRegistry(deviceObject, "CdRom",
                                     fdoExtension->DeviceNumber, NULL, 0);

    //
    // from above IoGetAttachedDeviceReference
    //

    ObDereferenceObject(lowerDevice);

    //
    // need to init timerlist here in case a remove occurs
    // without a start, since we check the list is empty on remove.
    //

    cdData->DelayedRetryIrp = NULL;
    cdData->DelayedRetryInterval = 0;

    //
    // need this to be initialized for RPC Phase 1 drives (rpc0)
    //

    KeInitializeMutex(&cdData->Rpc0RegionMutex, 0);
    
    //
    // The device is initialized properly - mark it as such.
    //

    CLEAR_FLAG(deviceObject->Flags, DO_DEVICE_INITIALIZING);

    return(STATUS_SUCCESS);

CreateCdRomDeviceObjectExit:

    //
    // Release the device since an error occured.
    //

    // ClassClaimDevice(PortDeviceObject,
    //                      LunInfo,
    //                      TRUE,
    //                      NULL);

    //
    // from above IoGetAttachedDeviceReference
    //

    ObDereferenceObject(lowerDevice);

    if (deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }

    return status;

} // end CreateCdRomDeviceObject()

NTSTATUS
NTAPI
CdRomInitDevice(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine will complete the cd-rom initialization.  This includes
    allocating sense info buffers and srb s-lists, reading drive capacity
    and setting up Media Change Notification (autorun).

    This routine will not clean up allocate resources if it fails - that
    is left for device stop/removal

Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
#if 0
    PCLASS_DRIVER_EXTENSION driverExtension = ClassGetDriverExtension(
                                                Fdo->DriverObject);
#endif

    PVOID senseData = NULL;

    ULONG timeOut;
    PCDROM_DATA cddata = NULL;

    //BOOLEAN changerDevice;
    BOOLEAN isMmcDevice = FALSE;

    ULONG bps;
    ULONG lastBit;


    NTSTATUS status;

    PAGED_CODE();

    //
    // Build the lookaside list for srb's for the physical disk.  Should only
    // need a couple.
    //

    ClassInitializeSrbLookasideList(&(fdoExtension->CommonExtension),
                                    CDROM_SRB_LIST_SIZE);

    //
    // Allocate request sense buffer.
    //

    senseData = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                      SENSE_BUFFER_SIZE,
                                      CDROM_TAG_SENSE_INFO);

    if (senseData == NULL) {

        //
        // The buffer cannot be allocated.
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CdRomInitDeviceExit;
    }

    //
    // Set the sense data pointer in the device extension.
    //

    fdoExtension->SenseData = senseData;

    //
    // CDROMs are not partitionable so starting offset is 0.
    //

    commonExtension->StartingOffset.LowPart = 0;
    commonExtension->StartingOffset.HighPart = 0;

    //
    // Set timeout value in seconds.
    //

    timeOut = ClassQueryTimeOutRegistryValue(Fdo);
    if (timeOut) {
        fdoExtension->TimeOutValue = timeOut;
    } else {
        fdoExtension->TimeOutValue = SCSI_CDROM_TIMEOUT;
    }

    cddata = (PCDROM_DATA)(commonExtension->DriverData);

    //
    // Set up media change support defaults.
    //

    KeInitializeSpinLock(&cddata->DelayedRetrySpinLock);

    cddata->DelayedRetryIrp = NULL;
    cddata->DelayedRetryInterval = 0;
    cddata->Mmc.WriteAllowed = FALSE;

    //
    // Scan for  controllers that require special processing.
    //

    ScanForSpecial(Fdo);

    //
    // Determine if the drive is MMC-Capable
    //

    CdRomIsDeviceMmcDevice(Fdo, &isMmcDevice);

    if (!isMmcDevice) {

        SET_FLAG(Fdo->Characteristics, FILE_READ_ONLY_DEVICE);
    
    } else {
        
        //
        // the drive supports at least a subset of MMC commands
        // (and therefore supports READ_CD, etc...)
        //

        cddata->Mmc.IsMmc = TRUE;
        
        //
        // allocate a buffer for all the capabilities and such
        //

        status = CdRomAllocateMmcResources(Fdo);
        if (!NT_SUCCESS(status)) {
            goto CdRomInitDeviceExit;
        }
        

#if 0
        //
        // determine all the various media types from the profiles feature
        //
        {
            PFEATURE_DATA_PROFILE_LIST profileHeader;
            ULONG mediaTypes = 0;
            ULONG i;

            KdPrintEx((DPFLTR_CDROM_ID, CdromDebugError,
                       "Checking all profiles for media types supported.\n"
                       ));

            profileHeader = CdRomFindFeaturePage(cddata->Mmc.CapabilitiesBuffer,
                                                 cddata->Mmc.CapabilitiesBufferSize,
                                                 FeatureProfileList);
            if (profileHeader == NULL) {

                //
                // if profiles don't exist, there is something seriously
                // wrong with this command -- it's either not a cdrom or
                // one that hasn't implemented the spec correctly.  exit
                // now while we have the chance to do so safely.
                //
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugError,
                           "CdromDevice supports GET_CONFIGURATION, but "
                           "doesn't provide profiles for PDO %p!\n",
                           fdoExtension->LowerPdo));
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
                goto CdRomInitDeviceExit;

            }

            for (i = 0; i < MAX_CDROM_MEDIA_TYPES; i++) {
                
                BOOLEAN profileFound;
                CdRomFindProfileInProfiles(profileHeader,
                                           MediaProfileMatch[i].Profile,
                                           &profileFound);
                if (profileFound) {
                    
                    KdPrintEx((DPFLTR_CDROM_ID, CdromDebugError,
                               "CdromInit -> Found Profile %x => Media %x "
                               "(%x total)\n",
                               MediaProfileMatch[i].Profile,
                               MediaProfileMatch[i].Media,
                               mediaTypes + 1
                               ));
                    
                    cddata->Mmc.MediaProfileMatches[mediaTypes] =
                        MediaProfileMatch[i];
                    mediaTypes++;
                
                }

            }

            if (mediaTypes == 0) {

                //
                // if profiles don't exist, there is something seriously
                // wrong with this command -- it's either not a cdrom or
                // one that hasn't implemented the spec correctly.  exit
                // now while we have the chance to do so safely.
                //
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugError,
                           "CdromDevice supports GET_CONFIGURATION, but "
                           "doesn't support any of the standard profiles "
                           "for PDO %p!\n", fdoExtension->LowerPdo));
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
                goto CdRomInitDeviceExit;
            
            }

            cddata->Mmc.MediaTypes = mediaTypes;


        }
#endif // media checks, and all failure paths due to bad firmware.

        //
        // if the drive supports target defect management and sector-addressable
        // writes, then we should allow writes to the media.
        //

        if (CdRomFindFeaturePage(cddata->Mmc.CapabilitiesBuffer,
                                 cddata->Mmc.CapabilitiesBufferSize,
                                 FeatureDefectManagement) &&
            CdRomFindFeaturePage(cddata->Mmc.CapabilitiesBuffer,
                                 cddata->Mmc.CapabilitiesBufferSize,
                                 FeatureRandomWritable)) {

            //
            // the drive is target defect managed, and supports random writes
            // on sector-aligment.  allow writes to occur by setting the error
            // handler to point to a private media change detection handler.
            //

            KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                       "Found a WRITE capable device: %p\n", Fdo));
            
            //
            // the write specific pages have been found -- 
            // set the error handler and set it to require an update!
            //

            cddata->Mmc.UpdateState = CdromMmcUpdateRequired;
            cddata->ErrorHandler = CdRomMmcErrorHandler;

        }

        //
        // ISSUE-2000/4/4-henrygab - mmc-compliant compliant drives should
        //                           be initialized based upon reported
        //                           capabilities, such as CSS, Analogue Audio,
        //                           READ_CD capabilities, and (possibly) even
        //                           drive capacity information.
        //
        
        TraceLog((CdromDebugWarning,
                  "Defaulting to READ_CD because device %p is MMC compliant\n",
                  Fdo));
        SET_FLAG(fdoExtension->DeviceFlags, DEV_SAFE_START_UNIT);
        SET_FLAG(cddata->XAFlags, XA_USE_READ_CD);

    }


    //
    // Set the default geometry for the cdrom to match what NT 4 used.
    // Classpnp will use these values to compute the cylinder count rather
    // than using it's NT 5.0 defaults.
    //

    fdoExtension->DiskGeometry.TracksPerCylinder = 0x40;
    fdoExtension->DiskGeometry.SectorsPerTrack = 0x20;

    //
    // Do READ CAPACITY. This SCSI command returns the last sector address
    // on the device and the bytes per sector. These are used to calculate
    // the drive capacity in bytes.
    //
    // NOTE: This should be change to send the Srb synchronously, then
    // call CdRomInterpretReadCapacity() to properly setup the defaults.
    //

    status = ClassReadDriveCapacity(Fdo);

    bps = fdoExtension->DiskGeometry.BytesPerSector;

    if (!NT_SUCCESS(status) || !bps) {

        TraceLog((CdromDebugWarning,
                    "CdRomStartDevice: Can't read capacity for device %wZ\n",
                    &(fdoExtension->CommonExtension.DeviceName)));

        //
        // Set disk geometry to default values (per ISO 9660).
        //

        bps = 2048;
        fdoExtension->SectorShift = 11;
        commonExtension->PartitionLength.QuadPart = (LONGLONG)(0x7fffffff);

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
    fdoExtension->DiskGeometry.BytesPerSector = bps;
    TraceLog((CdromDebugTrace, "CdRomInitDevice: Calc'd bps = %x\n", bps));


    ClassInitializeMediaChangeDetection(fdoExtension, "CdRom");


    //
    // test for audio read capabilities
    //

    TraceLog((CdromDebugWarning,
              "Detecting XA_READ capabilities\n"));

    if (CdRomGetDeviceType(Fdo) == FILE_DEVICE_DVD) {

        TraceLog((CdromDebugWarning,
                    "CdRomInitDevice: DVD Devices require START_UNIT\n"));


        //
        // all DVD devices must support the READ_CD command
        //

        TraceLog((CdromDebugWarning,
                    "CdRomDetermineRawReadCapabilities: DVD devices "
                    "support READ_CD command for FDO %p\n", Fdo));
        SET_FLAG(fdoExtension->DeviceFlags, DEV_SAFE_START_UNIT);
        SET_FLAG(cddata->XAFlags, XA_USE_READ_CD);


        status = STATUS_SUCCESS;

    } else if ((fdoExtension->DeviceDescriptor->BusType != BusTypeScsi)  &&
               (fdoExtension->DeviceDescriptor->BusType != BusTypeAta)   &&
               (fdoExtension->DeviceDescriptor->BusType != BusTypeAtapi) &&
               (fdoExtension->DeviceDescriptor->BusType != BusTypeUnknown)
               ) {

        //
        // devices on the newer busses must support READ_CD command
        //

        TraceLog((CdromDebugWarning,
                  "CdRomDetermineRawReadCapabilities: Devices for newer "
                  "busses must support READ_CD command for FDO %p, Bus %x\n",
                  Fdo, fdoExtension->DeviceDescriptor->BusType));
        SET_FLAG(fdoExtension->DeviceFlags, DEV_SAFE_START_UNIT);
        SET_FLAG(cddata->XAFlags, XA_USE_READ_CD);

    }

    //
    // now clear all our READ_CD flags if the drive should have supported
    // it, but we are not sure it actually does.  we still won't query
    // the drive more than one time if it supports the command.
    //

    if (TEST_FLAG(cddata->HackFlags, CDROM_HACK_FORCE_READ_CD_DETECTION)) {

        TraceLog((CdromDebugWarning,
                  "Forcing detection of READ_CD for FDO %p because "
                  "testing showed some firmware did not properly support it\n",
                  Fdo));
        CLEAR_FLAG(cddata->XAFlags, XA_USE_READ_CD);

    }


    //
    // read our READ_CD support in the registry if it was seeded.
    //
    {
        ULONG readCdSupported = 0;
        
        ClassGetDeviceParameter(fdoExtension,
                                CDROM_SUBKEY_NAME,
                                CDROM_READ_CD_NAME,
                                &readCdSupported
                                );
        
        if (readCdSupported != 0) {
            
            TraceLog((CdromDebugWarning,
                      "Defaulting to READ_CD because previously detected "
                      "that the device supports it for Fdo %p.\n",
                      Fdo
                      ));
            SET_FLAG(cddata->XAFlags, XA_USE_READ_CD);

        }

    }


    //
    // backwards-compatible hackish attempt to determine if the drive
    // supports any method of reading digital audio from the disc.
    //

    if (!TEST_FLAG(cddata->XAFlags, XA_USE_READ_CD)) {

        SCSI_REQUEST_BLOCK srb;
        PCDB cdb;
        ULONG length;
        PUCHAR buffer = NULL;
        ULONG count;

        //
        // ISSUE-2000/07/05-henrygab - use the mode page to determine
        //          READ_CD support, then fall back on the below
        //          (unreliable?) hack.
        //

        //
        // Build the MODE SENSE CDB. The data returned will be kept in the
        // device extension and used to set block size.
        //

        length = max(sizeof(ERROR_RECOVERY_DATA),sizeof(ERROR_RECOVERY_DATA10));

        buffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                       length,
                                       CDROM_TAG_MODE_DATA);

        if (!buffer) {
            TraceLog((CdromDebugWarning,
                        "CdRomDetermineRawReadCapabilities: cannot allocate "
                        "buffer, so leaving for FDO %p\n", Fdo));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto CdRomInitDeviceExit;
        }

        for (count = 0; count < 2; count++) {

            if (count == 0) {
                length = sizeof(ERROR_RECOVERY_DATA);
            } else {
                length = sizeof(ERROR_RECOVERY_DATA10);
            }

            RtlZeroMemory(buffer, length);
            RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));
            cdb = (PCDB)srb.Cdb;

            srb.TimeOutValue = fdoExtension->TimeOutValue;

            if (count == 0) {
                srb.CdbLength = 6;
                cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
                cdb->MODE_SENSE.PageCode = 0x1;
                // note: not setting DBD in order to get the block descriptor!
                cdb->MODE_SENSE.AllocationLength = (UCHAR)length;
            } else {
                srb.CdbLength = 10;
                cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
                cdb->MODE_SENSE10.PageCode = 0x1;
                // note: not setting DBD in order to get the block descriptor!
                cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(length >> 8);
                cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(length & 0xFF);
            }

            status = ClassSendSrbSynchronous(Fdo,
                                             &srb,
                                             buffer,
                                             length,
                                             FALSE);


            if (NT_SUCCESS(status) || (status == STATUS_DATA_OVERRUN)) {

                //
                // STATUS_DATA_OVERRUN means it's a newer drive with more info
                // to tell us, so it's probably able to support READ_CD
                //

                RtlZeroMemory(cdb, CDB12GENERIC_LENGTH);

                srb.CdbLength = 12;
                cdb->READ_CD.OperationCode = SCSIOP_READ_CD;

                status = ClassSendSrbSynchronous(Fdo,
                                                 &srb,
                                                 NULL,
                                                 0,
                                                 FALSE);

                if (NT_SUCCESS(status) ||
                    (status == STATUS_NO_MEDIA_IN_DEVICE) ||
                    (status == STATUS_NONEXISTENT_SECTOR) ||
                    (status == STATUS_UNRECOGNIZED_MEDIA)
                    ) {

                    //
                    // READ_CD works
                    //

                    TraceLog((CdromDebugWarning,
                              "CdRomDetermineRawReadCapabilities: Using "
                              "READ_CD for FDO %p due to status %x\n",
                              Fdo,
                              status));
                    SET_FLAG(cddata->XAFlags, XA_USE_READ_CD);
                    
                    //
                    // ignore errors in saving this info
                    //
                    
                    ClassSetDeviceParameter(fdoExtension,
                                            CDROM_SUBKEY_NAME,
                                            CDROM_READ_CD_NAME,
                                            1
                                            );
                                            

                    break; // out of the for loop

                }

                TraceLog((CdromDebugWarning,
                            "CdRomDetermineRawReadCapabilities: Using "
                            "%s-byte mode switching for FDO %p due to status "
                            "%x returned for READ_CD\n",
                            ((count == 0) ? "6" : "10"), Fdo, status));

                if (count == 0) {
                    SET_FLAG(cddata->XAFlags, XA_USE_6_BYTE);
                    RtlCopyMemory(&cddata->Header,
                                  buffer,
                                  sizeof(ERROR_RECOVERY_DATA));
                    cddata->Header.ModeDataLength = 0;
                } else {
                    SET_FLAG(cddata->XAFlags, XA_USE_10_BYTE);
                    RtlCopyMemory(&cddata->Header10,
                                  buffer,
                                  sizeof(ERROR_RECOVERY_DATA10));
                    cddata->Header10.ModeDataLength[0] = 0;
                    cddata->Header10.ModeDataLength[1] = 0;
                }
                break;  // out of for loop

            }
            TraceLog((CdromDebugWarning,
                      "FDO %p failed %x byte mode sense, status %x\n",
                      Fdo,
                      ((count == 0) ? 6 : 10),
                      status
                      ));

            //
            // mode sense failed
            //

        } // end of for loop to try 6 and 10-byte mode sense

        if (count == 2) {

            //
            // nothing worked.  we probably cannot support digital
            // audio extraction from this drive
            //

            TraceLog((CdromDebugWarning,
                        "CdRomDetermineRawReadCapabilities: FDO %p "
                        "cannot support READ_CD\n", Fdo));
            CLEAR_FLAG(cddata->XAFlags, XA_PLEXTOR_CDDA);
            CLEAR_FLAG(cddata->XAFlags, XA_NEC_CDDA);
            SET_FLAG(cddata->XAFlags, XA_NOT_SUPPORTED);

        } // end of count == 2

        //
        // free our resources
        //

        ExFreePool(buffer);

        //
        // set a successful status
        // (in case someone later checks this)
        //

        status = STATUS_SUCCESS;

    }

    //
    // Register interfaces for this device.
    //

    {
        UNICODE_STRING interfaceName;

        RtlInitUnicodeString(&interfaceName, NULL);

        status = IoRegisterDeviceInterface(fdoExtension->LowerPdo,
                                           (LPGUID) &CdRomClassGuid,
                                           NULL,
                                           &interfaceName);

        if(NT_SUCCESS(status)) {

            cddata->CdromInterfaceString = interfaceName;

            status = IoSetDeviceInterfaceState(
                        &interfaceName,
                        TRUE);

            if(!NT_SUCCESS(status)) {

                TraceLog((CdromDebugWarning,
                            "CdromInitDevice: Unable to register cdrom "
                            "DCA for fdo %p [%lx]\n",
                            Fdo, status));
            }
        }
    }

    return(STATUS_SUCCESS);

CdRomInitDeviceExit:

    CdRomDeAllocateMmcResources(Fdo);    
    RtlZeroMemory(&(cddata->Mmc), sizeof(CDROM_MMC_EXTENSION));
    
    return status;

}

NTSTATUS
NTAPI
CdRomStartDevice(
    IN PDEVICE_OBJECT Fdo
    )
/*++

Routine Description:

    This routine starts the timer for the cdrom

Arguments:

    Fdo - a pointer to the functional device object for this device

Return Value:

    status

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata = (PCDROM_DATA)(commonExtension->DriverData);
    PDVD_COPY_PROTECT_KEY copyProtectKey;
    PDVD_RPC_KEY rpcKey;
    IO_STATUS_BLOCK ioStatus;
    ULONG bufferLen;

    // CdRomCreateWellKnownName(Fdo);

    //
    // if we have a DVD-ROM
    //    if we have a rpc0 device
    //        fake a rpc2 device
    //    if device does not have a dvd region set
    //        select a dvd region for the user
    //
    
    cddata->DvdRpc0Device = FALSE;

    //
    // since StartIo() will call IoStartNextPacket() on error, allowing
    // StartIo() to be non-recursive prevents stack overflow bugchecks in
    // severe error cases (such as fault-injection in the verifier).
    //
    // the only difference is that the thread context may be different
    // in StartIo() than in the caller of IoStartNextPacket().
    //

    IoSetStartIoAttributes(Fdo, TRUE, TRUE);

    //
    // check to see if we have a DVD device
    //

    if (CdRomGetDeviceType(Fdo) != FILE_DEVICE_DVD) {
        return STATUS_SUCCESS;
    }

    //
    // we got a DVD drive.
    // now, figure out if we have a RPC0 device
    //

    bufferLen = DVD_RPC_KEY_LENGTH;
    copyProtectKey =
        (PDVD_COPY_PROTECT_KEY)ExAllocatePoolWithTag(PagedPool,
                                                     bufferLen,
                                                     DVD_TAG_RPC2_CHECK);

    if (copyProtectKey == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get the device region
    //
    RtlZeroMemory (copyProtectKey, bufferLen);
    copyProtectKey->KeyLength = DVD_RPC_KEY_LENGTH;
    copyProtectKey->KeyType = DvdGetRpcKey;

    //
    // Build a request for READ_KEY
    //
    ClassSendDeviceIoControlSynchronous(
        IOCTL_DVD_READ_KEY,
        Fdo,
        copyProtectKey,
        DVD_RPC_KEY_LENGTH,
        DVD_RPC_KEY_LENGTH,
        FALSE,
        &ioStatus
        );

    if (!NT_SUCCESS(ioStatus.Status)) {

        //
        // we have a rpc0 device
        //
        // NOTE: THIS MODIFIES THE BEHAVIOR OF THE IOCTL
        //

        cddata->DvdRpc0Device = TRUE;

        TraceLog((CdromDebugWarning,
                    "CdromStartDevice (%p): RPC Phase 1 drive detected\n",
                    Fdo));

        //
        // note: we could force this chosen now, but it's better to reduce
        // the number of code paths that could be taken.  always delay to
        // increase the percentage code coverage.
        //

        TraceLog((CdromDebugWarning,
                  "CdromStartDevice (%p): Delay DVD Region Selection\n",
                  Fdo));

        cddata->Rpc0SystemRegion           = 0xff;
        cddata->Rpc0SystemRegionResetCount = DVD_MAX_REGION_RESET_COUNT;
        cddata->PickDvdRegion              = 1;
        cddata->Rpc0RetryRegistryCallback  = 1;
        ExFreePool(copyProtectKey);
        return STATUS_SUCCESS;

    } else {

        rpcKey = (PDVD_RPC_KEY) copyProtectKey->KeyData;

        //
        // TypeCode of zero means that no region has been set.
        //

        if (rpcKey->TypeCode == 0) {
            TraceLog((CdromDebugWarning,
                        "CdromStartDevice (%p): must choose DVD region\n",
                        Fdo));
            cddata->PickDvdRegion = 1;
            CdRomPickDvdRegion(Fdo);
        }
    }

    ExFreePool (copyProtectKey);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CdRomStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )
{
    return STATUS_SUCCESS;
}

VOID
NTAPI
CdRomStartIo(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )
{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;

    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION  nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PIO_STACK_LOCATION  irpStack;

    PIRP                irp2 = NULL;

    ULONG               transferPages;
    ULONG               transferByteCount = currentIrpStack->Parameters.Read.Length;
    //LARGE_INTEGER       startingOffset = currentIrpStack->Parameters.Read.ByteOffset;
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

    cdData = (PCDROM_DATA)(fdoExtension->CommonExtension.DriverData);
    use6Byte = TEST_FLAG(cdData->XAFlags, XA_USE_6_BYTE);

    //
    // if this test is true, then we will exit the routine within this
    // code block, queueing the irp for later completion.
    //

    if ((cdData->Mmc.IsMmc) &&
        (cdData->Mmc.UpdateState != CdromMmcUpdateComplete)
        ) {

        USHORT queueDepth;
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdRomStartIo: [%p] Device needs to update capabilities\n",
                   Irp));
        ASSERT(cdData->Mmc.IsMmc);
        ASSERT(cdData->Mmc.CapabilitiesIrp != NULL);
        ASSERT(cdData->Mmc.CapabilitiesIrp != Irp);

        //
        // NOTE - REF #0002
        //
        // the state was either UpdateRequired (which means we will
        // have to start the work item) or UpdateStarted (which means
        // we have already started the work item at least once -- may
        // transparently change to UpdateComplete).
        //
        // if it's update required, we just queue it, change to UpdateStarted,
        // start the workitem, and start the next packet.
        //
        // else, we must queue the item and check the queue depth.  if the
        // queue depth is equal to 1, that means the worker item from the
        // previous attempt has already de-queued the items, so we should
        // call this routine again (retry) as an optimization rather than
        // re-add it this irp to the queue.  since this is tail recursion,
        // it won't take much/any stack to do this.
        //
        // NOTE: This presumes the following items are true:
        //
        // we only add to the list from CdRomStartIo(), which is serialized.
        // we only set to UpdateStarted from CdRomStartIo(), and only if
        //    the state was UpdateRequired.
        // we only set to UpdateRequired from CdRomMmcErrorHandler(), and
        //    only if the state was UpdateComplete.
        // we only set to UpdateComplete from the workitem, and assert the
        //    state was UpdateStarted.
        // we flush the entire queue in one atomic operation in the workitem,
        //    except in the special case described above when we dequeue
        //    the request immediately.
        //
        // order of operations is vitally important: queue, then test the depth
        // this will prevent lost irps.
        //

        ExInterlockedPushEntrySList(&(cdData->Mmc.DelayedIrps),
                                    (PSINGLE_LIST_ENTRY)&(Irp->Tail.Overlay.DriverContext[0]),
                                    &(cdData->Mmc.DelayedLock));
        
        queueDepth = ExQueryDepthSList(&(cdData->Mmc.DelayedIrps));
        if (queueDepth == 1) {

            if (cdData->Mmc.UpdateState == CdromMmcUpdateRequired) {
                LONG oldState;
                
                //
                // should free any old partition list info that
                // we've previously saved away and then start the WorkItem
                //

                oldState = InterlockedExchange(&cdData->Mmc.UpdateState,
                                               CdromMmcUpdateStarted);
                ASSERT(oldState == CdromMmcUpdateRequired);

                IoQueueWorkItem(cdData->Mmc.CapabilitiesWorkItem,
                                CdRomUpdateMmcDriveCapabilities,
                                DelayedWorkQueue,
                                NULL);

            } else {
                
                //
                // they *just* finished updating, so we should flush the list
                // back onto the StartIo queue and start the next packet.
                //

                CdRompFlushDelayedList(Fdo, &(cdData->Mmc), STATUS_SUCCESS, FALSE);

            }

        }
            
        //
        // start the next packet so we don't deadlock....
        //

        IoStartNextPacket(Fdo, FALSE);
        return;
    
    }

    //
    // If the flag is set in the device object
    // force a verify for READ, WRITE and RAW_READ requests
    // Note that ioctls are passed through....
    //

    if (TEST_FLAG(Fdo->Flags, DO_VERIFY_VOLUME) &&
        IS_READ_WRITE_REQUEST(currentIrpStack)) {
        
        TraceLog((CdromDebugTrace,
                    "CdRomStartIo: [%p] Volume needs verified\n", Irp));
        
        if (!(currentIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME)) {

            if (Irp->Tail.Overlay.Thread) {
                IoSetHardErrorOrVerifyDevice(Irp, Fdo);
            }

            Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;

            TraceLog((CdromDebugTrace,
                        "CdRomStartIo: [%p] Calling UpdateCapcity - "
                        "ioctl event = %p\n",
                        Irp,
                        nextIrpStack->Parameters.Others.Argument1
                      ));

            //
            // our device control dispatch routine stores an event in the next
            // stack location to signal when startio has completed.  We need to
            // pass this in so that the update capacity completion routine can
            // set it rather than completing the Irp.
            //

            status = CdRomUpdateCapacity(fdoExtension,
                                         Irp,
                                         nextIrpStack->Parameters.Others.Argument1
                                         );

            TraceLog((CdromDebugTrace,
                        "CdRomStartIo: [%p] UpdateCapacity returned %lx\n",
                        Irp, status));
            return;
        }
    }

    //
    // fail writes if they are not allowed...
    //

    if ((currentIrpStack->MajorFunction == IRP_MJ_WRITE) &&
        !(cdData->Mmc.WriteAllowed)) {

        TraceLog((CdromDebugError,
                    "CdRomStartIo: [%p] Device %p failing write request\n",
                    Irp, Fdo));

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        BAIL_OUT(Irp);
        CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
        return;
    }

    if (currentIrpStack->MajorFunction == IRP_MJ_READ ||
        currentIrpStack->MajorFunction == IRP_MJ_WRITE ) {

        ULONG maximumTransferLength = fdoExtension->AdapterDescriptor->MaximumTransferLength;

        //
        // Add partition byte offset to make starting byte relative to
        // beginning of disk.
        //

        currentIrpStack->Parameters.Read.ByteOffset.QuadPart +=
            (fdoExtension->CommonExtension.StartingOffset.QuadPart);

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

            //
            // a writable device must be MMC compliant, which supports
            // READ_CD commands.
            //

            ASSERT(currentIrpStack->MajorFunction != IRP_MJ_WRITE);

            ASSERT(!TEST_FLAG(cdData->XAFlags, XA_USE_READ_CD));

            //
            // Fire off a mode select to switch back to cooked sectors.
            //

            irp2 = IoAllocateIrp((CCHAR)(Fdo->StackSize+1), FALSE);

            if (!irp2) {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            srb = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(SCSI_REQUEST_BLOCK),
                                        CDROM_TAG_SRB);
            if (!srb) {
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

            cdb = (PCDB)srb->Cdb;

            //
            // Allocate sense buffer.
            //

            senseBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                                SENSE_BUFFER_SIZE,
                                                CDROM_TAG_SENSE_INFO);

            if (!senseBuffer) {
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
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
            irpStack->DeviceObject = Fdo;
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
            srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            srb->SrbStatus = srb->ScsiStatus = 0;
            srb->NextSrb = 0;
            srb->OriginalRequest = irp2;
            srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
            srb->SenseInfoBuffer = senseBuffer;

            transferByteCount = (use6Byte) ? sizeof(ERROR_RECOVERY_DATA) : sizeof(ERROR_RECOVERY_DATA10);

            dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                               transferByteCount,
                                               CDROM_TAG_RAW);

            if (!dataBuffer) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            transferByteCount,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
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

            if (use6Byte) {
                cdData->BlockDescriptor.BlockLength[0] = (UCHAR)(COOKED_SECTOR_SIZE >> 16) & 0xFF;
                cdData->BlockDescriptor.BlockLength[1] = (UCHAR)(COOKED_SECTOR_SIZE >>  8) & 0xFF;
                cdData->BlockDescriptor.BlockLength[2] = (UCHAR)(COOKED_SECTOR_SIZE & 0xFF);
            } else {
                cdData->BlockDescriptor10.BlockLength[0] = (UCHAR)(COOKED_SECTOR_SIZE >> 16) & 0xFF;
                cdData->BlockDescriptor10.BlockLength[1] = (UCHAR)(COOKED_SECTOR_SIZE >>  8) & 0xFF;
                cdData->BlockDescriptor10.BlockLength[2] = (UCHAR)(COOKED_SECTOR_SIZE & 0xFF);
            }

            //
            // Move error page into dataBuffer.
            //

            RtlCopyMemory(srb->DataBuffer, &cdData->Header, transferByteCount);

            //
            // Build and send a mode select to switch into raw mode.
            //

            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_OUT);
            srb->DataTransferLength = transferByteCount;
            srb->TimeOutValue = fdoExtension->TimeOutValue * 2;

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

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }


        //
        // Request needs to be split. Completion of each portion of the
        // request will fire off the next portion. The final request will
        // signal Io to send a new request.
        //

        transferPages =
            fdoExtension->AdapterDescriptor->MaximumPhysicalPages - 1;

        if(maximumTransferLength > (transferPages << PAGE_SHIFT)) {
            maximumTransferLength = transferPages << PAGE_SHIFT;
        }

        //
        // Check that the maximum transfer size is not zero
        //

        if(maximumTransferLength == 0) {
            maximumTransferLength = PAGE_SIZE;
        }

        ClassSplitRequest(Fdo, Irp, maximumTransferLength);
        return;

    } else if (currentIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {

        //
        // Allocate an irp, srb and associated structures.
        //

        irp2 = IoAllocateIrp((CCHAR)(Fdo->StackSize+1),
                              FALSE);

        if (!irp2) {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

            BAIL_OUT(Irp);
            CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
            return;
        }

        srb = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(SCSI_REQUEST_BLOCK),
                                    CDROM_TAG_SRB);
        if (!srb) {
            IoFreeIrp(irp2);
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

            BAIL_OUT(Irp);
            CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
            return;
        }

        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        cdb = (PCDB)srb->Cdb;

        //
        // Allocate sense buffer.
        //

        senseBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                            SENSE_BUFFER_SIZE,
                                            CDROM_TAG_SENSE_INFO);

        if (!senseBuffer) {
            ExFreePool(srb);
            IoFreeIrp(irp2);
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

            BAIL_OUT(Irp);
            CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
            return;
        }

        RtlZeroMemory(senseBuffer, SENSE_BUFFER_SIZE);

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
        irpStack->DeviceObject = Fdo;
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
        // keep track of the new irp as Argument3
        //

        nextIrpStack->Parameters.Others.Argument3 = irp2;


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
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
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

            if (!TEST_FLAG(cdData->XAFlags, XA_USE_READ_CD)) {

                PRAW_READ_INFO rawReadInfo =
                                   (PRAW_READ_INFO)currentIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
                ULONG          maximumTransferLength;
                ULONG          transferPages;

                if (cdData->RawAccess) {

                    ULONG  startingSector;
                    //UCHAR  min, sec, frame;

                    //
                    // Free the recently allocated irp, as we don't need it.
                    //

                    IoFreeIrp(irp2);

                    cdb = (PCDB)srb->Cdb;
                    RtlZeroMemory(cdb, CDB12GENERIC_LENGTH);

                    //
                    // Calculate starting offset.
                    //

                    startingSector = (ULONG)(rawReadInfo->DiskOffset.QuadPart >> fdoExtension->SectorShift);
                    transferByteCount  = rawReadInfo->SectorCount * RAW_SECTOR_SIZE;
                    maximumTransferLength = fdoExtension->AdapterDescriptor->MaximumTransferLength;
                    transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Irp->MdlAddress),
                                                                   transferByteCount);

                    //
                    // Determine if request is within limits imposed by miniport.
                    //
                    if (transferByteCount > maximumTransferLength ||
                        transferPages > fdoExtension->AdapterDescriptor->MaximumPhysicalPages) {

                        //
                        // The claim is that this won't happen, and is backed up by
                        // ActiveMovie usage, which does unbuffered XA reads of 0x18000, yet
                        // we get only 4 sector requests.
                        //

                        ExFreePool(senseBuffer);
                        ExFreePool(srb);

                        Irp->IoStatus.Information = 0;
                        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                        BAIL_OUT(Irp);
                        CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                        return;

                    }

                    srb->OriginalRequest = Irp;
                    srb->SrbFlags = fdoExtension->SrbFlags;
                    SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
                    SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
                    srb->DataTransferLength = transferByteCount;
                    srb->TimeOutValue = fdoExtension->TimeOutValue;
                    srb->CdbLength = 10;
                    srb->DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

                    if (rawReadInfo->TrackMode == CDDA) {
                        if (TEST_FLAG(cdData->XAFlags, XA_PLEXTOR_CDDA)) {

                            srb->CdbLength = 12;

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

                        } else if (TEST_FLAG(cdData->XAFlags, XA_NEC_CDDA)) {

                            cdb->NEC_READ_CDDA.LogicalBlockByte3  = (UCHAR) (startingSector & 0xFF);
                            cdb->NEC_READ_CDDA.LogicalBlockByte2  = (UCHAR) ((startingSector >>  8) & 0xFF);
                            cdb->NEC_READ_CDDA.LogicalBlockByte1  = (UCHAR) ((startingSector >> 16) & 0xFF);
                            cdb->NEC_READ_CDDA.LogicalBlockByte0  = (UCHAR) ((startingSector >> 24) & 0xFF);

                            cdb->NEC_READ_CDDA.TransferBlockByte1 = (UCHAR) (rawReadInfo->SectorCount & 0xFF);
                            cdb->NEC_READ_CDDA.TransferBlockByte0 = (UCHAR) (rawReadInfo->SectorCount >> 8);

                            cdb->NEC_READ_CDDA.OperationCode = 0xD4;
                        }
                    } else {

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

                    // HACKHACK - REF #0001

                    //
                    // Set up IoCompletion routine address.
                    //

                    IoSetCompletionRoutine(Irp,
                                           CdRomXACompletion,
                                           srb,
                                           TRUE,
                                           TRUE,
                                           TRUE);

                    IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, Irp);
                    return;

                } else {

                    transferByteCount = (use6Byte) ? sizeof(ERROR_RECOVERY_DATA) : sizeof(ERROR_RECOVERY_DATA10);
                    dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                                       transferByteCount,
                                                       CDROM_TAG_RAW );
                    if (!dataBuffer) {
                        ExFreePool(senseBuffer);
                        ExFreePool(srb);
                        IoFreeIrp(irp2);
                        Irp->IoStatus.Information = 0;
                        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                        BAIL_OUT(Irp);
                        CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                        return;

                    }

                    irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                                    transferByteCount,
                                                    FALSE,
                                                    FALSE,
                                                    (PIRP) NULL);

                    if (!irp2->MdlAddress) {
                        ExFreePool(senseBuffer);
                        ExFreePool(srb);
                        ExFreePool(dataBuffer);
                        IoFreeIrp(irp2);
                        Irp->IoStatus.Information = 0;
                        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                        BAIL_OUT(Irp);
                        CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                        return;
                    }

                    //
                    // Prepare the MDL
                    //

                    MmBuildMdlForNonPagedPool(irp2->MdlAddress);

                    srb->DataBuffer = dataBuffer;

                    //
                    // Set the new block size in the descriptor.
                    // This will set the block read size to RAW_SECTOR_SIZE
                    // TODO: Set density code, based on operation
                    //

                    if (use6Byte) {
                        cdData->BlockDescriptor.BlockLength[0] = (UCHAR)(RAW_SECTOR_SIZE >> 16) & 0xFF;
                        cdData->BlockDescriptor.BlockLength[1] = (UCHAR)(RAW_SECTOR_SIZE >>  8) & 0xFF;
                        cdData->BlockDescriptor.BlockLength[2] = (UCHAR)(RAW_SECTOR_SIZE & 0xFF);
                        cdData->BlockDescriptor.DensityCode = 0;
                    } else {
                        cdData->BlockDescriptor10.BlockLength[0] = (UCHAR)(RAW_SECTOR_SIZE >> 16) & 0xFF;
                        cdData->BlockDescriptor10.BlockLength[1] = (UCHAR)(RAW_SECTOR_SIZE >>  8) & 0xFF;
                        cdData->BlockDescriptor10.BlockLength[2] = (UCHAR)(RAW_SECTOR_SIZE & 0xFF);
                        cdData->BlockDescriptor10.DensityCode = 0;
                    }

                    //
                    // Move error page into dataBuffer.
                    //

                    RtlCopyMemory(srb->DataBuffer, &cdData->Header, transferByteCount);


                    //
                    // Build and send a mode select to switch into raw mode.
                    //

                    srb->SrbFlags = fdoExtension->SrbFlags;
                    SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
                    SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_OUT);
                    srb->DataTransferLength = transferByteCount;
                    srb->TimeOutValue = fdoExtension->TimeOutValue * 2;

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
                RtlZeroMemory(cdb, CDB12GENERIC_LENGTH);


                //
                // Calculate starting offset.
                //

                startingSector = (ULONG)(rawReadInfo->DiskOffset.QuadPart >> fdoExtension->SectorShift);
                transferByteCount  = rawReadInfo->SectorCount * RAW_SECTOR_SIZE;

                srb->OriginalRequest = Irp;
                srb->SrbFlags = fdoExtension->SrbFlags;
                SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
                SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
                srb->DataTransferLength = transferByteCount;
                srb->TimeOutValue = fdoExtension->TimeOutValue;
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
                    ExFreePool(senseBuffer);
                    ExFreePool(srb);
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                    BAIL_OUT(Irp);
                    CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                    return;
                }

                cdb->READ_CD.OperationCode = SCSIOP_READ_CD;

                nextIrpStack->MajorFunction = IRP_MJ_SCSI;
                nextIrpStack->Parameters.Scsi.Srb = srb;

                // HACKHACK - REF #0001

                //
                // Set up IoCompletion routine address.
                //

                IoSetCompletionRoutine(Irp,
                                       CdRomXACompletion,
                                       srb,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, Irp);
                return;

            }

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        //
        // the _EX version does the same thing on the front end
        //

        case IOCTL_DISK_GET_LENGTH_INFO:
        case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        case IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX:
        case IOCTL_CDROM_GET_DRIVE_GEOMETRY: {

            //
            // Issue ReadCapacity to update device extension
            // with information for current media.
            //

            TraceLog((CdromDebugError,
                        "CdRomStartIo: Get drive geometry/length "
                        "info (%p)\n", Irp));

            //
            // setup remaining srb and cdb parameters.
            //

            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);
            srb->CdbLength = 10;
            srb->TimeOutValue = fdoExtension->TimeOutValue;

            dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                               sizeof(READ_CAPACITY_DATA),
                                               CDROM_TAG_READ_CAP);
            if (!dataBuffer) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            sizeof(READ_CAPACITY_DATA),
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            //
            // Prepare the MDL
            //

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);

            srb->DataBuffer = dataBuffer;
            cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_GET_CONFIGURATION: {

            PGET_CONFIGURATION_IOCTL_INPUT inputBuffer;
            
            TraceLog((CdromDebugError,
                        "CdRomStartIo: Get configuration (%p)\n", Irp));

            if (!cdData->Mmc.IsMmc) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            transferByteCount = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            
            dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                               transferByteCount,
                                               CDROM_TAG_GET_CONFIG);
            if (!dataBuffer) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                             transferByteCount,
                                             FALSE,
                                             FALSE,
                                             (PIRP) NULL);
            if (!irp2->MdlAddress) {
                ExFreePool(dataBuffer);
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);
            
            //
            // setup remaining srb and cdb parameters
            //

            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = transferByteCount;
            srb->CdbLength = 10;
            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->DataBuffer = dataBuffer;
            
            cdb->GET_CONFIGURATION.OperationCode = SCSIOP_GET_CONFIGURATION;
            cdb->GET_CONFIGURATION.AllocationLength[0] = (UCHAR)(transferByteCount >> 8);
            cdb->GET_CONFIGURATION.AllocationLength[1] = (UCHAR)(transferByteCount & 0xff);

            inputBuffer = (PGET_CONFIGURATION_IOCTL_INPUT)Irp->AssociatedIrp.SystemBuffer;
            cdb->GET_CONFIGURATION.StartingFeature[0] = (UCHAR)(inputBuffer->Feature >> 8);
            cdb->GET_CONFIGURATION.StartingFeature[1] = (UCHAR)(inputBuffer->Feature & 0xff);
            cdb->GET_CONFIGURATION.RequestType        = (UCHAR)(inputBuffer->RequestType);
            
            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        case IOCTL_DISK_VERIFY: {
            
            PVERIFY_INFORMATION verifyInfo = Irp->AssociatedIrp.SystemBuffer;
            LARGE_INTEGER byteOffset;
            ULONG         sectorOffset;
            USHORT        sectorCount;

            if (!cdData->Mmc.WriteAllowed) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_MEDIA_WRITE_PROTECTED;
                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }
            //
            // Verify sectors
            //

            srb->CdbLength = 10;

            cdb->CDB10.OperationCode = SCSIOP_VERIFY;

            //
            // Add disk offset to starting sector.
            //

            byteOffset.QuadPart = commonExtension->StartingOffset.QuadPart +
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

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        case IOCTL_STORAGE_CHECK_VERIFY:
        case IOCTL_DISK_CHECK_VERIFY:
        case IOCTL_CDROM_CHECK_VERIFY: {

            //
            // Since a test unit ready is about to be performed, reset the
            // timer value to decrease the opportunities for it to race with
            // this code.
            //

            ClassResetMediaChangeTimer(fdoExtension);

            //
            // Set up the SRB/CDB
            //

            srb->CdbLength = 6;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
            srb->TimeOutValue = fdoExtension->TimeOutValue * 2;
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);


            TraceLog((CdromDebugTrace,
                        "CdRomStartIo: [%p] Sending CHECK_VERIFY irp %p\n",
                        Irp, irp2));
            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        case IOCTL_DVD_READ_STRUCTURE: {

            CdRomDeviceControlDvdReadStructure(Fdo, Irp, irp2, srb);
            return;

        }

        case IOCTL_DVD_END_SESSION: {
            CdRomDeviceControlDvdEndSession(Fdo, Irp, irp2, srb);
            return;
        }

        case IOCTL_DVD_START_SESSION:
        case IOCTL_DVD_READ_KEY: {

            CdRomDeviceControlDvdStartSessionReadKey(Fdo, Irp, irp2, srb);
            return;

        }


        case IOCTL_DVD_SEND_KEY:
        case IOCTL_DVD_SEND_KEY2: {

            CdRomDeviceControlDvdSendKey (Fdo, Irp, irp2, srb);
            return;


        }

        case IOCTL_CDROM_READ_TOC_EX: {
            
            PCDROM_READ_TOC_EX inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            transferByteCount = currentIrpStack->Parameters.Read.Length;

            dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                               transferByteCount,
                                               CDROM_TAG_TOC);
            if (!dataBuffer) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            transferByteCount,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            //
            // setup the request per user request
            // do validity checking in devctl dispatch, not here
            //

            cdb->READ_TOC.OperationCode = SCSIOP_READ_TOC;
            cdb->READ_TOC.Msf = inputBuffer->Msf;
            cdb->READ_TOC.Format2 = inputBuffer->Format;
            cdb->READ_TOC.StartingTrack = inputBuffer->SessionTrack;
            cdb->READ_TOC.AllocationLength[0] = (UCHAR)(transferByteCount >> 8);
            cdb->READ_TOC.AllocationLength[1] = (UCHAR)(transferByteCount & 0xff);

            //
            // Prepare the MDL
            //

            MmBuildMdlForNonPagedPool(irp2->MdlAddress);

            //
            // do the standard stuff....
            //

            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = transferByteCount;
            srb->CdbLength = 10;
            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->DataBuffer = dataBuffer;

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_GET_LAST_SESSION:
        case IOCTL_CDROM_READ_TOC: {

            if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode == 
                IOCTL_CDROM_GET_LAST_SESSION) {

                //
                // Set format to return first and last session numbers.
                //

                cdb->READ_TOC.Format = CDROM_READ_TOC_EX_FORMAT_SESSION;

            } else {

                //
                // Use MSF addressing
                //

                cdb->READ_TOC.Msf = 1;

            }


            transferByteCount =
                currentIrpStack->Parameters.Read.Length >
                    sizeof(CDROM_TOC) ? sizeof(CDROM_TOC):
                    currentIrpStack->Parameters.Read.Length;

            //
            // Set size of TOC structure.
            //

            cdb->READ_TOC.AllocationLength[0] = (UCHAR) (transferByteCount >> 8);
            cdb->READ_TOC.AllocationLength[1] = (UCHAR) (transferByteCount & 0xFF);

            //
            // setup remaining srb and cdb parameters.
            //

            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = transferByteCount;
            srb->CdbLength = 10;
            srb->TimeOutValue = fdoExtension->TimeOutValue;

            dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                               transferByteCount,
                                               CDROM_TAG_TOC);
            if (!dataBuffer) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            transferByteCount,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            //
            // Prepare the MDL
            //
            
            MmBuildMdlForNonPagedPool(irp2->MdlAddress);

            srb->DataBuffer = dataBuffer;
            cdb->READ_TOC.OperationCode = SCSIOP_READ_TOC;

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
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

            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_READ_Q_CHANNEL: {

#if 0
            PSUB_Q_CHANNEL_DATA userChannelData =
                             Irp->AssociatedIrp.SystemBuffer;
#endif
            PCDROM_SUB_Q_DATA_FORMAT inputBuffer =
                             Irp->AssociatedIrp.SystemBuffer;

            //
            // Allocate buffer for subq channel information.
            //

            dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                               sizeof(SUB_Q_CHANNEL_DATA),
                                               CDROM_TAG_SUB_Q);

            if (!dataBuffer) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                             sizeof(SUB_Q_CHANNEL_DATA),
                                             FALSE,
                                             FALSE,
                                             (PIRP) NULL);

            if (!irp2->MdlAddress) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
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
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
            srb->DataTransferLength = transferByteCount;
            srb->CdbLength = 10;
            srb->TimeOutValue = fdoExtension->TimeOutValue;

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_PAUSE_AUDIO: {

            cdb->PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
            cdb->PAUSE_RESUME.Action = CDB_AUDIO_PAUSE;

            srb->CdbLength = 10;
            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_RESUME_AUDIO: {

            cdb->PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
            cdb->PAUSE_RESUME.Action = CDB_AUDIO_RESUME;

            srb->CdbLength = 10;
            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
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

            srb->CdbLength = 10;
            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_STOP_AUDIO: {

            cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
            cdb->START_STOP.Immediate = 1;
            cdb->START_STOP.Start = 0;
            cdb->START_STOP.LoadEject = 0;

            srb->CdbLength = 6;
            srb->TimeOutValue = fdoExtension->TimeOutValue;

            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        case IOCTL_CDROM_GET_CONTROL: {

            //PAUDIO_OUTPUT audioOutput;
            //PCDROM_AUDIO_CONTROL audioControl = Irp->AssociatedIrp.SystemBuffer;

            //
            // Allocate buffer for volume control information.
            //

            dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                               MODE_DATA_SIZE,
                                               CDROM_TAG_VOLUME);

            if (!dataBuffer) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;

            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            MODE_DATA_SIZE,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
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

            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->DataTransferLength = MODE_DATA_SIZE;
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;

        }

        case IOCTL_CDROM_GET_VOLUME:
        case IOCTL_CDROM_SET_VOLUME: {

            dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                        MODE_DATA_SIZE,
                                        CDROM_TAG_VOLUME);

            if (!dataBuffer) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
                return;
            }

            irp2->MdlAddress = IoAllocateMdl(dataBuffer,
                                            MODE_DATA_SIZE,
                                            FALSE,
                                            FALSE,
                                            (PIRP) NULL);

            if (!irp2->MdlAddress) {
                ExFreePool(senseBuffer);
                ExFreePool(srb);
                ExFreePool(dataBuffer);
                IoFreeIrp(irp2);
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                BAIL_OUT(Irp);
                CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
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

            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->DataTransferLength = MODE_DATA_SIZE;
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);

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

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;

        }

        case IOCTL_STORAGE_SET_READ_AHEAD: {

            PSTORAGE_SET_READ_AHEAD readAhead = Irp->AssociatedIrp.SystemBuffer;

            ULONG blockAddress;
            PFOUR_BYTE fourByte = (PFOUR_BYTE) &blockAddress;

            //
            // setup the SRB for a set readahead command
            //

            cdb->SET_READ_AHEAD.OperationCode = SCSIOP_SET_READ_AHEAD;

            blockAddress = (ULONG) (readAhead->TriggerAddress.QuadPart >>
                                    fdoExtension->SectorShift);

            cdb->SET_READ_AHEAD.TriggerLBA[0] = fourByte->Byte3;
            cdb->SET_READ_AHEAD.TriggerLBA[1] = fourByte->Byte2;
            cdb->SET_READ_AHEAD.TriggerLBA[2] = fourByte->Byte1;
            cdb->SET_READ_AHEAD.TriggerLBA[3] = fourByte->Byte0;

            blockAddress = (ULONG) (readAhead->TargetAddress.QuadPart >>
                                    fdoExtension->SectorShift);

            cdb->SET_READ_AHEAD.ReadAheadLBA[0] = fourByte->Byte3;
            cdb->SET_READ_AHEAD.ReadAheadLBA[1] = fourByte->Byte2;
            cdb->SET_READ_AHEAD.ReadAheadLBA[2] = fourByte->Byte1;
            cdb->SET_READ_AHEAD.ReadAheadLBA[3] = fourByte->Byte0;

            srb->CdbLength = 12;
            srb->TimeOutValue = fdoExtension->TimeOutValue;

            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);

            IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp2);
            return;
        }

        case IOCTL_DISK_GET_DRIVE_LAYOUT:
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
        case IOCTL_DISK_GET_PARTITION_INFO:
        case IOCTL_DISK_GET_PARTITION_INFO_EX: {

            ASSERT(irp2);
            ASSERT(senseBuffer);
            ASSERT(srb);

            ExFreePool(srb);
            ExFreePool(senseBuffer);
            IoFreeIrp(irp2);

            //
            // NOTE: should probably update the media's capacity first...
            //
            
            CdromFakePartitionInfo(commonExtension, Irp);
            return;
        }

        case IOCTL_DISK_IS_WRITABLE: {
            
            TraceLog((CdromDebugWarning,
                        "CdRomStartIo: DiskIsWritable (%p) - returning %s\n",
                        Irp, (cdData->Mmc.WriteAllowed ? "TRUE" : "false")));
            
            ASSERT(irp2);
            ASSERT(senseBuffer);
            ASSERT(srb);

            ExFreePool(srb);
            ExFreePool(senseBuffer);
            IoFreeIrp(irp2);

            Irp->IoStatus.Information = 0;
            if (cdData->Mmc.WriteAllowed) {
                Irp->IoStatus.Status = STATUS_SUCCESS;
            } else {
                Irp->IoStatus.Status = STATUS_MEDIA_WRITE_PROTECTED;
            }
            CdRomCompleteIrpAndStartNextPacketSafely(Fdo, Irp);
            return;
        }

        default: {

            UCHAR uniqueAddress;

            //
            // Just complete the request - CdRomClassIoctlCompletion will take
            // care of it for us 
            //
            // NOTE: THIS IS A SYNCHRONIZATION METHOD!!!
            //

            //
            // Acquire a new copy of the lock so that ClassCompleteRequest
            // doesn't get confused when we complete the other request while
            // holding the lock.
            //

            //
            // NOTE: CdRomDeviceControlDispatch/CdRomDeviceControlCompletion
            //       wait for the event and eventually calls
            //       IoStartNextPacket()
            //

            ASSERT(irp2);
            ASSERT(senseBuffer);
            ASSERT(srb);

            ExFreePool(srb);
            ExFreePool(senseBuffer);
            IoFreeIrp(irp2);



            ClassAcquireRemoveLock(Fdo, (PIRP)&uniqueAddress);
            ClassReleaseRemoveLock(Fdo, Irp);
            ClassCompleteRequest(Fdo, Irp, IO_NO_INCREMENT);
            ClassReleaseRemoveLock(Fdo, (PIRP)&uniqueAddress);
            return;
        }

        } // end switch()
    } else if (currentIrpStack->MajorFunction == IRP_MJ_SHUTDOWN ||
               currentIrpStack->MajorFunction == IRP_MJ_FLUSH_BUFFERS) {

        currentIrpStack->Parameters.Others.Argument1 = 0;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        CdRomShutdownFlushCompletion(Fdo, NULL, Irp);
        return;

    }

    //
    // If a read or an unhandled IRP_MJ_XX, end up here. The unhandled IRP_MJ's
    // are expected and composed of AutoRun Irps, at present.
    //

    IoCallDriver(commonExtension->LowerDeviceObject, Irp);
    return;
}

NTSTATUS
NTAPI
CdRomReadWriteVerification(
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
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG               transferByteCount = currentIrpStack->Parameters.Read.Length;
    LARGE_INTEGER       startingOffset = currentIrpStack->Parameters.Read.ByteOffset;

    //PCDROM_DATA         cdData = (PCDROM_DATA)(commonExtension->DriverData);

    //SCSI_REQUEST_BLOCK  srb;
    //PCDB                cdb = (PCDB)srb.Cdb;
    //NTSTATUS            status;

    PAGED_CODE();

    //
    // note: we are no longer failing write commands immediately
    //       they are now failed in StartIo based upon media ability
    //

    //
    // If the cd is playing music then reject this request.
    //

    if (PLAY_ACTIVE(fdoExtension)) {
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

    if (!fdoExtension->DiskGeometry.BytesPerSector) {
        fdoExtension->DiskGeometry.BytesPerSector = 2048;
    }

    if ((startingOffset.QuadPart > commonExtension->PartitionLength.QuadPart) ||
        (transferByteCount & (fdoExtension->DiskGeometry.BytesPerSector - 1))) {

        //
        // Fail request with status of invalid parameters.
        //

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

        return STATUS_INVALID_PARAMETER;
    }


    return STATUS_SUCCESS;

} // end CdRomReadWriteVerification()

NTSTATUS
NTAPI
CdRomSwitchModeCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIO_STACK_LOCATION  realIrpStack;
    PIO_STACK_LOCATION  realIrpNextStack;
    PIRP                realIrp = NULL;
    NTSTATUS            status;
    BOOLEAN             retry;
    PSCSI_REQUEST_BLOCK srb     = Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PCDROM_DATA         cdData = (PCDROM_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    //BOOLEAN             use6Byte = TEST_FLAG(cdData->XAFlags, XA_USE_6_BYTE);
    ULONG retryCount;

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

        ULONG retryInterval;

        TraceLog((CdromDebugTrace,
                    "CdRomSetVolumeIntermediateCompletion: Irp %p, Srb %p, Real Irp %p\n",
                    Irp,
                    srb,
                    realIrp));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ClassReleaseQueue(DeviceObject);
        }


        retry = ClassInterpretSenseInfo(DeviceObject,
                                        srb,
                                        irpStack->MajorFunction,
                                        irpStack->Parameters.DeviceIoControl.IoControlCode,
                                        MAXIMUM_RETRIES - ((ULONG)(ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1),
                                        &status,
                                        &retryInterval);

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (realIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME &&
            status == STATUS_VERIFY_REQUIRED) {

            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        //
        // get current retry count
        //
        retryCount = PtrToUlong(realIrpNextStack->Parameters.Others.Argument1);

        if (retry && retryCount) {

            //
            // decrement retryCount and update
            //
            realIrpNextStack->Parameters.Others.Argument1 = UlongToPtr(retryCount-1);

            if (((ULONG)(ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1)) {

                //
                // Retry request.
                //

                TraceLog((CdromDebugWarning,
                            "Retry request %p - Calling StartIo\n", Irp));


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

                CdRomRetryRequest(fdoExtension,
                                  realIrp,
                                  retryInterval,
                                  FALSE);

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

        sectorSize =  cdData->BlockDescriptor.BlockLength[0] << 16;
        sectorSize |= (cdData->BlockDescriptor.BlockLength[1] << 8);
        sectorSize |= (cdData->BlockDescriptor.BlockLength[2]);

        cdData->RawAccess = (sectorSize == RAW_SECTOR_SIZE) ? TRUE : FALSE;

        //
        // Free the old data buffer, mdl.
        // reuse the SenseInfoBuffer and Srb
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
            //UCHAR min, sec, frame;

            //
            // Calculate starting offset.
            //

            startingSector = (ULONG)(rawReadInfo->DiskOffset.QuadPart >> fdoExtension->SectorShift);
            transferByteCount  = rawReadInfo->SectorCount * RAW_SECTOR_SIZE;
            maximumTransferLength = fdoExtension->AdapterDescriptor->MaximumTransferLength;
            transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(realIrp->MdlAddress),
                                                           transferByteCount);

            //
            // Determine if request is within limits imposed by miniport.
            // If the request is larger than the miniport's capabilities, split it.
            //

            if (transferByteCount > maximumTransferLength ||
                transferPages > fdoExtension->AdapterDescriptor->MaximumPhysicalPages) {


                ExFreePool(srb->SenseInfoBuffer);
                ExFreePool(srb);
                realIrp->IoStatus.Information = 0;
                realIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                BAIL_OUT(realIrp);
                CdRomCompleteIrpAndStartNextPacketSafely(DeviceObject, realIrp);
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            srb->OriginalRequest = realIrp;
            srb->SrbFlags = fdoExtension->SrbFlags;
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);

            srb->DataTransferLength = transferByteCount;
            srb->TimeOutValue = fdoExtension->TimeOutValue;
            srb->CdbLength = 10;
            srb->DataBuffer = MmGetMdlVirtualAddress(realIrp->MdlAddress);

            if (rawReadInfo->TrackMode == CDDA) {
                if (TEST_FLAG(cdData->XAFlags, XA_PLEXTOR_CDDA)) {

                    srb->CdbLength = 12;

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

                } else if (TEST_FLAG(cdData->XAFlags, XA_NEC_CDDA)) {

                    cdb->NEC_READ_CDDA.LogicalBlockByte3  = (UCHAR) (startingSector & 0xFF);
                    cdb->NEC_READ_CDDA.LogicalBlockByte2  = (UCHAR) ((startingSector >>  8) & 0xFF);
                    cdb->NEC_READ_CDDA.LogicalBlockByte1  = (UCHAR) ((startingSector >> 16) & 0xFF);
                    cdb->NEC_READ_CDDA.LogicalBlockByte0  = (UCHAR) ((startingSector >> 24) & 0xFF);

                    cdb->NEC_READ_CDDA.TransferBlockByte1 = (UCHAR) (rawReadInfo->SectorCount & 0xFF);
                    cdb->NEC_READ_CDDA.TransferBlockByte0 = (UCHAR) (rawReadInfo->SectorCount >> 8);

                    cdb->NEC_READ_CDDA.OperationCode = 0xD4;
                }
            } else {
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

            PSTORAGE_ADAPTER_DESCRIPTOR adapterDescriptor;
            ULONG maximumTransferLength;
            ULONG transferPages;

            //
            // a writable device must be MMC compliant, which supports
            // READ_CD commands, so writes and mode switching should
            // never occur on the same device.
            //

            ASSERT(realIrpStack->MajorFunction != IRP_MJ_WRITE);

            //
            // free the SRB and SenseInfoBuffer since they aren't used
            // by either ClassBuildRequest() nor ClassSplitRequest().
            //

            ExFreePool(srb->SenseInfoBuffer);
            ExFreePool(srb);
            
            //
            // Back to cooked sectors. Build and send a normal read.
            // The real work for setting offsets was done in startio.
            //
            
            adapterDescriptor =
                commonExtension->PartitionZeroExtension->AdapterDescriptor;
            maximumTransferLength = adapterDescriptor->MaximumTransferLength;
            transferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                                MmGetMdlVirtualAddress(realIrp->MdlAddress),
                                realIrpStack->Parameters.Read.Length);

            if ((realIrpStack->Parameters.Read.Length > maximumTransferLength) ||
                (transferPages > adapterDescriptor->MaximumPhysicalPages)) {

                ULONG maxPages = adapterDescriptor->MaximumPhysicalPages;

                if (maxPages != 0) {
                    maxPages --; // to account for page boundaries
                }

                TraceLog((CdromDebugTrace,
                            "CdromSwitchModeCompletion: Request greater than "
                            " maximum\n"));
                TraceLog((CdromDebugTrace,
                            "CdromSwitchModeCompletion: Maximum is %lx\n",
                            maximumTransferLength));
                TraceLog((CdromDebugTrace,
                            "CdromSwitchModeCompletion: Byte count is %lx\n",
                            realIrpStack->Parameters.Read.Length));

                //
                // Check that the maximum transfer length fits within
                // the maximum number of pages the device can handle.
                //
                
                if (maximumTransferLength > maxPages << PAGE_SHIFT) {
                    maximumTransferLength = maxPages << PAGE_SHIFT;
                }

                //
                // Check that maximum transfer size is not zero
                //

                if (maximumTransferLength == 0) {
                    maximumTransferLength = PAGE_SIZE;
                }

                //
                // Request needs to be split. Completion of each portion
                // of the request will fire off the next portion. The final
                // request will signal Io to send a new request.
                //

                ClassSplitRequest(DeviceObject, realIrp, maximumTransferLength);
                return STATUS_MORE_PROCESSING_REQUIRED;

            } else {

                //
                // Build SRB and CDB for this IRP.
                //

                ClassBuildRequest(DeviceObject, realIrp);

            }
        }

        //
        // Call the port driver.
        //

        IoCallDriver(commonExtension->LowerDeviceObject, realIrp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Update device Extension flags to indicate that XA isn't supported.
    //

    TraceLog((CdromDebugWarning,
                "Device Cannot Support CDDA (but tested positive) "
                "Now Clearing CDDA flags for FDO %p\n", DeviceObject));
    SET_FLAG(cdData->XAFlags, XA_NOT_SUPPORTED);
    CLEAR_FLAG(cdData->XAFlags, XA_PLEXTOR_CDDA);
    CLEAR_FLAG(cdData->XAFlags, XA_NEC_CDDA);

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

        if (realIrp->Tail.Overlay.Thread) {
            IoSetHardErrorOrVerifyDevice(realIrp, DeviceObject);
        }
        realIrp->IoStatus.Information = 0;
    }

    CdRomCompleteIrpAndStartNextPacketSafely(DeviceObject, realIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
NTAPI
ScanForSpecialHandler(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    ULONG_PTR HackFlags
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension;
    PCDROM_DATA cdData;

    PAGED_CODE();

    CLEAR_FLAG(HackFlags, CDROM_HACK_INVALID_FLAGS);
    
    commonExtension = &(FdoExtension->CommonExtension);
    cdData = (PCDROM_DATA)(commonExtension->DriverData);
    cdData->HackFlags = HackFlags;

    return;
}

VOID
NTAPI
ScanForSpecial(
    PDEVICE_OBJECT DeviceObject
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
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension;
    PCDROM_DATA cdData;
    
    PAGED_CODE();

    fdoExtension = DeviceObject->DeviceExtension;
    commonExtension = DeviceObject->DeviceExtension;
    cdData = (PCDROM_DATA)(commonExtension->DriverData);

    
    //
    // set our hack flags
    //

    ClassScanForSpecial(fdoExtension, CdromHackItems, ScanForSpecialHandler);

    //
    // All CDRom's can ignore the queue lock failure for power operations
    // and do not require handling the SpinUp case (unknown result of sending
    // a cdrom a START_UNIT command -- may eject disks?)
    //
    // We send the stop command mostly to stop outstanding asynch operations
    // (like audio playback) from running when the system is powered off.
    // Because of this and the unlikely chance that a PLAY command will be
    // sent in the window between the STOP and the time the machine powers down
    // we don't require queue locks.  This is important because without them
    // classpnp's power routines will send the START_STOP_UNIT command to the
    // device whether or not it supports locking (atapi does not support locking
    // and if we requested them we would end up not stopping audio on atapi
    // devices).
    //

    SET_FLAG(fdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_DISABLE_SPIN_UP);
    SET_FLAG(fdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_NO_QUEUE_LOCK);

    if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_HITACHI_1750)
        && ( fdoExtension->AdapterDescriptor->AdapterUsesPio )
        ) {

        //
        // Read-ahead must be disabled in order to get this cdrom drive
        // to work on scsi adapters that use PIO.
        //


        TraceLog((CdromDebugWarning,
                    "CdRom ScanForSpecial:  Found Hitachi CDR-1750S.\n"));

        //
        // Setup an error handler to reinitialize the cd rom after it is reset.
        //

        cdData->ErrorHandler = HitachiProcessError;

        //
        // Lock down the hitachi error processing code.
        //

        MmLockPagableCodeSection(HitachiProcessError);
        SET_FLAG(cdData->HackFlags, CDROM_HACK_LOCKED_PAGES);


    } else if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_TOSHIBA_SD_W1101)) {

        TraceLog((CdromDebugError,
                    "CdRom ScanForSpecial: Found Toshiba SD-W1101 DVD-RAM "
                    "-- This drive will *NOT* support DVD-ROM playback.\n"));

    } else if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_HITACHI_GD_2000)) {

        TraceLog((CdromDebugWarning,
                    "CdRom ScanForSpecial: Found Hitachi GD-2000\n"));

        //
        // Setup an error handler to spin up the drive when it idles out
        // since it seems to like to fail to spin itself back up on its
        // own for a REPORT_KEY command.  It may also lose the AGIDs that
        // it has given, which will result in DVD playback failures.
        // This routine will just do what it can...
        //

        cdData->ErrorHandler = HitachiProcessErrorGD2000;

        //
        // this drive may require START_UNIT commands to spin
        // the drive up when it's spun itself down.
        //

        SET_FLAG(fdoExtension->DeviceFlags, DEV_SAFE_START_UNIT);

        //
        // Lock down the hitachi error processing code.
        //

        MmLockPagableCodeSection(HitachiProcessErrorGD2000);
        SET_FLAG(cdData->HackFlags, CDROM_HACK_LOCKED_PAGES);

    } else if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_FUJITSU_FMCD_10x)) {

        //
        // When Read command is issued to FMCD-101 or FMCD-102 and there is a music
        // cd in it. It takes longer time than SCSI_CDROM_TIMEOUT before returning
        // error status.
        //

        fdoExtension->TimeOutValue = 20;

    } else if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_DEC_RRD)) {

        PMODE_PARM_READ_WRITE_DATA modeParameters;
        SCSI_REQUEST_BLOCK         srb;
        PCDB                       cdb;
        NTSTATUS                   status;


        TraceLog((CdromDebugWarning,
                    "CdRom ScanForSpecial:  Found DEC RRD.\n"));

        cdData->IsDecRrd = TRUE;

        //
        // Setup an error handler to reinitialize the cd rom after it is reset?
        //
        //commonExtension->DevInfo->ClassError = DecRrdProcessError;

        //
        // Found a DEC RRD cd-rom.  These devices do not pass MS HCT
        // multi-media tests because the DEC firmware modifieds the block
        // from the PC-standard 2K to 512.  Change the block transfer size
        // back to the PC-standard 2K by using a mode select command.
        //

        modeParameters = ExAllocatePoolWithTag(NonPagedPool,
                                               sizeof(MODE_PARM_READ_WRITE_DATA),
                                               CDROM_TAG_MODE_DATA
                                               );
        if (modeParameters == NULL) {
            return;
        }

        RtlZeroMemory(modeParameters, sizeof(MODE_PARM_READ_WRITE_DATA));
        RtlZeroMemory(&srb,           sizeof(SCSI_REQUEST_BLOCK));

        //
        // Set the block length to 2K.
        //

        modeParameters->ParameterListHeader.BlockDescriptorLength =
                sizeof(MODE_PARAMETER_BLOCK);

        //
        // Set block length to 2K (0x0800) in Parameter Block.
        //

        modeParameters->ParameterListBlock.BlockLength[0] = 0x00; //MSB
        modeParameters->ParameterListBlock.BlockLength[1] = 0x08;
        modeParameters->ParameterListBlock.BlockLength[2] = 0x00; //LSB

        //
        // Build the mode select CDB.
        //

        srb.CdbLength = 6;
        srb.TimeOutValue = fdoExtension->TimeOutValue;

        cdb = (PCDB)srb.Cdb;
        cdb->MODE_SELECT.PFBit               = 1;
        cdb->MODE_SELECT.OperationCode       = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.ParameterListLength = HITACHI_MODE_DATA_SIZE;

        //
        // Send the request to the device.
        //

        status = ClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         modeParameters,
                                         sizeof(MODE_PARM_READ_WRITE_DATA),
                                         TRUE);

        if (!NT_SUCCESS(status)) {
            TraceLog((CdromDebugWarning,
                        "CdRom ScanForSpecial: Setting DEC RRD to 2K block"
                        "size failed [%x]\n", status));
        }
        ExFreePool(modeParameters);

    } else if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_TOSHIBA_XM_3xx)) {

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

        srb.TimeOutValue = fdoExtension->TimeOutValue;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.PageCode = 0x1;
        // NOTE: purposely not setting DBD because it is what is needed.
        cdb->MODE_SENSE.AllocationLength = (UCHAR)length;

        buffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                (sizeof(MODE_READ_RECOVERY_PAGE) + MODE_BLOCK_DESC_LENGTH + MODE_HEADER_LENGTH),
                                CDROM_TAG_MODE_DATA);
        if (!buffer) {
            return;
        }

        status = ClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         buffer,
                                         length,
                                         FALSE);

        ((PERROR_RECOVERY_DATA)buffer)->BlockDescriptor.DensityCode = 0x83;
        ((PERROR_RECOVERY_DATA)buffer)->Header.ModeDataLength = 0x0;

        RtlCopyMemory(&cdData->Header, buffer, sizeof(ERROR_RECOVERY_DATA));

        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        //
        // Build the MODE SENSE CDB.
        //

        srb.CdbLength = 6;
        cdb = (PCDB)srb.Cdb;

        //
        // Set timeout value from device extension.
        //

        srb.TimeOutValue = fdoExtension->TimeOutValue;

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = 1;
        cdb->MODE_SELECT.ParameterListLength = (UCHAR)length;

        status = ClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         buffer,
                                         length,
                                         TRUE);

        if (!NT_SUCCESS(status)) {
            TraceLog((CdromDebugWarning,
                        "Cdrom.ScanForSpecial: Setting density code on Toshiba failed [%x]\n",
                        status));
        }

        cdData->ErrorHandler = ToshibaProcessError;

        //
        // Lock down the toshiba error section.
        //

        MmLockPagableCodeSection(ToshibaProcessError);
        SET_FLAG(cdData->HackFlags, CDROM_HACK_LOCKED_PAGES);

        ExFreePool(buffer);

    }

    //
    // Determine special CD-DA requirements.
    //

    if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_READ_CD_SUPPORTED)) {

        SET_FLAG(cdData->XAFlags, XA_USE_READ_CD);

    } else if (!TEST_FLAG(cdData->XAFlags, XA_USE_READ_CD)) {

        if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_PLEXTOR_CDDA)) {
            SET_FLAG(cdData->XAFlags, XA_PLEXTOR_CDDA);
        } else if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_NEC_CDDA)) {
            SET_FLAG(cdData->XAFlags, XA_NEC_CDDA);
        }

    }

    if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_LOCKED_PAGES)) {
        KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL,
                   "Locking pages for error handler\n"));
    }

    
    return;
}

VOID
NTAPI
HitachiProcessErrorGD2000(
    PDEVICE_OBJECT Fdo,
    PSCSI_REQUEST_BLOCK OriginalSrb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )
/*++

Routine Description:

   This routine checks the type of error.  If the error suggests that the
   drive has spun down and cannot reinitialize itself, send a
   START_UNIT or READ to the device.  This will force the drive to spin
   up.  This drive also loses the AGIDs it has granted when it spins down,
   which may result in playback failure the first time around.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - return the final status for this command?

    Retry - return if the command should be retried.

Return Value:

    None.

--*/
{
    PSENSE_DATA         senseBuffer = OriginalSrb->SenseInfoBuffer;

    UNREFERENCED_PARAMETER(Status);
    UNREFERENCED_PARAMETER(Retry);

    if (!TEST_FLAG(OriginalSrb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) {
        return;
    }

    if (((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_HARDWARE_ERROR) &&
        (senseBuffer->AdditionalSenseCode == 0x44)) {

        //PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
        //PIRP                irp;
        //PIO_STACK_LOCATION  irpStack;
        //PCOMPLETION_CONTEXT context;
        //PSCSI_REQUEST_BLOCK newSrb;
        //PCDB                cdb;

        TraceLog((CdromDebugWarning,
                    "HitachiProcessErrorGD2000 (%p) => Internal Target "
                    "Failure Detected -- spinning up drive\n", Fdo));

        //
        // the request should be retried because the device isn't ready
        //

        *Retry = TRUE;
        *Status = STATUS_DEVICE_NOT_READY;

        //
        // send a START_STOP unit to spin up the drive
        // NOTE: this temporarily violates the StartIo serialization
        //       mechanism, but the completion routine on this will NOT
        //       call StartNextPacket(), so it's a temporary disruption
        //       of the serialization only.
        //

        ClassSendStartUnit(Fdo);

    }

    return;
}

VOID
NTAPI
HitachiProcessError(
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
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PSENSE_DATA         senseBuffer = Srb->SenseInfoBuffer;
    LARGE_INTEGER       largeInt;
    PUCHAR              modePage;
    PIO_STACK_LOCATION  irpStack;
    PIRP                irp;
    PSCSI_REQUEST_BLOCK srb;
    PCOMPLETION_CONTEXT context;
    PCDB                cdb;
    ULONG_PTR            alignment;

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
    // Found an HITACHI cd-rom that does not work with PIO
    // adapters when read-ahead is enabled.  Read-ahead is disabled by
    // a mode select command.  The mode select page code is zero and the
    // length is 6 bytes.  All of the other bytes should be zero.
    //

    if ((senseBuffer->SenseKey & 0xf) == SCSI_SENSE_UNIT_ATTENTION) {

        TraceLog((CdromDebugWarning,
                    "HitachiProcessError: Reinitializing the CD-ROM.\n"));

        //
        // Send the special mode select command to disable read-ahead
        // on the CD-ROM reader.
        //

        alignment = DeviceObject->AlignmentRequirement ?
            DeviceObject->AlignmentRequirement : 1;

        context = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(COMPLETION_CONTEXT) +  HITACHI_MODE_DATA_SIZE + (ULONG)alignment,
            CDROM_TAG_HITACHI_ERROR
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

        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->TimeOutValue = fdoExtension->TimeOutValue;

        //
        // Set the transfer length.
        //

        srb->DataTransferLength = HITACHI_MODE_DATA_SIZE;
        srb->SrbFlags = fdoExtension->SrbFlags;
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_OUT);
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_AUTOSENSE);
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

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

        if (irp == NULL) {

            //
            // If there is not enough memory to fulfill this request,
            // simply return. A subsequent retry will fail and another
            // chance to start the unit.
            //

            ExFreePool(context);
            return;
        }

        ClassAcquireRemoveLock(DeviceObject, irp);

        IoSetCompletionRoutine(irp,
                   (PIO_COMPLETION_ROUTINE)ClassAsynchronousCompletion,
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

        (VOID)IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

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
    // Free all of the allocations.
    //

    ClassReleaseRemoveLock(DeviceObject, Irp);

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
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PCDROM_DATA         cdData = (PCDROM_DATA)(commonExtension->DriverData);
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


        irp = IoAllocateIrp((CCHAR)(DeviceObject->StackSize+1),
                              FALSE);

        if (!irp) {
            return;
        }

        srb = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(SCSI_REQUEST_BLOCK),
                                    CDROM_TAG_TOSHIBA_ERROR);
        if (!srb) {
            IoFreeIrp(irp);
            return;
        }


        length = sizeof(ERROR_RECOVERY_DATA);
        dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                           length,
                                           CDROM_TAG_TOSHIBA_ERROR);
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
        irpStack->DeviceObject = DeviceObject;

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

        ClassAcquireRemoveLock(DeviceObject, irp);

        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->SrbStatus = srb->ScsiStatus = 0;
        srb->NextSrb = 0;
        srb->OriginalRequest = irp;
        srb->SenseInfoBufferLength = 0;

        //
        // Set the transfer length.
        //

        srb->DataTransferLength = length;
        srb->SrbFlags = fdoExtension->SrbFlags;
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_OUT);
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_AUTOSENSE);
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);


        srb->CdbLength = 6;
        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.PFBit = 1;
        cdb->MODE_SELECT.ParameterListLength = (UCHAR)length;

        //
        // Copy the Mode page into the databuffer.
        //

        RtlCopyMemory(srb->DataBuffer, &cdData->Header, length);

        //
        // Set the density code.
        //

        ((PERROR_RECOVERY_DATA)srb->DataBuffer)->BlockDescriptor.DensityCode = 0x83;

        IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);
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
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    IO_STATUS_BLOCK ioStatus;
    PSUB_Q_CURRENT_POSITION currentBuffer;

    PAGED_CODE();

    //
    // if we don't think it is playing audio, don't bother checking.
    //

    if (!PLAY_ACTIVE(fdoExtension)) {
        return(FALSE);
    }

    currentBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                          sizeof(SUB_Q_CURRENT_POSITION),
                                          CDROM_TAG_PLAY_ACTIVE);

    if (currentBuffer == NULL) {
        return(FALSE);
    }

    ((PCDROM_SUB_Q_DATA_FORMAT) currentBuffer)->Format = IOCTL_CDROM_CURRENT_POSITION;
    ((PCDROM_SUB_Q_DATA_FORMAT) currentBuffer)->Track = 0;

    //
    // Build the synchronous request to be sent to ourself
    // to perform the request.
    //

    ClassSendDeviceIoControlSynchronous(
        IOCTL_CDROM_READ_Q_CHANNEL,
        DeviceObject,
        currentBuffer,
        sizeof(CDROM_SUB_Q_DATA_FORMAT),
        sizeof(SUB_Q_CURRENT_POSITION),
        FALSE,
        &ioStatus);

    if (!NT_SUCCESS(ioStatus.Status)) {
        ExFreePool(currentBuffer);
        return FALSE;
    }

    //
    // should update the playactive flag here.
    //

    if (currentBuffer->Header.AudioStatus == AUDIO_STATUS_IN_PROGRESS) {
        PLAY_ACTIVE(fdoExtension) = TRUE;
    } else {
        PLAY_ACTIVE(fdoExtension) = FALSE;
    }

    ExFreePool(currentBuffer);

    return(PLAY_ACTIVE(fdoExtension));

}

VOID
NTAPI
CdRomTickHandler(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine handles the once per second timer provided by the
    Io subsystem.  It is used to do delayed retries for cdroms.

Arguments:

    DeviceObject - what to check.

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    ULONG isRemoved;

    KIRQL             oldIrql;

    //PIRP              irp;
    //PIRP              heldIrpList;
    //PIRP              nextIrp;
    //PLIST_ENTRY       listEntry;
    PCDROM_DATA       cddata;
    //PIO_STACK_LOCATION irpStack;
    UCHAR             uniqueAddress;

    isRemoved = ClassAcquireRemoveLock(DeviceObject, (PIRP) &uniqueAddress);

    //
    // We stop the timer before deleting the device.  It's safe to keep going
    // if the flag value is REMOVE_PENDING because the removal thread will be
    // blocked trying to stop the timer.
    //

    ASSERT(isRemoved != REMOVE_COMPLETE);

    //
    // This routine is reasonably safe even if the device object has a pending
    // remove

    cddata = commonExtension->DriverData;

    //
    // Since cdrom is completely synchronized there can never be more than one
    // irp delayed for retry at any time.
    //

    KeAcquireSpinLock(&(cddata->DelayedRetrySpinLock), &oldIrql);

    if(cddata->DelayedRetryIrp != NULL) {

        PIRP irp = cddata->DelayedRetryIrp;

        //
        // If we've got a delayed retry at this point then there had beter
        // be an interval for it.
        //

        ASSERT(cddata->DelayedRetryInterval != 0);
        cddata->DelayedRetryInterval--;

        if(isRemoved) {

            //
            // This device is removed - flush the timer queue
            //

            cddata->DelayedRetryIrp = NULL;
            cddata->DelayedRetryInterval = 0;

            KeReleaseSpinLock(&(cddata->DelayedRetrySpinLock), oldIrql);

            ClassReleaseRemoveLock(DeviceObject, irp);
            ClassCompleteRequest(DeviceObject, irp, IO_CD_ROM_INCREMENT);

        } else if (cddata->DelayedRetryInterval == 0) {

            //
            // Submit this IRP to the lower driver.  This IRP does not
            // need to be remembered here.  It will be handled again when
            // it completes.
            //

            cddata->DelayedRetryIrp = NULL;

            KeReleaseSpinLock(&(cddata->DelayedRetrySpinLock), oldIrql);

            TraceLog((CdromDebugWarning,
                        "CdRomTickHandler: Reissuing request %p (thread = %p)\n",
                        irp,
                        irp->Tail.Overlay.Thread));

            //
            // feed this to the appropriate port driver
            //

            CdRomRerunRequest(fdoExtension, irp, cddata->DelayedRetryResend);
        } else {
            KeReleaseSpinLock(&(cddata->DelayedRetrySpinLock), oldIrql);
        }
    } else {
        KeReleaseSpinLock(&(cddata->DelayedRetrySpinLock), oldIrql);
    }

    ClassReleaseRemoveLock(DeviceObject, (PIRP) &uniqueAddress);
}

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
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension;

    PSCSI_REQUEST_BLOCK srb = (PSCSI_REQUEST_BLOCK) Context;
    PREAD_CAPACITY_DATA readCapacityBuffer;
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    BOOLEAN             retry;
    ULONG               retryCount;
    //ULONG               lastSector;
    PIRP                originalIrp;
    //PCDROM_DATA         cddata;
    //UCHAR               uniqueAddress;

    //
    // Get items saved in the private IRP stack location.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    retryCount = (ULONG)(ULONG_PTR) irpStack->Parameters.Others.Argument1;
    originalIrp = (PIRP) irpStack->Parameters.Others.Argument2;

    if (!DeviceObject) {
        DeviceObject = irpStack->DeviceObject;
    }
    ASSERT(DeviceObject);

    fdoExtension = DeviceObject->DeviceExtension;
    commonExtension = DeviceObject->DeviceExtension;
    //cddata = commonExtension->DriverData;
    readCapacityBuffer = srb->DataBuffer;

    if ((NT_SUCCESS(Irp->IoStatus.Status)) && (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS)) {

        CdRomInterpretReadCapacity(DeviceObject, readCapacityBuffer);

    } else {

        ULONG retryInterval;

        TraceLog((CdromDebugWarning,
                    "CdRomUpdateGeometryCompletion: [%p] unsuccessful "
                    "completion of buddy-irp %p (status - %lx)\n",
                    originalIrp, Irp, Irp->IoStatus.Status));

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ClassReleaseQueue(DeviceObject);
        }

        retry = ClassInterpretSenseInfo(DeviceObject,
                                        srb,
                                        IRP_MJ_SCSI,
                                        0,
                                        retryCount,
                                        &status,
                                        &retryInterval);
        if (retry) {
            retryCount--;
            if ((retryCount) && (commonExtension->IsRemoved == NO_REMOVE)) {
                PCDB cdb;

                TraceLog((CdromDebugWarning,
                            "CdRomUpdateGeometryCompletion: [%p] Retrying "
                            "request %p .. thread is %p\n",
                            originalIrp, Irp, Irp->Tail.Overlay.Thread));

                //
                // set up a one shot timer to get this process started over
                //

                irpStack->Parameters.Others.Argument1 = ULongToPtr( retryCount );
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
                srb->TimeOutValue = fdoExtension->TimeOutValue;
                srb->SrbStatus = srb->ScsiStatus = 0;
                srb->NextSrb = 0;
                srb->Length = SCSI_REQUEST_BLOCK_SIZE;
                srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
                srb->SrbFlags = fdoExtension->SrbFlags;
                SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
                SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
                srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);

                //
                // Set up the CDB
                //

                cdb = (PCDB) &srb->Cdb[0];
                cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;

                //
                // Requests queued onto this list will be sent to the
                // lower level driver during CdRomTickHandler
                //

                CdRomRetryRequest(fdoExtension, Irp, retryInterval, TRUE);

                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            if (commonExtension->IsRemoved != NO_REMOVE) {

                //
                // We cannot retry the request.  Fail it.
                //

                originalIrp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

            } else {

                //
                // This has been bounced for a number of times.  Error the
                // original request.
                //

                originalIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                RtlZeroMemory(&(fdoExtension->DiskGeometry),
                              sizeof(DISK_GEOMETRY));
                fdoExtension->DiskGeometry.BytesPerSector = 2048;
                fdoExtension->SectorShift = 11;
                commonExtension->PartitionLength.QuadPart =
                    (LONGLONG)(0x7fffffff);
                fdoExtension->DiskGeometry.MediaType = RemovableMedia;
            }
        } else {

            //
            // Set up reasonable defaults
            //

            RtlZeroMemory(&(fdoExtension->DiskGeometry),
                          sizeof(DISK_GEOMETRY));
            fdoExtension->DiskGeometry.BytesPerSector = 2048;
            fdoExtension->SectorShift = 11;
            commonExtension->PartitionLength.QuadPart = (LONGLONG)(0x7fffffff);
            fdoExtension->DiskGeometry.MediaType = RemovableMedia;
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
    Irp = NULL;

    if (originalIrp->Tail.Overlay.Thread) {

        TraceLog((CdromDebugTrace,
                    "CdRomUpdateGeometryCompletion: [%p] completing "
                    "original IRP\n", originalIrp));

    } else {

        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugError,
                   "CdRomUpdateGeometryCompletion: completing irp %p which has "
                   "no thread\n", originalIrp));

    }
    
    {
        // NOTE: should the original irp be sent down to the device object?
        //       it probably should if the SL_OVERRIDER_VERIFY_VOLUME flag
        //       is set!
        KIRQL oldIrql;
        PIO_STACK_LOCATION realIrpStack;

        realIrpStack = IoGetCurrentIrpStackLocation(originalIrp);
        oldIrql = KeRaiseIrqlToDpcLevel();

        if (TEST_FLAG(realIrpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME)) {
            CdRomStartIo(DeviceObject, originalIrp);
        } else {
            originalIrp->IoStatus.Status = STATUS_VERIFY_REQUIRED;
            originalIrp->IoStatus.Information = 0;
            CdRomCompleteIrpAndStartNextPacketSafely(DeviceObject, originalIrp);
        }
        KeLowerIrql(oldIrql);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
CdRomUpdateCapacity(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
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
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION) DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION) DeviceExtension;

    PCDB                cdb;
    PIRP                irp;
    PSCSI_REQUEST_BLOCK srb;
    PREAD_CAPACITY_DATA capacityBuffer;
    PIO_STACK_LOCATION  irpStack;
    PUCHAR              senseBuffer;
    //NTSTATUS            status;

    irp = IoAllocateIrp((CCHAR)(commonExtension->DeviceObject->StackSize+1),
                        FALSE);

    if (irp) {

        srb = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(SCSI_REQUEST_BLOCK),
                                    CDROM_TAG_UPDATE_CAP);
        if (srb) {
            capacityBuffer = ExAllocatePoolWithTag(
                                NonPagedPoolCacheAligned,
                                sizeof(READ_CAPACITY_DATA),
                                CDROM_TAG_UPDATE_CAP);

            if (capacityBuffer) {


                senseBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                                    SENSE_BUFFER_SIZE,
                                                    CDROM_TAG_UPDATE_CAP);

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
                        irpStack->DeviceObject = commonExtension->DeviceObject;
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
                        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
                        srb->SrbFlags = DeviceExtension->SrbFlags;
                        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
                        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
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

                        //
                        // Set the return value in the IRP that will be completed
                        // upon completion of the read capacity.
                        //

                        IrpToComplete->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
                        IoMarkIrpPending(IrpToComplete);

                        IoCallDriver(commonExtension->LowerDeviceObject, irp);

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

    //
    // complete the original irp with a failure.
    // ISSUE-2000/07/05-henrygab - find a way to avoid failure.
    //

    RtlZeroMemory(&(fdoExtension->DiskGeometry),
                  sizeof(DISK_GEOMETRY));
    fdoExtension->DiskGeometry.BytesPerSector = 2048;
    fdoExtension->SectorShift = 11;
    commonExtension->PartitionLength.QuadPart =
        (LONGLONG)(0x7fffffff);
    fdoExtension->DiskGeometry.MediaType = RemovableMedia;

    IrpToComplete->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
    IrpToComplete->IoStatus.Information = 0;

    BAIL_OUT(IrpToComplete);
    CdRomCompleteIrpAndStartNextPacketSafely(commonExtension->DeviceObject,
                                             IrpToComplete);
    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
NTAPI
CdRomRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    )

/*++

Routine Description:

    This routine is responsible for releasing any resources in use by the
    cdrom driver and shutting down it's timer routine.  This routine is called
    when all outstanding requests have been completed and the device has
    disappeared - no requests may be issued to the lower drivers.

Arguments:

    DeviceObject - the device object being removed

Return Value:

    none - this routine may not fail

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension =
        DeviceObject->DeviceExtension;

    PCDROM_DATA cdData = deviceExtension->CommonExtension.DriverData;

    PAGED_CODE();

    if((Type == IRP_MN_QUERY_REMOVE_DEVICE) ||
       (Type == IRP_MN_CANCEL_REMOVE_DEVICE)) {
        return STATUS_SUCCESS;
    }

    if(cdData->DelayedRetryIrp != NULL) {
        cdData->DelayedRetryInterval = 1;
        CdRomTickHandler(DeviceObject);
    }

    CdRomDeAllocateMmcResources(DeviceObject);

    if (deviceExtension->DeviceDescriptor) {
        ExFreePool(deviceExtension->DeviceDescriptor);
        deviceExtension->DeviceDescriptor = NULL;
    }

    if (deviceExtension->AdapterDescriptor) {
        ExFreePool(deviceExtension->AdapterDescriptor);
        deviceExtension->AdapterDescriptor = NULL;
    }

    if (deviceExtension->SenseData) {
        ExFreePool(deviceExtension->SenseData);
        deviceExtension->SenseData = NULL;
    }

    ClassDeleteSrbLookasideList(&deviceExtension->CommonExtension);

    if(cdData->CdromInterfaceString.Buffer != NULL) {
        IoSetDeviceInterfaceState(
            &(cdData->CdromInterfaceString),
            FALSE);
        RtlFreeUnicodeString(&(cdData->CdromInterfaceString));
        RtlInitUnicodeString(&(cdData->CdromInterfaceString), NULL);
    }

    if(cdData->VolumeInterfaceString.Buffer != NULL) {
        IoSetDeviceInterfaceState(
            &(cdData->VolumeInterfaceString),
            FALSE);
        RtlFreeUnicodeString(&(cdData->VolumeInterfaceString));
        RtlInitUnicodeString(&(cdData->VolumeInterfaceString), NULL);
    }

    CdRomDeleteWellKnownName(DeviceObject);

    ASSERT(cdData->DelayedRetryIrp == NULL);

    if(Type == IRP_MN_REMOVE_DEVICE) {

        if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_LOCKED_PAGES)) {
            
            //
            // unlock locked pages by locking (to get Mm pointer)
            // and then unlocking twice.
            //
            
            PVOID locked;

            if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_HITACHI_1750)) {
                
                locked = MmLockPagableCodeSection(HitachiProcessError);

            } else if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_HITACHI_GD_2000)) {

                locked = MmLockPagableCodeSection(HitachiProcessErrorGD2000);

            } else if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_TOSHIBA_XM_3xx )) {
            
                locked = MmLockPagableCodeSection(ToshibaProcessError);
            
            } else {

                // this is a problem!
                // workaround by locking this twice, once for us and
                // once for the non-existant locker from ScanForSpecial
                ASSERT(!"hack flags show locked section, but none exists?");
                locked = MmLockPagableCodeSection(CdRomRemoveDevice);
                locked = MmLockPagableCodeSection(CdRomRemoveDevice);


            }

            MmUnlockPagableImageSection(locked);
            MmUnlockPagableImageSection(locked);

        }

        //
        // keep the system-wide count accurate, as
        // programs use this info to know when they 
        // have found all the cdroms in a system.
        //

        TraceLog((CdromDebugTrace,
                    "CDROM.SYS Remove device\n"));
        IoGetConfigurationInformation()->CdRomCount--;
    }

    //
    // so long, and thanks for all the fish!
    //

    return STATUS_SUCCESS;
}

DEVICE_TYPE
NTAPI
CdRomGetDeviceType(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine figures out the real device type
    by checking CDVD_CAPABILITIES_PAGE

Arguments:

    DeviceObject -

Return Value:

    FILE_DEVICE_CD_ROM or FILE_DEVICE_DVD


--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCDROM_DATA cdromExtension;
    ULONG bufLength;
    SCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    PMODE_PARAMETER_HEADER10 modePageHeader;
    PCDVD_CAPABILITIES_PAGE capPage;
    ULONG capPageOffset;
    DEVICE_TYPE deviceType;
    NTSTATUS status;
    BOOLEAN use6Byte;

    PAGED_CODE();

    //
    // NOTE: don't cache this until understand how it affects GetMediaTypes()
    //

    //
    // default device type
    //

    deviceType = FILE_DEVICE_CD_ROM;

    fdoExtension = DeviceObject->DeviceExtension;

    cdromExtension = fdoExtension->CommonExtension.DriverData;

    use6Byte = TEST_FLAG(cdromExtension->XAFlags, XA_USE_6_BYTE);

    RtlZeroMemory(&srb, sizeof(srb));
    cdb = (PCDB)srb.Cdb;

    //
    // Build the MODE SENSE CDB. The data returned will be kept in the
    // device extension and used to set block size.
    //
    if (use6Byte) {

        bufLength = sizeof(CDVD_CAPABILITIES_PAGE) +
                    sizeof(MODE_PARAMETER_HEADER);

        capPageOffset = sizeof(MODE_PARAMETER_HEADER);

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = 1;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_CAPABILITIES;
        cdb->MODE_SENSE.AllocationLength = (UCHAR)bufLength;
        srb.CdbLength = 6;
    } else {

        bufLength = sizeof(CDVD_CAPABILITIES_PAGE) +
                    sizeof(MODE_PARAMETER_HEADER10);

        capPageOffset = sizeof(MODE_PARAMETER_HEADER10);

        cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
        cdb->MODE_SENSE10.Dbd = 1;
        cdb->MODE_SENSE10.PageCode = MODE_PAGE_CAPABILITIES;
        cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(bufLength >> 8);
        cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(bufLength >> 0);
        srb.CdbLength = 10;
    }

    //
    // Set timeout value from device extension.
    //
    srb.TimeOutValue = fdoExtension->TimeOutValue;

    modePageHeader = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                           bufLength,
                                           CDROM_TAG_MODE_DATA);
    if (modePageHeader) {

        RtlZeroMemory(modePageHeader, bufLength);

        status = ClassSendSrbSynchronous(
                     DeviceObject,
                     &srb,
                     modePageHeader,
                     bufLength,
                     FALSE);

        if (NT_SUCCESS(status) ||
            (status == STATUS_DATA_OVERRUN) ||
            (status == STATUS_BUFFER_OVERFLOW)
            ) {

            capPage = (PCDVD_CAPABILITIES_PAGE) (((PUCHAR) modePageHeader) + capPageOffset);

            if ((capPage->PageCode == MODE_PAGE_CAPABILITIES) &&
                (capPage->DVDROMRead || capPage->DVDRRead ||
                 capPage->DVDRAMRead || capPage->DVDRWrite ||
                 capPage->DVDRAMWrite)) {

                deviceType = FILE_DEVICE_DVD;
            }
        }
        ExFreePool (modePageHeader);
    }

    return deviceType;
}

NTSTATUS
NTAPI
CdRomCreateWellKnownName(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine creates a symbolic link to the cdrom device object
    under \dosdevices.  The number of the cdrom device does not neccessarily
    match between \dosdevices and \device, but usually will be the same.

    Saves the buffer

Arguments:

    DeviceObject -

Return Value:

    NTSTATUS

--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PCDROM_DATA cdromData = commonExtension->DriverData;

    UNICODE_STRING unicodeLinkName;
    WCHAR wideLinkName[64];
    PWCHAR savedName;

    LONG cdromNumber = fdoExtension->DeviceNumber;

    NTSTATUS status;

    //
    // if already linked, assert then return
    //

    if (cdromData->WellKnownName.Buffer != NULL) {

        TraceLog((CdromDebugError,
                    "CdRomCreateWellKnownName: link already exists %p\n",
                    cdromData->WellKnownName.Buffer));
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;

    }

    //
    // find an unused CdRomNN to link to
    //

    do {

        swprintf(wideLinkName, L"\\DosDevices\\CdRom%d", cdromNumber);
        RtlInitUnicodeString(&unicodeLinkName, wideLinkName);
        status = IoCreateSymbolicLink(&unicodeLinkName,
                                      &(commonExtension->DeviceName));

        cdromNumber++;

    } while((status == STATUS_OBJECT_NAME_COLLISION) ||
            (status == STATUS_OBJECT_NAME_EXISTS));

    if (!NT_SUCCESS(status)) {

        TraceLog((CdromDebugWarning,
                    "CdRomCreateWellKnownName: Error %lx linking %wZ to "
                    "device %wZ\n",
                    status,
                    &unicodeLinkName,
                    &(commonExtension->DeviceName)));
        return status;

    }

    TraceLog((CdromDebugWarning,
                "CdRomCreateWellKnownName: successfully linked %wZ "
                "to device %wZ\n",
                &unicodeLinkName,
                &(commonExtension->DeviceName)));

    //
    // Save away the symbolic link name in the driver data block.  We need
    // it so we can delete the link when the device is removed.
    //

    savedName = ExAllocatePoolWithTag(PagedPool,
                                      unicodeLinkName.MaximumLength,
                                      CDROM_TAG_STRINGS);

    if (savedName == NULL) {
        IoDeleteSymbolicLink(&unicodeLinkName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(savedName,
                  unicodeLinkName.Buffer,
                  unicodeLinkName.MaximumLength);

    RtlInitUnicodeString(&(cdromData->WellKnownName), savedName);

    //
    // the name was saved and the link created
    //

    return STATUS_SUCCESS;
}

VOID
NTAPI
CdRomDeleteWellKnownName(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PCDROM_DATA cdromData = commonExtension->DriverData;

    if(cdromData->WellKnownName.Buffer != NULL) {

        IoDeleteSymbolicLink(&(cdromData->WellKnownName));
        ExFreePool(cdromData->WellKnownName.Buffer);
        cdromData->WellKnownName.Buffer = NULL;
        cdromData->WellKnownName.Length = 0;
        cdromData->WellKnownName.MaximumLength = 0;

    }
    return;
}

NTSTATUS
NTAPI
CdRomGetDeviceParameter (
    IN     PDEVICE_OBJECT      Fdo,
    IN     PWSTR               ParameterName,
    IN OUT PULONG              ParameterValue
    )
/*++

Routine Description:

    retrieve a devnode registry parameter

Arguments:

    DeviceObject - Cdrom Device Object

    ParameterName - parameter name to look up

    ParameterValuse - default parameter value

Return Value:

    NT Status

--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    NTSTATUS                 status;
    HANDLE                   deviceParameterHandle;
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    ULONG                    defaultParameterValue;

    PAGED_CODE();

    //
    // open the given parameter
    //
    status = IoOpenDeviceRegistryKey(fdoExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     KEY_READ,
                                     &deviceParameterHandle);

    if(NT_SUCCESS(status)) {

        RtlZeroMemory(queryTable, sizeof(queryTable));

        defaultParameterValue = *ParameterValue;

        queryTable->Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
        queryTable->Name          = ParameterName;
        queryTable->EntryContext  = ParameterValue;
        queryTable->DefaultType   = REG_NONE;
        queryTable->DefaultData   = NULL;
        queryTable->DefaultLength = 0;

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR) deviceParameterHandle,
                                        queryTable,
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {

            *ParameterValue = defaultParameterValue;
        }

        //
        // close what we open
        //
        ZwClose(deviceParameterHandle);
    }

    return status;

} // CdRomGetDeviceParameter

NTSTATUS
NTAPI
CdRomSetDeviceParameter (
    IN PDEVICE_OBJECT Fdo,
    IN PWSTR          ParameterName,
    IN ULONG          ParameterValue
    )
/*++

Routine Description:

    save a devnode registry parameter

Arguments:

    DeviceObject - Cdrom Device Object

    ParameterName - parameter name

    ParameterValuse - parameter value

Return Value:

    NT Status

--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    NTSTATUS                 status;
    HANDLE                   deviceParameterHandle;

    PAGED_CODE();

    //
    // open the given parameter
    //
    status = IoOpenDeviceRegistryKey(fdoExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     KEY_READ | KEY_WRITE,
                                     &deviceParameterHandle);

    if(NT_SUCCESS(status)) {

        status = RtlWriteRegistryValue(
                    RTL_REGISTRY_HANDLE,
                    (PWSTR) deviceParameterHandle,
                    ParameterName,
                    REG_DWORD,
                    &ParameterValue,
                    sizeof (ParameterValue));

        //
        // close what we open
        //
        ZwClose(deviceParameterHandle);
    }

    return status;

} // CdromSetDeviceParameter

VOID
NTAPI
CdRomPickDvdRegion(
    IN PDEVICE_OBJECT Fdo
    )
/*++

Routine Description:

    pick a default dvd region

Arguments:

    DeviceObject - Cdrom Device Object

Return Value:

    NT Status

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata = (PCDROM_DATA)(commonExtension->DriverData);

    //
    // these five pointers all point to dvdReadStructure or part of
    // its data, so don't deallocate them more than once!
    //

    PDVD_READ_STRUCTURE dvdReadStructure;
    PDVD_COPY_PROTECT_KEY copyProtectKey;
    PDVD_COPYRIGHT_DESCRIPTOR dvdCopyRight;
    PDVD_RPC_KEY rpcKey;
    PDVD_SET_RPC_KEY dvdRpcKey;

    IO_STATUS_BLOCK ioStatus;
    ULONG bufferLen;
    UCHAR mediaRegion;
    ULONG pickDvdRegion;
    ULONG defaultDvdRegion;
    ULONG dvdRegion;
    ULONG a, b;


    PAGED_CODE();

    if ((pickDvdRegion = InterlockedExchange(&cddata->PickDvdRegion, 0)) == 0) {

        //
        // it was non-zero, so either another thread will do this, or
        // we no longer need to pick a region
        //

        return;
    }

    //
    // short-circuit if license agreement violated
    //

    if (cddata->DvdRpc0LicenseFailure) {
        TraceLog((CdromDebugWarning,
                    "DVD License failure.  Refusing to pick a region\n"));
        InterlockedExchange(&cddata->PickDvdRegion, 0);
        return;
    }



    a = max(sizeof(DVD_DESCRIPTOR_HEADER) +
                            sizeof(DVD_COPYRIGHT_DESCRIPTOR),
                        sizeof(DVD_READ_STRUCTURE)
                        );
                        b = max(DVD_RPC_KEY_LENGTH,
                        DVD_SET_RPC_KEY_LENGTH
                        );
    bufferLen = max(a, b);

    dvdReadStructure = (PDVD_READ_STRUCTURE)
        ExAllocatePoolWithTag(PagedPool, bufferLen, DVD_TAG_DVD_REGION);

    if (dvdReadStructure == NULL) {
        InterlockedExchange(&cddata->PickDvdRegion, pickDvdRegion);
        return;
    }

    if (cddata->DvdRpc0Device && cddata->Rpc0RetryRegistryCallback) {

        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): now retrying RPC0 callback\n",
                    Fdo));

        //
        // get the registry settings again
        //

        ioStatus.Status = CdRomGetRpc0Settings(Fdo);

        if (ioStatus.Status == STATUS_LICENSE_VIOLATION) {

            //
            // if this is the returned error, then
            // the routine should have set this!
            //

            ASSERT(cddata->DvdRpc0LicenseFailure);
            cddata->DvdRpc0LicenseFailure = 1;
            TraceLog((CdromDebugWarning,
                        "CdRomPickDvdRegion (%p): "
                        "setting to fail all dvd ioctls due to CSS licensing "
                        "failure.\n", Fdo));

            pickDvdRegion = 0;
            goto getout;

        }

        //
        // get the device region, again
        //

        copyProtectKey = (PDVD_COPY_PROTECT_KEY)dvdReadStructure;
        RtlZeroMemory(copyProtectKey, bufferLen);
        copyProtectKey->KeyLength = DVD_RPC_KEY_LENGTH;
        copyProtectKey->KeyType = DvdGetRpcKey;

        //
        // Build a request for READ_KEY
        //

        ClassSendDeviceIoControlSynchronous(
            IOCTL_DVD_READ_KEY,
            Fdo,
            copyProtectKey,
            DVD_RPC_KEY_LENGTH,
            DVD_RPC_KEY_LENGTH,
            FALSE,
            &ioStatus);

        if (!NT_SUCCESS(ioStatus.Status)) {
            TraceLog((CdromDebugWarning,
                        "CdRomPickDvdRegion: Unable to get "
                        "device RPC data (%x)\n", ioStatus.Status));
            pickDvdRegion = 0;
            goto getout;
        }

        //
        // now that we have gotten the device's RPC data,
        // we have set the device extension to usable data.
        // no need to call back into this section of code again
        //

        cddata->Rpc0RetryRegistryCallback = 0;


        rpcKey = (PDVD_RPC_KEY) copyProtectKey->KeyData;

        //
        // TypeCode of zero means that no region has been set.
        //

        if (rpcKey->TypeCode != 0) {
            TraceLog((CdromDebugWarning,
                        "CdRomPickDvdRegion (%p): DVD Region already "
                        "chosen\n", Fdo));
            pickDvdRegion = 0;
            goto getout;
        }

        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): must choose initial DVD "
                    " Region\n", Fdo));
    }



    copyProtectKey = (PDVD_COPY_PROTECT_KEY) dvdReadStructure;

    dvdCopyRight = (PDVD_COPYRIGHT_DESCRIPTOR)
        ((PDVD_DESCRIPTOR_HEADER) dvdReadStructure)->Data;

    //
    // get the media region
    //

    RtlZeroMemory (dvdReadStructure, bufferLen);
    dvdReadStructure->Format = DvdCopyrightDescriptor;

    //
    // Build and send a request for READ_KEY
    //

    TraceLog((CdromDebugTrace,
                "CdRomPickDvdRegion (%p): Getting Copyright Descriptor\n",
                Fdo));

    ClassSendDeviceIoControlSynchronous(
        IOCTL_DVD_READ_STRUCTURE,
        Fdo,
        dvdReadStructure,
        sizeof(DVD_READ_STRUCTURE),
        sizeof (DVD_DESCRIPTOR_HEADER) +
        sizeof(DVD_COPYRIGHT_DESCRIPTOR),
        FALSE,
        &ioStatus
        );
    TraceLog((CdromDebugTrace,
                "CdRomPickDvdRegion (%p): Got Copyright Descriptor %x\n",
                Fdo, ioStatus.Status));

    if ((NT_SUCCESS(ioStatus.Status)) &&
        (dvdCopyRight->CopyrightProtectionType == 0x01)
        ) {

        //
        // keep the media region bitmap around
        // a 1 means ok to play
        //

        if (dvdCopyRight->RegionManagementInformation == 0xff) {
            TraceLog((CdromDebugError,
                      "CdRomPickDvdRegion (%p): RegionManagementInformation "
                      "is set to dis-allow playback for all regions.  This is "
                      "most likely a poorly authored disc.  defaulting to all "
                      "region disc for purpose of choosing initial region\n",
                      Fdo));
            dvdCopyRight->RegionManagementInformation = 0;
        }


        mediaRegion = ~dvdCopyRight->RegionManagementInformation;

    } else {

        //
        // could be media, can't set the device region
        //

        if (!cddata->DvdRpc0Device) {

            //
            // can't automatically pick a default region on a rpc2 drive
            // without media, so just exit
            //
            TraceLog((CdromDebugWarning,
                        "CdRomPickDvdRegion (%p): failed to auto-choose "
                        "a region due to status %x getting copyright "
                        "descriptor\n", Fdo, ioStatus.Status));
            goto getout;

        } else {

            //
            // for an RPC0 drive, we can try to pick a region for
            // the drive
            //

            mediaRegion = 0x0;
        }

    }

    //
    // get the device region
    //

    RtlZeroMemory (copyProtectKey, bufferLen);
    copyProtectKey->KeyLength = DVD_RPC_KEY_LENGTH;
    copyProtectKey->KeyType = DvdGetRpcKey;

    //
    // Build and send a request for READ_KEY for RPC key
    //

    TraceLog((CdromDebugTrace,
                "CdRomPickDvdRegion (%p): Getting RpcKey\n",
                Fdo));
    ClassSendDeviceIoControlSynchronous(
        IOCTL_DVD_READ_KEY,
        Fdo,
        copyProtectKey,
        DVD_RPC_KEY_LENGTH,
        DVD_RPC_KEY_LENGTH,
        FALSE,
        &ioStatus
        );
    TraceLog((CdromDebugTrace,
                "CdRomPickDvdRegion (%p): Got RpcKey %x\n",
                Fdo, ioStatus.Status));

    if (!NT_SUCCESS(ioStatus.Status)) {

        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): failed to get RpcKey from "
                    "a DVD Device\n", Fdo));
        goto getout;

    }

    //
    // so we now have what we can get for the media region and the
    // drive region.  we will not set a region if the drive has one
    // set already (mask is not all 1's), nor will we set a region
    // if there are no more user resets available.
    //

    rpcKey = (PDVD_RPC_KEY) copyProtectKey->KeyData;


    if (rpcKey->RegionMask != 0xff) {
        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): not picking a region since "
                    "it is already chosen\n", Fdo));
        goto getout;
    }

    if (rpcKey->UserResetsAvailable <= 1) {
        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): not picking a region since "
                    "only one change remains\n", Fdo));
        goto getout;
    }

    defaultDvdRegion = 0;

    //
    // the proppage dvd class installer sets
    // this key based upon the system locale
    //

    CdRomGetDeviceParameter (
        Fdo,
        DVD_DEFAULT_REGION,
        &defaultDvdRegion
        );

    if (defaultDvdRegion > DVD_MAX_REGION) {

        //
        // the registry has a bogus default
        //

        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): registry has a bogus default "
                    "region value of %x\n", Fdo, defaultDvdRegion));
        defaultDvdRegion = 0;

    }

    //
    // if defaultDvdRegion == 0, it means no default.
    //

    //
    // we will select the initial dvd region for the user
    //

    if ((defaultDvdRegion != 0) &&
        (mediaRegion &
         (1 << (defaultDvdRegion - 1))
         )
        ) {

        //
        // first choice:
        // the media has region that matches
        // the default dvd region.
        //

        dvdRegion = (1 << (defaultDvdRegion - 1));

        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): Choice #1: media matches "
                    "drive's default, chose region %x\n", Fdo, dvdRegion));


    } else if (mediaRegion) {

        //
        // second choice:
        // pick the lowest region number
        // from the media
        //

        UCHAR mask;

        mask = 1;
        dvdRegion = 0;
        while (mediaRegion && !dvdRegion) {

            //
            // pick the lowest bit
            //
            dvdRegion = mediaRegion & mask;
            mask <<= 1;
        }

        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): Choice #2: choosing lowest "
                    "media region %x\n", Fdo, dvdRegion));

    } else if (defaultDvdRegion) {

        //
        // third choice:
        // default dvd region from the dvd class installer
        //

        dvdRegion = (1 << (defaultDvdRegion - 1));
        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): Choice #3: using default "
                    "region for this install %x\n", Fdo, dvdRegion));

    } else {

        //
        // unable to pick one for the user -- this should rarely
        // happen, since the proppage dvd class installer sets
        // the key based upon the system locale
        //
        TraceLog((CdromDebugWarning,
                    "CdRomPickDvdRegion (%p): Choice #4: failed to choose "
                    "a media region\n", Fdo));
        goto getout;

    }

    //
    // now that we've chosen a region, set the region by sending the
    // appropriate request to the drive
    //

    RtlZeroMemory (copyProtectKey, bufferLen);
    copyProtectKey->KeyLength = DVD_SET_RPC_KEY_LENGTH;
    copyProtectKey->KeyType = DvdSetRpcKey;
    dvdRpcKey = (PDVD_SET_RPC_KEY) copyProtectKey->KeyData;
    dvdRpcKey->PreferredDriveRegionCode = (UCHAR) ~dvdRegion;

    //
    // Build and send request for SEND_KEY
    //
    TraceLog((CdromDebugTrace,
                "CdRomPickDvdRegion (%p): Sending new Rpc Key to region %x\n",
                Fdo, dvdRegion));

    ClassSendDeviceIoControlSynchronous(
        IOCTL_DVD_SEND_KEY2,
        Fdo,
        copyProtectKey,
        DVD_SET_RPC_KEY_LENGTH,
        0,
        FALSE,
        &ioStatus);
    TraceLog((CdromDebugTrace,
                "CdRomPickDvdRegion (%p): Sent new Rpc Key %x\n",
                Fdo, ioStatus.Status));

    if (!NT_SUCCESS(ioStatus.Status)) {
        DebugPrint ((1, "CdRomPickDvdRegion (%p): unable to set dvd initial "
                     " region code (%p)\n", Fdo, ioStatus.Status));
    } else {
        DebugPrint ((1, "CdRomPickDvdRegion (%p): Successfully set dvd "
                     "initial region\n", Fdo));
        pickDvdRegion = 0;
    }

getout:
    if (dvdReadStructure) {
        ExFreePool (dvdReadStructure);
    }

    //
    // update the new PickDvdRegion value
    //

    InterlockedExchange(&cddata->PickDvdRegion, pickDvdRegion);

    return;
}

NTSTATUS
NTAPI
CdRomRetryRequest(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PIRP Irp,
    IN ULONG Delay,
    IN BOOLEAN ResendIrp
    )
{
    PCDROM_DATA cdData;
    KIRQL oldIrql;

    if(Delay == 0) {
        return CdRomRerunRequest(FdoExtension, Irp, ResendIrp);
    }

    cdData = FdoExtension->CommonExtension.DriverData;

    KeAcquireSpinLock(&(cdData->DelayedRetrySpinLock), &oldIrql);

    ASSERT(cdData->DelayedRetryIrp == NULL);
    ASSERT(cdData->DelayedRetryInterval == 0);

    cdData->DelayedRetryIrp = Irp;
    cdData->DelayedRetryInterval = Delay;
    cdData->DelayedRetryResend = ResendIrp;

    KeReleaseSpinLock(&(cdData->DelayedRetrySpinLock), oldIrql);

    return STATUS_PENDING;
}

NTSTATUS
NTAPI
CdRomRerunRequest(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN OPTIONAL PIRP Irp,
    IN BOOLEAN ResendIrp
    )
{
    if(ResendIrp) {
        return IoCallDriver(FdoExtension->CommonExtension.LowerDeviceObject,
                            Irp);
    } else {
        KIRQL oldIrql;

        oldIrql = KeRaiseIrqlToDpcLevel();
        CdRomStartIo(FdoExtension->DeviceObject, Irp);
        KeLowerIrql(oldIrql);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
}

/*++

Routine Description:

    This routine just checks for media change sense/asc/ascq and
    also for other events, such as bus resets.  this is used to
    determine if the device behaviour has changed, to allow for
    read and write operations to be allowed and/or disallowed.

Arguments:

    ISSUE-2000/3/30-henrygab - not fully doc'd

Return Value:

    NTSTATUS

--*/

VOID
NTAPI
CdRomMmcErrorHandler(
    IN PDEVICE_OBJECT Fdo,
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT PNTSTATUS Status,
    OUT PBOOLEAN Retry
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    //BOOLEAN queryCapabilities = FALSE;

    if (TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) {
        
        PCDROM_DATA cddata = (PCDROM_DATA)commonExtension->DriverData;
        PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;

        //
        // the following sense keys could indicate a change in
        // capabilities.
        //

        //
        // we used to expect this to be serialized, and only hit from our
        // own routine. we now allow some requests to continue during our
        // processing of the capabilities update in order to allow
        // IoReadPartitionTable() to succeed.
        //

        switch (senseBuffer->SenseKey & 0xf) {
        
        case SCSI_SENSE_NOT_READY: {
            if (senseBuffer->AdditionalSenseCode ==
                SCSI_ADSENSE_NO_MEDIA_IN_DEVICE) {
                
                if (cddata->Mmc.WriteAllowed) {
                    KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                               "CdromErrorHandler: media removed, writes will be "
                               "failed until new media detected\n"));
                }

                // NOTE - REF #0002
                cddata->Mmc.WriteAllowed = FALSE;
            } else
            if ((senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
                (senseBuffer->AdditionalSenseCodeQualifier ==
                 SCSI_SENSEQ_BECOMING_READY)) {
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "CdromErrorHandler: media becoming ready, "
                           "SHOULD notify shell of change time by sending "
                           "GESN request immediately!\n"));
            }
            break;
        } // end SCSI_SENSE_NOT_READY

        case SCSI_SENSE_UNIT_ATTENTION: {
            switch (senseBuffer->AdditionalSenseCode) {
            case SCSI_ADSENSE_MEDIUM_CHANGED: {
                
                //
                // always update if the medium may have changed
                //
                
                // NOTE - REF #0002
                cddata->Mmc.WriteAllowed = FALSE;
                InterlockedCompareExchange(&(cddata->Mmc.UpdateState),
                                           CdromMmcUpdateRequired,
                                           CdromMmcUpdateComplete);
    
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "CdromErrorHandler: media change detected, need to "
                           "update drive capabilities\n"));
                break;

            } // end SCSI_ADSENSE_MEDIUM_CHANGED
            
            case SCSI_ADSENSE_BUS_RESET: {
                
                // NOTE - REF #0002
                cddata->Mmc.WriteAllowed = FALSE;
                InterlockedCompareExchange(&(cddata->Mmc.UpdateState),
                                           CdromMmcUpdateRequired,
                                           CdromMmcUpdateComplete);

                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "CdromErrorHandler: bus reset detected, need to "
                           "update drive capabilities\n"));
                break;

            } // end SCSI_ADSENSE_BUS_RESET

            case SCSI_ADSENSE_OPERATOR_REQUEST: {

                BOOLEAN b = FALSE;
                
                switch (senseBuffer->AdditionalSenseCodeQualifier) {
                case SCSI_SENSEQ_MEDIUM_REMOVAL: {
                                        
                    //
                    // eject notification currently handled by classpnp
                    //

                    KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                               "CdromErrorHandler: Eject requested by user\n"));
                    *Retry = TRUE;
                    *Status = STATUS_DEVICE_BUSY;
                    break;
                }

                case SCSI_SENSEQ_WRITE_PROTECT_DISABLE:
                    b = TRUE;
                case SCSI_SENSEQ_WRITE_PROTECT_ENABLE: {
                    
                    KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                               "CdromErrorHandler: Write protect %s requested "
                               "by user\n",
                               (b ? "disable" : "enable")));
                    *Retry = TRUE;
                    *Status = STATUS_DEVICE_BUSY;
                    // NOTE - REF #0002
                    cddata->Mmc.WriteAllowed = FALSE;
                    InterlockedCompareExchange(&(cddata->Mmc.UpdateState),
                                               CdromMmcUpdateRequired,
                                               CdromMmcUpdateComplete);

                }

                } // end of AdditionalSenseCodeQualifier switch


                break;

            } // end SCSI_ADSENSE_OPERATOR_REQUEST

            default: {
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "CdromErrorHandler: Unit attention %02x/%02x\n",
                           senseBuffer->AdditionalSenseCode,
                           senseBuffer->AdditionalSenseCodeQualifier));
                break;
            }

            } // end of AdditionSenseCode switch
            break;

        } // end SCSI_SENSE_UNIT_ATTENTION

        case SCSI_SENSE_ILLEGAL_REQUEST: {
            if (senseBuffer->AdditionalSenseCode ==
                SCSI_ADSENSE_WRITE_PROTECT) {
                
                if (cddata->Mmc.WriteAllowed) {
                    KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                               "CdromErrorHandler: media was writable, but "
                               "failed request with WRITE_PROTECT error...\n"));
                }
                // NOTE - REF #0002
                // do not update all the capabilities just because
                // we can't write to the disc.
                cddata->Mmc.WriteAllowed = FALSE;
            }
            break;
        } // end SCSI_SENSE_ILLEGAL_REQUEST

        } // end of SenseKey switch

    } // end of SRB_STATUS_AUTOSENSE_VALID
}

/*++

Routine Description:

    This routine checks for a device-specific error handler
    and calls it if it exists.  This allows multiple drives
    that require their own error handler to co-exist.

--*/
VOID
NTAPI
CdRomErrorHandler(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PCDROM_DATA cddata = (PCDROM_DATA)commonExtension->DriverData;
    PSENSE_DATA sense = Srb->SenseInfoBuffer;

    if ((Srb->SenseInfoBufferLength >=
         RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA,AdditionalSenseCodeQualifier)) &&
        TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) {

        //
        //  Many non-WHQL certified drives (mostly CD-RW) return
        //  2/4/0 when they have no media instead of the obvious
        //  choice of:
        //
        //      SCSI_SENSE_NOT_READY/SCSI_ADSENSE_NO_MEDIA_IN_DEVICE
        //
        //  These drives should not pass WHQL certification due
        //  to this discrepency.
        //
        //  However, we have to retry on 2/4/0 (Not ready, LUN not ready,
        //  no info) and also 3/2/0 (no seek complete).
        //
        //  These conditions occur when the shell tries to examine an
        //  injected CD (e.g. for autoplay) before the CD is spun up.
        //
        //  The drive should be returning an ASCQ of SCSI_SENSEQ_BECOMING_READY
        //  (0x01) in order to comply with WHQL standards.
        //
        //  The default retry timeout of one second is acceptable to balance
        //  these discrepencies.  don't modify the status, though....
        //

        if (((sense->SenseKey & 0xf) == SCSI_SENSE_NOT_READY) &&
            (sense->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
            (sense->AdditionalSenseCodeQualifier == SCSI_SENSEQ_CAUSE_NOT_REPORTABLE)
            ) {

            *Retry = TRUE;

        } else if (((sense->SenseKey & 0xf) == SCSI_SENSE_MEDIUM_ERROR) &&
                   (sense->AdditionalSenseCode == 0x2) &&
                   (sense->AdditionalSenseCodeQualifier == 0x0)
                   ) {

            *Retry = TRUE;

        } else if ((sense->AdditionalSenseCode == 0x57) &&
                   (sense->AdditionalSenseCodeQualifier == 0x00)
                   ) {

            //
            // UNABLE_TO_RECOVER_TABLE_OF_CONTENTS
            // the Matshita CR-585 returns this for all read commands
            // on blank CD-R and CD-RW media, and we need to handle
            // this for READ_CD detection ability.
            //
            
            *Retry = FALSE;
            *Status = STATUS_UNRECOGNIZED_MEDIA;

        }

    }

    //
    // tail recursion in both cases takes no stack
    //

    if (cddata->ErrorHandler) {
        cddata->ErrorHandler(DeviceObject, Srb, Status, Retry);
    }
    return;
}

/*++

Routine Description:

    This routine is called for a shutdown and flush IRPs.
    These are sent by the system before it actually shuts
    down or when the file system does a flush.

Arguments:

    DriverObject - Pointer to device object to being shutdown by system.

    Irp - IRP involved.

Return Value:

    NT Status

--*/
NTSTATUS
NTAPI
CdRomShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    IoMarkIrpPending(Irp);
    IoStartPacket(DeviceObject, Irp, NULL, NULL);
    return STATUS_PENDING;

}

/*++

Routine Description:

    This routine is called for intermediate work a shutdown or
    flush IRPs would need to do.  We just want to free our resources
    and return STATUS_MORE_PROCESSING_REQUIRED.

Arguments:

    DeviceObject - NULL?

    Irp - IRP to free
    
    Context - NULL

Return Value:

    NT Status

--*/
NTSTATUS
NTAPI
CdRomShutdownFlushCompletion(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP NewIrp,
    IN PIRP OriginalIrp 
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION originalIrpStack;
    ULONG_PTR iteration;
    NTSTATUS status = STATUS_SUCCESS;
    
    ASSERT(OriginalIrp);

    originalIrpStack = IoGetCurrentIrpStackLocation(OriginalIrp);

    //
    // always use a new irp so we can call
    // CdRomCompleteIrpAndStartNextPacketSafely() from this routine.
    //
    
    if (NewIrp != NULL) {
        status = NewIrp->IoStatus.Status;
        IoFreeIrp(NewIrp);
        NewIrp = NULL;
    }

    if (!NT_SUCCESS(status)) {
        BAIL_OUT(OriginalIrp);
        goto SafeExit;
    }
    
    //
    // the current irpstack saves the counter which states
    // what part of the multi-part shutdown or flush we are in.
    //

    iteration = (ULONG_PTR)originalIrpStack->Parameters.Others.Argument1;
    iteration++;
    originalIrpStack->Parameters.Others.Argument1 = (PVOID)iteration;

    switch (iteration) {
    case 2:
        if (originalIrpStack->MajorFunction != IRP_MJ_SHUTDOWN) {
            //
            // then we don't want to send the unlock command
            // the incrementing of the state was done above.
            // return the completion routine's result.
            //
            return CdRomShutdownFlushCompletion(Fdo, NULL, OriginalIrp);
        }
        // else fall through....

    case 1: {
        
        PIRP                newIrp = NULL;
        PSCSI_REQUEST_BLOCK newSrb = NULL;
        PCDB                newCdb = NULL;
        PIO_STACK_LOCATION  newIrpStack = NULL;
        ULONG               isRemoved;

        newIrp = IoAllocateIrp((CCHAR)(Fdo->StackSize+1), FALSE);
        if (newIrp == NULL) {
            BAIL_OUT(OriginalIrp);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto SafeExit;
        }
        newSrb = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(SCSI_REQUEST_BLOCK),
                                        CDROM_TAG_SRB);
        if (newSrb == NULL) {
            IoFreeIrp(newIrp);
            BAIL_OUT(OriginalIrp);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto SafeExit;
        }
        
        //
        // ClassIoComplete will free the SRB, but we need a routine
        // that will free the irp.  then just call ClassSendAsync,
        // and don't care about the return value, since the completion
        // routine will be called anyways.
        //

        IoSetNextIrpStackLocation(newIrp);
        newIrpStack = IoGetCurrentIrpStackLocation(newIrp);
        newIrpStack->DeviceObject = Fdo;
        IoSetCompletionRoutine(newIrp,
                               (PIO_COMPLETION_ROUTINE)CdRomShutdownFlushCompletion,
                               OriginalIrp,
                               TRUE, TRUE, TRUE);
        IoSetNextIrpStackLocation(newIrp);
        newIrpStack = IoGetCurrentIrpStackLocation(newIrp);
        newIrpStack->DeviceObject = Fdo;

        //
        // setup the request
        //

        RtlZeroMemory(newSrb, sizeof(SCSI_REQUEST_BLOCK));
        newCdb = (PCDB)(newSrb->Cdb);
        
        newSrb->QueueTag = SP_UNTAGGED;
        newSrb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
        newSrb->Function = SRB_FUNCTION_EXECUTE_SCSI;

        //
        // tell classpnp not to call StartNextPacket()
        //

        newSrb->SrbFlags = SRB_FLAGS_DONT_START_NEXT_PACKET;

        if (iteration == 1) {

            //
            // first synchronize the cache
            //
            
            newSrb->TimeOutValue = fdoExtension->TimeOutValue * 4;
            newSrb->CdbLength = 10;
            newCdb->SYNCHRONIZE_CACHE10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;
            
        } else if (iteration == 2) {

            //
            // then unlock the medium
            //

            ASSERT( originalIrpStack->MajorFunction == IRP_MJ_SHUTDOWN );
            
            newSrb->TimeOutValue = fdoExtension->TimeOutValue;
            newSrb->CdbLength = 6;
            newCdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            newCdb->MEDIA_REMOVAL.Prevent = FALSE;
            
        }


        isRemoved = ClassAcquireRemoveLock(Fdo, newIrp);
        if (isRemoved) {
            IoFreeIrp(newIrp);
            ExFreePool(newSrb);
            ClassReleaseRemoveLock(Fdo, newIrp);
            BAIL_OUT(OriginalIrp);
            status = STATUS_DEVICE_DOES_NOT_EXIST;
            goto SafeExit;
        }
        ClassSendSrbAsynchronous(Fdo, newSrb, newIrp, NULL, 0, FALSE);
        break;
    }

    case 3: {

        PSCSI_REQUEST_BLOCK srb;
        PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(OriginalIrp);
        
        //
        // forward this request to the device appropriately,
        // don't use this completion routine anymore...
        //
        
        srb = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(SCSI_REQUEST_BLOCK),
                                    CDROM_TAG_SRB);
        if (srb == NULL) {
            BAIL_OUT(OriginalIrp);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto SafeExit;
        }

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->TimeOutValue = fdoExtension->TimeOutValue * 4;
        srb->QueueTag = SP_UNTAGGED;
        srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
        srb->SrbFlags = fdoExtension->SrbFlags;
        srb->CdbLength = 0;
        srb->OriginalRequest = OriginalIrp;

        if (originalIrpStack->MajorFunction == IRP_MJ_SHUTDOWN) {
            srb->Function = SRB_FUNCTION_SHUTDOWN;
        } else {
            srb->Function = SRB_FUNCTION_FLUSH;
        }

        //
        // Set up IoCompletion routine address.
        //

        IoSetCompletionRoutine(OriginalIrp,
                               ClassIoComplete,
                               srb,
                               TRUE, TRUE, TRUE);

        //
        // Set the retry count to zero.
        //

        originalIrpStack->Parameters.Others.Argument4 = (PVOID) 0;

        //
        // Get next stack location and set major function code.
        //

        nextIrpStack->MajorFunction = IRP_MJ_SCSI;

        //
        // Set up SRB for execute scsi request.
        // Save SRB address in next stack for port driver.
        //

        nextIrpStack->Parameters.Scsi.Srb = srb;

        //
        // Call the port driver to process the request.
        //

        IoCallDriver(commonExtension->LowerDeviceObject, OriginalIrp);
        
        break;

    }
    default: {
        ASSERT(FALSE);
        break;
    }

    } // end switch
    
    status = STATUS_SUCCESS;

SafeExit:

    if (!NT_SUCCESS(status)) {
        OriginalIrp->IoStatus.Status = status;
        CdRomCompleteIrpAndStartNextPacketSafely(Fdo, OriginalIrp);
    }

    //
    // always return STATUS_MORE_PROCESSING_REQUIRED, so noone else tries
    // to access the new irp that we free'd....
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // end CdromShutdownFlush()

VOID
NTAPI
CdromFakePartitionInfo(
    IN PCOMMON_DEVICE_EXTENSION CommonExtension,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioctl = currentIrpStack->Parameters.DeviceIoControl.IoControlCode;
    PVOID systemBuffer = Irp->AssociatedIrp.SystemBuffer;

    ASSERT(systemBuffer);

    if ((ioctl != IOCTL_DISK_GET_DRIVE_LAYOUT) &&
        (ioctl != IOCTL_DISK_GET_DRIVE_LAYOUT_EX) &&
        (ioctl != IOCTL_DISK_GET_PARTITION_INFO) &&
        (ioctl != IOCTL_DISK_GET_PARTITION_INFO_EX)) {
        TraceLog((CdromDebugError,
                    "CdromFakePartitionInfo: unhandled ioctl %x\n", ioctl));
        Irp->IoStatus.Status = STATUS_INTERNAL_ERROR;
        Irp->IoStatus.Information = 0;
        CdRomCompleteIrpAndStartNextPacketSafely(CommonExtension->DeviceObject,
                                                 Irp);
        return;
    }

    //
    // nothing to fail from this point on, so set the size appropriately
    // and set irp's status to success.
    //

    TraceLog((CdromDebugWarning,
                "CdromFakePartitionInfo: incoming ioctl %x\n", ioctl));


    Irp->IoStatus.Status = STATUS_SUCCESS;
    switch (ioctl) {
    case IOCTL_DISK_GET_DRIVE_LAYOUT:
        Irp->IoStatus.Information = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION,
                                                 PartitionEntry[1]);
        RtlZeroMemory(systemBuffer, FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION,
                                                 PartitionEntry[1]));
        break;
    case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
        Irp->IoStatus.Information = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX,
                                                 PartitionEntry[1]);
        RtlZeroMemory(systemBuffer, FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX,
                                                 PartitionEntry[1]));
        break;
    case IOCTL_DISK_GET_PARTITION_INFO:
        Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
        RtlZeroMemory(systemBuffer, sizeof(PARTITION_INFORMATION));
        break;
    case IOCTL_DISK_GET_PARTITION_INFO_EX:
        Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);
        RtlZeroMemory(systemBuffer, sizeof(PARTITION_INFORMATION_EX));
        break;
    default:
        ASSERT(!"Invalid ioctl should not have reached this point\n");
        break;
    }

    //
    // if we are getting the drive layout, then we need to start by
    // adding some of the non-partition stuff that says we have
    // exactly one partition available.
    //


    if (ioctl == IOCTL_DISK_GET_DRIVE_LAYOUT) {
        
        PDRIVE_LAYOUT_INFORMATION layout;
        layout = (PDRIVE_LAYOUT_INFORMATION)systemBuffer;
        layout->PartitionCount = 1;
        layout->Signature = 1;
        systemBuffer = (PVOID)(layout->PartitionEntry);
        ioctl = IOCTL_DISK_GET_PARTITION_INFO;
        
    } else if (ioctl == IOCTL_DISK_GET_DRIVE_LAYOUT_EX) {

        PDRIVE_LAYOUT_INFORMATION_EX layoutEx;
        layoutEx = (PDRIVE_LAYOUT_INFORMATION_EX)systemBuffer;
        layoutEx->PartitionStyle = PARTITION_STYLE_MBR;
        layoutEx->PartitionCount = 1;
        layoutEx->Mbr.Signature = 1;
        systemBuffer = (PVOID)(layoutEx->PartitionEntry);
        ioctl = IOCTL_DISK_GET_PARTITION_INFO_EX;

    }

    //
    // NOTE: the local var 'ioctl' is now modified to either EX or
    // non-EX version. the local var 'systemBuffer' is now pointing
    // to the partition information structure.
    //

    if (ioctl == IOCTL_DISK_GET_PARTITION_INFO) {

        PPARTITION_INFORMATION partitionInfo;
        partitionInfo = (PPARTITION_INFORMATION)systemBuffer;
        partitionInfo->RewritePartition = FALSE;
        partitionInfo->RecognizedPartition = TRUE;
        partitionInfo->PartitionType = PARTITION_FAT32;
        partitionInfo->BootIndicator = FALSE;
        partitionInfo->HiddenSectors = 0;
        partitionInfo->StartingOffset.QuadPart = 0;
        partitionInfo->PartitionLength = CommonExtension->PartitionLength;
        partitionInfo->PartitionNumber = 0;
    
    } else {

        PPARTITION_INFORMATION_EX partitionInfo;
        partitionInfo = (PPARTITION_INFORMATION_EX)systemBuffer;
        partitionInfo->PartitionStyle = PARTITION_STYLE_MBR;
        partitionInfo->RewritePartition = FALSE;
        partitionInfo->Mbr.RecognizedPartition = TRUE;
        partitionInfo->Mbr.PartitionType = PARTITION_FAT32;
        partitionInfo->Mbr.BootIndicator = FALSE;
        partitionInfo->Mbr.HiddenSectors = 0;
        partitionInfo->StartingOffset.QuadPart = 0;
        partitionInfo->PartitionLength = CommonExtension->PartitionLength;
        partitionInfo->PartitionNumber = 0;
    
    }
    TraceLog((CdromDebugWarning,
                "CdromFakePartitionInfo: finishing ioctl %x\n",
                currentIrpStack->Parameters.DeviceIoControl.IoControlCode));

    //
    // complete the irp
    //

    CdRomCompleteIrpAndStartNextPacketSafely(CommonExtension->DeviceObject,
                                             Irp);
    return;

}
