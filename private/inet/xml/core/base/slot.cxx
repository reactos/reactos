/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _CORE_BASE_VMM
#include "core/base/vmm.hxx"
#endif

#ifndef _CORE_BASE_SLOT
#include "slot.hxx"
#endif

#if DBG==1
void * g_pUnkTrace;     // see _unknown.hxx
#endif

#undef SLOT_LOCK

// this is to help indicate which increments would need an InterlockedIncrement
// if we were to remove the critical section....
#if 1
 #define _Increment( x) (++x)
 #define _Decrement( x) (--x)
#else
 #define _Increment( x) InterlockedIncrement(&(x))
 #define _Decrement( x) InterlockedDecrement(&(x))
#endif

DeclareTag(tagSlotAllocator, "SlotAllocator", "Slot memory manager");
DeclareTag(tagHeapCheck, "SlotHeapCheck", "Slot memory manager");

/********* class SlotPage **********************************************/ 

SlotPage::SlotPage( VMBlock * pBlock, SlotAllocator * pRoot, long cFreeSlots)
{
    // this should stay here to always turn on debug out
    EnableTag( tagSlotAllocator, TRUE);

    _pPrevPage = null;
    _pNextPage = null;
    _cFreeSlots = cFreeSlots;
    _nNextFree = pRoot->cbHeader;
    _pFreeSlots = null;
    _pBlock = pBlock;
#ifdef SPECIAL_OBJECT_ALLOCATION
    _pMark = POINTER_TO_MARK(null, void);
#endif
    _pRoot = pRoot;
}

SlotPage::~SlotPage()
{
}

/********* class Slot *********************************************************/

class Slot
{
public:
#if DBG == 1
    ULONG _ulCount;
    // used to force 8 byte alignment for RENTAL
    DWORD _dwFill;
#endif
    void ** _pMark;
    Slot * _pNextSlot;
};

inline void * DATAFROMSLOT(Slot * p) 
{ 
#if DBG == 1
    if (!p)
        return null;
#endif
    return &p->_pMark;
}

inline Slot * SLOTFROMDATA(void * p) 
{ 
    return (Slot *)((INT_PTR)p - (INT_PTR)&((Slot *)0)->_pMark); 
}

/********* class SlotAllocator *****************************************/ 

#if DBG ==1
SlotAllocator * SlotAllocator::s_pAllocators = null;
extern CSMutex * g_pMutex;
#endif

SlotAllocator::SlotAllocator( VMManager * pVMManager, long _cbSlotSize, long cbHeaderSize)
{
#ifdef SLOT_LOCK
    _lock.ClaimExclusiveLock();
#endif
    // slot size must be at least large enough 
    cbSlotSize = (_cbSlotSize < sizeof(Slot)) ? (long)sizeof(Slot) : _cbSlotSize;
#if DBG == 1
    cbSlotSize += (INT_PTR)&((Slot *)0)->_pMark;
#endif
    cbHeader = (cbSlotSize * ((cbHeaderSize + cbSlotSize-1) / cbSlotSize));
    cSlotsPerPage = (_VMM_PAGESIZE*1024 - cbHeader - (cbSlotSize-1)) / cbSlotSize;
    Assert( cSlotsPerPage > 1);
    pPages = pLastAlloc = pLastFastAlloc = null;
    _pVMM = pVMManager;
    _allRefs = 1;
    _pDelayedFreeSlots = null;
    _ulPages = 0;
#if DBG == 1
    {
        MutexLock lock(g_pMutex);

        ulAllocs = ulFrees = ulDelayedFrees = 0;

        if ( s_pAllocators == null)
        {
            pNextAllocator = null;
            pPrevAllocator = null;
        }
        else 
        {
            pNextAllocator = s_pAllocators;
            s_pAllocators->pPrevAllocator = this;
            pPrevAllocator = null;
        }
        s_pAllocators = this;
    }
#endif
#ifdef SLOT_LOCK
    _lock.ReleaseExclusiveLock();
#endif
}

ULONG 
SlotAllocator::Release()
{
    // check delayed free list
    if (_pDelayedFreeSlots)
    {
        Slot * pSlot = _pDelayedFreeSlots;
        // null it out so recursive Release doesn't come in here !
        _pDelayedFreeSlots = null;
        while(pSlot)
        {
            Slot * pNextSlot = pSlot->_pNextSlot;
            DelayedFree(pSlot);
            pSlot = pNextSlot;
        }
    }
    ULONG ul = Decrement();
    if (ul == 0) 
    {
        weakRelease();
    }
    return ul;
}


void
SlotAllocator::weakRelease()
{
    if (--_allRefs == 0)
        delete this;
}


void
SlotAllocator::classExit()
{
#if DBG == 1
    if ( s_pAllocators != null)
    {
//        Assert( 0 && "SlotAllocators still alive");
        TraceTag((tagSlotAllocator, "Memory Leak !! Slots still allocated:"));

        SlotAllocator * pAlloc = s_pAllocators;
        while ( pAlloc)
        {
            SlotPage * pPage = pAlloc->pPages;
            while ( pPage)
            {
                // sweep over block
                Slot * pSlotWalk = (Slot *)((byte *)pPage + pAlloc->cbHeader);
                long cLeft = pAlloc->cSlotsPerPage;
                while ( cLeft--) 
                {
                    if (pSlotWalk->_pMark) 
                    {
                        ULONG ulWhichAlloc = pSlotWalk->_ulCount;
                        TraceTag((tagSlotAllocator, "  Unfree-d slot: %u (SlotAllocator: %X, SlotPage: %X)",
                                ulWhichAlloc, pAlloc, pPage));
                    }
                    pSlotWalk = (Slot *)(((byte *)pSlotWalk) + pAlloc->cbSlotSize);
                }
                pPage = pPage->_pNextPage;
            }
            pAlloc = pAlloc->pNextAllocator;
        }
    }
#endif
}

SlotAllocator::~SlotAllocator()
{
#ifdef SLOT_LOCK
    _lock.ClaimExclusiveLock();
#endif
    Assert(ulFrees == ulAllocs);
    Assert(ulDelayedFrees == ulAllocs);
#if DBG == 1
    CheckHeap();
#endif
    if ( pPages != null) {
        SlotPage * pPage = pPages;
        SlotPage * pNextPage = null;
        while ( pPage) {
            pNextPage = pPage->_pNextPage;
            // there should only be empty pages left....
            Assert( pPage->_cFreeSlots == cSlotsPerPage);
            FreePage( pPage);
            pPage = pNextPage;
        }
        pPages = null;
    }
    Assert( pPages == null);
#ifdef SLOT_LOCK
    _lock.ReleaseExclusiveLock();
#endif

//#ifdef SLOT_LOCK
    _lock.EmbeddedRelease();
//#endif
    _pVMM = null;

#if DBG == 1
    {
        MutexLock lock(g_pMutex);

        if ( pPrevAllocator != null)
        {
            pPrevAllocator->pNextAllocator = pNextAllocator;
        }
        if ( pNextAllocator != null)
        {
            pNextAllocator->pPrevAllocator = pPrevAllocator;
        }
        if ( s_pAllocators == this)
        {
            if ( pNextAllocator)
                s_pAllocators = pNextAllocator;
            else 
                s_pAllocators = pPrevAllocator;
        }
    }
#endif
}

#if DBG == 1
extern void SuspendAllThreads();
#endif

void * SlotAllocator::Alloc()
{
    Assert(RefCount());

    Slot * pSlot = null;
    SlotPage * pPage;

    // check delayed free list
    if (_pDelayedFreeSlots)
    {
        Slot * pDelayedSlot = (Slot *)INTERLOCKEDEXCHANGE_PTR(&_pDelayedFreeSlots, null);

		// use first one...
        pSlot = pDelayedSlot;
#if DBG == 1
        ulDelayedFrees++;
#endif
        pDelayedSlot = pDelayedSlot->_pNextSlot;
        memset( pSlot, 0, cbSlotSize);
        while(pDelayedSlot)
        {
            Slot * pNextSlot = pDelayedSlot->_pNextSlot;
            DelayedFree(pDelayedSlot);
            pDelayedSlot = pNextSlot;
        }
        goto ReturnSlot;
    }

    // make sure that the page chain doesn't change while we walk it.
#ifdef SLOT_LOCK
    _lock.ClaimExclusiveLock();
#endif
#if DBG == 1
    CheckHeap();
#endif

    pPage = pLastAlloc;
    while (pPage) {
        if (pPage->_cFreeSlots > 0) {
            pSlot = pPage->Alloc( this);
            // we may not have actually gotten a slot
            if ( pSlot) {
                goto Return;
            }
        }
        pPage = pPage->_pNextPage;
    }

    // start back at the begining of the list
    pPage = pPages;
    while (pPage && pPage != pLastAlloc) {
        if (pPage->_cFreeSlots > 0) {
            pSlot = pPage->Alloc( this);
            if ( pSlot) {
                goto Return;
            }
        }
        pPage = pPage->_pNextPage;
    }
#ifdef SLOT_LOCK
    _lock.ReleaseExclusiveLock();
#endif
    // didn't find any free space

    // need a new block !
    pPage = NewPage();
    if (!pPage)
        return null;
    pSlot = pPage->Alloc( this);
    Assert( pSlot);

    // make sure nobody else is walking page chain while we change it.
#ifdef SLOT_LOCK
    _lock.ClaimExclusiveLock();
#endif

    pPage->_pNextPage = pPages;
    if ( pPages)
        pPages->_pPrevPage = pPage;
    pPages = pPage;

Return:
    pLastAlloc = pPage;

ReturnSlot:
#if DBG == 1
    if (pLastAlloc && pPages && *(void **)pLastAlloc != *(void **)pPages)
    {
        SuspendAllThreads();
        DebugBreak();
    }
    if ( pSlot)
    {
        ulAllocs++;
        pSlot->_ulCount = ulAllocs;
    }
#endif
#ifdef SLOT_LOCK
    _lock.ReleaseExclusiveLock();
#endif
    return DATAFROMSLOT(pSlot);
}

void * SlotAllocator::AllocFast()
{
    Assert(RefCount());

    Slot * pSlot = null;

#ifdef SLOT_LOCK
    _lock.ClaimExclusiveLock();
#endif

#if DBG == 1
    CheckHeap();
#endif

    SlotPage * pPage = pLastFastAlloc;
    if (pPage && pPage->_cFreeSlots > 0) {
        pSlot = pPage->Alloc( this);
        if ( pSlot) {
            goto Return;
        }
    }
#ifdef SLOT_LOCK
    _lock.ReleaseExclusiveLock();
#endif

    // need a new block !
    pPage = NewPage();
    Assert( pPage);
    if (!pPage)
        return null;
    pSlot = pPage->Alloc(this);
    Assert( pSlot);

    // insert at head of chain
#ifdef SLOT_LOCK
    _lock.ClaimExclusiveLock();
#endif

    pPage->_pNextPage = pPages;
    if ( pPages)
        pPages->_pPrevPage = pPage;
    pPages = pPage;
    pLastFastAlloc = pPage;

Return:
#if DBG == 1
    if ( pSlot)
    {
        ulAllocs++;
        pSlot->_ulCount = ulAllocs;
    }
#endif
#ifdef SLOT_LOCK
    _lock.ReleaseExclusiveLock();
#endif
    return DATAFROMSLOT(pSlot);
}


void SlotAllocator::Free( void * pUser)
{
    Slot * pSlot = SLOTFROMDATA(pUser);
    SlotPage * pPage = SlotPage::PAGE( pSlot);

    // make this wait in case IsObject is looking at the page...
    TLSDATA * ptlsdata = GetTlsData();
    // remember locked page in tls in case we look at this page when the 
    // thread is frozen
    ptlsdata->_pPageLocked = pPage;
    ULONG_PTR ul = pPage->lockPage();
    // mark it free
    pSlot->_pMark = null;
    pPage->unlockPage(ul);
    ptlsdata->_pPageLocked = null;

    SlotAllocator * pRoot = pPage->_pRoot;
#if DBG == 1
    pSlot->_ulCount = 0;
    InterlockedIncrement((LPLONG)&pRoot->ulFrees);
    memset(pSlot + 1, 0xFD, pRoot->cbSlotSize - sizeof(Slot));
    pRoot->CheckHeap();
#endif
    // if object was already completely released nobody can
    // allocate so it is safe to free immediately
    if (!pRoot->RefCount())
    {
        pRoot->DelayedFree(pSlot);
    }
    else
    {
        Slot * pNextSlot;
        do
        {
            pNextSlot = pSlot->_pNextSlot = pRoot->_pDelayedFreeSlots;
		}
        while ((Slot*)InterlockedCompareExchange((PVOID*)&pRoot->_pDelayedFreeSlots, (PVOID)pSlot, pNextSlot) != pNextSlot);
    }
}

void SlotAllocator::DelayedFree( Slot * pSlot)
{
#if DBG == 1
    ulDelayedFrees++;
#endif
    SlotPage * pPage = SlotPage::PAGE( pSlot);

    Assert(pSlot->_pMark == null);

    // crit-sec over any modification to page chain
#ifdef SLOT_LOCK
    _lock.ClaimExclusiveLock();
#endif
#if DBG == 1
    CheckHeap();
#endif

    pSlot->_pNextSlot = pPage->_pFreeSlots;
    pPage->_pFreeSlots = pSlot;
    pPage->_cFreeSlots++;

    // if page no longer used, and it is not the first page 
    // (we only free the first page on delete)
    if ( pPage->_cFreeSlots == cSlotsPerPage)
    {
        if ( pPage->_pPrevPage) 
        {
            pPage->_pPrevPage->_pNextPage = pPage->_pNextPage;
        } 
        else 
        {
            // first page in list
            Assert( pPages == pPage);
            pPages = pPage->_pNextPage;
        }

        if ( pPage->_pNextPage)
            pPage->_pNextPage->_pPrevPage = pPage->_pPrevPage;

        if ( pLastAlloc == pPage)
            pLastAlloc = null;
        if ( pLastFastAlloc == pPage)
            pLastFastAlloc = null;

        FreePage( pPage);
    }
#ifdef SLOT_LOCK
    _lock.ReleaseExclusiveLock();
#endif
}

void SlotAllocator::FreePage( SlotPage * pPage)
{
#ifdef SPECIAL_OBJECT_ALLOCATION
    SafeRemovePointerFromCache(pPage);
#endif
    VMBlock * pBlock = pPage->_pBlock;
    Assert( pBlock);
    delete pPage;
    pBlock->_pVMM->Free( pPage, pBlock);
    _ulPages--;
    weakRelease();
}

SlotPage * SlotAllocator::NewPage()
{
    VMBlock * pBlock;

    void * pPage = (SlotPage *)_pVMM->Alloc( &pBlock);
    if ( pPage) 
    {
        SlotPage * pNewPage = new (pPage) SlotPage( pBlock, this, cSlotsPerPage);
#ifdef SPECIAL_OBJECT_ALLOCATION
        TRY
        {
            AddPointerToCache(pPage);
        }
        CATCH
        {
            delete pPage;
            pBlock->_pVMM->Free( pPage, pBlock);
            return null;
        }
        ENDTRY
#endif
        _ulPages++;
        weakAddRef();
        return pNewPage;
    }
    return null;
}

void * 
SlotPage::DataFromPointer(void *p)
{
    unsigned long cbHeader = _pRoot->cbHeader;
    INT_PTR s = (INT_PTR)p - (INT_PTR)this - cbHeader;
    if (s >= 0)
    {
        Slot * pSlot = (Slot *)((byte *)this + cbHeader + s - (s % _pRoot->cbSlotSize));
        if (pSlot->_pMark)
            return DATAFROMSLOT(pSlot);
    }
    return null;
}

/**
 * precondition: pPage->_cFreeSlots has already been decremented (will be un-dec'ed if no slot found)
 */
Slot * SlotPage::Alloc( SlotAllocator * pAlloc) 
{
    Slot * pSlot;

    Assert(_cFreeSlots);

    if ( _nNextFree + pAlloc->cbSlotSize < _VMM_PAGESIZE*1024) 
    {
        pSlot = (Slot *)((byte *)this + _nNextFree);
        _nNextFree += pAlloc->cbSlotSize;
    } 
    else 
    {
        pSlot = this->_pFreeSlots;
        Assert(pSlot);
        this->_pFreeSlots = pSlot->_pNextSlot;
        memset( pSlot, 0, pAlloc->cbSlotSize);
    }

    _Decrement( this->_cFreeSlots);
    Assert(!pSlot || (pSlot->_ulCount == 0 && pSlot->_pMark == null));
    return pSlot;
}

#if DBG == 1
bool SlotAllocator::validate(void *p)
{
    SlotPage * pPage = SlotPage::PAGE(p);

    return pPage->DataFromPointer(p) == p;
}
#endif

#if DBG == 1
void SlotAllocator::CheckHeap()
{
    if (!IsTagEnabled(tagHeapCheck))
        return;

    // you can nest CritSect enters and exits, so long as you stay in the same thread...
#ifdef SLOT_LOCK
    _lock.ClaimExclusiveLock();
#endif

    SlotPage * pPrevPage = null;
    SlotPage * pPage = pPages;
    while (pPage != null) {
        Assert( pPage->_pPrevPage == pPrevPage);
        // check SlotPage struct fields..

        // sweep over block, check for slots with 1st dword == nul (but not freed)
        Slot * pSlotWalk = (Slot *)((byte *)pPage + cbHeader);
        long cLeft = cSlotsPerPage;
        while ( cLeft--) {
            // look for any slots which have not been freed, but which have a 1st DWORD == 0
            // (which would be thought a free slot in retail!)
            if (pSlotWalk->_ulCount && !pSlotWalk->_pMark)
            {
                Assert( 0 && "1st DWORD should never be 0 !!!");
            }

            pSlotWalk = (Slot *)(((byte *)pSlotWalk) + cbSlotSize);
        }

        pPrevPage = pPage;
        pPage = pPage->_pNextPage;
    }

#ifdef SLOT_LOCK
    _lock.ReleaseExclusiveLock();
#endif
}
#endif


void SlotAllocator::Lock()
{
    _lock.ClaimExclusiveLock();
}

void SlotAllocator::Unlock()
{
    _lock.ReleaseExclusiveLock();
}
