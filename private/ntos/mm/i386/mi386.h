/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mi386.h

Abstract:

    This module contains the private data structures and procedure
    prototypes for the hardware dependent portion of the
    memory management system.

    This module is specifically tailored for the Intel 386,

Author:

    Lou Perazzoli (loup) 6-Jan-1990

Revision History:

--*/


/*++

    Virtual Memory Layout on the i386 is:

                 +------------------------------------+
        00000000 |                                    |
                 |                                    |
                 |                                    |
                 | User Mode Addresses                |
                 |                                    |
                 |   All pages within this range      |
                 |   are potentially accessible while |
                 |   the CPU is in USER mode.         |
                 |                                    |
                 |                                    |
                 +------------------------------------+
        7ffff000 | 64k No Access Area                 |
                 +------------------------------------+
        80000000 |                                    |
                 | HAL loads kernel and initial       |
                 | boot drivers in first 16mb         |
                 | of this region.                    |
                 | Kernel mode access only.           |
                 |                                    |
                 | If large pages are enabled, the    |
                 | initial non paged pool is relocated|
                 | to here during system startup.     |
                 |                                    |
                 +------------------------------------+
        81000000 |                                    |
                 | Unused  NO ACCESS                  |
                 |                                    |
                 +------------------------------------+
        A0000000 |                                    |
                 |         System mapped views        |
                 |                OR                  |
                 |           Session space            |
                 |                                    |
                 +------------------------------------+
        A4000000 |                                    |
                 | Additional System PTEs             |
                 |                                    |
                 +------------------------------------+
        C0000000 | Page Table Pages mapped through    |
                 |   this 4mb region                  |
                 |   Kernel mode access only.         |
                 |                                    |
                 +------------------------------------+
        C0400000 | HyperSpace - working set lists     |
                 |  and per process memory management |
                 |  structures mapped in this 4mb     |
                 |  region.                           |
                 |  Kernel mode access only.          |
                 +------------------------------------+
        C0800000 | NO ACCESS AREA (4MB)               |
                 |                                    |
                 +------------------------------------+
        C0C00000 | System Cache Structures            |
                 |   reside in this 4mb region        |
                 |   Kernel mode access only.         |
                 +------------------------------------+
        C1000000 | System cache resides here.         |
                 |   Kernel mode access only.         |
                 |                                    |
                 |                                    |
                 +------------------------------------+
        E1000000 | Start of paged system area         |
                 |   Kernel mode access only.         |
                 |                                    |
                 |                                    |
                 +------------------------------------+
                 |                                    |
                 | System Pte area - for mapping      |
                 |   kernel thread stacks and MDLs    |
                 |   that require system VAs.         |
                 |   Kernel mode access only.         |
                 |                                    |
                 +------------------------------------+
                 |                                    |
                 | NonPaged System area               |
                 |   Kernel mode access only.         |
                 |                                    |
                 +------------------------------------+
        FFBE0000 | Crash Dump Driver area             |
                 |   Kernel mode access only.         |
                 +------------------------------------+
        FFC00000 | Last 4mb reserved for HAL usage    |
                 +------------------------------------+


  Virtual Memory Layout of the (48MB) Session Space (on 2GB user systems) is:

                 +------------------------------------+
        A0000000 |                                    |
                 | win32k.sys and rebased NT4 printer |
                 |            drivers.                |
                 |                                    |
                 |             (8MB)                  |
                 |                                    |
                 +------------------------------------+
        A0800000 |                                    |
                 |   MM_SESSION_SPACE & Session WSLs  |
                 |              (4MB)                 |
                 |                                    |
                 +------------------------------------+
        A0C00000 |                                    |
                 |   Mapped Views for this session    |
                 |              (20MB)                |
                 |                                    |
                 +------------------------------------+
        A2000000 |                                    |
                 |   Paged Pool for this session      |
                 |              (16MB)                |
                 |                                    |
        A3000000 +------------------------------------+
                 |   System Mapped Views (not session)|
                 |              (16MB)                |
                 |                                    |
        A4000000 +------------------------------------+

--*/

#if !defined(_X86PAE_)
#if defined(NT_UP)
#define _MI_USE_CLAIMS_ 1
#endif

//
// Define empty list markers.
//

#define MM_EMPTY_LIST ((ULONG)0xFFFFFFFF) //
#define MM_EMPTY_PTE_LIST ((ULONG)0xFFFFF) // N.B. tied to MMPTE definition

#define MI_PTE_BASE_FOR_LOWEST_KERNEL_ADDRESS (MiGetPteAddress (0x80000000))

#define MM_SESSION_SPACE_DEFAULT        (0xA0000000)

#define MM_BOOT_IMAGE_SIZE              (16 * 1024 * 1024)

//
// Additional system PTEs are allocated out of this virtual address range.
//

#define KSTACK_POOL_START                (0xA4000000)

#define KSTACK_POOL_END                  (0xBFFFFFFF)

#define KSTACK_POOL_SIZE                 ((KSTACK_POOL_END-KSTACK_POOL_START) \
                                          & ~(PAGE_SIZE-1))

extern ULONG MiNumberOfExtraSystemPdes;

extern ULONG MiMaximumSystemCacheSizeExtra;

extern PVOID MiSystemCacheStartExtra;

extern PVOID MiSystemCacheEndExtra;

//
// PAGE_SIZE for Intel i386 is 4k, virtual page is 20 bits with a PAGE_SHIFT
// byte offset.
//

#define MM_VIRTUAL_PAGE_FILLER 0
#define MM_VIRTUAL_PAGE_SIZE 20

//
// Address space layout definitions.
//

#define MM_KSEG0_BASE ((ULONG)0x80000000)

#define MM_KSEG2_BASE ((ULONG)0xA0000000)

#define MM_PAGES_IN_KSEG0 ((MM_KSEG2_BASE - MM_KSEG0_BASE) >> PAGE_SHIFT)

extern ULONG MmKseg2Frame;

#define CODE_START MM_KSEG0_BASE

#define CODE_END   MM_KSEG2_BASE

#define MM_SYSTEM_SPACE_START (0xC0800000)

#define MM_SYSTEM_SPACE_END (0xFFFFFFFF)

#define PTE_TOP 0xC03FFFFF
#define PDE_TOP 0xC03FFFFF

#define HYPER_SPACE ((PVOID)0xC0400000)

#define HYPER_SPACE_END (0xC07fffff)

#define MM_SYSTEM_VIEW_START (0xA0000000)

#define MM_SYSTEM_VIEW_SIZE (48*1024*1024)

#define MM_SYSTEM_VIEW_START_IF_HYDRA (0xA3000000)

#define MM_SYSTEM_VIEW_SIZE_IF_HYDRA (16*1024*1024)

#define MM_LOWEST_4MB_START ((32*1024*1024)/PAGE_SIZE) //32mb

#define MM_DEFAULT_4MB_START (((1024*1024)/PAGE_SIZE)*4096) //4gb

#define MM_HIGHEST_4MB_START (((1024*1024)/PAGE_SIZE)*4096) //4gb

#define MM_USER_ADDRESS_RANGE_LIMIT 0xFFFFFFFF // user address range limit
#define MM_MAXIMUM_ZERO_BITS 21         // maximum number of zero bits

//
// Define the start and maximum size for the system cache.
// Maximum size is normally 512MB, but can be up to 512MB + 448MB = 960MB for
// large system cache machines.
//

#define MM_SYSTEM_CACHE_WORKING_SET (0xC0C00000)

#define MM_SYSTEM_CACHE_START (0xC1000000)

#define MM_SYSTEM_CACHE_END (0xE1000000)

#define MM_SYSTEM_CACHE_START_EXTRA (0xA4000000)

#define MM_SYSTEM_CACHE_END_EXTRA (0xC0000000)

#define MM_PAGED_POOL_START ((PVOID)(0xE1000000))

#define MM_LOWEST_NONPAGED_SYSTEM_START ((PVOID)(0xEB000000))

#define MmProtopte_Base ((ULONG)0xE1000000)

#define MM_NONPAGED_POOL_END ((PVOID)(0xFFBE0000))

#define MM_CRASH_DUMP_VA ((PVOID)(0xFFBE0000))

#define MM_DEBUG_VA  ((PVOID)0xFFBFF000)

#define NON_PAGED_SYSTEM_END   ((ULONG)0xFFFFFFF0)  //quadword aligned.

extern BOOLEAN MiWriteCombiningPtes;

//
// Define absolute minimum and maximum count for system PTEs.
//

#define MM_MINIMUM_SYSTEM_PTES 7000

#define MM_MAXIMUM_SYSTEM_PTES 50000

#define MM_DEFAULT_SYSTEM_PTES 11000

//
// Pool limits
//

//
// The maximum amount of nonpaged pool that can be initially created.
//

#define MM_MAX_INITIAL_NONPAGED_POOL ((ULONG)(128*1024*1024))

//
// The total amount of nonpaged pool (initial pool + expansion).
//

#define MM_MAX_ADDITIONAL_NONPAGED_POOL ((ULONG)(128*1024*1024))

//
// The maximum amount of paged pool that can be created.
//

#define MM_MAX_PAGED_POOL ((ULONG)MM_NONPAGED_POOL_END - (ULONG)MM_PAGED_POOL_START)

#define MM_MAX_TOTAL_POOL (((ULONG)MM_NONPAGED_POOL_END) - ((ULONG)(MM_PAGED_POOL_START)))


//
// Structure layout definitions.
//

#define MM_PROTO_PTE_ALIGNMENT ((ULONG)PAGE_SIZE)

#define PAGE_DIRECTORY_MASK    ((ULONG)0x003FFFFF)

#define MM_VA_MAPPED_BY_PDE (0x400000)

#define LOWEST_IO_ADDRESS 0xa0000

#define PTE_SHIFT 2

//
// The number of bits in a physical address.
//

#define PHYSICAL_ADDRESS_BITS 32

#define MM_MAXIMUM_NUMBER_OF_COLORS (1)

//
// i386 does not require support for colored pages.
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
//  Define 256k worth of secondary colors.
//

#define MM_SECONDARY_COLORS_DEFAULT (64)

#define MM_SECONDARY_COLORS_MIN (2)

#define MM_SECONDARY_COLORS_MAX (1024)

//
// Mask for isolating secondary color from physical page number;
//

extern ULONG MmSecondaryColorMask;

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

#define MM_MAXIMUM_WORKING_SET MiMaximumWorkingSet

extern ULONG MiMaximumWorkingSet;


#define MM_WORKING_SET_END ((ULONG)0xC07FF000)


//
// Define masks for fields within the PTE.
///

#define MM_PTE_VALID_MASK         0x1
#if defined(NT_UP)
#define MM_PTE_WRITE_MASK         0x2
#else
#define MM_PTE_WRITE_MASK         0x800
#endif
#define MM_PTE_OWNER_MASK         0x4
#define MM_PTE_WRITE_THROUGH_MASK 0x8
#define MM_PTE_CACHE_DISABLE_MASK 0x10
#define MM_PTE_ACCESS_MASK        0x20
#if defined(NT_UP)
#define MM_PTE_DIRTY_MASK         0x40
#else
#define MM_PTE_DIRTY_MASK         0x42
#endif
#define MM_PTE_LARGE_PAGE_MASK    0x80
#define MM_PTE_GLOBAL_MASK        0x100
#define MM_PTE_COPY_ON_WRITE_MASK 0x200
#define MM_PTE_PROTOTYPE_MASK     0x400
#define MM_PTE_TRANSITION_MASK    0x800

//
// Bit fields to or into PTE to make a PTE valid based on the
// protection field of the invalid PTE.
//

#define MM_PTE_NOACCESS          0x0   // not expressable on i386
#define MM_PTE_READONLY          0x0
#define MM_PTE_READWRITE         MM_PTE_WRITE_MASK
#define MM_PTE_WRITECOPY         0x200 // read-only copy on write bit set.
#define MM_PTE_EXECUTE           0x0   // read-only on i386
#define MM_PTE_EXECUTE_READ      0x0
#define MM_PTE_EXECUTE_READWRITE MM_PTE_WRITE_MASK
#define MM_PTE_EXECUTE_WRITECOPY 0x200 // read-only copy on write bit set.
#define MM_PTE_NOCACHE           0x010
#define MM_PTE_GUARD             0x0  // not expressable on i386
#define MM_PTE_CACHE             0x0

#define MM_PROTECT_FIELD_SHIFT 5

//
// Bits available for the software working set index within the hardware PTE.
//

#define MI_MAXIMUM_PTE_WORKING_SET_INDEX 0

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

#define MM_USER_PAGE_TABLE_PAGES (768)


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
//    This macro makes a valid PTE from a page frame number, protection mask,
//    and owner.
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

#define MI_MAKE_VALID_PTE(OUTPTE,FRAME,PMASK,PPTE)                            \
       (OUTPTE).u.Long = ((FRAME << 12) |                                     \
                         (MmProtectToPteMask[PMASK]) |                        \
                          MiDetermineUserGlobalPteMask ((PMMPTE)PPTE));

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

#define MI_MAKE_VALID_PTE_TRANSITION(OUTPTE,PROTECT) \
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
                (OUTPTE).u.Trans.Protection = PROTECT;     \
                (OUTPTE).u.Trans.Owner = MI_DETERMINE_OWNER(PPTE);


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

#define MI_MAKE_TRANSITION_PTE_VALID(OUTPTE,PPTE)                             \
        ASSERT (((PPTE)->u.Hard.Valid == 0) &&                                \
                ((PPTE)->u.Trans.Prototype == 0) &&                           \
                ((PPTE)->u.Trans.Transition == 1));                           \
       (OUTPTE).u.Long = (((PPTE)->u.Long & 0xFFFFF000) |                     \
                         (MmProtectToPteMask[(PPTE)->u.Trans.Protection]) |   \
                          MiDetermineUserGlobalPteMask ((PMMPTE)PPTE));

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
//    Since the i386 PTE has no free bits nothing needs to be done on this
//    architecture.
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

#define MI_SET_PTE_IN_WORKING_SET(PTE, WSINDEX)

//++
//ULONG WsIndex
//MI_GET_WORKING_SET_FROM_PTE(
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro returns the working set index from the argument PTE.
//    Since the i386 PTE has no free bits nothing needs to be done on this
//    architecture.
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

#define MI_GET_WORKING_SET_FROM_PTE(PTE)  0

//++
//VOID
//MI_SET_PTE_WRITE_COMBINE (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and enables WriteCombining as the
//    caching state.  Note that the PTE bits may only be set this way
//    if the Page Attribute Table is present and the PAT has been
//    initialized to provide Write Combining.
//
//    If either of the above conditions is not satisfied, then
//    the macro enables WEAK UC (PCD = 1, PWT = 0) in the PTE.
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
//

#define MI_SET_PTE_WRITE_COMBINE(PTE) \
            {                                                               \
                if (MiWriteCombiningPtes == TRUE) {                         \
                    ((PTE).u.Hard.CacheDisable = 0);                        \
                    ((PTE).u.Hard.WriteThrough = 1);                        \
                } else {                                                    \
                    ((PTE).u.Hard.CacheDisable = 1);                        \
                    ((PTE).u.Hard.WriteThrough = 0);                        \
                }                                                           \
            }

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

#define MI_SET_PTE_DIRTY(PTE) (PTE).u.Long |= HARDWARE_PTE_DIRTY_MASK


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

#define MI_SET_PTE_CLEAN(PTE) (PTE).u.Long &= ~HARDWARE_PTE_DIRTY_MASK



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

#define MI_IS_PTE_DIRTY(PTE) ((PTE).u.Hard.Dirty != 0)



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
// Arguments
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

#define MI_SET_GLOBAL_BIT_IF_SYSTEM(OUTPTE,PPTE)                             \
   if ((((PMMPTE)PPTE) > MiHighestUserPte) &&                                \
       ((((PMMPTE)PPTE) <= MiGetPteAddress (PTE_BASE)) ||                    \
       (((PMMPTE)PPTE) >= MiGetPteAddress (MM_SYSTEM_CACHE_WORKING_SET)))) { \
           (OUTPTE).u.Long |= MmPteGlobal.u.Long;                            \
   }                                                                         \
   else {                                                                    \
           (OUTPTE).u.Long &= ~MmPteGlobal.u.Long;                           \
   }


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
// Arguments
//
//    PTE - Supplies the PTE to set global state into.
//
//    STATE - Supplies 1 if global, 0 if not.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_GLOBAL_STATE(PTE,STATE)                              \
           if (STATE) {                                             \
               (PTE).u.Long |= MmPteGlobal.u.Long;                  \
           }                                                        \
           else {                                                   \
               (PTE).u.Long &= ~MmPteGlobal.u.Long;                 \
           }





//++
//VOID
//MI_ENABLE_CACHING (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and sets the caching state to be
//    enabled.  This is performed by clearing the PCD and PWT bits in the PTE.
//
//    Semantics of the overlap between PCD, PWT, and the
//    USWC memory type in the MTRR are:
//
//    PCD   PWT   Mtrr Mem Type      Effective Memory Type
//     1     0    USWC               USWC
//     1     1    USWC               UC
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

#define MI_ENABLE_CACHING(PTE) \
            {                                                                \
                ((PTE).u.Hard.CacheDisable = 0);                             \
                ((PTE).u.Hard.WriteThrough = 0);                             \
            }



//++
//VOID
//MI_DISABLE_CACHING (
//    IN MMPTE PTE
//    );
//
// Routine Description:
//
//    This macro takes a valid PTE and sets the caching state to be
//    disabled.  This is performed by setting the PCD and PWT bits in the PTE.
//
//    Semantics of the overlap between PCD, PWT, and the
//    USWC memory type in the MTRR are:
//
//    PCD   PWT   Mtrr Mem Type      Effective Memory Type
//     1     0    USWC               USWC
//     1     1    USWC               UC
//
//    Since an effective memory type of UC is desired here,
//    the WT bit is set.
//
// Arguments
//
//    PTE - Supplies a pointer to the valid PTE.
//
// Return Value:
//
//     None.
//
//--


#define MI_DISABLE_CACHING(PTE) \
            {                                                                \
                ((PTE).u.Hard.CacheDisable = 1);                             \
                ((PTE).u.Hard.WriteThrough = 1);                             \
            }




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
// Arguments
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
    PERFINFO_DELETE_PAGE(PPFN); \
    PPFN->PteAddress = (PMMPTE)0xFFFFFFFF;




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

#define MI_IS_PFN_DELETED(PPFN)   \
            ((PPFN)->PteAddress == (PMMPTE)(ULONG_PTR)0xFFFFFFFF)


//++
//VOID
//MI_CHECK_PAGE_ALIGNMENT (
//    IN ULONG PAGE,
//    IN PMMPTE PPTE
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
//    PPTE - Supplies a pointer to the new PTE which will contain the page.
//
// Return Value:
//
//    none.
//
//--

// does nothing on i386.

#define MI_CHECK_PAGE_ALIGNMENT(PAGE,PPTE)




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

// does nothing on i386.

#define MI_INITIALIZE_HYPERSPACE_MAP(INDEX)


//++
//ULONG
//MI_GET_PAGE_COLOR_FROM_PTE (
//    IN PMMPTE PTEADDRESS
//    );
//
// Routine Description:
//
//    This macro determines the page's color based on the PTE address
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
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//    ADDRESS - Supplies the address the page is (or was) mapped at.
//
// Return Value:
//
//    The page's color.
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
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//
// Return Value:
//
//    The page's color.
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
//    This macro determines the page's color based on the PTE address
//    that maps the page.
//
// Arguments
//
//    ADDRESS - Supplies the address the page is (or was) mapped at.
//
// Return Value:
//
//    The page's color.
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
// Arguments
//
//    COLOR - Supplies the color to return the previous of.
//
// Return Value:
//
//    Previous color in sequence.
//
//--

#define MI_GET_PREVIOUS_COLOR(COLOR)  (0)


#define MI_GET_SECONDARY_COLOR(PAGE,PFN) (PAGE & MmSecondaryColorMask)


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
//    This macro returns the first page destined for a paging
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
// Arguments
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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     None.
//
//--

#if defined(NT_UP)
#define MI_MAKE_VALID_PTE_WRITE_COPY(PPTE) \
                    if ((PPTE)->u.Hard.Write == 1) {    \
                        (PPTE)->u.Hard.CopyOnWrite = 1; \
                        (PPTE)->u.Hard.Write = 0;       \
                    }
#else
#define MI_MAKE_VALID_PTE_WRITE_COPY(PPTE) \
                    if ((PPTE)->u.Hard.Write == 1) {    \
                        (PPTE)->u.Hard.CopyOnWrite = 1; \
                        (PPTE)->u.Hard.Write = 0;       \
                        (PPTE)->u.Hard.Writable = 0;    \
                    }
#endif



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

#define MI_DETERMINE_OWNER(PPTE)   \
    ((((PPTE) <= MiHighestUserPte) ||                                       \
      ((PPTE) >= MiGetPdeAddress(NULL) &&                                   \
      ((PPTE) <= MiHighestUserPde))) ? 1 : 0)



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
//     None
//
//--

#if defined(NT_UP)
#define MI_SET_ACCESSED_IN_PTE(PPTE,ACCESSED) \
                    ((PPTE)->u.Hard.Accessed = ACCESSED)
#else

//
// Don't do anything on MP systems.
//

#define MI_SET_ACCESSED_IN_PTE(PPTE,ACCESSED)
#endif


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

#if defined(NT_UP)
#define MI_GET_ACCESSED_IN_PTE(PPTE) ((PPTE)->u.Hard.Accessed)
#else
#define MI_GET_ACCESSED_IN_PTE(PPTE) 0
#endif


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

#define MI_SET_OWNER_IN_PTE(PPTE,OWNER) ((PPTE)->u.Hard.Owner = OWNER)




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

#define MI_GET_OWNER_IN_PTE(PPTE) ((PPTE)->u.Hard.Owner)



//
// bit mask to clear out fields in a PTE to or in prototype pte offset.
//

#define CLEAR_FOR_PROTO_PTE_ADDRESS ((ULONG)0x701)

//
// bit mask to clear out fields in a PTE to or in paging file location.
//

#define CLEAR_FOR_PAGE_FILE 0x000003E0


//++
//VOID
//MI_SET_PAGING_FILE_INFO (
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
//    None.
//
//--

#define MI_SET_PAGING_FILE_INFO(OUTPTE,PPTE,FILEINFO,OFFSET)            \
       (OUTPTE).u.Long = (PPTE).u.Long;                                 \
       (OUTPTE).u.Long &= CLEAR_FOR_PAGE_FILE;                          \
       (OUTPTE).u.Long |= ((FILEINFO << 1) | (OFFSET << 12));


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
//    NOTE THAT A PROTOPTE CAN ONLY RESIDE IN PAGED POOL!!!!!!
//
//    MAX SIZE = 2^(2+7+21) = 2^30 = 1GB.
//
//    NOTE that the valid bit must be zero!
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


#define MiPteToProto(lpte) ((PMMPTE)(((((lpte)->u.Long) >> 11) << 9) +  \
                (((((lpte)->u.Long)) << 24) >> 23) \
                + MmProtopte_Base))


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
// Arguments
//
//    proto_va - Supplies the address of the prototype PTE.
//
// Return Value:
//
//    Mask to set into the PTE.
//
//--

#define MiProtoAddressForPte(proto_va)  \
   ((((((ULONG)proto_va - MmProtopte_Base) >> 1) & (ULONG)0x000000FE)   | \
    (((((ULONG)proto_va - MmProtopte_Base) << 2) & (ULONG)0xfffff800))) | \
    MM_PTE_PROTOTYPE_MASK)




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
// Arguments
//
//    proto_va - Supplies the address of the prototype PTE.
//
// Return Value:
//
//    Mask to set into the PTE.
//
//--

//  not different on x86.

#define MiProtoAddressForKernelPte(proto_va)  MiProtoAddressForPte(proto_va)


#define MM_SUBSECTION_MAP (128*1024*1024)

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
//    MAXIMUM NONPAGED POOL = 2^(3+4+21) = 2^28 = 256mb.
//
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

//#define MiGetSubsectionAddress(lpte)                        \
//            ((PSUBSECTION)((ULONG)MM_NONPAGED_POOL_END -    \
//                (((((lpte)->u.Long)>>11)<<7) |              \
//                (((lpte)->u.Long<<2) & 0x78))))

#define MiGetSubsectionAddress(lpte)                              \
    (((lpte)->u.Long & 0x80000000) ?                              \
            ((PSUBSECTION)((ULONG)MmSubsectionBase +    \
                ((((lpte)->u.Long & 0x7ffff800) >> 4) |              \
                (((lpte)->u.Long<<2) & 0x78)))) \
      : \
            ((PSUBSECTION)((ULONG)MmNonPagedPoolEnd -    \
                (((((lpte)->u.Long)>>11)<<7) |              \
                (((lpte)->u.Long<<2) & 0x78)))))



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
// Arguments
//
//    VA - Supplies a pointer to the subsection to encode.
//
// Return Value:
//
//     The mask to set into the PTE to make it reference the supplied
//     subsection.
//
//--

//#define MiGetSubsectionAddressForPte(VA)  \
//   (((((ULONG)MM_NONPAGED_POOL_END - (ULONG)VA)>>2) & (ULONG)0x0000001E) |  \
//   ((((((ULONG)MM_NONPAGED_POOL_END - (ULONG)VA)<<4) & (ULONG)0xfffff800))))

#define MiGetSubsectionAddressForPte(VA)                   \
            (((ULONG)(VA) < (ULONG)MM_KSEG2_BASE) ?                  \
   (((((ULONG)VA - (ULONG)MmSubsectionBase)>>2) & (ULONG)0x0000001E) |  \
   ((((((ULONG)VA - (ULONG)MmSubsectionBase)<<4) & (ULONG)0x7ffff800)))| \
   0x80000000) \
            : \
   (((((ULONG)MmNonPagedPoolEnd - (ULONG)VA)>>2) & (ULONG)0x0000001E) |  \
   ((((((ULONG)MmNonPagedPoolEnd - (ULONG)VA)<<4) & (ULONG)0x7ffff800)))))




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

#define MiGetPpeAddress(va)  ((PMMPTE)0)


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
// Arguments
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
// Arguments
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
//MiGetPpeOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPpeOffset returns the offset into a page root
//    for a given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the offset for.
//
// Return Value:
//
//    The offset into the page root table the corresponding PPE is at.
//
//--

#define MiGetPpeOffset(va) (0)

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
// Arguments
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
//MiGetPpePdeOffset (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPdeOffset returns the offset into a page directory
//    for a given virtual address.
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

#define MiGetPpePdeOffset MiGetPdeOffset



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
// Arguments
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

#define MiGetVirtualAddressMappedByPpe(PPE) (NULL)

//++
//PVOID
//MiGetVirtualAddressMappedByPde (
//    IN PMMPTE PTE
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

#define MiGetVirtualAddressMappedByPde(PDE) ((PVOID)((ULONG)(PDE) << 20))


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

#define MiGetVirtualAddressMappedByPte(PTE) ((PVOID)((ULONG)(PTE) << 10))


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

#define MiIsVirtualAddressOnPpeBoundary(VA) (FALSE)


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
//    TRUE if on a 4MB PDE boundary, FALSE if not.
//
//--

#define MiIsVirtualAddressOnPdeBoundary(VA) (((ULONG_PTR)(VA) & PAGE_DIRECTORY_MASK) == 0)


//++
//LOGICAL
//MiIsPteOnPpeBoundary (
//    IN PVOID PTE
//    );
//
// Routine Description:
//
//    MiIsPteOnPpeBoundary returns TRUE if the PTE is
//    on a page directory parent entry boundary.
//
// Arguments
//
//    PTE - Supplies the PTE to check.
//
// Return Value:
//
//    TRUE if on a boundary, FALSE if not.
//
//--

#define MiIsPteOnPpeBoundary(PTE) (FALSE)


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
//    TRUE if on a 4MB PDE boundary, FALSE if not.
//
//--

#define MiIsPteOnPdeBoundary(PTE) (((ULONG_PTR)(PTE) & (PAGE_SIZE - 1)) == 0)


//++
//LOGICAL
//MiDoesPpeExistAndMakeValid (
//    IN PMMPTE PointerPpe,
//    IN PEPROCESS TargetProcess,
//    IN ULONG PfnMutexHeld
//    OUT PULONG Waited
//    );
//
// Routine Description:
//
//    MiDoesPpeExistAndMakeValid returns TRUE if the specified PPE entry
//    exists and can be made valid.
//
// Arguments
//
//    PointerPpe - Supplies the PPE entry to check.
//
//    TargetProcess - Supplies a pointer to the current process.
//
//    PfnMutexHeld - Supplies the value TRUE if the PFN mutex is held, FALSE
//                   otherwise.
//
//    Waited - Supplies a pointer to increment if the mutex was released and
//             reacquired.
//
// Return Value:
//
//    TRUE if valid, FALSE if not.  Always TRUE on x86.
//
//--

#define MiDoesPpeExistAndMakeValid(PPE, TARGETPROCESS, PFNMUTEXHELD, WAITED) (1)


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

#define GET_PAGING_FILE_NUMBER(PTE) ((((PTE).u.Long) >> 1) & 0x0000000F)



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
// Arguments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//     Returns 0 if the PTE is demand zero, non-zero otherwise.
//
//--

#define IS_PTE_NOT_DEMAND_ZERO(PTE) ((PTE).u.Long & (ULONG)0xFFFFFC01)




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
        if ((PTE).u.Soft.Protection & MM_PROTECTION_WRITE_MASK) {      \
            (PTE).u.Long |= MM_PROTECTION_COPY_MASK << MM_PROTECT_FIELD_SHIFT;      \
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

#if defined(NT_UP)
#define MI_SET_PAGE_DIRTY(PPTE,VA,PFNHELD)
#else
#define MI_SET_PAGE_DIRTY(PPTE,VA,PFNHELD)                          \
            if ((PPTE)->u.Hard.Dirty == 1) {                        \
                MiSetDirtyBit ((VA),(PPTE),(PFNHELD));              \
            }
#endif




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

#if defined(NT_UP)
#define MI_NO_FAULT_FOUND(TEMP,PPTE,VA,PFNHELD)
#else
#define MI_NO_FAULT_FOUND(TEMP,PPTE,VA,PFNHELD) \
        if (StoreInstruction && ((PPTE)->u.Hard.Dirty == 0)) {  \
            MiSetDirtyBit ((VA),(PPTE),(PFNHELD));     \
        }
#endif




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

#define MI_CAPTURE_DIRTY_BIT_TO_PFN(PPTE,PPFN)                      \
         ASSERT (KeGetCurrentIrql() > APC_LEVEL);                   \
         if (((PPFN)->u3.e1.Modified == 0) &&                       \
            ((PPTE)->u.Hard.Dirty != 0)) {               \
             (PPFN)->u3.e1.Modified = 1;                            \
             if (((PPFN)->OriginalPte.u.Soft.Prototype == 0) &&     \
                          ((PPFN)->u3.e1.WriteInProgress == 0)) {   \
                 MiReleasePageFileSpace ((PPFN)->OriginalPte);      \
                 (PPFN)->OriginalPte.u.Soft.PageFileHigh = 0;       \
             }                                                      \
         }


//++
//BOOLEAN
//MI_IS_PHYSICAL_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
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
     (((ULONG)Va >= MM_KSEG0_BASE) && ((ULONG)Va < MM_KSEG2_BASE) && (MmKseg2Frame))


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
// Arguments
//
//    VA - Supplies a pointer to the physical address.
//
// Return Value:
//
//    Returns the PFN for the page.
//
//--


#define MI_CONVERT_PHYSICAL_TO_PFN(Va)    (((ULONG)Va << 3) >> 15)


typedef struct _MMCOLOR_TABLES {
    ULONG Flink;
    PVOID Blink;
} MMCOLOR_TABLES, *PMMCOLOR_TABLES;

typedef struct _MMPRIMARY_COLOR_TABLES {
    LIST_ENTRY ListHead;
} MMPRIMARY_COLOR_TABLES, *PMMPRIMARY_COLOR_TABLES;

extern PMMCOLOR_TABLES MmFreePagesByColor[2];

extern ULONG MmTotalPagesForPagingFile;


//
// A VALID Page Table Entry on an Intel 386/486 has the following definition.
//

#define MI_PTE_LOOKUP_NEEDED (0xfffff)

typedef struct _MMPTE_SOFTWARE {
    ULONG Valid : 1;
    ULONG PageFileLow : 4;
    ULONG Protection : 5;
    ULONG Prototype : 1;
    ULONG Transition : 1;
    ULONG PageFileHigh : 20;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION {
    ULONG Valid : 1;
    ULONG Write : 1;
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Protection : 5;
    ULONG Prototype : 1;
    ULONG Transition : 1;
    ULONG PageFrameNumber : 20;
} MMPTE_TRANSITION;

typedef struct _MMPTE_PROTOTYPE {
    ULONG Valid : 1;
    ULONG ProtoAddressLow : 7;
    ULONG ReadOnly : 1;  // if set allow read only access.
    ULONG WhichPool : 1;
    ULONG Prototype : 1;
    ULONG ProtoAddressHigh : 21;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_SUBSECTION {
    ULONG Valid : 1;
    ULONG SubsectionAddressLow : 4;
    ULONG Protection : 5;
    ULONG Prototype : 1;
    ULONG SubsectionAddressHigh : 20;
    ULONG WhichPool : 1;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST {
    ULONG Valid : 1;
    ULONG OneEntry : 1;
    ULONG filler10 : 10;
    ULONG NextEntry : 20;
} MMPTE_LIST;

//
// A Page Table Entry on an Intel 386/486 has the following definition.
//

#if defined(NT_UP)

//
// Uniprocessor version.
//

typedef struct _MMPTE_HARDWARE {
    ULONG Valid : 1;
    ULONG Write : 1;  // UP version
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG CopyOnWrite : 1; // software field
    ULONG Prototype : 1;   // software field
    ULONG reserved : 1;  // software field
    ULONG PageFrameNumber : 20;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

#define HARDWARE_PTE_DIRTY_MASK     0x40

#else

//
// MP version to avoid stalls when flushing TBs across processors.
//

typedef struct _MMPTE_HARDWARE {
    ULONG Valid : 1;
    ULONG Writable : 1;      //changed for MP version
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG CopyOnWrite : 1; // software field
    ULONG Prototype : 1;   // software field
    ULONG Write : 1;       // software field - MP change
    ULONG PageFrameNumber : 20;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

#define HARDWARE_PTE_DIRTY_MASK     0x42

#endif //NT_UP

#define MI_GET_PAGE_FRAME_FROM_PTE(PTE) ((PTE)->u.Hard.PageFrameNumber)
#define MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(PTE) ((PTE)->u.Trans.PageFrameNumber)
#define MI_GET_PROTECTION_FROM_SOFT_PTE(PTE) ((PTE)->u.Soft.Protection)
#define MI_GET_PROTECTION_FROM_TRANSITION_PTE(PTE) ((PTE)->u.Trans.Protection)

typedef struct _MMPTE {
    union  {
        ULONG Long;
        MMPTE_HARDWARE Hard;
        HARDWARE_PTE Flush;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TRANSITION Trans;
        MMPTE_SUBSECTION Subsect;
        MMPTE_LIST List;
        } u;
} MMPTE;

typedef MMPTE *PMMPTE;

extern MMPTE MmPteGlobal; // Set if processor supports Global Page, else zero.

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
             RtlFillMemoryUlong ((Destination), (Length), (Pattern))

ULONG
FASTCALL
MiDetermineUserGlobalPteMask (
    IN PMMPTE Pte
    );

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
            ((PVOID)(VA) >= (PVOID)PDE_BASE && (PVOID)(VA) <= (PVOID)PDE_TOP)


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

#define MI_IS_SYSTEM_CACHE_ADDRESS(VA)                            \
         (((PVOID)(VA) >= (PVOID)MmSystemCacheStart &&            \
		     (PVOID)(VA) <= (PVOID)MmSystemCacheEnd)  ||          \
          ((PVOID)(VA) >= (PVOID)MiSystemCacheStartExtra &&       \
			  (PVOID)(VA) <= (PVOID)MiSystemCacheEndExtra))

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

// does nothing on i386.

#define MI_BARRIER_SYNCHRONIZE(TimeStamp)

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

// does nothing on i386.

#define MI_BARRIER_STAMP_ZEROED_PAGE(PointerTimeStamp)

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
//    Since there are no ASNs on the x86, this routine becomes a single
//    TB invalidate.
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
    PreviousPte.u.Flush = KeFlushSingleTb (Virtual,      \
                                           TRUE,         \
                                           TRUE,         \
                                           PtePointer,   \
                                           PteValue);

//++
//VOID
//MI_FLUSH_ENTIRE_SESSION_TB (
//    IN ULONG Invalid,
//    IN LOGICAL AllProcessors
//    );
//
// Routine Description:
//
//    MI_FLUSH_ENTIRE_SESSION_TB flushes the entire TB on processors which
//    support ASNs.
//
//    Since there are no ASNs on the x86, this routine does nothing.
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
    NOTHING;

#else
#include "i386\mipae.h"
#endif
