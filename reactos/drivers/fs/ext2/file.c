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
#include <internal/string.h>
#include <wstring.h>

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
	Ext2ReadSectors(DeviceExt->StorageDevice,
			block,
			1,
			TempBuffer);
	block = TempBuffer[offset];
	ExFreePool(TempBuffer);
	return(block);
     }
   DbgPrint("Failed at %s:%d\n",__FILE__,__LINE__);
   for(;;);
}
