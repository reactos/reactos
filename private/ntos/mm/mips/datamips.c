/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   datamips.c

Abstract:

    This module contains the private hardware specific global storage for
    the memory management subsystem.

Author:

    Lou Perazzoli (loup) 27-Mar-1990

Revision History:

--*/

#include "mi.h"

//
// A zero Pte.
//

MMPTE ZeroPte = { 0 };

//
// A kernel zero PTE.
//

#ifdef R3000
MMPTE ZeroKernelPte = { 0 };
#endif //R3000

#ifdef R4000
MMPTE ZeroKernelPte = { MM_PTE_GLOBAL_MASK };
#endif //R4000

#ifdef R3000
MMPTE ValidKernelPte = { MM_PTE_VALID_MASK |
                         MM_PTE_WRITE_MASK |
                         MM_PTE_DIRTY_MASK |
                         MM_PTE_GLOBAL_MASK };
#endif //R3000

#ifdef R4000
MMPTE ValidKernelPte = { MM_PTE_VALID_MASK |
                         MM_PTE_CACHE_ENABLE_MASK |
                         MM_PTE_WRITE_MASK |
                         MM_PTE_DIRTY_MASK |
                         MM_PTE_GLOBAL_MASK };
#endif //R4000


MMPTE ValidUserPte =   { MM_PTE_VALID_MASK |
                         MM_PTE_WRITE_MASK |
                         MM_PTE_CACHE_ENABLE_MASK |
                         MM_PTE_DIRTY_MASK };

MMPTE ValidPtePte =   { MM_PTE_VALID_MASK |
                        MM_PTE_WRITE_MASK |
                        MM_PTE_CACHE_ENABLE_MASK |
                        MM_PTE_DIRTY_MASK };

MMPTE ValidPdePde =   { MM_PTE_VALID_MASK |
                        MM_PTE_WRITE_MASK |
                        MM_PTE_CACHE_ENABLE_MASK |
                        MM_PTE_DIRTY_MASK };

MMPTE ValidKernelPde =   { MM_PTE_VALID_MASK |
                           MM_PTE_WRITE_MASK |
                           MM_PTE_CACHE_ENABLE_MASK |
                           MM_PTE_DIRTY_MASK |
                           MM_PTE_GLOBAL_MASK };

#ifdef R3000
MMPTE DemandZeroPde = { MM_READWRITE << 4 };
#endif //R3000

#ifdef R4000
MMPTE DemandZeroPde = { MM_READWRITE << 3 };
#endif //R4000

#ifdef R3000
MMPTE DemandZeroPte = { MM_READWRITE << 4 };
#endif //R3000


#ifdef R4000
MMPTE DemandZeroPte = { MM_READWRITE << 3 };
#endif //R4000

#ifdef R3000
MMPTE TransitionPde = { 0x2 | (MM_READWRITE << 4) };
#endif //R3000

#ifdef R4000
MMPTE TransitionPde = { MM_PTE_TRANSITION_MASK | (MM_READWRITE << 3) };
#endif //R4000

#ifdef R3000
MMPTE PrototypePte = { 0xFFFFF000 | (MM_READWRITE << 4) | MM_PTE_PROTOTYPE_MASK };
#endif //R3000

#ifdef R4000
MMPTE PrototypePte = { 0xFFFFF000 | (MM_READWRITE << 3) | MM_PTE_PROTOTYPE_MASK };
#endif //R4000

//
// PTE which generates an access violation when referenced.
//

#ifdef R3000
MMPTE NoAccessPte = {MM_NOACCESS << 4};
#endif //R3000

#ifdef R4000
MMPTE NoAccessPte = {MM_NOACCESS << 3};
#endif //R4000


//
// Pool start and end.
//

PVOID MmNonPagedPoolStart;

PVOID MmNonPagedPoolEnd = ((PVOID)MM_NONPAGED_POOL_END);

PVOID MmPagedPoolStart =  (PVOID)0xE1000000;

PVOID MmPagedPoolEnd;


//
// Color tables for free and zeroed pages.
//

MMPFNLIST MmFreePagesByPrimaryColor[2][MM_MAXIMUM_NUMBER_OF_COLORS];

PMMCOLOR_TABLES MmFreePagesByColor[2];

MMPFNLIST MmStandbyPageListByColor[MM_MAXIMUM_NUMBER_OF_COLORS] = {
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST
                            };


//
// Color tables for modified pages destined for the paging file.
//

MMPFNLIST MmModifiedPageListByColor[MM_MAXIMUM_NUMBER_OF_COLORS] = {
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST};

ULONG MmSecondaryColorMask;

//
// Count of the number of modified pages destined for the paging file.
//

ULONG MmTotalPagesForPagingFile;


//
// PTE reserved for mapping physical data for debugger.
//

PMMPTE MmDebugPte = (MiGetPteAddress((PVOID)MM_NONPAGED_POOL_END));

//
// 17 PTEs reserved for mapping MDLs (64k max) + 1 to ensure g-bits right.
//

PMMPTE MmCrashDumpPte = (MiGetPteAddress((PVOID)MM_NONPAGED_POOL_END));
