/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/minix/rw.c
 * PURPOSE:          Minix FSD
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

#define NDEBUG
#include <internal/debug.h>

#include "minix.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS MinixWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("MinixWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS MinixRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   ULONG Length;
   PVOID Buffer;
   ULONG Offset;
   ULONG CurrentOffset;
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   MINIX_DEVICE_EXTENSION* DeviceExt = DeviceObject->DeviceExtension;
   struct minix_inode* inode = (struct minix_inode *)FileObject->FsContext;
   unsigned int i;
   PCCB Ccb = NULL;
   
   DPRINT("MinixRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   Length = Stack->Parameters.Read.Length;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   Offset = Stack->Parameters.Read.ByteOffset.LowPart;
   
   DPRINT("Length %d Buffer %x Offset %x\n",Length,Buffer,Offset);
   
   CurrentOffset=Offset;
   
   DPRINT("inode->i_size %d\n",inode->i_size);
   
   if (Offset > inode->i_size)
     {
	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return(STATUS_UNSUCCESSFUL);
     }
   if ((Offset+Length) > inode->i_size)
     {
	Length = inode->i_size - Offset;
     }
   
   if ((Offset%BLOCKSIZE)!=0)
     {
	CHECKPOINT;
	
	CurrentOffset = Offset - (Offset%BLOCKSIZE);
	
	MinixReadBlock(DeviceExt,inode,
		       CurrentOffset/BLOCKSIZE,
		       &Ccb);
	memcpy(Buffer,Ccb->Buffer+(Offset%BLOCKSIZE),
		  min(BLOCKSIZE - (Offset%BLOCKSIZE),Length));
	DPRINT("(BLOCKSIZE - (Offset%BLOCKSIZE)) %d\n",
	       (BLOCKSIZE - (Offset%BLOCKSIZE)));
	DPRINT("Length %d\n",Length);
	CurrentOffset = CurrentOffset + BLOCKSIZE;
	Buffer = Buffer + BLOCKSIZE - (Offset%BLOCKSIZE);
	Length = Length - min(BLOCKSIZE - (Offset%BLOCKSIZE),Length);
	DPRINT("CurrentOffset %d Buffer %x Length %d\n",CurrentOffset,Buffer,
	       Length);
     }
   for (i=0;i<(Length/BLOCKSIZE);i++)
     {
	CHECKPOINT;
	
	DPRINT("Length %d\n",Length);
	
	MinixReadBlock(DeviceExt,inode,
		       CurrentOffset/BLOCKSIZE,&Ccb);
	memcpy(Buffer,Ccb->Buffer,BLOCKSIZE);
	CurrentOffset = CurrentOffset + BLOCKSIZE;
	Buffer = Buffer + BLOCKSIZE;
     }
   if ((Length%BLOCKSIZE) > 0)
     {
	CHECKPOINT;
	
	DPRINT("Length %x Buffer %x\n",(Length%BLOCKSIZE),Buffer);
	
	MinixReadBlock(DeviceExt,inode,
		       CurrentOffset/BLOCKSIZE,
		       &Ccb);	
	memcpy(Buffer,Ccb->Buffer,(Length%BLOCKSIZE));
     }
   
   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = Length;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);
   return(STATUS_SUCCESS);
}
