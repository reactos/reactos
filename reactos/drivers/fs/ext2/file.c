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

/* FUNCTIONS ****************************************************************/

#define addr_per_block (BLOCKSIZE / sizeof(ULONG))

ULONG Ext2BlockMap(PDEVICE_EXTENSION DeviceExt,
		  struct ext2_inode* inode,
		  ULONG offset)
{
   ULONG block;
   PULONG TempBuffer;
   BOOL b;
   
   DPRINT("Ext2BlockMap(DeviceExt %x, inode %x, offset %d)\n",
	   DeviceExt,inode,offset);
   if (offset < EXT2_NDIR_BLOCKS)
     {
	block = inode->i_block[offset];
	DPRINT("block %d\n",block);
	return(block);
     }
   offset = offset - EXT2_NDIR_BLOCKS;
   if (offset < addr_per_block)
     {
	block = inode->i_block[EXT2_IND_BLOCK];
	TempBuffer = ExAllocatePool(NonPagedPool, BLOCKSIZE);
	b = Ext2ReadSectors(DeviceExt->StorageDevice,
			    block,
			    1,
			    TempBuffer);
	if (!b)
	  {
	     DbgPrint("ext2fs:%s:%d: Disk io failed\n", __FILE__, __LINE__);
	     return(0);
	  }
	block = TempBuffer[offset];
	ExFreePool(TempBuffer);
	return(block);
     }
   offset = offset - addr_per_block;
   DbgPrint("Failed at %s:%d\n",__FILE__,__LINE__);
   for(;;);
}

