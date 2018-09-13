/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1995  Intel Corporation

Module Name:

    miia64.h

Abstract:

    This module contains the private data structures and procedure
    prototypes for the hardware dependent portion of the
    memory management system.

    This module is specifically tailored for the Intel MERCED,

Author:

    Lou Perazzoli (loup) 6-Jan-1990

Revision History:

--*/

/*++

    Virtual Memory Layout on MERCED is:

/*++

    Virtual Memory Layout for the initial Merced port is:

                 +------------------------------------+
0000000000000000 | User mode addresses - 8tb minus 8gb| UADDRESS_BASE
                 |                                    |
                 |                                    |
000003FFFFFEFFFF |                                    | MM_HIGHEST_USER_ADDRESS
                 +------------------------------------+
000003FFFFFF0000 | 64k No Access Region               | MM_USER_PROBE_ADDRESS
                 +------------------------------------+
0000040000000000 | HyperSpace - working set lists     | HYPER_SPACE
                 |  and per process memory management |
                 |  structures mapped in this 8gb     |
00000401FFFFFFFF |  region.                           | HYPER_SPACE_END
                 +------------------------------------+
0000040200000000 | Remaining per process addresses for|
                 | eventual 43-bit address space.     |
                 |                                    |
                 |                                    |
000007FFFFFFFFFF |                                    |
                 +------------------------------------+
                                   .
                                   .
                 +------------------------------------+
1FFFFF0000000000 | 8gb leaf level page table map      | PTE_UBASE
                 |        for user space              |
1FFFFF01FFFFFFFF |                                    | PTE_UTOP
                 +------------------------------------+
                 +------------------------------------+
1FFFFFFFC0000000 | 8mb page directory (2nd level)     | PDE_UBASE
                 | table map for user space           |
1FFFFFFFC07FFFFF |                                    | PDE_UTOP
                 +------------------------------------+
                 +------------------------------------+
1FFFFFFFFFF00000 | 8KB parent directory (1st level)   | PDE_UTBASE
                 +------------------------------------+
                                   .
                                   .
                 +------------------------------------+
2000000000000000 |  Hydra - 8gb                       | MM_SESSION_SPACE_DEFAULT
                 |  and per process memory management |
                 |  structures mapped in this 8gb     |
                 |  region.                           |
                 +------------------------------------+
                                   .
                 +------------------------------------+
3FFFFF0000000000 | 8gb leaf level page table map      | PTE_SBASE
                 |        for session space           |
3FFFFF01FFFFFFFF |                                    | PTE_STOP
                 +------------------------------------+
                 +------------------------------------+
3FFFFFFFC0000000 | 8mb page directory (2nd level)     | PDE_SBASE
                 | table map for session space        |
3FFFFFFFC07FFFFF |                                    | PDE_STOP
                 +------------------------------------+
                 +------------------------------------+
3FFFFFFFFFF00000 | 8KB parent directory (1st level)   | PDE_STBASE
                 +------------------------------------+
                                   .
                 +------------------------------------+ 
8000000000000000 |   physical addressable memory      | KSEG3_BASE
                 |   for 44-bit of address space      | 
80000FFFFFFFFFFF |   mapped by VHPT 64KB page         | KSEG3_LIMIT
                 +------------------------------------+
                                   .
                 +------------------------------------+
9FFFFF00000000000| vhpt 64kb page for KSEG3 space     |
                 |            (not used)              |
                 +------------------------------------+
                                   .
                                   .
                 +------------------------------------+ MM_SYSTEM_RANGE_START
E000000000000000 |                                    | KADDRESS_BASE
                 +------------------------------------+
E000000080000000 | The HAL, kernel, initial drivers,  | KSEG0_BASE
                 | NLS data, and registry load in the |
                 | first 16mb of this region which    |
                 | physically addresses memory.       |
                 |                                    |
                 | Kernel mode access only.           |
                 |                                    |
                 | Initial NonPaged Pool is within    |
                 | KSEG0                              |
                 |                                    | KSEG2_BASE
                 +------------------------------------+
E0000000A0000000 | System mapped views                | MM_SYSTEM_VIEW_START
                 |   Win32k.sys                       |     MM_SYSTEM_CACHE_END
                 |                                    |
                 +------------------------------------+
E0000000FF000000 | Shared system page                 | KI_USER_SHARED_DATA
                 +------------------------------------+
E0000000FF002000 |   Reserved for the HAL.            |
                 |                                    |
                 |                                    |
E0000000FFFFFFFF |                                    | 
                 +------------------------------------+
                                   .
                                   .
                 +------------------------------------+
E000000200000000 |                                    |
                 |                                    |
                 |                                    |
                 |                                    |
                 +------------------------------------+
E000000400000000 |   The system cache working set     | MM_SYSTEM_CACHE_WORKING_SET
                 |                                    | MM_SYSTEM_SPACE_START
                 |   information resides in this 8gb  |
                 |   region.                          |
                 +------------------------------------+
E000000600000000 | System cache resides here.         | MM_SYSTEM_CACHE_START
                 |  Kernel mode access only.          |
                 |  1tb.                              |
                 +------------------------------------+
E000010600000000 | Start of paged system area.        | MM_PAGED_POOL_START
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
E000012600000000 | System PTE pool.                   | MM_LOWEST_NONPAGED_SYSTEM_START
                 |  Kernel mode access only.          |
                 |  128gb.                            |
                 +------------------------------------+
E000014600000000 | NonPaged pool.                     | MM_NON_PAGED_POOL_START
                 |  Kernel mode access only.          |
                 |  128gb.                            |
                 |                                    |
E0000165FFFFFFFF |  NonPaged System area              | MM_NONPAGED_POOL_END
                 +------------------------------------+ 
                                   .
                                   .
E000040000000000 +------------------------------------+ MM_PFN_DATABASE_START
                 | PFN Database space                 |
                 |  Kernel mode access only.          |
                 |  2tb.                              |
E000060000000000 +------------------------------------+ MM_PFN_DATABASE_END
                                   .                    MM_SYSTEM_SPACE_END
                                   .
                                   .
                 +------------------------------------+
FFFFFF0000000000 | 8gb leaf level page table map      | PTE_KBASE
                 |       for kernel space             |
FFFFFF01FFFFFFFF |                                    | PTE_KTOP
                 +------------------------------------+
                 +------------------------------------+
FFFFFFFFC0000000 | 8mb page directory (2nd level)     | PDE_KBASE
                 | table map for kernel space         |
FFFFFFFFC07FFFFF |                                    | PDE_KTOP
                 +------------------------------------+
                 +------------------------------------+
FFFFFFFFFFF00000 | 8KB parent directory (1st level)   | PDE_KTBASE
                 +------------------------------------+

--*/

#define _MM64_ 1

#define _MIALT4K_ 1
// #define HYPERMAP 1

//
// Define empty list markers.
//

#define MM_EMPTY_LIST ((ULONG)0xFFFFFFFF) //
#define MM_EMPTY_PTE_LIST ((ULONG)0xFFFFFFFF) // N.B. tied to MMPTE definition

#define MI_PTE_BASE_FOR_LOWEST_KERNEL_ADDRESS ((PMMPTE)PTE_KBASE)

//
// 43-Bit virtual address mask.
//

#define MASK_43 0x7FFFFFFFFFFUI64       //

//
// 44-Bit Physical address mask.
//

#define MASK_44 0xFFFFFFFFFFFUI64       

#define MM_PAGES_IN_KSEG0 ((ULONG)((KSEG2_BASE - KSEG0_BASE) >> PAGE_SHIFT))

extern ULONG_PTR MmKseg2Frame;
extern ULONGLONG MmPageSizeInfo;

#define MM_USER_ADDRESS_RANGE_LIMIT (0xFFFFFFFFFFFFFFFFUI64) // user address range limit
#define MM_MAXIMUM_ZERO_BITS 53         // maximum number of zero bits

//
// PAGE_SIZE for Intel MERCED is 8k, virtual page is 20 bits with a PAGE_SHIFT
// byte offset.
//

#define MM_VIRTUAL_PAGE_FILLER (PAGE_SHIFT - 12)
#define MM_VIRTUAL_PAGE_SIZE (64-PAGE_SHIFT)

//
// Address space layout definitions.
//

#define CODE_START KSEG0_BASE

#define CODE_END   KSEG2_BASE

#define MM_SYSTEM_SPACE_START (KADDRESS_BASE + 0x400000000UI64)

#define MM_SYSTEM_SPACE_END (KADDRESS_BASE + 0x60000000000UI64)

#define PDE_TOP PDE_UTOP

//
// Define Althernate 4KB permission table space for X86
//

#define ALT4KB_PERMISSION_TABLE_START (UADDRESS_BASE + 0x40000000000)

#define ALT4KB_PERMISSION_TABLE_END   (UADDRESS_BASE + 0x40000400000)

//
// Define hyper space 
//

#define HYPER_SPACE ((PVOID)(UADDRESS_BASE + 0x40000800000))

#define HYPER_SPACE_END (UADDRESS_BASE + 0x401FFFFFFFF)

//
// Define area for mapping views into system space
//

#define MM_SYSTEM_VIEW_START (KADDRESS_BASE + 0xA0000000)

#define MM_SYSTEM_VIEW_SIZE (48*1024*1024)

#define MM_SESSION_SPACE_DEFAULT (0x2000000000000000UI64)  // make it the region 1 space

#define MM_SYSTEM_VIEW_START_IF_HYDRA MM_SYSTEM_VIEW_START

#define MM_SYSTEM_VIEW_SIZE_IF_HYDRA MM_SYSTEM_VIEW_SIZE

//
// Define the start and maximum size for the system cache.
// Maximum size 512MB.
//

#define MM_SYSTEM_CACHE_WORKING_SET (KADDRESS_BASE + 0x400000000UI64)

#define MM_SYSTEM_CACHE_START (KADDRESS_BASE + 0x600000000UI64)

#define MM_SYSTEM_CACHE_END (KADDRESS_BASE + 0x1005FFFFFFFFUI64)

#define MM_MAXIMUM_SYSTEM_CACHE_SIZE     \
   (((ULONG_PTR)MM_SYSTEM_CACHE_END - (ULONG_PTR)MM_SYSTEM_CACHE_START) >> PAGE_SHIFT)

#define MM_PAGED_POOL_START ((PVOID)(KADDRESS_BASE + 0x10600000000UI64))

#define MM_LOWEST_NONPAGED_SYSTEM_START ((PVOID)(KADDRESS_BASE + 0x12600000000UI64))

#define MmProtopte_Base (KADDRESS_BASE)

#define MM_NONPAGED_POOL_END ((PVOID)(KADDRESS_BASE + 0x16600000000UI64 - (16 * PAGE_SIZE)))

#define MM_CRASH_DUMP_VA ((PVOID)(KADDRESS_BASE + 0xFF800000))

// EPC VA at 0xFFA00000 (see ntia64.h)

#define MM_DEBUG_VA  ((PVOID)(KADDRESS_BASE + 0xFF900000))

#define NON_PAGED_SYSTEM_END   (KADDRESS_BASE + 0x16600000000UI64)  //quadword aligned.

#define MM_PFN_DATABASE_START (KADDRESS_BASE + 0x40000000000UI64)

#define MM_PFN_DATABASE_END (KADDRESS_BASE + 0x60000000000UI64)

extern ULONG MiMaximumSystemCacheSize;

//
// Define absolute minumum and maximum count for system ptes.
//

#define MM_MINIMUM_SYSTEM_PTES 7000

#define MM_MAXIMUM_SYSTEM_PTES 50000

#define MM_DEFAULT_SYSTEM_PTES 11000

//
// Pool limits
//

//
// The maximim amount of nonpaged pool that can be initially created.
//

#define MM_MAX_INITIAL_NONPAGED_POOL ((SIZE_T)(128 * 1024 * 1024))

//
// The total amount of nonpaged pool (initial pool + expansion + system PTEs).
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
// Structure layout defintions.
//

#define MM_PROTO_PTE_ALIGNMENT ((ULONG)PAGE_SIZE)

//
// Define the address bits mapped by PPE and PDE entries.
//
// A PPE entry maps 10+10+13 = 33 bits of address space.
// A PDE entry maps 10+13 = 23 bits of address space.
//

#define PAGE_DIRECTORY1_MASK (((ULONG_PTR)1 << PDI1_SHIFT) - 1)
#define PAGE_DIRECTORY2_MASK (((ULONG_PTR)1 << PDI_SHIFT) -1)

#define MM_VA_MAPPED_BY_PDE ((ULONG_PTR)1 << PDI_SHIFT)

#define LOWEST_IO_ADDRESS 0xa0000

//
// The number of bits in a physical address.
//

#define PHYSICAL_ADDRESS_BITS 44

#define MM_MAXIMUM_NUMBER_OF_COLORS (1)

//
// MERCED does not require support for colored pages.
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

#define FIRST_MAPPING_PTE   ((PMMPTE)HYPER_SPACE)

#define NUMBER_OF_MAPPING_PTES 255
#define LAST_MAPPING_PTE   \
     ((ULONG_PTR)((ULONG_PTR)FIRST_MAPPING_PTE + (NUMBER_OF_MAPPING_PTES * PAGE_SIZE)))

#define IMAGE_MAPPING_PTE   ((PMMPTE)((ULONG_PTR)LAST_MAPPING_PTE + PAGE_SIZE))

#define ZEROING_PAGE_PTE    ((PMMPTE)((ULONG_PTR)IMAGE_MAPPING_PTE + PAGE_SIZE))

#define WORKING_SET_LIST   ((PVOID)((ULONG_PTR)ZEROING_PAGE_PTE + PAGE_SIZE))

#define MM_MAXIMUM_WORKING_SET \
       ((ULONG)((ULONG)2*1024*1024*1024 - 64*1024*1024) >> PAGE_SHIFT) //2Gb-64Mb

#define MM_WORKING_SET_END (UADDRESS_BASE + 0x3FFFFFFFFFFUI64)

//
// Define memory attributes fields within PTE
//

#define MM_PTE_TB_MA_WB         (0x0 << 2) // cacheable, write-back
#define MM_PTE_TB_MA_UC         (0x4 << 2) // uncheable
#define MM_PTE_TB_MA_UCE        (0x5 << 2) // uncheable, exporting fetchadd
#define MM_PTE_TB_MA_WC         (0x6 << 2) // uncheable, coalesing
#define MM_PTE_TB_MA_NATPAGE    (0x7 << 2) // Nat Page

//
// Define masks for the PTE cache attributes
//

#define MM_PTE_CACHE_ENABLED     0     // WB
#define MM_PTE_CACHE_DISABLED    4     // UC
#define MM_PTE_CACHE_DISPLAY     6     // WC
#define MM_PTE_CACHE_RESERVED    1     // special encoding to cause a TLB miss

//
// Define masks for fields within the PTE.
//

#define MM_PTE_OWNER_MASK         0x0180
#define MM_PTE_VALID_MASK         1
#define MM_PTE_CACHE_DISABLE_MASK MM_PTE_TB_MA_UC
#define MM_PTE_ACCESS_MASK        0x0020
#define MM_PTE_DIRTY_MASK         0x0040
#define MM_PTE_EXECUTE_MASK       0x0200
#define MM_PTE_WRITE_MASK         0x0400
#define MM_PTE_LARGE_PAGE_MASK    0
#define MM_PTE_COPY_ON_WRITE_MASK ((ULONG)1 << (PAGE_SHIFT-1))

#define MM_PTE_PROTOTYPE_MASK     0x0002
#define MM_PTE_TRANSITION_MASK    0x0080

//
// Bit fields to or into PTE to make a PTE valid based on the
// protection field of the invalid PTE.
//

#define MM_PTE_NOACCESS          0x0
#define MM_PTE_READONLY          0x0
#define MM_PTE_READWRITE         MM_PTE_WRITE_MASK
#define MM_PTE_WRITECOPY         MM_PTE_COPY_ON_WRITE_MASK
#define MM_PTE_EXECUTE           MM_PTE_EXECUTE_MASK
#define MM_PTE_EXECUTE_READ      MM_PTE_EXECUTE_MASK
#define MM_PTE_EXECUTE_READWRITE MM_PTE_EXECUTE_MASK | MM_PTE_WRITE_MASK
#define MM_PTE_EXECUTE_WRITECOPY MM_PTE_EXECUTE_MASK | MM_PTE_COPY_ON_WRITE_MASK
#define MM_PTE_GUARD             0x0
#define MM_PTE_CACHE             MM_PTE_TB_MA_WB
#define MM_PTE_NOCACHE           MM_PTE_CACHE     // PAGE_NOCACHE is cached
#define MM_PTE_EXC_DEFER         0x10000000000000 // defer exception


#define MM_PROTECT_FIELD_SHIFT 2

//
// Define masks for fields within the EM TB entry
//

#define MM_PTE_TB_VALID          0x0001
#define MM_PTE_TB_ACCESSED       0x0020
#define MM_PTE_TB_MODIFIED       0x0040
#define MM_PTE_TB_WRITE          0x0400
#define MM_PTE_TB_EXECUTE        0x0200  // read/execute on EM
#define MM_PTE_TB_EXC_DEFER      0x10000000000000 // defer exception

//
// Define masks for PTE PageSize field
//

#define MM_PTE_1MB_PAGE          20
#define MM_PTE_2MB_PAGE          21
#define MM_PTE_4MB_PAGE          22
#define MM_PTE_16MB_PAGE         24
#define MM_PTE_64MB_PAGE         26
#define MM_PTE_256MB_PAGE        28

//
// Define the number of VHPT pages
//

#define MM_VHPT_PAGES           32

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

#define MM_DEMAND_ZERO_WRITE_PTE ((ULONGLONG)MM_READWRITE << MM_PROTECT_FIELD_SHIFT)


//
// A demand zero PTE with a protection or PAGE_READWRITE for system space.
//

#define MM_KERNEL_DEMAND_ZERO_PTE ((ULONGLONG)MM_READWRITE << MM_PROTECT_FIELD_SHIFT)

//
// A no access PTE for system space.
//

#define MM_KERNEL_NOACCESS_PTE ((ULONGLONG)MM_NOACCESS << MM_PROTECT_FIELD_SHIFT)

extern ULONG_PTR MmPteGlobal; // One if processor supports Global Page, else zero.

//
// Kernel stack alignment requirements.
//

#define MM_STACK_ALIGNMENT 0x0

#define MM_STACK_OFFSET 0x0

//
// System process definitions
//

#define PDE_PER_PAGE ((ULONG)(PAGE_SIZE/(1 << PTE_SHIFT)))
 
#define PTE_PER_PAGE ((ULONG)(PAGE_SIZE/(1 << PTE_SHIFT)))

#define PTE_PER_PAGE_BITS 11    // This handles the case where the page is full

#if PTE_PER_PAGE_BITS > 32
error - too many bits to fit into MMPTE_SOFTWARE or MMPFN.u1
#endif

//
// Number of page table pages for user addresses.
//

#define MM_USER_PAGE_TABLE_PAGES PTE_PER_PAGE


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

#if !defined(_MIALT4K_)

#define MI_MAKE_VALID_PTE(OUTPTE,FRAME,PMASK,PPTE)                            \
       (OUTPTE).u.Long = 0;                                                   \
       (OUTPTE).u.Hard.Valid = 1;                                             \
       (OUTPTE).u.Hard.Cache = MM_PTE_CACHE_ENABLED;                          \
       (OUTPTE).u.Hard.Accessed = 1;                                          \
       (OUTPTE).u.Hard.Exception = 1;                                         \
       (OUTPTE).u.Hard.PageFrameNumber = FRAME;                               \
       (OUTPTE).u.Hard.Owner = MI_DETERMINE_OWNER(PPTE);                      \
       (OUTPTE).u.Long |= (MmProtectToPteMask[PMASK]);

#endif

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

#if !defined(_MIALT4K_)

#define MI_MAKE_TRANSITION_PTE_VALID(OUTPTE,PPTE)                             \
        ASSERT (((PPTE)->u.Hard.Valid == 0) &&                                \
                ((PPTE)->u.Trans.Prototype == 0) &&                           \
                ((PPTE)->u.Trans.Transition == 1));                           \
       (OUTPTE).u.Long = (PPTE)->u.Long & 0x1FFFFFFFE000;                     \
       (OUTPTE).u.Hard.Valid = 1;                                             \
       (OUTPTE).u.Hard.Cache = MM_PTE_CACHE_ENABLED;                          \
       (OUTPTE).u.Hard.Accessed = 1;                                          \
       (OUTPTE).u.Hard.Exception = 1;                                         \
       (OUTPTE).u.Hard.Owner = MI_DETERMINE_OWNER(PPTE);                      \
       (OUTPTE).u.Long |= (MmProtectToPteMask[(PPTE)->u.Trans.Protection]);
#endif

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

#define MI_SET_PTE_WRITE_COMBINE(PTE)  \
    ((PTE).u.Hard.Cache = MM_PTE_CACHE_DISABLED)

#define MI_SET_PTE_WRITE_COMBINE2(PTE)  \
    ((PTE).u.Hard.Cache = MM_PTE_CACHE_DISPLAY)


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

#define MI_SET_PTE_DIRTY(PTE) (PTE).u.Hard.Dirty = 1


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

#define MI_SET_PTE_CLEAN(PTE) (PTE).u.Hard.Dirty = 0



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
//    STATE - Supplies 1 if global, 0 if not.
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

#define MI_ENABLE_CACHING(PTE) ((PTE).u.Hard.Cache = MM_PTE_CACHE_ENABLED)



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
//    PTE - Supplies a pointer to the valid PTE.
//
// Return Value:
//
//     None.
//
//--

#define MI_DISABLE_CACHING(PTE) ((PTE).u.Hard.Cache = MM_PTE_CACHE_DISABLED)




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

#define MI_IS_CACHING_DISABLED(PPTE) \
            ((PPTE)->u.Hard.Cache == MM_PTE_CACHE_DISABLED)



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

#define MI_SET_PFN_DELETED(PPFN) (((PPFN)->PteAddress = (PMMPTE)((INT_PTR)(LONG)0xFFFFFFFF)))




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
            ((PPFN)->PteAddress == (PMMPTE)((INT_PTR)(LONG)0xFFFFFFFF))


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
//    is compatable with the new address of the page.  If they are
//    not compatible, the D cache is flushed.
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

// does nothing on MERCED.

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
// Argments
//
//    None.
//
// Return Value:
//
//    None.
//
//--

// does nothing on MERCED.

#define MI_INITIALIZE_HYPERSPACE_MAP(INDEX)


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
// Argments
//
//
// Return Value:
//
//    The pages color.
//
//--


#define MI_PAGE_COLOR_PTE_PROCESS(PTE,COLOR)  \
         (ULONG)((ULONG_PTR)((*(COLOR))++) & MmSecondaryColorMask)



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

#define MI_GET_PREVIOUS_COLOR(COLOR)  (0)


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

#define MI_MAKE_VALID_PTE_WRITE_COPY(PPTE) \
                    if ((PPTE)->u.Hard.Write == 1) {    \
                        (PPTE)->u.Hard.CopyOnWrite = 1; \
                        (PPTE)->u.Hard.Write = 0;       \
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
//     3 if the owner is USER_MODE, 0 if the owner is KERNEL_MODE.
//
//--

#if defined(_MIALT4K_)

#define MI_DETERMINE_OWNER(PPTE)   \
     ((((((PPTE) >= (PMMPTE)PTE_UBASE) && ((PPTE) <= MiHighestUserPte))) || \
       (MI_IS_ALT_PAGE_TABLE_ADDRESS(PPTE))) ? 3 : 0)

#else

#define MI_DETERMINE_OWNER(PPTE)   \
    ((((PPTE) >= (PMMPTE)PTE_UBASE) && \
      ((PPTE) <= MiHighestUserPte)) ? 3 : 0)
#endif


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
// Argments
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

#define MI_SET_PAGING_FILE_INFO(OUTPTE,PTE,FILEINFO,OFFSET) \
        (OUTPTE).u.Long = (((PTE).u.Soft.Protection << MM_PROTECT_FIELD_SHIFT) | \
         ((ULONGLONG)(FILEINFO) << _MM_PAGING_FILE_LOW_SHIFT) | \
         ((ULONGLONG)(OFFSET) << _MM_PAGING_FILE_HIGH_SHIFT));


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
//    NOTE THAT AS PROTOPTE CAN ONLY RESIDE IN PAGED POOL!!!!!!
//
//    MAX SIZE = 2^(2+7+21) = 2^30 = 1GB.
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


#define MiPteToProto(lpte) \
            ((PMMPTE) ((ULONG_PTR)((lpte)->u.Proto.ProtoAddress) + MmProtopte_Base))

//++
//ULONG_PTR
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
        (( (ULONGLONG)((ULONG_PTR)proto_va - MmProtopte_Base) <<  \
          (_MM_PROTO_ADDRESS_SHIFT)) | MM_PTE_PROTOTYPE_MASK)

#define MISetProtoAddressForPte(PTE, proto_va) \
        (PTE).u.Long = 0;                      \
        (PTE).u.Proto.Prototype = 1;           \
        (PTE).u.Proto.ProtoAddress = (ULONG_PTR)proto_va - MmProtopte_Base;


//++
//ULONG_PTR
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
    (((lpte)->u.Subsect.WhichPool == 1) ?                              \
     ((PSUBSECTION)((ULONG_PTR)MmSubsectionBase +    \
                    ((ULONG_PTR)(lpte)->u.Subsect.SubsectionAddress))) \
     : \
     ((PSUBSECTION)((ULONG_PTR)MM_NONPAGED_POOL_END -    \
                    ((ULONG_PTR)(lpte)->u.Subsect.SubsectionAddress))))

//++
//ULONGLONG
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
   ( ((ULONG_PTR)(VA) < (ULONG_PTR)KSEG2_BASE) ?                  \
     ( ((ULONGLONG)((ULONG_PTR)VA - (ULONG_PTR)MmSubsectionBase) \
          << (_MM_PTE_SUBSECTION_ADDRESS_SHIFT)) | 0x80) \
     : \
       ((ULONGLONG)((ULONG_PTR)MM_NONPAGED_POOL_END - (ULONG_PTR)VA) \
          << (_MM_PTE_SUBSECTION_ADDRESS_SHIFT)) )

#define MiSetSubsectionAddressForPte(PTE, VA)  \
   (PTE).u.Long = 0;                          \
   if ((ULONG_PTR)(VA) < (ULONG_PTR)KSEG2_BASE) { \
       (PTE).u.Subsect.SubsectionAddress = (ULONG_PTR)VA - (ULONG_PTR)MmSubsectionBase; \
       (PTE).u.Subsect.WhichPool = 1;  \
   } else { \
       (PTE).u.Subsect.SubsectionAddress = (ULONG_PTR)MM_NONPAGED_POOL_END - (ULONG_PTR)VA; \
   }

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
//    LWFIX: expand for 3-level
//
//--

#define MiGetPpeOffset(va) ((ULONG)(((ULONG_PTR)(va) >> PDI1_SHIFT) & PDI_MASK))

//++
//ULONG_PTR
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

#define MiGetPdeOffset(va) ((ULONG) (((ULONG_PTR)(va) >> PDI_SHIFT) & PDI_MASK))

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

#define MiGetPpePdeOffset(va) ((ULONG) ((ULONG_PTR)(va) >> PDI_SHIFT))

//++
//ULONG_PTR
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

#define MiGetPteOffset(va) ((ULONG) (((ULONG_PTR)(va) >> PTI_SHIFT) & PDI_MASK))


//++
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
// Argments
//
//    PTE - Supplies the PTE to get the virtual address for.
//
// Return Value:
//
//    Virtual address mapped by the PTE.
//
//--

#ifdef  _WIN64

#define MiGetVirtualAddressMappedByPte(PTE) \
  (((ULONG_PTR)(PTE) & PTA_SIGN) ? \
   (PVOID)(((ULONG_PTR)(PTE) & VRN_MASK) | VA_FILL | \
           (((ULONG_PTR)(PTE)-PTE_BASE) << (PAGE_SHIFT - PTE_SHIFT))) : \
   (PVOID)(((ULONG_PTR)(PTE) & VRN_MASK) | (((ULONG_PTR)(PTE)-PTE_BASE) << (PAGE_SHIFT - PTE_SHIFT))))

#else

#define MiGetVirtualAddressMappedByPte(PTE) ((PVOID)((ULONG_PTR)(PTE) << (PAGE_SHIFT - PTE_SHIFT)))

#endif


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
//    TRUE if on a 4MB PDE boundary, FALSE if not.
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
// Argments
//
//    PTE - Supplies the PTE to operate upon.
//
// Return Value:
//
//    The paging file number.
//
//--

#define GET_PAGING_FILE_NUMBER(PTE) ((ULONG) (PTE).u.Soft.PageFileLow)



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

#define GET_PAGING_FILE_OFFSET(PTE) ((ULONG) (PTE).u.Soft.PageFileHigh)




//++
//ULONG_PTR
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

#define IS_PTE_NOT_DEMAND_ZERO(PTE) \
                 ((PTE).u.Long & ((ULONG_PTR)0xFFFFFFFFFFFFF000 |  \
                                  MM_PTE_VALID_MASK |       \
                                  MM_PTE_PROTOTYPE_MASK |   \
                                  MM_PTE_TRANSITION_MASK))


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



//++
//VOID
//MI_MAKE_PROTECT_WRITE_COPY (
//    IN OUT MMPTE PPTE
//    );
//
// Routine Description:
//
//    This macro makes a writable PTE a writeable-copy PTE.
//
// Argments
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
            if ((PPTE)->u.Hard.Dirty == 1) {                        \
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

#define MI_NO_FAULT_FOUND(TEMP,PPTE,VA,PFNHELD) \
        if (StoreInstruction && ((PPTE)->u.Hard.Dirty == 0)) {  \
            MiSetDirtyBit ((VA),(PPTE),(PFNHELD));     \
        }


//++
//ULONG_PTR
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
     ((((ULONG_PTR)(Va) >= KSEG3_BASE) && ((ULONG_PTR)(Va) < KSEG3_LIMIT)) || \
      (((ULONG_PTR)Va >= KSEG0_BASE) && ((ULONG_PTR)Va < KSEG2_BASE)))


//++
//ULONG_PTR
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

PVOID KiGetPhysicalAddress(
    IN PVOID VirtualAddress
    );

#define MI_CONVERT_PHYSICAL_TO_PFN(Va)   \
    (((ULONG_PTR)(Va) < KSEG0_BASE) ?                             \
     ((PFN_NUMBER)(((ULONG_PTR)(Va) - KSEG3_BASE) >> PAGE_SHIFT)) : \
     ((PFN_NUMBER)(((ULONG_PTR)KiGetPhysicalAddress(Va)) >> PAGE_SHIFT)))


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


//
// A VALID Page Table Entry on an Intel IA64 has the following definition.
//

#define _MM_PAGING_FILE_LOW_SHIFT 28
#define _MM_PAGING_FILE_HIGH_SHIFT 32

#define MI_PTE_LOOKUP_NEEDED ((ULONG64)0xffffffff)

typedef struct _MMPTE_SOFTWARE {
    ULONGLONG Valid : 1;
    ULONGLONG Prototype : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Transition : 1;
    ULONGLONG UsedPageTableEntries : PTE_PER_PAGE_BITS;
    ULONGLONG Reserved : 20 - PTE_PER_PAGE_BITS;
    ULONGLONG PageFileLow: 4;
    ULONGLONG PageFileHigh : 32;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION {
    ULONGLONG Valid : 1;
    ULONGLONG Prototype : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Transition : 1;
    ULONGLONG Rsvd0 : PAGE_SHIFT - 8;
    ULONGLONG PageFrameNumber : 50 - PAGE_SHIFT;
    ULONGLONG Rsvd1 : 14;
} MMPTE_TRANSITION;


#define _MM_PROTO_ADDRESS_SHIFT 12

typedef struct _MMPTE_PROTOTYPE {
    ULONGLONG Valid : 1;
    ULONGLONG Prototype : 1;
    ULONGLONG ReadOnly : 1;  // if set allow read only access.
    ULONGLONG Rsvd : 9;
    ULONGLONG ProtoAddress : 52;
} MMPTE_PROTOTYPE;


#define _MM_PTE_SUBSECTION_ADDRESS_SHIFT  12

typedef struct _MMPTE_SUBSECTION {
    ULONGLONG Valid : 1;
    ULONGLONG Prototype : 1;
    ULONGLONG Protection : 5;
    ULONGLONG WhichPool : 1;
    ULONGLONG Rsvd : 4;
    ULONGLONG SubsectionAddress : 52;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST {
    ULONGLONG Valid : 1;
    ULONGLONG OneEntry : 1;
    ULONGLONG filler10 : 10;
    ULONGLONG NextEntry : 32;
    ULONGLONG Rsvd : 20;
} MMPTE_LIST;


//
// A Page Table Entry on an Intel IA64 has the following definition.
//

#define _HARDWARE_PTE_WORKING_SET_BITS  11

typedef struct _MMPTE_HARDWARE {
    ULONGLONG Valid : 1;
    ULONGLONG Rsvd0 : 1;
    ULONGLONG Cache : 3;
    ULONGLONG Accessed : 1;
    ULONGLONG Dirty : 1;
    ULONGLONG Owner : 2;
    ULONGLONG Execute : 1;
    ULONGLONG Write : 1;
    ULONGLONG Rsvd1 : PAGE_SHIFT - 12;
    ULONGLONG CopyOnWrite : 1;
    ULONGLONG PageFrameNumber : 50 - PAGE_SHIFT;
    ULONGLONG Rsvd2 : 2;
    ULONGLONG Exception : 1;
    ULONGLONG SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

typedef struct _MMPTE_LARGEPAGE {
    ULONGLONG Valid : 1;
    ULONGLONG Rsvd0 : 1;
    ULONGLONG Cache : 3;
    ULONGLONG Accessed : 1;
    ULONGLONG Dirty : 1;
    ULONGLONG Owner : 2;
    ULONGLONG Execute : 1;
    ULONGLONG Write : 1;
    ULONGLONG Rsvd1 : PAGE_SHIFT - 12;
    ULONGLONG CopyOnWrite : 1;
    ULONGLONG PageFrameNumber : 50 - PAGE_SHIFT;
    ULONGLONG Rsvd2 : 2;
    ULONGLONG Exception : 1;
    ULONGLONG Rsvd3 : 1;
    ULONGLONG LargePage : 1;
    ULONGLONG PageSize : 6;
    ULONGLONG Rsvd4 : 3;
} MMPTE_LARGEPAGE, *PMMPTE_LARGEPAGE;

typedef struct _ALT_4KPTE {
    ULONGLONG Commit : 1;
    ULONGLONG Rsvd0 : 1;
    ULONGLONG Cache : 3;
    ULONGLONG Accessed : 1;
    ULONGLONG Dirty : 1;
    ULONGLONG Owner : 2;
    ULONGLONG Execute : 1;
    ULONGLONG Write : 1;
    ULONGLONG Rsvd1 : 1;
    ULONGLONG PteOffset : 32;
    ULONGLONG Rsvd2 : 8;
    ULONGLONG Exception : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Lock : 1;
    ULONGLONG FillZero : 1;
    ULONGLONG NoAccess : 1;
    ULONGLONG CopyOnWrite : 1;
    ULONGLONG PteIndirect : 1;
    ULONGLONG Private : 1;
} ALT_4KPTE, *PALT_4KPTE;

#define MI_GET_PAGE_FRAME_FROM_PTE(PTE) ((ULONG)((PTE)->u.Hard.PageFrameNumber))
#define MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(PTE) ((ULONG)((PTE)->u.Trans.PageFrameNumber))
#define MI_GET_PROTECTION_FROM_SOFT_PTE(PTE) ((ULONG)((PTE)->u.Soft.Protection))
#define MI_GET_PROTECTION_FROM_TRANSITION_PTE(PTE) ((ULONG)((PTE)->u.Trans.Protection))


typedef struct _MMPTE {
    union  {
        ULONGLONG Long;
        MMPTE_HARDWARE Hard;
        MMPTE_LARGEPAGE Large;
        HARDWARE_PTE Flush;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TRANSITION Trans;
        MMPTE_SUBSECTION Subsect;
        MMPTE_LIST List;
        ALT_4KPTE Alt;
        } u;
} MMPTE;

typedef MMPTE *PMMPTE;

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

//
// For EM build, need to export this function to ps/psldt.c
//

extern PVOID
MiCreatePebOrTeb (
    IN PEPROCESS TargetProcess,
    IN ULONG Size
    );


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
             RtlFillMemoryUlonglong ((Destination), (Length), (Pattern))


#define KiWbInvalidateCache


//++
//BOOLEAN
//MI_IS_PAGE_TABLE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
//    page table address (PTE, PDE, PPE).
//
// Arguments
//
//    VA - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a page table address, TRUE if it is.
//
//--

#if defined(_MIALT4K_)
#define MI_IS_PAGE_TABLE_ADDRESS(VA) \
    ((((ULONG_PTR)VA >= PTE_UBASE) && ((ULONG_PTR)VA <= (PDE_UTBASE + PAGE_SIZE))) || \
     (((ULONG_PTR)VA >= PTE_KBASE) && ((ULONG_PTR)VA <= (PDE_KTBASE + PAGE_SIZE))) || \
     (((ULONG_PTR)VA >= PTE_SBASE) && ((ULONG_PTR)VA <= (PDE_STBASE + PAGE_SIZE))) || \
     (((ULONG_PTR)VA >= ALT4KB_PERMISSION_TABLE_START) && \
      ((ULONG_PTR)VA <= ALT4KB_PERMISSION_TABLE_END)))
#else
#define MI_IS_PAGE_TABLE_ADDRESS(VA) \
    ((((ULONG_PTR)VA >= PTE_UBASE) && ((ULONG_PTR)VA <= (PDE_UTBASE + PAGE_SIZE))) || \
     (((ULONG_PTR)VA >= PTE_KBASE) && ((ULONG_PTR)VA <= (PDE_KTBASE + PAGE_SIZE))) || \
     (((ULONG_PTR)VA >= PTE_SBASE) && ((ULONG_PTR)VA <= (PDE_STBASE + PAGE_SIZE))))
#endif

//++
//BOOLEAN
//MI_IS_HYPER_SPACE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address resides in 
//    the hyper space.
//
// Arguments
//
//    VA - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a hyper space address, TRUE if it is.
//
//--

#define MI_IS_HYPER_SPACE_ADDRESS(VA) \
    (((ULONG_PTR)VA >= (ULONG_PTR)HYPER_SPACE) && ((ULONG_PTR)VA <= HYPER_SPACE_END))

//++
//BOOLEAN
//MI_IS_PTE_ADDRESS (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
//    page table page (PTE) address.
//
// Arguments
//
//    PTE - Supplies the PTE virtual address.
//
// Return Value:
//
//    FALSE if it is not a PTE address, TRUE if it is.
//
//--

#define MI_IS_PTE_ADDRESS(PTE) \
    (((PTE >= (PMMPTE)PTE_UBASE) && (PTE <= (PMMPTE)PTE_UTOP)) || \
     ((PTE >= (PMMPTE)PTE_KBASE) && (PTE <= (PMMPTE)PTE_KTOP)) || \
     ((PTE >= (PMMPTE)PTE_SBASE) && (PTE <= (PMMPTE)PTE_STOP)))


#define MI_IS_PPE_ADDRESS(PTE) \
    (((PTE >= (PMMPTE)PDE_UTBASE) && (PTE <= (PMMPTE)(PDE_UTBASE + PAGE_SIZE))) || \
     ((PTE >= (PMMPTE)PDE_KTBASE) && (PTE <= (PMMPTE)(PDE_KTBASE + PAGE_SIZE))) || \
     ((PTE >= (PMMPTE)PDE_STBASE) && (PTE <= (PMMPTE)(PDE_STBASE + PAGE_SIZE))))

//++
//BOOLEAN
//MI_IS_KERNEL_PTE_ADDRESS (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
//    kernel page table page (PTE) address.
//
// Arguments
//
//    PTE - Supplies the PTE virtual address.
//
// Return Value:
//
//    FALSE if it is not a kernel PTE address, TRUE if it is.
//
//--

#define MI_IS_KERNEL_PTE_ADDRESS(PTE) \
     (((PMMPTE)PTE >= (PMMPTE)PTE_KBASE) && ((PMMPTE)PTE <= (PMMPTE)PTE_KTOP))


//++
//BOOLEAN
//MI_IS_USER_PTE_ADDRESS (
//    IN PMMPTE PTE
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
//    page table page (PTE) address.
//
// Arguments
//
//    PTE - Supplies the PTE virtual address.
//
// Return Value:
//
//    FALSE if it is not a PTE address, TRUE if it is.
//
//--

#define MI_IS_USER_PTE_ADDRESS(PTE) \
    ((PTE >= (PMMPTE)PTE_UBASE) && (PTE <= (PMMPTE)PTE_UTOP))


//++
//BOOLEAN
//MI_IS_PAGE_DIRECTORY_ADDRESS (
//    IN PMMPTE PDE
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
//    page directory page (PDE) address.
//
// Arguments
//
//    PDE - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a PDE address, TRUE if it is.
//
//--

#define MI_IS_PAGE_DIRECTORY_ADDRESS(PDE) \
    (((PDE >= (PMMPTE)PDE_UBASE) && (PDE <= (PMMPTE)PDE_UTOP)) || \
     ((PDE >= (PMMPTE)PDE_KBASE) && (PDE <= (PMMPTE)PDE_KTOP)) || \
     ((PDE >= (PMMPTE)PDE_SBASE) && (PDE <= (PMMPTE)PDE_STOP)))

//++
//BOOLEAN
//MI_IS_USER_PDE_ADDRESS (
//    IN PMMPTE PDE
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
//    user page directory page (PDE) address.
//
// Arguments
//
//    PDE - Supplies the PDE virtual address.
//
// Return Value:
//
//    FALSE if it is not a user PDE address, TRUE if it is.
//
//--

#define MI_IS_USER_PDE_ADDRESS(PDE) \
    ((PDE >= (PMMPTE)PDE_UBASE) && (PDE <= (PMMPTE)PDE_UTOP)) 


//++
//BOOLEAN
//MI_IS_KERNEL_PDE_ADDRESS (
//    IN PMMPTE PDE
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address is really a
//    kernel page directory page (PDE) address.
//
// Arguments
//
//    PDE - Supplies the PDE virtual address.
//
// Return Value:
//
//    FALSE if it is not a user PDE address, TRUE if it is.
//
//--

#define MI_IS_KERNEL_PDE_ADDRESS(PDE) \
    ((PDE >= (PMMPTE)PDE_KBASE) && (PDE <= (PMMPTE)PDE_KTOP)) 


//++
//BOOLEAN
//MI_IS_PROCESS_SPACE_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address resides in 
//    the per-process space.
//
// Arguments
//
//    VA - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a per-process address, TRUE if it is.
//
//--

#define MI_IS_PROCESS_SPACE_ADDRESS(VA) (((ULONG_PTR)VA >> 61) == UREGION_INDEX)

//++
//BOOLEAN
//MI_IS_SYSTEM_ADDRESS (
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    This macro determines if a given virtual address resides in 
//    the system (global) space.
//
// Arguments
//
//    VA - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a system (global) address, TRUE if it is.
//
//--

#define MI_IS_SYSTEM_ADDRESS(VA) (((ULONG_PTR)VA >> 61) == KREGION_INDEX)


//
// Generate kernel segment physical address
//

//PVOID
//MiGetKSegAddress (
//    PFN_NUMBER FrameNumber
//    );

//
//++
//PVOID
//KSEG_ADDRESS (
//    IN ULONG PAGE
//    );
//
// Routine Description:
//
//    This macro returns a KSEG virtual address which maps the page.
//
// Arguments:
//
//    PAGE - Supplies the physical page frame number
//
// Return Value:
//
//    The address of the KSEG address
//
//--

// #define KSEG_ADDRESS(PAGE) ((PVOID)(KSEG3_BASE | ((ULONG_PTR)(PAGE) << PAGE_SHIFT)))

// #define KSEG_ADDRESS(PAGE) MiGetKSegAddress ((ULONG)PAGE)


//
//++
//PVOID
//KSEG0_ADDRESS (
//    IN ULONG PAGE
//    );
//
// Routine Description:
//
//    This macro returns a KSEG0 virtual address which maps the page.
//
// Arguments:
//
//    PAGE - Supplies the physical page frame number
//
// Return Value:
//
//    The address of the KSEG address
//
//--

#define KSEG0_ADDRESS(PAGE) \
     (PVOID)(KSEG0_BASE | ((PAGE) <<  PAGE_SHIFT)) 


extern MMPTE ValidPpePte;

extern PFN_NUMBER MmSystemParentTablePage;

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
//    The address of the PPE.   // LWFIX: expand for 3 level
//
//--

#if PTE_THASH

__inline
PMMPTE
MiGetPpeAddress_XX(
    IN PVOID Va
    )
{
    if (((ULONG_PTR)(Va) & PTE_BASE) == PTE_BASE) {
        
        return ((PMMPTE)(((ULONG_PTR)Va & VRN_MASK) | (PDE_TBASE + PAGE_SIZE - sizeof(MMPTE))));

    } else {

        return ((PMMPTE)(__thash(__thash(__thash((ULONG_PTR)Va)))));

    }
}

#define MiGetPpeAddress(va) MiGetPpeAddress_XX((PVOID)(va))

#else

#define MiGetPpeAddress(Va) \
  (((((ULONG_PTR)(Va)) & PTE_BASE) == PTE_BASE) ? \
   ((PMMPTE)((((ULONG_PTR)(Va)) & VRN_MASK) | (PDE_TBASE + PAGE_SIZE - sizeof(MMPTE)))) :\
   ((PMMPTE)(((((ULONG_PTR)(Va)) & VRN_MASK)) | \
              ((((((ULONG_PTR)(Va)) >> PDI1_SHIFT) << PTE_SHIFT) & \
                (~(PDE_TBASE|VRN_MASK)) ) + PDE_TBASE))))

#endif

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

#if PTE_THASH

__inline
PMMPTE
MiGetPdeAddress_XX(
    IN PVOID Va
    )
{
    if (((ULONG_PTR)(Va) & PDE_BASE) == PDE_BASE) {
        
        return ((PMMPTE)(((ULONG_PTR)Va & VRN_MASK) | (PDE_TBASE + PAGE_SIZE - sizeof(MMPTE))));

    } else {

       return ((PMMPTE)(__thash(__thash((ULONG_PTR)(Va)))));
    }
}

#define MiGetPdeAddress(va) MiGetPdeAddress_XX((PVOID)(va))

#else

#define MiGetPdeAddress(Va) \
  (((((ULONG_PTR)(Va)) & PDE_BASE) == PDE_BASE) ? \
   ((PMMPTE)((((ULONG_PTR)(Va)) & VRN_MASK) | (PDE_TBASE + PAGE_SIZE - sizeof(MMPTE)))) :\
   ((PMMPTE)(((((ULONG_PTR)(Va)) & VRN_MASK)) | \
             ((((((ULONG_PTR)(Va)) >> PDI_SHIFT) << PTE_SHIFT) & (~(PDE_BASE|VRN_MASK))) + PDE_BASE))))

#endif


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

#if PTE_THASH

__inline
PMMPTE
MiGetPteAddress_XX(
    IN PVOID Va
    )
{
    if (((ULONG_PTR)(Va) & PDE_TBASE) == PDE_TBASE) {
        
        return ((PMMPTE)(((ULONG_PTR)Va & VRN_MASK) | (PDE_TBASE + PAGE_SIZE - sizeof(MMPTE))));

    } else {

        return ((PMMPTE)(__thash((ULONG_PTR)(Va))));

    }
}

#define MiGetPteAddress(va) MiGetPteAddress_XX((PVOID)(va))

#else

#define MiGetPteAddress(Va) \
  (((((ULONG_PTR)(Va)) & PDE_TBASE) == PDE_TBASE) ? \
   ((PMMPTE)((((ULONG_PTR)(Va)) & VRN_MASK) | (PDE_TBASE + PAGE_SIZE - sizeof(MMPTE)))) :\
   ((PMMPTE)(((((ULONG_PTR)(Va)) & VRN_MASK)) | \
             ((((((ULONG_PTR)(Va)) >> PTI_SHIFT) << PTE_SHIFT) & (~(PTE_BASE|VRN_MASK))) + PTE_BASE))))

#endif


#define MI_IS_PTE_PROTOTYPE(PointerPte)  (!MI_IS_USER_PTE_ADDRESS (PointerPte))

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


#if defined(_MIALT4K_)

//
// Define constants and macros for alternate 4kb table.
//
// These are constants and defines that mimic the PAGE_SIZE constant but are
// hard coded to use 4K page values.
//

#define PAGE_4K         4096
#define PAGE_4K_SHIFT   12
#define PAGE_4K_MASK    (PAGE_4K - 1)
#define PAGE_4K_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_4K - 1)))
#define ROUND_TO_4K_PAGES(Size)  (((ULONG_PTR)(Size) + PAGE_4K - 1) & ~(PAGE_4K - 1))

#define PAGE_NEXT_ALIGN(Va) ((PVOID)(PAGE_ALIGN((ULONG_PTR)Va + PAGE_SIZE - 1)))

#define BYTES_TO_4K_PAGES(Size)  ((ULONG)((ULONG_PTR)(Size) >> PAGE_4K_SHIFT) + \
                               (((ULONG)(Size) & (PAGE_4K - 1)) != 0))

//
// Relative constants between native pages and 4K pages
//

#define SPLITS_PER_PAGE (PAGE_SIZE / PAGE_4K)
#define PAGE_SHIFT_DIFF (PAGE_SHIFT - PAGE_4K_SHIFT)

#define ALT_PTE_SHIFT 3

#define ALT_PROTECTION_MASK (MM_PTE_EXECUTE_MASK|MM_PTE_WRITE_MASK)

#define MiGetAltPteAddress(VA) \
      ((PMMPTE) (ALT4KB_PERMISSION_TABLE_START + \
                     ((((ULONG_PTR) (VA)) >> PAGE_4K_SHIFT) << ALT_PTE_SHIFT)))

//
// define Alternate 4k table flags
//

#define MI_ALTFLG_FLUSH2G         0x0000000000000001

//
// define MiProtectFor4kPage flages
//

#define ALT_ALLOCATE      1
#define ALT_COMMIT        2
#define ALT_CHANGE        4

//
// define ATE protection bits
//

#define MM_ATE_COMMIT             0x0000000000000001
#define MM_ATE_ACCESS             0x0000000000000020

#define MM_ATE_READONLY           0x0000000000000200
#define MM_ATE_EXECUTE            0x0400000000000200  
#define MM_ATE_EXECUTE_READ       0x0400000000000200
#define MM_ATE_READWRITE          0x0000000000000600
#define MM_ATE_WRITECOPY          0x0020000000000200
#define MM_ATE_EXECUTE_READWRITE  0x0400000000000600
#define MM_ATE_EXECUTE_WRITECOPY  0x0420000000000400

#define MM_ATE_ZEROFILL           0x0800000000000000
#define MM_ATE_NOACCESS           0x1000000000000000
#define MM_ATE_COPY_ON_WRITE      0x2000000000000000
#define MM_ATE_PRIVATE            0x8000000000000000
#define MM_ATE_PROTO_MASK         0x0000000000000621


//
// How big is the IA32 subsystem allowed?
// Assume it will be "not Large Address Aware" so limited to 2G
//
#define _MAX_WOW64_ADDRESS       (0x00000000080000000UI64)

MmX86Fault (
    IN BOOLEAN StoreInstruction,
    IN PVOID VirtualAddress, 
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
    );

VOID
MiProtectFor4kPage(
    IN PVOID Base,
    IN SIZE_T Size,
    IN ULONG NewProtect,
    IN ULONG Flags,
    IN PEPROCESS Process
    );

VOID
MiProtectMapFileFor4kPage(
    IN PVOID Base,
    IN SIZE_T Size,
    IN ULONG NewProtect,
    IN PMMPTE PointerPte,
    IN PEPROCESS Process
    );

VOID
MiProtectImageFileFor4kPage(
    IN PVOID Base,
    IN SIZE_T Size,
    IN PMMPTE PointerPte,
    IN PEPROCESS Process
    );

VOID
MiReleaseFor4kPage(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    );

VOID
MiDecommitFor4kPage(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    );

VOID
MiDeleteFor4kPage(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    );

VOID
MiQueryRegionFor4kPage(
    IN PVOID BaseAddress,
    IN PVOID EndAddress,
    IN OUT PSIZE_T RegionSize,
    IN OUT PULONG RegionState,
    IN OUT PULONG RegionProtect,
    IN PEPROCESS Process
    );

ULONG
MiQueryProtectionFor4kPage (
    IN PVOID BaseAddress,
    IN PEPROCESS Process
    );

NTSTATUS
MiInitializeAlternateTable(
    PEPROCESS Process
    );

VOID
MiDeleteAlternateTable(
    PEPROCESS Process
    );

VOID
MiLockFor4kPage(
    PVOID CapturedBase,
    SIZE_T CapturedRegionSize,
    PEPROCESS Process
    );

NTSTATUS
MiUnlockFor4kPage(
    PVOID CapturedBase,
    SIZE_T CapturedRegionSize,
    PEPROCESS Process
    );

BOOLEAN
MiShouldBeUnlockedFor4kPage(
    PVOID VirtualAddress,
    PEPROCESS Process
    );

VOID
MiMarkSplitPages(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PULONG Bitmap,
    IN BOOLEAN SetBit
    );

ULONG
MiMakeProtectForNativePage(
    IN PVOID VirtualAddress,
    IN ULONG NewProtect,
    IN PEPROCESS Process
    );


extern ULONG MmProtectToPteMaskForIA32[32];
extern ULONG MmProtectToPteMaskForSplit[32];
extern ULONGLONG MmProtectToAteMask[32];


#define MiMakeProtectionAteMask(NewProtect) MmProtectToAteMask[NewProtect]


#define _ALTPERM_BITMAP_MASK ((_MAX_WOW64_ADDRESS - 1) >> PTI_SHIFT)

#if 1
#define MI_MAKE_VALID_PTE(OUTPTE,FRAME,PMASK,PPTE)                           \
       (OUTPTE).u.Long = 0;                                                  \
       (OUTPTE).u.Hard.Valid = 1;                                            \
       (OUTPTE).u.Hard.Cache = MM_PTE_CACHE_ENABLED;                         \
       (OUTPTE).u.Hard.Exception = 1;                                        \
       (OUTPTE).u.Hard.PageFrameNumber = FRAME;                              \
       (OUTPTE).u.Hard.Owner = MI_DETERMINE_OWNER(PPTE);                     \
{ \
  PWOW64_PROCESS _Wow64Process = PsGetCurrentProcess()->Wow64Process; \
  if ((_Wow64Process != NULL) && \
      ((PPTE >= (PMMPTE)PTE_UBASE) && \
       (PPTE < MiGetPteAddress(_MAX_WOW64_ADDRESS)))) {  \
      if (MI_CHECK_BIT(_Wow64Process->AltPermBitmap, \
             ((ULONG_PTR)PPTE >> PTE_SHIFT) & _ALTPERM_BITMAP_MASK) != 0) { \
          (OUTPTE).u.Long |= (MmProtectToPteMaskForSplit[PMASK]); \
      } else { \
          (OUTPTE).u.Long |= (MmProtectToPteMaskForIA32[PMASK]); \
          (OUTPTE).u.Hard.Accessed = 1; \
      } \
  } else { \
      (OUTPTE).u.Hard.Accessed = 1; \
      (OUTPTE).u.Long |= (MmProtectToPteMask[PMASK]);\
  }\
}


#define MI_MAKE_TRANSITION_PTE_VALID(OUTPTE,PPTE)                        \
        ASSERT (((PPTE)->u.Hard.Valid == 0) &&                           \
                ((PPTE)->u.Trans.Prototype == 0) &&                      \
                ((PPTE)->u.Trans.Transition == 1));                      \
       (OUTPTE).u.Long = (PPTE)->u.Long & 0x1FFFFFFFE000;                \
       (OUTPTE).u.Hard.Valid = 1;                                        \
       (OUTPTE).u.Hard.Cache = MM_PTE_CACHE_ENABLED;                     \
       (OUTPTE).u.Hard.Exception = 1;                                    \
       (OUTPTE).u.Hard.Owner = MI_DETERMINE_OWNER(PPTE);                 \
{ \
  PWOW64_PROCESS _Wow64Process = PsGetCurrentProcess()->Wow64Process; \
  if ((_Wow64Process != NULL) && \
      ((PPTE >= (PMMPTE)PTE_UBASE) && \
       (PPTE < MiGetPteAddress(_MAX_WOW64_ADDRESS)))) { \
      if (MI_CHECK_BIT(_Wow64Process->AltPermBitmap, \
             ((ULONG_PTR)PPTE >> PTE_SHIFT) & _ALTPERM_BITMAP_MASK) != 0) { \
          (OUTPTE).u.Long |= (MmProtectToPteMaskForSplit[(PPTE)->u.Trans.Protection]); \
      } else { \
          (OUTPTE).u.Long |= (MmProtectToPteMaskForIA32[(PPTE)->u.Trans.Protection]); \
          (OUTPTE).u.Hard.Accessed = 1; \
      } \
  } else { \
      (OUTPTE).u.Hard.Accessed = 1; \
      (OUTPTE).u.Long |=(MmProtectToPteMask[(PPTE)->u.Trans.Protection]);\
  }\
}
#else
#define MI_MAKE_VALID_PTE(OUTPTE,FRAME,PMASK,PPTE)                           \
       (OUTPTE).u.Long = 0;                                                  \
       (OUTPTE).u.Hard.Valid = 1;                                            \
       (OUTPTE).u.Hard.Cache = MM_PTE_CACHE_ENABLED;                         \
       (OUTPTE).u.Hard.Exception = 1;                                        \
       (OUTPTE).u.Hard.PageFrameNumber = FRAME;                              \
       (OUTPTE).u.Hard.Owner = MI_DETERMINE_OWNER(PPTE);                     \
{ \
  PWOW64_PROCESS _Wow64Process = PsGetCurrentProcess()->Wow64Process; \
  if ((_Wow64Process != NULL) && \
      ((PPTE >= (PMMPTE)PTE_UBASE) && \
       (PPTE < MiGetPteAddress(_MAX_WOW64_ADDRESS)))) {  \
      (OUTPTE).u.Long |= MM_PTE_READONLY; \
  } else { \
      (OUTPTE).u.Hard.Accessed = 1; \
      (OUTPTE).u.Long |= (MmProtectToPteMask[PMASK]);\
  }\
}


#define MI_MAKE_TRANSITION_PTE_VALID(OUTPTE,PPTE)                        \
        ASSERT (((PPTE)->u.Hard.Valid == 0) &&                           \
                ((PPTE)->u.Trans.Prototype == 0) &&                      \
                ((PPTE)->u.Trans.Transition == 1));                      \
       (OUTPTE).u.Long = (PPTE)->u.Long & 0x1FFFFFFFE000;                \
       (OUTPTE).u.Hard.Valid = 1;                                        \
       (OUTPTE).u.Hard.Cache = MM_PTE_CACHE_ENABLED;                     \
       (OUTPTE).u.Hard.Exception = 1;                                    \
       (OUTPTE).u.Hard.Owner = MI_DETERMINE_OWNER(PPTE);                 \
{ \
  PWOW64_PROCESS _Wow64Process = PsGetCurrentProcess()->Wow64Process; \
  if ((_Wow64Process != NULL) && \
      ((PPTE >= (PMMPTE)PTE_UBASE) && \
       (PPTE < MiGetPteAddress(_MAX_WOW64_ADDRESS)))) { \
      (OUTPTE).u.Long |= MM_PTE_READONLY; \
  } else { \
      (OUTPTE).u.Hard.Accessed = 1; \
      (OUTPTE).u.Long |=(MmProtectToPteMask[(PPTE)->u.Trans.Protection]);\
  }\
}
#endif

#define LOCK_ALTERNATE_TABLE(PWOW64) \
        ExAcquireFastMutex( &(PWOW64)->AlternateTableLock)

#define UNLOCK_ALTERNATE_TABLE(PWOW64) \
        ExReleaseFastMutex(&(PWOW64)->AlternateTableLock)

#define MI_IS_ALT_PAGE_TABLE_ADDRESS(PPTE) \
            (((PPTE) >= (PMMPTE)ALT4KB_PERMISSION_TABLE_START) && \
             ((PPTE) < (PMMPTE)ALT4KB_PERMISSION_TABLE_END))

#endif

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

// currently does nothing on MERCED.

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

// currently does nothing on MERCED.

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
//    Since IA64 supports ASNs and session space doesn't have one, the entire
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
//    MI_FLUSH_ENTIRE_SESSION_TB flushes the entire TB on IA64 since
//    the IA64 supports ASNs.
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

VOID
MiSweepCacheMachineDependent(
    IN PVOID VirtualAddress,
    IN SIZE_T Size,
    IN MEMORY_CACHING_TYPE CacheType
    );

#define MI_IS_PCR_PAGE(Va) (((PVOID)PCR <= Va) && (Va < (PVOID)(PCR+PAGE_SIZE)))

//++
//BOOLEAN
//MI_IS_ADDRESS_VALID_FOR_KD (
//    IN PVOID VirtualAddress
//    );
//
// Routine Description:
//
//    This macro deterines if a give virtual address is really a valid 
//    virtual address for the kerenl debugger memory references
//
// Argments
//
//    VirtualAddress - Supplies the virtual address.
//
// Return Value:
//
//    FALSE if it is not a physical address, TRUE if it is.
//
//--


#define MI_IS_ADDRESS_VALID_FOR_KD(VirtualAddress)  \
         ((VirtualAddress <= (PVOID)(HYPER_SPACE_END)) || \
          (MI_IS_PTE_ADDRESS((PMMPTE)VirtualAddress)) || \
          (MI_IS_PAGE_DIRECTORY_ADDRESS((PMMPTE)VirtualAddress)) || \
          (MI_IS_PPE_ADDRESS((PMMPTE)VirtualAddress)) || \
          ((VirtualAddress >= MM_SYSTEM_RANGE_START) && \
           (VirtualAddress < (PVOID)MM_SYSTEM_SPACE_END)) || \
          (MI_IS_SESSION_ADDRESS(VirtualAddress)) || \
          (MI_IS_PHYSICAL_ADDRESS(VirtualAddress)) || \
          (MI_IS_PCR_PAGE(VirtualAddress)))
