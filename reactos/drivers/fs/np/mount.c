/* $Id: mount.c,v 1.7 2001/05/10 23:38:31 ekohl Exp $
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

//#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
{
   PNPFS_DEVICE_EXTENSION DeviceExtension;
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName;
   UNICODE_STRING LinkName;
   NTSTATUS Status;
   
   DbgPrint("Named Pipe FSD 0.0.2\n");
   
   DeviceObject->Flags = 0;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = NpfsCreate;
   DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] =
     NpfsCreateNamedPipe;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = NpfsClose;
   DriverObject->MajorFunction[IRP_MJ_READ] = NpfsRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = NpfsWrite;
//   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
//     NpfsDirectoryControl;
//   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
//     NpfsQueryInformation;
//   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
//     NpfsSetInformation;
//   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = NpfsFlushBuffers;
//   DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = NpfsShutdown;
//   DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = 
//     NpfsQuerySecurity;
//   DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] =
//     NpfsSetSecurity;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
     NpfsFileSystemControl;
   
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
   
#if 0
   /* FIXME: this should really be done by SMSS!! */
   RtlInitUnicodeString(&LinkName, L"\\??\\PIPE");
   Status = IoCreateSymbolicLink(&LinkName,
				 &DeviceName);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failed to create named pipe symbolic link! (Status %x)\n", Status);
//	IoDeleteDevice();
	return(Status);
     }
#endif
   
   /* initialize the device extension */
   DeviceExtension = DeviceObject->DeviceExtension;
   InitializeListHead(&DeviceExtension->PipeListHead);
   KeInitializeMutex(&DeviceExtension->PipeListLock,
		     0);
   
   return(STATUS_SUCCESS);
}

/* EOF */
