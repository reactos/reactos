/*--

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    ioctl.c

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

#if DBG
    PUCHAR READ_DVD_STRUCTURE_FORMAT_STRINGS[DvdMaxDescriptor+1] = {
        "Physical",
        "Copyright",
        "DiskKey",
        "BCA",
        "Manufacturer",
        "Unknown"
    };
#endif // DBG

#define DEFAULT_CDROM_SECTORS_PER_TRACK 32
#define DEFAULT_TRACKS_PER_CYLINDER     64

NTSTATUS
NTAPI
CdRomDeviceControlDispatch(
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
    PFUNCTIONAL_DEVICE_EXTENSION  fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextStack;
    PCDROM_DATA        cdData = (PCDROM_DATA)(commonExtension->DriverData);

    //BOOLEAN            use6Byte = TEST_FLAG(cdData->XAFlags, XA_USE_6_BYTE);
    SCSI_REQUEST_BLOCK srb;
    PCDB cdb = (PCDB)srb.Cdb;
    //PVOID outputBuffer;
    //ULONG bytesTransferred = 0;
    NTSTATUS status;
    //NTSTATUS status2;
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

    TraceLog((CdromDebugTrace,
                "CdRomDeviceControl: Ioctl Code = %lx, Base Code = %lx,"
                " Function Code = %lx\n",
                ioctlCode,
                baseCode,
                functionCode
              ));

    if((functionCode >= 0x200) && (functionCode <= 0x300)) {

        ioctlCode = (ioctlCode & 0x0000ffff) | CTL_CODE(IOCTL_CDROM_BASE, 0, 0, 0);

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Class Code - new ioctl code is %lx\n",
                    ioctlCode));

        irpStack->Parameters.DeviceIoControl.IoControlCode = ioctlCode;

    }

    switch (ioctlCode) {

    case IOCTL_STORAGE_GET_MEDIA_TYPES_EX: {

        PGET_MEDIA_TYPES  mediaTypes = Irp->AssociatedIrp.SystemBuffer;
        PDEVICE_MEDIA_INFO mediaInfo = &mediaTypes->MediaInfo[0];
        ULONG sizeNeeded;

        sizeNeeded = sizeof(GET_MEDIA_TYPES);
        
        //
        // IsMmc is static...
        //

        if (cdData->Mmc.IsMmc) {
            sizeNeeded += sizeof(DEVICE_MEDIA_INFO) * 1; // return two media types
        }

        //
        // Ensure that buffer is large enough.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeNeeded) {

            //
            // Buffer too small.
            //

            Irp->IoStatus.Information = sizeNeeded;
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        
        RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, sizeNeeded);

        //
        // ISSUE-2000/5/11-henrygab - need to update GET_MEDIA_TYPES_EX
        //

        mediaTypes->DeviceType = CdRomGetDeviceType(DeviceObject);

        mediaTypes->MediaInfoCount = 1;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType = CD_ROM;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.NumberMediaSides = 1;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics = MEDIA_READ_ONLY;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.Cylinders.QuadPart = fdoExtension->DiskGeometry.Cylinders.QuadPart;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.TracksPerCylinder = fdoExtension->DiskGeometry.TracksPerCylinder;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.SectorsPerTrack = fdoExtension->DiskGeometry.SectorsPerTrack;
        mediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector = fdoExtension->DiskGeometry.BytesPerSector;

        if (cdData->Mmc.IsMmc) {
            
            //
            // also report a removable disk
            //
            mediaTypes->MediaInfoCount += 1;
            
            mediaInfo++;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType = RemovableMedia;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.NumberMediaSides = 1;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics = MEDIA_READ_WRITE;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.Cylinders.QuadPart = fdoExtension->DiskGeometry.Cylinders.QuadPart;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.TracksPerCylinder = fdoExtension->DiskGeometry.TracksPerCylinder;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.SectorsPerTrack = fdoExtension->DiskGeometry.SectorsPerTrack;
            mediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector = fdoExtension->DiskGeometry.BytesPerSector;
            mediaInfo--;
        
        }

        //
        // Status will either be success, if media is present, or no media.
        // It would be optimal to base from density code and medium type, but not all devices
        // have values for these fields.
        //

        //
        // Send a TUR to determine if media is present.
        //

        srb.CdbLength = 6;
        cdb = (PCDB)srb.Cdb;
        cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

        //
        // Set timeout value.
        //

        srb.TimeOutValue = fdoExtension->TimeOutValue;

        status = ClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);


        TraceLog((CdromDebugWarning,
                   "CdRomDeviceControl: GET_MEDIA_TYPES status of TUR - %lx\n",
                   status));

        if (NT_SUCCESS(status)) {

            //
            // set the disk's media as current if we can write to it.
            //

            if (cdData->Mmc.IsMmc && cdData->Mmc.WriteAllowed) {

                mediaInfo++;
                SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics,
                         MEDIA_CURRENTLY_MOUNTED);
                mediaInfo--;


            } else {

                SET_FLAG(mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics,
                         MEDIA_CURRENTLY_MOUNTED);

            }

        }

        Irp->IoStatus.Information = sizeNeeded;
        status = STATUS_SUCCESS;
        break;
    }


    case IOCTL_CDROM_RAW_READ: {

        LARGE_INTEGER  startingOffset;
        ULONGLONG      transferBytes;
        ULONGLONG      endOffset;
        ULONGLONG      mdlBytes;
        //ULONG          startingSector;
        PRAW_READ_INFO rawReadInfo = (PRAW_READ_INFO)irpStack->Parameters.DeviceIoControl.Type3InputBuffer;

        //
        // Ensure that XA reads are supported.
        //

        if (TEST_FLAG(cdData->XAFlags, XA_NOT_SUPPORTED)) {
            TraceLog((CdromDebugWarning,
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
            // Called from user space. Save the userbuffer in the 
            // Type3InputBuffer so we can reduce the number of code paths.
            //

            irpStack->Parameters.DeviceIoControl.Type3InputBuffer =
                Irp->AssociatedIrp.SystemBuffer;

            //
            // Called from user space.  Validate the buffers.
            //

            rawReadInfo = (PRAW_READ_INFO)irpStack->Parameters.DeviceIoControl.Type3InputBuffer;

            if (rawReadInfo == NULL) {

                TraceLog((CdromDebugWarning,
                            "CdRomDeviceControl: Invalid I/O parameters for "
                            "XA Read (No extent info\n"));
                status = STATUS_INVALID_PARAMETER;
                break;

            }

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength !=
                sizeof(RAW_READ_INFO)) {

                TraceLog((CdromDebugWarning,
                            "CdRomDeviceControl: Invalid I/O parameters for "
                            "XA Read (Invalid info buffer\n"));
                status = STATUS_INVALID_PARAMETER;
                break;

            }
        }

        //
        // if they don't request any data, just fail the request
        //

        if (rawReadInfo->SectorCount == 0) {

            status = STATUS_INVALID_PARAMETER;
            break;

        }

        startingOffset.QuadPart = rawReadInfo->DiskOffset.QuadPart;
        /* startingSector = (ULONG)(rawReadInfo->DiskOffset.QuadPart >>
                                 fdoExtension->SectorShift); */
        transferBytes = (ULONGLONG)rawReadInfo->SectorCount * RAW_SECTOR_SIZE;
        
        endOffset = (ULONGLONG)rawReadInfo->SectorCount * COOKED_SECTOR_SIZE;
        endOffset += startingOffset.QuadPart;

        //
        // check for overflows....
        //
        
        if (transferBytes < (ULONGLONG)(rawReadInfo->SectorCount)) {
            TraceLog((CdromDebugWarning,
                        "CdRomDeviceControl: Invalid I/O parameters for XA "
                        "Read (TransferBytes Overflow)\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (endOffset < (ULONGLONG)startingOffset.QuadPart) {
            TraceLog((CdromDebugWarning,
                        "CdRomDeviceControl: Invalid I/O parameters for XA "
                        "Read (EndingOffset Overflow)\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            transferBytes) {
            TraceLog((CdromDebugWarning,
                        "CdRomDeviceControl: Invalid I/O parameters for XA "
                        "Read (Bad buffer size)\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (endOffset > (ULONGLONG)commonExtension->PartitionLength.QuadPart) {
            TraceLog((CdromDebugWarning,
                        "CdRomDeviceControl: Invalid I/O parameters for XA "
                        "Read (Request Out of Bounds)\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // cannot validate the MdlAddress, since it is not included in any
        // other location per the DDK and file system calls.
        //

        //
        // validate the mdl describes at least the number of bytes
        // requested from us.
        //

        mdlBytes = (ULONGLONG)MmGetMdlByteCount(Irp->MdlAddress);
        if (mdlBytes < transferBytes) {
            TraceLog((CdromDebugWarning,
                        "CdRomDeviceControl: Invalid MDL %s, Irp %p\n",
                        "size (5)", Irp));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

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
        // once XA_READ is removed, then this hack can also be
        // removed.
        //
        irpStack->MinorFunction = MAXIMUM_RETRIES; // HACKHACK - REF #0001

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
    case IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX: {
        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Get drive geometryEx\n"));
        if ( irpStack->Parameters.DeviceIoControl.OutputBufferLength <
             FIELD_OFFSET(DISK_GEOMETRY_EX, Data)) {
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = FIELD_OFFSET(DISK_GEOMETRY_EX, Data);
            break;
        }
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;
    }

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    case IOCTL_CDROM_GET_DRIVE_GEOMETRY: {

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Get drive geometry\n"));

        if ( irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof( DISK_GEOMETRY ) ) {

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_READ_TOC_EX: {

        PCDROM_READ_TOC_EX inputBuffer;
        
        if (CdRomIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            break;
        }

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_READ_TOC_EX)) {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            MINIMUM_CDROM_READ_TOC_EX_SIZE) {
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = MINIMUM_CDROM_READ_TOC_EX_SIZE;
            break;
        }

        if (irpStack->Parameters.Read.Length > ((USHORT)-1)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        inputBuffer = Irp->AssociatedIrp.SystemBuffer;

        if ((inputBuffer->Reserved1 != 0) ||
            (inputBuffer->Reserved2 != 0) ||
            (inputBuffer->Reserved3 != 0)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // NOTE: when adding new formats, ensure that first two bytes
        //       specify the amount of additional data available.
        //

        if ((inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_TOC     ) ||
            (inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_FULL_TOC) ||
            (inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_CDTEXT  )) {
            
            // SessionTrack field is used

        } else
        if ((inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_SESSION) ||
            (inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_PMA)     ||
            (inputBuffer->Format == CDROM_READ_TOC_EX_FORMAT_ATIP)) {
            
            // SessionTrack field is reserved
            
            if (inputBuffer->SessionTrack != 0) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            
        } else {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;
    }

    case IOCTL_CDROM_GET_LAST_SESSION: {

        //
        // If the cd is playing music then reject this request.
        //

        if (CdRomIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            break;
        }

        //
        // Make sure the caller is requesting enough data to make this worth
        // our while.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(CDROM_TOC_SESSION_DATA)) {

            //
            // they didn't request the entire TOC -- use _EX version
            // for partial transfers and such.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(CDROM_TOC_SESSION_DATA);
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_READ_TOC:  {

        //
        // If the cd is playing music then reject this request.
        //

        if (CdRomIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            break;
        }

        //
        // Make sure the caller is requesting enough data to make this worth
        // our while.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(CDROM_TOC)) {

            //
            // they didn't request the entire TOC -- use _EX version
            // for partial transfers and such.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(CDROM_TOC);
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_PLAY_AUDIO_MSF: {

        //
        // Play Audio MSF
        //

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Play audio MSF\n"));

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_PLAY_AUDIO_MSF)) {

            //
            // Indicate unsuccessful status.
            //

            status = STATUS_INFO_LENGTH_MISMATCH;
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

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Seek audio MSF\n"));

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_SEEK_AUDIO_MSF)) {

            //
            // Indicate unsuccessful status.
            //

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;

    }

    case IOCTL_CDROM_PAUSE_AUDIO: {

        //
        // Pause audio
        //

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Pause audio\n"));

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;

        break;
    }

    case IOCTL_CDROM_RESUME_AUDIO: {

        //
        // Resume audio
        //

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Resume audio\n"));

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_READ_Q_CHANNEL: {

        PCDROM_SUB_Q_DATA_FORMAT inputBuffer =
                         Irp->AssociatedIrp.SystemBuffer;

        if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_SUB_Q_DATA_FORMAT)) {

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        //
        // check for all valid types of request
        //

        if (inputBuffer->Format != IOCTL_CDROM_CURRENT_POSITION &&
            inputBuffer->Format != IOCTL_CDROM_MEDIA_CATALOG &&
            inputBuffer->Format != IOCTL_CDROM_TRACK_ISRC ) {
            status = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_GET_CONTROL: {

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Get audio control\n"));

        //
        // Verify user buffer is large enough for the data.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(CDROM_AUDIO_CONTROL)) {

            //
            // Indicate unsuccessful status and no data transferred.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(CDROM_AUDIO_CONTROL);
            break;

        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_GET_VOLUME: {

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Get volume control\n"));

        //
        // Verify user buffer is large enough for data.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(VOLUME_CONTROL)) {

            //
            // Indicate unsuccessful status and no data transferred.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(VOLUME_CONTROL);
            break;

        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_SET_VOLUME: {

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Set volume control\n"));

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(VOLUME_CONTROL)) {

            //
            // Indicate unsuccessful status.
            //

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;

        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_CDROM_STOP_AUDIO: {

        //
        // Stop play.
        //

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Stop audio\n"));

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_DISK_CHECK_VERIFY:
    case IOCTL_CDROM_CHECK_VERIFY: {

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: [%p] Check Verify\n", Irp));

        if((irpStack->Parameters.DeviceIoControl.OutputBufferLength) &&
           (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))) {

           TraceLog((CdromDebugWarning,
                       "CdRomDeviceControl: Check Verify: media count "
                       "buffer too small\n"));

           status = STATUS_BUFFER_TOO_SMALL;
           Irp->IoStatus.Information = sizeof(ULONG);
           break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_DVD_READ_STRUCTURE: {

        TraceLog((CdromDebugTrace,
                    "DvdDeviceControl: [%p] IOCTL_DVD_READ_STRUCTURE\n", Irp));

        if (cdData->DvdRpc0Device && cdData->DvdRpc0LicenseFailure) {
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: License Failure\n"));
            status = STATUS_COPY_PROTECTION_FAILURE;
            break;
        }

        if (cdData->DvdRpc0Device && cdData->Rpc0RetryRegistryCallback) {
            //
            // if currently in-progress, this will just return.
            // prevents looping by doing that interlockedExchange()
            //
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: PickRegion() from "
                        "READ_STRUCTURE\n"));
            CdRomPickDvdRegion(DeviceObject);
        }


        if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
           sizeof(DVD_READ_STRUCTURE)) {

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl - READ_STRUCTURE: input buffer "
                        "length too small (was %d should be %d)\n",
                        irpStack->Parameters.DeviceIoControl.InputBufferLength,
                        sizeof(DVD_READ_STRUCTURE)));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if(irpStack->Parameters.DeviceIoControl.OutputBufferLength <
           sizeof(READ_DVD_STRUCTURES_HEADER)) {

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl - READ_STRUCTURE: output buffer "
                        "cannot hold header information\n"));
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(READ_DVD_STRUCTURES_HEADER);            
            break;
        }

        if(irpStack->Parameters.DeviceIoControl.OutputBufferLength >
           MAXUSHORT) {

            //
            // key length must fit in two bytes
            //
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl - READ_STRUCTURE: output buffer "
                        "too large\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_DVD_START_SESSION: {

        TraceLog((CdromDebugTrace,
                    "DvdDeviceControl: [%p] IOCTL_DVD_START_SESSION\n", Irp));

        if (cdData->DvdRpc0Device && cdData->DvdRpc0LicenseFailure) {
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: License Failure\n"));
            status = STATUS_COPY_PROTECTION_FAILURE;
            break;
        }

        if(irpStack->Parameters.DeviceIoControl.OutputBufferLength <
           sizeof(DVD_SESSION_ID)) {

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: DVD_START_SESSION - output "
                        "buffer too small\n"));
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(DVD_SESSION_ID);
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_DVD_SEND_KEY:
    case IOCTL_DVD_SEND_KEY2: {

        PDVD_COPY_PROTECT_KEY key = Irp->AssociatedIrp.SystemBuffer;
        //ULONG keyLength;

        TraceLog((CdromDebugTrace,
                    "DvdDeviceControl: [%p] IOCTL_DVD_SEND_KEY\n", Irp));

        if (cdData->DvdRpc0Device && cdData->DvdRpc0LicenseFailure) {
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: License Failure\n"));
            status = STATUS_COPY_PROTECTION_FAILURE;
            break;
        }

        if((irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(DVD_COPY_PROTECT_KEY)) ||
           (irpStack->Parameters.DeviceIoControl.InputBufferLength !=
            key->KeyLength)) {

            //
            // Key is too small to have a header or the key length doesn't
            // match the input buffer length.  Key must be invalid
            //

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: [%p] IOCTL_DVD_SEND_KEY - "
                        "key is too small or does not match KeyLength\n",
                        Irp));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // allow only certain key type (non-destructive) to go through
        // IOCTL_DVD_SEND_KEY (which only requires READ access to the device
        //
        if (ioctlCode == IOCTL_DVD_SEND_KEY) {

            if ((key->KeyType != DvdChallengeKey) &&
                (key->KeyType != DvdBusKey2) &&
                (key->KeyType != DvdInvalidateAGID)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }
        }

        if (cdData->DvdRpc0Device) {

            if (key->KeyType == DvdSetRpcKey) {

                PDVD_SET_RPC_KEY rpcKey = (PDVD_SET_RPC_KEY) key->KeyData;

                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    DVD_SET_RPC_KEY_LENGTH) {

                    status = STATUS_INVALID_PARAMETER;
                    break;
                }

                //
                // we have a request to set region code
                // on a RPC0 device which doesn't support
                // region coding.
                //
                // we have to fake it.
                //

                KeWaitForMutexObject(
                    &cdData->Rpc0RegionMutex,
                    UserRequest,
                    KernelMode,
                    FALSE,
                    NULL
                    );

                if (cdData->DvdRpc0Device && cdData->Rpc0RetryRegistryCallback) {
                    //
                    // if currently in-progress, this will just return.
                    // prevents looping by doing that interlockedExchange()
                    //
                    TraceLog((CdromDebugWarning,
                                "DvdDeviceControl: PickRegion() from "
                                "SEND_KEY\n"));
                    CdRomPickDvdRegion(DeviceObject);
                }

                if (cdData->Rpc0SystemRegion == rpcKey->PreferredDriveRegionCode) {

                    //
                    // nothing to change
                    //
                    TraceLog((CdromDebugWarning,
                                "DvdDeviceControl (%p) => not changing "
                                "regions -- requesting current region\n",
                                DeviceObject));
                    status = STATUS_SUCCESS;

                } else if (cdData->Rpc0SystemRegionResetCount == 0) {

                    //
                    // not allowed to change it again
                    //

                    TraceLog((CdromDebugWarning,
                                "DvdDeviceControl (%p) => no more region "
                                "changes are allowed for this device\n",
                                DeviceObject));
                    status = STATUS_CSS_RESETS_EXHAUSTED;

                } else {

                    //ULONG i;
                    UCHAR mask;
                    ULONG bufferLen;
                    PDVD_READ_STRUCTURE dvdReadStructure;
                    PDVD_COPYRIGHT_DESCRIPTOR dvdCopyRight;
                    IO_STATUS_BLOCK ioStatus;
                    UCHAR mediaRegionData;

                    mask = ~rpcKey->PreferredDriveRegionCode;

                    if (CountOfSetBitsUChar(mask) != 1) {

                        status = STATUS_INVALID_DEVICE_REQUEST;
                        break;
                    }

                    //
                    // this test will always be TRUE except during initial
                    // automatic selection of the first region.
                    //

                    if (cdData->Rpc0SystemRegion != 0xff) {

                        //
                        // make sure we have a media in the drive with the same
                        // region code if the drive is already has a region set
                        //

                        TraceLog((CdromDebugTrace,
                                    "DvdDeviceControl (%p) => Checking "
                                    "media region\n",
                                    DeviceObject));

                        bufferLen = max(sizeof(DVD_DESCRIPTOR_HEADER) +
                                            sizeof(DVD_COPYRIGHT_DESCRIPTOR),
                                        sizeof(DVD_READ_STRUCTURE)
                                        );

                        dvdReadStructure = (PDVD_READ_STRUCTURE)
                            ExAllocatePoolWithTag(PagedPool,
                                                  bufferLen,
                                                  DVD_TAG_RPC2_CHECK);

                        if (dvdReadStructure == NULL) {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            KeReleaseMutex(&cdData->Rpc0RegionMutex,FALSE);
                            break;
                        }

                        dvdCopyRight = (PDVD_COPYRIGHT_DESCRIPTOR)
                            ((PDVD_DESCRIPTOR_HEADER) dvdReadStructure)->Data;

                        //
                        // check to see if we have a DVD device
                        //

                        RtlZeroMemory (dvdReadStructure, bufferLen);
                        dvdReadStructure->Format = DvdCopyrightDescriptor;

                        //
                        // Build a request for READ_KEY
                        //
                        ClassSendDeviceIoControlSynchronous(
                            IOCTL_DVD_READ_STRUCTURE,
                            DeviceObject,
                            dvdReadStructure,
                            sizeof(DVD_READ_STRUCTURE),
                            sizeof(DVD_DESCRIPTOR_HEADER) +
                                sizeof(DVD_COPYRIGHT_DESCRIPTOR),
                            FALSE,
                            &ioStatus);

                        //
                        // this is just to prevent bugs from creeping in
                        // if status is not set later in development
                        //

                        status = ioStatus.Status;

                        //
                        // handle errors
                        //

                        if (!NT_SUCCESS(status)) {
                            KeReleaseMutex(&cdData->Rpc0RegionMutex,FALSE);
                            ExFreePool(dvdReadStructure);
                            status = STATUS_INVALID_DEVICE_REQUEST;
                            break;
                        }

                        //
                        // save the mediaRegionData before freeing the
                        // allocated memory
                        //

                        mediaRegionData =
                            dvdCopyRight->RegionManagementInformation;
                        ExFreePool(dvdReadStructure);

                        TraceLog((CdromDebugWarning,
                                    "DvdDeviceControl (%p) => new mask is %x"
                                    " MediaRegionData is %x\n", DeviceObject,
                                    rpcKey->PreferredDriveRegionCode,
                                    mediaRegionData));

                        //
                        // the media region must match the requested region
                        // for RPC0 drives for initial region selection
                        //

                        if (((UCHAR)~(mediaRegionData | rpcKey->PreferredDriveRegionCode)) == 0) {
                            KeReleaseMutex(&cdData->Rpc0RegionMutex,FALSE);
                            status = STATUS_CSS_REGION_MISMATCH;
                            break;
                        }

                    }

                    //
                    // now try to set the region
                    //

                    TraceLog((CdromDebugTrace,
                                "DvdDeviceControl (%p) => Soft-Setting "
                                "region of RPC1 device to %x\n",
                                DeviceObject,
                                rpcKey->PreferredDriveRegionCode
                                ));

                    status = CdRomSetRpc0Settings(DeviceObject,
                                                  rpcKey->PreferredDriveRegionCode);

                    if (!NT_SUCCESS(status)) {
                        TraceLog((CdromDebugWarning,
                                    "DvdDeviceControl (%p) => Could not "
                                    "set region code (%x)\n",
                                    DeviceObject, status
                                    ));
                    } else {

                        TraceLog((CdromDebugTrace,
                                    "DvdDeviceControl (%p) => New region set "
                                    " for RPC1 drive\n", DeviceObject));

                        //
                        // if it worked, our extension is already updated.
                        // release the mutex
                        //

                        DebugPrint ((4, "DvdDeviceControl (%p) => DVD current "
                                     "region bitmap  0x%x\n", DeviceObject,
                                     cdData->Rpc0SystemRegion));
                        DebugPrint ((4, "DvdDeviceControl (%p) => DVD region "
                                     " reset Count     0x%x\n", DeviceObject,
                                     cdData->Rpc0SystemRegionResetCount));
                    }

                }

                KeReleaseMutex(&cdData->Rpc0RegionMutex,FALSE);
                break;
            } // end of key->KeyType == DvdSetRpcKey
        } // end of Rpc0Device hacks

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;
    }

    case IOCTL_DVD_READ_KEY: {

        PDVD_COPY_PROTECT_KEY keyParameters = Irp->AssociatedIrp.SystemBuffer;
        ULONG keyLength;

        TraceLog((CdromDebugTrace,
                    "DvdDeviceControl: [%p] IOCTL_DVD_READ_KEY\n", Irp));

        if (cdData->DvdRpc0Device && cdData->DvdRpc0LicenseFailure) {
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: License Failure\n"));
            status = STATUS_COPY_PROTECTION_FAILURE;
            break;
        }

        if (cdData->DvdRpc0Device && cdData->Rpc0RetryRegistryCallback) {
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: PickRegion() from READ_KEY\n"));
            CdRomPickDvdRegion(DeviceObject);
        }


        if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(DVD_COPY_PROTECT_KEY)) {

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: EstablishDriveKey - challenge "
                        "key buffer too small\n"));

            status = STATUS_INVALID_PARAMETER;
            break;

        }


        switch(keyParameters->KeyType) {

            case DvdChallengeKey:
                keyLength = DVD_CHALLENGE_KEY_LENGTH;
                break;

            case DvdBusKey1:
            case DvdBusKey2:

                keyLength = DVD_BUS_KEY_LENGTH;
                break;

            case DvdTitleKey:
                keyLength = DVD_TITLE_KEY_LENGTH;
                break;

            case DvdAsf:
                keyLength = DVD_ASF_LENGTH;
                break;

            case DvdDiskKey:
                keyLength = DVD_DISK_KEY_LENGTH;
                break;

            case DvdGetRpcKey:
                keyLength = DVD_RPC_KEY_LENGTH;
                break;

            default:
                keyLength = sizeof(DVD_COPY_PROTECT_KEY);
                break;
        }

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            keyLength) {

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: EstablishDriveKey - output "
                        "buffer too small\n"));
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = keyLength;
            break;
        }

        if (keyParameters->KeyType == DvdGetRpcKey) {

            CdRomPickDvdRegion(DeviceObject);
        }

        if ((keyParameters->KeyType == DvdGetRpcKey) &&
            (cdData->DvdRpc0Device)) {

            PDVD_RPC_KEY rpcKey;
            rpcKey = (PDVD_RPC_KEY)keyParameters->KeyData;
            RtlZeroMemory (rpcKey, sizeof (*rpcKey));

            KeWaitForMutexObject(
                &cdData->Rpc0RegionMutex,
                UserRequest,
                KernelMode,
                FALSE,
                NULL
                );

            //
            // make up the data
            //
            rpcKey->UserResetsAvailable = cdData->Rpc0SystemRegionResetCount;
            rpcKey->ManufacturerResetsAvailable = 0;
            if (cdData->Rpc0SystemRegion == 0xff) {
                rpcKey->TypeCode = 0;
            } else {
                rpcKey->TypeCode = 1;
            }
            rpcKey->RegionMask = (UCHAR) cdData->Rpc0SystemRegion;
            rpcKey->RpcScheme = 1;

            KeReleaseMutex(
                &cdData->Rpc0RegionMutex,
                FALSE
                );

            Irp->IoStatus.Information = DVD_RPC_KEY_LENGTH;
            status = STATUS_SUCCESS;
            break;

        } else if (keyParameters->KeyType == DvdDiskKey) {

            PDVD_COPY_PROTECT_KEY keyHeader;
            PDVD_READ_STRUCTURE readStructureRequest;

            //
            // Special case - build a request to get the dvd structure
            // so we can get the disk key.
            //

            //
            // save the key header so we can restore the interesting
            // parts later
            //

            keyHeader = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(DVD_COPY_PROTECT_KEY),
                                              DVD_TAG_READ_KEY);

            if(keyHeader == NULL) {

                //
                // Can't save the context so return an error
                //

                TraceLog((CdromDebugWarning,
                            "DvdDeviceControl - READ_KEY: unable to "
                            "allocate context\n"));
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlCopyMemory(keyHeader,
                          Irp->AssociatedIrp.SystemBuffer,
                          sizeof(DVD_COPY_PROTECT_KEY));

            IoCopyCurrentIrpStackLocationToNext(Irp);

            nextStack = IoGetNextIrpStackLocation(Irp);

            nextStack->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_DVD_READ_STRUCTURE;

            readStructureRequest = Irp->AssociatedIrp.SystemBuffer;
            readStructureRequest->Format = DvdDiskKeyDescriptor;
            readStructureRequest->BlockByteOffset.QuadPart = 0;
            readStructureRequest->LayerNumber = 0;
            readStructureRequest->SessionId = keyHeader->SessionId;

            nextStack->Parameters.DeviceIoControl.InputBufferLength =
                sizeof(DVD_READ_STRUCTURE);

            nextStack->Parameters.DeviceIoControl.OutputBufferLength =
                sizeof(READ_DVD_STRUCTURES_HEADER) + sizeof(DVD_DISK_KEY_DESCRIPTOR);

            IoSetCompletionRoutine(Irp,
                                   CdRomDvdReadDiskKeyCompletion,
                                   (PVOID) keyHeader,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            {
                UCHAR uniqueAddress;
                ClassAcquireRemoveLock(DeviceObject, (PIRP)&uniqueAddress);
                ClassReleaseRemoveLock(DeviceObject, Irp);

                IoMarkIrpPending(Irp);
                IoCallDriver(commonExtension->DeviceObject, Irp);
                status = STATUS_PENDING;

                ClassReleaseRemoveLock(DeviceObject, (PIRP)&uniqueAddress);
            }

            return STATUS_PENDING;

        } else {

            IoMarkIrpPending(Irp);
            IoStartPacket(DeviceObject, Irp, NULL, NULL);

        }
        return STATUS_PENDING;
    }

    case IOCTL_DVD_END_SESSION: {

        PDVD_SESSION_ID sessionId = Irp->AssociatedIrp.SystemBuffer;

        TraceLog((CdromDebugTrace,
                    "DvdDeviceControl: [%p] END_SESSION\n", Irp));

        if (cdData->DvdRpc0Device && cdData->DvdRpc0LicenseFailure) {
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: License Failure\n"));
            status = STATUS_COPY_PROTECTION_FAILURE;
            break;
        }

        if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(DVD_SESSION_ID)) {

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: EndSession - input buffer too "
                        "small\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        IoMarkIrpPending(Irp);

        if(*sessionId == DVD_END_ALL_SESSIONS) {

            status = CdRomDvdEndAllSessionsCompletion(DeviceObject, Irp, NULL);

            if(status == STATUS_SUCCESS) {

                //
                // Just complete the request - it was never issued to the
                // lower device
                //

                break;

            } else {

                return STATUS_PENDING;

            }
        }

        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_DVD_GET_REGION: {

        PDVD_COPY_PROTECT_KEY copyProtectKey;
        ULONG keyLength;
        IO_STATUS_BLOCK ioStatus;
        PDVD_DESCRIPTOR_HEADER dvdHeader;
        PDVD_COPYRIGHT_DESCRIPTOR copyRightDescriptor;
        PDVD_REGION dvdRegion;
        PDVD_READ_STRUCTURE readStructure;
        PDVD_RPC_KEY rpcKey;

        TraceLog((CdromDebugTrace,
                    "DvdDeviceControl: [%p] IOCTL_DVD_GET_REGION\n", Irp));

        if (cdData->DvdRpc0Device && cdData->DvdRpc0LicenseFailure) {
            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: License Failure\n"));
            status = STATUS_COPY_PROTECTION_FAILURE;
            break;
        }

        if(irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(DVD_REGION)) {

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: output buffer DVD_REGION too small\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // figure out how much data buffer we need
        //

        keyLength = max(sizeof(DVD_DESCRIPTOR_HEADER) +
                            sizeof(DVD_COPYRIGHT_DESCRIPTOR),
                        sizeof(DVD_READ_STRUCTURE)
                        );
        keyLength = max(keyLength,
                        DVD_RPC_KEY_LENGTH
                        );

        //
        // round the size to nearest ULONGLONG -- why?
        //

        keyLength += sizeof(ULONGLONG) - (keyLength & (sizeof(ULONGLONG) - 1));

        readStructure = ExAllocatePoolWithTag(NonPagedPool,
                                              keyLength,
                                              DVD_TAG_READ_KEY);
        if (readStructure == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlZeroMemory (readStructure, keyLength);
        readStructure->Format = DvdCopyrightDescriptor;

        //
        // Build a request for READ_STRUCTURE
        //

        ClassSendDeviceIoControlSynchronous(
            IOCTL_DVD_READ_STRUCTURE,
            DeviceObject,
            readStructure,
            keyLength,
            sizeof(DVD_DESCRIPTOR_HEADER) +
                sizeof(DVD_COPYRIGHT_DESCRIPTOR),
            FALSE,
            &ioStatus);

        status = ioStatus.Status;

        if (!NT_SUCCESS(status)) {
            TraceLog((CdromDebugWarning,
                        "CdRomDvdGetRegion => read structure failed %x\n",
                        status));
            ExFreePool(readStructure);
            break;
        }

        //
        // we got the copyright descriptor, so now get the region if possible
        //

        dvdHeader = (PDVD_DESCRIPTOR_HEADER) readStructure;
        copyRightDescriptor = (PDVD_COPYRIGHT_DESCRIPTOR) dvdHeader->Data;

        //
        // the original irp's systembuffer has a copy of the info that
        // should be passed down in the request
        //

        dvdRegion = Irp->AssociatedIrp.SystemBuffer;

        dvdRegion->CopySystem = copyRightDescriptor->CopyrightProtectionType;
        dvdRegion->RegionData = copyRightDescriptor->RegionManagementInformation;

        //
        // now reuse the buffer to request the copy protection info
        //

        copyProtectKey = (PDVD_COPY_PROTECT_KEY) readStructure;
        RtlZeroMemory (copyProtectKey, DVD_RPC_KEY_LENGTH);
        copyProtectKey->KeyLength = DVD_RPC_KEY_LENGTH;
        copyProtectKey->KeyType = DvdGetRpcKey;

        //
        // send a request for READ_KEY
        //

        ClassSendDeviceIoControlSynchronous(
            IOCTL_DVD_READ_KEY,
            DeviceObject,
            copyProtectKey,
            DVD_RPC_KEY_LENGTH,
            DVD_RPC_KEY_LENGTH,
            FALSE,
            &ioStatus);
        status = ioStatus.Status;

        if (!NT_SUCCESS(status)) {
            TraceLog((CdromDebugWarning,
                        "CdRomDvdGetRegion => read key failed %x\n",
                        status));
            ExFreePool(readStructure);
            break;
        }

        //
        // the request succeeded.  if a supported scheme is returned,
        // then return the information to the caller
        //

        rpcKey = (PDVD_RPC_KEY) copyProtectKey->KeyData;

        if (rpcKey->RpcScheme == 1) {

            if (rpcKey->TypeCode) {

                dvdRegion->SystemRegion = ~rpcKey->RegionMask;
                dvdRegion->ResetCount = rpcKey->UserResetsAvailable;

            } else {

                //
                // the drive has not been set for any region
                //

                dvdRegion->SystemRegion = 0;
                dvdRegion->ResetCount = rpcKey->UserResetsAvailable;
            }
            Irp->IoStatus.Information = sizeof(DVD_REGION);

        } else {

            TraceLog((CdromDebugWarning,
                        "CdRomDvdGetRegion => rpcKey->RpcScheme != 1\n"));
            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        ExFreePool(readStructure);
        break;
    }


    case IOCTL_STORAGE_SET_READ_AHEAD: {

        if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
           sizeof(STORAGE_SET_READ_AHEAD)) {

            TraceLog((CdromDebugWarning,
                        "DvdDeviceControl: SetReadAhead buffer too small\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;
    }

    case IOCTL_DISK_IS_WRITABLE: {

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);

        return STATUS_PENDING;

    }

    case IOCTL_DISK_GET_DRIVE_LAYOUT: {

        ULONG size;

        //
        // we always fake zero or one partitions, and one partition
        // structure is included in DRIVE_LAYOUT_INFORMATION
        //
        
        size = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry[1]);


        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Get drive layout\n"));
        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < size) {            
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = size;
            break;
        }
        
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;


    }
    case IOCTL_DISK_GET_DRIVE_LAYOUT_EX: {
        
        ULONG size;

        //
        // we always fake zero or one partitions, and one partition
        // structure is included in DRIVE_LAYOUT_INFORMATION_EX
        //

        size = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[1]);

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControl: Get drive layout ex\n"));
        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < size) {            
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = size;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;    
    
    }

    
    case IOCTL_DISK_GET_PARTITION_INFO: {

        //
        // Check that the buffer is large enough.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARTITION_INFORMATION)) {

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
            break;
        }
        
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;
    
    }
    case IOCTL_DISK_GET_PARTITION_INFO_EX: {

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARTITION_INFORMATION_EX)) {

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;
    }

    case IOCTL_DISK_VERIFY: {

        TraceLog((CdromDebugTrace,
                    "IOCTL_DISK_VERIFY to device %p through irp %p\n",
                    DeviceObject, Irp));

        //
        // Validate buffer length.
        //

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(VERIFY_INFORMATION)) {

            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;
    }

    case IOCTL_DISK_GET_LENGTH_INFO: {
        
        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(GET_LENGTH_INFORMATION)) {
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
            break;
        }
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;
    }

    case IOCTL_CDROM_GET_CONFIGURATION: {

        PGET_CONFIGURATION_IOCTL_INPUT inputBuffer;

        TraceLog((CdromDebugTrace,
                    "IOCTL_CDROM_GET_CONFIGURATION to via irp %p\n", Irp));

        //
        // Validate buffer length.
        //

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength !=
            sizeof(GET_CONFIGURATION_IOCTL_INPUT)) {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(GET_CONFIGURATION_HEADER)) {
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(GET_CONFIGURATION_HEADER);
            break;
        }
        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength > 0xffff) {
            // output buffer is too large
            status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        //
        // also verify the arguments are reasonable.
        //

        inputBuffer = Irp->AssociatedIrp.SystemBuffer;
        if (inputBuffer->Feature > 0xffff) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if ((inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE) &&
            (inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_CURRENT) &&
            (inputBuffer->RequestType != SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (inputBuffer->Reserved[0] || inputBuffer->Reserved[1]) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, NULL, NULL);
        return STATUS_PENDING;

    }

    default: {

        BOOLEAN synchronize = (KeGetCurrentIrql() == PASSIVE_LEVEL);
        PKEVENT deviceControlEvent;

        //
        // If the ioctl has come in at passive level then we will synchronize
        // with our start-io routine when sending the ioctl.  If the ioctl
        // has come in at a higher interrupt level and it was not handled
        // above then it's unlikely to be a request for the class DLL - however
        // we'll still use it's common code to forward the request through.
        //

        if (synchronize) {

            deviceControlEvent = ExAllocatePoolWithTag(NonPagedPool,
                                                       sizeof(KEVENT),
                                                       CDROM_TAG_DC_EVENT);

            if (deviceControlEvent == NULL) {

                //
                // must complete this irp unsuccessful here
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;

            } else {

                //PIO_STACK_LOCATION currentStack;

                KeInitializeEvent(deviceControlEvent, NotificationEvent, FALSE);

                //currentStack = IoGetCurrentIrpStackLocation(Irp);
                nextStack = IoGetNextIrpStackLocation(Irp);

                //
                // Copy the stack down a notch
                //

                IoCopyCurrentIrpStackLocationToNext(Irp);

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
                // will be forced through the synchronization.  Once this
                // location goes away we get the old value back
                //

                SET_FLAG(nextStack->Flags, SL_OVERRIDE_VERIFY_VOLUME);

                IoStartPacket(DeviceObject, Irp, NULL, NULL);

                //
                // Wait for CdRomClassIoctlCompletion to set the event. This
                // ensures serialization remains intact for these unhandled device
                // controls.
                //

                KeWaitForSingleObject(
                    deviceControlEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);

                ExFreePool(deviceControlEvent);

                TraceLog((CdromDebugTrace,
                            "CdRomDeviceControl: irp %p synchronized\n", Irp));

                status = Irp->IoStatus.Status;
            }

        } else {
            status = STATUS_SUCCESS;
        }

        //
        // If an error occured then propagate that back up - we are no longer
        // guaranteed synchronization and the upper layers will have to
        // retry.
        //
        // If no error occured, call down to the class driver directly
        // then start up the next request.
        //

        if (NT_SUCCESS(status)) {

            UCHAR uniqueAddress;

            //
            // The class device control routine will release the remove
            // lock for this Irp.  We need to make sure we have one
            // available so that it's safe to call IoStartNextPacket
            //

            if(synchronize) {

                ClassAcquireRemoveLock(DeviceObject, (PIRP)&uniqueAddress);

            }

            status = ClassDeviceControl(DeviceObject, Irp);

            if(synchronize) {
                KeRaiseIrql(DISPATCH_LEVEL, &irql);
                IoStartNextPacket(DeviceObject, FALSE);
                KeLowerIrql(irql);
                ClassReleaseRemoveLock(DeviceObject, (PIRP)&uniqueAddress);
            }
            return status;

        }

        //
        // an error occurred (either STATUS_INSUFFICIENT_RESOURCES from
        // attempting to synchronize or  StartIo() error'd this one
        // out), so we need to finish the irp, which is
        // done at the end of this routine.
        //
        break;

    } // end default case

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

        if (Irp->Tail.Overlay.Thread) {
            IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        }

    }

    //
    // Update IRP with completion status.
    //

    Irp->IoStatus.Status = status;

    //
    // Complete the request.
    //

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_DISK_INCREMENT);
    TraceLog((CdromDebugTrace,
                "CdRomDeviceControl: Status is %lx\n", status));
    return status;

} // end CdRomDeviceControl()

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
    The irp completion is short-circuited so that CdRomDeviceControlDispatch
    can reissue it once it wakes up.

Arguments:

    DeviceObject - the device object
    Irp - the request we are synchronizing
    Context - a PKEVENT that we need to signal

Return Value:

    NTSTATUS

--*/
{
    PKEVENT syncEvent = (PKEVENT) Context;

    TraceLog((CdromDebugTrace,
                "CdRomClassIoctlCompletion: setting event for irp %p\n", Irp));

    //
    // We released the lock when we completed this request.  Reacquire it.
    //

    ClassAcquireRemoveLock(DeviceObject, Irp);

    KeSetEvent(syncEvent, IO_DISK_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
CdRomDeviceControlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PCDROM_DATA         cdData = (PCDROM_DATA)(commonExtension->DriverData);
    BOOLEAN             use6Byte = TEST_FLAG(cdData->XAFlags, XA_USE_6_BYTE);

    PIO_STACK_LOCATION  irpStack        = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION  realIrpStack;
    PIO_STACK_LOCATION  realIrpNextStack;

    PSCSI_REQUEST_BLOCK srb     = Context;

    PIRP                realIrp = NULL;

    NTSTATUS            status;
    BOOLEAN             retry;
    ULONG retryCount;

    //
    // Extract the 'real' irp from the irpstack.
    //

    realIrp = (PIRP) irpStack->Parameters.Others.Argument2;
    realIrpStack = IoGetCurrentIrpStackLocation(realIrp);
    realIrpNextStack = IoGetNextIrpStackLocation(realIrp);

    //
    // check that we've really got the correct irp
    //

    ASSERT(realIrpNextStack->Parameters.Others.Argument3 == Irp);

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        ULONG retryInterval;

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControlCompletion: Irp %p, Srb %p Real Irp %p Status %lx\n",
                    Irp,
                    srb,
                    realIrp,
                    srb->SrbStatus));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            TraceLog((CdromDebugTrace,
                        "CdRomDeviceControlCompletion: Releasing Queue\n"));
            ClassReleaseQueue(DeviceObject);
        }


        retry = ClassInterpretSenseInfo(DeviceObject,
                                        srb,
                                        irpStack->MajorFunction,
                                        irpStack->Parameters.DeviceIoControl.IoControlCode,
                                        MAXIMUM_RETRIES - ((ULONG)(ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1),
                                        &status,
                                        &retryInterval);

        TraceLog((CdromDebugTrace,
                    "CdRomDeviceControlCompletion: IRP will %sbe retried\n",
                    (retry ? "" : "not ")));

        //
        // Some of the Device Controls need special cases on non-Success status's.
        //

        if (realIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
            if ((realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_GET_LAST_SESSION) ||
                (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_READ_TOC)         ||
                (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_READ_TOC_EX)      ||
                (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_GET_CONTROL)      ||
                (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_GET_VOLUME)) {

                if (status == STATUS_DATA_OVERRUN) {
                    status = STATUS_SUCCESS;
                    retry = FALSE;
                }
            }

            if (realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_READ_Q_CHANNEL) {
                PLAY_ACTIVE(fdoExtension) = FALSE;
            }
        }

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (realIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME &&
            status == STATUS_VERIFY_REQUIRED) {

            // note: status gets overwritten here
            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;

            if (((realIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) ||
                 (realIrpStack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)
                ) &&
                ((realIrpStack->Parameters.DeviceIoControl.IoControlCode ==
                  IOCTL_CDROM_CHECK_VERIFY) ||
                 (realIrpStack->Parameters.DeviceIoControl.IoControlCode ==
                  IOCTL_STORAGE_CHECK_VERIFY) ||
                 (realIrpStack->Parameters.DeviceIoControl.IoControlCode ==
                  IOCTL_STORAGE_CHECK_VERIFY2) ||
                 (realIrpStack->Parameters.DeviceIoControl.IoControlCode ==
                  IOCTL_DISK_CHECK_VERIFY)
                )
               ) {

                //
                // Update the geometry information, as the media could have
                // changed. The completion routine for this will complete
                // the real irp and start the next packet.
                //

                if (srb) {
                    if (srb->SenseInfoBuffer) {
                        ExFreePool(srb->SenseInfoBuffer);
                    }
                    if (srb->DataBuffer) {
                        ExFreePool(srb->DataBuffer);
                    }
                    ExFreePool(srb);
                    srb = NULL;
                }

                if (Irp->MdlAddress) {
                    IoFreeMdl(Irp->MdlAddress);
                    Irp->MdlAddress = NULL;
                }

                IoFreeIrp(Irp);
                Irp = NULL;

                status = CdRomUpdateCapacity(fdoExtension, realIrp, NULL);
                TraceLog((CdromDebugTrace,
                            "CdRomDeviceControlCompletion: [%p] "
                            "CdRomUpdateCapacity completed with status %lx\n",
                            realIrp, status));
                
                //
                // needed to update the capacity.
                // the irp's already handed off to CdRomUpdateCapacity().
                // we've already free'd the current irp.
                // nothing left to do in this code path.
                //
                
                return STATUS_MORE_PROCESSING_REQUIRED;

            } // end of ioctls to update capacity

        }

        //
        // get current retry count
        //
        retryCount = PtrToUlong(realIrpNextStack->Parameters.Others.Argument1);

        if (retry && retryCount) {

            //
            // update retry count
            //
            realIrpNextStack->Parameters.Others.Argument1 = UlongToPtr(retryCount-1);

            if (((ULONG)(ULONG_PTR)realIrpNextStack->Parameters.Others.Argument1)) {

                //
                // Retry request.
                //

                TraceLog((CdromDebugWarning,
                            "Retry request %p - Calling StartIo\n", Irp));


                ExFreePool(srb->SenseInfoBuffer);
                if (srb->DataBuffer) {
                    ExFreePool(srb->DataBuffer);
                }
                ExFreePool(srb);
                if (Irp->MdlAddress) {
                    IoFreeMdl(Irp->MdlAddress);
                }

                realIrpNextStack->Parameters.Others.Argument3 = (PVOID)-1;
                IoFreeIrp(Irp);

                CdRomRetryRequest(fdoExtension, realIrp, retryInterval, FALSE);
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            //
            // Exhausted retries. Fall through and complete the request with
            // the appropriate status.
            //

        }
    } else {

        //
        // Set status for successful request.
        //

        status = STATUS_SUCCESS;

    }


    if (NT_SUCCESS(status)) {
        
        //BOOLEAN b = FALSE;


        switch (realIrpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_CDROM_GET_CONFIGURATION: {
            RtlMoveMemory(realIrp->AssociatedIrp.SystemBuffer,
                          srb->DataBuffer,
                          srb->DataTransferLength);
            realIrp->IoStatus.Information = srb->DataTransferLength;
            break;
        }

        case IOCTL_DISK_GET_LENGTH_INFO: {
            
            PGET_LENGTH_INFORMATION lengthInfo;
            
            CdRomInterpretReadCapacity(DeviceObject,
                                       (PREAD_CAPACITY_DATA)srb->DataBuffer);

            lengthInfo = (PGET_LENGTH_INFORMATION)realIrp->AssociatedIrp.SystemBuffer;
            lengthInfo->Length = commonExtension->PartitionLength;
            realIrp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);
            status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
        case IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX: {
            
            PDISK_GEOMETRY_EX geometryEx;
            
            CdRomInterpretReadCapacity(DeviceObject,
                                       (PREAD_CAPACITY_DATA)srb->DataBuffer);

            geometryEx = (PDISK_GEOMETRY_EX)(realIrp->AssociatedIrp.SystemBuffer);
            geometryEx->DiskSize = commonExtension->PartitionLength;
            geometryEx->Geometry = fdoExtension->DiskGeometry;
            realIrp->IoStatus.Information =
                FIELD_OFFSET(DISK_GEOMETRY_EX, Data);
            break;
        }

        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        case IOCTL_CDROM_GET_DRIVE_GEOMETRY: {

            PDISK_GEOMETRY geometry;
            
            CdRomInterpretReadCapacity(DeviceObject,
                                       (PREAD_CAPACITY_DATA)srb->DataBuffer);

            geometry = (PDISK_GEOMETRY)(realIrp->AssociatedIrp.SystemBuffer);
            *geometry = fdoExtension->DiskGeometry;
            realIrp->IoStatus.Information = sizeof(DISK_GEOMETRY);
            break;
        }

        case IOCTL_DISK_VERIFY: {
            //
            // nothing to do but return the status...
            //
            break;
        }

        case IOCTL_DISK_CHECK_VERIFY:
        case IOCTL_STORAGE_CHECK_VERIFY:
        case IOCTL_CDROM_CHECK_VERIFY: {

            if((realIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_CDROM_CHECK_VERIFY) &&
               (realIrpStack->Parameters.DeviceIoControl.OutputBufferLength)) {

                *((PULONG)realIrp->AssociatedIrp.SystemBuffer) =
                    commonExtension->PartitionZeroExtension->MediaChangeCount;

                realIrp->IoStatus.Information = sizeof(ULONG);
            } else {
                realIrp->IoStatus.Information = 0;
            }

            TraceLog((CdromDebugTrace,
                        "CdRomDeviceControlCompletion: [%p] completing "
                        "CHECK_VERIFY buddy irp %p\n", realIrp, Irp));
            break;
        }

        case IOCTL_CDROM_READ_TOC_EX: {

            if (srb->DataTransferLength < MINIMUM_CDROM_READ_TOC_EX_SIZE) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            //
            // Copy the returned info into the user buffer.
            //

            RtlMoveMemory(realIrp->AssociatedIrp.SystemBuffer,
                          srb->DataBuffer,
                          srb->DataTransferLength);

            //
            // update information field.
            //

            realIrp->IoStatus.Information = srb->DataTransferLength;
            break;
        }


        case IOCTL_CDROM_GET_LAST_SESSION:
        case IOCTL_CDROM_READ_TOC: {

            //
            // Copy the returned info into the user buffer.
            //

            RtlMoveMemory(realIrp->AssociatedIrp.SystemBuffer,
                          srb->DataBuffer,
                          srb->DataTransferLength);

            //
            // update information field.
            //

            realIrp->IoStatus.Information = srb->DataTransferLength;
            break;
        }

        case IOCTL_DVD_READ_STRUCTURE: {

            DVD_STRUCTURE_FORMAT format = ((PDVD_READ_STRUCTURE) realIrp->AssociatedIrp.SystemBuffer)->Format;

            PDVD_DESCRIPTOR_HEADER header = realIrp->AssociatedIrp.SystemBuffer;

            //FOUR_BYTE fourByte;
            //PTWO_BYTE twoByte;
            //UCHAR tmp;

            TraceLog((CdromDebugTrace,
                        "DvdDeviceControlCompletion - IOCTL_DVD_READ_STRUCTURE: completing irp %p (buddy %p)\n",
                        Irp,
                        realIrp));

            TraceLog((CdromDebugTrace,
                        "DvdDCCompletion - READ_STRUCTURE: descriptor format of %d\n", format));

            RtlMoveMemory(header,
                          srb->DataBuffer,
                          srb->DataTransferLength);

            //
            // Cook the data.  There are a number of fields that really
            // should be byte-swapped for the caller.
            //

            TraceLog((CdromDebugInfo,
                      "DvdDCCompletion - READ_STRUCTURE:\n"
                      "\tHeader at %p\n"
                      "\tDvdDCCompletion - READ_STRUCTURE: data at %p\n"
                      "\tDataBuffer was at %p\n"
                      "\tDataTransferLength was %lx\n",
                      header,
                      header->Data,
                      srb->DataBuffer,
                      srb->DataTransferLength));

            //
            // First the fields in the header
            //

            TraceLog((CdromDebugInfo, "READ_STRUCTURE: header->Length %lx -> ",
                           header->Length));
            REVERSE_SHORT(&header->Length);
            TraceLog((CdromDebugInfo, "%lx\n", header->Length));

            //
            // Now the fields in the descriptor
            //

            if(format == DvdPhysicalDescriptor) {

                PDVD_LAYER_DESCRIPTOR layer = (PDVD_LAYER_DESCRIPTOR) &(header->Data[0]);

                TraceLog((CdromDebugInfo, "READ_STRUCTURE: StartingDataSector %lx -> ",
                               layer->StartingDataSector));
                REVERSE_LONG(&(layer->StartingDataSector));
                TraceLog((CdromDebugInfo, "%lx\n", layer->StartingDataSector));

                TraceLog((CdromDebugInfo, "READ_STRUCTURE: EndDataSector %lx -> ",
                               layer->EndDataSector));
                REVERSE_LONG(&(layer->EndDataSector));
                TraceLog((CdromDebugInfo, "%lx\n", layer->EndDataSector));

                TraceLog((CdromDebugInfo, "READ_STRUCTURE: EndLayerZeroSector %lx -> ",
                               layer->EndLayerZeroSector));
                REVERSE_LONG(&(layer->EndLayerZeroSector));
                TraceLog((CdromDebugInfo, "%lx\n", layer->EndLayerZeroSector));
            }

            TraceLog((CdromDebugTrace, "Status is %lx\n", Irp->IoStatus.Status));
            TraceLog((CdromDebugTrace, "DvdDeviceControlCompletion - "
                        "IOCTL_DVD_READ_STRUCTURE: data transfer length of %d\n",
                        srb->DataTransferLength));

            realIrp->IoStatus.Information = srb->DataTransferLength;
            break;
        }

        case IOCTL_DVD_READ_KEY: {

            PDVD_COPY_PROTECT_KEY copyProtectKey = realIrp->AssociatedIrp.SystemBuffer;

            PCDVD_KEY_HEADER keyHeader = srb->DataBuffer;
            ULONG dataLength;

            ULONG transferLength =
                srb->DataTransferLength -
                FIELD_OFFSET(CDVD_KEY_HEADER, Data);

            //
            // Adjust the data length to ignore the two reserved bytes in the
            // header.
            //

            dataLength = (keyHeader->DataLength[0] << 8) +
                         keyHeader->DataLength[1];
            dataLength -= 2;

            //
            // take the minimum of the transferred length and the
            // length as specified in the header.
            //

            if(dataLength < transferLength) {
                transferLength = dataLength;
            }

            TraceLog((CdromDebugTrace,
                        "DvdDeviceControlCompletion: [%p] - READ_KEY with "
                        "transfer length of (%d or %d) bytes\n",
                        Irp,
                        dataLength,
                        srb->DataTransferLength - 2));

            //
            // Copy the key data into the return buffer
            //
            if(copyProtectKey->KeyType == DvdTitleKey) {

                RtlMoveMemory(copyProtectKey->KeyData,
                              keyHeader->Data + 1,
                              transferLength - 1);
                copyProtectKey->KeyData[transferLength - 1] = 0;

                //
                // If this is a title key then we need to copy the CGMS flags
                // as well.
                //
                copyProtectKey->KeyFlags = *(keyHeader->Data);

            } else {

                RtlMoveMemory(copyProtectKey->KeyData,
                              keyHeader->Data,
                              transferLength);
            }

            copyProtectKey->KeyLength = sizeof(DVD_COPY_PROTECT_KEY);
            copyProtectKey->KeyLength += transferLength;

            realIrp->IoStatus.Information = copyProtectKey->KeyLength;
            break;
        }

        case IOCTL_DVD_START_SESSION: {

            PDVD_SESSION_ID sessionId = realIrp->AssociatedIrp.SystemBuffer;

            PCDVD_KEY_HEADER keyHeader = srb->DataBuffer;
            PCDVD_REPORT_AGID_DATA keyData = (PCDVD_REPORT_AGID_DATA) keyHeader->Data;

            *sessionId = keyData->AGID;

            realIrp->IoStatus.Information = sizeof(DVD_SESSION_ID);

            break;
        }

        case IOCTL_DVD_END_SESSION:
        case IOCTL_DVD_SEND_KEY:
        case IOCTL_DVD_SEND_KEY2:

            //
            // nothing to return
            //
            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_PLAY_AUDIO_MSF:

            PLAY_ACTIVE(fdoExtension) = TRUE;

            break;

        case IOCTL_CDROM_READ_Q_CHANNEL: {

            PSUB_Q_CHANNEL_DATA userChannelData = realIrp->AssociatedIrp.SystemBuffer;
            PCDROM_SUB_Q_DATA_FORMAT inputBuffer = realIrp->AssociatedIrp.SystemBuffer;
            PSUB_Q_CHANNEL_DATA subQPtr = srb->DataBuffer;

#if DBG
            switch( inputBuffer->Format ) {

            case IOCTL_CDROM_CURRENT_POSITION:
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Audio Status is %u\n", subQPtr->CurrentPosition.Header.AudioStatus ));
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: ADR = 0x%x\n", subQPtr->CurrentPosition.ADR ));
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Control = 0x%x\n", subQPtr->CurrentPosition.Control ));
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Track = %u\n", subQPtr->CurrentPosition.TrackNumber ));
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Index = %u\n", subQPtr->CurrentPosition.IndexNumber ));
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Absolute Address = %x\n", *((PULONG)subQPtr->CurrentPosition.AbsoluteAddress) ));
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Relative Address = %x\n", *((PULONG)subQPtr->CurrentPosition.TrackRelativeAddress) ));
                break;

            case IOCTL_CDROM_MEDIA_CATALOG:
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Audio Status is %u\n", subQPtr->MediaCatalog.Header.AudioStatus ));
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Mcval is %u\n", subQPtr->MediaCatalog.Mcval ));
                break;

            case IOCTL_CDROM_TRACK_ISRC:
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Audio Status is %u\n", subQPtr->TrackIsrc.Header.AudioStatus ));
                TraceLog((CdromDebugTrace,"CdRomDeviceControlCompletion: Tcval is %u\n", subQPtr->TrackIsrc.Tcval ));
                break;

            }
#endif

            //
            // Update the play active status.
            //

            if (subQPtr->CurrentPosition.Header.AudioStatus == AUDIO_STATUS_IN_PROGRESS) {

                PLAY_ACTIVE(fdoExtension) = TRUE;

            } else {

                PLAY_ACTIVE(fdoExtension) = FALSE;

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

            PLAY_ACTIVE(fdoExtension) = FALSE;
            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_RESUME_AUDIO:

            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_SEEK_AUDIO_MSF:

            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_STOP_AUDIO:

            PLAY_ACTIVE(fdoExtension) = FALSE;
            realIrp->IoStatus.Information = 0;
            break;

        case IOCTL_CDROM_GET_CONTROL: {

            PCDROM_AUDIO_CONTROL audioControl = srb->DataBuffer;
            PAUDIO_OUTPUT        audioOutput;
            ULONG                bytesTransferred;

            audioOutput = ClassFindModePage((PCHAR)audioControl,
                                            srb->DataTransferLength,
                                            CDROM_AUDIO_CONTROL_PAGE,
                                            use6Byte);
            //
            // Verify the page is as big as expected.
            //

            bytesTransferred = (ULONG)((PCHAR) audioOutput - (PCHAR) audioControl) +
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
            ULONG i;
            ULONG bytesTransferred;

            audioOutput = ClassFindModePage((PCHAR)volumeControl,
                                                 srb->DataTransferLength,
                                                 CDROM_AUDIO_CONTROL_PAGE,
                                                 use6Byte);

            //
            // Verify the page is as big as expected.
            //

            bytesTransferred = (ULONG)((PCHAR) audioOutput - (PCHAR) volumeControl) +
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

            realIrp->IoStatus.Information = 0;
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

        TraceLog((CdromDebugWarning,
                    "CdRomDeviceCompletion - Setting Hard Error on realIrp %p\n",
                    realIrp));
        if (realIrp->Tail.Overlay.Thread) {
            IoSetHardErrorOrVerifyDevice(realIrp, DeviceObject);
        }

        realIrp->IoStatus.Information = 0;
    }
    
    //
    // note: must complete the realIrp, as the completed irp (above)
    //       was self-allocated.
    //

    CdRomCompleteIrpAndStartNextPacketSafely(DeviceObject, realIrp);
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
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    PCDROM_DATA         cdData = (PCDROM_DATA)(commonExtension->DriverData);
    BOOLEAN             use6Byte = TEST_FLAG(cdData->XAFlags, XA_USE_6_BYTE);
    PIO_STACK_LOCATION  realIrpStack;
    PIO_STACK_LOCATION  realIrpNextStack;
    PSCSI_REQUEST_BLOCK srb     = Context;
    PIRP                realIrp = NULL;
    NTSTATUS            status;
    BOOLEAN             retry;
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

        //
        // get current retry count
        //
        retryCount = PtrToUlong(realIrpNextStack->Parameters.Others.Argument1);

        if (retry && retryCount) {

            //
            // update retry count
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

                CdRomRetryRequest(deviceExtension,
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

        PAUDIO_OUTPUT   audioInput = NULL;
        PAUDIO_OUTPUT   audioOutput;
        PVOLUME_CONTROL volumeControl = realIrp->AssociatedIrp.SystemBuffer;
        ULONG           i,bytesTransferred,headerLength;
        PVOID           dataBuffer;
        PCDB            cdb;

        audioInput = ClassFindModePage((PCHAR)srb->DataBuffer,
                                             srb->DataTransferLength,
                                             CDROM_AUDIO_CONTROL_PAGE,
                                             use6Byte);

        //
        // Check to make sure the mode sense data is valid before we go on
        //

        if(audioInput == NULL) {

            TraceLog((CdromDebugWarning,
                        "Mode Sense Page %d not found\n",
                        CDROM_AUDIO_CONTROL_PAGE));

            realIrp->IoStatus.Information = 0;
            realIrp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            goto SafeExit;
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

        dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                    bytesTransferred,
                                    CDROM_TAG_VOLUME_INT);

        if (!dataBuffer) {
            realIrp->IoStatus.Information = 0;
            realIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            goto SafeExit;
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

        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        ExFreePool(srb->DataBuffer);

        //
        // set the data buffer to new allocation, so it can be
        // freed in the exit path
        //
        
        srb->DataBuffer = dataBuffer;

        //
        // rebuild the srb.
        //

        cdb = (PCDB)srb->Cdb;
        RtlZeroMemory(cdb, CDB12GENERIC_LENGTH);

        srb->SrbStatus = srb->ScsiStatus = 0;
        srb->SrbFlags = deviceExtension->SrbFlags;
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DATA_OUT);
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
            goto SafeExit;
        }

        MmBuildMdlForNonPagedPool(Irp->MdlAddress);

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

        IoCallDriver(commonExtension->LowerDeviceObject, Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

SafeExit:

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

NTSTATUS
NTAPI
CdRomDvdEndAllSessionsCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine will setup the next stack location to issue an end session
    to the device.  It will increment the session id in the system buffer
    and issue an END_SESSION for that AGID if the AGID is valid.

    When the new AGID is > 3 this routine will complete the request.

Arguments:

    DeviceObject - the device object for this drive

    Irp - the request

    Context - done

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED if there is another AGID to clear
    status otherwise.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    //PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);

    PDVD_SESSION_ID sessionId = Irp->AssociatedIrp.SystemBuffer;

    //NTSTATUS status;

    if(++(*sessionId) > MAX_COPY_PROTECT_AGID) {

        //
        // We're done here - just return success and let the io system
        // continue to complete it.
        //

        return STATUS_SUCCESS;

    }

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           CdRomDvdEndAllSessionsCompletion,
                           NULL,
                           TRUE,
                           FALSE,
                           FALSE);

    IoMarkIrpPending(Irp);

    IoCallDriver(fdoExtension->CommonExtension.DeviceObject, Irp);

    //
    // At this point we have to assume the irp may have already been
    // completed.  Ignore the returned status and return.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
CdRomDvdReadDiskKeyCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles the completion of a request to obtain the disk
    key from the dvd media.  It will transform the raw 2K of key data into
    a DVD_COPY_PROTECT_KEY structure and copy back the saved key parameters
    from the context pointer before returning.

Arguments:

    DeviceObject -

    Irp -

    Context - a DVD_COPY_PROTECT_KEY pointer which contains the key
              parameters handed down by the caller.

Return Value:

    STATUS_SUCCESS;

--*/

{
    PDVD_COPY_PROTECT_KEY savedKey = Context;

    PREAD_DVD_STRUCTURES_HEADER rawKey = Irp->AssociatedIrp.SystemBuffer;
    PDVD_COPY_PROTECT_KEY outputKey = Irp->AssociatedIrp.SystemBuffer;

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        //
        // Shift the data down to its new position.
        //

        RtlMoveMemory(outputKey->KeyData,
                      rawKey->Data,
                      sizeof(DVD_DISK_KEY_DESCRIPTOR));

        RtlCopyMemory(outputKey,
                      savedKey,
                      sizeof(DVD_COPY_PROTECT_KEY));

        outputKey->KeyLength = DVD_DISK_KEY_LENGTH;

        Irp->IoStatus.Information = DVD_DISK_KEY_LENGTH;

    } else {

        TraceLog((CdromDebugWarning,
                    "DiskKey Failed with status %x, %p (%x) bytes\n",
                    Irp->IoStatus.Status,
                    (PVOID)Irp->IoStatus.Information,
                    ((rawKey->Length[0] << 16) | rawKey->Length[1])
                    ));

    }

    //
    // release the context block
    //

    ExFreePool(Context);

    return STATUS_SUCCESS;
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
    PSCSI_REQUEST_BLOCK srb = Context;
    PFUNCTIONAL_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    BOOLEAN retry;

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        ULONG retryInterval;

        TraceLog((CdromDebugTrace, "CdromXAComplete: IRP %p  SRB %p  Status %x\n",
                    Irp, srb, srb->SrbStatus));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ClassReleaseQueue(DeviceObject);
        }

        retry = ClassInterpretSenseInfo(
            DeviceObject,
            srb,
            irpStack->MajorFunction,
            irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL ? irpStack->Parameters.DeviceIoControl.IoControlCode : 0,
            MAXIMUM_RETRIES - irpStack->MinorFunction, // HACKHACK - REF #0001
            &status,
            &retryInterval);

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (irpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME &&
            status == STATUS_VERIFY_REQUIRED) {

            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        if (retry) {

            if (irpStack->MinorFunction != 0) { // HACKHACK - REF #0001

                irpStack->MinorFunction--;      // HACKHACK - REF #0001

                //
                // Retry request.
                //

                TraceLog((CdromDebugWarning,
                            "CdRomXACompletion: Retry request %p (%x) - "
                            "Calling StartIo\n", Irp, irpStack->MinorFunction));


                ExFreePool(srb->SenseInfoBuffer);
                ExFreePool(srb);

                //
                // Call StartIo directly since IoStartNextPacket hasn't been called,
                // the serialisation is still intact.
                //

                CdRomRetryRequest(deviceExtension,
                                  Irp,
                                  retryInterval,
                                  FALSE);

                return STATUS_MORE_PROCESSING_REQUIRED;

            }

            //
            // Exhausted retries, fall through and complete the request
            // with the appropriate status
            //

            TraceLog((CdromDebugWarning,
                        "CdRomXACompletion: Retries exhausted for irp %p\n",
                        Irp));

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

    ExFreePool(srb->SenseInfoBuffer);
    ExFreePool(srb);

    //
    // Set status in completing IRP.
    //

    Irp->IoStatus.Status = status;    

    //
    // Set the hard error if necessary.
    //

    if (!NT_SUCCESS(status) &&
        IoIsErrorUserInduced(status) &&
        Irp->Tail.Overlay.Thread != NULL ) {

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
    
    {
        KIRQL oldIrql = KeRaiseIrqlToDpcLevel();
        IoStartNextPacket(DeviceObject, FALSE);
        KeLowerIrql(oldIrql);
    }
    ClassReleaseRemoveLock(DeviceObject, Irp);
    
    return status;
}

VOID
NTAPI
CdRomDeviceControlDvdReadStructure(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP OriginalIrp,
    IN PIRP NewIrp,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(OriginalIrp);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCDB cdb = (PCDB)Srb->Cdb;
    PVOID dataBuffer;

    PDVD_READ_STRUCTURE request;
    USHORT dataLength;
    ULONG blockNumber;
    PFOUR_BYTE fourByte;

    dataLength =
        (USHORT)currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    request = OriginalIrp->AssociatedIrp.SystemBuffer;
    blockNumber =
        (ULONG)(request->BlockByteOffset.QuadPart >> fdoExtension->SectorShift);
    fourByte = (PFOUR_BYTE) &blockNumber;

    Srb->CdbLength = 12;
    Srb->TimeOutValue = fdoExtension->TimeOutValue;
    Srb->SrbFlags = fdoExtension->SrbFlags;
    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_IN);

    cdb->READ_DVD_STRUCTURE.OperationCode = SCSIOP_READ_DVD_STRUCTURE;
    cdb->READ_DVD_STRUCTURE.RMDBlockNumber[0] = fourByte->Byte3;
    cdb->READ_DVD_STRUCTURE.RMDBlockNumber[1] = fourByte->Byte2;
    cdb->READ_DVD_STRUCTURE.RMDBlockNumber[2] = fourByte->Byte1;
    cdb->READ_DVD_STRUCTURE.RMDBlockNumber[3] = fourByte->Byte0;
    cdb->READ_DVD_STRUCTURE.LayerNumber   = request->LayerNumber;
    cdb->READ_DVD_STRUCTURE.Format        = (UCHAR)request->Format;

#if DBG
    {
        if ((UCHAR)request->Format > DvdMaxDescriptor) {
            TraceLog((CdromDebugWarning,
                        "READ_DVD_STRUCTURE format %x = %s (%x bytes)\n",
                        (UCHAR)request->Format,
                        READ_DVD_STRUCTURE_FORMAT_STRINGS[DvdMaxDescriptor],
                        dataLength
                        ));
        } else {
            TraceLog((CdromDebugWarning,
                        "READ_DVD_STRUCTURE format %x = %s (%x bytes)\n",
                        (UCHAR)request->Format,
                        READ_DVD_STRUCTURE_FORMAT_STRINGS[(UCHAR)request->Format],
                        dataLength
                        ));
        }
    }
#endif // DBG

    if (request->Format == DvdDiskKeyDescriptor) {

        cdb->READ_DVD_STRUCTURE.AGID = (UCHAR) request->SessionId;

    }

    cdb->READ_DVD_STRUCTURE.AllocationLength[0] = (UCHAR)(dataLength >> 8);
    cdb->READ_DVD_STRUCTURE.AllocationLength[1] = (UCHAR)(dataLength & 0xff);
    Srb->DataTransferLength = dataLength;



    dataBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                       dataLength,
                                       DVD_TAG_READ_STRUCTURE);

    if (!dataBuffer) {
        ExFreePool(Srb->SenseInfoBuffer);
        ExFreePool(Srb);
        IoFreeIrp(NewIrp);
        OriginalIrp->IoStatus.Information = 0;
        OriginalIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

        BAIL_OUT(OriginalIrp);
        CdRomCompleteIrpAndStartNextPacketSafely(Fdo, OriginalIrp);
        return;
    }
    RtlZeroMemory(dataBuffer, dataLength);

    NewIrp->MdlAddress = IoAllocateMdl(dataBuffer,
                                       currentIrpStack->Parameters.Read.Length,
                                       FALSE,
                                       FALSE,
                                       (PIRP) NULL);

    if (NewIrp->MdlAddress == NULL) {
        ExFreePool(dataBuffer);
        ExFreePool(Srb->SenseInfoBuffer);
        ExFreePool(Srb);
        IoFreeIrp(NewIrp);
        OriginalIrp->IoStatus.Information = 0;
        OriginalIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

        BAIL_OUT(OriginalIrp);
        CdRomCompleteIrpAndStartNextPacketSafely(Fdo, OriginalIrp);
        return;
    }

    //
    // Prepare the MDL
    //

    MmBuildMdlForNonPagedPool(NewIrp->MdlAddress);

    Srb->DataBuffer = dataBuffer;

    IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, NewIrp);

    return;
}

VOID
NTAPI
CdRomDeviceControlDvdEndSession(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP OriginalIrp,
    IN PIRP NewIrp,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    //PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(OriginalIrp);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCDB cdb = (PCDB)Srb->Cdb;

    PDVD_SESSION_ID sessionId = OriginalIrp->AssociatedIrp.SystemBuffer;

    Srb->CdbLength = 12;
    Srb->TimeOutValue = fdoExtension->TimeOutValue;
    Srb->SrbFlags = fdoExtension->SrbFlags;
    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_NO_DATA_TRANSFER);

    cdb->SEND_KEY.OperationCode = SCSIOP_SEND_KEY;
    cdb->SEND_KEY.AGID = (UCHAR) (*sessionId);
    cdb->SEND_KEY.KeyFormat = DVD_INVALIDATE_AGID;

    IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, NewIrp);
    return;

}

VOID
NTAPI
CdRomDeviceControlDvdStartSessionReadKey(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP OriginalIrp,
    IN PIRP NewIrp,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(OriginalIrp);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCDB cdb = (PCDB)Srb->Cdb;
    NTSTATUS status;

    PDVD_COPY_PROTECT_KEY keyParameters;
    PCDVD_KEY_HEADER keyBuffer = NULL;

    ULONG keyLength;

    ULONG allocationLength;
    PFOUR_BYTE fourByte;

    //
    // Both of these use REPORT_KEY commands.
    // Determine the size of the input buffer
    //

    if(currentIrpStack->Parameters.DeviceIoControl.IoControlCode ==
       IOCTL_DVD_READ_KEY) {

        keyParameters = OriginalIrp->AssociatedIrp.SystemBuffer;

        keyLength = sizeof(CDVD_KEY_HEADER) +
                    (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength -
                     sizeof(DVD_COPY_PROTECT_KEY));
    } else {

        keyParameters = NULL;
        keyLength = sizeof(CDVD_KEY_HEADER) +
                    sizeof(CDVD_REPORT_AGID_DATA);
    }

    TRY {

        keyBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                          keyLength,
                                          DVD_TAG_READ_KEY);

        if(keyBuffer == NULL) {

            TraceLog((CdromDebugWarning,
                        "IOCTL_DVD_READ_KEY - couldn't allocate "
                        "%d byte buffer for key\n",
                        keyLength));
            status = STATUS_INSUFFICIENT_RESOURCES;
            LEAVE;
        }


        NewIrp->MdlAddress = IoAllocateMdl(keyBuffer,
                                           keyLength,
                                           FALSE,
                                           FALSE,
                                           (PIRP) NULL);

        if(NewIrp->MdlAddress == NULL) {

            TraceLog((CdromDebugWarning,
                        "IOCTL_DVD_READ_KEY - couldn't create mdl\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            LEAVE;
        }

        MmBuildMdlForNonPagedPool(NewIrp->MdlAddress);

        Srb->DataBuffer = keyBuffer;
        Srb->CdbLength = 12;

        cdb->REPORT_KEY.OperationCode = SCSIOP_REPORT_KEY;

        allocationLength = keyLength;
        fourByte = (PFOUR_BYTE) &allocationLength;
        cdb->REPORT_KEY.AllocationLength[0] = fourByte->Byte1;
        cdb->REPORT_KEY.AllocationLength[1] = fourByte->Byte0;

        Srb->DataTransferLength = keyLength;

        //
        // set the specific parameters....
        //

        if(currentIrpStack->Parameters.DeviceIoControl.IoControlCode ==
           IOCTL_DVD_READ_KEY) {

            if(keyParameters->KeyType == DvdTitleKey) {

                ULONG logicalBlockAddress;

                logicalBlockAddress = (ULONG)
                    (keyParameters->Parameters.TitleOffset.QuadPart >>
                     fdoExtension->SectorShift);

                fourByte = (PFOUR_BYTE) &(logicalBlockAddress);

                cdb->REPORT_KEY.LogicalBlockAddress[0] = fourByte->Byte3;
                cdb->REPORT_KEY.LogicalBlockAddress[1] = fourByte->Byte2;
                cdb->REPORT_KEY.LogicalBlockAddress[2] = fourByte->Byte1;
                cdb->REPORT_KEY.LogicalBlockAddress[3] = fourByte->Byte0;
            }

            cdb->REPORT_KEY.KeyFormat = (UCHAR)keyParameters->KeyType;
            cdb->REPORT_KEY.AGID = (UCHAR) keyParameters->SessionId;
            TraceLog((CdromDebugWarning,
                        "CdRomDvdReadKey => sending irp %p for irp %p (%s)\n",
                        NewIrp, OriginalIrp, "READ_KEY"));

        } else {

            cdb->REPORT_KEY.KeyFormat = DVD_REPORT_AGID;
            cdb->REPORT_KEY.AGID = 0;
            TraceLog((CdromDebugWarning,
                        "CdRomDvdReadKey => sending irp %p for irp %p (%s)\n",
                        NewIrp, OriginalIrp, "START_SESSION"));
        }

        Srb->TimeOutValue = fdoExtension->TimeOutValue;
        Srb->SrbFlags = fdoExtension->SrbFlags;
        SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_IN);

        IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, NewIrp);

        status = STATUS_SUCCESS;

    } FINALLY {

        if (!NT_SUCCESS(status)) {

            //
            // An error occured during setup - free resources and
            // complete this request.
            //
            if (NewIrp->MdlAddress != NULL) {
                IoFreeMdl(NewIrp->MdlAddress);
            }

            if (keyBuffer != NULL) {
                ExFreePool(keyBuffer);
            }
            ExFreePool(Srb->SenseInfoBuffer);
            ExFreePool(Srb);
            IoFreeIrp(NewIrp);

            OriginalIrp->IoStatus.Information = 0;
            OriginalIrp->IoStatus.Status = status;

            BAIL_OUT(OriginalIrp);
            CdRomCompleteIrpAndStartNextPacketSafely(Fdo, OriginalIrp);

        } // end !NT_SUCCESS
    }
    return;
}

VOID
NTAPI
CdRomDeviceControlDvdSendKey(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP OriginalIrp,
    IN PIRP NewIrp,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    //PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(OriginalIrp);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCDB cdb = (PCDB)Srb->Cdb;

    PDVD_COPY_PROTECT_KEY key;
    PCDVD_KEY_HEADER keyBuffer = NULL;

    NTSTATUS status;
    ULONG keyLength;
    PFOUR_BYTE fourByte;

    key = OriginalIrp->AssociatedIrp.SystemBuffer;
    keyLength = (key->KeyLength - sizeof(DVD_COPY_PROTECT_KEY)) +
                sizeof(CDVD_KEY_HEADER);

    TRY {

        keyBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                          keyLength,
                                          DVD_TAG_SEND_KEY);

        if(keyBuffer == NULL) {

            TraceLog((CdromDebugWarning,
                        "IOCTL_DVD_SEND_KEY - couldn't allocate "
                        "%d byte buffer for key\n",
                        keyLength));
            status = STATUS_INSUFFICIENT_RESOURCES;
            LEAVE;
        }

        RtlZeroMemory(keyBuffer, keyLength);

        //
        // keylength is decremented here by two because the
        // datalength does not include the header, which is two
        // bytes.  keylength is immediately incremented later
        // by the same amount.
        //

        keyLength -= 2;
        fourByte = (PFOUR_BYTE) &keyLength;
        keyBuffer->DataLength[0] = fourByte->Byte1;
        keyBuffer->DataLength[1] = fourByte->Byte0;
        keyLength += 2;

        //
        // copy the user's buffer to our own allocated buffer
        //

        RtlMoveMemory(keyBuffer->Data,
                      key->KeyData,
                      key->KeyLength - sizeof(DVD_COPY_PROTECT_KEY));


        NewIrp->MdlAddress = IoAllocateMdl(keyBuffer,
                                           keyLength,
                                           FALSE,
                                           FALSE,
                                           (PIRP) NULL);

        if(NewIrp->MdlAddress == NULL) {
            TraceLog((CdromDebugWarning,
                        "IOCTL_DVD_SEND_KEY - couldn't create mdl\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            LEAVE;
        }


        MmBuildMdlForNonPagedPool(NewIrp->MdlAddress);

        Srb->CdbLength = 12;
        Srb->DataBuffer = keyBuffer;
        Srb->DataTransferLength = keyLength;

        Srb->TimeOutValue = fdoExtension->TimeOutValue;
        Srb->SrbFlags = fdoExtension->SrbFlags;
        SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_OUT);

        cdb->REPORT_KEY.OperationCode = SCSIOP_SEND_KEY;

        fourByte = (PFOUR_BYTE) &keyLength;

        cdb->SEND_KEY.ParameterListLength[0] = fourByte->Byte1;
        cdb->SEND_KEY.ParameterListLength[1] = fourByte->Byte0;
        cdb->SEND_KEY.KeyFormat = (UCHAR)key->KeyType;
        cdb->SEND_KEY.AGID = (UCHAR) key->SessionId;

        if (key->KeyType == DvdSetRpcKey) {
            TraceLog((CdromDebugWarning,
                        "IOCTL_DVD_SEND_KEY - Setting RPC2 drive region\n"));
        } else {
            TraceLog((CdromDebugWarning,
                        "IOCTL_DVD_SEND_KEY - key type %x\n", key->KeyType));
        }

        IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, NewIrp);

        status = STATUS_SUCCESS;

    } FINALLY {

        if (!NT_SUCCESS(status)) {

            //
            // An error occured during setup - free resources and
            // complete this request.
            //

            if (NewIrp->MdlAddress != NULL) {
                IoFreeMdl(NewIrp->MdlAddress);
            }

            if (keyBuffer != NULL) {
                ExFreePool(keyBuffer);
            }

            ExFreePool(Srb->SenseInfoBuffer);
            ExFreePool(Srb);
            IoFreeIrp(NewIrp);

            OriginalIrp->IoStatus.Information = 0;
            OriginalIrp->IoStatus.Status = status;

            BAIL_OUT(OriginalIrp);
            CdRomCompleteIrpAndStartNextPacketSafely(Fdo, OriginalIrp);

        }
    }

    return;
}

VOID
NTAPI
CdRomInterpretReadCapacity(
    IN PDEVICE_OBJECT Fdo,
    IN PREAD_CAPACITY_DATA ReadCapacityBuffer
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    ULONG               lastSector;
    ULONG               bps;
    ULONG               lastBit;
    ULONG               tmp;

    ASSERT(ReadCapacityBuffer);
    ASSERT(commonExtension->IsFdo);
    
    TraceLog((CdromDebugError,
                "CdRomInterpretReadCapacity: Entering\n"));

    //
    // Swizzle bytes from Read Capacity and translate into
    // the necessary geometry information in the device extension.
    //

    tmp = ReadCapacityBuffer->BytesPerBlock;
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

    fdoExtension->DiskGeometry.BytesPerSector = bps;

    TraceLog((CdromDebugTrace, "CdRomInterpretReadCapacity: Calculated bps %#x\n",
                fdoExtension->DiskGeometry.BytesPerSector));

    //
    // Copy last sector in reverse byte order.
    //

    tmp = ReadCapacityBuffer->LogicalBlockAddress;
    ((PFOUR_BYTE)&lastSector)->Byte0 = ((PFOUR_BYTE)&tmp)->Byte3;
    ((PFOUR_BYTE)&lastSector)->Byte1 = ((PFOUR_BYTE)&tmp)->Byte2;
    ((PFOUR_BYTE)&lastSector)->Byte2 = ((PFOUR_BYTE)&tmp)->Byte1;
    ((PFOUR_BYTE)&lastSector)->Byte3 = ((PFOUR_BYTE)&tmp)->Byte0;

    //
    // Calculate sector to byte shift.
    //

    WHICH_BIT(bps, fdoExtension->SectorShift);

    TraceLog((CdromDebugTrace,"CdRomInterpretReadCapacity: Sector size is %d\n",
        fdoExtension->DiskGeometry.BytesPerSector));

    TraceLog((CdromDebugTrace,"CdRomInterpretReadCapacity: Number of Sectors is %d\n",
        lastSector + 1));

    //
    // Calculate media capacity in bytes.
    //

    commonExtension->PartitionLength.QuadPart = (LONGLONG)(lastSector + 1);

    //
    // we've defaulted to 32/64 forever.  don't want to change this now...
    //

    fdoExtension->DiskGeometry.TracksPerCylinder = 0x40;
    fdoExtension->DiskGeometry.SectorsPerTrack = 0x20;

    //
    // Calculate number of cylinders.
    //

    fdoExtension->DiskGeometry.Cylinders.QuadPart = (LONGLONG)((lastSector + 1) / (32 * 64));

    commonExtension->PartitionLength.QuadPart =
        (commonExtension->PartitionLength.QuadPart << fdoExtension->SectorShift);


    ASSERT(TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA));
    
    //
    // This device supports removable media.
    //

    fdoExtension->DiskGeometry.MediaType = RemovableMedia;

    return;
}
