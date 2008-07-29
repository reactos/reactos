/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.
Copyright (c) Timo Kreuzer  All rights reserved.

Header Name:

    mmtypes.h (AMD64)

Abstract:

    AMD64 Type definitions for the Memory Manager

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004
    Timo Kreuzer (timo.kreuzer@reactos.com)   29-Jul-2008

--*/

#ifndef _AMD64_MMTYPES_H
#define _AMD64_MMTYPES_H

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
typedef struct _HARDWARE_PTE_AMD64
{
    ULONGLONG Valid:1;
    ULONGLONG Write:1;
    ULONGLONG Owner:1;
    ULONGLONG WriteThrough:1;
    ULONGLONG CacheDisable:1;
    ULONGLONG Accessed:1;
    ULONGLONG Dirty:1;
    ULONGLONG LargePage:1;
    ULONGLONG Global:1;
    ULONGLONG CopyOnWrite:1;
    ULONGLONG Prototype: 1;
    ULONGLONG reserved1: 1;
    ULONGLONG PageFrameNumber:40;
    ULONGLONG reserved2:11;
    ULONGLONG NoExecute:1;
} HARDWARE_PTE_AMD64, *PHARDWARE_PTE_AMD64;

typedef struct _MMPTE_SOFTWARE
{
    ULONG Valid:1;
    ULONG PageFileLow:4;
    ULONG Protection:5;
    ULONG Prototype:1;
    ULONG Transition:1;
    ULONG PageFileHigh:20;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION
{
    ULONG Valid:1;
    ULONG Write:1;
    ULONG Owner:1;
    ULONG WriteThrough:1;
    ULONG CacheDisable:1;
    ULONG Protection:5;
    ULONG Prototype:1;
    ULONG Transition:1;
    ULONG PageFrameNumber:20;
} MMPTE_TRANSITION;

typedef struct _MMPTE_PROTOTYPE
{
    ULONG Valid:1;
    ULONG ProtoAddressLow:7;
    ULONG ReadOnly:1;
    ULONG WhichPool:1;
    ULONG Prototype:1;
    ULONG ProtoAddressHigh:21;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_SUBSECTION
{
    ULONG Valid:1;
    ULONG SubsectionAddressLow:4;
    ULONG Protection:5;
    ULONG Prototype:1;
    ULONG SubsectionAddressHigh:20;
    ULONG WhichPool:1;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST
{
    ULONG Valid:1;
    ULONG OneEntry:1;
    ULONG filler0:8;
    ULONG NextEntry:20;
    ULONG Prototype:1;
    ULONG filler1:1;
} MMPTE_LIST;

#ifndef CONFIG_SMP

typedef struct _MMPTE_HARDWARE
{
    ULONGLONG Valid:1;
    ULONGLONG Write:1;
    ULONGLONG Owner:1;
    ULONGLONG WriteThrough:1;
    ULONGLONG CacheDisable:1;
    ULONGLONG Accessed:1;
    ULONGLONG Dirty:1;
    ULONGLONG LargePage:1;
    ULONGLONG Global:1;
    ULONGLONG CopyOnWrite:1;
    ULONGLONG Prototype: 1;
    ULONGLONG reserved1: 1;
    ULONGLONG PageFrameNumber:40;
    ULONGLONG reserved2:11;
    ULONGLONG NoExecute:1;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

#else

#error MMPTE_HARDWARE undeclared

#endif

//
// Use the right PTE structure
//
#define HARDWARE_PTE        HARDWARE_PTE_AMD64
#define PHARDWARE_PTE       PHARDWARE_PTE_AMD64

#endif
