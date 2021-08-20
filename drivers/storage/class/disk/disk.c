/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    disk.c

Abstract:

    SCSI disk class driver

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#define DEBUG_MAIN_SOURCE   1
#include "disk.h"


//
// Now instantiate the GUIDs
//

#include "initguid.h"
#include "ntddstor.h"
#include "ntddvol.h"
#include "ioevent.h"

#ifdef DEBUG_USE_WPP
#include "disk.tmh"
#endif

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DiskUnload)
#pragma alloc_text(PAGE, DiskCreateFdo)
#pragma alloc_text(PAGE, DiskDetermineMediaTypes)
#pragma alloc_text(PAGE, DiskModeSelect)
#pragma alloc_text(PAGE, DisableWriteCache)
#pragma alloc_text(PAGE, DiskSetSpecialHacks)
#pragma alloc_text(PAGE, DiskGetCacheInformation)
#pragma alloc_text(PAGE, DiskSetCacheInformation)
#pragma alloc_text(PAGE, DiskLogCacheInformation)
#pragma alloc_text(PAGE, DiskSetInfoExceptionInformation)
#pragma alloc_text(PAGE, DiskGetInfoExceptionInformation)
#pragma alloc_text(PAGE, DiskIoctlGetCacheSetting)
#pragma alloc_text(PAGE, DiskIoctlSetCacheSetting)
#pragma alloc_text(PAGE, DiskIoctlGetLengthInfo)
#pragma alloc_text(PAGE, DiskIoctlGetDriveGeometry)
#pragma alloc_text(PAGE, DiskIoctlGetDriveGeometryEx)
#pragma alloc_text(PAGE, DiskIoctlGetCacheInformation)
#pragma alloc_text(PAGE, DiskIoctlSetCacheInformation)
#pragma alloc_text(PAGE, DiskIoctlGetMediaTypesEx)
#pragma alloc_text(PAGE, DiskIoctlPredictFailure)
#pragma alloc_text(PAGE, DiskIoctlEnableFailurePrediction)
#pragma alloc_text(PAGE, DiskIoctlReassignBlocks)
#pragma alloc_text(PAGE, DiskIoctlReassignBlocksEx)
#pragma alloc_text(PAGE, DiskIoctlIsWritable)
#pragma alloc_text(PAGE, DiskIoctlUpdateDriveSize)
#pragma alloc_text(PAGE, DiskIoctlGetVolumeDiskExtents)
#pragma alloc_text(PAGE, DiskIoctlSmartGetVersion)
#pragma alloc_text(PAGE, DiskIoctlSmartReceiveDriveData)
#pragma alloc_text(PAGE, DiskIoctlSmartSendDriveCommand)
#pragma alloc_text(PAGE, DiskIoctlVerifyThread)

#endif

//
//  ETW related globals
//
BOOLEAN DiskETWEnabled = FALSE;

BOOLEAN DiskIsPastReinit = FALSE;

const GUID GUID_NULL = { 0 };
#define DiskCompareGuid(_First,_Second) \
    (memcmp ((_First),(_Second), sizeof (GUID)))

//
// This macro is used to work around a bug in the definition of
// DISK_CACHE_RETENTION_PRIORITY.  The value KeepReadData should be
// assigned 0xf rather than 0x2.  Since the interface was already published
// when this was discovered the disk driver has been modified to translate
// between the interface value and the correct scsi value.
//
// 0x2 is turned into 0xf
// 0xf is turned into 0x2 - this ensures that future SCSI defintions can be
//                          accomodated.
//

#define TRANSLATE_RETENTION_PRIORITY(_x)\
        ((_x) == 0xf ?  0x2 :           \
            ((_x) == 0x2 ? 0xf : _x)    \
        )

#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN  CTL_CODE(IOCTL_VOLUME_BASE, 0, METHOD_BUFFERED, FILE_READ_ACCESS)

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskDriverReinit(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Nothing,
    IN ULONG Count
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(Nothing);
    UNREFERENCED_PARAMETER(Count);

    DiskIsPastReinit = TRUE;
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskBootDriverReinit(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID Nothing,
    IN ULONG Count
    )
{
    IoRegisterDriverReinitialization(DriverObject, DiskDriverReinit, NULL);

#if defined(_X86_) || defined(_AMD64_)

    DiskDriverReinitialization(DriverObject, Nothing, Count);

#else

    UNREFERENCED_PARAMETER(Nothing);
    UNREFERENCED_PARAMETER(Count);

#endif

}

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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
    CLASS_INIT_DATA InitializationData = { 0 };
    CLASS_QUERY_WMI_REGINFO_EX_LIST classQueryWmiRegInfoExList = { 0 };
    GUID guidQueryRegInfoEx = GUID_CLASSPNP_QUERY_REGINFOEX;
    GUID guidSrbSupport = GUID_CLASSPNP_SRB_SUPPORT;
    ULONG srbSupport;

    NTSTATUS status;

    //
    // Initializes tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

#if defined(_X86_) || defined(_AMD64_)

    //
    // Read the information NtDetect squirreled away about the disks in this
    // system.
    //

    DiskSaveDetectInfo(DriverObject);

#endif

    InitializationData.InitializationDataSize = sizeof(CLASS_INIT_DATA);

    //
    // Setup sizes and entry points for functional device objects
    //

    InitializationData.FdoData.DeviceExtensionSize   = FUNCTIONAL_EXTENSION_SIZE;
    InitializationData.FdoData.DeviceType            = FILE_DEVICE_DISK;
    InitializationData.FdoData.DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN;

    InitializationData.FdoData.ClassInitDevice    = DiskInitFdo;
    InitializationData.FdoData.ClassStartDevice   = DiskStartFdo;
    InitializationData.FdoData.ClassStopDevice    = DiskStopDevice;
    InitializationData.FdoData.ClassRemoveDevice  = DiskRemoveDevice;
    InitializationData.FdoData.ClassPowerDevice   = ClassSpinDownPowerHandler;

    InitializationData.FdoData.ClassError         = DiskFdoProcessError;
    InitializationData.FdoData.ClassReadWriteVerification = DiskReadWriteVerification;
    InitializationData.FdoData.ClassDeviceControl = DiskDeviceControl;
    InitializationData.FdoData.ClassShutdownFlush = DiskShutdownFlush;
    InitializationData.FdoData.ClassCreateClose   = NULL;


    InitializationData.FdoData.ClassWmiInfo.GuidCount               = 7;
    InitializationData.FdoData.ClassWmiInfo.GuidRegInfo             = DiskWmiFdoGuidList;
    InitializationData.FdoData.ClassWmiInfo.ClassQueryWmiRegInfo    = DiskFdoQueryWmiRegInfo;
    InitializationData.FdoData.ClassWmiInfo.ClassQueryWmiDataBlock  = DiskFdoQueryWmiDataBlock;
    InitializationData.FdoData.ClassWmiInfo.ClassSetWmiDataBlock    = DiskFdoSetWmiDataBlock;
    InitializationData.FdoData.ClassWmiInfo.ClassSetWmiDataItem     = DiskFdoSetWmiDataItem;
    InitializationData.FdoData.ClassWmiInfo.ClassExecuteWmiMethod   = DiskFdoExecuteWmiMethod;
    InitializationData.FdoData.ClassWmiInfo.ClassWmiFunctionControl = DiskWmiFunctionControl;

    InitializationData.ClassAddDevice = DiskAddDevice;
    InitializationData.ClassUnload = DiskUnload;

    //
    // Initialize regregistration data structures
    //

    DiskInitializeReregistration();

    //
    // Call the class init routine
    //

    status = ClassInitialize(DriverObject, RegistryPath, &InitializationData);

    if (NT_SUCCESS(status)) {

        IoRegisterBootDriverReinitialization(DriverObject,
                                             DiskBootDriverReinit,
                                             NULL);
    }

    //
    // Call class init Ex routine to register a
    // PCLASS_QUERY_WMI_REGINFO_EX routine
    //

    classQueryWmiRegInfoExList.Size = sizeof(CLASS_QUERY_WMI_REGINFO_EX_LIST);
    classQueryWmiRegInfoExList.ClassFdoQueryWmiRegInfoEx = DiskFdoQueryWmiRegInfoEx;

    (VOID)ClassInitializeEx(DriverObject,
                            &guidQueryRegInfoEx,
                            &classQueryWmiRegInfoExList);

    //
    // Call class init Ex routine to register SRB support
    //
    srbSupport = CLASS_SRB_SCSI_REQUEST_BLOCK | CLASS_SRB_STORAGE_REQUEST_BLOCK;
    if (!NT_SUCCESS(ClassInitializeEx(DriverObject,
                                      &guidSrbSupport,
                                      &srbSupport))) {
        //
        // Should not fail
        //
        NT_ASSERT(FALSE);
    }


    return status;

} // end DriverEntry()


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();

#if defined(_X86_) || defined(_AMD64_)
    DiskCleanupDetectInfo(DriverObject);
#else
    // NB: Need to use UNREFERENCED_PARAMETER to prevent build error
    //     DriverObject is not referenced below in WPP_CLEANUP.
    //     WPP_CLEANUP is currently an "NOOP" marco.
    UNREFERENCED_PARAMETER(DriverObject);
#endif


    //
    // Cleans up tracing
    //
    WPP_CLEANUP(DriverObject);

    return;
}


NTSTATUS
DiskCreateFdo(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PULONG DeviceCount,
    IN BOOLEAN DasdAccessOnly
    )

/*++

Routine Description:

    This routine creates an object for the functional device

Arguments:

    DriverObject - Pointer to driver object created by system.

    PhysicalDeviceObject - Lower level driver we should attach to

    DeviceCount  - Number of previously installed devices.

    DasdAccessOnly - indicates whether or not a file system is allowed to mount
                     on this device object.  Used to avoid double-mounting of
                     file systems on super-floppies (which can unfortunately be
                     fixed disks).  If set the i/o system will only allow rawfs
                     to be mounted.

Return Value:

    NTSTATUS

--*/

{
    PCCHAR deviceName = NULL;
    HANDLE handle = NULL;
    PDEVICE_OBJECT lowerDevice  = NULL;
    PDEVICE_OBJECT deviceObject = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    NTSTATUS status;

    PAGED_CODE();

    *DeviceCount = 0;

    //
    // Set up an object directory to contain the objects for this
    // device and all its partitions.
    //

    do {

        WCHAR dirBuffer[64] = { 0 };
        UNICODE_STRING dirName;
        OBJECT_ATTRIBUTES objectAttribs;

        status = RtlStringCchPrintfW(dirBuffer, sizeof(dirBuffer) / sizeof(dirBuffer[0]) - 1, L"\\Device\\Harddisk%d", *DeviceCount);
        if (!NT_SUCCESS(status)) {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP, "DiskCreateFdo: Format symbolic link failed with error: 0x%X\n", status));
            return status;
        }

        RtlInitUnicodeString(&dirName, dirBuffer);

        InitializeObjectAttributes(&objectAttribs,
                                   &dirName,
                                   OBJ_CASE_INSENSITIVE | OBJ_PERMANENT | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        status = ZwCreateDirectoryObject(&handle,
                                         DIRECTORY_ALL_ACCESS,
                                         &objectAttribs);

        (*DeviceCount)++;

    } while((status == STATUS_OBJECT_NAME_COLLISION) ||
            (status == STATUS_OBJECT_NAME_EXISTS));

    if (!NT_SUCCESS(status)) {

        TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP, "DiskCreateFdo: Could not create directory - %lx\n", status));

        return(status);
    }

    //
    // When this loop exits the count is inflated by one - fix that.
    //

    (*DeviceCount)--;

    //
    // Claim the device.
    //

    lowerDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);

    status = ClassClaimDevice(lowerDevice, FALSE);

    if (!NT_SUCCESS(status)) {
        ZwMakeTemporaryObject(handle);
        ZwClose(handle);
        ObDereferenceObject(lowerDevice);
        return status;
    }

    //
    // Create a device object for this device. Each physical disk will
    // have at least one device object. The required device object
    // describes the entire device. Its directory path is
    // \Device\HarddiskN\Partition0, where N = device number.
    //

    status = DiskGenerateDeviceName(*DeviceCount, &deviceName);

    if(!NT_SUCCESS(status)) {
        TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP, "DiskCreateFdo - couldn't create name %lx\n", status));

        goto DiskCreateFdoExit;

    }

    status = ClassCreateDeviceObject(DriverObject,
                                     deviceName,
                                     PhysicalDeviceObject,
                                     TRUE,
                                     &deviceObject);

    if (!NT_SUCCESS(status)) {
        TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP, "DiskCreateFdo: Can not create device object %s\n", deviceName));
        goto DiskCreateFdoExit;
    }

    FREE_POOL(deviceName);

    //
    // Indicate that IRPs should include MDLs for data transfers.
    //

    SET_FLAG(deviceObject->Flags, DO_DIRECT_IO);

    fdoExtension = deviceObject->DeviceExtension;

    if(DasdAccessOnly) {

        //
        // Inidicate that only RAW should be allowed to mount on the root
        // partition object.  This ensures that a file system can't doubly
        // mount on a super-floppy by mounting once on P0 and once on P1.
        //

#ifdef _MSC_VER
#pragma prefast(suppress:28175);
#endif
        SET_FLAG(deviceObject->Vpb->Flags, VPB_RAW_MOUNT);
    }

    //
    // Initialize lock count to zero. The lock count is used to
    // disable the ejection mechanism on devices that support
    // removable media. Only the lock count in the physical
    // device extension is used.
    //

    fdoExtension->LockCount = 0;

    //
    // Save system disk number.
    //

    fdoExtension->DeviceNumber = *DeviceCount;

    //
    // Set the alignment requirements for the device based on the
    // host adapter requirements
    //

    if (lowerDevice->AlignmentRequirement > deviceObject->AlignmentRequirement) {
        deviceObject->AlignmentRequirement = lowerDevice->AlignmentRequirement;
    }

    //
    // Finally, attach to the pdo
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
        goto DiskCreateFdoExit;
    }

    //
    // Clear the init flag.
    //

    CLEAR_FLAG(deviceObject->Flags, DO_DEVICE_INITIALIZING);

    //
    // Store a handle to the device object directory for this disk
    //

    fdoExtension->DeviceDirectory = handle;

    ObDereferenceObject(lowerDevice);

    return STATUS_SUCCESS;

DiskCreateFdoExit:

    if (deviceObject != NULL)
    {
        IoDeleteDevice(deviceObject);
    }

    FREE_POOL(deviceName);

    ObDereferenceObject(lowerDevice);

    ZwMakeTemporaryObject(handle);
    ZwClose(handle);

    return status;
}


NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskReadWriteVerification(
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
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG residualBytes;
    ULONG residualOffset;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Make sure that the request is within the bounds of the partition,
    // the number of bytes to transfer and the byte offset are a
    // multiple of the sector size.
    //

    residualBytes = irpSp->Parameters.Read.Length & (commonExtension->PartitionZeroExtension->DiskGeometry.BytesPerSector - 1);
    residualOffset = irpSp->Parameters.Read.ByteOffset.LowPart & (commonExtension->PartitionZeroExtension->DiskGeometry.BytesPerSector - 1);

    if ((irpSp->Parameters.Read.ByteOffset.QuadPart > commonExtension->PartitionLength.QuadPart) ||
        (irpSp->Parameters.Read.ByteOffset.QuadPart < 0) ||
        (residualBytes != 0) ||
        (residualOffset != 0))
    {
        NT_ASSERT(residualOffset == 0);
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        ULONGLONG bytesRemaining = commonExtension->PartitionLength.QuadPart - irpSp->Parameters.Read.ByteOffset.QuadPart;

        if ((ULONGLONG)irpSp->Parameters.Read.Length > bytesRemaining)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (!NT_SUCCESS(status))
    {
        //
        // This error may be caused by the fact that the drive is not ready.
        //

        status = ((PDISK_DATA) commonExtension->DriverData)->ReadyStatus;

        if (!NT_SUCCESS(status)) {

            //
            // Flag this as a user error so that a popup is generated.
            //

            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "DiskReadWriteVerification: ReadyStatus is %lx\n", status));

            if (IoIsErrorUserInduced(status) && Irp->Tail.Overlay.Thread != NULL) {
                IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
            }

            //
            // status will keep the current error
            //

        } else if ((residualBytes == 0) && (residualOffset == 0)) {

            //
            // This failed because we think the physical disk is too small.
            // Send it down to the drive and let the hardware decide for
            // itself.
            //

            status = STATUS_SUCCESS;

        } else {

            //
            // Note fastfat depends on this parameter to determine when to
            // remount due to a sector size change.
            //

            status = STATUS_INVALID_PARAMETER;
        }
    }

    Irp->IoStatus.Status = status;

    return status;

} // end DiskReadWrite()


NTSTATUS
DiskDetermineMediaTypes(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP     Irp,
    IN UCHAR    MediumType,
    IN UCHAR    DensityCode,
    IN BOOLEAN  MediaPresent,
    IN BOOLEAN  IsWritable
    )

/*++

Routine Description:

    Determines number of types based on the physical device, validates the user buffer
    and builds the MEDIA_TYPE information.

Arguments:

    DeviceObject - Pointer to functional device object created by system.
    Irp - IOCTL_STORAGE_GET_MEDIA_TYPES_EX Irp.
    MediumType - byte returned in mode data header.
    DensityCode - byte returned in mode data block descriptor.
    NumberOfTypes - pointer to be updated based on actual device.

Return Value:

    Status is returned.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    PGET_MEDIA_TYPES  mediaTypes = Irp->AssociatedIrp.SystemBuffer;
    PDEVICE_MEDIA_INFO mediaInfo = &mediaTypes->MediaInfo[0];
    BOOLEAN deviceMatched = FALSE;

    PAGED_CODE();

    //
    // this should be checked prior to calling into this routine
    // as we use the buffer as mediaTypes
    //

    NT_ASSERT(irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
           sizeof(GET_MEDIA_TYPES));

    //
    // Determine if this device is removable or fixed.
    //

    if (!TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {

        //
        // Fixed disk.
        //

        mediaTypes->DeviceType = FILE_DEVICE_DISK;
        mediaTypes->MediaInfoCount = 1;

        mediaInfo->DeviceSpecific.DiskInfo.Cylinders.QuadPart   = fdoExtension->DiskGeometry.Cylinders.QuadPart;
        mediaInfo->DeviceSpecific.DiskInfo.MediaType            = FixedMedia;
        mediaInfo->DeviceSpecific.DiskInfo.TracksPerCylinder    = fdoExtension->DiskGeometry.TracksPerCylinder;
        mediaInfo->DeviceSpecific.DiskInfo.SectorsPerTrack      = fdoExtension->DiskGeometry.SectorsPerTrack;
        mediaInfo->DeviceSpecific.DiskInfo.BytesPerSector       = fdoExtension->DiskGeometry.BytesPerSector;
        mediaInfo->DeviceSpecific.DiskInfo.NumberMediaSides     = 1;
        mediaInfo->DeviceSpecific.DiskInfo.MediaCharacteristics = (MEDIA_CURRENTLY_MOUNTED | MEDIA_READ_WRITE);

        if (!IsWritable) {

            SET_FLAG(mediaInfo->DeviceSpecific.DiskInfo.MediaCharacteristics,
                     MEDIA_WRITE_PROTECTED);
        }

    } else {

        PCCHAR vendorId = (PCCHAR) fdoExtension->DeviceDescriptor + fdoExtension->DeviceDescriptor->VendorIdOffset;
        PCCHAR productId = (PCCHAR) fdoExtension->DeviceDescriptor + fdoExtension->DeviceDescriptor->ProductIdOffset;
        PCCHAR productRevision = (PCCHAR) fdoExtension->DeviceDescriptor + fdoExtension->DeviceDescriptor->ProductRevisionOffset;
        DISK_MEDIA_TYPES_LIST const *mediaListEntry;
        ULONG  currentMedia;
        ULONG  i;
        ULONG  j;
        ULONG  sizeNeeded;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                   "DiskDetermineMediaTypes: Vendor %s, Product %s\n",
                   vendorId,
                   productId));


        //
        // If there's an entry with such vendorId & ProductId in the DiskMediaTypesExclude list,
        // this device shouldn't be looked up in the DiskMediaTypes list to determine a medium type.
        // The exclude table allows to narrow down the set of devices described by the DiskMediaTypes
        // list (e.g.: DiskMediaTypes says "all HP devices" and DiskMediaTypesExlclude says
        // "except for HP RDX")
        //

        for (i = 0; DiskMediaTypesExclude[i].VendorId != NULL; i++) {
            mediaListEntry = &DiskMediaTypesExclude[i];

            if (strncmp(mediaListEntry->VendorId,vendorId,strlen(mediaListEntry->VendorId))) {
                continue;
            }

            if ((mediaListEntry->ProductId != NULL) &&
                 strncmp(mediaListEntry->ProductId, productId, strlen(mediaListEntry->ProductId))) {
                continue;
            }

            goto SkipTable;
        }

        //
        // Run through the list until we find the entry with a NULL Vendor Id.
        //

        for (i = 0; DiskMediaTypes[i].VendorId != NULL; i++) {

            mediaListEntry = &DiskMediaTypes[i];

            if (strncmp(mediaListEntry->VendorId,vendorId,strlen(mediaListEntry->VendorId))) {
                continue;
            }

            if ((mediaListEntry->ProductId != NULL) &&
                 strncmp(mediaListEntry->ProductId, productId, strlen(mediaListEntry->ProductId))) {
                continue;
            }

            if ((mediaListEntry->Revision != NULL) &&
                 strncmp(mediaListEntry->Revision, productRevision, strlen(mediaListEntry->Revision))) {
                continue;
            }

            deviceMatched = TRUE;

            mediaTypes->DeviceType = FILE_DEVICE_DISK;
            mediaTypes->MediaInfoCount = mediaListEntry->NumberOfTypes;

            //
            // Ensure that buffer is large enough.
            //

            sizeNeeded = FIELD_OFFSET(GET_MEDIA_TYPES, MediaInfo[0]) +
                         (mediaListEntry->NumberOfTypes *
                          sizeof(DEVICE_MEDIA_INFO)
                          );

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeNeeded) {

                //
                // Buffer too small
                //

                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                return STATUS_BUFFER_TOO_SMALL;
            }

            for (j = 0; j < mediaListEntry->NumberOfTypes; j++) {

                mediaInfo->DeviceSpecific.RemovableDiskInfo.Cylinders.QuadPart = fdoExtension->DiskGeometry.Cylinders.QuadPart;
                mediaInfo->DeviceSpecific.RemovableDiskInfo.TracksPerCylinder = fdoExtension->DiskGeometry.TracksPerCylinder;
                mediaInfo->DeviceSpecific.RemovableDiskInfo.SectorsPerTrack = fdoExtension->DiskGeometry.SectorsPerTrack;
                mediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector = fdoExtension->DiskGeometry.BytesPerSector;
                mediaInfo->DeviceSpecific.RemovableDiskInfo.NumberMediaSides = mediaListEntry->NumberOfSides;

                //
                // Set the type.
                //

                mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType = mediaListEntry->MediaTypes[j];

                if (mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType == MO_5_WO) {
                    mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics = MEDIA_WRITE_ONCE;
                } else {
                    mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics = MEDIA_READ_WRITE;
                }

                //
                // Status will either be success, if media is present, or no media.
                // It would be optimal to base from density code and medium type, but not all devices
                // have values for these fields.
                //

                if (MediaPresent) {

                    //
                    // The usage of MediumType and DensityCode is device specific, so this may need
                    // to be extended to further key off of product/vendor ids.
                    // Currently, the MO units are the only devices that return this information.
                    //

                    if (MediumType == 2) {
                        currentMedia = MO_5_WO;
                    } else if (MediumType == 3) {
                        currentMedia = MO_5_RW;

                        if (DensityCode == 0x87) {

                            //
                            // Indicate that the pinnacle 4.6 G media
                            // is present. Other density codes will default to normal
                            // RW MO media.
                            //

                            currentMedia = PINNACLE_APEX_5_RW;
                        }
                    } else {
                        currentMedia = 0;
                    }

                    if (currentMedia) {
                        if (mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType == (STORAGE_MEDIA_TYPE)currentMedia) {
                            SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics, MEDIA_CURRENTLY_MOUNTED);
                        }

                    } else {
                        SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics, MEDIA_CURRENTLY_MOUNTED);
                    }
                }

                if (!IsWritable) {
                    SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics, MEDIA_WRITE_PROTECTED);
                }

                //
                // Advance to next entry.
                //

                mediaInfo++;
            }
        }

SkipTable:

        if (!deviceMatched) {

            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                       "DiskDetermineMediaTypes: Unknown device. Vendor: %s Product: %s Revision: %s\n",
                                   vendorId,
                                   productId,
                                   productRevision));
            //
            // Build an entry for unknown.
            //

            mediaTypes->DeviceType = FILE_DEVICE_DISK;
            mediaTypes->MediaInfoCount = 1;

            mediaInfo->DeviceSpecific.RemovableDiskInfo.Cylinders.QuadPart   = fdoExtension->DiskGeometry.Cylinders.QuadPart;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType            = RemovableMedia;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.TracksPerCylinder    = fdoExtension->DiskGeometry.TracksPerCylinder;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.SectorsPerTrack      = fdoExtension->DiskGeometry.SectorsPerTrack;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector       = fdoExtension->DiskGeometry.BytesPerSector;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.NumberMediaSides     = 1;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics = MEDIA_READ_WRITE;

            if (MediaPresent) {

                SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics, MEDIA_CURRENTLY_MOUNTED);
            }

            if (!IsWritable) {

                SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics, MEDIA_WRITE_PROTECTED);
            }
        }
    }

    Irp->IoStatus.Information =
        FIELD_OFFSET(GET_MEDIA_TYPES, MediaInfo[0]) +
        (mediaTypes->MediaInfoCount * sizeof(DEVICE_MEDIA_INFO));

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    I/O system entry for device controls to SCSI disks.

Arguments:

    Fdo - Pointer to functional device object created by system.
    Irp - IRP involved.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               ioctlCode;

    NT_ASSERT(DeviceObject != NULL);

    Irp->IoStatus.Information = 0;
    ioctlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL, "DiskDeviceControl: Received IOCTL 0x%X for device %p through IRP %p\n",
                ioctlCode, DeviceObject, Irp));


    switch (ioctlCode) {

        case IOCTL_DISK_GET_CACHE_INFORMATION: {
            status = DiskIoctlGetCacheInformation(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_SET_CACHE_INFORMATION: {
            status = DiskIoctlSetCacheInformation(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_GET_CACHE_SETTING: {
            status = DiskIoctlGetCacheSetting(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_SET_CACHE_SETTING: {
            status = DiskIoctlSetCacheSetting(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_GET_DRIVE_GEOMETRY: {
            status = DiskIoctlGetDriveGeometry(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX: {
            status = DiskIoctlGetDriveGeometryEx( DeviceObject, Irp );
            break;
        }

        case IOCTL_DISK_VERIFY: {
            status = DiskIoctlVerify(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_GET_LENGTH_INFO: {
            status = DiskIoctlGetLengthInfo(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_IS_WRITABLE: {
            status = DiskIoctlIsWritable(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_UPDATE_DRIVE_SIZE: {
            status = DiskIoctlUpdateDriveSize(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_REASSIGN_BLOCKS: {
            status = DiskIoctlReassignBlocks(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_REASSIGN_BLOCKS_EX: {
            status = DiskIoctlReassignBlocksEx(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_INTERNAL_SET_VERIFY: {
            status = DiskIoctlSetVerify(DeviceObject, Irp);
            break;
        }

        case IOCTL_DISK_INTERNAL_CLEAR_VERIFY: {
            status = DiskIoctlClearVerify(DeviceObject, Irp);
            break;
        }

        case IOCTL_STORAGE_GET_MEDIA_TYPES_EX: {
            status = DiskIoctlGetMediaTypesEx(DeviceObject, Irp);
            break;
        }

        case IOCTL_STORAGE_PREDICT_FAILURE : {
            status = DiskIoctlPredictFailure(DeviceObject, Irp);
            break;
        }

        #if (NTDDI_VERSION >= NTDDI_WINBLUE)
        case IOCTL_STORAGE_FAILURE_PREDICTION_CONFIG : {
            status = DiskIoctlEnableFailurePrediction(DeviceObject, Irp);
            break;
        }
        #endif

        case SMART_GET_VERSION: {
            status = DiskIoctlSmartGetVersion(DeviceObject, Irp);
            break;
        }

        case SMART_RCV_DRIVE_DATA: {
            status = DiskIoctlSmartReceiveDriveData(DeviceObject, Irp);
            break;
        }

        case SMART_SEND_DRIVE_COMMAND: {
            status = DiskIoctlSmartSendDriveCommand(DeviceObject, Irp);
            break;
        }

        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN: {
            status = DiskIoctlGetVolumeDiskExtents(DeviceObject, Irp);
            break;
        }

        default: {

            //
            // Pass the request to the common device control routine.
            //
            return(ClassDeviceControl(DeviceObject, Irp));
            break;
        }
    } // end switch

    if (!NT_SUCCESS(status)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskDeviceControl: IOCTL 0x%X to device %p failed with error 0x%X\n",
                    ioctlCode, DeviceObject, status));
        if (IoIsErrorUserInduced(status) &&
            (Irp->Tail.Overlay.Thread != NULL)) {
            IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        }
    }

    //
    // DiskIoctlVerify() (IOCTL_DISK_VERIFY) function returns STATUS_PENDING
    // and completes the IRP in the work item. Do not touch or complete
    // the IRP if STATUS_PENDING is returned.
    //

    if (status != STATUS_PENDING) {


        Irp->IoStatus.Status = status;
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
    }

    return(status);
} // end DiskDeviceControl()

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the handler for shutdown and flush requests. It sends
    down a synch cache command to the device if its cache is enabled.  If
    the request is a  shutdown and the media is removable,  it sends down
    an unlock request

    Finally,  an SRB_FUNCTION_SHUTDOWN or SRB_FUNCTION_FLUSH is sent down
    the stack

Arguments:

    DeviceObject - The device object processing the request
    Irp - The shutdown | flush request being serviced

Return Value:

    STATUS_PENDING if successful, an error code otherwise

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = commonExtension->PartitionZeroExtension;
    PDISK_DATA diskData = (PDISK_DATA) commonExtension->DriverData;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG srbSize;
    PSCSI_REQUEST_BLOCK srb;
    PSTORAGE_REQUEST_BLOCK srbEx = NULL;
    PSTOR_ADDR_BTL8 storAddrBtl8 = NULL;
    PSRBEX_DATA_SCSI_CDB16 srbExDataCdb16 = NULL;
    PCDB cdb;
    KIRQL irql;

    //
    // Flush requests are combined and need to be handled in a special manner
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (irpStack->MajorFunction == IRP_MJ_FLUSH_BUFFERS) {

        if (TEST_FLAG(fdoExtension->DeviceFlags, DEV_POWER_PROTECTED)) {

            //
            // We've been assured that both the disk
            // and adapter caches are battery-backed
            //

            Irp->IoStatus.Status = STATUS_SUCCESS;
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        KeAcquireSpinLock(&diskData->FlushContext.Spinlock, &irql);

        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_SCSI, "DiskShutdownFlush: IRP %p flags = 0x%x\n", Irp, irpStack->Flags));

        //
        // This request will most likely be completed asynchronously
        //
        IoMarkIrpPending(Irp);

        //
        // Look to see if a flush is in progress
        //

        if (diskData->FlushContext.CurrIrp != NULL) {

            //
            // There is an outstanding flush. Queue this
            // request to the group that is next in line
            //

            if (diskData->FlushContext.NextIrp != NULL) {

                #if DBG
                    diskData->FlushContext.DbgTagCount++;
                #endif

                InsertTailList(&diskData->FlushContext.NextList, &Irp->Tail.Overlay.ListEntry);

                KeReleaseSpinLock(&diskData->FlushContext.Spinlock, irql);

                //
                // This request will be completed by its representative
                //

            } else {

                #if DBG
                    if (diskData->FlushContext.DbgTagCount < 64) {

                        diskData->FlushContext.DbgRefCount[diskData->FlushContext.DbgTagCount]++;
                    }

                    diskData->FlushContext.DbgSavCount += diskData->FlushContext.DbgTagCount;
                    diskData->FlushContext.DbgTagCount  = 0;
                #endif

                diskData->FlushContext.NextIrp = Irp;
                NT_ASSERT(IsListEmpty(&diskData->FlushContext.NextList));


                KeReleaseSpinLock(&diskData->FlushContext.Spinlock, irql);


                    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_SCSI, "DiskShutdownFlush: waiting for event\n"));

                    //
                    // Wait for the outstanding flush to complete
                    //
                    KeWaitForSingleObject(&diskData->FlushContext.Event, Executive, KernelMode, FALSE, NULL);

                    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_SCSI, "DiskShutdownFlush: event signal\n"));

                    //
                    // Make this group the outstanding one and free up the next slot
                    //

                    KeAcquireSpinLock(&diskData->FlushContext.Spinlock, &irql);

                    NT_ASSERT(IsListEmpty(&diskData->FlushContext.CurrList));

                    while (!IsListEmpty(&diskData->FlushContext.NextList)) {

                        PLIST_ENTRY listEntry = RemoveHeadList(&diskData->FlushContext.NextList);
                        InsertTailList(&diskData->FlushContext.CurrList, listEntry);
                    }

#ifndef __REACTOS__
                    // ReactOS hits this assert, because CurrIrp can already be freed at this point
                    // and it's possible that NextIrp has the same pointer value
                    NT_ASSERT(diskData->FlushContext.CurrIrp != diskData->FlushContext.NextIrp);
#endif
                    diskData->FlushContext.CurrIrp = diskData->FlushContext.NextIrp;
                    diskData->FlushContext.NextIrp = NULL;

                    KeReleaseSpinLock(&diskData->FlushContext.Spinlock, irql);

                    //
                    // Send this request down to the device
                    //
                    DiskFlushDispatch(DeviceObject, &diskData->FlushContext);
            }

        } else {

            diskData->FlushContext.CurrIrp = Irp;
            NT_ASSERT(IsListEmpty(&diskData->FlushContext.CurrList));

            NT_ASSERT(diskData->FlushContext.NextIrp == NULL);
            NT_ASSERT(IsListEmpty(&diskData->FlushContext.NextList));


            KeReleaseSpinLock(&diskData->FlushContext.Spinlock, irql);

                DiskFlushDispatch(DeviceObject, &diskData->FlushContext);
        }

    } else {

        //
        // Allocate SCSI request block.
        //

        if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
            srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
        } else {
            srbSize = sizeof(SCSI_REQUEST_BLOCK);
        }

        srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                    srbSize,
                                    DISK_TAG_SRB);
        if (srb == NULL) {

            //
            // Set the status and complete the request.
            //

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(srb, srbSize);
        if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {

            srbEx = (PSTORAGE_REQUEST_BLOCK)srb;

            //
            // Set up STORAGE_REQUEST_BLOCK fields
            //

            srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
            srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
            srbEx->Signature = SRB_SIGNATURE;
            srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
            srbEx->SrbLength = srbSize;
            srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
            srbEx->RequestPriority = IoGetIoPriorityHint(Irp);
            srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);
            srbEx->NumSrbExData = 1;

            // Set timeout value and mark the request as not being a tagged request.
            srbEx->TimeOutValue = fdoExtension->TimeOutValue * 4;
            srbEx->RequestTag = SP_UNTAGGED;
            srbEx->RequestAttribute = SRB_SIMPLE_TAG_REQUEST;
            srbEx->SrbFlags = fdoExtension->SrbFlags;

            //
            // Set up address fields
            //

            storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
            storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
            storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

            //
            // Set up SCSI SRB extended data fields
            //

            srbEx->SrbExDataOffset[0] = sizeof(STORAGE_REQUEST_BLOCK) +
                sizeof(STOR_ADDR_BTL8);
            if ((srbEx->SrbExDataOffset[0] + sizeof(SRBEX_DATA_SCSI_CDB16)) <= srbEx->SrbLength) {
                srbExDataCdb16 = (PSRBEX_DATA_SCSI_CDB16)((PUCHAR)srbEx + srbEx->SrbExDataOffset[0]);
                srbExDataCdb16->Type = SrbExDataTypeScsiCdb16;
                srbExDataCdb16->Length = SRBEX_DATA_SCSI_CDB16_LENGTH;

                cdb = (PCDB)srbExDataCdb16->Cdb;
            } else {
                // Should not happen
                NT_ASSERT(FALSE);

                //
                // Set the status and complete the request.
                //

                Irp->IoStatus.Status = STATUS_INTERNAL_ERROR;
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                return(STATUS_INTERNAL_ERROR);
            }

        } else {

            //
            // Write length to SRB.
            //

            srb->Length = SCSI_REQUEST_BLOCK_SIZE;

            //
            // Set timeout value and mark the request as not being a tagged request.
            //

            srb->TimeOutValue = fdoExtension->TimeOutValue * 4;
            srb->QueueTag = SP_UNTAGGED;
            srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
            srb->SrbFlags = fdoExtension->SrbFlags;
            srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

            cdb = (PCDB)srb->Cdb;
        }

        //
        // If the write cache is enabled then send a synchronize cache request.
        //

        if (TEST_FLAG(fdoExtension->DeviceFlags, DEV_WRITE_CACHE)) {

            if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
                srbExDataCdb16->CdbLength = 10;
            } else {
                srb->CdbLength = 10;
            }

            cdb->CDB10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;

            status = ClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             NULL,
                                             0,
                                             TRUE);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "DiskShutdownFlush: Synchonize cache sent. Status = %lx\n", status));
        }

        //
        // Unlock the device if it contains removable media
        //

        if (TEST_FLAG(DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA))
        {

            if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {

                //
                // Reinitialize status fields to 0 in case there was a previous request
                //

                srbEx->SrbStatus = 0;
                srbExDataCdb16->ScsiStatus = 0;

                srbExDataCdb16->CdbLength = 6;

                //
                // Set timeout value
                //

                srbEx->TimeOutValue = fdoExtension->TimeOutValue;

            } else {

                //
                // Reinitialize status fields to 0 in case there was a previous request
                //

                srb->SrbStatus = 0;
                srb->ScsiStatus = 0;

                srb->CdbLength = 6;

                //
                // Set timeout value.
                //

                srb->TimeOutValue = fdoExtension->TimeOutValue;
            }

            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            cdb->MEDIA_REMOVAL.Prevent = FALSE;

            status = ClassSendSrbSynchronous(DeviceObject,
                                             srb,
                                             NULL,
                                             0,
                                             TRUE);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "DiskShutdownFlush: Unlock device request sent. Status = %lx\n", status));
        }

        //
        // Set up a SHUTDOWN SRB
        //

        if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
            srbEx->NumSrbExData = 0;
            srbEx->SrbExDataOffset[0] = 0;
            srbEx->SrbFunction = SRB_FUNCTION_SHUTDOWN;
            srbEx->OriginalRequest = Irp;
            srbEx->SrbLength = CLASS_SRBEX_NO_SRBEX_DATA_BUFFER_SIZE;
            srbEx->SrbStatus = 0;
        } else {
            srb->CdbLength = 0;
            srb->Function = SRB_FUNCTION_SHUTDOWN;
            srb->SrbStatus = 0;
            srb->OriginalRequest = Irp;
        }

        //
        // Set the retry count to zero.
        //

        irpStack->Parameters.Others.Argument4 = (PVOID) 0;

        //
        // Set up IoCompletion routine address.
        //

        IoSetCompletionRoutine(Irp, ClassIoComplete, srb, TRUE, TRUE, TRUE);

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
        // Call the port driver to process the request.
        //

        IoMarkIrpPending(Irp);
        IoCallDriver(commonExtension->LowerDeviceObject, Irp);
    }

    return STATUS_PENDING;
}


VOID
DiskFlushDispatch(
    IN PDEVICE_OBJECT Fdo,
    IN PDISK_GROUP_CONTEXT FlushContext
    )

/*++

Routine Description:

    This routine is the handler for flush requests. It sends down a synch
    cache command to the device if its cache is enabled. This is followed
    by an SRB_FUNCTION_FLUSH

Arguments:

    Fdo - The device object processing the flush request
    FlushContext - The flush group context

Return Value:

    None

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PSCSI_REQUEST_BLOCK srb = &FlushContext->Srb.Srb;
    PSTORAGE_REQUEST_BLOCK srbEx = &FlushContext->Srb.SrbEx;
    PIO_STACK_LOCATION  irpSp = NULL;
    PSTOR_ADDR_BTL8 storAddrBtl8;
    PSRBEX_DATA_SCSI_CDB16 srbExDataCdb16;
    NTSTATUS SyncCacheStatus = STATUS_SUCCESS;

    //
    // Fill in the srb fields appropriately
    //
    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        RtlZeroMemory(srbEx, sizeof(FlushContext->Srb.SrbExBuffer));

        srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
        srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
        srbEx->Signature = SRB_SIGNATURE;
        srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
        srbEx->SrbLength = sizeof(FlushContext->Srb.SrbExBuffer);
        srbEx->RequestPriority = IoGetIoPriorityHint(FlushContext->CurrIrp);
        srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);
        srbEx->TimeOutValue = fdoExt->TimeOutValue * 4;
        srbEx->RequestTag = SP_UNTAGGED;
        srbEx->RequestAttribute  = SRB_SIMPLE_TAG_REQUEST;
        srbEx->SrbFlags = fdoExt->SrbFlags;

        //
        // Set up address fields
        //

        storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
        storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
        storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

    } else {
        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

        srb->Length       = SCSI_REQUEST_BLOCK_SIZE;
        srb->TimeOutValue = fdoExt->TimeOutValue * 4;
        srb->QueueTag     = SP_UNTAGGED;
        srb->QueueAction  = SRB_SIMPLE_TAG_REQUEST;
        srb->SrbFlags     = fdoExt->SrbFlags;
    }

    //
    // If write caching is enabled then send down a synchronize cache request
    //
    if (TEST_FLAG(fdoExt->DeviceFlags, DEV_WRITE_CACHE))
    {

        if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
            srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
            srbEx->NumSrbExData = 1;

            //
            // Set up SCSI SRB extended data fields
            //

            srbEx->SrbExDataOffset[0] = sizeof(STORAGE_REQUEST_BLOCK) +
                sizeof(STOR_ADDR_BTL8);
            if ((srbEx->SrbExDataOffset[0] + sizeof(SRBEX_DATA_SCSI_CDB16)) <= srbEx->SrbLength) {
                srbExDataCdb16 = (PSRBEX_DATA_SCSI_CDB16)((PUCHAR)srbEx + srbEx->SrbExDataOffset[0]);
                srbExDataCdb16->Type = SrbExDataTypeScsiCdb16;
                srbExDataCdb16->Length = SRBEX_DATA_SCSI_CDB16_LENGTH;
                srbExDataCdb16->CdbLength = 10;
                srbExDataCdb16->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;
            } else {
                // Should not happen
                NT_ASSERT(FALSE);
                return;
            }

        } else {
            srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            srb->CdbLength = 10;
            srb->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;
        }

        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_SCSI, "DiskFlushDispatch: sending sync cache\n"));

        SyncCacheStatus = ClassSendSrbSynchronous(Fdo, srb, NULL, 0, TRUE);
    }

    //
    // Set up a FLUSH SRB
    //
    if (fdoExt->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx->SrbFunction = SRB_FUNCTION_FLUSH;
        srbEx->NumSrbExData = 0;
        srbEx->SrbExDataOffset[0] = 0;
        srbEx->OriginalRequest = FlushContext->CurrIrp;
        srbEx->SrbStatus = 0;

        //
        // Make sure that this srb does not get freed
        //
        SET_FLAG(srbEx->SrbFlags, SRB_CLASS_FLAGS_PERSISTANT);

   } else {
        srb->Function  = SRB_FUNCTION_FLUSH;
        srb->CdbLength = 0;
        srb->OriginalRequest = FlushContext->CurrIrp;
        srb->SrbStatus = 0;
        srb->ScsiStatus = 0;

        //
        // Make sure that this srb does not get freed
        //
        SET_FLAG(srb->SrbFlags, SRB_CLASS_FLAGS_PERSISTANT);
    }

    //
    // Make sure that this request does not get retried
    //
    irpSp = IoGetCurrentIrpStackLocation(FlushContext->CurrIrp);

    irpSp->Parameters.Others.Argument4 = (PVOID) 0;

    //
    // Fill in the irp fields appropriately
    //
    irpSp = IoGetNextIrpStackLocation(FlushContext->CurrIrp);

    irpSp->MajorFunction       = IRP_MJ_SCSI;
    irpSp->Parameters.Scsi.Srb = srb;

    IoSetCompletionRoutine(FlushContext->CurrIrp, DiskFlushComplete, (PVOID)(ULONG_PTR)SyncCacheStatus, TRUE, TRUE, TRUE);

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_SCSI, "DiskFlushDispatch: sending srb flush on irp %p\n", FlushContext->CurrIrp));

    //
    // Send down the flush request
    //
    IoCallDriver(((PCOMMON_DEVICE_EXTENSION)fdoExt)->LowerDeviceObject, FlushContext->CurrIrp);
}



NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskFlushComplete(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This completion routine is a wrapper around ClassIoComplete. It
    will complete all the flush requests that are tagged to it, set
    an event to signal the next group to proceed and return

Arguments:

    Fdo - The device object which requested the completion routine
    Irp - The irp that is being completed
    Context - If disk had write cache enabled and SYNC CACHE command was sent as 1st part of FLUSH processing
                   then context must carry the completion status of SYNC CACHE request,
              else context must be set to STATUS_SUCCESS.

Return Value:

    STATUS_SUCCESS if successful, an error code otherwise

--*/

{
    PDISK_GROUP_CONTEXT FlushContext;
    NTSTATUS status;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PDISK_DATA diskData;
#ifdef _MSC_VER
    #pragma warning(suppress:4311) // pointer truncation from 'PVOID' to 'NTSTATUS'
#endif
    NTSTATUS SyncCacheStatus = (NTSTATUS)(ULONG_PTR)Context;

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL, "DiskFlushComplete: %p %p\n", Fdo, Irp));

    //
    // Get the flush context from the device extension
    //
    fdoExt = (PFUNCTIONAL_DEVICE_EXTENSION)Fdo->DeviceExtension;
    diskData = (PDISK_DATA)fdoExt->CommonExtension.DriverData;
    NT_ASSERT(diskData != NULL);
    _Analysis_assume_(diskData != NULL);

    FlushContext = &diskData->FlushContext;

    //
    // Make sure everything is in order
    //
    NT_ASSERT(Irp == FlushContext->CurrIrp);

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_SCSI, "DiskFlushComplete: completing irp %p\n", Irp));
    status = ClassIoComplete(Fdo, Irp, &FlushContext->Srb.Srb);

    //
    // Make sure that ClassIoComplete did not decide to retry this request
    //
    NT_ASSERT(status != STATUS_MORE_PROCESSING_REQUIRED);

    //
    // If sync cache failed earlier, final status of the flush request needs to be failure
    // even if SRB_FUNCTION_FLUSH srb request succeeded
    //
    if (NT_SUCCESS(status) &&
        (!NT_SUCCESS(SyncCacheStatus))) {
        Irp->IoStatus.Status = status = SyncCacheStatus;
    }

    //
    // Complete the flush requests tagged to this one
    //

    while (!IsListEmpty(&FlushContext->CurrList)) {

        PLIST_ENTRY listEntry = RemoveHeadList(&FlushContext->CurrList);
        PIRP tempIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        InitializeListHead(&tempIrp->Tail.Overlay.ListEntry);
        tempIrp->IoStatus = Irp->IoStatus;

        ClassReleaseRemoveLock(Fdo, tempIrp);
        ClassCompleteRequest(Fdo, tempIrp, IO_NO_INCREMENT);
    }


        //
        // Notify the next group's representative that it may go ahead now
        //
        KeSetEvent(&FlushContext->Event, IO_NO_INCREMENT, FALSE);


    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_SCSI, "DiskFlushComplete: irp %p status = 0x%x\n", Irp, status));

    return status;
}



NTSTATUS
DiskModeSelect(
    IN PDEVICE_OBJECT Fdo,
    _In_reads_bytes_(Length) PCHAR ModeSelectBuffer,
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
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCDB cdb;
    SCSI_REQUEST_BLOCK srb = {0};
    ULONG retries = 1;
    ULONG length2;
    NTSTATUS status;
    PULONG buffer;
    PMODE_PARAMETER_BLOCK blockDescriptor;
    UCHAR srbExBuffer[CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE] = {0};
    PSTORAGE_REQUEST_BLOCK srbEx = (PSTORAGE_REQUEST_BLOCK)srbExBuffer;
    PSTOR_ADDR_BTL8 storAddrBtl8;
    PSRBEX_DATA_SCSI_CDB16 srbExDataCdb16;
    PSCSI_REQUEST_BLOCK srbPtr;

    PAGED_CODE();

    //
    // Check whether block length is available
    //

    if (fdoExtension->DiskGeometry.BytesPerSector == 0) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskModeSelect: Block length is not available. Unable to send mode select\n"));
        NT_ASSERT(fdoExtension->DiskGeometry.BytesPerSector != 0);
        return STATUS_INVALID_PARAMETER;
    }



    length2 = Length + sizeof(MODE_PARAMETER_HEADER) + sizeof(MODE_PARAMETER_BLOCK);

    //
    // Allocate buffer for mode select header, block descriptor, and mode page.
    //

    buffer = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                   length2,
                                   DISK_TAG_MODE_DATA);

    if (buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(buffer, length2);

    //
    // Set length in header to size of mode page.
    //

    ((PMODE_PARAMETER_HEADER)buffer)->BlockDescriptorLength = sizeof(MODE_PARAMETER_BLOCK);

    blockDescriptor = (PMODE_PARAMETER_BLOCK)(buffer + 1);

    //
    // Set block length from the cached disk geometry
    //

    blockDescriptor->BlockLength[2] = (UCHAR) (fdoExtension->DiskGeometry.BytesPerSector >> 16);
    blockDescriptor->BlockLength[1] = (UCHAR) (fdoExtension->DiskGeometry.BytesPerSector >> 8);
    blockDescriptor->BlockLength[0] = (UCHAR) (fdoExtension->DiskGeometry.BytesPerSector);

    //
    // Copy mode page to buffer.
    //

    RtlCopyMemory(buffer + 3, ModeSelectBuffer, Length);

    //
    // Build the MODE SELECT CDB.
    //

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {

        //
        // Set up STORAGE_REQUEST_BLOCK fields
        //

        srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
        srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
        srbEx->Signature = SRB_SIGNATURE;
        srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
        srbEx->SrbLength = sizeof(srbExBuffer);
        srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
        srbEx->RequestPriority = IoPriorityNormal;
        srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);
        srbEx->NumSrbExData = 1;

        // Set timeout value from device extension.
        srbEx->TimeOutValue = fdoExtension->TimeOutValue * 2;

       //
       // Set up address fields
       //

       storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
       storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
       storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

       //
       // Set up SCSI SRB extended data fields
       //

       srbEx->SrbExDataOffset[0] = sizeof(STORAGE_REQUEST_BLOCK) +
           sizeof(STOR_ADDR_BTL8);
       if ((srbEx->SrbExDataOffset[0] + sizeof(SRBEX_DATA_SCSI_CDB16)) <= srbEx->SrbLength) {
           srbExDataCdb16 = (PSRBEX_DATA_SCSI_CDB16)((PUCHAR)srbEx + srbEx->SrbExDataOffset[0]);
           srbExDataCdb16->Type = SrbExDataTypeScsiCdb16;
           srbExDataCdb16->Length = SRBEX_DATA_SCSI_CDB16_LENGTH;
           srbExDataCdb16->CdbLength = 6;

           cdb = (PCDB)srbExDataCdb16->Cdb;
       } else {
           // Should not happen
           NT_ASSERT(FALSE);

           FREE_POOL(buffer);
           TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskModeSelect: Insufficient extended SRB size\n"));
           return STATUS_INTERNAL_ERROR;
       }

       srbPtr = (PSCSI_REQUEST_BLOCK)srbEx;

    } else {

        srb.CdbLength = 6;
        cdb = (PCDB)srb.Cdb;

        //
        // Set timeout value from device extension.
        //

        srb.TimeOutValue = fdoExtension->TimeOutValue * 2;

        srbPtr = &srb;
    }

    cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
    cdb->MODE_SELECT.SPBit = SavePage;
    cdb->MODE_SELECT.PFBit = 1;
    cdb->MODE_SELECT.ParameterListLength = (UCHAR)(length2);

Retry:

    status = ClassSendSrbSynchronous(Fdo,
                                     srbPtr,
                                     buffer,
                                     length2,
                                     TRUE);

    if (status == STATUS_VERIFY_REQUIRED) {

        //
        // Routine ClassSendSrbSynchronous does not retry requests returned with
        // this status.
        //

        if (retries--) {

            //
            // Retry request.
            //

            goto Retry;
        }

    } else if (SRB_STATUS(srbPtr->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {
        status = STATUS_SUCCESS;
    }

    FREE_POOL(buffer);

    return status;
} // end DiskModeSelect()


//
// This routine is structured as a work-item routine
//
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DisableWriteCache(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID Context
    )

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)Fdo->DeviceExtension;
    DISK_CACHE_INFORMATION cacheInfo = { 0 };
    NTSTATUS status;
    PIO_WORKITEM WorkItem = (PIO_WORKITEM)Context;

    PAGED_CODE();

    NT_ASSERT(WorkItem != NULL);
    _Analysis_assume_(WorkItem != NULL);

    status = DiskGetCacheInformation(fdoExtension, &cacheInfo);

    if (NT_SUCCESS(status) && (cacheInfo.WriteCacheEnabled == TRUE)) {

        cacheInfo.WriteCacheEnabled = FALSE;

        DiskSetCacheInformation(fdoExtension, &cacheInfo);
    }

    IoFreeWorkItem(WorkItem);
}


//
// This routine is structured as a work-item routine
//
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskIoctlVerifyThread(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID Context
    )
{
    PDISK_VERIFY_WORKITEM_CONTEXT WorkContext = (PDISK_VERIFY_WORKITEM_CONTEXT)Context;
    PIRP Irp = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)Fdo->DeviceExtension;
    PDISK_DATA DiskData = (PDISK_DATA)FdoExtension->CommonExtension.DriverData;
    PVERIFY_INFORMATION verifyInfo = NULL;
    PSCSI_REQUEST_BLOCK Srb = NULL;
    PCDB Cdb = NULL;
    LARGE_INTEGER byteOffset;
    LARGE_INTEGER sectorOffset;
    ULONG sectorCount;
    NTSTATUS status = STATUS_SUCCESS;
    PSTORAGE_REQUEST_BLOCK srbEx = NULL;
    PSTOR_ADDR_BTL8 storAddrBtl8 = NULL;
    PSRBEX_DATA_SCSI_CDB16 srbExDataCdb16 = NULL;

    PAGED_CODE();

    NT_ASSERT(WorkContext != NULL);
    _Analysis_assume_(WorkContext != NULL);

    Srb = WorkContext->Srb;
    Irp = WorkContext->Irp;
    verifyInfo = (PVERIFY_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    //
    // We don't need to hold on to this memory as
    // the following operation may take some time
    //

    IoFreeWorkItem(WorkContext->WorkItem);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlVerifyThread: Spliting up the request\n"));

    //
    // Add disk offset to starting the sector
    //

    byteOffset.QuadPart = FdoExtension->CommonExtension.StartingOffset.QuadPart +
                          verifyInfo->StartingOffset.QuadPart;

    //
    // Convert byte offset to the sector offset
    //

    sectorOffset.QuadPart = byteOffset.QuadPart >> FdoExtension->SectorShift;

    //
    // Convert byte count to sector count.
    //

    sectorCount = verifyInfo->Length >> FdoExtension->SectorShift;

    //
    // Make sure  that all previous verify requests have indeed completed
    // This greatly reduces the possibility of a Denial-of-Service attack
    //

    KeWaitForMutexObject(&DiskData->VerifyMutex,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL);

    //
    // Initialize SCSI SRB for a verify CDB
    //
    if (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        RtlZeroMemory(Srb, CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE);
        srbEx = (PSTORAGE_REQUEST_BLOCK)Srb;

        //
        // Set up STORAGE_REQUEST_BLOCK fields
        //

        srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
        srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
        srbEx->Signature = SRB_SIGNATURE;
        srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
        srbEx->SrbLength = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
        srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
        srbEx->RequestPriority = IoGetIoPriorityHint(Irp);
        srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);
        srbEx->NumSrbExData = 1;

        //
        // Set up address fields
        //

        storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
        storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
        storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

        //
        // Set up SCSI SRB extended data fields
        //

        srbEx->SrbExDataOffset[0] = sizeof(STORAGE_REQUEST_BLOCK) +
               sizeof(STOR_ADDR_BTL8);
        if ((srbEx->SrbExDataOffset[0] + sizeof(SRBEX_DATA_SCSI_CDB16)) <= srbEx->SrbLength) {
            srbExDataCdb16 = (PSRBEX_DATA_SCSI_CDB16)((PUCHAR)srbEx + srbEx->SrbExDataOffset[0]);
            srbExDataCdb16->Type = SrbExDataTypeScsiCdb16;
            srbExDataCdb16->Length = SRBEX_DATA_SCSI_CDB16_LENGTH;

            Cdb = (PCDB)srbExDataCdb16->Cdb;
            if (TEST_FLAG(FdoExtension->DeviceFlags, DEV_USE_16BYTE_CDB)) {
                srbExDataCdb16->CdbLength = 16;
                Cdb->CDB16.OperationCode = SCSIOP_VERIFY16;
            } else {
                srbExDataCdb16->CdbLength = 10;
                Cdb->CDB10.OperationCode = SCSIOP_VERIFY;
            }
        } else {
            // Should not happen
            NT_ASSERT(FALSE);

            FREE_POOL(Srb);
            FREE_POOL(WorkContext);
            status = STATUS_INTERNAL_ERROR;
        }

    } else {
        RtlZeroMemory(Srb, SCSI_REQUEST_BLOCK_SIZE);

        Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

        Cdb = (PCDB)Srb->Cdb;
        if (TEST_FLAG(FdoExtension->DeviceFlags, DEV_USE_16BYTE_CDB)) {
            Srb->CdbLength = 16;
            Cdb->CDB16.OperationCode = SCSIOP_VERIFY16;
        } else {
            Srb->CdbLength = 10;
            Cdb->CDB10.OperationCode = SCSIOP_VERIFY;
        }

    }

    while (NT_SUCCESS(status) && (sectorCount != 0)) {

        USHORT numSectors = (USHORT) min(sectorCount, MAX_SECTORS_PER_VERIFY);

        if (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {

            //
            // Reset status fields
            //

            srbEx->SrbStatus = 0;
            srbExDataCdb16->ScsiStatus = 0;

            //
            // Calculate the request timeout value based
            // on  the number of sectors  being verified
            //

            srbEx->TimeOutValue = ((numSectors + 0x7F) >> 7) * FdoExtension->TimeOutValue;
        } else {

            //
            // Reset status fields
            //

            Srb->SrbStatus = 0;
            Srb->ScsiStatus = 0;

            //
            // Calculate the request timeout value based
            // on  the number of sectors  being verified
            //

            Srb->TimeOutValue = ((numSectors + 0x7F) >> 7) * FdoExtension->TimeOutValue;
        }

        //
        // Update verify CDB info.
        // NOTE - CDB opcode and length has been initialized prior to entering
        // the while loop
        //

        if (TEST_FLAG(FdoExtension->DeviceFlags, DEV_USE_16BYTE_CDB)) {

            REVERSE_BYTES_QUAD(&Cdb->CDB16.LogicalBlock, &sectorOffset);
            REVERSE_BYTES_SHORT(&Cdb->CDB16.TransferLength[2], &numSectors);
        } else {

            //
            // Move little endian values into CDB in big endian format
            //

            Cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&sectorOffset)->Byte3;
            Cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&sectorOffset)->Byte2;
            Cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&sectorOffset)->Byte1;
            Cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&sectorOffset)->Byte0;

            Cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&numSectors)->Byte1;
            Cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&numSectors)->Byte0;
        }

        status = ClassSendSrbSynchronous(Fdo,
                                         Srb,
                                         NULL,
                                         0,
                                         FALSE);

        NT_ASSERT(status != STATUS_NONEXISTENT_SECTOR);

        sectorCount  -= numSectors;
        sectorOffset.QuadPart += numSectors;
    }

    KeReleaseMutex(&DiskData->VerifyMutex, FALSE);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    ClassReleaseRemoveLock(Fdo, Irp);
    ClassCompleteRequest(Fdo, Irp, IO_NO_INCREMENT);

    FREE_POOL(Srb);
    FREE_POOL(WorkContext);
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskFdoProcessError(
    PDEVICE_OBJECT Fdo,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )

/*++

Routine Description:

   This routine checks the type of error.  If the error indicates an underrun
   then indicate the request should be retried.

Arguments:

    Fdo - Supplies a pointer to the functional device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - Status with which the IRP will be completed.

    Retry - Indication of whether the request will be retried.

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PSTORAGE_REQUEST_BLOCK srbEx;
    PCDB cdb = NULL;
    UCHAR scsiStatus = 0;
    UCHAR senseBufferLength = 0;
    PVOID senseBuffer = NULL;
    CDB noOp = {0};

    //
    // Get relevant fields from SRB
    //
    if (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {

        srbEx = (PSTORAGE_REQUEST_BLOCK)Srb;

        //
        // Look for SCSI SRB specific fields
        //
        if ((srbEx->SrbFunction == SRB_FUNCTION_EXECUTE_SCSI) &&
            (srbEx->NumSrbExData > 0)) {
            cdb = GetSrbScsiData(srbEx, NULL, NULL, &scsiStatus, &senseBuffer, &senseBufferLength);

            //
            // cdb and sense buffer should not be NULL
            //
            NT_ASSERT(cdb != NULL);
            NT_ASSERT(senseBuffer != NULL);

        }

        if (cdb == NULL) {

            //
            // Use a cdb that is all 0s
            //
            cdb = &noOp;
        }

    } else {

        cdb = (PCDB)(Srb->Cdb);
        scsiStatus = Srb->ScsiStatus;
        senseBufferLength = Srb->SenseInfoBufferLength;
        senseBuffer = Srb->SenseInfoBuffer;
    }

    if (*Status == STATUS_DATA_OVERRUN &&
        (cdb != NULL) &&
        (IS_SCSIOP_READWRITE(cdb->CDB10.OperationCode))) {

            *Retry = TRUE;

            //
            // Update the error count for the device.
            //

            fdoExtension->ErrorCount++;

    } else if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_ERROR &&
               scsiStatus == SCSISTAT_BUSY) {

        //
        // a disk drive should never be busy this long. Reset the scsi bus
        // maybe this will clear the condition.
        //

        ResetBus(Fdo);

        //
        // Update the error count for the device.
        //

        fdoExtension->ErrorCount++;

    } else {

        BOOLEAN invalidatePartitionTable = FALSE;

        //
        // See if this might indicate that something on the drive has changed.
        //

        if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
            (senseBuffer != NULL) && (cdb != NULL)) {

            BOOLEAN validSense = FALSE;
            UCHAR senseKey = 0;
            UCHAR asc = 0;
            UCHAR ascq = 0;

            validSense = ScsiGetSenseKeyAndCodes(senseBuffer,
                                                 senseBufferLength,
                                                 SCSI_SENSE_OPTIONS_FIXED_FORMAT_IF_UNKNOWN_FORMAT_INDICATED,
                                                 &senseKey,
                                                 &asc,
                                                 &ascq);

            if (validSense) {

                switch (senseKey) {

                    case SCSI_SENSE_ILLEGAL_REQUEST: {

                        switch (asc) {

                            case SCSI_ADSENSE_INVALID_CDB:
                            {
                                //
                                // Look to see if this is an Io request with the ForceUnitAccess flag set
                                //
                                if (((cdb->CDB10.OperationCode == SCSIOP_WRITE)   ||
                                    (cdb->CDB10.OperationCode == SCSIOP_WRITE16)) &&
                                    (cdb->CDB10.ForceUnitAccess))
                                {
                                    PDISK_DATA diskData = (PDISK_DATA)fdoExtension->CommonExtension.DriverData;

                                    if (diskData->WriteCacheOverride == DiskWriteCacheEnable)
                                    {
                                        PIO_ERROR_LOG_PACKET logEntry = NULL;

                                        //
                                        // The user has explicitly requested that write caching be turned on.
                                        // Warn the user that writes with FUA enabled are not working and that
                                        // they should disable write cache.
                                        //

                                        logEntry = IoAllocateErrorLogEntry(fdoExtension->DeviceObject,
                                                                           sizeof(IO_ERROR_LOG_PACKET) + (4 * sizeof(ULONG)));

                                        if (logEntry != NULL)
                                        {
                                            logEntry->FinalStatus       = *Status;
                                            logEntry->ErrorCode         = IO_WARNING_WRITE_FUA_PROBLEM;
                                            logEntry->SequenceNumber    = 0;
                                            logEntry->MajorFunctionCode = IRP_MJ_SCSI;
                                            logEntry->IoControlCode     = 0;
                                            logEntry->RetryCount        = 0;
                                            logEntry->UniqueErrorValue  = 0;
                                            logEntry->DumpDataSize      = 4 * sizeof(ULONG);

                                            logEntry->DumpData[0] = diskData->ScsiAddress.PortNumber;
                                            logEntry->DumpData[1] = diskData->ScsiAddress.PathId;
                                            logEntry->DumpData[2] = diskData->ScsiAddress.TargetId;
                                            logEntry->DumpData[3] = diskData->ScsiAddress.Lun;

                                            //
                                            // Write the error log packet.
                                            //

                                            IoWriteErrorLogEntry(logEntry);
                                        }
                                    }
                                    else
                                    {
                                        //
                                        // Turn off write caching on this device. This is so that future
                                        // critical requests need not be sent down with  ForceUnitAccess
                                        //
                                        PIO_WORKITEM workItem = IoAllocateWorkItem(Fdo);

                                        if (workItem)
                                        {
                                            IoQueueWorkItem(workItem, DisableWriteCache, CriticalWorkQueue, workItem);
                                        }
                                    }

                                    SET_FLAG(fdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_FUA_NOT_SUPPORTED);
                                    ADJUST_FUA_FLAG(fdoExtension);


                                    cdb->CDB10.ForceUnitAccess = FALSE;
                                    *Retry = TRUE;

                                } else if ((cdb->CDB6FORMAT.OperationCode == SCSIOP_MODE_SENSE) &&
                                           (cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL)) {

                                    //
                                    // Mode sense for all pages failed. This command could fail with
                                    // SCSI_SENSE_ILLEGAL_REQUEST / SCSI_ADSENSE_INVALID_CDB if the data
                                    // to be returned is more than 256 bytes. In which case, try to get
                                    // only MODE_PAGE_CACHING since we only need the block descriptor.
                                    //
                                    // Simply change the page code and retry the request
                                    //

                                    cdb->MODE_SENSE.PageCode = MODE_PAGE_CACHING;
                                    *Retry = TRUE;
                                }

                                break;
                            }
                        } // end switch(asc)
                        break;
                    }

                    case SCSI_SENSE_NOT_READY: {

                        switch (asc) {
                        case SCSI_ADSENSE_LUN_NOT_READY: {
                            switch (ascq) {
                            case SCSI_SENSEQ_BECOMING_READY:
                            case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED:
                            case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE: {
                                invalidatePartitionTable = TRUE;
                                break;
                            }
                            } // end switch(ascq)
                            break;
                        }

                        case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE: {
                            invalidatePartitionTable = TRUE;
                            break;
                        }
                        } // end switch(asc)
                        break;
                    }

                    case SCSI_SENSE_MEDIUM_ERROR: {
                        invalidatePartitionTable = TRUE;
                        break;
                    }

                    case SCSI_SENSE_HARDWARE_ERROR: {
                        invalidatePartitionTable = TRUE;
                        break;
                    }

                    case SCSI_SENSE_UNIT_ATTENTION:
                    {
                        invalidatePartitionTable = TRUE;
                        break;
                    }

                    case SCSI_SENSE_RECOVERED_ERROR: {
                        invalidatePartitionTable = TRUE;
                        break;
                    }

                } // end switch(senseKey)
            } // end if (validSense)
        } else {

            //
            // On any exceptional scsi condition which might indicate that the
            // device was changed we will flush out the state of the partition
            // table.
            //

            switch (SRB_STATUS(Srb->SrbStatus)) {
                case SRB_STATUS_INVALID_LUN:
                case SRB_STATUS_INVALID_TARGET_ID:
                case SRB_STATUS_NO_DEVICE:
                case SRB_STATUS_NO_HBA:
                case SRB_STATUS_INVALID_PATH_ID:
                case SRB_STATUS_COMMAND_TIMEOUT:
                case SRB_STATUS_TIMEOUT:
                case SRB_STATUS_SELECTION_TIMEOUT:
                case SRB_STATUS_REQUEST_FLUSHED:
                case SRB_STATUS_UNEXPECTED_BUS_FREE:
                case SRB_STATUS_PARITY_ERROR:
                {
                    invalidatePartitionTable = TRUE;
                    break;
                }

                case SRB_STATUS_ERROR:
                {
                    if (scsiStatus == SCSISTAT_RESERVATION_CONFLICT)
                    {
                        invalidatePartitionTable = TRUE;
                    }

                    break;
                }
            } // end switch(Srb->SrbStatus)
        }

        if (invalidatePartitionTable && TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {

            //
            // Inform the upper layers that the volume
            // on this disk is in need of verification
            //

            SET_FLAG(Fdo->Flags, DO_VERIFY_VOLUME);
        }
    }

    return;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskSetSpecialHacks(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN ULONG_PTR Data
    )

/*++

Routine Description:

    This function checks to see if an SCSI logical unit requires speical
    flags to be set.

Arguments:

    Fdo - Supplies the device object to be tested.

    InquiryData - Supplies the inquiry data returned by the device of interest.

    AdapterDescriptor - Supplies the capabilities of the device object.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT fdo = FdoExtension->DeviceObject;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "Disk SetSpecialHacks, Setting Hacks %p\n", (void*) Data));

    //
    // Found a listed controller.  Determine what must be done.
    //

    if (TEST_FLAG(Data, HackDisableTaggedQueuing)) {

        //
        // Disable tagged queuing.
        //

        CLEAR_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);
    }

    if (TEST_FLAG(Data, HackDisableSynchronousTransfers)) {

        //
        // Disable synchronous data transfers.
        //

        SET_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

    }

    if (TEST_FLAG(Data, HackDisableSpinDown)) {

        //
        // Disable spinning down of drives.
        //

        SET_FLAG(FdoExtension->ScanForSpecialFlags,
                 CLASS_SPECIAL_DISABLE_SPIN_DOWN);

    }

    if (TEST_FLAG(Data, HackDisableWriteCache)) {

        //
        // Disable the drive's write cache
        //

        SET_FLAG(FdoExtension->ScanForSpecialFlags,
                 CLASS_SPECIAL_DISABLE_WRITE_CACHE);

    }

    if (TEST_FLAG(Data, HackCauseNotReportableHack)) {

        SET_FLAG(FdoExtension->ScanForSpecialFlags,
                 CLASS_SPECIAL_CAUSE_NOT_REPORTABLE_HACK);
    }

    if (TEST_FLAG(fdo->Characteristics, FILE_REMOVABLE_MEDIA) &&
        TEST_FLAG(Data, HackRequiresStartUnitCommand)
        ) {

        //
        // this is a list of vendors who require the START_UNIT command
        //

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "DiskScanForSpecial (%p) => This unit requires "
                    " START_UNITS\n", fdo));
        SET_FLAG(FdoExtension->DeviceFlags, DEV_SAFE_START_UNIT);

    }

    return;
}


VOID
ResetBus(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This command sends a reset bus command to the SCSI port driver.

Arguments:

    Fdo - The functional device object for the logical unit with hardware problem.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION irpStack;
    PIRP irp;

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCOMPLETION_CONTEXT context;
    PSTORAGE_REQUEST_BLOCK srbEx = NULL;
    PSTOR_ADDR_BTL8 storAddrBtl8 = NULL;

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "Disk ResetBus: Sending reset bus request to port driver.\n"));

    //
    // Allocate Srb from nonpaged pool.
    //

    context = ExAllocatePoolWithTag(NonPagedPoolNx,
                                    sizeof(COMPLETION_CONTEXT),
                                    DISK_TAG_CCONTEXT);

    if(context == NULL) {
        return;
    }

    //
    // Save the device object in the context for use by the completion
    // routine.
    //

    context->DeviceObject = Fdo;
    srb = &context->Srb.Srb;

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx = &context->Srb.SrbEx;

        //
        // Zero out srb
        //

        RtlZeroMemory(srbEx, sizeof(context->Srb.SrbExBuffer));

        //
        // Set up STORAGE_REQUEST_BLOCK fields
        //

        srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
        srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
        srbEx->Signature = SRB_SIGNATURE;
        srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
        srbEx->SrbLength = sizeof(context->Srb.SrbExBuffer);
        srbEx->SrbFunction = SRB_FUNCTION_RESET_BUS;
        srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);

        //
        // Set up address fields
        //

        storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
        storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
        storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

    } else {

        //
        // Zero out srb.
        //

        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

        //
        // Write length to SRB.
        //

        srb->Length = SCSI_REQUEST_BLOCK_SIZE;

        srb->Function = SRB_FUNCTION_RESET_BUS;

    }

    //
    // Build the asynchronous request to be sent to the port driver.
    // Since this routine is called from a DPC the IRP should always be
    // available.
    //

    irp = IoAllocateIrp(Fdo->StackSize, FALSE);

    if (irp == NULL) {
        FREE_POOL(context);
        return;
    }

    ClassAcquireRemoveLock(Fdo, irp);

    IoSetCompletionRoutine(irp,
                           (PIO_COMPLETION_ROUTINE)ClassAsynchronousCompletion,
                           context,
                           TRUE,
                           TRUE,
                           TRUE);

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx->RequestPriority = IoGetIoPriorityHint(irp);
        srbEx->OriginalRequest = irp;
    } else {
        srb->OriginalRequest = irp;
    }

    //
    // Store the SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = srb;

    //
    // Call the port driver with the IRP.
    //

    IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

    return;

} // end ResetBus()



VOID
DiskLogCacheInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PDISK_CACHE_INFORMATION CacheInfo,
    IN NTSTATUS Status
    )
{
    PIO_ERROR_LOG_PACKET logEntry = NULL;

    PAGED_CODE();

    logEntry = IoAllocateErrorLogEntry(FdoExtension->DeviceObject, sizeof(IO_ERROR_LOG_PACKET) + (4 * sizeof(ULONG)));

    if (logEntry != NULL)
    {
        PDISK_DATA diskData = FdoExtension->CommonExtension.DriverData;
        BOOLEAN bIsEnabled  = TEST_FLAG(FdoExtension->DeviceFlags, DEV_WRITE_CACHE);

        logEntry->FinalStatus       = Status;
        logEntry->ErrorCode         = (bIsEnabled) ? IO_WRITE_CACHE_ENABLED : IO_WRITE_CACHE_DISABLED;
        logEntry->SequenceNumber    = 0;
        logEntry->MajorFunctionCode = IRP_MJ_SCSI;
        logEntry->IoControlCode     = 0;
        logEntry->RetryCount        = 0;
        logEntry->UniqueErrorValue  = 0x1;
        logEntry->DumpDataSize      = 4 * sizeof(ULONG);

        logEntry->DumpData[0] = diskData->ScsiAddress.PathId;
        logEntry->DumpData[1] = diskData->ScsiAddress.TargetId;
        logEntry->DumpData[2] = diskData->ScsiAddress.Lun;
        logEntry->DumpData[3] = CacheInfo->WriteCacheEnabled;

        //
        // Write the error log packet.
        //

        IoWriteErrorLogEntry(logEntry);
    }
}

NTSTATUS
DiskGetInfoExceptionInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PMODE_INFO_EXCEPTIONS ReturnPageData
    )
{
    PMODE_PARAMETER_HEADER modeData;
    PMODE_INFO_EXCEPTIONS pageData;
    ULONG length;

    NTSTATUS status;

    PAGED_CODE();

    //
    // ReturnPageData is allocated by the caller
    //

    modeData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                         MODE_DATA_SIZE,
                                         DISK_TAG_INFO_EXCEPTION);

    if (modeData == NULL) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_WMI, "DiskGetInfoExceptionInformation: Unable to allocate mode "
                       "data buffer\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeData, MODE_DATA_SIZE);

    length = ClassModeSense(FdoExtension->DeviceObject,
                            (PCHAR) modeData,
                            MODE_DATA_SIZE,
                            MODE_PAGE_FAULT_REPORTING);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        length = ClassModeSense(FdoExtension->DeviceObject,
                                (PCHAR) modeData,
                                MODE_DATA_SIZE,
                                MODE_PAGE_FAULT_REPORTING);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_WMI, "DiskGetInfoExceptionInformation: Mode Sense failed\n"));
            FREE_POOL(modeData);
            return STATUS_IO_DEVICE_ERROR;
        }
    }

    //
    // If the length is greater than length indicated by the mode data reset
    // the data to the mode data.
    //

    if (length > (ULONG) (modeData->ModeDataLength + 1)) {
        length = modeData->ModeDataLength + 1;
    }

    //
    // Find the mode page for info exceptions
    //

    pageData = ClassFindModePage((PCHAR) modeData,
                                 length,
                                 MODE_PAGE_FAULT_REPORTING,
                                 TRUE);

    if (pageData != NULL) {
        RtlCopyMemory(ReturnPageData, pageData, sizeof(MODE_INFO_EXCEPTIONS));
        status =  STATUS_SUCCESS;
    } else {
        status = STATUS_NOT_SUPPORTED;
    }

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_WMI, "DiskGetInfoExceptionInformation: %s support SMART for device %p\n",
                  NT_SUCCESS(status) ? "does" : "does not",
                  FdoExtension->DeviceObject));

    FREE_POOL(modeData);

    return(status);
}


NTSTATUS
DiskSetInfoExceptionInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PMODE_INFO_EXCEPTIONS PageData
    )

{
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // We will attempt (twice) to issue the mode select with the page.
    // Make the setting persistant so that we don't have to turn it back
    // on after a bus reset.
    //

    for (i = 0; i < 2; i++)
    {
        status = DiskModeSelect(FdoExtension->DeviceObject,
                                (PCHAR) PageData,
                                sizeof(MODE_INFO_EXCEPTIONS),
                                TRUE);
    }

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_WMI, "DiskSetInfoExceptionInformation: %s for device %p\n",
                        NT_SUCCESS(status) ? "succeeded" : "failed",
                        FdoExtension->DeviceObject));

    return status;
}

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskGetCacheInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PDISK_CACHE_INFORMATION CacheInfo
    )
/*++

Routine Description:

    This function gets the caching mode page from the drive. This function
    is called from DiskIoctlGetCacheInformation() in response to the IOCTL
    IOCTL_DISK_GET_CACHE_INFORMATION. This is also called from the
    DisableWriteCache() worker thread to disable caching when write commands fail.

Arguments:

    FdoExtension - The device extension for this device.

    CacheInfo - Buffer to receive the Cache Information.

Return Value:

    NTSTATUS code

--*/

{
    PMODE_PARAMETER_HEADER modeData;
    PMODE_CACHING_PAGE pageData;

    ULONG length;

    PAGED_CODE();



    modeData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                     MODE_DATA_SIZE,
                                     DISK_TAG_DISABLE_CACHE);

    if (modeData == NULL) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskGetSetCacheInformation: Unable to allocate mode "
                       "data buffer\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeData, MODE_DATA_SIZE);

    length = ClassModeSense(FdoExtension->DeviceObject,
                            (PCHAR) modeData,
                            MODE_DATA_SIZE,
                            MODE_PAGE_CACHING);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        length = ClassModeSense(FdoExtension->DeviceObject,
                                (PCHAR) modeData,
                                MODE_DATA_SIZE,
                                MODE_PAGE_CACHING);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskGetCacheInformation: Mode Sense failed\n"));

            FREE_POOL(modeData);
            return STATUS_IO_DEVICE_ERROR;
        }
    }

    //
    // If the length is greater than length indicated by the mode data reset
    // the data to the mode data.
    //

    if (length > (ULONG) (modeData->ModeDataLength + 1)) {
        length = modeData->ModeDataLength + 1;
    }

    //
    // Check to see if the write cache is enabled.
    //

    pageData = ClassFindModePage((PCHAR) modeData,
                                 length,
                                 MODE_PAGE_CACHING,
                                 TRUE);

    //
    // Check if valid caching page exists.
    //

    if (pageData == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskGetCacheInformation: Unable to find caching mode page.\n"));
        FREE_POOL(modeData);
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Copy the parameters over.
    //

    RtlZeroMemory(CacheInfo, sizeof(DISK_CACHE_INFORMATION));

    CacheInfo->ParametersSavable = pageData->PageSavable;

    CacheInfo->ReadCacheEnabled = !(pageData->ReadDisableCache);
    CacheInfo->WriteCacheEnabled = pageData->WriteCacheEnable;


    //
    // Translate the values in the mode page into the ones defined in
    // ntdddisk.h.
    //

    CacheInfo->ReadRetentionPriority =
        TRANSLATE_RETENTION_PRIORITY(pageData->ReadRetensionPriority);
    CacheInfo->WriteRetentionPriority =
        TRANSLATE_RETENTION_PRIORITY(pageData->WriteRetensionPriority);

    CacheInfo->DisablePrefetchTransferLength =
        ((pageData->DisablePrefetchTransfer[0] << 8) +
         pageData->DisablePrefetchTransfer[1]);

    CacheInfo->ScalarPrefetch.Minimum =
        ((pageData->MinimumPrefetch[0] << 8) + pageData->MinimumPrefetch[1]);

    CacheInfo->ScalarPrefetch.Maximum =
        ((pageData->MaximumPrefetch[0] << 8) + pageData->MaximumPrefetch[1]);

    if(pageData->MultiplicationFactor) {
        CacheInfo->PrefetchScalar = TRUE;
        CacheInfo->ScalarPrefetch.MaximumBlocks =
            ((pageData->MaximumPrefetchCeiling[0] << 8) +
             pageData->MaximumPrefetchCeiling[1]);
    }


    FREE_POOL(modeData);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DiskSetCacheInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PDISK_CACHE_INFORMATION CacheInfo
    )
/*++

Routine Description:

    This function sets the caching mode page in the drive. This function
    is also called from the DisableWriteCache() worker thread to disable
    caching when write commands fail.

Arguments:

    FdoExtension - The device extension for this device.

    CacheInfo - Buffer the contains the Cache Information to be set on the drive.

Return Value:

    NTSTATUS code

--*/
{
    PMODE_PARAMETER_HEADER modeData;
    ULONG length;
    PMODE_CACHING_PAGE pageData;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    modeData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                     MODE_DATA_SIZE,
                                     DISK_TAG_DISABLE_CACHE);

    if (modeData == NULL) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskSetCacheInformation: Unable to allocate mode "
                       "data buffer\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeData, MODE_DATA_SIZE);

    length = ClassModeSense(FdoExtension->DeviceObject,
                            (PCHAR) modeData,
                            MODE_DATA_SIZE,
                            MODE_PAGE_CACHING);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        length = ClassModeSense(FdoExtension->DeviceObject,
                                (PCHAR) modeData,
                                MODE_DATA_SIZE,
                                MODE_PAGE_CACHING);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskSetCacheInformation: Mode Sense failed\n"));

            FREE_POOL(modeData);
            return STATUS_IO_DEVICE_ERROR;
        }
    }

    //
    // If the length is greater than length indicated by the mode data reset
    // the data to the mode data.
    //

    if (length > (ULONG) (modeData->ModeDataLength + 1)) {
        length = modeData->ModeDataLength + 1;
    }

    //
    // Check to see if the write cache is enabled.
    //

    pageData = ClassFindModePage((PCHAR) modeData,
                                 length,
                                 MODE_PAGE_CACHING,
                                 TRUE);

    //
    // Check if valid caching page exists.
    //

    if (pageData == NULL) {
        FREE_POOL(modeData);
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Don't touch any of the normal parameters - not all drives actually
    // use the correct size of caching mode page.  Just change the things
    // which the user could have modified.
    //

    pageData->PageSavable = FALSE;

    pageData->ReadDisableCache = !(CacheInfo->ReadCacheEnabled);
    pageData->MultiplicationFactor = CacheInfo->PrefetchScalar;
    pageData->WriteCacheEnable = CacheInfo->WriteCacheEnabled;

    pageData->WriteRetensionPriority = (UCHAR)
        TRANSLATE_RETENTION_PRIORITY(CacheInfo->WriteRetentionPriority);
    pageData->ReadRetensionPriority = (UCHAR)
        TRANSLATE_RETENTION_PRIORITY(CacheInfo->ReadRetentionPriority);

    pageData->DisablePrefetchTransfer[0] =
        (UCHAR) (CacheInfo->DisablePrefetchTransferLength >> 8);
    pageData->DisablePrefetchTransfer[1] =
        (UCHAR) (CacheInfo->DisablePrefetchTransferLength & 0x00ff);

    pageData->MinimumPrefetch[0] =
        (UCHAR) (CacheInfo->ScalarPrefetch.Minimum >> 8);
    pageData->MinimumPrefetch[1] =
        (UCHAR) (CacheInfo->ScalarPrefetch.Minimum & 0x00ff);

    pageData->MaximumPrefetch[0] =
        (UCHAR) (CacheInfo->ScalarPrefetch.Maximum >> 8);
    pageData->MaximumPrefetch[1] =
        (UCHAR) (CacheInfo->ScalarPrefetch.Maximum & 0x00ff);

    if(pageData->MultiplicationFactor) {

        pageData->MaximumPrefetchCeiling[0] =
            (UCHAR) (CacheInfo->ScalarPrefetch.MaximumBlocks >> 8);
        pageData->MaximumPrefetchCeiling[1] =
            (UCHAR) (CacheInfo->ScalarPrefetch.MaximumBlocks & 0x00ff);
    }

    //
    // We will attempt (twice) to issue the mode select with the page.
    //

    for (i = 0; i < 2; i++) {

        status = DiskModeSelect(FdoExtension->DeviceObject,
                                (PCHAR) pageData,
                                (pageData->PageLength + 2),
                                CacheInfo->ParametersSavable);

        if (NT_SUCCESS(status)) {

            if (CacheInfo->WriteCacheEnabled)
            {
                SET_FLAG(FdoExtension->DeviceFlags, DEV_WRITE_CACHE);
            }
            else
            {
                CLEAR_FLAG(FdoExtension->DeviceFlags, DEV_WRITE_CACHE);
            }
            ADJUST_FUA_FLAG(FdoExtension);

            break;
        }
    }

    if (NT_SUCCESS(status))
    {
    } else {

        //
        // We were unable to modify the disk write cache setting
        //

        SET_FLAG(FdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_MODIFY_CACHE_UNSUCCESSFUL);
    }

    FREE_POOL(modeData);
    return status;
}

NTSTATUS
DiskIoctlGetCacheSetting(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine description:

    This routine services IOCTL_DISK_GET_CACHE_SETTING. It looks to
    see if there are any issues with the disk cache and whether the
    user had previously indicated that the cache is power-protected

Arguments:

    Fdo - The functional device object processing the request
    Irp - The ioctl to be processed

Return Value:

    STATUS_SUCCESS if successful, an error code otherwise

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_CACHE_SETTING))
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        PDISK_CACHE_SETTING cacheSetting = (PDISK_CACHE_SETTING)Irp->AssociatedIrp.SystemBuffer;

        cacheSetting->Version = sizeof(DISK_CACHE_SETTING);
        cacheSetting->State   = DiskCacheNormal;

        //
        // Determine whether it is safe to turn on the cache
        //
        if (TEST_FLAG(fdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_FUA_NOT_SUPPORTED))
        {
            cacheSetting->State = DiskCacheWriteThroughNotSupported;
        }

        //
        // Determine whether it is possible to modify the cache setting
        //
        if (TEST_FLAG(fdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_MODIFY_CACHE_UNSUCCESSFUL))
        {
            cacheSetting->State = DiskCacheModifyUnsuccessful;
        }

        cacheSetting->IsPowerProtected = TEST_FLAG(fdoExtension->DeviceFlags, DEV_POWER_PROTECTED);

        Irp->IoStatus.Information = sizeof(DISK_CACHE_SETTING);
    }

    return status;
}


NTSTATUS
DiskIoctlSetCacheSetting(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine description:

    This routine services IOCTL_DISK_SET_CACHE_SETTING. It allows
    the user to specify whether the disk cache is power-protected
    or not

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    Fdo - The functional device object processing the request
    Irp - The ioctl to be processed

Return Value:

    STATUS_SUCCESS if successful, an error code otherwise

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(DISK_CACHE_SETTING))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else
    {
        PDISK_CACHE_SETTING cacheSetting = (PDISK_CACHE_SETTING)Irp->AssociatedIrp.SystemBuffer;

        if (cacheSetting->Version == sizeof(DISK_CACHE_SETTING))
        {
            ULONG isPowerProtected;

            //
            // Save away the user-defined override in our extension and the registry
            //
            if (cacheSetting->IsPowerProtected)
            {
                SET_FLAG(fdoExtension->DeviceFlags, DEV_POWER_PROTECTED);
                isPowerProtected = 1;
            }
            else
            {
                CLEAR_FLAG(fdoExtension->DeviceFlags, DEV_POWER_PROTECTED);
                isPowerProtected = 0;
            }
            ADJUST_FUA_FLAG(fdoExtension);

            ClassSetDeviceParameter(fdoExtension, DiskDeviceParameterSubkey, DiskDeviceCacheIsPowerProtected, isPowerProtected);
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
DiskIoctlGetLengthInfo(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_GET_LENGTH_INFO. It returns
    the disk geometry to the caller.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack;
    PGET_LENGTH_INFORMATION lengthInfo;
    PFUNCTIONAL_DEVICE_EXTENSION p0Extension;
    PCOMMON_DEVICE_EXTENSION commonExtension;
    PDISK_DATA partitionZeroData;
    NTSTATUS oldReadyStatus;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Initialization
    //

    commonExtension = DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    p0Extension = commonExtension->PartitionZeroExtension;
    partitionZeroData = ((PDISK_DATA) p0Extension->CommonExtension.DriverData);

    //
    // Check that the buffer is large enough.
    //

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_LENGTH_INFORMATION)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Update the geometry in case it has changed
    //

    status = DiskReadDriveCapacity(p0Extension->DeviceObject);

    //
    // Note whether the drive is ready.  If the status has changed then
    // notify pnp.
    //

    oldReadyStatus = InterlockedExchange(&(partitionZeroData->ReadyStatus), status);

    if(partitionZeroData->ReadyStatus != oldReadyStatus) {
        IoInvalidateDeviceRelations(p0Extension->LowerPdo, BusRelations);
    }

    if(!NT_SUCCESS(status)) {
        return status;
    }
    lengthInfo = (PGET_LENGTH_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    lengthInfo->Length = commonExtension->PartitionLength;

    status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);

    return status;
}

NTSTATUS
DiskIoctlGetDriveGeometry(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_GET_DRIVE_GEOMETRY. It returns
    the disk geometry to the caller.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - IRP with a return buffer large enough to receive the
            extended geometry information.

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetDriveGeometry: Output buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (TEST_FLAG(DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA)) {

        //
        // Issue ReadCapacity to update device extension
        // with information for current media.
        //

        status = DiskReadDriveCapacity(commonExtension->PartitionZeroExtension->DeviceObject);

        //
        // Note whether the drive is ready.
        //

        diskData->ReadyStatus = status;

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    //
    // Copy drive geometry information from device extension.
    //

    RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                  &(fdoExtension->DiskGeometry),
                  sizeof(DISK_GEOMETRY));

    if (((PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer)->BytesPerSector == 0) {
        ((PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer)->BytesPerSector = 512;
    }
    Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
    return STATUS_SUCCESS;
}

typedef struct _DISK_GEOMETRY_EX_INTERNAL {
    DISK_GEOMETRY Geometry;
    LARGE_INTEGER DiskSize;
    DISK_PARTITION_INFO Partition;
    DISK_DETECTION_INFO Detection;
} DISK_GEOMETRY_EX_INTERNAL, *PDISK_GEOMETRY_EX_INTERNAL;

NTSTATUS
DiskIoctlGetDriveGeometryEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_GET_DRIVE_GEOMETRY_EX. It returns
    the extended disk geometry to the caller.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - The device object to obtain the geometry for.

    Irp - IRP with a return buffer large enough to receive the
            extended geometry information.

Return Value:

    NTSTATUS code

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack;
    PCOMMON_DEVICE_EXTENSION commonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PDISK_DATA diskData;
    PDISK_GEOMETRY_EX_INTERNAL geometryEx;
    ULONG OutputBufferLength;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Setup parameters
    //

    commonExtension = DeviceObject->DeviceExtension;
    fdoExtension = DeviceObject->DeviceExtension;
    diskData = (PDISK_DATA)(commonExtension->DriverData);
    irpStack = IoGetCurrentIrpStackLocation ( Irp );
    geometryEx = NULL;
    OutputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Check that the buffer is large enough. It must be large enough
    // to hold at lest the Geometry and DiskSize fields of of the
    // DISK_GEOMETRY_EX structure.
    //

    if ( (LONG)OutputBufferLength < FIELD_OFFSET (DISK_GEOMETRY_EX, Data) ) {

        //
        // Buffer too small. Bail out, telling the caller the required
        // size.
        //

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetDriveGeometryEx: Output buffer too small.\n"));
        status = STATUS_BUFFER_TOO_SMALL;
        return status;
    }

    if (TEST_FLAG (DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA)) {

        //
        // Issue a ReadCapacity to update device extension
        // with information for the current media.
        //

        status = DiskReadDriveCapacity(commonExtension->PartitionZeroExtension->DeviceObject);

        diskData->ReadyStatus = status;

        if (!NT_SUCCESS (status)) {
            return status;
        }
    }

    //
    // Copy drive geometry.
    //

    geometryEx = (PDISK_GEOMETRY_EX_INTERNAL)Irp->AssociatedIrp.SystemBuffer;
    geometryEx->Geometry = fdoExtension->DiskGeometry;
    if (geometryEx->Geometry.BytesPerSector == 0) {
        geometryEx->Geometry.BytesPerSector = 512;
    }
    geometryEx->DiskSize = commonExtension->PartitionZeroExtension->CommonExtension.PartitionLength;

    //
    // If the user buffer is large enough to hold the partition information
    // then add that as well.
    //

    if ((LONG)OutputBufferLength >=  FIELD_OFFSET (DISK_GEOMETRY_EX_INTERNAL, Detection)) {

        geometryEx->Partition.SizeOfPartitionInfo = sizeof (geometryEx->Partition);
        geometryEx->Partition.PartitionStyle = diskData->PartitionStyle;

        switch ( diskData->PartitionStyle ) {

            case PARTITION_STYLE_GPT:

                //
                // Copy GPT signature.
                //

                geometryEx->Partition.Gpt.DiskId = diskData->Efi.DiskId;
                break;

            case PARTITION_STYLE_MBR:

                //
                // Copy MBR signature and checksum.
                //

                geometryEx->Partition.Mbr.Signature = diskData->Mbr.Signature;
                geometryEx->Partition.Mbr.CheckSum = diskData->Mbr.MbrCheckSum;
                break;

            default:

                //
                // This is a raw disk. Zero out the signature area so
                // nobody gets confused.
                //

                RtlZeroMemory(&geometryEx->Partition, sizeof (geometryEx->Partition));
        }
    }

    //
    // If the buffer is large enough to hold the detection information,
    // then also add that.
    //

    if (OutputBufferLength >= sizeof (DISK_GEOMETRY_EX_INTERNAL)) {

        geometryEx->Detection.SizeOfDetectInfo = sizeof (geometryEx->Detection);

        status = DiskGetDetectInfo(fdoExtension, &geometryEx->Detection);

        //
        // Failed to obtain detection information, set to none.
        //

        if (!NT_SUCCESS (status)) {
            geometryEx->Detection.DetectionType = DetectNone;
        }
    }

    status = STATUS_SUCCESS;
    Irp->IoStatus.Information = min (OutputBufferLength, sizeof (DISK_GEOMETRY_EX_INTERNAL));

    return status;
}

NTSTATUS
DiskIoctlGetCacheInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_GET_CACHE_INFORMATION. It reads
    the caching mode page from the device and returns information to
    the caller. After validating the user parameter it calls the
    DiskGetCacheInformation() function to get the mode page.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    PDISK_CACHE_INFORMATION cacheInfo = Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS status;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlGetCacheInformation: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_CACHE_INFORMATION)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetCacheInformation: Output buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    status = DiskGetCacheInformation(fdoExtension, cacheInfo);

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(DISK_CACHE_INFORMATION);

        //
        // Make sure write cache setting is reflected in device extension
        //
        if (cacheInfo->WriteCacheEnabled)
        {
            SET_FLAG(fdoExtension->DeviceFlags, DEV_WRITE_CACHE);
        }
        else
        {
            CLEAR_FLAG(fdoExtension->DeviceFlags, DEV_WRITE_CACHE);
        }
        ADJUST_FUA_FLAG(fdoExtension);

    }
    return status;
}


NTSTATUS
DiskIoctlSetCacheInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_SET_CACHE_INFORMATION. It allows
    the caller to set the caching mode page on the device. This function
    validates the user parameter and calls the DiskSetCacheInformation()
    function to set the mode page. It also stores the cache value in the
    device extension and registry.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    PDISK_CACHE_INFORMATION cacheInfo = Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS status;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL is equal or above DISPATCH_LEVEL.
    //

    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlSetCacheInformation: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DISK_CACHE_INFORMATION)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSetCacheInformation: Input buffer length mismatch.\n"));
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    status = DiskSetCacheInformation(fdoExtension, cacheInfo);

    //
    // Save away the user-defined override in our extension and the registry
    //
    if (cacheInfo->WriteCacheEnabled) {
        diskData->WriteCacheOverride = DiskWriteCacheEnable;
    } else {
        diskData->WriteCacheOverride = DiskWriteCacheDisable;
    }

    ClassSetDeviceParameter(fdoExtension, DiskDeviceParameterSubkey,
                            DiskDeviceUserWriteCacheSetting, diskData->WriteCacheOverride);

    DiskLogCacheInformation(fdoExtension, cacheInfo, status);

    return status;
}

NTSTATUS
DiskIoctlGetMediaTypesEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_STORAGE_GET_MEDIA_TYPES_EX. It returns
    the media type information to the caller. After validating the user
    parameter it calls DiskDetermineMediaTypes() to get the media type.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status;

    PMODE_PARAMETER_HEADER modeData;
    PMODE_PARAMETER_BLOCK blockDescriptor;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    ULONG modeLength;
    ULONG retries = 4;
    UCHAR densityCode = 0;
    BOOLEAN writable = TRUE;
    BOOLEAN mediaPresent = FALSE;
    ULONG srbSize;
    PSTORAGE_REQUEST_BLOCK srbEx = NULL;
    PSTOR_ADDR_BTL8 storAddrBtl8 = NULL;
    PSRBEX_DATA_SCSI_CDB16 srbExDataCdb16 = NULL;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlGetMediaTypesEx: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_MEDIA_TYPES)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetMediaTypesEx: Output buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
    } else {
        srbSize = SCSI_REQUEST_BLOCK_SIZE;
    }

    srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                srbSize,
                                DISK_TAG_SRB);

    if (srb == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetMediaTypesEx: Unable to allocate memory.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, srbSize);

    //
    // Send a TUR to determine if media is present.
    //

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx = (PSTORAGE_REQUEST_BLOCK)srb;

        //
        // Set up STORAGE_REQUEST_BLOCK fields
        //

        srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
        srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
        srbEx->Signature = SRB_SIGNATURE;
        srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
        srbEx->SrbLength = srbSize;
        srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
        srbEx->RequestPriority = IoGetIoPriorityHint(Irp);
        srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);
        srbEx->NumSrbExData = 1;

        // Set timeout value.
        srbEx->TimeOutValue = fdoExtension->TimeOutValue;

        //
        // Set up address fields
        //

        storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
        storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
        storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

        //
        // Set up SCSI SRB extended data fields
        //

        srbEx->SrbExDataOffset[0] = sizeof(STORAGE_REQUEST_BLOCK) +
            sizeof(STOR_ADDR_BTL8);
        if ((srbEx->SrbExDataOffset[0] + sizeof(SRBEX_DATA_SCSI_CDB16)) <= srbEx->SrbLength) {
            srbExDataCdb16 = (PSRBEX_DATA_SCSI_CDB16)((PUCHAR)srbEx + srbEx->SrbExDataOffset[0]);
            srbExDataCdb16->Type = SrbExDataTypeScsiCdb16;
            srbExDataCdb16->Length = SRBEX_DATA_SCSI_CDB16_LENGTH;
            srbExDataCdb16->CdbLength = 6;

            cdb = (PCDB)srbExDataCdb16->Cdb;
        } else {
            // Should not happen
            NT_ASSERT(FALSE);

            FREE_POOL(srb);
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetMediaTypesEx: Insufficient extended SRB size.\n"));
            return STATUS_INTERNAL_ERROR;
        }

    } else {

        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->CdbLength = 6;
        cdb = (PCDB)srb->Cdb;

        //
        // Set timeout value.
        //

        srb->TimeOutValue = fdoExtension->TimeOutValue;

    }
    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    status = ClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     NULL,
                                     0,
                                     FALSE);

    if (NT_SUCCESS(status)) {
        mediaPresent = TRUE;
    }

    modeLength = MODE_DATA_SIZE;
    modeData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                     modeLength,
                                     DISK_TAG_MODE_DATA);

    if (modeData == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetMediaTypesEx: Unable to allocate memory.\n"));
        FREE_POOL(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeData, modeLength);

    //
    // Build the MODE SENSE CDB using previous SRB.
    //

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx->SrbStatus = 0;
        srbExDataCdb16->ScsiStatus = 0;
        srbExDataCdb16->CdbLength = 6;

        //
        // Set timeout value from device extension.
        //

        srbEx->TimeOutValue = fdoExtension->TimeOutValue;
    } else {
        srb->SrbStatus = 0;
        srb->ScsiStatus = 0;
        srb->CdbLength = 6;

        //
        // Set timeout value from device extension.
        //

        srb->TimeOutValue = fdoExtension->TimeOutValue;
    }

    //
    // Page code of 0x3F will return all pages.
    // This command could fail if the data to be returned is
    // more than 256 bytes. In which case, we should get only
    // the caching page since we only need the block descriptor.
    // DiskFdoProcessError will change the page code to
    // MODE_PAGE_CACHING if there is an error.
    //

    cdb->MODE_SENSE.OperationCode    = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode         = MODE_SENSE_RETURN_ALL;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)modeLength;

Retry:
    status = ClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     modeData,
                                     modeLength,
                                     FALSE);

    if (status == STATUS_VERIFY_REQUIRED) {

        if (retries--) {

            //
            // Retry request.
            //

            goto Retry;
        }
    } else if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status) || (status == STATUS_NO_MEDIA_IN_DEVICE)) {

        //
        // Get the block descriptor.
        //

        if (modeData->BlockDescriptorLength != 0) {

            blockDescriptor = (PMODE_PARAMETER_BLOCK)((ULONG_PTR)modeData + sizeof(MODE_PARAMETER_HEADER));
            densityCode = blockDescriptor->DensityCode;
        }

        if (TEST_FLAG(modeData->DeviceSpecificParameter,
                      MODE_DSP_WRITE_PROTECT)) {

            writable = FALSE;
        }

        status = DiskDetermineMediaTypes(DeviceObject,
                                         Irp,
                                         modeData->MediumType,
                                         densityCode,
                                         mediaPresent,
                                         writable);
        //
        // If the buffer was too small, DetermineMediaTypes updated the status and information and the request will fail.
        //

    } else {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetMediaTypesEx: Mode sense for header/bd failed. %lx\n", status));
    }

    FREE_POOL(srb);
    FREE_POOL(modeData);

    return status;
}

NTSTATUS
DiskIoctlPredictFailure(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_STORAGE_PREDICT_FAILURE. If the device
    supports SMART then it returns any available failure data.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status = STATUS_SUCCESS;

    PSTORAGE_PREDICT_FAILURE checkFailure;
    STORAGE_FAILURE_PREDICT_STATUS diskSmartStatus;
    IO_STATUS_BLOCK ioStatus = { 0 };
    KEVENT event;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlPredictFailure: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_PREDICT_FAILURE)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlPredictFailure: Output buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // See if the disk is predicting failure
    //

    checkFailure = (PSTORAGE_PREDICT_FAILURE)Irp->AssociatedIrp.SystemBuffer;

    if (diskData->FailurePredictionCapability == FailurePredictionSense) {
        ULONG readBufferSize;
        PUCHAR readBuffer;
        PIRP readIrp;
        PDEVICE_OBJECT topOfStack;

        checkFailure->PredictFailure = 0;

        KeInitializeEvent(&event, SynchronizationEvent, FALSE);

        topOfStack = IoGetAttachedDeviceReference(DeviceObject);

        //
        // SCSI disks need to have a read sent down to provoke any
        // failures to be reported.
        //
        // Issue a normal read operation.  The error-handling code in
        // classpnp will take care of a failure prediction by logging the
        // correct event.
        //

        readBufferSize = fdoExtension->DiskGeometry.BytesPerSector;
        readBuffer = ExAllocatePoolWithTag(NonPagedPoolNx,
                                           readBufferSize,
                                           DISK_TAG_SMART);

        if (readBuffer != NULL) {
            LARGE_INTEGER offset;

            offset.QuadPart = 0;
            readIrp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                                   topOfStack,
                                                   readBuffer,
                                                   readBufferSize,
                                                   &offset,
                                                   &event,
                                                   &ioStatus);

            if (readIrp != NULL) {

                status = IoCallDriver(topOfStack, readIrp);
                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                    status = ioStatus.Status;
                }


            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            FREE_POOL(readBuffer);
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        ObDereferenceObject(topOfStack);
    }

    if (status != STATUS_INSUFFICIENT_RESOURCES)
    {
        if ((diskData->FailurePredictionCapability == FailurePredictionSmart) ||
            (diskData->FailurePredictionCapability == FailurePredictionSense)) {

            status = DiskReadFailurePredictStatus(fdoExtension, &diskSmartStatus);

            if (NT_SUCCESS(status)) {

                status = DiskReadFailurePredictData(fdoExtension,
                                                    Irp->AssociatedIrp.SystemBuffer);

                if (diskSmartStatus.PredictFailure) {
                    checkFailure->PredictFailure = 1;
                } else {
                    checkFailure->PredictFailure = 0;
                }

                Irp->IoStatus.Information = sizeof(STORAGE_PREDICT_FAILURE);
            }
        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    return status;
}

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
NTSTATUS
DiskIoctlEnableFailurePrediction(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_STORAGE_FAILURE_PREDICTION_CONFIG. If the device
    supports SMART then it returns any available failure data.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status = STATUS_SUCCESS;
    PSTORAGE_FAILURE_PREDICTION_CONFIG enablePrediction;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlEnableFailurePrediction: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_FAILURE_PREDICTION_CONFIG) ||
        irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_FAILURE_PREDICTION_CONFIG)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlEnableFailurePrediction: Buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    enablePrediction = (PSTORAGE_FAILURE_PREDICTION_CONFIG)Irp->AssociatedIrp.SystemBuffer;

    if (enablePrediction->Version != STORAGE_FAILURE_PREDICTION_CONFIG_V1 ||
        enablePrediction->Size < sizeof(STORAGE_FAILURE_PREDICTION_CONFIG)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlEnableFailurePrediction: Buffer version or size is incorrect.\n"));
        status = STATUS_INVALID_PARAMETER;
    }

    if (enablePrediction->Reserved != 0) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlEnableFailurePrediction: Reserved bytes are not zero!\n"));
        status = STATUS_INVALID_PARAMETER;
    }

    //
    // Default to success.  This might get overwritten on failure below.
    //
    status = STATUS_SUCCESS;

    //
    // If this is a "set" and the current state (enabled/disabled) is
    // different from the sender's desired state,
    //
    if (enablePrediction->Set && enablePrediction->Enabled != diskData->FailurePredictionEnabled) {
        if (diskData->FailurePredictionCapability == FailurePredictionSmart ||
            diskData->FailurePredictionCapability == FailurePredictionIoctl) {
            //
            // SMART or IOCTL based failure prediction is being used so call
            // the generic function that is normally called in the WMI path.
            //
            status = DiskEnableDisableFailurePrediction(fdoExtension, enablePrediction->Enabled);
        } else if (diskData->ScsiInfoExceptionsSupported) {
            //
            // If we know that the device supports the Informational Exceptions
            // mode page, try to enable/disable failure prediction that way.
            //
            status = DiskEnableInfoExceptions(fdoExtension, enablePrediction->Enabled);
        }
    }

    //
    // Return the current state regardless if this was a "set" or a "get".
    //
    enablePrediction->Enabled = diskData->FailurePredictionEnabled;

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = sizeof(STORAGE_FAILURE_PREDICTION_CONFIG);
    }

    return status;
}
#endif //(NTDDI_VERSION >= NTDDI_WINBLUE)

NTSTATUS
DiskIoctlVerify(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_VERIFY. After verifying
    user input, it starts the worker thread DiskIoctlVerifyThread()
    to verify the device.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    PVERIFY_INFORMATION verifyInfo = Irp->AssociatedIrp.SystemBuffer;
    PDISK_VERIFY_WORKITEM_CONTEXT Context = NULL;
    PSCSI_REQUEST_BLOCK srb;
    LARGE_INTEGER byteOffset;
    ULONG srbSize;

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlVerify: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(VERIFY_INFORMATION)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlVerify: Input buffer length mismatch.\n"));
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
    } else {
        srbSize = SCSI_REQUEST_BLOCK_SIZE;
    }
    srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                srbSize,
                                DISK_TAG_SRB);

    if (srb == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlVerify: Unable to allocate memory.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, srbSize);

    //
    // Add disk offset to starting sector.
    //

    byteOffset.QuadPart = commonExtension->StartingOffset.QuadPart +
                          verifyInfo->StartingOffset.QuadPart;

    //
    // Perform a bounds check on the sector range
    //

    if ((verifyInfo->StartingOffset.QuadPart > commonExtension->PartitionLength.QuadPart) ||
        (verifyInfo->StartingOffset.QuadPart < 0)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlVerify: Verify request to invalid sector.\n"));
        FREE_POOL(srb)
        return STATUS_NONEXISTENT_SECTOR;
    } else {

        ULONGLONG bytesRemaining = commonExtension->PartitionLength.QuadPart - verifyInfo->StartingOffset.QuadPart;

        if ((ULONGLONG)verifyInfo->Length > bytesRemaining) {

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlVerify: Verify request to invalid sector.\n"));
            FREE_POOL(srb)
            return STATUS_NONEXISTENT_SECTOR;
        }
    }

    Context = ExAllocatePoolWithTag(NonPagedPoolNx,
                                    sizeof(DISK_VERIFY_WORKITEM_CONTEXT),
                                    DISK_TAG_WI_CONTEXT);
    if (Context) {

        Context->Irp = Irp;
        Context->Srb = srb;
        Context->WorkItem = IoAllocateWorkItem(DeviceObject);

        if (Context->WorkItem) {

            //
            // Queue the work item and return.
            //

            IoMarkIrpPending(Irp);

            IoQueueWorkItem(Context->WorkItem,
                            DiskIoctlVerifyThread,
                            DelayedWorkQueue,
                            Context);

            return STATUS_PENDING;
        }
        FREE_POOL(Context);
    }
    FREE_POOL(srb)
    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
DiskIoctlReassignBlocks(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_REASSIGN_BLOCKS. This IOCTL
    allows the caller to remap the defective blocks to a new
    location on the disk.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status;
    PREASSIGN_BLOCKS badBlocks = Irp->AssociatedIrp.SystemBuffer;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    ULONG bufferSize;
    ULONG blockNumber;
    ULONG blockCount;
    ULONG srbSize;
    PSTORAGE_REQUEST_BLOCK srbEx;
    PSTOR_ADDR_BTL8 storAddrBtl8;
    PSRBEX_DATA_SCSI_CDB16 srbExDataCdb16;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocks: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(REASSIGN_BLOCKS)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocks: Input buffer length mismatch.\n"));
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // Make sure we have some data in the input buffer.
    //

    if (badBlocks->Count == 0) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocks: Invalid block count\n"));
        return STATUS_INVALID_PARAMETER;
    }

    bufferSize = sizeof(REASSIGN_BLOCKS) + ((badBlocks->Count - 1) * sizeof(ULONG));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < bufferSize) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocks: Input buffer length mismatch for bad blocks.\n"));
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
    } else {
        srbSize = SCSI_REQUEST_BLOCK_SIZE;
    }
    srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                srbSize,
                                DISK_TAG_SRB);

    if (srb == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocks: Unable to allocate memory.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, srbSize);

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
        REVERSE_BYTES((PFOUR_BYTE) &badBlocks->BlockNumber[blockCount-1], (PFOUR_BYTE) &blockNumber);
    }

    //
    // Build a SCSI SRB containing a SCSIOP_REASSIGN_BLOCKS cdb
    //

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx = (PSTORAGE_REQUEST_BLOCK)srb;

        //
        // Set up STORAGE_REQUEST_BLOCK fields
        //

        srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
        srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
        srbEx->Signature = SRB_SIGNATURE;
        srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
        srbEx->SrbLength = srbSize;
        srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
        srbEx->RequestPriority = IoGetIoPriorityHint(Irp);
        srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);
        srbEx->NumSrbExData = 1;

        // Set timeout value.
        srbEx->TimeOutValue = fdoExtension->TimeOutValue;

        //
        // Set up address fields
        //

        storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
        storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
        storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

        //
        // Set up SCSI SRB extended data fields
        //

        srbEx->SrbExDataOffset[0] = sizeof(STORAGE_REQUEST_BLOCK) +
            sizeof(STOR_ADDR_BTL8);
        if ((srbEx->SrbExDataOffset[0] + sizeof(SRBEX_DATA_SCSI_CDB16)) <= srbEx->SrbLength) {
            srbExDataCdb16 = (PSRBEX_DATA_SCSI_CDB16)((PUCHAR)srbEx + srbEx->SrbExDataOffset[0]);
            srbExDataCdb16->Type = SrbExDataTypeScsiCdb16;
            srbExDataCdb16->Length = SRBEX_DATA_SCSI_CDB16_LENGTH;
            srbExDataCdb16->CdbLength = 6;

            cdb = (PCDB)srbExDataCdb16->Cdb;
        } else {
            // Should not happen
            NT_ASSERT(FALSE);

            FREE_POOL(srb);
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocks: Insufficient extended SRB size.\n"));
            return STATUS_INTERNAL_ERROR;
        }

    } else {
        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->CdbLength = 6;

        //
        // Set timeout value.
        //

        srb->TimeOutValue = fdoExtension->TimeOutValue;

        cdb = (PCDB)srb->Cdb;
    }

    cdb->CDB6GENERIC.OperationCode = SCSIOP_REASSIGN_BLOCKS;

    status = ClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     badBlocks,
                                     bufferSize,
                                     TRUE);

    FREE_POOL(srb);
    return status;
}

NTSTATUS
DiskIoctlReassignBlocksEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_REASSIGN_BLOCKS_EX. This IOCTL
    allows the caller to remap the defective blocks to a new
    location on the disk. The input buffer contains 8-byte LBAs.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status;
    PREASSIGN_BLOCKS_EX badBlocks = Irp->AssociatedIrp.SystemBuffer;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    LARGE_INTEGER blockNumber;
    ULONG bufferSize;
    ULONG blockCount;
    ULONG srbSize;
    PSTORAGE_REQUEST_BLOCK srbEx;
    PSTOR_ADDR_BTL8 storAddrBtl8;
    PSRBEX_DATA_SCSI_CDB16 srbExDataCdb16;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocksEx: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(REASSIGN_BLOCKS_EX)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocksEx: Input buffer length mismatch.\n"));
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // Make sure we have some data in the input buffer.
    //

    if (badBlocks->Count == 0) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocksEx: Invalid block count\n"));
        return STATUS_INVALID_PARAMETER;
    }

    bufferSize = sizeof(REASSIGN_BLOCKS_EX) + ((badBlocks->Count - 1) * sizeof(LARGE_INTEGER));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < bufferSize) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocksEx: Input buffer length mismatch for bad blocks.\n"));
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
    } else {
        srbSize = SCSI_REQUEST_BLOCK_SIZE;
    }
    srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                srbSize,
                                DISK_TAG_SRB);

    if (srb == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocks: Unable to allocate memory.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, srbSize);

    //
    // Build the data buffer to be transferred in the input buffer.
    // The format of the data to the device is:
    //
    //      2 bytes Reserved
    //      2 bytes Length
    //      x * 8 btyes Block Address
    //
    // All values are big endian.
    //

    badBlocks->Reserved = 0;
    blockCount = badBlocks->Count;

    //
    // Convert # of entries to # of bytes.
    //

    blockCount *= 8;
    badBlocks->Count = (USHORT) ((blockCount >> 8) & 0XFF);
    badBlocks->Count |= (USHORT) ((blockCount << 8) & 0XFF00);

    //
    // Convert back to number of entries.
    //

    blockCount /= 8;

    for (; blockCount > 0; blockCount--) {

        blockNumber = badBlocks->BlockNumber[blockCount-1];
        REVERSE_BYTES_QUAD(&badBlocks->BlockNumber[blockCount-1], &blockNumber);
    }

    //
    // Build a SCSI SRB containing a SCSIOP_REASSIGN_BLOCKS cdb
    //

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx = (PSTORAGE_REQUEST_BLOCK)srb;

        //
        // Set up STORAGE_REQUEST_BLOCK fields
        //

        srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
        srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
        srbEx->Signature = SRB_SIGNATURE;
        srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
        srbEx->SrbLength = srbSize;
        srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
        srbEx->RequestPriority = IoGetIoPriorityHint(Irp);
        srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);
        srbEx->NumSrbExData = 1;

        // Set timeout value.
        srbEx->TimeOutValue = fdoExtension->TimeOutValue;

        //
        // Set up address fields
        //

        storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
        storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
        storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

        //
        // Set up SCSI SRB extended data fields
        //

        srbEx->SrbExDataOffset[0] = sizeof(STORAGE_REQUEST_BLOCK) +
            sizeof(STOR_ADDR_BTL8);
        if ((srbEx->SrbExDataOffset[0] + sizeof(SRBEX_DATA_SCSI_CDB16)) <= srbEx->SrbLength) {
            srbExDataCdb16 = (PSRBEX_DATA_SCSI_CDB16)((PUCHAR)srbEx + srbEx->SrbExDataOffset[0]);
            srbExDataCdb16->Type = SrbExDataTypeScsiCdb16;
            srbExDataCdb16->Length = SRBEX_DATA_SCSI_CDB16_LENGTH;
            srbExDataCdb16->CdbLength = 6;

            cdb = (PCDB)srbExDataCdb16->Cdb;
        } else {
            // Should not happen
            NT_ASSERT(FALSE);

            FREE_POOL(srb);
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlReassignBlocks: Insufficient extended SRB size.\n"));
            return STATUS_INTERNAL_ERROR;
        }

    } else {
        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->CdbLength = 6;

        //
        // Set timeout value.
        //

        srb->TimeOutValue = fdoExtension->TimeOutValue;

        cdb = (PCDB)srb->Cdb;
    }

    cdb->CDB6GENERIC.OperationCode = SCSIOP_REASSIGN_BLOCKS;
    cdb->CDB6GENERIC.CommandUniqueBits =  1; // LONGLBA

    status = ClassSendSrbSynchronous(DeviceObject,
                                     srb,
                                     badBlocks,
                                     bufferSize,
                                     TRUE);

    FREE_POOL(srb);
    return status;
}

NTSTATUS
DiskIoctlIsWritable(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_IS_WRITABLE. This function
    returns whether the disk is writable. If the device is not
    writable then STATUS_MEDIA_WRITE_PROTECTED will be returned.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;

    PMODE_PARAMETER_HEADER modeData;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb = NULL;
    ULONG modeLength;
    ULONG retries = 4;
    ULONG srbSize;
    PSTORAGE_REQUEST_BLOCK srbEx;
    PSTOR_ADDR_BTL8 storAddrBtl8;
    PSRBEX_DATA_SCSI_CDB16 srbExDataCdb16;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlIsWritable: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
    } else {
        srbSize = SCSI_REQUEST_BLOCK_SIZE;
    }
    srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                srbSize,
                                DISK_TAG_SRB);

    if (srb == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlIsWritable: Unable to allocate memory.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srb, srbSize);

    //
    // Allocate memory for a mode header and then some
    // for port drivers that need to convert to MODE10
    // or always return the MODE_PARAMETER_BLOCK (even
    // when memory was not allocated for this purpose)
    //

    modeLength = MODE_DATA_SIZE;
    modeData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                     modeLength,
                                     DISK_TAG_MODE_DATA);

    if (modeData == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlIsWritable: Unable to allocate memory.\n"));
        FREE_POOL(srb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeData, modeLength);

    //
    // Build the MODE SENSE CDB
    //

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx = (PSTORAGE_REQUEST_BLOCK)srb;

        //
        // Set up STORAGE_REQUEST_BLOCK fields
        //

        srbEx->Length = FIELD_OFFSET(STORAGE_REQUEST_BLOCK, Signature);
        srbEx->Function = SRB_FUNCTION_STORAGE_REQUEST_BLOCK;
        srbEx->Signature = SRB_SIGNATURE;
        srbEx->Version = STORAGE_REQUEST_BLOCK_VERSION_1;
        srbEx->SrbLength = srbSize;
        srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
        srbEx->RequestPriority = IoGetIoPriorityHint(Irp);
        srbEx->AddressOffset = sizeof(STORAGE_REQUEST_BLOCK);
        srbEx->NumSrbExData = 1;

        // Set timeout value.
        srbEx->TimeOutValue = fdoExtension->TimeOutValue;

        //
        // Set up address fields
        //

        storAddrBtl8 = (PSTOR_ADDR_BTL8) ((PUCHAR)srbEx + srbEx->AddressOffset);
        storAddrBtl8->Type = STOR_ADDRESS_TYPE_BTL8;
        storAddrBtl8->AddressLength = STOR_ADDR_BTL8_ADDRESS_LENGTH;

        //
        // Set up SCSI SRB extended data fields
        //

        srbEx->SrbExDataOffset[0] = sizeof(STORAGE_REQUEST_BLOCK) +
            sizeof(STOR_ADDR_BTL8);
        if ((srbEx->SrbExDataOffset[0] + sizeof(SRBEX_DATA_SCSI_CDB16)) <= srbEx->SrbLength) {
            srbExDataCdb16 = (PSRBEX_DATA_SCSI_CDB16)((PUCHAR)srbEx + srbEx->SrbExDataOffset[0]);
            srbExDataCdb16->Type = SrbExDataTypeScsiCdb16;
            srbExDataCdb16->Length = SRBEX_DATA_SCSI_CDB16_LENGTH;
            srbExDataCdb16->CdbLength = 6;

            cdb = (PCDB)srbExDataCdb16->Cdb;
        } else {
            // Should not happen
            NT_ASSERT(FALSE);

            FREE_POOL(srb);
            FREE_POOL(modeData);
            return STATUS_INTERNAL_ERROR;
        }

    } else {
        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->CdbLength = 6;

        //
        // Set timeout value.
        //

        srb->TimeOutValue = fdoExtension->TimeOutValue;

        cdb = (PCDB)srb->Cdb;
    }

    //
    // Page code of 0x3F will return all pages.
    // This command could fail if the data to be returned is
    // more than 256 bytes. In which case, we should get only
    // the caching page since we only need the block descriptor.
    // DiskFdoProcessError will change the page code to
    // MODE_PAGE_CACHING if there is an error.
    //

    cdb->MODE_SENSE.OperationCode    = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode         = MODE_SENSE_RETURN_ALL;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)modeLength;

    while (retries != 0) {

        status = ClassSendSrbSynchronous(DeviceObject,
                                         srb,
                                         modeData,
                                         modeLength,
                                         FALSE);

        if (status != STATUS_VERIFY_REQUIRED) {
            if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {
                status = STATUS_SUCCESS;
            }
            break;
        }
        retries--;
    }

    if (NT_SUCCESS(status)) {

        if (TEST_FLAG(modeData->DeviceSpecificParameter, MODE_DSP_WRITE_PROTECT)) {
            status = STATUS_MEDIA_WRITE_PROTECTED;
        }
    }

    FREE_POOL(srb);
    FREE_POOL(modeData);
    return status;
}

NTSTATUS
DiskIoctlSetVerify(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_INTERNAL_SET_VERIFY.
    This is an internal function used to set the DO_VERIFY_VOLUME
    device object flag. Only a kernel mode component can send this
    IOCTL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlSetVerify: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    //
    // If the caller is kernel mode, set the verify bit.
    //

    if (Irp->RequestorMode == KernelMode) {

        SET_FLAG(DeviceObject->Flags, DO_VERIFY_VOLUME);
        status = STATUS_SUCCESS;

    }
    return status;
}

NTSTATUS
DiskIoctlClearVerify(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_INTERNAL_CLEAR_VERIFY.
    This is an internal function used to clear the DO_VERIFY_VOLUME
    device object flag. Only a kernel mode component can send this
    IOCTL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlClearVerify: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    //
    // If the caller is kernel mode, set the verify bit.
    //

    if (Irp->RequestorMode == KernelMode) {

        CLEAR_FLAG(DeviceObject->Flags, DO_VERIFY_VOLUME);
        status = STATUS_SUCCESS;

    }
    return status;
}

NTSTATUS
DiskIoctlUpdateDriveSize(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_DISK_UPDATE_DRIVE_SIZE.
    This function is used to inform the disk driver to update
    the device geometry information cached in the device extension
    This is normally initiated from the drivers layers above disk
    driver.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    TARGET_DEVICE_CUSTOM_NOTIFICATION Notification = {0};
    NTSTATUS status;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlUpdateDriveSize: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlUpdateDriveSize: Output buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    status = DiskReadDriveCapacity(DeviceObject);

    //
    // Note whether the drive is ready.
    //

    diskData->ReadyStatus = status;

    if (NT_SUCCESS(status)) {

        //
        // Copy drive geometry information from the device extension.
        //

        RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                      &(fdoExtension->DiskGeometry),
                      sizeof(DISK_GEOMETRY));

        if (((PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer)->BytesPerSector == 0) {
            ((PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer)->BytesPerSector = 512;
        }
        Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        status = STATUS_SUCCESS;

        //
        // Notify everyone that the disk layout may have changed
        //

        Notification.Event   = GUID_IO_DISK_LAYOUT_CHANGE;
        Notification.Version = 1;
        Notification.Size    = (USHORT)FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer);
        Notification.FileObject = NULL;
        Notification.NameBufferOffset = -1;

        IoReportTargetDeviceChangeAsynchronous(fdoExtension->LowerPdo,
                                               &Notification,
                                               NULL,
                                               NULL);
    }
    return status;
}

NTSTATUS
DiskIoctlGetVolumeDiskExtents(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS
    and IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS_ADMIN. This function
    returns the physical location of a volume.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlGetVolumeDiskExtents: DeviceObject %p Irp %p\n", DeviceObject, Irp));


    if (TEST_FLAG(DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA)) {

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_DISK_EXTENTS)) {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlGetVolumeDiskExtents: Output buffer too small.\n"));
            return STATUS_BUFFER_TOO_SMALL;
        }

        status = DiskReadDriveCapacity(commonExtension->PartitionZeroExtension->DeviceObject);

        //
        // Note whether the drive is ready.
        //

        diskData->ReadyStatus = status;

        if (NT_SUCCESS(status)) {

            PVOLUME_DISK_EXTENTS pVolExt = (PVOLUME_DISK_EXTENTS)Irp->AssociatedIrp.SystemBuffer;

            pVolExt->NumberOfDiskExtents = 1;
            pVolExt->Extents[0].DiskNumber     = commonExtension->PartitionZeroExtension->DeviceNumber;
            pVolExt->Extents[0].StartingOffset = commonExtension->StartingOffset;
            pVolExt->Extents[0].ExtentLength   = commonExtension->PartitionLength;

            Irp->IoStatus.Information = sizeof(VOLUME_DISK_EXTENTS);
        }
    }

    return status;
}

NTSTATUS
DiskIoctlSmartGetVersion(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services SMART_GET_VERSION. It returns the
    SMART version information.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status;

    PGETVERSIONINPARAMS versionParams;
    PSRB_IO_CONTROL srbControl;
    IO_STATUS_BLOCK ioStatus = { 0 };
    PUCHAR buffer;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlSmartGetVersion: DeviceObject %p Irp %p\n", DeviceObject, Irp));


    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GETVERSIONINPARAMS)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartGetVersion: Output buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    srbControl = ExAllocatePoolWithTag(NonPagedPoolNx,
                                       sizeof(SRB_IO_CONTROL) +
                                       sizeof(GETVERSIONINPARAMS),
                                       DISK_TAG_SMART);

    if (srbControl == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartGetVersion: Unable to allocate memory.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(srbControl, sizeof(SRB_IO_CONTROL) + sizeof(GETVERSIONINPARAMS));

    //
    // fill in srbControl fields
    //

    srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
    RtlMoveMemory (srbControl->Signature, "SCSIDISK", 8);
    srbControl->Timeout = fdoExtension->TimeOutValue;
    srbControl->Length = sizeof(GETVERSIONINPARAMS);
    srbControl->ControlCode = IOCTL_SCSI_MINIPORT_SMART_VERSION;

    //
    // Point to the 'buffer' portion of the SRB_CONTROL
    //

    buffer = (PUCHAR)srbControl + srbControl->HeaderLength;

    //
    // Ensure correct target is set in the cmd parameters.
    //

    versionParams = (PGETVERSIONINPARAMS)buffer;
    versionParams->bIDEDeviceMap = diskData->ScsiAddress.TargetId;

    ClassSendDeviceIoControlSynchronous(
                                    IOCTL_SCSI_MINIPORT,
                                    commonExtension->LowerDeviceObject,
                                    srbControl,
                                    sizeof(SRB_IO_CONTROL) + sizeof(GETVERSIONINPARAMS),
                                    sizeof(SRB_IO_CONTROL) + sizeof(GETVERSIONINPARAMS),
                                    FALSE,
                                    &ioStatus);

    status = ioStatus.Status;

    //
    // If successful, copy the data received into the output buffer.
    // This should only fail in the event that the IDE driver is older
    // than this driver.
    //

    if (NT_SUCCESS(status)) {

        buffer = (PUCHAR)srbControl + srbControl->HeaderLength;

        RtlMoveMemory (Irp->AssociatedIrp.SystemBuffer, buffer, sizeof(GETVERSIONINPARAMS));
        Irp->IoStatus.Information = sizeof(GETVERSIONINPARAMS);
    }

    FREE_POOL(srbControl);

    return status;
}

NTSTATUS
DiskIoctlSmartReceiveDriveData(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services SMART_RCV_DRIVE_DATA. This function
    allows the caller to read SMART information from the device.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    PSENDCMDINPARAMS cmdInParameters = ((PSENDCMDINPARAMS)Irp->AssociatedIrp.SystemBuffer);
    PSRB_IO_CONTROL srbControl;
    IO_STATUS_BLOCK ioStatus = { 0 };
    ULONG controlCode = 0;
    PUCHAR buffer;
    PIRP irp2;
    KEVENT event;
    ULONG length = 0;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlSmartReceiveDriveData: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < (sizeof(SENDCMDINPARAMS) - 1)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartReceiveDriveData: Input buffer length invalid.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < (sizeof(SENDCMDOUTPARAMS) + 512 - 1)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartReceiveDriveData: Output buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Create notification event object to be used to signal the
    // request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // use controlCode as a sort of 'STATUS_SUCCESS' to see if it's
    // a valid request type
    //

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

            case SMART_READ_LOG: {

                if (diskData->FailurePredictionCapability != FailurePredictionSmart) {
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartReceiveDriveData: SMART failure prediction not supported.\n"));
                    return STATUS_INVALID_DEVICE_REQUEST;
                }

                //
                // Calculate additional length based on number of sectors to be read.
                // Then verify the output buffer is large enough.
                //

                length = cmdInParameters->irDriveRegs.bSectorCountReg * SMART_LOG_SECTOR_SIZE;

                //
                // Ensure at least 1 sector is going to be read
                //
                if (length == 0) {
                    return STATUS_INVALID_PARAMETER;
                }

                length += max(sizeof(SENDCMDOUTPARAMS), sizeof(SENDCMDINPARAMS));

                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < length - 1) {
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartReceiveDriveData: Output buffer too small for SMART_READ_LOG.\n"));
                    return STATUS_BUFFER_TOO_SMALL;
                }

                controlCode = IOCTL_SCSI_MINIPORT_READ_SMART_LOG;
                break;
            }
        }
    }

    if (controlCode == 0) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartReceiveDriveData: Invalid request.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    srbControl = ExAllocatePoolWithTag(NonPagedPoolNx,
                                       sizeof(SRB_IO_CONTROL) + length,
                                       DISK_TAG_SMART);

    if (srbControl == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartReceiveDriveData: Unable to allocate memory.\n"));
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // fill in srbControl fields
    //

    srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
    RtlMoveMemory (srbControl->Signature, "SCSIDISK", 8);
    srbControl->Timeout = fdoExtension->TimeOutValue;
    srbControl->Length = length;
    srbControl->ControlCode = controlCode;

    //
    // Point to the 'buffer' portion of the SRB_CONTROL
    //

    buffer = (PUCHAR)srbControl + srbControl->HeaderLength;

    //
    // Ensure correct target is set in the cmd parameters.
    //

    cmdInParameters->bDriveNumber = diskData->ScsiAddress.TargetId;

    //
    // Copy the IOCTL parameters to the srb control buffer area.
    //

    RtlMoveMemory(buffer,
                  Irp->AssociatedIrp.SystemBuffer,
                  sizeof(SENDCMDINPARAMS) - 1);

    irp2 = IoBuildDeviceIoControlRequest(IOCTL_SCSI_MINIPORT,
                                        commonExtension->LowerDeviceObject,
                                        srbControl,
                                        sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1,
                                        srbControl,
                                        sizeof(SRB_IO_CONTROL) + length,
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp2 == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartReceiveDriveData: Unable to allocate IRP.\n"));
        FREE_POOL(srbControl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call the port driver with the request and wait for it to complete.
    //

    status = IoCallDriver(commonExtension->LowerDeviceObject, irp2);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // Copy the data received into the output buffer. Since the status buffer
    // contains error information also, always perform this copy. IO will
    // either pass this back to the app, or zero it, in case of error.
    //

    buffer = (PUCHAR)srbControl + srbControl->HeaderLength;

    if (NT_SUCCESS(status)) {

        RtlMoveMemory ( Irp->AssociatedIrp.SystemBuffer, buffer, length - 1);
        Irp->IoStatus.Information = length - 1;

    } else {

        RtlMoveMemory ( Irp->AssociatedIrp.SystemBuffer, buffer, (sizeof(SENDCMDOUTPARAMS) - 1));
        Irp->IoStatus.Information = sizeof(SENDCMDOUTPARAMS) - 1;

    }

    FREE_POOL(srbControl);

    return status;
}

NTSTATUS
DiskIoctlSmartSendDriveCommand(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine services SMART_SEND_DRIVE_COMMAND. This function
    allows the caller to send SMART commands to the device.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDISK_DATA diskData = (PDISK_DATA)(commonExtension->DriverData);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    PSENDCMDINPARAMS cmdInParameters = ((PSENDCMDINPARAMS)Irp->AssociatedIrp.SystemBuffer);
    PSRB_IO_CONTROL srbControl;
    IO_STATUS_BLOCK ioStatus = { 0 };
    ULONG controlCode = 0;
    PUCHAR buffer;
    PIRP irp2;
    KEVENT event;
    ULONG length = 0;

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();
    CHECK_IRQL();

    //
    // Validate the request.
    //

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DiskIoctlSmartSendDriveCommand: DeviceObject %p Irp %p\n", DeviceObject, Irp));

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < (sizeof(SENDCMDINPARAMS) - 1)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartSendDriveCommand: Input buffer size invalid.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < (sizeof(SENDCMDOUTPARAMS) - 1)) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartSendDriveCommand: Output buffer too small.\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Create notification event object to be used to signal the
    // request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    if (cmdInParameters->irDriveRegs.bCommandReg == SMART_CMD) {

        switch (cmdInParameters->irDriveRegs.bFeaturesReg) {

            case SMART_WRITE_LOG: {

                if (diskData->FailurePredictionCapability != FailurePredictionSmart) {

                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartSendDriveCommand: SMART failure prediction not supported.\n"));
                    return STATUS_INVALID_DEVICE_REQUEST;
                }

                //
                // Calculate additional length based on number of sectors to be written.
                // Then verify the input buffer is large enough.
                //

                length = cmdInParameters->irDriveRegs.bSectorCountReg * SMART_LOG_SECTOR_SIZE;

                //
                // Ensure at least 1 sector is going to be written
                //

                if (length == 0) {
                    return STATUS_INVALID_PARAMETER;
                }

                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                        (sizeof(SENDCMDINPARAMS) - 1) + length) {

                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartSendDriveCommand: Input buffer too small for SMART_WRITE_LOG.\n"));
                    return STATUS_BUFFER_TOO_SMALL;
                }

                controlCode = IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG;
                break;
            }

            case ENABLE_SMART:
                controlCode = IOCTL_SCSI_MINIPORT_ENABLE_SMART;
                break;

            case DISABLE_SMART:
                controlCode = IOCTL_SCSI_MINIPORT_DISABLE_SMART;
                break;

            case RETURN_SMART_STATUS:

                //
                // Ensure bBuffer is at least 2 bytes (to hold the values of
                // cylinderLow and cylinderHigh).
                //

                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                    (sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(IDEREGS))) {

                    return STATUS_BUFFER_TOO_SMALL;
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
                //
                // Validate that this is an ok self test command
                //
                if (DiskIsValidSmartSelfTest(cmdInParameters->irDriveRegs.bSectorNumberReg)) {

                    controlCode = IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS;
                }
                break;

            case ENABLE_DISABLE_AUTO_OFFLINE:
                controlCode = IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTO_OFFLINE;
                break;
        }
    }

    if (controlCode == 0) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartSendDriveCommand: Invalid request.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    length += max(sizeof(SENDCMDOUTPARAMS), sizeof(SENDCMDINPARAMS));
    srbControl = ExAllocatePoolWithTag(NonPagedPoolNx,
                                       sizeof(SRB_IO_CONTROL) + length,
                                       DISK_TAG_SMART);

    if (srbControl == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartSendDriveCommand: Unable to allocate memory.\n"));
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // fill in srbControl fields
    //

    srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
    RtlMoveMemory (srbControl->Signature, "SCSIDISK", 8);
    srbControl->Timeout = fdoExtension->TimeOutValue;
    srbControl->Length = length;
    srbControl->ControlCode = controlCode;

    //
    // Point to the 'buffer' portion of the SRB_CONTROL
    //

    buffer = (PUCHAR)srbControl + srbControl->HeaderLength;

    //
    // Ensure correct target is set in the cmd parameters.
    //

    cmdInParameters->bDriveNumber = diskData->ScsiAddress.TargetId;

    //
    // Copy the IOCTL parameters to the srb control buffer area.
    //

    if (cmdInParameters->irDriveRegs.bFeaturesReg == SMART_WRITE_LOG) {
        RtlMoveMemory(buffer,
                     Irp->AssociatedIrp.SystemBuffer,
                     sizeof(SENDCMDINPARAMS) - 1 +
                        cmdInParameters->irDriveRegs.bSectorCountReg * SMART_LOG_SECTOR_SIZE);
    } else {
        RtlMoveMemory(buffer, Irp->AssociatedIrp.SystemBuffer, sizeof(SENDCMDINPARAMS) - 1);
    }

    irp2 = IoBuildDeviceIoControlRequest(IOCTL_SCSI_MINIPORT,
                                        commonExtension->LowerDeviceObject,
                                        srbControl,
                                        sizeof(SRB_IO_CONTROL) + length,
                                        srbControl,
                                        sizeof(SRB_IO_CONTROL) + length,
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp2 == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DiskIoctlSmartSendDriveCommand: Unable to allocate IRP.\n"));
        FREE_POOL(srbControl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call the port driver with the request and wait for it to complete.
    //

    status = IoCallDriver(commonExtension->LowerDeviceObject, irp2);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // Copy the data received into the output buffer. Since the status buffer
    // contains error information also, always perform this copy. IO will
    // either pass this back to the app, or zero it, in case of error.
    //

    buffer = (PUCHAR)srbControl + srbControl->HeaderLength;

    //
    // Update the return buffer size based on the sub-command.
    //

    if (cmdInParameters->irDriveRegs.bFeaturesReg == RETURN_SMART_STATUS) {
        length = sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(IDEREGS);
    } else {
        length = sizeof(SENDCMDOUTPARAMS) - 1;
    }

    RtlMoveMemory ( Irp->AssociatedIrp.SystemBuffer, buffer, length);
    Irp->IoStatus.Information = length;

    FREE_POOL(srbControl);

    return status;
}


VOID
DiskEtwEnableCallback (
    _In_ LPCGUID                      SourceId,
    _In_ ULONG                        IsEnabled,
    _In_ UCHAR                        Level,
    _In_ ULONGLONG                    MatchAnyKeyword,
    _In_ ULONGLONG                    MatchAllKeyword,
    _In_opt_ PEVENT_FILTER_DESCRIPTOR FilterData,
    _In_opt_ PVOID                    CallbackContext
    )
/*+++

Routine Description:

    This routine is the enable callback routine for ETW. It gets called when
    tracing is enabled or disabled for our provider.

Arguments:

    As per the ETW callback.

Return Value:

    None.
--*/

{
    //
    // Initialize locals.
    //
    UNREFERENCED_PARAMETER(SourceId);
    UNREFERENCED_PARAMETER(Level);
    UNREFERENCED_PARAMETER(MatchAnyKeyword);
    UNREFERENCED_PARAMETER(MatchAllKeyword);
    UNREFERENCED_PARAMETER(FilterData);
    UNREFERENCED_PARAMETER(CallbackContext);

    //
    // Set the ETW tracing enable state.
    //
    DiskETWEnabled = IsEnabled ? TRUE : FALSE;

    return;
}


