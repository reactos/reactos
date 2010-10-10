/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    mmtypes.h (PPC)

Abstract:

    PowerPC Type definitions for the Memory Manager

Author:

    Art Yerkes (ayerkes@speakeasy.net)   04-Dec-2005

--*/

#ifndef _POWERPC_MMTYPES_H
#define _POWERPC_MMTYPES_H

//
// Dependencies
//

//
// Page-related Macros
//
#define PAGE_SIZE                         0x1000
#define PAGE_SHIFT                        12L

typedef unsigned long long MMPTE_HARDWARE;
typedef unsigned long long MMPTE_SOFTWARE;
typedef unsigned long long MMPTE_PROTOTYPE;
typedef unsigned long long MMPTE_SUBSECTION;
typedef unsigned long long MMPTE_TRANSITION;
typedef unsigned long long MMPTE_LIST;

//
// Page Table Entry Definition
//
typedef struct _HARDWARE_PTE_PPC
{
    ULONG Dirty:2;
    ULONG Valid:1;
    ULONG GuardedStorage:1;
    ULONG MemoryCoherence:1;
    ULONG CacheDisable:1;
    ULONG WriteThrough:1;
    ULONG Change:1;
    ULONG Reference:1;
    ULONG Write:1;
    ULONG CopyOnWrite:1;
    ULONG rsvd1:1;
    ULONG PageFrameNumber:20;
} HARDWARE_PTE_PPC, *PHARDWARE_PTE_PPC;

#ifndef HARDWARE_PTE
#define HARDWARE_PTE HARDWARE_PTE_PPC
#define PHARDWARE_PTE PHARDWARE_PTE_PPC
#endif

#endif/*_POWERPC_MMTYPES_H*/
