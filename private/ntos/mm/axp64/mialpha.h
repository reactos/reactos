/*++

Copyright (c) 1990 Microsoft Corporation
Copyright (c) 1992 Digital Equipment Corporation

Module Name:

    mialpha.h

Abstract:

    This module contains the private data structures and procedure
    prototypes for the hardware dependent portion of the
    memory management system.

    It is specifically tailored for the DEC ALPHA architecture.

Author:
    Lou Perazzoli (loup) 12-Mar-1990
    Joe Notarangelo  23-Apr-1992   ALPHA version

Revision History:

    Landy Wang (landyw) 02-June-1998 : Modifications for full 3-level 64-bit NT.

--*/

/*++

    Virtual Memory Layout on the AXP64 is:

                 +------------------------------------+
0000000000000000 | User mode addresses - 4tb          |
                 |                                    |
000003FFFFFEFFFF |                                    | MM_HIGHEST_USER_ADDRESS
                 +------------------------------------+
000003FFFFFF0000 | 64k No Access Region               | MM_USER_PROBE_ADDRESS
                 +------------------------------------+

                 +------------------------------------+
FFFFFC0000000000 | Start of System space and 2tb of   | MM_SYSTEM_RANGE_START
                 | physically addressable memory.     |     KSEG43_BASE
                 |                                    |
                 +------------------------------------+
FFFFFE0000000000 | 8gb three level page table map.    | PTE_BASE
                 +------------------------------------+     KSEG43_LIMIT
FFFFFE0200000000 | HyperSpace - working set lists     | HYPER_SPACE
                 |  and per process memory management |
                 |  structures mapped in this 8gb     |
                 |  region.                           | HYPER_SPACE_END
                 +------------------------------------+     MM_WORKING_SET_END
FFFFFE0400000000 | win32k.sys                         |
                 |                                    |
                 | Non-Hydra configurations also have |
                 | 48mb of system mapped views here.  |
                 |                                    |
                 | Hydra configurations have session  |
                 | data structures here instead of    |
                 | system mapped views.               |
                 |                                    |
                 | This is an 8gb region.             |
                 +------------------------------------+
FFFFFE0600000000 |   The system cache working set     | MM_SYSTEM_CACHE_WORKING_SET
                                                        MM_SYSTEM_SPACE_START
                 |   information resides in this 8gb  |
                 |   region.                          |
                 +------------------------------------+
FFFFFE0800000000 | System cache resides here.         | MM_SYSTEM_CACHE_START
                 |  Kernel mode access only.          |
                 |  1tb.                              |
                 |                                    | MM_SYSTEM_CACHE_END
                 +------------------------------------+
FFFFFF0800000000 | Start of paged system area.        | MM_PAGED_POOL_START
                 |  Kernel mode access only.          |
                 |  128gb.                            |
                 +------------------------------------+
                 |                                    |
                                   .
                                   .

In general, the next two areas (system PTE pool and nonpaged pool) will both
be shifted upwards to conserve a PPE...

                                   .
                                   .
                 +------------------------------------+
FFFFFF2800000000 | System PTE pool.                   | MM_LOWEST_NONPAGED_SYSTEM_START
                 |  Kernel mode access only.          |
                 |  128gb.                            |
                 +------------------------------------+
FFFFFF4800000000 | NonPaged pool.                     | MM_NON_PAGED_POOL_START
                 |  Kernel mode access only.          |
                 |  128gb.                            |
                 |                                    |
FFFFFF67FFFFFFFF |  NonPaged System area              | MM_NONPAGED_POOL_END
                 +------------------------------------+
                 |                                    |
                                   .
                                   .
                                   .
                 +------------------------------------+
FFFFFFFF80000000 | The HAL, kernel, initial drivers,  | KSEG0_BASE
                 | NLS data, and registry load in the |
                 | first 16mb of this region which    |
                 | physically addresses memory.       |
                 |                                    |
                 | Kernel mode access only.           |
                 |                                    |
                 | Initial NonPaged Pool is within    |
                 | KSEG0                              |
                 |                                    |
                 +------------------------------------+
FFFFFFFFC0000000 | Unused.                            | KSEG2_BASE
                 |                                    |
                 |                                    |
                 |                                    |
                 +------------------------------------+
FFFFFFFFFC000000 | Hydra configurations only have     | MM_SYSTEM_VIEW_START
                 | system mapped views here.          |
                 | 48mb.                              |
                 +------------------------------------+
FFFFFFFFFF000000 | Shared system page                 | KI_USER_SHARED_DATA
                 +------------------------------------+
FFFFFFFFFF002000 | Reserved for the HAL.              |
                 |                                    |
                 |                                    |
FFFFFFFFFFFFFFFF |                                    | MM_SYSTEM_SPACE_END
                 +------------------------------------+

--*/

//
// Define empty list marker.
//

#define MM_EMPTY_LIST (-1)              //
#define MM_EMPTY_PTE_LIST 0xFFFFFFFFUI64 // N.B. tied to MMPTE definition

#define MI_PTE_BASE_FOR_LOWEST_KERNEL_ADDRESS (MiGetPteAddress (PTE_BASE))

//
// Define start of KSEG0.
//

#define MM_KSEG0_BASE KSEG0_BASE        //

//
// 43-Bit virtual address mask.
//

#define MASK_43 0x7FFFFFFFFFFUI64       //

//
// Top level page parent is the same for both kernel and user in AXP64.
//

#define PDE_KTBASE  PDE_TBASE

//
// Address space definitions.
//

#define PDE_TOP 0xFFFFFE01FFFFFFFFUI64

#define MM_PAGES_IN_KSEG0 (ULONG)(((KSEG2_BASE - KSEG0_BASE) >> PAGE_SHIFT))

#define MM_USER_ADDRESS_RANGE_LIMIT    0xFFFFFFFFFFFFFFFF // user address range limit
#define MM_MAXIMUM_ZERO_BITS 53         // maximum number of zero bits

#define MM_SYSTEM_SPACE_START 0xFFFFFE0600000000UI64

#define MM_SYSTEM_CACHE_START 0xFFFFFE0800000000UI64

#define MM_SYSTEM_CACHE_END   0xFFFFFF0800000000UI64

#define MM_MAXIMUM_SYSTEM_CACHE_SIZE \
    ((MM_SYSTEM_CACHE_END - MM_SYSTEM_CACHE_START) >> PAGE_SHIFT)

#define MM_SYSTEM_CACHE_WORKING_SET 0xFFFFFE0600000000UI64

//
// Define area for mapping views into system space.
//

#define MM_SESSION_SPACE_DEFAULT 0xFFFFFE0400000000UI64

#define MM_SYSTEM_VIEW_START MM_SESSION_SPACE_DEFAULT

#define MM_SYSTEM_VIEW_START_IF_HYDRA (KI_USER_SHARED_DATA - MM_SYSTEM_VIEW_SIZE)

#define MM_SYSTEM_VIEW_SIZE (48 * 1024 * 1024)

#define MM_SYSTEM_VIEW_SIZE_IF_HYDRA MM_SYSTEM_VIEW_SIZE

#define MM_PAGED_POOL_START ((PVOID)0xFFFFFF0800000000)

#define MM_LOWEST_NONPAGED_SYSTEM_START ((PVOID)0xFFFFFF2800000000)

#define MM_NONPAGED_POOL_END ((PVOID)(0xFFFFFF6800000000 - (16 * PAGE_SIZE)))

#define NON_PAGED_SYSTEM_END ((PVOID)0xFFFFFFFFFFFFFFF0)  //quadword aligned.

#define MM_SYSTEM_SPACE_END 0xFFFFFFFFFFFFFFFFUI64

//
// Define absolute minimum and maximum count for system PTEs.
//

#define MM_MINIMUM_SYSTEM_PTES 5000
#define MM_MAXIMUM_SYSTEM_PTES (16*1024*1024)
#define MM_DEFAULT_SYSTEM_PTES 11000

//
// Pool limits.
//
// The maximum amount of nonpaged pool that can be initially created.
//

#define MM_MAX_INITIAL_NONPAGED_POOL (96 * 1024 * 1024)

//
// The total amount of nonpaged pool.
//

#define MM_MAX_ADDITIONAL_NONPAGED_POOL (((SIZE_T)128 * 1024 * 1024 * 1024) - 16)

//
// The maximum amount of paged pool that can be created.
//

#define MM_MAX_PAGED_POOL ((SIZE_T)128 * 1024 * 1024 * 1024)

//
// Define the maximum default for pool (user specified 0 in registry).
//

#define MM_MAX_DEFAULT_NONPAGED_POOL ((SIZE_T)8 * 1024 * 1024 * 1024)

//
// Granularity Hint definitions.
//

//
// Granularity Hint = 3, page size = 8**3 * PAGE_SIZE
//

#define GH3 (3)
#define GH3_PAGE_SIZE  (PAGE_SIZE << 9)

//
// Granularity Hint = 2, page size = 8**2 * PAGE_SIZE
//

#define GH2 (2)
#define GH2_PAGE_SIZE  (PAGE_SIZE << 6)

//
// Granularity Hint = 1, page size = 8**1 * PAGE_SIZE
//

#define GH1 (1)
#define GH1_PAGE_SIZE  (PAGE_SIZE << 3)

//
// Granularity Hint = 0, page size = PAGE_SIZE
//

#define GH0 (0)
#define GH0_PAGE_SIZE  PAGE_SIZE


//
// Physical memory size and boundary constants.
//

#define __1GB (0x40000000)

//
// PAGE_SIZE for ALPHA (at least current implementation) is 8k
// PAGE_SHIFT bytes for an offset leaves 19
//

#define MM_VIRTUAL_PAGE_FILLER (13 - 12)
#define MM_VIRTUAL_PAGE_SIZE (43 - 13)


#define MM_PROTO_PTE_ALIGNMENT ((ULONG)MM_MAXIMUM_NUMBER_OF_COLORS * (ULONG)PAGE_SIZE)

//
// Define maximum number of paging files
//

#define MAX_PAGE_FILES (8)

//
// Define the address bits mapped by one PPE entry.
//

#define PAGE_PARENT_MASK 0x1FFFFFFFFUI64

#define MM_VA_MAPPED_BY_PPE (0x200000000UI64)

//
// Define the address bits mapped by PPE and PDE entries.
//
// A PPE entry maps 10+10+13 = 33 bits of address space.
// A PDE entry maps 10+13 = 23 bits of address space.
//

#define PAGE_DIRECTORY1_MASK 0x1FFFFFFFFUI64
#define PAGE_DIRECTORY2_MASK 0x7FFFFFUI64

#define MM_VA_MAPPED_BY_PDE (0x800000)

#define LOWEST_IO_ADDRESS (0)

#define PTE_SHIFT (3)

//
// Number of physical address bits, maximum for ALPHA architecture = 48.
//

#define PHYSICAL_ADDRESS_BITS (48)

#define MM_MAXIMUM_NUMBER_OF_COLORS 1

//
// Alpha does not require support for colored pages.
//

#define MM_NUMBER_OF_COLORS (1)

//
// Mask for obtaining color from a physical page number.
//

#define MM_COLOR_MASK (0)

//
// Boundary for aligned pages of like color upon.
//

#define MM_COLOR_ALIGNMENT (0)

//
// Mask for isolating color from virtual address.
//

#define MM_COLOR_MASK_VIRTUAL (0)

//
//  Define 1mb worth of secondary colors.
//

#define MM_SECONDARY_COLORS_DEFAULT ((1024 * 1024) >> PAGE_SHIFT)

#define MM_SECONDARY_COLORS_MIN (2)

#define MM_SECONDARY_COLORS_MAX (2048)

//
// Mask for isolating secondary color from physical page number;
//

extern ULONG MmSecondaryColorMask;

//
// Hyper space definitions.
//
// Hyper space consists of a single top level page directory parent entry
// that maps a series of PDE/PTEs that can be used for temporary per process
// mapping and the working set list.
//

#define HYPER_SPACE     ((PVOID)0xFFFFFE0200000000)

#define FIRST_MAPPING_PTE       0xFFFFFE0200000000UI64

#define NUMBER_OF_MAPPING_PTES 639              // LWFIX

#define LAST_MAPPING_PTE   \
    (FIRST_MAPPING_PTE + (NUMBER_OF_MAPPING_PTES * PAGE_SIZE))

#define IMAGE_MAPPING_PTE ((PMMPTE)((ULONG_PTR)LAST_MAPPING_PTE + PAGE_SIZE))

#define ZEROING_PAGE_PTE ((PMMPTE)((ULONG_PTR)IMAGE_MAPPING_PTE + PAGE_SIZE))

#define WORKING_SET_LIST ((PVOID)((ULONG_PTR)ZEROING_PAGE_PTE + PAGE_SIZE))

#define MM_MAXIMUM_WORKING_SET \
    ((((ULONG_PTR)4 * 1024 * 1024 * 1024 * 1024) - (64 * 1024 * 1024)) >> PAGE_SHIFT) //4Tb-64Mb

#define HYPER_SPACE_END 0xFFFFFE03FFFFFFFFUI64

#define MM_WORKING_SET_END 0xFFFFFE0400000000UI64

//
// Define PTE mask bits.
//
// These definitions are derived from the hardware PTE format and from the
// software PTE formats. They are defined as masks to avoid the cost of
// shifting and masking to insert and extract these fields.
//

#define MM_PTE_VALID_MASK         0x1105 // kernel read-write, fault-on-write, valid
#define MM_PTE_PROTOTYPE_MASK     0x2   // not valid and prototype
#define MM_PTE_DIRTY_MASK         0x4   // fault on write
#define MM_PTE_TRANSITION_MASK    0x4   // not valid and transition
#define MM_PTE_GLOBAL_MASK        0x10  // global
#define MM_PTE_WRITE_MASK         0x10000 // software write
#define MM_PTE_COPY_ON_WRITE_MASK 0x20000 // software copy-on-write
#define MM_PTE_OWNER_MASK         0x2200 // user read-write

//
// Bit fields to or into PTE to make a PTE valid based on the protection
// field of the invalid PTE.
//

#define MM_PTE_NOACCESS          0x0    // not expressable on ALPHA
#define MM_PTE_READONLY          0x4    // fault on write
#define MM_PTE_READWRITE         (MM_PTE_WRITE_MASK) // software write enable
#define MM_PTE_WRITECOPY         (MM_PTE_WRITE_MASK | MM_PTE_COPY_ON_WRITE_MASK) //
#define MM_PTE_EXECUTE           0x4    // fault on write
#define MM_PTE_EXECUTE_READ      0x4    // fault on write
#define MM_PTE_EXECUTE_READWRITE (MM_PTE_WRITE_MASK) // software write enable
#define MM_PTE_EXECUTE_WRITECOPY (MM_PTE_WRITE_MASK | MM_PTE_COPY_ON_WRITE_MASK) //
#define MM_PTE_NOCACHE           0x0    // not expressable on ALPHA
#define MM_PTE_GUARD             0x0    // not expressable on ALPHA
#define MM_PTE_CACHE             0x0    //

#define MM_PROTECT_FIELD_SHIFT 3

//
// Bits available for the software working set index within the hardware PTE.
//

#define MI_MAXIMUM_PTE_WORKING_SET_INDEX (1 << _HARDWARE_PTE_WORKING_SET_BITS)

//
// Zero PTE
//

#define MM_ZERO_PTE 0

//
// Zero Kernel PTE
//

#define MM_ZERO_KERNEL_PTE 0

//
// A demand zero PTE with a protection or PAGE_READWRITE.
//

#define MM_DEMAND_ZERO_WRITE_PTE (MM_READWRITE << MM_PROTECT_FIELD_SHIFT)

//
// A demand zero PTE with a protection or PAGE_READWRITE for system space.
//

#define MM_KERNEL_DEMAND_ZERO_PTE (MM_READWRITE << MM_PROTECT_FIELD_SHIFT)

//
// A no access PTE for system space.
//

#define MM_KERNEL_NOACCESS_PTE (MM_NOACCESS << MM_PROTECT_FIELD_SHIFT)

//
// Dirty bit definitions for clean and dirty.
//

#define MM_PTE_CLEAN 1
#define MM_PTE_DIRTY 0

//
// Kernel stack alignment requirements.
//

#define MM_STACK_ALIGNMENT 0x0
#define MM_STACK_OFFSET 0x0

//
// System process definitions
//

#define PDE_PER_PAGE 1024
#define PTE_PER_PAGE 1024
#define PTE_PER_PAGE_BITS 11    // This handles the case where the page is full

#if PTE_PER_PAGE_BITS > 32
error - too many bits to fit into MMPTE_SOFTWARE or MMPFN.u1
#endif

//
// Number of page table pages for user addresses.
//

#define MM_USER_PAGE_TABLE_PAGES 1024

//++
//VOID
//MI_MAKE_VALID_PTE (
//    OUT OUTPTE,
//    IN FRAME,
//    IN PMASK,
//    IN PPTE
//    );
//
// Routine Description:
//
//    This macro makes a valid PTE from a page frame number, protection
//    mask, and owner.
//
// Arguments
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

#define MI_MAKE_VALID_PTE(OUTPTE, FRAME, PMASK, PPTE) {             \
    (OUTPTE).u.Long = MmProtectToPteMask[PMASK] | MM_PTE_VALID_MASK; \
    (OUTPTE).u.Hard.PageFrameNumber = (FRAME);                      \
    if (MI_DETERMINE_OWNER(PPTE)) {                                 \
        (OUTPTE).u.Long |= MM_PTE_OWNER_MASK;                       \
    }                                                               \
    if (((PMMPTE)PPTE) >= MiGetPteAddress(MM_SYSTEM_SPACE_START)) { \
        (OUTPTE).u.Hard.Global = 1;                                 \
    }                                                               \
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
// Arguments
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

#define MI_MAKE_VALID_PTE_TRANSITION(OUTPTE, PROTECT)     \
                (OUTPTE).u.Soft.Transition = 1;           \
                (OUTPTE).u.Soft.Valid = 0;                \
                (OUTPTE).u.Soft.Prototype = 0;            \
                (OUTPTE).u.Soft.Protection = PROTECT;

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
// Arguments
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
// Arguments
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

#define MI_MAKE_TRANSITION_PTE_VALID(OUTPTE, PPTE) {                \
    (OUTPTE).u.Long = MmProtectToPteMask[(PPTE)->u.Trans.Protection] | MM_PTE_VALID_MASK; \
    (OUTPTE).u.Hard.PageFrameNumber = (PPTE)->u.Hard.PageFrameNumber; \
    if (MI_DETERMINE_OWNER(PPTE)) {                                 \
        (OUTPTE).u.Long |= MM_PTE_OWNER_MASK;                       \
    }                                                               \
    if (((PMMPTE)PPTE) >= MiGetPteAddress(MM_SYSTEM_SPACE_START)) { \
        (OUTPTE).u.Hard.Global = 1;                                 \
    }                                                               \
}

//++
//VOID
//MI_SET_PTE_IN_WORKING_SET (
//    OUT PMMPTE PTE,
//    IN ULONG WSINDEX
//    );
//
// Routine Description:
//
//    This macro inserts the specified working set index into the argument PTE.
//
//    No TB invalidation is needed for other processors (or this one) even
//    though the entry may already be in a TB - it's just a software field
//    update and doesn't affect miss resolution.
//
// Arguments
//
//    OUTPTE - Supplies the PTE in which to insert the working set index.
//
//    WSINDEX - Supplies the working set index for the PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_IN_WORKING_SET(PTE, WSINDEX) {             \
    MMPTE _TempPte;                                           \
    _TempPte = *(PTE);                                        \
    _TempPte.u.Hard.SoftwareWsIndex = (WSINDEX);              \
    ASSERT (_TempPte.u.Long != 0);                            \
    *(PTE) = _TempPte;                                        \
}

//++
//ULONG WsIndex
//MI_GET_WORKING_SET_FROM_PTE(
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro returns the working set index from the argument PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to extract the working set index from.
//
// Return Value:
//
//    This macro returns the working set index for the argument PTE.
//
//--

#define MI_GET_WORKING_SET_FROM_PTE(PTE)  (ULONG)(PTE)->u.Hard.SoftwareWsIndex

//++
//VOID
//MI_SET_PTE_WRITE_COMBINE (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro sets the write combined bit(s) in the specified PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to set dirty.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_WRITE_COMBINE(PTE)  // fixfix - to be done

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
// Arguments
//
//    PTE - Supplies the PTE to set dirty.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_DIRTY(PTE) (PTE).u.Hard.FaultOnWrite = MM_PTE_DIRTY

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
// Arguments
//
//    PTE - Supplies the PTE to set clear.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_PTE_CLEAN(PTE) (PTE).u.Hard.FaultOnWrite = MM_PTE_CLEAN

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
// Arguments
//
//    PTE - Supplies the PTE to check.
//
// Return Value:
//
//    TRUE if the page is dirty (modified), FALSE otherwise.
//
//--

#define MI_IS_PTE_DIRTY(PTE) ((PTE).u.Hard.FaultOnWrite != MM_PTE_CLEAN)

//++
// VOID
// MI_SET_GLOBAL_BIT_IF_SYSTEM (
//     OUT OUTPTE,
//     IN PPTE
//     );
//
// Routine Description:
//
//    This macro sets the global bit if the pointer PTE is within
//    system space.
//
// Arguments
//
//    OUTPTE - Supplies the PTE in which to build the valid PTE.
//
//    PPTE - Supplies a pointer to the PTE becoming valid.
//
// Return Value:
//
//    None.
//
//--

#define MI_SET_GLOBAL_BIT_IF_SYSTEM(OUTPTE, PPTE)

//++
// VOID
// MI_SET_GLOBAL_STATE (
//     IN MMPTE PTE,
//     IN ULONG STATE
//     );
//
// Routine Description:
//
//    This macro sets the global bit in the PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to set global state into.
//
// Return Value:
//
//    None.
//
//--

#define MI_SET_GLOBAL_STATE(PTE, STATE) (PTE).u.Hard.Global = STATE;

//++
// VOID
// MI_ENABLE_CACHING (
//     IN MMPTE PTE
//     );
//
// Routine Description:
//
//    This macro takes a valid PTE and sets the caching state to be
//    enabled.
//
// Arguments
//
//    PTE - Supplies a valid PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_ENABLE_CACHING(PTE)

//++
// VOID
// MI_DISABLE_CACHING (
//     IN MMPTE PTE
//     );
//
// Routine Description:
//
//    This macro takes a valid PTE and sets the caching state to be
//    disabled.
//
//    N.B. This function performs no operation on Alpha. Caching is
//         never disabled.
//
// Arguments
//
//    PTE - Supplies a valid PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_DISABLE_CACHING(PTE)

//++
// BOOLEAN
// MI_IS_CACHING_DISABLED (
//     IN PMMPTE PPTE
//     );
//
// Routine Description:
//
//    This macro takes a valid PTE and returns TRUE if caching is
//    disabled.
//
//    N.B. This function always return FALSE for alpha.
//
// Arguments
//
//    PPTE - Supplies a pointer to the valid PTE.
//
// Return Value:
//
//    FALSE.
//
//--

#define MI_IS_CACHING_DISABLED(PPTE) FALSE

//++
// VOID
// MI_SET_PFN_DELETED (
//     IN PMMPFN PPFN
//     );
//
// Routine Description:
//
//    This macro takes a pointer to a PFN element and indicates that
//    the PFN is no longer in use.
//
// Arguments
//
//    PPTE - Supplies a pointer to the PFN element.
//
// Return Value:
//
//    none.
//
//--

#define MI_SET_PFN_DELETED(PPFN) \
    (((ULONG_PTR)(PPFN)->PteAddress) = ((((ULONG_PTR)(PPFN)->PteAddress) << 1) >> 1))

//++
//BOOLEAN
//MI_IS_PFN_DELETED (
//    IN PMMPFN PPFN
//    );
//
// Routine Description:
//
//    This macro takes a pointer to a PFN element and determines if
//    the PFN is no longer in use.
//
// Arguments
//
//    PPTE - Supplies a pointer to the PFN element.
//
// Return Value:
//
//     TRUE if PFN is no longer used, FALSE if it is still being used.
//
//--

#define MI_IS_PFN_DELETED(PPFN) \
    ((((ULONG_PTR)(PPFN)->PteAddress) >> 63) == 0)

//++
// VOID
// MI_CHECK_PAGE_ALIGNMENT (
//    IN ULONG PAGE,
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro takes a PFN element number (Page) and checks to see
//    if the virtual alignment for the previous address of the page
//    is compatible with the new address of the page.  If they are
//    not compatible, the D cache is flushed.
//
// Arguments
//
//    PAGE - Supplies the PFN element.
//    COLOR - Supplies the new page color of the page.
//
// Return Value:
//
//    none.
//
//--

#define MI_CHECK_PAGE_ALIGNMENT(PAGE, COLOR)

//++
//VOID
//MI_INITIALIZE_HYPERSPACE_MAP (
//    VOID
//    );
//
// Routine Description:
//
//    This macro initializes the PTEs reserved for double mapping within
//    hyperspace.
//
// Arguments
//
//    None.
//
// Return Value:
//
//    None.
//
//--

#define MI_INITIALIZE_HYPERSPACE_MAP(HYPER_PAGE)

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
// Arguments
//
//    PTEADDRESS - Supplies the PTE address the page is (or was) mapped at.
//
// Return Value:
//
//    The page's color.
//
//--

#define MI_GET_PAGE_COLOR_FROM_PTE(PTEADDRESS)  \
         ((ULONG)((MmSystemPageColor++) & MmSecondaryColorMask))

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
// Arguments
//
//    ADDRESS - Supplies the address the page is (or was) mapped at.
//
// Return Value:
//
//    The pages color.
//
//--

#define MI_GET_PAGE_COLOR_FROM_VA(ADDRESS)  \
         ((ULONG)((MmSystemPageColor++) & MmSecondaryColorMask))

//++
//ULONG
//MI_GET_PAGE_COLOR_FROM_SESSION (
//    IN PMM_SESSION_SPACE SessionSpace
//    );
//
// Routine Description:
//
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//    SessionSpace - Supplies the session space the page will be mapped into.
//
// Return Value:
//
//    The page's color.
//
//--


#define MI_GET_PAGE_COLOR_FROM_SESSION(_SessionSpace)  \
         ((ULONG)((_SessionSpace->Color++) & MmSecondaryColorMask))

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
// Arguments
//
//
// Return Value:
//
//    The pages color.
//
//--


#define MI_PAGE_COLOR_PTE_PROCESS(PTE,COLOR)  \
         ((ULONG)((*(COLOR))++) & MmSecondaryColorMask)

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
// Arguments
//
//    ADDRESS - Supplies the address the page is (or was) mapped at.
//
// Return Value:
//
//    The pages color.
//
//--

#define MI_PAGE_COLOR_VA_PROCESS(ADDRESS,COLOR) \
         ((ULONG)((*(COLOR))++) & MmSecondaryColorMask)

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
// Arguments
//
//    COLOR - Supplies the color to return the next of.
//
// Return Value:
//
//    Next color in sequence.
//
//--

#define MI_GET_NEXT_COLOR(COLOR) ((COLOR+1) & MM_COLOR_MASK)

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
// Arguments
//
//    COLOR - Supplies the color to return the previous of.
//
// Return Value:
//
//    Previous color in sequence.
//
//--

#define MI_GET_PREVIOUS_COLOR(COLOR) ((COLOR-1) & MM_COLOR_MASK)

#define MI_GET_SECONDARY_COLOR(PAGE,PFN) ((ULONG)(PAGE & MmSecondaryColorMask))

#define MI_GET_COLOR_FROM_SECONDARY(SECONDARY_COLOR) (0)

//++
//VOID
//MI_GET_MODIFIED_PAGE_BY_COLOR (
//    OUT ULONG PAGE,
//    IN ULONG COLOR
//    );
//
// Routine Description:
//
//    This macro returns the first page destined fro a paging
//    file with the desired color.  It does NOT remove the page
//    from its list.
//
// Arguments
//
//    PAGE - Returns the page located, the value MM_EMPTY_LIST is
//           returned if there is no page of the specified color.
//
//    COLOR - Supplies the color of page to locate.
//
// Return Value:
//
//    None.
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
// Arguments
//
//    PAGE - Returns the page located, the value MM_EMPTY_LIST is
//           returned if there  is no page of the specified color.
//
//    COLOR - Supplies the color of the page to locate and returns the
//            color of the page located.
//
// Return Value:
//
//    None.
//
//--

#define MI_GET_MODIFIED_PAGE_ANY_COLOR(PAGE,COLOR)                        \
{                                                                         \
    if( MmTotalPagesForPagingFile == 0 ){                                 \
        PAGE = MM_EMPTY_LIST;                                             \
    } else {                                                              \
        while( MmModifiedPageListByColor[COLOR].Flink == MM_EMPTY_LIST ){ \
            COLOR = MI_GET_NEXT_COLOR(COLOR);                             \
        }                                                                 \
        PAGE = MmModifiedPageListByColor[COLOR].Flink;                    \
    }                                                                     \
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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     None.
//
//--

#define MI_MAKE_VALID_PTE_WRITE_COPY(PPTE)                         \
                    if ((PPTE)->u.Hard.Write == 1) {               \
                        (PPTE)->u.Hard.CopyOnWrite = 1;            \
                        (PPTE)->u.Hard.FaultOnWrite = MM_PTE_CLEAN;\
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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     1 if the owner is USER_MODE, 0 if the owner is KERNEL_MODE.
//
//--

#define MI_DETERMINE_OWNER(PPTE) \
    ((PMMPTE)(PPTE) <= MiHighestUserPte)

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
// Arguments
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
// Arguments
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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    None.
//
//--

#define MI_SET_OWNER_IN_PTE(PPTE, OWNER) \
    ((PPTE)->u.Hard.UserReadAccess = (PPTE)->u.Hard.UserWriteAccess = OWNER)

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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     The state of the OWNER field.
//
//--

#define MI_GET_OWNER_IN_PTE(PPTE) ((PPTE)->u.Hard.UserReadAccess)

//
// Mask to clear all fields but protection in a PTE to or in paging file
// location.
//

#define CLEAR_FOR_PAGE_FILE 0xF8

//++
// ULONG_PTR
// MI_SET_PAGING_FILE_INFO (
//    OUT MMPTE OUTPTE,
//    IN MMPTE PPTE,
//    IN ULONG FILEINFO,
//    IN ULONG OFFSET
//    );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//
// Arguments
//
//    OUTPTE - Supplies the PTE in which to store the result.
//
//    PTE - Supplies the PTE to operate upon.
//
//    FILEINFO - Supplies the number of the paging file.
//
//    OFFSET - Supplies the offset into the paging file.
//
// Return Value:
//
//    PTE Value.
//
//--

#define MI_SET_PAGING_FILE_INFO(OUTPTE,PPTE,FILEINFO,OFFSET)            \
       (OUTPTE).u.Long = (PPTE).u.Long;                                 \
       (OUTPTE).u.Long &= CLEAR_FOR_PAGE_FILE;                          \
       (OUTPTE).u.Long |= ((((FILEINFO) & 0xF) << 28) |                 \
            (((ULONG64)(OFFSET) & 0xFFFFFFFF) << 32));


//++
// PMMPTE
// MiPteToProto (
//     IN OUT MMPTE PPTE
//     );
//
// Routine Description:
//
//    This macro returns the address of the corresponding prototype which
//    was encoded earlier into the supplied PTE.
//
// Arguments
//
//    lpte - Supplies the PTE to operate upon.
//
// Return Value:
//
//    Pointer to the prototype PTE that backs this PTE.
//
//--

#define MiPteToProto(lpte) \
    ((PMMPTE)((lpte)->u.Proto.ProtoAddress))

//++
// ULONG_PTR
// MiProtoAddressForPte (
//     IN PMMPTE proto_va
//     );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//    MiProtoAddressForPte returns the bit field to OR into the PTE to
//    reference a prototype PTE.  And set the MM_PTE_PROTOTYPE_MASK PTE
//    bit.
//
//    N.B. This macro is dependent on the layout of the prototype PTE.
//
// Arguments
//
//    proto_va - Supplies the address of the prototype PTE.
//
// Return Value:
//
//    Mask to set into the PTE.
//
//--

#define MiProtoAddressForPte(proto_va) \
    (((ULONG_PTR)proto_va << 16) | MM_PTE_PROTOTYPE_MASK)

//++
// ULONG_PTR
// MiProtoAddressForKernelPte (
//     IN PMMPTE proto_va
//     );
//
// Routine Description:
//
//    This macro sets into the specified PTE the supplied information
//    to indicate where the backing store for the page is located.
//    MiProtoAddressForPte returns the bit field to OR into the PTE to
//    reference a prototype PTE.  And set the MM_PTE_PROTOTYPE_MASK PTE
//    bit.
//
//    This macro also sets any other information (such as global bits)
//    required for kernel mode PTEs.
//
// Arguments
//
//    proto_va - Supplies the address of the prototype PTE.
//
// Return Value:
//
//    Mask to set into the PTE.
//
//--

#define MiProtoAddressForKernelPte(proto_va) MiProtoAddressForPte(proto_va)

//++
// PSUBSECTION
// MiGetSubsectionAddress (
//     IN PMMPTE lpte
//     );
//
// Routine Description:
//
//    This macro takes a PTE and returns the address of the subsection that
//    the PTE refers to. Subsections are quadword structures allocated from
//    paged and nonpaged pool.
//
// Arguments
//
//    lpte - Supplies the PTE to operate upon.
//
// Return Value:
//
//    A pointer to the subsection referred to by the supplied PTE.
//
//--

#define MiGetSubsectionAddress(lpte) \
    ((PSUBSECTION)((lpte)->u.Subsect.SubsectionAddress))

//++
// ULONG_PTR
// MiGetSubsectionAddressForPte (
//     IN PSUBSECTION VA
//     );
//
//    N.B. This macro is dependent on the layout of the subsection PTE.
//
// Routine Description:
//
//    This macro takes the address of a subsection and encodes it for use
//    in a PTE.
//
// Arguments
//
//    VA - Supplies a pointer to the subsection to encode.
//
// Return Value:
//
//    The mask to set into the PTE to make it reference the supplied
//    subsection.
//
//--

#define MiGetSubsectionAddressForPte(VA) ((ULONG_PTR)VA << 16)

//++
//PMMPTE
//MiGetPpeAddress (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPpeAddress returns the address of the page directory parent entry
//    which maps the given virtual address.  This is one level above the
//    page directory.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the PPE for.
//
// Return Value:
//
//    The address of the PPE.
//
//--

#define MiGetPpeAddress(va)   ((PMMPTE)PDE_TBASE + MiGetPpeOffset(va))

//++
//PMMPTE
//MiGetPdeAddress (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    This funtion computes the address of the second level PDE which maps
//    the given virtual address. The computation is done by recursively
//    applying the computation to find the PTE that maps the virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address for which to compute the second level
//        PDE address.
//
// Return Value:
//
//    The address of the PDE.
//
//--

#define MiGetPdeAddress(va)  \
    MiGetPteAddress(MiGetPteAddress(va))

#define MiGetPdeAddress64(va) \
    MiGetPteAddress(MiGetPteAddress(va))

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
// Arguments
//
//    Va - Supplies the virtual address to locate the PTE for.
//
// Return Value:
//
//    The address of the PTE.
//
//--

#define MiGetPteAddress(va) \
    ((PMMPTE)(((((ULONG_PTR)(va) & MASK_43) >> PTI_SHIFT) << 3) + PTE_BASE))

#define MiGetPteAddress64(va) \
    ((PMMPTE)(((((ULONG_PTR)(va) & MASK_43) >> PTI_SHIFT) << 3) + PTE_BASE))

//++
// ULONG
// MiGetPpeOffset (
//     IN PVOID va
//     );
//
// Routine Description:
//
//    MiGetPpeOffset returns the offset into a page directory parent for a
//    given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the parent page directory table the corresponding
//    PPE is at.
//
//--

#define MiGetPpeOffset(va) ((ULONG)(((ULONG_PTR)(va) >> PDI1_SHIFT) & PDI_MASK))

//++
// ULONG
// MiGetPdeOffset (
//     IN PVOID va
//     );
//
// Routine Description:
//
//    MiGetPdeOffset returns the offset into a page directory for a given
//    virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the page directory table the corresponding PDE is at.
//
//--

#define MiGetPdeOffset(va) ((ULONG)(((ULONG_PTR)(va) >> PDI2_SHIFT) & PDI_MASK))

//++
//ULONG
//MiGetPpePdeOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPpePdeOffset returns the offset into a page directory
//    for a given virtual address.
//
//    N.B. This does not mask off PPE bits.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the page directory (and parent) table the
//    corresponding PDE is at.
//
//--

#define MiGetPpePdeOffset(va) ((ULONG)((ULONG_PTR)(va) >> PDI2_SHIFT))

//++
// ULONG
// MiGetPteOffset (
//     IN PVOID va
//     );
//
// Routine Description:
//
//    MiGetPteOffset returns the offset into a page table page for a given
//    virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the page table page table the corresponding PTE is at.
//
//--

#define MiGetPteOffset(va) ((ULONG)(((ULONG_PTR)(va) >> PTI_SHIFT) & PDI_MASK))

//++
//PVOID
//MiGetVirtualAddressMappedByPpe (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    MiGetVirtualAddressMappedByPpe returns the virtual address
//    which is mapped by a given PPE address.
//
// Arguments
//
//    PPE - Supplies the PPE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PPE.
//
//--

#define MiGetVirtualAddressMappedByPpe(PPE) \
    MiGetVirtualAddressMappedByPte(MiGetVirtualAddressMappedByPde(PPE))

//++
//PVOID
//MiGetVirtualAddressMappedByPde (
//    IN PMMPTE PDE
//    );
//
// Routine Description:
//
//    MiGetVirtualAddressMappedByPde returns the virtual address
//    which is mapped by a given PDE address.
//
// Arguments
//
//    PDE - Supplies the PDE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PDE.
//
//--

#define MiGetVirtualAddressMappedByPde(Pde) \
    MiGetVirtualAddressMappedByPte(MiGetVirtualAddressMappedByPte(Pde))

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
// Arguments
//
//    PTE - Supplies the PTE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PTE.
//
//--

#define MiGetVirtualAddressMappedByPte(Pte) \
    ((PVOID)((LONG_PTR)(((LONG_PTR)(Pte) - PTE_BASE) << (PAGE_SHIFT + VA_SHIFT - 3)) >> VA_SHIFT))

#define MiGetVirtualAddressMappedByPte64(Pte) \
    ((PVOID)((LONG_PTR)(((LONG_PTR)(Pte) - PTE_BASE64) << (PAGE_SHIFT + VA_SHIFT - 3)) >> VA_SHIFT))

#define MiGetVirtualPageNumberMappedByPte64(Pte) \
    ((PVOID)(((ULONG_PTR)(Pte) - PTE_BASE64) >> 3))

//++
//LOGICAL
//MiIsVirtualAddressOnPpeBoundary (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    MiIsVirtualAddressOnPpeBoundary returns TRUE if the virtual address is
//    on a page directory entry boundary.
//
// Arguments
//
//    VA - Supplies the virtual address to check.
//
// Return Value:
//
//    TRUE if on a boundary, FALSE if not.
//
//--

#define MiIsVirtualAddressOnPpeBoundary(VA) (((ULONG_PTR)(VA) & PAGE_DIRECTORY1_MASK) == 0)


//++
//LOGICAL
//MiIsVirtualAddressOnPdeBoundary (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    MiIsVirtualAddressOnPdeBoundary returns TRUE if the virtual address is
//    on a page directory entry boundary.
//
// Arguments
//
//    VA - Supplies the virtual address to check.
//
// Return Value:
//
//    TRUE if on an 8MB PDE boundary, FALSE if not.
//
//--

#define MiIsVirtualAddressOnPdeBoundary(VA) (((ULONG_PTR)(VA) & PAGE_DIRECTORY2_MASK) == 0)


//++
//LOGICAL
//MiIsPteOnPpeBoundary (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    MiIsPteOnPpeBoundary returns TRUE if the PTE is
//    on a page directory parent entry boundary.
//
// Arguments
//
//    VA - Supplies the virtual address to check.
//
// Return Value:
//
//    TRUE if on a boundary, FALSE if not.
//
//--

#define MiIsPteOnPpeBoundary(PTE) (((ULONG_PTR)(PTE) & (MM_VA_MAPPED_BY_PDE - 1)) == 0)


//++
//LOGICAL
//MiIsPteOnPdeBoundary (
//    IN PVOID PTE
//    );
//
// Routine Description:
//
//    MiIsPteOnPdeBoundary returns TRUE if the PTE is
//    on a page directory entry boundary.
//
// Arguments
//
//    PTE - Supplies the PTE to check.
//
// Return Value:
//
//    TRUE if on a 8MB PDE boundary, FALSE if not.
//
//--

#define MiIsPteOnPdeBoundary(PTE) (((ULONG_PTR)(PTE) & (PAGE_SIZE - 1)) == 0)

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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    The paging file number.
//
//--

#define GET_PAGING_FILE_NUMBER(PTE) ((ULONG)(((PTE).u.Soft.PageFileLow)))

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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    The paging file offset.
//
//--

#define GET_PAGING_FILE_OFFSET(PTE) ((ULONG)(((PTE).u.Soft.PageFileHigh)))

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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     Returns 0 if the PTE is demand zero, non-zero otherwise.
//
//--

#define IS_PTE_NOT_DEMAND_ZERO(PTE) ((PTE).u.Long & ~0xFE)

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
// Arguments
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
// Arguments
//
//    SYSTEM_WIDE - Supplies TRUE if this will happen on all processors.
//
// Return Value:
//
//    None.
//
//--

#define MI_MAKING_MULTIPLE_PTES_INVALID(SYSTEM_WIDE)

//++
//VOID
//MI_MAKE_PROTECT_WRITE_COPY (
//    IN OUT MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro makes a writable PTE a writable-copy PTE.
//
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    NONE
//
//--

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
// Arguments
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
            if ((PPTE)->u.Hard.FaultOnWrite == MM_PTE_CLEAN) {      \
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
// Arguments
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

#define MI_NO_FAULT_FOUND(TEMP, PPTE, VA, PFNHELD)                  \
            if (StoreInstruction && ((PPTE)->u.Hard.FaultOnWrite == MM_PTE_CLEAN)) {  \
                MiSetDirtyBit((VA),(PPTE), (PFNHELD));              \
            } else {                                                \
                KiFlushSingleTb(1, VA);                             \
            }

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
// Arguments
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

#define MI_CAPTURE_DIRTY_BIT_TO_PFN(PPTE,PPFN)                               \
         if (((PPFN)->u3.e1.Modified == 0) &&                                \
             ((PPTE)->u.Hard.FaultOnWrite == MM_PTE_DIRTY)) {                       \
             (PPFN)->u3.e1.Modified = 1;                                     \
             if (((PPFN)->OriginalPte.u.Soft.Prototype == 0) &&              \
                          ((PPFN)->u3.e1.WriteInProgress == 0)) {            \
                 MiReleasePageFileSpace ((PPFN)->OriginalPte);               \
                 (PPFN)->OriginalPte.u.Soft.PageFileHigh = 0;                \
             }                                                               \
         }

//++
//BOOLEAN
//MI_IS_PHYSICAL_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro determines if a give virtual address is really a
//    physical address.
//
// Arguments
//
//    VA - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a physical address, TRUE if it is.
//
//--

#define MI_IS_PHYSICAL_ADDRESS(Va) \
    ((((ULONG_PTR)(Va) >= KSEG43_BASE) && ((ULONG_PTR)(Va) < KSEG43_LIMIT)) || \
    (((ULONG_PTR)(Va) >= KSEG0_BASE) && ((ULONG_PTR)(Va) < KSEG2_BASE)))

//++
//PFN_NUMBER
//MI_CONVERT_PHYSICAL_TO_PFN (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro converts a physical address (see MI_IS_PHYSICAL_ADDRESS)
//    to its corresponding physical frame number.
//
// Arguments
//
//    VA - Supplies a pointer to the physical address.
//
// Return Value:
//
//    Returns the PFN for the page.
//
//--

#define MI_CONVERT_PHYSICAL_TO_PFN(Va)                            \
    (((ULONG_PTR)(Va) < KSEG0_BASE) ?                              \
        ((PFN_NUMBER)(((ULONG_PTR)(Va) - KSEG43_BASE) >> PAGE_SHIFT)) : \
        ((PFN_NUMBER)(((ULONG_PTR)(Va) - KSEG0_BASE) >> PAGE_SHIFT)))

//++
// PFN_NUMBER
// MI_CONVERT_PHYSICAL_BUS_TO_PFN(
//   PHYSICAL_ADDRESS Pa,
//   )
//
// Routine Description:
//
//    This macro takes a physical address and returns the pfn to which
//    it corresponds.
//
// Arguments
//
//    Pa - Supplies the physical address to convert.
//
// Return Value:
//
//    The Pfn that corresponds to the physical address is returned.
//
//--

#define MI_CONVERT_PHYSICAL_BUS_TO_PFN(Pa) \
    ((PFN_NUMBER)((Pa).QuadPart >> ((CCHAR)PAGE_SHIFT)))


typedef struct _MMCOLOR_TABLES {
    PFN_NUMBER Flink;
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

#define MI_PTE_LOOKUP_NEEDED ((ULONG64)0xffffffff)


//
// The hardware PTE is defined in ntos\inc\alpha.h.
//
// Invalid PTEs have the following definition.
//

typedef struct _MMPTE_SOFTWARE {
    ULONGLONG Valid: 1;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG Protection : 5;
    ULONGLONG UsedPageTableEntries : PTE_PER_PAGE_BITS;
    ULONGLONG Reserved : 20 - PTE_PER_PAGE_BITS;
    ULONGLONG PageFileLow: 4;
    ULONGLONG PageFileHigh : 32;
} MMPTE_SOFTWARE;


typedef struct _MMPTE_TRANSITION {
    ULONGLONG Valid : 1;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG Protection : 5;
    ULONGLONG filler01 : 24;
    ULONGLONG PageFrameNumber : 32;
} MMPTE_TRANSITION;


typedef struct _MMPTE_PROTOTYPE {
    ULONGLONG Valid : 1;
    ULONGLONG Prototype : 1;
    ULONGLONG ReadOnly : 1;         // LWFIX: remove
    ULONGLONG Protection : 5;
    ULONGLONG filler02 : 8;
    LONGLONG ProtoAddress : 48;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_LIST {
    ULONGLONG Valid : 1;
    ULONGLONG filler07 : 7;
    ULONGLONG OneEntry : 1;
    ULONGLONG filler03 : 23;
    ULONGLONG NextEntry : 32;
} MMPTE_LIST;

typedef struct _MMPTE_SUBSECTION {
    ULONGLONG Valid : 1;
    ULONGLONG Prototype : 1;
    ULONGLONG WhichPool : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Filler04 : 8;
    LONGLONG SubsectionAddress : 48;
} MMPTE_SUBSECTION;

//
// A Valid Page Table Entry on a DEC AXP64 system has the following format.
//
// typedef struct _HARDWARE_PTE {
//     ULONGLONG Valid : 1;
//     ULONGLONG Reserved1 : 1;
//     ULONGLONG FaultOnWrite : 1;
//     ULONGLONG Reserved2 : 1;
//     ULONGLONG Global : 1;
//     ULONGLONG GranularityHint : 2;
//     ULONGLONG Reserved3 : 1;
//     ULONGLONG KernelReadAccess : 1;
//     ULONGLONG UserReadAccess : 1;
//     ULONGLONG Reserved4 : 2;
//     ULONGLONG KernelWriteAccess : 1;
//     ULONGLONG UserWriteAccess : 1;
//     ULONGLONG Reserved5 : 2;
//     ULONGLONG Write : 1;
//     ULONGLONG CopyOnWrite: 1;
//     ULONGLONG SoftwareWsIndex : 14;
//     ULONGLONG PageFrameNumber : 32;
// } HARDWARE_PTE, *PHARDWARE_PTE;
//

#define MI_GET_PAGE_FRAME_FROM_PTE(PTE) ((PFN_NUMBER)(PTE)->u.Hard.PageFrameNumber)
#define MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(PTE) ((PFN_NUMBER)(PTE)->u.Trans.PageFrameNumber)
#define MI_GET_PROTECTION_FROM_SOFT_PTE(PTE) ((ULONG)(PTE)->u.Soft.Protection)
#define MI_GET_PROTECTION_FROM_TRANSITION_PTE(PTE) ((ULONG)(PTE)->u.Trans.Protection)

//
// A Page Table Entry on a DEC ALPHA has the following definition.
//

typedef struct _MMPTE {
    union  {
        ULONG_PTR Long;
        HARDWARE_PTE Hard;
        HARDWARE_PTE Flush;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TRANSITION Trans;
        MMPTE_LIST List;
        MMPTE_SUBSECTION Subsect;
        } u;
} MMPTE, *PMMPTE;

//++
//VOID
//MI_WRITE_VALID_PTE (
//    IN PMMPTE PointerPte,
//    IN MMPTE PteContents
//    );
//
// Routine Description:
//
//    MI_WRITE_VALID_PTE fills in the specified PTE making it valid with the
//    specified contents.
//
// Arguments
//
//    PointerPte - Supplies a PTE to fill.
//
//    PteContents - Supplies the contents to put in the PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_WRITE_VALID_PTE(_PointerPte, _PteContents)    \
            (*(_PointerPte) = (_PteContents))

//++
//VOID
//MI_WRITE_INVALID_PTE (
//    IN PMMPTE PointerPte,
//    IN MMPTE PteContents
//    );
//
// Routine Description:
//
//    MI_WRITE_INVALID_PTE fills in the specified PTE making it invalid with the
//    specified contents.
//
// Arguments
//
//    PointerPte - Supplies a PTE to fill.
//
//    PteContents - Supplies the contents to put in the PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_WRITE_INVALID_PTE(_PointerPte, _PteContents)  \
            (*(_PointerPte) = (_PteContents))

//++
//VOID
//MI_WRITE_VALID_PTE_NEW_PROTECTION (
//    IN PMMPTE PointerPte,
//    IN MMPTE PteContents
//    );
//
// Routine Description:
//
//    MI_WRITE_VALID_PTE_NEW_PROTECTION fills in the specified PTE (which was
//    already valid) changing only the protection or the dirty bit.
//
// Arguments
//
//    PointerPte - Supplies a PTE to fill.
//
//    PteContents - Supplies the contents to put in the PTE.
//
// Return Value:
//
//    None.
//
//--

#define MI_WRITE_VALID_PTE_NEW_PROTECTION(_PointerPte, _PteContents)    \
            (*(_PointerPte) = (_PteContents))

//++
//VOID
//MiFillMemoryPte (
//    IN PMMPTE Destination,
//    IN ULONG  Length,
//    IN MMPTE  Pattern,
//    };
//
// Routine Description:
//
//    This function fills memory with the specified PTE pattern.
//
// Arguments
//
//    Destination - Supplies a pointer to the memory to fill.
//
//    Length      - Supplies the length, in bytes, of the memory to be
//                  filled.
//
//    Pattern     - Supplies the PTE fill pattern.
//
// Return Value:
//
//    None.
//
//--

#define MiFillMemoryPte(Destination, Length, Pattern) \
    RtlFillMemoryUlonglong((Destination), (Length), (Pattern))

//++
//BOOLEAN
//MI_IS_PAGE_TABLE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a page table address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a page table address, FALSE if not.
//
//--

#define MI_IS_PAGE_TABLE_ADDRESS(VA)   \
            ((PVOID)(VA) >= (PVOID)PTE_BASE && (PVOID)(VA) <= (PVOID)PDE_TOP)

//++
//BOOLEAN
//MI_IS_KERNEL_PAGE_TABLE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a page table address for a kernel address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a kernel page table address, FALSE if not.
//
//--

#define MI_IS_KERNEL_PAGE_TABLE_ADDRESS(VA)   \
            ((PVOID)(VA) >= (PVOID)MiGetPteAddress(MmSystemRangeStart) && (PVOID)(VA) <= (PVOID)PDE_TOP)


//++
//BOOLEAN
//MI_IS_PAGE_DIRECTORY_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a page directory address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a page directory address, FALSE if not.
//
//--

#define MI_IS_PAGE_DIRECTORY_ADDRESS(VA)   \
            ((PVOID)(VA) >= (PVOID)PDE_UBASE && (PVOID)(VA) <= (PVOID)PDE_TOP)


//++
//BOOLEAN
//MI_IS_HYPER_SPACE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a hyper space address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a hyper space address, FALSE if not.
//
//--

#define MI_IS_HYPER_SPACE_ADDRESS(VA)   \
            ((PVOID)(VA) >= (PVOID)HYPER_SPACE && (PVOID)(VA) <= (PVOID)HYPER_SPACE_END)


//++
//BOOLEAN
//MI_IS_PROCESS_SPACE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a process-specific address.  This is an address in user space
//    or page table pages or hyper space.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is a process-specific address, FALSE if not.
//
//--

#define MI_IS_PROCESS_SPACE_ADDRESS(VA)   \
            (((PVOID)(VA) <= (PVOID)MM_HIGHEST_USER_ADDRESS) || \
             ((PVOID)(VA) >= (PVOID)PTE_BASE && (PVOID)(VA) <= (PVOID)HYPER_SPACE_END))

//++
//BOOLEAN
//MI_IS_PTE_PROTOTYPE (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a PTE address and determines if it is a prototype PTE.
//
// Arguments
//
//    PTE - Supplies the virtual address of the PTE to check.
//
// Return Value:
//
//    TRUE if the PTE is in a segment (ie, a prototype PTE), FALSE if not.
//
//--

#define MI_IS_PTE_PROTOTYPE(PTE)   \
            ((PTE) > (PMMPTE)PDE_TOP)

//++
//BOOLEAN
//MI_IS_SYSTEM_CACHE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro takes a virtual address and determines if
//    it is a system cache address.
//
// Arguments
//
//    VA - Supplies a virtual address.
//
// Return Value:
//
//    TRUE if the address is in the system cache, FALSE if not.
//
//--

#define MI_IS_SYSTEM_CACHE_ADDRESS(VA)                      \
         (((PVOID)(VA) >= (PVOID)MmSystemCacheStart &&      \
		     (PVOID)(VA) <= (PVOID)MmSystemCacheEnd))

//++
//VOID
//MI_BARRIER_SYNCHRONIZE (
//    IN ULONG TimeStamp
//    );
//
// Routine Description:
//
//    MI_BARRIER_SYNCHRONIZE compares the argument timestamp against the
//    current IPI barrier sequence stamp.  When equal, all processors will
//    issue memory barriers to ensure that newly created pages remain coherent.
//
//    When a page is put in the zeroed or free page list the current
//    barrier sequence stamp is read (interlocked - this is necessary
//    to get the correct value - memory barriers won't do the trick)
//    and stored in the pfn entry for the page. The current barrier
//    sequence stamp is maintained by the IPI send logic and is
//    incremented (interlocked) when the target set of an IPI send
//    includes all processors, but the one doing the send. When a page
//    is needed its sequence number is compared against the current
//    barrier sequence number.  If it is equal, then the contents of
//    the page may not be coherent on all processors, and an IPI must
//    be sent to all processors to ensure a memory barrier is
//    executed (generic call can be used for this). Sending the IPI
//    automatically updates the barrier sequence number. The compare
//    is for equality as this is the only value that requires the IPI
//    (i.e., the sequence number wraps, values in both directions are
//    older). When a page is removed in this fashion and either found
//    to be coherent or made coherent, it cannot be modified between
//    that time and writing the PTE. If the page is modified between
//    these times, then an IPI must be sent.
//
// Arguments
//
//    TimeStamp - Supplies the timestamp at the time when the page was zeroed.
//
// Return Value:
//
//    None.
//
//--

#if defined(NT_UP)
#define MI_BARRIER_SYNCHRONIZE(TimeStamp)               \
        __MB();
#else
#define MI_BARRIER_SYNCHRONIZE(TimeStamp)               \
        if ((ULONG)TimeStamp == KeReadMbTimeStamp()) {  \
            KeSynchronizeMemoryAccess();                \
        }
#endif

//++
//VOID
//MI_BARRIER_STAMP_ZEROED_PAGE (
//    IN PULONG PointerTimeStamp
//    );
//
// Routine Description:
//
//    MI_BARRIER_STAMP_ZEROED_PAGE issues an interlocked read to get the
//    current IPI barrier sequence stamp.  This is called AFTER a page is
//    zeroed.
//
// Arguments
//
//    PointerTimeStamp - Supplies a timestamp pointer to fill with the
//                       current IPI barrier sequence stamp.
//
// Return Value:
//
//    None.
//
//--

#if defined(NT_UP)
#define MI_BARRIER_STAMP_ZEROED_PAGE(PointerTimeStamp) NOTHING
#else
#define MI_BARRIER_STAMP_ZEROED_PAGE(PointerTimeStamp) (*(PULONG)PointerTimeStamp = KeReadMbTimeStamp())
#endif

//++
//VOID
//MI_FLUSH_SINGLE_SESSION_TB (
//    IN PVOID Virtual,
//    IN ULONG Invalid,
//    IN LOGICAL AllProcessors,
//    IN PMMPTE PtePointer,
//    IN MMPTE PteValue,
//    IN MMPTE PreviousPte
//    );
//
// Routine Description:
//
//    MI_FLUSH_SINGLE_SESSION_TB flushes the requested single address
//    translation from the TB.  
//
//    Since Alpha supports ASNs and session space doesn't have one, the entire
//    TB needs to be flushed.
//
// Arguments
//
//    Virtual - Supplies the virtual address to invalidate.
//
//    Invalid - TRUE if invalidating.
//
//    AllProcessors - TRUE if all processors need to be IPI'd.
//
//    PtePointer - Supplies the PTE to invalidate.
//
//    PteValue - Supplies the new PTE value.
//
//    PreviousPte - The previous PTE value is returned here.
//
// Return Value:
//
//    None.
//
//--

#define MI_FLUSH_SINGLE_SESSION_TB(Virtual, Invalid, AllProcessors, PtePointer, PteValue, PreviousPte) \
    PreviousPte.u.Flush = *PtePointer;                  \
    *PtePointer = PteValue;                             \
    KeFlushEntireTb (TRUE, TRUE);


//++
//VOID
//MI_FLUSH_ENTIRE_SESSION_TB (
//    IN ULONG Invalid,
//    IN LOGICAL AllProcessors
//    );
//
// Routine Description:
//
//    MI_FLUSH_ENTIRE_SESSION_TB flushes the entire TB on Alphas since
//    the Alpha supports ASNs.
//
// Arguments
//
//    Invalid - TRUE if invalidating.
//
//    AllProcessors - TRUE if all processors need to be IPI'd.
//
// Return Value:
//
//    None.
//

#define MI_FLUSH_ENTIRE_SESSION_TB(Invalid, AllProcessors) \
    KeFlushEntireTb (Invalid, AllProcessors);
