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
/* $Id: ntfs.c,v 1.2 2002/08/20 20:37:06 hyperion Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/ntfs.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "ntfs.h"


/* GLOBALS *****************************************************************/

PNTFS_GLOBAL_DATA NtfsGlobalData;


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
  UNICODE_STRING DeviceName = UNICODE_STRING_INITIALIZER(L"\\Ntfs");

  DPRINT("NTFS 0.0.1\n");

  Status = IoCreateDevice(DriverObject,
			  sizeof(NTFS_GLOBAL_DATA),
			  &DeviceName,
			  FILE_DEVICE_DISK_FILE_SYSTEM,
			  0,
			  FALSE,
			  &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /* Initialize global data */
  NtfsGlobalData = DeviceObject->DeviceExtension;
  RtlZeroMemory(NtfsGlobalData,
		sizeof(NTFS_GLOBAL_DATA));
  NtfsGlobalData->DriverObject = DriverObject;
  NtfsGlobalData->DeviceObject = DeviceObject;

  /* Initialize driver data */
  DeviceObject->Flags = DO_DIRECT_IO;
//  DriverObject->MajorFunction[IRP_MJ_CLOSE] = NtfsClose;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = NtfsCreate;
//  DriverObject->MajorFunction[IRP_MJ_READ] = NtfsRead;
//  DriverObject->MajorFunction[IRP_MJ_WRITE] = NtfsWrite;
  DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
    NtfsFileSystemControl;
  DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
    NtfsDirectoryControl;
  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
    NtfsQueryInformation;
//  DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
//    NtfsQueryVolumeInformation;
//  DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] =
//    NtfsSetVolumeInformation;

  DriverObject->DriverUnload = NULL;

  IoRegisterFileSystem(DeviceObject);

  return(STATUS_SUCCESS);
}

