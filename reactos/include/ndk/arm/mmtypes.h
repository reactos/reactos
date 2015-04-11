/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    mmtypes.h (ARM)

Abstract:

    ARM Type definitions for the Memory Manager

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _ARM_MMTYPES_H
#define _ARM_MMTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

//
// Dependencies
//

//
// Page-related Macros
//
#ifndef PAGE_SIZE
#define PAGE_SIZE                         0x1000
#endif
#define PAGE_SHIFT                        12L
#define MM_ALLOCATION_GRANULARITY         0x10000
#define MM_ALLOCATION_GRANULARITY_SHIFT   16L
#define MM_PAGE_FRAME_NUMBER_SIZE         20

//
// User space range limit
//
#define MI_HIGHEST_USER_ADDRESS                 (PVOID)0x7FFEFFFF

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
typedef struct _HARDWARE_PDE_ARMV6
{
    ULONG Valid:1;     // Only for small pages
    ULONG LargePage:1; // Note, if large then Valid = 0
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG NoExecute:1;
    ULONG Domain:4;
    ULONG Ecc:1;
    ULONG PageFrameNumber:22;
} HARDWARE_PDE_ARMV6, *PHARDWARE_PDE_ARMV6;

typedef struct _HARDWARE_LARGE_PTE_ARMV6
{
    ULONG Valid:1;     // Only for small pages
    ULONG LargePage:1; // Note, if large then Valid = 0
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG NoExecute:1;
    ULONG Domain:4;
    ULONG Ecc:1;
    ULONG Sbo:1;
    ULONG Owner:1;
    ULONG CacheAttributes:3;
    ULONG ReadOnly:1;
    ULONG Shared:1;
    ULONG NonGlobal:1;
    ULONG SuperLagePage:1;
    ULONG Reserved:1;
    ULONG PageFrameNumber:12;
} HARDWARE_LARGE_PTE_ARMV6, *PHARDWARE_LARGE_PTE_ARMV6;

typedef struct _HARDWARE_PTE_ARMV6
{
    ULONG NoExecute:1;
    ULONG Valid:1;
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG Sbo:1;
    ULONG Owner:1;
    ULONG CacheAttributes:3;
    ULONG ReadOnly:1;
    ULONG Shared:1;
    ULONG NonGlobal:1;
    ULONG PageFrameNumber:20;
} HARDWARE_PTE_ARMV6, *PHARDWARE_PTE_ARMV6;

C_ASSERT(sizeof(HARDWARE_PDE_ARMV6) == sizeof(ULONG));
C_ASSERT(sizeof(HARDWARE_LARGE_PTE_ARMV6) == sizeof(ULONG));
C_ASSERT(sizeof(HARDWARE_PTE_ARMV6) == sizeof(ULONG));

typedef struct _MMPTE_SOFTWARE
{
    ULONG Valid:2;
    ULONG PageFileLow:4;
    ULONG Protection:4;
    ULONG Prototype:1;
    ULONG Transition:1;
    ULONG PageFileHigh:20;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION
{
    ULONG Valid:2;
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG Owner:1;
    ULONG Protection:4;
    ULONG ReadOnly:1;
    ULONG Prototype:1;
    ULONG Transition:1;
    ULONG PageFrameNumber:20;
} MMPTE_TRANSITION;

typedef struct _MMPTE_PROTOTYPE
{
    ULONG Valid:2;
    ULONG ProtoAddressLow:7;
    ULONG ReadOnly:1;
    ULONG Prototype:1;
    ULONG ProtoAddressHigh:21;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_SUBSECTION
{
    ULONG Valid:2;
    ULONG SubsectionAddressLow:4;
    ULONG Protection:4;
    ULONG Prototype:1;
    ULONG SubsectionAddressHigh:20;
    ULONG WhichPool:1;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST
{
    ULONG Valid:2;
    ULONG OneEntry:1;
    ULONG filler0:8;
    ULONG NextEntry:20;
    ULONG Prototype:1;
} MMPTE_LIST;

typedef union _MMPTE_HARDWARE
{
    ULONG NoExecute:1;
    ULONG Valid:1;
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG Sbo:1;
    ULONG Owner:1;
    ULONG CacheAttributes:3;
    ULONG ReadOnly:1;
    ULONG Prototype:1;
    ULONG NonGlobal:1;
    ULONG PageFrameNumber:20;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

typedef union _MMPDE_HARDWARE
{
    ULONG Valid:1;
    ULONG LargePage:1;
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG NoExecute:1;
    ULONG Domain:4;
    ULONG Ecc:1;
    ULONG PageFrameNumber:22;
} MMPDE_HARDWARE, *PMMPDE_HARDWARE;

typedef struct _MMPDE
{
    union
    {
        MMPDE_HARDWARE Hard;
        ULONG Long;
    } u;
} MMPDE, *PMMPDE;

//
// Use the right PTE structure
//
#define HARDWARE_PTE        HARDWARE_PTE_ARMV6
#define PHARDWARE_PTE       PHARDWARE_PTE_ARMV6

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
