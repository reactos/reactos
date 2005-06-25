/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/mmtypes.h
 * PURPOSE:         Definitions for Memory Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _MMTYPES_H
#define _MMTYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/

/* ENUMERATIONS **************************************************************/
typedef enum _PP_NPAGED_LOOKASIDE_NUMBER
{
    LookasideSmallIrpList = 0,
    LookasideLargeIrpList = 1,
    LookasideMdlList = 2,
    LookasideCreateInfoList = 3,
    LookasideNameBufferList = 4,
    LookasideTwilightList = 5,
    LookasideCompletionList = 6,
    LookasideMaximumList = 7
} PP_NPAGED_LOOKASIDE_NUMBER;

/* TYPES *********************************************************************/

/* FIXME: Forced to do this for now, because of EPROCESS, will go away before 0.3.0 */
#ifndef __NTOSKRNL__
typedef struct _MADDRESS_SPACE
{
    PVOID MemoryAreaRoot;
    FAST_MUTEX Lock;
    PVOID LowestAddress;
    struct _EPROCESS* Process;
    PUSHORT PageTableRefCountTable;
    ULONG PageTableRefCountTableSize;
} MADDRESS_SPACE, *PMADDRESS_SPACE;
#endif

typedef struct _PP_LOOKASIDE_LIST 
{
    struct _GENERAL_LOOKASIDE *P;
    struct _GENERAL_LOOKASIDE *L;
} PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;

typedef struct _ADDRESS_RANGE
{
    ULONG BaseAddrLow;
    ULONG BaseAddrHigh;
    ULONG LengthLow;
    ULONG LengthHigh;
    ULONG Type;
} ADDRESS_RANGE, *PADDRESS_RANGE;

typedef struct _MMADDRESS_NODE
{
    union
    {
        ULONG Balance:2;
        struct _MMADDRESS_NODE *Parent;
    } u1;
    struct _MMADDRESS_NODE *LeftChild;
    struct _MMADDRESS_NODE *RightChild;
    ULONG StartingVpn;
    ULONG EndingVpn;
} MMADDRESS_NODE, *PMMADDRESS_NODE;

typedef struct _MM_AVL_TABLE
{
    MMADDRESS_NODE BalancedRoot;
    ULONG DepthOfTree:5;
    ULONG Unused:3;
    ULONG NumberGenericTableElements:24;
    PVOID NodeHint;
    PVOID NodeFreeHint;
} MM_AVL_TABLE, *PMM_AVL_TABLE;

typedef struct _MMWSLENTRY
{
    ULONG Valid:1;
    ULONG LockedInWs:1;
    ULONG LockedInMemory:1;
    ULONG Protection:5;
    ULONG Hashed:1;
    ULONG Direct:1;
    ULONG Age:2;
    ULONG VirtualPageNumber:14;
} MMWSLENTRY, *PMMWSLENTRY;

typedef struct _MMWSLE
{
    union
    {
        PVOID VirtualAddress;
        ULONG Long;
        MMWSLENTRY e1;
    };
} MMWSLE, *PMMWSLE;

typedef struct _MMWSLE_HASH
{
    PVOID Key;
    ULONG Index;
} MMWSLE_HASH, *PMMWSLE_HASH;

typedef struct _MMWSL
{
    ULONG FirstFree;
    ULONG FirstDynamic;
    ULONG LastEntry;
    ULONG NextSlot;
    PMMWSLE Wsle;
    ULONG LastInitializedWsle;
    ULONG NonDirectcout;
    PMMWSLE_HASH HashTable;
    ULONG HashTableSize;
    ULONG NumberOfCommittedPageTables;
    PVOID HashTableStart;
    PVOID HighestPermittedHashAddress;
    ULONG NumberOfImageWaiters;
    ULONG VadBitMapHint;
    USHORT UsedPageTableEntries[768];
    ULONG CommittedPageTables[24];
} MMWSL, *PMMWSL;

typedef struct _MMSUPPORT_FLAGS
{
    ULONG SessionSpace:1;
    ULONG BeingTrimmed:1;
    ULONG SessionLeader:1;
    ULONG TrimHard:1;
    ULONG WorkingSetHard:1;
    ULONG AddressSpaceBeingDeleted :1;
    ULONG Available:10;
    ULONG AllowWorkingSetAdjustment:8;
    ULONG MemoryPriority:8;
} MMSUPPORT_FLAGS, *PMMSUPPORT_FLAGS;

typedef struct _MMSUPPORT
{
    LARGE_INTEGER LastTrimTime;
    MMSUPPORT_FLAGS Flags;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG MinimumWorkingSetSize;
    ULONG MaximumWorkingSetSize;
    PMMWSL MmWorkingSetList;
    LIST_ENTRY WorkingSetExpansionLinks;
    ULONG Claim;
    ULONG NextEstimationSlot;
    ULONG NextAgingSlot;
    ULONG EstimatedAvailable;
    ULONG GrowthSinceLastEstimate;
} MMSUPPORT, *PMMSUPPORT;

#endif
