/* $Id: mount.c,v 1.2 1999/12/04 20:58:42 ea Exp $
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

NTSTATUS NpfsMount(PDEVICE_OBJECT DeviceToMount)
{
   NTSTATUS Status;
   PDEVICE_OBJECT DeviceObject;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   
   Status = IoCreateDevice(DriverObject,
			   sizeof(NPFS_DEVICE_EXTENSION),
			   NULL,
			   FILE_DEVICE_FILE_SYSTEM,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   
   DeviceExt->StorageDevice = IoAttachDeviceToDeviceStack(DeviceObject,
							  DeviceToMount);
   
   return(STATUS_SUCCESS);
}

NTSTATUS NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
			       PIRP Irp)
{
   PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
   PVPB Vpb = IoStack->Parameters.Mount.Vpb;
   PDEVICE_OBJECT DeviceToMount = IoStack->Parameters.Mount.DeviceObject;
   NTSTATUS Status;
   
   Status = NpfsMount(DeviceToMount);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
   
}

NTSTATUS DriverEntry(PDRIVER_OBJECT _DriverObject,
		     PUNICODE_STRING RegistryPath)
{
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS Status;
   UNICODE_STRING DeviceName;
   
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
   
   DeviceObject->Flags = 0;
   DeviceObject->MajorFunction[IRP_MJ_CLOSE] = NpfsClose;
   DeviceObject->MajorFunction[IRP_MJ_CREATE] = NpfsCreate;
   DeviceObject->MajorFunction[IRP_MJ_READ] = NpfsRead;
   DeviceObject->MajorFunction[IRP_MJ_WRITE] = NpfsWrite;
   DeviceObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
     NpfsFileSystemControl;
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
   DeviceObject->MajorFunction[IRP_MJ_QUERY_QUOTA] =
     NpfsQueryQuota;
   DeviceObject->MajorFunction[IRP_MJ_SET_QUOTA] =
     NpfsSetQuota;
   
   DriverObject->DriverUnload = NULL;
   
   IoRegisterFileSystem(DeviceObject);
   
   return(STATUS_SUCCESS);
}


/* EOF */
