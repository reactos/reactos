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
/* $Id: arcname.c,v 1.7 2002/09/04 13:58:56 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/arcname.c
 * PURPOSE:         creates ARC names for boot devices
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */


/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include "internal/io.h"
#include "internal/xhal.h"

#define NDEBUG
#include <internal/debug.h>

/* MACROS *******************************************************************/

#define FS_VOLUME_BUFFER_SIZE (MAX_PATH + sizeof(FILE_FS_VOLUME_INFORMATION))

/* FUNCTIONS ****************************************************************/

NTSTATUS
IoCreateArcNames(VOID)
{
  PCONFIGURATION_INFORMATION ConfigInfo;
  PDRIVE_LAYOUT_INFORMATION LayoutInfo = NULL;
  WCHAR DeviceNameBuffer[80];
  WCHAR ArcNameBuffer[80];
  UNICODE_STRING DeviceName;
  UNICODE_STRING ArcName;
  ULONG i, j;
  NTSTATUS Status;

  DPRINT("IoCreateArcNames() called\n");

  ConfigInfo = IoGetConfigurationInformation();

  /* create ARC names for floppy drives */
  DPRINT("Floppy drives: %lu\n", ConfigInfo->FloppyCount);
  for (i = 0; i < ConfigInfo->FloppyCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\Floppy%lu",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      swprintf(ArcNameBuffer,
	       L"\\ArcName\\multi(0)disk(0)fdisk(%lu)",
	       i);
      RtlInitUnicodeString(&ArcName,
			   ArcNameBuffer);
      DPRINT("%wZ ==> %wZ\n",
	     &ArcName,
	     &DeviceName);

      Status = IoAssignArcName(&ArcName,
			       &DeviceName);
      if (!NT_SUCCESS(Status))
	return(Status);
    }

  /* create ARC names for hard disk drives */
  DPRINT("Disk drives: %lu\n", ConfigInfo->DiskCount);
  for (i = 0; i < ConfigInfo->DiskCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\Harddisk%lu\\Partition0",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      swprintf(ArcNameBuffer,
	       L"\\ArcName\\multi(0)disk(0)rdisk(%lu)partition(0)",
	       i);
      RtlInitUnicodeString(&ArcName,
			   ArcNameBuffer);
      DPRINT("%wZ ==> %wZ\n",
	     &ArcName,
	     &DeviceName);

      Status = IoAssignArcName(&ArcName,
			       &DeviceName);
      if (!NT_SUCCESS(Status))
	return(Status);

      Status = xHalQueryDriveLayout(&DeviceName,
				    &LayoutInfo);
      if (!NT_SUCCESS(Status))
	return(Status);

      DPRINT("Number of partitions: %u\n", LayoutInfo->PartitionCount);

      for (j = 0;j < LayoutInfo->PartitionCount; j++)
	{
	  swprintf(DeviceNameBuffer,
		   L"\\Device\\Harddisk%lu\\Partition%lu",
		   i,
		   j + 1);
	  RtlInitUnicodeString(&DeviceName,
			       DeviceNameBuffer);

	  swprintf(ArcNameBuffer,
		   L"\\ArcName\\multi(0)disk(0)rdisk(%lu)partition(%lu)",
		   i,
		   j + 1);
	  RtlInitUnicodeString(&ArcName,
			       ArcNameBuffer);
	  DPRINT("%wZ ==> %wZ\n",
		 &ArcName,
		 &DeviceName);

	  Status = IoAssignArcName(&ArcName,
				   &DeviceName);
	  if (!NT_SUCCESS(Status))
	    return(Status);
	}

      ExFreePool(LayoutInfo);
      LayoutInfo = NULL;
    }

  /* create ARC names for cdrom drives */
  DPRINT("CD-ROM drives: %lu\n", ConfigInfo->CDRomCount);
  for (i = 0; i < ConfigInfo->CDRomCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      swprintf(ArcNameBuffer,
	       L"\\ArcName\\multi(0)disk(0)cdrom(%lu)",
	       i);
      RtlInitUnicodeString(&ArcName,
			   ArcNameBuffer);
      DPRINT("%wZ ==> %wZ\n",
	     &ArcName,
	     &DeviceName);

      Status = IoAssignArcName(&ArcName,
			       &DeviceName);
      if (!NT_SUCCESS(Status))
	return(Status);
    }

  DPRINT("IoCreateArcNames() done\n");

  return(STATUS_SUCCESS);
}


static NTSTATUS
IopCheckCdromDevices(PULONG DeviceNumber)
{
  PFILE_FS_VOLUME_INFORMATION FileFsVolume;
  PCONFIGURATION_INFORMATION ConfigInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  WCHAR DeviceNameBuffer[32];
  HANDLE Handle;
  ULONG i;
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;
  USHORT Buffer[FS_VOLUME_BUFFER_SIZE];

  FileFsVolume = (PFILE_FS_VOLUME_INFORMATION)Buffer;

  ConfigInfo = IoGetConfigurationInformation();
  for (i = 0; i < ConfigInfo->CDRomCount; i++)
    {
      swprintf(DeviceNameBuffer,
	       L"\\Device\\CdRom%lu\\",
	       i);
      RtlInitUnicodeString(&DeviceName,
			   DeviceNameBuffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &DeviceName,
				 0,
				 NULL,
				 NULL);

      Status = NtOpenFile(&Handle,
			  FILE_ALL_ACCESS,
			  &ObjectAttributes,
			  NULL,
			  0,
			  0);
      DPRINT("NtOpenFile()  DeviceNumber %lu  Status %lx\n", i, Status);
      if (NT_SUCCESS(Status))
	{
	  Status = NtQueryVolumeInformationFile(Handle,
						&IoStatusBlock,
						FileFsVolume,
						FS_VOLUME_BUFFER_SIZE,
						FileFsVolumeInformation);
	  DPRINT("NtQueryVolumeInformationFile()  Status %lx\n", Status);
	  if (NT_SUCCESS(Status))
	    {
	      DPRINT("VolumeLabel: '%S'\n", FileFsVolume->VolumeLabel);
	      if (_wcsicmp(FileFsVolume->VolumeLabel, L"REACTOS") == 0)
		{
		  NtClose(Handle);
		  *DeviceNumber = i;
		  return(STATUS_SUCCESS);
		}
	    }
	  NtClose(Handle);
	}
    }

  *DeviceNumber = (ULONG)-1;

  return(STATUS_UNSUCCESSFUL);
}


NTSTATUS
IoCreateSystemRootLink(PCHAR ParameterLine)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING LinkName;
  UNICODE_STRING DeviceName;
  UNICODE_STRING ArcName;
  UNICODE_STRING BootPath;
  PCHAR ParamBuffer;
  PWCHAR ArcNameBuffer;
  PCHAR p;
  NTSTATUS Status;
  ULONG Length;
  HANDLE Handle;

  /* Create local parameter line copy */
  ParamBuffer = ExAllocatePool(PagedPool, 256);
  strcpy(ParamBuffer, (char *)ParameterLine);

  DPRINT("%s\n", ParamBuffer);
  /* Format: <arc_name>\<path> [options...] */

  /* cut options off */
  p = strchr(ParamBuffer, ' ');
  if (p)
    *p = 0;
  DPRINT("%s\n", ParamBuffer);

  /* extract path */
  p = strchr(ParamBuffer, '\\');
  if (p)
    {
      DPRINT("Boot path: %s\n", p);
      RtlCreateUnicodeStringFromAsciiz(&BootPath, p);
      *p = 0;
    }
  else
    {
      DPRINT("Boot path: %s\n", "\\");
      RtlCreateUnicodeStringFromAsciiz(&BootPath, "\\");
    }
  DPRINT("ARC name: %s\n", ParamBuffer);

  p = strstr(ParamBuffer, "cdrom");
  if (p != NULL)
    {
      ULONG DeviceNumber;

      DPRINT("Booting from CD-ROM!\n");
      Status = IopCheckCdromDevices(&DeviceNumber);
      if (!NT_SUCCESS(Status))
	{
	  CPRINT("Failed to find setup disk!\n");
	  return(Status);
	}

      sprintf(p, "cdrom(%lu)", DeviceNumber);

      DPRINT("New ARC name: %s\n", ParamBuffer);

      /* Adjust original command line */
      p = strstr(ParameterLine, "cdrom");
      if (p != NULL);
	{
	  char temp[256];
	  char *q;

	  q = strchr(p, ')');
	  if (q != NULL)
	    {

	      q++;
	      strcpy(temp, q);
	      sprintf(p, "cdrom(%lu)", DeviceNumber);
	      strcat(p, temp);
	    }
	}
    }

  /* Only arc name left - build full arc name */
  ArcNameBuffer = ExAllocatePool(PagedPool, 256 * sizeof(WCHAR));
  swprintf(ArcNameBuffer,
	   L"\\ArcName\\%S", ParamBuffer);
  RtlInitUnicodeString(&ArcName, ArcNameBuffer);
  DPRINT("Arc name: %wZ\n", &ArcName);

  /* free ParamBuffer */
  ExFreePool(ParamBuffer);

  /* allocate device name string */
  DeviceName.Length = 0;
  DeviceName.MaximumLength = 256 * sizeof(WCHAR);
  DeviceName.Buffer = ExAllocatePool(PagedPool, 256 * sizeof(WCHAR));

  InitializeObjectAttributes(&ObjectAttributes,
			     &ArcName,
			     OBJ_OPENLINK,
			     NULL,
			     NULL);

  Status = NtOpenSymbolicLinkObject(&Handle,
				    SYMBOLIC_LINK_ALL_ACCESS,
				    &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&BootPath);
      RtlFreeUnicodeString(&DeviceName);
      CPRINT("NtOpenSymbolicLinkObject() '%wZ' failed (Status %x)\n",
	     &ArcName,
	     Status);
      RtlFreeUnicodeString(&ArcName);

      return(Status);
    }
  RtlFreeUnicodeString(&ArcName);

  Status = NtQuerySymbolicLinkObject(Handle,
				     &DeviceName,
				     &Length);
  NtClose (Handle);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&BootPath);
      RtlFreeUnicodeString(&DeviceName);
      CPRINT("NtQuerySymbolicObject() failed (Status %x)\n",
	     Status);

      return(Status);
    }
  DPRINT("Length: %lu DeviceName: %wZ\n", Length, &DeviceName);

  RtlAppendUnicodeStringToString(&DeviceName,
				 &BootPath);

  RtlFreeUnicodeString(&BootPath);
  DPRINT("DeviceName: %wZ\n", &DeviceName);

  /* create the '\SystemRoot' link */
  RtlInitUnicodeStringFromLiteral(&LinkName,
		       L"\\SystemRoot");

  Status = IoCreateSymbolicLink(&LinkName,
				&DeviceName);
  RtlFreeUnicodeString (&DeviceName);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCreateSymbolicLink() failed (Status %x)\n",
	     Status);

      return(Status);
    }

  /* Check whether '\SystemRoot'(LinkName) can be opened */
  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     0,
			     NULL,
			     NULL);

  Status = NtOpenFile(&Handle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      NULL,
		      0,
		      0);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("NtOpenFile() failed to open '\\SystemRoot' (Status %x)\n",
	     Status);
      return(Status);
    }

  NtClose(Handle);

  return(STATUS_SUCCESS);
}

/* EOF */
