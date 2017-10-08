/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ke/i386/patpge.c
* PURPOSE:         Support for PAT and PGE (Large Pages)
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define PDE_BITS 10
#define PTE_BITS 10

/* FUNCTIONS *****************************************************************/

INIT_SECTION
ULONG_PTR
NTAPI
Ki386EnableGlobalPage(IN ULONG_PTR Context)
{
    //PLONG Count;
#if defined(_GLOBAL_PAGES_ARE_AWESOME_)
    ULONG Cr4;
#endif
    BOOLEAN Enable;

    /* Disable interrupts */
    Enable = KeDisableInterrupts();

    /* Spin until other processors are ready */
    //Count = (PLONG)Context;
    //InterlockedDecrement(Count);
    //while (*Count) YieldProcessor();

#if defined(_GLOBAL_PAGES_ARE_AWESOME_)

    /* Get CR4 and ensure global pages are disabled */
    Cr4 = __readcr4();
    ASSERT(!(Cr4 & CR4_PGE));

    /* Reset CR3 to flush the TLB */
    __writecr3(__readcr3());

    /* Now enable PGE */
    __writecr4(Cr4 | CR4_PGE);

#endif

    /* Restore interrupts and return */
    KeRestoreInterrupts(Enable);
    return 0;
}

VOID
NTAPI
INIT_FUNCTION
KiInitializePAT(VOID)
{
    /* FIXME: Support this */
    DPRINT("PAT support detected but not yet taken advantage of\n");
}

ULONG_PTR
NTAPI
INIT_FUNCTION
Ki386EnableTargetLargePage(IN ULONG_PTR Context)
{
    PLARGE_IDENTITY_MAP IdentityMap = (PLARGE_IDENTITY_MAP)Context;

    /* Call helper function with the start address and temporary page table pointer */
    Ki386EnableCurrentLargePage(IdentityMap->StartAddress, IdentityMap->Cr3);

    return 0;
}

PVOID
NTAPI
Ki386AllocateContiguousMemory(PLARGE_IDENTITY_MAP IdentityMap,
                              ULONG PagesCount,
                              BOOLEAN InLower4Gb)
{
    PHYSICAL_ADDRESS AddressMask;
    PVOID Result;
    ULONG SizeInBytes = PagesCount * PAGE_SIZE;

    /* Initialize acceptable address mask to any possible address */
    AddressMask.LowPart = 0xFFFFFFFF;
    AddressMask.HighPart = 0xFFFFFFFF;

    /* Mark out higher 4Gb if caller asked so */
    if (InLower4Gb) AddressMask.HighPart = 0;

    /* Try to allocate the memory */
    Result = MmAllocateContiguousMemory(SizeInBytes, AddressMask);
    if (!Result) return NULL;

    /* Zero it out */
    RtlZeroMemory(Result, SizeInBytes);

    /* Track allocated pages in the IdentityMap structure */
    IdentityMap->PagesList[IdentityMap->PagesCount] = Result;
    IdentityMap->PagesCount++;

    return Result;
}

PHYSICAL_ADDRESS
NTAPI
Ki386BuildIdentityBuffer(PLARGE_IDENTITY_MAP IdentityMap,
                         PVOID StartPtr,
                         ULONG Length)
{
    // TODO: Check whether all pages are below 4GB boundary
    return MmGetPhysicalAddress(StartPtr);
}

BOOLEAN
NTAPI
Ki386IdentityMapMakeValid(PLARGE_IDENTITY_MAP IdentityMap,
                          PHARDWARE_PTE Pde,
                          PHARDWARE_PTE *PageTable)
{
    ULONG NewPage;

    if (Pde->Valid == 0)
    {
        /* Invalid, so allocate a new page for it */
        NewPage = (ULONG)Ki386AllocateContiguousMemory(IdentityMap, 1, FALSE);
        if (!NewPage) return FALSE;

        /* Set PFN to its virtual address and mark it as valid */
        Pde->PageFrameNumber = NewPage >> PAGE_SHIFT;
        Pde->Valid = 1;

        /* Pass page table address to the caller */
        if (PageTable) *PageTable = (PHARDWARE_PTE)NewPage;
    }
    else
    {
        /* Valid entry, just pass the page table address to the caller */
        if (PageTable)
            *PageTable = (PHARDWARE_PTE)(Pde->PageFrameNumber << PAGE_SHIFT);
    }

    return TRUE;
}

BOOLEAN
NTAPI
Ki386MapAddress(PLARGE_IDENTITY_MAP IdentityMap,
                ULONG_PTR VirtualPtr,
                PHYSICAL_ADDRESS PhysicalPtr)
{
    PHARDWARE_PTE Pde, Pte;
    PHARDWARE_PTE PageTable;

    /* Allocate page directory on demand */
    if (!IdentityMap->TopLevelDirectory)
    {
        IdentityMap->TopLevelDirectory = Ki386AllocateContiguousMemory(IdentityMap, 1, 1);
        if (!IdentityMap->TopLevelDirectory) return FALSE;
    }

    /* Get PDE of VirtualPtr and make it writable and valid */
    Pde = &IdentityMap->TopLevelDirectory[(VirtualPtr >> 22) & ((1 << PDE_BITS) - 1)];
    if (!Ki386IdentityMapMakeValid(IdentityMap, Pde, &PageTable)) return FALSE;
    Pde->Write = 1;

    /* Get PTE of VirtualPtr, make it valid, and map PhysicalPtr there */
    Pte = &PageTable[(VirtualPtr >> 12) & ((1 << PTE_BITS) - 1)];
    Pte->Valid = 1;
    Pte->PageFrameNumber = (PFN_NUMBER)(PhysicalPtr.QuadPart >> PAGE_SHIFT);

    return TRUE;
}

VOID
NTAPI
Ki386ConvertPte(PHARDWARE_PTE Pte)
{
    PVOID VirtualPtr;
    PHYSICAL_ADDRESS PhysicalPtr;

    /* Get virtual and physical addresses */
    VirtualPtr = (PVOID)(Pte->PageFrameNumber << PAGE_SHIFT);
    PhysicalPtr = MmGetPhysicalAddress(VirtualPtr);

    /* Map its physical address in the page table provided by the caller */
    Pte->PageFrameNumber = (PFN_NUMBER)(PhysicalPtr.QuadPart >> PAGE_SHIFT);
}

BOOLEAN
NTAPI
Ki386CreateIdentityMap(PLARGE_IDENTITY_MAP IdentityMap, PVOID StartPtr, ULONG PagesCount)
{
    ULONG_PTR Ptr;
    ULONG PteIndex;
    PHYSICAL_ADDRESS IdentityPtr;

    /* Zero out the IdentityMap contents */
    RtlZeroMemory(IdentityMap, sizeof(LARGE_IDENTITY_MAP));

    /* Get the pointer to the physical address and save it in the struct */
    IdentityPtr = Ki386BuildIdentityBuffer(IdentityMap, StartPtr, PagesCount);
    IdentityMap->StartAddress = IdentityPtr.LowPart;
    if (IdentityMap->StartAddress == 0)
    {
        DPRINT1("Failed to get buffer for large pages identity mapping\n");
        return FALSE;
    }
    DPRINT("IdentityMap->StartAddress %p\n", IdentityMap->StartAddress);

    /* Map all pages */
    for (Ptr = (ULONG_PTR)StartPtr;
         Ptr < (ULONG_PTR)StartPtr + PagesCount * PAGE_SIZE;
         Ptr += PAGE_SIZE, IdentityPtr.QuadPart += PAGE_SIZE)
    {
        /* Map virtual address */
        if (!Ki386MapAddress(IdentityMap, Ptr, IdentityPtr)) return FALSE;

        /* Map physical address */
        if (!Ki386MapAddress(IdentityMap, IdentityPtr.LowPart, IdentityPtr)) return FALSE;
    }

    /* Convert all PTEs in the page directory from virtual to physical,
       because Ki386IdentityMapMakeValid mapped only virtual addresses */
    for (PteIndex = 0; PteIndex < (PAGE_SIZE / sizeof(HARDWARE_PTE)); PteIndex++)
    {
        if (IdentityMap->TopLevelDirectory[PteIndex].Valid != 0)
            Ki386ConvertPte(&IdentityMap->TopLevelDirectory[PteIndex]);
    }

    /* Save the page directory address (allocated by Ki386MapAddress) */
    IdentityMap->Cr3 = MmGetPhysicalAddress(IdentityMap->TopLevelDirectory).LowPart;

    DPRINT("IdentityMap->Cr3 0x%x\n", IdentityMap->Cr3);

    return TRUE;
}

VOID
NTAPI
Ki386FreeIdentityMap(PLARGE_IDENTITY_MAP IdentityMap)
{
    ULONG Page;

    DPRINT("Freeing %lu pages allocated for identity mapping\n", IdentityMap->PagesCount);

    /* Free all allocated pages, if any */
    for (Page = 0; Page < IdentityMap->PagesCount; Page++)
        MmFreeContiguousMemory(IdentityMap->PagesList[Page]);
}
