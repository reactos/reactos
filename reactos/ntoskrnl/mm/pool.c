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
//   DbgPrint("ExAllocatePool(NumberOfBytes %d) caller %x\n",
//            NumberOfBytes,((PULONG)&PoolType)[-1]);
   Block = ExAllocatePoolWithTag(PoolType,NumberOfBytes,TAG_NONE);
//   DbgPrint("ExAllocatePool() = %x\n",Block);
   return(Block);
}

PVOID ExAllocatePoolWithTag(ULONG type, ULONG size, ULONG Tag)
{
   PVOID Block;
   
   if (type == NonPagedPoolCacheAligned || 
       type == NonPagedPoolCacheAlignedMustS)
     {
	UNIMPLEMENTED;
     }
   
   switch(type)
     {
      case NonPagedPool:
      case NonPagedPoolMustSucceed:
      case NonPagedPoolCacheAligned:
      case NonPagedPoolCacheAlignedMustS:
	Block = ExAllocateNonPagedPoolWithTag(type,size,Tag);
	break;
	
      case PagedPool:
      case PagedPoolCacheAligned:
	Block = ExAllocatePagedPoolWithTag(type,size,Tag);
	break;
	
      default:
	return(NULL);
     };
   
   if ((type==NonPagedPoolMustSucceed || type==NonPagedPoolCacheAlignedMustS)
       && Block==NULL)     
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
