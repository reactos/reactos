/* $Id: kmap.c,v 1.10 2001/05/05 19:13:10 chorns Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/kmap.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
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
static unsigned int AllocMap[ALLOC_MAP_SIZE/32]={0,};
static KSPIN_LOCK AllocMapLock;

static PVOID NonPagedPoolBase;

/* FUNCTIONS ***************************************************************/

VOID 
ExUnmapPage(PVOID Addr)
{
   KIRQL oldIrql;
   ULONG i = (Addr - NonPagedPoolBase) / PAGESIZE;
   
   DPRINT("ExUnmapPage(Addr %x)\n",Addr);
   DPRINT("i %x\n",i);
   
   KeAcquireSpinLock(&AllocMapLock, &oldIrql);
   MmDeleteVirtualMapping(NULL, (PVOID)Addr, FALSE, NULL, NULL);
   clear_bit(i%32, &AllocMap[i/32]);
   KeReleaseSpinLock(&AllocMapLock, oldIrql);
}

PVOID 
ExAllocatePage(VOID)
{
  ULONG PhysPage;

  PhysPage = (ULONG)MmAllocPage(0);
  if (PhysPage == 0)
    {
      return(NULL);
    }

  return(ExAllocatePageWithPhysPage(PhysPage));
}

NTSTATUS
MiZeroPage(ULONG PhysPage)
{
  PVOID TempAddress;

  TempAddress = ExAllocatePageWithPhysPage(PhysPage);
  if (TempAddress == NULL)
    {
      return(STATUS_NO_MEMORY);
    }
  memset(TempAddress, 0, PAGESIZE);
  ExUnmapPage(TempAddress);
  return(STATUS_SUCCESS);
}

NTSTATUS
MiCopyFromUserPage(ULONG DestPhysPage, PVOID SourceAddress)
{
  PVOID TempAddress;

  TempAddress = ExAllocatePageWithPhysPage(DestPhysPage);
  if (TempAddress == NULL)
    {
      return(STATUS_NO_MEMORY);
    }
  memcpy(TempAddress, SourceAddress, PAGESIZE);
  ExUnmapPage(TempAddress);
  return(STATUS_SUCCESS);
}

PVOID 
ExAllocatePageWithPhysPage(ULONG PhysPage)
{
   KIRQL oldlvl;
   ULONG addr;
   ULONG i;
   NTSTATUS Status;

   KeAcquireSpinLock(&AllocMapLock, &oldlvl);
   for (i=1; i<ALLOC_MAP_SIZE;i++)
     {
	if (!test_bit(i%32,&AllocMap[i/32]))
	  {
	     DPRINT("i %x\n",i);
	     set_bit(i%32,&AllocMap[i/32]);
	     addr = (ULONG)(NonPagedPoolBase + (i*PAGESIZE));
	     Status = MmCreateVirtualMapping(NULL, 
					     (PVOID)addr, 
					     PAGE_READWRITE | PAGE_SYSTEM, 
					     PhysPage);
	     if (!NT_SUCCESS(Status))
	       {
		  DbgPrint("Unable to create virtual mapping\n");
		  KeBugCheck(0);
	       }
	     KeReleaseSpinLock(&AllocMapLock, oldlvl);
	     return((PVOID)addr);
	  }
     }
   KeReleaseSpinLock(&AllocMapLock, oldlvl);
   return(NULL);
}

VOID 
MmInitKernelMap(PVOID BaseAddress)
{
   NonPagedPoolBase = BaseAddress;
   KeInitializeSpinLock(&AllocMapLock);
}

PVOID
MiAllocNonPagedPoolRegion(ULONG nr_pages)
/*
 * FUNCTION: Allocates a region of pages within the nonpaged pool area
 */
{
   unsigned int start = 0;
   unsigned int length = 0;
   unsigned int i,j;

   for (i=1; i<ALLOC_MAP_SIZE;i++)
     {
	if (!test_bit(i%32,&AllocMap[i/32]))
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
		  for (j=start;j<(start+length);j++)
		    {
		       set_bit(j%32,&AllocMap[j/32]);
		    }
      DPRINT("returning %x\n",((start*PAGESIZE)+NonPagedPoolBase));
		  return(((start*PAGESIZE)+NonPagedPoolBase));
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
