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
#include <internal/string.h>

//#define NDEBUG
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

#define INODES_PER_PAGE (PAGESIZE / sizeof(struct ext2_inode))
#define INODES_PER_BLOCK (BLOCKSIZE / sizeof(struct ext2_inode))

VOID Ext2LoadInode(PDEVICE_EXTENSION DeviceExt,
		   ULONG ino,
		   PEXT2_INODE Inode)
{
   ULONG block_group;
   struct ext2_group_desc* gdp;
   ULONG offset;  
   ULONG dsec;
   BOOLEAN Uptodate;
   struct ext2_inode* ibuffer;
   
   DPRINT("Ext2LoadInode(DeviceExt %x, ino %d, Inode %x)\n",
	  DeviceExt, ino, Inode);
   
   block_group = (ino - 1) / DeviceExt->superblock->s_inodes_per_group;
   
   DPRINT("block_group %d\n",block_group);
   
   gdp = Ext2LoadGroupDesc(DeviceExt, block_group);

   offset = (ino - 1) % DeviceExt->superblock->s_inodes_per_group;
   
   DPRINT("offset %d\n", offset);
   
   dsec = (gdp->bg_inode_table + (offset / INODES_PER_BLOCK)) * BLOCKSIZE;
   
   DPRINT("dsec %d (dsec/BLOCKSIZE) %d PAGE_ROUND_DOWN(dsec) %d\n", 
	  dsec, (dsec/BLOCKSIZE), PAGE_ROUND_DOWN(dsec));
   
   CcRequestCachePage(DeviceExt->Bcb,
		      PAGE_ROUND_DOWN(dsec),
		      &Inode->BaseAddress,
		      &Uptodate,
		      &Inode->CacheSeg);
   DPRINT("PAGE_ROUND_DOWN(dsec)/BLOCKSIZE %d\n",
	  PAGE_ROUND_DOWN(dsec)/BLOCKSIZE);
   if (!Uptodate)
     {
	Ext2ReadSectors(DeviceExt->StorageDevice,
			PAGE_ROUND_DOWN(dsec) / BLOCKSIZE,
			4,
			Inode->BaseAddress);
     }
   ibuffer = ((struct ext2_inode *)Inode->BaseAddress) +
             (dsec - PAGE_ROUND_DOWN(dsec));
   DPRINT("Inode->BaseAddress 0x%x ibuffer 0x%x\n",
	  Inode->BaseAddress, ibuffer);
   Inode->inode = &ibuffer[offset % INODES_PER_PAGE];
   
   DPRINT("inode->i_uid %d\n",Inode->inode->i_uid);
   DPRINT("inode->i_links_count %d\n",Inode->inode->i_links_count);
   DPRINT("inode->i_blocks %d\n",Inode->inode->i_blocks);
   
   DPRINT("Ext2LoadInode() finished\n");
}

VOID Ext2ReleaseInode(PDEVICE_EXTENSION DeviceExt,
		      PEXT2_INODE Inode)
{
   CcReleaseCachePage(DeviceExt->Bcb,
		      Inode->CacheSeg,
		      TRUE);
   Inode->CacheSeg = NULL;
   Inode->BaseAddress = NULL;
   Inode->inode = NULL;
}
   
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
