/* $Id: rw.c,v 1.1 2000/03/26 22:00:09 dwelch Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       services/fs/np/rw.c
 * PURPOSE:    Named pipe filesystem
 * PROGRAMMER: David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <internal/debug.h>

#include "npfs.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS NpfsRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION IoStack;
   PFILE_OBJECT FileObject;
   NTSTATUS Status;
   PNPFS_DEVICE_EXTENSION DeviceExt;
   PWSTR PipeName;
   PNPFS_FSCONTEXT PipeDescr;
   NTSTATUS Status;
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PNPFS_CONTEXT current;
   ULONG Information;
   
   DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   FileObject = IoStack->FileObject;
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
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = Information;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   return(Status);
}


/* EOF */
