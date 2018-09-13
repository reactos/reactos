/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  IBM Corporation

Module Name:

    mippc.h

Abstract:

    This module contains the private data structures and procedure
    prototypes for the hardware dependent portion of the
    memory management system.

    It is specifically tailored for PowerPC.

Author:

    Lou Perazzoli (loup) 9-Jan-1991

    Modified for PowerPC by Mark Mergen (mergen@watson.ibm.com) 6-Oct-1993

Revision History:

--*/

/*++

    Virtual Memory Layout for PowerPC is:

                 +------------------------------------+
        00000000 |                                    |
                 |                                    |
                 |                                    |
                 | User Mode Addresses                |
                 |                                    |
                 |   All pages within this range      |
                 |   are potentially accessable while |
                 |   the CPU is in USER mode.         |
                 |                                    |
                 |                                    |
                 +------------------------------------+
        7fff0000 | 64k No Access Area                 |
                 +------------------------------------+
        80000000 |                                    | KSEG0
                 | OsLoader loads critical parts      |
                 | of boot code and data in           |
                 | this region.  Mapped by BAT0.      |
                 | Kernel mode access only.           |
                 |                                    |
                 +------------------------------------+
        8xxx0000 |                                    | KSEG1 KSEG2
                 | OsLoader loads remaining boot      |
                 | code and data here.  Mapped        |
                 | by segment register 8.             |
                 | Kernel mode access only.           |
                 |                                    |
                 +------------------------------------+
        8yyy0000 |                                    |
                 |                                    |
                 | Unused   NO ACCESS                 |
                 |                                    |
                 |                                    |
                 +------------------------------------+
        90000000 | System Cache Working Set           |
        90400000 | System Cache                       |
                 |                                    |
                 |                                    |
                 |                                    |
        AE000000 |   Kernel mode access only.         |
                 +------------------------------------+
        C0000000 | Page Table Pages mapped through    |
                 |   this 4mb region                  |
                 |   Kernel mode access only.         |
                 |                                    |
                 +------------------------------------+
        C0400000 | HyperSpace - working set lists     |
                 |   and per process memory mangement |
                 |   structures mapped in this 4mb    |
                 |   region.                          |
                 |   Kernel mode access only.         |
                 +------------------------------------+
        C0800000 | NO ACCESS AREA                     |
                 |                                    |
                 +------------------------------------+
        D0000000 | System mapped views                |
                 |   Kernel mode access only.         |
                 |                                    |
                 +------------------------------------+
        D3000000 | Start of paged system area         |
                 |   Kernel mode access only.         |
                 |                                    |
                 |                                    |
                 |                                    |
                 +------------------------------------+
        E0000000 |                                    |
                 |   Kernel mode access only.         |
                 |                                    |
                 |                                    |
        EFBFFFFF | NonPaged System area               |
                 +------------------------------------+
        EFC00000 | Last 4mb reserved for HAL usage    |
                 +------------------------------------+
        F0000000 | Unused, No access.                 |
                 |                                    |
        FFFFD000 | Per Processor PCR                  |
        FFFFE000 | Shared PCR2                        |
        FFFFF000 | Debugger Page for physical memory  |
                 +------------------------------------+

    Segment Register usage

    0 - 7     User mode addresses, switched at Process Switch time
    8         Constant, shared amongst processors and processes.
              No change on switch to user mode but always invalid for
              user mode.  Very low part of this range is KSEG0, mapped
              by a BAT register.
    9 - A     Constant, Shared amongst processors and processes,
              invalidated while in user mode.
    C         Per process kernel data.  invalidated while in user mode.
    D         Constant, Shared amongst processors and processes,
              invalidated while in user mode.
    E         Constant, shared amongst processors and processes.
              No change on switch to user mode but always invalid for
              user mode.
    F         Per processor.  Kernel mode access only.

--*/

//
// PAGE_SIZE for PowerPC is 4k, virtual page is 20 bits with a PAGE_SHIFT
// byte offset.
//

#define MM_VIRTUAL_PAGE_SHIFT 20

//
// Address space layout definitions.
//

//#define PDE_BASE ((ULONG)0xC0300000)

//#define PTE_BASE ((ULONG)0xC0000000)

#define MM_SYSTEM_SPACE_START (0xD0000000)

//
// N.B. This should ONLY be used for copying PDEs.
//      Segment 15 is only used for PCR pages,
//      hardwired PDE for the debuggers, and
//      crash dump.
//

#define MM_SYSTEM_SPACE_END (0xFFFFFFFF)

#define MM_HAL_RESERVED (0xFFC00000)

#define PDE_TOP 0xC03FFFFF

#define HYPER_SPACE ((PVOID)0xC0400000)

#define HYPER_SPACE_END 0xC07fffff

//
// Define the start and maximum size for the system cache.
// Maximum size 476MB.
//

#define MM_SYSTEM_CACHE_AND_POOL_DISJOINT 1

#define MM_SYSTEM_CACHE_WORKING_SET (0x90000000)

#define MM_SYSTEM_CACHE_START (0x90400000)

#define MM_SYSTEM_CACHE_END (0xAE000000)

#define MM_MAXIMUM_SYSTEM_CACHE_SIZE     \
   (((ULONG)MM_SYSTEM_CACHE_END - (ULONG)MM_SYSTEM_CACHE_START) >> PAGE_SHIFT)

//
// Tell MM that boot code and data is pageable.
//

#define MM_BOOT_CODE_PAGEABLE 1

#define MM_BOOT_CODE_START (0x80000000)
#define MM_BOOT_CODE_END (0x90000000)

//
// Define MM_SYSTEM_CACHE_AND_POOL_DISJOINT so that MmCreateProcessAddressSpace
// knows that it has to do two RtlCopyMemorys to copy the PDEs for the cache
// and the rest of system space.
//

#define MM_SYSTEM_CACHE_AND_POOL_DISJOINT 1


//
// Define area for mapping views into system space.
//

#define MM_SYSTEM_VIEW_START (0xD0000000)

#define MM_SYSTEM_VIEW_SIZE (48*1024*1024)

#define MM_PAGED_POOL_START ((PVOID)(0xD3000000))

#define MM_LOWEST_NONPAGED_SYSTEM_START ((PVOID)(0xE0000000))

#define MmProtopte_Base ((ULONG)0xD3000000)

#define MM_NONPAGED_POOL_END ((PVOID)(0xEFC00000))

#define NON_PAGED_SYSTEM_END   ((ULONG)0xEFFFFFF0)  //quadword aligned.

//
// Define absolute minumum and maximum count for system ptes.
//

#define MM_MINIMUM_SYSTEM_PTES 9000

#define MM_MAXIMUM_SYSTEM_PTES 35000

#define MM_DEFAULT_SYSTEM_PTES 15000

//
// Pool limits
//

//
// The maximim amount of nonpaged pool that can be initially created.
//

#define MM_MAX_INITIAL_NONPAGED_POOL ((ULONG)(128*1024*1024))

//
// The total amount of nonpaged pool (initial pool + expansion + system PTEs).
//

#define MM_MAX_ADDITIONAL_NONPAGED_POOL ((ULONG)(192*1024*1024))

//
// The maximum amount of paged pool that can be created.
//

#define MM_MAX_PAGED_POOL ((ULONG)(176*1024*1024))

#define MM_MAX_TOTAL_POOL (((ULONG)MM_NONPAGED_POOL_END) - ((ULONG)(MM_PAGED_POOL_START)))


//
// Structure layout defintions.
//

#define PAGE_DIRECTORY_MASK    ((ULONG)0x003FFFFF)

#define MM_VA_MAPPED_BY_PDE (0x400000)

// N.B. this is probably a real address, for what purpose?
#define LOWEST_IO_ADDRESS (0x80000000)

#define PTE_SHIFT (2)

//
// The number of bits in a physical address.
//

#define PHYSICAL_ADDRESS_BITS (32)

#define MM_PROTO_PTE_ALIGNMENT ((ULONG)MM_MAXIMUM_NUMBER_OF_COLORS * (ULONG)PAGE_SIZE)

//
// Maximum number of paging files.
//

#define MAX_PAGE_FILES 16

//
// Hyper space definitions.
//

#define FIRST_MAPPING_PTE   ((ULONG)0xC0400000)

#define NUMBER_OF_MAPPING_PTES 255

#define LAST_MAPPING_PTE   \
     ((ULONG)((ULONG)FIRST_MAPPING_PTE + (NUMBER_OF_MAPPING_PTES * PAGE_SIZE)))

#define IMAGE_MAPPING_PTE   ((PMMPTE)((ULONG)LAST_MAPPING_PTE + PAGE_SIZE))

#define ZEROING_PAGE_PTE    ((PMMPTE)((ULONG)IMAGE_MAPPING_PTE + PAGE_SIZE))

#define WORKING_SET_LIST   ((PVOID)((ULONG)ZEROING_PAGE_PTE + PAGE_SIZE))

#define MM_MAXIMUM_WORKING_SET \
       ((ULONG)((ULONG)2*1024*1024*1024 - 64*1024*1024) >> PAGE_SHIFT) //2Gb-64Mb

#define MM_WORKING_SET_END ((ULONG)0xC07FF000)

//
// Define masks for fields within the PTE.
//

#define MM_PTE_PROTOTYPE_MASK     0x1
#define MM_PTE_VALID_MASK         0x4
#define MM_PTE_CACHE_DISABLE_MASK 0x28      // CacheInhibit | Guard
#define MM_PTE_TRANSITION_MASK    0x2
#define MM_PTE_WRITE_MASK         0x200
#define MM_PTE_COPY_ON_WRITE_MASK 0x400

//
// Bit fields to or into PTE to make a PTE valid based on the
// protection field of the invalid PTE.
//

#define MM_PTE_NOACCESS          0x0    // not expressable on PowerPC
#define MM_PTE_READONLY          0x3
#define MM_PTE_READWRITE         (0x3 | MM_PTE_WRITE_MASK)
#define MM_PTE_WRITECOPY         (0x3 | MM_PTE_WRITE_MASK | MM_PTE_COPY_ON_WRITE_MASK)
#define MM_PTE_EXECUTE           0x3    // read-only on PowerPC
#define MM_PTE_EXECUTE_READ      0x3
#define MM_PTE_EXECUTE_READWRITE (0x3 | MM_PTE_WRITE_MASK)
#define MM_PTE_EXECUTE_WRITECOPY (0x3 | MM_PTE_WRITE_MASK | MM_PTE_COPY_ON_WRITE_MASK)
#define MM_PTE_NOCACHE           (MM_PTE_CACHE_DISABLE_MASK)
#define MM_PTE_GUARD             0x0    // not expressable on PowerPC
#define MM_PTE_CACHE             0x0

#define MM_PROTECT_FIELD_SHIFT 3

//
// Zero PTE
//

#define MM_ZERO_PTE 0

//
// Zero Kernel PTE
//

#define MM_ZERO_KERNEL_PTE 0


//
// A demand zero PTE with a protection of PAGE_READWRITE.
//

#define MM_DEMAND_ZERO_WRITE_PTE (MM_READWRITE << MM_PROTECT_FIELD_SHIFT)

//
// A demand zero PTE with a protection of PAGE_READWRITE for system space.
//

#define MM_KERNEL_DEMAND_ZERO_PTE (MM_READWRITE << MM_PROTECT_FIELD_SHIFT)

//
// A no access PTE for system space.
//

#define MM_KERNEL_NOACCESS_PTE (MM_NOACCESS << MM_PROTECT_FIELD_SHIFT)

//
// Dirty bit definitions for clean and dirty.
//

#define MM_PTE_CLEAN 3
#define MM_PTE_DIRTY 0


//
// Kernel stack alignment requirements.
//

#define MM_STACK_ALIGNMENT 0x0
#define MM_STACK_OFFSET 0x0

//
// System process definitions
//

#define PDE_PER_PAGE ((ULONG)1024)

#define PTE_PER_PAGE ((ULONG)1024)

//
// Number of page table pages for user addresses.
//

#define MM_USER_PAGE_TABLE_PAGES (512)

//
// Indicate the number of page colors required.
//

#define MM_NUMBER_OF_COLORS 2
#define MM_MAXIMUM_NUMBER_OF_COLORS 2

//
// Mask for obtaining color from a physical page number.
//

#define MM_COLOR_MASK 1

//
// Define secondary color stride.
//

#define MM_COLOR_STRIDE 3

//
// Boundary for aligned pages of like color upon.
//

#define MM_COLOR_ALIGNMENT 0x2000

//
// Mask for isolating color from virtual address.
//

#define MM_COLOR_MASK_VIRTUAL 0x1000

//
//  Define 256K worth of secondary colors.
//

#define MM_SECONDARY_COLORS_DEFAULT ((256*1024) >> PAGE_SHIFT)

#define MM_SECONDARY_COLORS_MIN (2)

#define MM_SECONDARY_COLORS_MAX (2048)

//
// Mask for isolating secondary color from physical page number;
//

extern ULONG MmSecondaryColorMask;

//
// Define macro to initialize directory table base.
//

#define INITIALIZE_DIRECTORY_TABLE_BASE(dirbase,pfn) \
     *((PULONG)(dirbase)) = ((pfn) << PAGE_SHIFT)


//++
//VOID
//MI_MAKE_VALID_PTE (
//    OUT OUTPTE,
//    IN FRAME,
//    IN PMASK,
//    IN OWNER
//    );
//
// Routine Description:
//
//    This macro makes a valid PTE from a page frame number, protection mask,
//    and owner.
//
// Argments
//
//    OUTPTE - Supplies the PTE in which to build the transition PTE.
//
//    FRAME - Supplies the page frame number for the PTE.
//
//    PMASK - Supplies the protection to set in the transition PTE.
//
//    PPTE - Supplies a pointer to the PTE which is being made valid.
//           For prototype PTEs NULL should be specified.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_VALID_PTE(OUTPTE,FRAME,PMASK,PPTE)          \
    {                                                       \
       (OUTPTE).u.Long = ((FRAME << 12) |                   \
                         (MmProtectToPteMask[PMASK]) |      \
                          MM_PTE_VALID_MASK);               \
       if (((OUTPTE).u.Hard.Write == 1) &&                  \
          (((PMMPTE)PPTE) >= MiGetPteAddress(MM_LOWEST_NONPAGED_SYSTEM_START)))\
       {                                                    \
           (OUTPTE).u.Hard.Dirty = MM_PTE_DIRTY;            \
       }                                                    \
    }


//++
//VOID
//MI_MAKE_VALID_PTE_TRANSITION (
//    IN OUT OUTPTE
//    IN PROTECT
//    );
//
// Routine Description:
//
//    This macro takes a valid pte and turns it into a transition PTE.
//
// Argments
//
//    OUTPTE - Supplies the current valid PTE.  This PTE is then
//             modified to become a transition PTE.
//
//    PROTECT - Supplies the protection to set in the transition PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_VALID_PTE_TRANSITION(OUTPTE,PROTECT) \
                (OUTPTE).u.Trans.Transition = 1;          \
                (OUTPTE).u.Trans.Valid = 0;               \
                (OUTPTE).u.Trans.Prototype = 0;           \
                (OUTPTE).u.Trans.Protection = PROTECT;


//++
//VOID
//MI_MAKE_TRANSITION_PTE (
//    OUT OUTPTE,
//    IN PAGE,
//    IN PROTECT,
//    IN PPTE
//    );
//
// Routine Description:
//
//    This macro takes a valid pte and turns it into a transition PTE.
//
// Argments
//
//    OUTPTE - Supplies the PTE in which to build the transition PTE.
//
//    PAGE - Supplies the page frame number for the PTE.
//
//    PROTECT - Supplies the protection to set in the transition PTE.
//
//    PPTE - Supplies a pointer to the PTE, this is used to determine
//           the owner of the PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_TRANSITION_PTE(OUTPTE,PAGE,PROTECT,PPTE)   \
                (OUTPTE).u.Long = 0;                       \
                (OUTPTE).u.Trans.PageFrameNumber = PAGE;   \
                (OUTPTE).u.Trans.Transition = 1;           \
                (OUTPTE).u.Trans.Protection = PROTECT;


//++
//VOID
//MI_MAKE_TRANSITION_PTE_VALID (
//    OUT OUTPTE,
//    IN PPTE
//    );
//
// Routine Description:
//
//    This macro takes a transition pte and makes it a valid PTE.
//
// Argments
//
//    OUTPTE - Supplies the PTE in which to build the valid PTE.
//
//    PPTE - Supplies a pointer to the transition PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_TRANSITION_PTE_VALID(OUTPTE,PPTE)                             \
       (OUTPTE).u.Long = (((PPTE)->u.Long & 0xFFFFF000) |                     \
                         (MmProtectToPteMask[(PPTE)->u.Trans.Protection]) |   \
                          MM_PTE_VALID_MASK);

//++
//VOID
//MI_SET_PTE_DIRTY (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro sets the dirty bit(s) in the specified PTE.
//
// Argments
//
//    PTE - Supplies the PTE to set dirty.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_DIRTY(PTE) (PTE).u.Hard.Dirty = MM_PTE_DIRTY


//++
//VOID
//MI_SET_PTE_CLEAN (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro clears the dirty bit(s) in the specified PTE.
//
// Argments
//
//    PTE - Supplies the PTE to set clear.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_CLEAN(PTE) (PTE).u.Hard.Dirty = MM_PTE_CLEAN



//++
//VOID
//MI_IS_PTE_DIRTY (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro checks the dirty bit(s) in the specified PTE.
//
// Argments
//
//    PTE - Supplies the PTE to check.
//
// Return Value:
//
//    TRUE if the page is dirty (modified), FALSE otherwise.
//
//--

#define MI_IS_PTE_DIRTY(PTE) ((PTE).u.Hard.Dirty != MM_PTE_CLEAN)




//++
//VOID
//MI_SET_GLOBAL_BIT_IF_SYSTEM (
//    OUT OUTPTE,
//    IN PPTE
//    );
//
// Routine Description:
//
//    This macro sets the global bit if the pointer PTE is within
//    system space.
//
// Argments
//
//    OUTPTE - Supplies the PTE in which to build the valid PTE.
//
//    PPTE - Supplies a pointer to the PTE becoming valid.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_GLOBAL_BIT_IF_SYSTEM(OUTPTE,PPTE)


//++
//VOID
//MI_SET_GLOBAL_STATE (
//    IN MMPTE PTE,
//    IN ULONG STATE
//    );
//
// Routine Description:
//
//    This macro sets the global bit in the PTE. if the pointer PTE is within
//
// Argments
//
//    PTE - Supplies the PTE to set global state into.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_GLOBAL_STATE(PTE,STATE)



//++
//VOID
//MI_ENABLE_CACHING (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and sets the caching state to be
//    enabled.
//
// Argments
//
//    PTE - Supplies a valid PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_ENABLE_CACHING(PTE) \
        ((PTE).u.Hard.CacheDisable = (PTE).u.Hard.GuardedStorage = 0)


//++
//VOID
//MI_DISABLE_CACHING (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and sets the caching state to be
//    disabled.
//
// Argments
//
//    PTE - Supplies a valid PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_DISABLE_CACHING(PTE) \
        ((PTE).u.Hard.CacheDisable = (PTE).u.Hard.GuardedStorage = 1)


//++
//BOOLEAN
//MI_IS_CACHING_DISABLED (
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and returns TRUE if caching is
//    disabled.
//
// Argments
//
//    PPTE - Supplies a pointer to the valid PTE.
//
// Return Value:
//
//     TRUE if caching is disabled, FALSE if it is enabled.
//
//--

#define MI_IS_CACHING_DISABLED(PPTE)   \
            ((PPTE)->u.Hard.CacheDisable == 1)


//++
//VOID
//MI_SET_PFN_DELETED (
//    IN PMMPFN PPFN
//    );
//
// Routine Description:
//
//    This macro takes a pointer to a PFN element and indicates that
//    the PFN is no longer in use.
//
// Argments
//
//    PPTE - Supplies a pointer to the PFN element.
//
// Return Value:
//
//    none.
//
//--

#define MI_SET_PFN_DELETED(PPFN) ((PPFN)->PteAddress = (PMMPTE)0xFFFFFFFF)


//++
//BOOLEAN
//MI_IS_PFN_DELETED (
//    IN PMMPFN PPFN
//    );
//
// Routine Description:
//
//    This macro takes a pointer to a PFN element a determines if
//    the PFN is no longer in use.
//
// Argments
//
//    PPTE - Supplies a pointer to the PFN element.
//
// Return Value:
//
//     TRUE if PFN is no longer used, FALSE if it is still being used.
//
//--

#define MI_IS_PFN_DELETED(PPFN)   \
            ((PPFN)->PteAddress == (PMMPTE)0xFFFFFFFF)


//++
//VOID
//MI_CHECK_PAGE_ALIGNMENT (
//    IN ULONG PAGE,
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro takes a PFN element number (Page) and checks to see
//    if the virtual alignment for the previous address of the page
//    is compatable with the new address of the page.  If they are
//    not compatable, the D cache is flushed.
//
// Argments
//
//    PAGE - Supplies the PFN element.
//    PPTE - Supplies a pointer to the new PTE which will contain the page.
//
// Return Value:
//
//    none.
//
//--

#define MI_CHECK_PAGE_ALIGNMENT(PAGE,COLOR)                                \
{                                                                          \
    PMMPFN PPFN;                                                           \
    ULONG OldColor;                                                        \
    PPFN = MI_PFN_ELEMENT(PAGE);                                           \
    OldColor = PPFN->u3.e1.PageColor;                                      \
    if ((COLOR) != OldColor) {                                             \
        PPFN->u3.e1.PageColor = COLOR;                                     \
    }                                                                      \
}


//++
//VOID
//MI_INITIALIZE_HYPERSPACE_MAP (
//    HYPER_PAGE
//    );
//
// Routine Description:
//
//    This macro initializes the PTEs reserved for double mapping within
//    hyperspace.
//
// Argments
//
//    HYPER_PAGE - Phyical page number for the page to become hyperspace.
//
// Return Value:
//
//    None.
//
//--

#define MI_INITIALIZE_HYPERSPACE_MAP(HYPER_PAGE)                               \
    {                                                                          \
        PMMPTE Base;                                                           \
        KIRQL OldIrql;                                                         \
        Base = MiMapPageInHyperSpace (HYPER_PAGE, &OldIrql);                   \
        Base->u.Hard.PageFrameNumber = NUMBER_OF_MAPPING_PTES;                 \
        MiUnmapPageInHyperSpace (OldIrql);                                     \
    }



//++
//ULONG
//MI_GET_PAGE_COLOR_FROM_PTE (
//    IN PMMPTE PTEADDRESS
//    );
//
// Routine Description:
//
//    This macro determines the pages color based on the PTE address
//    that maps the page.
//
// Argments
//
//    PTEADDRESS - Supplies the PTE address the page is (or was) mapped at.
//
// Return Value:
//
//    The pages color.
//
//--

#define MI_GET_PAGE_COLOR_FROM_PTE(PTEADDRESS)  \
         ((ULONG)((MmSystemPageColor += MM_COLOR_STRIDE) &      \
                   MmSecondaryColorMask) |                      \
         ((((ULONG)(PTEADDRESS)) >> 2) & MM_COLOR_MASK))

//++
//ULONG
//MI_GET_PAGE_COLOR_FROM_VA (
//    IN PVOID ADDRESS
//    );
//
// Routine Description:
//
//    This macro determines the pages color based on the PTE address
//    that maps the page.
//
// Argments
//
//    ADDRESS - Supplies the address the page is (or was) mapped at.
//
// Return Value:
//
//    The pages color.
//
//--

#define MI_GET_PAGE_COLOR_FROM_VA(ADDRESS)  \
         ((ULONG)((MmSystemPageColor += MM_COLOR_STRIDE) &      \
                   MmSecondaryColorMask) |                      \
         ((((ULONG)(ADDRESS)) >> PAGE_SHIFT) & MM_COLOR_MASK))


//++
//ULONG
//MI_PAGE_COLOR_PTE_PROCESS (
//    IN PCHAR COLOR,
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro determines the pages color based on the PTE address
//    that maps the page.
//
// Argments
//
//
// Return Value:
//
//    The pages color.
//
//--

#define MI_PAGE_COLOR_PTE_PROCESS(PTE,COLOR)  \
         ((ULONG)(((*(COLOR)) += MM_COLOR_STRIDE) &             \
                    MmSecondaryColorMask) |                     \
         ((((ULONG)(PTE)) >> 2) & MM_COLOR_MASK))


//++
//ULONG
//MI_PAGE_COLOR_VA_PROCESS (
//    IN PVOID ADDRESS,
//    IN PEPROCESS COLOR
//    );
//
// Routine Description:
//
//    This macro determines the pages color based on the PTE address
//    that maps the page.
//
// Argments
//
//    ADDRESS - Supplies the address the page is (or was) mapped at.
//
// Return Value:
//
//    The pages color.
//
//--

#define MI_PAGE_COLOR_VA_PROCESS(ADDRESS,COLOR) \
         ((ULONG)(((*(COLOR)) += MM_COLOR_STRIDE) &             \
                    MmSecondaryColorMask) |                     \
         ((((ULONG)(ADDRESS)) >> PAGE_SHIFT) & MM_COLOR_MASK))


//++
//ULONG
//MI_GET_NEXT_COLOR (
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the next color in the sequence.
//
// Argments
//
//    COLOR - Supplies the color to return the next of.
//
// Return Value:
//
//    Next color in sequence.
//
//--

#define MI_GET_NEXT_COLOR(COLOR)  ((COLOR + 1) & MM_COLOR_MASK)


//++
//ULONG
//MI_GET_PREVIOUS_COLOR (
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the previous color in the sequence.
//
// Argments
//
//    COLOR - Supplies the color to return the previous of.
//
// Return Value:
//
//    Previous color in sequence.
//
//--

#define MI_GET_PREVIOUS_COLOR(COLOR)  ((COLOR - 1) & MM_COLOR_MASK)

#define MI_GET_SECONDARY_COLOR(PAGE,PFN)           \
         ((((ULONG)(PAGE) & MmSecondaryColorMask)) | (PFN)->u3.e1.PageColor)

#define MI_GET_COLOR_FROM_SECONDARY(COLOR)  ((COLOR) & MM_COLOR_MASK)


//++
//VOID
//MI_GET_MODIFIED_PAGE_BY_COLOR (
//    OUT ULONG PAGE,
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the first page destined for a paging
//    file with the desired color.  It does NOT remove the page
//    from its list.
//
// Argments
//
//    PAGE - Returns the page located, the value MM_EMPTY_LIST is
//           returned if there is no page of the specified color.
//
//    COLOR - Supplies the color of page to locate.
//
// Return Value:
//
//    none.
//
//--

#define MI_GET_MODIFIED_PAGE_BY_COLOR(PAGE,COLOR) \
            PAGE = MmModifiedPageListByColor[COLOR].Flink


//++
//VOID
//MI_GET_MODIFIED_PAGE_ANY_COLOR (
//    OUT ULONG PAGE,
//    IN OUT ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the first page destined for a paging
//    file with the desired color.  If not page of the desired
//    color exists, all colored lists are searched for a page.
//    It does NOT remove the page from its list.
//
// Argments
//
//    PAGE - Returns the page located, the value MM_EMPTY_LIST is
//           returned if there is no page of the specified color.
//
//    COLOR - Supplies the color of page to locate and returns the
//            color of the page located.
//
// Return Value:
//
//    none.
//
//--

#define MI_GET_MODIFIED_PAGE_ANY_COLOR(PAGE,COLOR) \
            {                                                                \
                if (MmTotalPagesForPagingFile == 0) {                        \
                    PAGE = MM_EMPTY_LIST;                                    \
                } else {                                                     \
                    while (MmModifiedPageListByColor[COLOR].Flink ==         \
                                                            MM_EMPTY_LIST) { \
                        COLOR = MI_GET_NEXT_COLOR(COLOR);                    \
                    }                                                        \
                    PAGE = MmModifiedPageListByColor[COLOR].Flink;           \
                }                                                            \
            }


//++
//VOID
//MI_MAKE_VALID_PTE_WRITE_COPY (
//    IN OUT PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro checks to see if the PTE indicates that the
//    page is writable and if so it clears the write bit and
//    sets the copy-on-write bit.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_VALID_PTE_WRITE_COPY(PPTE)                   \
                    if ((PPTE)->u.Hard.Write == 1) {         \
                        (PPTE)->u.Hard.CopyOnWrite = 1;      \
                        (PPTE)->u.Hard.Dirty = MM_PTE_CLEAN; \
                    }


//++
//ULONG
//MI_DETERMINE_OWNER (
//    IN MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro examines the virtual address of the PTE and determines
//    if the PTE resides in system space or user space.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     1 if the owner is USER_MODE, 0 if the owner is KERNEL_MODE.
//
//--

#define MI_DETERMINE_OWNER(PPTE)   \
    ((((PPTE) <= MiGetPteAddress(MM_HIGHEST_USER_ADDRESS)) ||               \
      ((PPTE) >= MiGetPdeAddress(NULL) &&                                    \
      ((PPTE) <= MiGetPdeAddress(MM_HIGHEST_USER_ADDRESS)))) ? 1 : 0)


//++
//VOID
//MI_SET_ACCESSED_IN_PTE (
//    IN OUT MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro sets the ACCESSED field in the PTE.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     1 if the owner is USER_MODE, 0 if the owner is KERNEL_MODE.
//
//--

#define MI_SET_ACCESSED_IN_PTE(PPTE,ACCESSED)


//++
//ULONG
//MI_GET_ACCESSED_IN_PTE (
//    IN OUT MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro returns the state of the ACCESSED field in the PTE.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     The state of the ACCESSED field.
//
//--

#define MI_GET_ACCESSED_IN_PTE(PPTE) 0


//++
//VOID
//MI_SET_OWNER_IN_PTE (
//    IN PMMPTE PPTE
//    IN ULONG OWNER
//    );
//
// Routine Description:
//
//    This macro sets the owner field in the PTE.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    None.
//
//--

#define MI_SET_OWNER_IN_PTE(PPTE,OWNER)


//++
//ULONG
//MI_GET_OWNER_IN_PTE (
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro gets the owner field from the PTE.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     The state of the OWNER field.
//
//--

#define MI_GET_OWNER_IN_PTE(PPTE) KernelMode


// bit mask to clear out fields in a PTE to or in paging file location.

#define CLEAR_FOR_PAGE_FILE ((ULONG)(0x0F8))


//++
//VOID
//MI_SET_PAGING_FILE_INFO (
//    IN OUT MMPTE PPTE,
//    IN ULONG FILEINFO,
//    IN ULONG OFFSET
//    );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
//    FILEINFO - Supplies the number of the paging file.
//
//    OFFSET - Supplies the offset into the paging file.
//
// Return Value:
//
//    None.
//
//--

#define SET_PAGING_FILE_INFO(PTE,FILEINFO,OFFSET)                     \
                        ((((PTE).u.Long & CLEAR_FOR_PAGE_FILE) |      \
                        (((FILEINFO) << 8) |                          \
                        (OFFSET << 12))))


//++
//PMMPTE
//MiPteToProto (
//    IN OUT MMPTE PPTE,
//    IN ULONG FILEINFO,
//    IN ULONG OFFSET
//    );
//
// Routine Description:
//
//   This macro returns the address of the corresponding prototype which
//   was encoded earlier into the supplied PTE.
//
// NOTE THAT AS PROTOPTE CAN RESIDE IN BOTH PAGED AND NONPAGED POOL
// THIS MACRO LIMITS THE COMBINED SIZES OF TWO POOLS AND REQUIRES THEM
// TO BE WITHIN THE MAX SIZE CONSTRAINTS
//
//  MAX SIZE = 2^(2+8+20) = 2^30 = 1GB
//
//    NOTE, that the valid bit must be zero!
//
// Argments
//
//    lpte - Supplies the PTE to operate upon.
//
// Return Value:
//
//    Pointer to the prototype PTE that backs this PTE.
//
//--

#define MiPteToProto(lpte) ((PMMPTE)((((lpte)->u.Long >> 4) << 2) +  \
                                        MmProtopte_Base))


//++
//ULONG
//MiProtoAddressForPte (
//    IN PMMPTE proto_va
//    );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//    MiProtoAddressForPte returns the bit field to OR into the PTE to
//    reference a prototype PTE.  And set the protoPTE bit,
//    MM_PTE_PROTOTYPE_MASK.
//
// Argments
//
//    proto_va - Supplies the address of the prototype PTE.
//
// Return Value:
//
//    Mask to set into the PTE.
//
//--

#define MiProtoAddressForPte(proto_va)  \
  ((ULONG)((((ULONG)proto_va - MmProtopte_Base) << 2) | MM_PTE_PROTOTYPE_MASK))


//++
//ULONG
//MiProtoAddressForKernelPte (
//    IN PMMPTE proto_va
//    );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//    MiProtoAddressForPte returns the bit field to OR into the PTE to
//    reference a prototype PTE.  And set the protoPTE bit,
//    MM_PTE_PROTOTYPE_MASK.
//
//    This macro also sets any other information (such as global bits)
//    required for kernel mode PTEs.
//
// Argments
//
//    proto_va - Supplies the address of the prototype PTE.
//
// Return Value:
//
//    Mask to set into the PTE.
//
//--

#define MiProtoAddressForKernelPte(proto_va)  MiProtoAddressForPte(proto_va)


//++
//PSUBSECTION
//MiGetSubsectionAddress (
//    IN PMMPTE lpte
//    );
//
// Routine Description:
//
//   This macro takes a PTE and returns the address of the subsection that
//   the PTE refers to.  Subsections are quadword structures allocated
//   from nonpaged pool.
//
//   NOTE THIS MACRO LIMITS THE SIZE OF NONPAGED POOL!
//    MAXIMUM NONPAGED POOL = 2^(3+1+24) = 2^28 = 256mb.
//
//
// Argments
//
//    lpte - Supplies the PTE to operate upon.
//
// Return Value:
//
//    A pointer to the subsection referred to by the supplied PTE.
//
//--

#define MiGetSubsectionAddress(lpte)                              \
            ((PSUBSECTION)((ULONG)MM_NONPAGED_POOL_END -          \
                 (((((lpte)->u.Long) >> 8) << 4) |                \
                 ((((lpte)->u.Long) << 2) & 0x8))))


//++
//ULONG
//MiGetSubsectionAddressForPte (
//    IN PSUBSECTION VA
//    );
//
// Routine Description:
//
//    This macro takes the address of a subsection and encodes it for use
//    in a PTE.
//
//    NOTE - THE SUBSECTION ADDRESS MUST BE QUADWORD ALIGNED!
//
// Argments
//
//    VA - Supplies a pointer to the subsection to encode.
//
// Return Value:
//
//     The mask to set into the PTE to make it reference the supplied
//     subsetion.
//
//--

#define MiGetSubsectionAddressForPte(VA)                   \
    (((((ULONG)MM_NONPAGED_POOL_END - (ULONG)VA) << 4) & (ULONG)0xffffff00) | \
    ((((ULONG)MM_NONPAGED_POOL_END - (ULONG)VA) >> 2) & (ULONG)0x2))


//++
//PMMPTE
//MiGetPdeAddress (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPdeAddress returns the address of the PDE which maps the
//    given virtual address.
//
// Argments
//
//    Va - Supplies the virtual address to locate the PDE for.
//
// Return Value:
//
//    The address of the PDE.
//
//--

#define MiGetPdeAddress(va)  ((PMMPTE)(((((ULONG)(va)) >> 22) << 2) + PDE_BASE))


//++
//PMMPTE
//MiGetPteAddress (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPteAddress returns the address of the PTE which maps the
//    given virtual address.
//
// Argments
//
//    Va - Supplies the virtual address to locate the PTE for.
//
// Return Value:
//
//    The address of the PTE.
//
//--

#define MiGetPteAddress(va) ((PMMPTE)(((((ULONG)(va)) >> 12) << 2) + PTE_BASE))


//++
//ULONG
//MiGetPdeOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPdeOffset returns the offset into a page directory
//    for a given virtual address.
//
// Argments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the page directory table the corresponding PDE is at.
//
//--

#define MiGetPdeOffset(va) (((ULONG)(va)) >> 22)


//++
//ULONG
//MiGetPteOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPteOffset returns the offset into a page table page
//    for a given virtual address.
//
// Argments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the page table page table the corresponding PTE is at.
//
//--

#define MiGetPteOffset(va) ((((ULONG)(va)) << 10) >> 22)



//++
//PVOID
//MiGetVirtualAddressMappedByPte (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    MiGetVirtualAddressMappedByPte returns the virtual address
//    which is mapped by a given PTE address.
//
// Argments
//
//    PTE - Supplies the PTE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PTE.
//
//--

#define MiGetVirtualAddressMappedByPte(va) ((PVOID)((ULONG)(va) << 10))


//++
//ULONG
//GET_PAGING_FILE_NUMBER (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro extracts the paging file number from a PTE.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    The paging file number.
//
//--

#define GET_PAGING_FILE_NUMBER(PTE) ((((PTE).u.Long) >> 8) & 0xF)


//++
//ULONG
//GET_PAGING_FILE_OFFSET (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro extracts the offset into the paging file from a PTE.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    The paging file offset.
//
//--

#define GET_PAGING_FILE_OFFSET(PTE) ((((PTE).u.Long) >> 12) & 0x000FFFFF)


//++
//ULONG
//IS_PTE_NOT_DEMAND_ZERO (
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro checks to see if a given PTE is NOT a demand zero PTE.
//
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     Returns 0 if the PTE is demand zero, non-zero otherwise.
//
//--

#define IS_PTE_NOT_DEMAND_ZERO(PTE) ((PTE).u.Long & (ULONG)0xFFFFF007)


//++
//VOID
//MI_MAKING_VALID_PTE_INVALID(
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    Prepare to make a single valid PTE invalid.
//    No action is required on x86.
//
// Argments
//
//    SYSTEM_WIDE - Supplies TRUE if this will happen on all processors.
//
// Return Value:
//
//    None.
//
//--

#define MI_MAKING_VALID_PTE_INVALID(SYSTEM_WIDE)


//++
//VOID
//MI_MAKING_VALID_MULTIPLE_PTES_INVALID(
//    IN PMMPTE PPTE
//    );
//
// Routine Description:
//
//    Prepare to make multiple valid PTEs invalid.
//    No action is required on x86.
//
// Argments
//
//    SYSTEM_WIDE - Supplies TRUE if this will happen on all processors.
//
// Return Value:
//
//    None.
//
//--

#define MI_MAKING_MULTIPLE_PTES_INVALID(SYSTEM_WIDE)


//
// Make a writable PTE, writeable-copy PTE.  This takes advantage of
// the fact that the protection field in the PTE (5 bit protection) is
// set up such that write is a bit.
//

#define MI_MAKE_PROTECT_WRITE_COPY(PTE) \
        if ((PTE).u.Long & 0x20) {      \
            ((PTE).u.Long |= 0x8);      \
        }


//++
//VOID
//MI_SET_PAGE_DIRTY(
//    IN PMMPTE PPTE,
//    IN PVOID VA,
//    IN PVOID PFNHELD
//    );
//
// Routine Description:
//
//    This macro sets the dirty bit (and release page file space).
//
// Argments
//
//    TEMP - Supplies a temporary for usage.
//
//    PPTE - Supplies a pointer to the PTE that corresponds to VA.
//
//    VA - Supplies a the virtual address of the page fault.
//
//    PFNHELD - Supplies TRUE if the PFN lock is held.
//
// Return Value:
//
//    None.
//
//--

#define MI_SET_PAGE_DIRTY(PPTE,VA,PFNHELD)                          \
            if ((PPTE)->u.Hard.Dirty == MM_PTE_CLEAN) {             \
                MiSetDirtyBit ((VA),(PPTE),(PFNHELD));              \
            }


//++
//VOID
//MI_NO_FAULT_FOUND(
//    IN TEMP,
//    IN PMMPTE PPTE,
//    IN PVOID VA,
//    IN PVOID PFNHELD
//    );
//
// Routine Description:
//
//    This macro handles the case when a page fault is taken and no
//    PTE with the valid bit clear is found.
//
// Argments
//
//    TEMP - Supplies a temporary for usage.
//
//    PPTE - Supplies a pointer to the PTE that corresponds to VA.
//
//    VA - Supplies a the virtual address of the page fault.
//
//    PFNHELD - Supplies TRUE if the PFN lock is held.
//
// Return Value:
//
//    None.
//
//--

#define MI_NO_FAULT_FOUND(TEMP,PPTE,VA,PFNHELD)    \
            if (StoreInstruction && ((PPTE)->u.Hard.Dirty == MM_PTE_CLEAN)) { \
                MiSetDirtyBit ((VA),(PPTE),(PFNHELD));     \
            } else {                        \
                KeFillEntryTb ((PHARDWARE_PTE)PPTE, VA, FALSE);   \
            }
//            KeFillEntryTb((PHARDWARE_PTE)(MiGetPdeAddress(VA)),(PVOID)PPTE,FALSE);
            //
            // If the PTE was already valid, assume that the PTE
            // in the TB is stall and just reload the PTE.
            //


//++
//ULONG
//MI_CAPTURE_DIRTY_BIT_TO_PFN (
//    IN PMMPTE PPTE,
//    IN PMMPFN PPFN
//    );
//
// Routine Description:
//
//    This macro gets captures the state of the dirty bit to the PFN
//    and frees any associated page file space if the PTE has been
//    modified element.
//
//    NOTE - THE PFN LOCK MUST BE HELD!
//
// Argments
//
//    PPTE - Supplies the PTE to operate upon.
//
//    PPFN - Supplies a pointer to the PFN database element that corresponds
//           to the page mapped by the PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_CAPTURE_DIRTY_BIT_TO_PFN(PPTE,PPFN) \
         if (((PPFN)->u3.e1.Modified == 0) &&  \
                      ((PPTE)->u.Hard.Dirty == MM_PTE_DIRTY)) { \
             (PPFN)->u3.e1.Modified = 1;  \
             if (((PPFN)->OriginalPte.u.Soft.Prototype == 0) &&     \
                          ((PPFN)->u3.e1.WriteInProgress == 0)) {  \
                 MiReleasePageFileSpace ((PPFN)->OriginalPte);    \
                 (PPFN)->OriginalPte.u.Soft.PageFileHigh = 0;     \
             }                                                     \
         }


//++
//BOOLEAN
//MI_IS_PHYSICAL_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro deterines if a give virtual address is really a
//    physical address.
//
// Argments
//
//    VA - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a physical address, TRUE if it is.
//
//--

#define MI_IS_PHYSICAL_ADDRESS(Va) \
     (((ULONG)Va >= KSEG0_BASE) && ((ULONG)Va < KSEG2_BASE))


//++
//ULONG
//MI_CONVERT_PHYSICAL_TO_PFN (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro converts a physical address (see MI_IS_PHYSICAL_ADDRESS)
//    to its corresponding physical frame number.
//
// Argments
//
//    VA - Supplies a pointer to the physical address.
//
// Return Value:
//
//    Returns the PFN for the page.
//
//--

#define MI_CONVERT_PHYSICAL_TO_PFN(Va)    (((ULONG)Va << 2) >> 14)


typedef struct _MMCOLOR_TABLES {
    ULONG Flink;
    PVOID Blink;
} MMCOLOR_TABLES, *PMMCOLOR_TABLES;

typedef struct _MMPRIMARY_COLOR_TABLES {
    LIST_ENTRY ListHead;
} MMPRIMARY_COLOR_TABLES, *PMMPRIMARY_COLOR_TABLES;


#if MM_MAXIMUM_NUMBER_OF_COLORS > 1
extern MMPFNLIST MmFreePagesByPrimaryColor[2][MM_MAXIMUM_NUMBER_OF_COLORS];
#endif

extern PMMCOLOR_TABLES MmFreePagesByColor[2];

extern ULONG MmTotalPagesForPagingFile;



//
// A valid Page Table Entry has the following definition.
//

// N.B. defined as in comments below in ../public/sdk/inc/ntppc.h

// typedef struct _HARDWARE_PTE {
//     ULONG Dirty : 2;
//     ULONG Valid : 1;                 // software
//     ULONG GuardedStorage : 1;
//     ULONG MemoryCoherence : 1;
//     ULONG CacheDisable : 1;
//     ULONG WriteThrough : 1;
//     ULONG Change : 1;
//     ULONG Reference : 1;
//     ULONG Write : 1;                 // software
//     ULONG CopyOnWrite : 1;           // software
//     ULONG rsvd1 : 1;
//     ULONG PageFrameNumber : 20;
// } HARDWARE_PTE, *PHARDWARE_PTE;


//
// Invalid Page Table Entries have the following definitions.
//

typedef struct _MMPTE_TRANSITION {
    ULONG Prototype : 1;
    ULONG Transition : 1;
    ULONG Valid : 1;
    ULONG Protection : 5;
    ULONG filler4 : 4;
    ULONG PageFrameNumber : 20;
} MMPTE_TRANSITION;

typedef struct _MMPTE_SOFTWARE {
    ULONG Prototype : 1;
    ULONG Transition : 1;
    ULONG Valid : 1;
    ULONG Protection : 5;
    ULONG PageFileLow : 4;
    ULONG PageFileHigh : 20;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_PROTOTYPE {
    ULONG Prototype : 1;
    ULONG filler1 : 1;
    ULONG Valid : 1;
    ULONG ReadOnly : 1;
    ULONG ProtoAddressLow : 8;
    ULONG ProtoAddressHigh : 20;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_SUBSECTION {
    ULONG Prototype : 1;
    ULONG SubsectionAddressLow : 1;
    ULONG Valid : 1;
    ULONG Protection : 5;
    ULONG SubsectionAddressHigh : 24;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST {
    ULONG filler2 : 2;
    ULONG Valid : 1;
    ULONG OneEntry : 1;
    ULONG filler8 : 8;
    ULONG NextEntry : 20;
} MMPTE_LIST;


//
// A Page Table Entry has the following definition.
//

typedef struct _MMPTE {
    union  {
        ULONG Long;
        HARDWARE_PTE Hard;
        HARDWARE_PTE Flush;
        MMPTE_TRANSITION Trans;
        MMPTE_SOFTWARE Soft;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SUBSECTION Subsect;
        MMPTE_LIST List;
        } u;
} MMPTE;

typedef MMPTE *PMMPTE;

