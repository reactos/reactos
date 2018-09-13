/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cdfs_rec.c

Abstract:

    This module contains the mini-file system recognizer for CDFS.

Author:

    Darryl E. Havens (darrylh) 8-dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "fs_rec.h"

//
//  The local debug trace level
//

#define Dbg                              (FSREC_DEBUG_LEVEL_CDFS)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CdfsRecFsControl)
#endif // ALLOC_PRAGMA


NTSTATUS
CdfsRecFsControl(
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

    PAGED_CODE();

    //
    // Begin by determining what function that is to be performed.
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( irpSp->MinorFunction ) {

    case IRP_MN_MOUNT_VOLUME:

        //
        //  Always request the filesystem driver.
        //
        
        status = STATUS_FS_DRIVER_REQUIRED;
        break;

    case IRP_MN_LOAD_FILE_SYSTEM:

        status = FsRecLoadFileSystem( DeviceObject,
                                      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Cdfs" );
        break;

    default:
        
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    //
    // Finally, complete the request and return the same status code to the
    // caller.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}
