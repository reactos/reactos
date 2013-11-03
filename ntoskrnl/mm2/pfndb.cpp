/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/pfndb.cpp
 * PURPOSE:         PFN database implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "memorymanager.hpp"

//#define NDEBUG
#include <debug.h>

// Page coloring: http://www.freebsd.org/doc/en/articles/vm-design/page-coloring-optimizations.html

VOID
PFN_DATABASE::
Map(PLOADER_PARAMETER_BLOCK LoaderBlock, BOOLEAN LargePages)
{
    if (LargePages)
    {
        UNIMPLEMENTED;
        LocatedInPool = TRUE;
    }
    else
    {
        // Get PfnDatabase's virtual address
        PTENTRY *PointerPte = MemoryManager->SystemPtes.Reserve(GetAllocationSize(), NonPagedPoolExpansion, 0, 0, TRUE);
        PfnDatabase = (PMMPFN)PointerPte->PteToAddress();
        LocatedInPool = FALSE;

        PFN_NUMBER FreePage, FreePageCount, PagesLeft, BasePage, PageCount;
        PLIST_ENTRY NextEntry;
        PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
        PTENTRY *LastPte;
        PTENTRY TempPte = MemoryManager->ValidKernelPte;

        /* Get current page data, since we won't be using MxGetNextPage as it would corrupt our state */
        FreePage = MemoryManager->LbFreeDescriptor->BasePage;
        FreePageCount = MemoryManager->LbFreeDescriptor->PageCount;
        PagesLeft = 0;

        /* Loop the memory descriptors */
        NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
        {
            /* Get the descriptor */
            MdBlock = CONTAINING_RECORD(NextEntry,
                                        MEMORY_ALLOCATION_DESCRIPTOR,
                                        ListEntry);
            if ((MdBlock->MemoryType == LoaderFirmwarePermanent) ||
                (MdBlock->MemoryType == LoaderBBTMemory) ||
                (MdBlock->MemoryType == LoaderSpecialMemory))
            {
                /* These pages are not part of the PFN database */
                NextEntry = MdBlock->ListEntry.Flink;
                continue;
            }

            /* Next, check if this is our special free descriptor we've found */
            if (MdBlock == MemoryManager->LbFreeDescriptor)
            {
                /* Use the real numbers instead */
                BasePage = MemoryManager->LbOldFreeDescriptor.BasePage;
                PageCount = MemoryManager->LbOldFreeDescriptor.PageCount;
            }
            else
            {
                /* Use the descriptor's numbers */
                BasePage = MdBlock->BasePage;
                PageCount = MdBlock->PageCount;
            }

            /* Get the PTEs for this range */
            PointerPte = PTENTRY::AddressToPte((ULONG_PTR)GetEntry(BasePage));
            LastPte = PTENTRY::AddressToPte(((ULONG_PTR)GetEntry(BasePage + PageCount)) - 1);
            DPRINT("MD Type: %lx Base: %lx Count: %lx\n", MdBlock->MemoryType, BasePage, PageCount);

            /* Loop them */
            while (PointerPte <= LastPte)
            {
                /* We'll only touch PTEs that aren't already valid */
                if (!PointerPte->IsValid())
                {
                    /* Use the next free page */
                    TempPte.SetPfn(FreePage);
                    ASSERT(FreePageCount != 0);

                    /* Consume free pages */
                    FreePage++;
                    FreePageCount--;
                    if (!FreePageCount)
                    {
                        /* Out of memory */
                        KeBugCheckEx(INSTALL_MORE_MEMORY,
                                     MmNumberOfPhysicalPages,
                                     FreePageCount,
                                     MemoryManager->LbOldFreeDescriptor.PageCount,
                                     1);
                    }

                    /* Write out this PTE */
                    PagesLeft++;
                    PointerPte->WriteValidPte(&TempPte);

                    /* Zero this page */
                    RtlZeroMemory(PointerPte->PteToAddress(), PAGE_SIZE);
                }

                /* Next! */
                PointerPte++;
            }

            /* Do the next address range */
            NextEntry = MdBlock->ListEntry.Flink;
        }

        /* Now update the free descriptors to consume the pages we used up during the PFN allocation loop */
        MemoryManager->LbFreeDescriptor->BasePage = FreePage;
        MemoryManager->LbFreeDescriptor->PageCount = FreePageCount;
    }
}

VOID
PFN_DATABASE::
InitializePfnDatabase(PLOADER_PARAMETER_BLOCK LoaderBlock, BOOLEAN LargePages)
{
    // Init page lists
    ZeroedPageListHead.Total = 0;
    ZeroedPageListHead.ListName = ZeroedPageList;
    ZeroedPageListHead.Flink = MM_EMPTY_LIST;
    ZeroedPageListHead.Flink = MM_EMPTY_LIST;

    FreePageListHead.Total = 0;
    FreePageListHead.ListName = FreePageList;
    FreePageListHead.Flink = MM_EMPTY_LIST;
    FreePageListHead.Flink = MM_EMPTY_LIST;

    StandbyPageListHead.Total = 0;
    StandbyPageListHead.ListName = StandbyPageList;
    StandbyPageListHead.Flink = MM_EMPTY_LIST;
    StandbyPageListHead.Flink = MM_EMPTY_LIST;

    ModifiedPageListHead.Total = 0;
    ModifiedPageListHead.ListName = ModifiedPageList;
    ModifiedPageListHead.Flink = MM_EMPTY_LIST;
    ModifiedPageListHead.Flink = MM_EMPTY_LIST;

    ModifiedNoWritePageListHead.Total = 0;
    ModifiedNoWritePageListHead.ListName = ModifiedNoWritePageList;
    ModifiedNoWritePageListHead.Flink = MM_EMPTY_LIST;
    ModifiedNoWritePageListHead.Flink = MM_EMPTY_LIST;

    BadPageListHead.Total = 0;
    BadPageListHead.ListName = BadPageList;
    BadPageListHead.Flink = MM_EMPTY_LIST;
    BadPageListHead.Flink = MM_EMPTY_LIST;

    ModifiedPageListByColor.Total = 0;
    ModifiedPageListByColor.ListName = ModifiedPageList;
    ModifiedPageListByColor.Flink = MM_EMPTY_LIST;
    ModifiedPageListByColor.Flink = MM_EMPTY_LIST;
    TotalPagesForPagingFile = 0;

    PageLocationList[ZeroedPageList] =          &ZeroedPageListHead;
    PageLocationList[FreePageList] =            &FreePageListHead;
    PageLocationList[StandbyPageList] =         &StandbyPageListHead;
    PageLocationList[ModifiedPageList] =        &ModifiedPageListHead;
    PageLocationList[ModifiedNoWritePageList] = &ModifiedNoWritePageListHead;
    PageLocationList[BadPageList] =             &BadPageListHead;
    PageLocationList[ActiveAndValid] =          NULL;
    PageLocationList[TransitionPage] =          NULL;

    AvailablePages = 0;
    LowMemoryThreshold = 2;
    HighMemoryThreshold = 19;

    KeInitializeEvent (&LowMemoryEvent, NotificationEvent, TRUE);
    KeInitializeEvent (&HighMemoryEvent, NotificationEvent, TRUE);

    // Scan memory and start setting up PFN entries/
    BuildPfnDatabaseFromPages(LoaderBlock);

    // Add the zero page */
    BuildPfnDatabaseZeroPage();

    // Scan the loader block and build the rest of the PFN database
    BuildPfnDatabaseFromLoaderBlock(LoaderBlock, LargePages);

    // Finally add the pages for the PFN database itself
    BuildPfnDatabaseSelf();
}

VOID
PFN_DATABASE::
BuildPfnDatabaseFromPages(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PTENTRY *PointerPde;
    PTENTRY *PointerPte;
    ULONG i, Count, j;
    PFN_NUMBER PageFrameIndex, StartupPdIndex, PtePageIndex;
    PMMPFN Pfn1, Pfn2;
    ULONG_PTR BaseAddress = 0;

    /* PFN of the startup page directory */
    StartupPdIndex = PTENTRY::AddressToPde(PDE_BASE)->GetPfn();

    /* Start with the first PDE and scan them all */
    PointerPde = PTENTRY::AddressToPde(NULL);
    Count = PD_COUNT * PDE_COUNT;
    for (i = 0; i < Count; i++)
    {
        /* Check for valid PDE */
        if (PointerPde->IsValid() && !PointerPde->IsLargePage())
        {
            /* Get the PFN from it */
            PageFrameIndex = PointerPde->GetPfn();

            /* Set up PFN entry for this page */
            Pfn1 = GetEntry(PageFrameIndex);
            Pfn1->u4.PteFrame = StartupPdIndex;
            Pfn1->PteAddress = PointerPde;
            Pfn1->u2.ShareCount++;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiNonCached;
            Pfn1->u3.e1.PageColor = 0;

            // FIXME: Global pages support

            /* Now get the PTE and scan the pages */
            PointerPte = PTENTRY::AddressToPte(BaseAddress);
            for (j = 0; j < PTE_COUNT; j++)
            {
                /* Check for a valid PTE */
                if (PointerPte->IsValid())
                {
                    // FIXME: Global pages support

                    /* Increase the shared count of the PFN entry for the PDE */
                    ASSERT(Pfn1 != NULL);
                    Pfn1->u2.ShareCount++;

                    /* Now check if the PTE is valid memory too */
                    PtePageIndex = PointerPte->GetPfn();

                    // Only add pages above the end of system code
                    if ((BaseAddress >= MM_KSEG2_BASE) &&
                        (PtePageIndex < MemoryManager->GetHighestPhysicalPage()))
                    {
                        /* Get the PFN entry and make sure it too is valid */
                        Pfn2 = GetEntry(PtePageIndex);
                        if ((MemoryManager->IsAddressValid(Pfn2)) &&
                            (MemoryManager->IsAddressValid(Pfn2 + 1)))
                        {
                            /* Setup the PFN entry */
                            Pfn2->u4.PteFrame = PageFrameIndex;
                            Pfn2->PteAddress = PointerPte;
                            Pfn2->u2.ShareCount++;
                            Pfn2->u3.e2.ReferenceCount = 1;
                            Pfn2->u3.e1.PageLocation = ActiveAndValid;
                            Pfn2->u3.e1.CacheAttribute = MiNonCached;
                            Pfn2->u3.e1.PageColor = 0;
                        }
                    }
                }

                /* Next PTE */
                PointerPte++;
                BaseAddress += PAGE_SIZE;
            }
        }
        else
        {
            /* Next PDE mapped address */
            BaseAddress += PDE_MAPPED_VA;
        }

        /* Next PTE */
        PointerPde++;
    }

    KIRQL OldIrql;
    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
    KeFlushCurrentTb();
    KeLowerIrql (OldIrql);
}

VOID
PFN_DATABASE::
BuildPfnDatabaseZeroPage(VOID)
{
    PMMPFN Pfn;
    PTENTRY *PointerPde;
    PFN_NUMBER LowestPage = MemoryManager->GetLowestPhysicalPage();

    /* Grab the lowest page and check if it has no real references */
    Pfn = GetEntry(LowestPage);
    if (!(LowestPage) && !(Pfn->u3.e2.ReferenceCount))
    {
        /* Make it a bogus page to catch errors */
        PointerPde = PTENTRY::AddressToPde(0xFFFFFFFF);
        Pfn->u4.PteFrame = PointerPde->GetPfn();
        Pfn->PteAddress = PointerPde;
        Pfn->u2.ShareCount++;
        Pfn->u3.e2.ReferenceCount = 0xFFF0;
        Pfn->u3.e1.PageLocation = ActiveAndValid;
        Pfn->u3.e1.CacheAttribute = MiNonCached;
        Pfn->u3.e1.PageColor = 0;
    }
}

VOID
PFN_DATABASE::
BuildPfnDatabaseFromLoaderBlock(IN PLOADER_PARAMETER_BLOCK LoaderBlock, BOOLEAN LargePages)
{
    PLIST_ENTRY NextEntry;
    PFN_NUMBER PageCount = 0;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PTENTRY *PointerPte, *PointerPde;
    PFN_NUMBER HighestPhysicalPage = MemoryManager->GetHighestPhysicalPage();

    /* Now loop through the descriptors */
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        /* Get the current descriptor */
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        /* Read its data */
        PageCount = MdBlock->PageCount;
        PageFrameIndex = MdBlock->BasePage;

        /* Don't allow memory above what the PFN database is mapping */
        if (PageFrameIndex > HighestPhysicalPage)
        {
            /* Since they are ordered, everything past here will be larger */
            break;
        }

        /* On the other hand, the end page might be higher up... */
        if ((PageFrameIndex + PageCount) > (HighestPhysicalPage + 1))
        {
            /* In which case we'll trim the descriptor to go as high as we can */
            PageCount = HighestPhysicalPage + 1 - PageFrameIndex;
            MdBlock->PageCount = PageCount;

            /* But if there's nothing left to trim, we got too high, so quit */
            if (!PageCount) break;
        }

        /* Now check the descriptor type */
        switch (MdBlock->MemoryType)
        {
            /* Check for bad RAM */
            case LoaderBad:

                DPRINT1("You either have specified /BURNMEMORY or damaged RAM modules.\n");
                break;

            /* Check for free RAM */
            case LoaderFree:
            case LoaderLoadedProgram:
            case LoaderFirmwareTemporary:
            case LoaderOsloaderStack:

                /* Get the last page of this descriptor. Note we loop backwards */
                PageFrameIndex += PageCount - 1;
                Pfn1 = GetEntry(PageFrameIndex);

                while (PageCount--)
                {
                    /* If the page really has no references, mark it as free */
                    if (!Pfn1->u3.e2.ReferenceCount)
                    {
                        /* Add it to the free list */
                        Pfn1->u3.e1.CacheAttribute = MiNonCached;
                        InsertInList(PageLocationList[FreePageList], PageFrameIndex);
                    }

                    /* Go to the next page */
                    Pfn1--;
                    PageFrameIndex--;
                }

                /* Done with this block */
                break;

            /* Check for pages that are invisible to us */
            case LoaderFirmwarePermanent:
            case LoaderSpecialMemory:
            case LoaderBBTMemory:

                /* And skip them */
                break;

            default:

                /* Map these pages with the KSEG0 mapping that adds 0x80000000 */
                PointerPte = PTENTRY::AddressToPte(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
                Pfn1 = GetEntry(PageFrameIndex);
                while (PageCount--)
                {
                    /* Check if the page is really unused */
                    PointerPde = PTENTRY::AddressToPde(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
                    if (!Pfn1->u3.e2.ReferenceCount)
                    {
                        /* Mark it as being in-use */
                        if (LargePages)
                        {
                            UNIMPLEMENTED;
                        }
                        else
                        {
                            Pfn1->u4.PteFrame = PointerPde->GetPfn();
                        }
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount++;
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        Pfn1->u3.e1.CacheAttribute = MiNonCached;
                        Pfn1->u3.e1.PageColor = 0;
                    }

                    /* Advance page structures */
                    Pfn1++;
                    PageFrameIndex++;
                    PointerPte++;
                }
                break;
        }

        /* Next descriptor entry */
        NextEntry = MdBlock->ListEntry.Flink;
    }
}

VOID
PFN_DATABASE::
BuildPfnDatabaseSelf()
{
    if (LocatedInPool)
    {
        UNIMPLEMENTED;
    }
    else
    {
        // Mark pages which build up PFN itself
        PFN_NUMBER PfnIndex = GetIndexFromPhysical((ULONG_PTR)PfnDatabase);
        PMMPFN Pfn = GetEntry(PfnIndex);

        ULONG k;
        ULONG PfnDbSize = GetAllocationSize();
        for (k = 0; k < PfnDbSize; k++)
        {
            Pfn->PteAddress = (PTENTRY *)(PfnIndex << 2);
            Pfn->u3.e1.PageColor = 0;
            Pfn->u3.e2.ReferenceCount++;
            PfnIndex++;
        }

        // Add fully zero pages to the free list too
        PMMPFN HighestPfn;
        for (HighestPfn = GetEntry(MemoryManager->GetHighestPhysicalPage()); HighestPfn > PfnDatabase;)
        {
            PMMPFN StartPfn, EndPfn;
            if ((ULONG_PTR)HighestPfn & (PAGE_SIZE - 1))
            {
                StartPfn = (PMMPFN)((ULONG_PTR)HighestPfn & ~(PAGE_SIZE - 1));
                EndPfn = HighestPfn + 1;
            }
            else
            {
                StartPfn = (PMMPFN)((ULONG_PTR)HighestPfn - PAGE_SIZE);
                EndPfn = HighestPfn;
            }

            while (HighestPfn > StartPfn)
                HighestPfn--;

            ULONG_PTR ZeroArea = (ULONG_PTR)EndPfn - (ULONG_PTR)HighestPfn;
            if (RtlCompareMemoryUlong(HighestPfn, ZeroArea, 0) == ZeroArea)
            {
                DPRINT1("Adding fully zeroed PFN %x clusters, size %d to the free list\n", HighestPfn, ZeroArea);
                UNIMPLEMENTED;
            }
        }
    }
}

// FIXME: Machine dependent?
VOID
PFN_DATABASE::
InitializeFreePagesByColor()
{
    FreePagesByColor[0] = (PMMCOLOR_TABLES)GetEntry(MemoryManager->GetHighestPhysicalPage() + 1);
    FreePagesByColor[1] = &FreePagesByColor[0][MemoryManager->GetSecondaryColors()];

    // Loop the color list PTEs to map them/
    if (FreePagesByColor[0] > (PMMCOLOR_TABLES)MM_KSEG2_BASE)
    {
        // Get start and end pointers
        PTENTRY *PointerPte = PTENTRY::AddressToPte((ULONG_PTR)FreePagesByColor[0]);
        PTENTRY *LastPte = PTENTRY::AddressToPte((ULONG_PTR)&FreePagesByColor[1][MemoryManager->GetSecondaryColors()] - 1);
        PTENTRY TempEntry = MemoryManager->ValidKernelPte;

        // Loop through them
        while (PointerPte <= LastPte)
        {
            if (!PointerPte->IsValid())
            {
                // Get the next free page from the loaderblock
                TempEntry.SetPfn(MemoryManager->LbGetNextPage(1));
                *PointerPte = TempEntry;

                // Zero the page it maps
                RtlZeroMemory(PointerPte->PteToAddress(), PAGE_SIZE);
            }

            /* Keep going */
            PointerPte++;
        }
    }

    ULONG k;
    for (k = 0; k < MemoryManager->GetSecondaryColors(); k++)
    {
        FreePagesByColor[ZeroedPageList][k].Flink = EmptyList;
        FreePagesByColor[FreePageList][k].Flink = EmptyList;
    }
}

VOID
PFN_DATABASE::
InitializeElement(ULONG PageFrameIndex, PTENTRY *PointerPte, ULONG Modified)
{
    PMMPFN Pfn1;
    PTENTRY *PointerPtePte;
    //ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    // Setup the PTE
    Pfn1 = GetEntry(PageFrameIndex);
    Pfn1->PteAddress = PointerPte;

    // Check if this PFN is part of a valid address space
    if (PointerPte->IsValid())
    {
        // Only valid from MmCreateProcessAddressSpace path
        //ASSERT(PsGetCurrentProcess()->Vm.WorkingSetSize == 0);

        // Make this a demand zero PTE
        Pfn1->OriginalPte.u.Long = MemoryManager->DemandZeroPte.u.Long;

        // Disable caching if needed
        if (PointerPte->IsCachingDisabled())
            Pfn1->OriginalPte.u.Soft.Protection = MM_READWRITE | MM_NOCACHE;
    }
    else
    {
        // Copy the PTE data
        Pfn1->OriginalPte = *PointerPte;
        ASSERT(!((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                 (Pfn1->OriginalPte.u.Soft.Transition == 1)));
    }

    // Otherwise this is a fresh page -- set it up
    ASSERT(Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT(Pfn1->u2.ShareCount == 0);
    Pfn1->u3.e2.ReferenceCount = 1;
    Pfn1->u2.ShareCount = 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    ASSERT(Pfn1->u3.e1.Rom == 0);
    Pfn1->u3.e1.Modified = Modified;

    // Get the page table for the PTE
    PointerPtePte = PointerPte->AddressToPte((ULONG_PTR)PointerPte);

    // Get the PFN for the page table
    PageFrameIndex = PointerPtePte->GetPfn();
    ASSERT(PageFrameIndex != 0);
    Pfn1->u4.PteFrame = PageFrameIndex;

    // Increase its share count so we don't get rid of it
    Pfn1 = GetEntry(PageFrameIndex);
    Pfn1->u2.ShareCount++;
}

ULONG
PFN_DATABASE::
RemoveAnyPage(ULONG PageColor)
{
    PFN_NUMBER PageIndex;

    // Make sure PFN lock is held and we have pages
    //ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(AvailablePages != 0);
    ASSERT(PageColor < MemoryManager->GetSecondaryColors());

    // Check the colored free list
    if (FreePagesByColor[FreePageList][PageColor].Flink != EmptyList)
    {
        // There are pages in the colored free list - return them
        PageIndex = FreePagesByColor[FreePageList][PageColor].Flink;
        RemovePageByColor(PageIndex, PageColor);
        DbgCheckPage(PageIndex);
        return PageIndex;
    }
    // Check the colored zero list
    else if (FreePagesByColor[ZeroedPageList][PageColor].Flink != EmptyList)
    {
        // There are pages in the colored zero list - return them
        PageIndex = FreePagesByColor[ZeroedPageList][PageColor].Flink;
        RemovePageByColor(PageIndex, PageColor);
        DbgCheckPage(PageIndex);
        return PageIndex;
    }
    // Time to try non-colored lists
    else
    {
        // Check the free list
        if  (FreePageListHead.Flink != EmptyList)
        {
            PageIndex = FreePageListHead.Flink;
            PageColor = MemoryManager->GetSecondaryColor(PageIndex);
            RemovePageByColor(PageIndex, PageColor);
            DbgCheckPage(PageIndex);
            return PageIndex;

        }
        // Check the zero list
        else if (ZeroedPageListHead.Flink != EmptyList)
        {
            PageIndex = ZeroedPageListHead.Flink;
            PageColor = MemoryManager->GetSecondaryColor(PageIndex);
            RemovePageByColor(PageIndex, PageColor);
            DbgCheckPage(PageIndex);
            return PageIndex;
         }
    }

    // We couldn't find anything. See if there are any pages in the free list
    if (FreePageListHead.Total)
    {
        // Get one from the free list
        PageIndex = RemovePageFromList(&FreePageListHead);
    }
    else
    {
        // Free list is empty, try to check the zero one
        if (ZeroedPageListHead.Total)
        {
            // Get one from the zero list
            PageIndex = RemovePageFromList(&ZeroedPageListHead);
        }
        else
        {
            // Standby list - last resort!
            ASSERT(StandbyPageListHead.Total);
            PageIndex = RemovePageFromList(&StandbyPageListHead);
        }
    }

    // Check the newly removed page (we get one for sure if we came here)
    DbgCheckPage(PageIndex);

    // Return the page
    return PageIndex;
}

ULONG
PFN_DATABASE::
RemoveZeroPage(ULONG PageColor)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
PFN_DATABASE::
RemovePageByColor(PFN_NUMBER PageIndex, ULONG Color)
{
    PMMPFN Pfn1;
    PMMPFNLIST ListHead;
    MMLISTS ListName;
    PFN_NUMBER OldFlink, OldBlink;
    USHORT OldColor, OldCache;
    PMMCOLOR_TABLES ColorTable;

    // Make sure PFN lock is held
    //ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(Color < MemoryManager->GetSecondaryColors());

    // Get the PFN entry
    Pfn1 = GetEntry(PageIndex);
    ASSERT(Pfn1->u3.e1.RemovalRequested == 0);
    ASSERT(Pfn1->u3.e1.Rom == 0);

    // Capture data for later
    OldColor = Pfn1->u3.e1.PageColor;
    OldCache = Pfn1->u3.e1.CacheAttribute;

    // Could be either on free or zero list
    ListHead = PageLocationList[Pfn1->u3.e1.PageLocation];
    ListName = ListHead->ListName;
    ASSERT(ListName <= FreePageList);

    // Remove a page
    ListHead->Total--;

    // Get the forward and back pointers
    OldFlink = Pfn1->u1.Flink;
    OldBlink = Pfn1->u2.Blink;

    // Check if the next entry is the list head
    if (OldFlink != EmptyList)
    {
        // It is not, so set the backlink of the actual entry, to our backlink
        GetEntry(OldFlink)->u2.Blink = OldBlink;
    }
    else
    {
        // Set the list head's backlink instead
        ListHead->Blink = OldBlink;
    }

    // Check if the back entry is the list head
    if (OldBlink != EmptyList)
    {
        // It is not, so set the backlink of the actual entry, to our backlink
        GetEntry(OldBlink)->u1.Flink = OldFlink;
    }
    else
    {
        // Set the list head's backlink instead
        ListHead->Flink = OldFlink;
    }

    // We are not on a list anymore
    Pfn1->u1.Flink = Pfn1->u2.Blink = 0;

    // Zero flags but restore color and cache
    Pfn1->u3.e2.ShortFlags = 0;
    Pfn1->u3.e1.PageColor = OldColor;
    Pfn1->u3.e1.CacheAttribute = OldCache;

    // Get the first page on the color list
    ASSERT(Color < MemoryManager->GetSecondaryColors());
    ColorTable = &FreePagesByColor[ListName][Color];
    ASSERT(ColorTable->Count >= 1);

    // Set the forward link to whoever we were pointing to
    ColorTable->Flink = Pfn1->OriginalPte.u.Long;

    // One less page
    AvailablePages--;

    // Check if we are too low on pages
    if (AvailablePages < MemoryManager->GetMinimumFreePages())
    {
        DPRINT1("Running low on pages: %d remaining\n", AvailablePages);

        UNIMPLEMENTED;
    }
}

ULONG
PFN_DATABASE::
RemovePageFromList(PMMPFNLIST ListHead)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
PFN_DATABASE::
InsertInList(PMMPFNLIST ListHead, PFN_NUMBER PageFrameIndex)
{
    PFN_NUMBER Flink, LastPage;
    PMMPFN Pfn1, Pfn2;
    MMLISTS ListName;
    PMMCOLOR_TABLES ColorHead;
    ULONG Color;

    /* Make sure the lock is held */
    //ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Make sure the PFN is valid */
    ASSERT((PageFrameIndex) &&
           (PageFrameIndex <= MemoryManager->GetHighestPhysicalPage()) &&
           (PageFrameIndex >= MemoryManager->GetLowestPhysicalPage()));

    /* Page should be unused */
    Pfn1 = GetEntry(PageFrameIndex);
    ASSERT(Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT(Pfn1->u3.e1.Rom != 1);

    /* Is a standby or modified page being inserted? */
    ListName = ListHead->ListName;
    if ((ListName == StandbyPageList) || (ListName == ModifiedPageList))
    {
        /* If the page is in transition, it must also be a prototype page */
        if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
            (Pfn1->OriginalPte.u.Soft.Transition == 1))
        {
            /* Crash the system on inconsistency */
            KeBugCheckEx(MEMORY_MANAGEMENT, 0x8888, 0, 0, 0);
        }
    }

    /* Increment the list count */
    ListHead->Total++;

    /* Is a modified page being inserted? */
    if (ListHead == &ModifiedPageListHead)
    {
        /* For now, only single-prototype pages should end up in this path */
        DPRINT("Modified page being added: %lx\n", PageFrameIndex);
        ASSERT(Pfn1->OriginalPte.u.Soft.Prototype == 0);

        /* Modified pages are colored when they are selected for page file */
        ListHead = &ModifiedPageListByColor;
        ASSERT (ListHead->ListName == ListName);
        ListHead->Total++;

        /* Increment the number of paging file modified pages */
        TotalPagesForPagingFile++;
    }

    /* Don't handle bad pages yet yet */
    ASSERT(Pfn1->u3.e1.RemovalRequested == 0);

    /* Get the last page on the list */
    LastPage = ListHead->Blink;
    if (LastPage != EmptyList)
    {
        /* Link us with the previous page, so we're at the end now */
        GetEntry(LastPage)->u1.Flink = PageFrameIndex;
    }
    else
    {
        /* The list is empty, so we are the first page */
        ListHead->Flink = PageFrameIndex;
    }

    /* Now make the list head point back to us (since we go at the end) */
    ListHead->Blink = PageFrameIndex;

    /* And initialize our own list pointers */
    Pfn1->u1.Flink = EmptyList;
    Pfn1->u2.Blink = LastPage;

    /* Move the page onto its new location */
    Pfn1->u3.e1.PageLocation = ListName;

    /* For zero/free pages, we also have to handle the colored lists */
    if (ListName <= StandbyPageList)
    {
        /* One more page on the system */
        AvailablePages++;

        /* Check if we've reached the configured low memory threshold */
        if (AvailablePages == LowMemoryThreshold)
        {
            /* Clear the event, because now we're ABOVE the threshold */
            KeSetEvent(&LowMemoryEvent, 0, FALSE);
        }
        else if (AvailablePages == HighMemoryThreshold)
        {
            /* Otherwise check if we reached the high threshold and signal the event */
            KeSetEvent(&HighMemoryEvent, 0, FALSE);
        }

        if (ListName == FreePageList || ListName == ZeroedPageList)
        {
            /* Get the page color */
            Color = MemoryManager->GetSecondaryColor(PageFrameIndex);

            /* Get the list for this color */
            ColorHead = &FreePagesByColor[ListName][Color];

            /* Get the old head */
            Flink = ColorHead->Flink;

            /* Make this page point back to the list, and point forwards to the old head */
            Pfn1->OriginalPte.u.Long = EmptyList;

            /* Was the head empty? */
            if (Flink != EmptyList)
            {
                /* No, so make the old head point to this page */
                Pfn2 = (PMMPFN)ColorHead->Blink;
                Pfn2->OriginalPte.u.Long = PageFrameIndex;
                ASSERT(Pfn2 != Pfn1);
            }
            else
            {
                /* Set the new head */
                ColorHead->Flink = PageFrameIndex;
            }

            /* Make it loop back to this page */
            ColorHead->Blink = (PVOID)Pfn1;

            /* One more paged on the colored list */
            ColorHead->Count++;

            /* Wake up zeroing page thread if necessary */
            if (ListName == FreePageList)
            {
                if (FreePageListHead.Total >= 8)
                    MemoryManager->WakeupZeroingThread();
            }
        }
    }
    else if (ListName == ModifiedPageList)
    {
        /* Page must be destined for page file, and not yet written out */
        if (Pfn1->OriginalPte.u.Soft.Prototype == 0)
            ASSERT(Pfn1->OriginalPte.u.Soft.PageFileHigh == 0);

        /* Increment the number of per-process modified pages */
        PsGetCurrentProcess()->ModifiedPageCount++;

        /* Wake up modified page writer if there are not enough free pages */
        if (ModifiedPageListHead.Total >= MemoryManager->GetMaxModifiedPages())
            MemoryManager->WakeupMPWThread();
    }
}

PFN_NUMBER
PFN_DATABASE::
GetAllocationSize()
{
    // Calculate the number of bytes for the PFN database
    // then add the color tables and convert to pages
    PFN_NUMBER Size = (MemoryManager->GetHighestPhysicalPage() + 1) * sizeof(MMPFN);
    Size += MemoryManager->GetSecondaryColors() * sizeof(MMCOLOR_TABLES) * 2;
    Size >>= PAGE_SHIFT;

    // We have to add one to the count here, because in the process of
    // shifting down to the page size, we actually ended up getting the
    // lower aligned size (so say, 0x5FFFF bytes is now 0x5F pages).
    // Later on, we'll shift this number back into bytes, which would cause
    // us to end up with only 0x5F000 bytes -- when we actually want to have
    // 0x60000 bytes.
    Size++;

    DPRINT("PFN DB allocation size %d. Highest physical page: %d\n", Size, MemoryManager->GetHighestPhysicalPage());

    return Size;
}

