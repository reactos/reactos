/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: mup.c,v 1.1 2002/06/25 22:21:41 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/mup/mup.c
 * PURPOSE:          Multi UNC Provider
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "mup.h"


/* GLOBALS *****************************************************************/


/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
  PDEVICE_OBJECT DeviceObject;
  NTSTATUS Status;
  UNICODE_STRING DeviceName;

  DPRINT("MUP 0.0.1\n");

  RtlInitUnicodeString(&DeviceName,
		       L"\\Device\\Mup");
  Status = IoCreateDevice(DriverObject,
			  sizeof(DEVICE_EXTENSION),
			  &DeviceName,
			  FILE_DEVICE_MULTI_UNC_PROVIDER,
			  0,
			  FALSE,
			  &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /* Initialize driver data */
  DeviceObject->Flags = DO_DIRECT_IO;
//  DriverObject->MajorFunction[IRP_MJ_CLOSE] = NtfsClose;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = MupCreate;
  DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = MupCreate;
  DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = MupCreate;
//  DriverObject->MajorFunction[IRP_MJ_READ] = NtfsRead;
//  DriverObject->MajorFunction[IRP_MJ_WRITE] = NtfsWrite;
//  DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
//    NtfsFileSystemControl;
//  DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
//    NtfsDirectoryControl;
//  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
//    NtfsQueryInformation;
//  DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
//    NtfsQueryVolumeInformation;
//  DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] =
//    NtfsSetVolumeInformation;

  DriverObject->DriverUnload = NULL;


  /* Initialize global data */
//  DeviceExtensionNtfsGlobalData = DeviceObject->DeviceExtension;
//  RtlZeroMemory(NtfsGlobalData,
//		sizeof(NTFS_GLOBAL_DATA));
//  NtfsGlobalData->DriverObject = DriverObject;
//  NtfsGlobalData->DeviceObject = DeviceObject;

  return(STATUS_SUCCESS);
}

