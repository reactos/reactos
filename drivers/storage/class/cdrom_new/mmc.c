/*--

Copyright (C) Microsoft Corporation, 2000

Module Name:

    mmc.c

Abstract:

    This file is used to extend cdrom.sys to detect and use mmc-compatible
    drives' capabilities more wisely.

Environment:

    kernel mode only

Notes:

    SCSI Tape, CDRom and Disk class drivers share common routines
    that can be found in the CLASS directory (..\ntos\dd\class).

Revision History:

--*/

#include "cdrom.h"

NTSTATUS
NTAPI
CdRomGetConfiguration(
    IN PDEVICE_OBJECT Fdo,
    OUT PGET_CONFIGURATION_HEADER *Buffer,
    OUT PULONG BytesReturned,
    IN FEATURE_NUMBER StartingFeature,
    IN ULONG RequestedType
    );

VOID
NTAPI
CdRompPrintAllFeaturePages(
    IN PGET_CONFIGURATION_HEADER Buffer,
    IN ULONG Usable
    );

NTSTATUS
NTAPI
CdRomUpdateMmcDriveCapabilitiesCompletion(
    IN PDEVICE_OBJECT Unused,
    IN PIRP Irp,
    IN PDEVICE_OBJECT Fdo
    );

VOID
NTAPI
CdRomPrepareUpdateCapabilitiesIrp(
    PDEVICE_OBJECT Fdo
    );

/*++

    NOT DOCUMENTED YET - may be called at up to DISPATCH_LEVEL
    if memory is non-paged
    PRESUMES ALL DATA IS ACCESSIBLE based on FeatureBuffer
    
--*/
VOID
NTAPI
CdRomFindProfileInProfiles(
    IN PFEATURE_DATA_PROFILE_LIST ProfileHeader,
    IN FEATURE_PROFILE_TYPE ProfileToFind,
    OUT PBOOLEAN Found
    )
{
    PFEATURE_DATA_PROFILE_LIST_EX profile;
    ULONG numberOfProfiles;
    ULONG i;
    
    ASSERT((ProfileHeader->Header.AdditionalLength % 4) == 0);

    *Found = FALSE;

    numberOfProfiles = ProfileHeader->Header.AdditionalLength / 4;
    profile = ProfileHeader->Profiles; // zero-sized array
    
    for (i = 0; i < numberOfProfiles; i++) {

        FEATURE_PROFILE_TYPE currentProfile;

        currentProfile =
            (profile->ProfileNumber[0] << 8) |
            (profile->ProfileNumber[1] & 0xff);
        
        if (currentProfile == ProfileToFind) {

            *Found = TRUE;

        }
        
        profile++;
    }
    return;

}


/*++

    NOT DOCUMENTED YET - may be called at up to DISPATCH_LEVEL
    if memory is non-paged
    
--*/
PVOID
NTAPI
CdRomFindFeaturePage(
    IN PGET_CONFIGURATION_HEADER FeatureBuffer,
    IN ULONG Length,
    IN FEATURE_NUMBER Feature
    )
{
    PUCHAR buffer;
    PUCHAR limit;
    
    if (Length < sizeof(GET_CONFIGURATION_HEADER) + sizeof(FEATURE_HEADER)) {
        return NULL;
    }

    //
    // set limit to point to first illegal address
    //

    limit  = (PUCHAR)FeatureBuffer;
    limit += Length;

    //
    // set buffer to point to first page
    //

    buffer = FeatureBuffer->Data;

    //
    // loop through each page until we find the requested one, or
    // until it's not safe to access the entire feature header
    // (if equal, have exactly enough for the feature header)
    //
    while (buffer + sizeof(FEATURE_HEADER) <= limit) {

        PFEATURE_HEADER header = (PFEATURE_HEADER)buffer;
        FEATURE_NUMBER thisFeature;

        thisFeature  =
            (header->FeatureCode[0] << 8) |
            (header->FeatureCode[1]);

        if (thisFeature == Feature) {

            PUCHAR temp;

            //
            // if don't have enough memory to safely access all the feature
            // information, return NULL
            //
            temp = buffer;
            temp += sizeof(FEATURE_HEADER);
            temp += header->AdditionalLength;
            
            if (temp > limit) {

                //
                // this means the transfer was cut-off, an insufficiently
                // small buffer was given, or other arbitrary error.  since
                // it's not safe to view the amount of data (even though
                // the header is safe) in this feature, pretend it wasn't
                // transferred at all...
                //

                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "Feature %x exists, but not safe to access all its "
                           "data.  returning NULL\n", Feature));
                return NULL;
            } else {
                return buffer;
            }
        }

        if (header->AdditionalLength % 4) {
            ASSERT(!"Feature page AdditionalLength field must be integral multiple of 4!\n");
            return NULL;
        }

        buffer += sizeof(FEATURE_HEADER);
        buffer += header->AdditionalLength;
    
    }
    return NULL;
}

/*++

Private so we can later expose to someone wanting to use a preallocated buffer

--*/
NTSTATUS
NTAPI
CdRompGetConfiguration(
    IN PDEVICE_OBJECT Fdo,
    IN PGET_CONFIGURATION_HEADER Buffer,
    IN ULONG BufferSize,
    OUT PULONG ValidBytes,
    IN FEATURE_NUMBER StartingFeature,
    IN ULONG RequestedType
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCDROM_DATA cdData;
    SCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    ULONG_PTR returned;
    NTSTATUS status;

    PAGED_CODE();
    ASSERT(Buffer);
    ASSERT(ValidBytes);

    *ValidBytes = 0;
    returned = 0;

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));
    RtlZeroMemory(Buffer, BufferSize);

    fdoExtension = Fdo->DeviceExtension;
    cdData = (PCDROM_DATA)(fdoExtension->CommonExtension.DriverData);

    if (TEST_FLAG(cdData->HackFlags, CDROM_HACK_BAD_GET_CONFIG_SUPPORT)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    srb.TimeOutValue = CDROM_GET_CONFIGURATION_TIMEOUT;
    srb.CdbLength = 10;

    cdb = (PCDB)srb.Cdb;
    cdb->GET_CONFIGURATION.OperationCode = SCSIOP_GET_CONFIGURATION;
    cdb->GET_CONFIGURATION.RequestType = (UCHAR)RequestedType;
    cdb->GET_CONFIGURATION.StartingFeature[0] = (UCHAR)(StartingFeature >> 8);
    cdb->GET_CONFIGURATION.StartingFeature[1] = (UCHAR)(StartingFeature & 0xff);
    cdb->GET_CONFIGURATION.AllocationLength[0] = (UCHAR)(BufferSize >> 8);
    cdb->GET_CONFIGURATION.AllocationLength[1] = (UCHAR)(BufferSize & 0xff);

    status = ClassSendSrbSynchronous(Fdo,  &srb,  Buffer,
                                     BufferSize, FALSE);
    returned = srb.DataTransferLength;

    KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
               "CdromGetConfiguration: Status was %x\n", status));

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW) {

        //
        // if returned more than can be stored in a ULONG, return false
        //

        if (returned > (ULONG)(-1)) {
            return STATUS_UNSUCCESSFUL;
        }
        ASSERT(returned <= BufferSize);
        *ValidBytes = (ULONG)returned;
        return STATUS_SUCCESS;

    } else {

        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: failed %x\n", status));
        return status;

    }
    ASSERT(FALSE);
    return STATUS_UNSUCCESSFUL;
}

/*++

    Allocates buffer with configuration info, returns STATUS_SUCCESS
    or an error if one occurred

    NOTE: does not handle case where more than 65000 bytes are returned,
          which requires multiple calls with different starting feature
          numbers.

--*/
NTSTATUS
NTAPI
CdRomGetConfiguration(
    IN PDEVICE_OBJECT Fdo,
    OUT PGET_CONFIGURATION_HEADER *Buffer,
    OUT PULONG BytesReturned,
    IN FEATURE_NUMBER StartingFeature,
    IN ULONG RequestedType
    )
{
    //PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    GET_CONFIGURATION_HEADER header;  // eight bytes, not a lot
    PGET_CONFIGURATION_HEADER buffer;
    ULONG returned;
    ULONG size;
    ULONG i;
    NTSTATUS status;

    PAGED_CODE();


    //fdoExtension = Fdo->DeviceExtension;
    *Buffer = NULL;
    *BytesReturned = 0;

    buffer = NULL;
    returned = 0;

    //
    // send the first request down to just get the header
    //

    status = CdRompGetConfiguration(Fdo, &header, sizeof(header),
                                    &returned, StartingFeature, RequestedType);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // now try again, using information returned to allocate
    // just enough memory
    //

    size = header.DataLength[0] << 24 |
           header.DataLength[1] << 16 |
           header.DataLength[2] <<  8 |
           header.DataLength[3] <<  0 ;


    for (i = 0; i < 4; i++) {

        //
        // the datalength field is the size *following*
        // itself, so adjust accordingly
        //

        size += 4*sizeof(UCHAR);

        //
        // make sure the size is reasonable
        //

        if (size <= sizeof(FEATURE_HEADER)) {
            KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                       "CdromGetConfiguration: drive reports only %x bytes?\n",
                       size));
            return STATUS_UNSUCCESSFUL;
        }

        //
        // allocate the memory
        //

        buffer = (PGET_CONFIGURATION_HEADER)
                 ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                       size,
                                       CDROM_TAG_FEATURE);

        if (buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // send the first request down to just get the header
        //

        status = CdRompGetConfiguration(Fdo, buffer, size, &returned,
                                        StartingFeature, RequestedType);

        if (!NT_SUCCESS(status)) {
            ExFreePool(buffer);
            return status;
        }

        if (returned > size) {
            ExFreePool(buffer);
            return STATUS_INTERNAL_ERROR;
        }

        returned = buffer->DataLength[0] << 24 |
                   buffer->DataLength[1] << 16 |
                   buffer->DataLength[2] <<  8 |
                   buffer->DataLength[3] <<  0 ;
        returned += 4*sizeof(UCHAR);

        if (returned <= size) {
            *Buffer = buffer;
            *BytesReturned = size;  // amount of 'safe' memory
            return STATUS_SUCCESS;
        }

        //
        // else retry using the new size....
        //

        size = returned;
        ExFreePool(buffer);
        buffer = NULL;
        
    }

    //
    // it failed after a number of attempts, so just fail.
    //

    KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
               "CdRomGetConfiguration: Failed %d attempts to get all feature "
               "information\n", i));
    return STATUS_IO_DEVICE_ERROR;
}

VOID
NTAPI
CdRomIsDeviceMmcDevice(
    IN PDEVICE_OBJECT Fdo,
    OUT PBOOLEAN IsMmc
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cdData = commonExtension->DriverData;
    GET_CONFIGURATION_HEADER localHeader;
    NTSTATUS status;
    ULONG usable;
    ULONG size;
    ULONG previouslyFailed;

    PAGED_CODE();
    ASSERT( commonExtension->IsFdo );

    *IsMmc = FALSE;

    //
    // read the registry in case the drive failed previously,
    // and a timeout is occurring.
    //

    previouslyFailed = FALSE;
    ClassGetDeviceParameter(fdoExtension,
                            CDROM_SUBKEY_NAME,
                            CDROM_NON_MMC_DRIVE_NAME,
                            &previouslyFailed
                            );

    if (previouslyFailed) {
        SET_FLAG(cdData->HackFlags, CDROM_HACK_BAD_GET_CONFIG_SUPPORT);
    }

    //
    // check for the following profiles:
    //
    // ProfileList
    //

    status = CdRompGetConfiguration(Fdo,
                                    &localHeader,
                                    sizeof(localHeader),
                                    &usable,
                                    FeatureProfileList,
                                    SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL);
    
    if (status == STATUS_INVALID_DEVICE_REQUEST ||
        status == STATUS_NO_MEDIA_IN_DEVICE     ||
        status == STATUS_IO_DEVICE_ERROR        ||
        status == STATUS_IO_TIMEOUT) {
        
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "GetConfiguration Failed (%x), device %p not mmc-compliant\n",
                   status, Fdo
                   ));
        previouslyFailed = TRUE;
        ClassSetDeviceParameter(fdoExtension,
                                CDROM_SUBKEY_NAME,
                                CDROM_NON_MMC_DRIVE_NAME,
                                previouslyFailed
                                );
        return;
    
    } else if (!NT_SUCCESS(status)) {
        
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugError,
                   "GetConfiguration Failed, status %x -- defaulting to -ROM\n",
                   status));
        return;

    } else if (usable < sizeof(GET_CONFIGURATION_HEADER)) {

        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "GetConfiguration Failed, returned only %x bytes!\n", usable));
        previouslyFailed = TRUE;
        ClassSetDeviceParameter(fdoExtension,
                                CDROM_SUBKEY_NAME,
                                CDROM_NON_MMC_DRIVE_NAME,
                                previouslyFailed
                                );
        return;

    }

    size = (localHeader.DataLength[0] << 24) |
           (localHeader.DataLength[1] << 16) |
           (localHeader.DataLength[2] <<  8) |
           (localHeader.DataLength[3] <<  0);

    if(size <= 4) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "GetConfiguration Failed, claims MMC support but doesn't "
                   "correctly return config length!\n"));
        return;
    }
    
    size += 4; // sizeof the datalength fields
    
#if DBG
    {
        PGET_CONFIGURATION_HEADER dbgBuffer;
        NTSTATUS dbgStatus;

        dbgBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                          (SIZE_T)size,
                                          CDROM_TAG_FEATURE);
        if (dbgBuffer != NULL) {
            RtlZeroMemory(dbgBuffer, size);
            
            dbgStatus = CdRompGetConfiguration(Fdo, dbgBuffer, size,
                                               &size, FeatureProfileList,
                                               SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL);
        
            if (NT_SUCCESS(dbgStatus)) {
                CdRompPrintAllFeaturePages(dbgBuffer, usable);            
            }
            ExFreePool(dbgBuffer);
        }
    }
#endif // DBG
    
    *IsMmc = TRUE;
    return;
}

VOID
NTAPI
CdRompPrintAllFeaturePages(
    IN PGET_CONFIGURATION_HEADER Buffer,
    IN ULONG Usable
    )
{
    PFEATURE_HEADER header;

////////////////////////////////////////////////////////////////////////////////
// items expected to ALWAYS be current
////////////////////////////////////////////////////////////////////////////////
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureProfileList);
    if (header != NULL) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: CurrentProfile %x "
                   "with %x bytes of data at %p\n",
                   Buffer->CurrentProfile[0] << 8 |
                   Buffer->CurrentProfile[1],
                   Usable, Buffer));
    }
    
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureCore);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "CORE Features"
                   ));
    }
    
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureMorphing);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Morphing"
                   ));
    }
    
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureMultiRead);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Multi-Read"
                   ));
    }
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureRemovableMedium);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Removable Medium"
                   ));
    }
    
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureTimeout);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Timeouts"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeaturePowerManagement);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Power Management"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureEmbeddedChanger);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Embedded Changer"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureLogicalUnitSerialNumber);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "LUN Serial Number"
                   ));
    }


    header = CdRomFindFeaturePage(Buffer, Usable, FeatureMicrocodeUpgrade);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Microcode Update"
                   ));
    }
        
////////////////////////////////////////////////////////////////////////////////
// items expected not to always be current
////////////////////////////////////////////////////////////////////////////////
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureCDAudioAnalogPlay);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Analogue CD Audio Operations"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureCdRead);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "reading from CD-ROM/R/RW"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureCdMastering);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "CD Recording (Mastering)"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureCdTrackAtOnce);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "CD Recording (Track At Once)"
                   ));
    }
    
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureDvdCSS);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD CSS"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureDvdRead);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD Structure Reads"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureDvdRecordableWrite);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD Recording (Mastering)"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureDiscControlBlocks);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "DVD Disc Control Blocks"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureFormattable);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Formatting"
                   ));
    }
    
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureRandomReadable);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Random Reads"
                   ));
    }
    
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureRandomWritable);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Random Writes"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureRestrictedOverwrite);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Restricted Overwrites."
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureWriteOnce);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Write Once Media"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureSectorErasable);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Sector Erasable Media"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureIncrementalStreamingWritable);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Incremental Streaming Writing"
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureRealTimeStreaming);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "Real-time Streaming Reads"
                   ));
    }
    
    header = CdRomFindFeaturePage(Buffer, Usable, FeatureSMART);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "S.M.A.R.T."
                   ));
    }

    header = CdRomFindFeaturePage(Buffer, Usable, FeatureDefectManagement);
    if (header) {
        KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                   "CdromGetConfiguration: %s %s\n",
                   (header->Current ?
                    "Currently supports" : "Is able to support"),
                   "defect management"
                   ));
    }
    return;
}

NTSTATUS
NTAPI
CdRomUpdateMmcDriveCapabilitiesCompletion(
    IN PDEVICE_OBJECT Unused,
    IN PIRP Irp,
    IN PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    //PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cdData = fdoExtension->CommonExtension.DriverData;
    PCDROM_MMC_EXTENSION mmcData = &(cdData->Mmc);
    PSCSI_REQUEST_BLOCK srb = &(mmcData->CapabilitiesSrb);
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    //PIRP delayedIrp;
    ULONG retryCount;
    LARGE_INTEGER delay;

    
    // completion routine should retry as neccessary.
    // when success, clear the flag to allow startio to proceed.
    // else fail original request when retries are exhausted.

    ASSERT(mmcData->CapabilitiesIrp == Irp);

    // for now, if succeeded, just print the new pages.

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {
        
        //
        // ISSUE-2000/4/20-henrygab - should we try to reallocate if size
        //                            available became larger than what we
        //                            originally allocated?  otherwise, it
        //                            is possible (not probable) that we
        //                            would miss the feature.  can check
        //                            that by finding out what the last
        //                            feature is in the current group.
        //

        BOOLEAN retry;
        ULONG retryInterval;
        
        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ClassReleaseQueue(Fdo);
        }

        retry = ClassInterpretSenseInfo(
                    Fdo,
                    srb,
                    irpStack->MajorFunction,
                    0,
                    MAXIMUM_RETRIES - ((ULONG)(ULONG_PTR)irpStack->Parameters.Others.Argument4),
                    &status,
                    &retryInterval);

        //
        // DATA_OVERRUN is not an error in this case....
        //

        if (status == STATUS_DATA_OVERRUN) {
            status = STATUS_SUCCESS;
        }

        //
        // override verify_volume based on original irp's settings
        //

        if (TEST_FLAG(irpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME) &&
            status == STATUS_VERIFY_REQUIRED) {
            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        //
        // get current retry count
        //
        retryCount = PtrToUlong(irpStack->Parameters.Others.Argument1);

        if (retry && retryCount) {

            //
            // update retry count
            //
            irpStack->Parameters.Others.Argument1 = UlongToPtr(retryCount-1);


            delay.QuadPart = retryInterval;
            delay.QuadPart *= (LONGLONG)1000 * 1000 * 10;
            
            //
            // retry the request
            //

            KdPrintEx((DPFLTR_CDROM_ID, CdromDebugError,
                       "Not using ClassRetryRequest Yet\n"));
            KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                       "Retry update capabilities %p\n", Irp));
            CdRomPrepareUpdateCapabilitiesIrp(Fdo);
            
            CdRomRetryRequest(fdoExtension, Irp, retryInterval, TRUE);

            //
            // ClassRetryRequest(Fdo, Irp, delay);
            //
            
            return STATUS_MORE_PROCESSING_REQUIRED;
        
        }

    } else {
        
        status = STATUS_SUCCESS;

    }

    Irp->IoStatus.Status = status;

    KeSetEvent(&mmcData->CapabilitiesEvent, IO_CD_ROM_INCREMENT, FALSE);


    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
NTAPI
CdRomPrepareUpdateCapabilitiesIrp(
    PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    //PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cdData = fdoExtension->CommonExtension.DriverData;
    PCDROM_MMC_EXTENSION mmcData = &(cdData->Mmc);
    PIO_STACK_LOCATION nextStack;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    ULONG bufferSize;
    PIRP irp;
    
    ASSERT(mmcData->UpdateState);
    ASSERT(ExQueryDepthSList(&(mmcData->DelayedIrps)) != 0);
    ASSERT(mmcData->CapabilitiesIrp != NULL);
    ASSERT(mmcData->CapabilitiesMdl != NULL);
    ASSERT(mmcData->CapabilitiesBuffer);
    ASSERT(mmcData->CapabilitiesBufferSize != 0);
    ASSERT(fdoExtension->SenseData);
    
    //
    // do *NOT* call IoReuseIrp(), since it would zero out our
    // current irp stack location, which we really don't want
    // to happen.  it would also set the current irp stack location
    // to one greater than currently exists (to give max irp usage),
    // but we don't want that either, since we use the top irp stack.
    //
    // IoReuseIrp(mmcData->CapabilitiesIrp, STATUS_UNSUCCESSFUL);
    //

    irp = mmcData->CapabilitiesIrp;
    srb = &(mmcData->CapabilitiesSrb);
    cdb = (PCDB)(srb->Cdb);
    bufferSize = mmcData->CapabilitiesBufferSize;

    //
    // zero stuff out
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
    RtlZeroMemory(fdoExtension->SenseData, sizeof(SENSE_DATA));
    RtlZeroMemory(mmcData->CapabilitiesBuffer, bufferSize);
    
    //
    // setup the srb
    //
    
    srb->TimeOutValue = CDROM_GET_CONFIGURATION_TIMEOUT;
    srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    srb->SenseInfoBuffer = fdoExtension->SenseData;
    srb->DataBuffer = mmcData->CapabilitiesBuffer;
    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
    srb->DataTransferLength = mmcData->CapabilitiesBufferSize;
    srb->ScsiStatus = 0;
    srb->SrbStatus = 0;
    srb->NextSrb = NULL;
    srb->OriginalRequest = irp;
    srb->SrbFlags = fdoExtension->SrbFlags;
    srb->CdbLength = 10;
    SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_IN);
    SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);

    //
    // setup the cdb
    //

    cdb->GET_CONFIGURATION.OperationCode = SCSIOP_GET_CONFIGURATION;
    cdb->GET_CONFIGURATION.RequestType = SCSI_GET_CONFIGURATION_REQUEST_TYPE_CURRENT;
    cdb->GET_CONFIGURATION.StartingFeature[0] = 0;
    cdb->GET_CONFIGURATION.StartingFeature[1] = 0;
    cdb->GET_CONFIGURATION.AllocationLength[0] = (UCHAR)(bufferSize >> 8);
    cdb->GET_CONFIGURATION.AllocationLength[1] = (UCHAR)(bufferSize & 0xff);

    //
    // setup the irp
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    nextStack->MajorFunction = IRP_MJ_SCSI;
    nextStack->Parameters.Scsi.Srb = srb;
    irp->MdlAddress = mmcData->CapabilitiesMdl;
    irp->AssociatedIrp.SystemBuffer = mmcData->CapabilitiesBuffer;
    IoSetCompletionRoutine(irp, (PIO_COMPLETION_ROUTINE)CdRomUpdateMmcDriveCapabilitiesCompletion, Fdo,
                           TRUE, TRUE, TRUE);

    return;

}

VOID
NTAPI
CdRomUpdateMmcDriveCapabilities(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID Context
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cdData = fdoExtension->CommonExtension.DriverData;
    PCDROM_MMC_EXTENSION mmcData = &(cdData->Mmc);
    PIO_STACK_LOCATION thisStack = IoGetCurrentIrpStackLocation(mmcData->CapabilitiesIrp);
    PSCSI_REQUEST_BLOCK srb = &(mmcData->CapabilitiesSrb);
    NTSTATUS status;


    ASSERT(Context == NULL);

    //
    // NOTE: a remove lock is unneccessary, since the delayed irp
    // will have said lock held for itself, preventing a remove.
    //
    CdRomPrepareUpdateCapabilitiesIrp(Fdo);
    
    ASSERT(thisStack->Parameters.Others.Argument1 == Fdo);
    ASSERT(thisStack->Parameters.Others.Argument2 == mmcData->CapabilitiesBuffer);
    ASSERT(thisStack->Parameters.Others.Argument3 == &(mmcData->CapabilitiesSrb));
    
    mmcData->WriteAllowed = FALSE; // default to read-only

    //
    // set max retries, and also allow volume verify override based on
    // original (delayed) irp
    //
    
    thisStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

    //
    // send to self... note that SL_OVERRIDE_VERIFY_VOLUME is not required,
    // as this is IRP_MJ_INTERNAL_DEVICE_CONTROL 
    //

    IoCallDriver(commonExtension->LowerDeviceObject, mmcData->CapabilitiesIrp);

    KeWaitForSingleObject(&mmcData->CapabilitiesEvent,
                          Executive, KernelMode, FALSE, NULL);
    
    status = mmcData->CapabilitiesIrp->IoStatus.Status;
    
    if (!NT_SUCCESS(status)) {

        goto FinishDriveUpdate;
    
    }

    //
    // we've updated the feature set, so update whether or not reads and writes
    // are allowed or not.
    //

    KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
               "CdRomUpdateMmc => Succeeded "
               "--------------------"
               "--------------------\n"));

    /*++
    
    NOTE: It is important to only use srb->DataTransferLength worth
          of data at this point, since the bufferSize is what is
          *available* to use, not what was *actually* used.
    
    --*/

#if DBG
    CdRompPrintAllFeaturePages(mmcData->CapabilitiesBuffer,
                               srb->DataTransferLength);
#endif // DBG

    //
    // update whether or not writes are allowed.  this is currently defined
    // as requiring TargetDefectManagement and RandomWritable features
    //
    {
        PFEATURE_HEADER defectHeader;
        PFEATURE_HEADER writableHeader;

        defectHeader   = CdRomFindFeaturePage(mmcData->CapabilitiesBuffer,
                                              srb->DataTransferLength,
                                              FeatureDefectManagement);
        writableHeader = CdRomFindFeaturePage(mmcData->CapabilitiesBuffer,
                                              srb->DataTransferLength,
                                              FeatureRandomWritable);

        if ((defectHeader != NULL)  && (writableHeader != NULL) &&
            (defectHeader->Current) && (writableHeader->Current)) {

            //
            // this should be the *ONLY* place writes are set to allowed 
            //

            KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                       "CdRomUpdateMmc => Writes *allowed*\n"));
            mmcData->WriteAllowed = TRUE;

        } else {

            if (defectHeader == NULL) {
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "CdRomUpdateMmc => No writes - %s = %s\n",
                           "defect management", "DNE"));
            } else {
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "CdRomUpdateMmc => No writes - %s = %s\n",
                           "defect management", "Not Current"));
            }
            if (writableHeader == NULL) {
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "CdRomUpdateMmc => No writes - %s = %s\n",
                           "sector writable", "DNE"));
            } else {
                KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                           "CdRomUpdateMmc => No writes - %s = %s\n",
                           "sector writable", "Not Current"));
            }
        } // end of feature checking
    } // end of check for writability

    //
    // update the cached partition table information
    //
    // NOTE: THIS WILL CURRENTLY CAUSE A DEADLOCK!
    //
    // ISSUE-2000/06/20-henrygab - partition support not implemented
    //                             IoReadPartitionTable must be done
    //                             at PASSIVE level, requiring a thread
    //                             or worker item or other such method.
    //
#if 0
    status = IoReadPartitionTable(Fdo, 1 << fdoExtension->SectorShift,
                                  TRUE, &mmcData->PartitionList);
    if (!NT_SUCCESS(status)) {

        goto FinishDriveUpdate;

    }
#endif

    status = STATUS_SUCCESS;

FinishDriveUpdate:

    CdRompFlushDelayedList(Fdo, mmcData, status, TRUE);

    return;
}

VOID
NTAPI
CdRompFlushDelayedList(
    IN PDEVICE_OBJECT Fdo,
    IN PCDROM_MMC_EXTENSION MmcData,
    IN NTSTATUS Status,
    IN BOOLEAN CalledFromWorkItem
    )
{
    PSINGLE_LIST_ENTRY list;
    PIRP irp;

    // NOTE - REF #0002
    //
    // need to set the new state first to prevent deadlocks.
    // this is only done from the workitem, to prevent any
    // edge cases where we'd "lose" the UpdateRequired
    //
    // then, must ignore the state, since it's not guaranteed to
    // be the same any longer.  the only thing left is to handle
    // all the delayed irps by flushing the queue and sending them
    // back onto the StartIo queue for the device.
    //

    if (CalledFromWorkItem) {
        
        LONG oldState;
        LONG newState;

        if (NT_SUCCESS(Status)) {
            newState = CdromMmcUpdateComplete;
        } else {
            newState = CdromMmcUpdateRequired;
        }

        oldState = InterlockedCompareExchange(&MmcData->UpdateState,
                                              newState,
                                              CdromMmcUpdateStarted);
        ASSERT(oldState == CdromMmcUpdateStarted);

    } else {

        //
        // just flushing the queue if not called from the workitem,
        // and we don't want to ever fail the queue in those cases.
        //

        ASSERT(NT_SUCCESS(Status));

    }

    list = ExInterlockedFlushSList(&MmcData->DelayedIrps);
    
    // if this assert fires, it means that we have started
    // a workitem when the previous workitem took the delayed
    // irp.  if this happens, then the logic in HACKHACK #0002
    // is either flawed or the rules set within are not being
    // followed.  this would require investigation.
    
    ASSERT(list != NULL);

    //
    // now either succeed or fail all the delayed irps, according
    // to the update status.
    //

    while (list != NULL) {

        irp = (PIRP)( ((PUCHAR)list) -
                      FIELD_OFFSET(IRP, Tail.Overlay.DriverContext[0])
                      );
        list = list->Next;
        irp->Tail.Overlay.DriverContext[0] = 0;
        irp->Tail.Overlay.DriverContext[1] = 0;
        irp->Tail.Overlay.DriverContext[2] = 0;
        irp->Tail.Overlay.DriverContext[3] = 0;

        if (NT_SUCCESS(Status)) {
            
            KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                       "CdRomUpdateMmc => Re-sending delayed irp %p\n",
                       irp));
            IoStartPacket(Fdo, irp, NULL, NULL);

        } else {
            
            KdPrintEx((DPFLTR_CDROM_ID, CdromDebugFeatures,
                       "CdRomUpdateMmc => Failing delayed irp %p with "
                       " status %x\n", irp, Status));
            irp->IoStatus.Information = 0;
            irp->IoStatus.Status = Status;
            ClassReleaseRemoveLock(Fdo, irp);
            IoCompleteRequest(irp, IO_CD_ROM_INCREMENT);

        }

    } // while (list)

    return;

}

VOID
NTAPI
CdRomDeAllocateMmcResources(
    IN PDEVICE_OBJECT Fdo
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata = commonExtension->DriverData;
    PCDROM_MMC_EXTENSION mmcData = &cddata->Mmc;
    //NTSTATUS status;

    if (mmcData->CapabilitiesWorkItem) {
        IoFreeWorkItem(mmcData->CapabilitiesWorkItem);
        mmcData->CapabilitiesWorkItem = NULL;
    }
    if (mmcData->CapabilitiesIrp) {
        IoFreeIrp(mmcData->CapabilitiesIrp);
        mmcData->CapabilitiesIrp = NULL;
    }
    if (mmcData->CapabilitiesMdl) {
        IoFreeMdl(mmcData->CapabilitiesMdl);
        mmcData->CapabilitiesMdl = NULL;
    }
    if (mmcData->CapabilitiesBuffer) {
        ExFreePool(mmcData->CapabilitiesBuffer);
        mmcData->CapabilitiesBuffer = NULL;
    }
    mmcData->CapabilitiesBuffer = 0;
    mmcData->IsMmc = FALSE;
    mmcData->WriteAllowed = FALSE;
    
    return;
}

NTSTATUS
NTAPI
CdRomAllocateMmcResources(
    IN PDEVICE_OBJECT Fdo
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata = commonExtension->DriverData;
    PCDROM_MMC_EXTENSION mmcData = &cddata->Mmc;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status;

    ASSERT(mmcData->CapabilitiesWorkItem == NULL);
    ASSERT(mmcData->CapabilitiesIrp == NULL);
    ASSERT(mmcData->CapabilitiesMdl == NULL);
    ASSERT(mmcData->CapabilitiesBuffer == NULL);
    ASSERT(mmcData->CapabilitiesBufferSize == 0);

    status = CdRomGetConfiguration(Fdo,
                                   &mmcData->CapabilitiesBuffer,
                                   &mmcData->CapabilitiesBufferSize,
                                   FeatureProfileList,
                                   SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL);
    if (!NT_SUCCESS(status)) {
        ASSERT(mmcData->CapabilitiesBuffer     == NULL);
        ASSERT(mmcData->CapabilitiesBufferSize == 0);
        return status;
    }
    ASSERT(mmcData->CapabilitiesBuffer     != NULL);
    ASSERT(mmcData->CapabilitiesBufferSize != 0);
    
    mmcData->CapabilitiesMdl = IoAllocateMdl(mmcData->CapabilitiesBuffer,
                                             mmcData->CapabilitiesBufferSize,
                                             FALSE, FALSE, NULL);
    if (mmcData->CapabilitiesMdl == NULL) {
        ExFreePool(mmcData->CapabilitiesBuffer);
        mmcData->CapabilitiesBufferSize = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

        
    mmcData->CapabilitiesIrp = IoAllocateIrp(Fdo->StackSize + 2, FALSE);
    if (mmcData->CapabilitiesIrp == NULL) {
        IoFreeMdl(mmcData->CapabilitiesMdl);
        ExFreePool(mmcData->CapabilitiesBuffer);
        mmcData->CapabilitiesBufferSize = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    mmcData->CapabilitiesWorkItem = IoAllocateWorkItem(Fdo);
    if (mmcData->CapabilitiesWorkItem == NULL) {
        IoFreeIrp(mmcData->CapabilitiesIrp);
        IoFreeMdl(mmcData->CapabilitiesMdl);
        ExFreePool(mmcData->CapabilitiesBuffer);
        mmcData->CapabilitiesBufferSize = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
            
    //
    // everything has been allocated, so now prepare it all....
    //

    MmBuildMdlForNonPagedPool(mmcData->CapabilitiesMdl);
    ExInitializeSListHead(&mmcData->DelayedIrps);
    KeInitializeSpinLock(&mmcData->DelayedLock);

    //
    // use the extra stack for internal bookkeeping
    //
    IoSetNextIrpStackLocation(mmcData->CapabilitiesIrp);
    irpStack = IoGetCurrentIrpStackLocation(mmcData->CapabilitiesIrp);
    irpStack->Parameters.Others.Argument1 = Fdo;
    irpStack->Parameters.Others.Argument2 = mmcData->CapabilitiesBuffer;
    irpStack->Parameters.Others.Argument3 = &(mmcData->CapabilitiesSrb);
    // arg 4 is the retry count

    //
    // set the completion event to FALSE for now
    //

    KeInitializeEvent(&mmcData->CapabilitiesEvent,
                      SynchronizationEvent, FALSE);
    return STATUS_SUCCESS;

}

