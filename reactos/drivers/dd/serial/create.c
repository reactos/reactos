/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/create.c
 * PURPOSE:         Serial IRP_MJ_CREATE operations
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */

#define NDEBUG
#include "serial.h"

NTSTATUS STDCALL
SerialCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PFILE_OBJECT FileObject;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;
	
	DPRINT("Serial: IRP_MJ_CREATE\n");
	Stack = IoGetCurrentIrpStackLocation(Irp);
	FileObject = Stack->FileObject;
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	if (Stack->Parameters.Create.Options & FILE_DIRECTORY_FILE)
	{
		CHECKPOINT;
		Status = STATUS_NOT_A_DIRECTORY;
		goto ByeBye;
	}
	
	if (FileObject->FileName.Length != 0 || 
		FileObject->RelatedFileObject != NULL)
	{
		CHECKPOINT;
		Status = STATUS_ACCESS_DENIED;
		goto ByeBye;
	}
	
	DPRINT("Serial: open COM%lu: successfull\n", DeviceExtension->ComPort);
	Status = STATUS_SUCCESS;
	
ByeBye:
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
