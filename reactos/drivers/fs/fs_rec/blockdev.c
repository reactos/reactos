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
/* $Id: blockdev.c,v 1.2 2002/05/15 18:02:59 ekohl Exp $
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
  PKEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  Event = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
  if (Event == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  KeInitializeEvent(Event,
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
				     Event,
				     &IoStatus);
  if (Irp == NULL)
    {
      DPRINT("IoBuildSynchronousFsdRequest failed\n");
      ExFreePool(Event);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  DPRINT("Calling IO Driver... with irp %x\n", Irp);
  Status = IoCallDriver(DeviceObject, Irp);
  if (Status == STATUS_PENDING)
    {
      DPRINT("Operation pending\n");
      KeWaitForSingleObject(Event, Suspended, KernelMode, FALSE, NULL);
      Status = IoStatus.Status;
    }

  ExFreePool(Event);

  return(STATUS_SUCCESS);
}


NTSTATUS
FsRecDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
		     IN ULONG ControlCode,
		     IN PVOID InputBuffer,
		     IN ULONG InputBufferSize,
		     IN OUT PVOID OutputBuffer,
		     IN OUT PULONG OutputBufferSize)
{
  ULONG BufferSize = 0;
  PKEVENT Event;
  PIRP Irp;
  IO_STATUS_BLOCK IoStatus;
  NTSTATUS Status;

  if (OutputBufferSize != NULL)
    {
      BufferSize = *OutputBufferSize;
    }

  Event = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
  if (Event == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  KeInitializeEvent(Event, NotificationEvent, FALSE);

  DPRINT("Building device I/O control request ...\n");
  Irp = IoBuildDeviceIoControlRequest(ControlCode,
				      DeviceObject,
				      InputBuffer,
				      InputBufferSize,
				      OutputBuffer,
				      BufferSize,
				      FALSE,
				      Event,
				      &IoStatus);
  if (Irp == NULL)
    {
      DPRINT("IoBuildDeviceIoControlRequest() failed\n");
      ExFreePool(Event);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  DPRINT("Calling IO Driver... with irp %x\n", Irp);
  Status = IoCallDriver(DeviceObject, Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(Event, Suspended, KernelMode, FALSE, NULL);
      Status = IoStatus.Status;
    }

  if (OutputBufferSize)
    {
      *OutputBufferSize = BufferSize;
    }

  ExFreePool(Event);

  return(Status);
}

/* EOF */
