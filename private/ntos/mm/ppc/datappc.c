/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  IBM Corporation

Module Name:

   datappc.c

Abstract:

    This module contains the private hardware specific global storage for
    the memory management subsystem.

Author:

    Lou Perazzoli (loup) 27-Mar-1990

    Modified for PowerPC by Mark Mergen (mergen@watson.ibm.com) 6-Oct-93

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

MMPTE ZeroKernelPte = { 0 };


MMPTE ValidKernelPte = { MM_PTE_VALID_MASK |
                         MM_PTE_WRITE_MASK };


MMPTE ValidUserPte =   { MM_PTE_VALID_MASK |
                         MM_PTE_WRITE_MASK };


MMPTE ValidPtePte =   { MM_PTE_VALID_MASK |
                        MM_PTE_WRITE_MASK };


MMPTE ValidPdePde =   { MM_PTE_VALID_MASK |
                        MM_PTE_WRITE_MASK };


MMPTE ValidKernelPde =   { MM_PTE_VALID_MASK |
                           MM_PTE_WRITE_MASK };


MMPTE DemandZeroPde = { MM_READWRITE << 3 };


MMPTE DemandZeroPte = { MM_READWRITE << 3 };


MMPTE TransitionPde = { MM_PTE_TRANSITION_MASK | (MM_READWRITE << 3) };


MMPTE PrototypePte = { 0xFFFFF000 | (MM_READWRITE << 3) | MM_PTE_PROTOTYPE_MASK };


//
// PTE which generates an access violation when referenced.
//

MMPTE NoAccessPte = {MM_NOACCESS << 3};


//
// Pool start and end.
//

PVOID MmNonPagedPoolStart;

PVOID MmNonPagedPoolEnd = ((PVOID)MM_NONPAGED_POOL_END);

PVOID MmPagedPoolStart =  ((PVOID)MM_PAGED_POOL_START);

PVOID MmPagedPoolEnd;

#if MM_MAXIMUM_NUMBER_OF_COLORS > 1
MMPFNLIST MmFreePagesByPrimaryColor[2][MM_MAXIMUM_NUMBER_OF_COLORS];

MMPFNLIST MmStandbyPageListByColor[MM_MAXIMUM_NUMBER_OF_COLORS] = {
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                             0, StandbyPageList, MM_EMPTY_LIST, MM_EMPTY_LIST
                            };


#endif

PMMCOLOR_TABLES MmFreePagesByColor[2];


//
// Color tables for modified pages destined for the paging file.
//

MMPFNLIST MmModifiedPageListByColor[MM_MAXIMUM_NUMBER_OF_COLORS] = {
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST,
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST};

ULONG MmSecondaryColorMask;

//
// Count of the number of modified pages destined for the paging file.
//

ULONG MmTotalPagesForPagingFile = 0;

//
// PTE reserved for mapping physical data for debugger.
// Use 1 page from last 4MB of virtual address space
// reserved for the HAL.
//

PMMPTE MmDebugPte = (MiGetPteAddress((PVOID)MM_HAL_RESERVED));


//
// 16 PTEs reserved for mapping MDLs (64k max).
//

PMMPTE MmCrashDumpPte = (MiGetPteAddress((PVOID)MM_HAL_RESERVED));

