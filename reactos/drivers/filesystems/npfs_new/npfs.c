/*
 * PROJECT:         ReactOS Drivers
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/npfs/npfs.c
 * PURPOSE:         Named pipe filesystem
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include "npfs.h"

//#define NDEBUG
#include <debug.h>

ULONG NpfsDebugLevel = NPFS_DL_API_TRACE;

static VOID NpfsInitializeVcb(PNPFS_VCB Vcb);

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
			PUNICODE_STRING RegistryPath)
{
	PNPFS_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING DeviceName;
	NTSTATUS Status;

	DPRINT("Named Pipe File System Driver v0.1\n");

	ASSERT (sizeof(NPFS_CONTEXT) <= FIELD_OFFSET(IRP, Tail.Overlay.DriverContext));
	ASSERT (sizeof(NPFS_WAITER_ENTRY) <= FIELD_OFFSET(IRP, Tail.Overlay.DriverContext));

	DriverObject->MajorFunction[IRP_MJ_CREATE] = NpfsCreate;
	DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = NpfsCreateNamedPipe;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = NpfsClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = NpfsRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = NpfsWrite;
	DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = NpfsQueryInformation;
	DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = NpfsSetInformation;
	DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = NpfsQueryVolumeInformation;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NpfsCleanup;
	DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = NpfsFlushBuffers;
	DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = NpfsDirectoryControl;
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = NpfsFileSystemControl;
	//DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = NpfsQuerySecurity;
	//DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = NpfsSetSecurity;

	DriverObject->DriverUnload = NULL;

	RtlInitUnicodeString(&DeviceName, L"\\Device\\NamedPipe");
	Status = IoCreateDevice(DriverObject,
		sizeof(NPFS_DEVICE_EXTENSION),
		&DeviceName,
		FILE_DEVICE_NAMED_PIPE,
		0,
		FALSE,
		&DeviceObject);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Failed to create named pipe device! (Status %x)\n", Status);
		return Status;
	}

	/* initialize the device object */
	DeviceObject->Flags = DO_DIRECT_IO;

	/* initialize the device extension */
	DeviceExtension = DeviceObject->DeviceExtension;

	NpfsInitializeVcb(&DeviceExtension->Vcb);

	InitializeListHead(&DeviceExtension->PipeListHead);
	InitializeListHead(&DeviceExtension->ThreadListHead);
	KeInitializeMutex(&DeviceExtension->PipeListLock, 0);
	DeviceExtension->EmptyWaiterCount = 0;

	/* set the size quotas */
	DeviceExtension->MinQuota = PAGE_SIZE;
	DeviceExtension->DefaultQuota = 8 * PAGE_SIZE;
	DeviceExtension->MaxQuota = 64 * PAGE_SIZE;

	return STATUS_SUCCESS;
}

static VOID
NpfsInitializeVcb(PNPFS_VCB Vcb)
{
	/* Initialize Volume Control Block, it's one per whole named pipe
	   file system. */

	NpfsDbgPrint(NPFS_DL_API_TRACE, "NpfsInitializeVcb(), Vcb = %p\n", Vcb);

	/* Zero-init whole VCB */
	RtlZeroMemory(Vcb, sizeof(NPFS_VCB));

	/* Initialize the common header */
	Vcb->NodeTypeCode = NPFS_NODETYPE_VCB;
	Vcb->NodeByteSize = sizeof(NPFS_VCB);
}

VOID NTAPI
NpfsDbgPrint(ULONG Level, char *fmt, ...)
{
	va_list args;
	char str[300];

	if (!(NpfsDebugLevel & Level))
		return;

	va_start(args, fmt);
	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);
	DbgPrint ("%4.4d: NPFS %s", PsGetCurrentProcessId(), str);
}


/* EOF */
