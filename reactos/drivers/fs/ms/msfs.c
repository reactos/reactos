/* $Id: msfs.c,v 1.5 2002/09/08 10:22:10 chorns Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/ms/msfs.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "msfs.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
{
   PMSFS_DEVICE_EXTENSION DeviceExtension;
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName;
   NTSTATUS Status;
   
   DbgPrint("Mailslot FSD 0.0.1\n");
   
   DriverObject->Flags = 0;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = MsfsCreate;
   DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] =
     MsfsCreateMailslot;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = MsfsClose;
   DriverObject->MajorFunction[IRP_MJ_READ] = MsfsRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = MsfsWrite;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
     MsfsQueryInformation;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
     MsfsSetInformation;
//   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
//     MsfsDirectoryControl;
//   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = MsfsFlushBuffers;
//   DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = MsfsShutdown;
//   DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = 
//     MsfsQuerySecurity;
//   DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] =
//     MsfsSetSecurity;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
     MsfsFileSystemControl;
   
   DriverObject->DriverUnload = NULL;
   
   RtlInitUnicodeString(&DeviceName,
			L"\\Device\\MailSlot");
   Status = IoCreateDevice(DriverObject,
			   sizeof(MSFS_DEVICE_EXTENSION),
			   &DeviceName,
			   FILE_DEVICE_MAILSLOT,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   /* initialize device extension */
   DeviceExtension = DeviceObject->DeviceExtension;
   InitializeListHead(&DeviceExtension->MailslotListHead);
   KeInitializeMutex(&DeviceExtension->MailslotListLock,
		     0);
   
   return(STATUS_SUCCESS);
}

/* EOF */
