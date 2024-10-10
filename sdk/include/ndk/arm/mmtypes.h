/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NDK mmtypes for ARMv7
 * COPYRIGHT:   Copyright 2004 Alex Ionescu <alex.ionescu@reactos.com>
 *              Copyright 2021 Leandro Friedrich <email@leandrofriedrich.de>
 */

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
// Address of the shared user page
//
#define MM_SHARED_USER_DATA_VA 0x7FFE0000

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
 typedef struct _HARDWARE_PDE
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

typedef struct _HARDWARE_LARGE_PTE
{
    ULONG Valid:1;     // Only for small pages
    ULONG LargePage:1; // Note, if large then Valid = 0
    ULONG Buffered:1;
    ULONG Cached:1;
    ULONG NoExecute:1;
    ULONG Domain:4;
    ULONG Ecc:1;
    ULONG Sbo:1; // ULONG Accessed:1;?
    ULONG Owner:1;
    ULONG CacheAttributes:3;
    ULONG ReadOnly:1;
    ULONG Shared:1;
    ULONG NonGlobal:1;
    ULONG SuperLagePage:1;
    ULONG Reserved:1;
    ULONG PageFrameNumber:12;
} HARDWARE_LARGE_PTE, *PHARDWARE_LARGE_PTE;

typedef struct _HARDWARE_PTE
{
	ULONG Valid:1;
	ULONG CacheType:2;
	ULONG Accessed:1;
	ULONG Owner:1;
	ULONG TypeExtention:1;
	ULONG Writable:1;
	ULONG CopyOnWrite:1;
	ULONG ReadOnly:1;
	ULONG LargePage:1;
	ULONG NonGlobal:1;
	ULONG PageFrameNumber:20;
} HARDWARE_PTE, *PHARDWARE_PTE;

typedef struct _MMPTE_SOFTWARE
{
	ULONG Valid:2;
	ULONG PageFileLow:1;
	ULONG PageFileReserved:1;
	ULONG PageFileAllocated:1;
	ULONG Protection:5;
	ULONG Prototype:1;
	ULONG Transition:1;
	ULONG InStore:1;
	ULONG PageFileHigh:19;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION
{
	ULONG Valid:2;
	ULONG CacheType:2;
	ULONG Spare:1;
	ULONG Protection:5;
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
    ULONG SubsectionAddressLow:8;
    ULONG Prototype:1;
    ULONG SubsectionAddressHigh:21;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST
{
    ULONG Valid:2;
    ULONG OneEntry:1;
    ULONG filler0:7;
	ULONG Prototype:1;
	ULONG filler1:1;
    ULONG NextEntry:20;
} MMPTE_LIST;

typedef struct _MMPTE_HARDWARE
{
	ULONG Valid:2;
	ULONG CacheType:2;
	ULONG Accessed:1;
	ULONG Owner:1;
	ULONG TypeExtention:1;
	ULONG Writable:1;
	ULONG CopyOnWrite:1;
	ULONG NotDirty:1;
	ULONG LargePage:1;
	ULONG NonGlobal:1;
	ULONG PageFrameNumber:20;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

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
} MMPTE, *PMMPTE;

typedef union _MMPDE_HARDWARE
{
	ULONG Valid:2;
	ULONG CacheType:2;
	ULONG Accessed:1;
	ULONG Owner:1;
	ULONG TypeExtention:1;
	ULONG Writable:1;
	ULONG CopyOnWrite:1;
	ULONG NotDirty:1;
	ULONG LargePage:1;
	ULONG NonGlobal:1;
	ULONG PageFrameNumber:20;
} MMPDE_HARDWARE, *PMMPDE_HARDWARE;

typedef struct _MMPDE
{
    union
    {
        MMPDE_HARDWARE Hard;
        ULONG Long;
    } u;
} MMPDE, *PMMPDE;

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
