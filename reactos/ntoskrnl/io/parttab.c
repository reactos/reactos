/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/parttab.c
 * PURPOSE:         Handling fixed disks (partition table functions)
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS FASTCALL
IoReadPartitionTable(PDEVICE_OBJECT DeviceObject,
		     ULONG SectorSize,
		     BOOLEAN ReturnRecognizedPartitions,
		     PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
  return(HalIoReadPartitionTable(DeviceObject,
				 SectorSize,
				 ReturnRecognizedPartitions,
				 PartitionBuffer));
}


NTSTATUS FASTCALL
IoSetPartitionInformation(PDEVICE_OBJECT DeviceObject,
			  ULONG SectorSize,
			  ULONG PartitionNumber,
			  ULONG PartitionType)
{
  return(HalIoSetPartitionInformation(DeviceObject,
				      SectorSize,
				      PartitionNumber,
				      PartitionType));
}


NTSTATUS FASTCALL
IoWritePartitionTable(PDEVICE_OBJECT DeviceObject,
		      ULONG SectorSize,
		      ULONG SectorsPerTrack,
		      ULONG NumberOfHeads,
		      PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
  return(HalIoWritePartitionTable(DeviceObject,
				  SectorSize,
				  SectorsPerTrack,
				  NumberOfHeads,
				  PartitionBuffer));
}

/* EOF */
