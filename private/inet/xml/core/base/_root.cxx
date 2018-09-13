/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop 

#include <teb.hxx>

#ifdef PRFDATA
#include "core/prfdata/msxmlprfcounters.h"
#endif

#ifdef STRONGREFATOM
#undef STRONGREFATOM
#endif

#ifdef STRONGREFATOM
#undef FASTRENTALGC
#endif

extern TLSDATA * g_ptlsdata;
TLSDATA * s_ptlsdataGC = NULL; // start of tlsdata list what the GC will use

const DWORD MAX_FINALIZE_DEPTH = 512;

DeclareTag(tagRefCount, "Base", "Base reference counting");
DeclareTag(tagZeroListCount, "Base", "Base zero list counting");

#if DBG==1
Base * g_pBaseTrace = NULL;     // trace refcounts on this object
#endif

#if DBG == 1
LONG g_cbObjects;
#endif

#undef DEBUGINRETAIL

bool g_fClassInitCalled = false; // whether class init functions such as Name::classInit() have been called

#ifdef DEBUGINRETAIL
void Print(TCHAR * pch)
{
    TCHAR buf[1024];

    TCHAR * pBuf = buf;
    DWORD dw = GetTlsData()->_dwTID;
    *pBuf++ = _T('[');
    _itot(dw, pBuf, 10);
    long l = _tcslen(pBuf);
    pBuf += l;
    *pBuf++ = _T(']');
    l = _tcslen(pch);
    memcpy(pBuf, pch, l * sizeof(TCHAR));
    pBuf[l] = 0;
    OutputDebugString(buf);
}
#endif

SRClass Base::_class; 

extern ShareMutex * g_pMutexGC;
extern ShareMutex * g_pMutexFullGC;

Class * Base::getClass() const
{ 
    return _getClass(); 
} 

Class * Base::_getClass() 
{ 
    if (!_class) 
        _class = Base::newClass(_T("Base"), null, null); 
    return _class; 
}

#if DBG == 1
unsigned g_ulMarked;
unsigned g_ulCollected;
unsigned g_ulOnZeroList;
unsigned g_ulLocked;
LONG g_lZeroListCount;
unsigned g_fMonitor;
extern LONG g_lDocumentCount;
#endif

#ifdef DEBUGINRETAIL

Base * s_pBaseTemp;

#define FINALIZEDARRAY  100000

Base * s_pBaseFinalized[FINALIZEDARRAY];
void * s_pBaseFinalizedVTable[FINALIZEDARRAY];
ULONG_PTR s_lBaseFinalizedRefs[FINALIZEDARRAY];
unsigned s_uFinalized = 0;

#define DELETEDARRAY  100000

Base * s_pBaseDeleted[DELETEDARRAY];
void * s_pBaseDeletedVTable[DELETEDARRAY];
unsigned s_uDeleted = 0;

#define MARKEDARRAY 10000

Base * s_pBaseMarked[MARKEDARRAY];
void * s_pVTable[MARKEDARRAY];
ULONG_PTR s_lMarkedRefs[MARKEDARRAY];
unsigned s_uMarked = 0;

#endif DEBUGINRETAIL


// CHANGE THIS TO OUTPUT CLASS NAMES IN TRACETAGS
#define RTTINAME(o) "" // typeid(o).name()

TLSDATA * Base::s_ptlsCheckZeroList = null;
TLSDATA * Base::s_ptlsGC = null;
bool Base::s_fInFreeObjects = false;
ULONG Base::s_ulGCCycle = 0;

static unsigned MARKEDSIZE = 4096;

static Base ** _markedObjects;
static Base ** _markedCurrent;
static bool _overflowed;

static unsigned LOCKEDSIZE = 1024;

static Base ** _lockedObjects;
static Base ** _lockedCurrent = _lockedObjects;

BOOL GCInit()
{
    _markedObjects = (Base **)HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, MARKEDSIZE * sizeof(Base *));
    _lockedObjects = (Base **)HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, LOCKEDSIZE * sizeof(Base *));
    return _markedObjects != NULL && _lockedObjects != NULL;
}

void GCExit()
{
    if (_markedObjects)
        HeapFree(g_hProcessHeap, 0, _markedObjects);
    if (_lockedObjects)
        HeapFree(g_hProcessHeap, 0, _lockedObjects);
}

ULONG_PTR
Base::tryLock()
{
    ULONG_PTR * p = &_refs;
    ULONG_PTR l = INTERLOCKEDEXCHANGE_PTR(p, REF_LOCKED);
#if DBG == 1
    if (l != REF_LOCKED)
    {
        _dwTID = GetTlsData()->_dwTID;
    }
#endif
    return l;
}

extern bool g_fMultiProcessor;

#if DBG == 1
void SuspendAllThreads()
{
    TLSDATA * ptlscurrent = GetTlsData();
    for (TLSDATA * ptlsdata = g_ptlsdata; ptlsdata; ptlsdata = ptlsdata->_pNext)
    {
        if (ptlsdata != ptlscurrent)
        {
            // if this fails the thread is not alive any more
            if (::SuspendThread(ptlsdata->_hThread) == 0xFFFFFFFF)
            {
                continue;
            }
        }
    }
    DebugBreak();
}
#endif


#if DBG == 1
ULONG_PTR SpinLock(ULONG_PTR * p, Base * b)
#else
ULONG_PTR SpinLock(ULONG_PTR * p)
#endif
{
    long lSpin = g_fMultiProcessor ? 4000 : 0;
    for (;;)
    {
        if (*p != REF_LOCKED)
        {
            ULONG_PTR l = INTERLOCKEDEXCHANGE_PTR(p, REF_LOCKED);
            if (l != REF_LOCKED)
            {
#if DBG == 1
                if (b)
                {
                    Assert(0 == (l & REF_RENTAL));
                    if (b->_dwTID != 0)
                        SuspendAllThreads();
                    b->_dwTID = GetTlsData()->_dwTID;
                }
#endif
                return l;
            }
        }
        if (lSpin)
        {
            lSpin--;
            continue;
        }

#if DBG == 1
        if (b)
        {
            TraceTag((tagRefCount, "[%d] %p:%s had to sleep in spinlock", GetTlsData()->_dwTID, b, RTTINAME(*b)));
        }
#endif
        Sleep(0);
    }
}


#if DBG == 1
void SpinUnlock(ULONG_PTR *p, LONG_PTR l, Base * b)
#else
void SpinUnlock(ULONG_PTR *p, LONG_PTR l)
#endif
{
#if DBG == 1
    if (b)
    {
        ULONG_PTR ref = *p;
        if (ref != REF_LOCKED)
            SuspendAllThreads();
        DWORD dwTID = b->_dwTID;
        if (b->_dwTID == 0)
            SuspendAllThreads();
        if (dwTID != GetTlsData()->_dwTID)
            SuspendAllThreads();
        b->_dwTID = 0;
    }
#endif
    *p = l;
}


#ifdef RENTAL_MODEL
#define REF_TO_POINTER(p)   ((Base *)((ULONG_PTR)p & ~(REF_MARKED | REF_RENTAL)))
#else
#define REF_TO_POINTER(p)   ((Base *)((ULONG_PTR)p & ~(REF_MARKED)))
#endif
#define POINTER_TO_REF(p)   ((ULONG_PTR)p)
#define ZEROLISTHEAD(p)    ((Base *)&p->_baseHead)
#define ZEROLISTHEADLOCKED(p) ((Base *)&p->_baseHeadLocked)

__inline Base * lockZeroList(TLSDATA * ptlsdata)
{
    return REF_TO_POINTER(ZEROLISTHEAD(ptlsdata)->spinLock());
}

__inline void unLockZeroList(TLSDATA * ptlsdata, Base * pZeroList)
{
    ZEROLISTHEAD(ptlsdata)->unLock(POINTER_TO_REF(pZeroList));
}

Base *
Base::addToZeroList(TLSDATA * ptlsdata)
{
    s_lZeroListCount++;
    TraceTag((tagZeroListCount, "%p : ++s_lZeroListCount = %d : addToZeroList()", this, s_lZeroListCount));

    Assert(_refs == REF_LOCKED);

#if DBG == 1
    InterlockedIncrement(&g_lZeroListCount);
#endif

    Base * pHead = ZEROLISTHEAD(ptlsdata);
    Base * pNext = REF_TO_POINTER(pHead->spinLock());
    pHead->unLock(POINTER_TO_REF(this));
    return pNext;
}


#if DBG == 1
unsigned s_uMaxLength;
unsigned __int64 s_uAveLength;
unsigned s_uCount;
#endif


bool
FindOnZeroList(Base * pHead, Base * pStop, Base * pWhat, Base * pFollow)
{
    Base * pNext;
    ULONG_PTR nrefs;
    Base * pBase = pHead;
    ULONG_PTR refs = pHead->tryLock();
    if (refs == REF_LOCKED)
    {
        // ran into locked object, try it later...
        return false;
    }
#if DBG == 1
    unsigned uLength = 1;
#endif

    // try to find it in this TLS
    for ( ; ; )
    {
        pNext = REF_TO_POINTER(refs);
        if (pNext == pWhat)
        {
            pBase->unLock(POINTER_TO_REF(pFollow) | (refs & REF_MARKED));
#if DBG == 1
            if (s_uMaxLength < uLength)
                s_uMaxLength = uLength;
            s_uAveLength += (unsigned __int64)uLength;
            s_uCount++;
#endif
            return true;
        }
        else if (pNext == pStop)
        {
            pBase->unLock(refs);
            break;
        }
        else
        {
            nrefs = pNext->tryLock();
            pBase->unLock(refs);
            if (nrefs == REF_LOCKED)
            {
                // ran into locked object, try it later...
                break;
            }
            refs = nrefs;
            pBase = pNext;
        }
#if DBG == 1
        uLength++;
#endif
    }
#if DBG == 1
    if (s_uMaxLength < uLength)
        s_uMaxLength = uLength;
    s_uAveLength += (unsigned __int64)uLength;
    s_uCount++;
#endif
    return false;
}

ULONG_PTR
Base::removeFromZeroList(ULONG_PTR lBaseRefs, TLSDATA * ptlsdata)
{
    Assert(_refs == REF_LOCKED);
    bool fEntered = false;

    Base * pFollow = REF_TO_POINTER(lBaseRefs);
    Base * pBase;
    ULONG_PTR refs;

    bool fFound = FindOnZeroList(ZEROLISTHEAD(ptlsdata), ZEROLISTHEAD(ptlsdata), this, pFollow);

    if (!fFound)
    {
        if (!g_pMutexGC->TryEnter())
            goto Exit;

        fEntered = true;

        fFound = FindOnZeroList(ZEROLISTHEADLOCKED(ptlsdata), ZEROLISTHEAD(ptlsdata), this, pFollow);

        if (!fFound)
        {
            // the object is on an other thread's zero count list
            // have to go to the end of the list to find the head...
            for (pBase = pFollow; ((TLSDATA::BASE *)pBase)->_pfnvtable != null; )
            {
                refs = pBase->tryLock();
#if DBG == 1 || defined(DEBUGRENTAL)
                if (!REF_TO_POINTER(refs))
                {
                    DebugBreak();
                }
#endif
                if (refs == REF_LOCKED)
                {
                    // ran into locked object, try it later...
                    goto Exit;
                }
                pBase->unLock(refs);
                pBase = REF_TO_POINTER(refs);
            }

            // now p should point to _baseHead in a TLSDATA structure...
            ptlsdata = (TLSDATA *)((BYTE *)pBase - (ULONG_PTR)ZEROLISTHEAD(((TLSDATA *)0)));
            Assert(ZEROLISTHEAD(ptlsdata) == pBase);

            fFound = FindOnZeroList(ZEROLISTHEAD(ptlsdata), ZEROLISTHEAD(ptlsdata), this, pFollow);

            if (!fFound)
            {
                fFound = FindOnZeroList(ZEROLISTHEADLOCKED(ptlsdata), ZEROLISTHEAD(ptlsdata), this, pFollow);

                if (!fFound)
                    goto Exit;
            }
        }
    }

    lBaseRefs = REF_OFFSET | REF_REFERENCE | (lBaseRefs & REF_MARKED);

    s_lZeroListCount--;
    TraceTag((tagZeroListCount, "%p : --s_lZeroListCount = %d : removeFromZeroList(ULONG_PTR lBaseRefs)", this, s_lZeroListCount));

Exit:
    if (fEntered)
        g_pMutexGC->Leave();

    return lBaseRefs;
}

#ifdef FAST_OBJECT_LIST
void 
Base::flushToZeroList(TLSDATA * ptlsdata)
{
    // quick check to see if we the list is empty
    if (ptlsdata->_uNextObject)
    {
        Base * pHead = ZEROLISTHEAD(ptlsdata);
        Base * pNext = REF_TO_POINTER(pHead->spinLock());
        Base ** ppStart = ptlsdata->_ppObjects;
        Base ** ppEnd = ppStart + SIZEOFOBJECTLIST;
        while (ppStart < ppEnd)
        {
            Base * pBase = *ppStart;
            if (pBase)
            {
                ULONG_PTR refs = pBase->spinLock();
                Assert((INT_PTR)ptlsdata->_ppObjects[refs >> REF_SHIFT] == (INT_PTR)*ppStart);
                pBase->unLock(POINTER_TO_REF(pNext) | (refs & REF_MARKED));
                s_lZeroListCount++;
                TraceTag((tagZeroListCount, "%p : ++s_lZeroListCount = %d : flushToZeroList(TLSDATA * ptlsdata)", pBase, s_lZeroListCount));
                pNext = pBase;
                *ppStart = null;
            }
            ppStart++;
        }
        pHead->unLock(POINTER_TO_REF(pNext));
        ptlsdata->_uNextObject = 0;
    }
}

void
Base::addToObjectList(TLSDATA * ptlsdata)
{
    unLock(ptlsdata->_uNextObject << REF_SHIFT);
    ptlsdata->_ppObjects[ptlsdata->_uNextObject++] = this;
    if (ptlsdata->_uNextObject == SIZEOFOBJECTLIST)
    {
        flushToZeroList(ptlsdata);
        // do the test only if we are not currently in the
        // GC on this thread !
        if (ptlsdata != s_ptlsGC)
            testForGC(0);
    }
}
#endif // FAST_OBJECT_LIST

#ifdef RENTAL_MODEL

BOOL
Base::isRental()
{
    ULONG_PTR refs = _refs;
    if (refs == REF_LOCKED)
    {
        unLock(refs = spinLock());
        Assert(0 == (refs & REF_RENTAL));
    }
    return (BOOL)(refs & REF_RENTAL);
}

RentalEnum
Base::model()
{
    return isRental() ? Rental : MultiThread;
}

void
Base::addToRentalList(ULONG_PTR lBaseRefs, TLSDATA * ptlsdata)
{
#if DBG == 1
    if (_refs != REF_RENTAL)
        SuspendAllThreads();
    DWORD dwTID = _dwTID;
    if (_dwTID == 0)
        SuspendAllThreads();
    if (dwTID != GetTlsData()->_dwTID)
        SuspendAllThreads();
    _dwTID = 0;
#endif
    _refs = POINTER_TO_REF(ptlsdata->_pRentalList) | REF_RENTAL | ((ULONG_PTR)lBaseRefs & REF_MARKED);
    Assert(!(lBaseRefs & REF_MARKED));
    ptlsdata->_pRentalList = this;

#if FASTRENTALGC
    if (ptlsdata->_uRentals++ > PerThreadGCFrequency && 
        ptlsdata != s_ptlsGC &&
        !ptlsdata->_fPartialRental)
    {
        partialFreeRentalObjects(GetTlsData());
    }
#else
    if (ptlsdata->_uRentals++ > PerThreadGCFrequency && ptlsdata != s_ptlsGC)
    {
        testForGC(GC_FORCE | GC_FULL);
    }
#endif
}

void 
Base::removeFromRentalList(ULONG_PTR lBaseRefs, TLSDATA * ptlsdata)
{
    Base * pBase = ptlsdata->_pRentalList;

    Assert(lBaseRefs & REF_RENTAL);
#ifdef DEBUGRENTAL
    if (!(lBaseRefs & REF_RENTAL))
        DebugBreak();
#endif
    // quick check to check if the first matches
    if (pBase == this)
    {
        ptlsdata->_pRentalList = REF_TO_POINTER(lBaseRefs);
    }
    else
    {
        // BUGBUG have to make sure that parser thread enters a mutex
        // protecting the rental list !
        for (;;)
        {
            ULONG_PTR refs = pBase->_refs;
#ifdef DEBUGRENTAL
            if (!(refs & REF_RENTAL))
                DebugBreak();
#endif
            Assert(refs & REF_RENTAL);
            if (REF_TO_POINTER(refs) == this)
                break;
            pBase = REF_TO_POINTER(refs);
            Assert(pBase);
        }
        pBase->_refs = lBaseRefs;
        Assert(!(pBase->_refs & REF_MARKED));
    }
    _refs = REF_OFFSET | REF_REFERENCE | REF_RENTAL | (lBaseRefs & REF_MARKED);
    Assert(!(_refs & REF_MARKED));
    ptlsdata->_uRentals--;
}

#if FASTRENTALGC
#define RENTALPTRHASHSHIFT 6
#define RENTALPTRHASH(p) ((((unsigned)p) >> RENTALPTRHASHSHIFT) % SIZEOFRENTALGCLIST)

void
Base::partialFreeRentalObjects(TLSDATA * ptlsdata)
{
    // return if we are already in freeRentalObjects
    if (ptlsdata->_fPartialRental)
        return;

    Model model(ptlsdata, Rental);
    unsigned uRentalsInit;
    unsigned uRentalsFinalized;
    Base ** apObjects;
    Base * pBase;
    Base * pFirstHash = ptlsdata->_pRentalList;
    Base * pLastHash;
    unsigned uHash;

#if DBG == 1
    int nHashed;
#endif

    ptlsdata->_fPartialRental = true;

    while (1)
    {
        uRentalsInit = ptlsdata->_uRentals;
        uRentalsFinalized = 0;

        // build a fixed size hash table of objects on the rental zero list
        apObjects = ptlsdata->_apZLObjects;
        memset(apObjects, 0, SIZEOFRENTALGCLIST*sizeof(Base*));

#if DBG == 1
        TraceTag((tagZeroListCount, "before partialFreeRentalObjects _uRentals=%d", uRentalsInit)); 
        nHashed = 0;
#endif

        pBase = pFirstHash;
        // this should only be called when there are items on the rental list
        Assert(pBase); 

        // skip first one since we will also skip when we remove items
        pBase = REF_TO_POINTER(pBase->_refs);

        uHash;
        while (pBase)
        {
            ULONG_PTR refs = pBase->_refs;
#ifdef DEBUGRENTAL
            if (!(refs & REF_RENTAL))
                DebugBreak();
#endif
            Assert(refs & REF_RENTAL);

            uHash = RENTALPTRHASH(pBase);
            // stop when we encounter our first collision 
            // (lets hope this doesn't happen too fast!)
            if (apObjects[uHash])
                break;
            Assert(!(((unsigned)pBase) & 7));
            apObjects[uHash] = pBase;

            pBase = REF_TO_POINTER(refs);
#if DBG == 1
            nHashed++;
#endif
        }
        Base * pLastHash = pBase;

        TEB * pteb = ptlsdata->_pTEB;

        INT_PTR * bottom = (INT_PTR *)pteb->StackLimit; // (int *)&context; // (int *)teb->StackLimit;
        // adjust the bottom of the stack by 4K, because the
        // thread environment block for a thread under Win9x 
        // lies about the stack size by this amount
        if (g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) 
        {
            bottom += 0x400;
        }
        INT_PTR * top = (INT_PTR *)pteb->StackBase;

        // walk stack and mark all objects pointed from there
        markRentalStackObjects(bottom, top, apObjects);

        // check context
        CONTEXT context;
        if( GETTHREADCONTEXT(ptlsdata->_hThread, &context) )
        {
            markRentalStackObjects((INT_PTR *)&context, (INT_PTR *)(&context + 1), apObjects);
        }

        // anything left is safe to finalize

        Base * pPrevBase = pFirstHash;
        pBase = REF_TO_POINTER(pPrevBase->_refs);
        Base * pNextBase = null;

FreeObjectsLoop:
        TRY
        {
            // skip first one to simplify insert logic
            while (pBase != pLastHash)
            {
                ULONG_PTR refs = pBase->_refs;
#ifdef DEBUGRENTAL
                if (!(refs & REF_RENTAL))
                    DebugBreak();
#endif
                Assert(!!(refs & REF_RENTAL));

                pNextBase = REF_TO_POINTER(refs);

                uHash = RENTALPTRHASH(pBase);
                // if it is still in the hashtable, we can finalize it
                if (apObjects[uHash])
                {
                    Assert(apObjects[uHash] == pBase);
                    Assert(pNextBase == REF_TO_POINTER(pBase->_refs));
                    pPrevBase->_refs = POINTER_TO_REF(pNextBase) | REF_RENTAL;
                    ptlsdata->_uRentals--;

                    // release outstanding references
                    pBase->finalize();

                    // set reference to 0 for destructor and mark it finalized (we don't want to mark it again) !
                    Assert(pNextBase == REF_TO_POINTER(pBase->_refs));
                    pBase->_refs = REF_MARKED | REF_REFERENCE | REF_RENTAL;

                    // now release weak ref
                    pBase->weakRelease(); // delete r;

                    uRentalsFinalized++;
                }
                else
                    pPrevBase = pBase;

                pBase = pNextBase;
            }
        }
        CATCH
        {
            // BUGBUG? what to do?
            pBase = pNextBase;
            // for now, we just drop whatever went wrong on the floor
            // and continue GCing objects.  This may cause leaks.
            goto FreeObjectsLoop;
        }
        ENDTRY

        if (uRentalsFinalized < 16)
        {
            if (!pLastHash)
                break;
            pFirstHash = pLastHash;
        }
        if (uRentalsInit > ptlsdata->_uRentals)
            break;

        TraceTag((tagZeroListCount, "retry partialFreeRentalObjects _uRentals=%d finalized=%d hashed=%d", 
            ptlsdata->_uRentals, uRentalsFinalized, nHashed)); 
    }
        
    ptlsdata->_fPartialRental = false;

    TraceTag((tagZeroListCount, "after partialFreeRentalObjects _uRentals=%d finalized=%d hashed=%d", 
        ptlsdata->_uRentals, uRentalsFinalized, nHashed)); 
}

void
Base::markRentalStackObjects(INT_PTR * bottom, INT_PTR * top, 
                             Base ** apObjects)
{
    INT_PTR u;
    Base * pBase; 
    TLSDATA * ptlsdata = GetTlsData();

    __try
    {
        while(--top >= bottom)
        {
            u = *top;
            if (u && (u & 3) == 0)
            {
#if NEVER
#if 1
                unsigned uHash = RENTALPTRHASH(u);
                apObjects[uHash] = null;
#else
                unsigned uHash = RENTALPTRHASH(u);
                if (uHash > 0)
                    apObjects[uHash-1] = null;
                apObjects[uHash] = null;
                if (++uHash < SIZEOFRENTALGCLIST)
                    apObjects[uHash] = null;
#endif
#else
                ULONG_PTR r;
                pBase = isObject((void *)u, &r, ptlsdata);
                if (!pBase)
                    continue;
                Assert(pBase->getBase() == pBase);
                ULONG_PTR refs;
                if (r != REF_LOCKED && (r & REF_RENTAL))
                {
                    unsigned uHash = RENTALPTRHASH(pBase);
                    //Assert(0 == apObjects[uHash] || pBase == apObjects[uHash]);
                    apObjects[uHash] = null;
                }
#endif
            }
        }
    }
    __except(GetExceptionCode() == STATUS_STACK_OVERFLOW ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    {
    }
#ifdef UNIX
    _endexcept
#endif // UNIX
}
#endif //FASTRENTALGC


void
Base::freeRentalObjects(TLSDATA * ptlsdata, bool fCheckMarked)
{
    // return if we are already in freeRentalObjects
    if (ptlsdata->_fReleaseRental)
        return;

    TraceTag((tagZeroListCount, "before freeRentalObjects _uRentals=%d fCheckedMarked=%s", 
        ptlsdata->_uRentals, fCheckMarked ? "true" : "false")); 

    Model model(ptlsdata, Rental);

    ptlsdata->_fReleaseRental = true;

    Base * pBase = ptlsdata->_pRentalList;
    Base * pBaseMarked = null;
    bool fCheckSaved = ptlsdata->_fCheckMarked;
    ptlsdata->_fCheckMarked = fCheckMarked;
    ptlsdata->_uRentals = 0;

FreeObjectsLoop:
    TRY
    {
        while ((pBase = ptlsdata->_pRentalList) != null)
        {
            ULONG_PTR refs = pBase->_refs;
    #ifdef DEBUGRENTAL
            if (!(refs & REF_RENTAL))
                DebugBreak();
    #endif

            Assert(refs != REF_LOCKED);
            Assert(!(refs & REF_REFERENCE));

            ptlsdata->_pRentalList = REF_TO_POINTER(refs);
            if (fCheckMarked && pBase->isMarked(refs))
            {
                // cannot free it, link it into marked chain
                pBase->_refs = POINTER_TO_REF(pBaseMarked) | REF_RENTAL;
                pBaseMarked = pBase;
                ptlsdata->_uRentals++;
            }
            else
            {
    #ifdef DEBUGINRETAIL
                if (s_uFinalized < FINALIZEDARRAY)
                {
                    s_pBaseFinalized[s_uFinalized] = pBase;
                    s_lBaseFinalizedRefs[s_uFinalized] = refs;
                    s_pBaseFinalizedVTable[s_uFinalized++] = *(void **)pBase;
                }
                if (pBase == s_pBaseTemp)
                    DebugBreak();
    #endif
                // release outstanding references
                pBase->finalize();
                // set reference to 0 for destructor and mark it finalized (we don't want to mark it again) !
                pBase->_refs = REF_MARKED | REF_REFERENCE | REF_RENTAL;
                // now release weak ref
                pBase->weakRelease(); // delete r;
            }
        }
    }
    CATCH
    {
        // BUGBUG? what to do?
        // for now, we just drop whatever went wrong on the floor
        // and continue GCing objects.  This may cause leaks.
        goto FreeObjectsLoop;
    }
    ENDTRY

    ptlsdata->_pRentalList = pBaseMarked;
    ptlsdata->_fCheckMarked = fCheckSaved;
    ptlsdata->_fReleaseRental = false;

    TraceTag((tagZeroListCount, "after freeRentalObjects _uRentals=%d fCheckedMarked=%s", 
        ptlsdata->_uRentals, fCheckMarked ? "true" : "false")); 
}

#endif // RENTAL_MODEL

Base::Base(CloningEnum)
{
#if DBG == 1
    InterlockedIncrement(&g_cbObjects);
#endif
#ifdef PRFDATA
    PrfCountObjects(+1);
#endif
    TLSDATA * ptlsdata = GetTlsData();
#ifdef DEBUGRENTAL
    _dwTIDCreated = ptlsdata->_dwTID;
    _lModelCreated = ptlsdata->_reModel;
#endif
#ifdef RENTAL_MODEL
    if (ptlsdata->_reModel == Rental)
    {
        addToRentalList(0, ptlsdata);
    }
    else
    {
#endif
#ifdef FAST_OBJECT_LIST
        addToObjectList(ptlsdata);
#else
        // start on the zero count list...
        unLock(POINTER_TO_REF(addToZeroList(ptlsdata)));
        // do the test only if we are not currently in the
        // GC on this thread !
        if (ptlsdata != s_ptlsGC)
            testForGC(0);
#endif
#ifdef RENTAL_MODEL
    }
#endif
}

Base::Base()
{
#if DBG == 1
    InterlockedIncrement(&g_cbObjects);
#endif
#ifdef PRFDATA
    PrfCountObjects(+1);
#endif
    TLSDATA * ptlsdata = GetTlsData();
#ifdef DEBUGRENTAL
    _dwTIDCreated = ptlsdata->_dwTID;
    _lModelCreated = ptlsdata->_reModel;
#endif
#ifdef RENTAL_MODEL
    if (ptlsdata->_reModel == Rental)
    {
        addToRentalList(0, ptlsdata);
    }
    else
    {
#endif
#ifdef FAST_OBJECT_LIST
        addToObjectList(ptlsdata);
#else
        // start on the zero count list...
        unLock(POINTER_TO_REF(addToZeroList(ptlsdata)));
        // do the test only if we are not currently in the
        // GC on this thread !
        if (ptlsdata != s_ptlsGC)
            testForGC(0);
#endif
#ifdef RENTAL_MODEL
    }
#endif
}


Base::Base(NoZeroListEnum)
{
#ifdef DEBUGRENTAL
    _dwTIDCreated = GetTlsData()->_dwTID;
    _lModelCreated = MultiThread;
#endif
#if DBG == 1
    InterlockedIncrement(&g_cbObjects);
#endif
#ifdef PRFDATA
    PrfCountObjects(+1);
#endif
    unLock(REF_REFERENCE);
}


Base::~Base()
{
#ifdef DEBUGINRETAIL
    if (s_uDeleted < DELETEDARRAY)
    {
        s_pBaseDeleted[s_uDeleted] = this;
        s_pBaseDeletedVTable[s_uDeleted++] = *(void **)this;
    }
#endif
#if DBG == 1
    InterlockedDecrement(&g_cbObjects);
#endif
#ifdef PRFDATA
    PrfCountObjects(-1);
#endif
    Assert(_refs == REF_LOCKED || (_refs & REF_REFERENCE && _refs & REF_MARKED));
}

HRESULT Base::QueryInterface(REFIID iid, void ** ppvObject)
{
    if (iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else if (iid == IID_Object)
    {
        // no refcount !
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = null;
        return E_NOINTERFACE;
    }
}

ULONG
Base::_addRef()
{
    ULONG_PTR refs;

#ifdef RENTAL_MODEL
    refs = _refs;
    if (refs != REF_LOCKED && (refs & REF_RENTAL))
    {
#ifdef DEBUGRENTAL
        if (_lModelCreated != Rental)
            DebugBreak();
#endif
        if (refs & REF_REFERENCE)
        {
            Assert("Trying to addref a finalized object !" && (refs >> REF_SHIFT) != 0);
            refs += REF_OFFSET;
            _refs = refs;

            TraceTag((tagRefCount, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));

        #if DBG==1
            if (this == g_pBaseTrace)
            {
                TraceTag((0, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));
                TraceCallers(0, 0, 12);
            }
        #endif
            return PtrToUlong((VOID*)(refs >> REF_SHIFT));
        }
        else
        {
            removeFromRentalList(refs, GetTlsData());

            TraceTag((tagRefCount, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), 1));
    #if DBG==1
            if (this == g_pBaseTrace)
            {
                TraceTag((0, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), 1));
                TraceCallers(0, 0, 12);
            }
    #endif
            return 1;
        }
    }
#endif
again:

    refs = spinLock();

#ifdef DEBUGRENTAL
    if (_lModelCreated == Rental)
        DebugBreak();
#endif
#ifdef RENTAL_MODEL
	// rental objects shouldn't be locked !
	Assert(!(refs & REF_RENTAL));
#endif

    if (refs & REF_REFERENCE)
    {
        Assert("Trying to addref a finalized object !" && (refs >> REF_SHIFT) != 0);
        refs += REF_OFFSET;
    }
    else
    {
        TLSDATA * ptlsdata = GetTlsData();
#ifdef FAST_OBJECT_LIST
        // check it again in case refs was locked in the first check...
        if ((refs >> REF_SHIFT) < SIZEOFOBJECTLIST)
        {
            ptlsdata->_ppObjects[refs >> REF_SHIFT] = null;
            unLock(REF_OFFSET | REF_REFERENCE | (refs & REF_MARKED));
        }
        else
#endif
        {
            // first addref, remove it from zero count list and addref it again...
            refs = removeFromZeroList(refs, ptlsdata);
            unLock(refs);
            if (!(refs & REF_REFERENCE))
            {
                // remove didn't succeed, have to try it again...
    #ifdef DEBUGINRETAIL
                s_pBaseTemp = this;
    #endif
    #ifdef DEBUGINRETAIL
                Print(_T("addref retry\n"));
    #endif
                Sleep(0);
                goto again;
            }
    #if DBG == 1
            InterlockedDecrement(&g_lZeroListCount);
    #endif
            TraceTag((tagRefCount, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), 1));
    #if DBG==1
            if (this == g_pBaseTrace)
            {
                TraceTag((0, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), 1));
                TraceCallers(0, 0, 12);
            }
    #endif

    #ifdef DEBUGINRETAIL
            s_pBaseTemp = null;
    #endif
        }
        return 1;
    }

    TraceTag((tagRefCount, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));

#if DBG==1
    if (this == g_pBaseTrace)
    {
        TraceTag((0, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));
        TraceCallers(0, 0, 12);
    }
#endif

    unLock(refs);

#ifdef DEBUGINRETAIL
    s_pBaseTemp = null;
#endif

    return (ULONG)(refs >> REF_SHIFT);
}

ULONG
Base::_release()
{
    ULONG_PTR refs;
#ifdef RENTAL_MODEL
    refs = _refs;
    if (refs != REF_LOCKED && (refs & REF_RENTAL))
    {
        Assert(refs & REF_REFERENCE);
#ifdef DEBUGRENTAL
        if (_lModelCreated != Rental)
            DebugBreak();
#endif

        refs -= REF_OFFSET;

        TraceTag((tagRefCount, "[%d] %p:%s release %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));

    #if DBG==1
        if (this == g_pBaseTrace)
        {
            TraceTag((0, "[%d] %p:%s release %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));
            TraceCallers(0, 0, 12);
        }
    #endif

        if (!(refs >> REF_SHIFT))
        {
            TLSDATA * ptlsdata = GetTlsData();
            // check if we can finalize the objects here (this is true during freeRentalObjects()
            // and also check the depth of the stack in case finalize() is recursing too deep
            if (ptlsdata->_fReleaseRental && (!ptlsdata->_fCheckMarked || !isMarked(refs)) && ptlsdata->_dwDepth < MAX_FINALIZE_DEPTH)
            {
                Assert(0 == (_refs & REF_MARKED));
                Assert(0 == (refs & REF_MARKED));
#ifdef DEBUGINRETAIL
                if (s_uFinalized < FINALIZEDARRAY)
                {
                    s_pBaseFinalized[s_uFinalized] = this;
                    s_lBaseFinalizedRefs[s_uFinalized] = refs;
                    s_pBaseFinalizedVTable[s_uFinalized++] = *(void **)this;
                }
                if (this == s_pBaseTemp)
                    DebugBreak();
#endif
                // release outstanding references
                ptlsdata->_dwDepth++;
                finalize();
                ptlsdata->_dwDepth--;
                // set reference to 0 for destructor and mark it finalized (we don't want to mark it again) !
                _refs = REF_MARKED | REF_REFERENCE | REF_RENTAL;
                // now release weak ref
                weakRelease(); //    delete this;
            }
            else
            {
#if DBG == 1
                _dwTID = ptlsdata->_dwTID;
                _refs = REF_RENTAL;
#endif
                addToRentalList(refs & REF_MARKED, ptlsdata);
            }
            // return from here because object could be freed already !
            return 0;
        }
        else
        {
            _refs = refs;
            return (ULONG)(refs >> REF_SHIFT);
        }
    }
#endif
    refs = spinLock();

#ifdef DEBUGRENTAL
    if (_lModelCreated == Rental)
        DebugBreak();
#endif
#ifdef RENTAL_MODEL
	// rental objects shouldn't be locked !
	Assert(!(refs & REF_RENTAL));
#endif
    Assert(refs & REF_REFERENCE);

    refs -= REF_OFFSET;
    
    TraceTag((tagRefCount, "[%d] %p:%s release %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));

#if DBG==1
    if (this == g_pBaseTrace)
    {
        TraceTag((0, "[%d] %p:%s release %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));
        TraceCallers(0, 0, 12);
    }
#endif

    if (!(refs >> REF_SHIFT))
    {
        TLSDATA * ptlsdata = GetTlsData();
        // refcount is 0, add it to zero count list
        // if inside zero list check release then we can free it, 
        // there is no reference to it from the stack...
        // and also check the depth of the stack in case finalize() is recursing too deep
        if (s_ptlsCheckZeroList == ptlsdata && !isMarked(refs) && ptlsdata->_dwDepth < MAX_FINALIZE_DEPTH)
        {
            TraceTag((tagRefCount, "[%d] %p:%s deleting", GetTlsData()->_dwTID, this, RTTINAME(*this)));

            Assert(0 == (refs & REF_MARKED));
#ifdef DEBUGINRETAIL
            if (s_uFinalized < FINALIZEDARRAY)
            {
                s_pBaseFinalized[s_uFinalized] = this;
                s_lBaseFinalizedRefs[s_uFinalized] = refs;
                s_pBaseFinalizedVTable[s_uFinalized++] = *(void **)this;
            }
            if (this == s_pBaseTemp)
                DebugBreak();
#endif
            // release outstanding references
            ptlsdata->_dwDepth++;
            finalize();
            ptlsdata->_dwDepth--;
            // set reference to 0 for destructor and mark it finalized (we don't want to mark it again) !
            unLock(REF_MARKED | REF_REFERENCE);
            // now release weak ref
            weakRelease(); //    delete this;
            // return from here because object could be freed already !
            return 0;
        }
        else
        {
            refs = POINTER_TO_REF(addToZeroList(ptlsdata)) | (refs & REF_MARKED);
        }
        unLock(refs);
        // do the test only if we are not currently in the
        // GC on this thread !
        if (ptlsdata != s_ptlsGC)
            testForGC(0);

        return 0;
    }
    unLock(refs);

    return (ULONG)(refs >> REF_SHIFT);
}

ULONG
Base::_qAddRef()
{
    ULONG_PTR refs = spinLock();

    Assert(refs & REF_REFERENCE);

    refs += REF_OFFSET;

    unLock(refs);

    TraceTag((tagRefCount, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));

#if DBG==1
    if (this == g_pBaseTrace)
    {
        TraceTag((0, "[%d] %p:%s addRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));
        TraceCallers(0, 0, 12);
    }
#endif

    return (ULONG)(refs >> REF_SHIFT);
}


ULONG
Base::_qRelease()
{
    ULONG_PTR refs = spinLock();

    Assert(refs & REF_REFERENCE);

    Assert((refs >> REF_SHIFT) != 0);

    refs -= REF_OFFSET;

    TraceTag((tagRefCount, "[%d] %p:%s release %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));

#if DBG==1
    if (this == g_pBaseTrace)
    {
        TraceTag((0, "[%d] %p:%s release %d", GetTlsData()->_dwTID, this, RTTINAME(*this), refs >> REF_SHIFT));
        TraceCallers(0, 0, 12);
    }
#endif
    unLock(refs);

    return (ULONG)(refs >> REF_SHIFT);
}


void 
Base::weakAddRef() 
{ 
    Assert(FALSE && "shouldn't be called"); 
} 

void 
Base::weakRelease() 
{
    Assert((_refs & REF_REFERENCE) && "shouldn't be called"); 
    delete this; 
} 

DEFINE_CLASS_MEMBERS_CLASS(GenericBase, _T("GenericBase"), Base);
// We don't need anything beyond what GenericBase already defines...
//DEFINE_CLASS_MEMBERS_CLASS(HashtableBase, _T("HashtableBase"), GenericBase);

void 
GenericBase::_weakAddRef()
{
#ifdef RENTAL_MODEL
    ULONG_PTR refs = _refs;
    if (refs != REF_LOCKED && (refs & REF_RENTAL))
    {
        _allRefs++;
    }
    else
#endif
    InterlockedIncrement(&_allRefs);
    TraceTag((tagRefCount, "[%d] %p:%s weakAddRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), _allRefs));

#if DBG==1
    if ((Base*)this == g_pBaseTrace)
    {
        TraceTag((0, "[%d] %p:%s weakAddRef %d", GetTlsData()->_dwTID, this, RTTINAME(*this), _allRefs));
        TraceCallers(0, 0, 12);
    }
#endif
}

void
GenericBase::_weakRelease()
{
    TraceTag((tagRefCount, "[%d] %p:%s weakRelease %d", GetTlsData()->_dwTID, this, RTTINAME(*this), _allRefs - 1));

#if DBG==1
    if ((Base*)this == g_pBaseTrace)
    {
        TraceTag((0, "[%d] %p:%s weakRelease %d", GetTlsData()->_dwTID, this, RTTINAME(*this), _allRefs - 1));
        TraceCallers(0, 0, 12);
    }
#endif

#ifdef RENTAL_MODEL
    ULONG_PTR refs = _refs;
    if (refs != REF_LOCKED && (refs & REF_RENTAL))
    {
        if (!--_allRefs)
            delete this;
    }
    else
#endif
    if (!InterlockedDecrement(&_allRefs))
    {
//        Assert("strong ref should be 0" && !_refs);
        delete this;
    }
}

int
Base::isMarked(ULONG_PTR refs)
{
    if (s_fInFreeObjects)
        return FALSE;

    if (refs & REF_MARKED)
        return TRUE;

    for (Base ** ppBaseLocked = _lockedCurrent - 1; ppBaseLocked >= _lockedObjects; ppBaseLocked--)
    {
        if (*ppBaseLocked == this)
            return TRUE;
    }

    return FALSE;
}

void
Base::markStackObjects(INT_PTR * bottom, INT_PTR * top, BOOL fCurrentThread, TLSDATA * ptlsFrozen)
{
    INT_PTR u;
    Base * pBase; 

    __try
    {
        while(--top >= bottom)
        {
            u = *top;
            if (u && (u & 3) == 0)
            {
                ULONG_PTR r;
                pBase = isObject((void *)u, &r, ptlsFrozen);
                if (!pBase)
                    continue;
                Assert(pBase->getBase() == pBase);
                ULONG_PTR refs;
#ifdef RENTAL_MODEL
                if (r != REF_LOCKED && (r & REF_RENTAL))
                {
                    if (!fCurrentThread)
                        continue;
                    // signal the code below that it cannot mark it with the bit REF_MARKED
                    refs = REF_LOCKED;
#ifdef DEBUGRENTAL
                    if (pBase->_lModelCreated != (long)Rental)
                        DebugBreak();
#endif
                }
                else
                {
#endif
                    refs = pBase->tryLock();
#ifdef DEBUGRENTAL
                    if (refs != REF_LOCKED && pBase->_lModelCreated == Rental)
                        DebugBreak();
#endif
#ifdef RENTAL_MODEL
                }
#endif
                if (refs == REF_LOCKED)
                {
#if DBG == 1
                    g_ulLocked++;
#endif
                    // can happen if looking at a thread currently suspended...
                    // if there were too many objects locked, try it later...
                    if (_lockedCurrent == _lockedObjects + LOCKEDSIZE)
                    {
                        _overflowed = true;
                        // there is nothing to unlock, return
                        return;
                    }
                    // remember this because we can't mark it
                    Assert(_lockedCurrent < _lockedObjects + LOCKEDSIZE);
                    *_lockedCurrent++ = pBase;
                    TraceTag((tagRefCount, "[%d] %p:%s found locked on stack", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));
                }
                else
                {
                    Assert(!(refs & REF_RENTAL));
#ifdef DEBUGINRETAIL
                    if (s_uMarked < MARKEDARRAY)
                    {
                        s_pVTable[s_uMarked] = *(void **)pBase;
                        s_lMarkedRefs[s_uMarked] = refs;
                        s_pBaseMarked[s_uMarked++] = pBase;
                    }
#endif
                    if (!(refs & REF_MARKED)) // && (refs & REF_REFERENCE || REF_TO_POINTER(refs)))
                    {
                        if (_markedCurrent < _markedObjects + MARKEDSIZE)
                        {
                            *_markedCurrent++ = pBase;
                            refs |= REF_MARKED;
                        }
                        else
                        {
                            _overflowed = true;
                            // unlock and return
                            pBase->unLock(refs);
                            return;
                        }
                        TraceTag((tagRefCount, "[%d] %p:%s marked on stack when marking", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));
#if DBG == 1
                        g_ulMarked++;
#endif
                    }
                    pBase->unLock(refs);
                }
            }
        }
    }
    __except(GetExceptionCode() == STATUS_STACK_OVERFLOW ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
    {
    }
#ifdef UNIX
    _endexcept
#endif // UNIX
}

//static __int64 start, end, freq;

// to allow disabling garbage collection inside tracetags...

// NOTE: This flag is no longer debug only.  The FiberInputStream
// requires being able to disable garbage collection.
LONG Base::s_lInGC = 0;

void copyZeroList(TLSDATA * ptlsdata, Base * pFrom)
{
    Base * pNext;
    Base * pHead = ZEROLISTHEAD(ptlsdata);
    ULONG_PTR refs = pHead->spinLock();
    Base * pBase = pHead;

    for ( ; ; )
    {
        Assert(!(refs & REF_REFERENCE));
        Assert(!(refs & REF_MARKED));
        pNext = REF_TO_POINTER(refs);
        if (pNext == pHead)
        {
            pBase->unLock(POINTER_TO_REF(pFrom));
            break;
        }
        else
        {
            refs = pNext->spinLock();
#ifdef DEBUGINRETAIL
            if (POINTER_TO_REF(refs) == 0)
                DebugBreak();
#endif
            Assert(POINTER_TO_REF(refs));
            pBase->unLock(POINTER_TO_REF(pNext));
            pBase = pNext;
        }
    }
}

#if DBG == 1
int s_cbTried;
int s_cbCollected;
#endif

extern HANDLE g_hEventGC;

DeclareTag(tagMemory, "memory", "Track memory and objects");
extern ApartmentMutex * g_pMutexAtom;
extern ApartmentMutex * g_pMutexName;

LONG Base::s_lZeroListCount = 0;
LONG Base::s_lLastZeroListCount = 0;
LONG Base::s_lGCFrequency = 2 * PerThreadGCFrequency;
LONG Base::s_lObjectsWaiting = 0;
bool Base::s_fStartedPartialGC = FALSE;
LONG Base::s_lcbAllocated = 0;
LONG Base::s_lcbLastAllocated = 0;
LONG Base::s_lGCAllocated = 2 * PerThreadGCAllocated;

// this is meant as an optimization, thus we don't need an accurate count
// and can avoid the const of an interlocked increment
LONG Base::s_lGCTestEntryCount = 0;

void
Base::reportObjects(LONG lObjects)
{
    s_lObjectsWaiting += lObjects;
}

void
Base::testForGC(DWORD dwFlags)
{
    LONG d = s_lZeroListCount - s_lLastZeroListCount;
    LONG dcb = s_lcbAllocated - s_lcbLastAllocated;

#ifdef DEBUGINRETAIL
    bool fGCForce = false;
    bool fGCFreq = false;
    bool fGCObjWait = false;
    bool fGCAlloc = false;
    if ((fGCForce = (0 != (dwFlags & GC_FORCE))) || 
        (fGCFreq = (d >= s_lGCFrequency)) || 
        (fGCObjWait = (s_lObjectsWaiting >= s_lGCFrequency)) || 
        (fGCAlloc = (dcb > s_lGCAllocated)))
#else
    if ((dwFlags & GC_FORCE) || d >= s_lGCFrequency || s_lObjectsWaiting >= s_lGCFrequency || 
        dcb > s_lGCAllocated)
#endif
    {
        TLSDATA * ptlsCurrent = GetTlsData();

        // don't do anything if this thread is already in GC
        if (ptlsCurrent != s_ptlsGC && 
            !ptlsCurrent->_fReleaseRental &&
            // Also, there is the following potential deadlock to watch out for:
            //
            //  Thread 1                    Thread 2
            //  ----------------------      -----------------------
            //  doing GC                    creating an Atom
            //  g_pMutexGC locked           g_pMutexAtom is locked
            //  cleanup atoms               allocates memory - calls this method
            //  found atom to cleanup       hits GC threshold
            //  MutexLock(g_pMutexAtom)     MutexLock(g_pMutexGC) 
            //
            // So if we enter this method while the g_pMutexAtom is locked, then we
            // just return and DO not do the GC.

            g_pMutexAtom->getOwner() != ptlsCurrent && 
            g_pMutexName->getOwner() != ptlsCurrent)
        {
            MutexLock lock(g_pMutexGC);
#ifdef DEBUGINRETAIL
            Print(L"Starting GC:");
            if (fGCForce) OutputDebugString(L" fGCForce");
            if (fGCFreq) OutputDebugString(L" fGCFreq");
            if (fGCObjWait) OutputDebugString(L" fGCObjWait");
            if (fGCAlloc) OutputDebugString(L" fGCAlloc");
            OutputDebugString(L"\n");
#endif

            s_lLastZeroListCount = s_lZeroListCount;
            s_lcbLastAllocated = s_lcbAllocated;
            s_lObjectsWaiting = 0;

            TraceTag((tagZeroListCount, "testForGC(%d) s_lZeroListCount=%d delta=%d", dwFlags, s_lZeroListCount, d)); 

        // WARNING: please look at checkZeroCountList when you change 
        // this function because it relies on being able to return
        // without doing ANYTHING in case the Name or Atom hashtable
        // mutexes are locked on THIS thread !

            // check if we already started a partial GC 
            if (s_fStartedPartialGC)
            {
                // finish it in case we did...
                checkZeroCountList(dwFlags);
            }
            // don't do anything if there is one in progress
            else if (!s_lInGC)
            {
                s_lInGC++;

                if (dwFlags & GC_FULL)
                {
                    checkZeroCountList(dwFlags);
                }
                else
                {
                    StartGC();
                    // if already on top of the stack and this is there are no
                    // other threads counted finish here !
                    if (ptlsCurrent->_iRunning == 0 && s_lRunning == 0)
                    {
                        lock.Release();
                        FinishGC();
                    }
                }
            }
        }
    }
}

extern TLSDATA * g_ptlsdataExtra;

void
Base::checkZeroCountList(DWORD dwFlags)
{
    TLSDATA * ptlsdata;
    TLSDATA * ptlscurrent;

    CONTEXT context;

#ifdef DEBUGINRETAIL
    s_uFinalized = 0;
    s_uMarked = 0;
    s_uDeleted = 0;
#endif

#if DBG == 1
    g_ulMarked = 0;
    g_ulCollected = 0;
    g_ulOnZeroList = 0;
    g_ulLocked = 0;
#endif

    Assert(s_lInGC);

    TraceTag((tagZeroListCount, "before checkZeroCountList s_lZeroListCount=%d", s_lZeroListCount)); 

    // protect pointer cache
    MutexLock lockPointerCache(g_pMutexFullGC);

    // if this is a recursive entry for these two mutexes
    // we have to come back here later (we can't GC while
    // adding a new entry to either table....)
    if (g_pMutexName->users() > 0 || g_pMutexAtom->users() > 0)
    {
        // if we wanted to finish a partial GC don't touch s_lInGC
        if (!s_fStartedPartialGC)
            s_lInGC--;
#ifdef DEBUGINRETAIL
        Print(_T("checkZeroCountList(): early abort\n"));
#endif
        // we'll try later !
        return;
    }

    s_ulGCCycle++;

    TRY // catch CTRL-C so we can release the mutex.
    {
#ifdef PRFDATA
        PrfCountGC(+1);
#endif
        Assert(s_ptlsCheckZeroList == null);

#if DBG == 1
        s_cbCollected++;
#endif
#ifdef DEBUGINRETAIL
        Print(_T("checkZeroCountList()\n"));
#endif

        _overflowed = false;
        // reset to beginning of mark buffer
        _markedCurrent = _markedObjects;

//      QueryPerformanceCounter((LARGE_INTEGER *)&start);

        ptlscurrent = GetTlsData();

        Assert(ptlscurrent->_iRunning > 0 && "No stack entry on this thread !");

#ifdef FAST_OBJECT_LIST
        // always flush current object list
        flushToZeroList(ptlscurrent);
#endif

        s_ptlsdataGC = g_ptlsdata;

        // cycle thru threads and take ownership of zerolists
        for (ptlsdata = s_ptlsdataGC; ptlsdata; ptlsdata = ptlsdata->_pNext)
        {
            // flush zero list for threads exited...
            if (ptlsdata->_fThreadExited)
            {
                flushToZeroList(ptlsdata);
            }
            // can happen if we are finishing instead of FinishGC()
            if (!ptlsdata->_fLocked)
            {
                Assert(REF_TO_POINTER(ZEROLISTHEADLOCKED(ptlsdata)->_refs) == ZEROLISTHEAD(ptlsdata));
                ZEROLISTHEADLOCKED(ptlsdata)->spinLock();
                ZEROLISTHEADLOCKED(ptlsdata)->unLock(POINTER_TO_REF(lockZeroList(ptlsdata)));
                unLockZeroList(ptlsdata, ZEROLISTHEAD(ptlsdata));
                ptlsdata->_fLocked = true;
            }
            else
            {
                Assert(s_fStartedPartialGC);
                // locked by StartGC(), reset _counted !
                ptlsdata->_fCounted = false;
            }
        }
        
        if (s_fStartedPartialGC)
        {
            // there are too many objects on the zero list to wait for the
            // normal FinishGC, let's scan the stack
            Assert(g_pfnExit == StackExitBlocked);
            g_pfnExit = StackExitNormal;
            s_fStartedPartialGC = FALSE;
        }

        // reset to beginning of lock buffer
        _lockedCurrent = _lockedObjects;

        // update GC frequency based on the number of threads running...
        s_lGCFrequency = PerThreadGCFrequency;

        s_lGCAllocated = PerThreadGCAllocated;

        // cycle thru threads and mark object on the stacks
        for (ptlsdata = s_ptlsdataGC; ptlsdata; ptlsdata = ptlsdata->_pNext)
        {
#ifdef RENTAL_MODEL
            // in rental model we have to prevent other threads running
            // freeRentalObjects() modifying the cache while we are
            // checking the stack and calling isObject()
            extern ShareMutex * g_pMutexPointer;
            MutexLock lock(g_pMutexPointer);
#endif
            if (ptlsdata->_iRunning)
            {
                s_lGCFrequency += PerThreadGCFrequency;
                s_lGCAllocated += PerThreadGCAllocated;
            }
            else
            {
                continue;
            }
            if (ptlsdata != ptlscurrent)
            {
                // if this fails the thread is not alive any more
                DWORD dw = ::SuspendThread(ptlsdata->_hThread);
                if (dw == 0xFFFFFFFF)
                {
#ifdef DEBUGINRETAIL
                    Print(_T("Suspend failed"));
#endif
                    continue;
                }
                // check running again
                if (!ptlsdata->_iRunning)
                {
                    DWORD dw = ::ResumeThread(ptlsdata->_hThread);
                    Assert(dw != 0xFFFFFFFF);
#ifdef DEBUGINRETAIL
                    Print(_T("Skipping after suspend"));
#endif
                    continue;
                }
            }
            ptlsdata->_fSuspended = true;

            TEB * pteb = ptlsdata->_pTEB;

            // don't do this for the current thread when called at
            // the top of the stack...

            if (!((dwFlags & GC_STACKTOP) && ptlsdata == ptlscurrent))
            {
#ifdef UNIX   // UNIX Hack for Solaris
                // The stack grows up on PA-RISC.  For the HP ports we
                // need to swap these around.
#ifdef ux10
                pteb->StackLimit = (int*)MwGetStackBase();               
                pteb->StackBase  = (int*)MwGetCurrentStackPointer();     
#else
                pteb->StackBase  = (int*)MwGetStackBase();               
                pteb->StackLimit = (int*)MwGetCurrentStackPointer();     
#endif
#endif
                INT_PTR * bottom = (INT_PTR *)pteb->StackLimit; // (int *)&context; // (int *)teb->StackLimit;
                // adjust the bottom of the stack by 4K, because the
                // thread environment block for a thread under Win9x 
                // lies about the stack size by this amount
                if (g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) 
                {
                    bottom += 0x400;
                }
                INT_PTR * top = (INT_PTR *)pteb->StackBase;

                // walk stack and mark all objects pointed from there
                markStackObjects(bottom, top, ptlsdata == ptlscurrent, ptlsdata);

                // check context
                context.ContextFlags = CONTEXT_FULL;
                if( GETTHREADCONTEXT(ptlsdata->_hThread, &context) )
                {
                    markStackObjects((INT_PTR *)&context, (INT_PTR *)(&context + 1), ptlsdata == ptlscurrent, ptlsdata);
                }
            }

            if (ptlsdata != ptlscurrent)
            {
                DWORD dw = ::ResumeThread(ptlsdata->_hThread);
                Assert(dw != 0xFFFFFFFF);
            }
            ptlsdata->_fSuspended = false;

            if (_overflowed)
            {
                // BUGBUG: we could resize the arrays here...

                // the marked objects or locked object array overflowed
                // we cannot finish the GC so go to exit, the next
                // partial GC will finish to job
                goto exit;
            }
        }

        s_ptlsGC = s_ptlsCheckZeroList = ptlscurrent;

#ifdef RENTAL_MODEL
        // free rental objects non-marked for this thread
        freeRentalObjects(ptlscurrent, true);
#endif

        // cycle thru threads and release objects on zero lists
        for (ptlsdata = s_ptlsdataGC; ptlsdata; ptlsdata = ptlsdata->_pNext)
        {
            // walk zero count list and delete objects not marked
            FreeObjects(ptlsdata);
        }

// enable/disable hashtable cleanup
#if 1
        if (g_fClassInitCalled)
        {
            {
                // protect Name objects
                MutexLock lockName(g_pMutexName);
                // check Name hashtable
                FreeObjects(Name::s_pNames);
            }

            {
                // protect Atom objects
                MutexLock lockAtom(g_pMutexAtom);
                // check Atom hashtable
                FreeObjects(Atom::atoms);
            }
        }
#endif // enable/disable hashtable cleanup

        s_ptlsGC = s_ptlsCheckZeroList = null;

        // reset to beginning of lock buffer
        _lockedCurrent = _lockedObjects;
    }
    CATCH
    {
        Assert(FALSE && "Exception in garbage collecting !");
        // make sure threads are resumed
        for (ptlsdata = s_ptlsdataGC; ptlsdata; ptlsdata = ptlsdata->_pNext)
        {
            // resume thread
            if (ptlsdata->_fSuspended)
            {
                if (ptlsdata != ptlscurrent)
                {
                    DWORD dw = ::ResumeThread(ptlsdata->_hThread);
                    Assert(dw != 0xFFFFFFFF);
                }

                ptlsdata->_fSuspended = false;
            }
        }
    }
    ENDTRY

exit:

    // unmark objects on normal marked list
    while (_markedCurrent > _markedObjects)
    {
        Base * pBase = *--_markedCurrent;
        ULONG_PTR refs = pBase->spinLock();
        Assert(!(refs & REF_RENTAL));
        pBase->unLock(refs & ~REF_MARKED);
    }

    // swap back zero count lists...
    for (TLSDATA ** pptls = &s_ptlsdataGC; ptlsdata = *pptls; )
    {
        Assert(!ptlsdata->_fSuspended);
        if (ptlsdata->_fLocked)
        {
            // copy over objects added to zero list during GC...
            copyZeroList(ptlsdata, REF_TO_POINTER(ZEROLISTHEADLOCKED(ptlsdata)->spinLock()));
            ZEROLISTHEADLOCKED(ptlsdata)->unLock(POINTER_TO_REF(ZEROLISTHEAD(ptlsdata)));
            ptlsdata->_fLocked = false;
        }
        // if thread exited and there are no more objects on the zero list unlink the structure and delete it
        if (ptlsdata->_fThreadExited && 
            0 == ptlsdata->_uRentals &&
            0 == ptlsdata->_uNextObject &&
            REF_TO_POINTER(ZEROLISTHEAD(ptlsdata)->_refs) == ZEROLISTHEAD(ptlsdata) && 
            ptlsdata != g_ptlsdataExtra &&
            ptlsdata != s_ptlsdataGC)
        {
            *pptls = ptlsdata->_pNext;
            delete ptlsdata;
        }
        else
        {
            pptls = &ptlsdata->_pNext;
        }
    }

    TraceTag((tagMemory, "[%X] collected:%d onzerolist:%d marked:%d locked:%d ", GetTlsData()->_dwTID, g_ulCollected, g_ulOnZeroList, g_ulMarked, g_ulLocked));
    TraceTag((tagMemory, "[%X] after gc objects:%d zerolist:%ld memory:%d documents:%ld components:%ld", GetTlsData()->_dwTID, s_lZeroListCount, g_lZeroListCount, DbgTotalAllocated(), g_lDocumentCount, GetComponentCount()));

#if DBG == 1
    if (g_fMonitor == 1)
    {
        g_fMonitor++;
        DbgDumpMemory();
    }
#endif

    s_lInGC--;

    TraceTag((tagZeroListCount, "after checkZeroCountList s_lZeroListCount=%d", s_lZeroListCount)); 
}

void 
Base::StartFreeObjects()
{
    s_lInGC++;

    s_ptlsGC = s_ptlsCheckZeroList = GetTlsData();

    s_fInFreeObjects = true;
}

void 
Base::FinishFreeObjects()
{
    TraceTag((tagMemory, "[%X] before FreeObjects %d objects %ld zerolist %d memory", GetTlsData()->_dwTID, g_cbObjects, g_lZeroListCount, DbgTotalAllocated()));

#if DBG == 1
    g_ulMarked = 0;
    g_ulCollected = 0;
    g_ulOnZeroList = 0;
    g_ulLocked = 0;
#endif

#if DBG == 1
    if (s_uCount)
        s_uAveLength /= s_uCount;
#endif
    TLSDATA * ptlscurrent = GetTlsData();

    TLSDATA * ptlsdata;
    BOOL fLoop = TRUE;

    Assert(!s_fStartedPartialGC);
    if (s_fStartedPartialGC)
    {
        // there are too many objects on the zero list to wait for the
        // normal FinishGC, let's scan the stack
        Assert(g_pfnExit == StackExitBlocked);
        g_pfnExit = StackExitNormal;
        s_fStartedPartialGC = FALSE;
    }

    s_ulGCCycle++;

    while(fLoop)
    {
        fLoop = FALSE;
        for (ptlsdata = g_ptlsdata; ptlsdata; ptlsdata = ptlsdata->_pNext)
        {
#ifdef FAST_OBJECT_LIST
            Base::flushToZeroList(ptlsdata);
#endif
            // can happen if we are finishing instead of FinishGC()
            if (!ptlsdata->_fLocked)
            {
                Assert(REF_TO_POINTER(ZEROLISTHEADLOCKED(ptlsdata)->_refs) == ZEROLISTHEAD(ptlsdata));
                // swap zero list with locked
                ZEROLISTHEADLOCKED(ptlsdata)->spinLock();
                ZEROLISTHEADLOCKED(ptlsdata)->unLock(POINTER_TO_REF(lockZeroList(ptlsdata)));
                unLockZeroList(ptlsdata, ZEROLISTHEAD(ptlsdata));
            }
            else
            {
                // locked by StartGC(), reset _counted !
                ptlsdata->_fCounted = ptlsdata->_fLocked = false;
            }
            // free all objects on zero list
            fLoop |= FreeObjects(ptlsdata);

            if (g_fClassInitCalled)
            {
                // check Name hashtable
                fLoop |= FreeObjects(Name::s_pNames);
                // check Atom hashtable
                fLoop |= FreeObjects(Atom::atoms);
            }
        }
    }

    if (g_fClassInitCalled)
    {
        Atom::atoms->Release();
        Atom::atoms = null;
        Name::s_pNames->Release();
        Name::s_pNames = null;
    }
    ClearReferences();
#if DBG == 1
    for (ptlsdata = g_ptlsdata; ptlsdata; ptlsdata = ptlsdata->_pNext)
    {
        Assert(ptlsdata->_baseHead._refs == POINTER_TO_REF(ZEROLISTHEAD(ptlsdata)));
        Assert(ptlsdata->_baseHeadLocked._refs == POINTER_TO_REF(ZEROLISTHEAD(ptlsdata)));
    }
#endif

    s_lInGC--;
    s_ptlsGC = s_ptlsCheckZeroList = null;

    TraceTag((tagMemory, "[%X] collected:%d onzerolist:%d marked:%d locked:%d ", GetTlsData()->_dwTID, g_ulCollected, g_ulOnZeroList, g_ulMarked, g_ulLocked));
    TraceTag((tagMemory, "[%X] after gc %d objects %ld zerolist %d memory", GetTlsData()->_dwTID, g_cbObjects, g_lZeroListCount, DbgTotalAllocated()));
}

// free objects in tls, return TRUE if there is more left
BOOL Base::FreeObjects(TLSDATA * ptlsdata)
{
    Base * pHead = ZEROLISTHEADLOCKED(ptlsdata);
    Base * pPrev = pHead;
    ULONG_PTR refs = pHead->spinLock();
    Base * pBase = REF_TO_POINTER(refs);

FreeObjectsLoop:
    TRY
    {
        for ( ; pBase != ZEROLISTHEAD(ptlsdata); )
        {
            TraceTag((tagRefCount, "[%d] %p:%s checking on zerolist", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));
#if DBG == 1
            g_ulOnZeroList++;
#endif

            refs = pBase->tryLock();

            if (refs == REF_LOCKED)
            {
#if DBG == 1
                g_ulLocked++;
#endif
                // can happen if looking at a thread currently suspended...
                TraceTag((tagRefCount, "[%d] %p:%s locked on zerolist", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));
                // give up here, leave pBase in zero list
                refs = POINTER_TO_REF(pBase);
                break;
            }
            else
            {
                Assert(!(refs & REF_REFERENCE));

                if (pBase->isMarked(refs))
                {
                    // if object is marked, cannot be deleted...
                    pPrev->unLock(POINTER_TO_REF(pBase));

                    // leave if unlocked
                    pPrev = pBase;
                }
                else
                {
                    // remove it from zero list by leaving pPrev as it is ...

#if DBG == 1
                    g_ulCollected++;
#endif
                    TraceTag((tagRefCount, "[%d] %p:%s deleting", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));
#ifdef DEBUGINRETAIL
                    if (s_uFinalized < FINALIZEDARRAY)
                    {
                        s_pBaseFinalized[s_uFinalized] = pBase;
                        s_lBaseFinalizedRefs[s_uFinalized] = refs;
                        s_pBaseFinalizedVTable[s_uFinalized++] = *(void **)pBase;
                    }
                    if (pBase == s_pBaseTemp)
                        DebugBreak();
#endif
#if DBG == 1
                    InterlockedDecrement(&g_lZeroListCount);
#endif
                    s_lZeroListCount--;
                    TraceTag((tagZeroListCount, "%p : --s_lZeroListCount = %d : FreeObjects(TLSDATA * ptlsdata)", pBase, s_lZeroListCount));

                    // release outstanding references
                    pBase->finalize();
                    // set reference to 0 for destructor and mark it finalized (we don't want to mark it again) !
                    pBase->unLock(REF_MARKED | REF_REFERENCE);
                    // now release weak ref
                    pBase->weakRelease(); // delete r;
                }

                // advance now, so that any exceptions will
                // leave us in a state where we can continue
                pBase = REF_TO_POINTER(refs);

                Assert(pPrev->_refs == REF_LOCKED);
            }
        }
    }
    CATCH
    {
        // advance to next object
        pBase = REF_TO_POINTER(refs);

        // BUGBUG? what to do?
        // for now, we just drop whatever went wrong on the floor
        // and continue GCing objects.  This may cause leaks.
        goto FreeObjectsLoop;
    }
    ENDTRY
    s_lLastZeroListCount = s_lZeroListCount;
    Assert(pPrev->_refs == REF_LOCKED);
    pPrev->unLock(refs);
    return ptlsdata->_baseHead._refs != POINTER_TO_REF(ZEROLISTHEAD(ptlsdata));
}

// free objects in hashtable, return true if we actually GC'd something.
BOOL Base::FreeObjects(Hashtable * pht)
{
    ULONG ul = 0;
    int i = 0;

FreeObjectsLoop:
    TRY
    {
        for (; i < pht->_table->length();)
        {
            HashEntry& entry = (*pht->_table)[i];
            if (entry.isOccupied())
            {
                Base * pBase = (Base *)entry.value();

                ULONG_PTR refs = pBase->tryLock();

                if (refs == REF_LOCKED)
                {
#if DBG == 1
                    g_ulLocked++;
#endif
                    // can happen if looking at a thread currently suspended...
                    TraceTag((tagRefCount, "[%d] %p:%s locked on zerolist", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));
                }
                else
                {
#ifdef STRONGREFATOM
                    if (((refs >> REF_SHIFT) != 1) || 
                        pBase->isMarked(refs) ||
                        ((HashtableBase *)pBase)->wasGCWarned())
#else
                    if (((refs >> REF_SHIFT) != 0) || 
                        pBase->isMarked(refs) ||
                        ((HashtableBase *)pBase)->wasGCWarned())
#endif
                    {
                        // still used, leave it there
                        pBase->unLock(refs);
                    }
                    else
                    {
                        // call finalize on it which will remove it from the hashtable...
#if DBG == 1
                        g_ulCollected++;
#endif
                        TraceTag((tagRefCount, "[%d] %p:%s deleting", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));
#ifdef DEBUGINRETAIL
                        if (s_uFinalized < FINALIZEDARRAY)
                        {
                            s_pBaseFinalized[s_uFinalized] = pBase;
                            s_lBaseFinalizedRefs[s_uFinalized] = refs;
                            s_pBaseFinalizedVTable[s_uFinalized++] = *(void **)pBase;
                        }
                        if (pBase == s_pBaseTemp)
                            DebugBreak();
#endif

                        // count how many objects we remove
                        ul++;

#ifdef STRONGREFATOM
                        // BUGBUG: this is moderately scary.  It would be better, for error checking
                        // to not have to unLock this until after we have removed it, but in order
                        // to use a normal hashtable, we need to ulLock it so that the hashtable 
                        // can Release() it.

                        // set reference to 1 for destructor and mark it (we don't want to mark it again) !
                        pBase->unLock(REF_OFFSET | REF_MARKED | REF_REFERENCE);

                        // this removes item from hashtable!!
                        ((HashtableBase *)pBase)->removeFromHashtable();

                        // finalize expects it to be spinlocked
                        pBase->spinLock();
#else
                        // this removes item from hashtable!!
                        ((HashtableBase *)pBase)->removeFromHashtable();

                        pBase->unLock(REF_MARKED | REF_REFERENCE);
#endif

                        pBase->finalize();

                        // now release weak ref
                        pBase->weakRelease(); // delete r;
                    }
                }
            }
            ++i;
        }
    }
    CATCH
    {
        ++i;
        // BUGBUG? what to do?
        // for now, we just drop whatever went wrong on the floor
        // and continue GCing objects.  This may cause leaks.
        goto FreeObjectsLoop;
    }
    ENDTRY

    return ul;
}

// count of threads GC is waiting on to exit
LONG Base::s_lRunning;

TLSDATA * 
Base::StackEntryNormal()
{
    TLSDATA * ptlsdata = EnsureTlsData();
    if (ptlsdata)
    {
        ptlsdata->_iRunning++;
    }
    return ptlsdata;
}

void 
Base::StackExitNormal(TLSDATA * ptlsdata)
{
#ifdef RENTAL_MODEL
    if (ptlsdata)
    {
        if (ptlsdata->_iRunning == 0)
        {
            if (ptlsdata->_pRentalList)
                freeRentalObjects(ptlsdata, false);
        }
    }
#endif
}

TLSDATA * 
Base::StackEntryBlocked()
{
    // this way we no that if _iRunning is NOT set the thread is definitely not running inside
    TLSDATA * ptlsdata = StackEntryNormal();
    WaitForSingleObject(g_hEventGC, INFINITE);
    return ptlsdata;
}

void 
Base::StackExitBlocked(TLSDATA * ptlsdata)
{
    WaitForSingleObject(g_hEventGC, INFINITE);
    if (ptlsdata)
    {
        if (ptlsdata->_iRunning == 0 && !ptlsdata->_fReleaseRental)
        {
#ifdef RENTAL_MODEL
            if (ptlsdata->_pRentalList)
                freeRentalObjects(ptlsdata, false);
#endif
            if (ptlsdata->_fCounted)
            {
#ifdef FAST_OBJECT_LIST
                // always flush object list here
                flushToZeroList(ptlsdata);
#endif
                ptlsdata->_fCounted = false;
                if (InterlockedDecrement(&s_lRunning) == 0)
                {
                    Base::FinishGC();
                    ptlsdata->_iRunning++;
                    // test for objects on zero list
                    testForGC(GC_STACKTOP | GC_FULL);
                    ptlsdata->_iRunning--;
                }
            }
        }
    }
}

void 
Base::StartGC()
{
    TLSDATA * ptlsdata;
#ifdef FAST_OBJECT_LIST
    TLSDATA * ptlsCurrent = GetTlsData();
#endif
    TraceTag((tagZeroListCount, "StartGC s_lZeroListCount=%d", s_lZeroListCount)); 

    s_fStartedPartialGC = TRUE;
    s_lRunning = 0;

    // set block for stack entry and exit
    ResetEvent(g_hEventGC);

    // set function pointers
    g_pfnEntry = StackEntryBlocked;
    g_pfnExit = StackExitBlocked;

#ifdef FAST_OBJECT_LIST
    // always flush object list here
    flushToZeroList(ptlsCurrent);
#endif

    s_ptlsdataGC = g_ptlsdata;

    for (ptlsdata = s_ptlsdataGC; ptlsdata; ptlsdata = ptlsdata->_pNext)
    {
        // flush zero list for threads exited...
        if (ptlsdata->_fThreadExited)
        {
            flushToZeroList(ptlsdata);
        }
        // swap out zero list
        Assert(REF_TO_POINTER(ZEROLISTHEADLOCKED(ptlsdata)->_refs) == ZEROLISTHEAD(ptlsdata));
        ZEROLISTHEADLOCKED(ptlsdata)->spinLock();
        ZEROLISTHEADLOCKED(ptlsdata)->unLock(POINTER_TO_REF(lockZeroList(ptlsdata)));
        unLockZeroList(ptlsdata, ZEROLISTHEAD(ptlsdata));
        ptlsdata->_fLocked = true;
        // count threads running 
        if (ptlsdata->_iRunning)
        {
            s_lRunning++;
            // mark thread so it decrements running count at exit
            ptlsdata->_fCounted = true;
        }
    }

    // unblock threads
    g_pfnEntry = StackEntryNormal;
    SetEvent(g_hEventGC);
}


void 
Base::FinishGC()
{
    TLSDATA * ptlsdata;
    // how many items stayed on the zero list
    long lZeroListRemainCount;

    // lock out new TLSDATA coming in
    MutexLock lock(g_pMutexGC);

    // check if checkZeroCountList already did the job !
    if (!s_fStartedPartialGC)
    {
        Assert(g_pfnExit == StackExitNormal);
        return;
    }

    TraceTag((tagZeroListCount, "before FinishGC s_lZeroListCount=%d", s_lZeroListCount)); 

    Assert(s_lInGC);

    s_ptlsGC = GetTlsData();

    g_pfnExit = StackExitNormal;
    Assert(s_fStartedPartialGC);
    s_fStartedPartialGC = FALSE;
    lZeroListRemainCount = s_lZeroListCount;

    for (TLSDATA ** pptls = &s_ptlsdataGC; ptlsdata = *pptls; )
    {
        // free secondary zero lists
        Base * pHead = ZEROLISTHEADLOCKED(ptlsdata);
        ULONG_PTR refs = pHead->_refs;
        Base * pBase = REF_TO_POINTER(refs);
FreeObjectsLoop:
        TRY
        {
            for ( ; pBase != ZEROLISTHEAD(ptlsdata); )
            {
                TraceTag((tagRefCount, "[%d] %p:%s checking on zerolist", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));

                refs = pBase->_refs;

                Assert(refs != REF_LOCKED);
                Assert(!(refs & REF_REFERENCE));

                TraceTag((tagRefCount, "[%d] %p:%s deleting", GetTlsData()->_dwTID, pBase, RTTINAME(*pBase)));
    #ifdef DEBUGINRETAIL
                if (s_uFinalized < FINALIZEDARRAY)
                {
                    s_pBaseFinalized[s_uFinalized] = pBase;
                    s_lBaseFinalizedRefs[s_uFinalized] = refs;
                    s_pBaseFinalizedVTable[s_uFinalized++] = *(void **)pBase;
                }
                if (pBase == s_pBaseTemp)
                    DebugBreak();
    #endif
                s_lZeroListCount--;
                lZeroListRemainCount--;

                TraceTag((tagZeroListCount, "%p : --s_lZeroListCount = %d : FinishGC()", pBase, s_lZeroListCount));

                // release outstanding references
                pBase->finalize();
                // set reference to 0 for destructor and mark it finalized (we don't want to mark it again) !
                pBase->_refs = REF_MARKED | REF_REFERENCE;
                // now release weak ref
                pBase->weakRelease(); // delete r;

                pBase = REF_TO_POINTER(refs);
            }
        }
        CATCH
        {
            // Just drop whatever went wrong on the floor
            // and continue GCing objects.  This may cause leaks.
            pBase = REF_TO_POINTER(refs);
            goto FreeObjectsLoop;
        }
        ENDTRY
        Assert(REF_TO_POINTER(refs) == ZEROLISTHEAD(ptlsdata));
        pHead->_refs = refs;
        ptlsdata->_fLocked = false;
        // if thread exited unlink the structure and delete it
        if (ptlsdata->_fThreadExited && 
            0 == ptlsdata->_uRentals &&
            0 == ptlsdata->_uNextObject &&
            REF_TO_POINTER(ZEROLISTHEAD(ptlsdata)->_refs) == ZEROLISTHEAD(ptlsdata) && 
            ptlsdata != g_ptlsdataExtra &&
            ptlsdata != s_ptlsdataGC)
        {
            *pptls = ptlsdata->_pNext;
            delete ptlsdata;
        }
        else
        {
            pptls = &ptlsdata->_pNext;
        }
    }

    s_lInGC--;
    s_ptlsGC = null;
    s_lLastZeroListCount = lZeroListRemainCount;

    TraceTag((tagZeroListCount, "after FinishGC s_lZeroListCount=%d", s_lZeroListCount)); 
}


