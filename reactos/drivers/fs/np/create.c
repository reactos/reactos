/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/create.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <internal/debug.h>

#include "np.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS NpfsCreatePipe(PNPFS_DEVICE_EXTENSION DeviceExt,
			PFILE_OBJECT FileObject)
{
   PWSTR PipeName;
   PNPFS_FSCONTEXT PipeDescr;
   NTSTATUS Status;
   
   PipeName = wcsrchr(FileObject->FileName.Buffer, L'\\');
   if (PipeName == NULL)
     {
	PipeName = FileObject->FileName.Buffer;
     }
   
   PipeDescr = ExAllocatePool(NonPagedPool, sizeof(NPFS_FSCONTEXT));
   if (PipeDescr == NULL)
     {
	return(STATUS_OUT_OF_MEMORY);
     }
   
   Status = NpfsCreateEntry(PipeName, PipeDescr);
   if (!NT_SUCCESS(Status))
     {
	ExFreePool(PipeDescr);
	return(Status);
     }
   
   FileObject->FsContext = PipeDescr;
   
   return(Status);
}

NTSTATUS NpfsCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   
   Status = NpfsCreatePipe(DeviceExt, FileObject);
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}
