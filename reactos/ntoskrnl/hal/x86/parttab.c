/* $Id: parttab.c,v 1.2 2000/06/30 22:51:34 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/parttab.c (was ntoskrnl/io/fdisk.c)
 * PURPOSE:         Handling fixed disks (partition table functions)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 * 2000-03-25 (ea)
 * 	Moved here from ntoskrnl/io/fdisk.c
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
IoReadPartitionTable (
	PDEVICE_OBJECT			DeviceObject,
	ULONG				SectorSize,
	BOOLEAN				ReturnRecognizedPartitions,
	PDRIVE_LAYOUT_INFORMATION	* PartitionBuffer
	)
{
	UNIMPLEMENTED;
}

NTSTATUS
STDCALL
IoSetPartitionInformation (
	PDEVICE_OBJECT	DeviceObject,
	ULONG		SectorSize,
	ULONG		PartitionNumber,
	ULONG		PartitionType)
{
	UNIMPLEMENTED;
}

NTSTATUS
STDCALL
IoWritePartitionTable (
	PDEVICE_OBJECT			DeviceObject,
	ULONG				SectorSize,
	ULONG				SectorsPerTrack,
	ULONG				NumberOfHeads,
	PDRIVE_LAYOUT_INFORMATION	* PBuffer
	)
{
	UNIMPLEMENTED;
}


/* EOF */
