/* $Id: mount.c,v 1.5 2001/05/01 11:09:01 ekohl Exp $
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


/* GLOBALS *******************************************************************/

//static PDRIVER_OBJECT DriverObject;

/* FUNCTIONS *****************************************************************/

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,
		     PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS Status;
   UNICODE_STRING DeviceName;
   UNICODE_STRING LinkName;
   
   DbgPrint("Named Pipe Filesystem\n");
   
//   DriverObject = _DriverObject;
   
#if 0
   RtlInitUnicodeString(&DeviceName, L"\\Device\\Npfs");
   Status = IoCreateDevice(DriverObject,
			   0,
			   &DeviceName,
			   FILE_DEVICE_NAMED_PIPE,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   RtlInitUnicodeString(&LinkName, L"\\??\\Pipe");
   Status = IoCreateSymbolicLink(&LinkName,
				 &DeviceName);
#endif
   
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
//   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NpfsCleanup;
//   DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = 
//     NpfsQuerySecurity;
//   DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] =
//     NpfsSetSecurity;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
     NpfsFileSystemControl;
   
   DriverObject->DriverUnload = NULL;
   
   RtlInitUnicodeString(&DeviceName, L"\\Device\\Npfs");
   Status = IoCreateDevice(DriverObject,
			   0,
			   &DeviceName,
			   FILE_DEVICE_NAMED_PIPE,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("Failed to create named pipe device! (Status %x)\n", Status);
	return(Status);
     }
   
   RtlInitUnicodeString(&LinkName, L"\\??\\Pipe");
   Status = IoCreateSymbolicLink(&LinkName,
				 &DeviceName);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("Failed to create named pipe symbolic link! (Status %x)\n", Status);

//	IoDeleteDevice();
	return(Status);
     }
   
   NpfsInitPipeList();
   
   return(STATUS_SUCCESS);
}


/* EOF */
