/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/pfndb.hpp
 * PURPOSE:         PFN database header
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

typedef enum _MI_PFN_CACHE_ATTRIBUTE
{
    MiNonCached,
    MiCached,
    MiWriteCombined,
    MiNotMapped
} MI_PFN_CACHE_ATTRIBUTE, *PMI_PFN_CACHE_ATTRIBUTE;

//
// These two mappings are actually used by Windows itself, based on the ASSERTS
//
#define StartOfAllocation ReadInProgress
#define EndOfAllocation WriteInProgress

typedef struct _MMPFNENTRY
{
    USHORT Modified:1;
    USHORT ReadInProgress:1;                 // StartOfAllocation
    USHORT WriteInProgress:1;                // EndOfAllocation
    USHORT PrototypePte:1;
    USHORT PageColor:4;
    USHORT PageLocation:3;
    USHORT RemovalRequested:1;
    USHORT CacheAttribute:2;
    USHORT Rom:1;
    USHORT ParityError:1;                    // HasRmap
} MMPFNENTRY;

typedef struct _MMPFN
{
    union
    {
        PFN_NUMBER Flink;
        ULONG WsIndex;                       // SavedSwapEntry
        PKEVENT Event;
        NTSTATUS ReadStatus;
        SINGLE_LIST_ENTRY NextStackPfn;
    } u1;
    PTENTRY *PteAddress;
    union
    {
        PFN_NUMBER Blink;
        ULONG_PTR ShareCount;
    } u2;
    union
    {
        struct
        {
            USHORT ReferenceCount;           // ReferenceCount
            MMPFNENTRY e1;
        };
        struct
        {
            USHORT ReferenceCount;
            USHORT ShortFlags;
        } e2;
    } u3;
    union
    {
        PTENTRY OriginalPte;
        LONG AweReferenceCount;              // RmapListHead
    };
    union
    {
        ULONG_PTR EntireFrame;
        struct
        {
            ULONG_PTR PteFrame:25;
            ULONG_PTR InPageError:1;
            ULONG_PTR VerifierAllocation:1;
            ULONG_PTR AweAllocation:1;
            ULONG_PTR Priority:3;
            ULONG_PTR MustBeCached:1;
        };
    } u4;
#if MI_TRACE_PFNS
    MI_PFN_USAGES PfnUsage;
    CHAR ProcessName[16];
#endif
} MMPFN, *PMMPFN;

extern PMMPFN MmPfnDatabase;

typedef struct _MMPFNLIST
{
    PFN_NUMBER Total;
    MMLISTS ListName;
    PFN_NUMBER Flink;
    PFN_NUMBER Blink;
} MMPFNLIST, *PMMPFNLIST;

extern MMPFNLIST MmZeroedPageListHead;
extern MMPFNLIST MmFreePageListHead;
extern MMPFNLIST MmStandbyPageListHead;
extern MMPFNLIST MmModifiedPageListHead;
extern MMPFNLIST MmModifiedNoWritePageListHead;

typedef struct _MMCOLOR_TABLES
{
    PFN_NUMBER Flink;
    PVOID Blink;
    PFN_NUMBER Count;
} MMCOLOR_TABLES, *PMMCOLOR_TABLES;

class PFN_DATABASE
{
public:
    PMMCOLOR_TABLES FreePagesByColor[2]; // public due to MEMORY_MANAGER access

    VOID Map(PLOADER_PARAMETER_BLOCK LoaderBlock, BOOLEAN LargePages);
    VOID InitializePfnDatabase(PLOADER_PARAMETER_BLOCK LoaderBlock, BOOLEAN LargePages);
    VOID InitializeElement(ULONG PageFrameIndex, PTENTRY *PointerPte, ULONG Modified);
    VOID InitializeFreePagesByColor();
    ULONG RemoveAnyPage(ULONG Color);
    ULONG RemoveZeroPage(ULONG Color);
    VOID RemovePageByColor(PFN_NUMBER PageIndex, ULONG Color);
    ULONG RemovePageFromList(PMMPFNLIST ListHead);
    VOID InsertInList(PMMPFNLIST ListHead, PFN_NUMBER Pfn);

    PMMPFN GetEntry(PFN_NUMBER Pfn)
    {
        /* Get the entry */
        PMMPFN Page = &PfnDatabase[Pfn];
        return Page;
    }

    PFN_NUMBER GetIndexFromPhysical(ULONG_PTR Addr);

    static PFN_NUMBER GetAllocationSize();
    ULONG GetAvailablePages() { return AvailablePages; };

    KIRQL AcquireLock()
    {
        // Lock PFN database
        return KeAcquireQueuedSpinLock(LockQueuePfnLock);
    };

    VOID ReleaseLock(KIRQL OldIrql)
    {
        // Release PFN lock
        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    };

private:
    VOID BuildPfnDatabaseFromPages(IN PLOADER_PARAMETER_BLOCK LoaderBlock);
    VOID BuildPfnDatabaseZeroPage(VOID);
    VOID BuildPfnDatabaseFromLoaderBlock(IN PLOADER_PARAMETER_BLOCK LoaderBlock, BOOLEAN LargePages);
    VOID BuildPfnDatabaseSelf();

    VOID DbgCheckPage(PFN_NUMBER PageIndex)
    {
        // Sanity checks
        PMMPFN Pfn1 = GetEntry(PageIndex);
        ASSERT(Pfn1->u3.e2.ReferenceCount == 0);
        //ASSERT(Pfn1->u2.ShareCount == 0);
        //ASSERT(Pfn1->u3.e1.PageColor == MemoryManager->GetSecondaryColor(PageIndex));
    };

    PMMPFN PfnDatabase;

    // Page lists
    MMPFNLIST ZeroedPageListHead;
    MMPFNLIST FreePageListHead;
    MMPFNLIST StandbyPageListHead;
    MMPFNLIST ModifiedPageListHead;
    MMPFNLIST ModifiedNoWritePageListHead;
    MMPFNLIST BadPageListHead;
    PMMPFNLIST PageLocationList[TransitionPage + 1];

    // Modified pages for pagefile
    ULONG TotalPagesForPagingFile;
    MMPFNLIST ModifiedPageListByColor;

    BOOLEAN LocatedInPool;
    ULONG AvailablePages;

    const static ULONG EmptyList = 0xFFFFFFFF;
    PFN_NUMBER LowMemoryThreshold;
    PFN_NUMBER HighMemoryThreshold;

    KEVENT LowMemoryEvent;
    KEVENT HighMemoryEvent;
};