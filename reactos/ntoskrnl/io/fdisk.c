/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/fdisk.c
 * PURPOSE:         Handling fixed disks
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS IoReadPartitionTable(PDEVICE_OBJECT DeviceObject,
			      ULONG SectorSize,
			      BOOLEAN ReturnRecognizedPartitions,
			      struct _DRIVE_LAYOUT_INFORMATION** PBuffer)
{
   UNIMPLEMENTED;
}

NTSTATUS IoSetPartitionInformation(PDEVICE_OBJECT DeviceObject,
				   ULONG SectorSize,
				   ULONG PartitionNumber,
				   ULONG PartitionType)
{
   UNIMPLEMENTED;
}

NTSTATUS IoWritePartitionTable(PDEVICE_OBJECT DeviceObject,
			       ULONG SectorSize,
			       ULONG SectorsPerTrack,
			       ULONG NumberOfHeads,
			       struct _DRIVE_LAYOUT_INFORMATION* PBuffer)
{
   UNIMPLEMENTED;
}
