/*--

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    ioctl.c

Abstract:

    Include all funtions for processing IOCTLs

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "stddef.h"
#include "string.h"

#include "ntddk.h"
#include "ntddstor.h"
#include "cdrom.h"
#include "ioctl.h"
#include "scratch.h"
#include "mmc.h"


#ifdef DEBUG_USE_WPP
#include "ioctl.tmh"
#endif


#define FirstDriveLetter 'C'
#define LastDriveLetter  'Z'

#if DBG
    LPCSTR READ_DVD_STRUCTURE_FORMAT_STRINGS[DvdMaxDescriptor+1] = {
        "Physical",
        "Copyright",
        "DiskKey",
        "BCA",
        "Manufacturer",
        "Unknown"
    };
#endif // DBG

_IRQL_requires_max_(APC_LEVEL)
VOID
GetConfigurationDataConversionTypeAllToTypeOne(
    _In_    FEATURE_NUMBER       RequestedFeature,
    _In_    PSCSI_REQUEST_BLOCK  Srb,
    _Out_   size_t *             DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
GetConfigurationDataSynthesize(
    _In_reads_bytes_(InputBufferSize)    PVOID           InputBuffer,
    _In_                            ULONG           InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize)  PVOID           OutputBuffer,
    _In_                            size_t          OutputBufferSize,
    _In_                            FEATURE_NUMBER  StartingFeature,
    _In_                            ULONG           RequestType,
    _Out_                           size_t *        DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
PCDB
RequestGetScsiPassThroughCdb(
    _In_ PIRP Irp
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, DeviceIsPlayActive)
#pragma alloc_text(PAGE, RequestHandleGetDvdRegion)
#pragma alloc_text(PAGE, RequestHandleReadTOC)
#pragma alloc_text(PAGE, RequestHandleReadTocEx)
#pragma alloc_text(PAGE, RequestHandleGetConfiguration)
#pragma alloc_text(PAGE, RequestHandleGetDriveGeometry)
#pragma alloc_text(PAGE, RequestHandleDiskVerify)
#pragma alloc_text(PAGE, RequestHandleCheckVerify)
#pragma alloc_text(PAGE, RequestHandleFakePartitionInfo)
#pragma alloc_text(PAGE, RequestHandleEjectionControl)
#pragma alloc_text(PAGE, RequestHandleEnableStreaming)
#pragma alloc_text(PAGE, RequestHandleSendOpcInformation)
#pragma alloc_text(PAGE, RequestHandleGetPerformance)
#pragma alloc_text(PAGE, RequestHandleMcnSyncFakeIoctl)
#pragma alloc_text(PAGE, RequestHandleLoadEjectMedia)
#pragma alloc_text(PAGE, RequestHandleReserveRelease)
#pragma alloc_text(PAGE, RequestHandlePersistentReserve)
#pragma alloc_text(PAGE, DeviceHandleRawRead)
#pragma alloc_text(PAGE, DeviceHandlePlayAudioMsf)
#pragma alloc_text(PAGE, DeviceHandleReadQChannel)
#pragma alloc_text(PAGE, ReadQChannel)
#pragma alloc_text(PAGE, DeviceHandlePauseAudio)
#pragma alloc_text(PAGE, DeviceHandleResumeAudio)
#pragma alloc_text(PAGE, DeviceHandleSeekAudioMsf)
#pragma alloc_text(PAGE, DeviceHandleStopAudio)
#pragma alloc_text(PAGE, DeviceHandleGetSetVolume)
#pragma alloc_text(PAGE, DeviceHandleReadDvdStructure)
#pragma alloc_text(PAGE, ReadDvdStructure)
#pragma alloc_text(PAGE, DeviceHandleDvdEndSession)
#pragma alloc_text(PAGE, DeviceHandleDvdStartSessionReadKey)
#pragma alloc_text(PAGE, DvdStartSessionReadKey)
#pragma alloc_text(PAGE, DeviceHandleDvdSendKey)
#pragma alloc_text(PAGE, DvdSendKey)
#pragma alloc_text(PAGE, DeviceHandleSetReadAhead)
#pragma alloc_text(PAGE, DeviceHandleSetSpeed)
#pragma alloc_text(PAGE, RequestHandleExclusiveAccessQueryLockState)
#pragma alloc_text(PAGE, RequestHandleExclusiveAccessLockDevice)
#pragma alloc_text(PAGE, RequestHandleExclusiveAccessUnlockDevice)
#pragma alloc_text(PAGE, RequestHandleScsiPassThrough)
#pragma alloc_text(PAGE, RequestGetScsiPassThroughCdb)
#pragma alloc_text(PAGE, GetConfigurationDataConversionTypeAllToTypeOne)
#pragma alloc_text(PAGE, GetConfigurationDataSynthesize)

#endif

NTSTATUS
RequestHandleUnknownIoctl(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request
    )
/*++

Routine Description:

    All unknown IOCTLs will be forward to lower level driver.

Arguments:

    Device - device object
    Request - request to be handled

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_UNSUCCESSFUL;
    PCDROM_DEVICE_EXTENSION     deviceExtension = DeviceGetExtension(Device);
    PCDROM_REQUEST_CONTEXT      requestContext = RequestGetContext(Request);
    BOOLEAN                     syncRequired = requestContext->SyncRequired;

    ULONG                       sendOptionsFlags = 0;
    BOOLEAN                     requestSent = FALSE;

    WdfRequestFormatRequestUsingCurrentType(Request);

    if (syncRequired)
    {
        sendOptionsFlags = WDF_REQUEST_SEND_OPTION_SYNCHRONOUS;
    }
    else
    {
        WdfRequestSetCompletionRoutine(Request, RequestDummyCompletionRoutine, NULL);
    }

    status = RequestSend(deviceExtension,
                         Request,
                         deviceExtension->IoTarget,
                         sendOptionsFlags,
                         &requestSent);

    if (requestSent)
    {
        if (syncRequired)
        {
            // the request needs to be completed here.
            RequestCompletion(deviceExtension, Request, status, WdfRequestGetInformation(Request));
        }
    }
    else
    {
        // failed to send the request to IoTarget
        RequestCompletion(deviceExtension, Request, status, WdfRequestGetInformation(Request));
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
DeviceIsPlayActive(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This routine determines if the cd is currently playing music.

Arguments:

    Device - Device object.

Return Value:

    BOOLEAN - TRUE if the device is playing music.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PSUB_Q_CURRENT_POSITION currentBuffer;
    size_t                  bytesRead = 0;

    PAGED_CODE ();

    // if we don't think it is playing audio, don't bother checking.
    if (!deviceExtension->DeviceAdditionalData.PlayActive)
    {
        return FALSE;
    }

    // Allocate the required memory
    NT_ASSERT(sizeof(SUB_Q_CURRENT_POSITION) >= sizeof(CDROM_SUB_Q_DATA_FORMAT));
    currentBuffer = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                          sizeof(SUB_Q_CURRENT_POSITION),
                                          CDROM_TAG_PLAY_ACTIVE);
    if (currentBuffer == NULL)
    {
        return FALSE;
    }

    // set the options in the output buffer format
    ((PCDROM_SUB_Q_DATA_FORMAT) currentBuffer)->Format = IOCTL_CDROM_CURRENT_POSITION;
    ((PCDROM_SUB_Q_DATA_FORMAT) currentBuffer)->Track = 0;

    // Send SCSI command to read Q Channel information.
    status = ReadQChannel(deviceExtension,
                          NULL,
                          currentBuffer,
                          sizeof(CDROM_SUB_Q_DATA_FORMAT),
                          currentBuffer,
                          sizeof(SUB_Q_CURRENT_POSITION),
                          &bytesRead);

    if (!NT_SUCCESS(status))
    {
        ExFreePool(currentBuffer);
        return FALSE;
    }

    // update the playactive flag appropriately
    if (currentBuffer->Header.AudioStatus == AUDIO_STATUS_IN_PROGRESS)
    {
        deviceExtension->DeviceAdditionalData.PlayActive = TRUE;
    }
    else
    {
        deviceExtension->DeviceAdditionalData.PlayActive = FALSE;
    }

    ExFreePool(currentBuffer);

    return deviceExtension->DeviceAdditionalData.PlayActive;
}

NTSTATUS
RequestHandleGetInquiryData(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength)
/*++

Routine Description:

   Handler for IOCTL_CDROM_GET_INQUIRY_DATA

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PCDROM_DATA cdData = &(DeviceExtension->DeviceAdditionalData);

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength == 0)
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        PVOID outputBuffer = NULL;

        *DataLength = min(cdData->CachedInquiryDataByteCount,
                          RequestParameters.Parameters.DeviceIoControl.OutputBufferLength);

        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);

        if (NT_SUCCESS(status) &&
            (outputBuffer != NULL))
        {
            // Always copy as much data as possible
            RtlCopyMemory(outputBuffer,
                          cdData->CachedInquiryData,
                          *DataLength);
        }

        // and finally decide between two possible status values
        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < cdData->CachedInquiryDataByteCount)
        {
            status = STATUS_BUFFER_OVERFLOW;
        }
    }

    return status;
}


NTSTATUS
RequestHandleGetMediaTypeEx(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handler for IOCTL_STORAGE_GET_MEDIA_TYPES_EX

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCDROM_DATA         cdData = &(DeviceExtension->DeviceAdditionalData);

    PGET_MEDIA_TYPES        mediaTypes = NULL;
    PDEVICE_MEDIA_INFO      mediaInfo = NULL; //&mediaTypes->MediaInfo[0];
    ULONG                   sizeNeeded = 0;
    PZERO_POWER_ODD_INFO    zpoddInfo = DeviceExtension->ZeroPowerODDInfo;

    *DataLength = 0;

    // Must run below dispatch level.
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        NT_ASSERT(FALSE);
        return STATUS_INVALID_LEVEL;
    }

    sizeNeeded = sizeof(GET_MEDIA_TYPES);

    // IsMmc is static...
    if (cdData->Mmc.IsMmc)
    {
        sizeNeeded += sizeof(DEVICE_MEDIA_INFO) * 1; // return two media types
    }

    status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeNeeded,
                                            (PVOID*)&mediaTypes,
                                            NULL);

    if (NT_SUCCESS(status) &&
        (mediaTypes != NULL))
    {
        mediaInfo = &mediaTypes->MediaInfo[0];

        RtlZeroMemory(mediaTypes, sizeNeeded);

        // ISSUE-2000/5/11-henrygab - need to update GET_MEDIA_TYPES_EX

        mediaTypes->DeviceType = cdData->DriveDeviceType;

        mediaTypes->MediaInfoCount = 1;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType = CD_ROM;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.NumberMediaSides = 1;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics = MEDIA_READ_ONLY;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.Cylinders.QuadPart = DeviceExtension->DiskGeometry.Cylinders.QuadPart;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.TracksPerCylinder = DeviceExtension->DiskGeometry.TracksPerCylinder;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.SectorsPerTrack = DeviceExtension->DiskGeometry.SectorsPerTrack;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector = DeviceExtension->DiskGeometry.BytesPerSector;

        if (cdData->Mmc.IsMmc)
        {
            // also report a removable disk
            mediaTypes->MediaInfoCount += 1;

            mediaInfo++;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType = RemovableMedia;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.NumberMediaSides = 1;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics = MEDIA_READ_WRITE;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.Cylinders.QuadPart = DeviceExtension->DiskGeometry.Cylinders.QuadPart;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.TracksPerCylinder = DeviceExtension->DiskGeometry.TracksPerCylinder;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.SectorsPerTrack = DeviceExtension->DiskGeometry.SectorsPerTrack;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector = DeviceExtension->DiskGeometry.BytesPerSector;
            mediaInfo--;

        }

        // Status will either be success, if media is present, or no media.
        // It would be optimal to base from density code and medium type, but not all devices
        // have values for these fields.

        // Send a TUR to determine if media is present, only if the device is not in ZPODD mode.
        if ((!EXCLUSIVE_MODE(cdData) ||
             EXCLUSIVE_OWNER(cdData, WdfRequestGetFileObject(Request))) &&
            ((zpoddInfo == NULL) ||
             (zpoddInfo->InZeroPowerState == FALSE)))
        {
            SCSI_REQUEST_BLOCK  srb;
            PCDB                cdb = (PCDB)srb.Cdb;

            RtlZeroMemory(&srb,sizeof(SCSI_REQUEST_BLOCK));

            srb.CdbLength = 6;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

            srb.TimeOutValue = CDROM_TEST_UNIT_READY_TIMEOUT;

            status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                                &srb,
                                                NULL,
                                                0,
                                                FALSE,
                                                Request);

            if (NT_SUCCESS(status))
            {
                // set the disk's media as current if we can write to it.
                if (cdData->Mmc.IsMmc && cdData->Mmc.WriteAllowed)
                {
                    mediaInfo++;
                    SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics,
                             MEDIA_CURRENTLY_MOUNTED);
                    mediaInfo--;
                }
                else
                {
                    SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics,
                             MEDIA_CURRENTLY_MOUNTED);
                }
            }
            else
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                           "RequestHandleGetMediaTypeEx: GET_MEDIA_TYPES status of TUR - %lx\n", status));
            }
        }

        // per legacy cdrom behavior, always return success
        status = STATUS_SUCCESS;
    }

    *DataLength = sizeNeeded;

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleGetDvdRegion(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handler for IOCTL_DVD_GET_REGION

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;

    PVOID               outputBuffer = NULL;
    size_t              bytesReturned = 0;

    PDVD_COPY_PROTECT_KEY       copyProtectKey = NULL;
    ULONG                       keyLength = 0;
    PDVD_DESCRIPTOR_HEADER      dvdHeader;
    PDVD_COPYRIGHT_DESCRIPTOR   copyRightDescriptor;
    PDVD_REGION                 dvdRegion = NULL;
    PDVD_READ_STRUCTURE         readStructure = NULL;
    PDVD_RPC_KEY                rpcKey;

    PAGED_CODE ();

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                "RequestHandleGetDvdRegion: [%p] IOCTL_DVD_GET_REGION\n", Request));

    *DataLength = 0;

    // reject the request if it's not a DVD device.
    if (DeviceExtension->DeviceAdditionalData.DriveDeviceType != FILE_DEVICE_DVD)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                sizeof(DVD_REGION),
                                                &outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        // figure out how much data buffer we need
        keyLength = max((sizeof(DVD_DESCRIPTOR_HEADER) + sizeof(DVD_COPYRIGHT_DESCRIPTOR)),
                        sizeof(DVD_READ_STRUCTURE));
        keyLength = max(keyLength,
                        DVD_RPC_KEY_LENGTH);

        // round the size to nearest ULONGLONG -- why?
        // could this be to deal with device alignment issues?
        keyLength += sizeof(ULONGLONG) - (keyLength & (sizeof(ULONGLONG) - 1));

        readStructure = ExAllocatePoolWithTag(NonPagedPoolNx,
                                              keyLength,
                                              DVD_TAG_READ_KEY);
        if (readStructure == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(status))
    {
        RtlZeroMemory (readStructure, keyLength);
        readStructure->Format = DvdCopyrightDescriptor;

        // use READ_STRUCTURE to read copyright descriptor
        status = ReadDvdStructure(DeviceExtension,
                                  Request,
                                  readStructure,
                                  keyLength,
                                  readStructure,
                                  sizeof(DVD_DESCRIPTOR_HEADER) + sizeof(DVD_COPYRIGHT_DESCRIPTOR),
                                  &bytesReturned);
    }

    if (NT_SUCCESS(status))
    {
        // we got the copyright descriptor, so now get the region if possible
        dvdHeader = (PDVD_DESCRIPTOR_HEADER) readStructure;
        copyRightDescriptor = (PDVD_COPYRIGHT_DESCRIPTOR) dvdHeader->Data;

        // the original irp's systembuffer has a copy of the info that
        // should be passed down in the request
        dvdRegion = outputBuffer;

        dvdRegion->CopySystem = copyRightDescriptor->CopyrightProtectionType;
        dvdRegion->RegionData = copyRightDescriptor->RegionManagementInformation;

        // now reuse the buffer to request the copy protection info
        copyProtectKey = (PDVD_COPY_PROTECT_KEY) readStructure;
        RtlZeroMemory (copyProtectKey, DVD_RPC_KEY_LENGTH);
        copyProtectKey->KeyLength = DVD_RPC_KEY_LENGTH;
        copyProtectKey->KeyType = DvdGetRpcKey;

        // send a request for READ_KEY
        status = DvdStartSessionReadKey(DeviceExtension,
                                        IOCTL_DVD_READ_KEY,
                                        Request,
                                        copyProtectKey,
                                        DVD_RPC_KEY_LENGTH,
                                        copyProtectKey,
                                        DVD_RPC_KEY_LENGTH,
                                        &bytesReturned);
    }

    if (NT_SUCCESS(status))
    {
        // the request succeeded.  if a supported scheme is returned,
        // then return the information to the caller
        rpcKey = (PDVD_RPC_KEY) copyProtectKey->KeyData;

        if (rpcKey->RpcScheme == 1)
        {
            if (rpcKey->TypeCode)
            {
                dvdRegion->SystemRegion = ~rpcKey->RegionMask;
                dvdRegion->ResetCount = rpcKey->UserResetsAvailable;
            }
            else
            {
                // the drive has not been set for any region
                dvdRegion->SystemRegion = 0;
                dvdRegion->ResetCount = rpcKey->UserResetsAvailable;
            }

            *DataLength = sizeof(DVD_REGION);
        }
        else
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "RequestHandleGetDvdRegion => rpcKey->RpcScheme != 1\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    // Clean up
    if (readStructure != NULL)
    {
        ExFreePool(readStructure);
    }

    return status;
}

NTSTATUS
RequestValidateRawRead(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_CDROM_RAW_READ

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCDROM_DATA         cdData = &(DeviceExtension->DeviceAdditionalData);

    PVOID               inputBuffer = NULL;
    PIRP                irp = NULL;
    PIO_STACK_LOCATION  currentStack = NULL;

    LARGE_INTEGER       startingOffset = {0};
    ULONGLONG           transferBytes = 0;
    ULONGLONG           endOffset;
    ULONGLONG           mdlBytes;
    RAW_READ_INFO       rawReadInfo = {0};

    *DataLength = 0;

    irp = WdfRequestWdmGetIrp(Request);
    currentStack = IoGetCurrentIrpStackLocation(irp);

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &inputBuffer,
                                           NULL);

    // Check that ending sector is on disc and buffers are there and of
    // correct size.
    if (NT_SUCCESS(status) &&
        (RequestParameters.Parameters.DeviceIoControl.Type3InputBuffer == NULL))
    {
        //  This is a call from user space.  This is the only time that we need to validate parameters.
        //  Validate the input and get the input buffer into Type3InputBuffer
        //  so the rest of the code will be uniform.
        if (inputBuffer != NULL)
        {
            currentStack->Parameters.DeviceIoControl.Type3InputBuffer = inputBuffer;
            RequestParameters.Parameters.DeviceIoControl.Type3InputBuffer = inputBuffer;

            if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(RAW_READ_INFO))
            {
                *DataLength = sizeof(RAW_READ_INFO);
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        //  Since this ioctl is METHOD_OUT_DIRECT, we need to copy away the input buffer before interpreting it.
        //  This prevents a malicious app from messing with the input buffer while we are interpreting it.
        rawReadInfo = *(PRAW_READ_INFO)RequestParameters.Parameters.DeviceIoControl.Type3InputBuffer;

        startingOffset.QuadPart = rawReadInfo.DiskOffset.QuadPart;

        if ((rawReadInfo.TrackMode == CDDA)        ||
            (rawReadInfo.TrackMode == YellowMode2) ||
            (rawReadInfo.TrackMode == XAForm2)     )
        {
            transferBytes = (ULONGLONG)rawReadInfo.SectorCount * RAW_SECTOR_SIZE;
        }
        else if (rawReadInfo.TrackMode == RawWithSubCode)
        {
            transferBytes = (ULONGLONG)rawReadInfo.SectorCount * CD_RAW_SECTOR_WITH_SUBCODE_SIZE;
        }
        else if (rawReadInfo.TrackMode == RawWithC2)
        {
            transferBytes = (ULONGLONG)rawReadInfo.SectorCount * CD_RAW_SECTOR_WITH_C2_SIZE;
        }
        else if (rawReadInfo.TrackMode == RawWithC2AndSubCode)
        {
            transferBytes = (ULONGLONG)rawReadInfo.SectorCount * CD_RAW_SECTOR_WITH_C2_AND_SUBCODE_SIZE;
        }
        else
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                      "RequestValidateRawRead: Invalid TrackMode type %x for XA read\n",
                      rawReadInfo.TrackMode
                      ));
        }

        endOffset = (ULONGLONG)rawReadInfo.SectorCount * COOKED_SECTOR_SIZE;
        endOffset += startingOffset.QuadPart;

        // check for overflows....
        if (rawReadInfo.SectorCount == 0)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "RequestValidateRawRead: Invalid I/O parameters for XA "
                        "Read (zero sectors requested)\n"));
            status = STATUS_INVALID_PARAMETER;
        }
        else if (transferBytes < (ULONGLONG)(rawReadInfo.SectorCount))
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "RequestValidateRawRead: Invalid I/O parameters for XA "
                        "Read (TransferBytes Overflow)\n"));
            status = STATUS_INVALID_PARAMETER;
        }
        else if (endOffset < (ULONGLONG)startingOffset.QuadPart)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "RequestValidateRawRead: Invalid I/O parameters for XA "
                        "Read (EndingOffset Overflow)\n"));
            status = STATUS_INVALID_PARAMETER;
        }
        else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < transferBytes)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "RequestValidateRawRead: Invalid I/O parameters for XA "
                        "Read (Bad buffer size)\n"));
            status = STATUS_INVALID_PARAMETER;
        }
        else if (endOffset > (ULONGLONG)DeviceExtension->PartitionLength.QuadPart)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "RequestValidateRawRead: Invalid I/O parameters for XA "
                        "Read (Request Out of Bounds)\n"));
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        // cannot validate the MdlAddress, since it is not included in any
        // other location per the DDK and file system calls.

        // validate the mdl describes at least the number of bytes
        // requested from us.
        mdlBytes = (ULONGLONG)MmGetMdlByteCount(irp->MdlAddress);
        if (mdlBytes < transferBytes)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "RequestValidateRawRead: Invalid MDL %s, Irp %p\n",
                        "size (5)", irp));
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        // check the buffer for alignment
        // This is important for x86 as some busses (ie ATAPI)
        // require word-aligned buffers.
        if ( ((ULONG_PTR)MmGetMdlVirtualAddress(irp->MdlAddress)) &
             DeviceExtension->AdapterDescriptor->AlignmentMask )
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                      "RequestValidateRawRead: Invalid I/O parameters for "
                      "XA Read (Buffer %p not aligned with mask %x\n",
                      RequestParameters.Parameters.DeviceIoControl.Type3InputBuffer,
                      DeviceExtension->AdapterDescriptor->AlignmentMask));
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        // Validate the request is not too large for the adapter
        BOOLEAN bufferIsPageAligned = FALSE;
        ULONG   maxLength = 0;

        // if buffer is not page-aligned, then subtract one as the
        // transfer could cross a page boundary.
        if ((((ULONG_PTR)MmGetMdlVirtualAddress(irp->MdlAddress)) & (PAGE_SIZE-1)) == 0)
        {
            bufferIsPageAligned = TRUE;
        }

        if (bufferIsPageAligned)
        {
            maxLength = cdData->MaxPageAlignedTransferBytes;
        }
        else
        {
            maxLength = cdData->MaxUnalignedTransferBytes;
        }

        if (transferBytes > maxLength)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                      "RequestValidateRawRead: The XA Read (type %x) would require %I64x bytes, "
                      "but the adapter can only handle %x bytes (for a%saligned buffer)\n",
                      rawReadInfo.TrackMode,
                      transferBytes,
                      maxLength,
                      (bufferIsPageAligned ? " " : "n un")
                      ));
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        //
        // HACKHACK - REF #0001
        // The retry count will be in this irp's IRP_MN function,
        // as the new irp was freed, and we therefore cannot use
        // this irp's next stack location for this function.
        // This may be a good location to store this info for
        // when we remove RAW_READ (mode switching), as we will
        // no longer have the nextIrpStackLocation to play with
        // when that occurs
        //
        currentStack->MinorFunction = MAXIMUM_RETRIES; // HACKHACK - REF #0001
    }

    return status;
}

NTSTATUS
RequestValidateReadTocEx(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_CDROM_READ_TOC_EX

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;

    PCDROM_READ_TOC_EX  inputBuffer = NULL;

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
        sizeof(CDROM_READ_TOC_EX))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
             MINIMUM_CDROM_READ_TOC_EX_SIZE)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        *DataLength = MINIMUM_CDROM_READ_TOC_EX_SIZE;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength >
             ((USHORT)-1))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength &
             DeviceExtension->AdapterDescriptor->AlignmentMask)
    {
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if ((inputBuffer->Reserved1 != 0) ||
            (inputBuffer->Reserved2 != 0) ||
            (inputBuffer->Reserved3 != 0))
        {
            status = STATUS_INVALID_PARAMETER;
        }
        // NOTE: when adding new formats, ensure that first two bytes
        //       specify the amount of additional data available.
        else if ((inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_TOC     ) ||
                 (inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_FULL_TOC) ||
                 (inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_CDTEXT  ))
        {
            // SessionTrack field is used
        }
        else if ((inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_SESSION) ||
                 (inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_PMA)     ||
                 (inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_ATIP))
        {
            // SessionTrack field is reserved
            if (inputBuffer->SessionTrack != 0)
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateReadToc(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_CDROM_READ_TOC

Arguments:

    DeviceExtension - device context
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(CDROM_TOC))
    {
        // they didn't request the entire TOC -- use _EX version
        // for partial transfers and such.
        status = STATUS_BUFFER_TOO_SMALL;
        *DataLength = sizeof(CDROM_TOC);
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength &
             DeviceExtension->AdapterDescriptor->AlignmentMask)
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

NTSTATUS
RequestValidateGetLastSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_CDROM_GET_LAST_SESSION

Arguments:

    DeviceExtension - device context
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(CDROM_TOC_SESSION_DATA))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        *DataLength = sizeof(CDROM_TOC_SESSION_DATA);
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength &
             DeviceExtension->AdapterDescriptor->AlignmentMask)
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

NTSTATUS
RequestValidateReadQChannel(
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_CDROM_READ_Q_CHANNEL

Arguments:

    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                 status = STATUS_SUCCESS;
    PCDROM_SUB_Q_DATA_FORMAT inputBuffer = NULL;
    ULONG                    transferByteCount = 0;

    *DataLength = 0;

    if(RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
       sizeof(CDROM_SUB_Q_DATA_FORMAT))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        // check for all valid types of request
        if (inputBuffer->Format == IOCTL_CDROM_CURRENT_POSITION)
        {
            transferByteCount = sizeof(SUB_Q_CURRENT_POSITION);
        }
        else if (inputBuffer->Format == IOCTL_CDROM_MEDIA_CATALOG)
        {
            transferByteCount = sizeof(SUB_Q_MEDIA_CATALOG_NUMBER);
        }
        else if (inputBuffer->Format == IOCTL_CDROM_TRACK_ISRC)
        {
            transferByteCount = sizeof(SUB_Q_TRACK_ISRC);
        }
        else
        {
            // Format not valid
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            transferByteCount)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            *DataLength = transferByteCount;
        }
    }

    return status;
}

NTSTATUS
RequestValidateDvdReadStructure(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_DVD_READ_STRUCTURE

Arguments:

    DeviceExtension - device context
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        if(RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
           sizeof(DVD_READ_STRUCTURE))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestValidateDvdReadStructure - input buffer "
                        "length too small (was %d should be %d)\n",
                        (int)RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                        sizeof(DVD_READ_STRUCTURE)));
            status = STATUS_INVALID_PARAMETER;
        }
        else if(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(READ_DVD_STRUCTURES_HEADER))
        {

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestValidateDvdReadStructure - output buffer "
                        "cannot hold header information\n"));
            status = STATUS_BUFFER_TOO_SMALL;
            *DataLength = sizeof(READ_DVD_STRUCTURES_HEADER);
        }
        else if(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength >
                MAXUSHORT)
        {
            // key length must fit in two bytes
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestValidateDvdReadStructure - output buffer "
                        "too large\n"));
            status = STATUS_INVALID_PARAMETER;
        }
        else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength &
                 DeviceExtension->AdapterDescriptor->AlignmentMask)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else if (DeviceExtension->DeviceAdditionalData.DriveDeviceType != FILE_DEVICE_DVD)
        {
            // reject the request if it's not a DVD device.
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    return status;
}

NTSTATUS
RequestValidateDvdStartSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

    Validate request of IOCTL_DVD_START_SESSION

Arguments:

    DeviceExtension - device context
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        if(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
           sizeof(DVD_SESSION_ID))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestValidateDvdStartSession: DVD_START_SESSION - output "
                        "buffer too small\n"));
            status = STATUS_BUFFER_TOO_SMALL;
            *DataLength = sizeof(DVD_SESSION_ID);
        }
        else if (DeviceExtension->DeviceAdditionalData.DriveDeviceType != FILE_DEVICE_DVD)
        {
            // reject the request if it's not a DVD device.
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    return status;
}

NTSTATUS
RequestValidateDvdSendKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_DVD_SEND_KEY, IOCTL_DVD_SEND_KEY2

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDVD_COPY_PROTECT_KEY   key = NULL;

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &key,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        if((RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(DVD_COPY_PROTECT_KEY)) ||
           (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != key->KeyLength))
        {

            //
            // Key is too small to have a header or the key length doesn't
            // match the input buffer length.  Key must be invalid
            //

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestValidateDvdSendKey: [%p] IOCTL_DVD_SEND_KEY - "
                        "key is too small or does not match KeyLength\n",
                        Request));
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        // allow only certain key type (non-destructive) to go through
        // IOCTL_DVD_SEND_KEY (which only requires READ access to the device)
        if (RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_DVD_SEND_KEY)
        {
            if ((key->KeyType != DvdChallengeKey) &&
                (key->KeyType != DvdBusKey2) &&
                (key->KeyType != DvdInvalidateAGID))
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else if ((key->KeyType != DvdChallengeKey) &&
                (key->KeyType != DvdBusKey1) &&
                (key->KeyType != DvdBusKey2) &&
                (key->KeyType != DvdTitleKey) &&
                (key->KeyType != DvdAsf) &&
                (key->KeyType != DvdSetRpcKey) &&
                (key->KeyType != DvdGetRpcKey) &&
                (key->KeyType != DvdDiskKey) &&
                (key->KeyType != DvdInvalidateAGID))
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else if (DeviceExtension->DeviceAdditionalData.DriveDeviceType != FILE_DEVICE_DVD)
        {
            // reject the request if it's not a DVD device.
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    return status;
}

NTSTATUS
RequestValidateGetConfiguration(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_CDROM_GET_CONFIGURATION

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(GET_CONFIGURATION_HEADER))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        *DataLength = sizeof(GET_CONFIGURATION_HEADER);
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > 0xffff)
    {
        // output buffer is too large
        status = STATUS_INVALID_BUFFER_SIZE;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength &
        DeviceExtension->AdapterDescriptor->AlignmentMask)
    {
        // buffer is not proper size multiple
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status))
    {

#if BUILD_WOW64_ENABLED && defined(_WIN64)

        if (WdfRequestIsFrom32BitProcess(Request))
        {
            PGET_CONFIGURATION_IOCTL_INPUT32 inputBuffer = NULL;

            if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
                sizeof(GET_CONFIGURATION_IOCTL_INPUT32))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            //
            // also verify the arguments are reasonable.
            //
            if (NT_SUCCESS(status))
            {
                status = WdfRequestRetrieveInputBuffer(Request,
                                                       RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                                       &inputBuffer,
                                                       NULL);
            }

            if (NT_SUCCESS(status))
            {
                if (inputBuffer->Feature > 0xffff)
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if ((inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE) &&
                         (inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_CURRENT) &&
                         (inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (inputBuffer->Reserved[0] || inputBuffer->Reserved[1])
                {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
        }
        else

#endif

        {
            PGET_CONFIGURATION_IOCTL_INPUT inputBuffer = NULL;

            if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
                sizeof(GET_CONFIGURATION_IOCTL_INPUT))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            // also verify the arguments are reasonable.
            if (NT_SUCCESS(status))
            {
                status = WdfRequestRetrieveInputBuffer(Request,
                                                       RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                                       &inputBuffer,
                                                       NULL);
            }

            if (NT_SUCCESS(status))
            {
                if (inputBuffer->Feature > 0xffff)
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if ((inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE) &&
                         (inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_CURRENT) &&
                         (inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (inputBuffer->Reserved[0] || inputBuffer->Reserved[1])
                {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
        }
    }

    return status;
}

NTSTATUS
RequestValidateSetSpeed(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_CDROM_SET_SPEED

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);
    PCDROM_SET_SPEED        inputBuffer = NULL;
    ULONG                   requiredLength = 0;

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(CDROM_SET_SPEED))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        // Get the request type using CDROM_SET_SPEED structure
        status = WdfRequestRetrieveInputBuffer(Request,
                                               sizeof(CDROM_SET_SPEED),
                                               &inputBuffer,
                                               NULL);

    }

    if (NT_SUCCESS(status))
    {
        if (inputBuffer->RequestType > CdromSetStreaming)
        {
            // Unknown request type.
            status = STATUS_INVALID_PARAMETER;
        }
        else if (inputBuffer->RequestType == CdromSetSpeed)
        {
            requiredLength = sizeof(CDROM_SET_SPEED);
        }
        else
        {
            // Don't send SET STREAMING command if this is not a MMC compliant device
            if (cdData->Mmc.IsMmc == FALSE)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

            requiredLength = sizeof(CDROM_SET_STREAMING);
        }
    }

    if (NT_SUCCESS(status))
    {
        if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < requiredLength)
        {
            // Input buffer too small
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsReadMediaKeyBlock(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_READ_MEDIA_KEY_BLOCK

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);
    PAACS_LAYER_NUMBER      layerNumber = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(AACS_LAYER_NUMBER))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < 8)
    {
        // This is a variable-length structure, but we're pretty sure
        // it can never be less than eight bytes...
        *DataLength = 8;
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &layerNumber,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (*layerNumber > 255)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsStartSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_START_SESSION

Arguments:

    DeviceExtension - device context
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(DVD_SESSION_ID))
    {
        *DataLength = sizeof(DVD_SESSION_ID);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > sizeof(DVD_SESSION_ID))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    return status;
}

NTSTATUS
RequestValidateAacsSendCertificate(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_SEND_CERTIFICATE

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);
    PAACS_SEND_CERTIFICATE  inputBuffer = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(AACS_SEND_CERTIFICATE))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (inputBuffer->SessionId > MAX_COPY_PROTECT_AGID)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsGetCertificate(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_GET_CERTIFICATE

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);
    PDVD_SESSION_ID         sessionId = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(DVD_SESSION_ID))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(AACS_CERTIFICATE))
    {
        *DataLength = sizeof(AACS_CERTIFICATE);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > sizeof(AACS_CERTIFICATE))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &sessionId,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (*sessionId > MAX_COPY_PROTECT_AGID)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsGetChallengeKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_GET_CHALLENGE_KEY

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);
    PDVD_SESSION_ID         sessionId = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(DVD_SESSION_ID))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(AACS_CHALLENGE_KEY))
    {
        *DataLength = sizeof(AACS_CHALLENGE_KEY);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > sizeof(AACS_CHALLENGE_KEY))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &sessionId,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (*sessionId > MAX_COPY_PROTECT_AGID)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsSendChallengeKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_SEND_CHALLENGE_KEY

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PCDROM_DATA                 cdData = &(DeviceExtension->DeviceAdditionalData);
    PAACS_SEND_CHALLENGE_KEY    inputBuffer = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(AACS_SEND_CHALLENGE_KEY))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (inputBuffer->SessionId > MAX_COPY_PROTECT_AGID)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsReadVolumeId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_READ_VOLUME_ID

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);
    PDVD_SESSION_ID         sessionId = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(DVD_SESSION_ID))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(AACS_VOLUME_ID))
    {
        *DataLength = sizeof(AACS_VOLUME_ID);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > sizeof(AACS_VOLUME_ID))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &sessionId,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (*sessionId > MAX_COPY_PROTECT_AGID)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsReadSerialNumber(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_READ_SERIAL_NUMBER

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);
    PDVD_SESSION_ID         sessionId = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(DVD_SESSION_ID))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(AACS_SERIAL_NUMBER))
    {
        *DataLength = sizeof(AACS_SERIAL_NUMBER);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > sizeof(AACS_SERIAL_NUMBER))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &sessionId,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (*sessionId > MAX_COPY_PROTECT_AGID)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsReadMediaId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_READ_MEDIA_ID

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DATA             cdData = &(DeviceExtension->DeviceAdditionalData);
    PDVD_SESSION_ID         sessionId = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(DVD_SESSION_ID))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(AACS_MEDIA_ID))
    {
        *DataLength = sizeof(AACS_MEDIA_ID);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > sizeof(AACS_MEDIA_ID))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &sessionId,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (*sessionId > MAX_COPY_PROTECT_AGID)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateAacsBindingNonce(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_READ_BINDING_NONCE
                       IOCTL_AACS_GENERATE_BINDING_NONCE

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                 status = STATUS_SUCCESS;
    PCDROM_DATA              cdData = &(DeviceExtension->DeviceAdditionalData);
    PAACS_READ_BINDING_NONCE inputBuffer = NULL;

    *DataLength = 0;

    if (!cdData->Mmc.IsAACS)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(AACS_READ_BINDING_NONCE))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(AACS_BINDING_NONCE))
    {
        *DataLength = sizeof(AACS_BINDING_NONCE);
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > sizeof(AACS_BINDING_NONCE))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (inputBuffer->SessionId > MAX_COPY_PROTECT_AGID)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else if (inputBuffer->NumberOfSectors > 255)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else if (inputBuffer->StartLba > MAXULONG)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTATUS
RequestValidateExclusiveAccess(
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_CDROM_EXCLUSIVE_ACCESS

Arguments:

    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_EXCLUSIVE_ACCESS exclusiveAccess = NULL;

    *DataLength = 0;

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "RequestValidateExclusiveAccess: IOCTL must be called at passive level.\n"));
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(CDROM_EXCLUSIVE_ACCESS))
    {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "RequestValidateExclusiveAccess: Input buffer too small\n"));
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &exclusiveAccess,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        switch (exclusiveAccess->RequestType)
        {
            case ExclusiveAccessQueryState:
            {
                if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(CDROM_EXCLUSIVE_LOCK_STATE))
                {
                    //
                    // Output buffer too small.
                    //
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "RequestValidateExclusiveAccess: Output buffer too small\n"));
                    *DataLength = sizeof(CDROM_EXCLUSIVE_LOCK_STATE);
                    status = STATUS_BUFFER_TOO_SMALL;
                }
                break;
            }

            case ExclusiveAccessLockDevice:
            {
                if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(CDROM_EXCLUSIVE_LOCK))
                {
                    //
                    // Input buffer too small
                    //
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "RequestValidateExclusiveAccess: Input buffer too small\n"));
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }
            case ExclusiveAccessUnlockDevice:
            {
                //
                // Nothing to check
                //
                break;
            }

            default:
            {
                //
                // Unknown request type.
                //
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "RequestValidateExclusiveAccess: Invalid request type\n"));
                status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleExclusiveAccessQueryLockState(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_EXCLUSIVE_ACCESS with ExclusiveAccessQueryState

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION     deviceExtension = DeviceGetExtension(Device);
    PCDROM_DATA                 cdData = &deviceExtension->DeviceAdditionalData;
    PCDROM_EXCLUSIVE_LOCK_STATE exclusiveLockState = NULL;

    PAGED_CODE();

    status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeof(CDROM_EXCLUSIVE_LOCK_STATE),
                                            &exclusiveLockState,
                                            NULL);
    NT_ASSERT(NT_SUCCESS(status));

    RtlZeroMemory(exclusiveLockState, sizeof(CDROM_EXCLUSIVE_LOCK_STATE));

    if (EXCLUSIVE_MODE(cdData))
    {
        // Device is locked for exclusive use
        exclusiveLockState->LockState = TRUE;

        RtlCopyMemory(&exclusiveLockState->CallerName,
                      &cdData->CallerName,
                      CDROM_EXCLUSIVE_CALLER_LENGTH);

    }
    else
    {
        // Device is not locked
        exclusiveLockState->LockState = FALSE;
    }

    RequestCompletion(deviceExtension, Request, status, sizeof(CDROM_EXCLUSIVE_LOCK_STATE));

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleExclusiveAccessLockDevice(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_EXCLUSIVE_ACCESS with ExclusiveAccessLockDevice

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PCDROM_DATA             cdData = &deviceExtension->DeviceAdditionalData;
    PCDROM_EXCLUSIVE_LOCK   exclusiveLock = NULL;
    PIO_ERROR_LOG_PACKET    logEntry;

    WDFFILEOBJECT           fileObject = NULL;
    ULONG                   idx = 0;
    ULONG                   nameLength = 0;

    PAGED_CODE();

    fileObject = WdfRequestGetFileObject(Request);

    if (fileObject == NULL)
    {
        status = STATUS_INVALID_HANDLE;

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                    "RequestHandleExclusiveAccessLockDevice: FileObject is NULL, cannot grant exclusive access\n"));
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               sizeof(CDROM_EXCLUSIVE_LOCK),
                                               &exclusiveLock,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        // Validate the caller name string
        for (idx = 0; (idx < CDROM_EXCLUSIVE_CALLER_LENGTH) && (exclusiveLock->CallerName[idx] != '\0'); idx++)
        {
            if (!ValidChar(exclusiveLock->CallerName[idx]))
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                            "RequestHandleExclusiveAccessLockDevice: Invalid characters in caller name\n"));
                // error out
                status = STATUS_INVALID_PARAMETER;
                break;
            }
        }
    }

    if (NT_SUCCESS(status))
    {
        if ((idx == 0) || (idx >= CDROM_EXCLUSIVE_CALLER_LENGTH))
        {

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestHandleExclusiveAccessLockDevice: Not a valid null terminated string.\n"));
            //error out
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            nameLength = idx+1; // Add 1 for the NULL character
            NT_ASSERT(nameLength <= CDROM_EXCLUSIVE_CALLER_LENGTH);
        }
    }

    // If the file system is still mounted on this device fail the request,
    // unless the force lock flag is set.
    if (NT_SUCCESS(status))
    {
        if ((TEST_FLAG(exclusiveLock->Access.Flags, CDROM_LOCK_IGNORE_VOLUME) == FALSE) &&
            IsVolumeMounted(deviceExtension->DeviceObject))
        {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                            "RequestHandleExclusiveAccessLockDevice: Unable to lock device, file system mounted\n"));
                status = STATUS_INVALID_DEVICE_STATE;
        }
    }

    // Lock the device for exclusive access if the device is not already locked
    if (NT_SUCCESS(status))
    {
        if (InterlockedCompareExchangePointer((PVOID)&cdData->ExclusiveOwner, (PVOID)fileObject, NULL) == NULL)
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                        "RequestHandleExclusiveAccessLockDevice: Entering exclusive mode! Device locked by file object %p\n", fileObject));

            // Zero out the CallerName before storing it in the extension
            RtlZeroMemory(&cdData->CallerName, CDROM_EXCLUSIVE_CALLER_LENGTH);
            RtlCopyMemory(&cdData->CallerName,
                          &exclusiveLock->CallerName,
                          nameLength);

            // Send Exclusive Lock notification
            DeviceSendNotification(deviceExtension,
                                   &GUID_IO_CDROM_EXCLUSIVE_LOCK,
                                   0,
                                   NULL);

            // Log an informational event with the caller name
            logEntry = IoAllocateErrorLogEntry(
                                deviceExtension->DeviceObject,
                                sizeof(IO_ERROR_LOG_PACKET) + CDROM_EXCLUSIVE_CALLER_LENGTH);

            if (logEntry != NULL)
            {
                PUCHAR dumpDataPtr = (PUCHAR) logEntry->DumpData;

                logEntry->FinalStatus       = STATUS_SUCCESS;
                logEntry->ErrorCode         = IO_CDROM_EXCLUSIVE_LOCK;
                logEntry->SequenceNumber    = 0;
                logEntry->MajorFunctionCode = IRP_MJ_DEVICE_CONTROL;
                logEntry->IoControlCode     = IOCTL_CDROM_EXCLUSIVE_ACCESS;
                logEntry->RetryCount        = 0;
                logEntry->UniqueErrorValue  = 0x1;
                logEntry->DumpDataSize      = CDROM_EXCLUSIVE_CALLER_LENGTH;

                RtlCopyMemory(dumpDataPtr,
                                (PUCHAR)&cdData->CallerName,
                                CDROM_EXCLUSIVE_CALLER_LENGTH);

                // Write the error log packet.
                IoWriteErrorLogEntry(logEntry);
            }

        }
        else
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestHandleExclusiveAccessLockDevice: Unable to lock device, device already locked.\n"));

            status = STATUS_ACCESS_DENIED;
        }
    }

    RequestCompletion(deviceExtension, Request, status, 0);

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleExclusiveAccessUnlockDevice(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_EXCLUSIVE_ACCESS with ExclusiveAccessUnlockDevice

Arguments:

    Device - device handle
    Request - request to be handled

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PCDROM_EXCLUSIVE_ACCESS exclusiveAccess = NULL;
    WDFFILEOBJECT           fileObject = NULL;

    PAGED_CODE();

    fileObject = WdfRequestGetFileObject(Request);

    if (fileObject == NULL)
    {
        // The device can be unlocked from exclusive mode only via the file object which locked it.
        status = STATUS_INVALID_HANDLE;

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                    "RequestHandleExclusiveAccessUnlockDevice: FileObject is NULL, cannot release exclusive access\n"));
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               sizeof(PCDROM_EXCLUSIVE_ACCESS),
                                               &exclusiveAccess,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = DeviceUnlockExclusive(deviceExtension, fileObject,
                        TEST_FLAG(exclusiveAccess->Flags, CDROM_NO_MEDIA_NOTIFICATIONS));

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "RequestHandleExclusiveAccessUnlockDevice: Device unlocked\n"));
    }

    RequestCompletion(deviceExtension, Request, status, 0);

    return status;
}

NTSTATUS
RequestHandleQueryPropertyRetrieveCachedData(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_QUERY_PROPERTY when the required data is cached.

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PSTORAGE_PROPERTY_QUERY inputBuffer = NULL;

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        if (inputBuffer->PropertyId == StorageDeviceProperty)
        {
            // check output buffer length
            if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength == 0)
            {
                // According to MSDN, an output buffer of size 0 can be used to determine if a property exists
                // so this must be a success case with no data transferred
                *DataLength = 0;
                status = STATUS_SUCCESS;
            }
            else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_DESCRIPTOR_HEADER))
            {
                // Buffer too small
                *DataLength = DeviceExtension->DeviceDescriptor->Size;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PSTORAGE_DEVICE_DESCRIPTOR  outputDescriptor = NULL;
                CHAR*                       localDescriptorBuffer = (CHAR*)DeviceExtension->DeviceDescriptor;

                status = WdfRequestRetrieveOutputBuffer(Request,
                                                        RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                        &outputDescriptor,
                                                        NULL);

                if (NT_SUCCESS(status))
                {
                    // transfer as much data out as the buffer will allow
                    *DataLength = min(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                      DeviceExtension->DeviceDescriptor->Size);

                    RtlCopyMemory(outputDescriptor,
                                  DeviceExtension->DeviceDescriptor,
                                  *DataLength);

                    // walk through and update offset variables to reflect data that didn't make it into the output buffer
                    if ((*DataLength >= RTL_SIZEOF_THROUGH_FIELD(STORAGE_DEVICE_DESCRIPTOR, VendorIdOffset)) &&
                        (DeviceExtension->DeviceDescriptor->VendorIdOffset != 0) &&
                        (DeviceExtension->DeviceDescriptor->VendorIdOffset != 0xFFFFFFFF))
                    {
                        // set VendorIdOffset appropriately
                        if (*DataLength <
                            (DeviceExtension->DeviceDescriptor->VendorIdOffset + strlen(localDescriptorBuffer + DeviceExtension->DeviceDescriptor->VendorIdOffset)))
                        {
                            outputDescriptor->VendorIdOffset = 0;
                        }
                    }

                    if ((*DataLength >= RTL_SIZEOF_THROUGH_FIELD(STORAGE_DEVICE_DESCRIPTOR, ProductIdOffset)) &&
                        (DeviceExtension->DeviceDescriptor->ProductIdOffset != 0) &&
                        (DeviceExtension->DeviceDescriptor->ProductIdOffset != 0xFFFFFFFF))
                    {
                        // set ProductIdOffset appropriately
                        if (*DataLength <
                            (DeviceExtension->DeviceDescriptor->ProductIdOffset + strlen(localDescriptorBuffer + DeviceExtension->DeviceDescriptor->ProductIdOffset)))
                        {
                            outputDescriptor->ProductIdOffset = 0;
                        }
                    }

                    if ((*DataLength >= RTL_SIZEOF_THROUGH_FIELD(STORAGE_DEVICE_DESCRIPTOR, ProductRevisionOffset)) &&
                        (DeviceExtension->DeviceDescriptor->ProductRevisionOffset != 0) &&
                        (DeviceExtension->DeviceDescriptor->ProductRevisionOffset != 0xFFFFFFFF))
                    {
                        // set ProductRevisionOffset appropriately
                        if (*DataLength <
                            (DeviceExtension->DeviceDescriptor->ProductRevisionOffset + strlen(localDescriptorBuffer + DeviceExtension->DeviceDescriptor->ProductRevisionOffset)))
                        {
                            outputDescriptor->ProductRevisionOffset = 0;
                        }
                    }

                    if ((*DataLength >= RTL_SIZEOF_THROUGH_FIELD(STORAGE_DEVICE_DESCRIPTOR, SerialNumberOffset)) &&
                        (DeviceExtension->DeviceDescriptor->SerialNumberOffset != 0) &&
                        (DeviceExtension->DeviceDescriptor->SerialNumberOffset != 0xFFFFFFFF))
                    {
                        // set SerialNumberOffset appropriately
                        if (*DataLength <
                            (DeviceExtension->DeviceDescriptor->SerialNumberOffset + strlen(localDescriptorBuffer + DeviceExtension->DeviceDescriptor->SerialNumberOffset)))
                        {
                            // NOTE: setting this to 0 since that is what most port drivers do
                            //       [this could cause issues with SCSI port devices whose clients expect -1 in this field]
                            outputDescriptor->SerialNumberOffset = 0;
                        }
                    }
                    status = STATUS_SUCCESS;
                }
            }
        }   //end of StorageDeviceProperty
        else if (inputBuffer->PropertyId == StorageAdapterProperty)
        {
            if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength == 0)
            {
                // According to MSDN, an output buffer of size 0 can be used to determine if a property exists
                // so this must be a success case with no data transferred
                *DataLength = 0;
                status = STATUS_SUCCESS;
            }
            else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_DESCRIPTOR_HEADER))
            {
                // Buffer too small
                *DataLength = DeviceExtension->AdapterDescriptor->Size;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                PSTORAGE_ADAPTER_DESCRIPTOR outputDescriptor = NULL;

                status = WdfRequestRetrieveOutputBuffer(Request,
                                                        RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                        &outputDescriptor,
                                                        NULL);
                if (NT_SUCCESS(status))
                {
                    // copy as much data out as the buffer will allow
                    *DataLength = min(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                      DeviceExtension->AdapterDescriptor->Size);

                    RtlCopyMemory(outputDescriptor,
                                  DeviceExtension->AdapterDescriptor,
                                  *DataLength);

                    // set status
                    status = STATUS_SUCCESS;
                }
            }
        }
    }

    return status;
}

NTSTATUS
RequestHandleQueryPropertyDeviceUniqueId(
    _In_ WDFDEVICE        Device,
    _In_ WDFREQUEST       Request
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_QUERY_PROPERTY with StorageDeviceUniqueIdProperty.

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION     deviceExtension = DeviceGetExtension(Device);
    PSTORAGE_PROPERTY_QUERY     inputBuffer = NULL;
    PSTORAGE_DESCRIPTOR_HEADER  descHeader = NULL;
    size_t                      outLength = 0;
    WDF_REQUEST_PARAMETERS      requestParameters;

    // Get the Request parameters
    WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
    WdfRequestGetParameters(Request, &requestParameters);

    status = WdfRequestRetrieveInputBuffer(Request,
                                           requestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        BOOLEAN overflow = FALSE;
        BOOLEAN infoFound = FALSE;

        // Must run at less then dispatch.
        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            NT_ASSERT(FALSE);
            outLength = 0;
            status = STATUS_INVALID_LEVEL;
        }
        else if (inputBuffer->QueryType == PropertyExistsQuery)
        {
            outLength = 0;
            status = STATUS_SUCCESS;
        }
        else if (inputBuffer->QueryType != PropertyStandardQuery)
        {
            outLength = 0;
            status = STATUS_NOT_SUPPORTED;
        }
        else
        {
            // Check AdditionalParameters validity.
            if (inputBuffer->AdditionalParameters[0] == DUID_INCLUDE_SOFTWARE_IDS)
            {
                // Do nothing
            }
            else if (inputBuffer->AdditionalParameters[0] == DUID_HARDWARE_IDS_ONLY)
            {
                // Do nothing
            }
            else
            {
                outLength = 0;
                status = STATUS_INVALID_PARAMETER;
            }

            if (NT_SUCCESS(status) &&
                (outLength < sizeof(STORAGE_DESCRIPTOR_HEADER)))
            {
                outLength = 0;
                status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        // From this point forward the status depends on the overflow
        // and infoFound flags.
        if (NT_SUCCESS(status))
        {
            outLength = requestParameters.Parameters.DeviceIoControl.OutputBufferLength;
            status = WdfRequestRetrieveOutputBuffer(Request,
                                                    requestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                    &descHeader,
                                                    NULL);
        }

        if (NT_SUCCESS(status))
        {
            RtlZeroMemory(descHeader, outLength);

            descHeader->Version = DUID_VERSION_1;
            descHeader->Size = sizeof(STORAGE_DEVICE_UNIQUE_IDENTIFIER);

            // Try to build device unique id from StorageDeviceIdProperty.
            status = RequestDuidGetDeviceIdProperty(deviceExtension,
                                                    Request,
                                                    requestParameters,
                                                    &outLength);

            if (status == STATUS_BUFFER_OVERFLOW)
            {
                overflow = TRUE;
            }

            if (NT_SUCCESS(status))
            {
                infoFound = TRUE;
            }

            // Try to build device unique id from StorageDeviceProperty.
            status = RequestDuidGetDeviceProperty(deviceExtension,
                                                  Request,
                                                  requestParameters,
                                                  &outLength);

            if (status == STATUS_BUFFER_OVERFLOW)
            {
                overflow = TRUE;
            }

            if (NT_SUCCESS(status))
            {
                infoFound = TRUE;
            }

            // Return overflow, success, or a generic error.
            if (overflow)
            {
                // If output buffer is STORAGE_DESCRIPTOR_HEADER, then return
                // success to the user.  Otherwise, send an error so the user
                // knows a larger buffer is required.
                if (outLength == sizeof(STORAGE_DESCRIPTOR_HEADER))
                {
                    status = STATUS_SUCCESS;
                }
                else
                {
                    outLength = (ULONG)WdfRequestGetInformation(Request);
                    status = STATUS_BUFFER_OVERFLOW;
                }

            }
            else if (infoFound)
            {
                status = STATUS_SUCCESS;

                // Exercise the compare routine.  This should always succeed.
                NT_ASSERT(DuidExactMatch == CompareStorageDuids((PSTORAGE_DEVICE_UNIQUE_IDENTIFIER)descHeader,
                                                             (PSTORAGE_DEVICE_UNIQUE_IDENTIFIER)descHeader));

            }
            else
            {
                status = STATUS_NOT_FOUND;
            }
        }
    }

    RequestCompletion(deviceExtension, Request, status, outLength);

    return status;
}

NTSTATUS
RequestHandleQueryPropertyWriteCache(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_QUERY_PROPERTY with StorageDeviceWriteCacheProperty.

Arguments:

    DeviceExtension - device context
    Request - request to be handled

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                      status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION       deviceExtension = DeviceGetExtension(Device);
    PSTORAGE_PROPERTY_QUERY       query = NULL;
    PSTORAGE_WRITE_CACHE_PROPERTY writeCache = NULL;
    PMODE_PARAMETER_HEADER        modeData = NULL;
    PMODE_CACHING_PAGE            pageData = NULL;
    size_t                        length = 0;
    ULONG                         information = 0;
    PSCSI_REQUEST_BLOCK           srb = NULL;
    WDF_REQUEST_PARAMETERS        requestParameters;

    // Get the Request parameters
    WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
    WdfRequestGetParameters(Request, &requestParameters);

    status = WdfRequestRetrieveInputBuffer(Request,
                                           requestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &query,
                                           NULL);

    if (NT_SUCCESS(status))
    {

        // Must run at less then dispatch.
        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            NT_ASSERT(FALSE);
            status = STATUS_INVALID_LEVEL;
        }
        else if (query->QueryType == PropertyExistsQuery)
        {
            information = 0;
            status = STATUS_SUCCESS;
        }
        else if (query->QueryType != PropertyStandardQuery)
        {
            status = STATUS_NOT_SUPPORTED;
        }
    }

    if (NT_SUCCESS(status))
    {
        length = requestParameters.Parameters.DeviceIoControl.OutputBufferLength;

        if (length < sizeof(STORAGE_DESCRIPTOR_HEADER))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                requestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &writeCache,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        RtlZeroMemory(writeCache, length);

        // Set version and required size.
        writeCache->Version = sizeof(STORAGE_WRITE_CACHE_PROPERTY);
        writeCache->Size = sizeof(STORAGE_WRITE_CACHE_PROPERTY);

        if (length < sizeof(STORAGE_WRITE_CACHE_PROPERTY))
        {
            // caller only wants header information, bail out.
            information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            status = STATUS_SUCCESS;

            RequestCompletion(deviceExtension, Request, status, information);
            return status;
        }
    }

    if (NT_SUCCESS(status))
    {
        srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                    sizeof(SCSI_REQUEST_BLOCK) +
                                    (sizeof(ULONG_PTR) * 2),
                                    CDROM_TAG_SRB);

        if (srb == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(status))
    {
        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        // Set known values
        writeCache->NVCacheEnabled = FALSE;
        writeCache->UserDefinedPowerProtection = TEST_FLAG(deviceExtension->DeviceFlags, DEV_POWER_PROTECTED);

        // Check for flush cache support by sending a sync cache command
        // to the device.

        // Set timeout value and mark the request as not being a tagged request.
        srb->Length = SCSI_REQUEST_BLOCK_SIZE;
        srb->TimeOutValue = TimeOutValueGetCapValue(deviceExtension->TimeOutValue, 4);
        srb->QueueTag = SP_UNTAGGED;
        srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
        srb->SrbFlags = deviceExtension->SrbFlags;

        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->CdbLength = 10;

        srb->Cdb[0] = SCSIOP_SYNCHRONIZE_CACHE;

        status = DeviceSendSrbSynchronously(Device,
                                            srb,
                                            NULL,
                                            0,
                                            TRUE,   //flush drive cache
                                            Request);

        if (NT_SUCCESS(status))
        {
            writeCache->FlushCacheSupported = TRUE;
        }
        else
        {
            // Device does not support sync cache
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                        "RequestHandleQueryPropertyWriteCache: Synchronize cache failed with status 0x%X\n", status));
            writeCache->FlushCacheSupported = FALSE;

            // Reset the status if there was any failure
            status = STATUS_SUCCESS;
        }

        modeData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                         MODE_PAGE_DATA_SIZE,
                                         CDROM_TAG_MODE_DATA);

        if (modeData == NULL)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "RequestHandleQueryPropertyWriteCache: Unable to allocate mode data buffer\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(status))
    {
        RtlZeroMemory(modeData, MODE_PAGE_DATA_SIZE);

        length = DeviceRetrieveModeSenseUsingScratch(deviceExtension,
                                                     (PCHAR)modeData,
                                                     MODE_PAGE_DATA_SIZE,
                                                     MODE_PAGE_CACHING,
                                                     MODE_SENSE_CURRENT_VALUES);

        if (length < sizeof(MODE_PARAMETER_HEADER))
        {
            // Retry the request in case of a check condition.
            length = DeviceRetrieveModeSenseUsingScratch(deviceExtension,
                                                         (PCHAR)modeData,
                                                         MODE_PAGE_DATA_SIZE,
                                                         MODE_PAGE_CACHING,
                                                         MODE_SENSE_CURRENT_VALUES);

            if (length < sizeof(MODE_PARAMETER_HEADER))
            {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL, "RequestHandleQueryPropertyWriteCache: Mode Sense failed\n"));
                status = STATUS_IO_DEVICE_ERROR;
            }
        }
    }

    if (NT_SUCCESS(status))
    {
        // If the length is greater than length indicated by the mode data reset
        // the data to the mode data.
        if (length > (ULONG) (modeData->ModeDataLength + 1))
        {
            length = modeData->ModeDataLength + 1;
        }

        // Look for caching page in the returned mode page data.
        pageData = ModeSenseFindSpecificPage((PCHAR)modeData,
                                             length,
                                             MODE_PAGE_CACHING,
                                             TRUE);

        // Check if valid caching page exists.
        if (pageData == NULL)
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "RequestHandleQueryPropertyWriteCache: Unable to find caching mode page.\n"));

            // Set write cache value as unknown.
            writeCache->WriteCacheEnabled = WriteCacheEnableUnknown;
            writeCache->WriteCacheType = WriteCacheTypeUnknown;
        }
        else
        {
            writeCache->WriteCacheEnabled = pageData->WriteCacheEnable
                                            ? WriteCacheEnabled
                                            : WriteCacheDisabled;

            writeCache->WriteCacheType = pageData->WriteCacheEnable
                                         ? WriteCacheTypeWriteBack
                                         : WriteCacheTypeUnknown;
        }

        // Check write through support.
        if (modeData->DeviceSpecificParameter & MODE_DSP_FUA_SUPPORTED)
        {
            writeCache->WriteThroughSupported = WriteThroughSupported;
        }
        else
        {
            writeCache->WriteThroughSupported = WriteThroughNotSupported;
        }

        // Get the changeable caching mode page and check write cache is changeable.
        RtlZeroMemory(modeData, MODE_PAGE_DATA_SIZE);

        length = DeviceRetrieveModeSenseUsingScratch(deviceExtension,
                                                     (PCHAR) modeData,
                                                     MODE_PAGE_DATA_SIZE,
                                                     MODE_PAGE_CACHING,
                                                     MODE_SENSE_CHANGEABLE_VALUES);

        if (length < sizeof(MODE_PARAMETER_HEADER))
        {
            // Retry the request in case of a check condition.
            length = DeviceRetrieveModeSenseUsingScratch(deviceExtension,
                                                         (PCHAR) modeData,
                                                         MODE_PAGE_DATA_SIZE,
                                                         MODE_PAGE_CACHING,
                                                         MODE_SENSE_CHANGEABLE_VALUES);

            if (length < sizeof(MODE_PARAMETER_HEADER))
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "RequestHandleQueryPropertyWriteCache: Mode Sense failed\n"));

                // If the device fails to return changeable pages, then
                // set the write cache changeable value to unknown.
                writeCache->WriteCacheChangeable = WriteCacheChangeUnknown;
                information = sizeof(STORAGE_WRITE_CACHE_PROPERTY);
            }
        }
    }

    if (NT_SUCCESS(status))
    {
        // If the length is greater than length indicated by the mode data reset
        // the data to the mode data.
        if (length > (ULONG) (modeData->ModeDataLength + 1))
        {
            length = modeData->ModeDataLength + 1;
        }

        // Look for caching page in the returned mode page data.
        pageData = ModeSenseFindSpecificPage((PCHAR)modeData,
                                             length,
                                             MODE_PAGE_CACHING,
                                             TRUE);
        // Check if valid caching page exists.
        if (pageData == NULL)
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "RequestHandleQueryPropertyWriteCache: Unable to find caching mode page.\n"));

            // Set write cache changeable value to unknown.
            writeCache->WriteCacheChangeable = WriteCacheChangeUnknown;
        }
        else
        {
            writeCache->WriteCacheChangeable = pageData->WriteCacheEnable
                                               ? WriteCacheChangeable
                                               : WriteCacheNotChangeable;
        }

        information = sizeof(STORAGE_WRITE_CACHE_PROPERTY);

    }

    FREE_POOL(srb);
    FREE_POOL(modeData);

    RequestCompletion(deviceExtension, Request, status, information);

    return status;
}

NTSTATUS
RequestValidateDvdReadKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_DVD_READ_KEY

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDVD_COPY_PROTECT_KEY   keyParameters = NULL;
    ULONG                   keyLength = 0;

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &keyParameters,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        if(RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(DVD_COPY_PROTECT_KEY))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "DvdDeviceControl: EstablishDriveKey - challenge "
                        "key buffer too small\n"));
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        switch(keyParameters->KeyType)
        {

            case DvdChallengeKey:
            {
                C_ASSERT(sizeof(DVD_COPY_PROTECT_KEY) <= DVD_CHALLENGE_KEY_LENGTH);
                keyLength = DVD_CHALLENGE_KEY_LENGTH;
                break;
            }
            case DvdBusKey1:
            case DvdBusKey2:
            {
                C_ASSERT(sizeof(DVD_COPY_PROTECT_KEY) <= DVD_BUS_KEY_LENGTH);
                keyLength = DVD_BUS_KEY_LENGTH;
                break;
            }
            case DvdTitleKey:
            {
                C_ASSERT(sizeof(DVD_COPY_PROTECT_KEY) <= DVD_TITLE_KEY_LENGTH);
                keyLength = DVD_TITLE_KEY_LENGTH;
                break;
            }
            case DvdAsf:
            {
                C_ASSERT(sizeof(DVD_COPY_PROTECT_KEY) <= DVD_ASF_LENGTH);
                keyLength = DVD_ASF_LENGTH;
                break;
            }
            case DvdDiskKey:
            {
                C_ASSERT(sizeof(DVD_COPY_PROTECT_KEY) <= DVD_DISK_KEY_LENGTH);
                keyLength = DVD_DISK_KEY_LENGTH;
                break;
            }
            case DvdGetRpcKey:
            {
                C_ASSERT(sizeof(DVD_COPY_PROTECT_KEY) <= DVD_RPC_KEY_LENGTH);
                keyLength = DVD_RPC_KEY_LENGTH;
                break;
            }
            default:
            {
                keyLength = sizeof(DVD_COPY_PROTECT_KEY);
                break;
            }
        }

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < keyLength)
        {

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "DvdDeviceControl: EstablishDriveKey - output "
                        "buffer too small\n"));
            status = STATUS_BUFFER_TOO_SMALL;
            *DataLength = keyLength;
        }
        else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength &
                 DeviceExtension->AdapterDescriptor->AlignmentMask)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else if (DeviceExtension->DeviceAdditionalData.DriveDeviceType != FILE_DEVICE_DVD)
        {
            // reject the request if it's not a DVD device.
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    return status;
}


NTSTATUS
RequestValidateDvdEndSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DVD_END_SESSION

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID sessionId = NULL;

    UNREFERENCED_PARAMETER(DeviceExtension);

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &sessionId,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        if(RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
            sizeof(DVD_SESSION_ID))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "DvdDeviceControl: EndSession - input buffer too "
                        "small\n"));
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}


NTSTATUS
RequestValidateAacsEndSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_AACS_END_SESSION

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID sessionId = NULL;
    PCDROM_DATA     cdData = &(DeviceExtension->DeviceAdditionalData);

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &sessionId,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        if (!cdData->Mmc.IsAACS)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else if(RequestParameters.Parameters.DeviceIoControl.InputBufferLength != sizeof(DVD_SESSION_ID))
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}


NTSTATUS
RequestValidateEnableStreaming(
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

    Validates an IOCTL_CDROM_ENABLE_STREAMING request

Arguments:

    Request - request to be handled
    RequestParameters - request parameters
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;

    PCDROM_STREAMING_CONTROL    inputBuffer = NULL;

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
        sizeof(CDROM_STREAMING_CONTROL))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        // Get the request type using CDROM_STREAMING_CONTROL structure
        status = WdfRequestRetrieveInputBuffer(Request,
                                               sizeof(CDROM_STREAMING_CONTROL),
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (inputBuffer->RequestType != CdromStreamingDisable &&
            inputBuffer->RequestType != CdromStreamingEnableForReadOnly &&
            inputBuffer->RequestType != CdromStreamingEnableForWriteOnly &&
            inputBuffer->RequestType != CdromStreamingEnableForReadWrite)
        {
            // Unknown request type
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}


NTSTATUS
RequestValidateSendOpcInformation(
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

    Validates an IOCTL_CDROM_SEND_OPC_INFORMATION request

Arguments:

    Request - request to be handled
    RequestParameters - request parameters
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;

    PCDROM_SIMPLE_OPC_INFO      inputBuffer = NULL;

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
        sizeof(CDROM_SIMPLE_OPC_INFO))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        // Get the request type using CDROM_SIMPLE_OPC_INFO structure
        status = WdfRequestRetrieveInputBuffer(Request,
                                               sizeof(CDROM_SIMPLE_OPC_INFO),
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (inputBuffer->RequestType != SimpleOpcInfo)
        {
            // Unknown request type
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}


NTSTATUS
RequestValidateGetPerformance(
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

    Validates an IOCTL_CDROM_GET_PERFORMANCE request

Arguments:

    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PCDROM_WRITE_SPEED_REQUEST  writeSpeedRequest = NULL;
    PCDROM_PERFORMANCE_REQUEST  performanceRequest = NULL;

    *DataLength = 0;

    // CDROM_WRITE_SPEED_REQUEST is the smallest performance request that we support.
    // We use it to retrieve request type and then check input length more carefully
    // on a per request type basis.
    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
        sizeof(CDROM_WRITE_SPEED_REQUEST))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               sizeof(CDROM_WRITE_SPEED_REQUEST),
                                               (PVOID*)&writeSpeedRequest,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (writeSpeedRequest->RequestType == CdromPerformanceRequest)
        {
            // CDROM_PERFORMANCE_REQUEST is bigger than CDROM_WRITE_SPEED_REQUEST,
            // so we perform more checks and retrieve more bytes through WDF.
            if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_PERFORMANCE_REQUEST))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }
            if (NT_SUCCESS(status))
            {
                status = WdfRequestRetrieveInputBuffer(Request,
                                                       sizeof(CDROM_PERFORMANCE_REQUEST),
                                                       &performanceRequest,
                                                       NULL);
            }

            if (!NT_SUCCESS(status))
            {
                // just pass the status code from above
            }
            // validate all enum-type fields of CDROM_PERFORMANCE_REQUEST
            else if (performanceRequest->PerformanceType != CdromReadPerformance &&
                     performanceRequest->PerformanceType != CdromWritePerformance)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (performanceRequest->Exceptions != CdromNominalPerformance &&
                     performanceRequest->Exceptions != CdromEntirePerformanceList &&
                     performanceRequest->Exceptions != CdromPerformanceExceptionsOnly)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (performanceRequest->Tolerance != Cdrom10Nominal20Exceptions)
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else if (writeSpeedRequest->RequestType == CdromWriteSpeedRequest)
        {
            // No additional checks here: all remaining fields are ignored
            // if RequestType == CdromWriteSpeedRequest.
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    // finally, check output buffer length
    if (NT_SUCCESS(status))
    {
        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
                 sizeof(CDROM_PERFORMANCE_HEADER))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            *DataLength = sizeof(CDROM_PERFORMANCE_HEADER);
        }
        else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength >
            ((USHORT)-1))
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
PCDB
RequestGetScsiPassThroughCdb(
    _In_ PIRP Irp
    )
/*++

Routine Description:

    Get the CDB structure from the SCSI pass through

Arguments:

    Irp - request to be handled

Return Value:

    PCDB

--*/
{
    PCDB                cdb = NULL;
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG               inputBufferLength = 0;
    PVOID               inputBuffer = NULL;
    BOOLEAN             legacyPassThrough = FALSE;

    PAGED_CODE();

    if (((currentIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH) ||
         (currentIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT) ||
         (currentIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_EX) ||
         (currentIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT_EX)) &&
        (Irp->AssociatedIrp.SystemBuffer != NULL))
    {
        inputBufferLength = currentIrpStack->Parameters.DeviceIoControl.InputBufferLength;
        inputBuffer = Irp->AssociatedIrp.SystemBuffer;
        legacyPassThrough = TRUE;

        if ((currentIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_EX) ||
            (currentIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT_EX))
        {
            legacyPassThrough = FALSE;
        }

        //
        //  If this is a 32 bit application running on 64 bit then thunk the
        //  input structures to grab the cdb.
        //

#if BUILD_WOW64_ENABLED && defined(_WIN64)

        if (IoIs32bitProcess(Irp))
        {
            if (legacyPassThrough)
            {
                if (inputBufferLength >= sizeof(SCSI_PASS_THROUGH32))
                {
                    cdb = (PCDB)((PSCSI_PASS_THROUGH32)inputBuffer)->Cdb;
                }
            }
            else
            {
                if (inputBufferLength >= sizeof(SCSI_PASS_THROUGH32_EX))
                {
                    cdb = (PCDB)((PSCSI_PASS_THROUGH32_EX)inputBuffer)->Cdb;
                }
            }

        }
        else

#endif

        {
            if (legacyPassThrough)
            {
                if (inputBufferLength >= sizeof(SCSI_PASS_THROUGH))
                {
                    cdb = (PCDB)((PSCSI_PASS_THROUGH)inputBuffer)->Cdb;
                }
            }
            else
            {
                if (inputBufferLength >= sizeof(SCSI_PASS_THROUGH_EX))
                {
                    cdb = (PCDB)((PSCSI_PASS_THROUGH_EX)inputBuffer)->Cdb;
                }
            }
        }
    }

    return cdb;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleScsiPassThrough(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request
    )
/*++

Routine Description:

   Handle request of IOCTL_SCSI_PASS_THROUGH
                     IOCTL_SCSI_PASS_THROUGH_DIRECT

   The function sets the MinorFunction field of irpStack,
   and pass the request to lower level driver.

Arguments:

    Device - device object
    Request - request to be handled

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_UNSUCCESSFUL;
    PCDROM_DEVICE_EXTENSION     deviceExtension = DeviceGetExtension(Device);
    PIRP                        irp = WdfRequestWdmGetIrp(Request);
    PZERO_POWER_ODD_INFO        zpoddInfo = deviceExtension->ZeroPowerODDInfo;
    PCDB                        cdb = NULL;
    BOOLEAN                     isSoftEject = FALSE;

#if DBG
    PCDROM_REQUEST_CONTEXT      requestContext = RequestGetContext(Request);
#endif


    PAGED_CODE();

#if DBG
    // SPTI is always processed in sync manner.
    NT_ASSERT(requestContext->SyncRequired);
#endif

    if ((zpoddInfo != NULL) &&
        (zpoddInfo->LoadingMechanism == LOADING_MECHANISM_TRAY) &&
        (zpoddInfo->Load == 0))                                         // Drawer
    {
        cdb = RequestGetScsiPassThroughCdb(irp);

        if ((cdb != NULL) &&
            (cdb->AsByte[0] == SCSIOP_START_STOP_UNIT) &&
            (cdb->START_STOP.LoadEject == 1) &&
            (cdb->START_STOP.Start == 0))
        {
            isSoftEject = TRUE;
        }
    }

    WdfRequestFormatRequestUsingCurrentType(Request);

    // Special for SPTI, set the MinorFunction.
    {
        PIO_STACK_LOCATION  nextStack = IoGetNextIrpStackLocation(irp);

        nextStack->MinorFunction = 1;
    }


    status = RequestSend(deviceExtension,
                         Request,
                         deviceExtension->IoTarget,
                         WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
                         NULL);


    if (!NT_SUCCESS(status) &&
        (isSoftEject != FALSE))
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                   "RequestHandleScsiPassThrough: soft eject detected, device marked as active\n"));

        DeviceMarkActive(deviceExtension, TRUE, FALSE);
    }

    RequestCompletion(deviceExtension, Request, status, WdfRequestGetInformation(Request));

    return status;
}

NTSTATUS
RequestHandleMountQueryUniqueId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_MOUNTDEV_QUERY_UNIQUE_ID

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PMOUNTDEV_UNIQUE_ID uniqueId = NULL;

    *DataLength = 0;

    if (!DeviceExtension->MountedDeviceInterfaceName.Buffer)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_UNIQUE_ID))
    {
        *DataLength = sizeof(MOUNTDEV_UNIQUE_ID);
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &uniqueId,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        RtlZeroMemory(uniqueId, RequestParameters.Parameters.DeviceIoControl.OutputBufferLength);

        uniqueId->UniqueIdLength = DeviceExtension->MountedDeviceInterfaceName.Length;

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            (sizeof(USHORT) + DeviceExtension->MountedDeviceInterfaceName.Length))
        {
            *DataLength = sizeof(MOUNTDEV_UNIQUE_ID);
            status = STATUS_BUFFER_OVERFLOW;
        }
    }

    if (NT_SUCCESS(status))
    {
        RtlCopyMemory(uniqueId->UniqueId,
                      DeviceExtension->MountedDeviceInterfaceName.Buffer,
                      uniqueId->UniqueIdLength);

        *DataLength = sizeof(USHORT) + uniqueId->UniqueIdLength;
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
RequestHandleMountQueryDeviceName(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_MOUNTDEV_QUERY_DEVICE_NAME

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PMOUNTDEV_NAME  name = NULL;

    *DataLength = 0;

    NT_ASSERT(DeviceExtension->DeviceName.Buffer);

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_NAME))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        *DataLength = sizeof(MOUNTDEV_NAME);
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &name,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        RtlZeroMemory(name, RequestParameters.Parameters.DeviceIoControl.OutputBufferLength);
        name->NameLength = DeviceExtension->DeviceName.Length;

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            (sizeof(USHORT) + DeviceExtension->DeviceName.Length))
        {
            status = STATUS_BUFFER_OVERFLOW;
            *DataLength = sizeof(MOUNTDEV_NAME);
        }
    }

    if (NT_SUCCESS(status))
    {
        RtlCopyMemory(name->Name,
                      DeviceExtension->DeviceName.Buffer,
                      name->NameLength);

        status = STATUS_SUCCESS;
        *DataLength = sizeof(USHORT) + name->NameLength;
    }

    return status;
}

NTSTATUS
RequestHandleMountQuerySuggestedLinkName(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;

    PMOUNTDEV_SUGGESTED_LINK_NAME suggestedName = NULL;

    WCHAR                    driveLetterNameBuffer[10] = {0};
    RTL_QUERY_REGISTRY_TABLE queryTable[2] = {0};
    PWSTR                    valueName = NULL;
    UNICODE_STRING           driveLetterName = {0};

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_SUGGESTED_LINK_NAME))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        *DataLength = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
    }

    if (NT_SUCCESS(status))
    {
        valueName = ExAllocatePoolWithTag(PagedPool,
                                          DeviceExtension->DeviceName.Length + sizeof(WCHAR),
                                          CDROM_TAG_STRINGS);
        if (valueName == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(status))
    {
        RtlCopyMemory(valueName,
                      DeviceExtension->DeviceName.Buffer,
                      DeviceExtension->DeviceName.Length);
        valueName[DeviceExtension->DeviceName.Length/sizeof(WCHAR)] = 0;

        driveLetterName.Buffer = driveLetterNameBuffer;
        driveLetterName.MaximumLength = sizeof(driveLetterNameBuffer);
        driveLetterName.Length = 0;

        queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
        queryTable[0].Name = valueName;
        queryTable[0].EntryContext = &driveLetterName;
        queryTable[0].DefaultType = (REG_SZ << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_NONE;

        status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        L"\\Registry\\Machine\\System\\DISK", // why hard coded?
                                        queryTable, NULL, NULL);
    }

    if (NT_SUCCESS(status))
    {
        if ((driveLetterName.Length == 4) &&
            (driveLetterName.Buffer[0] == '%') &&
            (driveLetterName.Buffer[1] == ':'))
        {
            driveLetterName.Buffer[0] = 0xFF;
        }
        else if ((driveLetterName.Length != 4) ||
                 (driveLetterName.Buffer[0] < FirstDriveLetter) ||
                 (driveLetterName.Buffer[0] > LastDriveLetter) ||
                 (driveLetterName.Buffer[1] != ':'))
        {
            status = STATUS_NOT_FOUND;
        }
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &suggestedName,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        RtlZeroMemory(suggestedName, RequestParameters.Parameters.DeviceIoControl.OutputBufferLength);
        suggestedName->UseOnlyIfThereAreNoOtherLinks = TRUE;
        suggestedName->NameLength = 28;

        *DataLength = FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name) + 28;

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < *DataLength)
        {
            *DataLength = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
            status = STATUS_BUFFER_OVERFLOW;
        }
    }

    if (NT_SUCCESS(status))
    {
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                               L"\\Registry\\Machine\\System\\DISK",
                               valueName);

        RtlCopyMemory(suggestedName->Name, L"\\DosDevices\\", 24);
        suggestedName->Name[12] = driveLetterName.Buffer[0];
        suggestedName->Name[13] = ':';
    }

    FREE_POOL(valueName);

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleReadTOC(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_READ_TOC
                     IOCTL_CDROM_GET_LAST_SESSION

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    VOID*    outputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    if (DeviceIsPlayActive(DeviceExtension->Device))
    {
        status = STATUS_DEVICE_BUSY;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    // handle the request
    if (NT_SUCCESS(status))
    {
        size_t  transferSize;
        CDB     cdb;

        transferSize = min(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength, sizeof(CDROM_TOC));

        RtlZeroMemory(outputBuffer, transferSize);

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        if (RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_GET_LAST_SESSION)
        {
            // Set format to return first and last session numbers.
            cdb.READ_TOC.Format2 = CDROM_READ_TOC_EX_FORMAT_SESSION;
        }
        else
        {
            // Use MSF addressing
            cdb.READ_TOC.Msf = 1;
        }

        cdb.READ_TOC.OperationCode = SCSIOP_READ_TOC;
        cdb.READ_TOC.AllocationLength[0] = (UCHAR)(transferSize >> 8);
        cdb.READ_TOC.AllocationLength[1] = (UCHAR)(transferSize & 0xFF);

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, (ULONG)transferSize, TRUE, &cdb, 10);

        if (NT_SUCCESS(status))
        {
            *DataLength = DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength;
            RtlCopyMemory(outputBuffer,
                          DeviceExtension->ScratchContext.ScratchBuffer,
                          *DataLength);
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleReadTocEx(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_READ_TOC_EX

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCDROM_READ_TOC_EX  inputBuffer = NULL;
    VOID*               outputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    if (DeviceIsPlayActive(DeviceExtension->Device))
    {
        status = STATUS_DEVICE_BUSY;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    // handle the request
    if (NT_SUCCESS(status))
    {
        size_t  transferSize;
        CDB     cdb;

        transferSize = min(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength, MAXUSHORT);
        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.READ_TOC.OperationCode = SCSIOP_READ_TOC;
        cdb.READ_TOC.Msf = inputBuffer->Msf;
        cdb.READ_TOC.Format2 = inputBuffer->Format;
        cdb.READ_TOC.StartingTrack = inputBuffer->SessionTrack;
        cdb.READ_TOC.AllocationLength[0] = (UCHAR)(transferSize >> 8);
        cdb.READ_TOC.AllocationLength[1] = (UCHAR)(transferSize & 0xFF);

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, (ULONG)transferSize, TRUE, &cdb, 10);

        if (NT_SUCCESS(status))
        {
            if (DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength < MINIMUM_CDROM_READ_TOC_EX_SIZE)
            {
                *DataLength = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                *DataLength = DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength;
                RtlCopyMemory(outputBuffer,
                              DeviceExtension->ScratchContext.ScratchBuffer,
                              *DataLength);
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
VOID
GetConfigurationDataConversionTypeAllToTypeOne(
    _In_    FEATURE_NUMBER       RequestedFeature,
    _In_    PSCSI_REQUEST_BLOCK  Srb,
    _Out_   size_t *             DataLength
    )
/*++

Routine Description:

    Some CDROM devices do not handle the GET CONFIGURATION commands with
    TYPE ONE request. The command will time out causing a bus reset.
    To avoid this problem we set a device flag during start device if the device
    fails a TYPE ONE request. If this flag is set the TYPE ONE requests will be
    tried as TYPE ALL request and the data will be converted to TYPE ONE format
    in this routine.

Arguments:

    RequestedFeature - device context
    Srb - request to be handled
    DataLength - transfer data length

Return Value:

    NTSTATUS

--*/
{
    PFEATURE_HEADER     featureHeader = NULL;
    FEATURE_NUMBER      thisFeature;
    ULONG               totalLength = 0;
    ULONG               featureLength = 0;
    ULONG               headerLength = 0;

    PGET_CONFIGURATION_HEADER   header = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    if (Srb->DataTransferLength < RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength))
    {
        // do not have valid data.
        return;
    }

    // Calculate the length of valid data available in the
    // capabilities buffer from the DataLength field
    header = (PGET_CONFIGURATION_HEADER) Srb->DataBuffer;
    REVERSE_BYTES(&totalLength, header->DataLength);

    totalLength += RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength);

    // Make sure the we have enough data in the SRB
    totalLength = min(totalLength, Srb->DataTransferLength);

    // If we have received enough data from the device
    // check for the given feature.
    if (totalLength >= (sizeof(GET_CONFIGURATION_HEADER) + sizeof(FEATURE_HEADER)))
    {
        // Feature information is present. Verify the feature.
        featureHeader = (PFEATURE_HEADER)((PUCHAR)Srb->DataBuffer + sizeof(GET_CONFIGURATION_HEADER));

        thisFeature  = (featureHeader->FeatureCode[0] << 8) | (featureHeader->FeatureCode[1]);

        if (thisFeature == RequestedFeature)
        {
            // Calculate the feature length
            featureLength = sizeof(FEATURE_HEADER) + featureHeader->AdditionalLength;
        }
    }

    // Calculate the total size
    totalLength = sizeof(GET_CONFIGURATION_HEADER) + featureLength;

    headerLength = totalLength -
        RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength);

    REVERSE_BYTES(header->DataLength, &headerLength);

    *DataLength = totalLength;

    return;
}

_IRQL_requires_max_(APC_LEVEL)
VOID
GetConfigurationDataSynthesize(
    _In_reads_bytes_(InputBufferSize)    PVOID           InputBuffer,
    _In_                            ULONG           InputBufferSize,
    _Out_writes_bytes_(OutputBufferSize)  PVOID           OutputBuffer,
    _In_                            size_t          OutputBufferSize,
    _In_                            FEATURE_NUMBER  StartingFeature,
    _In_                            ULONG           RequestType,
    _Out_                           size_t *        DataLength
    )
/*++

Routine Description:

    Get Configuration is a frequently used command, and we don't want it to wake
    up the device in case it is in Zero Power state. Before entering Zero Power state,
    the complete response of the command is saved in cache, and since the response
    is always the same in case there is no media, we can synthesize the response
    based on the user request.

Arguments:

    InputBuffer - buffer containing cached command response
    InputBufferSize - size of above buffer
    OutputBuffer - buffer to fill in result
    OutputBufferSize - size of above buffer
    StartingFeature - requested Starting Feature Number
    RequestType - requested Request Type
    DataLength - transfer data length

Return Value:

--*/
{
    PFEATURE_HEADER     featureHeader = NULL;
    ULONG               validLength = 0;
    ULONG               featureLength = 0;
    ULONG               headerLength = 0;
    PUCHAR              buffer = NULL;
    ULONG               bytesRemaining = 0;
    FEATURE_NUMBER      featureCode = 0;
    BOOLEAN             shouldCopy = FALSE;
    size_t              copyLength = 0;
    size_t              transferedLength = 0;
    size_t              requiredLength = 0;

    PGET_CONFIGURATION_HEADER   header = NULL;

    PAGED_CODE ();

    if (InputBufferSize < sizeof (GET_CONFIGURATION_HEADER))
    {
        // do not have valid data.
        *DataLength = 0;

        return;
    }

    // Calculate the length of valid data available in the
    // capabilities buffer from the DataLength field
    header = (PGET_CONFIGURATION_HEADER) InputBuffer;
    REVERSE_BYTES(&validLength, header->DataLength);

    validLength += RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength);

    // Make sure we have enough data
    validLength = min(validLength, InputBufferSize);

    // Copy the header first
    copyLength = min(OutputBufferSize, sizeof (GET_CONFIGURATION_HEADER));

    RtlMoveMemory(OutputBuffer,
                  InputBuffer,
                  copyLength);

    transferedLength = copyLength;
    requiredLength = sizeof (GET_CONFIGURATION_HEADER);

    if (validLength > sizeof (GET_CONFIGURATION_HEADER))
    {
        buffer = header->Data;
        bytesRemaining = validLength - sizeof (GET_CONFIGURATION_HEADER);

        // Ignore incomplete feature descriptor
        while (bytesRemaining >= sizeof (FEATURE_HEADER))
        {
            featureHeader = (PFEATURE_HEADER) buffer;
            shouldCopy = FALSE;

            featureCode = (featureHeader->FeatureCode[0] << 8) | (featureHeader->FeatureCode[1]);
            featureLength = sizeof (FEATURE_HEADER) + featureHeader->AdditionalLength;

            if (featureCode >= StartingFeature)
            {
                switch (RequestType) {

                case SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL:

                    shouldCopy = TRUE;
                    break;

                case SCSI_GET_CONFIGURATION_REQUEST_TYPE_CURRENT:

                    if (featureHeader->Current)
                    {
                        shouldCopy = TRUE;
                    }
                    break;

                case SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE:

                    if (featureCode == StartingFeature)
                    {
                        shouldCopy = TRUE;
                    }
                    break;

                default:

                    break;
                }
            }

            if (shouldCopy != FALSE)
            {
                copyLength = min(featureLength, bytesRemaining);
                copyLength = min(copyLength, OutputBufferSize - transferedLength);

                RtlMoveMemory((PUCHAR) OutputBuffer + transferedLength,
                              buffer,
                              copyLength);

                transferedLength += copyLength;
                requiredLength += featureLength;
            }

            buffer += min(featureLength, bytesRemaining);
            bytesRemaining -= min(featureLength, bytesRemaining);
        }
    }

    // Adjust Data Length field in header
    if (transferedLength >= RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength))
    {
        headerLength = (ULONG) requiredLength - RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength);

        header = (PGET_CONFIGURATION_HEADER) OutputBuffer;
        REVERSE_BYTES(header->DataLength, &headerLength);
    }

    *DataLength = transferedLength;

    return;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleGetConfiguration(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_GET_CONFIGURATION

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PCDROM_DATA                     cdData = &(DeviceExtension->DeviceAdditionalData);
    PGET_CONFIGURATION_IOCTL_INPUT  inputBuffer = NULL;
    VOID*                           outputBuffer = NULL;
    size_t                          transferByteCount = 0;
    PZERO_POWER_ODD_INFO            zpoddInfo = DeviceExtension->ZeroPowerODDInfo;
    BOOLEAN                         inZeroPowerState = FALSE;

    PAGED_CODE ();

    *DataLength = 0;

    //
    if (!cdData->Mmc.IsMmc)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &inputBuffer,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        // If device is Zero Power state, there should be no media in device, thus we can synthesize the response
        // from our cache. Avoid waking up the device in this case.
        if ((zpoddInfo != NULL) &&
            (zpoddInfo->InZeroPowerState != FALSE))
        {
            inZeroPowerState = TRUE;
        }

        if ((inZeroPowerState == FALSE) ||
            (zpoddInfo->GetConfigurationBuffer == NULL))
        {
            CDB cdb;

            //The maximum number of bytes that a Drive may return
            //to describe its Features in one GET CONFIGURATION Command is 65,534
            transferByteCount = min(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength, (MAXUSHORT - 1));

            // If this is a TYPE ONE request and if this device can't handle this
            // request, then we need to send TYPE ALL request to the device and
            // convert the data in the completion routine. If required allocate a big
            // buffer to get both configuration and feature header.
            if ((inputBuffer->RequestType == SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE) &&
                TEST_FLAG(cdData->HackFlags, CDROM_HACK_BAD_TYPE_ONE_GET_CONFIG))
            {
                transferByteCount = max(transferByteCount,
                                        sizeof(GET_CONFIGURATION_HEADER) + sizeof(FEATURE_HEADER));
            }

            ScratchBuffer_BeginUse(DeviceExtension);

            RtlZeroMemory(&cdb, sizeof(CDB));
            // Set up the CDB
            cdb.GET_CONFIGURATION.OperationCode = SCSIOP_GET_CONFIGURATION;
            cdb.GET_CONFIGURATION.AllocationLength[0] = (UCHAR)(transferByteCount >> 8);
            cdb.GET_CONFIGURATION.AllocationLength[1] = (UCHAR)(transferByteCount & 0xff);

            cdb.GET_CONFIGURATION.StartingFeature[0] = (UCHAR)(inputBuffer->Feature >> 8);
            cdb.GET_CONFIGURATION.StartingFeature[1] = (UCHAR)(inputBuffer->Feature & 0xff);
            cdb.GET_CONFIGURATION.RequestType        = (UCHAR)(inputBuffer->RequestType);

            // If the device does not support TYPE ONE get configuration commands
            // then change the request type to TYPE ALL. Convert the returned data to
            // TYPE ONE format in the completion routine.
            if ((inputBuffer->RequestType == SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE) &&
                TEST_FLAG(cdData->HackFlags, CDROM_HACK_BAD_TYPE_ONE_GET_CONFIG))
            {

                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                           "DeviceHandleGetConfiguration: Changing TYPE_ONE Get Config to TYPE_ALL\n"));
                cdb.GET_CONFIGURATION.RequestType = SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL;
            }

            status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, (ULONG)transferByteCount, TRUE, &cdb, 12);

            if (NT_SUCCESS(status))
            {
                if ((inputBuffer->RequestType == SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE) &&
                    TEST_FLAG(cdData->HackFlags, CDROM_HACK_BAD_TYPE_ONE_GET_CONFIG))
                {

                    if (DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength < sizeof(GET_CONFIGURATION_HEADER))
                    {
                        // Not enough data to calculate the data length.
                        // So assume feature is not present
                        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "DeviceHandleGetConfiguration: No get config header!\n"));
                        *DataLength = 0;
                        status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                    else
                    {
                        //Some CDROM devices do not handle the GET CONFIGURATION commands with
                        //TYPE ONE request. The command will time out causing a bus reset.
                        //To avoid this problem we set a device flag during start device if the device
                        //fails a TYPE ONE request. If this flag is set the TYPE ONE requests will be
                        //tried as TYPE ALL request and the data will be converted to TYPE ONE format
                        //in this routine.
                        GetConfigurationDataConversionTypeAllToTypeOne(inputBuffer->Feature, DeviceExtension->ScratchContext.ScratchSrb, DataLength);
                        *DataLength = min(*DataLength, RequestParameters.Parameters.DeviceIoControl.OutputBufferLength);
                    }
                }
                else
                {
                    *DataLength = DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength;
                }

                // copy data to output buffer
                if (NT_SUCCESS(status))
                {
                    RtlMoveMemory(outputBuffer,
                                  DeviceExtension->ScratchContext.ScratchBuffer,
                                  *DataLength);
                }
            }

            ScratchBuffer_EndUse(DeviceExtension);
        }
        else
        {
            // We are in Zero Power state, and our cached response is available.
            // Synthesize the requested data.
            GetConfigurationDataSynthesize(zpoddInfo->GetConfigurationBuffer,
                                           zpoddInfo->GetConfigurationBufferSize,
                                           outputBuffer,
                                           RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                           inputBuffer->Feature,
                                           inputBuffer->RequestType,
                                           DataLength
                                           );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandleGetDriveGeometry(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DISK_GET_LENGTH_INFO
                     IOCTL_DISK_GET_DRIVE_GEOMETRY
                     IOCTL_DISK_GET_DRIVE_GEOMETRY_EX
                     IOCTL_CDROM_GET_DRIVE_GEOMETRY
                     IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX
                     IOCTL_STORAGE_READ_CAPACITY

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    VOID*               outputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveOutputBuffer(Request,
                                            RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                            &outputBuffer,
                                            NULL);

    // Issue ReadCapacity to update device extension
    // with information for current media.
    if (NT_SUCCESS(status))
    {
        status = MediaReadCapacity(DeviceExtension->Device);
    }

    if (NT_SUCCESS(status))
    {
        switch(RequestParameters.Parameters.DeviceIoControl.IoControlCode)
        {
            case IOCTL_DISK_GET_LENGTH_INFO:
            {
                PGET_LENGTH_INFORMATION lengthInfo = (PGET_LENGTH_INFORMATION)outputBuffer;

                lengthInfo->Length = DeviceExtension->PartitionLength;
                *DataLength = sizeof(GET_LENGTH_INFORMATION);
                break;
            }
            case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
            {
                PDISK_GEOMETRY geometry = (PDISK_GEOMETRY)outputBuffer;

                *geometry = DeviceExtension->DiskGeometry;
                *DataLength = sizeof(DISK_GEOMETRY);
                break;
            }
            case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
            case IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX:
            {
                PDISK_GEOMETRY_EX geometryEx = (PDISK_GEOMETRY_EX)outputBuffer;

                geometryEx->DiskSize = DeviceExtension->PartitionLength;
                geometryEx->Geometry = DeviceExtension->DiskGeometry;
                *DataLength = FIELD_OFFSET(DISK_GEOMETRY_EX, Data);
                break;
            }
            case IOCTL_STORAGE_READ_CAPACITY:
            {
                PSTORAGE_READ_CAPACITY readCapacity = (PSTORAGE_READ_CAPACITY)outputBuffer;

                readCapacity->Version = sizeof(STORAGE_READ_CAPACITY);
                readCapacity->Size = sizeof(STORAGE_READ_CAPACITY);

                readCapacity->BlockLength = DeviceExtension->DiskGeometry.BytesPerSector;
                if (readCapacity->BlockLength > 0)
                {
                    readCapacity->NumberOfBlocks.QuadPart = DeviceExtension->PartitionLength.QuadPart/readCapacity->BlockLength;
                }
                else
                {
                    readCapacity->NumberOfBlocks.QuadPart = 0;
                }

                readCapacity->DiskLength = DeviceExtension->PartitionLength;

                *DataLength = sizeof(STORAGE_READ_CAPACITY);
                break;
            }
            default:
            {
                NT_ASSERT(FALSE);
                break;
            }
        } // end of switch()
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleDiskVerify(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DISK_VERIFY

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCDROM_DATA         cdData = &(DeviceExtension->DeviceAdditionalData);
    PVERIFY_INFORMATION verifyInfo = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    if (!cdData->Mmc.WriteAllowed)
    {
        status = STATUS_MEDIA_WRITE_PROTECTED;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &verifyInfo,
                                               NULL);
    }

    // handle the request
    if (NT_SUCCESS(status))
    {
        LARGE_INTEGER byteOffset = {0};

        // Add disk offset to starting sector.
        byteOffset.QuadPart = DeviceExtension->StartingOffset.QuadPart +
                              verifyInfo->StartingOffset.QuadPart;

        // prevent overflow returning success but only validating small area
        if (((DeviceExtension->StartingOffset.QuadPart + verifyInfo->StartingOffset.QuadPart) < DeviceExtension->StartingOffset.QuadPart) ||
            ((verifyInfo->Length >> DeviceExtension->SectorShift) > MAXUSHORT) ||
            ((byteOffset.QuadPart >> DeviceExtension->SectorShift) > MAXULONG) )
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            ULONG   transferSize = 0;
            ULONG   timeoutValue = 0;
            CDB     cdb;
            ULONG   sectorOffset;
            USHORT  sectorCount;

            ScratchBuffer_BeginUse(DeviceExtension);

            // Convert byte offset to sector offset.
            sectorOffset = (ULONG)(byteOffset.QuadPart >> DeviceExtension->SectorShift);

            // Convert ULONG byte count to USHORT sector count.
            sectorCount = (USHORT)(verifyInfo->Length >> DeviceExtension->SectorShift);

            RtlZeroMemory(&cdb, sizeof(CDB));
            // Set up the CDB
            cdb.CDB10.OperationCode = SCSIOP_VERIFY;

            // Move little endian values into CDB in big endian format.
            cdb.CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&sectorOffset)->Byte3;
            cdb.CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&sectorOffset)->Byte2;
            cdb.CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&sectorOffset)->Byte1;
            cdb.CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&sectorOffset)->Byte0;

            cdb.CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&sectorCount)->Byte1;
            cdb.CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&sectorCount)->Byte0;

            // The verify command is used by the NT FORMAT utility and
            // requests are sent down for 5% of the volume size. The
            // request timeout value is calculated based on the number of
            // sectors verified.
            if (sectorCount != 0)
            {
                // sectorCount is a USHORT, so no overflow here...
                timeoutValue = TimeOutValueGetCapValue(DeviceExtension->TimeOutValue, ((sectorCount + 128) / 128));
            }

            status = ScratchBuffer_ExecuteCdbEx(DeviceExtension, Request, transferSize, FALSE, &cdb, 10, timeoutValue);

            // nothing to do after the command finishes.
            ScratchBuffer_EndUse(DeviceExtension);
        }
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleCheckVerify(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_CHECK_VERIFY2
                     IOCTL_STORAGE_CHECK_VERIFY

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;

    PAGED_CODE ();

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        ULONG   transferSize = 0;
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

        status = ScratchBuffer_ExecuteCdbEx(DeviceExtension, Request, transferSize, FALSE, &cdb, 6, CDROM_TEST_UNIT_READY_TIMEOUT);

        if (NT_SUCCESS(status))
        {
            if((RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_CHECK_VERIFY) &&
               (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength))
            {
                PULONG outputBuffer = NULL;
                status = WdfRequestRetrieveOutputBuffer(Request,
                                                        RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                        &outputBuffer,
                                                        NULL);

                if (outputBuffer != NULL)
                {
                    *outputBuffer = DeviceExtension->MediaChangeCount;
                    *DataLength = sizeof(ULONG);
                }
            }
            else
            {
                *DataLength = 0;
            }
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleFakePartitionInfo(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DISK_GET_DRIVE_LAYOUT
                     IOCTL_DISK_GET_DRIVE_LAYOUT_EX
                     IOCTL_DISK_GET_PARTITION_INFO
                     IOCTL_DISK_GET_PARTITION_INFO_EX

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    VOID*       outputBuffer = NULL;
    ULONG       ioctl = RequestParameters.Parameters.DeviceIoControl.IoControlCode;

    PAGED_CODE ();

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        if ((ioctl != IOCTL_DISK_GET_DRIVE_LAYOUT) &&
            (ioctl != IOCTL_DISK_GET_DRIVE_LAYOUT_EX) &&
            (ioctl != IOCTL_DISK_GET_PARTITION_INFO) &&
            (ioctl != IOCTL_DISK_GET_PARTITION_INFO_EX))
        {
            status = STATUS_INTERNAL_ERROR;
        }
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    // handle the request
    if (NT_SUCCESS(status))
    {
        switch (ioctl)
        {
        case IOCTL_DISK_GET_DRIVE_LAYOUT:
            *DataLength = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry[1]);
            RtlZeroMemory(outputBuffer, *DataLength);
            break;
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
            *DataLength = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[1]);
            RtlZeroMemory(outputBuffer, *DataLength);
            break;
        case IOCTL_DISK_GET_PARTITION_INFO:
            *DataLength = sizeof(PARTITION_INFORMATION);
            RtlZeroMemory(outputBuffer, *DataLength);
            break;
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
            *DataLength = sizeof(PARTITION_INFORMATION_EX);
            RtlZeroMemory(outputBuffer, *DataLength);
            break;
        default:
            NT_ASSERT(!"Invalid ioctl should not have reached this point\n");
            break;
        }

        // if we are getting the drive layout, then we need to start by
        // adding some of the non-partition stuff that says we have
        // exactly one partition available.
        if (ioctl == IOCTL_DISK_GET_DRIVE_LAYOUT)
        {
            PDRIVE_LAYOUT_INFORMATION layout;
            layout = (PDRIVE_LAYOUT_INFORMATION)outputBuffer;
            layout->PartitionCount = 1;
            layout->Signature = 1;
            outputBuffer = (PVOID)(layout->PartitionEntry);
            ioctl = IOCTL_DISK_GET_PARTITION_INFO;
        }
        else if (ioctl == IOCTL_DISK_GET_DRIVE_LAYOUT_EX)
        {
            PDRIVE_LAYOUT_INFORMATION_EX layoutEx;
            layoutEx = (PDRIVE_LAYOUT_INFORMATION_EX)outputBuffer;
            layoutEx->PartitionStyle = PARTITION_STYLE_MBR;
            layoutEx->PartitionCount = 1;
            layoutEx->Mbr.Signature = 1;
            outputBuffer = (PVOID)(layoutEx->PartitionEntry);
            ioctl = IOCTL_DISK_GET_PARTITION_INFO_EX;
        }

        // NOTE: the local var 'ioctl' is now modified to either EX or
        // non-EX version. the local var 'systemBuffer' is now pointing
        // to the partition information structure.
        if (ioctl == IOCTL_DISK_GET_PARTITION_INFO)
        {
            PPARTITION_INFORMATION partitionInfo;
            partitionInfo = (PPARTITION_INFORMATION)outputBuffer;
            partitionInfo->RewritePartition = FALSE;
            partitionInfo->RecognizedPartition = TRUE;
            partitionInfo->PartitionType = PARTITION_FAT32;
            partitionInfo->BootIndicator = FALSE;
            partitionInfo->HiddenSectors = 0;
            partitionInfo->StartingOffset.QuadPart = 0;
            partitionInfo->PartitionLength = DeviceExtension->PartitionLength;
            partitionInfo->PartitionNumber = 0;
        }
        else
        {
            PPARTITION_INFORMATION_EX partitionInfo;
            partitionInfo = (PPARTITION_INFORMATION_EX)outputBuffer;
            partitionInfo->PartitionStyle = PARTITION_STYLE_MBR;
            partitionInfo->RewritePartition = FALSE;
            partitionInfo->Mbr.RecognizedPartition = TRUE;
            partitionInfo->Mbr.PartitionType = PARTITION_FAT32;
            partitionInfo->Mbr.BootIndicator = FALSE;
            partitionInfo->Mbr.HiddenSectors = 0;
            partitionInfo->StartingOffset.QuadPart = 0;
            partitionInfo->PartitionLength = DeviceExtension->PartitionLength;
            partitionInfo->PartitionNumber = 0;
        }
    }

    return status;
}

NTSTATUS
RequestHandleGetDeviceNumber(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_GET_DEVICE_NUMBER

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    *DataLength = 0;

    if(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength >=
       sizeof(STORAGE_DEVICE_NUMBER))
    {
        PSTORAGE_DEVICE_NUMBER deviceNumber = NULL;
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &deviceNumber,
                                                NULL);
        if (NT_SUCCESS(status))
        {
            deviceNumber->DeviceType = DeviceExtension->DeviceObject->DeviceType;
            deviceNumber->DeviceNumber = DeviceExtension->DeviceNumber;
            deviceNumber->PartitionNumber = (ULONG)-1; // legacy reason, return (-1) for this IOCTL.

            status = STATUS_SUCCESS;
            *DataLength = sizeof(STORAGE_DEVICE_NUMBER);
        }
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
        *DataLength = sizeof(STORAGE_DEVICE_NUMBER);
    }

    return status;
}

NTSTATUS
RequestHandleGetHotPlugInfo(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_GET_HOTPLUG_INFO

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    *DataLength = 0;

    if(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength >=
       sizeof(STORAGE_HOTPLUG_INFO))
    {
        PSTORAGE_HOTPLUG_INFO info = NULL;
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &info,
                                                NULL);
        if (NT_SUCCESS(status))
        {
            *info = DeviceExtension->PrivateFdoData->HotplugInfo;

            status = STATUS_SUCCESS;
            *DataLength = sizeof(STORAGE_HOTPLUG_INFO);
        }
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
        *DataLength = sizeof(STORAGE_HOTPLUG_INFO);
    }

    return status;
}

NTSTATUS
RequestHandleSetHotPlugInfo(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_SET_HOTPLUG_INFO

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PSTORAGE_HOTPLUG_INFO   info = NULL;

    *DataLength = 0;

    if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
        sizeof(STORAGE_HOTPLUG_INFO))
    {
        // Indicate unsuccessful status and no data transferred.
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &info,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        if (info->Size != DeviceExtension->PrivateFdoData->HotplugInfo.Size)
        {
            status = STATUS_INVALID_PARAMETER_1;
        }

        if (info->MediaRemovable != DeviceExtension->PrivateFdoData->HotplugInfo.MediaRemovable)
        {
            status = STATUS_INVALID_PARAMETER_2;
        }

        if (info->MediaHotplug != DeviceExtension->PrivateFdoData->HotplugInfo.MediaHotplug)
        {
            status = STATUS_INVALID_PARAMETER_3;
        }

        if (info->WriteCacheEnableOverride != DeviceExtension->PrivateFdoData->HotplugInfo.WriteCacheEnableOverride)
        {
            status = STATUS_INVALID_PARAMETER_5;
        }
    }

    if (NT_SUCCESS(status))
    {
        DeviceExtension->PrivateFdoData->HotplugInfo.DeviceHotplug = info->DeviceHotplug;

        // Store the user-defined override in the registry
        DeviceSetParameter(DeviceExtension,
                           CLASSP_REG_SUBKEY_NAME,
                           CLASSP_REG_REMOVAL_POLICY_VALUE_NAME,
                           (info->DeviceHotplug) ? RemovalPolicyExpectSurpriseRemoval : RemovalPolicyExpectOrderlyRemoval);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleEventNotification(
    _In_      PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_opt_  WDFREQUEST               Request,
    _In_opt_  PWDF_REQUEST_PARAMETERS  RequestParameters,
    _Out_     size_t *                 DataLength
    )
/*++

Routine Description:

    This routine handles the process of IOCTL_STORAGE_EVENT_NOTIFICATION

Arguments:

    DeviceExtension - device context

    Request - request to be handled

    RequestParameters - request parameter

    DataLength - data transferred

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS                     status = STATUS_SUCCESS;
    PMEDIA_CHANGE_DETECTION_INFO info = NULL;
    LONG                         requestInUse;
    PSTORAGE_EVENT_NOTIFICATION  eventBuffer = NULL;

    *DataLength = 0;

    info = DeviceExtension->MediaChangeDetectionInfo;

    // Since AN is ASYNCHRONOUS and can happen at any time,
    // make certain not to do anything before properly initialized.
    if ((!DeviceExtension->IsInitialized) || (info == NULL))
    {
        status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(status) && (Request != NULL) && (RequestParameters != NULL)) {

        //
        // Validate IOCTL parameters
        //
        if (RequestParameters->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(STORAGE_EVENT_NOTIFICATION)) {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // Check for an supported event
        //
        if (NT_SUCCESS(status)) {
            status = WdfRequestRetrieveInputBuffer(Request,
                                                   RequestParameters->Parameters.DeviceIoControl.InputBufferLength,
                                                   &eventBuffer,
                                                   NULL);
            if (NT_SUCCESS(status)) {
                if ((eventBuffer->Version != STORAGE_EVENT_NOTIFICATION_VERSION_V1) ||
                    (eventBuffer->Size != sizeof(STORAGE_EVENT_NOTIFICATION))) {
                    status = STATUS_INVALID_PARAMETER;
                } else if ((eventBuffer->Events &
                           (STORAGE_EVENT_MEDIA_STATUS | STORAGE_EVENT_DEVICE_STATUS | STORAGE_EVENT_DEVICE_OPERATION)) == 0) {
                    status = STATUS_NOT_SUPPORTED;
                }
            }
        }

    }

    if (NT_SUCCESS(status))
    {
        if (info->MediaChangeDetectionDisableCount != 0)
        {
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_MCN,
                       "RequestHandleEventNotification: device %p has detection disabled \n",
                       DeviceExtension->DeviceObject));
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status))
    {
        // if the request is not in use, mark it as such.
        requestInUse = InterlockedCompareExchange((PLONG)&info->MediaChangeRequestInUse, 1, 0);

        if (requestInUse != 0)
        {
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status))
    {
        // The last MCN finished. ok to issue the new one.
        RequestSetupMcnSyncIrp(DeviceExtension);

        // The irp will go into KMDF framework and a request will be created there to represent it.
        IoCallDriver(DeviceExtension->DeviceObject, info->MediaChangeSyncIrp);
    }

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandleEjectionControl(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_MEDIA_REMOVAL
                     IOCTL_STORAGE_EJECTION_CONTROL

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PPREVENT_MEDIA_REMOVAL  mediaRemoval = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    if(RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
       sizeof(PREVENT_MEDIA_REMOVAL))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               &mediaRemoval,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = PerformEjectionControl(DeviceExtension,
                                        Request,
                                        ((RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_EJECTION_CONTROL)
                                          ? SecureMediaLock
                                          : SimpleMediaLock),
                                        mediaRemoval->PreventMediaRemoval);
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleEnableStreaming(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

    Handles an IOCTL_CDROM_ENABLE_STREAMING request

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    DataLength - transferred data length

Notes:

    This IOCTL is serialized because it changes read/write
    behavior and we want to make sure that all previous
    reads/writes have been completed before we change the
    behavior.

Return Value:

    NTSTATUS

--*/
{
    PCDROM_DATA                 cdData = &(DeviceExtension->DeviceAdditionalData);
    PCDROM_PRIVATE_FDO_DATA     fdoData = DeviceExtension->PrivateFdoData;

    WDFFILEOBJECT               fileObject = NULL;
    PFILE_OBJECT_CONTEXT        fileObjectContext = NULL;

    NTSTATUS                    status = STATUS_SUCCESS;
    PCDROM_STREAMING_CONTROL    inputBuffer = NULL;

    BOOLEAN                     enforceStreamingRead = FALSE;
    BOOLEAN                     enforceStreamingWrite = FALSE;
    BOOLEAN                     streamingReadSupported, streamingWriteSupported;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           sizeof(CDROM_STREAMING_CONTROL),
                                           &inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        // get file object context
        fileObject = WdfRequestGetFileObject(Request);
        if (fileObject != NULL) {
            fileObjectContext = FileObjectGetContext(fileObject);
        }
        NT_ASSERT(fileObjectContext != NULL);

        if (fileObjectContext == NULL)
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestHandleEnableStreaming: cannot find file object context\n"));
            status = STATUS_INVALID_HANDLE;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (inputBuffer->RequestType == CdromStreamingDisable)
        {
            enforceStreamingRead = FALSE;
            enforceStreamingWrite = FALSE;
        }
        else if (inputBuffer->RequestType == CdromStreamingEnableForReadOnly)
        {
            enforceStreamingRead = TRUE;
            enforceStreamingWrite = FALSE;
        }
        else if (inputBuffer->RequestType == CdromStreamingEnableForWriteOnly)
        {
            enforceStreamingRead = FALSE;
            enforceStreamingWrite = TRUE;
        }
        else if (inputBuffer->RequestType == CdromStreamingEnableForReadWrite)
        {
            enforceStreamingRead = TRUE;
            enforceStreamingWrite = TRUE;
        }

        streamingReadSupported = cdData->Mmc.StreamingReadSupported && !TEST_FLAG(fdoData->HackFlags, FDO_HACK_NO_STREAMING);
        streamingWriteSupported = cdData->Mmc.StreamingWriteSupported && !TEST_FLAG(fdoData->HackFlags, FDO_HACK_NO_STREAMING);
        if ((enforceStreamingRead && !streamingReadSupported) ||
            (enforceStreamingWrite && !streamingWriteSupported))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                        "RequestHandleEnableStreaming: requested Streaming mode is not supported\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            fileObjectContext->EnforceStreamingRead = enforceStreamingRead;
            fileObjectContext->EnforceStreamingWrite = enforceStreamingWrite;
        }
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleSendOpcInformation(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

    Handles an IOCTL_CDROM_SEND_OPC_INFORMATION request

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_SIMPLE_OPC_INFO  inputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           sizeof(CDROM_SIMPLE_OPC_INFO),
                                           (PVOID*)&inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        CDB                         cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));

        // we support only the simplest version for now
        cdb.SEND_OPC_INFORMATION.OperationCode = SCSIOP_SEND_OPC_INFORMATION;
        cdb.SEND_OPC_INFORMATION.DoOpc = 1;
        cdb.SEND_OPC_INFORMATION.Exclude0 = (inputBuffer->Exclude0 ? 1 : 0);
        cdb.SEND_OPC_INFORMATION.Exclude1 = (inputBuffer->Exclude1 ? 1 : 0);

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, 0, FALSE, &cdb, sizeof(cdb.SEND_OPC_INFORMATION));

        // nothing to do after the command finishes
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleGetPerformance(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

    Handles an IOCTL_CDROM_GET_PERFORMANCE request

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PCDROM_PERFORMANCE_REQUEST  inputBuffer = NULL;
    PVOID                       outputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    // Retrieve pointers to input/output data. The size has been validated earlier
    // in RequestValidateGetPerformance, so we do not check it again.
    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               (PVOID*)&inputBuffer,
                                               NULL);
    }
    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        USHORT                      descriptorSize = 0;
        USHORT                      descriptorCount = 0;
        size_t                      transferSize = 0;
        CDB                         cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        // Set up the CDB
        RtlZeroMemory(&cdb, sizeof(CDB));

        if (inputBuffer->RequestType == CdromPerformanceRequest)
        {
            cdb.GET_PERFORMANCE.Type = 0;

            // 10b is the only defined tolerance in MMCr6
            cdb.GET_PERFORMANCE.Tolerance = 2;

            switch (inputBuffer->Exceptions) {
                case CdromNominalPerformance:
                    cdb.GET_PERFORMANCE.Except = 0;
                    descriptorSize = sizeof(CDROM_NOMINAL_PERFORMANCE_DESCRIPTOR);
                    break;
                case CdromEntirePerformanceList:
                    cdb.GET_PERFORMANCE.Except = 1;
                    descriptorSize = sizeof(CDROM_NOMINAL_PERFORMANCE_DESCRIPTOR);
                    break;
                case CdromPerformanceExceptionsOnly:
                    cdb.GET_PERFORMANCE.Except = 2;
                    descriptorSize = sizeof(CDROM_EXCEPTION_PERFORMANCE_DESCRIPTOR);
                    break;
            }

            switch (inputBuffer->PerformanceType) {
                case CdromReadPerformance:
                    cdb.GET_PERFORMANCE.Write = 0; break;
                case CdromWritePerformance:
                    cdb.GET_PERFORMANCE.Write = 1; break;
            }

            REVERSE_BYTES(&cdb.GET_PERFORMANCE.StartingLBA, &inputBuffer->StaringLba);
        }
        else if (inputBuffer->RequestType == CdromWriteSpeedRequest)
        {
            cdb.GET_PERFORMANCE.Type = 3;
            descriptorSize = sizeof(CDROM_WRITE_SPEED_DESCRIPTOR);
        }

        cdb.GET_PERFORMANCE.OperationCode = SCSIOP_GET_PERFORMANCE;

        // calculate how many descriptors can fit into the output buffer
        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength >=
                sizeof(CDROM_PERFORMANCE_HEADER) &&
            RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <=
                MAXUSHORT &&
            descriptorSize > 0)
        {
            descriptorCount = (USHORT)(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength - sizeof(CDROM_PERFORMANCE_HEADER));
            descriptorCount /= descriptorSize;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }

        REVERSE_BYTES_SHORT(&cdb.GET_PERFORMANCE.MaximumNumberOfDescriptors, &descriptorCount);

        // Calculate transfer size. We round it up to meet adapter requirements.
        // Extra bytes are discarded later, when we copy data to the output buffer.
        transferSize = RequestParameters.Parameters.DeviceIoControl.OutputBufferLength;
        transferSize += DeviceExtension->AdapterDescriptor->AlignmentMask;
        transferSize &= ~DeviceExtension->AdapterDescriptor->AlignmentMask;

        if (NT_SUCCESS(status))
        {
            status = ScratchBuffer_ExecuteCdbEx(DeviceExtension, Request, (ULONG)transferSize, TRUE, &cdb, sizeof(cdb.GET_PERFORMANCE), CDROM_GET_PERFORMANCE_TIMEOUT);
        }

        if (NT_SUCCESS(status))
        {
            if (DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength < sizeof(CDROM_PERFORMANCE_HEADER))
            {
                *DataLength = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                *DataLength = min(RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                 DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength);
                RtlCopyMemory(outputBuffer,
                              DeviceExtension->ScratchContext.ScratchBuffer,
                              *DataLength);
            }
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleMcnSyncFakeIoctl(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

    Handles an IOCTL_MCN_SYNC_FAKE_IOCTL request

Arguments:

    DeviceExtension - device context
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PMEDIA_CHANGE_DETECTION_INFO    info = DeviceExtension->MediaChangeDetectionInfo;
    BOOLEAN                         shouldRetry = TRUE;
    BOOLEAN                         requestSent = FALSE;

    PAGED_CODE ();

    *DataLength = 0;

    //
    // Try to acquire the media change event.  If we can't do it immediately
    // then bail out and assume the caller will try again later.
    //
    while (shouldRetry)
    {

        status = RequestSetupMcnRequest(DeviceExtension,
                                        info->Gesn.Supported);

        if (!NT_SUCCESS(status))
        {
            shouldRetry = FALSE;
        }

        if (NT_SUCCESS(status))
        {
            requestSent = RequestSendMcnRequest(DeviceExtension);

            if (requestSent)
            {
                shouldRetry = RequestPostWorkMcnRequest(DeviceExtension);
            }
            else
            {
                shouldRetry = FALSE;
            }
        }
    }

    // If there were any media change notifications that were not delivered
    // for some reason, make an attempt to do so at this time.
    DeviceSendDelayedMediaChangeNotifications(DeviceExtension);

    // Set the status and then complete the original request.
    // The timer handler will be able to send the next request.
    status = STATUS_SUCCESS;

    return status;
}

BOOLEAN
RequestIsRealtimeStreaming(
    _In_  WDFREQUEST       Request,
    _In_  BOOLEAN          IsReadRequest
    )
/*++

Routine Description:

    Checks whether a given read/write request should
    be performed in Real-Time Streaming mode.

Arguments:

    Request - request to be checked
    IsReadRequest - TRUE = read request; FALSE = write request

Return Value:

    TRUE  - a Real-Time Streaming operation has to be performed
    FALSE - a normal (non-Streaming) operation has to be performed

--*/
{
    BOOLEAN     useStreaming = FALSE;

    if (!useStreaming) {
        //
        // Check if we're required to use Streaming via I/O Stack Location flags
        //
        UCHAR currentStackLocationFlags = 0;
        currentStackLocationFlags = RequestGetCurrentStackLocationFlags(Request);

        useStreaming = TEST_FLAG(currentStackLocationFlags, SL_REALTIME_STREAM);
    }

    if (!useStreaming) {
        //
        // Check if we were previously requested to enforce Streaming for
        // the file handle through which this request was sent.
        //

        WDFFILEOBJECT           fileObject;
        PFILE_OBJECT_CONTEXT    fileObjectContext;

        fileObject = WdfRequestGetFileObject(Request);

        if (fileObject != NULL) {
            fileObjectContext = FileObjectGetContext(fileObject);
            NT_ASSERT(fileObjectContext != NULL);

            if (IsReadRequest && fileObjectContext->EnforceStreamingRead)
            {
                useStreaming = TRUE;
            }

            if (!IsReadRequest && fileObjectContext->EnforceStreamingWrite)
            {
                useStreaming = TRUE;
            }
        }
    }

    return useStreaming;
}


NTSTATUS
RequestValidateReadWrite(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters
    )
/*++

Routine Description:

   Validate Read/Write request

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCDROM_DATA         cdData = &(DeviceExtension->DeviceAdditionalData);

    BOOLEAN             isValid = TRUE;
    LONGLONG            startingOffset = 0;
    size_t              transferByteCount = 0;
    PIRP                irp = NULL;
    PIO_STACK_LOCATION  currentStack = NULL;

    irp = WdfRequestWdmGetIrp(Request);
    currentStack = IoGetCurrentIrpStackLocation(irp);

    if (TEST_FLAG(DeviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME) &&
        (currentStack->MinorFunction != CDROM_VOLUME_VERIFY_CHECKED) &&
        !TEST_FLAG(currentStack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
    {
        //  DO_VERIFY_VOLUME is set for the device object,
        //  but this request is not itself a verify request.
        //  So fail this request.

        //set the status for volume verification.
        status = STATUS_VERIFY_REQUIRED;
    }

    if (NT_SUCCESS(status))
    {
        if (PLAY_ACTIVE(DeviceExtension))
        {
            status = STATUS_DEVICE_BUSY;
        }
    }

    if (NT_SUCCESS(status))
    {
        // If the device is in exclusive mode, check whether the request is from
        // the handle that locked the device.
        if (EXCLUSIVE_MODE(cdData) && !EXCLUSIVE_OWNER(cdData, WdfRequestGetFileObject(Request)))
        {
            // This request is not from the owner. We can't let the operation go.
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW, "RequestValidateReadWrite: Access Denied! Device in exclusive mode.\n"));

            status = STATUS_ACCESS_DENIED;
        }
    }

    // Validate the request alignment.
    if (NT_SUCCESS(status))
    {
        if (RequestParameters.Type == WdfRequestTypeRead)
        {
            startingOffset = RequestParameters.Parameters.Read.DeviceOffset;
            transferByteCount = RequestParameters.Parameters.Read.Length;
        }
        else
        {
            startingOffset = RequestParameters.Parameters.Write.DeviceOffset;
            transferByteCount = RequestParameters.Parameters.Write.Length;
        }

        if (!DeviceExtension->DiskGeometry.BytesPerSector)
        {
            DeviceExtension->DiskGeometry.BytesPerSector = 2048;
        }

        if (!DeviceExtension->SectorShift)
        {
            DeviceExtension->SectorShift = 11;
        }

        // Perform some basic validation up front
        if (TEST_FLAG(startingOffset, DeviceExtension->DiskGeometry.BytesPerSector - 1) ||
            TEST_FLAG(transferByteCount,       DeviceExtension->DiskGeometry.BytesPerSector - 1))
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    // validate the request against the current mmc schema
    if (NT_SUCCESS(status))
    {
        FEATURE_NUMBER schema = cdData->Mmc.ValidationSchema;

        // We validate read requests according to the RandomWritable schema, except in the
        // case of IncrementalStreamingWritable, wherein  the drive is responsible for all
        // of the verification
        if (RequestParameters.Type == WdfRequestTypeRead)
        {
            if (!cdData->Mmc.WriteAllowed)
            {
                // standard legacy validation of read irps
                // if writing is not allowed on the media
                schema =  FeatureRandomWritable;
            }
            else if (schema != FeatureIncrementalStreamingWritable)
            {
                // standard legacy validation of read irps
                // if not using streaming writes on writable media
                schema =  FeatureRandomWritable;
            }
        }

        // Fail write requests to read-only media
        if ((RequestParameters.Type == WdfRequestTypeWrite) &&
            !(cdData->Mmc.WriteAllowed))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW,  "RequestValidateReadWrite: Write request to read-only media\n"));
            isValid = FALSE;
        }

        if (isValid)
        {
            switch (schema)
            {
                case FeatureDefectManagement:
                case FeatureRandomWritable:
                    // Ensure that the request is within bounds for ROM drives.
                    // Writer drives do not need to have bounds as outbounds request should not damage the drive.
                    if(!cdData->Mmc.IsWriter)
                    {
                        if ((startingOffset >= DeviceExtension->PartitionLength.QuadPart) ||
                            startingOffset < 0)
                        {
                            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW,  "RequestValidateReadWrite: Request is out of bounds\n"));
                            isValid = FALSE;

                        }
                        else
                        {
                            ULONGLONG bytesRemaining = DeviceExtension->PartitionLength.QuadPart - startingOffset;

                            if ((ULONGLONG)transferByteCount > bytesRemaining)
                            {
                                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW,  "RequestValidateReadWrite: Request is out of bounds\n"));
                                isValid = FALSE;
                            }
                        }
                    }
                    break;

                case FeatureRigidRestrictedOverwrite:
                    // Ensure that the number of blocks is a multiple of the blocking size
                    if (((transferByteCount >> DeviceExtension->SectorShift) % cdData->Mmc.Blocking) != 0)
                    {
                        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW,
                                   "RequestValidateReadWrite: Number of blocks is not a multiple of the blocking size (%x)\n",
                                   cdData->Mmc.Blocking));

                        isValid = FALSE;
                    }
                    // Fall through
                case FeatureRestrictedOverwrite:
                    // Ensure that the request begins on a blocking boundary
                    if ((Int64ShrlMod32(startingOffset, DeviceExtension->SectorShift) % cdData->Mmc.Blocking) != 0)
                    {
                        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW,
                                   "RequestValidateReadWrite: Starting block is not a multiple of the blocking size (%x)\n",
                                   cdData->Mmc.Blocking));

                        isValid = FALSE;
                    }
                    break;

                case FeatureIncrementalStreamingWritable:
                    // Let the drive handle the verification
                    break;

                default:
                    // Unknown schema. Fail the request
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW,
                               "RequestValidateReadWrite: Unknown validation schema (%x)\n",
                               schema));

                    isValid = FALSE;
                    break;
            } //end of switch (schema)
        } // end of if (isValid)

        if (!isValid)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    } // end of mmc schema validation

    // validate that the Real-Time Streaming requests meet device capabilties
    if (NT_SUCCESS(status))
    {
        // We do not check for FDO_HACK_NO_STREAMING in DeviceExtension->PrivateFdoData->HackFlags here,
        // because we're going to hide device failures related to streaming reads/writes from the sender
        // of the request. FDO_HACK_NO_STREAMING is going to be taken into account later during actual
        // processing of the request.
        if (RequestIsRealtimeStreaming(Request, RequestParameters.Type == WdfRequestTypeRead))
        {
            if (RequestParameters.Type == WdfRequestTypeRead && !cdData->Mmc.StreamingReadSupported)
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW,
                           "RequestValidateReadWrite: Streaming reads are not supported.\n"));

                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            if (RequestParameters.Type == WdfRequestTypeWrite && !cdData->Mmc.StreamingWriteSupported)
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_RW,
                           "RequestValidateReadWrite: Streaming writes are not supported.\n"));

                status = STATUS_INVALID_DEVICE_REQUEST;
            }
        }
    }

    return status;
}

NTSTATUS
RequestHandleReadWrite(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters
    )
/*++

Routine Description:

    Handle a read/write request

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCDROM_DATA         cdData = &(DeviceExtension->DeviceAdditionalData);

    size_t              transferByteCount = 0;
    PIRP                irp = NULL;
    PIO_STACK_LOCATION  currentStack = NULL;

    PUCHAR              dataBuffer;


    irp = WdfRequestWdmGetIrp(Request);
    currentStack = IoGetCurrentIrpStackLocation(irp);
    dataBuffer = MmGetMdlVirtualAddress(irp->MdlAddress);

    if (NT_SUCCESS(status))
    {
        if (RequestParameters.Type == WdfRequestTypeRead)
        {
            transferByteCount = RequestParameters.Parameters.Read.Length;
        }
        else
        {
            transferByteCount = RequestParameters.Parameters.Write.Length;
        }

        if (transferByteCount == 0)
        {
            //  Several parts of the code turn 0 into 0xffffffff,
            //  so don't process a zero-length request any further.
            status = STATUS_SUCCESS;
            RequestCompletion(DeviceExtension, Request, status, 0);
            return status;
        }

        // Add partition byte offset to make starting byte relative to
        // beginning of disk.
        currentStack->Parameters.Read.ByteOffset.QuadPart += (DeviceExtension->StartingOffset.QuadPart);

        //not very necessary as the starting offset for CD/DVD device is always 0.
        if (RequestParameters.Type == WdfRequestTypeRead)
        {
            RequestParameters.Parameters.Read.DeviceOffset = currentStack->Parameters.Read.ByteOffset.QuadPart;
        }
        else
        {
            RequestParameters.Parameters.Write.DeviceOffset = currentStack->Parameters.Write.ByteOffset.QuadPart;
        }
    }

    if (NT_SUCCESS(status))
    {
        ULONG   entireXferLen = currentStack->Parameters.Read.Length;
        ULONG   maxLength = 0;
        ULONG   packetsCount = 0;

        PCDROM_SCRATCH_READ_WRITE_CONTEXT readWriteContext;
        PCDROM_REQUEST_CONTEXT            requestContext;
        PCDROM_REQUEST_CONTEXT            originalRequestContext;

        // get the count of packets we need to send.
        if ((((ULONG_PTR)dataBuffer) & (PAGE_SIZE-1)) == 0)
        {
            maxLength = cdData->MaxPageAlignedTransferBytes;
        }
        else
        {
            maxLength = cdData->MaxUnalignedTransferBytes;
        }

        packetsCount = entireXferLen / maxLength;

        if (entireXferLen % maxLength != 0)
        {
            packetsCount++;
        }

        originalRequestContext = RequestGetContext(Request);


        ScratchBuffer_BeginUse(DeviceExtension);

        readWriteContext = &DeviceExtension->ScratchContext.ScratchReadWriteContext;
        requestContext = RequestGetContext(DeviceExtension->ScratchContext.ScratchRequest);

        readWriteContext->PacketsCount = packetsCount;
        readWriteContext->EntireXferLen = entireXferLen;
        readWriteContext->MaxLength = maxLength;
        readWriteContext->StartingOffset = currentStack->Parameters.Read.ByteOffset;
        readWriteContext->DataBuffer = dataBuffer;
        readWriteContext->TransferedBytes = 0;
        readWriteContext->IsRead = (RequestParameters.Type == WdfRequestTypeRead);

        requestContext->OriginalRequest = Request;
        requestContext->DeviceExtension = DeviceExtension;

        //
        // Setup the READ/WRITE fields in the original request which is what
        // we use to properly synchronize cancellation logic between the
        // cancel callback and the timer routine.
        //

        originalRequestContext->ReadWriteIsCompleted = FALSE;
        originalRequestContext->ReadWriteRetryInitialized = FALSE;
        originalRequestContext->DeviceExtension = DeviceExtension;

        status = ScratchBuffer_PerformNextReadWrite(DeviceExtension, TRUE);

        // We do not call ScratchBuffer_EndUse here, because we're not releasing the scratch SRB.
        // It will be released in the completion routine.
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandleLoadEjectMedia(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_EJECT_MEDIA
                     IOCTL_STORAGE_LOAD_MEDIA
                     IOCTL_STORAGE_LOAD_MEDIA2

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PZERO_POWER_ODD_INFO        zpoddInfo = DeviceExtension->ZeroPowerODDInfo;

    PAGED_CODE ();

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        // Synchronize with ejection control and ejection cleanup code as
        // well as other eject/load requests.
        WdfWaitLockAcquire(DeviceExtension->EjectSynchronizationLock, NULL);

        if(DeviceExtension->ProtectedLockCount != 0)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,  "RequestHandleLoadEjectMedia: call to eject protected locked "
                                                                "device - failure\n"));
            status = STATUS_DEVICE_BUSY;
        }

        if (NT_SUCCESS(status))
        {
            SCSI_REQUEST_BLOCK  srb;
            PCDB                cdb = (PCDB)srb.Cdb;

            RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

            srb.CdbLength = 6;

            cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
            cdb->START_STOP.LoadEject = 1;

            if(RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_EJECT_MEDIA)
            {
                cdb->START_STOP.Start = 0;

                // We are sending down a soft eject, and in this case we should take an active ref
                // if the command succeeds.
                if ((zpoddInfo != NULL) &&
                    (zpoddInfo->LoadingMechanism == LOADING_MECHANISM_TRAY) &&
                    (zpoddInfo->Load == 0))                                         // Drawer
                {
                    zpoddInfo->MonitorStartStopUnit = TRUE;
                }
            }
            else
            {
                cdb->START_STOP.Start = 1;
            }

            status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                                &srb,
                                                NULL,
                                                0,
                                                FALSE,
                                                Request);

            if (zpoddInfo != NULL)
            {
                zpoddInfo->MonitorStartStopUnit = FALSE;
            }
        }

        WdfWaitLockRelease(DeviceExtension->EjectSynchronizationLock);
    }

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandleReserveRelease(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_RESERVE
                     IOCTL_STORAGE_RELEASE

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PSCSI_REQUEST_BLOCK srb = NULL;
    PCDB                cdb = NULL;
    ULONG               ioctlCode = 0;

    PAGED_CODE ();

    *DataLength = 0;

    srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                         sizeof(SCSI_REQUEST_BLOCK) +
                         (sizeof(ULONG_PTR) * 2),
                         CDROM_TAG_SRB);

    if (srb == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status))
    {
        ioctlCode = RequestParameters.Parameters.DeviceIoControl.IoControlCode;
        cdb = (PCDB)srb->Cdb;
        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        if (TEST_FLAG(DeviceExtension->PrivateFdoData->HackFlags, FDO_HACK_NO_RESERVE6))
        {
            srb->CdbLength = 10;
            cdb->CDB10.OperationCode = (ioctlCode == IOCTL_STORAGE_RESERVE) ? SCSIOP_RESERVE_UNIT10 : SCSIOP_RELEASE_UNIT10;
        }
        else
        {
            srb->CdbLength = 6;
            cdb->CDB6GENERIC.OperationCode = (ioctlCode == IOCTL_STORAGE_RESERVE) ? SCSIOP_RESERVE_UNIT : SCSIOP_RELEASE_UNIT;
        }

        // Set timeout value.
        srb->TimeOutValue = DeviceExtension->TimeOutValue;

        // Send reserves as tagged requests.
        if (ioctlCode == IOCTL_STORAGE_RESERVE)
        {
            SET_FLAG(srb->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);
            srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
        }

        status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                            srb,
                                            NULL,
                                            0,
                                            FALSE,
                                            Request);
        // no data transfer.
        *DataLength = 0;

        FREE_POOL(srb);
    }

    return status;
} // end RequestHandleReserveRelease()

static // __REACTOS__
BOOLEAN
ValidPersistentReserveScope(
    UCHAR Scope)
{
    switch (Scope) {
    case RESERVATION_SCOPE_LU:
    case RESERVATION_SCOPE_ELEMENT:

        return TRUE;

    default:

        return FALSE;
    }
}

static // __REACTOS__
BOOLEAN
ValidPersistentReserveType(
    UCHAR Type)
{
    switch (Type) {
    case RESERVATION_TYPE_WRITE_EXCLUSIVE:
    case RESERVATION_TYPE_EXCLUSIVE:
    case RESERVATION_TYPE_WRITE_EXCLUSIVE_REGISTRANTS:
    case RESERVATION_TYPE_EXCLUSIVE_REGISTRANTS:

        return TRUE;

    default:

        return FALSE;
    }
}

NTSTATUS
RequestValidatePersistentReserve(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Validate request of IOCTL_STORAGE_PERSISTENT_RESERVE_IN
                     IOCTL_STORAGE_PERSISTENT_RESERVE_OUT

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               ioctlCode = RequestParameters.Parameters.DeviceIoControl.IoControlCode;

    PPERSISTENT_RESERVE_COMMAND reserveCommand = NULL;

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&reserveCommand,
                                           NULL);
    if (NT_SUCCESS(status))
    {
        if ((RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(PERSISTENT_RESERVE_COMMAND)) ||
            (reserveCommand->Size < sizeof(PERSISTENT_RESERVE_COMMAND)))
        {
            *DataLength = 0;
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else if ((ULONG_PTR)reserveCommand & DeviceExtension->AdapterDescriptor->AlignmentMask)
        {
            // Check buffer alignment. Only an issue if another kernel mode component
            // (not the I/O manager) allocates the buffer.
            *DataLength = 0;
            status = STATUS_INVALID_USER_BUFFER;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (ioctlCode == IOCTL_STORAGE_PERSISTENT_RESERVE_IN)
        {
            if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < reserveCommand->PR_IN.AllocationLength)
            {
                *DataLength = 0;
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                switch (reserveCommand->PR_IN.ServiceAction)
                {
                case RESERVATION_ACTION_READ_KEYS:
                    if (reserveCommand->PR_IN.AllocationLength < sizeof(PRI_REGISTRATION_LIST))
                    {
                        *DataLength = 0;
                        status = STATUS_INVALID_PARAMETER;
                    }
                    break;

                case RESERVATION_ACTION_READ_RESERVATIONS:
                    if (reserveCommand->PR_IN.AllocationLength < sizeof(PRI_RESERVATION_LIST))
                    {
                        *DataLength = 0;
                        status = STATUS_INVALID_PARAMETER;
                    }
                    break;

                default:
                    *DataLength = 0;
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
            }
        }
        else // case of IOCTL_STORAGE_PERSISTENT_RESERVE_OUT
        {
            // Verify ServiceAction, Scope, and Type
            switch (reserveCommand->PR_OUT.ServiceAction)
            {
            case RESERVATION_ACTION_REGISTER:
            case RESERVATION_ACTION_REGISTER_IGNORE_EXISTING:
            case RESERVATION_ACTION_CLEAR:
                // Scope and type ignored.
                break;

            case RESERVATION_ACTION_RESERVE:
            case RESERVATION_ACTION_RELEASE:
            case RESERVATION_ACTION_PREEMPT:
            case RESERVATION_ACTION_PREEMPT_ABORT:
                if (!ValidPersistentReserveScope(reserveCommand->PR_OUT.Scope) ||
                    !ValidPersistentReserveType(reserveCommand->PR_OUT.Type))
                {
                    *DataLength = 0;
                    status = STATUS_INVALID_PARAMETER;
                }
                break;

            default:
                *DataLength = 0;
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            // Check input buffer for PR Out.
            // Caller must include the PR parameter list.
            if (NT_SUCCESS(status))
            {
                if ((RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
                        (sizeof(PERSISTENT_RESERVE_COMMAND) + sizeof(PRO_PARAMETER_LIST))) ||
                    (reserveCommand->Size < RequestParameters.Parameters.DeviceIoControl.InputBufferLength))
                {
                    *DataLength = 0;
                    status = STATUS_INVALID_PARAMETER;
                }
            }
        } // end of validation for IOCTL_STORAGE_PERSISTENT_RESERVE_OUT
    }

    return status;
} // end RequestValidatePersistentReserve()


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandlePersistentReserve(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_PERSISTENT_RESERVE_IN
                     IOCTL_STORAGE_PERSISTENT_RESERVE_OUT

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               ioctlCode = RequestParameters.Parameters.DeviceIoControl.IoControlCode;
    PSCSI_REQUEST_BLOCK srb = NULL;
    PCDB                cdb = NULL;
    BOOLEAN             writeToDevice;

    PPERSISTENT_RESERVE_COMMAND reserveCommand = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&reserveCommand,
                                           NULL);
    if (NT_SUCCESS(status))
    {
        srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                             sizeof(SCSI_REQUEST_BLOCK) +
                             (sizeof(ULONG_PTR) * 2),
                             CDROM_TAG_SRB);

        if (srb == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            cdb = (PCDB)srb->Cdb;
            RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
        }
    }

    if (NT_SUCCESS(status))
    {
        size_t dataBufLen = 0;

        // Fill in the CDB.
        if (ioctlCode == IOCTL_STORAGE_PERSISTENT_RESERVE_IN)
        {
            cdb->PERSISTENT_RESERVE_IN.OperationCode    = SCSIOP_PERSISTENT_RESERVE_IN;
            cdb->PERSISTENT_RESERVE_IN.ServiceAction    = reserveCommand->PR_IN.ServiceAction;

            REVERSE_BYTES_SHORT(&(cdb->PERSISTENT_RESERVE_IN.AllocationLength),
                                &(reserveCommand->PR_IN.AllocationLength));

            dataBufLen = RequestParameters.Parameters.DeviceIoControl.OutputBufferLength;
            writeToDevice = FALSE;
        }
        else
        {
            cdb->PERSISTENT_RESERVE_OUT.OperationCode   = SCSIOP_PERSISTENT_RESERVE_OUT;
            cdb->PERSISTENT_RESERVE_OUT.ServiceAction   = reserveCommand->PR_OUT.ServiceAction;
            cdb->PERSISTENT_RESERVE_OUT.Scope           = reserveCommand->PR_OUT.Scope;
            cdb->PERSISTENT_RESERVE_OUT.Type            = reserveCommand->PR_OUT.Type;

            cdb->PERSISTENT_RESERVE_OUT.ParameterListLength[1] = (UCHAR)sizeof(PRO_PARAMETER_LIST);

            // Move the parameter list to the beginning of the data buffer (so it is aligned
            // correctly and that the MDL describes it correctly).
            RtlMoveMemory(reserveCommand,
                          reserveCommand->PR_OUT.ParameterList,
                          sizeof(PRO_PARAMETER_LIST));

            dataBufLen = sizeof(PRO_PARAMETER_LIST);
            writeToDevice = TRUE;
        }

        srb->CdbLength = 10;
        srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

        status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                            srb,
                                            reserveCommand,
                                            (ULONG) dataBufLen,
                                            writeToDevice,
                                            Request);

        FREE_POOL(srb);
    }

    return status;
}

#if (NTDDI_VERSION >= NTDDI_WIN8)
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleAreVolumesReady(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DISK_ARE_VOLUMES_READY

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN                 completeRequest = TRUE;
    NTSTATUS                status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(RequestParameters);

    *DataLength = 0;

    if (DeviceExtension->IsVolumeOnlinePending == FALSE)
    {
        status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Add to the volume ready queue. No worries about request cancellation,
    // since KMDF will automatically handle that while request is in queue.
    //
    status = WdfRequestForwardToIoQueue(Request,
                                        DeviceExtension->ManualVolumeReadyQueue
                                        );

    if(!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    status = STATUS_PENDING;
    completeRequest = FALSE;

Cleanup:

    if (completeRequest)
    {
        RequestCompletion(DeviceExtension, Request, status, 0);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleVolumeOnline(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_VOLUME_ONLINE / IOCTL_VOLUME_POST_ONLINE

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    WDFREQUEST              request;
    PIRP                    irp = NULL;
    PIO_STACK_LOCATION      nextStack = NULL;

    UNREFERENCED_PARAMETER(RequestParameters);

    *DataLength = 0;

    DeviceExtension->IsVolumeOnlinePending = FALSE;

    // Complete all parked volume ready requests.
    for (;;)
    {
        status = WdfIoQueueRetrieveNextRequest(DeviceExtension->ManualVolumeReadyQueue,
                                               &request);

        if (!NT_SUCCESS(status))
        {
            break;
        }

        RequestCompletion(DeviceExtension, request, STATUS_SUCCESS, 0);
    }

    // Change the IOCTL code to IOCTL_DISK_VOLUMES_ARE_READY, and forward the request down,
    // so that if the underlying port driver will also know that volume is ready.
    WdfRequestFormatRequestUsingCurrentType(Request);
    irp = WdfRequestWdmGetIrp(Request);
    nextStack = IoGetNextIrpStackLocation(irp);

    irp->AssociatedIrp.SystemBuffer = &DeviceExtension->DeviceNumber;
    nextStack->Parameters.DeviceIoControl.OutputBufferLength = 0;
    nextStack->Parameters.DeviceIoControl.InputBufferLength = sizeof (DeviceExtension->DeviceNumber);
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_DISK_VOLUMES_ARE_READY;

    // Send the request straight down (synchronously).
    RequestSend(DeviceExtension,
                Request,
                DeviceExtension->IoTarget,
                WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
                NULL);

    return STATUS_SUCCESS;
}
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceHandleRawRead(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_RAW_READ

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    // Determine whether the drive is currently in raw or cooked mode,
    // and which command to use to read the data.

    NTSTATUS        status = STATUS_SUCCESS;
    PCDROM_DATA     cdData = &(DeviceExtension->DeviceAdditionalData);
    RAW_READ_INFO   rawReadInfo = {0};
    PVOID           outputVirtAddr = NULL;
    ULONG           startingSector;

    PIRP                irp = WdfRequestWdmGetIrp(Request);
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(irp);

    VOID*               outputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    if (TEST_FLAG(DeviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME) &&
        (currentIrpStack->MinorFunction != CDROM_VOLUME_VERIFY_CHECKED) &&
        !TEST_FLAG(currentIrpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
    {
        //  DO_VERIFY_VOLUME is set for the device object,
        //  but this request is not itself a verify request.
        //  So fail this request.
        (VOID) MediaReadCapacity(DeviceExtension->Device);

        *DataLength = 0;
        //set the status for volume verification.
        status = STATUS_VERIFY_REQUIRED;
    }

    if (NT_SUCCESS(status))
    {
        // Since this ioctl is METHOD_OUT_DIRECT, we need to copy away
        // the input buffer before interpreting it.  This prevents a
        // malicious app from messing with the input buffer while we
        // are interpreting it. This is done in dispacth.
        //
        // Here, we are going to get the input buffer out of saved place.
        rawReadInfo = *(PRAW_READ_INFO)currentIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength > 0)
        {
            // Make sure that any user buffer that we pass down to
            // the hardware is properly aligned
            NT_ASSERT(irp->MdlAddress);
            outputVirtAddr = MmGetMdlVirtualAddress(irp->MdlAddress);
            if ((ULONG_PTR)outputVirtAddr & DeviceExtension->AdapterDescriptor->AlignmentMask)
            {
                NT_ASSERT(!((ULONG_PTR)outputVirtAddr & DeviceExtension->AdapterDescriptor->AlignmentMask));
                *DataLength = 0;
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        switch (rawReadInfo.TrackMode)
        {
        case CDDA:
        case YellowMode2:
        case XAForm2:
            // no check needed.
            break;

        case RawWithC2AndSubCode:
            if (!cdData->Mmc.ReadCdC2Pointers || !cdData->Mmc.ReadCdSubCode)
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                          "Request to read C2 & Subcode rejected.  "
                          "Is C2 supported: %d   Is Subcode supported: %d\n",
                          cdData->Mmc.ReadCdC2Pointers,
                          cdData->Mmc.ReadCdSubCode
                          ));
                *DataLength = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;
        case RawWithC2:
            if (!cdData->Mmc.ReadCdC2Pointers)
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                          "Request to read C2 rejected because drive does not "
                          "report support for C2 pointers\n"
                          ));
                *DataLength = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;
        case RawWithSubCode:

            if (!cdData->Mmc.ReadCdSubCode)
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                          "Request to read subcode rejected because drive does "
                          "not report support for reading the subcode data\n"
                          ));
                *DataLength = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;

        default:
            *DataLength = 0;
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        PSCSI_REQUEST_BLOCK srb = DeviceExtension->ScratchContext.ScratchSrb;
        PCDB                cdb = (PCDB)(srb->Cdb);

        size_t              transferByteCount;
        BOOLEAN             shouldRetry = TRUE;
        ULONG               timesAlreadyRetried = 0;
        LONGLONG            retryIn100nsUnits = 0;

        transferByteCount = RequestParameters.Parameters.DeviceIoControl.OutputBufferLength;

        ScratchBuffer_BeginUse(DeviceExtension);

        while (shouldRetry)
        {
            // Setup cdb depending upon the sector type we want.
            switch (rawReadInfo.TrackMode)
            {
            case CDDA:
                transferByteCount  = rawReadInfo.SectorCount * RAW_SECTOR_SIZE;
                cdb->READ_CD.ExpectedSectorType = CD_DA_SECTOR;
                cdb->READ_CD.IncludeUserData = 1;
                cdb->READ_CD.HeaderCode = 3;
                cdb->READ_CD.IncludeSyncData = 1;
                break;

            case YellowMode2:
                transferByteCount  = rawReadInfo.SectorCount * RAW_SECTOR_SIZE;
                cdb->READ_CD.ExpectedSectorType = YELLOW_MODE2_SECTOR;
                cdb->READ_CD.IncludeUserData = 1;
                cdb->READ_CD.HeaderCode = 1;
                cdb->READ_CD.IncludeSyncData = 1;
                break;

            case XAForm2:
                transferByteCount  = rawReadInfo.SectorCount * RAW_SECTOR_SIZE;
                cdb->READ_CD.ExpectedSectorType = FORM2_MODE2_SECTOR;
                cdb->READ_CD.IncludeUserData = 1;
                cdb->READ_CD.HeaderCode = 3;
                cdb->READ_CD.IncludeSyncData = 1;
                break;

            case RawWithC2AndSubCode:
                transferByteCount  = rawReadInfo.SectorCount * CD_RAW_SECTOR_WITH_C2_AND_SUBCODE_SIZE;
                cdb->READ_CD.ExpectedSectorType = 0;    // Any sector type
                cdb->READ_CD.IncludeUserData = 1;
                cdb->READ_CD.HeaderCode = 3;            // Header and subheader returned
                cdb->READ_CD.IncludeSyncData = 1;
                cdb->READ_CD.ErrorFlags = 2;            // C2 and block error
                cdb->READ_CD.SubChannelSelection = 1;   // raw subchannel data
                break;

            case RawWithC2:
                transferByteCount  = rawReadInfo.SectorCount * CD_RAW_SECTOR_WITH_C2_SIZE;
                cdb->READ_CD.ExpectedSectorType = 0;    // Any sector type
                cdb->READ_CD.IncludeUserData = 1;
                cdb->READ_CD.HeaderCode = 3;            // Header and subheader returned
                cdb->READ_CD.IncludeSyncData = 1;
                cdb->READ_CD.ErrorFlags = 2;            // C2 and block error
                break;

            case RawWithSubCode:
                transferByteCount  = rawReadInfo.SectorCount * CD_RAW_SECTOR_WITH_SUBCODE_SIZE;
                cdb->READ_CD.ExpectedSectorType = 0;    // Any sector type
                cdb->READ_CD.IncludeUserData = 1;
                cdb->READ_CD.HeaderCode = 3;            // Header and subheader returned
                cdb->READ_CD.IncludeSyncData = 1;
                cdb->READ_CD.SubChannelSelection = 1;   // raw subchannel data
                break;

            default:
                // should already checked before coming in loop.
                NT_ASSERT(FALSE);
                break;
            }

            ScratchBuffer_SetupSrb(DeviceExtension, Request, (ULONG)transferByteCount, TRUE);
            // Restore the buffer, buffer size and MdlAddress. They got changed but we don't want to use the scratch buffer.
            {
                PIRP    scratchIrp = WdfRequestWdmGetIrp(DeviceExtension->ScratchContext.ScratchRequest);
                scratchIrp->MdlAddress = irp->MdlAddress;
                srb->DataBuffer = outputVirtAddr;
                srb->DataTransferLength = (ULONG)transferByteCount;
            }
            // Calculate starting offset.
            startingSector = (ULONG)(rawReadInfo.DiskOffset.QuadPart >> DeviceExtension->SectorShift);

            // Fill in CDB fields.
            srb->CdbLength = 12;

            cdb->READ_CD.OperationCode = SCSIOP_READ_CD;
            cdb->READ_CD.TransferBlocks[2]  = (UCHAR) (rawReadInfo.SectorCount & 0xFF);
            cdb->READ_CD.TransferBlocks[1]  = (UCHAR) (rawReadInfo.SectorCount >> 8 );
            cdb->READ_CD.TransferBlocks[0]  = (UCHAR) (rawReadInfo.SectorCount >> 16);

            cdb->READ_CD.StartingLBA[3]  = (UCHAR) (startingSector & 0xFF);
            cdb->READ_CD.StartingLBA[2]  = (UCHAR) ((startingSector >>  8));
            cdb->READ_CD.StartingLBA[1]  = (UCHAR) ((startingSector >> 16));
            cdb->READ_CD.StartingLBA[0]  = (UCHAR) ((startingSector >> 24));

            ScratchBuffer_SendSrb(DeviceExtension, TRUE, NULL);

            if ((DeviceExtension->ScratchContext.ScratchSrb->SrbStatus == SRB_STATUS_ABORTED) &&
                (DeviceExtension->ScratchContext.ScratchSrb->InternalStatus == STATUS_CANCELLED))
            {
                shouldRetry = FALSE;
                status = STATUS_CANCELLED;
            }
            else
            {
                shouldRetry = RequestSenseInfoInterpretForScratchBuffer(DeviceExtension,
                                                                        timesAlreadyRetried,
                                                                        &status,
                                                                        &retryIn100nsUnits);
                if (shouldRetry)
                {
                    LARGE_INTEGER t;
                    t.QuadPart = -retryIn100nsUnits;
                    timesAlreadyRetried++;
                    KeDelayExecutionThread(KernelMode, FALSE, &t);
                    // keep items clean
                    ScratchBuffer_ResetItems(DeviceExtension, FALSE);
                }
            }
        }

        if (NT_SUCCESS(status))
        {
            *DataLength = srb->DataTransferLength;
        }

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandlePlayAudioMsf(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_PLAY_AUDIO_MSF

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_PLAY_AUDIO_MSF   inputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&inputBuffer,
                                           NULL);


    if (NT_SUCCESS(status))
    {
        ULONG   transferSize = 0;
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.PLAY_AUDIO_MSF.OperationCode = SCSIOP_PLAY_AUDIO_MSF;

        cdb.PLAY_AUDIO_MSF.StartingM = inputBuffer->StartingM;
        cdb.PLAY_AUDIO_MSF.StartingS = inputBuffer->StartingS;
        cdb.PLAY_AUDIO_MSF.StartingF = inputBuffer->StartingF;

        cdb.PLAY_AUDIO_MSF.EndingM = inputBuffer->EndingM;
        cdb.PLAY_AUDIO_MSF.EndingS = inputBuffer->EndingS;
        cdb.PLAY_AUDIO_MSF.EndingF = inputBuffer->EndingF;

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 10);

        if (NT_SUCCESS(status))
        {
            PLAY_ACTIVE(DeviceExtension) = TRUE;
            *DataLength = 0;
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleReadQChannel(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_READ_Q_CHANNEL

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PVOID       inputBuffer = NULL;
    PVOID       outputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = ReadQChannel(DeviceExtension,
                              Request,
                              inputBuffer,
                              RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                              outputBuffer,
                              RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                              DataLength);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
ReadQChannel(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_opt_  WDFREQUEST           OriginalRequest,
    _In_  PVOID                    InputBuffer,
    _In_  size_t                   InputBufferLength,
    _In_  PVOID                    OutputBuffer,
    _In_  size_t                   OutputBufferLength,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   base function to handle request of IOCTL_CDROM_READ_Q_CHANNEL

Arguments:

    DeviceExtension - device context
    OriginalRequest - original request to be handled
    InputBuffer - input buffer
    InputBufferLength - length of input buffer
    OutputBuffer - output buffer
    OutputBufferLength - length of output buffer

Return Value:

    NTSTATUS
    DataLength - returned data length

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    ULONG                       transferByteCount = 0;
    CDB                         cdb;
    PCDROM_SUB_Q_DATA_FORMAT    inputBuffer = (PCDROM_SUB_Q_DATA_FORMAT)InputBuffer;
    PSUB_Q_CHANNEL_DATA         userChannelData = (PSUB_Q_CHANNEL_DATA)OutputBuffer;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER(InputBufferLength);

    *DataLength = 0;

    // Set size of channel data -- however, this is dependent on
    // what information we are requesting (which Format)
    switch( inputBuffer->Format )
    {
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

    ScratchBuffer_BeginUse(DeviceExtension);

    RtlZeroMemory(&cdb, sizeof(CDB));
    // Set up the CDB
    // Always logical unit 0, but only use MSF addressing
    // for IOCTL_CDROM_CURRENT_POSITION
    if (inputBuffer->Format==IOCTL_CDROM_CURRENT_POSITION)
    {
        cdb.SUBCHANNEL.Msf = CDB_USE_MSF;
    }

    // Return subchannel data
    cdb.SUBCHANNEL.SubQ = CDB_SUBCHANNEL_BLOCK;

    // Specify format of informatin to return
    cdb.SUBCHANNEL.Format = inputBuffer->Format;

    // Specify which track to access (only used by Track ISRC reads)
    if (inputBuffer->Format==IOCTL_CDROM_TRACK_ISRC)
    {
        cdb.SUBCHANNEL.TrackNumber = inputBuffer->Track;
    }

    cdb.SUBCHANNEL.AllocationLength[0] = (UCHAR) (transferByteCount >> 8);
    cdb.SUBCHANNEL.AllocationLength[1] = (UCHAR) (transferByteCount & 0xFF);
    cdb.SUBCHANNEL.OperationCode = SCSIOP_READ_SUB_CHANNEL;

    status = ScratchBuffer_ExecuteCdb(DeviceExtension, OriginalRequest, transferByteCount, TRUE, &cdb, 10);

    if (NT_SUCCESS(status))
    {
        PSUB_Q_CHANNEL_DATA subQPtr = DeviceExtension->ScratchContext.ScratchSrb->DataBuffer;

#if DBG
        switch( inputBuffer->Format )
        {
        case IOCTL_CDROM_CURRENT_POSITION:
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Audio Status is %u\n", subQPtr->CurrentPosition.Header.AudioStatus ));
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: ADR = 0x%x\n", subQPtr->CurrentPosition.ADR ));
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Control = 0x%x\n", subQPtr->CurrentPosition.Control ));
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Track = %u\n", subQPtr->CurrentPosition.TrackNumber ));
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Index = %u\n", subQPtr->CurrentPosition.IndexNumber ));
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Absolute Address = %x\n", *((PULONG)subQPtr->CurrentPosition.AbsoluteAddress) ));
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Relative Address = %x\n", *((PULONG)subQPtr->CurrentPosition.TrackRelativeAddress) ));
            break;

        case IOCTL_CDROM_MEDIA_CATALOG:
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Audio Status is %u\n", subQPtr->MediaCatalog.Header.AudioStatus ));
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Mcval is %u\n", subQPtr->MediaCatalog.Mcval ));
            break;

        case IOCTL_CDROM_TRACK_ISRC:
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Audio Status is %u\n", subQPtr->TrackIsrc.Header.AudioStatus ));
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,"ReadQChannel: Tcval is %u\n", subQPtr->TrackIsrc.Tcval ));
            break;
        }
#endif

        // Update the play active status.
        if (subQPtr->CurrentPosition.Header.AudioStatus == AUDIO_STATUS_IN_PROGRESS)
        {
            PLAY_ACTIVE(DeviceExtension) = TRUE;
        }
        else
        {
            PLAY_ACTIVE(DeviceExtension) = FALSE;
        }

        // Check if output buffer is large enough to contain
        // the data.
        if (OutputBufferLength < DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength)
        {
            DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength = (ULONG)OutputBufferLength;
        }

        // Copy our buffer into users.
        RtlMoveMemory(userChannelData,
                      subQPtr,
                      DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength);

        *DataLength = DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength;
    }

    // nothing to do after the command finishes.
    ScratchBuffer_EndUse(DeviceExtension);

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandlePauseAudio(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_PAUSE_AUDIO

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE ();

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        ULONG   transferSize = 0;
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
        cdb.PAUSE_RESUME.Action = CDB_AUDIO_PAUSE;

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 10);

        if (NT_SUCCESS(status))
        {
            PLAY_ACTIVE(DeviceExtension) = FALSE;
            *DataLength = 0;
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleResumeAudio(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_RESUME_AUDIO

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE ();

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        ULONG   transferSize = 0;
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
        cdb.PAUSE_RESUME.Action = CDB_AUDIO_RESUME;

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 10);

        if (NT_SUCCESS(status))
        {
            PLAY_ACTIVE(DeviceExtension) = TRUE;  //not in original code. But we should set it.
            *DataLength = 0;
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleSeekAudioMsf(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_SEEK_AUDIO_MSF

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS              status = STATUS_SUCCESS;
    PCDROM_SEEK_AUDIO_MSF inputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        ULONG   transferSize = 0;
        CDB     cdb;
        ULONG   logicalBlockAddress;

        logicalBlockAddress = MSF_TO_LBA(inputBuffer->M, inputBuffer->S, inputBuffer->F);

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.SEEK.OperationCode      = SCSIOP_SEEK;
        cdb.SEEK.LogicalBlockAddress[0] = ((PFOUR_BYTE)&logicalBlockAddress)->Byte3;
        cdb.SEEK.LogicalBlockAddress[1] = ((PFOUR_BYTE)&logicalBlockAddress)->Byte2;
        cdb.SEEK.LogicalBlockAddress[2] = ((PFOUR_BYTE)&logicalBlockAddress)->Byte1;
        cdb.SEEK.LogicalBlockAddress[3] = ((PFOUR_BYTE)&logicalBlockAddress)->Byte0;

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 10);

        if (NT_SUCCESS(status))
        {
            *DataLength = 0;
        }

        // nothing to do after the command finishes.

        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleStopAudio(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _Out_ size_t *                 DataLength
    )
{
    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE ();

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        ULONG   transferSize = 0;
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
        cdb.START_STOP.Immediate = 1;
        cdb.START_STOP.Start = 0;
        cdb.START_STOP.LoadEject = 0;

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 6);

        if (NT_SUCCESS(status))
        {
            PLAY_ACTIVE(DeviceExtension) = FALSE;
            *DataLength = 0;
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleGetSetVolume(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_GET_VOLUME
                     IOCTL_CDROM_SET_VOLUME

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS              status = STATUS_SUCCESS;

    PAGED_CODE ();

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        ULONG   transferSize = MODE_DATA_SIZE;
        CDB     cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
        cdb.MODE_SENSE10.PageCode = CDROM_AUDIO_CONTROL_PAGE;
        cdb.MODE_SENSE10.AllocationLength[0] = (UCHAR)(MODE_DATA_SIZE >> 8);
        cdb.MODE_SENSE10.AllocationLength[1] = (UCHAR)(MODE_DATA_SIZE & 0xFF);

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, TRUE, &cdb, 10);

        if (NT_SUCCESS(status))
        {
            if (RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_GET_VOLUME)
            {
                PAUDIO_OUTPUT   audioOutput;
                PVOLUME_CONTROL volumeControl;
                ULONG           bytesTransferred;

                status = WdfRequestRetrieveOutputBuffer(Request,
                                                        RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                        (PVOID*)&volumeControl,
                                                        NULL);
                if (NT_SUCCESS(status))
                {
                    audioOutput = ModeSenseFindSpecificPage((PCHAR)DeviceExtension->ScratchContext.ScratchSrb->DataBuffer,
                                                            DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength,
                                                            CDROM_AUDIO_CONTROL_PAGE,
                                                            FALSE);

                    // Verify the page is as big as expected.
                    bytesTransferred = (ULONG)((PCHAR)audioOutput - (PCHAR)DeviceExtension->ScratchContext.ScratchSrb->DataBuffer) +
                                        sizeof(AUDIO_OUTPUT);

                    if ((audioOutput != NULL) &&
                        (DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength >= bytesTransferred))
                    {
                        ULONG i;
                        for (i=0; i<4; i++)
                        {
                            volumeControl->PortVolume[i] = audioOutput->PortOutput[i].Volume;
                        }

                        // Set bytes transferred in IRP.
                        *DataLength = sizeof(VOLUME_CONTROL);

                    }
                    else
                    {
                        *DataLength = 0;
                        status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                }
            }
            else    //IOCTL_CDROM_SET_VOLUME
            {
                PAUDIO_OUTPUT   audioInput = NULL;
                PAUDIO_OUTPUT   audioOutput = NULL;
                PVOLUME_CONTROL volumeControl = NULL;
                ULONG           i,bytesTransferred,headerLength;

                status = WdfRequestRetrieveInputBuffer(Request,
                                                       RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                                       (PVOID*)&volumeControl,
                                                       NULL);

                if (NT_SUCCESS(status))
                {
                    audioInput = ModeSenseFindSpecificPage((PCHAR)DeviceExtension->ScratchContext.ScratchSrb->DataBuffer,
                                                           DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength,
                                                           CDROM_AUDIO_CONTROL_PAGE,
                                                           FALSE);

                    // Check to make sure the mode sense data is valid before we go on
                    if(audioInput == NULL)
                    {
                        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                                    "Mode Sense Page %d not found\n",
                                    CDROM_AUDIO_CONTROL_PAGE));

                        *DataLength = 0;
                        status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                }

                if (NT_SUCCESS(status))
                {
                    // keep items clean; clear the command history
                    ScratchBuffer_ResetItems(DeviceExtension, TRUE);

                    headerLength = sizeof(MODE_PARAMETER_HEADER10);
                    bytesTransferred = sizeof(AUDIO_OUTPUT) + headerLength;

                    // use the scratch buffer as input buffer.
                    // the content of this buffer will not be changed in the following loop.
                    audioOutput = (PAUDIO_OUTPUT)((PCHAR)DeviceExtension->ScratchContext.ScratchBuffer + headerLength);

                    for (i=0; i<4; i++)
                    {
                        audioOutput->PortOutput[i].Volume = volumeControl->PortVolume[i];
                        audioOutput->PortOutput[i].ChannelSelection = audioInput->PortOutput[i].ChannelSelection;
                    }

                    audioOutput->CodePage = CDROM_AUDIO_CONTROL_PAGE;
                    audioOutput->ParameterLength = sizeof(AUDIO_OUTPUT) - 2;
                    audioOutput->Immediate = MODE_SELECT_IMMEDIATE;

                    RtlZeroMemory(&cdb, sizeof(CDB));
                    // Set up the CDB
                    cdb.MODE_SELECT10.OperationCode = SCSIOP_MODE_SELECT10;
                    cdb.MODE_SELECT10.ParameterListLength[0] = (UCHAR) (bytesTransferred >> 8);
                    cdb.MODE_SELECT10.ParameterListLength[1] = (UCHAR) (bytesTransferred & 0xFF);
                    cdb.MODE_SELECT10.PFBit = 1;

                    status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, bytesTransferred, FALSE, &cdb, 10);

                }
                *DataLength = 0;
            }
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleReadDvdStructure(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DVD_READ_STRUCTURE

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PVOID       inputBuffer = NULL;
    PVOID       outputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = ReadDvdStructure(DeviceExtension,
                                  Request,
                                  inputBuffer,
                                  RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                  outputBuffer,
                                  RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                  DataLength);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
ReadDvdStructure(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_opt_  WDFREQUEST           OriginalRequest,
    _In_  PVOID                    InputBuffer,
    _In_  size_t                   InputBufferLength,
    _In_  PVOID                    OutputBuffer,
    _In_  size_t                   OutputBufferLength,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   base function to handle request of IOCTL_DVD_START_SESSION
                                      IOCTL_DVD_READ_KEY

Arguments:

    DeviceExtension - device context
    OriginalRequest - original request to be handled
    InputBuffer - input buffer
    InputBufferLength - length of input buffer
    OutputBuffer - output buffer
    OutputBufferLength - length of output buffer

Return Value:

    NTSTATUS
    DataLength - returned data length

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDVD_READ_STRUCTURE     request = (PDVD_READ_STRUCTURE)InputBuffer;
    PDVD_DESCRIPTOR_HEADER  header = (PDVD_DESCRIPTOR_HEADER)OutputBuffer;
    CDB                     cdb;

    USHORT                  dataLength;
    ULONG                   blockNumber;
    PFOUR_BYTE              fourByte;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER(InputBufferLength);

    if (DeviceExtension->DeviceAdditionalData.DriveDeviceType != FILE_DEVICE_DVD)
    {
        *DataLength = 0;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    dataLength = (USHORT)OutputBufferLength;
    blockNumber = (ULONG)(request->BlockByteOffset.QuadPart >> DeviceExtension->SectorShift);
    fourByte = (PFOUR_BYTE)&blockNumber;

    ScratchBuffer_BeginUse(DeviceExtension);

    RtlZeroMemory(&cdb, sizeof(CDB));
    // Set up the CDB
    cdb.READ_DVD_STRUCTURE.OperationCode = SCSIOP_READ_DVD_STRUCTURE;
    cdb.READ_DVD_STRUCTURE.RMDBlockNumber[0] = fourByte->Byte3;
    cdb.READ_DVD_STRUCTURE.RMDBlockNumber[1] = fourByte->Byte2;
    cdb.READ_DVD_STRUCTURE.RMDBlockNumber[2] = fourByte->Byte1;
    cdb.READ_DVD_STRUCTURE.RMDBlockNumber[3] = fourByte->Byte0;
    cdb.READ_DVD_STRUCTURE.LayerNumber   = request->LayerNumber;
    cdb.READ_DVD_STRUCTURE.Format        = (UCHAR)request->Format;

#if DBG
    {
        if ((UCHAR)request->Format > DvdMaxDescriptor)
        {
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                        "READ_DVD_STRUCTURE format %x = %s (%x bytes)\n",
                        (UCHAR)request->Format,
                        READ_DVD_STRUCTURE_FORMAT_STRINGS[DvdMaxDescriptor],
                        dataLength
                        ));
        }
        else
        {
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                        "READ_DVD_STRUCTURE format %x = %s (%x bytes)\n",
                        (UCHAR)request->Format,
                        READ_DVD_STRUCTURE_FORMAT_STRINGS[(UCHAR)request->Format],
                        dataLength
                        ));
        }
    }
#endif // DBG

    if (request->Format == DvdDiskKeyDescriptor)
    {
        cdb.READ_DVD_STRUCTURE.AGID = (UCHAR)request->SessionId;
    }

    cdb.READ_DVD_STRUCTURE.AllocationLength[0] = (UCHAR)(dataLength >> 8);
    cdb.READ_DVD_STRUCTURE.AllocationLength[1] = (UCHAR)(dataLength & 0xff);

    status = ScratchBuffer_ExecuteCdb(DeviceExtension, OriginalRequest, dataLength, TRUE, &cdb, 12);

    if (NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "DvdDCCompletion - READ_STRUCTURE: descriptor format of %d\n", request->Format));

        RtlMoveMemory(header,
                      DeviceExtension->ScratchContext.ScratchSrb->DataBuffer,
                      DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength);

        // Cook the data.  There are a number of fields that really
        // should be byte-swapped for the caller.
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                  "DvdDCCompletion - READ_STRUCTURE:\n"
                  "\tHeader at %p\n"
                  "\tDvdDCCompletion - READ_STRUCTURE: data at %p\n"
                  "\tDataBuffer was at %p\n"
                  "\tDataTransferLength was %lx\n",
                  header,
                  header->Data,
                  DeviceExtension->ScratchContext.ScratchSrb->DataBuffer,
                  DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength));

        // First the fields in the header
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "READ_STRUCTURE: header->Length %lx -> ",
                   header->Length));

        REVERSE_SHORT(&header->Length);

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "%lx\n", header->Length));

        // Now the fields in the descriptor
        if(request->Format == DvdPhysicalDescriptor)
        {
            ULONG tempLength = (DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength > (ULONG)FIELD_OFFSET(DVD_DESCRIPTOR_HEADER, Data))
                                ? (DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength - FIELD_OFFSET(DVD_DESCRIPTOR_HEADER, Data))
                                : 0;

            PDVD_LAYER_DESCRIPTOR layer = (PDVD_LAYER_DESCRIPTOR)&(header->Data[0]);

            // Make sure the buffer size is good for swapping bytes.
            if (tempLength >= RTL_SIZEOF_THROUGH_FIELD(DVD_LAYER_DESCRIPTOR, StartingDataSector))
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "READ_STRUCTURE: StartingDataSector %lx -> ",
                               layer->StartingDataSector));
                REVERSE_LONG(&(layer->StartingDataSector));
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "%lx\n", layer->StartingDataSector));
            }
            if (tempLength >= RTL_SIZEOF_THROUGH_FIELD(DVD_LAYER_DESCRIPTOR, EndDataSector))
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "READ_STRUCTURE: EndDataSector %lx -> ",
                               layer->EndDataSector));
                REVERSE_LONG(&(layer->EndDataSector));
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "%lx\n", layer->EndDataSector));
            }
            if (tempLength >= RTL_SIZEOF_THROUGH_FIELD(DVD_LAYER_DESCRIPTOR, EndLayerZeroSector))
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "READ_STRUCTURE: EndLayerZeroSector %lx -> ",
                               layer->EndLayerZeroSector));
                REVERSE_LONG(&(layer->EndLayerZeroSector));
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "%lx\n", layer->EndLayerZeroSector));
            }
        }

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "Status is %lx\n", status));
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DvdDeviceControlCompletion - "
                        "IOCTL_DVD_READ_STRUCTURE: data transfer length of %d\n",
                        DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength));
        }

        *DataLength = DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength;
    }

    ScratchBuffer_EndUse(DeviceExtension);

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleDvdEndSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DVD_END_SESSION

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PDVD_SESSION_ID sessionId = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&sessionId,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        ULONG           transferSize = 0;
        CDB             cdb;
        DVD_SESSION_ID  currentSession = 0;
        DVD_SESSION_ID  limitSession = 0;

        if(*sessionId == DVD_END_ALL_SESSIONS)
        {
            currentSession = 0;
            limitSession = MAX_COPY_PROTECT_AGID - 1;
        }
        else
        {
            currentSession = *sessionId;
            limitSession = *sessionId;
        }

        ScratchBuffer_BeginUse(DeviceExtension);

        do
        {
            RtlZeroMemory(&cdb, sizeof(CDB));
            // Set up the CDB
            cdb.SEND_KEY.OperationCode = SCSIOP_SEND_KEY;
            cdb.SEND_KEY.AGID = (UCHAR)(currentSession);
            cdb.SEND_KEY.KeyFormat = DVD_INVALIDATE_AGID;

            status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 12);

            currentSession++;
        } while ((currentSession <= limitSession) && NT_SUCCESS(status));

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleDvdStartSessionReadKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DVD_START_SESSION
                     IOCTL_DVD_READ_KEY

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDVD_COPY_PROTECT_KEY   keyParameters = NULL;
    PVOID                   outputBuffer = NULL;

    PAGED_CODE ();

    if (NT_SUCCESS(status))
    {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                                &outputBuffer,
                                                NULL);
    }

    if (NT_SUCCESS(status) && RequestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_DVD_READ_KEY)
    {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                               (PVOID*)&keyParameters,
                                               NULL);
    }

    if (NT_SUCCESS(status))
    {
        status = DvdStartSessionReadKey(DeviceExtension,
                                        RequestParameters.Parameters.DeviceIoControl.IoControlCode,
                                        Request,
                                        keyParameters,
                                        RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                        outputBuffer,
                                        RequestParameters.Parameters.DeviceIoControl.OutputBufferLength,
                                        DataLength);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DvdStartSessionReadKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  ULONG                    IoControlCode,
    _In_opt_  WDFREQUEST           OriginalRequest,
    _In_opt_  PVOID                InputBuffer,
    _In_  size_t                   InputBufferLength,
    _In_  PVOID                    OutputBuffer,
    _In_  size_t                   OutputBufferLength,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   base function to handle request of IOCTL_DVD_START_SESSION
                                      IOCTL_DVD_READ_KEY

Arguments:

    DeviceExtension - device context
    IoControlCode - IOCTL_DVD_READ_KEY or IOCTL_DVD_START_SESSION
    OriginalRequest - original request to be handled
    InputBuffer - input buffer
    InputBufferLength - length of input buffer
    OutputBuffer - output buffer
    OutputBufferLength - length of output buffer

Return Value:

    NTSTATUS
    DataLength - returned data length

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    ULONG                   keyLength = 0;
    ULONG                   result = 0;
    ULONG                   allocationLength;
    PFOUR_BYTE              fourByte;
    PDVD_COPY_PROTECT_KEY   keyParameters = (PDVD_COPY_PROTECT_KEY)InputBuffer;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER(InputBufferLength);

    *DataLength = 0;

    fourByte = (PFOUR_BYTE)&allocationLength;

    if (IoControlCode == IOCTL_DVD_READ_KEY)
    {
        if (keyParameters == NULL)
        {
            status = STATUS_INTERNAL_ERROR;
        }

        // First of all, initialize the DVD region of the drive, if it has not been set yet
        if (NT_SUCCESS(status) &&
            (keyParameters->KeyType == DvdGetRpcKey) &&
            DeviceExtension->DeviceAdditionalData.Mmc.IsCssDvd)
        {
            DevicePickDvdRegion(DeviceExtension->Device);
        }

        if (NT_SUCCESS(status) &&
            (keyParameters->KeyType == DvdDiskKey))
        {
            // Special case - need to use READ DVD STRUCTURE command to get the disk key.
            PDVD_COPY_PROTECT_KEY   keyHeader = NULL;
            PDVD_READ_STRUCTURE     readStructureRequest = (PDVD_READ_STRUCTURE)keyParameters;

            // save the key header so we can restore the interesting parts later
            keyHeader = ExAllocatePoolWithTag(NonPagedPoolNx,
                                              sizeof(DVD_COPY_PROTECT_KEY),
                                              DVD_TAG_READ_KEY);

            if(keyHeader == NULL)
            {
                // Can't save the context so return an error
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                            "DvdDeviceControl - READ_KEY: unable to allocate context\n"));
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(status))
            {
                PREAD_DVD_STRUCTURES_HEADER rawKey = OutputBuffer;
                PDVD_COPY_PROTECT_KEY       outputKey = OutputBuffer;

                // save input parameters
                RtlCopyMemory(keyHeader,
                              InputBuffer,
                              sizeof(DVD_COPY_PROTECT_KEY));

                readStructureRequest->Format = DvdDiskKeyDescriptor;
                readStructureRequest->BlockByteOffset.QuadPart = 0;
                readStructureRequest->LayerNumber = 0;
                readStructureRequest->SessionId = keyHeader->SessionId;

                status = ReadDvdStructure(DeviceExtension,
                                          OriginalRequest,
                                          InputBuffer,
                                          sizeof(DVD_READ_STRUCTURE),
                                          OutputBuffer,
                                          sizeof(READ_DVD_STRUCTURES_HEADER) + sizeof(DVD_DISK_KEY_DESCRIPTOR),
                                          DataLength);

                // fill the output buffer, it's not touched in DeviceHandleReadDvdStructure()
                // for this specific request type: DvdDiskKeyDescriptor
                if (NT_SUCCESS(status))
                {
                    // Shift the data down to its new position.
                    RtlMoveMemory(outputKey->KeyData,
                                  rawKey->Data,
                                  sizeof(DVD_DISK_KEY_DESCRIPTOR));

                    RtlCopyMemory(outputKey,
                                  keyHeader,
                                  sizeof(DVD_COPY_PROTECT_KEY));

                    outputKey->KeyLength = DVD_DISK_KEY_LENGTH;

                    *DataLength = DVD_DISK_KEY_LENGTH;
                }
                else
                {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                                "StartSessionReadKey Failed with status %x, %xI64 (%x) bytes\n",
                                status,
                                (unsigned int)*DataLength,
                                ((rawKey->Length[0] << 16) | rawKey->Length[1]) ));
                }

                FREE_POOL(keyHeader);
            }

            // special process finished. return from here.
            return status;
        }

        if (NT_SUCCESS(status))
        {
            status = RtlULongSub((ULONG)OutputBufferLength,
                                 (ULONG)sizeof(DVD_COPY_PROTECT_KEY), &result);
        }

        if (NT_SUCCESS(status))
        {
            status = RtlULongAdd(result, sizeof(CDVD_KEY_HEADER), &keyLength);
        }

        if (NT_SUCCESS(status))
        {
            //The data length field of REPORT KEY Command occupies two bytes
            keyLength = min(keyLength, MAXUSHORT);
        }
    }
    else    //IOCTL_DVD_START_SESSION
    {
        keyParameters = NULL;
        keyLength = sizeof(CDVD_KEY_HEADER) + sizeof(CDVD_REPORT_AGID_DATA);
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        CDB cdb;

        allocationLength = keyLength;

        // Defensive coding. Prefix cannot recognize this usage.
        UNREFERENCED_PARAMETER(allocationLength);

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.REPORT_KEY.OperationCode = SCSIOP_REPORT_KEY;
        cdb.REPORT_KEY.AllocationLength[0] = fourByte->Byte1;
        cdb.REPORT_KEY.AllocationLength[1] = fourByte->Byte0;

        // set the specific parameters....
        if(IoControlCode == IOCTL_DVD_READ_KEY)
        {
            if(keyParameters->KeyType == DvdTitleKey)
            {
                ULONG logicalBlockAddress;

                logicalBlockAddress = (ULONG)(keyParameters->Parameters.TitleOffset.QuadPart >>
                                              DeviceExtension->SectorShift);

                fourByte = (PFOUR_BYTE)&(logicalBlockAddress);

                cdb.REPORT_KEY.LogicalBlockAddress[0] = fourByte->Byte3;
                cdb.REPORT_KEY.LogicalBlockAddress[1] = fourByte->Byte2;
                cdb.REPORT_KEY.LogicalBlockAddress[2] = fourByte->Byte1;
                cdb.REPORT_KEY.LogicalBlockAddress[3] = fourByte->Byte0;
            }

            cdb.REPORT_KEY.KeyFormat = (UCHAR)keyParameters->KeyType;
            cdb.REPORT_KEY.AGID = (UCHAR)keyParameters->SessionId;
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                        "DvdStartSessionReadKey => sending irp %p (%s)\n",
                        OriginalRequest, "READ_KEY"));
        }
        else
        {
            cdb.REPORT_KEY.KeyFormat = DVD_REPORT_AGID;
            cdb.REPORT_KEY.AGID = 0;
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                        "DvdStartSessionReadKey => sending irp %p (%s)\n",
                        OriginalRequest, "START_SESSION"));
        }

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, OriginalRequest, keyLength, TRUE, &cdb, 12);

        if (NT_SUCCESS(status))
        {
            if(IoControlCode == IOCTL_DVD_READ_KEY)
            {
                NTSTATUS                tempStatus;
                PDVD_COPY_PROTECT_KEY   copyProtectKey = (PDVD_COPY_PROTECT_KEY)OutputBuffer;
                PCDVD_KEY_HEADER        keyHeader = DeviceExtension->ScratchContext.ScratchSrb->DataBuffer;
                ULONG                   dataLength;
                ULONG                   transferLength;

                tempStatus = RtlULongSub(DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength,
                                         FIELD_OFFSET(CDVD_KEY_HEADER, Data),
                                         &transferLength);

                dataLength = (keyHeader->DataLength[0] << 8) + keyHeader->DataLength[1];

                if (NT_SUCCESS(tempStatus) && (dataLength >= 2))
                {
                    // Adjust the data length to ignore the two reserved bytes in the
                    // header.
                    dataLength -= 2;

                    // take the minimum of the transferred length and the
                    // length as specified in the header.
                    if(dataLength < transferLength)
                    {
                        transferLength = dataLength;
                    }

                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                                "DvdDeviceControlCompletion: [%p] - READ_KEY with "
                                "transfer length of (%d or %d) bytes\n",
                                OriginalRequest,
                                dataLength,
                                DeviceExtension->ScratchContext.ScratchSrb->DataTransferLength - 2));

                    // Copy the key data into the return buffer
                    if(copyProtectKey->KeyType == DvdTitleKey)
                    {
                        RtlMoveMemory(copyProtectKey->KeyData,
                                      keyHeader->Data + 1,
                                      transferLength - 1);

                        copyProtectKey->KeyData[transferLength - 1] = 0;

                        // If this is a title key then we need to copy the CGMS flags
                        // as well.
                        copyProtectKey->KeyFlags = *(keyHeader->Data);

                    }
                    else
                    {
                        RtlMoveMemory(copyProtectKey->KeyData,
                                      keyHeader->Data,
                                      transferLength);
                    }

                    copyProtectKey->KeyLength = sizeof(DVD_COPY_PROTECT_KEY);
                    copyProtectKey->KeyLength += transferLength;

                    *DataLength = copyProtectKey->KeyLength;
                }
                else
                {
                    //There is no valid data from drive.
                    //This may happen when Key Format = 0x3f that does not require data back from drive.
                    status = STATUS_SUCCESS;
                    *DataLength = 0;
                }
            }
            else
            {
                PDVD_SESSION_ID         sessionId = (PDVD_SESSION_ID)OutputBuffer;
                PCDVD_KEY_HEADER        keyHeader = DeviceExtension->ScratchContext.ScratchSrb->DataBuffer;
                PCDVD_REPORT_AGID_DATA  keyData = (PCDVD_REPORT_AGID_DATA)keyHeader->Data;

                *sessionId = keyData->AGID;

                *DataLength = sizeof(DVD_SESSION_ID);
            }
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleDvdSendKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_DVD_SEND_KEY
                     IOCTL_DVD_SEND_KEY2

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PVOID                   inputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           &inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        status = DvdSendKey(DeviceExtension,
                            Request,
                            inputBuffer,
                            RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                            DataLength);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DvdSendKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_opt_  WDFREQUEST           OriginalRequest,
    _In_  PVOID                    InputBuffer,
    _In_  size_t                   InputBufferLength,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   base function to handle request of IOCTL_DVD_SEND_KEY(2)
   NOTE: cdrom does not process this IOCTL if the input buffer length is bigger than Port transfer length.

Arguments:

    DeviceExtension - device context
    OriginalRequest - original request to be handled
    InputBuffer - input buffer
    InputBufferLength - length of input buffer

Return Value:

    NTSTATUS
    DataLength - returned data length

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDVD_COPY_PROTECT_KEY   key = (PDVD_COPY_PROTECT_KEY)InputBuffer;

    ULONG                   keyLength = 0;
    ULONG                   result = 0;
    PFOUR_BYTE              fourByte;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER(InputBufferLength);

    *DataLength = 0;

    if (NT_SUCCESS(status))
    {
        if ((key->KeyLength < sizeof(DVD_COPY_PROTECT_KEY)) ||
            ((key->KeyLength - sizeof(DVD_COPY_PROTECT_KEY)) > DeviceExtension->ScratchContext.ScratchBufferSize))
        {
            NT_ASSERT(FALSE);
            status = STATUS_INTERNAL_ERROR;
        }
    }

    if (NT_SUCCESS(status))
    {
        status = RtlULongSub(key->KeyLength, sizeof(DVD_COPY_PROTECT_KEY), &result);
    }

    if (NT_SUCCESS(status))
    {
        status = RtlULongAdd(result, sizeof(CDVD_KEY_HEADER), &keyLength);
    }

    if (NT_SUCCESS(status))
    {
        keyLength = min(keyLength, DeviceExtension->ScratchContext.ScratchBufferSize);

        if (keyLength < 2)
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status))
    {
        PCDVD_KEY_HEADER            keyBuffer = NULL;
        CDB                         cdb;

        ScratchBuffer_BeginUse(DeviceExtension);

        // prepare the input buffer
        keyBuffer = (PCDVD_KEY_HEADER)DeviceExtension->ScratchContext.ScratchBuffer;

        // keylength is decremented here by two because the
        // datalength does not include the header, which is two
        // bytes.  keylength is immediately incremented later
        // by the same amount.
        keyLength -= 2;
        fourByte = (PFOUR_BYTE)&keyLength;
        keyBuffer->DataLength[0] = fourByte->Byte1;
        keyBuffer->DataLength[1] = fourByte->Byte0;
        keyLength += 2;

        RtlMoveMemory(keyBuffer->Data,
                      key->KeyData,
                      key->KeyLength - sizeof(DVD_COPY_PROTECT_KEY));

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.REPORT_KEY.OperationCode = SCSIOP_SEND_KEY;

        cdb.SEND_KEY.ParameterListLength[0] = fourByte->Byte1;
        cdb.SEND_KEY.ParameterListLength[1] = fourByte->Byte0;
        cdb.SEND_KEY.KeyFormat = (UCHAR)key->KeyType;
        cdb.SEND_KEY.AGID = (UCHAR)key->SessionId;

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, OriginalRequest, keyLength, FALSE, &cdb, 12);

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleSetReadAhead(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_STORAGE_SET_READ_AHEAD

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PSTORAGE_SET_READ_AHEAD readAhead = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&readAhead,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        ULONG       transferSize = 0;
        CDB         cdb;
        ULONG       blockAddress;
        PFOUR_BYTE  fourByte = (PFOUR_BYTE)&blockAddress;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        cdb.SET_READ_AHEAD.OperationCode = SCSIOP_SET_READ_AHEAD;

        blockAddress = (ULONG)(readAhead->TriggerAddress.QuadPart >>
                               DeviceExtension->SectorShift);

        // Defensive coding. Prefix cannot recognize this usage.
        UNREFERENCED_PARAMETER(blockAddress);

        cdb.SET_READ_AHEAD.TriggerLBA[0] = fourByte->Byte3;
        cdb.SET_READ_AHEAD.TriggerLBA[1] = fourByte->Byte2;
        cdb.SET_READ_AHEAD.TriggerLBA[2] = fourByte->Byte1;
        cdb.SET_READ_AHEAD.TriggerLBA[3] = fourByte->Byte0;

        blockAddress = (ULONG)(readAhead->TargetAddress.QuadPart >>
                               DeviceExtension->SectorShift);

        cdb.SET_READ_AHEAD.ReadAheadLBA[0] = fourByte->Byte3;
        cdb.SET_READ_AHEAD.ReadAheadLBA[1] = fourByte->Byte2;
        cdb.SET_READ_AHEAD.ReadAheadLBA[2] = fourByte->Byte1;
        cdb.SET_READ_AHEAD.ReadAheadLBA[3] = fourByte->Byte0;

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 12);

        if (NT_SUCCESS(status))
        {
            *DataLength = 0;
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleSetSpeed(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  WDFREQUEST               Request,
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    )
/*++

Routine Description:

   Handle request of IOCTL_CDROM_SET_SPEED

Arguments:

    DeviceExtension - device context
    Request - request to be handled
    RequestParameters - request parameter
    DataLength - transferred data length

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PCDROM_DATA         cdData = &(DeviceExtension->DeviceAdditionalData);
    PCDROM_SET_SPEED    inputBuffer = NULL;

    PAGED_CODE ();

    *DataLength = 0;

    status = WdfRequestRetrieveInputBuffer(Request,
                                           RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                           (PVOID*)&inputBuffer,
                                           NULL);

    if (NT_SUCCESS(status))
    {
        ULONG               transferSize = 0;
        CDB                 cdb;
        CDROM_SPEED_REQUEST requestType = inputBuffer->RequestType;

        ScratchBuffer_BeginUse(DeviceExtension);

        RtlZeroMemory(&cdb, sizeof(CDB));
        // Set up the CDB
        if (requestType == CdromSetSpeed)
        {
            PCDROM_SET_SPEED speed = inputBuffer;

            cdb.SET_CD_SPEED.OperationCode = SCSIOP_SET_CD_SPEED;
            cdb.SET_CD_SPEED.RotationControl = speed->RotationControl;
            REVERSE_BYTES_SHORT(&cdb.SET_CD_SPEED.ReadSpeed, &speed->ReadSpeed);
            REVERSE_BYTES_SHORT(&cdb.SET_CD_SPEED.WriteSpeed, &speed->WriteSpeed);
        }
        else
        {
            PCDROM_SET_STREAMING    stream = (PCDROM_SET_STREAMING)inputBuffer;
            PPERFORMANCE_DESCRIPTOR perfDescriptor;

            transferSize = sizeof(PERFORMANCE_DESCRIPTOR);

            perfDescriptor = DeviceExtension->ScratchContext.ScratchBuffer;
            RtlZeroMemory(perfDescriptor, transferSize);

            perfDescriptor->RandomAccess = stream->RandomAccess;
            perfDescriptor->Exact = stream->SetExact;
            perfDescriptor->RestoreDefaults = stream->RestoreDefaults;
            perfDescriptor->WriteRotationControl = stream->RotationControl;

            REVERSE_BYTES(&perfDescriptor->StartLba,  &stream->StartLba);
            REVERSE_BYTES(&perfDescriptor->EndLba,    &stream->EndLba);
            REVERSE_BYTES(&perfDescriptor->ReadSize,  &stream->ReadSize);
            REVERSE_BYTES(&perfDescriptor->ReadTime,  &stream->ReadTime);
            REVERSE_BYTES(&perfDescriptor->WriteSize, &stream->WriteSize);
            REVERSE_BYTES(&perfDescriptor->WriteTime, &stream->WriteTime);

            cdb.SET_STREAMING.OperationCode = SCSIOP_SET_STREAMING;
            REVERSE_BYTES_SHORT(&cdb.SET_STREAMING.ParameterListLength, &transferSize);

            // set value in extension by user inputs.
            cdData->RestoreDefaults = stream->Persistent ? FALSE : TRUE;

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DeviceHandleSetSpeed: Restore default speed on media change set to %s\n",
                       cdData->RestoreDefaults ? "true" : "false"));
        }

        status = ScratchBuffer_ExecuteCdb(DeviceExtension, Request, transferSize, FALSE, &cdb, 12);

        if (NT_SUCCESS(status))
        {
            *DataLength = 0;
        }

        // nothing to do after the command finishes.
        ScratchBuffer_EndUse(DeviceExtension);
    }

    return status;
}


