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
#include <string.h>
#include <internal/string.h>
#include <internal/bitops.h>
#include <ddk/ntifs.h>

#define NDEBUG
#include <internal/debug.h>

#include "minix.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS MinixDeleteInode(PDEVICE_OBJECT Volume,
			  MINIX_DEVICE_EXTENSION* DeviceExt,
			  ULONG ino)
{
   PULONG Buffer;
   ULONG off;
   
   Buffer = ExAllocatePool(NonPagedPool,BLOCKSIZE);
   MinixReadSector(Volume, (ino / 8192)+2, (PVOID)Buffer);
   off = ino % 8192;
   clear_bit(off%32,&Buffer[off/32]);
   MinixWriteSector(Volume, (ino / 8192)+2, (PVOID)Buffer);
   return(STATUS_SUCCESS);
}

static ULONG MinixAllocateInode(PDEVICE_OBJECT Volume,
				MINIX_DEVICE_EXTENSION* DeviceExt)
{
   ULONG i;
   PULONG Buffer;
   ULONG ino;
   
   Buffer = ExAllocatePool(NonPagedPool,BLOCKSIZE);
   for (i=0; i<DeviceExt->sb->s_imap_blocks; i++)
     {
	MinixReadSector(Volume,i + 2,Buffer);
	ino = find_first_zero_bit(Buffer,8192);
	if (ino < 8192)
	  {
	     set_bit(ino%32,&Buffer[32]);
	     MinixWriteSector(Volume,i + 2,Buffer);
	     ExFreePool(Buffer);
	     return(ino + (i*8192));
	  }
     }
   ExFreePool(Buffer);
   return(0);
}

ULONG MinixNewInode(PDEVICE_OBJECT Volume,
		    MINIX_DEVICE_EXTENSION* DeviceExt,
		    struct minix_inode* new_inode)
{
   ULONG ino;
   
   ino = MinixAllocateInode(Volume,DeviceExt);
   if (ino == 0)
     {
	return(0);
     }
   MinixWriteInode(Volume,DeviceExt,ino,new_inode);
   return(ino);
}

NTSTATUS MinixWriteInode(PDEVICE_OBJECT Volume,
			 MINIX_DEVICE_EXTENSION* DeviceExt,
			 ULONG ino, 
			 struct minix_inode* result)
{
   int block;
   char* buffer;
   struct minix_inode* inodes;
   
   DPRINT("MinixWriteInode(ino %x, result %x)\n",ino,result);
   
   buffer = ExAllocatePool(NonPagedPool,1024);
   inodes = (struct minix_inode *)buffer;
   
   block = 2 + DeviceExt->sb->s_imap_blocks + DeviceExt->sb->s_zmap_blocks 
           + ((ino-1) / MINIX_INODES_PER_BLOCK);
   MinixReadSector(Volume,block,buffer);
   memcpy(&inodes[(ino-1)%MINIX_INODES_PER_BLOCK],result,
	  sizeof(struct minix_inode));
   MinixWriteSector(Volume,block,buffer);
   
   ExFreePool(buffer);
   return(STATUS_SUCCESS);
}

NTSTATUS MinixReadInode(PDEVICE_OBJECT DeviceObject,
			MINIX_DEVICE_EXTENSION* DeviceExt,
			ULONG ino, 
			struct minix_inode* result)
{
   PCCB Ccb;
   int block;
   struct minix_inode* inodes;
   
   DPRINT("MinixReadInode(ino %x, result %x)\n",ino,result);
   
   block = 2 + DeviceExt->sb->s_imap_blocks + DeviceExt->sb->s_zmap_blocks 
           + ((ino-1) / MINIX_INODES_PER_BLOCK);
   DPRINT("Reading block %x offset %x\n",block,block*BLOCKSIZE);
   DPRINT("Index %x\n",(ino-1)%MINIX_INODES_PER_BLOCK);
   
   Ccb = CbAcquireForRead(&DeviceExt->Dccb,
			  block);
   inodes = (struct minix_inode *)Ccb->Buffer;
     
   memcpy(result,&inodes[(ino-1)%MINIX_INODES_PER_BLOCK],
	  sizeof(struct minix_inode));
   DPRINT("result->i_uid %x\n",result->i_uid);
   DPRINT("result->i_size %x\n",result->i_size);

   CbReleaseFromRead(&DeviceExt->Dccb,Ccb);
   
   return(STATUS_SUCCESS);
}
