/* $Id: rw.c,v 1.3 2001/07/29 16:40:20 ekohl Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/rw.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include "npfs.h"

//#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   PWSTR PipeName;
//   PNPFS_FSCONTEXT PipeDescr;
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
//   PNPFS_CONTEXT current;
   ULONG Information;
   
   DPRINT1("NpfsRead()\n");
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
   
   Status = STATUS_SUCCESS;
   Information = 0;
   
#if 0
   PipeDescr = FileObject->FsContext;
   
   if (PipeType & NPFS_READMODE_BYTE)
     {
	PLIST_ENTRY current_entry;
	PNPFS_MSG current;
	KIRQL oldIrql;
	ULONG Length;
	PVOID Buffer;
	
	KeAcquireSpinLock(&PipeDescr->MsgListLock, &oldIrql);
	current_entry = PipeDescr->MsgListHead.Flink;
	Information = 0;
	Length = IoStack->Parameters.Read.Length;
	Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
	while (Length > 0 &&
	       current_entry != &PipeDescr->MsgListHead)
	  {
	     current = CONTAINING_RECORD(current_entry,
					 NPFS_MSG,
					 ListEntry);
	     
	     memcpy(Buffer, current->Buffer, current->Length);
	     Buffer = Buffer + current->Length;
	     Length = Length - current->Length;
	     Information = Information + current->Length;
	     
	     current_entry = current_entry->Flink;
	  }
     }
   else
     {
     }
#endif
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = Information;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}


NTSTATUS STDCALL
NpfsWrite(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PIO_STACK_LOCATION IoStack;
  PFILE_OBJECT FileObject;
  PNPFS_FCB Fcb = NULL;
  PNPFS_PIPE Pipe = NULL;
  PUCHAR Buffer;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG Length;
  ULONG Offset;

  DPRINT("NpfsWrite()\n");

  IoStack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = IoStack->FileObject;
  DPRINT("FileObject %p\n", FileObject);
  DPRINT("Pipe name %wZ\n", &FileObject->FileName);

  Fcb = FileObject->FsContext;
  Pipe = Fcb->Pipe;

  Length = IoStack->Parameters.Write.Length;
  Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  Offset = IoStack->Parameters.Write.ByteOffset.u.LowPart;


  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Length;
  
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  
  return(Status);
}

/* EOF */
