/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    udfs_rec.c

Abstract:

    This module contains the mini-file system recognizer for UDFS.

Author:

    Dan Lovinger (danlo) 13-Feb-1997

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "fs_rec.h"
#include "udfs_rec.h"

//
//  The local debug trace level
//

#define Dbg                              (FSREC_DEBUG_LEVEL_UDFS)

//
//  Tables of tokens we have to parse up from mount-time on-disk structures
//

PARSE_KEYVALUE VsdIdentParseTable[] = {
    { VSD_IDENT_BEA01, VsdIdentBEA01 },
    { VSD_IDENT_TEA01, VsdIdentTEA01 },
    { VSD_IDENT_CDROM, VsdIdentCDROM },
    { VSD_IDENT_CD001, VsdIdentCD001 },
    { VSD_IDENT_CDW01, VsdIdentCDW01 },
    { VSD_IDENT_CDW02, VsdIdentCDW02 },
    { VSD_IDENT_NSR01, VsdIdentNSR01 },
    { VSD_IDENT_NSR02, VsdIdentNSR02 },
    { VSD_IDENT_BOOT2, VsdIdentBOOT2 },
    { NULL,            VsdIdentBad }
    };

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,IsUdfsVolume)
#pragma alloc_text(PAGE,UdfsFindInParseTable)
#pragma alloc_text(PAGE,UdfsRecFsControl)
#endif // ALLOC_PRAGMA


NTSTATUS
UdfsRecFsControl(
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


 -*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION deviceExtension;
    UNICODE_STRING driverName;
    ULONG bytesPerSector;
    PDEVICE_OBJECT targetDevice;

    PAGED_CODE();

    //
    // Begin by determining what function that is to be performed.
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( irpSp->MinorFunction ) {

    case IRP_MN_MOUNT_VOLUME:

        //
        // Attempt to mount a volume:  There are two different cases here:
        //
        //     1)  The device is being opened for DASD access, that is, no
        //         file system is required, thus it is OK to allow RAW to
        //         to open it.
        //
        //     2)  We need to rummage the media to see if this is a UDF volume.
        //

        status = STATUS_UNRECOGNIZED_VOLUME;

        targetDevice = irpSp->Parameters.MountVolume.DeviceObject;

        if (FsRecGetDeviceSectorSize( targetDevice,
                                      &bytesPerSector )) {
        
            if (IsUdfsVolume( targetDevice,
                              bytesPerSector )) {

                status = STATUS_FS_DRIVER_REQUIRED;
            }
        }

        break;

    case IRP_MN_LOAD_FILE_SYSTEM:

        status = FsRecLoadFileSystem( DeviceObject,
                                      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Udfs" );
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
IsUdfsVolume (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize
    )

/*++

Routine Description:

    This routine walks the Volume Recognition Sequence to determine
    whether this volume contains an NSR02 (ISO 13346 Section 4) image.
    
    Note: this routine is pretty much diked out of UdfsRecognizeVolume
    in the real filesystem, modulo fitting it into the fs recognizer.

Arguments:

    DeviceObject - device we are checking

    SectorSize - size of a physical sector on this device

Return Value:

    Boolean TRUE if we found NSR02, FALSE otherwise.

--*/

{
    BOOLEAN FoundNSR = FALSE;

    BOOLEAN FoundBEA = FALSE;
    BOOLEAN Resolved = FALSE;

    PVSD_GENERIC VolumeStructureDescriptor = NULL;
    ULONGLONG Offset = SectorAlignN( SectorSize, VRA_BOUNDARY_LOCATION );

    PAGED_CODE();

    DebugTrace(( +1, Dbg,
                 "IsUdfsVolume, DevObj %08x SectorSize %08x\n",
                 DeviceObject,
                 SectorSize ));

    while (!Resolved) {
    
        if (!FsRecReadBlock( DeviceObject,
                             (PLARGE_INTEGER)&Offset,
                             sizeof(VSD_GENERIC),
                             SectorSize,
                             (PVOID) &VolumeStructureDescriptor,
                             NULL )) {

            break;
        }
    
        //
        //  Now check the type of the descriptor. All ISO 13346 VSDs are
        //  of Type 0, 9660 PVDs are Type 1, 9660 SVDs are Type 2, and 9660
        //  terminating descriptors are Type 255.
        //
    
        if (VolumeStructureDescriptor->Type == 0) {

            //
            //  In order to properly recognize the volume, we must know all of the
            //  Structure identifiers in ISO 13346 so that we can terminate if a
            //  badly formatted (or, shockingly, non 13346) volume is presented to us.
            //

            switch (UdfsFindInParseTable( VsdIdentParseTable,
                                         VolumeStructureDescriptor->Ident,
                                         VSD_LENGTH_IDENT )) {
                case VsdIdentBEA01:

                    //
                    //  Only one BEA may exist and its version must be 1 (2/9.2.3)
                    //

                    DebugTrace(( 0, Dbg, "IsUdfsVolume, got a BEA01\n" ));


                    if ((FoundBEA &&
                         DebugTrace(( 0, Dbg,
                                      "IsUdfsVolume, ... but it is a duplicate!\n" ))) ||

                        (VolumeStructureDescriptor->Version != 1 &&
                         DebugTrace(( 0, Dbg,
                                      "IsUdfsVolume, ... but it has a wacky version number %02x != 1!\n",
                                      VolumeStructureDescriptor->Version )))) {

                        Resolved = TRUE;
                        break;
                    }

                    FoundBEA = TRUE;
                    break;

                case VsdIdentTEA01:

                    //
                    //  If we reach the TEA it must be the case that we don't recognize
                    //

                    DebugTrace(( 0, Dbg, "IsUdfsVolume, got a TEA01\n" ));

                    Resolved = TRUE;
                    break;

                case VsdIdentNSR02:

                    //
                    //  We recognize NSR02 version 1 embedded after a BEA (3/9.1.3).  For
                    //  simplicity we will not bother being a complete nitpick and check
                    //  for a bounding TEA, although we will be optimistic in the case where
                    //  we fail to match the version.
                    //

                    DebugTrace(( 0, Dbg, "IsUdfsVolume, got an NSR02\n" ));

                    if ((FoundBEA ||
                         !DebugTrace(( 0, Dbg, "IsUdfsVolume, ... but we haven't seen a BEA01 yet!\n" ))) &&

                        (VolumeStructureDescriptor->Version == 1 ||
                         !DebugTrace(( 0, Dbg, "IsUdfsVolume, ... but it has a wacky version number %02x != 1\n",
                                       VolumeStructureDescriptor->Version )))) {

                        FoundNSR = Resolved = TRUE;
                        break;
                    }

                    break;

                case VsdIdentCD001:
                case VsdIdentCDW01:
                case VsdIdentNSR01:
                case VsdIdentCDW02:
                case VsdIdentBOOT2:

                    DebugTrace(( 0, Dbg, "IsUdfsVolume, got a valid but uninteresting 13346 descriptor\n" ));

                    //
                    //  Valid but uninteresting (to us) descriptors
                    //

                    break;

                default:

                    DebugTrace(( 0, Dbg, "IsUdfsVolume, got an invalid 13346 descriptor\n" ));

                    //
                    //  Stumbling across something we don't know, it must be that this
                    //  is not a valid 13346 image
                    //

                    Resolved = TRUE;
                    break;

            }

        } else if (!FoundBEA && (VolumeStructureDescriptor->Type < 3 ||
                                 VolumeStructureDescriptor->Type == 255)) {

            DebugTrace(( 0, Dbg, "IsUdfsVolume, got a 9660 descriptor\n" ));

            //
            //  Only HSG (CDROM) and 9660 (CD001) are possible, and they are only legal
            //  before the ISO 13346 BEA/TEA extent.  By design, an ISO 13346 VSD precisely
            //  overlaps a 9660 PVD/SVD in the appropriate fields.
            //
            //  Note that we aren't being strict about the structure of the 9660 descriptors
            //  since that really isn't very interesting.  We care more about the 13346.
            //  
            //

            switch (UdfsFindInParseTable( VsdIdentParseTable,
                                          VolumeStructureDescriptor->Ident,
                                          VSD_LENGTH_IDENT )) {
                case VsdIdentCDROM:
                case VsdIdentCD001:

                    DebugTrace(( 0, Dbg, "IsUdfsVolume, ... seems we have 9660 here\n" ));

                    //
                    //  Note to our caller that we seem to have ISO 9660 here
                    //

                    break;

                default:

                    DebugTrace(( 0, Dbg, "IsUdfsVolume, ... but it looks wacky\n" ));

                    //
                    //  This probably was a false alert, but in any case there is nothing
                    //  on this volume for us.
                    //

                    Resolved = TRUE;
                    break;
            }

        } else {

            //
            //  Something else must be recorded on this volume.
            //

            DebugTrace(( 0, Dbg, "IsUdfsVolume, got an unrecognizeable descriptor, probably not 13346/9660\n" ));
            break;
        }

        //
        //  Align our next read with the sector following the current descriptor
        //

        Offset += SectorAlignN( SectorSize, sizeof(VSD_GENERIC) );
    }

    DebugTrace(( -1, Dbg, "IsUdfsVolume -> %c\n", ( FoundNSR ? 'T' : 'F' )));

    //
    //  Free up our temporary buffer
    //

    if (VolumeStructureDescriptor) {
    
        ExFreePool( VolumeStructureDescriptor );
    }

    return FoundNSR;
}


ULONG
UdfsFindInParseTable (
    IN PPARSE_KEYVALUE ParseTable,
    IN PCHAR Id,
    IN ULONG MaxIdLen
    )

/*++

Routine Description:

    This routine walks a table of string key/value information for a match of the
    input Id.  MaxIdLen can be set to get a prefix match.

Arguments:

    Table - This is the table being searched.

    Id - Key value.

    MaxIdLen - Maximum possible length of Id.

Return Value:

    Value of matching entry, or the terminating (NULL) entry's value.

--*/

{
    PAGED_CODE();

    while (ParseTable->Key != NULL) {

        if (RtlEqualMemory(ParseTable->Key, Id, MaxIdLen)) {

            break;
        }

        ParseTable++;
    }

    return ParseTable->Value;
}

