/* $Id: xhaldisp.c,v 1.12 2004/11/21 06:51:18 ion Exp $
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* DATA **********************************************************************/


HAL_DISPATCH EXPORTED HalDispatchTable =
{
	HAL_DISPATCH_VERSION,
	(pHalQuerySystemInformation) NULL,	// HalQuerySystemInformation
	(pHalSetSystemInformation) NULL,	// HalSetSystemInformation
	(pHalQueryBusSlots) NULL,			// HalQueryBusSlots
	0,
	(pHalExamineMBR) xHalExamineMBR,
	(pHalIoAssignDriveLetters) xHalIoAssignDriveLetters,
	(pHalIoReadPartitionTable) xHalIoReadPartitionTable,
	(pHalIoSetPartitionInformation) xHalIoSetPartitionInformation,
	(pHalIoWritePartitionTable) xHalIoWritePartitionTable,
	(pHalHandlerForBus) NULL,			// HalReferenceHandlerForBus
	(pHalReferenceBusHandler) NULL,		// HalReferenceBusHandler
	(pHalReferenceBusHandler) NULL,		// HalDereferenceBusHandler
	(pHalInitPnpDriver) NULL,               //HalInitPnpDriver;
	(pHalInitPowerManagement) NULL,         //HalInitPowerManagement;
	(pHalGetDmaAdapter) NULL,               //HalGetDmaAdapter;
	(pHalGetInterruptTranslator) NULL,      //HalGetInterruptTranslator;
	(pHalStartMirroring) NULL,              //HalStartMirroring;
	(pHalEndMirroring) NULL,                //HalEndMirroring;
	(pHalMirrorPhysicalMemory) NULL,        //HalMirrorPhysicalMemory;
	(pHalEndOfBoot) NULL,                   //HalEndOfBoot;
	(pHalMirrorVerify) NULL                //HalMirrorVerify;
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
NTSTATUS
STDCALL
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
NTSTATUS
STDCALL
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
NTSTATUS
STDCALL
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
NTSTATUS
STDCALL
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
NTSTATUS
STDCALL
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
NTSTATUS
STDCALL
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
NTSTATUS
STDCALL
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
NTSTATUS
STDCALL
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
NTSTATUS
STDCALL
IoWritePartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _DRIVE_LAYOUT_INFORMATION_EX* DriveLayfout
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
