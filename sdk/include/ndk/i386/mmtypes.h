/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    mmtypes.h (X86)

Abstract:

    i386 Type definitions for the Memory Manager

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _I386_MMTYPES_H
#define _I386_MMTYPES_H

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
// PAE SEG0 Base?
//
#define KSEG0_BASE_PAE                    0xE0000000

//
// Page Table Entry Definitions
//
#if !defined(_X86PAE_)

typedef struct _HARDWARE_PTE_X86
{
    ULONG Valid:1;
    ULONG Write:1;
    ULONG Owner:1;
    ULONG WriteThrough:1;
    ULONG CacheDisable:1;
    ULONG Accessed:1;
    ULONG Dirty:1;
    ULONG LargePage:1;
    ULONG Global:1;
    ULONG CopyOnWrite:1;
    ULONG Prototype: 1;
    ULONG reserved: 1;
    ULONG PageFrameNumber:20;
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

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
    ULONG Prototype:1;
    ULONG filler1:1;
    ULONG NextEntry:20;
} MMPTE_LIST;

typedef struct _MMPTE_HARDWARE
{
    ULONG Valid:1;
#ifndef CONFIG_SMP
    ULONG Write:1;
#else
    ULONG Writable:1;
#endif
    ULONG Owner:1;
    ULONG WriteThrough:1;
    ULONG CacheDisable:1;
    ULONG Accessed:1;
    ULONG Dirty:1;
    ULONG LargePage:1;
    ULONG Global:1;
    ULONG CopyOnWrite:1;
    ULONG Prototype:1;
#ifndef CONFIG_SMP
    ULONG reserved:1;
#else
    ULONG Write:1;
#endif
    ULONG PageFrameNumber:20;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

#else

typedef struct _HARDWARE_PTE_X86
{
    union
    {
        struct
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
            ULONGLONG Prototype:1;
            ULONGLONG reserved0:1;
            ULONGLONG PageFrameNumber:26;
            ULONGLONG reserved1:25;
            ULONGLONG NoExecute:1;
        };
        struct
        {
            ULONG LowPart;
            ULONG HighPart;
        };
    };
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

typedef struct _MMPTE_SOFTWARE
{
    ULONGLONG Valid:1;
    ULONGLONG PageFileLow:4;
    ULONGLONG Protection:5;
    ULONGLONG Prototype:1;
    ULONGLONG Transition:1;
    ULONGLONG Unused:20;
    ULONGLONG PageFileHigh:32;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION
{
    ULONGLONG Valid:1;
    ULONGLONG Write:1;
    ULONGLONG Owner:1;
    ULONGLONG WriteThrough:1;
    ULONGLONG CacheDisable:1;
    ULONGLONG Protection:5;
    ULONGLONG Prototype:1;
    ULONGLONG Transition:1;
    ULONGLONG PageFrameNumber:26;
    ULONGLONG Unused:26;
} MMPTE_TRANSITION;

typedef struct _MMPTE_PROTOTYPE
{
    ULONGLONG Valid:1;
    ULONGLONG Unused0:7;
    ULONGLONG ReadOnly:1;
    ULONGLONG Unused1:1;
    ULONGLONG Prototype:1;
    ULONGLONG Protection:5;
    ULONGLONG Unused:16;
    ULONGLONG ProtoAddress:32;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_SUBSECTION
{
    ULONGLONG Valid:1;
    ULONGLONG Unused0:4;
    ULONGLONG Protection:5;
    ULONGLONG Prototype:1;
    ULONGLONG Unused1:21;
    ULONGLONG SubsectionAddress:32;
} MMPTE_SUBSECTION;

typedef struct _MMPTE_LIST
{
    ULONGLONG Valid:1;
    ULONGLONG OneEntry:1;
    ULONGLONG filler0:8;
    ULONGLONG Prototype:1;
    ULONGLONG filler1:21;
    ULONGLONG NextEntry:32;
} MMPTE_LIST;

typedef struct _MMPTE_HARDWARE
{
    ULONGLONG Valid:1;
#ifndef CONFIG_SMP
    ULONGLONG Write:1;
#else
    ULONGLONG Writable:1;
#endif
    ULONGLONG Owner:1;
    ULONGLONG WriteThrough:1;
    ULONGLONG CacheDisable:1;
    ULONGLONG Accessed:1;
    ULONGLONG Dirty:1;
    ULONGLONG LargePage:1;
    ULONGLONG Global:1;
    ULONGLONG CopyOnWrite:1;
    ULONGLONG Prototype:1;
#ifndef CONFIG_SMP
    ULONGLONG reserved0:1;
#else
    ULONGLONG Write:1;
#endif
    ULONGLONG PageFrameNumber:26;
    ULONGLONG reserved1:26;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

#endif

//
// Use the right PTE structure
//
#define HARDWARE_PTE        HARDWARE_PTE_X86
#define PHARDWARE_PTE       PHARDWARE_PTE_X86

typedef struct _MMPTE
{
    union
    {
#if !defined(_X86PAE_)
        ULONG Long;
#else
        ULONGLONG Long;
        struct
        {
            ULONG LowPart;
            ULONG HighPart;
        } HighLow;
#endif
        HARDWARE_PTE Flush;
        MMPTE_HARDWARE Hard;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TRANSITION Trans;
        MMPTE_SUBSECTION Subsect;
        MMPTE_LIST List;
    } u;
} MMPTE, *PMMPTE,
  MMPDE, *PMMPDE;

#if !defined(_X86PAE_)
C_ASSERT(sizeof(MMPTE) == sizeof(ULONG));
#else
C_ASSERT(sizeof(MMPTE) == sizeof(ULONGLONG));
#endif

#ifdef __cplusplus
}; // extern "C"
#endif

#endif
