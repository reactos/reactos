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
/* $Id: blockdev.c,v 1.1 2002/05/15 09:40:47 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/fs_rec/blockdev.c
 * PURPOSE:          Filesystem recognizer driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "fs_rec.h"


/* FUNCTIONS ****************************************************************/

NTSTATUS
FsRecReadSectors(IN PDEVICE_OBJECT DeviceObject,
		 IN ULONG DiskSector,
		 IN ULONG SectorCount,
		 IN ULONG SectorSize,
		 IN OUT PUCHAR Buffer)
{
  PIO_STACK_LOCATION Stack;
  IO_STATUS_BLOCK IoStatus;
  LARGE_INTEGER Offset;
  ULONG BlockSize;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  KeInitializeEvent(&Event,
		    NotificationEvent,
		    FALSE);

  Offset.QuadPart = (LONGLONG)DiskSector * (LONGLONG)SectorSize;
  BlockSize = SectorCount * SectorSize;

  DPRINT("FsrecReadSectors(DeviceObject %x, DiskSector %d, Buffer %x)\n",
	 DeviceObject, DiskSector, Buffer);
  DPRINT("Offset %I64x BlockSize %ld\n",
	 Offset.QuadPart,
	 BlockSize);

  DPRINT("Building synchronous FSD Request...\n");
  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				     DeviceObject,
				     Buffer,
				     BlockSize,
				     &Offset,
				     &Event,
				     &IoStatus);
  if (Irp == NULL)
    {
      DPRINT("IoBuildSynchronousFsdRequest failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  DPRINT("Calling IO Driver... with irp %x\n", Irp);
  Status = IoCallDriver(DeviceObject, Irp);

  DPRINT("Waiting for IO Operation for %x\n", Irp);
  if (Status == STATUS_PENDING)
    {
      DPRINT("Operation pending\n");
      KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
      DPRINT("Getting IO Status... for %x\n", Irp);
      Status = IoStatus.Status;
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT("FsrecReadSectors() failed (Status %x)\n", Status);
      DPRINT("(DeviceObject %x, DiskSector %x, Buffer %x, Offset 0x%I64x)\n",
	     DeviceObject, DiskSector, Buffer,
	     Offset.QuadPart);
      return(Status);
    }

  DPRINT("Block request succeeded for %x\n", Irp);

  return(STATUS_SUCCESS);
}

/* EOF */
