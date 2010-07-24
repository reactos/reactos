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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/blockdev.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/


/* FUNCTIONS ****************************************************************/

NTSTATUS
NtfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
		IN ULONG DiskSector,
		IN ULONG SectorCount,
		IN ULONG SectorSize,
		IN OUT PUCHAR Buffer,
		IN BOOLEAN Override)
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

  DPRINT("NtfsReadSectors(DeviceObject %p, DiskSector %d, Buffer %p)\n",
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
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  if (Override)
    {
      Stack = IoGetNextIrpStackLocation(Irp);
      Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

  DPRINT("Calling IO Driver... with irp %p\n", Irp);
  Status = IoCallDriver(DeviceObject, Irp);

  DPRINT("Waiting for IO Operation for %p\n", Irp);
  if (Status == STATUS_PENDING)
    {
      DPRINT("Operation pending\n");
      KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
      DPRINT("Getting IO Status... for %p\n", Irp);
      Status = IoStatus.Status;
    }

  DPRINT("NtfsReadSectors() done (Status %x)\n", Status);

  return Status;
}


NTSTATUS
NtfsDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
		    IN ULONG ControlCode,
		    IN PVOID InputBuffer,
		    IN ULONG InputBufferSize,
		    IN OUT PVOID OutputBuffer,
		    IN OUT PULONG OutputBufferSize,
		    IN BOOLEAN Override)
{
  PIO_STACK_LOCATION Stack;
  IO_STATUS_BLOCK IoStatus;
  KEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  KeInitializeEvent(&Event, NotificationEvent, FALSE);

  DPRINT("Building device I/O control request ...\n");
  Irp = IoBuildDeviceIoControlRequest(ControlCode,
				      DeviceObject,
				      InputBuffer,
				      InputBufferSize,
				      OutputBuffer,
				      (OutputBufferSize) ? *OutputBufferSize : 0,
				      FALSE,
				      &Event,
				      &IoStatus);
  if (Irp == NULL)
    {
      DPRINT("IoBuildDeviceIoControlRequest() failed\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  if (Override)
    {
      Stack = IoGetNextIrpStackLocation(Irp);
      Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

  DPRINT("Calling IO Driver... with irp %p\n", Irp);
  Status = IoCallDriver(DeviceObject, Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
      Status = IoStatus.Status;
    }

  if (OutputBufferSize)
    {
      *OutputBufferSize = IoStatus.Information;
    }

  return Status;
}

/* EOF */
