/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mdl.c
 * PURPOSE:         Manipulates MDLs
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeMdlImplementation)
#endif

/* GLOBALS *******************************************************************/

#define TAG_MDL    TAG('M', 'D', 'L', ' ')
#define MI_MDL_MAPPING_REGION_SIZE       (256*1024*1024)

PVOID MiMdlMappingRegionBase = NULL;
RTL_BITMAP MiMdlMappingRegionAllocMap;
ULONG MiMdlMappingRegionHint;
KSPIN_LOCK MiMdlMappingRegionLock;
extern ULONG MmPageArraySize;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
INIT_FUNCTION
NTAPI
MmInitializeMdlImplementation(VOID)
{
    MEMORY_AREA* Result;
    NTSTATUS Status;
    PVOID Buffer;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    
    BoundaryAddressMultiple.QuadPart = 0;
    MiMdlMappingRegionHint = 0;
    MiMdlMappingRegionBase = NULL;
    
    MmLockAddressSpace(MmGetKernelAddressSpace());
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_MDL_MAPPING,
                                &MiMdlMappingRegionBase,
                                MI_MDL_MAPPING_REGION_SIZE,
                                PAGE_READWRITE,
                                &Result,
                                FALSE,
                                0,
                                BoundaryAddressMultiple);
    if (!NT_SUCCESS(Status))
    {
        MmUnlockAddressSpace(MmGetKernelAddressSpace());
        ASSERT(FALSE);
    }
    MmUnlockAddressSpace(MmGetKernelAddressSpace());
    
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   MI_MDL_MAPPING_REGION_SIZE / (PAGE_SIZE * 8),
                                   TAG_MDL);
    
    RtlInitializeBitMap(&MiMdlMappingRegionAllocMap, Buffer, MI_MDL_MAPPING_REGION_SIZE / PAGE_SIZE);
    RtlClearAllBits(&MiMdlMappingRegionAllocMap);
    
    KeInitializeSpinLock(&MiMdlMappingRegionLock);
}

/* PUBLIC FUNCTIONS ***********************************************************/


/*
 * @implemented
 */
PMDL
NTAPI
MmCreateMdl(IN PMDL Mdl,
            IN PVOID Base,
            IN ULONG Length)
{
    ULONG Size;
    
    /* Check if we don't have an MDL built */
    if (!Mdl)
    {
        /* Calcualte the size we'll need  and allocate the MDL */
        Size = MmSizeOfMdl(Base, Length);
        Mdl = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_MDL);
        if (!Mdl) return NULL;
    }
    
    /* Initialize it */
    MmInitializeMdl(Mdl, Base, Length);
    DPRINT("Creating MDL: %p\n", Mdl);
    DPRINT("Base: %p. Length: %lx\n", Base, Length);
    return Mdl;
}

/*
 * @implemented
 */
ULONG
NTAPI
MmSizeOfMdl(IN PVOID Base,
            IN ULONG Length)
{
    /* Return the MDL size */
    return sizeof(MDL) + (ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Length) * sizeof(PFN_NUMBER));
}

/*
 * @implemented
 */
VOID
NTAPI
MmBuildMdlForNonPagedPool(IN PMDL Mdl)
{
    ULONG i;
    ULONG PageCount;
    PPFN_NUMBER MdlPages;
    PVOID Base;
    DPRINT("Building MDL: %p\n", Mdl);
    
    /* Sanity checks */
    ASSERT(Mdl->ByteCount != 0);
    ASSERT((Mdl->MdlFlags & (MDL_PAGES_LOCKED |
                             MDL_MAPPED_TO_SYSTEM_VA |
                             MDL_SOURCE_IS_NONPAGED_POOL |
                             MDL_PARTIAL)) == 0);
    
    /* We know the MDL isn't associated to a process now */
    Mdl->Process = NULL;
    
    /* Get page and VA information */
    MdlPages = (PPFN_NUMBER)(Mdl + 1);
    Base = Mdl->StartVa;
    
    /* Set the system address and now get the page count */
    Mdl->MappedSystemVa = (PVOID)((ULONG_PTR)Base + Mdl->ByteOffset);
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Mdl->MappedSystemVa, Mdl->ByteCount);
    ASSERT(PageCount != 0);
    
    /* Go through each page */
    for (i = 0; i < PageCount; i++)
    {
        /* Map it */
        *MdlPages++ = MmGetPfnForProcess(NULL,
                                         (PVOID)((ULONG_PTR)Base + (i * PAGE_SIZE)));
    }
    
    /* Set the final flag */
    Mdl->MdlFlags |= MDL_SOURCE_IS_NONPAGED_POOL;
}

/*
 * @implemented
 */
VOID
NTAPI
MmFreePagesFromMdl(IN PMDL Mdl)
{
    PVOID Base;
    PPFN_NUMBER Pages;
    LONG NumberOfPages;
    DPRINT("Freeing MDL: %p\n", Mdl);
    
    /* Sanity checks */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT((Mdl->MdlFlags & MDL_IO_SPACE) == 0);
    ASSERT(((ULONG_PTR)Mdl->StartVa & (PAGE_SIZE - 1)) == 0);
    
    /* Get address and page information */
    Base = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);
    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Mdl->ByteCount);
    
    /* Loop all the MDL pages */
    Pages = (PPFN_NUMBER)(Mdl + 1);
    while (--NumberOfPages >= 0)
    {
        /* Dereference each one of them */
        MmDereferencePage(Pages[NumberOfPages]);
    }
    
    /* Remove the pages locked flag */
    Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
}

/*
 * @implemented
 */
PVOID
NTAPI
MmMapLockedPages(IN PMDL Mdl,
                 IN KPROCESSOR_MODE AccessMode)
{
    /* Call the extended version */
    return MmMapLockedPagesSpecifyCache(Mdl,
                                        AccessMode,
                                        MmCached,
                                        NULL,
                                        TRUE,
                                        HighPagePriority);
}

/*
 * @implemented
 */
VOID
NTAPI
MmUnlockPages(IN PMDL Mdl)
{
    ULONG i;
    PPFN_NUMBER MdlPages;
    PFN_NUMBER Page;
    PEPROCESS Process;
    PVOID Base;
    ULONG Flags, PageCount;
    DPRINT("Unlocking MDL: %p\n", Mdl);
    
    /* Sanity checks */
    ASSERT((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0);
    ASSERT((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0);
    ASSERT((Mdl->MdlFlags & MDL_PARTIAL) == 0);
    ASSERT(Mdl->ByteCount != 0);
    
    /* Get the process associated and capture the flags which are volatile */
    Process = Mdl->Process;
    Flags = Mdl->MdlFlags;
    
    /* Automagically undo any calls to MmGetSystemAddressForMdl's for this mdl */
    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        /* Unmap the pages from system spage */
        MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
    }
    
    /* Get the page count */
    MdlPages = (PPFN_NUMBER)(Mdl + 1);
    Base = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);
    PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Mdl->ByteCount);
    ASSERT(PageCount != 0);
    
    /* We don't support AWE */
    if (Flags & MDL_DESCRIBES_AWE) ASSERT(FALSE);    
    
    /* Check if the buffer is mapped I/O space */
    if (Flags & MDL_IO_SPACE)
    {
        /* Check if this was a wirte */
        if (Flags & MDL_WRITE_OPERATION)
        {
            /* Windows keeps track of the modified bit */
        }
        
        /* Check if we have a process */
        if (Process)
        {
            /* Handle the accounting of locked pages */
            /* ASSERT(Process->NumberOfLockedPages >= 0); */ // always true
            InterlockedExchangeAddSizeT(&Process->NumberOfLockedPages,
                                        -PageCount);
        }
        
        /* We're done */
        Mdl->MdlFlags &= ~MDL_IO_SPACE;
        Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
        return;
    }
    
    /* Check if we have a process */
    if (Process)
    {
        /* Handle the accounting of locked pages */
        /* ASSERT(Process->NumberOfLockedPages >= 0); */ // always true
        InterlockedExchangeAddSizeT(&Process->NumberOfLockedPages,
                                    -PageCount);
    }
    
    /* Scan each page */
    for (i = 0; i < PageCount; i++)
    {
        /* Get the page entry */
        
        /* Unlock and dereference it */
        Page = MdlPages[i];
        MmUnlockPage(Page);
        MmDereferencePage(Page);
    }
    
    /* We're done */
    Mdl->MdlFlags &= ~MDL_PAGES_LOCKED;
}

/*
 * @implemented
 */
VOID
NTAPI
MmUnmapLockedPages(IN PVOID BaseAddress,
                   IN PMDL Mdl)
{
    KIRQL oldIrql;
    ULONG i, PageCount;
    ULONG Base;
    MEMORY_AREA *MemoryArea;
    DPRINT("Unmapping MDL: %p\n", Mdl);
    DPRINT("Base: %p\n", BaseAddress);
    
    /* Sanity check */
    ASSERT(Mdl->ByteCount != 0);
    
    /* Check if this is a kernel request */
    if (BaseAddress > MM_HIGHEST_USER_ADDRESS)
    {
        /* Get base and count information */
        Base = (ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset;
        PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Mdl->ByteCount);

        /* Sanity checks */
        ASSERT((Mdl->MdlFlags & MDL_PARENT_MAPPED_SYSTEM_VA) == 0);
        ASSERT(PageCount != 0);
        ASSERT(Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA);
        
        /* ReactOS does not support this flag */
        if (Mdl->MdlFlags & MDL_FREE_EXTRA_PTES) ASSERT(FALSE);
        
        /* Remove flags */
        Mdl->MdlFlags &= ~(MDL_MAPPED_TO_SYSTEM_VA |
                           MDL_PARTIAL_HAS_BEEN_MAPPED |
                           MDL_FREE_EXTRA_PTES);
        
        /* If we came from non-paged pool, on ReactOS, we can leave */
        if (Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) return;
        
        /* Loop each page */
        BaseAddress = PAGE_ALIGN(BaseAddress);
        for (i = 0; i < PageCount; i++)
        {
            /* Delete it */
            MmDeleteVirtualMapping(NULL,
                                   (PVOID)((ULONG_PTR)BaseAddress + (i * PAGE_SIZE)),
                                   FALSE,
                                   NULL,
                                   NULL);
        }
        
        /* Lock the mapping region */
        KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);
        
        /* Deallocate all the pages used. */
        Base = ((ULONG_PTR)BaseAddress - (ULONG_PTR)MiMdlMappingRegionBase) / PAGE_SIZE;
        RtlClearBits(&MiMdlMappingRegionAllocMap, Base, PageCount);
        MiMdlMappingRegionHint = min(MiMdlMappingRegionHint, Base);
        
        /* Release the lock */
        KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);
    }
    else
    {
        /* Sanity check */
        ASSERT(Mdl->Process == PsGetCurrentProcess());
        
        /* Find the memory area */
        MemoryArea = MmLocateMemoryAreaByAddress(&Mdl->Process->VadRoot,
                                                 BaseAddress);
        ASSERT(MemoryArea);

        /* Free it */
        MmFreeMemoryArea(&Mdl->Process->VadRoot,
                         MemoryArea,
                         NULL,
                         NULL);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
MmProbeAndLockPages(IN PMDL Mdl,
                    IN KPROCESSOR_MODE AccessMode,
                    IN LOCK_OPERATION Operation)
{
    PPFN_TYPE MdlPages;
    PVOID Base, Address;
    ULONG i, j;
    ULONG NrPages;
    NTSTATUS Status = STATUS_SUCCESS;
    PFN_TYPE Page;
    PEPROCESS CurrentProcess;
    PETHREAD Thread;
    PMM_AVL_TABLE AddressSpace;
	KIRQL OldIrql = KeGetCurrentIrql();
    DPRINT("Probing MDL: %p\n", Mdl);
    
    /* Sanity checks */
    ASSERT(Mdl->ByteCount != 0);
    ASSERT(((ULONG)Mdl->ByteOffset & ~(PAGE_SIZE - 1)) == 0);
    ASSERT(((ULONG_PTR)Mdl->StartVa & (PAGE_SIZE - 1)) == 0);
    ASSERT((Mdl->MdlFlags & (MDL_PAGES_LOCKED |
                             MDL_MAPPED_TO_SYSTEM_VA |
                             MDL_SOURCE_IS_NONPAGED_POOL |
                             MDL_PARTIAL |
                             MDL_IO_SPACE)) == 0);
    
    /* Get page and base information */
    MdlPages = (PPFN_NUMBER)(Mdl + 1);
    Base = (PVOID)Mdl->StartVa;
    Address = (PVOID)((ULONG_PTR)Base + Mdl->ByteOffset);
    NrPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Address, Mdl->ByteCount);
    ASSERT(NrPages != 0);

    /* Check if this is an MDL in I/O Space */
    if (Mdl->StartVa >= MmSystemRangeStart &&
        MmGetPfnForProcess(NULL, Mdl->StartVa) >= MmPageArraySize)
    {
        /* Just loop each page */
        for (i = 0; i < NrPages; i++)
        {
            /* And map it */
            MdlPages[i] = MmGetPfnForProcess(NULL,
                                             (PVOID)((ULONG_PTR)Mdl->StartVa + (i * PAGE_SIZE)));
        }
        
        /* Set the flags and exit */
        Mdl->MdlFlags |= MDL_PAGES_LOCKED|MDL_IO_SPACE;
        return;
    }
    
    /* Get the thread and process */
    Thread = PsGetCurrentThread();
    if (Address <= MM_HIGHEST_USER_ADDRESS)
    {
        /* Get the process */
        CurrentProcess = PsGetCurrentProcess();
    }
    else
    {
        /* No process */
        CurrentProcess = NULL;
    }

    /* Check what kind of operaiton this is */
    if (Operation != IoReadAccess)
    {
        /* Set the write flag */
        Mdl->MdlFlags |= MDL_WRITE_OPERATION;
    }
    else
    {
        /* Remove the write flag */
        Mdl->MdlFlags &= ~(MDL_WRITE_OPERATION);
    }
    
    /* Check if this came from kernel mode */
    if (Base >= MM_HIGHEST_USER_ADDRESS)
    {
        /* We should not have a process */
        ASSERT(CurrentProcess == NULL);
        Mdl->Process = NULL;
        AddressSpace = MmGetKernelAddressSpace();
    }
    else
    {
        /* Sanity checks */
        ASSERT(NrPages != 0);
        ASSERT(CurrentProcess == PsGetCurrentProcess());
        
        /* Track locked pages */
        InterlockedExchangeAddSizeT(&CurrentProcess->NumberOfLockedPages,
                                    NrPages);
        
        /* Save the process */
        Mdl->Process = CurrentProcess;
        
        /* Use the process lock */
        AddressSpace = &CurrentProcess->VadRoot;
    }
    
    
    /*
     * Lock the pages
     */
	if (OldIrql < DISPATCH_LEVEL)
		MmLockAddressSpace(AddressSpace);
	else
		MmAcquirePageListLock(&OldIrql);
    
    for (i = 0; i < NrPages; i++)
    {
        PVOID Address;
        
        Address = (char*)Mdl->StartVa + (i*PAGE_SIZE);
        
        if (!MmIsPagePresent(NULL, Address))
        {
            /* Fault the page in */
            Status = MmAccessFault(FALSE, Address, AccessMode, NULL);
            if (!NT_SUCCESS(Status))
            {
				goto cleanup;
            }
        }
        else
        {
            MmLockPage(MmGetPfnForProcess(NULL, Address));
        }
        
        if ((Operation == IoWriteAccess || Operation == IoModifyAccess) &&
            (!(MmGetPageProtect(NULL, (PVOID)Address) & PAGE_READWRITE)))
        {
            Status = MmAccessFault(TRUE, Address, AccessMode, NULL);
            if (!NT_SUCCESS(Status))
            {
                for (j = 0; j < i; j++)
                {
                    Page = MdlPages[j];
                    if (Page < MmPageArraySize)
                    {
                        MmUnlockPage(Page);
                        MmDereferencePage(Page);
                    }
                }
				goto cleanup;
            }
        }
        Page = MmGetPfnForProcess(NULL, Address);
        MdlPages[i] = Page;
        if (Page >= MmPageArraySize)
        {
            Mdl->MdlFlags |= MDL_IO_SPACE;
        }        
        else
        {
            MmReferencePage(Page);
        }
    }

cleanup:
	if (OldIrql < DISPATCH_LEVEL)
		MmUnlockAddressSpace(AddressSpace);
	else
		MmReleasePageListLock(OldIrql);

	if (!NT_SUCCESS(Status))
		ExRaiseStatus(STATUS_ACCESS_VIOLATION);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;
	return;
}

/*
 * @implemented
 */
PMDL
NTAPI
MmAllocatePagesForMdl(IN PHYSICAL_ADDRESS LowAddress,
                      IN PHYSICAL_ADDRESS HighAddress,
                      IN PHYSICAL_ADDRESS SkipBytes,
                      IN SIZE_T Totalbytes)
{
    PMDL Mdl;
    PPFN_TYPE Pages;
    ULONG NumberOfPagesWanted, NumberOfPagesAllocated;
    ULONG Ret;
    DPRINT("Allocating pages: %p\n", LowAddress.LowPart);

    /* SkipBytes must be a multiple of the page size */
    if (BYTE_OFFSET(SkipBytes.LowPart)) return NULL;

    /* Create the actual MDL */
    Mdl = MmCreateMdl(NULL, NULL, Totalbytes);
    if (!Mdl) return NULL;

    /* Allocate pages into the MDL */
    NumberOfPagesAllocated = 0;
    NumberOfPagesWanted = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;
    Pages = (PPFN_TYPE)(Mdl + 1);
    while (NumberOfPagesWanted > 0)
    {
        Ret = MmAllocPagesSpecifyRange(MC_NPPOOL,
                                       LowAddress,
                                       HighAddress,
                                       NumberOfPagesWanted,
                                       Pages + NumberOfPagesAllocated);
        if (Ret == (ULONG)-1) break;
        
        NumberOfPagesAllocated += Ret;
        NumberOfPagesWanted -= Ret;
        
        if (SkipBytes.QuadPart == 0) break;
        LowAddress.QuadPart += SkipBytes.QuadPart;
        HighAddress.QuadPart += SkipBytes.QuadPart;
    }
    
    /* If nothing was allocated, fail */
    if (NumberOfPagesAllocated)
    {
        /* Free our MDL */
        ExFreePool(Mdl);
        return NULL;
    }
    
    /* Zero out the MDL pages */
    //RtlZeroMemory(LowAddress.LowPart, NumberOfPagesAllocated * PAGE_SIZE);
    
    /* Return the MDL */
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;
    Mdl->ByteCount = (ULONG)(NumberOfPagesAllocated * PAGE_SIZE);
    return Mdl;
}

/*
 * @unimplemented
 */
PMDL
NTAPI
MmAllocatePagesForMdlEx(IN PHYSICAL_ADDRESS LowAddress,
                        IN PHYSICAL_ADDRESS HighAddress,
                        IN PHYSICAL_ADDRESS SkipBytes,
                        IN SIZE_T Totalbytes,
                        IN MEMORY_CACHING_TYPE CacheType,
                        IN ULONG Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @implemented
 */
PVOID
NTAPI
MmMapLockedPagesSpecifyCache(IN PMDL Mdl,
                             IN KPROCESSOR_MODE AccessMode,
                             IN MEMORY_CACHING_TYPE CacheType,
                             IN PVOID BaseAddress,
                             IN ULONG BugCheckOnFailure,
                             IN MM_PAGE_PRIORITY Priority)
{
    PVOID Base;
    PULONG MdlPages;
    KIRQL oldIrql;
    ULONG PageCount;
    ULONG StartingOffset;
    PEPROCESS CurrentProcess;
    NTSTATUS Status;
    ULONG Protect;
    MEMORY_AREA *Result;
    LARGE_INTEGER BoundaryAddressMultiple;
    DPRINT("Mapping MDL: %p\n", Mdl);
    DPRINT("Base: %p\n", BaseAddress);
    
    /* Sanity checks */
    ASSERT(Mdl->ByteCount != 0);
    
    /* Get the base */
    Base = (PVOID)((ULONG_PTR)Mdl->StartVa + Mdl->ByteOffset);

    /* Set default page protection */
    Protect = PAGE_READWRITE;
    if (CacheType == MmNonCached) Protect |= PAGE_NOCACHE;
    
    /* Handle kernel case first */
    if (AccessMode == KernelMode)
    {
        /* Get the list of pages and count */
        MdlPages = (PPFN_NUMBER)(Mdl + 1);
        PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Base, Mdl->ByteCount);
        
        /* Sanity checks */
        ASSERT((Mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |
                                 MDL_SOURCE_IS_NONPAGED_POOL |
                                 MDL_PARTIAL_HAS_BEEN_MAPPED)) == 0);
        ASSERT((Mdl->MdlFlags & (MDL_PAGES_LOCKED | MDL_PARTIAL)) != 0);
        
        /* Allocate that number of pages from the mdl mapping region. */
        KeAcquireSpinLock(&MiMdlMappingRegionLock, &oldIrql);
        StartingOffset = RtlFindClearBitsAndSet(&MiMdlMappingRegionAllocMap,
                                                PageCount,
                                                MiMdlMappingRegionHint);
        if (StartingOffset == 0xffffffff)
        {
            KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);
            DPRINT("Out of MDL mapping space\n");
            if ((Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL) || !BugCheckOnFailure)
            {
                return NULL;
            }
            ASSERT(FALSE);
        }
        Base = (PVOID)((ULONG_PTR)MiMdlMappingRegionBase + StartingOffset * PAGE_SIZE);
        if (MiMdlMappingRegionHint == StartingOffset) MiMdlMappingRegionHint += PageCount;
        KeReleaseSpinLock(&MiMdlMappingRegionLock, oldIrql);
        
        /* Set the virtual mappings for the MDL pages. */
        if (Mdl->MdlFlags & MDL_IO_SPACE)
        {
            /* Map the pages */
            Status = MmCreateVirtualMappingUnsafe(NULL,
                                                  Base,
                                                  Protect,
                                                  MdlPages,
                                                  PageCount);
        }
        else
        {
            /* Map the pages */
            Status = MmCreateVirtualMapping(NULL,
                                            Base,
                                            Protect,
                                            MdlPages,
                                            PageCount);
        }
        
        /* Check if the mapping suceeded */
        if (!NT_SUCCESS(Status))
        {
            /* If it can fail, return NULL */
            if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL) return NULL;
            
            /* Should we bugcheck? */
            if (!BugCheckOnFailure) return NULL;
            
            /* Yes, crash the system */
            KeBugCheckEx(NO_MORE_SYSTEM_PTES, 0, PageCount, 0, 0);
        }

        /* Mark it as mapped */
        ASSERT((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);
        Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
        
        /* Check if it was partial */
        if (Mdl->MdlFlags & MDL_PARTIAL)
        {
            /* Write the appropriate flag here too */
            Mdl->MdlFlags |= MDL_PARTIAL_HAS_BEEN_MAPPED;
        }
        
        /* Save the mapped address */
        Base = (PVOID)((ULONG_PTR)Base + Mdl->ByteOffset);
        Mdl->MappedSystemVa = Base;
        return Base;
    }
    
    
    /* Calculate the number of pages required. */
    MdlPages = (PPFN_NUMBER)(Mdl + 1);
    PageCount = PAGE_ROUND_UP(Mdl->ByteCount + Mdl->ByteOffset) / PAGE_SIZE;

    BoundaryAddressMultiple.QuadPart = 0;
    Base = BaseAddress;
    
    CurrentProcess = PsGetCurrentProcess();
    
    MmLockAddressSpace(&CurrentProcess->VadRoot);
    Status = MmCreateMemoryArea(&CurrentProcess->VadRoot,
                                MEMORY_AREA_MDL_MAPPING,
                                &Base,
                                PageCount * PAGE_SIZE,
                                Protect,
                                &Result,
                                (Base != NULL),
                                0,
                                BoundaryAddressMultiple);
    MmUnlockAddressSpace(&CurrentProcess->VadRoot);
    if (!NT_SUCCESS(Status))
    {
        if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL)
        {
            return NULL;
        }
        
        /* Throw exception */
        ExRaiseStatus(STATUS_ACCESS_VIOLATION);
        ASSERT(0);
    }
    
    /* Set the virtual mappings for the MDL pages. */
    if (Mdl->MdlFlags & MDL_IO_SPACE)
    {
        /* Map the pages */
        Status = MmCreateVirtualMappingUnsafe(CurrentProcess,
                                              Base,
                                              Protect,
                                              MdlPages,
                                              PageCount);
    }
    else
    {
        /* Map the pages */
        Status = MmCreateVirtualMapping(CurrentProcess,
                                        Base,
                                        Protect,
                                        MdlPages,
                                        PageCount);
    }
    
    /* Check if the mapping suceeded */
    if (!NT_SUCCESS(Status))
    {
        /* If it can fail, return NULL */
        if (Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL) return NULL;

        /* Throw exception */
        ExRaiseStatus(STATUS_ACCESS_VIOLATION);
    }

    /* Return the base */
    Base = (PVOID)((ULONG_PTR)Base + Mdl->ByteOffset);
    return Base;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmAdvanceMdl(IN PMDL Mdl,
             IN ULONG NumberOfBytes)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
MmMapLockedPagesWithReservedMapping(IN PVOID MappingAddress,
                                    IN ULONG PoolTag,
                                    IN PMDL MemoryDescriptorList,
                                    IN MEMORY_CACHING_TYPE CacheType)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmUnmapReservedMapping(IN PVOID BaseAddress,
                       IN ULONG PoolTag,
                       IN PMDL MemoryDescriptorList)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmPrefetchPages(IN ULONG NumberOfLists,
                IN PREAD_LIST *ReadLists)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmProtectMdlSystemAddress(IN PMDL MemoryDescriptorList,
                          IN ULONG NewProtect)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmProbeAndLockProcessPages(IN OUT PMDL MemoryDescriptorList,
                           IN PEPROCESS Process,
                           IN KPROCESSOR_MODE AccessMode,
                           IN LOCK_OPERATION Operation)
{
	UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
NTAPI
MmProbeAndLockSelectedPages(IN OUT PMDL MemoryDescriptorList,
                            IN LARGE_INTEGER PageList[],
                            IN KPROCESSOR_MODE AccessMode,
                            IN LOCK_OPERATION Operation)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmMapMemoryDumpMdl(IN PMDL Mdl)
{
    UNIMPLEMENTED;
}

/* EOF */

