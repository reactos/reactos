#ifndef __INCLUDE_INTERNAL_XHAL_H
#define __INCLUDE_INTERNAL_XHAL_H

VOID
FASTCALL
xHalExamineMBR (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	ULONG		SectorSize,
	IN	ULONG		MBRTypeIdentifier,
	OUT	PVOID		* Buffer
	);

VOID
FASTCALL
xHalIoAssignDriveLetters (
	IN	PLOADER_PARAMETER_BLOCK	LoaderBlock,
	IN	PSTRING			NtDeviceName,
	OUT	PUCHAR			NtSystemPath,
	OUT	PSTRING			NtSystemPathString
	);

#endif
