/* $Id: mount.c,v 1.4 2000/05/13 13:51:08 dwelch Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/mount.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <internal/debug.h>

#include "npfs.h"

/* GLOBALS *******************************************************************/

static PDRIVER_OBJECT DriverObject;

/* FUNCTIONS *****************************************************************/

NTSTATUS DriverEntry(PDRIVER_OBJECT _DriverObject,
		     PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS Status;
   UNICODE_STRING DeviceName;
   UNICODE_STRING LinkName;
   
   DbgPrint("Named Pipe Filesystem\n");
   
   DriverObject = _DriverObject;
   
   RtlInitUnicodeString(&DeviceName, L"\\Device\\Npfs");
   Status = IoCreateDevice(DriverObject,
			   0,
			   &DeviceName,
			   FILE_DEVICE_FILE_SYSTEM,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   RtlInitUnicodeString(&LinkName, L"\\??\\pipe");
   Status = IoCreateSymbolicLink(&LinkName,
				 &DeviceName);
   
   DeviceObject->Flags = 0;
   DeviceObject->MajorFunction[IRP_MJ_CLOSE] = NpfsClose;
   DeviceObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] =
     NpfsCreateNamedPipe;
   DeviceObject->MajorFunction[IRP_MJ_CREATE] = NpfsCreate;
   DeviceObject->MajorFunction[IRP_MJ_READ] = NpfsRead;
   DeviceObject->MajorFunction[IRP_MJ_WRITE] = NpfsWrite;
   DeviceObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
     NpfsDirectoryControl;
   DeviceObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
     NpfsQueryInformation;
   DeviceObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
     NpfsSetInformation;
   DeviceObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = NpfsFlushBuffers;
   DeviceObject->MajorFunction[IRP_MJ_SHUTDOWN] = NpfsShutdown;
   DeviceObject->MajorFunction[IRP_MJ_CLEANUP] = NpfsCleanup;
   DeviceObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = 
     NpfsQuerySecurity;
   DeviceObject->MajorFunction[IRP_MJ_SET_SECURITY] =
     NpfsSetSecurity;
   DeviceObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
     NpfsFileSystemControl;
   
   DriverObject->DriverUnload = NULL;
   
   NpfsInitPipeList();
   
   return(STATUS_SUCCESS);
}


/* EOF */
