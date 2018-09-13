/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fat_rec.c

Abstract:

    This module contains the mini-file system recognizer for FAT.

Author:

    Darryl E. Havens (darrylh) 8-dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "fs_rec.h"
#include "fat_rec.h"

//
//  The local debug trace level
//

#define Dbg                              (FSREC_DEBUG_LEVEL_FAT)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,FatRecFsControl)
#pragma alloc_text(PAGE,IsFatVolume)
#pragma alloc_text(PAGE,UnpackBiosParameterBlock)
#endif // ALLOC_PRAGMA


NTSTATUS
FatRecFsControl(
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
    UNICODE_STRING driverName;
    ULONG bytesPerSector;
    BOOLEAN isDeviceFailure = FALSE;

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
        // question is a FAT volume and, if so, let the I/O system know that it
        // is by returning a special status code so that this driver can get
        // called back to load the FAT file system.
        //

        status = STATUS_UNRECOGNIZED_VOLUME;

        //
        // Attempt to determine whether or not the target volume being mounted
        // is a FAT volume.  Note that if an error occurs, and this is a floppy
        // drive, and the error occurred on the actual read from the device,
        // then the FAT file system will actually be loaded to handle the
        // problem since this driver is a place holder and does not need to
        // know all of the protocols for handling floppy errors.
        //

        targetDevice = irpSp->Parameters.MountVolume.DeviceObject;

        //
        //  First retrieve the sector size for this media.
        //

        if (FsRecGetDeviceSectorSize( targetDevice,
                                      &bytesPerSector )) {

            byteOffset.QuadPart = 0;
            buffer = NULL;

            if (FsRecReadBlock( targetDevice,
                                &byteOffset,
                                512,
                                bytesPerSector,
                                &buffer,
                                &isDeviceFailure ) &&
                IsFatVolume( buffer )) {
                    
                status = STATUS_FS_DRIVER_REQUIRED;
                
            }

            if (buffer != NULL) {
                ExFreePool( buffer );
            }
            
         } else {

             //
             //  Devices that can't get us this much ...
             //

             isDeviceFailure = TRUE;
         }
            
         //
         //  See if we should make the real filesystem take a shot at a wacky floppy.
         //
         
         if (isDeviceFailure) {
             if (targetDevice->Characteristics & FILE_FLOPPY_DISKETTE) {
                 status = STATUS_FS_DRIVER_REQUIRED;
             }
         }

         break;

    case IRP_MN_LOAD_FILE_SYSTEM:

        status = FsRecLoadFileSystem( DeviceObject,
                                      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Fastfat" );
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
IsFatVolume(
    IN PPACKED_BOOT_SECTOR Buffer
    )

/*++

Routine Description:

    This routine looks at the buffer passed in which contains the FAT boot
    sector and determines whether or not it represents an actual FAT boot
    sector.

Arguments:

    Buffer - Pointer to buffer containing potential boot block.

Return Value:

    The function returns TRUE if the buffer contains a recognizable FAT boot
    sector, otherwise it returns FALSE.

--*/

{
    BIOS_PARAMETER_BLOCK bios;
    BOOLEAN result;

    PAGED_CODE();

    //
    // Begin by unpacking the Bios Parameter Block that is packed in the boot
    // sector so that it can be examined without incurring alignment faults.
    //

    UnpackBiosParameterBlock( &Buffer->PackedBpb, &bios );

    //
    // Assume that the sector represents a FAT boot block and then determine
    // whether or not it really does.
    //

    result = TRUE;

    if (bios.Sectors) {
        bios.LargeSectors = 0;
    }

    // FMR Jul.11.1994 NaokiM - Fujitsu -
    // FMR boot sector has 'IPL1' string at the beginnig.

    if (Buffer->Jump[0] != 0x49 && /* FMR */
        Buffer->Jump[0] != 0xe9 &&
        Buffer->Jump[0] != 0xeb) {

        result = FALSE;


    // FMR Jul.11.1994 NaokiM - Fujitsu -
    // Sector size of FMR partition is 2048.

    } else if (bios.BytesPerSector !=  128 &&
               bios.BytesPerSector !=  256 &&
               bios.BytesPerSector !=  512 &&
               bios.BytesPerSector != 1024 &&
               bios.BytesPerSector != 2048 && /* FMR */
               bios.BytesPerSector != 4096) {

        result = FALSE;

    } else if (bios.SectorsPerCluster !=  1 &&
               bios.SectorsPerCluster !=  2 &&
               bios.SectorsPerCluster !=  4 &&
               bios.SectorsPerCluster !=  8 &&
               bios.SectorsPerCluster != 16 &&
               bios.SectorsPerCluster != 32 &&
               bios.SectorsPerCluster != 64 &&
               bios.SectorsPerCluster != 128) {

        result = FALSE;

    } else if (!bios.ReservedSectors) {

        result = FALSE;

    } else if (!bios.Fats) {

        result = FALSE;

    //
    // Prior to DOS 3.2 might contains value in both of Sectors and
    // Sectors Large.
    //
    } else if (!bios.Sectors && !bios.LargeSectors) {

        result = FALSE;

    // FMR Jul.11.1994 NaokiM - Fujitsu -
    // 1. Media descriptor of FMR partitions is 0xfa.
    // 2. Media descriptor of partitions formated by FMR OS/2 is 0x00.
    // 3. Media descriptor of floppy disks formated by FMR DOS is 0x01.

    } else if (bios.Media != 0x00 && /* FMR */
               bios.Media != 0x01 && /* FMR */
               bios.Media != 0xf0 &&
               bios.Media != 0xf8 &&
               bios.Media != 0xf9 &&
               bios.Media != 0xfa && /* FMR */
               bios.Media != 0xfb &&
               bios.Media != 0xfc &&
               bios.Media != 0xfd &&
               bios.Media != 0xfe &&
               bios.Media != 0xff) {

        result = FALSE;

    } else if (bios.SectorsPerFat != 0 && bios.RootEntries == 0) {

        result = FALSE;
    }

    return result;
}


VOID
UnpackBiosParameterBlock(
    IN PPACKED_BIOS_PARAMETER_BLOCK Bios,
    OUT PBIOS_PARAMETER_BLOCK UnpackedBios
    )

/*++

Routine Description:

    This routine copies a packed Bios Parameter Block to an unpacked Bios
    Parameter Block.

Arguments:

    Bios - Pointer to the packed Bios Parameter Block.

    UnpackedBios - Pointer to the unpacked Bios Parameter Block.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Unpack the Bios Parameter Block.
    //

    CopyUchar2( &UnpackedBios->BytesPerSector, &Bios->BytesPerSector[0] );
    CopyUchar2( &UnpackedBios->BytesPerSector, &Bios->BytesPerSector[0] );
    CopyUchar1( &UnpackedBios->SectorsPerCluster, &Bios->SectorsPerCluster[0] );
    CopyUchar2( &UnpackedBios->ReservedSectors, &Bios->ReservedSectors[0] );
    CopyUchar1( &UnpackedBios->Fats, &Bios->Fats[0] );
    CopyUchar2( &UnpackedBios->RootEntries, &Bios->RootEntries[0] );
    CopyUchar2( &UnpackedBios->Sectors, &Bios->Sectors[0] );
    CopyUchar1( &UnpackedBios->Media, &Bios->Media[0] );
    CopyUchar2( &UnpackedBios->SectorsPerFat, &Bios->SectorsPerFat[0] );
    CopyUchar2( &UnpackedBios->SectorsPerTrack, &Bios->SectorsPerTrack[0] );
    CopyUchar2( &UnpackedBios->Heads, &Bios->Heads[0] );
    CopyUchar4( &UnpackedBios->HiddenSectors, &Bios->HiddenSectors[0] );
    CopyUchar4( &UnpackedBios->LargeSectors, &Bios->LargeSectors[0] );
}
