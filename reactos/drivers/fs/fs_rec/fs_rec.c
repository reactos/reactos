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
/* $Id: fs_rec.c,v 1.2 2002/05/15 18:02:59 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/fs_rec/fs_rec.c
 * PURPOSE:          Filesystem recognizer driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "fs_rec.h"


/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
FsRecCreate(IN PDEVICE_OBJECT DeviceObject,
	    IN PIRP Irp)
{
  NTSTATUS Status;


  Status = STATUS_SUCCESS;


  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(Status);
}


NTSTATUS STDCALL
FsRecClose(IN PDEVICE_OBJECT DeviceObject,
	   IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
FsRecFsControl(IN PDEVICE_OBJECT DeviceObject,
	       IN PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExt;
  NTSTATUS Status;

  DeviceExt = DeviceObject->DeviceExtension;
  switch (DeviceExt->FsType)
    {
      case FS_TYPE_VFAT:
	Status = FsRecVfatFsControl(DeviceObject, Irp);
	break;

      case FS_TYPE_CDFS:
	Status = FsRecCdfsFsControl(DeviceObject, Irp);
	break;

      case FS_TYPE_NTFS:
	Status = FsRecNtfsFsControl(DeviceObject, Irp);
	break;

      default:
	Status = STATUS_INVALID_DEVICE_REQUEST;
    }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(Status);
}


VOID STDCALL
FsRecUnload(IN PDRIVER_OBJECT DriverObject)
{
  PDEVICE_OBJECT NextDevice;
  PDEVICE_OBJECT ThisDevice;

  /* Delete all remaining device objects */
  NextDevice = DriverObject->DeviceObject;
  while (NextDevice != NULL)
    {
      ThisDevice = NextDevice;
      NextDevice = NextDevice->NextDevice;
      IoDeleteDevice(ThisDevice);
    }
}


static NTSTATUS
FsRecRegisterFs(PDRIVER_OBJECT DriverObject,
		PWSTR FsName,
		PWSTR RecognizerName,
		ULONG DeviceType,
		ULONG FsType)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatus;
  PDEVICE_EXTENSION DeviceExt;
  UNICODE_STRING DeviceName;
  UNICODE_STRING FileName;
  PDEVICE_OBJECT DeviceObject;
  HANDLE FileHandle;
  NTSTATUS Status;

  RtlInitUnicodeString(&FileName,
		       FsName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE,
			     0,
			     NULL);

  Status = ZwCreateFile(&FileHandle,
			0x100000,
			&ObjectAttributes,
			&IoStatus,
			NULL,
			0,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			OPEN_EXISTING,
			0,
			NULL,
			0);
  if (NT_SUCCESS(Status))
    {
      ZwClose(FileHandle);
      return(STATUS_IMAGE_ALREADY_LOADED);
    }

  /* Create recognizer device object */
  RtlInitUnicodeString(&DeviceName,
		       RecognizerName);

  Status = IoCreateDevice(DriverObject,
			  sizeof(DEVICE_EXTENSION),
			  &DeviceName,
			  DeviceType,
			  0,
			  FALSE,
			  &DeviceObject);

  if (NT_SUCCESS(Status))
    {
      DeviceExt = DeviceObject->DeviceExtension;
      DeviceExt->FsType = FsType;
      IoRegisterFileSystem(DeviceObject);
      DPRINT("Created recognizer device '%wZ'\n", &DeviceName);
    }

  return(Status);
}


NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
{
  PCONFIGURATION_INFORMATION ConfigInfo;
  ULONG DeviceCount;
  NTSTATUS Status;

  DPRINT("FileSystem recognizer 0.0.1\n");

  DeviceCount = 0;

  DriverObject->MajorFunction[IRP_MJ_CREATE] = FsRecCreate;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsRecClose;
  DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FsRecClose;
  DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = FsRecFsControl;
  DriverObject->DriverUnload = FsRecUnload;

  ConfigInfo = IoGetConfigurationInformation();

  if (ConfigInfo->CDRomCount > 0)
    {
      Status = FsRecRegisterFs(DriverObject,
			       L"\\Cdfs",
			       L"\\FileSystem\\CdfsRecognizer",
			       FILE_DEVICE_CD_ROM_FILE_SYSTEM,
			       FS_TYPE_CDFS);
      if (NT_SUCCESS(Status))
	{
	  DeviceCount++;
	}
    }

  Status = FsRecRegisterFs(DriverObject,
			   L"\\Fat",
			   L"\\FileSystem\\FatRecognizer",
			   FILE_DEVICE_DISK_FILE_SYSTEM,
			   FS_TYPE_VFAT);
  if (NT_SUCCESS(Status))
    {
      DeviceCount++;
    }

  Status = FsRecRegisterFs(DriverObject,
			   L"\\Ntfs",
			   L"\\FileSystem\\NtfsRecognizer",
			   FILE_DEVICE_DISK_FILE_SYSTEM,
			   FS_TYPE_NTFS);
  if (NT_SUCCESS(Status))
    {
      DeviceCount++;
    }

  return((DeviceCount > 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

/* EOF */
