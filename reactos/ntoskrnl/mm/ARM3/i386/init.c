/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/i386/init.c
 * PURPOSE:         ARM Memory Manager Initialization for x86
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::INIT"
#define MODULE_INVOLVED_IN_ARM3
#include "../../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

//
// These are all registry-configurable, but by default, the memory manager will
// figure out the most appropriate values.
//
ULONG MmMaximumNonPagedPoolPercent;
ULONG MmSizeOfNonPagedPoolInBytes;
ULONG MmMaximumNonPagedPoolInBytes;

//
// These numbers describe the discrete equation components of the nonpaged
// pool sizing algorithm.
//
// They are described on http://support.microsoft.com/default.aspx/kb/126402/ja
// along with the algorithm that uses them, which is implemented later below.
//
ULONG MmMinimumNonPagedPoolSize = 256 * 1024;
ULONG MmMinAdditionNonPagedPoolPerMb = 32 * 1024;
ULONG MmDefaultMaximumNonPagedPool = 1024 * 1024; 
ULONG MmMaxAdditionNonPagedPoolPerMb = 400 * 1024;

//
// The memory layout (and especially variable names) of the NT kernel mode
// components can be a bit hard to twig, especially when it comes to the non
// paged area.
//
// There are really two components to the non-paged pool:
//
// - The initial nonpaged pool, sized dynamically up to a maximum.
// - The expansion nonpaged pool, sized dynamically up to a maximum.
//
// The initial nonpaged pool is physically continuous for performance, and
// immediately follows the PFN database, typically sharing the same PDE. It is
// a very small resource (32MB on a 1GB system), and capped at 128MB.
//
// Right now we call this the "ARM³ Nonpaged Pool" and it begins somewhere after
// the PFN database (which starts at 0xB0000000).
//
// The expansion nonpaged pool, on the other hand, can grow much bigger (400MB 
// for a 1GB system). On ARM³ however, it is currently capped at 128MB.
//
// The address where the initial nonpaged pool starts is aptly named
// MmNonPagedPoolStart, and it describes a range of MmSizeOfNonPagedPoolInBytes
// bytes.
//
// Expansion nonpaged pool starts at an address described by the variable called
// MmNonPagedPoolExpansionStart, and it goes on for MmMaximumNonPagedPoolInBytes
// minus MmSizeOfNonPagedPoolInBytes bytes, always reaching MmNonPagedPoolEnd
// (because of the way it's calculated) at 0xFFBE0000.
//
// Initial nonpaged pool is allocated and mapped early-on during boot, but what
// about the expansion nonpaged pool? It is instead composed of special pages
// which belong to what are called System PTEs. These PTEs are the matter of a
// later discussion, but they are also considered part of the "nonpaged" OS, due
// to the fact that they are never paged out -- once an address is described by
// a System PTE, it is always valid, until the System PTE is torn down.
//
// System PTEs are actually composed of two "spaces", the system space proper,
// and the nonpaged pool expansion space. The latter, as we've already seen, 
// begins at MmNonPagedPoolExpansionStart. Based on the number of System PTEs
// that the system will support, the remaining address space below this address
// is used to hold the system space PTEs. This address, in turn, is held in the
// variable named MmNonPagedSystemStart, which itself is never allowed to go
// below 0xEB000000 (thus creating an upper bound on the number of System PTEs).
//
// This means that 330MB are reserved for total nonpaged system VA, on top of
// whatever the initial nonpaged pool allocation is.
//
// The following URLs, valid as of April 23rd, 2008, support this evidence:
//
// http://www.cs.miami.edu/~burt/journal/NT/memory.html
// http://www.ditii.com/2007/09/28/windows-memory-management-x86-virtual-address-space/
//
PVOID MmNonPagedSystemStart;
PVOID MmNonPagedPoolStart;
PVOID MmNonPagedPoolExpansionStart;
PVOID MmNonPagedPoolEnd = MI_NONPAGED_POOL_END;

//
// Old ReactOS Mm nonpaged pool
//
extern PVOID MiNonPagedPoolStart;
extern ULONG MiNonPagedPoolLength;

//
// This is where paged pool starts by default
//
PVOID MmPagedPoolStart = MI_PAGED_POOL_START;
PVOID MmPagedPoolEnd;

//
// And this is its default size
//
ULONG MmSizeOfPagedPoolInBytes = MI_MIN_INIT_PAGED_POOLSIZE;
PFN_NUMBER MmSizeOfPagedPoolInPages = MI_MIN_INIT_PAGED_POOLSIZE / PAGE_SIZE;

//
// Session space starts at 0xBFFFFFFF and grows downwards
// By default, it includes an 8MB image area where we map win32k and video card
// drivers, followed by a 4MB area containing the session's working set. This is
// then followed by a 20MB mapped view area and finally by the session's paged
// pool, by default 16MB.
// 
// On a normal system, this results in session space occupying the region from
// 0xBD000000 to 0xC0000000
//
// See miarm.h for the defines that determine the sizing of this region. On an
// NT system, some of these can be configured through the registry, but we don't
// support that yet.
//
PVOID MiSessionSpaceEnd;    // 0xC0000000
PVOID MiSessionImageEnd;    // 0xC0000000
PVOID MiSessionImageStart;  // 0xBF800000
PVOID MiSessionViewStart;   // 0xBE000000
PVOID MiSessionPoolEnd;     // 0xBE000000
PVOID MiSessionPoolStart;   // 0xBD000000
PVOID MmSessionBase;        // 0xBD000000
ULONG MmSessionSize;
ULONG MmSessionViewSize;
ULONG MmSessionPoolSize;
ULONG MmSessionImageSize;

//
// The system view space, on the other hand, is where sections that are memory
// mapped into "system space" end up.
//
// By default, it is a 16MB region.
//
PVOID MiSystemViewStart;
ULONG MmSystemViewSize;

//
// A copy of the system page directory (the page directory associated with the
// System process) is kept (double-mapped) by the manager in order to lazily
// map paged pool PDEs into external processes when they fault on a paged pool
// address.
//
PFN_NUMBER MmSystemPageDirectory;
PMMPTE MmSystemPagePtes;

//
// Windows NT seems to choose between 7000, 11000 and 50000
// On systems with more than 32MB, this number is then doubled, and further
// aligned up to a PDE boundary (4MB).
//
ULONG MmNumberOfSystemPtes;

//
// This is how many pages the PFN database will take up
// In Windows, this includes the Quark Color Table, but not in ARM³
//
ULONG MxPfnAllocation;

//
// This structure describes the different pieces of RAM-backed address space
//
PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;

//
// Before we have a PFN database, memory comes straight from our physical memory
// blocks, which is nice because it's guaranteed contiguous and also because once
// we take a page from here, the system doesn't see it anymore.
// However, once the fun is over, those pages must be re-integrated back into
// PFN society life, and that requires us keeping a copy of the original layout
// so that we can parse it later.
//
PMEMORY_ALLOCATION_DESCRIPTOR MxFreeDescriptor;
MEMORY_ALLOCATION_DESCRIPTOR MxOldFreeDescriptor;

//
// This is where we keep track of the most basic physical layout markers
//
ULONG MmNumberOfPhysicalPages, MmHighestPhysicalPage, MmLowestPhysicalPage;

//
// The total number of pages mapped by the boot loader, which include the kernel
// HAL, boot drivers, registry, NLS files and other loader data structures is
// kept track of here. This depends on "LoaderPagesSpanned" being correct when
// coming from the loader.
//
// This number is later aligned up to a PDE boundary.
//
ULONG MmBootImageSize;

//
// These three variables keep track of the core separation of address space that
// exists between kernel mode and user mode.
//
ULONG MmUserProbeAddress;
PVOID MmHighestUserAddress;
PVOID MmSystemRangeStart;

/* PRIVATE FUNCTIONS **********************************************************/

//
// In Bavaria, this is probably a hate crime
//
VOID
FASTCALL
MiSyncARM3WithROS(IN PVOID AddressStart,
                  IN PVOID AddressEnd)
{
    //
    // Puerile piece of junk-grade carbonized horseshit puss sold to the lowest bidder
    //
    ULONG Pde = ADDR_TO_PDE_OFFSET(AddressStart);
    while (Pde <= ADDR_TO_PDE_OFFSET(AddressEnd))
    {
        //
        // This both odious and heinous
        //
        extern ULONG MmGlobalKernelPageDirectory[1024];
        MmGlobalKernelPageDirectory[Pde] = ((PULONG)PDE_BASE)[Pde];
        Pde++;
    }
}

PFN_NUMBER
NTAPI
MxGetNextPage(IN PFN_NUMBER PageCount)
{
    PFN_NUMBER Pfn;
 
    //
    // Make sure we have enough pages
    //
    if (PageCount > MxFreeDescriptor->PageCount)
    {
        //
        // Crash the system
        //
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MxFreeDescriptor->PageCount,
                     MxOldFreeDescriptor.PageCount,
                     PageCount);
    }
    
    //
    // Use our lowest usable free pages
    //
    Pfn = MxFreeDescriptor->BasePage;
    MxFreeDescriptor->BasePage += PageCount;
    MxFreeDescriptor->PageCount -= PageCount;
    return Pfn;
}

PPHYSICAL_MEMORY_DESCRIPTOR
NTAPI
MmInitializeMemoryLimits(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                         IN PBOOLEAN IncludeType)
{
    PLIST_ENTRY NextEntry;
    ULONG Run = 0, InitialRuns = 0;
    PFN_NUMBER NextPage = -1, PageCount = 0;
    PPHYSICAL_MEMORY_DESCRIPTOR Buffer, NewBuffer;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    
    //
    // Scan the memory descriptors
    //
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        //
        // For each one, increase the memory allocation estimate
        //
        InitialRuns++;
        NextEntry = NextEntry->Flink;
    }
    
    //
    // Allocate the maximum we'll ever need
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
                                   sizeof(PHYSICAL_MEMORY_RUN) *
                                   (InitialRuns - 1),
                                   'lMmM');
    if (!Buffer) return NULL;

    //
    // For now that's how many runs we have
    //
    Buffer->NumberOfRuns = InitialRuns;
    
    //
    // Now loop through the descriptors again
    //
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        //
        // Grab each one, and check if it's one we should include
        //
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        if ((MdBlock->MemoryType < LoaderMaximum) &&
            (IncludeType[MdBlock->MemoryType]))
        {
            //
            // Add this to our running total
            //
            PageCount += MdBlock->PageCount;
            
            //
            // Check if the next page is described by the next descriptor
            //            
            if (MdBlock->BasePage == NextPage)
            {
                //
                // Combine it into the same physical run
                //
                ASSERT(MdBlock->PageCount != 0);
                Buffer->Run[Run - 1].PageCount += MdBlock->PageCount;
                NextPage += MdBlock->PageCount;
            }
            else
            {
                //
                // Otherwise just duplicate the descriptor's contents
                //
                Buffer->Run[Run].BasePage = MdBlock->BasePage;
                Buffer->Run[Run].PageCount = MdBlock->PageCount;
                NextPage = Buffer->Run[Run].BasePage + Buffer->Run[Run].PageCount;
                
                //
                // And in this case, increase the number of runs
                //
                Run++;
            }
        }
        
        //
        // Try the next descriptor
        //
        NextEntry = MdBlock->ListEntry.Flink;
    }
    
    //
    // We should not have been able to go past our initial estimate
    //
    ASSERT(Run <= Buffer->NumberOfRuns);

    //
    // Our guess was probably exaggerated...
    //
    if (InitialRuns > Run)
    {
        //
        // Allocate a more accurately sized buffer
        //
        NewBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
                                          sizeof(PHYSICAL_MEMORY_RUN) *
                                          (Run - 1),
                                          'lMmM');
        if (NewBuffer)
        {
            //
            // Copy the old buffer into the new, then free it
            //
            RtlCopyMemory(NewBuffer->Run,
                          Buffer->Run,
                          sizeof(PHYSICAL_MEMORY_RUN) * Run);
            ExFreePool(Buffer);
            
            //
            // Now use the new buffer
            //
            Buffer = NewBuffer;
        }
    }
    
    //
    // Write the final numbers, and return it
    //
    Buffer->NumberOfRuns = Run;
    Buffer->NumberOfPages = PageCount;
    return Buffer;
}

VOID
NTAPI
MiBuildPagedPool(VOID)
{
    PMMPTE PointerPte, PointerPde;
    MMPTE TempPte = HyperTemplatePte;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    ULONG Size, BitMapSize;
    
    //
    // Get the page frame number for the system page directory
    //
    PointerPte = MiAddressToPte(PDE_BASE);
    MmSystemPageDirectory = PFN_FROM_PTE(PointerPte);
    
    //
    // Allocate a system PTE which will hold a copy of the page directory
    //
    PointerPte = MiReserveSystemPtes(1, SystemPteSpace);
    ASSERT(PointerPte);
    MmSystemPagePtes = MiPteToAddress(PointerPte);

    //
    // Make this system PTE point to the system page directory.
    // It is now essentially double-mapped. This will be used later for lazy
    // evaluation of PDEs accross process switches, similarly to how the Global
    // page directory array in the old ReactOS Mm is used (but in a less hacky
    // way).
    //
    TempPte = HyperTemplatePte;
    TempPte.u.Hard.PageFrameNumber = MmSystemPageDirectory;
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(TempPte.u.Hard.Valid == 1);
    *PointerPte = TempPte;

    //
    // Let's get back to paged pool work: size it up.
    // By default, it should be twice as big as nonpaged pool.
    //
    MmSizeOfPagedPoolInBytes = 2 * MmMaximumNonPagedPoolInBytes;
    if (MmSizeOfPagedPoolInBytes > ((ULONG_PTR)MmNonPagedSystemStart -
                                    (ULONG_PTR)MmPagedPoolStart))
    {
        //
        // On the other hand, we have limited VA space, so make sure that the VA
        // for paged pool doesn't overflow into nonpaged pool VA. Otherwise, set
        // whatever maximum is possible.
        //
        MmSizeOfPagedPoolInBytes = (ULONG_PTR)MmNonPagedSystemStart -
                                   (ULONG_PTR)MmPagedPoolStart;
    }

    //
    // Get the size in pages and make sure paged pool is at least 32MB.
    //
    Size = MmSizeOfPagedPoolInBytes;
    if (Size < MI_MIN_INIT_PAGED_POOLSIZE) Size = MI_MIN_INIT_PAGED_POOLSIZE;
    Size = BYTES_TO_PAGES(Size);

    //
    // Now check how many PTEs will be required for these many pages.
    //
    Size = (Size + (1024 - 1)) / 1024;

    //
    // Recompute the page-aligned size of the paged pool, in bytes and pages.
    //
    MmSizeOfPagedPoolInBytes = Size * PAGE_SIZE * 1024;
    MmSizeOfPagedPoolInPages = MmSizeOfPagedPoolInBytes >> PAGE_SHIFT;

    //
    // Let's be really sure this doesn't overflow into nonpaged system VA
    //
    ASSERT((MmSizeOfPagedPoolInBytes + (ULONG_PTR)MmPagedPoolStart) <= 
           (ULONG_PTR)MmNonPagedSystemStart);

    //
    // This is where paged pool ends
    //
    MmPagedPoolEnd = (PVOID)(((ULONG_PTR)MmPagedPoolStart +
                              MmSizeOfPagedPoolInBytes) - 1);

    //
    // So now get the PDE for paged pool and zero it out
    //
    PointerPde = MiAddressToPde(MmPagedPoolStart);
    RtlZeroMemory(PointerPde,
                  (1 + MiAddressToPde(MmPagedPoolEnd) - PointerPde) * sizeof(MMPTE));

    //
    // Next, get the first and last PTE
    //
    PointerPte = MiAddressToPte(MmPagedPoolStart);
    MmPagedPoolInfo.FirstPteForPagedPool = PointerPte;
    MmPagedPoolInfo.LastPteForPagedPool = MiAddressToPte(MmPagedPoolEnd);

    //
    // Lock the PFN database
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    //
    // Allocate a page and map the first paged pool PDE
    //
    PageFrameIndex = MmAllocPage(MC_NPPOOL, 0);
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    ASSERT(PointerPde->u.Hard.Valid == 0);
    ASSERT(TempPte.u.Hard.Valid == 1);
    *PointerPde = TempPte;

    //
    // Release the PFN database lock
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    //
    // We only have one PDE mapped for now... at fault time, additional PDEs
    // will be allocated to handle paged pool growth. This is where they'll have
    // to start.
    //
    MmPagedPoolInfo.NextPdeForPagedPoolExpansion = PointerPde + 1;

    //
    // We keep track of each page via a bit, so check how big the bitmap will
    // have to be (make sure to align our page count such that it fits nicely
    // into a 4-byte aligned bitmap.
    //
    // We'll also allocate the bitmap header itself part of the same buffer.
    //
    Size = Size * 1024;
    ASSERT(Size == MmSizeOfPagedPoolInPages);
    BitMapSize = sizeof(RTL_BITMAP) + (((Size + 31) / 32) * sizeof(ULONG));

    //
    // Allocate the allocation bitmap, which tells us which regions have not yet
    // been mapped into memory
    //
    MmPagedPoolInfo.PagedPoolAllocationMap = ExAllocatePoolWithTag(NonPagedPool,
                                                                   BitMapSize,
                                                                   '  mM');
    ASSERT(MmPagedPoolInfo.PagedPoolAllocationMap);

    //
    // Initialize it such that at first, only the first page's worth of PTEs is
    // marked as allocated (incidentially, the first PDE we allocated earlier).
    //
    RtlInitializeBitMap(MmPagedPoolInfo.PagedPoolAllocationMap,
                        (PULONG)(MmPagedPoolInfo.PagedPoolAllocationMap + 1),
                        BitMapSize);
    RtlSetAllBits(MmPagedPoolInfo.PagedPoolAllocationMap);
    RtlClearBits(MmPagedPoolInfo.PagedPoolAllocationMap, 0, 1024);

    //
    // We have a second bitmap, which keeps track of where allocations end.
    // Given the allocation bitmap and a base address, we can therefore figure
    // out which page is the last page of that allocation, and thus how big the
    // entire allocation is.
    //
    MmPagedPoolInfo.EndOfPagedPoolBitmap = ExAllocatePoolWithTag(NonPagedPool,
                                                                 BitMapSize,
                                                                 '  mM');
    ASSERT(MmPagedPoolInfo.EndOfPagedPoolBitmap);
    RtlInitializeBitMap(MmPagedPoolInfo.EndOfPagedPoolBitmap,
                        (PULONG)(MmPagedPoolInfo.EndOfPagedPoolBitmap + 1),
                        BitMapSize);

    //
    // Since no allocations have been made yet, there are no bits set as the end
    //
    RtlClearAllBits(MmPagedPoolInfo.EndOfPagedPoolBitmap);

    //
    // Initialize paged pool.
    //
    //InitializePool(PagedPool, 0);
}

NTSTATUS
NTAPI
MmArmInitSystem(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    ULONG FreePages = 0;
    PMEMORY_AREA MArea;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    PFN_NUMBER PageFrameIndex;
    PMMPTE StartPde, EndPde, PointerPte, LastPte;
    MMPTE TempPde = HyperTemplatePte, TempPte = HyperTemplatePte;
    PVOID NonPagedPoolExpansionVa, BaseAddress;
    NTSTATUS Status;
    ULONG OldCount;
    BOOLEAN IncludeType[LoaderMaximum];
    ULONG i;
    BoundaryAddressMultiple.QuadPart = 0;
    
    if (Phase == 0)
    {
        //
        // Define the basic user vs. kernel address space separation
        //
        MmSystemRangeStart = (PVOID)KSEG0_BASE;
        MmUserProbeAddress = (ULONG_PTR)MmSystemRangeStart - 0x10000;
        MmHighestUserAddress = (PVOID)(MmUserProbeAddress - 1);
        
        //
        // Get the size of the boot loader's image allocations and then round
        // that region up to a PDE size, so that any PDEs we might create for
        // whatever follows are separate from the PDEs that boot loader might've
        // already created (and later, we can blow all that away if we want to).
        //
        MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned;
        MmBootImageSize *= PAGE_SIZE;
        MmBootImageSize = (MmBootImageSize + (4 * 1024 * 1024) - 1) & ~((4 * 1024 * 1024) - 1);
        ASSERT((MmBootImageSize % (4 * 1024 * 1024)) == 0);
        
        //
        // Set the size of session view, pool, and image
        //
        MmSessionSize = MI_SESSION_SIZE;
        MmSessionViewSize = MI_SESSION_VIEW_SIZE;
        MmSessionPoolSize = MI_SESSION_POOL_SIZE;
        MmSessionImageSize = MI_SESSION_IMAGE_SIZE;
        
        //
        // Set the size of system view
        //
        MmSystemViewSize = MI_SYSTEM_VIEW_SIZE;
        
        //
        // This is where it all ends
        //
        MiSessionImageEnd = (PVOID)PTE_BASE;
        
        //
        // This is where we will load Win32k.sys and the video driver
        //
        MiSessionImageStart = (PVOID)((ULONG_PTR)MiSessionImageEnd -
                                      MmSessionImageSize);
        
        //
        // So the view starts right below the session working set (itself below
        // the image area)
        //
        MiSessionViewStart = (PVOID)((ULONG_PTR)MiSessionImageEnd -
                                     MmSessionImageSize -
                                     MI_SESSION_WORKING_SET_SIZE - 
                                     MmSessionViewSize);
        
        //
        // Session pool follows
        //
        MiSessionPoolEnd = MiSessionViewStart;
        MiSessionPoolStart = (PVOID)((ULONG_PTR)MiSessionPoolEnd -
                                     MmSessionPoolSize);
        
        //
        // And it all begins here
        //
        MmSessionBase = MiSessionPoolStart;
        
        //
        // Sanity check that our math is correct
        //
        ASSERT((ULONG_PTR)MmSessionBase + MmSessionSize == PTE_BASE);
        
        //
        // Session space ends wherever image session space ends
        //
        MiSessionSpaceEnd = MiSessionImageEnd;
        
        //
        // System view space ends at session space, so now that we know where
        // this is, we can compute the base address of system view space itself.
        //
        MiSystemViewStart = (PVOID)((ULONG_PTR)MmSessionBase -
                                    MmSystemViewSize);
        
        //
        // Set CR3 for the system process
        //
        PointerPte = MiAddressToPde(PTE_BASE);
        PageFrameIndex = PFN_FROM_PTE(PointerPte) << PAGE_SHIFT;
        PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PageFrameIndex;
        
        //
        // Blow away user-mode
        //
        StartPde = MiAddressToPde(0);
        EndPde = MiAddressToPde(KSEG0_BASE);
        RtlZeroMemory(StartPde, (EndPde - StartPde) * sizeof(MMPTE));
        
        //
        // Loop the memory descriptors
        //
        NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
        {
            //
            // Get the memory block
            //
            MdBlock = CONTAINING_RECORD(NextEntry,
                                        MEMORY_ALLOCATION_DESCRIPTOR,
                                        ListEntry);
            
            //
            // Skip invisible memory
            //
            if ((MdBlock->MemoryType != LoaderFirmwarePermanent) &&
                (MdBlock->MemoryType != LoaderSpecialMemory) &&
                (MdBlock->MemoryType != LoaderHALCachedMemory) &&
                (MdBlock->MemoryType != LoaderBBTMemory))
            {
                //
                // Check if BURNMEM was used
                //
                if (MdBlock->MemoryType != LoaderBad)
                {
                    //
                    // Count this in the total of pages
                    //
                    MmNumberOfPhysicalPages += MdBlock->PageCount;
                }
                
                //
                // Check if this is the new lowest page
                //
                if (MdBlock->BasePage < MmLowestPhysicalPage)
                {
                    //
                    // Update the lowest page
                    //
                    MmLowestPhysicalPage = MdBlock->BasePage;
                }
                
                //
                // Check if this is the new highest page
                //
                PageFrameIndex = MdBlock->BasePage + MdBlock->PageCount;
                if (PageFrameIndex > MmHighestPhysicalPage)
                {
                    //
                    // Update the highest page
                    //
                    MmHighestPhysicalPage = PageFrameIndex - 1;
                }
                
                //
                // Check if this is free memory
                //
                if ((MdBlock->MemoryType == LoaderFree) ||
                    (MdBlock->MemoryType == LoaderLoadedProgram) ||
                    (MdBlock->MemoryType == LoaderFirmwareTemporary) ||
                    (MdBlock->MemoryType == LoaderOsloaderStack))
                {
                    //
                    // Check if this is the largest memory descriptor
                    //
                    if (MdBlock->PageCount > FreePages)
                    {
                        //
                        // For now, it is
                        //
                        FreePages = MdBlock->PageCount;
                        MxFreeDescriptor = MdBlock;
                    }
                }
            }
            
            //
            // Keep going
            //
            NextEntry = MdBlock->ListEntry.Flink;
        }
        
        //
        // Save original values of the free descriptor, since it'll be
        // altered by early allocations
        //
        MxOldFreeDescriptor = *MxFreeDescriptor;
        
        //
        // Check if this is a machine with less than 19MB of RAM
        //
        if (MmNumberOfPhysicalPages < MI_MIN_PAGES_FOR_SYSPTE_TUNING)
        {
            //
            // Use the very minimum of system PTEs
            //
            MmNumberOfSystemPtes = 7000;
        }
        else
        {
            //
            // Use the default, but check if we have more than 32MB of RAM
            //
            MmNumberOfSystemPtes = 11000;
            if (MmNumberOfPhysicalPages > MI_MIN_PAGES_FOR_SYSPTE_BOOST)
            {
                //
                // Double the amount of system PTEs
                //
                MmNumberOfSystemPtes <<= 1;
            }
        }
        
        DPRINT("System PTE count has been tuned to %d (%d bytes)\n",
               MmNumberOfSystemPtes, MmNumberOfSystemPtes * PAGE_SIZE);
        
        //
        // Check if this is a machine with less than 256MB of RAM, and no overide
        //
        if ((MmNumberOfPhysicalPages <= MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING) &&
            !(MmSizeOfNonPagedPoolInBytes))
        {
            //
            // Force the non paged pool to be 2MB so we can reduce RAM usage
            //
            MmSizeOfNonPagedPoolInBytes = 2 * 1024 * 1024;
        }
        
        //
        // Check if the user gave a ridicuously large nonpaged pool RAM size
        //
        if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
            (MmNumberOfPhysicalPages * 7 / 8))
        {
            //
            // More than 7/8ths of RAM was dedicated to nonpaged pool, ignore!
            //
            MmSizeOfNonPagedPoolInBytes = 0;
        }
        
        //
        // Check if no registry setting was set, or if the setting was too low
        //
        if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize)
        {
            //
            // Start with the minimum (256 KB) and add 32 KB for each MB above 4
            //
            MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;
            MmSizeOfNonPagedPoolInBytes += (MmNumberOfPhysicalPages - 1024) /
                                           256 * MmMinAdditionNonPagedPoolPerMb;
        }
        
        //
        // Check if the registy setting or our dynamic calculation was too high
        //
        if (MmSizeOfNonPagedPoolInBytes > MI_MAX_INIT_NONPAGED_POOL_SIZE)
        {
            //
            // Set it to the maximum
            //
            MmSizeOfNonPagedPoolInBytes = MI_MAX_INIT_NONPAGED_POOL_SIZE;
        }
        
        //
        // Check if a percentage cap was set through the registry
        //
        if (MmMaximumNonPagedPoolPercent)
        {
            //
            // Don't feel like supporting this right now
            //
            UNIMPLEMENTED;
        }
        
        //
        // Page-align the nonpaged pool size
        //
        MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);
        
        //
        // Now, check if there was a registry size for the maximum size
        //
        if (!MmMaximumNonPagedPoolInBytes)
        {
            //
            // Start with the default (1MB) and add 400 KB for each MB above 4
            //
            MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;
            MmMaximumNonPagedPoolInBytes += (MmNumberOfPhysicalPages - 1024) /
                                             256 * MmMaxAdditionNonPagedPoolPerMb;
        }
        
        //
        // Don't let the maximum go too high
        //
        if (MmMaximumNonPagedPoolInBytes > MI_MAX_NONPAGED_POOL_SIZE)
        {
            //
            // Set it to the upper limit
            //
            MmMaximumNonPagedPoolInBytes = MI_MAX_NONPAGED_POOL_SIZE;
        }
        
        //
        // Calculate the number of bytes, and then convert to pages
        //
        MxPfnAllocation = (MmHighestPhysicalPage + 1) * sizeof(MMPFN);
        MxPfnAllocation >>= PAGE_SHIFT;
        
        //
        // We have to add one to the count here, because in the process of
        // shifting down to the page size, we actually ended up getting the
        // lower aligned size (so say, 0x5FFFF bytes is now 0x5F pages).
        // Later on, we'll shift this number back into bytes, which would cause
        // us to end up with only 0x5F000 bytes -- when we actually want to have
        // 0x60000 bytes.
        //
        MxPfnAllocation++;
        
        //
        // Now calculate the nonpaged pool expansion VA region
        //
        MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmNonPagedPoolEnd -
                                      MmMaximumNonPagedPoolInBytes +
                                      MmSizeOfNonPagedPoolInBytes);
        MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);
        NonPagedPoolExpansionVa = MmNonPagedPoolStart;
        DPRINT("NP Pool has been tuned to: %d bytes and %d bytes\n",
               MmSizeOfNonPagedPoolInBytes, MmMaximumNonPagedPoolInBytes);
        
        //
        // Now calculate the nonpaged system VA region, which includes the
        // nonpaged pool expansion (above) and the system PTEs. Note that it is
        // then aligned to a PDE boundary (4MB).
        //
        MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedPoolStart -
                                        (MmNumberOfSystemPtes + 1) * PAGE_SIZE);
        MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedSystemStart &
                                        ~((4 * 1024 * 1024) - 1));
        
        //
        // Don't let it go below the minimum
        //
        if (MmNonPagedSystemStart < (PVOID)0xEB000000)
        {
            //
            // This is a hard-coded limit in the Windows NT address space
            //
            MmNonPagedSystemStart = (PVOID)0xEB000000;
            
            //
            // Reduce the amount of system PTEs to reach this point
            //
            MmNumberOfSystemPtes = ((ULONG_PTR)MmNonPagedPoolStart -
                                    (ULONG_PTR)MmNonPagedSystemStart) >>
                                    PAGE_SHIFT;
            MmNumberOfSystemPtes--;
            ASSERT(MmNumberOfSystemPtes > 1000);
        }
        
        //
        // Normally, the PFN database should start after the loader images.
        // This is already the case in ReactOS, but for now we want to co-exist
        // with the old memory manager, so we'll create a "Shadow PFN Database"
        // instead, and arbitrarly start it at 0xB0000000.
        //
        MmPfnDatabase = (PVOID)0xB0000000;
        ASSERT(((ULONG_PTR)MmPfnDatabase & ((4 * 1024 * 1024) - 1)) == 0);
                
        //
        // Non paged pool comes after the PFN database
        //
        MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmPfnDatabase +
                                      (MxPfnAllocation << PAGE_SHIFT));

        //
        // Now we actually need to get these many physical pages. Nonpaged pool
        // is actually also physically contiguous (but not the expansion)
        //
        PageFrameIndex = MxGetNextPage(MxPfnAllocation +
                                       (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT));
        ASSERT(PageFrameIndex != 0);
        DPRINT("PFN DB PA PFN begins at: %lx\n", PageFrameIndex);
        DPRINT("NP PA PFN begins at: %lx\n", PageFrameIndex + MxPfnAllocation);

        //
        // Now we need some pages to create the page tables for the NP system VA
        // which includes system PTEs and expansion NP
        //
        StartPde = MiAddressToPde(MmNonPagedSystemStart);
        EndPde = MiAddressToPde((PVOID)((ULONG_PTR)MmNonPagedPoolEnd - 1));
        while (StartPde <= EndPde)
        {
            //
            // Sanity check
            //
            ASSERT(StartPde->u.Hard.Valid == 0);
            
            //
            // Get a page
            //
            TempPde.u.Hard.PageFrameNumber = MxGetNextPage(1);
            ASSERT(TempPde.u.Hard.Valid == 1);
            *StartPde = TempPde;
            
            //
            // Zero out the page table
            //
            PointerPte = MiPteToAddress(StartPde);
            RtlZeroMemory(PointerPte, PAGE_SIZE);
            
            //
            // Next
            //
            StartPde++;
        }

        //
        // Now we need pages for the page tables which will map initial NP
        //
        StartPde = MiAddressToPde(MmPfnDatabase);
        EndPde = MiAddressToPde((PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                        MmSizeOfNonPagedPoolInBytes - 1));
        while (StartPde <= EndPde)
        {
            //
            // Sanity check
            //
            ASSERT(StartPde->u.Hard.Valid == 0);
            
            //
            // Get a page
            //
            TempPde.u.Hard.PageFrameNumber = MxGetNextPage(1);
            ASSERT(TempPde.u.Hard.Valid == 1);
            *StartPde = TempPde;
            
            //
            // Zero out the page table
            //
            PointerPte = MiPteToAddress(StartPde);
            RtlZeroMemory(PointerPte, PAGE_SIZE);
            
            //
            // Next
            //
            StartPde++;
        }

        //
        // Now remember where the expansion starts
        //
        MmNonPagedPoolExpansionStart = NonPagedPoolExpansionVa;

        //
        // Last step is to actually map the nonpaged pool
        //
        PointerPte = MiAddressToPte(MmNonPagedPoolStart);
        LastPte = MiAddressToPte((PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                         MmSizeOfNonPagedPoolInBytes - 1));
        while (PointerPte <= LastPte)
        {
            //
            // Use one of our contigous pages
            //
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex++;
            ASSERT(PointerPte->u.Hard.Valid == 0);
            ASSERT(TempPte.u.Hard.Valid == 1);
            *PointerPte++ = TempPte;
        }
        
        //
        // ReactOS requires a memory area to keep the initial NP area off-bounds
        //
        BaseAddress = MmNonPagedPoolStart;
        Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                    MEMORY_AREA_SYSTEM | MEMORY_AREA_STATIC,
                                    &BaseAddress,
                                    MmSizeOfNonPagedPoolInBytes,
                                    PAGE_READWRITE,
                                    &MArea,
                                    TRUE,
                                    0,
                                    BoundaryAddressMultiple);
        ASSERT(Status == STATUS_SUCCESS);
        
        //
        // And we need one more for the system NP
        //
        BaseAddress = MmNonPagedSystemStart;
        Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                    MEMORY_AREA_SYSTEM | MEMORY_AREA_STATIC,
                                    &BaseAddress,
                                    (ULONG_PTR)MmNonPagedPoolEnd -
                                    (ULONG_PTR)MmNonPagedSystemStart,
                                    PAGE_READWRITE,
                                    &MArea,
                                    TRUE,
                                    0,
                                    BoundaryAddressMultiple);
        ASSERT(Status == STATUS_SUCCESS);
        
        //
        // Sanity check: make sure we have properly defined the system PTE space
        //
        ASSERT(MiAddressToPte(MmNonPagedSystemStart) <
               MiAddressToPte(MmNonPagedPoolExpansionStart));
        
        //
        // Now go ahead and initialize the ARM³ nonpaged pool
        //
        MiInitializeArmPool();
    }
    else if (Phase == 1) // IN BETWEEN, THE PFN DATABASE IS NOW CREATED
    {        
        //
        // Initialize the nonpaged pool
        //
        InitializePool(NonPagedPool, 0);
        
        //
        // We PDE-aligned the nonpaged system start VA, so haul some extra PTEs!
        //
        PointerPte = MiAddressToPte(MmNonPagedSystemStart);
        OldCount = MmNumberOfSystemPtes;
        MmNumberOfSystemPtes = MiAddressToPte(MmNonPagedPoolExpansionStart) -
                               PointerPte;
        MmNumberOfSystemPtes--;
        DPRINT("Final System PTE count: %d (%d bytes)\n",
               MmNumberOfSystemPtes, MmNumberOfSystemPtes * PAGE_SIZE);
        
        //
        // Create the system PTE space
        //
        MiInitializeSystemPtes(PointerPte, MmNumberOfSystemPtes, SystemPteSpace);
        
        //
        // Get the PDE For hyperspace
        //
        StartPde = MiAddressToPde(HYPER_SPACE);
        
        //
        // Allocate a page for it and create it
        //
        PageFrameIndex = MmAllocPage(MC_SYSTEM, 0);
        TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
        TempPde.u.Hard.Global = FALSE; // Hyperspace is local!
        ASSERT(StartPde->u.Hard.Valid == 0);
        ASSERT(TempPde.u.Hard.Valid == 1);
        *StartPde = TempPde;
        
        //
        // Zero out the page table now
        //
        PointerPte = MiAddressToPte(HYPER_SPACE);
        RtlZeroMemory(PointerPte, PAGE_SIZE);
        
        //
        // Setup the mapping PTEs
        //
        MmFirstReservedMappingPte = MiAddressToPte(MI_MAPPING_RANGE_START);
        MmLastReservedMappingPte = MiAddressToPte(MI_MAPPING_RANGE_END);
        MmFirstReservedMappingPte->u.Hard.PageFrameNumber = MI_HYPERSPACE_PTES;

        //
        // Reserve system PTEs for zeroing PTEs and clear them
        //
        MiFirstReservedZeroingPte = MiReserveSystemPtes(MI_ZERO_PTES,
                                                        SystemPteSpace);
        RtlZeroMemory(MiFirstReservedZeroingPte, MI_ZERO_PTES * sizeof(MMPTE));
        
        //
        // Set the counter to maximum to boot with
        //
        MiFirstReservedZeroingPte->u.Hard.PageFrameNumber = MI_ZERO_PTES - 1;
        
        //
        // Sync us up with ReactOS Mm
        //
        MiSyncARM3WithROS(MmNonPagedSystemStart, (PVOID)((ULONG_PTR)MmNonPagedPoolEnd - 1));
        MiSyncARM3WithROS(MmPfnDatabase, (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes - 1));
        MiSyncARM3WithROS((PVOID)HYPER_SPACE, (PVOID)(HYPER_SPACE + PAGE_SIZE - 1));
    }
    else // NOW WE HAVE NONPAGED POOL
    {
        //
        // Instantiate memory that we don't consider RAM/usable
        // We use the same exclusions that Windows does, in order to try to be
        // compatible with WinLDR-style booting
        //
        for (i = 0; i < LoaderMaximum; i++) IncludeType[i] = TRUE;
        IncludeType[LoaderBad] = FALSE;
        IncludeType[LoaderFirmwarePermanent] = FALSE;
        IncludeType[LoaderSpecialMemory] = FALSE;
        IncludeType[LoaderBBTMemory] = FALSE;
        
        //
        // Build the physical memory block
        //
        MmPhysicalMemoryBlock = MmInitializeMemoryLimits(LoaderBlock,
                                                         IncludeType);
        for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i++)
        {
            //
            // Dump it for debugging
            //
            PPHYSICAL_MEMORY_RUN Run;
            Run = &MmPhysicalMemoryBlock->Run[i];
            DPRINT("PHYSICAL RAM [0x%08p to 0x%08p]\n",
                   Run->BasePage << PAGE_SHIFT,
                   (Run->BasePage + Run->PageCount) << PAGE_SHIFT);
        }
        
        //
        // Size up paged pool and build the shadow system page directory
        //
        MiBuildPagedPool();
        
        //
        // Print the memory layout
        //
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmSystemRangeStart,
                (ULONG_PTR)MmSystemRangeStart + MmBootImageSize,
                "Boot Loaded Image");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MiNonPagedPoolStart,
                (ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength,
                "Non Paged Pool");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmPagedPoolBase,
                (ULONG_PTR)MmPagedPoolBase + MmPagedPoolSize,
                "Paged Pool");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmPfnDatabase,
                (ULONG_PTR)MmPfnDatabase + (MxPfnAllocation << PAGE_SHIFT),
                "PFN Database");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmNonPagedPoolStart,
                (ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes,
                "ARM³ Non Paged Pool");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MiSystemViewStart,
                (ULONG_PTR)MiSystemViewStart + MmSystemViewSize,
                "System View Space");        
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmSessionBase,
                MiSessionSpaceEnd,
                "Session Space");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                PTE_BASE, PDE_BASE,
                "Page Tables");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                PDE_BASE, HYPER_SPACE,
                "Page Directories");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                HYPER_SPACE, HYPER_SPACE + (4 * 1024 * 1024),
                "Hyperspace");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmPagedPoolStart,
                (ULONG_PTR)MmPagedPoolStart + MmSizeOfPagedPoolInBytes,
                "ARM³ Paged Pool");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmNonPagedSystemStart, MmNonPagedPoolExpansionStart,
                "System PTE Space");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmNonPagedPoolExpansionStart, MmNonPagedPoolEnd,
                "Non Paged Pool Expansion PTE Space");
    }
    
    //
    // Always return success for now
    //
    return STATUS_SUCCESS;
}

/* EOF */
