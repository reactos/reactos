/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/pool.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <internal/ntoskrnl.h>
#include <ddk/ntddk.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define TAG_NONE (ULONG)(('N'<<0) + ('o'<<8) + ('n'<<16) + ('e'<<24))

/* FUNCTIONS ***************************************************************/

PVOID ExAllocatePool(POOL_TYPE PoolType, ULONG NumberOfBytes)
/*
 * FUNCTION: Allocates pool memory of a specified type and returns a pointer
 * to the allocated block. This routine is used for general purpose allocation
 * of memory
 * ARGUMENTS:
 *        PoolType
 *               Specifies the type of memory to allocate which can be one
 *               of the following:
 *  
 *               NonPagedPool
 *               NonPagedPoolMustSucceed
 *               NonPagedPoolCacheAligned
 *               NonPagedPoolCacheAlignedMustS
 *               PagedPool
 *               PagedPoolCacheAligned
 *        
 *        NumberOfBytes
 *               Specifies the number of bytes to allocate
 * RETURNS: The allocated block on success
 *          NULL on failure
 */
{
   PVOID Block;
   Block = ExAllocateNonPagedPoolWithTag(PoolType,
					 NumberOfBytes,
					 TAG_NONE,
					 (&PoolType)[-1]);
   return(Block);
}

PVOID ExAllocatePoolWithTag(ULONG PoolType, ULONG NumberOfBytes, ULONG Tag)
{
   PVOID Block;
   
   if (PoolType == NonPagedPoolCacheAligned || 
       PoolType == NonPagedPoolCacheAlignedMustS)
     {
	UNIMPLEMENTED;
     }
   
   switch(PoolType)
     {
      case NonPagedPool:
      case NonPagedPoolMustSucceed:
      case NonPagedPoolCacheAligned:
      case NonPagedPoolCacheAlignedMustS:
	Block = ExAllocateNonPagedPoolWithTag(PoolType,
					      NumberOfBytes,
					      Tag,
					      (&PoolType)[-1]);
	break;
	
      case PagedPool:
      case PagedPoolCacheAligned:
	Block = ExAllocatePagedPoolWithTag(PoolType,NumberOfBytes,Tag);
	break;
	
      default:
	return(NULL);
     };
   
   if ((PoolType==NonPagedPoolMustSucceed || 
	PoolType==NonPagedPoolCacheAlignedMustS) && Block==NULL)     
     {
	KeBugCheck(MUST_SUCCEED_POOL_EMPTY);
     }
   return(Block);
}

PVOID ExAllocatePoolWithQuotaTag(POOL_TYPE PoolType, ULONG NumberOfBytes,
				 ULONG Tag)
{
   PVOID Block;
   PKTHREAD current = KeGetCurrentThread();
   
   Block = ExAllocatePoolWithTag(PoolType,NumberOfBytes,Tag);
   switch(PoolType)
     {
      case NonPagedPool:
      case NonPagedPoolMustSucceed:
      case NonPagedPoolCacheAligned:
      case NonPagedPoolCacheAlignedMustS:
//	current->NPagedPoolQuota = current->NPagedPoolQuota - NumberOfBytes;
	break;
	
      case PagedPool:
      case PagedPoolCacheAligned:
//	current->PagedPoolQuota = current->PagedPoolQuota - NumberOfBytes;
	break;	
     };
   return(Block);
}
   
PVOID ExAllocatePoolWithQuota(POOL_TYPE PoolType, ULONG NumberOfBytes)
{
   return(ExAllocatePoolWithQuotaTag(PoolType,NumberOfBytes,TAG_NONE));
}
