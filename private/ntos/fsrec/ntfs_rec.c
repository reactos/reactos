/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ntfs_rec.c

Abstract:

    This module contains the mini-file system recognizer for NTFS.

Author:

    Darryl E. Havens (darrylh) 8-dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "fs_rec.h"
#include "ntfs_rec.h"

//
//  The local debug trace level
//

#define Dbg                              (FSREC_DEBUG_LEVEL_NTFS)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtfsRecFsControl)
#pragma alloc_text(PAGE,IsNtfsVolume)
#endif // ALLOC_PRAGMA


NTSTATUS
NtfsRecFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function performs the mount and driver reload functions for this mini-
    file system recognizer driver.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet (IRP) representing the function to
        be performed.

Return Value:

    The function value is the final status of the operation.


--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT targetDevice;
    PPACKED_BOOT_SECTOR buffer;
    LARGE_INTEGER byteOffset;
    LARGE_INTEGER secondByteOffset;
    LARGE_INTEGER lastByteOffset;
    UNICODE_STRING driverName;
    ULONG bytesPerSector;
    LARGE_INTEGER numberOfSectors;

    PAGED_CODE();

    //
    // Begin by determining what function that is to be performed.
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( irpSp->MinorFunction ) {

    case IRP_MN_MOUNT_VOLUME:

        //
        // Attempt to mount a volume:  Determine whether or not the volume in
        // question is an NTFS volume and, if so, let the I/O system know that it
        // is by returning a special status code so that this driver can get
        // called back to load the NTFS file system.
        //

        status = STATUS_UNRECOGNIZED_VOLUME;
        
        //
        // Attempt to determine whether or not the target volume being mounted
        // is an NTFS volume.
        //

        targetDevice = irpSp->Parameters.MountVolume.DeviceObject;

        if (FsRecGetDeviceSectorSize( targetDevice,
                                      &bytesPerSector ) &&
            FsRecGetDeviceSectors( targetDevice,
                                   bytesPerSector,
                                   &numberOfSectors )) {

            byteOffset.QuadPart = 0;
            buffer = NULL;
            secondByteOffset.QuadPart = numberOfSectors.QuadPart >> 1;
            secondByteOffset.QuadPart *= (LONG) bytesPerSector;
            lastByteOffset.QuadPart = (numberOfSectors.QuadPart - 1) * (LONG) bytesPerSector;

            if (FsRecReadBlock( targetDevice,
                                &byteOffset,
                                sizeof( PACKED_BOOT_SECTOR ),
                                bytesPerSector,
                                (PVOID *)&buffer,
                                NULL ) &&
                IsNtfsVolume( buffer, bytesPerSector, &numberOfSectors )) {
                    
                status = STATUS_FS_DRIVER_REQUIRED;
            
            } else {

                if (FsRecReadBlock( targetDevice,
                                    &secondByteOffset,
                                    sizeof( PACKED_BOOT_SECTOR ),
                                    bytesPerSector,
                                    (PVOID *)&buffer,
                                    NULL ) &&
                    IsNtfsVolume( buffer, bytesPerSector, &numberOfSectors )) {

                    status = STATUS_FS_DRIVER_REQUIRED;

                } else {
                    
                    if (FsRecReadBlock( targetDevice,
                                        &lastByteOffset,
                                        sizeof( PACKED_BOOT_SECTOR ),
                                        bytesPerSector,
                                        (PVOID *)&buffer,
                                        NULL ) &&
                        IsNtfsVolume( buffer, bytesPerSector, &numberOfSectors )) {
                        
                        status = STATUS_FS_DRIVER_REQUIRED;
                    }
                }
            }
            
            if (buffer != NULL) {
                ExFreePool( buffer );
            }
        }

        break;

    case IRP_MN_LOAD_FILE_SYSTEM:

        status = FsRecLoadFileSystem( DeviceObject,
                                      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Ntfs" );
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // Finally, complete the request and return the same status code to the
    // caller.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}


BOOLEAN
IsNtfsVolume(
    IN PPACKED_BOOT_SECTOR BootSector,
    IN ULONG BytesPerSector,
    IN PLARGE_INTEGER NumberOfSectors
    )

/*++

Routine Description:

    This routine looks at the buffer passed in which contains the NTFS boot
    sector and determines whether or not it represents an NTFS volume.

Arguments:

    BootSector - Pointer to buffer containing a potential NTFS boot sector.

    BytesPerSector - Supplies the number of bytes per sector for the drive.

    NumberOfSectors - Supplies the number of sectors on the partition.

Return Value:

    The function returns TRUE if the buffer contains a recognizable NTFS boot
    sector, otherwise it returns FALSE.

--*/

{
    PAGED_CODE();

    //
    // Now perform all the checks, starting with the Name and Checksum.
    // The remaining checks should be obvious, including some fields which
    // must be 0 and other fields which must be a small power of 2.
    //

    if (BootSector->Oem[0] == 'N' &&
        BootSector->Oem[1] == 'T' &&
        BootSector->Oem[2] == 'F' &&
        BootSector->Oem[3] == 'S' &&
        BootSector->Oem[4] == ' ' &&
        BootSector->Oem[5] == ' ' &&
        BootSector->Oem[6] == ' ' &&
        BootSector->Oem[7] == ' '

            &&

        //
        // Check number of bytes per sector.  The low order byte of this
        // number must be zero (smallest sector size = 0x100) and the
        // high order byte shifted must equal the bytes per sector gotten
        // from the device and stored in the Vcb.  And just to be sure,
        // sector size must be less than page size.
        //

        BootSector->PackedBpb.BytesPerSector[0] == 0

            &&

        ((ULONG) (BootSector->PackedBpb.BytesPerSector[1] << 8) == BytesPerSector)

            &&

        BootSector->PackedBpb.BytesPerSector[1] << 8 <= PAGE_SIZE

            &&

        //
        //  Sectors per cluster must be a power of 2.
        //

        (BootSector->PackedBpb.SectorsPerCluster[0] == 0x1 ||
         BootSector->PackedBpb.SectorsPerCluster[0] == 0x2 ||
         BootSector->PackedBpb.SectorsPerCluster[0] == 0x4 ||
         BootSector->PackedBpb.SectorsPerCluster[0] == 0x8 ||
         BootSector->PackedBpb.SectorsPerCluster[0] == 0x10 ||
         BootSector->PackedBpb.SectorsPerCluster[0] == 0x20 ||
         BootSector->PackedBpb.SectorsPerCluster[0] == 0x40 ||
         BootSector->PackedBpb.SectorsPerCluster[0] == 0x80)

            &&

        //
        //  These fields must all be zero.  For both Fat and HPFS, some of
        //  these fields must be nonzero.
        //

        BootSector->PackedBpb.ReservedSectors[0] == 0 &&
        BootSector->PackedBpb.ReservedSectors[1] == 0 &&
        BootSector->PackedBpb.Fats[0] == 0 &&
        BootSector->PackedBpb.RootEntries[0] == 0 &&
        BootSector->PackedBpb.RootEntries[1] == 0 &&
        BootSector->PackedBpb.Sectors[0] == 0 &&
        BootSector->PackedBpb.Sectors[1] == 0 &&
        BootSector->PackedBpb.SectorsPerFat[0] == 0 &&
        BootSector->PackedBpb.SectorsPerFat[1] == 0 &&
        BootSector->PackedBpb.LargeSectors[0] == 0 &&
        BootSector->PackedBpb.LargeSectors[1] == 0 &&
        BootSector->PackedBpb.LargeSectors[2] == 0 &&
        BootSector->PackedBpb.LargeSectors[3] == 0

            &&

        //
        //  Number of Sectors cannot be greater than the number of sectors
        //  on the partition.
        //

        !( BootSector->NumberSectors.QuadPart > NumberOfSectors->QuadPart )

            &&

        //
        //  Check that both Lcn values are for sectors within the partition.
        //

        !( BootSector->MftStartLcn.QuadPart *
                    BootSector->PackedBpb.SectorsPerCluster[0] >
                NumberOfSectors->QuadPart )

            &&

        !( BootSector->Mft2StartLcn.QuadPart *
                    BootSector->PackedBpb.SectorsPerCluster[0] >
                NumberOfSectors->QuadPart )

            &&

        //
        //  Clusters per file record segment and default clusters for Index
        //  Allocation Buffers must be a power of 2.  A negative number indicates
        //  a shift value to get the actual size of the structure.
        //

        ((BootSector->ClustersPerFileRecordSegment >= -31 &&
          BootSector->ClustersPerFileRecordSegment <= -9) ||
         BootSector->ClustersPerFileRecordSegment == 0x1 ||
         BootSector->ClustersPerFileRecordSegment == 0x2 ||
         BootSector->ClustersPerFileRecordSegment == 0x4 ||
         BootSector->ClustersPerFileRecordSegment == 0x8 ||
         BootSector->ClustersPerFileRecordSegment == 0x10 ||
         BootSector->ClustersPerFileRecordSegment == 0x20 ||
         BootSector->ClustersPerFileRecordSegment == 0x40)

            &&

        ((BootSector->DefaultClustersPerIndexAllocationBuffer >= -31 &&
          BootSector->DefaultClustersPerIndexAllocationBuffer <= -9) ||
         BootSector->DefaultClustersPerIndexAllocationBuffer == 0x1 ||
         BootSector->DefaultClustersPerIndexAllocationBuffer == 0x2 ||
         BootSector->DefaultClustersPerIndexAllocationBuffer == 0x4 ||
         BootSector->DefaultClustersPerIndexAllocationBuffer == 0x8 ||
         BootSector->DefaultClustersPerIndexAllocationBuffer == 0x10 ||
         BootSector->DefaultClustersPerIndexAllocationBuffer == 0x20 ||
         BootSector->DefaultClustersPerIndexAllocationBuffer == 0x40)) {

        return TRUE;

    } else {

        //
        // This does not appear to be an NTFS volume.
        //

        return FALSE;
    }
}

