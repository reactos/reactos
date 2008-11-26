/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Device Functions
 * FILE:              subsys/win32k/eng/device.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
DWORD STDCALL
EngDeviceIoControl(HANDLE  hDevice,
		   DWORD   dwIoControlCode,
		   LPVOID  lpInBuffer,
		   DWORD   nInBufferSize,
		   LPVOID  lpOutBuffer,
		   DWORD   nOutBufferSize,
		   DWORD *lpBytesReturned)
{
  PIRP Irp;
  NTSTATUS Status;
  KEVENT Event;
  IO_STATUS_BLOCK Iosb;
  PDEVICE_OBJECT DeviceObject;

  DPRINT("EngDeviceIoControl() called\n");

  KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

  DeviceObject = (PDEVICE_OBJECT) hDevice;

  Irp = IoBuildDeviceIoControlRequest(dwIoControlCode,
				      DeviceObject,
				      lpInBuffer,
				      nInBufferSize,
				      lpOutBuffer,
				      nOutBufferSize, FALSE, &Event, &Iosb);
  if (!Irp) return ERROR_NOT_ENOUGH_MEMORY;

  Status = IoCallDriver(DeviceObject, Irp);

  if (Status == STATUS_PENDING)
    {
      (VOID)KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
      Status = Iosb.Status;
    }

  DPRINT("EngDeviceIoControl(): Returning %X/%X\n", Iosb.Status,
	 Iosb.Information);

  /* Return information to the caller about the operation. */
  *lpBytesReturned = Iosb.Information;

  /* Convert NT status values to win32 error codes. */
  switch (Status)
    {
    case STATUS_INSUFFICIENT_RESOURCES: return ERROR_NOT_ENOUGH_MEMORY;
    case STATUS_BUFFER_OVERFLOW: return ERROR_MORE_DATA;
    case STATUS_NOT_IMPLEMENTED: return ERROR_INVALID_FUNCTION;
    case STATUS_INVALID_PARAMETER: return ERROR_INVALID_PARAMETER;
    case STATUS_BUFFER_TOO_SMALL: return ERROR_INSUFFICIENT_BUFFER;
    case STATUS_DEVICE_DOES_NOT_EXIST: return ERROR_DEV_NOT_EXIST;
    case STATUS_PENDING: return ERROR_IO_PENDING;
    }
  return Status;
}

/* EOF */
