/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/freelist.c
 * PURPOSE:         Handle the list of free physical pages
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Robert Bergkvist
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializePageList)
#endif


/* TYPES *******************************************************************/

#define MM_PHYSICAL_PAGE_FREE    (0x1)
#define MM_PHYSICAL_PAGE_USED    (0x2)
#define MM_PHYSICAL_PAGE_BIOS    (0x3)

typedef struct _PHYSICAL_PAGE
{
   union
   {
      struct
      {
        ULONG Type: 2;
        ULONG Consumer: 3;
	ULONG Zero: 1;
      }
      Flags;
      ULONG AllFlags;
   };

   LIST_ENTRY ListEntry;
   ULONG ReferenceCount;
   SWAPENTRY SavedSwapEntry;
   ULONG LockCount;
   ULONG MapCount;
   struct _MM_RMAP_ENTRY* RmapListHead;
}
PHYSICAL_PAGE, *PPHYSICAL_PAGE;


#define ASSERT_PFN(x) ASSERT((x)->Flags.Type != 0)

/* GLOBALS ****************************************************************/

static PPHYSICAL_PAGE MmPageArray;
ULONG MmPageArraySize;

static KSPIN_LOCK PageListLock;
static LIST_ENTRY UserPageListHead;
static LIST_ENTRY FreeZeroedPageListHead;
static LIST_ENTRY FreeUnzeroedPageListHead;

static KEVENT ZeroPageThreadEvent;
static BOOLEAN ZeroPageThreadShouldTerminate = FALSE;

static ULONG UnzeroedPageCount = 0;

/* FUNCTIONS *************************************************************/

FORCEINLINE
PPHYSICAL_PAGE
MiGetPfnEntry(IN PFN_TYPE Pfn)
{
    PPHYSICAL_PAGE Page;

    /* Make sure the PFN number is valid */
    ASSERT(Pfn <= MmPageArraySize);
    
    /* Get the entry */
    Page = &MmPageArray[Pfn];
    
    /* Make sure it's valid */
    ASSERT_PFN(Page);
    
    /* Return it */
    return Page;
}

PFN_TYPE
NTAPI
MmGetLRUFirstUserPage(VOID)
{
   PLIST_ENTRY NextListEntry;
   PHYSICAL_PAGE* PageDescriptor;
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   NextListEntry = UserPageListHead.Flink;
   if (NextListEntry == &UserPageListHead)
   {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return 0;
   }
   PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
   ASSERT_PFN(PageDescriptor);
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return PageDescriptor - MmPageArray;
}

VOID
NTAPI
MmSetLRULastPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type == MM_PHYSICAL_PAGE_USED &&
       Page->Flags.Consumer == MC_USER)
   {
      RemoveEntryList(&Page->ListEntry);
      InsertTailList(&UserPageListHead, &Page->ListEntry);
   }
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

PFN_TYPE
NTAPI
MmGetLRUNextUserPage(PFN_TYPE PreviousPfn)
{
   PLIST_ENTRY NextListEntry;
   PHYSICAL_PAGE* PageDescriptor;
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Page = MiGetPfnEntry(PreviousPfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED ||
       Page->Flags.Consumer != MC_USER)
   {
      NextListEntry = UserPageListHead.Flink;
   }
   else
   {
      NextListEntry = Page->ListEntry.Flink;
   }
   if (NextListEntry == &UserPageListHead)
   {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return 0;
   }
   PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return PageDescriptor - MmPageArray;
}

PFN_TYPE
NTAPI
MmGetContinuousPages(ULONG NumberOfBytes,
                     PHYSICAL_ADDRESS LowestAcceptableAddress,
                     PHYSICAL_ADDRESS HighestAcceptableAddress,
                     PHYSICAL_ADDRESS BoundaryAddressMultiple)
{
   ULONG NrPages;
   ULONG i, j;
   ULONG start;
   ULONG last;
   ULONG length;
   ULONG boundary;
   KIRQL oldIrql;

   NrPages = PAGE_ROUND_UP(NumberOfBytes) / PAGE_SIZE;

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   last = min(HighestAcceptableAddress.LowPart / PAGE_SIZE, MmPageArraySize - 1);
   boundary = BoundaryAddressMultiple.LowPart / PAGE_SIZE;

   for (j = 0; j < 2; j++)
   {
      start = -1;
      length = 0;
      /* First try to allocate the pages above the 16MB area. This may fail
       * because there are not enough continuous pages or we cannot allocate
       * pages above the 16MB area because the caller has specify an upper limit.
       * The second try uses the specified lower limit.
       */
      for (i = j == 0 ? 0x100000 / PAGE_SIZE : LowestAcceptableAddress.LowPart / PAGE_SIZE; i <= last; )
      {
         if (MiGetPfnEntry(i)->Flags.Type == MM_PHYSICAL_PAGE_FREE)
         {
            if (start == (ULONG)-1)
            {
               start = i;
               length = 1;
            }
            else
            {
               length++;
               if (boundary)
               {
                  if (start / boundary != i / boundary)
                  {
                      start = i;
                      length = 1;
                  }
               }
            }
            if (length == NrPages)
            {
               break;
            }
         }
         else
         {
            start = (ULONG)-1;
         }
         i++;
      }

      if (start != (ULONG)-1 && length == NrPages)
      {
         for (i = start; i < (start + length); i++)
         {
            PPHYSICAL_PAGE Page;
            Page = MiGetPfnEntry(i);
            RemoveEntryList(&Page->ListEntry);
            if (MmPageArray[i].Flags.Zero == 0)
            {
               UnzeroedPageCount--;
            }
            MmStats.NrFreePages--;
            MmStats.NrSystemPages++;
            Page->Flags.Type = MM_PHYSICAL_PAGE_USED;
            Page->Flags.Consumer = MC_NPPOOL;
            Page->ReferenceCount = 1;
            Page->LockCount = 0;
            Page->MapCount = 0;
            Page->SavedSwapEntry = 0;
         }
         KeReleaseSpinLock(&PageListLock, oldIrql);
         for (i = start; i < (start + length); i++)
         {
            if (MiGetPfnEntry(i)->Flags.Zero == 0)
            {
	       MiZeroPage(i);
            }
            else
            {
      	       MiGetPfnEntry(i)->Flags.Zero = 0;
            }
         }
         return start;
      }
   }
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return 0;
}


BOOLEAN
NTAPI
MiIsPfnRam(PADDRESS_RANGE BIOSMemoryMap,
           ULONG AddressRangeCount,
	   PFN_TYPE Pfn)
{
   BOOLEAN IsUsable;
   LARGE_INTEGER BaseAddress;
   LARGE_INTEGER EndAddress;
   ULONG i;
   if (BIOSMemoryMap != NULL && AddressRangeCount > 0)
   {
      IsUsable = FALSE;
      for (i = 0; i < AddressRangeCount; i++)
      {
	 BaseAddress.u.LowPart = BIOSMemoryMap[i].BaseAddrLow;
	 BaseAddress.u.HighPart = BIOSMemoryMap[i].BaseAddrHigh;
	 EndAddress.u.LowPart = BIOSMemoryMap[i].LengthLow;
	 EndAddress.u.HighPart = BIOSMemoryMap[i].LengthHigh;
	 EndAddress.QuadPart += BaseAddress.QuadPart;
	 BaseAddress.QuadPart = PAGE_ROUND_DOWN(BaseAddress.QuadPart);
         EndAddress.QuadPart = PAGE_ROUND_UP(EndAddress.QuadPart);

	 if ((BaseAddress.QuadPart >> PAGE_SHIFT) <= Pfn &&
	     Pfn < (EndAddress.QuadPart >> PAGE_SHIFT))
	 {
	    if (BIOSMemoryMap[i].Type == 1)
	    {
	       IsUsable = TRUE;
	    }
	    else
	    {
	       return FALSE;
	    }
	 }
      }
      return IsUsable;
   }
   return TRUE;
}


PVOID
INIT_FUNCTION
NTAPI
MmInitializePageList(IN ULONG_PTR FirstPhysKernelAddress,
                     IN ULONG_PTR LastPhysKernelAddress,
                     IN ULONG HighestPage,
                     IN ULONG_PTR LastKernelAddress,
                     IN PADDRESS_RANGE BIOSMemoryMap,
                     IN ULONG AddressRangeCount)
{
    ULONG i;
    ULONG Reserved;
    NTSTATUS Status;
    PFN_TYPE Pfn = 0;
    PHYSICAL_PAGE UsedPage;
    extern PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;
    ULONG PdeStart = PsGetCurrentProcess()->Pcb.DirectoryTableBase.LowPart;
    ULONG PdePageStart, PdePageEnd;
    ULONG VideoPageStart, VideoPageEnd;
    ULONG KernelPageStart, KernelPageEnd;

    /* Initialize the page lists */
    KeInitializeSpinLock(&PageListLock);
    InitializeListHead(&UserPageListHead);
    InitializeListHead(&FreeUnzeroedPageListHead);
    InitializeListHead(&FreeZeroedPageListHead);
 
    /* Set the size and start of the PFN Database */
    MmPageArraySize = HighestPage;
    MmPageArray = (PHYSICAL_PAGE *)LastKernelAddress;
    Reserved = PAGE_ROUND_UP((MmPageArraySize * sizeof(PHYSICAL_PAGE))) / PAGE_SIZE;
    
    /* Update the last kernel address pointers */
    LastKernelAddress = ((ULONG_PTR)LastKernelAddress + (Reserved * PAGE_SIZE));
    LastPhysKernelAddress = (ULONG_PTR)LastPhysKernelAddress + (Reserved * PAGE_SIZE);

    /* Loop every page required to hold the PFN database */
    for (i = 0; i < Reserved; i++)
    {
        PVOID Address = (char*)MmPageArray + (i * PAGE_SIZE);

        /* Check if FreeLDR has already allocated it for us */
        if (!MmIsPagePresent(NULL, Address))
        {
            /* Use one of our highest usable pages */
            Pfn = MiFreeDescriptor->BasePage + MiFreeDescriptor->PageCount - 1;
            MiFreeDescriptor->PageCount--;
            
            /* Set the PFN */
            Status = MmCreateVirtualMappingForKernel(Address,
                                                     PAGE_READWRITE,
                                                     &Pfn,
                                                     1);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Unable to create virtual mapping\n");
                KEBUGCHECK(0);
            }
        }
        else
        {
            /* Setting the page protection is necessary to set the global bit */
            MmSetPageProtect(NULL, Address, PAGE_READWRITE);
        }
    }
    
    /* Clear the PFN database */
    RtlZeroMemory(MmPageArray, MmPageArraySize * sizeof(PHYSICAL_PAGE));

    /* This is what a used page looks like */
    RtlZeroMemory(&UsedPage, sizeof(UsedPage));
    UsedPage.Flags.Type = MM_PHYSICAL_PAGE_USED;
    UsedPage.Flags.Consumer = MC_NPPOOL;
    UsedPage.ReferenceCount = 2;
    UsedPage.MapCount = 1;

    /* We'll be applying a bunch of hacks -- precompute some static values */
    PdePageStart = PdeStart / PAGE_SIZE;
    PdePageEnd = MmFreeLdrPageDirectoryEnd / PAGE_SIZE;
    VideoPageStart = 0xA0000 / PAGE_SIZE;
    VideoPageEnd = 0x100000 / PAGE_SIZE;
    KernelPageStart = FirstPhysKernelAddress / PAGE_SIZE;
    KernelPageEnd = LastPhysKernelAddress / PAGE_SIZE;
    
    /* Loop every page on the system */
    for (i = 0; i <= MmPageArraySize; i++)
    {                
        /* Check if it's part of RAM */
        if (MiIsPfnRam(BIOSMemoryMap, AddressRangeCount, i))
        {
            /* Apply assumptions that all computers are built the same way */
            if (i == 0)
            {
                /* Page 0 is reserved for the IVT */
                MmPageArray[i] = UsedPage;
                MmStats.NrSystemPages++;
            }
            else if (i == 1)
            {
                /* Page 1 is reserved for the PCR */
                MmPageArray[i] = UsedPage;
                MmStats.NrSystemPages++;
            }
            else if (i == 2)
            {
                /* Page 2 is reserved for the KUSER_SHARED_DATA */
                MmPageArray[i] = UsedPage;
                MmStats.NrSystemPages++;
            }
            else if ((i >= PdePageStart) && (i < PdePageEnd))
            {
                /* These pages contain the initial FreeLDR PDEs */
                MmPageArray[i] = UsedPage;
                MmStats.NrSystemPages++;
            }
            else if ((i >= VideoPageStart) && (i < VideoPageEnd))
            {
                /*
                 * These pages are usually for the Video ROM BIOS.
                 * Supposedly anyway. We'll simply ignore the fact that
                 * many systems have this area somewhere else entirely
                 * (which we'll assume to be "free" a couple of lines below)
                 */
                MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
                MmPageArray[i].Flags.Consumer = MC_NPPOOL;
                MmStats.NrSystemPages++;
            }
            else if ((i >= KernelPageStart) && (i < KernelPageEnd))
            {
                /* These are pages beloning to the kernel */
                MmPageArray[i] = UsedPage;
                MmStats.NrSystemPages++;
            }
            else if (i >= (MiFreeDescriptor->BasePage + MiFreeDescriptor->PageCount))
            {
                /* These are pages we allocated above to hold the PFN DB */
                MmPageArray[i] = UsedPage;
                MmStats.NrSystemPages++;
            }
            else
            {
                /*
                 * These are supposedly free pages.
                 * By the way, not all of them are, some contain vital
                 * FreeLDR data, but since we choose to ignore the Memory
                 * Descriptor List, why bother, right?
                 */
                MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
                InsertTailList(&FreeUnzeroedPageListHead,
                               &MmPageArray[i].ListEntry);
                UnzeroedPageCount++;
                MmStats.NrFreePages++;
            }
        }
        else
        {
            /* These are pages reserved by the BIOS/ROMs */
            MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
            MmPageArray[i].Flags.Consumer = MC_NPPOOL;
            MmStats.NrSystemPages++;
        }
    }
    
    KeInitializeEvent(&ZeroPageThreadEvent, NotificationEvent, TRUE);
    
    MmStats.NrTotalPages = MmStats.NrFreePages + MmStats.NrSystemPages + MmStats.NrUserPages;
    MmInitializeBalancer(MmStats.NrFreePages, MmStats.NrSystemPages);
    return((PVOID)LastKernelAddress);
}

VOID
NTAPI
MmSetFlagsPage(PFN_TYPE Pfn, ULONG Flags)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MiGetPfnEntry(Pfn)->AllFlags = Flags;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmSetRmapListHeadPage(PFN_TYPE Pfn, struct _MM_RMAP_ENTRY* ListHead)
{
   KIRQL oldIrql;
    
   KeAcquireSpinLock(&PageListLock, &oldIrql);    
   MiGetPfnEntry(Pfn)->RmapListHead = ListHead;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

struct _MM_RMAP_ENTRY*
NTAPI
MmGetRmapListHeadPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   struct _MM_RMAP_ENTRY* ListHead;
    
   KeAcquireSpinLock(&PageListLock, &oldIrql);    
   ListHead = MiGetPfnEntry(Pfn)->RmapListHead;
   KeReleaseSpinLock(&PageListLock, oldIrql);
    
   return(ListHead);
}

VOID
NTAPI
MmMarkPageMapped(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   if (Pfn < MmPageArraySize)
   {
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      Page = MiGetPfnEntry(Pfn);
      if (Page->Flags.Type == MM_PHYSICAL_PAGE_FREE)
      {
         DPRINT1("Mapping non-used page\n");
         KEBUGCHECK(0);
      }
      Page->MapCount++;
      Page->ReferenceCount++;
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

VOID
NTAPI
MmMarkPageUnmapped(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   if (Pfn < MmPageArraySize)
   {
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      Page = MiGetPfnEntry(Pfn);
      if (Page->Flags.Type == MM_PHYSICAL_PAGE_FREE)
      {
         DPRINT1("Unmapping non-used page\n");
         KEBUGCHECK(0);
      }
      if (Page->MapCount == 0)
      {
         DPRINT1("Unmapping not mapped page\n");
         KEBUGCHECK(0);
      }
      Page->MapCount--;
      Page->ReferenceCount--;
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

ULONG
NTAPI
MmGetFlagsPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG Flags;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Flags = MiGetPfnEntry(Pfn)->AllFlags;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(Flags);
}


VOID
NTAPI
MmSetSavedSwapEntryPage(PFN_TYPE Pfn,  SWAPENTRY SavedSwapEntry)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MiGetPfnEntry(Pfn)->SavedSwapEntry = SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_TYPE Pfn)
{
   SWAPENTRY SavedSwapEntry;
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   SavedSwapEntry = MiGetPfnEntry(Pfn)->SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(SavedSwapEntry);
}

VOID
NTAPI
MmReferencePageUnsafe(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   DPRINT("MmReferencePageUnsafe(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      return;
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Referencing non-used page\n");
      KEBUGCHECK(0);
   }

   Page->ReferenceCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmReferencePage(PFN_TYPE Pfn)
{
   DPRINT("MmReferencePage(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   MmReferencePageUnsafe(Pfn);
}

ULONG
NTAPI
MmGetReferenceCountPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG RCount;
   PPHYSICAL_PAGE Page;

   DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Getting reference count for free page\n");
      KEBUGCHECK(0);
   }

   RCount = Page->ReferenceCount;

   KeReleaseSpinLock(&PageListLock, oldIrql);
   return(RCount);
}

BOOLEAN
NTAPI
MmIsPageInUse(PFN_TYPE Pfn)
{

   DPRINT("MmIsPageInUse(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   return (MiGetPfnEntry(Pfn)->Flags.Type == MM_PHYSICAL_PAGE_USED);
}

VOID
NTAPI
MmDereferencePage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   DPRINT("MmDereferencePage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);
    
   Page = MiGetPfnEntry(Pfn);

   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Dereferencing free page\n");
      KEBUGCHECK(0);
   }
   if (Page->ReferenceCount == 0)
   {
      DPRINT1("Derefrencing page with reference count 0\n");
      KEBUGCHECK(0);
   }

   Page->ReferenceCount--;
   if (Page->ReferenceCount == 0)
   {
      MmStats.NrFreePages++;
      MmStats.NrSystemPages--;
      if (Page->Flags.Consumer == MC_USER) RemoveEntryList(&Page->ListEntry);
      if (Page->RmapListHead != NULL)
      {
         DPRINT1("Freeing page with rmap entries.\n");
         KEBUGCHECK(0);
      }
      if (Page->MapCount != 0)
      {
         DPRINT1("Freeing mapped page (0x%x count %d)\n",
                  Pfn << PAGE_SHIFT, Page->MapCount);
         KEBUGCHECK(0);
      }
      if (Page->LockCount > 0)
      {
         DPRINT1("Freeing locked page\n");
         KEBUGCHECK(0);
      }
      if (Page->SavedSwapEntry != 0)
      {
         DPRINT1("Freeing page with swap entry.\n");
         KEBUGCHECK(0);
      }
      if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
      {
         DPRINT1("Freeing page with flags %x\n",
                  Page->Flags.Type);
         KEBUGCHECK(0);
      }
      Page->Flags.Type = MM_PHYSICAL_PAGE_FREE;
      Page->Flags.Consumer = MC_MAXIMUM;
      InsertTailList(&FreeUnzeroedPageListHead,
                     &Page->ListEntry);
      UnzeroedPageCount++;
      if (UnzeroedPageCount > 8 && 0 == KeReadStateEvent(&ZeroPageThreadEvent))
      {
         KeSetEvent(&ZeroPageThreadEvent, IO_NO_INCREMENT, FALSE);
      }
   }
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

ULONG
NTAPI
MmGetLockCountPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG LockCount;
   PPHYSICAL_PAGE Page;

   DPRINT("MmGetLockCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Getting lock count for free page\n");
      KEBUGCHECK(0);
   }

   LockCount = Page->LockCount;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(LockCount);
}

VOID
NTAPI
MmLockPageUnsafe(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   DPRINT("MmLockPageUnsafe(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Locking free page\n");
      KEBUGCHECK(0);
   }

   Page->LockCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmLockPage(PFN_TYPE Pfn)
{
   DPRINT("MmLockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   MmLockPageUnsafe(Pfn);
}

VOID
NTAPI
MmUnlockPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   DPRINT("MmUnlockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Unlocking free page\n");
      KEBUGCHECK(0);
   }

   Page->LockCount--;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

PFN_TYPE
NTAPI
MmAllocPage(ULONG Consumer, SWAPENTRY SavedSwapEntry)
{
   PFN_TYPE PfnOffset;
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   KIRQL oldIrql;
   BOOLEAN NeedClear = FALSE;

   DPRINT("MmAllocPage()\n");

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   if (IsListEmpty(&FreeZeroedPageListHead))
   {
      if (IsListEmpty(&FreeUnzeroedPageListHead))
      {
         DPRINT1("MmAllocPage(): Out of memory\n");
         KeReleaseSpinLock(&PageListLock, oldIrql);
         return 0;
      }
      ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);
      UnzeroedPageCount--;

      PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);

      NeedClear = TRUE;
   }
   else
   {
      ListEntry = RemoveTailList(&FreeZeroedPageListHead);

      PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
   }

   if (PageDescriptor->Flags.Type != MM_PHYSICAL_PAGE_FREE)
   {
      DPRINT1("Got non-free page from freelist\n");
      KEBUGCHECK(0);
   }
   if (PageDescriptor->MapCount != 0)
   {
      DPRINT1("Got mapped page from freelist\n");
      KEBUGCHECK(0);
   }
   if (PageDescriptor->ReferenceCount != 0)
   {
      DPRINT1("%d\n", PageDescriptor->ReferenceCount);
      KEBUGCHECK(0);
   }
   PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
   PageDescriptor->Flags.Consumer = Consumer;
   PageDescriptor->ReferenceCount = 1;
   PageDescriptor->LockCount = 0;
   PageDescriptor->MapCount = 0;
   PageDescriptor->SavedSwapEntry = SavedSwapEntry;

   MmStats.NrSystemPages++;
   MmStats.NrFreePages--;

   KeReleaseSpinLock(&PageListLock, oldIrql);

   PfnOffset = PageDescriptor - MmPageArray;
   MmSetLRULastPage(PfnOffset);
   if (NeedClear)
   {
      MiZeroPage(PfnOffset);
   }
   if (PageDescriptor->MapCount != 0)
   {
      DPRINT1("Returning mapped page.\n");
      KEBUGCHECK(0);
   }
   return PfnOffset;
}

LONG
NTAPI
MmAllocPagesSpecifyRange(ULONG Consumer,
                         PHYSICAL_ADDRESS LowestAddress,
                         PHYSICAL_ADDRESS HighestAddress,
                         ULONG NumberOfPages,
                         PPFN_TYPE Pages)
{
   PPHYSICAL_PAGE PageDescriptor;
   KIRQL oldIrql;
   PFN_TYPE LowestPage, HighestPage;
   PFN_TYPE pfn;
   ULONG NumberOfPagesFound = 0;
   ULONG i;

   DPRINT("MmAllocPagesSpecifyRange()\n"
          "    LowestAddress = 0x%08x%08x\n"
          "    HighestAddress = 0x%08x%08x\n"
          "    NumberOfPages = %d\n",
          LowestAddress.u.HighPart, LowestAddress.u.LowPart,
          HighestAddress.u.HighPart, HighestAddress.u.LowPart,
          NumberOfPages);

   if (NumberOfPages == 0)
      return 0;

   LowestPage = LowestAddress.LowPart / PAGE_SIZE;
   HighestPage = HighestAddress.LowPart / PAGE_SIZE;
   if ((HighestAddress.u.LowPart % PAGE_SIZE) != 0)
      HighestPage++;

   if (LowestPage >= MmPageArraySize)
   {
      DPRINT1("MmAllocPagesSpecifyRange(): Out of memory\n");
      return -1;
   }
   if (HighestPage > MmPageArraySize)
      HighestPage = MmPageArraySize;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   if (LowestPage == 0 && HighestPage == MmPageArraySize)
   {
      PLIST_ENTRY ListEntry;
      while (NumberOfPagesFound < NumberOfPages)
      {
         if (!IsListEmpty(&FreeZeroedPageListHead))
         {
            ListEntry = RemoveTailList(&FreeZeroedPageListHead);
         }
         else if (!IsListEmpty(&FreeUnzeroedPageListHead))
         {
            ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);
            UnzeroedPageCount--;
         }
         else
         {
            if (NumberOfPagesFound == 0)
            {
               KeReleaseSpinLock(&PageListLock, oldIrql);
               DPRINT1("MmAllocPagesSpecifyRange(): Out of memory\n");
               return -1;
            }
            else
            {
               break;
            }
         }
         PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);

         ASSERT(PageDescriptor->Flags.Type == MM_PHYSICAL_PAGE_FREE);
         ASSERT(PageDescriptor->MapCount == 0);
         ASSERT(PageDescriptor->ReferenceCount == 0);

         /* Allocate the page */
         PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
         PageDescriptor->Flags.Consumer = Consumer;
         PageDescriptor->ReferenceCount = 1;
         PageDescriptor->LockCount = 0;
         PageDescriptor->MapCount = 0;
         PageDescriptor->SavedSwapEntry = 0; /* FIXME: Do we need swap entries? */

         MmStats.NrSystemPages++;
         MmStats.NrFreePages--;

         /* Remember the page */
         pfn = PageDescriptor - MmPageArray;
         Pages[NumberOfPagesFound++] = pfn;
         MmSetLRULastPage(pfn);
      }
   }
   else
   {
      INT LookForZeroedPages;
      for (LookForZeroedPages = 1; LookForZeroedPages >= 0; LookForZeroedPages--)
      {
         for (pfn = LowestPage; pfn < HighestPage; pfn++)
         {
            PageDescriptor = MmPageArray + pfn;

            if (PageDescriptor->Flags.Type != MM_PHYSICAL_PAGE_FREE)
               continue;
            if (PageDescriptor->Flags.Zero != LookForZeroedPages)
               continue;

            ASSERT(PageDescriptor->MapCount == 0);
            ASSERT(PageDescriptor->ReferenceCount == 0);

            /* Allocate the page */
            PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
            PageDescriptor->Flags.Consumer = Consumer;
            PageDescriptor->ReferenceCount = 1;
            PageDescriptor->LockCount = 0;
            PageDescriptor->MapCount = 0;
            PageDescriptor->SavedSwapEntry = 0; /* FIXME: Do we need swap entries? */

            if (!PageDescriptor->Flags.Zero)
               UnzeroedPageCount--;
            MmStats.NrSystemPages++;
            MmStats.NrFreePages--;

            /* Remember the page */
            Pages[NumberOfPagesFound++] = pfn;
            MmSetLRULastPage(pfn);
            if (NumberOfPagesFound == NumberOfPages)
               break;
         }
         if (NumberOfPagesFound == NumberOfPages)
            break;
      }
   }
   KeReleaseSpinLock(&PageListLock, oldIrql);

   /* Zero unzero-ed pages */
   for (i = 0; i < NumberOfPagesFound; i++)
   {
      pfn = Pages[i];
      if (MiGetPfnEntry(pfn)->Flags.Zero == 0)
      {
         MiZeroPage(pfn);
      }
      else
      {
         MiGetPfnEntry(pfn)->Flags.Zero = 0;
      }
   }

   return NumberOfPagesFound;
}

NTSTATUS
NTAPI
MmZeroPageThreadMain(PVOID Ignored)
{
   NTSTATUS Status;
   KIRQL oldIrql;
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   PFN_TYPE Pfn;
   ULONG Count;

   /* Free initial kernel memory */
   //MiFreeInitMemory();

   /* Set our priority to 0 */
   KeGetCurrentThread()->BasePriority = 0;
   KeSetPriorityThread(KeGetCurrentThread(), 0);

   while(1)
   {
      Status = KeWaitForSingleObject(&ZeroPageThreadEvent,
                                     0,
                                     KernelMode,
                                     FALSE,
                                     NULL);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("ZeroPageThread: Wait failed\n");
         KEBUGCHECK(0);
      }

      if (ZeroPageThreadShouldTerminate)
      {
         DPRINT1("ZeroPageThread: Terminating\n");
         return STATUS_SUCCESS;
      }
      Count = 0;
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      while (!IsListEmpty(&FreeUnzeroedPageListHead))
      {
         ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);
         UnzeroedPageCount--;
         PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
         /* We set the page to used, because MmCreateVirtualMapping failed with unused pages */
         PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
         KeReleaseSpinLock(&PageListLock, oldIrql);
         Pfn = PageDescriptor - MmPageArray;
         Status = MiZeroPage(Pfn);

         KeAcquireSpinLock(&PageListLock, &oldIrql);
         if (PageDescriptor->MapCount != 0)
         {
            DPRINT1("Mapped page on freelist.\n");
            KEBUGCHECK(0);
         }
	 PageDescriptor->Flags.Zero = 1;
         PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_FREE;
         if (NT_SUCCESS(Status))
         {
            InsertHeadList(&FreeZeroedPageListHead, ListEntry);
            Count++;
         }
         else
         {
            InsertHeadList(&FreeUnzeroedPageListHead, ListEntry);
            UnzeroedPageCount++;
         }

      }
      DPRINT("Zeroed %d pages.\n", Count);
      KeResetEvent(&ZeroPageThreadEvent);
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }

   return STATUS_SUCCESS;
}

/* EOF */
