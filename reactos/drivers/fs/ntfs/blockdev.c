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
/* $Id: blockdev.c,v 1.1 2002/06/25 22:23:05 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/blockdev.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "ntfs.h"


/* FUNCTIONS ****************************************************************/

NTSTATUS
NtfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
		IN ULONG DiskSector,
		IN ULONG SectorCount,
		IN ULONG SectorSize,
		IN OUT PUCHAR Buffer)
{
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

  DPRINT("NtfsReadSectors(DeviceObject %x, DiskSector %d, Buffer %x)\n",
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
      if (Status == STATUS_VERIFY_REQUIRED)
        {
	  PDEVICE_OBJECT DeviceToVerify;
	  NTSTATUS NewStatus;

	  DPRINT1("STATUS_VERIFY_REQUIRED\n");
	  DeviceToVerify = IoGetDeviceToVerify(PsGetCurrentThread());
	  IoSetDeviceToVerify(PsGetCurrentThread(), NULL);

	  NewStatus = IoVerifyVolume(DeviceToVerify, FALSE);
	  DPRINT1("IoVerifyVolume() retuned (Status %lx)\n", NewStatus);
        }

      DPRINT("NtfsReadSectors() failed (Status %x)\n", Status);
      DPRINT("(DeviceObject %x, DiskSector %x, Buffer %x, Offset 0x%I64x)\n",
	     DeviceObject, DiskSector, Buffer,
	     Offset.QuadPart);
      return(Status);
    }

  DPRINT("Block request succeeded for %x\n", Irp);

  return(STATUS_SUCCESS);
}


NTSTATUS
NtfsReadRawSectors(IN PDEVICE_OBJECT DeviceObject,
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

  DPRINT("NtfsReadSectors(DeviceObject %x, DiskSector %d, Buffer %x)\n",
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

//  Stack = IoGetCurrentIrpStackLocation(Irp);
//  Stack->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

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
      DPRINT("NtfsReadSectors() failed (Status %x)\n", Status);
      DPRINT("(DeviceObject %x, DiskSector %x, Buffer %x, Offset 0x%I64x)\n",
	     DeviceObject, DiskSector, Buffer,
	     Offset.QuadPart);
      return(Status);
    }

  DPRINT("Block request succeeded for %x\n", Irp);

  return(STATUS_SUCCESS);
}


NTSTATUS
NtfsDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
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
