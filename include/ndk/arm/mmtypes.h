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
typedef struct _HARDWARE_PTE_ARM
{
    union
    {
        union
        {
            struct
            {
                ULONG Type:2;
                ULONG Unused:30;
            } Fault;
            struct
            {
                ULONG Type:2;
                ULONG Ignored:2;
                ULONG Reserved:1;
                ULONG Domain:4;
                ULONG Ignored1:1;
                ULONG BaseAddress:22;
            } Coarse;
            struct
            {
                ULONG Type:2;
                ULONG Buffered:1;
                ULONG Cached:1;
                ULONG Reserved:1;
                ULONG Domain:4;
                ULONG Ignored:1;
                ULONG Access:2;
                ULONG Ignored1:8;
                ULONG BaseAddress:12;
            } Section;
            struct
            {
                ULONG Type:2;
                ULONG Reserved:3;
                ULONG Domain:4;
                ULONG Ignored:3;
                ULONG BaseAddress:20;
            } Fine;
        } L1;
        union
        {
            struct
            {
                ULONG Type:2;
                ULONG Unused:30;
            } Fault;
            struct
            {
                ULONG Type:2;
                ULONG Buffered:1;
                ULONG Cached:1;
                ULONG Access0:2;
                ULONG Access1:2;
                ULONG Access2:2;
                ULONG Access3:2;
                ULONG Ignored:4;
                ULONG BaseAddress:16;
            } Large;
            struct
            {
                ULONG Type:2;
                ULONG Buffered:1;
                ULONG Cached:1;
                ULONG Access0:2;
                ULONG Access1:2;
                ULONG Access2:2;
                ULONG Access3:2;
                ULONG BaseAddress:20;
            } Small;
            struct
            {
                ULONG Type:2;
                ULONG Buffered:1;
                ULONG Cached:1;
                ULONG Access0:2;
                ULONG Ignored:4;
                ULONG BaseAddress:22;
            } Tiny; 
        } L2;
        ULONG AsUlong;
    };
} HARDWARE_PTE_ARM, *PHARDWARE_PTE_ARM;

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

typedef struct _MMPDE_HARDWARE // FIXFIX: Find a way to make this more portable
{
    union
    {
        union
        {
            struct
            {
                ULONG Valid:1;
                ULONG Section:1;
                ULONG Sbz:3;
                ULONG Domain:4;
                ULONG EccEnabled:1;
                ULONG PageFrameNumber:22;
            } Coarse;
            struct
            {
                ULONG Coarse:1;
                ULONG Valid:1;
                ULONG Buffered:1;
                ULONG Cached:1;
                ULONG Reserved:1;
                ULONG Domain:4;
                ULONG EccEnabled:1;
                ULONG Access:2;
                ULONG ExtendedAccess:3;
                ULONG Sbz:3;
                ULONG SuperSection:1;
                ULONG Sbz1:1;
                ULONG PageFrameNumber:12;
            } Section;
            ULONG AsUlong;
        } Hard;
    } u;
} MMPDE_HARDWARE, *PMMPDE_HARDWARE;

typedef union _MMPTE_HARDWARE
{
    struct
    {
        ULONG ExecuteNever:1;
        ULONG Valid:1;
        ULONG Buffered:1;
        ULONG Cached:1;
        ULONG Access:2;
        ULONG TypeExtension:3;
        ULONG ExtendedAccess:1;
        ULONG Shared:1;
        ULONG NonGlobal:1;
        ULONG PageFrameNumber:20;
    };
    ULONG AsUlong;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

//
// Use the right PTE structure
//
#define HARDWARE_PTE        HARDWARE_PTE_ARM
#define PHARDWARE_PTE       PHARDWARE_PTE_ARM

#endif
