/* $Id: xhaldisp.c,v 1.9 2004/06/23 21:42:50 ion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/xhaldisp.c
 * PURPOSE:         Hal dispatch tables
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 19/06/2000
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/xhal.h>

#define NDEBUG
#include <internal/debug.h>


/* DATA **********************************************************************/


HAL_DISPATCH EXPORTED HalDispatchTable =
{
	HAL_DISPATCH_VERSION,
	(pHalQuerySystemInformation) NULL,	// HalQuerySystemInformation
	(pHalSetSystemInformation) NULL,	// HalSetSystemInformation
	(pHalQueryBusSlots) NULL,			// HalQueryBusSlots
	(pHalDeviceControl) NULL,			// HalDeviceControl
	(pHalExamineMBR) xHalExamineMBR,
	(pHalIoAssignDriveLetters) xHalIoAssignDriveLetters,
	(pHalIoReadPartitionTable) xHalIoReadPartitionTable,
	(pHalIoSetPartitionInformation) xHalIoSetPartitionInformation,
	(pHalIoWritePartitionTable) xHalIoWritePartitionTable,
	(pHalHandlerForBus) NULL,			// HalReferenceHandlerForBus
	(pHalReferenceBusHandler) NULL,		// HalReferenceBusHandler
	(pHalReferenceBusHandler) NULL		// HalDereferenceBusHandler
};


HAL_PRIVATE_DISPATCH EXPORTED HalPrivateDispatchTable =
{
	HAL_PRIVATE_DISPATCH_VERSION
				// HalHandlerForBus
				// HalHandlerForConfigSpace
				// HalCompleteDeviceControl
				// HalRegisterBusHandler
				// ??
				// ??
				// ??
				// ??
				// ??
};

/*
 * @unimplemented
 *
STDCALL
VOID
IoAssignDriveLetters(
   		IN PLOADER_PARAMETER_BLOCK   	 LoaderBlock,
		IN PSTRING  	NtDeviceName,
		OUT PUCHAR  	NtSystemPath,
		OUT PSTRING  	NtSystemPathString
    )
{
	UNIMPLEMENTED;
*/

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoCreateDisk(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _CREATE_DISK* Disk
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoGetBootDiskInformation(
    IN OUT PBOOTDISK_INFORMATION BootDiskInformation,
    IN ULONG Size
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoReadDiskSignature(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG BytesPerSector,
    OUT PDISK_SIGNATURE Signature
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoReadPartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _DRIVE_LAYOUT_INFORMATION_EX** DriveLayout
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoSetPartitionInformationEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG PartitionNumber,
    IN struct _SET_PARTITION_INFORMATION_EX* PartitionInfo
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoSetSystemPartition(
    PUNICODE_STRING VolumeNameString
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoVerifyPartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FixErrors
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoVolumeDeviceToDosName(
    IN  PVOID           VolumeDeviceObject,
    OUT PUNICODE_STRING DosName
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
NTSTATUS
IoWritePartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _DRIVE_LAYOUT_INFORMATION_EX* DriveLayfout
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
