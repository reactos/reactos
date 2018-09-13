/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    partmgrp.h

Abstract:

    This file defines the public interfaces for the PARTMGR driver.

Author:

    norbertk

Revision History:

--*/

//
// Define IOCTL so that volume managers can get another crack at
// partitions that are unclaimed.
//

#define IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS    CTL_CODE('p', 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL is for clusters to tell the volume managers for the
// given disk to stop using it.  You can undo this operation with
// IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS.
//

#define IOCTL_PARTMGR_EJECT_VOLUME_MANAGERS         CTL_CODE('p', 1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
