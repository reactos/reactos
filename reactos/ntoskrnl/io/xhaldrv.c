/* $Id: xhaldrv.c,v 1.1 2000/06/30 22:52:49 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/xhaldrv.c
 * PURPOSE:         Hal drive routines
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 19/06/2000
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/xhal.h>

//#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
FASTCALL
xHalExamineMBR (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	ULONG		SectorSize,
	IN	ULONG		MBRTypeIdentifier,
	OUT	PVOID		* Buffer
	)
{
	KEVENT Event;
	IO_STATUS_BLOCK StatusBlock;
	LARGE_INTEGER Offset;
	PUCHAR LocalBuffer;
	PIRP Irp;
	NTSTATUS Status;

	DPRINT ("xHalExamineMBR()\n");
	*Buffer = NULL;

	if (SectorSize < 512)
		SectorSize = 512;
	if (SectorSize > 4096)
		SectorSize = 4096;

	LocalBuffer = (PUCHAR)ExAllocatePool (PagedPool,
	                                      SectorSize);
	if (LocalBuffer == NULL)
		return;

	KeInitializeEvent (&Event,
	                   NotificationEvent,
	                   FALSE);

	Offset.QuadPart = 0;

	Irp = IoBuildSynchronousFsdRequest (IRP_MJ_READ,
	                                    DeviceObject,
	                                    LocalBuffer,
	                                    SectorSize,
	                                    &Offset,
	                                    &Event,
	                                    &StatusBlock);

	Status = IoCallDriver (DeviceObject,
	                       Irp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject (&Event,
		                       Executive,
		                       KernelMode,
		                       FALSE,
		                       NULL);
		Status = StatusBlock.Status;
	}

	if (!NT_SUCCESS(Status))
	{
		DPRINT ("xHalExamineMBR failed (Status = 0x%08lx)\n",
		        Status);
		ExFreePool (LocalBuffer);
		return;
	}

	if (LocalBuffer[0x1FE] != 0x55 || LocalBuffer[0x1FF] != 0xAA)
	{
		DPRINT ("xHalExamineMBR: invalid MBR signature\n");
		ExFreePool (LocalBuffer);
		return;
	}

	if (LocalBuffer[0x1C2] != MBRTypeIdentifier)
	{
		DPRINT ("xHalExamineMBR: invalid MBRTypeIdentifier\n");
		ExFreePool (LocalBuffer);
		return;
	}

	*Buffer = (PVOID)LocalBuffer;
}

VOID
FASTCALL
xHalIoAssignDriveLetters (
	IN	PLOADER_PARAMETER_BLOCK	LoaderBlock,
	IN	PSTRING			NtDeviceName,
	OUT	PUCHAR			NtSystemPath,
	OUT	PSTRING			NtSystemPathString
	)
{
	PCONFIGURATION_INFORMATION ConfigInfo;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK StatusBlock;
	UNICODE_STRING UnicodeString1;
	UNICODE_STRING UnicodeString2;
	HANDLE FileHandle;
	PWSTR Buffer1;
	PWSTR Buffer2;
	ULONG i;
	NTSTATUS Status;

	DPRINT ("xHalIoAssignDriveLetters()\n");

	ConfigInfo = IoGetConfigurationInformation ();

	Buffer1 = (PWSTR)ExAllocatePool (PagedPool,
	                                 64 * sizeof(WCHAR));
	Buffer2 = (PWSTR)ExAllocatePool (PagedPool,
	                                 32 * sizeof(WCHAR));

	// Create PhysicalDrive links
	DPRINT ("Physical disk drives: %d\n", ConfigInfo->DiskCount);

	for (i = 0; i < ConfigInfo->DiskCount; i++)
	{
		swprintf (Buffer1,
		          L"\\Device\\Harddisk%d\\Partition0",
		          i);
		RtlInitUnicodeString (&UnicodeString1,
		                      Buffer1);

		InitializeObjectAttributes (&ObjectAttributes,
		                            &UnicodeString1,
		                            0,
		                            NULL,
		                            NULL);

		Status = NtOpenFile (&FileHandle,
		                     0x10001,
		                     &ObjectAttributes,
		                     &StatusBlock,
		                     1,
		                     0x20);
		if (NT_SUCCESS(Status))
		{
			swprintf (Buffer2,
			          L"\\??\\PhysicalDrive%d",
			          i);
			RtlInitUnicodeString (&UnicodeString2,
			                      Buffer2);

			DPRINT ("Creating link: %S ==> %S\n",
			        Buffer2,
			        Buffer1);

			IoCreateSymbolicLink (&UnicodeString2,
			                      &UnicodeString1);

			NtClose (FileHandle);
		}
	}
	ExFreePool (Buffer2);
	ExFreePool (Buffer1);

	// Assign pre-assigned (registry) partitions

	// Assign bootable partitions

	// Assign remaining primary partitions

	// Assign extended (logical) partitions

	// Assign floppy drives
	DPRINT("Floppy drives: %d\n", ConfigInfo->FloppyCount);

	// Assign cdrom drives
	DPRINT("CD-Rom drives: %d\n", ConfigInfo->CDRomCount);

	// Any more ??

}

/* EOF */