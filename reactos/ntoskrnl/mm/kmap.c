/* $Id: kmap.c,v 1.19 2002/09/07 15:12:59 chorns Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/kmap.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *****************************************************************/

#define ALLOC_MAP_SIZE (NONPAGED_POOL_SIZE / PAGE_SIZE)

/*
 * One bit for each page in the kmalloc region
 *      If set then the page is used by a kmalloc block
 */
static ULONG AllocMap[ALLOC_MAP_SIZE/32]={0,};
static KSPIN_LOCK AllocMapLock;
static ULONG AllocMapHint = 1;

static PVOID NonPagedPoolBase;

/* FUNCTIONS ***************************************************************/

VOID 
ExUnmapPage(PVOID Addr)
{
   KIRQL oldIrql;
   ULONG i = (Addr - NonPagedPoolBase) / PAGE_SIZE;
   
   DPRINT("ExUnmapPage(Addr %x)\n",Addr);
   DPRINT("i %x\n",i);
   
   MmDeleteVirtualMapping(NULL, (PVOID)Addr, FALSE, NULL, NULL);
   KeAcquireSpinLock(&AllocMapLock, &oldIrql);   
   AllocMap[i / 32] &= (~(1 << (i % 32)));
   AllocMapHint = min(AllocMapHint, i);
   KeReleaseSpinLock(&AllocMapLock, oldIrql);
}

PVOID 
ExAllocatePage(VOID)
{
  PHYSICAL_ADDRESS PhysPage;
  NTSTATUS Status;

  Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &PhysPage);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }

  return(ExAllocatePageWithPhysPage(PhysPage));
}

NTSTATUS
MiZeroPage(PHYSICAL_ADDRESS PhysPage)
{
  PVOID TempAddress;

  TempAddress = ExAllocatePageWithPhysPage(PhysPage);
  if (TempAddress == NULL)
    {
      return(STATUS_NO_MEMORY);
    }
  memset(TempAddress, 0, PAGE_SIZE);
  ExUnmapPage(TempAddress);
  return(STATUS_SUCCESS);
}

NTSTATUS
MiCopyFromUserPage(PHYSICAL_ADDRESS DestPhysPage, PVOID SourceAddress)
{
  PVOID TempAddress;

  TempAddress = ExAllocatePageWithPhysPage(DestPhysPage);
  if (TempAddress == NULL)
    {
      return(STATUS_NO_MEMORY);
    }
  memcpy(TempAddress, SourceAddress, PAGE_SIZE);
  ExUnmapPage(TempAddress);
  return(STATUS_SUCCESS);
}

PVOID
ExAllocatePageWithPhysPage(PHYSICAL_ADDRESS PhysPage)
{
   KIRQL oldlvl;
   ULONG addr;
   ULONG i;
   NTSTATUS Status;

   KeAcquireSpinLock(&AllocMapLock, &oldlvl);
   for (i = AllocMapHint; i < ALLOC_MAP_SIZE; i++)
     {
       if (!(AllocMap[i / 32] & (1 << (i % 32))))
	 {
	    DPRINT("i %x\n",i);
	    AllocMap[i / 32] |= (1 << (i % 32));
	    AllocMapHint = i + 1;
	    addr = (ULONG)(NonPagedPoolBase + (i*PAGE_SIZE));
	    Status = MmCreateVirtualMapping(NULL, 
					    (PVOID)addr, 
					    PAGE_READWRITE | PAGE_SYSTEM, 
					    PhysPage,
					    FALSE);
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

VOID
MiFreeNonPagedPoolRegion(PVOID Addr, ULONG Count, BOOLEAN Free)
{
  ULONG i;
  ULONG Base = (Addr - NonPagedPoolBase) / PAGE_SIZE;
  ULONG Offset;
  KIRQL oldlvl;
  
  KeAcquireSpinLock(&AllocMapLock, &oldlvl);
  AllocMapHint = min(AllocMapHint, Base);
  for (i = 0; i < Count; i++)
    {
      Offset = Base + i;
      AllocMap[Offset / 32] &= (~(1 << (Offset % 32)));       
      MmDeleteVirtualMapping(NULL, 
			     Addr + (i * PAGE_SIZE), 
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
   for (i=AllocMapHint; i<ALLOC_MAP_SIZE;i++)
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
		 AllocMapHint = start + length;
		 for (j=start;j<(start+length);j++)
		   {
		     AllocMap[j / 32] |= (1 << (j % 32));
		   }
		 DPRINT("returning %x\n",((start*PAGE_SIZE)+NonPagedPoolBase));
		 KeReleaseSpinLock(&AllocMapLock, oldlvl);
		 return(((start*PAGE_SIZE)+NonPagedPoolBase));
	       }
	  }
       else
	 {
	   start=0;
	   length=0;
	 }
     }
   DbgPrint("CRITICAL: Out of non-paged pool space\n");
   KeBugCheck(0);
   return(0);
}
