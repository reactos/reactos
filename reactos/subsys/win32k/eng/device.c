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

DWORD APIENTRY EngDeviceIoControl(
   HANDLE  hDevice,
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
   PDEVICE_OBJECT  theDevice;

   ObReferenceObjectByHandle(hDevice, GENERIC_ALL, NULL, UserMode,
                            (PVOID *)&theDevice, NULL);

   KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

   Irp = IoBuildDeviceIoControlRequest(dwIoControlCode,
                                       theDevice,
                                       lpInBuffer,
                                       nInBufferSize,
                                       lpOutBuffer,
                                       nOutBufferSize,
                                       FALSE,
                                       &Event,
                                       &Iosb);

   Status = IoCallDriver(theDevice, Irp);

   if (Status == STATUS_PENDING)
   {
     (void) KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);
   }

   return (Status);
}
