/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ext2/inode.c
 * PURPOSE:          Manipulating inodes
 * PROGRAMMER:       David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *          26/12/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

#define NDEBUG
#include <internal/debug.h>

#include "ext2fs.h"

/* FUNCTIONS ***************************************************************/

struct ext2_group_desc* Ext2LoadGroupDesc(PDEVICE_EXTENSION DeviceExt,
					  ULONG block_group)
{
   struct ext2_group_desc* buffer;
   ULONG block;
   struct ext2_group_desc* gdp;
   
   buffer = ExAllocatePool(NonPagedPool, BLOCKSIZE);
   
   block = block_group / (BLOCKSIZE / sizeof(struct ext2_group_desc));
   
   Ext2ReadSectors(DeviceExt->StorageDevice,
		   2 + block,
		   1,
		   buffer);
   
   gdp = &buffer[block_group % (BLOCKSIZE / sizeof(struct ext2_group_desc))];
   
   DPRINT("gdp->bg_free_blocks_count %d\n",gdp->bg_free_blocks_count);
   DPRINT("gdp->bg_inode_table %d\n",gdp->bg_inode_table);
   
   return(gdp);
   
}

#define INODES_PER_BLOCK (BLOCKSIZE / sizeof(struct ext2_inode))

VOID Ext2ReadInode(PDEVICE_EXTENSION DeviceExt,
		   ULONG ino,
		   struct ext2_inode* inode)
{
   ULONG block_group;
   struct ext2_group_desc* gdp;
   ULONG offset;
   struct ext2_inode* buffer;
   
   DPRINT("Ext2ReadInode(DeviceExt %x, ino %d, inode %x)\n",
	  DeviceExt,ino,inode);
   
   block_group = (ino - 1) / DeviceExt->superblock->s_inodes_per_group;
   
   gdp = Ext2LoadGroupDesc(DeviceExt, block_group);
   

   
   offset = (ino - 1) % DeviceExt->superblock->s_inodes_per_group;
   
   buffer = ExAllocatePool(NonPagedPool, BLOCKSIZE);
   Ext2ReadSectors(DeviceExt->StorageDevice,
		   gdp->bg_inode_table + (offset / INODES_PER_BLOCK),
		   1,
		   buffer);
   memcpy(inode,&buffer[offset % INODES_PER_BLOCK],sizeof(struct ext2_inode));
   
   DPRINT("inode->i_uid %d\n",inode->i_uid);
   DPRINT("inode->i_links_count %d\n",inode->i_links_count);
   DPRINT("inode->i_blocks %d\n",inode->i_blocks);
}
