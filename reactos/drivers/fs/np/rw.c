/* $Id: rw.c,v 1.2 2001/05/01 11:09:01 ekohl Exp $
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
   NTSTATUS Status;

   DPRINT1("NpfsWrite()\n");

   Status = STATUS_SUCCESS;

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}

/* EOF */
