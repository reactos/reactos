/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    mmtypes.h (AMD64)

Abstract:

    AMD64 Type definitions for the Memory Manager

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004
    Timo Kreuzer (timo.kreuzer@reactos.com)   15-Aug-2008

--*/

#ifndef _AMD64_MMTYPES_H
#define _AMD64_MMTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

//
// Dependencies
//

//
// Page-related Macros
//
#define PAGE_SIZE                         0x1000
#define PAGE_SHIFT                        12L
#define MM_ALLOCATION_GRANULARITY         0x10000
#define MM_ALLOCATION_GRANULARITY_SHIFT   16L
#define MM_PAGE_FRAME_NUMBER_SIZE         52

//
// User space range limit
//
#define MI_HIGHEST_USER_ADDRESS         (PVOID)0x000007FFFFFEFFFFULL

//
// Address of the shared user page
//
#define MM_SHARED_USER_DATA_VA 0x7FFE0000ULL

//
// Sanity checks for Paging Macros
//
#ifdef C_ASSERT
C_ASSERT(PAGE_SIZE == (1 << PAGE_SHIFT));
C_ASSERT(MM_ALLOCATION_GRANULARITY == (1 << MM_ALLOCATION_GRANULARITY_SHIFT));
C_ASSERT(MM_ALLOCATION_GRANULARITY &&
         !(MM_ALLOCATION_GRANULARITY & (MM_ALLOCATION_GRANULARITY - 1)));
C_ASSERT(MM_ALLOCATION_GRANULARITY >= PAGE_SIZE);
#endif

//
// Page Table Entry Definitions
//
typedef struct _HARDWARE_PTE
{
    ULONG64 Valid:1;
    ULONG64 Write:1;
    ULONG64 Owner:1;
    ULONG64 WriteThrough:1;
    ULONG64 CacheDisable:1;
    ULONG64 Accessed:1;
    ULONG64 Dirty:1;
    ULONG64 LargePage:1;
    ULONG64 Global:1;
    ULONG64 CopyOnWrite:1;
    ULONG64 Prototype:1;
    ULONG64 reserved0:1;
    ULONG64 PageFrameNumber:28;
    ULONG64 reserved1:12;
    ULONG64 SoftwareWsIndex:11;
    ULONG64 NoExecute:1;
} HARDWARE_PTE, *PHARDWARE_PTE;

typedef struct _MMPTE_SOFTWARE
{
    ULONG64 Valid:1;
    ULONG64 PageFileLow:4;
    ULONG64 Protection:5;
    ULONG64 Prototype:1;
    ULONG64 Transition:1;
    ULONG64 UsedPageTableEntries:10;
    ULONG64 Reserved:10;
    ULONG64 PageFileHigh:32;
} MMPTE_SOFTWARE, *PMMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION
{
    ULONG64 Valid:1;
    ULONG64 Write:1;
    ULONG64 Owner:1;
    ULONG64 WriteThrough:1;
    ULONG64 CacheDisable:1;
    ULONG64 Protection:5;
    ULONG64 Prototype:1;
    ULONG64 Transition:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG64 PageFrameNumber:36;
    ULONG64 Unused:16;
#else
    ULONG64 PageFrameNumber:28;
    ULONG64 Unused:24;
#endif
} MMPTE_TRANSITION;

typedef struct _MMPTE_PROTOTYPE
{
    ULONG64 Valid:1;
    ULONG64 Unused0:7;
    ULONG64 ReadOnly:1;
    ULONG64 Unused1:1;
    ULONG64 Prototype:1;
    ULONG64 Protection:5;
    LONG64 ProtoAddress:48;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_SUBSECTION
{
    ULONG64 Valid:1;
    ULONG64 Unused0:4;
    ULONG64 Protection:5;
    ULONG64 Prototype:1;
    ULONG64 Unused1:5;
    LONG64 SubsectionAddress:48;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST
{
    ULONG64 Valid:1;
    ULONG64 OneEntry:1;
    ULONG64 filler0:3;
    ULONG64 Protection:5;
    ULONG64 Prototype:1;
    ULONG64 Transition:1;
    ULONG64 filler1:20;
    ULONG64 NextEntry:32;
} MMPTE_LIST;

typedef struct _MMPTE_HARDWARE
{
    ULONG64 Valid:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG64 Dirty1:1;
#else
#ifdef CONFIG_SMP
    ULONG64 Writable:1;
#else
    ULONG64 Write:1;
#endif
#endif
    ULONG64 Owner:1;
    ULONG64 WriteThrough:1;
    ULONG64 CacheDisable:1;
    ULONG64 Accessed:1;
    ULONG64 Dirty:1;
    ULONG64 LargePage:1;
    ULONG64 Global:1;
    ULONG64 CopyOnWrite:1;
    ULONG64 Prototype:1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG64 Write:1;
    ULONG64 PageFrameNumber:36;
    ULONG64 reserved1:4;
#else
#ifdef CONFIG_SMP
    ULONG64 Write:1;
#else
    ULONG64 reserved0:1;
#endif
    ULONG64 PageFrameNumber:28;
    ULONG64 reserved1:12;
#endif
    ULONG64 SoftwareWsIndex:11;
    ULONG64 NoExecute:1;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

typedef struct _MMPTE_HARDWARE_LARGEPAGE
{
    ULONG64 Valid:1;
    ULONG64 Write:1;
    ULONG64 Owner:1;
    ULONG64 WriteThrough:1;
    ULONG64 CacheDisable:1;
    ULONG64 Accessed:1;
    ULONG64 Dirty:1;
    ULONG64 LargePage:1;
    ULONG64 Global:1;
    ULONG64 CopyOnWrite:1;
    ULONG64 Prototype:1;
    ULONG64 reserved0:1;
    ULONG64 PAT:1;
    ULONG64 reserved1:8;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG64 PageFrameNumber:27;
    ULONG64 reserved2:16;
#else
    ULONG64 PageFrameNumber:19;
    ULONG64 reserved2:24;
#endif
} MMPTE_HARDWARE_LARGEPAGE, *PMMPTE_HARDWARE_LARGEPAGE;

typedef struct _MMPTE
{
    union
    {
        ULONG_PTR Long;
        HARDWARE_PTE Flush;
        MMPTE_HARDWARE Hard;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TRANSITION Trans;
        MMPTE_SUBSECTION Subsect;
        MMPTE_LIST List;
    } u;
} MMPTE, *PMMPTE,
  MMPDE, *PMMPDE,
  MMPPE, *PMMPPE,
  MMPXE, *PMMPXE;

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // !AMD64_MMTYPES_H
