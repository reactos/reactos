/* $Id: ioctrl.c,v 1.12 2001/03/21 23:27:18 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/ioctrl.c
 * PURPOSE:         Device IO control
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Filled in ZwDeviceIoControlFile 22/02/99
 *                  Fixed IO method handling 08/03/99
 *                  Added APC support 05/11/99
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtDeviceIoControlFile (IN HANDLE DeviceHandle,
					IN HANDLE Event,
					IN PIO_APC_ROUTINE UserApcRoutine,
					IN PVOID UserApcContext,
					OUT PIO_STATUS_BLOCK IoStatusBlock,
					IN ULONG IoControlCode,
					IN PVOID InputBuffer,
					IN ULONG InputBufferSize,
					OUT PVOID OutputBuffer,
					IN ULONG OutputBufferSize)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   KEVENT KEvent;

   DPRINT("NtDeviceIoControlFile(DeviceHandle %x Event %x UserApcRoutine %x "
          "UserApcContext %x IoStatusBlock %x IoControlCode %x "
          "InputBuffer %x InputBufferSize %x OutputBuffer %x "
          "OutputBufferSize %x)\n",
          DeviceHandle,Event,UserApcRoutine,UserApcContext,IoStatusBlock,
          IoControlCode,InputBuffer,InputBufferSize,OutputBuffer,
          OutputBufferSize);

   Status = ObReferenceObjectByHandle(DeviceHandle,
				      FILE_READ_DATA | FILE_WRITE_DATA,
				      IoFileObjectType,
				      KernelMode,
				      (PVOID *) &FileObject,
				      NULL);
   
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   DeviceObject = FileObject->DeviceObject;

   KeInitializeEvent(&KEvent,NotificationEvent,TRUE);

   Irp = IoBuildDeviceIoControlRequest(IoControlCode,
				       DeviceObject,
				       InputBuffer,
				       InputBufferSize,
				       OutputBuffer,
				       OutputBufferSize,
				       FALSE,
				       &KEvent,
				       IoStatusBlock);

   Irp->Overlay.AsynchronousParameters.UserApcRoutine = UserApcRoutine;
   Irp->Overlay.AsynchronousParameters.UserApcContext = UserApcContext;

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   StackPtr->DeviceObject = DeviceObject;
   StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferSize;
   StackPtr->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferSize;

   Status = IoCallDriver(DeviceObject,Irp);
   if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
   {
      KeWaitForSingleObject(&KEvent,Executive,KernelMode,FALSE,NULL);
      return(IoStatusBlock->Status);
   }
   return(Status);
}

/* EOF */
