/* $Id: kmap.c,v 1.2 2000/07/04 08:52:42 dwelch Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/kmap.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>
#include <internal/string.h>
#include <internal/stddef.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/bitops.h>
#include <internal/ntoskrnl.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define ALLOC_MAP_SIZE (NONPAGED_POOL_SIZE / PAGESIZE)

/*
 * One bit for each page in the kmalloc region
 *      If set then the page is used by a kmalloc block
 */
static unsigned int alloc_map[ALLOC_MAP_SIZE/32]={0,};
static KSPIN_LOCK AllocMapLock;

static PVOID kernel_pool_base;

/* FUNCTIONS ***************************************************************/

VOID ExUnmapPage(PVOID Addr)
{
   KIRQL oldIrql;
   ULONG i = (Addr - kernel_pool_base) / PAGESIZE;
   
   DPRINT("ExUnmapPage(Addr %x)\n",Addr);
   DPRINT("i %x\n",i);
   
   KeAcquireSpinLock(&AllocMapLock, &oldIrql);
   MmSetPage(NULL, (PVOID)Addr, 0, 0);
   clear_bit(i%32, &alloc_map[i/32]);
   KeReleaseSpinLock(&AllocMapLock, oldIrql);
}

PVOID ExAllocatePage(VOID)
{
   KIRQL oldlvl;
   ULONG addr;
   ULONG i;
   ULONG PhysPage;

   PhysPage = (ULONG)MmAllocPage(0);
   DPRINT("Allocated page %x\n",PhysPage);
   if (PhysPage == 0)
     {
	return(NULL);
     }

   KeAcquireSpinLock(&AllocMapLock, &oldlvl);
   for (i=1; i<ALLOC_MAP_SIZE;i++)
     {
	if (!test_bit(i%32,&alloc_map[i/32]))
	  {
	     DPRINT("i %x\n",i);
	     set_bit(i%32,&alloc_map[i/32]);
	     addr = (ULONG)(kernel_pool_base + (i*PAGESIZE));
	     MmSetPage(NULL, (PVOID)addr, PAGE_READWRITE, PhysPage);
	     KeReleaseSpinLock(&AllocMapLock, oldlvl);
	     return((PVOID)addr);
	  }
     }
   KeReleaseSpinLock(&AllocMapLock, oldlvl);
   return(NULL);
}

VOID MmInitKernelMap(PVOID BaseAddress)
{
   kernel_pool_base = BaseAddress;
   KeInitializeSpinLock(&AllocMapLock);
}

unsigned int alloc_pool_region(unsigned int nr_pages)
/*
 * FUNCTION: Allocates a region of pages within the nonpaged pool area
 */
{
   unsigned int start = 0;
   unsigned int length = 0;
   unsigned int i,j;
   
   OLD_DPRINT("alloc_pool_region(nr_pages = %d)\n",nr_pages);

   for (i=1; i<ALLOC_MAP_SIZE;i++)
     {
	if (!test_bit(i%32,&alloc_map[i/32]))
	  {
	     if (length == 0)
	       {
		  start=i;
		  length = 1;
	       }
	     else
	       {
		  length++;
	       }
	     if (length==nr_pages)
	       {
                  OLD_DPRINT("found region at %d for %d\n",start,
			 length);
		  for (j=start;j<(start+length);j++)
		    {
		       set_bit(j%32,&alloc_map[j/32]);
		    }
                  OLD_DPRINT("returning %x\n",(start*PAGESIZE)
			 +kernel_pool_base);
		  return((ULONG)((start*PAGESIZE)+kernel_pool_base));
	       }
	  }
	else
	  {
	     start=0;
	     length=0;
	  }
     }
   DbgPrint("CRITICAL: Out of non-paged pool space\n");
   for(;;);
   return(0);
}
