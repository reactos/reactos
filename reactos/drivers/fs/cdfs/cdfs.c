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
/* $Id: cdfs.c,v 1.3 2002/05/14 23:16:23 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/cdfs.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

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
  UNICODE_STRING DeviceName;

  DbgPrint("CDFS 0.0.2\n");

  RtlInitUnicodeString(&DeviceName,
		       L"\\Device\\cdfs");
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
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = CdfsClose;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = CdfsCreate;
  DriverObject->MajorFunction[IRP_MJ_READ] = CdfsRead;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = CdfsWrite;
  DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
    CdfsFileSystemControl;
  DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
    CdfsDirectoryControl;
  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
    CdfsQueryInformation;
  DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
    CdfsQueryVolumeInformation;
  DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] =
    CdfsSetVolumeInformation;

  DriverObject->DriverUnload = NULL;

  IoRegisterFileSystem(DeviceObject);

  return(STATUS_SUCCESS);
}

