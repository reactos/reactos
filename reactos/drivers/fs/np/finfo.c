/* $Id: finfo.c,v 1.1 2001/06/12 12:35:04 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/finfo.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "npfs.h"

//#define NDEBUG
#include <debug.h>




/* FUNCTIONS *****************************************************************/


NTSTATUS STDCALL
NpfsQueryInformation(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   FILE_INFORMATION_CLASS FileInformationClass;
   PFILE_OBJECT FileObject;
   PNPFS_DEVICE_EXTENSION DeviceExtension;
   PNPFS_FCB Fcb;
   PNPFS_PIPE Pipe;
   PVOID SystemBuffer;
   ULONG BufferLength;
   NTSTATUS Status;
   
   DPRINT("NpfsQueryInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);
   
   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   DeviceExtension = DeviceObject->DeviceExtension;
   FileObject = IoStack->FileObject;
   Fcb = (PNPFS_FCB)FileObject->FsContext;
   Pipe = Fcb->Pipe;
   
   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
   BufferLength = IoStack->Parameters.QueryFile.Length;
   
   DPRINT("Pipe name: %wZ\n", &Pipe->PipeName);
   DPRINT("FileInformationClass %d\n", FileInformationClass);
   DPRINT("SystemBuffer %x\n", SystemBuffer);
   DPRINT("BufferLength %lu\n", BufferLength);
   
   switch (FileInformationClass)
     {
     case FilePipeInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
     case FilePipeLocalInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
     case FilePipeRemoteInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
     default:
	Status = STATUS_NOT_SUPPORTED;
     }
   
   Irp->IoStatus.Status = Status;
   if (NT_SUCCESS(Status))
     Irp->IoStatus.Information =
       IoStack->Parameters.QueryFile.Length - BufferLength;
   else
     Irp->IoStatus.Information = 0;
   IoCompleteRequest (Irp, IO_NO_INCREMENT);
   
   return(Status);
}


NTSTATUS STDCALL
NpfsSetInformation(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   FILE_INFORMATION_CLASS FileInformationClass;
   PFILE_OBJECT FileObject;
   PNPFS_FCB Fcb;
   PNPFS_PIPE Pipe;
   PVOID SystemBuffer;
   ULONG BufferLength;
   NTSTATUS Status;
   
   DPRINT("NpfsSetInformation(DeviceObject %p Irp %p)\n", DeviceObject, Irp);
   
   IoStack = IoGetCurrentIrpStackLocation (Irp);
   FileInformationClass = IoStack->Parameters.QueryFile.FileInformationClass;
   FileObject = IoStack->FileObject;
   Fcb = (PNPFS_FCB)FileObject->FsContext;
   Pipe = Fcb->Pipe;
   
   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
   BufferLength = IoStack->Parameters.QueryFile.Length;
   
   DPRINT("Pipe name: %wZ\n", &Pipe->PipeName);
   DPRINT("FileInformationClass %d\n", FileInformationClass);
   DPRINT("SystemBuffer %x\n", SystemBuffer);
   DPRINT("BufferLength %lu\n", BufferLength);
   
   switch (FileInformationClass)
     {
     case FilePipeInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
     case FilePipeLocalInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
     case FilePipeRemoteInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
     default:
	Status = STATUS_NOT_SUPPORTED;
     }
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp,
		     IO_NO_INCREMENT);
   
   return(Status);
}

/* EOF */
