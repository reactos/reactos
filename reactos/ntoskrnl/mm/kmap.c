/* $Id: kmap.c,v 1.15 2002/05/13 18:10:40 chorns Exp $
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
   AllocMap[i / 32] &= (~(1 << (i % 32)));
   KeReleaseSpinLock(&AllocMapLock, oldIrql);
}

PVOID
ExAllocatePage(VOID)
{
  ULONG_PTR Page;
  NTSTATUS Status;

  Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Page);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }

  return(ExAllocatePageWithPhysPage(Page));
}

NTSTATUS
MiZeroPage(IN ULONG_PTR  Page)
{
  PVOID TempAddress;

  TempAddress = ExAllocatePageWithPhysPage(Page);
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
ExAllocatePageWithPhysPage(IN ULONG_PTR  Page)
{
   KIRQL oldlvl;
   PVOID addr;
   ULONG i;
   NTSTATUS Status;

   KeAcquireSpinLock(&AllocMapLock, &oldlvl);
   for (i = 1; i < ALLOC_MAP_SIZE; i++)
     {
       if (!(AllocMap[i / 32] & (1 << (i % 32))))
	 {
	    DPRINT("i %x\n",i);
	    AllocMap[i / 32] |= (1 << (i % 32));
	    addr = (PVOID) (NonPagedPoolBase + (i*PAGESIZE));
	    Status = MmCreateVirtualMapping(NULL, 
					    addr, 
					    PAGE_READWRITE | PAGE_SYSTEM, 
					    Page,
					    FALSE);
	    if (!NT_SUCCESS(Status))
	      {
		DbgPrint("Unable to create virtual mapping\n");
		KeBugCheck(0);
	      }
	    KeReleaseSpinLock(&AllocMapLock, oldlvl);
	    return(addr);
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

VOID
MiFreeNonPagedPoolRegion(PVOID Addr, ULONG Count, BOOLEAN Free)
{
  ULONG i;
  ULONG Base = (Addr - NonPagedPoolBase) / PAGESIZE;
  ULONG Offset;
  KIRQL oldlvl;
  
  KeAcquireSpinLock(&AllocMapLock, &oldlvl);
  for (i = 0; i < Count; i++)
    {
      Offset = Base + i;
      AllocMap[Offset / 32] &= (~(1 << (Offset % 32))); 
      MmDeleteVirtualMapping(NULL, 
			     Addr + (i * PAGESIZE), 
			     Free, 
			     NULL, 
			     NULL);
    }
  KeReleaseSpinLock(&AllocMapLock, oldlvl);
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
   KIRQL oldlvl;

   KeAcquireSpinLock(&AllocMapLock, &oldlvl);
   for (i=1; i<ALLOC_MAP_SIZE;i++)
     {
       if (!(AllocMap[i/32] & (1 << (i % 32))))
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
		      AllocMap[j / 32] |= (1 << (j % 32));
		    }
		  DPRINT("returning %x\n",((start*PAGESIZE)+NonPagedPoolBase));
		  KeReleaseSpinLock(&AllocMapLock, oldlvl);
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
