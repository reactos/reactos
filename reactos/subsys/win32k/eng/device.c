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
/* $Id: device.c,v 1.9 2003/07/11 15:59:37 royce Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Device Functions
 * FILE:              subsys/win32k/eng/device.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/ntddk.h>
#include <win32k/misc.h>

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
  PFILE_OBJECT FileObject;

  DPRINT("EngDeviceIoControl() called\n");

  KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

  /* Switch to process context in which hDevice is valid */
  KeAttachProcess(W32kDeviceProcess);

  Status = ObReferenceObjectByHandle(hDevice,
				     FILE_READ_DATA | FILE_WRITE_DATA,
				     IoFileObjectType,
				     KernelMode,
				     (PVOID *)&FileObject,
				     NULL);
  KeDetachProcess();

  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Irp = IoBuildDeviceIoControlRequest(dwIoControlCode,
				      FileObject->DeviceObject,
				      lpInBuffer,
				      nInBufferSize,
				      lpOutBuffer,
				      nOutBufferSize, FALSE, &Event, &Iosb);

  Status = IoCallDriver(FileObject->DeviceObject, Irp);

  if (Status == STATUS_PENDING)
  {
    (void) KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
  }

  ObDereferenceObject(FileObject);

  return (Status);
}
/* EOF */
