/* $Id: ioctrl.c,v 1.24 2004/08/15 16:39:03 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/ioctrl.c
 * PURPOSE:         Device IO control
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 *                  Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Filled in ZwDeviceIoControlFile 22/02/99
 *                  Fixed IO method handling 08/03/99
 *                  Added APC support 05/11/99
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
NtDeviceIoControlFile (IN HANDLE DeviceHandle,
		       IN HANDLE Event OPTIONAL,
		       IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
		       IN PVOID UserApcContext OPTIONAL,
		       OUT PIO_STATUS_BLOCK IoStatusBlock,
		       IN ULONG IoControlCode,
		       IN PVOID InputBuffer,
		       IN ULONG InputBufferLength OPTIONAL,
		       OUT PVOID OutputBuffer,
		       IN ULONG OutputBufferLength OPTIONAL)
{
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PDEVICE_OBJECT DeviceObject;
  PIRP Irp;
  PIO_STACK_LOCATION StackPtr;
  PKEVENT EventObject;
  KPROCESSOR_MODE PreviousMode;

  DPRINT("NtDeviceIoControlFile(DeviceHandle %x Event %x UserApcRoutine %x "
         "UserApcContext %x IoStatusBlock %x IoControlCode %x "
         "InputBuffer %x InputBufferLength %x OutputBuffer %x "
         "OutputBufferLength %x)\n",
         DeviceHandle,Event,UserApcRoutine,UserApcContext,IoStatusBlock,
         IoControlCode,InputBuffer,InputBufferLength,OutputBuffer,
         OutputBufferLength);

  if (IoStatusBlock == NULL)
    return STATUS_ACCESS_VIOLATION;

  PreviousMode = ExGetPreviousMode();

  Status = ObReferenceObjectByHandle (DeviceHandle,
				      FILE_READ_DATA | FILE_WRITE_DATA,
				      IoFileObjectType,
				      PreviousMode,
				      (PVOID *) &FileObject,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (Event != NULL)
    {
      Status = ObReferenceObjectByHandle (Event,
                                          SYNCHRONIZE,
                                          ExEventObjectType,
                                          PreviousMode,
                                          (PVOID*)&EventObject,
                                          NULL);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject (FileObject);
	  return Status;
	}
     }
   else
     {
       EventObject = &FileObject->Event;
       KeResetEvent (EventObject);
     }

  DeviceObject = FileObject->DeviceObject;

  Irp = IoBuildDeviceIoControlRequest (IoControlCode,
				       DeviceObject,
				       InputBuffer,
				       InputBufferLength,
				       OutputBuffer,
				       OutputBufferLength,
				       FALSE,
				       EventObject,
				       IoStatusBlock);

  /* Trigger FileObject/Event dereferencing */
  Irp->Tail.Overlay.OriginalFileObject = FileObject;

  Irp->RequestorMode = PreviousMode;
  Irp->Overlay.AsynchronousParameters.UserApcRoutine = UserApcRoutine;
  Irp->Overlay.AsynchronousParameters.UserApcContext = UserApcContext;

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->FileObject = FileObject;
  StackPtr->DeviceObject = DeviceObject;
  StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
  StackPtr->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;

  Status = IoCallDriver(DeviceObject,Irp);
  if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
    {
      Status = KeWaitForSingleObject (EventObject,
				      Executive,
				      PreviousMode,
				      FileObject->Flags & FO_ALERTABLE_IO,
				      NULL);
      if (Status != STATUS_WAIT_0)
	{
	  /* Wait failed. */
	  return Status;
	}

      Status = IoStatusBlock->Status;
    }

  return Status;
}

/* EOF */
