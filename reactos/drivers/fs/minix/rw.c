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

static NTSTATUS MinixReadFilePage(PDEVICE_OBJECT DeviceObject,
				  PMINIX_DEVICE_EXTENSION DeviceExt,
				  PMINIX_FSCONTEXT FsContext,
				  ULONG Offset,
				  PVOID* Buffer)
{
   NTSTATUS Status;
   ULONG i;
   ULONG DiskOffset;

   *Buffer = ExAllocatePool(NonPagedPool, 4096);

   for (i=0; i<4; i++)
     {
	Status = MinixReadBlock(DeviceObject,
				DeviceExt,
				&FsContext->inode,
				Offset + (i * BLOCKSIZE),
				&DiskOffset);
	MinixReadSector(DeviceObject,
			DiskOffset / BLOCKSIZE,
			(*Buffer) + (i * BLOCKSIZE));
     }
   return(STATUS_SUCCESS);
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
   PMINIX_FSCONTEXT FsContext = (PMINIX_FSCONTEXT)FileObject->FsContext;
   unsigned int i;
   PVOID DiskBuffer;
   
   DPRINT("MinixRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   Length = Stack->Parameters.Read.Length;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   Offset = Stack->Parameters.Read.ByteOffset.u.LowPart;
   
   DPRINT("Length %d Buffer %x Offset %x\n",Length,Buffer,Offset);
   
   CurrentOffset=Offset;
   
   DPRINT("inode->i_size %d\n",inode->i_size);
   
   if (Offset > FsContext->inode.i_size)
     {
	Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return(STATUS_UNSUCCESSFUL);
     }
   if ((Offset+Length) > FsContext->inode.i_size)
     {
	Length = FsContext->inode.i_size - Offset;
     }
   
   if ((Offset%PAGESIZE)!=0)
     {
	CurrentOffset = Offset - (Offset%PAGESIZE);
	
	MinixReadFilePage(DeviceObject,
			  DeviceExt,
			  FsContext,
			  CurrentOffset,
			  &DiskBuffer);

	memcpy(Buffer,
	       DiskBuffer+(Offset%PAGESIZE),
	       min(PAGESIZE - (Offset%PAGESIZE),Length));
	
	ExFreePool(DiskBuffer);
	
	DPRINT("(BLOCKSIZE - (Offset%BLOCKSIZE)) %d\n",
	       (BLOCKSIZE - (Offset%BLOCKSIZE)));
	DPRINT("Length %d\n",Length);
	CurrentOffset = CurrentOffset + PAGESIZE;
	Buffer = Buffer + PAGESIZE - (Offset%PAGESIZE);
	Length = Length - min(PAGESIZE - (Offset%PAGESIZE),Length);
	DPRINT("CurrentOffset %d Buffer %x Length %d\n",CurrentOffset,Buffer,
	       Length);
     }
   for (i=0;i<(Length/PAGESIZE);i++)
     {
	CHECKPOINT;
	
	DPRINT("Length %d\n",Length);
	
	MinixReadFilePage(DeviceObject,
			  DeviceExt,
			  FsContext,
			  CurrentOffset,
			  &DiskBuffer);
	memcpy(Buffer, DiskBuffer, PAGESIZE);
	
	ExFreePool(DiskBuffer);
	
	CurrentOffset = CurrentOffset + PAGESIZE;
	Buffer = Buffer + PAGESIZE;
     }
   if ((Length%PAGESIZE) > 0)
     {
	CHECKPOINT;
	
	DPRINT("Length %x Buffer %x\n",(Length%PAGESIZE),Buffer);
	
	MinixReadFilePage(DeviceObject,
			  DeviceExt,
			  FsContext,
			  CurrentOffset,
			  &DiskBuffer);

	memcpy(Buffer, DiskBuffer, (Length%PAGESIZE));

	ExFreePool(DiskBuffer);

     }
   
   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = Length;
   IoCompleteRequest(Irp,IO_NO_INCREMENT);
   return(STATUS_SUCCESS);
}
