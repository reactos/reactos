/* $Id: ioctrl.c,v 1.8 1999/11/07 14:07:35 ekohl Exp $
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

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

ULONG IoGetFunctionCodeFromCtlCode(ULONG ControlCode)
{
   UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtDeviceIoControlFile (
	IN	HANDLE			DeviceHandle,
	IN	HANDLE			Event		OPTIONAL,
	IN	PIO_APC_ROUTINE		UserApcRoutine,
	IN	PVOID			UserApcContext	OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG			IoControlCode,
	IN	PVOID			InputBuffer,
	IN	ULONG			InputBufferSize,
	OUT	PVOID			OutputBuffer,
	IN	ULONG			OutputBufferSize
	)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT DeviceObject;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   KEVENT KEvent;

   assert_irql(PASSIVE_LEVEL);
   
   DPRINT("NtDeviceIoControlFile(DeviceHandle %x Event %x UserApcRoutine %x "
          "UserApcContext %x IoStatusBlock %x IoControlCode %x "
          "InputBuffer %x InputBufferSize %x OutputBuffer %x "
          "OutputBufferSize %x)\n",
          DeviceHandle,Event,UserApcRoutine,UserApcContext,IoStatusBlock,
          IoControlCode,InputBuffer,InputBufferSize,OutputBuffer,
          OutputBufferSize);

   Status = ObReferenceObjectByHandle(DeviceHandle,
				      FILE_READ_DATA | FILE_WRITE_DATA,
				      NULL,
				      KernelMode,
				      (PVOID *) &FileObject,
				      NULL);

   if (Status != STATUS_SUCCESS)
   {
      return(Status);
   }

   DeviceObject = FileObject->DeviceObject;
   assert(DeviceObject != NULL);

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
