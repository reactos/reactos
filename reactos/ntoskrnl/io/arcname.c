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
/* $Id: arcname.c,v 1.1 2002/03/13 01:27:06 ekohl Exp $
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
#include <debug.h>


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

/* EOF */