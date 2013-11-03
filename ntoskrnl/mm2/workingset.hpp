/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/workingset.hpp
 * PURPOSE:         Working set header
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

#define GET_WS_OBJECT(_Process) ((WORKING_SET *)(&((_Process)->Vm)))

// Encapsulates MMWSLE
class WS_LIST_ENTRY
{
public:
    VOID SetVirtualAddress(PVOID Ptr)
    {
        u1.VirtualAddress = Ptr;
    }

    PVOID GetVirtualAddress()
    {
        return u1.VirtualAddress;
    }


    VOID InitValidDirect()
    {
        u1.e1.Valid = 1;
        u1.e1.LockedInWs = 1;
        u1.e1.Direct = 1;
    }

    VOID SetFree(ULONG Index)
    {
        u1.Long = Index << 4;
    }

    VOID SetEndOfList()
    {
        u1.Long = (ULONG)0xFFFFFFF << 4;
    }

private:
    union
    {
        PVOID VirtualAddress;
        ULONG Long;
        MMWSLENTRY e1;
    } u1;
};

// Encapsulates MMWSL
class WORKING_SET_LIST
{
public:
    VOID Initialize(PEPROCESS Process);

//private:
    ULONG Quota;
    ULONG FirstFree;
    ULONG FirstDynamic;
    ULONG LastEntry;
    ULONG NextSlot;
    WS_LIST_ENTRY* Wsle;
    ULONG LastInitializedWsle;
    ULONG NonDirectCount;
    PMMWSLE_HASH HashTable;
    ULONG HashTableSize;
    ULONG NumberOfCommittedPageTables;
    PVOID HashTableStart;
    PVOID HighestPermittedHashAddress;
    ULONG NumberOfImageWaiters;
    ULONG VadBitMapHint;
    USHORT UsedPageTableEntries[768];
    ULONG CommittedPageTables[24];
    PKEVENT ImageMappingEvent;
};

// Encapsulates MMSUPPORT stuff from EPROCESS.
class WORKING_SET
{
public:
    LARGE_INTEGER LastTrimTime;
    ULONG LastTrimFaultCount;
    ULONG PageFaultCount;
    ULONG PeakSize;
    ULONG CurrentSize;
    ULONG MinimumSize;
    ULONG MaximumSize;
    WORKING_SET_LIST *VmWorkingSetList;
    LIST_ENTRY ExpansionLinks;
    UCHAR AllowAdjustment;
    BOOLEAN AddressSpaceBeingDeleted;
    UCHAR ForegroundSwitchCount;
    UCHAR MemoryPriority;

    VOID UpdateLastTrimTime()
    {
        KeQuerySystemTime(&LastTrimTime);
    }

    VOID SetWorkingListSet(WORKING_SET_LIST *WorkingSetList)
    {
        VmWorkingSetList = WorkingSetList;
    }

private:
};

C_ASSERT(sizeof(WORKING_SET) <= sizeof(MMSUPPORT));
