/* $Id: npfs.c,v 1.6 2003/06/21 19:55:55 hbirr Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/mount.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
{
   PNPFS_DEVICE_EXTENSION DeviceExtension;
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName;
   NTSTATUS Status;
   
   DbgPrint("Named Pipe FSD 0.0.2\n");
   
   DriverObject->MajorFunction[IRP_MJ_CREATE] = NpfsCreate;
   DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] =
     NpfsCreateNamedPipe;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = NpfsClose;
   DriverObject->MajorFunction[IRP_MJ_READ] = NpfsRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = NpfsWrite;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
     NpfsQueryInformation;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
     NpfsSetInformation;
   DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = 
     NpfsQueryVolumeInformation;
//   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NpfsCleanup;
//   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = NpfsFlushBuffers;
//   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
//     NpfsDirectoryControl;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
     NpfsFileSystemControl;
//   DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = 
//     NpfsQuerySecurity;
//   DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] =
//     NpfsSetSecurity;
   
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
	return(Status);
     }
   
   /* initialize the device object */
   DeviceObject->Flags = DO_DIRECT_IO;
   
   /* initialize the device extension */
   DeviceExtension = DeviceObject->DeviceExtension;
   InitializeListHead(&DeviceExtension->PipeListHead);
   KeInitializeMutex(&DeviceExtension->PipeListLock,
		     0);

   /* set the size quotas */
   DeviceExtension->MinQuota = PAGE_SIZE;
   DeviceExtension->DefaultQuota = 8 * PAGE_SIZE;
   DeviceExtension->MaxQuota = 64 * PAGE_SIZE;

   return(STATUS_SUCCESS);
}

/* EOF */
