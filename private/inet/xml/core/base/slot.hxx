/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CORE_BASE_SLOT
#define _CORE_BASE_SLOT

#ifndef _CORE_BASE_VMM
#include "core/base/vmm.hxx"
#endif

#define SLOT_LOCK

/*****************************************************
 *************** Slot Allocator *********************/

class Slot;
class SlotAllocator;

struct SlotPage
{
    friend class SlotAllocator;
    friend Base * isObject(void * p, ULONG_PTR * pulRef, TLSDATA * ptlsFrozen);

public:
    SlotPage( VMBlock *, SlotAllocator *, long cFreeSlots);
    ~SlotPage();

    void *operator new( size_t stAllocateBlock, void * pv) { return pv; }
    void operator delete( void *) {}

public:
    // try and allocate a slot within this page
    Slot * Alloc( SlotAllocator *);

    static inline SlotPage * PAGE(void * _p) { return (SlotPage *)((VMManager::SIZET)_p & VMManager::s_uPageMask); }

    SlotAllocator * getSlotManager() { return _pRoot; }

    void * DataFromPointer(void *p);

#ifdef SPECIAL_OBJECT_ALLOCATION
    void * _pMark;
#endif
    ULONG_PTR _lPageLock;

    // use cast for now until we change the spinlock API to use ULONG_PTR
    inline ULONG_PTR tryLockPage()
    {
        return INTERLOCKEDEXCHANGE_PTR(&_lPageLock, REF_LOCKED);
    }

    inline ULONG_PTR lockPage()
    {
        return (ULONG_PTR)::SpinLock(&_lPageLock);
    }

    inline void unlockPage(ULONG_PTR ul)
    {
        ::SpinUnlock((ULONG_PTR *)&_lPageLock, ul);
    }

private:
    SlotAllocator * _pRoot;  // used to figure out slot size etc...

    SlotPage * _pPrevPage, * _pNextPage;

    long _cFreeSlots;	// count of used slots
    long _nNextFree;     // index to the next (sequential) free slot
    Slot * _pFreeSlots;      // next free slot on the freelist in block

    VMBlock * _pBlock;      // VM block header addr

#if DBG==1
    SlotPage() { Assert( 0); }
    SlotPage( const SlotPage &) { Assert( 0); }
    void operator =( const SlotPage &) { Assert( 0); }
#endif
};

DEFINE_CLASS(VMManager);
DEFINE_CLASS(SlotAllocator);

class SlotAllocator : public SimpleIUnknown
{
    friend struct SlotPage;
public:
    typedef unsigned long	SLOT_SIZE;

public:
    public: ULONG STDMETHODCALLTYPE Release();
    public: void weakAddRef() { _allRefs++; }
    public: void weakRelease();

    /**
     * void * Alloc() - Allocate a new Slot
     */
    void * Alloc();

    /**
     * void * Alloc( void *) - Allocate a new Slot, from the given Block,
     *                          creating a new Block if necessary. The new
     *                          block will be passed back via the param
     */
    void * AllocFast();

    static void Free( void *);
    void DelayedFree( Slot *);

public:
    SLOT_SIZE GetSlotSize() const { return cbSlotSize; }

public:
    SlotAllocator(VMManager *, long _cbSlotSize, long cbHeaderSize = sizeof(SlotPage));

    static void classExit();

    void Lock();
    void Unlock();

protected:
    ~SlotAllocator();

protected:
    void * AllocSlotInPage( SlotPage * _pPage);

    // grab a new SlotPage from the VM
    SlotPage * NewPage();

    // free a SlotPage and return it to the VM
    void FreePage( SlotPage * _pPage);

    // used to make sure that only one thread modifies the page list at a time
    ShareMutex _lock;

public:
    SlotPage * pPages;

    // page from which last Alloc pulled slot
    SlotPage * pLastAlloc;

    // page from which last AllocFast pulled a slot
    SlotPage * pLastFastAlloc;

#if DBG == 1
    void CheckHeap();
    bool validate(void * p);
#endif
    ULONG getPages() { return _ulPages; }

protected:
    unsigned long cbSlotSize;
    long cSlotsPerPage;
    unsigned long cbHeader;
    long _allRefs;

    // the Virtual Memory Manager responsible for providing this allocator 
    //  with blocks of memory
    RVMManager _pVMM;
    Slot * _pDelayedFreeSlots;  // free slot list added to free list later

protected:
    ULONG _ulPages;

#if DBG == 1
    ULONG ulAllocs;
    ULONG ulFrees;
    ULONG ulDelayedFrees;

    SlotAllocator * pNextAllocator;
    SlotAllocator * pPrevAllocator;
    static SlotAllocator * s_pAllocators;
#endif
};

#endif // _CORE_BASE_SLOT