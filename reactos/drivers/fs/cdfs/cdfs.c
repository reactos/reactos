/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003 ReactOS Team
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
/* $Id: cdfs.c,v 1.11 2003/11/17 02:12:49 hyperion Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/cdfs/cdfs.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 *                   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* GLOBALS *****************************************************************/

PCDFS_GLOBAL_DATA CdfsGlobalData;


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
  UNICODE_STRING DeviceName = ROS_STRING_INITIALIZER(L"\\Cdfs");

  DPRINT("CDFS 0.0.3\n");

  Status = IoCreateDevice(DriverObject,
			  sizeof(CDFS_GLOBAL_DATA),
			  &DeviceName,
			  FILE_DEVICE_CD_ROM_FILE_SYSTEM,
			  0,
			  FALSE,
			  &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /* Initialize global data */
  CdfsGlobalData = DeviceObject->DeviceExtension;
  RtlZeroMemory(CdfsGlobalData,
		sizeof(CDFS_GLOBAL_DATA));
  CdfsGlobalData->DriverObject = DriverObject;
  CdfsGlobalData->DeviceObject = DeviceObject;

  /* Initialize driver data */
  DeviceObject->Flags = DO_DIRECT_IO;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)CdfsClose;
  DriverObject->MajorFunction[IRP_MJ_CLEANUP] = (PDRIVER_DISPATCH)CdfsCleanup;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)CdfsCreate;
  DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH)CdfsRead;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH)CdfsWrite;
  DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
    (PDRIVER_DISPATCH)CdfsFileSystemControl;
  DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
    (PDRIVER_DISPATCH)CdfsDirectoryControl;
  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
    (PDRIVER_DISPATCH)CdfsQueryInformation;
  DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = 
    (PDRIVER_DISPATCH)CdfsSetInformation;
  DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
    (PDRIVER_DISPATCH)CdfsQueryVolumeInformation;
  DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] =
    (PDRIVER_DISPATCH)CdfsSetVolumeInformation;

  DriverObject->DriverUnload = NULL;

  IoRegisterFileSystem(DeviceObject);

  return(STATUS_SUCCESS);
}

