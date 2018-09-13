/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _CORE_BASE_MPHEAP
#include "core/base/mpheap.hxx"
#endif

#ifndef MSXMLPRFCOUNTERS_H
#include "core/prfdata/msxmlprfcounters.h"  // PrfHeapAlloc, PrfHeapFree
#endif

#if SMALLBLOCKHEAP
#define _CRTBLD 1
#include "../crt/winheap.h"
inline __sbh_heap_t * __sbh_address_to_heap (void * p)
{
        __sbh_page_t *    ppage;

        ppage = (__sbh_page_t *)((ULONG_PTR)p &
                          ~((UINT_PTR)_PAGESIZE_ - 1));
        return MARK_TO_POINTER(ppage->p_heap, __sbh_heap_t);
}

#endif

#if MPHEAP

HANDLE g_hMpHeap;
extern bool g_fMultiProcessor;
#ifdef RENTAL_MODEL
static DWORD s_dwAlignAdjustment = 0;
#endif

BOOL
MpHeapInit()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    g_fMultiProcessor = si.dwNumberOfProcessors > 1;
    g_hMpHeap = MpHeapCreate(0, 0, 0);
#ifdef RENTAL_MODEL
    if (g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        s_dwAlignAdjustment = REF_RENTAL;
#endif
    return g_hMpHeap != NULL;
}

// size_t EXPORT DbgTotalAllocated()

void
MpHeapExit()
{
#if DBG == 1
    if (!DbgTotalAllocated())
#endif
        MpHeapDestroy(g_hMpHeap);
}   

/*++ 
 
Abstract: 
 
    This DLL is a wrapper that sits on top of the Win32 Heap* api.  It 
    provides multiple heaps and handles all the serialization itself. 
 
    Many multithreaded applications that use the standard memory allocation 
    routines (malloc/free, LocalAlloc/LocalFree, HeapAlloc/HeapFree) suffer 
    a significant a significant performance penalty when running on a 
    multi-processor machine.  This is due to the serialization used by the 
    default heap package.  On a multiprocessor machine, more than one 
    thread may simultaneously try to allocate memory.  One thread will 
    block on the critical section guarding the heap.  The other thread must 
    then signal the critical section when it is finished to unblock the 
    waiting thread.  The additional codepath of blocking and signalling adds 
    significant overhead to the frequent memory allocation path. 
 
    By providing multiple heaps, this DLL allows simultaneous operations on 
    each heap.  A thread on processor 0 can allocate memory from one heap 
    at the same time that a thread on processor 1 is allocating from a 
    different heap.  The additional overhead in this DLL is compensated by 
    drastically reducing the number of times a thread must wait for heap 
    access. 
 
    The basic scheme is to attempt to lock each heap in turn with the new 
    TryEnterCriticalSection API.  This will enter the critical section if 
    it is unowned.  If the critical section is owned by a different thread, 
    TryEnterCriticalSection returns failure instead of blocking until the 
    other thread leaves the critical section. 
 
    Another trick to increase performance is the use of a lookaside list to 
    satisfy frequent allocations.  By using InterlockedExchange to remove 
    lookaside list entries and InterlockedCompareExchange to add lookaside 
    list entries, allocations and frees can be completed without needing a 
    critical section lock. 
 
    The final trick is the use of delayed frees.  If a chunk of memory is 
    being freed, and the required lock is already held by a different 
    thread, the free block is simply added to a delayed free list and the 
    API completes immediately.  The next thread to acquire the heap lock 
    will free everything on the list. 
 
    Every application uses memory allocation routines in different ways. 
    In order to allow better tuning of this package, MpHeapGetStatistics 
    allows an application to monitor the amount of contention it is 
    getting.  Increasing the number of heaps increases the potential 
    concurrency, but also increases memory overhead.  Some experimentation 
    is recommended to determine the optimal settings for a given number of 
    processors. 
 
    Some applications can benefit from additional techniques.  For example, 
    per-thread lookaside lists for common allocation sizes can be very 
    effective.  No locking is required for a per-thread structure, since no 
    other thread will ever be accessing it.  Since each thread reuses the 
    same memory, per-thread structures also improve locality of reference. 
 
--*/ 
 
#define MPHEAP_VALID_OPTIONS  0
/*
                              (MPHEAP_GROWABLE                 | \ 
                               MPHEAP_REALLOC_IN_PLACE_ONLY    | \ 
                               MPHEAP_TAIL_CHECKING_ENABLED    | \ 
                               MPHEAP_FREE_CHECKING_ENABLED    | \ 
                               MPHEAP_DISABLE_COALESCE_ON_FREE | \ 
                               MPHEAP_ZERO_MEMORY              | \ 
                               MPHEAP_COLLECT_STATS) 
*/ 
// 
// Flags that are not passed on to the Win32 heap package 
// 
#define MPHEAP_PRIVATE_FLAGS (MPHEAP_COLLECT_STATS | MPHEAP_ZERO_MEMORY); 
 
// 
// Define the heap header that gets tacked on the front of 
// every allocation. Eight bytes is a lot, but we can't make 
// it any smaller or else the allocation will not be properly 
// aligned for 64-bit quantities. 
// 
typedef struct _MP_HEADER { 
    union { 
        struct _MP_HEAP_ENTRY *HeapEntry; 
        PSINGLE_LIST_ENTRY Next; 
    }; 
    ULONG LookasideIndex; 
} MP_HEADER, *PMP_HEADER; 
// 
// Definitions and structures for lookaside list 
// 
#define LIST_ENTRIES 128 
 
typedef struct _MP_HEAP_LOOKASIDE { 
    PMP_HEADER Entry; 
} MP_HEAP_LOOKASIDE, *PMP_HEAP_LOOKASIDE; 
 
#define NO_LOOKASIDE 0x0fffffff
#define SMALL_BLOCK_HEAP_HEADER 0x0ffffffe
#define MARK_MISALIGNMENT 0x80000000
#define MASK_MISALIGNMENT(p) (p & ~MARK_MISALIGNMENT)
#define MaxLookasideSize (8*LIST_ENTRIES-7) 
#define LookasideIndexFromSize(s) ((s < MaxLookasideSize) ? ((s) >> 3) : NO_LOOKASIDE) 
// 
// Define the structure that describes the entire MP heap. 
// 
// There is one MP_HEAP_ENTRY structure for each Win32 heap 
// and a MP_HEAP structure that contains them all. 
// 
// Each MP_HEAP structure contains a lookaside list for quick 
// lock-free alloc/free of various size blocks. 
// 
 
typedef struct _MP_HEAP_ENTRY { 

    _MP_HEAP_ENTRY(HANDLE h);
    ~_MP_HEAP_ENTRY();

    void * operator new(size_t s, void * p)
    {
        return p;
    }

    void operator delete(void * p)
    {
    }

    HANDLE Heap; 
#if SMALLBLOCKHEAP
    __sbh_heap_t SBHeap;
#endif
    PSINGLE_LIST_ENTRY DelayedFreeList;
    ShareMutex Lock;
    DWORD Allocations; 
    DWORD Frees; 
    DWORD LookasideAllocations; 
    DWORD LookasideFrees; 
    DWORD DelayedFrees; 
    MP_HEAP_LOOKASIDE Lookaside[LIST_ENTRIES]; 
} MP_HEAP_ENTRY, *PMP_HEAP_ENTRY; 
 
_MP_HEAP_ENTRY::_MP_HEAP_ENTRY(HANDLE h)
{
    Heap = h;
    DelayedFreeList = NULL; 
    ZeroMemory(&Lookaside, sizeof(Lookaside)); 
#if SMALLBLOCKHEAP
    __sbh_init_heap(&SBHeap);
#endif
}

_MP_HEAP_ENTRY::~_MP_HEAP_ENTRY()
{
#if SMALLBLOCKHEAP
    __sbh_finish_heap(&SBHeap);
#endif
    Lock.EmbeddedRelease();
    if (Heap)
        HeapDestroy(Heap);
}
 
typedef struct _MP_HEAP { 
    DWORD HeapCount; 
    DWORD Flags; 
    DWORD Hint; 
    DWORD PadTo32Bytes; 
    MP_HEAP_ENTRY Entry[1];     // variable size 
} MP_HEAP, *PMP_HEAP; 
 
VOID 
ProcessDelayedFreeList( 
    IN PMP_HEAP_ENTRY HeapEntry 
    ); 
 
// 
// _dwHeapHint is a per-thread variable that offers a hint as to which heap to 
// check first.  By giving each thread affinity towards a different heap, 
// it is more likely that the first heap a thread picks for its allocation 
// will be available.  It also improves a thread's locality of reference, 
// which is very important for good MP performance 
// 
 
HANDLE 
WINAPI 
MpHeapCreate( 
    DWORD flOptions, 
    DWORD dwInitialSize, 
    DWORD dwParallelism 
    ) 
/*++ 
 
Routine Description: 
 
    This routine creates an MP-enhanced heap. An MP heap consists of a 
    collection of standard Win32 heaps whose serialization is controlled 
    by the routines in this module to allow multiple simultaneous allocations. 
 
Arguments: 
 
    flOptions - Supplies the options for this heap. 
 
        Currently valid flags are: 
 
            MPHEAP_GROWABLE 
            MPHEAP_REALLOC_IN_PLACE_ONLY 
            MPHEAP_TAIL_CHECKING_ENABLED 
            MPHEAP_FREE_CHECKING_ENABLED 
            MPHEAP_DISABLE_COALESCE_ON_FREE 
            MPHEAP_ZERO_MEMORY 
            MPHEAP_COLLECT_STATS 
 
    dwInitialSize - Supplies the initial size of the combined heaps. 
 
    dwParallelism - Supplies the number of Win32 heaps that will make up the 
        MP heap. A value of zero defaults to three + # of processors. 
 
Return Value: 
 
    HANDLE - Returns a handle to the MP heap that can be passed to the 
             other routines in this package. 
 
    NULL - Failure, GetLastError() specifies the exact error code. 
 
--*/ 
{ 
    DWORD Error; 
    DWORD i; 
    HANDLE Heap; 
    PMP_HEAP MpHeap; 
    DWORD HeapSize; 
    DWORD PrivateFlags; 
 
    if (flOptions & ~MPHEAP_VALID_OPTIONS) { 
        SetLastError(ERROR_INVALID_PARAMETER); 
        return(NULL); 
    } 
 
    flOptions |= HEAP_NO_SERIALIZE; 
 
    PrivateFlags = flOptions & MPHEAP_PRIVATE_FLAGS; 
 
    flOptions &= ~MPHEAP_PRIVATE_FLAGS; 
 
    if (dwParallelism == 0) { 
        SYSTEM_INFO SystemInfo; 
 
        GetSystemInfo(&SystemInfo); 
        dwParallelism = 3 + SystemInfo.dwNumberOfProcessors; 
    } 
 
    HeapSize = dwInitialSize / dwParallelism; 
 
    // 
    // The first heap is special, since the MP_HEAP structure itself 
    // is allocated from there. 
    // 
    Heap = HeapCreate(flOptions,HeapSize,0); 
    if (Heap == NULL) { 
        // 
        // HeapCreate has already set last error appropriately. 
        // 
        return(NULL); 
    } 
 
    MpHeap = (PMP_HEAP)PrfHeapAlloc(Heap, 0, sizeof(MP_HEAP) + 
        (dwParallelism-1)*sizeof(MP_HEAP_ENTRY)); 
    if (MpHeap==NULL) { 
        SetLastError(ERROR_NOT_ENOUGH_MEMORY); 
        HeapDestroy(Heap); 
        return(NULL); 
    } 
    // 
    // Initialize the MP heap structure 
    // 
    MpHeap->HeapCount = 1; 
    MpHeap->Flags = PrivateFlags; 
    MpHeap->Hint = 0; 
 
    // 
    // Initialize the first heap 
    // 
    new (&MpHeap->Entry[0]) _MP_HEAP_ENTRY(Heap);
 
    // 
    // Initialize the remaining heaps. Note that the heap has been 
    // sufficiently initialized to use MpHeapDestroy for cleanup 
    // if something bad happens. 
    // 
    for (i=1; i<dwParallelism; i++) { 
        Heap = HeapCreate(flOptions, HeapSize, 0);
        if (Heap == NULL) { 
            Error = GetLastError(); 
            MpHeapDestroy((HANDLE)MpHeap); 
            SetLastError(Error); 
            return(NULL); 
        } 
        new  (&MpHeap->Entry[i]) _MP_HEAP_ENTRY(Heap);
        ++MpHeap->HeapCount; 
    } 
 
    return((HANDLE)MpHeap); 
} 
 
BOOL 
WINAPI 
MpHeapDestroy( 
    HANDLE hMpHeap 
    ) 
{ 
    DWORD i; 
    DWORD HeapCount; 
    PMP_HEAP MpHeap; 
    BOOL Success = TRUE; 
 
    MpHeap = (PMP_HEAP)hMpHeap; 
    HeapCount = MpHeap->HeapCount; 
 
    // 
    // Lock down all the heaps so we don't end up hosing people 
    // who may be allocating things while we are deleting the heaps. 
    // By setting MpHeap->HeapCount = 0 we also attempt to prevent 
    // people from getting hosed as soon as we delete the critical 
    // sections and heaps. 
    // 
    MpHeap->HeapCount = 0; 
    for (i=0; i<HeapCount; i++) { 
        MpHeap->Entry[i].Lock.ClaimExclusiveLock();
        ProcessDelayedFreeList(&MpHeap->Entry[i]); 
    } 
 
    // 
    // Delete the heaps and their associated critical sections. 
    // Note that the order is important here. Since the MpHeap 
    // structure was allocated from MpHeap->Heap[0] we must 
    // delete that last. 
    // 
    for (i=HeapCount-1; i>0; i--) {
        delete &MpHeap->Entry[i];
    } 
    HANDLE Heap = MpHeap->Entry[0].Heap;
    MpHeap->Entry[0].Heap = NULL;
    delete &MpHeap->Entry[0];
    HeapDestroy(Heap); 

    return(Success); 
} 
 
BOOL 
WINAPI 
MpHeapValidate( 
    HANDLE hMpHeap, 
    LPVOID lpMem 
    ) 
{ 
    PMP_HEAP MpHeap; 
    DWORD i; 
    BOOL Success; 
    PMP_HEADER Header; 
    PMP_HEAP_ENTRY Entry; 
 
    MpHeap = (PMP_HEAP)hMpHeap; 
 
    if (lpMem == NULL) { 
 
        // 
        // Lock and validate each heap in turn. 
        // 
        for (i=0; i < MpHeap->HeapCount; i++) { 
            Entry = &MpHeap->Entry[i]; 
            TRY
            { 
                MutexLock lock(&Entry->Lock); 
#if SMALLBLOCKHEAP
                Success = __sbh_heap_check(&Entry->SBHeap) == 0;
                if (Success)
#endif
                    Success = HeapValidate(Entry->Heap, 0, NULL); 
            } 
            CATCHE
            { 
                return(FALSE); 
            } 
            ENDTRY
 
            if (!Success) { 
                return(FALSE); 
            } 
        } 
        return(TRUE); 
    } else { 
 
        // 
        // Lock and validate the given heap entry 
        // 
#if SMALLBLOCKHEAP
        __sbh_heap_t * pheap;
        __sbh_region_t * preg;
        __sbh_page_t * ppage;
        __page_map_t * pmap;
        int Index;

        pheap = __sbh_address_to_heap(lpMem);
        Entry = (PMP_HEAP_ENTRY)((BYTE *)pheap - (BYTE *)&((PMP_HEAP_ENTRY)0)->SBHeap);
        Index = (int)(Entry - &MpHeap->Entry[0]);
        if (Index >= 0 && Index < (int)MpHeap->HeapCount &&
            pheap == &MpHeap->Entry[Index].SBHeap)
        {
            MutexLock lock(&Entry->Lock); 
            return __sbh_heap_check(pheap) == 0;
        }
#endif

        Header = ((PMP_HEADER)lpMem) - 1; 
        TRY
        { 
            MutexLock lock(&Header->HeapEntry->Lock); 
            Success = HeapValidate(Header->HeapEntry->Heap, 0, Header); 
        }
        CATCHE
        { 
            return(FALSE); 
        } 
        ENDTRY
        return(Success); 
    } 
}
 
/* 
UINT 
WINAPI 
MpHeapCompact( 
    HANDLE hMpHeap 
    ) 
{ 
    PMP_HEAP MpHeap; 
    DWORD i; 
    DWORD LargestFreeSize=0; 
    DWORD FreeSize; 
    PMP_HEAP_ENTRY Entry; 
 
    MpHeap = (PMP_HEAP)hMpHeap; 
 
    // 
    // Lock and compact each heap in turn. 
    // 
    for (i=0; i < MpHeap->HeapCount; i++) { 
        Entry = &MpHeap->Entry[i]; 
        {
            MutexLock lock(&Entry->Lock); 
            FreeSize = HeapCompact(Entry->Heap, 0); 
        }
        if (FreeSize > LargestFreeSize) { 
            LargestFreeSize = FreeSize; 
        } 
    } 
 
    return(LargestFreeSize); 
 
} 
*/
  
LPVOID 
WINAPI 
MpHeapAlloc( 
    HANDLE hMpHeap, 
    DWORD flOptions, 
    DWORD dwBytes 
    ) 
{ 
    PMP_HEADER Header; 
    PMP_HEAP MpHeap; 
    DWORD i; 
    PMP_HEAP_ENTRY Entry; 
    DWORD Index; 
    DWORD Size; 
    DWORD Hint;
 
    MpHeap = (PMP_HEAP)hMpHeap; 
 
    flOptions |= MpHeap->Flags; 

    Assert(dwBytes != 0);
 
    Size = ((dwBytes + 7) & (ULONG)~7) + sizeof(MP_HEADER); 
    Index=LookasideIndexFromSize(Size); 
 
    // 
    // Iterate through the heap locks looking for one 
    // that is not owned. 
    // 
    TLSDATA * ptlsdata = GetTlsData();
    if (ptlsdata)
        Hint = i = ptlsdata->_dwHeapHint; 
    else
        Hint = i = 0;
    if (i>=MpHeap->HeapCount) { 
        i=0; 
        ptlsdata->_dwHeapHint=0; 
    } 
    Entry = &MpHeap->Entry[i]; 
    do { 
        // 
        // Check the lookaside list for a suitable allocation. 
        // 
        if ((Index != NO_LOOKASIDE) && 
            (Entry->Lookaside[Index].Entry != NULL)) { 
            if ((Header = (PMP_HEADER)INTERLOCKEDEXCHANGE_PTR(&Entry->Lookaside[Index].Entry, 
                                                          NULL)) != NULL) { 
                // 
                // We have a lookaside hit, return it immediately. 
                // 
                ++Entry->LookasideAllocations; 
                Assert(MASK_MISALIGNMENT(Header->LookasideIndex) == Index);
                if (flOptions & MPHEAP_ZERO_MEMORY) { 
                    ZeroMemory(Header + 1, dwBytes); 
                }
                if (ptlsdata)
                    ptlsdata->_dwHeapHint=i; 
                return(Header + 1); 
            } 
        } 
 
        // 
        // Attempt to lock this heap without blocking. 
        // 
        if (Entry->Lock.TryEnter()) { 
            // 
            // success, go allocate immediately 
            // 
            goto LockAcquired; 
        } 
 
        // 
        // This heap is owned by another thread, try 
        // the next one. 
        // 
        i++; 
        Entry++; 
        if (i==MpHeap->HeapCount) { 
            i=0; 
            Entry=&MpHeap->Entry[0]; 
        } 
    } while ( i != Hint ); 
 
    // 
    // All of the critical sections were owned by someone else, 
    // so we have no choice but to wait for a critical section. 
    // 
    Entry->Lock.ClaimExclusiveLock(); 
 
LockAcquired: 
    ++Entry->Allocations; 
    if (Entry->DelayedFreeList != NULL) { 
        ProcessDelayedFreeList(Entry); 
    } 
#if SMALLBLOCKHEAP
#ifdef SPECIAL_OBJECT_ALLOCATION
    if (flOptions & MPHEAP_OBJECT)
#endif
    {
        /* round up to the nearest paragraph */
#ifdef SPECIAL_OBJECT_ALLOCATION
        Assert(dwBytes);
#else
        if (!dwBytes)
            dwBytes = 1;
#endif
        size_t cbr = (dwBytes + _PARASIZE - 1) & ~(_PARASIZE - 1);

        if (cbr < _DEFAULT_THRESHOLD)
        {
            void * pv = __sbh_alloc_block(&Entry->SBHeap, cbr >> _PARASHIFT);
            if (pv)
            {
                Entry->Lock.ReleaseExclusiveLock(); 
                if (flOptions & HEAP_ZERO_MEMORY)
                    ZeroMemory(pv, dwBytes);
                if (ptlsdata)
                    ptlsdata->_dwHeapHint = i;
#ifdef SPECIAL_OBJECT_ALLOCATION
                Assert(isObjectRegion(pv));
#endif
                return pv;
            }
        }
    }
#endif
#ifdef RENTAL_MODEL
    Header = (PMP_HEADER)PrfHeapAlloc(Entry->Heap, 0, Size + s_dwAlignAdjustment); 
#else
    Header = (PMP_HEADER)PrfHeapAlloc(Entry->Heap, 0, Size); 
#endif
    // we use the free block as header in case it is added to the delayed list
    Assert(Size >= 2 * sizeof(MP_HEADER));
    Entry->Lock.ReleaseExclusiveLock(); 
    if (Header != NULL) { 
#ifdef RENTAL_MODEL
        if ((INT_PTR)Header & REF_RENTAL)
        {
            Header = (PMP_HEADER)((BYTE *)Header + REF_RENTAL);
            Index |= MARK_MISALIGNMENT;
        }
#endif
        Header->HeapEntry = Entry; 
        Header->LookasideIndex = Index; 
        if (flOptions & MPHEAP_ZERO_MEMORY) { 
            ZeroMemory(Header + 1, dwBytes); 
        }
        if (ptlsdata)
            ptlsdata->_dwHeapHint = i; 
        return(Header + 1); 
    } else { 
        return(NULL); 
    } 
} 

/*
LPVOID 
WINAPI 
MpHeapReAlloc( 
    HANDLE hMpHeap, 
    LPVOID lpMem, 
    DWORD dwBytes 
    ) 
{ 
    PMP_HEADER Header; 
 
    Header = ((PMP_HEADER)lpMem) - 1; 
    dwBytes = ((dwBytes + 7) & (ULONG)~7) + sizeof(MP_HEADER); 
 
    {
        MutexLock lock(&Header->HeapEntry->Lock);
        Header = (PMP_HEADER)HeapReAlloc(Header->HeapEntry->Heap, 0, Header, dwBytes); 
    }
    
    if (Header != NULL) { 
        Header->LookasideIndex = LookasideIndexFromSize(dwBytes); 
        return(Header + 1); 
    } else { 
        return(NULL); 
    } 
} 
*/

BOOL 
WINAPI 
MpHeapFree( 
    HANDLE hMpHeap, 
    LPVOID lpMem 
    ) 
{ 
    PMP_HEADER Header; 
    BOOL Success; 
    PMP_HEAP_ENTRY HeapEntry; 
    PSINGLE_LIST_ENTRY Next; 
    PMP_HEAP MpHeap; 
 
    MpHeap = (PMP_HEAP)hMpHeap;

    TLSDATA * ptlsdata = GetTlsData();

#if SMALLBLOCKHEAP
    __sbh_heap_t * pheap;
    __sbh_region_t * preg;
    __sbh_page_t * ppage;
    __page_map_t * pmap;
    int Index;

    pheap = __sbh_address_to_heap(lpMem);
    HeapEntry = CONTAINING_RECORD(pheap, MP_HEAP_ENTRY, SBHeap);
    Index = (int)(HeapEntry - &MpHeap->Entry[0]);
    if (Index >= 0 && Index < (int)MpHeap->HeapCount &&
        pheap == &MpHeap->Entry[Index].SBHeap)
    {
        if (HeapEntry->Lock.TryEnter()) 
        { 
            if ((pmap = __sbh_find_block(pheap, lpMem, &preg, &ppage)) != NULL)  
            {
                ++HeapEntry->Frees; 
                if (ptlsdata)
                    ptlsdata->_dwHeapHint = Index;
                __sbh_free_block(pheap, preg, ppage, pmap);
                HeapEntry->Lock.ReleaseExclusiveLock(); 
                return TRUE;
            }
            HeapEntry->Lock.ReleaseExclusiveLock(); 
        }
        // we always allocate at least MP_HEADER size bytes so we can 
        // use the free block as the header...
        Header = (PMP_HEADER)lpMem;
        // to differentiate from normal heap objects
        Header->LookasideIndex = SMALL_BLOCK_HEAP_HEADER;
    }
    else
    {
#endif

        Header = ((PMP_HEADER)lpMem) - 1; 
        HeapEntry = Header->HeapEntry; 
 
        if (ptlsdata)
            ptlsdata->_dwHeapHint = (DWORD)(HeapEntry - &MpHeap->Entry[0]); 
 
        if (MASK_MISALIGNMENT(Header->LookasideIndex) != NO_LOOKASIDE) { 
            // 
            // Try and put this back on the lookaside list 
            // 
            if (InterlockedCompareExchange((PVOID*)&HeapEntry->Lookaside[MASK_MISALIGNMENT(Header->LookasideIndex)], 
                                           Header, 
                                           NULL) == NULL) { 
                // 
                // Successfully freed to lookaside list. 
                // 
                ++HeapEntry->LookasideFrees; 
                return(TRUE); 
            } 
        } 
 
        if (HeapEntry->Lock.TryEnter()) { 
            ++HeapEntry->Frees; 
#ifdef RENTAL_MODEL
            if (Header->LookasideIndex & MARK_MISALIGNMENT)
                Header = (PMP_HEADER)((BYTE *)Header - REF_RENTAL);
#endif
            Success = PrfHeapFree(HeapEntry->Heap, 0, Header); 
            HeapEntry->Lock.ReleaseExclusiveLock(); 
            return(Success); 
        } 

#if SMALLBLOCKHEAP
    }
#endif

    // 
    // The necessary heap critical section could not be immediately 
    // acquired. Post this free onto the Delayed free list and let 
    // whoever has the lock process it. 
    // 
    do { 
        Next = HeapEntry->DelayedFreeList; 
        Header->Next = Next; 
    } while ( (PVOID)InterlockedCompareExchange((PVOID*)&HeapEntry->DelayedFreeList, 
                                         &Header->Next, 
                                         Next) != (VOID*)Next); 
    return(TRUE); 
} 
 
VOID 
ProcessDelayedFreeList( 
    IN PMP_HEAP_ENTRY HeapEntry 
    ) 
{ 
    PSINGLE_LIST_ENTRY FreeList; 
    PSINGLE_LIST_ENTRY Next; 
    PMP_HEADER Header; 
#if SMALLBLOCKHEAP
    __sbh_heap_t * pheap;
    __sbh_region_t * preg;
    __sbh_page_t * ppage;
    __page_map_t * pmap;
#endif
    
    // 
    // Capture the entire delayed free list with a single interlocked exchange. 
    // Once we have removed the entire list, free each entry in turn. 
    // 
    FreeList = (PSINGLE_LIST_ENTRY)INTERLOCKEDEXCHANGE_PTR(&HeapEntry->DelayedFreeList, NULL); 
    while (FreeList != NULL) { 
        Next = FreeList->Next; 
        Header = CONTAINING_RECORD(FreeList, MP_HEADER, Next); 
        ++HeapEntry->DelayedFrees; 
#if SMALLBLOCKHEAP
        if (Header->LookasideIndex == SMALL_BLOCK_HEAP_HEADER)
        {
            // this was a potential small block heap allocation
            pheap = &HeapEntry->SBHeap;
            if ((pmap = __sbh_find_block(pheap, Header, &preg, &ppage)) != NULL)  
            {
                __sbh_free_block(pheap, preg, ppage, pmap);
            }
            else
            {
                Header = Header - 1;
                Next = FreeList->Next; 
                Assert(Header->HeapEntry == HeapEntry);
                PrfHeapFree(HeapEntry->Heap, 0, Header); 
            }
        }
        else
#endif
        {
#ifdef RENTAL_MODEL
            if (Header->LookasideIndex & MARK_MISALIGNMENT)
                Header = (PMP_HEADER)((BYTE *)Header - REF_RENTAL);
#endif
            PrfHeapFree(HeapEntry->Heap, 0, Header); 
        }
        FreeList = Next; 
    } 
} 
 
DWORD 
MpHeapGetStatistics( 
    HANDLE hMpHeap, 
    LPDWORD lpdwSize, 
    MPHEAP_STATISTICS Stats[] 
    ) 
{ 
    PMP_HEAP MpHeap; 
    PMP_HEAP_ENTRY Entry; 
    DWORD i; 
    DWORD RequiredSize; 
 
    MpHeap = (PMP_HEAP)hMpHeap; 
    RequiredSize = MpHeap->HeapCount * sizeof(MPHEAP_STATISTICS); 
    if (*lpdwSize < RequiredSize) { 
        *lpdwSize = RequiredSize; 
        return(ERROR_MORE_DATA); 
    } 
    ZeroMemory(Stats, MpHeap->HeapCount * sizeof(MPHEAP_STATISTICS)); 
    for (i=0; i < MpHeap->HeapCount; i++) { 
        Entry = &MpHeap->Entry[i]; 
 
//        Stats[i].Contention = Entry->Lock.DebugInfo->ContentionCount; 
        Stats[i].TotalAllocates = (Entry->Allocations + Entry->LookasideAllocations); 
        Stats[i].TotalFrees = (Entry->Frees + Entry->LookasideFrees + Entry->DelayedFrees); 
        Stats[i].LookasideAllocates = Entry->LookasideAllocations; 
        Stats[i].LookasideFrees = Entry->LookasideFrees; 
        Stats[i].DelayedFrees = Entry->DelayedFrees; 
    } 
    *lpdwSize = RequiredSize; 
    return(ERROR_SUCCESS); 
}

#endif

#ifdef SPECIAL_OBJECT_ALLOCATION

#ifdef _WIN64
// WIN64 BUGBUG - With the current 64 bit compiler/linker I need to do this otherwise g_abRegion is unresolved
extern "C"{
#endif 
// WIN64 BUGBUG - Trying sizeof(INT_PTR) or sizeof(__int64) gives 0 length for array "compiler error"
const INT_PTR c_cabRegion = 1 << (sizeof(int) * 8 - REGION_GRANULARITY_BITS  - 3);
BYTE g_abRegion[c_cabRegion];
#ifdef _WIN64
}
#endif 

extern CSMutex * g_pMutex;

void addObjectRegion(void * pRegion, long lSize)
{
    if (g_pMutex)
        g_pMutex->Enter();

    UINT_PTR p = (UINT_PTR)pRegion;

//    TraceTag((0, "Alloc region: %p - %p", p, p + lSize));
    for (long i = 0; i < lSize; i += 1 << REGION_GRANULARITY_BITS)
    {
        UINT_PTR q = p >> REGION_GRANULARITY_BITS;
        g_abRegion[q >> 3] |= 1 << (q & 7);
        p += 1 << REGION_GRANULARITY_BITS;
    }
    if (g_pMutex)
        g_pMutex->Leave();

#if NEVER
    TCHAR buf[40] = _T("\r\nAdded:");
    _itot((int)pRegion, buf + 8, 16);
    OutputDebugString(buf);
#endif
}

void delObjectRegion(void * pRegion, long lSize)
{
    if (g_pMutex)
        g_pMutex->Enter();

    UINT_PTR p = (UINT_PTR)pRegion;

//    TraceTag((0, "Free region: %p - %p", p, p + _PAGESIZE_ * _PAGES_PER_COMMITMENT));
    for (long i = 0; i < lSize; i += 1 << REGION_GRANULARITY_BITS)
    {
        UINT_PTR q = p >> REGION_GRANULARITY_BITS;
        g_abRegion[q >> 3] &= ~(1 << (q & 7));
        p += 1 << REGION_GRANULARITY_BITS;
    }

    if (g_pMutex)
        g_pMutex->Leave();

#if NEVER
    TCHAR buf[40] = _T("\r\nRemoved:");
    _itot((int)pRegion, buf + 10, 16);
    OutputDebugString(buf);
#endif
}
#endif

#if DBG==1
extern ShareMutex * g_pMutexPointer;
Base *
LockingIsObject(void * p)
{
    if (g_pMutexPointer)
        g_pMutexPointer->Enter();
    ULONG_PTR ulRef;
    Base * pBase = isObject(p, &ulRef, NULL);
    if (g_pMutexPointer)
        g_pMutexPointer->Leave();
    return pBase;
}
#endif

Base *
isObject(void * p, ULONG_PTR * pulRef, TLSDATA * ptlsFrozen)
{
    Base * pBase = null;

#ifdef SPECIAL_OBJECT_ALLOCATION

    if (isObjectRegion(p))
    {
        // check for node manager first, PAGE(p) moves the pointer p to the beginning of the page
        SlotPage * pSlotPage = SlotPage::PAGE(p);
        // the first pointer is a VTABLE pointer in an object
        // small block heap and slot pages are marked in the first pointer with bit 0 set.
        if (IsCachedPointer((INT_PTR)pSlotPage) && pSlotPage->_pMark == POINTER_TO_MARK(null, void))
        {
            // lock it to prevent the page modifying this slot, no need to lock it
            // when the thread is frozen and set the page it is locking in the TLS !
            ULONG_PTR ul = pSlotPage->tryLockPage();
            bool fUnlock = (REF_LOCKED != ul);
            bool fLock = !fUnlock && !(ptlsFrozen && ptlsFrozen->_pPageLocked == pSlotPage);
            if (fLock)
            {
                ul = pSlotPage->lockPage();
                fUnlock = true;
            }
            pBase = (Base *)pSlotPage->DataFromPointer(p);
            if (pBase)
            {
                if (!*(void **)pBase)
                    pBase = null;
#if DBG == 1
                else if (*(void **)pBase == (void *)0x9d9d9d9d || *(void **)pBase == (void *)0xadadadad)
                    pBase = null;
#endif
                else
                    *pulRef = (ULONG_PTR)pBase->_refs;
            }
            if (fUnlock)
                pSlotPage->unlockPage(ul);
            return pBase;
        }
        // check small block heap
        void * pPage = (void *)((UINT_PTR)p & ~(_PAGESIZE_ - 1));
        if (IsCachedPointer((INT_PTR)pPage))
        {
            PMP_HEAP MpHeap = (PMP_HEAP)g_hMpHeap;
            __sbh_page_t * psbhpage = (__sbh_page_t *)pPage;
            Assert(ISMARKED((INT_PTR)psbhpage->p_heap));
            if (p >= &psbhpage->alloc_blocks[0])
            {
                __sbh_heap_t * pheap = MARK_TO_POINTER(psbhpage->p_heap, __sbh_heap_t);
                PMP_HEAP_ENTRY Entry = (PMP_HEAP_ENTRY)((BYTE *)pheap - (BYTE *)&((PMP_HEAP_ENTRY)0)->SBHeap);
                int Index = (int)(Entry - &MpHeap->Entry[0]);
                if (Index >= 0 && Index < (int)MpHeap->HeapCount &&
                    pheap == &MpHeap->Entry[Index].SBHeap)
                {
                    // look back for the allocated byte
                    __page_map_t * pmap =  &(psbhpage->alloc_map[0]) + ((__para_t *)p -
                                            &(psbhpage->alloc_blocks[0]));

                    // lock it to prevent the page modifying this slot, no need to lock it
                    // when the thread is frozen and set the page it is locking in the TLS !
                    ULONG_PTR ul = __sbh_try_lock_page(psbhpage);
                    bool fUnlock = (REF_LOCKED != ul);
                    bool fLock = !fUnlock && !(ptlsFrozen && ptlsFrozen->_pPageLocked == psbhpage);
                    // lock it to prevent changes in the map
                    if (fLock)
                    {
                        ul = __sbh_lock_page(psbhpage);
                        fUnlock = true;
                    }

                    while (*pmap == 0 && pmap > &psbhpage->alloc_map[0])
                        pmap--;
                    if (*pmap)
                    {
                        void * q =  &(psbhpage->alloc_blocks[pmap - &(psbhpage->alloc_map[0])]); 
                        pBase = (Base *)PointerRequestFromActual(q);
                        if ((BYTE *)pBase < (BYTE *)q + *pmap * _PARASIZE)
                        {
                            if (!*(void **)pBase)
                                pBase = null;
    #if DBG == 1
                            else if (*(void **)pBase == (void *)0x9d9d9d9d || *(void **)pBase == (void *)0xadadadad)
                                pBase = null;
    #endif
                            else
                                *pulRef = (ULONG_PTR)pBase->_refs;
                        }
                        else
                            pBase = null;
                    }
                    if (fUnlock)
                        __sbh_unlock_page(psbhpage, ul);
                }
            }
        }
    }
    else
    {
        if (IsCachedPointer((INT_PTR)p) && !ISMARKED(p))
        {
            // check the VTABLE
            if (*(int *)p != null)
            {
                TraceTag((tagRefCount, "[%d] %p empty object found on stack", GetTlsData()->_dwTID, p));
                // this means this object is already added it's pointer
                // to the pointer cache but it is not fully initialized yet
                // so cannot be on the zero count list...
                pBase = ((Object *)p)->getBase();
                *pulRef = (ULONG_PTR)pBase->_refs;
            }
        }
    }

#else // SPECIAL_OBJECT_ALLOCATION

    if (IsCachedPointer((INT_PTR)p))
    {
        // check the VTABLE
        if (*(int *)p != null)
        {
            pBase = ((Object *)p)->getBase();
            *pulRef = (ULONG_PTR)pBase->_refs;
        }
#if DBG == 1
        else
        {
            TraceTag((tagRefCount, "[%d] %p empty object found on stack", GetTlsData()->_dwTID, p));
            // this means this object is already added it's pointer
            // to the pointer cache but it is not fully initialized yet
            // so cannot be on the zero count list...
        }
#endif
    }
#endif // SPECIAL_OBJECT_ALLOCATION

    return pBase;
}

