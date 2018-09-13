/**
 * pointer.cxx - pointer cache functions and data
 *
 * Copyright (c) 1997, Microsoft Corporation. All rights reserved.
 *
 * A pointer is divided into three parts and these used as
 * indexes into lookup table. The lowest level is a bitmap 
 * where 1 bit is allocated for every 32 bit.
 *
 */

#include "core.hxx"
#pragma hdrstop
#ifndef MSXMLPRFCOUNTERS_H
#include "core/prfdata/msxmlprfcounters.h"  // PrfHeapAlloc, PrfHeapFree
#endif

DeclareTag(tagPointerCache, "Memory", "Pointer cache");
DeclareTag(tagPointerLeaks, "Memory", "Trace pointer cache leaks");
extern DLLEXPORT TAG tagRefCount;

// number of bits per level

// BUGBUG WIN64 - This needs to be looked at
#ifdef _WIN64
#define LEVEL1BITS    15
#define LEVEL2BITS    11
#define LEVEL3BITS    13
#define WORDLEVELBITS    23
#else // !_WIN64
#ifdef UNIX
#define LEVEL1BITS    11
#else // !UNIX
#define LEVEL1BITS    10
#endif
#define LEVEL2BITS    7    
#define WORDLEVELBITS    12
#endif // _WIN64

#ifndef _WIN64
#define I1SHIFT(X)  ((X) >> (LEVEL2BITS + WORDLEVELBITS))
#define I2SHIFT(X)  (((X) >> WORDLEVELBITS) & MASKBITS(LEVEL2BITS))
#define WORDLEVELSHIFT(X)  (((X) >> WORDLEVELBITS) & MASKBITS(LEVEL2BITS))
#define WORDLEVELPARENT  level2->_level[i2]
#define WORDLEVELPARENT2(X)  (level2->_level[(X)])
#define FINALBITS  LEVEL2BITS
#else
#define I1SHIFT(X)  ((X) >> (LEVEL2BITS + LEVEL3BITS + WORDLEVELBITS))
#define I2SHIFT(X)  (((X) >> (LEVEL3BITS + WORDLEVELBITS)) & MASKBITS(LEVEL2BITS))
#define I3SHIFT(X)  (((X) >> WORDLEVELBITS) & MASKBITS(LEVEL3BITS))
#define WORDLEVELSHIFT(X)  (((X) >> WORDLEVELBITS) & MASKBITS(LEVEL3BITS))
#define WORDLEVELPARENT  level3->_level[i3]
#define WORDLEVELPARENT2(X)  (level3->_level[(X)])
#define FINALBITS  LEVEL3BITS
#endif


// number if bits mapped to 1 bit in lowest level

#define WORDBITS 5

// pointer granularity (at least 32 bit aligned)

#define GRANULARITY 2

struct WORDLEVEL
{
    ULONG   _ulUsed;
    unsigned int _level[((1 << WORDLEVELBITS) >> WORDBITS)];

    void * __cdecl operator new(size_t cb)
        { return PrfHeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, cb); }

    void __cdecl operator delete(void *pv)
        { PrfHeapFree(g_hProcessHeap, 0, pv); }
};

#ifdef _WIN64
// This level is only used for Win64
struct LEVEL3
{
    ULONG   _ulUsed;
    WORDLEVEL * _level[(1 <<LEVEL3BITS)];

    void * __cdecl operator new(size_t cb)
        { return PrfHeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, cb); }

    void __cdecl operator delete(void *pv)
        { PrfHeapFree(g_hProcessHeap, 0, pv); }
};
#endif // _WIN64


struct LEVEL2
{
    ULONG   _ulUsed;
#ifndef _WIN64
	WORDLEVEL * _level[(1 <<LEVEL2BITS)];
#else
	LEVEL3 * _level[(1 <<LEVEL2BITS)];
#endif // _WIN64
    void * __cdecl operator new(size_t cb)
        { return PrfHeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, cb); }

    void __cdecl operator delete(void *pv)
        { PrfHeapFree(g_hProcessHeap, 0, pv); }
};


// level 1 is static

LEVEL2 * level1[1 << LEVEL1BITS];

// useful macros

#define MASKBITS(B) (unsigned int)((int)(1 << (B)) - 1)

#if DBG == 1
static LONG added = 0;
static LONG removed = 0;
#endif

extern ShareMutex * g_pMutexPointer;
extern ShareMutex * g_pMutexFullGC;

#define POINTER_CACHE_BRK
#ifdef POINTER_CACHE_BRK
#if DBG == 1
static TLSDATA * g_ptlsdatapointer;
#else
static bool g_fPointerCacheMod = false;
#endif
#   if DBG == 1
#   define POINTER_CACHE_ENTER \
    { \
        TLSDATA * ptlsdata = g_ptlsdatapointer; \
        if (ptlsdata) \
            DebugBreak(); \
        g_ptlsdatapointer = GetTlsData(); \
    } 
#   define POINTER_CACHE_LEAVE \
    { \
        g_ptlsdatapointer = null; \
    }
#   else
#   define POINTER_CACHE_ENTER \
    { \
        if (g_fPointerCacheMod==true) \
            DebugBreak(); \
        g_fPointerCacheMod = true; \
    } 
#   define POINTER_CACHE_LEAVE \
    { \
        g_fPointerCacheMod = false; \
    }
#   endif
#else
#define POINTER_CACHE_ENTER 
#define POINTER_CACHE_LEAVE 
#endif

// NOTE:
//	It _is_safe_ to call IsCachedPointer and AddPointerToCache at the same time, 
//	since AddPointerToCache only adds info, and does not take away anything (free memory)
// !!	It is _NOT_safe_ to call RemovePointerFromCache and IsCachedPointer at the same time
//	since Remove might remove the cache-level-array which IsCachedPointer is using 
//	(causing IsCachedPointer to reference free-d memory and crash)

void AddPointerToCache(void * p)
{
    TraceTag((tagPointerCache, "add pointer %p", p));

#if DBG == 1
    InterlockedIncrement(&added);
#endif

    if (g_pMutexPointer)
        g_pMutexPointer->ClaimExclusiveLock();

    Assert((BOOL)((UINT_PTR)p & MASKBITS(GRANULARITY)) == 0);

    UINT_PTR u = ((UINT_PTR)p >> GRANULARITY);
    UINT_PTR i1 = I1SHIFT(u);
    UINT_PTR i2 = I2SHIFT(u);
#ifdef _WIN64
    UINT_PTR i3 = I3SHIFT(u);
#endif // _WIN64
    Assert(i1 < (1 << LEVEL1BITS));

    LEVEL2 * level2 = level1[i1];
    WORDLEVEL * wordlevel;
#ifdef _WIN64
    LEVEL3 * level3;
#endif // _WIN64

    if (!level2)
    {
        level2 = level1[i1] = new LEVEL2; // (unsigned int **)MemAllocClear((1 <<LEVEL2BITS) * sizeof(unsigned int *));
        if (!level2)
            goto OutOfMemory;
        TraceTag((tagPointerCache, "allocated level2 %x size %d", level2, (1 <<LEVEL2BITS) * sizeof(unsigned int *)));
    }

#ifdef _WIN64
    level3 = level2->_level[i2];
    if (!level3)
    {
        level3 = level2->_level[i2] = new LEVEL3; // (unsigned int **)MemAllocClear((1 <<LEVEL2BITS) * sizeof(unsigned int *));
        if (!level3)
            goto OutOfMemory;
        TraceTag((tagPointerCache, "allocated level3 %x size %d", level3, (1 <<LEVEL3BITS) * sizeof(unsigned int *)));
        level2->_ulUsed++;  // WIN64 REVIEW - Do we need to do this?? I need to look at delete code below
    }
#endif

	wordlevel = WORDLEVELPARENT;
    if (!wordlevel)
    {
        wordlevel = WORDLEVELPARENT = new WORDLEVEL; // (unsigned int *)MemAllocClear(((1 << WORDLEVELBITS) >> WORDBITS) * sizeof(unsigned int));
        if (!wordlevel)
            goto OutOfMemory;
#ifdef _WIN64
        level3->_ulUsed++;
#else
        level2->_ulUsed++;
#endif // _WIN64
        TraceTag((tagPointerCache, "allocated wordlevel %x size %d", wordlevel, ((1 << WORDLEVELBITS) >> WORDBITS) * sizeof(unsigned int)));
    }
    {
        UINT_PTR iwl = u & MASKBITS(WORDLEVELBITS);
        Assert((wordlevel->_level[iwl >> WORDBITS] & 1 << (iwl & MASKBITS(WORDBITS))) == 0);
        wordlevel->_level[iwl >> WORDBITS] |= 1 << (iwl & MASKBITS(WORDBITS));
        wordlevel->_ulUsed++;
    }

    if (g_pMutexPointer)
        g_pMutexPointer->ReleaseExclusiveLock();

    return;

OutOfMemory:
    Exception::throwEOutOfMemory();
}

void SafeRemovePointerFromCache(void * p)
{
#if 0
    // shouldn't try to enter if we are already inside a GC 
    bool fLock = (g_pMutexFullGC != null) && (GetTlsData() != Base::getTlsGC());
    if (fLock)
    {
        g_pMutexFullGC->ClaimShareLock();
//        TraceTag((0, "[%x] Claimed share lock in FreeRentalList", GetCurrentThreadId()));
    }
#endif
    RemovePointerFromCache(p);
#if 0
    if (fLock)
    {
        g_pMutexFullGC->ReleaseShareLock();
//        TraceTag((0, "[%x] Released share lock in FreeRentalList", GetCurrentThreadId()));
    }
#endif
}

void RemovePointerFromCache(void * p)
{
    TraceTag((tagPointerCache, "remove pointer %p", p));

#if DBG == 1
    InterlockedIncrement(&removed);
#endif

    Assert(((UINT_PTR)p & MASKBITS(GRANULARITY)) == 0);

	UINT_PTR u = ((UINT_PTR)p >> GRANULARITY);
    UINT_PTR i1 = I1SHIFT(u);
    Assert(i1 < (1 << LEVEL1BITS));

    {
        if (g_pMutexPointer)
            g_pMutexPointer->ClaimExclusiveLock();
        POINTER_CACHE_ENTER;

        LEVEL2 * level2 = level1[i1];
	    UINT_PTR i2 = I2SHIFT(u);
#ifdef _WIN64
        LEVEL3 * level3 = level2->_level[i2];
		UINT_PTR i3 = I3SHIFT(u);
#endif // _WIN64
		WORDLEVEL * wordlevel = WORDLEVELPARENT;
        UINT_PTR iwl = u & MASKBITS(WORDLEVELBITS);
        Assert((wordlevel->_level[iwl >> WORDBITS] & (1 << (iwl & MASKBITS(WORDBITS)))) != 0);
        wordlevel->_level[iwl >> WORDBITS] &= ~(1 << (iwl & MASKBITS(WORDBITS)));
        if (--wordlevel->_ulUsed == 0)
        {
            delete wordlevel;
            WORDLEVELPARENT = 0;
#ifdef _WIN64
            if (--level3->_ulUsed == 0)
            {
                delete level3;
                level2->_level[i2] = 0;
				if (--level2->_ulUsed == 0)
				{
					delete level2;
					level1[i1] = 0;
				}
			}
#else
            if (--level2->_ulUsed == 0)
            {
                delete level2;
                level1[i1] = 0;
            }
#endif
        }

        POINTER_CACHE_LEAVE;
        if (g_pMutexPointer)
            g_pMutexPointer->ReleaseExclusiveLock();
    }
}

bool IsCachedPointer(INT_PTR pp)
{
#ifndef UNIX
    INT_PTR p = pp;
#else
    UINT_PTR p = (UINT_PTR)pp;
#endif
    if (p>0 && !(p & MASKBITS(GRANULARITY)))
    {
        POINTER_CACHE_ENTER;
        p >>= GRANULARITY;
        Assert(I1SHIFT(p) < (1 << LEVEL1BITS));
        LEVEL2 * level2 = level1[I1SHIFT(p)];
        if (level2)
        {
#ifdef _WIN64
	        LEVEL3 * level3 = level2->_level[I2SHIFT(p)];
			if (level3)
#endif
			{
				WORDLEVEL * wordlevel = WORDLEVELPARENT2(WORDLEVELSHIFT(p));
				if (wordlevel)
				{
					ULONG_PTR iwl = p & MASKBITS(WORDLEVELBITS);
                    if(wordlevel->_level[iwl >> WORDBITS]     & (1 << (iwl & MASKBITS(WORDBITS))))
					{
						TraceTag((tagPointerCache, "recognized pointer %p", p << GRANULARITY));
						POINTER_CACHE_LEAVE;
						return true;
					}
				} // (wordlevel)
			} // if (level3)
		} //if (level2)
    }
    POINTER_CACHE_LEAVE;
    return false;
}

#if DBG == 1
extern LONG g_cbObjects;
extern BOOL g_fMemoryLeak;
extern TAG  tagLeaks;
#endif

void ClearPointerCache()
{
#if DBG == 1
    if (g_cbObjects != 0)
    {
        g_fMemoryLeak = TRUE;
    }
    BOOL fTraceLeaks = (added > removed);
    ULONG ulLeaks = 0;
#endif
    TraceTag((tagLeaks, "objects left after pointer cache %d\n", g_cbObjects));
    TraceTag((tagLeaks, "   # of objects: %d\n", g_cbObjects));
    TraceTag((tagLeaks, "   added: %d removed %d", added, removed));
    Assert(g_cbObjects == 0 && added == removed);
#if DBG == 1
    if (added != removed)
    {
        g_fMemoryLeak = TRUE;
    }
#endif

/*
#ifndef _WIN64
#define I1SHIFT(X)  ((X) >> (LEVEL2BITS + WORDLEVELBITS))
#define I2SHIFT(X)  (((X) >> WORDLEVELBITS) & MASKBITS(LEVEL2BITS))
#define WORDLEVELSHIFT(X)  (((X) >> WORDLEVELBITS) & MASKBITS(LEVEL2BITS))
#define WORDLEVELPARENT  level2->_level[i2]
#define WORDLEVELPARENT2(X)  (level2->_level[(X)])
#else
#define I1SHIFT(X)  ((X) >> (LEVEL2BITS + LEVEL3BITS + WORDLEVELBITS))
#define I2SHIFT(X)  (((X) >> (LEVEL3BITS + WORDLEVELBITS)) & MASKBITS(LEVEL2BITS))
#define I3SHIFT(X)  (((X) >> WORDLEVELBITS) & MASKBITS(LEVEL3BITS))
#define WORDLEVELSHIFT(X)  (((X) >> WORDLEVELBITS) & MASKBITS(LEVEL3BITS))
#define WORDLEVELPARENT  level3->_level[i3]
#define WORDLEVELPARENT2(X)  (level3->_level[(X)])
#endif
*/

    LEVEL2 ** ppl2 = level1;
    for (unsigned int i2 = 1 <<LEVEL1BITS; i2--; ppl2++)
    {
        LEVEL2 * pl2 = *ppl2;
        if (pl2)
        {

#ifdef _WIN64
			LEVEL3 ** ppl3 = pl2->_level;
			for (unsigned int i3 = 1 <<LEVEL2BITS; i3--; ppl3++)
            {
				LEVEL3 * pl3 = *ppl3;
				if (pl3)
				{
				WORDLEVEL ** ppwl = pl3->_level;
#else
				WORDLEVEL ** ppwl = pl2->_level;
#endif // _WIN64
				for (unsigned int j = 1 << FINALBITS; j--; ppwl++)
				{
					WORDLEVEL * pwl = *ppwl;
					if (pwl)
					{
	#if DBG == 1
						if (fTraceLeaks)
						{
							unsigned int k, b;
							unsigned int *pWord = pwl->_level;
							for (k=0; k < (1<<WORDLEVELBITS)>>WORDBITS; ++k, ++pWord)
							{
								if (*pWord)
								{
									for (b=0; b < (1<<WORDBITS); ++b)
									{
										if (*pWord & (1<<b))
										{
// BUGBUG - NEED TO FIX THIS FOR WIN64
#ifndef _WIN64
											Base *p = (Base *)(
	(((((ppl2-level1)<<LEVEL2BITS) | (ppwl-pl2->_level))
											<< WORDLEVELBITS) | ((k<<WORDBITS) | b))
			<< GRANULARITY  );
	#if NEVER
	// this would be nice, but we can't do this because we
	// already freed VMM allocated pages and thus Nodes and 
	// Slot pages which were leaked, are no longer available!
											if (ISMARKED(*(INT_PTR *)p))
												TraceTag((tagLeaks,
													"%ld leaked page %p",
													++ulLeaks, p
													));
											else
												TraceTag((tagLeaks,
													"%ld leaked pointer 0x%p  refs %ld",
													++ulLeaks, p, p->_refs >> 2
													));
	#else
											TraceTag((tagLeaks,
												"%ld leaked object/page 0x%x",
												++ulLeaks, p
												));
	#endif
#endif // _WIN64
										}
									}
								}
							} // for (k=0; k < (1<<WORDLEVELBITS)>>WORDBITS; ++k, ++pWord)
						}
	#endif
						TraceTag((tagPointerCache, "free wordlevel 0x%x", pwl));
						delete pwl;
					} // if (pwl)
				} // for (unsigned int j = 1 << LEVEL2BITS; j--; ppwl++)
#ifdef _WIN64
				TraceTag((tagPointerCache, "free level3 0x%x", pl3));
		        delete pl3;
				} // if (pl3)
			}
#endif // _WIN64
			TraceTag((tagPointerCache, "free level2 0x%x", pl2));
            delete pl2;
        } // if (pl2)
    }
}
