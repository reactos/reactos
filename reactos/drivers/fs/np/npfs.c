/* $Id: npfs.c,v 1.7 2003/09/20 20:31:57 weiden Exp $
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
   
   DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)NpfsCreate;
   DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] =
     (PDRIVER_DISPATCH)NpfsCreateNamedPipe;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)NpfsClose;
   DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH)NpfsRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH)NpfsWrite;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
     (PDRIVER_DISPATCH)NpfsQueryInformation;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
     (PDRIVER_DISPATCH)NpfsSetInformation;
   DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = 
     (PDRIVER_DISPATCH)NpfsQueryVolumeInformation;
//   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = (PDRIVER_DISPATCH)NpfsCleanup;
//   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = (PDRIVER_DISPATCH)NpfsFlushBuffers;
//   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
//     (PDRIVER_DISPATCH)NpfsDirectoryControl;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
     (PDRIVER_DISPATCH)NpfsFileSystemControl;
//   DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = 
//     (PDRIVER_DISPATCH)NpfsQuerySecurity;
//   DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] =
//     (PDRIVER_DISPATCH)NpfsSetSecurity;
   
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
