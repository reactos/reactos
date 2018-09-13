
/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    hanfnc.c

Abstract:

    default handlers for hal functions which don't get handlers
    installed by the hal

--*/

#ifndef _DRIVESUP_H_
#define _DRIVESUP_H_

#define BOOTABLE_PARTITION  0
#define PRIMARY_PARTITION   1
#define LOGICAL_PARTITION   2
#define FT_PARTITION        3
#define OTHER_PARTITION     4

NTSTATUS
FASTCALL
xHalIoClearPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads
    );

#endif
