/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Device Functions
 * FILE:              subsys/win32k/eng/device.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

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

  Status = ObReferenceObjectByHandle(hDevice,
				     FILE_READ_DATA | FILE_WRITE_DATA,
				     IoFileObjectType,
				     KernelMode,
				     (PVOID *)&FileObject,
				     NULL);
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

  return (Status);
}
