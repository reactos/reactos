/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mi.h

Abstract:

    This module contains the private data structures and procedure
    prototypes for the memory management system.

Author:

    Lou Perazzoli (loup) 20-Mar-1989
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/

#ifndef _MI_
#define _MI_

#include "ntos.h"
#include "ntimage.h"
#include "ki.h"
#include "fsrtl.h"
#include "zwapi.h"
#include "pool.h"
#include "ntiodump.h"
#include "stdio.h"
#include "string.h"
#include "safeboot.h"
#include "triage.h"

#if defined(_X86_)
#include "..\mm\i386\mi386.h"

#elif defined(_AXP64_)
#include "..\mm\axp64\mialpha.h"

#elif defined(_ALPHA_)
#include "..\mm\alpha\mialpha.h"

#elif defined(_IA64_)
#include "..\mm\ia64\miia64.h"

#else
#error "mm: a target architecture must be defined."
#endif

#if defined (_WIN64)
#define ASSERT32(exp)
#define ASSERT64(exp)   ASSERT(exp)
#else
#define ASSERT32(exp)   ASSERT(exp)
#define ASSERT64(exp)
#endif

//
// Special pool constants
//
#define MI_SPECIAL_POOL_PAGABLE         0x8000
#define MI_SPECIAL_POOL_VERIFIER        0x4000
#define MI_SPECIAL_POOL_PTE_PAGABLE     0x0002
#define MI_SPECIAL_POOL_PTE_NONPAGABLE  0x0004


#define _2gb 0x80000000                 // 2 gigabytes
#define _4gb 0x100000000                // 4 gigabytes

#define MM_FLUSH_COUNTER_MASK (0xFFFFF)

#define MM_FREE_WSLE_SHIFT 4

#define WSLE_NULL_INDEX ((ULONG)0xFFFFFFF)

#define MM_FREE_POOL_SIGNATURE (0x50554F4C)

#define MM_MINIMUM_PAGED_POOL_NTAS ((SIZE_T)(48*1024*1024))

#define MM_ALLOCATION_FILLS_VAD ((PMMPTE)(ULONG_PTR)~3)

#define MM_WORKING_SET_LIST_SEARCH 17

#define MM_FLUID_WORKING_SET 8

#define MM_FLUID_PHYSICAL_PAGES 32  //see MmResidentPages below.

#define MM_USABLE_PAGES_FREE 32

#define X64K (ULONG)65536

#define MM_HIGHEST_VAD_ADDRESS ((PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (64 * 1024)))


#define MM_NO_WS_EXPANSION ((PLIST_ENTRY)0)
#define MM_WS_EXPANSION_IN_PROGRESS ((PLIST_ENTRY)35)
#define MM_WS_SWAPPED_OUT ((PLIST_ENTRY)37)
#define MM_IO_IN_PROGRESS ((PLIST_ENTRY)97)  // MUST HAVE THE HIGHEST VALUE

#define MM_PAGES_REQUIRED_FOR_MAPPED_IO 7

#define MM4K_SHIFT    12  //MUST BE LESS THAN OR EQUAL TO PAGE_SHIFT
#define MM4K_MASK  0xfff

#define MMSECTOR_SHIFT 9  //MUST BE LESS THAN OR EQUAL TO PAGE_SHIFT

#define MMSECTOR_MASK 0x1ff

#define MM_LOCK_BY_REFCOUNT 0

#define MM_LOCK_BY_NONPAGE 1

#define MM_FORCE_TRIM 6

#define MM_GROW_WSLE_HASH 20

#define MM_MAXIMUM_WRITE_CLUSTER (MM_MAXIMUM_DISK_IO_SIZE / PAGE_SIZE)

//
// Number of PTEs to flush singularly before flushing the entire TB.
//

#define MM_MAXIMUM_FLUSH_COUNT (FLUSH_MULTIPLE_MAXIMUM-1)

//
// Page protections
//

#define MM_ZERO_ACCESS         0  // this value is not used.
#define MM_READONLY            1
#define MM_EXECUTE             2
#define MM_EXECUTE_READ        3
#define MM_READWRITE           4  // bit 2 is set if this is writable.
#define MM_WRITECOPY           5
#define MM_EXECUTE_READWRITE   6
#define MM_EXECUTE_WRITECOPY   7

#define MM_NOCACHE            0x8
#define MM_GUARD_PAGE         0x10
#define MM_DECOMMIT           0x10   //NO_ACCESS, Guard page
#define MM_NOACCESS           0x18   //NO_ACCESS, Guard_page, nocache.
#define MM_UNKNOWN_PROTECTION 0x100  //bigger than 5 bits!
#define MM_LARGE_PAGES        0x111

#define PROTECT_KSTACKS       1

#define MM_KSTACK_OUTSWAPPED  0x1F   //Debug marking for kernel stacks

#define MM_PROTECTION_WRITE_MASK     4
#define MM_PROTECTION_COPY_MASK      1
#define MM_PROTECTION_OPERATION_MASK 7 // mask off guard page and nocache.
#define MM_PROTECTION_EXECUTE_MASK   2

#define MM_SECURE_DELETE_CHECK 0x55

//
// Debug flags
//

#define MM_DBG_WRITEFAULT       0x1
#define MM_DBG_PTE_UPDATE       0x2
#define MM_DBG_DUMP_WSL         0x4
#define MM_DBG_PAGEFAULT        0x8
#define MM_DBG_WS_EXPANSION     0x10
#define MM_DBG_MOD_WRITE        0x20
#define MM_DBG_CHECK_PTE        0x40
#define MM_DBG_VAD_CONFLICT     0x80
#define MM_DBG_SECTIONS         0x100
#define MM_DBG_SYS_PTES         0x400
#define MM_DBG_CLEAN_PROCESS    0x800
#define MM_DBG_COLLIDED_PAGE    0x1000
#define MM_DBG_DUMP_BOOT_PTES   0x2000
#define MM_DBG_FORK             0x4000
#define MM_DBG_DIR_BASE         0x8000
#define MM_DBG_FLUSH_SECTION    0x10000
#define MM_DBG_PRINTS_MODWRITES 0x20000
#define MM_DBG_PAGE_IN_LIST     0x40000
#define MM_DBG_CHECK_PFN_LOCK   0x80000
#define MM_DBG_PRIVATE_PAGES    0x100000
#define MM_DBG_WALK_VAD_TREE    0x200000
#define MM_DBG_SWAP_PROCESS     0x400000
#define MM_DBG_LOCK_CODE        0x800000
#define MM_DBG_STOP_ON_ACCVIO   0x1000000
#define MM_DBG_PAGE_REF_COUNT   0x2000000
#define MM_DBG_SHOW_NT_CALLS    0x10000000
#define MM_DBG_SHOW_FAULTS      0x40000000
#define MM_DBG_SESSIONS         0x80000000

//
// if the PTE.protection & MM_COPY_ON_WRITE_MASK == MM_COPY_ON_WRITE_MASK
// then the pte is copy on write.
//

#define MM_COPY_ON_WRITE_MASK  5

extern ULONG MmProtectToValue[32];
extern ULONG MmProtectToPteMask[32];
extern ULONG MmMakeProtectNotWriteCopy[32];
extern ACCESS_MASK MmMakeSectionAccess[8];
extern ACCESS_MASK MmMakeFileAccess[8];


//
// Time constants
//

extern LARGE_INTEGER MmSevenMinutes;
extern LARGE_INTEGER MmWorkingSetProtectionTime;
extern LARGE_INTEGER MmOneSecond;
extern LARGE_INTEGER MmTwentySeconds;
extern LARGE_INTEGER MmShortTime;
extern LARGE_INTEGER MmHalfSecond;
extern LARGE_INTEGER Mm30Milliseconds;
extern LARGE_INTEGER MmCriticalSectionTimeout;

//
// A month's worth
//

extern ULONG MmCritsectTimeoutSeconds;

//
// this is the csrss process !
//

extern PEPROCESS ExpDefaultErrorPortProcess;

extern SIZE_T MmExtendedCommit;

//
// The total number of pages needed for the loader to successfully hibernate.
//

extern PFN_NUMBER MmHiberPages;

//
//  The counters and reasons to retry IO to protect against verifier induced
//  failures and temporary conditions.
//

extern ULONG MiIoRetryLevel;
extern ULONG MiFaultRetries;
extern ULONG MiUserIoRetryLevel;
extern ULONG MiUserFaultRetries;
                        
#define MmIsRetryIoStatus(S) (((S) == STATUS_INSUFFICIENT_RESOURCES) || \
                              ((S) == STATUS_WORKING_SET_QUOTA) ||      \
                              ((S) == STATUS_NO_MEMORY))

//++
//
// ULONG
// MI_CONVERT_FROM_PTE_PROTECTION (
//     IN ULONG PROTECTION_MASK
//     )
//
// Routine Description:
//
//  This routine converts a PTE protection into a Protect value.
//
// Arguments:
//
//
// Return Value:
//
//     Returns the
//
//--

#define MI_CONVERT_FROM_PTE_PROTECTION(PROTECTION_MASK)      \
                                     (MmProtectToValue[PROTECTION_MASK])

#define MI_MASK_TO_PTE(PROTECTION_MASK) MmProtectToPteMask[PROTECTION_MASK]


#define MI_IS_PTE_PROTECTION_COPY_WRITE(PROTECTION_MASK)  \
   (((PROTECTION_MASK) & MM_COPY_ON_WRITE_MASK) == MM_COPY_ON_WRITE_MASK)

//++
//
// ULONG
// MI_ROUND_TO_64K (
//     IN ULONG LENGTH
//     )
//
// Routine Description:
//
//
// The ROUND_TO_64k macro takes a LENGTH in bytes and rounds it up to a multiple
// of 64K.
//
// Arguments:
//
//     LENGTH - LENGTH in bytes to round up to 64k.
//
// Return Value:
//
//     Returns the LENGTH rounded up to a multiple of 64k.
//
//--

#define MI_ROUND_TO_64K(LENGTH)  (((LENGTH) + X64K - 1) & ~(X64K - 1))

//++
//
// ULONG
// MI_ROUND_TO_SIZE (
//     IN ULONG LENGTH,
//     IN ULONG ALIGNMENT
//     )
//
// Routine Description:
//
//
// The ROUND_TO_SIZE macro takes a LENGTH in bytes and rounds it up to a
// multiple of the alignment.
//
// Arguments:
//
//     LENGTH - LENGTH in bytes to round up to.
//
//     ALIGNMENT - alignment to round to, must be a power of 2, e.g, 2**n.
//
// Return Value:
//
//     Returns the LENGTH rounded up to a multiple of the alignment.
//
//--

#define MI_ROUND_TO_SIZE(LENGTH,ALIGNMENT)     \
                    (((LENGTH) + ((ALIGNMENT) - 1)) & ~((ALIGNMENT) - 1))

//++
//
// PVOID
// MI_64K_ALIGN (
//     IN PVOID VA
//     )
//
// Routine Description:
//
//
// The MI_64K_ALIGN macro takes a virtual address and returns a 64k-aligned
// virtual address for that page.
//
// Arguments:
//
//     VA - Virtual address.
//
// Return Value:
//
//     Returns the 64k aligned virtual address.
//
//--

#define MI_64K_ALIGN(VA) ((PVOID)((ULONG_PTR)(VA) & ~((LONG)X64K - 1)))


//++
//
// PVOID
// MI_ALIGN_TO_SIZE (
//     IN PVOID VA
//     IN ULONG ALIGNMENT
//     )
//
// Routine Description:
//
//
// The MI_ALIGN_TO_SIZE macro takes a virtual address and returns a
// virtual address for that page with the specified alignment.
//
// Arguments:
//
//     VA - Virtual address.
//
//     ALIGNMENT - alignment to round to, must be a power of 2, e.g, 2**n.
//
// Return Value:
//
//     Returns the aligned virtual address.
//
//--

#define MI_ALIGN_TO_SIZE(VA,ALIGNMENT) ((PVOID)((ULONG_PTR)(VA) & ~((ULONG_PTR) ALIGNMENT - 1)))

//++
//
// LONGLONG
// MI_STARTING_OFFSET (
//     IN PSUBSECTION SUBSECT
//     IN PMMPTE PTE
//     )
//
// Routine Description:
//
//    This macro takes a pointer to a PTE within a subsection and a pointer
//    to that subsection and calculates the offset for that PTE within the
//    file.
//
// Arguments:
//
//     PTE - PTE within subsection.
//
//     SUBSECT - Subsection
//
// Return Value:
//
//     Offset for issuing I/O from.
//
//--

#define MI_STARTING_OFFSET(SUBSECT,PTE) \
           (((LONGLONG)((ULONG_PTR)((PTE) - ((SUBSECT)->SubsectionBase))) << PAGE_SHIFT) + \
             ((LONGLONG)((SUBSECT)->StartingSector) << MMSECTOR_SHIFT));


// PVOID
// MiFindEmptyAddressRangeDown (
//    IN ULONG_PTR SizeOfRange,
//    IN PVOID HighestAddressToEndAt,
//    IN ULONG_PTR Alignment
//    )
//
//
// Routine Description:
//
//    The function examines the virtual address descriptors to locate
//    an unused range of the specified size and returns the starting
//    address of the range.  This routine looks from the top down.
//
// Arguments:
//
//    SizeOfRange - Supplies the size in bytes of the range to locate.
//
//    HighestAddressToEndAt - Supplies the virtual address to begin looking
//                            at.
//
//    Alignment - Supplies the alignment for the address.  Must be
//                 a power of 2 and greater than the page_size.
//
//Return Value:
//
//    Returns the starting address of a suitable range.
//

#define MiFindEmptyAddressRangeDown(SizeOfRange,HighestAddressToEndAt,Alignment) \
               (MiFindEmptyAddressRangeDownTree(                             \
                    (SizeOfRange),                                           \
                    (HighestAddressToEndAt),                                 \
                    (Alignment),                                             \
                    (PMMADDRESS_NODE)(PsGetCurrentProcess()->VadRoot)))

// PMMVAD
// MiGetPreviousVad (
//     IN PMMVAD Vad
//     )
//
// Routine Description:
//
//     This function locates the virtual address descriptor which contains
//     the address range which logically precedes the specified virtual
//     address descriptor.
//
// Arguments:
//
//     Vad - Supplies a pointer to a virtual address descriptor.
//
// Return Value:
//
//     Returns a pointer to the virtual address descriptor containing the
//     next address range, NULL if none.
//
//

#define MiGetPreviousVad(VAD) ((PMMVAD)MiGetPreviousNode((PMMADDRESS_NODE)(VAD)))


// PMMVAD
// MiGetNextVad (
//     IN PMMVAD Vad
//     )
//
// Routine Description:
//
//     This function locates the virtual address descriptor which contains
//     the address range which logically follows the specified address range.
//
// Arguments:
//
//     VAD - Supplies a pointer to a virtual address descriptor.
//
// Return Value:
//
//     Returns a pointer to the virtual address descriptor containing the
//     next address range, NULL if none.
//

#define MiGetNextVad(VAD) ((PMMVAD)MiGetNextNode((PMMADDRESS_NODE)(VAD)))



// PMMVAD
// MiGetFirstVad (
//     Process
//     )
//
// Routine Description:
//
//     This function locates the virtual address descriptor which contains
//     the address range which logically is first within the address space.
//
// Arguments:
//
//     Process - Specifies the process in which to locate the VAD.
//
// Return Value:
//
//     Returns a pointer to the virtual address descriptor containing the
//     first address range, NULL if none.

#define MiGetFirstVad(Process) \
    ((PMMVAD)MiGetFirstNode((PMMADDRESS_NODE)(Process->VadRoot)))



// PMMVAD
// MiCheckForConflictingVad (
//     IN PVOID StartingAddress,
//     IN PVOID EndingAddress
//     )
//
// Routine Description:
//
//     The function determines if any addresses between a given starting and
//     ending address is contained within a virtual address descriptor.
//
// Arguments:
//
//     StartingAddress - Supplies the virtual address to locate a containing
//                       descriptor.
//
//     EndingAddress - Supplies the virtual address to locate a containing
//                       descriptor.
//
// Return Value:
//
//     Returns a pointer to the first conflicting virtual address descriptor
//     if one is found, otherwise a NULL value is returned.
//

#define MiCheckForConflictingVad(StartingAddress,EndingAddress)           \
    ((PMMVAD)MiCheckForConflictingNode(                                   \
                    MI_VA_TO_VPN(StartingAddress),                        \
                    MI_VA_TO_VPN(EndingAddress),                          \
                    (PMMADDRESS_NODE)(PsGetCurrentProcess()->VadRoot)))

// PMMCLONE_DESCRIPTOR
// MiGetNextClone (
//     IN PMMCLONE_DESCRIPTOR Clone
//     )
//
// Routine Description:
//
//     This function locates the virtual address descriptor which contains
//     the address range which logically follows the specified address range.
//
// Arguments:
//
//     Clone - Supplies a pointer to a virtual address descriptor.
//
// Return Value:
//
//     Returns a pointer to the virtual address descriptor containing the
//     next address range, NULL if none.
//
//

#define MiGetNextClone(CLONE) \
 ((PMMCLONE_DESCRIPTOR)MiGetNextNode((PMMADDRESS_NODE)(CLONE)))



// PMMCLONE_DESCRIPTOR
// MiGetPreviousClone (
//     IN PMMCLONE_DESCRIPTOR Clone
//     )
//
// Routine Description:
//
//     This function locates the virtual address descriptor which contains
//     the address range which logically precedes the specified virtual
//     address descriptor.
//
// Arguments:
//
//     Clone - Supplies a pointer to a virtual address descriptor.
//
// Return Value:
//
//     Returns a pointer to the virtual address descriptor containing the
//     next address range, NULL if none.


#define MiGetPreviousClone(CLONE)  \
             ((PMMCLONE_DESCRIPTOR)MiGetPreviousNode((PMMADDRESS_NODE)(CLONE)))



// PMMCLONE_DESCRIPTOR
// MiGetFirstClone (
//     )
//
// Routine Description:
//
//     This function locates the virtual address descriptor which contains
//     the address range which logically is first within the address space.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Returns a pointer to the virtual address descriptor containing the
//     first address range, NULL if none.
//


#define MiGetFirstClone() \
    ((PMMCLONE_DESCRIPTOR)MiGetFirstNode((PMMADDRESS_NODE)(PsGetCurrentProcess()->CloneRoot)))



// VOID
// MiInsertClone (
//     IN PMMCLONE_DESCRIPTOR Clone
//     )
//
// Routine Description:
//
//     This function inserts a virtual address descriptor into the tree and
//     reorders the splay tree as appropriate.
//
// Arguments:
//
//     Clone - Supplies a pointer to a virtual address descriptor
//
//
// Return Value:
//
//     None.
//

#define MiInsertClone(CLONE) \
    {                                           \
        ASSERT ((CLONE)->NumberOfPtes != 0);     \
        MiInsertNode(((PMMADDRESS_NODE)(CLONE)),(PMMADDRESS_NODE *)&(PsGetCurrentProcess()->CloneRoot)); \
    }




// VOID
// MiRemoveClone (
//     IN PMMCLONE_DESCRIPTOR Clone
//     )
//
// Routine Description:
//
//     This function removes a virtual address descriptor from the tree and
//     reorders the splay tree as appropriate.
//
// Arguments:
//
//     Clone - Supplies a pointer to a virtual address descriptor.
//
// Return Value:
//
//     None.
//

#define MiRemoveClone(CLONE) \
    MiRemoveNode((PMMADDRESS_NODE)(CLONE),(PMMADDRESS_NODE *)&(PsGetCurrentProcess()->CloneRoot));



// PMMCLONE_DESCRIPTOR
// MiLocateCloneAddress (
//     IN PVOID VirtualAddress
//     )
//
// /*++
//
// Routine Description:
//
//     The function locates the virtual address descriptor which describes
//     a given address.
//
// Arguments:
//
//     VirtualAddress - Supplies the virtual address to locate a descriptor
//                      for.
//
// Return Value:
//
//     Returns a pointer to the virtual address descriptor which contains
//     the supplied virtual address or NULL if none was located.
//

#define MiLocateCloneAddress(VA)                                            \
    (PsGetCurrentProcess()->CloneRoot ?                                     \
        ((PMMCLONE_DESCRIPTOR)MiLocateAddressInTree(((ULONG_PTR)VA),                   \
                   (PMMADDRESS_NODE *)&(PsGetCurrentProcess()->CloneRoot))) :  \
        ((PMMCLONE_DESCRIPTOR)NULL))


// PMMCLONE_DESCRIPTOR
// MiCheckForConflictingClone (
//     IN PVOID StartingAddress,
//     IN PVOID EndingAddress
//     )
//
// Routine Description:
//
//     The function determines if any addresses between a given starting and
//     ending address is contained within a virtual address descriptor.
//
// Arguments:
//
//     StartingAddress - Supplies the virtual address to locate a containing
//                       descriptor.
//
//     EndingAddress - Supplies the virtual address to locate a containing
//                       descriptor.
//
// Return Value:
//
//     Returns a pointer to the first conflicting virtual address descriptor
//     if one is found, otherwise a NULL value is returned.
//

#define MiCheckForConflictingClone(START,END)                             \
    ((PMMCLONE_DESCRIPTOR)(MiCheckForConflictingNode(START,END,           \
                   (PMMADDRESS_NODE)(PsGetCurrentProcess()->CloneRoot))))


//
// MiGetVirtualPageNumber returns the virtual page number
// for a given address.
//

//#define MiGetVirtualPageNumber(va) ((ULONG_PTR)(va) >> PAGE_SHIFT)

#define MI_VA_TO_PAGE(va) ((ULONG_PTR)(va) >> PAGE_SHIFT)

#define MI_VA_TO_VPN(va)  ((ULONG_PTR)(va) >> PAGE_SHIFT)

#define MI_VPN_TO_VA(vpn)  (PVOID)((vpn) << PAGE_SHIFT)

#define MI_VPN_TO_VA_ENDING(vpn)  (PVOID)(((vpn) << PAGE_SHIFT) | (PAGE_SIZE - 1))

#define MiGetByteOffset(va) ((ULONG_PTR)(va) & (PAGE_SIZE - 1))

#define MI_PFN_ELEMENT(index) (&MmPfnDatabase[index])

//
// Make a write-copy PTE, only writable.
//

#define MI_MAKE_PROTECT_NOT_WRITE_COPY(PROTECT) \
            (MmMakeProtectNotWriteCopy[PROTECT])

//
// Define macros to lock and unlock the PFN database.
//

#if defined(_ALPHA_) || defined(_X86_)

#define MiLockPfnDatabase(OldIrql) \
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock)

#define MiUnlockPfnDatabase(OldIrql) \
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql)

#define MiTryToLockPfnDatabase(OldIrql) \
    KeTryToAcquireQueuedSpinLock(LockQueuePfnLock, &OldIrql)

#define MiReleasePfnLock() \
    KiReleaseQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[LockQueuePfnLock])

#define MiLockSystemSpace(OldIrql) \
    OldIrql = KeAcquireQueuedSpinLock(LockQueueSystemSpaceLock)

#define MiUnlockSystemSpace(OldIrql) \
    KeReleaseQueuedSpinLock(LockQueueSystemSpaceLock, OldIrql)

#define MiLockSystemSpaceAtDpcLevel() \
    KiAcquireQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[LockQueueSystemSpaceLock])

#define MiUnlockSystemSpaceFromDpcLevel() \
    KiReleaseQueuedSpinLock(&KeGetCurrentPrcb()->LockQueue[LockQueueSystemSpaceLock])

#else

#define MiLockPfnDatabase(OldIrql) \
    ExAcquireSpinLock(&MmPfnLock, &OldIrql)

#define MiUnlockPfnDatabase(OldIrql) \
    ExReleaseSpinLock(&MmPfnLock, OldIrql)

#define MiTryToLockPfnDatabase(OldIrql) \
    KeTryToAcquireSpinLock(&MmPfnLock, &OldIrql)

#define MiReleasePfnLock() \
    KiReleaseSpinLock(&MmPfnLock)

#define MiLockSystemSpace(OldIrql) \
    ExAcquireSpinLock(&MmSystemSpaceLock, &OldIrql)

#define MiUnlockSystemSpace(OldIrql) \
    ExReleaseSpinLock(&MmSystemSpaceLock, OldIrql)

#define MiLockSystemSpaceAtDpcLevel() \
    ExAcquireSpinLockAtDpcLevel(&MmSystemSpaceLock)

#define MiUnlockSystemSpaceFromDpcLevel() \
    ExReleaseSpinLockFromDpcLevel(&MmSystemSpaceLock)

#endif

#if PFN_CONSISTENCY

#define CONSISTENCY_LOCK_PFN(OLDIRQL)  LOCK_PFN(OLDIRQL)
#define CONSISTENCY_UNLOCK_PFN(OLDIRQL)  UNLOCK_PFN(OLDIRQL)

#define CONSISTENCY_LOCK_PFN2(OLDIRQL)  LOCK_PFN2(OLDIRQL)
#define CONSISTENCY_UNLOCK_PFN2(OLDIRQL)  UNLOCK_PFN2(OLDIRQL)

#define PFN_CONSISTENCY_SET \
    (MiPfnLockOwner = PsGetCurrentThread(), MiMapInPfnDatabase())
#define PFN_CONSISTENCY_UNSET \
    (MiUnMapPfnDatabase(), MiPfnLockOwner = (PETHREAD)0)

VOID
MiMapInPfnDatabase (
    VOID
    );

VOID
MiUnMapPfnDatabase (
    VOID
    );

VOID
MiSetModified (
    IN PMMPFN Pfn1,
    IN ULONG Dirty
    );

extern PMMPTE MiPfnStartPte;
extern PFN_NUMBER MiPfnPtes;
extern BOOLEAN MiPfnProtectionEnabled;
extern PETHREAD MiPfnLockOwner;

#define PFN_LOCK_OWNED_BY_ME()      (MiPfnLockOwner == PsGetCurrentThread())


#else // PFN_CONSISTENCY

#define CONSISTENCY_LOCK_PFN(OLDIRQL)
#define CONSISTENCY_UNLOCK_PFN(OLDIRQL)

#define CONSISTENCY_LOCK_PFN2(OLDIRQL)
#define CONSISTENCY_UNLOCK_PFN2(OLDIRQL)

#define PFN_CONSISTENCY_SET
#define PFN_CONSISTENCY_UNSET

#endif // PFN_CONSISTENCY

#define LOCK_PFN(OLDIRQL) ASSERT (KeGetCurrentIrql() <= APC_LEVEL); \
                          MiLockPfnDatabase(OLDIRQL);               \
                          PFN_CONSISTENCY_SET;

#define LOCK_PFN_WITH_TRY(OLDIRQL)                                   \
    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);                        \
    do {                                                             \
    } while (MiTryToLockPfnDatabase(OLDIRQL) == FALSE);              \
    PFN_CONSISTENCY_SET;

#define UNLOCK_PFN(OLDIRQL)                                        \
    PFN_CONSISTENCY_UNSET;                                         \
    MiUnlockPfnDatabase(OLDIRQL);                                  \
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

#define LOCK_PFN2(OLDIRQL) ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL); \
                          MiLockPfnDatabase(OLDIRQL);               \
                          PFN_CONSISTENCY_SET;

#define UNLOCK_PFN2(OLDIRQL)                                       \
    PFN_CONSISTENCY_UNSET;                                         \
    MiUnlockPfnDatabase(OLDIRQL);                                  \
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

#define UNLOCK_PFN_AND_THEN_WAIT(OLDIRQL)                          \
                {                                                  \
                    KIRQL XXX;                                     \
                    ASSERT (KeGetCurrentIrql() == 2);              \
                    ASSERT (OLDIRQL <= APC_LEVEL);                 \
                    KiLockDispatcherDatabase (&XXX);               \
                    PFN_CONSISTENCY_UNSET;                         \
                    MiReleasePfnLock();                            \
                    (KeGetCurrentThread())->WaitIrql = OLDIRQL;    \
                    (KeGetCurrentThread())->WaitNext = TRUE;       \
                }

#define LOCK_AWE(PROCESS,OldIrql) \
    ExAcquireSpinLock(&((PROCESS)->AweLock), &OldIrql)

#define UNLOCK_AWE(PROCESS,OldIrql) \
    ExReleaseSpinLock(&((PROCESS)->AweLock), OldIrql)

extern KMUTANT MmSystemLoadLock;

#if DBG
#define SYSLOAD_LOCK_OWNED_BY_ME()      ((PETHREAD)MmSystemLoadLock.OwnerThread == PsGetCurrentThread())
#else
#define SYSLOAD_LOCK_OWNED_BY_ME()
#endif

#if DBG
#define MM_PFN_LOCK_ASSERT() \
    if (MmDebug & 0x80000) { \
        ASSERT (KeGetCurrentIrql() == 2);   \
    }

extern PETHREAD MiExpansionLockOwner;

#define MM_SET_EXPANSION_OWNER()  ASSERT (MiExpansionLockOwner == NULL); \
                                  MiExpansionLockOwner = PsGetCurrentThread();

#define MM_CLEAR_EXPANSION_OWNER()  ASSERT (MiExpansionLockOwner == PsGetCurrentThread()); \
                                    MiExpansionLockOwner = NULL;

#else
#define MM_PFN_LOCK_ASSERT()
#define MM_SET_EXPANSION_OWNER()
#define MM_CLEAR_EXPANSION_OWNER()
#endif //DBG


#define LOCK_EXPANSION(OLDIRQL)     ASSERT (KeGetCurrentIrql() <= APC_LEVEL); \
                                ExAcquireSpinLock (&MmExpansionLock, &OLDIRQL);\
                                MM_SET_EXPANSION_OWNER ();

#define UNLOCK_EXPANSION(OLDIRQL)    MM_CLEAR_EXPANSION_OWNER (); \
                                ExReleaseSpinLock (&MmExpansionLock, OLDIRQL); \
                                ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

#define UNLOCK_EXPANSION_AND_THEN_WAIT(OLDIRQL)                    \
                {                                                  \
                    KIRQL XXX;                                     \
                    ASSERT (KeGetCurrentIrql() == 2);              \
                    ASSERT (OLDIRQL <= APC_LEVEL);                 \
                    KiLockDispatcherDatabase (&XXX);               \
                    MM_CLEAR_EXPANSION_OWNER ();                   \
                    KiReleaseSpinLock (&MmExpansionLock);          \
                    (KeGetCurrentThread())->WaitIrql = OLDIRQL;    \
                    (KeGetCurrentThread())->WaitNext = TRUE;       \
                }

#if defined(_ALPHA_) && !defined(_AXP64_)
#define LOCK_EXPANSION_IF_ALPHA(OLDIRQL)            \
 ExAcquireSpinLock (&MmExpansionLock, &OLDIRQL);    \
 MM_SET_EXPANSION_OWNER ();
#else
#define LOCK_EXPANSION_IF_ALPHA(OLDIRQL)
#endif //ALPHA


#if defined(_ALPHA_) && !defined(_AXP64_)
#define UNLOCK_EXPANSION_IF_ALPHA(OLDIRQL)            \
 MM_CLEAR_EXPANSION_OWNER ();                         \
 ExReleaseSpinLock ( &MmExpansionLock, OLDIRQL )
#else
#define UNLOCK_EXPANSION_IF_ALPHA(OLDIRQL)
#endif //ALPHA


extern LIST_ENTRY MmLockConflictList;
extern PETHREAD MmSystemLockOwner;

#define LOCK_SYSTEM_WS(OLDIRQL)                                 \
            ASSERT (KeGetCurrentIrql() <= APC_LEVEL);           \
            KeRaiseIrql(APC_LEVEL,&OLDIRQL);                    \
            ExAcquireResourceExclusive(&MmSystemWsLock,TRUE);   \
            ASSERT (MmSystemLockOwner == NULL);                 \
            MmSystemLockOwner = PsGetCurrentThread();

#define UNLOCK_SYSTEM_WS(OLDIRQL)                               \
            ASSERT (MmSystemLockOwner == PsGetCurrentThread()); \
            MmSystemLockOwner = NULL;                           \
            ExReleaseResource (&MmSystemWsLock);                \
            KeLowerIrql (OLDIRQL);                              \
            ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

#define UNLOCK_SYSTEM_WS_NO_IRQL()                              \
            ASSERT (MmSystemLockOwner == PsGetCurrentThread()); \
            MmSystemLockOwner = NULL;                           \
            ExReleaseResource (&MmSystemWsLock);

#define MM_SYSTEM_WS_LOCK_ASSERT()                              \
        ASSERT (PsGetCurrentThread() == MmSystemLockOwner);

#define LOCK_HYPERSPACE(OLDIRQL)                            \
    ExAcquireSpinLock ( &(PsGetCurrentProcess())->HyperSpaceLock, OLDIRQL );


#define UNLOCK_HYPERSPACE(OLDIRQL)                         \
    ExReleaseSpinLock ( &(PsGetCurrentProcess())->HyperSpaceLock, OLDIRQL );

#if defined (_AXP64_)
#define MI_WS_OWNER(PROCESS) ((PROCESS)->WorkingSetLock.Owner == KeGetCurrentThread())
#define MI_NOT_WS_OWNER(PROCESS) (!MI_WS_OWNER(PROCESS))
#else
#define MI_WS_OWNER(PROCESS)        1
#define MI_NOT_WS_OWNER(PROCESS)    1
#endif

#define MI_MUTEX_ACQUIRED_UNSAFE    0x88

#define MI_IS_WS_UNSAFE(PROCESS) ((PROCESS)->WorkingSetLock.OldIrql == MI_MUTEX_ACQUIRED_UNSAFE)

#define LOCK_WS(PROCESS)                                            \
            ASSERT (MI_NOT_WS_OWNER(PROCESS));                      \
            ExAcquireFastMutex( &((PROCESS)->WorkingSetLock));      \
            ASSERT (!MI_IS_WS_UNSAFE(PROCESS));

#define LOCK_WS_UNSAFE(PROCESS)                                     \
            ASSERT (MI_NOT_WS_OWNER(PROCESS));                      \
            ASSERT (KeGetCurrentIrql() == APC_LEVEL);               \
            ExAcquireFastMutexUnsafe( &((PROCESS)->WorkingSetLock));\
            (PROCESS)->WorkingSetLock.OldIrql = MI_MUTEX_ACQUIRED_UNSAFE;

#define MI_MUST_BE_UNSAFE(PROCESS)                                  \
            ASSERT (KeGetCurrentIrql() == APC_LEVEL);               \
            ASSERT (MI_WS_OWNER(PROCESS));                          \
            ASSERT (MI_IS_WS_UNSAFE(PROCESS));

#define MI_MUST_BE_SAFE(PROCESS)                                    \
            ASSERT (MI_WS_OWNER(PROCESS));                          \
            ASSERT (!MI_IS_WS_UNSAFE(PROCESS));
#if 0

#define MI_MUST_BE_UNSAFE(PROCESS)                                  \
            if (KeGetCurrentIrql() != APC_LEVEL) {                  \
                KeBugCheckEx(MEMORY_MANAGEMENT, 0x32, (ULONG_PTR)PROCESS, KeGetCurrentIrql(), 0); \
            }                                                       \
            if (!MI_WS_OWNER(PROCESS)) {                            \
                KeBugCheckEx(MEMORY_MANAGEMENT, 0x33, (ULONG_PTR)PROCESS, 0, 0); \
            }                                                       \
            if (!MI_IS_WS_UNSAFE(PROCESS)) {                        \
                KeBugCheckEx(MEMORY_MANAGEMENT, 0x34, (ULONG_PTR)PROCESS, 0, 0); \
            }

#define MI_MUST_BE_SAFE(PROCESS)                                    \
            if (!MI_WS_OWNER(PROCESS)) {                            \
                KeBugCheckEx(MEMORY_MANAGEMENT, 0x42, (ULONG_PTR)PROCESS, 0, 0); \
            }                                                       \
            if (MI_IS_WS_UNSAFE(PROCESS)) {                         \
                KeBugCheckEx(MEMORY_MANAGEMENT, 0x43, (ULONG_PTR)PROCESS, 0, 0); \
            }
#endif


#define UNLOCK_WS(PROCESS)                                          \
            MI_MUST_BE_SAFE(PROCESS);                               \
            ExReleaseFastMutex(&((PROCESS)->WorkingSetLock));

#define UNLOCK_WS_UNSAFE(PROCESS)                                   \
            MI_MUST_BE_UNSAFE(PROCESS);                             \
            ExReleaseFastMutexUnsafe(&((PROCESS)->WorkingSetLock)); \
            ASSERT (KeGetCurrentIrql() == APC_LEVEL);

#define LOCK_ADDRESS_SPACE(PROCESS)                                  \
            ExAcquireFastMutex( &((PROCESS)->AddressCreationLock))

#define LOCK_WS_AND_ADDRESS_SPACE(PROCESS)                          \
        LOCK_ADDRESS_SPACE(PROCESS);                                \
        LOCK_WS_UNSAFE(PROCESS);

#define UNLOCK_WS_AND_ADDRESS_SPACE(PROCESS)                        \
        UNLOCK_WS_UNSAFE(PROCESS);                                  \
        UNLOCK_ADDRESS_SPACE(PROCESS);

#define UNLOCK_ADDRESS_SPACE(PROCESS)                            \
            ExReleaseFastMutex( &((PROCESS)->AddressCreationLock))

//
// The working set lock may have been acquired safely or unsafely.
// Release and reacquire it regardless.
//

#define UNLOCK_WS_REGARDLESS(PROCESS, WSHELDSAFE)                   \
            ASSERT (MI_WS_OWNER (PROCESS));                         \
            if (MI_IS_WS_UNSAFE (PROCESS)) {                        \
                UNLOCK_WS_UNSAFE (PROCESS);                         \
                WSHELDSAFE = FALSE;                                 \
            }                                                       \
            else {                                                  \
                UNLOCK_WS (PROCESS);                                \
                WSHELDSAFE = TRUE;                                  \
            }

#define LOCK_WS_REGARDLESS(PROCESS, WSHELDSAFE)                     \
            if (WSHELDSAFE == TRUE) {                               \
                LOCK_WS (PROCESS);                                  \
            }                                                       \
            else {                                                  \
                LOCK_WS_UNSAFE (PROCESS);                           \
            }

#define ZERO_LARGE(LargeInteger)                \
        (LargeInteger).LowPart = 0;             \
        (LargeInteger).HighPart = 0;

//++
//
// ULONG
// MI_CHECK_BIT (
//     IN PULONG ARRAY
//     IN ULONG BIT
//     )
//
// Routine Description:
//
//     The MI_CHECK_BIT macro checks to see if the specified bit is
//     set within the specified array.
//
// Arguments:
//
//     ARRAY - First element of the array to check.
//
//     BIT - bit number (first bit is 0) to check.
//
// Return Value:
//
//     Returns the value of the bit (0 or 1).
//
//--

#define MI_CHECK_BIT(ARRAY,BIT)  \
        (((ULONG)ARRAY[(BIT) / (sizeof(ULONG)*8)] >> ((BIT) & 0x1F)) & 1)


//++
//
// VOID
// MI_SET_BIT (
//     IN PULONG ARRAY
//     IN ULONG BIT
//     )
//
// Routine Description:
//
//     The MI_SET_BIT macro sets the specified bit within the
//     specified array.
//
// Arguments:
//
//     ARRAY - First element of the array to set.
//
//     BIT - bit number.
//
// Return Value:
//
//     None.
//
//--

#define MI_SET_BIT(ARRAY,BIT)  \
        (ULONG)ARRAY[(BIT) / (sizeof(ULONG)*8)] |= (1 << ((BIT) & 0x1F))


//++
//
// VOID
// MI_CLEAR_BIT (
//     IN PULONG ARRAY
//     IN ULONG BIT
//     )
//
// Routine Description:
//
//     The MI_CLEAR_BIT macro sets the specified bit within the
//     specified array.
//
// Arguments:
//
//     ARRAY - First element of the array to clear.
//
//     BIT - bit number.
//
// Return Value:
//
//     None.
//
//--

#define MI_CLEAR_BIT(ARRAY,BIT)  \
        (ULONG)ARRAY[(BIT) / (sizeof(ULONG)*8)] &= ~(1 << ((BIT) & 0x1F))


#define MI_MAGIC_AWE_PTEFRAME   0xffffedcb

#define MI_PFN_IS_AWE(Pfn1)                                     \
        ((Pfn1->u2.ShareCount <= 3) &&                          \
         (Pfn1->u3.e1.PageLocation == ActiveAndValid) &&        \
         (Pfn1->u3.e1.VerifierAllocation == 0) &&               \
         (Pfn1->u3.e1.LargeSessionAllocation == 0) &&           \
         (Pfn1->u3.e1.StartOfAllocation == 1) &&                \
         (Pfn1->u3.e1.EndOfAllocation == 1) &&                  \
         (Pfn1->PteFrame == MI_MAGIC_AWE_PTEFRAME))


typedef ULONG WSLE_NUMBER, *PWSLE_NUMBER;

//
// PFN database element.
//

//
// Define pseudo fields for start and end of allocation.
//

#define StartOfAllocation ReadInProgress

#define EndOfAllocation WriteInProgress

#define LargeSessionAllocation PrototypePte

//
// The PteFrame field size determines the largest physical page that
// can be supported on the system.  On a 4k page sized machine, 20 bits
// limits it to 4GBs.
//

typedef struct _MMPFNENTRY {
    ULONG Modified : 1;
    ULONG ReadInProgress : 1;
    ULONG WriteInProgress : 1;
    ULONG PrototypePte: 1;
    ULONG PageColor : 3;
    ULONG ParityError : 1;
    ULONG PageLocation : 3;
    ULONG InPageError : 1;
    ULONG VerifierAllocation : 1;
    ULONG RemovalRequested : 1;
    ULONG Reserved : 1;
    ULONG LockCharged : 1;
    ULONG DontUse : 16; //overlays USHORT for reference count field.
} MMPFNENTRY;

#if defined (_X86PAE_)
#pragma pack(1)
#endif

typedef struct _MMPFN {
    union {
        PFN_NUMBER Flink;
        WSLE_NUMBER WsIndex;
        PKEVENT Event;
        NTSTATUS ReadStatus;
        struct _MMPFN *NextStackPfn;
    } u1;
    PMMPTE PteAddress;
    union {
        PFN_NUMBER Blink;
        ULONG ShareCount;
        ULONG SecondaryColorFlink;
    } u2;
    union {
        MMPFNENTRY e1;
        struct {
            USHORT ShortFlags;
            USHORT ReferenceCount;
        } e2;
    } u3;
#if defined (_WIN64)
    ULONG UsedPageTableEntries;
#endif
    MMPTE OriginalPte;
    PFN_NUMBER PteFrame;
} MMPFN, *PMMPFN;

#if defined (_X86PAE_)
#pragma pack()
#endif

#if defined (_WIN64)

//
// Note there are some places where these portable macros are not currently
// used because we are not in the correct address space required.
//

#define MI_CAPTURE_USED_PAGETABLE_ENTRIES(PFN) \
        ASSERT ((PFN)->UsedPageTableEntries <= PTE_PER_PAGE); \
        (PFN)->OriginalPte.u.Soft.UsedPageTableEntries = (PFN)->UsedPageTableEntries;

#define MI_RETRIEVE_USED_PAGETABLE_ENTRIES_FROM_PTE(RBL, PTE) \
        ASSERT ((PTE)->u.Soft.UsedPageTableEntries <= PTE_PER_PAGE); \
        (RBL)->UsedPageTableEntries = (ULONG)(((PMMPTE)(PTE))->u.Soft.UsedPageTableEntries);

#define MI_ZERO_USED_PAGETABLE_ENTRIES_IN_INPAGE_SUPPORT(INPAGE_SUPPORT) \
            (INPAGE_SUPPORT)->UsedPageTableEntries = 0;

#define MI_ZERO_USED_PAGETABLE_ENTRIES_IN_PFN(PFN) (PFN)->UsedPageTableEntries = 0;

#define MI_INSERT_USED_PAGETABLE_ENTRIES_IN_PFN(PFN, INPAGE_SUPPORT) \
        ASSERT ((INPAGE_SUPPORT)->UsedPageTableEntries <= PTE_PER_PAGE); \
        (PFN)->UsedPageTableEntries = (INPAGE_SUPPORT)->UsedPageTableEntries;

#define MI_ZERO_USED_PAGETABLE_ENTRIES(PFN) \
        (PFN)->UsedPageTableEntries = 0;

#define MI_GET_USED_PTES_HANDLE(VA) \
        ((PVOID)MI_PFN_ELEMENT((PFN_NUMBER)MiGetPdeAddress(VA)->u.Hard.PageFrameNumber))

#define MI_GET_USED_PTES_FROM_HANDLE(PFN) \
        ((ULONG)(((PMMPFN)(PFN))->UsedPageTableEntries))

#define MI_INCREMENT_USED_PTES_BY_HANDLE(PFN) \
        (((PMMPFN)(PFN))->UsedPageTableEntries += 1); \
        ASSERT (((PMMPFN)(PFN))->UsedPageTableEntries <= PTE_PER_PAGE)

#define MI_DECREMENT_USED_PTES_BY_HANDLE(PFN) \
        (((PMMPFN)(PFN))->UsedPageTableEntries -= 1); \
        ASSERT (((PMMPFN)(PFN))->UsedPageTableEntries < PTE_PER_PAGE)

#else

#define MI_CAPTURE_USED_PAGETABLE_ENTRIES(PFN)
#define MI_RETRIEVE_USED_PAGETABLE_ENTRIES_FROM_PTE(RBL, PTE)
#define MI_ZERO_USED_PAGETABLE_ENTRIES_IN_INPAGE_SUPPORT(INPAGE_SUPPORT)
#define MI_ZERO_USED_PAGETABLE_ENTRIES_IN_PFN(PFN)

#define MI_INSERT_USED_PAGETABLE_ENTRIES_IN_PFN(PFN, INPAGE_SUPPORT)

#define MI_GET_USED_PTES_HANDLE(VA) ((PVOID)&MmWorkingSetList->UsedPageTableEntries[MiGetPpePdeOffset(VA)])

#define MI_GET_USED_PTES_FROM_HANDLE(PDSHORT) ((ULONG)(*(PUSHORT)(PDSHORT)))

#define MI_INCREMENT_USED_PTES_BY_HANDLE(PDSHORT) ((*(PUSHORT)(PDSHORT)) += 1); \
    ASSERT (((*(PUSHORT)(PDSHORT)) <= PTE_PER_PAGE))

#define MI_DECREMENT_USED_PTES_BY_HANDLE(PDSHORT) ((*(PUSHORT)(PDSHORT)) -= 1); \
    ASSERT (((*(PUSHORT)(PDSHORT)) < PTE_PER_PAGE))

#endif

extern LOGICAL MmDynamicPfn;

extern FAST_MUTEX MmDynamicMemoryMutex;

extern PFN_NUMBER MmSystemLockPagesCount;

#if DBG

#define MI_LOCK_ID_COUNTER_MAX 64
ULONG MiLockIds[MI_LOCK_ID_COUNTER_MAX];

#define MI_MARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId)      \
         ASSERT (Pfn->u3.e1.LockCharged == 0);          \
         ASSERT (CallerId < MI_LOCK_ID_COUNTER_MAX);    \
         MiLockIds[CallerId] += 1;                      \
         Pfn->u3.e1.LockCharged = 1;

#define MI_UNMARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId)    \
         ASSERT (Pfn->u3.e1.LockCharged == 1);          \
         ASSERT (CallerId < MI_LOCK_ID_COUNTER_MAX);    \
         MiLockIds[CallerId] += 1;                      \
         Pfn->u3.e1.LockCharged = 0;

#else
#define MI_MARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId)
#define MI_UNMARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId)
#endif

//++
//
// VOID
// MI_ADD_LOCKED_PAGE_CHARGE (
//     IN PMMPFN Pfn
//     )
//
// Routine Description:
//
//     Charge the systemwide count of locked pages if this is the initial
//     lock for this page (multiple concurrent locks are only charged once).
//
// Arguments:
//
//     Pfn - the PFN index to operate on.
//
// Return Value:
//
//     None.
//
//--
//
#define MI_ADD_LOCKED_PAGE_CHARGE(Pfn, CallerId)                \
    ASSERT (Pfn->u3.e2.ReferenceCount != 0);                    \
    if (Pfn->u3.e2.ReferenceCount == 1) {                       \
        if (Pfn->u2.ShareCount != 0) {                          \
            ASSERT (Pfn->u3.e1.PageLocation == ActiveAndValid); \
            MI_MARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId);         \
            MmSystemLockPagesCount += 1;                        \
        }                                                       \
        else {                                                  \
            ASSERT (Pfn->u3.e1.LockCharged == 1);               \
        }                                                       \
    }

#define MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE(Pfn, CallerId) \
    ASSERT (Pfn->u3.e1.PageLocation != ActiveAndValid);    \
    ASSERT (Pfn->u2.ShareCount == 0);                      \
    if (Pfn->u3.e2.ReferenceCount == 0) {                  \
        MI_MARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId);        \
        MmSystemLockPagesCount += 1;                       \
    }

#define MI_ADD_LOCKED_PAGE_CHARGE_FOR_TRANSITION_PAGE(Pfn, CallerId) \
    ASSERT (Pfn->u3.e1.PageLocation == ActiveAndValid);    \
    ASSERT (Pfn->u2.ShareCount == 0);                      \
    ASSERT (Pfn->u3.e2.ReferenceCount != 0);               \
    if (Pfn->u3.e2.ReferenceCount == 1) {                  \
        MI_MARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId);        \
        MmSystemLockPagesCount += 1;                       \
    }

//++
//
// VOID
// MI_REMOVE_LOCKED_PAGE_CHARGE (
//     IN PMMPFN Pfn
//     )
//
// Routine Description:
//
//     Remove the charge from the systemwide count of locked pages if this
//     is the last lock for this page (multiple concurrent locks are only
//     charged once).
//
// Arguments:
//
//     Pfn - the PFN index to operate on.
//
// Return Value:
//
//     None.
//
//--
//
#define MI_REMOVE_LOCKED_PAGE_CHARGE(Pfn, CallerId)                         \
        ASSERT (Pfn->u3.e2.ReferenceCount != 0);                            \
        if (Pfn->u3.e2.ReferenceCount == 2) {                               \
            if (Pfn->u2.ShareCount >= 1) {                                  \
                ASSERT (Pfn->u3.e1.PageLocation == ActiveAndValid);         \
                MI_UNMARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId);               \
                MmSystemLockPagesCount -= 1;                                \
            }                                                               \
            else {                                                          \
                /*                                                          \
                 * There are multiple referencers to this page and the      \
                 * page is no longer valid in any process address space.    \
                 * The systemwide lock count can only be decremented        \
                 * by the last dereference.                                 \
                 */                                                         \
                NOTHING;                                                    \
            }                                                               \
        }                                                                   \
        else if (Pfn->u3.e2.ReferenceCount == 1) {                          \
            /*                                                              \
             * This page has already been deleted from all process address  \
             * spaces.  It is sitting in limbo (not on any list) awaiting   \
             * this last dereference.                                       \
             */                                                             \
            ASSERT (Pfn->u3.e1.PageLocation != ActiveAndValid);             \
            ASSERT (Pfn->u2.ShareCount == 0);                               \
            MI_UNMARK_PFN_AS_LOCK_CHARGED(Pfn, CallerId);                   \
            MmSystemLockPagesCount -= 1;                                    \
        }                                                                   \
        else {                                                              \
            /*                                                              \
             * There are still multiple referencers to this page (it may    \
             * or may not be resident in any process address space).        \
             * Since the systemwide lock count can only be decremented      \
             * by the last dereference (and this is not it), no action      \
             * is taken here.                                               \
             */                                                             \
            NOTHING;                                                        \
        }


//++
//
// VOID
// MI_ZERO_WSINDEX (
//     IN PMMPFN Pfn
//     )
//
// Routine Description:
//
//     Zero the Working Set Index field of the argument PFN entry.
//     There is a subtlety here on systems where the WsIndex ULONG is
//     overlaid with an Event pointer and sizeof(ULONG) != sizeof(PKEVENT).
//     Note this will need to be updated if we ever decide to allocate bodies of
//     thread objects on 4GB boundaries.
//
// Arguments:
//
//     Pfn - the PFN index to operate on.
//
// Return Value:
//
//     None.
//
//--
//
#define MI_ZERO_WSINDEX(Pfn) \
    Pfn->u1.Event = NULL;

typedef enum _MMSHARE_TYPE {
    Normal,
    ShareCountOnly,
    AndValid
} MMSHARE_TYPE;

typedef struct _MMWSLE_HASH {
    ULONG_PTR Key;
    WSLE_NUMBER Index;
} MMWSLE_HASH, *PMMWSLE_HASH;

//++
//
// WSLE_NUMBER
// MI_WSLE_HASH (
//     IN ULONG_PTR VirtualAddress,
//     IN PMMWSL WorkingSetList
//     )
//
// Routine Description:
//
//     Hash the address
//
// Arguments:
//
//     VirtualAddress - the address to hash.
//
//     WorkingSetList - the working set to hash the address into.
//
// Return Value:
//
//     The hash key.
//
//--
//
#define MI_WSLE_HASH(Address, Wsl) \
    ((WSLE_NUMBER)(((ULONG_PTR)PAGE_ALIGN(Address) >> (PAGE_SHIFT - 2)) % \
        ((Wsl)->HashTableSize - 1)))

//
// Working Set List Entry.
//

typedef struct _MMWSLENTRY {
    ULONG_PTR Valid : 1;
    ULONG_PTR LockedInWs : 1;
    ULONG_PTR LockedInMemory : 1;
    ULONG_PTR Protection : 5;
    ULONG_PTR SameProtectAsProto : 1;
    ULONG_PTR Direct : 1;
    ULONG_PTR Age : 2;
#if MM_VIRTUAL_PAGE_FILLER
    ULONG_PTR Filler : MM_VIRTUAL_PAGE_FILLER;
#endif
    ULONG_PTR VirtualPageNumber : MM_VIRTUAL_PAGE_SIZE;
} MMWSLENTRY;

typedef struct _MMWSLE {
    union {
        PVOID VirtualAddress;
        ULONG_PTR Long;
        MMWSLENTRY e1;
    } u1;
} MMWSLE;

#define MI_GET_PROTECTION_FROM_WSLE(Wsl) ((Wsl)->u1.e1.Protection)

typedef MMWSLE *PMMWSLE;

//
// Working Set List.  Must be quadword sized.
//

typedef struct _MMWSL {
    SIZE_T Quota;
    WSLE_NUMBER FirstFree;
    WSLE_NUMBER FirstDynamic;
    WSLE_NUMBER LastEntry;
    WSLE_NUMBER NextSlot;               // The next slot to trim
    PMMWSLE Wsle;
    SIZE_T NumberOfCommittedPageTables;
    WSLE_NUMBER LastInitializedWsle;
    WSLE_NUMBER NonDirectCount;
    PMMWSLE_HASH HashTable;
    ULONG HashTableSize;
    PKEVENT WaitingForImageMapping;
    PVOID HashTableStart;
    PVOID HighestPermittedHashAddress;

    //MUST BE QUADWORD ALIGNED AT THIS POINT!

#if !defined (_WIN64)
    PVOID Align1;   // to quadword align
    PVOID Align2;   // to quadword align
    //
    // This must be at the end.
    // not used in system cache or session working set lists.
    //
    USHORT UsedPageTableEntries[MM_USER_PAGE_TABLE_PAGES];
#endif

#ifndef _WIN64
    ULONG CommittedPageTables[MM_USER_PAGE_TABLE_PAGES/(sizeof(ULONG)*8)];
#endif

} MMWSL, *PMMWSL;

#ifdef _MI_USE_CLAIMS_

//
// The claim estimate of unused pages in a working set is limited
// to grow by this amount per estimation period.
//

#define MI_CLAIM_INCR 30

#endif

//
// The maximum number of different ages a page can be.
//

#define MI_USE_AGE_COUNT 4
#define MI_USE_AGE_MAX (MI_USE_AGE_COUNT - 1)

//
// If more than this "percentage" of the working set is estimated to
// be used then allow it to grow freely.
//

#define MI_REPLACEMENT_FREE_GROWTH_SHIFT 5

//
// If more than this "percentage" of the working set has been claimed
// then force replacement in low memory.
//

#define MI_REPLACEMENT_CLAIM_THRESHOLD_SHIFT 3

//
// If more than this "percentage" of the working set is estimated to
// be available then force replacement in low memory.
//

#define MI_REPLACEMENT_EAVAIL_THRESHOLD_SHIFT 3

//
// If while doing replacement a page is found of this age or older then
// replace it.  Otherwise the oldest is selected.
//

#define MI_IMMEDIATE_REPLACEMENT_AGE 2

//
// When trimming, use these ages for different passes.
//

#define MI_MAX_TRIM_PASSES 4
#define MI_PASS0_TRIM_AGE 2
#define MI_PASS1_TRIM_AGE 1
#define MI_PASS2_TRIM_AGE 1
#define MI_PASS3_TRIM_AGE 1
#define MI_PASS4_TRIM_AGE 0

//
// If not a forced trim, trim pages older than this age.
//

#define MI_TRIM_AGE_THRESHOLD 2

//
// This "percentage" of a claim is up for grabs in a foreground process.
//

#define MI_FOREGROUND_CLAIM_AVAILABLE_SHIFT 3

//
// This "percentage" of a claim is up for grabs in a background process.
//

#define MI_BACKGROUND_CLAIM_AVAILABLE_SHIFT 1

//++
//
// DWORD
// MI_CALC_NEXT_VALID_ESTIMATION_SLOT (
//     DWORD Previous,
//     DWORD Minimum,
//     DWORD Maximum,
//     MI_NEXT_ESTIMATION_SLOT_CONST NextEstimationSlotConst,
//     PMMWSLE Wsle
//     )
//
// Routine Description:
//
//      We iterate through the working set array in a non-sequential
//      manner so that the sample is independent of any aging or trimming.
//
//      This algorithm walks through the working set with a stride of
//      2^MiEstimationShift elements.
//
// Arguments:
//
//      Previous - Last slot used
//
//      Minimum - Minimum acceptable slot (ie. the first dynamic one)
//
//      Maximum - max slot number + 1
//
//      NextEstimationSlotConst - for this algorithm it contains the stride
//
//      Wsle - the working set array
//
// Return Value:
//
//      Next slot.
//
// Environment:
//
//      Kernel mode, APCs disabled, working set lock held and PFN lock held.
//
//--

typedef struct _MI_NEXT_ESTIMATION_SLOT_CONST {
    WSLE_NUMBER Stride;
} MI_NEXT_ESTIMATION_SLOT_CONST;


#define MI_CALC_NEXT_ESTIMATION_SLOT_CONST(NextEstimationSlotConst, WorkingSetList) \
    (NextEstimationSlotConst).Stride = 1 << MiEstimationShift;

#define MI_NEXT_VALID_ESTIMATION_SLOT(Previous, StartEntry, Minimum, Maximum, NextEstimationSlotConst, Wsle) \
    ASSERT(((Previous) >= Minimum) && ((Previous) <= Maximum)); \
    ASSERT(((StartEntry) >= Minimum) && ((StartEntry) <= Maximum)); \
    do { \
        (Previous) += (NextEstimationSlotConst).Stride; \
        if ((Previous) > Maximum) { \
            (Previous) = Minimum + ((Previous + 1) & (NextEstimationSlotConst.Stride - 1)); \
            StartEntry += 1; \
            (Previous) = StartEntry; \
        } \
        if ((Previous) > Maximum || (Previous) < Minimum) { \
            StartEntry = Minimum; \
            (Previous) = StartEntry; \
        } \
    } while (Wsle[Previous].u1.e1.Valid == 0);

//++
//
// WSLE_NUMBER
// MI_NEXT_VALID_AGING_SLOT (
//     DWORD Previous,
//     DWORD Minimum,
//     DWORD Maximum,
//     PMMWSLE Wsle
//     )
//
// Routine Description:
//
//      This finds the next slot to valid slot to age.  It walks
//      through the slots sequentialy.
//
// Arguments:
//
//      Previous - Last slot used
//
//      Minimum - Minimum acceptable slot (ie. the first dynamic one)
//
//      Maximum - Max slot number + 1
//
//      Wsle - the working set array
//
// Return Value:
//
//      None.
//
// Environment:
//
//      Kernel mode, APCs disabled, working set lock held and PFN lock held.
//
//--

#define MI_NEXT_VALID_AGING_SLOT(Previous, Minimum, Maximum, Wsle) \
    ASSERT(((Previous) >= Minimum) && ((Previous) <= Maximum)); \
    do { \
        (Previous) += 1; \
        if ((Previous) > Maximum) { \
            Previous = Minimum; \
        } \
    } while ((Wsle[Previous].u1.e1.Valid == 0));

//++
//
// ULONG
// MI_CALCULATE_USAGE_ESTIMATE (
//     IN ULONG *SampledAgeCounts
//     )
//
// Routine Description:
//
//      In Usage Estimation, we count the number of pages of each age in
//      a sample.  The function turns the SampledAgeCounts into an
//      estimate of the unused pages.
//
// Arguments:
//
//      SampledAgeCounts - counts of pages of each different age in the sample
//
// Return Value:
//
//      The number of pages to walk in the working set to get a good
//      estimate of the number available.
//
//--

#define MI_CALCULATE_USAGE_ESTIMATE(SampledAgeCounts) \
                (((SampledAgeCounts)[1] + \
                    (SampledAgeCounts)[2] + (SampledAgeCounts)[3]) \
                    << MiEstimationShift)

//++
//
// VOID
// MI_RESET_WSLE_AGE (
//     IN PMMPTE PointerPte,
//     IN PMMWSLE Wsle
//     )
//
// Routine Description:
//
//      Clear the age counter for the working set entry.
//
// Arguments:
//
//      PointerPte - pointer to the working set list entry's PTE.
//
//      Wsle - pointer to the working set list entry.
//
// Return Value:
//
//      None.
//
//--
#define MI_RESET_WSLE_AGE(PointerPte, Wsle) \
    (Wsle)->u1.e1.Age = 0;

//++
//
// ULONG
// MI_GET_WSLE_AGE (
//     IN PMMPTE PointerPte,
//     IN PMMWSLE Wsle
//     )
//
// Routine Description:
//
//      Clear the age counter for the working set entry.
//
// Arguments:
//
//      PointerPte - pointer to the working set list entry's pte
//      Wsle - pointer to the working set list entry
//
// Return Value:
//
//      Age group of the working set entry
//
//--
#define MI_GET_WSLE_AGE(PointerPte, Wsle) \
    ((Wsle)->u1.e1.Age)

//++
//
// VOID
// MI_INC_WSLE_AGE (
//     IN PMMPTE PointerPte,
//     IN PMMWSLE Wsle,
//     )
//
// Routine Description:
//
//      Increment the age counter for the working set entry.
//
// Arguments:
//
//      PointerPte - pointer to the working set list entry's PTE.
//
//      Wsle - pointer to the working set list entry.
//
// Return Value:
//
//      None
//
//--

#define MI_INC_WSLE_AGE(PointerPte, Wsle) \
    if ((Wsle)->u1.e1.Age < 3) { \
        (Wsle)->u1.e1.Age += 1; \
    }

//++
//
// VOID
// MI_UPDATE_USE_ESTIMATE (
//     IN PMMPTE PointerPte,
//     IN PMMWSLE Wsle,
//     IN ULONG *SampledAgeCounts
//     )
//
// Routine Description:
//
//      Update the sampled age counts.
//
// Arguments:
//
//      PointerPte - pointer to the working set list entry's PTE.
//
//      Wsle - pointer to the working set list entry.
//
//      SampledAgeCounts - array of age counts to be updated.
//
// Return Value:
//
//      None
//
//--

#define MI_UPDATE_USE_ESTIMATE(PointerPte, Wsle, SampledAgeCounts) \
    (SampledAgeCounts)[(Wsle)->u1.e1.Age] += 1;

//++
//
// BOOLEAN
// MI_WS_GROWING_TOO_FAST (
//     IN PMMSUPPORT VmSupport
//     )
//
// Routine Description:
//
//      Limit the growth rate of processes as the
//      available memory approaches zero.  Note the caller must ensure that
//      MmAvailablePages is low enough so this calculation does not wrap.
//
// Arguments:
//
//      VmSupport - a working set.
//
// Return Value:
//
//      TRUE if the growth rate is too fast, FALSE otherwise.
//
//--

#define MI_WS_GROWING_TOO_FAST(VmSupport) \
    ((VmSupport)->GrowthSinceLastEstimate > \
        (((MI_CLAIM_INCR * (MmAvailablePages*MmAvailablePages)) / (64*64)) + 1))

//
// Memory Management Object structures.
//

#define SECTION_BASE_ADDRESS(_NtSection) \
    (*((PVOID *)&(_NtSection)->PointerToRelocations))

typedef enum _SECTION_CHECK_TYPE {
    CheckDataSection,
    CheckImageSection,
    CheckUserDataSection,
    CheckBothSection
} SECTION_CHECK_TYPE;

typedef struct _MMEXTEND_INFO {
    UINT64 CommittedSize;
    ULONG ReferenceCount;
} MMEXTEND_INFO, *PMMEXTEND_INFO;

typedef struct _SEGMENT {
    struct _CONTROL_AREA *ControlArea;
    PVOID SegmentBaseAddress;
    ULONG TotalNumberOfPtes;
    ULONG NonExtendedPtes;
    UINT64 SizeOfSegment;
    SIZE_T ImageCommitment;
    PSECTION_IMAGE_INFORMATION ImageInformation;
    PVOID SystemImageBase;
    SIZE_T NumberOfCommittedPages;
    MMPTE SegmentPteTemplate;
    PVOID BasedAddress;
    PMMEXTEND_INFO ExtendInfo;
    PMMPTE PrototypePte;
    MMPTE ThePtes[MM_PROTO_PTE_ALIGNMENT / PAGE_SIZE];

} SEGMENT, *PSEGMENT;

typedef struct _EVENT_COUNTER {
    ULONG RefCount;
    KEVENT Event;
    LIST_ENTRY ListEntry;
} EVENT_COUNTER, *PEVENT_COUNTER;

typedef struct _MMSECTION_FLAGS {
    unsigned BeingDeleted : 1;
    unsigned BeingCreated : 1;
    unsigned BeingPurged : 1;
    unsigned NoModifiedWriting : 1;
    unsigned FailAllIo : 1;
    unsigned Image : 1;
    unsigned Based : 1;
    unsigned File : 1;
    unsigned Networked : 1;
    unsigned NoCache : 1;
    unsigned PhysicalMemory : 1;
    unsigned CopyOnWrite : 1;
    unsigned Reserve : 1;  // not a spare bit!
    unsigned Commit : 1;
    unsigned FloppyMedia : 1;
    unsigned WasPurged : 1;
    unsigned UserReference : 1;
    unsigned GlobalMemory : 1;
    unsigned DeleteOnClose : 1;
    unsigned FilePointerNull : 1;
    unsigned DebugSymbolsLoaded : 1;
    unsigned SetMappedFileIoComplete : 1;
    unsigned CollidedFlush : 1;
    unsigned NoChange : 1;
    unsigned HadUserReference : 1;
    unsigned ImageMappedInSystemSpace : 1;
    unsigned filler0 : 1;
    unsigned Accessed : 1;
    unsigned GlobalOnlyPerSession : 1;
    unsigned filler : 3;
} MMSECTION_FLAGS;

typedef struct _CONTROL_AREA {      // must be quadword sized.
    PSEGMENT Segment;
    LIST_ENTRY DereferenceList;
    ULONG NumberOfSectionReferences;
    ULONG NumberOfPfnReferences;
    ULONG NumberOfMappedViews;
    USHORT NumberOfSubsections;
    USHORT FlushInProgressCount;
    ULONG NumberOfUserReferences;
    union {
        ULONG LongFlags;
        MMSECTION_FLAGS Flags;
    } u;
    PFILE_OBJECT FilePointer;
    PEVENT_COUNTER WaitingForDeletion;
    USHORT ModifiedWriteCount;
    USHORT NumberOfSystemCacheViews;
    SIZE_T PagedPoolUsage;
    SIZE_T NonPagedPoolUsage;
} CONTROL_AREA, *PCONTROL_AREA;

typedef struct _LARGE_CONTROL_AREA {      // must be quadword sized.
    PSEGMENT Segment;
    LIST_ENTRY DereferenceList;
    ULONG NumberOfSectionReferences;
    ULONG NumberOfPfnReferences;
    ULONG NumberOfMappedViews;
    USHORT NumberOfSubsections;
    USHORT FlushInProgressCount;
    ULONG NumberOfUserReferences;
    union {
        ULONG LongFlags;
        MMSECTION_FLAGS Flags;
    } u;
    PFILE_OBJECT FilePointer;
    PEVENT_COUNTER WaitingForDeletion;
    USHORT ModifiedWriteCount;
    USHORT NumberOfSystemCacheViews;
    SIZE_T PagedPoolUsage;
    SIZE_T NonPagedPoolUsage;
    LIST_ENTRY UserGlobalList;
    ULONG SessionId;
    ULONG Pad;
} LARGE_CONTROL_AREA, *PLARGE_CONTROL_AREA;

typedef struct _MMSUBSECTION_FLAGS {
    unsigned ReadOnly : 1;
    unsigned ReadWrite : 1;
    unsigned CopyOnWrite : 1;
    unsigned GlobalMemory: 1;
    unsigned Protection : 5;
    unsigned LargePages : 1;
    unsigned StartingSector4132 : 10;   // 2 ** (42+12) == 4MB*4GB == 16K TB
    unsigned SectorEndOffset : 12;
} MMSUBSECTION_FLAGS;

typedef struct _SUBSECTION { // Must start on quadword boundary and be quad sized
    PCONTROL_AREA ControlArea;
    union {
        ULONG LongFlags;
        MMSUBSECTION_FLAGS SubsectionFlags;
    } u;
    ULONG StartingSector;
    ULONG NumberOfFullSectors;  // (4GB-1) * 4K == 16TB-4K limit per subsection
    PMMPTE SubsectionBase;
    ULONG UnusedPtes;
    ULONG PtesInSubsection;
    struct _SUBSECTION *NextSubsection;
} SUBSECTION, *PSUBSECTION;

#define MI_MAXIMUM_SECTION_SIZE ((UINT64)16*1024*1024*1024*1024*1024 - (1<<MM4K_SHIFT))

#if DBG
VOID
MiSubsectionConsistent(
    IN PSUBSECTION Subsection
    );
#endif

//++
//ULONG
//Mi4KStartForSubsection (
//    IN PLARGE_INTEGER address,
//    IN OUT PSUBSECTION subsection
//    );
//
// Routine Description:
//
//    This macro sets into the specified subsection the supplied information
//    indicating the start address (in 4K units) of this portion of the file.
//
// Arguments
//
//    address - Supplies the 64-bit address (in 4K units) of the start of this
//              portion of the file.
//
//    subsection - Supplies the subsection address to store the address in.
//
// Return Value:
//
//    None.
//
//--

#define Mi4KStartForSubsection(address, subsection)  \
   subsection->StartingSector = ((PLARGE_INTEGER)address)->LowPart; \
   subsection->u.SubsectionFlags.StartingSector4132 = \
        (((PLARGE_INTEGER)(address))->HighPart & 0x3ff);

//++
//ULONG
//Mi4KStartFromSubsection (
//    IN OUT PLARGE_INTEGER address,
//    IN PSUBSECTION subsection
//    );
//
// Routine Description:
//
//    This macro gets the start 4K offset from the specified subsection.
//
// Arguments
//
//    address - Supplies the 64-bit address (in 4K units) to place the
//              start of this subsection into.
//
//    subsection - Supplies the subsection address to get the address from.
//
// Return Value:
//
//    None.
//
//--

#define Mi4KStartFromSubsection(address, subsection)  \
   ((PLARGE_INTEGER)address)->LowPart = subsection->StartingSector; \
   ((PLARGE_INTEGER)address)->HighPart = subsection->u.SubsectionFlags.StartingSector4132;

typedef struct _MMDEREFERENCE_SEGMENT_HEADER {
    KSPIN_LOCK Lock;
    KSEMAPHORE Semaphore;
    LIST_ENTRY ListHead;
} MMDEREFERENCE_SEGMENT_HEADER;

//
// This entry is used for calling the segment dereference thread
// to perform page file expansion.  It has a similar structure
// to a control area to allow either a control area or a page file
// expansion entry to be placed on the list.  Note that for a control
// area the segment pointer is valid whereas for page file expansion
// it is null.
//

typedef struct _MMPAGE_FILE_EXPANSION {
    PSEGMENT Segment;
    LIST_ENTRY DereferenceList;
    SIZE_T RequestedExpansionSize;
    SIZE_T ActualExpansion;
    KEVENT Event;
    ULONG InProgress;
    ULONG PageFileNumber;
} MMPAGE_FILE_EXPANSION, *PMMPAGE_FILE_EXPANSION;

#define MI_EXTEND_ANY_PAGEFILE      ((ULONG)-1)

typedef struct _MMWORKING_SET_EXPANSION_HEAD {
    LIST_ENTRY ListHead;
} MMWORKING_SET_EXPANSION_HEAD;

#define SUBSECTION_READ_ONLY      1L
#define SUBSECTION_READ_WRITE     2L
#define SUBSECTION_COPY_ON_WRITE  4L
#define SUBSECTION_SHARE_ALLOW    8L

typedef struct _MMFLUSH_BLOCK {
    LARGE_INTEGER ErrorOffset;
    IO_STATUS_BLOCK IoStatus;
    KEVENT IoEvent;
    ULONG IoCount;
} MMFLUSH_BLOCK, *PMMFLUSH_BLOCK;

typedef struct _MMINPAGE_SUPPORT {
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER ReadOffset;
    ULONG WaitCount;
#if defined (_WIN64)
    ULONG UsedPageTableEntries;
#endif
    union {
        PETHREAD Thread;
        PMMFLUSH_BLOCK Flush;
    } u;
    PFILE_OBJECT FilePointer;
    PMMPTE BasePte;
    PMMPFN Pfn;
    LOGICAL Completed;
    MDL Mdl;
    PFN_NUMBER Page[MM_MAXIMUM_READ_CLUSTER_SIZE + 1];
    LIST_ENTRY ListEntry;
#if defined (_PREFETCH_)
    PMDL PrefetchMdl;
#endif
} MMINPAGE_SUPPORT, *PMMINPAGE_SUPPORT;

#if defined (_PREFETCH_)
#define MI_PF_DUMMY_PAGE_PTE ((PMMPTE)0x23452345)
#endif

typedef struct _MMPAGE_READ {
    LARGE_INTEGER ReadOffset;
    PFILE_OBJECT FilePointer;
    PMMPTE BasePte;
    PMMPFN Pfn;
    MDL Mdl;
    PFN_NUMBER Page[MM_MAXIMUM_READ_CLUSTER_SIZE + 1];
} MMPAGE_READ, *PMMPAGE_READ;

//
// Address Node.
//

typedef struct _MMADDRESS_NODE {
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
    struct _MMADDRESS_NODE *Parent;
    struct _MMADDRESS_NODE *LeftChild;
    struct _MMADDRESS_NODE *RightChild;
} MMADDRESS_NODE, *PMMADDRESS_NODE;

typedef struct _SECTION {
    MMADDRESS_NODE Address;
    PSEGMENT Segment;
    LARGE_INTEGER SizeOfSection;
    union {
        ULONG LongFlags;
        MMSECTION_FLAGS Flags;
    } u;
    ULONG InitialPageProtection;
} SECTION, *PSECTION;

//
// Banked memory descriptor.  Pointed to by VAD which has
// the PhysicalMemory flags set and the Banked pointer field as
// non-NULL.
//

typedef struct _MMBANKED_SECTION {
    PFN_NUMBER BasePhysicalPage;
    PMMPTE BasedPte;
    ULONG BankSize;
    ULONG BankShift; //shift for PTEs to calculate bank number
    PBANKED_SECTION_ROUTINE BankedRoutine;
    PVOID Context;
    PMMPTE CurrentMappedPte;
    MMPTE BankTemplate[1];
} MMBANKED_SECTION, *PMMBANKED_SECTION;


//
// Virtual address descriptor
//
// ***** NOTE **********
//  The first part of a virtual address descriptor is a MMADDRESS_NODE!!!
//

#if defined (_WIN64)

#define COMMIT_SIZE 51

#if ((COMMIT_SIZE + PAGE_SHIFT) < 63)
#error COMMIT_SIZE too small
#endif

#else
#define COMMIT_SIZE 19

#if ((COMMIT_SIZE + PAGE_SHIFT) < 31)
#error COMMIT_SIZE too small
#endif
#endif

#define MM_MAX_COMMIT (((ULONG_PTR) 1 << COMMIT_SIZE) - 1)

#define MM_VIEW_UNMAP 0
#define MM_VIEW_SHARE 1

typedef struct _MMVAD_FLAGS {
    ULONG_PTR CommitCharge : COMMIT_SIZE; //limits system to 4k pages or bigger!
    ULONG_PTR PhysicalMapping : 1;
    ULONG_PTR ImageMap : 1;
    ULONG_PTR UserPhysicalPages : 1;
    ULONG_PTR NoChange : 1;
    ULONG_PTR WriteWatch : 1;
    ULONG_PTR Protection : 5;
    ULONG_PTR LargePages : 1;
    ULONG_PTR MemCommit: 1;
    ULONG_PTR PrivateMemory : 1;    //used to tell VAD from VAD_SHORT
} MMVAD_FLAGS;

typedef struct _MMVAD_FLAGS2 {
    unsigned FileOffset : 24;        // number of 64k units into file
    unsigned SecNoChange : 1;        // set if SEC_NOCHANGE specified
    unsigned OneSecured : 1;         // set if u3 field is a range
    unsigned MultipleSecured : 1;    // set if u3 field is a list head
    unsigned ReadOnly : 1;           // protected as ReadOnly
    unsigned StoredInVad : 1;        // set if secure is stored in VAD
    unsigned ExtendableFile : 1;
    unsigned Inherit : 1; //1 = ViewShare, 0 = ViewUnmap
    unsigned CopyOnWrite : 1;
} MMVAD_FLAGS2;

typedef struct _MMADDRESS_LIST {
    ULONG_PTR StartVpn;
    ULONG_PTR EndVpn;
} MMADDRESS_LIST, *PMMADDRESS_LIST;

typedef struct _MMSECURE_ENTRY {
    union {
        ULONG_PTR LongFlags2;
        MMVAD_FLAGS2 VadFlags2;
    } u2;
    ULONG_PTR StartVpn;
    ULONG_PTR EndVpn;
    LIST_ENTRY List;
} MMSECURE_ENTRY, *PMMSECURE_ENTRY;

typedef struct _MMVAD {
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
    struct _MMVAD *Parent;
    struct _MMVAD *LeftChild;
    struct _MMVAD *RightChild;
    union {
        ULONG_PTR LongFlags;
        MMVAD_FLAGS VadFlags;
    } u;
    PCONTROL_AREA ControlArea;
    PMMPTE FirstPrototypePte;
    PMMPTE LastContiguousPte;
    union {
        ULONG LongFlags2;
        MMVAD_FLAGS2 VadFlags2;
    } u2;
    union {
        LIST_ENTRY List;
        MMADDRESS_LIST Secured;
    } u3;
    union {
        PMMBANKED_SECTION Banked;
        PMMEXTEND_INFO ExtendedInfo;
    } u4;
} MMVAD, *PMMVAD;


typedef struct _MMVAD_SHORT {
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
    struct _MMVAD *Parent;
    struct _MMVAD *LeftChild;
    struct _MMVAD *RightChild;
    union {
        ULONG_PTR LongFlags;
        MMVAD_FLAGS VadFlags;
    } u;
} MMVAD_SHORT, *PMMVAD_SHORT;

#define MI_GET_PROTECTION_FROM_VAD(_Vad) ((ULONG)(_Vad)->u.VadFlags.Protection)

typedef struct _MI_PHYSICAL_VIEW {
    LIST_ENTRY ListEntry;
    PMMVAD Vad;
    PCHAR StartVa;
    PCHAR EndVa;
    PRTL_BITMAP BitMap;   // Only initialized if Vad->u.VadFlags.WriteWatch == 1
} MI_PHYSICAL_VIEW, *PMI_PHYSICAL_VIEW;

#define MI_PHYSICAL_VIEW_KEY 'vpmM'
#define MI_WRITEWATCH_VIEW_KEY 'wWmM'

//
// Stuff for support of Write Watch.
//

extern ULONG_PTR MiActiveWriteWatch;

VOID
MiCaptureWriteWatchDirtyBit (
    IN PEPROCESS Process,
    IN PVOID VirtualAddress
    );

VOID
MiMarkProcessAsWriteWatch (
    IN PEPROCESS Process
    );

//
// Stuff for support of POSIX Fork.
//


typedef struct _MMCLONE_BLOCK {
    MMPTE ProtoPte;
    LONG CloneRefCount;
} MMCLONE_BLOCK;

typedef MMCLONE_BLOCK *PMMCLONE_BLOCK;

typedef struct _MMCLONE_HEADER {
    ULONG NumberOfPtes;
    ULONG NumberOfProcessReferences;
    PMMCLONE_BLOCK ClonePtes;
} MMCLONE_HEADER, *PMMCLONE_HEADER;


typedef struct _MMCLONE_DESCRIPTOR {
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
    struct _MMCLONE_DESCRIPTOR *Parent;
    struct _MMCLONE_DESCRIPTOR *LeftChild;
    struct _MMCLONE_DESCRIPTOR *RightChild;
    PMMCLONE_HEADER CloneHeader;
    ULONG NumberOfPtes;
    ULONG NumberOfReferences;
    SIZE_T PagedPoolQuotaCharge;
} MMCLONE_DESCRIPTOR, *PMMCLONE_DESCRIPTOR;

//
// The following macro will allocate and initialize a bitmap from the
// specified pool of the specified size
//
//      VOID
//      MiCreateBitMap (
//          OUT PRTL_BITMAP *BitMapHeader,
//          IN SIZE_T SizeOfBitMap,
//          IN POOL_TYPE PoolType
//          );
//

#define MiCreateBitMap(BMH,S,P) {                          \
    ULONG _S;                                              \
    _S = sizeof(RTL_BITMAP) + (ULONG)((((S) + 31) / 32) * 4);         \
    *(BMH) = (PRTL_BITMAP)ExAllocatePoolWithTag( (P), _S, '  mM');       \
    if (*(BMH)) { \
        RtlInitializeBitMap( *(BMH), (PULONG)((*(BMH))+1), (ULONG)S); \
    }                                                          \
}

#define MiRemoveBitMap(BMH)     {                          \
    ExFreePool(*(BMH));                                    \
    *(BMH) = NULL;                                         \
}

#define MI_INITIALIZE_ZERO_MDL(MDL) { \
    MDL->Next = (PMDL) NULL; \
    MDL->MdlFlags = 0; \
    MDL->StartVa = NULL; \
    MDL->ByteOffset = 0; \
    MDL->ByteCount = 0; \
    }

//
// Page File structures.
//

typedef struct _MMMOD_WRITER_LISTHEAD {
    LIST_ENTRY ListHead;
    KEVENT Event;
} MMMOD_WRITER_LISTHEAD, *PMMMOD_WRITER_LISTHEAD;

typedef struct _MMMOD_WRITER_MDL_ENTRY {
    LIST_ENTRY Links;
    LARGE_INTEGER WriteOffset;
    union {
        IO_STATUS_BLOCK IoStatus;
        LARGE_INTEGER LastByte;
    } u;
    PIRP Irp;
    ULONG_PTR LastPageToWrite;
    PMMMOD_WRITER_LISTHEAD PagingListHead;
    PLIST_ENTRY CurrentList;
    struct _MMPAGING_FILE *PagingFile;
    PFILE_OBJECT File;
    PCONTROL_AREA ControlArea;
    PERESOURCE FileResource;
    MDL Mdl;
    PFN_NUMBER Page[1];
} MMMOD_WRITER_MDL_ENTRY, *PMMMOD_WRITER_MDL_ENTRY;


#define MM_PAGING_FILE_MDLS 2

typedef struct _MMPAGING_FILE {
    PFN_NUMBER Size;
    PFN_NUMBER MaximumSize;
    PFN_NUMBER MinimumSize;
    PFN_NUMBER FreeSpace;
    PFN_NUMBER CurrentUsage;
    PFN_NUMBER PeakUsage;
    PFN_NUMBER Hint;
    PFN_NUMBER HighestPage;
    PMMMOD_WRITER_MDL_ENTRY Entry[MM_PAGING_FILE_MDLS];
    PRTL_BITMAP Bitmap;
    PFILE_OBJECT File;
    UNICODE_STRING PageFileName;
    ULONG PageFileNumber;
    BOOLEAN Extended;
    BOOLEAN HintSetToZero;
    } MMPAGING_FILE, *PMMPAGING_FILE;

typedef struct _MMINPAGE_SUPPORT_LIST {
    LIST_ENTRY ListHead;
    ULONG Count;
} MMINPAGE_SUPPORT_LIST, *PMMINPAGE_SUPPORT_LIST;

typedef struct _MMEVENT_COUNT_LIST {
    LIST_ENTRY ListHead;
    ULONG Count;
} MMEVENT_COUNT_LIST, *PMMEVENT_COUNT_LIST;

//
// System PTE structures.
//

#define MM_SYS_PTE_TABLES_MAX 5

typedef enum _MMSYSTEM_PTE_POOL_TYPE {
    SystemPteSpace,
    NonPagedPoolExpansion,
    MaximumPtePoolTypes
} MMSYSTEM_PTE_POOL_TYPE;

typedef struct _MMFREE_POOL_ENTRY {
    LIST_ENTRY List;
    PFN_NUMBER Size;
    ULONG Signature;
    struct _MMFREE_POOL_ENTRY *Owner;
} MMFREE_POOL_ENTRY, *PMMFREE_POOL_ENTRY;


typedef struct _MMLOCK_CONFLICT {
    LIST_ENTRY List;
    PETHREAD Thread;
} MMLOCK_CONFLICT, *PMMLOCK_CONFLICT;

//
// System view structures
//

typedef struct _MMVIEW {
    ULONG_PTR Entry;
    PCONTROL_AREA ControlArea;
} MMVIEW, *PMMVIEW;

//
// The MMSESSION structure represents kernel memory that is only valid on a
// per-session basis, thus the calling thread must be in the proper session
// to access this structure.
//

typedef struct _MMSESSION {

    //
    // Never refer to the SystemSpaceViewLock directly - always use the pointer
    // following it or you will break support for multiple concurrent sessions.
    //

    FAST_MUTEX SystemSpaceViewLock;

    //
    // This points to the mutex above and is needed because the MMSESSION
    // is mapped in session space on Hydra and the mutex needs to be globally
    // visible for proper KeWaitForSingleObject & KeSetEvent operation.
    //

    PFAST_MUTEX SystemSpaceViewLockPointer;
    PCHAR SystemSpaceViewStart;
    PMMVIEW SystemSpaceViewTable;
    ULONG SystemSpaceHashSize;
    ULONG SystemSpaceHashEntries;
    ULONG SystemSpaceHashKey;
    PRTL_BITMAP SystemSpaceBitMap;

} MMSESSION, *PMMSESSION;

extern MMSESSION   MmSession;

#define LOCK_SYSTEM_VIEW_SPACE(_Session) \
            ExAcquireFastMutex (_Session->SystemSpaceViewLockPointer)

#define UNLOCK_SYSTEM_VIEW_SPACE(_Session) \
            ExReleaseFastMutex (_Session->SystemSpaceViewLockPointer)

//
// List for flushing TBs singularly.
//

typedef struct _MMPTE_FLUSH_LIST {
    ULONG Count;
    PMMPTE FlushPte[MM_MAXIMUM_FLUSH_COUNT];
    PVOID FlushVa[MM_MAXIMUM_FLUSH_COUNT];
} MMPTE_FLUSH_LIST, *PMMPTE_FLUSH_LIST;

typedef struct _LOCK_TRACKER {
    LIST_ENTRY ListEntry;
    PMDL Mdl;
    PVOID StartVa;
    PFN_NUMBER Count;
    ULONG Offset;
    ULONG Length;
    PFN_NUMBER Page;
    PVOID CallingAddress;
    PVOID CallersCaller;
    LIST_ENTRY GlobalListEntry;
    ULONG Who;
    PEPROCESS Process;
} LOCK_TRACKER, *PLOCK_TRACKER;

extern LOGICAL  MmTrackLockedPages;
extern BOOLEAN  MiTrackingAborted;

typedef struct _LOCK_HEADER {
    LIST_ENTRY ListHead;
    PFN_NUMBER Count;
} LOCK_HEADER, *PLOCK_HEADER;

extern LOGICAL MmSnapUnloads;

#define MI_UNLOADED_DRIVERS 50

typedef struct _UNLOADED_DRIVERS {
    UNICODE_STRING Name;
    PVOID StartAddress;
    PVOID EndAddress;
    LARGE_INTEGER CurrentTime;
} UNLOADED_DRIVERS, *PUNLOADED_DRIVERS;

extern PUNLOADED_DRIVERS MiUnloadedDrivers;

VOID
MiRemoveConflictFromList (
    IN PMMLOCK_CONFLICT Conflict
    );

VOID
MiInsertConflictInList (
    IN PMMLOCK_CONFLICT Conflict
    );


VOID
MiInitMachineDependent (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiBuildPagedPool (
    VOID
    );

VOID
MiInitializeNonPagedPool (
    VOID
    );

BOOLEAN
MiInitializeSystemSpaceMap (
    PVOID Session OPTIONAL
    );

VOID
MiFindInitializationCode (
    OUT PVOID *StartVa,
    OUT PVOID *EndVa
    );

VOID
MiFreeInitializationCode (
    IN PVOID StartVa,
    IN PVOID EndVa
    );


ULONG
MiSectionInitialization (
    VOID
    );

VOID
FASTCALL
MiDecrementReferenceCount (
    IN PFN_NUMBER PageFrameIndex
    );

VOID
FASTCALL
MiDecrementShareCount (
    IN PFN_NUMBER PageFrameIndex
    );

#define MiDecrementShareCountOnly(P) MiDecrementShareCount(P)

#define MiDecrementShareAndValidCount(P) MiDecrementShareCount(P)

//
// Routines which operate on the Page Frame Database Lists
//

VOID
FASTCALL
MiInsertPageInList (
    IN PMMPFNLIST ListHead,
    IN PFN_NUMBER PageFrameIndex
    );

VOID
FASTCALL
MiInsertStandbyListAtFront (
    IN PFN_NUMBER PageFrameIndex
    );

PFN_NUMBER  //PageFrameIndex
FASTCALL
MiRemovePageFromList (
    IN PMMPFNLIST ListHead
    );

VOID
FASTCALL
MiUnlinkPageFromList (
    IN PMMPFN Pfn
    );

VOID
MiUnlinkFreeOrZeroedPage (
    IN PFN_NUMBER Page
    );

VOID
FASTCALL
MiInsertFrontModifiedNoWrite (
    IN PFN_NUMBER PageFrameIndex
    );

ULONG
FASTCALL
MiEnsureAvailablePageOrWait (
    IN PEPROCESS Process,
    IN PVOID VirtualAddress
    );

PFN_NUMBER
FASTCALL
MiRemoveZeroPage (
    IN ULONG PageColor
    );

#define MiRemoveZeroPageIfAny(COLOR)   \
    (MmFreePagesByColor[ZeroedPageList][COLOR].Flink != MM_EMPTY_LIST) ? \
                       MiRemoveZeroPage(COLOR) : 0


PFN_NUMBER  //PageFrameIndex
FASTCALL
MiRemoveAnyPage (
    IN ULONG PageColor
    );

PVOID
MiFindContiguousMemory (
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PFN_NUMBER SizeInPages,
    IN PVOID CallingAddress
    );

PVOID
MiCheckForContiguousMemory (
    IN PVOID BaseAddress,
    IN PFN_NUMBER BaseAddressPages,
    IN PFN_NUMBER SizeInPages,
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn
    );

//
// Routines which operate on the page frame database entry.
//

VOID
MiInitializePfn (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN ULONG ModifiedState
    );

VOID
MiInitializePfnForOtherProcess (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN PFN_NUMBER ContainingPageFrame
    );

VOID
MiInitializeCopyOnWritePfn (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN WSLE_NUMBER WorkingSetIndex,
    IN PVOID SessionSpace
    );

VOID
MiInitializeTransitionPfn (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPte,
    IN WSLE_NUMBER WorkingSetIndex
    );

VOID
MiFlushInPageSupportBlock (
    );

VOID
MiFreeInPageSupportBlock (
    IN PMMINPAGE_SUPPORT Support
    );

PMMINPAGE_SUPPORT
MiGetInPageSupportBlock (
    VOID
    );

//
// Routines which require a physical page to be mapped into hyperspace
// within the current process.
//

VOID
FASTCALL
MiZeroPhysicalPage (
    IN PFN_NUMBER PageFrameIndex,
    IN ULONG Color
    );

VOID
FASTCALL
MiRestoreTransitionPte (
    IN PFN_NUMBER PageFrameIndex
    );

PSUBSECTION
MiGetSubsectionAndProtoFromPte (
    IN PMMPTE PointerPte,
    IN PMMPTE *ProtoPte,
    IN PEPROCESS Process
    );

PVOID
MiMapPageInHyperSpace (
    IN PFN_NUMBER PageFrameIndex,
    OUT PKIRQL OldIrql
    );

#define MiUnmapPageInHyperSpace(OLDIRQL) UNLOCK_HYPERSPACE(OLDIRQL)


PVOID
MiMapImageHeaderInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
    );

VOID
MiUnmapImageHeaderInHyperSpace (
    VOID
    );

VOID
MiUpdateImageHeaderPage (
    IN PMMPTE PointerPte,
    IN PFN_NUMBER PageFrameNumber,
    IN PCONTROL_AREA ControlArea
    );

PFN_NUMBER
MiGetPageForHeader (
    VOID
    );

VOID
MiRemoveImageHeaderPage (
    IN PFN_NUMBER PageFrameNumber
    );

PVOID
MiMapPageToZeroInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
    );


NTSTATUS
MiGetWritablePagesInSection(
    IN PSECTION Section,
    OUT PULONG WritablePages
    );

//
// Routines to obtain and release system PTEs.
//

PMMPTE
MiReserveSystemPtes (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPteType,
    IN ULONG Alignment,
    IN ULONG Offset,
    IN ULONG BugCheckOnFailure
    );

VOID
MiReleaseSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPteType
    );

VOID
MiInitializeSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPteType
    );

VOID
MiAddMappedPtes (
    IN PMMPTE FirstPte,
    IN ULONG NumberOfPtes,
    IN PCONTROL_AREA ControlArea
    );

VOID
MiInitializeIoTrackers (
    VOID
    );

PVOID
MiMapSinglePage (
     IN PVOID VirtualAddress OPTIONAL,
     IN PFN_NUMBER PageFrameIndex,
     IN MEMORY_CACHING_TYPE CacheType,
     IN MM_PAGE_PRIORITY Priority
     );

VOID
MiUnmapSinglePage (
     IN PVOID BaseAddress
     );

//
// Access Fault routines.
//

NTSTATUS
MiDispatchFault (
    IN BOOLEAN StoreInstrution,
    IN PVOID VirtualAdress,
    IN PMMPTE PointerPte,
    IN PMMPTE PointerProtoPte,
    IN PEPROCESS Process,
    OUT PLOGICAL ApcNeeded
    );

NTSTATUS
MiResolveDemandZeroFault (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte,
    IN PEPROCESS Process,
    IN ULONG PrototypePte
    );

NTSTATUS
MiResolveTransitionFault (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN PEPROCESS Process,
    IN ULONG PfnLockHeld,
    OUT PLOGICAL ApcNeeded
    );

NTSTATUS
MiResolvePageFileFault (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN PMMINPAGE_SUPPORT *ReadBlock,
    IN PEPROCESS Process
    );

NTSTATUS
MiResolveProtoPteFault (
    IN BOOLEAN StoreInstruction,
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte,
    IN PMMPTE PointerProtoPte,
    IN PMMINPAGE_SUPPORT *ReadBlock,
    IN PEPROCESS Process,
    OUT PLOGICAL ApcNeeded
    );


NTSTATUS
MiResolveMappedFileFault (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN PMMINPAGE_SUPPORT *ReadBlock,
    IN PEPROCESS Process
    );

VOID
MiAddValidPageToWorkingSet (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn1,
    IN ULONG WsleMask
    );

NTSTATUS
MiWaitForInPageComplete (
    IN PMMPFN Pfn,
    IN PMMPTE PointerPte,
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPteContents,
    IN PMMINPAGE_SUPPORT InPageSupport,
    IN PEPROCESS CurrentProcess
    );

NTSTATUS
FASTCALL
MiCopyOnWrite (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte
    );

VOID
MiSetDirtyBit (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN ULONG PfnHeld
    );

VOID
MiSetModifyBit (
    IN PMMPFN Pfn
    );

PMMPTE
MiFindActualFaultingPte (
    IN PVOID FaultingAddress
    );

VOID
MiInitializeReadInProgressPfn (
    IN PMDL Mdl,
    IN PMMPTE BasePte,
    IN PKEVENT Event,
    IN WSLE_NUMBER WorkingSetIndex
    );

NTSTATUS
MiAccessCheck (
    IN PMMPTE PointerPte,
    IN BOOLEAN WriteOperation,
    IN KPROCESSOR_MODE PreviousMode,
    IN ULONG Protection,
    IN BOOLEAN CallerHoldsPfnLock
    );

NTSTATUS
FASTCALL
MiCheckForUserStackOverflow (
    IN PVOID FaultingAddress
    );

PMMPTE
MiCheckVirtualAddress (
    IN PVOID VirtualAddress,
    OUT PULONG ProtectCode
    );

NTSTATUS
FASTCALL
MiCheckPdeForPagedPool (
    IN PVOID VirtualAddress
    );

VOID
MiInitializeMustSucceedPool (
    VOID
    );

//
// Routines which operate on an address tree.
//

PMMADDRESS_NODE
FASTCALL
MiGetNextNode (
    IN PMMADDRESS_NODE Node
    );

PMMADDRESS_NODE
FASTCALL
MiGetPreviousNode (
    IN PMMADDRESS_NODE Node
    );


PMMADDRESS_NODE
FASTCALL
MiGetFirstNode (
    IN PMMADDRESS_NODE Root
    );

PMMADDRESS_NODE
MiGetLastNode (
    IN PMMADDRESS_NODE Root
    );

VOID
FASTCALL
MiInsertNode (
    IN PMMADDRESS_NODE Node,
    IN OUT PMMADDRESS_NODE *Root
    );

VOID
FASTCALL
MiRemoveNode (
    IN PMMADDRESS_NODE Node,
    IN OUT PMMADDRESS_NODE *Root
    );

PMMADDRESS_NODE
FASTCALL
MiLocateAddressInTree (
    IN ULONG_PTR Vpn,
    IN PMMADDRESS_NODE *Root
    );

PMMADDRESS_NODE
MiCheckForConflictingNode (
    IN ULONG_PTR StartVpn,
    IN ULONG_PTR EndVpn,
    IN PMMADDRESS_NODE Root
    );

PVOID
MiFindEmptyAddressRangeInTree (
    IN SIZE_T SizeOfRange,
    IN ULONG_PTR Alignment,
    IN PMMADDRESS_NODE Root,
    OUT PMMADDRESS_NODE *PreviousVad
    );

PVOID
MiFindEmptyAddressRangeDownTree (
    IN SIZE_T SizeOfRange,
    IN PVOID HighestAddressToEndAt,
    IN ULONG_PTR Alignment,
    IN PMMADDRESS_NODE Root
    );

VOID
NodeTreeWalk (
    PMMADDRESS_NODE Start
    );

//
// Routines which operate on the tree of virtual address descriptors.
//

VOID
MiInsertVad (
    IN PMMVAD Vad
    );

VOID
MiRemoveVad (
    IN PMMVAD Vad
    );

PMMVAD
FASTCALL
MiLocateAddress (
    IN PVOID Vad
    );

PVOID
MiFindEmptyAddressRange (
    IN SIZE_T SizeOfRange,
    IN ULONG_PTR Alignment,
    IN ULONG QuickCheck
    );

//
// Routines which operate on the clone tree structure.
//


NTSTATUS
MiCloneProcessAddressSpace (
    IN PEPROCESS ProcessToClone,
    IN PEPROCESS ProcessToInitialize,
    IN PFN_NUMBER PdePhysicalPage,
    IN PFN_NUMBER HyperPhysicalPage
    );


ULONG
MiDecrementCloneBlockReference (
    IN PMMCLONE_DESCRIPTOR CloneDescriptor,
    IN PMMCLONE_BLOCK CloneBlock,
    IN PEPROCESS CurrentProcess
    );

VOID
MiWaitForForkToComplete (
    IN PEPROCESS CurrentProcess
    );

//
// Routines which operate on the working set list.
//

WSLE_NUMBER
MiLocateAndReserveWsle (
    IN PMMSUPPORT WsInfo
    );

VOID
MiReleaseWsle (
    IN WSLE_NUMBER WorkingSetIndex,
    IN PMMSUPPORT WsInfo
    );

VOID
MiUpdateWsle (
    IN PWSLE_NUMBER DesiredIndex,
    IN PVOID VirtualAddress,
    IN PMMWSL WorkingSetList,
    IN PMMPFN Pfn
    );

VOID
MiInitializeWorkingSetList (
    IN PEPROCESS CurrentProcess
    );

VOID
MiGrowWsleHash (
    IN PMMSUPPORT WsInfo
    );

ULONG
MiTrimWorkingSet (
    ULONG Reduction,
    IN PMMSUPPORT WsInfo,
    IN ULONG ForcedReduction
    );

VOID
MiRemoveWorkingSetPages (
    IN PMMWSL WorkingSetList,
    IN PMMSUPPORT WsInfo
    );

#ifdef _MI_USE_CLAIMS_
VOID
MiAgeAndEstimateAvailableInWorkingSet(
    IN PMMSUPPORT VmSupport,
    IN BOOLEAN DoAging,
    IN OUT PULONG TotalClaim,
    IN OUT PULONG TotalEstimatedAvailable
    );
#endif

VOID
FASTCALL
MiInsertWsle (
    IN WSLE_NUMBER Entry,
    IN PMMWSL WorkingSetList
    );

VOID
FASTCALL
MiRemoveWsle (
    IN WSLE_NUMBER Entry,
    IN PMMWSL WorkingSetList
    );

VOID
MiFreeWorkingSetRange (
    IN PVOID StartVa,
    IN PVOID EndVa,
    IN PMMSUPPORT WsInfo
    );

WSLE_NUMBER
FASTCALL
MiLocateWsle (
    IN PVOID VirtualAddress,
    IN PMMWSL WorkingSetList,
    IN WSLE_NUMBER WsPfnIndex
    );

ULONG
MiFreeWsle (
    IN WSLE_NUMBER WorkingSetIndex,
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    );

VOID
MiSwapWslEntries (
    IN WSLE_NUMBER SwapEntry,
    IN WSLE_NUMBER Entry,
    IN PMMSUPPORT WsInfo
    );

VOID
MiRemoveWsleFromFreeList (
    IN ULONG Entry,
    IN PMMWSLE Wsle,
    IN PMMWSL WorkingSetList
    );

ULONG
MiRemovePageFromWorkingSet (
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn1,
    IN PMMSUPPORT WsInfo
    );

VOID
MiTakePageFromWorkingSet (
    IN ULONG Entry,
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    );

PFN_NUMBER
MiDeleteSystemPagableVm (
    IN PMMPTE PointerPte,
    IN PFN_NUMBER NumberOfPtes,
    IN MMPTE NewPteValue,
    IN LOGICAL SessionAllocation,
    OUT PPFN_NUMBER ResidentPages OPTIONAL
    );

VOID
MiLockCode (
    IN PMMPTE FirstPte,
    IN PMMPTE LastPte,
    IN ULONG LockType
    );

PLDR_DATA_TABLE_ENTRY
MiLookupDataTableEntry (
    IN PVOID AddressWithinSection,
    IN ULONG ResourceHeld
    );

VOID
MiFlushTb (
    VOID
    );

//
// Routines which perform working set management.
//

VOID
MiObtainFreePages (
    VOID
    );

VOID
MiModifiedPageWriter (
    IN PVOID StartContext
    );

VOID
MiMappedPageWriter (
    IN PVOID StartContext
    );

LOGICAL
MiIssuePageExtendRequest (
    IN PMMPAGE_FILE_EXPANSION PageExtend
    );

VOID
MiIssuePageExtendRequestNoWait (
    IN PFN_NUMBER SizeInPages
    );

SIZE_T
MiExtendPagingFiles (
    IN PMMPAGE_FILE_EXPANSION PageExpand
    );

VOID
MiContractPagingFiles (
    VOID
    );

VOID
MiAttemptPageFileReduction (
    VOID
    );

LOGICAL
MiCancelWriteOfMappedPfn (
    IN PFN_NUMBER PageToStop
    );

//
// Routines to delete address space.
//

VOID
MiDeleteVirtualAddresses (
    IN PUCHAR StartingAddress,
    IN PUCHAR EndingAddress,
    IN ULONG AddressSpaceDeletion,
    IN PMMVAD Vad
    );

VOID
MiDeletePte (
    IN PMMPTE PointerPte,
    IN PVOID VirtualAddress,
    IN ULONG AddressSpaceDeletion,
    IN PEPROCESS CurrentProcess,
    IN PMMPTE PrototypePte,
    IN PMMPTE_FLUSH_LIST PteFlushList OPTIONAL
    );

VOID
MiDeletePageTablesForPhysicalRange (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    );

VOID
MiFlushPteList (
    IN PMMPTE_FLUSH_LIST PteFlushList,
    IN ULONG AllProcessors,
    IN MMPTE FillPte
    );


ULONG
FASTCALL
MiReleasePageFileSpace (
    IN MMPTE PteContents
    );

VOID
FASTCALL
MiUpdateModifiedWriterMdls (
    IN ULONG PageFileNumber
    );

VOID
MiRemoveUserPhysicalPagesVad (
    IN PMMVAD_SHORT FoundVad
    );

VOID
MiCleanPhysicalProcessPages (
    IN PEPROCESS Process
    );

VOID
MiPhysicalViewRemover (
    IN PEPROCESS Process,
    IN PMMVAD Vad
    );

VOID
MiPhysicalViewAdjuster (
    IN PEPROCESS Process,
    IN PMMVAD OldVad,
    IN PMMVAD NewVad
    );

//
// General support routines.
//

ULONG
MiDoesPdeExistAndMakeValid (
    IN PMMPTE PointerPde,
    IN PEPROCESS TargetProcess,
    IN ULONG PfnMutexHeld,
    OUT PULONG Waited
    );

#if defined (_WIN64)
LOGICAL
MiDoesPpeExistAndMakeValid (
    IN PMMPTE PointerPpe,
    IN PEPROCESS TargetProcess,
    IN ULONG PfnMutexHeld,
    OUT PULONG Waited
    );
#endif

ULONG
MiMakePdeExistAndMakeValid (
    IN PMMPTE PointerPde,
    IN PEPROCESS TargetProcess,
    IN ULONG PfnMutexHeld
    );

#if defined (_WIN64)
ULONG
MiMakePpeExistAndMakeValid (
    IN PMMPTE PointerPde,
    IN PEPROCESS TargetProcess,
    IN ULONG PfnMutexHeld
    );
#else
#define MiMakePpeExistAndMakeValid(PDE, PROCESS, PFNMUTEXHELD)
#endif

ULONG
FASTCALL
MiMakeSystemAddressValid (
    IN PVOID VirtualAddress,
    IN PEPROCESS CurrentProcess
    );

ULONG
FASTCALL
MiMakeSystemAddressValidPfnWs (
    IN PVOID VirtualAddress,
    IN PEPROCESS CurrentProcess OPTIONAL
    );

ULONG
FASTCALL
MiMakeSystemAddressValidPfnSystemWs (
    IN PVOID VirtualAddress
    );

ULONG
FASTCALL
MiMakeSystemAddressValidPfn (
    IN PVOID VirtualAddress
    );

ULONG
FASTCALL
MiLockPagedAddress (
    IN PVOID VirtualAddress,
    IN ULONG PfnLockHeld
    );

VOID
FASTCALL
MiUnlockPagedAddress (
    IN PVOID VirtualAddress,
    IN ULONG PfnLockHeld
    );

ULONG
FASTCALL
MiIsPteDecommittedPage (
    IN PMMPTE PointerPte
    );

ULONG
FASTCALL
MiIsProtectionCompatible (
    IN ULONG OldProtect,
    IN ULONG NewProtect
    );

ULONG
FASTCALL
MiMakeProtectionMask (
    IN ULONG Protect
    );

ULONG
MiIsEntireRangeCommitted (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    );

ULONG
MiIsEntireRangeDecommitted (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    );

//++
//PMMPTE
//MiGetProtoPteAddress (
//    IN PMMPTE VAD,
//    IN PVOID VA
//    );
//
// Routine Description:
//
//    MiGetProtoPteAddress returns a pointer to the prototype PTE which
//    is mapped by the given virtual address descriptor and address within
//    the virtual address descriptor.
//
// Arguments
//
//    VAD - Supplies a pointer to the virtual address descriptor that contains
//          the VA.
//
//    VPN - Supplies the virtual page number.
//
// Return Value:
//
//    A pointer to the proto PTE which corresponds to the VA.
//
//--


#define MiGetProtoPteAddress(VAD,VPN)                                        \
    ((((((VPN) - (VAD)->StartingVpn) << PTE_SHIFT) +                         \
      (ULONG_PTR)(VAD)->FirstPrototypePte) <= (ULONG_PTR)(VAD)->LastContiguousPte) ? \
    ((PMMPTE)(((((VPN) - (VAD)->StartingVpn) << PTE_SHIFT) +                 \
        (ULONG_PTR)(VAD)->FirstPrototypePte))) :                                  \
        MiGetProtoPteAddressExtended ((VAD),(VPN)))

PMMPTE
FASTCALL
MiGetProtoPteAddressExtended (
    IN PMMVAD Vad,
    IN ULONG_PTR Vpn
    );

PSUBSECTION
FASTCALL
MiLocateSubsection (
    IN PMMVAD Vad,
    IN ULONG_PTR Vpn
    );

VOID
MiInitializeSystemCache (
    IN ULONG MinimumWorkingSet,
    IN ULONG MaximumWorkingSet
    );

VOID
MiAdjustWorkingSetManagerParameters(
    BOOLEAN WorkStation
    );

//
// Section support
//

VOID
FASTCALL
MiInsertBasedSection (
    IN PSECTION Section
    );

NTSTATUS
MiMapViewOfPhysicalSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN ULONG ProtectionMask,
    IN ULONG_PTR ZeroBits,
    IN ULONG AllocationType,
    IN BOOLEAN WriteCombined,
    OUT PBOOLEAN ReleasedWsMutex
    );

VOID
MiRemoveImageSectionObject(
    IN PFILE_OBJECT File,
    IN PCONTROL_AREA ControlArea
    );

VOID
MiAddSystemPtes(
    IN PMMPTE StartingPte,
    IN ULONG  NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    );

VOID
MiRemoveMappedView (
    IN PEPROCESS CurrentProcess,
    IN PMMVAD Vad
    );

PVOID
MiFindEmptySectionBaseDown (
    IN ULONG SizeOfRange,
    IN PVOID HighestAddressToEndAt
    );

VOID
MiSegmentDelete (
    PSEGMENT Segment
    );

VOID
MiSectionDelete (
    PVOID Object
    );

VOID
MiDereferenceSegmentThread (
    IN PVOID StartContext
    );

NTSTATUS
MiCreateImageFileMap (
    IN PFILE_OBJECT File,
    OUT PSEGMENT *Segment
    );

NTSTATUS
MiCreateDataFileMap (
    IN PFILE_OBJECT File,
    OUT PSEGMENT *Segment,
    IN PUINT64 MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN ULONG IgnoreFileSizing
    );

NTSTATUS
MiCreatePagingFileMap (
    OUT PSEGMENT *Segment,
    IN PUINT64 MaximumSize,
    IN ULONG ProtectionMask,
    IN ULONG AllocationAttributes
    );

VOID
MiPurgeSubsectionInternal (
    IN PSUBSECTION Subsection,
    IN ULONG PteOffset
    );

VOID
MiPurgeImageSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process
    );

VOID
MiCleanSection (
    IN PCONTROL_AREA ControlArea,
    IN LOGICAL DirtyDataPagesOk
    );

VOID
MiCheckControlArea (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS CurrentProcess,
    IN KIRQL PreviousIrql
    );

LOGICAL
MiCheckPurgeAndUpMapCount (
    IN PCONTROL_AREA ControlArea
    );

VOID
MiCheckForControlAreaDeletion (
    IN PCONTROL_AREA ControlArea
    );

BOOLEAN
MiCheckControlAreaStatus (
    IN SECTION_CHECK_TYPE SectionCheckType,
    IN PSECTION_OBJECT_POINTERS SectionObjectPointers,
    IN ULONG DelayClose,
    OUT PCONTROL_AREA *ControlArea,
    OUT PKIRQL OldIrql
    );

PEVENT_COUNTER
MiGetEventCounter (
    );

VOID
MiFlushEventCounter (
    );

VOID
MiFreeEventCounter (
    IN PEVENT_COUNTER Support,
    IN ULONG Flush
    );

ULONG
MiCanFileBeTruncatedInternal (
    IN PSECTION_OBJECT_POINTERS SectionPointer,
    IN PLARGE_INTEGER NewFileSize OPTIONAL,
    IN LOGICAL BlockNewViews,
    OUT PKIRQL PreviousIrql
    );

#define STATUS_MAPPED_WRITER_COLLISION (0xC0033333)

NTSTATUS
MiFlushSectionInternal (
    IN PMMPTE StartingPte,
    IN PMMPTE FinalPte,
    IN PSUBSECTION FirstSubsection,
    IN PSUBSECTION LastSubsection,
    IN ULONG Synchronize,
    IN LOGICAL WriteInProgressOk,
    OUT PIO_STATUS_BLOCK IoStatus
    );

//
// protection stuff...
//

NTSTATUS
MiProtectVirtualMemory (
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PSIZE_T CapturedRegionSize,
    IN ULONG Protect,
    IN PULONG LastProtect
    );

ULONG
MiGetPageProtection (
    IN PMMPTE PointerPte,
    IN PEPROCESS Process
    );

ULONG
MiSetProtectionOnSection (
    IN PEPROCESS Process,
    IN PMMVAD Vad,
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN ULONG NewProtect,
    OUT PULONG CapturedOldProtect,
    IN ULONG DontCharge
    );

NTSTATUS
MiCheckSecuredVad (
    IN PMMVAD Vad,
    IN PVOID Base,
    IN ULONG_PTR Size,
    IN ULONG ProtectionMask
    );

ULONG
MiChangeNoAccessForkPte (
    IN PMMPTE PointerPte,
    IN ULONG ProtectionMask
    );

VOID
MiSetImageProtect (
    IN PSEGMENT Segment,
    IN ULONG Protection
    );

//
// Routines for charging quota and commitment.
//

ULONG
FASTCALL
MiChargePageFileQuota (
    IN SIZE_T QuotaCharge,
    IN PEPROCESS CurrentProcess
    );

VOID
MiReturnPageFileQuota (
    IN SIZE_T QuotaCharge,
    IN PEPROCESS CurrentProcess
    );

LOGICAL
FASTCALL
MiChargeCommitment (
    IN SIZE_T QuotaCharge,
    IN PEPROCESS Process OPTIONAL
    );

LOGICAL
FASTCALL
MiChargeCommitmentCantExpand (
    IN SIZE_T QuotaCharge,
    IN ULONG MustSucceed
    );

VOID
FASTCALL
MiReturnCommitment (
    IN SIZE_T QuotaCharge
    );

extern SIZE_T MmPeakCommitment;

extern SIZE_T MmTotalCommitLimitMaximum;

SIZE_T
MiCalculatePageCommitment (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad,
    IN PEPROCESS Process
    );

VOID
MiReturnPageTablePageCommitment (
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PEPROCESS CurrentProcess,
    IN PMMVAD PreviousVad,
    IN PMMVAD NextVad
    );

VOID
MiEmptyAllWorkingSets (
    VOID
    );

VOID
MiFlushAllPages (
    VOID
    );

VOID
MiModifiedPageWriterTimerDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

LONGLONG
MiStartingOffset(
    IN PSUBSECTION Subsection,
    IN PMMPTE PteAddress
    );

LARGE_INTEGER
MiEndingOffset(
    IN PSUBSECTION Subsection
    );

VOID
MiReloadBootLoadedDrivers (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

LOGICAL
MiInitializeLoadedModuleList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiEnableRandomSpecialPool (
    IN LOGICAL Enable
    );

LOGICAL
MiTriageSystem (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

LOGICAL
MiTriageAddDrivers (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

LOGICAL
MiTriageVerifyDriver (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

#if defined (_WIN64)
#define MM_SPECIAL_POOL_PTES ((1024 * 1024) / sizeof (MMPTE))
#else
#define MM_SPECIAL_POOL_PTES 25000
#endif

#define MI_SUSPECT_DRIVER_BUFFER_LENGTH 512

extern WCHAR MmVerifyDriverBuffer[];
extern ULONG MmVerifyDriverBufferLength;
extern ULONG MmVerifyDriverLevel;

extern LOGICAL MmDontVerifyRandomDrivers;
extern LOGICAL MmSnapUnloads;
extern LOGICAL MmProtectFreedNonPagedPool;
extern ULONG MmEnforceWriteProtection;
extern LOGICAL MmTrackLockedPages;
extern LOGICAL MmTrackPtes;

#define VI_POOL_FREELIST_END  ((ULONG_PTR)-1)

typedef struct _VI_POOL_ENTRY_INUSE {
    PVOID VirtualAddress;
    PVOID CallingAddress;
    SIZE_T NumberOfBytes;
    ULONG_PTR Tag;
} VI_POOL_ENTRY_INUSE, *PVI_POOL_ENTRY_INUSE;

typedef struct _VI_POOL_ENTRY {
    union {
        VI_POOL_ENTRY_INUSE InUse;
        ULONG_PTR FreeListNext;
    };
} VI_POOL_ENTRY, *PVI_POOL_ENTRY;

#define MI_VERIFIER_ENTRY_SIGNATURE            0x98761940

typedef struct _MI_VERIFIER_DRIVER_ENTRY {
    LIST_ENTRY Links;
    ULONG Loads;
    ULONG Unloads;

    UNICODE_STRING BaseName;
    PVOID StartAddress;
    PVOID EndAddress;

#define VI_VERIFYING_DIRECTLY   0x1
#define VI_VERIFYING_INVERSELY  0x2

    ULONG Flags;
    ULONG_PTR Signature;
    ULONG_PTR Reserved;
    KSPIN_LOCK VerifierPoolLock;

    PVI_POOL_ENTRY PoolHash;
    ULONG_PTR PoolHashSize;
    ULONG_PTR PoolHashFree;
    ULONG_PTR PoolHashReserved;

    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;
    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;

    SIZE_T PagedBytes;
    SIZE_T NonPagedBytes;
    SIZE_T PeakPagedBytes;
    SIZE_T PeakNonPagedBytes;

} MI_VERIFIER_DRIVER_ENTRY, *PMI_VERIFIER_DRIVER_ENTRY;

typedef struct _MI_VERIFIER_POOL_HEADER {
    ULONG_PTR ListIndex;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
} MI_VERIFIER_POOL_HEADER, *PMI_VERIFIER_POOL_HEADER;

typedef struct _MM_DRIVER_VERIFIER_DATA {
    ULONG Level;
    ULONG RaiseIrqls;
    ULONG AcquireSpinLocks;
    ULONG SynchronizeExecutions;

    ULONG AllocationsAttempted;
    ULONG AllocationsSucceeded;
    ULONG AllocationsSucceededSpecialPool;
    ULONG AllocationsWithNoTag;

    ULONG TrimRequests;
    ULONG Trims;
    ULONG AllocationsFailed;
    ULONG AllocationsFailedDeliberately;

    ULONG Loads;
    ULONG Unloads;
    ULONG UnTrackedPool;
    ULONG Fill;

    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;
    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;

    SIZE_T PagedBytes;
    SIZE_T NonPagedBytes;
    SIZE_T PeakPagedBytes;
    SIZE_T PeakNonPagedBytes;

} MM_DRIVER_VERIFIER_DATA, *PMM_DRIVER_VERIFIER_DATA;

LOGICAL
MiInitializeDriverVerifierList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

LOGICAL
MiApplyDriverVerifier (
    IN PLDR_DATA_TABLE_ENTRY,
    IN PMI_VERIFIER_DRIVER_ENTRY Verifier
    );

VOID
MiVerifyingDriverUnloading (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

VOID
MiVerifierCheckThunks (
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

extern ULONG MiActiveVerifierThunks;
extern LIST_ENTRY MiSuspectDriverList;

extern LOGICAL KernelVerifier;

extern MM_DRIVER_VERIFIER_DATA MmVerifierData;

VOID
MiEnableKernelVerifier (
    VOID
    );

#if 0

#define MM_COMMIT_COUNTER_MAX 70

#define MM_TRACK_COMMIT(_index, bump) \
    if (_index >= MM_COMMIT_COUNTER_MAX) { \
        DbgPrint("Mm: Invalid commit counter %d %d\n", _index, MM_COMMIT_COUNTER_MAX); \
        DbgBreakPoint(); \
    } \
    else { \
        MmTrackCommit[_index] += (bump); \
    }

extern SIZE_T MmTrackCommit[MM_COMMIT_COUNTER_MAX];

#else

#define MM_TRACK_COMMIT(_index, bump)

#endif

#define MI_FREED_SPECIAL_POOL_SIGNATURE 0x98764321

#define MI_STACK_BYTES 1024

typedef struct _MI_FREED_SPECIAL_POOL {
    POOL_HEADER OverlaidPoolHeader;
    MI_VERIFIER_POOL_HEADER OverlaidVerifierPoolHeader;

    ULONG Signature;
    ULONG TickCount;
    ULONG NumberOfBytesRequested;
    ULONG Pagable;

    PVOID VirtualAddress;
    PVOID StackPointer;
    ULONG StackBytes;
    PETHREAD Thread;

    UCHAR StackData[MI_STACK_BYTES];
} MI_FREED_SPECIAL_POOL, *PMI_FREED_SPECIAL_POOL;

#define MM_DBG_COMMIT_NONPAGED_POOL_EXPANSION           0
#define MM_DBG_COMMIT_PAGED_POOL_PAGETABLE              1
#define MM_DBG_COMMIT_PAGED_POOL_PAGES                  2
#define MM_DBG_COMMIT_SESSION_POOL_PAGE_TABLES          3
#define MM_DBG_COMMIT_ALLOCVM1                          4
#define MM_DBG_COMMIT_ALLOCVM2                          5
#define MM_DBG_COMMIT_IMAGE                             6
#define MM_DBG_COMMIT_PAGEFILE_BACKED_SHMEM             7
#define MM_DBG_COMMIT_INDEPENDENT_PAGES                 8
#define MM_DBG_COMMIT_CONTIGUOUS_PAGES                  9
#define MM_DBG_COMMIT_MDL_PAGES                         10
#define MM_DBG_COMMIT_NONCACHED_PAGES                   11
#define MM_DBG_COMMIT_MAPVIEW_DATA                      12
#define MM_DBG_COMMIT_FILL_SYSTEM_DIRECTORY             13
#define MM_DBG_COMMIT_EXTRA_SYSTEM_PTES                 14
#define MM_DBG_COMMIT_DRIVER_PAGING_AT_INIT             15
#define MM_DBG_COMMIT_PAGEFILE_FULL                     16
#define MM_DBG_COMMIT_PROCESS_CREATE                    17
#define MM_DBG_COMMIT_KERNEL_STACK_CREATE               18
#define MM_DBG_COMMIT_SET_PROTECTION                    19
#define MM_DBG_COMMIT_SESSION_CREATE                    20
#define MM_DBG_COMMIT_SESSION_IMAGE_PAGES               21
#define MM_DBG_COMMIT_SESSION_PAGETABLE_PAGES           22
#define MM_DBG_COMMIT_SESSION_SHARED_IMAGE              23
#define MM_DBG_COMMIT_DRIVER_PAGES                      24
#define MM_DBG_COMMIT_INSERT_VAD                        25
#define MM_DBG_COMMIT_SESSION_WS_INIT                   26
#define MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_PAGES       27
#define MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_HASHPAGES   28

#define MM_DBG_COMMIT_RETURN_NONPAGED_POOL_EXPANSION    29
#define MM_DBG_COMMIT_RETURN_PAGED_POOL_PAGES           30
#define MM_DBG_COMMIT_RETURN_SESSION_POOL_PAGE_TABLES   31
#define MM_DBG_COMMIT_RETURN_ALLOCVM1                   32
#define MM_DBG_COMMIT_RETURN_ALLOCVM2                   33
#define MM_DBG_COMMIT_RETURN_ALLOCVM3                   34
#define MM_DBG_COMMIT_RETURN_IMAGE_NO_LARGE_CA          35
#define MM_DBG_COMMIT_RETURN_PAGEFILE_BACKED_SHMEM      36
#define MM_DBG_COMMIT_RETURN_NTFREEVM1                  37
#define MM_DBG_COMMIT_RETURN_NTFREEVM2                  38
#define MM_DBG_COMMIT_RETURN_NTFREEVM3                  39
#define MM_DBG_COMMIT_RETURN_NTFREEVM4                  40
#define MM_DBG_COMMIT_RETURN_MDL_PAGES                  41
#define MM_DBG_COMMIT_RETURN_NONCACHED_PAGES            42
#define MM_DBG_COMMIT_RETURN_MAPVIEW_DATA               43
#define MM_DBG_COMMIT_RETURN_PAGETABLES                 44
#define MM_DBG_COMMIT_RETURN_PROTECTION                 45
#define MM_DBG_COMMIT_RETURN_SEGMENT_DELETE1            46
#define MM_DBG_COMMIT_RETURN_SEGMENT_DELETE2            47
#define MM_DBG_COMMIT_RETURN_SESSION_CREATE_FAILURE1    48
#define MM_DBG_COMMIT_RETURN_SESSION_DEREFERENCE        49
#define MM_DBG_COMMIT_RETURN_SESSION_IMAGE_FAILURE1     50
#define MM_DBG_COMMIT_RETURN_SESSION_PAGETABLE_PAGES    51
#define MM_DBG_COMMIT_RETURN_VAD                        52
#define MM_DBG_COMMIT_RETURN_SESSION_WSL_FAILURE        54
#define MM_DBG_COMMIT_RETURN_PROCESS_CREATE_FAILURE1    55
#define MM_DBG_COMMIT_RETURN_PROCESS_DELETE             56
#define MM_DBG_COMMIT_RETURN_PROCESS_CLEAN_PAGETABLES   57
#define MM_DBG_COMMIT_RETURN_KERNEL_STACK_FAILURE1      58
#define MM_DBG_COMMIT_RETURN_KERNEL_STACK_FAILURE2      59
#define MM_DBG_COMMIT_RETURN_KERNEL_STACK_DELETE        60
#define MM_DBG_COMMIT_RETURN_SESSION_DRIVER_LOAD_FAILURE1 61
#define MM_DBG_COMMIT_RETURN_DRIVER_INIT_CODE           62
#define MM_DBG_COMMIT_RETURN_DRIVER_UNLOAD              63
#define MM_DBG_COMMIT_RETURN_DRIVER_UNLOAD1             64


#if 1   // LWFIX - TEMP TEMP TEMP

#define MM_BUMP_COUNTER_MAX 60

#define MM_BUMP_COUNTER(_index, bump) \
    if (_index >= MM_BUMP_COUNTER_MAX) { \
        DbgPrint("Mm: Invalid bump counter %d %d\n", _index, MM_BUMP_COUNTER_MAX); \
        DbgBreakPoint(); \
    } \
    else { \
        MmResTrack[_index] += (bump); \
    }
#else

#define MM_BUMP_COUNTER(_index, bump)

#endif

extern ULONG MiSpecialPagesNonPaged;
extern ULONG MiSpecialPagesNonPagedMaximum;

extern ULONG MmLockPagesPercentage;

//++
//PFN_NUMBER
//MI_NONPAGABLE_MEMORY_AVAILABLE(
//    VOID
//    );
//
// Routine Description:
//
//    This routine lets callers know how many pages can be charged against
//    the resident available, factoring in earlier Mm promises that
//    may not have been redeemed at this point (ie: nonpaged pool expansion,
//    etc, that must be honored at a later point if requested).
//
// Arguments
//
//    None.
//
// Return Value:
//
//    The number of currently available pages in the resident available.
//
//    N.B.  This is a signed quantity and can be negative.
//
//--
#define MI_NONPAGABLE_MEMORY_AVAILABLE()                                    \
        ((SPFN_NUMBER)                                                      \
            (MmResidentAvailablePages -                                     \
             MmTotalFreeSystemPtes[NonPagedPoolExpansion] -                 \
             (MiSpecialPagesNonPagedMaximum - MiSpecialPagesNonPaged) -     \
             MmSystemLockPagesCount))

extern ULONG MmLargePageMinimum;

//
// hack stuff for testing.
//

VOID
MiDumpValidAddresses (
    VOID
    );

VOID
MiDumpPfn ( VOID );

VOID
MiDumpWsl ( VOID );


VOID
MiFormatPte (
    IN PMMPTE PointerPte
    );

VOID
MiCheckPfn ( VOID );

VOID
MiCheckPte ( VOID );

VOID
MiFormatPfn (
    IN PMMPFN PointerPfn
    );




extern MMPTE ZeroPte;

extern MMPTE ZeroKernelPte;

extern MMPTE ValidKernelPteLocal;

extern MMPTE ValidKernelPte;

extern MMPTE ValidKernelPde;

extern MMPTE ValidKernelPdeLocal;

extern MMPTE ValidUserPte;

extern MMPTE ValidPtePte;

extern MMPTE ValidPdePde;

extern MMPTE DemandZeroPde;

extern MMPTE DemandZeroPte;

extern MMPTE KernelPrototypePte;

extern MMPTE TransitionPde;

extern MMPTE PrototypePte;

extern MMPTE NoAccessPte;

extern ULONG_PTR MmSubsectionBase;

extern ULONG_PTR MmSubsectionTopPage;

extern BOOLEAN MiHydra;

extern ULONG ExpMultiUserTS;

// extern MMPTE UserNoCommitPte;

//
// Virtual alignment for PTEs (machine specific) minimum value is
// 4k maximum value is 64k.  The maximum value can be raised by
// changing the MM_PROTO_PTE_ALIGNMENT constant and adding more
// reserved mapping PTEs in hyperspace.
//

//
// Total number of physical pages on the system.
//

extern PFN_COUNT MmNumberOfPhysicalPages;

//
// Lowest physical page number on the system.
//

extern PFN_NUMBER MmLowestPhysicalPage;

//
// Highest physical page number on the system.
//

extern PFN_NUMBER MmHighestPhysicalPage;

//
// Highest possible physical page number in the system.
//

extern PFN_NUMBER MmHighestPossiblePhysicalPage;

//
// Total number of available pages on the system.  This
// is the sum of the pages on the zeroed, free and standby lists.
//

extern PFN_COUNT MmAvailablePages;

//
// Total number of free pages to base working set trimming on.
//

extern PFN_NUMBER MmMoreThanEnoughFreePages;

//
// Total number physical pages which would be usable if every process
// was at it's minimum working set size.  This value is initialized
// at system initialization to MmAvailablePages - MM_FLUID_PHYSICAL_PAGES.
// Everytime a thread is created, the kernel stack is subtracted from
// this and every time a process is created, the minimum working set
// is subtracted from this.  If the value would become negative, the
// operation (create process/kernel stack/ adjust working set) fails.
// The PFN LOCK must be owned to manipulate this value.
//

extern SPFN_NUMBER MmResidentAvailablePages;

//
// The total number of pages which would be removed from working sets
// if every working set was at its minimum.
//

extern PFN_NUMBER MmPagesAboveWsMinimum;

//
// The total number of pages which would be removed from working sets
// if every working set above its maximum was at its maximum.
//

extern PFN_NUMBER MmPagesAboveWsMaximum;

//
// If memory is becoming short and MmPagesAboveWsMinimum is
// greater than MmPagesAboveWsThreshold, trim working sets.
//

extern PFN_NUMBER MmPagesAboveWsThreshold;

//
// The number of pages to add to a working set if there are ample
// available pages and the working set is below its maximum.
//

extern PFN_NUMBER MmWorkingSetSizeIncrement;

//
// The number of pages to extend the maximum working set size by
// if the working set at its maximum and there are ample available pages.

extern PFN_NUMBER MmWorkingSetSizeExpansion;

//
// The number of pages required to be freed by working set reduction
// before working set reduction is attempted.
//

extern PFN_NUMBER MmWsAdjustThreshold;

//
// The number of pages available to allow the working set to be
// expanded above its maximum.
//

extern PFN_NUMBER MmWsExpandThreshold;

//
// The total number of pages to reduce by working set trimming.
//

extern PFN_NUMBER MmWsTrimReductionGoal;

extern ULONG MiDelayPageFaults;

extern PMMPFN MmPfnDatabase;

extern MMPFNLIST MmZeroedPageListHead;

extern MMPFNLIST MmFreePageListHead;

extern MMPFNLIST MmStandbyPageListHead;

extern MMPFNLIST MmModifiedPageListHead;

extern MMPFNLIST MmModifiedNoWritePageListHead;

extern MMPFNLIST MmBadPageListHead;

extern PMMPFNLIST MmPageLocationList[NUMBER_OF_PAGE_LISTS];

extern MMPFNLIST MmModifiedPageListByColor[MM_MAXIMUM_NUMBER_OF_COLORS];

extern ULONG MmModNoWriteInsert;

//
// Event for available pages, set means pages are available.
//

extern KEVENT MmAvailablePagesEvent;

extern KEVENT MmAvailablePagesEventHigh;

//
// Event for the zeroing page thread.
//

extern KEVENT MmZeroingPageEvent;

//
// Boolean to indicate if the zeroing page thread is currently
// active.  This is set to true when the zeroing page event is
// set and set to false when the zeroing page thread is done
// zeroing all the pages on the free list.
//

extern BOOLEAN MmZeroingPageThreadActive;

//
// Minimum number of free pages before zeroing page thread starts.
//

extern PFN_NUMBER MmMinimumFreePagesToZero;

//
// Global event to synchronize mapped writing with cleaning segments.
//

extern KEVENT MmMappedFileIoComplete;

//
// Hyper space items.
//

extern PMMPTE MmFirstReservedMappingPte;

extern PMMPTE MmLastReservedMappingPte;

//
// System space sizes - MmNonPagedSystemStart to MM_NON_PAGED_SYSTEM_END
// defines the ranges of PDEs which must be copied into a new process's
// address space.
//

extern PVOID MmNonPagedSystemStart;

extern PCHAR MmSystemSpaceViewStart;

extern LOGICAL MmProtectFreedNonPagedPool;

//
// Pool sizes.
//

extern SIZE_T MmSizeOfNonPagedPoolInBytes;

extern SIZE_T MmMinimumNonPagedPoolSize;

extern SIZE_T MmDefaultMaximumNonPagedPool;

extern ULONG MmMinAdditionNonPagedPoolPerMb;

extern ULONG MmMaxAdditionNonPagedPoolPerMb;

extern SIZE_T MmSizeOfPagedPoolInBytes;

extern SIZE_T MmMaximumNonPagedPoolInBytes;

extern PFN_NUMBER MmAllocatedNonPagedPool;

extern PFN_NUMBER MmSizeOfNonPagedMustSucceed;

extern PVOID MmNonPagedPoolExpansionStart;

extern ULONG MmExpandedPoolBitPosition;

extern PFN_NUMBER MmNumberOfFreeNonPagedPool;

extern ULONG MmMustSucceedPoolBitPosition;

extern ULONG MmNumberOfSystemPtes;

extern ULONG MiRequestedSystemPtes;

extern ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];

extern SIZE_T MmLockPagesLimit;

extern PMMPTE MmSystemPagePtes;

#if !defined (_X86PAE_)
extern ULONG MmSystemPageDirectory;
#else
extern ULONG MmSystemPageDirectory[];
#endif

extern PMMPTE MmPagedPoolBasePde;

extern SIZE_T MmHeapSegmentReserve;

extern SIZE_T MmHeapSegmentCommit;

extern SIZE_T MmHeapDeCommitTotalFreeThreshold;

extern SIZE_T MmHeapDeCommitFreeBlockThreshold;

#define MI_MAX_FREE_LIST_HEADS  4

extern LIST_ENTRY MmNonPagedPoolFreeListHead[MI_MAX_FREE_LIST_HEADS];

//
// Counter for flushes of the entire TB.
//

extern ULONG MmFlushCounter;

//
// Pool start and end.
//

extern PVOID MmNonPagedPoolStart;

extern PVOID MmNonPagedPoolEnd;

extern PVOID MmPagedPoolStart;

extern PVOID MmPagedPoolEnd;

extern PVOID MmNonPagedMustSucceed;

//
// Pool bit maps and other related structures.
//

typedef struct _MM_PAGED_POOL_INFO {

    PRTL_BITMAP PagedPoolAllocationMap;
    PRTL_BITMAP EndOfPagedPoolBitmap;
    PRTL_BITMAP PagedPoolLargeSessionAllocationMap;     // HYDRA only
    PMMPTE FirstPteForPagedPool;
    PMMPTE LastPteForPagedPool;
    PMMPTE NextPdeForPagedPoolExpansion;
    ULONG PagedPoolHint;
    SIZE_T PagedPoolCommit;
    SIZE_T AllocatedPagedPool;

} MM_PAGED_POOL_INFO, *PMM_PAGED_POOL_INFO;

extern MM_PAGED_POOL_INFO MmPagedPoolInfo;

extern PVOID MmPageAlignedPoolBase[2];

extern PRTL_BITMAP VerifierLargePagedPoolMap;

//
// MmFirstFreeSystemPte contains the offset from the
// Nonpaged system base to the first free system PTE.
// Note, that an offset of zero indicates an empty list.
//

extern MMPTE MmFirstFreeSystemPte[MaximumPtePoolTypes];

extern ULONG_PTR MiSystemViewStart;

//
// System cache sizes.
//

//extern MMSUPPORT MmSystemCacheWs;

extern PMMWSL MmSystemCacheWorkingSetList;

extern PMMWSLE MmSystemCacheWsle;

extern PVOID MmSystemCacheStart;

extern PVOID MmSystemCacheEnd;

extern PRTL_BITMAP MmSystemCacheAllocationMap;

extern PRTL_BITMAP MmSystemCacheEndingMap;

extern PFN_NUMBER MmSystemCacheWsMinimum;

extern PFN_NUMBER MmSystemCacheWsMaximum;

//
// Virtual alignment for PTEs (machine specific) minimum value is
// 0 (no alignment) maximum value is 64k.  The maximum value can be raised by
// changing the MM_PROTO_PTE_ALIGNMENT constant and adding more
// reserved mapping PTEs in hyperspace.
//

extern ULONG MmAliasAlignment;

//
// Mask to AND with virtual address to get an offset to go
// with the alignment.  This value is page aligned.
//

extern ULONG MmAliasAlignmentOffset;

//
// Mask to and with PTEs to determine if the alias mapping is compatible.
// This value is usually (MmAliasAlignment - 1)
//

extern ULONG MmAliasAlignmentMask;

//
// Cells to track unused thread kernel stacks to avoid TB flushes
// every time a thread terminates.
//

extern ULONG MmNumberDeadKernelStacks;
extern ULONG MmMaximumDeadKernelStacks;
extern PMMPFN MmFirstDeadKernelStack;

//
// MmSystemPteBase contains the address of 1 PTE before
// the first free system PTE (zero indicates an empty list).
// The value of this field does not change once set.
//

extern PMMPTE MmSystemPteBase;

extern PMMWSL MmWorkingSetList;

extern PMMWSLE MmWsle;

//
// Root of system space virtual address descriptors.  These define
// the pagable portion of the system.
//

extern PMMVAD MmVirtualAddressDescriptorRoot;

extern PMMADDRESS_NODE MmSectionBasedRoot;

extern PVOID MmHighSectionBase;

//
// Section commit mutex.
//

extern FAST_MUTEX MmSectionCommitMutex;

//
// Section base address mutex.
//

extern FAST_MUTEX MmSectionBasedMutex;

//
// Resource for section extension.
//

extern ERESOURCE MmSectionExtendResource;
extern ERESOURCE MmSectionExtendSetResource;

//
// Event to synchronize threads within process mapping images via hyperspace.
//

extern KEVENT MmImageMappingPteEvent;

//
// Inpage cluster sizes for executable pages (set based on memory size).
//

extern ULONG MmDataClusterSize;

extern ULONG MmCodeClusterSize;

//
// Pagefile creation mutex.
//

extern FAST_MUTEX MmPageFileCreationLock;

//
// Event to set when first paging file is created.
//

extern PKEVENT MmPagingFileCreated;

//
// Paging file debug information.
//

extern ULONG_PTR MmPagingFileDebug[];

//
// Spinlock which guards PFN database.  This spinlock is used by
// memory management for accessing the PFN database.  The I/O
// system makes use of it for unlocking pages during I/O complete.
//

extern KSPIN_LOCK MmPfnLock;

//
// Spinlock which guards the working set list for the system shared
// address space (paged pool, system cache, pagable drivers).
//

extern ERESOURCE MmSystemWsLock;

//
// Spin lock for allocating non-paged PTEs from system space.
//

extern KSPIN_LOCK MmSystemSpaceLock;

//
// Spin lock for operating on page file commit charges.
//

extern KSPIN_LOCK MmChargeCommitmentLock;

//
// Spin lock for allowing working set expansion.
//

extern KSPIN_LOCK MmExpansionLock;

//
// To prevent optimizations.
//

extern MMPTE GlobalPte;

//
// Page color for system working set.
//

extern ULONG MmSystemPageColor;

extern ULONG MmSecondaryColors;

extern ULONG MmProcessColorSeed;

//
// Set from ntos\config\CMDAT3.C  Used by customers to disable paging
// of executive on machines with lots of memory.  Worth a few TPS on a
// data base server.
//

extern ULONG MmDisablePagingExecutive;


//
// For debugging.


#if DBG
extern ULONG MmDebug;
#endif

//
// Unused segment management
//

extern MMDEREFERENCE_SEGMENT_HEADER MmDereferenceSegmentHeader;

extern LIST_ENTRY MmUnusedSegmentList;

extern KEVENT MmUnusedSegmentCleanup;

extern SIZE_T MmMaxUnusedSegmentPagedPoolUsage;

extern SIZE_T MmUnusedSegmentPagedPoolUsage;
extern SIZE_T MiUnusedSegmentPagedPoolUsage;

extern SIZE_T MmUnusedSegmentPagedPoolReduction;

extern SIZE_T MmMaxUnusedSegmentNonPagedPoolUsage;

extern SIZE_T MmUnusedSegmentNonPagedPoolUsage;
extern SIZE_T MiUnusedSegmentNonPagedPoolUsage;

extern SIZE_T MmUnusedSegmentNonPagedPoolReduction;

extern ULONG MmUnusedSegmentTrimLevel;

extern ULONG MmUnusedSegmentCount;

#define MI_UNUSED_SEGMENTS_COUNT_UPDATE(_count) \
        MmUnusedSegmentCount += (_count);

#define MI_FILESYSTEM_NONPAGED_POOL_CHARGE 150

#define MI_FILESYSTEM_PAGED_POOL_CHARGE 1024

//++
//LOGICAL
//MI_UNUSED_SEGMENTS_SURPLUS (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    This routine determines whether a surplus of unused
//    segments exist.  If so, the caller can initiate a trim to free pool.
//
// Arguments
//
//    None.
//
// Return Value:
//
//    TRUE if unused segment trimming should be initiated, FALSE if not.
//
//--
#define MI_UNUSED_SEGMENTS_SURPLUS()                                    \
        (MmUnusedSegmentPagedPoolUsage > MmMaxUnusedSegmentPagedPoolUsage) || \
        (MmUnusedSegmentNonPagedPoolUsage > MmMaxUnusedSegmentNonPagedPoolUsage)

//++
//VOID
//MI_UNUSED_SEGMENTS_INSERT_CHARGE (
//    IN PCONTROL_AREA _ControlArea
//    );
//
// Routine Description:
//
//    This routine manages pool charges during insertions of segments to
//    the unused segment list.
//
// Arguments
//
//    _ControlArea - Supplies the control area to obtain the pool charges from.
//
// Return Value:
//
//    None.
//
//--
#define MI_UNUSED_SEGMENTS_INSERT_CHARGE(_ControlArea)                       \
        {                                                                    \
           MM_PFN_LOCK_ASSERT();                                             \
           ASSERT (_ControlArea->PagedPoolUsage >= sizeof(SEGMENT));         \
           ASSERT (_ControlArea->NonPagedPoolUsage >= sizeof(CONTROL_AREA) + sizeof(SUBSECTION));          \
           ASSERT (MiUnusedSegmentNonPagedPoolUsage + _ControlArea->NonPagedPoolUsage > MiUnusedSegmentNonPagedPoolUsage); \
           ASSERT (MiUnusedSegmentPagedPoolUsage + _ControlArea->PagedPoolUsage > MiUnusedSegmentPagedPoolUsage); \
           MiUnusedSegmentPagedPoolUsage += _ControlArea->PagedPoolUsage;    \
           MiUnusedSegmentNonPagedPoolUsage += _ControlArea->NonPagedPoolUsage;\
           MmUnusedSegmentPagedPoolUsage += (_ControlArea->PagedPoolUsage + MI_FILESYSTEM_PAGED_POOL_CHARGE);    \
           MmUnusedSegmentNonPagedPoolUsage += (_ControlArea->NonPagedPoolUsage + MI_FILESYSTEM_NONPAGED_POOL_CHARGE);\
           MI_UNUSED_SEGMENTS_COUNT_UPDATE(1); \
        }

//++
//VOID
//MI_UNUSED_SEGMENTS_REMOVE_CHARGE (
//    IN PCONTROL_AREA _ControlArea
//    );
//
// Routine Description:
//
//    This routine manages pool charges during removals of segments from
//    the unused segment list.
//
// Arguments
//
//    _ControlArea - Supplies the control area to obtain the pool charges from.
//
// Return Value:
//
//    None.
//
//--
#define MI_UNUSED_SEGMENTS_REMOVE_CHARGE(_ControlArea)                       \
        {                                                                    \
           MM_PFN_LOCK_ASSERT();                                             \
           ASSERT (_ControlArea->PagedPoolUsage >= sizeof(SEGMENT));         \
           ASSERT (_ControlArea->NonPagedPoolUsage >= sizeof(CONTROL_AREA) + sizeof(SUBSECTION));          \
           ASSERT (MmUnusedSegmentNonPagedPoolUsage - _ControlArea->NonPagedPoolUsage < MmUnusedSegmentNonPagedPoolUsage); \
           ASSERT (MmUnusedSegmentPagedPoolUsage - _ControlArea->PagedPoolUsage < MmUnusedSegmentPagedPoolUsage); \
           MiUnusedSegmentPagedPoolUsage -= _ControlArea->PagedPoolUsage;    \
           MiUnusedSegmentNonPagedPoolUsage -= _ControlArea->NonPagedPoolUsage;\
           MmUnusedSegmentPagedPoolUsage -= (_ControlArea->PagedPoolUsage + MI_FILESYSTEM_PAGED_POOL_CHARGE);    \
           MmUnusedSegmentNonPagedPoolUsage -= (_ControlArea->NonPagedPoolUsage + MI_FILESYSTEM_NONPAGED_POOL_CHARGE);\
           MI_UNUSED_SEGMENTS_COUNT_UPDATE(-1); \
        }

//
// List heads
//

extern MMWORKING_SET_EXPANSION_HEAD MmWorkingSetExpansionHead;

extern MMPAGE_FILE_EXPANSION MmAttemptForCantExtend;

//
// Paging files
//

extern MMMOD_WRITER_LISTHEAD MmPagingFileHeader;

extern MMMOD_WRITER_LISTHEAD MmMappedFileHeader;

extern PMMPAGING_FILE MmPagingFile[MAX_PAGE_FILES];

#define MM_MAPPED_FILE_MDLS 4


extern PMMMOD_WRITER_MDL_ENTRY MmMappedFileMdl[MM_MAPPED_FILE_MDLS];

extern LIST_ENTRY MmFreePagingSpaceLow;

extern ULONG MmNumberOfActiveMdlEntries;

extern ULONG MmNumberOfPagingFiles;

extern KEVENT MmModifiedPageWriterEvent;

extern KEVENT MmCollidedFlushEvent;

extern KEVENT MmCollidedLockEvent;

//
// Total number of committed pages.
//

extern SIZE_T MmTotalCommittedPages;

extern SIZE_T MmTotalCommitLimit;

extern SIZE_T MmOverCommit;

extern SIZE_T MmSharedCommit;

//
// Modified page writer.
//

extern PFN_NUMBER MmMinimumFreePages;

extern PFN_NUMBER MmFreeGoal;

extern PFN_NUMBER MmModifiedPageMaximum;

extern PFN_NUMBER MmModifiedPageMinimum;

extern ULONG MmModifiedWriteClusterSize;

extern ULONG MmMinimumFreeDiskSpace;

extern ULONG MmPageFileExtension;

extern ULONG MmMinimumPageFileReduction;

extern LARGE_INTEGER MiModifiedPageLife;

extern BOOLEAN MiTimerPending;

extern KEVENT MiMappedPagesTooOldEvent;

extern KDPC MiModifiedPageWriterTimerDpc;

extern KTIMER MiModifiedPageWriterTimer;

//
// System process working set sizes.
//

extern PFN_NUMBER MmSystemProcessWorkingSetMin;

extern PFN_NUMBER MmSystemProcessWorkingSetMax;

extern PFN_NUMBER MmMinimumWorkingSetSize;

//
// Support for debugger's mapping physical memory.
//

extern PMMPTE MmDebugPte;

extern PMMPTE MmCrashDumpPte;

extern ULONG MiOverCommitCallCount;

extern SIZE_T MmResTrack[MM_BUMP_COUNTER_MAX];

//
// Event tracing routines
//

extern PPAGE_FAULT_NOTIFY_ROUTINE MmPageFaultNotifyRoutine;
extern PHARD_FAULT_NOTIFY_ROUTINE MmHardFaultNotifyRoutine;

//
// This is a special value loaded into an EPROCESS pointer to indicate that
// the action underway is for a Hydra session, not really the current process.
// (Any value could be used here that is not a valid system pointer or NULL).
//

#define HYDRA_PROCESS   ((PEPROCESS)1)

/*++

  Virtual Memory Layout of the 48MB Session space when loaded at 0xA0000000

                 +------------------------------------+
        A0000000 |                                    |
                 | win32k.sys, video drivers and any  |
                 | rebased NT4 printer drivers.       |
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

--*/

extern ULONG_PTR MmSessionBase;

//
// Sizes of Session fields.
//

#define MI_SESSION_SPACE_STRUCT_SIZE MM_ALLOCATION_GRANULARITY

#define MI_SESSION_SPACE_WS_SIZE  (4*1024*1024 - MI_SESSION_SPACE_STRUCT_SIZE)

#define MI_SESSION_IMAGE_SIZE      (8*1024*1024)

#define MI_SESSION_VIEW_SIZE      (20*1024*1024)

#define MI_SESSION_POOL_SIZE      (16*1024*1024)

#define MM_SESSION_SPACE_DATA_SIZE PAGE_SIZE

#define MI_SESSION_SPACE_TOTAL_SIZE \
            (MI_SESSION_IMAGE_SIZE + \
             MI_SESSION_SPACE_STRUCT_SIZE + \
             MI_SESSION_SPACE_WS_SIZE + \
             MI_SESSION_VIEW_SIZE + \
             MI_SESSION_POOL_SIZE)

//
// Actually this is a little larger due to page table pages, etc, but this is
// all handled ok.
//

#define MI_SESSION_MAXIMUM_WORKING_SET \
       ((ULONG)(MI_SESSION_SPACE_TOTAL_SIZE >> PAGE_SHIFT))

//
// The number of page table pages required to map all of session space.
//

#define MI_SESSION_SPACE_PAGE_TABLES \
            (MI_SESSION_SPACE_TOTAL_SIZE / MM_VA_MAPPED_BY_PDE)


//
// Offsets from MmSessionBase.
//

#define MI_SESSION_IMAGE_OFFSET   (0)

#define MI_SESSION_DATA_OFFSET    (8*1024*1024)

#define MI_SESSION_WS_OFFSET      (8*1024*1024 + MI_SESSION_SPACE_STRUCT_SIZE)

#define MI_SESSION_VIEW_OFFSET    (12*1024*1024)

#define MI_SESSION_POOL_OFFSET    (32*1024*1024)



//
// Virtual addresses of various session fields.
//

#define MI_SESSION_IMAGE_START  ((ULONG_PTR)MmSessionBase)

#define MI_SESSION_SPACE_WS     (MmSessionBase + MI_SESSION_WS_OFFSET)

#define MI_SESSION_VIEW_START   (MmSessionBase + MI_SESSION_VIEW_OFFSET)

#define MI_SESSION_POOL         (MmSessionBase + MI_SESSION_POOL_OFFSET)

#define MI_SESSION_SPACE_END    ((ULONG_PTR)MmSessionBase +          \
                                        MI_SESSION_SPACE_TOTAL_SIZE)

//
// Macros to determine if a given address lies in the specified session range.
//

#define MI_IS_SESSION_IMAGE_ADDRESS(VirtualAddress) \
        (MiHydra == TRUE && (PVOID)(VirtualAddress) >= (PVOID)MI_SESSION_IMAGE_START && (PVOID)(VirtualAddress) < (PVOID)(MI_SESSION_IMAGE_START + MI_SESSION_IMAGE_SIZE))

#define MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) \
        (MiHydra == TRUE && (PVOID)(VirtualAddress) >= (PVOID)MI_SESSION_POOL && (PVOID)(VirtualAddress) < (PVOID)(MI_SESSION_POOL + MI_SESSION_POOL_SIZE))

#define MI_IS_SESSION_ADDRESS(_VirtualAddress) \
        (MiHydra == TRUE && (PVOID)(_VirtualAddress) >= (PVOID)MmSessionBase && (PVOID)(_VirtualAddress) < (PVOID)(MI_SESSION_SPACE_END))

#define MI_IS_SESSION_PTE(_Pte) \
        (MiHydra == TRUE && (PMMPTE)(_Pte) >= MiSessionBasePte && (PMMPTE)(_Pte) < MiSessionLastPte)


#define SESSION_GLOBAL(_Session)    (_Session->GlobalVirtualAddress)

#define MM_DBG_SESSION_INITIAL_PAGETABLE_ALLOC          0
#define MM_DBG_SESSION_INITIAL_PAGETABLE_FREE_RACE      1
#define MM_DBG_SESSION_INITIAL_PAGE_ALLOC               2
#define MM_DBG_SESSION_INITIAL_PAGE_FREE_FAIL1          3
#define MM_DBG_SESSION_INITIAL_PAGETABLE_FREE_FAIL1     4
#define MM_DBG_SESSION_WS_PAGE_FREE                     5
#define MM_DBG_SESSION_PAGETABLE_ALLOC                  6
#define MM_DBG_SESSION_SYSMAPPED_PAGES_ALLOC            7
#define MM_DBG_SESSION_WS_PAGETABLE_ALLOC               8
#define MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_ALLOC        9
#define MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_FREE_FAIL1   10
#define MM_DBG_SESSION_WS_PAGE_ALLOC                    11
#define MM_DBG_SESSION_WS_PAGE_ALLOC_GROWTH             12
#define MM_DBG_SESSION_INITIAL_PAGE_FREE                13
#define MM_DBG_SESSION_PAGETABLE_FREE                   14
#define MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_ALLOC1       15
#define MM_DBG_SESSION_DRIVER_PAGES_LOCKED              16
#define MM_DBG_SESSION_DRIVER_PAGES_UNLOCKED            17
#define MM_DBG_SESSION_WS_HASHPAGE_ALLOC                18
#define MM_DBG_SESSION_SYSMAPPED_PAGES_COMMITTED        19

#define MM_DBG_SESSION_COMMIT_PAGEDPOOL_PAGES           30
#define MM_DBG_SESSION_COMMIT_DELETE_VM_RETURN          31
#define MM_DBG_SESSION_COMMIT_POOL_FREED                32
#define MM_DBG_SESSION_COMMIT_IMAGE_UNLOAD              33
#define MM_DBG_SESSION_COMMIT_IMAGELOAD_FAILED1         34
#define MM_DBG_SESSION_COMMIT_IMAGELOAD_FAILED2         35
#define MM_DBG_SESSION_COMMIT_IMAGELOAD_NOACCESS        36

#if DBG
#define MM_SESS_COUNTER_MAX 40

#define MM_BUMP_SESS_COUNTER(_index, bump) \
    if (_index >= MM_SESS_COUNTER_MAX) { \
        DbgPrint("Mm: Invalid bump counter %d %d\n", _index, MM_SESS_COUNTER_MAX); \
        DbgBreakPoint(); \
    } \
    else if (MiHydra == TRUE) { \
        MmSessionSpace->Debug[_index] += (bump); \
    }

typedef struct _MM_SESSION_MEMORY_COUNTERS {
    SIZE_T NonPagablePages;
    SIZE_T CommittedPages;
} MM_SESSION_MEMORY_COUNTERS, *PMM_SESSION_MEMORY_COUNTERS;

#define MM_SESS_MEMORY_COUNTER_MAX  8

#define MM_SNAP_SESS_MEMORY_COUNTERS(_index) \
    if (_index >= MM_SESS_MEMORY_COUNTER_MAX) { \
        DbgPrint("Mm: Invalid session mem counter %d %d\n", _index, MM_SESS_MEMORY_COUNTER_MAX); \
        DbgBreakPoint(); \
    } \
    else { \
        MmSessionSpace->Debug2[_index].NonPagablePages = MmSessionSpace->NonPagablePages; \
        MmSessionSpace->Debug2[_index].CommittedPages = MmSessionSpace->CommittedPages; \
    }

#else
#define MM_BUMP_SESS_COUNTER(_index, bump)
#define MM_SNAP_SESS_MEMORY_COUNTERS(_index)
#endif


#define MM_SESSION_FAILURE_NO_IDS                   0
#define MM_SESSION_FAILURE_NO_COMMIT                1
#define MM_SESSION_FAILURE_NO_RESIDENT              2
#define MM_SESSION_FAILURE_RACE_DETECTED            3
#define MM_SESSION_FAILURE_NO_SYSPTES               4
#define MM_SESSION_FAILURE_NO_PAGED_POOL            5
#define MM_SESSION_FAILURE_NO_NONPAGED_POOL         6
#define MM_SESSION_FAILURE_NO_IMAGE_VA_SPACE        7
#define MM_SESSION_FAILURE_NO_SESSION_PAGED_POOL    8

#define MM_SESSION_FAILURE_CAUSES                   9

ULONG MmSessionFailureCauses[MM_SESSION_FAILURE_CAUSES];

#define MM_BUMP_SESSION_FAILURES(_index) MmSessionFailureCauses[_index] += 1;

//
// Note that modifying session space flag bits must be synchronized on Alpha.
//

typedef struct _MM_SESSION_SPACE_FLAGS {
    ULONG Initialized : 1;
    ULONG BeingDeleted : 1;
    ULONG WorkingSetInserted : 1;
    ULONG SessionListInserted : 1;
    ULONG HasWsLock : 1;
    ULONG DeletePending : 1;
    ULONG Filler : 26;
} MM_SESSION_SPACE_FLAGS;

//
// The session space data structure - allocated per session and only visible at
// MM_SESSION_SPACE_BASE when in the context of a process from the session.
// This virtual address space is rotated at context switch time when switching
// from a process in session A to a process in session B.  This rotation is
// useful for things like providing paged pool per session so many sessions
// won't exhaust the VA space which backs the system global pool.
//
// A kernel PTE is also allocated to double map this page so that global
// pointers can be maintained to provide system access from any process context.
// This is needed for things like mutexes and WSL chains.
//

typedef struct _MM_SESSION_SPACE {

    ULONG ReferenceCount;
    union {
        ULONG LongFlags;
        MM_SESSION_SPACE_FLAGS Flags;
    } u;
    ULONG SessionId;

    //
    // All the page tables for session space use this as their parent.
    // Note that it's not really a page directory - it's really a page
    // table page itself (the one used to map this very structure).
    //
    // This provides a reference to something that won't go away and
    // is relevant regardless of which process within the session is current.
    //

    PFN_NUMBER SessionPageDirectoryIndex;

    //
    // This is a pointer in global system address space, used to make various
    // fields that can be referenced from any process visible from any process
    // context.  This is for things like mutexes, WSL chains, etc.
    //

    struct _MM_SESSION_SPACE *GlobalVirtualAddress;

    KSPIN_LOCK SpinLock;

    //
    // This is the list of the processes in this group that have
    // session space entries.
    //

    LIST_ENTRY ProcessList;

    //
    // Pool allocation counts - these are always valid.
    //

    SIZE_T NonPagedPoolBytes;
    SIZE_T PagedPoolBytes;
    ULONG NonPagedPoolAllocations;
    ULONG PagedPoolAllocations;

    //
    // This is the count of non paged allocations to support this session
    // space.  This includes the session structure page table and data pages,
    // WSL page table and data pages, session pool page table pages and session
    // image page table pages.  These are all charged against
    // MmResidentAvailable.
    //

    SIZE_T NonPagablePages;

    //
    // This is the count of pages in this session that have been charged against
    // the systemwide commit.  This includes all the NonPagablePages plus the
    // data pages they typically map.
    //

    SIZE_T CommittedPages;

    LARGE_INTEGER LastProcessSwappedOutTime;

#if defined (_WIN64)

    //
    // The page directory that maps session space is saved here so
    // trimmers can attach.
    //

    MMPTE PageDirectory;

#else

    //
    // The second level page tables that map session space are shared
    // by all processes in the session.
    //

    MMPTE PageTables[MI_SESSION_SPACE_PAGE_TABLES];

#endif

    //
    // Session space paged pool support.
    //

    FAST_MUTEX PagedPoolMutex;

    //
    // Start of session paged pool virtual space.
    //

    PVOID PagedPoolStart;

    //
    // Current end of pool virtual space. Can be extended to the
    // end of the session space.
    //

    PVOID PagedPoolEnd;

    //
    // PTE pointers for pool.
    //

    PMMPTE PagedPoolBasePde;

    MM_PAGED_POOL_INFO PagedPoolInfo;

    ULONG Color;

    ULONG ProcessOutSwapCount;

    //
    // This is the list of system images currently valid in
    // this session space.  This information is in addition
    // to the module global information in PsLoadedModuleList.
    //

    LIST_ENTRY ImageList;

    //
    // The system PTE self-map entry.
    //

    PMMPTE GlobalPteEntry;

    ULONG CopyOnWriteCount;

    //
    // The count of "known attachers and the associated event.
    //

    ULONG AttachCount;

    KEVENT AttachEvent;

    PEPROCESS LastProcess;

    //
    // Working set information.
    //

    MMSUPPORT  Vm;
    PMMWSLE    Wsle;

    ERESOURCE  WsLock;         // owned by WorkingSetLockOwner

    //
    // This chain is in global system addresses (not session VAs) and can
    // be walked from any system context, ie: for WSL trimming.
    //

    LIST_ENTRY WsListEntry;

    //
    // Support for mapping system views into session space.  Each desktop
    // allocates a 3MB heap and the global system view space is only 48M
    // total.  This would limit us to only 20-30 users - rotating the
    // system view space with each session removes this limitation.
    //

    MMSESSION Session;

    //
    // This is the driver object entry for WIN32K.SYS
    //
    // It is not a real driver object, but contained here
    // for information such as the DriverUnload routine.
    //

    DRIVER_OBJECT Win32KDriverObject;

    PETHREAD WorkingSetLockOwner;

    //
    // Pool descriptor for less than 1 page allocations.
    //

    POOL_DESCRIPTOR PagedPool;

#if defined(_IA64_)
    REGION_MAP_INFO SessionMapInfo;
#endif

#if DBG
    PETHREAD SessionLockOwner;
    ULONG WorkingSetLockOwnerCount;

    ULONG_PTR Debug[MM_SESS_COUNTER_MAX];

    MM_SESSION_MEMORY_COUNTERS Debug2[MM_SESS_MEMORY_COUNTER_MAX];
#endif

} MM_SESSION_SPACE, *PMM_SESSION_SPACE;

extern PMM_SESSION_SPACE MmSessionSpace;

extern ULONG MiSessionCount;

//
// This could be improved to just flush the non-global TB entries.
//

#define MI_FLUSH_SESSION_TB(OLDIRQL) KeRaiseIrql (DISPATCH_LEVEL, &OLDIRQL); \
                                     KeFlushEntireTb (TRUE, TRUE); \
                                     KeLowerIrql (OLDIRQL);

#if DBG
#define MM_SET_SESSION_LOCK_OWNER()  ASSERT (MmSessionSpace->SessionLockOwner == NULL); \
                                  MmSessionSpace->SessionLockOwner = PsGetCurrentThread();

#define MM_CLEAR_SESSION_LOCK_OWNER()  ASSERT (MmSessionSpace->SessionLockOwner == PsGetCurrentThread()); \
                                    MmSessionSpace->SessionLockOwner = NULL;
#else
#define MM_SET_SESSION_LOCK_OWNER()
#define MM_CLEAR_SESSION_LOCK_OWNER()
#endif

#define LOCK_SESSION(OLDIRQL)   ExAcquireSpinLock (&((SESSION_GLOBAL(MmSessionSpace))->SpinLock), &OLDIRQL); \
      MM_SET_SESSION_LOCK_OWNER ();

#define UNLOCK_SESSION(OLDIRQL)    MM_CLEAR_SESSION_LOCK_OWNER (); \
    ExReleaseSpinLock (&((SESSION_GLOBAL(MmSessionSpace))->SpinLock), OLDIRQL);

//
// The default number of pages for the session working set minimum & maximum.
//

#define MI_SESSION_SPACE_WORKING_SET_MINIMUM 20

#define MI_SESSION_SPACE_WORKING_SET_MAXIMUM 384

NTSTATUS
MiSessionCommitPageTables(
    PVOID StartVa,
    PVOID EndVa
    );

NTSTATUS
MiInitializeSessionPool(
    VOID
    );

VOID
MiCheckSessionPoolAllocations(
    VOID
    );

VOID
MiFreeSessionPoolBitMaps(
    VOID
    );

VOID
MiDetachSession(
    VOID
    );

VOID
MiAttachSession(
    IN PMM_SESSION_SPACE SessionGlobal
    );

#define MM_SET_SESSION_RESOURCE_OWNER()                                 \
        ASSERT (MmSessionSpace->WorkingSetLockOwner == NULL);           \
        MmSessionSpace->WorkingSetLockOwner = PsGetCurrentThread();

#define MM_CLEAR_SESSION_RESOURCE_OWNER()                               \
        ASSERT (MmSessionSpace->WorkingSetLockOwner == PsGetCurrentThread()); \
        MmSessionSpace->WorkingSetLockOwner = NULL;

#define MM_SESSION_SPACE_WS_LOCK_ASSERT()                               \
        ASSERT (MmSessionSpace->WorkingSetLockOwner == PsGetCurrentThread())

#define LOCK_SESSION_SPACE_WS(OLDIRQL)                                  \
            ASSERT (KeGetCurrentIrql() <= APC_LEVEL);                   \
            KeRaiseIrql(APC_LEVEL,&OLDIRQL);                            \
            ExAcquireResourceExclusive(&MmSessionSpace->WsLock, TRUE);  \
            MM_SET_SESSION_RESOURCE_OWNER ();

#define UNLOCK_SESSION_SPACE_WS(OLDIRQL)                                 \
                            MM_CLEAR_SESSION_RESOURCE_OWNER ();          \
                            ExReleaseResource (&MmSessionSpace->WsLock); \
                            KeLowerIrql (OLDIRQL);                       \
                            ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

extern PMMPTE MiHighestUserPte;
extern PMMPTE MiHighestUserPde;

extern PMMPTE MiSessionBasePte;
extern PMMPTE MiSessionLastPte;

NTSTATUS
MiEmptyWorkingSet (
    IN PMMSUPPORT WsInfo,
    IN LOGICAL WaitOk
    );

//++
//ULONG
//MiGetPdeSessionIndex (
//    IN PVOID va
//    );
//
// Routine Description:
//
//    MiGetPdeSessionIndex returns the session structure index for the PDE
//    will (or does) map the given virtual address.
//
// Arguments
//
//    Va - Supplies the virtual address to locate the PDE index for.
//
// Return Value:
//
//    The index of the PDE entry.
//
//--

#define MiGetPdeSessionIndex(va)  ((ULONG)(((ULONG_PTR)(va) - (ULONG_PTR)MmSessionBase) >> PDI_SHIFT))

//
// Session space contains the image loader and tracker, virtual
// address allocator, paged pool allocator, system view image mappings,
// and working set for kernel mode virtual addresses that are instanced
// for groups of processes in a Session process group. This
// process group is identified by a SessionId.
//
// Each Session process group's loaded kernel modules, paged pool
// allocations, working set, and mapped system views are separate from
// other Session process groups, even though they have the same
// virtual addresses.
//
// This is to support the Hydra multi-user Windows NT system by
// replicating WIN32K.SYS, and its complement of video and printer drivers,
// desktop heaps, memory allocations, etc.
//

//
// Structure linked into a session space structure to describe
// which system images in PsLoadedModuleTable and
// SESSION_DRIVER_GLOBAL_LOAD_ADDRESS's
// have been allocated for the current session space.
//
// The reference count tracks the number of loads of this image within
// this session.
//

typedef struct _IMAGE_ENTRY_IN_SESSION {
    LIST_ENTRY Link;
    PVOID Address;
    PVOID LastAddress;
    ULONG ImageCountInThisSession;
    PMMPTE PrototypePtes;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
} IMAGE_ENTRY_IN_SESSION, *PIMAGE_ENTRY_IN_SESSION;

extern LIST_ENTRY MiSessionWsList;

NTSTATUS
FASTCALL
MiCheckPdeForSessionSpace(
    IN PVOID VirtualAddress
    );

NTSTATUS
MiShareSessionImage (
    IN PSECTION Section,
    IN OUT PSIZE_T ViewSize
    );

VOID
MiSessionWideInitializeAddresses (
    VOID
    );

NTSTATUS
MiSessionWideReserveImageAddress (
    IN PUNICODE_STRING pImageName,
    IN PSECTION Section,
    IN ULONG_PTR Alignment,
    OUT PVOID *ppAddr,
    OUT PBOOLEAN pAlreadyLoaded
    );

VOID
MiInitializeSessionIds (
    VOID
    );

VOID
MiInitializeSessionWsSupport(
    VOID
    );

VOID
MiSessionAddProcess (
    IN PEPROCESS NewProcess
    );

VOID
MiSessionRemoveProcess (
    VOID
    );

NTSTATUS
MiRemovePsLoadedModule(
    PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

NTSTATUS
MiRemoveImageSessionWide(
    IN PVOID BaseAddr
    );

NTSTATUS
MiDeleteSessionVirtualAddresses(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes
    );

NTSTATUS
MiUnloadSessionImageByForce (
    IN SIZE_T NumberOfPtes,
    IN PVOID ImageBase
    );

NTSTATUS
MiSessionWideGetImageSize(
    IN PVOID BaseAddress,
    OUT PSIZE_T NumberOfBytes OPTIONAL,
    OUT PSIZE_T CommitPages OPTIONAL
    );

PIMAGE_ENTRY_IN_SESSION
MiSessionLookupImage (
    IN PVOID BaseAddress
    );

NTSTATUS
MiSessionCommitImagePages(
    PVOID BaseAddr,
    SIZE_T Size
    );

VOID
MiSessionUnloadAllImages (
    VOID
    );

VOID
MiFreeSessionSpaceMap (
    VOID
    );

NTSTATUS
MiSessionInitializeWorkingSetList (
    VOID
    );

VOID
MiSessionUnlinkWorkingSet (
    VOID
    );

NTSTATUS
MiSessionCopyOnWrite (
    IN PMM_SESSION_SPACE SessionSpace,
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte
    );

VOID
MiSessionOutSwapProcess (
    IN PEPROCESS Process
    );

VOID
MiSessionInSwapProcess (
    IN PEPROCESS Process
    );

#if !defined (_X86PAE_)

#define MI_GET_DIRECTORY_FRAME_FROM_PROCESS(_Process) \
        MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&((_Process)->Pcb.DirectoryTableBase[0])))

#else

#define MI_GET_DIRECTORY_FRAME_FROM_PROCESS(_Process) \
        ((_Process)->PaePageDirectoryPage)
#endif

PERFINFO_MIH_DECL

#if defined(_MIALT4K_)
NTSTATUS
MiSetCopyPagesFor4kPage(
    IN PEPROCESS Process,
    IN OUT PMMVAD *Vad,
    IN OUT PVOID *StartingAddress,
    IN OUT PVOID *EndingAddress,
    IN ULONG ProtectionMask);
#endif

#endif  // MI
