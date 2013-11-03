/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/memorymanager.hpp
 * PURPOSE:         Memory Manager class declaration
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

extern "C" {
#include <ntoskrnl.h>
}

/* Make the code cleaner with some definitions for size multiples */
#define _1KB (1024u)
#define _1MB (1024 * _1KB)
#define _1GB (1024 * _1MB)

/* Everyone loves 64K */
#define _64K (64 * _1KB)

/* Area mapped by a PDE */
#define PDE_MAPPED_VA  (PTE_COUNT * PAGE_SIZE)

/* Architecture specific count of PDEs in a directory, and count of PTEs in a PT */
#ifdef _M_IX86
#define PD_COUNT  1
#define PDE_COUNT 1024
#define PTE_COUNT 1024

// Address space
#define MI_HIGHEST_USER_ADDRESS       (PVOID)0x7FFEFFFF
#define MI_USER_PROBE_ADDRESS         (PVOID)0x7FFF0000
#define MI_DEFAULT_SYSTEM_RANGE_START (PVOID)0x80000000
#define MM_KSEG2_BASE                 ((ULONG_PTR)0xA0000000)
#define MI_SYSTEM_CACHE_START         (PVOID)0xC1000000
#define MI_SYSTEM_CACHE_WS_START      (PVOID)0xC0C00000
#else
#error Not set
#endif

//
// Protection Bits part of the internal memory manager Protection Mask, from:
// http://reactos.org/wiki/Techwiki:Memory_management_in_the_Windows_XP_kernel
// and public assertions.
//
#define MM_ZERO_ACCESS         0
#define MM_READONLY            1
#define MM_EXECUTE             2
#define MM_EXECUTE_READ        3
#define MM_READWRITE           4
#define MM_WRITECOPY           5
#define MM_EXECUTE_READWRITE   6
#define MM_EXECUTE_WRITECOPY   7
#define MM_NOCACHE             8
#define MM_DECOMMIT            0x10
#define MM_NOACCESS            (MM_DECOMMIT | MM_NOCACHE)
#define MM_INVALID_PROTECTION  0xFFFFFFFF


#include "pte.hpp"
#include "pfndb.hpp"
#include "systemptes.hpp"
#include "virtualmemory.hpp"
#include "pool.hpp"
#include "poolmanager.hpp"
#include "workingset.hpp"
#include "systemcache.hpp"

#ifndef _NEW_DELETE_OPERATORS_
#define _NEW_DELETE_OPERATORS_

inline PVOID operator new(
  size_t iSize,
  POOL_TYPE poolType)
{
  PVOID result = ExAllocatePoolWithTag(poolType,iSize,'wNCK');
  if (result) {
    RtlZeroMemory(result,iSize);
  }
  return result;
}

inline PVOID operator new(
  size_t iSize,
  POOL_TYPE poolType,
  ULONG tag)
{
  PVOID result = ExAllocatePoolWithTag(poolType,iSize,tag);
  if (result) {
    RtlZeroMemory(result,iSize);
  }
  return result;
}

inline void __cdecl operator delete(
  PVOID pVoid)
{
  if (pVoid) ExFreePool(pVoid);
}

#endif /* _NEW_DELETE_OPERATORS_ */

typedef struct _PHYSICAL_MEMORY_RUN
{
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR
{
    ULONG NumberOfRuns;
    PFN_NUMBER NumberOfPages;
    PHYSICAL_MEMORY_RUN Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR, *PPHYSICAL_MEMORY_DESCRIPTOR;

extern KSPIN_LOCK MmPfnLock; // Because of init/usage in krnlinit.c

class MEMORY_MANAGER
{
public:
    SYSTEM_PTES SystemPtes;
    POOL_MANAGER Pools;
    PFN_DATABASE PfnDb;

    // Stub PTE/PDEs
    PTENTRY ValidKernelPte;
    PTENTRY ZeroKernelPte;
    PTENTRY ValidKernelPde;
    PTENTRY DemandZeroPte;

    // FIXME: Public because of PFN_DATABASE::Map()
    // Loader block
    ULONG LbNumberDescriptors; /* Number of memory descriptors in the loader block */
    PFN_NUMBER LbNumberOfFreePages; /* Number of free pages in the loader block */
    PMEMORY_ALLOCATION_DESCRIPTOR LbFreeDescriptor;
    MEMORY_ALLOCATION_DESCRIPTOR LbOldFreeDescriptor;

    LIST_ENTRY WorkingSetExpansionHead;

    MEMORY_MANAGER();
    virtual ~MEMORY_MANAGER();

    BOOLEAN Initialize0(PLOADER_PARAMETER_BLOCK LoaderBlock);
    BOOLEAN Initialize1(PLOADER_PARAMETER_BLOCK LoaderBlock);
    BOOLEAN Initialize2(PLOADER_PARAMETER_BLOCK LoaderBlock);

    NTSTATUS InitializeProcessAddressSpace(PEPROCESS Process, PEPROCESS ProcessClone, PVOID Section, PULONG Flags, POBJECT_NAME_INFORMATION *AuditName);

    NTSTATUS HandleAccessFault(IN BOOLEAN StoreInstruction,
                               IN PVOID Address,
                               IN KPROCESSOR_MODE Mode,
                               IN PVOID TrapInformation);

    PFN_NUMBER GetHighestPhysicalPage() { return HighestPhysicalPage; };
    PFN_NUMBER GetLowestPhysicalPage() { return LowestPhysicalPage; };
    ULONG GetSecondaryColors() { return SecondaryColors; };
    ULONG GetSecondaryColor(ULONG Page) { return Page & SecondaryColorMask; };
    ULONG GetPageColor() { return (SystemPageColor++) & SecondaryColorMask; };
    ULONG GetMaxModifiedPages() { return 400; }
    PFN_NUMBER LbGetNextPage(PFN_NUMBER PageCount);
    ULONG GetMinimumFreePages() { return MinimumFreePages; };
    BOOLEAN IsAddressValid(IN PVOID Ptr);
    ULONG IsGlobalSupported() { return 0; };
    VOID WakeupZeroingThread();
    VOID WakeupMPWThread();

    WS_LIST_ENTRY *GetWsle() { return Wsle; };
    WORKING_SET_LIST *GetWorkingSetList() { return WorkingSetList; };
    ULONG GetWorkingSetLimit() { return WorkingSetLimit; };

    VOID GrowWsleHash(WORKING_SET *Ws, BOOLEAN PfnLockAcquired);
private:

protected:
    FAST_MUTEX SectionCommitMutex;
    FAST_MUTEX SectionBasedMutex;
    KMUTANT SystemLoadLock;
    ERESOURCE SystemWsLock;
    KSPIN_LOCK ChargeCommitmentLock;
    KSPIN_LOCK ExpansionLock;
    FAST_MUTEX PageFileCreationLock;
    ERESOURCE SectionExtendResource;
    ERESOURCE SectionExtendSetResource;

    // Events
    KEVENT AvailablePagesEvent;
    KEVENT AvailablePagesEventHigh;
    KEVENT MappedFileIoComplete;
    KEVENT ImageMappingPteEvent;
    KEVENT ZeroingPageEvent;
    KEVENT CollidedFlushEvent;
    KEVENT CollidedLockEvent;
    KEVENT ModifiedPageWriterEvent;
    BOOLEAN ZeroingPageThreadActive;

    // Lists
    LIST_ENTRY InPageSupportListHead;
    LIST_ENTRY EventCountListHead;

    // Misc
    //PFN_COUNT NumberOfPhysicalPages; // Use global MmNumberOfPhysicalPages instead for now
    ULONG TotalCommitLimit;
    ULONG OverCommit;
    //PVOID NonPagedSystemStart; // In SYSTEM_PTES instead
    PFN_NUMBER HighestPhysicalPage;
    PFN_NUMBER LowestPhysicalPage;
    BOOLEAN LargePagesMapped;
    PPHYSICAL_MEMORY_DESCRIPTOR PhysicalMemoryBlock;
    PFN_NUMBER ResidentAvailablePages; // FIXME: Move to PFN database?
    PFN_NUMBER ResidentAvailableAtInit; // FIXME: Move to PFN database?

    // System Cache
    SYSTEM_CACHE SystemCache;

    // Colors
    ULONG SystemPageColor;
    ULONG SecondaryColorMask;
    ULONG SecondaryColors;

    // WorkingSet
    ULONG WorkingSetLimit;
    ULONG MaxWorkingSetSize;
    ULONG MinWorkingSetSize;
    WORKING_SET_LIST *WorkingSetList;
    WS_LIST_ENTRY *Wsle;

    // MPW
    ULONG MinimumFreePages;

    // Mapping PTEs
    PTENTRY *FirstReservedMappingPte;
    PTENTRY *LastReservedMappingPte;

    // Functions
    VOID InitializeMemoryLimits(PLOADER_PARAMETER_BLOCK LoaderBlock, PBOOLEAN IncludeType, PPHYSICAL_MEMORY_DESCRIPTOR Memory);
    VOID ScanMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock);
    VOID BuildPagedPool();

    VOID MachineDependentInit0(PLOADER_PARAMETER_BLOCK LoaderBlock);
    VOID MachineDependentInit1(PLOADER_PARAMETER_BLOCK LoaderBlock);

    ULONG RemoveAnyPage(ULONG PageHash);
    ULONG RemoveZeroPage(ULONG PageHash);
    VOID RemovePageByHash(ULONG Page, ULONG Color);
    ULONG RemovePageFromList(PMMPFNLIST ListHead);

    VOID ObtainFreePages();
    VOID RestoreTransitionPte(ULONG PageFrameIndex);

    NTSTATUS HandleSystemAddressFault(IN BOOLEAN StoreInstruction,
                                      IN ULONG_PTR Address,
                                      IN KPROCESSOR_MODE Mode,
                                      IN PVOID TrapInformation,
                                      IN BOOLEAN *Continue);

    NTSTATUS CheckAndInitPde(ULONG_PTR Address);
};

extern MEMORY_MANAGER *MemoryManager;
