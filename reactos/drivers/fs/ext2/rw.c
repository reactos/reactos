/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ext2/super.c
 * PURPOSE:          ext2 filesystem
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

#include "ext2fs.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS Ext2ReadFile(PDEVICE_EXTENSION DeviceExt, 
		      PFILE_OBJECT FileObject,
		      PVOID Buffer, 
		      ULONG Length, 
		      LARGE_INTEGER OffsetL)
/*
 * FUNCTION: Reads data from a file
 */
{
   PEXT2_FCB Fcb;
   PVOID TempBuffer;
   ULONG Offset = OffsetL.LowPart;
   ULONG block;
   ULONG Delta;
   ULONG i;
   
   DPRINT("Ext2ReadFile(DeviceExt %x, FileObject %x, Buffer %x, Length %d, \n"
	  "OffsetL %d)\n",DeviceExt,FileObject,Buffer,Length,(ULONG)OffsetL);
   
   Fcb = (PEXT2_FCB)FileObject->FsContext;
   TempBuffer = ExAllocatePool(NonPagedPool, BLOCKSIZE);
   
   if (Offset >= Fcb->inode.i_size)
     {
	ExFreePool(TempBuffer);
	return(STATUS_END_OF_FILE);
     }
   if ((Offset + Length) > Fcb->inode.i_size)
     {
	Length = Fcb->inode.i_size - Offset;
     }
   
   CHECKPOINT;
   if ((Offset % BLOCKSIZE) != 0)
     {
	block = Ext2BlockMap(DeviceExt, &Fcb->inode, Offset / BLOCKSIZE);
	Delta = min(BLOCKSIZE - (Offset % BLOCKSIZE),Length);
	Ext2ReadSectors(DeviceExt->StorageDevice,
			block,
			1,
			TempBuffer);
	memcpy(Buffer, TempBuffer + (Offset % BLOCKSIZE), Delta);
	Length = Length - Delta;
	Offset = Offset + Delta;
	Buffer = Buffer + Delta;
     }
   CHECKPOINT;
   for (i=0; i<(Length/BLOCKSIZE); i++)
     {
	block = Ext2BlockMap(DeviceExt, &Fcb->inode, 
			     (Offset / BLOCKSIZE)+i);
	Ext2ReadSectors(DeviceExt->StorageDevice,
			block,
			1,
			Buffer);
	Length = Length - BLOCKSIZE;
	Offset = Offset + BLOCKSIZE;
	Buffer = Buffer + BLOCKSIZE;
     }
   CHECKPOINT;
   if ((Length % BLOCKSIZE) != 0)
     {
	block = Ext2BlockMap(DeviceExt, &Fcb->inode, Offset / BLOCKSIZE);
	Ext2ReadSectors(DeviceExt->StorageDevice,
			block,
			1,
			TempBuffer);
	memcpy(Buffer,TempBuffer,Length);
     }
   
   ExFreePool(TempBuffer);
   
   return(STATUS_SUCCESS);
}
