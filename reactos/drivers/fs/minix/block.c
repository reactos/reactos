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
#include <internal/string.h>
#include <wstring.h>

#define NDEBUG
#include <internal/debug.h>

#include "minix.h"

/* FUNCTIONS ****************************************************************/

static unsigned int MinixGetBlock(PMINIX_DEVICE_EXTENSION DeviceExt,
				  struct minix_inode* inode, 
				  int blk)
{
   int block;
   PCCB Ccb;
   
   DPRINT("MinixGetBlock(inode %x, blk %d)\n",inode,blk);
   
   if (blk < 7)
     {
	block = inode->i_zone[blk];
	return(block);
     }
   blk = blk - 7;
   
   if (blk < 512)
     {
	block = inode->i_zone[7];
	Ccb = CbAcquireForRead(&DeviceExt->Dccb,block);
	block = ((PUSHORT)Ccb->Buffer)[blk];
	CbReleaseFromRead(&DeviceExt->Dccb,Ccb);
	return(block);
     }
   blk = blk - 512;
   block = inode->i_zone[8];
   
   Ccb = CbAcquireForRead(&DeviceExt->Dccb,block);
   block = ((PUSHORT)Ccb->Buffer)[(blk>>9)&511];
   CbReleaseFromRead(&DeviceExt->Dccb,Ccb);
   
   Ccb = CbAcquireForRead(&DeviceExt->Dccb,block);
   block = ((PUSHORT)Ccb->Buffer)[blk&512];
   CbReleaseFromRead(&DeviceExt->Dccb,Ccb);
   
   return(block);
}

NTSTATUS MinixReadBlock(PMINIX_DEVICE_EXTENSION DeviceExt,
			struct minix_inode* inode, 
			int blk,
			PCCB* Ccb)
{
   unsigned int block;
   
   DPRINT("DeviceExt %x\n",DeviceExt);
   DPRINT("inode %x\n",inode);
   DPRINT("blk %d\n",blk);
   DPRINT("Ccb %x\n",Ccb);
   DPRINT("MinixReadBlock(DeviceExt %x, inode %x, blk %d, Ccb %x)\n",
	  DeviceExt,inode,blk,Ccb);
   
   block = MinixGetBlock(DeviceExt,inode,blk);
   (*Ccb) = CbAcquireForRead(&DeviceExt->Dccb,block);
   return(STATUS_SUCCESS);
}
