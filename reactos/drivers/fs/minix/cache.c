/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/minix/cache.c
 * PURPOSE:          Minix FSD
 * PROGRAMMER:       David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>

//#define NDEBUG
#include <debug.h>

#include "minix.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS MinixRequestCacheBlock(PDEVICE_OBJECT DeviceObject,
				PBCB Bcb,
				ULONG FileOffset,
				PVOID* BaseAddress,
				PCACHE_SEGMENT* CacheSeg)
{
   BOOLEAN UptoDate;
   
   CcRosRequestCacheSegment(Bcb,
		      FileOffset,
		      BaseAddress,
		      &UptoDate,
		      CacheSeg);
   if (!UptoDate)
     {
	MinixReadPage(DeviceObject,
		      PAGE_ROUND_DOWN(FileOffset),
		      BaseAddress);
     }		      
   BaseAddress = BaseAddress + (FileOffset % PAGESIZE);
   
   return(STATUS_SUCCESS);
}

