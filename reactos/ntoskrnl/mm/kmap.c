/* $Id: kmap.c,v 1.1 2000/05/13 13:51:05 dwelch Exp $
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

/*
 * Memory managment initalized symbol for the base of the pool
 */
static ULONG MmKernelMapBase = 0;

#define ALLOC_MAP_SIZE (NONPAGED_POOL_SIZE / PAGESIZE)

/*
 * One bit for each page in the kmalloc region
 *      If set then the page is used by a kmalloc block
 */
static ULONG alloc_map[ALLOC_MAP_SIZE/32]={0,};
static KSPIN_LOCK AllocMapLock;

/* FUNCTIONS ***************************************************************/

PVOID MmAllocPageFrame(VOID)
{
   KIRQL oldIrql;
   ULONG i = ((ULONG)Addr - kernel_pool_base) / PAGESIZE;
   
   KeAcquireSpinLock(&AllocMapLock, &oldIrql);
   MmSetPage(NULL, (PVOID)Addr, 0, 0);
   clear_bit(i%32, &alloc_map[i/32]);
   KeReleaseSpinLock(&AllocMapLock, oldIrql);
}

VOID MmFreePageFrame(PVOID Addr)
{
}

VOID ExUnmapPage(PVOID Addr)
{
   KIRQL oldIrql;
   ULONG i = ((ULONG)Addr - kernel_pool_base) / PAGESIZE;
   
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

   PhysPage = (ULONG)MmAllocPage();
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
	     addr = kernel_pool_base + (i*PAGESIZE);
	     MmSetPage(NULL, (PVOID)addr, PAGE_READWRITE, PhysPage);
	     KeReleaseSpinLock(&AllocMapLock, oldlvl);
	     return((PVOID)addr);
	  }
     }
   KeReleaseSpinLock(&AllocMapLock, oldlvl);
   return(NULL);
}

VOID MmKernelMapInit(ULONG BaseAddress)
{
   MmKernelMapBase = BaseAddress;
   KeInitializeSpinLock(&AllocMapLock);
}

