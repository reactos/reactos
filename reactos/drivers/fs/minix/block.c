/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/minix/minix.c
 * PURPOSE:          Minix FSD
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

#include "minix.h"

/* FUNCTIONS ****************************************************************/

static unsigned int MinixGetBlock(PDEVICE_OBJECT DeviceObject,
				  PMINIX_DEVICE_EXTENSION DeviceExt,
				  struct minix_inode* inode, 
				  ULONG FileOffset)
{
   int block;
   PVOID BaseAddress;
   PCACHE_SEGMENT CacheSeg;
   ULONG blk;
   
   DPRINT("MinixGetBlock(inode %x, blk %d)\n",inode,blk);
   
   blk = FileOffset / BLOCKSIZE;
   
   /*
    * The first few blocks are available in the inode
    */
   if (blk < 7)
     {
	block = inode->i_zone[blk];
	return(block);
     }
   blk = blk - 7;
   
   /*
    * Retrieve a single-indirect block
    */
   if (blk < 512)
     {
	block = inode->i_zone[7];
	
	MinixRequestCacheBlock(DeviceObject,
			       DeviceExt->Bcb,
			       block * BLOCKSIZE,
			       &BaseAddress,
			       &CacheSeg);
			       
	block = ((PUSHORT)BaseAddress)[blk];

	CcReleaseCachePage(DeviceExt->Bcb,
			   CacheSeg,
			   TRUE);
	
	return(block);
     }
   
   /*
    * Get a double indirect block
    */
   blk = blk - 512;
   block = inode->i_zone[8];

   MinixRequestCacheBlock(DeviceObject,
			  DeviceExt->Bcb,
			  block * BLOCKSIZE,
			  &BaseAddress,
			  &CacheSeg);
   
   block = ((PUSHORT)BaseAddress)[(blk>>9)&511];

   CcReleaseCachePage(DeviceExt->Bcb,
		      CacheSeg,
		      TRUE);
   
   MinixRequestCacheBlock(DeviceObject,
			  DeviceExt->Bcb,
			  block * BLOCKSIZE,
			  &BaseAddress,
			  &CacheSeg);
   
   block = ((PUSHORT)BaseAddress)[blk&512];

   CcReleaseCachePage(DeviceExt->Bcb,
		      CacheSeg,
		      TRUE);

   
   return(block);
}

NTSTATUS MinixReadBlock(PDEVICE_OBJECT DeviceObject,
			PMINIX_DEVICE_EXTENSION DeviceExt,
			struct minix_inode* inode, 
			ULONG FileOffset,
			PULONG DiskOffset)
{
   unsigned int block;
   NTSTATUS Status;
   
   DPRINT("MinixReadBlock()\n");
   
   block = MinixGetBlock(DeviceObject, DeviceExt,inode, FileOffset);
   
   (*DiskOffset) = block * BLOCKSIZE;

   return(STATUS_SUCCESS);
}
